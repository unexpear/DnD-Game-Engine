/**
 * @file audio_system.cpp
 * @brief Implementation of AudioSystem with miniaudio backend.
 * 
 * Uses miniaudio engine for audio playback with per-source ma_sound
 * handles. A custom PCM data source (PcmDataSource) wraps AudioClip's
 * decoded f32 buffer for in-memory playback without intermediate files.
 * 
 * @author DND Engine Team
 * @date 2026-06-06
 * @version 0.7.0
 */

#include "audio/audio_system.h"
#include "audio/audio_source.h"
#include "audio/audio_listener.h"
#include "audio/audio_clip.h"
#include "ecs/components/transform.h"
#include "core/log.h"

#define MINIAUDIO_NO_IMPLEMENTATION
#include "miniaudio.h"

#include <vector>
#include <cstring>

namespace Dnd {

// ==========================================
// Custom PCM Data Source
// ==========================================

struct PcmDataSource {
    ma_data_source_base base;
    const f32* pcmData;
    u64 totalFrames;
    u32 channels;
    u32 sampleRate;
    u64 cursor;
};

static ma_result pcm_ds_read(ma_data_source* pDataSource, void* pFramesOut, u64 frameCount, u64* pFramesRead) {
    PcmDataSource* ds = (PcmDataSource*)pDataSource;
    u64 remaining = ds->totalFrames - ds->cursor;
    u64 toRead = (frameCount < remaining) ? frameCount : remaining;
    if (toRead > 0) {
        memcpy(pFramesOut, ds->pcmData + ds->cursor * ds->channels, toRead * ds->channels * sizeof(f32));
        ds->cursor += toRead;
    }
    *pFramesRead = toRead;
    return MA_SUCCESS;
}

static ma_result pcm_ds_seek(ma_data_source* pDataSource, u64 frameIndex) {
    PcmDataSource* ds = (PcmDataSource*)pDataSource;
    if (frameIndex > ds->totalFrames) return MA_INVALID_ARGS;
    ds->cursor = frameIndex;
    return MA_SUCCESS;
}

static ma_result pcm_ds_get_data_format(ma_data_source* pDataSource, ma_format* pFormat, u32* pChannels, u32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap) {
    (void)pChannelMap;
    (void)channelMapCap;
    PcmDataSource* ds = (PcmDataSource*)pDataSource;
    *pFormat = ma_format_f32;
    *pChannels = ds->channels;
    *pSampleRate = ds->sampleRate;
    return MA_SUCCESS;
}

static ma_result pcm_ds_get_cursor(ma_data_source* pDataSource, u64* pCursor) {
    *pCursor = ((PcmDataSource*)pDataSource)->cursor;
    return MA_SUCCESS;
}

static ma_result pcm_ds_get_length(ma_data_source* pDataSource, u64* pLength) {
    *pLength = ((PcmDataSource*)pDataSource)->totalFrames;
    return MA_SUCCESS;
}

static ma_data_source_vtable g_pcmVTable = {
    pcm_ds_read,
    pcm_ds_seek,
    pcm_ds_get_data_format,
    pcm_ds_get_cursor,
    pcm_ds_get_length
};

static void pcm_ds_init(PcmDataSource* ds, const f32* data, u64 totalFrames, u32 channels, u32 sampleRate) {
    memset(ds, 0, sizeof(*ds));
    ds->base.vtable = &g_pcmVTable;
    ds->pcmData = data;
    ds->totalFrames = totalFrames;
    ds->channels = channels;
    ds->sampleRate = sampleRate;
    ds->cursor = 0;
}

// ==========================================
// Active Sound (per entity)
// ==========================================

struct AudioSystem::Impl {
    ma_engine engine;
    b8 initialized;

    struct SoundSlot {
        ma_sound sound;
        PcmDataSource dataSource;
        std::vector<f32> pcmCopy;
        u32 entityIndex;
        b8 active;
    };

    std::vector<SoundSlot> slots;
};

AudioSystem::AudioSystem()
    : System("AudioSystem", SystemPhase::UPDATE, 6)
    , m_impl(new Impl())
{
    m_impl->initialized = false;

    ma_engine_config config = ma_engine_config_init();
    if (ma_engine_init(&config, &m_impl->engine) == MA_SUCCESS) {
        m_impl->initialized = true;
        DND_LOG_INFO("AudioSystem: miniaudio engine initialized");
    } else {
        DND_LOG_ERROR("AudioSystem: failed to initialize miniaudio engine");
    }
}

AudioSystem::~AudioSystem() {
    if (m_impl) {
        for (auto& slot : m_impl->slots) {
            if (slot.active) {
                ma_sound_uninit(&slot.sound);
            }
        }
        m_impl->slots.clear();

        if (m_impl->initialized) {
            ma_engine_uninit(&m_impl->engine);
            DND_LOG_INFO("AudioSystem: miniaudio engine shut down");
        }
        delete m_impl;
        m_impl = nullptr;
    }
}

void AudioSystem::OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) {
    (void)deltaTime;
    (void)em;

    if (!m_impl || !m_impl->initialized) return;

    auto* sourceSet = cr.GetSparseSet<AudioSource>();
    auto* listenerSet = cr.GetSparseSet<AudioListener>();
    auto* transformSet = cr.GetSparseSet<Transform>();

    // Clean up orphaned slots (entity removed)
    if (sourceSet) {
        for (auto& slot : m_impl->slots) {
            if (slot.active) {
                b8 alive = false;
                for (u32 si = 0; si < sourceSet->Size(); si++) {
                    if (sourceSet->GetEntityIndex(si) == slot.entityIndex) {
                        alive = true;
                        break;
                    }
                }
                if (!alive) {
                    ma_sound_stop(&slot.sound);
                    ma_sound_uninit(&slot.sound);
                    slot.active = false;
                    slot.pcmCopy.clear();
                }
            }
        }
    }

    if (!sourceSet || sourceSet->Empty()) return;

    Vec3 listenerPos(0, 0, 0);
    f32 listenerVolume = 1.0f;
    if (listenerSet && !listenerSet->Empty()) {
        u32 li = listenerSet->GetEntityIndex(0);
        const AudioListener* listener = listenerSet->Get(li);
        if (listener) {
            listenerVolume = listener->volume;
            const Transform* lt = transformSet ? transformSet->Get(li) : nullptr;
            if (lt) listenerPos = lt->position;
        }
        ma_engine_listener_set_position(&m_impl->engine, 0, listenerPos.x, listenerPos.y, listenerPos.z);
    }

    for (u32 i = 0; i < sourceSet->Size(); i++) {
        AudioSource& source = const_cast<AudioSource&>(sourceSet->GetDense()[i]);
        u32 entityIndex = sourceSet->GetEntityIndex(i);

        const Transform* st = transformSet ? transformSet->Get(entityIndex) : nullptr;
        Vec3 sourcePos = st ? st->position : Vec3(0, 0, 0);

        if (source.IsPlaying() && source.clip) {
            b8 found = false;
            for (auto& slot : m_impl->slots) {
                if (slot.active && slot.entityIndex == entityIndex) {
                    found = true;
                    ma_sound_set_volume(&slot.sound, source.volume * listenerVolume);
                    if (source.is3D) {
                        ma_sound_set_position(&slot.sound, sourcePos.x, sourcePos.y, sourcePos.z);
                    }
                    break;
                }
            }
            if (!found) {
                Impl::SoundSlot slot;
                slot.entityIndex = entityIndex;
                slot.active = false;

                u64 totalFrames = source.clip->GetTotalFrames();
                u32 channels = source.clip->GetNumChannels();
                u32 sampleRate = source.clip->GetSampleRate();

                if (totalFrames == 0) continue;

                const f32* srcData = reinterpret_cast<const f32*>(source.clip->GetData());
                if (!srcData) continue;

                slot.pcmCopy.assign(srcData, srcData + totalFrames * channels);

                pcm_ds_init(&slot.dataSource, slot.pcmCopy.data(), totalFrames, channels, sampleRate);

                if (ma_sound_init_from_data_source(&m_impl->engine, &slot.dataSource.base, 0, nullptr, &slot.sound) != MA_SUCCESS) {
                    DND_LOG_WARN("AudioSystem: failed to create sound for entity index %u", entityIndex);
                    continue;
                }

                ma_sound_start(&slot.sound);

                if (source.isLooping) {
                    ma_sound_set_looping(&slot.sound, MA_TRUE);
                }

                if (source.is3D) {
                    ma_sound_set_position(&slot.sound, sourcePos.x, sourcePos.y, sourcePos.z);
                    ma_sound_set_attenuation_model(&slot.sound, ma_attenuation_model_linear);
                    ma_sound_set_min_distance(&slot.sound, source.minDistance < 0.01f ? 1.0f : source.minDistance);
                    ma_sound_set_max_distance(&slot.sound, source.maxDistance < 0.01f ? 50.0f : source.maxDistance);
                }

                ma_sound_set_volume(&slot.sound, source.volume * listenerVolume);

                slot.active = true;
                m_impl->slots.push_back(std::move(slot));
            }
        } else {
            for (auto& slot : m_impl->slots) {
                if (slot.active && slot.entityIndex == entityIndex) {
                    ma_sound_stop(&slot.sound);
                    ma_sound_uninit(&slot.sound);
                    slot.active = false;
                    break;
                }
            }
        }
    }

    // Remove inactive slots
    for (i64 i = static_cast<i64>(m_impl->slots.size()) - 1; i >= 0; i--) {
        if (!m_impl->slots[i].active) {
            m_impl->slots[i].pcmCopy.clear();
            m_impl->slots.erase(m_impl->slots.begin() + i);
        }
    }
}

} // namespace Dnd
