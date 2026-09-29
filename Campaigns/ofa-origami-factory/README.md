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

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
