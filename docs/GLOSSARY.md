# Engine Glossary

Terms used throughout the D&D Game Engine codebase.

## Core Concepts

| Term | Definition |
|------|------------|
| **Entity** | A unique identifier representing a thing in the game world. Has no data itself — data comes from Components. |
| **Component** | A piece of data attached to an Entity. Examples: Transform, MeshRenderer, Rigidbody. |
| **Scene** | A collection of Entities and their Components. The engine manages one active scene at a time. |
| **Subsystem** | A major engine module (Rendering, Physics, Audio, etc.) with its own lifecycle. |
| **Tick** | A single iteration of the main loop. One tick = one frame of game logic + one frame of rendering. |

## Math

| Term | Definition |
|------|------------|
| **Vec2/3/4** | 2D, 3D, and 4D vectors. Used for positions, directions, colors, homogeneous coordinates. |
| **Mat4** | 4x4 transformation matrix. Column-major, OpenGL-compatible. Stores rotation, translation, scale. |
| **Quaternion** | A 4D rotation representation (XYZW). Avoids gimbal lock. Used for smooth rotation interpolation. |
| **Right-Handed** | Coordinate convention: X right, Y up, Z forward (away from viewer). Cross product follows right-hand rule. |
| **Column-Major** | Matrix storage where columns are contiguous in memory. OpenGL convention. |
| **NDC** | Normalized Device Coordinates. Range [-1, 1] for X and Y, [0, 1] for Z. Output of projection matrices. |
| **FOV** | Field of View. The angular extent of the visible scene, measured in radians. |

## Rendering

| Term | Definition |
|------|------------|
| **VAO** | Vertex Array Object. Stores the format of vertex data for rendering. |
| **VBO** | Vertex Buffer Object. Stores actual vertex data (positions, normals, UVs) in GPU memory. |
| **IBO** | Index Buffer Object. Stores indices for indexed drawing (reuses vertices). |
| **Shader** | A GPU program. Written in GLSL. Vertex shaders transform vertices, fragment shaders compute pixel colors. |
| **Material** | A Shader plus its uniform values (colors, textures, parameters). Multiple meshes can share a material. |
| **Mesh** | Geometry data: vertices, indices, and attributes. Loaded from model files or generated procedurally. |
| **Texture** | An image loaded into GPU memory. Used for surface detail, environment maps, etc. |
| **Framebuffer** | An off-screen render target. Used for post-processing, shadow maps, and reflections. |
| **Draw Call** | A single command to the GPU to render geometry. Minimizing draw calls is a key optimization. |
| **VSync** | Vertical synchronization. Caps the frame rate to the monitor's refresh rate. Prevents screen tearing. |

## Physics

| Term | Definition |
|------|------------|
| **Rigidbody** | An entity with mass, velocity, and forces. Affected by gravity and collisions. |
| **Collider** | A shape attached to an entity for collision detection. Box, sphere, capsule, mesh. |
| **Trigger** | A collider that does NOT produce physical response — only detects overlap. |
| **Manifold** | The result of collision detection: contact points, normal, penetration depth. |
| **Impulse** | An instantaneous change in velocity. Used for collision resolution. |
| **Broad Phase** | The first stage of collision detection. Quickly finds pairs of objects that MIGHT be colliding. |
| **Narrow Phase** | The second stage. Precisely calculates contact details for pairs that ARE colliding. |
| **GJK** | Gilbert-Johnson-Keerthi algorithm. Determines if two convex shapes are intersecting. |
| **EPA** | Expanding Polytope Algorithm. Finds the penetration depth and contact normal after GJK detects collision. |

## Animation

| Term | Definition |
|------|------------|
| **Bone** | A joint in a skeleton. Has a local transform (position, rotation, scale) relative to its parent. |
| **Skeleton** | A hierarchy of bones. Defines the structure of a character or animated object. |
| **Keyframe** | A snapshot of a bone's transform at a specific time in an animation. |
| **Animation Clip** | A sequence of keyframes for all bones over a time range. One complete animation (e.g., "walk", "attack"). |
| **Slerp** | Spherical Linear Interpolation. Used to smoothly interpolate between quaternion rotations. |
| **Skinning** | The process of deforming a mesh based on bone transforms and vertex bone weights. |
| **State Machine** | A system that manages which animation is playing, with transitions between states (Idle -> Walk -> Run). |

## Audio

| Term | Definition |
|------|------------|
| **Source** | An audio emitter. Has a position, velocity, volume, and pitch. Plays a sound. |
| **Listener** | The "ears" of the game. Usually attached to the camera. Determines how spatial audio is heard. |
| **Attenuation** | How sound volume decreases with distance from the source. |
| **Doppler** | The change in pitch when a sound source moves relative to the listener. |

## Network

| Term | Definition |
|------|------------|
| **Authoritative Server** | The server is the source of truth. Clients send inputs, server simulates, clients render results. |
| **Client Prediction** | The client predicts its own movement before the server confirms it. Reduces perceived latency. |
| **Delta Compression** | Sending only the differences between the current and previous state. Reduces bandwidth. |

## Editor/Tools

| Term | Definition |
|------|------------|
| **Inspector** | A panel showing all properties of the selected object. Allows live editing. |
| **Hierarchy** | A tree view of all entities in the current scene. Shows parent-child relationships. |
| **Viewport** | The main 3D view in the editor. Supports orbit, pan, zoom, and gizmo manipulation. |
| **Gizmo** | Visual handles for transforming objects. Move (arrows), Rotate (rings), Scale (cubes). |
| **Prefab** | A reusable entity template. Instances can override specific properties. |
| **Asset** | Any file the engine uses: models, textures, sounds, scripts, materials. |
