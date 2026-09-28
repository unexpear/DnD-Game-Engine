/**
 * @file character_class.h
 * @brief D&D character class definitions and database.
 *
 * Defines the 12 standard D&D 5e character classes, their hit dice,
 * primary abilities, skill choices, and spellcasting capabilities.
 * The ClassDatabase provides static lookup for class data by ClassType.
 *
 * Each class defines:
 *   - Hit Die (used for HP calculation at each level)
 *   - Class skills (available skills the character can choose from)
 *   - Number of skill choices at creation
 *   - Spellcasting ability (if the class is a spellcaster)
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
// ENUM: ClassType
// ==========================================

/**
 * @enum ClassType
 * @brief The 12 standard D&D 5e character classes.
 *
 * Each class represents a different adventuring archetype with unique
 * mechanics, hit dice, and role within a party. The list covers all
 * core classes from the Player's Handbook.
 */
enum class ClassType : u32 {
    BARBARIAN  = 0,  ///< Fierce melee combatant. d12 hit die. Relies on STR and CON.
    BARD       = 1,  ///< Versatile spellcaster and support. d8 hit die. Relies on CHA.
    CLERIC     = 2,  ///< Divine spellcaster and healer. d8 hit die. Relies on WIS.
    DRUID      = 3,  ///< Nature-themed spellcaster and shapeshifter. d8 hit die. Relies on WIS.
    FIGHTER    = 4,  ///< Martial combatant, highly versatile. d10 hit die. Relies on STR or DEX.
    MONK       = 5,  ///< Unarmed martial artist. d8 hit die. Relies on DEX and WIS.
    PALADIN    = 6,  ///< Holy warrior with divine magic. d10 hit die. Relies on STR and CHA.
    RANGER     = 7,  ///< Wilderness expert with martial skills. d10 hit die. Relies on DEX and WIS.
    ROGUE      = 8,  ///< Stealth and precision specialist. d8 hit die. Relies on DEX.
    SORCERER   = 9,  ///< Innate spellcaster with metamagic. d6 hit die. Relies on CHA.
    WARLOCK    = 10, ///< Pact-bound spellcaster. d8 hit die. Relies on CHA.
    WIZARD     = 11  ///< Arcane spellcaster with spellbook. d6 hit die. Relies on INT.
};

// ==========================================
// STRUCT: ClassData
// ==========================================

/**
 * @struct ClassData
 * @brief Complete data for a single D&D character class.
 *
 * Contains all class-defining properties: hit die size for HP
 * calculation, the list of available skills for proficiency selection,
 * the number of skills a character can choose at 1st level, and
 * spellcasting information.
 */
struct ClassData {
    ClassType type;                    ///< The class type enum value.
    const char* name;                  ///< Human-readable class name (e.g., "Fighter", "Wizard").
    i32 hitDie;                        ///< Hit die size (e.g., 10 for Fighter = d10).
    std::vector<Skill> classSkills;    ///< Skills available for this class to choose from.
    i32 numSkillChoices;               ///< Number of skills the class can choose at 1st level.
    b8 isSpellcaster;                  ///< Whether this class can cast spells.
    const char* spellcastingAbility;   ///< Ability used for spellcasting (e.g., "INT", "WIS", "CHA").
                                       ///< Empty string "" for non-spellcasters.
};

// ==========================================
// CLASS: ClassDatabase
// ==========================================

/**
 * @class ClassDatabase
 * @brief Static lookup database for class definitions.
 *
 * Provides read-only access to pre-defined class data. All class data
 * is defined at compile time and returned by value. ClassDatabase has
 * no mutable state and is safe to call from any context.
 *
 * Usage:
 *   ClassData fighter = ClassDatabase::GetClass(ClassType::FIGHTER);
 *   for (Skill s : fighter.classSkills) { ... }
 */
class ClassDatabase {
public:
    /**
     * @brief Returns the full ClassData for the given class type.
     *
     * Includes hit die, available skills, skill choice count, and
     * spellcasting information. The returned data is a copy from the
     * internal static table.
     *
     * @param type The class type to look up.
     * @return ClassData struct containing all class properties.
     *         Calling with an invalid ClassType returns Barbarian data.
     */
    static ClassData GetClass(ClassType type);

private:
    /**
     * @brief Internal static table of all class definitions.
     * @return Reference to the compile-time class data array.
     */
    static const ClassData* GetClassTable();
};

} // namespace Dnd
