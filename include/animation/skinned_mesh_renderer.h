/**
 * @file skinned_mesh_renderer.h
 * @brief SkinnedMeshRenderer component for skeletal animation rendering.
 *
 * Extends the MeshRenderer concept with bone skinning matrix data.
 * Stores up to 128 skinning matrices (global * inverse bind pose) for
 * vertex skinning on the GPU. The AnimationSystem fills these matrices
 * each frame after updating the skeleton.
 *
 * Usage:
 * @code
 * Entity entity = entityManager.Create();
 * SkinnedMeshRenderer* skin = componentRegistry.AddComponent<SkinnedMeshRenderer>(entity);
 * skin->mesh = skinnedMesh;
 * skin->boneCount = skeleton.GetBoneCount();
 * // AnimationSystem fills skinningMatrices each frame
 * @endcode
 *
 * @see MeshRenderer
 * @see AnimationSystem
 * @see Skeleton
 *
 * @author DND Engine Team
 * @date 2026-05-08
 * @version 0.5.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"

namespace Dnd {

// Forward declarations
class Mesh;
class Shader;

// ==========================================
// STRUCT: SkinnedMeshRenderer
// ==========================================

/**
 * @struct SkinnedMeshRenderer
 * @brief ECS component: renderable skinned mesh with bone skinning matrices.
 *
 * Like MeshRenderer, this component marks an entity as renderable with a
 * specific mesh, but additionally stores an array of skinning matrices
 * (one per bone) for GPU vertex skinning.
 *
 * The skinning matrices are computed by the AnimationSystem from the
 * Skeleton component's global transforms and inverse bind poses. They
 * are uploaded to the GPU shader via UploadToShader().
 *
 * Maximum bones: MAX_BONES (128). This is compatible with most hardware
 * and GLSL array limits.
 */
struct SkinnedMeshRenderer {
    /** @brief Maximum number of bones supported. */
    static constexpr u32 MAX_BONES = 128;

    const Mesh* mesh;                            ///< Pointer to the skinned mesh (not owned)
    Color color;                                 ///< Tint color for rendering
    b8 visible;                                  ///< Whether the entity should be rendered
    u32 boneCount;                               ///< Number of active bones (<= MAX_BONES)
    Mat4 skinningMatrices[MAX_BONES];            ///< Skinning matrices for GPU vertex skinning

    /// @brief Default constructor (no mesh, white, visible, zero bones).
    SkinnedMeshRenderer()
        : mesh(nullptr), color(1, 1, 1, 1), visible(true), boneCount(0) {
        for (u32 i = 0; i < MAX_BONES; i++) {
            skinningMatrices[i] = Mat4::Identity();
        }
    }

    /**
     * @brief Constructs a SkinnedMeshRenderer with a mesh.
     * @param meshPtr Pointer to the skinned mesh (must outlive this component).
     * @param tint Tint color (default white).
     */
    explicit SkinnedMeshRenderer(const Mesh* meshPtr, const Color& tint = Color(1, 1, 1, 1))
        : mesh(meshPtr), color(tint), visible(true), boneCount(0) {
        for (u32 i = 0; i < MAX_BONES; i++) {
            skinningMatrices[i] = Mat4::Identity();
        }
    }

    /**
     * @brief Uploads the skinning matrices to a shader uniform array.
     *
     * Sets the uniform array `u_bones` (or equivalent) on the given shader
     * with the current skinning matrices. Only `boneCount` matrices are
     * uploaded (the rest are ignored).
     *
     * The shader must declare:
     * @code
     * uniform mat4 u_bones[128];
     * @endcode
     *
     * @param shader Pointer to the shader program. Must be compiled.
     *              If nullptr or not compiled, the call is a no-op.
     *
     * @note The shader should be bound before calling this method, or
     *       this method will bind it.
     */
    void UploadToShader(Shader* shader) const;
};

} // namespace Dnd
