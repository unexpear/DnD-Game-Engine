/**
 * @file campaign_session.h
 * @brief The one active campaign, held apart from engine globals.
 *
 * One campaign is active. Activate replaces that pack's manifest and
 * ruleset binding as a whole. A failed activate leaves the previous pack
 * in place. Reset drops the session back to an empty base-safe state:
 * no id, no tokens, no combat flag. Neither operation writes engine
 * singletons; the host clears leftover scene state separately, and only
 * with a fixed allowlist, when it actually swaps the scene.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#pragma once

#include "campaign/campaign_registry.h"
#include "campaign/campaign_ruleset.h"
#include "core/types.h"
#include <string>

namespace Dnd {

/**
 * @class CampaignSession
 * @brief Active-pack state for the table.
 */
class CampaignSession {
public:
    /**
     * @brief Makes @p id the active pack.
     *
     * Validates first. The previous session is replaced only after the
     * ruleset bind succeeds.
     * @param registry Catalog to resolve @p id against.
     * @param id Pack id.
     * @param error Reason on failure. Previous session is unchanged.
     * @return true if this session now holds @p id.
     */
    b8 Activate(const CampaignRegistry& registry, const std::string& id, std::string& error);

    /**
     * @brief Drops the active pack. Does not touch engine singletons.
     */
    void ResetToBase();

    /** @brief True after a successful Activate and before ResetToBase. */
    b8 IsActive() const { return m_active; }

    /** @brief Active pack id, or empty when inactive. */
    const std::string& ActiveId() const { return m_activeId; }

    /** @brief Copy of the active manifest. Empty when inactive. */
    const CampaignManifest& Manifest() const { return m_manifest; }

    /** @brief Session-local ruleset binding. */
    const RulesetBinding& Rules() const { return m_rules; }

private:
    b8 m_active = false;
    std::string m_activeId;
    CampaignManifest m_manifest;
    RulesetBinding m_rules;
};

} // namespace Dnd
