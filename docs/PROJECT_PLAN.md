# Volumetric multi-material FDM project plan

Build a CAD-agnostic workflow for assigning physical materials to bodies, explicitly selecting an interface, and controlling composition across a transition of known width. Keep geometry and fraction sampling independent of machine behavior. Start with two materials. A robotic gripper is the eventual demonstrator.

| Phase | Completion gate | Rough effort |
| --- | --- | --- |
| 0. Baseline | Source map, provenance, build and test evidence, risk assessment, and handoff | 1–3 engineering days plus dependency setup |
| 1. Planar preview | Two positioned cuboids, adjustable transition, deterministic 3D assignments, preview and error metrics | 2–5 days |
| 2. Slicer integration | Opt-in assignments reach inspected toolpaths and ordinary slicing passes regression checks | 1–3 weeks |
| 3. Physical printing | Human-approved machine configuration and inspected specimens | Hardware dependent |
| 4. Advanced geometry | Curved interfaces, tolerances, overlapping transitions and geometry regressions | Scope after Phase 3 |
| 5. Characterization | Sharp-interface controls, repeated measurements and uncertainty | Equipment dependent |
| 6. Robotics | Gripper with measured functional outcomes | Scope after characterization |

Phase 1 can use generated geometry. Preserve distinct bodies and shared coordinates. Explicit interface selection is required even for touching bodies. CAD import fixtures, mechanical inference, printer commands and geometry generalization are outside the first implementation. Extend to more materials only after the two-material path is stable.
