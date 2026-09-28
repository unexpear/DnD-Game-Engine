/**
 * @file encounter_builder.h
 * @brief Headless encounter roster: monster catalog, HP, and initiative order.
 *
 * The encounter builder is the first DM-toolkit slice. It tracks a roster of
 * monsters copied from a small built-in catalog (the same three archetypes the
 * demo maze already uses) and can roll initiative for that roster plus any
 * extra combatants the caller supplies (typically the player party).
 *
 * No window, mesh, or ECS types are required. A separate scene bridge turns
 * roster changes into visible entities and mirrors an active encounter onto
 * CombatSystem when one is present.
 *
 * @author DND Engine Team
 * @date 2026-09-28
 * @version 0.1.0
 *
 * @note Initiative math here is d20 + DEX modifier, highest first. It does not
 *       call CombatState::RollInitiative(), which rolls every entity in the scene.
 * @note A roster creature at 0 HP is skipped during combat and omitted from the
 *       next roll, but a heal (HitPoints::Heal) or SetCurrentHp puts them back
 *       on the initiative they already have.
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include "ecs/entity.h"
#include "game/combat_stats.h"
#include <functional>
#include <string>
#include <vector>

namespace Dnd {

/**
 * @struct MonsterStatBlock
 * @brief One catalog entry a DM can drop into an encounter.
 *
 * Numbers match the demo maze's floor baselines (goblin / skeleton / bugbear),
 * not a full Monster Manual.
 */
struct MonsterStatBlock {
    const char* id;          ///< Stable catalog key, e.g. "goblin".
    const char* name;        ///< Display name, e.g. "Goblin".
    i32 maxHp;               ///< Starting maximum hit points.
    i32 armorClass;          ///< Armor class.
    i32 dexMod;              ///< DEX modifier added to initiative.
    const char* crLabel;     ///< Challenge label for display only (not an XP formula).
    Color tint;              ///< Miniature tint used when the creature is spawned.
};

/**
 * @struct EncounterMember
 * @brief One creature the DM has added to the current encounter.
 */
struct EncounterMember {
    u32 id = 0;                 ///< Stable roster id (never reused during this builder's life).
    std::string statId;        ///< Catalog key this member was copied from.
    std::string name;          ///< Unique label within the roster ("Goblin", "Goblin 2").
    i32 currentHp = 0;         ///< Current hit points. 0 means down.
    i32 maxHp = 0;             ///< Maximum hit points.
    i32 armorClass = 10;       ///< Armor class copied from the stat block.
    i32 dexMod = 0;            ///< DEX modifier. Kept here so re-rolls do not read Initiative.value.
    i32 initiative = -1;       ///< Last initiative total, or -1 if this member has not rolled.
    i32 d20 = -1;              ///< Last raw d20, or -1 if this member has not rolled.
    Entity entity = Entity::None(); ///< Scene entity, filled in by the scene bridge.
    Color tint;                ///< Miniature color at full health.
};

/**
 * @struct InitiativeInput
 * @brief One combatant to include in an initiative roll.
 *
 * Roster members are added by EncounterBuilder. Callers append extras
 * (party members already in the scene) with ids at or above kPartyIdBase
 * so they do not collide with roster ids.
 */
struct InitiativeInput {
    u32 id = 0;            ///< Caller-chosen id. Roster ids start at 1.
    std::string label;     ///< Name shown in the turn order.
    i32 dexMod = 0;        ///< Added to the d20.
};

/**
 * @struct InitiativeRoll
 * @brief One combatant's result after initiative is rolled.
 */
struct InitiativeRoll {
    u32 id = 0;            ///< Matches InitiativeInput::id.
    std::string label;     ///< Copied from the input.
    i32 d20 = 0;           ///< Face returned by the injected roller.
    i32 total = 0;         ///< d20 + DEX modifier.
};

/// @brief Supplies one d20 face. Tests inject a fixed sequence; the panel uses DiceRoller.
using D20RollFn = std::function<i32()>;

/**
 * @brief Rolls initiative for a group and sorts the order.
 *
 * Higher totals act first. Equal totals break toward the lower id so the
 * result is deterministic. Does not touch CombatState.
 *
 * @param combatants Combatants to roll. An empty list yields an empty order.
 * @param rollD20 Called once per combatant. If empty, the result is empty.
 * @return Rolls sorted from highest total to lowest.
 */
std::vector<InitiativeRoll> RollInitiativeOrder(const std::vector<InitiativeInput>& combatants,
                                                const D20RollFn& rollD20);

/**
 * @class EncounterBuilder
 * @brief DM-facing roster and initiative tracker with no rendering dependencies.
 *
 * @par Example:
 * @code
 * EncounterBuilder encounter;
 * encounter.Add("goblin");
 * encounter.RollInitiative([]() { return 12; }, {});
 * encounter.StartCombat();
 * @endcode
 *
 * @see EncounterSceneBridge
 * @see EncounterPanel
 */
class EncounterBuilder {
public:
    /// @brief First id reserved for non-roster combatants (the player party).
    static constexpr u32 kPartyIdBase = 1000000u;

    /**
     * @brief Returns the built-in monster catalog.
     * @return Goblin, skeleton, and bugbear stat blocks.
     */
    static const std::vector<MonsterStatBlock>& Catalog();

    /**
     * @brief Looks up a catalog entry by id.
     * @param statId Catalog key, e.g. "goblin".
     * @return The block, or nullptr if the id is unknown.
     */
    static const MonsterStatBlock* FindStat(const char* statId);

    /**
     * @brief Adds a monster from the catalog at full hit points.
     * @param statId Catalog key.
     * @return The new member id, or 0 if statId is unknown.
     */
    u32 Add(const char* statId);

    /**
     * @brief Removes a member from the roster and from the turn order.
     * @param memberId Roster id returned by Add.
     * @return true if the member existed.
     */
    b8 Remove(u32 memberId);

    /**
     * @brief Finds a member by roster id.
     * @param memberId Roster id.
     * @return Pointer into the roster, or nullptr. Invalidated by Add/Remove.
     */
    EncounterMember* Find(u32 memberId);
    const EncounterMember* Find(u32 memberId) const;

    /// @brief Returns the roster in insertion order.
    const std::vector<EncounterMember>& Members() const { return m_members; }

    /**
     * @brief Returns true if a scene entity was spawned for a roster member.
     * @param entity Entity to test.
     */
    b8 Owns(Entity entity) const;

    /**
     * @brief Sets current HP, clamped to [0, max].
     *
     * 0 HP does not erase the rolled initiative entry. During combat their turn
     * is skipped until they have HP again. Removing the member drops the entry.
     *
     * @param memberId Roster id.
     * @param hp New current hit points. This can raise a creature from 0.
     * @return true if the member existed.
     */
    b8 SetCurrentHp(u32 memberId, i32 hp);

    /**
     * @brief Sets maximum HP (minimum 1) and clamps current HP down to it.
     * @param memberId Roster id.
     * @param hp New maximum.
     * @return true if the member existed and hp >= 1.
     */
    b8 SetMaxHp(u32 memberId, i32 hp);

    /**
     * @brief Applies damage through HitPoints::TakeDamage.
     * @param memberId Roster id.
     * @param amount Damage. Values <= 0 do nothing.
     * @return true if hit points changed.
     */
    b8 ApplyDamage(u32 memberId, i32 amount);

    /**
     * @brief Heals through HitPoints::Heal, including from 0 HP.
     * @param memberId Roster id.
     * @param amount Healing. Values <= 0 do nothing.
     * @return true if hit points changed.
     */
    b8 Heal(u32 memberId, i32 amount);

    /**
     * @brief Rolls initiative for living members plus extras, and stores the order.
     *
     * Does not start combat. A member at 0 HP is skipped. If combat was already
     * active, it stays active and the turn resets to the top of the new order.
     * An empty result ends combat.
     *
     * @param rollD20 Die source. Called once per living combatant.
     * @param extras Extra combatants (party). Ids should be >= kPartyIdBase.
     * @return true if at least one combatant was rolled.
     */
    b8 RollInitiative(const D20RollFn& rollD20, const std::vector<InitiativeInput>& extras);

    /// @brief Last initiative order (empty if none has been rolled).
    const std::vector<InitiativeRoll>& Order() const { return m_order; }

    /// @brief Starts combat at the top of the current order.
    /// @return false if no initiative order exists.
    b8 StartCombat();

    /// @brief Ends combat. The last order is kept so the DM can start again.
    void EndCombat();

    /**
     * @brief Advances to the next combatant, wrapping to the top.
     * @return false if combat is not active.
     */
    b8 NextTurn();

    /// @brief Whether the DM has started combat and someone is still in the order.
    b8 IsCombatActive() const { return m_combatActive; }

    /// @brief Index into Order() of the creature whose turn it is.
    i32 TurnIndex() const { return m_turnIndex; }

    /**
     * @brief Returns the active turn, or nullptr if combat is not running.
     * @note The pointer is invalidated by the next roll, damage that drops
     *       someone, or any other edit to the order.
     */
    const InitiativeRoll* CurrentTurn() const;

    /// @brief When true, the panel also rolls friendly CharacterInfo entities.
    void SetIncludeParty(b8 include) { m_includeParty = include; }
    b8 IncludeParty() const { return m_includeParty; }

private:
    /// @brief Removes an id from the stored turn order and fixes the turn index.
    void DropFromOrder(u32 id);

    /**
     * @brief If combat is active, moves the turn off any roster member at 0 HP.
     *
     * Ends combat when every roster member still in the order is down. Party
     * extras (ids with no roster entry) are not skipped.
     */
    void SkipDowned();

    std::vector<EncounterMember> m_members;
    std::vector<InitiativeRoll> m_order;
    u32 m_nextId = 1;
    i32 m_turnIndex = 0;
    b8 m_combatActive = false;
    b8 m_includeParty = true;
};

} // namespace Dnd
