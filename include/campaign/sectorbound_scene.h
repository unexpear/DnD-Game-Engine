/**
 * @file sectorbound_scene.h
 * @brief Playable outpost slice for the Sectorbound pack.
 *
 * A small block yard on the table: floor grid, walls, crates, a beacon,
 * and a gate. One scout walks while this pack's control scheme is
 * installed. F stows a survey marker or scans the beacon with DiceRoller.
 * A successful scan lifts the gate. F3 (engine) switches the tabletop
 * orbit and a camera behind the scout. Unloading the pack restores base
 * D&D controls via Engine::ResetViewToBase.
 *
 * The blocks are a fixed cube yard, not a voxel engine and not a port of
 * the Sectorbound repository.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#pragma once

#include "campaign/campaign_manifest.h"
#include "core/math.h"
#include "scene/scene.h"
#include "ecs/entity.h"
#include <string>
#include <vector>

namespace Dnd {

/**
 * @class SectorboundScene
 * @brief One scout, one sector claim.
 */
class SectorboundScene : public Scene {
public:
    explicit SectorboundScene(const CampaignManifest& manifest);

protected:
    void OnLoad() override;
    void OnUnload() override;
    void OnUpdate(f32 deltaTime) override;
    void OnRenderOverlay(f32 screenW, f32 screenH) override;

private:
    struct Block {
        Entity entity;
        Vec3 center;
        Vec3 half;
        b8 solid;
    };

    Entity AddProp(const Vec3& center, const Vec3& scale, const Color& color, const char* name, b8 solid);
    b8 HitsSolid(const Vec3& feet) const;
    void ScanBeacon();
    void StowMarker();
    void OpenGate();

    CampaignManifest m_manifest;
    std::vector<Block> m_blocks;
    Entity m_scout;
    Entity m_marker;
    Entity m_lamp;
    i32 m_gateIndex;
    f32 m_yaw;
    b8 m_markerStowed;
    b8 m_claimed;
    b8 m_hasScan;
    b8 m_lastSuccess;
    i32 m_lastDie;
    i32 m_lastTotal;
    i32 m_lastBonus;
    static constexpr i32 kScanDC = 12;
    static constexpr i32 kMarkerBonus = 2;
};

} // namespace Dnd
