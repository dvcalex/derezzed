#include "drz/core/Engine.hpp"
#include "drz/core/App.hpp"
#include "drz/gfx/Renderer.hpp"
#include "drz/gfx/Shader.hpp"
#include "drz/gfx/MeshPool.hpp"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <optional>
#include <string>

static std::string res_path(const std::string& relative)
{
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class MatrixRain : public drz::App
{
public:
    void init() override
    {
        m_pool.emplace(64, 64);

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

        m_quad = m_pool->upload({
            .positions = positions,
            .uvs = uvs,
            .indices = indices,
        });

        m_shader.emplace(res_path("shaders/matrix.vert"), res_path("shaders/matrix.frag"));
    }

    void handle_event(const SDL_Event&) override {}

    void update(float, double elapsed) override
    {
        m_time = static_cast<float>(elapsed);
    }

    void render(drz::Renderer& renderer) override
    {
        if (!m_registered)
        {
            m_pipeline = renderer.register_state({.shader = m_shader->handle()});
            renderer.use_mesh_pool(*m_pool);
            m_registered = true;
        }

        renderer.clear(0.0f, 0.0f, 0.0f, 1.0f);

        auto [w, h] = m_engine->framebuffer_size();
        m_shader->set_uniform("u_time", m_time);
        m_shader->set_uniform("u_resolution", glm::vec2(static_cast<float>(w), static_cast<float>(h)));

        renderer.submit(drz::gen_sort_key(0, m_pipeline), {.state_id = m_pipeline, .mesh = m_quad});
    }

private:
    std::optional<drz::MeshPool> m_pool;
    drz::MeshHandle m_quad;
    std::optional<drz::Shader> m_shader;
    float m_time = 0.0f;

    bool m_registered = false;
    drz::PipelineStateId m_pipeline = 0;
};

drz::App* create_app()
{
    return new MatrixRain();
}
