/**
 * @file dice.cpp
 * @brief Implementation of dice rolling and randomization.
 *
 * Implements the DiceRoller singleton PRNG, individual die rolls,
 * advantage/disadvantage mechanics, stat generation (4d6 drop lowest),
 * and percentile dice. All random state is managed through a single
 * std::mt19937 instance seeded via std::random_device.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "game/dice.h"
#include <algorithm>
#include <numeric>

namespace Dnd {

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

std::mt19937 DiceRoller::s_generator;
b8 DiceRoller::s_initialized = false;

// ==========================================
// DiceRoll::Roll
// ==========================================

i32 DiceRoll::Roll() const {
    return DiceRoller::Roll(numDice, numSides, modifier);
}

// ==========================================
// DiceRoller::Initialize
// ==========================================

void DiceRoller::Initialize() {
    if (s_initialized) {
        return;
    }
    std::random_device rd;
    s_generator.seed(rd());
    s_initialized = true;
}

// ==========================================
// DiceRoller::RollDie
// ==========================================

i32 DiceRoller::RollDie(i32 max) {
    if (!s_initialized) {
        Initialize();
    }
    if (max < 1) {
        return 0;
    }
    std::uniform_int_distribution<i32> dist(1, max);
    return dist(s_generator);
}

// ==========================================
// DiceRoller::Roll
// ==========================================

i32 DiceRoller::Roll(i32 numDice, i32 numSides, i32 modifier) {
    if (numDice <= 0 || numSides <= 0) {
        return modifier;
    }

    i32 total = 0;
    for (i32 i = 0; i < numDice; i++) {
        total += RollDie(numSides);
    }
    return total + modifier;
}

// ==========================================
// DiceRoller::RollWithAdvantage
// ==========================================

i32 DiceRoller::RollWithAdvantage(i32 numDice, i32 numSides) {
    i32 roll1 = Roll(numDice, numSides);
    i32 roll2 = Roll(numDice, numSides);
    return std::max(roll1, roll2);
}

// ==========================================
// DiceRoller::RollWithDisadvantage
// ==========================================

i32 DiceRoller::RollWithDisadvantage(i32 numDice, i32 numSides) {
    i32 roll1 = Roll(numDice, numSides);
    i32 roll2 = Roll(numDice, numSides);
    return std::min(roll1, roll2);
}

// ==========================================
// DiceRoller::RollStat
// ==========================================

i32 DiceRoller::RollStat() {
    // Roll 4d6 and drop the lowest
    i32 rolls[4];
    for (i32 i = 0; i < 4; i++) {
        rolls[i] = RollDie(6);
    }

    // Find and sum the three highest values
    i32 lowestIndex = 0;
    for (i32 i = 1; i < 4; i++) {
        if (rolls[i] < rolls[lowestIndex]) {
            lowestIndex = i;
        }
    }

    i32 total = 0;
    for (i32 i = 0; i < 4; i++) {
        if (i != lowestIndex) {
            total += rolls[i];
        }
    }
    return total;
}

// ==========================================
// DiceRoller::RollPercentile
// ==========================================

i32 DiceRoller::RollPercentile() {
    i32 tens = RollDie(10) - 1; // 0-9
    i32 ones = RollDie(10);     // 1-10

    i32 result = tens * 10 + ones;
    // Per D&D convention: 00 on tens and 0 on ones = 100
    if (result == 0) {
        return 100;
    }
    // If tens is 0 (00) and ones is 1-10, result is 1-10
    // If ones is 10 and tens is 1-9, result is tens*10 + 10 = (tens+1)*10
    // which is already correct since ones=10 gives us 10-90
    // But if tens is 0 and ones is 10, result = 0*10 + 10 = 10, which is correct.
    return result;
}

// ==========================================
// DiceRoller::RollDetailed
// ==========================================

RollResult DiceRoller::RollDetailed(i32 numDice, i32 numSides, i32 modifier) {
    RollResult result;
    result.modifier = modifier;
    result.total = modifier;

    if (numDice > 0 && numSides > 0) {
        result.individualRolls.reserve(numDice);
        for (i32 i = 0; i < numDice; i++) {
            i32 roll = RollDie(numSides);
            result.individualRolls.push_back(roll);
            result.total += roll;
        }
    }

    return result;
}

} // namespace Dnd
