/**
 * @file scene_manager.cpp
 * @brief Implementation of the SceneManager class.
 * 
 * See scene_manager.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#include "scene/scene_manager.h"
#include "core/log.h"

namespace Dnd {

void SceneManager::PushScene(std::unique_ptr<Scene> scene) {
    // Unload the current top scene if it exists
    if (!m_sceneStack.empty()) {
        m_sceneStack.back()->OnUnload();
    }

    // Load and push the new scene
    scene->OnLoad();
    m_sceneStack.push_back(std::move(scene));

    DND_LOG_DEBUG("Scene pushed: %s (stack size: %u)",
                  m_sceneStack.back()->GetName(),
                  static_cast<u32>(m_sceneStack.size()));
}

b8 SceneManager::PopScene() {
    if (m_sceneStack.empty()) {
        DND_LOG_WARN("Cannot pop scene: stack is empty");
        return false;
    }

    // Unload the top scene
    m_sceneStack.back()->OnUnload();
    m_sceneStack.pop_back();

    // Load the new top scene if any
    if (!m_sceneStack.empty()) {
        m_sceneStack.back()->OnLoad();
        DND_LOG_DEBUG("Scene popped, returned to: %s", m_sceneStack.back()->GetName());
    } else {
        DND_LOG_DEBUG("Scene popped, stack is now empty");
    }

    return true;
}

void SceneManager::SetActiveScene(std::unique_ptr<Scene> scene) {
    // Unload all current scenes
    Clear();

    // Load and push the new scene
    scene->OnLoad();
    m_sceneStack.push_back(std::move(scene));

    DND_LOG_INFO("Active scene set: %s", m_sceneStack.back()->GetName());
}

Scene* SceneManager::GetActiveScene() const {
    if (m_sceneStack.empty()) {
        return nullptr;
    }
    return m_sceneStack.back().get();
}

void SceneManager::UpdateActiveScene(f32 deltaTime) {
    Scene* activeScene = GetActiveScene();
    if (activeScene) {
        activeScene->Update(deltaTime);
    }
}

void SceneManager::Clear() {
    for (auto& scene : m_sceneStack) {
        scene->OnUnload();
    }
    m_sceneStack.clear();
    DND_LOG_DEBUG("All scenes cleared");
}

} // namespace Dnd
