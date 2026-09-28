/**
 * @file shader.cpp
 * @brief Implementation of the Shader class.
 * 
 * See shader.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "rendering/shader.h"
#include "core/log.h"
#include <glad/glad.h>
#include <cstring>

namespace Dnd {

// ==========================================
// CONSTRUCTION / DESTRUCTION
// ==========================================

/**
 * @brief Default constructor.
 * 
 * Initializes program ID to 0 (invalid) and compiled flag to false.
 */
Shader::Shader()
    : m_programId(0)
    , m_compiled(false)
    , m_uniformCount(0)
{
    // Initialize uniform cache arrays
    std::memset(m_uniformNames, 0, sizeof(m_uniformNames));
    std::memset(m_uniformLocations, -1, sizeof(m_uniformLocations));
}

/**
 * @brief Destructor. Deletes the OpenGL program if valid.
 */
Shader::~Shader() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        DND_LOG_DEBUG("Shader program %u deleted", m_programId);
    }
}

// Move constructor: transfers ownership of the OpenGL program
Shader::Shader(Shader&& other) noexcept
    : m_programId(other.m_programId)
    , m_compiled(other.m_compiled)
    , m_uniformCount(other.m_uniformCount)
{
    // Invalidate the source so it doesn't delete our program
    other.m_programId = 0;
    other.m_compiled = false;
    other.m_uniformCount = 0;

    // Copy uniform cache
    std::memcpy(m_uniformNames, other.m_uniformNames, sizeof(m_uniformNames));
    std::memcpy(m_uniformLocations, other.m_uniformLocations, sizeof(m_uniformLocations));
}

// Move assignment: transfers ownership
Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        // Delete our existing program
        if (m_programId != 0) {
            glDeleteProgram(m_programId);
        }

        // Take ownership of other's program
        m_programId = other.m_programId;
        m_compiled = other.m_compiled;
        m_uniformCount = other.m_uniformCount;
        std::memcpy(m_uniformNames, other.m_uniformNames, sizeof(m_uniformNames));
        std::memcpy(m_uniformLocations, other.m_uniformLocations, sizeof(m_uniformLocations));

        // Invalidate source
        other.m_programId = 0;
        other.m_compiled = false;
        other.m_uniformCount = 0;
    }
    return *this;
}

// ==========================================
// COMPILATION
// ==========================================

b8 Shader::Compile(const char* vertexSource, const char* fragmentSource) {
    if (vertexSource == nullptr || fragmentSource == nullptr) {
        DND_LOG_ERROR("Shader::Compile received null source pointer");
        return false;
    }

    // Step 1-4: Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    // Check vertex shader compilation status
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        // Get the compilation error log
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        DND_LOG_ERROR("Vertex shader compilation failed: %s", infoLog);
        glDeleteShader(vertexShader);
        return false;
    }

    // Step 5-8: Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    // Check fragment shader compilation status
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        DND_LOG_ERROR("Fragment shader compilation failed: %s", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Step 6-9: Create program, attach shaders, link
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check program link status
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        DND_LOG_ERROR("Shader program linking failed: %s", infoLog);
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Step 10: Delete shader objects (they're now part of the program)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Step 11-12: Store program ID, reset uniform cache
    m_programId = program;
    m_compiled = true;
    m_uniformCount = 0;
    std::memset(m_uniformNames, 0, sizeof(m_uniformNames));
    std::memset(m_uniformLocations, -1, sizeof(m_uniformLocations));

    DND_LOG_INFO("Shader compiled and linked successfully (program ID: %u)", m_programId);
    return true;
}

// ==========================================
// BINDING
// ==========================================

void Shader::Bind() const {
    if (m_programId != 0) {
        glUseProgram(m_programId);
    }
}

void Shader::Unbind() const {
    glUseProgram(0);
}

u32 Shader::GetProgramId() const {
    return m_programId;
}

b8 Shader::IsCompiled() const {
    return m_compiled;
}

// ==========================================
// UNIFORM SETTERS
// ==========================================

void Shader::SetUniform(const char* name, i32 value) {
    i32 location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void Shader::SetUniform(const char* name, f32 value) {
    i32 location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void Shader::SetUniform(const char* name, const Vec3& value) {
    i32 location = GetUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, value.x, value.y, value.z);
    }
}

void Shader::SetUniform(const char* name, const Vec4& value) {
    i32 location = GetUniformLocation(name);
    if (location != -1) {
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }
}

void Shader::SetUniform(const char* name, const Color& color) {
    i32 location = GetUniformLocation(name);
    if (location != -1) {
        glUniform4f(location, color.r, color.g, color.b, color.a);
    }
}

void Shader::SetUniform(const char* name, const Mat4& matrix) {
    i32 location = GetUniformLocation(name);
    if (location != -1) {
        // GL_FALSE for transpose because our Mat4 is already column-major
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix.m);
    }
}

// ==========================================
// UNIFORM LOCATION CACHING
// ==========================================

i32 Shader::GetUniformLocation(const char* name) {
    // Step 1: Search the cache for this uniform name
    for (i32 i = 0; i < m_uniformCount; i++) {
        if (std::strcmp(m_uniformNames[i], name) == 0) {
            return m_uniformLocations[i]; // Cache hit
        }
    }

    // Step 2: Cache miss — query OpenGL
    if (m_uniformCount >= MAX_UNIFORMS) {
        DND_LOG_WARN("Shader uniform cache full (%d). Querying glGetUniformLocation directly.", MAX_UNIFORMS);
        // Fall back to direct query without caching
        return glGetUniformLocation(m_programId, name);
    }

    // Query the uniform location
    i32 location = glGetUniformLocation(m_programId, name);

    // Step 3: Cache the result
    i32 index = m_uniformCount;
    std::strncpy(m_uniformNames[index], name, 63);
    m_uniformNames[index][63] = '\0'; // Ensure null termination
    m_uniformLocations[index] = location;
    m_uniformCount++;

    // Step 4: Warn if uniform not found (might be optimized out by GLSL compiler)
    if (location == -1) {
        DND_LOG_DEBUG("Uniform '%s' not found in shader program %u (may be unused)", name, m_programId);
    }

    return location;
}

} // namespace Dnd
