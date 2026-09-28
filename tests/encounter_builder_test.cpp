/**
 * @file encounter_builder_test.cpp
 * @brief Headless tests for the DM encounter roster and initiative order.
 *
 * Covers the rules the encounter panel drives, without a window or GL context:
 *   - catalog add / naming
 *   - damage, heal, and the full-HP override
 *   - initiative sort, ties, and skipping creatures at 0 HP
 *   - start / next / end, including dropping the active creature
 *
 * Build (from repo root):
 *   g++ -std=c++17 -Wall -Wextra -Iinclude \
 *       tests/encounter_builder_test.cpp \
 *       src/tools/encounter_builder.cpp \
 *       -o build/encounter_builder_test
 *
 * Exit code: 0 = all checks passed, 1 = one or more failures.
 *
 * @author DND Engine Team
 * @date 2026-09-28
 * @version 0.1.0
 */

#include "tools/encounter_builder.h"

#include <cstdio>
#include <vector>

using namespace Dnd;

static int g_failures = 0;

/**
 * @brief Records one pass/fail line.
 * @param condition Check result.
 * @param description What was checked.
 */
static void Check(bool condition, const char* description) {
    std::printf("  [%s] %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) ++g_failures;
}

/**
 * @brief A d20 that walks a fixed list and then repeats the last face.
 */
class ScriptDie {
public:
    explicit ScriptDie(std::vector<i32> faces) : m_faces(std::move(faces)) {}

    i32 Roll() {
        if (m_faces.empty()) return 1;
        if (m_index >= m_faces.size()) return m_faces.back();
        return m_faces[m_index++];
    }

private:
    std::vector<i32> m_faces;
    size_t m_index = 0;
};

int main() {
    std::printf("\n-- Catalog --\n");
    Check(EncounterBuilder::Catalog().size() == 3, "catalog has goblin, skeleton, bugbear");
    Check(EncounterBuilder::FindStat("goblin") != nullptr, "goblin stat block exists");
    Check(EncounterBuilder::FindStat("goblin")->maxHp == 9, "goblin HP matches the demo floor baseline");
    Check(EncounterBuilder::FindStat("skeleton")->armorClass == 13, "skeleton AC matches the demo");
    Check(EncounterBuilder::FindStat("nope") == nullptr, "unknown stat id is rejected");

    EncounterBuilder encounter;
    Check(encounter.IncludeParty(), "party is included in initiative by default");
    Check(encounter.Add("nope") == 0, "adding an unknown monster fails");
    Check(encounter.Members().empty(), "failed add does not touch the roster");

    const u32 goblin = encounter.Add("goblin");
    const u32 goblin2 = encounter.Add("goblin");
    const u32 skeleton = encounter.Add("skeleton");
    Check(goblin != 0 && goblin2 != 0 && skeleton != 0, "three monsters added");
    Check(encounter.Find(goblin)->name == "Goblin", "first goblin keeps the catalog name");
    Check(encounter.Find(goblin2)->name == "Goblin 2", "second goblin is disambiguated");
    Check(encounter.Find(goblin)->currentHp == 9, "new monster starts at full HP");
    Check(encounter.Find(skeleton)->dexMod == 1, "skeleton keeps its DEX modifier");

    std::printf("\n-- Hit points --\n");
    Check(encounter.ApplyDamage(goblin, 4), "damage applies");
    Check(encounter.Find(goblin)->currentHp == 5, "9 - 4 = 5");
    Check(!encounter.ApplyDamage(goblin, 0), "zero damage is a no-op");
    Check(!encounter.ApplyDamage(goblin, -3), "negative damage is a no-op");
    Check(encounter.Heal(goblin, 2), "heal applies");
    Check(encounter.Find(goblin)->currentHp == 7, "5 + 2 = 7");
    Check(encounter.ApplyDamage(goblin, 100), "overkill damage lands");
    Check(encounter.Find(goblin)->currentHp == 0, "HP does not go below 0");
    Check(encounter.Heal(goblin, 5), "heal from 0 restores HP via HitPoints::Heal");
    Check(encounter.Find(goblin)->currentHp == 5, "0 + 5 = 5");
    Check(encounter.SetCurrentHp(goblin, 9), "DM can set current HP directly");
    Check(encounter.Find(goblin)->currentHp == 9, "revived goblin is at catalog max");
    Check(encounter.SetCurrentHp(goblin, 50) && encounter.Find(goblin)->currentHp == 9,
          "current HP clamps to max");
    Check(encounter.SetMaxHp(skeleton, 4), "max HP can be lowered");
    Check(encounter.Find(skeleton)->currentHp == 4, "current HP clamps down with max");
    Check(!encounter.SetMaxHp(skeleton, 0), "max HP below 1 is rejected");

    std::printf("\n-- Initiative --\n");
    encounter.SetMaxHp(skeleton, 14);
    encounter.SetCurrentHp(skeleton, 14);
    encounter.SetCurrentHp(goblin, 9);

    ScriptDie dice({20, 10, 15});
    // Roll order follows living members: goblin (dex 0), goblin 2 (dex 0), skeleton (dex +1).
    // 20+0=20, 10+0=10, 15+1=16. Order: Goblin, Skeleton, Goblin 2.
    Check(encounter.RollInitiative([&]() { return dice.Roll(); }, {}), "roll succeeds");
    Check(encounter.Order().size() == 3, "three living combatants rolled");
    Check(encounter.Order()[0].id == goblin && encounter.Order()[0].total == 20, "highest total acts first");
    Check(encounter.Order()[1].id == skeleton && encounter.Order()[1].total == 16, "DEX modifier is added");
    Check(encounter.Order()[2].id == goblin2 && encounter.Order()[2].total == 10, "lowest total is last");
    Check(encounter.Find(goblin)->d20 == 20, "raw d20 is stored on the member");
    Check(!encounter.IsCombatActive(), "rolling does not start combat");

    Check(encounter.StartCombat() && encounter.IsCombatActive(), "start after a roll succeeds");
    Check(encounter.CurrentTurn() && encounter.CurrentTurn()->id == goblin, "combat opens on the top of the order");
    Check(encounter.NextTurn() && encounter.CurrentTurn()->id == skeleton, "next turn advances");
    Check(encounter.NextTurn() && encounter.CurrentTurn()->id == goblin2, "next turn advances again");
    Check(encounter.NextTurn() && encounter.CurrentTurn()->id == goblin, "next turn wraps");
    Check(encounter.ApplyDamage(goblin, 100), "down the creature whose turn it is");
    Check(encounter.IsCombatActive(), "combat continues while someone is standing");
    Check(encounter.CurrentTurn() && encounter.CurrentTurn()->id == skeleton,
          "turn skips the creature at 0 HP");
    Check(encounter.SetCurrentHp(goblin, 9), "revive the skipped goblin in place");
    Check(encounter.CurrentTurn()->id == skeleton, "reviving someone does not steal the turn");

    encounter.EndCombat();
    Check(!encounter.IsCombatActive(), "end combat clears the active flag");
    Check(encounter.Order().size() == 3, "end combat keeps the order");
    Check(encounter.CurrentTurn() == nullptr, "no current turn while combat is stopped");
    Check(!encounter.NextTurn(), "next turn does nothing outside combat");

    std::printf("\n-- Ties, extras, and 0 HP --\n");
    EncounterBuilder ties;
    const u32 a = ties.Add("goblin");
    const u32 b = ties.Add("goblin");
    ScriptDie tied({12, 12});
    Check(ties.RollInitiative([&]() { return tied.Roll(); }, {}), "tied roll succeeds");
    Check(ties.Order()[0].id == a && ties.Order()[1].id == b, "equal totals break toward the lower id");

    InitiativeInput ally;
    ally.id = EncounterBuilder::kPartyIdBase;
    ally.label = "Finn";
    ally.dexMod = 3;
    ScriptDie withAlly({10, 10, 10});
    Check(ties.RollInitiative([&]() { return withAlly.Roll(); }, { ally }), "party extra is rolled");
    Check(ties.Order().size() == 3, "extra is appended to the order");
    Check(ties.Order()[0].id == ally.id && ties.Order()[0].total == 13, "higher DEX wins a tied d20");
    Check(ties.Order()[0].label == "Finn", "extra keeps its label");

    Check(ties.ApplyDamage(a, 100), "drop a goblin");
    ScriptDie afterDrop({8, 8, 8});
    Check(ties.RollInitiative([&]() { return afterDrop.Roll(); }, {}), "re-roll after a knockout");
    Check(ties.Order().size() == 1 && ties.Order()[0].id == b, "0 HP members are left out of initiative");
    Check(ties.Find(a)->initiative < 0, "downed member's initiative is cleared");

    Check(ties.StartCombat(), "start with the survivor");
    Check(ties.ApplyDamage(b, 100), "knock out the active creature");
    Check(!ties.IsCombatActive(), "combat ends when nobody is standing");
    Check(ties.Order().size() == 1, "initiative entry is kept while they are at 0 HP");
    Check(!ties.StartCombat(), "start fails when every rolled creature is down");

    Check(ties.SetCurrentHp(b, 7), "healing the last creature does not require a new roll");
    Check(ties.StartCombat() && ties.CurrentTurn() && ties.CurrentTurn()->id == b,
          "start resumes on the revived creature");
    ties.EndCombat();

    Check(ties.SetCurrentHp(a, 9), "DM sets the other goblin back to full");
    Check(ties.Find(a)->currentHp == 9, "override restored HP");
    Check(ties.Order().size() == 1, "reviving does not invent an initiative entry");

    Check(ties.Remove(a), "remove works");
    Check(ties.Find(a) == nullptr, "removed member is gone");
    Check(!ties.Remove(a), "removing twice fails");

    std::printf("\n-- Empty roller --\n");
    EncounterBuilder emptyRoll;
    emptyRoll.Add("bugbear");
    Check(!emptyRoll.RollInitiative(D20RollFn(), {}), "missing die does not roll");
    Check(emptyRoll.Order().empty(), "failed roll leaves no order");

    std::printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
