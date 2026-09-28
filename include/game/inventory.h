/**
 * @file inventory.h
 * @brief D&D 5e inventory and equipment systems: item stacks, inventory component, equipment slots.
 *
 * Defines the ECS components for managing a character's carried items and
 * equipped gear:
 *   - ItemStack: a quantity of a single item type in an inventory
 *   - Inventory: a container of item stacks with add/remove/find operations
 *   - EquipmentSlots: tracked equipped items with slot-aware equip/unequip
 *
 * Design principles:
 *   - Inventory is a bounded container (maxItems limits stack count, not total quantity)
 *   - EquipmentSlots maps items to slots by type (WEAPON, ARMOR, RING, AMULET)
 *   - All item lookups are by name (case-sensitive, via std::strcmp)
 *
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include "game/item.h"
#include <vector>
#include <cstring>

namespace Dnd {

// ==========================================
// STRUCT: ItemStack
// ==========================================

/**
 * @struct ItemStack
 * @brief A stack of identical items in an inventory.
 *
 * Represents one or more copies of the same item (by pointer identity).
 * A stack is empty when the item pointer is null or quantity is 0.
 * Non-equippable items (potions, scrolls, tools) are correctly identified
 * by IsEquippable().
 */
struct ItemStack {
    const ItemData* item; ///< Pointer to the item definition. Null for empty stacks.
    i32 quantity;         ///< Number of copies in this stack. Must be >= 0.

    /**
     * @brief Default constructor. Creates an empty stack.
     */
    ItemStack()
        : item(nullptr), quantity(0) {}

    /**
     * @brief Parameterized constructor.
     * @param item Pointer to the item definition.
     * @param quantity Number of copies in this stack (default 1).
     */
    ItemStack(const ItemData* item, i32 quantity = 1)
        : item(item), quantity(quantity) {}

    /**
     * @brief Checks whether this stack is empty (no item or zero quantity).
     * @return true if the stack holds no items.
     */
    b8 IsEmpty() const {
        return !item || quantity <= 0;
    }

    /**
     * @brief Checks whether the item in this stack can be equipped.
     *
     * Equippable items are those that EquipmentSlots::Equip() accepts:
     * WEAPON, ARMOR, RING, and AMULET types.
     *
     * @return true if the item can be equipped to a slot.
     */
    b8 IsEquippable() const {
        if (!item) return false;
        return item->type == ItemType::WEAPON
            || item->type == ItemType::ARMOR
            || item->type == ItemType::RING
            || item->type == ItemType::AMULET;
    }
};

// ==========================================
// STRUCT: Inventory (ECS Component)
// ==========================================

/**
 * @struct Inventory
 * @brief ECS component tracking all items carried by a character.
 *
 * Stores a vector of ItemStack entries, each representing one or more
 * copies of a single item. The maxItems cap limits the number of distinct
 * stacks (not the total item count). Adding more of an existing item
 * bypasses the cap and merges into the existing stack.
 *
 * Usage:
 *   Inventory inv;
 *   const ItemData* potion = ItemDatabase::GetItem("Potion of Healing");
 *   inv.AddItem(potion, 3);
 *   i32 count = inv.GetItemCount("Potion of Healing"); // 3
 *   inv.RemoveItem("Potion of Healing", 1); // now 2
 *
 * @note All item lookups use case-sensitive name comparison.
 * @warning AddItem returns false only when a new stack is needed and
 *          the stack count has reached maxItems.
 */
struct Inventory {
    std::vector<ItemStack> items; ///< All item stacks in this inventory.
    i32 maxItems;                 ///< Maximum number of distinct stacks allowed.

    /**
     * @brief Default constructor. Initializes with 20 max stacks.
     */
    Inventory()
        : maxItems(20) {}

    /**
     * @brief Parameterized constructor.
     * @param maxItems Maximum number of distinct stacks (default 20).
     */
    explicit Inventory(i32 maxItems)
        : maxItems(maxItems) {}

    /**
     * @brief Adds a quantity of an item to the inventory.
     *
     * If the item already exists in a stack, the quantity is merged
     * into that stack (bypassing the maxItems cap). Otherwise a new
     * stack is created if there is room.
     *
     * @param item The item to add. Must not be null.
     * @param quantity Number of copies to add (default 1). Must be > 0.
     * @return true if the item was added or merged successfully.
     *         Returns false if item is null, quantity <= 0, or the
     *         inventory is full and the item is not already present.
     */
    b8 AddItem(const ItemData* item, i32 quantity = 1) {
        if (!item || quantity <= 0) return false;

        // Merge into existing stack if present
        for (auto& stack : items) {
            if (stack.item == item || std::strcmp(stack.item->name, item->name) == 0) {
                stack.quantity += quantity;
                return true;
            }
        }

        // Check capacity for new stack
        if (static_cast<i32>(items.size()) >= maxItems) return false;

        items.push_back(ItemStack(item, quantity));
        return true;
    }

    /**
     * @brief Removes a quantity of an item from the inventory.
     *
     * Finds the first stack with a matching item name and decrements
     * its quantity. If the stack reaches 0, it is removed entirely.
     *
     * @param itemName The name of the item to remove.
     * @param quantity Number of copies to remove (default 1). Must be > 0.
     * @return true if the item was found and removed. Returns false if
     *         the item was not found, itemName is null, or quantity <= 0.
     */
    b8 RemoveItem(const char* itemName, i32 quantity = 1) {
        if (!itemName || quantity <= 0) return false;

        for (auto it = items.begin(); it != items.end(); ++it) {
            if (it->item && std::strcmp(it->item->name, itemName) == 0) {
                it->quantity -= quantity;
                if (it->quantity <= 0) {
                    items.erase(it);
                }
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Returns the total quantity of an item in the inventory.
     *
     * Searches all stacks for a matching item name and returns the
     * quantity of the first match. Since stacks are unique per item,
     * there will be at most one matching stack.
     *
     * @param itemName The item name to search for.
     * @return The quantity of the matching stack, or 0 if not found
     *         or itemName is null.
     */
    i32 GetItemCount(const char* itemName) const {
        if (!itemName) return 0;

        for (const auto& stack : items) {
            if (stack.item && std::strcmp(stack.item->name, itemName) == 0) {
                return stack.quantity;
            }
        }
        return 0;
    }

    /**
     * @brief Finds the first stack matching the given item name.
     *
     * Returns a mutable pointer to the ItemStack so callers can
     * inspect or modify the stack. The pointer is valid until the
     * inventory's items vector is modified.
     *
     * @param itemName The item name to search for.
     * @return Pointer to the matching ItemStack, or nullptr if not found.
     */
    ItemStack* FindItem(const char* itemName) {
        if (!itemName) return nullptr;

        for (auto& stack : items) {
            if (stack.item && std::strcmp(stack.item->name, itemName) == 0) {
                return &stack;
            }
        }
        return nullptr;
    }

    /**
     * @brief Finds the first stack matching the given item name (const).
     * @param itemName The item name to search for.
     * @return Const pointer to the matching ItemStack, or nullptr.
     */
    const ItemStack* FindItem(const char* itemName) const {
        if (!itemName) return nullptr;

        for (const auto& stack : items) {
            if (stack.item && std::strcmp(stack.item->name, itemName) == 0) {
                return &stack;
            }
        }
        return nullptr;
    }
};

// ==========================================
// STRUCT: EquipmentSlots (ECS Component)
// ==========================================

/**
 * @struct EquipmentSlots
 * @brief ECS component tracking all equipped items on a character.
 *
 * Manages 10 equipment slots: weapon, armor, shield, two rings, amulet,
 * helmet, boots, gloves, and cloak. The Equip() method auto-assigns items
 * to the correct slot based on ItemType and ArmorType.
 *
 * Slot assignment logic:
 *   - WEAPON items go to the weapon slot
 *   - ARMOR items with ArmorType::SHIELD go to the shield slot
 *   - ARMOR items with any other ArmorType go to the armor slot
 *   - RING items go to ring1 (if empty) or ring2
 *   - AMULET items go to the amulet slot
 *   - All other item types cannot be auto-equipped via Equip()
 *
 * The helmet, boots, gloves, and cloak slots must be managed manually
 * via Unequip() / direct slot assignment for CLOTHING items.
 *
 * Usage:
 *   EquipmentSlots slots;
 *   slots.Equip(ItemDatabase::GetItem("Longsword"));
 *   slots.Equip(ItemDatabase::GetItem("Leather Armor"));
 *   const ItemData* weapon = slots.GetSlot("weapon");
 */
struct EquipmentSlots {
    const ItemData* weapon;  ///< Main hand or two-handed weapon.
    const ItemData* armor;   ///< Body armor (light, medium, heavy).
    const ItemData* shield;  ///< Shield (off-hand).
    const ItemData* ring1;   ///< First ring slot.
    const ItemData* ring2;   ///< Second ring slot.
    const ItemData* amulet;  ///< Neck/amulet slot.
    const ItemData* helmet;  ///< Head/helmet slot.
    const ItemData* boots;   ///< Feet/boots slot.
    const ItemData* gloves;  ///< Hands/gloves slot.
    const ItemData* cloak;   ///< Back/cloak slot.

    /**
     * @brief Default constructor. All slots start empty (nullptr).
     */
    EquipmentSlots()
        : weapon(nullptr), armor(nullptr), shield(nullptr)
        , ring1(nullptr), ring2(nullptr), amulet(nullptr)
        , helmet(nullptr), boots(nullptr), gloves(nullptr), cloak(nullptr) {}

    /**
     * @brief Equips an item into the correct slot automatically.
     *
     * Determines the correct slot based on item type:
     *   - WEAPON -> weapon slot
     *   - ARMOR (non-SHIELD) -> armor slot
     *   - ARMOR (SHIELD) -> shield slot
     *   - RING -> ring1 (preferred) or ring2
     *   - AMULET -> amulet slot
     *
     * @param item The item to equip. Must not be null.
     * @return true if the item was successfully equipped.
     *         Returns false if the item is null, the slot is already
     *         occupied, or the item type cannot be auto-equipped.
     */
    b8 Equip(const ItemData* item) {
        if (!item) return false;

        switch (item->type) {
            case ItemType::WEAPON:
                if (weapon) return false;
                weapon = item;
                return true;

            case ItemType::ARMOR:
                if (item->armor.type == ArmorType::SHIELD) {
                    if (shield) return false;
                    shield = item;
                } else {
                    if (armor) return false;
                    armor = item;
                }
                return true;

            case ItemType::RING:
                if (!ring1) { ring1 = item; return true; }
                if (!ring2) { ring2 = item; return true; }
                return false;

            case ItemType::AMULET:
                if (amulet) return false;
                amulet = item;
                return true;

            default:
                return false;
        }
    }

    /**
     * @brief Unequips the item in the given slot by name.
     *
     * Sets the specified slot to nullptr. Acceptable slot names:
     *   "weapon", "armor", "shield", "ring1", "ring2", "amulet",
     *   "helmet", "boots", "gloves", "cloak"
     *
     * @param slotName The name of the slot to unequip.
     * @return true if the slot was found and cleared.
     *         Returns false if the slot name is unknown or null.
     */
    b8 Unequip(const char* slotName) {
        if (!slotName) return false;

        if      (std::strcmp(slotName, "weapon") == 0)  { weapon = nullptr; return true; }
        else if (std::strcmp(slotName, "armor") == 0)   { armor = nullptr; return true; }
        else if (std::strcmp(slotName, "shield") == 0)  { shield = nullptr; return true; }
        else if (std::strcmp(slotName, "ring1") == 0)   { ring1 = nullptr; return true; }
        else if (std::strcmp(slotName, "ring2") == 0)   { ring2 = nullptr; return true; }
        else if (std::strcmp(slotName, "amulet") == 0)  { amulet = nullptr; return true; }
        else if (std::strcmp(slotName, "helmet") == 0)  { helmet = nullptr; return true; }
        else if (std::strcmp(slotName, "boots") == 0)   { boots = nullptr; return true; }
        else if (std::strcmp(slotName, "gloves") == 0)  { gloves = nullptr; return true; }
        else if (std::strcmp(slotName, "cloak") == 0)   { cloak = nullptr; return true; }

        return false;
    }

    /**
     * @brief Returns a pointer to the item in the specified slot.
     *
     * Acceptable slot names: "weapon", "armor", "shield", "ring1",
     * "ring2", "amulet", "helmet", "boots", "gloves", "cloak".
     *
     * @param slotName The name of the slot to query.
     * @return Pointer to the item in the slot, or nullptr if the slot
     *         is empty, the slot name is unknown, or slotName is null.
     */
    const ItemData* GetSlot(const char* slotName) const {
        if (!slotName) return nullptr;

        if      (std::strcmp(slotName, "weapon") == 0)  { return weapon; }
        else if (std::strcmp(slotName, "armor") == 0)   { return armor; }
        else if (std::strcmp(slotName, "shield") == 0)  { return shield; }
        else if (std::strcmp(slotName, "ring1") == 0)   { return ring1; }
        else if (std::strcmp(slotName, "ring2") == 0)   { return ring2; }
        else if (std::strcmp(slotName, "amulet") == 0)  { return amulet; }
        else if (std::strcmp(slotName, "helmet") == 0)  { return helmet; }
        else if (std::strcmp(slotName, "boots") == 0)   { return boots; }
        else if (std::strcmp(slotName, "gloves") == 0)  { return gloves; }
        else if (std::strcmp(slotName, "cloak") == 0)   { return cloak; }

        return nullptr;
    }
};

} // namespace Dnd
