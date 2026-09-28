# Phase 4: Physics System

## Overview
Implements a 3D physics engine with collision detection, collision response, and rigid body dynamics. Uses an impulse-based resolution system for realistic interactions. Integrates with the ECS using existing `RigidBody` and `Transform` components, plus new `Collider` components.

## Key Features
1. **Rigid Body Dynamics**: Velocity, acceleration, gravity, damping, mass
2. **Collision Shapes**: Sphere, Box, Capsule, Plane
3. **Broad-Phase**: Spatial hash grid for efficient overlap detection
4. **Narrow-Phase**: GJK + EPA for convex shapes, analytical tests for spheres
5. **Collision Response**: Impulse-based resolution with friction and restitution
6. **Collision Events**: OnTriggerEnter, OnTriggerStay, OnTriggerExit, OnCollisionEnter, OnCollisionStay, OnCollisionExit
7. **Physics Layers**: Configurable collision matrix (which layers collide with which)
8. **Debug Visualization**: Wireframe collision shapes, contact points, normals

## Physics Components

### Collider
```cpp
struct Collider {
    ColliderType type;      // Sphere, Box, Capsule
    bool isTrigger;         // Trigger = no collision response, just events
    bool isEnabled;         // Whether this collider is active
    Vec3 center;            // Local offset from transform position
    union {
        f32 radius;         // Sphere
        Vec3 halfExtents;   // Box
        struct { f32 radius; f32 height; } capsule; // Capsule
    };
    u32 layer;              // Physics layer for collision filtering
};
```

### Physics Material
```cpp
struct PhysicsMaterial {
    f32 friction;           // Coefficient of friction (0 = ice, 1 = rubber)
    f32 restitution;        // Bounciness (0 = no bounce, 1 = perfectly elastic)
};
```

## Collision Detection Pipeline

### Broad Phase (Spatial Hash)
1. Compute bounding box for each collider based on its transform
2. Insert colliders into spatial hash grid cells
3. For each cell, test all pairs of colliders
4. Output: candidate pairs for narrow phase

### Narrow Phase
1. **Sphere vs Sphere**: Distance comparison
2. **Sphere vs Box**: Point-to-box distance
3. **Box vs Box**: Separating Axis Theorem (15 axes)
4. **Sphere vs Capsule**: Point-to-segment distance
5. **Capsule vs Capsule**: Segment-to-segment distance

### Contact Generation
Each collision produces a `Contact` struct:
```cpp
struct Contact {
    Entity entityA, entityB;
    Vec3 normal;            // From A to B
    Vec3 point;             // Contact point in world space
    f32 penetration;        // Depth of overlap
    PhysicsMaterial materialA, materialB;
};
```

## Collision Response

### Impulse Resolution
1. Calculate relative velocity at contact point
2. Calculate impulse magnitude (based on mass, restitution, normal)
3. Apply impulse to both bodies' velocities
4. Apply friction impulse (tangential to contact)
5. Positional correction (prevent sinking)

### Positional Correction
```
correction = max(penetration - slop, 0) / (1/massA + 1/massB) * percent
positionA -= correction / massA * normal
positionB += correction / massB * normal
```

## Integration (per FixedUpdate step)
1. Apply forces (gravity, custom forces) to acceleration
2. Integrate velocity: `velocity += acceleration * dt`
3. Apply damping: `velocity *= (1 - damping * dt)`
4. Integrate position: `position += velocity * dt`
5. Update angular velocity and rotation (future)
6. Clear accumulated forces

## Physics Layers
```cpp
// Layer mask: 32-bit integer where each bit is a layer
enum PhysicsLayer : u32 {
    DEFAULT     = 1 << 0,
    PLAYER      = 1 << 1,
    ENEMY       = 1 << 2,
    ENVIRONMENT = 1 << 3,
    PROJECTILE  = 1 << 4,
    TRIGGER     = 1 << 5,
    // ... up to 32 layers
};

// Collision matrix: 32x32 bitmask
// Default: all layers collide with all except TRIGGER
// TRIGGER only detects OnTriggerEnter/Exit
```

## Implementation Order
1. Collider components (Sphere, Box, Capsule)
2. Physics material component
3. Spatial hash grid (broad phase)
4. Narrow phase collision tests
5. Contact generation
6. Impulse-based collision response
7. PhysicsSystem (integration + resolution loop)
8. Collision event system
9. Debug visualization
10. Physics demo scene

## Files

### Headers
- `include/physics/collider.h` — Collider component, collision types
- `include/physics/physics_material.h` — Physics material properties
- `include/physics/spatial_hash.h` — Spatial hash grid for broad phase
- `include/physics/collision.h` — Contact struct, collision detection functions
- `include/physics/physics_system.h` — Physics system (ECS system)
- `include/physics/physics_config.h` — Physics settings, layer mask, collision matrix

### Sources
- `src/physics/collider.cpp`
- `src/physics/physics_material.cpp`
- `src/physics/spatial_hash.cpp`
- `src/physics/collision.cpp`
- `src/physics/physics_system.cpp`
- `src/physics/physics_config.cpp`

## Success Criteria
- Colliders detect overlaps with correct bounding checks
- Broad phase efficiently reduces pair count
- Narrow phase produces accurate contact data
- Impulse resolution produces realistic bouncing/friction
- Positional correction prevents object sinking
- Collision events fire correctly (Enter, Stay, Exit)
- Physics layers filter collisions properly
- Debug visualization shows collision shapes and contacts
- Performance: 100+ objects at 60 FPS
- All code documented per DOCS_STANDARDS.md
