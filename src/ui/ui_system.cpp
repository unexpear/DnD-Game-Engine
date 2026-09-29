/**
 * @file ui_system.cpp
 * @brief Implementation of UISystem.
 *
 * See ui_system.h for full documentation.
 *
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.7.0
 */

#include "ui/ui_system.h"
#include "ui/ui_widget.h"
#include "ui/ui_renderer.h"
#include "core/log.h"

namespace Dnd {

// ==========================================
// SINGLETON INSTANCE
// ==========================================

static UISystem* s_instance = nullptr;

UISystem& UISystem::Get() {
    if (!s_instance) {
        s_instance = new UISystem();
    }
    return *s_instance;
}

// ==========================================
// CONSTRUCTOR / DESTRUCTOR
// ==========================================

UISystem::UISystem()
    : m_screenWidth(1280), m_screenHeight(720), m_initialized(false),
      m_mouseCaptured(false) {
}

UISystem::~UISystem() {
    // Destroy all widgets
    for (auto* widget : m_widgets) {
        delete widget;
    }
    m_widgets.clear();
}

// ==========================================
// INITIALIZATION
// ==========================================

b8 UISystem::Initialize() {
    if (m_initialized) return true;

    DND_LOG_INFO("Initializing UI System...");

    // Create and initialize the UI renderer
    if (!UIRenderer::Initialize(m_screenWidth, m_screenHeight)) {
        DND_LOG_ERROR("UISystem: Failed to initialize UIRenderer");
        return false;
    }

    m_initialized = true;
    DND_LOG_INFO("UI System initialized successfully");
    return true;
}

void UISystem::ClearWidgets() {
    for (auto* widget : m_widgets) {
        delete widget;
    }
    m_widgets.clear();
    m_mouseCaptured = false;
}

void UISystem::Shutdown() {
    if (!m_initialized) return;

    DND_LOG_INFO("Shutting down UI System...");

    // Destroy all widgets
    for (auto* widget : m_widgets) {
        delete widget;
    }
    m_widgets.clear();

    // Shut down the UI renderer
    UIRenderer::Shutdown();

    m_initialized = false;
}

// ==========================================
// UPDATE / RENDER
// ==========================================

void UISystem::Update(f32 deltaTime) {
    if (!m_initialized) return;

    for (auto* widget : m_widgets) {
        if (widget) {
            widget->Update(deltaTime);
        }
    }
}

void UISystem::Render(const Mat4& projMatrix) {
    DND_UNUSED(projMatrix);
    if (!m_initialized) return;

    // Step 1: Build orthographic projection matrix from screen dimensions
    // Maps pixel coordinates with (0,0) at top-left to OpenGL NDC [-1, 1]
    // X: 0 -> -1 (left), screenWidth -> +1 (right)
    // Y: 0 -> +1 (top), screenHeight -> -1 (bottom)
    Mat4 orthoProj = Mat4::Orthographic(
        0.0f,
        static_cast<f32>(m_screenWidth),
        static_cast<f32>(m_screenHeight),
        0.0f,
        -1.0f,
        1.0f
    );

    // Step 2: Begin the UI rendering frame
    UIRenderer::BeginFrame(orthoProj);

    // Step 3: Render all visible widgets
    for (auto* widget : m_widgets) {
        if (widget && widget->IsVisible()) {
            widget->Render(orthoProj);
        }
    }

    // Step 4: End the UI rendering frame
    UIRenderer::EndFrame();
}

// ==========================================
// INPUT PROCESSING
// ==========================================

void UISystem::ProcessMouse(f32 mouseX, f32 mouseY, b8 mouseDown) {
    m_mouseCaptured = false;
    if (!m_initialized) return;

    // Process in reverse order (top-most widget first)
    for (auto it = m_widgets.rbegin(); it != m_widgets.rend(); ++it) {
        if (*it && (*it)->HandleInput(mouseX, mouseY, mouseDown)) {
            // A widget consumed the event — the UI owns the mouse this frame.
            m_mouseCaptured = true;
            break;
        }
    }
}

// ==========================================
// WIDGET MANAGEMENT
// ==========================================

UILabel* UISystem::CreateLabel(const std::string& name, const std::string& text) {
    UILabel* label = new UILabel(name, text);
    m_widgets.push_back(label);
    return label;
}

UIButton* UISystem::CreateButton(const std::string& name, const std::string& text) {
    UIButton* button = new UIButton(name, text);
    m_widgets.push_back(button);
    return button;
}

UIPanel* UISystem::CreatePanel(const std::string& name) {
    UIPanel* panel = new UIPanel(name);
    m_widgets.push_back(panel);
    return panel;
}

UISlider* UISystem::CreateSlider(const std::string& name, f32 minValue, f32 maxValue, f32 value) {
    UISlider* slider = new UISlider(name, minValue, maxValue, value);
    m_widgets.push_back(slider);
    return slider;
}

UIWidget* UISystem::GetWidget(const std::string& name) {
    return FindWidgetInternal(name);
}

void UISystem::DestroyWidget(const std::string& name) {
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it && (*it)->GetName() == name) {
            // Remove from any parent panel to avoid dangling child pointers
            for (auto* panel : m_widgets) {
                if (panel && panel != *it) {
                    auto* p = dynamic_cast<UIPanel*>(panel);
                    if (p) p->RemoveChild(*it);
                }
            }
            delete *it;
            m_widgets.erase(it);
            return;
        }
    }
}

UIWidget* UISystem::FindWidgetInternal(const std::string& name) {
    for (auto* widget : m_widgets) {
        if (widget && widget->GetName() == name) {
            return widget;
        }
    }
    return nullptr;
}

void UISystem::SetScreenSize(u32 width, u32 height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

} // namespace Dnd
