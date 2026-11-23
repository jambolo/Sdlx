# Developing Sdlx

## Architecture

Sdlx is a small C++17 library in the `Sdlx` namespace, built on SDL3 and SDL3_image. It is intended to provide RAII
ownership of SDL resources and abstractions over SDL's C API; the RAII wrappers are not implemented yet.

Current pieces:

- **Helpers** (`Sdlx.h`, `Sdlx.cpp`)
  - `LoadSurface` and `LoadColorKeyedSurface` load a file with SDL3_image (`IMG_Load`). The caller owns the returned
    surface and frees it with `SDL_DestroySurface`. SDL3 has no "display format", so `LoadSurface` returns the image
    in the format in which it was loaded. `LoadColorKeyedSurface` converts a paletted image to 32-bit RGBA before it
    applies the key, because on a paletted surface a color maps to the nearest palette entry, which may not be the
    key color. The functions are not named `LoadImage`, because `<windows.h>` defines a macro with that name.
  - `EventLoop` runs until `SDL_EVENT_QUIT`. With no idle callback it blocks in `SDL_WaitEvent`. If an idle callback
    returns `false` it is not called again until after the next event. It returns `true` when the event handler ends
    the loop, and `false` if waiting for an event fails, which happens when the SDL events subsystem is not
    initialized.
  - `MakeColor` and `MakeRect` are deprecated. Use brace initialization (`SDL_Color{r, g, b, 255}`).
- **Sprites** (`Sprite.h`, `Sprite.cpp`)
  - `Sprite` is a rectangle on a sheet `SDL_Surface`, with an origin offset and a float position. `Draw` blits it
    to a destination surface. It does not own the sheet.
  - `AnimatedSprite` adds time-based animation. An `AnimationGroup` (shared by many sprites, not owned by them)
    holds the image rectangles and the animations. Each `Animation` is a list of frames with durations and a mode
    (`MODE_ONCE`, `MODE_LOOP` or `MODE_PINGPONG`). `AdvanceTime` is the core: it tracks the current frame and the
    time spent in it, and updates the sprite's rectangle and offset. For the two repeating modes it first drops the
    whole repetitions from the elapsed time, so a very large time cannot stall it, and an animation whose frames
    have no duration stays on its current frame. A new sprite starts on the first frame of the first animation.
  - An `AnimatedSprite` can be without a current image: when it was default-constructed (no animation group), when
    the group has no animations, or when the current animation has no frames. Its rectangle is then empty and
    `Draw` draws nothing, unlike a plain `Sprite`, for which an empty rectangle means the whole sheet. That is why
    `Draw` is virtual. The animation functions do nothing in these cases, and they assert in debug builds when
    there is no animation group.
  - `SpriteFactory` is a stub. Its load functions return 0.

### Dependencies

SDL3 and SDL3_image are linked through CMake targets. If the targets `SDL3::SDL3` or `SDL3_image::SDL3_image`
already exist, because a parent project added the library itself, Sdlx uses them. Otherwise they are fetched with
`FetchContent`, but only when they are not already installed (`FIND_PACKAGE_ARGS`). `SDL3::SDL3` is a public
dependency, because the public headers include `<SDL3/SDL.h>`. `SDL3_image::SDL3_image` is private. Notes on the
fetch:

- SDL3_image is fetched without its git submodules, with `SDLIMAGE_VENDORED` off and the AVIF, JXL, TIFF and WebP
  formats disabled. Cloning the submodules fails on Windows when the build path is long. PNG, JPEG and the other
  built-in formats still work.
- The option `Sdlx_INSTALL` controls whether Sdlx generates install and export rules. It is on by default only when
  Sdlx is the top-level project. When it is on, `SDL_INSTALL` and `SDLIMAGE_INSTALL` are forced on, so a fetched
  SDL can be part of Sdlx's export set, and installing Sdlx also installs a fetched SDL3 and SDL3_image. If a
  parent project provides the SDL targets and turns `Sdlx_INSTALL` on, its SDL must have install rules too.
- On Linux, SDL3's own configure step fails unless the X11 or Wayland development packages are installed. For a
  build that needs no windows, such as the tests, pass
  `-DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF -DSDL_WAYLAND=OFF`. CI does this.

## File structure

```
include/Sdlx/        Public headers (include as <Sdlx/Sdlx.h>)
src/                 Implementation files
test/                GoogleTest tests; each .cpp becomes its own executable
cmake/               Package config template (SdlxConfig.cmake.in)
Doxyfile.in          Doxygen configuration, filled in by CMake
.clang-format        Formatting style (Microsoft)
.clang-tidy          Lint checks
.github/workflows/   CI and CD
```

The source files include the headers by bare name (`#include "Sdlx.h"`). This works through the library's private
include path, `include/Sdlx`.

## Building

```sh
cmake -S . -B build
cmake --build build --config Release
```

CMake 3.24 or later is required. The first configure downloads SDL3 and SDL3_image if they aren't installed. On
Windows, keep the build directory path short, since a long path can break the SDL3_image clone. On Linux, see the
note about X11 and Wayland under [Dependencies](#dependencies).

The library is static unless you pass `-DBUILD_SHARED_LIBS=ON`. The API has no export annotations, so on Windows the
DLL exports every symbol (`WINDOWS_EXPORT_ALL_SYMBOLS`). Debug builds use the postfix `d`.

The public headers are listed in `PUBLIC_HEADERS` in `CMakeLists.txt`. That list is the target's header file set,
which is what `cmake --install` copies, so a new public header must be added to it.

## Testing

Tests use GoogleTest, which CMake fetches. They are off unless you enable them, and can be enabled only when Sdlx
is the top-level project:

```sh
cmake -S . -B build -DSdlx_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Every `*.cpp` file in `test/` is built as its own executable named `Sdlx_<filename>`, and its tests are discovered
automatically. The tests run headless (in-memory surfaces and the SDL event queue, no window):

- `AnimatedSpriteTest.cpp` covers `Sprite` and `AnimatedSprite`: playback modes, frame boundaries, `SetTime` and
  `SetFrame`.
- `SdlxTest.cpp` covers the helpers, image loading with color keys, `Sprite::Draw` pixel output, and `EventLoop`.

Run a single test, by name or by GoogleTest filter:

```sh
ctest --test-dir build -C Release -R Suite.TestName
build/test/Release/Sdlx_<filename> --gtest_filter=Suite.TestName    # multi-config generators
build/test/Sdlx_<filename> --gtest_filter=Suite.TestName            # single-config generators
```

## Documentation

Doxygen and Graphviz (`dot`) must be installed. Setting the output directory turns on the `docs` target:

```sh
cmake -S . -B build -DSdlx_DOXYGEN_OUTPUT_DIRECTORY=docs
cmake --build build --target docs
```

The HTML is written to `docs/html`. Doxygen reads `README.md` (as the main page), `DEVELOPMENT.md`, `src/` and
`include/Sdlx/`. Keep the build free of Doxygen warnings.

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

Formatting uses `clang-format` with the Microsoft style (4-space indent, no tabs, 120 columns, braces on their own
line), which is set in `.clang-format` at the repository root. Run `clang-format -i` on the files you change before
opening a pull request. The lint checks are set in `.clang-tidy`: the compiler's diagnostics and the Clang static
analyzer, with every finding treated as an error. The test steps fail if no tests are found. Coverage upload needs
a `CODECOV_TOKEN` repository secret.

### Releases

`cd.yml` runs on pushes to `master` that touch `CMakeLists.txt`. It builds and tests, reads the version from the
`project()` call in `CMakeLists.txt`, and creates the tag `v<version>` if it doesn't exist yet. It then merges
`master` into `develop`, unless `develop` already contains it. The merge doesn't depend on the tag, so if it fails
(a conflict, for example), fix the cause and re-run the job. To release, bump `VERSION` in `CMakeLists.txt` and
merge to `master`.
