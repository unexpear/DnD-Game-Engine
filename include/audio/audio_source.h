/**
 * @file audio_source.h
 * @brief AudioSource ECS component for playing audio clips.
 * 
 * The AudioSource component attaches to an entity to play
 * audio clips. It supports:
 *   - 2D audio (no position attenuation)
 *   - 3D audio (position-based attenuation)
 *   - Looping, volume, pitch control
 *   - Play/pause/stop controls
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.6.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"

namespace Dnd {

// Forward declaration
class AudioClip;

// ==========================================
// ENUM: AudioSourceState
// ==========================================

/**
 * @enum AudioSourceState
 * @brief Playback state of an audio source.
 */
enum class AudioSourceState : i32 {
    STOPPED,    ///< Not playing
    PLAYING,    ///< Currently playing
    PAUSED      ///< Paused (can be resumed)
};

// ==========================================
// STRUCT: AudioSource
// ==========================================

/**
 * @struct AudioSource
 * @brief ECS component: audio playback for an entity.
 * 
 * Attach this to an entity to play audio. The entity's
 * Transform position is used for 3D audio attenuation.
 * 
 * Usage:
 * @code
 * Entity entity = entityManager.Create();
 * AudioSource* source = componentRegistry.AddComponent<AudioSource>(entity);
 * source->clip = explosionClip;
 * source->Play();
 * @endcode
 */
struct AudioSource {
    AudioClip* clip;           ///< Audio clip to play (resource pointer)
    f32 volume;                ///< Volume multiplier (0.0 to 1.0)
    f32 pitch;                 ///< Pitch multiplier (1.0 = normal)
    b8 isLooping;              ///< Whether to loop the audio
    b8 is3D;                  ///< Whether to use 3D attenuation
    f32 minDistance;           ///< Min distance for 3D falloff
    f32 maxDistance;           ///< Max distance for 3D falloff
    AudioSourceState state;     ///< Current playback state

    /// @brief Default constructor.
    AudioSource()
        : clip(nullptr), volume(1.0f), pitch(1.0f), isLooping(false),
          is3D(false), minDistance(1.0f), maxDistance(50.0f),
          state(AudioSourceState::STOPPED) {}

    /**
     * @brief Sets the audio clip.
     * @param newClip Pointer to audio clip resource.
     */
    void SetClip(AudioClip* newClip) {
        clip = newClip;
        state = AudioSourceState::STOPPED;
    }

    /// @brief Starts playback.
    void Play() { state = AudioSourceState::PLAYING; }

    /// @brief Pauses playback (can be resumed).
    void Pause() { state = AudioSourceState::PAUSED; }

    /// @brief Stops and rewinds.
    void Stop() { state = AudioSourceState::STOPPED; }

    /// @brief Checks if currently playing.
    b8 IsPlaying() const { return state == AudioSourceState::PLAYING; }
};

} // namespace Dnd
