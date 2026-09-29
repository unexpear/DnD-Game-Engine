# ============================================================================
# CMakeLists.txt — D&D Game Engine Build Configuration
# ============================================================================
#
# This file configures the build system for the entire D&D Game Engine.
#
# Build instructions (Windows):
#   mkdir build && cd build
#   cmake .. -G "Visual Studio 17 2022"
#   cmake --build . --config Debug
#
# For command-line builds without an IDE:
#   cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build
#
# ============================================================================

# Require CMake 3.20 or later. This version supports:
#   - file(GLOB_RECURSE) with CONFIGURE_DEPENDS
#   - Modern target-based commands
#   - Improved generator expressions
cmake_minimum_required(VERSION 3.20)

# Project declaration: name, version, languages.
# CXX = C++, C = C (needed for GLAD's C source files).
project(DnDEngine VERSION 0.1.0 LANGUAGES CXX C)

# Use C++17 standard. Required for:
#   - std::filesystem (though we use our own for now)
#   - Structured bindings
#   - if constexpr
#   - std::optional, std::variant
# CMAKE_CXX_STANDARD_REQUIRED ON means CMake fails if compiler doesn't support C++17.
# CMAKE_CXX_EXTENSIONS OFF disables compiler-specific extensions (e.g., GNU extensions).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDE integration (VS Code, CLion).
# This file contains the exact compile command for every source file,
# enabling IntelliSense and clang-tidy to work correctly.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pass version numbers to source code via preprocessor defines.
# This allows engine code to report its version at runtime.
add_compile_definitions(
    DND_ENGINE_VERSION_MAJOR=${DnDEngine_VERSION_MAJOR}
    DND_ENGINE_VERSION_MINOR=${DnDEngine_VERSION_MINOR}
    DND_ENGINE_VERSION_PATCH=${DnDEngine_VERSION_PATCH}
)

# ==========================================
# SOURCE FILE COLLECTION
# ==========================================
#
# GLOB_RECURSE finds all .cpp and .c files in each subsystem directory.
#
# NOTE: GLOB_RECURSE does NOT automatically detect new files after CMake
# generates the build files. If you add a new .cpp file, you must re-run
# cmake to regenerate the build system. This is a known CMake limitation.
#
# Alternative: list files explicitly, but GLOB_RECURSE is acceptable during
# rapid development. Switch to explicit lists before release.

file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS "src/core/*.cpp" "src/core/*.c")
file(GLOB_RECURSE RENDERING_SRC CONFIGURE_DEPENDS "src/rendering/*.cpp" "src/rendering/*.c")
file(GLOB_RECURSE ECS_SRC CONFIGURE_DEPENDS "src/ecs/*.cpp" "src/ecs/*.c")
file(GLOB_RECURSE SCENE_SRC CONFIGURE_DEPENDS "src/scene/*.cpp" "src/scene/*.c")
file(GLOB_RECURSE PHYSICS_SRC CONFIGURE_DEPENDS "src/physics/*.cpp" "src/physics/*.c")
file(GLOB_RECURSE ANIMATION_SRC CONFIGURE_DEPENDS "src/animation/*.cpp" "src/animation/*.c")
file(GLOB_RECURSE UI_SRC CONFIGURE_DEPENDS "src/ui/*.cpp" "src/ui/*.c")
file(GLOB_RECURSE TOOLS_SRC CONFIGURE_DEPENDS "src/tools/*.cpp" "src/tools/*.c")
file(GLOB_RECURSE AUDIO_SRC CONFIGURE_DEPENDS "src/audio/*.cpp" "src/audio/*.c")
file(GLOB_RECURSE NETWORK_SRC CONFIGURE_DEPENDS "src/network/*.cpp" "src/network/*.c")
file(GLOB_RECURSE ASSETS_SRC CONFIGURE_DEPENDS "src/assets/*.cpp" "src/assets/*.c")
file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS "src/game/*.cpp" "src/game/*.c")
file(GLOB_RECURSE AI_SRC CONFIGURE_DEPENDS "src/ai/*.cpp" "src/ai/*.c")
file(GLOB_RECURSE CAMPAIGN_SRC CONFIGURE_DEPENDS "src/campaign/*.cpp" "src/campaign/*.c")

# Combine all source lists into a single variable for the executable target.
set(ALL_SRC
    ${CORE_SRC}
    ${RENDERING_SRC}
    ${ECS_SRC}
    ${SCENE_SRC}
    ${PHYSICS_SRC}
    ${ANIMATION_SRC}
    ${UI_SRC}
    ${TOOLS_SRC}
    ${AUDIO_SRC}
    ${NETWORK_SRC}
    ${ASSETS_SRC}
    ${GAME_SRC}
    ${AI_SRC}
    ${CAMPAIGN_SRC}
)

# ==========================================
# EXECUTABLE TARGET
# ==========================================
# Creates the main engine executable from all collected source files.
add_executable(${PROJECT_NAME} ${ALL_SRC} src/main.cpp)

# Include directories for header file resolution.
# PRIVATE means these include paths are only used when compiling this target,
# not propagated to targets that link against it.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Source-tree root so the campaign loader can find Campaigns/ when the
# process is not started from the repository root.
target_compile_definitions(${PROJECT_NAME} PRIVATE
    DND_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

# ==========================================
# THIRD-PARTY DEPENDENCIES
# ==========================================

# OpenGL — the rendering API. find_package locates the system OpenGL libraries.
# On Windows, this links opengl32.lib from the system SDK.
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

# GLFW — cross-platform window creation and input library.
# We build it from source via CMake subdirectory for ABI compatibility.
#
# Setup: Clone GLFW into third_party/glfw:
#   git clone https://github.com/glfw/glfw.git third_party/glfw
if(NOT TARGET glfw)
    # Suppress GLFW's installation rules, examples, and tests.
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/glfw)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# GLAD — OpenGL function loader.
# GLAD generates C source files that load OpenGL function pointers at runtime.
# Necessary because Windows only ships with OpenGL 1.1 headers.
#
# Setup: Generate at https://glad.dav1d.de/
#   - Language: C/C++, Specification: gl, API: gl 4.6, Profile: Core
#   - Options: Generate a loader
# Download and extract to third_party/glad/
if(NOT TARGET glad)
    add_library(glad STATIC third_party/glad/src/glad.c)
    target_include_directories(glad PUBLIC third_party/glad/include)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE glad)

# stb_image — header-only image loading library.
# Supports PNG, JPEG, BMP, TGA. Single-header means no build step.
#
# Setup: Download stb_image.h from https://github.com/nothings/stb
# Place in third_party/stb/stb_image.h
target_include_directories(${PROJECT_NAME} PRIVATE third_party/stb)

# miniaudio — single-header audio library.
# Header-only, no build step. Implementation defined in src/audio/miniaudio_impl.cpp.
target_include_directories(${PROJECT_NAME} PRIVATE third_party/miniaudio)

# ==========================================
# PLATFORM-SPECIFIC SETTINGS
# ==========================================

if(WIN32)
    # Link additional Windows system libraries:
    #   opengl32 — OpenGL runtime
    #   gdi32 — Device context handling (needed by GLFW)
    #   mfplat/mfreadwrite/mfuuid/propsys — Media Foundation (MP4/AAC)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32 gdi32 mfplat mfreadwrite mfuuid propsys)

    # WIN32_EXECUTABLE FALSE: show console window for logging during development.
    # Set to TRUE for release builds to hide the console.
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
        OUTPUT_NAME "DnDEngine"
    )
endif()

# ==========================================
# POST-BUILD COMMANDS
# ==========================================

# Copy the assets/ directory to the build output directory after each build.
# This ensures the executable can find its assets at runtime.
# NOTE: This runs on EVERY build, even if assets haven't changed.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets
    COMMENT "Copying assets to build directory..."
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Campaigns
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/Campaigns
    COMMENT "Copying campaign packs to build directory..."
)

# Headless checks for pack discovery and session isolation. Does not link
# the renderer, so it can run without a display.
add_executable(campaign_registry_test
    tests/campaign_registry_test.cpp
    src/campaign/campaign_manifest.cpp
    src/campaign/campaign_registry.cpp
    src/campaign/campaign_ruleset.cpp
    src/campaign/campaign_session.cpp
    src/core/json.cpp
)
target_include_directories(campaign_registry_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(campaign_registry_test PRIVATE
    DND_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
