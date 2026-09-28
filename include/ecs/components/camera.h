/**
 * @file camera.h
 * @brief Camera component for view/projection matrices.
 * 
 * Provides perspective and orthographic projection settings. The camera's
 * view matrix is computed from the entity's Transform component.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"

namespace Dnd {

/**
 * @struct ECSCamera
 * @brief Camera settings for view and projection.
 */
struct ECSCamera {
    f32 fov;          ///< Field of view in degrees (perspective)
    f32 nearPlane;    ///< Near clipping plane distance
    f32 farPlane;     ///< Far clipping plane distance
    f32 aspectRatio;  ///< Width / height ratio
    b8 isActive;      ///< Whether this camera is the active view camera
    b8 isOrthographic;///< Use orthographic projection

    /// @brief Default constructor (60 FOV, 0.1-1000 range, perspective, active).
    ECSCamera()
        : fov(60.0f), nearPlane(0.1f), farPlane(1000.0f),
          aspectRatio(16.0f / 9.0f), isActive(true), isOrthographic(false) {}

    /**
     * @brief Computes the projection matrix.
     * @return 4x4 projection matrix (perspective or orthographic).
     */
    Mat4 GetProjectionMatrix() const {
        if (isOrthographic) {
            f32 halfSize = 5.0f * aspectRatio;
            f32 halfHeight = 5.0f;
            return Mat4::Orthographic(-halfSize, halfSize, -halfHeight, halfHeight, nearPlane, farPlane);
        }
        return Mat4::Perspective(fov * 0.0174533f, aspectRatio, nearPlane, farPlane);
    }
};

} // namespace Dnd
