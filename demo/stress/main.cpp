#include <drz/core/engine.hpp>
#include <drz/core/app.hpp>
#include <drz/gfx/renderer.hpp>
#include <drz/gfx/shader.hpp>
#include <drz/gfx/mesh.hpp>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <optional>
#include <random>
#include <string>
#include <vector>

static std::string res_path(const std::string& relative) {
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

// N small quads scattered in NDC, each with its own offset/scale/tint, using one of 3 shaders. Submits + flushes per quad.
class StressApp : public drz::App {
private:
    static constexpr size_t quad_count = 5000;

    std::optional<drz::QuadMesh> quad;
    std::vector<drz::Shader> shaders;

    std::vector<glm::vec2> offsets;
    std::vector<float> scales;
    std::vector<glm::vec4> tints;
    std::vector<uint32_t> shader_idx;

public:
    void init() override {
        quad.emplace();

        shaders.reserve(3);
        shaders.emplace_back(res_path("shaders/quad.vert"), res_path("shaders/solid.frag"));
        shaders.emplace_back(res_path("shaders/quad.vert"), res_path("shaders/checker.frag"));
        shaders.emplace_back(res_path("shaders/quad.vert"), res_path("shaders/pulse.frag"));

        // Deterministic scene rng so numbers compare across runs.
        std::mt19937 rng(0xC0FFEEu);
        std::uniform_real_distribution<float> pos(-0.95f, 0.95f);
        std::uniform_real_distribution<float> scl(0.01f, 0.04f);
        std::uniform_real_distribution<float> tint(0.2f, 1.0f);
        std::uniform_int_distribution<uint32_t> si(0, static_cast<uint32_t>(shaders.size()) - 1);

        offsets.reserve(quad_count);
        scales.reserve(quad_count);
        tints.reserve(quad_count);
        shader_idx.reserve(quad_count);
        for (size_t i = 0; i < quad_count; ++i) {
            offsets.push_back({pos(rng), pos(rng)});
            scales.push_back(scl(rng));
            tints.push_back({tint(rng), tint(rng), tint(rng), 1.0f});
            shader_idx.push_back(si(rng));
        }
    }

    void handle_event(const SDL_Event& event) override {}

    void update(float, double) override {}

    void render(drz::Renderer& renderer) override {
        renderer.clear(0.05f, 0.05f, 0.08f, 1.0f);
        for (size_t i = 0; i < quad_count; ++i) {
            auto& sh = shaders[shader_idx[i]];
            sh.set_uniform("u_offset", offsets[i]);
            sh.set_uniform("u_scale", scales[i]);
            sh.set_uniform("u_tint", tints[i]);
            renderer.submit({
                .shader = sh.handle(),
                .vertex_layout = quad->vertex_layout_handle(),
                .index_count = quad->index_count(),
                .vertex_count = quad->vertex_count(),
            });
            renderer.flush();
        }
    }
};

drz::App* create_app() {
    return new StressApp();
}
