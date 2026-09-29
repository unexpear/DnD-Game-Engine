# Holloday Factory

Pack id: `holloday-factory`

## Source

Included from the platform brief so this title is not left out. This repository has no sibling survey to import, so the folder is a stub that still loads.

## Tabletop mapping

Treat a shift as a scene. Stations are tokens. A step on the line is a check, the same stretch used for Origami Factory: the table hosts the puzzle, it does not host a factory simulator. Spelling follows the request (`Holloday`).

## What loads today

Station, line, and quota tokens load so the pack can be selected without crashing.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Gaps

Same hole as Origami Factory: stations, a line, quotas, and machine logic need scripting, placement UI, and a station scene. No sibling project was in this repo to slice, so the id is reserved. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
