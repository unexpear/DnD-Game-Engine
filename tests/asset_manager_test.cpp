#include "assets/asset_manager.h"
#include "core/types.h"
#include <cstdio>

using namespace Dnd;

static int g_failures = 0;

static void Check(bool condition, const char* description) {
    std::printf("  [%s] %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) ++g_failures;
}

int main() {
    // ==========================================
    // AssetHandle
    // ==========================================
    std::printf("-- AssetHandle --\n");

    AssetHandle invalid;
    Check(!invalid.IsValid(), "default handle is invalid");

    AssetHandle valid(0, AssetType::MESH);
    Check(valid.IsValid(), "handle with id=0 is valid");
    Check(valid.type == AssetType::MESH, "handle type is MESH");
    Check(valid.id == 0, "handle id is 0");

    AssetHandle copy = valid;
    Check(copy.IsValid(), "copied handle is valid");
    Check(copy.id == 0 && copy.type == AssetType::MESH, "copied handle matches source");

    AssetHandle moved = std::move(valid);
    Check(moved.IsValid(), "moved-to handle is valid");
    Check(moved.id == 0 && moved.type == AssetType::MESH, "moved-to handle matches");

    // ==========================================
    // AssetManager singleton
    // ==========================================
    std::printf("-- AssetManager Singleton --\n");

    AssetManager& mgr = AssetManager::Get();
    Check(&mgr != nullptr, "AssetManager::Get() returns non-null reference");

    AssetManager& mgr2 = AssetManager::Get();
    Check(&mgr == &mgr2, "AssetManager::Get() returns same instance");

    // ==========================================
    // Search paths
    // ==========================================
    std::printf("-- Search Paths --\n");

    mgr.AddSearchPath("Assets/");
    mgr.AddSearchPath("Data/");
    // No crash = pass; we can't easily verify internal state without an accessor

    // ResolvePath on a non-existent file returns empty
    std::string r = mgr.ResolvePath("__nonexistent_file_xyz__.obj");
    Check(r.empty(), "ResolvePath returns empty for missing file");

    std::printf("\n==== %s (%d failure%s) ====\n",
                g_failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
