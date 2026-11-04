#include "drz/gfx/Shader.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

static std::string get_program_source(const std::string& filepath)
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

// Takes source code as a string, extracts linked files, then replace each include where it appears in the original
// source. Returns new source with linkage handled.
static std::string handle_linkage(const std::string& source, const std::string& source_filepath)
{
    std::istringstream source_stream(source);
    std::string line;
    std::string output;

    while (std::getline(source_stream, line))
    {
        std::istringstream line_stream(line);
        std::string directive;
        line_stream >> directive;

        if (directive == "#include")
        {
            // parse quoted file path from remainder of line_stream
            line_stream >> std::ws;

            if (line_stream.get() != '"')
            {
                throw std::runtime_error("Expected quoted include path in " + source_filepath);
            }

            std::string include_path;
            if (!std::getline(line_stream, include_path, '"') || line_stream.eof() || include_path.empty())
            {
                throw std::runtime_error("Invalid include path in " + source_filepath);
            }

            const auto resolved_path =
                (std::filesystem::path(source_filepath).parent_path() / include_path).lexically_normal();

            const std::string filepath = resolved_path.string();

            const std::string included_source = get_program_source(filepath);

            output += handle_linkage(included_source, filepath);
            output += '\n';
        }
        else
        {
            // append the original line and '\n' to output
            output += line;
            output += '\n';
        }
    }
    return output;
}

static GLuint compile_shader(GLenum type, const std::string& source)
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

int32_t Shader::get_uniform_location(const std::string& name)
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
    std::string vert_src = get_program_source(vertex_path);
    std::string frag_src = get_program_source(fragment_path);

    // handle linkage
    vert_src = handle_linkage(vert_src, vertex_path);
    frag_src = handle_linkage(frag_src, fragment_path);

    // compile
    GLuint vert_id = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint frag_id = 0;
    try
    {
        frag_id = compile_shader(GL_FRAGMENT_SHADER, frag_src);
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

void Shader::set_uniform(const std::string& name, int value)
{
    glProgramUniform1i(m_program_id, get_uniform_location(name), value);
}

void Shader::set_uniform(const std::string& name, float value)
{
    glProgramUniform1f(m_program_id, get_uniform_location(name), value);
}

void Shader::set_uniform(const std::string& name, const glm::vec2& value)
{
    glProgramUniform2f(m_program_id, get_uniform_location(name), value.x, value.y);
}

void Shader::set_uniform(const std::string& name, const glm::vec3& value)
{
    glProgramUniform3f(m_program_id, get_uniform_location(name), value.x, value.y, value.z);
}

void Shader::set_uniform(const std::string& name, const glm::vec4& value)
{
    glProgramUniform4f(m_program_id, get_uniform_location(name), value.x, value.y, value.z, value.w);
}

void Shader::set_uniform(const std::string& name, const glm::mat4& value)
{
    glProgramUniformMatrix4fv(m_program_id, get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::set_texture_slot(const std::string& name, uint32_t slot)
{
    glProgramUniform1i(m_program_id, get_uniform_location(name), slot);
}

} // namespace drz
