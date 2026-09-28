/**
 * @file physics_system.cpp
 * @brief Implementation of the PhysicsSystem class.
 * 
 * See physics_system.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-04-30
 * @version 0.4.0
 */

#include "physics/physics_system.h"
#include "rendering/debug_draw.h"
#include "core/log.h"
#include "core/math.h"

namespace Dnd {

PhysicsSystem::PhysicsSystem(const PhysicsConfig& config)
    : System("PhysicsSystem", SystemPhase::FIXED_UPDATE, 5),
      m_config(config), m_spatialHash(config.spatialHashCellSize),
      m_accumulator(0.0f) {
}

void PhysicsSystem::OnUpdate(f32 deltaTime, EntityManager& em, ComponentRegistry& cr) {
    DND_UNUSED(em);

    // Accumulate time for fixed timestep
    m_accumulator += deltaTime;

    // Step physics in fixed increments
    while (m_accumulator >= m_config.fixedTimestep) {
        f32 dt = m_config.fixedTimestep;
        m_accumulator -= dt;

        // Step 1: Integrate rigid bodies
        Integrate(dt, em, cr);

        // Step 2: Detect and resolve collisions
        DetectAndResolveCollisions(em, cr);
    }

    // Step 3: Draw debug shapes (every frame, not just fixed steps)
    if (m_config.enableDebugDraw) {
        DrawDebugShapes(em, cr);
    }
}

void PhysicsSystem::Integrate(f32 dt, EntityManager& /*em*/, ComponentRegistry& cr) {
    auto* rigidBodySet = cr.GetSparseSet<RigidBody>();
    auto* transformSet = cr.GetSparseSet<Transform>();

    if (!rigidBodySet || rigidBodySet->Empty()) return;

    // Iterate over all rigid bodies
    for (u32 i = 0; i < rigidBodySet->Size(); i++) {
        RigidBody& rb = const_cast<RigidBody&>(rigidBodySet->GetDense()[i]);

        // Skip static bodies
        if (rb.isStatic) continue;

        u32 entityIndex = rigidBodySet->GetEntityIndex(i);

        // Apply gravity
        rb.velocity += rb.gravity * dt;

        // Apply damping: v *= (1 - damping * dt)
        rb.velocity = rb.velocity * (1.0f - rb.damping * dt);

        // Integrate position: p += v * dt
        Transform* transform = transformSet ? transformSet->Get(entityIndex) : nullptr;
        if (transform) {
            transform->position += rb.velocity * dt;
        }
    }
}

void PhysicsSystem::DetectAndResolveCollisions(EntityManager& em, ComponentRegistry& cr) {
    DND_UNUSED(em);
    auto* sphereSet = cr.GetSparseSet<SphereCollider>();
    auto* boxSet = cr.GetSparseSet<BoxCollider>();
    auto* capsuleSet = cr.GetSparseSet<CapsuleCollider>();
    auto* rigidBodySet = cr.GetSparseSet<RigidBody>();
    auto* transformSet = cr.GetSparseSet<Transform>();

    if (!transformSet || transformSet->Empty()) return;

    // Clear spatial hash
    m_spatialHash.Clear();

    // Insert all colliders into spatial hash
    auto insertCollider = [&](u32 entityIndex, const Vec3& center, const Vec3& scale) {
        // Check sphere collider
        if (sphereSet && sphereSet->Has(entityIndex)) {
            const SphereCollider* sphere = sphereSet->Get(entityIndex);
            if (sphere && sphere->isEnabled) {
                Vec3 worldCenter = center + sphere->center;
                f32 radius = sphere->radius * Math::Max(scale.x, Math::Max(scale.y, scale.z));
                m_spatialHash.Insert(entityIndex,
                    Vec3(worldCenter.x - radius, worldCenter.y - radius, worldCenter.z - radius),
                    Vec3(worldCenter.x + radius, worldCenter.y + radius, worldCenter.z + radius));
                return;
            }
        }

        // Check box collider
        if (boxSet && boxSet->Has(entityIndex)) {
            const BoxCollider* box = boxSet->Get(entityIndex);
            if (box && box->isEnabled) {
                auto bounds = box->GetWorldBounds(center, scale);
                m_spatialHash.Insert(entityIndex, bounds.min, bounds.max);
                return;
            }
        }

        // Check capsule collider
        if (capsuleSet && capsuleSet->Has(entityIndex)) {
            const CapsuleCollider* capsule = capsuleSet->Get(entityIndex);
            if (capsule && capsule->isEnabled) {
                auto bounds = capsule->GetWorldBounds(center, scale);
                f32 r = bounds.radius;
                Vec3 c = bounds.center;
                m_spatialHash.Insert(entityIndex,
                    Vec3(c.x - r, c.y - r, c.z - r),
                    Vec3(c.x + r, c.y + r, c.z + r));
                return;
            }
        }
    };

    // Iterate over all entities with transforms
    for (u32 i = 0; i < transformSet->Size(); i++) {
        u32 entityIndex = transformSet->GetEntityIndex(i);
        const Transform& transform = transformSet->GetDense()[i];
        insertCollider(entityIndex, transform.position, transform.scale);
    }

    // Get candidate pairs from spatial hash
    auto pairs = m_spatialHash.GetCandidatePairs();

    // Process each candidate pair
    for (const auto& [idxA, idxB] : pairs) {
        // Get transforms
        const Transform* transformA = transformSet->Get(idxA);
        const Transform* transformB = transformSet->Get(idxB);
        if (!transformA || !transformB) continue;

        Contact contacts[3];
        i32 contactCount = 0;

        // Sphere vs Sphere
        if (sphereSet && sphereSet->Has(idxA) && sphereSet->Has(idxB)) {
            const SphereCollider* sa = sphereSet->Get(idxA);
            const SphereCollider* sb = sphereSet->Get(idxB);
            if (!sa || !sb || !sa->isEnabled || !sb->isEnabled) continue;
            if (!Collision::ShouldCollide(sa->layer, sb->layer)) continue;

            Vec3 wa = transformA->position + sa->center;
            Vec3 wb = transformB->position + sb->center;
            f32 ra = sa->radius * Math::Max(transformA->scale.x, Math::Max(transformA->scale.y, transformA->scale.z));
            f32 rb = sb->radius * Math::Max(transformB->scale.x, Math::Max(transformB->scale.y, transformB->scale.z));

            if (Collision::TestSphereSphere(wa, ra, wb, rb)) {
                Entity ea = Entity(Entity::EncodeId(idxA, 0));
                Entity eb = Entity(Entity::EncodeId(idxB, 0));
                contacts[contactCount++] = Collision::GenerateSphereSphere(ea, eb, wa, ra, wb, rb);
            }
        }

        // Sphere vs Box
        if (sphereSet && sphereSet->Has(idxA) && boxSet && boxSet->Has(idxB)) {
            const SphereCollider* sa = sphereSet->Get(idxA);
            const BoxCollider* bb = boxSet->Get(idxB);
            if (!sa || !bb || !sa->isEnabled || !bb->isEnabled) continue;
            if (!Collision::ShouldCollide(sa->layer, bb->layer)) continue;

            Vec3 sc = transformA->position + sa->center;
            f32 sr = sa->radius * Math::Max(transformA->scale.x, Math::Max(transformA->scale.y, transformA->scale.z));
            auto bbounds = bb->GetWorldBounds(transformB->position, transformB->scale);

            if (Collision::TestSphereBox(sc, sr, bbounds.min, bbounds.max)) {
                Entity ea = Entity(Entity::EncodeId(idxA, 0));
                Entity eb = Entity(Entity::EncodeId(idxB, 0));
                contacts[contactCount++] = Collision::GenerateSphereBox(ea, eb, sc, sr, bbounds.min, bbounds.max);
            }
        }

        // Box vs Sphere (reverse)
        if (boxSet && boxSet->Has(idxA) && sphereSet && sphereSet->Has(idxB)) {
            const BoxCollider* ba = boxSet->Get(idxA);
            const SphereCollider* sb = sphereSet->Get(idxB);
            if (!ba || !sb || !ba->isEnabled || !sb->isEnabled) continue;
            if (!Collision::ShouldCollide(ba->layer, sb->layer)) continue;

            auto abounds = ba->GetWorldBounds(transformA->position, transformA->scale);
            Vec3 sc = transformB->position + sb->center;
            f32 sr = sb->radius * Math::Max(transformB->scale.x, Math::Max(transformB->scale.y, transformB->scale.z));

            if (Collision::TestSphereBox(sc, sr, abounds.min, abounds.max)) {
                Entity ea = Entity(Entity::EncodeId(idxA, 0));
                Entity eb = Entity(Entity::EncodeId(idxB, 0));
                contacts[contactCount++] = Collision::GenerateSphereBox(eb, ea, sc, sr, abounds.min, abounds.max);
                if (contactCount > 0) {
                    contacts[contactCount - 1].normal = contacts[contactCount - 1].normal * -1.0f;
                }
            }
        }

        // Box vs Box
        if (boxSet && boxSet->Has(idxA) && boxSet && boxSet->Has(idxB)) {
            const BoxCollider* ba = boxSet->Get(idxA);
            const BoxCollider* bb = boxSet->Get(idxB);
            if (!ba || !bb || !ba->isEnabled || !bb->isEnabled) continue;
            if (!Collision::ShouldCollide(ba->layer, bb->layer)) continue;

            auto abounds = ba->GetWorldBounds(transformA->position, transformA->scale);
            auto bbounds = bb->GetWorldBounds(transformB->position, transformB->scale);

            if (Collision::TestBoxBox(abounds.min, abounds.max, bbounds.min, bbounds.max)) {
                Entity ea = Entity(Entity::EncodeId(idxA, 0));
                Entity eb = Entity(Entity::EncodeId(idxB, 0));
                contacts[contactCount++] = Collision::GenerateBoxBox(ea, eb, abounds.min, abounds.max, bbounds.min, bbounds.max);
            }
        }

        // Resolve all contacts
        for (i32 c = 0; c < contactCount; c++) {
            Contact& contact = contacts[c];
            RigidBody* rbA = rigidBodySet ? rigidBodySet->Get(idxA) : nullptr;
            RigidBody* rbB = rigidBodySet ? rigidBodySet->Get(idxB) : nullptr;
            Transform* tA = transformSet->Get(idxA);
            Transform* tB = transformSet->Get(idxB);

            if (rbA && rbB && tA && tB) {
                ResolveContact(contact, *rbA, *rbB, *tA, *tB);
            }
        }
    }
}

void PhysicsSystem::ResolveContact(const Contact& contact, RigidBody& rbA, RigidBody& rbB,
                                    Transform& transformA, Transform& transformB) {
    DND_UNUSED(transformB);

    // Skip if both are static
    if (rbA.isStatic && rbB.isStatic) return;

    // Calculate relative velocity
    Vec3 relativeVelocity = rbA.velocity - rbB.velocity;
    f32 velAlongNormal = relativeVelocity.Dot(contact.normal);

    // Don't resolve if velocities are separating
    if (velAlongNormal > 0) return;

    // Calculate restitution
    f32 e = contact.restitution;

    // Calculate impulse scalar
    f32 j = -(1.0f + e) * velAlongNormal;
    f32 invMassA = rbA.isStatic ? 0.0f : 1.0f / rbA.mass;
    f32 invMassB = rbB.isStatic ? 0.0f : 1.0f / rbB.mass;
    j /= (invMassA + invMassB);

    // Apply impulse
    Vec3 impulse = contact.normal * j;
    if (!rbA.isStatic) {
        rbA.velocity += impulse * invMassA;
    }
    if (!rbB.isStatic) {
        rbB.velocity -= impulse * invMassB;
    }

    // Apply friction impulse
    Vec3 tangent = relativeVelocity - contact.normal * velAlongNormal;
    if (tangent.LengthSquared() > 0.0001f) {
        tangent = tangent.Normalized();
        f32 jt = -relativeVelocity.Dot(tangent);
        jt /= (invMassA + invMassB);

        f32 frictionCoeff = contact.friction;
        if (Math::Abs(jt) > j * frictionCoeff) {
            jt = j * frictionCoeff * (jt > 0 ? 1.0f : -1.0f);
        }

        Vec3 frictionImpulse = tangent * jt;
        if (!rbA.isStatic) {
            rbA.velocity += frictionImpulse * invMassA;
        }
        if (!rbB.isStatic) {
            rbB.velocity -= frictionImpulse * invMassB;
        }
    }

    // Positional correction
    f32 percent = m_config.percent;
    f32 slop = m_config.slop;
    f32 correction = Math::Max(contact.penetration - slop, 0.0f) /
                     (invMassA + invMassB) * percent;
    Vec3 correctionVec = contact.normal * correction;

    if (!rbA.isStatic) {
        transformA.position -= correctionVec * invMassA;
    }
    if (!rbB.isStatic) {
        transformB.position += correctionVec * invMassB;
    }
}

void PhysicsSystem::DrawDebugShapes(EntityManager& em, ComponentRegistry& cr) {
    DND_UNUSED(em);

    auto* sphereSet = cr.GetSparseSet<SphereCollider>();
    auto* boxSet = cr.GetSparseSet<BoxCollider>();
    auto* capsuleSet = cr.GetSparseSet<CapsuleCollider>();
    auto* transformSet = cr.GetSparseSet<Transform>();

    if (!transformSet) return;

    Color sphereColor = Color(0, 1, 0, 0.5f);
    Color boxColor = Color(1, 1, 0, 0.5f);
    Color capsuleColor = Color(1, 0, 1, 0.5f);
    Color triggerColor = Color(1, 0, 0, 0.3f);

    // Draw sphere colliders
    if (sphereSet) {
        for (u32 i = 0; i < sphereSet->Size(); i++) {
            const SphereCollider& sphere = sphereSet->GetDense()[i];
            if (!sphere.isEnabled) continue;

            u32 entityIndex = sphereSet->GetEntityIndex(i);
            const Transform* transform = transformSet->Get(entityIndex);
            if (!transform) continue;

            Vec3 worldCenter = transform->position + sphere.center;
            f32 radius = sphere.radius * Math::Max(transform->scale.x, Math::Max(transform->scale.y, transform->scale.z));
            Color col = sphere.isTrigger ? triggerColor : sphereColor;

            DebugDraw::DrawSphere(worldCenter, radius, 16, col);
        }
    }

    // Draw box colliders
    if (boxSet) {
        for (u32 i = 0; i < boxSet->Size(); i++) {
            const BoxCollider& box = boxSet->GetDense()[i];
            if (!box.isEnabled) continue;

            u32 entityIndex = boxSet->GetEntityIndex(i);
            const Transform* transform = transformSet->Get(entityIndex);
            if (!transform) continue;

            auto bounds = box.GetWorldBounds(transform->position, transform->scale);
            Color col = box.isTrigger ? triggerColor : boxColor;
            DebugDraw::DrawBox((bounds.min + bounds.max) * 0.5f,
                               (bounds.max - bounds.min) * 0.5f, col);
        }
    }

    // Draw capsule colliders
    if (capsuleSet) {
        for (u32 i = 0; i < capsuleSet->Size(); i++) {
            const CapsuleCollider& capsule = capsuleSet->GetDense()[i];
            if (!capsule.isEnabled) continue;

            u32 entityIndex = capsuleSet->GetEntityIndex(i);
            const Transform* transform = transformSet->Get(entityIndex);
            if (!transform) continue;

            Vec3 worldCenter = transform->position + capsule.center;
            f32 radius = capsule.radius * Math::Max(transform->scale.x, transform->scale.z);
            Color col = capsule.isTrigger ? triggerColor : capsuleColor;

            Vec3 top = worldCenter + Vec3(0, capsule.height * transform->scale.y * 0.5f, 0);
            Vec3 bottom = worldCenter - Vec3(0, capsule.height * transform->scale.y * 0.5f, 0);
            DebugDraw::DrawSphere(top, radius, 12, col);
            DebugDraw::DrawSphere(bottom, radius, 12, col);
        }
    }
}

} // namespace Dnd
