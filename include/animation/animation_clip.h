/**
 * @file animation_clip.h
 * @brief Animation clip resource: keyframe tracks for skeletal animation.
 * 
 * An AnimationClip is a resource (not an ECS component) that stores
 * keyframe data for one animation (e.g., "walk", "idle", "attack").
 * Multiple entities can reference the same clip.
 * 
 * Each clip contains tracks for each bone: position, rotation, and scale
 * keyframes with timestamps. The clip handles interpolation between
 * keyframes based on the current animation time.
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.5.0
 */

#pragma once

#include "core/types.h"
#include "core/math.h"
#include <vector>
#include <string>

namespace Dnd {

// ==========================================
// STRUCT: Keyframe
// ==========================================

/**
 * @struct Keyframe
 * @brief A single keyframe in an animation track.
 * 
 * Stores the transform (position, rotation, scale) at a specific time.
 */
struct Keyframe {
    f32 time;          ///< Time in seconds from animation start
    Vec3 position;     ///< Local position
    Quaternion rotation;///< Local rotation
    Vec3 scale;        ///< Local scale

    /// @brief Default constructor.
    Keyframe() : time(0.0f), position(Vec3(0,0,0)), rotation(Quaternion::Identity()), scale(Vec3(1,1,1)) {}

    /**
     * @brief Constructs a keyframe with all values.
     * @param t Time in seconds.
     * @param pos Position.
     * @param rot Rotation.
     * @param sc Scale.
     */
    Keyframe(f32 t, const Vec3& pos, const Quaternion& rot, const Vec3& sc)
        : time(t), position(pos), rotation(rot), scale(sc) {}
};

// ==========================================
// STRUCT: BoneTrack
// ==========================================

/**
 * @struct BoneTrack
 * @brief Animation track for a single bone.
 * 
 * Contains sorted keyframes for position, rotation, and scale.
 * The track handles interpolation between keyframes.
 */
struct BoneTrack {
    std::vector<Keyframe> keyframes; ///< Sorted by time

    /**
     * @brief Gets interpolated keyframe at a specific time.
     * @param time Time in seconds.
     * @param loop Whether to loop the animation.
     * @param duration Total clip duration.
     * @return Interpolated keyframe.
     */
    Keyframe GetKeyframe(f32 time, b8 loop, f32 duration) const;

    /**
     * @brief Adds a keyframe (maintains sorted order by time).
     * @param keyframe The keyframe to add.
     */
    void AddKeyframe(const Keyframe& keyframe);

    /// @brief Gets the number of keyframes.
    u32 GetKeyframeCount() const { return static_cast<u32>(keyframes.size()); }
};

// ==========================================
// CLASS: AnimationClip
// ==========================================

/**
 * @class AnimationClip
 * @brief A complete animation resource with tracks for all bones.
 * 
 * Animation clips are resources that can be shared between multiple
 * entities. They are typically loaded from asset files (glTF, FBX, etc.)
 * but can also be created programmatically.
 * 
 * Usage:
 * @code
 * AnimationClip* clip = new AnimationClip("walk");
 * clip->SetDuration(1.0f);
 * clip->SetBoneCount(10);
 * // Add keyframes for each bone...
 * @endcode
 */
class AnimationClip {
public:
    /**
     * @brief Constructs an animation clip.
     * @param name Human-readable name (e.g., "idle", "walk").
     */
    explicit AnimationClip(const std::string& name);

    /// @brief Destructor.
    ~AnimationClip() = default;

    /// @brief Gets the clip name.
    const char* GetName() const { return m_name.c_str(); }

    /// @brief Gets the clip duration in seconds.
    f32 GetDuration() const { return m_duration; }

    /// @brief Sets the clip duration.
    void SetDuration(f32 duration) { m_duration = duration; }

    /// @brief Gets the number of bone tracks.
    u32 GetBoneCount() const { return m_boneCount; }

    /**
     * @brief Sets the number of bones (resizes tracks array).
     * @param count Number of bones in the skeleton.
     */
    void SetBoneCount(u32 count);

    /**
     * @brief Gets the track for a specific bone.
     * @param boneIndex Index of the bone (must be < bone count).
     * @return Pointer to the bone track, or nullptr if invalid.
     */
    BoneTrack* GetTrack(u32 boneIndex);
    const BoneTrack* GetTrack(u32 boneIndex) const;

    /**
     * @brief Samples the animation at a given time.
     * 
     * Fills an array of transforms (one per bone) by interpolating
     * the keyframes at the given time.
     * 
     * @param time Time in seconds (can exceed duration if looping).
     * @param loop Whether to loop the animation.
     * @param outTransforms Output array of local bone transforms (size = bone count).
     */
    void Sample(f32 time, b8 loop, Mat4* outTransforms) const;

private:
    std::string m_name;           ///< Clip name
    f32 m_duration;               ///< Duration in seconds
    u32 m_boneCount;              ///< Number of bones
    std::vector<BoneTrack> m_tracks; ///< One track per bone
};

} // namespace Dnd
