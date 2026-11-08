// Read-only audit for the P2-015 Bambu Studio H2D shared-face reference.
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { readFileSync } from 'node:fs';

const modelRoles = new Set([
    'Bottom surface', 'Bridge', 'Floating vertical shell', 'Inner wall',
    'Internal solid infill', 'Outer wall', 'Sparse infill', 'Top surface'
]);
const fixture = [[163, 173], [173, 187]];

function audit(file, slots) {
    const data = readFileSync(file);
    const lines = data.toString('utf8').split(/\r?\n/);
    for (const header of [
        '; BambuStudio 02.09.01.70', '; printer_model = Bambu Lab H2D',
        '; curr_bed_type = Textured PEI Plate',
        '; filament_map = 1,2', '; filament_map_2 = 0,1'
    ])
        assert(lines.includes(header), `${file}: expected ${header}`);
    const result = {
        sha256: createHash('sha256').update(data).digest('hex'),
        layers: 0, changes: { '0→1': 0, '1→0': 0 }, flushLengthsMm: [],
        model: [{ moves: 0, filamentMm: 0 }, { moves: 0, filamentMm: 0 }],
        tower: { moves: 0, filamentMm: 0 }
    };
    const position = { X: NaN, Y: NaN, Z: NaN };
    const layerTools = [];
    let activeTool, change, role;
    let printing = false;
    let relativeXYZ = false;
    let relativeE = false;
    let flush = false;

    for (const [index, line] of lines.entries()) {
        const at = `${file}:${index + 1}`;
        if (line === '; CHANGE_LAYER') {
            printing = true;
            ++result.layers;
            layerTools.push(new Set());
        } else if (line.startsWith('; FEATURE: ')) {
            role = line.slice(11);
            if (role === 'Custom')
                printing = false;
        } else if (line === '; CP TOOLCHANGE START') {
            assert(printing && !change && activeTool !== undefined, at);
            change = { from: activeTool, steps: [] };
        } else if (line === '; CP TOOLCHANGE END') {
            assert(change && activeTool !== change.from && !flush, at);
            assert.deepEqual(change.steps, ['open', 'select', 'close'], at);
            assert.deepEqual(change.nozzles, [change.from, activeTool, change.from, activeTool], at);
            ++result.changes[`${change.from}→${activeTool}`];
            change = undefined;
        } else if (line.startsWith('; NOZZLE_CHANGE_START ')) {
            const nozzle = line.match(/^; NOZZLE_CHANGE_START OF([01]) NF([01]) ON([01]) NN([01])$/);
            assert(change && nozzle && !change.nozzles, at);
            change.nozzles = nozzle.slice(1).map(Number);
        } else if (line === '; VFLUSH_START' || line === '; VFLUSH_END') {
            flush = line === '; VFLUSH_START';
        }

        const command = line.split(';', 1)[0].trim();
        const tokens = command.split(/\s+/);
        const opcode = tokens[0];
        const tool = opcode.match(/^T([01])$/);
        if (change && opcode === 'M620' && /^S[01]A$/.test(tokens[1])) {
            assert.equal(tokens[2], 'H-1', at);
            change.to = Number(tokens[1][1]);
            change.steps.push('open');
        } else if (tool) {
            activeTool = Number(tool[1]);
            if (change) {
                assert.equal(activeTool, change.to, at);
                assert.equal(tokens[1], 'H-1', at);
                change.steps.push('select');
            }
        } else if (change && opcode === 'M621' && /^S[01]A$/.test(tokens[1])) {
            assert.equal(Number(tokens[1][1]), change.to, at);
            change.steps.push('close');
        } else if (change && opcode === 'M620.10') {
            const length = tokens.find(token => /^L-?[\d.]+$/.test(token));
            if (length) {
                const value = Number(length.slice(1));
                assert(Number.isFinite(value) && value >= 0, at);
                result.flushLengthsMm.push(value);
            }
        }

        if (opcode === 'G90' || opcode === 'G91')
            relativeXYZ = opcode === 'G91';
        if (opcode === 'M82' || opcode === 'M83')
            relativeE = opcode === 'M83';
        if (opcode === 'G92' && printing)
            assert(!tokens.slice(1).some(token => /^[XYZ]/.test(token)), at);
        if (!['G0', 'G1', 'G2', 'G3'].includes(opcode))
            continue;

        const before = { ...position };
        const words = Object.fromEntries(tokens.slice(1).map(token => [token[0], Number(token.slice(1))]));
        for (const axis of ['X', 'Y', 'Z'])
            if (axis in words)
                position[axis] = (relativeXYZ ? position[axis] : 0) + words[axis];

        const model = modelRoles.has(role);
        if (!printing || flush || (!model && role !== 'Prime tower') ||
            !('E' in words) || !('X' in words || 'Y' in words))
            continue;
        assert(relativeE && Number.isFinite(words.E), at);
        if (words.E <= 0 || (before.X === position.X && before.Y === position.Y))
            continue;
        assert(!relativeXYZ && opcode === 'G1' && activeTool !== undefined, at);

        const side = slots.indexOf(activeTool);
        const [xMin, xMax] = model ? fixture[side] : [25, 325];
        const [yMin, yMax] = model ? [154, 166] : [0, 320];
        for (const { X, Y, Z } of [before, position]) {
            assert(X >= xMin && X <= xMax && Y >= yMin && Y <= yMax && Z >= 0 && Z <= 4.001, at);
        }
        const metric = model ? result.model[side] : result.tower;
        ++metric.moves;
        metric.filamentMm += words.E;
        if (model)
            layerTools.at(-1).add(activeTool);
    }

    assert(!change && !flush && result.layers === 20, file);
    assert(layerTools.every(tools => tools.size === 2), file);
    assert.deepEqual(Object.values(result.changes), [10, 10], file);
    assert.equal(result.flushLengthsMm.length, 40, file);
    result.flushLengthsMm = [...new Set(result.flushLengthsMm)];
    assert(result.tower.moves > 0, file);
    return result;
}

assert.equal(process.argv.length, 4, 'Usage: node scripts/audit_h2d_reference.mjs AB.gcode BA.gcode');
const [ab, ba] = process.argv.slice(2);
console.log(JSON.stringify({ ab: audit(ab, [0, 1]), ba: audit(ba, [1, 0]) }, null, 2));
