#pragma once

#include "core/types.h"
#include "core/math.h"
#include "ui/ui_widget.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace Dnd {

struct DockZoneRect {
    f32 x, y, w, h;
    bool Contains(f32 px, f32 py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

class DockManager {
public:
    static DockManager& Get();

    void Update(f32 dt, u32 screenWidth, u32 screenHeight);
    void Render(const Mat4& projMatrix);
    bool HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown);

    void RegisterPanel(DockPanel* panel);
    void UnregisterPanel(DockPanel* panel);
    void ClearPanels();  // Deletes all panels and clears the list

    void RecalculateZones(u32 screenWidth, u32 screenHeight);

    DockPanel* HitTest(f32 mx, f32 my) const;
    bool TrySnapPanel(DockPanel* panel, f32 mx, f32 my);

    const std::vector<DockPanel*>& GetPanels() const { return m_panels; }
    bool IsDragging() const { return m_draggingPanel != nullptr; }
    bool IsActive() const { return m_active; }
    void SetActive(b8 active) { m_active = active; }

    // Tab switching
    void SetActiveTab(DockPanel* panel);
    DockPanel* GetActivePanelInZone(const std::string& zoneId) const;

    DockZoneRect GetZoneRect(const std::string& zoneId) const;
    const std::unordered_map<std::string, DockZoneRect>& GetZoneRects() const { return m_zoneRects; }

private:
    DockManager();
    ~DockManager();
    static DockManager* s_instance;

    struct SnapZone {
        std::string id;
        DockZoneRect snapRect;   // Area that triggers snap
        DockZoneRect dockRect;   // Area panel occupies when docked
        std::vector<DockPanel*> panels;
        int activeTab = 0;
    };

    void BuildZones(u32 w, u32 h);
    std::string FindSnapZone(f32 mx, f32 my) const;
    void UpdateHoverStates(f32 mouseX, f32 mouseY);
    void RemoveFromZone(DockPanel* panel);

    std::vector<DockPanel*> m_panels;
    std::vector<SnapZone> m_zones;
    std::unordered_map<std::string, DockZoneRect> m_zoneRects;
    u32 m_screenWidth = 1280;
    u32 m_screenHeight = 720;
    b8 m_active = false;

    // Drag state
    DockPanel* m_draggingPanel = nullptr;
    Vec2 m_dragOffset;
    b8 m_wasMouseDown = false;

    // Resize state (only for floating panels)
    bool m_isResizing = false;
    DockPanel* m_resizingPanel = nullptr;
    DockPanel::ResizeEdge m_resizeEdge = DockPanel::ResizeEdge::None;
    Vec2 m_resizeStartMouse;
    Vec2 m_resizeStartSize;
    Vec2 m_resizeStartPos;

public:
    // Constants
    static constexpr f32 TOP_HEIGHT = 200.0f;
    static constexpr f32 BOTTOM_HEIGHT = 200.0f;
    static constexpr f32 SIDE_WIDTH = 250.0f;
    static constexpr f32 SNAP_THRESHOLD = 60.0f;
private:
};

} // namespace Dnd
