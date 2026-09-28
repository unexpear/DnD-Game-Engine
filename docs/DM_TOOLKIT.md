# DM Toolkit

The engine's editor (the same dock used by Hierarchy, Inspector, and the Asset Browser) now includes an **Encounter** panel. This is the first dungeon-master slice from the product vision: a roster you can run in prep or mid-session. It is not the full toolkit.

## Open it

1. Launch the demo (`DnDEngine`).
2. Press **F5** to toggle editor mode. The Encounter panel floats over the viewport, to the right of the hierarchy.
3. Press **F7** to jump straight into the editor with the Encounter panel shown. While the editor is already open, **F7** hides or shows that panel.
4. Drag the **Encounter** title bar to move the panel. Drop it on a dock edge to snap it beside the other tools. The close box hides it until the next **F7**.

Editor mode replaces the play HUD until you press **F5** again. Spawned monsters stay in the scene.

## What you can do

The catalog is the same three archetypes the demo maze already places: **Goblin** (9 HP, AC 12), **Skeleton** (14 HP, AC 13), and **Bugbear** (19 HP, AC 14).

| Action | What happens |
| --- | --- |
| Goblin / Skeleton / Bugbear | Adds one to the roster and spawns a miniature next to Finn. It shows up in the Hierarchy under that name. The translate gizmo (**1**) can slide it into place. |
| Party: On / Off | When on, **Roll Init** also rolls living friendly characters already in the scene (Finn, in the demo). Their DEX modifier comes from `CharacterInfo`. |
| Roll Init | Rolls a d20 + DEX modifier for every living roster member (and the party, if included). Higher totals go first. Ties break toward the lower id. |
| Roster row | Selects that creature for the buttons below and for the inspector / gizmo. |
| Dmg 5 / Heal 5 | Changes current HP through `HitPoints::TakeDamage` and `HitPoints::Heal`. Neither can leave the 0–max range. Heal does add HP from 0, matching that component. |
| Full HP | Sets current HP to the creature's max. |
| Remove | Deletes the miniature and drops it from the turn order. |
| Start | Begins combat using the last initiative order. The status line names whose turn it is. That creature's miniature brightens and stands a little taller. |
| Next Turn | Advances the order, wrapping after the last combatant. |
| End | Stops combat. The rolled order stays, so Start can be pressed again without re-rolling. |

A miniature darkens and shrinks as it loses HP. At 0 HP it falls flat. While combat is running, a downed roster creature is skipped; if you heal them, they keep the initiative they already rolled. If everyone on the roster is down, combat stops, and **Start** works again after someone has HP. A fresh **Roll Init** still skips anyone left at 0.

Initiative is tracked by the encounter roster. Starting combat copies that order onto the scene's `CombatSystem` for the creatures in the encounter (and Finn, when Party is on). It does **not** call `CombatState::RollInitiative()`, which would re-roll every entity in the scene.

## Still not in this slice

From the vision's DM toolkit, these are unchanged:

- **Encounter builder, rest of it.** No drag-from-compendium, gear, or behavior scripts. The catalog is three stat blocks, not a monster manual. You cannot type an arbitrary HP value; use Dmg / Heal / Full HP.
- **Map editor.** No terrain paint, walls, doors, or lighting tools. Move a spawned miniature with the existing translate gizmo.
- **Storyboard.** No scene graph, triggers, or loot tables.
- **NPC dialogue editor.** No dialogue trees.
- **Rules overrides.** No flanking / feats / multiclass toggles and no homebrew editor.
- **In-session extras.** No forced die result, conditions, fog toggle, or vision cones. Fog of war in the demo is unchanged.

## Tests

Roster and initiative rules are covered without a window:

```bash
g++ -std=c++17 -Wall -Wextra -Iinclude \
    tests/encounter_builder_test.cpp \
    src/tools/encounter_builder.cpp \
    -o build/encounter_builder_test
./build/encounter_builder_test
```
