#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace drz
{

struct MeshHandle
{
    uint32_t id = 0; // 0 is invalid
};

struct MeshDesc
{
    std::span<const float> positions; // 3 floats per vert
    std::span<const float> normals;   // 3 floats per vert
    std::span<const float> uvs;       // 2 floats per vert
    std::span<const uint32_t> indices;
};

struct MeshSlice
{
    uint32_t first_index;
    uint32_t index_count;
    uint32_t base_vertex;
};

// One shared vao, vbo, and ibo holding many meshed packed together
// All meshes share the format: pos(3) + normal(3) + uv(2)
class MeshPool
{
public:
    MeshPool(size_t max_vertices, size_t max_indices);
    ~MeshPool();

    MeshPool(const MeshPool&) = delete;
    MeshPool& operator=(const MeshPool&) = delete;
    MeshPool(MeshPool&&) = delete;
    MeshPool& operator=(MeshPool&&) = delete;

    MeshHandle Upload(const MeshDesc& desc);
    MeshSlice Slice(MeshHandle h) const;
    void Bind(); // binds the pool's vertex layout / vao

private:
    uint32_t m_vertex_layout_handle = 0;
    uint32_t m_vertex_buffer_handle = 0;
    uint32_t m_index_buffer_handle = 0;

    size_t m_vertex_capacity = 0;
    size_t m_index_capacity = 0;
    size_t m_vertex_head = 0;
    size_t m_index_head = 0;

    std::vector<MeshSlice> m_slices;
};

} // namespace drz
