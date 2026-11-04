# derezzed

WIP toy graphics engine

## Features

- CMake build system
  - Engine builds as a static library
  - Each demo links against the engine through the root `CMakeLists.txt`
- SDL3 for windowing, input, media, etc.
- Engine/header api-agnostic interfaces
  - OpenGL backend (can add more with some work)
- DOD focus (WIP)
- Demos

## Dependencies

- Everything is vendored under `vendor/`.

### Required:

- A C++23 compiler
- [CMake](https://cmake.org/) 3.26+

## Build

Clone:

```bash
git clone <repo-url>
```

Configure and build debug:

```bash
cmake --preset debug
cmake --build --preset debug
```

Or configure and build optimized release:

```bash
cmake --preset release
cmake --build --preset release
```

Release disables asserts and debug logging, including the renderer stats output.

To keep logging enabled in Release:

```bash
cmake --preset release -DDRZ_ENABLE_LOGGING=ON
cmake --build --preset release
```

This keeps Release optimization enabled. The option is saved in the build directory; configure with `-DDRZ_ENABLE_LOGGING=OFF` to turn it off again. Debug logging is always enabled.

Both build commands build all demos. To build just one, add its target name:

```bash
cmake --build --preset release --target ManyCubes
```

> SDL3 may take a while to build the first time for each preset. Subsequent builds are incremental.

Demo executables go in `build/debug/bin/<Demo>/` or `build/release/bin/<Demo>/`. Their assets should be copied beside the binaries, so relative paths work.

## Run

```bash
# Debug
./build/debug/bin/Hello/Hello

# Release
./build/release/bin/Hello/Hello
```

## Demos

Live in the `demo/` folder, build and run like above. Use these for testing and for reference on how to build apps.

## Writing an App

Implement `drz::App` and provide a `create_app()` factory:

_psuedo code_

```cpp
#include <drz/core/Engine.hpp>
#include <drz/core/App.hpp>
#include <drz/gfx/Renderer.hpp>

#include <iostream>

class MyApp : public drz::App {
public:
    void init() override {
        std::cout << "Hello World!\n";
    }

    void handle_event(const SDL_Event& event) override {}

    void update(float dt, double elapsed) override {}

    void render(drz::Renderer& renderer) override {}
};

drz::App* create_app() {
    return new MyApp();
}
```

Put your app in `demo/MyApp/main.cpp`, with any assets in `demo/MyApp/res/`. Add `MyApp` to the demos list in the root `CMakeLists.txt`. `drz_add_demo` links the engine and copies the resources beside the binary file.
