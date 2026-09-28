/**
 * @file script.h
 * @brief ScriptComponent for attaching external scripts to entities.
 *
 * Allows entities to be driven by external scripting languages
 * (Python, Node.js) via a shell-out JSON stdin/stdout protocol.
 */
#pragma once

#include "core/types.h"
#include <string>

namespace Dnd {

/**
 * @enum ScriptLanguage
 * @brief Supported scripting languages.
 */
enum class ScriptLanguage : i32 {
    None   = 0,
    Python = 1,
    NodeJS = 2
};

/**
 * @struct ScriptComponent
 * @brief ECS component that attaches an external script to an entity.
 *
 * The script is launched as a child process and communicates via JSON
 * on stdin/stdout. Messages flow both directions (engine -> script and
 * script -> engine).
 */
struct ScriptComponent {
    /** @brief Path to the script file (relative to project root or absolute). */
    char scriptPath[256];
    /** @brief Language/runner to use (Python or Node.js). */
    ScriptLanguage language = ScriptLanguage::None;
    /** @brief Whether the script should auto-start when the entity loads. */
    b8 autoStart = true;
    /** @brief Whether the script is currently running. */
    b8 isRunning = false;
    /** @brief Whether AI DM mode is active (polling for responses). */
    b8 awaitingResponse = false;
    /** @brief Pipe write handle (stdin to child). */
    void* pipeWrite = nullptr;
    /** @brief Pipe read handle (stdout from child). */
    void* pipeRead = nullptr;
    /** @brief Process handle for the child. */
    void* processHandle = nullptr;
    /** @brief Buffer for partial line reads from child stdout. */
    char readBuffer[4096];
    /** @brief Number of bytes in readBuffer. */
    i32 readLen = 0;
    /** @brief Frame counter for polling interval. */
    i32 pollCounter = 0;
};

} // namespace Dnd
