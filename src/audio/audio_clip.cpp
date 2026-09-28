/**
 * @file audio_clip.cpp
 * @brief Implementation of AudioClip using miniaudio + Media Foundation.
 * 
 * AudioClip decodes audio files to PCM using miniaudio (WAV/MP3/FLAC/OGG)
 * and Media Foundation (MP4/AAC). Decoded PCM data is stored in f32 format.
 * 
 * @author DND Engine Team
 * @date 2026-06-06
 * @version 0.7.0
 */

#include "audio/audio_clip.h"
#include "core/log.h"

#define MINIAUDIO_NO_IMPLEMENTATION
#include "miniaudio.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

#define INITGUID
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "propsys")
#pragma comment(lib, "mf")

namespace Dnd {

static wchar_t* ToWide(const char* utf8) {
    if (!utf8) return nullptr;
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (len <= 0) return nullptr;
    wchar_t* wide = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, len);
    return wide;
}

AudioClip::AudioClip()
    : m_sampleRate(44100), m_numChannels(1), m_bitsPerSample(16), m_duration(0.0f) {
}

AudioClip::~AudioClip() {
    m_pcmData.clear();
}

b8 AudioClip::DecodeWithMiniaudio(const char* filepath) {
    ma_decoder decoder;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);

    if (ma_decoder_init_file(filepath, &config, &decoder) != MA_SUCCESS) {
        DND_LOG_ERROR("AudioClip: miniaudio failed to open '%s'", filepath);
        return false;
    }

    m_sampleRate = decoder.outputSampleRate;
    m_numChannels = decoder.outputChannels;
    m_bitsPerSample = 32;

    ma_uint64 totalFrames = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
    if (totalFrames == 0) totalFrames = 1024;

    std::vector<f32> floatBuffer;
    floatBuffer.resize(static_cast<size_t>(totalFrames) * m_numChannels);

    ma_uint64 framesRead = 0;
    while (framesRead < totalFrames) {
        ma_uint64 chunk = 0;
        ma_result res = ma_decoder_read_pcm_frames(&decoder,
            floatBuffer.data() + framesRead * m_numChannels,
            totalFrames - framesRead, &chunk);
        framesRead += chunk;
        if (res != MA_SUCCESS || chunk == 0) break;
    }

    ma_decoder_uninit(&decoder);

    if (framesRead == 0) {
        DND_LOG_ERROR("AudioClip: no PCM frames decoded from '%s'", filepath);
        return false;
    }

    m_pcmData.resize(framesRead * m_numChannels * sizeof(f32));
    memcpy(m_pcmData.data(), floatBuffer.data(), m_pcmData.size());
    m_duration = static_cast<f32>(framesRead) / static_cast<f32>(m_sampleRate);

    DND_LOG_INFO("AudioClip: loaded '%s' (%u Hz, %u ch, f32, %.2f s)",
                 m_name.c_str(), m_sampleRate, m_numChannels, m_duration);
    return true;
}

b8 AudioClip::LoadFromFile(const char* filepath) {
    if (!filepath) return false;

    const char* slash = strrchr(filepath, '/');
    if (!slash) slash = strrchr(filepath, '\\');
    m_name = slash ? (slash + 1) : filepath;
    m_filePath = filepath;

    b8 ok;
    const char* ext = strrchr(filepath, '.');
    if (ext && (strcmp(ext, ".mp4") == 0 || strcmp(ext, ".MP4") == 0 || strcmp(ext, ".m4a") == 0 || strcmp(ext, ".M4A") == 0)) {
        ok = LoadFromMP4(filepath);
    } else {
        ok = DecodeWithMiniaudio(filepath);
    }

    return ok;
}

b8 AudioClip::LoadFromMP4(const char* filepath) {
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        DND_LOG_ERROR("AudioClip: MFStartup failed (0x%08lx)", hr);
        return false;
    }

    wchar_t* widePath = ToWide(filepath);
    if (!widePath) {
        MFShutdown();
        return false;
    }

    IMFSourceReader* reader = nullptr;
    hr = MFCreateSourceReaderFromURL(widePath, nullptr, &reader);
    delete[] widePath;

    if (FAILED(hr) || !reader) {
        DND_LOG_ERROR("AudioClip: MFCreateSourceReaderFromURL failed (0x%08lx)", hr);
        MFShutdown();
        return false;
    }

    hr = reader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
    if (FAILED(hr)) {
        reader->Release();
        MFShutdown();
        DND_LOG_ERROR("AudioClip: no audio stream in '%s'", filepath);
        return false;
    }

    IMFMediaType* pcmType = nullptr;
    hr = MFCreateMediaType(&pcmType);
    if (SUCCEEDED(hr)) {
        pcmType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        pcmType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
        hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pcmType);
        pcmType->Release();
        if (FAILED(hr)) {
            pcmType = nullptr;
            hr = MFCreateMediaType(&pcmType);
            if (SUCCEEDED(hr)) {
                pcmType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
                pcmType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
                hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pcmType);
                pcmType->Release();
            }
        }
    }

    IMFMediaType* outputType = nullptr;
    hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &outputType);
    if (FAILED(hr) || !outputType) {
        reader->Release();
        MFShutdown();
        DND_LOG_ERROR("AudioClip: cannot determine output media type for '%s'", filepath);
        return false;
    }

    UINT32 sampleRate = 0, numChannels = 0, bitsPerSample = 16;
    {
        UINT32 val = 0;
        outputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &val); sampleRate = val;
        val = 0;
        outputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &val); numChannels = val;
        GUID subtype;
        outputType->GetGUID(MF_MT_SUBTYPE, &subtype);
        bitsPerSample = (subtype == MFAudioFormat_Float) ? 32 : 16;
    }
    outputType->Release();

    m_sampleRate = sampleRate;
    m_numChannels = numChannels;
    m_bitsPerSample = bitsPerSample;

    std::vector<u8> allData;
    while (true) {
        DWORD flags = 0;
        LONGLONG ts = 0;
        IMFSample* sample = nullptr;
        hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, &ts, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
            if (sample) sample->Release();
            break;
        }

        if (sample) {
            IMFMediaBuffer* buf = nullptr;
            hr = sample->ConvertToContiguousBuffer(&buf);
            if (SUCCEEDED(hr) && buf) {
                BYTE* data = nullptr;
                DWORD len = 0;
                hr = buf->Lock(&data, nullptr, &len);
                if (SUCCEEDED(hr) && data && len > 0) {
                    size_t off = allData.size();
                    allData.resize(off + len);
                    memcpy(allData.data() + off, data, len);
                    buf->Unlock();
                }
                buf->Release();
            }
            sample->Release();
        }
    }

    reader->Release();
    MFShutdown();

    if (allData.empty()) {
        DND_LOG_ERROR("AudioClip: no audio data in MP4 '%s'", filepath);
        return false;
    }

    if (m_bitsPerSample == 16) {
        u32 totalSamples = static_cast<u32>(allData.size()) / 2;
        std::vector<f32> floatBuf(totalSamples);
        const short* src = reinterpret_cast<const short*>(allData.data());
        for (u32 i = 0; i < totalSamples; ++i)
            floatBuf[i] = static_cast<f32>(src[i]) / 32768.0f;
        m_pcmData.resize(totalSamples * sizeof(f32));
        memcpy(m_pcmData.data(), floatBuf.data(), m_pcmData.size());
        m_bitsPerSample = 32;
    } else {
        m_pcmData = std::move(allData);
    }

    u32 bytesPerFrame = (m_bitsPerSample / 8) * m_numChannels;
    u32 totalFrames = bytesPerFrame > 0 ? static_cast<u32>(m_pcmData.size()) / bytesPerFrame : 0;
    m_duration = sampleRate > 0 ? static_cast<f32>(totalFrames) / static_cast<f32>(sampleRate) : 0.0f;

    DND_LOG_INFO("AudioClip: loaded MP4 '%s' (%u Hz, %u ch, f32, %.2f s)",
                 m_name.c_str(), m_sampleRate, m_numChannels, m_duration);
    return true;
}

u64 AudioClip::GetTotalFrames() const {
    u32 bpf = (m_bitsPerSample / 8) * m_numChannels;
    return bpf > 0 ? static_cast<u64>(m_pcmData.size()) / bpf : 0;
}



} // namespace Dnd
