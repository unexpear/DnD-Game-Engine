/**
 * @file shadow_test_scene.h
 * @brief Minimal scene for validating ray-cast spotlight shadows.
 *
 * One large cube on a floor, lit by a single angled spotlight that slowly
 * orbits the cube (aimed at the top edge of whichever side faces it). The
 * cast shadow should sweep across the floor as the light orbits — an easy
 * visual check that shadows track the light correctly.
 *
 * @author DND Engine Team
 * @date 2026-06-03
 */

#pragma once

#include "scene/scene.h"
#include "core/math.h"
#include "rendering/mesh.h"
#include "ecs/entity.h"

namespace Dnd {

class ShadowTestScene : public Scene {
public:
    ShadowTestScene();

protected:
    void OnLoad() override;
    void OnUnload() override;
    void OnUpdate(f32 deltaTime) override; // orbits the spotlight

private:
    Mesh* m_cubeMesh;
    Mesh* m_quadMesh;
    Entity m_cube;
    Entity m_camera;
    f32 m_lightAngle; ///< Accumulated orbit angle of the spotlight (radians).
};

} // namespace Dnd
