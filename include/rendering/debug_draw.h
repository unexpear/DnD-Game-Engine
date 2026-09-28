/**
 * @file debug_draw.h
 * @brief OpenGL debug rendering: lines, boxes, spheres, vectors.
 * 
 * Provides simple wireframe rendering for visualizing physics colliders,
 * coordinate axes, bounding boxes, and debug geometry in the 3D viewport.
 * 
 * Features:
 *   - Draw lines between two points
 *   - Draw wireframe boxes (axis-aligned and transformed)
 *   - Draw wireframe spheres
 *   - Draw arrows (direction vectors with arrowheads)
 *   - Draw coordinate axes (RGB = XYZ)
 * 
 * All debug geometry is rendered as unlit wireframes with a solid color.
 * Depth testing is enabled so debug geometry respects scene depth.
 * 
 * @note Debug rendering should happen AFTER the main scene render,
 *       so debug geometry appears on top of rendered objects.
 * @warning This is a simple immediate-mode system — geometry is built
 *          and drawn each frame. Not suitable for large amounts of geometry.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"

namespace Dnd {

// Forward declaration
class Shader;

// ==========================================
// CLASS: DebugDraw
// ==========================================

/**
 * @class DebugDraw
 * @brief Immediate-mode debug rendering system.
 * 
 * All methods are static. Debug geometry is accumulated into a buffer
 * and drawn all at once when Flush() is called.
 * 
 * Usage:
 * @code
 * DebugDraw::DrawLine(Vec3(0,0,0), Vec3(1,0,0), Color::Red());
 * DebugDraw::DrawBox(center, size, Color::Green());
 * DebugDraw::DrawArrow(position, direction, Color::Blue());
 * DebugDraw::Flush(viewMatrix, projectionMatrix);
 * @endcode
 */
class DebugDraw {
public:
    /**
     * @brief Initializes the debug drawing system.
     * 
     * Implementation:
     *   1. Compile the debug shader (simple unlit line shader)
     *   2. Create the vertex and index buffers
     *   3. Reserve space in the vertex buffer (max vertices per frame)
     * 
     * @return true if initialization succeeded.
     */
    static b8 Initialize();

    /** @brief Shuts down the debug drawing system. */
    static void Shutdown();

    /**
     * @brief Draws a line between two points.
     * @param start Start point of the line.
     * @param end End point of the line.
     * @param color Line color.
     */
    static void DrawLine(const Vec3& start, const Vec3& end, const Color& color = Color::White());

    /**
     * @brief Draws a wireframe axis-aligned box.
     * @param center Center of the box.
     * @param halfSize Half-extents of the box (size/2 on each axis).
     * @param color Box color.
     */
    static void DrawBox(const Vec3& center, const Vec3& halfSize, const Color& color = Color::White());

    /**
     * @brief Draws a wireframe sphere.
     * @param center Center of the sphere.
     * @param radius Sphere radius.
     * @param segments Number of segments per ring (more = smoother). Default: 16.
     * @param color Sphere color.
     */
    static void DrawSphere(const Vec3& center, f32 radius, i32 segments = 16, const Color& color = Color::White());

    /**
     * @brief Draws an arrow (line with arrowhead).
     * @param start Start point of the arrow shaft.
     * @param direction Direction vector of the arrow.
     * @param color Arrow color.
     * @param headLength Length of the arrowhead. Default: 0.2.
     */
    static void DrawArrow(const Vec3& start, const Vec3& direction, const Color& color = Color::White(), f32 headLength = 0.2f);

    /**
     * @brief Draws the coordinate axes at a point.
     * 
     * X axis = red, Y axis = green, Z axis = blue.
     * 
     * @param origin Origin point for the axes.
     * @param length Length of each axis. Default: 1.0.
     */
    static void DrawAxes(const Vec3& origin = Vec3(0, 0, 0), f32 length = 1.0f);

    /**
     * @brief Renders all accumulated debug geometry.
     * 
     * Implementation:
     *   1. If no vertices accumulated, return immediately
     *   2. Upload vertex data to GPU
     *   3. Bind the debug shader
     *   4. Set the view and projection matrix uniforms
     *   5. Draw all lines with glDrawArrays(GL_LINES)
     *   6. Reset the vertex count for the next frame
     * 
     * @param viewMatrix The camera view matrix.
     * @param projectionMatrix The camera projection matrix.
     * @note Call once per frame AFTER rendering the main scene.
     */
    static void Flush(const Mat4& viewMatrix, const Mat4& projectionMatrix);

    /** @brief Clears all accumulated debug geometry without rendering. */
    static void Clear();

    /** @brief Gets the current number of accumulated vertices. */
    static i32 GetVertexCount();

private:
    /** @brief Private constructor. Enforces static-only access. */
    DebugDraw() = default;

    /** @brief Internal vertex for debug lines. */
    struct DebugVertex {
        Vec3 position;
        Vec4 color;
    };

    /** @brief Maximum number of debug vertices per frame. */
    static constexpr i32 MAX_VERTICES = 65536;
    /** @brief Debug vertex buffer. */
    static DebugVertex s_vertices[MAX_VERTICES];
    /** @brief Current number of vertices in the buffer. */
    static i32 s_vertexCount;
    /** @brief Debug shader program. */
    static Shader* s_shader;
    /** @brief OpenGL buffer ID for vertex data. */
    static u32 s_vbo;
    /** @brief OpenGL VAO ID. */
    static u32 s_vao;
    /** @brief Whether debug draw has been initialized. */
    static b8 s_initialized;

    /**
     * @brief Adds a vertex to the debug buffer.
     * @param position Vertex position.
     * @param color Vertex color.
     * @note Returns without adding if buffer is full.
     */
    static void AddVertex(const Vec3& position, const Color& color);
};

} // namespace Dnd
