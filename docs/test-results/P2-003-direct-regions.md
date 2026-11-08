# P2-003 direct planar region adapter

Windows x64 Release with the P2-002 fixture. The adapter uses the ordinary layer contours and per-object `experimental_planar_material_regions = {8.0, 4.0, 1.0, 1.0}`. The four values are the plane offset from the sliced envelope's minimum X, transition width, XY pitch and Z pitch in millimeters. A missing or empty option leaves the existing slicing path in use. The entry point is the internal model config, with no GUI control.

| Metric | Ordered volume reference | Direct layer regions |
| --- | ---: | ---: |
| B slice-region volume | 240 mm³ | 240 mm³ |
| Largest per-layer B area error against the cell grid | 0 mm² | 0 mm² |
| Incorrect sampled cell-center labels | 0 of 2,160 | 0 of 2,160 |
| Peak material components per layer | 3 | 6 |
| Smallest component bounding-box span | 1 mm | 1 mm |
| Tools on external perimeters | 0, 1 | 0, 1 |
| Tool-change events | 21 | 20 |
| Bridge-classified moves | 58 | 58 |

Both routes met the 108 mm² footprint check on all 20 layers, with less than 0.01 mm² overlap between material regions. Each upper layer also met the previous-layer footprint overlap check. The direct route's additional components show that agreement at cell centers and by area does not establish identical region connectivity or identical toolpaths.

The adapter requires two configured physical slots, one unpainted model part, a constant axis-aligned rectangular footprint, grid-aligned extents, and layers that do not cross a Z band. The XY pitch must be at least twice the largest configured nozzle diameter. Compensation and interlocking are rejected in this prototype. The tests check a 1 mm component bounding span and a tool-change ceiling of 40 events for this fixture. These are geometric and fixture-level checks. They provide no printer-wide switch limit, minimum neck-width guarantee, deposition-volume measurement, or hardware qualification.

The targeted test passed all five sections and 1,013 assertions. Reapplying the model after removing the option restored the 20-layer single-material footprint in the same Print instance. A 0.5 mm pitch was rejected with 0.4 mm nozzles. See the [reproduction commands](../TESTING.md#experimental-direct-planar-regions). Raw logs are under ignored `build-phase0/p2-003-*.log`.

The Windows Release app and existing test executables rebuilt successfully. Full CTest passed 380 of 388 in 56.88 seconds, with the same eight baseline failures. The nine standalone cases also passed.
