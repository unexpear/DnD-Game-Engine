# Sandhold

Pack id: `sandhold`

## Source

Wasteland squad survival, played here as a campaign rather than a separate sim.

## Tabletop mapping

The squad is a handful of tokens. The camp is the table's home cell. Scarcity and travel are checks on the session ruleset, then a fight uses the table's combat beat when something catches the squad. Inventory simulation from the original is not ported.

## What loads today

`SandholdScene` builds a sandy courtyard: a sand grid, low dunes, a tent, a crate, a torch, a stone altar, a campfire, and one warden. While this pack is active, WASD walks the warden relative to the view. Dunes, walls, and props stop the warden with a box test in the scene. F at the campfire rests the warden (brace +2) and brightens the fire. F at the north gate rolls a d20 plus that bonus against DC 12 using `DiceRoller`. Success lifts the dust bar so the warden can walk to the cairn. F3 switches the tabletop orbit and a camera behind the warden. The survival repository is not imported.

Ruleset module: `tabletop-generic`. Entry scene: `sandhold`. Control scheme: `sandhold` (WASD belongs to the warden until the pack unloads). `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path. `tableTokens` still name Scout, Medic, and Camp; the scene places its own props.

## Gaps

Still missing if squad survival were rebuilt here: needs meters, inventory, a camp that persists after unload, a travel map, more than one squad member, and squad pathing. This courtyard is a fixed set of cubes. The brace is `DiceRoller`, and the pack does not enter the demo combat loop. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
