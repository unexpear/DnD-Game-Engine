/**
 * @file mesh_renderer.h
 * @brief MeshRenderer component for renderable entities.
 * 
 * References a mesh resource for rendering. The mesh data is managed
 * externally; this component holds a pointer to the mesh.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "core/types.h"
#include "assets/asset_manager.h"

namespace Dnd {

// Forward declaration
class Mesh;

struct MeshRenderer {
    const Mesh* mesh;
    Color color;
    b8 visible;
    AssetHandle diffuseTexture;
    f32 shininess = 32.0f;

    MeshRenderer() : mesh(nullptr), color(1, 1, 1, 1), visible(true) {}

    explicit MeshRenderer(const Mesh* meshPtr, const Color& tint = Color(1, 1, 1, 1))
        : mesh(meshPtr), color(tint), visible(true) {}
};

} // namespace Dnd
