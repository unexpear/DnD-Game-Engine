/**
 * @file collider.h
 * @brief Collider components for physics collision detection.
 * 
 * Defines collision shape types (Sphere, Box, Capsule) as ECS components.
 * Each collider has a type, bounding parameters, layer, and trigger flag.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include "ecs/entity.h"

namespace Dnd {

// ==========================================
// ENUM: ColliderType
// ==========================================

/**
 * @enum ColliderType
 * @brief Types of collision shapes.
 */
enum class ColliderType : i32 {
    SPHERE = 0,  ///< Sphere defined by radius
    BOX = 1,     ///< Axis-aligned box defined by half-extents
    CAPSULE = 2  ///< Capsule defined by radius and height
};

// ==========================================
// ENUM: PhysicsLayer
// ==========================================

/**
 * @enum PhysicsLayer
 * @brief Physics collision layers for filtering.
 */
enum class PhysicsLayer : u32 {
    DEFAULT     = 1 << 0,
    PLAYER      = 1 << 1,
    ENEMY       = 1 << 2,
    ENVIRONMENT = 1 << 3,
    PROJECTILE  = 1 << 4,
    TRIGGER     = 1 << 5,
    IGNORE      = 0 ///< Special: does not collide with anything
};

// ==========================================
// STRUCT: Bounding volumes (collider query results)
// ==========================================

/**
 * @struct BoundingSphere
 * @brief World-space bounding sphere returned by collider queries.
 */
struct BoundingSphere {
    Vec3 center;   ///< World-space center
    f32 radius;    ///< World-space radius
};

/**
 * @struct AABB
 * @brief World-space axis-aligned bounding box returned by collider queries.
 */
struct AABB {
    Vec3 min;      ///< Minimum corner
    Vec3 max;      ///< Maximum corner
};

// ==========================================
// STRUCT: SphereCollider
// ==========================================

/**
 * @struct SphereCollider
 * @brief Sphere collision shape.
 */
struct SphereCollider {
    Vec3 center;    ///< Local offset from entity position
    f32 radius;     ///< Sphere radius
    u32 layer;      ///< Physics layer bitmask
    b8 isTrigger;   ///< If true, no collision response (events only)
    b8 isEnabled;   ///< Whether this collider is active

    /// @brief Default constructor (unit sphere at origin).
    SphereCollider()
        : center(0, 0, 0), radius(1.0f), layer(static_cast<u32>(PhysicsLayer::DEFAULT)),
          isTrigger(false), isEnabled(true) {}

    /**
     * @brief Constructs a sphere collider.
     * @param r Radius.
     * @param centerOffset Local offset.
     * @param physLayer Physics layer.
     */
    explicit SphereCollider(f32 r, const Vec3& centerOffset = Vec3(0,0,0), u32 physLayer = static_cast<u32>(PhysicsLayer::DEFAULT))
        : center(centerOffset), radius(r), layer(physLayer), isTrigger(false), isEnabled(true) {}

    /**
     * @brief Gets the world-space bounding sphere.
     * @param worldPosition Entity world position.
     * @return World-space center and radius.
     */
    BoundingSphere GetWorldBounds(const Vec3& worldPosition) const {
        // For sphere, rotation doesn't affect the shape
        return { worldPosition + center, radius };
    }
};

// ==========================================
// STRUCT: BoxCollider
// ==========================================

/**
 * @struct BoxCollider
 * @brief Box collision shape (axis-aligned in local space).
 */
struct BoxCollider {
    Vec3 center;        ///< Local offset from entity position
    Vec3 halfExtents;   ///< Half-size along each axis
    u32 layer;          ///< Physics layer bitmask
    b8 isTrigger;       ///< If true, no collision response (events only)
    b8 isEnabled;       ///< Whether this collider is active

    /// @brief Default constructor (unit box at origin).
    BoxCollider()
        : center(0, 0, 0), halfExtents(0.5f, 0.5f, 0.5f),
          layer(static_cast<u32>(PhysicsLayer::DEFAULT)),
          isTrigger(false), isEnabled(true) {}

    /**
     * @brief Constructs a box collider.
     * @param halfSize Half-extents.
     * @param centerOffset Local offset.
     * @param physLayer Physics layer.
     */
    explicit BoxCollider(const Vec3& halfSize, const Vec3& centerOffset = Vec3(0,0,0),
                         u32 physLayer = static_cast<u32>(PhysicsLayer::DEFAULT))
        : center(centerOffset), halfExtents(halfSize), layer(physLayer),
          isTrigger(false), isEnabled(true) {}

    /**
     * @brief Gets the world-space axis-aligned bounding box.
     * @param worldPosition Entity world position.
     * @param worldScale Entity world scale.
     * @return Min and max corners of the AABB.
     */
    AABB GetWorldBounds(const Vec3& worldPosition, const Vec3& worldScale) const {
        Vec3 scaledHalf = Vec3(
            halfExtents.x * worldScale.x,
            halfExtents.y * worldScale.y,
            halfExtents.z * worldScale.z
        );
        Vec3 worldCenter = worldPosition + center;
        return { worldCenter - scaledHalf, worldCenter + scaledHalf };
    }
};

// ==========================================
// STRUCT: CapsuleCollider
// ==========================================

/**
 * @struct CapsuleCollider
 * @brief Capsule collision shape (cylinder with hemispherical ends, aligned with Y axis).
 */
struct CapsuleCollider {
    Vec3 center;    ///< Local offset from entity position
    f32 radius;     ///< Capsule radius
    f32 height;     ///< Height of the cylindrical part (not including caps)
    u32 layer;      ///< Physics layer bitmask
    b8 isTrigger;   ///< If true, no collision response (events only)
    b8 isEnabled;   ///< Whether this collider is active

    /// @brief Default constructor (unit capsule at origin).
    CapsuleCollider()
        : center(0, 0, 0), radius(0.5f), height(1.0f),
          layer(static_cast<u32>(PhysicsLayer::DEFAULT)),
          isTrigger(false), isEnabled(true) {}

    /**
     * @brief Constructs a capsule collider.
     * @param r Radius.
     * @param h Height of cylindrical section.
     * @param centerOffset Local offset.
     * @param physLayer Physics layer.
     */
    explicit CapsuleCollider(f32 r, f32 h, const Vec3& centerOffset = Vec3(0,0,0),
                             u32 physLayer = static_cast<u32>(PhysicsLayer::DEFAULT))
        : center(centerOffset), radius(r), height(h), layer(physLayer),
          isTrigger(false), isEnabled(true) {}

    /**
     * @brief Gets the world-space bounding sphere (conservative).
     * @param worldPosition Entity world position.
     * @param worldScale Entity world scale.
     * @return World-space center and radius.
     */
    BoundingSphere GetWorldBounds(const Vec3& worldPosition, const Vec3& worldScale) const {
        f32 scaledRadius = radius * Math::Max(worldScale.x, worldScale.z);
        f32 scaledHeight = height * worldScale.y;
        f32 totalRadius = scaledRadius + scaledHeight * 0.5f;
        return { worldPosition + center, totalRadius };
    }
};

} // namespace Dnd
