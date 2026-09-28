/**
 * @file dnd_demo_scene.h
 * @brief Comprehensive D&D 5e demo scene showcasing all game systems.
 * 
 * This demo scene creates a turn-based combat encounter between
 * a player party (Fighter, Wizard, Rogue, Cleric) and enemy
 * goblins. Demonstrates:
 *   - Character creation with races, classes, abilities, skills
 *   - Turn-based combat with initiative, actions, attacks
 *   - Dice rolling with advantage/disadvantage
 *   - Spellcasting (Wizard casting Fire Bolt)
 *   - Hit points, damage, healing
 *   - Equipment and inventory
 *   - UI overlays for character sheets and combat log
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.10.0
 */

#pragma once

#include "scene/scene.h"
#include "core/math.h"
#include "rendering/mesh.h"
#include "game/race.h"
#include "game/character_class.h"
#include "game/character_stats.h"
#include "game/combat.h"
#include "ai/pathfinding.h"
#include <vector>
#include <string>

namespace Dnd {

class UISlider;  // on-screen light sliders (created in SetupUI)
struct AiAction; // structured action returned by the local AI DM (see ai_brain.h)

/// @brief Types of dungeon traps.
enum class TrapType { PRESSURE_PLATE, SPIKE, TELEPORTER, LEVER, CRUMBLING_FLOOR };

/// @brief A single trap placed on the maze grid.
struct TrapData {
    Cell cell;              ///< Grid cell the trap occupies.
    TrapType type;          ///< What kind of trap.
    b8 active;              ///< Whether the trap is still active (can trigger).
    b8 triggered;           ///< Whether it has triggered recently (visual cooldown).

    // Shared fields
    f32 damage;             ///< Damage dealt (spikes, crumbling).
    f32 cooldownTimer;      ///< Seconds remaining before re-arming.
    f32 cooldownMax;        ///< Seconds between triggers.
    std::string triggerMsg; ///< Message shown when triggered.

    // Pressure-plate specific: which trap index to activate.
    i32 linkedTrap;

    // Teleporter specific: destination cell.
    Cell destCell;

    // Lever specific: wall cell to toggle (destroy/create wall entity).
    Cell leverWallCell;
    b8 leverState;          ///< false = lever up (wall up), true = lever down (wall removed).

    // Crumbling-floor specific: how long Finn has stood on it.
    f32 standTimer;
    f32 standThreshold;     ///< Seconds before the floor gives way (default 1.5).

    /// @brief Zero every field. Placement code only fills what it uses;
    ///        a garbage `triggered` or `cooldownTimer` used to swallow the trap.
    TrapData()
        : cell{-1, -1}
        , type(TrapType::SPIKE)
        , active(false)
        , triggered(false)
        , damage(0.0f)
        , cooldownTimer(0.0f)
        , cooldownMax(0.0f)
        , linkedTrap(-1)
        , destCell{-1, -1}
        , leverWallCell{-1, -1}
        , leverState(false)
        , standTimer(0.0f)
        , standThreshold(1.5f) {}
};

/**
 * @class DndDemoScene
 * @brief Full D&D 5e combat demo with characters, enemies, and UI.
 */
class DndDemoScene : public Scene {
public:
    DndDemoScene();

protected:
    void OnLoad() override;
    void OnUnload() override;
    void OnUpdate(f32 deltaTime) override;
    void OnRenderOverlay(f32 screenW, f32 screenH) override;

private:
    /// @brief Monster archetypes, one per dungeon floor (goblin / skeleton / bugbear).
    ///        Declared here (before CreateEnemy) so the method signature can use it.
    enum class EnemyType { GOBLIN, SKELETON, BUGBEAR };

    /**
     * @brief Creates a player character entity with full D&D stats.
     * @param name Character name.
     * @param race Character race.
     * @param classType Character class.
     * @param abilities Base ability scores.
     * @param position World position.
     */
    Entity CreateCharacter(const char* name, RaceType race, ClassType classType,
                           const AbilityScores& abilities, const Vec3& position);

    /**
     * @brief Creates an enemy entity with simplified stats.
     * @param name Enemy name.
     * @param level Challenge level.
     * @param position World position.
     */
    Entity CreateEnemy(const char* name, EnemyType type, i32 level, const Vec3& position);

    /**
     * @brief Creates a visual mesh entity (for 3D representation).
     * @param mesh Mesh to use.
     * @param position Position.
     * @param color Tint color.
     * @param scale Scale factor.
     */
    Entity CreateVisualEntity(Mesh* mesh, const Vec3& position, const Color& color, f32 scale = 1.0f);

    /**
     * @brief Sets up the UI overlay for the demo.
     */
    void SetupUI();

    // Mesh resources
    Mesh* m_cubeMesh;
    Mesh* m_quadMesh;

    // Player entities
    Entity m_fighter;
    Entity m_wizard;
    Entity m_rogue;
    Entity m_cleric;

    // Enemy entities
    Entity m_goblin1;
    Entity m_goblin2;
    Entity m_goblin3;

    // Camera
    Entity m_cameraEntity;

    // ---- Live-tunable key light (driven by the on-screen sliders) ----
    Vec3 m_lightPos;        ///< Key light world position.
    f32  m_lightIntensity;  ///< Key light brightness multiplier.

    // Slider widgets (owned by UISystem; not freed here).
    UISlider* m_lightSliderX;
    UISlider* m_lightSliderY;
    UISlider* m_lightSliderZ;
    UISlider* m_lightSliderI;

    /// @brief Rebuilds the key light from m_lightPos/m_lightIntensity and uploads it.
    void UpdateKeyLight();

    // ---- Combat HUD + paced "opening round" replay ----
    enum class Delivery { MELEE, BOLT, ARROW }; ///< How an attack reaches its target.
    CombatSystem* m_combatSystem;              ///< Cached combat system (paced queuing).
    std::vector<CombatAction> m_replayActions; ///< Scripted round, fed out one beat at a time.
    std::vector<Delivery> m_replayDelivery;    ///< Delivery per action (parallel to m_replayActions).
    std::vector<std::string> m_combatLog;      ///< Recent on-screen combat log lines.
    u32 m_replayIndex;                         ///< Next replay action to queue.
    f32 m_replayTimer;                         ///< Seconds accumulated toward the next action.

    // ---- Attack motion: a melee lunge OR a ranged projectile flying to the target ----
    enum class LungePhase { IDLE, OUT, BACK, FLY };
    LungePhase m_lungePhase;    ///< Current attack-motion state.
    f32        m_lungeT;        ///< Progress [0,1] within the current leg / flight.
    Vec3       m_lungeHome;     ///< Lunge start / projectile launch point.
    Vec3       m_lungeBump;     ///< Lunge bump / projectile impact point.
    Entity     m_lungeAttacker; ///< Melee attacker currently lunging.

    // Projectile placeholders: a sphere (fire bolt) and a capsule (arrow), reused per shot.
    Mesh*  m_boltMesh;
    Mesh*  m_arrowMesh;
    Entity m_boltProjectile;
    Entity m_arrowProjectile;
    Entity m_flyingProjectile;  ///< Whichever projectile is mid-flight.

    // Golden "turn" halo that sits under whoever is currently acting.
    Mesh*  m_haloMesh;
    Entity m_haloEntity;
    f32    m_haloTime;          ///< Accumulator for the halo's gentle pulse.

    // ---- Lock-pick interaction: the player issues intent; the rogue walks over,
    //      the engine works out the DEX check + DC, and the player rolls. ----
    enum class LockPhase { IDLE, APPROACH, AWAIT_ROLL, RETURN };
    Entity     m_chest;        ///< The locked chest the rogue can attempt.
    Entity     m_floor;        ///< The floor quad entity (recolored per floor).
    LockPhase  m_lockPhase;
    f32        m_lockT;        ///< Progress [0,1] of the walk to / from the chest.
    Vec3       m_lockHome;     ///< Rogue's position before the attempt.
    Vec3       m_lockSpot;     ///< Standing position just in front of the chest.
    i32        m_lockDexMod;   ///< Rogue's DEX modifier (for display).
    i32        m_lockProf;     ///< Proficiency bonus (thieves' tools).
    i32        m_lockMod;      ///< Total roll modifier (DEX + proficiency).
    i32        m_lockDC;       ///< Difficulty class to beat (set by the DM/AI).
    b8         m_chestOpen;    ///< Whether the chest has been opened.
    b8         m_awaitingAi;   ///< True while waiting on the local AI DM's reply.
    b8         m_pickPending;  ///< Walking to the chest, intending to pick its lock on arrival.

    // ---- Multi-floor dungeon ----
    static constexpr i32 kMaxFloors = 3;  ///< Total dungeon floors (Ruins, Cave, Catacombs).
    i32 m_currentFloor;                   ///< Current dungeon floor (0-indexed).
    b8 m_descentPending;                  ///< True while waiting to descend (post-chest).
    f32 m_descentTimer;                   ///< Countdown before auto-descent.

    void DescendFloor();                  ///< Clear current floor, generate the next depth.
    void ClearFloorEntities();            ///< Destroy all floor-specific entities.

    // ---- Maze + pathfinding (single-player dungeon-crawl test) ----
    static constexpr i32 kGrid   = 17;    ///< Full grid is kGrid x kGrid (odd: passages + walls).
    static constexpr f32 kCell   = 2.0f;  ///< World units per cell.
    static constexpr i32 kStartX = 1, kStartY = 1; ///< Finn's start cell.
    static constexpr i32 kExitX  = 15, kExitY = 15; ///< Goal cell (stairs / chest / boss).
    static constexpr i32 kRevealR = 4;    ///< Fog reveal radius (corridor steps) around Finn.
    b8  m_walk[kGrid][kGrid];             ///< true = walkable floor, false = wall.
    std::vector<Cell> m_path;             ///< Current navigation path (cells).
    u32 m_pathIndex;                      ///< Current segment along m_path.
    b8  m_walking;                        ///< Whether Finn is following a path.
    f32 m_walkT;                          ///< [0,1] progress along the current segment.

    // ---- Fog of war: the maze is hidden until Finn explores it ----
    b8  m_explored[kGrid][kGrid];         ///< Cells Finn has revealed.
    b8  m_chestFound;                     ///< Has the chest been discovered yet?
    b8  m_exploring;                      ///< Auto-exploring toward the nearest unknown.
    std::vector<Entity> m_walls;          ///< Wall entities (toggled by fog).
    std::vector<Cell>   m_wallCells;      ///< Each wall's cell (parallel to m_walls).

    void RevealAround(const Cell& c);     ///< Flood-reveal the corridor + bordering walls near c.
    void UpdateFogVisibility();           ///< Show only explored walls / found chest / seen foes.
    b8   FindNearestUnexplored(const Cell& from, Cell& out) const; ///< Nearest unseen floor cell.
    b8   StartExplore();                  ///< Begin walking toward the nearest unexplored cell.

    // ---- Enemies that hold the corridors and must be cut down (typed "attack") ----
    std::vector<Entity> m_enemies;        ///< Goblins placed along the route to the exit.
    std::vector<Cell>   m_enemyCells;     ///< Each enemy's cell (parallel to m_enemies).
    std::vector<b8>     m_enemyAlerted;   ///< Has this goblin spotted Finn? (0 = oblivious).
    std::vector<b8>     m_enemyContested; ///< Has its sight/Stealth contest been rolled yet?
    std::vector<EnemyType> m_enemyTypes;  ///< Type of each enemy (parallel to m_enemies, see EnemyType above).
    Entity m_blockingEnemy;               ///< Living goblin currently halting Finn (or None()).
    b8     m_mazeAttack;                  ///< True while a maze melee lunge is resolving.
    Entity m_attackTarget;                ///< Target of the in-progress maze attack.
    static constexpr i32 kSight = 5;      ///< Goblin sight range, in cells.

    std::vector<Entity> m_scriptInited;   ///< Enemies that have received their script init event.
    i32  EnemyIndex(Entity e) const;      ///< Index of an enemy entity in m_enemies, or -1.
    b8   HasLineOfSight(const Cell& a, const Cell& b) const; ///< Clear (wall-free) grid sight.
    void CheckEnemyDetection();           ///< Roll Stealth vs Perception for goblins now in view.

    void GenerateMaze();                  ///< Carve a perfect maze into m_walk.
    void BuildMazeWalls();                ///< Spawn a wall cube for each blocked cell.
    void PlaceMazeEnemies();              ///< Spawn goblins on cells along the route.
    Vec3 CellToWorld(i32 cx, i32 cy) const;
    Cell WorldToCell(const Vec3& p) const;
    b8 Walkable(i32 x, i32 y) const;
    Entity LivingEnemyAt(const Cell& c) const; ///< A living goblin on cell c, else None().
    b8 RepathToExit();                    ///< (Re)compute + start the A* path to the chest.
    void ResolveMazeAttack();             ///< Apply Finn's strike (the foe acts on their own turn).

    // ---- Corridor encounter: explore pauses, turns resolve, then explore resumes ----
    enum class DemoOutcome { ONGOING, VICTORY, DEFEAT };
    DemoOutcome m_outcome = DemoOutcome::ONGOING;
    b8 m_inEncounter = false;             ///< Turn order is live.
    Entity m_encounterFoe = Entity::None();
    b8 m_ambush = false;                  ///< Foe hadn't noticed Finn when the fight started.
    b8 m_awaitingPlayer = false;          ///< Finn's turn: waiting for attack / disengage / drink.
    b8 m_strikeIsEnemy = false;           ///< The in-progress lunge is the foe's attack.
    f32 m_turnDelay = 0.0f;               ///< Seconds before an enemy turn auto-resolves.

    const char* ActorName(Entity e) const;
    b8 IsFinalFloor() const { return m_currentFloor >= kMaxFloors - 1; }
    void BeginEncounter(Entity foe);
    void AnnounceTurn();
    void EndEncounter(b8 resumeTravel);
    void FinishCombatAction();
    void StartEnemyStrike();
    void ResolveEnemyStrike();
    void CancelMotion();
    void OnVictory();
    void OnDefeat(const char* how);
    void HandleExitArrival();

    // ---- Loot on the maze floor: hidden until a Perception (WIS) check reveals it ----
    std::vector<Entity> m_items;          ///< Loot entities (small glint spheres).
    std::vector<Cell>   m_itemCells;      ///< Each item's cell (parallel to m_items).
    std::vector<i32>    m_itemDC;         ///< Perception DC to spot each item.
    std::vector<b8>     m_itemFound;      ///< Revealed by a successful search.
    std::vector<b8>     m_itemTaken;      ///< Already picked up.
    Mesh* m_itemMesh;                     ///< Small sphere mesh shared by all loot.

    void PlaceMazeItems();                ///< Scatter hidden loot along the route.
    void DoPerceptionSearch();            ///< WIS (Perception) check; reveal nearby hidden loot.
    void DoPickup();                      ///< Grab every revealed item within reach.

    // ---- Procedural humanoid rigs: head/torso/arms/legs that walk, idle, attack ----
    struct CharacterRig {
        Entity root;                      ///< The character entity whose Transform drives the rig.
        Entity head, torso, armL, armR, legL, legR; ///< Body parts (positioned in world each frame).
        f32    yaw;                       ///< Current facing (radians), eased toward movement/target.
        f32    phase;                     ///< Gait cycle phase (advances with distance walked).
        Vec3   lastPos;                   ///< Root position last frame (for movement detection).
        b8     isPlayer;                  ///< Player is always shown; goblins follow fog + life.
    };
    std::vector<CharacterRig> m_rigs;     ///< One rig per character (Finn + goblins).
    Mesh* m_headMesh;                     ///< Shared sphere mesh for heads.
    Mesh* m_limbMesh;                     ///< Shared capsule mesh for arms/legs.
    f32   m_animTime;                     ///< Accumulator for idle motion.

    void BuildRig(Entity root, const Color& skin, b8 isPlayer); ///< Spawn a humanoid rig for a character.
    void AnimateRigs(f32 dt);             ///< Pose every rig from its root + animation state.
    void SetRigVisible(const CharacterRig& rig, b8 visible);    ///< Toggle all of a rig's parts.

    /// @brief Feed the moving characters + nearby walls to the shadow caster each frame.
    void UpdateShadows();

    // ---- Trap system: pressure plates, spikes, teleporters, levers, crumbling floors ----
    std::vector<TrapData> m_traps;          ///< All traps on the current floor.
    std::vector<Entity>   m_trapMarkers;    ///< Visual marker entities for traps.
    static constexpr i32 kTrapRevealR = 3;  ///< Cells within which Finn auto-detects a trap (Perception).
    static constexpr f32 kTrapPerceptionDC = 14; ///< Perception DC to spot a trap on reveal.

    void PlaceTraps();                      ///< Scatter traps along the maze.
    void ClearTrapEntities();               ///< Destroy trap marker entities.
    i32  TrapAt(i32 x, i32 y) const;        ///< Index of a trap at cell (x,y), or -1.
    void TriggerTrap(i32 idx);              ///< Activate a trap's effect.
    void CheckTraps(f32 dt);                ///< Frame-by-frame trap logic (cooldowns, crumbling).

    /// @brief Have the rogue path through the maze to the chest and attempt the lock.
    void TryStartLockPick(i32 dc = 15, const std::string& flavor = "");

    /// @brief Enter the DEX / thieves'-tools roll phase once Finn is at the chest.
    void BeginLockRoll();

    /// @brief Player command entry point: try the deterministic rules first, else the AI DM.
    void Command(const std::string& intent);

    /// @brief Deterministic keyword parser: text -> a known action. False if unrecognized.
    b8 ParseIntentLocal(const std::string& text, AiAction& out);

    /// @brief Run a resolved action (lock-pick animates; other actions are abstract checks).
    void ExecuteAction(const AiAction& action, b8 fromAi);

    /// @brief Hand an unrecognized intent to the local LLM DM (async fallback).
    void AskAi(const std::string& intent);

    /// @brief Resolve a non-movement action as an abstract ability check.
    void GenericCheck(const AiAction& action);

    /// @brief Appends a line to the on-screen combat log (keeps only the most recent few).
    void PushLog(const std::string& line);
};

} // namespace Dnd
