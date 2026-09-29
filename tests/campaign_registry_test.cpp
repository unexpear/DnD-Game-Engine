/**
 * @file campaign_registry_test.cpp
 * @brief Headless checks for campaign discovery and single-pack isolation.
 *
 * No window and no OpenGL. Confirms every shipped pack registers, that a
 * pack cannot escape its directory, and that the next pack replaces the
 * previous one. One session holds one active id.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/campaign_registry.h"
#include "campaign/campaign_session.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

i32 g_failures = 0;

void Expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        g_failures += 1;
    }
}

void WriteFile(const std::filesystem::path& path, const std::string& body) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << body;
}

const Dnd::CampaignManifest* FindId(const Dnd::CampaignRegistry& registry, const std::string& id) {
    return registry.Find(id);
}

bool HasTag(const Dnd::CampaignManifest& manifest, const std::string& tag) {
    for (const std::string& entry : manifest.genreTags) {
        if (entry == tag) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    const std::vector<std::string> kExpected = {
        "dead-center",
        "dnd",
        "dwarf-fortress-runescape",
        "holloday-factory",
        "hunting",
        "icarus-like-survival",
        "life-sim-bench",
        "ofa-origami-factory",
        "pirate-games",
        "rank-wars",
        "rpg-hopefully",
        "rpggame",
        "sandhold",
        "sectorbound",
        "stealth-hope",
        "superhero-vigilante",
    };

    Dnd::CampaignRegistry shipped;
    const std::string sourceRoot = std::string(DND_SOURCE_DIR) + "/Campaigns";
    const Dnd::CampaignScanReport shippedReport = shipped.ScanDirectory(sourceRoot);
    Expect(shippedReport.rejected == 0, "shipped packs should all parse");
    for (const std::string& message : shippedReport.messages) {
        std::cerr << "scan: " << message << "\n";
    }
    Expect(shipped.All().size() == kExpected.size(), "shipped pack count");
    for (const std::string& id : kExpected) {
        const Dnd::CampaignManifest* pack = FindId(shipped, id);
        Expect(pack != nullptr, "missing shipped pack " + id);
        if (pack == nullptr) {
            continue;
        }
        Expect(pack->id == id, id + " id matches directory");
        Expect(!pack->title.empty(), id + " has a title");
        Expect(pack->resolvedAssetsRoot.find(pack->packDirectory) == 0,
               id + " assets stay inside the pack");
        if (id == "dnd") {
            Expect(pack->entryScene == "dnd_demo", "dnd entry scene stays the demo");
            Expect(pack->ruleset == "dnd5e", "dnd ruleset stays dnd5e");
            Expect(pack->controlScheme.empty() || pack->controlScheme == "base",
                   "dnd keeps base controls");
            Expect(!pack->pinnedBase, "honest dnd manifest is not rewritten");
        } else if (id == "rpg-hopefully") {
            Expect(pack->entryScene == "rpg_hopefully", "sample pack has its own scene");
            Expect(pack->controlScheme == "rpg-hopefully", "sample pack names its controls");
            Expect(pack->ruleset == "tabletop-generic", "sample pack ruleset");
            Expect(!pack->tableTokens.empty(), "sample pack still describes its figures");
        } else if (id == "sectorbound") {
            Expect(pack->entryScene == "sectorbound", "sectorbound has its own scene");
            Expect(pack->controlScheme == "sectorbound", "sectorbound names its controls");
            Expect(pack->ruleset == "tabletop-generic", "sectorbound ruleset");
            Expect(!pack->tableTokens.empty(), "sectorbound still describes its tokens");
        } else if (id == "sandhold") {
            Expect(pack->entryScene == "sandhold", "sandhold has its own scene");
            Expect(pack->controlScheme == "sandhold", "sandhold names its controls");
            Expect(pack->ruleset == "tabletop-generic", "sandhold ruleset");
            Expect(!pack->tableTokens.empty(), "sandhold still describes its tokens");
            Expect(HasTag(*pack, "wasteland-squad"), "sandhold keeps its squad tag");
        } else {
            Expect(pack->entryScene == "campaign_table", id + " uses the shared table scene");
            Expect(pack->ruleset == "tabletop-generic", id + " uses the generic ruleset");
            Expect(!pack->tableTokens.empty(), id + " places at least one token");
        }
    }

    const Dnd::CampaignManifest* hopeful = FindId(shipped, "rpg-hopefully");
    Expect(hopeful != nullptr && hopeful->tableTokens.size() >= 3,
           "rpg-hopefully is the loadable sample pack");

    Dnd::CampaignSession session;
    std::string error;
    Expect(session.Activate(shipped, "rpg-hopefully", error), "activate rpg-hopefully: " + error);
    Expect(session.ActiveId() == "rpg-hopefully", "session id is the sample pack");
    Expect(session.Rules().moduleId == "tabletop-generic", "sample ruleset is session-local");
    Expect(!session.Rules().usesEngineCombat, "sample pack does not claim engine combat");
    Expect(HasTag(session.Manifest(), "class-cousin"), "sample pack keeps its own tags");
    const std::string sampleSummary = session.Manifest().summary;

    Expect(session.Activate(shipped, "sandhold", error), "activate sandhold: " + error);
    Expect(session.ActiveId() == "sandhold", "session replaced the previous id");
    Expect(!HasTag(session.Manifest(), "class-cousin"), "previous pack tags do not carry over");
    Expect(session.Manifest().summary != sampleSummary, "previous summary does not carry over");
    Expect(session.Rules().usesEngineCombat == false, "sandhold stays off engine combat");

    Expect(session.Activate(shipped, "dnd", error), "activate dnd: " + error);
    Expect(session.Rules().usesEngineCombat, "dnd session opts into engine combat");
    Expect(session.Rules().moduleId == "dnd5e", "dnd ruleset binding");
    Expect(session.Manifest().entryScene == "dnd_demo", "dnd session scene is the demo");
    Expect(!HasTag(session.Manifest(), "wasteland-squad"), "sandhold tags are gone after switching to dnd");

    const std::string dndTitle = session.Manifest().title;
    Expect(!session.Activate(shipped, "not-a-pack", error), "unknown id is refused");
    Expect(session.ActiveId() == "dnd", "failed activate leaves the current pack");
    Expect(session.Manifest().title == dndTitle, "failed activate does not clear the title");

    session.ResetToBase();
    Expect(!session.IsActive(), "reset clears the active flag");
    Expect(session.ActiveId().empty(), "reset clears the id");
    Expect(!session.Rules().usesEngineCombat, "reset drops the combat flag");
    Expect(session.Manifest().genreTags.empty(), "reset drops pack content");

    const std::filesystem::path temp =
        std::filesystem::temp_directory_path() / "dnd-campaign-registry-test";
    std::filesystem::remove_all(temp);
    const std::filesystem::path rootA = temp / "a";
    const std::filesystem::path rootB = temp / "b";

    WriteFile(rootA / "escape" / "campaign.json", R"({
        "format": "dndcampaign",
        "formatVersion": 1,
        "id": "escape",
        "title": "Escape",
        "ruleset": "tabletop-generic",
        "assetsRoot": "../secret",
        "entryScene": "campaign_table"
    })");
    Dnd::CampaignManifest rejected;
    Expect(!Dnd::LoadCampaignManifestFile((rootA / "escape" / "campaign.json").string(), rejected, error),
           "parent traversal in assetsRoot is rejected");

    WriteFile(rootA / "script-escape" / "campaign.json", R"({
        "format": "dndcampaign",
        "formatVersion": 1,
        "id": "script-escape",
        "title": "Script Escape",
        "ruleset": "tabletop-generic",
        "assetsRoot": ".",
        "entryScene": "campaign_table",
        "entryScript": "../../src/main.cpp"
    })");
    Expect(!Dnd::LoadCampaignManifestFile((rootA / "script-escape" / "campaign.json").string(), rejected, error),
           "parent traversal in entryScript is rejected");

    WriteFile(rootA / "absolute" / "campaign.json", R"({
        "format": "dndcampaign",
        "formatVersion": 1,
        "id": "absolute",
        "title": "Absolute",
        "ruleset": "tabletop-generic",
        "assetsRoot": "/tmp",
        "entryScene": "campaign_table"
    })");
    Expect(!Dnd::LoadCampaignManifestFile((rootA / "absolute" / "campaign.json").string(), rejected, error),
           "absolute assetsRoot is rejected");

    WriteFile(rootA / "wrong-scene" / "campaign.json", R"({
        "format": "dndcampaign",
        "formatVersion": 1,
        "id": "wrong-scene",
        "title": "Wrong Scene",
        "ruleset": "tabletop-generic",
        "assetsRoot": ".",
        "entryScene": "bevy_port"
    })");
    Expect(!Dnd::LoadCampaignManifestFile((rootA / "wrong-scene" / "campaign.json").string(), rejected, error),
           "unknown entry scene is rejected");

    WriteFile(rootA / "dnd" / "campaign.json", R"({
        "format": "dndcampaign",
        "formatVersion": 1,
        "id": "dnd",
        "title": "Replaced Demo",
        "ruleset": "tabletop-generic",
        "assetsRoot": ".",
        "entryScene": "campaign_table"
    })");
    Dnd::CampaignRegistry pinned;
    const Dnd::CampaignScanReport pinReport = pinned.ScanDirectory(rootA.string());
    Expect(pinReport.rejected >= 4, "unsafe packs were rejected");
    const Dnd::CampaignManifest* pinnedDnd = pinned.Find("dnd");
    Expect(pinnedDnd != nullptr, "pinned dnd still registers");
    if (pinnedDnd != nullptr) {
        Expect(pinnedDnd->entryScene == "dnd_demo", "dnd id is pinned to the demo scene");
        Expect(pinnedDnd->ruleset == "dnd5e", "dnd id is pinned to dnd5e");
        Expect(pinnedDnd->pinnedBase, "pin records that the file was rewritten");
        Expect(pinnedDnd->title == "Replaced Demo", "pin keeps the pack title");
    }

    WriteFile(rootB / "dnd" / "campaign.json", R"({
        "format": "dndcampaign",
        "formatVersion": 1,
        "id": "dnd",
        "title": "Other Root",
        "ruleset": "dnd5e",
        "assetsRoot": ".",
        "entryScene": "dnd_demo"
    })");
    const Dnd::CampaignScanReport second = pinned.ScanDirectory(rootB.string());
    Expect(second.skippedDuplicate == 1, "a second dnd pack does not replace the first");
    Expect(pinned.Find("dnd") != nullptr && pinned.Find("dnd")->title == "Replaced Demo",
           "first registered dnd pack stands");

    pinned.Clear();
    pinned.EnsureBuiltinDnd();
    const Dnd::CampaignManifest* builtin = pinned.Find("dnd");
    Expect(builtin != nullptr && builtin->builtin, "builtin dnd exists with no disk pack");
    Expect(builtin != nullptr && builtin->entryScene == "dnd_demo", "builtin launches the demo");

    std::filesystem::remove_all(temp);

    if (g_failures != 0) {
        std::cerr << g_failures << " failure(s)\n";
        return 1;
    }
    std::cout << "campaign registry tests passed (" << kExpected.size() << " packs)\n";
    return 0;
}
