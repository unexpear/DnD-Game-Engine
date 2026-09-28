/**
 * @file collision.cpp
 * @brief Implementation of collision detection and contact generation.
 * 
 * See collision.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#include "physics/collision.h"
#include "core/log.h"
#include <algorithm>

namespace Dnd {

namespace Collision {

bool TestSphereSphere(const Vec3& centerA, f32 radiusA, const Vec3& centerB, f32 radiusB) {
    Vec3 diff = centerB - centerA;
    f32 distSq = diff.Dot(diff);
    f32 radiusSum = radiusA + radiusB;
    return distSq <= radiusSum * radiusSum;
}

bool TestSphereBox(const Vec3& center, f32 radius, const Vec3& min, const Vec3& max) {
    Vec3 closest = ClosestPointOnBox(min, max, center);
    Vec3 diff = center - closest;
    return diff.Dot(diff) <= radius * radius;
}

bool TestBoxBox(const Vec3& minA, const Vec3& maxA, const Vec3& minB, const Vec3& maxB) {
    // Check overlap on each axis
    if (maxA.x < minB.x || minA.x > maxB.x) return false;
    if (maxA.y < minB.y || minA.y > maxB.y) return false;
    if (maxA.z < minB.z || minA.z > maxB.z) return false;
    return true;
}

Contact GenerateSphereSphere(Entity entityA, Entity entityB,
                             const Vec3& centerA, f32 radiusA,
                             const Vec3& centerB, f32 radiusB) {
    Contact contact;
    contact.entityA = entityA;
    contact.entityB = entityB;

    Vec3 diff = centerB - centerA;
    f32 dist = diff.Length();

    if (dist > 0.0001f) {
        contact.normal = diff / dist;
    } else {
        contact.normal = Vec3(0, 1, 0); // Arbitrary direction if centers coincide
    }

    contact.penetration = (radiusA + radiusB) - dist;
    contact.point = centerA + contact.normal * radiusA;

    return contact;
}

Contact GenerateSphereBox(Entity entityA, Entity entityB,
                          const Vec3& center, f32 radius,
                          const Vec3& min, const Vec3& max) {
    Contact contact;
    contact.entityA = entityA;
    contact.entityB = entityB;

    Vec3 closest = ClosestPointOnBox(min, max, center);
    Vec3 diff = center - closest;
    f32 dist = diff.Length();

    if (dist > 0.0001f) {
        contact.normal = diff / dist;
    } else {
        // Sphere center is inside box — find the shortest axis to push out
        Vec3 boxCenter = (min + max) * 0.5f;
        diff = center - boxCenter;
        Vec3 halfSize = (max - min) * 0.5f;

        // Clamp the difference to the box extents
        Vec3 clampedDiff = Vec3(
            Math::Clamp(diff.x, -halfSize.x, halfSize.x),
            Math::Clamp(diff.y, -halfSize.y, halfSize.y),
            Math::Clamp(diff.z, -halfSize.z, halfSize.z)
        );

        // Find the axis with the least overlap
        Vec3 overlap = halfSize - Vec3(Math::Abs(clampedDiff.x), Math::Abs(clampedDiff.y), Math::Abs(clampedDiff.z));
        f32 minOverlap = Math::Min(overlap.x, Math::Min(overlap.y, overlap.z));

        if (minOverlap == overlap.x) contact.normal = Vec3(diff.x > 0 ? 1 : -1, 0, 0);
        else if (minOverlap == overlap.y) contact.normal = Vec3(0, diff.y > 0 ? 1 : -1, 0);
        else contact.normal = Vec3(0, 0, diff.z > 0 ? 1 : -1);
    }

    contact.penetration = radius - dist;
    contact.point = closest;

    return contact;
}

Contact GenerateBoxBox(Entity entityA, Entity entityB,
                       const Vec3& minA, const Vec3& maxA,
                       const Vec3& minB, const Vec3& maxB) {
    Contact contact;
    contact.entityA = entityA;
    contact.entityB = entityB;

    // Compute centers and half-sizes
    Vec3 centerA = (minA + maxA) * 0.5f;
    Vec3 centerB = (minB + maxB) * 0.5f;
    Vec3 halfA = (maxA - minA) * 0.5f;
    Vec3 halfB = (maxB - minB) * 0.5f;

    // Compute overlap on each axis
    Vec3 diff = centerB - centerA;
    f32 overlapX = halfA.x + halfB.x - Math::Abs(diff.x);
    f32 overlapY = halfA.y + halfB.y - Math::Abs(diff.y);
    f32 overlapZ = halfA.z + halfB.z - Math::Abs(diff.z);

    // Find the axis with minimum overlap
    f32 minOverlap = Math::Min(overlapX, Math::Min(overlapY, overlapZ));
    contact.penetration = minOverlap;

    // Determine the collision normal based on the minimum overlap axis
    if (minOverlap == overlapX) {
        contact.normal = Vec3(diff.x > 0 ? -1 : 1, 0, 0);
    } else if (minOverlap == overlapY) {
        contact.normal = Vec3(0, diff.y > 0 ? -1 : 1, 0);
    } else {
        contact.normal = Vec3(0, 0, diff.z > 0 ? -1 : 1);
    }

    // Compute contact point (average of the two closest points)
    Vec3 closestA = ClosestPointOnBox(minA, maxA, centerB);
    Vec3 closestB = ClosestPointOnBox(minB, maxB, centerA);
    contact.point = (closestA + closestB) * 0.5f;

    return contact;
}

Vec3 ClosestPointOnBox(const Vec3& min, const Vec3& max, const Vec3& point) {
    return Vec3(
        Math::Clamp(point.x, min.x, max.x),
        Math::Clamp(point.y, min.y, max.y),
        Math::Clamp(point.z, min.z, max.z)
    );
}

bool ShouldCollide(u32 layerA, u32 layerB) {
    // Default: layers collide if they share any bit
    // Exception: IGNORE layer (0) never collides
    if (layerA == 0 || layerB == 0) return false;
    return true; // Default: all non-ignore layers collide
}

bool IntersectRayAABB(const Ray& ray, const Vec3& min, const Vec3& max, f32& outT) {
    outT = -1.0f;

    f32 tMin = -1e30f;
    f32 tMax = 1e30f;

    // For each axis, compute slab intersection
    for (i32 axis = 0; axis < 3; ++axis) {
        f32 originComp = (&ray.origin.x)[axis];
        f32 dirComp    = (&ray.direction.x)[axis];
        f32 minComp    = (&min.x)[axis];
        f32 maxComp    = (&max.x)[axis];

        // Ray is parallel to this slab
        if (std::abs(dirComp) < 1e-10f) {
            if (originComp < minComp || originComp > maxComp) {
                return false; // Ray is outside the slab on this axis
            }
            // Inside, so no intersection on this axis
            continue;
        }

        f32 invDir = 1.0f / dirComp;
        f32 t0 = (minComp - originComp) * invDir;
        f32 t1 = (maxComp - originComp) * invDir;

        if (t0 > t1) {
            f32 temp = t0;
            t0 = t1;
            t1 = temp;
        }

        if (t0 > tMin) tMin = t0;
        if (t1 < tMax) tMax = t1;

        if (tMin > tMax) return false;
    }

    // Check for intersection in front of the ray (tMin >= 0)
    if (tMin < 0.0f) {
        // Ray origin is inside the box; use tMax
        if (tMax < 0.0f) return false;
        outT = tMax;
    } else {
        outT = tMin;
    }

    return true;
}

} // namespace Collision

} // namespace Dnd
