#pragma once

#include <cstdint>

namespace drz
{
class RenderTarget
{
public:
    RenderTarget(int width, int height, bool depth);
    ~RenderTarget();

    // no copy
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    // moveable
    RenderTarget(RenderTarget&& other) noexcept;
    RenderTarget& operator=(RenderTarget&& other) noexcept;

    void bind() const;
    static void unbind();

    constexpr uint32_t framebuffer_handle() const
    {
        return m_fbo_handle;
    }
    constexpr uint32_t color_tex_handle() const
    {
        return m_color_tex_handle;
    }
    constexpr uint32_t depth_tex_handle() const
    {
        return m_depth_tex_handle;
    }
    constexpr int width() const
    {
        return m_width;
    }
    constexpr int height() const
    {
        return m_height;
    }

private:
    uint32_t m_fbo_handle = 0;       // frame buffer object handle
    uint32_t m_color_tex_handle = 0; // color texture handle
    uint32_t m_depth_tex_handle = 0; // depth texture handle
    int m_width, m_height;
};
} // namespace drz
