#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>

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
    // Pointer to engine implementation (PIMPL).
    // Let's us swap out engine impl at compile time.
    struct Impl;
    Impl* impl;
    // Owning ref to app
    std::unique_ptr<App> app = nullptr;
    uint64_t last_time = 0;
    bool quit_requested = false;
};

} // namespace drz
