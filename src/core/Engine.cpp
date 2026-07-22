#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>

#include "drz/core/Engine.hpp"
#include "drz/core/App.hpp"
#include "drz/util/Logger.hpp"

#include <stdexcept>
#include <string>
#include <memory>

extern drz::App* create_app(); // user provides impl

namespace drz
{
Engine::Engine(int initial_width, int initial_height, std::string_view title)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
    }

    // Let the renderer backend configure SDL attributes before window creation.
    Renderer::configure_window_attributes();

    std::string title_str(title);
    m_window = SDL_CreateWindow(title_str.c_str(), initial_width, initial_height, Renderer::window_flags());
    if (!m_window)
    {
        throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
    }
    SDL_GetWindowSizeInPixels(m_window, &m_width, &m_height); // just ask SDL for size after we init it

    m_keys_state = SDL_GetKeyboardState(nullptr); // first keyboard state

    // ### Init Renderer ###

    m_renderer = std::make_unique<Renderer>(m_window);

    // ### Init App stuff ###

    m_app.reset(create_app());
    if (!m_app)
    {
        throw std::runtime_error("create_app() returned nullptr");
    }

    m_app->m_engine = this; // set app engine BEFORE WE INIT APP
    m_app->init();

    m_last_time = SDL_GetTicks();
}

Engine::~Engine()
{
    m_app.reset();               // Free GPU resources user app allocated
    m_renderer.reset();          // Free rendering context from renderer
    SDL_DestroyWindow(m_window); // Kill window
    SDL_Quit();                  // Finally, kill SDL
}

void Engine::tick()
{
    m_renderer->reset_frame_stats();

    uint64_t now_time = SDL_GetTicks();            // returns current time in ms
    float dt = (now_time - m_last_time) / 1000.0f; // get delta, then convert to seconds
    double elapsed = now_time / 1000.0;
    m_last_time = now_time;

    m_app->update(dt, elapsed); // update with app's update func
    m_app->render(*m_renderer); // render with app's render func, pass in renderer for app to submit draw calls
    m_renderer->flush();        // flush draw commands to make draw calls

    uint64_t now_ms = SDL_GetTicks();
    if (now_ms - m_last_stats_log_ms >= 250)
    {
        const auto& s = m_renderer->last_frame_stats();
        DRZ_LOGF("[stats] submits={} draws={} prog_binds={} ssbo_binds={} cpu={:.2f}ms",
                 s.submits,
                 s.draw_calls,
                 s.shader_binds,
                 s.ssbo_binds,
                 s.cpu_flush_ms);
        DRZ_FLUSH_LOG();
        m_last_stats_log_ms = now_ms;
    }

    SDL_GL_SwapWindow(m_window); // Swap render buffers
}

SDL_AppResult Engine::handle_event(const SDL_Event& event)
{
    // Handle all engine-level events
    switch (event.type)
    {
    case SDL_EVENT_WINDOW_RESIZED:
    {
        int w = 0;
        int h = 0;
        SDL_GetWindowSizeInPixels(m_window, &w, &h);
        m_renderer->set_viewport(w, h); // update viewport to match new window size
        m_width = w;
        m_height = h;
        break;
    }
    default:
        break;
    }

    // Forward to user App if they want to handle more stuff
    m_app->handle_event(event);

    // Handle SDL app quit event (do this at the end so that events reach Engine and App)
    if (event.type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    // Also do custom event quit and report to SDL
    if (m_quit_requested)
    {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

void Engine::set_title(std::string_view title)
{
    SDL_SetWindowTitle(m_window, std::string(title).c_str());
}

std::pair<int, int> Engine::framebuffer_size() const
{
    return {m_width, m_height};
}

void Engine::request_quit()
{
    m_quit_requested = true;
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

static constexpr const char* sdl_app_result_to_string(SDL_AppResult result) noexcept
{
    switch (result)
    {
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
extern "C"
{

    SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
    {
        try
        {
            // make engine and pass to global SDL appstate
            *appstate = new drz::Engine(1280, 720, "Derezzed");
            return SDL_APP_CONTINUE;
        }
        catch (const std::exception& e)
        {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", e.what());
            return SDL_APP_FAILURE;
        }
    }

    SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
    {
        // Get engine and return back whatever it returns after handling events
        return static_cast<drz::Engine*>(appstate)->handle_event(*event);
    }

    SDL_AppResult SDL_AppIterate(void* appstate)
    {
        static_cast<drz::Engine*>(appstate)->tick(); // tick once
        return SDL_APP_CONTINUE;
    }

    void SDL_AppQuit(void* appstate, SDL_AppResult result)
    {
        SDL_Log("Quitting SDL App with result: %s", drz::sdl_app_result_to_string(result));
        delete static_cast<drz::Engine*>(appstate); // cleanup engine
    }
}
