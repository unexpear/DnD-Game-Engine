# Engine gaps

Living list of tools this engine does not yet have if a pack is rebuilt here from scratch. Packs run one at a time on the current table. A faithful slice is enough to play the fantasy. A full port of Bevy, Vulkan, or Unreal projects is out of scope until the rows below close.

Update this file when a system lands, and keep the matching `## Gaps` section in `Campaigns/<id>/README.md` honest.

## What a pack can use today

- One active manifest, one entry scene, one ruleset binding (`dnd5e` or `tabletop-generic`).
- Allowlisted scenes: `dnd_demo` (the current maze, dice, traps, and combat), `campaign_table` (up to eight colored tokens), `rpg_hopefully` (practice yard).
- **F3** switches tabletop orbit and an in-game camera behind a subject the scene publishes. **F8** picks the next pack and unloads the previous one.
- A pack may set `controlScheme`. While it is not `base`, that scene reads WASD. Unload calls `ResetViewToBase`, which restores tabletop view and base D&D controls.
- `DiceRoller` for table checks. Forward OpenGL with one spotlight list and optional occluders. Procedural cubes and quads. OBJ meshes and image textures through `AssetManager`, which packs do **not** mount.
- `entryScript` is stored and not executed.

## Cross-cutting gaps

### Rendering

The play path is a forward pass: meshes, a key spotlight, and maze occluders in the demo. Pack scenes draw untextured cubes. Missing for most of these games: deferred or clustered lighting, a real material/PBR set, skinned meshes in pack scenes (`AnimationSystem` exists and is unused by packs), particles, decals, post-processing used by gameplay, terrain or foliage, voxel or chunk meshing, water, weather, and a first-person weapon camera. The immersed view is a boom behind one subject (eye about 3.3 back and 1.55 up), not a free look inside a level and not a split screen.

### Physics

`PhysicsSystem` integrates rigid bodies and resolves contacts with a spatial hash. Campaign scenes do not register it. The demo's dice checks are `DiceRoller`, not tumbling dice bodies. Missing: character controller, vehicles, buoyancy, destruction, soft bodies, voxel collision, and a query API packs can call without editing the engine scene.

### Networking

`include/network/network_manager.h` describes client, server, and messages. There is no running replication path, no interest management, no lockstep, and no way for a pack to host a session. Co-op, versus, and MMO play cannot be recreated from a manifest.

### UI

Overlays are `UIRenderer` text and a widget list cleared on every pack switch. Missing: inventories, skill trees, dialogue, minimaps, factory or build palettes, and a per-pack HUD that survives only while that pack is active without hand-writing a scene class.

### Scripting and rules

Rules modules are compiled in. A pack cannot ship a new module. `entryScript` is not run. The script system and `AiBrain` are Windows process bridges, not a pack API. Missing: data-driven abilities, behavior trees, a scenario clock, and save/load of campaign state.

### Assets

`AssetManager::LoadMesh` parses OBJ. Textures go through stb. The asset browser mentions glTF and FBX; the loader does not import them. `assetsRoot` is checked so it stays inside the pack, then ignored by the search path, so a pack cannot drop in models or sounds and have them load. Audio playback leans on Media Foundation and does not mount pack files. No voxel, animation-clip, or level format is accepted from `Campaigns/`.

### Controls and cameras

One global scheme string. No rebinding screen, no per-pack gamepad map, no context stacks (walk vs menu vs build). View modes are tabletop and immersed only. Arrow keys and the mouse still orbit in tabletop mode. A pack that needs a different camera writes it by publishing a subject, or it does not get one.

## Per game

### dnd

The demo already is this pack: maze explore, Finn's rig, dice checks, traps, combat, tabletop orbit, and F3 behind Finn. Rebuilding a full table campaign still needs persistent character sheets, a save of the delve, imported battle maps (the maze is procedural plus a few PNGs), spell and area effects, fog of war, a dice-body tray, audio that is not tied to Windows media APIs, and a network seat for remote players. Rules content beyond the compiled `dnd5e` binding cannot arrive as pack data.

### rpg-hopefully

Ships a real slice: a bounded practice yard, four class figures (Hopeful CHA +2, Warden STR +3, Hexer INT +3, Chorus WIS +1), WASD walk relative to view yaw, Q/E or Tab to switch class, R or Space for `DiceRoller` d20+mod against DC 12, and F3 between the table and a camera behind the figure. The hopeful repository is not imported. Still missing: that game's classes, items, and levels as data; character sheets and progression; anything but four hardcoded modifiers; skinned characters (these are cubes); hit reactions on the dummy (it is a static mesh); inventory, dialogue, save, and the original UI. `tableTokens` in the manifest are descriptive only. The scene does not place them.

### rpggame

The original is a Bevy first-person extraction shooter. Today the pack is three tokens on `campaign_table`. Recreating it needs a level importer (Bevy scenes or glTF; this loader reads OBJ only), a weapon camera and hitscan or projectile combat, guard perception, an extraction clock, navmesh or physics collision for real cover, and co-op replication. `entryScript` will not start a Bevy app. The immersed camera is a third-person boom, not an FPS view.

### sectorbound

A voxel sandbox needs chunk streaming, meshing, a block palette, edit tools, and collision against edited volumes. This engine has none of those. The rigid-body spatial hash is not a voxel world. There is no world save. The pack places sector, claim, and encounter tokens and stops there.

### sandhold

Squad survival needs needs-meters, inventory, a camp that persists, travel across a wasteland map, and squad coordination. The engine can show a few tokens and roll a check. It cannot simulate scarcity, path a squad, or save the camp. Combat would fall through to the D&D beat only if a future scene opts into `dnd5e`; this pack's binding does not.

### rank-wars

Rank battles need an arena, trigger loadouts as ability data, beams or projectiles, round structure, and a stored rank. None of that exists as engine services. Versus play needs the missing network path. Tokens for attacker, defender, and rank are the whole load.

### pirate-games

A sailing sandbox needs water rendering, buoyancy, a ship controller, wind, a voyage map, and crew state. There is no vehicle or fluid system. Port-to-port play would be hand-built scenes plus checks. Ship, crew, and port tokens are placeholders.

### ofa-origami-factory

Factory puzzles need belts or a step graph, deformable paper or machine meshes, placement UI, and puzzle state. Contact impulses are not a conveyor. Origami deformation is not a mesh feature. The pack is bench, fold, and sheet tokens. `entryScript` cannot encode a fold sequence yet.

### life-sim-bench

A bench needs a scenario runner, many agents, a clock, metrics, and data-driven labs. ECS entities exist, but there is no sim scheduler, no result export, and no script execution. Lab tokens do not step a simulation.

### superhero-vigilante

City patrol needs a streamed city, traversal (swing, glide, climb) beyond walking a token, powers as abilities, crime spawning, and a day/night cycle. The renderer has no city pipeline and no time of day. There is no character controller. Vigilante, threat, and block tokens stand in for the patrol.

### dwarf-fortress-runescape

This needs a job/stock fortress sim, digging or tile maps, pathfinding for many agents, skill XP, and a stocks UI. The engine's UI and AI are not that scale, and there is no tile or voxel edit API. Fort, delve, and caravan tokens only mark the loop of leaving town and going down.

### icarus-like-survival

A prospect needs a drop timer, storm or weather, crafting trees, build pieces, terrain, and survival meters. Weather, crafting, and building are absent. The storm token is a label in the manifest, not a clock. Session save is missing, so an extract cannot persist loot.

### holloday-factory

Same factory gap as Origami Factory: stations, a line, quotas, and machine logic. No sibling project was in this repo to slice, so the manifest is a reservation. A shift cannot be simulated until scripting, placement UI, and a real station scene exist.

### stealth-hope

Stealth needs vision cones, light levels that change detection, noise, patrol routes, and cover. Maze occluders are shadows for the demo, not a stealth lighting model. `AiBrain` is not a pack service. Infiltrator, guard, and objective tokens do not see each other.

### hunting

Hunting needs animal behavior, scent or tracks as state, a large outdoor terrain, and ballistics. None are engine features. The immersed camera can sit behind one subject; it is not a stalk or scope camera. Hunter, trail, and quarry tokens do not simulate a hunt.

### dead-center

A holdout needs wave spawning, a director, weapon feedback, and cover that can break. The table combat beat is the D&D demo's, and this pack does not enter it. There is no horde system and no destruction. Center, holder, and pressure tokens mark the last stand only.
