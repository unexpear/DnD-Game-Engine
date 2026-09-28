/**
 * @file ui_system.h
 * @brief UI system for managing widgets and input.
 * 
 * The UISystem is NOT an ECS system — it manages
 * retained-mode UI widgets directly.
 * 
 * Features:
 *   - Owns all UI widgets
 *   - Processes mouse input and dispatches to widgets
 *   - Renders all visible widgets
 *   - Manages widget hierarchy (parent/child)
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.7.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include <vector>
#include <string>
#include <functional>

namespace Dnd {

// Forward declarations
class UIWidget;
class UILabel;
class UIButton;
class UIPanel;
class UISlider;

// ==========================================
// CLASS: UISystem
// ==========================================

/**
 * @class UISystem
 * @brief Manages all UI widgets and input processing.
 * 
 * The UISystem is a singleton that owns all UI widgets.
 * It processes input and renders widgets each frame.
 * 
 * Usage:
 * @code
 * UISystem::Get().Initialize();
 * UILabel* label = UISystem::Get().CreateLabel("FPS", "FPS: 60");
 * UIButton* btn = UISystem::Get().CreateButton("Quit", "Quit");
 * btn->SetCallback([]() { Engine::SetState(EngineState::SHUTTING_DOWN); });
 * @endcode
 */
class UISystem {
public:
    /**
     * @brief Gets the singleton instance.
     * @return Reference to the UISystem instance.
     */
    static UISystem& Get();

    /**
     * @brief Initializes the UI system.
     * @return true if successful.
     */
    b8 Initialize();

    /**
     * @brief Shuts down the UI system.
     */
    void Shutdown();

    /**
     * @brief Updates all UI widgets.
     * @param deltaTime Frame delta time.
     */
    void Update(f32 deltaTime);

    /**
     * @brief Renders all UI widgets.
     * @param projMatrix Orthographic projection matrix.
     */
    void Render(const Mat4& projMatrix);

    /**
     * @brief Processes mouse input.
     * @param mouseX Mouse X position (screen pixels).
     * @param mouseY Mouse Y position (screen pixels).
     * @param mouseDown Whether mouse button is pressed.
     */
    void ProcessMouse(f32 mouseX, f32 mouseY, b8 mouseDown);

    // Widget creation methods

    /**
     * @brief Creates a label widget.
     * @param name Widget name.
     * @param text Initial text.
     * @return Pointer to the created label.
     */
    UILabel* CreateLabel(const std::string& name, const std::string& text = "");

    /**
     * @brief Creates a button widget.
     * @param name Widget name.
     * @param text Button text.
     * @return Pointer to the created button.
     */
    UIButton* CreateButton(const std::string& name, const std::string& text = "Button");

    /**
     * @brief Creates a panel widget.
     * @param name Widget name.
     * @return Pointer to the created panel.
     */
    UIPanel* CreatePanel(const std::string& name);

    /**
     * @brief Creates a horizontal slider widget.
     * @param name Widget name.
     * @param minValue Minimum value (left end).
     * @param maxValue Maximum value (right end).
     * @param value Initial value.
     * @return Pointer to the created slider.
     */
    UISlider* CreateSlider(const std::string& name, f32 minValue, f32 maxValue, f32 value);

    /**
     * @brief Whether the UI is currently capturing the mouse (a slider is being
     *        dragged, or a press landed on a panel). Callers can use this to
     *        suppress 3D camera input while the user works the UI.
     * @return true if the UI consumed the mouse this frame.
     */
    b8 IsCapturingMouse() const { return m_mouseCaptured; }

    /**
     * @brief Gets a widget by name.
     * @param name Widget name to find.
     * @return Pointer to widget, or nullptr if not found.
     */
    UIWidget* GetWidget(const std::string& name);

    /**
     * @brief Destroys a widget.
     * @param name Widget name to destroy.
     */
    void DestroyWidget(const std::string& name);

    /**
     * @brief Sets the screen dimensions.
     * @param width Screen width in pixels.
     * @param height Screen height in pixels.
     */
    void SetScreenSize(u32 width, u32 height);

private:
    /**
     * @brief Private constructor (singleton).
     */
    UISystem();

    /**
     * @brief Private destructor.
     */
    ~UISystem();

    /**
     * @brief Finds a widget by name (internal).
     */
    UIWidget* FindWidgetInternal(const std::string& name);

    std::vector<UIWidget*> m_widgets;  ///< All widgets (owned)
    u32 m_screenWidth;                   ///< Screen width
    u32 m_screenHeight;                  ///< Screen height
    b8 m_initialized;                    ///< Whether system is initialized
    b8 m_mouseCaptured;                  ///< Whether the UI consumed the mouse this frame
};

} // namespace Dnd
