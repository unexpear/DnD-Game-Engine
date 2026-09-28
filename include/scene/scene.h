/**
 * @file scene.h
 * @brief Scene class containing entities, components, and systems.
 * 
 * A Scene is a container for all ECS data: entities, components, and systems.
 * Scenes can be loaded, unloaded, and switched by the SceneManager.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/system.h"
#include "core/types.h"

namespace Dnd {

/**
 * @class Scene
 * @brief Container for ECS entities, components, and systems.
 * 
 * Each scene has its own EntityManager, ComponentRegistry, and SystemManager.
 * Scenes are independent and can be loaded/unloaded independently.
 */
class Scene {
public:
    /**
     * @brief Constructs a scene with a name.
     * @param sceneName Human-readable scene name.
     */
    explicit Scene(const char* sceneName);

    virtual ~Scene();

    /**
     * @brief Called when the scene is loaded.
     * Override to set up scene-specific entities and components.
     */
    virtual void OnLoad();

    /**
     * @brief Called when the scene is unloaded.
     * Override to clean up scene-specific resources.
     */
    virtual void OnUnload();

    /**
     * @brief Per-frame hook for subclasses to add custom logic (animation, etc.).
     * Called by Update() before the systems run. Default does nothing.
     * @param deltaTime Time since last frame.
     */
    virtual void OnUpdate(f32 deltaTime) { (void)deltaTime; }

    /**
     * @brief Per-frame hook for subclasses to draw a 2D overlay (HUD) on top of
     * the 3D scene. Called by the engine during the UI pass, inside an active
     * UIRenderer frame (so UIRenderer::DrawRect/DrawText work directly). Default does nothing.
     * @param screenW Overlay width in pixels.
     * @param screenH Overlay height in pixels.
     */
    virtual void OnRenderOverlay(f32 screenW, f32 screenH) { (void)screenW; (void)screenH; }

    /**
     * @brief Updates all systems in the scene.
     * @param deltaTime Time since last frame.
     */
    void Update(f32 deltaTime);

    /// @brief Returns the scene name.
    const char* GetName() const { return m_name; }

    /// @brief Returns the entity manager.
    EntityManager& GetEntityManager() { return m_entityManager; }
    /// @brief Returns the component registry.
    ComponentRegistry& GetComponentRegistry() { return m_componentRegistry; }
    /// @brief Returns the system manager.
    SystemManager& GetSystemManager() { return m_systemManager; }

    /// @brief Returns the number of alive entities.
    u32 GetEntityCount() const { return m_entityManager.GetAliveCount(); }

protected:
    const char* m_name; ///< Scene name
    EntityManager m_entityManager; ///< Entity manager for this scene
    ComponentRegistry m_componentRegistry; ///< Component registry for this scene
    SystemManager m_systemManager; ///< System manager for this scene
    b8 m_isLoaded; ///< Whether the scene has been loaded
};

} // namespace Dnd
