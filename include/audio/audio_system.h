/**
 * @file audio_system.h
 * @brief System for updating audio sources and 3D audio.
 * 
 * The AudioSystem processes entities with AudioSource components.
 * It handles playback state, 3D spatial audio, and volume control.
 * 
 * Uses miniaudio as the audio playback backend (opaque PIMPL).
 * 
 * @author DND Engine Team
 * @date 2026-06-06
 * @version 0.7.0
 */

#pragma once

#include "ecs/system.h"
#include "core/math.h"

namespace Dnd {

struct AudioSource;
struct AudioListener;
class AudioClip;

/**
 * @class AudioSystem
 * @brief ECS system for audio playback and 3D spatial audio.
 */
class AudioSystem : public System {
public:
    AudioSystem();
    ~AudioSystem();

protected:
    void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) override;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Dnd
