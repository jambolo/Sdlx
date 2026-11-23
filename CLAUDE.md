# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Sdlx is a C++17 library wrapping SDL3 (RAII and abstractions; `Surface` is the only RAII wrapper so far). `README.md` covers usage and `DEVELOPMENT.md` covers architecture, build, test, docs and CI in detail. Read them rather than duplicating them here.

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
- On Linux, a fetched SDL3 fails to configure without the X11 or Wayland development packages. For a headless build, add `-DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF -DSDL_WAYLAND=OFF`, as the workflows do (`SDL_LINUX_ARGS`).
- Tests: each `*.cpp` in `test/` becomes its own executable `Sdlx_<filename>`. The tests run headless, so keep them free of windows and video initialization.
- Doxygen must build with zero warnings. It reads `README.md`, `DEVELOPMENT.md`, `src/` and `include/Sdlx/`.

## Things that aren't obvious from one file

- **Include paths:** sources include headers by bare name (`#include "Sdlx.h"`) through the private include path `include/Sdlx`. Public consumers use `<Sdlx/Sdlx.h>`. A new public header goes in `include/Sdlx/` and must be added to the `PUBLIC_HEADERS` list in `CMakeLists.txt`; that list is the header file set that `cmake --install` copies. A new source file must be added to the `SOURCES` list.
- **SDL3 only:** the code was ported from SDL 1.2. Don't reintroduce `SDL_DisplayFormat`, `SDL_FreeSurface`, `SDL_QUIT` or similar. `SDL3::SDL3` is a public link dependency because the public headers include `<SDL3/SDL.h>`. `SDL3_image::SDL3_image` is private.
- **Dependency fetch flags in `CMakeLists.txt` are deliberate:** SDL3_image is fetched without submodules (`GIT_SUBMODULES ""`), `SDLIMAGE_VENDORED` is off, and AVIF, JXL, TIFF and WebP are disabled. Each fetch is skipped when its target (`SDL3::SDL3`, `SDL3_image::SDL3_image`) already exists, so a parent project can provide its own SDL. Install and export rules are behind the `Sdlx_INSTALL` option, which defaults to on only for the top-level project; when it is on, `SDL_INSTALL` and `SDLIMAGE_INSTALL` are forced on, because Sdlx's export set fails to generate without them. The package config (`cmake/SdlxConfig.cmake.in`) must keep matching the public dependencies.
- **No RLE on color-keyed surfaces:** `loadColorKeyedSurface` deliberately doesn't call `SDL_SetSurfaceRLE`. With SDL3, an RLE-encoded surface that has an alpha channel ignores its color key when blitted. A test covers this.
- **Paletted images are converted:** `loadColorKeyedSurface` converts a paletted image to RGBA32 before it applies the key. On a paletted surface, `SDL_MapSurfaceRGB` returns the nearest palette entry, so the wrong color could become transparent. `loadSurface` does no conversion.
- **Ownership:** `Sprite` does not own its sheet surface, and `AnimatedSprite` does not own its `AnimationGroup`, which is shared. Surfaces from `loadSurface` are freed with `SDL_DestroySurface`.
- **`makeColor` and `makeRect` are deprecated.** Use `SDL_Color{r, g, b, 255}` and `SDL_Rect{x, y, w, h}`. Don't call them from the library.
- **`advanceTime`:** in `AnimatedSprite`, it keeps `frameTime_` as the time spent in the current frame. `MODE_ONCE` freezes at the start of the last frame, and `MODE_PINGPONG` plays the end frame twice when it reverses. That repeat may be intentional, so don't change it unprompted. For `MODE_LOOP` and `MODE_PINGPONG` it first reduces the elapsed time with `fmod` over one repetition (ping-pong repeats after twice the total duration), and returns early if the frames have no duration; both guard against an endless loop.
- **An `AnimatedSprite` may have no image:** when it has no animation group (default-constructed), the group has no animations, or the current animation has no frames. `updateImage` then leaves `rect_` empty, and `AnimatedSprite::draw` draws nothing; a plain `Sprite` with an empty `rect_` draws the whole sheet, which is why `draw` is virtual. The animation functions do nothing in these cases and assert only for a missing group.
- **`eventLoop` returns `bool`:** it returns `false` only when no event was available and `SDL_WasInit(SDL_INIT_EVENTS)` is 0, because then no event can ever arrive. Don't return just because `SDL_WaitEvent` returned false: with a shown window it can do that while events still work, so the loop delays 1 ms and retries.
- **Build settings don't leak:** the default build type is set only for the top-level project, and the `d` debug postfix is a property of the Sdlx target, not `CMAKE_DEBUG_POSTFIX`.
- **Style:** Doxygen comments use `//!` with `@param` for every parameter. Type names are PascalCase (`AnimatedSprite`). Member functions and free functions are camelCase (`get`, `loadSurface`). Class member variables, public and private, are camelCase with a `_` suffix (`frameTime_`, `rect_`), not an `m_` prefix. File-scope variables are camelCase with a `g_` prefix (`g_events`); class-scope and function-scope static variables are camelCase with an `s_` prefix. Compile-time constants (`constexpr` and the like), macros and enum values are UPPER_SNAKE_CASE (`MODE_LOOP`, `BLACK`), with no prefix. A runtime constant (a `const` variable whose value is computed at run time) follows the variable rules above (`imageIndex`). Fields of the plain structs (`Frame`, `Animation`, `AnimationGroup`) have no suffix.

## Releases

When a push to `master` touches `CMakeLists.txt`, `cd.yml` tags `v<version>` if the `project()` `VERSION` has no tag yet, then merges `master` into `develop`. Don't change `VERSION` casually.
