#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "tools/undo.h"
#include <string>

namespace Dnd {

class DockPanel;
class UIPropertyGrid;
class EntityManager;
class ComponentRegistry;

/**
 * @class InspectorPanel
 * @brief Editor panel that displays and edits components of the selected entity.
 *
 * Each frame (when the editor is active) the panel reads
 * EditorSettings::selectedEntity, checks which ECS components exist on
 * that entity, and populates a UIPropertyGrid with editable fields.
 *
 * Callbacks write modified values back through the ComponentRegistry.
 */
class InspectorPanel {
public:
    InspectorPanel();
    ~InspectorPanel();

    /**
     * @brief Rebuilds the property grid for the currently selected entity.
     * @param em EntityManager for entity queries.
     * @param cr ComponentRegistry for reading/writing component data.
     */
    void Update(EntityManager& em, ComponentRegistry& cr);

    /// @brief Gets the underlying DockPanel (for DockManager registration).
    DockPanel* GetDockPanel() const { return m_dockPanel; }

    /// @brief Gets the property grid widget.
    UIPropertyGrid* GetPropertyGrid() const { return m_grid; }

private:
    DockPanel*       m_dockPanel;
    UIPropertyGrid*  m_grid;
    Entity           m_lastEntity = Entity::None();
    EntitySnapshot   m_snapshot;
    std::string      m_pendingDropPath;
    ComponentRegistry* m_sceneCR = nullptr;
    EntityManager*     m_sceneEM = nullptr;
};

} // namespace Dnd
