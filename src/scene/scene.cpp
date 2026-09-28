/**
 * @file scene.cpp
 * @brief Implementation of the Scene class.
 * 
 * See scene.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#include "scene/scene.h"
#include "core/log.h"

namespace Dnd {

Scene::Scene(const char* sceneName)
    : m_name(sceneName), m_entityManager(1024), m_isLoaded(false) {
}

Scene::~Scene() {
    if (m_isLoaded) {
        OnUnload();
    }
}

void Scene::OnLoad() {
    m_isLoaded = true;
    DND_LOG_INFO("Scene loaded: %s", m_name);
}

void Scene::OnUnload() {
    // Shut down all systems
    m_systemManager.Clear(m_entityManager, m_componentRegistry);

    // Clear all components and entities
    m_componentRegistry.Clear();
    m_entityManager.Clear();

    m_isLoaded = false;
    DND_LOG_INFO("Scene unloaded: %s", m_name);
}

void Scene::Update(f32 deltaTime) {
    if (!m_isLoaded) {
        return;
    }

    // Per-frame subclass hook (animation, etc.) before the systems run.
    OnUpdate(deltaTime);

    // Run all systems in all phases
    m_systemManager.RunAll(deltaTime, m_entityManager, m_componentRegistry);
}

} // namespace Dnd
