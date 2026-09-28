/**
 * @file spell.cpp
 * @brief Implementation of spellcasting: slot consumption, spell lookup, and effect application.
 *
 * See spell.h for full documentation.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.2.0
 */

#include "game/spell.h"
#include "game/combat_stats.h"
#include "game/rules.h"
#include "core/log.h"

namespace Dnd {

// ==========================================
// SpellcastingSystem::OnUpdate
// ==========================================

void SpellcastingSystem::OnUpdate(f32 /*deltaTime*/, EntityManager& em, ComponentRegistry& cr) {
    if (m_pendingCasts.empty()) return;

    for (const auto& request : m_pendingCasts) {
        ProcessCast(request, em, cr);
    }

    m_pendingCasts.clear();
}

// ==========================================
// SpellcastingSystem::ProcessCast
// ==========================================

b8 SpellcastingSystem::ProcessCast(const SpellCastRequest& request, EntityManager& em, ComponentRegistry& cr) {
    DND_UNUSED(em);

    u32 casterIdx = Entity::GetIndex(request.caster.id);

    // Retrieve components
    auto* slots = cr.GetComponent<SpellSlots>(request.caster);
    auto* spellbook = cr.GetComponent<Spellbook>(request.caster);

    if (!slots) {
        DND_LOG_WARN("[Spellcast] Entity index %u has no SpellSlots component.", casterIdx);
        return false;
    }
    if (!spellbook) {
        DND_LOG_WARN("[Spellcast] Entity index %u has no Spellbook component.", casterIdx);
        return false;
    }

    // Validate spell index
    if (request.spellIndex < 0 ||
        request.spellIndex >= static_cast<i32>(spellbook->spells.size())) {
        DND_LOG_WARN("[Spellcast] Entity index %u has invalid spell index %d.",
                     casterIdx, request.spellIndex);
        return false;
    }

    const SpellData& spell = spellbook->spells[request.spellIndex];

    DND_LOG_INFO("[Spellcast] %s casting '%s' (level %d).", "Entity", spell.name, spell.level);

    // Check slot availability
    if (!slots->HasSlot(spell.level)) {
        DND_LOG_WARN("[Spellcast] %s has no available level %d spell slot for '%s'.",
                     "Entity", spell.level, spell.name);
        return false;
    }

    // Log component requirements
    if (spell.requiresVerbal) {
        DND_LOG_DEBUG("[Spellcast] Verbal component: incantation spoken.");
    }
    if (spell.requiresSomatic) {
        DND_LOG_DEBUG("[Spellcast] Somatic component: gestures performed.");
    }
    if (spell.requiresMaterial) {
        DND_LOG_DEBUG("[Spellcast] Material component: consumed/present.");
    }

    // Spell attack bonus and save DC come from the caster's class, when known.
    const CharacterInfo* casterInfo = cr.GetComponent<CharacterInfo>(request.caster);
    const i32 spellMod = casterInfo ? SpellcastingAbilityModifier(*casterInfo) : 0;
    const i32 proficiency = casterInfo ? casterInfo->proficiencyBonus : 0;

    // Apply effects: handle damaging spells.
    // Roll order for an attack spell: one d20, then damage dice (crit dice if the d20 is 20).
    // Roll order for a save spell: one d20, then damage dice. A successful save deals
    // half (round down) when the spell says so, otherwise none.
    if (spell.requiresAttackRoll) {
        Entity target = request.target;
        u32 targetIdx = Entity::GetIndex(target.id);

        i32 targetAC = 10;
        auto* targetACComp = cr.GetComponent<ArmorClass>(target);
        if (targetACComp) {
            targetAC = targetACComp->GetTotal();
        }

        const i32 attackBonus = SpellAttackBonus(spellMod, proficiency);
        const D20Check attack = RollD20Check(RollMode::NORMAL, attackBonus);
        if (AttackHits(attack, targetAC)) {
            DamageRoll rolled = spell.damage;
            rolled.isCritical = IsCriticalHit(attack);
            const i32 damage = rolled.RollDamage();

            auto* targetHP = cr.GetComponent<HitPoints>(target);
            if (targetHP) {
                targetHP->TakeDamage(damage);
                DND_LOG_INFO("[Spellcast] '%s' hits entity index %u for %d damage (d20 %d %+d vs AC %d, HP: %d/%d).",
                             spell.name, targetIdx, damage, attack.natural, attack.modifier,
                             targetAC, targetHP->current, targetHP->maximum);

                if (!targetHP->IsAlive()) {
                    DND_LOG_INFO("[Spellcast] Entity index %u slain by '%s'!",
                                 targetIdx, spell.name);
                }
            }
        } else {
            DND_LOG_INFO("[Spellcast] '%s' missed entity index %u (d20 %d %+d vs AC %d).",
                         spell.name, targetIdx, attack.natural, attack.modifier, targetAC);
        }
    } else if (spell.requiresSavingThrow) {
        Entity target = request.target;
        u32 targetIdx = Entity::GetIndex(target.id);
        const i32 saveDC = SpellSaveDC(spellMod, proficiency);

        i32 saveBonus = 0;
        const CharacterInfo* targetInfo = cr.GetComponent<CharacterInfo>(target);
        if (targetInfo) {
            saveBonus = targetInfo->saves.GetBonus(spell.saveAbility, targetInfo->abilities,
                                                   targetInfo->proficiencyBonus);
        }
        const D20Check save = RollD20Check(RollMode::NORMAL, saveBonus);
        const b8 saved = CheckSucceeds(save, saveDC);

        i32 damage = 0;
        if (spell.damage.dice.numDice > 0 || spell.damage.dice.modifier != 0) {
            damage = spell.damage.RollDamage();
            damage = DamageAfterSave(damage, saved, spell.halfOnSave);
        }

        auto* targetHP = cr.GetComponent<HitPoints>(target);
        if (targetHP && damage > 0) {
            targetHP->TakeDamage(damage);
        }
        DND_LOG_INFO("[Spellcast] '%s' save d20 %d %+d = %d vs DC %d (%s), damage %d.",
                     spell.name, save.natural, save.modifier, save.total, saveDC,
                     saved ? "success" : "failure", damage);
        DND_UNUSED(targetIdx);
    } else if (spell.damage.dice.numDice > 0 || spell.damage.dice.modifier != 0) {
        // Auto-hit damaging spells (magic missile, etc.)
        Entity target = request.target;
        u32 targetIdx = Entity::GetIndex(target.id);
        i32 damage = spell.damage.RollDamage();

        auto* targetHP = cr.GetComponent<HitPoints>(target);
        if (targetHP) {
            targetHP->TakeDamage(damage);
            DND_LOG_INFO("[Spellcast] '%s' hits entity index %u for %d damage (no save).",
                         spell.name, targetIdx, damage);
        }
    } else {
        // Non-damaging spell (healing, buff, utility)
        DND_LOG_INFO("[Spellcast] '%s' cast successfully (non-damaging).", spell.name);
    }

    // Consume the spell slot (cantrips are not consumed)
    slots->UseSlot(spell.level);

    DND_LOG_INFO("[Spellcast] '%s' cast complete.", spell.name);
    return true;
}

} // namespace Dnd
