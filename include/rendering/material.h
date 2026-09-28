#pragma once

#include "core/types.h"
#include "core/math.h"
#include "assets/asset_manager.h"

namespace Dnd {

struct Material {
    Color diffuseColor = Color(1, 1, 1, 1);
    f32 shininess = 32.0f;

    AssetHandle diffuseTexture;
};

} // namespace Dnd
