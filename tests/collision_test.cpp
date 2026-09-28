#include "core/math.h"
#include "physics/collision.h"
#include <cstdio>

using namespace Dnd;

static int g_failures = 0;

static void Check(bool condition, const char* description) {
    std::printf("  [%s] %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) ++g_failures;
}

int main() {
    // ==========================================
    // Ray-AABB intersection (slabs method)
    // ==========================================
    std::printf("-- Ray-AABB --\n");

    // Box from (0,0,0) to (2,2,2)
    const Vec3 aabbMin(0, 0, 0);
    const Vec3 aabbMax(2, 2, 2);

    // Ray pointing at the center from +Z
    f32 t;
    bool hit = Collision::IntersectRayAABB(
        Ray(Vec3(1, 1, 5), Vec3(0, 0, -1)), aabbMin, aabbMax, t);
    Check(hit, "ray from +Z hits AABB center");
    Check(t > 0 && t < 5, "  t is in (0,5)");

    // Ray pointing away -- no hit
    hit = Collision::IntersectRayAABB(
        Ray(Vec3(1, 1, 5), Vec3(0, 0, 1)), aabbMin, aabbMax, t);
    Check(!hit, "ray pointing away misses");

    // Ray starting inside the box
    hit = Collision::IntersectRayAABB(
        Ray(Vec3(1, 1, 1), Vec3(1, 0, 0)), aabbMin, aabbMax, t);
    Check(hit, "ray from inside the box hits");
    Check(t >= 0, "  t >= 0");

    // Glancing ray that clips the +X face
    hit = Collision::IntersectRayAABB(
        Ray(Vec3(-1, 1, 1), Vec3(1, 0, 0)), aabbMin, aabbMax, t);
    Check(hit, "grazing ray hits +X face");
    Check(t > 0, "  t > 0");

    // Ray far off to the side
    hit = Collision::IntersectRayAABB(
        Ray(Vec3(10, 10, 5), Vec3(0, 0, -1)), aabbMin, aabbMax, t);
    Check(!hit, "ray far off to the side misses");

    // ==========================================
    // Sphere-Sphere overlap
    // ==========================================
    std::printf("-- Sphere-Sphere --\n");

    Check(Collision::TestSphereSphere(Vec3(0,0,0), 2, Vec3(1,0,0), 2),
          "overlapping spheres");
    Check(!Collision::TestSphereSphere(Vec3(0,0,0), 1, Vec3(5,0,0), 1),
          "separated spheres");
    Check(Collision::TestSphereSphere(Vec3(0,0,0), 2, Vec3(4,0,0), 2),
          "touching spheres");

    Contact c = Collision::GenerateSphereSphere(
        Entity::None(), Entity::None(),
        Vec3(0,0,0), 2, Vec3(1,0,0), 2);
    Check(c.penetration > 0, "sphere-sphere contact has penetration > 0");

    // ==========================================
    // Sphere-Box overlap
    // ==========================================
    std::printf("-- Sphere-Box --\n");

    Check(Collision::TestSphereBox(Vec3(2.5f, 1, 1), 2, aabbMin, aabbMax),
          "sphere overlaps box from +X");
    Check(!Collision::TestSphereBox(Vec3(10, 10, 10), 1, aabbMin, aabbMax),
          "sphere far from box misses");
    Check(!Collision::TestSphereBox(Vec3(5, 1, 1), 1, aabbMin, aabbMax),
          "sphere near but not touching box");

    // ==========================================
    // Box-Box overlap
    // ==========================================
    std::printf("-- Box-Box --\n");

    Check(Collision::TestBoxBox(Vec3(0,0,0), Vec3(2,2,2), Vec3(1,1,1), Vec3(3,3,3)),
          "overlapping boxes");
    Check(!Collision::TestBoxBox(Vec3(0,0,0), Vec3(2,2,2), Vec3(5,5,5), Vec3(7,7,7)),
          "separated boxes");

    // ==========================================
    // ClosestPointOnBox
    // ==========================================
    std::printf("-- ClosestPointOnBox --\n");

    Vec3 cp = Collision::ClosestPointOnBox(aabbMin, aabbMax, Vec3(5, 1, 1));
    Check(cp == Vec3(2, 1, 1), "closest point clamped to box surface");

    // ==========================================
    // Summary
    // ==========================================
    std::printf("\n==== %s (%d failure%s) ====\n",
                g_failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
