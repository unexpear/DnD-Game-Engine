/**
 * @file rules_test.cpp
 * @brief Locks the 5e check, attack, damage, armor, and spell-slot math.
 *
 * Build (from repo root):
 *   g++ -std=c++17 -Wall -Wextra -Iinclude -Isrc \
 *       tests/rules_test.cpp \
 *       src/game/rules.cpp src/game/dice.cpp src/game/spell.cpp \
 *       src/game/character_class.cpp src/game/progression.cpp src/game/item.cpp \
 *       src/ecs/system.cpp src/ecs/entity.cpp src/core/log.cpp \
 *       -o build/rules_test
 *
 * Exit code: 0 = all checks passed, 1 = one or more failures.
 */

#include "game/rules.h"
#include "game/spell.h"
#include "game/combat_stats.h"
#include "game/item.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/system.h"
#include "core/log.h"

#include <cstdio>
#include <memory>

using namespace Dnd;

static int g_failures = 0;

static void Check(bool condition, const char* description) {
    std::printf("  [%s] %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) {
        ++g_failures;
    }
}

static void TestModifiers() {
    std::printf("\n-- Ability modifiers (floor division) --\n");
    Check(AbilityScores::GetModifier(1) == -5, "score 1 -> -5");
    Check(AbilityScores::GetModifier(3) == -4, "score 3 -> -4");
    Check(AbilityScores::GetModifier(7) == -2, "score 7 -> -2");
    Check(AbilityScores::GetModifier(8) == -1, "score 8 -> -1");
    Check(AbilityScores::GetModifier(9) == -1, "score 9 -> -1 (not 0)");
    Check(AbilityScores::GetModifier(10) == 0, "score 10 -> 0");
    Check(AbilityScores::GetModifier(11) == 0, "score 11 -> 0");
    Check(AbilityScores::GetModifier(12) == 1, "score 12 -> +1");
    Check(AbilityScores::GetModifier(20) == 5, "score 20 -> +5");
    Check(AbilityScores::GetModifier(30) == 10, "score 30 -> +10");
}

static void TestD20() {
    std::printf("\n-- d20 checks and attacks --\n");
    Check(CombineRollMode(true, true) == RollMode::NORMAL, "advantage and disadvantage cancel");
    Check(CombineRollMode(false, false) == RollMode::NORMAL, "neither is a normal roll");
    Check(CombineRollMode(true, false) == RollMode::ADVANTAGE, "advantage alone");
    Check(CombineRollMode(false, true) == RollMode::DISADVANTAGE, "disadvantage alone");

    const D20Check advantage = ResolveD20(8, 15, RollMode::ADVANTAGE, 3);
    Check(advantage.natural == 15 && advantage.total == 18, "advantage keeps the higher die");
    const D20Check disadvantage = ResolveD20(8, 15, RollMode::DISADVANTAGE, 0);
    Check(disadvantage.natural == 8 && disadvantage.total == 8, "disadvantage keeps the lower die");
    const D20Check normal = ResolveD20(12, 19, RollMode::NORMAL, 2);
    Check(normal.natural == 12 && normal.diceRolled == 1, "a normal roll ignores the second die");

    const D20Check nat20 = ResolveD20(20, 0, RollMode::NORMAL, 0);
    Check(!CheckSucceeds(nat20, 25), "natural 20 does not auto-succeed an ability check");
    const D20Check nat1 = ResolveD20(1, 0, RollMode::NORMAL, 10);
    Check(CheckSucceeds(nat1, 11), "natural 1 does not auto-fail an ability check");
    Check(CheckSucceeds(ResolveD20(10, 0, RollMode::NORMAL, 5), 15), "meeting the DC succeeds");
    Check(!CheckSucceeds(ResolveD20(10, 0, RollMode::NORMAL, 4), 15), "one short of the DC fails");

    Check(AttackHits(nat20, 30), "natural 20 hits any AC");
    Check(IsCriticalHit(nat20), "natural 20 is a critical hit");
    Check(!AttackHits(ResolveD20(1, 0, RollMode::NORMAL, 19), 2), "natural 1 misses");
    Check(AttackHits(ResolveD20(10, 0, RollMode::NORMAL, 5), 15), "attack meets AC");
    Check(!AttackHits(ResolveD20(10, 0, RollMode::NORMAL, 4), 15), "attack one short of AC misses");
}

static void TestSkillsAndSaves() {
    std::printf("\n-- Skills, expertise, saves, passive scores --\n");
    Check(CheckBonus(3, 2, ProficiencyRank::NONE) == 3, "untrained is the ability modifier");
    Check(CheckBonus(3, 2, ProficiencyRank::PROFICIENT) == 5, "proficiency adds the bonus once");
    Check(CheckBonus(3, 2, ProficiencyRank::EXPERTISE) == 7, "expertise adds the bonus twice");

    AbilityScores scores(8, 17, 12, 13, 11, 14); // Finn
    SkillProficiencies skills;
    skills.AddExpertise(Skill::STEALTH);
    skills.AddProficiency(Skill::PERCEPTION);
    Check(skills.HasExpertise(Skill::STEALTH) && skills.HasProficiency(Skill::STEALTH),
          "expertise grants proficiency");
    Check(skills.GetBonus(Skill::STEALTH, scores, 2) == 7, "Stealth expertise is DEX +3 + 4");
    Check(skills.GetBonus(Skill::PERCEPTION, scores, 2) == 2, "Perception is WIS +0 + 2");
    Check(skills.GetBonus(Skill::PERSUASION, scores, 2) == 2, "untrained Persuasion is CHA only");
    Check(PassiveScore(2) == 12, "passive score is 10 + modifier");
    Check(PassiveScore(2, RollMode::ADVANTAGE) == 17, "passive advantage is +5");
    Check(PassiveScore(2, RollMode::DISADVANTAGE) == 7, "passive disadvantage is -5");

    SavingThrowProficiencies rogue;
    ApplyClassSavingThrows(ClassType::ROGUE, rogue);
    Check(rogue.IsProficient(Ability::DEXTERITY) && rogue.IsProficient(Ability::INTELLIGENCE),
          "rogues save with Dexterity and Intelligence");
    Check(!rogue.IsProficient(Ability::STRENGTH) && !rogue.IsProficient(Ability::WISDOM),
          "rogues are not proficient in Strength or Wisdom saves");
    Check(rogue.GetBonus(Ability::DEXTERITY, scores, 2) == 5, "proficient DEX save is +5");
    Check(rogue.GetBonus(Ability::STRENGTH, scores, 2) == -1, "untrained STR save is -1");

    SavingThrowProficiencies wizard;
    ApplyClassSavingThrows(ClassType::WIZARD, wizard);
    Check(wizard.IsProficient(Ability::INTELLIGENCE) && wizard.IsProficient(Ability::WISDOM),
          "wizards save with Intelligence and Wisdom");
    Check(!wizard.IsProficient(Ability::CHARISMA), "wizards are not proficient in Charisma saves");

    SavingThrowProficiencies fighter;
    ApplyClassSavingThrows(ClassType::FIGHTER, fighter);
    Check(fighter.IsProficient(Ability::STRENGTH) && fighter.IsProficient(Ability::CONSTITUTION),
          "fighters save with Strength and Constitution");
    Check(!fighter.IsProficient(Ability::DEXTERITY), "fighters are not proficient in Dexterity saves");

    Skill parsed;
    Ability parsedAbility;
    Check(TryParseSkill("Perception", parsed) && parsed == Skill::PERCEPTION, "parses Perception");
    Check(TryParseSkill("sleight of hand", parsed) && parsed == Skill::SLEIGHT_OF_HAND,
          "parses Sleight of Hand");
    Check(!TryParseSkill("Thieves' Tools", parsed), "thieves' tools are not a skill");
    Check(TryParseAbility("DEX", parsedAbility) && parsedAbility == Ability::DEXTERITY, "parses DEX");
    Check(SkillAbility(Skill::STEALTH) == Ability::DEXTERITY, "Stealth uses Dexterity");
    Check(SkillAbility(Skill::PERCEPTION) == Ability::WISDOM, "Perception uses Wisdom");
}

static void TestDamageAndArmor() {
    std::printf("\n-- Damage, crits, affinity, armor, sneak attack --\n");
    Check(ComposeDamage(4, 0, 0, 0, 3, false) == 7, "normal damage is dice plus modifier");
    Check(ComposeDamage(4, 6, 0, 0, 3, true) == 13, "a crit adds dice, not the modifier");
    Check(ComposeDamage(3, 5, 4, 6, 2, true) == 20, "sneak attack dice are doubled on a crit");
    Check(ComposeDamage(3, 5, 4, 6, 2, false) == 9, "without a crit the extra dice are rolled once");
    Check(ComposeDamage(1, 0, 0, 0, -3, false) == 0, "damage does not go negative");

    Check(ApplyAffinity(5, DamageAffinity::RESISTANT) == 2, "resistance halves, rounding down");
    Check(ApplyAffinity(1, DamageAffinity::RESISTANT) == 0, "resistance can reduce 1 to 0");
    Check(ApplyAffinity(5, DamageAffinity::VULNERABLE) == 10, "vulnerability doubles");
    Check(ApplyAffinity(5, DamageAffinity::IMMUNE) == 0, "immunity negates damage");
    Check(ApplyAffinity(5, CombineAffinity(true, true, false)) == 5,
          "resistance and vulnerability cancel");
    Check(CombineAffinity(true, false, true) == DamageAffinity::IMMUNE, "immunity wins");
    Check(DamageAfterSave(5, true, true) == 2, "a successful save halves when the spell says so");
    Check(DamageAfterSave(5, true, false) == 0, "a successful save negates damage otherwise");
    Check(DamageAfterSave(1, true, true) == 0, "half of 1 rounds down to 0");
    Check(DamageAfterSave(5, false, true) == 5, "a failed save takes full damage");

    Check(SneakAttackDice(0) == 0, "no sneak attack below 1st level");
    Check(SneakAttackDice(1) == 1 && SneakAttackDice(2) == 1, "1st and 2nd level are 1d6");
    Check(SneakAttackDice(3) == 2 && SneakAttackDice(5) == 3, "dice step up on odd levels");
    Check(SneakAttackDice(19) == 10 && SneakAttackDice(20) == 10, "sneak attack caps at 10d6");

    Check(WeaponAbilityModifier(1, 4, true, false) == 4, "finesse uses the higher modifier");
    Check(WeaponAbilityModifier(4, 1, true, false) == 4, "finesse can still use Strength");
    Check(WeaponAbilityModifier(3, 1, false, false) == 3, "ordinary melee uses Strength");
    Check(WeaponAbilityModifier(3, 1, false, true) == 1, "ranged attacks use Dexterity");

    Check(ComputeArmorClass(3, nullptr, false) == 13, "unarmored is 10 + Dexterity");
    ArmorData leather(ArmorType::LIGHT, 11, 99, false);
    Check(ComputeArmorClass(3, &leather, false) == 14, "leather is 11 + Dexterity");
    Check(ComputeArmorClass(3, &leather, true) == 16, "a shield adds 2");
    ArmorData chain(ArmorType::HEAVY, 16, 0, true);
    Check(ComputeArmorClass(3, &chain, false) == 16, "heavy armor ignores positive Dexterity");
    Check(ComputeArmorClass(-1, &chain, false) == 16, "heavy armor ignores negative Dexterity");
    ArmorData medium(ArmorType::MEDIUM, 14, 2, false);
    Check(ComputeArmorClass(4, &medium, false) == 16, "medium armor caps Dexterity at +2");
    Check(ComputeArmorClass(-1, &medium, false) == 13, "medium armor still adds a penalty");

    DiceRoller::Seed(42);
    const i32 first = DiceRoller::Roll(1, 6, 0);
    const i32 second = DiceRoller::Roll(1, 6, 0);
    DiceRoller::Seed(42);
    const DamageRoll crit(DiceRoll(1, 6, 4), DamageType::SLASHING, true);
    Check(crit.RollDamage() == first + second + 4, "DamageRoll crit doubles the die, not +4");
}

static void TestMonsters() {
    std::printf("\n-- Goblin, skeleton, bugbear --\n");
    const MonsterProfile goblin = GetMonsterProfile(MonsterKind::GOBLIN);
    Check(goblin.hitPoints == 7 && goblin.armorClass == 15, "goblin is 7 HP, AC 15");
    Check(goblin.experience == 50, "goblin is worth 50 XP");
    Check(MonsterAttackBonus(goblin) == 4, "goblin scimitar is +4 to hit");
    Check(MonsterDamageModifier(goblin) == 2, "goblin scimitar damage is +2");
    Check(goblin.melee.numDice == 1 && goblin.melee.dieSize == 6, "goblin scimitar is 1d6");
    Check(goblin.melee.damageType == DamageType::SLASHING, "goblin scimitar is slashing");
    Check(MonsterDamageAffinity(goblin, DamageType::PIERCING) == DamageAffinity::NORMAL,
          "goblins have no damage affinity");

    SkillProficiencies goblinSkills;
    ApplyMonsterSkills(goblin, goblinSkills);
    Check(goblinSkills.GetBonus(Skill::STEALTH, goblin.abilities, goblin.proficiencyBonus) == 6,
          "goblin Stealth is +6");
    Check(goblinSkills.GetBonus(Skill::PERCEPTION, goblin.abilities, goblin.proficiencyBonus) == -1,
          "goblin Perception is -1");
    Check(PassiveScore(goblin.perceptionBonus) == 9, "goblin passive Perception is 9");

    const MonsterProfile skeleton = GetMonsterProfile(MonsterKind::SKELETON);
    Check(skeleton.hitPoints == 13 && skeleton.armorClass == 13, "skeleton is 13 HP, AC 13");
    Check(skeleton.experience == 50, "skeleton is worth 50 XP");
    Check(MonsterAttackBonus(skeleton) == 4, "skeleton shortsword is +4");
    Check(MonsterDamageAffinity(skeleton, DamageType::BLUDGEONING) == DamageAffinity::VULNERABLE,
          "skeletons are vulnerable to bludgeoning");
    Check(MonsterDamageAffinity(skeleton, DamageType::POISON) == DamageAffinity::IMMUNE,
          "skeletons are immune to poison");
    Check(MonsterDamageAffinity(skeleton, DamageType::PIERCING) == DamageAffinity::NORMAL,
          "skeletons are not resistant to piercing");

    const MonsterProfile bugbear = GetMonsterProfile(MonsterKind::BUGBEAR);
    Check(bugbear.hitPoints == 27 && bugbear.armorClass == 16, "bugbear is 27 HP, AC 16");
    Check(bugbear.experience == 200, "bugbear is worth 200 XP");
    Check(MonsterAttackBonus(bugbear) == 4 && MonsterDamageModifier(bugbear) == 2,
          "bugbear morningstar is +4, damage +2");
    Check(bugbear.melee.numDice == 2 && bugbear.melee.dieSize == 8, "bugbear morningstar is 2d8");

    ItemDatabase::Initialize();
    const ItemData* scimitar = ItemDatabase::GetItem("Scimitar");
    Check(scimitar != nullptr && scimitar->weapon.properties.isFinesse
              && scimitar->weapon.damage.type == DamageType::SLASHING
              && scimitar->weapon.damage.dice.numSides == 6,
          "scimitar item is finesse, 1d6 slashing");
}

static void TestSeededStrike() {
    std::printf("\n-- Seeded strike (crit doubles sneak dice) --\n");
    u32 seed = 1;
    for (; seed < 5000; ++seed) {
        DiceRoller::Seed(seed);
        if (DiceRoller::Roll(1, 20) == 20) break;
    }
    DiceRoller::Seed(seed);
    const i32 d20 = DiceRoller::Roll(1, 20);
    const i32 weapon = DiceRoller::Roll(1, 6, 0);
    const i32 extra = DiceRoller::Roll(1, 6, 0);
    const i32 weaponCrit = DiceRoller::Roll(1, 6, 0);
    const i32 extraCrit = DiceRoller::Roll(1, 6, 0);

    DiceRoller::Seed(seed);
    StrikeSpec spec;
    spec.attackBonus = 5;
    spec.targetArmorClass = 25;
    spec.weaponDiceCount = 1;
    spec.weaponDieSize = 6;
    spec.extraDiceCount = 1;
    spec.extraDieSize = 6;
    spec.flatModifier = 3;
    const StrikeResult result = RollStrike(spec);
    const i32 expected = weapon + extra + weaponCrit + extraCrit + 3;
    Check(d20 == 20 && result.hit && result.critical, "seeded natural 20 hits and crits");
    Check(result.damage == expected, "crit damage is both dice pools plus one modifier");
    Check(result.damage != expected + 3, "the modifier was not added a second time");
}

static Entity MakeCaster(EntityManager& em, ComponentRegistry& cr, b8 withInfo) {
    Entity caster = em.Create();
    SpellSlots slots;
    slots.maxSlots[1] = 2;
    slots.slots[1] = 2;
    cr.AddComponent<SpellSlots>(caster, slots);
    Spellbook book;
    SpellData bolt;
    bolt.name = "Test Bolt";
    bolt.level = 1;
    bolt.requiresAttackRoll = true;
    bolt.damage.dice.numDice = 0;
    bolt.damage.dice.numSides = 0;
    bolt.damage.dice.modifier = 5;
    bolt.damage.type = DamageType::FIRE;
    book.spells.push_back(bolt);
    cr.AddComponent<Spellbook>(caster, book);
    if (withInfo) {
        // INT 16 (+3), wizard, level 1 proficiency +2 -> spell attack +5, DC 13.
        CharacterInfo info("Tester", RaceType::HUMAN, ClassType::WIZARD,
                           AbilityScores(10, 10, 10, 16, 10, 10));
        ApplyClassSavingThrows(ClassType::WIZARD, info.saves);
        cr.AddComponent<CharacterInfo>(caster, info);
    }
    return caster;
}

static void TestSpellcasting() {
    std::printf("\n-- Spell attack bonus, saves, and slots --\n");
    Check(SpellAttackBonus(3, 2) == 5, "spell attack is ability + proficiency");
    Check(SpellSaveDC(3, 2) == 13, "spell save DC is 8 + ability + proficiency");
    CharacterInfo wizard("Tester", RaceType::HUMAN, ClassType::WIZARD,
                         AbilityScores(10, 10, 10, 16, 12, 8));
    Check(SpellcastingAbilityModifier(wizard) == 3, "a wizard casts with Intelligence");
    CharacterInfo rogue("Finn", RaceType::HALFLING, ClassType::ROGUE,
                        AbilityScores(8, 17, 12, 13, 11, 14));
    Check(SpellcastingAbilityModifier(rogue) == 0, "a rogue has no spellcasting ability");

    // Find a d20 in the window where +5 hits AC 15 and +0 misses (10 through 14).
    u32 seed = 1;
    i32 face = 0;
    for (; seed < 10000; ++seed) {
        DiceRoller::Seed(seed);
        face = DiceRoller::Roll(1, 20);
        if (face >= 10 && face <= 14) break;
    }

    EntityManager em;
    ComponentRegistry cr;
    Entity trained = MakeCaster(em, cr, true);
    Entity untrained = MakeCaster(em, cr, false);
    Entity target = em.Create();
    HitPoints hp;
    hp.maximum = 30;
    hp.current = 30;
    cr.AddComponent<HitPoints>(target, hp);
    ArmorClass ac;
    ac.base = 15;
    cr.AddComponent<ArmorClass>(target, ac);

    SystemManager systems;
    systems.RegisterSystem(std::make_unique<SpellcastingSystem>(), em, cr);
    SpellcastingSystem* spells = systems.GetSystem<SpellcastingSystem>("SpellcastingSystem");
    Check(spells != nullptr, "SpellcastingSystem is registered");
    if (!spells) return;

    DiceRoller::Seed(seed);
    spells->RequestCast(trained, 0, target);
    systems.RunAll(0.016f, em, cr);
    HitPoints* targetHp = cr.GetComponent<HitPoints>(target);
    SpellSlots* trainedSlots = cr.GetComponent<SpellSlots>(trained);
    Check(targetHp->current == 25, "a +5 spell attack hits AC 15 for the flat 5 damage");
    Check(trainedSlots->slots[1] == 1, "a leveled spell consumes one slot");

    targetHp->current = 30;
    DiceRoller::Seed(seed);
    spells->RequestCast(untrained, 0, target);
    systems.RunAll(0.016f, em, cr);
    Check(targetHp->current == 30, "without a spellcasting bonus the same d20 misses");
    Check(cr.GetComponent<SpellSlots>(untrained)->slots[1] == 1, "a miss still spends the slot");

    // Cantrip: level 0 does not spend the remaining 1st-level slot.
    Spellbook* book = cr.GetComponent<Spellbook>(trained);
    SpellData cantrip;
    cantrip.name = "Test Cantrip";
    cantrip.level = 0;
    cantrip.requiresAttackRoll = false;
    book->spells.push_back(cantrip);
    spells->RequestCast(trained, 1, target);
    systems.RunAll(0.016f, em, cr);
    Check(trainedSlots->slots[1] == 1, "a cantrip does not consume a spell slot");

    // Saving throw: flat 5 damage, half on a success, DC 13, save bonus 0.
    SpellData blast;
    blast.name = "Test Blast";
    blast.level = 1;
    blast.requiresSavingThrow = true;
    blast.halfOnSave = true;
    blast.saveAbility = Ability::DEXTERITY;
    blast.damage.dice.modifier = 5;
    book->spells.push_back(blast);

    u32 saveSeed = 1;
    i32 saveFace = 0;
    for (; saveSeed < 10000; ++saveSeed) {
        DiceRoller::Seed(saveSeed);
        saveFace = DiceRoller::Roll(1, 20);
        if (saveFace >= 13) break;
    }
    targetHp->current = 30;
    DiceRoller::Seed(saveSeed);
    spells->RequestCast(trained, 2, target);
    systems.RunAll(0.016f, em, cr);
    Check(saveFace + 0 >= 13 && targetHp->current == 28, "a successful save takes half of 5, rounded down");
    Check(trainedSlots->slots[1] == 0, "the save spell spent the last slot");

    // No slot left: the cast fails and HP does not change.
    const i32 hpBefore = targetHp->current;
    spells->RequestCast(trained, 2, target);
    systems.RunAll(0.016f, em, cr);
    Check(targetHp->current == hpBefore, "a cast with no slot deals no damage");
}

int main() {
    Logger::Initialize(nullptr);
    DiceRoller::Initialize();

    TestModifiers();
    TestD20();
    TestSkillsAndSaves();
    TestDamageAndArmor();
    TestMonsters();
    TestSeededStrike();
    TestSpellcasting();

    std::printf("\n==== %s (%d failure%s) ====\n",
                g_failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                g_failures, g_failures == 1 ? "" : "s");
    Logger::Shutdown();
    return g_failures == 0 ? 0 : 1;
}
