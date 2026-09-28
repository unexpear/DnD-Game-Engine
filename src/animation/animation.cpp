/**
 * @file animation.cpp
 * @brief Implementation of Animation component.
 * 
 * See animation.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.5.0
 */

#include "animation/animation.h"
#include "animation/animation_clip.h"
#include "core/math.h"

namespace Dnd {

void Animation::Update(f32 deltaTime) {
    if (!isPlaying || !clip) return;

    // Advance time
    time += deltaTime * speed;

    // Handle end of animation
    if (time >= clip->GetDuration()) {
        if (loop) {
            // Loop: wrap around
            time = fmodf(time, clip->GetDuration());
        } else {
            // Stop at end
            time = clip->GetDuration();
            isPlaying = false;
        }
    }
}

b8 Animation::IsFinished() const {
    if (!clip) return true;
    return !loop && time >= clip->GetDuration();
}

} // namespace Dnd
