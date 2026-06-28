#include "drz/core/engine.hpp"
#include "drz/core/app.hpp"
#include "drz/gfx/renderer.hpp"
#include "drz/gfx/shader.hpp"
#include "drz/gfx/mesh_pool.hpp"

#include <SDL3/SDL.h>

#include <optional>
#include <string>

static std::string ResPath(const std::string& relative)
{
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class HelloApp : public drz::App
{
public:
    void Init() override
    {
        m_pool.emplace(64, 64);

        constexpr float positions[] = {
            0.0f,
            0.5f,
            0.0f,
            -0.5f,
            -0.5f,
            0.0f,
            0.5f,
            -0.5f,
            0.0f,
        };
        constexpr float uvs[] = {
            0.5f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
        };
        constexpr uint32_t indices[] = {0, 1, 2};

        m_triangle = m_pool->Upload({
            .positions = positions,
            .uvs = uvs,
            .indices = indices,
        });

        m_shader.emplace(ResPath("shaders/hello.vert"), ResPath("shaders/hello.frag"));
    }

    void HandleEvent(const SDL_Event&) override {}
    void Update(float, double) override {}

    void Render(drz::Renderer& renderer) override
    {
        if (!m_registered)
        {
            m_pipeline = renderer.RegisterState({.shader = m_shader->Handle()});
            renderer.UseMeshPool(*m_pool);
            m_registered = true;
        }

        renderer.Clear(0.1f, 0.1f, 0.1f, 1.0f);
        renderer.Submit(drz::GenSortKey(0, m_pipeline), {.state_id = m_pipeline, .mesh = m_triangle});
    }

private:
    std::optional<drz::MeshPool> m_pool;
    drz::MeshHandle m_triangle;
    std::optional<drz::Shader> m_shader;

    bool m_registered = false;
    drz::PipelineStateId m_pipeline = 0;
};

drz::App* CreateApp()
{
    return new HelloApp();
}
