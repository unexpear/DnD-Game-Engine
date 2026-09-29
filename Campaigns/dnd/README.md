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

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
