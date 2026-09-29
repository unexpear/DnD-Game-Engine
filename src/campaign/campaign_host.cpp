/**
 * @file campaign_host.cpp
 * @brief Campaign scan, scene load, and the F8 pack picker.
 *
 * Switching campaigns resets UI widgets, the undo stack, editor
 * selection, spotlight occluders, and the orbit camera before the new
 * scene loads. Rules stay on the session. entryScript is recorded and
 * is not executed.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/campaign_host.h"
#include "campaign/campaign_session.h"
#include "campaign/campaign_table_scene.h"
#include "core/engine.h"
#include "core/input.h"
#include "core/log.h"
#include "rendering/renderer.h"
#include "scene/dnd_demo_scene.h"
#include "scene/scene_manager.h"
#include "tools/editor_settings.h"
#include "tools/undo.h"
#include "ui/ui_renderer.h"
#include "ui/ui_system.h"

#include <cstdio>
#include <memory>
#include <string>

namespace Dnd {

namespace {

SceneManager* s_scenes = nullptr;
CampaignRegistry s_registry;
CampaignSession s_session;
b8 s_pickerOpen = false;
i32 s_pickerIndex = 0;
f32 s_screenW = 1280.0f;
f32 s_screenH = 720.0f;

constexpr f32 kPickerW = 520.0f;
constexpr f32 kPickerRowH = 22.0f;
constexpr f32 kPickerRowTop = 78.0f;

f32 PickerX() {
    f32 x = s_screenW - kPickerW - 24.0f;
    if (x < 8.0f) {
        x = 8.0f;
    }
    return x;
}

std::unique_ptr<Scene> CreateEntryScene(const CampaignManifest& manifest) {
    if (manifest.entryScene == "dnd_demo") {
        return std::make_unique<DndDemoScene>();
    }
    if (manifest.entryScene == "campaign_table") {
        return std::make_unique<CampaignTableScene>(manifest);
    }
    return nullptr;
}

void ResetCrossSceneState() {
    UISystem::Get().ClearWidgets();
    UndoStack::Get().Clear();
    EditorSettings::Get().selectedEntity = Entity::None();
    Renderer::SetOccluders(nullptr, nullptr, 0);
    Engine::ResetViewToBase();
}

b8 LoadActiveScene() {
    if (s_scenes == nullptr || !s_session.IsActive()) {
        return false;
    }
    std::unique_ptr<Scene> scene = CreateEntryScene(s_session.Manifest());
    if (!scene) {
        DND_LOG_ERROR("Campaign %s has no loadable entry scene", s_session.ActiveId().c_str());
        return false;
    }
    ResetCrossSceneState();
    s_scenes->SetActiveScene(std::move(scene));
    DND_LOG_INFO("Campaign active: %s (%s, ruleset %s, combat %s)",
                 s_session.ActiveId().c_str(),
                 s_session.Manifest().title.c_str(),
                 s_session.Rules().moduleId.c_str(),
                 s_session.Rules().usesEngineCombat ? "engine" : "session-only");
    if (!s_session.Manifest().entryScript.empty()) {
        DND_LOG_INFO("Campaign entryScript recorded and not executed: %s",
                     s_session.Manifest().entryScript.c_str());
    }
    return true;
}

b8 ActivateAndLoad(const std::string& id) {
    std::string error;
    if (!s_session.Activate(s_registry, id, error)) {
        DND_LOG_WARN("Campaign switch refused: %s", error.c_str());
        return false;
    }
    if (!LoadActiveScene()) {
        s_session.ResetToBase();
        return false;
    }
    for (i32 i = 0; i < static_cast<i32>(s_registry.All().size()); ++i) {
        if (s_registry.All()[static_cast<size_t>(i)].id == id) {
            s_pickerIndex = i;
            break;
        }
    }
    return true;
}

void MovePicker(i32 delta) {
    const i32 count = static_cast<i32>(s_registry.All().size());
    if (count <= 0) {
        s_pickerIndex = 0;
        return;
    }
    s_pickerIndex = (s_pickerIndex + delta) % count;
    if (s_pickerIndex < 0) {
        s_pickerIndex += count;
    }
}

void ActivatePickerSelection() {
    const std::vector<CampaignManifest>& packs = s_registry.All();
    if (packs.empty()) {
        return;
    }
    if (s_pickerIndex < 0 || s_pickerIndex >= static_cast<i32>(packs.size())) {
        s_pickerIndex = 0;
    }
    const std::string id = packs[static_cast<size_t>(s_pickerIndex)].id;
    if (ActivateAndLoad(id)) {
        s_pickerOpen = false;
    }
}

} // namespace

b8 CampaignHost::Initialize(SceneManager* scenes, const char* requestedId) {
    if (scenes == nullptr) {
        return false;
    }
    s_scenes = scenes;
    s_registry.Clear();
    s_session.ResetToBase();
    s_pickerOpen = false;
    s_pickerIndex = 0;

    for (const std::string& root : CampaignRegistry::DefaultSearchRoots()) {
        const CampaignScanReport report = s_registry.ScanDirectory(root);
        for (const std::string& message : report.messages) {
            DND_LOG_WARN("Campaign scan: %s", message.c_str());
        }
        if (report.loaded > 0 || report.skippedDuplicate > 0) {
            DND_LOG_INFO("Campaign scan %s: %d loaded, %d rejected, %d already registered",
                         root.c_str(), report.loaded, report.rejected, report.skippedDuplicate);
        }
    }
    s_registry.EnsureBuiltinDnd();

    DND_LOG_INFO("Campaign packs available:");
    for (const CampaignManifest& pack : s_registry.All()) {
        DND_LOG_INFO("  %s - %s [%s]",
                     pack.id.c_str(), pack.title.c_str(), pack.entryScene.c_str());
    }

    const std::string wanted = (requestedId != nullptr && requestedId[0] != '\0')
        ? std::string(requestedId)
        : std::string("dnd");
    if (!ActivateAndLoad(wanted)) {
        DND_LOG_WARN("Requested campaign '%s' did not load; falling back to dnd", wanted.c_str());
        if (!ActivateAndLoad("dnd")) {
            s_session.ResetToBase();
            ResetCrossSceneState();
            scenes->SetActiveScene(std::make_unique<DndDemoScene>());
            DND_LOG_ERROR("Campaign fallback created the D&D demo scene directly");
        }
    }
    return true;
}

void CampaignHost::Shutdown() {
    s_scenes = nullptr;
    s_pickerOpen = false;
    s_session.ResetToBase();
    s_registry.Clear();
}

b8 CampaignHost::IsPickerOpen() {
    return s_pickerOpen;
}

b8 CampaignHost::HandleInput() {
    if (Input::IsTextInputActive()) {
        return false;
    }

    if (Input::IsKeyPressed(KeyCode::F8)) {
        s_pickerOpen = !s_pickerOpen;
        if (s_pickerOpen) {
            for (i32 i = 0; i < static_cast<i32>(s_registry.All().size()); ++i) {
                if (s_registry.All()[static_cast<size_t>(i)].id == s_session.ActiveId()) {
                    s_pickerIndex = i;
                    break;
                }
            }
        }
        DND_LOG_INFO("Campaign picker %s", s_pickerOpen ? "open" : "closed");
        return true;
    }

    if (!s_pickerOpen) {
        return false;
    }

    if (Input::IsKeyPressed(KeyCode::ESCAPE)) {
        s_pickerOpen = false;
        return true;
    }
    if (Input::IsKeyPressed(KeyCode::UP)) {
        MovePicker(-1);
    } else if (Input::IsKeyPressed(KeyCode::DOWN)) {
        MovePicker(1);
    } else if (Input::IsKeyPressed(KeyCode::ENTER)) {
        ActivatePickerSelection();
    } else if (Input::IsMouseButtonPressed(MouseButton::LEFT)) {
        const f32 x = Input::GetMouseX();
        const f32 y = Input::GetMouseY();
        const f32 panelX = PickerX();
        const i32 count = static_cast<i32>(s_registry.All().size());
        if (x >= panelX && x <= panelX + kPickerW) {
            const i32 row = static_cast<i32>((y - kPickerRowTop) / kPickerRowH);
            if (row >= 0 && row < count) {
                s_pickerIndex = row;
                ActivatePickerSelection();
            }
        }
    } else {
        const KeyCode digits[9] = {
            KeyCode::D1, KeyCode::D2, KeyCode::D3, KeyCode::D4, KeyCode::D5,
            KeyCode::D6, KeyCode::D7, KeyCode::D8, KeyCode::D9
        };
        for (i32 i = 0; i < 9; ++i) {
            if (Input::IsKeyPressed(digits[i]) &&
                i < static_cast<i32>(s_registry.All().size())) {
                s_pickerIndex = i;
                ActivatePickerSelection();
                break;
            }
        }
    }
    return true;
}

void CampaignHost::RenderChrome(const Mat4& ortho, f32 screenW, f32 screenH) {
    s_screenW = screenW;
    s_screenH = screenH;
    UIRenderer::BeginFrame(ortho);

    const char* title = s_session.IsActive() ? s_session.Manifest().title.c_str() : "Dungeons & Dragons";
    char chip[160];
    std::snprintf(chip, sizeof(chip), "%s    F8 campaigns", title);
    const f32 chipW = 360.0f;
    const f32 chipX = s_screenW * 0.5f - chipW * 0.5f;
    const f32 chipY = s_screenH - 28.0f;
    UIRenderer::DrawRect(chipX, chipY, chipW, 20.0f, Vec4(0.05f, 0.05f, 0.06f, 0.72f));
    UIRenderer::DrawText(chipX + 8.0f, chipY + 4.0f, chip, Vec4(0.86f, 0.84f, 0.75f, 1.0f), 8.0f);

    if (s_pickerOpen) {
        const std::vector<CampaignManifest>& packs = s_registry.All();
        const f32 panelX = PickerX();
        const f32 panelY = 40.0f;
        const f32 panelW = kPickerW;
        const f32 rowH = kPickerRowH;
        const f32 panelH = 72.0f + rowH * static_cast<f32>(packs.size());
        UIRenderer::DrawRect(panelX, panelY, panelW, panelH, Vec4(0.06f, 0.06f, 0.08f, 0.94f));
        UIRenderer::DrawText(panelX + 12.0f, panelY + 10.0f, "Campaign packs",
                             Vec4(0.95f, 0.88f, 0.62f, 1.0f), 12.0f);
        UIRenderer::DrawText(panelX + 12.0f, panelY + 30.0f,
                             "Up/Down select   Enter load   Esc close   1-9 jump",
                             Vec4(0.70f, 0.72f, 0.68f, 1.0f), 8.0f);

        for (i32 i = 0; i < static_cast<i32>(packs.size()); ++i) {
            const f32 y = kPickerRowTop + rowH * static_cast<f32>(i);
            const b8 selected = (i == s_pickerIndex);
            const b8 active = packs[static_cast<size_t>(i)].id == s_session.ActiveId();
            if (selected) {
                UIRenderer::DrawRect(panelX + 8.0f, y, panelW - 16.0f, rowH - 2.0f,
                                     Vec4(0.28f, 0.22f, 0.12f, 1.0f));
            }
            char line[192];
            std::snprintf(line, sizeof(line), "%s %s  %s",
                          active ? ">" : " ",
                          packs[static_cast<size_t>(i)].id.c_str(),
                          packs[static_cast<size_t>(i)].title.c_str());
            UIRenderer::DrawText(panelX + 14.0f, y + 4.0f, line,
                                 selected ? Vec4(1.0f, 0.95f, 0.80f, 1.0f)
                                          : Vec4(0.82f, 0.84f, 0.80f, 1.0f),
                                 8.0f);
        }
    }

    UIRenderer::EndFrame();
}

} // namespace Dnd
