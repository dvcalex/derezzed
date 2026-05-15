#pragma once

#include <SDL3/SDL_video.h>
#include <vector>
#include <cstdint>

namespace drz {

struct FrameStats {
    uint32_t submits = 0;    // total draw commands submitted this frame
    uint32_t draw_calls = 0; // actual glDraws issues
    uint32_t shader_binds = 0;
    uint32_t vertex_layout_binds = 0;
    float cpu_flush_ms = 0.0f; // elapsed time inside flush() this frame
};

struct DrawCommand {
    uint32_t shader;
    uint32_t vertex_layout;
    uint32_t index_count;
    uint32_t vertex_count;
};

class Renderer {
public:
    // For implementing graphics api-specific setup called by Engine before window creation.
    // ex. Lets the renderer configure SDL/GL attributes in backend.
    static void configure_window_attributes();
    static SDL_WindowFlags window_flags();

    Renderer(SDL_Window* window);
    ~Renderer();

    // Delete copy
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void submit(DrawCommand cmd);             // Add a command to the draw commands buffer
    void flush();                             // Flush current draw commands buffer and make draw calls
    void set_viewport(int width, int height); // called by Engine on window resize
    void clear(float r, float g, float b, float a);
    void reset_frame_stats();
    const FrameStats last_frame_stats() const {
        return frame_stats;
    }

private:
    SDL_GLContext context = nullptr;
    std::vector<DrawCommand> draw_commands;
    FrameStats frame_stats;
};
} // namespace drz
