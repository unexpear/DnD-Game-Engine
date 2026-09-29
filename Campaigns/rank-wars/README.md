# Rank Wars

Pack id: `rank-wars`

## Source

World Trigger rank battles, staged as a campaign encounter.

## Tabletop mapping

One side of the table is the attacker's tokens, the other is the defender's. The rank badge marks the match. A round is the existing combat beat. Trigger loadouts can later be items; the anime sim is not reimplemented.

## What loads today

Attacker, defender, and rank tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
