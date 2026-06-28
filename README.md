# derezzed
WIP toy graphics engine

## Features
- CMake build system
    - Engine builds as a static library
    - each app has its own `CMakeLists.txt` that links against the engine
- SDL3 for windowing, input, media, etc.
- Engine/header api-agnostic interfaces
  - OpenGL backend (can add more with some work)
- DOD focus (WIP)
- Demos

## Dependencies
- Everything is vendored under `vendor/`.
- SDL3 is built from source and referenced as a git submodule.

### Required:

- A C++20 compiler
- [CMake](https://cmake.org/) 3.21+
- `git` for submodule

## Build

Clone with submodules or init after:

```bash
git clone --recurse-submodules <repo-url>
# or, if already cloned:
git submodule update --init --depth 1
```

Then configure and build with the cmake preset:

```bash
# Configure (first time only)
cmake --preset default

# Build everything
cmake --build --preset default

# Or optimized build
cmake --preset default -DCMAKE_BUILD_TYPE=Release && cmake --build --preset default
```

> SDL3 might take a while, should only concern the first build since it's incremental

Each demo executable is placed in `build/bin/`, with its `res/` copied next to it.

## Run

```bash
./build/bin/hello
```

## Demos

Live in the `demo/` folder, build and run like above. Use these for testing and for reference on how to build apps.

## Writing an App

Implement `drz::App` and provide a `create_app()` factory:

*psuedo code*
```cpp
#include <drz/core/engine.hpp>
#include <drz/core/app.hpp>
#include <drz/gfx/renderer.hpp>

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

Add a `CMakeLists.txt` next to your sources that links the engine (look at a demo's `CMakeLists.txt` for reference).

Then add it to the build by dropping `add_subdirectory(path/to/myapp)` into the root `CMakeLists.txt`.
