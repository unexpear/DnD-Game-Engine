/**
 * @file transform_system.h
 * @brief System for computing world transforms from parent-child hierarchy.
 * 
 * Processes entities with Transform components and computes their world
 * matrices based on parent relationships.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "ecs/system.h"
#include "ecs/components/transform.h"

namespace Dnd {

/**
 * @class TransformSystem
 * @brief Computes world transforms for entities with parent-child relationships.
 * 
 * This system runs during the UPDATE phase. It iterates over all entities
 * with Transform components and ensures their world matrices are up to date.
 */
class TransformSystem : public System {
public:
    TransformSystem() : System("TransformSystem", SystemPhase::UPDATE, 10) {}

protected:
    void OnUpdate(f32 /*deltaTime*/, EntityManager& /*em*/, ComponentRegistry& /*cr*/) override;
};

} // namespace Dnd
