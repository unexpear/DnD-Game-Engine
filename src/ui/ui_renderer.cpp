/**
 * @file ui_renderer.cpp
 * @brief Implementation of the UIRenderer class.
 *
 * See ui_renderer.h for full documentation.
 *
 * @author DND Engine Team
 * @date 2026-05-08
 * @version 0.7.0
 */

#include "ui/ui_renderer.h"
#include "ui/font8x8.h"
#include "rendering/shader.h"
#include "rendering/mesh.h"
#include "core/log.h"
#include <glad/glad.h>
#include <cstring>

namespace Dnd {

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

Shader* UIRenderer::s_shader = nullptr;
Mesh* UIRenderer::s_quadMesh = nullptr;
u32 UIRenderer::s_fontTexture = 0;
u32 UIRenderer::s_screenWidth = 1280;
u32 UIRenderer::s_screenHeight = 720;
b8 UIRenderer::s_initialized = false;

// ==========================================
// GLSL SHADER SOURCE
// ==========================================

/**
 * @brief Vertex shader for 2D UI rendering.
 *
 * Transforms vertices from model-local space to NDC using the
 * combined model and orthographic projection matrices.
 * Only uses vertex attribute location 0 (position).
 */
static const char* s_uiVertexShader = R"(
#version 460 core

layout(location = 0) in vec3 a_position;

uniform mat4 u_projection;
uniform mat4 u_model;
uniform vec4 u_uvRect; // (u0, v0, du, dv): glyph cell in the font atlas

out vec2 v_uv;

void main() {
    // Unit quad spans local [-0.5, 0.5]; map to [0,1] (x: left->right, y: top->bottom on screen).
    vec2 local = a_position.xy + vec2(0.5);
    v_uv = u_uvRect.xy + local * u_uvRect.zw;
    gl_Position = u_projection * u_model * vec4(a_position, 1.0);
}
)";

/**
 * @brief Fragment shader for 2D UI rendering.
 *
 * Outputs a uniform color with full alpha support.
 * Blending is handled externally (enabled in BeginFrame).
 */
static const char* s_uiFragmentShader = R"(
#version 460 core

in vec2 v_uv;

uniform vec4 u_color;
uniform int u_useTexture;   // 0 = solid fill, 1 = font atlas (coverage mask), 2 = thumbnail (full RGBA)
uniform sampler2D u_font;
uniform sampler2D u_thumbnail;

out vec4 fragColor;

void main() {
    if (u_useTexture == 1) {
        float coverage = texture(u_font, v_uv).r;
        if (coverage < 0.5) discard;
        fragColor = u_color;
    } else if (u_useTexture == 2) {
        vec4 texel = texture(u_thumbnail, v_uv);
        fragColor = texel * u_color;
    } else {
        fragColor = u_color;
    }
}
)";

// ==========================================
// INITIALIZATION
// ==========================================

b8 UIRenderer::Initialize(u32 screenWidth, u32 screenHeight) {
    // Store screen dimensions
    s_screenWidth = screenWidth;
    s_screenHeight = screenHeight;

    DND_LOG_INFO("Initializing UI Renderer (%ux%u)...", s_screenWidth, s_screenHeight);

    // Step 1: Create and compile the UI shader
    s_shader = new Shader();
    if (!s_shader->Compile(s_uiVertexShader, s_uiFragmentShader)) {
        DND_LOG_ERROR("UIRenderer: Failed to compile UI shader");
        delete s_shader;
        s_shader = nullptr;
        return false;
    }
    DND_LOG_DEBUG("UIRenderer: UI shader compiled (program ID: %u)", s_shader->GetProgramId());

    // Step 2: Create the unit quad mesh
    // The quad is a 1x1 unit square centered at the origin in the XY plane.
    // The model matrix (translate + scale) positions and sizes it in screen space.
    s_quadMesh = new Mesh();

    Vertex vertices[4] = {};

    // Bottom-left
    vertices[0].position = Vec3(-0.5f, -0.5f, 0.0f);
    // Bottom-right
    vertices[1].position = Vec3(0.5f, -0.5f, 0.0f);
    // Top-right
    vertices[2].position = Vec3(0.5f, 0.5f, 0.0f);
    // Top-left
    vertices[3].position = Vec3(-0.5f, 0.5f, 0.0f);

    u32 indices[6] = { 0, 1, 2, 0, 2, 3 };

    s_quadMesh->Create(vertices, 4, indices, 6);
    if (!s_quadMesh->IsCreated()) {
        DND_LOG_ERROR("UIRenderer: Failed to create quad mesh");
        delete s_shader;
        s_shader = nullptr;
        delete s_quadMesh;
        s_quadMesh = nullptr;
        return false;
    }
    DND_LOG_DEBUG("UIRenderer: Quad mesh created (VAO: %u)", s_quadMesh->IsCreated());

    // Step 3: Bake the 8x8 font into a single-channel atlas texture.
    // Layout: 16 columns x 8 rows of 8x8 glyphs => 128x64 texels (ASCII 0..127).
    {
        const i32 ATLAS_W = 128, ATLAS_H = 64;
        unsigned char atlas[ATLAS_W * ATLAS_H];
        std::memset(atlas, 0, sizeof(atlas));
        for (i32 ch = 0; ch < 128; ++ch) {
            const i32 cellCol = ch % 16;
            const i32 cellRow = ch / 16;
            for (i32 gy = 0; gy < 8; ++gy) {
                const unsigned char bits = kFont8x8[ch][gy];
                for (i32 gx = 0; gx < 8; ++gx) {
                    if (bits & (1u << gx)) { // bit 0 (LSB) = left-most pixel
                        atlas[(cellRow * 8 + gy) * ATLAS_W + (cellCol * 8 + gx)] = 255;
                    }
                }
            }
        }
        glGenTextures(1, &s_fontTexture);
        glBindTexture(GL_TEXTURE_2D, s_fontTexture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS_W, ATLAS_H, 0, GL_RED, GL_UNSIGNED_BYTE, atlas);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        DND_LOG_DEBUG("UIRenderer: Font atlas baked (texture id: %u)", s_fontTexture);
    }

    s_initialized = true;
    DND_LOG_INFO("UI Renderer initialized successfully");
    return true;
}

// ==========================================
// SHUTDOWN
// ==========================================

void UIRenderer::Shutdown() {
    if (!s_initialized) return;

    DND_LOG_INFO("Shutting down UI Renderer...");

    // Destroy the font atlas texture
    if (s_fontTexture) {
        glDeleteTextures(1, &s_fontTexture);
        s_fontTexture = 0;
    }

    // Destroy the quad mesh
    if (s_quadMesh) {
        s_quadMesh->Destroy();
        delete s_quadMesh;
        s_quadMesh = nullptr;
    }

    // Destroy the shader
    if (s_shader) {
        delete s_shader;
        s_shader = nullptr;
    }

    s_initialized = false;
    DND_LOG_INFO("UI Renderer shut down");
}

// ==========================================
// FRAME MANAGEMENT
// ==========================================

void UIRenderer::BeginFrame(const Mat4& projMatrix) {
    if (!s_initialized) return;

    // Configure OpenGL state for UI rendering
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Bind the UI shader and set the projection matrix
    s_shader->Bind();
    s_shader->SetUniform("u_projection", projMatrix);

    // Default to solid-fill mode; bind the font atlas on texture unit 0 for text draws.
    s_shader->SetUniform("u_useTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_fontTexture);
    s_shader->SetUniform("u_font", 0);
}

void UIRenderer::EndFrame() {
    if (!s_initialized) return;

    // Unbind the UI shader
    s_shader->Unbind();

    // Restore OpenGL state for subsequent 3D rendering
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ==========================================
// DRAWING METHODS
// ==========================================

void UIRenderer::DrawRect(f32 x, f32 y, f32 w, f32 h, const Vec4& color) {
    if (!s_initialized || w <= 0.0f || h <= 0.0f) return;

    // Compute model matrix:
    // The unit quad is centered at origin (extends from -0.5 to 0.5).
    // To place it at screen position (x, y) with size (w, h):
    //   1. Scale by (w, h, 1) to reach the desired size
    //   2. Translate by (x + w/2, y + h/2, 0) to position the center
    Mat4 model = Mat4::Translate(Vec3(x + w * 0.5f, y + h * 0.5f, 0.0f))
               * Mat4::Scale(Vec3(w, h, 1.0f));

    s_shader->SetUniform("u_useTexture", 0);
    s_shader->SetUniform("u_model", model);
    s_shader->SetUniform("u_color", color);

    s_quadMesh->Bind();
    s_quadMesh->DrawIndexed();
    s_quadMesh->Unbind();
}

void UIRenderer::DrawTexturedRect(f32 x, f32 y, f32 w, f32 h, u32 glTextureId, const Vec4& tint) {
    if (!s_initialized || w <= 0.0f || h <= 0.0f || glTextureId == 0) return;

    Mat4 model = Mat4::Translate(Vec3(x + w * 0.5f, y + h * 0.5f, 0.0f))
               * Mat4::Scale(Vec3(w, h, 1.0f));

    // Bind thumbnail on unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, glTextureId);
    s_shader->SetUniform("u_thumbnail", 1);

    s_shader->SetUniform("u_useTexture", 2);
    s_shader->SetUniform("u_model", model);
    s_shader->SetUniform("u_color", tint);
    s_shader->SetUniform("u_uvRect", Vec4(0.0f, 0.0f, 1.0f, 1.0f));

    s_quadMesh->Bind();
    s_quadMesh->DrawIndexed();
    s_quadMesh->Unbind();

    // Restore font atlas on unit 0 and solid-fill default
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_fontTexture);
    s_shader->SetUniform("u_font", 0);
    s_shader->SetUniform("u_useTexture", 0);
}

void UIRenderer::DrawText(f32 x, f32 y, const char* text, const Vec4& color, f32 size) {
    if (!s_initialized || !text || size <= 0.0f) return;

    // Each glyph is an 8x8 cell drawn as a square; advance by the full cell
    // (the font's built-in right-hand padding gives natural letter spacing).
    const f32 glyph = size;

    // Switch to textured mode and bind the atlas once for the whole string.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_fontTexture);
    s_shader->SetUniform("u_font", 0);
    s_shader->SetUniform("u_useTexture", 1);
    s_shader->SetUniform("u_color", color);
    s_quadMesh->Bind();

    f32 cursorX = x;
    for (const char* p = text; *p; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (c != ' ' && c < 128) {
            // Select this glyph's cell in the 16x8 atlas (128x64 texels).
            const i32 col = c % 16;
            const i32 row = c / 16;
            const Vec4 uvRect(
                static_cast<f32>(col * 8) / 128.0f,
                static_cast<f32>(row * 8) / 64.0f,
                8.0f / 128.0f,
                8.0f / 64.0f);
            s_shader->SetUniform("u_uvRect", uvRect);

            const Mat4 model = Mat4::Translate(Vec3(cursorX + glyph * 0.5f, y + glyph * 0.5f, 0.0f))
                             * Mat4::Scale(Vec3(glyph, glyph, 1.0f));
            s_shader->SetUniform("u_model", model);
            s_quadMesh->DrawIndexed();
        }
        cursorX += glyph;
    }

    s_quadMesh->Unbind();

    // Restore solid-fill defaults for following DrawRect calls.
    s_shader->SetUniform("u_useTexture", 0);
    s_shader->SetUniform("u_uvRect", Vec4(0.0f, 0.0f, 1.0f, 1.0f));
}

} // namespace Dnd
