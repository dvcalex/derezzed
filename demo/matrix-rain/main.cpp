#include "drz/core/engine.hpp"
#include "drz/core/app.hpp"
#include "drz/gfx/renderer.hpp"
#include "drz/gfx/shader.hpp"
#include "drz/gfx/mesh_pool.hpp"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <optional>
#include <string>

static std::string ResPath(const std::string& relative)
{
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class MatrixRain : public drz::App
{
public:
    void Init() override
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

        m_quad = m_pool->Upload({
            .positions = positions,
            .uvs = uvs,
            .indices = indices,
        });

        m_shader.emplace(ResPath("shaders/matrix.vert"), ResPath("shaders/matrix.frag"));
    }

    void HandleEvent(const SDL_Event&) override {}

    void Update(float, double elapsed) override
    {
        m_time = static_cast<float>(elapsed);
    }

    void Render(drz::Renderer& renderer) override
    {
        if (!m_registered)
        {
            m_pipeline = renderer.RegisterState({.shader = m_shader->Handle()});
            renderer.UseMeshPool(*m_pool);
            m_registered = true;
        }

        renderer.Clear(0.0f, 0.0f, 0.0f, 1.0f);

        auto [w, h] = m_engine->FramebufferSize();
        m_shader->SetUniform("u_time", m_time);
        m_shader->SetUniform("u_resolution", glm::vec2(static_cast<float>(w), static_cast<float>(h)));

        renderer.Submit(drz::GenSortKey(0, m_pipeline), {.state_id = m_pipeline, .mesh = m_quad});
    }

private:
    std::optional<drz::MeshPool> m_pool;
    drz::MeshHandle m_quad;
    std::optional<drz::Shader> m_shader;
    float m_time = 0.0f;

    bool m_registered = false;
    drz::PipelineStateId m_pipeline = 0;
};

drz::App* CreateApp()
{
    return new MatrixRain();
}
