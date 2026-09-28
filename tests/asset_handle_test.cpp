#include "assets/asset_manager.h"
#include <cstdio>

using namespace Dnd;

static int g_failures = 0;

static void Check(bool condition, const char* description) {
    std::printf("  [%s] %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) ++g_failures;
}

int main() {
    std::printf("-- AssetHandle --\n");

    AssetHandle invalid;
    Check(!invalid.IsValid(), "default handle is invalid");

    AssetHandle valid(0, AssetType::MESH);
    Check(valid.IsValid(), "handle with id=0 / type=MESH is valid");
    Check(valid.type == AssetType::MESH, "type is MESH");
    Check(valid.id == 0, "id is 0");

    AssetHandle copy = valid;
    Check(copy.IsValid(), "copied handle is valid");
    Check(copy.id == 0 && copy.type == AssetType::MESH, "copied handle matches");

    AssetHandle moved = std::move(valid);
    Check(moved.IsValid(), "moved-to handle is valid");
    Check(moved.id == 0 && moved.type == AssetType::MESH, "moved-to handle matches");

    Check(!AssetHandle::Invalid().IsValid(), "AssetHandle::Invalid() is invalid");

    AssetHandle other(0, AssetType::AUDIO_CLIP);
    Check(other.IsValid(), "different type handle is valid");
    Check(other.type == AssetType::AUDIO_CLIP, "type is AUDIO_CLIP");

    AssetHandle highId(9999, AssetType::TEXTURE);
    Check(highId.IsValid(), "high-id handle is valid");
    Check(highId.id == 9999 && highId.type == AssetType::TEXTURE, "high-id handle values");

    std::printf("\n==== %s (%d failure%s) ====\n",
                g_failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
