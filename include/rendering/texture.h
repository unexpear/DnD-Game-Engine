#pragma once

#include "core/types.h"
#include <string>

namespace Dnd {

class Texture2D {
public:
    Texture2D();
    ~Texture2D();

    bool LoadFromFile(const std::string& path);
    void CreateCheckerboard(u32 size, u32 gridSize, u32 color1, u32 color2);
    void Destroy();

    void Bind(u32 unit = 0) const;
    void Unbind() const;

    u32 GetWidth() const { return m_width; }
    u32 GetHeight() const { return m_height; }
    u32 GetGLId() const { return m_glId; }
    bool IsLoaded() const { return m_loaded; }

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

private:
    u32 m_glId = 0;
    u32 m_width = 0;
    u32 m_height = 0;
    bool m_loaded = false;
};

} // namespace Dnd
