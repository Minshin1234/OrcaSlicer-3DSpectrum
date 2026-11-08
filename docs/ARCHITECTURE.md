# Architecture map

The map describes the local source by inspection. Runtime coverage is tracked separately in [testing](TESTING.md).

| Capability | Existing source and reuse |
| --- | --- |
| Physical assignment | [GUI_ObjectList.cpp](../src/slic3r/GUI/GUI_ObjectList.cpp) uses the object and volume `extruder` setting. [Model.hpp](../src/libslic3r/Model.hpp) separates mesh-bearing ModelVolume from placed ModelInstance. Preserve existing assignment and transforms. |
| Virtual filament recipes | [MixedFilament.hpp](../src/libslic3r/MixedFilament.hpp) defines MixedFilamentDefinition, weighted components, behavior and MixedFilamentManager. [MixedFilamentDialog.cpp](../src/slic3r/GUI/MixedFilamentDialog.cpp) edits recipes. Treat virtual recipe IDs and physical tool IDs as separate domains. |
| Gradient and Local-Z | [MixedFilament/Gradient.cpp](../src/libslic3r/MixedFilament/Gradient.cpp) normalizes components, weights and stops. [PrintObjectSlice.cpp](../src/libslic3r/PrintObjectSlice.cpp) calls sample_mixed_gradient and sample_mixed_gradient_local_z and builds independent cadence. [Print.cpp](../src/libslic3r/Print.cpp) accounts for Local-Z passes in wipe-tower planning. [MixedFilament/GradientPreview.cpp](../src/libslic3r/MixedFilament/GradientPreview.cpp) implements shared sampling over progress within a configured domain. No explicit two-body, signed-distance transition contract was identified in this path. |
| Painting | [GLGizmoMmuSegmentation.cpp](../src/slic3r/GUI/Gizmos/GLGizmoMmuSegmentation.cpp) edits facet annotations. ModelVolume holds mmu_segmentation_facets and can hold image-map data. [MultiMaterialSegmentation.cpp](../src/libslic3r/MultiMaterialSegmentation.cpp) and PrintObjectSlice.cpp turn painting into per-layer regions. This is a candidate downstream consumer for geometry-derived masks after checking clipping and precedence. [ImageMap/VolumeData.hpp](../src/libslic3r/ImageMap/VolumeData.hpp) holds image-map settings and data. [ImageMap/AdaptiveLocalZRenderer.cpp](../src/libslic3r/ImageMap/AdaptiveLocalZRenderer.cpp) and [ImageMap/PerimeterEnvelopeRenderer.cpp](../src/libslic3r/ImageMap/PerimeterEnvelopeRenderer.cpp) are further reuse candidates for studying mask-to-path constraints. |
| Assembly and persistence | [Model.cpp](../src/libslic3r/Model.cpp) routes imports. [Format/3mf.cpp](../src/libslic3r/Format/3mf.cpp) and [Format/bbs_3mf.cpp](../src/libslic3r/Format/bbs_3mf.cpp) handle 3MF. BBS 3MF applies build transforms to instances and handles volume transforms. [FullSpectrum3mf](../src/libslic3r/Format/FullSpectrum3mf) provides typed recipe persistence. [STEP.cpp](../src/libslic3r/Format/STEP.cpp) uses STEPCAFControl_Reader, accumulates assembly locations and can add bodies as volumes. Verify actual relative placements through import fixtures and GUI placement. Separately exported STL has no assembly placement metadata to recover. |
| Toolpaths and export | [Print.cpp](../src/libslic3r/Print.cpp) orchestrates Print::process. [PrintObject.cpp](../src/libslic3r/PrintObject.cpp) makes perimeters and infill. [ToolOrdering.cpp](../src/libslic3r/GCode/ToolOrdering.cpp) plans tool use. [GCode.cpp](../src/libslic3r/GCode.cpp) implements do_export, process_layer and set_extruder. Reuse region and toolpath generation, preview and export once the integration boundary is validated. |
| Existing tests | [tests/CMakeLists.txt](../tests/CMakeLists.txt) configures Catch2. Relevant tests include test_mixed_filament.cpp, test_gradient_port.cpp, test_gradient_preview.cpp, test_3mf.cpp and test_gcodewriter.cpp. |

## Proposed boundaries

1. Geometry supplies two eligible bodies in a shared specimen-local millimeter frame and an explicitly selected plane. Define the interface normal from A to B. Limit eligibility to the selected bodies and their finite shared interface neighborhood.
2. The field evaluates B fraction as `clamp(0.5 + signed_distance_mm / width_mm, 0, 1)` for positive width. Zero width gives a sharp split with an explicit tie convention. Signed distance uses a unit normal. Geometry, fractions and material identities remain separate.
3. A discretizer assigns A or B over an explicit 3D cell grid with deterministic origin and phase. Pitch and layer height are inputs. Preserve pure endpoints and report errors in windows rather than treating exact agreement at each cell as meaningful. Preview cells are candidates for deposition and require validation against supported extrusion paths.
4. Preview reports material labels, target fraction and achieved fraction. In Phase 2 compare assigning clipped regions before perimeter/infill generation against using existing painted-region masks. Preserve perimeters, support and purge logic. Avoid changing tool switching to encode the field.
5. A printer adapter resolves physical tools from existing machine settings only after assignment and printability checks. Keep interface settings opt-in and version their persistence if introduced.

Risks include misplaced separately imported bodies, unintended contact fusion, overlapping or gapped geometry, loss of narrow mask fragments, biased boundary cells, unsupported islands, mismatches between nominal cells and extruded volume, and switching or purge overhead. Gate each with fixtures or measured toolpath evidence before advancing.

## Standalone planar fraction sampler

[PlanarFraction](../src/libslic3r/PlanarGradient/PlanarFraction.hpp) takes a plane point, an A-to-B normal and total transition width in millimeters. It normalizes the normal, returns the clamped B fraction, and selects B at the zero-width tie. It rejects non-finite inputs, a zero normal and a negative width. The field is unbounded. The caller is responsible for body eligibility and clipping.

The [two-cuboid fixture](../tests/data/planar_gradient/TwoCuboidFixture.hpp) defines separate A and B bounds in one translated coordinate frame. They share the selected face at x = 12 mm and have disjoint interiors. The standalone [test project](../tests/planar_gradient/CMakeLists.txt) builds the sampler without linking the application.

## Standalone ordered cell assignment

[OrderedCells](../src/libslic3r/PlanarGradient/OrderedCells.hpp) maps signed grid indices to cell centers using `origin + (index + 0.5) * pitch` per axis. The phase shifts the threshold pattern in index space and leaves physical centers fixed. The caller determines occupancy and supplies a B fraction only for occupied cells. An empty cell receives no label.

The pattern repeats every four cells on each axis. Two octant ranks produce a permutation of ranks 0–63. A cell selects B when its fraction meets or exceeds the midpoint threshold `(rank + 0.5) / 64`. At a uniform fraction f over a complete occupied period, the B count is `floor(64 * f + 0.5)`, so the absolute fraction error is at most `1/128` (0.78125 percentage points). Partial periods and varying fields have no such guarantee. The two-cuboid test uses half-open occupancy by cell center. This is a prototype sampling convention and provides no extrusion or manufacturability guarantees.

## Standalone preview and error reporting

The [fixture preview executable](../tests/planar_gradient/planar_gradient_preview.cpp) composes the existing field and ordered grid. It samples cell centers inside the two cuboids and uses the resulting labels for both the SVG and CSV. It reports actual-minus-target B fraction across the whole fixture, across the transition, and in 1 mm X slabs. Target is the mean field value over occupied centers. Actual is the occupied-cell share assigned to B. Each cell has the same nominal volume. See the [measured result](test-results/P1-003-preview.md).

## Placement and material assignment audit

The [P2-001 fixture](../tests/libslic3r/test_assembly_placement.cpp) checks the STEP reader and native BBS 3MF project path. It calculates world bounds from mesh vertices and the composed instance and volume matrices. A nested translated assembly with a rotated child imports as two named volumes, and its world-space bounds and the assigned per-volume extruder IDs survive a native project round trip with two physical filaments declared.

The STL reader preserves the coordinates baked into vertices. The desktop [Plater import path](../src/slic3r/GUI/Plater.cpp) can center non-project imports and offer multipart conversion, which [Model::convert_multipart_object](../src/libslic3r/Model.cpp) compensates for using the stored origin translation. The fixture verifies raw STL imports only. Coordinates omitted from the file cannot be recovered by grouping. See [run evidence](test-results/P2-001-placement.md).

## Offline toolpath routing comparison

The [P2-002 fixture](../tests/fff_print/test_planar_routing.cpp) feeds ordered cell labels to the existing volume slicer as contiguous X runs with per-volume extruder IDs. The same envelope is also sliced unpainted and with one end face painted. Painting enters through facet projection and segmentation in `PrintObject::slice_volumes`. That route has no direct representation for buried ordered cells. An early region adapter could consume masks without building cell meshes, but must preserve the envelope and retain material assignments through perimeter and infill generation.

The comparison uses the normal `Print::apply`, slicing, perimeter, and offline G-code export paths. It does not add a production setting. See the [fixture results](test-results/P2-002-routing.md).

## Experimental direct region adapter

[PlanarRegions](../src/libslic3r/PlanarGradient/PlanarRegions.cpp) uses ordinary layer contours as an envelope. It unions the ordered B grid cells into a mask for each Z band, intersects that mask with the envelope, and assigns the complement to A. The adapter runs before perimeter generation. [Print::apply](../src/libslic3r/PrintApply.cpp) registers physical regions using existing region configuration logic and invalidates slices when the option changes. It does not run facet projection for the volumetric input.

The first implementation requires a constant, grid-aligned rectangular footprint. It rejects conflicting paint, compensation and interlocking. The pitch and layer-band guards constrain this fixture class. They do not qualify paths or material adhesion on hardware.

## Gradual transition on a single solid

[PlanarTransition](../src/libslic3r/PlanarGradient/PlanarTransition.cpp) stores an object-local plane point and normal, a physical band width, and two physical slot IDs. The centered slice transform puts the plane in the same frame as the extrusion paths. The existing layer contours, walls, and infill are generated first. At export, the transition assigns portions of those paths to either filament using the signed-distance field and the ordered threshold grid. Neighboring portions with the same assignment are kept together. Whole loops keep their normal seam handling, and mixed loops emit open paths at material changes. Tool ordering reserves the two slots where the layer can use both.

The first route uses an XY pitch of three nozzle diameters and a Z pitch equal to nominal layer height. Physical tool routing remains the responsibility of the selected printer profile. See the [acceptance plan](TASKS.md#p2-007-implementation-plan).

For P2-009, the X and Y direction buttons set the plane normal in the build plate and switch the numeric plane position to the corresponding axis. A normal with zero Z component gives the same target material field across layers. The ordered assignment can still vary between layers.

For P2-010, a single object may contain two unpainted solids with matching base print settings. In the transformed slicing frame, the chosen plane must put the bodies on opposite sides. The mesh triangles on the plane must form coincident faces that cover both bodies' projections. This conservative check limits the transition to fully shared planar contacts. The model volumes and their transforms stay intact. The normal slicer generates the combined envelope before the existing path assignment runs.

P2-011 proposes the plane from mesh triangles on a supporting face. Candidates use the placed instance geometry so the contact tolerance stays in physical millimeters. The detector verifies the proposed plane with the same geometry check used by slicing, then returns the plane in object coordinates with its normal pointing toward the second body. Aligning updates only the preview. Applying uses the existing transition setting and print validation.

## STEP components and shared material fields

P2-017 uses the existing STEP loader and `ModelVolume` for each solid. Batch assembly copies the raw STEP body placements and centers the assembly once as a whole. Adding parts uses the translation from the saved STEP mesh origin to the current volume offset. All destination parts must agree on the translation and retain their original orientation in object space. A common instance transform can move or rotate the assembly without changing that test. The saved source metadata and per-volume filament config supply the needed information after native 3MF reload. The user confirms that the separate files share a CAD frame because file coordinates alone cannot establish that fact.

P2-018 through P2-021 will reuse the existing layer-path assignment. The field supplies a target B fraction at a point in the slicing frame. A manual plane supplies signed distance and an optional tangential footprint. A component contact supplies a signed distance to the selected interface, with sign set by component ownership. Keep contact eligibility, fraction evaluation, ordered cells, and machine routing separate. Apply gradients only to explicitly selected interfaces.

For an asymmetric band with positive extents `w_A` and `w_B`, map signed distance `d` to a clamped coordinate `t`. Use `t = 0.5 + 0.5 d / w_A` on side A and `t = 0.5 + 0.5 d / w_B` on side B. This preserves the 50/50 contact even when extents differ. Linear uses `f_B = t`. Smoothstep uses `f_B = t²(3 − 2t)`. A symmetric power curve `tᵖ / (tᵖ + (1 − t)ᵖ)` with positive `p` changes steepness while preserving endpoints and midpoint. A zero extent makes that side pure up to the contact. A zero-width band on both sides selects a sharp boundary.

For a finite manual plane, use its local tangent and bitangent for the rectangle and for rotated fit. Outside the footprint, use the existing assignment for that path. A tangential taper can interpolate from the base assignment to the plane field near the edge. The footprint changes material assignment only. Preserve the geometry and the existing unlimited-plane behavior for projects without extent data. Reject ambiguous contact ownership and overlapping configured bands until conflict rules are validated.
