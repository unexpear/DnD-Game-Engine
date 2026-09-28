# Phase 3: Scene System & ECS (Entity Component System)

## Overview
Implements an Entity Component System (ECS) for flexible, data-oriented scene management. Entities are IDs, components are data-only structs, and systems process entities with specific component combinations. This enables composition over inheritance and cache-friendly data layout.

## Key Features
1. **Entity Manager**: Handles entity creation, destruction, and ID recycling
2. **Component Registry**: Type-safe component storage with sparse sets
3. **System Interface**: Base class for game logic systems
4. **Scene Manager**: Manages scenes, loading/unloading, transitions
5. **Built-in Components**: Transform, MeshRenderer, Camera, Light, AudioSource
6. **Scene Graph**: Parent-child hierarchy with world matrix computation
7. **Prefab System**: Template entities that can be instantiated

## ECS Design
- **Entity**: 32-bit ID with generation counter to prevent dangling references
- **Components**: POD structs with no virtual functions, stored in contiguous arrays
- **Systems**: Process entities matching specific component signatures
- **Sparse Sets**: Fast iteration, O(1) component lookup, efficient destruction

## Component List
| Component | Data | Purpose |
|-----------|------|---------|
| Transform | position (Vec3), rotation (Quat), scale (Vec3), parent (Entity) | Spatial hierarchy |
| MeshRenderer | meshId (u32), materialId (u32), visible (b8) | Renderable objects |
| Camera | fov, near, far, aspect, active (b8) | View/projection matrices |
| Light | type (enum), color (Vec3), intensity (f32), range (f32) | Scene lighting |
| AudioSource | sourceId (u32), clipId (u32), playing (b8), loop (b8) | Sound playback |
| Tag | name (string hash) | Entity identification |
| RigidBody | mass, velocity, acceleration, isStatic | Physics integration |

## System List
| System | Processes | Runs During |
|--------|-----------|-------------|
| TransformSystem | Transform (with parent) | Update |
| RenderSystem | Transform + MeshRenderer + Camera (active) | Render |
| LightSystem | Transform + Light | Render |
| AudioSystem | Transform + AudioSource | Update |
| PhysicsSystem | Transform + RigidBody | FixedUpdate |

## Scene Features
- Active scene management
- Scene loading from files (future: JSON/binary)
- Scene transitions (fade, crossfade)
- Entity serialization (future)
- Scene graph traversal

## Prefab Features
- Prefab definition as entity template
- Instantiate prefab into scene
- Override specific component values
- Prefab nesting (future)

## Implementation Order
1. ECS core (EntityManager, ComponentRegistry)
2. Component types (Transform, MeshRenderer, Camera, etc.)
3. System interface and built-in systems
4. SceneManager (active scene, scene stack)
5. Scene graph (parent-child hierarchy)
6. Prefab system
7. Engine integration (replace hardcoded scene setup)

## Files
### Headers
- `include/ecs/entity.h` — Entity type, generation counter
- `include/ecs/component.h` — Component base, registry
- `include/ecs/system.h` — System interface
- `include/ecs/components/transform.h` — Transform component
- `include/ecs/components/mesh_renderer.h` — MeshRenderer component
- `include/ecs/components/camera.h` — Camera component
- `include/ecs/components/light.h` — Light component
- `include/ecs/components/audio_source.h` — AudioSource component
- `include/ecs/components/tag.h` — Tag component
- `include/ecs/components/rigidbody.h` — RigidBody component
- `include/ecs/systems/transform_system.h` — Transform system
- `include/ecs/systems/render_system.h` — Render system
- `include/ecs/systems/light_system.h` — Light system
- `include/scene/scene.h` — Scene class
- `include/scene/scene_manager.h` — SceneManager class
- `include/scene/prefab.h` — Prefab class

### Sources
- `src/ecs/entity.cpp`
- `src/ecs/component.cpp`
- `src/ecs/system.cpp`
- `src/ecs/components/transform.cpp`
- `src/ecs/components/mesh_renderer.cpp`
- `src/ecs/components/camera.cpp`
- `src/ecs/components/light.cpp`
- `src/ecs/components/audio_source.cpp`
- `src/ecs/components/tag.cpp`
- `src/ecs/components/rigidbody.cpp`
- `src/ecs/systems/transform_system.cpp`
- `src/ecs/systems/render_system.cpp`
- `src/ecs/systems/light_system.cpp`
- `src/scene/scene.cpp`
- `src/scene/scene_manager.cpp`
- `src/scene/prefab.cpp`

## Success Criteria
- Create/destroy entities dynamically
- Add/remove/query components by type
- Systems process matching entities automatically
- Parent-child hierarchy computes correct world transforms
- SceneManager handles active scene switching
- Prefabs can be instantiated with overrides
- Engine uses ECS for scene management (replaces hardcoded setup)
- All code documented per DOCS_STANDARDS.md
