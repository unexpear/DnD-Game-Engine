/**
 * @file rpg_hopefully_scene.cpp
 * @brief Practice yard for RPG Hopefully: walk a class, roll its check.
 *
 * See rpg_hopefully_scene.h.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/rpg_hopefully_scene.h"
#include "core/engine.h"
#include "core/input.h"
#include "core/log.h"
#include "ecs/components/camera.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/tag.h"
#include "ecs/components/transform.h"
#include "ecs/systems/render_system.h"
#include "ecs/systems/transform_system.h"
#include "game/dice.h"
#include "rendering/mesh.h"
#include "rendering/renderer.h"
#include "ui/ui_renderer.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace Dnd {

namespace {

Mesh* YardCube() {
    static Mesh* cube = nullptr;
    if (cube == nullptr) {
        cube = new Mesh(Mesh::CreateCube());
    }
    return cube;
}

Mesh* YardFloor() {
    static Mesh* floor = nullptr;
    if (floor == nullptr) {
        floor = new Mesh(Mesh::CreateQuad(14.0f, 14.0f));
    }
    return floor;
}

constexpr f32 kYard = 5.5f;

} // namespace

RpgHopefullyScene::RpgHopefullyScene(const CampaignManifest& manifest)
    : Scene("RpgHopefully")
    , m_manifest(manifest)
    , m_active(0)
    , m_lastDie(0)
    , m_lastTotal(0)
    , m_hasRoll(false)
    , m_lastSuccess(false) {
}

void RpgHopefullyScene::SelectFigure(i32 index) {
    if (m_figures.empty()) {
        return;
    }
    if (index < 0) {
        index = static_cast<i32>(m_figures.size()) - 1;
    }
    if (index >= static_cast<i32>(m_figures.size())) {
        index = 0;
    }
    m_active = index;
    m_hasRoll = false;
}

void RpgHopefullyScene::RollPractice() {
    if (m_figures.empty()) {
        return;
    }
    DiceRoller::Initialize();
    const Figure& figure = m_figures[static_cast<size_t>(m_active)];
    m_lastDie = DiceRoller::Roll(1, 20, 0);
    m_lastTotal = m_lastDie + figure.modifier;
    m_lastSuccess = m_lastTotal >= kPracticeDC;
    m_hasRoll = true;
    DND_LOG_INFO("RPG Hopefully %s rolls d20 %d %+d = %d vs DC %d (%s)",
                 figure.name.c_str(), m_lastDie, figure.modifier, m_lastTotal, kPracticeDC,
                 m_lastSuccess ? "success" : "miss");
}

void RpgHopefullyScene::OnLoad() {
    Scene::OnLoad();
    DiceRoller::Initialize();

    m_systemManager.RegisterSystem(std::make_unique<TransformSystem>(),
                                   m_entityManager, m_componentRegistry);
    m_systemManager.RegisterSystem(std::make_unique<RenderSystem>(nullptr),
                                   m_entityManager, m_componentRegistry);

    const Vec3 eye(0.0f, 9.0f, 11.0f);
    Entity camera = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(camera, Tag("MainCamera"));
    m_componentRegistry.AddComponent<Transform>(camera, Transform(
        eye, Quat::LookAt(eye, Vec3(0, 0, 0), Vec3(0, 1, 0)), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<ECSCamera>(camera, ECSCamera());

    Entity ground = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(ground, Tag("PracticeYard"));
    m_componentRegistry.AddComponent<Transform>(ground, Transform(
        Vec3(0, 0, 0), Quat::Identity(), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<MeshRenderer>(ground, MeshRenderer(
        YardFloor(), Color(0.34f, 0.42f, 0.28f, 1.0f)));

    const Vec3 posts[4] = {
        Vec3(-5.2f, 0.7f, -5.2f), Vec3(5.2f, 0.7f, -5.2f),
        Vec3(-5.2f, 0.7f, 5.2f), Vec3(5.2f, 0.7f, 5.2f)
    };
    for (const Vec3& post : posts) {
        Entity marker = m_entityManager.Create();
        m_componentRegistry.AddComponent<Tag>(marker, Tag("Post"));
        m_componentRegistry.AddComponent<Transform>(marker, Transform(
            post, Quat::Identity(), Vec3(0.28f, 1.4f, 0.28f)));
        m_componentRegistry.AddComponent<MeshRenderer>(marker, MeshRenderer(
            YardCube(), Color(0.42f, 0.30f, 0.18f, 1.0f)));
    }

    Entity dummy = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(dummy, Tag("PracticeDummy"));
    m_componentRegistry.AddComponent<Transform>(dummy, Transform(
        Vec3(0.0f, 0.7f, -3.2f), Quat::Identity(), Vec3(0.7f, 1.4f, 0.45f)));
    m_componentRegistry.AddComponent<MeshRenderer>(dummy, MeshRenderer(
        YardCube(), Color(0.55f, 0.28f, 0.22f, 1.0f)));

    struct Seed { const char* name; const char* ability; i32 mod; f32 r, g, b; f32 x; };
    const Seed seeds[4] = {
        {"Hopeful", "CHA", 2, 0.86f, 0.55f, 0.22f, -2.4f},
        {"Warden", "STR", 3, 0.30f, 0.50f, 0.82f, -0.8f},
        {"Hexer", "INT", 3, 0.55f, 0.28f, 0.72f, 0.8f},
        {"Chorus", "WIS", 1, 0.85f, 0.78f, 0.35f, 2.4f},
    };
    for (const Seed& seed : seeds) {
        Figure figure;
        figure.name = seed.name;
        figure.ability = seed.ability;
        figure.modifier = seed.mod;
        figure.r = seed.r;
        figure.g = seed.g;
        figure.b = seed.b;
        figure.yaw = 3.14159265f;
        figure.entity = m_entityManager.Create();
        m_componentRegistry.AddComponent<Tag>(figure.entity, Tag(seed.name));
        m_componentRegistry.AddComponent<Transform>(figure.entity, Transform(
            Vec3(seed.x, 0.55f, 1.4f), Quat::Identity(), Vec3(0.5f, 1.1f, 0.5f)));
        m_componentRegistry.AddComponent<MeshRenderer>(figure.entity, MeshRenderer(
            YardCube(), Color(seed.r, seed.g, seed.b, 1.0f)));
        m_figures.push_back(figure);
    }

    Spotlight key;
    key.position = Vec3(0.0f, 10.0f, 4.0f);
    key.direction = Vec3(0.0f, -1.0f, -0.25f).Normalized();
    key.color = Vec3(1.05f, 0.98f, 0.86f);
    key.cosInner = 0.75f;
    key.cosOuter = 0.35f;
    key.range = 32.0f;
    Renderer::SetSpotlights(&key, 1);
    Renderer::SetOccluders(nullptr, nullptr, 0);

    DND_LOG_INFO("RPG Hopefully yard ready: %u classes", static_cast<u32>(m_figures.size()));
}

void RpgHopefullyScene::OnUnload() {
    Scene::OnUnload();
}

void RpgHopefullyScene::OnUpdate(f32 deltaTime) {
    Renderer::SetOccluders(nullptr, nullptr, 0);
    if (m_figures.empty()) {
        return;
    }

    if (!Input::IsTextInputActive() && Engine::GetControlScheme() != nullptr &&
        std::strcmp(Engine::GetControlScheme(), "rpg-hopefully") == 0) {
        if (Input::IsKeyPressed(KeyCode::TAB) || Input::IsKeyPressed(KeyCode::E)) {
            SelectFigure(m_active + 1);
        } else if (Input::IsKeyPressed(KeyCode::Q)) {
            SelectFigure(m_active - 1);
        }
        if (Input::IsKeyPressed(KeyCode::R) || Input::IsKeyPressed(KeyCode::SPACE)) {
            RollPractice();
        }

        const f32 viewYaw = Engine::GetViewYaw();
        f32 x = 0.0f;
        f32 z = 0.0f;
        if (Input::IsKeyDown(KeyCode::W)) { x += std::sin(viewYaw); z += std::cos(viewYaw); }
        if (Input::IsKeyDown(KeyCode::S)) { x -= std::sin(viewYaw); z -= std::cos(viewYaw); }
        if (Input::IsKeyDown(KeyCode::A)) { x -= std::cos(viewYaw); z += std::sin(viewYaw); }
        if (Input::IsKeyDown(KeyCode::D)) { x += std::cos(viewYaw); z -= std::sin(viewYaw); }
        const f32 len = std::sqrt(x * x + z * z);
        Figure& figure = m_figures[static_cast<size_t>(m_active)];
        Transform* body = m_componentRegistry.GetComponent<Transform>(figure.entity);
        if (body && len > 0.001f) {
            const f32 speed = 3.4f;
            body->position.x += (x / len) * speed * deltaTime;
            body->position.z += (z / len) * speed * deltaTime;
            if (body->position.x < -kYard) body->position.x = -kYard;
            if (body->position.x > kYard) body->position.x = kYard;
            if (body->position.z < -kYard) body->position.z = -kYard;
            if (body->position.z > kYard) body->position.z = kYard;
            figure.yaw = std::atan2(x, z);
        }
    }

    const Figure& active = m_figures[static_cast<size_t>(m_active)];
    const Transform* body = m_componentRegistry.GetComponent<Transform>(active.entity);
    const Vec3 pos = body ? body->position : Vec3(0.0f, 0.5f, 0.0f);
    Engine::SetCameraFocus(Vec3(pos.x, 0.5f, pos.z));
    Engine::SetViewSubject(pos, active.yaw);

    for (i32 i = 0; i < static_cast<i32>(m_figures.size()); ++i) {
        MeshRenderer* mesh = m_componentRegistry.GetComponent<MeshRenderer>(m_figures[static_cast<size_t>(i)].entity);
        Transform* xf = m_componentRegistry.GetComponent<Transform>(m_figures[static_cast<size_t>(i)].entity);
        if (!mesh || !xf) {
            continue;
        }
        const b8 selected = (i == m_active);
        xf->scale = selected ? Vec3(0.62f, 1.35f, 0.62f) : Vec3(0.48f, 1.05f, 0.48f);
        const Figure& figure = m_figures[static_cast<size_t>(i)];
        mesh->color = selected ? Color(figure.r, figure.g, figure.b, 1.0f)
                               : Color(figure.r * 0.55f, figure.g * 0.55f, figure.b * 0.55f, 1.0f);
    }
}

void RpgHopefullyScene::OnRenderOverlay(f32 screenW, f32 screenH) {
    (void)screenW;
    (void)screenH;
    if (m_figures.empty()) {
        return;
    }
    const Figure& figure = m_figures[static_cast<size_t>(m_active)];
    const char* view = Engine::GetViewMode() == ViewMode::Immersed ? "inside the yard" : "table";
    UIRenderer::DrawRect(16.0f, 16.0f, 520.0f, 118.0f, Vec4(0.08f, 0.07f, 0.06f, 0.86f));
    UIRenderer::DrawText(28.0f, 24.0f, "RPG Hopefully  practice yard",
                         Vec4(0.96f, 0.90f, 0.72f, 1.0f), 13.0f);

    char line[160];
    std::snprintf(line, sizeof(line), "%s   %s %+d   practice DC %d   view: %s",
                  figure.name.c_str(), figure.ability.c_str(), figure.modifier, kPracticeDC, view);
    UIRenderer::DrawText(28.0f, 48.0f, line, Vec4(0.90f, 0.90f, 0.84f, 1.0f), 9.0f);

    if (m_hasRoll) {
        std::snprintf(line, sizeof(line), "d20 %d %+d = %d  %s",
                      m_lastDie, figure.modifier, m_lastTotal,
                      m_lastSuccess ? "the dummy staggers" : "the swing goes wide");
    } else {
        std::snprintf(line, sizeof(line), "No roll yet.");
    }
    UIRenderer::DrawText(28.0f, 70.0f, line, Vec4(1.0f, 0.92f, 0.70f, 1.0f), 10.0f);
    UIRenderer::DrawText(28.0f, 96.0f, "WASD walk   Q/E class   R roll   F3 view   F8 campaigns",
                         Vec4(0.75f, 0.78f, 0.72f, 1.0f), 8.0f);
}

} // namespace Dnd
