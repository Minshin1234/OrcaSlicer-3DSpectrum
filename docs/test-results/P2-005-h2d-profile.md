# P2-005 H2D dual 0.4 mm profile research

## Implementations reviewed

| Source | Relevant implementation |
| --- | --- |
| [Bambu Studio H2D profile](https://github.com/bambulab/BambuStudio/blob/355d71eee5a9ebc794693fe57e63674806019770/resources/profiles/BBL/machine/Bambu%20Lab%20H2D%200.4%20nozzle.json) | Vendor preset separates physical hotend identity, nozzle variant and filament routing. Its [tool-change template](https://github.com/bambulab/BambuStudio/blob/355d71eee5a9ebc794693fe57e63674806019770/resources/profiles/BBL/machine/Bambu%20Lab%20H2D%200.4%20nozzle%20template%20change_filament_gcode.json) selects the hotend and filament separately, with firmware-controlled flushing and wipe behavior. |
| [OrcaSlicer 2.3.2 H2D profile](https://github.com/OrcaSlicer/OrcaSlicer/blob/c724a3f5f51c52336624b689e846c8fbc943a912/resources/profiles/BBL/machine/Bambu%20Lab%20H2D%200.4%20nozzle.json) | Released profile uses two 0.4 mm entries, separate reachable areas and H2D-specific scripts. Even the older tool-change script depends on H2D-specific flush data. [Later OrcaSlicer support](https://github.com/OrcaSlicer/OrcaSlicer/pull/14125) extends the nozzle-specific model. |
| [CadenceSlicer H2D profile](https://github.com/HMITCHR/CadenceSlicer/blob/84a430ab161ba95a604f7f991034f10bca5c7a12/resources/profiles/BBL/machine/Bambu%20Lab%20H2D%200.4%20nozzle.json) | This OrcaSlicer derivative retains the same reach geometry and uses nozzle IDs in its tool-change script. It is a useful comparison of the nozzle-routing contract. |

The machine JSONs distinguish the two nozzle rectangles. One spans X = 0–325 mm and the other X = 25–350 mm. Both span Y = 0–320 mm. Their intersection is X = 25–325 mm, which yields a 300 × 320 mm shared area. The manufacturer's specification gives a 325 mm Z range and the same 300 mm dual-nozzle width. [Bambu Lab H2D specification](https://cdn1.bambulab.com/documentation/h2d/en/H2D_Laser_Full_Combo_20250305.pdf)

## Local overlay

The [offline profile](../../resources/profiles/offline/Bambu%20Lab%20H2D%200.4%20dual%20-%20offline.json) uses the shared rectangle in machine coordinates, the 325 mm Z limit, and two 0.4 mm nozzle entries. It uses generic Marlin test output with an explicit offline marker and a distinct model identity. It is stored outside the installed BBL preset index and loaded by the FFF test fixture.

This checkout's `PrintConfig` lacks per-extruder reach and hotend-variant schema keys used by upstream, including `extruder_printable_area` and `printer_extruder_id`. Its G-code context lacks required H2D placeholders such as `next_hotend`, `nozzle_diameter_at_nozzle_id`, and the older `flush_temperatures`. A hardware profile needs corresponding slicer support for hotend mapping, reach/keep-out checks, and firmware-specific startup, switching, purging, and shutdown. The overlay is for offline slicing and must not be used to print.

## Verification

Windows Release FFF test. The normal JSON configuration loader accepted the overlay with no unrecognized keys or substitutions. The centered planar fixture produced 240 mm³ of B slice regions and 20 generic tool changes. Both tool IDs generated external perimeters. The model-extrusion endpoints were in the dual-nozzle intersection, and the export had the offline marker and no M620/M621 command lines. These checks use generic offline G-code.

[Reproduction commands](../TESTING.md#check-the-h2d-offline-profile). Raw logs and fetched upstream files are under ignored `build-phase0/`.

Full Windows Release CTest passed 382 of 390. The eight failure names and outcomes matched the P2-004 baseline.
