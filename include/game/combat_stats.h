/**
 * @file combat_stats.h
 * @brief Combat-related stat components for D&D entities.
 *
 * This module defines the core combat statistics used during encounters:
 * hit points, armor class, initiative, and movement speed. These structs
 * are designed to be used as ECS components attached to game entities.
 *
 * Each struct is self-contained with methods for modification and querying:
 *   - HitPoints: damage, healing, death checks, temporary HP
 *   - ArmorClass: base AC + bonus stack
 *   - Initiative: roll value and advantage tracking
 *   - Speed: movement in multiple modes (walk, fly, swim, climb)
 *
 * @note All values use engine types (i32, f32, b8) for portability.
 * @warning HitPoints::TakeDamage will not reduce HP below 0 for simplicity.
 *          Track overkill damage separately if needed.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include <vector>

namespace Dnd {

// ==========================================
// STRUCT: HitPoints
// ==========================================

/**
 * @struct HitPoints
 * @brief ECS component tracking a character's hit points during combat.
 *
 * Manages current, maximum, and temporary hit points per D&D 5e rules:
 *   - Damage reduces current HP first, then temporary HP
 *   - Temporary HP is a buffer that absorbs damage before current HP
 *   - Healing cannot exceed maximum HP
 *   - A character is alive while current HP > 0
 *
 * @note Temporary HP does not stack — only the highest value applies.
 *       This implementation always uses the latest set value for temp HP.
 */
struct HitPoints {
    i32 current;    ///< Current hit points. Must be in [0, maximum]. 0 = unconscious/dead.
    i32 maximum;    ///< Maximum hit points. Set at character creation and level-up.
    i32 temporary;  ///< Temporary hit points (buffer). Does not stack with itself.

    /**
     * @brief Default constructor. All HP set to 0.
     */
    HitPoints()
        : current(0), maximum(0), temporary(0) {}

    /**
     * @brief Parameterized constructor.
     * @param maximum Maximum HP. Current HP defaults to maximum.
     */
    explicit HitPoints(i32 maximum)
        : current(maximum), maximum(maximum), temporary(0) {}

    /**
     * @brief Parameterized constructor with all values.
     * @param current Starting current HP.
     * @param maximum Maximum HP ceiling.
     * @param temporary Temporary HP buffer (default 0).
     */
    HitPoints(i32 current, i32 maximum, i32 temporary = 0)
        : current(current), maximum(maximum), temporary(temporary) {}

    /**
     * @brief Applies damage to this character.
     *
     * Damage reduces temporary HP first, then current HP.
     * Current HP will not go below 0. Overkill damage is discarded.
     *
     * @param amount The amount of damage to apply. Must be >= 0.
     *               Values < 0 are treated as 0 (no effect).
     */
    void TakeDamage(i32 amount) {
        if (amount <= 0) {
            return;
        }

        // Absorb with temporary HP first
        if (temporary > 0) {
            if (amount <= temporary) {
                temporary -= amount;
                return;
            }
            amount -= temporary;
            temporary = 0;
        }

        // Apply remaining damage to current HP
        current -= amount;
        if (current < 0) {
            current = 0;
        }
    }

    /**
     * @brief Heals this character for a given amount.
     *
     * Increases current HP by the heal amount, capped at maximum HP.
     * Does NOT restore temporary HP. Does nothing if the character
     * is dead (current HP is 0) — raise dead effects are separate.
     *
     * @param amount The amount of healing to apply. Must be >= 0.
     *               Values < 0 are treated as 0 (no effect).
     */
    void Heal(i32 amount) {
        if (amount <= 0) {
            return;
        }
        current += amount;
        if (current > maximum) {
            current = maximum;
        }
    }

    /**
     * @brief Checks whether this character is alive.
     *
     * A character is alive if current HP > 0.
     * At exactly 0 HP, the character is unconscious and dying.
     *
     * @return true if current HP > 0, false otherwise.
     */
    b8 IsAlive() const {
        return current > 0;
    }

    /**
     * @brief Resets current HP to maximum and clears temporary HP.
     *
     * Used after a long rest or when reviving a character.
     */
    void Reset() {
        current = maximum;
        temporary = 0;
    }

    /**
     * @brief Sets temporary HP. Does not stack — overwrites previous temp HP.
     *
     * Per D&D 5e rules, temporary HP does not stack. If the new value
     * is higher than the current temporary HP, it replaces it. If lower,
     * the current temporary HP is retained.
     *
     * @param amount The new temporary HP value. Must be >= 0.
     */
    void SetTemporary(i32 amount) {
        if (amount > temporary) {
            temporary = amount;
        }
    }
};

// ==========================================
// STRUCT: ArmorClass
// ==========================================

/**
 * @struct ArmorClass
 * @brief ECS component tracking a character's armor class (AC).
 *
 * Armor Class represents how difficult it is to land a damaging hit
 * on a character. The total AC is computed as:
 *   total = base + sum(bonuses)
 *
 * The base AC comes from armor, natural armor, or unarmored defense.
 * Bonuses stack and come from shields, magic items, spells, etc.
 */
struct ArmorClass {
    i32 base;                 ///< Base AC from armor, natural armor, or class features.
    std::vector<i32> bonuses; ///< Stacking bonuses (shield, magic items, spells, etc.).

    /**
     * @brief Default constructor. Creates AC 10 (unarmored baseline).
     */
    ArmorClass()
        : base(10) {}

    /**
     * @brief Parameterized constructor.
     * @param base Base AC value.
     */
    explicit ArmorClass(i32 base)
        : base(base) {}

    /**
     * @brief Computes the total AC = base + sum of all bonuses.
     *
     * Iterates over all active bonuses and adds them to the base AC.
     * The total AC is clamped to a minimum of 1 (a natural 1 always
     * misses, but AC below 1 would make impossible-to-miss targets).
     *
     * @return Total armor class. Minimum 1.
     */
    i32 GetTotal() const {
        i32 total = base;
        for (i32 bonus : bonuses) {
            total += bonus;
        }
        return total < 1 ? 1 : total;
    }

    /**
     * @brief Adds a bonus to the AC bonus list.
     * @param bonus The bonus value to add (e.g., +2 for a shield).
     */
    void AddBonus(i32 bonus) {
        bonuses.push_back(bonus);
    }

    /**
     * @brief Removes all bonuses from the bonus list.
     *
     * Used when unequipping items or when spell effects end.
     */
    void ClearBonuses() {
        bonuses.clear();
    }
};

// ==========================================
// STRUCT: Initiative
// ==========================================

/**
 * @struct Initiative
 * @brief ECS component tracking a character's initiative for turn order.
 *
 * Initiative determines the order in which combatants act during
 * an encounter. `value` is the modifier (typically Dexterity) added to a d20.
 * Rolling initiative writes the total into `total` and leaves `value` alone,
 * so a later encounter can roll again without inflating the modifier.
 * Advantage on initiative checks means rolling twice and taking the higher die.
 */
struct Initiative {
    i32 value;        ///< Modifier added to the d20. Not replaced by the roll.
    i32 total;        ///< Last rolled total (d20 + value). 0 until rolled.
    b8 hasAdvantage;  ///< Whether this character has advantage on initiative rolls.

    /**
     * @brief Default constructor. Creates a +0 modifier, no roll yet, no advantage.
     */
    Initiative()
        : value(0), total(0), hasAdvantage(false) {}

    /**
     * @brief Parameterized constructor.
     * @param value The initiative modifier (typically Dexterity).
     * @param hasAdvantage Whether the character has advantage (default false).
     */
    explicit Initiative(i32 value, b8 hasAdvantage = false)
        : value(value), total(0), hasAdvantage(hasAdvantage) {}
};

// ==========================================
// STRUCT: Speed
// ==========================================

/**
 * @struct Speed
 * @brief ECS component tracking a character's movement speeds.
 *
 * Stores movement speed in feet per round for various movement modes.
 * Walk speed is the base movement mode; fly, swim, and climb speeds
 * apply only when using those specific movement types.
 *
 * Speeds are in feet per round (1 round = 6 seconds in D&D 5e).
 * Standard PC speeds are 30ft (most races) or 25ft (dwarves/halflings).
 *
 * @note If a movement mode has speed 0, the character cannot move
 *       using that mode (cannot fly, cannot swim, etc.).
 */
struct Speed {
    f32 walk;  ///< Walking/running speed in ft/round. Standard: 30.
    f32 fly;   ///< Flying speed in ft/round. 0 = cannot fly.
    f32 swim;  ///< Swimming speed in ft/round. 0 = cannot swim (half speed).
    f32 climb; ///< Climbing speed in ft/round. 0 = cannot climb (half speed).

    /**
     * @brief Default constructor. Sets walk to 30, others to 0.
     */
    Speed()
        : walk(30.0f), fly(0.0f), swim(0.0f), climb(0.0f) {}

    /**
     * @brief Parameterized constructor.
     * @param walk Walking speed in ft/round.
     * @param fly Flying speed in ft/round (default 0).
     * @param swim Swimming speed in ft/round (default 0).
     * @param climb Climbing speed in ft/round (default 0).
     */
    Speed(f32 walk, f32 fly = 0.0f, f32 swim = 0.0f, f32 climb = 0.0f)
        : walk(walk), fly(fly), swim(swim), climb(climb) {}
};

} // namespace Dnd
