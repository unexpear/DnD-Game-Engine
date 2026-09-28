/**
 * @file physics_config.h
 * @brief Physics system configuration and settings.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#pragma once

#include "core/math.h"

namespace Dnd {

/**
 * @struct PhysicsConfig
 * @brief Global physics settings.
 */
struct PhysicsConfig {
    Vec3 gravity;           ///< Global gravity vector (default: 0, -9.81, 0)
    f32 fixedTimestep;      ///< Fixed update interval in seconds (default: 1/60)
    i32 velocityIterations; ///< Number of velocity solver iterations (default: 10)
    i32 positionIterations; ///< Number of position solver iterations (default: 5)
    f32 slop;               ///< Penetration allowance in meters (default: 0.01)
    f32 percent;            ///< Positional correction percentage (default: 0.2)
    f32 spatialHashCellSize;///< Size of spatial hash grid cells (default: 10)
    b8 enableDebugDraw;     ///< Whether to draw collision shapes (default: false)

    /// @brief Default constructor with sensible defaults.
    PhysicsConfig();
};

} // namespace Dnd
