/**
 * @file encounter_builder.cpp
 * @brief Encounter roster, catalog, and initiative ordering.
 *
 * See encounter_builder.h. Damage and healing go through HitPoints so the
 * toolkit does not keep a second copy of those rules.
 *
 * @author DND Engine Team
 * @date 2026-09-28
 * @version 0.1.0
 */

#include "tools/encounter_builder.h"
#include <algorithm>
#include <cstring>

namespace Dnd {

namespace {

/**
 * @brief Clamps an integer into [lo, hi].
 * @param v Value to clamp.
 * @param lo Inclusive lower bound.
 * @param hi Inclusive upper bound.
 * @return Clamped value.
 */
i32 ClampI(i32 v, i32 lo, i32 hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

} // namespace

const std::vector<MonsterStatBlock>& EncounterBuilder::Catalog() {
    // HP/AC match DndDemoScene::PlaceMazeEnemies floor baselines
    // (9/12, 14/13, 19/14). DEX mods match CreateEnemy's per-level scores.
    static const std::vector<MonsterStatBlock> kCatalog = {
        { "goblin",   "Goblin",    9, 12, 0, "demo floor 1", Color(0.50f, 0.68f, 0.18f, 1.0f) },
        { "skeleton", "Skeleton", 14, 13, 1, "demo floor 2", Color(0.88f, 0.82f, 0.72f, 1.0f) },
        { "bugbear",  "Bugbear",  19, 14, 1, "demo floor 3", Color(0.38f, 0.22f, 0.08f, 1.0f) },
    };
    return kCatalog;
}

const MonsterStatBlock* EncounterBuilder::FindStat(const char* statId) {
    if (!statId) return nullptr;
    for (const MonsterStatBlock& block : Catalog()) {
        if (std::strcmp(block.id, statId) == 0) return &block;
    }
    return nullptr;
}

std::vector<InitiativeRoll> RollInitiativeOrder(const std::vector<InitiativeInput>& combatants,
                                                const D20RollFn& rollD20) {
    std::vector<InitiativeRoll> rolls;
    if (!rollD20) return rolls;
    rolls.reserve(combatants.size());
    for (const InitiativeInput& in : combatants) {
        InitiativeRoll roll;
        roll.id = in.id;
        roll.label = in.label;
        roll.d20 = rollD20();
        roll.total = roll.d20 + in.dexMod;
        rolls.push_back(roll);
    }
    std::sort(rolls.begin(), rolls.end(), [](const InitiativeRoll& a, const InitiativeRoll& b) {
        if (a.total != b.total) return a.total > b.total;
        return a.id < b.id;
    });
    return rolls;
}

u32 EncounterBuilder::Add(const char* statId) {
    const MonsterStatBlock* block = FindStat(statId);
    if (!block) return 0;

    i32 existing = 0;
    for (const EncounterMember& member : m_members) {
        if (member.statId == block->id) ++existing;
    }

    EncounterMember member;
    member.id = m_nextId++;
    member.statId = block->id;
    member.name = block->name;
    if (existing > 0) {
        member.name += " ";
        member.name += std::to_string(existing + 1);
    }
    member.currentHp = block->maxHp;
    member.maxHp = block->maxHp;
    member.armorClass = block->armorClass;
    member.dexMod = block->dexMod;
    member.tint = block->tint;
    m_members.push_back(member);
    return member.id;
}

b8 EncounterBuilder::Remove(u32 memberId) {
    for (size_t i = 0; i < m_members.size(); ++i) {
        if (m_members[i].id == memberId) {
            DropFromOrder(memberId);
            m_members.erase(m_members.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

EncounterMember* EncounterBuilder::Find(u32 memberId) {
    for (EncounterMember& member : m_members) {
        if (member.id == memberId) return &member;
    }
    return nullptr;
}

const EncounterMember* EncounterBuilder::Find(u32 memberId) const {
    for (const EncounterMember& member : m_members) {
        if (member.id == memberId) return &member;
    }
    return nullptr;
}

b8 EncounterBuilder::Owns(Entity entity) const {
    if (!entity.IsValid()) return false;
    for (const EncounterMember& member : m_members) {
        if (member.entity.id == entity.id) return true;
    }
    return false;
}

b8 EncounterBuilder::SetCurrentHp(u32 memberId, i32 hp) {
    EncounterMember* member = Find(memberId);
    if (!member) return false;
    member->currentHp = ClampI(hp, 0, member->maxHp);
    SkipDowned();
    return true;
}

b8 EncounterBuilder::SetMaxHp(u32 memberId, i32 hp) {
    EncounterMember* member = Find(memberId);
    if (!member || hp < 1) return false;
    member->maxHp = hp;
    if (member->currentHp > member->maxHp) member->currentHp = member->maxHp;
    SkipDowned();
    return true;
}

b8 EncounterBuilder::ApplyDamage(u32 memberId, i32 amount) {
    EncounterMember* member = Find(memberId);
    if (!member || amount <= 0) return false;
    HitPoints hp(member->currentHp, member->maxHp, 0);
    const i32 before = hp.current;
    hp.TakeDamage(amount);
    member->currentHp = hp.current;
    SkipDowned();
    return member->currentHp != before;
}

b8 EncounterBuilder::Heal(u32 memberId, i32 amount) {
    EncounterMember* member = Find(memberId);
    if (!member || amount <= 0) return false;
    HitPoints hp(member->currentHp, member->maxHp, 0);
    const i32 before = hp.current;
    hp.Heal(amount);
    member->currentHp = hp.current;
    return member->currentHp != before;
}

b8 EncounterBuilder::RollInitiative(const D20RollFn& rollD20,
                                   const std::vector<InitiativeInput>& extras) {
    if (!rollD20) return false;

    std::vector<InitiativeInput> inputs;
    inputs.reserve(m_members.size() + extras.size());
    for (const EncounterMember& member : m_members) {
        if (member.currentHp <= 0) continue;
        InitiativeInput in;
        in.id = member.id;
        in.label = member.name;
        in.dexMod = member.dexMod;
        inputs.push_back(in);
    }
    for (const InitiativeInput& extra : extras) {
        b8 duplicate = false;
        for (const InitiativeInput& in : inputs) {
            if (in.id == extra.id) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) inputs.push_back(extra);
    }

    m_order = RollInitiativeOrder(inputs, rollD20);
    m_turnIndex = 0;
    if (m_order.empty()) m_combatActive = false;

    for (EncounterMember& member : m_members) {
        member.initiative = -1;
        member.d20 = -1;
    }
    for (const InitiativeRoll& roll : m_order) {
        if (EncounterMember* member = Find(roll.id)) {
            member->initiative = roll.total;
            member->d20 = roll.d20;
        }
    }
    return !m_order.empty();
}

b8 EncounterBuilder::StartCombat() {
    if (m_order.empty()) return false;
    m_combatActive = true;
    m_turnIndex = 0;
    SkipDowned();
    return m_combatActive;
}

void EncounterBuilder::EndCombat() {
    m_combatActive = false;
    m_turnIndex = 0;
}

b8 EncounterBuilder::NextTurn() {
    if (!m_combatActive || m_order.empty()) return false;
    ++m_turnIndex;
    if (m_turnIndex >= static_cast<i32>(m_order.size())) m_turnIndex = 0;
    SkipDowned();
    return m_combatActive;
}

const InitiativeRoll* EncounterBuilder::CurrentTurn() const {
    if (!m_combatActive || m_order.empty()) return nullptr;
    if (m_turnIndex < 0 || m_turnIndex >= static_cast<i32>(m_order.size())) return nullptr;
    return &m_order[static_cast<size_t>(m_turnIndex)];
}

void EncounterBuilder::SkipDowned() {
    if (!m_combatActive) return;
    if (m_order.empty()) {
        m_combatActive = false;
        m_turnIndex = 0;
        return;
    }

    const i32 size = static_cast<i32>(m_order.size());
    if (m_turnIndex < 0 || m_turnIndex >= size) m_turnIndex = 0;
    for (i32 step = 0; step < size; ++step) {
        const i32 index = (m_turnIndex + step) % size;
        const EncounterMember* member = Find(m_order[static_cast<size_t>(index)].id);
        // Party extras are not roster members; the scene still tracks their HP.
        if (member && member->currentHp <= 0) continue;
        m_turnIndex = index;
        return;
    }
    m_combatActive = false;
    m_turnIndex = 0;
}

void EncounterBuilder::DropFromOrder(u32 id) {
    for (i32 i = 0; i < static_cast<i32>(m_order.size()); ++i) {
        if (m_order[static_cast<size_t>(i)].id != id) continue;
        m_order.erase(m_order.begin() + i);
        if (m_order.empty()) {
            m_combatActive = false;
            m_turnIndex = 0;
            return;
        }
        if (i < m_turnIndex) --m_turnIndex;
        if (m_turnIndex >= static_cast<i32>(m_order.size())) m_turnIndex = 0;
        if (m_turnIndex < 0) m_turnIndex = 0;
        return;
    }
}

} // namespace Dnd
