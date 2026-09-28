/**
 * @file asset_manager.h
 * @brief Asset management: loading, caching, and accessing game assets.
 * 
 * The AssetManager is a singleton that loads and caches assets
 * (textures, meshes, audio, animations, etc.).
 * 
 * Features:
 *   - Reference counting for shared assets
 *   - Synchronous and asynchronous loading
 *   - Asset hot-reloading (file watcher)
 *   - Search paths for asset resolution
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.9.0
 */

#pragma once

#include "core/types.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Dnd {

// Forward declarations
class Mesh;
class AudioClip;
class Texture2D;

// ==========================================
// ENUM: AssetType
// ==========================================

/**
 * @enum AssetType
 * @brief Types of assets that can be loaded.
 */
enum class AssetType : i32 {
    MESH,
    TEXTURE,
    AUDIO_CLIP,
    ANIMATION_CLIP,
    SHADER,
    MATERIAL,
    UNKNOWN
};

// ==========================================
// STRUCT: AssetHandle
// ==========================================

/**
 * @struct AssetHandle
 * @brief Weak reference to a loaded asset.
 * 
 * Asset handles are lightweight references to assets.
 * The actual asset data is owned by the AssetManager.
 */
struct AssetHandle {
    u32 id;          ///< Internal asset ID
    AssetType type;   ///< Asset type

    /// @brief Default constructor (invalid handle).
    AssetHandle() : id(0xFFFFFFFF), type(AssetType::UNKNOWN) {}

    /// @brief Constructs an asset handle.
    AssetHandle(u32 assetId, AssetType assetType) : id(assetId), type(assetType) {}

    /// @brief Checks if handle is valid.
    bool IsValid() const { return id != 0xFFFFFFFF; }

    /// @brief Invalid handle constant.
    static AssetHandle Invalid() { return AssetHandle(); }
};

// ==========================================
// CLASS: AssetManager
// ==========================================

/**
 * @class AssetManager
 * @brief Singleton asset manager for loading and caching assets.
 * 
 * The AssetManager loads assets from disk and caches them by path.
 * Multiple requests for the same asset return the same cached instance.
 * 
 * Usage:
 * @code
 * AssetManager::Get().Initialize();
 * AssetHandle meshHandle = AssetManager::Get().LoadMesh("assets/models/cube.obj");
 * Mesh* mesh = AssetManager::Get().GetMesh(meshHandle);
 * @endcode
 */
class AssetManager {
public:
    /**
     * @brief Gets the singleton instance.
     * @return Reference to the AssetManager instance.
     */
    static AssetManager& Get();

    /**
     * @brief Initializes the asset manager.
     * @return true if successful.
     */
    b8 Initialize();

    /**
     * @brief Shuts down the asset manager (frees all assets).
     */
    void Shutdown();

    /**
     * @brief Adds a search path for asset resolution.
     * @param path Directory path to add.
     */
    void AddSearchPath(const char* path);

    /**
     * @brief Resolves a relative path to an absolute path.
     * @param relativePath Relative asset path.
     * @return Absolute path, or empty if not found.
     */
    std::string ResolvePath(const char* relativePath);

    // Asset loading methods

    /**
     * @brief Loads a mesh asset.
     * @param path Path to mesh file (OBJ, glTF, etc.).
     * @return Asset handle, or invalid if failed.
     */
    AssetHandle LoadMesh(const char* path);

    /**
     * @brief Loads an audio clip asset.
     * @param path Path to audio file (WAV, OGG).
     * @return Asset handle, or invalid if failed.
     */
    AssetHandle LoadAudioClip(const char* path);

    /**
     * @brief Gets a loaded mesh by handle.
     * @param handle Asset handle.
     * @return Pointer to mesh, or nullptr if not found/invalid type.
     */
    Mesh* GetMesh(AssetHandle handle);

    /**
     * @brief Gets a loaded audio clip by handle.
     * @param handle Asset handle.
     * @return Pointer to audio clip, or nullptr if not found/invalid type.
     */
    AudioClip* GetAudioClip(AssetHandle handle);

    AssetHandle LoadTexture(const char* path);
    Texture2D* GetTexture(AssetHandle handle);

    /**
     * @brief Gets the file path of a loaded asset.
     * @param handle Asset handle.
     * @return Path string, or nullptr if handle is invalid.
     */
    const char* GetAssetPath(AssetHandle handle) const;

    /**
     * @brief Finds an asset handle by its data pointer.
     * @param data Pointer to the loaded asset data (e.g., Mesh*).
     * @return AssetHandle if found, or invalid handle.
     */
    AssetHandle FindAssetByData(void* data) const;

    /**
     * @brief Unloads an asset (decrements reference count).
     * @param handle Asset handle.
     */
    void UnloadAsset(AssetHandle handle);

    /**
     * @brief Gets the total number of loaded assets.
     * @return Asset count.
     */
    u32 GetLoadedAssetCount() const;

private:
    /**
     * @brief Private constructor (singleton).
     */
    AssetManager();

    /**
     * @brief Private destructor.
     */
    ~AssetManager();

    /**
     * @brief Asset entry in the cache.
     */
    struct AssetEntry {
        std::string path;      ///< Asset file path
        AssetType type;         ///< Asset type
        void* data;             ///< Pointer to asset data (owned)
        u32 refCount;           ///< Reference count

        AssetEntry() : path(""), type(AssetType::UNKNOWN), data(nullptr), refCount(0) {}
    };

    std::vector<AssetEntry> m_assets;       ///< Asset cache
    std::vector<std::string> m_searchPaths; ///< Search paths
    b8 m_initialized;                      ///< Whether initialized
};

} // namespace Dnd
