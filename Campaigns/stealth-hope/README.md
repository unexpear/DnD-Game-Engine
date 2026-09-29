# Stealth Hope

Pack id: `stealth-hope`

## Source

Included from the platform brief so this title is not left out. This repository has no sibling survey to import, so the folder is a stub that still loads.

## Tabletop mapping

The infiltrator explores. Detection is a contested check on the session ruleset once a guard token can see them. A failed check becomes the table's combat beat. No stealth engine is ported.

## What loads today

Infiltrator, guard, and objective tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
