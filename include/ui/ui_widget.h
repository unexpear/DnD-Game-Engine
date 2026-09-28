/**
 * @file ui_widget.h
 * @brief Base UI widget class and common widget types.
 * 
 * The UI system provides a simple retained-mode GUI for
 * game menus, HUDs, and debug overlays.
 * 
 * Widget types:
 *   - Label: Text display
 *   - Button: Clickable with callback
 *   - Panel: Container with background
 *   - Image: Texture display
 *   - Slider: Draggable value control
 *   - DockPanel: Docking panel with title bar, close, resize, tabs
 *   - UITreeView: Hierarchical tree with expand/collapse, search, selection
 *   - UIPropertyGrid: Dynamic property editor with typed fields (float, int, bool, string, Vec3, enum, slider)
 *   - UIFileBrowser: Directory/file listing with navigation, path bar, file filter
 * 
 * All widgets have a position (screen space), size, and visibility.
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.8.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace Dnd {

struct DragPayload;

// ==========================================
// ENUM: UIAnchor
// ==========================================

/**
 * @enum UIAnchor
 * @brief Anchor point for widget positioning.
 */
enum class UIAnchor : i32 {
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    MIDDLE_LEFT,
    MIDDLE_CENTER,
    MIDDLE_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
};

// ==========================================
// STRUCT: UIRect
// ==========================================

/**
 * @struct UIRect
 * @brief Rectangle in screen coordinates.
 */
struct UIRect {
    f32 x, y;         ///< Position (screen pixels)
    f32 width, height; ///< Dimensions (screen pixels)

    /// @brief Default constructor.
    UIRect() : x(0), y(0), width(0), height(0) {}

    /**
     * @brief Constructs a UIRect.
     * @param px X position.
     * @param py Y position.
     * @param w Width.
     * @param h Height.
     */
    UIRect(f32 px, f32 py, f32 w, f32 h) : x(px), y(py), width(w), height(h) {}

    /// @brief Checks if a point is inside the rect.
    bool Contains(f32 px, f32 py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
};

// ==========================================
// CLASS: UIWidget (Abstract Base)
// ==========================================

/**
 * @class UIWidget
 * @brief Base class for all UI widgets.
 * 
 * All UI widgets derive from this class.
 * Widgets are owned by the UISystem (not ECS components).
 */
class UIWidget {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~UIWidget() = default;

    /**
     * @brief Updates the widget (called each frame).
     * @param deltaTime Frame delta time.
     */
    virtual void Update(f32 deltaTime) { DND_UNUSED(deltaTime); }

    /**
     * @brief Renders the widget.
     * @param projMatrix Orthographic projection matrix.
     */
    virtual void Render(const Mat4& projMatrix) = 0;

    /**
     * @brief Handles mouse input.
     * @param mouseX Mouse X position (screen pixels).
     * @param mouseY Mouse Y position (screen pixels).
     * @param mouseDown Whether mouse button is pressed.
     * @return true if the event was consumed.
     */
    virtual b8 HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) { DND_UNUSED(mouseX); DND_UNUSED(mouseY); DND_UNUSED(mouseDown); return false; }

    /**
     * @brief Whether this widget accepts a drag-drop payload.
     * @param payload The current drag payload.
     * @return true if this widget can accept the drop.
     */
    virtual b8 CanAcceptDrop(const DragPayload& payload) { DND_UNUSED(payload); return false; }

    /**
     * @brief Called when a drag payload is dropped on this widget.
     * @param payload The dropped payload.
     * @return true if the drop was handled.
     */
    virtual b8 OnDrop(const DragPayload& payload) { DND_UNUSED(payload); return false; }

    /// @brief Gets the widget position.
    Vec2 GetPosition() const { return m_position; }

    /// @brief Sets the widget position.
    void SetPosition(const Vec2& pos) { m_position = pos; }

    /// @brief Gets the widget size.
    Vec2 GetSize() const { return m_size; }

    /// @brief Sets the widget size.
    void SetSize(const Vec2& size) { m_size = size; }

    /// @brief Checks if widget is visible.
    b8 IsVisible() const { return m_visible; }

    /// @brief Sets widget visibility.
    void SetVisible(b8 visible) { m_visible = visible; }

    /// @brief Gets the widget name (for debugging).
    const char* GetName() const { return m_name.c_str(); }

protected:
    /**
     * @brief Protected constructor.
     * @param name Widget name (for debugging).
     */
    explicit UIWidget(const std::string& name)
        : m_position(0, 0), m_size(100, 30), m_visible(true), m_name(name) {}

    Vec2 m_position;    ///< Screen position (pixels)
    Vec2 m_size;        ///< Widget dimensions (pixels)
    b8 m_visible;       ///< Whether widget is visible
    std::string m_name;  ///< Widget name
};

// ==========================================
// CLASS: UILabel
// ==========================================

/**
 * @class UILabel
 * @brief Text display widget.
 */
class UILabel : public UIWidget {
public:
    /**
     * @brief Constructs a label.
     * @param name Widget name.
     * @param text Label text.
     */
    explicit UILabel(const std::string& name, const std::string& text = "");

    void Render(const Mat4& projMatrix) override;

    /// @brief Gets the label text.
    const char* GetText() const { return m_text.c_str(); }

    /// @brief Sets the label text.
    void SetText(const std::string& text) { m_text = text; }

    /// @brief Gets the text color.
    Vec4 GetColor() const { return m_color; }

    /// @brief Sets the text color.
    void SetColor(const Vec4& color) { m_color = color; }

private:
    std::string m_text;  ///< Label text
    Vec4 m_color;          ///< Text color (RGBA)
};

// ==========================================
// CLASS: UIButton
// ==========================================

/**
 * @class UIButton
 * @brief Clickable button widget.
 */
class UIButton : public UIWidget {
public:
    /**
     * @brief Button click callback type.
     */
    using ClickCallback = std::function<void()>;

    /**
     * @brief Constructs a button.
     * @param name Widget name.
     * @param text Button text.
     */
    explicit UIButton(const std::string& name, const std::string& text = "Button");

    b8 HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) override;
    void Render(const Mat4& projMatrix) override;

    /// @brief Gets the button text.
    const char* GetText() const { return m_text.c_str(); }

    /// @brief Sets the button text.
    void SetText(const std::string& text) { m_text = text; }

    /// @brief Sets the click callback.
    void SetCallback(ClickCallback cb) { m_callback = cb; }

    /// @brief Gets the button color.
    Vec4 GetColor() const { return m_color; }

    /// @brief Sets the button color.
    void SetColor(const Vec4& color) { m_color = color; }

private:
    std::string m_text;      ///< Button text
    Vec4 m_color;              ///< Button color
    ClickCallback m_callback;  ///< Click callback
    b8 m_wasPressed;          ///< Was button pressed last frame
};

// ==========================================
// CLASS: UIPanel
// ==========================================

/**
 * @class UIPanel
 * @brief Container widget with background.
 */
class UIPanel : public UIWidget {
public:
    /**
     * @brief Constructs a panel.
     * @param name Widget name.
     */
    explicit UIPanel(const std::string& name);

    void Render(const Mat4& projMatrix) override;

    /**
     * @brief Consumes presses that land inside the panel so they don't
     *        fall through and orbit the 3D camera behind it.
     */
    b8 HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) override;

    /// @brief Gets the background color.
    Vec4 GetColor() const { return m_color; }

    /// @brief Sets the background color.
    void SetColor(const Vec4& color) { m_color = color; }

    /// @brief Adds a child widget.
    void AddChild(UIWidget* child) { m_children.push_back(child); }

    /// @brief Removes a child widget (does not delete).
    void RemoveChild(UIWidget* child) {
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end()) m_children.erase(it);
    }

private:
    Vec4 m_color;                      ///< Background color
    std::vector<UIWidget*> m_children;  ///< Child widgets
};

// ==========================================
// CLASS: UISlider
// ==========================================

/**
 * @class UISlider
 * @brief Horizontal draggable value control.
 *
 * Renders a track with a colored handle. Dragging the handle (or pressing
 * anywhere on the track) maps the mouse X position to a value in
 * [minValue, maxValue]. An optional callback fires whenever the value changes.
 */
class UISlider : public UIWidget {
public:
    /// @brief Value-changed callback type.
    using ChangeCallback = std::function<void(f32)>;

    /**
     * @brief Constructs a slider.
     * @param name Widget name.
     * @param minValue Minimum value (left end).
     * @param maxValue Maximum value (right end).
     * @param value Initial value (clamped into range).
     */
    UISlider(const std::string& name, f32 minValue, f32 maxValue, f32 value);

    b8 HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) override;
    void Render(const Mat4& projMatrix) override;

    /// @brief Gets the current value.
    f32 GetValue() const { return m_value; }

    /// @brief Sets the current value (clamped into range; fires the callback).
    void SetValue(f32 value);

    /// @brief Gets the minimum value.
    f32 GetMin() const { return m_min; }

    /// @brief Gets the maximum value.
    f32 GetMax() const { return m_max; }

    /// @brief Sets the value-changed callback.
    void SetCallback(ChangeCallback cb) { m_callback = cb; }

    /// @brief Whether the handle is currently being dragged.
    b8 IsDragging() const { return m_dragging; }

    /// @brief Sets the handle (knob) color.
    void SetHandleColor(const Vec4& color) { m_handleColor = color; }

    /// @brief Sets the track (background) color.
    void SetTrackColor(const Vec4& color) { m_trackColor = color; }

private:
    f32 m_min;                 ///< Minimum value.
    f32 m_max;                 ///< Maximum value.
    f32 m_value;               ///< Current value.
    Vec4 m_trackColor;         ///< Track background color.
    Vec4 m_fillColor;          ///< Filled-portion color.
    Vec4 m_handleColor;        ///< Handle (knob) color.
    ChangeCallback m_callback; ///< Value-changed callback.
    b8 m_dragging;             ///< Whether the handle is being dragged.
};

// ==========================================
// CLASS: DockPanel
// ==========================================

/**
 * @class DockPanel
 * @brief Docking panel widget with title bar, close button, resize handle, and tab support.
 *
 * DockPanel is a container widget that provides window chrome (title bar, close button,
 * resize handle) and can be docked to zones or floated freely. Managed by DockManager.
 */
class DockPanel : public UIWidget {
public:
    /// @brief Close callback type.
    using CloseCallback = std::function<void()>;

    /// @brief Resize edge/corner identifiers for drag-resizing.
    enum class ResizeEdge : u8 {
        None,
        Left, Right, Top, Bottom,
        TopLeft, TopRight, BottomLeft, BottomRight
    };

    /**
     * @brief Constructs a docking panel.
     * @param name Widget name.
     * @param title Panel title displayed in the title bar.
     */
    explicit DockPanel(const std::string& name, const std::string& title = "Panel");

    void Update(f32 deltaTime) override;
    b8 HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) override;
    void Render(const Mat4& projMatrix) override;

    // --- Content ---

    /// @brief Sets the content widget (the panel body).
    void SetContent(UIWidget* content) { m_content = content; }

    /// @brief Gets the content widget.
    UIWidget* GetContent() const { return m_content; }

    // --- Title ---

    /// @brief Sets the panel title.
    void SetTitle(const std::string& title) { m_title = title; }

    /// @brief Gets the panel title.
    const char* GetTitle() const { return m_title.c_str(); }

    // --- Close ---

    /// @brief Sets the close callback.
    void SetCloseCallback(CloseCallback cb) { m_onClose = cb; }

    /// @brief Gets the close callback.
    const CloseCallback& GetCloseCallback() const { return m_onClose; }

    // --- Dock state ---

    /// @brief Whether the panel is docked in a zone.
    void SetDocked(b8 docked) { m_docked = docked; }
    b8 IsDocked() const { return m_docked; }

    /// @brief Sets the zone ID (e.g. "top_0", "left_2").
    void SetZoneId(const std::string& zoneId) { m_zoneId = zoneId; }
    const std::string& GetZoneId() const { return m_zoneId; }

    /// @brief Position when floating (undocked).
    void SetFloatingPos(const Vec2& pos) { m_floatPosition = pos; }
    Vec2 GetFloatingPos() const { return m_floatPosition; }

    // --- Tab support ---

    /// @brief Whether this panel is the active tab in its zone.
    void SetActiveTab(b8 active) { m_activeTab = active; }
    b8 IsActiveTab() const { return m_activeTab; }

    /// @brief Whether the close button is currently hovered.
    b8 IsCloseHovered() const { return m_hoverClose; }

    /// @brief Sets the close button hover state (updated by DockManager).
    void SetCloseHovered(b8 hovered) { m_hoverClose = hovered; }

    // --- Hit testing (called by DockManager) ---

    /// @brief Whether a point is over the title bar (excluding close button).
    b8 HitTestTitleBar(f32 mx, f32 my) const;

    /// @brief Whether a point is over the close button.
    b8 HitTestCloseButton(f32 mx, f32 my) const;

    /// @brief Returns which resize edge/corner the point is on (None if not over any).
    ResizeEdge HitTestResizeEdge(f32 mx, f32 my) const;

    /// @brief Whether a point is over the resize handle (bottom-right corner).
    b8 HitTestResizeHandle(f32 mx, f32 my) const;

    /// @brief The currently hovered resize edge (set by DockManager).
    ResizeEdge GetHoverResizeEdge() const { return m_hoverResizeEdge; }
    void SetHoverResizeEdge(ResizeEdge edge) { m_hoverResizeEdge = edge; }

    /// @brief Close button rect for input routing.
    UIRect GetCloseButtonRect() const;

    // --- Layout constants ---

    static constexpr f32 TITLE_HEIGHT = 24.0f;
    static constexpr f32 TAB_HEIGHT = 22.0f;
    static constexpr f32 CLOSE_SIZE = 14.0f;
    static constexpr f32 RESIZE_HANDLE = 12.0f;
    static constexpr f32 PADDING = 4.0f;
    static constexpr f32 EDGE_HIT = 6.0f;   ///< Edge hit-test width in pixels

private:
    std::string m_title;            ///< Panel title text
    UIWidget* m_content = nullptr;  ///< The panel body widget
    CloseCallback m_onClose;        ///< Called when close button is clicked

    // Dock state
    b8 m_docked = true;             ///< Whether snapped to a zone
    std::string m_zoneId;           ///< Zone identifier string
    Vec2 m_floatPosition;           ///< Position when floating

    // Tab state
    b8 m_activeTab = true;          ///< Whether this panel is the active tab

    // Interaction state
    b8 m_hoverClose = false;        ///< Whether mouse is over the close button
    ResizeEdge m_hoverResizeEdge = ResizeEdge::None; ///< Currently hovered edge
};

// ==========================================
// CLASS: UITreeView
// ==========================================

class UITreeView : public UIWidget {
public:
    using SelectCallback = std::function<void(i32 index, u64 userData)>;
    using ActionCallback  = std::function<void(i32 index, u64 userData)>;
    using ContextMenuCallback = std::function<void(i32 index, u64 userData, f32 mouseX, f32 mouseY)>;
    using DragStartCallback = std::function<void(i32 index, u64 userData, f32 mouseX, f32 mouseY)>;

    explicit UITreeView(const std::string& name);

    void Update(f32 deltaTime) override;
    b8 HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) override;
    void Render(const Mat4& projMatrix) override;

    i32 AddNode(const std::string& label, i32 parentIndex = -1, u64 userData = 0);
    void RemoveNode(i32 index);
    void Clear();
    i32 GetNodeCount() const { return static_cast<i32>(m_nodes.size()); }
    u64 GetNodeUserData(i32 index) const;
    const char* GetNodeLabel(i32 index) const;

    void Expand(i32 index);
    void Collapse(i32 index);
    void ExpandAll();
    void CollapseAll();
    b8 IsExpanded(i32 index) const;

    void SetSelected(i32 index);
    i32 GetSelected() const { return m_selectedIndex; }
    u64 GetSelectedUserData() const;

    void SetSearchFilter(const std::string& filter);
    const std::string& GetSearchFilter() const { return m_searchFilter; }

    void SetSelectCallback(SelectCallback cb)       { m_selectCallback = cb; }
    void SetActionCallback(ActionCallback cb)        { m_actionCallback = cb; }
    void SetContextMenuCallback(ContextMenuCallback cb) { m_contextMenuCallback = cb; }
    void SetDragStartCallback(DragStartCallback cb)  { m_dragStartCallback = cb; }
    i32 GetNodeIndexAt(f32 mx, f32 my) const;

private:
    struct TreeNode {
        std::string label;
        i32 parent = -1;
        std::vector<i32> children;
        u64 userData = 0;
        b8 expanded = true;
        b8 visible = true;
    };

    void RebuildVisibleOrder();
    i32 ComputeVisibleIndex(i32 treeIndex) const;
    i32 HitTestRow(f32 mx, f32 my) const;

    std::vector<TreeNode> m_nodes;
    std::vector<i32> m_visibleOrder;
    i32 m_selectedIndex = -1;
    std::string m_searchFilter;

    SelectCallback m_selectCallback;
    ActionCallback m_actionCallback;
    ContextMenuCallback m_contextMenuCallback;
    DragStartCallback m_dragStartCallback;

    b8 m_wasMouseDown = false;
    b8 m_dragPending = false;
    f32 m_dragStartX = 0.0f;
    f32 m_dragStartY = 0.0f;
    i32 m_dragItemIndex = -1;
    b8 m_hoverScrollBar = false;
    f32 m_scrollOffset = 0.0f;

    static constexpr f32 ROW_HEIGHT = 20.0f;
    static constexpr f32 INDENT_WIDTH = 16.0f;
    static constexpr f32 ARROW_WIDTH = 14.0f;
    static constexpr f32 SCROLLBAR_WIDTH = 10.0f;
};

// ==========================================
// CLASS: UIPropertyGrid
// ==========================================

/**
 * @class UIPropertyGrid
 * @brief Dynamic property editor widget with typed field rows.
 *
 * Displays a scrollable list of property rows, each containing a label
 * and an interactive editor appropriate to the field type:
 *   - Float: drag left/right to scrub the value.
 *   - Int: drag left/right to scrub (snaps to integers).
 *   - Bool: click to toggle checkbox.
 *   - String: click to begin keyboard text editing.
 *   - Vec3: three drag-scrub sub-fields (X, Y, Z).
 *   - Enum: click to cycle through options.
 *   - Slider: drag handle or click track.
 *   - Label: read-only text display.
 *   - Category: section header with background.
 *
 * Usage (by InspectorPanel):
 * @code
 * grid->Clear();
 * grid->AddCategory("Transform");
 * grid->AddVec3("Position", xf.position, [&](const Vec3& v) { xf.position = v; });
 * grid->AddBool("Visible", renderer.visible, [&](b8 v) { renderer.visible = v; });
 * @endcode
 */
class UIPropertyGrid : public UIWidget {
public:
    using FloatCallback   = std::function<void(f32)>;
    using IntCallback     = std::function<void(i32)>;
    using BoolCallback    = std::function<void(b8)>;
    using StringCallback  = std::function<void(const std::string&)>;
    using Vec3Callback    = std::function<void(const Vec3&)>;
    using EnumCallback    = std::function<void(i32)>;

    using CommitCallback = std::function<void()>;

    explicit UIPropertyGrid(const std::string& name);

    void Update(f32 deltaTime) override;
    b8 HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) override;
    void Render(const Mat4& projMatrix) override;

    /// @brief Clears all property entries.
    void Clear();

    /// @brief Adds a category section header.
    void AddCategory(const std::string& name);

    /// @brief Adds a float property (drag left/right to scrub).
    void AddFloat(const std::string& label, f32 value, FloatCallback onChange);

    /// @brief Adds an integer property (drag left/right to scrub, snaps to int).
    void AddInt(const std::string& label, i32 value, IntCallback onChange);

    /// @brief Adds a boolean property (click to toggle).
    void AddBool(const std::string& label, b8 value, BoolCallback onChange);

    /// @brief Adds a string property (click to begin text editing).
    void AddString(const std::string& label, const std::string& value, StringCallback onChange);

    /// @brief Adds a Vec3 property (three drag-scrub sub-fields).
    void AddVec3(const std::string& label, const Vec3& value, Vec3Callback onChange);

    /// @brief Adds an enum property (click to cycle through options).
    void AddEnum(const std::string& label, i32 value,
                 const std::vector<std::string>& options, EnumCallback onChange);

    /// @brief Adds a slider property (drag handle or click track).
    void AddSlider(const std::string& label, f32 value, f32 min, f32 max,
                   FloatCallback onChange);

    /// @brief Adds a read-only label row.
    void AddLabel(const std::string& label, const std::string& value);

    /// @brief Sets the commit callback, fired when any property edit completes.
    void SetCommitCallback(CommitCallback cb) { m_onCommit = cb; }

    // --- Layout constants ---

    static constexpr f32 ROW_HEIGHT      = 22.0f;
    static constexpr f32 CATEGORY_HEIGHT = 24.0f;
    static constexpr f32 LABEL_WIDTH     = 110.0f;
    static constexpr f32 VALUE_OFFSET    = 114.0f;
    static constexpr f32 SCROLLBAR_WIDTH = 10.0f;
    static constexpr f32 CHECKBOX_SIZE   = 14.0f;
    static constexpr f32 HANDLE_SIZE     = 10.0f;
    static constexpr f32 TRACK_HEIGHT    = 6.0f;

private:
    /// @brief Internal property entry type enum.
    enum class PropType {
        CATEGORY, FLOAT, INT, BOOL, STRING, VEC3, ENUM, SLIDER, LABEL
    };

    /**
     * @struct PropEntry
     * @brief Stores one property row's data and callbacks.
     */
    struct PropEntry {
        PropType type;                     ///< The property type.
        std::string label;                 ///< Field label text.

        // Value storage
        f32 floatVal = 0.0f;
        f32 floatMin = 0.0f;
        f32 floatMax = 1.0f;
        i32 intVal   = 0;
        b8  boolVal  = false;
        std::string stringVal;
        Vec3 vec3Val;
        i32 enumVal = 0;
        std::vector<std::string> enumOptions;

        // Callbacks
        FloatCallback  onFloatChange;
        IntCallback    onIntChange;
        BoolCallback   onBoolChange;
        StringCallback onStringChange;
        Vec3Callback   onVec3Change;
        EnumCallback   onEnumChange;

        // Drag interaction state
        b8  dragActive    = false;
        f32 dragStartX    = 0.0f;
        f32 dragStartVal  = 0.0f;
        i32 dragSubField  = -1;  ///< For Vec3: 0=X, 1=Y, 2=Z; -1 = whole-field.
    };

    std::vector<PropEntry> m_entries;
    f32  m_scrollOffset  = 0.0f;
    i32  m_textEditIndex = -1;  ///< Index of entry being text-edited, or -1.
    b8   m_wasMouseDown  = false;
    CommitCallback m_onCommit;

    /// @brief Computes the total height of all entries.
    f32 TotalContentHeight() const;

    /// @brief Hits a mouse position against property rows.
    /// @return Entry index, or -1 if none.
    i32 HitTestRow(f32 mx, f32 my) const;

};

// ==========================================
// CLASS: UIFileBrowser
// ==========================================

/**
 * @class UIFileBrowser
 * @brief Directory and file listing widget with navigation.
 *
 * Displays a path bar at the top, then a scrollable list of files and
 * subdirectories in the current directory. Icons are rendered as colored
 * rectangles (folders in yellow, files by extension type).
 *
 * Navigation:
 *   - Double-click a directory to enter it.
 *   - Single-click a file to select it (fires the file callback).
 *   - A parent-directory entry ("..") is shown at the top.
 *   - The path bar at the top can be clicked to go up one level.
 *
 * Usage (by AssetBrowserPanel):
 * @code
 * browser->SetRootDirectory("./Assets");
 * browser->SetFileFilter(".obj");
 * browser->SetFileCallback([](const std::string& path) {
 *     DND_LOG_INFO("Selected: %s", path.c_str());
 * });
 * browser->Refresh();
 * @endcode
 */
class UIFileBrowser : public UIWidget {
public:
    /// @brief File selection callback.
    using FileCallback = std::function<void(const std::string& path)>;
    /// @brief Directory navigation callback.
    using DirCallback = std::function<void(const std::string& path)>;
    /// @brief Drag start callback (file path, mouse x, mouse y).
    using DragStartCallback = std::function<void(const std::string& path, f32 mouseX, f32 mouseY)>;

    explicit UIFileBrowser(const std::string& name);

    void Update(f32 deltaTime) override;
    b8 HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) override;
    void Render(const Mat4& projMatrix) override;

    /// @brief Sets the root directory (the highest level the browser can go up to).
    void SetRootDirectory(const std::string& path);

    /// @brief Gets the current directory path.
    const std::string& GetCurrentDirectory() const { return m_currentDir; }

    /**
     * @brief Navigates to a subdirectory (relative or absolute).
     * @param path Subdirectory name (relative) or absolute path.
     */
    void NavigateTo(const std::string& path);

    /**
     * @brief Sets a file extension filter.
     * @param extension File extension to show (e.g. ".obj"), or empty to show all.
     */
    void SetFileFilter(const std::string& extension);

    /// @brief Re-scans the current directory.
    void Refresh();

    /// @brief Sets the file selection callback.
    void SetFileCallback(FileCallback cb) { m_fileCallback = cb; }

    /// @brief Sets the directory navigation callback.
    void SetDirCallback(DirCallback cb) { m_dirCallback = cb; }

    /// @brief Sets the drag-start callback (fired when user starts dragging a file item).
    void SetDragStartCallback(DragStartCallback cb) { m_dragStartCallback = cb; }

    // --- Layout constants ---

    static constexpr f32 ROW_HEIGHT       = 36.0f;
    static constexpr f32 ICON_SIZE        = 28.0f;
    static constexpr f32 PATH_BAR_HEIGHT  = 22.0f;
    static constexpr f32 SCROLLBAR_WIDTH  = 10.0f;
    static constexpr f32 SIZE_COLUMN_WIDTH = 70.0f;

private:
    /**
     * @struct FileEntry
     * @brief Describes one item in the file list.
     */
    struct FileEntry {
        std::string name;       ///< Display name
        std::string path;       ///< Full filesystem path
        b8  isDirectory = false;///< Whether this is a subdirectory
        u64 fileSize    = 0;    ///< File size in bytes (0 for directories)
    };

    std::string m_rootDir;         ///< Upper bound for directory navigation
    std::string m_currentDir;      ///< Currently displayed directory
    std::string m_fileFilter;      ///< Extension filter (empty = show all)
    std::vector<FileEntry> m_files;///< File listing for the current directory

    f32  m_scrollOffset  = 0.0f;
    i32  m_selectedIndex = -1;
    b8   m_wasMouseDown  = false;
    f32  m_lastClickTime = 0.0f;  ///< For double-click detection
    i32  m_lastClickIndex = -1;   ///< For double-click detection
    b8   m_needsRefresh  = true;  ///< Whether the listing needs to be re-scanned
    b8   m_dragPending   = false; ///< Mouse down on file, waiting for drag threshold
    f32  m_dragStartX    = 0.0f;  ///< Mouse X when drag-pending began
    f32  m_dragStartY    = 0.0f;  ///< Mouse Y when drag-pending began
    i32  m_dragItemIndex = -1;    ///< Index of the item being dragged

    FileCallback m_fileCallback;
    DirCallback  m_dirCallback;
    DragStartCallback m_dragStartCallback;

    /// @brief Scans the current directory and populates m_files.
    void ScanDirectory();

    /// @brief Returns the display name for a path (parent directory, etc.).
    static std::string GetDisplayName(const std::string& path);

    /// @brief Hit-tests a mouse position against visible file rows.
    i32 HitTestRow(f32 mx, f32 my) const;

    /// @brief Draws a file-type icon at the given position.
    void DrawIcon(f32 x, f32 y, const FileEntry& entry);

    /// @brief Gets or creates an OpenGL texture ID for a thumbnail by path.
    u32 GetOrCreateThumbnail(const std::string& path);

    /// @brief Clears the thumbnail texture cache.
    void ClearThumbnailCache();

    /// @brief Map from file path to cached OpenGL texture ID for thumbnails.
    std::unordered_map<std::string, u32> m_thumbnailCache;
};

} // namespace Dnd
