/**
 * @file rules.h
 * @brief D&D 5e check, attack, damage, and armor calculations.
 *
 * This is a slice of the Player's Handbook, not the whole book. It covers
 * the math the demo and the spell system already need:
 *   - d20 checks with advantage and disadvantage (they cancel)
 *   - ability checks and saves (a natural 20 is not an automatic success)
 *   - attack rolls (a natural 20 hits and crits; a natural 1 misses)
 *   - critical damage (extra dice, modifier once) and damage affinity
 *   - armor class from armor category and Dexterity
 *   - spell attack bonus and spell save DC
 *   - sneak attack dice by rogue level
 *   - goblin, skeleton, and bugbear stat blocks
 *
 * Still stubbed: conditions, death saves, concentration, feats, cover,
 * weapon-proficiency lists, and the rest of the class feature list.
 *
 * @author DND Engine Team
 * @date 2026-09-28
 * @version 0.3.0
 */

#pragma once

#include "game/progression.h"
#include "game/item.h"
#include "game/dice.h"

namespace Dnd {

// ==========================================
// ENUM: RollMode
// ==========================================

/**
 * @enum RollMode
 * @brief How many d20s a check rolls, and which face is kept.
 */
enum class RollMode : u32 {
    NORMAL        = 0, ///< One d20.
    ADVANTAGE     = 1, ///< Two d20s, keep the higher.
    DISADVANTAGE  = 2  ///< Two d20s, keep the lower.
};

/**
 * @brief Advantage and disadvantage cancel each other out.
 */
inline RollMode CombineRollMode(b8 advantage, b8 disadvantage) {
    if (advantage == disadvantage) {
        return RollMode::NORMAL;
    }
    return advantage ? RollMode::ADVANTAGE : RollMode::DISADVANTAGE;
}

// ==========================================
// STRUCT: D20Check
// ==========================================

/**
 * @struct D20Check
 * @brief One resolved d20 check, before it is compared to a DC or AC.
 *
 * `natural` is the face that counts. On advantage or disadvantage both
 * faces are stored; `diceRolled` is 1 for a normal roll.
 */
struct D20Check {
    i32 dice[2];
    i32 diceRolled;
    i32 natural;
    i32 modifier;
    i32 total;
    b8 natural20;
    b8 natural1;

    D20Check()
        : diceRolled(1), natural(0), modifier(0), total(0)
        , natural20(false), natural1(false) {
        dice[0] = 0;
        dice[1] = 0;
    }
};

/**
 * @brief Picks the kept d20 and adds the modifier. Does not roll.
 *
 * `second` is ignored when mode is NORMAL.
 */
inline D20Check ResolveD20(i32 first, i32 second, RollMode mode, i32 modifier) {
    D20Check check;
    check.dice[0] = first;
    check.modifier = modifier;
    if (mode == RollMode::NORMAL) {
        check.diceRolled = 1;
        check.natural = first;
    } else {
        check.dice[1] = second;
        check.diceRolled = 2;
        if (mode == RollMode::ADVANTAGE) {
            check.natural = first > second ? first : second;
        } else {
            check.natural = first < second ? first : second;
        }
    }
    check.total = check.natural + modifier;
    check.natural20 = (check.natural == 20);
    check.natural1 = (check.natural == 1);
    return check;
}

/**
 * @brief Rolls a d20 check with the engine dice.
 */
D20Check RollD20Check(RollMode mode, i32 modifier);

/**
 * @brief Ability check or saving throw. Meets the DC on a tie.
 *
 * A natural 20 does not auto-succeed, and a natural 1 does not auto-fail.
 * Those rules are for attack rolls only.
 */
inline b8 CheckSucceeds(const D20Check& check, i32 dc) {
    return check.total >= dc;
}

/**
 * @brief Attack roll against armor class.
 *
 * A natural 20 always hits. A natural 1 always misses. Otherwise the
 * total must meet or beat AC.
 */
inline b8 AttackHits(const D20Check& check, i32 armorClass) {
    if (check.natural20) return true;
    if (check.natural1) return false;
    return check.total >= armorClass;
}

/**
 * @brief True only on a natural 20 that hits. No expanded crit range.
 */
inline b8 IsCriticalHit(const D20Check& check) {
    return check.natural20;
}

// ==========================================
// SKILLS, SAVES, PASSIVE SCORES
// ==========================================

/**
 * @brief The ability a skill uses by default.
 */
Ability SkillAbility(Skill skill);

/**
 * @brief Parses "STR" / "Dexterity" / "wis". Returns false if unknown.
 */
b8 TryParseAbility(const char* text, Ability& out);

/**
 * @brief Parses a PHB skill name, case-insensitive. Returns false if unknown.
 */
b8 TryParseSkill(const char* text, Skill& out);

/**
 * @brief Passive score: 10 + modifier, plus or minus 5 for advantage.
 */
inline i32 PassiveScore(i32 modifier, RollMode mode = RollMode::NORMAL) {
    i32 score = 10 + modifier;
    if (mode == RollMode::ADVANTAGE) score += 5;
    else if (mode == RollMode::DISADVANTAGE) score -= 5;
    return score;
}

/**
 * @brief Writes the two saving throws granted by a PHB class.
 *
 * Clears any previous flags first. Monster stat blocks should not call this.
 */
void ApplyClassSavingThrows(ClassType classType, SavingThrowProficiencies& saves);

/**
 * @struct ContestResult
 * @brief An opposed check. The actor rolls first.
 *
 * A tie does not change the situation: the actor keeps the current state.
 * For Stealth vs Perception, the sneaker is the actor, so a tie stays hidden.
 */
struct ContestResult {
    D20Check actor;
    D20Check opposed;
    b8 tie;

    ContestResult()
        : tie(false) {}
};

ContestResult RollContest(i32 actorBonus, i32 opposedBonus,
                          RollMode actorMode = RollMode::NORMAL,
                          RollMode opposedMode = RollMode::NORMAL);

/**
 * @brief True when the opposed check strictly beats the actor.
 */
inline b8 OpposedCheckBeats(const ContestResult& contest) {
    return contest.opposed.total > contest.actor.total;
}

// ==========================================
// DAMAGE
// ==========================================

/**
 * @enum DamageAffinity
 * @brief How a creature treats one damage type.
 */
enum class DamageAffinity : u32 {
    NORMAL     = 0,
    RESISTANT  = 1, ///< Half damage, round down.
    VULNERABLE = 2, ///< Double damage.
    IMMUNE     = 3  ///< No damage.
};

/**
 * @brief Resistance and vulnerability cancel. Immunity wins over both.
 */
inline DamageAffinity CombineAffinity(b8 resistant, b8 vulnerable, b8 immune) {
    if (immune) return DamageAffinity::IMMUNE;
    if (resistant && vulnerable) return DamageAffinity::NORMAL;
    if (resistant) return DamageAffinity::RESISTANT;
    if (vulnerable) return DamageAffinity::VULNERABLE;
    return DamageAffinity::NORMAL;
}

/**
 * @brief Applies affinity after the damage roll. Input is clamped at 0.
 */
inline i32 ApplyAffinity(i32 damage, DamageAffinity affinity) {
    if (damage < 0) damage = 0;
    switch (affinity) {
        case DamageAffinity::IMMUNE:     return 0;
        case DamageAffinity::RESISTANT:  return damage / 2;
        case DamageAffinity::VULNERABLE: return damage * 2;
        case DamageAffinity::NORMAL:     return damage;
    }
    return damage;
}

/**
 * @brief Adds weapon dice, extra dice (sneak attack), and one flat modifier.
 *
 * On a critical hit the caller passes a second set of dice. The modifier
 * is not doubled. The result is never negative.
 */
inline i32 ComposeDamage(i32 weaponDice, i32 weaponCritDice,
                         i32 extraDice, i32 extraCritDice,
                         i32 flatModifier, b8 critical) {
    i32 total = weaponDice + extraDice + flatModifier;
    if (critical) {
        total += weaponCritDice + extraCritDice;
    }
    return total < 0 ? 0 : total;
}

/**
 * @brief Damage after a saving throw. Half damage rounds down.
 */
inline i32 DamageAfterSave(i32 damage, b8 saved, b8 halfOnSuccess) {
    if (damage < 0) damage = 0;
    if (!saved) return damage;
    if (!halfOnSuccess) return 0;
    return damage / 2;
}

/**
 * @brief Ability modifier used for a weapon attack and its damage.
 *
 * Ranged weapons use Dexterity. Finesse uses whichever of Strength or
 * Dexterity is higher. Other melee weapons use Strength.
 */
inline i32 WeaponAbilityModifier(i32 strengthMod, i32 dexterityMod, b8 finesse, b8 ranged) {
    if (ranged) return dexterityMod;
    if (finesse) return dexterityMod > strengthMod ? dexterityMod : strengthMod;
    return strengthMod;
}

/**
 * @brief Sneak Attack dice (d6s) for a rogue of the given level.
 *
 * 1d6 at 1st level, +1d6 at every odd level after that, cap 10d6.
 * Returns 0 below level 1.
 */
i32 SneakAttackDice(i32 rogueLevel);

// ==========================================
// STRIKES
// ==========================================

/**
 * @struct StrikeSpec
 * @brief Inputs for one weapon or natural attack.
 */
struct StrikeSpec {
    i32 attackBonus;
    RollMode mode;
    i32 targetArmorClass;
    i32 weaponDiceCount;
    i32 weaponDieSize;
    i32 extraDiceCount; ///< Rolled only on a hit. Doubled again on a crit.
    i32 extraDieSize;
    i32 flatModifier;
    DamageAffinity affinity;

    StrikeSpec()
        : attackBonus(0), mode(RollMode::NORMAL), targetArmorClass(10)
        , weaponDiceCount(0), weaponDieSize(0)
        , extraDiceCount(0), extraDieSize(0)
        , flatModifier(0), affinity(DamageAffinity::NORMAL) {}
};

/**
 * @struct StrikeResult
 * @brief The d20, whether it hit, and damage after affinity.
 */
struct StrikeResult {
    D20Check check;
    b8 hit;
    b8 critical;
    i32 damageBeforeAffinity;
    i32 damage;

    StrikeResult()
        : hit(false), critical(false)
        , damageBeforeAffinity(0), damage(0) {}
};

/**
 * @brief Rolls the attack and, on a hit, the damage dice.
 *
 * Roll order: the d20 (two of them with advantage or disadvantage),
 * then weapon dice, then extra dice, then the critical copies of each.
 */
StrikeResult RollStrike(const StrikeSpec& spec);

// ==========================================
// ARMOR CLASS
// ==========================================

/**
 * @brief Armor class from one body armor (or none) and an optional shield.
 *
 * Unarmored is 10 + Dexterity. Light armor adds the full Dexterity
 * modifier. Medium armor adds Dexterity up to the armor's cap; a negative
 * modifier still applies. Heavy armor ignores Dexterity. A shield adds +2.
 * Pass null for no body armor. Do not pass the shield item as `armor`.
 */
i32 ComputeArmorClass(i32 dexterityModifier, const ArmorData* armor, b8 wearingShield);

// ==========================================
// SPELLS
// ==========================================

/**
 * @brief Spell attack bonus: spellcasting ability modifier + proficiency.
 */
inline i32 SpellAttackBonus(i32 spellcastingAbilityMod, i32 proficiencyBonus) {
    return spellcastingAbilityMod + proficiencyBonus;
}

/**
 * @brief Spell save DC: 8 + spellcasting ability modifier + proficiency.
 */
inline i32 SpellSaveDC(i32 spellcastingAbilityMod, i32 proficiencyBonus) {
    return 8 + spellcastingAbilityMod + proficiencyBonus;
}

/**
 * @brief Modifier of the class's spellcasting ability. 0 if it has none.
 */
i32 SpellcastingAbilityModifier(const CharacterInfo& info);

// ==========================================
// MONSTER PROFILES
// ==========================================

/**
 * @enum MonsterKind
 * @brief The three creatures the demo already places in the maze.
 */
enum class MonsterKind : u32 {
    GOBLIN   = 0,
    SKELETON = 1,
    BUGBEAR  = 2
};

/**
 * @struct MonsterMelee
 * @brief The melee attack printed on a monster stat block.
 */
struct MonsterMelee {
    const char* name;
    i32 numDice;
    i32 dieSize;
    DamageType damageType;
    b8 finesse;

    MonsterMelee()
        : name(""), numDice(1), dieSize(4)
        , damageType(DamageType::PIERCING), finesse(false) {}
};

/**
 * @struct MonsterProfile
 * @brief A short Monster Manual stat block for one demo creature.
 *
 * Hit points and AC are the averages from the book, not rolled.
 * Saving throws and condition immunities are not applied yet.
 */
struct MonsterProfile {
    const char* name;
    AbilityScores abilities;
    i32 hitPoints;
    i32 armorClass;
    i32 proficiencyBonus;
    i32 experience;
    i32 stealthBonus;
    i32 perceptionBonus;
    b8 stealthExpertise;
    b8 perceptionProficient;
    MonsterMelee melee;
    const char* weaponItem; ///< ItemDatabase name, or null if the attack is not a PC weapon.

    MonsterProfile()
        : name(""), hitPoints(1), armorClass(10), proficiencyBonus(2)
        , experience(0), stealthBonus(0), perceptionBonus(0)
        , stealthExpertise(false), perceptionProficient(false)
        , weaponItem(nullptr) {}
};

MonsterProfile GetMonsterProfile(MonsterKind kind);

/**
 * @brief Ability modifier the melee attack uses (finesse takes the higher).
 */
i32 MonsterDamageModifier(const MonsterProfile& profile);

/**
 * @brief To-hit bonus: damage ability modifier + the block's proficiency bonus.
 */
i32 MonsterAttackBonus(const MonsterProfile& profile);

/**
 * @brief Damage affinity from the stat block. Goblins and bugbears have none.
 *
 * Skeletons are vulnerable to bludgeoning and immune to poison.
 * They are not resistant to piercing.
 */
DamageAffinity MonsterDamageAffinity(const MonsterProfile& profile, DamageType type);

/**
 * @brief Copies the profile's skill training onto a proficiency set.
 */
void ApplyMonsterSkills(const MonsterProfile& profile, SkillProficiencies& skills);

} // namespace Dnd
