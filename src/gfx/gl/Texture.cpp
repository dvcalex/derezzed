#include "drz/gfx/Texture.hpp"
#include "drz/util/Logger.hpp"

#include <glad/gl.h>
#include <stb_image/stb_image.h>

#include <algorithm>
#include <cmath>

namespace drz
{

Texture::Texture(const std::string& path) : m_texture_handle(0), m_width(0), m_height(0), m_channels_in_file(0)
{
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels_in_file, 4);

    if (!data)
    {
        DRZ_LOGF("Failed to load texture: {}", path);
        return;
    }

    // create texture object
    glCreateTextures(GL_TEXTURE_2D, 1, &m_texture_handle);

    // assign tex params
    glTextureParameteri(m_texture_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_texture_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_texture_handle, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_texture_handle, GL_TEXTURE_WRAP_T, GL_REPEAT);

    int max_dimension = std::max(m_width, m_height);
    // how many times can the larger dimention half itself (log2)?
    // + 1 since we are counting mip level 0 towards the total count
    // mip levels = floor(log2(max)) + 1
    int mip_levels = static_cast<int>(std::floor(std::log2(static_cast<float>(max_dimension)))) + 1;

    // specify storage requirements and reserve space
    glTextureStorage2D(m_texture_handle, mip_levels, GL_RGBA8, m_width, m_height);

    // copy data onto that reserved texture space
    glTextureSubImage2D(m_texture_handle, 0, 0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateTextureMipmap(m_texture_handle);

    stbi_image_free(data);
}

Texture::~Texture()
{
    if (m_texture_handle)
        glDeleteTextures(1, &m_texture_handle);
}

Texture::Texture(Texture&& other) noexcept
    : m_texture_handle(other.m_texture_handle), m_width(other.m_width), m_height(other.m_height),
      m_channels_in_file(other.m_channels_in_file)
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
    glBindTextureUnit(slot, m_texture_handle);
}

} // namespace drz
