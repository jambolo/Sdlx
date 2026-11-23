# Sdlx

[![CI](https://github.com/jambolo/Sdlx/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/jambolo/Sdlx/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/jambolo/Sdlx/branch/develop/graph/badge.svg)](https://codecov.io/gh/jambolo/Sdlx)

Sdlx is a C++ wrapper library around [SDL3](https://www.libsdl.org/). Its main purposes are to provide
[RAII](https://en.cppreference.com/w/cpp/language/raii) ownership of SDL resources and higher-level abstractions
over SDL's C API.

Sdlx is a library only. It has no executable, and it needs SDL3 and SDL3_image, which CMake fetches for you if they
aren't already installed.

## Status

Sdlx is early in its SDL3 port. What exists today:

- Image loading, with optional color keying (`LoadSurface`, `LoadColorKeyedSurface`), via SDL3_image.
- A simple event loop with idle processing (`EventLoop`).
- 2D sprites and time-based sprite animation (`Sprite`, `AnimatedSprite`) that blit from a sheet surface.

The RAII wrappers are not written yet. For now, loaded surfaces are plain `SDL_Surface *` that you free with
`SDL_DestroySurface`.

## Requirements

- CMake 3.24 or later
- A C++17 compiler
- Git, to let CMake fetch SDL3 and SDL3_image when they are not installed
- On Linux, the X11 or Wayland development packages, which SDL3 needs when it is built from source (see
  [SDL's build dependencies](https://wiki.libsdl.org/SDL3/README-linux#build-dependencies)). For a build that needs
  no windows, pass `-DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF -DSDL_WAYLAND=OFF` to CMake instead.

## Using Sdlx in your project

Sdlx exports the target `Sdlx::Sdlx`. Linking to it also gives you SDL3 (`SDL3::SDL3`) and the `include/` directory,
so you can write `#include <Sdlx/Sdlx.h>`.

### With FetchContent (recommended)

```cmake
include(FetchContent)
FetchContent_Declare(
    Sdlx
    GIT_REPOSITORY https://github.com/jambolo/Sdlx.git
    GIT_TAG        master   # or a release tag, e.g. v0.1.0
)
FetchContent_MakeAvailable(Sdlx)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE Sdlx::Sdlx)
```

### As a subdirectory

```cmake
add_subdirectory(external/Sdlx)
target_link_libraries(myapp PRIVATE Sdlx::Sdlx)
```

With either method, if your project already defines the targets `SDL3::SDL3` or `SDL3_image::SDL3_image` before it
adds Sdlx, Sdlx uses them and does not fetch its own. Sdlx adds no install rules to your project unless you set
`Sdlx_INSTALL` to `ON`.

### As an installed package

Build and install Sdlx, then find it:

```sh
cmake -S . -B build
cmake --build build --config Release
cmake --install build --config Release --prefix /path/to/prefix
```

```cmake
find_package(Sdlx REQUIRED)    # add /path/to/prefix to CMAKE_PREFIX_PATH
target_link_libraries(myapp PRIVATE Sdlx::Sdlx)
```

Sdlx's package config looks for SDL3 and SDL3_image with `find_dependency`, so both must be findable by your project
too.

### Example

```cpp
#include <Sdlx/Sdlx.h>
#include <Sdlx/Sprite.h>

SDL_Surface *sheet = Sdlx::LoadColorKeyedSurface("sprites.png", SDL_Color{255, 0, 255, 255});
Sdlx::Sprite sprite(sheet, SDL_Rect{0, 0, 32, 32}, 16, 16, 100.0f, 100.0f);
// ...
sprite.Draw(screenSurface);
```

## Documentation

API documentation is generated with Doxygen and published at <https://jambolo.github.io/Sdlx>.

## Development

See [DEVELOPMENT.md](DEVELOPMENT.md) for the architecture, file layout, and how to build and test.

## License

MIT. See [LICENSE](LICENSE).
