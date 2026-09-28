/**
 * @file race.h
 * @brief D&D character race definitions and database.
 *
 * Defines the standard playable races in D&D 5e, their ability score
 * modifiers, movement speeds, sizes, and racial traits. The RaceDatabase
 * provides a static lookup interface for race data by RaceType.
 *
 * Supported races:
 *   Human, Elf, Dwarf, Halfling, Dragonborn, Gnome, Half-Elf,
 *   Half-Orc, Tiefling
 *
 * @note Racial ability score modifiers are already reflected in
 *       RaceData::abilityModifiers. Apply them to base AbilityScores
 *       during character creation.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include "game/character_stats.h"
#include <vector>

namespace Dnd {

// ==========================================
// ENUM: RaceType
// ==========================================

/**
 * @enum RaceType
 * @brief The standard playable races in D&D 5e.
 *
 * Each entry corresponds to a core race from the Player's Handbook.
 * Half-Elf and Half-Orc are treated as distinct races rather than
 * hybrid templates for simplicity in this system.
 */
enum class RaceType : u32 {
    HUMAN       = 0, ///< Versatile, adaptable. +1 to all ability scores.
    ELF         = 1, ///< Graceful, long-lived. +2 DEX, Fey Ancestry, Darkvision.
    DWARF       = 2, ///< Stout, resilient. +2 CON, Darkvision, Dwarven Resilience.
    HALFLING    = 3, ///< Small, lucky. +2 DEX, Lucky trait, Brave.
    DRAGONBORN  = 4, ///< Draconic heritage. +2 STR, +1 CHA, Breath Weapon.
    GNOME       = 5, ///< Inventive, curious. +2 INT, Gnome Cunning, Darkvision.
    HALF_ELF    = 6, ///< Dual heritage. +2 CHA, +1 to two other scores, Fey Ancestry.
    HALF_ORC    = 7, ///< Strong, intimidating. +2 STR, +1 CON, Relentless Endurance.
    TIEFLING    = 8  ///< Infernal bloodline. +2 CHA, +1 INT, Darkvision, Infernal Legacy.
};

// ==========================================
// STRUCT: RaceData
// ==========================================

/**
 * @struct RaceData
 * @brief Complete data for a single D&D race.
 *
 * Contains all racial traits, ability score modifiers, size, speed,
 * and a human-readable name. Used by RaceDatabase to serve race
 * definitions to character creation and game logic systems.
 */
struct RaceData {
    RaceType type;                          ///< The race type enum value.
    const char* name;                       ///< Human-readable race name (e.g., "Elf", "Dwarf").
    AbilityScores abilityModifiers;         ///< Racial ability score adjustments.
    f32 speed;                              ///< Walking speed in feet per round.
    std::vector<const char*> traits;        ///< List of racial trait names (e.g., "Darkvision").
    const char* size;                       ///< Size category: "Medium" or "Small".
};

// ==========================================
// CLASS: RaceDatabase
// ==========================================

/**
 * @class RaceDatabase
 * @brief Static lookup database for race definitions.
 *
 * Provides read-only access to pre-defined race data. All race data
 * is defined at compile time and returned by value. RaceDatabase has
 * no mutable state and is safe to call from any context.
 *
 * Usage:
 *   RaceData elf = RaceDatabase::GetRace(RaceType::ELF);
 *   const char* name = RaceDatabase::GetName(RaceType::ELF);
 */
class RaceDatabase {
public:
    /**
     * @brief Returns the full RaceData for the given race type.
     *
     * Includes ability score modifiers, speed, size, and a list of
     * racial trait names. The returned data is a copy from the
     * internal static table.
     *
     * @param type The race type to look up.
     * @return RaceData struct containing all racial properties.
     *         Calling with an invalid RaceType returns Human data.
     */
    static RaceData GetRace(RaceType type);

    /**
     * @brief Returns the human-readable name for a race type.
     *
     * Convenience method equivalent to GetRace(type).name but without
     * copying the full RaceData struct.
     *
     * @param type The race type to get the name for.
     * @return C-string name of the race. Returns "Human" for invalid types.
     */
    static const char* GetName(RaceType type);

private:
    /**
     * @brief Internal static table of all race definitions.
     * @return Reference to the compile-time race data array.
     */
    static const RaceData* GetRaceTable();
};

} // namespace Dnd
