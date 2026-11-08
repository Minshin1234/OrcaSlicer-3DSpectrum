# P2-004 native persistence and offline toolpath audit

Windows x64 Release. Used the P2-003 rectangle, a 4 mm transition, 1 mm cells, 0.2 mm layers, two 0.4 mm nozzle entries and 0.45 mm nominal line width. The generic two-tool fixture disables the prime tower, support generation and arc fitting.

## Project persistence

A native BBS 3MF round trip retained the exact four-value vector `{8.0, 4.0, 1.0, 1.0}` on the opted-in object. A second, ordinary object in the same project loaded without the option. The test generates the project in a temporary directory and exercises native model and config loading.

## Commanded volume and path support

| Metric | Ordered-volume reference | Direct regions |
| --- | ---: | ---: |
| Tool 0 commanded model volume (mm³) | 152.669 | 152.675 |
| Tool 1 commanded model volume (mm³) | 185.860 | 185.865 |
| Tool 1 share of commanded model volume | 54.902% | 54.902% |
| Upper-layer model centerlines (mm) | 3,750.629 | 3,750.630 |
| Centerlines outside previous bead footprint (mm) | 174.360 | 174.401 |
| Of those, moves outside bridge/overhang roles (mm) | 112.533 | 112.574 |
| Tool-change events | 21 | 20 |

Commanded volume is the sum of positive E deltas for model extrusions times the cross-sectional area of 1.75 mm filament. It excludes skirt, brim, support, purge and custom extrusion. The direct route commanded a 54.902% B share versus 55.556% by slice-region volume, a difference of −0.654 percentage points. These are G-code and slice measurements. Physical deposition remains unmeasured.

For the support estimate, each previous-layer model segment is swept by half its recorded bead width with round ends. The audit unions those footprints and clips the next layer’s centerlines against them. It excludes the first layer and reports bridge and overhang roles separately. About 4.65% of upper-layer centerline length was outside the modeled footprint in either route, including about 3.00% of the total outside bridge/overhang roles. This is a geometric screening metric. Sparse infill, bead shape, bridge tension and adhesion need separate evaluation.

## In-plane continuity

Eroded each original per-material component by half the trial width. At a trial width of 0.45 mm, neither route lost a whole component, but 20 component-layer instances split in each route. At 0.90 mm, both routes again had 20 splits and zero whole-component losses. This screens for connections that cannot retain the tested clearance and does not locate or calculate an exact minimum neck width. Both routes have a continuity concern even though their component bounding-box spans are at least 1 mm.

## Printer qualification

The local checkout has no H2D machine preset. The included 0.4 mm U1 machine preset declares four nozzle entries, while the current adapter requires exactly two. The tests use the generic two-tool fixture. Printer-profile, purge and hardware qualification remain open.

The [targeted Windows Release commands](../TESTING.md#check-planar-project-persistence-and-toolpaths) passed both cases. The raw build, metrics and test logs are under ignored `build-phase0/p2-004-*.log`.

Full Windows Release CTest passed 381 of 389. The eight failure names and outcomes matched the P2-003 baseline.
