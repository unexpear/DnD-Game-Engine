/**
 * @file encounter_scene_bridge.cpp
 * @brief Scene glue for the encounter builder.
 *
 * See encounter_scene_bridge.h.
 *
 * @author DND Engine Team
 * @date 2026-09-28
 * @version 0.1.0
 */

#include "tools/encounter_scene_bridge.h"
#include "scene/scene.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/components/transform.h"
#include "ecs/components/tag.h"
#include "ecs/components/mesh_renderer.h"
#include "rendering/mesh.h"
#include "game/combat.h"
#include "game/combat_stats.h"
#include "game/character_stats.h"
#include "game/progression.h"
#include "core/log.h"
#include <cstring>

namespace Dnd {

namespace {

/**
 * @brief Clamps a color channel into [0, 1].
 * @param v Channel value.
 * @return Clamped channel.
 */
f32 Clamp01(f32 v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

} // namespace

EncounterSceneBridge::EncounterSceneBridge() = default;

EncounterSceneBridge::~EncounterSceneBridge() {
    Shutdown();
}

void EncounterSceneBridge::Bind(Scene* scene) {
    if (!scene) {
        m_entities = nullptr;
        m_components = nullptr;
        m_systems = nullptr;
        return;
    }
    m_entities = &scene->GetEntityManager();
    m_components = &scene->GetComponentRegistry();
    m_systems = &scene->GetSystemManager();
}

void EncounterSceneBridge::EnsureMesh() {
    if (m_mesh) return;
    m_mesh = new Mesh(Mesh::CreateCube());
}

Vec3 EncounterSceneBridge::Anchor() const {
    Vec3 anchor(0.0f, 0.5f, 0.0f);
    if (!m_entities || !m_components) return anchor;
    m_entities->ForEachAlive([&](Entity entity) {
        Tag* tag = m_components->GetComponent<Tag>(entity);
        if (!tag || !tag->name) return;
        if (std::strcmp(tag->name, "Finn") != 0) return;
        if (Transform* xf = m_components->GetComponent<Transform>(entity)) {
            anchor = xf->position;
        }
    });
    return anchor;
}

b8 EncounterSceneBridge::Realize(EncounterMember& member) {
    if (!m_entities || !m_components) return false;
    EnsureMesh();

    const Vec3 anchor = Anchor();
    const i32 col = m_spawnSerial % 3;
    const i32 row = m_spawnSerial / 3;
    ++m_spawnSerial;
    // Maze cells are 2 units wide, so a full-cell step drops the miniature in a wall.
    // Keep the cluster inside Finn's cell, beside him.
    const Vec3 pos(anchor.x + 0.55f + static_cast<f32>(col) * 0.40f,
                   0.5f,
                   anchor.z + 0.15f + static_cast<f32>(row) * 0.40f);

    Entity entity = m_entities->Create();
    m_components->AddComponent<Tag>(entity, Tag(member.name.c_str()));
    m_components->AddComponent<Transform>(entity, Transform(
        pos, Quat::Identity(), Vec3(0.70f, 1.10f, 0.70f)));
    m_components->AddComponent<MeshRenderer>(entity, MeshRenderer(m_mesh, member.tint));
    m_components->AddComponent<HitPoints>(entity, HitPoints(member.currentHp, member.maxHp, 0));
    m_components->AddComponent<ArmorClass>(entity, ArmorClass(member.armorClass));
    m_components->AddComponent<Initiative>(entity, Initiative(member.dexMod));

    member.entity = entity;
    DND_LOG_INFO("Encounter: spawned %s HP %d/%d AC %d at (%.1f, %.1f, %.1f)",
                 member.name.c_str(), member.currentHp, member.maxHp, member.armorClass,
                 pos.x, pos.y, pos.z);
    return true;
}

void EncounterSceneBridge::SyncHitPoints(const EncounterMember& member) {
    if (!m_components || !member.entity.IsValid()) return;
    HitPoints* hp = m_components->GetComponent<HitPoints>(member.entity);
    if (!hp) return;
    hp->maximum = member.maxHp;
    hp->current = member.currentHp;
    if (hp->current < 0) hp->current = 0;
    if (hp->current > hp->maximum) hp->current = hp->maximum;
}

void EncounterSceneBridge::SyncVisual(const EncounterMember& member, b8 isCurrentTurn) {
    if (!m_components || !member.entity.IsValid()) return;
    SyncHitPoints(member);

    Transform* xf = m_components->GetComponent<Transform>(member.entity);
    MeshRenderer* mr = m_components->GetComponent<MeshRenderer>(member.entity);
    if (!mr) return;

    if (member.currentHp <= 0) {
        mr->color = Color(0.28f, 0.08f, 0.08f, 1.0f);
        if (xf) xf->scale = Vec3(0.90f, 0.22f, 0.90f);
        return;
    }

    const f32 frac = (member.maxHp > 0)
        ? static_cast<f32>(member.currentHp) / static_cast<f32>(member.maxHp)
        : 0.0f;
    const f32 shade = 0.40f + 0.60f * frac;
    Color color(member.tint.r * shade, member.tint.g * shade, member.tint.b * shade, 1.0f);
    if (isCurrentTurn) {
        color.r = Clamp01(color.r + 0.28f);
        color.g = Clamp01(color.g + 0.24f);
        color.b = Clamp01(color.b + 0.08f);
    }
    mr->color = color;

    if (xf) {
        const f32 s = 0.72f + 0.28f * frac;
        const f32 pulse = isCurrentTurn ? 1.12f : 1.0f;
        xf->scale = Vec3(0.70f * s * pulse, 1.10f * s * pulse, 0.70f * s * pulse);
    }
}

void EncounterSceneBridge::SyncInitiative(const EncounterBuilder& builder) {
    if (!m_components) return;
    for (const EncounterMember& member : builder.Members()) {
        if (!member.entity.IsValid() || member.initiative < 0) continue;
        Initiative* init = m_components->GetComponent<Initiative>(member.entity);
        if (init) init->value = member.initiative;
    }
}

std::vector<InitiativeInput> EncounterSceneBridge::CollectParty(const EncounterBuilder& builder) {
    m_party.clear();
    std::vector<InitiativeInput> extras;
    if (!m_entities || !m_components) return extras;

    u32 count = 0;
    m_entities->ForEachAlive([&](Entity entity) {
        if (builder.Owns(entity)) return;
        CharacterInfo* info = m_components->GetComponent<CharacterInfo>(entity);
        if (!info || info->isHostile) return;
        HitPoints* hp = m_components->GetComponent<HitPoints>(entity);
        if (!hp || !hp->IsAlive()) return;

        InitiativeInput in;
        in.id = EncounterBuilder::kPartyIdBase + count;
        ++count;
        in.label = (info->characterName && info->characterName[0] != '\0')
            ? info->characterName : "Ally";
        in.dexMod = AbilityScores::GetModifier(info->abilities.dexterity);
        extras.push_back(in);
        m_party[in.id] = entity;
    });
    return extras;
}

void EncounterSceneBridge::ClearParty() {
    m_party.clear();
}

Entity EncounterSceneBridge::Resolve(const EncounterBuilder& builder, u32 id) const {
    if (const EncounterMember* member = builder.Find(id)) return member->entity;
    auto it = m_party.find(id);
    if (it == m_party.end()) return Entity::None();
    return it->second;
}

CombatSystem* EncounterSceneBridge::Combat() const {
    if (!m_systems) return nullptr;
    return m_systems->GetSystem<CombatSystem>("CombatSystem");
}

void EncounterSceneBridge::PushCombatState(const EncounterBuilder& builder) {
    CombatSystem* combat = Combat();
    if (!combat) return;
    if (!builder.IsCombatActive() && !m_droveCombat) return;

    CombatState& state = combat->GetState();
    state.initiativeOrder.clear();
    state.currentTurn = 0;
    state.isActive = builder.IsCombatActive();
    m_droveCombat = builder.IsCombatActive();
    if (!state.isActive) return;

    for (const InitiativeRoll& roll : builder.Order()) {
        Entity entity = Resolve(builder, roll.id);
        if (entity.IsValid()) state.initiativeOrder.push_back(entity);
    }
    state.currentTurn = builder.TurnIndex();
    if (state.initiativeOrder.empty()) {
        state.isActive = false;
        state.currentTurn = 0;
        m_droveCombat = false;
        return;
    }
    if (state.currentTurn < 0 ||
        state.currentTurn >= static_cast<i32>(state.initiativeOrder.size())) {
        state.currentTurn = 0;
    }
}

void EncounterSceneBridge::Despawn(Entity entity) {
    if (m_entities && entity.IsValid()) m_entities->Destroy(entity);
}

void EncounterSceneBridge::Shutdown() {
    if (m_droveCombat) {
        EncounterBuilder idle;
        PushCombatState(idle);
    }
    delete m_mesh;
    m_mesh = nullptr;
    m_entities = nullptr;
    m_components = nullptr;
    m_systems = nullptr;
    m_party.clear();
}

} // namespace Dnd
