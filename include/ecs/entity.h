/**
 * @file entity.h
 * @brief Entity type with generation counter for safe ID recycling.
 * 
 * Entities are lightweight identifiers (32-bit) with a generation counter
 * to prevent dangling references. When an entity is destroyed, its ID can
 * be recycled, but the generation counter increments. Any stale references
 * to the old entity will have a mismatched generation and be rejected.
 * 
 * Entity ID layout (32 bits):
 *   [31:20] Generation counter (12 bits, max 4096 recycles)
 *   [19:0]  Entity index (20 bits, max 1,048,576 concurrent entities)
 * 
 * The entity index maps to an array slot in the EntityManager. The
 * generation counter ensures that recycled IDs don't accidentally
 * reference new entities that happen to reuse the same slot.
 * 
 * @note Entity is a POD type — can be used in unions, serialized, etc.
 * @warning Entity handles do NOT imply the entity is alive. Always check
 *          validity via EntityManager::IsValid() before using.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "core/types.h"
#include <vector>

namespace Dnd {

// ==========================================
// STRUCT: Entity
// ==========================================

/**
 * @struct Entity
 * @brief Lightweight entity identifier with generation counter.
 * 
 * Entity is an opaque handle. The raw bits encode an index and generation.
 * Entity::None represents an invalid/null entity.
 */
struct Entity {
    u32 id; ///< Raw 32-bit ID (index + generation encoded)

    /// @brief Default constructor. Creates Entity::None.
    constexpr Entity() : id(INVALID_ID) {}

    /// @brief Constructs an entity from a raw ID.
    explicit constexpr Entity(u32 rawId) : id(rawId) {}

    /// @brief Equality comparison.
    constexpr bool operator==(const Entity& other) const { return id == other.id; }
    /// @brief Inequality comparison.
    constexpr bool operator!=(const Entity& other) const { return id != other.id; }
    /// @brief Less-than comparison (for use in sorted containers).
    constexpr bool operator<(const Entity& other) const { return id < other.id; }

    /// @brief Returns true if this entity is valid (not None).
    constexpr bool IsValid() const { return id != INVALID_ID; }

    /// @brief Special value representing an invalid/null entity.
    static constexpr Entity None() { return Entity(INVALID_ID); }

    /// @brief Bitmask for the index portion (20 bits).
    static constexpr u32 INDEX_MASK = 0xFFFFF;
    /// @brief Number of bits for the index portion.
    static constexpr u32 INDEX_BITS = 20;
    /// @brief Maximum number of concurrent entities.
    static constexpr u32 MAX_ENTITIES = (1u << INDEX_BITS) - 1;
    /// @brief Maximum generation counter value (must not produce encoded 0xFFFFFFFF).
    static constexpr u16 MAX_GENERATION = 0xFFE;

    /// @brief Extracts the entity index from a raw ID.
    static constexpr u32 GetIndex(u32 rawId) { return rawId & INDEX_MASK; }
    /// @brief Extracts the generation counter from a raw ID.
    static constexpr u16 GetGeneration(u32 rawId) { return static_cast<u16>((rawId >> INDEX_BITS) & 0xFFF); }
    /// @brief Encodes an index and generation into a raw entity ID.
    static constexpr u32 EncodeId(u32 index, u16 generation) {
        return (static_cast<u32>(generation) << INDEX_BITS) | (index & INDEX_MASK);
    }

    /// @brief Sentinel value representing an invalid/null entity.
    static constexpr u32 INVALID_ID = 0xFFFFFFFF;
};

// ==========================================
// CLASS: EntityManager
// ==========================================

/**
 * @class EntityManager
 * @brief Manages entity creation, destruction, and ID recycling.
 * 
 * Implementation:
 *   - Maintains a pool of entity slots, each with a generation counter
 *   - When an entity is created, finds a free slot or expands the pool
 *   - When destroyed, marks the slot as free (generation increments)
 *   - Free slots are tracked in a stack for O(1) allocation
 * 
 * Entity ID encoding:
 *   - Bits [19:0]: Index into the entity pool (max 1,048,576 entities)
 *   - Bits [31:20]: Generation counter (max 4096 recycles per slot)
 */
class EntityManager {
public:
    /**
     * @brief Constructs an EntityManager with preallocated capacity.
     * @param initialCapacity Number of entity slots to preallocate (default 1024).
     */
    explicit EntityManager(u32 initialCapacity = 1024);

    /**
     * @brief Creates a new entity.
     * 
     * Implementation:
     *   1. Pop a free slot from the free list, or expand the pool
     *   2. If reusing a slot, increment the generation counter
     *   3. Mark the slot as alive
     *   4. Return the entity handle (index + generation encoded)
     * 
     * @return Entity handle for the newly created entity.
     */
    Entity Create();

    /**
     * @brief Destroys an entity.
     * 
     * Implementation:
     *   1. Validate the entity (check index bounds and generation match)
     *   2. If valid, mark the slot as dead
     *   3. Increment the generation counter (wraps at max)
     *   4. Push the slot index onto the free list
     * 
     * @param entity The entity to destroy.
     * @return true if the entity was destroyed, false if it was already dead or invalid.
     */
    b8 Destroy(Entity entity);

    /**
     * @brief Checks if an entity is alive.
     * 
     * Implementation:
     *   1. Extract the index and generation from the entity ID
     *   2. Check if index is within bounds
     *   3. Check if the slot is marked as alive
     *   4. Check if the stored generation matches the entity's generation
     * 
     * @param entity The entity to check.
     * @return true if the entity is alive, false otherwise.
     */
    b8 IsValid(Entity entity) const;

    /** @brief Returns the number of alive entities. */
    u32 GetAliveCount() const;

    /** @brief Returns the total number of entity slots (alive + dead). */
    u32 GetCapacity() const;

    /** @brief Returns the generation counter for a slot index (0 if out of bounds). */
    u16 GetGeneration(u32 index) const;

    /**
     * @brief Iterates over all alive entities, calling `func(Entity)` for each.
     * 
     * Implementation:
     *   1. Walk every slot index up to GetCapacity()
     *   2. If slot is alive, reconstruct full Entity via EncodeId and invoke func
     * 
     * @tparam Func Callable accepting (Entity)
     * @param func The callable to invoke for each alive entity.
     */
    template<typename Func>
    void ForEachAlive(Func func) const {
        for (u32 i = 0; i < m_slots.size(); ++i) {
            const EntitySlot& slot = m_slots[i];
            if (slot.isAlive) {
                Entity e;
                e.id = Entity::EncodeId(i, slot.generation);
                func(e);
            }
        }
    }

    /**
     * @brief Restores a previously destroyed entity slot (editor undo).
     *
     * Forces the slot at the given index to be alive with the given generation.
     * If the slot is currently free, it is removed from the free list. This
     * is intended for editor undo operations where the slot has not been
     * recycled by a subsequent Create() call.
     *
     * @param index The entity slot index to restore.
     * @param generation The generation to restore (must match the slot or be stale).
     * @return Entity handle with the restored index and generation.
     */
    Entity Restore(u32 index, u16 generation);

    /**
     * @brief Destroys all entities and resets the manager.
     * 
     * Implementation:
     *   1. Clear all slots (mark as dead, reset generations)
     *   2. Clear the free list
     *   3. Reset alive count to 0
     */
    void Clear();

private:
    /// @brief Internal entity slot data.
    struct EntitySlot {
        b8 isAlive; ///< Whether this slot holds a live entity
        u16 generation; ///< Generation counter (increments on recycle)
    };

    std::vector<EntitySlot> m_slots; ///< Entity slots (alive/dead state + generation)
    std::vector<u32> m_freeList; ///< Stack of free slot indices for O(1) allocation
    u32 m_aliveCount; ///< Number of currently alive entities
};

} // namespace Dnd
