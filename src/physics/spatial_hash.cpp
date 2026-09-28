/**
 * @file spatial_hash.cpp
 * @brief Implementation of the SpatialHash class.
 * 
 * See spatial_hash.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#include "physics/spatial_hash.h"

namespace Dnd {

SpatialHash::SpatialHash(f32 cellSize)
    : m_cellSize(cellSize), m_invCellSize(1.0f / cellSize) {
}

void SpatialHash::Clear() {
    m_grid.clear();
}

void SpatialHash::Insert(u32 entityIndex, const Vec3& min, const Vec3& max) {
    // Compute cell range that the AABB overlaps
    i32 minX = static_cast<i32>(std::floor(min.x * m_invCellSize));
    i32 minY = static_cast<i32>(std::floor(min.y * m_invCellSize));
    i32 minZ = static_cast<i32>(std::floor(min.z * m_invCellSize));
    i32 maxX = static_cast<i32>(std::floor(max.x * m_invCellSize));
    i32 maxY = static_cast<i32>(std::floor(max.y * m_invCellSize));
    i32 maxZ = static_cast<i32>(std::floor(max.z * m_invCellSize));

    // Insert entity into all overlapping cells
    for (i32 x = minX; x <= maxX; x++) {
        for (i32 y = minY; y <= maxY; y++) {
            for (i32 z = minZ; z <= maxZ; z++) {
                // Compute a unique cell key from (x, y, z)
                i64 key = (static_cast<i64>(x) << 42) |
                          (static_cast<i64>(y) << 21) |
                          static_cast<i64>(z & 0x1FFFFF);

                auto& cell = m_grid[key];
                // Avoid duplicates in the same cell
                bool found = false;
                for (u32 existing : cell) {
                    if (existing == entityIndex) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    cell.push_back(entityIndex);
                }
            }
        }
    }
}

std::vector<std::pair<u32, u32>> SpatialHash::GetCandidatePairs() const {
    std::unordered_set<i64> seenPairs;
    std::vector<std::pair<u32, u32>> pairs;

    // For each cell, generate all pairs within that cell
    for (const auto& [key, entities] : m_grid) {
        for (size_t i = 0; i < entities.size(); i++) {
            for (size_t j = i + 1; j < entities.size(); j++) {
                // Create a unique pair key (order-independent)
                u32 a = entities[i] < entities[j] ? entities[i] : entities[j];
                u32 b = entities[i] < entities[j] ? entities[j] : entities[i];
                i64 pairKey = (static_cast<i64>(a) << 32) | b;

                if (seenPairs.find(pairKey) == seenPairs.end()) {
                    seenPairs.insert(pairKey);
                    pairs.emplace_back(a, b);
                }
            }
        }
    }

    return pairs;
}

i64 SpatialHash::ComputeCellKey(const Vec3& pos) const {
    i32 x = static_cast<i32>(std::floor(pos.x * m_invCellSize));
    i32 y = static_cast<i32>(std::floor(pos.y * m_invCellSize));
    i32 z = static_cast<i32>(std::floor(pos.z * m_invCellSize));
    return (static_cast<i64>(x) << 42) |
           (static_cast<i64>(y) << 21) |
           static_cast<i64>(z & 0x1FFFFF);
}

} // namespace Dnd
