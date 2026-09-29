/**
 * @file sectorbound_scene.cpp
 * @brief Block outpost for Sectorbound: walk a scout, scan the beacon.
 *
 * See sectorbound_scene.h.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/sectorbound_scene.h"
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

Mesh* BlockCube() {
    static Mesh* cube = nullptr;
    if (cube == nullptr) {
        cube = new Mesh(Mesh::CreateCube());
    }
    return cube;
}

constexpr f32 kYard = 3.65f;
constexpr f32 kScoutY = 0.77f;
const Vec3 kScoutHalf(0.26f, 0.50f, 0.26f);
const Vec3 kBeacon(0.0f, 0.68f, -0.6f);
const Vec3 kMarkerPos(-1.8f, 0.40f, 1.8f);
const Vec3 kGatePos(0.0f, 0.68f, -2.4f);
const Vec3 kGateOpen(0.0f, 2.20f, -2.4f);
const Vec3 kFarSide(0.0f, 0.40f, -3.45f);

f32 DistXZ(const Vec3& a, const Vec3& b) {
    const f32 dx = a.x - b.x;
    const f32 dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

} // namespace

SectorboundScene::SectorboundScene(const CampaignManifest& manifest)
    : Scene("Sectorbound")
    , m_manifest(manifest)
    , m_scout(Entity::None())
    , m_marker(Entity::None())
    , m_lamp(Entity::None())
    , m_gateIndex(-1)
    , m_yaw(3.14159265f)
    , m_markerStowed(false)
    , m_claimed(false)
    , m_hasScan(false)
    , m_lastSuccess(false)
    , m_lastDie(0)
    , m_lastTotal(0)
    , m_lastBonus(0) {
}

Entity SectorboundScene::AddProp(const Vec3& center, const Vec3& scale, const Color& color,
                                 const char* name, b8 solid) {
    Entity entity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(entity, Tag(name));
    m_componentRegistry.AddComponent<Transform>(entity, Transform(center, Quat::Identity(), scale));
    m_componentRegistry.AddComponent<MeshRenderer>(entity, MeshRenderer(BlockCube(), color));
    if (solid) {
        Block block;
        block.entity = entity;
        block.center = center;
        block.half = Vec3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f);
        block.solid = true;
        m_blocks.push_back(block);
    }
    return entity;
}

b8 SectorboundScene::HitsSolid(const Vec3& feet) const {
    for (const Block& block : m_blocks) {
        if (!block.solid) {
            continue;
        }
        if (std::fabs(feet.x - block.center.x) < kScoutHalf.x + block.half.x &&
            std::fabs(feet.y - block.center.y) < kScoutHalf.y + block.half.y &&
            std::fabs(feet.z - block.center.z) < kScoutHalf.z + block.half.z) {
            return true;
        }
    }
    return false;
}

void SectorboundScene::StowMarker() {
    m_markerStowed = true;
    if (Transform* marker = m_componentRegistry.GetComponent<Transform>(m_marker)) {
        marker->position = Vec3(0.0f, -3.0f, 0.0f);
        marker->scale = Vec3(0.05f, 0.05f, 0.05f);
    }
    DND_LOG_INFO("Sectorbound survey marker stowed (+%d scan)", kMarkerBonus);
}

void SectorboundScene::OpenGate() {
    m_claimed = true;
    if (m_gateIndex >= 0 && m_gateIndex < static_cast<i32>(m_blocks.size())) {
        Block& gate = m_blocks[static_cast<size_t>(m_gateIndex)];
        gate.solid = false;
        gate.center = kGateOpen;
        if (Transform* body = m_componentRegistry.GetComponent<Transform>(gate.entity)) {
            body->position = kGateOpen;
        }
    }
    if (MeshRenderer* lamp = m_componentRegistry.GetComponent<MeshRenderer>(m_lamp)) {
        lamp->color = Color(0.95f, 0.86f, 0.35f, 1.0f);
    }
}

void SectorboundScene::ScanBeacon() {
    DiceRoller::Initialize();
    const i32 bonus = m_markerStowed ? kMarkerBonus : 0;
    m_lastBonus = bonus;
    m_lastDie = DiceRoller::Roll(1, 20, 0);
    m_lastTotal = m_lastDie + bonus;
    m_lastSuccess = m_lastTotal >= kScanDC;
    m_hasScan = true;
    if (m_lastSuccess) {
        OpenGate();
    }
    DND_LOG_INFO("Sectorbound scan d20 %d %+d = %d vs DC %d (%s)",
                 m_lastDie, bonus, m_lastTotal, kScanDC,
                 m_lastSuccess ? "claimed" : "hold");
}

void SectorboundScene::OnLoad() {
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

    for (i32 x = -4; x <= 4; ++x) {
        for (i32 z = -4; z <= 4; ++z) {
            const b8 alt = ((x + z) & 1) == 0;
            const Color tile = alt ? Color(0.30f, 0.40f, 0.38f, 1.0f)
                                   : Color(0.22f, 0.30f, 0.32f, 1.0f);
            AddProp(Vec3(static_cast<f32>(x), 0.11f, static_cast<f32>(z)),
                    Vec3(0.94f, 0.22f, 0.94f), tile, "Floor", false);
        }
    }

    const Color wall(0.42f, 0.48f, 0.46f, 1.0f);
    const i32 wallX[8] = {-4, -3, -2, -1, 1, 2, 3, 4};
    for (i32 x : wallX) {
        AddProp(Vec3(static_cast<f32>(x), 0.68f, -2.4f),
                Vec3(0.92f, 0.92f, 0.92f), wall, "Wall", true);
    }
    const Entity gate = AddProp(kGatePos, Vec3(0.92f, 0.92f, 0.92f),
                                Color(0.72f, 0.55f, 0.22f, 1.0f), "Gate", true);
    m_gateIndex = static_cast<i32>(m_blocks.size()) - 1;
    (void)gate;

    AddProp(Vec3(2.4f, 0.68f, 1.6f), Vec3(0.92f, 0.92f, 0.92f),
            Color(0.48f, 0.34f, 0.22f, 1.0f), "Crate", true);
    AddProp(Vec3(3.2f, 0.68f, 2.3f), Vec3(0.92f, 0.92f, 0.92f),
            Color(0.40f, 0.28f, 0.18f, 1.0f), "Crate", true);
    AddProp(Vec3(2.4f, 1.58f, 1.6f), Vec3(0.88f, 0.88f, 0.88f),
            Color(0.55f, 0.40f, 0.24f, 1.0f), "Crate", true);
    AddProp(Vec3(-3.2f, 0.68f, -0.4f), Vec3(0.92f, 0.92f, 0.92f),
            Color(0.36f, 0.40f, 0.42f, 1.0f), "Crate", true);

    AddProp(kBeacon, Vec3(0.70f, 0.92f, 0.70f),
            Color(0.25f, 0.55f, 0.62f, 1.0f), "Beacon", true);
    AddProp(Vec3(0.0f, 1.48f, -0.6f), Vec3(0.42f, 0.70f, 0.42f),
            Color(0.35f, 0.70f, 0.74f, 1.0f), "Beacon", true);
    m_lamp = AddProp(Vec3(0.0f, 2.05f, -0.6f), Vec3(0.28f, 0.28f, 0.28f),
                     Color(0.55f, 0.85f, 0.78f, 1.0f), "BeaconLamp", false);

    m_marker = AddProp(kMarkerPos, Vec3(0.28f, 0.18f, 0.28f),
                       Color(0.90f, 0.78f, 0.28f, 1.0f), "SurveyMarker", false);
    AddProp(kFarSide, Vec3(0.55f, 0.18f, 0.55f),
            Color(0.62f, 0.32f, 0.28f, 1.0f), "FarSide", false);

    m_scout = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_scout, Tag("Scout"));
    m_componentRegistry.AddComponent<Transform>(m_scout, Transform(
        Vec3(0.0f, kScoutY, 2.4f), Quat::Identity(), Vec3(0.48f, 1.05f, 0.48f)));
    m_componentRegistry.AddComponent<MeshRenderer>(m_scout, MeshRenderer(
        BlockCube(), Color(0.42f, 0.82f, 0.70f, 1.0f)));

    Spotlight key;
    key.position = Vec3(0.0f, 10.0f, 3.0f);
    key.direction = Vec3(0.0f, -1.0f, -0.2f).Normalized();
    key.color = Vec3(0.90f, 1.00f, 0.95f);
    key.cosInner = 0.72f;
    key.cosOuter = 0.32f;
    key.range = 32.0f;
    Renderer::SetSpotlights(&key, 1);
    Renderer::SetOccluders(nullptr, nullptr, 0);

    DND_LOG_INFO("Sectorbound outpost ready");
}

void SectorboundScene::OnUnload() {
    Scene::OnUnload();
}

void SectorboundScene::OnUpdate(f32 deltaTime) {
    Renderer::SetOccluders(nullptr, nullptr, 0);
    Transform* body = m_componentRegistry.GetComponent<Transform>(m_scout);
    if (body == nullptr) {
        return;
    }

    if (!Input::IsTextInputActive() && Engine::GetControlScheme() != nullptr &&
        std::strcmp(Engine::GetControlScheme(), "sectorbound") == 0) {
        const f32 viewYaw = Engine::GetViewYaw();
        f32 x = 0.0f;
        f32 z = 0.0f;
        if (Input::IsKeyDown(KeyCode::W)) { x += std::sin(viewYaw); z += std::cos(viewYaw); }
        if (Input::IsKeyDown(KeyCode::S)) { x -= std::sin(viewYaw); z -= std::cos(viewYaw); }
        if (Input::IsKeyDown(KeyCode::A)) { x -= std::cos(viewYaw); z += std::sin(viewYaw); }
        if (Input::IsKeyDown(KeyCode::D)) { x += std::cos(viewYaw); z -= std::sin(viewYaw); }
        const f32 len = std::sqrt(x * x + z * z);
        if (len > 0.001f) {
            const f32 speed = 3.2f;
            const f32 stepX = (x / len) * speed * deltaTime;
            const f32 stepZ = (z / len) * speed * deltaTime;
            Vec3 next = body->position;
            next.x += stepX;
            if (next.x < -kYard) next.x = -kYard;
            if (next.x > kYard) next.x = kYard;
            if (HitsSolid(next)) {
                next.x = body->position.x;
            }
            next.z += stepZ;
            if (next.z < -kYard) next.z = -kYard;
            if (next.z > kYard) next.z = kYard;
            if (HitsSolid(next)) {
                next.z = body->position.z;
            }
            body->position = next;
            m_yaw = std::atan2(x, z);
        }

        if (Input::IsKeyPressed(KeyCode::F)) {
            const b8 nearMarker = !m_markerStowed && DistXZ(body->position, kMarkerPos) < 1.15f;
            const b8 nearBeacon = DistXZ(body->position, kBeacon) < 1.65f;
            if (nearMarker) {
                StowMarker();
            } else if (nearBeacon && !m_claimed) {
                ScanBeacon();
            }
        }
    }

    Engine::SetCameraFocus(Vec3(body->position.x, 0.5f, body->position.z));
    Engine::SetViewSubject(body->position, m_yaw);
}

void SectorboundScene::OnRenderOverlay(f32 screenW, f32 screenH) {
    (void)screenW;
    (void)screenH;
    const Transform* body = m_componentRegistry.GetComponent<Transform>(m_scout);
    const Vec3 pos = body ? body->position : Vec3(0.0f, kScoutY, 0.0f);
    const char* view = Engine::GetViewMode() == ViewMode::Immersed ? "inside the outpost" : "table";
    const i32 bonus = m_markerStowed ? kMarkerBonus : 0;
    const b8 nearMarker = !m_markerStowed && DistXZ(pos, kMarkerPos) < 1.15f;
    const b8 nearBeacon = DistXZ(pos, kBeacon) < 1.65f;
    const b8 onFarSide = m_claimed && DistXZ(pos, kFarSide) < 1.15f;

    UIRenderer::DrawRect(16.0f, 16.0f, 540.0f, 132.0f, Vec4(0.06f, 0.08f, 0.08f, 0.88f));
    UIRenderer::DrawText(28.0f, 24.0f, "Sectorbound  outpost",
                         Vec4(0.78f, 0.94f, 0.88f, 1.0f), 13.0f);

    char line[180];
    std::snprintf(line, sizeof(line), "Scout   survey %+d   scan DC %d   view: %s",
                  bonus, kScanDC, view);
    UIRenderer::DrawText(28.0f, 48.0f, line, Vec4(0.88f, 0.92f, 0.88f, 1.0f), 9.0f);

    if (onFarSide) {
        std::snprintf(line, sizeof(line), "Far side of the sector. The claim holds.");
    } else if (m_claimed && m_hasScan) {
        std::snprintf(line, sizeof(line), "d20 %d %+d = %d  beacon locks. The gate lifts.",
                      m_lastDie, m_lastBonus, m_lastTotal);
    } else if (m_hasScan) {
        std::snprintf(line, sizeof(line), "d20 %d %+d = %d  the gate stays shut.",
                      m_lastDie, m_lastBonus, m_lastTotal);
    } else if (m_markerStowed) {
        std::snprintf(line, sizeof(line), "Marker stowed. Scan the beacon.");
    } else {
        std::snprintf(line, sizeof(line), "No scan yet.");
    }
    UIRenderer::DrawText(28.0f, 70.0f, line, Vec4(0.96f, 0.92f, 0.72f, 1.0f), 10.0f);

    const char* hint = "Stow the gold marker, then scan the beacon.";
    if (nearMarker) {
        hint = "F stows the survey marker.";
    } else if (nearBeacon && !m_claimed) {
        hint = "F scans the beacon.";
    } else if (m_claimed) {
        hint = "Gate is open. Walk through to the far side.";
    }
    UIRenderer::DrawText(28.0f, 94.0f, hint, Vec4(0.80f, 0.88f, 0.82f, 1.0f), 8.0f);
    UIRenderer::DrawText(28.0f, 114.0f, "WASD walk   F interact   F3 view   F8 campaigns",
                         Vec4(0.70f, 0.78f, 0.74f, 1.0f), 8.0f);
}

} // namespace Dnd
