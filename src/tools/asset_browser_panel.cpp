#include "tools/asset_browser_panel.h"
#include "ui/ui_widget.h"
#include "ui/drag_drop.h"
#include "core/log.h"

namespace Dnd {

AssetBrowserPanel::AssetBrowserPanel() {
    m_browser = new UIFileBrowser("AssetBrowserFileList");
    m_browser->SetRootDirectory("./Assets");
    m_browser->SetFileFilter(""); // show all
    m_browser->NavigateTo("./Assets");
    m_browser->SetFileCallback([](const std::string& path) {
        DND_LOG_INFO("Asset selected: %s", path.c_str());
    });
    m_browser->SetDirCallback([this](const std::string& path) {
        DND_LOG_DEBUG("Navigated to: %s", path.c_str());
    });
    // Override the file callback - on double-click, call the spawn callback for supported types
    m_browser->SetFileCallback([this](const std::string& path) {
        std::string ext;
        size_t dot = path.find_last_of('.');
        if (dot != std::string::npos) {
            ext = path.substr(dot);
            for (auto& c : ext) c = static_cast<char>(std::tolower(c));
        }
        bool isSpawnable = (ext == ".obj" || ext == ".png" || ext == ".jpg" || ext == ".jpeg");
        if (isSpawnable && m_spawnCallback) {
            m_spawnCallback(path);
        }
    });
    m_browser->SetDragStartCallback([](const std::string& path, f32 mouseX, f32 mouseY) {
        // Extract filename for the ghost label
        const char* label = "File";
        size_t sep = path.find_last_of("/\\");
        std::string filename = (sep != std::string::npos) ? path.substr(sep + 1) : path;
        // Determine asset type from extension
        std::string ext;
        size_t dot = filename.find_last_of('.');
        if (dot != std::string::npos) {
            ext = filename.substr(dot);
            for (auto& c : ext) c = static_cast<char>(std::tolower(c));
        }
        DragPayload payload;
        payload.type = DragPayloadType::STRING;
        payload.label = filename.c_str();
        static std::string s_dragPath;
        s_dragPath = path;
        payload.stringVal = s_dragPath.c_str();
        DragDropManager::Get().BeginDrag(nullptr, payload, mouseX, mouseY);
    });

    m_dockPanel = new DockPanel("AssetBrowser", "Asset Browser");
    m_dockPanel->SetContent(m_browser);
    m_dockPanel->SetCloseCallback([this]() {
        m_dockPanel->SetVisible(false);
    });
}

AssetBrowserPanel::~AssetBrowserPanel() {
}

void AssetBrowserPanel::Refresh() {
    m_browser->Refresh();
}

} // namespace Dnd
