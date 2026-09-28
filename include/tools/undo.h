#pragma once

#include "core/types.h"
#include "core/math.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/components/transform.h"
#include "ecs/components/tag.h"
#include "ecs/components/rigidbody.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/camera.h"
#include "ecs/components/light.h"
#include "audio/audio_source.h"
#include <vector>

namespace Dnd {

class UndoCommand {
public:
    virtual ~UndoCommand() = default;
    virtual void Undo(ComponentRegistry& registry, EntityManager& entities) = 0;
    virtual void Redo(ComponentRegistry& registry, EntityManager& entities) = 0;
};

class TransformCommand : public UndoCommand {
public:
    TransformCommand(Entity entity, const Vec3& beforePos, const Quat& beforeRot, const Vec3& beforeScale,
                     const Vec3& afterPos, const Quat& afterRot, const Vec3& afterScale);
    void Undo(ComponentRegistry& registry, EntityManager& entities) override;
    void Redo(ComponentRegistry& registry, EntityManager& entities) override;
private:
    Entity m_entity;
    Vec3 m_beforePos, m_afterPos;
    Quat m_beforeRot, m_afterRot;
    Vec3 m_beforeScale, m_afterScale;
};

struct EntitySnapshot {
    Entity entity;
    bool hasTransform = false; Transform transform;
    bool hasTag = false; Tag tag;
    std::string tagName;
    bool hasMeshRenderer = false; MeshRenderer meshRenderer;
    bool hasRigidBody = false; RigidBody rigidBody;
    bool hasCamera = false; ECSCamera camera;
    bool hasLight = false; Light light;
    bool hasAudioSource = false; AudioSource audioSource;
};

void CaptureEntitySnapshot(Entity entity, ComponentRegistry& registry, EntitySnapshot& out);
void ApplyEntitySnapshot(Entity entity, ComponentRegistry& registry, const EntitySnapshot& snap);

class DeleteEntityCommand : public UndoCommand {
public:
    DeleteEntityCommand(Entity entity, ComponentRegistry& registry);
    void Undo(ComponentRegistry& registry, EntityManager& entities) override;
    void Redo(ComponentRegistry& registry, EntityManager& entities) override;
private:
    EntitySnapshot m_snapshot;
    u32 m_index;
    u16 m_generation;
};

class CreateEntityCommand : public UndoCommand {
public:
    CreateEntityCommand(Entity entity, ComponentRegistry& registry);
    void Undo(ComponentRegistry& registry, EntityManager& entities) override;
    void Redo(ComponentRegistry& registry, EntityManager& entities) override;
private:
    Entity m_entity;
    EntitySnapshot m_snapshot;
};

class PropertyEditCommand : public UndoCommand {
public:
    PropertyEditCommand(Entity entity, const EntitySnapshot& before,
                        const EntitySnapshot& after);
    void Undo(ComponentRegistry& registry, EntityManager& entities) override;
    void Redo(ComponentRegistry& registry, EntityManager& entities) override;
private:
    Entity m_entity;
    EntitySnapshot m_before;
    EntitySnapshot m_after;
};

class UndoStack {
public:
    static UndoStack& Get();

    void Push(UndoCommand* cmd);
    b8 Undo(ComponentRegistry& registry, EntityManager& entities);
    b8 Redo(ComponentRegistry& registry, EntityManager& entities);
    void Clear();

private:
    UndoStack() = default;
    ~UndoStack();
    UndoStack(const UndoStack&) = delete;
    UndoStack& operator=(const UndoStack&) = delete;

    static UndoStack* s_instance;
    static constexpr i32 kMaxDepth = 50;

    std::vector<UndoCommand*> m_undoStack;
    std::vector<UndoCommand*> m_redoStack;
};

} // namespace Dnd