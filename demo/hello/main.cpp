#include <derezzed/engine.hpp>
#include <derezzed/app.hpp>

#include <iostream>

class HelloApp : public drz::App {
public:
    void init() override {
        std::cout << "App inited!\n";
    }

    void handle_event(const SDL_Event& event) override {
        // For handling SDL events on user side
    }

    void update(float dt, double elapsed) override {}

    void render() override {}
};

drz::App* create_app() {
    return new HelloApp();
}
