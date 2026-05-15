# derezzed
WIP toy graphics engine

## Features
- CMake build system
    - Engine builds as static library
    - each app has its own CMakeLists.txt that links against the engine
- SDL3 for windowing, input, media, etc.
- Engine/header api-agnostic interfaces
  - OpenGL backend (can add more with some work)
- DOD focus (WIP)
- Demos

## Dependencies

Required:

- **CMake** 3.21+
- **A C++20 compiler**
- **OpenGL** driver and dev headers (should already be available)
- **SDL3** (CMake will fetch for you if it can't find it in system)

## Build

```bash
# Configure (debug or release)
cmake --preset debug

# Build
cmake --build --preset debug
```

Output goes to `build/debug/bin/` or `build/release/bin/`.

## Run

```bash
./build/debug/bin/hello
```

## Demos

Live in the `demo/` folder, build and run like above. Use these for testing and for reference on how to build apps.

## Writing an App

Implement `drz::App` and provide a `create_app()` factory:

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

Link your executable against `drz SDL3::SDL3 OpenGL::GL` (look at a demo `CMakeLists.txt` for reference).
