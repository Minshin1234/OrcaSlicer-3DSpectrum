# OrcaSlicer 3DSpectrum

3DSpectrum is an experimental FDM slicer for printing gradual transitions between two filaments within one part. Set a plane where the materials should meet, then choose how wide the transition should be. The goal is to control where material properties change inside a print, including at interfaces between CAD bodies.

## What works today

- **Adjustable transition.** Position or rotate the plane, assign filaments A and B, and set the transition width with a slider or an exact millimeter value. The plane is the 50/50 target. The band edges mark the pure A and pure B targets.
- **Within-layer gradients.** A vertical or tilted plane can route both filaments within the same layer. The slicer approximates the target fractions by assigning short sections of walls and infill to either filament.
- **Shared CAD faces.** For two touching solids imported as one object, the slicer can align the plane to a fully shared flat face. Both CAD bodies keep their original placement and slice as one continuous part across the interface. The two solids need matching base print settings.
- **Explicit opt-in.** Alignment previews the plane. Applying the transition enables it for the selected object and saves the settings in the native 3MF project.

![Simulated two-cuboid fixture showing interleaved material cells in XY and XZ sections](docs/assets/planar-preview.svg)

*Standalone simulation of a 4 mm transition. See the [fixture and material fraction measurements](docs/test-results/P1-003-preview.md).*

## Import STEP components

Import a multi-body STEP or select several STEP files at once. Choose **Keep STEP component placement** if the files share a CAD coordinate system. Each body remains selectable in the Objects panel, where you can assign its filament. Use **Add part → Load** to append STEP bodies while the existing object still has a consistent STEP frame. For files exported with independent origins, import separately, align with **Move**, then use **Assemble**. See the [STEP workflow and placement checks](docs/TESTING.md#audit-step-native-3mf-and-separate-stl-placement).

## Try it in Prepare

1. Select one solid or one object with two solid parts. Configure two physical filaments with matching nozzle diameters.
2. Turn on **Advance** if needed, then open **Cut → Planar → Material transition**.
3. Move or rotate the plane. For two parts that share a complete flat face, choose **Align to shared face**.
4. Pick filaments A and B, set the **Transition width**, and select **Apply transition**.
5. Slice the plate and select **Filament** in Preview to see where each filament is assigned.

The Windows prototype has been checked with horizontal, tilted, diagonal XY and two-body STEP fixtures. Try the same two-body model using the [STEP fixture](tests/data/planar_gradient/SharedFace.step), or see the [filament preview](docs/assets/p2-011-shared-face-filament-preview.jpg).

## H2D offline testing

The Bambu Lab H2D profile and its companion process preset are for offline slicing and preview. The companion preset resolves the acceleration warning under the generic offline limits. See the [setup and test instructions](docs/TESTING.md#check-the-h2d-offline-profile).

The Bambu Studio reference audit verified logical tool selection in both filament assignments and checked model and prime-tower extrusion endpoints against the shared reach. 3DSpectrum still exports generic tool commands for this profile. Native H2D tool changes, physical left/right nozzle mapping, and purge behavior remain unqualified. See the [reference audit](docs/test-results/P2-015-h2d-vendor-export-audit.md).

## Where the project is going

Next are resizable manual planes and gradients driven by the materials on either side of a selected CAD contact. Later work will extend those gradients to curved interfaces and assemblies with multiple contacts. Printer qualification and specimen comparisons remain separate milestones. A robotic gripper with rigid and flexible regions is the planned demonstrator. See the [task backlog](docs/TASKS.md), [current status](docs/PROJECT_STATE.md), and [architecture](docs/ARCHITECTURE.md).

## Build and test

Build from source using the [Windows Release recipe](docs/TESTING.md#reproduce-on-windows). The [validation guide](docs/TESTING.md) covers the standalone planar sampler, native 3MF round-trips, and offline G-code checks. The [engineering log](docs/DOCUMENTATION.md) records the test results and known baseline failures.

## Credits and license

3DSpectrum builds on [FullSpectrum](https://github.com/ratdoux/OrcaSlicer-FullSpectrum) by ratdoux, which builds on [Snapmaker Orca](https://github.com/Snapmaker/OrcaSlicer) and [OrcaSlicer](https://github.com/OrcaSlicer/OrcaSlicer). FullSpectrum's virtual mixed-color filaments remain a separate feature, with color blending powered by [FilamentMixer](https://github.com/justinh-rahb/filament-mixer). See [LICENSE.txt](LICENSE.txt) for the GNU Affero General Public License, version 3.
