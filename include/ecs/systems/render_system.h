/**
 * @file render_system.h
 * @brief System for rendering entities with MeshRenderer components.
 * 
 * Collects all visible entities with Transform + MeshRenderer components
 * and submits them to the Renderer for drawing.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "ecs/system.h"
#include "ecs/components/transform.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/camera.h"

namespace Dnd {

// Forward declarations
class DebugDraw;

/**
 * @class RenderSystem
 * @brief Renders all visible entities with Transform + MeshRenderer components.
 * 
 * This system runs during the RENDER phase. It:
 *   1. Finds the active camera
 *   2. Computes view matrix from camera's transform
 *   3. Iterates all entities with Transform + MeshRenderer
 *   4. Submits visible entities to the renderer
 *   5. Draws debug geometry
 */
class RenderSystem : public System {
public:
    /**
     * @brief Constructs a RenderSystem.
     * @param debugDraw Pointer to the debug drawing system (can be nullptr).
     */
    explicit RenderSystem(DebugDraw* debugDraw = nullptr);

protected:
    void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) override;

private:
    DebugDraw* m_debugDraw; ///< Pointer to debug drawing system (not owned)
};

} // namespace Dnd
