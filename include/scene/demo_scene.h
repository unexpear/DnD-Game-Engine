/**
 * @file demo_scene.h
 * @brief Demo scene using ECS — rotating cube, floating triangle, table quad.
 * 
 * This scene demonstrates the ECS by creating entities with Transform,
 * MeshRenderer, and ECSCamera components instead of hardcoded rendering.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#pragma once

#include "scene/scene.h"

namespace Dnd {

/**
 * @class DemoScene
 * @brief A demo scene showcasing ECS entities.
 * 
 * Creates:
 *   - A camera entity for view/projection
 *   - A table (green quad at Y=0)
 *   - A rotating blue cube
 *   - A floating colorful triangle
 *   - Debug axis gizmos
 */
class DemoScene : public Scene {
public:
    DemoScene();

protected:
    void OnLoad() override;
    void OnUnload() override;

private:
    // Mesh IDs (created during OnLoad)
    u32 m_triangleMeshId;
    u32 m_cubeMeshId;
    u32 m_tableMeshId;

    // Entity handles
    Entity m_cameraEntity;
    Entity m_tableEntity;
    Entity m_cubeEntity;
    Entity m_triangleEntity;
};

} // namespace Dnd
