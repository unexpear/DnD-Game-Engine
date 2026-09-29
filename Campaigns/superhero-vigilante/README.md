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

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
