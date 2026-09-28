/**
 * @file mesh.h
 * @brief OpenGL mesh abstraction: VAO, VBO, IBO management.
 * 
 * Handles vertex buffer creation, vertex attribute configuration,
 * and indexed drawing for 3D geometry.
 * 
 * Features:
 *   - Vertex buffer (VBO) for vertex data
 *   - Index buffer (IBO/EBO) for indexed drawing
 *   - Vertex Array Object (VAO) for attribute layout
 *   - Configurable vertex format (position, normal, UV, color)
 *   - Draw call abstraction (Draw, DrawIndexed)
 * 
 * Vertex format:
 *   - Position: Vec3 (3 floats)
 *   - Normal: Vec3 (3 floats)
 *   - UV: Vec2 (2 floats)
 *   - Color: Vec4 (4 floats)
 *   - Total: 12 floats per vertex (48 bytes)
 * 
 * @note Meshes are NOT automatically cleaned up on destruction.
 *       Call Destroy() to free OpenGL resources.
 * @warning All OpenGL calls require an active context.
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
// STRUCT: Vertex
// ==========================================

/**
 * @struct Vertex
 * @brief Complete vertex data for rendering.
 * 
 * Contains all attributes needed for a vertex in the rendering pipeline:
 * position (for geometry), normal (for lighting), UV (for textures),
 * and color (for tinting/debugging).
 * 
 * Memory layout: [posX, posY, posZ, normX, normY, normZ, u, v, r, g, b, a]
 * Total size: 12 * 4 = 48 bytes per vertex.
 */
struct Vertex {
    /** @brief Vertex position in local/model space. */
    Vec3 position;
    /** @brief Vertex normal in local space. Used for lighting calculations. */
    Vec3 normal;
    /** @brief UV texture coordinates. U = horizontal, V = vertical. Range: [0, 1]. */
    Vec2 uv;
    /** @brief Vertex color (RGBA). Used for tinting and debug visualization. */
    Vec4 color;

    /** @brief Default constructor. Zeroes all values. */
    Vertex();

    /**
     * @brief Creates a vertex with position and default normal/UV/color.
     * @param pos Vertex position.
     */
    Vertex(const Vec3& pos);
};

// ==========================================
// CLASS: Mesh
// ==========================================

/**
 * @class Mesh
 * @brief OpenGL mesh with vertex and index buffers.
 * 
 * Manages GPU-side vertex and index data for rendering geometry.
 * Supports both indexed and non-indexed drawing.
 * 
 * Usage:
 * @code
 * Mesh mesh;
 * Vertex vertices[] = { ... };
 * u32 indices[] = { ... };
 * mesh.Create(vertices, 3, indices, 3);
 * 
 * // Each frame:
 * mesh.Bind();
 * mesh.DrawIndexed();
 * mesh.Unbind();
 * @endcode
 */
class Mesh {
public:
    /** @brief Default constructor. Creates an empty mesh. */
    Mesh();

    /**
     * @brief Destructor. Destroys OpenGL resources if not already destroyed.
     */
    ~Mesh();

    // No copy — OpenGL resources can't be safely copied
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Move allowed
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    /**
     * @brief Creates the mesh with vertex and index data.
     * 
     * Implementation:
     *   1. If already created, destroy existing resources first
     *   2. Store vertex count and index count
     *   3. Generate and bind VAO
     *   4. Generate and bind VBO:
     *      - Copy vertex data to GPU with glBufferData(GL_ARRAY_BUFFER)
     *      - Configure vertex attributes:
     *        * Position: location 0, 3 floats, offset 0
     *        * Normal:   location 1, 3 floats, offset 12
     *        * UV:       location 2, 2 floats, offset 24
     *        * Color:    location 3, 4 floats, offset 32
     *      - Enable all attribute arrays
     *   5. Generate and bind IBO:
     *      - Copy index data to GPU with glBufferData(GL_ELEMENT_ARRAY_BUFFER)
     *   6. Unbind VAO
     * 
     * @param vertices Array of vertex data.
     * @param vertexCount Number of vertices in the array.
     * @param indices Array of vertex indices. Can be nullptr for non-indexed meshes.
     * @param indexCount Number of indices. 0 if indices is nullptr.
     */
    void Create(const Vertex* vertices, i32 vertexCount, const u32* indices, i32 indexCount);

    /**
     * @brief Destroys the OpenGL resources.
     * 
     * Implementation:
     *   1. Delete VAO, VBO, IBO with glDelete*()
     *   2. Set IDs to 0, reset counts
     */
    void Destroy();

    /**
     * @brief Binds the mesh for rendering.
     * 
     * Implementation:
     *   1. Bind the VAO with glBindVertexArray()
     * 
     * @note Call before Draw() or DrawIndexed().
     */
    void Bind() const;

    /**
     * @brief Unbinds the mesh.
     * 
     * Implementation:
     *   1. Unbind the VAO with glBindVertexArray(0)
     */
    void Unbind() const;

    /**
     * @brief Draws the mesh without indices.
     * 
     * Implementation:
     *   1. Call glDrawArrays(GL_TRIANGLES, 0, vertexCount)
     * 
     * @note Only works if the mesh was created without indices.
     *       Vertices must be ordered for triangle lists.
     */
    void Draw() const;

    /**
     * @brief Draws the mesh with indices.
     * 
     * Implementation:
     *   1. Call glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0)
     * 
     * @note Only works if the mesh was created with indices.
     */
    void DrawIndexed() const;

    /** @brief Gets the number of vertices. */
    i32 GetVertexCount() const;

    /** @brief Gets the number of indices. Returns 0 if non-indexed. */
    i32 GetIndexCount() const;

    /** @brief Checks if this mesh has been created. */
    b8 IsCreated() const;

    // ==========================================
    // BUILT-IN MESH GENERATORS
    // ==========================================

    /**
     * @brief Creates a unit cube mesh centered at origin.
     * 
     * Dimensions: 1x1x1, from (-0.5, -0.5, -0.5) to (0.5, 0.5, 0.5).
     * Normals face outward. UVs span [0,1] on each face.
     */
    static Mesh CreateCube();

    /**
     * @brief Creates a flat quad mesh in the XZ plane.
     * 
     * Dimensions: width x depth, centered at origin.
     * Y = 0. Normal = (0, 1, 0). UVs span [0,1].
     * 
     * @param width Width along X axis.
     * @param depth Depth along Z axis.
     */
    static Mesh CreateQuad(f32 width, f32 depth);

    /**
     * @brief Creates a triangle mesh (the simplest 3D shape).
     * 
     * Vertices: (0, 1, 0), (-1, -1, 0), (1, -1, 0).
     * Useful for testing the rendering pipeline.
     */
    static Mesh CreateTriangle();

private:
    /** @brief Vertex Array Object ID. */
    u32 m_vao;
    /** @brief Vertex Buffer Object ID. */
    u32 m_vbo;
    /** @brief Index Buffer Object ID. 0 if non-indexed. */
    u32 m_ibo;
    /** @brief Number of vertices. */
    i32 m_vertexCount;
    /** @brief Number of indices. 0 if non-indexed. */
    i32 m_indexCount;
    /** @brief Whether this mesh has been created. */
    b8 m_created;
};

} // namespace Dnd
