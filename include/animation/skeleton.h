/**
 * @file skeleton.h
 * @brief Skeleton component for skeletal animation.
 * 
 * The Skeleton component stores the bone hierarchy and inverse bind pose
 * matrices needed for skinning. Each bone has:
 *   - Parent bone index (or -1 for root)
 *   - Inverse bind pose matrix (transforms from model space to bone space)
 *   - Local bind transform (position, rotation, scale in parent space)
 * 
 * The skeleton is typically loaded from a 3D model file (glTF, FBX)
 * or created programmatically for procedural animation.
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.5.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include <vector>
#include <string>

namespace Dnd {

// ==========================================
// STRUCT: Bone
// ==========================================

/**
 * @struct Bone
 * @brief A single bone in a skeleton hierarchy.
 * 
 * Bones form a tree structure via parent indices.
 * Each bone has a local bind transform (relative to parent)
 * and an inverse bind pose matrix (for shader skinning).
 */
struct Bone {
    std::string name;      ///< Bone name (for debugging/lookup)
    i32 parentIndex;        ///< Parent bone index (-1 for root)
    Vec3 localPosition;     ///< Bind position in parent space
    Quaternion localRotation;///< Bind rotation in parent space
    Vec3 localScale;        ///< Bind scale in parent space
    Mat4 inverseBindPose;   ///< Inverse of the bind pose matrix (model -> bone space)

    /// @brief Default constructor (root bone at origin).
    Bone()
        : name("root"), parentIndex(-1),
          localPosition(0, 0, 0), localRotation(Quaternion::Identity()),
          localScale(1, 1, 1), inverseBindPose(Mat4::Identity()) {}

    /**
     * @brief Constructs a bone with a name and parent.
     * @param boneName Bone name.
     * @param parent Parent index (-1 for root).
     */
    explicit Bone(const std::string& boneName, i32 parent = -1)
        : name(boneName), parentIndex(parent),
          localPosition(0, 0, 0), localRotation(Quaternion::Identity()),
          localScale(1, 1, 1), inverseBindPose(Mat4::Identity()) {}
};

// ==========================================
// STRUCT: Skeleton
// ==========================================

/**
 * @struct Skeleton
 * @brief ECS component: skeleton data for an entity.
 * 
 * This component is added to entities that have animated meshes.
 * The skeleton defines the bone hierarchy and is used by the
 * AnimationSystem to compute final bone transforms.
 * 
 * Usage:
 * @code
 * Entity entity = entityManager.Create();
 * Skeleton* sk = componentRegistry.AddComponent<Skeleton>(entity);
 * sk->AddBone("root", -1);
 * sk->AddBone("spine", 0);
 * sk->CalculateInverseBindPoses();
 * @endcode
 */
struct Skeleton {
    std::vector<Bone> bones;                    ///< Bone array (index = bone ID)
    std::vector<Mat4> localTransforms;          ///< Cached local transforms per bone
    std::vector<Mat4> globalTransforms;         ///< Cached global transforms per bone
    std::vector<Mat4> skinningMatrices;         ///< Final matrices for shader (global * invBindPose)

    /// @brief Default constructor.
    Skeleton() = default;

    /// @brief Gets the number of bones.
    u32 GetBoneCount() const { return static_cast<u32>(bones.size()); }

    /**
     * @brief Adds a bone to the skeleton.
     * @param name Bone name.
     * @param parentIndex Parent bone index (-1 for root).
     * @return Bone index, or -1 on failure.
     */
    i32 AddBone(const std::string& name, i32 parentIndex = -1);

    /**
     * @brief Gets a bone by index.
     * @param index Bone index.
     * @return Pointer to bone, or nullptr if invalid.
     */
    Bone* GetBone(u32 index);
    const Bone* GetBone(u32 index) const;

    /**
     * @brief Finds a bone by name.
     * @param name Bone name to find.
     * @return Bone index, or -1 if not found.
     */
    i32 FindBone(const std::string& name) const;

    /**
     * @brief Calculates inverse bind pose matrices for all bones.
     * 
     * Computes the global bind pose matrix for each bone, then
     * stores its inverse. This must be called after setting up
     * the bone hierarchy and local bind transforms.
     */
    void CalculateInverseBindPoses();

    /**
     * @brief Updates global transforms from local transforms.
     * 
     * Called each frame to compute global bone transforms
     * from local (parent-relative) transforms.
     * 
     * @param parentGlobal Optional parent global transform (for nested skeletons).
     */
    void UpdateGlobalTransforms(const Mat4& parentGlobal = Mat4::Identity());

    /**
     * @brief Gets the skinning matrix for a bone.
     * @param boneIndex Bone index.
     * @return Skinning matrix (global * inverseBindPose).
     */
    const Mat4& GetSkinningMatrix(u32 boneIndex) const {
        return skinningMatrices[boneIndex];
    }
};

} // namespace Dnd
