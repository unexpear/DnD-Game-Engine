/**
 * @file physics_demo_scene.h
 * @brief Physics demo scene for testing the physics system.
 * 
 * Creates entities with RigidBody + Collider components to demonstrate:
 *   - Gravity and falling objects
 *   - Sphere and box collisions
 *   - Collision response (impulse resolution)
 *   - Static vs dynamic bodies
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#pragma once

#include "scene/scene.h"

namespace Dnd {

/**
 * @class PhysicsDemoScene
 * @brief A demo scene testing physics simulation.
 * 
 * Creates:
 *   - A camera entity positioned to view the scene
 *   - A static floor (box collider + static rigid body)
 *   - Several falling spheres (sphere collider + dynamic rigid body)
 *   - A falling box (box collider + dynamic rigid body)
 *   - Debug shapes for collision visualization
 */
class PhysicsDemoScene : public Scene {
public:
    PhysicsDemoScene();

protected:
    void OnLoad() override;
    void OnUnload() override;

private:
    // Mesh IDs (only need cube and floor)
    u32 m_cubeMeshId;
    u32 m_floorMeshId;

    // Entity handles
    Entity m_cameraEntity;
    Entity m_floorEntity;
    Entity m_sphereEntities[6];
    Entity m_boxEntity;
};

} // namespace Dnd
