#include "tools/dock_manager.h"
#include "ui/ui_widget.h"
#include "ui/ui_renderer.h"
#include "core/log.h"
#include "core/input.h"
#include <algorithm>

namespace Dnd {

DockManager* DockManager::s_instance = nullptr;

DockManager& DockManager::Get() {
    if (!s_instance) {
        s_instance = new DockManager();
    }
    return *s_instance;
}

DockManager::DockManager() {
    m_screenWidth = 0;
    m_screenHeight = 0;
}
DockManager::~DockManager() {}

void DockManager::RegisterPanel(DockPanel* panel) {
    if (!panel) return;
    m_panels.push_back(panel);
    DND_LOG_DEBUG("DockManager: registered panel '%s'", panel->GetName());
}

void DockManager::UnregisterPanel(DockPanel* panel) {
    if (!panel) return;
    for (auto it = m_panels.begin(); it != m_panels.end(); ++it) {
        if (*it == panel) {
            m_panels.erase(it);
            if (m_draggingPanel == panel) m_draggingPanel = nullptr;
            return;
        }
    }
}

void DockManager::ClearPanels() {
    for (auto* panel : m_panels) {
        delete panel->GetContent();
        delete panel;
    }
    m_panels.clear();
    m_draggingPanel = nullptr;
    m_zones.clear();
    m_zoneRects.clear();
}

void DockManager::BuildZones(u32 w, u32 h) {
    m_zones.clear();

    f32 fw = static_cast<f32>(w);
    f32 fh = static_cast<f32>(h);

    // Top row: 5 zones
    f32 topW = fw / 5.0f;
    for (int i = 0; i < 5; ++i) {
        SnapZone zone;
        zone.id = "top_" + std::to_string(i);
        zone.snapRect = { i * topW, 0.0f, topW, TOP_HEIGHT };
        zone.dockRect = { i * topW + 2.0f, 2.0f, topW - 4.0f, TOP_HEIGHT - 4.0f };
        m_zones.push_back(zone);
    }

    // Bottom row: 5 zones
    f32 botW = fw / 5.0f;
    for (int i = 0; i < 5; ++i) {
        SnapZone zone;
        zone.id = "bottom_" + std::to_string(i);
        zone.snapRect = { i * botW, fh - BOTTOM_HEIGHT, botW, BOTTOM_HEIGHT };
        zone.dockRect = { i * botW + 2.0f, fh - BOTTOM_HEIGHT + 2.0f, botW - 4.0f, BOTTOM_HEIGHT - 4.0f };
        m_zones.push_back(zone);
    }

    // Left column: 3 zones (avoid overlapping top/bottom)
    f32 leftH = (fh - TOP_HEIGHT - BOTTOM_HEIGHT) / 3.0f;
    for (int i = 0; i < 3; ++i) {
        SnapZone zone;
        zone.id = "left_" + std::to_string(i);
        zone.snapRect = { 0.0f, TOP_HEIGHT + i * leftH, SIDE_WIDTH, leftH };
        zone.dockRect = { 2.0f, TOP_HEIGHT + i * leftH + 2.0f, SIDE_WIDTH - 4.0f, leftH - 4.0f };
        m_zones.push_back(zone);
    }

    // Right column: 3 zones (avoid overlapping top/bottom)
    f32 rightH = (fh - TOP_HEIGHT - BOTTOM_HEIGHT) / 3.0f;
    for (int i = 0; i < 3; ++i) {
        SnapZone zone;
        zone.id = "right_" + std::to_string(i);
        zone.snapRect = { fw - SIDE_WIDTH, TOP_HEIGHT + i * rightH, SIDE_WIDTH, rightH };
        zone.dockRect = { fw - SIDE_WIDTH + 2.0f, TOP_HEIGHT + i * rightH + 2.0f, SIDE_WIDTH - 4.0f, rightH - 4.0f };
        m_zones.push_back(zone);
    }

    // Update lookup map
    m_zoneRects.clear();
    for (auto& z : m_zones) {
        m_zoneRects[z.id] = z.dockRect;
    }
}

void DockManager::RecalculateZones(u32 screenWidth, u32 screenHeight) {
    if (screenWidth != m_screenWidth || screenHeight != m_screenHeight) {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
        BuildZones(m_screenWidth, m_screenHeight);
    }
}

std::string DockManager::FindSnapZone(f32 mx, f32 my) const {
    // Find the zone whose snapRect is closest to the given point
    // and whose snapRect contains the point (or is within SNAP_THRESHOLD)
    for (const auto& zone : m_zones) {
        DockZoneRect r = zone.snapRect;
        // Extend snap rect by SNAP_THRESHOLD for easier targeting
        DockZoneRect extended = { r.x - SNAP_THRESHOLD, r.y - SNAP_THRESHOLD,
                                  r.w + SNAP_THRESHOLD * 2, r.h + SNAP_THRESHOLD * 2 };
        if (extended.Contains(mx, my)) {
            return zone.id;
        }
    }
    return "";
}

DockPanel* DockManager::HitTest(f32 mx, f32 my) const {
    for (auto it = m_panels.rbegin(); it != m_panels.rend(); ++it) {
        DockPanel* panel = *it;
        if (!panel || !panel->IsVisible()) continue;
        Vec2 pos = panel->GetPosition();
        Vec2 size = panel->GetSize();
        if (mx >= pos.x && mx <= pos.x + size.x && my >= pos.y && my <= pos.y + size.y) {
            return panel;
        }
    }
    return nullptr;
}

bool DockManager::TrySnapPanel(DockPanel* panel, f32 mx, f32 my) {
    if (!panel) return false;

    std::string zoneId = FindSnapZone(mx, my);
    if (zoneId.empty()) return false;

    // Find the zone
    for (auto& zone : m_zones) {
        if (zone.id == zoneId) {
            // Add panel to zone
            zone.panels.push_back(panel);
            zone.activeTab = static_cast<int>(zone.panels.size()) - 1;
            panel->SetDocked(true);
            panel->SetZoneId(zoneId);

            // Position panel at dock rect
            panel->SetPosition(Vec2(zone.dockRect.x, zone.dockRect.y));
            panel->SetSize(Vec2(zone.dockRect.w, zone.dockRect.h));
            panel->SetActiveTab(true);

            DND_LOG_DEBUG("DockManager: snapped panel '%s' to zone '%s'",
                          panel->GetName(), zone.id.c_str());
            return true;
        }
    }
    return false;
}

DockZoneRect DockManager::GetZoneRect(const std::string& zoneId) const {
    auto it = m_zoneRects.find(zoneId);
    if (it != m_zoneRects.end()) return it->second;
    return { 0, 0, 0, 0 };
}

void DockManager::SetActiveTab(DockPanel* panel) {
    if (!panel || !panel->IsDocked()) return;
    for (auto& zone : m_zones) {
        for (size_t i = 0; i < zone.panels.size(); ++i) {
            zone.panels[i]->SetActiveTab(zone.panels[i] == panel);
            if (zone.panels[i] == panel) {
                zone.activeTab = static_cast<int>(i);
                // Position this panel at the zone rect
                zone.panels[i]->SetPosition(Vec2(zone.dockRect.x, zone.dockRect.y));
                zone.panels[i]->SetSize(Vec2(zone.dockRect.w, zone.dockRect.h));
            }
        }
    }
}

DockPanel* DockManager::GetActivePanelInZone(const std::string& zoneId) const {
    for (const auto& zone : m_zones) {
        if (zone.id == zoneId && !zone.panels.empty()) {
            if (zone.activeTab >= 0 && zone.activeTab < static_cast<int>(zone.panels.size())) {
                return zone.panels[zone.activeTab];
            }
            return zone.panels[0];
        }
    }
    return nullptr;
}

void DockManager::RemoveFromZone(DockPanel* panel) {
    if (!panel) return;
    for (auto& zone : m_zones) {
        for (auto it = zone.panels.begin(); it != zone.panels.end(); ++it) {
            if (*it == panel) {
                zone.panels.erase(it);
                if (zone.activeTab >= static_cast<int>(zone.panels.size())) {
                    zone.activeTab = static_cast<int>(zone.panels.size()) - 1;
                }
                // Make the new active tab visible
                if (!zone.panels.empty() && zone.activeTab >= 0) {
                    for (size_t i = 0; i < zone.panels.size(); ++i) {
                        bool active = (static_cast<int>(i) == zone.activeTab);
                        zone.panels[i]->SetActiveTab(active);
                        zone.panels[i]->SetVisible(active);
                    }
                }
                return;
            }
        }
    }
}

void DockManager::Update(f32 dt, u32 screenWidth, u32 screenHeight) {
    if (!m_active) return;

    RecalculateZones(screenWidth, screenHeight);

    f32 mx = Input::GetMouseX();
    f32 my = Input::GetMouseY();

    // Handle drag movement (continuous, not event-based)
    if (m_draggingPanel) {
        m_draggingPanel->SetPosition(Vec2(mx - m_dragOffset.x, my - m_dragOffset.y));
    }

    // Handle resize movement (continuous)
    if (m_isResizing && m_resizingPanel) {
        Vec2 delta(mx - m_resizeStartMouse.x, my - m_resizeStartMouse.y);
        Vec2 newSize = m_resizeStartSize;
        Vec2 newPos = m_resizeStartPos;
        f32 minW = 100.0f, minH = 80.0f;

        auto Clamp = [](f32 v, f32 lo) -> f32 { return v < lo ? lo : v; };

        switch (m_resizeEdge) {
            case DockPanel::ResizeEdge::Right:
                newSize.x = Clamp(m_resizeStartSize.x + delta.x, minW);
                break;
            case DockPanel::ResizeEdge::Left:
                newSize.x = Clamp(m_resizeStartSize.x - delta.x, minW);
                newPos.x = m_resizeStartPos.x + (m_resizeStartSize.x - newSize.x);
                break;
            case DockPanel::ResizeEdge::Bottom:
                newSize.y = Clamp(m_resizeStartSize.y + delta.y, minH);
                break;
            case DockPanel::ResizeEdge::Top:
                newSize.y = Clamp(m_resizeStartSize.y - delta.y, minH);
                newPos.y = m_resizeStartPos.y + (m_resizeStartSize.y - newSize.y);
                break;
            case DockPanel::ResizeEdge::BottomRight:
                newSize.x = Clamp(m_resizeStartSize.x + delta.x, minW);
                newSize.y = Clamp(m_resizeStartSize.y + delta.y, minH);
                break;
            case DockPanel::ResizeEdge::BottomLeft:
                newSize.x = Clamp(m_resizeStartSize.x - delta.x, minW);
                newPos.x = m_resizeStartPos.x + (m_resizeStartSize.x - newSize.x);
                newSize.y = Clamp(m_resizeStartSize.y + delta.y, minH);
                break;
            case DockPanel::ResizeEdge::TopRight:
                newSize.x = Clamp(m_resizeStartSize.x + delta.x, minW);
                newSize.y = Clamp(m_resizeStartSize.y - delta.y, minH);
                newPos.y = m_resizeStartPos.y + (m_resizeStartSize.y - newSize.y);
                break;
            case DockPanel::ResizeEdge::TopLeft:
                newSize.x = Clamp(m_resizeStartSize.x - delta.x, minW);
                newPos.x = m_resizeStartPos.x + (m_resizeStartSize.x - newSize.x);
                newSize.y = Clamp(m_resizeStartSize.y - delta.y, minH);
                newPos.y = m_resizeStartPos.y + (m_resizeStartSize.y - newSize.y);
                break;
            default:
                break;
        }

        m_resizingPanel->SetSize(newSize);
        m_resizingPanel->SetPosition(newPos);
    }

    // Update resize edge hover state for floating panels
    for (auto* panel : m_panels) {
        if (panel && panel->IsVisible() && !panel->IsDocked()) {
            panel->SetHoverResizeEdge(panel->HitTestResizeEdge(mx, my));
        }
    }

    // Update all panels
    for (auto* panel : m_panels) {
        if (panel && panel->IsVisible()) {
            panel->Update(dt);
        }
    }
}

void DockManager::Render(const Mat4& projMatrix) {
    if (!m_active) return;

    // Render tab bars for zones with multiple panels
    for (const auto& zone : m_zones) {
        if (zone.panels.size() > 1) {
            // Draw tab bar at top of zone rect
            DockZoneRect r = zone.dockRect;
            f32 tabBarH = DockPanel::TAB_HEIGHT;
            UIRenderer::DrawRect(r.x, r.y - tabBarH, r.w, tabBarH,
                                 Vec4(0.14f, 0.14f, 0.16f, 0.95f));

            // Draw each tab
            f32 tabX = r.x + 2.0f;
            for (size_t i = 0; i < zone.panels.size(); ++i) {
                DockPanel* p = zone.panels[i];
                if (!p) continue;

                bool isActive = (static_cast<int>(i) == zone.activeTab);
                // Tab background
                UIRenderer::DrawRect(tabX, r.y - tabBarH + 2.0f, 120.0f, tabBarH - 4.0f,
                    isActive ? Vec4(0.30f, 0.30f, 0.35f, 1.0f)
                             : Vec4(0.18f, 0.18f, 0.20f, 1.0f));

                // Tab text
                UIRenderer::DrawText(tabX + 6.0f, r.y - tabBarH + 4.0f,
                                     p->GetTitle(), Vec4(0.8f, 0.8f, 0.9f, 1.0f), 11.0f);
                tabX += 124.0f;
            }
        }
    }

    // Render all panels (reverse order so first in list renders behind)
    for (auto it = m_panels.rbegin(); it != m_panels.rend(); ++it) {
        DockPanel* panel = *it;
        if (panel && panel->IsVisible()) {
            panel->Render(projMatrix);
        }
    }
}

void DockManager::UpdateHoverStates(f32 mouseX, f32 mouseY) {
    for (auto* panel : m_panels) {
        if (panel && panel->IsVisible()) {
            panel->SetCloseHovered(panel->HitTestCloseButton(mouseX, mouseY));
        }
    }
}

bool DockManager::HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) {
    if (!m_active) return false;

    // Update hover states each frame
    UpdateHoverStates(mouseX, mouseY);

    // --- Resize release ---
    if (m_isResizing && m_wasMouseDown && !mouseDown) {
        m_isResizing = false;
        m_resizingPanel = nullptr;
        m_wasMouseDown = false;
        return true;
    }

    // --- Drag release ---
    if (m_draggingPanel && m_wasMouseDown && !mouseDown) {
        bool snapped = TrySnapPanel(m_draggingPanel, mouseX, mouseY);
        if (!snapped) {
            m_draggingPanel->SetDocked(false);
            m_draggingPanel->SetFloatingPos(m_draggingPanel->GetPosition());
        }
        m_draggingPanel = nullptr;
        m_wasMouseDown = false;
        return true;
    }

    if (!mouseDown) {
        m_wasMouseDown = false;
        return false;
    }

    // --- Tab click ---
    for (auto& zone : m_zones) {
        if (zone.panels.size() <= 1) continue;

        DockZoneRect r = zone.dockRect;
        f32 tabBarH = DockPanel::TAB_HEIGHT;
        f32 tabX = r.x + 2.0f;

        for (size_t i = 0; i < zone.panels.size(); ++i) {
            DockZoneRect tabRect = { tabX, r.y - tabBarH + 2.0f, 120.0f, tabBarH - 4.0f };
            if (tabRect.Contains(mouseX, mouseY)) {
                zone.activeTab = static_cast<int>(i);
                for (size_t j = 0; j < zone.panels.size(); ++j) {
                    bool active = (j == static_cast<size_t>(zone.activeTab));
                    zone.panels[j]->SetActiveTab(active);
                    zone.panels[j]->SetVisible(active);
                    if (active) {
                        zone.panels[j]->SetPosition(Vec2(r.x, r.y));
                        zone.panels[j]->SetSize(Vec2(r.w, r.h));
                    }
                }
                return true;
            }
            tabX += 124.0f;
        }
    }

    // --- Click on panel ---
    DockPanel* hit = HitTest(mouseX, mouseY);
    if (!hit) {
        m_wasMouseDown = false;
        return false;
    }

    // Resize handle on floating panel → start resize
    if (!hit->IsDocked()) {
        DockPanel::ResizeEdge edge = hit->HitTestResizeEdge(mouseX, mouseY);
        if (edge != DockPanel::ResizeEdge::None) {
            m_isResizing = true;
            m_resizingPanel = hit;
            m_resizeEdge = edge;
            m_resizeStartMouse = Vec2(mouseX, mouseY);
            m_resizeStartSize = hit->GetSize();
            m_resizeStartPos = hit->GetPosition();
            m_wasMouseDown = true;
            return true;
        }
    }

    // Close button
    if (hit->HitTestCloseButton(mouseX, mouseY)) {
        hit->SetVisible(false);
        RemoveFromZone(hit);
        return true;
    }

    // Title bar → start drag
    if (hit->HitTestTitleBar(mouseX, mouseY)) {
        Vec2 pos = hit->GetPosition();
        RemoveFromZone(hit);
        hit->SetDocked(false);
        hit->SetVisible(true);
        m_draggingPanel = hit;
        m_dragOffset = Vec2(mouseX - pos.x, mouseY - pos.y);
        m_wasMouseDown = true;
        return true;
    }

    // Click on a docked but inactive tab → activate it
    if (hit->IsDocked()) {
        for (auto& zone : m_zones) {
            for (size_t i = 0; i < zone.panels.size(); ++i) {
                if (zone.panels[i] == hit && static_cast<int>(i) != zone.activeTab) {
                    SetActiveTab(hit);
                    return true;
                }
            }
        }
    }

    // Dispatch input to the panel's content widget (e.g. UIFileBrowser, UITreeView)
    if (hit->GetContent() && hit->GetContent()->IsVisible()) {
        Vec2 pos = hit->GetPosition();
        Vec2 sz = hit->GetSize();
        // Content area excludes the title bar
        f32 contentY = pos.y + DockPanel::TITLE_HEIGHT;
        f32 contentH = sz.y - DockPanel::TITLE_HEIGHT;
        if (mouseX >= pos.x && mouseX <= pos.x + sz.x &&
            mouseY >= contentY && mouseY <= contentY + contentH) {
            if (hit->GetContent()->HandleInput(mouseX, mouseY, mouseDown)) {
                m_wasMouseDown = mouseDown;
                return true;
            }
        }
    }

    m_wasMouseDown = mouseDown;
    return false;
}

} // namespace Dnd
