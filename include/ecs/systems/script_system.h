/**
 * @file script_system.h
 * @brief ECS system that manages external script lifecycle and communication.
 *
 * Launches child processes (Python/Node.js), communicates via JSON on
 * stdin/stdout, and dispatches events between the engine and scripts.
 */
#pragma once

#include "ecs/system.h"
#include "core/types.h"
#include "ecs/component.h"
#include "ecs/components/script.h"
#include <string>

namespace Dnd {

class EntityManager;

/**
 * @class ScriptSystem
 * @brief ECS system for managing external script processes.
 *
 * Processes entities with ScriptComponent. Handles process lifecycle
 * (launch, poll, kill) and JSON message I/O.
 *
 * Registered in the UPDATE phase.
 */
class ScriptSystem : public System {
public:
    ScriptSystem();
    ~ScriptSystem() override;

    void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& registry) override;

    /**
     * @brief Starts a script process for the given entity.
     * @param registry Component registry containing the entity.
     * @param entityId Index of the entity.
     * @return true if the process was launched successfully.
     */
    b8 StartScript(ComponentRegistry& registry, u32 entityId);

    /**
     * @brief Stops (kills) a script process for the given entity.
     * @param registry Component registry containing the entity.
     * @param entityId Index of the entity.
     */
    void StopScript(ComponentRegistry& registry, u32 entityId);

    /**
     * @brief Sends a JSON message to a script's stdin.
     * @param script Pointer to the ScriptComponent.
     * @param message The JSON string to send.
     */
    static void ScriptSendMessage(ScriptComponent* script, const std::string& message);

    /**
     * @brief Reads pending JSON responses from a script's stdout.
     * @param script Pointer to the ScriptComponent.
     * @return A string containing all complete lines read (newline-separated).
     */
    static std::string ReadResponse(ScriptComponent* script);

    /**
     * @brief Notifies all scripts on an entity of an event via JSON.
     * @param registry Component registry.
     * @param entity The entity to notify.
     * @param eventJson The JSON event string (without newline).
     */
    static void NotifyEvent(ComponentRegistry& registry, Entity entity, const std::string& eventJson);

    /**
     * @brief Stops all running scripts.
     * @param registry Component registry.
     */
    void StopAll(ComponentRegistry& registry);

private:
    /** @brief Interval (in frames) between stdout polls. */
    static constexpr i32 POLL_INTERVAL = 5;

    /**
     * @brief Builds the command line to launch a script.
     * @param script The ScriptComponent.
     * @return Command string (e.g., "python script.py").
     */
    static std::string BuildCommand(const ScriptComponent& script);
};

} // namespace Dnd
