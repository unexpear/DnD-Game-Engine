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

This directory is the only content root for the pack. It cannot replace engine defaults, the D&D demo, or another pack. Switching away drops this session's manifest and ruleset binding. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
