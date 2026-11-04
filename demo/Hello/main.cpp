#include "drz/core/Engine.hpp"
#include "drz/core/App.hpp"
#include "drz/gfx/Renderer.hpp"
#include "drz/gfx/Shader.hpp"
#include "drz/gfx/MeshPool.hpp"
#include "drz/gfx/Texture.hpp"

#include <SDL3/SDL.h>

#include <optional>
#include <string>

static std::string res_path(const std::string& relative)
{
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class HelloApp : public drz::App
{
public:
    void init(drz::Renderer& renderer) override
    {
        m_pool.emplace(64, 64);

        constexpr float positions[] = {0.5f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f};
        constexpr float uvs[] = {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        constexpr uint32_t indices[] = {0, 1, 2, 2, 3, 0};

        m_triangle = m_pool->upload({
            .positions = positions,
            .uvs = uvs,
            .indices = indices,
        });

        m_shader.emplace(res_path("shaders/hello.vert"), res_path("shaders/hello.frag"));
        m_albedo_tex.emplace(res_path("cereal.png"));

        uint32_t albedo_slot = 0;
        m_shader->set_texture_slot("u_albedo", albedo_slot);
        m_albedo_tex->bind(albedo_slot);

        m_pipeline = renderer.register_state({.shader = m_shader->handle()});
        renderer.use_mesh_pool(*m_pool);
    }

    void handle_event(const SDL_Event&) override {}
    void update(float, double) override {}

    void render(drz::Renderer& renderer) override
    {
        renderer.clear(0.1f, 0.1f, 0.1f, 1.0f);
        renderer.submit(drz::gen_sort_key(0, m_pipeline), {.state_id = m_pipeline, .mesh = m_triangle});
    }

private:
    std::optional<drz::MeshPool> m_pool;
    drz::MeshHandle m_triangle;
    std::optional<drz::Shader> m_shader;
    std::optional<drz::Texture> m_albedo_tex;

    drz::PipelineStateId m_pipeline = 0;
};

drz::App* create_app()
{
    return new HelloApp();
}
