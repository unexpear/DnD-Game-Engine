#pragma once

#include "core/types.h"
#include "core/math.h"
#include <vector>
#include <string>
#include <chrono>

namespace Dnd {

class Profiler {
public:
    static Profiler& Get();

    void Toggle();
    void SetVisible(b8 visible) { m_visible = visible; }
    b8 IsVisible() const { return m_visible; }

    void BeginFrame();
    void EndFrame();
    void BeginSystem(const char* name);
    void EndSystem();

    void SetEntityCount(u32 count) { m_entityCount = count; }
    void SetDrawCalls(u32 count) { m_drawCallCount = count; }

    void Render(u32 screenW, u32 screenH);

private:
    Profiler();

    struct TimingSlice {
        std::string name;
        double elapsedMs;
    };

    static constexpr u32 HISTORY_FRAMES = 60;

    b8 m_visible;
    u32 m_frameCount;

    std::chrono::high_resolution_clock::time_point m_frameStart;
    std::chrono::high_resolution_clock::time_point m_systemStart;
    u32 m_entityCount;
    u32 m_drawCallCount;

    double m_frameTimeMs;
    double m_systemTimeMs;
    double m_gameTimeMs;

    double m_historyFps[HISTORY_FRAMES];
    double m_historyFrameMs[HISTORY_FRAMES];
    u32 m_historyIndex;

    u32 m_currentSystemDepth;
    TimingSlice m_currentSlice;
    std::vector<TimingSlice> m_slices;
};

} // namespace Dnd
