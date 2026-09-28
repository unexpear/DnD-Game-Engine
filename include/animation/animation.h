/**
 * @file animation.h
 * @brief Animation state component for skeletal animation playback.
 * 
 * The Animation component stores the current playback state:
 *   - Which animation clip is playing
 *   - Current time in the animation
 *   - Playback speed and loop settings
 * 
 * Multiple Animation components can reference the same AnimationClip
 * (resource sharing). The AnimationSystem updates all Animation
 * components each frame.
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.5.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include <string>

namespace Dnd {

// Forward declaration
class AnimationClip;

// ==========================================
// ENUM: AnimationPlayMode
// ==========================================

/**
 * @enum AnimationPlayMode
 * @brief How the animation plays back.
 */
enum class AnimationPlayMode : i32 {
    ONCE,       ///< Play once, then stop
    LOOP,       ///< Loop continuously
    PING_PONG   ///< Play forward, then backward, then forward...
};

// ==========================================
// STRUCT: Animation
// ==========================================

/**
 * @struct Animation
 * @brief ECS component: animation playback state.
 * 
 * Attach this to an entity that has a Skeleton component
 * and a MeshRenderer with a skinned mesh.
 * 
 * Usage:
 * @code
 * Entity entity = entityManager.Create();
 * Skeleton* sk = componentRegistry.AddComponent<Skeleton>(entity);
 * Animation* anim = componentRegistry.AddComponent<Animation>(entity);
 * anim->SetClip(walkClip);
 * anim->Play();
 * @endcode
 */
struct Animation {
    AnimationClip* clip;       ///< Current animation clip (resource pointer, not owned)
    f32 time;                  ///< Current time in seconds
    f32 speed;                 ///< Playback speed multiplier (1.0 = normal)
    b8 isPlaying;              ///< Whether animation is currently playing
    b8 loop;                  ///< Whether to loop (overrides playMode for simplicity)
    AnimationPlayMode playMode; ///< How to play the animation

    /// @brief Default constructor (no clip, stopped).
    Animation()
        : clip(nullptr), time(0.0f), speed(1.0f), isPlaying(false),
          loop(false), playMode(AnimationPlayMode::ONCE) {}

    /**
     * @brief Sets the animation clip.
     * @param newClip Pointer to the animation clip resource.
     */
    void SetClip(AnimationClip* newClip) {
        clip = newClip;
        time = 0.0f;
    }

    /**
     * @brief Starts or resumes playback.
     */
    void Play() { isPlaying = true; }

    /**
     * @brief Pauses playback.
     */
    void Pause() { isPlaying = false; }

    /**
     * @brief Stops playback and resets to time 0.
     */
    void Stop() { isPlaying = false; time = 0.0f; }

    /**
     * @brief Updates the animation time.
     * 
     * Called by AnimationSystem each frame.
     * 
     * @param deltaTime Frame delta time in seconds.
     */
    void Update(f32 deltaTime);

    /**
     * @brief Gets the current animation time.
     * @return Time in seconds.
     */
    f32 GetTime() const { return time; }

    /**
     * @brief Sets the current animation time.
     * @param t Time in seconds.
     */
    void SetTime(f32 t) { time = t; }

    /**
     * @brief Checks if the animation has finished (non-looping).
     * @return true if finished.
     */
    b8 IsFinished() const;
};

} // namespace Dnd
