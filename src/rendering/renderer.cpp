/**
 * @file renderer.cpp
 * @brief Implementation of the Renderer class.
 * 
 * See renderer.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "rendering/renderer.h"
#include "rendering/shader.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "core/window.h"
#include "core/log.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Dnd {

// ==========================================
// STATIC MEMBER INITIALIZATION
// ==========================================

/** @brief Whether the renderer has been initialized. */
b8 Renderer::s_initialized = false;

/** @brief Current clear color. Defaults to dark gray. */
Color Renderer::s_clearColor = Color(0.05f, 0.05f, 0.05f, 1.0f);

/** @brief Current view matrix. */
Mat4 Renderer::s_viewMatrix = Mat4::Identity();

/** @brief Current projection matrix. */
Mat4 Renderer::s_projectionMatrix = Mat4::Identity();

/** @brief Scene shader program. */
Shader* Renderer::s_sceneShader = nullptr;

/** @brief Lighting direction. */
Vec3 Renderer::s_lightDir = Vec3(1.0f, 2.0f, 1.0f).Normalized();

/** @brief Lighting color. */
Vec3 Renderer::s_lightColor = Vec3(1.0f, 0.95f, 0.9f);

/** @brief Ambient lighting intensity. */
Vec3 Renderer::s_ambient = Vec3(0.14f, 0.14f, 0.17f); // low base; the spotlights are the key light

/** @brief Draw call counter (reset each frame in BeginFrame). */
u32 Renderer::s_drawCallCount = 0;

/** @brief Active spotlights uploaded to the scene shader each frame. */
Spotlight Renderer::s_spotlights[Renderer::MAX_SPOTLIGHTS] = {};
/** @brief Number of active spotlights. */
i32 Renderer::s_numSpotlights = 0;

/** @brief Lazily-compiled beam shader. */
Shader* Renderer::s_beamShader = nullptr;
/** @brief Lazily-built unit cone mesh for the beams. */
Mesh* Renderer::s_beamCone = nullptr;

/** @brief Occluder box min corners. */
Vec3 Renderer::s_boxMin[Renderer::MAX_BOXES] = {};
/** @brief Occluder box max corners. */
Vec3 Renderer::s_boxMax[Renderer::MAX_BOXES] = {};
/** @brief Number of active occluder boxes. */
i32 Renderer::s_numBoxes = 0;

// ==========================================
// INITIALIZATION
// ==========================================

// GL debug-message callback (KHR_debug; core in GL 4.3+). Routes driver-reported
// errors and warnings into the engine log so problems surface immediately instead
// of failing silently (which is exactly how the black-screen bugs hid).
static void APIENTRY GLDebugCallback(GLenum /*source*/, GLenum /*type*/, GLuint /*id*/,
                                     GLenum severity, GLsizei /*length*/,
                                     const GLchar* message, const void* /*userParam*/) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:   DND_LOG_ERROR("[GL] %s", message); break;
        case GL_DEBUG_SEVERITY_MEDIUM: DND_LOG_WARN("[GL] %s", message);  break;
        default:                       DND_LOG_DEBUG("[GL] %s", message); break;
    }
}

b8 Renderer::Initialize() {
    // Step 1: Check if already initialized
    if (s_initialized) {
        DND_LOG_WARN("Renderer::Initialize called but already initialized");
        return true;
    }

    // Step 2: Load OpenGL function pointers via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        DND_LOG_FATAL("Failed to initialize GLAD (OpenGL loader)");
        return false;
    }

    // Step 3: Log OpenGL version and renderer information
    DND_LOG_INFO("OpenGL Version: %s", glGetString(GL_VERSION));
    DND_LOG_INFO("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    DND_LOG_INFO("GPU: %s", glGetString(GL_RENDERER));
    DND_LOG_INFO("Vendor: %s", glGetString(GL_VENDOR));

    // Register the GL debug callback (core in 4.3+; our context is 4.6, requested
    // as a debug context in Window::Create). Surfaces GL errors/warnings as logs.
    if (GLAD_GL_VERSION_4_3) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(GLDebugCallback, nullptr);
        // Silence low-severity notification spam (buffer-allocated info, etc.).
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
                              0, nullptr, GL_FALSE);
        DND_LOG_INFO("GL debug output enabled");
    }

    // Step 4: Set OpenGL default state

    // Enable depth testing — ensures closer fragments occlude farther ones
    glEnable(GL_DEPTH_TEST);

    // Enable back-face culling — don't render triangles facing away from camera
    // This halves the number of triangles rendered for closed meshes
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);  // Cull back faces
    glFrontFace(GL_CCW);  // Counter-clockwise = front face

    // Set default clear color
    glClearColor(s_clearColor.r, s_clearColor.g, s_clearColor.b, s_clearColor.a);

    // Enable blending for transparent objects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard alpha blending

    // Set initial viewport
    i32 width = Window::GetFramebufferWidth();
    i32 height = Window::GetFramebufferHeight();
    glViewport(0, 0, width, height);

    // Step 5: Mark as initialized
    s_initialized = true;
    DND_LOG_INFO("Renderer initialized. Viewport: %dx%d", width, height);

    return true;
}

// ==========================================
// SHUTDOWN
// ==========================================

void Renderer::Shutdown() {
    if (!s_initialized) {
        return;
    }

    DND_LOG_INFO("Renderer shutting down");

    delete s_sceneShader;  s_sceneShader = nullptr;
    delete s_beamShader;   s_beamShader = nullptr;
    delete s_beamCone;     s_beamCone = nullptr;

    s_initialized = false;
}

// ==========================================
// FRAME MANAGEMENT
// ==========================================

void Renderer::BeginFrame() {
    if (!s_initialized) {
        return;
    }

    // Step 1: Clear the color buffer (background) and depth buffer
    // GL_COLOR_BUFFER_BIT: clears the entire screen to the clear color
    // GL_DEPTH_BUFFER_BIT: resets depth values for the new frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Step 2: Update viewport to handle window resizing
    i32 width = Window::GetFramebufferWidth();
    i32 height = Window::GetFramebufferHeight();
    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);
    }

    // Reset per-frame GL state in case a subsystem (e.g. UI) changed it.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);

    s_drawCallCount = 0;
}

void Renderer::EndFrame() {
    if (!s_initialized) {
        return;
    }

    // Step 1: Check for OpenGL errors
    CheckGLError(__FILE__, __LINE__);

    // Step 2: Swap buffers to present the rendered frame
    Window::SwapBuffers();
}

// ==========================================
// VIEWPORT AND CLEAR COLOR
// ==========================================

void Renderer::SetViewport(i32 x, i32 y, i32 width, i32 height) {
    glViewport(x, y, width, height);
}

void Renderer::SetClearColor(const Color& color) {
    s_clearColor = color;
    glClearColor(s_clearColor.r, s_clearColor.g, s_clearColor.b, s_clearColor.a);
}

Color Renderer::GetClearColor() {
    return s_clearColor;
}

// ==========================================
// ERROR CHECKING
// ==========================================

b8 Renderer::CheckGLError(const char* sourceFile, i32 sourceLine) {
    bool hasError = false;
    GLenum error;

    // Loop because multiple errors can be queued
    while ((error = glGetError()) != GL_NO_ERROR) {
        hasError = true;
        const char* errorStr = "UNKNOWN";
        switch (error) {
            case GL_INVALID_ENUM:                  errorStr = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 errorStr = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             errorStr = "INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorStr = "INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY:                 errorStr = "OUT_OF_MEMORY"; break;
            case GL_STACK_UNDERFLOW:               errorStr = "STACK_UNDERFLOW"; break;
            case GL_STACK_OVERFLOW:                errorStr = "STACK_OVERFLOW"; break;
        }
        DND_LOG_ERROR("GL Error: %s at %s:%d", errorStr, sourceFile, sourceLine);
    }

    return hasError;
}

// ==========================================
// INFO QUERIES
// ==========================================

const char* Renderer::GetOpenGLVersion() {
    return reinterpret_cast<const char*>(glGetString(GL_VERSION));
}

const char* Renderer::GetRendererString() {
    return reinterpret_cast<const char*>(glGetString(GL_RENDERER));
}

// ==========================================
// HIGH-LEVEL RENDERING
// ==========================================

void Renderer::SetCameraMatrices(const Mat4& view, const Mat4& projection) {
    s_viewMatrix = view;
    s_projectionMatrix = projection;

    // Initialize scene shader if not yet created
    if (s_sceneShader == nullptr) {
        const char* vertexSource = R"(
            #version 460 core
            layout (location = 0) in vec3 a_position;
            layout (location = 1) in vec3 a_normal;
            layout (location = 2) in vec2 a_uv;
            layout (location = 3) in vec4 a_color;

            uniform mat4 u_model;
            uniform mat4 u_view;
            uniform mat4 u_projection;

            out vec3 v_normal;
            out vec3 v_worldPos;
            out vec4 v_color;
            out vec2 v_uv;

            void main() {
                v_color = a_color;
                v_normal = mat3(u_model) * a_normal;
                v_worldPos = (u_model * vec4(a_position, 1.0)).xyz;
                v_uv = a_uv;
                gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
            }
        )";

        const char* fragmentSource = R"(
            #version 460 core
            in vec3 v_normal;
            in vec3 v_worldPos;
            in vec4 v_color;
            in vec2 v_uv;

            out vec4 frag_color;

            #define MAX_SPOTS 8
            #define MAX_BOXES 16
            uniform int   u_numSpots;
            uniform vec3  u_spotPos[MAX_SPOTS];
            uniform vec3  u_spotDir[MAX_SPOTS];
            uniform vec3  u_spotColor[MAX_SPOTS];
            uniform float u_spotCosInner[MAX_SPOTS];
            uniform float u_spotCosOuter[MAX_SPOTS];
            uniform float u_spotRange[MAX_SPOTS];
            uniform int   u_numBoxes;
            uniform vec3  u_boxMin[MAX_BOXES];
            uniform vec3  u_boxMax[MAX_BOXES];
            uniform vec3  u_ambient;
            uniform vec4  u_color;
            uniform sampler2D u_diffuseTexture;
            uniform int   u_useTexture;

            // Ray-cast occlusion (slab test): is origin -> origin+dir*maxDist blocked?
            bool inShadow(vec3 origin, vec3 dir, float maxDist) {
                for (int b = 0; b < u_numBoxes; ++b) {
                    vec3 inv = 1.0 / dir;
                    vec3 t0 = (u_boxMin[b] - origin) * inv;
                    vec3 t1 = (u_boxMax[b] - origin) * inv;
                    vec3 tlo = min(t0, t1);
                    vec3 thi = max(t0, t1);
                    float tNear = max(max(tlo.x, tlo.y), tlo.z);
                    float tFar  = min(min(thi.x, thi.y), thi.z);
                    if (tFar >= max(tNear, 0.0) && tNear > 0.003 && tNear < maxDist) {
                        return true;
                    }
                }
                return false;
            }

            void main() {
                vec3 N = normalize(v_normal);
                vec3 lit = u_ambient;

                // Per-fragment ray-cast lighting: each spotlight is a ray from the
                // light to this fragment; accumulate cone * distance falloff * diffuse,
                // skipping any light whose path to the fragment is occluded (shadow).
                for (int i = 0; i < u_numSpots; ++i) {
                    vec3 toLight = u_spotPos[i] - v_worldPos;
                    float dist = length(toLight);
                    vec3 L = toLight / max(dist, 1e-4);
                    float theta = dot(-L, normalize(u_spotDir[i]));
                    float denom = max(u_spotCosInner[i] - u_spotCosOuter[i], 1e-4);
                    float cone = clamp((theta - u_spotCosOuter[i]) / denom, 0.0, 1.0);
                    if (cone <= 0.0) continue;
                    float diff = max(dot(N, L), 0.0);
                    if (diff <= 0.0) continue;
                    vec3 origin = v_worldPos + N * 0.04; // bias off the surface
                    if (inShadow(origin, L, dist - 0.06)) continue;
                    float atten = clamp(1.0 - dist / u_spotRange[i], 0.0, 1.0);
                    atten *= atten;
                    lit += u_spotColor[i] * (diff * cone * atten);
                }

                vec3 baseColor = v_color.rgb * u_color.rgb;
                if (u_useTexture != 0) {
                    baseColor *= texture(u_diffuseTexture, v_uv).rgb;
                }
                vec3 base = pow(baseColor, vec3(2.2));
                vec3 color = pow(base * lit, vec3(1.0 / 2.2));
                frag_color = vec4(color, v_color.a * u_color.a);
            }
        )";

        s_sceneShader = new Shader();
        if (!s_sceneShader->Compile(vertexSource, fragmentSource)) {
            DND_LOG_FATAL("Failed to compile scene shader");
        }
    }

    // Bind shader and set camera/lighting uniforms
    s_sceneShader->Bind();
    s_sceneShader->SetUniform("u_view", s_viewMatrix);
    s_sceneShader->SetUniform("u_projection", s_projectionMatrix);
    s_sceneShader->SetUniform("u_ambient", s_ambient);

    // Upload the spotlights consumed by the per-fragment ray-cast lighting.
    s_sceneShader->SetUniform("u_numSpots", s_numSpotlights);
    for (i32 i = 0; i < s_numSpotlights; ++i) {
        char n[48];
        std::snprintf(n, sizeof(n), "u_spotPos[%d]", i);      s_sceneShader->SetUniform(n, s_spotlights[i].position);
        std::snprintf(n, sizeof(n), "u_spotDir[%d]", i);      s_sceneShader->SetUniform(n, s_spotlights[i].direction);
        std::snprintf(n, sizeof(n), "u_spotColor[%d]", i);    s_sceneShader->SetUniform(n, s_spotlights[i].color);
        std::snprintf(n, sizeof(n), "u_spotCosInner[%d]", i); s_sceneShader->SetUniform(n, s_spotlights[i].cosInner);
        std::snprintf(n, sizeof(n), "u_spotCosOuter[%d]", i); s_sceneShader->SetUniform(n, s_spotlights[i].cosOuter);
        std::snprintf(n, sizeof(n), "u_spotRange[%d]", i);    s_sceneShader->SetUniform(n, s_spotlights[i].range);
    }

    // Bind texture unit 0 for diffuse textures
    s_sceneShader->SetUniform("u_diffuseTexture", 0);

    // Upload occluder boxes for ray-cast shadows.
    s_sceneShader->SetUniform("u_numBoxes", s_numBoxes);
    for (i32 i = 0; i < s_numBoxes; ++i) {
        char n[48];
        std::snprintf(n, sizeof(n), "u_boxMin[%d]", i); s_sceneShader->SetUniform(n, s_boxMin[i]);
        std::snprintf(n, sizeof(n), "u_boxMax[%d]", i); s_sceneShader->SetUniform(n, s_boxMax[i]);
    }
}

void Renderer::DrawMesh(const Mesh* mesh, const Mat4& model, const Color& color,
                        const Texture2D* texture) {
    if (mesh == nullptr || s_sceneShader == nullptr) {
        return;
    }

    s_sceneShader->Bind();
    s_sceneShader->SetUniform("u_model", model);
    s_sceneShader->SetUniform("u_color", color);

    s_sceneShader->SetUniform("u_diffuseTexture", 0);
    if (texture && texture->IsLoaded()) {
        texture->Bind(0);
        s_sceneShader->SetUniform("u_useTexture", 1);
    } else {
        s_sceneShader->SetUniform("u_useTexture", 0);
    }

    mesh->Bind();
    if (mesh->GetIndexCount() > 0) {
        mesh->DrawIndexed();
    } else {
        mesh->Draw();
    }
    mesh->Unbind();
    s_drawCallCount++;
}

void Renderer::SetSpotlights(const Spotlight* spotlights, i32 count) {
    if (spotlights == nullptr || count <= 0) { s_numSpotlights = 0; return; }
    if (count > MAX_SPOTLIGHTS) count = MAX_SPOTLIGHTS;
    for (i32 i = 0; i < count; ++i) {
        s_spotlights[i] = spotlights[i];
    }
    s_numSpotlights = count;
}

void Renderer::DrawSpotlightBeams() {
    if (s_numSpotlights == 0) {
        return;
    }

    // Lazily build the beam shader (unlit; alpha fades from apex toward the floor).
    if (s_beamShader == nullptr) {
        const char* vs = R"(
            #version 460 core
            layout(location = 0) in vec3 a_position;
            uniform mat4 u_model;
            uniform mat4 u_view;
            uniform mat4 u_projection;
            out float v_h;
            out vec3 v_world;
            void main() {
                v_h = -a_position.y; // 0 at apex, 1 at base
                vec4 wp = u_model * vec4(a_position, 1.0);
                v_world = wp.xyz;
                gl_Position = u_projection * u_view * wp;
            }
        )";
        const char* fs = R"(
            #version 460 core
            in float v_h;
            in vec3 v_world;
            out vec4 frag_color;
            uniform vec3  u_beamColor;
            uniform float u_beamAlpha;
            uniform vec3  u_beamLightPos;
            #define MAX_BOXES 16
            uniform int   u_numBoxes;
            uniform vec3  u_boxMin[MAX_BOXES];
            uniform vec3  u_boxMax[MAX_BOXES];

            bool blocked(vec3 origin, vec3 dir, float maxDist) {
                for (int b = 0; b < u_numBoxes; ++b) {
                    vec3 inv = 1.0 / dir;
                    vec3 t0 = (u_boxMin[b] - origin) * inv;
                    vec3 t1 = (u_boxMax[b] - origin) * inv;
                    vec3 tlo = min(t0, t1);
                    vec3 thi = max(t0, t1);
                    float tn = max(max(tlo.x, tlo.y), tlo.z);
                    float tf = min(min(thi.x, thi.y), thi.z);
                    if (tf >= max(tn, 0.0) && tn > 0.003 && tn < maxDist) return true;
                }
                return false;
            }

            void main() {
                // Volumetric occlusion: a point in the shaft that can't see the light
                // (a cube blocks the path) adds no glow -> the shaft is cut behind cubes.
                vec3 toLight = u_beamLightPos - v_world;
                float d = length(toLight);
                vec3 L = toLight / max(d, 1e-4);
                float vis = blocked(v_world, L, d - 0.04) ? 0.0 : 1.0;
                float a = u_beamAlpha * (1.0 - v_h * 0.85) * vis; // brightest near the light
                frag_color = vec4(u_beamColor, a);
            }
        )";
        s_beamShader = new Shader();
        if (!s_beamShader->Compile(vs, fs)) {
            DND_LOG_FATAL("Failed to compile beam shader");
            return;
        }
    }

    // Lazily build a unit cone: apex at origin, base ring (radius 1) at y = -1.
    if (s_beamCone == nullptr) {
        const i32 SEG = 24;
        Vertex verts[SEG + 1];
        verts[0] = Vertex(Vec3(0.0f, 0.0f, 0.0f));
        for (i32 k = 0; k < SEG; ++k) {
            f32 ang = (static_cast<f32>(k) / static_cast<f32>(SEG)) * 6.2831853f;
            verts[1 + k] = Vertex(Vec3(std::cos(ang), -1.0f, std::sin(ang)));
        }
        u32 idx[SEG * 3];
        for (i32 k = 0; k < SEG; ++k) {
            idx[k * 3 + 0] = 0u;
            idx[k * 3 + 1] = static_cast<u32>(1 + k);
            idx[k * 3 + 2] = static_cast<u32>(1 + (k + 1) % SEG);
        }
        s_beamCone = new Mesh();
        s_beamCone->Create(verts, static_cast<i32>(SEG + 1), idx, static_cast<i32>(SEG * 3));
    }

    // Additive, translucent, no depth writes, both cone faces visible.
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    s_beamShader->Bind();
    s_beamShader->SetUniform("u_view", s_viewMatrix);
    s_beamShader->SetUniform("u_projection", s_projectionMatrix);

    // Occluders for volumetric beam shadowing (same boxes the scene shader uses).
    s_beamShader->SetUniform("u_numBoxes", s_numBoxes);
    for (i32 b = 0; b < s_numBoxes; ++b) {
        char bn[48];
        std::snprintf(bn, sizeof(bn), "u_boxMin[%d]", b); s_beamShader->SetUniform(bn, s_boxMin[b]);
        std::snprintf(bn, sizeof(bn), "u_boxMax[%d]", b); s_beamShader->SetUniform(bn, s_boxMax[b]);
    }

    for (i32 i = 0; i < s_numSpotlights; ++i) {
        const Spotlight& sp = s_spotlights[i];
        const Vec3 dir = sp.direction; // normalized, pointing away from the light (down-ish)
        if (dir.y >= -0.001f) {
            continue; // never reaches the floor
        }
        const f32 length = sp.position.y / (-dir.y);        // distance along the ray to y=0
        const f32 angle = std::acos(sp.cosOuter);           // cone half-angle
        const f32 radius = length * std::tan(angle) * 1.05f;
        // Orient the unit cone (local -Y axis) onto the light direction so the
        // shaft leans with the angled light instead of dropping straight down.
        const Quat rot = Quat::FromToRotation(Vec3(0.0f, -1.0f, 0.0f), dir);
        const Mat4 model = Mat4::Translate(sp.position) * rot.ToMatrix() *
                           Mat4::Scale(Vec3(radius, length, radius));
        s_beamShader->SetUniform("u_model", model);
        s_beamShader->SetUniform("u_beamColor", sp.color);
        s_beamShader->SetUniform("u_beamAlpha", 0.16f);
        s_beamShader->SetUniform("u_beamLightPos", sp.position);
        s_beamCone->Bind();
        s_beamCone->DrawIndexed();
        s_beamCone->Unbind();
        s_drawCallCount++;
    }

    // Restore default render state.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
}

void Renderer::SetOccluders(const Vec3* mins, const Vec3* maxs, i32 count) {
    if (count < 0) count = 0;
    if (count > MAX_BOXES) count = MAX_BOXES;
    for (i32 i = 0; i < count; ++i) {
        s_boxMin[i] = mins[i];
        s_boxMax[i] = maxs[i];
    }
    s_numBoxes = count;
}

} // namespace Dnd
