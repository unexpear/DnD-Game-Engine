# Dungeons & Dragons

Pack id: `dnd`

## Source

This repository. The engine play loop is the base state.

## Tabletop mapping

This pack is the current demo, unchanged: the maze, Finn, dice checks, traps, and combat all run in `DndDemoScene`. Other packs sit beside it. They do not replace this scene.

## What loads today

Launching with no `--campaign` flag loads this pack. The id `dnd` is pinned to entry scene `dnd_demo` and ruleset `dnd5e`.

Ruleset module: `dnd5e`. Entry scene: `dnd_demo`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
