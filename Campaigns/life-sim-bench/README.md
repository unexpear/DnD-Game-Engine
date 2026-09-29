# Life Sim Bench

Pack id: `life-sim-bench`

## Source

Simulation labs. Scenarios are framed as table encounters when a full sim is the wrong host.

## Tabletop mapping

A lab is one scenario: a token, a short brief in the summary, and a check or two. The bench can host several labs as several packs later; this pack is the bench itself. No simulation harness is embedded.

## What loads today

Lab A, Lab B, and the bench token load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
