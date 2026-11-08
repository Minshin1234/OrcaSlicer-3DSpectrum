# Research notes

## Foundation and licensing

The [FullSpectrum repository](https://github.com/ratdoux/OrcaSlicer-FullSpectrum) identifies Snapmaker Orca as its parent. The [0.9.14 release](https://github.com/ratdoux/OrcaSlicer-FullSpectrum/releases/tag/v0.9.14) describes Local-Z cadence and shared gradient preview behavior. The local [license](../LICENSE.txt) is AGPL-3.0. Preserve copyright and license notices, review corresponding-source obligations for distribution and the network-interaction clause if relevant, and check third-party notices separately before any release. Dependencies declared by the CMake projects include Boost, wxWidgets, TBB and OpenCASCADE. Full dependency license inventory remains open.

## H2D and U1

The [H2D manufacturer setup guide](https://cdn1.bambulab.com/documentation/h2d/en/H2D_Laser_Full_Combo_20250305.pdf) documents both hotends and an AMS feed path to the right hotend. Its specifications list third-party standard G-code support with limits on advanced features. [Bambu Lab's authorization announcement](https://blog.bambulab.com/firmware-update-introducing-new-authorization-control-system-2/) describes Bambu Connect file transfer as a route for third-party software. Confirm the exact H2D firmware and transfer workflow before use. [OrcaSlicer PR #14125](https://github.com/OrcaSlicer/OrcaSlicer/pull/14125) merged upstream H2D/X2D mixed-nozzle-size work. The local resources contain an H2D bed mesh and a filament preset referencing H2D. The scan found no H2D-named machine JSON or H2D entry in the local BBL profile index. These sources establish the hardware and upstream development route. Local dual-nozzle export, accepted transfer method, firmware-specific commands, nozzle reach and purge behavior still need verification. Snapmaker U1 remains the closest existing preset path for a software-only baseline.

## Research queue

- Compare ordered dithering, error diffusion and blue-noise approaches with the same occupancy, cell pitch and volume-based error metrics.
- Evaluate signed distance and spatial acceleration only after the planar specimen works. Include ambiguous contacts, gaps, overlaps and non-manifold surfaces.
- Investigate compatible material pairs and characterize bonding and stiffness against sharp-interface controls using measured specimens.

## Agent compatibility

[Google Antigravity workspace rules](https://www.antigravity.google/docs/rules-workflows?tab=ide) list root AGENTS.md for current 2.0 and CLI. This project uses that canonical entrypoint. On IDE variants that require rule activation, reference AGENTS.md from an Always On workspace rule.
