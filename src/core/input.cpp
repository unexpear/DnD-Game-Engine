/**
 * @file input.cpp
 * @brief Implementation of the Input class.
 * 
 * See input.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "core/input.h"
#include "core/window.h"
#include "core/log.h"
#include <GLFW/glfw3.h>
#include <cstring>

namespace Dnd {

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

/** @brief Current frame key state. */
b8 Input::s_keys[Input::KEY_COUNT] = {false};

/** @brief Previous frame key state. */
b8 Input::s_prevKeys[Input::KEY_COUNT] = {false};

/** @brief Current frame mouse button state. */
b8 Input::s_mouseButtons[5] = {false};

/** @brief Previous frame mouse button state. */
b8 Input::s_prevMouseButtons[5] = {false};

/** @brief Current mouse X position. */
f32 Input::s_mouseX = 0.0f;

/** @brief Current mouse Y position. */
f32 Input::s_mouseY = 0.0f;

/** @brief Previous mouse X position. */
f32 Input::s_prevMouseX = 0.0f;

/** @brief Previous mouse Y position. */
f32 Input::s_prevMouseY = 0.0f;

/** @brief Scroll wheel delta this frame. */
f32 Input::s_scrollDelta = 0.0f;

/** @brief Captured text buffer (in-game command box). */
std::string Input::s_textInput;

/** @brief Whether text capture is active. */
b8 Input::s_textActive = false;

// ==========================================
// INITIALIZATION
// ==========================================

void Input::Initialize() {
    // Reset all input state to defaults
    std::memset(s_keys, 0, sizeof(s_keys));
    std::memset(s_prevKeys, 0, sizeof(s_prevKeys));
    std::memset(s_mouseButtons, 0, sizeof(s_mouseButtons));
    std::memset(s_prevMouseButtons, 0, sizeof(s_prevMouseButtons));
    s_mouseX = 0.0f;
    s_mouseY = 0.0f;
    s_prevMouseX = 0.0f;
    s_prevMouseY = 0.0f;
    s_scrollDelta = 0.0f;
    s_textInput.clear();
    s_textActive = false;

    // Capture typed characters via GLFW (for the in-game command box).
    GLFWwindow* window = Window::GetGLFWWindow();
    if (window != nullptr) {
        glfwSetCharCallback(window, CharCallback);
    }

    DND_LOG_INFO("Input system initialized");
}

// ==========================================
// FRAME UPDATE
// ==========================================

void Input::Update() {
    GLFWwindow* window = Window::GetGLFWWindow();
    if (window == nullptr) {
        return;
    }

    // Step 1: Save current state as previous state
    std::memcpy(s_prevKeys, s_keys, sizeof(s_keys));
    std::memcpy(s_prevMouseButtons, s_mouseButtons, sizeof(s_mouseButtons));
    s_prevMouseX = s_mouseX;
    s_prevMouseY = s_mouseY;

    // Step 2: Query current key states from GLFW
    for (i32 i = 0; i < KEY_COUNT; i++) {
        s_keys[i] = (glfwGetKey(window, i) == GLFW_PRESS);
    }

    // Step 3: Query current mouse position (window coordinates)
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    s_mouseX = static_cast<f32>(mouseX);
    s_mouseY = static_cast<f32>(mouseY);

    // Step 4: Query mouse button states
    for (i32 i = 0; i < 5; i++) {
        s_mouseButtons[i] = (glfwGetMouseButton(window, i) == GLFW_PRESS);
    }

    // Step 5: Query scroll delta
    // Scroll is accumulated from events — we read and reset each frame
    // For now, we'll compute it from cursor position changes
    // In a full implementation, we'd use a scroll callback
    s_scrollDelta = 0.0f;
}

// ==========================================
// KEYBOARD QUERIES
// ==========================================

b8 Input::IsKeyDown(KeyCode key) {
    i32 index = static_cast<i32>(key);
    if (index < 0 || index >= KEY_COUNT) {
        return false;
    }
    return s_keys[index];
}

b8 Input::IsKeyPressed(KeyCode key) {
    i32 index = static_cast<i32>(key);
    if (index < 0 || index >= KEY_COUNT) {
        return false;
    }
    // Pressed = currently down AND was up last frame
    return s_keys[index] && !s_prevKeys[index];
}

b8 Input::IsKeyReleased(KeyCode key) {
    i32 index = static_cast<i32>(key);
    if (index < 0 || index >= KEY_COUNT) {
        return false;
    }
    // Released = currently up AND was down last frame
    return !s_keys[index] && s_prevKeys[index];
}

// ==========================================
// MOUSE QUERIES
// ==========================================

f32 Input::GetMouseX() {
    return s_mouseX;
}

f32 Input::GetMouseY() {
    return s_mouseY;
}

Vec2 Input::GetMousePosition() {
    return Vec2(s_mouseX, s_mouseY);
}

f32 Input::GetMouseDeltaX() {
    return s_mouseX - s_prevMouseX;
}

f32 Input::GetMouseDeltaY() {
    return s_mouseY - s_prevMouseY;
}

Vec2 Input::GetMouseDelta() {
    return Vec2(s_mouseX - s_prevMouseX, s_mouseY - s_prevMouseY);
}

b8 Input::IsMouseButtonDown(MouseButton button) {
    i32 index = static_cast<i32>(button);
    if (index < 0 || index >= 5) {
        return false;
    }
    return s_mouseButtons[index];
}

b8 Input::IsMouseButtonPressed(MouseButton button) {
    i32 index = static_cast<i32>(button);
    if (index < 0 || index >= 5) {
        return false;
    }
    return s_mouseButtons[index] && !s_prevMouseButtons[index];
}

f32 Input::GetScrollDelta() {
    return s_scrollDelta;
}

// ==========================================
// TEXT INPUT
// ==========================================

void Input::CharCallback(GLFWwindow* /*window*/, unsigned int codepoint) {
    // Only collect printable ASCII while capture is active.
    if (s_textActive && codepoint >= 32 && codepoint < 127) {
        s_textInput += static_cast<char>(codepoint);
    }
}

void Input::BeginTextInput() {
    s_textInput.clear();
    s_textActive = true;
}

void Input::EndTextInput() {
    s_textActive = false;
}

b8 Input::IsTextInputActive() {
    return s_textActive;
}

const std::string& Input::GetTextInput() {
    return s_textInput;
}

void Input::Backspace() {
    if (!s_textInput.empty()) s_textInput.pop_back();
}

} // namespace Dnd
