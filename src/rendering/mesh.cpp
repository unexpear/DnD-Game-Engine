/**
 * @file mesh.cpp
 * @brief Implementation of the Mesh class and Vertex struct.
 * 
 * See mesh.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.1.0
 */

#include "rendering/mesh.h"
#include "core/log.h"
#include <glad/glad.h>
#include <cstring>

namespace Dnd {

// ==========================================
// VERTEX STRUCT
// ==========================================

/**
 * @brief Default constructor for Vertex.
 * 
 * Zeroes all attributes: position=(0,0,0), normal=(0,0,0), uv=(0,0), color=(0,0,0,0).
 */
Vertex::Vertex()
    : position(0, 0, 0)
    , normal(0, 0, 0)
    , uv(0, 0)
    , color(0, 0, 0, 1)
{
}

/**
 * @brief Creates a vertex with the given position.
 * 
 * Normal defaults to (0,1,0) (up), UV to (0,0), color to white (1,1,1,1).
 * @param pos Vertex position.
 */
Vertex::Vertex(const Vec3& pos)
    : position(pos)
    , normal(0, 1, 0)
    , uv(0, 0)
    , color(1, 1, 1, 1)
{
}

// ==========================================
// MESH CONSTRUCTION / DESTRUCTION
// ==========================================

/**
 * @brief Default constructor. Initializes all IDs to 0.
 */
Mesh::Mesh()
    : m_vao(0)
    , m_vbo(0)
    , m_ibo(0)
    , m_vertexCount(0)
    , m_indexCount(0)
    , m_created(false)
{
}

/**
 * @brief Destructor. Destroys OpenGL resources if they exist.
 */
Mesh::~Mesh() {
    if (m_created) {
        Destroy();
    }
}

// Move constructor: transfers OpenGL resource ownership
Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao)
    , m_vbo(other.m_vbo)
    , m_ibo(other.m_ibo)
    , m_vertexCount(other.m_vertexCount)
    , m_indexCount(other.m_indexCount)
    , m_created(other.m_created)
{
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ibo = 0;
    other.m_vertexCount = 0;
    other.m_indexCount = 0;
    other.m_created = false;
}

// Move assignment
Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        if (m_created) {
            Destroy();
        }
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ibo = other.m_ibo;
        m_vertexCount = other.m_vertexCount;
        m_indexCount = other.m_indexCount;
        m_created = other.m_created;

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ibo = 0;
        other.m_vertexCount = 0;
        other.m_indexCount = 0;
        other.m_created = false;
    }
    return *this;
}

// ==========================================
// MESH CREATION
// ==========================================

void Mesh::Create(const Vertex* vertices, i32 vertexCount, const u32* indices, i32 indexCount) {
    if (vertices == nullptr || vertexCount <= 0) {
        DND_LOG_ERROR("Mesh::Create called with null vertices or zero vertex count");
        return;
    }

    // Step 1: Destroy existing resources if already created
    if (m_created) {
        Destroy();
    }

    // Step 2: Store counts
    m_vertexCount = vertexCount;
    m_indexCount = indexCount;

    // Step 3: Generate and bind VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Step 4: Generate and bind VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

    // Configure vertex attributes
    // Each vertex: [pos(3) | normal(3) | uv(2) | color(4)] = 12 floats = 48 bytes
    constexpr i32 VERTEX_STRIDE = sizeof(Vertex);

    // Attribute 0: Position (Vec3) — offset 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, (void*)0);

    // Attribute 1: Normal (Vec3) — offset 12 bytes
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, (void*)12);

    // Attribute 2: UV (Vec2) — offset 24 bytes
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, (void*)24);

    // Attribute 3: Color (Vec4) — offset 32 bytes
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, (void*)32);

    // Step 5: Generate and bind IBO (if indexed)
    if (indices != nullptr && indexCount > 0) {
        glGenBuffers(1, &m_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(u32), indices, GL_STATIC_DRAW);
    } else {
        m_ibo = 0;
        m_indexCount = 0;
    }

    // Step 6: Unbind VAO
    glBindVertexArray(0);

    // Mark as created
    m_created = true;

    DND_LOG_DEBUG("Mesh created: %d vertices, %d indices (VAO: %u, VBO: %u, IBO: %u)",
        vertexCount, m_indexCount, m_vao, m_vbo, m_ibo);
}

// ==========================================
// MESH DESTRUCTION
// ==========================================

void Mesh::Destroy() {
    if (!m_created) {
        return;
    }

    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    if (m_ibo != 0) {
        glDeleteBuffers(1, &m_ibo);
    }

    m_vao = 0;
    m_vbo = 0;
    m_ibo = 0;
    m_vertexCount = 0;
    m_indexCount = 0;
    m_created = false;
}

// ==========================================
// BINDING AND DRAWING
// ==========================================

void Mesh::Bind() const {
    if (m_created) {
        glBindVertexArray(m_vao);
    }
}

void Mesh::Unbind() const {
    glBindVertexArray(0);
}

void Mesh::Draw() const {
    if (!m_created) {
        DND_LOG_WARN("Mesh::Draw called on uncreated mesh");
        return;
    }
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
}

void Mesh::DrawIndexed() const {
    if (!m_created || m_indexCount == 0) {
        DND_LOG_WARN("Mesh::DrawIndexed called on non-indexed or uncreated mesh");
        return;
    }
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
}

i32 Mesh::GetVertexCount() const { return m_vertexCount; }
i32 Mesh::GetIndexCount() const { return m_indexCount; }
b8 Mesh::IsCreated() const { return m_created; }

// ==========================================
// BUILT-IN MESH GENERATORS
// ==========================================

/**
 * @brief Creates a unit cube mesh.
 * 
 * 8 vertices, 36 indices (12 triangles = 6 faces x 2 triangles).
 * Each face has correct outward-facing normals.
 */
Mesh Mesh::CreateCube() {
    // 8 corner vertices of a unit cube centered at origin
    // Each vertex is duplicated for each face to have correct normals
    Vertex vertices[24]; // 4 vertices per face * 6 faces = 24 (one unique per face corner)
    u32 indices[36];     // 6 indices per face * 6 faces = 36

    // We'll define face-by-face for correct normals
    f32 h = 0.5f; // half-size

    // Front face (Z = +h, normal = +Z)
    vertices[0]  = Vertex(Vec3(-h, -h,  h)); vertices[0].normal = Vec3(0, 0, 1);  vertices[0].uv = Vec2(0, 0);
    vertices[1]  = Vertex(Vec3( h, -h,  h)); vertices[1].normal = Vec3(0, 0, 1);  vertices[1].uv = Vec2(1, 0);
    vertices[2]  = Vertex(Vec3( h,  h,  h)); vertices[2].normal = Vec3(0, 0, 1);  vertices[2].uv = Vec2(1, 1);
    vertices[3]  = Vertex(Vec3(-h,  h,  h)); vertices[3].normal = Vec3(0, 0, 1);  vertices[3].uv = Vec2(0, 1);

    // Back face (Z = -h, normal = -Z)
    vertices[4]  = Vertex(Vec3( h, -h, -h)); vertices[4].normal = Vec3(0, 0, -1); vertices[4].uv = Vec2(0, 0);
    vertices[5]  = Vertex(Vec3(-h, -h, -h)); vertices[5].normal = Vec3(0, 0, -1); vertices[5].uv = Vec2(1, 0);
    vertices[6]  = Vertex(Vec3(-h,  h, -h)); vertices[6].normal = Vec3(0, 0, -1); vertices[6].uv = Vec2(1, 1);
    vertices[7]  = Vertex(Vec3( h,  h, -h)); vertices[7].normal = Vec3(0, 0, -1); vertices[7].uv = Vec2(0, 1);

    // Top face (Y = +h, normal = +Y)
    vertices[8]  = Vertex(Vec3(-h,  h,  h)); vertices[8].normal = Vec3(0, 1, 0);  vertices[8].uv = Vec2(0, 0);
    vertices[9]  = Vertex(Vec3( h,  h,  h)); vertices[9].normal = Vec3(0, 1, 0);  vertices[9].uv = Vec2(1, 0);
    vertices[10] = Vertex(Vec3( h,  h, -h)); vertices[10].normal = Vec3(0, 1, 0); vertices[10].uv = Vec2(1, 1);
    vertices[11] = Vertex(Vec3(-h,  h, -h)); vertices[11].normal = Vec3(0, 1, 0); vertices[11].uv = Vec2(0, 1);

    // Bottom face (Y = -h, normal = -Y)
    vertices[12] = Vertex(Vec3(-h, -h, -h)); vertices[12].normal = Vec3(0, -1, 0); vertices[12].uv = Vec2(0, 0);
    vertices[13] = Vertex(Vec3( h, -h, -h)); vertices[13].normal = Vec3(0, -1, 0); vertices[13].uv = Vec2(1, 0);
    vertices[14] = Vertex(Vec3( h, -h,  h)); vertices[14].normal = Vec3(0, -1, 0); vertices[14].uv = Vec2(1, 1);
    vertices[15] = Vertex(Vec3(-h, -h,  h)); vertices[15].normal = Vec3(0, -1, 0); vertices[15].uv = Vec2(0, 1);

    // Right face (X = +h, normal = +X)
    vertices[16] = Vertex(Vec3( h, -h,  h)); vertices[16].normal = Vec3(1, 0, 0);  vertices[16].uv = Vec2(0, 0);
    vertices[17] = Vertex(Vec3( h, -h, -h)); vertices[17].normal = Vec3(1, 0, 0);  vertices[17].uv = Vec2(1, 0);
    vertices[18] = Vertex(Vec3( h,  h, -h)); vertices[18].normal = Vec3(1, 0, 0);  vertices[18].uv = Vec2(1, 1);
    vertices[19] = Vertex(Vec3( h,  h,  h)); vertices[19].normal = Vec3(1, 0, 0);  vertices[19].uv = Vec2(0, 1);

    // Left face (X = -h, normal = -X)
    vertices[20] = Vertex(Vec3(-h, -h, -h)); vertices[20].normal = Vec3(-1, 0, 0); vertices[20].uv = Vec2(0, 0);
    vertices[21] = Vertex(Vec3(-h, -h,  h)); vertices[21].normal = Vec3(-1, 0, 0); vertices[21].uv = Vec2(1, 0);
    vertices[22] = Vertex(Vec3(-h,  h,  h)); vertices[22].normal = Vec3(-1, 0, 0); vertices[22].uv = Vec2(1, 1);
    vertices[23] = Vertex(Vec3(-h,  h, -h)); vertices[23].normal = Vec3(-1, 0, 0); vertices[23].uv = Vec2(0, 1);

    // Indices: 2 triangles per face (CCW order for front-facing)
    // Even faces (0,2,4 = +Z,+Y,+X) use CCW; odd faces (1,3,5 = -Z,-Y,-X) reverse winding
    for (i32 i = 0; i < 6; i++) {
        i32 base = i * 4;
        if (i % 2 == 0) {
            indices[i * 6 + 0] = base + 0;
            indices[i * 6 + 1] = base + 1;
            indices[i * 6 + 2] = base + 2;
            indices[i * 6 + 3] = base + 0;
            indices[i * 6 + 4] = base + 2;
            indices[i * 6 + 5] = base + 3;
        } else {
            indices[i * 6 + 0] = base + 0;
            indices[i * 6 + 1] = base + 2;
            indices[i * 6 + 2] = base + 1;
            indices[i * 6 + 3] = base + 0;
            indices[i * 6 + 4] = base + 3;
            indices[i * 6 + 5] = base + 2;
        }
    }

    Mesh mesh;
    mesh.Create(vertices, 24, indices, 36);
    return mesh;
}

/**
 * @brief Creates a flat quad in the XZ plane.
 * @param width Width along X axis.
 * @param depth Depth along Z axis.
 */
Mesh Mesh::CreateQuad(f32 width, f32 depth) {
    f32 hw = width * 0.5f;
    f32 hd = depth * 0.5f;

    Vertex vertices[4];
    vertices[0] = Vertex(Vec3(-hw, 0,  hd)); vertices[0].normal = Vec3(0, 1, 0); vertices[0].uv = Vec2(0, 0);
    vertices[1] = Vertex(Vec3( hw, 0,  hd)); vertices[1].normal = Vec3(0, 1, 0); vertices[1].uv = Vec2(1, 0);
    vertices[2] = Vertex(Vec3( hw, 0, -hd)); vertices[2].normal = Vec3(0, 1, 0); vertices[2].uv = Vec2(1, 1);
    vertices[3] = Vertex(Vec3(-hw, 0, -hd)); vertices[3].normal = Vec3(0, 1, 0); vertices[3].uv = Vec2(0, 1);

    u32 indices[6] = { 0, 1, 2, 0, 2, 3 };

    Mesh mesh;
    mesh.Create(vertices, 4, indices, 6);
    return mesh;
}

/**
 * @brief Creates a simple triangle for testing.
 */
Mesh Mesh::CreateTriangle() {
    Vertex vertices[3];
    vertices[0] = Vertex(Vec3( 0,  1, 0)); vertices[0].color = Vec4(1, 0, 0, 1);
    vertices[1] = Vertex(Vec3(-1, -1, 0)); vertices[1].color = Vec4(0, 1, 0, 1);
    vertices[2] = Vertex(Vec3( 1, -1, 0)); vertices[2].color = Vec4(0, 0, 1, 1);

    Mesh mesh;
    mesh.Create(vertices, 3, nullptr, 0);
    return mesh;
}

} // namespace Dnd
