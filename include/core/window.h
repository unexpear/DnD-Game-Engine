/**
 * @file window.h
 * @brief GLFW-based window management abstraction.
 * 
 * Provides a clean interface for creating, managing, and interacting with
 * the application window. Handles GLFW initialization, window creation,
 * event polling, and viewport resize events.
 * 
 * Features:
 *   - Configurable resolution, fullscreen, VSync
 *   - Automatic framebuffer-to-window-size mapping
 *   - Window close detection
 *   - Aspect ratio calculation
 *   - Cursor mode management (normal, hidden, captured)
 * 
 * @note All methods are static. The window is a singleton — only one window supported.
 * @warning GLFW must be initialized before calling any Window methods.
 *          Window::Create() handles GLFW initialization internally.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"

// Forward declare GLFW types to avoid including GLFW in the header
struct GLFWwindow;

namespace Dnd {

// ==========================================
// ENUM: CursorMode
// ==========================================

/**
 * @enum CursorMode
 * @brief Mouse cursor behavior modes.
 */
enum class CursorMode : i32 {
    NORMAL,   ///< Cursor is visible and moves freely over the window.
    HIDDEN,   ///< Cursor is invisible but still functions normally.
    CAPTURED  ///< Cursor is locked to the window center (for camera orbiting).
};

// ==========================================
// STRUCT: WindowConfig
// ==========================================

/**
 * @struct WindowConfig
 * @brief Configuration parameters for window creation.
 */
struct WindowConfig {
    /** @brief Window title displayed in the title bar. */
    const char* title;
    /** @brief Window width in pixels. */
    i32 width;
    /** @brief Window height in pixels. */
    i32 height;
    /** @brief Whether the window can be resized by the user. */
    b8  resizable;
    /** @brief Whether VSync is enabled (caps framerate to monitor refresh rate). */
    b8  vsync;
    /** @brief Whether to start in fullscreen mode. */
    b8  fullscreen;

    /** @brief Default constructor with development-friendly defaults. */
    WindowConfig();
};

// ==========================================
// CLASS: Window
// ==========================================

/**
 * @class Window
 * @brief Singleton window manager wrapping GLFW functionality.
 * 
 * Handles window creation, event polling, input callbacks, and viewport management.
 * All methods are static — there is only one window instance.
 * 
 * Usage:
 * @code
 * WindowConfig config;
 * config.title = "D&D Engine";
 * config.width = 1280;
 * config.height = 720;
 * Window::Create(config);
 * while (!Window::ShouldClose()) {
 *     Window::Update();
 *     // Render...
 * }
 * Window::Destroy();
 * @endcode
 */
class Window {
public:
    /**
     * @brief Creates the application window and initializes GLFW.
     * 
     * Implementation:
     *   1. Check if already created (return true if so)
     *   2. Initialize GLFW with glfwInit()
     *      - If fails, log error, return false
     *   3. Configure GLFW window hints:
     *      - OpenGL version: 4.6 Core Profile
     *      - Forward compatible context
     *      - Resizable based on config
     *   4. Create the window with glfwCreateWindow()
     *      - If nullptr returned, log error, terminate GLFW, return false
     *   5. Make the window's context current with glfwMakeContextCurrent()
     *   6. Enable VSync if configured (glfwSwapInterval)
     *   7. Set user pointer to nullptr
     *   8. Register callbacks:
     *      - Framebuffer resize -> OnFramebufferResize
     *      - Window close -> OnWindowClose
     *      - Key -> OnKey
     *      - Mouse button -> OnMouseButton
     *      - Cursor position -> OnCursorPos
     *      - Scroll -> OnScroll
     *   9. Store initial framebuffer size
     *   10. Store config copy, mark as created
     *   11. Log "Window created: WxH"
     * 
     * @param config Window configuration parameters.
     * @return true if window was created successfully, false otherwise.
     * @post Window is visible, OpenGL context is current, callbacks are registered.
     */
    static b8 Create(const WindowConfig& config);

    /**
     * @brief Destroys the window and terminates GLFW.
     * 
     * Implementation:
     *   1. Check if not created (return immediately)
     *   2. Destroy the window with glfwDestroyWindow()
     *   3. Terminate GLFW with glfwTerminate()
     *   4. Set s_window = nullptr, s_created = false
     * 
     * @post Window is destroyed, GLFW is terminated, no more window operations possible.
     */
    static void Destroy();

    /**
     * @brief Polls window events and updates internal state.
     * 
     * Implementation:
     *   1. Call glfwPollEvents() to process all queued OS events
     *   2. Process internal input state updates
     * 
     * @note Call once per frame, before game logic.
     */
    static void Update();

    /**
     * @brief Checks if the window has been requested to close.
     * @return true if the close button was clicked or glfwSetWindowShouldClose was called.
     */
    static b8 ShouldClose();

    /**
     * @brief Swaps the front and back framebuffers (presents the rendered frame).
     * 
     * Implementation:
     *   1. Call glfwSwapBuffers(s_window)
     * 
     * @note Call at the end of each frame after all rendering is complete.
     * @note If VSync is enabled, this call blocks until the next vertical refresh.
     */
    static void SwapBuffers();

    /**
     * @brief Gets the current framebuffer width in pixels.
     * @return Framebuffer width. This may differ from window width on HiDPI displays.
     */
    static i32 GetFramebufferWidth();

    /**
     * @brief Gets the current framebuffer height in pixels.
     * @return Framebuffer height. This may differ from window height on HiDPI displays.
     */
    static i32 GetFramebufferHeight();

    /**
     * @brief Gets the framebuffer aspect ratio.
     * @return width / height. Returns 1.0f if height is zero (guard against division by zero).
     */
    static f32 GetAspectRatio();

    /**
     * @brief Gets the window title.
     * @return The current window title string.
     */
    static const char* GetTitle();

    /**
     * @brief Sets the mouse cursor mode.
     * @param mode The desired cursor mode (NORMAL, HIDDEN, or CAPTURED).
     */
    static void SetCursorMode(CursorMode mode);

    /**
     * @brief Gets the raw GLFWwindow pointer.
     * @return GLFWwindow pointer for direct GLFW API access when needed.
     * @warning Use only when the Window abstraction doesn't cover the needed functionality.
     */
    static GLFWwindow* GetGLFWWindow();

private:
    /** @brief Private constructor. Enforces singleton pattern. */
    Window() = default;

    /**
     * @brief Callback for framebuffer resize events.
     * @param window GLFW window pointer.
     * @param width New framebuffer width.
     * @param height New framebuffer height.
     */
    static void OnFramebufferResize(GLFWwindow* window, i32 width, i32 height);

    /** @brief Callback for window close requests. */
    static void OnWindowClose(GLFWwindow* window);

    /** @brief The underlying GLFW window pointer. */
    static GLFWwindow* s_window;
    /** @brief Whether the window has been created. */
    static b8 s_created;
    /** @brief Current framebuffer width. */
    static i32 s_framebufferWidth;
    /** @brief Current framebuffer height. */
    static i32 s_framebufferHeight;
    /** @brief Window configuration copy. */
    static WindowConfig s_config;
};

} // namespace Dnd
