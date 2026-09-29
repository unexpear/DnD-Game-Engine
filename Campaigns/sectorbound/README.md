# Sectorbound

Pack id: `sectorbound`

## Source

Voxel sandbox RPG. Themes come across as a campaign; the voxel sim does not.

## Tabletop mapping

A sector is the table for one session. The claim marker is the party's stake. An encounter token is what the explore loop walks toward. Block placement and chunk streaming stay in the original project.

## What loads today

Sector, claim, and encounter tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
