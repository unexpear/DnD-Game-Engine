/**
 * @file collision.h
 * @brief Collision detection and contact generation.
 * 
 * Provides narrow-phase collision tests between pairs of colliders
 * and generates contact data for collision response.
 * 
 * Supported tests:
 *   - Sphere vs Sphere
 *   - Sphere vs Box (AABB)
 *   - Box vs Box (AABB)
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include "ecs/entity.h"

#include "physics/collider.h"

namespace Dnd {

// ==========================================
// STRUCT: Contact
// ==========================================

/**
 * @struct Contact
 * @brief Contact point data between two colliders.
 * 
 * A contact represents a single point of collision between two objects.
 * Multiple contacts can exist per collision (e.g., box vs box can have
 * up to 8 contacts).
 */
struct Contact {
    Entity entityA;       ///< First entity
    Entity entityB;       ///< Second entity
    Vec3 normal;          ///< Collision normal (from A to B, normalized)
    Vec3 point;           ///< Contact point in world space
    f32 penetration;      ///< Depth of overlap (positive = overlapping)
    f32 restitution;      ///< Combined restitution
    f32 friction;         ///< Combined friction

    /// @brief Default constructor.
    Contact()
        : entityA(Entity::None()), entityB(Entity::None()),
          normal(0, 1, 0), point(0, 0, 0),
          penetration(0), restitution(0.5f), friction(0.5f) {}
};

// ==========================================
// NAMESPACE: Collision
// ==========================================

/**
 * @namespace Collision
 * @brief Collision detection and contact generation functions.
 */
namespace Collision {

/**
 * @brief Tests overlap between two spheres.
 * @param centerA Center of sphere A.
 * @param radiusA Radius of sphere A.
 * @param centerB Center of sphere B.
 * @param radiusB Radius of sphere B.
 * @return true if the spheres overlap.
 */
bool TestSphereSphere(const Vec3& centerA, f32 radiusA, const Vec3& centerB, f32 radiusB);

/**
 * @brief Tests overlap between a sphere and an AABB.
 * @param center Center of the sphere.
 * @param radius Radius of the sphere.
 * @param minMin Minimum corner of the AABB.
 * @param maxMax Maximum corner of the AABB.
 * @return true if the sphere overlaps the AABB.
 */
bool TestSphereBox(const Vec3& center, f32 radius, const Vec3& min, const Vec3& max);

/**
 * @brief Tests overlap between two AABBs.
 * @param minA Minimum corner of AABB A.
 * @param maxA Maximum corner of AABB A.
 * @param minB Minimum corner of AABB B.
 * @param maxB Maximum corner of AABB B.
 * @return true if the AABBs overlap.
 */
bool TestBoxBox(const Vec3& minA, const Vec3& maxA, const Vec3& minB, const Vec3& maxB);

/**
 * @brief Generates a contact between two overlapping spheres.
 * @param entityA First entity.
 * @param entityB Second entity.
 * @param centerA Center of sphere A.
 * @param radiusA Radius of sphere A.
 * @param centerB Center of sphere B.
 * @param radiusB Radius of sphere B.
 * @return Contact data (penetration > 0).
 */
Contact GenerateSphereSphere(Entity entityA, Entity entityB,
                             const Vec3& centerA, f32 radiusA,
                             const Vec3& centerB, f32 radiusB);

/**
 * @brief Generates a contact between an overlapping sphere and AABB.
 * @param entityA First entity (sphere).
 * @param entityB Second entity (box).
 * @param center Center of the sphere.
 * @param radius Radius of the sphere.
 * @param minMin Minimum corner of the AABB.
 * @param maxMax Maximum corner of the AABB.
 * @return Contact data (penetration > 0).
 */
Contact GenerateSphereBox(Entity entityA, Entity entityB,
                          const Vec3& center, f32 radius,
                          const Vec3& min, const Vec3& max);

/**
 * @brief Generates a contact between two overlapping AABBs.
 * @param entityA First entity.
 * @param entityB Second entity.
 * @param minA Minimum corner of AABB A.
 * @param maxA Maximum corner of AABB A.
 * @param minB Minimum corner of AABB B.
 * @param maxB Maximum corner of AABB B.
 * @return Contact data (penetration > 0).
 */
Contact GenerateBoxBox(Entity entityA, Entity entityB,
                       const Vec3& minA, const Vec3& maxA,
                       const Vec3& minB, const Vec3& maxB);

/**
 * @brief Computes the closest point on an AABB to a given point.
 * @param minMin Minimum corner of the AABB.
 * @param maxMax Maximum corner of the AABB.
 * @param point The query point.
 * @return Closest point on or inside the AABB.
 */
Vec3 ClosestPointOnBox(const Vec3& min, const Vec3& max, const Vec3& point);

/**
 * @brief Checks if two physics layers should collide.
 * @param layerA First entity's layer bitmask.
 * @param layerB Second entity's layer bitmask.
 * @return true if the layers should be tested for collision.
 */
bool ShouldCollide(u32 layerA, u32 layerB);

/**
 * @brief Tests a ray against an axis-aligned bounding box (slabs method).
 * @param ray The ray (origin + normalized direction).
 * @param min Minimum corner of the AABB.
 * @param max Maximum corner of the AABB.
 * @param outT Output: distance along the ray to the nearest intersection, or -1 if no hit.
 * @return true if the ray intersects the AABB.
 */
bool IntersectRayAABB(const Ray& ray, const Vec3& min, const Vec3& max, f32& outT);

} // namespace Collision

} // namespace Dnd
