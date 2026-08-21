#include "FrameRingBuffer.hpp"

#include "drz/util/BumpAllocator.hpp"

#include <glad/gl.h>
#include <stdexcept>

namespace drz
{

FrameRingBuffer::FrameRingBuffer(size_t per_region_bytes, size_t alignment)
{
    m_ssbo_align = alignment;
    m_region_bytes = per_region_bytes;
    if (m_region_bytes % m_ssbo_align != 0)
    {
        throw std::runtime_error("FrameRingBuffer region size must be multiple of alignment");
    }
    size_t total = m_region_bytes * REGIONS;

    glCreateBuffers(1, &m_ssbo);
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glNamedBufferStorage(m_ssbo, total, nullptr, flags);
    m_mapped = static_cast<std::byte*>(glMapNamedBufferRange(m_ssbo, 0, total, flags));
    if (!m_mapped)
    {
        throw std::runtime_error("SSBO map failed");
    }

    for (size_t i = 0; i < REGIONS; ++i)
    {
        m_regions[i] = BumpAllocator(m_mapped + i * m_region_bytes, m_region_bytes);
    }
}

FrameRingBuffer::~FrameRingBuffer()
{
    if (m_ssbo)
    {
        glUnmapNamedBuffer(m_ssbo);
        glDeleteBuffers(1, &m_ssbo);
    }
}

FrameRingBuffer::Slot FrameRingBuffer::allocate(size_t bytes, size_t align)
{
    size_t effective_align = std::max(align, m_ssbo_align);
    std::byte* ptr = m_regions[m_current].allocate(bytes, effective_align);
    if (!ptr)
    {
        throw std::runtime_error("FrameRingBuffer region overflow");
    }
    uint32_t offset = static_cast<uint32_t>(ptr - m_mapped);
    return {ptr, offset, static_cast<uint32_t>(bytes)};
}

void FrameRingBuffer::next_frame()
{
    m_current = (m_current + 1) % REGIONS;
    m_regions[m_current].reset();
}

} // namespace drz
