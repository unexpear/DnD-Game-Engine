#include "tools/undo.h"
#include "ecs/components/transform.h"
#include "core/log.h"

namespace Dnd {

UndoStack* UndoStack::s_instance = nullptr;

// ==========================================
// TransformCommand
// ==========================================

TransformCommand::TransformCommand(Entity entity,
    const Vec3& beforePos, const Quat& beforeRot, const Vec3& beforeScale,
    const Vec3& afterPos, const Quat& afterRot, const Vec3& afterScale)
    : m_entity(entity)
    , m_beforePos(beforePos), m_afterPos(afterPos)
    , m_beforeRot(beforeRot), m_afterRot(afterRot)
    , m_beforeScale(beforeScale), m_afterScale(afterScale) {}

void TransformCommand::Undo(ComponentRegistry& registry, EntityManager&) {
    Transform* xf = registry.GetComponent<Transform>(m_entity);
    if (xf) {
        xf->position = m_beforePos;
        xf->rotation = m_beforeRot;
        xf->scale = m_beforeScale;
    }
}

void TransformCommand::Redo(ComponentRegistry& registry, EntityManager&) {
    Transform* xf = registry.GetComponent<Transform>(m_entity);
    if (xf) {
        xf->position = m_afterPos;
        xf->rotation = m_afterRot;
        xf->scale = m_afterScale;
    }
}

// ==========================================
// Entity snapshot helpers
// ==========================================

void CaptureEntitySnapshot(Entity entity, ComponentRegistry& registry, EntitySnapshot& out) {
    out.entity = entity;
    if (Transform* c = registry.GetComponent<Transform>(entity)) {
        out.hasTransform = true; out.transform = *c;
    }
    if (Tag* c = registry.GetComponent<Tag>(entity)) {
        out.hasTag = true;
        out.tag = *c;
    }
    if (MeshRenderer* c = registry.GetComponent<MeshRenderer>(entity)) {
        out.hasMeshRenderer = true; out.meshRenderer = *c;
    }
    if (RigidBody* c = registry.GetComponent<RigidBody>(entity)) {
        out.hasRigidBody = true; out.rigidBody = *c;
    }
    if (ECSCamera* c = registry.GetComponent<ECSCamera>(entity)) {
        out.hasCamera = true; out.camera = *c;
    }
    if (Light* c = registry.GetComponent<Light>(entity)) {
        out.hasLight = true; out.light = *c;
    }
    if (AudioSource* c = registry.GetComponent<AudioSource>(entity)) {
        out.hasAudioSource = true; out.audioSource = *c;
    }
}

void ApplyEntitySnapshot(Entity entity, ComponentRegistry& registry, const EntitySnapshot& snap) {
    if (snap.hasTransform) registry.AddComponent<Transform>(entity, snap.transform);
    if (snap.hasTag) {
        registry.AddComponent<Tag>(entity, snap.tag);
    }
    if (snap.hasMeshRenderer) registry.AddComponent<MeshRenderer>(entity, snap.meshRenderer);
    if (snap.hasRigidBody) registry.AddComponent<RigidBody>(entity, snap.rigidBody);
    if (snap.hasCamera) registry.AddComponent<ECSCamera>(entity, snap.camera);
    if (snap.hasLight) registry.AddComponent<Light>(entity, snap.light);
    if (snap.hasAudioSource) registry.AddComponent<AudioSource>(entity, snap.audioSource);
}

// ==========================================
// DeleteEntityCommand
// ==========================================

DeleteEntityCommand::DeleteEntityCommand(Entity entity, ComponentRegistry& registry)
    : m_index(Entity::GetIndex(entity.id))
    , m_generation(Entity::GetGeneration(entity.id))
{
    CaptureEntitySnapshot(entity, registry, m_snapshot);
}

void DeleteEntityCommand::Undo(ComponentRegistry& registry, EntityManager& entities) {
    // Restore the entity slot to life with its original generation
    m_snapshot.entity = entities.Restore(m_index, m_generation);
    ApplyEntitySnapshot(m_snapshot.entity, registry, m_snapshot);
}

void DeleteEntityCommand::Redo(ComponentRegistry& registry, EntityManager& entities) {
    entities.Destroy(m_snapshot.entity);
}

// ==========================================
// CreateEntityCommand
// ==========================================

CreateEntityCommand::CreateEntityCommand(Entity entity, ComponentRegistry& registry)
    : m_entity(entity)
{
    CaptureEntitySnapshot(entity, registry, m_snapshot);
}

void CreateEntityCommand::Undo(ComponentRegistry& registry, EntityManager& entities) {
    entities.Destroy(m_entity);
}

void CreateEntityCommand::Redo(ComponentRegistry& registry, EntityManager& entities) {
    // Re-create the entity at the same index/generation if possible
    m_entity = entities.Restore(Entity::GetIndex(m_entity.id), Entity::GetGeneration(m_entity.id));
    ApplyEntitySnapshot(m_entity, registry, m_snapshot);
}

// ==========================================
// PropertyEditCommand
// ==========================================

PropertyEditCommand::PropertyEditCommand(Entity entity, const EntitySnapshot& before,
                                         const EntitySnapshot& after)
    : m_entity(entity), m_before(before), m_after(after) {}

void PropertyEditCommand::Undo(ComponentRegistry& registry, EntityManager&) {
    ApplyEntitySnapshot(m_entity, registry, m_before);
}

void PropertyEditCommand::Redo(ComponentRegistry& registry, EntityManager&) {
    ApplyEntitySnapshot(m_entity, registry, m_after);
}

// ==========================================
// UndoStack
// ==========================================

UndoStack& UndoStack::Get() {
    if (!s_instance) {
        s_instance = new UndoStack();
    }
    return *s_instance;
}

UndoStack::~UndoStack() {
    for (UndoCommand* cmd : m_undoStack) delete cmd;
    for (UndoCommand* cmd : m_redoStack) delete cmd;
    m_undoStack.clear();
    m_redoStack.clear();
}

void UndoStack::Push(UndoCommand* cmd) {
    for (UndoCommand* c : m_redoStack) delete c;
    m_redoStack.clear();

    if (static_cast<i32>(m_undoStack.size()) >= kMaxDepth) {
        delete m_undoStack.front();
        m_undoStack.erase(m_undoStack.begin());
    }
    m_undoStack.push_back(cmd);
}

b8 UndoStack::Undo(ComponentRegistry& registry, EntityManager& entities) {
    if (m_undoStack.empty()) return false;
    UndoCommand* cmd = m_undoStack.back();
    m_undoStack.pop_back();
    cmd->Undo(registry, entities);
    m_redoStack.push_back(cmd);
    return true;
}

b8 UndoStack::Redo(ComponentRegistry& registry, EntityManager& entities) {
    if (m_redoStack.empty()) return false;
    UndoCommand* cmd = m_redoStack.back();
    m_redoStack.pop_back();
    cmd->Redo(registry, entities);
    m_undoStack.push_back(cmd);
    return true;
}

void UndoStack::Clear() {
    for (UndoCommand* cmd : m_undoStack) delete cmd;
    for (UndoCommand* cmd : m_redoStack) delete cmd;
    m_undoStack.clear();
    m_redoStack.clear();
}

} // namespace Dnd