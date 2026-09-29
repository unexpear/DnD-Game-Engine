# Hunting

Pack id: `hunting`

## Source

Included from the platform brief so this title is not left out. This repository has no sibling survey to import, so the folder is a stub that still loads.

## Tabletop mapping

The hunt is explore toward a quarry token. Tracking is a check. The shot or the struggle is the combat beat. Animal simulation and ballistics are not ported.

## What loads today

Hunter, trail, and quarry tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Gaps

Hunting needs animal behavior, tracks as state, outdoor terrain, and ballistics. The immersed camera sits behind one subject; it is not a scope. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
