#include "drz/core/Camera.hpp"
#include "drz/util/MathUtils.hpp"
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace drz
{
using drz::math::deg_to_rad;
using std::cos;
using std::sin;

void Camera::set_position(const vec3& pos)
{
    m_pos = pos;
}

void Camera::set_yaw(float yaw_deg)
{
    m_yaw_deg = yaw_deg;
}

void Camera::set_pitch(float pitch_deg)
{
    m_pitch_deg = std::clamp(pitch_deg, -89.0f, 89.0f);
}

vec3 Camera::forward() const
{
    float yaw_rad = m_yaw_deg * deg_to_rad<float>;
    float pitch_rad = m_pitch_deg * deg_to_rad<float>;
    return vec3(sin(yaw_rad) * cos(pitch_rad), sin(pitch_rad), -cos(yaw_rad) * cos(pitch_rad));
}

vec3 Camera::up() const
{
    return glm::cross(this->right(), this->forward());
}

vec3 Camera::right() const
{
    float yaw_rad = m_yaw_deg * deg_to_rad<float>;
    return vec3(cos(yaw_rad), 0.f, sin(yaw_rad));
}

mat4 Camera::view() const
{
    return glm::lookAt(m_pos, m_pos + forward(), vec3(0.f, 1.f, 0.f));
}

mat4 Camera::proj(float aspect) const
{
    return glm::perspective(m_fov_y_deg * deg_to_rad<float>, aspect, m_near, m_far);
}
} // namespace drz
