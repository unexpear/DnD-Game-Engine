/**
 * @file physics_demo_scene.cpp
 * @brief Implementation of the PhysicsDemoScene class.
 * 
 * See physics_demo_scene.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#include "scene/physics_demo_scene.h"
#include "core/log.h"
#include "core/math.h"
#include "rendering/mesh.h"
#include "rendering/debug_draw.h"
#include "ecs/components/transform.h"
#include "ecs/components/camera.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/tag.h"
#include "ecs/components/rigidbody.h"
#include "ecs/systems/transform_system.h"
#include "ecs/systems/render_system.h"
#include "physics/physics_system.h"
#include "physics/collider.h"
#include "physics/physics_config.h"

namespace Dnd {

PhysicsDemoScene::PhysicsDemoScene()
    : Scene("PhysicsDemoScene"),
      m_cubeMeshId(0), m_floorMeshId(0),
      m_cameraEntity(Entity::None()), m_floorEntity(Entity::None()),
      m_boxEntity(Entity::None()) {
    for (i32 i = 0; i < 6; i++) {
        m_sphereEntities[i] = Entity::None();
    }
}

void PhysicsDemoScene::OnLoad() {
    Scene::OnLoad();

    // Step 1: Create meshes (stored as static for renderer access)
    static Mesh* s_cubeMesh = nullptr;
    static Mesh* s_floorMesh = nullptr;

    s_cubeMesh = new Mesh(Mesh::CreateCube());
    s_floorMesh = new Mesh(Mesh::CreateQuad(20.0f, 20.0f));

    // Step 2: Register systems
    m_systemManager.RegisterSystem(
        std::make_unique<TransformSystem>(),
        m_entityManager, m_componentRegistry);

    // Create physics system with debug visualization enabled
    PhysicsConfig config;
    config.enableDebugDraw = true;

    m_systemManager.RegisterSystem(
        std::make_unique<PhysicsSystem>(config),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::make_unique<RenderSystem>(nullptr),
        m_entityManager, m_componentRegistry);

    // Step 3: Create camera entity
    m_cameraEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_cameraEntity, Tag("MainCamera"));
    m_componentRegistry.AddComponent<Transform>(m_cameraEntity, Transform(
        Vec3(0, 5, 12), Quat::LookAt(Vec3(0, 5, 12), Vec3(0, 2, 0), Vec3(0, 1, 0)), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<ECSCamera>(m_cameraEntity, ECSCamera());

    // Step 4: Create floor entity (static, green)
    m_floorEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_floorEntity, Tag("Floor"));
    m_componentRegistry.AddComponent<Transform>(m_floorEntity, Transform(
        Vec3(0, -0.5f, 0), Quat::Identity(), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<MeshRenderer>(m_floorEntity, MeshRenderer(s_floorMesh, Color(0.3f, 0.5f, 0.3f, 1)));
    m_componentRegistry.AddComponent<RigidBody>(m_floorEntity, RigidBody::Static());
    m_componentRegistry.AddComponent<BoxCollider>(m_floorEntity, BoxCollider(Vec3(10.0f, 0.5f, 10.0f)));

    // Step 5: Create falling spheres (dynamic, various colors)
    Color sphereColors[] = {
        Color(0.9f, 0.2f, 0.2f, 1), // Red
        Color(0.2f, 0.9f, 0.2f, 1), // Green
        Color(0.2f, 0.2f, 0.9f, 1), // Blue
        Color(0.9f, 0.9f, 0.2f, 1), // Yellow
        Color(0.9f, 0.2f, 0.9f, 1), // Magenta
        Color(0.2f, 0.9f, 0.9f, 1)  // Cyan
    };

    for (i32 i = 0; i < 6; i++) {
        m_sphereEntities[i] = m_entityManager.Create();
        char tagBuf[32];
        snprintf(tagBuf, sizeof(tagBuf), "Sphere_%d", i);

        Vec3 pos = Vec3((i % 3) * 2.0f - 2.0f, 8.0f + i * 2.0f, (i / 3) * 2.0f - 1.0f);

        m_componentRegistry.AddComponent<Tag>(m_sphereEntities[i], Tag(tagBuf));
        m_componentRegistry.AddComponent<Transform>(m_sphereEntities[i], Transform(
            pos, Quat::Identity(), Vec3(1, 1, 1)));
        m_componentRegistry.AddComponent<MeshRenderer>(m_sphereEntities[i], MeshRenderer(s_cubeMesh, sphereColors[i]));
        m_componentRegistry.AddComponent<RigidBody>(m_sphereEntities[i], RigidBody(1.0f, 0.5f, false));
        m_componentRegistry.AddComponent<SphereCollider>(m_sphereEntities[i], SphereCollider(0.5f));
    }

    // Step 6: Create falling box (dynamic, orange)
    m_boxEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_boxEntity, Tag("FallingBox"));
    m_componentRegistry.AddComponent<Transform>(m_boxEntity, Transform(
        Vec3(0, 10.0f, 0), Quat::Identity(), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<MeshRenderer>(m_boxEntity, MeshRenderer(s_cubeMesh, Color(0.9f, 0.5f, 0.1f, 1)));
    m_componentRegistry.AddComponent<RigidBody>(m_boxEntity, RigidBody(2.0f, 0.3f, false));
    m_componentRegistry.AddComponent<BoxCollider>(m_boxEntity, BoxCollider(Vec3(0.5f, 0.5f, 0.5f)));

    DND_LOG_INFO("PhysicsDemoScene entities created: %u total", m_entityManager.GetAliveCount());
}

void PhysicsDemoScene::OnUnload() {
    Scene::OnUnload();
}

} // namespace Dnd
