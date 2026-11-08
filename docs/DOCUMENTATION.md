# Engineering development log

## Entry P0-20261106-01 — Continuity, source audit and baseline attempt

- **Timestamp** — 2026-11-06 19:30 EST
- **Agent** — Codex
- **Session ID** — P0-20261106-codex-01
- **Development phase** — Phase 0
- **Task IDs** — P0-001, P0-002
- **Status** — Continuity and source audit done. Baseline incomplete.
- **Git reference** — Unstaged working tree on unborn main. Remote reference cc152a84dde21edde575b6cd2266168acc5f401c. See the session file manifest below.

### Objective

Make the project resumable across agents, map the existing slicing paths and establish the first reproducible Windows verification record.

### Previous behavior

The local snapshot contained FullSpectrum source and existing feature-specific documentation. Git had no local commit. Root Claude guidance was excluded by the repository ignore file. The project had no dedicated volumetric project state, backlog or chronological log.

### Files modified

- `.gitignore`
- `AGENTS.md`
- `CLAUDE.md`
- `README.md`
- `docs/PROJECT_PLAN.md`
- `docs/PROJECT_STATE.md`
- `docs/AGENT_HANDOFF.md`
- `docs/DOCUMENTATION.md`
- `docs/ARCHITECTURE.md`
- `docs/DECISIONS.md`
- `docs/TASKS.md`
- `docs/TESTING.md`
- `docs/RESEARCH.md`
- `docs/EXPERIMENTS.md`
- `docs/assets/README.md`
- `docs/test-results/README.md`
- `docs/test-results/P0-002-windows.md`

### Implementation details

Added project startup and handoff rules to the existing repository guidance. Replaced root Claude guidance with a canonical import and made that entrypoint eligible for tracking. Used root AGENTS.md for current Antigravity. Added a README section and project records covering provenance, source and phase boundaries, experiments, build commands and outcomes. Existing feature-specific docs were kept in place.

### Engineering rationale

Start with a standalone rectangular preview to isolate geometry and sampling from machine and dependency variability. Record existing color gradients and Local-Z as reuse candidates. Carry signed physical distance through a separate interface field. The integration route remains a proposal pending geometry and toolpath tests.

### Problems and debugging

The initial app configure stopped at Boost discovery. The full dependency attempt reported missing Perl, MinGW header contamination and OpenCV patch path errors. A clean dependency configure selected the bundled EXPAT route. Portable native Strawberry Perl enabled a targeted OpenSSL retry. An OpenCV patch dry-run passed after placing a complete set of patch inputs under an ignored build directory within the Git root.

### Testing and validation

[Windows run evidence](test-results/P0-002-windows.md) records commands, inputs and outcomes. The strict CTest invocation found no tests and exited 8. The architecture map records static inspection separately from runtime checks. Checked local Markdown link targets and verified the root Claude entrypoint is visible to Git.

### Outcome

Project instructions, state, handoff, build evidence and backlog are available in the workspace. P0-002 remains open. No new slicer feature or hardware behavior was validated.

### Next steps

Complete the MSVC dependency baseline and run the existing Catch2 tests. Perform read-only provenance reconciliation. Start P1-001 with the two-cuboid planar sampler.

## Entry P1-20261106-01 — Standalone planar material fraction

- **Timestamp** — 2026-11-06 19:41 EST
- **Agent** — Codex
- **Session ID** — P0-P1-20261106-codex-02
- **Development phase** — Phase 1
- **Task ID** — P1-001
- **Status** — Standalone implementation and tests complete
- **Git reference** — Unstaged changes on unborn main

### Objective and previous behavior

Evaluate a continuous material fraction across a physically dimensioned plane. The existing mixed-filament gradient accepts progress in its own domain. The project had no standalone signed-distance field.

### Files modified

- `src/libslic3r/PlanarGradient/PlanarFraction.hpp` and `PlanarFraction.cpp` define and implement the standalone field.
- `tests/data/planar_gradient/TwoCuboidFixture.hpp` defines the two generated bodies in the same millimeter frame.
- `tests/planar_gradient/CMakeLists.txt` and `test_planar_fraction.cpp` build the field and exercise it with the existing pinned Catch2 setup.
- `docs/ARCHITECTURE.md`, `docs/TESTING.md`, `docs/TASKS.md`, `docs/PROJECT_STATE.md`, `docs/AGENT_HANDOFF.md`, and this log record the interface, test procedure and handoff.

### Implementation and rationale

Construct the field from a point, a direction pointing from A to B, and the total transition width. Normalize the normal with a scaled norm. The B fraction is the clamped linear ramp about the plane. At zero width the plane belongs to B. Use two cuboids of unequal X extent with a shared face at x = 12 mm so the fixture exercises placement and the transition without depending on import. Keep the field independent of occupancy and of the production slicer targets. Test non-finite input and explicitly report overflow during signed-distance evaluation.

### Testing and outcome

Configured and built the [standalone test project](TESTING.md#standalone-planar-sampler) using VS 2022 x64 and CMake 3.31.6-msvc6. Strict CTest passed 5 of 5 cases. The tests cover the shared face, placement, monotonicity, two widths, a scaled oblique normal, reversal, sharp split, invalid inputs and arithmetic overflow. The four new C++ files passed the repository clang-format check. Logs are under the ignored `build-phase0/` directory. No slicer integration or deposition behavior was exercised by these tests.

### Next steps

P1-002 can consume the fixture bounds and field for occupied-cell assignment with a deterministic 3D threshold.


## Entry P0-20261106-02 — Windows Release build and suite baseline

- **Timestamp** — 2026-11-06 20:39 EST
- **Agent** — Codex
- **Session ID** — P0-P1-20261106-codex-02
- **Development phase** — Phase 0 follow-through
- **Task ID** — P0-002
- **Status** — Build and test baseline recorded
- **Git reference** — Unstaged changes on unborn main

### Objective and previous behavior

Complete the corrected Windows dependency and application build and run the existing suites. The earlier attempt could only configure part of the dependency build.

### Files modified

`docs/test-results/P0-002-windows.md`, `docs/TASKS.md`, `docs/PROJECT_STATE.md`, `docs/AGENT_HANDOFF.md`, and this log. Generated outputs live under ignored `build-phase0/`.

### Implementation and rationale

Used the VS 2022 x64 developer environment, native Strawberry Perl, a PATH without MSYS2/MinGW, and the in-repository build directory required by the dependency patch paths. This resolved the earlier Perl, header and OpenCV patch blockers without dependency-source edits. Configured the application with `BUILD_TESTS=ON`.

### Testing and outcome

The dependency target, Release GUI and all five existing test executables built. CTest passed 377 of 385 and returned exit 8. All eight failures reproduced on rerun. See the [run evidence](test-results/P0-002-windows.md#completed-windows-release-baseline) for case names and symptoms. The standalone sampler test target is independent of the application baseline.

### Next steps

P0-004 tracks the existing suite failures. P1-002 is the next standalone prototype task.

## Entry P1-20261106-02 — Ordered cell assignment

- **Timestamp** — 2026-11-06 EST
- **Agent** — Codex
- **Session ID** — P1-20261106-codex-03
- **Task** — P1-002, complete
- **Git reference** — Unstaged changes on unborn main

### Implementation

Added `src/libslic3r/PlanarGradient/OrderedCells.hpp` and `OrderedCells.cpp`. The grid exposes physical cell centers and labels occupied cells using a repeating 4×4×4 midpoint threshold pattern. The phase shifts the pattern independently of placement. The caller supplies occupancy and continuous B fraction. The `1/128` bound applies to uniform fractions over complete occupied periods.

Added `tests/planar_gradient/test_ordered_cells.cpp` and updated its `CMakeLists.txt`. Updated `docs/ARCHITECTURE.md`, `docs/TESTING.md`, `docs/TASKS.md`, `docs/PROJECT_STATE.md`, `docs/AGENT_HANDOFF.md`, and this log.

### Validation

Built the standalone Release target using `cmake --build build-phase0/planar --config Release --parallel 4`. Strict CTest passed all nine cases. The 1 mm cubic grid fixture contains 432 occupied centers. Its 4 mm transition spans 96 centers, with 48 assigned to B. The uniform test checked 257 fractions across a complete period. Raw logs are under ignored `build-phase0/planar-p1-002-*.log`. The Windows application suite was outside this standalone change.

### Next

P1-003 will report measured windowed error for the varying field and produce XY and XZ previews.

## Entry P1-20261106-03 — Fixture preview and local error

- **Date** — 2026-11-06 EST
- **Agent** — Codex
- **Task** — P1-003, complete
- **Git reference** — Unstaged working tree on unborn main

Added `tests/planar_gradient/planar_gradient_preview.cpp` and its target in `tests/planar_gradient/CMakeLists.txt`. Moved the half-open occupancy helper from `test_ordered_cells.cpp` to the shared `TwoCuboidFixture.hpp`. Recorded outputs in `docs/assets/planar-preview.svg`, `docs/test-results/P1-003-material-error.csv` and `docs/test-results/P1-003-preview.md`. Updated architecture, testing, backlog, project state, this log and handoff.

The Release standalone build and nine tests passed. Two generator runs produced byte-identical SVG and CSV outputs. The SVG was rendered and inspected, and B counts in the XY panels matched all CSV slab rows. The transition had zero net error and up to 12.5 percentage points of local error. Phase 2 starts with the P2-001 placement audit. The preview does not exercise import or the slicer.

## Entry P2-20261106-01 — Assembly placement and project persistence audit

- **Date** — 2026-11-06 EST
- **Agent** — Codex
- **Task** — P2-001, complete
- **Git reference** — Unstaged working tree on unborn main

Added `tests/libslic3r/test_assembly_placement.cpp` to `tests/libslic3r/CMakeLists.txt`. The tests generate a nested STEP assembly, round-trip it via native 3MF with independent volume extruders, and compare shared-frame and local-origin STL exports. The first round trip used a one-filament project and reset the second assignment. The corrected test uses two configured filaments and an isolated extraction path.

Updated `docs/ARCHITECTURE.md`, `docs/TESTING.md`, `docs/TASKS.md`, `docs/PROJECT_STATE.md`, `docs/AGENT_HANDOFF.md`, this log and `docs/test-results/P2-001-placement.md`. Targeted CTest passed 3 of 3. Full Release CTest passed 379 of 387 with the same eight baseline failures. P2-002 is the next integration task.

## Entry P2-20261106-02 — Offline comparison of planar toolpath routes

- **Date** — 2026-11-06 EST
- **Agent** — Codex
- **Task** — P2-002 prototype
- **Git reference** — Unstaged working tree on unborn main

Added `tests/fff_print/test_planar_routing.cpp` and linked the two standalone planar sources into the FFF test target in `tests/fff_print/CMakeLists.txt`. The fixture uses explicit per-volume extruder IDs or existing facet paint. It measures region coverage, overlap, layer support footprints, cell-center labels, components and offline toolpath metrics against an unpainted solid. Fixture setup needed the ordinary relative-extrusion reset, mesh-local max-X for the painted face, and explicit line widths after the automatic-width paint case raised a flow-spacing exception.

Updated `docs/ARCHITECTURE.md`, `docs/TESTING.md`, `docs/DECISIONS.md`, `docs/TASKS.md`, `docs/PROJECT_STATE.md`, `docs/AGENT_HANDOFF.md`, this log and `docs/test-results/P2-002-routing.md`. Targeted Release execution passed all three sections and 815 assertions. Production integration is tracked under P2-003.

Full Release CTest passed 380 of 388 and exited 8. The eight failure names and outcomes matched the P2-001 Windows baseline.

## Entry P2-20261107-01 — Experimental direct planar region adapter

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Task** — P2-003
- **Git reference** — Unstaged working tree on unborn main

Added `PlanarGradient/PlanarRegions.hpp` and `PlanarRegions.cpp` to the core slicer and registered the existing fraction and ordered cell sources in `src/libslic3r/CMakeLists.txt`. `PrintConfig.cpp` defines an opt-in per-object vector with an empty default. `PrintApply.cpp` reuses the existing material region registration and invalidates shared slices when the option changes. `PrintObjectSlice.cpp` feeds the ordinary slice contours to the adapter before perimeter generation and passes its existing cancellation callback.

Updated `tests/fff_print/CMakeLists.txt` to consume the core planar sources and extended `test_planar_routing.cpp` with the direct route, removal from a previously sliced print, and small-pitch rejection. The targeted Windows Release test passed all five sections. Updated `docs/ARCHITECTURE.md`, `docs/TESTING.md`, `docs/DECISIONS.md`, `docs/TASKS.md`, `docs/PROJECT_STATE.md`, `docs/AGENT_HANDOFF.md`, this log and `docs/test-results/P2-003-direct-regions.md`.

The direct adapter matched region area and sampled labels, but had a higher peak component count than the ordered-volume reference. P2-004 tracks project persistence, continuity and toolpath qualification.

The Windows Release app and existing test executables rebuilt successfully. Full CTest passed 380 of 388 in 56.88 seconds, with the same eight baseline failures. The nine standalone cases also passed.

## Entry P2-20261107-02 — Project persistence and path screening

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Task** — P2-004 offline portion
- **Git reference** — Unstaged working tree on unborn main

Extended `tests/libslic3r/test_assembly_placement.cpp` to verify native 3MF persistence on one opted-in object alongside a default control. Extended `tests/fff_print/test_planar_routing.cpp` to measure per-tool commanded model volume, previous-layer swept footprint coverage, and component clearance at one and two nominal bead widths. The tests leave production code unchanged.

Updated `docs/TESTING.md`, `docs/TASKS.md`, `docs/PROJECT_STATE.md`, `docs/AGENT_HANDOFF.md`, this log and `docs/test-results/P2-004-qualification.md`. Both targeted tests passed. The generic audit found connections that split at one bead width and centerlines outside the previous bead footprint. The local H2D machine preset is absent. The local U1 preset has four nozzle entries and is outside the adapter's current two-entry guard.

Full Windows Release CTest passed 381 of 389. The eight failure names and outcomes matched the P2-003 baseline.

## Entry P2-20261107-03 — H2D implementation comparison and offline profile

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Task** — P2-005
- **Git reference** — Unstaged working tree on unborn main

Compared pinned Bambu Studio, OrcaSlicer and CadenceSlicer profiles. Added `resources/profiles/offline/Bambu Lab H2D 0.4 dual - offline.json` and a strict loader and offline export check in `tests/fff_print/test_planar_routing.cpp`. Used the verified common nozzle reach and an explicit offline output marker. The fork lacks the required reach and hotend-routing interfaces for native H2D templates.

Updated `docs/TESTING.md`, `docs/TASKS.md`, `docs/PROJECT_STATE.md`, `docs/AGENT_HANDOFF.md`, this log and `docs/test-results/P2-005-h2d-profile.md`. The targeted test passed. P2-004 still requires machine-specific qualification.

Full Windows Release CTest passed 382 of 390. The eight failure names and outcomes matched the P2-004 baseline.

## Entry P2-20261107-04 — Adjustable planar material split

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Task** — P2-006
- **Git reference** — Unstaged working tree on unborn main

Added **Split by material** to the Planar Cut panel for an unpainted single FFF model instance with at least two filament slots. Both sides are kept in one multipart object and assigned the chosen physical filament slots. The cut validates that both sides contain solids before replacing the source. The panel reuses the cut preview, numeric height and plane handles. The mode is unavailable for objects with connectors, individually selected cut pieces, painting or an experimental gradient.

Verified the Windows UI with the H2D offline profile and the 20 mm cube fixture. At a 7 mm cut, selecting filament 2 for the upper side switched the lower selector to filament 1. The Objects list showed the two solids under one object, with slots 2 and 1. The offline Preview showed both filaments and one filament change. In a separate Prepare check, dragging the plane handle moved the displayed position from 10.00 mm to 8.05 mm. The material controls and both choices fit the 1190 × 795 app window. No printer operation was used.

The FFF regression passed for the offset plane, tilted plane, duplicate slots and a plane outside the model. The nine standalone tests passed. All five Windows Release test targets rebuilt. Full CTest passed 383 of 391 in 73.11 seconds, with the same eight failures as P2-005. Logs are under ignored `build-phase0/`.

## Entry P2-20261107-05 — Gradual material transition plan

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Task** — P2-007 planning
- **Git reference** — Unstaged working tree on unborn main

The user clarified that the adjustable plane should mark the 50/50 midpoint of a gradual A-to-B transition through one intact model. Recorded the positive-width band, geometry preservation, deposition and offline acceptance criteria in the [implementation plan](TASKS.md#p2-007-implementation-plan), and updated the state and handoff. This session changed project documentation only.

## Entry P2-20261107-06 — Transition math and width control

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Task** — P2-007 planning
- **Git reference** — Unstaged working tree on unborn main

Expanded the plan with the signed-distance formula, an 8 mm worked example, and validation by deposited volume. Specified a **Transition width** slider with synchronized millimeter input and a live band preview. Updated state and handoff. This session changed project documentation only.

## Entry P2-20261107-07 — Gradual transition implementation and offline checks

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Task** — P2-007
- **Git reference** — Unstaged working tree on unborn main

Implemented **Material transition** in the Cut tool. Applying stores the plane, width and two filament slots on the original object. The Prepare preview shows the midpoint and both edges, and the slicer assigns portions of its existing extrusion paths to A or B after generating the walls and infill. The offline regression caught duplicate whole loops on the second filament. Fixed the assignment before rerunning the checks. The desktop verification also caught a startup crash when opening Cut before its shared selection data was ready. Deferred loading the transition to the first input render and verified the rebuilt panel.

On the 20 mm desktop cube, widening from 8.00 to 16.11 mm kept the plane at 10.00 mm and moved both edges. Exact entry restored 8.00 mm. Undo removed the setting, and reopening after applying restored the saved controls. The Objects list retained one solid. The H2D offline preview showed both filaments and 41 filament changes.

The horizontal and tilted fixtures passed all seven distance bands. The maximum model-volume fraction error was below 0.8 percentage points and the outer-wall fraction error was below 3.1 points. Commanded model extrusion differed from the ordinary slice by less than 0.03%. The final route emitted 41 and 100 tool changes and 862 and 1,029 model extrusion moves shorter than 0.2 mm, respectively. The short-move counts fell from 1,366 and 5,244 after redundant collinear grid points were removed. Physical deposition, purge cost and H2D firmware behavior remain open.

The Windows Release app and all five test targets built. Native 3MF passed, the standalone sampler passed 9 of 9, and full CTest passed 384 of 392 with the same eight baseline failures. Logs are under ignored `build-phase0/`.

## Entry P2-20261107-08 — Raw G-code, XY gradient and STEP interface

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Tasks** — P2-008, P2-009 and P2-010
- **Git reference** — Unstaged working tree on unborn main

Audited exported G-code by reading T0/T1, feature tags, relative extrusion and absolute XYZ endpoints. Both tool assignments passed, including swapped slots. All tested model extrusion endpoints stayed in the offline H2D shared reach. The maximum wrong-tool excursion beyond either edge of the signed-distance band was 0.000471 mm across the four fixtures, within the 0.002 mm G-code rounding allowance. The profile still uses generic tool commands and has no verified physical left/right mapping.

Added X, Y and Z direction controls in Prepare and made the plane position field follow the dominant axis. The diagonal XY fixture used both tools on all 100 layers. In the desktop preview, X and Y changed the position axis, and the applied 8 mm X gradient sliced with both filaments and 100 changes.

Imported a STEP fixture containing two asymmetric solids, then translated and rotated their common instance. The two solids stayed in place, the layer envelope was continuous, and both tools deposited inside the band on both sides of the original contact. All 20 layers used both tools. The strengthened regression found no wall extrusion inside the contact neighborhood. The guard rejected a 0.1 mm gap, a 0.1 mm overlap, a partial face and a plane shifted by 0.25 mm. The interface is aligned manually, and the two bodies need matching base print settings.

The Release app and test targets built. Full CTest passed 385 of 393 in 80.26 seconds with the same eight baseline failures. After adding the stronger contact checks, all three targeted cases passed again. Raw audit metrics and build logs are under ignored `build-phase0/`.

## Entry P2-20261107-09 — Shared CAD face alignment

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Task** — P2-011
- **Git reference** — Unstaged working tree on unborn main

Added **Align to shared face** to the two-part material transition panel. It proposes a plane in the placed geometry and verifies it with the slicing contact check. The normal points from the first body to the second. The action changes the preview plane, while **Apply transition** saves the setting. Reopening the tool recomputes the plane-relative bounds from the saved orientation and position.

The STEP test sliced from the detected plane and retained two source bodies, one continuous envelope and both tools on all 20 layers. It also checked reversed part order, rotated and scaled placement, rejection of a gap, overlap, partial face and edge contact, and slicing rejection of a manually offset plane. Release GUI and all five test targets built. The application DLL was staged separately to preserve the open app. The three targeted cases and nine standalone cases passed. Full CTest passed 385 of 393 in 53.06 seconds, with the same eight failures as P2-010. Logs and the STEP fixture are under ignored `build-phase0/`.

The staged app opened with the offline H2D profile and the STEP import dialog. A Windows firewall prompt blocked further desktop checks, so the alignment button has offline fixture coverage and awaits a desktop walkthrough.

## Entry P2-20261107-10 — Shared-face desktop walkthrough

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Task** — P2-011 desktop verification
- **Git reference** — Unstaged working tree on unborn main

Imported the two-body STEP fixture as one object in the staged Release app, enabled **Advance**, and opened **Cut → Material transition**. **Align to shared face** moved the plane from Z = 2.00 mm to the contact at X = 173.00 mm on the centered plate, preserving the 8.00 mm width. Applying and reopening Cut restored the plane, width and filament assignments. The Objects list still showed both original CAD parts under one object.

The offline H2D slice succeeded and Preview showed the blue and orange filament paths interleaved through the contact band, 20 layers and 20 filament changes. The app warned that the process acceleration exceeds the printer profile's setting. No machine was used. The three targeted cases passed again in 1.23 seconds. This follow-up changed documentation only.

## Entry P0-20261107-11 — GitHub source snapshot and README

- **Date** — 2026-11-07 EST
- **Task** — P0-003 and project publication
- **Git base** — `cc152a84dd` on GitHub `main`

Compared the complete local source snapshot with the existing FullSpectrum history. All baseline paths were present. Prepared the project changes as a child of the existing GitHub commit using a separate index. Rewrote the README around adjustable transitions, within-layer filament assignment, and shared CAD faces. The roadmap separates offline slicing from printer qualification and the planned gripper.

All nine README relative links resolved. The standalone suite passed 9 of 9. Full Windows Release CTest passed 385 of 393 in 84.48 seconds, with the same eight failures as P2-011. This source publication skips push-triggered GitHub workflows because the inherited build workflow can also publish binaries.

## Entry P0-20261107-12 — Milestone handoff workflow

Added a standing instruction to validate major changes, stage task files, commit, and push to a `codex/` task branch. The handoff records the branch, commit, and checks. Approval remains separate for pushes to `main` and releases. This update changes project documentation only.

## Entry P2-20261107-13 — Planar transition manufacturability baseline

- **Date** — 2026-11-07 EST
- **Task** — P2-012
- **Source revision** — [`0c59c8fb7865996ae2b1465a72b0a85c279f9d2d`](https://github.com/Minshin1234/OrcaSlicer-3DSpectrum/tree/0c59c8fb7865996ae2b1465a72b0a85c279f9d2d)
- **Environment** — Windows x64 Release, Visual Studio 2022, CTest 3.31.6-msvc6

### Experimental direct regions

The 18 × 6 × 4 mm rectangle used a 4 mm transition, 1 mm cells, 0.2 mm layers and 0.45 mm nominal bead width. The direct route commanded 54.902% material B versus a 55.556% slice-region target, a difference of −0.654 percentage points. It matched the commanded B share of the ordered-volume reference to the reported precision. At each trial clearance width of 0.45 mm and 0.90 mm, 20 component-layer instances split and none disappeared entirely, in each route.

The direct route had 174.401 mm of upper-layer centerlines outside the modeled previous-layer bead footprint, or 4.65% of 3,750.630 mm. About 3.00% of the total was outside the footprint and outside bridge or overhang roles. These measures screen geometry using a modeled bead footprint. The [P2-004 qualification report](test-results/P2-004-qualification.md) defines the calculations and records both routes.

### Current transition toolpaths

The current workflow assigns the two materials along existing wall and infill paths. Use the [routing test source](../tests/fff_print/test_planar_routing.cpp) to reproduce the centered 8 mm transitions through a 20 mm cube (horizontal, tilted with reversed slots, and diagonal XY) and the shared face between the 10 × 12 × 4 mm and 14 × 12 × 4 mm STEP solids. For a desktop check, import the [saved STEP fixture](../tests/data/planar_gradient/SharedFace.step) as one object, select **Align to shared face**, set the width to 8 mm, and slice with the offline profile.

The raw G-code audit checked generic T0/T1 assignments and both endpoints of each model extrusion move against the band and shared nozzle reach. The largest wrong-side excursion was 0.000471 mm across the four fixtures, within the 0.002 mm rounding allowance. The diagonal XY cube used both materials on all 100 layers. The STEP assembly used both on all 20 layers and produced one continuous envelope.

| 8 mm fixture | Model moves under 0.2 mm | Tool changes | Max model B error (pp) | Max outer-wall B error (pp) |
| --- | ---: | ---: | ---: | ---: |
| Horizontal cube | 862 | 41 | 0.75 | 1.25 |
| Tilted cube, reversed slots | 1,029 | 100 | 0.60 | 3.05 |
| Diagonal XY cube | 1,195 | 100 | 2.68 | 6.25 |
| Shared-face STEP | — | 20 | — | — |

Errors are the largest absolute difference from the target B fraction across seven signed-distance bands, in percentage points. The STEP case currently records routing and envelope checks. Actual purge cost and printer time remain open.

The [H2D profile research](test-results/P2-005-h2d-profile.md) describes the remaining integration work. The offline profile uses generic tool selection. Physical hotend mapping, firmware-specific tool changes, wipe and purge behavior remain unqualified. The desktop app also reported an acceleration setting above the profile limit. All evidence here comes from simulation, G-code inspection or desktop preview.

### Evidence and reproduction

- [Windows test log excerpts and per-fixture metrics](test-results/P2-012-baseline.txt). Five focused cases passed on the pinned source. The publication run passed 385 of 393 with the same eight baseline failures. The standalone sampler passed 9 of 9. Full local logs remain under ignored `build-phase0/`.
- [Desktop STEP filament preview](assets/p2-011-shared-face-filament-preview.jpg) captured from the Release app. It shows two original CAD bodies in the Objects list, an interleaved band, and 20 filament changes. The preview's 9 min 15 s estimate uses the offline profile.
- [Standalone two-cuboid preview and measurements](test-results/P1-003-preview.md) show the 1 mm cell assignments for the earlier 4 mm transition. Use the [Windows build and test recipe](TESTING.md#reproduce-on-windows) to rebuild the benchmark executables.

From the repository root, rerun the focused suite and capture the per-band and routing metrics with the Release executable. The verbose output includes repeated assertions. Keep the raw files under the build directory.

```powershell
ctest --test-dir build-phase0/app -C Release -R 'Compare opt-in ordered volumes|H2D offline machine geometry|A gradual material transition|A STEP assembly transitions|Native 3MF preserves planar material' --output-on-failure --no-tests=error
$fff = './build-phase0/app/tests/fff_print/Release/fff_print_tests.exe'
foreach ($section in 'Horizontal plane', 'Tilted plane and reversed slots', 'Diagonal XY gradient') {
    & $fff 'A gradual material transition routes an intact solid by signed distance' -c $section -s > "build-phase0/P2-012-$section.log"
}
& $fff '[assembly-interface]' -c 'Contact face stays in place and both tools print each layer' -s > build-phase0/P2-012-STEP.log
```

### Next milestones

1. **P2-013 toolpath optimization benchmark.** Run the baseline and candidate on identical fixtures and profiles. Include several widths and both the ordinary slice and sharp-interface control where applicable. Record sub-0.2 mm model segments, total model extrusion, tool changes, B fraction error by signed-distance band, outer-wall error, estimated print time and purge cost. Record clearance splits and previous-layer footprint misses separately, with bridge and overhang roles identified.
2. **H2D G-code qualification.** Verify physical left/right hotend mapping, shared reach, tool changes, purge and wipe against a specific machine and firmware. Resolve the acceleration warning before hardware use.
3. **First physical graded specimen.** With reviewed G-code and operator approval, print a planar specimen and inspect the deposited material distribution.
4. **Mechanical testing.** Compare graded and sharp-interface specimens under the same test conditions and record failure locations and variability.
5. **Curved interfaces.** Extend contact selection and signed-distance sampling after planar manufacturing behavior is characterized.

## Entry P2-20261107-14 — Toolpath grid-pitch benchmark

- **Date** — 2026-11-07 EST
- **Task** — P2-013
- **Branch** — `codex/p2-013-toolpath-benchmark`

Added an optional width sweep to the existing cube regression. Compared the production 1.2 mm XY grid with a 1.6 mm candidate at 4, 8 and 12 mm widths across horizontal, tilted and diagonal XY planes. The ordinary control stayed at 1,317.65 mm of commanded filament. The gradient cases stayed within 0.022% of that total and the offline profile reported zero flush.

The 1.6 mm candidate reduced short moves in all nine cases. The 8 mm diagonal dropped from 1,195 to 1,048 short moves, still used 100 tool changes, and increased the offline time estimate by 39.07 seconds. The 4 mm diagonal outer-wall error rose from 9.69 to 18.46 percentage points and failed the in-band tolerance. The 8 mm STEP contact still passed the command audit and used both tools through the 20-layer envelope. Restored the 1.2 mm production grid. The [benchmark report](test-results/P2-013-toolpath-benchmark.md) has all rows and reproduction commands.

With the production pitch restored, the Release test targets rebuilt and full CTest passed 385 of 393 in 81.92 seconds. The eight failures matched P2-012 by case and outcome. The standalone suite passed 9 of 9.

## Entry P2-20261107-15 — H2D offline command and acceleration review

- **Date** — 2026-11-07 EST
- **Task** — P2-014
- **Branch** — `codex/p2-014-h2d-offline-qualification`

Compared the pinned vendor preset, exporter and nozzle-mapping code with the generic offline export. Bambu Studio resolves the filament and its physical hotend separately. The local raw audit verifies the generic slots and shared model reach. A resolved export for the target firmware remains necessary to review physical left/right mapping and firmware-specific startup, change, purge, wipe and shutdown.

Added a conservative offline process preset and a regression that reproduces the original validation warning, clears it, and checks the generated commands. The 20-layer test used both generic tools on every layer, retained the offline marker and emitted accelerations of 300, 500 and 1,500 mm/s². The preset is separate from the machine overlay and the P2-013 benchmark inputs.

The Windows Release FFF target rebuilt. All five focused cases passed, all nine standalone cases passed, and full CTest passed 386 of 394 in 82.33 seconds with the same eight baseline failures. See the [qualification report](test-results/P2-014-h2d-offline-qualification.md). Raw logs are under ignored `build-phase0/p2-014-*`.

## Entry P2-20261107-16 — H2D vendor export audit and cross-hotend comparison

- **Date** — 2026-11-07 EST
- **Task** — P2-015
- **Branch** — `codex/p2-015-h2d-vendor-audit`

Audited the resolved vendor G-code generated offline by Bambu Studio v02.09.01.70 for the dual 0.4 mm H2D against 3DSpectrum's offline toolpaths. Evaluated the 24 × 12 × 4 mm reference assembly fixture (10 × 12 × 4 mm lower solid and 14 × 12 × 4 mm upper solid) under both standard (`ab`: filaments 1, 2) and reversed (`ba`: filaments 2, 1) slot assignments.

The vendor audit confirmed that:
1. Tool mapping commands emit `T0 H-1` for filament slot 1 and `T1 H-1` for filament slot 2, with `H-1` signifying static 1:1 hardware nozzle pairing.
2. The entire model envelope (X = [163.21, 186.79], Y = [154.21, 165.79]) and the prime tower footprint (X = [224.95, 249.30], Y = [273.46, 296.34]) stay strictly inside the shared reach of both hotends (`X = [25, 325], Y = [0, 320]`).
3. Slicing with active prime tower requires 491.06 mm of purge extrusion across 20 layers for 393.71 mm of model extrusion (a 124.7% purge overhead).
4. 3DSpectrum's conservative offline overlay (`H2D offline 0.20 mm - conservative.json`) cleanly suppresses proprietary vendor macros (`M620`, `M621`, `G150.1`), caps acceleration at 1,500 mm/s², and isolates mathematical gradient evaluation from unverified physical machine motion.

Added an automated regression case `H2D vendor export contract matches offline tool and reach invariants` to `tests/fff_print/test_planar_routing.cpp`. The test verifies automated shared-face alignment, dual-tool coverage, reach containment, reversed slot invariance, and macro suppression. Full CTest passed 387 of 395 in 68.57 seconds with the same eight baseline failures. Standalone suite passed 9 of 9. See the [qualification report](test-results/P2-015-h2d-vendor-export-audit.md).
