/**
 * @file combat.cpp
 * @brief Implementation of combat systems: initiative rolling, action processing, damage application.
 *
 * See combat.h for full documentation.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.2.0
 */

#include "game/combat.h"
#include "game/combat_stats.h"
#include "core/log.h"
#include "ecs/systems/script_system.h"
#include <algorithm>
#include <utility>

namespace Dnd {

namespace {

/// @brief d20 (+ advantage) + modifier. Leaves Initiative::value unchanged.
i32 RollOneInitiative(Initiative& init) {
    i32 d20;
    if (init.hasAdvantage) {
        const i32 r1 = DiceRoller::Roll(1, 20);
        const i32 r2 = DiceRoller::Roll(1, 20);
        d20 = (r1 > r2) ? r1 : r2;
    } else {
        d20 = DiceRoller::Roll(1, 20);
    }
    init.total = d20 + init.value;
    return init.total;
}

} // namespace

// ==========================================
// CombatState::RollInitiative
// ==========================================

void CombatState::RollInitiative(EntityManager& em, ComponentRegistry& cr) {
    std::vector<Entity> participants;
    auto* initiativeSet = cr.GetSparseSet<Initiative>();
    if (initiativeSet) {
        participants.reserve(initiativeSet->Size());
        for (u32 i = 0; i < initiativeSet->Size(); ++i) {
            // Generation 0 matches freshly created slots; component lookup is by index.
            const u32 entityIdx = initiativeSet->GetEntityIndex(i);
            participants.emplace_back(Entity::EncodeId(entityIdx, 0));
        }
    }
    RollInitiative(em, cr, participants);
}

void CombatState::RollInitiative(EntityManager& em, ComponentRegistry& cr,
                                 const std::vector<Entity>& participants) {
    DND_UNUSED(em);
    initiativeOrder.clear();
    currentTurn = 0;
    isActive = false;

    std::vector<std::pair<i32, Entity>> pairs;
    pairs.reserve(participants.size());

    for (Entity entity : participants) {
        Initiative* init = cr.GetComponent<Initiative>(entity);
        if (!init) continue;
        const HitPoints* hp = cr.GetComponent<HitPoints>(entity);
        if (hp && !hp->IsAlive()) continue;
        const i32 total = RollOneInitiative(*init);
        pairs.emplace_back(total, entity);
    }

    if (pairs.empty()) {
        DND_LOG_WARN("CombatState::RollInitiative: No living participants with Initiative.");
        return;
    }

    // Highest total first. Ties broken by entity index (lower = earlier).
    std::sort(pairs.begin(), pairs.end(),
        [](const std::pair<i32, Entity>& a, const std::pair<i32, Entity>& b) {
            if (a.first != b.first) return a.first > b.first;
            return Entity::GetIndex(a.second.id) < Entity::GetIndex(b.second.id);
        });

    for (const auto& pair : pairs) {
        initiativeOrder.push_back(pair.second);
    }
    isActive = true;

    DND_LOG_INFO("Combat started with %zu participants. First turn: entity index %u.",
                 initiativeOrder.size(), initiativeOrder[0].id & Entity::INDEX_MASK);
}

void CombatState::EndCombat() {
    initiativeOrder.clear();
    currentTurn = 0;
    isActive = false;
}

// ==========================================
// CombatState::RemoveEntity
// ==========================================

b8 CombatState::RemoveEntity(Entity entity) {
    u32 targetIndex = Entity::GetIndex(entity.id);

    for (i32 i = 0; i < static_cast<i32>(initiativeOrder.size()); i++) {
        if (Entity::GetIndex(initiativeOrder[i].id) == targetIndex) {
            initiativeOrder.erase(initiativeOrder.begin() + i);

            if (initiativeOrder.empty()) {
                isActive = false;
                currentTurn = 0;
                return true;
            }

            // Removing someone earlier in the order shifts the current actor left.
            // Removing the current actor leaves currentTurn on whoever followed them.
            // Removing the last actor wraps to the top of the round.
            if (i < currentTurn) {
                currentTurn--;
            } else if (currentTurn >= static_cast<i32>(initiativeOrder.size())) {
                currentTurn = 0;
            }

            return true;
        }
    }

    return false;
}

// ==========================================
// CombatSystem::QueueAction
// ==========================================

void CombatSystem::QueueAction(const CombatAction& action) {
    m_actions.push_back(action);
}

// ==========================================
// CombatSystem::OnUpdate
// ==========================================

void CombatSystem::OnUpdate(f32 /*deltaTime*/, EntityManager& em, ComponentRegistry& cr) {
    if (!m_state.isActive) return;

    // Process all queued actions
    for (const auto& action : m_actions) {
        u32 targetIdx = Entity::GetIndex(action.target.id);

        // Log the action
        if (action.description && action.description[0] != '\0') {
            DND_LOG_INFO("[Combat] %s", action.description);
        }

        // Apply damage: check if the attack hits and the target has HitPoints
        if (action.type == ActionType::ATTACK || action.type == ActionType::CAST_SPELL) {
            // Get the target's armor class for hit check
            i32 targetAC = 10;
            auto* ac = cr.GetComponent<ArmorClass>(action.target);
            if (ac) {
                targetAC = ac->GetTotal();
            }

            if (action.attack.IsHit(targetAC)) {
                i32 damage = action.damage.RollDamage();

                // Apply damage to target's HitPoints
                auto* hp = cr.GetComponent<HitPoints>(action.target);
                if (hp) {
                    hp->TakeDamage(damage);
                    DND_LOG_INFO("[Combat] Entity index %u took %d damage (HP: %d/%d).",
                                 targetIdx, damage, hp->current, hp->maximum);

                    // Notify scripts on the target entity
                    ScriptSystem::NotifyEvent(cr, action.target,
                        "{\"type\":\"damage\",\"amount\":" + std::to_string(damage) +
                        ",\"hp\":" + std::to_string(hp->current) + "}");

                    // Check for death
                    if (!hp->IsAlive()) {
                        DND_LOG_INFO("[Combat] Entity index %u has fallen!", targetIdx);
                        ScriptSystem::NotifyEvent(cr, action.target,
                            "{\"type\":\"death\"}");
                        m_state.RemoveEntity(action.target);
                    }
                }
            } else {
                DND_LOG_INFO("[Combat] Attack against entity index %u missed (AC %d).",
                             targetIdx, targetAC);
            }
        }

        // Check if the source entity is still alive after taking damage
        auto* sourceHP = cr.GetComponent<HitPoints>(action.source);
        if (sourceHP && !sourceHP->IsAlive()) {
            DND_LOG_INFO("[Combat] Source entity index %u has fallen!",
                         Entity::GetIndex(action.source.id));
            m_state.RemoveEntity(action.source);
        }
    }

    m_actions.clear();

    // Check HP of every entity in initiative order for death
    std::vector<Entity> toRemove;
    for (const auto& entity : m_state.initiativeOrder) {
        auto* hp = cr.GetComponent<HitPoints>(entity);
        if (hp && !hp->IsAlive()) {
            toRemove.push_back(entity);
        }
    }
    for (const auto& entity : toRemove) {
        DND_LOG_INFO("[Combat] Removing dead entity index %u from initiative.",
                     Entity::GetIndex(entity.id));
        m_state.RemoveEntity(entity);
    }

    DND_UNUSED(em);
}

} // namespace Dnd
