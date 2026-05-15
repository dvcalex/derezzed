#pragma once

#include <cstddef>
#include <cstdint>

namespace drz {

// linear allocator that can only allocate, bump, and reset. No freeing individual allocations.
class BumpAllocator {
public:
    BumpAllocator() = default;
    BumpAllocator(std::byte* base, size_t capacity) : base(base), capacity(capacity) {}

    // Returns nullptr if request doesn't fit
    std::byte* allocate(size_t bytes, size_t align) {
        // round head up to the nearest multiple of align (idk how this works really)
        // Should we enforce align to be a power of 2?
        size_t aligned_head = (head + align - 1) & ~(align - 1);
        if (aligned_head + bytes > capacity) {
            return nullptr; // not enough space
        }
        std::byte* ptr = base + aligned_head;
        head = aligned_head + bytes;
        return ptr;
    }
    void reset() {
        head = 0;
    }
    size_t bytes_used() const {
        return head;
    }
    size_t bytes_capacity() const {
        return capacity;
    }

private:
    std::byte* base = nullptr;
    size_t capacity = 0;
    size_t head = 0;
};
} // namespace drz
