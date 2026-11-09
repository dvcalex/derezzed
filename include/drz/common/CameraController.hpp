#pragma once

#include "drz/core/Camera.hpp"
#include "drz/core/Engine.hpp"

namespace drz
{
class CameraController
{
public:
    virtual ~CameraController() = default;
    CameraController(Camera& camera, Engine& engine) : m_camera(camera), m_engine(engine) {}
    virtual void update(float dt_ms) = 0;

protected:
    // non-owning refs
    Camera& m_camera;
    Engine& m_engine;
};

class FPSCameraController : public CameraController
{
public:
    FPSCameraController(Camera& camera, Engine& engine, float move_speed, float rotate_speed)
        : CameraController(camera, engine), m_move_speed(move_speed), m_rotate_speed(rotate_speed)
    {
    }

    void update(float dt_ms) override;

private:
    float m_move_speed = 0;
    float m_rotate_speed = 0;
};
} // namespace drz
