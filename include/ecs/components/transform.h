/**
 * @file transform.h
 * @brief Transform component for spatial hierarchy.
 * 
 * Stores position, rotation, and scale for an entity. Supports parent-child
 * relationships with automatic world matrix computation.
 * 
 * Coordinate system: Right-handed, Y-up.
 * Rotation: Stored as a quaternion (no gimbal lock).
 * Scale: Non-uniform scaling supported.
 * 
 * @note World transform is computed lazily when GetWorldMatrix() is called.
 * @warning Modifying local transform invalidates cached world matrix.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include "ecs/entity.h"
#include <functional>

namespace Dnd {

/**
 * @struct Transform
 * @brief Position, rotation, and scale of an entity.
 * 
 * Local transform is relative to the parent. World transform is computed
 * by multiplying the parent's world transform with the local transform.
 */
struct Transform {
    Vec3 position;    ///< Local position relative to parent (or world if no parent)
    Quat rotation;    ///< Local rotation as quaternion (identity = no rotation)
    Vec3 scale;       ///< Local scale (1,1,1 = no scaling)
    Entity parent;    ///< Parent entity (Entity::None() if root)

    /// @brief Default constructor (origin, identity rotation, unit scale, no parent).
    Transform()
        : position(0, 0, 0), rotation(1, 0, 0, 0), scale(1, 1, 1), parent(Entity::None()) {}

    /**
     * @brief Constructs a transform with explicit values.
     * @param pos Local position.
     * @param rot Local rotation (quaternion).
     * @param scl Local scale.
     * @param parentEntity Parent entity (Entity::None() if root).
     */
    Transform(const Vec3& pos, const Quat& rot, const Vec3& scl, Entity parentEntity = Entity::None())
        : position(pos), rotation(rot), scale(scl), parent(parentEntity) {}

    /**
     * @brief Gets the world (global) transformation matrix.
     * 
     * Implementation:
     *   1. Build local transform matrix from position, rotation, scale
     *   2. If has parent, multiply parent's world matrix with local matrix
     *   3. Return the resulting world matrix
     * 
     * @param getWorldMatrixFn Callback to get parent's world matrix.
     * @return 4x4 world transformation matrix.
     */
    Mat4 GetWorldMatrix(const std::function<Mat4(Entity)>& getWorldMatrixFn) const {
        Mat4 local = Mat4::TRS(position, rotation, scale);
        if (parent.IsValid() && getWorldMatrixFn) {
            return getWorldMatrixFn(parent) * local;
        }
        return local;
    }
};

} // namespace Dnd
