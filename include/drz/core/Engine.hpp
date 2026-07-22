#pragma once

#include "drz/gfx/Renderer.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <string_view>
#include <memory>

namespace drz
{

class App;

class Engine
{
public:
    Engine(int width, int height, std::string_view title);
    ~Engine();

    void tick();
    SDL_AppResult handle_event(const SDL_Event& event);
    void set_title(std::string_view);
    std::pair<int, int> framebuffer_size() const;
    void request_quit();

private:
    SDL_Window* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
    uint64_t m_last_stats_log_ms = 0;
    const bool* m_keys_state = nullptr; // SDL keyboard state array. Use SDL_SCANCODE_<key> to index array.
    uint64_t m_last_time = 0;
    bool m_quit_requested = false;

    std::unique_ptr<App> m_app;
    std::unique_ptr<Renderer> m_renderer;
};

} // namespace drz
