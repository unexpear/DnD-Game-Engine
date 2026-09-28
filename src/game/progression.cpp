/**
 * @file progression.cpp
 * @brief Implementation of progression tables, leveling, and HP calculations.
 *
 * Contains the compile-time XP and proficiency bonus tables that exactly
 * match the D&D 5e Player's Handbook, along with the logic for level
 * calculation, hit point progression, and the Experience/CharacterInfo
 * ECS component methods.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "game/progression.h"
#include "game/dice.h"
#include <algorithm>

namespace Dnd {

// ==========================================
// STATIC TABLE DATA
// ==========================================

/**
 * @brief Proficiency bonus per level (index = level - 1).
 *
 * D&D 5e PHB progression:
 *   Levels 1-4:  +2
 *   Levels 5-8:  +3
 *   Levels 9-12: +4
 *   Levels 13-16: +5
 *   Levels 17-20: +6
 */
const i32 Progression::s_proficiencyTable[20] = {
    2, 2, 2, 2,   // Levels 1-4
    3, 3, 3, 3,   // Levels 5-8
    4, 4, 4, 4,   // Levels 9-12
    5, 5, 5, 5,   // Levels 13-16
    6, 6, 6, 6    // Levels 17-20
};

/**
 * @brief Cumulative XP required to reach each level (index = level - 1).
 *
 * Values from the D&D 5e PHB "Character Advancement" table.
 * Level 1 requires 0 XP (starting level).
 */
const i32 Progression::s_xpTable[20] = {
        0,      300,      900,     2700,       // Levels 1-4
     6500,    14000,    23000,    34000,       // Levels 5-8
    48000,    64000,    85000,   100000,       // Levels 9-12
   120000,   140000,   165000,   195000,       // Levels 13-16
   225000,   265000,   305000,   355000        // Levels 17-20
};

// ==========================================
// GetProficiencyBonus
// ==========================================

i32 Progression::GetProficiencyBonus(i32 level) {
    if (level < 1) {
        return s_proficiencyTable[0];
    }
    if (level > 20) {
        return s_proficiencyTable[19];
    }
    return s_proficiencyTable[level - 1];
}

// ==========================================
// GetExperienceRequired
// ==========================================

i32 Progression::GetExperienceRequired(i32 level) {
    if (level < 1) {
        return s_xpTable[0];
    }
    if (level > 20) {
        return s_xpTable[19];
    }
    return s_xpTable[level - 1];
}

// ==========================================
// CalculateLevel
// ==========================================

i32 Progression::CalculateLevel(i32 totalXP) {
    if (totalXP <= 0) {
        return 1;
    }

    i32 level = 1;
    for (i32 i = 0; i < 20; i++) {
        if (totalXP >= s_xpTable[i]) {
            level = i + 1;
        } else {
            break;
        }
    }
    return level;
}

// ==========================================
// GetHitPointsGained
// ==========================================

i32 Progression::GetHitPointsGained(ClassType classType, i32 conModifier, b8 useAverage) {
    i32 hitDie = ClassDatabase::GetClass(classType).hitDie;
    i32 gained;

    if (useAverage) {
        // D&D 5e average = (hitDie / 2) + 1, rounded up
        // e.g., d8 -> 4 + 1 = 5, d6 -> 3 + 1 = 4, d12 -> 6 + 1 = 7
        gained = (hitDie / 2) + 1;
    } else {
        // Roll the hit die: 1d(hitDie)
        gained = DiceRoller::Roll(1, hitDie);
    }

    gained += conModifier;

    // Per D&D 5e rules, a character always gains at least 1 HP per level
    return gained < 1 ? 1 : gained;
}

// ==========================================
// CalculateMaxHP
// ==========================================

i32 Progression::CalculateMaxHP(ClassType classType, i32 level, i32 conModifier, b8 useAverage) {
    if (level <= 0) {
        return 0;
    }

    i32 hitDie = ClassDatabase::GetClass(classType).hitDie;

    // Level 1: full hit die + CON modifier
    i32 total = hitDie + conModifier;
    if (total < 1) {
        total = 1;
    }

    // Levels 2+: each level adds GetHitPointsGained
    for (i32 lvl = 2; lvl <= level; lvl++) {
        total += GetHitPointsGained(classType, conModifier, useAverage);
    }

    return total;
}

// ==========================================
// Experience::AddXP
// ==========================================

void Experience::AddXP(i32 amount) {
    if (amount <= 0) {
        return;
    }

    currentXP += amount;

    i32 calculatedLevel = Progression::CalculateLevel(currentXP);
    if (calculatedLevel > level) {
        level = calculatedLevel;
    }
}

// ==========================================
// Experience::GetCurrentLevel
// ==========================================

i32 Experience::GetCurrentLevel() {
    return Progression::CalculateLevel(currentXP);
}

// ==========================================
// CharacterInfo Constructor
// ==========================================

CharacterInfo::CharacterInfo(const char* characterName, RaceType race, ClassType classType,
                             const AbilityScores& abilities,
                             const SkillProficiencies& skills,
                             i32 level, b8 isNPC, b8 isHostile)
    : characterName(characterName)
    , race(race)
    , classType(classType)
    , abilities(abilities)
    , skills(skills)
    , level(level)
    , isNPC(isNPC)
    , isHostile(isHostile)
{
    // Calculate derived statistics
    i32 conMod = AbilityScores::GetModifier(abilities.constitution);
    maxHP = Progression::CalculateMaxHP(classType, level, conMod, true);

    // Base unarmored AC = 10 + DEX modifier
    i32 dexMod = AbilityScores::GetModifier(abilities.dexterity);
    armorClass = 10 + dexMod;

    proficiencyBonus = Progression::GetProficiencyBonus(level);
}

} // namespace Dnd
