#include "drz/gfx/shader.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

static std::string GetProgramSource(const std::string& filepath)
{
    // open file and check for errors
    std::ifstream stream(filepath);
    if (!stream.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + filepath);
    }

    std::string line;
    std::stringstream ss;

    while (getline(stream, line))
    {
        ss << line << '\n';
    }
    std::string str = ss.str();
    return str;
}

static GLuint CompileShader(GLenum type, const std::string& source)
{
    GLuint id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    // error handling for shader source code
    GLint result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        GLint len;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        std::string message(len, '\0');
        glGetShaderInfoLog(id, len, &len, message.data());

        glDeleteShader(id);

        throw std::runtime_error(std::string("Failed to compile ") +
                                 (type == GL_VERTEX_SHADER ? "vertex" : "fragment") + " shader:\n" + message);
    }
    return id;
}

namespace drz
{

int32_t Shader::GetUniformLocation(const std::string& name)
{
    if (m_uniform_location_cache.find(name) != m_uniform_location_cache.end())
    {
        return m_uniform_location_cache[name];
    }

    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    if (location == -1)
    {
        std::cout << "Warning: uniform '" << name << "' doesn't exist or was optimized out." << std::endl;
    }

    m_uniform_location_cache[name] = location;
    return location;
}

Shader::Shader(const std::string& vertex_path, const std::string& fragment_path)
{
    // grab source code
    std::string vert_src = GetProgramSource(vertex_path);
    std::string frag_src = GetProgramSource(fragment_path);

    // compile
    GLuint vert_id = CompileShader(GL_VERTEX_SHADER, vert_src);
    GLuint frag_id = 0;
    try
    {
        frag_id = CompileShader(GL_FRAGMENT_SHADER, frag_src);
    }
    catch (...)
    {
        glDeleteShader(vert_id); // cleanup vert shader if frag shader fails
        throw;                   // rethrow exception
    }

    // now create program
    m_program_id = glCreateProgram();

    // link
    glAttachShader(m_program_id, vert_id);
    glAttachShader(m_program_id, frag_id);
    glLinkProgram(m_program_id);

    // check link errors
    GLint result;
    glGetProgramiv(m_program_id, GL_LINK_STATUS, &result);
    if (result == GL_FALSE)
    {
        GLint len;
        glGetProgramiv(m_program_id, GL_INFO_LOG_LENGTH, &len);
        std::string message(len, '\0');
        glGetProgramInfoLog(m_program_id, len, &len, message.data());

        glDeleteShader(vert_id);
        glDeleteShader(frag_id);
        glDeleteProgram(m_program_id);

        throw std::runtime_error("Failed to link shader program:\n" + message);
    }
    // glValidateProgram(program_id);

    // cleanup once built
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
}

Shader::~Shader()
{
    glDeleteProgram(m_program_id);
}

Shader::Shader(Shader&& other) noexcept
    : m_program_id(other.m_program_id), m_uniform_location_cache(std::move(other.m_uniform_location_cache))
{
    other.m_program_id = 0; // prevent gl from releasing program
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        glDeleteProgram(m_program_id); // release current program
        m_program_id = other.m_program_id;
        other.m_program_id = 0; // prevent gl from releasing program
        m_uniform_location_cache = std::move(other.m_uniform_location_cache);
    }
    return *this;
}

void Shader::SetUniform(const std::string& name, int value)
{
    glProgramUniform1i(m_program_id, GetUniformLocation(name), value);
}

void Shader::SetUniform(const std::string& name, float value)
{
    glProgramUniform1f(m_program_id, GetUniformLocation(name), value);
}

void Shader::SetUniform(const std::string& name, const glm::vec2& value)
{
    glProgramUniform2f(m_program_id, GetUniformLocation(name), value.x, value.y);
}

void Shader::SetUniform(const std::string& name, const glm::vec3& value)
{
    glProgramUniform3f(m_program_id, GetUniformLocation(name), value.x, value.y, value.z);
}

void Shader::SetUniform(const std::string& name, const glm::vec4& value)
{
    glProgramUniform4f(m_program_id, GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::SetUniform(const std::string& name, const glm::mat4& value)
{
    glProgramUniformMatrix4fv(m_program_id, GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace drz
