/**
 * @file audio_capture.cpp
 * @brief Implementation of AudioCapture using miniaudio capture API.
 * 
 * @author DND Engine Team
 * @date 2026-06-06
 * @version 0.1.0
 */

#include "audio/audio_capture.h"
#include "core/log.h"

#define MINIAUDIO_NO_IMPLEMENTATION
#include "miniaudio.h"

#include <cstring>

namespace Dnd {

// ==========================================
// PIMPL Implementation
// ==========================================

struct AudioCapture::Impl {
    ma_device device;
    CaptureCallback callback;
    b8 initialized;
};

// ==========================================
// Callback (static thunk)
// ==========================================

static void CaptureCallbackThunk(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pOutput;
    AudioCapture::Impl* impl = static_cast<AudioCapture::Impl*>(pDevice->pUserData);
    if (!impl || !impl->callback) return;

    u32 channels = pDevice->capture.channels;
    u32 totalSamples = frameCount * channels;
    impl->callback(static_cast<const f32*>(pInput), totalSamples, channels);
}

// ==========================================
// Lifecycle
// ==========================================

AudioCapture::AudioCapture()
    : m_impl(new Impl())
    , m_capturing(false)
{
    m_impl->initialized = false;
}

AudioCapture::~AudioCapture() {
    StopCapture();
    delete m_impl;
}

b8 AudioCapture::StartCapture(u32 sampleRate, u32 channels, CaptureCallback callback) {
    if (m_capturing) StopCapture();
    if (!callback) return false;

    m_impl->callback = std::move(callback);

    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format = ma_format_f32;
    config.capture.channels = channels > 0 ? channels : 1;
    config.sampleRate = sampleRate > 0 ? sampleRate : 44100;
    config.dataCallback = CaptureCallbackThunk;
    config.pUserData = m_impl;
    config.periodSizeInFrames = 512;

    if (ma_device_init(nullptr, &config, &m_impl->device) != MA_SUCCESS) {
        DND_LOG_ERROR("AudioCapture: failed to initialize capture device");
        return false;
    }

    if (ma_device_start(&m_impl->device) != MA_SUCCESS) {
        ma_device_uninit(&m_impl->device);
        DND_LOG_ERROR("AudioCapture: failed to start capture device");
        return false;
    }

    m_impl->initialized = true;
    m_capturing = true;
    DND_LOG_INFO("AudioCapture: started (%u Hz, %u ch)", config.sampleRate, config.capture.channels);
    return true;
}

void AudioCapture::StopCapture() {
    if (!m_capturing || !m_impl) return;

    if (m_impl->initialized) {
        ma_device_stop(&m_impl->device);
        ma_device_uninit(&m_impl->device);
        m_impl->initialized = false;
    }

    m_capturing = false;
    m_impl->callback = nullptr;
    DND_LOG_INFO("AudioCapture: stopped");
}

} // namespace Dnd
