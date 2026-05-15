#include <drz/gfx/renderer.hpp>
#include <glad/gl.h>
#include <SDL3/SDL_video.h>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace drz {

// api-specific window configuration
void Renderer::configure_window_attributes() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
}

SDL_WindowFlags Renderer::window_flags() {
    return SDL_WINDOW_OPENGL;
}

Renderer::Renderer(SDL_Window* window) {
    // Create OpenGL context
    context = SDL_GL_CreateContext(window);
    if (!context) {
        throw std::runtime_error(std::string("SDL_GL_CreateContext: ") + SDL_GetError());
    }
    // Load GLAD
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        throw std::runtime_error("gladLoadGL failed");
    }

    // Set initial viewport in pixel size
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    glViewport(0, 0, w, h); // set viewport to window size
}

Renderer::~Renderer() {
    SDL_GL_DestroyContext(context);
}

void Renderer::set_viewport(int width, int height) {
    glViewport(0, 0, width, height);
}

void Renderer::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::submit(DrawCommand cmd) {
    draw_commands.push_back(cmd);
}

void Renderer::flush() {
    // Sort commands by shader and vertex layout to minimize state changes (binds)
    std::sort(draw_commands.begin(), draw_commands.end(), [](const DrawCommand& a, const DrawCommand& b) {
        if (a.shader != b.shader) {
            return a.shader < b.shader;
        }
        return a.vertex_layout < b.vertex_layout;
    });

    // Track current state
    uint32_t current_shader = 0;
    uint32_t current_vertex_layout = 0;

    // Iterate over handles and make draw calls
    for (const auto& cmd : draw_commands) {
        // Check if shader needs to be rebinded
        if (cmd.shader != current_shader) {
            glUseProgram(cmd.shader);
            current_shader = cmd.shader;
        }
        // Check if vao needs to be rebinded
        if (cmd.vertex_layout != current_vertex_layout) {
            glBindVertexArray(cmd.vertex_layout);
            current_vertex_layout = cmd.vertex_layout;
        }
        if (cmd.index_count > 0) {
            glDrawElements(GL_TRIANGLES, cmd.index_count, GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, cmd.vertex_count);
        }
    }
    draw_commands.clear(); // clear buffer after flushing
}

} // namespace drz
