#include "drz/gfx/ModelLoader.hpp"
#include "drz/util/Logger.hpp"

#include <tinygltf/tiny_gltf.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cassert>
#include <cstring>
#include <vector>

namespace drz::model_loader
{
static void
read_float_attrib(const tinygltf::Model& model, const tinygltf::Accessor& acc, int components, std::vector<float>& out)
{
    assert(acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

    const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
    const size_t elem_size = components * sizeof(float);
    const size_t stride = bv.byteStride != 0 ? bv.byteStride : elem_size;

    const unsigned char* base = model.buffers[bv.buffer].data.data() + bv.byteOffset + acc.byteOffset;

    std::vector<float> tmp(acc.count * components);
    for (size_t i = 0; i < acc.count; ++i)
        std::memcpy(tmp.data() + i * components, base + i * stride, elem_size);

    out.insert(out.end(), tmp.begin(), tmp.end());
}

static void read_indices(const tinygltf::Model& model, const tinygltf::Accessor& acc, std::vector<uint32_t>& out)
{
    assert(acc.type == TINYGLTF_TYPE_SCALAR);

    const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];

    // element size is fixed per accessor by component type, so resolve it once up front
    size_t elem_size = 0;
    switch (acc.componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        elem_size = sizeof(uint8_t);
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        elem_size = sizeof(uint16_t);
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        elem_size = sizeof(uint32_t);
        break;
    default:
        assert(false && "unsupported index component type");
        return;
    }

    const size_t stride = bv.byteStride != 0 ? bv.byteStride : elem_size;
    const unsigned char* base = model.buffers[bv.buffer].data.data() + bv.byteOffset + acc.byteOffset;

    for (size_t i = 0; i < acc.count; ++i)
    {
        const unsigned char* p = base + i * stride;
        switch (acc.componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        {
            uint8_t v;
            std::memcpy(&v, p, sizeof(v));
            out.push_back(v);
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        {
            uint16_t v;
            std::memcpy(&v, p, sizeof(v));
            out.push_back(v);
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        {
            uint32_t v;
            std::memcpy(&v, p, sizeof(v));
            out.push_back(v);
            break;
        }
        default:
            assert(false && "unsupported index component type");
            return;
        }
    }
}


MeshHandle load_mesh_gltf(const std::string& path, MeshPool& pool)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    // TODO: placeholder image loader callback, not implemented properly yet
    loader.SetImageLoader(
        [](tinygltf::Image* image,
           int /*image_idx*/,
           std::string* /*err*/,
           std::string* /*warn*/,
           int /*req_w*/,
           int /*req_h*/,
           const unsigned char* bytes,
           int size,
           void* /*user*/) -> bool
        {
            // placeholders
            (void)bytes;
            (void)size;
            image->width = 1;
            image->height = 1;
            image->component = 4; // rgba convention
            image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
            return true;
        },
        nullptr);

    if (!loader.LoadBinaryFromFile(&model, &err, &warn, path))
    {
        DRZ_LOGF("gltf load failed: {} :: {}", path, err);
        return {};
    }
    if (!warn.empty())
    {
        DRZ_LOGF("gltf warn: {}", warn);
    }

    if (model.meshes.empty() || model.meshes[0].primitives.empty())
    {
        DRZ_LOGF("gltf has no mesh primitives: {}", path);
        return {};
    }

    const tinygltf::Primitive& primitive = model.meshes[0].primitives[0];

    const tinygltf::Accessor& idx_acc = model.accessors[primitive.indices];
    DRZ_LOGF("indices : count={} componentType={}", idx_acc.count, idx_acc.componentType);

    // inspect and print out contents of file
    for (const auto& [attrib_name, accessor_idx] : primitive.attributes)
    {
        const tinygltf::Accessor& accessor = model.accessors[accessor_idx];
        DRZ_LOGF("attrib {} : accessor count={} accessor component type={} accessor type={}",
                 attrib_name,
                 accessor.count,
                 accessor.componentType,
                 accessor.type);
    }

    std::vector<float> positions, normals, uvs;
    std::vector<uint32_t> indices;

    auto find_attr = [&](const char* name) -> const tinygltf::Accessor*
    {
        auto it = primitive.attributes.find(name);
        return it != primitive.attributes.end() ? &model.accessors[it->second] : nullptr;
    };

    if (const auto* a = find_attr("POSITION"))
        read_float_attrib(model, *a, 3, positions);
    else
    {
        DRZ_LOGF("gltf has no POSITION attribute: {}", path);
        return {};
    }

    const size_t vert_count = positions.size() / 3;

    if (const auto* a = find_attr("NORMAL"))
        read_float_attrib(model, *a, 3, normals);
    else
    {
        DRZ_LOGF("gltf has no NORMALs, defaulting to (0,1,0): {}", path);
        for (size_t i = 0; i < vert_count; ++i)
            normals.insert(normals.end(), {0.f, 1.f, 0.f});
    }

    if (const auto* a = find_attr("TEXCOORD_0"))
        read_float_attrib(model, *a, 2, uvs);
    else
    {
        DRZ_LOGF("gltf has no TEXCOORD_0, defaulting to (0,0): {}", path);
        uvs.assign(vert_count * 2, 0.f);
    }

    read_indices(model, model.accessors[primitive.indices], indices);

    drz::MeshDesc desc {.positions = positions, .normals = normals, .uvs = uvs, .indices = indices};
    return pool.upload(desc);
}
} // namespace drz::model_loader
