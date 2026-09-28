/**
 * @file render_system.cpp
 * @brief Implementation of the RenderSystem class.
 * 
 * See render_system.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.3.0
 */

#include "ecs/systems/render_system.h"
#include "rendering/renderer.h"
#include "rendering/debug_draw.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/shader.h"
#include "assets/asset_manager.h"

namespace Dnd {

// External shader (created by DemoScene or engine)
extern Shader* g_sceneShader;

RenderSystem::RenderSystem(DebugDraw* debugDraw)
    : System("RenderSystem", SystemPhase::RENDER, 100), m_debugDraw(debugDraw) {
}

void RenderSystem::OnUpdate(f32 /*deltaTime*/, EntityManager& em, ComponentRegistry& cr) {
    DND_UNUSED(em);

    // Step 1: Find the active camera
    const ECSCamera* camera = nullptr;
    const Transform* cameraTransform = nullptr;

    auto* cameraSet = cr.GetSparseSet<ECSCamera>();
    auto* transformSet = cr.GetSparseSet<Transform>();

    if (cameraSet && transformSet && !cameraSet->Empty()) {
        for (u32 i = 0; i < cameraSet->Size(); i++) {
            const ECSCamera& cam = cameraSet->GetDense()[i];
            if (cam.isActive) {
                u32 entityIndex = cameraSet->GetEntityIndex(i);
                camera = &cam;
                cameraTransform = transformSet->Get(entityIndex);
                break;
            }
        }
    }

    // Step 2: Compute view and projection matrices
    Mat4 viewMatrix = Mat4::Identity();
    if (cameraTransform) {
        Mat4 worldMatrix = Mat4::TRS(
            cameraTransform->position,
            cameraTransform->rotation,
            cameraTransform->scale
        );
        viewMatrix = worldMatrix.Inverse();
    }

    Mat4 projMatrix = camera ? camera->GetProjectionMatrix() : Mat4::Identity();

    // Step 3: Set camera matrices on renderer
    Renderer::SetCameraMatrices(viewMatrix, projMatrix);

    // Step 4: Render all visible entities with Transform + MeshRenderer
    auto* meshRendererSet = cr.GetSparseSet<MeshRenderer>();
    if (meshRendererSet && !meshRendererSet->Empty()) {
        for (u32 i = 0; i < meshRendererSet->Size(); i++) {
            const MeshRenderer& mr = meshRendererSet->GetDense()[i];
            if (!mr.visible || mr.mesh == nullptr) {
                continue;
            }

            u32 entityIndex = meshRendererSet->GetEntityIndex(i);
            const Transform* transform = transformSet ? transformSet->Get(entityIndex) : nullptr;

            Mat4 modelMatrix = transform
                ? Mat4::TRS(transform->position, transform->rotation, transform->scale)
                : Mat4::Identity();

            Texture2D* tex = mr.diffuseTexture.IsValid()
                ? AssetManager::Get().GetTexture(mr.diffuseTexture)
                : nullptr;
            Renderer::DrawMesh(mr.mesh, modelMatrix, mr.color, tex);
        }
    }

    // Step 4b: Translucent spotlight beam shafts (additive pass, after opaque).
    Renderer::DrawSpotlightBeams();

    // Step 5: Draw debug geometry
    if (m_debugDraw) {
        m_debugDraw->Flush(viewMatrix, projMatrix);
    }
}

} // namespace Dnd
