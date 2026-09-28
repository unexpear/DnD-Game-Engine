#include "tools/editor_settings.h"

namespace Dnd {

EditorSettings* EditorSettings::s_instance = nullptr;

EditorSettings& EditorSettings::Get() {
    if (!s_instance) {
        s_instance = new EditorSettings();
    }
    return *s_instance;
}

} // namespace Dnd
