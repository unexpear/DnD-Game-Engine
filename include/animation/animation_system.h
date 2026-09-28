/**
 * @file animation_system.h
 * @brief System for updating skeletal animations.
 * 
 * The AnimationSystem processes entities with Animation, Skeleton,
 * and (optionally) SkinnedMeshRenderer components.
 * 
 * Per frame:
 *   1. For each entity with Animation + Skeleton:
 *      - Advance animation time
 *      - Sample animation clip to get local bone transforms
 *      - Update skeleton global transforms
 *      - Compute skinning matrices
 *   2. If entity has SkinnedMeshRenderer:
 *      - Upload skinning matrices to GPU
 * 
 * Runs during the UPDATE phase (once per frame, not fixed).
 * 
 * @author DND Engine Team
 * @date 2026-05-08
 * @version 0.5.0
 */

#pragma once

#include "ecs/system.h"
#include "ecs/components/transform.h"

namespace Dnd {

// Forward declarations
struct Animation;
struct Skeleton;
struct SkinnedMeshRenderer;

// ==========================================
// CLASS: AnimationSystem
// ==========================================

/**
 * @class AnimationSystem
 * @brief Updates skeletal animation playback and bone transforms.
 * 
 * Runs during the UPDATE phase. Processes entities that have
 * both Animation and Skeleton components.
 * 
 * Per-frame update:
 *   1. Advance animation time based on delta time and playback speed
 *   2. Sample animation clip to get local bone transforms
 *   3. Update skeleton global transforms (parent-relative to world)
 *   4. Compute final skinning matrices (global * inverse bind pose)
 *   5. Upload skinning matrices to SkinnedMeshRenderer if present
 */
class AnimationSystem : public System {
public:
    /**
     * @brief Constructs the AnimationSystem.
     */
    AnimationSystem();

protected:
    /**
     * @brief Updates all animations.
     * @param deltaTime Frame delta time in seconds.
     * @param em Entity manager.
     * @param cr Component registry.
     */
    void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) override;

private:
    /**
     * @brief Updates a single animation.
     * @param anim Animation component.
     * @param skeleton Skeleton component.
     * @param deltaTime Frame delta time.
     */
    void UpdateAnimation(Animation& anim, Skeleton& skeleton, f32 deltaTime);

    /**
     * @brief Samples the animation and updates skeleton transforms.
     * @param anim Animation component.
     * @param skeleton Skeleton component.
     */
    void SampleAndUpdateTransforms(Animation& anim, Skeleton& skeleton);
};

} // namespace Dnd
