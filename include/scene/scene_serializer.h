#pragma once

#include "core/types.h"
#include <string>

namespace Dnd {

class Scene;

class SceneSerializer {
public:
    static bool Save(Scene& scene, const char* path);
    static bool Load(Scene& scene, const char* path);
};

} // namespace Dnd
