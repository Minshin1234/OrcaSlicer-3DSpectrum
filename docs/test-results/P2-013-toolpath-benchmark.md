# P2-013 planar transition grid-pitch benchmark

Windows x64 Release. The production slicer started at [`0c59c8fb78`](https://github.com/Minshin1234/OrcaSlicer-3DSpectrum/commit/0c59c8fb7865996ae2b1465a72b0a85c279f9d2d). The [benchmark case](../../tests/fff_print/test_planar_routing.cpp) slices the same 20 mm cube with a horizontal, tilted or diagonal XY plane, then slices the ordinary cube as a control. Each route uses two 0.4 mm nozzle entries, 0.2 mm layers, and the same offline H2D overlay. Compare the 4, 8 and 12 mm widths separately.

The baseline uses a 1.2 mm XY cell pitch, three nozzle diameters. The candidate used a 1.6 mm pitch, four nozzle diameters. This changes both the length and placement of the cells that assign filament to paths. The candidate retained the same band definition, Z pitch, model geometry, speeds, and command audit.

The short-move count includes positive-length model extrusion moves under 0.2 mm. The material errors are the largest absolute difference from the target B fraction in seven bands scaled with the transition width. The time estimate comes from the ordinary G-code processor under the generic offline profile. The fixture disables the prime tower. Any recorded flush is a slicer statistic under that profile. Physical H2D purge volume and switching time need separate qualification.

## Results

Each pair shows the 1.2 mm baseline followed by the 1.6 mm candidate. Tool-change counts were identical in each pair.

| Width (mm) | Plane | Model moves under 0.2 mm | Tool changes | Max model B error (pp) | Max outer-wall B error (pp) | Estimated time (s) |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 4 | Horizontal | 412 → 379 | 21 | 4.05 → 4.58 | 5.08 → 5.04 | 2043.20 → 2029.64 |
| 4 | Tilted | 554 → 455 | 100 | 1.07 → 2.10 | 2.44 → 4.05 | 2296.69 → 2302.36 |
| 4 | Diagonal XY | 533 → 451 | 100 | 3.76 → 2.85 | 9.69 → 18.46 | 2395.31 → 2395.37 |
| 8 | Horizontal | 862 → 733 | 41 | 0.75 → 2.94 | 1.25 → 1.30 | 2298.76 → 2310.88 |
| 8 | Tilted | 1029 → 835 | 100 | 0.60 → 1.12 | 3.05 → 2.56 | 2582.12 → 2588.67 |
| 8 | Diagonal XY | 1195 → 1048 | 100 | 2.68 → 3.94 | 6.25 → 9.58 | 2680.58 → 2719.65 |
| 12 | Horizontal | 1258 → 1023 | 61 | 0.84 → 1.80 | 0.91 → 1.68 | 2597.19 → 2613.35 |
| 12 | Tilted | 1485 → 1206 | 100 | 0.95 → 0.89 | 4.03 → 3.80 | 2861.45 → 2882.68 |
| 12 | Diagonal XY | 1649 → 1419 | 100 | 2.98 → 2.43 | 8.16 → 11.22 | 2920.16 → 2981.05 |

The baseline passed at all three widths. The candidate reduced the number of short moves in every case, but the 4 mm diagonal XY case failed the 15 pp in-band outer-wall limit at 18.46 pp. The 8 mm diagonal estimate rose by 39.07 s, and the 12 mm diagonal estimate rose by 60.89 s. The ordinary control estimated 1,769.57 s and commanded 1,317.65 mm of filament. The gradient cases commanded 1,317.69–1,317.93 mm. Both variants reported zero offline flush volume.

The 8 mm shared-face STEP contact check passed with both variants. Each used 20 tool changes, used both tools on all 20 layers, and retained a continuous envelope. The candidate's largest wrong-side excursion was 0.000584 mm against the 0.002 mm rounding allowance. The cube and STEP checks cover the combined sliced envelope. The earlier [direct-region audit](P2-004-qualification.md) separately covers per-material clearance and bead-footprint coverage. This comparison leaves footprint coverage of the current paths open.

Kept the 1.2 mm production pitch. The candidate saved short moves without reducing switches and increased the time estimate in eight of the nine cases. A future localized cleanup can target isolated short runs while preserving material fractions near the band edges. Raw build and sweep logs are under ignored `build-phase0/p2-013-*`.

## Reproduce

Build `fff_print_tests` using the [Windows Release recipe](../TESTING.md#reproduce-on-windows). Run the following with the baseline source, and again after changing the multiplier in `xy_pitch` in [PlanarTransition.cpp](../../src/libslic3r/PlanarGradient/PlanarTransition.cpp) from `3.` to `4.` and rebuilding. With the environment variable unset, the regression still uses its standard 8 mm band and quiet output.

```powershell
$fff = './build-phase0/app/tests/fff_print/Release/fff_print_tests.exe'
foreach ($width in 4, 8, 12) {
    $env:SLIC3R_TEST_PLANAR_WIDTH = [string]$width
    & $fff 'A gradual material transition routes an intact solid by signed distance' --reporter compact --colour-mode none
}
Remove-Item Env:SLIC3R_TEST_PLANAR_WIDTH
& $fff '[assembly-interface]'
```
