#include <derezzed/shader.hpp>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

static std::string get_program_source(const std::string& filepath) {
    // open file and check for errors
    std::ifstream stream(filepath);
    if (!stream.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filepath);
    }

    std::string line;
    std::stringstream ss;

    while (getline(stream, line)) {
        ss << line << '\n';
    }
    std::string str = ss.str();
    return str;
}

static GLuint compile_shader(GLenum type, const std::string& source) {
    GLuint id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    // error handling for shader source code
    GLint result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        GLint len;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        std::string message(len, '\0');
        glGetShaderInfoLog(id, len, &len, message.data());

        glDeleteShader(id);

        throw std::runtime_error(std::string("Failed to compile ") + (type == GL_VERTEX_SHADER ? "vertex" : "fragment") +
                                 " shader:\n" + message);
    }
    return id;
}

namespace drz {

int32_t Shader::get_uniform_location(const std::string& name) {
    if (uniform_location_cache.find(name) != uniform_location_cache.end()) {
        return uniform_location_cache[name];
    }

    GLint location = glGetUniformLocation(program_id, name.c_str());
    if (location == -1) {
        std::cout << "Warning: uniform '" << name << "' doesn't exist or was optimized out." << std::endl;
    }

    uniform_location_cache[name] = location;
    return location;
}

Shader::Shader(const std::string& vertex_path, const std::string& fragment_path) {
    // grab source code
    std::string vert_src = get_program_source(vertex_path);
    std::string frag_src = get_program_source(fragment_path);

    // compile
    GLuint vert_id = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint frag_id = 0;
    try {
        frag_id = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    } catch (...) {
        glDeleteShader(vert_id); // cleanup vert shader if frag shader fails
        throw;                   // rethrow exception
    }

    // now create program
    program_id = glCreateProgram();

    // link
    glAttachShader(program_id, vert_id);
    glAttachShader(program_id, frag_id);
    glLinkProgram(program_id);

    // check link errors
    GLint result;
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        GLint len;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &len);
        std::string message(len, '\0');
        glGetProgramInfoLog(program_id, len, &len, message.data());

        glDeleteShader(vert_id);
        glDeleteShader(frag_id);
        glDeleteProgram(program_id);

        throw std::runtime_error("Failed to link shader program:\n" + message);
    }
    // glValidateProgram(program_id);

    // cleanup once built
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
}

Shader::~Shader() {
    glDeleteProgram(program_id);
}

Shader::Shader(Shader&& other) noexcept
    : program_id(other.program_id), uniform_location_cache(std::move(other.uniform_location_cache)) {
    other.program_id = 0; // prevent gl from releasing program
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        glDeleteProgram(program_id); // release current program
        program_id = other.program_id;
        other.program_id = 0; // prevent gl from releasing program
        uniform_location_cache = std::move(other.uniform_location_cache);
    }
    return *this;
}

void Shader::set_uniform(const std::string& name, int value) {
    glProgramUniform1i(program_id, get_uniform_location(name), value);
}

void Shader::set_uniform(const std::string& name, float value) {
    glProgramUniform1f(program_id, get_uniform_location(name), value);
}

void Shader::set_uniform(const std::string& name, const glm::vec2& value) {
    glProgramUniform2f(program_id, get_uniform_location(name), value.x, value.y);
}

void Shader::set_uniform(const std::string& name, const glm::vec3& value) {
    glProgramUniform3f(program_id, get_uniform_location(name), value.x, value.y, value.z);
}

void Shader::set_uniform(const std::string& name, const glm::vec4& value) {
    glProgramUniform4f(program_id, get_uniform_location(name), value.x, value.y, value.z, value.w);
}

void Shader::set_uniform(const std::string& name, const glm::mat4& value) {
    glProgramUniformMatrix4fv(program_id, get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace drz
