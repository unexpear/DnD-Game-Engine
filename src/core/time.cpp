/**
 * @file time.cpp
 * @brief Implementation of the Time class.
 * 
 * See time.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "core/time.h"
#include <algorithm>

namespace Dnd {

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

/** @brief Time point of engine start. */
std::chrono::high_resolution_clock::time_point Time::s_startTime;

/** @brief Time point of the previous frame. */
std::chrono::high_resolution_clock::time_point Time::s_lastFrameTime;

/** @brief Time elapsed since last frame (capped at 0.1s). */
f64 Time::s_deltaTime = 0.0;

/** @brief Total time since engine start. */
f64 Time::s_totalTime = 0.0;

/** @brief Total frames rendered. */
u64 Time::s_frameCount = 0;

/** @brief Circular buffer of last 60 FPS values. Initialized to zeros. */
f64 Time::s_fpsHistory[60] = {0};

/** @brief Current write index into s_fpsHistory. */
i32 Time::s_fpsHistoryIndex = 0;

/** @brief Whether Time has been initialized. */
b8 Time::s_isInitialized = false;

/** @brief Whether the game simulation is paused. */
b8 Time::s_paused = false;

// ==========================================
// INITIALIZATION
// ==========================================

void Time::Initialize() {
    // Step 1: Record the current high-resolution clock value as start time
    s_startTime = std::chrono::high_resolution_clock::now();

    // Step 2: Set last frame time equal to start time
    s_lastFrameTime = s_startTime;

    // Step 3: Reset all counters
    s_deltaTime = 0.0;
    s_totalTime = 0.0;
    s_frameCount = 0;

    // Step 4: Reset FPS tracking array
    for (i32 i = 0; i < 60; i++) {
        s_fpsHistory[i] = 0.0;
    }

    // Step 5: Reset FPS history index
    s_fpsHistoryIndex = 0;

    // Step 6: Mark as initialized
    s_isInitialized = true;
}

// ==========================================
// FRAME UPDATE
// ==========================================

void Time::Update() {
    // Step 1: Get current high-resolution clock time
    auto now = std::chrono::high_resolution_clock::now();

    // Step 2: Compute raw delta time in seconds
    std::chrono::duration<f64> duration = now - s_lastFrameTime;
    f64 rawDelta = duration.count();

    // Step 3: Cap delta at 100ms to prevent physics spiral-of-death after pauses
    // If the game is paused (debugging, window minimized), a 30-second pause
    // would cause physics to explode with dt=30. Capping prevents this.
    s_deltaTime = std::min(rawDelta, 0.1);

    // Step 4: Update total time
    s_totalTime += s_deltaTime;

    // Step 5: Update last frame time
    s_lastFrameTime = now;

    // Step 6: Increment frame count
    s_frameCount++;

    // Step 7: Update FPS history (circular buffer)
    if (s_deltaTime > 0.0) {
        // Compute instantaneous FPS
        f64 instantFPS = 1.0 / s_deltaTime;
        // Store in circular buffer
        s_fpsHistory[s_fpsHistoryIndex] = instantFPS;
        // Advance index (wraps at 60)
        s_fpsHistoryIndex = (s_fpsHistoryIndex + 1) % 60;
    }
}

// ==========================================
// ACCESSORS
// ==========================================

void Time::SetPaused(b8 paused) {
    s_paused = paused;
}

b8 Time::GetPaused() {
    return s_paused;
}

f64 Time::GetDeltaTime() {
    return s_paused ? 0.0 : s_deltaTime;
}

f64 Time::GetTotalTime() {
    return s_totalTime;
}

u64 Time::GetFrameCount() {
    return s_frameCount;
}

f64 Time::GetFPS() {
    // On first frame (frame count = 0), no data yet
    if (s_frameCount == 0) {
        return 0.0;
    }

    // Sum all non-zero entries in the FPS history
    f64 sum = 0.0;
    i32 count = 0;

    // Count how many entries we've actually written (min of frameCount and 60)
    i32 validCount = static_cast<i32>(s_frameCount < 60 ? s_frameCount : 60);

    for (i32 i = 0; i < validCount; i++) {
        if (s_fpsHistory[i] > 0.0) {
            sum += s_fpsHistory[i];
            count++;
        }
    }

    // Return average (guard against divide by zero)
    return count > 0 ? sum / count : 0.0;
}

} // namespace Dnd
