#include "drz/gfx/Renderer.hpp"

#include "drz/gfx/MeshPool.hpp"
#include "drz/gfx/IndirectRingBuffer.hpp"
#include "drz/util/Logger.hpp"
#include "drz/util/DebugBreak.hpp"
#include "drz/gfx/FrameRingBuffer.hpp"

#include <glad/gl.h> // must precede <SDL3/SDL_opengl.h>

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

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
    // Trap on HIGH severity errors so debugger stops on the GL call in the stack frame
    // Outside of the debugger, the program will just exit
    if (severity == GL_DEBUG_SEVERITY_HIGH)
    {
        if (debugger_attached())
        {
            DEBUG_BREAK();
        }
        else
        {
            std::abort();
        }
    }
#endif
}

struct DrawElementsIndirectCommand
{
    uint32_t count;          // indices count
    uint32_t instance_count; // instances of this command to repeat
    uint32_t first_index;    // where in the ibo to start
    int32_t base_vertex;     // added to index values before vbo lookup
    uint32_t base_instance;
};
static_assert(sizeof(DrawElementsIndirectCommand) == 20); // guard against accidental padding in the struct

GLenum blend_factor(drz::BlendSrcFactor f)
{
    switch (f)
    {
    case drz::BlendSrcFactor::One:
        return GL_ONE;
    case drz::BlendSrcFactor::Zero:
        return GL_ZERO;
    case drz::BlendSrcFactor::SrcAlpha:
        return GL_SRC1_ALPHA;
    case drz::BlendSrcFactor::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    default:
        assert(false);
        std::abort();
        return 0;
    }
}

GLenum blend_factor(drz::BlendDstFactor f)
{
    switch (f)
    {
    case drz::BlendDstFactor::One:
        return GL_ONE;
    case drz::BlendDstFactor::Zero:
        return GL_ZERO;
    case drz::BlendDstFactor::SrcAlpha:
        return GL_SRC_ALPHA;
    case drz::BlendDstFactor::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    default:
        assert(false);
        std::abort();
        return 0;
    }
}

GLenum blend_equation(drz::BlendEquation e)
{
    switch (e)
    {
    case drz::BlendEquation::Add:
        return GL_FUNC_ADD;
    case drz::BlendEquation::Subtract:
        return GL_FUNC_SUBTRACT;
    default:
        assert(false);
        std::abort();
        return 0;
    }
}

} // anonymous namespace

namespace drz
{

// api-specific window configuration
void Renderer::configure_window_attributes()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
}

SDL_WindowFlags Renderer::window_flags()
{
    return SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
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
    DRZ_LOGF("ssbo align={}", ssbo_align);
    DRZ_FLUSH_LOG();

    // Build frame ring buffer
    constexpr size_t REGION_BYTES = 128 * 1024 * 1024;
    m_frame_ring_buffer = std::make_unique<FrameRingBuffer>(REGION_BYTES, static_cast<size_t>(ssbo_align));

    // Build indirect command ring buffer
    constexpr size_t INDIRECT_BYTES_PER_FRAME = 16 * 1024 * 1024;
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
        // if state is already registered, dont add it since we only want unique pipeline states
        if (m_states[i] == state_to_register)
        {
            return static_cast<PipelineStateId>(i);
        }
    }
    // add state and return
    m_states.push_back(state_to_register);
    return static_cast<PipelineStateId>(m_states.size() - 1);
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

    // bind whole ssbo once for the frame
    m_frame_ring_buffer->bind();
    ++m_frame_stats.ssbo_binds;

    // make space in the "sort-key index" permutation array
    m_perm.resize(m_sort_keys.size());

    // fill with 0,1,2, ... n
    std::iota(m_perm.begin(), m_perm.end(), 0u);

    // sort indices based on their keys
    std::sort(m_perm.begin(), m_perm.end(), [&](uint32_t a, uint32_t b) { return m_sort_keys[a] < m_sort_keys[b]; });

    // loop through draw commands
    const size_t N = m_perm.size();
    for (uint32_t run_start = 0; run_start < N;)
    {
        // get draw data ref and id for this draw command
        const DrawPacket& head_packt = m_draws[m_draw_indices[m_perm[run_start]]];
        const PipelineStateId head_state_id = head_packt.state_id;

        // get range of draw commands that have the same pipeline state as our current one
        size_t run_end = run_start + 1;
        while (run_end < N && m_draws[m_draw_indices[m_perm[run_end]]].state_id == head_state_id)
        {
            ++run_end;
        }
        const size_t run_count = run_end - run_start; // range of draw commands with the same state

        const PipelineState& cur_pipeline = m_states[head_state_id];

        if (!m_has_applied_state || m_last_applied_state != cur_pipeline) // TODO: do we need this check?
        {
            apply_pipeline_state(cur_pipeline); // make api calls to change some type of gpu state
            m_last_applied_state = cur_pipeline;
            m_has_applied_state = true;
            ++m_frame_stats.state_changes;
        }

        // Allocate space for our draw commands (DrawElementsIndirectCommand)
        IndirectRingBuffer::Allocation alloc =
            m_indirect_ring_buffer->allocate(static_cast<uint32_t>(run_count * sizeof(DrawElementsIndirectCommand)));

        // get ptr to draw command
        DrawElementsIndirectCommand* commands_ptr = static_cast<DrawElementsIndirectCommand*>(alloc.ptr);

        // loop through this set of draw commands
        for (size_t i = 0; i < run_count; ++i)
        {
            // draw command packet for this draw
            const DrawPacket& packet = m_draws[m_draw_indices[m_perm[run_start + i]]];

            // get mesh out of mesh pool by id
            const MeshSlice mesh = m_mesh_pool->slice(packet.mesh);

            // populate the draw commands allocation we made earlier
            commands_ptr[i] = DrawElementsIndirectCommand {
                .count = mesh.index_count,
                .instance_count = 1,
                .first_index = mesh.first_index,
                .base_vertex = static_cast<int32_t>(mesh.base_vertex),
                .base_instance = packet.base_instance,
            };
        }

        // indirect multi draw opengl call
        glMultiDrawElementsIndirect(GL_TRIANGLES,
                                    GL_UNSIGNED_INT,
                                    reinterpret_cast<const void*>(static_cast<uintptr_t>(
                                        alloc.byte_offset)), // expects a void ptr; TODO: is there a better way to do
                                                             // this? Integer to pointer cast pessimizes optimization
                                                             // opportunities (clang-tidy performance-no-int-to-ptr)
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

void Renderer::apply_pipeline_state(const PipelineState& state) const
{
    // TODO: look into seeing if its worth it to apply state per-field diffs rather always reapplying entire state

    glUseProgram(state.shader);

    if (state.enable_blend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    glBlendEquation(blend_equation(state.blend_equation));

    glBlendFunc(blend_factor(state.blend_src_factor), blend_factor(state.blend_dst_factor));

    if (state.enable_depth_test)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (state.enable_depth_write)
        glDepthMask(GL_TRUE);
    else
        glDepthMask(GL_FALSE);

    if (state.enable_cull_face)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    if (state.cull_mode == drz::CullMode::Back)
        glCullFace(GL_BACK);
    else
        glCullFace(GL_FRONT);

    // TODO
    if (state.enable_stencil_test)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);

    if (state.enable_scissor_test)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);

    glScissor(state.scissor_x, state.scissor_y, state.scissor_width, state.scissor_height);
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
