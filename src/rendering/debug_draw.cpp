/**
 * @file debug_draw.cpp
 * @brief Implementation of the DebugDraw class.
 * 
 * See debug_draw.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "rendering/debug_draw.h"
#include "rendering/shader.h"
#include "core/log.h"
#include <glad/glad.h>
#include <cmath>

namespace Dnd {

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

/** @brief Debug vertex buffer. */
DebugDraw::DebugVertex DebugDraw::s_vertices[MAX_VERTICES];

/** @brief Current number of vertices in the buffer. */
i32 DebugDraw::s_vertexCount = 0;

/** @brief Debug shader program. */
Shader* DebugDraw::s_shader = nullptr;

/** @brief OpenGL buffer ID for vertex data. */
u32 DebugDraw::s_vbo = 0;

/** @brief OpenGL VAO ID. */
u32 DebugDraw::s_vao = 0;

/** @brief Whether debug draw has been initialized. */
b8 DebugDraw::s_initialized = false;

// ==========================================
// INITIALIZATION
// ==========================================

b8 DebugDraw::Initialize() {
    if (s_initialized) {
        return true;
    }

    // Step 1: Compile the debug shader (simple unlit line shader)
    const char* vertexSource = R"(
        #version 460 core
        layout (location = 0) in vec3 a_position;
        layout (location = 1) in vec4 a_color;

        uniform mat4 u_view;
        uniform mat4 u_projection;

        out vec4 v_color;

        void main() {
            v_color = a_color;
            gl_Position = u_projection * u_view * vec4(a_position, 1.0);
        }
    )";

    const char* fragmentSource = R"(
        #version 460 core
        in vec4 v_color;
        out vec4 frag_color;

        void main() {
            frag_color = v_color;
        }
    )";

    s_shader = new Shader();
    if (!s_shader->Compile(vertexSource, fragmentSource)) {
        DND_LOG_ERROR("DebugDraw: Failed to compile debug shader");
        delete s_shader;
        s_shader = nullptr;
        return false;
    }

    // Step 2: Create VAO and VBO
    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);

    // Allocate buffer space (will be updated each frame)
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(DebugVertex), nullptr, GL_DYNAMIC_DRAW);

    // Configure vertex attributes
    // Position: location 0, 3 floats, offset 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)0);

    // Color: location 1, 4 floats, offset 12 bytes
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)12);

    glBindVertexArray(0);

    s_initialized = true;
    DND_LOG_INFO("DebugDraw initialized");
    return true;
}

void DebugDraw::Shutdown() {
    if (!s_initialized) {
        return;
    }

    if (s_shader != nullptr) {
        delete s_shader;
        s_shader = nullptr;
    }

    glDeleteVertexArrays(1, &s_vao);
    glDeleteBuffers(1, &s_vbo);

    s_vao = 0;
    s_vbo = 0;
    s_vertexCount = 0;
    s_initialized = false;

    DND_LOG_INFO("DebugDraw shutdown");
}

// ==========================================
// DRAWING FUNCTIONS
// ==========================================

void DebugDraw::DrawLine(const Vec3& start, const Vec3& end, const Color& color) {
    AddVertex(start, color);
    AddVertex(end, color);
}

void DebugDraw::DrawBox(const Vec3& center, const Vec3& halfSize, const Color& color) {
    // 8 corners of the box
    Vec3 corners[8];
    corners[0] = center + Vec3(-halfSize.x, -halfSize.y, -halfSize.z);
    corners[1] = center + Vec3( halfSize.x, -halfSize.y, -halfSize.z);
    corners[2] = center + Vec3( halfSize.x,  halfSize.y, -halfSize.z);
    corners[3] = center + Vec3(-halfSize.x,  halfSize.y, -halfSize.z);
    corners[4] = center + Vec3(-halfSize.x, -halfSize.y,  halfSize.z);
    corners[5] = center + Vec3( halfSize.x, -halfSize.y,  halfSize.z);
    corners[6] = center + Vec3( halfSize.x,  halfSize.y,  halfSize.z);
    corners[7] = center + Vec3(-halfSize.x,  halfSize.y,  halfSize.z);

    // 12 edges of the box (each edge is a line)
    DrawLine(corners[0], corners[1], color);
    DrawLine(corners[1], corners[2], color);
    DrawLine(corners[2], corners[3], color);
    DrawLine(corners[3], corners[0], color);
    DrawLine(corners[4], corners[5], color);
    DrawLine(corners[5], corners[6], color);
    DrawLine(corners[6], corners[7], color);
    DrawLine(corners[7], corners[4], color);
    DrawLine(corners[0], corners[4], color);
    DrawLine(corners[1], corners[5], color);
    DrawLine(corners[2], corners[6], color);
    DrawLine(corners[3], corners[7], color);
}

void DebugDraw::DrawSphere(const Vec3& center, f32 radius, i32 segments, const Color& color) {
    // Draw rings in XY, XZ, and YZ planes
    f32 step = (2.0f * Math::PI) / static_cast<f32>(segments);

    // XY plane rings (Z slices)
    for (i32 i = 0; i <= segments; i++) {
        f32 angle = i * step;
        Vec3 p1 = center + Vec3(std::cos(angle) * radius, std::sin(angle) * radius, 0);
        Vec3 p2 = center + Vec3(std::cos(angle + step) * radius, std::sin(angle + step) * radius, 0);
        DrawLine(p1, p2, color);

        // XZ plane
        p1 = center + Vec3(std::cos(angle) * radius, 0, std::sin(angle) * radius);
        p2 = center + Vec3(std::cos(angle + step) * radius, 0, std::sin(angle + step) * radius);
        DrawLine(p1, p2, color);

        // YZ plane
        p1 = center + Vec3(0, std::cos(angle) * radius, std::sin(angle) * radius);
        p2 = center + Vec3(0, std::cos(angle + step) * radius, std::sin(angle + step) * radius);
        DrawLine(p1, p2, color);
    }
}

void DebugDraw::DrawArrow(const Vec3& start, const Vec3& direction, const Color& color, f32 headLength) {
    Vec3 end = start + direction;

    // Shaft
    DrawLine(start, end, color);

    // Arrowhead (two lines from the tip back along the direction)
    Vec3 dir = direction.Normalized();
    // Find two perpendicular directions for the arrowhead
    Vec3 perp1;
    if (std::abs(dir.y) < 0.99f) {
        perp1 = Vec3(0, 1, 0).Cross(dir).Normalized();
    } else {
        perp1 = Vec3(1, 0, 0).Cross(dir).Normalized();
    }
    Vec3 perp2 = dir.Cross(perp1).Normalized();

    Vec3 headBase = end - dir * headLength;
    DrawLine(end, headBase + perp1 * headLength * 0.5f, color);
    DrawLine(end, headBase - perp1 * headLength * 0.5f, color);
    DrawLine(end, headBase + perp2 * headLength * 0.5f, color);
    DrawLine(end, headBase - perp2 * headLength * 0.5f, color);
}

void DebugDraw::DrawAxes(const Vec3& origin, f32 length) {
    DrawLine(origin, origin + Vec3(length, 0, 0), Color::Red());
    DrawLine(origin, origin + Vec3(0, length, 0), Color::Green());
    DrawLine(origin, origin + Vec3(0, 0, length), Color::Blue());
}

// ==========================================
// FLUSH (RENDER)
// ==========================================

void DebugDraw::Flush(const Mat4& viewMatrix, const Mat4& projectionMatrix) {
    if (!s_initialized || s_vertexCount == 0) {
        return;
    }

    // Step 1: Upload vertex data to GPU
    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, s_vertexCount * sizeof(DebugVertex), s_vertices);

    // Step 2: Bind shader and set uniforms
    s_shader->Bind();
    s_shader->SetUniform("u_view", viewMatrix);
    s_shader->SetUniform("u_projection", projectionMatrix);

    // Step 3: Disable depth write so debug lines appear on top
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Step 4: Draw all lines
    glDrawArrays(GL_LINES, 0, s_vertexCount);

    // Step 5: Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    s_shader->Unbind();
    glBindVertexArray(0);

    // Step 6: Reset vertex count for next frame
    s_vertexCount = 0;
}

void DebugDraw::Clear() {
    s_vertexCount = 0;
}

i32 DebugDraw::GetVertexCount() {
    return s_vertexCount;
}

// ==========================================
// INTERNAL HELPERS
// ==========================================

void DebugDraw::AddVertex(const Vec3& position, const Color& color) {
    if (s_vertexCount >= MAX_VERTICES) {
        DND_LOG_WARN("DebugDraw vertex buffer full (%d vertices)", MAX_VERTICES);
        return;
    }

    s_vertices[s_vertexCount].position = position;
    s_vertices[s_vertexCount].color = Vec4(color.r, color.g, color.b, color.a);
    s_vertexCount++;
}

} // namespace Dnd
