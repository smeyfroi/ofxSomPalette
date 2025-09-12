# Agent Guidelines for ofxSomPalette

## Build, Lint, Test
- Build examples (GNU Make): `cd example_PaletteFromAudio && make` or `cd example_ContinuousPalettesFromAudio && make`
- Clean rebuild: `make clean && make`
- Xcode: open the `.xcodeproj` in each example and build the shared scheme
- Run examples: `cd example_*/bin && ./example_*`
- Tests: none yet in `tests/` (placeholder). To add Catch2/GoogleTest, place sources under `tests/` and wire into Makefile; single-test run should use a test binary with `-t <name>` (Catch2) or `--gtest_filter=Suite.Name` (gtest)

## Code Style
- Imports/headers: use `#pragma once`; include `ofMain.h` first, then STL, then addon headers (e.g., `ofxSelfOrganizingMap`)
- Formatting: clang-format style (LLVM-like); 4 spaces; braces on same line for functions/types; wrap at ~120 cols
- Types: prefer modern C++ (auto where obvious, `std::array`, `std::unique_ptr`, `std::optional`); use `using` for aliases
- Naming: Classes/structs PascalCase; methods/vars camelCase; constants SCREAMING_SNAKE_CASE or `static constexpr` members
- Initialization: member initializer lists with `{}`; default members in-class when trivial
- Errors: assert invariants in debug; validate allocations (`isAllocated()`); return `std::optional`/`expected`-like patterns instead of sentinel values; log via `ofLogNotice/Warning/Error`
- Concurrency: prefer `ofThread` and `ofThreadChannel` for cross-thread data; guard shared state; stop/join in destructors
- OF usage: check `ofTexture/ofFbo/ofPixels` allocated before use; avoid blocking GL calls on non-GL threads
- File paths: use `ofFilePath` helpers and user home via `ofFilePath::getUserHomeDir()`; avoid hardcoded absolute paths

## Dependencies
- Requires `ofxSelfOrganizingMap` (see `addon_config.mk`); targets OpenFrameworks v0.12+
- No Cursor/Copilot rules found in repo; if added later, mirror key constraints here
