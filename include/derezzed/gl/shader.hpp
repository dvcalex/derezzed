#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

namespace drz {

class Shader {
public:
    Shader(const std::string& vertex_path, const std::string& fragment_path);
    ~Shader();

    // Delete copy (can't just copy gl objects normally)
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Implement move
    Shader(Shader&& other) noexcept : program_id(other.program_id), uniform_location_cache(std::move(other.uniform_location_cache)) {
        other.program_id = 0; // prevent gl from releasing program
    }
    Shader& operator=(Shader&& other) noexcept {
        if (this != &other) {
            glDeleteProgram(program_id); // release current program
            program_id = other.program_id;
            other.program_id = 0; // prevent gl from releasing program
            uniform_location_cache = std::move(other.uniform_location_cache);
        }
        return *this;
    }

    void bind() const;
    void unbind() const;

    // Uniform setters
    void set_uniform(const std::string& name, int value);
    void set_uniform(const std::string& name, float value);
    void set_uniform(const std::string& name, const glm::vec2& value);
    void set_uniform(const std::string& name, const glm::vec3& value);
    void set_uniform(const std::string& name, const glm::vec4& value);
    void set_uniform(const std::string& name, const glm::mat4& value);

private:
    GLuint program_id = 0;
    std::unordered_map<std::string, int> uniform_location_cache; // cache for uniform locations

    GLint get_uniform_location(const std::string& name);
};

} // namespace drz
