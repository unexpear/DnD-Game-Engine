# Dwarf Fortress x RuneScape

Pack id: `dwarf-fortress-runescape`

## Source

Desktop\IDEAS\dwarf-fortress-runescape

## Tabletop mapping

The fort is the home scene. The delve is explore underground. A caravan token is the overworld link between them, in the RuneScape sense of leaving town. The fortress sim and the MMO loop are not ported; the campaign is the reason you go down and come back.

## What loads today

Fort, delve, and caravan tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
