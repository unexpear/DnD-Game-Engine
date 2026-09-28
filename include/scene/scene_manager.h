/**
 * @file scene_manager.h
 * @brief Manager for loading, unloading, and switching scenes.
 * 
 * The SceneManager maintains a stack of scenes, allowing for scene
 * transitions (push/pop). The top scene is the active scene.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "scene/scene.h"
#include "core/types.h"
#include <memory>
#include <vector>

namespace Dnd {

/**
 * @class SceneManager
 * @brief Manages scene lifecycle and transitions.
 * 
 * Scenes are stored in a stack:
 *   - PushScene: Loads a new scene on top of the current one
 *   - PopScene: Unloads the top scene, returning to the previous
 *   - SetActiveScene: Replaces the current scene entirely
 * 
 * Only the top scene is updated each frame.
 */
class SceneManager {
public:
    /**
     * @brief Pushes a new scene onto the stack.
     * 
     * Implementation:
     *   1. Call OnUnload() on the current top scene (if any)
     *   2. Call OnLoad() on the new scene
     *   3. Push the new scene onto the stack
     * 
     * @param scene Unique pointer to the scene.
     */
    void PushScene(std::unique_ptr<Scene> scene);

    /**
     * @brief Pops the top scene from the stack.
     * 
     * Implementation:
     *   1. Call OnUnload() on the top scene
     *   2. Remove the scene from the stack
     * 
     * @return true if a scene was popped, false if the stack was empty.
     */
    b8 PopScene();

    /**
     * @brief Sets the active scene (replaces all scenes).
     * 
     * Implementation:
     *   1. Unload all current scenes
     *   2. Clear the stack
     *   3. Load and push the new scene
     * 
     * @param scene Unique pointer to the scene.
     */
    void SetActiveScene(std::unique_ptr<Scene> scene);

    /**
     * @brief Gets the active (top) scene.
     * @return Pointer to the active scene, or nullptr if none.
     */
    Scene* GetActiveScene() const;

    /**
     * @brief Updates the active scene.
     * @param deltaTime Time since last frame.
     */
    void UpdateActiveScene(f32 deltaTime);

    /// @brief Returns the number of scenes on the stack.
    u32 GetSceneCount() const { return static_cast<u32>(m_sceneStack.size()); }

    /// @brief Returns true if there are no scenes loaded.
    b8 IsEmpty() const { return m_sceneStack.empty(); }

    /**
     * @brief Unloads all scenes and clears the stack.
     */
    void Clear();

private:
    std::vector<std::unique_ptr<Scene>> m_sceneStack; ///< Stack of loaded scenes
};

} // namespace Dnd
