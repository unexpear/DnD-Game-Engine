/**
 * @file animation_system.cpp
 * @brief Implementation of AnimationSystem.
 * 
 * See animation_system.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-05-08
 * @version 0.5.0
 */

#include "animation/animation_system.h"
#include "animation/animation.h"
#include "animation/skeleton.h"
#include "animation/animation_clip.h"
#include "animation/skinned_mesh_renderer.h"
#include "core/log.h"

namespace Dnd {

AnimationSystem::AnimationSystem()
    : System("AnimationSystem", SystemPhase::UPDATE, 4) {
}

void AnimationSystem::OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) {
    DND_UNUSED(em);

    auto* animSet = cr.GetSparseSet<Animation>();
    auto* skeletonSet = cr.GetSparseSet<Skeleton>();
    auto* skinSet = cr.GetSparseSet<SkinnedMeshRenderer>();

    if (!animSet || !skeletonSet || animSet->Empty()) {
        return;
    }

    // Iterate over all animations
    for (u32 i = 0; i < animSet->Size(); i++) {
        Animation& anim = const_cast<Animation&>(animSet->GetDense()[i]);
        u32 entityIndex = animSet->GetEntityIndex(i);

        // Check if entity also has a skeleton
        Skeleton* skeleton = skeletonSet->Get(entityIndex);
        if (!skeleton) {
            continue;
        }

        // Update this animation (advance time, sample, update skeleton transforms)
        UpdateAnimation(anim, *skeleton, deltaTime);

        // Upload skinning matrices to SkinnedMeshRenderer if present
        if (skinSet) {
            SkinnedMeshRenderer* skin = skinSet->Get(entityIndex);
            if (skin) {
                u32 boneCount = skeleton->GetBoneCount();
                if (boneCount > SkinnedMeshRenderer::MAX_BONES) {
                    boneCount = SkinnedMeshRenderer::MAX_BONES;
                }
                skin->boneCount = boneCount;
                for (u32 b = 0; b < boneCount; b++) {
                    skin->skinningMatrices[b] = skeleton->GetSkinningMatrix(b);
                }
            }
        }
    }
}

void AnimationSystem::UpdateAnimation(Animation& anim, Skeleton& skeleton, f32 deltaTime) {
    if (!anim.clip || !anim.isPlaying) {
        return;
    }

    // Advance animation time
    anim.Update(deltaTime);

    // Sample animation and update skeleton transforms
    SampleAndUpdateTransforms(anim, skeleton);
}

void AnimationSystem::SampleAndUpdateTransforms(Animation& anim, Skeleton& skeleton) {
    if (!anim.clip) return;

    // Get bone count
    u32 boneCount = skeleton.GetBoneCount();
    if (boneCount == 0) return;

    // Temporary array for sampled local transforms
    std::vector<Mat4> localTransforms(boneCount);

    // Sample the animation clip to get local transforms
    anim.clip->Sample(anim.time, anim.loop, localTransforms.data());

    // Copy sampled transforms to skeleton
    for (u32 i = 0; i < boneCount; i++) {
        skeleton.localTransforms[i] = localTransforms[i];
    }

    // Update global transforms from local
    skeleton.UpdateGlobalTransforms();
}

} // namespace Dnd
