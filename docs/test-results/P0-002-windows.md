# P0-002 Windows baseline attempt

2026-11-06 EST. Windows x64, Visual Studio Build Tools 17.14, MSVC 19.44.35228.0, SDK 10.0.26100.0, CMake 3.31.6-msvc6, Git 2.53.0.windows.2. The source tree had an unborn main. Commands below use `cmake` and `ctest` as shorthand for the Visual Studio bundled executables.

| Command | Observed result |
| --- | --- |
| `cmake -S . -B "$env:TEMP/3DSpectrum-P0-app" -G "Visual Studio 17 2022" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DORCA_TOOLS=ON "-DCMAKE_PREFIX_PATH=$env:TEMP/3DSpectrum-P0-prefix/usr/local"` | Exit 1. Compiler detection passed. The install prefix did not exist, and Boost 1.83+ was missing. |
| `cmake --build "$env:TEMP/3DSpectrum-P0-app" --config Release --target Snapmaker_Orca` | Exit 1. MSBuild reported no generated project. |
| `ctest --test-dir "$env:TEMP/3DSpectrum-P0-app" -C Release --output-on-failure` | Exit 0 with zero tests discovered. This ran no tests. |
| `cmake -S deps -B "$env:TEMP/3DSpectrum-P0-deps" -G "Visual Studio 17 2022" -A x64 "-DDESTDIR=$env:TEMP/3DSpectrum-P0-prefix" -DCMAKE_BUILD_TYPE=Release -DDEP_DEBUG=OFF` | Exit 0. Discovered EXPAT in the MSYS2 UCRT prefix. |
| `cmake --build "$env:TEMP/3DSpectrum-P0-deps" --config Release --target deps --parallel 4` | Failed prerequisite steps. OpenSSL could not find Perl. Boost iostreams included MinGW headers after finding MSYS2 zstd. OpenCV could not resolve the generated relative patch path outside the worktree. |
| `cmake -S deps -B "$env:TEMP/3DSpectrum-P0-clean-deps" -G "Visual Studio 17 2022" -A x64 "-DDESTDIR=$env:TEMP/3DSpectrum-P0-clean-prefix" "-DCMAKE_IGNORE_PREFIX_PATH=C:/msys64/ucrt64;C:/msys64/mingw64" -DCMAKE_BUILD_TYPE=Release -DDEP_DEBUG=OFF` | Exit 0 after removing MSYS2/MinGW from PATH. Fell back to bundled EXPAT. |
| `cmake -S . -B "$env:TEMP/3DSpectrum-P0-test-app" -G "Visual Studio 17 2022" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DORCA_TOOLS=ON -DBUILD_TESTS=ON "-DCMAKE_PREFIX_PATH=$env:TEMP/3DSpectrum-P0-clean-prefix/usr/local"` | Exit 1 at Boost discovery. The clean dependency prefix had not been built. |
| `cmake --build "$env:TEMP/3DSpectrum-P0-test-app" --config Release --target tests` | Exit 1. MSBuild reported no generated tests project. |
| `ctest --test-dir "$env:TEMP/3DSpectrum-P0-test-app" -C Release --output-on-failure --no-tests=error` | Exit 8. Zero tests discovered. |
| `git apply --check --directory=build-phase0/opencv-patch-check --ignore-space-change --whitespace=fix deps/OpenCV/0001-vs2022.patch deps/OpenCV/0002-clang19-macos.patch` | Exit 0 using copied files from OpenCV 4.6.0 for every patch target. An earlier dry-run omitted the bundled libpng file and exited 1. |
| `cmake --build build-phase0/deps --config Release --target dep_OpenSSL --parallel 4` | Targeted retry after configuring with the clean options above, the x64 Visual Studio developer environment and verified portable Strawberry Perl 5.42.3. OpenSSL completed VC-WIN64A configuration and reached MSVC compilation. No terminal retry result had been observed at handoff. |

The initial dependency log is in `%TEMP%/3DSpectrum-P0-deps-build.log`. The OpenSSL retry log is in `%TEMP%/3DSpectrum-P0-openssl.log`. The downloaded dependency archives use the ignored `deps/DL_CACHE/` directory. [Testing](../TESTING.md) contains the updated recipe.


## Completed Windows Release baseline

2026-11-06, VS 2022 x64 Developer Command Prompt, MSVC 19.44.35228.0, CMake 3.31.6-msvc6, Windows SDK 10.0.26100.0 and native Strawberry Perl 5.42.3. The build reused the repo-local `build-phase0/deps` work from the OpenSSL retry. MSYS2/MinGW PATH entries were removed. The commands use the prefix and options in [TESTING.md](../TESTING.md).

| Step | Result |
| --- | --- |
| Dependency configure with `-DDEP_DEBUG=OFF` and the MSYS2 prefix exclusions | Exit 0 |
| `cmake --build build-phase0/deps --config Release --target deps --parallel 4` | Exit 0. OpenSSL, Boost and OpenCV completed in the corrected layout. |
| App configure with `-DBUILD_TESTS=ON` and `CMAKE_PREFIX_PATH` pointing at `build-phase0/prefix/usr/local` | Exit 0 |
| `cmake --build build-phase0/app --config Release --target Snapmaker_Orca_app_gui tests --parallel 4` | Exit 0. Built the GUI launcher, `Snapmaker_Orca.dll` and the five existing test executables. |
| `ctest --test-dir build-phase0/app -C Release --output-on-failure --no-tests=error` | Exit 8. 377 passed of 385. Eight failed, including one crash. 53.78 s. |
| `ctest --test-dir build-phase0/app -C Release --rerun-failed --output-on-failure` | Exit 8. All eight failures reproduced. 9.35 s. |
| Standalone [planar sampler run](../TESTING.md#standalone-planar-sampler) | Exit 0. Five passed of five. 0.29 s. |

| CTest case | Observed failure |
| --- | --- |
| 36, DynamicPrintConfig serialization | Configuration differed after round trip, `test_config.cpp:258`. |
| 166, mixed-filament `num_physical` differential | Catch2 reported a case failure with its single assertion passed. The case has `[!shouldfail]`. |
| 256, Placeholder parser scripting | Segmentation violation, `test_placeholder_parser.cpp:8`. |
| 310, Complete cached slots override direct nozzle data atomically | Expected empty flows, `test_sswcp_protocol.cpp:116`. |
| 342, Persistent image maps generate geometry-first perimeter path modulation V2 | Several width checks failed. One run reported 0.75 mm where more than 0.90 mm was expected, `test_print.cpp:1098`. |
| 360 and 361, mixed print and brim-introduced extruder bed temperature | 35 °C generated vs 65 °C expected, `test_bed_temperature.cpp:66,91`. |
| 362, `bed_temperature_initial_layer_single` expansion | Expected `M190 S65` absent, `test_bed_temperature.cpp:113`. |

Artifacts and raw logs are under ignored `build-phase0/`: `deps-config.log`, `deps-build.log`, `app-config.log`, `app-build.log`, `ctest.log`, `ctest-rerun.log`, and the `planar-*.log` files. The GUI launcher is `app/src/Release/snapmaker-orca.exe`. The new sampler is in a separate build graph, so the existing-suite results describe the application baseline.
