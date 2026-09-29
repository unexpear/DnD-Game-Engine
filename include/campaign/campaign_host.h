/**
 * @file campaign_host.h
 * @brief Loads the active campaign into the scene manager.
 *
 * The host is the only campaign code the engine loop calls. On startup
 * it scans pack directories into a catalog and loads one pack: "dnd"
 * unless the process asked for another id. A later pick unloads that
 * scene, clears leftovers (UI widgets, undo, editor selection, shadow
 * occluders, camera), and loads the new pack. The scene stack is replaced,
 * not layered. Two campaigns are never active together.
 *
 * F8 opens a picker. --campaign=<id> selects one at launch.
 * --list-campaigns is handled in main and does not need this host.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"

namespace Dnd {

class SceneManager;

/**
 * @class CampaignHost
 * @brief Static bridge between the engine loop and the campaign session.
 */
class CampaignHost {
public:
    /**
     * @brief Scans packs and loads the requested campaign, or "dnd".
     *
     * Always leaves a scene on @p scenes when it returns true. A missing
     * or unknown id falls back to the D&D demo.
     * @param scenes Scene manager the host does not own.
     * @param requestedId Pack id, or nullptr / empty for the default.
     * @return false only when @p scenes is null.
     */
    static b8 Initialize(SceneManager* scenes, const char* requestedId);

    /**
     * @brief Forgets the scene manager pointer. Does not delete the scene.
     */
    static void Shutdown();

    /**
     * @brief Picker and campaign hotkeys.
     * @return true when the engine should ignore this frame's game keys
     *         (picker is open, or F8 just toggled it).
     */
    static b8 HandleInput();

    /**
     * @brief Draws the campaign chip and, when open, the pack picker.
     * @param ortho Pixel orthographic matrix (top-left origin).
     * @param screenW Overlay width in pixels.
     * @param screenH Overlay height in pixels.
     */
    static void RenderChrome(const Mat4& ortho, f32 screenW, f32 screenH);

    /** @brief True while the pack picker is open. */
    static b8 IsPickerOpen();

private:
    CampaignHost() = default;
};

} // namespace Dnd
