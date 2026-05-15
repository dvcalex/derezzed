#pragma once

#include <SDL3/SDL_video.h>
#include <vector>
#include <cstdint>

namespace drz {

struct FrameStats {
    uint32_t submits = 0;    // total draw commands submitted this frame
    uint32_t draw_calls = 0; // actual glDraws issues
    uint32_t shader_binds = 0;
    uint32_t vertex_layout_binds = 0;
    float cpu_flush_ms = 0.0f; // elapsed time inside flush() this frame
};

using PipelineStateId = uint32_t;
using SortKey = uint64_t;

struct PipelineState {
    uint32_t shader = 0;
    uint32_t vertex_layout = 0;
    // TODO: blend, depth test, cull, stencil, scissor
};

struct DrawPacket {
    PipelineStateId state_id = 0;
    // uint32_t vertex_count = 0;
    uint32_t index_count = 0;
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

    PipelineStateId register_state(const PipelineState& state);
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

    std::vector<PipelineState> states; // states, index is PipelineStateId

    // per-frame queues cleared in flush()
    std::vector<SortKey> sort_keys;     // hot path. 8 bytes each
    std::vector<uint32_t> perm;         // hot path. 4 bytes each. permutation of draw_indices sorted by sort_keys
    std::vector<uint32_t> draw_indices; // index into draws
    std::vector<DrawPacket> draws;      // cold path. actual draw data
};
} // namespace drz
