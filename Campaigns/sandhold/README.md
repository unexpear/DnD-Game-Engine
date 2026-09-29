# Sandhold

Pack id: `sandhold`

## Source

Wasteland squad survival, played here as a campaign rather than a separate sim.

## Tabletop mapping

The squad is a handful of tokens. The camp is the table's home cell. Scarcity and travel are checks on the session ruleset, then a fight uses the table's combat beat when something catches the squad. Inventory simulation from the original is not ported.

## What loads today

Three squad-and-camp tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Gaps

Squad survival needs needs-meters, inventory, a camp save, a travel map, and squad pathing. A check and a few tokens are what the table can do. This pack's ruleset does not enter the demo combat loop. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
