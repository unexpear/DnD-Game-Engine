# rpggame

Pack id: `rpggame`

## Source

D:\dev\rpggame — first-person extraction RPG built in Bevy.

## Tabletop mapping

The Bevy project stays where it is. On this table a raid is a scene: an operative token, an extraction marker, and a hazard. Movement and shooting from the FPS game are not ported. Explore is walking the token; a fight is the table's combat beat; getting out is the scene goal.

## What loads today

Three placeholder tokens load on `campaign_table`. No Bevy runtime, levels, or assets are pulled in.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
