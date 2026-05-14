#include <drz/core/engine.hpp>
#include <drz/core/app.hpp>
#include <drz/gfx/renderer.hpp>
#include <drz/gfx/shader.hpp>
#include <drz/gfx/mesh.hpp>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <optional>
#include <string>

static std::string res_path(const std::string& relative) {
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class MatrixRain : public drz::App {
public:
    void init() override {
        quad.emplace();
        shader.emplace(res_path("shaders/matrix.vert"), res_path("shaders/matrix.frag"));
    }

    void handle_event(const SDL_Event& event) override {}

    void update(float dt, double elapsed) override {
        time = static_cast<float>(elapsed);
    }

    void render(drz::Renderer& renderer) override {
        renderer.clear(0.0f, 0.0f, 0.0f, 1.0f);

        auto [w, h] = engine->framebuffer_size();
        shader->set_uniform("u_time", time);
        shader->set_uniform("u_resolution", glm::vec2(static_cast<float>(w), static_cast<float>(h)));

        drz::DrawCommand cmd {
            .shader = shader->handle(),
            .vertex_layout = quad->vertex_layout_handle(),
            .index_count = quad->index_count(),
            .vertex_count = quad->vertex_count(),
        };
        renderer.submit(cmd);
    }

private:
    std::optional<drz::FullscreenQuadMesh> quad;
    std::optional<drz::Shader> shader;
    float time = 0.0f;
};

drz::App* create_app() {
    return new MatrixRain();
}
