# Pirate Games

Pack id: `pirate-games`

## Source

Pirate sandbox, framed as a campaign of voyages.

## Tabletop mapping

The ship token is the party. Crew tokens are the characters. A port is a scene you sail toward — explore between ports, a check to negotiate or sneak, a fight when a sail is hostile. The sailing sandbox itself is not ported.

## What loads today

Ship, crew, and port tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
