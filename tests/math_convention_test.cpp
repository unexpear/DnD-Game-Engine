/**
 * @file math_convention_test.cpp
 * @brief Headless unit tests that PIN the engine's matrix + camera conventions.
 *
 * Pure CPU — no GL, no window. Guards the exact class of bug that produced the
 * black screen (a transposed Quaternion::LookAt that inverted the camera and
 * pushed the whole scene off-screen). These conventions are now contractual:
 *   - Right-handed world space.
 *   - Column-major Mat4 storage: m[col*4 + row].
 *   - OpenGL clip space: NDC z in [-1, +1].
 *   - Camera looks down its local -Z axis.
 *
 * Build (from repo root, MSYS2 g++):
 *   g++ -std=c++17 -Wall -Wextra -Iinclude tests/math_convention_test.cpp -o build/math_test.exe
 *
 * Exit code: 0 = all pass, 1 = one or more failures.
 *
 * @author DND Engine Team
 * @date 2026-06-03
 */

#include "core/math.h"

#include <cstdio>
#include <cmath>

using namespace Dnd;

static int g_failures = 0;

static void check(bool ok, const char* description) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", description);
    if (!ok) ++g_failures;
}

static bool approx(f32 a, f32 b, f32 eps = 1.0e-3f) {
    return std::fabs(a - b) <= eps;
}

// Column-major Mat4 * (x,y,z,w) -> out[4]. Mirrors the m[col*4+row] convention.
static void mul(const Mat4& m, f32 x, f32 y, f32 z, f32 w, f32 out[4]) {
    out[0] = m.m[0] * x + m.m[4] * y + m.m[8]  * z + m.m[12] * w;
    out[1] = m.m[1] * x + m.m[5] * y + m.m[9]  * z + m.m[13] * w;
    out[2] = m.m[2] * x + m.m[6] * y + m.m[10] * z + m.m[14] * w;
    out[3] = m.m[3] * x + m.m[7] * y + m.m[11] * z + m.m[15] * w;
}

int main() {
    std::printf("-- 1. Mat4 basics --\n");
    Mat4 I;
    check(approx(I.m[0], 1) && approx(I.m[5], 1) && approx(I.m[10], 1) && approx(I.m[15], 1) &&
          approx(I.m[1], 0) && approx(I.m[4], 0),
          "default Mat4 is the identity matrix");

    // M * M^-1 == I for a non-trivial translate/rotate/scale.
    Mat4 M = Mat4::TRS(Vec3(3, -2, 5),
                       Quat::LookAt(Vec3(0, 0, 0), Vec3(1, 2, 3), Vec3(0, 1, 0)),
                       Vec3(2, 2, 2));
    Mat4 RT = M * M.Inverse();
    bool isIdentity = true;
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            isIdentity = isIdentity && approx(RT.m[c * 4 + r], (c == r) ? 1.0f : 0.0f, 1.0e-2f);
    check(isIdentity, "M * M.Inverse() == identity (TRS round-trip)");

    std::printf("\n-- 2. Perspective clip range (OpenGL z in [-1,1]) --\n");
    const f32 nearP = 0.1f, farP = 1000.0f;
    Mat4 proj = Mat4::Perspective(60.0f * 0.0174533f, 1280.0f / 720.0f, nearP, farP);
    f32 cNear[4], cFar[4];
    mul(proj, 0, 0, -nearP, 1, cNear); // a point on the near plane, in view space
    mul(proj, 0, 0, -farP,  1, cFar);  // a point on the far plane
    check(approx(cNear[2] / cNear[3], -1.0f, 2.0e-3f), "near plane maps to NDC z = -1");
    check(approx(cFar[2]  / cFar[3],  +1.0f, 2.0e-3f), "far plane maps to NDC z = +1");

    std::printf("\n-- 3. Camera regression guard (the transposed-LookAt bug) --\n");
    Vec3 eye(0, 8, 12), target(0, 0, 0), up(0, 1, 0);
    Quat q = Quat::LookAt(eye, target, up);
    Mat4 view = Mat4::TRS(eye, q, Vec3(1, 1, 1)).Inverse();

    f32 vpos[4];
    mul(view, 0, 0, 0, 1, vpos); // world origin transformed into view space
    check(vpos[2] < 0.0f, "world origin is IN FRONT of the camera (view-space z < 0)");
    check(approx(vpos[0], 0, 0.05f) && approx(vpos[1], 0, 0.05f),
          "world origin is centered in view (camera aims at it)");

    Mat4 VP = proj * view;
    f32 clip[4];
    mul(VP, 0, 0, 0, 1, clip);
    check(clip[3] > 0.0f, "origin has clip.w > 0 (not behind the camera)");
    check(std::fabs(clip[0] / clip[3]) <= 1.0f && std::fabs(clip[1] / clip[3]) <= 1.0f,
          "origin projects inside the screen (|NDC x,y| <= 1)");

    std::printf("\n-- 4. Camera forward actually points at the target --\n");
    Mat4 rot = q.ToMatrix();
    Vec3 fwd(-rot.m[8], -rot.m[9], -rot.m[10]); // camera looks down local -Z
    Vec3 want = (target - eye).Normalized();
    f32 dotFwd = fwd.x * want.x + fwd.y * want.y + fwd.z * want.z;
    check(approx(dotFwd, 1.0f, 1.0e-2f), "Quaternion::LookAt forward aims from eye toward target");

    std::printf("\n-- 5. Mat4::LookAt (direct view matrix) agrees --\n");
    Mat4 view2 = Mat4::LookAt(eye, target, up);
    f32 vpos2[4];
    mul(view2, 0, 0, 0, 1, vpos2);
    check(vpos2[2] < 0.0f, "Mat4::LookAt also puts the origin in front (view-space z < 0)");

    std::printf("\n==== %s (%d failure%s) ====\n",
                g_failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
