#include "ui/drag_drop.h"
#include "ui/ui_widget.h"
#include "ui/ui_renderer.h"

namespace Dnd {

DragDropManager* DragDropManager::s_instance = nullptr;

DragDropManager::DragDropManager() {}

DragDropManager::~DragDropManager() {}

DragDropManager& DragDropManager::Get() {
    if (!s_instance) s_instance = new DragDropManager();
    return *s_instance;
}

void DragDropManager::BeginDrag(UIWidget* source, const DragPayload& payload, f32 mouseX, f32 mouseY) {
    m_session.source = source;
    m_session.payload = payload;
    m_session.startX = mouseX;
    m_session.startY = mouseY;
    m_session.active = false;
    m_mouseX = mouseX;
    m_mouseY = mouseY;
    m_hoveredZone = nullptr;
}

void DragDropManager::Update(f32 mouseX, f32 mouseY, b8 mouseDown) {
    m_mouseX = mouseX;
    m_mouseY = mouseY;

    if (!m_session.source) return;

    if (!m_session.active) {
        if (!mouseDown) {
            m_session.source = nullptr;
            m_hoveredZone = nullptr;
            return;
        }
        f32 dx = mouseX - m_session.startX;
        f32 dy = mouseY - m_session.startY;
        if (dx * dx + dy * dy >= kDragThreshold * kDragThreshold) {
            m_session.active = true;
        }
        return;
    }

    if (!mouseDown) {
        DropZone* hitZone = nullptr;
        for (auto it = m_dropZones.rbegin(); it != m_dropZones.rend(); ++it) {
            if (it->widget && !it->widget->IsVisible()) continue;
            if (it->area.Contains(mouseX, mouseY) && it->canAccept(m_session.payload)) {
                hitZone = &(*it);
                break;
            }
        }
        if (hitZone) {
            bool consumed = (hitZone->widget && hitZone->widget->OnDrop(m_session.payload));
            if (!consumed) {
                hitZone->onDrop(m_session.payload);
            }
        }
        m_session.source = nullptr;
        m_session.active = false;
        m_hoveredZone = nullptr;
        return;
    }

    m_hoveredZone = nullptr;
    for (auto it = m_dropZones.rbegin(); it != m_dropZones.rend(); ++it) {
        if (it->widget && !it->widget->IsVisible()) continue;
        if (it->area.Contains(mouseX, mouseY) && it->canAccept(m_session.payload)) {
            m_hoveredZone = it->widget;
            break;
        }
    }
}

void DragDropManager::CancelDrag() {
    m_session.source = nullptr;
    m_session.active = false;
    m_hoveredZone = nullptr;
}

b8 DragDropManager::IsDragging() const {
    return m_session.source != nullptr;
}

b8 DragDropManager::IsDragActive() const {
    return m_session.source != nullptr && m_session.active;
}

const DragPayload* DragDropManager::GetActivePayload() const {
    return IsDragging() ? &m_session.payload : nullptr;
}

UIWidget* DragDropManager::GetDragSource() const {
    return m_session.source;
}

UIWidget* DragDropManager::GetHoveredZone() const {
    return m_hoveredZone;
}

f32 DragDropManager::GetMouseX() const { return m_mouseX; }
f32 DragDropManager::GetMouseY() const { return m_mouseY; }

void DragDropManager::RegisterDropZone(const DropZone& zone) {
    m_dropZones.push_back(zone);
}

void DragDropManager::UnregisterDropZone(UIWidget* widget) {
    for (auto it = m_dropZones.begin(); it != m_dropZones.end(); ) {
        if (it->widget == widget) it = m_dropZones.erase(it);
        else ++it;
    }
}

void DragDropManager::ClearDropZones() {
    m_dropZones.clear();
}

void DragDropManager::RenderGhost(const Mat4& projMatrix) {
    if (!IsDragActive()) return;

    const DragPayload& p = m_session.payload;
    f32 ghostX = m_mouseX + 12.0f;
    f32 ghostY = m_mouseY + 8.0f;
    f32 ghostW = 160.0f;
    f32 ghostH = 20.0f;
    const char* text = p.label ? p.label : "Drag";

    Vec4 bg(0.9f, 0.95f, 1.0f, 0.85f);
    Vec4 textColor(0.1f, 0.15f, 0.25f, 1.0f);

    UIRenderer::DrawRect(ghostX, ghostY, ghostW, ghostH, bg);
    UIRenderer::DrawText(ghostX + 4.0f, ghostY + 2.0f, text, textColor, 12.0f);

    // Draw hovered zone highlight
    if (m_hoveredZone) {
        Vec2 pos = m_hoveredZone->GetPosition();
        Vec2 sz = m_hoveredZone->GetSize();
        Vec4 highlight(0.4f, 0.7f, 1.0f, 0.25f);
        UIRenderer::DrawRect(pos.x, pos.y, sz.x, sz.y, highlight);
    }
}

} // namespace Dnd
