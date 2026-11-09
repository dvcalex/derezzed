#include "drz/common/CameraController.hpp"

namespace drz
{

void FPSCameraController::update(float dt_ms)
{
    vec3 move_input {0.f};
    if (m_engine.key_down(SDL_SCANCODE_W))
        move_input.z += dt_ms * m_move_speed;
    if (m_engine.key_down(SDL_SCANCODE_S))
        move_input.z -= dt_ms * m_move_speed;
    if (m_engine.key_down(SDL_SCANCODE_A))
        move_input.x -= dt_ms * m_move_speed;
    if (m_engine.key_down(SDL_SCANCODE_D))
        move_input.x += dt_ms * m_move_speed;
    if (m_engine.key_down(SDL_SCANCODE_Q))
        move_input.y -= dt_ms * m_move_speed;
    if (m_engine.key_down(SDL_SCANCODE_E))
        move_input.y += dt_ms * m_move_speed;
    m_camera.translate_local(move_input.x, move_input.y, move_input.z);

    auto [x, y] = m_engine.mouse_delta();
    m_camera.rotate(x * m_rotate_speed, -y * m_rotate_speed);
}
} // namespace drz
