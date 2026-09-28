/**
 * @file spell.h
 * @brief D&D 5e spellcasting system: schools, slot tracking, spellbooks, and casting flow.
 *
 * Defines the complete spellcasting subsystem used by spellcasting classes:
 *   - SpellSchool enumeration (8 schools of magic)
 *   - Casting time, duration, and range descriptors
 *   - SpellData for full spell definitions
 *   - SpellSlots ECS component for resource tracking
 *   - Spellbook ECS component for known/prepared spells
 *   - ECS system for processing spell cast requests
 *
 * Design principles:
 *   - SpellData is a POD-like description; actual effects are applied
 *     by the SpellcastingSystem based on the spell's fields
 *   - SpellSlots tracks both current and maximum slots per level (0-9)
 *     Level 0 represents cantrips (unlimited, never consumed)
 *   - The spellbook supports both "spells known" and "spells prepared" models
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
#include "game/combat.h"
#include <vector>
#include <cstring>

namespace Dnd {

// ==========================================
// ENUM: SpellSchool
// ==========================================

/**
 * @enum SpellSchool
 * @brief The eight schools of magic in D&D 5e.
 *
 * Each spell belongs to exactly one school, which categorizes its
 * magical discipline. Some class features and magic items interact
 * with specific schools (e.g., Abjuration Savant for wizards).
 */
enum class SpellSchool : u32 {
    ABJURATION     = 0, ///< Protective magic. Shield, counterspell, dispel magic.
    CONJURATION    = 1, ///< Summoning/teleportation. Conjure animals, dimension door.
    DIVINATION     = 2, ///< Information gathering. Detect magic, scrying.
    ENCHANTMENT    = 3, ///< Mind-affecting magic. Charm person, hold person.
    EVOCATION      = 4, ///< Raw energy manipulation. Fireball, lightning bolt.
    ILLUSION       = 5, ///< Deceptive magic. Minor illusion, invisibility.
    NECROMANCY     = 6, ///< Life/death manipulation. Animate dead, raise dead.
    TRANSMUTATION  = 7  ///< Matter transformation. Polymorph, enlarge/reduce.
};

// ==========================================
// ENUM: SpellCastingTime
// ==========================================

/**
 * @enum SpellCastingTime
 * @brief The possible casting time categories for spells.
 *
 * Most spells require an ACTION to cast. Bonus action spells can be
 * cast alongside a cantrip. Reaction spells trigger on specific events.
 * Minute/hour spells are typically cast outside of combat. RITUAL spells
 * can be cast without expending a slot if the caster has Ritual Casting.
 */
enum class SpellCastingTime : u32 {
    ACTION       = 0, ///< Standard casting time. Takes one action.
    BONUS_ACTION = 1, ///< Quick spell cast as a bonus action.
    REACTION     = 2, ///< Cast as a reaction to a trigger condition.
    MINUTE       = 3, ///< Takes one or more minutes (out of combat).
    HOUR         = 4, ///< Takes one or more hours (rituals, teleportation circle).
    RITUAL       = 5  ///< Can be cast as a ritual (takes +10 minutes, no slot).
};

// ==========================================
// STRUCT: SpellDuration
// ==========================================

/**
 * @struct SpellDuration
 * @brief Describes how long a spell's effects last.
 *
 * Duration is represented as a numeric value (in rounds or minutes
 * depending on context) plus flags for special cases. A duration of
 * INSTANT means the effect happens and immediately ends. CONCENTRATION
 * spells require the caster to maintain focus. PERMANENT spells last
 * until dispelled or the material component is destroyed.
 */
struct SpellDuration {
    i32 value;                ///< Duration length. 0 for instantaneous, otherwise in rounds/minutes.
    b8 isInstant;            ///< If true, the spell has instantaneous duration (value is 0).
    b8 requiresConcentration; ///< If true, the caster must concentrate to maintain the spell.
    b8 isPermanent;           ///< If true, the effect lasts until dispelled.

    /**
     * @brief Default constructor. Creates an instantaneous duration.
     */
    SpellDuration()
        : value(0), isInstant(true), requiresConcentration(false), isPermanent(false) {}

    /**
     * @brief Parameterized constructor.
     * @param value Duration length.
     * @param isInstant Whether the effect is instantaneous.
     * @param requiresConcentration Whether concentration is required.
     * @param isPermanent Whether the effect is permanent.
     */
    SpellDuration(i32 value, b8 isInstant, b8 requiresConcentration, b8 isPermanent)
        : value(value), isInstant(isInstant)
        , requiresConcentration(requiresConcentration), isPermanent(isPermanent) {}
};

// ==========================================
// STRUCT: SpellRange
// ==========================================

/**
 * @struct SpellRange
 * @brief Describes the targeting range of a spell.
 *
 * A spell's range determines who or what it can target. SELF spells
 * only affect the caster. TOUCH spells require the caster to touch
 * the target. RANGED spells have a distance in feet. Some spells
 * have both a normal and long range (not tracked here; use description).
 */
struct SpellRange {
    b8 isSelf;    ///< If true, the spell only affects the caster (rangeFeet is 0).
    b8 isTouch;   ///< If true, the spell requires physical contact.
    b8 isRanged;  ///< If true, the spell has a ranged component.
    i32 rangeFeet; ///< Range in feet. 0 for SELF or TOUCH spells.

    /**
     * @brief Default constructor. Creates a SELF range.
     */
    SpellRange()
        : isSelf(true), isTouch(false), isRanged(false), rangeFeet(0) {}

    /**
     * @brief Parameterized constructor.
     * @param isSelf Whether the spell is Self-targeted.
     * @param isTouch Whether the spell requires touch.
     * @param isRanged Whether the spell has a range.
     * @param rangeFeet Distance in feet (ignored if isSelf or isTouch).
     */
    SpellRange(b8 isSelf, b8 isTouch, b8 isRanged, i32 rangeFeet)
        : isSelf(isSelf), isTouch(isTouch), isRanged(isRanged), rangeFeet(rangeFeet) {}
};

// ==========================================
// STRUCT: SpellData
// ==========================================

/**
 * @struct SpellData
 * @brief Full definition of a single D&D 5e spell.
 *
 * Contains all mechanical information needed to process a spell:
 * name, level, school, casting time, duration, range, component
 * requirements, description text, damage data, and targeting info.
 *
 * This struct is designed to be stored in the Spellbook component.
 * Spell definitions can be loaded from a database or defined at
 * compile time.
 *
 * @note The description field is for UI/log display. Actual effects
 *       are applied based on the damage, requiresAttackRoll, and
 *       requiresSavingThrow fields.
 */
struct SpellData {
    const char* name;                ///< Spell name (e.g., "Fireball", "Cure Wounds").
    i32 level;                       ///< Spell level: 0 (cantrip) through 9.
    SpellSchool school;              ///< School of magic.
    SpellCastingTime castingTime;    ///< Time required to cast the spell.
    SpellDuration duration;          ///< How long the spell's effects persist.
    SpellRange range;                ///< Targeting range of the spell.
    b8 requiresVerbal;               ///< Requires spoken incantations (V component).
    b8 requiresSomatic;              ///< Requires hand gestures (S component).
    b8 requiresMaterial;             ///< Requires material components (M component).
    const char* description;         ///< Full spell description text (flavor + mechanics).
    DamageRoll damage;               ///< Damage data if the spell deals damage.
    b8 requiresAttackRoll;           ///< If true, the spell requires a spell attack roll.
    b8 requiresSavingThrow;          ///< If true, the target gets a saving throw.

    /**
     * @brief Default constructor. Creates an empty level-0 cantrip.
     */
    SpellData()
        : name(""), level(0), school(SpellSchool::ABJURATION)
        , castingTime(SpellCastingTime::ACTION), duration(), range()
        , requiresVerbal(false), requiresSomatic(false), requiresMaterial(false)
        , description(""), requiresAttackRoll(false), requiresSavingThrow(false) {}

    /**
     * @brief Parameterized constructor for a full spell definition.
     * @param name Spell name.
     * @param level Spell level (0-9).
     * @param school School of magic.
     * @param castingTime Casting time category.
     * @param duration Spell duration.
     * @param range Spell range.
     * @param requiresVerbal Verbal component required.
     * @param requiresSomatic Somatic component required.
     * @param requiresMaterial Material component required.
     * @param description Full spell description.
     * @param damage Damage roll data (for damaging spells).
     * @param requiresAttackRoll Whether a spell attack roll is needed.
     * @param requiresSavingThrow Whether the target gets a saving throw.
     */
    SpellData(const char* name, i32 level, SpellSchool school,
              SpellCastingTime castingTime, const SpellDuration& duration,
              const SpellRange& range,
              b8 requiresVerbal, b8 requiresSomatic, b8 requiresMaterial,
              const char* description, const DamageRoll& damage,
              b8 requiresAttackRoll, b8 requiresSavingThrow)
        : name(name), level(level), school(school)
        , castingTime(castingTime), duration(duration), range(range)
        , requiresVerbal(requiresVerbal), requiresSomatic(requiresSomatic)
        , requiresMaterial(requiresMaterial), description(description)
        , damage(damage)
        , requiresAttackRoll(requiresAttackRoll), requiresSavingThrow(requiresSavingThrow) {}
};

// ==========================================
// STRUCT: SpellSlots (ECS Component)
// ==========================================

/**
 * @struct SpellSlots
 * @brief ECS component tracking a spellcaster's available and maximum spell slots.
 *
 * Spell slots are indexed by spell level (0-9). Level 0 represents
 * cantrips, which are never consumed when cast. Levels 1-9 correspond
 * to standard D&D 5e spell slot levels.
 *
 * The slots array holds the current remaining slots, and maxSlots
 * holds the per-level maximums. Maximums are set at character creation
 * and class level-up; they do not change during an adventure except
 * through specific features (e.g., Arcane Recovery).
 *
 * Usage:
 *   SpellSlots slots;
 *   slots.maxSlots[1] = 2; // 2 first-level slots
 *   slots.slots[1] = 2;
 *   if (slots.HasSlot(1)) { slots.UseSlot(1); }
 */
struct SpellSlots {
    i32 slots[10];    ///< Current remaining spell slots per level [0..9].
    i32 maxSlots[10]; ///< Maximum spell slots per level [0..9].

    /**
     * @brief Default constructor. All slots are 0.
     */
    SpellSlots() {
        for (i32 i = 0; i < 10; i++) {
            slots[i] = 0;
            maxSlots[i] = 0;
        }
    }

    /**
     * @brief Parameterized constructor.
     * @param initialSlots Array of 10 initial slot values.
     * @param maximumSlots Array of 10 maximum slot values.
     */
    SpellSlots(const i32 initialSlots[10], const i32 maximumSlots[10]) {
        for (i32 i = 0; i < 10; i++) {
            slots[i] = initialSlots[i];
            maxSlots[i] = maximumSlots[i];
        }
    }

    /**
     * @brief Checks if a spell slot of the given level is available.
     *
     * Cantrips (level 0) are always available.
     *
     * @param level Spell level to check (0-9).
     * @return true if slots[level] > 0 or level == 0.
     *         Returns false if level is out of range [0, 9].
     */
    b8 HasSlot(i32 level) const {
        if (level < 0 || level > 9) return false;
        if (level == 0) return true; // Cantrips always available
        return slots[level] > 0;
    }

    /**
     * @brief Consumes one spell slot of the given level.
     *
     * Decrements the current slot count. Does nothing for cantrips
     * (level 0) since they are never consumed.
     *
     * @param level Spell level to consume (0-9).
     * @note Does nothing if level is out of range or no slots remain.
     *       Call HasSlot() first to check availability.
     */
    void UseSlot(i32 level) {
        if (level < 1 || level > 9) return;
        if (slots[level] > 0) {
            slots[level]--;
        }
    }

    /**
     * @brief Restores all spell slots to their maximum values.
     *
     * Copies maxSlots into slots for each level. Typically called
     * after a long rest.
     */
    void RestoreAll() {
        for (i32 i = 0; i < 10; i++) {
            slots[i] = maxSlots[i];
        }
    }
};

// ==========================================
// STRUCT: Spellbook (ECS Component)
// ==========================================

/**
 * @struct Spellbook
 * @brief ECS component tracking a character's known and prepared spells.
 *
 * Supports both prepared casting (clerics, druids, paladins, wizards)
 * and known casting (bards, sorcerers, warlocks, rangers). The spells
 * vector holds all spells the character knows. For prepared casters,
 * spellsPrepared limits how many can be readied each day; for known
 * casters, spellsKnown is the total the character can ever learn.
 *
 * GetSpellsByLevel returns pointers to spells of a given level.
 * The returned array is null-terminated and owned by the Spellbook.
 *
 * Usage:
 *   Spellbook book;
 *   book.spells.push_back(fireball);
 *   if (book.HasSpell("Fireball")) {
 *       SpellData** level3 = book.GetSpellsByLevel(3);
 *   }
 */
struct Spellbook {
    std::vector<SpellData> spells;  ///< All spells the character knows.
    i32 spellsKnown;                ///< Maximum number of spells the character can know (for known casters).
    i32 spellsPrepared;             ///< Maximum number of spells that can be prepared each day (for prepared casters).
    std::vector<SpellData*> m_cachedLevel; ///< Internal cache for GetSpellsByLevel(). Invalidated on each call.

    /**
     * @brief Default constructor. No spells known or prepared.
     */
    Spellbook()
        : spellsKnown(0), spellsPrepared(0) {}

    /**
     * @brief Gets all spells of a given level.
     *
     * Returns a pointer to an internal null-terminated array of
     * SpellData pointers matching the requested level. The array
     * is valid until the next call to GetSpellsByLevel() or until
     * the Spellbook is modified.
     *
     * @param level The spell level to filter by (0-9).
     * @return Null-terminated array of SpellData pointers.
     *         The array is empty (first element is nullptr) if no
     *         spells of the given level exist.
     */
    SpellData** GetSpellsByLevel(i32 level) {
        m_cachedLevel.clear();
        for (auto& spell : spells) {
            if (spell.level == level) {
                m_cachedLevel.push_back(&spell);
            }
        }
        m_cachedLevel.push_back(nullptr);
        return m_cachedLevel.data();
    }

    /**
     * @brief Checks if the spellbook contains a spell by name.
     *
     * Performs a case-sensitive string comparison against all
     * spells in the book.
     *
     * @param name The spell name to search for.
     * @return true if a spell with the given name exists.
     */
    b8 HasSpell(const char* name) const {
        for (const auto& spell : spells) {
            if (std::strcmp(spell.name, name) == 0) {
                return true;
            }
        }
        return false;
    }
};

// ==========================================
// CLASS: SpellcastingSystem
// ==========================================

/**
 * @class SpellcastingSystem
 * @brief ECS system that processes spell cast requests.
 *
 * The SpellcastingSystem runs during the UPDATE phase. External code
 * submits spell cast requests via RequestCast(), and the system
 * processes them in its OnUpdate:
 *   1. Validates the caster has SpellSlots and Spellbook components
 *   2. Looks up the spell in the caster's spellbook
 *   3. Checks that the required spell slot level is available
 *   4. Logs the verbal/somatic/material component usage
 *   5. If the spell has damage, applies it to the target
 *   6. Consumes the spell slot
 *
 * Usage:
 *   SystemManager sysMan;
 *   auto spellSys = std::make_unique<SpellcastingSystem>();
 *   sysMan.RegisterSystem(std::move(spellSys), em, cr);
 *   // Later:
 *   spellSys->RequestCast(caster, 0, target);
 */
class SpellcastingSystem : public System {
public:
    /**
     * @struct SpellCastRequest
     * @brief Internal struct for queued spell cast requests.
     */
    struct SpellCastRequest {
        Entity caster;     ///< The entity casting the spell.
        i32 spellIndex;    ///< Index into the Spellbook's spells vector.
        Entity target;     ///< The target entity (may be Entity::None() for self/AoE).
    };

    /**
     * @brief Constructs the SpellcastingSystem.
     * Priority 60 runs it during UPDATE after combat processing.
     */
    SpellcastingSystem()
        : System("SpellcastingSystem", SystemPhase::UPDATE, 60) {}

    /**
     * @brief Queues a spell cast request for processing.
     *
     * The request is processed during the next OnUpdate() call.
     * This method is thread-safe in the sense that it only pushes
     * to a vector; the system processes requests on the main thread.
     *
     * @param caster The entity casting the spell.
     * @param spellIndex Index into the caster's Spellbook::spells vector.
     * @param target The spell's target (may be Entity::None()).
     * @return true if the request was queued successfully.
     * @pre The caster should have SpellSlots and Spellbook components.
     */
    b8 RequestCast(Entity caster, i32 spellIndex, Entity target) {
        m_pendingCasts.push_back({caster, spellIndex, target});
        return true;
    }

protected:
    /**
     * @brief Processes all queued spell cast requests.
     *
     * For each pending cast:
     *   1. Retrieve SpellSlots and Spellbook from the caster
     *   2. Look up the SpellData by spellIndex
     *   3. Verify the spell slot is available
     *   4. Log component requirements (V/S/M)
     *   5. For damaging spells with an attack roll, process the attack
     *   6. Apply damage to the target
     *   7. Consume the spell slot
     *
     * After processing, the pending queue is cleared.
     *
     * @param deltaTime Time since last frame (unused).
     * @param em Entity manager (for entity lifecycle checks).
     * @param cr Component registry (for SpellSlots, Spellbook, HitPoints access).
     */
    void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) override;

    /**
     * @brief Processes a single spell cast request.
     *
     * Extracted into its own method for clarity. Performs all validation
     * and effect application for one spell cast.
     *
     * @param request The cast request to process.
     * @param em Entity manager reference.
     * @param cr Component registry reference.
     * @return true if the spell was successfully cast.
     */
    b8 ProcessCast(const SpellCastRequest& request, EntityManager& em, ComponentRegistry& cr);

private:
    std::vector<SpellCastRequest> m_pendingCasts; ///< Queue of pending spell cast requests.
};

} // namespace Dnd
