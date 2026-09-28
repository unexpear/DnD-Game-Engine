/**
 * @file encounter_scene_bridge.h
 * @brief Places encounter-roster monsters into a live scene and mirrors combat.
 *
 * The bridge is the glue between EncounterBuilder (pure roster data) and the
 * demo's ECS scene. Spawning creates a tagged miniature the hierarchy,
 * inspector, and translate gizmo already understand. Starting combat copies
 * the builder's order onto CombatSystem without calling RollInitiative(),
 * so creatures the DM did not add are left alone.
 *
 * @author DND Engine Team
 * @date 2026-09-28
 * @version 0.1.0
 *
 * @note The cube mesh is created on the first spawn and needs a current
 *       OpenGL context. Headless tests should not call Realize().
 */

#pragma once

#include "core/types.h"
#include "tools/encounter_builder.h"
#include <unordered_map>
#include <vector>

namespace Dnd {

class Scene;
class Mesh;
class EntityManager;
class ComponentRegistry;
class SystemManager;

/**
 * @class EncounterSceneBridge
 * @brief Spawns, tints, and despawns encounter miniatures in a Scene.
 *
 * @see EncounterBuilder
 * @see EncounterPanel
 */
class EncounterSceneBridge {
public:
    EncounterSceneBridge();
    ~EncounterSceneBridge();

    EncounterSceneBridge(const EncounterSceneBridge&) = delete;
    EncounterSceneBridge& operator=(const EncounterSceneBridge&) = delete;

    /**
     * @brief Binds the bridge to the active scene. Safe to call every frame.
     * @param scene Active scene, or nullptr to unbind.
     */
    void Bind(Scene* scene);

    /**
     * @brief Spawns a miniature for a roster member and stores the entity on it.
     * @param member Member to place. Name, HP, AC, and tint are read from here.
     * @return true if an entity was created.
     */
    b8 Realize(EncounterMember& member);

    /**
     * @brief Writes current/max HP onto the member's HitPoints component.
     * @param member Roster member that already has an entity.
     */
    void SyncHitPoints(const EncounterMember& member);

    /**
     * @brief Updates the miniature's color and scale from HP and turn state.
     * @param member Roster member to refresh.
     * @param isCurrentTurn True when this member is the active combatant.
     */
    void SyncVisual(const EncounterMember& member, b8 isCurrentTurn);

    /**
     * @brief Writes rolled initiative totals onto spawned members only.
     *
     * Party members are not modified. Their Initiative component still holds
     * the DEX modifier the rest of the demo stored at creation.
     *
     * @param builder Roster that has just been rolled.
     */
    void SyncInitiative(const EncounterBuilder& builder);

    /**
     * @brief Collects living, non-hostile CharacterInfo entities as initiative extras.
     *
     * Replaces the previous party map. Ids start at EncounterBuilder::kPartyIdBase.
     * Encounter miniatures are skipped.
     *
     * @param builder Roster used to skip already-spawned monsters.
     * @return Inputs suitable for EncounterBuilder::RollInitiative.
     */
    std::vector<InitiativeInput> CollectParty(const EncounterBuilder& builder);

    /// @brief Forgets party entities collected for the last roll.
    void ClearParty();

    /**
     * @brief Copies the builder's combat flag, order, and turn onto CombatSystem.
     *
     * No-ops if this bridge has never started combat and the builder is idle,
     * so a scene combat started by something else is left alone.
     *
     * @param builder Source of truth for the DM's encounter.
     */
    void PushCombatState(const EncounterBuilder& builder);

    /**
     * @brief Destroys a spawned entity. The mesh is kept for later spawns.
     * @param entity Entity previously returned via Realize.
     */
    void Despawn(Entity entity);

    /// @brief Ends mirrored combat, despawns nothing, and frees the cube mesh.
    void Shutdown();

private:
    /// @brief Creates the shared cube the first time a monster is spawned.
    void EnsureMesh();

    /// @brief World position to cluster new miniatures around (Finn, if present).
    Vec3 Anchor() const;

    /// @brief Resolves a roster id or a party id to a scene entity.
    Entity Resolve(const EncounterBuilder& builder, u32 id) const;

    /// @brief Combat system on the bound scene, or nullptr.
    class CombatSystem* Combat() const;

    EntityManager* m_entities = nullptr;
    ComponentRegistry* m_components = nullptr;
    SystemManager* m_systems = nullptr;
    Mesh* m_mesh = nullptr;
    i32 m_spawnSerial = 0;
    b8 m_droveCombat = false;
    std::unordered_map<u32, Entity> m_party;
};

} // namespace Dnd
