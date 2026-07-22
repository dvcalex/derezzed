#pragma once

#include <cstdint>

namespace drz
{

class IndirectRingBuffer
{
public:
    explicit IndirectRingBuffer(uint32_t bytes_per_frame);
    ~IndirectRingBuffer();
    IndirectRingBuffer(const IndirectRingBuffer&) = delete;
    IndirectRingBuffer& operator=(const IndirectRingBuffer&) = delete;
    IndirectRingBuffer(IndirectRingBuffer&&) = delete;
    IndirectRingBuffer& operator=(IndirectRingBuffer&&) = delete;

    void next_frame(); // advance to next sub-region, reset bump cursor

    struct Allocation
    {
        void* ptr; // CPU writable
        uint32_t byte_offset;
    };
    Allocation allocate(uint32_t bytes);

    uint32_t handle() const;
    void bind() const;

private:
    static constexpr uint32_t NUM_FRAMES = 3; // triple buffered
    uint32_t m_buffer_handle = 0;
    uint8_t* m_mapped = nullptr;
    uint32_t m_bytes_per_frame = 0;
    uint32_t m_cur_frame = 0;
    uint32_t m_cur_frame_offset = 0;
};
} // namespace drz
