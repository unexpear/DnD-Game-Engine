/**
 * @file campaign_ruleset.cpp
 * @brief Compiled-in ruleset modules. Bind writes a session struct only.
 *
 * See campaign_ruleset.h. There is no disk loader for ruleset code.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/campaign_ruleset.h"

namespace Dnd {

b8 IsKnownRuleset(const std::string& moduleId) {
    return moduleId == "dnd5e" || moduleId == "tabletop-generic";
}

b8 BindRuleset(const CampaignManifest& manifest, RulesetBinding& out, std::string& error) {
    if (!IsKnownRuleset(manifest.ruleset)) {
        error = "unknown ruleset module: " + manifest.ruleset;
        return false;
    }

    RulesetBinding binding;
    binding.moduleId = manifest.ruleset;
    if (manifest.ruleset == "dnd5e") {
        binding.dice = "d20";
        binding.checkModel = "ability-check";
        binding.usesEngineCombat = true;
    } else {
        binding.dice = "d20";
        binding.checkModel = "narrative-table";
        binding.usesEngineCombat = false;
    }
    out = std::move(binding);
    return true;
}

} // namespace Dnd
