/**
 * @file demo_scene.cpp
 * @brief Implementation of the DemoScene class.
 * 
 * See demo_scene.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#include "scene/demo_scene.h"
#include "core/log.h"
#include "core/math.h"
#include "rendering/mesh.h"
#include "rendering/debug_draw.h"
#include "ecs/components/transform.h"
#include "ecs/components/camera.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/tag.h"
#include "ecs/systems/transform_system.h"
#include "ecs/systems/render_system.h"

namespace Dnd {

DemoScene::DemoScene()
    : Scene("DemoScene"),
      m_triangleMeshId(0), m_cubeMeshId(0), m_tableMeshId(0) {
}

void DemoScene::OnLoad() {
    Scene::OnLoad();

    // Step 1: Create meshes (stored as static for renderer access)
    static Mesh* s_triangleMesh = nullptr;
    static Mesh* s_cubeMesh = nullptr;
    static Mesh* s_tableMesh = nullptr;

    s_triangleMesh = new Mesh(Mesh::CreateTriangle());
    s_cubeMesh = new Mesh(Mesh::CreateCube());
    s_tableMesh = new Mesh(Mesh::CreateQuad(8.0f, 8.0f));

    // Step 2: Register systems
    m_systemManager.RegisterSystem(
        std::make_unique<TransformSystem>(),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::make_unique<RenderSystem>(nullptr),
        m_entityManager, m_componentRegistry);

    // Step 3: Create camera entity
    m_cameraEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_cameraEntity, Tag("MainCamera"));
    m_componentRegistry.AddComponent<Transform>(m_cameraEntity, Transform(
        Vec3(0, 5, 10), Quat::LookAt(Vec3(0, 5, 10), Vec3(0, 0, 0), Vec3(0, 1, 0)), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<ECSCamera>(m_cameraEntity, ECSCamera());

    // Step 4: Create table entity (green)
    m_tableEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_tableEntity, Tag("Table"));
    m_componentRegistry.AddComponent<Transform>(m_tableEntity, Transform(
        Vec3(0, 0, 0), Quat::Identity(), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<MeshRenderer>(m_tableEntity, MeshRenderer(s_tableMesh, Color(0, 0.4f, 0, 1)));

    // Step 5: Create cube entity (blue)
    m_cubeEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_cubeEntity, Tag("Cube"));
    m_componentRegistry.AddComponent<Transform>(m_cubeEntity, Transform(
        Vec3(2, 0.5f, 0), Quat::Identity(), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<MeshRenderer>(m_cubeEntity, MeshRenderer(s_cubeMesh, Color(0.2f, 0.4f, 0.9f, 1)));

    // Step 6: Create triangle entity (per-vertex colors, white tint)
    m_triangleEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_triangleEntity, Tag("Triangle"));
    m_componentRegistry.AddComponent<Transform>(m_triangleEntity, Transform(
        Vec3(-2, 2, 1), Quat::Identity(), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<MeshRenderer>(m_triangleEntity, MeshRenderer(s_triangleMesh, Color::White()));

    // Draw debug box around cube
    DebugDraw::DrawBox(Vec3(2.0f, 0.5f, 0.0f), Vec3(0.5f, 0.5f, 0.5f), Color(0.2f, 0.4f, 0.9f, 0.5f));

    DND_LOG_INFO("DemoScene entities created: %u total", m_entityManager.GetAliveCount());
}

void DemoScene::OnUnload() {
    Scene::OnUnload();
}

} // namespace Dnd
