/**
 * @file campaign_ruleset.h
 * @brief Session-local binding for a compiled-in ruleset module.
 *
 * The one active pack names a ruleset by id. The catalog binds that id
 * into a RulesetBinding owned by that pack's session. Bind never writes
 * dice tables, combat globals, or any other engine singleton. A pack
 * cannot register a new module from disk; unknown ids fail the bind.
 * There is one binding, replaced on the next successful pick.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#pragma once

#include "campaign/campaign_manifest.h"
#include "core/types.h"
#include <string>

namespace Dnd {

/**
 * @struct RulesetBinding
 * @brief Rules knobs that belong to the active campaign only.
 *
 * These fields describe how the table should talk about rolls. They are
 * not copied into the engine's dice or combat systems.
 */
struct RulesetBinding {
    std::string moduleId;
    std::string dice;          ///< Descriptive die, e.g. "d20".
    std::string checkModel;    ///< Descriptive check model. Not a function pointer.
    b8 usesEngineCombat = false; ///< True only for the built-in dnd5e module.
};

/**
 * @brief Whether @p moduleId is a compiled-in ruleset module.
 */
b8 IsKnownRuleset(const std::string& moduleId);

/**
 * @brief Fills @p out from the manifest's ruleset id.
 *
 * On failure @p out is left untouched.
 * @param manifest Pack whose ruleset field is bound.
 * @param out Session-local binding.
 * @param error Reason when the module id is unknown.
 * @return true if the module is known.
 */
b8 BindRuleset(const CampaignManifest& manifest, RulesetBinding& out, std::string& error);

} // namespace Dnd
