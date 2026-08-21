#pragma once

#include <cstddef>

namespace drz
{

/**
 * @brief Linear allocator that can only allocate, bump, and reset. Cannot freeing individual allocations, just entire
 * buffer.
 */
class BumpAllocator
{
public:
    constexpr BumpAllocator() noexcept = default;

    constexpr BumpAllocator(std::byte* base, size_t capacity) noexcept : m_base(base), m_capacity(capacity) {}

    /**
     * @brief Calculates new head position based on size of allocation, rounded to the next multiple (and power of 2) of
     * align.
     *
     * @param bytes Size in bytes to allocate.
     * @param align Byte alignment for the new head ptr. ASSUMES A POWER OF 2!
     * @return std::byte* Returns nullptr if request doesn't fit.
     */
    constexpr std::byte* allocate(size_t bytes, size_t align) noexcept
    {
        // round head up to the next multiple of align
        size_t aligned_head = (m_head + align - 1) & ~(align - 1);
        if (aligned_head + bytes > m_capacity)
        {
            return nullptr; // not enough space
        }
        std::byte* ptr = m_base + aligned_head;
        m_head = aligned_head + bytes;
        return ptr;
    }

    /**
     * @brief Reset
     */
    constexpr void reset() noexcept
    {
        m_head = 0;
    }

    constexpr size_t bytes_used() const noexcept
    {
        return m_head;
    }

    constexpr size_t bytes_capacity() const noexcept
    {
        return m_capacity;
    }

private:
    std::byte* m_base = nullptr;
    size_t m_capacity = 0;
    size_t m_head = 0;
};
} // namespace drz
