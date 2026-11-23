# Developing Sdlx

## Architecture

Sdlx is a small C++17 library in the `Sdlx` namespace, built on SDL3 and SDL3_image. It is intended to provide RAII
ownership of SDL resources and abstractions over SDL's C API; the RAII wrappers are not implemented yet.

Current pieces:

- **Helpers** (`Sdlx.h`, `Sdlx.cpp`)
  - `LoadImage` and `LoadColorKeyedImage` load a file with SDL3_image (`IMG_Load`). The caller owns the returned
    surface and frees it with `SDL_DestroySurface`. SDL3 has no "display format", so no conversion is done.
  - `EventLoop` runs until `SDL_EVENT_QUIT`. With no idle callback it blocks in `SDL_WaitEvent`. If an idle callback
    returns `false` it is not called again until after the next event.
  - `MakeColor` and `MakeRect` are deprecated. Use brace initialization (`SDL_Color{ r, g, b, 255 }`).
- **Sprites** (`Sprite.h`, `Sprite.cpp`)
  - `Sprite` is a rectangle on a sheet `SDL_Surface`, with an origin offset and a float position. `Draw` blits it
    to a destination surface. It does not own the sheet.
  - `AnimatedSprite` adds time-based animation. An `AnimationGroup` (shared by many sprites, not owned by them)
    holds the image rectangles and the animations. Each `Animation` is a list of frames with durations and a mode
    (`MODE_ONCE`, `MODE_LOOP` or `MODE_PINGPONG`). `AdvanceTime` is the core: it tracks the current frame and the
    time spent in it, and updates the sprite's rectangle and offset.
  - `SpriteFactory` is a stub. Its load functions return 0.

### Dependencies

SDL3 and SDL3_image are linked through CMake targets and fetched with `FetchContent` only when they are not
already installed (`FIND_PACKAGE_ARGS`). `SDL3::SDL3` is a public dependency, because the public headers include
`<SDL3/SDL.h>`. `SDL3_image::SDL3_image` is private. Notes on the fetch:

- SDL3_image is fetched without its git submodules, with `SDLIMAGE_VENDORED` off and the AVIF, JXL, TIFF and WebP
  formats disabled. Cloning the submodules fails on Windows when the build path is long. PNG, JPEG and the other
  built-in formats still work.
- `SDL_INSTALL` and `SDLIMAGE_INSTALL` are forced on, so a fetched SDL can be part of Sdlx's export set.
  Installing Sdlx therefore also installs a fetched SDL3 and SDL3_image.

## File structure

```
include/Sdlx/        Public headers (include as <Sdlx/Sdlx.h>)
src/                 Implementation files
test/                GoogleTest tests; each .cpp becomes its own executable
cmake/               Package config template (SdlxConfig.cmake.in)
Doxyfile.in          Doxygen configuration, filled in by CMake
.github/workflows/   CI and CD
```

The source files include the headers by bare name (`#include "Sdlx.h"`). This works through the library's private
include path, `include/Sdlx`.

## Building

```sh
cmake -S . -B build
cmake --build build --config Release
```

The first configure downloads SDL3 and SDL3_image if they aren't installed. On Windows, keep the build directory
path short, since a long path can break the SDL3_image clone.

The library is static unless you pass `-DBUILD_SHARED_LIBS=ON`. Debug builds use the postfix `d`.

## Testing

Tests use GoogleTest, which CMake fetches. They are off unless you enable them, and can be enabled only when Sdlx
is the top-level project:

```sh
cmake -S . -B build -DSdlx_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

`test/` currently has only its `CMakeLists.txt`. Every `*.cpp` file you add there is built as its own executable
named `Sdlx_<filename>`, and its tests are discovered automatically.

Run a single test, by name or by GoogleTest filter:

```sh
ctest --test-dir build -C Release -R Suite.TestName
build/Release/Sdlx_<filename> --gtest_filter=Suite.TestName    # multi-config generators
build/test/Sdlx_<filename> --gtest_filter=Suite.TestName       # single-config generators
```

## Documentation

Doxygen and Graphviz (`dot`) must be installed. Setting the output directory turns on the `docs` target:

```sh
cmake -S . -B build -DSdlx_DOXYGEN_OUTPUT_DIRECTORY=docs
cmake --build build --target docs
```

The HTML is written to `docs/html`. Doxygen reads `README.md` (as the main page), `src/` and `include/Sdlx/`. Keep
the build free of Doxygen warnings.

When CI builds the documentation on `master`, it is deployed to <https://jambolo.github.io/Sdlx>. This requires
GitHub Pages to be enabled for the repository, with the source set to GitHub Actions.

## CI/CD

Branches are `master`, `develop` and `release/**`.

| Job | Runs | What it does |
|---|---|---|
| Build & test | every push and pull request | Configure, build and `ctest` on Ubuntu and Windows |
| Lint and format | pull requests only | `clang-format --dry-run --Werror` and `clang-tidy` |
| Docs | `master` only, after build and test | Builds Doxygen and deploys to GitHub Pages |
| Coverage | `develop` only, after build and test | Builds with `--coverage`, uploads to Codecov |

Formatting uses `clang-format` with its LLVM fallback style, because the repository has no `.clang-format`.
Coverage upload needs a `CODECOV_TOKEN` repository secret.

### Releases

`cd.yml` runs on pushes to `master` that touch `CMakeLists.txt`. It builds and tests, reads the version from the
`project()` call in `CMakeLists.txt`, and if the tag `v<version>` doesn't exist yet, creates it and merges `master`
into `develop`. To release, bump `VERSION` in `CMakeLists.txt` and merge to `master`.
