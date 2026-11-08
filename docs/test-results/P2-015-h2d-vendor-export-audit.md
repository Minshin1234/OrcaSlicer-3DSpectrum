# P2-015 H2D vendor export audit

This report records the offline audit of the resolved vendor G-code exported by Bambu Studio for the Bambu Lab H2D (dual 0.4 mm nozzles) against 3DSpectrum's offline toolpaths. It closes the prerequisite identified in P2-014.

## Slicing environment and inputs

- **Vendor slicer** — Bambu Studio v02.09.01.70 (`Bambu_Studio_win-v02.09.01.70-20261103183404`) executed offline via CLI.
- **Machine profile** — `Bambu Lab H2D 0.4 nozzle` (GM033, dual 0.4 mm nozzles, reach `[0, 325]` and `[25, 350]`, shared reach `[25, 325]` × `[0, 320]`).
- **Process profile** — `0.20mm Standard @BBL H2D` (layer height 0.2 mm, line width 0.45 mm).
- **Filament profile** — `Bambu PLA Basic @BBL H2D` (temperature 220 °C, flush temperature 240 °C).
- **Assembly fixture** — Two contiguous boxes sharing a planar contact at X = 173 mm:
  - `lower_x`: 10 × 12 × 4 mm, bounds X = [163, 173], Y = [154, 166], Z = [0, 4] mm.
  - `upper_x`: 14 × 12 × 4 mm, bounds X = [173, 187], Y = [154, 166], Z = [0, 4] mm.
  - Composed assembly: 24 × 12 × 4 mm, centered on the H2D build plate at (175, 160).
- **Trial assignments**:
  - `ab` — `lower_x` assigned to filament slot 1; `upper_x` assigned to filament slot 2.
  - `ba` — `lower_x` assigned to filament slot 2; `upper_x` assigned to filament slot 1.

## Audit metrics and results

### 1. Tool assignment and nozzle mapping

| Test case | Component | Slot | Commanded tool | Moves | Commanded model E | X bounds (mm) | Y bounds (mm) | Z bounds (mm) |
| --- | --- | ---: | --- | ---: | ---: | --- | --- | --- |
| `ab` | `lower_x` | 1 | `T0 H-1` | 871 | 147.34 mm | [163.21, 172.79] | [154.21, 165.79] | [0.20, 4.00] |
| `ab` | `upper_x` | 2 | `T1 H-1` | 976 | 246.37 mm | [173.21, 186.79] | [154.21, 165.79] | [0.20, 4.00] |
| `ba` | `lower_x` | 2 | `T1 H-1` | 871 | 147.34 mm | [163.21, 172.79] | [154.21, 165.79] | [0.20, 4.00] |
| `ba` | `upper_x` | 1 | `T0 H-1` | 976 | 246.37 mm | [173.21, 186.79] | [154.21, 165.79] | [0.20, 4.00] |

- In Bambu Studio, filament slot 1 commands tool index `0` (`T0 H-1`), and filament slot 2 commands tool index `1` (`T1 H-1`).
- Reversing the slot assignments inverts the tool mapping across the contact interface deterministically.
- The `H-1` argument confirms static 1:1 hotend pairing: source inspection of `studio-GCode.cpp` (`NOZZLE_ID_FOR_GCODE`) shows that when dynamic nozzle map is inactive, `H` is set to `-1`, delegating physical hotend selection to static firmware configuration.

### 2. Geometry reach and prime tower bounds

| Feature | Active tool | Moves | Commanded E | X bounds (mm) | Y bounds (mm) | Z bounds (mm) | Inside shared reach? |
| --- | --- | ---: | ---: | --- | --- | --- | --- |
| Model (`lower_x` + `upper_x`) | `T0` / `T1` | 1,847 | 393.71 mm | [163.21, 186.79] | [154.21, 165.79] | [0.20, 4.00] | Yes ([25, 325] × [0, 320]) |
| Prime tower (`ab`) | `T0` | 1,516 | 286.73 mm | [224.95, 249.30] | [273.46, 296.34] | [0.20, 4.00] | Yes |
| Prime tower (`ab`) | `T1` | 440 | 204.33 mm | [227.87, 246.37] | [276.80, 293.05] | [0.20, 4.00] | Yes |
| Bed prime line (`ab`) | `T1` | 14 | 3.77 mm | [18.00, 290.00] | [-0.50, 0.50] | [0.00, 0.20] | Front edge purge |

- Total model extrusion for 20 layers is 393.71 mm (1.15 g).
- Prime tower extrusion across 20 tool-change layers is 491.06 mm (Tool 0: 286.73 mm, Tool 1: 204.33 mm), representing a 124.7% purge overhead relative to the model volume.
- Both the model and prime tower reside strictly within the physical intersection rectangle of the dual hotends.

### 3. Firmware coordination and macro contract

The resolved vendor export emits the following specialized commands not present in standard Marlin:
1. `M620 S[filament]A H-1` — Material change declaration and AMS index.
2. `M621 S[filament]A` — Material change completion handshake.
3. `M620.10 A0/A1 ...` — Dynamic flush temperature, volumetric speed, and flush length parameters.
4. `M620.11 ...` — Retraction parameters during tool cut and exchange.
5. `M620.15 C...` — Target filament temperature compensation before prime tower entry.
6. `G150.1`, `G150.2`, `G150.3` — Nozzle mouth wiping and heatbed avoidance routines.
7. `M993` / `M972` — Nozzle camera dirty detection and optical inspection status control.
8. `M1015.3` / `M1015.4` — Extrusion air-printing and clog detection hooks.
9. `SYNC T...` — Heating and cooling delay synchronization.

### 4. Comparison with 3DSpectrum offline baseline

| Area | Vendor export (`Bambu Studio v02.09.01.70`) | 3DSpectrum offline H2D profile |
| --- | --- | --- |
| G-code flavor | Bambu proprietary (`M620`, `M1002`, `G150.1`) | Standard Marlin (`T0`, `T1`, relative extrusion `M83`) |
| Tool change | Full toolhead switch with purge, wipe, and camera checks | Generic tool change marker, empty script |
| Prime tower | Enabled (491 mm purge extrusion on 20 layers) | Disabled (`enable_prime_tower = 0`) |
| Acceleration limits | Up to 10,000 mm/s² | Capped at 1,500 mm/s² (500 mm/s² outer walls) |
| Tool identity | `T0 H-1` / `T1 H-1` (0-indexed) | `T0` / `T1` (0-indexed) |
| Shared reach check | Verified `[25, 325]` × `[0, 320]` | Verified `[25, 325]` × `[0, 320]` |

## Qualification conclusion

The vendor export confirms that:
1. Physical tool identification matches 3DSpectrum's 0-indexed tool assignment (`T0` for slot 1, `T1` for slot 2).
2. The `H-1` parameter in Bambu Studio signifies static 1:1 physical nozzle mapping, confirming that the current dual-nozzle configuration does not require dynamic toolhead reallocation.
3. 3DSpectrum's decision to disable the prime tower and suppress proprietary macros in the offline overlay correctly isolates mathematical gradient evaluation and signed-distance path slicing from machine-specific motion.
4. Any future native command generation for hardware execution (Phase 3) must wrap tool changes in the verified `M620`/`M621`/`G150.1` macro sequence and account for the substantial flush overhead.

