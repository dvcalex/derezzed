#pragma once

#include "drz/gfx/MeshPool.hpp"
#include "drz/gfx/FrameRingBuffer.hpp"
#include "drz/gfx/IndirectRingBuffer.hpp"
#include <SDL3/SDL_video.h>
#include <vector>
#include <cstdint>
#include <utility>
#include <cstring>
#include <memory>

namespace drz
{
struct FrameStats
{
    uint32_t submits = 0;    // total draw commands submitted this frame
    uint32_t draw_calls = 0; // actual glDraws issues
    uint32_t ssbo_binds = 0;
    uint32_t state_changes = 0;
    float cpu_flush_ms = 0.0f; // elapsed time inside flush() this frame
};

struct DrawDataSlot
{
    void* ptr;
    uint32_t offset;
    uint32_t bytes;
};

using PipelineStateId = uint32_t;
using SortKey = uint64_t;

enum class BlendSrcFactor : uint8_t
{
    Zero,
    One,
    SrcAlpha,
    OneMinusSrcAlpha
};

enum class BlendDstFactor : uint8_t
{
    Zero,
    One,
    SrcAlpha,
    OneMinusSrcAlpha
};

enum class BlendEquation : uint8_t
{
    Add,
    Subtract
};

enum class CullMode : uint8_t
{
    Back,
    Front
};


struct PipelineState
{
    uint32_t shader = 0;

    // blend state
    bool enable_blend = false;
    BlendSrcFactor blend_src_factor = BlendSrcFactor::One;
    BlendDstFactor blend_dst_factor = BlendDstFactor::Zero;
    BlendEquation blend_equation = BlendEquation::Add;

    // depth buffer flags
    bool enable_depth_test = false;
    bool enable_depth_write = false;

    // face culling
    bool enable_cull_face = false;
    CullMode cull_mode = CullMode::Back;

    // stencil buffer flags
    bool enable_stencil_test = false;
    // TODO

    // scissor test
    bool enable_scissor_test = false;
    int scissor_x = 0;
    int scissor_y = 0;
    int scissor_width = 0;
    int scissor_height = 0;
};

inline bool operator==(const PipelineState& lhs, const PipelineState& rhs)
{
    return lhs.shader == rhs.shader && lhs.enable_blend == rhs.enable_blend &&
        lhs.blend_src_factor == rhs.blend_src_factor && lhs.blend_dst_factor == rhs.blend_dst_factor &&
        lhs.blend_equation == rhs.blend_equation && lhs.enable_depth_test == rhs.enable_depth_test &&
        lhs.enable_depth_write == rhs.enable_depth_write && lhs.enable_cull_face == rhs.enable_cull_face &&
        lhs.cull_mode == rhs.cull_mode && lhs.enable_stencil_test == rhs.enable_stencil_test &&
        lhs.enable_scissor_test == rhs.enable_scissor_test && lhs.scissor_x == rhs.scissor_x &&
        lhs.scissor_y == rhs.scissor_y && lhs.scissor_width == rhs.scissor_width &&
        lhs.scissor_height == rhs.scissor_height;
}

inline bool operator!=(const PipelineState& lhs, const PipelineState& rhs)
{
    return !(lhs == rhs);
}


struct DrawPacket
{
    PipelineStateId state_id = 0;
    MeshHandle mesh;
    uint32_t draw_data_offset = 0; // byte offset into per-frame SSBO ring buffer
    uint32_t draw_data_bytes = 0;  // 0 == this draw doesn't use per-draw data
    // TODO: instance count, base vertex, base instance, ssbo offset
};

inline SortKey gen_sort_key(uint8_t pass, PipelineStateId state, uint32_t depth = 0)
{
    return (uint64_t(pass) << 56) | ((uint64_t(state) & 0xFFFFFFull) << 32) | uint64_t(depth); // :(
}

class Renderer
{
public:
    /**
     * @brief For handling graphics api-specific setup called by Engine before window creation.
     *  ex. Configure SDL/GL.
     */
    static void configure_window_attributes();

    static SDL_WindowFlags window_flags();

    Renderer(SDL_Window* window);

    ~Renderer();

    // Delete copy
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void use_mesh_pool(MeshPool& pool);

    PipelineStateId register_state(const PipelineState& state);

    DrawDataSlot allocate_draw_data(uint32_t bytes, uint32_t align);

    template <typename T> std::pair<uint32_t, uint32_t> push_draw_data(const T& data)
    {
        auto slot = allocate_draw_data(sizeof(T), alignof(T));
        std::memcpy(slot.ptr, &data, sizeof(T));
        return {slot.offset, slot.bytes};
    }

    void submit(SortKey key, const DrawPacket& packet);

    /**
     * @brief Flush current draw commands buffer and make draw calls
     */
    void flush();

    void set_viewport(int width, int height); // called by Engine on window resize

    void clear(float r, float g, float b, float a);

    void reset_frame_stats()
    {
        m_frame_stats = {};
    }

    const FrameStats& last_frame_stats() const
    {
        return m_frame_stats;
    }

private:
    void apply_pipeline_state(const PipelineState& state) const;

    SDL_GLContext m_context = nullptr;
    FrameStats m_frame_stats;

    MeshPool* m_mesh_pool = nullptr;

    std::vector<PipelineState> m_states; // all pipeline states where the index is PipelineStateId
    PipelineState m_last_applied_state;
    bool m_has_applied_state = false;

    std::unique_ptr<FrameRingBuffer> m_frame_ring_buffer; // manages per-frame SSBO for draw data

    std::unique_ptr<IndirectRingBuffer> m_indirect_ring_buffer;

    // ### Per-frame buffers cleared in flush() ###

    std::vector<SortKey> m_sort_keys;     // hot path. 8 bytes each
    std::vector<uint32_t> m_perm;         // hot path. 4 bytes each. a permutation of draw_indices sorted by sort_keys
    std::vector<uint32_t> m_draw_indices; // index into draws
    std::vector<DrawPacket> m_draws;      // cold path. actual draw data
};
} // namespace drz
