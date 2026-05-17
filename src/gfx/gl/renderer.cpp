#include <cstdio>
#include <cstdlib>
#include <drz/gfx/renderer.hpp>
#include <drz/util/logger.hpp>
#include "drz/gfx/mesh_pool.hpp"
#include "frame_ring_buffer.hpp"
#include <fstream>
#include <glad/gl.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_timer.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

namespace {

const char* gl_debug_source_str(GLenum source) {
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "WINDOW";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "SHADER";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "APP";
    case GL_DEBUG_SOURCE_OTHER:
        return "OTHER";
    default:
        return "?";
    }
}

const char* gl_debug_type_str(GLenum type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "DEPRECATED";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "UB";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "PERF";
    case GL_DEBUG_TYPE_MARKER:
        return "MARKER";
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "PUSH";
    case GL_DEBUG_TYPE_POP_GROUP:
        return "POP";
    case GL_DEBUG_TYPE_OTHER:
        return "OTHER";
    default:
        return "?";
    }
}

static bool debugger_attached() {
    static const bool cached = [] {
        std::ifstream status("/proc/self/status");
        for (std::string line; std::getline(status, line);) {
            if (line.rfind("TracerPid:", 0) == 0) {
                int tracer = 0;
                std::sscanf(line.c_str(), "TracerPid: %d", &tracer);
                return tracer != 0;
            }
        }
        return false;
    }();
    return cached;
}

void GLAPIENTRY gl_debug_message(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei /*length*/,
                                 const GLchar* message,
                                 const void* /*userParam*/) {
    // Filter out NOTIFICATION logs
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }

    const char* sev = "?";
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        sev = "HIGH";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        sev = "MED";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        sev = "LOW";
        break;
    default:
        break;
    }

    DRZ_LOGF("[GL][{}][{}][{}] id={} {}", sev, gl_debug_source_str(source), gl_debug_type_str(type), id, message);
    DRZ_FLUSH_LOG();

#ifndef NDEBUG
    // Trap on HIGH severity errors so debugger stops on the bad GL call in the stack frame.
    // Outside of a debugger, the program will exit.
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        if (debugger_attached()) {
            __builtin_trap(); // debugger stops on the bad GL call
        } else {
            std::abort(); // SIGABRT, OS cleans up cleanly
        }
    }
#endif
}

} // anonymous namespace

namespace drz {

// api-specific window configuration
void Renderer::configure_window_attributes() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
}

SDL_WindowFlags Renderer::window_flags() {
    return SDL_WINDOW_OPENGL;
}

Renderer::Renderer(SDL_Window* window) {
    // Create OpenGL context
    context = SDL_GL_CreateContext(window);
    if (!context) {
        throw std::runtime_error(std::string("SDL_GL_CreateContext: ") + SDL_GetError());
    }
    // Load GLAD
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        throw std::runtime_error("gladLoadGL failed");
    }

    // enable debug callback if we got a debug context
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(&gl_debug_message, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    // Set initial viewport in pixel size
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    glViewport(0, 0, w, h); // set viewport to window size

    // Query ssbo alignment
    GLint ssbo_align = 0;
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &ssbo_align);

    // Build frame ring buffer
    constexpr size_t REGION_BYTES = 2 * 1024 * 1024; // 2 MB per-frame of draw data
    frame_ring_buffer = std::make_unique<FrameRingBuffer>(REGION_BYTES, static_cast<size_t>(ssbo_align));
}

Renderer::~Renderer() {
    SDL_GL_DestroyContext(context);
}

void Renderer::use_mesh_pool(MeshPool& pool) {
    mesh_pool = &pool;
}

PipelineStateId Renderer::register_state(const PipelineState& state_to_register) {
    for (size_t i = 0; i < states.size(); ++i) {
        // if state is already registered, return
        if (states[i].shader == state_to_register.shader) {
            return static_cast<PipelineStateId>(i);
        }
    }
    // add state and return
    states.push_back(state_to_register);
    return static_cast<PipelineStateId>(states.size() - 1);
}

DrawDataSlot Renderer::allocate_draw_data(uint32_t bytes, uint32_t align) {
    auto slot = frame_ring_buffer->allocate(bytes, align);
    return {slot.ptr, slot.offset, slot.bytes};
}

void Renderer::submit(SortKey key, const DrawPacket& packet) {
    // append draw packet and sort key
    sort_keys.push_back(key);                                    // add draw's sort key
    draw_indices.push_back(static_cast<uint32_t>(draws.size())); // index is at end of current draws vector
    draws.push_back(packet);                                     // now add actual packet
    ++frame_stats.submits;
}

void Renderer::flush() {
    // Sorts, then iterates while tracking state
    auto t0 = std::chrono::steady_clock::now(); // starting time

    assert(mesh_pool && "Renderer::flush called before use_mesh_pool");
    mesh_pool->bind(); // one-time vao bind for mesh pool

    // Sort the (key, draw index) pairs by key.
    // Build index permutation sorted by keys.
    perm.resize(sort_keys.size());
    std::iota(perm.begin(), perm.end(), 0u); // fill with 0,1,2,...n
    std::sort(perm.begin(), perm.end(), [&](uint32_t a, uint32_t b) { return sort_keys[a] < sort_keys[b]; });

    uint32_t cur_shader = 0;

    for (uint32_t i : perm) {
        const DrawPacket& packet = draws[draw_indices[i]];       // get draw packet in its sorted order
        const PipelineState& pipeline = states[packet.state_id]; // get render state for this draw

        // ### per-draw bind pipeline state ###
        if (pipeline.shader != cur_shader) {
            glUseProgram(pipeline.shader);
            cur_shader = pipeline.shader;
            ++frame_stats.shader_binds;
        }
        if (packet.draw_data_bytes > 0) {
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER,
                              0,
                              frame_ring_buffer->handle(),
                              packet.draw_data_offset,
                              packet.draw_data_bytes);
            ++frame_stats.ssbo_binds;
        }

        // ### draw call ###
        MeshSlice s = mesh_pool->slice(packet.mesh);
        glDrawElementsBaseVertex(GL_TRIANGLES,
                                 s.index_count,
                                 GL_UNSIGNED_INT,
                                 reinterpret_cast<void*>(s.first_index * sizeof(uint32_t)),
                                 s.base_vertex);
        ++frame_stats.draw_calls;
    }

    sort_keys.clear();
    draw_indices.clear();
    draws.clear();

    frame_ring_buffer->next_frame(); // rotate to next frame's region and reset it

    auto t1 = std::chrono::steady_clock::now(); // ending time
    frame_stats.cpu_flush_ms += std::chrono::duration<float, std::milli>(t1 - t0).count();
}

void Renderer::set_viewport(int width, int height) {
    glViewport(0, 0, width, height);
}

void Renderer::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
} // namespace drz
