#pragma once

#include "drz/util/bump_allocator.hpp"
#include <glad/gl.h>
#include <array>
#include <cstdint>

namespace drz
{

// Buffer that manages the "ring" of N frames for rendering
class FrameRingBuffer
{
public:
    struct Slot
    {
        std::byte* ptr;  // CPU pointer to write into
        uint32_t offset; // global byte offset into the SSBO
        uint32_t bytes;  // size of the slot
    };

    FrameRingBuffer(size_t per_region_bytes, size_t alignment);
    ~FrameRingBuffer();

    // Delete copy
    FrameRingBuffer(const FrameRingBuffer&) = delete;
    FrameRingBuffer& operator=(const FrameRingBuffer&) = delete;

    Slot Allocate(size_t bytes, size_t align);
    void NextFrame(); // rotates to next region, resets it
    GLuint Handle() const
    {
        return m_ssbo;
    }

private:
    static constexpr size_t REGIONS = 3;
    GLuint m_ssbo = 0;
    size_t m_ssbo_align = 0;
    std::byte* m_mapped = nullptr; // persistently mapped pointer to entire buffer
    size_t m_region_bytes;
    size_t m_current = 0;
    std::array<BumpAllocator, REGIONS> m_regions;
};
} // namespace drz
