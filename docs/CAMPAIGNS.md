# Campaign packs

The engine's play loop stays the 3D table: orbit camera, dice, explore, and combat. A **campaign pack** is one playthrough on that table. D&D is the pack that loads on boot. Other games are other playthroughs. One is active at a time.

## One pack at a time

Packs do not run side by side. There is no shared session and no second game loaded behind the one you are playing. The registry is only a catalog so you can pick.

The flow is:

1. **Pick one pack.** Boot picks `dnd` when nothing else is named. F8 or `--campaign=<id>` picks another.
2. **Unload the previous one.** `SceneManager::SetActiveScene` drops the current scene. Campaigns are not pushed onto a stack.
3. **Clear leftovers** from that pack: UI widgets, undo, editor selection, spotlight occluders, and the orbit camera.
4. **Load that pack and play.** One entry scene, one ruleset binding.

A refused pick (unknown id, bad manifest) leaves the current playthrough where it is. Picking the pack that is already running does not reload it.

## View and controls

**F3** switches view for whichever pack is active:

- **Table** is the orbit camera over the whole scene.
- **Inside** is a camera behind the active figure (Finn in the demo, the selected class in RPG Hopefully).

The D&D demo keeps base controls: WASD orbits the table, and the existing explore and combat loop is unchanged. A pack may name a `controlScheme` in its manifest. While that scheme is active the pack reads WASD itself. `ResetViewToBase` runs before the next scene loads and puts the view back to tabletop and the scheme back to `base`.

`rpg-hopefully` is a playable slice: a practice yard, four classes, walking, and a d20 check. `sectorbound` is a playable outpost: a block yard, a scout, a survey marker, and a beacon scan. `sandhold` is a playable courtyard: dunes, a campfire rest, and a gate brace. The other packs are still scaffolds with gap notes. See [ENGINE_GAPS.md](ENGINE_GAPS.md).

## What a pack is

A pack is a directory:

```
Campaigns/<id>/
  campaign.json    manifest
  README.md        how this game sits on the table
```

`campaign.json` is the draft format (`format` = `dndcampaign`, `formatVersion` = 1). Fields:

| Field | Required | Meaning |
| --- | --- | --- |
| `id` | yes | Lowercase words with single hyphens. Must match the directory name. |
| `title` | yes | Name shown in the picker. |
| `genreTags` | no | Strings stored on the one active pack. |
| `ruleset` | yes | Compiled-in module id. See below. |
| `assetsRoot` | yes | Relative directory inside the pack. `"."` is the pack folder. |
| `entryScene` | yes | `dnd_demo`, `campaign_table`, `rpg_hopefully`, `sectorbound`, or `sandhold`. |
| `entryScript` | no | Relative path recorded for a future script. **Not executed.** |
| `controlScheme` | no | `base`, empty (base), or a pack id. While it is not base, that scene owns WASD. |
| `summary` | no | One line for the overlay. |
| `sourceNote` | no | Where the original project lives on disk. |
| `tableTokens` | no | Placeholder miniatures for `campaign_table`: `name`, `color` `[r,g,b]` in 0..1, `position` `[x,y,z]`. At most 8 are loaded. |

Example:

```json
{
  "format": "dndcampaign",
  "formatVersion": 1,
  "id": "rpg-hopefully",
  "title": "RPG Hopefully",
  "genreTags": ["dice", "class-cousin"],
  "ruleset": "tabletop-generic",
  "assetsRoot": ".",
  "entryScene": "rpg_hopefully",
  "entryScript": "",
  "controlScheme": "rpg-hopefully",
  "summary": "Practice yard: walk a class and roll d20 plus its modifier.",
  "tableTokens": [
    { "name": "Hopeful", "color": [0.86, 0.55, 0.22], "position": [-1.6, 0.45, 0.4] }
  ]
}
```

## How a pack is loaded

`CampaignRegistry` scans `Campaigns/` (next to the working directory, then next to the source tree) and keeps the catalog in memory. It does not recurse, and it does not instantiate those packs. `CampaignHost` then loads exactly one of them.

Select a pack with:

- `DnDEngine --list-campaigns` prints `id`, title, ruleset, and entry scene, then exits. Nothing is loaded.
- `DnDEngine` with no campaign flag loads `dnd`, the current demo.
- `DnDEngine --campaign=rpg-hopefully` loads that one pack at startup.
- **F8** opens the picker. Up/Down, Enter, Esc, number keys 1–9, or a click on a row. Enter unloads the current playthrough and loads the highlighted pack. Escape closes the picker and does not quit. F2, F5, and F6 stay the profiler, the editor, and pause.

`rpg-hopefully`, `sectorbound`, and `sandhold` are playable slices besides the demo. `RpgHopefullyScene` walks a class and rolls a d20 check. `SectorboundScene` walks a scout through a block outpost, stows a marker, and scans a beacon with `DiceRoller`. `SandholdScene` walks a warden through a sandy courtyard, rests at the fire, and braces the gate. The other folders are manifests so those games stay in the catalog. Selecting one still uses this same single-pack load; it does not start a second game. What each title still needs from the engine is in [ENGINE_GAPS.md](ENGINE_GAPS.md).

## Isolation rules

Isolation means a switch cannot corrupt the base demo or keep the previous pack's state. It does not mean several packs are resident together.

- **One copy of the active manifest.** `CampaignSession` holds the pack you are playing. The next successful pick replaces that copy.
- **Rules are a named module, not a plugin.** The only modules are `dnd5e` and `tabletop-generic`. `BindRuleset` fills a `RulesetBinding` on that one session (`dice`, `checkModel`, `usesEngineCombat`). It does not edit dice tables or combat globals. A pack cannot register a new module from disk. Unknown module ids are rejected.
- **`dnd` is pinned.** The id `dnd` always runs entry scene `dnd_demo` and ruleset `dnd5e`, even if its file says otherwise. The title can still change. If the file is missing or unreadable, the host inserts a built-in manifest that loads the same demo.
- **Entry scenes are an allowlist.** `dnd_demo` constructs the existing demo. `campaign_table` constructs `CampaignTableScene` from that pack's tokens. `rpg_hopefully` constructs the practice yard. `sectorbound` constructs the block outpost. `sandhold` constructs the sandy courtyard. Any other `entryScene` is rejected. There is no "load this class name" field.
- **`entryScript` is not run.** The string is stored and, when non-empty, logged. Godot scripts, Bevy apps, and other engines are not started.
- **Paths stay inside the pack.** `assetsRoot` and `entryScript` must be relative. Absolute paths, drive letters, and `..` segments are rejected. The resolved asset directory must still sit inside the pack directory. The loader does **not** add that directory to `AssetManager`, so a pack cannot shadow `Assets/`.
- **Ids do not collide.** The first registered id wins. A later folder with the same id is skipped. Scanning the same directory twice is a no-op.
- **Switching clears the previous pack** before the new scene's `OnLoad`:
  - the previous scene (entities and systems)
  - UI widgets (so the demo's light sliders do not sit on the next pack)
  - the editor undo stack
  - the selected entity
  - spotlight occluders
  - the orbit camera (yaw, pitch, distance, focus), the view mode, and the control scheme (`ResetViewToBase` restores tabletop and `base`)
- The new scene then sets its own light. `CampaignTableScene` clears occluders each frame so a maze shadow set cannot linger. `DndDemoScene` installs its own key light when it loads. After a load, the scene stack size is one.
- Editor toggles (grid, gizmo mode, whether the editor is open) are tool state, not pack content, and stay as the user left them.

`DndDemoScene` still allocates its meshes for the life of that scene instance. That predates packs. Switching away unloads the scene's entities; returning allocates a new demo scene.

## Adding a pack

1. Create `Campaigns/<id>/` with `<id>` in lowercase, hyphens allowed.
2. Add `campaign.json` using the fields above. Use `ruleset` `tabletop-generic` and `entryScene` `campaign_table` unless the pack is the D&D demo.
3. Add a `README.md` that says what the original game is, where it lives, and which parts of the table loop it uses (explore, a check, a fight, tokens).
4. Put art, maps, and later scripts under the pack directory and point `assetsRoot` at that folder. Do not point it at `Assets/` or another pack.
5. Run `campaign_registry_test` (built with the engine). It loads every shipped manifest and checks the isolation cases.
6. Launch with `--campaign=<id>` or pick the row with F8. The table should show the pack title and its tokens. Quit and launch with no arguments: the D&D demo is back.

A playable slice may add one allowlisted scene and a `controlScheme` when the fantasy needs its own space. Scaffold packs stay on `campaign_table`. A token and a summary are enough to reserve an id so the game is not dropped. Gap notes belong in the pack README and in [ENGINE_GAPS.md](ENGINE_GAPS.md).

## Shipped packs

| Id | Original | On the table today |
| --- | --- | --- |
| `dnd` | This demo | Unchanged explore / combat loop |
| `rpg-hopefully` | `C:\Users\micha\source\repos\rpg game hopefuly` | Playable yard: walk a class, roll d20+mod |
| `rpggame` | `D:\dev\rpggame` (Bevy extraction RPG) | Raid framed as tokens. Not a Bevy port |
| `sectorbound` | Voxel sandbox RPG | Playable outpost: walk a scout, stow a marker, scan the beacon |
| `sandhold` | Wasteland squad survival | Playable courtyard: walk a warden, rest at the fire, brace the gate |
| `rank-wars` | World Trigger rank battles | Two sides and a rank token |
| `pirate-games` | Pirate sandbox | Ship, crew, and port tokens |
| `ofa-origami-factory` | Origami / factory puzzles | Bench and fold-step tokens |
| `life-sim-bench` | Simulation labs | Scenario tokens |
| `superhero-vigilante` | `Desktop\IDEAS\superhero-vigilante-game` | Vigilante and threat tokens |
| `dwarf-fortress-runescape` | `Desktop\IDEAS\dwarf-fortress-runescape` | Fort and delve tokens |
| `icarus-like-survival` | `Desktop\IDEAS\icarus-like-survival` | Lander and storm tokens |
| `holloday-factory` | Named so it is not excluded | Factory-shift tokens. Stub |
| `stealth-hope` | Named so it is not excluded | Infiltrator and guard tokens. Stub |
| `hunting` | Named so it is not excluded | Hunter and quarry tokens. Stub |
| `dead-center` | Named so it is not excluded | Holdout tokens. Stub |

The last four titles were part of the platform request. This repository has no sibling survey to pull content from, so their folders reserve the ids and describe a tabletop framing. Selecting any one of them replaces the current playthrough. They are not finished games, and they are not loaded together.
