/**
 * @file entity.cpp
 * @brief Implementation of the EntityManager class.
 * 
 * See entity.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#include "ecs/entity.h"
#include "core/log.h"

namespace Dnd {

// ==========================================
// TYPE ID GENERATION
// ==========================================

static u32 s_nextTypeId = 0;

u32 GenerateNextTypeId() {
    return s_nextTypeId++;
}

// ==========================================
// ENTITY MANAGER
// ==========================================

EntityManager::EntityManager(u32 initialCapacity)
    : m_aliveCount(0) {
    // Preallocate entity slots
    m_slots.resize(initialCapacity);
    for (u32 i = 0; i < initialCapacity; i++) {
        m_slots[i].isAlive = false;
        m_slots[i].generation = 0;
        m_freeList.push_back(i);
    }

    DND_LOG_DEBUG("EntityManager initialized with capacity %u", initialCapacity);
}

Entity EntityManager::Create() {
    u32 index;
    u16 generation;

    if (!m_freeList.empty()) {
        // Reuse a free slot
        index = m_freeList.back();
        m_freeList.pop_back();
        generation = m_slots[index].generation;
    } else {
        // Expand the pool
        index = static_cast<u32>(m_slots.size());
        generation = 0;
        m_slots.push_back({ false, 0 });
    }

    // Mark as alive and encode the entity ID
    m_slots[index].isAlive = true;
    u32 entityId = Entity::EncodeId(index, generation);

    m_aliveCount++;
    return Entity(entityId);
}

b8 EntityManager::Destroy(Entity entity) {
    if (!IsValid(entity)) {
        DND_LOG_WARN("Attempted to destroy invalid entity (ID: 0x%08X)", entity.id);
        return false;
    }

    u32 index = Entity::GetIndex(entity.id);

    // Mark as dead and increment generation
    m_slots[index].isAlive = false;
    m_slots[index].generation++;

    // Wrap generation if it overflows
    if (m_slots[index].generation > Entity::MAX_GENERATION) {
        m_slots[index].generation = 0;
    }

    // Push to free list for reuse
    m_freeList.push_back(index);
    m_aliveCount--;

    return true;
}

u16 EntityManager::GetGeneration(u32 index) const {
    if (index >= m_slots.size()) return 0;
    return m_slots[index].generation;
}

b8 EntityManager::IsValid(Entity entity) const {
    if (entity.id == Entity::INVALID_ID) {
        return false;
    }

    u32 index = Entity::GetIndex(entity.id);
    u16 generation = Entity::GetGeneration(entity.id);

    // Check bounds
    if (index >= m_slots.size()) {
        return false;
    }

    // Check alive status and generation match
    return m_slots[index].isAlive && m_slots[index].generation == generation;
}

Entity EntityManager::Restore(u32 index, u16 generation) {
    if (index >= m_slots.size()) {
        return Entity::None();
    }

    // If the slot is already alive with a different generation, don't overwrite
    if (m_slots[index].isAlive) {
        return Entity(Entity::EncodeId(index, m_slots[index].generation));
    }

    // Remove from free list if present
    for (size_t i = 0; i < m_freeList.size(); ++i) {
        if (m_freeList[i] == index) {
            m_freeList.erase(m_freeList.begin() + static_cast<i32>(i));
            break;
        }
    }

    m_slots[index].isAlive = true;
    m_slots[index].generation = generation;
    m_aliveCount++;
    return Entity(Entity::EncodeId(index, generation));
}

u32 EntityManager::GetAliveCount() const {
    return m_aliveCount;
}

u32 EntityManager::GetCapacity() const {
    return static_cast<u32>(m_slots.size());
}

void EntityManager::Clear() {
    for (auto& slot : m_slots) {
        slot.isAlive = false;
        slot.generation = 0;
    }
    m_freeList.clear();
    for (u32 i = 0; i < m_slots.size(); i++) {
        m_freeList.push_back(i);
    }
    m_aliveCount = 0;

    DND_LOG_DEBUG("EntityManager cleared");
}

} // namespace Dnd
