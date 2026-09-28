/**
 * @file shadow_test_scene.cpp
 * @brief Implementation of the ray-cast-shadow test scene.
 *
 * See shadow_test_scene.h.
 *
 * @author DND Engine Team
 * @date 2026-06-03
 */

#include "scene/shadow_test_scene.h"
#include "core/log.h"
#include "core/math.h"
#include "rendering/mesh.h"
#include "rendering/renderer.h"
#include "ecs/components/transform.h"
#include "ecs/components/camera.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/tag.h"
#include "ecs/systems/transform_system.h"
#include "ecs/systems/render_system.h"

#include <cmath>
#include <memory>

namespace Dnd {

ShadowTestScene::ShadowTestScene()
    : Scene("ShadowTestScene"),
      m_cubeMesh(nullptr), m_quadMesh(nullptr),
      m_cube(Entity::None()), m_camera(Entity::None()), m_lightAngle(0.0f) {
}

void ShadowTestScene::OnLoad() {
    Scene::OnLoad();

    m_cubeMesh = new Mesh(Mesh::CreateCube());
    m_quadMesh = new Mesh(Mesh::CreateQuad(24.0f, 24.0f));

    m_systemManager.RegisterSystem(std::make_unique<TransformSystem>(),
                                   m_entityManager, m_componentRegistry);
    m_systemManager.RegisterSystem(std::make_unique<RenderSystem>(nullptr),
                                   m_entityManager, m_componentRegistry);

    // Camera (its Transform is driven each frame by Engine's orbit controls).
    m_camera = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_camera, Tag("MainCamera"));
    m_componentRegistry.AddComponent<Transform>(m_camera, Transform(
        Vec3(0.0f, 8.0f, 14.0f), Quat::Identity(), Vec3(1.0f, 1.0f, 1.0f)));
    m_componentRegistry.AddComponent<ECSCamera>(m_camera, ECSCamera());

    // Floor.
    Entity floor = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(floor, Tag("Floor"));
    m_componentRegistry.AddComponent<Transform>(floor, Transform(
        Vec3(0.0f, 0.0f, 0.0f), Quat::Identity(), Vec3(1.0f, 1.0f, 1.0f)));
    m_componentRegistry.AddComponent<MeshRenderer>(floor,
        MeshRenderer(m_quadMesh, Color(0.35f, 0.35f, 0.40f, 1.0f)));

    // One large cube. The unit cube spans [-0.5, 0.5]; scaled x3 and raised so it
    // sits on the floor -> world AABB (-1.5, 0, -1.5) .. (1.5, 3, 1.5).
    m_cube = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_cube, Tag("BigCube"));
    m_componentRegistry.AddComponent<Transform>(m_cube, Transform(
        Vec3(0.0f, 1.5f, 0.0f), Quat::Identity(), Vec3(3.0f, 3.0f, 3.0f)));
    m_componentRegistry.AddComponent<MeshRenderer>(m_cube,
        MeshRenderer(m_cubeMesh, Color(0.70f, 0.70f, 0.75f, 1.0f)));

    // Register that cube's world AABB as the shadow occluder.
    const Vec3 boxMin(-1.5f, 0.0f, -1.5f);
    const Vec3 boxMax( 1.5f, 3.0f,  1.5f);
    Renderer::SetOccluders(&boxMin, &boxMax, 1);

    DND_LOG_INFO("ShadowTestScene loaded: 1 large cube + orbiting spotlight");
}

void ShadowTestScene::OnUnload() {
    Scene::OnUnload();
}

void ShadowTestScene::OnUpdate(f32 deltaTime) {
    // Slowly orbit the spotlight around the cube. Aim it at the top edge of the
    // side face that currently faces the light, so the cube throws a visible
    // shadow that sweeps across the floor as the light moves.
    m_lightAngle += deltaTime * 0.5f; // ~0.5 rad/s (a full loop ~12s)

    const f32 radius   = 7.0f;  // horizontal orbit radius
    const f32 lightY   = 6.0f;  // light height
    const f32 cubeHalf = 1.5f;  // cube half-extent
    const f32 cubeTop  = 3.0f;  // cube top

    const Vec3 horiz(std::cos(m_lightAngle), 0.0f, std::sin(m_lightAngle)); // center -> light
    const Vec3 lightPos(horiz.x * radius, lightY, horiz.z * radius);
    const Vec3 target(horiz.x * cubeHalf, cubeTop, horiz.z * cubeHalf);     // near top edge

    Spotlight s;
    s.position  = lightPos;
    s.direction = (target - lightPos).Normalized();
    s.color     = Vec3(2.0f, 1.8f, 1.5f); // warm, bright
    s.cosInner  = 0.965f; // ~15-degree inner cone
    s.cosOuter  = 0.930f; // ~22-degree outer cone
    s.range     = 16.0f;
    Renderer::SetSpotlights(&s, 1);
}

} // namespace Dnd
