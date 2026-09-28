/**
 * @file skeleton.cpp
 * @brief Implementation of Skeleton component.
 * 
 * See skeleton.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.5.0
 */

#include "animation/skeleton.h"

namespace Dnd {

i32 Skeleton::AddBone(const std::string& name, i32 parentIndex) {
    // Validate parent index
    if (parentIndex >= static_cast<i32>(bones.size())) {
        return -1; // Invalid parent
    }

    Bone bone(name, parentIndex);
    bones.push_back(bone);
    localTransforms.push_back(Mat4::Identity());
    globalTransforms.push_back(Mat4::Identity());
    skinningMatrices.push_back(Mat4::Identity());

    return static_cast<i32>(bones.size()) - 1;
}

Bone* Skeleton::GetBone(u32 index) {
    if (index >= bones.size()) return nullptr;
    return &bones[index];
}

const Bone* Skeleton::GetBone(u32 index) const {
    if (index >= bones.size()) return nullptr;
    return &bones[index];
}

i32 Skeleton::FindBone(const std::string& name) const {
    for (u32 i = 0; i < bones.size(); i++) {
        if (bones[i].name == name) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

void Skeleton::CalculateInverseBindPoses() {
    // First compute global bind poses
    for (u32 i = 0; i < bones.size(); i++) {
        const Bone& bone = bones[i];
        Mat4 local = Mat4::TRS(bone.localPosition, bone.localRotation, bone.localScale);

        if (bone.parentIndex < 0) {
            globalTransforms[i] = local;
        } else {
            globalTransforms[i] = globalTransforms[bone.parentIndex] * local;
        }

        // Inverse bind pose
        bones[i].inverseBindPose = globalTransforms[i].Inverse();
    }
}

void Skeleton::UpdateGlobalTransforms(const Mat4& parentGlobal) {
    for (u32 i = 0; i < bones.size(); i++) {
        const Bone& bone = bones[i];
        Mat4 local = localTransforms[i];

        if (bone.parentIndex < 0) {
            globalTransforms[i] = parentGlobal * local;
        } else {
            globalTransforms[i] = globalTransforms[bone.parentIndex] * local;
        }

        // Compute skinning matrix: global * inverseBindPose
        skinningMatrices[i] = globalTransforms[i] * bones[i].inverseBindPose;
    }
}

} // namespace Dnd
