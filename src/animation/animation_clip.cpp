/**
 * @file animation_clip.cpp
 * @brief Implementation of AnimationClip and BoneTrack.
 * 
 * See animation_clip.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.5.0
 */

#include "animation/animation_clip.h"
#include "core/math.h"

namespace Dnd {

// ==========================================
// BONETRACK IMPLEMENTATION
// ==========================================

Keyframe BoneTrack::GetKeyframe(f32 time, b8 loop, f32 duration) const {
    if (keyframes.empty()) {
        return Keyframe();
    }

    if (keyframes.size() == 1) {
        return keyframes[0];
    }

    // Handle time wrapping for looping
    f32 animTime = time;
    if (loop && duration > 0.0f) {
        animTime = fmodf(animTime, duration);
    }

    // Clamp to valid range
    if (animTime <= keyframes.front().time) {
        return keyframes.front();
    }
    if (animTime >= keyframes.back().time) {
        return keyframes.back();
    }

    // Find the two keyframes surrounding the current time
    u32 i = 0;
    for (; i < keyframes.size() - 1; i++) {
        if (keyframes[i + 1].time > animTime) {
            break;
        }
    }

    const Keyframe& k0 = keyframes[i];
    const Keyframe& k1 = keyframes[i + 1];

    // Calculate interpolation factor
    f32 t = (animTime - k0.time) / (k1.time - k0.time);
    t = Math::Clamp(t, 0.0f, 1.0f);

    // Interpolate position (component-wise lerp)
    Keyframe result;
    result.time = animTime;
    result.position = Vec3(
        Math::Lerp(k0.position.x, k1.position.x, t),
        Math::Lerp(k0.position.y, k1.position.y, t),
        Math::Lerp(k0.position.z, k1.position.z, t)
    );

    // Interpolate rotation (basic slerp implementation)
    f32 dot = k0.rotation.x * k1.rotation.x + k0.rotation.y * k1.rotation.y +
              k0.rotation.z * k1.rotation.z + k0.rotation.w * k1.rotation.w;
    f32 rotT = t;
    if (dot < 0.0f) { dot = -dot; rotT = -rotT; }
    f32 theta = std::acos(Math::Clamp(dot, -1.0f, 1.0f));
    f32 sinTheta = std::sin(theta);
    if (sinTheta > 0.001f) {
        f32 invSin = 1.0f / sinTheta;
        f32 t0 = std::sin((1.0f - rotT) * theta) * invSin;
        f32 t1 = std::sin(rotT * theta) * invSin;
        result.rotation = Quaternion(
            k0.rotation.x * t0 + k1.rotation.x * t1,
            k0.rotation.y * t0 + k1.rotation.y * t1,
            k0.rotation.z * t0 + k1.rotation.z * t1,
            k0.rotation.w * t0 + k1.rotation.w * t1
        );
    } else {
        result.rotation = k0.rotation;
    }

    // Interpolate scale
    result.scale = Vec3(
        Math::Lerp(k0.scale.x, k1.scale.x, t),
        Math::Lerp(k0.scale.y, k1.scale.y, t),
        Math::Lerp(k0.scale.z, k1.scale.z, t)
    );

    return result;
}

void BoneTrack::AddKeyframe(const Keyframe& keyframe) {
    // Insert in sorted order by time
    auto it = keyframes.begin();
    while (it != keyframes.end() && it->time < keyframe.time) {
        ++it;
    }
    keyframes.insert(it, keyframe);
}

// ==========================================
// ANIMATIONCLIP IMPLEMENTATION
// ==========================================

AnimationClip::AnimationClip(const std::string& name)
    : m_name(name), m_duration(0.0f), m_boneCount(0) {
}

void AnimationClip::SetBoneCount(u32 count) {
    m_boneCount = count;
    m_tracks.resize(count);
}

BoneTrack* AnimationClip::GetTrack(u32 boneIndex) {
    if (boneIndex >= m_tracks.size()) return nullptr;
    return &m_tracks[boneIndex];
}

const BoneTrack* AnimationClip::GetTrack(u32 boneIndex) const {
    if (boneIndex >= m_tracks.size()) return nullptr;
    return &m_tracks[boneIndex];
}

void AnimationClip::Sample(f32 time, b8 loop, Mat4* outTransforms) const {
    if (!outTransforms || m_tracks.empty()) return;

    for (u32 i = 0; i < m_tracks.size(); i++) {
        const BoneTrack& track = m_tracks[i];
        Keyframe kf = track.GetKeyframe(time, loop, m_duration);

        // Create local transform matrix
        outTransforms[i] = Mat4::TRS(kf.position, kf.rotation, kf.scale);
    }
}

} // namespace Dnd
