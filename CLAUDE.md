# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Sdlx is a C++17 library wrapping SDL3 (RAII and abstractions; the RAII wrappers are not written yet). `README.md` covers usage and `DEVELOPMENT.md` covers architecture, build, test, docs and CI in detail. Read them rather than duplicating them here.

## Commands

```sh
cmake -S . -B build                                   # first run downloads SDL3 and SDL3_image
cmake --build build --config Release
cmake -S . -B build -DSdlx_BUILD_TESTS=ON             # enable tests (top-level project only)
ctest --test-dir build -C Release --output-on-failure
ctest --test-dir build -C Release -R Suite.TestName   # single test
cmake -S . -B build -DSdlx_DOXYGEN_OUTPUT_DIRECTORY=docs && cmake --build build --target docs
```

- Keep the build directory path short on Windows. A long path breaks the SDL3_image clone ("Filename too long").
- Tests: each `*.cpp` in `test/` becomes its own executable `Sdlx_<filename>`. The tests run headless, so keep them free of windows and video initialization.
- Lint and format (CI runs them only on pull requests): `clang-format --dry-run --Werror` (the style comes from `.clang-format`), and `clang-tidy -p build`.
- Doxygen must build with zero warnings. It reads `README.md`, `DEVELOPMENT.md`, `src/` and `include/Sdlx/`.

## Things that aren't obvious from one file

- **Include paths:** sources include headers by bare name (`#include "Sdlx.h"`) through the private include path `include/Sdlx`. Public consumers use `<Sdlx/Sdlx.h>`. A new public header goes in `include/Sdlx/`, and a new source file must be added to the `SOURCES` list in `CMakeLists.txt` (headers too, for IDE grouping).
- **SDL3 only:** the code was ported from SDL 1.2. Don't reintroduce `SDL_DisplayFormat`, `SDL_FreeSurface`, `SDL_QUIT` or similar. `SDL3::SDL3` is a public link dependency because the public headers include `<SDL3/SDL.h>`. `SDL3_image::SDL3_image` is private.
- **Dependency fetch flags in `CMakeLists.txt` are deliberate:** SDL3_image is fetched without submodules (`GIT_SUBMODULES ""`), `SDLIMAGE_VENDORED` is off, and AVIF, JXL, TIFF and WebP are disabled. `SDL_INSTALL` and `SDLIMAGE_INSTALL` are forced on, because Sdlx's export set fails to generate without them. The package config (`cmake/SdlxConfig.cmake.in`) must keep matching the public dependencies.
- **No RLE on color-keyed surfaces:** `LoadColorKeyedImage` deliberately doesn't call `SDL_SetSurfaceRLE`. With SDL3, an RLE-encoded surface that has an alpha channel ignores its color key when blitted. A test covers this.
- **Ownership:** `Sprite` does not own its sheet surface, and `AnimatedSprite` does not own its `AnimationGroup`, which is shared. Surfaces from `LoadImage` are freed with `SDL_DestroySurface`.
- **`MakeColor` and `MakeRect` are deprecated.** Use `SDL_Color{r, g, b, 255}` and `SDL_Rect{x, y, w, h}`. Don't call them from the library.
- **`AdvanceTime`:** in `AnimatedSprite`, it keeps `m_frameTime` as the time spent in the current frame. `MODE_ONCE` freezes at the start of the last frame, and `MODE_PINGPONG` plays the end frame twice when it reverses. That repeat may be intentional, so don't change it unprompted.
- **Style:** the code is formatted with clang-format's Microsoft style, set in `.clang-format` (4-space indent, no tabs, 120 columns, braces on their own line, `f(a, b)`). Run `clang-format -i` on the files you change. Doxygen comments use `//!` with `@param` for every parameter.

## Releases

`cd.yml` tags `v<version>` and merges `master` into `develop` when a push to `master` touches `CMakeLists.txt` and the `project()` `VERSION` has no tag yet. Don't change `VERSION` casually.
