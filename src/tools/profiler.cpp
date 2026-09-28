#include "tools/profiler.h"
#include "ui/ui_renderer.h"
#include <algorithm>
#include <cstdio>

namespace Dnd {

Profiler& Profiler::Get() {
    static Profiler instance;
    return instance;
}

Profiler::Profiler()
    : m_visible(false), m_frameCount(0), m_entityCount(0), m_drawCallCount(0),
      m_frameTimeMs(0.0), m_systemTimeMs(0.0), m_gameTimeMs(0.0),
      m_historyIndex(0), m_currentSystemDepth(0) {
    for (u32 i = 0; i < HISTORY_FRAMES; ++i) {
        m_historyFps[i] = 60.0;
        m_historyFrameMs[i] = 16.667;
    }
}

void Profiler::Toggle() {
    m_visible = !m_visible;
}

void Profiler::BeginFrame() {
    if (!m_visible) return;
    m_frameStart = std::chrono::high_resolution_clock::now();
    m_systemTimeMs = 0.0;
    m_gameTimeMs = 0.0;
    m_currentSystemDepth = 0;
    m_slices.clear();
}

void Profiler::EndFrame() {
    if (!m_visible) return;

    auto now = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(now - m_frameStart).count();
    m_frameTimeMs = elapsed;

    double fps = elapsed > 0.0 ? 1000.0 / elapsed : 0.0;

    m_historyFps[m_historyIndex] = fps;
    m_historyFrameMs[m_historyIndex] = elapsed;
    m_historyIndex = (m_historyIndex + 1) % HISTORY_FRAMES;

    m_frameCount++;
}

void Profiler::BeginSystem(const char* name) {
    if (!m_visible) return;
    if (m_currentSystemDepth == 0) {
        m_currentSlice.name = name;
        m_currentSlice.elapsedMs = 0.0;
        m_systemStart = std::chrono::high_resolution_clock::now();
    }
    m_currentSystemDepth++;
}

void Profiler::EndSystem() {
    if (!m_visible) return;
    if (m_currentSystemDepth == 0) return;
    m_currentSystemDepth--;

    if (m_currentSystemDepth == 0) {
        auto now = std::chrono::high_resolution_clock::now();
        m_currentSlice.elapsedMs = std::chrono::duration<double, std::milli>(now - m_systemStart).count();
        m_systemTimeMs += m_currentSlice.elapsedMs;

        b8 found = false;
        for (auto& s : m_slices) {
            if (s.name == m_currentSlice.name) {
                s.elapsedMs += m_currentSlice.elapsedMs;
                found = true;
                break;
            }
        }
        if (!found) {
            m_slices.push_back(m_currentSlice);
        }
    }
}

void Profiler::Render(u32 screenW, u32 screenH) {
    if (!m_visible) return;

    const f32 panelX = 10.0f;
    const f32 panelY = 10.0f;
    const f32 panelW = 300.0f;
    f32 lineH = 18.0f;
    f32 fontSize = 13.0f;
    Vec4 bgColor(0.0f, 0.0f, 0.0f, 0.7f);
    Vec4 textColor(0.0f, 1.0f, 0.3f, 1.0f);
    Vec4 dimColor(0.7f, 0.7f, 0.7f, 1.0f);
    Vec4 headerColor(0.3f, 0.8f, 1.0f, 1.0f);

    double avgFps = 0.0;
    double avgMs = 0.0;
    double minMs = 1e10;
    double maxMs = 0.0;
    for (u32 i = 0; i < HISTORY_FRAMES; ++i) {
        avgFps += m_historyFps[i];
        avgMs += m_historyFrameMs[i];
        if (m_historyFrameMs[i] < minMs) minMs = m_historyFrameMs[i];
        if (m_historyFrameMs[i] > maxMs) maxMs = m_historyFrameMs[i];
    }
    avgFps /= HISTORY_FRAMES;
    avgMs /= HISTORY_FRAMES;

    f32 y = panelY + 8.0f;

    char buf[128];

    snprintf(buf, sizeof(buf), "PROFILER  (F2: hide)");
    UIRenderer::DrawText(panelX + 8.0f, y, buf, headerColor, 1.0f);
    y += lineH;

    snprintf(buf, sizeof(buf), "FPS: %.1f  Frame: %.2f ms", avgFps, avgMs);
    UIRenderer::DrawText(panelX + 8.0f, y, buf, textColor, fontSize);
    y += lineH;

    snprintf(buf, sizeof(buf), "Min: %.2f ms  Max: %.2f ms", minMs, maxMs);
    UIRenderer::DrawText(panelX + 8.0f, y, buf, dimColor, fontSize);
    y += lineH;

    snprintf(buf, sizeof(buf), "Entities: %u  Draw Calls: %u", m_entityCount, m_drawCallCount);
    UIRenderer::DrawText(panelX + 8.0f, y, buf, dimColor, fontSize);
    y += lineH;

    snprintf(buf, sizeof(buf), "System: %.2f ms  Game: %.2f ms", m_systemTimeMs, m_gameTimeMs);
    UIRenderer::DrawText(panelX + 8.0f, y, buf, dimColor, fontSize);
    y += lineH + 4.0f;

    if (!m_slices.empty()) {
        snprintf(buf, sizeof(buf), "-- Systems --");
        UIRenderer::DrawText(panelX + 8.0f, y, buf, headerColor, fontSize);
        y += lineH;

        for (const auto& slice : m_slices) {
            if (slice.elapsedMs > 0.05) {
                Vec4 col = (slice.elapsedMs > 10.0) ? Vec4(1.0f, 0.3f, 0.3f, 1.0f)
                        : (slice.elapsedMs > 3.0)  ? Vec4(1.0f, 0.8f, 0.2f, 1.0f)
                                                   : dimColor;
                snprintf(buf, sizeof(buf), "  %s: %.2f ms", slice.name.c_str(), slice.elapsedMs);
                UIRenderer::DrawText(panelX + 8.0f, y, buf, col, fontSize);
                y += lineH;
            }
        }
        y += 4.0f;
    }

    // Mini frame-time graph (last 60 frames)
    f32 graphX = panelX + 8.0f;
    f32 graphY = y + 4.0f;
    f32 graphW = panelW - 16.0f;
    f32 graphH = 60.0f;
    f32 barW = graphW / HISTORY_FRAMES;
    f32 scale = 1.0f;
    double maxGraphMs = 33.333; // 30 FPS = 33.3ms
    for (u32 i = 0; i < HISTORY_FRAMES; ++i) {
        if (m_historyFrameMs[i] > maxGraphMs) maxGraphMs = m_historyFrameMs[i];
    }
    scale = static_cast<f32>(graphH / maxGraphMs);

    // Background
    UIRenderer::DrawRect(graphX, graphY, graphW, graphH, Vec4(0.0f, 0.0f, 0.0f, 0.5f));

    // 16.67ms line (60 FPS)
    f32 refY = graphY + graphH - 16.667f * scale;
    UIRenderer::DrawRect(graphX, refY, graphW, 1.0f, Vec4(0.0f, 1.0f, 0.0f, 0.3f));

    for (u32 i = 0; i < HISTORY_FRAMES; ++i) {
        f32 barH = static_cast<f32>(m_historyFrameMs[i]) * scale;
        if (barH > graphH) barH = graphH;
        f32 bx = graphX + static_cast<f32>(i) * barW;
        f32 by = graphY + graphH - barH;

        Vec4 barColor;
        if (m_historyFrameMs[i] > 33.333f) barColor = Vec4(1.0f, 0.2f, 0.2f, 0.8f);
        else if (m_historyFrameMs[i] > 16.667f) barColor = Vec4(1.0f, 0.8f, 0.1f, 0.8f);
        else barColor = Vec4(0.2f, 1.0f, 0.3f, 0.8f);

        UIRenderer::DrawRect(bx, by, barW + 1.0f, barH, barColor);
    }
}

} // namespace Dnd
