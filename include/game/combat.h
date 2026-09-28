/**
 * @file combat.h
 * @brief D&D 5e combat system: damage types, attack rolls, action economy, and turn management.
 *
 * Defines the core combat primitives used during encounters:
 *   - Damage type enumeration and damage roll structs
 *   - Attack roll mechanics with advantage/disadvantage
 *   - Action types available to combatants
 *   - Turn-based combat state management
 *   - ECS system for processing combat actions each frame
 *
 * Design principles:
 *   - All damage types map one-to-one to D&D 5e standard types
 *   - Attack rolls support full advantage/disadvantage/cancellation
 *   - Critical hits double the dice rolled (not the modifier)
 *   - Initiative is stored as a component and rolled at combat start
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.2.0
 */

#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "ecs/system.h"
#include "ecs/component.h"
#include "game/dice.h"
#include <vector>

namespace Dnd {

// ==========================================
// ENUM: DamageType
// ==========================================

/**
 * @enum DamageType
 * @brief The 13 standard D&D 5e damage types.
 *
 * Each damage type has specific interactions with creature resistances,
 * immunities, and vulnerabilities. Some spells and features care about
 * specific damage types (e.g., a Flame Tongue deals FIRE damage).
 */
enum class DamageType : u32 {
    ACID         = 0,  ///< Corrosive damage. Black dragon breath, vitriolic sphere.
    BLUDGEONING  = 1,  ///< Blunt force trauma. Maces, hammers, falling.
    COLD         = 2,  ///< Freezing damage. White dragon breath, cone of cold.
    FIRE         = 3,  ///< Heat/flame damage. Red dragon breath, fireball.
    FORCE        = 4,  ///< Pure magical energy. Magic missile, eldritch blast.
    LIGHTNING    = 5,  ///< Electrical damage. Blue dragon breath, lightning bolt.
    NECROTIC    = 6,  ///< Deathly/decay damage. Chill touch, inflict wounds.
    PIERCING     = 7,  ///< Sharp point damage. Daggers, arrows, spears.
    POISON       = 8,  ///< Toxic damage. Poisoner's kit, wyvern venom.
    PSYCHIC      = 9,  ///< Mental damage. Mind flayer attacks, psychic lance.
    RADIANT      = 10, ///< Holy/radiant damage. Sacred flame, sunbeam.
    SLASHING     = 11, ///< Cutting damage. Swords, axes, claws.
    THUNDER      = 12  ///< Sonic damage. Thunderwave, shatter.
};

// ==========================================
// STRUCT: DamageRoll
// ==========================================

/**
 * @struct DamageRoll
 * @brief Describes a damage roll with type and critical hit support.
 *
 * Combines a DiceRoll expression with a damage type. When RollDamage()
 * is called, the dice are rolled once normally; if isCritical is true,
 * the dice are rolled a second time and added (per D&D 5e critical hit
 * rules: double the dice, not the modifier).
 *
 * The damage type is informational for resistance/immunity checks.
 */
struct DamageRoll {
    DiceRoll dice;      ///< Dice expression (e.g., 2d6+3 for a greatsword).
    DamageType type;    ///< The damage type of this roll.
    b8 isCritical;      ///< If true, roll damage dice twice (per 5e crit rules).

    /**
     * @brief Default constructor. Creates a zero-damage roll (0d0, ACID, not critical).
     */
    DamageRoll()
        : type(DamageType::ACID), isCritical(false) {}

    /**
     * @brief Parameterized constructor.
     * @param dice The DiceRoll expression for this damage.
     * @param type The damage type.
     * @param isCritical Whether this is a critical hit (default false).
     */
    DamageRoll(const DiceRoll& dice, DamageType type, b8 isCritical = false)
        : dice(dice), type(type), isCritical(isCritical) {}

    /**
     * @brief Rolls the damage dice and returns the total.
     *
     * Rolls the weapon dice once. On a critical hit the dice are rolled
     * again and added; the flat modifier is applied once. Damage is never
     * negative (a penalty can reduce a hit to 0).
     *
     * @return Total damage, minimum 0.
     */
    i32 RollDamage() const {
        i32 diceTotal = 0;
        if (dice.numDice > 0 && dice.numSides > 0) {
            diceTotal = DiceRoller::Roll(dice.numDice, dice.numSides, 0);
            if (isCritical) {
                diceTotal += DiceRoller::Roll(dice.numDice, dice.numSides, 0);
            }
        }
        const i32 total = diceTotal + dice.modifier;
        return total < 0 ? 0 : total;
    }
};

// ==========================================
// STRUCT: AttackRoll
// ==========================================

/**
 * @struct AttackRoll
 * @brief Represents a D&D 5e attack roll with advantage/disadvantage support.
 *
 * An attack roll is a d20 plus the attack bonus. If both advantage and
 * disadvantage are true, they cancel out (normal roll). The MakeRoll()
 * method returns a RollResult with individual die values and the total.
 *
 * IsHit() is a convenience that rolls and checks against a target's AC.
 */
struct AttackRoll {
    i32 attackBonus;       ///< Flat bonus added to the d20 roll (proficiency + ability mod).
    b8 hasAdvantage;       ///< Roll twice, take the higher result.
    b8 hasDisadvantage;    ///< Roll twice, take the lower result.

    /**
     * @brief Default constructor. Creates a +0 attack with no advantage.
     */
    AttackRoll()
        : attackBonus(0), hasAdvantage(false), hasDisadvantage(false) {}

    /**
     * @brief Parameterized constructor.
     * @param attackBonus Flat bonus to the d20 roll.
     * @param hasAdvantage Whether the attacker has advantage (default false).
     * @param hasDisadvantage Whether the attacker has disadvantage (default false).
     */
    AttackRoll(i32 attackBonus, b8 hasAdvantage = false, b8 hasDisadvantage = false)
        : attackBonus(attackBonus), hasAdvantage(hasAdvantage), hasDisadvantage(hasDisadvantage) {}

    /**
     * @brief Rolls the d20 with advantage/disadvantage and returns the full result.
     *
     * If both advantage and disadvantage are set, they cancel — a single
     * d20 is rolled. The returned RollResult includes individual die values
     * and the total (dice result + attackBonus).
     *
     * @return RollResult with individual rolls and computed total.
     */
    RollResult MakeRoll() const {
        i32 diceResult;
        std::vector<i32> rolls;

        // Advantage and disadvantage cancel
        if (hasAdvantage == hasDisadvantage) {
            diceResult = DiceRoller::Roll(1, 20);
            rolls.push_back(diceResult);
        } else if (hasAdvantage) {
            i32 r1 = DiceRoller::Roll(1, 20);
            i32 r2 = DiceRoller::Roll(1, 20);
            rolls.push_back(r1);
            rolls.push_back(r2);
            diceResult = r1 > r2 ? r1 : r2;
        } else {
            i32 r1 = DiceRoller::Roll(1, 20);
            i32 r2 = DiceRoller::Roll(1, 20);
            rolls.push_back(r1);
            rolls.push_back(r2);
            diceResult = r1 < r2 ? r1 : r2;
        }

        return RollResult{rolls, diceResult + attackBonus, attackBonus};
    }

    /**
     * @brief Checks if this attack hits the given armor class.
     *
     * Convenience method that calls MakeRoll() and compares the total
     * to the target's AC. Rolls a natural 20 always hits (critical),
     * natural 1 always misses (per D&D 5e rules).
     *
     * @param armorClass The target's armor class to beat.
     * @return true if the total >= armorClass (or natural 20).
     */
    b8 IsHit(i32 armorClass) const {
        // We need the raw d20 value to check natural 20/1
        i32 rawRoll;

        if (hasAdvantage == hasDisadvantage) {
            rawRoll = DiceRoller::Roll(1, 20);
        } else if (hasAdvantage) {
            i32 r1 = DiceRoller::Roll(1, 20);
            i32 r2 = DiceRoller::Roll(1, 20);
            rawRoll = r1 > r2 ? r1 : r2;
        } else {
            i32 r1 = DiceRoller::Roll(1, 20);
            i32 r2 = DiceRoller::Roll(1, 20);
            rawRoll = r1 < r2 ? r1 : r2;
        }

        // Natural 20 always hits, natural 1 always misses
        if (rawRoll == 20) return true;
        if (rawRoll == 1) return false;

        return (rawRoll + attackBonus) >= armorClass;
    }
};

// ==========================================
// ENUM: ActionType
// ==========================================

/**
 * @enum ActionType
 * @brief The standard action types available in D&D 5e combat.
 *
 * Each creature gets one action, one bonus action, and one reaction
 * per round. Some actions (like DASH) are mutually exclusive with
 * casting certain spells. Ready is a special action that uses both
 * the action and the reaction.
 */
enum class ActionType : u32 {
    ATTACK       = 0,  ///< Make a weapon attack or combat maneuver.
    CAST_SPELL   = 1,  ///< Cast a spell (action casting time).
    DASH         = 2,  ///< Double your movement speed this turn.
    DISENGAGE    = 3,  ///< Movement does not provoke opportunity attacks.
    DODGE        = 4,  ///< All attacks have disadvantage against you until next turn.
    HELP         = 5,  ///< Grant an ally advantage on their next attack.
    HIDE         = 6,  ///< Make a Stealth check to become hidden.
    READY        = 7,  ///< Prepare an action to trigger on a specified condition.
    SEARCH       = 8,  ///< Make a Perception or Investigation check.
    USE_OBJECT   = 9,  ///< Interact with an object (e.g., open a door, drink a potion).
    BONUS_ACTION = 10, ///< Quick secondary action (offhand attack, class feature).
    REACTION     = 11  ///< Response to a trigger (opportunity attack, shield spell).
};

// ==========================================
// STRUCT: CombatAction
// ==========================================

/**
 * @struct CombatAction
 * @brief Describes a single action taken during combat.
 *
 * Captures all information needed to process an action: what was done,
 * who did it, who it targeted, what damage was rolled, and what attack
 * roll was made. The description string is used for combat log output.
 */
struct CombatAction {
    ActionType type;       ///< The type of action taken (attack, cast spell, etc.).
    Entity target;         ///< The entity this action targets. Entity::None() for self-targeting.
    Entity source;         ///< The entity taking the action.
    DamageRoll damage;     ///< Damage roll data (ignored if the action deals no damage).
    AttackRoll attack;     ///< Attack roll data (ignored for non-attack actions).
    const char* description; ///< Human-readable description for the combat log.

    /**
     * @brief Default constructor. Creates an empty ATTACK action with no source or target.
     */
    CombatAction()
        : type(ActionType::ATTACK), target(Entity::None()), source(Entity::None())
        , description("") {}
};

// ==========================================
// STRUCT: CombatState
// ==========================================

/**
 * @struct CombatState
 * @brief Tracks turn-based combat progress: initiative order and current turn.
 *
 * CombatState maintains the ordered list of participants, whose turn
 * it is, and whether combat is active. Use RollInitiative() to establish
 * turn order at the start of combat, then NextTurn() to advance.
 *
 * Usage:
 *   CombatState state;
 *   state.RollInitiative(em, cr);
 *   while (state.isActive) {
 *       Entity current = state.GetCurrentEntity();
 *       // Process current entity's turn
 *       state.NextTurn();
 *   }
 */
struct CombatState {
    std::vector<Entity> initiativeOrder; ///< Participants sorted by initiative (highest first).
    i32 currentTurn;                     ///< Index into initiativeOrder for the current actor.
    b8 isActive;                         ///< Whether combat is ongoing. Set false to end.

    /**
     * @brief Default constructor. Creates an inactive combat state with no participants.
     */
    CombatState()
        : currentTurn(0), isActive(false) {}

    /**
     * @brief Returns the entity whose turn it currently is.
     * @return The current Entity, or Entity::None() if combat is inactive or the turn index is invalid.
     */
    Entity GetCurrentEntity() const {
        if (!isActive || currentTurn < 0) {
            return Entity::None();
        }
        if (currentTurn >= static_cast<i32>(initiativeOrder.size())) {
            return Entity::None();
        }
        return initiativeOrder[currentTurn];
    }

    /**
     * @brief Advances to the next turn in initiative order.
     *
     * Increments the turn counter. When the end of the initiative order
     * is reached, wraps back to 0 (next round). Does nothing if combat
     * is not active.
     */
    void NextTurn() {
        if (!isActive) return;
        currentTurn++;
        if (currentTurn >= static_cast<i32>(initiativeOrder.size())) {
            currentTurn = 0;
        }
    }

    /**
     * @brief Rolls initiative for all entities with an Initiative component.
     *
     * Scans all entities that have the Initiative component. For each,
     * rolls a d20 (with advantage if the component indicates it), stores
     * the result back in the component, then sorts all participants by
     * initiative descending (highest goes first). Ties are broken by the
     * entity's index (lower index = higher in a tie).
     *
     * After this call:
     *   - initiativeOrder is populated and sorted
     *   - currentTurn resets to 0
     *   - isActive is set to true
     *
     * @note This uses the Initiative component's hasAdvantage field.
     *       The rolled value replaces the component's current value.
     *
     * @param em Entity manager for checking entity validity.
     * @param cr Component registry for accessing Initiative components.
     * @pre Initiative components should be attached to all combat participants.
     */
    void RollInitiative(EntityManager& em, ComponentRegistry& cr);

    /**
     * @brief Removes a dead entity from the initiative order.
     *
     * Called when an entity's hit points reach 0. Removes them from
     * the initiative order and adjusts the current turn index.
     *
     * @param entity The entity to remove.
     * @return true if the entity was found and removed.
     */
    b8 RemoveEntity(Entity entity);
};

// ==========================================
// CLASS: CombatSystem
// ==========================================

/**
 * @class CombatSystem
 * @brief ECS system that processes combat actions each frame.
 *
 * The CombatSystem runs during the UPDATE phase. It manages the
 * turn-based combat flow:
 *   1. Checks if combat is active and a current entity is set
 *   2. Processes queued CombatAction entries from game code
 *   3. Applies damage to targets via HitPoints component
 *   4. Checks for dead entities (HP <= 0) and removes them
 *   5. Advances the turn when the current actor's actions are done
 *
 * Usage:
 *   SystemManager sysMan;
 *   auto combatSys = std::make_unique<CombatSystem>();
 *   sysMan.RegisterSystem(std::move(combatSys), em, cr);
 *   // Later, queue an action:
 *   combatSys->QueueAction(action);
 */
class CombatSystem : public System {
public:
    /**
     * @brief Constructs the CombatSystem.
     * Priority 50 runs it during UPDATE phase after most movement/logic systems.
     */
    CombatSystem()
        : System("CombatSystem", SystemPhase::UPDATE, 50) {}

    /**
     * @brief Queues a combat action to be processed.
     *
     * Actions are stored and processed in OnUpdate(). After processing,
     * damage is applied to the target's HitPoints component, and the
     * action's description is logged.
     *
     * @param action The CombatAction to queue for processing.
     */
    void QueueAction(const CombatAction& action);

    /**
     * @brief Returns a mutable reference to the combat state.
     * @return Reference to the internal CombatState.
     */
    CombatState& GetState() { return m_state; }

    /**
     * @brief Returns a const reference to the combat state.
     * @return Const reference to the internal CombatState.
     */
    const CombatState& GetState() const { return m_state; }

    /**
     * @brief Returns a const reference to the action queue.
     * @return Const reference to the pending actions vector.
     */
    const std::vector<CombatAction>& GetPendingActions() const { return m_actions; }

protected:
    /**
     * @brief Processes all queued combat actions.
     *
     * For each action in the queue:
     *   1. If it's an ATTACK, check IsHit against the target's ArmorClass
     *   2. Apply damage to the target's HitPoints
     *   3. If the target's HP drops to 0, remove from initiative
     *   4. Log the action description
     *
     * After all actions are processed, the queue is cleared.
     *
     * @param deltaTime Time since last frame (unused).
     * @param em Entity manager (for entity lifecycle checks).
     * @param cr Component registry (for HitPoints, ArmorClass access).
     */
    void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) override;

private:
    CombatState m_state;                  ///< Current combat state (initiative order, turn tracking).
    std::vector<CombatAction> m_actions;  ///< Queue of pending combat actions to process.
};

} // namespace Dnd
