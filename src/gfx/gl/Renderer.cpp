#include <cstdio>
#include <cstdlib>
#include "drz/gfx/Renderer.hpp"
#include "drz/gfx/MeshPool.hpp"
#include "drz/gfx/IndirectRingBuffer.hpp"
#include "drz/util/Logger.hpp"
#include "FrameRingBuffer.hpp"
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

namespace
{

const char* gl_debug_source_str(GLenum source)
{
    switch (source)
    {
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

const char* gl_debug_type_str(GLenum type)
{
    switch (type)
    {
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

static bool debugger_attached()
{
    static const bool cached = []
    {
        std::ifstream status("/proc/self/status");
        for (std::string line; std::getline(status, line);)
        {
            if (line.rfind("TracerPid:", 0) == 0)
            {
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
                                 const void* /*userParam*/)
{
    // Filter out NOTIFICATION logs
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        return;
    }

    const char* sev = "?";
    switch (severity)
    {
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
    if (severity == GL_DEBUG_SEVERITY_HIGH)
    {
        if (debugger_attached())
        {
            __builtin_trap(); // debugger stops on the bad GL call
        }
        else
        {
            std::abort(); // SIGABRT, OS cleans up cleanly
        }
    }
#endif
}

struct DrawElementsIndirectCommand
{
    uint32_t count;
    uint32_t instance_count;
    uint32_t first_index;
    int32_t base_vertex;
    uint32_t base_instance;
};
static_assert(sizeof(DrawElementsIndirectCommand) == 20); // guard against accidental padding

} // anonymous namespace

namespace drz
{

// api-specific window configuration
void Renderer::configure_window_attributes()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
}

SDL_WindowFlags Renderer::window_flags()
{
    return SDL_WINDOW_OPENGL;
}

Renderer::Renderer(SDL_Window* window)
{
    // Create OpenGL context
    m_context = SDL_GL_CreateContext(window);
    if (!m_context)
    {
        throw std::runtime_error(std::string("SDL_GL_CreateContext: ") + SDL_GetError());
    }
    // Load GLAD
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
    {
        throw std::runtime_error("gladLoadGL failed");
    }

    // enable debug callback if we got a debug context
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
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
    m_frame_ring_buffer = std::make_unique<FrameRingBuffer>(REGION_BYTES, static_cast<size_t>(ssbo_align));

    // Build indirect command ring buffer
    constexpr size_t INDIRECT_BYTES_PER_FRAME = 256 * 1024;
    m_indirect_ring_buffer = std::make_unique<IndirectRingBuffer>(INDIRECT_BYTES_PER_FRAME);
}

Renderer::~Renderer()
{
    SDL_GL_DestroyContext(m_context);
}

void Renderer::use_mesh_pool(MeshPool& pool)
{
    m_mesh_pool = &pool;
}

PipelineStateId Renderer::register_state(const PipelineState& state_to_register)
{
    for (size_t i = 0; i < m_states.size(); ++i)
    {
        // if state is already registered, return
        if (m_states[i].shader == state_to_register.shader)
        {
            return static_cast<PipelineStateId>(i);
        }
    }
    // add state and return
    m_states.push_back(state_to_register);
    return static_cast<PipelineStateId>(m_states.size() - 1);
}

DrawDataSlot Renderer::allocate_draw_data(uint32_t bytes, uint32_t align)
{
    auto slot = m_frame_ring_buffer->allocate(bytes, align);
    return {slot.ptr, slot.offset, slot.bytes};
}

void Renderer::submit(SortKey key, const DrawPacket& packet)
{
    // append draw packet and sort key
    m_sort_keys.push_back(key);                                      // add draw's sort key
    m_draw_indices.push_back(static_cast<uint32_t>(m_draws.size())); // index is at end of current draws vector
    m_draws.push_back(packet);                                       // now add actual packet
    ++m_frame_stats.submits;
}

void Renderer::flush()
{
    auto t0 = std::chrono::steady_clock::now(); // starting time

    assert(m_mesh_pool && "Renderer::flush called before use_mesh_pool");
    m_mesh_pool->bind();            // one-time vao bind for mesh pool
    m_indirect_ring_buffer->bind(); // one-time bind for indirect draw commands buffer

    // Bind whole ssbo once for the frame
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_frame_ring_buffer->handle());
    ++m_frame_stats.ssbo_binds;

    // Sort the (key, draw index) pairs by key.
    // Build index permutation sorted by keys.
    m_perm.resize(m_sort_keys.size());
    std::iota(m_perm.begin(), m_perm.end(), 0u); // fill with 0,1,2, ... n
    std::sort(m_perm.begin(), m_perm.end(), [&](uint32_t a, uint32_t b) { return m_sort_keys[a] < m_sort_keys[b]; });

    uint32_t cur_shader = 0;
    const size_t N = m_perm.size();

    // Iterate over runs of identical state_id, one multidraw per run.
    for (uint32_t run_start = 0; run_start < N;)
    {
        const DrawPacket& head_packt = m_draws[m_draw_indices[m_perm[run_start]]];
        const PipelineStateId pipeline_state = head_packt.state_id;

        size_t run_end = run_start + 1;
        while (run_end < N && m_draws[m_draw_indices[m_perm[run_end]]].state_id == pipeline_state)
        {
            ++run_end;
        }
        const size_t run_count = run_end - run_start;

        const PipelineState& pipeline = m_states[pipeline_state];
        if (pipeline.shader != cur_shader)
        {
            glUseProgram(pipeline.shader);
            cur_shader = pipeline.shader;
            ++m_frame_stats.shader_binds;
        }

        auto alloc =
            m_indirect_ring_buffer->allocate(static_cast<uint32_t>(run_count * sizeof(DrawElementsIndirectCommand)));
        auto* commands_ptr = static_cast<DrawElementsIndirectCommand*>(alloc.ptr);

        for (size_t i = 0; i < run_count; ++i)
        {
            const DrawPacket& packet = m_draws[m_draw_indices[m_perm[run_start + i]]];
            const MeshSlice mesh = m_mesh_pool->slice(packet.mesh);

            uint32_t base_instance = 0;
            if (packet.draw_data_bytes > 0)
            {
                base_instance = packet.draw_data_offset / packet.draw_data_bytes;
            }

            commands_ptr[i] = DrawElementsIndirectCommand {
                .count = mesh.index_count,
                .instance_count = 1,
                .first_index = mesh.first_index,
                .base_vertex = static_cast<int32_t>(mesh.base_vertex),
                .base_instance = base_instance,
            };
        }

        glMultiDrawElementsIndirect(GL_TRIANGLES,
                                    GL_UNSIGNED_INT,
                                    reinterpret_cast<const void*>(static_cast<uintptr_t>(alloc.byte_offset)),
                                    static_cast<GLsizei>(run_count),
                                    0);
        ++m_frame_stats.draw_calls;

        run_start = run_end;
    }

    m_sort_keys.clear();
    m_draw_indices.clear();
    m_draws.clear();

    // rotate to next frame's region/frame and reset it
    m_frame_ring_buffer->next_frame();
    m_indirect_ring_buffer->next_frame();

    auto t1 = std::chrono::steady_clock::now(); // ending time
    m_frame_stats.cpu_flush_ms += std::chrono::duration<float, std::milli>(t1 - t0).count();
}

void Renderer::set_viewport(int width, int height)
{
    glViewport(0, 0, width, height);
}

void Renderer::clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
} // namespace drz
