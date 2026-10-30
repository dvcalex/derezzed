#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace drz
{
using glm::mat4;
using glm::vec3;

// Assume right handed y-up coordinate system
class Camera
{
public:
    Camera() = default;
    inline Camera(vec3 pos, float yaw_deg, float pitch_deg) : m_pos(pos), m_yaw_deg(yaw_deg), m_pitch_deg(pitch_deg) {}

    void set_position(const vec3& pos);

    void set_yaw(float yaw_deg);

    void set_pitch(float pitch_deg);

    // calculate forward vector from yaw/pitch
    vec3 forward() const;

    // calculate right vector from forward and world up
    vec3 right() const;

    vec3 up() const;

    // build view matrix by transforming world -> camera space
    mat4 view() const;

    // build projection matrix by transforming camera -> clip space
    mat4 proj(float aspect) const;

private:
    vec3 m_pos;
    float m_yaw_deg = 0.0f;
    float m_pitch_deg = 0.0f;
    float m_fov_y_deg = 45.0f;
    float m_near = 0.1f;
    float m_far = 1000.0f;
};
} // namespace drz
