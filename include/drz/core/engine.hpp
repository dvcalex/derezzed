#pragma once

#include <drz/gfx/renderer.hpp>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <string_view>
#include <memory>

namespace drz {

class App;

class Engine {
public:
    Engine(int width, int height, std::string_view title);
    ~Engine();

    void tick();
    SDL_AppResult handle_event(const SDL_Event& event);
    void set_title(std::string_view);
    std::pair<int, int> framebuffer_size() const;
    void request_quit();

private:
    SDL_Window* window = nullptr;
    int width = 0;
    int height = 0;
    uint64_t last_stats_log_ms = 0;
    const bool* keys_state = nullptr; // SDL keyboard state array. Use SDL_SCANCODE_<key> to index array.
    uint64_t last_time = 0;
    bool quit_requested = false;

    std::unique_ptr<App> app;
    std::unique_ptr<Renderer> renderer;
};

} // namespace drz
