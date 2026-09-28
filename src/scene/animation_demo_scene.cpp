/**
 * @file animation_demo_scene.cpp
 * @brief Implementation of the AnimationDemoScene class.
 *
 * See animation_demo_scene.h for full documentation.
 *
 * @author DND Engine Team
 * @date 2026-05-08
 * @version 0.5.0
 */

#include "scene/animation_demo_scene.h"
#include "core/log.h"
#include "core/math.h"
#include "rendering/mesh.h"
#include "rendering/debug_draw.h"
#include "ecs/components/transform.h"
#include "ecs/components/camera.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/tag.h"
#include "ecs/systems/transform_system.h"
#include "ecs/systems/render_system.h"
#include "animation/animation_system.h"
#include "animation/animation.h"
#include "animation/skeleton.h"
#include "animation/animation_clip.h"
#include "animation/skinned_mesh_renderer.h"

#include <cmath>

namespace Dnd {

// ==========================================
// HELPER: Convert Mat4 rotation to Quaternion
// ==========================================

/**
 * @brief Extracts a quaternion from the upper-left 3x3 rotation part of a Mat4.
 * @param m The 4x4 matrix.
 * @return Normalized quaternion representing the rotation.
 */
static Quaternion QuatFromMatrix(const Mat4& m) {
    f32 trace = m.m[0] + m.m[5] + m.m[10];
    Quaternion q;

    if (trace > 0.0f) {
        f32 s = 0.5f / std::sqrt(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (m.m[6] - m.m[9]) * s;
        q.y = (m.m[8] - m.m[2]) * s;
        q.z = (m.m[1] - m.m[4]) * s;
    } else {
        if (m.m[0] > m.m[5] && m.m[0] > m.m[10]) {
            f32 s = 2.0f * std::sqrt(1.0f + m.m[0] - m.m[5] - m.m[10]);
            q.w = (m.m[6] - m.m[9]) / s;
            q.x = 0.25f * s;
            q.y = (m.m[4] + m.m[1]) / s;
            q.z = (m.m[8] + m.m[2]) / s;
        } else if (m.m[5] > m.m[10]) {
            f32 s = 2.0f * std::sqrt(1.0f + m.m[5] - m.m[0] - m.m[10]);
            q.w = (m.m[8] - m.m[2]) / s;
            q.x = (m.m[4] + m.m[1]) / s;
            q.y = 0.25f * s;
            q.z = (m.m[9] + m.m[6]) / s;
        } else {
            f32 s = 2.0f * std::sqrt(1.0f + m.m[10] - m.m[0] - m.m[5]);
            q.w = (m.m[1] - m.m[4]) / s;
            q.x = (m.m[8] + m.m[2]) / s;
            q.y = (m.m[9] + m.m[6]) / s;
            q.z = 0.25f * s;
        }
    }

    return q.Normalized();
}

// ==========================================
// CLASS: BoneSyncSystem
// ==========================================

/**
 * @class BoneSyncSystem
 * @brief Syncs skeleton bone global transforms to visual bone entity transforms.
 *
 * Runs during the UPDATE phase, after the AnimationSystem has computed
 * the skeleton's global transforms. Reads each bone's global transform
 * from the skeleton and applies the position and rotation to the
 * corresponding bone entity's Transform component.
 */
class BoneSyncSystem : public System {
public:
    Entity skeletonEntity;            ///< Entity with Skeleton component
    Entity boneEntities[3];           ///< Visual entities for each bone
    Vec3 boneScales[3];               ///< Scale for each bone visual

    /**
     * @brief Constructs the BoneSyncSystem.
     */
    BoneSyncSystem()
        : System("BoneSyncSystem", SystemPhase::UPDATE, 5),
          skeletonEntity(Entity::None()) {
        for (i32 i = 0; i < 3; i++) {
            boneEntities[i] = Entity::None();
            boneScales[i] = Vec3(1, 1, 1);
        }
    }

protected:
    void OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) override {
        DND_UNUSED(deltaTime);
        DND_UNUSED(em);

        if (!skeletonEntity.IsValid()) {
            return;
        }

        Skeleton* skeleton = cr.GetComponent<Skeleton>(skeletonEntity);
        if (!skeleton) {
            return;
        }

        u32 boneCount = skeleton->GetBoneCount();
        if (boneCount > 3) {
            boneCount = 3;
        }

        for (u32 i = 0; i < boneCount; i++) {
            if (!boneEntities[i].IsValid()) {
                continue;
            }

            Transform* t = cr.GetComponent<Transform>(boneEntities[i]);
            if (!t) {
                continue;
            }

            // Extract position from global transform translation
            const Mat4& globalMat = skeleton->globalTransforms[i];
            t->position = Vec3(globalMat.m[12], globalMat.m[13], globalMat.m[14]);

            // Extract rotation from the 3x3 rotation part of the global matrix
            t->rotation = QuatFromMatrix(globalMat);

            // Set scale (bone visual size)
            t->scale = boneScales[i];
        }
    }
};

// ==========================================
// ANIMATIONDEMOSCENE IMPLEMENTATION
// ==========================================

AnimationDemoScene::AnimationDemoScene()
    : Scene("AnimationDemoScene"),
      m_skeletonEntity(Entity::None()),
      m_cameraEntity(Entity::None()),
      m_clip(nullptr) {
    for (i32 i = 0; i < 3; i++) {
        m_boneEntities[i] = Entity::None();
    }
}

void AnimationDemoScene::OnLoad() {
    Scene::OnLoad();

    // Step 1: Create shared meshes
    static Mesh* s_cubeMesh = nullptr;
    s_cubeMesh = new Mesh(Mesh::CreateCube());

    // Step 2: Create BoneSyncSystem (entity IDs will be set after creation)
    auto boneSyncSystem = std::make_unique<BoneSyncSystem>();
    BoneSyncSystem* boneSyncPtr = boneSyncSystem.get();

    // Step 3: Register systems
    m_systemManager.RegisterSystem(
        std::make_unique<TransformSystem>(),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::make_unique<AnimationSystem>(),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::move(boneSyncSystem),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::make_unique<RenderSystem>(nullptr),
        m_entityManager, m_componentRegistry);

    // Step 4: Create animation clip
    m_clip = new AnimationClip("ArmatureSwing");
    m_clip->SetDuration(2.0f);
    m_clip->SetBoneCount(3);

    // --- Bone 0 (root): sway around Z axis ---
    {
        BoneTrack* track = m_clip->GetTrack(0);
        if (track) {
            track->AddKeyframe(Keyframe(0.0f, Vec3(0, 0, 0),
                Quaternion::FromEulerAngles(0, 0, 0), Vec3(1, 1, 1)));
            track->AddKeyframe(Keyframe(1.0f, Vec3(0, 0, 0),
                Quaternion::FromEulerAngles(0, 0, Math::DegreesToRadians(20.0f)), Vec3(1, 1, 1)));
            track->AddKeyframe(Keyframe(2.0f, Vec3(0, 0, 0),
                Quaternion::FromEulerAngles(0, 0, Math::DegreesToRadians(-20.0f)), Vec3(1, 1, 1)));
        }
    }

    // --- Bone 1 (upper): swing around X axis ---
    {
        BoneTrack* track = m_clip->GetTrack(1);
        if (track) {
            track->AddKeyframe(Keyframe(0.0f, Vec3(0, 0.8f, 0),
                Quaternion::FromEulerAngles(Math::DegreesToRadians(-10.0f), 0, 0), Vec3(1, 1, 1)));
            track->AddKeyframe(Keyframe(1.0f, Vec3(0, 0.8f, 0),
                Quaternion::FromEulerAngles(Math::DegreesToRadians(20.0f), 0, 0), Vec3(1, 1, 1)));
            track->AddKeyframe(Keyframe(2.0f, Vec3(0, 0.8f, 0),
                Quaternion::FromEulerAngles(Math::DegreesToRadians(-10.0f), 0, 0), Vec3(1, 1, 1)));
        }
    }

    // --- Bone 2 (lower): swing more around X axis ---
    {
        BoneTrack* track = m_clip->GetTrack(2);
        if (track) {
            track->AddKeyframe(Keyframe(0.0f, Vec3(0, 0.8f, 0),
                Quaternion::FromEulerAngles(Math::DegreesToRadians(-20.0f), 0, 0), Vec3(1, 1, 1)));
            track->AddKeyframe(Keyframe(1.0f, Vec3(0, 0.8f, 0),
                Quaternion::FromEulerAngles(Math::DegreesToRadians(30.0f), 0, 0), Vec3(1, 1, 1)));
            track->AddKeyframe(Keyframe(2.0f, Vec3(0, 0.8f, 0),
                Quaternion::FromEulerAngles(Math::DegreesToRadians(-20.0f), 0, 0), Vec3(1, 1, 1)));
        }
    }

    // Step 5: Create skeleton entity with Animation + Skeleton
    m_skeletonEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_skeletonEntity, Tag("AnimatedSkeleton"));

    Skeleton skeleton;
    skeleton.AddBone("root", -1);
    skeleton.AddBone("upper", 0);
    skeleton.AddBone("lower", 1);

    // Position bones relative to parent
    skeleton.bones[0].localPosition = Vec3(0, 0, 0);
    skeleton.bones[1].localPosition = Vec3(0, 0.8f, 0);
    skeleton.bones[2].localPosition = Vec3(0, 0.8f, 0);

    // Calculate inverse bind poses from the bind pose
    skeleton.CalculateInverseBindPoses();

    m_componentRegistry.AddComponent<Skeleton>(m_skeletonEntity, std::move(skeleton));

    Animation anim;
    anim.SetClip(m_clip);
    anim.loop = true;
    anim.Play();
    m_componentRegistry.AddComponent<Animation>(m_skeletonEntity, std::move(anim));

    // Add SkinnedMeshRenderer to demonstrate the component
    m_componentRegistry.AddComponent<SkinnedMeshRenderer>(
        m_skeletonEntity, SkinnedMeshRenderer(s_cubeMesh, Color::White()));

    // Step 6: Create bone visual entities (colored boxes)
    const Color boneColors[3] = {
        Color(0.9f, 0.2f, 0.1f, 1),  // Root: red
        Color(0.1f, 0.8f, 0.2f, 1),  // Upper: green
        Color(0.1f, 0.3f, 0.9f, 1)   // Lower: blue
    };

    const Vec3 boneScales[3] = {
        Vec3(0.3f, 0.8f, 0.3f),
        Vec3(0.25f, 0.8f, 0.25f),
        Vec3(0.2f, 0.8f, 0.2f)
    };

    const char* boneNames[3] = { "Bone_Root", "Bone_Upper", "Bone_Lower" };

    for (i32 i = 0; i < 3; i++) {
        m_boneEntities[i] = m_entityManager.Create();
        m_componentRegistry.AddComponent<Tag>(m_boneEntities[i], Tag(boneNames[i]));
        m_componentRegistry.AddComponent<Transform>(m_boneEntities[i], Transform(
            Vec3(0, 0, 0), Quaternion::Identity(), boneScales[i]));
        m_componentRegistry.AddComponent<MeshRenderer>(
            m_boneEntities[i], MeshRenderer(s_cubeMesh, boneColors[i]));
    }

    // Step 7: Set up BoneSyncSystem with entity IDs and scales
    boneSyncPtr->skeletonEntity = m_skeletonEntity;
    for (i32 i = 0; i < 3; i++) {
        boneSyncPtr->boneEntities[i] = m_boneEntities[i];
        boneSyncPtr->boneScales[i] = boneScales[i];
    }

    // Step 8: Create camera entity
    m_cameraEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_cameraEntity, Tag("MainCamera"));
    m_componentRegistry.AddComponent<Transform>(m_cameraEntity, Transform(
        Vec3(0, 3, 6), Quat::LookAt(Vec3(0, 3, 6), Vec3(0, 1, 0), Vec3(0, 1, 0)), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<ECSCamera>(m_cameraEntity, ECSCamera());

    // Draw debug axes at origin
    DebugDraw::DrawAxes(Vec3(0, 0, 0), 2.0f);

    DND_LOG_INFO("AnimationDemoScene loaded: %u entities, 3 bones, clip='%s' (%.2fs)",
                 m_entityManager.GetAliveCount(),
                 m_clip->GetName(), m_clip->GetDuration());
}

void AnimationDemoScene::OnUnload() {
    delete m_clip;
    m_clip = nullptr;

    Scene::OnUnload();
}

} // namespace Dnd
