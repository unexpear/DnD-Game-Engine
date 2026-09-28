/**
 * @file input.h
 * @brief Input state tracking for keyboard, mouse, and gamepad.
 * 
 * Provides a polled input system — state is queried each frame rather
 * than driven by callbacks. This ensures deterministic, frame-locked
 * input handling that stays in sync with game logic.
 * 
 * Features:
 *   - Keyboard: key down, key pressed (this frame only), key released (this frame only)
 *   - Mouse: position, delta movement, button states, scroll wheel
 *   - Mouse coordinates are in window-space (top-left origin, Y down)
 *   - Gamepad: connection status, button states, axis values
 * 
 * @note Input state is updated once per frame during Window::Update().
 *       Call Input::Update() after Window::Update() in the main loop.
 * @warning Mouse coordinates are relative to the window, not the framebuffer.
 *          On HiDPI displays, these will differ from pixel coordinates.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include <string>

struct GLFWwindow;

namespace Dnd {

// ==========================================
// ENUM: KeyCode
// ==========================================

/**
 * @enum KeyCode
 * @brief Keyboard key codes matching GLFW key codes.
 * 
 * Only commonly used keys are enumerated here. Add more as needed.
 * Values match GLFW key codes for direct mapping.
 */
enum class KeyCode : i32 {
    UNKNOWN = 0,

    // Letter keys
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Number keys
    D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,

    // Zoom keys: main-row '-' and '=' (the '+' is shift+'='), plus numpad -/+
    MINUS = 45, EQUAL = 61, KP_SUBTRACT = 333, KP_ADD = 334,

    // Function keys
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Special keys
    SPACE = 32,
    ESCAPE = 256,
    ENTER = 257,
    TAB = 258,
    BACKSPACE = 259,
    DELETE_KEY = 261,

    // Arrow keys. Values MUST match GLFW (RIGHT=262, LEFT=263, DOWN=264, UP=265).
    // The previous "UP=265, DOWN, LEFT, RIGHT" yielded 266/267/268, so LEFT/RIGHT/DOWN
    // silently mapped to PAGE_DOWN/HOME/PAGE_UP and never registered.
    RIGHT = 262, LEFT = 263, DOWN = 264, UP = 265,

    // Modifier keys
    LEFT_SHIFT = 340, RIGHT_SHIFT,
    LEFT_CONTROL = 341, RIGHT_CONTROL,
    LEFT_ALT = 342, RIGHT_ALT,
};

// ==========================================
// ENUM: MouseButton
// ==========================================

/**
 * @enum MouseButton
 * @brief Mouse button codes.
 */
enum class MouseButton : i32 {
    LEFT = 0,     ///< Left mouse button.
    RIGHT = 1,    ///< Right mouse button.
    MIDDLE = 2,   ///< Middle mouse button (scroll wheel click).
    BUTTON_4 = 3, ///< Additional mouse button (often "back").
    BUTTON_5 = 4, ///< Additional mouse button (often "forward").
};

// ==========================================
// CLASS: Input
// ==========================================

/**
 * @class Input
 * @brief Static input state manager.
 * 
 * All methods are static. Input state is updated once per frame by calling
 * Input::Update() after Window::Update().
 * 
 * Key state terminology:
 *   - IsKeyDown: Key is currently held down (true every frame while held)
 *   - IsKeyPressed: Key was pressed THIS frame (true for exactly one frame)
 *   - IsKeyReleased: Key was released THIS frame (true for exactly one frame)
 */
class Input {
public:
    /**
     * @brief Initializes the input system.
     * 
     * Resets all input state to defaults. Must be called after Window::Create()
     * because GLFW must be initialized for key state queries to work.
     */
    static void Initialize();

    /**
     * @brief Updates input state from GLFW.
     * 
     * Implementation:
     *   1. Query current key states from GLFW for all tracked keys
     *   2. Compute "pressed" and "released" by comparing current vs previous frame
     *      - pressed = (current == pressed) AND (previous == not pressed)
     *      - released = (current == not pressed) AND (previous == pressed)
     *   3. Query current mouse position from GLFW
     *   4. Compute mouse delta from current vs previous position
     *   5. Query mouse button states
     *   6. Query scroll wheel delta
     *   7. Reset scroll delta after reading (scroll is per-event, not continuous)
     * 
     * @note Call once per frame after Window::Update().
     */
    static void Update();

    // ==========================================
    // KEYBOARD
    // ==========================================

    /**
     * @brief Checks if a key is currently held down.
     * @param key The key to check.
     * @return true if the key is down this frame, false otherwise.
     */
    static b8 IsKeyDown(KeyCode key);

    /**
     * @brief Checks if a key was pressed THIS frame.
     * @param key The key to check.
     * @return true if the key transitioned from up to down this frame.
     */
    static b8 IsKeyPressed(KeyCode key);

    /**
     * @brief Checks if a key was released THIS frame.
     * @param key The key to check.
     * @return true if the key transitioned from down to up this frame.
     */
    static b8 IsKeyReleased(KeyCode key);

    // ==========================================
    // MOUSE
    // ==========================================

    /** @brief Gets the current mouse X position in window coordinates. */
    static f32 GetMouseX();

    /** @brief Gets the current mouse Y position in window coordinates. */
    static f32 GetMouseY();

    /** @brief Gets the mouse position as a Vec2. */
    static Vec2 GetMousePosition();

    /** @brief Gets the mouse X movement delta since last frame. */
    static f32 GetMouseDeltaX();

    /** @brief Gets the mouse Y movement delta since last frame. */
    static f32 GetMouseDeltaY();

    /** @brief Gets the mouse movement delta as a Vec2. */
    static Vec2 GetMouseDelta();

    /**
     * @brief Checks if a mouse button is currently held down.
     * @param button The button to check.
     * @return true if the button is down this frame.
     */
    static b8 IsMouseButtonDown(MouseButton button);

    /**
     * @brief Checks if a mouse button was pressed THIS frame.
     * @param button The button to check.
     * @return true if the button transitioned from up to down this frame.
     */
    static b8 IsMouseButtonPressed(MouseButton button);

    /**
     * @brief Gets the scroll wheel delta since last frame.
     * @return Scroll delta. Positive = scroll up/away, negative = scroll down/toward.
     * @note Scroll delta is reset to zero after each Update() call.
     */
    static f32 GetScrollDelta();

    // ==========================================
    // TEXT INPUT (for the in-game command box)
    // ==========================================

    /// @brief Begins capturing typed characters (clears the buffer first).
    static void BeginTextInput();
    /// @brief Stops capturing typed text.
    static void EndTextInput();
    /// @brief Whether text capture is currently active.
    static b8 IsTextInputActive();
    /// @brief The text typed so far.
    static const std::string& GetTextInput();
    /// @brief Removes the last character from the buffer (backspace).
    static void Backspace();

private:
    /** @brief Private constructor. Enforces static-only access. */
    Input() = default;

    /** @brief GLFW character callback — appends typed chars while capture is active. */
    static void CharCallback(GLFWwindow* window, unsigned int codepoint);

    /** @brief Number of tracked keys (covers KeyCode range). */
    static constexpr i32 KEY_COUNT = 350;

    /** @brief Current frame key state: true = down, false = up. */
    static b8 s_keys[KEY_COUNT];
    /** @brief Previous frame key state. Used to detect press/release transitions. */
    static b8 s_prevKeys[KEY_COUNT];

    /** @brief Current frame mouse button state. */
    static b8 s_mouseButtons[5];
    /** @brief Previous frame mouse button state. */
    static b8 s_prevMouseButtons[5];

    /** @brief Current mouse X position. */
    static f32 s_mouseX;
    /** @brief Current mouse Y position. */
    static f32 s_mouseY;
    /** @brief Previous mouse X position. */
    static f32 s_prevMouseX;
    /** @brief Previous mouse Y position. */
    static f32 s_prevMouseY;

    /** @brief Scroll wheel delta this frame. Reset after each Update(). */
    static f32 s_scrollDelta;

    /** @brief Captured text buffer (the in-game command box). */
    static std::string s_textInput;
    /** @brief Whether text capture is active. */
    static b8 s_textActive;
};

} // namespace Dnd
