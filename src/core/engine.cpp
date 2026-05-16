#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>

#include <drz/core/engine.hpp>
#include <drz/core/app.hpp>
#include <drz/util/logger.hpp>

#include <stdexcept>
#include <string>
#include <memory>

extern drz::App* create_app(); // user provides impl

namespace drz {

Engine::Engine(int initial_width, int initial_height, std::string_view title) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
    }

    // Let the renderer backend configure SDL attributes before window creation.
    Renderer::configure_window_attributes();

    std::string title_str(title);
    window = SDL_CreateWindow(title_str.c_str(), initial_width, initial_height, Renderer::window_flags());
    if (!window) {
        throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
    }
    SDL_GetWindowSizeInPixels(window, &width, &height); // just ask SDL for size after we init it

    keys_state = SDL_GetKeyboardState(nullptr); // first keyboard state

    // ### Init Renderer ###

    renderer = std::make_unique<Renderer>(window);

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
    app.reset();               // Free GPU resources user app allocated
    renderer.reset();          // Free rendering context from renderer
    SDL_DestroyWindow(window); // Kill window
    SDL_Quit();                // Finally, kill SDL
}

void Engine::tick() {
    renderer->reset_frame_stats();

    uint64_t now_time = SDL_GetTicks();          // returns current time in ms
    float dt = (now_time - last_time) / 1000.0f; // get delta, then convert to seconds
    double elapsed = now_time / 1000.0;
    last_time = now_time;

    app->update(dt, elapsed); // update with app's update func
    app->render(*renderer);   // render with app's render func, pass in renderer for app to submit draw calls
    renderer->flush();        // flush draw commands to make draw calls

    uint64_t now_ms = SDL_GetTicks();
    if (now_ms - last_stats_log_ms >= 250) {
        const auto& s = renderer->last_frame_stats();
        DRZ_LOGF("[stats] submits={} draws={} prog_binds={} ssbo_binds={} cpu={:.2f}ms",
                 s.submits,
                 s.draw_calls,
                 s.shader_binds,
                 s.ssbo_binds,
                 s.cpu_flush_ms);
        DRZ_FLUSH_LOG();
        last_stats_log_ms = now_ms;
    }

    SDL_GL_SwapWindow(window); // Swap render buffers
}

SDL_AppResult Engine::handle_event(const SDL_Event& event) {
    // Handle all engine-level events
    switch (event.type) {
    case SDL_EVENT_WINDOW_RESIZED: {
        int w = 0;
        int h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        renderer->set_viewport(w, h); // update viewport to match new window size
        width = w;
        height = h;
        break;
    }
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
    SDL_SetWindowTitle(window, std::string(title).c_str());
}

std::pair<int, int> Engine::framebuffer_size() const {
    return {width, height};
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
