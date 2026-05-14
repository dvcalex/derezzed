#pragma once

#include <SDL3/SDL_video.h>
#include <vector>
#include <cstdint>

namespace drz {

struct DrawCommand {
    uint32_t shader;
    uint32_t vertex_layout;
    uint32_t index_count;
    uint32_t vertex_count;
};

class Renderer {
public:
    Renderer(SDL_Window* window);
    ~Renderer();

    // Delete copy
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void submit(DrawCommand cmd);             // Add a command to the draw commands buffer
    void flush();                             // Flush current draw commands buffer and make draw calls
    void set_viewport(int width, int height); // called by Engine on window resize
    void clear(float r, float g, float b, float a);

private:
    SDL_GLContext context = nullptr;
    std::vector<DrawCommand> draw_commands;
};
} // namespace drz
