/**
 * @file renderer.h
 * @brief OpenGL rendering abstraction and render pipeline management.
 * 
 * Provides the high-level rendering interface: initialization, frame
 * management, draw calls, and OpenGL state configuration.
 * 
 * Features:
 *   - OpenGL context initialization via GLAD
 *   - OpenGL error checking wrapper
 *   - Frame begin/end with buffer clearing
 *   - Viewport management
 *   - OpenGL state defaults (depth testing, face culling, blending)
 * 
 * @note This is the high-level renderer. For specific rendering tasks
 *       (shaders, meshes, textures), use the respective subsystems.
 * @warning All rendering methods assume an active OpenGL context.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"

namespace Dnd {

// ==========================================
// STRUCT: Spotlight
// ==========================================

/**
 * @struct Spotlight
 * @brief A cone light consumed by the scene shader's per-fragment ray-cast lighting.
 */
struct Spotlight {
    Vec3 position;   ///< World-space position of the light.
    Vec3 direction;  ///< Normalized cone axis (the way the light shines).
    Vec3 color;      ///< Light color / intensity (linear).
    f32 cosInner;    ///< Cosine of the inner cone half-angle (full brightness inside).
    f32 cosOuter;    ///< Cosine of the outer cone half-angle (falls to zero past this).
    f32 range;       ///< Distance falloff radius (no contribution beyond this).
};

// ==========================================
// CLASS: Renderer
// ==========================================

/**
 * @class Renderer
 * @brief Static OpenGL renderer manager.
 * 
 * All methods are static. Call Renderer::Initialize() after Window::Create()
 * (because GLAD needs an active OpenGL context), then call BeginFrame/EndFrame
 * each frame to manage the render pipeline.
 */
class Renderer {
public:
    /**
     * @brief Initializes the OpenGL renderer via GLAD.
     * 
     * Implementation:
     *   1. Check if already initialized
     *   2. Load OpenGL function pointers with gladLoadGLLoader()
     *      - Uses glfwGetProcAddress as the loader function
     *      - If fails, log error, return false
     *   3. Log OpenGL version and renderer string
     *   4. Set OpenGL default state:
     *      - Enable depth testing (GL_DEPTH_TEST)
     *      - Enable face culling (back faces only)
     *      - Set clear color to dark gray
     *      - Enable blending for transparency
     *      - Set viewport to current framebuffer size
     *   5. Mark as initialized
     * 
     * @return true if GLAD loaded successfully and state was configured.
     * @pre Window must be created with an active OpenGL context.
     */
    static b8 Initialize();

    /**
     * @brief Shuts down the renderer.
     * 
     * Implementation:
     *   1. Set initialized flag to false
     *   2. Log shutdown message
     * 
     * @note OpenGL contexts are cleaned up by GLFW during Window::Destroy().
     */
    static void Shutdown();

    /**
     * @brief Begins a new rendering frame.
     * 
     * Implementation:
     *   1. Clear the color buffer and depth buffer
     *   2. Update the viewport to match the current framebuffer size
     * 
     * @note Call at the START of each frame, before any draw calls.
     * @note The viewport is updated each frame to handle window resizing.
     */
    static void BeginFrame();

    /**
     * @brief Ends the current rendering frame.
     * 
     * Implementation:
     *   1. Check for OpenGL errors
     *   2. Swap the window buffers (presents the rendered frame)
     * 
     * @note Call at the END of each frame, after all draw calls.
     */
    static void EndFrame();

    /**
     * @brief Sets the OpenGL viewport dimensions.
     * @param x Viewport origin X (typically 0).
     * @param y Viewport origin Y (typically 0).
     * @param width Viewport width.
     * @param height Viewport height.
     */
    static void SetViewport(i32 x, i32 y, i32 width, i32 height);

    /**
     * @brief Sets the clear color (background color).
     * @param color The color to clear the screen to each frame.
     */
    static void SetClearColor(const Color& color);

    /**
     * @brief Gets the current clear color.
     * @return The current clear color.
     */
    static Color GetClearColor();

    /**
     * @brief Checks for OpenGL errors and logs them.
     * 
     * Implementation:
     *   1. Call glGetError() in a loop (multiple errors may be queued)
     *   2. For each error, log the error code and source location
     *   3. Return true if any errors were found
     * 
     * @param sourceFile Source file name (typically __FILE__).
     * @param sourceLine Source line number (typically __LINE__).
     * @return true if any OpenGL errors were detected.
     */
    static b8 CheckGLError(const char* sourceFile, i32 sourceLine);

    /**
     * @brief Gets the OpenGL version string.
     * @return OpenGL version string (e.g., "4.6.0 NVIDIA 531.68").
     */
    static const char* GetOpenGLVersion();

    /**
     * @brief Gets the OpenGL renderer string (GPU name).
     * @return Renderer string (e.g., "NVIDIA GeForce RTX 3080").
     */
    static const char* GetRendererString();

    /**
     * @brief Sets the camera view and projection matrices.
     * @param view The view matrix.
     * @param projection The projection matrix.
     */
    static void SetCameraMatrices(const Mat4& view, const Mat4& projection);

    /** @brief Returns the most recent view matrix (set by SetCameraMatrices). */
    static const Mat4& GetViewMatrix() { return s_viewMatrix; }

    /** @brief Returns the most recent projection matrix (set by SetCameraMatrices). */
    static const Mat4& GetProjectionMatrix() { return s_projectionMatrix; }

    static void DrawMesh(const class Mesh* mesh, const Mat4& model, const Color& color,
                         const class Texture2D* texture = nullptr);

    static u32 GetDrawCallCount() { return s_drawCallCount; }
    static void ResetDrawCallCount() { s_drawCallCount = 0; }

    /**
     * @brief Sets the spotlights the scene shader uses for ray-cast lighting.
     *
     * Copied into internal storage and uploaded to the scene shader each frame.
     * Up to MAX_SPOTLIGHTS are kept; extras are ignored.
     *
     * @param spotlights Array of spotlights.
     * @param count Number of spotlights in the array.
     */
    static void SetSpotlights(const Spotlight* spotlights, i32 count);

    /**
     * @brief Draws translucent additive "shafts" for each active spotlight.
     *
     * Call AFTER the opaque scene is drawn (camera matrices already set). Renders
     * a faint additive cone from each spotlight down to its ground pool.
     */
    static void DrawSpotlightBeams();

    /**
     * @brief Sets axis-aligned occluder boxes used for ray-cast spotlight shadows.
     *
     * Each fragment ray-casts toward every spotlight against these boxes; a hit
     * means the fragment is in shadow for that light.
     *
     * @param mins Array of box minimum corners (world space).
     * @param maxs Array of box maximum corners (world space).
     * @param count Number of boxes (clamped to MAX_BOXES; 0 disables shadows).
     */
    static void SetOccluders(const Vec3* mins, const Vec3* maxs, i32 count);

private:
    /** @brief Private constructor. Enforces static-only access. */
    Renderer() = default;

    /** @brief Whether the renderer has been initialized. */
    static b8 s_initialized;
    /** @brief Current clear color. */
    static Color s_clearColor;
    /** @brief Current view matrix. */
    static Mat4 s_viewMatrix;
    /** @brief Current projection matrix. */
    static Mat4 s_projectionMatrix;
    /** @brief Scene shader program. */
    static class Shader* s_sceneShader;
    /** @brief Lighting direction. */
    static Vec3 s_lightDir;
    /** @brief Lighting color. */
    static Vec3 s_lightColor;
    /** @brief Ambient lighting intensity. */
    static Vec3 s_ambient;

    /** @brief Maximum number of spotlights the scene shader supports. */
    static constexpr i32 MAX_SPOTLIGHTS = 8;
    /** @brief Active spotlights uploaded to the scene shader each frame. */
    static Spotlight s_spotlights[MAX_SPOTLIGHTS];
    /** @brief Number of active spotlights. */
    static i32 s_numSpotlights;

    /** @brief Lazily-compiled shader for the spotlight beam shafts. */
    static class Shader* s_beamShader;
    /** @brief Lazily-built unit cone mesh used for the beams. */
    static class Mesh* s_beamCone;

    /** @brief Draw call counter reset each frame. */
    static u32 s_drawCallCount;

    /** @brief Maximum ray-cast shadow occluder boxes. */
    static constexpr i32 MAX_BOXES = 16;
    /** @brief Occluder box min corners (world space). */
    static Vec3 s_boxMin[MAX_BOXES];
    /** @brief Occluder box max corners (world space). */
    static Vec3 s_boxMax[MAX_BOXES];
    /** @brief Number of active occluder boxes. */
    static i32 s_numBoxes;
};

// ==========================================
// OPENGL ERROR CHECKING MACRO
// ==========================================

/**
 * @brief Checks for OpenGL errors at the call site.
 * 
 * Usage: DND_GL_CHECK(); after any OpenGL call that might fail.
 * Expands to: Renderer::CheckGLError(__FILE__, __LINE__);
 */
#define DND_GL_CHECK() Dnd::Renderer::CheckGLError(__FILE__, __LINE__)

} // namespace Dnd
