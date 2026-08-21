#pragma once

#include "drz/util/BumpAllocator.hpp"
#include <glad/gl.h>
#include <array>
#include <cstdint>

namespace drz
{

/**
 * @brief Per draw data allocator for GPU consumption
 */
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

    FrameRingBuffer(FrameRingBuffer&&) = default;
    FrameRingBuffer& operator=(FrameRingBuffer&&) = default;

    /**
     * @brief
     *
     * @param bytes
     * @param align
     * @return Slot
     */
    Slot allocate(size_t bytes, size_t align);

    void next_frame(); // rotates to next region, resets it

    GLuint handle() const
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
