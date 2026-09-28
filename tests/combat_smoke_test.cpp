/**
 * @file combat_smoke_test.cpp
 * @brief Headless smoke test for the turn-based CombatSystem.
 *
 * Drives the real CombatSystem through a SystemManager (the same path the
 * engine uses via Scene::Update -> SystemManager::RunAll) with no window or
 * GL context, and asserts the RNG-independent invariants of the combat
 * pipeline:
 *   1. RollInitiative() populates the turn order and activates combat.
 *   2. Attacks apply damage to the target's HitPoints (and never go negative).
 *   3. A target reduced to 0 HP is removed from the initiative order.
 *
 * This is intentionally a self-contained smoke test (no test framework) so it
 * can be built with a single g++ invocation and run anywhere — including
 * environments with no system OpenGL.
 *
 * Build (from repo root, MSYS2 g++):
 *   g++ -std=c++17 -Wall -Wextra -Iinclude -Isrc \
 *       tests/combat_smoke_test.cpp \
 *       src/game/combat.cpp src/game/dice.cpp \
 *       src/ecs/system.cpp src/ecs/entity.cpp src/core/log.cpp \
 *       -o build/combat_smoke_test
 *
 * Exit code: 0 = all checks passed, 1 = one or more failures.
 *
 * @author DND Engine Team
 * @date 2026-06-03
 * @version 0.1.0
 */

#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/system.h"
#include "game/dice.h"
#include "game/combat.h"
#include "game/combat_stats.h"
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

/// @brief True if an entity (by index) is present in the initiative order.
static bool InInitiative(const CombatState& state, Entity e) {
    for (const Entity& x : state.initiativeOrder) {
        if (Entity::GetIndex(x.id) == Entity::GetIndex(e.id)) {
            return true;
        }
    }
    return false;
}

int main() {
    Logger::Initialize(nullptr); // console-only logging
    DiceRoller::Initialize();

    EntityManager em;
    ComponentRegistry cr;

    // Helper: create a combatant with just the components CombatSystem needs.
    auto makeCombatant = [&](i32 hp, i32 ac, i32 initiativeMod) -> Entity {
        Entity e = em.Create();
        HitPoints h; h.maximum = hp; h.current = hp;
        cr.AddComponent<HitPoints>(e, h);
        ArmorClass a; a.base = ac;
        cr.AddComponent<ArmorClass>(e, a);
        Initiative i; i.value = initiativeMod;
        cr.AddComponent<Initiative>(e, i);
        return e;
    };

    Entity hero   = makeCombatant(40, 15, 5);
    Entity tank   = makeCombatant(1000, 10, 0); // survives the damage test
    Entity goblin = makeCombatant(7, 10, 0);    // dies in the death test

    SystemManager systems;
    systems.RegisterSystem(std::make_unique<CombatSystem>(), em, cr);
    CombatSystem* combat = systems.GetSystem<CombatSystem>("CombatSystem");

    Check(combat != nullptr, "CombatSystem retrievable via GetSystem<T>()");
    if (!combat) {
        std::printf("\n==== CHECKS FAILED (cannot continue) ====\n");
        return 1;
    }

    std::printf("\n-- 1. Initiative --\n");
    combat->GetState().RollInitiative(em, cr);
    Check(combat->GetState().isActive, "combat is active after RollInitiative()");
    Check(combat->GetState().initiativeOrder.size() == 3, "all 3 combatants in initiative order");

    std::printf("\n-- 2. Damage application --\n");
    HitPoints* tankHP = cr.GetComponent<HitPoints>(tank);
    const i32 hpBefore = tankHP->current;
    // Five high-bonus attacks on a 1000-HP dummy. Only a natural 1 misses, so
    // the probability that all five miss is (1/20)^5 ~ 3e-7.
    for (int k = 0; k < 5; ++k) {
        CombatAction atk;
        atk.type = ActionType::ATTACK;
        atk.source = hero;
        atk.target = tank;
        atk.attack = AttackRoll(50);
        atk.damage = DamageRoll(DiceRoll(2, 6, 3), DamageType::SLASHING);
        atk.description = "hero tests a strike on the dummy";
        combat->QueueAction(atk);
    }
    systems.RunAll(0.016f, em, cr);
    Check(tankHP->current < hpBefore, "attacks reduced target HP");
    Check(tankHP->current >= 0, "HP never goes negative");
    Check(InInitiative(combat->GetState(), tank), "surviving target stays in initiative");

    std::printf("\n-- 3. Death removes from initiative --\n");
    HitPoints* goblinHP = cr.GetComponent<HitPoints>(goblin);
    // Overkill (20d12+50 >> 7 HP). Loop a few times only to absorb a rare nat-1 miss.
    for (int k = 0; k < 12 && goblinHP->current > 0; ++k) {
        CombatAction atk;
        atk.type = ActionType::ATTACK;
        atk.source = hero;
        atk.target = goblin;
        atk.attack = AttackRoll(50);
        atk.damage = DamageRoll(DiceRoll(20, 12, 50), DamageType::SLASHING);
        atk.description = "hero overkills the goblin";
        combat->QueueAction(atk);
        systems.RunAll(0.016f, em, cr);
    }
    Check(goblinHP->current == 0, "overkilled target is at 0 HP");
    Check(!InInitiative(combat->GetState(), goblin), "dead target removed from initiative");
    Check(combat->GetState().initiativeOrder.size() == 2, "initiative order shrank to 2");

    std::printf("\n==== %s (%d failure%s) ====\n",
                g_failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                g_failures, g_failures == 1 ? "" : "s");

    Logger::Shutdown();
    return g_failures == 0 ? 0 : 1;
}
