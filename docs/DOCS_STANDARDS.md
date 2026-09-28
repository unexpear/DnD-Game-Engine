# Documentation Standards

Every file, class, struct, function, and public member MUST be documented. No exceptions.

## File Headers

Every `.h` and `.cpp` file MUST begin with this header block:

```cpp
/**
 * @file filename.h
 * @brief One-line description of the file's purpose.
 * 
 * Detailed multi-line description of what this file does,
 * why it exists, and how it fits into the engine architecture.
 * 
 * @author DND Engine Team
 * @date YYYY-MM-DD
 * @version X.Y.Z
 * 
 * @note Any important notes about usage or limitations.
 * @warning Any warnings about potential pitfalls.
 */
```

## Class/Struct Documentation

Every class and struct MUST have a Doxygen-style block:

```cpp
/**
 * @class ClassName
 * @brief One-line description.
 * 
 * Detailed description of the class's responsibilities,
 * design decisions, and how it interacts with other systems.
 * 
 * @par Example:
 * @code
 * ClassName obj;
 * obj.Method();
 * @endcode
 * 
 * @see RelatedClass
 */
```

## Function Documentation

Every function MUST have:

```cpp
/**
 * @brief One-line description of what the function does.
 * 
 * Detailed description of behavior, edge cases, and side effects.
 * 
 * @param param_name Description of the parameter.
 * @param param_name Description of another parameter.
 * @return Description of the return value. Include units if applicable.
 * 
 * @note Important notes about usage.
 * @warning Warnings about misuse.
 * @pre Preconditions that must be true before calling.
 * @post Postconditions guaranteed after the function returns.
 */
```

## Member Variable Documentation

Every public/protected member variable MUST have an inline comment:

```cpp
/** @brief Description of the variable and its units. */
f32 m_velocity;  ///< Current velocity in meters per second.
```

## Code Section Dividers

Large files MUST use section dividers:

```cpp
// ==========================================
// SECTION NAME
// ==========================================
```

## Inline Comments

- Explain WHY, not WHAT (the code shows what)
- Document non-obvious algorithms, magic numbers, and workarounds
- Use `// TODO:` for incomplete features with a description
- Use `// NOTE:` for important implementation details
- Use `// HACK:` for temporary workarounds (must explain why)
- Use `// FIXME:` for known bugs (must include issue reference)

## Naming Conventions

- Types: `PascalCase` (e.g., `Vec3`, `MeshRenderer`)
- Functions: `PascalCase` (e.g., `CalculateNormal()`)
- Variables: `camelCase` (e.g., `playerHealth`)
- Member variables: `m_` prefix + `camelCase` (e.g., `m_playerHealth`)
- Static variables: `s_` prefix + `camelCase` (e.g., `s_instanceCount`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_PLAYERS`)
- Macros: `UPPER_SNAKE_CASE` (e.g., `DND_LOG_ERROR`)
- Namespaces: `PascalCase` (e.g., `Dnd::Rendering`)

## Module Structure

Each engine subsystem lives in its own directory under `src/`:
- `core/` — Engine foundation: types, math, logging, memory, events
- `rendering/` — OpenGL renderer: shaders, meshes, materials, cameras
- `physics/` — Physics simulation: collision detection, rigid bodies
- `animation/` — Animation system: skeletal animation, state machines
- `ui/` — User interface: widgets, layouts, rendering
- `tools/` — Editor tools: inspectors, debug visualizers
- `audio/` — Audio system: sound playback, spatial audio
- `network/` — Multiplayer: client/server, serialization
- `assets/` — Asset pipeline: loading, caching, hot-reloading
- `game/` — Game-specific logic: rules, characters, combat
