#include <derezzed/mesh.hpp>

#include <glad/gl.h>

#include <stdexcept>

namespace drz {
Mesh::Mesh(std::span<const glm::vec3> positions,
           std::span<const glm::vec3> normals,
           std::span<const glm::vec2> uvs,
           std::span<const uint32_t> indices) {
    // Validate positions exists and save size
    if (positions.empty()) {
        throw std::runtime_error("Mesh requires POSITIONS! Currently empty!");
    }
    m_vertex_count = positions.size();

    // Check if other vertex attribs are either empty or match position's size
    if (!normals.empty() && normals.size() != m_vertex_count) {
        throw std::runtime_error("NORMALS buffer size does not match POSITIONS buffer size! Vertex buffers must either be empty or "
                                 "match positions buffer.");
    }
    if (!uvs.empty() && uvs.size() != m_vertex_count) {
        throw std::runtime_error(
            "UVs buffer size does not match POSITIONS buffer size! Vertex buffers must either be empty or match positions buffer.");
    }

    // Generate vao and bind it so we capture layout
    glGenVertexArrays(1, &vertex_layout);
    glBindVertexArray(vertex_layout);

    // ### Upload each buffer to GPU ###

    // Positions
    glGenBuffers(1, &position_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
    glBufferData(GL_ARRAY_BUFFER, positions.size_bytes(), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    // Upload normals, skip if empty
    if (!normals.empty()) {
        glGenBuffers(1, &normal_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
        glBufferData(GL_ARRAY_BUFFER, normals.size_bytes(), normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);
    }

    // Upload UVs, skip if empty
    if (!uvs.empty()) {
        glGenBuffers(1, &uv_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
        glBufferData(GL_ARRAY_BUFFER, uvs.size_bytes(), uvs.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(2);
    }

    // Upload indices, skip if empty
    if (!indices.empty()) {
        m_index_count = indices.size(); // save off indices size
        glGenBuffers(1, &index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), GL_STATIC_DRAW);
    }

    glBindVertexArray(0); // unbind vao
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &vertex_layout);
    glDeleteBuffers(1, &position_buffer);
    glDeleteBuffers(1, &normal_buffer);
    glDeleteBuffers(1, &uv_buffer);
    glDeleteBuffers(1, &index_buffer);
}

Mesh::Mesh(Mesh&& other) noexcept
    : vertex_layout(other.vertex_layout), position_buffer(other.position_buffer), normal_buffer(other.normal_buffer),
      uv_buffer(other.uv_buffer), index_buffer(other.index_buffer), m_vertex_count(other.m_vertex_count),
      m_index_count(other.m_index_count) {
    other.vertex_layout = 0;
    other.position_buffer = 0;
    other.normal_buffer = 0;
    other.uv_buffer = 0;
    other.index_buffer = 0;
    other.m_vertex_count = 0;
    other.m_index_count = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        // Release all stuff that this currently has
        glDeleteVertexArrays(1, &vertex_layout);
        glDeleteBuffers(1, &position_buffer);
        glDeleteBuffers(1, &normal_buffer);
        glDeleteBuffers(1, &uv_buffer);
        glDeleteBuffers(1, &index_buffer);

        vertex_layout = other.vertex_layout;
        position_buffer = other.position_buffer;
        normal_buffer = other.normal_buffer;
        uv_buffer = other.uv_buffer;
        index_buffer = other.index_buffer;
        m_vertex_count = other.m_vertex_count;
        m_index_count = other.m_index_count;

        other.vertex_layout = 0;
        other.position_buffer = 0;
        other.normal_buffer = 0;
        other.uv_buffer = 0;
        other.index_buffer = 0;
        other.m_vertex_count = 0;
        other.m_index_count = 0;
    }
    return *this;
}

} // namespace drz
