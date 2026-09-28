/**
 * @file physics_config.cpp
 * @brief Implementation of PhysicsConfig.
 * 
 * See physics_config.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#include "physics/physics_config.h"
#include "core/math.h"

namespace Dnd {

PhysicsConfig::PhysicsConfig()
    : gravity(0.0f, -9.81f, 0.0f)
    , fixedTimestep(1.0f / 60.0f)
    , velocityIterations(10)
    , positionIterations(5)
    , slop(0.01f)
    , percent(0.2f)
    , spatialHashCellSize(10.0f)
    , enableDebugDraw(false) {
}

} // namespace Dnd
