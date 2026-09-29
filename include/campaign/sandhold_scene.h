/**
 * @file sandhold_scene.h
 * @brief Playable courtyard slice for the Sandhold pack.
 *
 * A sandy hold on the table: dunes, a tent, a crate, a torch, and a
 * campfire. One warden walks while this pack's control scheme is
 * installed. F at the fire rests (+2). F at the north gate braces it
 * with DiceRoller. A successful brace lifts the dust bar. F3 (engine)
 * switches the tabletop orbit and a camera behind the warden. Unloading
 * the pack restores base D&D controls via Engine::ResetViewToBase.
 *
 * This is a fixed courtyard, not a port of the Sandhold repository.
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
#include <vector>

namespace Dnd {

/**
 * @class SandholdScene
 * @brief One warden, one camp, one brace.
 */
class SandholdScene : public Scene {
public:
    explicit SandholdScene(const CampaignManifest& manifest);

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
    void RestAtFire();
    void BraceGate();
    void OpenGate();

    CampaignManifest m_manifest;
    std::vector<Block> m_blocks;
    Entity m_warden;
    Entity m_fire;
    i32 m_gateIndex;
    f32 m_yaw;
    b8 m_rested;
    b8 m_held;
    b8 m_hasRoll;
    b8 m_lastSuccess;
    i32 m_lastDie;
    i32 m_lastTotal;
    i32 m_lastBonus;
    static constexpr i32 kBraceDC = 12;
    static constexpr i32 kRestBonus = 2;
};

} // namespace Dnd
