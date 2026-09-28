/**
 * @file audio_listener.h
 * @brief AudioListener ECS component: defines the ears in 3D space.
 * 
 * The AudioListener component marks an entity as the "ears"
 * of the scene. Typically attached to the camera entity.
 * 
 * 3D audio is attenuated based on:
 *   - Distance to AudioSource
 *   - AudioSource min/max distance
 *   - Listener orientation (for directional audio)
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.6.0
 */

#pragma once

#include "core/types.h"

namespace Dnd {

// ==========================================
// STRUCT: AudioListener
// ==========================================

/**
 * @struct AudioListener
 * @brief ECS component: the ears in the 3D scene.
 * 
 * Only one entity should have an AudioListener component
 * (the first one found is used). Typically attached to
 * the camera entity.
 * 
 * Usage:
 * @code
 * Entity camera = entityManager.Create();
 * componentRegistry.AddComponent<AudioListener>(camera);
 * @endcode
 */
struct AudioListener {
    f32 volume;    ///< Master volume for this listener (0.0 to 1.0)

    /// @brief Default constructor (full volume).
    AudioListener() : volume(1.0f) {}
};

} // namespace Dnd
