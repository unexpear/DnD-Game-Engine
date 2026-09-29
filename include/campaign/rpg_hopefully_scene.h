/**
 * @file rpg_hopefully_scene.h
 * @brief Playable dice-and-class slice for the RPG Hopefully pack.
 *
 * A practice yard on the table: four class figures, one active. WASD walks
 * the active figure while this pack's control scheme is installed. R rolls
 * a d20 plus that class's modifier against a practice DC, using DiceRoller.
 * F3 (engine) switches the tabletop orbit and a camera behind the figure.
 * Unloading the pack restores base D&D controls via Engine::ResetViewToBase.
 *
 * This is a faithful slice of a class-and-dice cousin, not a port of the
 * hopeful repository.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#pragma once

#include "campaign/campaign_manifest.h"
#include "scene/scene.h"
#include "ecs/entity.h"
#include <string>
#include <vector>

namespace Dnd {

/**
 * @class RpgHopefullyScene
 * @brief One class on the yard, one check at a time.
 */
class RpgHopefullyScene : public Scene {
public:
    explicit RpgHopefullyScene(const CampaignManifest& manifest);

protected:
    void OnLoad() override;
    void OnUnload() override;
    void OnUpdate(f32 deltaTime) override;
    void OnRenderOverlay(f32 screenW, f32 screenH) override;

private:
    struct Figure {
        std::string name;
        std::string ability;
        i32 modifier;
        f32 r, g, b;
        Entity entity;
        f32 yaw;
    };

    void RollPractice();
    void SelectFigure(i32 index);

    CampaignManifest m_manifest;
    std::vector<Figure> m_figures;
    i32 m_active;
    i32 m_lastDie;
    i32 m_lastTotal;
    b8 m_hasRoll;
    b8 m_lastSuccess;
    static constexpr i32 kPracticeDC = 12;
};

} // namespace Dnd
