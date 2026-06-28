#pragma once

#include "drz/gfx/renderer.hpp"

#include <SDL3/SDL_events.h>

namespace drz
{

class Engine;

class App
{
public:
    virtual ~App() = default;

    /**
     * @brief Called once when app starts.
     *
     */
    virtual void Init() = 0;

    /**
     * @brief Called everytime SDL fires events.
     *
     * @param event
     */
    virtual void HandleEvent(const SDL_Event& event) = 0;

    /**
     * @brief Called every frame
     *
     * @param dt
     * @param elapsed
     */
    virtual void Update(float dt, double elapsed) = 0;

    /**
     * @brief Called every frame
     *
     * @param renderer
     */
    virtual void Render(Renderer& renderer) = 0;

protected:
    Engine* m_engine = nullptr; // non-owning ref to engine
    friend class Engine;        // Allow engine to set engine pointer referenced in app
};

} // namespace drz
