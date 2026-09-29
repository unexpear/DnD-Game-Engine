/**
 * @file campaign_registry.cpp
 * @brief Scans Campaigns/ and registers isolated pack manifests.
 *
 * See campaign_registry.h.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/campaign_registry.h"
#include "campaign/campaign_ruleset.h"

#include <algorithm>
#include <filesystem>

namespace Dnd {

void CampaignRegistry::Clear() {
    m_packs.clear();
}

void CampaignRegistry::SortById() {
    std::sort(m_packs.begin(), m_packs.end(),
              [](const CampaignManifest& a, const CampaignManifest& b) {
                  return a.id < b.id;
              });
}

b8 CampaignRegistry::Add(CampaignManifest manifest, std::string& error) {
    if (manifest.id == "dnd") {
        if (manifest.entryScene != "dnd_demo" || manifest.ruleset != "dnd5e") {
            manifest.entryScene = "dnd_demo";
            manifest.ruleset = "dnd5e";
            manifest.pinnedBase = true;
        }
        if (!manifest.controlScheme.empty() && manifest.controlScheme != "base") {
            manifest.controlScheme = "base";
            manifest.pinnedBase = true;
        }
    }

    if (!IsAllowedEntryScene(manifest.entryScene)) {
        error = "entry scene is not loadable: " + manifest.entryScene;
        return false;
    }
    if (!IsKnownRuleset(manifest.ruleset)) {
        error = "unknown ruleset module: " + manifest.ruleset;
        return false;
    }
    if (Find(manifest.id) != nullptr) {
        error = "pack id already registered: " + manifest.id;
        return false;
    }

    m_packs.push_back(std::move(manifest));
    SortById();
    return true;
}

CampaignScanReport CampaignRegistry::ScanDirectory(const std::string& root) {
    CampaignScanReport report;
    std::error_code ec;
    const std::filesystem::path rootPath(root);
    if (!std::filesystem::is_directory(rootPath, ec) || ec) {
        report.messages.push_back("campaign root not found: " + root);
        return report;
    }

    std::vector<std::filesystem::path> children;
    try {
        for (const std::filesystem::directory_entry& entry :
             std::filesystem::directory_iterator(rootPath, ec)) {
            if (ec) {
                report.messages.push_back("could not read " + root);
                return report;
            }
            if (entry.is_directory()) {
                children.push_back(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        report.messages.push_back(std::string("could not read ") + root + ": " + ex.what());
        return report;
    }
    std::sort(children.begin(), children.end());

    for (const std::filesystem::path& child : children) {
        const std::filesystem::path manifestPath = child / "campaign.json";
        if (!std::filesystem::is_regular_file(manifestPath, ec) || ec) {
            continue;
        }

        CampaignManifest manifest;
        std::string error;
        if (!LoadCampaignManifestFile(manifestPath.generic_string(), manifest, error)) {
            report.rejected += 1;
            report.messages.push_back(manifestPath.generic_string() + ": " + error);
            continue;
        }
        if (const CampaignManifest* existing = Find(manifest.id)) {
            if (existing->packDirectory == manifest.packDirectory) {
                continue;
            }
            report.skippedDuplicate += 1;
            continue;
        }
        if (!Add(std::move(manifest), error)) {
            report.rejected += 1;
            report.messages.push_back(manifestPath.generic_string() + ": " + error);
            continue;
        }
        report.loaded += 1;
    }
    return report;
}

void CampaignRegistry::EnsureBuiltinDnd() {
    if (Find("dnd") != nullptr) {
        return;
    }
    std::string error;
    Add(MakeBuiltinDndManifest(), error);
}

const CampaignManifest* CampaignRegistry::Find(const std::string& id) const {
    for (const CampaignManifest& pack : m_packs) {
        if (pack.id == id) {
            return &pack;
        }
    }
    return nullptr;
}

std::vector<std::string> CampaignRegistry::DefaultSearchRoots() {
    std::vector<std::string> roots;
    roots.emplace_back("Campaigns");
#ifdef DND_SOURCE_DIR
    roots.emplace_back(std::string(DND_SOURCE_DIR) + "/Campaigns");
#endif
    return roots;
}

} // namespace Dnd
