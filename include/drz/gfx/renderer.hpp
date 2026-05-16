#pragma once

#include <drz/gfx/mesh_pool.hpp>
#include <SDL3/SDL_video.h>
#include <vector>
#include <cstdint>
#include <utility>
#include <cstring>
#include <memory>

namespace drz {

struct FrameStats {
    uint32_t submits = 0;    // total draw commands submitted this frame
    uint32_t draw_calls = 0; // actual glDraws issues
    uint32_t shader_binds = 0;
    uint32_t ssbo_binds = 0;
    float cpu_flush_ms = 0.0f; // elapsed time inside flush() this frame
};

class FrameRingBuffer; // forward-decl. full type lives in src/gfx/gl/

struct DrawDataSlot {
    void* ptr;
    uint32_t offset;
    uint32_t bytes;
};

using PipelineStateId = uint32_t;
using SortKey = uint64_t;

struct PipelineState {
    uint32_t shader = 0;
    // TODO: blend, depth test, cull, stencil, scissor
};

struct DrawPacket {
    PipelineStateId state_id = 0;
    MeshHandle mesh;
    uint32_t draw_data_offset = 0; // byte offset into per-frame SSBO ring buffer
    uint32_t draw_data_bytes = 0;  // 0 == this draw doesn't use per-draw data
    // TODO: instance count, base vertex, base instance, ssbo offset
};

inline SortKey gen_sort_key(uint8_t pass, PipelineStateId state, uint32_t depth = 0) {
    return (uint64_t(pass) << 56) | ((uint64_t(state) & 0xFFFFFFull) << 32) | uint64_t(depth);
}

class Renderer {
public:
    // For implementing graphics api-specific setup called by Engine before window creation.
    // ex. Lets the renderer configure SDL/GL attributes in backend.
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
    template <typename T> std::pair<uint32_t, uint32_t> push_draw_data(const T& data) {
        auto slot = allocate_draw_data(sizeof(T), alignof(T));
        std::memcpy(slot.ptr, &data, sizeof(T));
        return {slot.offset, slot.bytes};
    }
    void submit(SortKey key, const DrawPacket& packet);
    void flush();                             // Flush current draw commands buffer and make draw calls
    void set_viewport(int width, int height); // called by Engine on window resize
    void clear(float r, float g, float b, float a);
    void reset_frame_stats() {
        frame_stats = {};
    }
    const FrameStats& last_frame_stats() const {
        return frame_stats;
    }

private:
    SDL_GLContext context = nullptr;
    FrameStats frame_stats;

    MeshPool* mesh_pool = nullptr;

    std::vector<PipelineState> states; // states, index is PipelineStateId

    std::unique_ptr<FrameRingBuffer> frame_ring_buffer; // manages per-frame SSBO for draw data

    // per-frame queues cleared in flush()
    std::vector<SortKey> sort_keys;     // hot path. 8 bytes each
    std::vector<uint32_t> perm;         // hot path. 4 bytes each. permutation of draw_indices sorted by sort_keys
    std::vector<uint32_t> draw_indices; // index into draws
    std::vector<DrawPacket> draws;      // cold path. actual draw data
};
} // namespace drz
