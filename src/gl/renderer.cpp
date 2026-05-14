#include <derezzed/renderer.hpp>
#include <glad/gl.h>
#include <SDL3/SDL_video.h>
#include <stdexcept>
#include <string>

namespace drz {

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
    int w, h;
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
    // Iterate over handles and make draw calls
    for (const auto& cmd : draw_commands) {
        glUseProgram(cmd.shader);
        glBindVertexArray(cmd.vertex_layout);
        if (cmd.index_count > 0) {
            glDrawElements(GL_TRIANGLES, cmd.index_count, GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, cmd.vertex_count);
        }
    }
    draw_commands.clear(); // clear buffer after flushing
}

} // namespace drz
