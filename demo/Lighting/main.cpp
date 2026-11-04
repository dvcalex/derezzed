#include "drz/core/Engine.hpp"
#include "drz/core/App.hpp"
#include "drz/core/Camera.hpp"
#include "drz/gfx/Renderer.hpp"
#include "drz/gfx/Shader.hpp"
#include "drz/gfx/MeshPool.hpp"
#include "drz/gfx/ModelLoader.hpp"
#include "drz/gfx/Texture.hpp"
#include "drz/util/Logger.hpp"


#include <SDL3/SDL.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <optional>
#include <string>

using glm::mat4;
using glm::vec3;
using glm::vec4;

static std::string res_path(const std::string& relative)
{
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class Lighting : public drz::App
{
    struct PerDraw
    {
        mat4 model;
    };

public:
    void init(drz::Renderer& renderer) override
    {
        m_pool.emplace(200000, 600000);

        const std::string path = res_path("cube.glb");
        m_mesh = drz::model_loader::load_mesh_gltf(path, *m_pool);
        if (m_mesh.id == 0)
        {
            throw std::runtime_error("mesh load failed");
        }

        // setup material system components
        m_shader.emplace(res_path("shaders/lit_cube.vert"), res_path("shaders/lit_cube.frag"));
        m_albedo_tex.emplace(res_path("cereal.png"));
        m_shader->set_texture_slot("u_albedo", 0);
        m_albedo_tex->bind(0);

        m_camera.emplace(vec3(0.f, 2.f, 13.f), 0.f, -15.f); // setup camera

        // Renderer registration
        m_pipeline = renderer.register_state({.shader = m_shader->handle(),
                                              .enable_depth_test = true,
                                              .enable_depth_write = true,
                                              .enable_cull_face = true});
        renderer.use_mesh_pool(*m_pool);
    }

    void handle_event(const SDL_Event&) override {}

    void update(float dt_ms, double) override {}

    void render(drz::Renderer& renderer) override
    {
        renderer.clear(0.05f, 0.06f, 0.08f, 1.0f);

        auto [w, h] = m_engine->framebuffer_size();
        m_shader->set_uniform("u_view", m_camera->view());
        m_shader->set_uniform("u_proj", m_camera->proj(static_cast<float>(w) / h));
        m_shader->set_uniform("u_camera_pos", m_camera->position());
        m_shader->set_uniform("u_light_pos", vec3(2.f, 3.f, 2.f));
        m_shader->set_uniform("u_light_color", vec3(1.f, 0.95f, 0.9f));

        const uint32_t base_instance_index = renderer.push_draw_data(PerDraw {.model = mat4(1.f)});
        renderer.submit(drz::gen_sort_key(0, m_pipeline),
                        {.state_id = m_pipeline, .mesh = m_mesh, .base_instance = base_instance_index});
    }

private:
    std::optional<drz::MeshPool> m_pool;
    drz::MeshHandle m_mesh;
    std::optional<drz::Shader> m_shader;
    std::optional<drz::Texture> m_albedo_tex;
    drz::PipelineStateId m_pipeline = 0;
    std::optional<drz::Camera> m_camera;
};

drz::App* create_app()
{
    return new Lighting();
}
