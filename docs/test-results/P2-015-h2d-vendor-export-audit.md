# P2-015 H2D vendor export audit

This report records the resolved Bambu Studio H2D reference for the two-body fixture. P2-016 regenerated both exports with Textured PEI Plate selected explicitly because the original CLI files recorded Cool Plate. It reran the audit with explicit feature tags and both endpoints of each counted extrusion move. The results below supersede the initial position-only estimates.

## Reference inputs

- **Slicer** — Bambu Studio 02.09.01.70, source revision [`56a57d00a5`](https://github.com/bambulab/BambuStudio/tree/56a57d00a591360cf41bf21383915ee5bf357be5). The SHA-256 of the [Windows portable ZIP](https://github.com/bambulab/BambuStudio/releases/tag/v02.09.01.70) matched the published digest `a4c0d5f94109cb2c2c07fb02ca1f740e96b586fbf3ae557a1647eeb5503a15b8`.
- **Presets** — Bambu Lab H2D 0.4 nozzle, 0.20mm Standard @BBL H2D, and Bambu PLA Basic @BBL H2D in both slots. Manual filament map `1,2`, Textured PEI Plate, prime tower enabled.
- **Geometry** — Two parts of one assembly, 24 × 12 × 4 mm total. The lower-X body spans X = 163–173 mm and the upper-X body spans X = 173–187 mm. Both span Y = 154–166 mm and Z = 0–4 mm. The geometry matches the [STEP fixture](../../tests/data/planar_gradient/SharedFace.step) after translation.
- **Assignments** — AB uses slots 1 and 2 on the lower- and upper-X bodies respectively. BA reverses the part assignments and keeps the same filament map.

## Checked output

| Export | Lower-X / upper-X tool | 0→1 / 1→0 changes | Model XY extrusion (mm E) | Prime tower XY extrusion (mm E) |
| --- | --- | --- | ---: | ---: |
| AB | T0 / T1 | 10 / 10 | 305.49 | 425.05 |
| BA | T1 / T0 | 10 / 10 | 305.49 | 425.05 |

Both tools deposited model material on all 20 layers. For every counted positive-E model or prime tower XY move, both endpoints were within the relevant component or the shared XY rectangle. Each export contained 20 in-layer change blocks with matching material-change request, tool selection and completion. The exporter reported old and new nozzle indices that matched the software map. Startup, wipe travel, extrusion-only recovery moves and shutdown are outside these endpoint metrics.

Each export requested 0 mm in all 40 in-layer flush-length fields. The prime tower still commanded 425.05 mm E across its XY moves, about 139% of the counted model E. This is a slicer-output measurement of tower deposition. It includes tower structure and priming, and physical purge remains unmeasured. The vendor estimated about 164 s under its Flush feature for each export, so a zero requested length does not imply zero change time.

The explicit tool arguments used `H-1`. In this exporter version, the hotend argument becomes `-1` when dynamic nozzle mapping is disabled. The tool IDs, mapping metadata and nozzle annotations establish the logical assignments in these two exports. Physical left/right mapping still needs confirmation for the target machine and firmware. [Bambu Studio exporter source](https://github.com/bambulab/BambuStudio/blob/56a57d00a591360cf41bf21383915ee5bf357be5/src/libslic3r/GCode.cpp)

3DSpectrum's offline profile still emits generic tool selection and has the prime tower disabled in this comparison. The four focused 3DSpectrum cases passed, including the H2D invariant checks and the shared-face STEP slice. The standalone suite passed 9 of 9.

## Recheck the captured files

The Textured PEI exports and CLI result metadata are under ignored `build-phase0/p2-015-vendor/out-textured-ab/` and `out-textured-ba/`. The executable G-code stays in the local build directory.

| Raw file | SHA-256 |
| --- | --- |
| AB `plate_1.gcode` | `fb9be38bc0a9b64e1a315ff3e351527bfc1a41fdb2f552c3b9592d51c9119368` |
| BA `plate_1.gcode` | `45c93faff30878de61bd1cf60b99b507107db047771c52b3ed97e3c78298c7a1` |

```powershell
$reference = 'build-phase0/p2-015-vendor'
node scripts/audit_h2d_reference.mjs "$reference/out-textured-ab/plate_1.gcode" "$reference/out-textured-ba/plate_1.gcode"
```

For regeneration, use the portable CLI with an isolated data directory and resolved machine, process and filament presets. The assembly list references two STL boxes at the coordinates above with count 1, the same assembly index, and the specified filament slots. Slice with `--arrange 0 --slice 0 --curr-bed-type "Textured PEI Plate" --filament-map-mode Manual --filament-map 1,2` and export a sliced 3MF. [Bambu Studio CLI usage](https://github.com/bambulab/BambuStudio/wiki/Command-Line-Usage/0a74eea5e1b11e3b8595fd2846d1050fafc43eb4)

The script is specific to these reference inputs. It rejected reversed file order, a mismatched change completion, an extrusion beyond the assigned component, and the original files with the unexpected plate. The next qualification gate is the target H2D firmware and installed hotend and feed configuration, followed by operator-reviewed machine-specific G-code before any printer motion.
