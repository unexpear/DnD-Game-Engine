/**
 * @file encounter_panel.h
 * @brief Dockable DM encounter panel.
 *
 * Registers with DockManager like Hierarchy and Inspector. The panel body is
 * a custom widget (buttons plus a roster) because UIPanel does not forward
 * clicks to its children. Roster rules live in EncounterBuilder; this class
 * only routes clicks and refreshes the scene bridge.
 *
 * @author DND Engine Team
 * @date 2026-09-28
 * @version 0.1.0
 *
 * @note Open the editor with F5. F7 shows or hides this panel while the editor
 *       is up. Drag the title bar to dock or float it.
 */

#pragma once

#include "core/types.h"
#include "tools/encounter_builder.h"
#include "tools/encounter_scene_bridge.h"

namespace Dnd {

class DockPanel;
class Scene;

/**
 * @class EncounterPanel
 * @brief DM encounter builder shown in the editor dock.
 *
 * A dungeon master can add goblins, skeletons, and bugbears, set their hit
 * points, roll initiative (optionally including friendly characters already
 * in the scene), and start or end that encounter.
 *
 * @see EncounterBuilder
 * @see DockManager
 */
class EncounterPanel {
public:
    EncounterPanel();
    ~EncounterPanel();

    EncounterPanel(const EncounterPanel&) = delete;
    EncounterPanel& operator=(const EncounterPanel&) = delete;

    /**
     * @brief Applies queued clicks and refreshes miniatures.
     * @param scene Active scene. May be nullptr.
     */
    void Update(Scene* scene);

    /// @brief Dock panel to pass to DockManager::RegisterPanel.
    DockPanel* GetDockPanel() const { return m_dockPanel; }

    /// @brief Shows or hides the panel without leaving editor mode.
    void SetOpen(b8 open);

    /// @brief Whether the panel is currently shown.
    b8 IsOpen() const;

private:
    class View;

    DockPanel* m_dockPanel = nullptr;
    View* m_view = nullptr;
    EncounterBuilder m_builder;
    EncounterSceneBridge m_bridge;
    u32 m_selected = 0;
};

} // namespace Dnd
