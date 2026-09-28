#pragma once

#include "core/types.h"
#include <string>
#include <functional>

namespace Dnd {

class DockPanel;
class UIFileBrowser;

/**
 * @class AssetBrowserPanel
 * @brief Editor panel for browsing project assets.
 *
 * Wraps a UIFileBrowser inside a DockPanel and points it at the
 * project's Assets/ directory. Provides file-selection callbacks
 * for future preview/import functionality.
 */
class AssetBrowserPanel {
public:
    AssetBrowserPanel();
    ~AssetBrowserPanel();

    /// @brief Gets the underlying DockPanel (for DockManager registration).
    DockPanel* GetDockPanel() const { return m_dockPanel; }

    /// @brief Gets the file browser widget.
    UIFileBrowser* GetFileBrowser() const { return m_browser; }

    /// @brief Refreshes the file listing.
    void Refresh();

    /// @brief Sets a callback that fires when a file is double-clicked for spawning.
    using SpawnCallback = std::function<void(const std::string& path)>;
    void SetSpawnCallback(SpawnCallback cb) { m_spawnCallback = cb; }

private:
    DockPanel*     m_dockPanel;
    UIFileBrowser* m_browser;
    SpawnCallback  m_spawnCallback;
};

} // namespace Dnd
