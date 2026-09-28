/**
 * @file audio_capture.h
 * @brief Microphone capture using miniaudio.
 * 
 * AudioCapture provides microphone input for in-game voice chat,
 * voice commands, or audio recording. Uses miniaudio's capture
 * device API to stream raw PCM from the default input device.
 * 
 * @author DND Engine Team
 * @date 2026-06-06
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include <functional>

namespace Dnd {

/**
 * @class AudioCapture
 * @brief Microphone capture via miniaudio.
 * 
 * Callback-based capture from the default input device.
 * Captured PCM data (f32 mono/stereo) is delivered via
 * the registered callback.
 * 
 * Usage:
 * @code
 * AudioCapture capture;
 * capture.StartCapture([](const f32* samples, u32 numSamples, u32 channels) {
 *     // Process microphone input
 * });
 * // ... later ...
 * capture.StopCapture();
 * @endcode
 */
class AudioCapture {
public:
    /**
     * @brief Callback receiving captured PCM frames.
     * @param samples Pointer to f32 sample data (interleaved).
     * @param numSamples Total number of samples.
     * @param channels Number of channels.
     */
    using CaptureCallback = std::function<void(const f32* samples, u32 numSamples, u32 channels)>;

    AudioCapture();
    ~AudioCapture();

    /**
     * @brief Starts capturing from the default microphone.
     * @param sampleRate Desired sample rate (0 = device default).
     * @param channels Desired channels (0 = device default).
     * @param callback Function called with captured data.
     * @return true if capture started successfully.
     */
    b8 StartCapture(u32 sampleRate, u32 channels, CaptureCallback callback);

    /**
     * @brief Stops capture.
     */
    void StopCapture();

    /**
     * @brief Whether capture is currently active.
     */
    b8 IsCapturing() const { return m_capturing; }

public:
    struct Impl;
private:
    Impl* m_impl;
    b8 m_capturing;
};

} // namespace Dnd
