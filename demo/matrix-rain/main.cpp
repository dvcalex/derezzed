#include <drz/core/engine.hpp>
#include <drz/core/app.hpp>
#include <drz/gfx/renderer.hpp>
#include <drz/gfx/shader.hpp>
#include <drz/gfx/mesh_pool.hpp>

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
        pool.emplace(64, 64);

        constexpr float positions[] = {
            -1.0f,
            1.0f,
            0.0f,
            -1.0f,
            -1.0f,
            0.0f,
            1.0f,
            -1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
        };
        constexpr float uvs[] = {
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
        };
        constexpr uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        quad = pool->upload({
            .positions = positions,
            .uvs = uvs,
            .indices = indices,
        });

        shader.emplace(res_path("shaders/matrix.vert"), res_path("shaders/matrix.frag"));
    }

    void handle_event(const SDL_Event&) override {}

    void update(float, double elapsed) override {
        time = static_cast<float>(elapsed);
    }

    void render(drz::Renderer& renderer) override {
        if (!registered) {
            pipeline = renderer.register_state({.shader = shader->handle()});
            renderer.use_mesh_pool(*pool);
            registered = true;
        }

        renderer.clear(0.0f, 0.0f, 0.0f, 1.0f);

        auto [w, h] = engine->framebuffer_size();
        shader->set_uniform("u_time", time);
        shader->set_uniform("u_resolution", glm::vec2(static_cast<float>(w), static_cast<float>(h)));

        renderer.submit(drz::gen_sort_key(0, pipeline), {.state_id = pipeline, .mesh = quad});
    }

private:
    std::optional<drz::MeshPool> pool;
    drz::MeshHandle quad;
    std::optional<drz::Shader> shader;
    float time = 0.0f;

    bool registered = false;
    drz::PipelineStateId pipeline = 0;
};

drz::App* create_app() {
    return new MatrixRain();
}
