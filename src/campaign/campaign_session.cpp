/**
 * @file campaign_session.cpp
 * @brief Replaces the active campaign as a whole, or not at all.
 *
 * See campaign_session.h.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/campaign_session.h"

namespace Dnd {

b8 CampaignSession::Activate(const CampaignRegistry& registry,
                             const std::string& id,
                             std::string& error) {
    const CampaignManifest* manifest = registry.Find(id);
    if (manifest == nullptr) {
        error = "unknown campaign: " + id;
        return false;
    }

    RulesetBinding rules;
    if (!BindRuleset(*manifest, rules, error)) {
        return false;
    }

    m_manifest = *manifest;
    m_rules = std::move(rules);
    m_activeId = manifest->id;
    m_active = true;
    return true;
}

void CampaignSession::ResetToBase() {
    m_active = false;
    m_activeId.clear();
    m_manifest = CampaignManifest();
    m_rules = RulesetBinding();
}

} // namespace Dnd
