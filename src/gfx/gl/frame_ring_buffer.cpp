#include "frame_ring_buffer.hpp"
#include "drz/util/bump_allocator.hpp"
#include "glad/gl.h"
#include <stdexcept>

namespace drz {

FrameRingBuffer::FrameRingBuffer(size_t per_region_bytes, size_t alignment) {
    ssbo_align = alignment;
    region_bytes = per_region_bytes;
    if (region_bytes % ssbo_align != 0) {
        throw std::runtime_error("FrameRingBuffer region size must be multiple of alignment");
    }
    size_t total = region_bytes * REGIONS;

    glCreateBuffers(1, &ssbo);
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glNamedBufferStorage(ssbo, total, nullptr, flags);
    mapped = static_cast<std::byte*>(glMapNamedBufferRange(ssbo, 0, total, flags));
    if (!mapped) {
        throw std::runtime_error("SSBO map failed");
    }

    for (size_t i = 0; i < REGIONS; ++i) {
        regions[i] = BumpAllocator(mapped + i * region_bytes, region_bytes);
    }
}

FrameRingBuffer::~FrameRingBuffer() {
    if (ssbo) {
        glUnmapNamedBuffer(ssbo);
        glDeleteBuffers(1, &ssbo);
    }
}

FrameRingBuffer::Slot FrameRingBuffer::allocate(size_t bytes, size_t align) {
    size_t effective_align = std::max(align, ssbo_align);
    std::byte* ptr = regions[current].allocate(bytes, effective_align);
    if (!ptr) {
        throw std::runtime_error("FrameRingBuffer region overflow");
    }
    uint32_t offset = static_cast<uint32_t>(ptr - mapped);
    return {ptr, offset, static_cast<uint32_t>(bytes)};
}

void FrameRingBuffer::next_frame() {
    current = (current + 1) % REGIONS;
    regions[current].reset();
}

} // namespace drz
