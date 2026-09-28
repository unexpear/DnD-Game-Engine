/**
 * @file character_stats.h
 * @brief D&D character ability scores, skills, and proficiency systems.
 *
 * This module defines the core character statistics used throughout the D&D
 * game system: the six ability scores (Strength, Dexterity, Constitution,
 * Intelligence, Wisdom, Charisma), their derived modifiers, the standard
 * skill list, and proficiency tracking.
 *
 * Modifier calculation follows D&D 5e rules:
 *   modifier = (score - 10) / 2 (integer division, rounding down)
 *
 * @note Ability scores typically range from 3-20 for player characters,
 *       but the system supports any non-negative value for monsters.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include <unordered_map>

namespace Dnd {

// ==========================================
// STRUCT: AbilityScores
// ==========================================

/**
 * @struct AbilityScores
 * @brief The six core D&D ability scores for a character or creature.
 *
 * Each ability score ranges from 0 to 30 in standard D&D, though most
 * player characters fall in the 3-20 range. Each score has an associated
 * modifier computed as (score - 10) / 2, used for saving throws, skill
 * checks, and attack rolls.
 *
 * Default constructor creates all-10s (no modifiers), suitable for
 * commoners or as a baseline before racial bonuses are applied.
 */
struct AbilityScores {
    i32 strength;      ///< Strength (STR): Melee attacks, athletics, carrying capacity.
    i32 dexterity;     ///< Dexterity (DEX): Initiative, AC, ranged attacks, acrobatics, stealth.
    i32 constitution;  ///< Constitution (CON): Hit points, concentration saves.
    i32 intelligence;  ///< Intelligence (INT): Arcana, history, investigation, nature, religion.
    i32 wisdom;        ///< Wisdom (WIS): Perception, insight, medicine, survival, animal handling.
    i32 charisma;      ///< Charisma (CHA): Persuasion, deception, intimidation, performance.

    /**
     * @brief Default constructor. Sets all scores to 10 (commoner baseline).
     */
    AbilityScores()
        : strength(10), dexterity(10), constitution(10)
        , intelligence(10), wisdom(10), charisma(10) {}

    /**
     * @brief Parameterized constructor.
     * @param strength Strength score.
     * @param dexterity Dexterity score.
     * @param constitution Constitution score.
     * @param intelligence Intelligence score.
     * @param wisdom Wisdom score.
     * @param charisma Charisma score.
     */
    AbilityScores(i32 strength, i32 dexterity, i32 constitution,
                  i32 intelligence, i32 wisdom, i32 charisma)
        : strength(strength), dexterity(dexterity), constitution(constitution)
        , intelligence(intelligence), wisdom(wisdom), charisma(charisma) {}

    /**
     * @brief Computes the ability modifier for a given score.
     *
     * Standard D&D 5e formula: modifier = (score - 10) / 2.
     * Integer division truncates toward zero, which produces the correct
     * D&D behavior (e.g., score 8 -> -1, score 9 -> -1, score 10 -> 0).
     *
     * @param score The ability score to compute the modifier for.
     * @return The ability modifier (typically in range [-5, +10]).
     */
    static i32 GetModifier(i32 score) {
        return (score - 10) / 2;
    }

    /**
     * @struct AbilityModifiers
     * @brief Holds all six computed ability modifiers.
     *
     * Returned by GetAllModifiers() for batch processing of modifier values,
     * useful for UI display, saving throw calculations, and skill checks.
     */
    struct AbilityModifiers {
        i32 strength;      ///< Strength modifier.
        i32 dexterity;     ///< Dexterity modifier.
        i32 constitution;  ///< Constitution modifier.
        i32 intelligence;  ///< Intelligence modifier.
        i32 wisdom;        ///< Wisdom modifier.
        i32 charisma;      ///< Charisma modifier.
    };

    /**
     * @brief Computes all six ability modifiers at once.
     *
     * Each modifier is computed using GetModifier() on the corresponding
     * ability score. Returns them as a struct for convenient use in
     * saving throw and skill check calculations.
     *
     * @return AbilityModifiers struct containing all six modifiers.
     */
    AbilityModifiers GetAllModifiers() const {
        AbilityModifiers mods;
        mods.strength     = GetModifier(strength);
        mods.dexterity    = GetModifier(dexterity);
        mods.constitution = GetModifier(constitution);
        mods.intelligence = GetModifier(intelligence);
        mods.wisdom       = GetModifier(wisdom);
        mods.charisma     = GetModifier(charisma);
        return mods;
    }
};

// ==========================================
// ENUM: Skill
// ==========================================

/**
 * @enum Skill
 * @brief The 18 standard D&D 5e skills, each tied to a specific ability score.
 *
 * Skill-to-ability mapping (for reference):
 *   - Acrobatics (DEX)
 *   - Animal Handling (WIS)
 *   - Arcana (INT)
 *   - Athletics (STR)
 *   - Deception (CHA)
 *   - History (INT)
 *   - Insight (WIS)
 *   - Intimidation (CHA)
 *   - Investigation (INT)
 *   - Medicine (WIS)
 *   - Nature (INT)
 *   - Perception (WIS)
 *   - Performance (CHA)
 *   - Persuasion (CHA)
 *   - Religion (INT)
 *   - Sleight of Hand (DEX)
 *   - Stealth (DEX)
 *   - Survival (WIS)
 */
enum class Skill : u32 {
    ACROBATICS       = 0,  ///< Dexterity. Balance, tumbling, escapes.
    ANIMAL_HANDLING  = 1,  ///< Wisdom. Calming, training, riding animals.
    ARCANA           = 2,  ///< Intelligence. Magic knowledge, spells, artifacts.
    ATHLETICS        = 3,  ///< Strength. Climbing, swimming, jumping, grappling.
    DECEPTION        = 4,  ///< Charisma. Lying, disguises, misdirection.
    HISTORY          = 5,  ///< Intelligence. Lore, past events, kingdoms.
    INSIGHT          = 6,  ///< Wisdom. Reading motives, detecting lies.
    INTIMIDATION     = 7,  ///< Charisma. Threats, coercion, frightening.
    INVESTIGATION    = 8,  ///< Intelligence. Searching for clues, deduction.
    MEDICINE         = 9,  ///< Wisdom. Diagnostics, treating wounds, poisons.
    NATURE           = 10, ///< Intelligence. Natural lore, weather, creatures.
    PERCEPTION       = 11, ///< Wisdom. Spotting, hearing, noticing details.
    PERFORMANCE      = 12, ///< Charisma. Acting, music, storytelling.
    PERSUASION       = 13, ///< Charisma. Convincing, negotiation, charm.
    RELIGION         = 14, ///< Intelligence. Deities, rites, holy symbols.
    SLEIGHT_OF_HAND  = 15, ///< Dexterity. Pickpocketing, lockpicking, legerdemain.
    STEALTH          = 16, ///< Dexterity. Sneaking, hiding, moving silently.
    SURVIVAL         = 17  ///< Wisdom. Tracking, foraging, navigating, shelter.
};

// ==========================================
// STRUCT: SkillProficiencies
// ==========================================

/**
 * @struct SkillProficiencies
 * @brief Tracks which skills a character is proficient in and computes skill bonuses.
 *
 * Skill bonus calculation (D&D 5e):
 *   skillBonus = abilityModifier + (proficiency ? proficiencyBonus : 0)
 *
 * This struct stores only the proficiency flags. The actual proficiency
 * bonus value (which scales with character level) is expected to be
 * provided externally or stored in the character's level data.
 *
 * Usage:
 *   SkillProficiencies profs;
 *   profs.proficiencies[Skill::STEALTH] = true;
 *   i32 bonus = profs.GetBonus(Skill::STEALTH, abilityScores, 3); // +3 prof bonus
 */
struct SkillProficiencies {
    /**
     * @brief Map from Skill to boolean proficiency flag.
     *
     * true = proficient, false = not proficient.
     * Skills not present in the map are treated as not proficient.
     * Use HasProficiency() to safely check without default-inserting.
     */
    std::unordered_map<Skill, b8> proficiencies;

    /**
     * @brief Checks if the character is proficient in a given skill.
     * @param skill The skill to check.
     * @return true if proficient, false otherwise.
     */
    b8 HasProficiency(Skill skill) const {
        auto it = proficiencies.find(skill);
        return it != proficiencies.end() && it->second;
    }

    /**
     * @brief Computes the total skill bonus for a given skill.
     *
     * Formula: abilityModifier + (proficient ? proficiencyBonus : 0).
     * The ability associated with each skill is provided via the
     * ability lookup function, which returns the relevant modifier.
     *
     * @param skill The skill to compute the bonus for.
     * @param abilityScores The character's ability scores (used to get modifier).
     * @param proficiencyBonus The character's global proficiency bonus (level-dependent).
     * @return Total skill check bonus.
     */
    i32 GetBonus(Skill skill, const AbilityScores& abilityScores, i32 proficiencyBonus) const {
        // Map skill to its associated ability score
        i32 abilityModifier = 0;
        switch (skill) {
            case Skill::ACROBATICS:
            case Skill::SLEIGHT_OF_HAND:
            case Skill::STEALTH:
                abilityModifier = AbilityScores::GetModifier(abilityScores.dexterity);
                break;
            case Skill::ANIMAL_HANDLING:
            case Skill::INSIGHT:
            case Skill::MEDICINE:
            case Skill::PERCEPTION:
            case Skill::SURVIVAL:
                abilityModifier = AbilityScores::GetModifier(abilityScores.wisdom);
                break;
            case Skill::ARCANA:
            case Skill::HISTORY:
            case Skill::INVESTIGATION:
            case Skill::NATURE:
            case Skill::RELIGION:
                abilityModifier = AbilityScores::GetModifier(abilityScores.intelligence);
                break;
            case Skill::ATHLETICS:
                abilityModifier = AbilityScores::GetModifier(abilityScores.strength);
                break;
            case Skill::DECEPTION:
            case Skill::INTIMIDATION:
            case Skill::PERFORMANCE:
            case Skill::PERSUASION:
                abilityModifier = AbilityScores::GetModifier(abilityScores.charisma);
                break;
        }

        // Add proficiency bonus if proficient
        if (HasProficiency(skill)) {
            abilityModifier += proficiencyBonus;
        }

        return abilityModifier;
    }

    /**
     * @brief Grants proficiency in a single skill.
     * @param skill The skill to become proficient in.
     */
    void AddProficiency(Skill skill) {
        proficiencies[skill] = true;
    }

    /**
     * @brief Removes proficiency from a single skill.
     * @param skill The skill to remove proficiency from.
     */
    void RemoveProficiency(Skill skill) {
        proficiencies[skill] = false;
    }

    /**
     * @brief Returns the number of skills this character is proficient in.
     * @return Count of proficient skills.
     */
    i32 GetProficientCount() const {
        i32 count = 0;
        for (const auto& [skill, prof] : proficiencies) {
            DND_UNUSED(skill);
            if (prof) count++;
        }
        return count;
    }
};

} // namespace Dnd
