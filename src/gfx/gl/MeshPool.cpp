#include <cstdint>
#include "drz/gfx/MeshPool.hpp"

#include <glad/gl.h>

#include <cassert>
#include <cstddef>
#include <vector>

namespace drz
{

namespace
{
struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};
static_assert(sizeof(Vertex) == 32);
} // namespace

MeshPool::MeshPool(size_t max_vertices, size_t max_indices)
    : m_vertex_capacity(max_vertices), m_index_capacity(max_indices)
{

    glCreateBuffers(1, &m_vertex_buffer_handle);
    glCreateBuffers(1, &m_index_buffer_handle);
    glNamedBufferStorage(m_vertex_buffer_handle, max_vertices * sizeof(Vertex), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(m_index_buffer_handle, max_indices * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateVertexArrays(1, &m_vertex_layout_handle);
    glVertexArrayVertexBuffer(m_vertex_layout_handle, 0, m_vertex_buffer_handle, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(m_vertex_layout_handle, m_index_buffer_handle);

    glEnableVertexArrayAttrib(m_vertex_layout_handle, 0);
    glVertexArrayAttribFormat(m_vertex_layout_handle, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, x));
    glVertexArrayAttribBinding(m_vertex_layout_handle, 0, 0);

    glEnableVertexArrayAttrib(m_vertex_layout_handle, 1);
    glVertexArrayAttribFormat(m_vertex_layout_handle, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, nx));
    glVertexArrayAttribBinding(m_vertex_layout_handle, 1, 0);

    glEnableVertexArrayAttrib(m_vertex_layout_handle, 2);
    glVertexArrayAttribFormat(m_vertex_layout_handle, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, u));
    glVertexArrayAttribBinding(m_vertex_layout_handle, 2, 0);
}

MeshPool::~MeshPool()
{
    glDeleteVertexArrays(1, &m_vertex_layout_handle);
    glDeleteBuffers(1, &m_vertex_buffer_handle);
    glDeleteBuffers(1, &m_index_buffer_handle);
}

MeshHandle MeshPool::upload(const MeshDesc& desc)
{
    const size_t vert_count = desc.positions.size() / 3;
    const size_t index_count = desc.indices.size();

    assert(desc.positions.size() % 3 == 0);
    assert(desc.normals.empty() || desc.normals.size() == vert_count * 3);
    assert(desc.uvs.empty() || desc.uvs.size() == vert_count * 2);

    if (m_vertex_head + vert_count > m_vertex_capacity)
    {
        return {};
    }
    if (m_index_head + index_count > m_index_capacity)
    {
        return {};
    }

    std::vector<Vertex> verts(vert_count);
    for (size_t i = 0; i < vert_count; ++i)
    {
        verts[i].x = desc.positions[i * 3 + 0];
        verts[i].y = desc.positions[i * 3 + 1];
        verts[i].z = desc.positions[i * 3 + 2];

        if (!desc.normals.empty())
        {
            verts[i].nx = desc.normals[i * 3 + 0];
            verts[i].ny = desc.normals[i * 3 + 1];
            verts[i].nz = desc.normals[i * 3 + 2];
        }
        if (!desc.uvs.empty())
        {
            verts[i].u = desc.uvs[i * 2 + 0];
            verts[i].v = desc.uvs[i * 2 + 1];
        }
    }

    glNamedBufferSubData(
        m_vertex_buffer_handle, m_vertex_head * sizeof(Vertex), verts.size() * sizeof(Vertex), verts.data());

    glNamedBufferSubData(
        m_index_buffer_handle, m_index_head * sizeof(uint32_t), desc.indices.size_bytes(), desc.indices.data());

    m_slices.push_back({
        .first_index = static_cast<uint32_t>(m_index_head),
        .index_count = static_cast<uint32_t>(index_count),
        .base_vertex = static_cast<uint32_t>(m_vertex_head),
    });

    m_vertex_head += vert_count;
    m_index_head += index_count;

    return MeshHandle {static_cast<uint32_t>(m_slices.size())};
}

MeshSlice MeshPool::slice(MeshHandle h) const
{
    return m_slices[h.id - 1];
}

void MeshPool::bind()
{
    glBindVertexArray(m_vertex_layout_handle);
}

} // namespace drz
