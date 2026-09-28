# Engine Architecture

## System Overview

```
+-------------------------------------------------------------------+
|                        GAME LAYER                                  |
|  D&D Rules | Characters | Combat | Workshop Content               |
+-------------------------------------------------------------------+
|                     ENGINE FRAMEWORK                               |
|  Scene Graph | Entity-Component System | Event Bus | State Machine  |
+--------+----------+----------+---------+----------+----------------+
|Render- |  Physics  |  Anima-  |  Audio  | Network  |  Tools/Editor  |
| ing    |           |  tion    |         |          |                |
+--------+-----------+----------+---------+----------+----------------+
|                       CORE SYSTEMS                                   |
|  Types | Math | Logging | Memory | Time | Config | File System | IO  |
+-------------------------------------------------------------------+
|                 PLATFORM / THIRD-PARTY                               |
|  GLFW (Window/Input) | GLAD (OpenGL) | stb_image (Images)           |
+-------------------------------------------------------------------+
```

## Module Dependency Graph

```
                    main.cpp
                       |
                    Engine
                   /   |   \
              Logger  Time  Config
                 |      |
              Window  Input
                 |
              Renderer
             /    |    \
          Shader Mesh  Texture
             \    |    /
              OpenGL
```

## Data Flow: One Frame

```
1. Time::Update()          — Compute delta time
2. Window::Update()        — Poll OS events (keyboard, mouse, window)
3. Input::Update()         — Update input state from window events
4. EventSystem::Process()  — Dispatch queued events to subscribers
5. PhysicsWorld::Step(dt)  — Simulate physics (fixed timestep)
6. GameLogic::Update(dt)   — AI, rules, combat, scripts
7. AnimationSystem::Update(dt) — Update bone transforms, blend animations
8. Renderer::BeginFrame()  — Clear buffers, set viewport
9. Renderer::RenderScene() — Sort, batch, draw all visible objects
10. Renderer::EndFrame()   — Swap buffers, present to screen
```

## Subsystem Descriptions

### Core Systems
The foundation everything else depends on. No subsystem depends on anything above it.

- **Types**: Fixed-width type aliases (i32, f32, etc.) for portability
- **Math**: Vec2/3/4, Mat4, Quaternion, Color, collision primitives
- **Logging**: Multi-level, thread-safe, console + file output
- **Memory**: Custom allocators (stack, pool, arena), leak detection
- **Time**: Delta time, fixed timestep, FPS tracking
- **Config**: Settings persistence, key-value storage
- **File System**: Path manipulation, file I/O, directory scanning
- **Events**: Pub/sub event bus with type-safe handlers

### Rendering
OpenGL-based 3D rendering pipeline.

- **Shader**: GLSL compilation, linking, uniform management
- **Mesh**: VAO/VBO/IBO abstraction, vertex attributes
- **Texture**: 2D textures, arrays, samplers (stb_image loading)
- **Material**: Shader + uniforms + textures = renderable surface
- **Camera**: Perspective/orthographic projection, view matrices
- **Renderer**: Render pass system, draw call batching
- **Framebuffer**: FBOs, render targets, post-processing
- **Debug Draw**: Wireframe rendering for physics/animation debugging

### Physics
Custom physics engine (no third-party physics library).

- **Collision Detection**: GJK/EPA for convex shapes, SAT for boxes
- **Broad Phase**: Spatial partitioning (BVH or grid)
- **Rigid Body**: Mass, inertia, velocity, forces, integration
- **Constraint Solver**: Sequential impulses for contact resolution
- **Colliders**: Box, sphere, capsule, mesh shapes
- **Dice Physics**: Polyhedral dice geometry, roll detection, result reading

### Animation
Skeletal animation system.

- **Bone Hierarchy**: Parent/child tree, local/global transforms
- **Skeleton**: Bone collection, transform evaluation
- **Animation Clip**: Keyframe data per bone, timeline
- **Animation Player**: Play, pause, loop, blend
- **State Machine**: States, transitions, conditions
- **Skinning**: Mesh deformation based on bone weights

### Audio
Sound playback and spatial audio.

- **Source**: Play, pause, stop, loop, volume, pitch
- **Spatial Audio**: 3D positioning, attenuation, doppler
- **Listener**: Camera-linked audio listener
- **Music**: Streaming playback, crossfade

### Network
Multiplayer support.

- **Transport**: TCP/UDP abstraction, message serialization
- **Server**: Authoritative simulation, client management
- **Client**: Input sending, state interpolation, prediction
- **Sync**: Entity state sync, delta compression

### Tools/Editor
In-engine development tools.

- **Scene Editor**: Hierarchy, inspector, viewport, asset browser
- **Map Builder**: Tile brush, object placement, terrain sculpting
- **Animation Editor**: Timeline, bone hierarchy, curve editing
- **Physics Debugger**: Collision visualization, step controls
- **Material Editor**: Parameter editing, preview

## Threading Model

**Phase 1-3: Single-threaded.** Everything runs on the main thread. Simple, deterministic, no race conditions.

**Phase 4+: Job System planned.** Physics simulation and rendering preparation can run on worker threads. Main thread handles game logic and render submission.

## Memory Model

**Phase 1-3: Standard heap (new/delete).** Simplicity over optimization.

**Phase 4+: Custom allocators.**
- **Arena**: Fast bump allocation for per-frame data (reset each frame)
- **Pool**: Fixed-size block allocation for entities, components
- **Stack**: LIFO allocation for temporary data (function scope)

## Event System

Centralized pub/sub event bus. Any system can publish events, any system can subscribe.

```
Window publishes: WindowResized, WindowClosed, KeyPressed, MouseMoved
Physics publishes: CollisionStarted, CollisionEnded, TriggerEntered
Game publishes: TurnStarted, DiceRolled, CharacterDamaged
```

Events are queued during the frame and processed at a deterministic point in the main loop.
