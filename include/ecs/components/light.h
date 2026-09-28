/**
 * @file light.h
 * @brief Light component for scene illumination.
 * 
 * Supports three light types: Directional, Point, and Spot.
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
 * @enum LightType
 * @brief Types of light sources.
 */
enum class LightType : i32 {
    DIRECTIONAL = 0, ///< Infinite direction light (like the sun)
    POINT = 1,       ///< Omnidirectional point light
    SPOT = 2         ///< Cone-shaped spotlight
};

/**
 * @struct Light
 * @brief Light source properties.
 */
struct Light {
    LightType type;   ///< Type of light source
    Vec3 color;       ///< Light color (RGB, normalized)
    f32 intensity;    ///< Light brightness multiplier
    f32 range;        ///< Maximum distance (point/spot lights)
    f32 spotAngle;    ///< Cone angle in degrees (spot lights only)

    /// @brief Default constructor (directional white light, intensity 1).
    Light()
        : type(LightType::DIRECTIONAL), color(1, 1, 1),
          intensity(1.0f), range(0.0f), spotAngle(45.0f) {}
};

} // namespace Dnd
