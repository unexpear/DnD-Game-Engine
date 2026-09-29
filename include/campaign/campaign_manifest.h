/**
 * @file campaign_manifest.h
 * @brief On-disk campaign pack manifest (campaign.json).
 *
 * A campaign pack is a directory under Campaigns/<id>/ with a campaign.json
 * manifest. The manifest names the pack, the compiled-in ruleset module it
 * may bind, and the entry scene the table loads. It does not contain code.
 *
 * Paths inside a manifest are relative to the pack directory. The parser
 * rejects absolute paths and parent-directory traversal so a pack cannot
 * point the loader at engine defaults or at another pack.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#pragma once

#include "core/types.h"
#include <string>
#include <vector>

namespace Dnd {

/**
 * @struct TableToken
 * @brief One placeholder miniature on a non-D&D campaign table.
 *
 * Color channels are linear RGBA-less RGB in [0, 1]. Position is world
 * space on the shared table (Y up).
 */
struct TableToken {
    std::string name;
    f32 r = 0.8f;
    f32 g = 0.8f;
    f32 b = 0.8f;
    f32 x = 0.0f;
    f32 y = 0.5f;
    f32 z = 0.0f;
};

/**
 * @struct CampaignManifest
 * @brief Parsed, validated description of one campaign pack.
 *
 * Copies of this struct are the only campaign data the session keeps.
 * Loading a different pack replaces the copy. Nothing here is written
 * into engine-global rules or asset tables.
 */
struct CampaignManifest {
    std::string format;             ///< Must be "dndcampaign" when present.
    i32 formatVersion = 1;          ///< Supported version is 1.
    std::string id;                 ///< Stable id. Must match the pack directory name.
    std::string title;              ///< Human title shown in the picker.
    std::vector<std::string> genreTags;
    std::string ruleset;            ///< Compiled-in ruleset module id.
    std::string assetsRoot;         ///< Relative asset root as written in the file.
    std::string resolvedAssetsRoot; ///< Absolute asset root, confined to the pack.
    std::string entryScene;         ///< "dnd_demo", "campaign_table", or "rpg_hopefully".
    std::string entryScript;        ///< Optional relative script path. Not executed.
    std::string controlScheme;      ///< "base" or a pack id. Empty means base.
    std::string summary;
    std::string sourceNote;         ///< Where the original game lives, for authors.
    std::string packDirectory;      ///< Absolute directory that owns this manifest.
    std::vector<TableToken> tableTokens;
    b8 builtin = false;             ///< True for the in-code D&D fallback.
    b8 pinnedBase = false;          ///< True when the dnd id was forced back to the demo.
};

/**
 * @brief Entry scenes the loader is willing to construct.
 *
 * Packs cannot name an arbitrary class. Unknown ids are rejected at
 * registration so a manifest cannot pull in other code.
 */
b8 IsAllowedEntryScene(const std::string& entryScene);

/**
 * @brief Parses a campaign.json file.
 *
 * On failure, @p out is left untouched and @p error explains why.
 * @param campaignJsonPath Path to campaign.json.
 * @param out Receives the manifest on success.
 * @param error Receives a reason on failure.
 * @return true if the file is a safe, complete manifest.
 */
b8 LoadCampaignManifestFile(const std::string& campaignJsonPath,
                            CampaignManifest& out,
                            std::string& error);

/**
 * @brief The in-code D&D manifest used when no Campaigns/dnd pack is readable.
 *
 * Entry scene is the existing demo. Ruleset is dnd5e. No asset override.
 */
CampaignManifest MakeBuiltinDndManifest();

} // namespace Dnd
