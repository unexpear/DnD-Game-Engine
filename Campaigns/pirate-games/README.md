# Pirate Games

Pack id: `pirate-games`

## Source

Pirate sandbox, framed as a campaign of voyages.

## Tabletop mapping

The ship token is the party. Crew tokens are the characters. A port is a scene you sail toward — explore between ports, a check to negotiate or sneak, a fight when a sail is hostile. The sailing sandbox itself is not ported.

## What loads today

Ship, crew, and port tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Gaps

Sailing needs water, buoyancy, a ship controller, wind, a voyage map, and crew state. There is no vehicle or fluid system. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
