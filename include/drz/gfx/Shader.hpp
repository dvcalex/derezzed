#pragma once

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <cstdint>

namespace drz
{

class Shader
{
public:
    Shader(const std::string& vertex_path, const std::string& fragment_path);
    ~Shader();

    // Delete copy (can't just copy gl objects normally)
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Define move in source file
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Uniform setters
    void set_uniform(const std::string& name, int value);
    void set_uniform(const std::string& name, float value);
    void set_uniform(const std::string& name, const glm::vec2& value);
    void set_uniform(const std::string& name, const glm::vec3& value);
    void set_uniform(const std::string& name, const glm::vec4& value);
    void set_uniform(const std::string& name, const glm::mat4& value);

    // Get program handle for draw calls
    uint32_t handle() const
    {
        return m_program_id;
    }

private:
    uint32_t m_program_id = 0;
    std::unordered_map<std::string, int32_t> m_uniform_location_cache;

    int32_t get_uniform_location(const std::string& name);
};

} // namespace drz
