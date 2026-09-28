/**
 * @file transform_system.cpp
 * @brief Implementation of the TransformSystem class.
 * 
 * See transform_system.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#include "ecs/systems/transform_system.h"

namespace Dnd {

void TransformSystem::OnUpdate(f32 /*deltaTime*/, EntityManager& /*em*/, ComponentRegistry& /*cr*/) {
    // Transform world matrix computation is lazy (computed on demand via GetWorldMatrix).
    // No per-frame processing needed unless we add caching.
    // Future optimization: cache world matrices and mark dirty on transform change.
}

} // namespace Dnd
