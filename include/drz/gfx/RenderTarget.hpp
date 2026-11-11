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

    // get color texture handle
    uint32_t color_texture() const;
    // get depth texture handle
    uint32_t depth_texture() const;
    // get width
    int width() const;
    // get height
    int height() const;

private:
    uint32_t m_fbo_handle = 0;       // frame buffer object handle
    uint32_t m_color_tex_handle = 0; // color texture handle
    uint32_t m_depth_tex_handle = 0; // depth texture handle
    int m_width, m_height;
};
} // namespace drz
