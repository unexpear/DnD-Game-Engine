/**
 * @file system.cpp
 * @brief Implementation of the System and SystemManager classes.
 * 
 * See system.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#include "ecs/system.h"
#include "core/log.h"
#include <algorithm>

namespace Dnd {

// ==========================================
// SYSTEM
// ==========================================

System::System(const char* name, SystemPhase phase, i32 priority)
    : m_name(name), m_phase(phase), m_priority(priority), m_enabled(true) {
}

void System::OnInit(EntityManager& /*em*/, ComponentRegistry& /*cr*/) {
    DND_LOG_DEBUG("System initialized: %s", m_name);
}

void System::OnShutdown(EntityManager& /*em*/, ComponentRegistry& /*cr*/) {
    DND_LOG_DEBUG("System shut down: %s", m_name);
}

// ==========================================
// SYSTEM MANAGER
// ==========================================

void SystemManager::RegisterSystem(std::unique_ptr<System> system,
                                    EntityManager& em, ComponentRegistry& cr) {
    SystemPhase phase = system->GetPhase();
    auto& phaseList = m_phases[static_cast<i32>(phase)];

    auto it = std::lower_bound(phaseList.begin(), phaseList.end(), system,
        [](const std::unique_ptr<System>& a, const std::unique_ptr<System>& b) {
            return a->GetPriority() < b->GetPriority();
        });

    auto* newSystem = phaseList.insert(it, std::move(system))->get();
    newSystem->OnInit(em, cr);

    DND_LOG_DEBUG("System registered: %s (phase: %d, priority: %d)",
                  newSystem->GetName(), static_cast<i32>(newSystem->GetPhase()),
                  newSystem->GetPriority());
}

b8 SystemManager::UnregisterSystem(const char* name, EntityManager& em, ComponentRegistry& cr) {
    for (auto& phaseList : m_phases) {
        for (auto it = phaseList.begin(); it != phaseList.end(); ++it) {
            if (std::strcmp((*it)->GetName(), name) == 0) {
                (*it)->OnShutdown(em, cr);
                phaseList.erase(it);
                DND_LOG_DEBUG("System unregistered: %s", name);
                return true;
            }
        }
    }
    DND_LOG_WARN("System not found for unregistration: %s", name);
    return false;
}

void SystemManager::RunPhase(SystemPhase phase, f32 deltaTime,
                              EntityManager& em, ComponentRegistry& cr) {
    auto& phaseList = m_phases[static_cast<i32>(phase)];
    for (auto& system : phaseList) {
        if (system->IsEnabled()) {
            system->OnUpdate(deltaTime, em, cr);
        }
    }
}

void SystemManager::RunAll(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) {
    for (auto& phaseList : m_phases) {
        for (auto& system : phaseList) {
            if (system->IsEnabled()) {
                system->OnUpdate(deltaTime, em, cr);
            }
        }
    }
}

void SystemManager::Clear(EntityManager& em, ComponentRegistry& cr) {
    for (auto& phaseList : m_phases) {
        for (auto& system : phaseList) {
            system->OnShutdown(em, cr);
        }
        phaseList.clear();
    }
    DND_LOG_DEBUG("All systems cleared");
}

} // namespace Dnd
