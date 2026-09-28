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

    // Apply effects: handle damaging spells
    if (spell.requiresAttackRoll) {
        Entity target = request.target;
        u32 targetIdx = Entity::GetIndex(target.id);

        // Get target's AC
        i32 targetAC = 10;
        auto* targetACComp = cr.GetComponent<ArmorClass>(target);
        if (targetACComp) {
            targetAC = targetACComp->GetTotal();
        }

        // Build a temporary attack roll (attack bonus from caster's stats would come from
        // a separate component; here we use the spell's implicit bonus or a default of 0).
        // In a full implementation, the caster's proficiency + spellcasting ability modifier
        // would be pulled from an AbilityScores component.
        AttackRoll spellAttack(0, false, false);
        if (spellAttack.IsHit(targetAC)) {
            i32 damage = spell.damage.RollDamage();

            auto* targetHP = cr.GetComponent<HitPoints>(target);
            if (targetHP) {
                targetHP->TakeDamage(damage);
                DND_LOG_INFO("[Spellcast] '%s' hits entity index %u for %d damage (HP: %d/%d).",
                             spell.name, targetIdx, damage, targetHP->current, targetHP->maximum);

                if (!targetHP->IsAlive()) {
                    DND_LOG_INFO("[Spellcast] Entity index %u slain by '%s'!",
                                 targetIdx, spell.name);
                }
            }
        } else {
            DND_LOG_INFO("[Spellcast] '%s' missed entity index %u (AC %d).",
                         spell.name, targetIdx, targetAC);
        }
    } else if (spell.requiresSavingThrow) {
        // Saving throw spells apply half or no damage based on the target's save.
        // For this implementation, we assume the target fails and apply full damage.
        Entity target = request.target;
        u32 targetIdx = Entity::GetIndex(target.id);

        if (spell.damage.dice.numDice > 0) {
            i32 damage = spell.damage.RollDamage();

            auto* targetHP = cr.GetComponent<HitPoints>(target);
            if (targetHP) {
                targetHP->TakeDamage(damage);
                DND_LOG_INFO("[Spellcast] '%s' deals %d damage to entity index %u (save failed).",
                             spell.name, damage, targetIdx);
            }
        }
    } else if (spell.damage.dice.numDice > 0) {
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
