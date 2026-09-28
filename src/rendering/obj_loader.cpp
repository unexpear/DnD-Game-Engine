/**
 * @file obj_loader.cpp
 * @brief Implementation of the Wavefront OBJ parser.
 *
 * See obj_loader.h for full documentation.
 *
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.1.0
 */

#include "rendering/obj_loader.h"
#include "core/log.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unordered_map>
#include <vector>

namespace Dnd {

// ==========================================
// INTERNAL: Vertex cache key
// ==========================================

/**
 * @brief Uniquely identifies a vertex by its OBJ index triplet.
 *
 * OBJ files reference vertices using separate indices for position,
 * normal, and texture coordinate. The same logical vertex may be
 * referenced multiple times across faces; this key deduplicates them.
 */
struct ObjVertexKey {
    i32 positionIndex;  ///< Index into the position array (-1 = unused)
    i32 normalIndex;    ///< Index into the normal array   (-1 = unused)
    i32 uvIndex;        ///< Index into the UV array       (-1 = unused)

    bool operator==(const ObjVertexKey& other) const {
        return positionIndex == other.positionIndex &&
               normalIndex   == other.normalIndex &&
               uvIndex       == other.uvIndex;
    }
};

/**
 * @brief Hash functor for ObjVertexKey (used by std::unordered_map).
 *
 * Combines the three indices using large primes for good distribution.
 */
struct ObjVertexKeyHash {
    size_t operator()(const ObjVertexKey& key) const {
        size_t h = 0;
        h ^= static_cast<size_t>(static_cast<u32>(key.positionIndex)) * 73856093u;
        h ^= static_cast<size_t>(static_cast<u32>(key.normalIndex))   * 19349663u;
        h ^= static_cast<size_t>(static_cast<u32>(key.uvIndex))       * 83492791u;
        return h;
    }
};

// ==========================================
// PUBLIC API
// ==========================================

b8 ObjLoader::Load(const char* filepath,
                   std::vector<Vertex>& outVertices,
                   std::vector<u32>& outIndices) {
    if (!filepath) {
        DND_LOG_ERROR("ObjLoader: null filepath");
        return false;
    }

    FILE* file = fopen(filepath, "r");
    if (!file) {
        DND_LOG_ERROR("ObjLoader: could not open file '%s'", filepath);
        return false;
    }

    // ---- Temporary storage for OBJ arrays ----
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    positions.reserve(2048);
    normals.reserve(2048);
    uvs.reserve(2048);

    // ---- Vertex deduplication cache ----
    std::unordered_map<ObjVertexKey, u32, ObjVertexKeyHash> vertexCache;

    outVertices.clear();
    outIndices.clear();

    // ---- Line-by-line parsing ----
    char line[1024];
    i32 lineNumber = 0;

    while (fgets(line, static_cast<i32>(sizeof(line)), file)) {
        ++lineNumber;

        // Skip empty lines and comments
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r' || line[0] == '\0') {
            continue;
        }

        // ---- v : vertex position ----
        if (line[0] == 'v' && (line[1] == ' ' || line[1] == '\t')) {
            Vec3 pos;
            if (sscanf(line + 2, "%f %f %f", &pos.x, &pos.y, &pos.z) >= 3) {
                positions.push_back(pos);
            }
            continue;
        }

        // ---- vn : vertex normal ----
        if (line[0] == 'v' && line[1] == 'n' && (line[2] == ' ' || line[2] == '\t')) {
            Vec3 norm;
            if (sscanf(line + 3, "%f %f %f", &norm.x, &norm.y, &norm.z) >= 3) {
                normals.push_back(norm);
            }
            continue;
        }

        // ---- vt : texture coordinate ----
        if (line[0] == 'v' && line[1] == 't' && (line[2] == ' ' || line[2] == '\t')) {
            Vec2 uv;
            if (sscanf(line + 3, "%f %f", &uv.x, &uv.y) >= 2) {
                uvs.push_back(uv);
            }
            continue;
        }

        // ---- f : face definition ----
        if (line[0] == 'f' && (line[1] == ' ' || line[1] == '\t')) {
            // Tokenise the face line into individual vertex references
            const char* ptr = line + 2;
            std::vector<u32> faceIndices;
            faceIndices.reserve(4);

            while (*ptr) {
                // Skip whitespace
                while (*ptr == ' ' || *ptr == '\t') {
                    ++ptr;
                }
                if (*ptr == '\n' || *ptr == '\r' || *ptr == '\0') {
                    break;
                }

                // Extract one vertex token (v, v/vt, v//vn, or v/vt/vn)
                const char* tokenStart = ptr;
                while (*ptr && *ptr != ' ' && *ptr != '\t' &&
                       *ptr != '\n' && *ptr != '\r') {
                    ++ptr;
                }

                // Build a null-terminated copy of the token for parsing
                char token[64];
                size_t tokenLen = static_cast<size_t>(ptr - tokenStart);
                if (tokenLen >= sizeof(token)) {
                    tokenLen = sizeof(token) - 1;
                }
                memcpy(token, tokenStart, tokenLen);
                token[tokenLen] = '\0';

                // ---- Parse the vertex reference ----
                i32 pIdx = 0;   // position index
                i32 tIdx = 0;   // texture coordinate index
                i32 nIdx = 0;   // normal index

                const char* s1 = strchr(token, '/');
                if (!s1) {
                    // Format: v
                    pIdx = atoi(token);
                } else {
                    // Extract position (everything before first '/')
                    {
                        size_t len = static_cast<size_t>(s1 - token);
                        if (len > 0 && len < sizeof(token)) {
                            char buf[64];
                            memcpy(buf, token, len);
                            buf[len] = '\0';
                            pIdx = atoi(buf);
                        }
                    }

                    const char* s2 = strchr(s1 + 1, '/');
                    if (!s2) {
                        // Format: v/vt
                        tIdx = atoi(s1 + 1);
                    } else if (s2 == s1 + 1) {
                        // Format: v//vn
                        nIdx = atoi(s2 + 1);
                    } else {
                        // Format: v/vt/vn
                        {
                            size_t len2 = static_cast<size_t>(s2 - (s1 + 1));
                            if (len2 > 0 && len2 < sizeof(token)) {
                                char buf[64];
                                memcpy(buf, s1 + 1, len2);
                                buf[len2] = '\0';
                                tIdx = atoi(buf);
                            }
                        }
                        nIdx = atoi(s2 + 1);
                    }
                }

                // Convert from OBJ 1-based indexing to 0-based
                // A value of 0 means "not present" (sentinel)
                ObjVertexKey key;
                key.positionIndex = pIdx - 1;
                key.uvIndex       = tIdx > 0 ? tIdx - 1 : -1;
                key.normalIndex   = nIdx > 0 ? nIdx - 1 : -1;

                // Check the vertex cache
                auto it = vertexCache.find(key);
                if (it != vertexCache.end()) {
                    faceIndices.push_back(it->second);
                } else {
                    u32 newIndex = static_cast<u32>(outVertices.size());
                    faceIndices.push_back(newIndex);
                    vertexCache[key] = newIndex;

                    Vertex vert;
                    if (key.positionIndex >= 0 &&
                        key.positionIndex < static_cast<i32>(positions.size())) {
                        vert.position = positions[key.positionIndex];
                    }
                    if (key.normalIndex >= 0 &&
                        key.normalIndex < static_cast<i32>(normals.size())) {
                        vert.normal = normals[key.normalIndex];
                    }
                    if (key.uvIndex >= 0 &&
                        key.uvIndex < static_cast<i32>(uvs.size())) {
                        vert.uv = uvs[key.uvIndex];
                    }
                    vert.color = Vec4(1, 1, 1, 1);

                    outVertices.push_back(vert);
                }
            }

            // ---- Triangulate the face ----
            i32 faceSize = static_cast<i32>(faceIndices.size());
            if (faceSize == 3) {
                // Triangle: 0-1-2
                outIndices.push_back(faceIndices[0]);
                outIndices.push_back(faceIndices[1]);
                outIndices.push_back(faceIndices[2]);
            } else if (faceSize == 4) {
                // Quad: triangulate to 0-1-2 and 0-2-3
                outIndices.push_back(faceIndices[0]);
                outIndices.push_back(faceIndices[1]);
                outIndices.push_back(faceIndices[2]);
                outIndices.push_back(faceIndices[0]);
                outIndices.push_back(faceIndices[2]);
                outIndices.push_back(faceIndices[3]);
            } else if (faceSize > 4) {
                // Polygon with N > 4: simple triangle fan (0-1-2, 0-2-3, ...)
                // This is a basic fan; concave polygons may produce bad geometry
                for (i32 i = 2; i < faceSize; ++i) {
                    outIndices.push_back(faceIndices[0]);
                    outIndices.push_back(faceIndices[i - 1]);
                    outIndices.push_back(faceIndices[i]);
                }
            }
            // faceSize < 3: degenerate face, skip
        }

        // All other lines (o, g, s, usemtl, mtllib, etc.) are ignored
    }

    fclose(file);

    // ---- Validate results ----
    if (outVertices.empty() || outIndices.empty()) {
        DND_LOG_WARN("ObjLoader: no geometry found in '%s'", filepath);
        return false;
    }

    DND_LOG_DEBUG("ObjLoader: loaded '%s' (%zu vertices, %zu indices, "
                  "%zu positions, %zu normals, %zu uvs)",
                  filepath,
                  outVertices.size(), outIndices.size(),
                  positions.size(), normals.size(), uvs.size());

    return true;
}

} // namespace Dnd
