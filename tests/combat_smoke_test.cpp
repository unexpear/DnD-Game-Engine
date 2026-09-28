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
 *   4. Initiative::value stays the modifier across repeated rolls, and the
 *      d20 total lands in Initiative::total.
 *   5. RollInitiative(participants) ignores bystanders.
 *   6. RemoveEntity keeps currentTurn on the actor who should go next.
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
#include "ecs/systems/script_system.h"
#include "game/dice.h"
#include "game/combat.h"
#include "game/combat_stats.h"
#include "core/log.h"

#include <cstdio>
#include <memory>
#include <vector>

namespace Dnd {
// script_system.cpp is Win32-only (windows.h). CombatSystem notifies it on
// damage; this headless stub keeps the documented g++ line linkable.
void ScriptSystem::NotifyEvent(ComponentRegistry&, Entity, const std::string&) {}
}

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
    Initiative* heroInit = cr.GetComponent<Initiative>(hero);
    const i32 heroMod = heroInit->value;
    combat->GetState().RollInitiative(em, cr);
    Check(combat->GetState().isActive, "combat is active after RollInitiative()");
    Check(combat->GetState().initiativeOrder.size() == 3, "all 3 combatants in initiative order");
    Check(heroInit->value == heroMod, "initiative modifier is preserved");
    Check(heroInit->total >= heroMod + 1 && heroInit->total <= heroMod + 20,
          "initiative total is one d20 plus the modifier");
    combat->GetState().RollInitiative(em, cr);
    Check(heroInit->value == heroMod, "a second roll still uses the original modifier");
    Check(heroInit->total >= heroMod + 1 && heroInit->total <= heroMod + 20,
          "a second roll does not add the previous total");
    Check(combat->GetState().initiativeOrder.size() == 3, "second roll still seats all 3");

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

    std::printf("\n-- 4. Removing the current actor does not rewind the turn --\n");
    {
        CombatState& st = combat->GetState();
        st.isActive = true;
        st.initiativeOrder = { hero, tank, goblin };
        st.currentTurn = 1; // tank is acting
        Check(st.RemoveEntity(tank), "current actor removed");
        Check(st.GetCurrentEntity() == goblin, "turn passes to whoever followed the removed actor");
        Check(st.initiativeOrder.size() == 2, "order is hero, goblin");

        st.initiativeOrder = { hero, tank, goblin };
        st.currentTurn = 1; // tank
        Check(st.RemoveEntity(hero), "actor earlier in the order removed");
        Check(st.GetCurrentEntity() == tank, "current actor stays put when someone earlier falls");

        st.initiativeOrder = { hero, tank, goblin };
        st.currentTurn = 2; // goblin, last
        Check(st.RemoveEntity(goblin), "last actor removed");
        Check(st.GetCurrentEntity() == hero, "removing the last actor wraps to the top of the round");
        Check(st.isActive, "combat stays active while someone remains");
    }

    std::printf("\n-- 5. Explicit participants exclude bystanders and the dead --\n");
    {
        Entity bystander = makeCombatant(10, 12, 4);
        std::vector<Entity> duel;
        duel.push_back(hero);
        duel.push_back(tank);
        duel.push_back(goblin); // dead: must not rejoin
        combat->GetState().RollInitiative(em, cr, duel);
        Check(combat->GetState().isActive, "duel roll activates combat");
        Check(combat->GetState().initiativeOrder.size() == 2, "dead combatant is left out of the duel");
        Check(InInitiative(combat->GetState(), hero), "hero is in the duel");
        Check(InInitiative(combat->GetState(), tank), "tank is in the duel");
        Check(!InInitiative(combat->GetState(), goblin), "dead goblin stays out");
        Check(!InInitiative(combat->GetState(), bystander), "bystander with Initiative is not pulled in");
        Initiative* bInit = cr.GetComponent<Initiative>(bystander);
        Check(bInit->total == 0, "bystander was not rolled");
        combat->GetState().EndCombat();
        Check(!combat->GetState().isActive, "EndCombat clears the encounter");
        Check(combat->GetState().initiativeOrder.empty(), "EndCombat drops the turn order");
    }

    std::printf("\n==== %s (%d failure%s) ====\n",
                g_failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                g_failures, g_failures == 1 ? "" : "s");

    Logger::Shutdown();
    return g_failures == 0 ? 0 : 1;
}
