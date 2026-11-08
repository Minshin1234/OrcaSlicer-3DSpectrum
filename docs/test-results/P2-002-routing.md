# P2-002 offline planar toolpath routing

Windows x64 Release. The fixture has an 18 × 6 × 4 mm envelope with two configured physical filament slots, 0.4 mm nozzles, 0.45 mm line widths and 0.2 mm layers. Support generation and the prime tower are disabled. This is a generic offline G-code comparison.

- **Solid control** — One unpainted volume assigned to material A.
- **Ordered volumes** — The P1 field and ordered grid classify 1 mm cells in the translated two-cuboid frame. Consecutive cells of the same label along X become ordinary model volumes with per-volume extruder IDs. The fixture is translated to the bed frame for slicing.
- **End-face painting** — The same solid envelope has only the X-max end face painted with material B using the existing facet annotation path. It represents a surface input and does not encode the 3D field.

| Route | B slice-region volume (mm³) | Peak material components per layer | Smallest component bounding-box span (mm) | Tools on external perimeters | Tool-change events | Bridge-classified moves |
| --- | ---: | ---: | ---: | --- | ---: | ---: |
| Solid control | 0 | 1 | 6.000 | 0 | 0 | 71 |
| Ordered volumes | 240.000 | 3 | 1.000 | 0, 1 | 21 | 58 |
| End-face painting | 29.865 | 2 | 0.857 | 0, 1 | 18 | 74 |

Each route covered the 108 mm² envelope on all 20 layers within 0.01 mm², with less than 0.01 mm² overlap between materials. Each layer above the first also had the expected footprint overlap with the preceding layer. The ordered route matched B's predicted area and all 2,160 sampled cell-center labels. Regions and bounding spans were measured before perimeter generation. Bounding span is not a minimum neck-width metric. Slice-region volume is not measured deposition volume. The overlap and bridge checks do not establish printability or material bonding.

The ordered route preserves the intended discrete regions and reaches physical tool IDs through existing paths. Face painting is useful for surface input. The tool-change counts compare two different material layouts and do not isolate the relative overhead of routing the same field.

The fixture is isolated in the FFF test target. No application UI, setting, or production adapter is introduced. Build and test commands are in [testing](../TESTING.md#compare-offline-planar-toolpath-routes). The targeted Release fixture passed all three sections and 815 assertions. Raw logs are under ignored `build-phase0/p2-002-*.log`.

Full Release CTest passed 380 of 388 and exited 8. The eight failure names and outcomes matched the P2-001 Windows baseline.
