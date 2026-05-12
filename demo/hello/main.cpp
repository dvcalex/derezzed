#include <derezzed/engine.hpp>
#include <derezzed/app.hpp>
#include <derezzed/gl/shader.hpp>

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <memory>

static std::string res_path(const std::string& relative) {
    const char* base = SDL_GetBasePath();
    return std::string(base) + "res/" + relative;
}

class HelloApp : public drz::App {
public:
    void init() override {
        float vertices[] = {
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

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        shader = std::make_unique<drz::Shader>(res_path("shaders/hello.vert"), res_path("shaders/hello.frag"));
    }

    void handle_event(const SDL_Event& event) override {}

    void update(float dt, double elapsed) override {}

    void render() override {
        glClear(GL_COLOR_BUFFER_BIT);
        shader->bind();
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    std::unique_ptr<drz::Shader> shader;
};

drz::App* create_app() {
    return new HelloApp();
}
