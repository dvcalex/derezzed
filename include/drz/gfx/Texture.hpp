#pragma once

#include <string>
#include <cstdint>

namespace drz
{

class Texture
{
public:
    explicit Texture(const std::string& path);
    ~Texture();

    // no copy
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // moveable
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    void bind(uint32_t texture_slot = 0) const;
    void unbind() const;

    int getWidth() const
    {
        return m_width;
    }
    int getHeight() const
    {
        return m_height;
    }

private:
    uint32_t m_texture_handle;
    int m_width, m_height;
    int m_channels_in_file; // pixel channels
};

} // namespace drz