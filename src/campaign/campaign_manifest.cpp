/**
 * @file campaign_manifest.cpp
 * @brief Parses and validates campaign.json manifests.
 *
 * See campaign_manifest.h for the isolation rules this parser enforces.
 *
 * @author DND Engine Team
 * @date 2026-09-29
 * @version 0.11.0
 */

#include "campaign/campaign_manifest.h"
#include "core/json.h"

#include <cctype>
#include <filesystem>
#include <string>

namespace Dnd {

namespace {

b8 IsSafeId(const std::string& id) {
    if (id.empty()) {
        return false;
    }
    if (id.front() == '-' || id.back() == '-') {
        return false;
    }
    b8 hyphen = false;
    for (char c : id) {
        const b8 ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
        if (!ok) {
            return false;
        }
        if (c == '-') {
            if (hyphen) {
                return false;
            }
            hyphen = true;
        } else {
            hyphen = false;
        }
    }
    return true;
}

b8 IsSafeRelativePath(const std::string& path, std::string& error) {
    if (path.empty()) {
        error = "path is empty";
        return false;
    }
    if (path.front() == '/' || path.front() == '\\') {
        error = "absolute paths are not allowed";
        return false;
    }
    if (path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':') {
        error = "drive paths are not allowed";
        return false;
    }

    std::string segment;
    for (size_t i = 0; i <= path.size(); ++i) {
        const char c = (i == path.size()) ? '/' : path[i];
        if (c == '/' || c == '\\') {
            if (segment == "..") {
                error = "parent-directory traversal is not allowed";
                return false;
            }
            segment.clear();
        } else {
            segment.push_back(c);
        }
    }
    return true;
}

b8 PathIsInside(const std::filesystem::path& root, const std::filesystem::path& candidate) {
    std::error_code ec;
    const std::filesystem::path canonRoot = std::filesystem::weakly_canonical(root, ec);
    if (ec) {
        return false;
    }
    const std::filesystem::path canonChild = std::filesystem::weakly_canonical(candidate, ec);
    if (ec) {
        return false;
    }
    std::string rootStr = canonRoot.lexically_normal().generic_string();
    const std::string childStr = canonChild.lexically_normal().generic_string();
    if (childStr == rootStr) {
        return true;
    }
    if (rootStr.empty() || rootStr.back() != '/') {
        rootStr.push_back('/');
    }
    return childStr.compare(0, rootStr.size(), rootStr) == 0;
}

f32 HashChannel(const std::string& name, int salt) {
    unsigned h = 2166136261u;
    for (unsigned char c : name) {
        h ^= c;
        h *= 16777619u;
    }
    h ^= static_cast<unsigned>(salt) * 0x9E3779B9u;
    return 0.28f + static_cast<f32>(h % 100) / 150.0f;
}

b8 ReadColorChannel(const JsonValue& color, size_t index, f32 fallback, f32& out) {
    if (color.type != JsonType::Array || index >= color.arrVal.size()) {
        out = fallback;
        return true;
    }
    const JsonValue& channel = color.arrVal[index];
    if (channel.type != JsonType::Number) {
        return false;
    }
    if (channel.numVal < 0.0 || channel.numVal > 1.0) {
        return false;
    }
    out = static_cast<f32>(channel.numVal);
    return true;
}

b8 ReadTokens(const JsonValue& root, CampaignManifest& manifest, std::string& error) {
    const JsonValue* tokens = root.Get("tableTokens");
    if (!tokens || tokens->IsNull()) {
        return true;
    }
    if (tokens->type != JsonType::Array) {
        error = "tableTokens must be an array";
        return false;
    }
    constexpr size_t kMaxTokens = 8;
    const size_t count = tokens->arrVal.size() < kMaxTokens ? tokens->arrVal.size() : kMaxTokens;
    for (size_t i = 0; i < count; ++i) {
        const JsonValue& item = tokens->arrVal[i];
        if (item.type != JsonType::Object) {
            error = "tableTokens entries must be objects";
            return false;
        }
        TableToken token;
        token.name = item.GetString("name", "");
        if (token.name.empty()) {
            error = "table token is missing a name";
            return false;
        }
        const f32 fallbackR = HashChannel(token.name, 1);
        const f32 fallbackG = HashChannel(token.name, 2);
        const f32 fallbackB = HashChannel(token.name, 3);
        const JsonValue* color = item.Get("color");
        if (color && !color->IsNull()) {
            if (!ReadColorChannel(*color, 0, fallbackR, token.r) ||
                !ReadColorChannel(*color, 1, fallbackG, token.g) ||
                !ReadColorChannel(*color, 2, fallbackB, token.b)) {
                error = "table token color must be an array of numbers in [0, 1]";
                return false;
            }
        } else {
            token.r = fallbackR;
            token.g = fallbackG;
            token.b = fallbackB;
        }

        const f32 spread = (static_cast<f32>(i) - static_cast<f32>(count) * 0.5f) * 1.6f;
        token.x = spread;
        token.y = 0.45f;
        token.z = 0.6f;
        const JsonValue* position = item.Get("position");
        if (position && !position->IsNull()) {
            if (position->type != JsonType::Array || position->arrVal.size() < 3) {
                error = "table token position must be [x, y, z]";
                return false;
            }
            for (size_t axis = 0; axis < 3; ++axis) {
                if (position->arrVal[axis].type != JsonType::Number) {
                    error = "table token position components must be numbers";
                    return false;
                }
            }
            token.x = static_cast<f32>(position->arrVal[0].numVal);
            token.y = static_cast<f32>(position->arrVal[1].numVal);
            token.z = static_cast<f32>(position->arrVal[2].numVal);
        }
        manifest.tableTokens.push_back(token);
    }
    return true;
}

} // namespace

b8 IsAllowedEntryScene(const std::string& entryScene) {
    return entryScene == "dnd_demo" || entryScene == "campaign_table" ||
           entryScene == "rpg_hopefully";
}

CampaignManifest MakeBuiltinDndManifest() {
    CampaignManifest manifest;
    manifest.format = "dndcampaign";
    manifest.formatVersion = 1;
    manifest.id = "dnd";
    manifest.title = "Dungeons & Dragons";
    manifest.genreTags = {"tabletop", "fantasy", "5e"};
    manifest.ruleset = "dnd5e";
    manifest.assetsRoot = ".";
    manifest.entryScene = "dnd_demo";
    manifest.summary = "Canonical base campaign. The current explore and combat demo.";
    manifest.sourceNote = "Built into the engine. Used when Campaigns/dnd cannot be read.";
    manifest.builtin = true;
    return manifest;
}

b8 LoadCampaignManifestFile(const std::string& campaignJsonPath,
                            CampaignManifest& out,
                            std::string& error) {
    const JsonValue root = JsonParser::ParseFile(campaignJsonPath.c_str());
    if (root.IsNull() || root.type != JsonType::Object) {
        error = "could not read campaign object from " + campaignJsonPath;
        return false;
    }

    CampaignManifest manifest;
    manifest.format = root.GetString("format", "dndcampaign");
    if (manifest.format != "dndcampaign") {
        error = "format must be dndcampaign";
        return false;
    }
    manifest.formatVersion = root.GetInt("formatVersion", 1);
    if (manifest.formatVersion != 1) {
        error = "unsupported formatVersion (only 1 is readable)";
        return false;
    }

    manifest.id = root.GetString("id", "");
    if (!IsSafeId(manifest.id)) {
        error = "id must be lowercase words separated by single hyphens";
        return false;
    }
    manifest.title = root.GetString("title", "");
    if (manifest.title.empty()) {
        error = "title is required";
        return false;
    }
    manifest.ruleset = root.GetString("ruleset", "");
    if (manifest.ruleset.empty()) {
        error = "ruleset is required";
        return false;
    }
    manifest.entryScene = root.GetString("entryScene", "");
    if (!IsAllowedEntryScene(manifest.entryScene)) {
        error = "entryScene must be dnd_demo or campaign_table";
        return false;
    }

    manifest.assetsRoot = root.GetString("assetsRoot", "");
    if (!IsSafeRelativePath(manifest.assetsRoot, error)) {
        error = "assetsRoot: " + error;
        return false;
    }
    manifest.entryScript = root.GetString("entryScript", "");
    if (!manifest.entryScript.empty() && !IsSafeRelativePath(manifest.entryScript, error)) {
        error = "entryScript: " + error;
        return false;
    }
    manifest.controlScheme = root.GetString("controlScheme", "");
    if (!manifest.controlScheme.empty() && manifest.controlScheme != "base" &&
        !IsSafeId(manifest.controlScheme)) {
        error = "controlScheme must be base or a pack id";
        return false;
    }

    const JsonValue* tags = root.Get("genreTags");
    if (tags && !tags->IsNull()) {
        if (tags->type != JsonType::Array) {
            error = "genreTags must be an array of strings";
            return false;
        }
        for (const JsonValue& tag : tags->arrVal) {
            if (tag.type != JsonType::String || tag.strVal.empty()) {
                error = "genreTags must be an array of strings";
                return false;
            }
            manifest.genreTags.push_back(tag.strVal);
        }
    }

    manifest.summary = root.GetString("summary", "");
    manifest.sourceNote = root.GetString("sourceNote", "");
    if (!ReadTokens(root, manifest, error)) {
        return false;
    }

    std::error_code ec;
    const std::filesystem::path jsonPath(campaignJsonPath);
    const std::filesystem::path packDir = std::filesystem::weakly_canonical(jsonPath.parent_path(), ec);
    if (ec) {
        error = "could not resolve pack directory for " + campaignJsonPath;
        return false;
    }
    if (packDir.filename().generic_string() != manifest.id) {
        error = "id must match the pack directory name";
        return false;
    }

    const std::filesystem::path assets = packDir / manifest.assetsRoot;
    if (!PathIsInside(packDir, assets)) {
        error = "assetsRoot escapes the pack directory";
        return false;
    }
    if (!manifest.entryScript.empty()) {
        const std::filesystem::path script = packDir / manifest.entryScript;
        if (!PathIsInside(packDir, script)) {
            error = "entryScript escapes the pack directory";
            return false;
        }
    }

    manifest.packDirectory = packDir.generic_string();
    manifest.resolvedAssetsRoot = std::filesystem::weakly_canonical(assets, ec).generic_string();
    if (ec) {
        error = "could not resolve assetsRoot";
        return false;
    }

    out = std::move(manifest);
    return true;
}

} // namespace Dnd
