#include "rendering/texture.h"
#include "core/log.h"
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cstring>

namespace Dnd {

Texture2D::Texture2D() {}

Texture2D::~Texture2D() {
    Destroy();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_glId(other.m_glId), m_width(other.m_width), m_height(other.m_height), m_loaded(other.m_loaded) {
    other.m_glId = 0;
    other.m_loaded = false;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        Destroy();
        m_glId = other.m_glId;
        m_width = other.m_width;
        m_height = other.m_height;
        m_loaded = other.m_loaded;
        other.m_glId = 0;
        other.m_loaded = false;
    }
    return *this;
}

bool Texture2D::LoadFromFile(const std::string& path) {
    Destroy();

    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data) {
        DND_LOG_ERROR("Failed to load texture: %s (%s)", path.c_str(), stbi_failure_reason());
        return false;
    }

    glGenTextures(1, &m_glId);
    glBindTexture(GL_TEXTURE_2D, m_glId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    m_width = static_cast<u32>(w);
    m_height = static_cast<u32>(h);
    m_loaded = true;

    stbi_image_free(data);
    DND_LOG_DEBUG("Loaded texture: %s (%dx%d)", path.c_str(), w, h);
    return true;
}

void Texture2D::CreateCheckerboard(u32 size, u32 gridSize, u32 color1, u32 color2) {
    Destroy();

    u32 numPixels = size * size;
    unsigned char* data = new unsigned char[numPixels * 4];

    for (u32 y = 0; y < size; ++y) {
        for (u32 x = 0; x < size; ++x) {
            u32 idx = (y * size + x) * 4;
            bool light = ((x / gridSize) + (y / gridSize)) % 2 == 0;
            u32 col = light ? color1 : color2;
            data[idx + 0] = static_cast<unsigned char>((col >> 24) & 0xFF);
            data[idx + 1] = static_cast<unsigned char>((col >> 16) & 0xFF);
            data[idx + 2] = static_cast<unsigned char>((col >> 8) & 0xFF);
            data[idx + 3] = static_cast<unsigned char>(col & 0xFF);
        }
    }

    glGenTextures(1, &m_glId);
    glBindTexture(GL_TEXTURE_2D, m_glId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<i32>(size), static_cast<i32>(size), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_width = size;
    m_height = size;
    m_loaded = true;

    delete[] data;
}

void Texture2D::Destroy() {
    if (m_glId != 0) {
        glDeleteTextures(1, &m_glId);
        m_glId = 0;
    }
    m_width = 0;
    m_height = 0;
    m_loaded = false;
}

void Texture2D::Bind(u32 unit) const {
    if (m_glId != 0) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_glId);
    }
}

void Texture2D::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace Dnd
