#pragma once

#include "drz/gfx/Renderer.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_scancode.h>

#include <string_view>
#include <memory>
#include <array>

namespace drz
{

class App;

class Engine
{
public:
    Engine(int width, int height, std::string_view title);
    ~Engine();

    // Handles ticking the engine forward one time
    void tick();

    SDL_AppResult handle_event(const SDL_Event& event);

    void set_title(std::string_view title);

    std::pair<int, int> framebuffer_size() const;

    void request_quit();

    // ### input API ###

    // Check the most recent key down state
    bool key_down(SDL_Scancode scancode) const;
    // Check if a key was pressed this frame
    bool key_pressed(SDL_Scancode scancode) const;
    // Check if a key was released this frame
    bool key_released(SDL_Scancode scancode) const;
    // Sum of motion since last frame
    std::pair<float, float> mouse_delta() const;
    std::pair<float, float> mouse_position() const;
    void set_mouse_grabbed(bool mouse_grabbed);

private:
    SDL_Window* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
    uint64_t m_last_stats_log_ms = 0;
    uint64_t m_last_time_ms = 0;
    bool m_quit_requested = false;

    // input handling
    std::array<bool, SDL_SCANCODE_COUNT> m_keys_down {false};
    std::array<bool, SDL_SCANCODE_COUNT> m_keys_pressed {false};
    std::array<bool, SDL_SCANCODE_COUNT> m_keys_released {false};
    std::pair<float, float> m_mouse_pos {0.f, 0.f};
    std::pair<float, float> m_mouse_delta {0.f, 0.f};

    std::unique_ptr<App> m_app;
    std::unique_ptr<Renderer> m_renderer;
};

} // namespace drz
