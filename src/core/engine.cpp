/**
 * @file engine.cpp
 * @brief Implementation of the Engine class and EngineConfig.
 * 
 * See engine.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.2.0
 */

#include "core/engine.h"
#include "core/log.h"
#include "core/time.h"
#include "core/window.h"
#include "core/input.h"
#include "rendering/renderer.h"
#include "rendering/shader.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"
#include "rendering/debug_draw.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/system.h"
#include "ecs/components/transform.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/camera.h"
#include "ecs/components/tag.h"
#include "ecs/systems/transform_system.h"
#include "scene/demo_scene.h"
#include "scene/physics_demo_scene.h"
#include "scene/animation_demo_scene.h"
#include "scene/dnd_demo_scene.h"
#include "scene/shadow_test_scene.h"
#include "scene/scene_serializer.h"
#include "ui/ui_system.h"
#include "ui/ui_widget.h"
#include "ui/ui_renderer.h"
#include "ui/drag_drop.h"
#include "tools/editor_settings.h"
#include "tools/dock_manager.h"
#include "tools/panel_hierarchy.h"
#include "tools/inspector_panel.h"
#include "tools/asset_browser_panel.h"
#include "tools/gizmo.h"
#include "tools/undo.h"
#include "tools/profiler.h"
#include "physics/collider.h"
#include "physics/collision.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include <cstring>

namespace Dnd {

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

/** @brief Current engine state. */
EngineState Engine::s_state = EngineState::INITIALIZING;

/** @brief Whether the engine has been initialized. */
b8 Engine::s_initialized = false;

/** @brief Engine configuration copy. */
EngineConfig Engine::s_config;

/** @brief Main scene camera. */
Camera* Engine::s_camera = nullptr;

/** @brief Camera orbit angle (yaw) in radians. */
f32 Engine::s_cameraYaw = Math::DegreesToRadians(45.0f);

/** @brief Camera orbit angle (pitch) in radians. */
f32 Engine::s_cameraPitch = Math::DegreesToRadians(42.0f); // tilted: see the maze AND the characters

/** @brief Camera orbit distance from target. */
f32 Engine::s_cameraDistance = 16.0f;

/** @brief Point the camera follows (default = old fixed focus until a scene sets it). */
Vec3 Engine::s_cameraFocus = Vec3(0.0f, 0.5f, 1.5f);
Vec3 Engine::s_cameraFocusSmoothed = Vec3(0.0f, 0.5f, 1.5f);

/** @brief Whether right mouse was held last frame. */
static b8 s_wasRightMouseDown = false;

/** @brief Scene manager for ECS-based scene handling. */
static SceneManager* s_sceneManager = nullptr;

/** @brief Editor hierarchy panel (todo: make a panel manager). */
static HierarchyPanel* s_hierarchyPanel = nullptr;

/** @brief Editor inspector panel for component editing. */
static InspectorPanel* s_inspectorPanel = nullptr;

/** @brief Editor asset browser panel. */
static AssetBrowserPanel* s_assetBrowserPanel = nullptr;

// ==========================================
// ENGINECONFIG DEFAULTS
// ==========================================

EngineConfig::EngineConfig()
    : applicationName("D&D Game Engine")
    , logFilePath("engine.log")
    , windowWidth(1280)
    , windowHeight(720)
    , vsyncEnabled(true)
    , fullscreenEnabled(false)
{
}

// ==========================================
// INITIALIZATION
// ==========================================

b8 Engine::Initialize(const EngineConfig& config) {
    s_state = EngineState::INITIALIZING;
    s_config = config;

    // Track what was initialized so we can clean up on failure
    b8 logOk = false, windowOk = false, rendererOk = false, debugDrawOk = false, uiOk = false;

    // Step 1: Initialize Logger
    DND_LOG_INFO("Initializing Logger...");
    if (!Logger::Initialize(s_config.logFilePath)) {
        fprintf(stderr, "[FATAL] Logger initialization failed.\n");
        return false;
    }
    logOk = true;

    // Step 2: Initialize Time
    DND_LOG_INFO("Initializing Time system...");
    Time::Initialize();

    // Step 3: Initialize Window
    DND_LOG_INFO("Initializing Window...");
    WindowConfig windowConfig;
    windowConfig.title = s_config.applicationName;
    windowConfig.width = s_config.windowWidth;
    windowConfig.height = s_config.windowHeight;
    windowConfig.vsync = s_config.vsyncEnabled;
    windowConfig.fullscreen = s_config.fullscreenEnabled;
    windowConfig.resizable = true;

    if (!Window::Create(windowConfig)) {
        DND_LOG_FATAL("Window creation failed");
        goto cleanup;
    }
    windowOk = true;

    // Step 4: Initialize Input
    DND_LOG_INFO("Initializing Input...");
    Input::Initialize();

    // Step 5: Initialize Renderer
    DND_LOG_INFO("Initializing Renderer...");
    if (!Renderer::Initialize()) {
        DND_LOG_FATAL("Renderer initialization failed");
        goto cleanup;
    }
    rendererOk = true;

    // Step 6: Initialize Debug Draw
    DND_LOG_INFO("Initializing DebugDraw...");
    if (!DebugDraw::Initialize()) {
        DND_LOG_FATAL("DebugDraw initialization failed");
        goto cleanup;
    }
    debugDrawOk = true;

    // Step 7: Initialize the UI system (on-screen overlay: panels, sliders).
    // Non-fatal — if it fails we just run without the overlay.
    DND_LOG_INFO("Initializing UI System...");
    UISystem::Get().SetScreenSize(s_config.windowWidth, s_config.windowHeight);
    if (!UISystem::Get().Initialize()) {
        DND_LOG_ERROR("UI System initialization failed (continuing without UI overlay)");
    } else {
        uiOk = true;
    }

    // ==========================================
    // ECS SCENE SETUP
    // ==========================================

    // Create the scene manager
    s_sceneManager = new SceneManager();

    // Create and load the D&D demo scene
    DndDemoScene* demoScene = new DndDemoScene();
    s_sceneManager->SetActiveScene(std::unique_ptr<DndDemoScene>(demoScene));

    // ==========================================
    // EDITOR SETUP
    // ==========================================

    // Create default editor panels
    {
        auto& dm = DockManager::Get();

        // Hierarchy panel (shows entity tree)
        s_hierarchyPanel = new HierarchyPanel();
        dm.RegisterPanel(s_hierarchyPanel->GetDockPanel());

        // Inspector panel (entity property editor)
        s_inspectorPanel = new InspectorPanel();
        dm.RegisterPanel(s_inspectorPanel->GetDockPanel());

        // Asset Browser panel (file browser)
        s_assetBrowserPanel = new AssetBrowserPanel();
        dm.RegisterPanel(s_assetBrowserPanel->GetDockPanel());

        // Build zones and dock panels into default positions
        dm.RecalculateZones(s_config.windowWidth, s_config.windowHeight);
        dm.TrySnapPanel(s_hierarchyPanel->GetDockPanel(), 2.0f, s_config.windowHeight * 0.5f);
        dm.TrySnapPanel(s_inspectorPanel->GetDockPanel(), static_cast<f32>(s_config.windowWidth) - 10.0f, s_config.windowHeight * 0.5f);
        dm.TrySnapPanel(s_assetBrowserPanel->GetDockPanel(), static_cast<f32>(s_config.windowWidth) * 0.5f, static_cast<f32>(s_config.windowHeight) - 10.0f);

        // Start with editor inactive
        dm.SetActive(false);
    }

    // Register viewport drop zone for mesh file drops
    {
        f32 vpX = static_cast<f32>(s_config.windowWidth);
        f32 vpY = static_cast<f32>(s_config.windowHeight);
        DropZone viewportZone;
        viewportZone.area = DropZoneRect(DockManager::SIDE_WIDTH, DockManager::TOP_HEIGHT,
                                         vpX - 2.0f * DockManager::SIDE_WIDTH,
                                         vpY - DockManager::TOP_HEIGHT - DockManager::BOTTOM_HEIGHT);
        viewportZone.widget = nullptr;
        viewportZone.canAccept = [](const DragPayload& p) -> bool {
            if (p.type != DragPayloadType::STRING || !p.stringVal) return false;
            const char* ext = strrchr(p.stringVal, '.');
            if (!ext) return false;
            const char* e = ext + 1;
            bool isMesh = (ext[0] == '.' &&
                           (ext[1] == 'o' || ext[1] == 'O') &&
                           (ext[2] == 'b' || ext[2] == 'B') &&
                           (ext[3] == 'j' || ext[3] == 'J') &&
                           ext[4] == '\0');
            if (isMesh) return true;
            bool isImage = (std::strcmp(e, "png") == 0 || std::strcmp(e, "PNG") == 0 ||
                            std::strcmp(e, "jpg") == 0 || std::strcmp(e, "JPG") == 0 ||
                            std::strcmp(e, "jpeg") == 0 || std::strcmp(e, "JPEG") == 0);
            return isImage;
        };
        viewportZone.onDrop = [](const DragPayload& p) {
            if (!s_sceneManager) return;
            Scene* scene = s_sceneManager->GetActiveScene();
            if (!scene) return;
            EntityManager& em = scene->GetEntityManager();
            ComponentRegistry& cr = scene->GetComponentRegistry();

            f32 yaw = Engine::s_cameraYaw;
            f32 pitch = Engine::s_cameraPitch;
            f32 dist = Engine::s_cameraDistance;
            f32 cx = std::cos(yaw) * std::cos(pitch) * dist;
            f32 cy = std::sin(pitch) * dist;
            f32 cz = std::sin(yaw) * std::cos(pitch) * dist;
            Vec3 eye(Engine::s_cameraFocusSmoothed.x + cx,
                     Engine::s_cameraFocusSmoothed.y + cy,
                     Engine::s_cameraFocusSmoothed.z + cz);
            Vec3 target = Engine::s_cameraFocusSmoothed;
            Mat4 viewMat = Mat4::TRS(eye, Quat::LookAt(eye, target, Vec3(0,1,0)), Vec3(1,1,1)).Inverse();

            Mat4 projMat;
            auto* camSet = cr.GetSparseSet<ECSCamera>();
            if (camSet && camSet->Size() > 0) {
                projMat = camSet->GetDense()[0].GetProjectionMatrix();
            } else {
                projMat = Mat4::Perspective(60.0f * 0.0174533f,
                    static_cast<f32>(Engine::s_config.windowWidth) / static_cast<f32>(Engine::s_config.windowHeight), 0.1f, 1000.0f);
            }

            f32 mx = DragDropManager::Get().GetMouseX();
            f32 my = DragDropManager::Get().GetMouseY();
            f32 vpW = static_cast<f32>(Engine::s_config.windowWidth);
            f32 vpH = static_cast<f32>(Engine::s_config.windowHeight);
            f32 ndcX = (2.0f * mx / vpW) - 1.0f;
            f32 ndcY = 1.0f - (2.0f * my / vpH);

            Mat4 invVP = (projMat * viewMat).Inverse();
            Vec4 nearH = invVP * Vec4(ndcX, ndcY, -1.0f, 1.0f);
            Vec4 farH  = invVP * Vec4(ndcX, ndcY,  1.0f, 1.0f);
            Vec3 nearW = nearH.ToVec3() / nearH.w;
            Vec3 farW  = farH.ToVec3() / farH.w;
            Vec3 dir = (farW - nearW).Normalized();

            f32 t = -nearW.y / dir.y;
            if (t < 0.0f) t = 0.0f;
            Vec3 dropPos = nearW + dir * t;

            const char* ext = strrchr(p.stringVal, '.');
            bool isMesh = ext && (ext[0] == '.' &&
                                  (ext[1] == 'o' || ext[1] == 'O') &&
                                  (ext[2] == 'b' || ext[2] == 'B') &&
                                  (ext[3] == 'j' || ext[3] == 'J') &&
                                  ext[4] == '\0');

            const char* fname = p.stringVal;
            const char* sep = strrchr(fname, '/');
            if (!sep) sep = strrchr(fname, '\\');
            if (sep) fname = sep + 1;

            Entity e = em.Create();
            cr.AddComponent<Transform>(e, Transform(dropPos, Quat::Identity(), Vec3(1, 1, 1)));
            cr.AddComponent<Tag>(e, Tag(fname));

            if (isMesh) {
                AssetHandle meshHandle = AssetManager::Get().LoadMesh(p.stringVal);
                Mesh* mesh = AssetManager::Get().GetMesh(meshHandle);
                if (mesh) {
                    cr.AddComponent<MeshRenderer>(e, MeshRenderer(mesh, Color(1, 1, 1, 1)));
                } else {
                    DND_LOG_ERROR("Failed to load mesh: %s", p.stringVal);
                }
            } else {
                AssetHandle texH = AssetManager::Get().LoadTexture(p.stringVal);
                if (texH.IsValid()) {
                    MeshRenderer mr;
                    mr.diffuseTexture = texH;
                    mr.color = Color(1, 1, 1, 1);
                    // Use a unit quad as the default mesh for texture drops
                    static Mesh* s_quadMesh = nullptr;
                    if (!s_quadMesh) {
                        s_quadMesh = new Mesh();
                        *s_quadMesh = Mesh::CreateQuad(1.0f, 1.0f);
                    }
                    mr.mesh = s_quadMesh;
                    cr.AddComponent<MeshRenderer>(e, mr);
                } else {
                    DND_LOG_ERROR("Failed to load texture: %s", p.stringVal);
                }
            }

            UndoStack::Get().Push(new CreateEntityCommand(e, cr));
            DND_LOG_INFO("Viewport drop: created entity '%s' at (%.2f, %.2f, %.2f)", fname, dropPos.x, dropPos.y, dropPos.z);
        };
        DragDropManager::Get().RegisterDropZone(viewportZone);
    }

    // Set up asset browser double-click spawn callback
    {
        s_assetBrowserPanel->SetSpawnCallback([](const std::string& path) {
            if (!s_sceneManager) return;
            Scene* scene = s_sceneManager->GetActiveScene();
            if (!scene) return;
            EntityManager& em = scene->GetEntityManager();
            ComponentRegistry& cr = scene->GetComponentRegistry();

            const char* ext = strrchr(path.c_str(), '.');
            if (!ext) return;
            const char* e = ext + 1;
            bool isMesh = (ext[0] == '.' &&
                           (ext[1] == 'o' || ext[1] == 'O') &&
                           (ext[2] == 'b' || ext[2] == 'B') &&
                           (ext[3] == 'j' || ext[3] == 'J') &&
                           ext[4] == '\0');

            const char* fname = path.c_str();
            const char* sep = strrchr(fname, '/');
            if (!sep) sep = strrchr(fname, '\\');
            if (sep) fname = sep + 1;

            // Spawn at origin
            Entity ent = em.Create();
            cr.AddComponent<Tag>(ent, Tag(fname));
            cr.AddComponent<Transform>(ent, Transform(Vec3(0, 0, 0), Quat::Identity(), Vec3(1, 1, 1)));

            if (isMesh) {
                AssetHandle meshHandle = AssetManager::Get().LoadMesh(path.c_str());
                Mesh* mesh = AssetManager::Get().GetMesh(meshHandle);
                if (mesh) {
                    cr.AddComponent<MeshRenderer>(ent, MeshRenderer(mesh, Color(1, 1, 1, 1)));
                } else {
                    DND_LOG_ERROR("Failed to load mesh: %s", path.c_str());
                }
            } else {
                AssetHandle texH = AssetManager::Get().LoadTexture(path.c_str());
                if (texH.IsValid()) {
                    static Mesh* s_quadMesh = nullptr;
                    if (!s_quadMesh) {
                        s_quadMesh = new Mesh();
                        *s_quadMesh = Mesh::CreateQuad(1.0f, 1.0f);
                    }
                    MeshRenderer mr;
                    mr.diffuseTexture = texH;
                    mr.color = Color(1, 1, 1, 1);
                    mr.mesh = s_quadMesh;
                    cr.AddComponent<MeshRenderer>(ent, mr);
                }
            }

            UndoStack::Get().Push(new CreateEntityCommand(ent, cr));
            DND_LOG_INFO("Spawned entity '%s' from asset browser", fname);
        });
    }

    // Initialize the gizmo manager
    GizmoManager::Get().Initialize();

    DND_LOG_INFO("=== %s v%d.%d.%d initialized successfully ===",
        s_config.applicationName,
        DND_ENGINE_VERSION_MAJOR,
        DND_ENGINE_VERSION_MINOR,
        DND_ENGINE_VERSION_PATCH);

    s_state = EngineState::RUNNING;
    s_initialized = true;

    return true;

cleanup:
    DND_LOG_ERROR("Engine initialization failed — cleaning up...");
    if (uiOk)        UISystem::Get().Shutdown();
    if (debugDrawOk) DebugDraw::Shutdown();
    if (rendererOk)  Renderer::Shutdown();
    if (windowOk)    Window::Destroy();
    Logger::Shutdown();
    s_state = EngineState::SHUTTING_DOWN;
    return false;
}

// ==========================================
// MAIN LOOP
// ==========================================

void Engine::Run() {
    DND_LOG_INFO("Entering main loop...");

    while (s_state != EngineState::SHUTTING_DOWN) {
        Profiler::Get().BeginFrame();

        // Step 1: Update time
        Time::Update();

        // Step 2: Poll window events
        Window::Update();

        // Step 3: Update input state
        Input::Update();

        // Step 4: Check for window close
        if (Window::ShouldClose()) {
            DND_LOG_INFO("Window close requested");
            break;
        }

        // Step 5: Editor panels consume mouse before game UI
        //         (if the editor is active and a panel is being interacted with).
        {
            DockManager::Get().HandleInput(Input::GetMouseX(), Input::GetMouseY(),
                                           Input::IsMouseButtonDown(MouseButton::LEFT));
        }

        // Step 6: Feed the UI the mouse, so it can claim the cursor
        // (e.g. while dragging a slider) before the camera reads it.
        {
            const f32 uiDt = static_cast<f32>(Time::GetDeltaTime());
            UISystem::Get().ProcessMouse(Input::GetMouseX(), Input::GetMouseY(),
                                         Input::IsMouseButtonDown(MouseButton::LEFT));
            UISystem::Get().Update(uiDt);
        }

        // Step 6b: Update drag-and-drop state (after UI input is processed)
        {
            DragDropManager::Get().Update(Input::GetMouseX(), Input::GetMouseY(),
                                          Input::IsMouseButtonDown(MouseButton::LEFT));
        }

        // Step 7: Update docked panel positions (drag following, zone layout)
        {
            DockManager::Get().Update(static_cast<f32>(Time::GetDeltaTime()),
                                      s_config.windowWidth, s_config.windowHeight);
        }

        // Step 7b: Update editor panel contents (hierarchy, inspector, etc.)
        if (EditorSettings::Get().editorActive && s_sceneManager) {
            Scene* scene = s_sceneManager->GetActiveScene();
            if (scene) {
                if (s_hierarchyPanel) {
                    s_hierarchyPanel->Update(scene->GetEntityManager(),
                                             scene->GetComponentRegistry());
                }
                if (s_inspectorPanel) {
                    s_inspectorPanel->Update(scene->GetEntityManager(),
                                             scene->GetComponentRegistry());
                }
            }
        }

        // Step 7c: Update the gizmo manager (viewport picking handles input via HandleInput)
        {
            Scene* gizmoScene = s_sceneManager ? s_sceneManager->GetActiveScene() : nullptr;
            GizmoManager::Get().Update(static_cast<f32>(Time::GetDeltaTime()),
                                       gizmoScene,
                                       static_cast<f32>(s_config.windowWidth),
                                       static_cast<f32>(s_config.windowHeight));
        }

        // Step 8: Handle input (camera controls, escape to quit)
        HandleInput();

        // Step 7: Begin frame (clear buffers)
        Renderer::BeginFrame();

        // Step 8: Update the active ECS scene
        Profiler::Get().BeginSystem("Scene");
        if (s_sceneManager) {
            s_sceneManager->UpdateActiveScene(static_cast<f32>(Time::GetDeltaTime()));
        }
        Profiler::Get().EndSystem();

        Profiler::Get().BeginSystem("Editor");

        // Step 9: Render editor gizmo and debug geometry when editor is active
        if (EditorSettings::Get().editorActive) {
            Scene* gizmoScene = s_sceneManager ? s_sceneManager->GetActiveScene() : nullptr;

            // Draw editor grid
            if (EditorSettings::Get().showGrid) {
                Color gridColor(0.25f, 0.25f, 0.30f, 1.0f);
                f32 gridSize = 20.0f;
                f32 step = 1.0f;
                for (f32 i = -gridSize; i <= gridSize; i += step) {
                    DebugDraw::DrawLine(Vec3(i, 0, -gridSize), Vec3(i, 0, gridSize), gridColor);
                    DebugDraw::DrawLine(Vec3(-gridSize, 0, i), Vec3(gridSize, 0, i), gridColor);
                }
                // Axes lines (brighter)
                DebugDraw::DrawLine(Vec3(-gridSize, 0, 0), Vec3(gridSize, 0, 0), Color(0.4f, 0.2f, 0.2f, 1.0f));
                DebugDraw::DrawLine(Vec3(0, -gridSize, 0), Vec3(0, gridSize, 0), Color(0.2f, 0.4f, 0.2f, 1.0f));
                DebugDraw::DrawLine(Vec3(0, 0, -gridSize), Vec3(0, 0, gridSize), Color(0.2f, 0.2f, 0.4f, 1.0f));
            }

            GizmoManager::Get().Render(gizmoScene);

            // Draw collider wireframes for the selected entity
            if (gizmoScene && EditorSettings::Get().showColliders) {
                Entity sel = EditorSettings::Get().selectedEntity;
                if (sel.IsValid()) {
                    ComponentRegistry& cr = gizmoScene->GetComponentRegistry();
                    Transform* t = cr.GetComponent<Transform>(sel);
                    Vec3 worldPos = t ? t->position : Vec3(0, 0, 0);

                    SphereCollider* sc = cr.GetComponent<SphereCollider>(sel);
                    if (sc && sc->isEnabled) {
                        DebugDraw::DrawSphere(worldPos + sc->center, sc->radius, 16, Color(0.0f, 1.0f, 0.3f, 1.0f));
                    }

                    BoxCollider* bc = cr.GetComponent<BoxCollider>(sel);
                    if (bc && bc->isEnabled) {
                        Color col(0.0f, 0.5f, 1.0f, 1.0f);
                        DebugDraw::DrawBox(worldPos + bc->center, bc->halfExtents, col);
                    }

                    CapsuleCollider* cc = cr.GetComponent<CapsuleCollider>(sel);
                    if (cc && cc->isEnabled) {
                        Vec3 c = worldPos + cc->center;
                        f32 halfH = cc->height * 0.5f;
                        Color col(1.0f, 0.5f, 0.0f, 1.0f);
                        DebugDraw::DrawSphere(c + Vec3(0, halfH, 0), cc->radius, 12, col);
                        DebugDraw::DrawSphere(c - Vec3(0, halfH, 0), cc->radius, 12, col);
                        DebugDraw::DrawLine(c + Vec3(0, halfH, 0), c - Vec3(0, halfH, 0), col);
                    }
                }
            }

            const Mat4& viewMat = Renderer::GetViewMatrix();
            const Mat4& projMat = Renderer::GetProjectionMatrix();
            DebugDraw::Flush(viewMat, projMat);
        }

        // Step 10: Draw the UI overlay on top of the 3D scene.
        const Mat4 uiOrtho = Mat4::Orthographic(
            0.0f, static_cast<f32>(s_config.windowWidth),
            static_cast<f32>(s_config.windowHeight), 0.0f, -1.0f, 1.0f);

        // When editor is active, skip game UI rendering (editor panels replace it)
        if (!EditorSettings::Get().editorActive) {
            Profiler::Get().BeginSystem("Game UI");
            UISystem::Get().Render(uiOrtho);

            if (s_sceneManager) {
                Scene* hudScene = s_sceneManager->GetActiveScene();
                if (hudScene) {
                    UIRenderer::BeginFrame(uiOrtho);
                    hudScene->OnRenderOverlay(static_cast<f32>(s_config.windowWidth),
                                              static_cast<f32>(s_config.windowHeight));
                    UIRenderer::EndFrame();
                }
            }
            Profiler::Get().EndSystem();
        } else {
            UIRenderer::BeginFrame(uiOrtho);
            DockManager::Get().Render(uiOrtho);
            DragDropManager::Get().RenderGhost(uiOrtho);
            UIRenderer::EndFrame();
        }

        Profiler::Get().EndSystem(); // Editor

        // Step 11: Profiler overlay (if visible)
        Profiler::Get().EndFrame();
        if (Profiler::Get().IsVisible()) {
            Scene* profScene = s_sceneManager ? s_sceneManager->GetActiveScene() : nullptr;
            if (profScene) {
                Profiler::Get().SetEntityCount(profScene->GetEntityCount());
            }
            Profiler::Get().SetDrawCalls(Renderer::GetDrawCallCount());

            UIRenderer::BeginFrame(uiOrtho);
            Profiler::Get().Render(static_cast<u32>(s_config.windowWidth),
                                   static_cast<u32>(s_config.windowHeight));
            UIRenderer::EndFrame();
        }

        // Step 12: End frame (swap buffers)
        Renderer::EndFrame();
    }

    DND_LOG_INFO("Main loop exited.");
}

// ==========================================
// INPUT HANDLING
// ==========================================

void Engine::HandleInput() {
    // Escape during a drag operation: cancel
    if (Input::IsKeyPressed(KeyCode::ESCAPE) && DragDropManager::Get().IsDragging()) {
        DragDropManager::Get().CancelDrag();
        return;
    }

    // Escape key: trigger shutdown
    if (Input::IsKeyPressed(KeyCode::ESCAPE) && !Input::IsTextInputActive()) {
        DND_LOG_INFO("Escape pressed — shutting down");
        s_state = EngineState::SHUTTING_DOWN;
        return;
    }

    // F2 key: toggle profiler overlay
    if (Input::IsKeyPressed(KeyCode::F2)) {
        Profiler::Get().SetVisible(!Profiler::Get().IsVisible());
        DND_LOG_INFO("Profiler %s", Profiler::Get().IsVisible() ? "shown" : "hidden");
    }

    // F6 key: toggle pause
    if (Input::IsKeyPressed(KeyCode::F6)) {
        Time::SetPaused(!Time::GetPaused());
        DND_LOG_INFO("Simulation %s", Time::GetPaused() ? "PAUSED" : "resumed");
    }

    // F5 key: toggle editor mode
    if (Input::IsKeyPressed(KeyCode::F5)) {
        EditorSettings::Get().editorActive = !EditorSettings::Get().editorActive;
        DockManager::Get().SetActive(EditorSettings::Get().editorActive);
        DND_LOG_INFO("Editor %s", EditorSettings::Get().editorActive ? "activated" : "deactivated");
    }

    // ---- Orbit camera ----
    // Left-drag (or arrow keys) orbits around the battlefield; scroll zooms.
    // The camera always looks at the focal point, so it stays framed on the scene.
    const f32 dt = static_cast<f32>(Time::GetDeltaTime());

    // Suppress camera orbit when the editor is active and using the mouse
    b8 editorUsingMouse = EditorSettings::Get().editorActive &&
        (DockManager::Get().IsDragging() || DockManager::Get().HitTest(Input::GetMouseX(), Input::GetMouseY()) != nullptr);

    if (Input::IsMouseButtonDown(MouseButton::LEFT) && !UISystem::Get().IsCapturingMouse() && !editorUsingMouse) {
        s_cameraYaw   += Input::GetMouseDeltaX() * 0.005f;
        s_cameraPitch += Input::GetMouseDeltaY() * 0.005f;
    }
    // Keyboard camera controls are suppressed while typing in the command box.
    if (!Input::IsTextInputActive()) {
        const f32 keySpeed = 2.0f; // radians/second
        if (Input::IsKeyDown(KeyCode::LEFT)  || Input::IsKeyDown(KeyCode::A)) s_cameraYaw   -= keySpeed * dt;
        if (Input::IsKeyDown(KeyCode::RIGHT) || Input::IsKeyDown(KeyCode::D)) s_cameraYaw   += keySpeed * dt;
        if (Input::IsKeyDown(KeyCode::UP)    || Input::IsKeyDown(KeyCode::W)) s_cameraPitch += keySpeed * dt;
        if (Input::IsKeyDown(KeyCode::DOWN)  || Input::IsKeyDown(KeyCode::S)) s_cameraPitch -= keySpeed * dt;
        s_cameraDistance -= Input::GetScrollDelta() * 1.5f;
        // +/- keys (main row or numpad) also zoom.
        const f32 zoomKeySpeed = 10.0f; // units/second
        if (Input::IsKeyDown(KeyCode::EQUAL) || Input::IsKeyDown(KeyCode::KP_ADD))      s_cameraDistance -= zoomKeySpeed * dt;
        if (Input::IsKeyDown(KeyCode::MINUS) || Input::IsKeyDown(KeyCode::KP_SUBTRACT)) s_cameraDistance += zoomKeySpeed * dt;
    }

    // Keep pitch off the poles and distance sane.
    const f32 minPitch = Math::DegreesToRadians(5.0f);
    const f32 maxPitch = Math::DegreesToRadians(85.0f);
    if (s_cameraPitch < minPitch) s_cameraPitch = minPitch;
    if (s_cameraPitch > maxPitch) s_cameraPitch = maxPitch;
    if (s_cameraDistance < 4.0f)  s_cameraDistance = 4.0f;
    if (s_cameraDistance > 60.0f) s_cameraDistance = 60.0f; // allow zooming out over a large maze

    // Focal point: smoothly follow whatever the active scene asked us to track
    // (the hero, as he explores). Frame-rate-independent easing.
    const f32 followLerp = 1.0f - std::exp(-7.0f * dt);
    s_cameraFocusSmoothed.x += (s_cameraFocus.x - s_cameraFocusSmoothed.x) * followLerp;
    s_cameraFocusSmoothed.y += (s_cameraFocus.y - s_cameraFocusSmoothed.y) * followLerp;
    s_cameraFocusSmoothed.z += (s_cameraFocus.z - s_cameraFocusSmoothed.z) * followLerp;
    const Vec3 target = s_cameraFocusSmoothed;

    // ---- Gizmo mode switching (editor only) ----
    if (EditorSettings::Get().editorActive && !Input::IsTextInputActive()) {
        b8 ctrlHeld = Input::IsKeyDown(KeyCode::LEFT_CONTROL) || Input::IsKeyDown(KeyCode::RIGHT_CONTROL);

        if (ctrlHeld && Input::IsKeyPressed(KeyCode::S) && s_sceneManager) {
            Scene* scene = s_sceneManager->GetActiveScene();
            if (scene) {
                SceneSerializer::Save(*scene, "scene_save.json");
                DND_LOG_INFO("Scene saved to scene_save.json");
            }
        }

        if (ctrlHeld && Input::IsKeyPressed(KeyCode::O) && s_sceneManager) {
            Scene* scene = s_sceneManager->GetActiveScene();
            if (scene) {
                SceneSerializer::Load(*scene, "scene_save.json");
                DND_LOG_INFO("Scene loaded from scene_save.json");
            }
        }

        // Ctrl+Z: undo, Ctrl+Y or Ctrl+Shift+Z: redo
        if (ctrlHeld && Input::IsKeyPressed(KeyCode::Z) && s_sceneManager) {
            Scene* scene = s_sceneManager->GetActiveScene();
            if (scene) {
                b8 shiftHeld = Input::IsKeyDown(KeyCode::LEFT_SHIFT) || Input::IsKeyDown(KeyCode::RIGHT_SHIFT);
                if (shiftHeld) {
                    if (UndoStack::Get().Redo(scene->GetComponentRegistry(), scene->GetEntityManager()))
                        DND_LOG_DEBUG("Redo");
                } else {
                    if (UndoStack::Get().Undo(scene->GetComponentRegistry(), scene->GetEntityManager()))
                        DND_LOG_DEBUG("Undo");
                }
            }
        }
        if (ctrlHeld && Input::IsKeyPressed(KeyCode::Y) && s_sceneManager) {
            Scene* scene = s_sceneManager->GetActiveScene();
            if (scene) {
                if (UndoStack::Get().Redo(scene->GetComponentRegistry(), scene->GetEntityManager()))
                    DND_LOG_DEBUG("Redo");
            }
        }

        if (Input::IsKeyPressed(KeyCode::D1)) {
            EditorSettings::Get().gizmoMode = GizmoMode::Translate;
            DND_LOG_DEBUG("Gizmo mode: Translate");
        } else if (Input::IsKeyPressed(KeyCode::D2)) {
            EditorSettings::Get().gizmoMode = GizmoMode::Rotate;
            DND_LOG_DEBUG("Gizmo mode: Rotate");
        } else if (Input::IsKeyPressed(KeyCode::D3)) {
            EditorSettings::Get().gizmoMode = GizmoMode::Scale;
            DND_LOG_DEBUG("Gizmo mode: Scale");
        }

        // F key: focus camera on selected entity
        if (Input::IsKeyPressed(KeyCode::F)) {
            Scene* scene = s_sceneManager ? s_sceneManager->GetActiveScene() : nullptr;
            Entity sel = EditorSettings::Get().selectedEntity;
            if (scene && sel.IsValid()) {
                Transform* xf = scene->GetComponentRegistry().GetComponent<Transform>(sel);
                if (xf) {
                    s_cameraFocus = xf->position;
                    s_cameraFocusSmoothed = xf->position;
                    s_cameraDistance = 8.0f;
                    DND_LOG_DEBUG("Camera focused on entity %u", sel.id);
                }
            }
        }

        // G key: toggle grid visualization
        if (Input::IsKeyPressed(KeyCode::G)) {
            EditorSettings::Get().showGrid = !EditorSettings::Get().showGrid;
            DND_LOG_INFO("Editor grid: %s",
                EditorSettings::Get().showGrid ? "ON" : "OFF");
        }

        // C key: toggle collider visualization
        if (Input::IsKeyPressed(KeyCode::C)) {
            EditorSettings::Get().showColliders = !EditorSettings::Get().showColliders;
            DND_LOG_INFO("Collider visualization: %s",
                EditorSettings::Get().showColliders ? "ON" : "OFF");
        }

        // Delete key: destroy selected entity (with undo)
        if (Input::IsKeyPressed(KeyCode::DELETE_KEY) && s_sceneManager) {
            Scene* scene = s_sceneManager->GetActiveScene();
            if (scene) {
                Entity sel = EditorSettings::Get().selectedEntity;
                if (sel.IsValid() && scene->GetEntityManager().IsValid(sel)) {
                    UndoStack::Get().Push(new DeleteEntityCommand(sel, scene->GetComponentRegistry()));
                    scene->GetEntityManager().Destroy(sel);
                    EditorSettings::Get().selectedEntity = Entity::None();
                    DND_LOG_DEBUG("Deleted entity %u", sel.id);
                }
            }
        }

        // Ctrl+Shift+N: create empty entity (with undo)
        if (ctrlHeld && Input::IsKeyPressed(KeyCode::N) && s_sceneManager) {
            Scene* scene = s_sceneManager->GetActiveScene();
            if (scene) {
                EntityManager& em = scene->GetEntityManager();
                ComponentRegistry& cr = scene->GetComponentRegistry();
                Entity e = em.Create();
                cr.AddComponent<Transform>(e, Transform(Vec3(0, 0, 0), Quat::Identity(), Vec3(1, 1, 1)));
                cr.AddComponent<Tag>(e, Tag("New Entity"));
                UndoStack::Get().Push(new CreateEntityCommand(e, cr));
                EditorSettings::Get().selectedEntity = e;
                DND_LOG_DEBUG("Created empty entity %u", e.id);
            }
        }
    }

    // Spherical orbit position around the target.
    const f32 cp = std::cos(s_cameraPitch), sp = std::sin(s_cameraPitch);
    const f32 cy = std::cos(s_cameraYaw),   sy = std::sin(s_cameraYaw);
    const Vec3 eye(
        target.x + s_cameraDistance * cp * sy,
        target.y + s_cameraDistance * sp,
        target.z + s_cameraDistance * cp * cy
    );

    // Drive the active scene's camera entity.
    if (!s_sceneManager) return;
    Scene* scene = s_sceneManager->GetActiveScene();
    if (!scene) return;
    ComponentRegistry& cr = scene->GetComponentRegistry();
    auto* camSet = cr.GetSparseSet<ECSCamera>();
    auto* xfSet  = cr.GetSparseSet<Transform>();
    if (!camSet || !xfSet) return;
    for (u32 i = 0; i < camSet->Size(); ++i) {
        if (!camSet->GetDense()[i].isActive) continue;
        Transform* t = xfSet->Get(camSet->GetEntityIndex(i));
        if (t) {
            t->position = eye;
            t->rotation = Quat::LookAt(eye, target, Vec3(0.0f, 1.0f, 0.0f));
        }
        break;
    }

    // ---- Viewport picking (editor only) ----
    // Skip if the gizmo is being dragged (it handles its own input)
    if (EditorSettings::Get().editorActive &&
        !GizmoManager::Get().IsDragging() &&
        Input::IsMouseButtonPressed(MouseButton::LEFT) &&
        !UISystem::Get().IsCapturingMouse() &&
        !editorUsingMouse) {

        // Build ray from camera through mouse position
        f32 mx = Input::GetMouseX();
        f32 my = Input::GetMouseY();
        f32 vpW = static_cast<f32>(s_config.windowWidth);
        f32 vpH = static_cast<f32>(s_config.windowHeight);

        // NDC: flip Y because window Y is top-down, NDC Y is bottom-up
        f32 ndcX = (2.0f * mx / vpW) - 1.0f;
        f32 ndcY = 1.0f - (2.0f * my / vpH);

        // Build camera view matrix from the orbit camera
        Mat4 viewMat = Mat4::TRS(eye, Quat::LookAt(eye, target, Vec3(0,1,0)), Vec3(1,1,1)).Inverse();

        // Get projection matrix from the active camera component
        Mat4 projMat;
        {
            ComponentRegistry& cr2 = scene->GetComponentRegistry();
            auto* camSet2 = cr2.GetSparseSet<ECSCamera>();
            if (camSet2 && camSet2->Size() > 0) {
                // Assume first camera is the active one
                projMat = camSet2->GetDense()[0].GetProjectionMatrix();
            } else {
                projMat = Mat4::Perspective(60.0f * 0.0174533f, vpW / vpH, 0.1f, 1000.0f);
            }
        }

        // Unproject near and far points
        Mat4 invVP = (projMat * viewMat).Inverse();
        Vec4 nearH = invVP * Vec4(ndcX, ndcY, -1.0f, 1.0f);
        Vec4 farH  = invVP * Vec4(ndcX, ndcY,  1.0f, 1.0f);
        Vec3 nearW = nearH.ToVec3() / nearH.w;
        Vec3 farW  = farH.ToVec3() / farH.w;

        Ray pickRay(nearW, (farW - nearW).Normalized());

        // Test against all alive entities with Transform components
        f32 closestT = 1e30f;
        Entity closestEntity = Entity::None();

        scene->GetEntityManager().ForEachAlive([&](Entity e) {
            u32 idx = Entity::GetIndex(e.id);
            auto* xfSet2 = cr.GetSparseSet<Transform>();
            if (!xfSet2 || !xfSet2->Has(idx)) return;

            const Transform& xf = *xfSet2->Get(idx);

            // Compute approximate AABB from transform
            Vec3 halfExt = xf.scale * 0.5f;
            if (halfExt.LengthSquared() < 0.01f) {
                halfExt = Vec3(0.5f, 0.5f, 0.5f); // default size
            }
            Vec3 min = xf.position - halfExt;
            Vec3 max = xf.position + halfExt;

            f32 t;
            if (Collision::IntersectRayAABB(pickRay, min, max, t)) {
                if (t < closestT) {
                    closestT = t;
                    closestEntity = e;
                }
            }
        });

        EditorSettings::Get().selectedEntity = closestEntity;
        if (closestEntity.IsValid()) {
            DND_LOG_DEBUG("Picked entity %u (t=%.2f)", closestEntity.id, closestT);
        } else {
            DND_LOG_DEBUG("No entity picked");
        }
    }
}

// ==========================================
// SHUTDOWN
// ==========================================

void Engine::Shutdown() {
    if (!s_initialized) {
        return;
    }

    s_state = EngineState::SHUTTING_DOWN;
    DND_LOG_INFO("Engine shutting down...");

    // Shutdown in REVERSE order of initialization:

    // Editor panels (clean up heap-allocated panels before UI system shuts down)
    delete s_hierarchyPanel;
    s_hierarchyPanel = nullptr;
    delete s_inspectorPanel;
    s_inspectorPanel = nullptr;
    delete s_assetBrowserPanel;
    s_assetBrowserPanel = nullptr;
    GizmoManager::Get().Shutdown();
    DockManager::Get().ClearPanels();
    DockManager::Get().SetActive(false);

    // ECS Scene Manager
    DND_LOG_INFO("Shutting down Scene Manager...");
    if (s_sceneManager) { delete s_sceneManager; s_sceneManager = nullptr; }

    // UI System (frees its GL shader + mesh before the renderer goes away)
    DND_LOG_INFO("Shutting down UI System...");
    UISystem::Get().Shutdown();

    // Debug Draw
    DND_LOG_INFO("Shutting down DebugDraw...");
    DebugDraw::Shutdown();

    // Renderer
    DND_LOG_INFO("Shutting down Renderer...");
    Renderer::Shutdown();

    // Input
    DND_LOG_INFO("Shutting down Input...");
    // Input has no explicit shutdown

    // Window
    DND_LOG_INFO("Shutting down Window...");
    Window::Destroy();

    // Time
    DND_LOG_INFO("Shutting down Time...");
    // Time has no explicit shutdown

    // Logger (MUST be last)
    DND_LOG_INFO("Shutting down Logger...");
    Logger::Shutdown();

    s_initialized = false;
    DND_LOG_INFO("Engine shutdown complete");
}

// ==========================================
// STATE MANAGEMENT
// ==========================================

EngineState Engine::GetState() {
    return s_state;
}

void Engine::SetState(EngineState state) {
    s_state = state;
}

void Engine::SetCameraFocus(const Vec3& focus) {
    s_cameraFocus = focus;
}

} // namespace Dnd
