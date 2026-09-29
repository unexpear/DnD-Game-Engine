# Dead Center

Pack id: `dead-center`

## Source

Included from the platform brief so this title is not left out. This repository has no sibling survey to import, so the folder is a stub that still loads.

## Tabletop mapping

The center token is the ground the party holds. Pressure comes in as enemy tokens. The scene is a last stand using the table's combat beat. No separate horde sim is ported. The name is reserved so the title is not dropped.

## What loads today

Center, holder, and pressure tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
