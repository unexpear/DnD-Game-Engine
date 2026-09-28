/**
 * @file rules.cpp
 * @brief D&D 5e check, attack, armor, and monster-profile implementation.
 *
 * See rules.h for what this slice covers and what is still stubbed.
 *
 * @author DND Engine Team
 * @date 2026-09-28
 * @version 0.3.0
 */

#include "game/rules.h"
#include <cstring>
#include <cctype>

namespace Dnd {

namespace {

struct SkillName {
    Skill skill;
    const char* name;
};

const SkillName kSkillNames[] = {
    { Skill::ACROBATICS,      "acrobatics" },
    { Skill::ANIMAL_HANDLING, "animal handling" },
    { Skill::ARCANA,          "arcana" },
    { Skill::ATHLETICS,       "athletics" },
    { Skill::DECEPTION,       "deception" },
    { Skill::HISTORY,         "history" },
    { Skill::INSIGHT,         "insight" },
    { Skill::INTIMIDATION,    "intimidation" },
    { Skill::INVESTIGATION,   "investigation" },
    { Skill::MEDICINE,        "medicine" },
    { Skill::NATURE,          "nature" },
    { Skill::PERCEPTION,      "perception" },
    { Skill::PERFORMANCE,     "performance" },
    { Skill::PERSUASION,      "persuasion" },
    { Skill::RELIGION,        "religion" },
    { Skill::SLEIGHT_OF_HAND, "sleight of hand" },
    { Skill::STEALTH,         "stealth" },
    { Skill::SURVIVAL,        "survival" }
};

void CopyLower(const char* text, char* dest, i32 destSize) {
    i32 i = 0;
    if (!text) {
        dest[0] = '\0';
        return;
    }
    for (; text[i] != '\0' && i < destSize - 1; ++i) {
        dest[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    }
    dest[i] = '\0';
}

MonsterMelee MakeMelee(const char* name, i32 numDice, i32 dieSize, DamageType type, b8 finesse) {
    MonsterMelee melee;
    melee.name = name;
    melee.numDice = numDice;
    melee.dieSize = dieSize;
    melee.damageType = type;
    melee.finesse = finesse;
    return melee;
}

} // namespace

// ==========================================
// D20
// ==========================================

D20Check RollD20Check(RollMode mode, i32 modifier) {
    const i32 first = DiceRoller::Roll(1, 20);
    i32 second = 0;
    if (mode != RollMode::NORMAL) {
        second = DiceRoller::Roll(1, 20);
    }
    return ResolveD20(first, second, mode, modifier);
}

Ability SkillAbility(Skill skill) {
    switch (skill) {
        case Skill::ATHLETICS:
            return Ability::STRENGTH;
        case Skill::ACROBATICS:
        case Skill::SLEIGHT_OF_HAND:
        case Skill::STEALTH:
            return Ability::DEXTERITY;
        case Skill::ARCANA:
        case Skill::HISTORY:
        case Skill::INVESTIGATION:
        case Skill::NATURE:
        case Skill::RELIGION:
            return Ability::INTELLIGENCE;
        case Skill::ANIMAL_HANDLING:
        case Skill::INSIGHT:
        case Skill::MEDICINE:
        case Skill::PERCEPTION:
        case Skill::SURVIVAL:
            return Ability::WISDOM;
        case Skill::DECEPTION:
        case Skill::INTIMIDATION:
        case Skill::PERFORMANCE:
        case Skill::PERSUASION:
            return Ability::CHARISMA;
    }
    return Ability::WISDOM;
}

b8 TryParseAbility(const char* text, Ability& out) {
    char lower[32];
    CopyLower(text, lower, static_cast<i32>(sizeof(lower)));
    if (std::strcmp(lower, "str") == 0 || std::strcmp(lower, "strength") == 0) {
        out = Ability::STRENGTH; return true;
    }
    if (std::strcmp(lower, "dex") == 0 || std::strcmp(lower, "dexterity") == 0) {
        out = Ability::DEXTERITY; return true;
    }
    if (std::strcmp(lower, "con") == 0 || std::strcmp(lower, "constitution") == 0) {
        out = Ability::CONSTITUTION; return true;
    }
    if (std::strcmp(lower, "int") == 0 || std::strcmp(lower, "intelligence") == 0) {
        out = Ability::INTELLIGENCE; return true;
    }
    if (std::strcmp(lower, "wis") == 0 || std::strcmp(lower, "wisdom") == 0) {
        out = Ability::WISDOM; return true;
    }
    if (std::strcmp(lower, "cha") == 0 || std::strcmp(lower, "charisma") == 0) {
        out = Ability::CHARISMA; return true;
    }
    return false;
}

b8 TryParseSkill(const char* text, Skill& out) {
    char lower[40];
    CopyLower(text, lower, static_cast<i32>(sizeof(lower)));
    const i32 count = static_cast<i32>(sizeof(kSkillNames) / sizeof(kSkillNames[0]));
    for (i32 i = 0; i < count; ++i) {
        if (std::strcmp(lower, kSkillNames[i].name) == 0) {
            out = kSkillNames[i].skill;
            return true;
        }
    }
    return false;
}

void ApplyClassSavingThrows(ClassType classType, SavingThrowProficiencies& saves) {
    saves = SavingThrowProficiencies();
    Ability first = Ability::STRENGTH;
    Ability second = Ability::CONSTITUTION;
    switch (classType) {
        case ClassType::BARBARIAN: first = Ability::STRENGTH;     second = Ability::CONSTITUTION; break;
        case ClassType::BARD:      first = Ability::DEXTERITY;    second = Ability::CHARISMA;     break;
        case ClassType::CLERIC:    first = Ability::WISDOM;       second = Ability::CHARISMA;     break;
        case ClassType::DRUID:     first = Ability::INTELLIGENCE; second = Ability::WISDOM;       break;
        case ClassType::FIGHTER:   first = Ability::STRENGTH;     second = Ability::CONSTITUTION; break;
        case ClassType::MONK:      first = Ability::STRENGTH;     second = Ability::DEXTERITY;    break;
        case ClassType::PALADIN:   first = Ability::WISDOM;       second = Ability::CHARISMA;     break;
        case ClassType::RANGER:    first = Ability::STRENGTH;     second = Ability::DEXTERITY;    break;
        case ClassType::ROGUE:     first = Ability::DEXTERITY;    second = Ability::INTELLIGENCE; break;
        case ClassType::SORCERER:  first = Ability::CONSTITUTION; second = Ability::CHARISMA;     break;
        case ClassType::WARLOCK:   first = Ability::WISDOM;       second = Ability::CHARISMA;     break;
        case ClassType::WIZARD:    first = Ability::INTELLIGENCE; second = Ability::WISDOM;       break;
    }
    saves.SetProficient(first, true);
    saves.SetProficient(second, true);
}

ContestResult RollContest(i32 actorBonus, i32 opposedBonus, RollMode actorMode, RollMode opposedMode) {
    ContestResult result;
    result.actor = RollD20Check(actorMode, actorBonus);
    result.opposed = RollD20Check(opposedMode, opposedBonus);
    result.tie = (result.actor.total == result.opposed.total);
    return result;
}

// ==========================================
// DAMAGE AND STRIKES
// ==========================================

i32 SneakAttackDice(i32 rogueLevel) {
    if (rogueLevel < 1) return 0;
    i32 dice = (rogueLevel + 1) / 2;
    if (dice > 10) dice = 10;
    return dice;
}

namespace {

i32 RollPool(i32 count, i32 sides) {
    if (count <= 0 || sides <= 0) return 0;
    return DiceRoller::Roll(count, sides, 0);
}

} // namespace

StrikeResult RollStrike(const StrikeSpec& spec) {
    StrikeResult result;
    result.check = RollD20Check(spec.mode, spec.attackBonus);
    result.hit = AttackHits(result.check, spec.targetArmorClass);
    result.critical = result.hit && IsCriticalHit(result.check);
    if (!result.hit) {
        return result;
    }

    const i32 weapon = RollPool(spec.weaponDiceCount, spec.weaponDieSize);
    const i32 extra = RollPool(spec.extraDiceCount, spec.extraDieSize);
    i32 weaponCrit = 0;
    i32 extraCrit = 0;
    if (result.critical) {
        weaponCrit = RollPool(spec.weaponDiceCount, spec.weaponDieSize);
        extraCrit = RollPool(spec.extraDiceCount, spec.extraDieSize);
    }
    result.damageBeforeAffinity = ComposeDamage(
        weapon, weaponCrit, extra, extraCrit, spec.flatModifier, result.critical);
    result.damage = ApplyAffinity(result.damageBeforeAffinity, spec.affinity);
    return result;
}

// ==========================================
// ARMOR
// ==========================================

i32 ComputeArmorClass(i32 dexterityModifier, const ArmorData* armor, b8 wearingShield) {
    i32 ac;
    if (!armor || armor->type == ArmorType::SHIELD) {
        ac = 10 + dexterityModifier;
    } else if (armor->type == ArmorType::HEAVY) {
        ac = armor->baseAC;
    } else if (armor->type == ArmorType::MEDIUM) {
        i32 dex = dexterityModifier;
        if (dex > armor->maxDexBonus) dex = armor->maxDexBonus;
        ac = armor->baseAC + dex;
    } else {
        ac = armor->baseAC + dexterityModifier;
    }
    if (wearingShield) ac += 2;
    return ac;
}

// ==========================================
// SPELLS
// ==========================================

i32 SpellcastingAbilityModifier(const CharacterInfo& info) {
    const ClassData cls = ClassDatabase::GetClass(info.classType);
    if (!cls.isSpellcaster || !cls.spellcastingAbility || cls.spellcastingAbility[0] == '\0') {
        return 0;
    }
    Ability ability = Ability::INTELLIGENCE;
    if (!TryParseAbility(cls.spellcastingAbility, ability)) {
        return 0;
    }
    return GetAbilityModifier(info.abilities, ability);
}

// ==========================================
// MONSTERS
// ==========================================

MonsterProfile GetMonsterProfile(MonsterKind kind) {
    MonsterProfile profile;
    switch (kind) {
        case MonsterKind::SKELETON:
            // CR 1/4. Vulnerable to bludgeoning, immune to poison.
            profile.name = "skeleton";
            profile.abilities = AbilityScores(10, 14, 15, 6, 8, 5);
            profile.hitPoints = 13;
            profile.armorClass = 13;
            profile.proficiencyBonus = 2;
            profile.experience = 50;
            profile.stealthBonus = 2;       // Dexterity only; no Stealth proficiency
            profile.perceptionBonus = -1;   // Wisdom 8
            profile.stealthExpertise = false;
            profile.perceptionProficient = false;
            profile.melee = MakeMelee("shortsword", 1, 6, DamageType::PIERCING, true);
            profile.weaponItem = "Shortsword";
            break;
        case MonsterKind::BUGBEAR:
            // CR 1. Morningstar is 2d8+2 on the stat block, not the PC weapon's 1d8.
            profile.name = "bugbear";
            profile.abilities = AbilityScores(15, 14, 13, 8, 11, 9);
            profile.hitPoints = 27;
            profile.armorClass = 16;
            profile.proficiencyBonus = 2;
            profile.experience = 200;
            profile.stealthBonus = 6;       // Dexterity +2 and expertise-equivalent training
            profile.perceptionBonus = 0;    // Wisdom 11, not proficient
            profile.stealthExpertise = true;
            profile.perceptionProficient = false;
            profile.melee = MakeMelee("morningstar", 2, 8, DamageType::PIERCING, false);
            profile.weaponItem = nullptr;
            break;
        case MonsterKind::GOBLIN:
        default:
            // CR 1/4. Scimitar +4, 1d6+2 slashing. Stealth +6. Passive Perception 9.
            profile.name = "goblin";
            profile.abilities = AbilityScores(8, 14, 10, 10, 8, 8);
            profile.hitPoints = 7;
            profile.armorClass = 15;
            profile.proficiencyBonus = 2;
            profile.experience = 50;
            profile.stealthBonus = 6;
            profile.perceptionBonus = -1;
            profile.stealthExpertise = true;
            profile.perceptionProficient = false;
            profile.melee = MakeMelee("scimitar", 1, 6, DamageType::SLASHING, true);
            profile.weaponItem = "Scimitar";
            break;
    }
    return profile;
}

i32 MonsterDamageModifier(const MonsterProfile& profile) {
    const i32 strength = AbilityScores::GetModifier(profile.abilities.strength);
    const i32 dexterity = AbilityScores::GetModifier(profile.abilities.dexterity);
    return WeaponAbilityModifier(strength, dexterity, profile.melee.finesse, false);
}

i32 MonsterAttackBonus(const MonsterProfile& profile) {
    return MonsterDamageModifier(profile) + profile.proficiencyBonus;
}

DamageAffinity MonsterDamageAffinity(const MonsterProfile& profile, DamageType type) {
    // Identified by name so a copied profile still matches. Only the skeleton
    // block in this slice has affinities.
    if (!profile.name || std::strcmp(profile.name, "skeleton") != 0) {
        return DamageAffinity::NORMAL;
    }
    if (type == DamageType::BLUDGEONING) return DamageAffinity::VULNERABLE;
    if (type == DamageType::POISON) return DamageAffinity::IMMUNE;
    return DamageAffinity::NORMAL;
}

void ApplyMonsterSkills(const MonsterProfile& profile, SkillProficiencies& skills) {
    if (profile.stealthExpertise) {
        skills.AddExpertise(Skill::STEALTH);
    }
    if (profile.perceptionProficient) {
        skills.AddProficiency(Skill::PERCEPTION);
    }
}

} // namespace Dnd
