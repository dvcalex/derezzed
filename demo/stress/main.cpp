#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"
#include <SDL3/SDL_filesystem.h>
#include <cstddef>
#include <cstdint>
#include <drz/core/engine.hpp>
#include <drz/core/app.hpp>
#include <drz/gfx/renderer.hpp>
#include <drz/gfx/shader.hpp>
#include <drz/gfx/mesh_pool.hpp>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <array>
#include <optional>
#include <random>
#include <string>
#include <vector>

static std::string res_path(const std::string& relative) {
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class StressApp : public drz::App {
private:
    struct PerDraw {
        glm::vec2 offset;
        float scale;
        float _pad;
        glm::vec4 tint;
    };
    static_assert(sizeof(PerDraw) == 32);
    static_assert(offsetof(PerDraw, tint) == 16);

    static constexpr size_t quad_count = 5000;

    std::optional<drz::MeshPool> pool;
    drz::MeshHandle quad;
    std::vector<drz::Shader> shaders;

    bool registered = false;
    std::array<drz::PipelineStateId, 3> pipelines {};

    // SoA per-quad data
    std::vector<glm::vec2> offsets;
    std::vector<float> scales;
    std::vector<glm::vec4> tints;
    std::vector<uint32_t> shader_idx;

public:
    void init() override {
        pool.emplace(64, 64);

        constexpr float positions[] = {
            -0.5f,
            0.5f,
            0.0f,
            -0.5f,
            -0.5f,
            0.0f,
            0.5f,
            -0.5f,
            0.0f,
            0.5f,
            0.5f,
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

        shaders.reserve(3);
        shaders.emplace_back(res_path("shaders/quad.vert"), res_path("shaders/solid.frag"));
        shaders.emplace_back(res_path("shaders/quad.vert"), res_path("shaders/checker.frag"));
        shaders.emplace_back(res_path("shaders/quad.vert"), res_path("shaders/pulse.frag"));

        // Deterministic scene rng so numbers compare across runs.
        std::mt19937 rng(0xC0FFEEu);
        std::uniform_real_distribution<float> pos(-0.95f, 0.95f);
        std::uniform_real_distribution<float> scl(0.015f, 0.05f);
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

    void handle_event(const SDL_Event&) override {}
    void update(float, double) override {}

    void render(drz::Renderer& renderer) override {
        if (!registered) {
            for (size_t s = 0; s < shaders.size(); ++s) {
                pipelines[s] = renderer.register_state({.shader = shaders[s].handle()});
            }
            renderer.use_mesh_pool(*pool);
            registered = true;
        }

        renderer.clear(0.05f, 0.05f, 0.08f, 1.0f);

        for (size_t i = 0; i < quad_count; ++i) {
            drz::PipelineStateId pid = pipelines[shader_idx[i]];

            auto [offset, bytes] = renderer.push_draw_data(PerDraw {
                .offset = offsets[i],
                .scale = scales[i],
                ._pad = 0.0f,
                .tint = tints[i],
            });

            renderer.submit(drz::gen_sort_key(0, pid),
                            {
                                .state_id = pid,
                                .mesh = quad,
                                .draw_data_offset = offset,
                                .draw_data_bytes = bytes,
                            });
        }
    }
};

drz::App* create_app() {
    return new StressApp();
}
