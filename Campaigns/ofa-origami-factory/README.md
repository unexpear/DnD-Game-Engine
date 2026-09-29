# Origami Factory

Pack id: `ofa-origami-factory`

## Source

Origami Factory / OFA. Puzzle-factory play, stretched onto the tabletop loop on purpose.

## Tabletop mapping

The workbench is the table. Each fold step is a token and a check instead of a conveyor sim. A finished sheet is the scene goal, the way a chest is the goal in the dungeon. Machines and belts are not ported.

## What loads today

Bench, fold, and sheet tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
