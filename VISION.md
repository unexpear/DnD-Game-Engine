# D&D Game Engine — Product Vision

## One Sentence

A downloadable, multiplayer 3D tabletop RPG platform that runs custom D&D 5e campaigns, ships on Steam, and gives DMs the tools to build, run, and share adventures without writing code.

---

## The User Experience

### For Players

You launch the game on Steam. The main menu shows your character roster, active campaigns, and the LFG board. You join a friend's campaign or start a solo adventure.

You see a 3D rendered tabletop world: dungeons, towns, wilderness. Your character is a miniature on the map. The camera orbits freely — top-down tactical view or third-person over-the-shoulder. Terrain has height, walls block vision, torches cast light.

Turn-based combat plays out on this map. When it's your turn, UI panels show your available actions: Attack, Cast Spell, Dash, Hide, etc. Click an enemy to target. The dice roll animation plays — a 3D d20 spins and lands. Damage numbers float up. The combat log scrolls on the right.

Out of combat, you explore in real-time. Your character moves with WASD. The DM (or the AI-driven script) describes rooms, triggers traps, presents dialogue trees. Skill checks prompt with a hotkey and show your bonus, then roll.

Your character sheet is always one tab away: stats, inventory, spellbook, features. Drag items between your inventory and the ground. Equip gear and see it on your model.

Voice chat works through the same client. You see who's speaking via a portrait ring.

### For Dungeon Masters

You open the Dungeon Master client (same executable, DM mode). You see everything the players see, plus a toolkit sidebar:

- **Encounter Builder**: Drag monsters from the compendium onto the map. Set their HP, gear, behavior scripts. Click "Roll Initiative" to start combat.
- **Map Editor**: Paint terrain tiles, place walls/doors/chests, set lighting. Works in 2D top-down and 3D view.
- **Storyboard**: A node graph of scenes. Each node has a description, triggers, skill checks, loot tables. Link them to build branching adventures.
- **NPC Dialogue Editor**: Write dialogue trees with conditional branches based on quest flags, skills, reputation.
- **Rules Overrides**: Toggle optional rules (flanking, feats, multiclassing). Homebrew a feat or magic item with a form editor.

During a session, the DM sees player vision cones, hears their voice, and can spawn monsters mid-combat, apply conditions, or override any roll.

### For Solo Players

A built-in AI-driven "Dungeon Master" reads the adventure script, describes rooms, runs NPCs, and adjusts encounters on the fly. You play through published adventures or community-made campaigns solo, with full 3D immersion.

---

## The Platform

### Campaign Ecosystem

- **Published Adventures**: Buy or download free campaigns. Each is a `.dndcampaign` file containing the map, encounters, dialogue trees, loot tables, and scripts.
- **Workshop Integration**: Steam Workshop for sharing campaigns, homebrew content (items, monsters, classes), and tile/module packs.
- **Modding**: Asset packs, custom rules modules, UI themes. Lua scripting API for custom encounter logic.
- **Versioning**: Campaigns have version manifests. Players auto-sync to the DM's version on join.

### Technical Pillars

| Pillar | What It Means |
|--------|---------------|
| **3D Rendering** | OpenGL 4.6 core. Dynamic lighting, shadow maps, fog of war, particle effects (fireballs, magic missiles), animated character models with equipped gear visible. Runs at 60fps on a mid-range 2020 GPU. |
| **D&D 5e Rules** | Complete PHB, DMG, and MM rules compiled into the engine. Every class, race, spell, monster, feat, condition, and magic item. All calculations are authoritative — the engine enforces the rules. |
| **Multiplayer** | Peer-to-peer with an authority host (the DM). <50ms latency for roll resolution. State sync for entity positions, HP, conditions. Voice chat integrated. Reconnection handling. |
| **Audio** | 3D positional audio for spell effects, footsteps, ambient environments (dungeon echoes, forest birds, tavern chatter). Per-entity volume curves. Music system with combat/stress/exploration tracks. |
| **UI** | Retained-mode widget system. Fully skinnable. Character sheets, combat log, inventory grid, spellbook, party overview, DM toolkit. Gamepad navigation support. |
| **Save System** | Campaign state saves at any point. Full serialization of all entity state, quest flags, dialogue trees. Load and continue mid-combat. Auto-save on scene transitions. |
| **Scripting** | Lua embedded DSL for adventure logic: encounter triggers, conditional dialogue, custom win/lose conditions, boss phases. Sandboxed, hot-reloadable during DM prep. |
| **Accessibility** | Colorblind modes, scalable UI, screen reader support for chat and combat log, rebindable controls, controller support. |

### Steam Integration

- Friends list for inviting to campaigns
- Achievements for both players and DMs
- Cloud saves for campaign progress
- Workshop for community content
- Steam Voice API for chat
- Rich presence showing what campaign/adventure you're in

---

## What Ships

A single Windows executable (`DnDEngine.exe`) that boots into a launcher with two modes:

1. **Player Mode** — Join campaigns, manage characters, play.
2. **Dungeon Master Mode** — Build and run campaigns, access all toolkit features.

Optional dedicated server binary for headless campaign hosting.

A `.dndcampaign` file format, an editor, a Lua reference, and a community workshop.

No subscriptions. One purchase includes the engine, all core rules, the DM toolkit, and the solo AI DM. Adventure modules and asset packs are sold separately or free via Workshop.

---

## Who It's For

- **TTRPG veterans** who want a virtual tabletop with real 3D and automated rules
- **DMs** who want to build and share campaigns without coding
- **Solo players** who can't find a group but want the D&D experience
- **Streamers** who want a visually engaging way to run D&D on Twitch
- **Homebrewers** who want to mod a game with their own content
