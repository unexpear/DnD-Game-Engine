/**
 * @file asset_manager.cpp
 * @brief Implementation of the AssetManager singleton for loading, caching, and
 *        managing game assets (meshes, audio clips, etc.).
 *
 * Provides search path resolution, reference-counted asset caching,
 * and deferred deletion when all references are released.
 *
 * @see asset_manager.h for full API documentation.
 *
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.9.0
 */

#include "assets/asset_manager.h"
#include "rendering/mesh.h"
#include "rendering/obj_loader.h"
#include "rendering/texture.h"
#include "audio/audio_clip.h"
#include "core/log.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace Dnd {

// ==========================================
// SINGLETON ACCESS
// ==========================================

AssetManager& AssetManager::Get() {
    static AssetManager instance;
    return instance;
}

// ==========================================
// LIFE CYCLE
// ==========================================

AssetManager::AssetManager()
    : m_initialized(false) {
}

AssetManager::~AssetManager() {
    Shutdown();
}

b8 AssetManager::Initialize() {
    m_initialized = true;
    DND_LOG_INFO("AssetManager initialized");
    return true;
}

void AssetManager::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Free all allocated assets
    for (auto& entry : m_assets) {
        if (!entry.data) {
            continue;
        }
        switch (entry.type) {
            case AssetType::MESH:
                delete static_cast<Mesh*>(entry.data);
                break;
            case AssetType::AUDIO_CLIP:
                delete static_cast<AudioClip*>(entry.data);
                break;
            case AssetType::TEXTURE:
                delete static_cast<Texture2D*>(entry.data);
                break;
            default:
                break;
        }
        entry.data = nullptr;
    }

    m_assets.clear();
    m_searchPaths.clear();
    m_initialized = false;

    DND_LOG_INFO("AssetManager shut down");
}

// ==========================================
// SEARCH PATH MANAGEMENT
// ==========================================

void AssetManager::AddSearchPath(const char* path) {
    if (!path || path[0] == '\0') {
        return;
    }

    std::string normalized = path;

    // Normalise backslashes to forward slashes for cross-platform consistency
    for (auto& ch : normalized) {
        if (ch == '\\') {
            ch = '/';
        }
    }

    // Ensure trailing forward slash
    if (normalized.back() != '/') {
        normalized += '/';
    }

    m_searchPaths.push_back(normalized);
    DND_LOG_DEBUG("AssetManager: added search path '%s'", normalized.c_str());
}

std::string AssetManager::ResolvePath(const char* relativePath) {
    if (!relativePath || relativePath[0] == '\0') {
        return "";
    }

    // 1. Try the path as-is (absolute or relative to working directory)
    FILE* f = fopen(relativePath, "rb");
    if (f) {
        fclose(f);
        return relativePath;
    }

    // 2. Try each registered search path in order
    for (const auto& searchPath : m_searchPaths) {
        std::string fullPath = searchPath + relativePath;
        f = fopen(fullPath.c_str(), "rb");
        if (f) {
            fclose(f);
            return fullPath;
        }
    }

    // Not found anywhere
    DND_LOG_WARN("AssetManager: could not resolve '%s'", relativePath);
    return "";
}

// ==========================================
// ASSET LOADING
// ==========================================

AssetHandle AssetManager::LoadMesh(const char* path) {
    if (!path || path[0] == '\0') {
        DND_LOG_ERROR("AssetManager::LoadMesh: invalid path (null or empty)");
        return AssetHandle::Invalid();
    }

    if (!m_initialized) {
        DND_LOG_ERROR("AssetManager::LoadMesh: manager not initialized");
        return AssetHandle::Invalid();
    }

    // ---- Resolve file path ----
    std::string resolved = ResolvePath(path);
    if (resolved.empty()) {
        DND_LOG_ERROR("AssetManager::LoadMesh: file not found '%s'", path);
        return AssetHandle::Invalid();
    }

    for (u32 i = 0; i < static_cast<u32>(m_assets.size()); ++i) {
        if (m_assets[i].path == resolved && m_assets[i].type == AssetType::MESH && m_assets[i].data != nullptr) {
            m_assets[i].refCount++;
            DND_LOG_DEBUG("AssetManager::LoadMesh: cache hit '%s' (ref %u)",
                          resolved.c_str(), m_assets[i].refCount);
            return AssetHandle(i, AssetType::MESH);
        }
    }

    // ---- Load geometry via OBJ parser ----
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    if (!ObjLoader::Load(resolved.c_str(), vertices, indices)) {
        DND_LOG_ERROR("AssetManager::LoadMesh: failed to parse '%s'", resolved.c_str());
        return AssetHandle::Invalid();
    }

    if (vertices.empty() || indices.empty()) {
        DND_LOG_ERROR("AssetManager::LoadMesh: no geometry in '%s'", resolved.c_str());
        return AssetHandle::Invalid();
    }

    // ---- Create GPU mesh ----
    Mesh* mesh = new Mesh();
    mesh->Create(
        vertices.data(),
        static_cast<i32>(vertices.size()),
        indices.data(),
        static_cast<i32>(indices.size())
    );

    // ---- Store in cache ----
    AssetEntry entry;
    entry.path     = resolved;
    entry.type     = AssetType::MESH;
    entry.data     = mesh;
    entry.refCount = 1;

    m_assets.push_back(std::move(entry));
    u32 id = static_cast<u32>(m_assets.size() - 1);

    DND_LOG_INFO("AssetManager::LoadMesh: loaded '%s' (%zu verts, %zu indices, id=%u)",
                 path, vertices.size(), indices.size(), id);
    return AssetHandle(id, AssetType::MESH);
}

AssetHandle AssetManager::LoadAudioClip(const char* path) {
    if (!path || path[0] == '\0') {
        DND_LOG_ERROR("AssetManager::LoadAudioClip: invalid path (null or empty)");
        return AssetHandle::Invalid();
    }

    if (!m_initialized) {
        DND_LOG_ERROR("AssetManager::LoadAudioClip: manager not initialized");
        return AssetHandle::Invalid();
    }

    // ---- Resolve file path ----
    std::string resolved = ResolvePath(path);
    if (resolved.empty()) {
        DND_LOG_ERROR("AssetManager::LoadAudioClip: file not found '%s'", path);
        return AssetHandle::Invalid();
    }

    for (u32 i = 0; i < static_cast<u32>(m_assets.size()); ++i) {
        if (m_assets[i].path == resolved && m_assets[i].type == AssetType::AUDIO_CLIP && m_assets[i].data != nullptr) {
            m_assets[i].refCount++;
            DND_LOG_DEBUG("AssetManager::LoadAudioClip: cache hit '%s' (ref %u)",
                          resolved.c_str(), m_assets[i].refCount);
            return AssetHandle(i, AssetType::AUDIO_CLIP);
        }
    }

    // ---- Load WAV file ----
    AudioClip* clip = new AudioClip();
    if (!clip->LoadFromFile(resolved.c_str())) {
        DND_LOG_ERROR("AssetManager::LoadAudioClip: failed to load '%s'", resolved.c_str());
        delete clip;
        return AssetHandle::Invalid();
    }

    // ---- Store in cache ----
    AssetEntry entry;
    entry.path     = resolved;
    entry.type     = AssetType::AUDIO_CLIP;
    entry.data     = clip;
    entry.refCount = 1;

    m_assets.push_back(std::move(entry));
    u32 id = static_cast<u32>(m_assets.size() - 1);

    DND_LOG_INFO("AssetManager::LoadAudioClip: loaded '%s' (id=%u)", path, id);
    return AssetHandle(id, AssetType::AUDIO_CLIP);
}

AssetHandle AssetManager::LoadTexture(const char* path) {
    if (!path || path[0] == '\0') {
        DND_LOG_ERROR("AssetManager::LoadTexture: invalid path");
        return AssetHandle::Invalid();
    }
    if (!m_initialized) {
        DND_LOG_ERROR("AssetManager::LoadTexture: manager not initialized");
        return AssetHandle::Invalid();
    }

    std::string resolved = ResolvePath(path);
    if (resolved.empty()) {
        DND_LOG_ERROR("AssetManager::LoadTexture: file not found '%s'", path);
        return AssetHandle::Invalid();
    }

    for (u32 i = 0; i < static_cast<u32>(m_assets.size()); ++i) {
        if (m_assets[i].path == resolved && m_assets[i].type == AssetType::TEXTURE && m_assets[i].data != nullptr) {
            m_assets[i].refCount++;
            return AssetHandle(i, AssetType::TEXTURE);
        }
    }

    Texture2D* tex = new Texture2D();
    if (!tex->LoadFromFile(resolved)) {
        delete tex;
        return AssetHandle::Invalid();
    }

    AssetEntry entry;
    entry.path = resolved;
    entry.type = AssetType::TEXTURE;
    entry.data = tex;
    entry.refCount = 1;
    m_assets.push_back(std::move(entry));
    u32 id = static_cast<u32>(m_assets.size() - 1);
    DND_LOG_INFO("AssetManager::LoadTexture: loaded '%s' (id=%u)", path, id);
    return AssetHandle(id, AssetType::TEXTURE);
}

Texture2D* AssetManager::GetTexture(AssetHandle handle) {
    if (!handle.IsValid()) return nullptr;
    if (handle.id >= static_cast<u32>(m_assets.size())) {
        DND_LOG_ERROR("AssetManager::GetTexture: handle id %u out of range", handle.id);
        return nullptr;
    }
    const auto& entry = m_assets[handle.id];
    if (entry.type != AssetType::TEXTURE) {
        DND_LOG_ERROR("AssetManager::GetTexture: handle %u is not a texture", handle.id);
        return nullptr;
    }
    return static_cast<Texture2D*>(entry.data);
}

// ==========================================
// ASSET RETRIEVAL
// ==========================================

Mesh* AssetManager::GetMesh(AssetHandle handle) {
    if (!handle.IsValid()) {
        return nullptr;
    }
    if (handle.id >= static_cast<u32>(m_assets.size())) {
        DND_LOG_ERROR("AssetManager::GetMesh: handle id %u out of range", handle.id);
        return nullptr;
    }

    const auto& entry = m_assets[handle.id];
    if (entry.type != AssetType::MESH) {
        DND_LOG_ERROR("AssetManager::GetMesh: handle %u is not a mesh", handle.id);
        return nullptr;
    }
    if (!entry.data) {
        DND_LOG_ERROR("AssetManager::GetMesh: handle %u data is null", handle.id);
        return nullptr;
    }

    return static_cast<Mesh*>(entry.data);
}

AudioClip* AssetManager::GetAudioClip(AssetHandle handle) {
    if (!handle.IsValid()) {
        return nullptr;
    }
    if (handle.id >= static_cast<u32>(m_assets.size())) {
        DND_LOG_ERROR("AssetManager::GetAudioClip: handle id %u out of range", handle.id);
        return nullptr;
    }

    const auto& entry = m_assets[handle.id];
    if (entry.type != AssetType::AUDIO_CLIP) {
        DND_LOG_ERROR("AssetManager::GetAudioClip: handle %u is not an audio clip", handle.id);
        return nullptr;
    }
    if (!entry.data) {
        DND_LOG_ERROR("AssetManager::GetAudioClip: handle %u data is null", handle.id);
        return nullptr;
    }

    return static_cast<AudioClip*>(entry.data);
}

// ==========================================
// ASSET UNLOADING
// ==========================================

void AssetManager::UnloadAsset(AssetHandle handle) {
    if (!handle.IsValid()) {
        return;
    }
    if (handle.id >= static_cast<u32>(m_assets.size())) {
        DND_LOG_ERROR("AssetManager::UnloadAsset: handle id %u out of range", handle.id);
        return;
    }

    auto& entry = m_assets[handle.id];
    if (entry.refCount == 0) {
        DND_LOG_WARN("AssetManager::UnloadAsset: handle %u already has zero ref count", handle.id);
        return;
    }

    entry.refCount--;

    if (entry.refCount == 0) {
        DND_LOG_INFO("AssetManager::UnloadAsset: freeing '%s'", entry.path.c_str());

        switch (entry.type) {
            case AssetType::MESH:
                delete static_cast<Mesh*>(entry.data);
                break;
            case AssetType::AUDIO_CLIP:
                delete static_cast<AudioClip*>(entry.data);
                break;
            case AssetType::TEXTURE:
                delete static_cast<Texture2D*>(entry.data);
                break;
            default:
                break;
        }
        entry.data = nullptr;
    } else {
        DND_LOG_DEBUG("AssetManager::UnloadAsset: '%s' ref now %u",
                      entry.path.c_str(), entry.refCount);
    }
}

// ==========================================
// METADATA
// ==========================================

const char* AssetManager::GetAssetPath(AssetHandle handle) const {
    if (!handle.IsValid()) return nullptr;
    if (handle.id >= static_cast<u32>(m_assets.size())) return nullptr;
    return m_assets[handle.id].path.c_str();
}

AssetHandle AssetManager::FindAssetByData(void* data) const {
    if (!data) return AssetHandle::Invalid();
    for (u32 i = 0; i < static_cast<u32>(m_assets.size()); ++i) {
        if (m_assets[i].data == data) {
            return AssetHandle(i, m_assets[i].type);
        }
    }
    return AssetHandle::Invalid();
}

// ==========================================
// QUERIES
// ==========================================

u32 AssetManager::GetLoadedAssetCount() const {
    u32 count = 0;
    for (const auto& entry : m_assets) {
        if (entry.data != nullptr) {
            ++count;
        }
    }
    return count;
}

} // namespace Dnd
