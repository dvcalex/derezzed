#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <exception>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>

#include <glad/gl.h>

#include <derezzed/engine.hpp>
#include <derezzed/app.hpp>

#include <stdexcept>
#include <string>
#include <memory>

extern drz::App* create_app(); // user provides impl

namespace drz {

struct Engine::Impl {
    SDL_Window* window = nullptr;
    int width, height;
    SDL_GLContext context = nullptr;
    // SDL keyboard state array. Use SDL_SCANCODE_<key> to index array.
    const bool* keys_state = nullptr;
};

Engine::Engine(int width, int height, std::string_view title) : impl(new Impl) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
    }

    /// ### Init SDL and OpenGL stuff ###

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    std::string title_str(title);
    impl->window = SDL_CreateWindow(title_str.c_str(), width, height, SDL_WINDOW_OPENGL);
    if (!impl->window) {
        throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
    }
    SDL_GetWindowSizeInPixels(impl->window, &impl->width, &impl->height); // just ask SDL for size after we init it

    impl->keys_state = SDL_GetKeyboardState(nullptr); // first keyboard state

    // Create OpenGL context
    impl->context = SDL_GL_CreateContext(impl->window);
    if (!impl->context) {
        throw std::runtime_error(std::string("SDL_GL_CreateContext: ") + SDL_GetError());
    }

    // Load GLAD
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        throw std::runtime_error("gladLoadGL failed");
    }

    // ### More OpenGL init ###

    glViewport(0, 0, impl->width, impl->height); // set viewport to window size

    // ### Init App stuff ###

    app.reset(create_app());
    if (!app) {
        throw std::runtime_error("create_app() returned nullptr");
    }

    app->engine = this; // set app engine BEFORE WE INIT APP
    app->init();

    last_time = SDL_GetTicks();
}

Engine::~Engine() {
    app.reset();                          // Cleanup user code
    SDL_GL_DestroyContext(impl->context); // Cleanup gl stuff
    SDL_DestroyWindow(impl->window);      // Kill window
    SDL_Quit();                           // Finally, kill SDL
    delete impl;
}

void Engine::tick() {
    uint64_t now_time = SDL_GetTicks();          // returns current time in ms
    float dt = (now_time - last_time) / 1000.0f; // get delta, then convert to seconds
    double elapsed = now_time / 1000.0;
    last_time = now_time;

    app->update(dt, elapsed);
    app->render();
    SDL_GL_SwapWindow(impl->window); // Swap render buffers
}

SDL_AppResult Engine::handle_event(const SDL_Event& event) {
    // Handle all engine-level events
    switch (event.type) {
    case SDL_EVENT_WINDOW_RESIZED:
        int w, h;
        SDL_GetWindowSizeInPixels(impl->window, &w, &h);
        glViewport(0, 0, w, h);
        impl->width = w;
        impl->height = h;
        break;

    default:
        break;
    }

    // Forward to user App if they want to handle more stuff
    app->handle_event(event);

    // Handle SDL app quit event (do this at the end so that events reach Engine and App)
    if (event.type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    // Also do custom event quit and report to SDL
    if (quit_requested) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

void Engine::set_title(std::string_view title) {
    SDL_SetWindowTitle(impl->window, std::string(title).c_str());
}

std::pair<int, int> Engine::framebuffer_size() const {
    return {impl->width, impl->height};
}

void Engine::request_quit() {
    quit_requested = true;
}

/*
static constexpr const char* sdl_event_type_to_string(Uint32 type) noexcept {
    switch (type) {
    case SDL_EVENT_QUIT:
        return "SDL_EVENT_QUIT";
    case SDL_EVENT_WINDOW_SHOWN:
        return "SDL_EVENT_WINDOW_SHOWN";
    case SDL_EVENT_WINDOW_HIDDEN:
        return "SDL_EVENT_WINDOW_HIDDEN";
    case SDL_EVENT_KEY_DOWN:
        return "SDL_EVENT_KEY_DOWN";
    case SDL_EVENT_KEY_UP:
        return "SDL_EVENT_KEY_UP";
    case SDL_EVENT_MOUSE_MOTION:
        return "SDL_EVENT_MOUSE_MOTION";
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        return "SDL_EVENT_MOUSE_BUTTON_DOWN";
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return "SDL_EVENT_MOUSE_BUTTON_UP";
    case SDL_EVENT_MOUSE_WHEEL:
        return "SDL_EVENT_MOUSE_WHEEL";
    default:
        return "UNKNOWN_EVENT";
    }
}
*/

static constexpr const char* sdl_app_result_to_string(SDL_AppResult result) noexcept {
    switch (result) {
    case SDL_APP_CONTINUE:
        return "SDL_APP_CONTINUE";
    case SDL_APP_SUCCESS:
        return "SDL_APP_SUCCESS";
    case SDL_APP_FAILURE:
        return "SDL_APP_FAILURE";
    default:
        return "UNKNOWN_SDL_APP_RESULT";
    }
}

} // namespace drz

// SDL3 callbacks need to be in global scope.
// wrap with extern "C" so that C++ compiler doesnt mangle function names at link time and SDL can find them.
extern "C" {

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    try {
        // make engine and pass to global SDL appstate
        *appstate = new drz::Engine(1280, 720, "Derezzed");
        return SDL_APP_CONTINUE;
    } catch (const std::exception& e) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", e.what());
        return SDL_APP_FAILURE;
    }
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    // Get engine and return back whatever it returns after handling events
    return static_cast<drz::Engine*>(appstate)->handle_event(*event);
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    static_cast<drz::Engine*>(appstate)->tick(); // tick once
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    SDL_Log("Quitting SDL App with result: %s", drz::sdl_app_result_to_string(result));
    delete static_cast<drz::Engine*>(appstate); // cleanup engine
}
}
