#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include <functional>

namespace Dnd {

class DockPanel;
class UITreeView;
class EntityManager;
class ComponentRegistry;

class HierarchyPanel {
public:
    HierarchyPanel();
    ~HierarchyPanel();

    void Update(EntityManager& em, ComponentRegistry& cr);

    DockPanel* GetDockPanel() const { return m_dockPanel; }
    UITreeView* GetTreeView() const { return m_treeView; }

private:
    void RebuildTree(EntityManager& em, ComponentRegistry& cr);

    DockPanel* m_dockPanel;
    UITreeView* m_treeView;
    Entity m_pendingReparentSrc = Entity::None();
    Entity m_pendingReparentDst = Entity::None();
};

} // namespace Dnd
