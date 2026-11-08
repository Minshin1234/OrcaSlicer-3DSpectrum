# Decisions

| ID | Status | Decision and rationale |
| --- | --- | --- |
| D-001 | Required by project brief | Preserve FullSpectrum and reuse existing controls, slicing and export. Leave ordinary slicing behavior intact. |
| D-002 | Proposed for Phase 1 | Start with a printer-independent, generated two-cuboid preview. This isolates fraction and discretization behavior from import and machine variables and allows validation while the full build is being established. |
| D-003 | Required by project brief | Require explicit interface selection. Mechanical contacts may need to remain separable. |
| D-004 | Proposed for Phase 2 investigation | Evaluate an early layer-region adapter alongside the existing painted-region route. Choose using geometry conservation, perimeters, support and physical tool identity tests. |
| D-005 | Session choice | Use the current checkout and leave its unborn branch and configured remote in place. Four matching files support the tag comparison, and a full-tree comparison is pending. |
| D-006 | Prototype direction | Use early material regions for volumetric input. The offline fixture preserved ordered cell labels through per-volume slicing and routed both tools. End-face painting is retained as a surface input and does not encode the same 3D field. A production region adapter and printability checks remain open. |
| D-007 | Experimental implementation | Split early slice regions using the ordered B cell mask and its complement. The opt-in is per object and defaults to empty. Constrain the first adapter to the documented grid-aligned rectangular envelope and reuse physical region registration. |
| D-008 | P2-007 implementation | Assign existing model extrusion paths to physical filaments using the signed-distance field. Generate the walls and infill on the intact solid before material assignment. Keep the early region adapter as a reference. |
| D-009 | P2-010 implementation | Require an explicit plane on a fully shared face before assigning a transition across two parts. Keep the source bodies and require matching base region settings for the first assembly route. |
| D-010 | P0-003 publication | Continue the existing GitHub history from the verified FullSpectrum baseline. Build the publication commit from a separate Git index to preserve local files during reconciliation. |
