#include "drz/gfx/RenderTarget.hpp"
#include "drz/util/Logger.hpp"
#include "glad/gl.h"

#include <cassert>

namespace drz
{
RenderTarget::RenderTarget(int width, int height, bool depth) : m_width(width), m_height(height)
{
    assert(width > 0 && height > 0);

    // set up framebuffer object
    glGenFramebuffers(1, &m_fbo_handle);

    // handle and attach color texture to fbo
    // create texture object
    glCreateTextures(GL_TEXTURE_2D, 1, &m_color_tex_handle);

    // assign tex params
    glTextureParameteri(m_color_tex_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_color_tex_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_color_tex_handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_color_tex_handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // specify storage requirements and reserve space
    glTextureStorage2D(m_color_tex_handle, 1, GL_RGBA8, width, height);

    // attach to fbo
    glNamedFramebufferTexture(m_fbo_handle, GL_COLOR_ATTACHMENT0, m_color_tex_handle, 0);

    // handle and attach depth to fbo
    if (depth)
    {
        // gen and bind texture slot
        glCreateTextures(GL_TEXTURE_2D, 1, &m_depth_tex_handle);

        // set texture params
        glTextureParameteri(m_depth_tex_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_depth_tex_handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(m_depth_tex_handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_depth_tex_handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // specify storage requirements and reserve space
        glTextureStorage2D(m_depth_tex_handle, 1, GL_DEPTH_COMPONENT24, width, height);

        // attatch to fbo
        glNamedFramebufferTexture(m_fbo_handle, GL_DEPTH_ATTACHMENT, m_depth_tex_handle, 0);
    }

    GLenum status = glCheckNamedFramebufferStatus(m_fbo_handle, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        DRZ_LOGF("Framebuffer incomplete with status {}", static_cast<uint32_t>(status));
}

RenderTarget::~RenderTarget()
{
    glDeleteFramebuffers(1, &m_fbo_handle);
    glDeleteTextures(1, &m_color_tex_handle);
    glDeleteTextures(1, &m_depth_tex_handle);
}

RenderTarget::RenderTarget(RenderTarget&& other) noexcept
    : m_fbo_handle(other.m_fbo_handle), m_color_tex_handle(other.m_color_tex_handle),
      m_depth_tex_handle(other.m_depth_tex_handle), m_width(other.m_width), m_height(other.m_height)
{
    other.m_fbo_handle = 0;
    other.m_color_tex_handle = 0;
    other.m_depth_tex_handle = 0;
}

RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept
{
    if (this != &other)
    {
        if (m_fbo_handle)
            glDeleteFramebuffers(1, &m_fbo_handle);
        if (m_color_tex_handle)
            glDeleteTextures(1, &m_color_tex_handle);
        if (m_depth_tex_handle)
            glDeleteTextures(1, &m_depth_tex_handle);

        m_fbo_handle = other.m_fbo_handle;
        m_color_tex_handle = other.m_color_tex_handle;
        m_depth_tex_handle = other.m_depth_tex_handle;

        m_width = other.m_width;
        m_height = other.m_height;

        other.m_fbo_handle = 0;
        other.m_color_tex_handle = 0;
        other.m_depth_tex_handle = 0;
    }
    return *this;
}
} // namespace drz
