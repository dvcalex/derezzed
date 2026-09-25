#include "drz/gfx/Texture.hpp"
#include "drz/util/Logger.hpp"
#include <stb_image/stb_image.h>
#include <glad/gl.h>

namespace drz
{

Texture::Texture(const std::string& path) : m_texture_handle(0), m_width(0), m_height(0), m_channels_in_file(0)
{
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels_in_file, 4);

    if (!data)
        DRZ_LOGF("Failed to load texture: {}", path);

    glGenTextures(1, &m_texture_handle);
    glBindTexture(GL_TEXTURE_2D, m_texture_handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    if (data)
        stbi_image_free(data);
}

Texture::~Texture()
{
    if (m_texture_handle)
        glDeleteTextures(1, &m_texture_handle);
}

Texture::Texture(Texture&& other) noexcept
    : m_texture_handle(other.m_texture_handle), m_width(other.m_width), m_height(other.m_height), m_channels_in_file(other.m_channels_in_file)
{
    other.m_texture_handle = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        if (m_texture_handle)
            glDeleteTextures(1, &m_texture_handle);

        m_texture_handle = other.m_texture_handle;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels_in_file = other.m_channels_in_file;

        other.m_texture_handle = 0;
    }
    return *this;
}

void Texture::bind(GLuint slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_texture_handle);
}

void Texture::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace drz