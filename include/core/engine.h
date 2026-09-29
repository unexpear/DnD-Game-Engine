/**
 * @file engine.h
 * @brief Engine core: lifecycle management, main loop, subsystem orchestration.
 * 
 * The Engine class is the root of the entire application. It controls:
 *   1. Initialization order of all subsystems
 *   2. The main game loop (update -> render -> present)
 *   3. Graceful shutdown in reverse initialization order
 *   4. Engine state transitions (INITIALIZING -> RUNNING -> SHUTTING_DOWN)
 * 
 * Initialization order (critical — dependencies must be initialized first):
 *   1. Logger  — Everything else needs logging
 *   2. Time    — Frame timing needed by all systems
 *   3. Window  — Needs logger for error reporting
 *   4. Input   — Needs window for event callbacks
 *   5. Renderer — Needs window for OpenGL context
 *   6. Debug Draw — Needs renderer for shader compilation
 * 
 * Shutdown order is the REVERSE of initialization.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.2.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include "scene/scene_manager.h"

namespace Dnd {

// Forward declarations
class Camera;

// ==========================================
// ENUM: EngineState
// ==========================================

/**
 * @enum EngineState
 * @brief High-level engine lifecycle states.
 */
/**
 * @enum ViewMode
 * @brief Where the player is looking from.
 *
 * Tabletop is the orbit camera over the table. Immersed is a camera
 * inside the environment, behind the active figure. One mode is active.
 */
enum class ViewMode : i32 {
    Tabletop,   ///< Overall view. Current demo orbit.
    Immersed    ///< In-game view behind the subject.
};

enum class EngineState : i32 {
    INITIALIZING,   ///< Subsystems are being set up. Not ready for game logic.
    RUNNING,        ///< Main loop active. Game logic and rendering executing.
    PAUSED,         ///< Main loop running but game logic skipped. Rendering continues.
    SHUTTING_DOWN   ///< Cleanup in progress. No game logic or rendering.
};

// ==========================================
// STRUCT: EngineConfig
// ==========================================

/**
 * @struct EngineConfig
 * @brief Configuration parameters for engine initialization.
 */
struct EngineConfig {
    /** @brief Name of the application. Displayed in window title and logs. */
    const char* applicationName;
    /** @brief Path to log file. nullptr for console-only logging. */
    const char* logFilePath;
    /** @brief Initial window width in pixels. Default: 1280. */
    i32 windowWidth;
    /** @brief Initial window height in pixels. Default: 720. */
    i32 windowHeight;
    /** @brief Whether VSync is enabled. Default: true. */
    b8  vsyncEnabled;
    /** @brief Whether to start in fullscreen. Default: false. */
    b8  fullscreenEnabled;
    /**
     * @brief Campaign pack to load instead of the D&D demo.
     * nullptr or empty selects the canonical "dnd" pack.
     * The pointer must outlive Engine::Initialize (argv storage is fine).
     */
    const char* campaignId;

    /** @brief Default constructor with sensible defaults for development. */
    EngineConfig();
};

// ==========================================
// CLASS: Engine
// ==========================================

/**
 * @class Engine
 * @brief Singleton engine manager controlling the application lifecycle.
 * 
 * Manages initialization, main loop, and shutdown of all engine subsystems.
 * The main loop runs at VSync-controlled framerate with capped delta time.
 */
class Engine {
public:
    /**
     * @brief Initializes the engine and all subsystems.
     * 
     * Initialization order:
     *   1. Logger
     *   2. Time
     *   3. Window (GLFW + OpenGL context)
     *   4. Input
     *   5. Renderer (GLAD + OpenGL state)
     *   6. Debug Draw (debug shader + buffers)
     *   7. Scene (creates test geometry)
     * 
     * @param config Engine configuration parameters.
     * @return true if all subsystems initialized successfully.
     */
    static b8 Initialize(const EngineConfig& config);

    /**
     * @brief Runs the main engine loop.
     * 
     * Per-frame sequence:
     *   1. Time::Update()
     *   2. Window::Update() — poll events
     *   3. Input::Update() — refresh input state
     *   4. Check window close -> break if true
     *   5. Handle input (camera orbit, escape to quit)
     *   6. Renderer::BeginFrame() — clear buffers
     *   7. RenderScene() — draw all geometry
     *   8. DebugDraw::Flush() — render debug geometry
     *   9. Renderer::EndFrame() — swap buffers
     * 
     * Blocks until the engine shuts down.
     */
    static void Run();

    /**
     * @brief Shuts down the engine and all subsystems.
     * 
     * Reverse order: Debug Draw -> Renderer -> Input -> Window -> Time -> Logger.
     * Safe to call multiple times.
     */
    static void Shutdown();

    /** @brief Gets the current engine state. */
    static EngineState GetState();

    /** @brief Sets the engine state. */
    static void SetState(EngineState state);

    /**
     * @brief Sets the point the orbit camera follows (smoothly).
     *
     * The active scene calls this each frame to make the camera track the hero
     * as he explores. If never called, the camera stays on its default focus.
     * @param focus World-space point to center the orbit on.
     */
    static void SetCameraFocus(const Vec3& focus);

    /**
     * @brief Publishes the figure the immersed camera should follow.
     *
     * Tabletop view ignores this and keeps the orbit. Immersed view sits
     * behind @p position, looking along @p yawRadians plus mouse look.
     * The D&D demo publishes Finn. A pack publishes its active figure.
     * Cleared by ResetViewToBase.
     */
    static void SetViewSubject(const Vec3& position, f32 yawRadians);

    /** @brief Yaw the camera is using, for pack movement (radians). */
    static f32 GetViewYaw();

    /** @brief Overall table orbit, or a camera inside the environment. */
    static ViewMode GetViewMode();

    /** @brief Switches tabletop and immersed. F3 calls this. */
    static void ToggleViewMode();

    /**
     * @brief Names the control scheme that owns translation keys.
     *
     * "base" (or null/empty) is the D&D default: WASD orbits the table.
     * Any other name means the active pack reads WASD itself. Unloading
     * a pack must call ResetViewToBase, which restores "base".
     */
    static void SetPackControlScheme(const char* name);

    /** @brief True when WASD still belongs to the tabletop camera. */
    static b8 UsesBaseControls();

    /** @brief Active scheme name. "base" is the D&D default. */
    static const char* GetControlScheme();

    /**
     * @brief Restores the orbit camera, tabletop view, and base controls.
     *
     * Called when a campaign pack is activated so the previous pack's
     * focus, zoom, angles, view mode, and control scheme do not carry over.
     */
    static void ResetViewToBase();

private:
    /** @brief Private constructor. */
    Engine() = default;

    /**
     * @brief Renders the 3D scene.
     * 
     * Draws the test triangle, cube, and table quad with proper
     * view and projection matrices.
     * 
     * @param deltaTime Time since last frame.
     */
    static void RenderScene(f32 deltaTime);

    /**
     * @brief Handles input for camera orbit and engine controls.
     * 
     * Implementation:
     *   - Right mouse drag: orbit camera around target
     *   - Scroll wheel: zoom in/out
     *   - Escape key: trigger shutdown
     */
    static void HandleInput();

    /** @brief Current engine state. */
    static EngineState s_state;
    /** @brief Whether the engine has been initialized. */
    static b8 s_initialized;
    /** @brief Engine configuration copy. */
    static EngineConfig s_config;
    /** @brief Main scene camera. */
    static Camera* s_camera;
    /** @brief Camera orbit angle (yaw). */
    static f32 s_cameraYaw;
    /** @brief Camera orbit angle (pitch). */
    static f32 s_cameraPitch;
    /** @brief Camera orbit distance from target. */
    static f32 s_cameraDistance;
    /** @brief Whether right mouse was held last frame (for orbit tracking). */
    static b8 s_wasRightMouseDown;
    /** @brief Point the camera follows (set by the active scene to track the hero). */
    static Vec3 s_cameraFocus;
    /** @brief Smoothed focus actually used as the orbit center (eases toward s_cameraFocus). */
    static Vec3 s_cameraFocusSmoothed;
};

} // namespace Dnd
