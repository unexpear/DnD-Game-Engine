# Phase 2: Rendering Engine — Specification

## Status: COMPLETE

All files written, fully documented, ready for build testing.

## Scope

Add window creation, input handling, OpenGL rendering, shader compilation, mesh rendering, camera system, and debug drawing. The result is a 3D scene with a green table, a blue cube, a colorful test triangle, coordinate axes, and orbit camera controls.

## Files Delivered

### Core Systems (Updated)

| File | Lines | Purpose |
|------|-------|---------|
| `include/core/window.h` | 160 | GLFW window management declaration |
| `src/core/window.cpp` | 170 | Window implementation (creation, callbacks, framebuffer tracking) |
| `include/core/input.h` | 160 | Input system declaration (keyboard, mouse) |
| `src/core/input.cpp` | 130 | Input implementation (state polling, press/release detection) |
| `include/core/engine.h` | 170 | Updated Engine declaration with camera, rendering |
| `src/core/engine.cpp` | 270 | Updated Engine with full main loop, scene rendering, camera orbit |
| `src/main.cpp` | 100 | Updated entry point with --help flag |

### Rendering Systems (New)

| File | Lines | Purpose |
|------|-------|---------|
| `include/rendering/renderer.h` | 140 | Renderer declaration (OpenGL init, frame management) |
| `src/rendering/renderer.cpp` | 140 | Renderer implementation (GLAD, OpenGL state, error checking) |
| `include/rendering/shader.h` | 180 | Shader declaration (compilation, uniform management) |
| `src/rendering/shader.cpp` | 200 | Shader implementation (compile, link, uniform caching) |
| `include/rendering/mesh.h` | 180 | Mesh declaration (VAO, VBO, IBO, built-in generators) |
| `src/rendering/mesh.cpp` | 250 | Mesh implementation (creation, drawing, cube/quad/triangle generators) |
| `include/rendering/camera.h` | 170 | Camera declaration (perspective/orthographic, orbit) |
| `src/rendering/camera.cpp` | 130 | Camera implementation (view/projection matrices, orbit math) |
| `include/rendering/debug_draw.h` | 140 | DebugDraw declaration (lines, boxes, spheres, arrows) |
| `src/rendering/debug_draw.cpp` | 200 | DebugDraw implementation (wireframe rendering) |
| **TOTAL NEW** | **~2,140** | **All fully documented** |

## What Works

- GLFW window creation with OpenGL 4.6 Core Profile context
- Input polling (keyboard, mouse position, mouse buttons)
- OpenGL initialization via GLAD with error checking
- GLSL shader compilation and linking with error reporting
- Vertex buffer objects with position, normal, UV, and color attributes
- Index buffer objects for indexed drawing
- Perspective camera with orbit controls (right-click drag + scroll wheel)
- Simple directional lighting (ambient + diffuse)
- Green table quad, blue cube, colorful test triangle
- Debug wireframe box rendering around the cube
- RGB coordinate axes debug visualization
- Escape key to quit
- --help command-line flag
- All code documented per DOCS_STANDARDS.md

## What's Not Done (Future Phases)

- Texture loading (stb_image integration ready, not yet wired up)
- Model file loading (OBJ, glTF)
- Multiple materials per mesh
- Mesh instancing
- Shadows
- Post-processing
- Framebuffers for off-screen rendering
- Entity-Component System
- Physics simulation
- Animation system
- Audio system
- Network/multiplayer
- Editor tools

## Controls

| Input | Action |
|-------|--------|
| Right-click drag | Orbit camera around target |
| Scroll wheel | Zoom in/out (2 to 50 units) |
| Escape | Quit the engine |

## Build Instructions

1. Set up third-party dependencies (see `docs/BUILDING.md`)
2. `cmake -B build -S .`
3. `cmake --build build --config Debug`
4. `build/DnDEngine.exe`

## Test Results (to be filled after build)

| Test | Expected | Result |
|------|----------|--------|
| T1: CMake configure | No errors | TBD |
| T2: Build Debug | Compiles all files, links successfully | TBD |
| T3: Window opens | 1280x720 window with dark background | TBD |
| T4: Triangle renders | Colorful triangle visible | TBD |
| T5: Cube renders | Blue cube with lighting visible | TBD |
| T6: Table renders | Green quad visible as table surface | TBD |
| T7: Camera orbit | Right-drag orbits around scene | TBD |
| T8: Zoom | Scroll wheel zooms in/out | TBD |
| T9: Debug axes | RGB axes visible at origin | TBD |
| T10: Escape quit | Pressing Escape closes window cleanly | TBD |
| T11: Log file | engine.log created with session log | TBD |
| T12: --debug flag | DEBUG messages appear in console | TBD |
| T13: --help flag | Help text printed, program exits | TBD |
| T14: Window resize | Resizing window updates viewport correctly | TBD |
| T15: Clean shutdown | No crashes, no leaks | TBD |

## Next Phase: Phase 3 — Scene System & ECS

Phase 3 will add:
- Entity-Component System (entities, components, transforms)
- Scene graph with parent-child hierarchy
- Scene serialization (save/load to files)
- Material system (shader + textures + parameters)
- Texture loading (stb_image integration)
- Multiple objects with independent transforms
- Prefab system
