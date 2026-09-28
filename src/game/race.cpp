/**
 * @file race.cpp
 * @brief Implementation of race data and database lookup.
 *
 * Defines the compile-time race data table with all standard D&D 5e
 * racial traits, ability score modifiers, speeds, and sizes.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "game/race.h"

namespace Dnd {

// ==========================================
// RACE DATA TABLE
// ==========================================

static const RaceData s_raceTable[] = {
    // HUMAN
    {
        RaceType::HUMAN,
        "Human",
        AbilityScores(1, 1, 1, 1, 1, 1),  // +1 to all
        30.0f,
        { "Extra Language" },
        "Medium"
    },
    // ELF
    {
        RaceType::ELF,
        "Elf",
        AbilityScores(0, 2, 0, 0, 0, 0),  // +2 DEX
        30.0f,
        { "Darkvision", "Keen Senses", "Fey Ancestry", "Trance" },
        "Medium"
    },
    // DWARF
    {
        RaceType::DWARF,
        "Dwarf",
        AbilityScores(0, 0, 2, 0, 0, 0),  // +2 CON
        25.0f,
        { "Darkvision", "Dwarven Resilience", "Dwarven Combat Training", "Stonecunning" },
        "Medium"
    },
    // HALFLING
    {
        RaceType::HALFLING,
        "Halfling",
        AbilityScores(0, 2, 0, 0, 0, 0),  // +2 DEX
        25.0f,
        { "Lucky", "Brave", "Halfling Nimbleness" },
        "Small"
    },
    // DRAGONBORN
    {
        RaceType::DRAGONBORN,
        "Dragonborn",
        AbilityScores(2, 0, 0, 0, 0, 1),  // +2 STR, +1 CHA
        30.0f,
        { "Draconic Ancestry", "Breath Weapon", "Damage Resistance" },
        "Medium"
    },
    // GNOME
    {
        RaceType::GNOME,
        "Gnome",
        AbilityScores(0, 0, 0, 2, 0, 0),  // +2 INT
        25.0f,
        { "Darkvision", "Gnome Cunning" },
        "Small"
    },
    // HALF_ELF
    {
        RaceType::HALF_ELF,
        "Half-Elf",
        AbilityScores(0, 0, 0, 0, 0, 2),  // +2 CHA (two +1s chosen at creation)
        30.0f,
        { "Darkvision", "Fey Ancestry", "Skill Versatility" },
        "Medium"
    },
    // HALF_ORC
    {
        RaceType::HALF_ORC,
        "Half-Orc",
        AbilityScores(2, 0, 1, 0, 0, 0),  // +2 STR, +1 CON
        30.0f,
        { "Darkvision", "Relentless Endurance", "Savage Attacks" },
        "Medium"
    },
    // TIEFLING
    {
        RaceType::TIEFLING,
        "Tiefling",
        AbilityScores(0, 0, 0, 1, 0, 2),  // +2 CHA, +1 INT
        30.0f,
        { "Darkvision", "Hellish Resistance", "Infernal Legacy" },
        "Medium"
    }
};

static constexpr i32 s_raceCount = sizeof(s_raceTable) / sizeof(s_raceTable[0]);

// ==========================================
// RaceDatabase Implementation
// ==========================================

const RaceData* RaceDatabase::GetRaceTable() {
    return s_raceTable;
}

RaceData RaceDatabase::GetRace(RaceType type) {
    i32 index = static_cast<i32>(type);
    if (index < 0 || index >= s_raceCount) {
        return s_raceTable[0]; // Default to Human
    }
    return s_raceTable[index];
}

const char* RaceDatabase::GetName(RaceType type) {
    i32 index = static_cast<i32>(type);
    if (index < 0 || index >= s_raceCount) {
        return s_raceTable[0].name;
    }
    return s_raceTable[index].name;
}

} // namespace Dnd
