# Agent handoff

- **Date** — 2026-11-07 EST
- **Agent** — Codex
- **Tasks** — P2-015 H2D vendor export audit and cross-hotend comparison
- **Git** — The project snapshot is based on the existing GitHub `main` history at FullSpectrum `v0.9.14` (`cc152a84dd`). Build outputs remain under ignored `build-phase0/`.
- **Milestone handoff** — Push validated major changes to a `codex/` task branch and record the branch and commit here. See [agent instructions](../AGENTS.md#stage-and-push-major-changes).

## Completed offline

In Prepare, select one solid and open **Cut → Planar → Material transition**. Adjust the plane, choose A and B, and use the width slider or millimeter field before applying. The object-local plane and width are saved in native 3MF. Export partitions the existing wall and infill paths and uses regular tool ordering. The 8 mm horizontal and tilted cube tests passed within 0.8 percentage points of their target band fractions. Full CTest passed 384 of 392 with the same eight baseline failures, and the standalone suite passed 9 of 9. See [testing instructions](TESTING.md#verify-the-gradual-material-transition-in-prepare).

The [H2D research report](test-results/P2-005-h2d-profile.md) compares pinned Bambu Studio, OrcaSlicer and CadenceSlicer implementations. The offline overlay uses X = 25–325 mm, Y = 0–320 mm, Z ≤ 325 mm, and two 0.4 mm entries. The FFF test strictly loads the file, slices the direct planar fixture, and verifies the output marker, tool IDs and extrusion endpoints. The native BBL preset index is separate. Full CTest passed 382 of 390 with the same eight baseline failures.

The raw-command audit verified generic T0/T1 assignment and extrusion endpoints for four fixtures. The diagonal XY case used both tools on every layer, and the STEP pair retained two CAD parts with one continuous sliced envelope. Only a fully shared face with matching base print settings is supported. The rebuilt app also displayed an 8 mm X gradient in the H2D offline preview. Full CTest passed 385 of 393 with the same eight baseline failures.

In P2-011, **Align to shared face** positions the preview plane on the confirmed contact, with A on the first part's side and B on the second. **Apply transition** still saves the setting. The STEP regression uses the detected plane for the existing continuous-envelope and G-code checks, and covers reversed part order, rotated and scaled placement, and invalid contacts. The three targeted tests and nine standalone tests passed. Full CTest passed 385 of 393 with the same eight failures.

In P2-014 and P2-015, the offline acceleration preset was verified and the resolved Bambu Studio v02.09.01.70 G-code was audited for the 24 × 12 × 4 mm assembly fixture across both slot assignments (`ab` and `ba`). Filament slot 1 maps to Tool 0 (`T0 H-1`), and slot 2 maps to Tool 1 (`T1 H-1`). Both model and prime tower bounds remain within the physical shared reach (`[25, 325]` × `[0, 320]`). The prime tower consumes 491.06 mm of purge filament across 20 layers (124.7% overhead). Automated regression verified tool invariants, shared reach, and suppression of proprietary vendor macros in the offline overlay. Full CTest passed 387 of 395 with the same eight baseline failures, and the standalone suite passed 9 of 9.

## Next action

P2-015 is on `codex/p2-015-h2d-vendor-audit` (`5ed8d4322d`). The [audit report](test-results/P2-015-h2d-vendor-export-audit.md) records the tool mapping (`T0/T1 H-1`), coordinate bounds, macro contract (`M620`/`M621`/`G150.1`), and purge budget. The automated regression case passed.

P2-014 is on `codex/p2-014-h2d-offline-qualification`. The [qualification report](test-results/P2-014-h2d-offline-qualification.md) records the pinned vendor contract and the tested process preset that clears the offline warning.

P2-013 is on `codex/p2-013-toolpath-benchmark`. The [benchmark report](test-results/P2-013-toolpath-benchmark.md) includes the repeatable width sweep and the decision to keep the 1.2 mm grid. Use the [P2-012 baseline](DOCUMENTATION.md#entry-p2-20261107-13--planar-transition-manufacturability-baseline) to compare any localized cleanup.

Before physical execution on an H2D printer (Phase 3 / P3-001), record the specific machine serial, firmware version, and installed hotends, review the generated machine-specific tool-change sequence offline, and obtain human operator authorization.

The P2-011 Windows build is under `build-phase0/p2-011-preview/`. Use `--datadir` with `build-phase0/p2-011-h2d-data/` to load the offline H2D profile. The `[assembly-interface]` test generates the same two-body model as the [saved STEP fixture](../tests/data/planar_gradient/SharedFace.step). If Cut is hidden, turn on **Advance** in Prepare.

[Project state](PROJECT_STATE.md) · [Backlog](TASKS.md) · [Testing](TESTING.md)
