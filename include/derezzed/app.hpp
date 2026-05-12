#pragma once

#include <SDL3/SDL_events.h>

namespace drz {

class Engine;

class App {
public:
    virtual ~App() = default;
    virtual void init() = 0;                               // Called once when app starts
    virtual void handle_event(const SDL_Event& event) = 0; // Called everytime SDL fires events
    virtual void update(float dt, double elapsed) = 0;     // Called every frame
    virtual void render() = 0;                             // Called every frame

protected:
    Engine* engine = nullptr; // non-owning ref to engine
    friend class Engine;      // Allow engine to set engine pointer referenced in app
};

} // namespace drz
