#pragma once

#include "drz/gfx/MeshPool.hpp"

#include <string>

namespace drz::model_loader
{
MeshHandle load_mesh_gltf(const std::string& path, MeshPool& pool);
} // namespace drz::model_loader
