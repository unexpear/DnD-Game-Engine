/**
 * @file character_class.cpp
 * @brief Implementation of class data and database lookup.
 *
 * Defines the compile-time class data table with all standard D&D 5e
 * class properties: hit dice, skill lists, and spellcasting data.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "game/character_class.h"

namespace Dnd {

// ==========================================
// CLASS DATA TABLE
// ==========================================

static const ClassData s_classTable[] = {
    // BARBARIAN
    {
        ClassType::BARBARIAN,
        "Barbarian",
        12,
        { Skill::ANIMAL_HANDLING, Skill::ATHLETICS, Skill::INTIMIDATION,
          Skill::NATURE, Skill::PERCEPTION, Skill::SURVIVAL },
        2,
        false,
        ""
    },
    // BARD
    {
        ClassType::BARD,
        "Bard",
        8,
        { Skill::ACROBATICS, Skill::ANIMAL_HANDLING, Skill::ARCANA,
          Skill::ATHLETICS, Skill::DECEPTION, Skill::HISTORY,
          Skill::INSIGHT, Skill::INTIMIDATION, Skill::INVESTIGATION,
          Skill::MEDICINE, Skill::NATURE, Skill::PERCEPTION,
          Skill::PERFORMANCE, Skill::PERSUASION, Skill::RELIGION,
          Skill::SLEIGHT_OF_HAND, Skill::STEALTH, Skill::SURVIVAL },
        3,
        true,
        "CHA"
    },
    // CLERIC
    {
        ClassType::CLERIC,
        "Cleric",
        8,
        { Skill::HISTORY, Skill::INSIGHT, Skill::MEDICINE,
          Skill::PERSUASION, Skill::RELIGION },
        2,
        true,
        "WIS"
    },
    // DRUID
    {
        ClassType::DRUID,
        "Druid",
        8,
        { Skill::ARCANA, Skill::ANIMAL_HANDLING, Skill::INSIGHT,
          Skill::MEDICINE, Skill::NATURE, Skill::PERCEPTION,
          Skill::RELIGION, Skill::SURVIVAL },
        2,
        true,
        "WIS"
    },
    // FIGHTER
    {
        ClassType::FIGHTER,
        "Fighter",
        10,
        { Skill::ACROBATICS, Skill::ANIMAL_HANDLING, Skill::ATHLETICS,
          Skill::HISTORY, Skill::INSIGHT, Skill::INTIMIDATION,
          Skill::PERCEPTION, Skill::SURVIVAL },
        2,
        false,
        ""
    },
    // MONK
    {
        ClassType::MONK,
        "Monk",
        8,
        { Skill::ACROBATICS, Skill::ATHLETICS, Skill::HISTORY,
          Skill::INSIGHT, Skill::RELIGION, Skill::STEALTH },
        2,
        false,
        ""
    },
    // PALADIN
    {
        ClassType::PALADIN,
        "Paladin",
        10,
        { Skill::ATHLETICS, Skill::INSIGHT, Skill::INTIMIDATION,
          Skill::MEDICINE, Skill::PERSUASION, Skill::RELIGION },
        2,
        true,
        "CHA"
    },
    // RANGER
    {
        ClassType::RANGER,
        "Ranger",
        10,
        { Skill::ANIMAL_HANDLING, Skill::ATHLETICS, Skill::INSIGHT,
          Skill::INVESTIGATION, Skill::NATURE, Skill::PERCEPTION,
          Skill::STEALTH, Skill::SURVIVAL },
        3,
        true,
        "WIS"
    },
    // ROGUE
    {
        ClassType::ROGUE,
        "Rogue",
        8,
        { Skill::ACROBATICS, Skill::ATHLETICS, Skill::DECEPTION,
          Skill::INSIGHT, Skill::INTIMIDATION, Skill::INVESTIGATION,
          Skill::PERCEPTION, Skill::PERFORMANCE, Skill::PERSUASION,
          Skill::SLEIGHT_OF_HAND, Skill::STEALTH },
        4,
        false,
        ""
    },
    // SORCERER
    {
        ClassType::SORCERER,
        "Sorcerer",
        6,
        { Skill::ARCANA, Skill::DECEPTION, Skill::INSIGHT,
          Skill::INTIMIDATION, Skill::PERSUASION, Skill::RELIGION },
        2,
        true,
        "CHA"
    },
    // WARLOCK
    {
        ClassType::WARLOCK,
        "Warlock",
        8,
        { Skill::ARCANA, Skill::DECEPTION, Skill::HISTORY,
          Skill::INTIMIDATION, Skill::INVESTIGATION, Skill::NATURE,
          Skill::RELIGION },
        2,
        true,
        "CHA"
    },
    // WIZARD
    {
        ClassType::WIZARD,
        "Wizard",
        6,
        { Skill::ARCANA, Skill::HISTORY, Skill::INSIGHT,
          Skill::INVESTIGATION, Skill::MEDICINE, Skill::RELIGION },
        2,
        true,
        "INT"
    }
};

static constexpr i32 s_classCount = sizeof(s_classTable) / sizeof(s_classTable[0]);

// ==========================================
// ClassDatabase Implementation
// ==========================================

const ClassData* ClassDatabase::GetClassTable() {
    return s_classTable;
}

ClassData ClassDatabase::GetClass(ClassType type) {
    i32 index = static_cast<i32>(type);
    if (index < 0 || index >= s_classCount) {
        return s_classTable[0]; // Default to Barbarian
    }
    return s_classTable[index];
}

} // namespace Dnd
