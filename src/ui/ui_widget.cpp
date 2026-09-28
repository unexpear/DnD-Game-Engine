/**
 * @file ui_widget.cpp
 * @brief Implementation of UI widgets.
 *
 * See ui_widget.h for full documentation.
 *
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.7.0
 */

#include "ui/ui_widget.h"
#include "ui/ui_renderer.h"
#include "ui/drag_drop.h"
#include "assets/asset_manager.h"
#include "rendering/texture.h"
#include "core/input.h"
#include "core/log.h"
#include <cstdio>
#include <filesystem>
#include <algorithm>

namespace Dnd {

// ==========================================
// UILABEL IMPLEMENTATION
// ==========================================

UILabel::UILabel(const std::string& name, const std::string& text)
    : UIWidget(name), m_text(text), m_color(1, 1, 1, 1) {
}

void UILabel::Render(const Mat4& projMatrix) {
    DND_UNUSED(projMatrix);
    if (!m_visible || m_text.empty()) return;

    // Render the label text using the UI renderer
    // Default character size is 16 pixels (matches typical UI font size)
    UIRenderer::DrawText(m_position.x, m_position.y, m_text.c_str(), m_color, 16.0f);
}

// ==========================================
// UIBUTTON IMPLEMENTATION
// ==========================================

UIButton::UIButton(const std::string& name, const std::string& text)
    : UIWidget(name), m_text(text), m_color(0.3f, 0.5f, 0.8f, 1.0f),
      m_callback(nullptr), m_wasPressed(false) {
}

b8 UIButton::HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) {
    if (!m_visible) return false;

    UIRect rect;
    rect.x = m_position.x;
    rect.y = m_position.y;
    rect.width = m_size.x;
    rect.height = m_size.y;

    bool inside = rect.Contains(mouseX, mouseY);

    if (inside && mouseDown && !m_wasPressed) {
        // Click!
        if (m_callback) {
            m_callback();
        }
        m_wasPressed = true;
        return true;
    }

    if (!mouseDown) {
        m_wasPressed = false;
    }

    return false;
}

void UIButton::Render(const Mat4& projMatrix) {
    DND_UNUSED(projMatrix);
    if (!m_visible) return;

    // Step 1: Draw the button background rectangle
    UIRenderer::DrawRect(m_position.x, m_position.y, m_size.x, m_size.y, m_color);

    // Step 2: Draw the button text centered in the rectangle
    if (!m_text.empty()) {
        f32 charSize = 16.0f;
        f32 charH = charSize * 1.5f;

        // Calculate text width in pixels
        f32 textWidth = static_cast<f32>(m_text.length()) * charSize;

        // Center text horizontally and vertically within the button
        f32 textX = m_position.x + (m_size.x - textWidth) * 0.5f;
        f32 textY = m_position.y + (m_size.y - charH) * 0.5f;

        UIRenderer::DrawText(textX, textY, m_text.c_str(), Vec4(1, 1, 1, 1), charSize);
    }
}

// ==========================================
// UIPANEL IMPLEMENTATION
// ==========================================

UIPanel::UIPanel(const std::string& name)
    : UIWidget(name), m_color(0.2f, 0.2f, 0.2f, 0.8f) {
}

void UIPanel::Render(const Mat4& projMatrix) {
    if (!m_visible) return;

    // Step 1: Draw the panel background rectangle
    UIRenderer::DrawRect(m_position.x, m_position.y, m_size.x, m_size.y, m_color);

    // Step 2: Render all child widgets
    for (auto* child : m_children) {
        if (child) {
            child->Render(projMatrix);
        }
    }
}

b8 UIPanel::HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) {
    if (!m_visible) return false;
    UIRect rect(m_position.x, m_position.y, m_size.x, m_size.y);
    // Block presses that land on the panel so they don't orbit the camera behind it.
    return mouseDown && rect.Contains(mouseX, mouseY);
}

// ==========================================
// UISLIDER IMPLEMENTATION
// ==========================================

namespace {
/// @brief Clamps a value into [lo, hi].
inline f32 SliderClampF(f32 v, f32 lo, f32 hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
/// @brief Pixel width of the draggable knob.
constexpr f32 kSliderHandleW = 12.0f;
} // namespace

UISlider::UISlider(const std::string& name, f32 minValue, f32 maxValue, f32 value)
    : UIWidget(name),
      m_min(minValue), m_max(maxValue),
      m_value(SliderClampF(value, minValue, maxValue)),
      m_trackColor(0.16f, 0.16f, 0.20f, 1.0f),
      m_fillColor(0.42f, 0.42f, 0.52f, 1.0f),
      m_handleColor(0.85f, 0.85f, 0.90f, 1.0f),
      m_callback(nullptr), m_dragging(false) {
    m_size = Vec2(200.0f, 16.0f);
}

void UISlider::SetValue(f32 value) {
    f32 v = SliderClampF(value, m_min, m_max);
    if (v != m_value) {
        m_value = v;
        if (m_callback) m_callback(m_value);
    }
}

b8 UISlider::HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) {
    if (!m_visible) return false;

    UIRect rect(m_position.x, m_position.y, m_size.x, m_size.y);
    const bool inside = rect.Contains(mouseX, mouseY);

    if (!mouseDown) {
        m_dragging = false;
        return false;
    }

    // Begin (or continue) dragging if the press started on the slider.
    if (inside) m_dragging = true;
    if (!m_dragging) return false;

    // Map mouse X to a value, accounting for the handle half-width on each end.
    const f32 usable = m_size.x - kSliderHandleW;
    f32 t = (usable > 0.0f)
        ? (mouseX - m_position.x - kSliderHandleW * 0.5f) / usable
        : 0.0f;
    t = SliderClampF(t, 0.0f, 1.0f);

    const f32 newValue = m_min + t * (m_max - m_min);
    if (newValue != m_value) {
        m_value = newValue;
        if (m_callback) m_callback(m_value);
    }
    return true; // consume so the camera doesn't orbit while dragging
}

void UISlider::Render(const Mat4& projMatrix) {
    DND_UNUSED(projMatrix);
    if (!m_visible) return;

    const f32 x = m_position.x;
    const f32 y = m_position.y;
    const f32 w = m_size.x;
    const f32 h = m_size.y;

    // Track: full width, thin, vertically centered.
    const f32 trackH = h * 0.40f;
    const f32 trackY = y + (h - trackH) * 0.5f;
    UIRenderer::DrawRect(x, trackY, w, trackH, m_trackColor);

    // Normalized handle position.
    f32 t = (m_max > m_min) ? (m_value - m_min) / (m_max - m_min) : 0.0f;
    t = SliderClampF(t, 0.0f, 1.0f);

    const f32 usable = w - kSliderHandleW;
    const f32 handleX = x + t * usable;

    // Filled portion up to the handle center.
    UIRenderer::DrawRect(x, trackY, (handleX - x) + kSliderHandleW * 0.5f, trackH, m_fillColor);

    // Handle (knob): full widget height for an easy grab target.
    UIRenderer::DrawRect(handleX, y, kSliderHandleW, h, m_handleColor);
}

// ==========================================
// DOCKPANEL IMPLEMENTATION
// ==========================================

DockPanel::DockPanel(const std::string& name, const std::string& title)
    : UIWidget(name), m_title(title) {
    m_size = Vec2(250.0f, 300.0f);
}

void DockPanel::Update(f32 deltaTime) {
    if (!m_visible) return;

    // Update content widget if present
    if (m_content && m_content->IsVisible()) {
        f32 offsetY = TITLE_HEIGHT;
        m_content->SetPosition(Vec2(m_position.x + PADDING, m_position.y + offsetY + PADDING));
        m_content->SetSize(Vec2(m_size.x - PADDING * 2.0f, m_size.y - offsetY - PADDING * 2.0f));
        m_content->Update(deltaTime);
    }
}

b8 DockPanel::HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) {
    // DockManager handles all input for dock panels.
    // This override exists for compatibility with UISystem but is intentionally empty
    // because DockPanels are managed by DockManager, not UISystem.
    DND_UNUSED(mouseX); DND_UNUSED(mouseY); DND_UNUSED(mouseDown);
    return false;
}

void DockPanel::Render(const Mat4& projMatrix) {
    if (!m_visible) return;

    // Panel background
    UIRenderer::DrawRect(m_position.x, m_position.y, m_size.x, m_size.y,
                         Vec4(0.18f, 0.18f, 0.20f, 0.95f));

    // Title bar
    UIRenderer::DrawRect(m_position.x, m_position.y, m_size.x, TITLE_HEIGHT,
                         m_activeTab ? Vec4(0.30f, 0.30f, 0.35f, 1.0f)
                                     : Vec4(0.22f, 0.22f, 0.25f, 1.0f));

    // Title text
    f32 titleX = m_position.x + PADDING + 2.0f;
    f32 titleY = m_position.y + (TITLE_HEIGHT - 16.0f * 1.5f) * 0.5f;
    UIRenderer::DrawText(titleX, titleY, m_title.c_str(),
                         Vec4(0.9f, 0.9f, 0.95f, 1.0f), 12.0f);

    // Close button
    f32 closeX = m_position.x + m_size.x - CLOSE_SIZE - PADDING;
    f32 closeY = m_position.y + (TITLE_HEIGHT - CLOSE_SIZE) * 0.5f;
    Vec4 closeColor = m_hoverClose ? Vec4(0.9f, 0.2f, 0.2f, 1.0f)
                                   : Vec4(0.5f, 0.5f, 0.55f, 1.0f);
    UIRenderer::DrawRect(closeX, closeY, CLOSE_SIZE, CLOSE_SIZE, closeColor);
    f32 cx = closeX + CLOSE_SIZE * 0.5f - 4.0f;
    f32 cy = closeY + CLOSE_SIZE * 0.5f - 6.0f;
    UIRenderer::DrawText(cx, cy, "x", Vec4(1, 1, 1, 1), 8.0f);

    // Resize edge highlights (when floating)
    if (!m_docked && m_hoverResizeEdge != ResizeEdge::None) {
        Vec4 highlight(0.6f, 0.7f, 1.0f, 0.3f);
        f32 h = EDGE_HIT;
        switch (m_hoverResizeEdge) {
            case ResizeEdge::Left:
                UIRenderer::DrawRect(m_position.x, m_position.y, h, m_size.y, highlight);
                break;
            case ResizeEdge::Right:
                UIRenderer::DrawRect(m_position.x + m_size.x - h, m_position.y, h, m_size.y, highlight);
                break;
            case ResizeEdge::Top:
                UIRenderer::DrawRect(m_position.x, m_position.y, m_size.x, h, highlight);
                break;
            case ResizeEdge::Bottom:
                UIRenderer::DrawRect(m_position.x, m_position.y + m_size.y - h, m_size.x, h, highlight);
                break;
            case ResizeEdge::TopLeft:
                UIRenderer::DrawRect(m_position.x, m_position.y, h * 2, h * 2, highlight);
                break;
            case ResizeEdge::TopRight:
                UIRenderer::DrawRect(m_position.x + m_size.x - h * 2, m_position.y, h * 2, h * 2, highlight);
                break;
            case ResizeEdge::BottomLeft:
                UIRenderer::DrawRect(m_position.x, m_position.y + m_size.y - h * 2, h * 2, h * 2, highlight);
                break;
            case ResizeEdge::BottomRight:
                UIRenderer::DrawRect(m_position.x + m_size.x - h * 2, m_position.y + m_size.y - h * 2, h * 2, h * 2, highlight);
                break;
            default: break;
        }
    }

    // Content widget
    if (m_content && m_content->IsVisible()) {
        m_content->Render(projMatrix);
    }

    // Corner resize handle (only when floating)
    if (!m_docked) {
        f32 rx = m_position.x + m_size.x - RESIZE_HANDLE;
        f32 ry = m_position.y + m_size.y - RESIZE_HANDLE;
        UIRenderer::DrawRect(rx, ry, RESIZE_HANDLE, RESIZE_HANDLE,
                             Vec4(0.35f, 0.35f, 0.40f, 1.0f));
        UIRenderer::DrawText(rx + 1, ry + 1, "/", Vec4(0.6f, 0.6f, 0.7f, 1.0f), 8.0f);
    }
}

// --- Hit-test helpers ---

b8 DockPanel::HitTestTitleBar(f32 mx, f32 my) const {
    if (!m_visible) return false;
    UIRect titleRect(m_position.x, m_position.y, m_size.x, TITLE_HEIGHT);
    if (!titleRect.Contains(mx, my)) return false;
    // Don't hit if over close button
    UIRect closeRect = GetCloseButtonRect();
    return !closeRect.Contains(mx, my);
}

b8 DockPanel::HitTestCloseButton(f32 mx, f32 my) const {
    if (!m_visible) return false;
    return GetCloseButtonRect().Contains(mx, my);
}

DockPanel::ResizeEdge DockPanel::HitTestResizeEdge(f32 mx, f32 my) const {
    if (!m_visible || m_docked) return ResizeEdge::None;
    f32 x = m_position.x, y = m_position.y, w = m_size.x, h = m_size.y;
    f32 e = EDGE_HIT;

    // Check corners first (they take priority)
    UIRect tl(x, y, e * 2, e * 2);
    UIRect tr(x + w - e * 2, y, e * 2, e * 2);
    UIRect bl(x, y + h - e * 2, e * 2, e * 2);
    UIRect br(x + w - e * 2, y + h - e * 2, e * 2, e * 2);

    if (tl.Contains(mx, my)) return ResizeEdge::TopLeft;
    if (tr.Contains(mx, my)) return ResizeEdge::TopRight;
    if (bl.Contains(mx, my)) return ResizeEdge::BottomLeft;
    if (br.Contains(mx, my)) return ResizeEdge::BottomRight;

    // Check edges
    if (mx >= x && mx <= x + e && my > y + e && my < y + h - e) return ResizeEdge::Left;
    if (mx >= x + w - e && mx <= x + w && my > y + e && my < y + h - e) return ResizeEdge::Right;
    if (my >= y && my <= y + e && mx > x + e && mx < x + w - e) return ResizeEdge::Top;
    if (my >= y + h - e && my <= y + h && mx > x + e && mx < x + w - e) return ResizeEdge::Bottom;

    return ResizeEdge::None;
}

b8 DockPanel::HitTestResizeHandle(f32 mx, f32 my) const {
    if (!m_visible || m_docked) return false;
    UIRect r(m_position.x + m_size.x - RESIZE_HANDLE,
             m_position.y + m_size.y - RESIZE_HANDLE,
             RESIZE_HANDLE, RESIZE_HANDLE);
    return r.Contains(mx, my);
}

UIRect DockPanel::GetCloseButtonRect() const {
    f32 x = m_position.x + m_size.x - CLOSE_SIZE - PADDING;
    f32 y = m_position.y + (TITLE_HEIGHT - CLOSE_SIZE) * 0.5f;
    return UIRect(x, y, CLOSE_SIZE, CLOSE_SIZE);
}

// ==========================================
// UITREEVIEW IMPLEMENTATION
// ==========================================

UITreeView::UITreeView(const std::string& name)
    : UIWidget(name) {
}

i32 UITreeView::AddNode(const std::string& label, i32 parentIndex, u64 userData) {
    TreeNode node;
    node.label = label;
    node.parent = parentIndex;
    node.userData = userData;
    node.expanded = true;
    node.visible = true;

    i32 idx = static_cast<i32>(m_nodes.size());
    m_nodes.push_back(node);

    if (parentIndex >= 0 && parentIndex < static_cast<i32>(m_nodes.size())) {
        m_nodes[parentIndex].children.push_back(idx);
    }

    RebuildVisibleOrder();
    return idx;
}

void UITreeView::RemoveNode(i32 index) {
    if (index < 0 || index >= static_cast<i32>(m_nodes.size())) return;

    // Remove from parent's children list
    i32 parent = m_nodes[index].parent;
    if (parent >= 0) {
        auto& siblings = m_nodes[parent].children;
        for (auto it = siblings.begin(); it != siblings.end(); ++it) {
            if (*it == index) { siblings.erase(it); break; }
        }
    }

    // Recursively remove children
    for (i32 child : m_nodes[index].children) {
        RemoveNode(child);
    }

    // Mark as invalid (we don't shift indices to keep existing references valid)
    m_nodes[index].label = "";
    m_nodes[index].visible = false;
    m_nodes[index].children.clear();

    if (m_selectedIndex == index) m_selectedIndex = -1;

    RebuildVisibleOrder();
}

void UITreeView::Clear() {
    m_nodes.clear();
    m_visibleOrder.clear();
    m_selectedIndex = -1;
    m_scrollOffset = 0.0f;
}

u64 UITreeView::GetNodeUserData(i32 index) const {
    if (index < 0 || index >= static_cast<i32>(m_nodes.size())) return 0;
    return m_nodes[index].userData;
}

const char* UITreeView::GetNodeLabel(i32 index) const {
    if (index < 0 || index >= static_cast<i32>(m_nodes.size())) return "";
    return m_nodes[index].label.c_str();
}

void UITreeView::Expand(i32 index) {
    if (index < 0 || index >= static_cast<i32>(m_nodes.size())) return;
    m_nodes[index].expanded = true;
    RebuildVisibleOrder();
}

void UITreeView::Collapse(i32 index) {
    if (index < 0 || index >= static_cast<i32>(m_nodes.size())) return;
    m_nodes[index].expanded = false;
    if (m_selectedIndex >= 0) {
        // If selected node is a descendant of collapsed node, deselect
        i32 sel = m_selectedIndex;
        while (sel >= 0) {
            if (sel == index) { m_selectedIndex = -1; break; }
            sel = m_nodes[sel].parent;
        }
    }
    RebuildVisibleOrder();
}

void UITreeView::ExpandAll() {
    for (auto& node : m_nodes) node.expanded = true;
    RebuildVisibleOrder();
}

void UITreeView::CollapseAll() {
    for (auto& node : m_nodes) node.expanded = false;
    m_selectedIndex = -1;
    RebuildVisibleOrder();
}

b8 UITreeView::IsExpanded(i32 index) const {
    if (index < 0 || index >= static_cast<i32>(m_nodes.size())) return false;
    return m_nodes[index].expanded;
}

void UITreeView::SetSelected(i32 index) {
    if (index < -1 || index >= static_cast<i32>(m_nodes.size())) return;
    i32 oldSel = m_selectedIndex;
    m_selectedIndex = index;
    if (index >= 0 && !m_nodes[index].visible) {
        m_selectedIndex = -1;
    }
    if (m_selectedIndex != oldSel && m_selectCallback) {
        m_selectCallback(m_selectedIndex, m_selectedIndex >= 0 ? m_nodes[m_selectedIndex].userData : 0);
    }
}

u64 UITreeView::GetSelectedUserData() const {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<i32>(m_nodes.size())) return 0;
    return m_nodes[m_selectedIndex].userData;
}

void UITreeView::SetSearchFilter(const std::string& filter) {
    m_searchFilter = filter;
    if (filter.empty()) {
        for (auto& node : m_nodes) node.visible = true;
    } else {
        std::string lowerFilter = filter;
        for (auto& c : lowerFilter) c = static_cast<char>(std::tolower(c));
        for (auto& node : m_nodes) {
            std::string lowerLabel = node.label;
            for (auto& c : lowerLabel) c = static_cast<char>(std::tolower(c));
            node.visible = lowerLabel.find(lowerFilter) != std::string::npos;
        }
        // Ensure parent chain is visible for any visible child
        for (i32 i = static_cast<i32>(m_nodes.size()) - 1; i >= 0; --i) {
            if (m_nodes[i].visible && m_nodes[i].parent >= 0) {
                m_nodes[m_nodes[i].parent].visible = true;
            }
        }
    }
    RebuildVisibleOrder();
}

void UITreeView::RebuildVisibleOrder() {
    m_visibleOrder.clear();
    m_visibleOrder.reserve(m_nodes.size());

    auto addChildren = [&](i32 parentIdx, auto&& self) -> void {
        for (i32 childIdx : m_nodes[parentIdx].children) {
            if (childIdx >= 0 && childIdx < static_cast<i32>(m_nodes.size()) && m_nodes[childIdx].visible) {
                m_visibleOrder.push_back(childIdx);
                if (m_nodes[childIdx].expanded) {
                    self(childIdx, self);
                }
            }
        }
    };

    for (i32 i = 0; i < static_cast<i32>(m_nodes.size()); ++i) {
        if (m_nodes[i].parent < 0 && m_nodes[i].visible) {
            m_visibleOrder.push_back(i);
            if (m_nodes[i].expanded) {
                addChildren(i, addChildren);
            }
        }
    }
}

i32 UITreeView::ComputeVisibleIndex(i32 treeIndex) const {
    for (i32 i = 0; i < static_cast<i32>(m_visibleOrder.size()); ++i) {
        if (m_visibleOrder[i] == treeIndex) return i;
    }
    return -1;
}

i32 UITreeView::GetNodeIndexAt(f32 mx, f32 my) const {
    return HitTestRow(mx, my);
}

i32 UITreeView::HitTestRow(f32 mx, f32 my) const {
    if (mx < m_position.x || mx > m_position.x + m_size.x - SCROLLBAR_WIDTH) return -1;
    f32 relY = my - (m_position.y + ROW_HEIGHT); // skip search bar row
    if (relY < 0) return -1;
    i32 row = static_cast<i32>((relY + m_scrollOffset) / ROW_HEIGHT);
    if (row >= 0 && row < static_cast<i32>(m_visibleOrder.size())) return m_visibleOrder[row];
    return -1;
}

void UITreeView::Update(f32 deltaTime) {
    DND_UNUSED(deltaTime);
    if (!m_visible) return;

    // Drag detection: check if threshold exceeded
    if (m_dragPending && m_dragStartCallback && m_dragItemIndex >= 0 &&
        Input::IsMouseButtonDown(MouseButton::LEFT)) {
        f32 dx = Input::GetMouseX() - m_dragStartX;
        f32 dy = Input::GetMouseY() - m_dragStartY;
        if (dx * dx + dy * dy >= 16.0f) { // 4px threshold squared
            if (m_dragItemIndex < static_cast<i32>(m_nodes.size())) {
                m_dragStartCallback(m_dragItemIndex, m_nodes[m_dragItemIndex].userData,
                                    Input::GetMouseX(), Input::GetMouseY());
            }
            m_dragPending = false;
        }
    }
}

b8 UITreeView::HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) {
    if (!m_visible) return false;
    if (mouseX < m_position.x || mouseX > m_position.x + m_size.x) return false;
    if (mouseY < m_position.y || mouseY > m_position.y + m_size.y) return false;

    // Scroll wheel / release
    if (!mouseDown && m_wasMouseDown) {
        m_wasMouseDown = false;
        m_dragPending = false;
        return false;
    }

    f32 maxScroll = std::max(0.0f, static_cast<f32>(m_visibleOrder.size()) * ROW_HEIGHT - (m_size.y - ROW_HEIGHT));

    // Check for scrollbar drag
    if (m_hoverScrollBar && mouseDown) {
        // Map mouse Y to scroll position
        f32 trackH = m_size.y - ROW_HEIGHT;
        f32 scrollH = trackH * trackH / (static_cast<f32>(m_visibleOrder.size()) * ROW_HEIGHT);
        f32 scrollTrackH = trackH - scrollH;
        if (scrollTrackH > 0) {
            f32 t = (mouseY - (m_position.y + ROW_HEIGHT)) / scrollTrackH;
            m_scrollOffset = t * maxScroll;
        }
        return true;
    }

    if (!mouseDown) {
        m_dragPending = false;
        return false;
    }

    // Check scrollbar area
    f32 sbX = m_position.x + m_size.x - SCROLLBAR_WIDTH;
    if (mouseX >= sbX) {
        // Only consume if we hit this far right
        if (m_visibleOrder.size() > 0) {
            f32 rowH = m_visibleOrder.size() * ROW_HEIGHT;
            if (rowH > m_size.y - ROW_HEIGHT) {
                m_hoverScrollBar = true;
                return true;
            }
        }
        return false;
    }

    // Check search bar row (top row)
    if (mouseY < m_position.y + ROW_HEIGHT) {
        return false; // Allow text input for search
    }

    // Hit test tree rows
    i32 hitIndex = HitTestRow(mouseX, mouseY);
    if (hitIndex < 0) return false;

    // Check if click was on the expand arrow
    i32 depth = 0;
    i32 p = m_nodes[hitIndex].parent;
    while (p >= 0) { depth++; p = m_nodes[p].parent; }
    f32 arrowX = m_position.x + 2.0f + static_cast<f32>(depth) * INDENT_WIDTH;
    bool arrowHit = (mouseX >= arrowX && mouseX < arrowX + ARROW_WIDTH);

    m_wasMouseDown = mouseDown;

    if (arrowHit && !m_nodes[hitIndex].children.empty()) {
        // Toggle expand/collapse
        if (m_nodes[hitIndex].expanded) {
            Collapse(hitIndex);
        } else {
            Expand(hitIndex);
        }
        return true;
    }

    // Select the node
    SetSelected(hitIndex);

    // Initiate drag detection (pending until threshold crossed in Update)
    m_dragPending = true;
    m_dragStartX = mouseX;
    m_dragStartY = mouseY;
    m_dragItemIndex = hitIndex;

    return true;
}

void UITreeView::Render(const Mat4& projMatrix) {
    if (!m_visible) return;

    f32 x = m_position.x;
    f32 y = m_position.y;
    f32 w = m_size.x;
    f32 h = m_size.y;

    // Background
    UIRenderer::DrawRect(x, y, w, h, Vec4(0.12f, 0.12f, 0.14f, 1.0f));

    // Search bar row
    UIRenderer::DrawRect(x, y, w, ROW_HEIGHT, Vec4(0.16f, 0.16f, 0.18f, 1.0f));
    std::string searchText = m_searchFilter.empty() ? "Search..." : m_searchFilter;
    UIRenderer::DrawText(x + 4.0f, y + 2.0f, searchText.c_str(),
                         m_searchFilter.empty() ? Vec4(0.4f, 0.4f, 0.5f, 1.0f)
                                                : Vec4(0.7f, 0.7f, 0.8f, 1.0f), 11.0f);

    // Clip to content area
    f32 contentY = y + ROW_HEIGHT;
    f32 contentH = h - ROW_HEIGHT;
    f32 totalH = static_cast<f32>(m_visibleOrder.size()) * ROW_HEIGHT;
    f32 maxScroll = std::max(0.0f, totalH - contentH);
    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
    if (m_scrollOffset < 0) m_scrollOffset = 0;

    // Draw visible rows
    i32 firstRow = static_cast<i32>(m_scrollOffset / ROW_HEIGHT);
    i32 lastRow = std::min(static_cast<i32>((m_scrollOffset + contentH) / ROW_HEIGHT) + 1,
                           static_cast<i32>(m_visibleOrder.size()));

    for (i32 vi = firstRow; vi < lastRow; ++vi) {
        i32 ti = m_visibleOrder[vi];
        if (ti < 0 || ti >= static_cast<i32>(m_nodes.size())) continue;

        f32 rowY = contentY + static_cast<f32>(vi) * ROW_HEIGHT - m_scrollOffset;
        if (rowY + ROW_HEIGHT < contentY || rowY > contentY + contentH) continue;

        bool isSelected = (ti == m_selectedIndex);

        // Row background
        if (isSelected) {
            UIRenderer::DrawRect(x + 1.0f, rowY, w - SCROLLBAR_WIDTH - 2.0f, ROW_HEIGHT,
                                 Vec4(0.25f, 0.35f, 0.50f, 1.0f));
        } else if (vi % 2 == 1) {
            UIRenderer::DrawRect(x + 1.0f, rowY, w - SCROLLBAR_WIDTH - 2.0f, ROW_HEIGHT,
                                 Vec4(0.13f, 0.13f, 0.15f, 1.0f));
        }

        // Indentation
        i32 depth = 0;
        i32 p = m_nodes[ti].parent;
        while (p >= 0) { depth++; p = m_nodes[p].parent; }

        f32 indent = static_cast<f32>(depth) * INDENT_WIDTH;

        // Expand arrow
        if (!m_nodes[ti].children.empty()) {
            f32 arrowX = x + 2.0f + indent;
            f32 arrowY = rowY + (ROW_HEIGHT - 8.0f) * 0.5f;
            const char* arrow = m_nodes[ti].expanded ? "v" : ">";
            UIRenderer::DrawText(arrowX, arrowY, arrow,
                                 Vec4(0.6f, 0.6f, 0.7f, 1.0f), 8.0f);
        }

        // Label
        f32 labelX = x + 4.0f + indent + ARROW_WIDTH;
        f32 labelY = rowY + 2.0f;
        UIRenderer::DrawText(labelX, labelY, m_nodes[ti].label.c_str(),
                             Vec4(0.8f, 0.8f, 0.9f, 1.0f), 11.0f);
    }

    // Scrollbar
    if (totalH > contentH) {
        f32 sbX = x + w - SCROLLBAR_WIDTH;
        f32 sbTrackH = contentH;
        f32 scrollH = sbTrackH * sbTrackH / totalH;
        f32 scrollT = (maxScroll > 0) ? m_scrollOffset / maxScroll : 0.0f;
        f32 scrollY = contentY + scrollT * (sbTrackH - scrollH);

        UIRenderer::DrawRect(sbX, contentY, SCROLLBAR_WIDTH, sbTrackH,
                             Vec4(0.15f, 0.15f, 0.17f, 1.0f));
        UIRenderer::DrawRect(sbX, scrollY, SCROLLBAR_WIDTH, scrollH,
                             Vec4(0.35f, 0.35f, 0.40f, 1.0f));
    }
}

// ==========================================
// UIPROPERTYGRID IMPLEMENTATION
// ==========================================

UIPropertyGrid::UIPropertyGrid(const std::string& name)
    : UIWidget(name) {
}

void UIPropertyGrid::Clear() {
    m_entries.clear();
    m_scrollOffset = 0.0f;
    m_textEditIndex = -1;
}

void UIPropertyGrid::AddCategory(const std::string& name) {
    PropEntry e;
    e.type = PropType::CATEGORY;
    e.label = name;
    m_entries.push_back(e);
}

void UIPropertyGrid::AddFloat(const std::string& label, f32 value, FloatCallback onChange) {
    PropEntry e;
    e.type = PropType::FLOAT;
    e.label = label;
    e.floatVal = value;
    e.onFloatChange = onChange;
    m_entries.push_back(e);
}

void UIPropertyGrid::AddInt(const std::string& label, i32 value, IntCallback onChange) {
    PropEntry e;
    e.type = PropType::INT;
    e.label = label;
    e.intVal = value;
    e.onIntChange = onChange;
    m_entries.push_back(e);
}

void UIPropertyGrid::AddBool(const std::string& label, b8 value, BoolCallback onChange) {
    PropEntry e;
    e.type = PropType::BOOL;
    e.label = label;
    e.boolVal = value;
    e.onBoolChange = onChange;
    m_entries.push_back(e);
}

void UIPropertyGrid::AddString(const std::string& label, const std::string& value, StringCallback onChange) {
    PropEntry e;
    e.type = PropType::STRING;
    e.label = label;
    e.stringVal = value;
    e.onStringChange = onChange;
    m_entries.push_back(e);
}

void UIPropertyGrid::AddVec3(const std::string& label, const Vec3& value, Vec3Callback onChange) {
    PropEntry e;
    e.type = PropType::VEC3;
    e.label = label;
    e.vec3Val = value;
    e.onVec3Change = onChange;
    m_entries.push_back(e);
}

void UIPropertyGrid::AddEnum(const std::string& label, i32 value,
                             const std::vector<std::string>& options, EnumCallback onChange) {
    PropEntry e;
    e.type = PropType::ENUM;
    e.label = label;
    e.enumVal = value;
    e.enumOptions = options;
    e.onEnumChange = onChange;
    m_entries.push_back(e);
}

void UIPropertyGrid::AddSlider(const std::string& label, f32 value, f32 min, f32 max,
                               FloatCallback onChange) {
    PropEntry e;
    e.type = PropType::SLIDER;
    e.label = label;
    e.floatVal = value;
    e.floatMin = min;
    e.floatMax = max;
    e.onFloatChange = onChange;
    m_entries.push_back(e);
}

void UIPropertyGrid::AddLabel(const std::string& label, const std::string& value) {
    PropEntry e;
    e.type = PropType::LABEL;
    e.label = label;
    e.stringVal = value;
    m_entries.push_back(e);
}

f32 UIPropertyGrid::TotalContentHeight() const {
    f32 h = 0.0f;
    for (const auto& e : m_entries) {
        h += (e.type == PropType::CATEGORY) ? CATEGORY_HEIGHT : ROW_HEIGHT;
    }
    return h;
}

i32 UIPropertyGrid::HitTestRow(f32 mx, f32 my) const {
    f32 y = m_position.y;
    for (i32 i = 0; i < static_cast<i32>(m_entries.size()); ++i) {
        f32 rowH = (m_entries[i].type == PropType::CATEGORY) ? CATEGORY_HEIGHT : ROW_HEIGHT;
        if (mx >= m_position.x && mx <= m_position.x + m_size.x - SCROLLBAR_WIDTH &&
            my >= y - m_scrollOffset && my < y - m_scrollOffset + rowH) {
            return i;
        }
        y += rowH;
    }
    return -1;
}

void UIPropertyGrid::Update(f32 deltaTime) {
    DND_UNUSED(deltaTime);
    if (!m_visible) return;

    // Text editing: poll keyboard input while editing a string property
    if (m_textEditIndex >= 0 && m_textEditIndex < static_cast<i32>(m_entries.size())) {
        auto& entry = m_entries[m_textEditIndex];
        if (entry.type == PropType::STRING && Input::IsTextInputActive()) {
            const std::string& input = Input::GetTextInput();
            if (!input.empty()) {
                entry.stringVal += input;
                if (entry.onStringChange) {
                    entry.onStringChange(entry.stringVal);
                }
            }
            if (Input::IsKeyPressed(KeyCode::BACKSPACE)) {
                if (!entry.stringVal.empty()) {
                    entry.stringVal.pop_back();
                    if (entry.onStringChange) {
                        entry.onStringChange(entry.stringVal);
                    }
                }
            }
            if (Input::IsKeyPressed(KeyCode::ENTER) || Input::IsKeyPressed(KeyCode::ESCAPE)) {
                Input::EndTextInput();
                m_textEditIndex = -1;
                if (m_onCommit) m_onCommit();
            }
        }
    }

    // Scroll with mouse wheel
    f32 scrollDelta = Input::GetScrollDelta();
    if (scrollDelta != 0.0f) {
        UIRect bounds(m_position.x, m_position.y, m_size.x, m_size.y);
        f32 mx = Input::GetMouseX();
        f32 my = Input::GetMouseY();
        if (bounds.Contains(mx, my)) {
            m_scrollOffset -= scrollDelta * 40.0f;
        }
    }

    // Clamp scroll
    f32 contentH = TotalContentHeight();
    f32 viewH = m_size.y;
    f32 maxScroll = std::max(0.0f, contentH - viewH);
    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
    if (m_scrollOffset < 0) m_scrollOffset = 0;

    // End active drags when mouse button is released
    if (!Input::IsMouseButtonDown(MouseButton::LEFT)) {
        bool hadDrag = false;
        for (auto& e : m_entries) {
            if (e.dragActive) {
                e.dragActive = false;
                e.dragSubField = -1;
                hadDrag = true;
            }
        }
        if (hadDrag && m_onCommit) m_onCommit();
    }
}

b8 UIPropertyGrid::HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) {
    if (!m_visible) return false;

    // Consume any press that lands in the grid area so camera won't orbit.
    UIRect bounds(m_position.x, m_position.y, m_size.x, m_size.y);
    if (!bounds.Contains(mouseX, mouseY)) return false;

    // On press (transition from up to down)
    if (mouseDown && !m_wasMouseDown) {
        m_wasMouseDown = true;

        i32 hit = HitTestRow(mouseX, mouseY);
        if (hit >= 0 && hit < static_cast<i32>(m_entries.size())) {
            PropEntry& entry = m_entries[hit];
            f32 localX = mouseX - m_position.x;

            switch (entry.type) {
                case PropType::FLOAT:
                    if (localX >= VALUE_OFFSET) {
                        entry.dragActive = true;
                        entry.dragStartX = mouseX;
                        entry.dragStartVal = entry.floatVal;
                    }
                    break;

                case PropType::INT:
                    if (localX >= VALUE_OFFSET) {
                        entry.dragActive = true;
                        entry.dragStartX = mouseX;
                        entry.dragStartVal = static_cast<f32>(entry.intVal);
                    }
                    break;

                case PropType::BOOL: {
                    // Checkbox hit-test
                    f32 cbX = m_position.x + VALUE_OFFSET;
                    // Compute row Y for this entry
                    f32 rowY = m_position.y;
                    for (i32 i = 0; i < hit; ++i) {
                        rowY += (m_entries[i].type == PropType::CATEGORY) ? CATEGORY_HEIGHT : ROW_HEIGHT;
                    }
                    f32 rowH = ROW_HEIGHT;
                    f32 cbY = rowY - m_scrollOffset + (rowH - CHECKBOX_SIZE) * 0.5f;
                    UIRect cbRect(cbX, cbY, CHECKBOX_SIZE, CHECKBOX_SIZE);
                    if (cbRect.Contains(mouseX, mouseY)) {
                        entry.boolVal = !entry.boolVal;
                        if (entry.onBoolChange) entry.onBoolChange(entry.boolVal);
                        if (m_onCommit) m_onCommit();
                    }
                    break;
                }

                case PropType::STRING:
                    if (localX >= VALUE_OFFSET) {
                        m_textEditIndex = hit;
                        Input::BeginTextInput();
                    }
                    break;

                case PropType::VEC3:
                    if (localX >= VALUE_OFFSET) {
                        f32 subW = (m_size.x - VALUE_OFFSET - SCROLLBAR_WIDTH - 8.0f) / 3.0f;
                        if (subW < 20.0f) subW = 20.0f;
                        i32 subField = static_cast<i32>((localX - VALUE_OFFSET) / (subW + 4.0f));
                        if (subField >= 0 && subField <= 2) {
                            entry.dragActive = true;
                            entry.dragStartX = mouseX;
                            entry.dragSubField = subField;
                            entry.dragStartVal = (subField == 0) ? entry.vec3Val.x
                                                : (subField == 1) ? entry.vec3Val.y
                                                : entry.vec3Val.z;
                        }
                    }
                    break;

                case PropType::ENUM:
                    if (localX >= VALUE_OFFSET) {
                        entry.enumVal = (entry.enumVal + 1) % static_cast<i32>(entry.enumOptions.size());
                        if (entry.onEnumChange) entry.onEnumChange(entry.enumVal);
                        if (m_onCommit) m_onCommit();
                    }
                    break;

                case PropType::SLIDER: {
                    f32 trackX = m_position.x + VALUE_OFFSET;
                    f32 trackW = m_size.x - VALUE_OFFSET - SCROLLBAR_WIDTH - 8.0f;
                    if (trackW > 0 && localX >= VALUE_OFFSET) {
                        f32 t = (mouseX - trackX) / trackW;
                        if (t < 0) t = 0.0f;
                        if (t > 1.0f) t = 1.0f;
                        entry.floatVal = entry.floatMin + t * (entry.floatMax - entry.floatMin);
                        if (entry.onFloatChange) entry.onFloatChange(entry.floatVal);
                        entry.dragActive = true;
                        entry.dragStartX = mouseX;
                        entry.dragStartVal = entry.floatVal;
                    }
                    break;
                }

                default: break;
            }
        }
        return true; // Consumed
    }

    // Update active drags (mouse held down, continuing from previous frame)
    if (mouseDown && m_wasMouseDown) {
        bool anyDrag = false;
        for (auto& entry : m_entries) {
            if (!entry.dragActive) continue;
            anyDrag = true;

            f32 delta = mouseX - entry.dragStartX;
            if (entry.type == PropType::INT) {
                i32 newVal = static_cast<i32>(entry.dragStartVal + delta * 0.1f);
                if (newVal != entry.intVal) {
                    entry.intVal = newVal;
                    if (entry.onIntChange) entry.onIntChange(entry.intVal);
                }
            } else if (entry.type == PropType::VEC3) {
                f32 newVal = entry.dragStartVal + delta * 0.02f;
                Vec3 v = entry.vec3Val;
                if (entry.dragSubField == 0) v.x = newVal;
                else if (entry.dragSubField == 1) v.y = newVal;
                else v.z = newVal;
                entry.vec3Val = v;
                if (entry.onVec3Change) entry.onVec3Change(v);
            } else if (entry.type == PropType::SLIDER) {
                f32 trackX = m_position.x + VALUE_OFFSET;
                f32 trackW = m_size.x - VALUE_OFFSET - SCROLLBAR_WIDTH - 8.0f;
                if (trackW > 0) {
                    f32 t = (mouseX - trackX) / trackW;
                    if (t < 0) t = 0.0f;
                    if (t > 1.0f) t = 1.0f;
                    f32 newVal = entry.floatMin + t * (entry.floatMax - entry.floatMin);
                    if (newVal != entry.floatVal) {
                        entry.floatVal = newVal;
                        if (entry.onFloatChange) entry.onFloatChange(newVal);
                    }
                }
            } else {
                f32 newVal = entry.dragStartVal + delta * 0.02f;
                if (newVal != entry.floatVal) {
                    entry.floatVal = newVal;
                    if (entry.onFloatChange) entry.onFloatChange(entry.floatVal);
                }
            }
        }
        if (anyDrag) return true;
    }

    // On release
    if (!mouseDown && m_wasMouseDown) {
        m_wasMouseDown = false;
    }

    return true; // Consume all mouse events within bounds
}

void UIPropertyGrid::Render(const Mat4& projMatrix) {
    DND_UNUSED(projMatrix);
    if (!m_visible) return;

    f32 x = m_position.x;
    f32 y = m_position.y;
    f32 w = m_size.x;
    f32 h = m_size.y;

    // Background
    UIRenderer::DrawRect(x, y, w, h, Vec4(0.12f, 0.12f, 0.14f, 1.0f));

    f32 contentH = TotalContentHeight();
    f32 viewH = h;
    f32 maxScroll = std::max(0.0f, contentH - viewH);
    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
    if (m_scrollOffset < 0) m_scrollOffset = 0;

    // Draw visible rows
    f32 drawY = y - m_scrollOffset;
    for (i32 i = 0; i < static_cast<i32>(m_entries.size()); ++i) {
        const PropEntry& entry = m_entries[i];
        f32 rowH = (entry.type == PropType::CATEGORY) ? CATEGORY_HEIGHT : ROW_HEIGHT;

        if (drawY + rowH >= y && drawY <= y + h) {
            switch (entry.type) {
                case PropType::CATEGORY:
                    UIRenderer::DrawRect(x, drawY, w, rowH, Vec4(0.18f, 0.18f, 0.22f, 1.0f));
                    UIRenderer::DrawText(x + 6.0f, drawY + 4.0f, entry.label.c_str(),
                                         Vec4(0.7f, 0.7f, 0.9f, 1.0f), 11.0f);
                    break;

                case PropType::FLOAT: {
                    UIRenderer::DrawText(x + 6.0f, drawY + 3.0f, entry.label.c_str(),
                                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%.3f", entry.floatVal);
                    f32 valW = w - VALUE_OFFSET - SCROLLBAR_WIDTH - 8.0f;
                    UIRenderer::DrawRect(x + VALUE_OFFSET, drawY + 2.0f, valW, ROW_HEIGHT - 4.0f,
                                         Vec4(0.16f, 0.16f, 0.18f, 1.0f));
                    UIRenderer::DrawText(x + VALUE_OFFSET + 4.0f, drawY + 3.0f, buf,
                                         Vec4(0.8f, 0.8f, 0.9f, 1.0f), 10.0f);
                    break;
                }

                case PropType::INT: {
                    UIRenderer::DrawText(x + 6.0f, drawY + 3.0f, entry.label.c_str(),
                                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%d", entry.intVal);
                    f32 valW = w - VALUE_OFFSET - SCROLLBAR_WIDTH - 8.0f;
                    UIRenderer::DrawRect(x + VALUE_OFFSET, drawY + 2.0f, valW, ROW_HEIGHT - 4.0f,
                                         Vec4(0.16f, 0.16f, 0.18f, 1.0f));
                    UIRenderer::DrawText(x + VALUE_OFFSET + 4.0f, drawY + 3.0f, buf,
                                         Vec4(0.8f, 0.8f, 0.9f, 1.0f), 10.0f);
                    break;
                }

                case PropType::BOOL: {
                    UIRenderer::DrawText(x + 6.0f, drawY + 3.0f, entry.label.c_str(),
                                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);
                    f32 cbX = x + VALUE_OFFSET;
                    f32 cbY = drawY + (ROW_HEIGHT - CHECKBOX_SIZE) * 0.5f;
                    UIRenderer::DrawRect(cbX, cbY, CHECKBOX_SIZE, CHECKBOX_SIZE,
                                         Vec4(0.25f, 0.25f, 0.30f, 1.0f));
                    if (entry.boolVal) {
                        UIRenderer::DrawRect(cbX + 3.0f, cbY + 3.0f,
                                             CHECKBOX_SIZE - 6.0f, CHECKBOX_SIZE - 6.0f,
                                             Vec4(0.3f, 0.6f, 0.9f, 1.0f));
                    }
                    UIRenderer::DrawText(cbX + CHECKBOX_SIZE + 6.0f, drawY + 3.0f,
                                         entry.boolVal ? "True" : "False",
                                         entry.boolVal ? Vec4(0.4f, 0.8f, 1.0f, 1.0f)
                                                       : Vec4(0.5f, 0.5f, 0.5f, 1.0f), 10.0f);
                    break;
                }

                case PropType::STRING: {
                    UIRenderer::DrawText(x + 6.0f, drawY + 3.0f, entry.label.c_str(),
                                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);
                    f32 valW = w - VALUE_OFFSET - SCROLLBAR_WIDTH - 8.0f;
                    UIRenderer::DrawRect(x + VALUE_OFFSET, drawY + 2.0f, valW, ROW_HEIGHT - 4.0f,
                                         Vec4(0.16f, 0.16f, 0.18f, 1.0f));
                    std::string displayText = (i == m_textEditIndex) ? entry.stringVal + "|" : entry.stringVal;
                    Vec4 textColor = (i == m_textEditIndex) ? Vec4(1.0f, 1.0f, 1.0f, 1.0f)
                                                            : Vec4(0.8f, 0.8f, 0.9f, 1.0f);
                    UIRenderer::DrawText(x + VALUE_OFFSET + 4.0f, drawY + 3.0f,
                                         displayText.c_str(), textColor, 10.0f);
                    break;
                }

                case PropType::VEC3: {
                    UIRenderer::DrawText(x + 6.0f, drawY + 3.0f, entry.label.c_str(),
                                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);
                    f32 subW = (w - VALUE_OFFSET - SCROLLBAR_WIDTH - 8.0f) / 3.0f;
                    if (subW < 20.0f) subW = 20.0f;
                    f32 subX = x + VALUE_OFFSET;
                    f32 subY = drawY + 2.0f;
                    f32 subH = ROW_HEIGHT - 4.0f;

                    for (int si = 0; si < 3; ++si) {
                        f32 val = (si == 0) ? entry.vec3Val.x
                                : (si == 1) ? entry.vec3Val.y
                                : entry.vec3Val.z;
                        const char* subLabel = (si == 0) ? "X" : (si == 1) ? "Y" : "Z";
                        Vec4 subColor = (si == 0) ? Vec4(0.7f, 0.3f, 0.3f, 1.0f)
                                       : (si == 1) ? Vec4(0.3f, 0.7f, 0.3f, 1.0f)
                                       : Vec4(0.3f, 0.3f, 0.7f, 1.0f);

                        UIRenderer::DrawRect(subX, subY, subW, subH,
                                             Vec4(0.16f, 0.16f, 0.18f, 1.0f));
                        UIRenderer::DrawText(subX + 2.0f, subY + 1.0f, subLabel, subColor, 8.0f);
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.2f", val);
                        UIRenderer::DrawText(subX + 12.0f, subY + 1.0f, buf,
                                             Vec4(0.8f, 0.8f, 0.9f, 1.0f), 9.0f);
                        subX += subW + 4.0f;
                    }
                    break;
                }

                case PropType::ENUM: {
                    UIRenderer::DrawText(x + 6.0f, drawY + 3.0f, entry.label.c_str(),
                                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);
                    f32 valW = w - VALUE_OFFSET - SCROLLBAR_WIDTH - 8.0f;
                    UIRenderer::DrawRect(x + VALUE_OFFSET, drawY + 2.0f, valW, ROW_HEIGHT - 4.0f,
                                         Vec4(0.16f, 0.16f, 0.18f, 1.0f));
                    const char* enumText = (entry.enumVal >= 0 &&
                                            entry.enumVal < static_cast<i32>(entry.enumOptions.size()))
                                           ? entry.enumOptions[entry.enumVal].c_str() : "???";
                    UIRenderer::DrawText(x + VALUE_OFFSET + 4.0f, drawY + 3.0f, enumText,
                                         Vec4(0.8f, 0.8f, 0.9f, 1.0f), 10.0f);
                    f32 arrowX = x + w - SCROLLBAR_WIDTH - 16.0f;
                    UIRenderer::DrawText(arrowX, drawY + 3.0f, "v",
                                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 8.0f);
                    break;
                }

                case PropType::SLIDER: {
                    UIRenderer::DrawText(x + 6.0f, drawY + 3.0f, entry.label.c_str(),
                                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);
                    f32 trackX = x + VALUE_OFFSET;
                    f32 trackW = w - VALUE_OFFSET - SCROLLBAR_WIDTH - 8.0f;
                    f32 trackY = drawY + (ROW_HEIGHT - TRACK_HEIGHT) * 0.5f;

                    UIRenderer::DrawRect(trackX, trackY, trackW, TRACK_HEIGHT,
                                         Vec4(0.16f, 0.16f, 0.20f, 1.0f));

                    f32 range = entry.floatMax - entry.floatMin;
                    f32 t = (range > 0.0f) ? (entry.floatVal - entry.floatMin) / range : 0.0f;
                    if (t < 0) t = 0.0f;
                    if (t > 1) t = 1.0f;

                    f32 fillW = t * trackW;
                    UIRenderer::DrawRect(trackX, trackY, fillW, TRACK_HEIGHT,
                                         Vec4(0.42f, 0.42f, 0.52f, 1.0f));

                    f32 handleX = trackX + t * (trackW - HANDLE_SIZE);
                    f32 handleY = drawY + (ROW_HEIGHT - HANDLE_SIZE - 2.0f) * 0.5f;
                    UIRenderer::DrawRect(handleX, handleY, HANDLE_SIZE, HANDLE_SIZE + 2.0f,
                                         Vec4(0.85f, 0.85f, 0.90f, 1.0f));

                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.2f", entry.floatVal);
                    UIRenderer::DrawText(trackX + trackW + 4.0f, drawY + 3.0f, buf,
                                         Vec4(0.7f, 0.7f, 0.8f, 1.0f), 9.0f);
                    break;
                }

                case PropType::LABEL: {
                    UIRenderer::DrawText(x + 6.0f, drawY + 3.0f, entry.label.c_str(),
                                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);
                    UIRenderer::DrawText(x + VALUE_OFFSET + 4.0f, drawY + 3.0f,
                                         entry.stringVal.c_str(),
                                         Vec4(0.7f, 0.7f, 0.8f, 1.0f), 10.0f);
                    break;
                }
            }
        }

        drawY += rowH;
    }

    // Scrollbar
    if (contentH > viewH) {
        f32 sbX = x + w - SCROLLBAR_WIDTH;
        f32 sbTrackH = viewH;
        f32 scrollH = sbTrackH * sbTrackH / contentH;
        f32 scrollT = (maxScroll > 0) ? m_scrollOffset / maxScroll : 0.0f;
        f32 scrollY = y + scrollT * (sbTrackH - scrollH);

        UIRenderer::DrawRect(sbX, y, SCROLLBAR_WIDTH, sbTrackH,
                             Vec4(0.15f, 0.15f, 0.17f, 1.0f));
        UIRenderer::DrawRect(sbX, scrollY, SCROLLBAR_WIDTH, scrollH,
                             Vec4(0.35f, 0.35f, 0.40f, 1.0f));
    }
}

// ==========================================
// UIFILEBROWSER IMPLEMENTATION
// ==========================================

namespace fs = std::filesystem;

UIFileBrowser::UIFileBrowser(const std::string& name)
    : UIWidget(name) {
    m_size = Vec2(300.0f, 400.0f);
}

void UIFileBrowser::SetRootDirectory(const std::string& path) {
    m_rootDir = path;
    if (m_currentDir.empty()) {
        m_currentDir = path;
    }
    m_needsRefresh = true;
}

void UIFileBrowser::NavigateTo(const std::string& path) {
    fs::path target;
    if (path.empty()) return;

    if (path == "..") {
        // Go up one level (but not past root)
        fs::path parent = fs::path(m_currentDir).parent_path();
        std::string parentStr = parent.string();
        if (parentStr.size() >= m_rootDir.size()) {
            m_currentDir = parentStr;
            m_needsRefresh = true;
        }
        return;
    }

    // Try as relative to current dir
    target = fs::path(m_currentDir) / path;
    std::string targetStr = target.string();

    // If doesn't exist, try as absolute
    if (!fs::exists(target)) {
        target = fs::path(path);
        if (!fs::exists(target)) return;
        targetStr = path;
    }

    if (fs::is_directory(target)) {
        m_currentDir = targetStr;
        m_needsRefresh = true;
    }
}

void UIFileBrowser::SetFileFilter(const std::string& extension) {
    m_fileFilter = extension;
    m_needsRefresh = true;
}

void UIFileBrowser::Refresh() {
    m_needsRefresh = true;
}

void UIFileBrowser::ScanDirectory() {
    m_files.clear();
    m_selectedIndex = -1;
    ClearThumbnailCache();

    if (!fs::exists(m_currentDir) || !fs::is_directory(m_currentDir)) return;

    try {
        // Add parent-directory entry unless at root
        fs::path current(m_currentDir);
        fs::path root(m_rootDir);
        std::string currentStr = current.lexically_normal().string();
        std::string rootStr = root.lexically_normal().string();
        if (currentStr.size() > rootStr.size() && currentStr.find(rootStr) == 0) {
            FileEntry parent;
            parent.name = "..";
            parent.path = current.parent_path().string();
            parent.isDirectory = true;
            m_files.push_back(parent);
        }

        // Scan directory entries
        for (const auto& entry : fs::directory_iterator(m_currentDir)) {
            FileEntry fe;
            fe.name = entry.path().filename().string();
            fe.path = entry.path().string();
            fe.isDirectory = entry.is_directory();

            if (fe.isDirectory) {
                // Always include directories
                m_files.push_back(fe);
            } else if (entry.is_regular_file()) {
                // Apply extension filter
                if (m_fileFilter.empty() ||
                    entry.path().extension().string() == m_fileFilter) {
                    fe.fileSize = static_cast<u64>(entry.file_size());
                    m_files.push_back(fe);
                }
            }
        }
    } catch (const std::exception& e) {
        DND_LOG_ERROR("UIFileBrowser::ScanDirectory failed: %s", e.what());
    }

    // Sort: directories first, then alphabetical
    std::sort(m_files.begin(), m_files.end(),
        [](const FileEntry& a, const FileEntry& b) {
            if (a.isDirectory != b.isDirectory) {
                return a.isDirectory > b.isDirectory; // dirs first
            }
            // Case-insensitive sort
            std::string al = a.name;
            std::string bl = b.name;
            for (auto& c : al) c = static_cast<char>(std::tolower(c));
            for (auto& c : bl) c = static_cast<char>(std::tolower(c));
            return al < bl;
        });

    m_scrollOffset = 0.0f;
}

void UIFileBrowser::Update(f32 deltaTime) {
    if (!m_visible) return;

    if (m_needsRefresh) {
        m_needsRefresh = false;
        ScanDirectory();
    }

    // Clamp scroll
    f32 contentH = static_cast<f32>(m_files.size()) * ROW_HEIGHT;
    f32 viewH = m_size.y - PATH_BAR_HEIGHT;
    f32 maxScroll = std::max(0.0f, contentH - viewH);
    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
    if (m_scrollOffset < 0) m_scrollOffset = 0;

    // Mouse wheel scroll
    f32 scrollDelta = Input::GetScrollDelta();
    if (scrollDelta != 0.0f) {
        UIRect bounds(m_position.x, m_position.y + PATH_BAR_HEIGHT,
                      m_size.x, m_size.y - PATH_BAR_HEIGHT);
        f32 mx = Input::GetMouseX();
        f32 my = Input::GetMouseY();
        if (bounds.Contains(mx, my)) {
            m_scrollOffset -= scrollDelta * 40.0f;
        }
    }

    // Track time for double-click
    if (m_lastClickTime > 0.0f) {
        m_lastClickTime -= deltaTime;
        if (m_lastClickTime < 0.0f) m_lastClickTime = 0.0f;
    }

    // Drag detection: if mouse is held on a file item, check threshold
    if (m_dragPending && m_dragStartCallback && m_dragItemIndex >= 0 && Input::IsMouseButtonDown(MouseButton::LEFT)) {
        f32 dx = Input::GetMouseX() - m_dragStartX;
        f32 dy = Input::GetMouseY() - m_dragStartY;
        if (dx * dx + dy * dy >= 16.0f) { // 4px threshold squared
            if (m_dragItemIndex < static_cast<i32>(m_files.size())) {
                m_dragStartCallback(m_files[m_dragItemIndex].path, Input::GetMouseX(), Input::GetMouseY());
            }
            m_dragPending = false;
        }
    }

} // UIFileBrowser::Update

b8 UIFileBrowser::HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) {
    if (!m_visible) return false;

    UIRect bounds(m_position.x, m_position.y, m_size.x, m_size.y);
    if (!bounds.Contains(mouseX, mouseY)) return false;

    if (!mouseDown) {
        m_wasMouseDown = false;
        m_dragPending = false;
        return true; // still consuming within bounds
    }

    if (m_wasMouseDown) return true; // already processed this press
    m_wasMouseDown = true;

    // Check path bar click (go up button)
    UIRect upBtn(m_position.x + 4.0f, m_position.y + 2.0f,
                 20.0f, PATH_BAR_HEIGHT - 4.0f);
    if (upBtn.Contains(mouseX, mouseY)) {
        NavigateTo("..");
        return true;
    }

    // Check file list area
    f32 listY = m_position.y + PATH_BAR_HEIGHT;
    if (mouseY < listY) return true;

    i32 hitIndex = HitTestRow(mouseX, mouseY);
    if (hitIndex < 0 || hitIndex >= static_cast<i32>(m_files.size())) return true;

    const FileEntry& entry = m_files[hitIndex];

    // Double-click detection (within 300ms)
    bool isDoubleClick = (hitIndex == m_lastClickIndex && m_lastClickTime > 0.0f);

    m_lastClickIndex = hitIndex;
    m_lastClickTime = 0.3f; // 300ms double-click window

    if (entry.isDirectory) {
        NavigateTo(entry.name);
        if (m_dirCallback) m_dirCallback(entry.path);
        return true;
    }

    // Single-click on file
    m_selectedIndex = hitIndex;

    if (isDoubleClick) {
        // Double-click on file: fire callback
        if (m_fileCallback) m_fileCallback(entry.path);
        m_lastClickTime = 0.0f;
    }

    // Prepare for potential drag initiation on file items (not directories)
    if (!entry.isDirectory) {
        m_dragPending = true;
        m_dragStartX = mouseX;
        m_dragStartY = mouseY;
        m_dragItemIndex = hitIndex;
    }

    return true;
}

i32 UIFileBrowser::HitTestRow(f32 mx, f32 my) const {
    f32 listY = m_position.y + PATH_BAR_HEIGHT;
    f32 relY = my - (listY - m_scrollOffset);
    if (relY < 0) return -1;
    i32 row = static_cast<i32>(relY / ROW_HEIGHT);
    if (row >= 0 && row < static_cast<i32>(m_files.size())) return row;
    return -1;
}

void UIFileBrowser::DrawIcon(f32 x, f32 y, const FileEntry& entry) {
    if (entry.isDirectory) {
        UIRenderer::DrawRect(x, y, ICON_SIZE, ICON_SIZE, Vec4(0.85f, 0.75f, 0.30f, 1.0f));
        return;
    }
    std::string ext;
    size_t dot = entry.name.find_last_of('.');
    if (dot != std::string::npos) {
        ext = entry.name.substr(dot);
        for (auto& c : ext) c = static_cast<char>(std::tolower(c));
    }
    bool isImage = (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga");
    if (isImage) {
        u32 texId = GetOrCreateThumbnail(entry.path);
        if (texId != 0) {
            UIRenderer::DrawTexturedRect(x, y, ICON_SIZE, ICON_SIZE, texId);
            return;
        }
    }
    // Fallback: colored rect by extension
    Vec4 color(0.45f, 0.55f, 0.70f, 1.0f);
    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
        color = Vec4(0.3f, 0.7f, 0.5f, 1.0f);
    } else if (isImage) {
        color = Vec4(0.8f, 0.4f, 0.6f, 1.0f);
    } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
        color = Vec4(0.6f, 0.4f, 0.8f, 1.0f);
    } else if (ext == ".hlsl" || ext == ".glsl" || ext == ".vert" || ext == ".frag") {
        color = Vec4(0.5f, 0.7f, 0.9f, 1.0f);
    } else if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".cs") {
        color = Vec4(0.4f, 0.8f, 0.4f, 1.0f);
    } else if (ext == ".txt" || ext == ".md" || ext == ".json" || ext == ".xml" || ext == ".yaml" || ext == ".yml") {
        color = Vec4(0.7f, 0.7f, 0.5f, 1.0f);
    }
    UIRenderer::DrawRect(x, y, ICON_SIZE, ICON_SIZE, color);
}

u32 UIFileBrowser::GetOrCreateThumbnail(const std::string& path) {
    auto it = m_thumbnailCache.find(path);
    if (it != m_thumbnailCache.end()) {
        return it->second;
    }
    AssetHandle h = AssetManager::Get().LoadTexture(path.c_str());
    Texture2D* tex = AssetManager::Get().GetTexture(h);
    if (!tex || !tex->IsLoaded()) {
        m_thumbnailCache[path] = 0;
        return 0;
    }
    u32 glId = tex->GetGLId();
    m_thumbnailCache[path] = glId;
    return glId;
}

void UIFileBrowser::ClearThumbnailCache() {
    m_thumbnailCache.clear();
}

void UIFileBrowser::Render(const Mat4& projMatrix) {
    DND_UNUSED(projMatrix);
    if (!m_visible) return;

    f32 x = m_position.x;
    f32 y = m_position.y;
    f32 w = m_size.x;
    f32 h = m_size.y;

    // Background
    UIRenderer::DrawRect(x, y, w, h, Vec4(0.12f, 0.12f, 0.14f, 1.0f));

    // Path bar background
    UIRenderer::DrawRect(x, y, w, PATH_BAR_HEIGHT, Vec4(0.16f, 0.16f, 0.18f, 1.0f));

    // Up button (arrow)
    UIRenderer::DrawRect(x + 4.0f, y + 2.0f, 20.0f, PATH_BAR_HEIGHT - 4.0f,
                         Vec4(0.20f, 0.20f, 0.24f, 1.0f));
    UIRenderer::DrawText(x + 8.0f, y + 3.0f, "^", Vec4(0.7f, 0.7f, 0.8f, 1.0f), 10.0f);

    // Current directory path text (truncated to fit)
    std::string dirDisplay = m_currentDir;
    f32 maxDirW = w - 40.0f;
    f32 dirTextW = static_cast<f32>(dirDisplay.size()) * 6.0f; // ~6px per char at 10px size
    (void)maxDirW; // suppress unused warning; we still draw it
    UIRenderer::DrawText(x + 28.0f, y + 3.0f, dirDisplay.c_str(),
                         Vec4(0.6f, 0.6f, 0.7f, 1.0f), 10.0f);

    // File list area
    f32 listX = x;
    f32 listY = y + PATH_BAR_HEIGHT;
    f32 listW = w - SCROLLBAR_WIDTH;
    f32 listH = h - PATH_BAR_HEIGHT;

    // Header row
    UIRenderer::DrawRect(listX, listY, w, ROW_HEIGHT, Vec4(0.16f, 0.16f, 0.18f, 1.0f));
    UIRenderer::DrawText(listX + ICON_SIZE + 8.0f, listY + 3.0f, "Name",
                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);
    UIRenderer::DrawText(listX + w - SCROLLBAR_WIDTH - SIZE_COLUMN_WIDTH + 4.0f, listY + 3.0f, "Size",
                         Vec4(0.5f, 0.5f, 0.6f, 1.0f), 10.0f);

    // Draw visible entries
    f32 firstY = listY + ROW_HEIGHT;
    f32 totalH = static_cast<f32>(m_files.size()) * ROW_HEIGHT;
    f32 maxScroll = std::max(0.0f, totalH - (listH - ROW_HEIGHT));
    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
    if (m_scrollOffset < 0) m_scrollOffset = 0;

    i32 firstRow = static_cast<i32>(m_scrollOffset / ROW_HEIGHT);
    i32 lastRow = std::min(static_cast<i32>((m_scrollOffset + listH - ROW_HEIGHT) / ROW_HEIGHT) + 1,
                           static_cast<i32>(m_files.size()));

    for (i32 i = firstRow; i < lastRow; ++i) {
        const FileEntry& entry = m_files[i];
        f32 rowY = firstY + static_cast<f32>(i) * ROW_HEIGHT - m_scrollOffset;

        if (rowY + ROW_HEIGHT < listY || rowY > listY + listH) continue;

        bool isSelected = (i == m_selectedIndex);

        // Row background
        if (isSelected) {
            UIRenderer::DrawRect(listX + 1.0f, rowY, listW - 2.0f, ROW_HEIGHT,
                                 Vec4(0.25f, 0.35f, 0.50f, 1.0f));
        } else if (i % 2 == 1) {
            UIRenderer::DrawRect(listX + 1.0f, rowY, listW - 2.0f, ROW_HEIGHT,
                                 Vec4(0.13f, 0.13f, 0.15f, 1.0f));
        }

        // Icon
        f32 iconX = listX + 4.0f;
        f32 iconY = rowY + (ROW_HEIGHT - ICON_SIZE) * 0.5f;
        DrawIcon(iconX, iconY, entry);

        // Name
        f32 nameX = listX + ICON_SIZE + 10.0f;
        f32 nameMaxW = listW - ICON_SIZE - SIZE_COLUMN_WIDTH - 20.0f;
        std::string displayName = entry.name;
        // Truncate if too long
        if (static_cast<f32>(displayName.size()) * 6.0f > nameMaxW && nameMaxW > 20.0f) {
            i32 maxChars = static_cast<i32>(nameMaxW / 6.0f) - 3;
            if (maxChars > 3) {
                displayName = displayName.substr(0, static_cast<size_t>(maxChars)) + "...";
            }
        }
        (void)nameMaxW;
        UIRenderer::DrawText(nameX, rowY + 3.0f, displayName.c_str(),
                             Vec4(0.8f, 0.8f, 0.9f, 1.0f), 10.0f);

        // Size (files only)
        if (!entry.isDirectory && entry.fileSize > 0) {
            char sizeBuf[32];
            if (entry.fileSize < 1024) {
                snprintf(sizeBuf, sizeof(sizeBuf), "%llu B", (unsigned long long)entry.fileSize);
            } else if (entry.fileSize < 1024 * 1024) {
                snprintf(sizeBuf, sizeof(sizeBuf), "%.1f KB", entry.fileSize / 1024.0f);
            } else {
                snprintf(sizeBuf, sizeof(sizeBuf), "%.1f MB", entry.fileSize / (1024.0f * 1024.0f));
            }
            f32 sizeX = listX + w - SCROLLBAR_WIDTH - SIZE_COLUMN_WIDTH + 4.0f;
            UIRenderer::DrawText(sizeX, rowY + 3.0f, sizeBuf,
                                 Vec4(0.5f, 0.5f, 0.6f, 1.0f), 9.0f);
        }
    }

    // Scrollbar
    if (totalH > listH - ROW_HEIGHT) {
        f32 sbX = x + w - SCROLLBAR_WIDTH;
        f32 sbY = listY + ROW_HEIGHT;
        f32 sbH = listH - ROW_HEIGHT;
        f32 scrollH = sbH * sbH / totalH;
        f32 scrollT = (maxScroll > 0) ? m_scrollOffset / maxScroll : 0.0f;
        f32 scrollY = sbY + scrollT * (sbH - scrollH);

        UIRenderer::DrawRect(sbX, sbY, SCROLLBAR_WIDTH, sbH,
                             Vec4(0.15f, 0.15f, 0.17f, 1.0f));
        UIRenderer::DrawRect(sbX, scrollY, SCROLLBAR_WIDTH, scrollH,
                             Vec4(0.35f, 0.35f, 0.40f, 1.0f));
    }
}

} // namespace Dnd
