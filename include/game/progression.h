/**
 * @file progression.h
 * @brief D&D 5e character progression system.
 *
 * Defines leveling, XP tables, proficiency bonus scaling, and hit point
 * progression for characters and creatures. Provides both static utility
 * functions (Progression) and ECS components (Experience, CharacterInfo).
 *
 * The XP and proficiency bonus tables exactly match the D&D 5e Player's
 * Handbook (PHB):
 *   Level  1:      0 XP  / +2 proficiency
 *   Level  2:    300 XP  / +2
 *   Level  3:    900 XP  / +2
 *   Level  4:   2700 XP  / +2
 *   Level  5:   6500 XP  / +3
 *   Level  6:  14000 XP  / +3
 *   Level  7:  23000 XP  / +3
 *   Level  8:  34000 XP  / +3
 *   Level  9:  48000 XP  / +4
 *   Level 10:  64000 XP  / +4
 *   Level 11:  85000 XP  / +4
 *   Level 12: 100000 XP  / +4
 *   Level 13: 120000 XP  / +5
 *   Level 14: 140000 XP  / +5
 *   Level 15: 165000 XP  / +5
 *   Level 16: 195000 XP  / +5
 *   Level 17: 225000 XP  / +6
 *   Level 18: 265000 XP  / +6
 *   Level 19: 305000 XP  / +6
 *   Level 20: 355000 XP  / +6
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include "game/character_class.h"
#include "game/character_stats.h"
#include "game/race.h"
#include <vector>

namespace Dnd {

// ==========================================
// STRUCT: LevelData
// ==========================================

/**
 * @struct LevelData
 * @brief Describes a single character level's properties.
 *
 * Contains the level number, the proficiency bonus at that level,
 * the cumulative experience required to reach it, and a list of
 * feature names the character gains (e.g., "Extra Attack", "Evasion").
 *
 * Usage:
 *   LevelData lvl = { 5, 3, 6500, { "Extra Attack" } };
 */
struct LevelData {
    i32 level;                          ///< Character level (1-20). 1 is the minimum starting level.
    i32 proficiencyBonus;               ///< Proficiency bonus at this level. Scales from +2 to +6.
    i32 experienceRequired;             ///< Cumulative XP required to reach this level from level 1.
    std::vector<const char*> features;  ///< Names of class features gained at this level.

    /**
     * @brief Default constructor. Creates level 1 data with 0 XP, +2 proficiency.
     */
    LevelData()
        : level(1), proficiencyBonus(2), experienceRequired(0) {}

    /**
     * @brief Parameterized constructor.
     * @param level The character level (1-20).
     * @param proficiencyBonus The proficiency bonus at this level.
     * @param experienceRequired Cumulative XP required to reach this level.
     * @param features List of feature names gained at this level (default empty).
     */
    LevelData(i32 level, i32 proficiencyBonus, i32 experienceRequired,
              const std::vector<const char*>& features = {})
        : level(level), proficiencyBonus(proficiencyBonus)
        , experienceRequired(experienceRequired), features(features) {}
};

// ==========================================
// CLASS: Progression
// ==========================================

/**
 * @class Progression
 * @brief Static utility class for D&D 5e progression calculations.
 *
 * Provides lookup and calculation methods for all level-based character
 * progression values: proficiency bonus, experience thresholds, and
 * hit point gains. All data is backed by compile-time tables that
 * exactly match the D&D 5e Player's Handbook.
 *
 * This class has no mutable state and is safe to call from any context.
 *
 * Usage:
 *   i32 prof = Progression::GetProficiencyBonus(5);         // 3
 *   i32 xp   = Progression::GetExperienceRequired(3);       // 900
 *   i32 lvl  = Progression::CalculateLevel(5000);           // 4
 *   i32 hp   = Progression::CalculateMaxHP(ClassType::FIGHTER, 5, 2);
 */
class Progression {
public:
    /**
     * @brief Returns the proficiency bonus for a given character level.
     *
     * Proficiency bonus scales with level per the PHB table:
     *   Levels 1-4:  +2
     *   Levels 5-8:  +3
     *   Levels 9-12: +4
     *   Levels 13-16: +5
     *   Levels 17-20: +6
     *
     * @param level The character level (1-20). Values outside this range
     *              are clamped to the nearest valid level.
     * @return The proficiency bonus (+2 through +6).
     *         Returns +2 for levels below 1, +6 for levels above 20.
     */
    static i32 GetProficiencyBonus(i32 level);

    /**
     * @brief Returns the cumulative XP required to reach a given level.
     *
     * XP thresholds from the PHB:
     *   1=0, 2=300, 3=900, 4=2700, 5=6500, 6=14000, 7=23000, 8=34000,
     *   9=48000, 10=64000, 11=85000, 12=100000, 13=120000, 14=140000,
     *   15=165000, 16=195000, 17=225000, 18=265000, 19=305000, 20=355000.
     *
     * @param level The target level (1-20). Values outside this range
     *              are clamped to the nearest valid level.
     * @return Cumulative XP required. Returns 0 for levels below 1,
     *         355000 for levels above 20.
     */
    static i32 GetExperienceRequired(i32 level);

    /**
     * @brief Determines the current character level from total XP.
     *
     * Finds the highest level whose XP threshold is <= totalXP.
     * For example, 5000 XP returns level 4 (threshold 2700 <= 5000 < 6500).
     *
     * @param totalXP The character's total accumulated XP. Must be >= 0.
     *                Negative values are treated as 0.
     * @return The current level (1-20). Returns 1 if totalXP < 300.
     */
    static i32 CalculateLevel(i32 totalXP);

    /**
     * @brief Returns the HP gained on a single level-up.
     *
     * In average mode, returns (hitDie / 2) + 1 + conModifier (the
     * standard D&D 5e average, rounded up). In roll mode, returns
     * 1d(hitDie) + conModifier.
     *
     * Hit die sizes by class:
     *   d12: Barbarian
     *   d10: Fighter, Paladin, Ranger
     *   d8:  Bard, Cleric, Druid, Monk, Rogue, Warlock
     *   d6:  Sorcerer, Wizard
     *
     * @param classType The class to determine hit die size from.
     * @param conModifier The character's Constitution modifier.
     * @param useAverage If true (default), use the rounded-up average.
     *                   If false, roll the hit die randomly.
     * @return The HP gained. Minimum 1 (even with negative conModifier).
     */
    static i32 GetHitPointsGained(ClassType classType, i32 conModifier, b8 useAverage = true);

    /**
     * @brief Computes the total maximum HP for a character.
     *
     * Level 1 grants the full hit die value + CON modifier. Each
     * subsequent level adds GetHitPointsGained() per level.
     *
     * In average mode, this is deterministic. In roll mode, each
     * level's HP is rolled independently.
     *
     * @param classType The character's class (determines hit die).
     * @param level The character's level (1-20). Levels below 1 return 0.
     * @param conModifier The character's Constitution modifier.
     * @param useAverage If true (default), use averages for all levels.
     *                   If false, roll each level's HP.
     * @return Total maximum HP. Minimum 1.
     */
    static i32 CalculateMaxHP(ClassType classType, i32 level, i32 conModifier, b8 useAverage = true);

private:
    /**
     * @brief Internal proficiency bonus table indexed by level-1.
     *        Length: 20 entries (levels 1-20).
     */
    static const i32 s_proficiencyTable[20];

    /**
     * @brief Internal XP threshold table indexed by level-1.
     *        Length: 20 entries (levels 1-20).
     *        Entry [level-1] = cumulative XP to reach that level.
     */
    static const i32 s_xpTable[20];
};

// ==========================================
// STRUCT: Experience
// ==========================================

/**
 * @struct Experience
 * @brief ECS component tracking a character's experience and level.
 *
 * Stores the total accumulated XP and the current level. The AddXP
 * method automatically handles leveling up when the XP threshold
 * for the next level is reached.
 *
 * Usage:
 *   Experience exp(0);       // Level 1, 0 XP
 *   exp.AddXP(300);          // Now level 2
 *   i32 level = exp.GetCurrentLevel();
 */
struct Experience {
    i32 currentXP; ///< Total accumulated experience points. Must be >= 0.
    i32 level;     ///< Current character level. Tracks the highest level reached.

    /**
     * @brief Default constructor. Creates a level 1 character with 0 XP.
     */
    Experience()
        : currentXP(0), level(1) {}

    /**
     * @brief Parameterized constructor.
     * @param startingXP Initial total XP. Level is auto-calculated.
     *                   Default 0 = level 1. Negative values treated as 0.
     */
    explicit Experience(i32 startingXP)
        : currentXP(startingXP > 0 ? startingXP : 0)
        , level(Progression::CalculateLevel(currentXP)) {}

    /**
     * @brief Adds experience points and auto-levels if thresholds are met.
     *
     * Increases currentXP by the given amount and checks if the new
     * total exceeds the XP threshold for the next level. If so, the
     * level field is updated to match.
     *
     * @param amount The amount of XP to add. Must be >= 0.
     *               Negative values are treated as 0 (no change).
     */
    void AddXP(i32 amount);

    /**
     * @brief Returns the current level based on total XP.
     *
     * Always recalculates from currentXP, which may differ from the
     * stored level field if XP was modified outside of AddXP().
     *
     * @return The character's level (1-20) based on current total XP.
     */
    i32 GetCurrentLevel();
};

// ==========================================
// STRUCT: CharacterInfo
// ==========================================

/**
 * @struct CharacterInfo
 * @brief Full character descriptor ECS component.
 *
 * Stores all core identity and gameplay information for a character
 * or creature: name, race, class, abilities, skills, and derived
 * combat statistics. The constructor auto-calculates max HP, armor
 * class, and proficiency bonus from the provided race, class, and
 * ability scores.
 *
 * Usage:
 *   AbilityScores scores(15, 14, 13, 12, 10, 8);
 *   CharacterInfo info("Grom", RaceType::DWARF, ClassType::FIGHTER, scores);
 *   // info.maxHP, info.armorClass, info.proficiencyBonus auto-calculated
 */
struct CharacterInfo {
    const char* characterName;       ///< Human-readable character name (e.g., "Grom Ironaxe").
    RaceType race;                   ///< The character's race (determines racial traits).
    ClassType classType;             ///< The character's class (determines hit die, features).
    AbilityScores abilities;         ///< The six ability scores (after racial adjustments).
    SkillProficiencies skills;       ///< Tracks which skills the character is proficient in.
    SavingThrowProficiencies saves;  ///< Class (or monster) saving throw proficiencies.
    ToolProficiencies tools;         ///< Tool proficiencies. Thieves' tools is the one that is real.
    i32 level;                       ///< Current character level (1-20).
    b8 isNPC;                        ///< Whether this is a non-player character.
    b8 isHostile;                    ///< Whether this character is hostile to the player party.

    // ---- Derived statistics (auto-calculated by constructor) ----

    i32 maxHP;             ///< Maximum hit points. Calculated from class, level, and CON modifier.
    i32 armorClass;        ///< Armor class. Base 10 + DEX modifier (unarmored baseline).
    i32 proficiencyBonus;  ///< Proficiency bonus. Scaled from class and level.

    /**
     * @brief Full parameterized constructor with auto-calculation.
     *
     * Computes the following derived values:
     *   - maxHP: Progression::CalculateMaxHP(classType, level, CON modifier)
     *   - armorClass: 10 + DEX modifier (unarmored baseline)
     *   - proficiencyBonus: Progression::GetProficiencyBonus(level)
     *
     * @param characterName The character's name. Must remain valid for the component's lifetime.
     * @param race The character's race (affects speed, size, traits).
     * @param classType The character's class (affects hit die, skills).
     * @param abilities The character's ability scores (after racial adjustments).
     * @param skills The character's skill proficiencies (default empty).
     * @param level Starting level (default 1). Must be 1-20.
     * @param isNPC Whether this is an NPC (default false).
     * @param isHostile Whether this character is hostile (default false).
     */
    CharacterInfo(const char* characterName, RaceType race, ClassType classType,
                  const AbilityScores& abilities,
                  const SkillProficiencies& skills = SkillProficiencies(),
                  i32 level = 1, b8 isNPC = false, b8 isHostile = false);
};

} // namespace Dnd
