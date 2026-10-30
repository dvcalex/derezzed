#include "drz/core/Engine.hpp"
#include "drz/core/App.hpp"
#include "drz/gfx/Renderer.hpp"
#include "drz/gfx/Shader.hpp"
#include "drz/gfx/MeshPool.hpp"

#include <SDL3/SDL.h>

#include <glm/gtc/matrix_transform.hpp>

#include <optional>
#include <string>

using glm::mat4;
using glm::vec3;

static std::string res_path(const std::string& relative)
{
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class Scene3D : public drz::App
{
public:
    void handle_event(const SDL_Event&) override {}
    void update(float, double) override {}

    void init() override
    {
        m_pool.emplace(1024, 1024);

        constexpr float positions[] = {
            -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, -0.5f,
            0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,
            -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f,
            -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,
        };
        constexpr float normals[] = {
            0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1,
            1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1, 0,  0,
            0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,  -1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1, 0,
        };
        constexpr float uvs[] = {
            0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1,
            0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1,
        };
        constexpr uint32_t indices[] = {
            0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,
            12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20,
        };

        drz::MeshDesc cube_description {.positions = positions, .normals = normals, .uvs = uvs, .indices = indices};
        m_cube = m_pool->upload(cube_description);

        m_shader.emplace(res_path("shaders/cube.vert"), res_path("shaders/cube.frag"));

        // cube at origin
        m_models[0] = mat4(1.0f);

        // cube floated up and right, and shrunk
        m_models[1] = glm::translate(mat4(1.0f), vec3(1.8f, 0.9f, 0.0f)) * glm::scale(mat4(1.0f), vec3(0.6f));

        // cube pushed back and left and rotated 45 deg about y axis
        m_models[2] = glm::translate(mat4(1.0f), vec3(-1.6f, 0.0f, -1.2f)) *
            glm::rotate(mat4(1.0f), glm::radians(45.0f), vec3(0.0f, 1.0f, 0.0f));
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

        renderer.clear(0.08f, 0.08f, 0.1f, 1.0f);

        auto [w, h] = m_engine->framebuffer_size();
        const mat4 view = glm::lookAt(vec3(3.f, 3.f, 5.f), vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f));
        const mat4 proj = glm::perspective(glm::radians(45.0f), static_cast<float>(w) / h, 0.1f, 100.f);

        m_shader->set_uniform("u_view", view);
        m_shader->set_uniform("u_proj", proj);

        for (int i = 0; i < 3; ++i)
        {
            auto [offset, bytes] = renderer.push_draw_data(PerDraw {.model = m_models[i]});
            renderer.submit(
                drz::gen_sort_key(0, m_pipeline),
                {.state_id = m_pipeline, .mesh = m_cube, .draw_data_offset = offset, .draw_data_bytes = bytes});
        }
    }

private:
    struct PerDraw
    {
        glm::mat4 model;
    };
    static_assert(sizeof(PerDraw) == 64);

    std::optional<drz::MeshPool> m_pool;
    drz::MeshHandle m_cube;
    std::optional<drz::Shader> m_shader;
    bool m_registered = false;
    drz::PipelineStateId m_pipeline = 0;
    mat4 m_models[3]; // 3 cubes in different spots
};

drz::App* create_app()
{
    return new Scene3D();
}
