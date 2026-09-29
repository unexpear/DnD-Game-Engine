# Dungeons & Dragons

Pack id: `dnd`

## Source

This repository. The engine play loop is the base state.

## Tabletop mapping

This pack is the current demo: the maze, Finn, dice checks, traps, and combat all run in `DndDemoScene`. F3 moves between the tabletop orbit and a camera behind Finn. WASD stays the base table controls. Loading another pack unloads this scene; loading `dnd` again builds a fresh demo.

## What loads today

Launching with no `--campaign` flag loads this pack. The id `dnd` is pinned to entry scene `dnd_demo` and ruleset `dnd5e`.

Ruleset module: `dnd5e`. Entry scene: `dnd_demo`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Gaps

The explore and combat loop already runs here. A full campaign built in this engine still needs persistent character sheets, a delve save, imported battle maps, spell and area effects, fog of war, audio that is not tied to Windows media APIs, and a network seat. Pack files cannot add rules beyond the compiled `dnd5e` module. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
