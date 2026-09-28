/**
 * @file ui_renderer.h
 * @brief 2D UI renderer that draws rectangles and text using OpenGL.
 *
 * The UIRenderer provides a simple immediate-mode 2D drawing API for
 * the UI system. It manages an orthographic projection, a dedicated
 * 2D shader, and a unit quad mesh for rendering filled rectangles.
 *
 * Text rendering uses a placeholder bitmap approach where each character
 * is drawn as a small colored rectangle. This will be replaced with
 * proper font rendering in a future phase.
 *
 * Coordinate system: pixel-based with (0,0) at top-left.
 *   X increases to the right, Y increases downward.
 *
 * Usage:
 * @code
 * // During initialization:
 * UIRenderer::Initialize(screenWidth, screenHeight);
 *
 * // Each frame:
 * Mat4 ortho = Mat4::Orthographic(0, w, h, 0, -1, 1);
 * UIRenderer::BeginFrame(ortho);
 * UIRenderer::DrawRect(10, 10, 200, 50, Vec4(0.3f, 0.5f, 0.8f, 1.0f));
 * UIRenderer::DrawText(50, 80, "Hello, World!", Vec4(1, 1, 1, 1), 16.0f);
 * UIRenderer::EndFrame();
 *
 * // During shutdown:
 * UIRenderer::Shutdown();
 * @endcode
 *
 * @note All methods are static. The UIRenderer is designed to be a singleton
 *       subsystem, managed by UISystem.
 * @warning Initialize() must be called with a valid OpenGL context active.
 *
 * @author DND Engine Team
 * @date 2026-05-08
 * @version 0.7.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"

namespace Dnd {

// Forward declarations
class Shader;
class Mesh;

// ==========================================
// CLASS: UIRenderer
// ==========================================

/**
 * @class UIRenderer
 * @brief Static 2D renderer for UI elements.
 *
 * Provides immediate-mode drawing of filled rectangles and placeholder
 * text using a dedicated orthographic shader and a unit quad mesh.
 * All rendering is batched through a single draw call per element.
 *
 * The renderer manages:
 *   - A compiled GLSL shader (orthographic projection + model transform)
 *   - A unit quad mesh (centered at origin, 1x1 in XY plane)
 *   - Screen dimensions for coordinate mapping
 */
class UIRenderer {
public:
    /**
     * @brief Initializes the UI renderer.
     *
     * Implementation:
     *   1. If already initialized, destroy and reinitialize
     *   2. Store screen dimensions
     *   3. Create the orthographic shader:
     *      - Vertex shader: transforms position by model * projection
     *      - Fragment shader: outputs uniform color
     *   4. Create a unit quad mesh (XY plane, centered at origin):
     *      - 4 vertices, 6 indices (2 triangles)
     *      - Spans from (-0.5, -0.5, 0) to (0.5, 0.5, 0)
     *   5. Log success/failure
     *
     * @param screenWidth Screen width in pixels.
     * @param screenHeight Screen height in pixels.
     * @return true if the shader compiled and the quad mesh was created successfully.
     * @pre An active OpenGL 4.6 Core context must exist.
     * @post Shader and quad mesh are ready for rendering.
     */
    static b8 Initialize(u32 screenWidth, u32 screenHeight);

    /**
     * @brief Shuts down the UI renderer and frees OpenGL resources.
     *
     * Implementation:
     *   1. Destroy the quad mesh
     *   2. Delete the shader
     *   3. Reset all static pointers to nullptr
     *   4. Clear initialized flag
     *
     * @post All OpenGL resources are freed.
     */
    static void Shutdown();

    /**
     * @brief Begins a UI rendering frame.
     *
     * Implementation:
     *   1. Bind the UI shader
     *   2. Set the orthographic projection matrix uniform
     *   3. Disable depth testing (UI is an overlay)
     *   4. Enable blending with GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
     *
     * @param projMatrix The orthographic projection matrix (pixel to NDC).
     * @pre Initialize() must have been called successfully.
     * @post Shader is bound and rendering state is configured.
     */
    static void BeginFrame(const Mat4& projMatrix);

    /**
     * @brief Ends the UI rendering frame.
     *
     * Implementation:
     *   1. Unbind the UI shader
     *   2. Re-enable depth testing (for subsequent 3D rendering)
     *
     * @post Shader is unbound and OpenGL state is restored.
     */
    static void EndFrame();

    /**
     * @brief Draws a filled rectangle.
     *
     * Implementation:
     *   1. Compute the model matrix:
     *      - Translate by (x + w/2, y + h/2, 0) to center the rect
     *      - Scale by (w, h, 1) to match the requested dimensions
     *   2. Set the model matrix and color uniforms on the shader
     *   3. Bind the quad mesh and issue an indexed draw call
     *   4. Unbind the mesh
     *
     * @param x Left edge of the rectangle in screen pixels.
     * @param y Top edge of the rectangle in screen pixels.
     * @param w Width of the rectangle in screen pixels.
     * @param h Height of the rectangle in screen pixels.
     * @param color Fill color as RGBA (all components in [0, 1]).
     * @pre BeginFrame() must have been called.
     */
    static void DrawRect(f32 x, f32 y, f32 w, f32 h, const Vec4& color);

    /**
     * @brief Draws a rectangle with a user-provided RGBA texture.
     *
     * Implementation:
     *   1. Bind the given texture on texture unit 1
     *   2. Set u_thumbnail sampler to unit 1
     *   3. Set u_useTexture = 2 (thumbnail mode)
     *   4. Compute and set model matrix
     *   5. Issue draw call with full UV rect
     *   6. Restore font atlas on unit 0, reset to solid-fill
     *
     * @param x Left edge in screen pixels.
     * @param y Top edge in screen pixels.
     * @param w Width in screen pixels.
     * @param h Height in screen pixels.
     * @param glTextureId OpenGL texture ID to sample from.
     * @param tint Optional tint color (multiplied with texture, default white).
     */
    static void DrawTexturedRect(f32 x, f32 y, f32 w, f32 h, u32 glTextureId, const Vec4& tint = Vec4(1,1,1,1));

    /**
     * @brief Draws text as placeholder character rectangles.
     *
     * Each printable ASCII character is rendered as a small filled rectangle.
     * The character cell size is `size` pixels wide and `size * 1.5` pixels tall.
     * Spaces advance the cursor without drawing. Non-printable characters are skipped.
     *
     * Implementation:
     *   1. Compute character cell dimensions (w = size, h = size * 1.5)
     *   2. Iterate through the string:
     *      - Spaces: advance cursor by char width
     *      - Printable (32-126): draw a filled rect at cursor, advance
     *      - Others: skip
     *
     * @param x Left edge of the first character in screen pixels.
     * @param y Top edge of the text in screen pixels.
     * @param text Null-terminated ASCII string to render.
     * @param color Text color as RGBA (all components in [0, 1]).
     * @param size Character width in screen pixels (height = size * 1.5).
     * @pre BeginFrame() must have been called.
     * @pre text must not be nullptr.
     */
    static void DrawText(f32 x, f32 y, const char* text, const Vec4& color, f32 size);

private:
    /** @brief UI shader program. Owned, heap-allocated. */
    static Shader* s_shader;

    /** @brief Unit quad mesh for rectangle rendering. Owned, heap-allocated. */
    static Mesh* s_quadMesh;

    /** @brief OpenGL texture id of the baked 8x8 font atlas (R8, 128x64). */
    static u32 s_fontTexture;

    /** @brief Cached screen width in pixels. */
    static u32 s_screenWidth;

    /** @brief Cached screen height in pixels. */
    static u32 s_screenHeight;

    /** @brief Whether the renderer has been initialized. */
    static b8 s_initialized;
};

} // namespace Dnd
