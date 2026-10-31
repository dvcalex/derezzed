#include "drz/core/Engine.hpp"
#include "drz/core/App.hpp"
#include "drz/gfx/Renderer.hpp"
#include "drz/gfx/MeshPool.hpp"
#include "drz/gfx/ModelLoader.hpp"
#include "drz/util/Logger.hpp"

#include <SDL3/SDL.h>

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

class PumpkinApp : public drz::App
{
    // per-draw payload, mirrored in cube.vert's PerDraw (std430).
    // NOTE: size MUST divide the FrameRingBuffer region bytes (2MB) exactly — the renderer derives
    // base_instance as global_offset / slot_bytes, so any remainder shifts every draw after a region
    // wrap. 64+16=80 does NOT divide 2^21; padded to 128 it does (16384 slots/region).
    struct PerDraw
    {
        mat4 model;
        vec4 tint;
        vec4 _pad[3];
    };
    static_assert(sizeof(PerDraw) == 128);

public:
    // tweak me: GRID*GRID pumpkins. ceiling ~114x114 (indirect ring = 256KB/frame / 20B per command).
    static constexpr int GRID = 12;

    void init() override
    {
        m_pool.emplace(200000, 600000);

        const std::string path = res_path("pumpkin.glb");
        DRZ_LOGF("loading gltf: {}", path);
        m_mesh = drz::model_loader::load_mesh_gltf(path, *m_pool);
        if (m_mesh.id == 0)
        {
            DRZ_LOG("mesh load failed — scene will stay empty");
            return;
        }

        // ground quad: unit XZ plane at y=0, +Y normal, CCW when viewed from above
        constexpr float gp[] = {-0.5f, 0.f, 0.5f, 0.5f, 0.f, 0.5f, 0.5f, 0.f, -0.5f, -0.5f, 0.f, -0.5f};
        constexpr float gn[] = {0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f};
        constexpr float gu[] = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};
        constexpr uint32_t gi[] = {0, 1, 2, 0, 2, 3};

        drz::MeshDesc ground {.positions = gp, .normals = gn, .uvs = gu, .indices = gi};
        m_ground = m_pool->upload(ground);

        m_shader.emplace(res_path("shaders/cube.vert"), res_path("shaders/cube.frag"));

        build_scene();

        DRZ_LOGF("scene: {}x{} grid = {} pumpkins + ground", GRID, GRID, GRID * GRID);
        DRZ_FLUSH_LOG();
    }

    void handle_event(const SDL_Event&) override {}

    void update(float dt_ms, double) override
    {
        m_time_s += dt_ms * 0.001f; // only the camera moves; pumpkins are frozen
    }

    void render(drz::Renderer& renderer) override
    {
        if (!m_registered)
        {
            m_pipeline = renderer.register_state({.shader = m_shader->handle(),
                                                  .enable_depth_test = true,
                                                  .enable_depth_write = true,
                                                  .enable_cull_face = true});
            renderer.use_mesh_pool(*m_pool);
            m_registered = true;
        }

        const vec3 eye = orbit_eye();

        renderer.clear(FOG_COLOR.x, FOG_COLOR.y, FOG_COLOR.z, 1.0f);

        auto [w, h] = m_engine->framebuffer_size();
        const mat4 view = glm::lookAt(eye, vec3(0.f, PUMPKIN_HEIGHT * 0.5f, 0.f), vec3(0.f, 1.f, 0.f));
        const mat4 proj = glm::perspective(glm::radians(45.0f), static_cast<float>(w) / h, 0.1f, ORBIT_RADIUS * 6.f);

        m_shader->set_uniform("u_view", view);
        m_shader->set_uniform("u_proj", proj);
        m_shader->set_uniform("u_cam_pos", eye);
        m_shader->set_uniform("u_fog_color", FOG_COLOR);
        m_shader->set_uniform("u_fog_near", ORBIT_RADIUS * 0.7f);
        m_shader->set_uniform("u_fog_far", (ORBIT_RADIUS + GRID_HALF_EXTENT) * 1.95f);

        // ground first, then the whole grid — all share one state_id so this is ONE multi-draw run
        submit_draw(renderer, m_ground, m_ground_model, GROUND_TINT);
        for (size_t i = 0; i < m_models.size(); ++i)
            submit_draw(renderer, m_mesh, m_models[i], m_tints[i]);
    }

private:
    void submit_draw(drz::Renderer& renderer, drz::MeshHandle mesh, const mat4& model, const vec4& tint)
    {
        auto [offset, bytes] = renderer.push_draw_data(PerDraw {.model = model, .tint = tint});
        renderer.submit(drz::gen_sort_key(0, m_pipeline),
                        {.state_id = m_pipeline, .mesh = mesh, .draw_data_offset = offset, .draw_data_bytes = bytes});
    }

    void build_scene()
    {
        const float ground_size = GRID_HALF_EXTENT * 6.f;
        m_ground_model = glm::scale(mat4(1.f), vec3(ground_size, 1.f, ground_size));

        m_models.reserve(GRID * GRID);
        m_tints.reserve(GRID * GRID);

        std::mt19937 rng(0xD1CE); // fixed seed: same layout every run
        std::uniform_real_distribution<float> yaw_dist(0.f, 6.2831853f);
        std::uniform_real_distribution<float> jitter(-0.07f, 0.07f);

        const float half = (GRID - 1) * 0.5f;
        for (int i = 0; i < GRID; ++i)
        {
            for (int j = 0; j < GRID; ++j)
            {
                // static transform: yaw in place, then move to grid slot — pumpkins never move again
                mat4 m = glm::translate(mat4(1.f), vec3((i - half) * SPACING, 0.f, (j - half) * SPACING)) *
                    glm::rotate(mat4(1.f), yaw_dist(rng), vec3(0.f, 1.f, 0.f));

                // pumpkin-orange with per-instance jitter so the grid doesn't look cloned
                float j = jitter(rng);
                vec4 tint(1.0f + j, 0.52f + j * 0.8f, 0.16f + j * 0.5f, 1.f);

                m_models.push_back(m);
                m_tints.push_back(tint);
            }
        }
    }

    vec3 orbit_eye() const
    {
        const float a = static_cast<float>(m_time_s) * ORBIT_SPEED;
        return vec3(std::cos(a) * ORBIT_RADIUS, ORBIT_HEIGHT + std::sin(a * 2.3f) * 1.5f, std::sin(a) * ORBIT_RADIUS);
    }

    // scene constants — the pumpkin glb is ~2.6 wide x 3 tall sitting on y=0 (from its POSITION min/max)
    static constexpr float PUMPKIN_HEIGHT = 3.1f;
    static constexpr float SPACING = 3.6f;
    static constexpr float GRID_HALF_EXTENT = (GRID - 1) * 0.5f * SPACING + 2.f;
    static constexpr float ORBIT_RADIUS = GRID_HALF_EXTENT * 1.7f + 4.f;
    static constexpr float ORBIT_HEIGHT = ORBIT_RADIUS * 0.38f + 2.f;
    static constexpr double ORBIT_SPEED = 0.22; // rad/s, full circle ~29s
    static constexpr vec3 FOG_COLOR = vec3(0.05f, 0.06f, 0.08f);
    static constexpr vec4 GROUND_TINT = vec4(0.13f, 0.14f, 0.12f, 1.f);

    std::optional<drz::MeshPool> m_pool;
    drz::MeshHandle m_mesh;
    drz::MeshHandle m_ground;
    mat4 m_ground_model {1.f};
    std::optional<drz::Shader> m_shader;
    bool m_registered = false;
    drz::PipelineStateId m_pipeline = 0;

    std::vector<mat4> m_models; // static, built once in init()
    std::vector<vec4> m_tints;

    double m_time_s = 0.0;
};

drz::App* create_app()
{
    return new PumpkinApp();
}
