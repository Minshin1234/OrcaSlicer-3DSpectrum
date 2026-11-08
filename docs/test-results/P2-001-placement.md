# P2-001 placement and material audit

Windows x64 Release, MSVC 19.44, using the existing app build and its OpenCASCADE dependency. The cases generate fixtures at runtime in a unique temporary directory.

## STEP and native project 3MF

The STEP fixture contains two unequal boxes nested under a translated parent. The B box rotates 90° about Z before translation. The reader returned one object with two named volumes. World bounds were computed from actual mesh vertices using the volume and instance transforms, with a tolerance of 0.0001 mm.

| Body | STEP world min → max (mm) | Native project world min → max (mm) | Extruder before → after |
| --- | --- | --- | --- |
| Placement_A | (12, 21, 3) → (14, 24, 7) | (42, 11, 8) → (44, 14, 12) | 1 → 1 |
| Placement_B | (16, 24, 4) → (19, 28, 6) | (46, 14, 9) → (49, 18, 11) | 2 → 2 |

The native project instance has an added offset of (30, −10, 5) mm. The extruders are assigned in the slicer model after STEP import. This check covers project persistence of those assignments. A one-filament test project initially reset assignment 2 to 0. The importer bounds valid extruder IDs using the project configuration. The passing fixture declares two filaments and isolates the importer’s extraction directory.

## Separate STL

Two exports with shared-frame coordinates preserved their distinct world bounds when loaded individually. Two exports at the local origin both loaded at the local origin. The test exercised direct library import. GUI placement and the combined-file dialog were reviewed in source, with no GUI run.

## Validation

The [targeted command](../TESTING.md#audit-step-native-3mf-and-separate-stl-placement) passed both new cases and the existing generic 3MF geometry cycle. The generic case covers geometry only. No third-party material mapping or toolpath generation is asserted here. Raw logs are under ignored `build-phase0/p2-001-*.log`.

Full Release CTest passed 379 of 387 in 59.38 seconds and exited 8. The eight failing test names and outcomes matched the prior Windows baseline. Both new cases passed.
