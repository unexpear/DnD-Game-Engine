# Sectorbound

Pack id: `sectorbound`

## Source

Voxel sandbox RPG. Themes come across as a campaign; the voxel sim does not.

## Tabletop mapping

A sector is the table for one session. The claim marker is the party's stake. An encounter token is what the explore loop walks toward. Block placement and chunk streaming stay in the original project.

## What loads today

`SectorboundScene` builds a block outpost: a floor grid, a wall with a gate, crates, a beacon, a gold survey marker, and one scout. While this pack is active, WASD walks the scout relative to the view. Walls and crates stop the scout with a box test in the scene. F next to the marker stows it (scan +2). F next to the beacon rolls a d20 plus that bonus against DC 12 using `DiceRoller`. Success lifts the gate so the scout can walk to the far side of the sector. F3 switches the tabletop orbit and a camera behind the scout. The voxel repository is not imported.

Ruleset module: `tabletop-generic`. Entry scene: `sectorbound`. Control scheme: `sectorbound` (WASD belongs to the scout until the pack unloads). `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path. `tableTokens` still name Sector, Claim, and Encounter; the scene places its own blocks.

## Gaps

Still missing if the voxel sandbox were rebuilt here: chunk streaming, meshing, a block palette, place and dig tools, edits that change the volume, a world save, and collision that follows those edits. This yard is a fixed set of cubes. The scan is `DiceRoller`, not a sector rules module. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
