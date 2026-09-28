#pragma once

#include "core/types.h"
#include "core/math.h"
#include "assets/asset_manager.h"
#include <functional>
#include <vector>
#include <string>

namespace Dnd {

class UIWidget;

enum class DragPayloadType : i32 {
    ASSET_HANDLE,
    ENTITY_ID,
    COLOR,
    FLOAT,
    INT,
    STRING
};

struct DragPayload {
    DragPayloadType type;
    const char* label;
    union {
        AssetHandle assetHandle;
        u32 entityId;
        Vec4 color;
        f32 floatVal;
        i32 intVal;
        const char* stringVal;
    };

    DragPayload() : type(DragPayloadType::FLOAT), label(""), floatVal(0.0f) {}
};

struct DropZoneRect {
    f32 x, y, w, h;
    DropZoneRect() : x(0), y(0), w(0), h(0) {}
    DropZoneRect(f32 px, f32 py, f32 pw, f32 ph) : x(px), y(py), w(pw), h(ph) {}
    bool Contains(f32 px, f32 py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

struct DropZone {
    UIWidget* widget;
    DropZoneRect area;
    std::function<bool(const DragPayload&)> canAccept;
    std::function<void(const DragPayload&)> onDrop;
};

class DragDropManager {
public:
    static DragDropManager& Get();

    void BeginDrag(UIWidget* source, const DragPayload& payload, f32 mouseX, f32 mouseY);
    void Update(f32 mouseX, f32 mouseY, b8 mouseDown);
    void CancelDrag();

    b8 IsDragging() const;
    b8 IsDragActive() const;
    const DragPayload* GetActivePayload() const;
    UIWidget* GetDragSource() const;
    UIWidget* GetHoveredZone() const;
    f32 GetMouseX() const;
    f32 GetMouseY() const;

    void RegisterDropZone(const DropZone& zone);
    void UnregisterDropZone(UIWidget* widget);
    void ClearDropZones();

    void RenderGhost(const Mat4& projMatrix);

private:
    DragDropManager();
    ~DragDropManager();

    struct DragSession {
        UIWidget* source = nullptr;
        DragPayload payload;
        f32 startX = 0, startY = 0;
        b8 active = false;
    };

    DragSession m_session;
    std::vector<DropZone> m_dropZones;
    f32 m_mouseX = 0, m_mouseY = 0;
    UIWidget* m_hoveredZone = nullptr;

    static DragDropManager* s_instance;
    static constexpr f32 kDragThreshold = 4.0f;
};

} // namespace Dnd
