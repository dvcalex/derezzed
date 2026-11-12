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
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_handle);

    // handle and attach color texture to fbo
    // gen and bind texture slot
    glGenTextures(1, &m_color_tex_handle);
    glBindTexture(GL_TEXTURE_2D, m_color_tex_handle);

    // set texture params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // reserve space in gpu memory for our color texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // attatch color component to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color_tex_handle, 0);

    // handle and attach depth to fbo
    if (depth)
    {
        // gen and bind texture slot
        glGenTextures(1, &m_depth_tex_handle);
        glBindTexture(GL_TEXTURE_2D, m_depth_tex_handle);

        // set texture params
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // reserve space in gpu memory for our depth texture
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        // attatch depth component to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth_tex_handle, 0);

        glBindTexture(GL_TEXTURE_2D, 0); // undbind depth
    }

    glBindTexture(GL_TEXTURE_2D, 0); // unbind color

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        DRZ_LOGF("Framebuffer incomplete with status {}", static_cast<uint32_t>(status));

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind fbo
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

void RenderTarget::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_handle);
}

void RenderTarget::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
} // namespace drz
