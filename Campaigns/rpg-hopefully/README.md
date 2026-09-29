# RPG Hopefully

Pack id: `rpg-hopefully`

## Source

C:\Users\micha\source\repos\rpg game hopefuly

## Tabletop mapping

The original is a dice-and-class cousin of this table, not a separate tabletop. Classes become miniatures around the table. Checks stay d20-shaped, described by the session ruleset binding, and do not rewrite the engine dice tables. A later pass can map each class onto the existing character sheet instead of inventing a second combat loop.

## What loads today

This is the sample secondary pack. It loads `campaign_table` with four class tokens. Placeholder geometry only — the hopeful repo is not imported.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
