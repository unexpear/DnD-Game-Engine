/**
 * @file campaign_registry.h
 * @brief Discovers campaign packs and keeps them from colliding.
 *
 * The registry is a list of validated manifests. It does not load scenes
 * and it does not touch engine singletons. The first pack to claim an id
 * wins; a later pack with the same id is rejected. The canonical id "dnd"
 * is pinned to the existing demo scene and the dnd5e ruleset module.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#pragma once

#include "campaign/campaign_manifest.h"
#include "core/types.h"
#include <string>
#include <vector>

namespace Dnd {

/**
 * @struct CampaignScanReport
 * @brief What a directory scan accepted and rejected.
 */
struct CampaignScanReport {
    i32 loaded = 0;
    i32 rejected = 0;
    /** @brief Same id already registered from another directory. First one stands. */
    i32 skippedDuplicate = 0;
    std::vector<std::string> messages;
};

/**
 * @class CampaignRegistry
 * @brief In-memory catalog of campaign packs for one process.
 */
class CampaignRegistry {
public:
    /**
     * @brief Removes every registered pack.
     */
    void Clear();

    /**
     * @brief Registers a manifest that has already been parsed.
     *
     * Applies the dnd pin, rejects unknown entry scenes and duplicate ids.
     * @param manifest Pack to add. May be rewritten by the pin.
     * @param error Reason when the pack is refused.
     * @return true if the pack is now in the catalog.
     */
    b8 Add(CampaignManifest manifest, std::string& error);

    /**
     * @brief Loads Campaigns/<folder>/campaign.json children of @p root.
     *
     * Does not recurse. A missing root is a soft miss (loaded stays 0).
     * @param root Directory whose children are pack folders.
     * @return Counts and human-readable notes. Never throws.
     */
    CampaignScanReport ScanDirectory(const std::string& root);

    /**
     * @brief Inserts the built-in D&D manifest when "dnd" is not registered.
     */
    void EnsureBuiltinDnd();

    /**
     * @brief Finds a pack by id.
     * @return Pointer into the catalog, or nullptr.
     */
    const CampaignManifest* Find(const std::string& id) const;

    /**
     * @brief Packs in stable id order.
     */
    const std::vector<CampaignManifest>& All() const { return m_packs; }

    /**
     * @brief Roots the loader searches: ./Campaigns, then the source tree.
     */
    static std::vector<std::string> DefaultSearchRoots();

private:
    void SortById();
    std::vector<CampaignManifest> m_packs;
};

} // namespace Dnd
