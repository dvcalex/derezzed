#include "glad/gl.h"
#include <cassert>
#include <cstdint>
#include <drz/gfx/indirect_ring_buffer.hpp>
#include <stdexcept>

namespace drz {

IndirectRingBuffer::IndirectRingBuffer(uint32_t bytes_per_frame) {
    this->bytes_per_frame = bytes_per_frame;

    glCreateBuffers(1, &buffer_handle);
    glNamedBufferStorage(buffer_handle,
                         bytes_per_frame * NUM_FRAMES,
                         nullptr,
                         GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    mapped =
        static_cast<uint8_t*>(glMapNamedBufferRange(buffer_handle,
                                                    0,
                                                    bytes_per_frame * NUM_FRAMES,
                                                    GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));
    if (!mapped) {
        throw std::runtime_error("Indirect buffer map failed");
    }
}

IndirectRingBuffer::~IndirectRingBuffer() {
    if (buffer_handle) {
        glUnmapNamedBuffer(buffer_handle);
        glDeleteBuffers(1, &buffer_handle);
    }
}

void IndirectRingBuffer::next_frame() {
    cur_frame = (cur_frame + 1) % NUM_FRAMES;
    cur_frame_offset = 0; // reset bump cursor
}

IndirectRingBuffer::Allocation IndirectRingBuffer::allocate(uint32_t bytes) {
    assert(cur_frame_offset + bytes <= bytes_per_frame && "indirect ring overflow");
    uint32_t frame_base = cur_frame * bytes_per_frame;
    Allocation a {
        .ptr = mapped + frame_base + cur_frame_offset,
        .byte_offset = frame_base + cur_frame_offset,
    };
    cur_frame_offset += bytes;
    return a;
}

uint32_t IndirectRingBuffer::handle() const {
    return buffer_handle;
}

void IndirectRingBuffer::bind() const {
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer_handle);
}

} // namespace drz
