/**
 * @file sandhold_scene.cpp
 * @brief Sandy courtyard for Sandhold: walk a warden, rest, brace the gate.
 *
 * See sandhold_scene.h.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/sandhold_scene.h"
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

Mesh* HoldCube() {
    static Mesh* cube = nullptr;
    if (cube == nullptr) {
        cube = new Mesh(Mesh::CreateCube());
    }
    return cube;
}

constexpr f32 kYard = 3.65f;
constexpr f32 kWardenY = 0.77f;
const Vec3 kWardenHalf(0.26f, 0.50f, 0.26f);
const Vec3 kFire(0.0f, 0.42f, 0.15f);
const Vec3 kGatePos(0.0f, 0.68f, -2.5f);
const Vec3 kGateOpen(0.0f, 2.15f, -2.5f);
const Vec3 kFarSide(0.15f, 0.36f, -3.45f);

f32 DistXZ(const Vec3& a, const Vec3& b) {
    const f32 dx = a.x - b.x;
    const f32 dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

} // namespace

SandholdScene::SandholdScene(const CampaignManifest& manifest)
    : Scene("Sandhold")
    , m_manifest(manifest)
    , m_warden(Entity::None())
    , m_fire(Entity::None())
    , m_gateIndex(-1)
    , m_yaw(3.14159265f)
    , m_rested(false)
    , m_held(false)
    , m_hasRoll(false)
    , m_lastSuccess(false)
    , m_lastDie(0)
    , m_lastTotal(0)
    , m_lastBonus(0) {
}

Entity SandholdScene::AddProp(const Vec3& center, const Vec3& scale, const Color& color,
                              const char* name, b8 solid) {
    Entity entity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(entity, Tag(name));
    m_componentRegistry.AddComponent<Transform>(entity, Transform(center, Quat::Identity(), scale));
    m_componentRegistry.AddComponent<MeshRenderer>(entity, MeshRenderer(HoldCube(), color));
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

b8 SandholdScene::HitsSolid(const Vec3& feet) const {
    for (const Block& block : m_blocks) {
        if (!block.solid) {
            continue;
        }
        if (std::fabs(feet.x - block.center.x) < kWardenHalf.x + block.half.x &&
            std::fabs(feet.y - block.center.y) < kWardenHalf.y + block.half.y &&
            std::fabs(feet.z - block.center.z) < kWardenHalf.z + block.half.z) {
            return true;
        }
    }
    return false;
}

void SandholdScene::RestAtFire() {
    m_rested = true;
    if (MeshRenderer* flame = m_componentRegistry.GetComponent<MeshRenderer>(m_fire)) {
        flame->color = Color(1.0f, 0.72f, 0.28f, 1.0f);
    }
    if (Transform* flame = m_componentRegistry.GetComponent<Transform>(m_fire)) {
        flame->scale = Vec3(0.38f, 0.62f, 0.38f);
    }
    DND_LOG_INFO("Sandhold warden rests at the fire (+%d brace)", kRestBonus);
}

void SandholdScene::OpenGate() {
    m_held = true;
    if (m_gateIndex >= 0 && m_gateIndex < static_cast<i32>(m_blocks.size())) {
        Block& gate = m_blocks[static_cast<size_t>(m_gateIndex)];
        gate.solid = false;
        gate.center = kGateOpen;
        if (Transform* body = m_componentRegistry.GetComponent<Transform>(gate.entity)) {
            body->position = kGateOpen;
        }
    }
}

void SandholdScene::BraceGate() {
    DiceRoller::Initialize();
    const i32 bonus = m_rested ? kRestBonus : 0;
    m_lastBonus = bonus;
    m_lastDie = DiceRoller::Roll(1, 20, 0);
    m_lastTotal = m_lastDie + bonus;
    m_lastSuccess = m_lastTotal >= kBraceDC;
    m_hasRoll = true;
    if (m_lastSuccess) {
        OpenGate();
    }
    DND_LOG_INFO("Sandhold brace d20 %d %+d = %d vs DC %d (%s)",
                 m_lastDie, bonus, m_lastTotal, kBraceDC,
                 m_lastSuccess ? "held" : "dust");
}

void SandholdScene::OnLoad() {
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
            const Color tile = alt ? Color(0.74f, 0.62f, 0.40f, 1.0f)
                                   : Color(0.64f, 0.52f, 0.32f, 1.0f);
            AddProp(Vec3(static_cast<f32>(x), 0.11f, static_cast<f32>(z)),
                    Vec3(0.94f, 0.22f, 0.94f), tile, "Sand", false);
        }
    }

    const Color dune(0.58f, 0.46f, 0.30f, 1.0f);
    for (i32 z = -1; z <= 2; ++z) {
        AddProp(Vec3(-3.35f, 0.42f, static_cast<f32>(z)),
                Vec3(1.15f, 0.48f, 0.90f), dune, "Dune", true);
        AddProp(Vec3(3.35f, 0.42f, static_cast<f32>(z)),
                Vec3(1.15f, 0.48f, 0.90f), dune, "Dune", true);
    }

    const Color wall(0.52f, 0.42f, 0.30f, 1.0f);
    const i32 wallX[8] = {-4, -3, -2, -1, 1, 2, 3, 4};
    for (i32 x : wallX) {
        AddProp(Vec3(static_cast<f32>(x), 0.68f, -2.5f),
                Vec3(0.92f, 0.92f, 0.92f), wall, "Wall", true);
    }
    AddProp(kGatePos, Vec3(0.92f, 0.92f, 0.92f),
            Color(0.62f, 0.48f, 0.34f, 1.0f), "DustGate", true);
    m_gateIndex = static_cast<i32>(m_blocks.size()) - 1;

    AddProp(Vec3(-2.15f, 0.55f, 1.55f), Vec3(0.72f, 0.66f, 0.72f),
            Color(0.48f, 0.34f, 0.22f, 1.0f), "Crate", true);
    AddProp(Vec3(1.85f, 0.72f, 1.45f), Vec3(0.18f, 1.05f, 0.18f),
            Color(0.36f, 0.26f, 0.16f, 1.0f), "TentPost", true);
    AddProp(Vec3(2.85f, 0.72f, 1.45f), Vec3(0.18f, 1.05f, 0.18f),
            Color(0.36f, 0.26f, 0.16f, 1.0f), "TentPost", true);
    AddProp(Vec3(2.35f, 1.32f, 1.45f), Vec3(1.45f, 0.10f, 1.05f),
            Color(0.62f, 0.46f, 0.30f, 1.0f), "Tent", false);
    AddProp(Vec3(-3.15f, 0.85f, -0.15f), Vec3(0.12f, 1.15f, 0.12f),
            Color(0.32f, 0.24f, 0.16f, 1.0f), "Torch", true);
    AddProp(Vec3(-3.15f, 1.52f, -0.15f), Vec3(0.18f, 0.22f, 0.18f),
            Color(0.95f, 0.48f, 0.16f, 1.0f), "Flame", false);
    AddProp(Vec3(1.2f, 0.40f, -0.85f), Vec3(0.70f, 0.36f, 0.48f),
            Color(0.55f, 0.50f, 0.42f, 1.0f), "Altar", true);

    AddProp(Vec3(0.42f, 0.28f, 0.42f), Vec3(0.55f, 0.16f, 0.18f),
            Color(0.40f, 0.26f, 0.14f, 1.0f), "Log", true);
    AddProp(Vec3(-0.38f, 0.28f, -0.05f), Vec3(0.50f, 0.16f, 0.18f),
            Color(0.36f, 0.22f, 0.12f, 1.0f), "Log", true);
    m_fire = AddProp(kFire, Vec3(0.26f, 0.42f, 0.26f),
                     Color(0.92f, 0.38f, 0.12f, 1.0f), "Campfire", false);

    AddProp(kFarSide, Vec3(0.42f, 0.28f, 0.42f),
            Color(0.50f, 0.40f, 0.30f, 1.0f), "Cairn", false);
    AddProp(Vec3(0.15f, 0.62f, -3.45f), Vec3(0.24f, 0.24f, 0.24f),
            Color(0.58f, 0.48f, 0.36f, 1.0f), "Cairn", false);

    m_warden = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_warden, Tag("Warden"));
    m_componentRegistry.AddComponent<Transform>(m_warden, Transform(
        Vec3(0.15f, kWardenY, 2.35f), Quat::Identity(), Vec3(0.48f, 1.05f, 0.48f)));
    m_componentRegistry.AddComponent<MeshRenderer>(m_warden, MeshRenderer(
        HoldCube(), Color(0.55f, 0.48f, 0.32f, 1.0f)));

    Spotlight key;
    key.position = Vec3(0.4f, 10.0f, 3.2f);
    key.direction = Vec3(0.0f, -1.0f, -0.18f).Normalized();
    key.color = Vec3(1.05f, 0.90f, 0.68f);
    key.cosInner = 0.72f;
    key.cosOuter = 0.32f;
    key.range = 32.0f;
    Renderer::SetSpotlights(&key, 1);
    Renderer::SetOccluders(nullptr, nullptr, 0);

    DND_LOG_INFO("Sandhold courtyard ready");
}

void SandholdScene::OnUnload() {
    Scene::OnUnload();
}

void SandholdScene::OnUpdate(f32 deltaTime) {
    Renderer::SetOccluders(nullptr, nullptr, 0);
    Transform* body = m_componentRegistry.GetComponent<Transform>(m_warden);
    if (body == nullptr) {
        return;
    }

    if (!Input::IsTextInputActive() && Engine::GetControlScheme() != nullptr &&
        std::strcmp(Engine::GetControlScheme(), "sandhold") == 0) {
        const f32 viewYaw = Engine::GetViewYaw();
        f32 x = 0.0f;
        f32 z = 0.0f;
        if (Input::IsKeyDown(KeyCode::W)) { x += std::sin(viewYaw); z += std::cos(viewYaw); }
        if (Input::IsKeyDown(KeyCode::S)) { x -= std::sin(viewYaw); z -= std::cos(viewYaw); }
        if (Input::IsKeyDown(KeyCode::A)) { x -= std::cos(viewYaw); z += std::sin(viewYaw); }
        if (Input::IsKeyDown(KeyCode::D)) { x += std::cos(viewYaw); z -= std::sin(viewYaw); }
        const f32 len = std::sqrt(x * x + z * z);
        if (len > 0.001f) {
            const f32 speed = 3.1f;
            Vec3 next = body->position;
            next.x += (x / len) * speed * deltaTime;
            if (next.x < -kYard) next.x = -kYard;
            if (next.x > kYard) next.x = kYard;
            if (HitsSolid(next)) {
                next.x = body->position.x;
            }
            next.z += (z / len) * speed * deltaTime;
            if (next.z < -kYard) next.z = -kYard;
            if (next.z > kYard) next.z = kYard;
            if (HitsSolid(next)) {
                next.z = body->position.z;
            }
            body->position = next;
            m_yaw = std::atan2(x, z);
        }

        if (Input::IsKeyPressed(KeyCode::F)) {
            const b8 nearFire = DistXZ(body->position, kFire) < 1.35f;
            const b8 nearGate = DistXZ(body->position, kGatePos) < 1.55f;
            if (nearFire && !m_rested) {
                RestAtFire();
            } else if (nearGate && !m_held) {
                BraceGate();
            }
        }
    }

    Engine::SetCameraFocus(Vec3(body->position.x, 0.5f, body->position.z));
    Engine::SetViewSubject(body->position, m_yaw);
}

void SandholdScene::OnRenderOverlay(f32 screenW, f32 screenH) {
    (void)screenW;
    (void)screenH;
    const Transform* body = m_componentRegistry.GetComponent<Transform>(m_warden);
    const Vec3 pos = body ? body->position : Vec3(0.0f, kWardenY, 0.0f);
    const char* view = Engine::GetViewMode() == ViewMode::Immersed ? "inside the hold" : "table";
    const i32 bonus = m_rested ? kRestBonus : 0;
    const b8 nearFire = !m_rested && DistXZ(pos, kFire) < 1.35f;
    const b8 nearGate = DistXZ(pos, kGatePos) < 1.55f;
    const b8 onFarSide = m_held && DistXZ(pos, kFarSide) < 1.15f;

    UIRenderer::DrawRect(16.0f, 16.0f, 540.0f, 132.0f, Vec4(0.10f, 0.08f, 0.05f, 0.88f));
    UIRenderer::DrawText(28.0f, 24.0f, "Sandhold  courtyard",
                         Vec4(0.96f, 0.86f, 0.62f, 1.0f), 13.0f);

    char line[180];
    std::snprintf(line, sizeof(line), "Warden   brace %+d   DC %d   view: %s",
                  bonus, kBraceDC, view);
    UIRenderer::DrawText(28.0f, 48.0f, line, Vec4(0.92f, 0.88f, 0.76f, 1.0f), 9.0f);

    if (onFarSide) {
        std::snprintf(line, sizeof(line), "The hold stands. The waste is quiet.");
    } else if (m_held && m_hasRoll) {
        std::snprintf(line, sizeof(line), "d20 %d %+d = %d  the gate holds.",
                      m_lastDie, m_lastBonus, m_lastTotal);
    } else if (m_hasRoll) {
        std::snprintf(line, sizeof(line), "d20 %d %+d = %d  the dust pushes in.",
                      m_lastDie, m_lastBonus, m_lastTotal);
    } else if (m_rested) {
        std::snprintf(line, sizeof(line), "Rested at the fire. Brace the north gate.");
    } else {
        std::snprintf(line, sizeof(line), "No brace yet.");
    }
    UIRenderer::DrawText(28.0f, 70.0f, line, Vec4(1.0f, 0.90f, 0.64f, 1.0f), 10.0f);

    const char* hint = "Rest at the campfire, then brace the north gate.";
    if (nearFire) {
        hint = "F rests at the campfire.";
    } else if (nearGate && !m_held) {
        hint = "F braces the gate.";
    } else if (m_held) {
        hint = "The gate holds. Walk through to the cairn.";
    } else if (m_rested) {
        hint = "The north gate is waiting.";
    }
    UIRenderer::DrawText(28.0f, 94.0f, hint, Vec4(0.86f, 0.80f, 0.66f, 1.0f), 8.0f);
    UIRenderer::DrawText(28.0f, 114.0f, "WASD walk   F rest/brace   F3 view   F8 campaigns",
                         Vec4(0.74f, 0.70f, 0.58f, 1.0f), 8.0f);
}

} // namespace Dnd
