#include "tools/inspector_panel.h"
#include "tools/editor_settings.h"
#include "tools/undo.h"
#include "ui/ui_widget.h"
#include "ui/drag_drop.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include "assets/asset_manager.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"
#include "core/log.h"
#include <cstdio>
#include <cstring>   // std::memcmp (MSVC pulls this in transitively; MinGW needs it explicit)
#include "ecs/components/transform.h"
#include "ecs/components/tag.h"
#include "ecs/components/rigidbody.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/camera.h"
#include "ecs/components/light.h"
#include "audio/audio_source.h"
#include "audio/audio_clip.h"

namespace Dnd {

InspectorPanel::InspectorPanel() {
    m_grid = new UIPropertyGrid("InspectorGrid");

    m_dockPanel = new DockPanel("Inspector", "Inspector");
    m_dockPanel->SetContent(m_grid);
    m_dockPanel->SetCloseCallback([this]() {
        m_dockPanel->SetVisible(false);
    });

    // Register a drop zone so assets can be assigned to the selected entity
    DropZone inspZone;
    inspZone.widget = m_grid;
    inspZone.area = DropZoneRect(0, 0, 10000, 10000);
    inspZone.canAccept = [](const DragPayload& p) -> bool {
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
        if (isImage) return true;
        bool isAudio = (std::strcmp(e, "wav") == 0 || std::strcmp(e, "WAV") == 0 ||
                        std::strcmp(e, "mp3") == 0 || std::strcmp(e, "MP3") == 0 ||
                        std::strcmp(e, "flac") == 0 || std::strcmp(e, "FLAC") == 0 ||
                        std::strcmp(e, "ogg") == 0 || std::strcmp(e, "OGG") == 0);
        return isAudio;
    };
    inspZone.onDrop = [this](const DragPayload& p) {
        m_pendingDropPath = p.stringVal ? p.stringVal : "";
    };
    DragDropManager::Get().RegisterDropZone(inspZone);
}

InspectorPanel::~InspectorPanel() {
}

void InspectorPanel::Update(EntityManager& em, ComponentRegistry& cr) {
    if (!m_dockPanel->IsVisible()) return;

    // Process pending drop from drag-and-drop
    if (!m_pendingDropPath.empty()) {
        Entity sel = EditorSettings::Get().selectedEntity;
        if (sel.IsValid()) {
            const char* ext = strrchr(m_pendingDropPath.c_str(), '.');
            if (ext) {
                bool isMesh = (ext[0] == '.' &&
                               (ext[1] == 'o' || ext[1] == 'O') &&
                               (ext[2] == 'b' || ext[2] == 'B') &&
                               (ext[3] == 'j' || ext[3] == 'J') &&
                               ext[4] == '\0');
                if (isMesh) {
                    EntitySnapshot before;
                    CaptureEntitySnapshot(sel, cr, before);

                    AssetHandle h = AssetManager::Get().LoadMesh(m_pendingDropPath.c_str());
                    Mesh* mesh = AssetManager::Get().GetMesh(h);
                    if (mesh) {
                        if (cr.HasComponent<MeshRenderer>(sel)) {
                            cr.GetComponent<MeshRenderer>(sel)->mesh = mesh;
                        } else {
                            cr.AddComponent<MeshRenderer>(sel, MeshRenderer(mesh, Color(1,1,1,1)));
                        }
                        EntitySnapshot after;
                        CaptureEntitySnapshot(sel, cr, after);
                        UndoStack::Get().Push(new PropertyEditCommand(sel, before, after));
                        m_lastEntity = Entity::None();
                        DND_LOG_INFO("Assigned mesh %s to entity %u", m_pendingDropPath.c_str(), sel.id);
                    }
                } else {
                    // Check if it's an image file (.png, .jpg, .jpeg)
                    bool isImage = false;
                    const char* e = ext + 1;
                    isImage = (std::strcmp(e, "png") == 0 || std::strcmp(e, "PNG") == 0 ||
                               std::strcmp(e, "jpg") == 0 || std::strcmp(e, "JPG") == 0 ||
                               std::strcmp(e, "jpeg") == 0 || std::strcmp(e, "JPEG") == 0);
                    if (isImage) {
                        EntitySnapshot before;
                        CaptureEntitySnapshot(sel, cr, before);
                        AssetHandle texH = AssetManager::Get().LoadTexture(m_pendingDropPath.c_str());
                        if (texH.IsValid()) {
                            if (cr.HasComponent<MeshRenderer>(sel)) {
                                cr.GetComponent<MeshRenderer>(sel)->diffuseTexture = texH;
                            } else {
                                MeshRenderer mr;
                                mr.diffuseTexture = texH;
                                cr.AddComponent<MeshRenderer>(sel, mr);
                            }
                            EntitySnapshot after;
                            CaptureEntitySnapshot(sel, cr, after);
                            UndoStack::Get().Push(new PropertyEditCommand(sel, before, after));
                            m_lastEntity = Entity::None();
                            DND_LOG_INFO("Assigned texture %s to entity %u", m_pendingDropPath.c_str(), sel.id);
                        }
                    }
                }
            }
        }
        m_pendingDropPath.clear();
    }

    Entity sel = EditorSettings::Get().selectedEntity;
    if (!sel.IsValid()) {
        if (m_lastEntity.IsValid()) {
            m_grid->Clear();
            m_grid->AddLabel("No entity selected", "");
            m_lastEntity = Entity::None();
        }
        return;
    }

    // Only rebuild when selection changes
    if (sel == m_lastEntity) return;
    m_lastEntity = sel;

    m_grid->Clear();

    // Capture snapshot for undo commit tracking
    m_sceneCR = &cr;
    m_sceneEM = &em;
    CaptureEntitySnapshot(sel, cr, m_snapshot);
    m_grid->SetCommitCallback([this]() {
        Entity sel = EditorSettings::Get().selectedEntity;
        if (!sel.IsValid() || !m_sceneEM->IsValid(sel)) return;
        EntitySnapshot after;
        CaptureEntitySnapshot(sel, *m_sceneCR, after);
        bool changed = false;
        const EntitySnapshot& a = m_snapshot;
        const EntitySnapshot& b = after;
        changed |= (a.hasTransform != b.hasTransform);
        if (!changed && a.hasTransform)
            changed |= (std::memcmp(&a.transform, &b.transform, sizeof(Transform)) != 0);
        changed |= (a.hasTag != b.hasTag);
        if (!changed && a.hasTag)
            changed |= (a.tag.nameHash != b.tag.nameHash);
        changed |= (a.hasMeshRenderer != b.hasMeshRenderer);
        if (!changed && a.hasMeshRenderer)
            changed |= (std::memcmp(&a.meshRenderer, &b.meshRenderer, sizeof(MeshRenderer)) != 0);
        changed |= (a.hasRigidBody != b.hasRigidBody);
        if (!changed && a.hasRigidBody)
            changed |= (std::memcmp(&a.rigidBody, &b.rigidBody, sizeof(RigidBody)) != 0);
        changed |= (a.hasCamera != b.hasCamera);
        if (!changed && a.hasCamera)
            changed |= (std::memcmp(&a.camera, &b.camera, sizeof(ECSCamera)) != 0);
        changed |= (a.hasLight != b.hasLight);
        if (!changed && a.hasLight)
            changed |= (std::memcmp(&a.light, &b.light, sizeof(Light)) != 0);
        changed |= (a.hasAudioSource != b.hasAudioSource);
        if (!changed && a.hasAudioSource)
            changed |= (std::memcmp(&a.audioSource, &b.audioSource, sizeof(AudioSource)) != 0);
        if (changed) {
            UndoStack::Get().Push(new PropertyEditCommand(sel, m_snapshot, after));
            m_snapshot = after;
        }
    });

    if (!em.IsValid(sel)) {
        m_grid->AddLabel("Entity", "Invalid / destroyed");
        return;
    }

    // ==========================================
    // TRANSFORM
    // ==========================================
    const Transform* xf = cr.GetComponent<Transform>(sel);
    if (xf) {
        m_grid->AddCategory("Transform");

        m_grid->AddVec3("Position", xf->position,
            [this, sel](const Vec3& v) {
                if (Transform* t = m_sceneCR->GetComponent<Transform>(sel)) {
                    t->position = v;
                }
            });

        m_grid->AddVec3("Rotation", Vec3(xf->rotation.x, xf->rotation.y, xf->rotation.z),
            [this, sel](const Vec3& v) {
                if (Transform* t = m_sceneCR->GetComponent<Transform>(sel)) {
                    t->rotation.x = v.x;
                    t->rotation.y = v.y;
                    t->rotation.z = v.z;
                }
            });
        m_grid->AddFloat("RotW", xf->rotation.w,
            [this, sel](f32 v) {
                if (Transform* t = m_sceneCR->GetComponent<Transform>(sel)) {
                    t->rotation.w = v;
                }
            });

        m_grid->AddVec3("Scale", xf->scale,
            [this, sel](const Vec3& v) {
                if (Transform* t = m_sceneCR->GetComponent<Transform>(sel)) {
                    t->scale = v;
                }
            });

        // Parent entity — read-only
        char parentBuf[64];
        if (xf->parent.IsValid()) {
            snprintf(parentBuf, sizeof(parentBuf), "Entity %u", xf->parent.id);
        } else {
            snprintf(parentBuf, sizeof(parentBuf), "None (root)");
        }
        m_grid->AddLabel("Parent", parentBuf);
    }

    // ==========================================
    // TAG
    // ==========================================
    const Tag* tag = cr.GetComponent<Tag>(sel);
    if (tag) {
        m_grid->AddCategory("Tag");
        m_grid->AddString("Name", tag->name,
            [this, sel](const std::string& v) {
                if (Tag* t = m_sceneCR->GetComponent<Tag>(sel)) {
                    t->SetName(v);
                }
            });
        char hashBuf[32];
        snprintf(hashBuf, sizeof(hashBuf), "%u", tag->nameHash);
        m_grid->AddLabel("Name Hash", hashBuf);
    }

    // ==========================================
    // RIGIDBODY
    // ==========================================
    const RigidBody* rb = cr.GetComponent<RigidBody>(sel);
    if (rb) {
        m_grid->AddCategory("RigidBody");

        m_grid->AddVec3("Velocity", rb->velocity,
            [this, sel](const Vec3& v) {
                if (RigidBody* r = m_sceneCR->GetComponent<RigidBody>(sel)) { r->velocity = v; }
            });
        m_grid->AddVec3("Acceleration", rb->acceleration,
            [this, sel](const Vec3& v) {
                if (RigidBody* r = m_sceneCR->GetComponent<RigidBody>(sel)) { r->acceleration = v; }
            });
        m_grid->AddVec3("Gravity", rb->gravity,
            [this, sel](const Vec3& v) {
                if (RigidBody* r = m_sceneCR->GetComponent<RigidBody>(sel)) { r->gravity = v; }
            });
        m_grid->AddFloat("Mass", rb->mass,
            [this, sel](f32 v) {
                if (RigidBody* r = m_sceneCR->GetComponent<RigidBody>(sel)) { r->mass = v; }
            });
        m_grid->AddFloat("Damping", rb->damping,
            [this, sel](f32 v) {
                if (RigidBody* r = m_sceneCR->GetComponent<RigidBody>(sel)) { r->damping = v; }
            });
        m_grid->AddBool("Is Static", rb->isStatic,
            [this, sel](b8 v) {
                if (RigidBody* r = m_sceneCR->GetComponent<RigidBody>(sel)) { r->isStatic = v; }
            });
    }

    // ==========================================
    // MESH RENDERER
    // ==========================================
    const MeshRenderer* mr = cr.GetComponent<MeshRenderer>(sel);
    if (mr) {
        m_grid->AddCategory("Mesh Renderer");

        char meshBuf[64];
        snprintf(meshBuf, sizeof(meshBuf), mr->mesh ? "Mesh* %p" : "None",
                 static_cast<const void*>(mr->mesh));
        m_grid->AddLabel("Mesh", meshBuf);

        m_grid->AddVec3("Color RGB", Vec3(mr->color.r, mr->color.g, mr->color.b),
            [this, sel](const Vec3& v) {
                if (MeshRenderer* m = m_sceneCR->GetComponent<MeshRenderer>(sel)) {
                    m->color.r = v.x;
                    m->color.g = v.y;
                    m->color.b = v.z;
                }
            });
        m_grid->AddFloat("Alpha", mr->color.a,
            [this, sel](f32 v) {
                if (MeshRenderer* m = m_sceneCR->GetComponent<MeshRenderer>(sel)) { m->color.a = v; }
            });
        m_grid->AddBool("Visible", mr->visible,
            [this, sel](b8 v) {
                if (MeshRenderer* m = m_sceneCR->GetComponent<MeshRenderer>(sel)) { m->visible = v; }
            });

        // Show diffuse texture path if available
        const char* texPath = mr->diffuseTexture.IsValid()
            ? AssetManager::Get().GetAssetPath(mr->diffuseTexture)
            : "None";
        m_grid->AddLabel("Diffuse Tex", texPath ? texPath : "None");

        m_grid->AddFloat("Shininess", mr->shininess,
            [this, sel](f32 v) {
                if (MeshRenderer* m = m_sceneCR->GetComponent<MeshRenderer>(sel)) { m->shininess = v; }
            });
    }

    // ==========================================
    // CAMERA (ECSCamera)
    // ==========================================
    const ECSCamera* cam = cr.GetComponent<ECSCamera>(sel);
    if (cam) {
        m_grid->AddCategory("Camera");

        m_grid->AddFloat("FOV", cam->fov,
            [this, sel](f32 v) {
                if (ECSCamera* c = m_sceneCR->GetComponent<ECSCamera>(sel)) { c->fov = v; }
            });
        m_grid->AddFloat("Near", cam->nearPlane,
            [this, sel](f32 v) {
                if (ECSCamera* c = m_sceneCR->GetComponent<ECSCamera>(sel)) { c->nearPlane = v; }
            });
        m_grid->AddFloat("Far", cam->farPlane,
            [this, sel](f32 v) {
                if (ECSCamera* c = m_sceneCR->GetComponent<ECSCamera>(sel)) { c->farPlane = v; }
            });
        char aspectBuf[32];
        snprintf(aspectBuf, sizeof(aspectBuf), "%.4f", cam->aspectRatio);
        m_grid->AddLabel("Aspect", aspectBuf);
        m_grid->AddBool("Is Active", cam->isActive,
            [this, sel](b8 v) {
                if (ECSCamera* c = m_sceneCR->GetComponent<ECSCamera>(sel)) { c->isActive = v; }
            });
        m_grid->AddBool("Orthographic", cam->isOrthographic,
            [this, sel](b8 v) {
                if (ECSCamera* c = m_sceneCR->GetComponent<ECSCamera>(sel)) { c->isOrthographic = v; }
            });
    }

    // ==========================================
    // LIGHT
    // ==========================================
    const Light* light = cr.GetComponent<Light>(sel);
    if (light) {
        m_grid->AddCategory("Light");

        std::vector<std::string> lightTypes = { "Directional", "Point", "Spot" };
        m_grid->AddEnum("Type", static_cast<i32>(light->type), lightTypes,
            [this, sel](i32 v) {
                if (Light* l = m_sceneCR->GetComponent<Light>(sel)) {
                    l->type = static_cast<LightType>(v);
                }
            });

        m_grid->AddVec3("Color", light->color,
            [this, sel](const Vec3& v) {
                if (Light* l = m_sceneCR->GetComponent<Light>(sel)) { l->color = v; }
            });
        m_grid->AddFloat("Intensity", light->intensity,
            [this, sel](f32 v) {
                if (Light* l = m_sceneCR->GetComponent<Light>(sel)) { l->intensity = v; }
            });
        m_grid->AddFloat("Range", light->range,
            [this, sel](f32 v) {
                if (Light* l = m_sceneCR->GetComponent<Light>(sel)) { l->range = v; }
            });
        m_grid->AddFloat("Spot Angle", light->spotAngle,
            [this, sel](f32 v) {
                if (Light* l = m_sceneCR->GetComponent<Light>(sel)) { l->spotAngle = v; }
            });
    }

    // ==========================================
    // AUDIO SOURCE
    // ==========================================
    const AudioSource* audio = cr.GetComponent<AudioSource>(sel);
    if (audio) {
        m_grid->AddCategory("Audio Source");

        m_grid->AddLabel("Clip", audio->clip ? audio->clip->GetName() : "(none)");
        m_grid->AddFloat("Volume", audio->volume,
            [this, sel](f32 v) {
                if (AudioSource* a = m_sceneCR->GetComponent<AudioSource>(sel)) { a->volume = v; }
            });
        m_grid->AddFloat("Pitch", audio->pitch,
            [this, sel](f32 v) {
                if (AudioSource* a = m_sceneCR->GetComponent<AudioSource>(sel)) { a->pitch = v; }
            });
        m_grid->AddBool("Looping", audio->isLooping,
            [this, sel](b8 v) {
                if (AudioSource* a = m_sceneCR->GetComponent<AudioSource>(sel)) { a->isLooping = v; }
            });
        m_grid->AddBool("3D", audio->is3D,
            [this, sel](b8 v) {
                if (AudioSource* a = m_sceneCR->GetComponent<AudioSource>(sel)) { a->is3D = v; }
            });
        m_grid->AddFloat("Min Dist", audio->minDistance,
            [this, sel](f32 v) {
                if (AudioSource* a = m_sceneCR->GetComponent<AudioSource>(sel)) { a->minDistance = v; }
            });
        m_grid->AddFloat("Max Dist", audio->maxDistance,
            [this, sel](f32 v) {
                if (AudioSource* a = m_sceneCR->GetComponent<AudioSource>(sel)) { a->maxDistance = v; }
            });
    }

    // If no components at all, show a message
    if (!xf && !tag && !rb && !mr && !cam && !light && !audio) {
        m_grid->AddLabel("No components", "This entity has no recognized components");
    }
}

} // namespace Dnd
