# Baseline and validation

## Reproduce on Windows

Use a Visual Studio 2022 x64 Developer PowerShell with the Desktop C++ workload and Windows SDK. Put CMake 3.31 and native Strawberry Perl on PATH. The local Windows wrapper and CI identify Perl as an OpenSSL prerequisite. Use an ignored out-of-source build directory under the Git root. The OpenCV patch step uses a repository-relative path. Remove MSYS2 and MinGW entries from PATH for the MSVC build.

```powershell
$root = Join-Path (Get-Location) 'build-phase0'
$prefix = Join-Path $root 'prefix'
$env:Path = (($env:Path -split ';' | Where-Object { $_ -notmatch '(?i)msys64|mingw' }) -join ';')
cmake -S deps -B "$root/deps" -G 'Visual Studio 17 2022' -A x64 "-DDESTDIR=$prefix" "-DCMAKE_IGNORE_PREFIX_PATH=C:/msys64/ucrt64;C:/msys64/mingw64" -DCMAKE_BUILD_TYPE=Release -DDEP_DEBUG=OFF
cmake --build "$root/deps" --config Release --target deps --parallel 4
cmake -S . -B "$root/app" -G 'Visual Studio 17 2022' -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DORCA_TOOLS=ON -DBUILD_TESTS=ON "-DCMAKE_PREFIX_PATH=$prefix/usr/local" "-DWIN10SDK_PATH=$env:WindowsSdkDir/Include/$($env:WindowsSDKVersion.TrimEnd('\'))"
cmake --build "$root/app" --config Release --target Snapmaker_Orca_app_gui tests --parallel 4
ctest --test-dir "$root/app" -C Release --output-on-failure --no-tests=error
```

Run each step after the previous one succeeds. BUILD_TESTS defaults to OFF. See [build_release_vs2022.bat](../build_release_vs2022.bat) and [build_deps.yml](../.github/workflows/build_deps.yml) for the upstream build conventions. The Linux and macOS wrappers provide additional platform entrypoints.

## Validation gates

- Baseline tests should include existing mixed filament, gradient, 3MF and G-code suites. Use CTest discovery after building, then narrow failures by suite. Capture a conventional single-material slice and a two-tool non-gradient slice before Phase 2.
- Phase 1 should test geometric placement, clipping, clamped endpoints, zero and invalid widths, deterministic assignments, windowed fraction error and XY/XZ previews.
- Phase 2 should test disabled-feature equivalence, region coverage, tool identities, purge behavior and regression fixtures. Inspect exported files offline.
- Phase 3 requires exact machine and firmware identification, reviewed commands, approval, and inspected printed specimens. See [experiments](EXPERIMENTS.md).

## Verify the gradual material transition in Prepare

With one unpainted solid instance and two physical filaments with matching nozzle diameters, open **Cut** in **Prepare** and enable **Material transition**. Turn on **Advance** if Cut is hidden. Drag or rotate the plane, choose A and B, and widen the band using the slider or the exact millimeter field. The two band edges move around the fixed midpoint. Select **Apply transition**, check that the Objects list retains the original solid, and slice. In Preview, select **Filament** to see both materials through the band. Reopen the tool to edit or remove the transition.

The Windows Release regression compares horizontal and tilted 8 mm transitions against an ordinary slice, with model extrusion grouped by signed distance. Native 3MF also checks the plane, width, slot ordering and object bounds after loading. The H2D profile is for offline preview.

**Gradient direction** sets X, Y or Z as the A-to-B axis. The X and Y settings give a vertical plane and a transition across the bed within a layer. Rotate the plane for a diagonal XY direction. For an imported pair, keep both solids in the same object and use the same base slicing settings. In **Material transition**, select **Align to shared face** to place the midpoint on their contact, then choose the two filaments and band width. Alignment puts A on the first part's side and B on the second. Use **Swap A and B** to reverse the material assignments. Select **Apply transition** to save the setup before slicing.

The raw-command audit checks tool selection and both endpoints of each model extrusion against the shared reach and the signed-distance band. The STEP fixture checks automatic alignment, including part order and instance transforms, and rejects gaps, overlaps, partial faces and edge contact. Slicing also rejects a manually shifted plane. The H2D offline profile emits generic tool commands. Physical nozzle assignment requires a hardware-qualified profile.

```powershell
cmake --build build-phase0/app --config Release --target Snapmaker_Orca_app_gui fff_print_tests libslic3r_tests --parallel 3
ctest --test-dir build-phase0/app -C Release -R 'A gradual material transition|A STEP assembly transitions|Native 3MF preserves planar material' --output-on-failure --no-tests=error
```

## Standalone planar sampler

Run from the repository root in PowerShell with CMake, Git, and the Visual Studio C++ workload. This uses the repository's pinned Catch2 configuration and needs network access for its first fetch, or a local Catch2 checkout supplied with `FETCHCONTENT_SOURCE_DIR_CATCH2`.

```powershell
cmake -S tests/planar_gradient -B build-phase0/planar -G 'Visual Studio 17 2022' -A x64
cmake --build build-phase0/planar --config Release --parallel 4
ctest --test-dir build-phase0/planar -C Release --output-on-failure --no-tests=error
```

The same standalone commands run the ordered cell tests. These cover signed indices and phase, the 64-cell uniform-fraction error bound, endpoint labels, two-cuboid occupancy, and invalid input.

## Generate the two-cuboid preview

After building the standalone target, run this from the repository root in PowerShell.

```powershell
& .\build-phase0\planar\Release\planar_gradient_preview.exe build-phase0/planar-preview
```

The executable creates the output directory and writes `planar-preview.svg` and `material-error.csv`. The timing printed to the console excludes file output and is kept separate from the deterministic artifacts. See the [recorded run](test-results/P1-003-preview.md).

## Audit STEP, native 3MF and separate STL placement

The `[placement-audit]` cases generate their inputs with OpenCASCADE and the STL writer under a unique temporary directory. With the existing Windows Release app build configured for tests, run:

```powershell
cmake --build build-phase0/app --config Release --target libslic3r_tests --parallel 4
ctest --test-dir build-phase0/app -C Release -R 'Nested STEP assembly|Separate STL files|Export\+Import geometry' --output-on-failure --no-tests=error
```

See [P2-001 results](test-results/P2-001-placement.md) for fixture coordinates, config requirements and scope.

## Compare offline planar toolpath routes

Rebuild the Windows Release FFF test target and run the three-route fixture.

```powershell
cmake --build build-phase0/app --config Release --target fff_print_tests --parallel 4
ctest --test-dir build-phase0/app -C Release -R 'Compare opt-in ordered volumes' --output-on-failure --no-tests=error
& .\build-phase0\app\tests\fff_print\Release\fff_print_tests.exe '[planar-routing]' -s
```

The last command prints route metrics with the assertions. Temporary G-code is removed after export inspection. See [routing results](test-results/P2-002-routing.md).

## Experimental direct planar regions

Set the per-object `experimental_planar_material_regions` option to four millimeter values: the plane's X offset from the sliced envelope's minimum X, total transition width, XY cell pitch, and Z cell pitch. The object must be a single unpainted, upright rectangular solid with two configured physical slots. The A-to-B direction is positive X in the slicing frame. An absent or empty value disables the adapter.

For the 18 × 6 × 4 mm fixture, the option is `{8.0, 4.0, 1.0, 1.0}`. Run the existing [offline routing fixture](#compare-offline-planar-toolpath-routes) to exercise direct slicing, removal of the option from an already-sliced print, and rejection of a too-small pitch. This is an internal model option without a GUI control.

## Check planar project persistence and toolpaths

```powershell
cmake --build build-phase0/app --config Release --target fff_print_tests libslic3r_tests --parallel 4
ctest --test-dir build-phase0/app -C Release -R 'Compare opt-in ordered volumes|Native 3MF preserves experimental' --output-on-failure --no-tests=error
& .\build-phase0\app\tests\fff_print\Release\fff_print_tests.exe '[planar-routing]' -s
```

The verbose command includes the metrics for both routes. The [run report](test-results/P2-004-qualification.md) records the measurement conventions and profile limits.

## Check the H2D offline profile

```powershell
cmake --build build-phase0/app --config Release --target fff_print_tests --parallel 4
ctest --test-dir build-phase0/app -C Release -R 'H2D offline machine geometry' --output-on-failure --no-tests=error
```

The case applies the overlay to the direct planar fixture and checks tool IDs and extrusion endpoints. Use the [profile research report](test-results/P2-005-h2d-profile.md) for the source revisions and hardware support gaps.

For the desktop offline preview, use **File → Import → Import Configs** to import the [conservative 0.20 mm process preset](../resources/profiles/offline/H2D%20offline%200.20%20mm%20-%20conservative.json). Select it with **Bambu Lab H2D 0.4 dual - OFFLINE ONLY**. The dedicated regression compares validation before and after applying the process and audits the resulting acceleration commands and two-tool model extrusion. The [H2D qualification report](test-results/P2-014-h2d-offline-qualification.md) records the remaining hardware gates.

```powershell
ctest --test-dir build-phase0/app -C Release -R 'H2D offline' --output-on-failure --no-tests=error
```

To run the full H2D suite including the vendor export contract audit across both standard and reversed filament slots:

```powershell
ctest --test-dir build-phase0/app -C Release -R 'H2D vendor export contract|H2D offline' --output-on-failure --no-tests=error
```
