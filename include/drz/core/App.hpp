#pragma once

#include "drz/gfx/Renderer.hpp"

#include <SDL3/SDL_events.h>

namespace drz
{

class Engine;

class App
{
    friend Engine;

public:
    virtual ~App() = default;

    virtual void init() = 0;                                 // Called once when app starts
    virtual void handle_event(const SDL_Event& event) = 0;   // Called everytime SDL fires events
    virtual void update(float dt_ms, double elapsed_ms) = 0; // Called every frame
    virtual void render(Renderer& renderer) = 0;             // Called every frame

protected:
    Engine* m_engine = nullptr; // non-owning ref to engine
};

} // namespace drz
