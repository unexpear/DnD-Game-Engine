# RPG Hopefully

Pack id: `rpg-hopefully`

## Source

C:\Users\micha\source\repos\rpg game hopefuly

## Tabletop mapping

The original is a dice-and-class cousin of this table, not a separate tabletop. Classes become miniatures around the table. Checks stay d20-shaped, described by the session ruleset binding, and do not rewrite the engine dice tables. A later pass can map each class onto the existing character sheet instead of inventing a second combat loop.

## What loads today

`RpgHopefullyScene` builds a practice yard: a floor, four posts, and a dummy. Four class figures stand on it — Hopeful (CHA +2), Warden (STR +3), Hexer (INT +3), Chorus (WIS +1). While this pack is active, WASD walks the selected class relative to the view, Q/E or Tab switches class, and R or Space rolls a d20 plus that modifier against DC 12 using `DiceRoller`. F3 switches the tabletop orbit and a camera behind the figure. The hopeful repository is not imported; this is the playable slice of that dice-and-class fantasy.

Ruleset module: `tabletop-generic`. Entry scene: `rpg_hopefully`. Control scheme: `rpg-hopefully` (WASD belongs to the yard until the pack unloads). `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path. `tableTokens` describe the four figures; the scene places its own meshes.

## Gaps

Still missing if the hopeful game were rebuilt here: its real class list, items, and levels; sheets and progression; skinned characters (the figures are cubes); a dummy that reacts; inventory, dialogue, save, and the original UI. Modifiers are four constants in the scene. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
