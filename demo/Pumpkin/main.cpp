#include "drz/core/Engine.hpp"
#include "drz/core/App.hpp"
#include "drz/gfx/Renderer.hpp"
#include "drz/gfx/MeshPool.hpp"
#include "drz/gfx/ModelLoader.hpp"
#include "drz/util/Logger.hpp"

#include <SDL3/SDL.h>

#include <optional>
#include <string>

static std::string res_path(const std::string& relative)
{
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class PumpkinApp : public drz::App
{
public:
    void init() override
    {
        // roomy enough?
        m_pool.emplace(200000, 600000);

        const std::string path = res_path("pumpkin.glb");
        DRZ_LOGF("loading gltf: {}", path);

        m_mesh = drz::model_loader::load_mesh_gltf(path, *m_pool);

        if (m_mesh.id == 0)
            DRZ_LOG("mesh load returned invalid handle");
        else
            DRZ_LOGF("loaded mesh id={}", m_mesh.id);

        DRZ_FLUSH_LOG();
    }

    void handle_event(const SDL_Event&) override {}
    void update(float, double) override {}

    void render(drz::Renderer& renderer) override
    {
        if (!m_registered)
        {
            renderer.use_mesh_pool(*m_pool);
            m_registered = true;
        }

        renderer.clear(0.08f, 0.08f, 0.1f, 1.0f);
    }

private:
    std::optional<drz::MeshPool> m_pool;
    drz::MeshHandle m_mesh;
    bool m_registered = false;
};

drz::App* create_app()
{
    return new PumpkinApp();
}
