# Icarus-like Survival

Pack id: `icarus-like-survival`

## Source

Desktop\IDEAS\icarus-like-survival

## Tabletop mapping

A session is a drop. The lander token is the extract. The storm is a scenario clock described in the summary, not a weather sim. Explore is the time on the ground; a fight is whatever is already in the prospect. Crafting trees stay in the idea folder until a later pack revision.

## What loads today

Lander, prospector, and storm tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
