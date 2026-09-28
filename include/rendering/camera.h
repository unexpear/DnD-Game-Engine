/**
 * @file camera.h
 * @brief 3D camera system with perspective/orthographic projection.
 * 
 * Manages the camera's position, orientation, and projection matrices
 * for rendering 3D scenes. Supports both perspective and orthographic
 * projection modes.
 * 
 * Features:
 *   - Perspective projection (3D view)
 *   - Orthographic projection (2D/UI view)
 *   - LookAt targeting (position + target point)
 *   - Orbit controls (rotate around a target point)
 *   - Field of view adjustment
 *   - Near/far clipping plane configuration
 * 
 * @note The view matrix is computed lazily — only when requested.
 *       Call UpdateViewMatrix() after changing position or orientation.
 * @warning The projection matrix must be recomputed when the aspect ratio changes.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"

namespace Dnd {

// ==========================================
// ENUM: ProjectionType
// ==========================================

/**
 * @enum ProjectionType
 * @brief Camera projection mode.
 */
enum class ProjectionType : i32 {
    PERSPECTIVE,   ///< Perspective projection (3D view with foreshortening).
    ORTHOGRAPHIC,  ///< Orthographic projection (2D view, no perspective).
};

// ==========================================
// CLASS: Camera
// ==========================================

/**
 * @class Camera
 * @brief 3D camera with position, orientation, and projection.
 * 
 * The camera defines the viewpoint for rendering. It consists of:
 *   - Position: where the camera is in world space
 *   - Target: the point the camera is looking at
 *   - Up vector: which direction is "up" for the camera
 *   - Projection: how 3D space maps to the 2D screen
 * 
 * The camera produces two matrices:
 *   - View matrix: transforms world coordinates to camera coordinates
 *   - Projection matrix: transforms camera coordinates to screen coordinates
 * 
 * Usage:
 * @code
 * Camera camera;
 * camera.SetPosition(Vec3(0, 5, 10));
 * camera.SetTarget(Vec3(0, 0, 0));
 * camera.SetPerspective(Math::DegreesToRadians(60.0f), aspectRatio, 0.1f, 100.0f);
 * 
 * shader.SetUniform("u_view", camera.GetViewMatrix());
 * shader.SetUniform("u_projection", camera.GetProjectionMatrix());
 * @endcode
 */
class Camera {
public:
    /**
     * @brief Default constructor.
     * 
     * Initializes camera at (0, 5, 10), looking at origin,
     * with perspective projection (60° FOV, 0.1-100 range).
     */
    Camera();

    // ==========================================
    // POSITION AND ORIENTATION
    // ==========================================

    /**
     * @brief Sets the camera position in world space.
     * @param position New camera position.
     * @note Call UpdateViewMatrix() after changing position.
     */
    void SetPosition(const Vec3& position);

    /** @brief Gets the camera position. */
    Vec3 GetPosition() const;

    /**
     * @brief Sets the target point the camera is looking at.
     * @param target New look-at target in world space.
     * @note Call UpdateViewMatrix() after changing target.
     */
    void SetTarget(const Vec3& target);

    /** @brief Gets the look-at target. */
    Vec3 GetTarget() const;

    /**
     * @brief Sets the camera's up vector.
     * @param up Up direction vector. Default: (0, 1, 0).
     */
    void SetUpVector(const Vec3& up);

    /** @brief Gets the up vector. */
    Vec3 GetUpVector() const;

    /**
     * @brief Sets the camera orientation using orbit angles.
     * 
     * Useful for orbiting around a target point.
     * 
     * @param yaw Yaw angle in radians (rotation around Y axis).
     * @param pitch Pitch angle in radians (rotation around local X axis).
     * @param distance Distance from the target point.
     * 
     * Implementation:
     *   position.x = target.x + sin(yaw) * sin(pitch) * distance
     *   position.y = target.y + cos(pitch) * distance
     *   position.z = target.z + cos(yaw) * sin(pitch) * distance
     */
    void SetOrbit(f32 yaw, f32 pitch, f32 distance);

    // ==========================================
    // PROJECTION
    // ==========================================

    /**
     * @brief Sets perspective projection parameters.
     * 
     * @param fov Vertical field of view in radians.
     * @param aspect Viewport aspect ratio (width / height).
     * @param near Distance to near clipping plane. Must be > 0.
     * @param far Distance to far clipping plane. Must be > near.
     */
    void SetPerspective(f32 fov, f32 aspect, f32 near, f32 far);

    /**
     * @brief Sets orthographic projection parameters.
     * 
     * @param size Half-height of the viewing volume.
     * @param aspect Viewport aspect ratio (width / height).
     * @param near Distance to near clipping plane.
     * @param far Distance to far clipping plane.
     */
    void SetOrthographic(f32 size, f32 aspect, f32 near, f32 far);

    /** @brief Gets the current projection type. */
    ProjectionType GetProjectionType() const;

    /** @brief Gets the field of view (perspective only). */
    f32 GetFOV() const;

    /** @brief Gets the near clipping plane distance. */
    f32 GetNearPlane() const;

    /** @brief Gets the far clipping plane distance. */
    f32 GetFarPlane() const;

    /**
     * @brief Updates the aspect ratio and recomputes the projection matrix.
     * 
     * Call this when the window is resized.
     * 
     * @param aspect New aspect ratio (width / height).
     */
    void SetAspect(f32 aspect);

    // ==========================================
    // MATRIX ACCESS
    // ==========================================

    /**
     * @brief Computes and returns the view matrix.
     * 
     * Implementation:
     *   1. If view matrix is dirty (position/target/up changed):
     *      - Compute using Mat4::LookAt(position, target, up)
     *      - Mark as clean
     *   2. Return cached view matrix
     * 
     * @return 4x4 view matrix (world-to-camera transformation).
     */
    Mat4 GetViewMatrix();

    /**
     * @brief Gets the projection matrix.
     * @return 4x4 projection matrix (camera-to-screen transformation).
     */
    Mat4 GetProjectionMatrix() const;

    /** @brief Forces the view matrix to be recomputed on next GetViewMatrix() call. */
    void UpdateViewMatrix();

private:
    /** @brief Camera position in world space. */
    Vec3 m_position;
    /** @brief Point the camera is looking at. */
    Vec3 m_target;
    /** @brief Up direction vector. */
    Vec3 m_up;

    /** @brief Current view matrix. */
    Mat4 m_viewMatrix;
    /** @brief Whether the view matrix needs recomputation. */
    b8 m_viewDirty;

    /** @brief Current projection matrix. */
    Mat4 m_projectionMatrix;
    /** @brief Projection type (perspective or orthographic). */
    ProjectionType m_projectionType;

    /** @brief Perspective FOV in radians. */
    f32 m_fov;
    /** @brief Viewport aspect ratio. */
    f32 m_aspect;
    /** @brief Near clipping plane distance. */
    f32 m_near;
    /** @brief Far clipping plane distance. */
    f32 m_far;
    /** @brief Orthographic half-size (only for orthographic mode). */
    f32 m_orthoSize;
};

} // namespace Dnd
