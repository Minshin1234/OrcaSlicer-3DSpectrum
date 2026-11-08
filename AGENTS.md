# Volumetric multi-material FDM project

Develop opt-in, interface-aware material gradients on FullSpectrum. Start with two materials and an explicitly selected planar interface. Preserve conventional slicing and the positions of distinct bodies. Keep geometry, material fractions, deposition decisions and machine commands separate.

## Start a session

1. Check the working directory, Git root, branch, HEAD, status and available build and test tools. An unborn branch can have a populated working tree.
2. Read [state](docs/PROJECT_STATE.md), [handoff](docs/AGENT_HANDOFF.md), [tasks](docs/TASKS.md), relevant [architecture](docs/ARCHITECTURE.md) and recent [log entries](docs/DOCUMENTATION.md), then applicable [decisions](docs/DECISIONS.md) and [testing instructions](docs/TESTING.md).
3. Verify prior claims against source, changes and test evidence. Before substantial edits, summarize phase, task, verified progress, uncommitted work, known issues and next action.
4. Use stable task IDs. When several agents are active, isolate work in separate branches or worktrees where possible and coordinate file ownership before editing shared files.

## Engineering boundaries

- Require explicit consent to blend across an interface. Touching parts may need to stay separate.
- Use the existing model, assignment, slicing and export systems wherever they meet the requirements. Keep the first prototype independent of printer behavior.
- Obtain human approval before major rewrites, scope changes, destructive operations, significant merges, pushes to `main`, releases, other public pushes, external licensing decisions or changes to critical printer safety behavior. The milestone workflow below authorizes commits and pushes to task branches.
- Inspect printer-specific output offline. Obtain human approval before unverified commands or hardware movement. Report physical validation only after printing and inspecting a specimen.

## End a session

Review Git status and changes. Run relevant checks, record actual results and artifacts, append a chronological entry to the [engineering log](docs/DOCUMENTATION.md), and update [tasks](docs/TASKS.md), [state](docs/PROJECT_STATE.md) and [handoff](docs/AGENT_HANDOFF.md). Record failures and remaining uncertainty. The repository is the source of truth across agent environments.

Review [README.md](README.md) before wrapping up a change. Update it in the same change whenever the project's purpose, user workflow, build or setup steps, supported features, validation status, or limitations would otherwise be out of date. Verify any links you add or change.

## Stage and push major changes

After each major implementation milestone, run the relevant checks and review the diff. Stage only files that belong to the task, commit them, and push the commit to a `codex/` task branch on the configured GitHub remote for review. Report the branch, commit, and check results in the handoff. Keep unrelated local changes, credentials, and build outputs out of the commit. A push to `main` or a release requires separate approval.


# Repository Guidelines

## Project Structure & Module Organization
Snapmaker_Orca’s C++17 sources live in `src/`, split by feature modules and platform adapters. User assets, icons, and printer presets are in `resources/`; translations stay in `localization/`. Tests sit in `tests/`, grouped by domain (`libslic3r/`, `sla_print/`, etc.) with fixtures under `tests/data/`. CMake helpers reside in `cmake/`, and longer references in `doc/` and `SoftFever_doc/`. Automation scripts belong in `scripts/` and `tools/`. Treat everything in `deps/` and `deps_src/` as vendored snapshots—do not modify without mirroring upstream tags.

## Build, Test, and Development Commands
Use out-of-source builds:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON` generates build files after dependencies are available. See [platform prerequisites and Windows commands](docs/TESTING.md).
- `cmake --build build --target Snapmaker_Orca --config Release` compiles the app; add `--parallel` to speed up.
- `cmake --build build --target tests` then `ctest --test-dir build --output-on-failure --no-tests=error` runs automated suites.
Platform helpers such as `build_linux.sh`, `build_release_macos.sh`, and `build_release_vs2022.bat` wrap the same flow with toolchain flags. Use `build_release_macos.sh -sx` when reproducing macOS build issues, and `scripts/DockerBuild.sh` for reproducible container builds.

## Coding Style & Naming Conventions
`.clang-format` enforces 4-space indents, a 140-column limit, aligned initializers, and brace wrapping for classes and functions. Run `clang-format -i <file>` before committing; the CMake `clang-format` target is available when LLVM tools are on your PATH. Prefer `CamelCase` for classes, `snake_case` for functions and locals, and `SCREAMING_CASE` for constants, matching conventions in `src/`. Keep headers self-contained and align include order with the IWYU pragmas.

## Testing Guidelines
Unit tests use Catch2 through `cmake/catch2.cmake`. Name specs after the component under test—for example `tests/libslic3r/TestPlanarHole.cpp`—and tag long-running cases so `ctest -L fast` remains useful. Cover new algorithms with deterministic fixtures or sample G-code stored in `tests/data/`. Document manual printer validation or regression slicer checks in your PR when automated coverage is insufficient.

## Commit & Pull Request Guidelines
The history favors concise, sentence-style subject lines with optional issue references, e.g., `Fix grid lines origin for multiple plates (#10724)`. Squash fixups locally before opening a PR. Complete `.github/pull_request_template.md`, include reproduction steps or screenshots for UI changes, and mention impacted presets or translations. Link issues via `Closes #NNNN` when applicable, and call out dependency bumps or profile migrations for maintainer review.

## Security & Configuration Tips
Follow `SECURITY.md` for vulnerability reporting. Keep API tokens and printer credentials out of tracked configs; use `sandboxes/` for experimental settings. When touching third-party code in `deps_src/`, record the upstream commit or release in your PR description and run the relevant platform build script to confirm integration.
