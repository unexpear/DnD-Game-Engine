/**
 * @file obj_loader.h
 * @brief Wavefront OBJ file parser for loading 3D geometry.
 *
 * Parses standard .obj files into engine vertex and index arrays.
 * Supports vertex positions (v), normals (vn), texture coordinates (vt),
 * and faces (f) with v/vt/vn, v//vn, v/vt, and v-only formats.
 *
 * Features:
 *   - Vertex deduplication via position/normal/UV cache
 *   - Automatic triangle fan triangulation for quads
 *   - Handles 1-based OBJ index convention (converts to 0-based)
 *   - Graceful skipping of unsupported elements (groups, materials, etc.)
 *
 * @note This is a minimal parser. It does NOT support:
 *       - Free-form geometry (curves, surfaces)
 *       - Material library references (mtllib/usemtl)
 *       - Line or point primitives
 *       - Object/group definitions for multi-object files
 *
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include "rendering/mesh.h"
#include <vector>

namespace Dnd {

/**
 * @class ObjLoader
 * @brief Static OBJ file loader that produces vertex and index arrays.
 *
 * Usage:
 * @code
 * std::vector<Vertex> vertices;
 * std::vector<u32> indices;
 * if (ObjLoader::Load("model.obj", vertices, indices)) {
 *     mesh.Create(vertices.data(), vertices.size(),
 *                 indices.data(), indices.size());
 * }
 * @endcode
 */
class ObjLoader {
public:
    /**
     * @brief Loads a Wavefront OBJ file and produces indexed triangle geometry.
     *
     * Parses the file line by line, extracting:
     *   - Vertex positions (v x y z)
     *   - Vertex normals (vn x y z)
     *   - Texture coordinates (vt u v)
     *   - Face definitions (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3)
     *
     * Vertices are deduplicated: if two faces reference the same
     * (position, normal, UV) combination, they share a single Vertex.
     *
     * Quad faces are automatically triangulated into two triangles.
     *
     * @param filepath Path to the .obj file (absolute or relative).
     * @param outVertices [out] Filled with deduplicated Vertex data.
     * @param outIndices  [out] Filled with triangle index triplets.
     *
     * @return true on success (file opened, geometry found and parsed).
     *         false on failure (file not found, parse error, no geometry).
     */
    static b8 Load(const char* filepath,
                   std::vector<Vertex>& outVertices,
                   std::vector<u32>& outIndices);

private:
    ObjLoader() = delete;  // Static class only
};

} // namespace Dnd
