/**
 * @file system.h
 * @brief Base class for ECS systems.
 * 
 * Systems contain the logic that processes entities with specific component
 * combinations. Each system runs during a specific phase of the game loop
 * (Update, FixedUpdate, Render, etc.).
 * 
 * System execution order:
 *   1. Systems are registered with a priority (lower = runs first)
 *   2. Systems are sorted by priority at registration
 *   3. During each game loop phase, all matching systems are executed
 * 
 * System types:
 *   - Update: Runs every frame, variable timestep
 *   - FixedUpdate: Runs at fixed intervals, deterministic physics
 *   - Render: Runs during the render pass, draws visible objects
 *   - LateUpdate: Runs after all updates, camera follow, etc.
 * 
 * @note Systems do NOT store entity state. They read/write components.
 * @warning Systems must be registered before the game loop starts.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include <memory>
#include <vector>
#include <cstring>

namespace Dnd {

// Forward declarations
class ComponentRegistry;
class EntityManager;

// ==========================================
// ENUM: SystemPhase
// ==========================================

/**
 * @enum SystemPhase
 * @brief Execution phases for systems in the game loop.
 * 
 * Order matters: systems in FixedUpdate run before Update, etc.
 */
enum class SystemPhase : i32 {
    FIXED_UPDATE = 0, ///< Deterministic physics updates
    UPDATE = 1,       ///< General game logic updates
    LATE_UPDATE = 2,  ///< Post-update logic (camera follow, etc.)
    RENDER = 3        ///< Rendering pass
};

// ==========================================
// CLASS: System
// ==========================================

/**
 * @class System
 * @brief Abstract base class for ECS systems.
 * 
 * Derive from this class to create custom systems. Override OnUpdate()
 * to implement system logic. Use the provided EntityManager and
 * ComponentRegistry to query and modify entities.
 * 
 * Example:
 *   class MovementSystem : public System {
 *   public:
 *       MovementSystem() : System("MovementSystem", SystemPhase::UPDATE, 100) {}
 *   protected:
 *       void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) override {
 *           // Process entities with Transform + Velocity components
 *       }
 *   };
 */
class System {
public:
    /**
     * @brief Constructs a system with a name, phase, and priority.
     * @param name Human-readable system name (for debugging).
     * @param phase The game loop phase this system runs in.
     * @param priority Execution order within the phase (lower = first).
     */
    System(const char* name, SystemPhase phase, i32 priority);

    virtual ~System() = default;

    /**
     * @brief Called once when the system is registered.
     * @param em Entity manager reference.
     * @param cr Component registry reference.
     */
    virtual void OnInit(EntityManager& em, ComponentRegistry& cr);

    /**
     * @brief Called once when the system is unregistered.
     * @param em Entity manager reference.
     * @param cr Component registry reference.
     */
    virtual void OnShutdown(EntityManager& em, ComponentRegistry& cr);

    /**
     * @brief Called every frame during the system's phase.
     * @param deltaTime Time since last frame (or fixed timestep for FIXED_UPDATE).
     * @param em Entity manager reference.
     * @param cr Component registry reference.
     */
    virtual void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) = 0;

    /// @brief Returns the system's name.
    const char* GetName() const { return m_name; }

    /// @brief Returns the system's phase.
    SystemPhase GetPhase() const { return m_phase; }

    /// @brief Returns the system's priority (lower = runs first).
    i32 GetPriority() const { return m_priority; }

    /// @brief Returns true if the system is enabled (will run).
    b8 IsEnabled() const { return m_enabled; }

    /// @brief Enables or disables the system.
    void SetEnabled(b8 enabled) { m_enabled = enabled; }

protected:
    const char* m_name; ///< Human-readable system name
    SystemPhase m_phase; ///< Execution phase
    i32 m_priority; ///< Priority within phase (lower = first)
    b8 m_enabled; ///< Whether the system is active
};

// ==========================================
// CLASS: SystemManager
// ==========================================

/**
 * @class SystemManager
 * @brief Manages registration and execution of ECS systems.
 * 
 * Systems are organized by phase and sorted by priority within each phase.
 * Call RunPhase() during the appropriate part of the game loop to execute
 * all systems for that phase.
 */
class SystemManager {
public:
    /**
     * @brief Registers a system.
     * 
     * Implementation:
     *   1. Add the system to the appropriate phase list
     *   2. Sort the phase list by priority
     *   3. Call system->OnInit()
     * 
     * @param system Unique pointer to the system.
     * @param em Entity manager reference (passed to OnInit).
     * @param cr Component registry reference (passed to OnInit).
     */
    void RegisterSystem(std::unique_ptr<System> system,
                        EntityManager& em, ComponentRegistry& cr);

    /**
     * @brief Unregisters a system by name.
     * @param name The system name.
     * @param em Entity manager reference.
     * @param cr Component registry reference.
     * @return true if the system was found and removed.
     */
    b8 UnregisterSystem(const char* name, EntityManager& em, ComponentRegistry& cr);

    /**
     * @brief Runs all systems in a specific phase.
     * @param phase The phase to execute.
     * @param deltaTime Time delta for this frame.
     * @param em Entity manager reference.
     * @param cr Component registry reference.
     */
    void RunPhase(SystemPhase phase, f32 deltaTime,
                  EntityManager& em, ComponentRegistry& cr);

    /**
     * @brief Runs all systems in all phases.
     * @param deltaTime Time delta for this frame.
     * @param em Entity manager reference.
     * @param cr Component registry reference.
     */
    void RunAll(f32 deltaTime, EntityManager& em, ComponentRegistry& cr);

    /**
     * @brief Gets a system by name.
     * @tparam T The system type.
     * @param name The system name.
     * @return Pointer to the system, or nullptr if not found.
     */
    template<typename T>
    T* GetSystem(const char* name) {
        for (auto& phaseList : m_phases) {
            for (auto& sys : phaseList) {
                if (std::strcmp(sys->GetName(), name) == 0) {
                    return static_cast<T*>(sys.get());
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief Unregisters all systems.
     * @param em Entity manager reference.
     * @param cr Component registry reference.
     */
    void Clear(EntityManager& em, ComponentRegistry& cr);

private:
    /// @brief All systems organized by phase.
    std::vector<std::unique_ptr<System>> m_phases[4]; ///< One vector per SystemPhase
};

} // namespace Dnd
