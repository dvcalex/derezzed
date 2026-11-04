#include "drz/core/Engine.hpp"
#include "drz/core/App.hpp"
#include "drz/gfx/Renderer.hpp"
#include "drz/gfx/Shader.hpp"
#include "drz/gfx/MeshPool.hpp"
#include "drz/gfx/ModelLoader.hpp"
#include "drz/util/Logger.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

using glm::mat4;
using glm::vec3;
using glm::vec4;

static std::string res_path(const std::string& relative)
{
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class ManyCubesApp : public drz::App
{
    struct PerDraw
    {
        mat4 model;
        vec4 tint;
    };

public:
    static constexpr int GRID = 200;
    static constexpr size_t PIPELINE_COUNT = 128;

    void init(drz::Renderer& renderer) override
    {
        m_pool.emplace(200000, 600000);

        const std::string path = res_path("cube.glb");
        m_mesh = drz::model_loader::load_mesh_gltf(path, *m_pool);
        if (m_mesh.id == 0)
        {
            DRZ_LOG("mesh load failed");
            return;
        }

        m_shader.emplace(res_path("shaders/cube.vert"), res_path("shaders/cube.frag"));

        build_scene();

        for (size_t i = 0; i < m_pipelines.size(); ++i)
        {
            m_pipelines[i] = renderer.register_state({.shader = m_shader->handle(),
                                                      .enable_depth_test = true,
                                                      .enable_depth_write = true,
                                                      .enable_cull_face = true,
                                                      .enable_scissor_test = true,
                                                      .scissor_width = 16384 + static_cast<int>(i),
                                                      .scissor_height = 16384});
        }
        SDL_GL_SetSwapInterval(0); // temporary to avoid the refresh rate cap
        renderer.use_mesh_pool(*m_pool);

        DRZ_LOGF("scene grid: {}x{}={}", GRID, GRID, GRID * GRID);
        DRZ_FLUSH_LOG();
    }

    void handle_event(const SDL_Event&) override {}

    void update(float dt_ms, double) override
    {
        m_time_s += dt_ms * 0.001f;
    }

    void render(drz::Renderer& renderer) override
    {
        const vec3 eye = orbit_eye();

        renderer.clear(BG_COLOR.x, BG_COLOR.y, BG_COLOR.z, 1.0f);

        auto [w, h] = m_engine->framebuffer_size();
        const mat4 view = glm::lookAt(eye, vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f));
        const mat4 proj = glm::perspective(glm::radians(45.0f), static_cast<float>(w) / h, 0.1f, ORBIT_RADIUS * 6.f);

        m_shader->set_uniform("u_view", view);
        m_shader->set_uniform("u_proj", proj);

        for (size_t i = 0; i < m_models.size(); ++i)
            submit_draw(renderer, m_mesh, m_models[i], m_tints[i], static_cast<uint32_t>(i));
    }

private:
    void submit_draw(drz::Renderer& renderer, drz::MeshHandle mesh, const mat4& model, const vec4& tint, uint32_t index)
    {
        const uint32_t base_instance_index = renderer.push_draw_data(PerDraw {.model = model, .tint = tint});

        uint32_t mixed = index * 747796405u + 2891336453u;
        mixed ^= mixed >> 16;
        const auto pipeline = m_pipelines[mixed % PIPELINE_COUNT];
        const auto pass = static_cast<uint8_t>((mixed >> 16) % 32);
        renderer.submit(drz::gen_sort_key(pass, pipeline, mixed),
                        {.state_id = pipeline, .mesh = mesh, .base_instance = base_instance_index});
    }

    void build_scene()
    {
        m_models.reserve(GRID * GRID);
        m_tints.reserve(GRID * GRID);

        std::mt19937 rng(0xD1CE);
        std::uniform_real_distribution<float> yaw_dist(0.f, 6.2831853f);
        std::uniform_real_distribution<float> jitter(-0.7f, 0.7f);

        std::uniform_real_distribution<float> hue_dist(0.f, 1.f);
        std::uniform_real_distribution<float> sat_dist(0.55f, 0.95f);
        std::uniform_real_distribution<float> val_dist(0.65f, 1.0f);
        auto hsv_to_rgb = [](float h, float s, float v) -> vec3
        {
            vec3 p = glm::abs(glm::fract(vec3(h) + vec3(0.f, 2.f / 3.f, 1.f / 3.f)) * 6.f - 3.f);
            return v * glm::mix(vec3(1.f), glm::clamp(p - 1.f, 0.f, 1.f), s);
        };

        const float half = (GRID - 1) * 0.5f;
        for (int i = 0; i < GRID; ++i)
        {
            for (int j = 0; j < GRID; ++j)
            {
                mat4 m = glm::translate(mat4(1.f), vec3((i - half) * SPACING, 0.f, (j - half) * SPACING)) *
                    glm::rotate(mat4(1.f), yaw_dist(rng), vec3(0.f, 1.f, 0.f));

                float jit = jitter(rng);
                std::uniform_real_distribution<float> hue_dist(0.f, 1.f);
                std::uniform_real_distribution<float> sat_dist(0.55f, 0.95f);
                std::uniform_real_distribution<float> val_dist(0.65f, 1.0f);

                vec3 rgb = hsv_to_rgb(hue_dist(rng), sat_dist(rng), val_dist(rng));
                vec4 tint(rgb, 1.f);

                m_models.push_back(m);
                m_tints.push_back(tint);
            }
        }
    }

    vec3 orbit_eye() const
    {
        const float a = m_time_s * ORBIT_SPEED;
        return vec3(std::cos(a) * ORBIT_RADIUS, ORBIT_HEIGHT + std::sin(a * 2.3f) * 1.5f, std::sin(a) * ORBIT_RADIUS);
    }

    static constexpr float SPACING = 3.6f;
    static constexpr float GRID_HALF_EXTENT = (GRID - 1) * 0.5f * SPACING + 2.f;
    static constexpr float ORBIT_RADIUS = GRID_HALF_EXTENT * 1.7f + 4.f;
    static constexpr float ORBIT_HEIGHT = ORBIT_RADIUS * 0.38f + 2.f;
    static constexpr float ORBIT_SPEED = 0.4;
    static constexpr vec3 BG_COLOR = vec3(0.05f, 0.06f, 0.08f);

    std::optional<drz::MeshPool> m_pool;
    drz::MeshHandle m_mesh;
    std::optional<drz::Shader> m_shader;
    std::array<drz::PipelineStateId, PIPELINE_COUNT> m_pipelines {};

    std::vector<mat4> m_models;
    std::vector<vec4> m_tints;

    double m_time_s = 0.0;
};

drz::App* create_app()
{
    return new ManyCubesApp();
}
