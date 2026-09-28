/**
 * @file ai_brain.h
 * @brief Bridge to a local LLM (Ollama) that acts as the AI Dungeon Master.
 *
 * The player describes an intent in natural language; the LLM returns a small
 * structured action (what kind of check, which ability/skill, a DC, and a line
 * of narration) that the engine executes deterministically.
 *
 * Transport: the engine shells out to curl.exe (built into Windows) to POST to
 * the local Ollama server at http://localhost:11434 — no API key, no cloud, it
 * runs entirely on this machine. The call runs as a background process that the
 * main loop polls each frame (Poll), so the game never freezes waiting on it.
 *
 * @author DND Engine Team
 * @date 2026-06-03
 * @version 0.11.0
 */
#pragma once

#include "core/types.h"
#include <string>

namespace Dnd {

/**
 * @struct AiAction
 * @brief A structured action parsed from the AI DM's reply.
 */
struct AiAction {
    std::string action;     ///< pick_lock | attack | search | persuade | sneak | move | other
    std::string target;     ///< Short target name (or "none").
    std::string ability;    ///< STR | DEX | CON | INT | WIS | CHA
    std::string skill;      ///< Relevant skill or tool (e.g. "Thieves' Tools").
    std::string narration;  ///< One-sentence flavor line from the DM.
    i32 dc = 10;            ///< Difficulty class the DM chose.
    b8 ok = false;         ///< True if the reply parsed into a usable action.
    std::string error;      ///< Populated when ok == false.
    std::string raw;        ///< The raw model reply (for debugging).
};

/**
 * @class AiBrain
 * @brief Static, fire-and-poll bridge to the local LLM DM.
 */
class AiBrain {
public:
    /**
     * @brief Starts an async request to the local LLM. Non-blocking.
     * @param systemPrompt Instructions + scene context for the DM.
     * @param userText The player's natural-language intent.
     * @param model Ollama model tag (default: a local 8B instruct model).
     * @return true if the request was launched; false if one is already running
     *         or the request could not be started.
     */
    static b8 Submit(const std::string& systemPrompt, const std::string& userText,
                     const char* model = "llama3.1:8b");

    /// @brief True while a request is in flight.
    static b8 IsBusy();

    /**
     * @brief Poll for a finished request. Call once per frame.
     * @param out Filled with the parsed action when a result is ready.
     * @return true exactly once, on the frame the result becomes available.
     */
    static b8 Poll(AiAction& out);

    /// @brief Tears down any in-flight request (call on shutdown).
    static void Shutdown();

private:
    AiBrain() = default;
};

} // namespace Dnd
