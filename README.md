# derezzed
WIP toy graphics engine

## Features
- CMake build system
    - Engine builds as static library
    - each app has its own CMakeLists.txt that links against the engine
- SDL3 for windowing, input, media, etc.
- Swappable (at compile time) rendering backend
    - OpenGL
- DOD focus (WIP)
- Demos

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

Live in demo folder, build and run like above. 

## Writing an App

Implement `drz::App` and provide a `create_app()` factory:

```cpp
#include <derezzed/engine.hpp>
#include <derezzed/app.hpp>

#include <iostream>

class MyApp : public drz::App {
public:
    void init() override {
        std::cout << "Hello World!\n";
    }

    void handle_event(const SDL_Event& event) override {}

    void update(float dt, double elapsed) override {}

    void render() override {}
};

drz::App* create_app() {
    return new MyApp();
}

```

Link your executable against `Derezzed SDL3::SDL3 OpenGL::GL` (Look at demo CMakeLists.txt for reference).
