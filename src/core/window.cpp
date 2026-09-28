/**
 * @file window.cpp
 * @brief Implementation of the Window class.
 * 
 * See window.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "core/window.h"
#include "core/log.h"
#include <GLFW/glfw3.h>

namespace Dnd {

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

/** @brief The underlying GLFW window pointer. */
GLFWwindow* Window::s_window = nullptr;

/** @brief Whether the window has been created. */
b8 Window::s_created = false;

/** @brief Current framebuffer width. */
i32 Window::s_framebufferWidth = 0;

/** @brief Current framebuffer height. */
i32 Window::s_framebufferHeight = 0;

/** @brief Window configuration copy. */
WindowConfig Window::s_config;

// ==========================================
// WINDOWCONFIG DEFAULTS
// ==========================================

/**
 * @brief Default constructor for WindowConfig.
 * 
 * Sets sensible defaults: 1280x720, resizable, VSync on, windowed mode.
 */
WindowConfig::WindowConfig()
    : title("D&D Game Engine")
    , width(1280)
    , height(720)
    , resizable(true)
    , vsync(true)
    , fullscreen(false)
{
}

// ==========================================
// CREATION
// ==========================================

b8 Window::Create(const WindowConfig& config) {
    // Step 1: Check if already created
    if (s_created) {
        DND_LOG_WARN("Window::Create called but window already exists");
        return true;
    }

    // Step 2: Initialize GLFW
    if (!glfwInit()) {
        DND_LOG_FATAL("Failed to initialize GLFW");
        return false;
    }
    DND_LOG_INFO("GLFW %s initialized", glfwGetVersionString());

    // Step 3: Configure GLFW window hints
    // OpenGL 4.6 Core Profile — modern OpenGL, no deprecated functions
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Request a debug context so the GL debug-message callback (registered in
    // Renderer::Initialize) delivers driver diagnostics. Negligible dev overhead.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    // Forward compatible: disables deprecated functionality
    // Required on macOS, harmless on other platforms
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // Resizable based on config
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    // Step 4: Create the window
    GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    s_window = glfwCreateWindow(
        config.width,
        config.height,
        config.title,
        monitor,
        nullptr
    );

    if (s_window == nullptr) {
        DND_LOG_FATAL("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    // Step 5: Make the window's OpenGL context current
    glfwMakeContextCurrent(s_window);

    // Step 6: Enable VSync if configured
    // glfwSwapInterval(1) = VSync on (wait for 1 vertical refresh)
    // glfwSwapInterval(0) = VSync off (uncapped framerate)
    glfwSwapInterval(config.vsync ? 1 : 0);

    // Step 8: Register callbacks
    glfwSetFramebufferSizeCallback(s_window, OnFramebufferResize);
    glfwSetWindowCloseCallback(s_window, OnWindowClose);

    // Step 9: Get initial framebuffer size
    glfwGetFramebufferSize(s_window, &s_framebufferWidth, &s_framebufferHeight);
    DND_LOG_INFO("Framebuffer size: %dx%d", s_framebufferWidth, s_framebufferHeight);

    // Step 10: Store config copy
    s_config = config;

    // Step 11: Mark as created and log success
    s_created = true;
    DND_LOG_INFO("Window created: '%s' %dx%d (VSync: %s, Fullscreen: %s)",
        config.title, config.width, config.height,
        config.vsync ? "ON" : "OFF",
        config.fullscreen ? "ON" : "OFF");

    return true;
}

// ==========================================
// DESTRUCTION
// ==========================================

void Window::Destroy() {
    if (!s_created) {
        return;
    }

    DND_LOG_INFO("Destroying window...");

    // Step 2: Destroy the window
    glfwDestroyWindow(s_window);
    s_window = nullptr;

    // Step 3: Terminate GLFW
    glfwTerminate();

    // Step 4: Reset state
    s_created = false;
    s_framebufferWidth = 0;
    s_framebufferHeight = 0;

    DND_LOG_INFO("Window destroyed, GLFW terminated");
}

// ==========================================
// EVENT POLLING
// ==========================================

void Window::Update() {
    // Step 1: Poll all queued window events from the OS
    glfwPollEvents();
}

// ==========================================
// WINDOW STATE QUERIES
// ==========================================

b8 Window::ShouldClose() {
    if (s_window == nullptr) {
        return true;
    }
    return glfwWindowShouldClose(s_window) ? true : false;
}

void Window::SwapBuffers() {
    if (s_window != nullptr) {
        glfwSwapBuffers(s_window);
    }
}

i32 Window::GetFramebufferWidth() {
    return s_framebufferWidth;
}

i32 Window::GetFramebufferHeight() {
    return s_framebufferHeight;
}

f32 Window::GetAspectRatio() {
    if (s_framebufferHeight == 0) {
        return 1.0f; // Guard against division by zero
    }
    return static_cast<f32>(s_framebufferWidth) / static_cast<f32>(s_framebufferHeight);
}

const char* Window::GetTitle() {
    return s_config.title;
}

void Window::SetCursorMode(CursorMode mode) {
    if (s_window == nullptr) {
        return;
    }

    i32 glfwMode;
    switch (mode) {
        case CursorMode::NORMAL:   glfwMode = GLFW_CURSOR_NORMAL; break;
        case CursorMode::HIDDEN:   glfwMode = GLFW_CURSOR_HIDDEN; break;
        case CursorMode::CAPTURED: glfwMode = GLFW_CURSOR_DISABLED; break;
        default:                   glfwMode = GLFW_CURSOR_NORMAL; break;
    }
    glfwSetInputMode(s_window, GLFW_CURSOR, glfwMode);
}

GLFWwindow* Window::GetGLFWWindow() {
    return s_window;
}

// ==========================================
// GLFW CALLBACKS
// ==========================================

/**
 * @brief Handles framebuffer resize events.
 * 
 * Called by GLFW whenever the window is resized or moved between
 * monitors with different DPI. Updates the stored framebuffer dimensions.
 * 
 * The game's renderer should use these dimensions to update the viewport
 * and projection matrix aspect ratio.
 */
void Window::OnFramebufferResize(GLFWwindow* window, i32 width, i32 height) {
    DND_UNUSED(window);
    s_framebufferWidth = width;
    s_framebufferHeight = height;
    DND_LOG_DEBUG("Framebuffer resized: %dx%d", width, height);
}

/**
 * @brief Handles window close requests.
 * 
 * Called by GLFW when the user clicks the close button or presses Alt+F4.
 * Currently just logs the event. The main loop checks ShouldClose() to exit.
 */
void Window::OnWindowClose(GLFWwindow* window) {
    DND_UNUSED(window);
    DND_LOG_INFO("Window close requested");
}

} // namespace Dnd
