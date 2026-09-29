# Stealth Hope

Pack id: `stealth-hope`

## Source

Included from the platform brief so this title is not left out. This repository has no sibling survey to import, so the folder is a stub that still loads.

## Tabletop mapping

The infiltrator explores. Detection is a contested check on the session ruleset once a guard token can see them. A failed check becomes the table's combat beat. No stealth engine is ported.

## What loads today

Infiltrator, guard, and objective tokens load as placeholders.

Ruleset module: `tabletop-generic`. Entry scene: `campaign_table`. `entryScript` is empty and is not executed. `assetsRoot` is this folder. The loader does not add it to the engine asset search path.

## Gaps

Stealth needs vision cones, light that changes detection, noise, patrols, and cover. Demo occluders are maze shadows, not a stealth model. `AiBrain` is not a pack service. The tokens do not see each other. Details: [docs/ENGINE_GAPS.md](../../docs/ENGINE_GAPS.md).

## Isolation

One campaign is on the table at a time. Choosing another pack unloads this one, then loads that pack. This folder cannot change engine defaults. See [docs/CAMPAIGNS.md](../../docs/CAMPAIGNS.md).
