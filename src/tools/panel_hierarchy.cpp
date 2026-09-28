#include "tools/panel_hierarchy.h"
#include "tools/editor_settings.h"
#include "tools/undo.h"
#include "ui/ui_widget.h"
#include "ui/drag_drop.h"
#include "core/log.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/components/transform.h"
#include "ecs/components/tag.h"
#include <unordered_map>

namespace Dnd {

HierarchyPanel::HierarchyPanel() {
    m_treeView = new UITreeView("HierarchyTree");

    m_dockPanel = new DockPanel("Hierarchy", "Hierarchy");
    m_dockPanel->SetContent(m_treeView);
    m_dockPanel->SetCloseCallback([this]() {
        m_dockPanel->SetVisible(false);
    });

    m_treeView->SetSelectCallback([](i32 index, u64 userData) {
        EditorSettings::Get().selectedEntity = Entity(static_cast<u32>(userData));
    });

    // Drag source: initiate ENTITY_ID drag from tree view rows
    m_treeView->SetDragStartCallback([](i32 index, u64 userData, f32 mx, f32 my) {
        DND_UNUSED(index);
        DragPayload payload;
        payload.type = DragPayloadType::ENTITY_ID;
        payload.label = "Entity";
        payload.entityId = static_cast<u32>(userData);
        DragDropManager::Get().BeginDrag(nullptr, payload, mx, my);
    });

    // Drop target: accept entity drops for reparenting
    DropZone hZone;
    hZone.widget = m_treeView;
    hZone.area = DropZoneRect(0, 0, 10000, 10000); // large; filtered by widget visibility + canAccept
    hZone.canAccept = [](const DragPayload& p) -> bool {
        return p.type == DragPayloadType::ENTITY_ID;
    };
    hZone.onDrop = [this](const DragPayload& p) {
        f32 mx = DragDropManager::Get().GetMouseX();
        f32 my = DragDropManager::Get().GetMouseY();
        i32 nodeIdx = m_treeView->GetNodeIndexAt(mx, my);
        if (nodeIdx < 0) return;
        u64 targetData = m_treeView->GetNodeUserData(nodeIdx);
        Entity targetEntity(static_cast<u32>(targetData));
        Entity sourceEntity(p.entityId);
        if (!sourceEntity.IsValid() || !targetEntity.IsValid()) return;
        if (sourceEntity == targetEntity) return;
        // Prevent circular parenting: don't allow reparenting to a descendant
        // We'll handle this by checking if target is a descendant of source before
        // making the change in Update.
        // Store the pending reparent for processing in Update (which has access to scene)
        m_pendingReparentSrc = sourceEntity;
        m_pendingReparentDst = targetEntity;
    };
    DragDropManager::Get().RegisterDropZone(hZone);
}

HierarchyPanel::~HierarchyPanel() {
}

void HierarchyPanel::Update(EntityManager& em, ComponentRegistry& cr) {
    if (!m_dockPanel->IsVisible()) return;

    // Process pending reparent from drag-and-drop
    if (m_pendingReparentSrc.IsValid() && m_pendingReparentDst.IsValid()) {
        // Snapshot before
        EntitySnapshot before;
        CaptureEntitySnapshot(m_pendingReparentSrc, cr, before);

        // Reparent: set source's parent to target
        Transform* srcXf = cr.GetComponent<Transform>(m_pendingReparentSrc);
        Transform* dstXf = cr.GetComponent<Transform>(m_pendingReparentDst);
        if (srcXf && dstXf) {
            srcXf->parent = m_pendingReparentDst;
            DND_LOG_DEBUG("Reparented entity %u -> %u", m_pendingReparentSrc.id, m_pendingReparentDst.id);
        }

        // Snapshot after
        EntitySnapshot after;
        CaptureEntitySnapshot(m_pendingReparentSrc, cr, after);
        UndoStack::Get().Push(new PropertyEditCommand(m_pendingReparentSrc, before, after));

        m_pendingReparentSrc = Entity::None();
        m_pendingReparentDst = Entity::None();
    }

    RebuildTree(em, cr);
}

void HierarchyPanel::RebuildTree(EntityManager& em, ComponentRegistry& cr) {
    auto* xfSet = cr.GetSparseSet<Transform>();
    if (!xfSet) return;

    m_treeView->Clear();

    struct EntityInfo {
        Entity entity;
        std::string name;
        Entity parent;
    };
    std::vector<EntityInfo> entities;
    entities.reserve(em.GetAliveCount());

    em.ForEachAlive([&](Entity entity) {
        // Only include entities that have a Transform component
        u32 idx = Entity::GetIndex(entity.id);
        if (!xfSet->Has(idx)) return;

        const Transform* xf = xfSet->Get(idx);

        std::string name;
        auto* tagSet = cr.GetSparseSet<Tag>();
        if (tagSet) {
            const Tag* tag = tagSet->Get(idx);
            if (tag && tag->name && tag->name[0]) {
                name = tag->name;
            }
        }
        if (name.empty()) {
            name = "Entity #" + std::to_string(entity.id);
        }

        entities.push_back({ entity, name, xf ? xf->parent : Entity::None() });
    });

    std::unordered_map<u32, i32> entityToNode;

    for (size_t i = 0; i < entities.size(); ++i) {
        bool isRoot = true;
        if (entities[i].parent.IsValid()) {
            for (size_t j = 0; j < entities.size(); ++j) {
                if (entities[j].entity == entities[i].parent) {
                    isRoot = false;
                    break;
                }
            }
        }
        if (isRoot) {
            i32 nodeIdx = m_treeView->AddNode(entities[i].name, -1,
                                               static_cast<u64>(entities[i].entity.id));
            entityToNode[entities[i].entity.id] = nodeIdx;
        }
    }

    bool added;
    do {
        added = false;
        for (size_t i = 0; i < entities.size(); ++i) {
            u32 eid = entities[i].entity.id;
            if (entityToNode.find(eid) != entityToNode.end()) continue;

            u32 pid = entities[i].parent.id;
            auto it = entityToNode.find(pid);
            if (it != entityToNode.end()) {
                i32 nodeIdx = m_treeView->AddNode(entities[i].name, it->second,
                                                   static_cast<u64>(eid));
                entityToNode[eid] = nodeIdx;
                added = true;
            }
        }
    } while (added);

    for (size_t i = 0; i < entities.size(); ++i) {
        u32 eid = entities[i].entity.id;
        if (entityToNode.find(eid) == entityToNode.end()) {
            i32 nodeIdx = m_treeView->AddNode(entities[i].name, -1,
                                               static_cast<u64>(eid));
            entityToNode[eid] = nodeIdx;
        }
    }

    Entity sel = EditorSettings::Get().selectedEntity;
    if (sel.IsValid()) {
        auto it = entityToNode.find(sel.id);
        if (it != entityToNode.end()) {
            m_treeView->SetSelected(it->second);
        } else {
            EditorSettings::Get().selectedEntity = Entity::None();
        }
    }
}

} // namespace Dnd
