/**
 * @file animation_demo_scene.h
 * @brief Demo scene showcasing skeletal animation with an ECS armature.
 * 
 * This scene demonstrates the animation pipeline by creating a simple
 * 3-bone skeleton (root -> upper -> lower) with a looping animation clip
 * that applies rotational keyframes to each bone.
 * 
 * The skeleton is rendered as three colored boxes, one per bone,
 * positioned and rotated according to the skeleton's global transforms.
 * 
 * Systems registered:
 *   - TransformSystem (UPDATE)
 *   - AnimationSystem (UPDATE) — advances animation, updates skeleton
 *   - BoneSyncSystem (UPDATE) — syncs skeleton transforms to bone visuals
 *   - RenderSystem (RENDER) — draws bone visual entities
 * 
 * @author DND Engine Team
 * @date 2026-05-08
 * @version 0.5.0
 */

#pragma once

#include "scene/scene.h"
#include "ecs/entity.h"

namespace Dnd {

// Forward declaration
class AnimationClip;

/**
 * @class AnimationDemoScene
 * @brief A demo scene that creates and animates a bone hierarchy.
 * 
 * Creates:
 *   - A skeleton entity with Animation + Skeleton components
 *   - Three bone visual entities (red/green/blue colored boxes)
 *   - A camera entity positioned to view the animated armature
 *   - A looping AnimationClip with rotation keyframes per bone
 * 
 * The BoneSyncSystem (defined in the .cpp) reads the skeleton's global
 * transforms each frame and updates the bone visual entities' Transform
 * components, so the colored boxes follow the bone hierarchy animation.
 */
class AnimationDemoScene : public Scene {
public:
    AnimationDemoScene();

protected:
    void OnLoad() override;
    void OnUnload() override;

private:
    Entity m_skeletonEntity;      ///< Entity with Animation + Skeleton
    Entity m_boneEntities[3];     ///< Visual cube entities for each bone
    Entity m_cameraEntity;        ///< Active camera entity
    AnimationClip* m_clip;        ///< The animation clip (owned by scene)
};

} // namespace Dnd
