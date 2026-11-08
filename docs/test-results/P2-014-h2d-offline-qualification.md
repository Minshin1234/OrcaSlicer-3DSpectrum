# P2-014 H2D offline qualification

This comparison starts from 3DSpectrum commit `95adaad2e7` and the [pinned H2D research sources](P2-005-h2d-profile.md). The target scope is the Bambu Lab H2D with two 0.4 mm nozzles. The firmware version and installed hotends on a physical printer still need to be recorded.

## Vendor export contract and local evidence

| Area | Pinned Bambu Studio implementation | 3DSpectrum offline output |
| --- | --- | --- |
| Hotend identity | The exporter resolves a nozzle from the filament and layer mapping, then populates the hotend argument separately from the filament ID. | The raw audit verifies generic T0/T1 selection for the two filament slots, including reversed slots. Physical left/right assignment is open. |
| Reach and clearance | The machine preset has separate reach polygons and clearance parameters. The two 0.4 mm reaches intersect at X = 25–325 mm and Y = 0–320 mm. | The audit checks model extrusion endpoints in that shared rectangle. Startup, change and wipe motion need independent clearance checks. |
| Tool-change sequence | The templates use distinct startup, change and shutdown sequences. Change arguments include the current and next hotend, filament and nozzle-specific context. | The overlay uses Marlin and an offline marker with an empty change script. No H2D sequence is generated. |
| Purge and wipe | The change template uses nozzle diameter, flush temperature, volumetric speed and requested flush length. The preset also supplies variant-specific flush data and a 5.6 s extruder-switch estimate. | The P2-013 fixture reported zero modeled flush and generic time estimates. Neither value establishes the firmware's purge volume or switch time. |

The vendor sources are pinned at `355d71eee5a9ebc794693fe57e63674806019770`. [Machine preset](https://github.com/bambulab/BambuStudio/blob/355d71eee5a9ebc794693fe57e63674806019770/resources/profiles/BBL/machine/Bambu%20Lab%20H2D%200.4%20nozzle.json) · [Nozzle mapping source](https://github.com/bambulab/BambuStudio/blob/355d71eee5a9ebc794693fe57e63674806019770/src/libslic3r/MultiNozzleUtils.cpp) · [G-code exporter](https://github.com/bambulab/BambuStudio/blob/355d71eee5a9ebc794693fe57e63674806019770/src/libslic3r/GCode.cpp) · [Tool-change template](https://github.com/bambulab/BambuStudio/blob/355d71eee5a9ebc794693fe57e63674806019770/resources/profiles/BBL/machine/Bambu%20Lab%20H2D%200.4%20nozzle%20template%20change_filament_gcode.json)

The [OrcaSlicer 2.3.2 preset](https://github.com/OrcaSlicer/OrcaSlicer/blob/c724a3f5f51c52336624b689e846c8fbc943a912/resources/profiles/BBL/machine/Bambu%20Lab%20H2D%200.4%20nozzle.json) uses its extruder index for the change and diameter lookup. [CadenceSlicer](https://github.com/HMITCHR/CadenceSlicer/blob/84a430ab161ba95a604f7f991034f10bca5c7a12/resources/profiles/BBL/machine/Bambu%20Lab%20H2D%200.4%20nozzle.json) uses separate nozzle IDs for the diameter lookup. That difference makes the exporter and its template a versioned pair for the qualification fixture.

## Offline acceleration preset

The generic machine overlay inherits a 1,500 mm/s² extrusion cap. Its Marlin flavor applies that cap when validating the travel setting too. The original process inherits acceleration requests up to 10,000 mm/s². The [conservative 0.20 mm process preset](../../resources/profiles/offline/H2D%20offline%200.20%20mm%20-%20conservative.json) sets inner walls and travel to 1,500 mm/s² and outer walls to 500 mm/s². It leaves the machine overlay and the P2-013 benchmark inputs intact. Select this process with **Bambu Lab H2D 0.4 dual - OFFLINE ONLY** for offline preview.

The Windows Release regression reproduced the original warning, then validated the corrected process without warnings. The 10 × 10 × 4 mm fixture used both tools on all 20 layers. Every model extrusion passed the existing generic tool, band and shared-reach checks. The emitted acceleration values were 300, 500 and 1,500 mm/s². The output retained the offline marker, and the audit found no `M201` machine envelope command.

## Remaining qualification gate

1. Record the target firmware, installed nozzle types, plate, and filament feeds. Obtain a known-good, same-machine vendor export for both A/B assignments and keep it offline for comparison.
2. Confirm the mapping from each filament slot to its physical hotend, then audit the entire file through startup, cross-hotend change, purge and wipe, and shutdown. Check motion and extrusion mode across every transition and keep-out segment.
3. Use the same fixture and profile inputs for a dry comparison of the native implementation and the vendor export. Record discrepancies and preserve the exact reviewed file and configuration. Printer motion requires operator approval after the offline review.
