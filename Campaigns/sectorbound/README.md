# Sectorbound

Pack id: `sectorbound`

## Source

Voxel sandbox RPG. Themes come across as a campaign; the voxel sim does not.

## Tabletop mapping

A sector is the table for one session. The claim marker is the party's stake. An encounter token is what the explore loop walks toward. Block placement and chunk streaming stay in the original project.

## What loads today

Sector, claim, and encounter tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Gaps

A voxel sandbox needs chunk streaming, meshing, a block palette, edit tools, voxel collision, and a world save. The rigid-body spatial hash is not that world. This pack does not load any of it. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
