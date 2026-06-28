#include <glad/gl.h>
#include <cassert>
#include <cstdint>
#include "drz/gfx/indirect_ring_buffer.hpp"
#include <stdexcept>

namespace drz
{

IndirectRingBuffer::IndirectRingBuffer(uint32_t bytes_per_frame)
{
    this->m_bytes_per_frame = bytes_per_frame;

    glCreateBuffers(1, &m_buffer_handle);
    glNamedBufferStorage(m_buffer_handle,
                         bytes_per_frame * NUM_FRAMES,
                         nullptr,
                         GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    m_mapped =
        static_cast<uint8_t*>(glMapNamedBufferRange(m_buffer_handle,
                                                    0,
                                                    bytes_per_frame * NUM_FRAMES,
                                                    GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));
    if (!m_mapped)
    {
        throw std::runtime_error("Indirect buffer map failed");
    }
}

IndirectRingBuffer::~IndirectRingBuffer()
{
    if (m_buffer_handle)
    {
        glUnmapNamedBuffer(m_buffer_handle);
        glDeleteBuffers(1, &m_buffer_handle);
    }
}

void IndirectRingBuffer::NextFrame()
{
    m_cur_frame = (m_cur_frame + 1) % NUM_FRAMES;
    m_cur_frame_offset = 0; // reset bump cursor
}

IndirectRingBuffer::Allocation IndirectRingBuffer::Allocate(uint32_t bytes)
{
    assert(m_cur_frame_offset + bytes <= m_bytes_per_frame && "indirect ring overflow");
    uint32_t frame_base = m_cur_frame * m_bytes_per_frame;
    Allocation a {
        .ptr = m_mapped + frame_base + m_cur_frame_offset,
        .byte_offset = frame_base + m_cur_frame_offset,
    };
    m_cur_frame_offset += bytes;
    return a;
}

uint32_t IndirectRingBuffer::Handle() const
{
    return m_buffer_handle;
}

void IndirectRingBuffer::Bind() const
{
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_buffer_handle);
}

} // namespace drz
