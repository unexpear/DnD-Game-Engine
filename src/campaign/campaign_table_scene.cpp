/**
 * @file campaign_table_scene.cpp
 * @brief Builds a placeholder table from a campaign manifest.
 *
 * See campaign_table_scene.h. Shared procedural meshes are created once
 * and reused so switching packs does not allocate a new cube every time.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/campaign_table_scene.h"
#include "core/engine.h"
#include "core/log.h"
#include "ecs/components/camera.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/tag.h"
#include "ecs/components/transform.h"
#include "ecs/systems/render_system.h"
#include "ecs/systems/transform_system.h"
#include "rendering/mesh.h"
#include "rendering/renderer.h"
#include "ui/ui_renderer.h"

#include <cstdio>
#include <string>

namespace Dnd {

namespace {

Mesh* SharedCube() {
    static Mesh* cube = nullptr;
    if (cube == nullptr) {
        cube = new Mesh(Mesh::CreateCube());
    }
    return cube;
}

Mesh* SharedTable() {
    static Mesh* table = nullptr;
    if (table == nullptr) {
        table = new Mesh(Mesh::CreateQuad(12.0f, 8.0f));
    }
    return table;
}

} // namespace

CampaignTableScene::CampaignTableScene(const CampaignManifest& manifest)
    : Scene("CampaignTable")
    , m_manifest(manifest) {
}

void CampaignTableScene::ApplyTableLight() {
    Spotlight key;
    key.position = Vec3(0.0f, 11.0f, 5.0f);
    key.direction = Vec3(0.0f, -1.0f, -0.35f).Normalized();
    key.color = Vec3(1.05f, 0.98f, 0.88f);
    key.cosInner = 0.82f;
    key.cosOuter = 0.45f;
    key.range = 36.0f;
    Renderer::SetSpotlights(&key, 1);
    Renderer::SetOccluders(nullptr, nullptr, 0);
}

void CampaignTableScene::OnLoad() {
    Scene::OnLoad();

    m_systemManager.RegisterSystem(
        std::make_unique<TransformSystem>(),
        m_entityManager, m_componentRegistry);
    m_systemManager.RegisterSystem(
        std::make_unique<RenderSystem>(nullptr),
        m_entityManager, m_componentRegistry);

    const Vec3 eye(0.0f, 8.0f, 10.0f);
    const Vec3 focus(0.0f, 0.0f, 0.0f);
    Entity camera = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(camera, Tag("MainCamera"));
    m_componentRegistry.AddComponent<Transform>(camera, Transform(
        eye, Quat::LookAt(eye, focus, Vec3(0.0f, 1.0f, 0.0f)), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<ECSCamera>(camera, ECSCamera());

    Entity table = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(table, Tag("Table"));
    m_componentRegistry.AddComponent<Transform>(table, Transform(
        Vec3(0, 0, 0), Quat::Identity(), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<MeshRenderer>(table, MeshRenderer(
        SharedTable(), Color(0.34f, 0.24f, 0.14f, 1.0f)));

    if (m_manifest.tableTokens.empty()) {
        TableToken marker;
        marker.name = m_manifest.title.empty() ? m_manifest.id : m_manifest.title;
        marker.r = 0.75f;
        marker.g = 0.62f;
        marker.b = 0.30f;
        marker.x = 0.0f;
        marker.y = 0.45f;
        marker.z = 0.0f;
        m_manifest.tableTokens.push_back(marker);
    }

    for (const TableToken& token : m_manifest.tableTokens) {
        Entity piece = m_entityManager.Create();
        m_componentRegistry.AddComponent<Tag>(piece, Tag(token.name.c_str()));
        m_componentRegistry.AddComponent<Transform>(piece, Transform(
            Vec3(token.x, token.y, token.z), Quat::Identity(), Vec3(0.55f, 0.9f, 0.55f)));
        m_componentRegistry.AddComponent<MeshRenderer>(piece, MeshRenderer(
            SharedCube(), Color(token.r, token.g, token.b, 1.0f)));
    }

    ApplyTableLight();
    Engine::SetCameraFocus(Vec3(0.0f, 0.4f, 0.2f));
    DND_LOG_INFO("Campaign table loaded: %s (%u tokens)",
                 m_manifest.id.c_str(),
                 static_cast<u32>(m_manifest.tableTokens.size()));
}

void CampaignTableScene::OnUnload() {
    Scene::OnUnload();
}

void CampaignTableScene::OnUpdate(f32 deltaTime) {
    (void)deltaTime;
    ApplyTableLight();
    Engine::SetCameraFocus(Vec3(0.0f, 0.4f, 0.2f));
}

void CampaignTableScene::OnRenderOverlay(f32 screenW, f32 screenH) {
    (void)screenH;
    const f32 panelW = 460.0f;
    const f32 panelH = 92.0f;
    UIRenderer::DrawRect(16.0f, 16.0f, panelW, panelH, Vec4(0.08f, 0.07f, 0.06f, 0.82f));

    char title[160];
    std::snprintf(title, sizeof(title), "%s", m_manifest.title.c_str());
    UIRenderer::DrawText(28.0f, 24.0f, title, Vec4(0.96f, 0.90f, 0.72f, 1.0f), 14.0f);

    char meta[192];
    std::snprintf(meta, sizeof(meta), "%s   %s   %s",
                  m_manifest.id.c_str(),
                  m_manifest.ruleset.c_str(),
                  m_manifest.entryScene.c_str());
    UIRenderer::DrawText(28.0f, 46.0f, meta, Vec4(0.75f, 0.78f, 0.72f, 1.0f), 9.0f);

    const char* summary = m_manifest.summary.empty()
        ? "Placeholder pack on the shared table."
        : m_manifest.summary.c_str();
    UIRenderer::DrawText(28.0f, 66.0f, summary, Vec4(0.90f, 0.90f, 0.86f, 1.0f), 8.0f);

    std::string names;
    for (size_t i = 0; i < m_manifest.tableTokens.size(); ++i) {
        if (i != 0) {
            names += "  ";
        }
        names += m_manifest.tableTokens[i].name;
    }
    UIRenderer::DrawText(28.0f, 84.0f, names.c_str(), Vec4(0.80f, 0.84f, 0.78f, 1.0f), 8.0f);
    (void)screenW;
}

} // namespace Dnd
