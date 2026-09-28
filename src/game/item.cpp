/**
 * @file item.cpp
 * @brief Implementation of the item database with PHB equipment table.
 *
 * Defines the compile-time item data table containing standard D&D 5e
 * weapons, armor, and consumables from the Player's Handbook. The table
 * provides base statistics for all core equipment items.
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "game/item.h"

namespace Dnd {

// ==========================================
// ITEM DATA TABLE
// ==========================================

static const ItemData s_itemTable[] = {
    // ==========================================
    // Dagger
    // ==========================================
    {
        "Dagger",
        "A small, balanced blade usable in melee or thrown at nearby foes.",
        RarityType::COMMON,
        1.0f,
        2,
        false, false,
        WeaponData(
            WeaponType::DAGGER,
            DamageRoll(DiceRoll(1, 4), DamageType::PIERCING),
            WeaponProperties(true, false, true, false, false, false, true, false, false, 20.0f, 60.0f)
        )
    },

    // ==========================================
    // Shortsword
    // ==========================================
    {
        "Shortsword",
        "A light, double-edged blade favored by rogues and skirmishers.",
        RarityType::COMMON,
        2.0f,
        10,
        false, false,
        WeaponData(
            WeaponType::SHORTSWORD,
            DamageRoll(DiceRoll(1, 6), DamageType::PIERCING),
            WeaponProperties(true, false, true, false, false, false, false, false, false)
        )
    },

    // ==========================================
    // Longsword
    // ==========================================
    {
        "Longsword",
        "A versatile martial sword that can be wielded in one or two hands.",
        RarityType::COMMON,
        3.0f,
        15,
        false, false,
        WeaponData(
            WeaponType::LONGSWORD,
            DamageRoll(DiceRoll(1, 8), DamageType::SLASHING),
            WeaponProperties(false, false, false, false, false, false, false, false, true)
        )
    },

    // ==========================================
    // Greatsword
    // ==========================================
    {
        "Greatsword",
        "A massive two-handed blade that cleaves through armor and flesh.",
        RarityType::COMMON,
        6.0f,
        50,
        false, false,
        WeaponData(
            WeaponType::GREATSWORD,
            DamageRoll(DiceRoll(2, 6), DamageType::SLASHING),
            WeaponProperties(false, true, false, false, false, false, false, true, false)
        )
    },

    // ==========================================
    // Longbow
    // ==========================================
    {
        "Longbow",
        "A large bow capable of firing arrows at great distances with power.",
        RarityType::COMMON,
        2.0f,
        50,
        false, false,
        WeaponData(
            WeaponType::LONGBOW,
            DamageRoll(DiceRoll(1, 8), DamageType::PIERCING),
            WeaponProperties(false, true, false, false, true, false, false, true, false, 150.0f, 600.0f)
        )
    },

    // ==========================================
    // Shortbow
    // ==========================================
    {
        "Shortbow",
        "A compact bow used for rapid firing at medium range.",
        RarityType::COMMON,
        2.0f,
        25,
        false, false,
        WeaponData(
            WeaponType::SHORTBOW,
            DamageRoll(DiceRoll(1, 6), DamageType::PIERCING),
            WeaponProperties(false, false, false, false, true, false, false, true, false, 80.0f, 320.0f)
        )
    },

    // ==========================================
    // Quarterstaff
    // ==========================================
    {
        "Quarterstaff",
        "A simple wooden staff useful as both a tool and a versatile weapon.",
        RarityType::COMMON,
        4.0f,
        0,
        false, false,
        WeaponData(
            WeaponType::QUARTERSTAFF,
            DamageRoll(DiceRoll(1, 6), DamageType::BLUDGEONING),
            WeaponProperties(false, false, false, false, false, false, false, false, true)
        )
    },

    // ==========================================
    // Club
    // ==========================================
    {
        "Club",
        "A crude but effective bludgeoning weapon, little more than a thick branch.",
        RarityType::COMMON,
        2.0f,
        0,
        false, false,
        WeaponData(
            WeaponType::CLUB,
            DamageRoll(DiceRoll(1, 4), DamageType::BLUDGEONING),
            WeaponProperties(false, false, true, false, false, false, false, false, false)
        )
    },

    // ==========================================
    // Leather Armor
    // ==========================================
    {
        "Leather Armor",
        "Light armor made of tough, flexible leather. Provides base AC 11 + DEX modifier.",
        RarityType::COMMON,
        10.0f,
        10,
        false, false,
        ArmorData(ArmorType::LIGHT, 11, 99, false)
    },

    // ==========================================
    // Chain Mail
    // ==========================================
    {
        "Chain Mail",
        "Heavy armor of interlocking metal rings. Provides base AC 16. Requires STR 13.",
        RarityType::COMMON,
        55.0f,
        75,
        false, false,
        ArmorData(ArmorType::HEAVY, 16, 0, true)
    },

    // ==========================================
    // Shield
    // ==========================================
    {
        "Shield",
        "A wooden or metal shield worn on the forearm. Provides +2 to AC.",
        RarityType::COMMON,
        6.0f,
        10,
        false, false,
        ArmorData(ArmorType::SHIELD, 2, 0, false)
    },

    // ==========================================
    // Potion of Healing
    // ==========================================
    {
        "Potion of Healing",
        "A crimson liquid that restores 2d4+2 hit points when consumed.",
        ItemType::POTION,
        RarityType::COMMON,
        0.5f,
        50,
        true, false
    }
};

static constexpr i32 s_itemCount = sizeof(s_itemTable) / sizeof(s_itemTable[0]);

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

b8 ItemDatabase::s_initialized = false;

// ==========================================
// ItemDatabase::Initialize
// ==========================================

void ItemDatabase::Initialize() {
    if (s_initialized) {
        return;
    }
    s_initialized = true;
}

// ==========================================
// ItemDatabase::GetItem
// ==========================================

const ItemData* ItemDatabase::GetItem(const char* name) {
    if (!s_initialized) {
        Initialize();
    }
    if (!name) {
        return nullptr;
    }
    for (i32 i = 0; i < s_itemCount; i++) {
        if (std::strcmp(s_itemTable[i].name, name) == 0) {
            return &s_itemTable[i];
        }
    }
    return nullptr;
}

// ==========================================
// ItemDatabase::GetItemTable
// ==========================================

const ItemData* ItemDatabase::GetItemTable() {
    return s_itemTable;
}

// ==========================================
// ItemDatabase::GetItemCount
// ==========================================

i32 ItemDatabase::GetItemCount() {
    return s_itemCount;
}

} // namespace Dnd
