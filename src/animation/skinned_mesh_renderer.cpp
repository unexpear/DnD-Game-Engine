/**
 * @file skinned_mesh_renderer.cpp
 * @brief Implementation of SkinnedMeshRenderer component methods.
 *
 * See skinned_mesh_renderer.h for full documentation.
 *
 * @author DND Engine Team
 * @date 2026-05-08
 * @version 0.5.0
 */

#include "animation/skinned_mesh_renderer.h"
#include "rendering/shader.h"
#include <cstdio>

namespace Dnd {

void SkinnedMeshRenderer::UploadToShader(Shader* shader) const {
    if (!shader || !shader->IsCompiled() || boneCount == 0) {
        return;
    }

    shader->Bind();

    // Upload each skinning matrix as an indexed uniform.
    // The shader must declare: uniform mat4 u_bones[128];
    for (u32 i = 0; i < boneCount && i < MAX_BONES; i++) {
        char name[32];
        std::snprintf(name, sizeof(name), "u_bones[%u]", i);
        shader->SetUniform(name, skinningMatrices[i]);
    }
}

} // namespace Dnd
