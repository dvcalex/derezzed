#pragma once

#include <glm/glm.hpp>

#include <span>
#include <cstdint>

namespace drz {

class Mesh {
public:
    // # of vertices is assumed to be positions.size()
    // Takes in a view of buffers and then does init / GPU allocations
    Mesh(std::span<const glm::vec3> positions,
         std::span<const glm::vec3> normals = {},
         std::span<const glm::vec2> uvs = {},
         std::span<const uint32_t> indices = {});
    // TODO: Constructor overload for 3d object files
    ~Mesh();

    // Delete copy (can't just copy gl objects normally)
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Define move in source file
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    uint32_t vertex_layout_handle() const {
        return vertex_layout;
    }
    uint32_t position_buffer_handle() const {
        return position_buffer;
    }
    uint32_t normal_buffer_handle() const {
        return normal_buffer;
    }
    uint32_t uv_buffer_handle() const {
        return uv_buffer;
    }
    uint32_t index_buffer_handle() const {
        return index_buffer;
    }
    uint32_t vertex_count() const {
        return m_vertex_count;
    }
    uint32_t index_count() const {
        return m_index_count;
    }

private:
    uint32_t vertex_layout = 0;
    uint32_t position_buffer = 0;
    uint32_t normal_buffer = 0;
    uint32_t uv_buffer = 0;
    uint32_t index_buffer = 0;
    uint32_t m_vertex_count = 0;
    uint32_t m_index_count = 0;
};
} // namespace drz
