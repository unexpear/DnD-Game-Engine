/**
 * @file campaign_table_scene.h
 * @brief Placeholder table scene for a non-D&D campaign pack.
 *
 * Same engine loop as the demo: a camera, a table, mesh tokens, orbit
 * focus, and an overlay. Content comes only from the pack manifest
 * (title, summary, tokens). Meshes are procedural. The scene does not
 * read the global Assets/ tree and does not execute entryScript.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#pragma once

#include "campaign/campaign_manifest.h"
#include "scene/scene.h"

namespace Dnd {

/**
 * @class CampaignTableScene
 * @brief A campaign's pieces on the shared 3D table.
 */
class CampaignTableScene : public Scene {
public:
    /**
     * @brief Stores a copy of @p manifest. The scene does not keep a
     * pointer into the registry.
     */
    explicit CampaignTableScene(const CampaignManifest& manifest);

protected:
    void OnLoad() override;
    void OnUnload() override;
    void OnUpdate(f32 deltaTime) override;
    void OnRenderOverlay(f32 screenW, f32 screenH) override;

private:
    void ApplyTableLight();

    CampaignManifest m_manifest;
};

} // namespace Dnd
