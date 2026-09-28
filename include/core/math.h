/**
 * @file math.h
 * @brief Core mathematics library: vectors, matrices, quaternions, and utility functions.
 * 
 * This module provides all mathematical types and operations used throughout the engine.
 * Design principles:
 *   - Right-handed coordinate system (X right, Y up, Z forward away from viewer)
 *   - Column-major matrix storage (OpenGL-compatible)
 *   - Matrices are applied right-to-left: M_final = M_projection * M_view * M_model
 *   - Angles are always in radians internally; conversion utilities provided
 *   - No dynamic allocation — all types are trivially copyable and POD-compatible
 *   - Quaternion rotations use the WXYZ convention (vector part xyz, scalar part w)
 * 
 * Coordinate system conventions:
 *   - World space: X = right, Y = up, Z = forward (away from viewer)
 *   - Screen space: X = right, Y = down (top-left origin)
 *   - NDC space: X = [-1,1] right, Y = [-1,1] up, Z = [0,1] depth (near to far)
 * 
 * @note All vector operations are component-wise unless explicitly documented (Dot, Cross, etc.)
 * @warning Matrix multiplication order matters! M * v means "transform v by M".
 *          Chaining: M_proj * M_view * M_model * v (rightmost applied first).
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include <cmath>

namespace Dnd {

// Forward declaration (defined after Mat4)
struct Quaternion;
using Quat = Quaternion;

// ==========================================
// STRUCT: Vec2
// ==========================================
// NAMESPACE: MATH UTILITIES
// ==========================================

/**
 * @namespace Math
 * @brief Free-standing mathematical utility functions and constants.
 * 
 * Contains constants (PI, TWO_PI), conversion functions (degrees/radians),
 * and utility functions (Clamp, Lerp) that operate on primitive types.
 */
namespace Math {

/** @brief Pi constant. Ratio of circumference to diameter. Precision: 20 decimal places. */
constexpr f32 PI = 3.14159265358979323846f;

/** @brief Two Pi constant (2pi). Full circle in radians. */
constexpr f32 TWO_PI = 6.28318530717958647692f;

/** @brief Conversion factor: degrees to radians. Multiply degrees by this to get radians. */
constexpr f32 DEG_TO_RAD = PI / 180.0f;

/** @brief Conversion factor: radians to degrees. Multiply radians by this to get degrees. */
constexpr f32 RAD_TO_DEG = 180.0f / PI;

/**
 * @brief Converts an angle from degrees to radians.
 * @param degrees Angle in degrees.
 * @return Angle in radians.
 */
inline f32 DegreesToRadians(f32 degrees) { return degrees * DEG_TO_RAD; }

/**
 * @brief Converts an angle from radians to degrees.
 * @param radians Angle in radians.
 * @return Angle in degrees.
 */
inline f32 RadiansToDegrees(f32 radians) { return radians * RAD_TO_DEG; }

/**
 * @brief Clamps a floating-point value to the range [min, max].
 * 
 * Returns min if value < min, max if value > max, otherwise value.
 * 
 * @param value The value to clamp.
 * @param min Lower bound (inclusive).
 * @param max Upper bound (inclusive).
 * @return The clamped value in [min, max].
 * @pre min <= max. Behavior is undefined if min > max.
 */
inline f32 Clamp(f32 value, f32 min, f32 max) {
    return value < min ? min : (value > max ? max : value);
}

/**
 * @brief Clamps a signed 32-bit integer to the range [min, max].
 * @param value The value to clamp.
 * @param min Lower bound (inclusive).
 * @param max Upper bound (inclusive).
 * @return The clamped value in [min, max].
 * @pre min <= max.
 */
inline i32 Clamp(i32 value, i32 min, i32 max) {
    return value < min ? min : (value > max ? max : value);
}

/**
 * @brief Returns the greater of two floating-point values.
 * @param a First value.
 * @param b Second value.
 * @return The larger of a and b.
 */
inline f32 Max(f32 a, f32 b) { return a > b ? a : b; }

/**
 * @brief Returns the greater of two signed 32-bit integers.
 * @param a First value.
 * @param b Second value.
 * @return The larger of a and b.
 */
inline i32 Max(i32 a, i32 b) { return a > b ? a : b; }

/**
 * @brief Returns the smaller of two floating-point values.
 * @param a First value.
 * @param b Second value.
 * @return The smaller of a and b.
 */
inline f32 Min(f32 a, f32 b) { return a < b ? a : b; }

/**
 * @brief Returns the smaller of two signed 32-bit integers.
 * @param a First value.
 * @param b Second value.
 * @return The smaller of a and b.
 */
inline i32 Min(i32 a, i32 b) { return a < b ? a : b; }

/**
 * @brief Returns the absolute value of a floating-point number.
 * @param value The value to get the absolute value of.
 * @return The absolute (non-negative) value.
 */
inline f32 Abs(f32 value) { return value < 0 ? -value : value; }

/**
 * @brief Returns the absolute value of a signed 32-bit integer.
 * @param value The value to get the absolute value of.
 * @return The absolute (non-negative) value.
 */
inline i32 Abs(i32 value) { return value < 0 ? -value : value; }

/**
 * @brief Linear interpolation between two values.
 * 
 * Computes a + (b - a) * t. When t=0, returns a. When t=1, returns b.
 * Values of t outside [0,1] perform extrapolation (no clamping).
 * 
 * @param a Start value (t=0).
 * @param b End value (t=1).
 * @param t Interpolation factor. 0=a, 1=b. Values outside [0,1] extrapolate.
 * @return Interpolated value.
 */
inline f32 Lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }

} // namespace Math

// ==========================================
// STRUCT: Vec2
// ==========================================

/**
 * @struct Vec2
 * @brief Two-dimensional vector for 2D positions, UV coordinates, and screen-space math.
 * 
 * A simple 2D vector with x and y components. Used primarily for:
 *   - 2D positions (screen coordinates, UI layout)
 *   - UV texture coordinates
 *   - 2D velocity and direction
 *   - Mouse position tracking
 * 
 * All operators are component-wise. The vector does NOT store magnitude
 * internally; Length() computes it on demand.
 */
struct Vec2 {
    /** @brief X component. Horizontal axis. Positive = right. */
    f32 x;
    /** @brief Y component. Vertical axis. Positive = up in world space, down in screen space. */
    f32 y;

    /** @brief Default constructor. Initializes to (0, 0). */
    Vec2() : x(0), y(0) {}
    /** @brief Parameterized constructor. */
    Vec2(f32 x, f32 y) : x(x), y(y) {}

    /** @brief Vector addition. Component-wise: (x+x, y+y). */
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    /** @brief Vector subtraction. Component-wise: (x-x, y-y). */
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    /** @brief Scalar multiplication. Each component multiplied by scalar. */
    Vec2 operator*(f32 scalar) const { return Vec2(x * scalar, y * scalar); }
    /** @brief Scalar division. Each component divided by scalar. Undefined if scalar=0. */
    Vec2 operator/(f32 scalar) const { return Vec2(x / scalar, y / scalar); }
    /** @brief Compound addition. */
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    /** @brief Compound subtraction. */
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    /** @brief Equality check. Exact comparison (no epsilon). */
    bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    /** @brief Inequality check. */
    bool operator!=(const Vec2& other) const { return !(*this == other); }

    /** @brief Computes the Euclidean length (magnitude). Returns sqrt(x^2 + y^2). */
    f32 Length() const { return std::sqrt(x * x + y * y); }
    /** @brief Computes the squared length. Faster than Length() (no sqrt). Returns x^2 + y^2. */
    f32 LengthSquared() const { return x * x + y * y; }
    /** @brief Returns a unit vector (length=1) in the same direction. Returns (0,0) if length is zero. */
    Vec2 Normalized() const { f32 len = Length(); return len > 0 ? *this / len : Vec2(0, 0); }
    /** @brief Computes the dot product. Returns x*other.x + y*other.y. */
    f32 Dot(const Vec2& other) const { return x * other.x + y * other.y; }
};

// ==========================================
// STRUCT: Vec3
// ==========================================

/**
 * @struct Vec3
 * @brief Three-dimensional vector for 3D positions, directions, colors, and physics.
 * 
 * A 3D vector with x, y, z components. The primary vector type used throughout
 * the engine for 3D world positions, direction vectors, RGB colors, rotation axes,
 * and physics calculations.
 * 
 * All operators are component-wise. Cross product uses the right-hand rule.
 */
struct Vec3 {
    /** @brief X component. Horizontal axis. Positive = right. */
    f32 x;
    /** @brief Y component. Vertical axis. Positive = up. */
    f32 y;
    /** @brief Z component. Depth axis. Positive = forward (away from viewer in right-handed system). */
    f32 z;

    /** @brief Default constructor. Initializes to (0, 0, 0). */
    Vec3() : x(0), y(0), z(0) {}
    /** @brief Parameterized constructor. */
    Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}

    /** @brief Vector addition. Component-wise. */
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    /** @brief Vector subtraction. Component-wise. */
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    /** @brief Scalar multiplication. */
    Vec3 operator*(f32 scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    /** @brief Scalar division. Undefined if scalar=0. */
    Vec3 operator/(f32 scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }
    /** @brief Component-wise multiplication (Hadamard product). NOT dot or cross product. */
    Vec3 operator*(const Vec3& other) const { return Vec3(x * other.x, y * other.y, z * other.z); }
    /** @brief Compound addition. */
    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    /** @brief Compound subtraction. */
    Vec3& operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    /** @brief Equality check. Exact comparison (no epsilon). */
    bool operator==(const Vec3& other) const { return x == other.x && y == other.y && z == other.z; }
    /** @brief Inequality check. */
    bool operator!=(const Vec3& other) const { return !(*this == other); }

    /** @brief Euclidean length. Returns sqrt(x^2 + y^2 + z^2). */
    f32 Length() const { return std::sqrt(x * x + y * y + z * z); }
    /** @brief Squared length. Faster than Length(). Returns x^2 + y^2 + z^2. */
    f32 LengthSquared() const { return x * x + y * y + z * z; }
    /** @brief Unit vector in same direction. Returns (0,0,0) if length is zero. */
    Vec3 Normalized() const { f32 len = Length(); return len > 0 ? *this / len : Vec3(0, 0, 0); }
    /** @brief Dot product. Returns x*ox + y*oy + z*oz. */
    f32 Dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }

    /**
     * @brief Cross product (vector product). Returns a vector perpendicular to both inputs.
     * 
     * Uses the right-hand rule: if index finger points along `this` and middle finger
     * along `other`, thumb points in the direction of the result.
     * Returns zero vector if vectors are parallel.
     * 
     * @param other The second vector.
     * @return Cross product vector (perpendicular to both inputs).
     */
    Vec3 Cross(const Vec3& other) const {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
};

// ==========================================
// STRUCT: Vec4
// ==========================================

/**
 * @struct Vec4
 * @brief Four-dimensional vector for homogeneous coordinates, quaternions, and RGBA colors.
 * 
 * Used for homogeneous coordinates (w for perspective division), quaternion storage
 * (x, y, z, w), RGBA color values (r, g, b, a), and SIMD vector operations.
 */
struct Vec4 {
    /** @brief X component. */
    f32 x;
    /** @brief Y component. */
    f32 y;
    /** @brief Z component. */
    f32 z;
    /** @brief W component. Homogeneous coordinate, quaternion scalar, or alpha. */
    f32 w;

    /** @brief Default constructor. Initializes to (0, 0, 0, 0). */
    Vec4() : x(0), y(0), z(0), w(0) {}
    /** @brief Parameterized constructor. */
    Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}

    /**
     * @brief Converts to Vec3 by discarding the w component.
     * @return Vec3(x, y, z). No perspective division performed.
     * @note If this Vec4 represents a homogeneous point, use TransformPoint() instead.
     */
    Vec3 ToVec3() const { return Vec3(x, y, z); }
};

// ==========================================
// STRUCT: Ray
// ==========================================

/**
 * @struct Ray
 * @brief A 3D ray with origin and normalized direction for picking/intersection.
 */
struct Ray {
    Vec3 origin;    ///< Ray start point (world space)
    Vec3 direction; ///< Normalized direction vector

    Ray() : origin(0, 0, 0), direction(0, 0, -1) {}
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}

    /// @brief Returns a point along the ray at parameter t: origin + direction * t.
    Vec3 GetPoint(f32 t) const { return origin + direction * t; }
};

// ==========================================
// STRUCT: Mat4
// ==========================================

/**
 * @struct Mat4
 * @brief 4x4 column-major transformation matrix for 3D transformations.
 * 
 * Storage is column-major (OpenGL convention):
 *   m[0] m[4] m[8]  m[12]   | right.x  up.x  forward.x  translate.x
 *   m[1] m[5] m[9]  m[13] = | right.y  up.y  forward.y  translate.y
 *   m[2] m[6] m[10] m[14]   | right.z  up.z  forward.z  translate.z
 *   m[3] m[7] m[11] m[15]   | 0        0     0          1
 * 
 * Matrix multiplication applies transformations right-to-left:
 *   M_final * v = M_projection * (M_view * (M_model * v))
 * 
 * @warning The default constructor creates an identity matrix, NOT a zero matrix.
 * @note This matrix is optimized for OpenGL. DirectX uses row-major by default.
 */
struct Mat4 {
    /** @brief Raw storage array. 16 floats in column-major order. m[column*4 + row]. */
    f32 m[16];

    /**
     * @brief Default constructor. Creates an identity matrix.
     * 
     * The identity matrix has 1s on the diagonal and 0s elsewhere.
     * Multiplying any matrix or vector by the identity returns the original unchanged.
     */
    Mat4() {
        for (i32 i = 0; i < 16; i++) m[i] = 0;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    /** @brief Creates an identity matrix. Same as default constructor. */
    static Mat4 Identity() { return Mat4(); }

    /**
     * @brief Creates a perspective projection matrix.
     * 
     * Transforms 3D world coordinates into NDC with perspective foreshortening.
     * Z values are non-linearly mapped to [0,1] — more precision near the camera.
     * 
     * @param fov Vertical field of view in radians. Typical: PI/4 (45 deg) to PI/3 (60 deg).
     * @param aspect Viewport aspect ratio (width / height).
     * @param near Distance to near clipping plane. Must be > 0. Typical: 0.1
     * @param far Distance to far clipping plane. Must be > near. Typical: 100.0
     * @return Perspective projection matrix.
     * @pre near > 0
     * @pre far > near
     * @warning If near is very small, z-fighting may occur.
     */
    static Mat4 Perspective(f32 fov, f32 aspect, f32 near, f32 far) {
        Mat4 result;
        f32 tanHalfFov = std::tan(fov / 2.0f);
        result.m[0] = 1.0f / (aspect * tanHalfFov);
        result.m[5] = 1.0f / tanHalfFov;
        result.m[10] = -(far + near) / (far - near);
        result.m[11] = -1.0f;
        result.m[14] = -(2.0f * far * near) / (far - near);
        result.m[15] = 0.0f;
        return result;
    }

    /**
     * @brief Creates a view (camera) matrix using the LookAt convention.
     * 
     * Positions the camera at `eye`, looking toward `target`, with `up` defining
     * the camera's up direction. Transforms world coordinates into camera space.
     * 
     * @param eye Camera position in world space.
     * @param target Point the camera is looking at in world space.
     * @param up Up direction vector. Must NOT be parallel to (target - eye).
     * @return View matrix (world-to-camera transformation).
     * @pre up must not be parallel to (target - eye).
     */
    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        Mat4 result;
        Vec3 f = (target - eye).Normalized();
        Vec3 r = f.Cross(up).Normalized();
        // Degenerate case: forward is parallel to up — pick an arbitrary right axis
        if (r.LengthSquared() < 0.001f) {
            r = (std::abs(f.y) < 0.9f) ? Vec3(0, 1, 0).Cross(f).Normalized()
                                       : Vec3(1, 0, 0).Cross(f).Normalized();
        }
        Vec3 u = r.Cross(f);

        result.m[0] = r.x; result.m[4] = r.y; result.m[8]  = r.z; result.m[12] = -r.Dot(eye);
        result.m[1] = u.x; result.m[5] = u.y; result.m[9]  = u.z; result.m[13] = -u.Dot(eye);
        result.m[2] = -f.x; result.m[6] = -f.y; result.m[10] = -f.z; result.m[14] = f.Dot(eye);
        result.m[3] = 0.0f; result.m[7] = 0.0f; result.m[11] = 0.0f; result.m[15] = 1.0f;
        return result;
    }

    /**
     * @brief Creates a translation matrix.
     * @param translation Translation offset (x, y, z).
     * @return Translation matrix.
     */
    static Mat4 Translate(const Vec3& translation) {
        Mat4 result;
        result.m[12] = translation.x;
        result.m[13] = translation.y;
        result.m[14] = translation.z;
        return result;
    }

    /**
     * @brief Creates a rotation matrix using Rodrigues' rotation formula.
     * 
     * Rotates points around the given axis by the specified angle (radians).
     * The axis is normalized internally. Positive angles rotate counter-clockwise
     * when looking along the axis toward the origin (right-hand rule).
     * 
     * @param angle Rotation angle in radians.
     * @param axis Axis of rotation. Will be normalized. Must not be zero vector.
     * @return Rotation matrix.
     * @pre axis must not be the zero vector.
     */
    static Mat4 Rotate(f32 angle, const Vec3& axis) {
        Mat4 result;
        f32 c = std::cos(angle);
        f32 s = std::sin(angle);
        f32 t = 1.0f - c;
        Vec3 a = axis.Normalized();

        result.m[0] = t * a.x * a.x + c;
        result.m[1] = t * a.x * a.y + s * a.z;
        result.m[2] = t * a.x * a.z - s * a.y;

        result.m[4] = t * a.x * a.y - s * a.z;
        result.m[5] = t * a.y * a.y + c;
        result.m[6] = t * a.y * a.z + s * a.x;

        result.m[8] = t * a.x * a.z + s * a.y;
        result.m[9] = t * a.y * a.z - s * a.x;
        result.m[10] = t * a.z * a.z + c;

        return result;
    }

    /**
     * @brief Creates a scaling matrix.
     * @param scale Scaling factors for x, y, z axes.
     * @return Scaling matrix.
     * @warning A scale factor of 0 collapses geometry. Negative values produce mirror reflections.
     */
    static Mat4 Scale(const Vec3& scale) {
        Mat4 result;
        result.m[0] = scale.x;
        result.m[5] = scale.y;
        result.m[10] = scale.z;
        return result;
    }

    /**
     * @brief Creates a combined translation-rotation-scale matrix.
     * @param position Translation offset.
     * @param rotation Quaternion rotation.
     * @param scale Scaling factors.
     * @return Combined TRS matrix.
     * @note Defined after Quaternion struct (requires full type).
     */
    static Mat4 TRS(const Vec3& position, const Quaternion& rotation, const Vec3& scale);

    /**
     * @brief Creates an orthographic projection matrix.
     * 
     * Transforms 3D coordinates into NDC without perspective foreshortening.
     * Used for UI rendering, minimaps, and 2D views.
     * 
     * @param left Left boundary of the viewing volume.
     * @param right Right boundary of the viewing volume.
     * @param bottom Bottom boundary of the viewing volume.
     * @param top Top boundary of the viewing volume.
     * @param near Distance to the near clipping plane.
     * @param far Distance to the far clipping plane.
     * @return Orthographic projection matrix.
     */
    static Mat4 Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
        Mat4 result;
        result.m[0] = 2.0f / (right - left);
        result.m[5] = 2.0f / (top - bottom);
        result.m[10] = -2.0f / (far - near);
        result.m[12] = -(right + left) / (right - left);
        result.m[13] = -(top + bottom) / (top - bottom);
        result.m[14] = -(far + near) / (far - near);
        return result;
    }

    /**
     * @brief Matrix multiplication.
     * 
     * Computes this * other using standard matrix multiplication.
     * The result represents applying `other`'s transformation FIRST, then `this`'s.
     * 
     * @param other The right-hand matrix.
     * @return Product matrix (this * other).
     */
    Mat4 operator*(const Mat4& other) const {
        Mat4 result;
        for (i32 i = 0; i < 4; i++) {
            for (i32 j = 0; j < 4; j++) {
                result.m[i * 4 + j] = 0;
                for (i32 k = 0; k < 4; k++) {
                    result.m[i * 4 + j] += m[k * 4 + j] * other.m[i * 4 + k];
                }
            }
        }
        return result;
    }

    /**
     * @brief Multiplies this 4x4 matrix by a Vec4 (vector * matrix, column-major).
     *
     * Computes result_j = sum_i(m[i*4 + j] * v[i]) for i,j in [0,3].
     * Equivalent to transforming the homogeneous vector by the matrix.
     *
     * @param v The Vec4 homogeneous vector.
     * @return Transformed Vec4.
     */
    Vec4 operator*(const Vec4& v) const {
        Vec4 result;
        result.x = m[0] * v.x + m[4] * v.y + m[8]  * v.z + m[12] * v.w;
        result.y = m[1] * v.x + m[5] * v.y + m[9]  * v.z + m[13] * v.w;
        result.z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w;
        result.w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w;
        return result;
    }

    /**
     * @brief Transforms a 3D point by this matrix with perspective division.
     * 
     * Multiplies the point by the matrix as a homogeneous vector (w=1),
     * then divides x, y, z by the resulting w component.
     * 
     * @param point The 3D point to transform.
     * @return Transformed 3D point after perspective division.
     * @warning If w=0, the result is undefined (division by zero).
     */
    Vec3 TransformPoint(const Vec3& point) const {
        f32 w = m[3] * point.x + m[7] * point.y + m[11] * point.z + m[15];
        return Vec3(
            (m[0] * point.x + m[4] * point.y + m[8]  * point.z + m[12]) / w,
            (m[1] * point.x + m[5] * point.y + m[9]  * point.z + m[13]) / w,
            (m[2] * point.x + m[6] * point.y + m[10] * point.z + m[14]) / w
        );
    }

    /**
     * @brief Computes the inverse of this matrix using Gauss-Jordan elimination.
     * @return Inverse matrix. Returns identity if matrix is singular.
     */
    Mat4 Inverse() const {
        Mat4 result;
        f32 inv[16];
        for (i32 i = 0; i < 16; i++) inv[i] = m[i];
        f32 tmp[16];
        for (i32 i = 0; i < 16; i++) tmp[i] = 0;
        tmp[0] = tmp[5] = tmp[10] = tmp[15] = 1.0f;

        for (i32 col = 0; col < 4; col++) {
            i32 pivot = col;
            for (i32 row = col + 1; row < 4; row++) {
                if (std::abs(inv[row * 4 + col]) > std::abs(inv[pivot * 4 + col])) pivot = row;
            }
            for (i32 j = 0; j < 4; j++) {
                f32 t = inv[col * 4 + j]; inv[col * 4 + j] = inv[pivot * 4 + j]; inv[pivot * 4 + j] = t;
                t = tmp[col * 4 + j]; tmp[col * 4 + j] = tmp[pivot * 4 + j]; tmp[pivot * 4 + j] = t;
            }
            f32 scale = inv[col * 4 + col];
            if (std::abs(scale) < 1e-10f) return Mat4();
            for (i32 j = 0; j < 4; j++) {
                inv[col * 4 + j] /= scale;
                tmp[col * 4 + j] /= scale;
            }
            for (i32 row = 0; row < 4; row++) {
                if (row == col) continue;
                f32 factor = inv[row * 4 + col];
                for (i32 j = 0; j < 4; j++) {
                    inv[row * 4 + j] -= factor * inv[col * 4 + j];
                    tmp[row * 4 + j] -= factor * tmp[col * 4 + j];
                }
            }
        }
        for (i32 i = 0; i < 16; i++) result.m[i] = tmp[i];
        return result;
    }
};

// ==========================================
// STRUCT: Quaternion
// ==========================================

/**
 * @struct Quaternion
 * @brief Unit quaternion for 3D rotation representation and interpolation.
 * 
 * Quaternions avoid gimbal lock (which affects Euler angles) and are more
 * efficient for interpolation than matrices. They use 16 bytes vs 64 for Mat4.
 * 
 * Storage order: XYZW (vector part xyz, scalar part w).
 * A quaternion q = (x, y, z, w) represents rotation of angle theta around axis (ax, ay, az):
 *   x = ax * sin(theta/2), y = ay * sin(theta/2), z = az * sin(theta/2), w = cos(theta/2)
 * 
 * @note Quaternions are NOT commutative: q1 * q2 != q2 * q1 in general.
 * @warning Always normalize after combining quaternions to prevent drift.
 */
struct Quaternion {
    /** @brief X component (vector part). Rotation around X axis. */
    f32 x;
    /** @brief Y component (vector part). Rotation around Y axis. */
    f32 y;
    /** @brief Z component (vector part). Rotation around Z axis. */
    f32 z;
    /** @brief W component (scalar part). cos(angle/2). */
    f32 w;

    /** @brief Default constructor. Creates identity quaternion (0, 0, 0, 1). No rotation. */
    Quaternion() : x(0), y(0), z(0), w(1) {}
    /** @brief Parameterized constructor. */
    Quaternion(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}

    /** @brief Returns the identity quaternion (no rotation). */
    static Quaternion Identity() { return Quaternion(0, 0, 0, 1); }

    /**
     * @brief Creates a quaternion that rotates from one direction to another.
     * @param from Starting direction (must be normalized).
     * @param to Target direction (must be normalized).
     * @return Quaternion representing the rotation.
     */
    static Quaternion FromToRotation(const Vec3& from, const Vec3& to) {
        f32 dot = from.Dot(to);
        if (dot > 0.99999f) {
            return Identity(); // Nearly identical directions
        }
        if (dot < -0.99999f) {
            // 180-degree rotation: find perpendicular axis
            Vec3 axis = from.Cross(Vec3(1, 0, 0));
            if (axis.Length() < 0.001f) {
                axis = from.Cross(Vec3(0, 1, 0));
            }
            axis = axis.Normalized();
            return Quaternion(axis.x, axis.y, axis.z, 0).Normalized();
        }
        Vec3 axis = from.Cross(to);
        f32 w = 1.0f + dot;
        Quaternion q(axis.x, axis.y, axis.z, w);
        return q.Normalized();
    }

    /**
     * @brief Creates a quaternion that looks at a target point.
     * @param position The observer position.
     * @param target The point to look at.
     * @param up The up direction.
     * @return Quaternion representing the look-at rotation.
     */
    static Quaternion LookAt(const Vec3& position, const Vec3& target, const Vec3& up) {
        Vec3 forward = (target - position).Normalized();
        Vec3 right = forward.Cross(up).Normalized();
        if (right.LengthSquared() < 0.001f) {
            right = (std::abs(forward.y) < 0.9f) ? Vec3(0, 1, 0).Cross(forward).Normalized()
                                                 : Vec3(1, 0, 0).Cross(forward).Normalized();
        }
        Vec3 actualUp = right.Cross(forward).Normalized();

        // Create rotation matrix columns
        Mat4 rotMat;
        // Camera basis vectors as COLUMNS = the camera-to-world rotation, which is
        // what the entity's Transform orientation expects. Previously these were
        // written as ROWS (the world-to-camera / view basis), which inverted the
        // camera's orientation and pushed the entire scene off-screen.
        rotMat.m[0] = right.x;    rotMat.m[1] = right.y;    rotMat.m[2] = right.z;
        rotMat.m[4] = actualUp.x; rotMat.m[5] = actualUp.y; rotMat.m[6] = actualUp.z;
        rotMat.m[8] = -forward.x; rotMat.m[9] = -forward.y; rotMat.m[10] = -forward.z;

        // Convert to quaternion
        Quaternion q;
        f32 trace = rotMat.m[0] + rotMat.m[5] + rotMat.m[10];
        if (trace > 0) {
            f32 s = 0.5f / std::sqrt(trace + 1.0f);
            q.w = 0.25f / s;
            q.x = (rotMat.m[6] - rotMat.m[9]) * s;
            q.y = (rotMat.m[8] - rotMat.m[2]) * s;
            q.z = (rotMat.m[1] - rotMat.m[4]) * s;
        } else {
            if (rotMat.m[5] > rotMat.m[0]) {
                f32 s = std::sqrt(rotMat.m[5] - rotMat.m[0] - rotMat.m[10] + 1.0f);
                q.y = 0.5f * s;
                q.w = (rotMat.m[8] - rotMat.m[2]) / (2.0f * s);
                q.x = (rotMat.m[4] + rotMat.m[1]) / (2.0f * s);
                q.z = (rotMat.m[9] + rotMat.m[6]) / (2.0f * s);
            } else if (rotMat.m[10] > rotMat.m[0]) {
                f32 s = std::sqrt(rotMat.m[10] - rotMat.m[0] - rotMat.m[5] + 1.0f);
                q.z = 0.5f * s;
                q.w = (rotMat.m[1] - rotMat.m[4]) / (2.0f * s);
                q.x = (rotMat.m[8] + rotMat.m[2]) / (2.0f * s);
                q.y = (rotMat.m[9] + rotMat.m[6]) / (2.0f * s);
            } else {
                f32 s = std::sqrt(rotMat.m[0] - rotMat.m[5] - rotMat.m[10] + 1.0f);
                q.x = 0.5f * s;
                q.w = (rotMat.m[6] - rotMat.m[9]) / (2.0f * s);
                q.y = (rotMat.m[4] + rotMat.m[1]) / (2.0f * s);
                q.z = (rotMat.m[8] + rotMat.m[2]) / (2.0f * s);
            }
        }
        return q.Normalized();
    }

    /**
     * @brief Creates a quaternion from Euler angles (pitch, yaw, roll).
     * 
     * Rotation order: yaw (Y) -> pitch (X) -> roll (Z).
     * 
     * @param pitch Rotation around X axis in radians (nose up/down).
     * @param yaw Rotation around Y axis in radians (turning left/right).
     * @param roll Rotation around Z axis in radians (tilting sideways).
     * @return Quaternion representing the combined rotation.
     */
    static Quaternion FromEulerAngles(f32 pitch, f32 yaw, f32 roll) {
        f32 cy = std::cos(yaw * 0.5f);
        f32 sy = std::sin(yaw * 0.5f);
        f32 cp = std::cos(pitch * 0.5f);
        f32 sp = std::sin(pitch * 0.5f);
        f32 cr = std::cos(roll * 0.5f);
        f32 sr = std::sin(roll * 0.5f);

        Quaternion q;
        q.w = cy * cp * cr + sy * sp * sr;
        q.x = cy * cp * sr - sy * sp * cr;
        q.y = sy * cp * sr + cy * sp * cr;
        q.z = sy * cp * cr - cy * sp * sr;
        return q;
    }

    /**
     * @brief Converts this quaternion to a 4x4 rotation matrix.
     * 
     * The resulting matrix has no translation or scaling — pure rotation.
     * @return 4x4 rotation matrix equivalent to this quaternion.
     * @note This quaternion should be normalized for correct results.
     */
    Mat4 ToMatrix() const {
        Mat4 result;
        f32 xx = x * x, yy = y * y, zz = z * z;
        f32 xy = x * y, xz = x * z, yz = y * z;
        f32 wx = w * x, wy = w * y, wz = w * z;

        result.m[0] = 1.0f - 2.0f * (yy + zz);
        result.m[1] = 2.0f * (xy + wz);
        result.m[2] = 2.0f * (xz - wy);

        result.m[4] = 2.0f * (xy - wz);
        result.m[5] = 1.0f - 2.0f * (xx + zz);
        result.m[6] = 2.0f * (yz + wx);

        result.m[8] = 2.0f * (xz + wy);
        result.m[9] = 2.0f * (yz - wx);
        result.m[10] = 1.0f - 2.0f * (xx + yy);

        return result;
    }

    /**
     * @brief Quaternion multiplication (composition of rotations).
     * 
     * Returns a quaternion representing applying `other`'s rotation FIRST,
     * then this quaternion's rotation. Result is NOT automatically normalized.
     * 
     * @param other The rotation to apply first.
     * @return Composed quaternion.
     */
    Quaternion operator*(const Quaternion& other) const {
        return Quaternion(
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        );
    }

    /** @brief Computes the magnitude (length). Unit quaternions have length 1. */
    f32 Length() const { return std::sqrt(x*x + y*y + z*z + w*w); }

    /** @brief Returns a normalized (unit-length) quaternion. Returns Identity() if length is zero. */
    Quaternion Normalized() const {
        f32 len = Length();
        return len > 0 ? Quaternion(x/len, y/len, z/len, w/len) : Identity();
    }
};

/// @brief Alias for Quaternion (shorter name).
using Quat = Quaternion;

// Out-of-line definition of Mat4::TRS (requires full Quaternion type)
inline Mat4 Mat4::TRS(const Vec3& position, const Quaternion& rotation, const Vec3& scale) {
    Mat4 rotMat = rotation.ToMatrix();
    Mat4 result;
    result.m[0] = rotMat.m[0] * scale.x;
    result.m[1] = rotMat.m[1] * scale.x;
    result.m[2] = rotMat.m[2] * scale.x;
    result.m[4] = rotMat.m[4] * scale.y;
    result.m[5] = rotMat.m[5] * scale.y;
    result.m[6] = rotMat.m[6] * scale.y;
    result.m[8] = rotMat.m[8] * scale.z;
    result.m[9] = rotMat.m[9] * scale.z;
    result.m[10] = rotMat.m[10] * scale.z;
    result.m[12] = position.x;
    result.m[13] = position.y;
    result.m[14] = position.z;
    return result;
}

// ==========================================
// STRUCT: Color
// ==========================================

/**
 * @struct Color
 * @brief Color representation in RGBA format with [0,1] component ranges.
 * 
 * All color components are in the range [0, 1] (linear color space).
 * Alpha: 0 = fully transparent, 1 = fully opaque.
 * 
 * @note Colors are stored in LINEAR space, not sRGB. Convert from sRGB before use
 *       if your textures are in sRGB (most image formats are).
 */
struct Color {
    /** @brief Red component. Range: [0, 1]. 0 = no red, 1 = full red. */
    f32 r;
    /** @brief Green component. Range: [0, 1]. */
    f32 g;
    /** @brief Blue component. Range: [0, 1]. */
    f32 b;
    /** @brief Alpha (opacity) component. Range: [0, 1]. 0 = transparent, 1 = opaque. */
    f32 a;

    /** @brief Default constructor. Creates white with full opacity: (1, 1, 1, 1). */
    Color() : r(1), g(1), b(1), a(1) {}
    /**
     * @brief Parameterized constructor.
     * @param r Red component [0, 1].
     * @param g Green component [0, 1].
     * @param b Blue component [0, 1].
     * @param a Alpha component [0, 1]. Defaults to 1.0 (fully opaque).
     */
    Color(f32 r, f32 g, f32 b, f32 a = 1.0f) : r(r), g(g), b(b), a(a) {}

    /** @brief Factory: pure white (1, 1, 1, 1). */
    static Color White() { return Color(1, 1, 1); }
    /** @brief Factory: pure black (0, 0, 0, 1). */
    static Color Black() { return Color(0, 0, 0); }
    /** @brief Factory: pure red (1, 0, 0, 1). */
    static Color Red() { return Color(1, 0, 0); }
    /** @brief Factory: pure green (0, 1, 0, 1). */
    static Color Green() { return Color(0, 1, 0); }
    /** @brief Factory: pure blue (0, 0, 1, 1). */
    static Color Blue() { return Color(0, 0, 1); }
    /** @brief Factory: grayscale value. v=0 is black, v=1 is white. Default: 0.5 (mid-gray). */
    static Color Gray(f32 v = 0.5f) { return Color(v, v, v); }
};

} // namespace Dnd
