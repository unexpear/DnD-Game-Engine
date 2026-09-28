/**
 * @file spatial_hash.h
 * @brief Spatial hash grid for broad-phase collision detection.
 * 
 * Divides space into uniform cells and maps colliders to cells based on
 * their bounding boxes. Only tests pairs that share at least one cell,
 * dramatically reducing the number of narrow-phase checks.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include "ecs/entity.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Dnd {

// ==========================================
// STRUCT: SpatialHash
// ==========================================

/**
 * @struct SpatialHash
 * @brief Uniform grid spatial hash for broad-phase collision detection.
 * 
 * Space is divided into cells of fixed size. Each cell stores a list of
 * entity indices that overlap that cell. During collision detection, only
 * pairs of entities in the same cell are tested.
 */
class SpatialHash {
public:
    /**
     * @brief Constructs a spatial hash with a given cell size.
     * @param cellSize Size of each grid cell. Should be larger than most objects.
     */
    explicit SpatialHash(f32 cellSize = 10.0f);

    /**
     * @brief Clears all entries.
     */
    void Clear();

    /**
     * @brief Inserts an entity into the hash based on its bounding box.
     * @param entityIndex Entity index.
     * @param minMin Minimum corner of the AABB.
     * @param maxMax Maximum corner of the AABB.
     */
    void Insert(u32 entityIndex, const Vec3& min, const Vec3& max);

    /**
     * @brief Gets all candidate pairs for narrow-phase testing.
     * @return Vector of entity index pairs. No duplicates.
     */
    std::vector<std::pair<u32, u32>> GetCandidatePairs() const;

    /// @brief Sets the cell size. Clears the hash.
    void SetCellSize(f32 cellSize) { m_cellSize = cellSize; m_invCellSize = 1.0f / cellSize; Clear(); }

    /// @brief Gets the current cell size.
    f32 GetCellSize() const { return m_cellSize; }

    /// @brief Gets the number of occupied cells.
    u32 GetCellCount() const { return static_cast<u32>(m_grid.size()); }

private:
    /**
     * @brief Computes the cell key for a world position.
     * @param pos World position.
     * @return Cell key (hashable integer).
     */
    i64 ComputeCellKey(const Vec3& pos) const;

    /**
     * @brief Hash function for cell keys.
     */
    struct CellHash {
        size_t operator()(i64 key) const { return static_cast<size_t>(key ^ (key >> 32)); }
    };

    f32 m_cellSize;       ///< Size of each grid cell
    f32 m_invCellSize;    ///< 1 / cellSize (precomputed)
    std::unordered_map<i64, std::vector<u32>, CellHash> m_grid; ///< Cell key -> entity indices
};

} // namespace Dnd
