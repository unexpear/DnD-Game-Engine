/**
 * @file shader.h
 * @brief GLSL shader compilation, linking, and uniform management.
 * 
 * Handles loading vertex and fragment shader source code from strings,
 * compiling them, linking them into a shader program, and managing
 * uniform variable locations and values.
 * 
 * Features:
 *   - Compile vertex and fragment shaders from source strings
 *   - Link into a shader program
 *   - Query and cache uniform locations
 *   - Set uniform values (int, float, Vec3, Vec4, Mat4)
 *   - Automatic uniform location caching (query once, store for reuse)
 * 
 * @note Shader source code is passed as strings, not loaded from files.
 *       File loading is handled by the asset system (future phase).
 * @warning Always check compilation/linking success in logs.
 *          Failed shaders produce empty or incorrect output silently.
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
// CLASS: Shader
// ==========================================

/**
 * @class Shader
 * @brief OpenGL shader program wrapper.
 * 
 * Manages the lifecycle of a GLSL shader program: compilation of vertex
 * and fragment shaders, linking, uniform management, and cleanup.
 * 
 * Usage:
 * @code
 * Shader shader;
 * shader.Compile(vertexSource, fragmentSource);
 * 
 * shader.Bind();
 * shader.SetUniform("u_projection", projectionMatrix);
 * shader.SetUniform("u_view", viewMatrix);
 * shader.SetUniform("u_color", Color::Red());
 * // Draw meshes...
 * shader.Unbind();
 * @endcode
 */
class Shader {
public:
    /** @brief Default constructor. Creates an empty shader (no program). */
    Shader();

    /**
     * @brief Destructor. Automatically deletes the OpenGL program.
     * 
     * Implementation:
     *   1. If program ID is valid (non-zero), delete it with glDeleteProgram()
     */
    ~Shader();

    // No copy — OpenGL resources can't be safely copied
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Move allowed — transfers ownership of the OpenGL program
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /**
     * @brief Compiles and links a shader program from source strings.
     * 
     * Implementation:
     *   1. Create a vertex shader object with glCreateShader(GL_VERTEX_SHADER)
     *   2. Set the vertex shader source with glShaderSource()
     *   3. Compile the vertex shader with glCompileShader()
     *   4. Check compilation status with glGetShaderiv(..., GL_COMPILE_STATUS)
     *      - If failed, get the info log with glGetShaderInfoLog() and log it
     *      - Delete the shader object, return false
     *   5. Repeat steps 1-4 for the fragment shader (GL_FRAGMENT_SHADER)
     *   6. Create a program object with glCreateProgram()
     *   7. Attach both shaders with glAttachShader()
     *   8. Link the program with glLinkProgram()
     *   9. Check link status with glGetProgramiv(..., GL_LINK_STATUS)
     *      - If failed, get the info log and log it, clean up, return false
     *  10. Delete the shader objects (they're now linked into the program)
     *  11. Store the program ID, mark as compiled
     *  12. Clear the uniform location cache
     * 
     * @param vertexSource GLSL vertex shader source code.
     * @param fragmentSource GLSL fragment shader source code.
     * @return true if compilation and linking succeeded, false otherwise.
     * @post Shader is ready to use. Bind before setting uniforms.
     */
    b8 Compile(const char* vertexSource, const char* fragmentSource);

    /**
     * @brief Binds this shader program as the active program.
     * 
     * Implementation:
     *   1. Call glUseProgram(m_programId)
     * 
     * @note Must be called before setting uniforms or drawing.
     */
    void Bind() const;

    /**
     * @brief Unbinds the current shader program.
     * 
     * Implementation:
     *   1. Call glUseProgram(0) — binds the default (null) program
     */
    void Unbind() const;

    /**
     * @brief Gets the OpenGL program ID.
     * @return The program ID for direct OpenGL calls when needed.
     */
    u32 GetProgramId() const;

    /**
     * @brief Checks if this shader has been successfully compiled.
     * @return true if Compile() was called and succeeded.
     */
    b8 IsCompiled() const;

    // ==========================================
    // UNIFORM SETTERS
    // ==========================================

    /**
     * @brief Sets a uniform integer value.
     * 
     * Implementation:
     *   1. Get the uniform location (cached after first query)
     *   2. Call glUniform1i(location, value)
     * 
     * @param name Uniform variable name in the GLSL shader (e.g., "u_texture").
     * @param value Integer value to set.
     * @note Shader must be bound before calling this.
     */
    void SetUniform(const char* name, i32 value);

    /**
     * @brief Sets a uniform float value.
     * @param name Uniform variable name.
     * @param value Float value to set.
     */
    void SetUniform(const char* name, f32 value);

    /**
     * @brief Sets a uniform Vec3 value.
     * @param name Uniform variable name.
     * @param value Vec3 value to set.
     */
    void SetUniform(const char* name, const Vec3& value);

    /**
     * @brief Sets a uniform Vec4 value.
     * @param name Uniform variable name.
     * @param value Vec4 value to set.
     */
    void SetUniform(const char* name, const Vec4& value);

    /**
     * @brief Sets a uniform Color value (converted to Vec4).
     * @param name Uniform variable name.
     * @param color Color value to set (r, g, b, a).
     */
    void SetUniform(const char* name, const Color& color);

    /**
     * @brief Sets a uniform Mat4 value.
     * 
     * Implementation:
     *   1. Get uniform location
     *   2. Call glUniformMatrix4fv with GL_FALSE for transpose
     *      - GL_FALSE because our Mat4 is already column-major
     *      - OpenGL expects column-major matrices natively
     * 
     * @param name Uniform variable name.
     * @param matrix Mat4 value to set.
     */
    void SetUniform(const char* name, const Mat4& matrix);

private:
    /**
     * @brief Gets the cached uniform location, querying OpenGL if not yet cached.
     * 
     * Implementation:
     *   1. Search the uniform cache for the given name
     *   2. If found, return the cached location
     *   3. If not found, query with glGetUniformLocation()
     *      - If location == -1, log a warning (uniform doesn't exist or is unused)
     *   4. Store in cache and return
     * 
     * @param name Uniform variable name.
     * @return Uniform location integer. -1 if the uniform doesn't exist.
     */
    i32 GetUniformLocation(const char* name);

    /** @brief OpenGL program ID. 0 if not compiled. */
    u32 m_programId;
    /** @brief Whether the shader has been successfully compiled. */
    b8 m_compiled;

    /** @brief Maximum number of cached uniforms. */
    static constexpr i32 MAX_UNIFORMS = 64;
    /** @brief Cached uniform names. */
    char m_uniformNames[MAX_UNIFORMS][64];
    /** @brief Cached uniform locations. */
    i32 m_uniformLocations[MAX_UNIFORMS];
    /** @brief Number of cached uniforms. */
    i32 m_uniformCount;
};

} // namespace Dnd
