/**
 * @file dice.h
 * @brief Dice rolling and randomization systems for D&D gameplay.
 *
 * This module provides all dice-related functionality for the D&D game engine,
 * including individual dice rolls, advantage/disadvantage mechanics, standard
 * stat generation (4d6 drop lowest), and percentile dice. The DiceRoller utility
 * uses a Mersenne Twister (std::mt19937) PRNG seeded once at engine startup.
 *
 * Design principles:
 *   - All random state is managed internally by DiceRoller (singleton-style)
 *   - Individual roll histories are captured via RollResult
 *   - DiceRoll struct supports re-usable dice expressions
 *   - Stat generation follows official 5e rules (4d6, drop lowest, repeat 6 times)
 *
 * @note Call DiceRoller::Initialize() once at engine startup before any rolls.
 * @warning DiceRoller is NOT thread-safe. All rolls must happen from the main thread.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include <vector>
#include <random>

namespace Dnd {

// ==========================================
// STRUCT: RollResult
// ==========================================

/**
 * @struct RollResult
 * @brief Captures the full result of a dice roll, including individual die values.
 *
 * Stores every individual die result along with the computed total and any flat
 * modifier applied. Useful for displaying detailed roll breakdowns in UI logs,
 * combat reports, and skill check summaries.
 */
struct RollResult {
    std::vector<i32> individualRolls; ///< Each individual die's face value in order rolled.
    i32 total;                        ///< Sum of individualRolls + modifier.
    i32 modifier;                     ///< Flat modifier added to the sum (e.g., ability modifier).
};

// ==========================================
// STRUCT: DiceRoll
// ==========================================

/**
 * @struct DiceRoll
 * @brief A re-usable dice expression describing how many dice to roll of which size.
 *
 * Represents a classic dice expression like "3d6+2" where:
 *   - numDice is the number of dice (3)
 *   - numSides is the die size (6)
 *   - modifier is the flat bonus (2)
 *
 * The Roll() method computes and returns the total each time it is called.
 */
struct DiceRoll {
    i32 numDice;   ///< Number of dice to roll. Must be >= 0. Zero rolls return 0.
    i32 numSides;  ///< Number of faces per die. Must be >= 1 for a valid roll.
    i32 modifier;  ///< Flat modifier added to the sum after rolling. Can be negative.

    /**
     * @brief Default constructor. Creates an invalid roll (0d0+0).
     */
    DiceRoll()
        : numDice(0), numSides(0), modifier(0) {}

    /**
     * @brief Parameterized constructor.
     * @param numDice Number of dice to roll.
     * @param numSides Number of faces per die.
     * @param modifier Flat modifier added to the total (default 0).
     */
    DiceRoll(i32 numDice, i32 numSides, i32 modifier = 0)
        : numDice(numDice), numSides(numSides), modifier(modifier) {}

    /**
     * @brief Rolls the dice and returns the total.
     *
     * Delegates to DiceRoller::Roll(), which uses the engine's global PRNG.
     * Each call produces an independent roll.
     *
     * @return The sum of all die rolls plus the modifier.
     *         Returns modifier only if numDice == 0.
     *         Returns 0 if numSides == 0.
     */
    i32 Roll() const;
};

// ==========================================
// CLASS: DiceRoller
// ==========================================

/**
 * @class DiceRoller
 * @brief Static utility class providing all common D&D dice rolling methods.
 *
 * DiceRoller manages a single std::mt19937 PRNG instance seeded by Initialize().
 * It provides convenience methods for all standard D&D roll types:
 *   - Basic: Roll(N, S, mod)
 *   - With advantage/disadvantage: RollWithAdvantage, RollWithDisadvantage
 *   - Stat generation: RollStat() (4d6 drop lowest)
 *   - Percentile: RollPercentile() (d100)
 *
 * Usage:
 *   DiceRoller::Initialize();
 *   i32 attack = DiceRoller::Roll(1, 20, 5);       // d20+5
 *   i32 adv    = DiceRoller::RollWithAdvantage(1, 20); // advantage
 *   i32 stat   = DiceRoller::RollStat();              // 4d6 drop lowest
 *   i32 perc   = DiceRoller::RollPercentile();        // d100
 *
 * @note Initialize() must be called before any roll method.
 *       It is safe to call Initialize() multiple times — only the first call seeds.
 * @warning Not thread-safe. All access from a single thread.
 */
class DiceRoller {
public:
    /**
     * @brief Seeds and initializes the global PRNG.
     *
     * Uses std::random_device to obtain a true random seed, then seeds
     * the internal std::mt19937 generator. Subsequent calls are no-ops
     * once the generator has been initialized.
     *
     * Call this once during engine startup, before any dice are rolled.
     */
    static void Initialize();

    /**
     * @brief Rolls dice and returns the total.
     *
     * Rolls `numDice` dice, each with `numSides` faces, sums them, and
     * adds the `modifier`. Returns the final total.
     *
     * @param numDice Number of dice to roll. Must be >= 0.
     * @param numSides Number of faces per die. Must be >= 1.
     * @param modifier Flat bonus or penalty to apply (default 0).
     * @return Sum of all die rolls plus modifier.
     *         Returns 0 if numDice <= 0 or numSides <= 0.
     */
    static i32 Roll(i32 numDice, i32 numSides, i32 modifier = 0);

    /**
     * @brief Rolls dice with advantage (roll twice, take the higher total).
     *
     * Performs two independent rolls of `numDice` d`numSides` and returns
     * the larger total. Used for D&D 5e advantage mechanics.
     *
     * @param numDice Number of dice per roll.
     * @param numSides Number of faces per die.
     * @return The higher of two independent roll totals.
     */
    static i32 RollWithAdvantage(i32 numDice, i32 numSides);

    /**
     * @brief Rolls dice with disadvantage (roll twice, take the lower total).
     *
     * Performs two independent rolls of `numDice` d`numSides` and returns
     * the smaller total. Used for D&D 5e disadvantage mechanics.
     *
     * @param numDice Number of dice per roll.
     * @param numSides Number of faces per die.
     * @return The lower of two independent roll totals.
     */
    static i32 RollWithDisadvantage(i32 numDice, i32 numSides);

    /**
     * @brief Generates a D&D 5e ability score using the 4d6 drop lowest method.
     *
     * Rolls 4 six-sided dice, discards the lowest value, and sums the
     * remaining three. This is the standard method for rolling ability
     * scores during character creation. Expected range: 3-18.
     *
     * @return An ability score between 3 and 18.
     */
    static i32 RollStat();

    /**
     * @brief Rolls percentile dice (d100).
     *
     * Returns a value between 1 and 100 by rolling a d10 for the tens
     * place and another d10 for the ones place. A roll of 00 on both
     * dice is interpreted as 100 (per standard D&D convention).
     *
     * @return A value between 1 and 100 inclusive.
     */
    static i32 RollPercentile();

    /**
     * @brief Returns the detailed result of a dice roll.
     *
     * Similar to Roll() but returns a RollResult struct containing
     * all individual die values, the total, and the modifier, rather
     * than just the total.
     *
     * @param numDice Number of dice to roll.
     * @param numSides Number of faces per die.
     * @param modifier Flat bonus or penalty to apply (default 0).
     * @return RollResult with individual rolls, total, and modifier.
     */
    static RollResult RollDetailed(i32 numDice, i32 numSides, i32 modifier = 0);

private:
    static std::mt19937 s_generator; ///< Global Mersenne Twister PRNG instance.
    static b8 s_initialized;         ///< Whether the generator has been seeded.

    /**
     * @brief Internal: generates a random integer in [1, max] inclusive.
     * @param max Upper bound (inclusive). Must be >= 1.
     * @return Random integer in range [1, max].
     */
    static i32 RollDie(i32 max);
};

} // namespace Dnd
