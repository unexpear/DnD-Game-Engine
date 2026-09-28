/**
 * @file component.h
 * @brief Type-safe component storage with sparse set implementation.
 * 
 * The ComponentRegistry manages all component types in the ECS. Each component
 * type gets its own sparse set, which stores component data in a dense array
 * for cache-friendly iteration while allowing O(1) lookup by entity.
 * 
 * Sparse set design:
 *   - Dense array: Contiguous storage of component data (cache-friendly iteration)
 *   - Sparse array: Maps entity index -> dense index (O(1) lookup)
 *   - When a component is added, it's appended to the dense array
 *   - When removed, the last element is swapped in (order not preserved)
 * 
 * Component types are identified by a unique type ID generated at compile time
 * using a template static counter. This avoids RTTI and works across DLLs.
 * 
 * @note Components must be POD types (no virtual functions, no complex ctors).
 * @warning Component storage is NOT thread-safe. Access from the main thread only.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include <any>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cassert>

namespace Dnd {

// ==========================================
// TYPE ID GENERATION
// ==========================================

/**
 * @brief Generates the next unique type ID.
 * @return Monotonically increasing type ID.
 */
u32 GenerateNextTypeId();

/**
 * @brief Generates a unique type ID at compile time for each component type.
 * 
 * Uses a static counter incremented per type instantiation. The counter
 * is stored in a template specialization, so each type gets a unique ID.
 * This avoids RTTI overhead and works consistently across all platforms.
 * 
 * @tparam T The component type.
 * @return Unique type ID for the component type.
 */
template<typename T>
u32 GetComponentTypeId() {
    static u32 id = GenerateNextTypeId();
    return id;
}

// ==========================================
// CLASS: SparseSet
// ==========================================

/**
 * @class SparseSet
 * @brief Sparse set data structure for storing components of a single type.
 * 
 * A sparse set provides:
 *   - O(1) component insertion
 *   - O(1) component removal (swap-with-last, order not preserved)
 *   - O(1) component lookup by entity
 *   - Cache-friendly iteration over all components
 * 
 * The sparse array maps entity indices to dense array indices. The dense
 * array stores the actual component data contiguously.
 * 
 * @tparam T The component data type (must be movable).
 */
template<typename T>
class SparseSet {
public:
    /**
     * @brief Adds a component to an entity.
     * @param entityIndex The entity's index (from EntityManager).
     * @param component The component data to store.
     */
    void Insert(u32 entityIndex, const T& component) {
        // Check if entity already has this component
        if (Has(entityIndex)) {
            // Update existing component
            m_dense[m_sparse[entityIndex]] = component;
            return;
        }

        // Expand the entity-indexed arrays if needed. m_sparse AND m_entityToDense
        // are both indexed by entityIndex; previously only m_sparse was resized, so
        // the writes below indexed past the end of (empty) vectors and corrupted
        // memory — a hard crash on the very first insert.
        if (entityIndex >= m_sparse.size()) {
            m_sparse.resize(entityIndex + 1, INVALID_DENSE_INDEX);
        }
        if (entityIndex >= m_entityToDense.size()) {
            m_entityToDense.resize(entityIndex + 1, INVALID_DENSE_INDEX);
        }

        // Append to dense array
        u32 denseIndex = static_cast<u32>(m_dense.size());
        m_dense.push_back(component);
        m_sparse[entityIndex] = denseIndex;
        m_entityToDense[entityIndex] = denseIndex;

        // m_denseToEntity is indexed by denseIndex (it parallels m_dense); grow to match.
        if (denseIndex >= m_denseToEntity.size()) {
            m_denseToEntity.resize(denseIndex + 1, INVALID_DENSE_INDEX);
        }
        m_denseToEntity[denseIndex] = entityIndex;
    }

    /**
     * @brief Removes a component from an entity.
     * @param entityIndex The entity's index.
     * @return true if the component was removed, false if it didn't exist.
     */
    b8 Remove(u32 entityIndex) {
        if (!Has(entityIndex)) {
            return false;
        }

        u32 denseIndex = m_sparse[entityIndex];
        u32 lastIndex = static_cast<u32>(m_dense.size()) - 1;

        if (denseIndex != lastIndex) {
            // Swap with last element to maintain dense array contiguity
            m_dense[denseIndex] = std::move(m_dense[lastIndex]);
            u32 swappedEntityIndex = m_denseToEntity[lastIndex];
            m_sparse[swappedEntityIndex] = denseIndex;
            m_entityToDense[swappedEntityIndex] = denseIndex;
            m_denseToEntity[denseIndex] = swappedEntityIndex;
        }

        m_dense.pop_back();
        m_sparse[entityIndex] = INVALID_DENSE_INDEX;
        m_entityToDense[entityIndex] = INVALID_DENSE_INDEX;
        // Note: m_denseToEntity[lastIndex] is now invalid (popped)

        return true;
    }

    /**
     * @brief Checks if an entity has this component.
     * @param entityIndex The entity's index.
     * @return true if the entity has this component.
     */
    b8 Has(u32 entityIndex) const {
        return entityIndex < m_sparse.size() &&
               m_sparse[entityIndex] != INVALID_DENSE_INDEX;
    }

    /**
     * @brief Gets a pointer to an entity's component.
     * @param entityIndex The entity's index.
     * @return Pointer to the component, or nullptr if not found.
     */
    T* Get(u32 entityIndex) {
        if (!Has(entityIndex)) {
            return nullptr;
        }
        return &m_dense[m_sparse[entityIndex]];
    }

    /// @brief Const version of Get.
    const T* Get(u32 entityIndex) const {
        if (!Has(entityIndex)) {
            return nullptr;
        }
        return &m_dense[m_sparse[entityIndex]];
    }

    /// @brief Returns the number of components in this set.
    u32 Size() const { return static_cast<u32>(m_dense.size()); }

    /// @brief Returns true if the set is empty.
    b8 Empty() const { return m_dense.empty(); }

    /// @brief Removes all components.
    void Clear() {
        m_dense.clear();
        for (auto& index : m_sparse) {
            index = INVALID_DENSE_INDEX;
        }
        m_denseToEntity.clear();
        m_entityToDense.clear();
    }

    // Iteration: iterate over dense array for cache-friendly access
    T* begin() { return m_dense.data(); }
    T* end() { return m_dense.data() + m_dense.size(); }
    const T* begin() const { return m_dense.data(); }
    const T* end() const { return m_dense.data() + m_dense.size(); }

    /// @brief Returns the dense array of components.
    const std::vector<T>& GetDense() const { return m_dense; }

    /// @brief Returns the entity index for a given dense index.
    u32 GetEntityIndex(u32 denseIndex) const {
        assert(denseIndex < m_denseToEntity.size() && "GetEntityIndex: denseIndex out of bounds");
        return m_denseToEntity[denseIndex];
    }

private:
    static constexpr u32 INVALID_DENSE_INDEX = 0xFFFFFFFF;

    std::vector<T> m_dense; ///< Dense array of component data
    std::vector<u32> m_sparse; ///< Sparse array: entity index -> dense index
    std::vector<u32> m_entityToDense; ///< Entity index -> dense index (mirror of sparse)
    std::vector<u32> m_denseToEntity; ///< Dense index -> entity index
};

// ==========================================
// CLASS: ComponentRegistry
// ==========================================

/**
 * @class ComponentRegistry
 * @brief Type-safe registry for managing all component types.
 * 
 * The registry stores a separate SparseSet for each component type.
 * Components are added/removed/queried by type using template methods.
 * 
 * Usage:
 *   ComponentRegistry registry;
 *   registry.AddComponent<Transform>(entity, transformData);
 *   Transform* t = registry.GetComponent<Transform>(entity);
 *   registry.RemoveComponent<Transform>(entity);
 */
class ComponentRegistry {
public:
    /**
     * @brief Adds a component to an entity.
     * @tparam T The component type.
     * @param entity The entity to add the component to.
     * @param component The component data.
     */
    template<typename T>
    void AddComponent(Entity entity, const T& component) {
        auto* sparseSet = GetSparseSet<T>();
        sparseSet->Insert(Entity::GetIndex(entity.id), component);
    }

    /**
     * @brief Removes a component from an entity.
     * @tparam T The component type.
     * @param entity The entity to remove the component from.
     * @return true if the component was removed, false if it didn't exist.
     */
    template<typename T>
    b8 RemoveComponent(Entity entity) {
        auto* sparseSet = GetSparseSet<T>();
        return sparseSet->Remove(Entity::GetIndex(entity.id));
    }

    /**
     * @brief Gets a pointer to an entity's component.
     * @tparam T The component type.
     * @param entity The entity.
     * @return Pointer to the component, or nullptr if not found.
     */
    template<typename T>
    T* GetComponent(Entity entity) {
        auto* sparseSet = GetSparseSet<T>();
        return sparseSet->Get(Entity::GetIndex(entity.id));
    }

    /// @brief Const version of GetComponent (returns nullptr if type unregistered).
    template<typename T>
    const T* GetComponent(Entity entity) const {
        auto* sparseSet = GetSparseSet<T>();
        return sparseSet ? sparseSet->Get(Entity::GetIndex(entity.id)) : nullptr;
    }

    template<typename T>
    b8 HasComponent(Entity entity) const {
        auto* sparseSet = GetSparseSet<T>();
        return sparseSet ? sparseSet->Has(Entity::GetIndex(entity.id)) : false;
    }

    /**
     * @brief Gets the sparse set for a specific component type.
     * @tparam T The component type.
     * @return Pointer to the SparseSet<T>.
     */
    template<typename T>
    SparseSet<T>* GetSparseSet() {
        u32 typeId = GetComponentTypeId<T>();
        if (m_sets.find(typeId) == m_sets.end()) {
            m_sets[typeId] = std::make_unique<SparseSetWrapper<T>>();
        }
        return static_cast<SparseSetWrapper<T>*>(m_sets[typeId].get())->GetSet();
    }

    /// @brief Const version of GetSparseSet.
    template<typename T>
    const SparseSet<T>* GetSparseSet() const {
        auto it = m_sets.find(GetComponentTypeId<T>());
        if (it == m_sets.end()) {
            return nullptr;
        }
        return static_cast<const SparseSetWrapper<T>*>(it->second.get())->GetSet();
    }

    /**
     * @brief Removes all components of all types.
     */
    void Clear() {
        for (auto& [typeId, set] : m_sets) {
            set->Clear();
        }
    }

private:
    /// @brief Base class for type-erased sparse set storage.
    class SparseSetBase {
    public:
        virtual ~SparseSetBase() = default;
        virtual void Clear() = 0;
    };

    /// @brief Typed wrapper for SparseSet storage.
    template<typename T>
    class SparseSetWrapper : public SparseSetBase {
    public:
        SparseSet<T>* GetSet() { return &m_set; }
        const SparseSet<T>* GetSet() const { return &m_set; }
        void Clear() override { m_set.Clear(); }
    private:
        SparseSet<T> m_set;
    };

    std::unordered_map<u32, std::unique_ptr<SparseSetBase>> m_sets; ///< Type ID -> SparseSet
};

} // namespace Dnd
