# Superhero Vigilante

Pack id: `superhero-vigilante`

## Source

Desktop\IDEAS\superhero-vigilante-game

## Tabletop mapping

Patrol is explore. The vigilante token moves; a threat token is the encounter. A takedown is the table's combat beat. City simulation and powers from the idea folder are not ported yet.

## What loads today

Vigilante, threat, and block tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
