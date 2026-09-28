/**
 * @file time.h
 * @brief High-resolution time tracking and frame timing management.
 * 
 * Provides frame delta time, total elapsed time, frame counting, and
 * smoothed FPS calculation. All timing uses the platform's high-resolution
 * performance counter for sub-millisecond accuracy.
 * 
 * Key features:
 *   - Delta time capped at 100ms to prevent physics spiral-of-death after pauses
 *   - Smoothed FPS over last 60 frames (not instantaneous, which is noisy)
 *   - Total elapsed time since engine start
 * 
 * @note All times are in seconds (f64 for precision in total elapsed).
 * @warning Delta time is NOT constant. It varies frame to frame.
 *          Physics should use fixed timestep, rendering uses variable.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include <chrono>

namespace Dnd {

// ==========================================
// CLASS: Time
// ==========================================

/**
 * @class Time
 * @brief Static time management utility.
 * 
 * All methods are static. No instance needed. Call Time::Initialize() once
 * at engine startup, then Time::Update() once per frame before any game logic.
 * 
 * Implementation uses std::chrono::high_resolution_clock internally.
 * On Windows, this maps to QueryPerformanceCounter.
 * On Linux, this maps to clock_gettime(CLOCK_MONOTONIC).
 */
class Time {
public:
    /**
     * @brief Initializes the time system.
     * 
     * Implementation:
     *   1. Record the current high-resolution clock value as s_startTime
     *   2. Set s_lastFrameTime = s_startTime
     *   3. Reset s_deltaTime = 0, s_totalTime = 0, s_frameCount = 0
     *   4. Reset FPS tracking array (s_fpsHistory, all zeros)
     *   5. Set s_fpsHistoryIndex = 0
     *   6. Set s_isInitialized = true
     */
    static void Initialize();

    /**
     * @brief Called once per frame to update time state.
     * 
     * This MUST be called at the START of each frame, before any game logic.
     * 
     * Implementation:
     *   1. Get current high-resolution clock time as 'now'
     *   2. Compute raw delta: rawDelta = now - s_lastFrameTime (in seconds)
     *   3. Cap delta: s_deltaTime = min(rawDelta, 0.1)  // 100ms max
     *      - Prevents physics explosion after pauses (spiral of death)
     *   4. Update total time: s_totalTime += s_deltaTime
     *   5. Update last frame time: s_lastFrameTime = now
     *   6. Increment frame count: s_frameCount++
     *   7. Update FPS history:
     *      - Compute instantaneous FPS: 1.0 / s_deltaTime (if dt > 0)
     *      - Store in s_fpsHistory[s_fpsHistoryIndex]
     *      - Advance index: s_fpsHistoryIndex = (s_fpsHistoryIndex + 1) % 60
     * 
     * @warning If called multiple times per frame, results will be incorrect.
     */
    static void Update();

    /**
     * @brief Pauses or unpauses the game simulation.
     * @param paused Whether the simulation should be paused.
     */
    static void SetPaused(b8 paused);

    /**
     * @brief Returns whether the game simulation is paused.
     */
    static b8 GetPaused();

    /**
     * @brief Gets the time elapsed since the last frame.
     * @return Delta time in seconds, capped at 0.1 (100ms). Returns 0 on first frame, or when paused.
     */
    static f64 GetDeltaTime();

    /**
     * @brief Gets the total elapsed time since engine initialization.
     * @return Total time in seconds. Monotonically increasing.
     */
    static f64 GetTotalTime();

    /**
     * @brief Gets the total number of frames rendered since initialization.
     * @return Frame count. Starts at 0, increments by 1 each Update() call.
     */
    static u64 GetFrameCount();

    /**
     * @brief Gets the smoothed FPS over the last 60 frames.
     * 
     * Averages the last 60 instantaneous FPS values.
     * If fewer than 60 frames have elapsed, averages whatever is available.
     * 
     * @return Smoothed FPS as f64. Returns 0 on first frame (no data yet).
     */
    static f64 GetFPS();

private:
    /// @brief Time point of engine start (high-resolution clock).
    static std::chrono::high_resolution_clock::time_point s_startTime;
    /// @brief Time point of the previous frame.
    static std::chrono::high_resolution_clock::time_point s_lastFrameTime;
    /// @brief Time elapsed since last frame (capped at 0.1s).
    static f64 s_deltaTime;
    /// @brief Total time since engine start.
    static f64 s_totalTime;
    /// @brief Total frames rendered.
    static u64 s_frameCount;
    /// @brief Circular buffer of last 60 FPS values.
    static f64 s_fpsHistory[60];
    /// @brief Current write index into s_fpsHistory.
    static i32 s_fpsHistoryIndex;
    /// @brief Whether Time has been initialized.
    static b8 s_isInitialized;
    /// @brief Whether the game simulation is paused.
    static b8 s_paused;
};

} // namespace Dnd
