/**
 * @file main.cpp
 * @brief Application entry point for the D&D Game Engine.
 * 
 * Parses command-line arguments, initializes the engine, runs the main loop,
 * and reports exit codes.
 * 
 * Command-line arguments:
 *   --debug        Enable debug logging (TRACE and DEBUG levels)
 *   --logfile=path Set log file path (default: "engine.log")
 *   --fullscreen   Start in fullscreen mode
 *   --width=N      Set window width (default: 1280)
 *   --height=N     Set window height (default: 720)
 *   --novsync      Disable VSync
 *   --campaign=id  Load a campaign pack (default: dnd)
 *   --list-campaigns  Print registered packs and exit
 * 
 * Exit codes:
 *   0  — Success. Engine ran and shut down normally.
 *   1  — Engine initialization failed.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.2.0
 */

#include "core/engine.h"
#include "core/log.h"
#include "core/types.h"
#include "campaign/campaign_registry.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

// ============================================================================
// Hybrid-GPU preference (laptops/desktops with Intel iGPU + a discrete GPU).
// These exported symbols are read by the NVIDIA Optimus and AMD PowerXpress
// drivers at startup and request that this app run on the DISCRETE GPU instead
// of the integrated Intel one. They must be exported from the executable
// module itself, which is why they live here in main.cpp.
// ============================================================================
#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;       // NVIDIA: use high-performance GPU
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;          // AMD: use high-performance GPU
}
#endif

/**
 * @brief Parses command-line arguments and configures the engine.
 * @param argc Argument count.
 * @param argv Argument strings.
 * @param outDebugLevel If true, debug logging should be enabled.
 * @return Configured EngineConfig.
 */
static Dnd::EngineConfig ParseArguments(i32 argc, char* argv[], bool& outDebugLevel, bool& outListCampaigns) {
    Dnd::EngineConfig config;
    outDebugLevel = false;
    outListCampaigns = false;

    for (i32 i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (std::strcmp(arg, "--logfile=") == 0 || std::strncmp(arg, "--logfile=", 10) == 0) {
            config.logFilePath = arg + 10;
        }
        else if (std::strcmp(arg, "--fullscreen") == 0) {
            config.fullscreenEnabled = true;
        }
        else if (std::strncmp(arg, "--width=", 8) == 0) {
            config.windowWidth = std::atoi(arg + 8);
        }
        else if (std::strncmp(arg, "--height=", 9) == 0) {
            config.windowHeight = std::atoi(arg + 9);
        }
        else if (std::strcmp(arg, "--novsync") == 0) {
            config.vsyncEnabled = false;
        }
        else if (std::strcmp(arg, "--debug") == 0) {
            outDebugLevel = true;
        }
        else if (std::strncmp(arg, "--campaign=", 11) == 0) {
            config.campaignId = arg + 11;
        }
        else if (std::strcmp(arg, "--list-campaigns") == 0) {
            outListCampaigns = true;
        }
        else if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            std::printf("D&D Game Engine\n");
            std::printf("Usage: DnDEngine [options]\n");
            std::printf("  --debug          Enable debug logging\n");
            std::printf("  --logfile=path   Set log file path\n");
            std::printf("  --fullscreen     Start in fullscreen\n");
            std::printf("  --width=N        Set window width\n");
            std::printf("  --height=N       Set window height\n");
            std::printf("  --novsync        Disable VSync\n");
            std::printf("  --campaign=id    Load a campaign pack (default: dnd)\n");
            std::printf("  --list-campaigns Print campaign packs and exit\n");
            std::printf("  --help, -h       Show this help\n");
            std::exit(0);
        }
        else {
            std::fprintf(stderr, "[WARN] Unknown argument: %s\n", arg);
        }
    }

    return config;
}

/**
 * @brief Prints every registered campaign pack and exits the process via main.
 * @return 0 when at least one pack is listed.
 */
static i32 ListCampaigns() {
    Dnd::CampaignRegistry registry;
    for (const std::string& root : Dnd::CampaignRegistry::DefaultSearchRoots()) {
        const Dnd::CampaignScanReport report = registry.ScanDirectory(root);
        for (const std::string& message : report.messages) {
            std::fprintf(stderr, "campaign: %s\n", message.c_str());
        }
    }
    registry.EnsureBuiltinDnd();
    if (registry.All().empty()) {
        std::fprintf(stderr, "No campaign packs found.\n");
        return 1;
    }
    for (const Dnd::CampaignManifest& pack : registry.All()) {
        std::printf("%s\t%s\t%s\t%s\n",
                    pack.id.c_str(),
                    pack.title.c_str(),
                    pack.ruleset.c_str(),
                    pack.entryScene.c_str());
    }
    return 0;
}

/**
 * @brief Application entry point.
 *
 * Implementation:
 *   1. Parse command-line arguments
 *   2. Optionally list campaign packs and exit
 *   3. Initialize the engine
 *   4. Run the main loop (blocks)
 *   5. Shutdown
 *
 * @param argc Argument count.
 * @param argv Argument strings.
 * @return 0 on success, 1 on initialization failure.
 */
i32 main(i32 argc, char* argv[]) {
    // Step 1: Parse command-line arguments
    bool enableDebug = false;
    bool listCampaigns = false;
    Dnd::EngineConfig config = ParseArguments(argc, argv, enableDebug, listCampaigns);

    if (listCampaigns) {
        return ListCampaigns();
    }

    // Step 2: Initialize the engine
    if (!Dnd::Engine::Initialize(config)) {
        std::fprintf(stderr, "[FATAL] Engine initialization failed.\n");
        return 1;
    }

    // Apply debug flag after logger init
    if (enableDebug) {
        Dnd::Logger::SetLevel(Dnd::LogLevel::DEBUG);
    }

    // Step 3: Run the main loop (blocks until shutdown)
    Dnd::Engine::Run();

    // Step 4: Shut down
    Dnd::Engine::Shutdown();

    // Step 5: Success
    return 0;
}
