/**
 * @file item.h
 * @brief D&D 5e item system: definitions, types, and static database.
 *
 * Defines the complete item system used by inventory and equipment:
 *   - RarityType, ItemType, WeaponType, ArmorType enumerations
 *   - WeaponProperties, WeaponData, ArmorData, ItemData structs
 *   - ItemDatabase static class for compile-time item lookup
 *
 * Design principles:
 *   - ItemData is POD-like; weapon and armor data stored inline
 *   - ItemDatabase uses a static table populated at compile time
 *   - All item lookups are O(n) over the small PHB table
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include "game/combat.h"
#include <cstring>

namespace Dnd {

// ==========================================
// ENUM: RarityType
// ==========================================

/**
 * @enum RarityType
 * @brief The six tiers of item rarity in D&D 5e.
 *
 * Rarity determines the general power level and market value of magic items.
 * COMMON items are everyday magical gear, while ARTIFACT items are unique
 * legendary relics that can shape campaigns.
 */
enum class RarityType : u32 {
    COMMON    = 0, ///< Everyday gear. Potions, basic scrolls, simple magic items.
    UNCOMMON  = 1, ///< Adventurer-grade gear. +1 weapons, basic wondrous items.
    RARE      = 2, ///< Powerful gear. +2 weapons, significant magic items.
    VERY_RARE = 3, ///< Extremely powerful. +3 weapons, major artifacts-lite.
    LEGENDARY = 4, ///< Campaign-defining power. Vorpal sword, holy avenger.
    ARTIFACT  = 5  ///< Unique, world-shaping relics. Hand of Vecna, Eye of the Deep.
};

// ==========================================
// ENUM: ItemType
// ==========================================

/**
 * @enum ItemType
 * @brief Broad category of an item, determining its usage and equippable slot.
 *
 * Each item belongs to exactly one type. The type determines how the item
 * can be used (consumed, equipped, wielded, etc.) and which equipment slot
 * it occupies when equipped.
 */
enum class ItemType : u32 {
    WEAPON       = 0, ///< Melee or ranged weapon. Wielded in weapon hand(s).
    ARMOR        = 1, ///< Body armor or shield. Worn on torso or arm.
    POTION       = 2, ///< Consumable magical liquid. Used from inventory.
    SCROLL       = 3, ///< One-use spell scroll. Consumed on use.
    WAND         = 4, ///< Spellcasting focus with charges. Wielded in hand.
    RING         = 5, ///< Finger-worn magic item. Equipped in ring slot.
    AMULET       = 6, ///< Neck-worn magic item. Equipped in amulet slot.
    TOOL         = 7, ///< Adventuring tool (crowbar, lockpicks, etc.).
    CLOTHING     = 8, ///< Non-armor worn items (cloak, boots, hat, etc.).
    FOOD         = 9, ///< Rations, trail bread, or magical sustenance.
    LIGHT        = 10, ///< Light source (torch, lantern, etc.).
    AMMUNITION   = 11, ///< Arrows, bolts, sling stones, or other projectiles.
    MISC         = 12  ///< Anything that doesn't fit another category.
};

// ==========================================
// ENUM: WeaponType
// ==========================================

/**
 * @enum WeaponType
 * @brief Specific weapon classification within the WEAPON item type.
 *
 * Maps to D&D 5e weapon list. Each weapon type has defined damage dice,
 * properties, and proficiency requirements.
 */
enum class WeaponType : u32 {
    CLUB         = 0, ///< Simple melee club. 1d4 bludgeoning, light.
    DAGGER       = 1, ///< Simple melee/thrown dagger. 1d4 piercing, finesse, light, thrown.
    GREATSWORD   = 2, ///< Martial heavy blade. 2d6 slashing, heavy, two-handed.
    HANDAXE      = 3, ///< Simple melee/thrown axe. 1d6 slashing, light, thrown.
    LONGBOW      = 4, ///< Martial ranged bow. 1d8 piercing, heavy, two-handed, range.
    LONGSWORD    = 5, ///< Martial versatile blade. 1d8 slashing, versatile.
    MACE         = 6, ///< Simple melee bludgeon. 1d6 bludgeoning.
    QUARTERSTAFF = 7, ///< Simple versatile staff. 1d6 bludgeoning, versatile.
    RAPIER       = 8, ///< Martial finesse blade. 1d8 piercing, finesse.
    SHORTBOW     = 9, ///< Simple ranged bow. 1d6 piercing, two-handed, range.
    SHORTSWORD   = 10, ///< Martial light blade. 1d6 piercing, finesse, light.
    SPEAR        = 11, ///< Simple versatile polearm. 1d6 piercing, thrown, versatile.
    STAFF        = 12, ///< Arcane focus quarterstaff. 1d6 bludgeoning, versatile.
    WARHAMMER    = 13, ///< Martial heavy hammer. 1d8 bludgeoning, versatile.
    SCIMITAR     = 14  ///< Martial curved blade. 1d6 slashing, finesse, light.
};

// ==========================================
// ENUM: ArmorType
// ==========================================

/**
 * @enum ArmorType
 * @brief Category of armor, determining AC calculation and DEX contribution.
 *
 * Light armor adds full DEX modifier to AC. Medium armor adds up to +2 DEX.
 * Heavy armor ignores DEX entirely. Shields provide a flat +2 AC bonus.
 */
enum class ArmorType : u32 {
    LIGHT   = 0, ///< Leather, Studded Leather. AC = base + DEX mod (full).
    MEDIUM  = 1, ///< Hide, Chain Shirt, Scale Mail. AC = base + DEX mod (max +2).
    HEAVY   = 2, ///< Ring Mail, Chain Mail, Plate. AC = base (no DEX).
    SHIELD  = 3  ///< Shield. AC = +2 (flat bonus, no DEX contribution).
};

// ==========================================
// STRUCT: WeaponProperties
// ==========================================

/**
 * @struct WeaponProperties
 * @brief Describes the mechanical properties of a weapon per D&D 5e rules.
 *
 * Each boolean property corresponds to a D&D 5e weapon property keyword
 * (Finesse, Heavy, Light, Loading, Ranged, Reach, Thrown, Two-Handed,
 * Versatile). Ranged weapons store their normal and long range in feet.
 *
 * @note The Ammunition property is omitted; ranged weapons use isRanged + rangeNormal.
 * @warning isLoading applies only to crossbows and similar weapons.
 */
struct WeaponProperties {
    b8 isFinesse;   ///< Can use DEX instead of STR for attack/damage.
    b8 isHeavy;     ///< Small creatures have disadvantage on attack rolls.
    b8 isLight;     ///< Can dual-wield with another light weapon.
    b8 isLoading;   ///< Can only fire one shot per action regardless of Extra Attack.
    b8 isRanged;    ///< Ranged weapon (uses rangeNormal/rangeLong).
    b8 isReach;     ///< Adds 5 ft to melee reach (typically 10 ft total).
    b8 isThrown;    ///< Can be thrown as a ranged attack.
    b8 isTwoHanded; ///< Requires two hands to wield.
    b8 isVersatile; ///< Can be wielded two-handed for higher damage.
    f32 rangeNormal; ///< Normal range in feet. Attacks beyond this have disadvantage.
    f32 rangeLong;   ///< Long range in feet. Cannot attack beyond this distance.

    /**
     * @brief Default constructor. All properties set to false/zero.
     */
    WeaponProperties()
        : isFinesse(false), isHeavy(false), isLight(false), isLoading(false)
        , isRanged(false), isReach(false), isThrown(false), isTwoHanded(false)
        , isVersatile(false), rangeNormal(0.0f), rangeLong(0.0f) {}

    /**
     * @brief Parameterized constructor for full weapon property specification.
     * @param isFinesse Whether the weapon has the finesse property.
     * @param isHeavy Whether the weapon has the heavy property.
     * @param isLight Whether the weapon has the light property.
     * @param isLoading Whether the weapon has the loading property.
     * @param isRanged Whether the weapon is a ranged weapon.
     * @param isReach Whether the weapon has the reach property.
     * @param isThrown Whether the weapon has the thrown property.
     * @param isTwoHanded Whether the weapon requires two hands.
     * @param isVersatile Whether the weapon is versatile.
     * @param rangeNormal Normal range in feet (default 0 for melee).
     * @param rangeLong Long range in feet (default 0 for melee).
     */
    WeaponProperties(b8 isFinesse, b8 isHeavy, b8 isLight, b8 isLoading,
                     b8 isRanged, b8 isReach, b8 isThrown, b8 isTwoHanded,
                     b8 isVersatile, f32 rangeNormal = 0.0f, f32 rangeLong = 0.0f)
        : isFinesse(isFinesse), isHeavy(isHeavy), isLight(isLight), isLoading(isLoading)
        , isRanged(isRanged), isReach(isReach), isThrown(isThrown), isTwoHanded(isTwoHanded)
        , isVersatile(isVersatile), rangeNormal(rangeNormal), rangeLong(rangeLong) {}
};

// ==========================================
// STRUCT: WeaponData
// ==========================================

/**
 * @struct WeaponData
 * @brief Full weapon statistics: type, damage roll, and properties.
 *
 * Stored inline within ItemData for weapons. Non-weapon items use
 * the default WeaponData (WeaponType::CLUB, zero damage).
 */
struct WeaponData {
    WeaponType type;                ///< Specific weapon classification.
    DamageRoll damage;              ///< Damage dice and type (e.g., 1d8 slashing).
    WeaponProperties properties;    ///< Weapon property flags and range.

    /**
     * @brief Default constructor. Sets type to CLUB with zero damage.
     */
    WeaponData()
        : type(WeaponType::CLUB), damage(), properties() {}

    /**
     * @brief Parameterized constructor.
     * @param type The weapon type.
     * @param damage The damage roll (dice + type).
     * @param properties Weapon properties.
     */
    WeaponData(WeaponType type, const DamageRoll& damage, const WeaponProperties& properties)
        : type(type), damage(damage), properties(properties) {}
};

// ==========================================
// STRUCT: ArmorData
// ==========================================

/**
 * @struct ArmorData
 * @brief Full armor statistics: type, AC contribution, and stealth penalty.
 *
 * Stored inline within ItemData for armor items. The baseAC field provides
 * the base armor value for body armor or the flat AC bonus for shields.
 * maxDexBonus caps the DEX modifier contribution (99 = unlimited).
 */
struct ArmorData {
    ArmorType type;        ///< Armor category (light, medium, heavy, shield).
    i32 baseAC;            ///< Base AC for body armor, or +2 for shields.
    i32 maxDexBonus;       ///< Maximum DEX modifier allowed. 99 = no cap. 0 = no DEX.
    b8 givesDisadvantageStealth; ///< If true, Stealth checks have disadvantage.

    /**
     * @brief Default constructor. Creates unarmored baseline (AC 10, light, no cap).
     */
    ArmorData()
        : type(ArmorType::LIGHT), baseAC(10), maxDexBonus(99), givesDisadvantageStealth(false) {}

    /**
     * @brief Parameterized constructor.
     * @param type Armor category.
     * @param baseAC Base AC value (10 for unarmored, 11 for leather, etc.).
     * @param maxDexBonus Maximum DEX modifier contribution. 99 = unlimited.
     * @param givesDisadvantageStealth Whether stealth checks are at disadvantage.
     */
    ArmorData(ArmorType type, i32 baseAC, i32 maxDexBonus, b8 givesDisadvantageStealth)
        : type(type), baseAC(baseAC), maxDexBonus(maxDexBonus)
        , givesDisadvantageStealth(givesDisadvantageStealth) {}
};

// ==========================================
// STRUCT: ItemData
// ==========================================

/**
 * @struct ItemData
 * @brief Full definition of a single D&D 5e item.
 *
 * Contains all information needed to represent an item in the world,
 * in an inventory, or equipped on a character. Weapon and armor specifics
 * are stored inline; non-weapon/armor items use default sub-structs.
 *
 * Usage:
 *   const ItemData* sword = ItemDatabase::GetItem("Longsword");
 *   if (sword->type == ItemType::WEAPON) {
 *       DamageRoll dmg = sword->weapon.damage;
 *   }
 */
struct ItemData {
    const char* name;                ///< Item name (e.g., "Longsword", "Potion of Healing").
    const char* description;         ///< Flavor and mechanical description text.
    ItemType type;                   ///< Broad item category.
    RarityType rarity;               ///< Rarity tier.
    f32 weight;                      ///< Weight in pounds.
    i32 value;                       ///< Market value in gold pieces (gp).
    b8 isMagical;                    ///< Whether the item has magical properties.
    b8 requiresAttunement;           ///< Whether the item requires attunement to use.
    WeaponData weapon;               ///< Weapon statistics (valid when type == WEAPON).
    ArmorData armor;                 ///< Armor statistics (valid when type == ARMOR).

    /**
     * @brief Default constructor. Creates an empty COMMON MISC item.
     */
    ItemData()
        : name(""), description(""), type(ItemType::MISC), rarity(RarityType::COMMON)
        , weight(0.0f), value(0), isMagical(false), requiresAttunement(false)
        , weapon(), armor() {}

    /**
     * @brief Constructs a non-equipment item (potions, scrolls, tools, etc.).
     * @param name Item name.
     * @param description Item description.
     * @param type Item category (must not be WEAPON or ARMOR).
     * @param rarity Rarity tier.
     * @param weight Weight in pounds.
     * @param value Value in gold pieces.
     * @param isMagical Whether the item is magical.
     * @param requiresAttunement Whether attunement is required.
     */
    ItemData(const char* name, const char* description, ItemType type,
             RarityType rarity, f32 weight, i32 value,
             b8 isMagical = false, b8 requiresAttunement = false)
        : name(name), description(description), type(type), rarity(rarity)
        , weight(weight), value(value), isMagical(isMagical)
        , requiresAttunement(requiresAttunement), weapon(), armor() {}

    /**
     * @brief Constructs a weapon item.
     * @param name Item name.
     * @param description Item description.
     * @param rarity Rarity tier.
     * @param weight Weight in pounds.
     * @param value Value in gold pieces.
     * @param isMagical Whether the item is magical.
     * @param requiresAttunement Whether attunement is required.
     * @param weapon Weapon data (type, damage, properties).
     */
    ItemData(const char* name, const char* description, RarityType rarity,
             f32 weight, i32 value, b8 isMagical, b8 requiresAttunement,
             const WeaponData& weapon)
        : name(name), description(description), type(ItemType::WEAPON), rarity(rarity)
        , weight(weight), value(value), isMagical(isMagical)
        , requiresAttunement(requiresAttunement), weapon(weapon), armor() {}

    /**
     * @brief Constructs an armor item.
     * @param name Item name.
     * @param description Item description.
     * @param rarity Rarity tier.
     * @param weight Weight in pounds.
     * @param value Value in gold pieces.
     * @param isMagical Whether the item is magical.
     * @param requiresAttunement Whether attunement is required.
     * @param armor Armor data (type, AC, DEX cap, stealth).
     */
    ItemData(const char* name, const char* description, RarityType rarity,
             f32 weight, i32 value, b8 isMagical, b8 requiresAttunement,
             const ArmorData& armor)
        : name(name), description(description), type(ItemType::ARMOR), rarity(rarity)
        , weight(weight), value(value), isMagical(isMagical)
        , requiresAttunement(requiresAttunement), weapon(), armor(armor) {}
};

// ==========================================
// CLASS: ItemDatabase
// ==========================================

/**
 * @class ItemDatabase
 * @brief Static lookup database for item definitions.
 *
 * Provides read-only access to a pre-defined table of D&D 5e items
 * (Player's Handbook equipment). The table is populated at compile time.
 * Initialize() must be called once at engine startup before GetItem().
 *
 * Usage:
 *   ItemDatabase::Initialize();
 *   const ItemData* sword = ItemDatabase::GetItem("Longsword");
 *   if (sword) { // use sword data }
 *
 * @note GetItem() performs a case-sensitive linear search over the table.
 *       The table is small (< 100 items), so performance is not a concern.
 * @warning Initialize() is safe to call multiple times — subsequent calls
 *          are no-ops once initialized.
 */
class ItemDatabase {
public:
    /**
     * @brief Initializes the item database.
     *
     * Must be called once before any calls to GetItem(). Subsequent
     * calls are no-ops. Currently a placeholder for future runtime
     * table building (e.g., loading modded items).
     */
    static void Initialize();

    /**
     * @brief Looks up an item by its name.
     *
     * Performs a case-sensitive string comparison against all items
     * in the database. Returns a pointer to the internal static item
     * data, which remains valid for the lifetime of the program.
     *
     * @param name The item name to search for (e.g., "Longsword").
     * @return Pointer to the matching ItemData, or nullptr if not found.
     */
    static const ItemData* GetItem(const char* name);

private:
    /**
     * @brief Returns the internal static item table.
     * @return Pointer to the compile-time item data array.
     */
    static const ItemData* GetItemTable();

    /**
     * @brief Returns the number of items in the static table.
     * @return Item count.
     */
    static i32 GetItemCount();

    static b8 s_initialized; ///< Whether Initialize() has been called.
};

} // namespace Dnd
