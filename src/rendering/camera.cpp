/**
 * @file camera.cpp
 * @brief Implementation of the Camera class.
 * 
 * See camera.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "rendering/camera.h"
#include "core/log.h"

namespace Dnd {

// ==========================================
// CONSTRUCTION
// ==========================================

/**
 * @brief Default constructor.
 * 
 * Sets up a default perspective camera at (0, 5, 10) looking at origin
 * with 60-degree FOV.
 */
Camera::Camera()
    : m_position(0, 5, 10)
    , m_target(0, 0, 0)
    , m_up(0, 1, 0)
    , m_viewMatrix(Mat4::Identity())
    , m_viewDirty(true)
    , m_projectionMatrix(Mat4::Identity())
    , m_projectionType(ProjectionType::PERSPECTIVE)
    , m_fov(Math::DegreesToRadians(60.0f))
    , m_aspect(16.0f / 9.0f)
    , m_near(0.1f)
    , m_far(100.0f)
    , m_orthoSize(5.0f)
{
}

// ==========================================
// POSITION AND ORIENTATION
// ==========================================

void Camera::SetPosition(const Vec3& position) {
    m_position = position;
    m_viewDirty = true;
}

Vec3 Camera::GetPosition() const {
    return m_position;
}

void Camera::SetTarget(const Vec3& target) {
    m_target = target;
    m_viewDirty = true;
}

Vec3 Camera::GetTarget() const {
    return m_target;
}

void Camera::SetUpVector(const Vec3& up) {
    m_up = up;
    m_viewDirty = true;
}

Vec3 Camera::GetUpVector() const {
    return m_up;
}

void Camera::SetOrbit(f32 yaw, f32 pitch, f32 distance) {
    // Clamp pitch to prevent camera flipping at poles
    pitch = Math::Clamp(pitch, 0.01f, Math::PI - 0.01f);

    // Compute camera position from spherical coordinates
    // Yaw = rotation around Y axis, Pitch = angle from Y axis
    f32 sinPitch = std::sin(pitch);
    f32 cosPitch = std::cos(pitch);
    f32 sinYaw = std::sin(yaw);
    f32 cosYaw = std::cos(yaw);

    m_position.x = m_target.x + sinYaw * sinPitch * distance;
    m_position.y = m_target.y + cosPitch * distance;
    m_position.z = m_target.z + cosYaw * sinPitch * distance;

    m_viewDirty = true;
}

// ==========================================
// PROJECTION
// ==========================================

void Camera::SetPerspective(f32 fov, f32 aspect, f32 near, f32 far) {
    m_projectionType = ProjectionType::PERSPECTIVE;
    m_fov = fov;
    m_aspect = aspect;
    m_near = near;
    m_far = far;

    // Recompute projection matrix
    m_projectionMatrix = Mat4::Perspective(fov, aspect, near, far);
}

void Camera::SetOrthographic(f32 size, f32 aspect, f32 near, f32 far) {
    m_projectionType = ProjectionType::ORTHOGRAPHIC;
    m_orthoSize = size;
    m_aspect = aspect;
    m_near = near;
    m_far = far;

    f32 right = size * aspect;
    f32 left = -right;
    f32 top = size;
    f32 bottom = -top;

    m_projectionMatrix = Mat4::Orthographic(left, right, bottom, top, near, far);
}

ProjectionType Camera::GetProjectionType() const {
    return m_projectionType;
}

f32 Camera::GetFOV() const {
    return m_fov;
}

f32 Camera::GetNearPlane() const {
    return m_near;
}

f32 Camera::GetFarPlane() const {
    return m_far;
}

void Camera::SetAspect(f32 aspect) {
    m_aspect = aspect;

    // Recompute projection matrix with new aspect ratio
    if (m_projectionType == ProjectionType::PERSPECTIVE) {
        m_projectionMatrix = Mat4::Perspective(m_fov, m_aspect, m_near, m_far);
    } else {
        f32 right = m_orthoSize * m_aspect;
        f32 left = -right;
        m_projectionMatrix = Mat4::Orthographic(left, right, -m_orthoSize, m_orthoSize, m_near, m_far);
    }
}

// ==========================================
// MATRIX ACCESS
// ==========================================

Mat4 Camera::GetViewMatrix() {
    // Recompute if position, target, or up has changed
    if (m_viewDirty) {
        m_viewMatrix = Mat4::LookAt(m_position, m_target, m_up);
        m_viewDirty = false;
    }
    return m_viewMatrix;
}

Mat4 Camera::GetProjectionMatrix() const {
    return m_projectionMatrix;
}

void Camera::UpdateViewMatrix() {
    m_viewDirty = true;
}

} // namespace Dnd
