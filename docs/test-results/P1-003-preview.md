# P1-003 two-cuboid preview

[Preview SVG](../assets/planar-preview.svg) · [Material error CSV](P1-003-material-error.csv) · [Reproduction commands](../TESTING.md#generate-the-two-cuboid-preview)

The fixture spans X = 4–22 mm, Y = −3–3 mm and Z = 1–5 mm. The grid origin is (4, −3, 1) mm, pitch is (1, 1, 1) mm, and phase is (0, 0, 0). The 4 mm transition spans X = 10–14 mm around the selected interface at X = 12 mm. The SVG shows all four XY layers and two central XZ sections.

| Window | Occupied cells | B cells | Target B | Assigned B | Signed error |
| --- | ---: | ---: | ---: | ---: | ---: |
| Overall | 432 | 240 | 55.556% | 55.556% | 0 pp |
| Transition | 96 | 48 | 50% | 50% | 0 pp |
| X = 10–11 mm | 24 | 6 | 12.5% | 25% | +12.5 pp |
| X = 11–12 mm | 24 | 6 | 37.5% | 25% | −12.5 pp |
| X = 12–13 mm | 24 | 12 | 62.5% | 50% | −12.5 pp |
| X = 13–14 mm | 24 | 24 | 87.5% | 100% | +12.5 pp |

The local errors cancel in the aggregate. Pure-material slabs matched exactly. The figures measure equal-volume cell assignments and do not establish deposited material fractions.

Two Windows x64 Release runs measured **16.2 µs and 16.6 µs** for sampling and assignment over the fixture, excluding file output. These are single-fixture observations. Both runs emitted byte-identical SVG and CSV files. An independent count from the XY panels matched all 18 CSV slab rows. The standalone suite passed 9 of 9 cases. Raw logs are under ignored `build-phase0/planar-p1-003-*.log`.
