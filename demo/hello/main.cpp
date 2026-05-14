#include <derezzed/engine.hpp>
#include <derezzed/renderer.hpp>
#include <derezzed/shader.hpp>
#include <derezzed/mesh.hpp>
#include <derezzed/app.hpp>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <optional>
#include <vector>

static std::string res_path(const std::string& relative) {
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class HelloApp : public drz::App {
public:
    void init() override {
        const std::vector<glm::vec3> positions = {
            {0.0f, 0.5f, 0.0f},
            {-0.5f, -0.5f, 0.0f},
            {0.5f, -0.5f, 0.0f},
        };

        mesh.emplace(positions); // construct mesh in-place

        shader.emplace(res_path("shaders/hello.vert"), res_path("shaders/hello.frag"));
    }

    void handle_event(const SDL_Event& event) override {}

    void update(float dt, double elapsed) override {}

    void render(drz::Renderer& renderer) override {
        renderer.clear(0.1f, 0.1f, 0.1f, 1.0f);
        drz::DrawCommand cmd {
            .shader = shader->handle(),
            .vertex_layout = mesh->vertex_layout_handle(),
            .index_count = mesh->index_count(),
            .vertex_count = mesh->vertex_count(),
        };
        renderer.submit(cmd);
    }

private:
    // Optionals for stack allocation and late init inside of init()
    std::optional<drz::Mesh> mesh;
    std::optional<drz::Shader> shader;
};

drz::App* create_app() {
    return new HelloApp();
}
