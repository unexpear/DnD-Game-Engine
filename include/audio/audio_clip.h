#pragma once

#include "core/types.h"
#include <string>
#include <vector>

namespace Dnd {

enum class AudioFormat : i32 {
    MONO8,
    MONO16,
    STEREO8,
    STEREO16
};

class AudioClip {
public:
    AudioClip();
    ~AudioClip();

    b8 LoadFromFile(const char* filepath);

    b8 LoadFromMP4(const char* filepath);

    const u8* GetData() const { return m_pcmData.data(); }
    u32 GetDataSize() const { return static_cast<u32>(m_pcmData.size()); }
    u32 GetSampleRate() const { return m_sampleRate; }
    u32 GetNumChannels() const { return m_numChannels; }
    u32 GetBitsPerSample() const { return m_bitsPerSample; }
    f32 GetDuration() const { return m_duration; }
    const char* GetName() const { return m_name.c_str(); }
    const char* GetFilePath() const { return m_filePath.c_str(); }

    u64 GetTotalFrames() const;

private:
    b8 DecodeWithMiniaudio(const char* filepath);

    std::string m_name;
    std::string m_filePath;
    std::vector<u8> m_pcmData;
    u32 m_sampleRate;
    u16 m_numChannels;
    u16 m_bitsPerSample;
    f32 m_duration;
};

} // namespace Dnd
