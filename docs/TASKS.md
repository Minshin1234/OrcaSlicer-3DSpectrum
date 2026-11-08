# Task backlog

| ID | Status | Objective and expected area | Acceptance evidence |
| --- | --- | --- | --- |
| P0-001 | Done | Document local provenance, source map, risks and cross-agent workflow. | Source paths, checked remote refs, decisions and handoff present. |
| P0-002 | Done | Establish dependency, application and Catch2 baseline through CMake. | Release app and five existing test targets built. CTest ran 385 cases with 377 passes and eight reproducible failures. |
| P0-003 | Done | Reconcile the source snapshot with the configured GitHub repository. | Published the project delta on the existing history as `0c59c8fb78`. [Agent instructions](../AGENTS.md#stage-and-push-major-changes) define how to push future major changes for review. |
| P0-004 | Open | Triage the eight Windows baseline failures before establishing a clean regression gate. | Reproduce the cases and document causes or verified fixes. |
| P1-001 | Done | Add pure C++17 planar fraction sampler and generated two-cuboid fixture in a standalone testable target. | Known relative bounds preserved. Tests cover center, transition edges, monotonicity, sharp split, invalid width and non-finite inputs. |
| P1-002 | Done | Add deterministic ordered 3D threshold assignment over occupied cells. | Identical input gives identical labels. Endpoints remain pure, no labels outside occupancy, and uniform fractions have a documented complete-period error bound. |
| P1-003 | Done | Produce a reproducible preview and a material error report. | XY and XZ views show both materials across multiple layers. Report overall and windowed fraction error, pitches and measured runtime for a fixed fixture. |
| P2-001 | Done | Audit 3MF and STEP placement with asymmetric multi-body fixtures. | Relative world-space positions and independent material IDs survive import and 3MF round trip. Check separate STL placement handling. |
| P2-002 | Prototype complete | Compare ordered material volumes and facet paint through existing toolpaths. | Region coverage, components, support footprints, tool IDs, switch counts and the solid control were checked offline. |
| P2-003 | Experimental adapter complete | Route explicit opt-in early regions through production slicing for a grid-aligned rectangle. | Direct and reference region checks passed. Removal restored ordinary slicing in the same print. The small-pitch input was rejected. |
| P2-004 | Offline audit complete, profile open | Qualify project persistence and toolpaths for the direct region adapter. | Native 3MF persistence and generic path metrics are recorded. Target-profile acceptance and mitigation of continuity and footprint findings remain open. |
| P2-005 | Offline profile complete | Compare upstream H2D implementations and create a local dual-0.4 mm profile for offline slicing. | Strict JSON load and two-tool slicing passed. Hardware-specific routing and macros remain separate. |
| P2-006 | Sharp-cut prototype complete | Add an adjustable planar material split to the Prepare Cut tool. | The desktop check verified plane placement and two separately assigned parts under one object. This prototype is separate from the gradual transition requested for P2-007. |
| P2-007 | Implemented, offline verification | Apply a volumetric A-to-B transition centered on an adjustable plane inside one intact model. | Horizontal and tilted 8 mm fixtures preserved the sliced envelope, achieved the target fractions within 0.8 percentage points across seven bands, and reverted to one filament on removal. Native 3MF preserved the plane, width, slots and placement. |
| P2-008 | Offline audit passed | Audit the exported tool commands and deposition coordinates. | Raw T0 and T1 commands followed slot assignments, including reversed slots. Both extrusion endpoints passed the band and shared reach checks. H2D physical nozzle mapping remains a separate qualification. |
| P2-009 | Offline verification passed | Expose XY direction controls and qualify a diagonal gradient within a layer. | X and Y turn the gradient across the bed. The diagonal XY fixture used both tools on all 100 model layers. The Windows X direction preview sliced successfully. |
| P2-010 | Offline fixture passed | Qualify an explicitly selected planar interface between two CAD bodies. | The translated, rotated STEP fixture retained both solids, sliced a continuous envelope, and used both tools on all 20 layers. Gap, overlap, partial contact and shifted plane fixtures were rejected. |
| P2-011 | Desktop and offline fixture passed | Align the material transition to a fully shared CAD face on demand. | The Windows app aligned the STEP contact, restored the applied 8 mm band when reopened, retained both CAD bodies, and previewed both filaments. Automated checks cover instance transforms, reversed part order and invalid contacts. |
| P2-012 | Baseline recorded | Preserve the planar transition manufacturability baseline. | Pin the source revision, fixtures, test evidence and desktop preview in the [engineering log](DOCUMENTATION.md#entry-p2-20261107-13--planar-transition-manufacturability-baseline). |
| P2-013 | Benchmark complete | Compare toolpath optimization candidates before H2D qualification. | [Grid-pitch benchmark](test-results/P2-013-toolpath-benchmark.md) covers three widths and three orientations with an ordinary control. The coarser pitch reduced short moves but failed the 4 mm diagonal outer-wall check. The production pitch stays at 1.2 mm. |
| P2-014 | Offline checks passed | Compare the H2D command contract and resolve the offline acceleration warning. | The [qualification report](test-results/P2-014-h2d-offline-qualification.md) records the source comparison and a tested companion process. A resolved vendor export for the target firmware is needed before native G-code qualification. |
| P2-015 | Offline audit passed | Audit resolved Bambu Studio H2D exports against 3DSpectrum offline toolpaths. | Bambu Studio 02.09.01.70 exported both slot assignments on the 24 × 12 × 4 mm assembly. The offline regression passed. |
| P2-016 | Offline checks passed | Verify the H2D reference pair with explicit motion and feature checks. | The [report](test-results/P2-015-h2d-vendor-export-audit.md) pins both file hashes and separates logical routing, tower extrusion and requested flush. The audit rejected four inputs with plate, routing, sequence or coordinate errors. |
| P2-017 | Offline checks passed | Import separate STEP files into one assembly and append parts in a shared CAD frame. | Explicit shared-coordinate choice, independent names and filament slots, preservation across native 3MF, safe fallback when the assembly frame is ambiguous. |
| P2-018 | Next | Add finite manual plane size, fit, and independent profile controls. | Exact width and height, resize handles, rotated object fit, full-object compatibility, bounded material effect, and native 3MF round trip. |
| P2-019 | Planned | Select a shared planar interface by component pair and use their filament assignments. | Explicit opt-in, asymmetric band extents and curve profile, unchanged envelope, and reassignment after a component filament changes. |
| P2-020 | Planned | Follow partial planar and curved CAD interfaces. | Validate contact geometry beyond bounds, test cylindrical and embedded contacts, and reject gaps and ambiguous overlaps. |
| P2-021 | Planned | Save multiple independently configured interfaces in larger assemblies. | Three-body and repeated-material fixtures, deterministic overlap handling, and ordinary slicing for untouched contacts. |
| P2-022 | Planned | Benchmark and improve assembly toolpaths. | Compare fraction error, short moves, continuity, footprint, cost, time, memory, and tool changes against the manual planar control. |
| P3-001 | Gated | Qualify a specific printer, firmware, nozzle pair and transfer method. | Reviewed machine-specific G-code and human authorization precede motion. Physical results require inspected specimens. |

## P2-007 implementation plan

1. Reuse the plane handles and numeric position from P2-006 in an opt-in **Material transition** mode. Add filament A and B selectors, a **Transition width** slider with an exact millimeter input, a direction swap, and a preview of the center plane and both band edges. Save the plane point, A-to-B normal, width, and slot assignments with the object in native 3MF. Applying the setting preserves the original mesh and placement.
2. Reuse `PlanarFraction` to evaluate the linear field below in the model's occupied volume. Start with one unpainted solid. Use the rectangle-only `PlanarRegions` adapter as an offline reference and extend coverage to tilted planes and ordinary solid contours.
3. Preserve the original outer envelope and allocate A or B to printable deposition segments within the transition band. With discrete extrusion, approximate each local fraction using the deposited volume of the two filaments. Compare candidate assignments on model walls and infill, and screen for internal cut walls, gaps, unsupported islands, short segments, and excessive tool changes or purge. Reuse the normal tool ordering and export path.
4. Qualify a single solid with both horizontal and tilted planes. Check the pure ends, midpoint, and fraction trend in distance bands using commanded model extrusion, excluding purge. Show the target band in Prepare and the actual material paths in Preview. Verify native 3MF round-trip, undo, and ordinary slicing with the setting disabled. Keep the H2D profile offline through the separate machine-specific qualification.

### Transition math

Let `p` be a point inside the model, `p₀` a point on the plane, and `n̂` the unit normal pointing from A toward B. Use the same physical coordinate frame for all three. For total transition width `W > 0` in millimeters, evaluate the target fractions by deposited model volume.

```text
d(p)   = (p − p₀) · n̂
f_B(p) = clamp(0.5 + d(p) / W, 0, 1)
f_A(p) = 1 − f_B(p)
```

The band edges are at `d = −W/2` and `d = +W/2`. For an 8 mm band, B goes from 0% at −4 mm to 25% at −2 mm, 50% at the plane, 75% at +2 mm, and 100% at +4 mm. Inside the band, B increases by `1/W` per millimeter, so doubling W halves the slope. When slicing with discrete filaments, compare the achieved share `V_B / (V_A + V_B)` in each occupied distance band against the target averaged over the same deposition volume. Exclude purge from both volumes.

### Transition width control

- Put **Transition width** beside the plane settings. The slider runs from **Sharper** on the left to **More gradual** on the right. The adjacent millimeter field edits the same total width and stays in sync with the slider.
- Update both preview edges as the width changes. They move symmetrically about the fixed 50/50 plane. Show when a band edge falls beyond the model and that side does not reach its pure-material endpoint.
- Use a positive width for this mode. Set practical slider bounds and step size during offline path qualification. Verify that widening the band updates the preview without moving the plane and that slider and typed input save the same value.
