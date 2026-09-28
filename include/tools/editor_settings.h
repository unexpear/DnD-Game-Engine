#pragma once

#include "core/types.h"
#include "ecs/entity.h"

namespace Dnd {

enum class GizmoMode : i32 {
    Translate,
    Rotate,
    Scale
};

class EditorSettings {
public:
    static EditorSettings& Get();

    b8 editorActive = false;
    Entity selectedEntity = Entity::None();
    GizmoMode gizmoMode = GizmoMode::Translate;
    b8 showGrid = true;
    b8 showColliders = true;

private:
    EditorSettings() = default;
    static EditorSettings* s_instance;
};

} // namespace Dnd
