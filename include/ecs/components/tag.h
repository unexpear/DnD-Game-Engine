#pragma once

#include "core/types.h"
#include <string>

namespace Dnd {

struct Tag {
    const char* name = "";
    u32 nameHash = 0;

    Tag() = default;

    explicit Tag(const char* tagName) {
        SetName(tagName);
    }

    Tag(const Tag& other) {
        SetName(other.name);
    }

    Tag(Tag&& other) noexcept
        : m_nameStorage(std::move(other.m_nameStorage))
        , nameHash(other.nameHash)
    {
        name = m_nameStorage.c_str();
        other.name = "";
        other.nameHash = 0;
    }

    Tag& operator=(const Tag& other) {
        if (this != &other) {
            SetName(other.name);
        }
        return *this;
    }

    Tag& operator=(Tag&& other) noexcept {
        if (this != &other) {
            m_nameStorage = std::move(other.m_nameStorage);
            name = m_nameStorage.c_str();
            nameHash = other.nameHash;
            other.name = "";
            other.nameHash = 0;
        }
        return *this;
    }

    void SetName(const char* n) {
        m_nameStorage = n ? n : "";
        name = m_nameStorage.c_str();
        nameHash = HashFNV1a(name);
    }

    void SetName(const std::string& n) {
        m_nameStorage = n;
        name = m_nameStorage.c_str();
        nameHash = HashFNV1a(name);
    }

private:
    std::string m_nameStorage;

    static u32 HashFNV1a(const char* str) {
        u32 hash = 0x811C9DC5;
        while (*str) {
            hash ^= static_cast<u32>(*str++);
            hash *= 0x01000193;
        }
        return hash;
    }
};

} // namespace Dnd
