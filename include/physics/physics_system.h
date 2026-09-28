/**
 * @file physics_system.h
 * @brief Physics system for rigid body dynamics and collision resolution.
 * 
 * Processes entities with RigidBody + Collider components.
 * Performs integration, collision detection, and impulse-based resolution.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#pragma once

#include "ecs/system.h"
#include "ecs/components/transform.h"
#include "ecs/components/rigidbody.h"
#include "physics/collider.h"
#include "physics/collision.h"
#include "physics/spatial_hash.h"
#include "physics/physics_config.h"

namespace Dnd {

/**
 * @class PhysicsSystem
 * @brief Runs physics simulation: integration, collision detection, and resolution.
 * 
 * Runs during the FIXED_UPDATE phase with a fixed timestep for deterministic results.
 * 
 * Per-step:
 *   1. Integrate rigid body velocities and positions
 *   2. Build spatial hash with collider bounding boxes
 *   3. Generate candidate pairs from spatial hash
 *   4. Run narrow-phase collision tests on candidates
 *   5. Resolve collisions with impulse-based response
 *   6. Draw debug shapes (if enabled)
 */
class PhysicsSystem : public System {
public:
    /**
     * @brief Constructs a PhysicsSystem.
     * @param config Physics configuration.
     */
    explicit PhysicsSystem(const PhysicsConfig& config = PhysicsConfig());

protected:
    void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) override;

private:
    /**
     * @brief Integrates all rigid bodies for one fixed timestep.
     * @param dt Fixed timestep.
     * @param em Entity manager.
     * @param cr Component registry.
     */
    void Integrate(f32 dt, EntityManager& em, ComponentRegistry& cr);

    /**
     * @brief Detects and resolves all collisions.
     * @param em Entity manager.
     * @param cr Component registry.
     */
    void DetectAndResolveCollisions(EntityManager& em, ComponentRegistry& cr);

    /**
     * @brief Resolves a single contact with impulse-based response.
     * @param contact The contact to resolve.
     * @param rbA Rigid body A.
     * @param rbB Rigid body B.
     * @param transformA Transform A.
     * @param transformB Transform B.
     */
    void ResolveContact(const Contact& contact, RigidBody& rbA, RigidBody& rbB,
                        Transform& transformA, Transform& transformB);

    /**
     * @brief Draws debug shapes for all colliders.
     * @param em Entity manager.
     * @param cr Component registry.
     */
    void DrawDebugShapes(EntityManager& em, ComponentRegistry& cr);

    PhysicsConfig m_config;     ///< Physics configuration
    SpatialHash m_spatialHash;  ///< Spatial hash for broad phase
    f32 m_accumulator;          ///< Time accumulator for fixed timestep
};

} // namespace Dnd
