/**
 * @file dnd_demo_scene.cpp
 * @brief Implementation of the D&D demo scene.
 * 
 * See dnd_demo_scene.h for full documentation.
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.10.0
 */

#include "scene/dnd_demo_scene.h"
#include "core/log.h"
#include "core/math.h"
#include "rendering/mesh.h"
#include "rendering/debug_draw.h"
#include "rendering/renderer.h"
#include "ecs/components/transform.h"
#include "ecs/components/camera.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/tag.h"
#include "ecs/components/rigidbody.h"
#include "ecs/systems/transform_system.h"
#include "ecs/systems/render_system.h"
#include "ecs/systems/script_system.h"
#include "audio/audio_system.h"
#include "audio/audio_listener.h"
#include "game/dice.h"
#include "game/character_stats.h"
#include "game/race.h"
#include "game/character_class.h"
#include "game/combat_stats.h"
#include "game/combat.h"
#include "game/spell.h"
#include "game/progression.h"
#include "game/item.h"
#include "game/inventory.h"
#include "game/rules.h"
#include "ui/ui_system.h"
#include "ui/ui_widget.h"
#include "ui/ui_renderer.h"
#include "core/time.h"
#include "core/input.h"
#include "core/engine.h"
#include "ai/ai_brain.h"
#include "ai/pathfinding.h"
#include <cstdio>
#include <cctype>
#include <cstring>
#include <cmath>
#include <utility>
#include <algorithm>

namespace Dnd {

namespace {
// Linear interpolation between two points.
Vec3 LerpVec(const Vec3& a, const Vec3& b, f32 t) {
    return Vec3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}
// Smooth ease-in/out on [0,1] so the dash accelerates and settles.
f32 SmoothStep01(f32 t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}
// Builds a capsule mesh along the +Y axis. halfHeight 0 yields a sphere.
// Outward normals + CCW winding so it works with back-face culling. Heap-allocated.
Mesh* BuildCapsuleMesh(f32 radius, f32 halfHeight, i32 rings, i32 sectors) {
    const f32 PI = 3.14159265358979f;
    std::vector<Vertex> verts;
    std::vector<u32> indices;
    for (i32 i = 0; i <= rings; ++i) {
        const f32 lat = PI * static_cast<f32>(i) / static_cast<f32>(rings); // 0=top .. PI=bottom
        const f32 sinLat = std::sin(lat), cosLat = std::cos(lat);
        const f32 yOff = (i * 2 <= rings) ? halfHeight : -halfHeight;       // split the sphere
        for (i32 j = 0; j <= sectors; ++j) {
            const f32 lon = 2.0f * PI * static_cast<f32>(j) / static_cast<f32>(sectors);
            const Vec3 n(sinLat * std::cos(lon), cosLat, sinLat * std::sin(lon));
            Vertex v;
            v.position = Vec3(radius * n.x, radius * n.y + yOff, radius * n.z);
            v.normal   = n;
            v.uv       = Vec2(static_cast<f32>(j) / static_cast<f32>(sectors),
                              static_cast<f32>(i) / static_cast<f32>(rings));
            v.color    = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            verts.push_back(v);
        }
    }
    const i32 stride = sectors + 1;
    for (i32 i = 0; i < rings; ++i) {
        for (i32 j = 0; j < sectors; ++j) {
            const u32 a = static_cast<u32>(i * stride + j);
            const u32 b = static_cast<u32>((i + 1) * stride + j);
            indices.push_back(a);     indices.push_back(b); indices.push_back(a + 1);
            indices.push_back(a + 1); indices.push_back(b); indices.push_back(b + 1);
        }
    }
    Mesh* mesh = new Mesh();
    mesh->Create(verts.data(), static_cast<i32>(verts.size()),
                 indices.data(), static_cast<i32>(indices.size()));
    return mesh;
}

// Builds a flat ring (annulus) in the XZ plane, double-sided so culling never
// hides it regardless of camera angle. Used for the golden turn halo.
Mesh* BuildRingMesh(f32 innerR, f32 outerR, i32 segments) {
    const f32 PI = 3.14159265358979f;
    std::vector<Vertex> verts;
    std::vector<u32> indices;
    for (i32 s = 0; s <= segments; ++s) {
        const f32 ang = 2.0f * PI * static_cast<f32>(s) / static_cast<f32>(segments);
        const f32 c = std::cos(ang), sn = std::sin(ang);
        Vertex vo; vo.position = Vec3(outerR * c, 0.0f, outerR * sn);
        vo.normal = Vec3(0.0f, 1.0f, 0.0f); vo.uv = Vec2(static_cast<f32>(s) / segments, 1.0f);
        vo.color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        Vertex vi; vi.position = Vec3(innerR * c, 0.0f, innerR * sn);
        vi.normal = Vec3(0.0f, 1.0f, 0.0f); vi.uv = Vec2(static_cast<f32>(s) / segments, 0.0f);
        vi.color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        verts.push_back(vo); verts.push_back(vi);
    }
    for (i32 s = 0; s < segments; ++s) {
        const u32 o0 = static_cast<u32>(2 * s),     i0 = static_cast<u32>(2 * s + 1);
        const u32 o1 = static_cast<u32>(2 * (s + 1)), i1 = static_cast<u32>(2 * (s + 1) + 1);
        indices.push_back(o0); indices.push_back(i0); indices.push_back(o1); // top
        indices.push_back(o1); indices.push_back(i0); indices.push_back(i1);
        indices.push_back(o0); indices.push_back(o1); indices.push_back(i0); // underside
        indices.push_back(o1); indices.push_back(i1); indices.push_back(i0);
    }
    Mesh* mesh = new Mesh();
    mesh->Create(verts.data(), static_cast<i32>(verts.size()),
                 indices.data(), static_cast<i32>(indices.size()));
    return mesh;
}
} // namespace

DndDemoScene::DndDemoScene()
    : Scene("DndDemoScene"),
      m_cubeMesh(nullptr), m_quadMesh(nullptr),
      m_fighter(Entity::None()), m_wizard(Entity::None()),
      m_rogue(Entity::None()), m_cleric(Entity::None()),
      m_goblin1(Entity::None()), m_goblin2(Entity::None()),
      m_goblin3(Entity::None()), m_cameraEntity(Entity::None()),
      m_lightPos(-7.0f, 14.0f, -5.0f), m_lightIntensity(2.4f),
      m_lightSliderX(nullptr), m_lightSliderY(nullptr),
      m_lightSliderZ(nullptr), m_lightSliderI(nullptr),
      m_combatSystem(nullptr), m_replayIndex(0), m_replayTimer(0.0f),
      m_lungePhase(LungePhase::IDLE), m_lungeT(0.0f),
      m_lungeHome(0.0f, 0.0f, 0.0f), m_lungeBump(0.0f, 0.0f, 0.0f),
      m_lungeAttacker(Entity::None()),
      m_boltMesh(nullptr), m_arrowMesh(nullptr),
      m_boltProjectile(Entity::None()), m_arrowProjectile(Entity::None()),
      m_flyingProjectile(Entity::None()),
      m_haloMesh(nullptr), m_haloEntity(Entity::None()), m_haloTime(0.0f),
      m_chest(Entity::None()), m_floor(Entity::None()), m_lockPhase(LockPhase::IDLE), m_lockT(0.0f),
      m_lockHome(0.0f, 0.0f, 0.0f), m_lockSpot(0.0f, 0.0f, 0.0f),
      m_lockDexMod(0), m_lockProf(0), m_lockMod(0), m_lockDC(0), m_chestOpen(false),
      m_awaitingAi(false), m_pickPending(false),
      m_pathIndex(0), m_walking(false), m_walkT(0.0f),
      m_blockingEnemy(Entity::None()), m_mazeAttack(false),
      m_attackTarget(Entity::None()), m_itemMesh(nullptr),
      m_currentFloor(0), m_descentPending(false), m_descentTimer(0.0f),
      m_chestFound(false), m_exploring(false),
      m_headMesh(nullptr), m_limbMesh(nullptr), m_animTime(0.0f) {
}

void DndDemoScene::OnLoad() {
    Scene::OnLoad();

    // Step 1: Initialize D&D databases
    ItemDatabase::Initialize();
    DiceRoller::Initialize();

    // Step 2: Create meshes
    m_cubeMesh = new Mesh(Mesh::CreateCube());
    {
        const f32 floorSize = (kGrid + 4) * kCell; // covers the whole (larger) maze
        // Use a tiling quad so the checker texture repeats across the floor
        const f32 tileScale = 4.0f; // texture repeats every 4 world units
        const f32 uvMax = floorSize / (kCell * tileScale);
        m_quadMesh = new Mesh();
        Vertex verts[4];
        verts[0].position = Vec3(-floorSize * 0.5f, 0,  floorSize * 0.5f);
        verts[0].normal = Vec3(0, 1, 0);
        verts[0].uv = Vec2(0, 0);
        verts[1].position = Vec3( floorSize * 0.5f, 0,  floorSize * 0.5f);
        verts[1].normal = Vec3(0, 1, 0);
        verts[1].uv = Vec2(uvMax, 0);
        verts[2].position = Vec3( floorSize * 0.5f, 0, -floorSize * 0.5f);
        verts[2].normal = Vec3(0, 1, 0);
        verts[2].uv = Vec2(uvMax, uvMax);
        verts[3].position = Vec3(-floorSize * 0.5f, 0, -floorSize * 0.5f);
        verts[3].normal = Vec3(0, 1, 0);
        verts[3].uv = Vec2(0, uvMax);
        u32 idx[6] = { 0, 1, 2, 0, 2, 3 };
        m_quadMesh->Create(verts, 4, idx, 6);
    }

    // Humanoid rig parts: a sphere head and capsule limbs (shared across all characters).
    m_headMesh = BuildCapsuleMesh(0.17f, 0.0f,  12, 16); // sphere
    m_limbMesh = BuildCapsuleMesh(0.09f, 0.18f,  8, 12); // capsule

    // Projectile placeholders for ranged attacks: a sphere (fire bolt) and a thin
    // capsule (arrow). Built once, reused per shot, parked off-screen when idle.
    m_boltMesh  = BuildCapsuleMesh(0.22f, 0.0f,  12, 18); // sphere (halfHeight 0)
    m_arrowMesh = BuildCapsuleMesh(0.07f, 0.34f,  8, 12); // long thin capsule
    {
        auto makeProjectile = [&](Mesh* mesh, const Color& col, const char* tag) -> Entity {
            Entity e = m_entityManager.Create();
            m_componentRegistry.AddComponent<Tag>(e, Tag(tag));
            m_componentRegistry.AddComponent<Transform>(e, Transform(
                Vec3(0.0f, -100.0f, 0.0f), Quat::Identity(), Vec3(1.0f, 1.0f, 1.0f)));
            MeshRenderer mr(mesh, col);
            mr.visible = false; // shown only while in flight
            m_componentRegistry.AddComponent<MeshRenderer>(e, mr);
            return e;
        };
        m_boltProjectile  = makeProjectile(m_boltMesh,  Color(1.00f, 0.55f, 0.15f, 1.0f), "FireBolt");
        m_arrowProjectile = makeProjectile(m_arrowMesh, Color(0.82f, 0.80f, 0.62f, 1.0f), "Arrow");
    }

    // Golden "turn" halo: a glowing ring that sits under whoever is currently acting.
    m_haloMesh = BuildRingMesh(0.55f, 0.95f, 48);
    {
        m_haloEntity = m_entityManager.Create();
        m_componentRegistry.AddComponent<Tag>(m_haloEntity, Tag("TurnHalo"));
        m_componentRegistry.AddComponent<Transform>(m_haloEntity, Transform(
            Vec3(0.0f, -100.0f, 0.0f), Quat::Identity(), Vec3(1.0f, 1.0f, 1.0f)));
        MeshRenderer hmr(m_haloMesh, Color(1.0f, 0.84f, 0.30f, 1.0f)); // warm gold
        hmr.visible = false;
        m_componentRegistry.AddComponent<MeshRenderer>(m_haloEntity, hmr);
    }

    // A locked chest off to the side for the rogue to attempt (the lock-pick demo).
    {
        m_chest = m_entityManager.Create();
        m_componentRegistry.AddComponent<Tag>(m_chest, Tag("LockedChest"));
        m_componentRegistry.AddComponent<Transform>(m_chest, Transform(
            Vec3(6.5f, 0.4f, -1.0f), Quat::Identity(), Vec3(1.2f, 0.8f, 0.8f)));
        m_componentRegistry.AddComponent<MeshRenderer>(m_chest,
            MeshRenderer(m_cubeMesh, Color(0.40f, 0.26f, 0.13f, 1.0f))); // wooden chest
    }

    // Step 3: Register systems
    m_systemManager.RegisterSystem(
        std::make_unique<TransformSystem>(),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::make_unique<CombatSystem>(),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::make_unique<SpellcastingSystem>(),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::make_unique<RenderSystem>(nullptr),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::make_unique<ScriptSystem>(),
        m_entityManager, m_componentRegistry);

    m_systemManager.RegisterSystem(
        std::make_unique<AudioSystem>(),
        m_entityManager, m_componentRegistry);

    // Step 4: Create camera
    m_cameraEntity = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_cameraEntity, Tag("MainCamera"));
    m_componentRegistry.AddComponent<Transform>(m_cameraEntity, Transform(
        Vec3(0, 8, 12), Quat::LookAt(Vec3(0, 8, 12), Vec3(0, 0, 0), Vec3(0, 1, 0)), Vec3(1, 1, 1)));
    m_componentRegistry.AddComponent<ECSCamera>(m_cameraEntity, ECSCamera());
    m_componentRegistry.AddComponent<AudioListener>(m_cameraEntity, AudioListener());

    // Step 5: Create floor
    m_floor = m_entityManager.Create();
    m_componentRegistry.AddComponent<Tag>(m_floor, Tag("Floor"));
    m_componentRegistry.AddComponent<Transform>(m_floor, Transform(
        Vec3(0, -0.5f, 0), Quat::Identity(), Vec3(1, 1, 1)));
    AssetHandle floorTex = AssetManager::Get().LoadTexture("Assets/stone_floor.png");
        MeshRenderer floorMR(m_quadMesh, Color(1, 1, 1, 1));
        floorMR.diffuseTexture = floorTex;
        floorMR.shininess = 2.0f;
        m_componentRegistry.AddComponent<MeshRenderer>(m_floor, floorMR);

    // Step 6: Single-player maze test -- carve the maze, then spawn only Finn (the rogue).
    GenerateMaze();
    BuildMazeWalls();

    m_rogue = CreateCharacter("Finn", RaceType::HALFLING, ClassType::ROGUE,
        AbilityScores(8, 17, 12, 13, 11, 14),
        CellToWorld(kStartX, kStartY));

    // Combat is added in a later layer; just cache the (idle) CombatSystem for now.
    m_combatSystem = m_systemManager.GetSystem<CombatSystem>("CombatSystem");

    // Step 9: Set starting equipment
    auto equipItem = [&](Entity entity, const char* itemName) {
        Inventory* inv = m_componentRegistry.GetComponent<Inventory>(entity);
        EquipmentSlots* eq = m_componentRegistry.GetComponent<EquipmentSlots>(entity);
        const ItemData* item = ItemDatabase::GetItem(itemName);
        if (inv && eq && item) {
            inv->AddItem(item);
            eq->Equip(item);
        }
    };
    equipItem(m_rogue, "Shortsword");
    equipItem(m_rogue, "Leather Armor");

    {
        CharacterInfo* info = m_componentRegistry.GetComponent<CharacterInfo>(m_rogue);
        EquipmentSlots* eq = m_componentRegistry.GetComponent<EquipmentSlots>(m_rogue);
        if (info && eq) {
            const i32 dex = AbilityScores::GetModifier(info->abilities.dexterity);
            const ArmorData* body = eq->armor ? &eq->armor->armor : nullptr;
            const i32 total = ComputeArmorClass(dex, body, eq->shield != nullptr);
            info->armorClass = total;
            if (ArmorClass* ac = m_componentRegistry.GetComponent<ArmorClass>(m_rogue)) {
                ac->base = total;
                ac->ClearBonuses();
            }
        }
        if (info) DND_LOG_INFO("Finn: lvl %d %s %s -- HP %d, AC %d",
            info->level, RaceDatabase::GetRace(info->race).name,
            ClassDatabase::GetClass(info->classType).name, info->maxHP, info->armorClass);
    }

    // Goblins hold the corridors between Finn and the chest -- he has to cut
    // through them ("attack") to get past.
    PlaceMazeEnemies();

    // Traps: pressure plates, spike traps, teleporters, levers, crumbling floors
    PlaceTraps();

    // Turn the placeholder cubes into little articulated humanoids that walk,
    // idle, and swing when they attack.
    BuildRig(m_rogue, Color(0.93f, 0.79f, 0.62f, 1.0f), /*isPlayer*/ true);    // halfling rogue
    for (Entity g : m_enemies)
        BuildRig(g, Color(0.46f, 0.62f, 0.34f, 1.0f), /*isPlayer*/ false);     // goblin green

    // Loot hidden in the dust -- invisible until a Perception ("search") check
    // turns it up, then "pick up" to grab it.
    PlaceMazeItems();

    // Shadows: too many walls to cast them all, so UpdateShadows() feeds just the
    // moving characters + the walls nearest Finn to the caster each frame.
    UpdateKeyLight();

    // The chest is the goal at the maze's far corner -- hidden until Finn finds it.
    {
        Transform* ct = m_componentRegistry.GetComponent<Transform>(m_chest);
        if (ct) ct->position = CellToWorld(kExitX, kExitY);
        MeshRenderer* cmr = m_componentRegistry.GetComponent<MeshRenderer>(m_chest);
        if (cmr) cmr->visible = false;
    }

    // Fog of war: everything starts dark; reveal only the cells around Finn's start.
    for (i32 y = 0; y < kGrid; ++y)
        for (i32 x = 0; x < kGrid; ++x) m_explored[y][x] = false;
    RevealAround({ kStartX, kStartY });
    UpdateFogVisibility();

    // On-screen slider panel to tune the key light at runtime.
    SetupUI();

    DND_LOG_INFO("DnDDemoScene loaded: %u entities", m_entityManager.GetAliveCount());
}

void DndDemoScene::OnUnload() {
    AiBrain::Shutdown();
    Scene::OnUnload();
}

Vec3 DndDemoScene::CellToWorld(i32 cx, i32 cy) const {
    return Vec3((static_cast<f32>(cx) - kGrid / 2) * kCell, 0.5f,
                (static_cast<f32>(cy) - kGrid / 2) * kCell);
}

Cell DndDemoScene::WorldToCell(const Vec3& p) const {
    Cell c;
    c.x = static_cast<i32>(std::lround(p.x / kCell)) + kGrid / 2;
    c.y = static_cast<i32>(std::lround(p.z / kCell)) + kGrid / 2;
    return c;
}

b8 DndDemoScene::Walkable(i32 x, i32 y) const {
    if (x < 0 || x >= kGrid || y < 0 || y >= kGrid) return false;
    return m_walk[y][x];
}

void DndDemoScene::ClearFloorEntities() {
    // Stop all running scripts before destroying entities
    ScriptSystem* ss = m_systemManager.GetSystem<ScriptSystem>("ScriptSystem");
    if (ss) ss->StopAll(m_componentRegistry);

    // Destroy walls
    for (Entity w : m_walls) m_entityManager.Destroy(w);
    m_walls.clear();
    m_wallCells.clear();

    // Destroy enemies and their rigs (skip the player rig at index 0)
    for (Entity e : m_enemies) {
        m_entityManager.Destroy(e);
        // Find and destroy the corresponding rig (1:1, after the player rig)
        for (size_t i = 1; i < m_rigs.size(); ++i) {
            if (m_rigs[i].root.id == e.id) {
                CharacterRig& rig = m_rigs[i];
                m_entityManager.Destroy(rig.head);
                m_entityManager.Destroy(rig.torso);
                m_entityManager.Destroy(rig.armL);
                m_entityManager.Destroy(rig.armR);
                m_entityManager.Destroy(rig.legL);
                m_entityManager.Destroy(rig.legR);
                m_rigs.erase(m_rigs.begin() + static_cast<i64>(i));
                break;
            }
        }
    }
    m_enemies.clear();
    m_enemyCells.clear();
    m_enemyAlerted.clear();
    m_enemyContested.clear();
    m_enemyTypes.clear();

    // Destroy loot items
    for (Entity it : m_items) m_entityManager.Destroy(it);
    m_items.clear();
    m_itemCells.clear();
    m_itemDC.clear();
    m_itemFound.clear();
    m_itemTaken.clear();

    // Destroy traps
    ClearTrapEntities();

    // Reset combat state
    m_blockingEnemy = Entity::None();
    m_mazeAttack = false;
    m_attackTarget = Entity::None();
    m_scriptInited.clear();
}

void DndDemoScene::DescendFloor() {
    ClearFloorEntities();
    m_currentFloor++;
    m_descentPending = false;
    m_descentTimer = 0.0f;

    // Reset fog and chest state
    m_chestFound = false;
    m_chestOpen = false;
    for (i32 y = 0; y < kGrid; ++y)
        for (i32 x = 0; x < kGrid; ++x) m_explored[y][x] = false;

    // Generate a new maze with a different seed (DiceRoller is fresh each play)
    GenerateMaze();
    BuildMazeWalls();

    // Place enemies scaled to the current floor
    PlaceMazeEnemies();

    // Place traps for this floor
    PlaceTraps();

    // Place loot items
    PlaceMazeItems();

    // Build rigs for the new enemies
    for (Entity g : m_enemies) {
        Color skin(0.46f + 0.1f * m_currentFloor, 0.62f - 0.12f * m_currentFloor,
                   0.34f - 0.08f * m_currentFloor, 1.0f);
        BuildRig(g, skin, /*isPlayer*/ false);
    }

    // Move the chest: visible only on floor 0
    {
        Transform* ct = m_componentRegistry.GetComponent<Transform>(m_chest);
        if (ct) ct->position = CellToWorld(kExitX, kExitY);
        MeshRenderer* cmr = m_componentRegistry.GetComponent<MeshRenderer>(m_chest);
        if (cmr) {
            cmr->visible = (m_currentFloor == 0);
            cmr->color = Color(0.40f, 0.26f, 0.13f, 1.0f);
        }
    }

    // Reposition Finn at the start cell
    Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    if (rt) rt->position = CellToWorld(kStartX, kStartY);

    // Reveal around Finn's start and update fog
    RevealAround({ kStartX, kStartY });
    UpdateFogVisibility();

    // Recolor the floor
    {
        MeshRenderer* fmr = m_componentRegistry.GetComponent<MeshRenderer>(m_floor);
        if (fmr) {
            Color fc;
            switch (m_currentFloor) {
                case 1: fc = Color(0.20f, 0.28f, 0.22f, 1.0f); break;  // Cave: dark moss
                case 2: fc = Color(0.25f, 0.20f, 0.22f, 1.0f); break;  // Catacombs: dark marble
                default: fc = Color(0.30f, 0.30f, 0.35f, 1.0f); break; // Ruins: gray
            }
            fmr->color = fc;
        }
    }

    // Theme-appropriate descent message
    {
        char msg[160];
        if (m_currentFloor == 1) {
            std::snprintf(msg, sizeof(msg), "Finn descends into damp caves -- the air grows thick and cold.");
        } else {
            std::snprintf(msg, sizeof(msg), "The passage opens into ancient catacombs. A chill wind howls.");
        }
        PushLog(msg);
    }

    // Update shadows and lighting
    UpdateKeyLight();

    DND_LOG_INFO("Descended to floor %d", m_currentFloor);
}

void DndDemoScene::GenerateMaze() {
    for (i32 y = 0; y < kGrid; ++y)
        for (i32 x = 0; x < kGrid; ++x) m_walk[y][x] = false; // start: all walls

    // Recursive-backtracker over the logical cells (passages at odd grid coords).
    const i32 cells = kGrid / 2;
    std::vector<char> visited(static_cast<size_t>(cells * cells), 0);
    auto cellIdx = [&](i32 cx, i32 cy) { return cy * cells + cx; };
    std::vector<std::pair<i32, i32>> stack;

    visited[cellIdx(0, 0)] = 1;
    m_walk[1][1] = true;
    stack.push_back(std::make_pair(0, 0));

    const i32 dx[4] = { 1, -1, 0, 0 };
    const i32 dy[4] = { 0, 0, 1, -1 };
    while (!stack.empty()) {
        const i32 cx = stack.back().first;
        const i32 cy = stack.back().second;
        i32 order[4] = { 0, 1, 2, 3 };
        for (i32 i = 3; i > 0; --i) {            // Fisher-Yates shuffle (DiceRoller RNG)
            const i32 j = DiceRoller::Roll(1, i + 1) - 1;
            std::swap(order[i], order[j]);
        }
        bool moved = false;
        for (i32 k = 0; k < 4; ++k) {
            const i32 d = order[k];
            const i32 nx = cx + dx[d];
            const i32 ny = cy + dy[d];
            if (nx < 0 || nx >= cells || ny < 0 || ny >= cells) continue;
            if (visited[cellIdx(nx, ny)]) continue;
            visited[cellIdx(nx, ny)] = 1;
            m_walk[ny * 2 + 1][nx * 2 + 1] = true;                  // neighbor passage
            m_walk[cy * 2 + 1 + dy[d]][cx * 2 + 1 + dx[d]] = true;  // wall between, knocked down
            stack.push_back(std::make_pair(nx, ny));
            moved = true;
            break;
        }
        if (!moved) stack.pop_back();
    }
}

void DndDemoScene::BuildMazeWalls() {
    AssetHandle wallTexH;
    Color wallColor(1, 1, 1, 1);
    switch (m_currentFloor) {
        case 0: wallTexH = AssetManager::Get().LoadTexture("Assets/stone_wall.png"); break;
        case 1: wallTexH = AssetManager::Get().LoadTexture("Assets/stone_dark.png"); break;
        default: wallTexH = AssetManager::Get().LoadTexture("Assets/stone_dark.png"); break;
    }
    for (i32 y = 0; y < kGrid; ++y) {
        for (i32 x = 0; x < kGrid; ++x) {
            if (m_walk[y][x]) continue;
            const Vec3 c = CellToWorld(x, y);
            Entity w = m_entityManager.Create();
            m_componentRegistry.AddComponent<Tag>(w, Tag("Wall"));
            m_componentRegistry.AddComponent<Transform>(w, Transform(
                Vec3(c.x, 0.75f, c.z), Quat::Identity(), Vec3(kCell, 1.5f, kCell)));
            MeshRenderer mr(m_cubeMesh, wallColor);
            mr.diffuseTexture = wallTexH;
            mr.shininess = 8.0f;
            mr.visible = false;
            m_componentRegistry.AddComponent<MeshRenderer>(w, mr);
            m_walls.push_back(w);
            m_wallCells.push_back({ x, y });
        }
    }
}

void DndDemoScene::RevealAround(const Cell& c) {
    if (c.x < 0 || c.x >= kGrid || c.y < 0 || c.y >= kGrid) return;

    // Flood outward over walkable cells up to kRevealR steps (so corridors reveal
    // around corners, but never straight through walls).
    i32 dist[kGrid][kGrid];
    for (i32 y = 0; y < kGrid; ++y)
        for (i32 x = 0; x < kGrid; ++x) dist[y][x] = -1;

    std::vector<Cell> q;
    q.push_back(c);
    dist[c.y][c.x] = 0;
    const i32 dx[4] = { 1, -1, 0, 0 };
    const i32 dy[4] = { 0, 0, 1, -1 };
    size_t head = 0;
    while (head < q.size()) {
        const Cell cur = q[head++];
        m_explored[cur.y][cur.x] = true;                  // reveal this floor cell
        // Reveal the walls bordering it (8-neighborhood) so the corridor's walls show.
        for (i32 oy = -1; oy <= 1; ++oy)
            for (i32 ox = -1; ox <= 1; ++ox) {
                const i32 nx = cur.x + ox, ny = cur.y + oy;
                if (nx >= 0 && nx < kGrid && ny >= 0 && ny < kGrid && !m_walk[ny][nx])
                    m_explored[ny][nx] = true;
            }
        if (dist[cur.y][cur.x] >= kRevealR) continue;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = cur.x + dx[d], ny = cur.y + dy[d];
            if (nx < 0 || nx >= kGrid || ny < 0 || ny >= kGrid) continue;
            if (!m_walk[ny][nx] || dist[ny][nx] != -1) continue;  // can't flood through walls
            dist[ny][nx] = dist[cur.y][cur.x] + 1;
            q.push_back({ nx, ny });
        }
    }

    // Discovering the chest's cell counts as finding the chest.
    if (!m_chestFound && m_explored[kExitY][kExitX]) m_chestFound = true;
}

void DndDemoScene::UpdateFogVisibility() {
    for (size_t i = 0; i < m_walls.size(); ++i) {
        MeshRenderer* mr = m_componentRegistry.GetComponent<MeshRenderer>(m_walls[i]);
        if (mr) mr->visible = m_explored[m_wallCells[i].y][m_wallCells[i].x];
    }
    if (MeshRenderer* cmr = m_componentRegistry.GetComponent<MeshRenderer>(m_chest))
        cmr->visible = (m_currentFloor == 0) && m_chestFound;
    // Trap markers: visible when the cell is explored (or within kTrapRevealR from Finn)
    for (size_t i = 0; i < m_trapMarkers.size(); ++i) {
        MeshRenderer* tmr = m_componentRegistry.GetComponent<MeshRenderer>(m_trapMarkers[i]);
        if (tmr && i < m_traps.size()) {
            tmr->visible = m_explored[m_traps[i].cell.y][m_traps[i].cell.x];
        }
    }
    // Goblin (rig) visibility is handled per-frame in AnimateRigs (alive + explored).
}

b8 DndDemoScene::FindNearestUnexplored(const Cell& from, Cell& out) const {
    if (from.x < 0 || from.x >= kGrid || from.y < 0 || from.y >= kGrid) return false;
    char seen[kGrid][kGrid];
    for (i32 y = 0; y < kGrid; ++y)
        for (i32 x = 0; x < kGrid; ++x) seen[y][x] = 0;

    std::vector<Cell> q;
    q.push_back(from);
    seen[from.y][from.x] = 1;
    const i32 dx[4] = { 1, -1, 0, 0 };
    const i32 dy[4] = { 0, 0, 1, -1 };
    size_t head = 0;
    while (head < q.size()) {
        const Cell cur = q[head++];
        if (!m_explored[cur.y][cur.x]) { out = cur; return true; } // nearest unseen floor cell
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = cur.x + dx[d], ny = cur.y + dy[d];
            if (nx < 0 || nx >= kGrid || ny < 0 || ny >= kGrid) continue;
            if (!m_walk[ny][nx] || seen[ny][nx]) continue;
            seen[ny][nx] = 1;
            q.push_back({ nx, ny });
        }
    }
    return false; // everything reachable is explored
}

b8 DndDemoScene::StartExplore() {
    const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    if (!rt) return false;
    const Cell from = WorldToCell(rt->position);
    Cell target;
    if (!FindNearestUnexplored(from, target)) return false;
    m_path = FindPath(kGrid, kGrid, from, target,
                      [this](i32 x, i32 y) { return Walkable(x, y) != 0; });
    if (m_path.size() <= 1) return false;
    m_pathIndex = 0;
    m_walkT = 0.0f;
    m_walking = true;
    return true;
}

void DndDemoScene::PlaceMazeEnemies() {
    // Route from start to exit, then drop enemies at points along it so Finn is
    // guaranteed to run into them. Distinct literal names so the tags read apart.
    static const char* kGoblinNames[] = { "Snik", "Grukk", "Mogg", "Krag" };
    static const char* kSkeletonNames[] = { "Bones", "Rattle", "Grimal", "Mort" };
    static const char* kBugbearNames[] = { "Grumsh", "Hurk", "Brug", "Mash" };
    const std::vector<Cell> route = FindPath(kGrid, kGrid, { kStartX, kStartY },
        { kExitX, kExitY }, [this](i32 x, i32 y) { return Walkable(x, y) != 0; });
    if (route.size() < 4) return;

    // Choose enemy type based on floor
    const EnemyType etype = (m_currentFloor == 0) ? EnemyType::GOBLIN
                          : (m_currentFloor == 1) ? EnemyType::SKELETON
                          : EnemyType::BUGBEAR;
    const char** names = (etype == EnemyType::SKELETON) ? kSkeletonNames
                        : (etype == EnemyType::BUGBEAR) ? kBugbearNames
                        : kGoblinNames;
    const char* typeLabel = (etype == EnemyType::SKELETON) ? "skeleton"
                           : (etype == EnemyType::BUGBEAR) ? "bugbear"
                           : "goblin";

    // Each floor uses that creature's stat block (HP, AC, abilities), not a linear scale.
    const f32 fracs[] = { 0.30f, 0.55f, 0.80f };
    const i32 count = (std::min)(m_currentFloor + 2, 3); // 2, 3, 3 enemies

    for (i32 i = 0; i < count; ++i) {
        const size_t idx = static_cast<size_t>(fracs[i] * static_cast<f32>(route.size() - 1));
        if (idx == 0 || idx + 1 >= route.size()) continue;
        const Cell c = route[idx];
        const char* name = names[(std::min)(i, 3)];
        const Entity g = CreateEnemy(name, etype, 1, CellToWorld(c.x, c.y));
        m_enemies.push_back(g);
        m_enemyCells.push_back(c);
        m_enemyAlerted.push_back(0);
        m_enemyContested.push_back(0);
        m_enemyTypes.push_back(etype);
        DND_LOG_INFO("Floor %d %s '%s' placed at cell (%d,%d).", m_currentFloor, typeLabel, name, c.x, c.y);
    }
}

void DndDemoScene::ClearTrapEntities() {
    for (Entity e : m_trapMarkers) m_entityManager.Destroy(e);
    m_trapMarkers.clear();
    m_traps.clear();
}

i32 DndDemoScene::TrapAt(i32 x, i32 y) const {
    for (i32 i = 0; i < static_cast<i32>(m_traps.size()); ++i)
        if (m_traps[i].cell.x == x && m_traps[i].cell.y == y) return i;
    return -1;
}

void DndDemoScene::PlaceTraps() {
    ClearTrapEntities();

    const std::vector<Cell> route = FindPath(kGrid, kGrid, { kStartX, kStartY },
        { kExitX, kExitY }, [this](i32 x, i32 y) { return Walkable(x, y) != 0; });
    if (route.size() < 6) return;

    // Helper: is a cell adjacent (not diagonal) to the route and walkable?
    auto adjacentWalkable = [&](const Cell& c) -> Cell {
        const i32 dx[4] = { 1, -1, 0, 0 };
        const i32 dy[4] = { 0, 0, 1, -1 };
        for (i32 k = 0; k < 4; ++k) {
            Cell n = { c.x + dx[k], c.y + dy[k] };
            if (n.x >= 0 && n.x < kGrid && n.y >= 0 && n.y < kGrid && Walkable(n.x, n.y)) return n;
        }
        return { -1, -1 };
    };

    auto placeMarker = [&](const Cell& cell, const Color& color) -> Entity {
        Entity e = m_entityManager.Create();
        m_componentRegistry.AddComponent<Tag>(e, Tag("TrapMarker"));
        Vec3 pos = CellToWorld(cell.x, cell.y);
        m_componentRegistry.AddComponent<Transform>(e, Transform(
            Vec3(pos.x, 0.01f, pos.z), Quat::Identity(), Vec3(kCell * 0.9f, 0.02f, kCell * 0.9f)));
        MeshRenderer mr(m_cubeMesh, color);
        mr.visible = false; // hidden until Finn reveals the area
        m_componentRegistry.AddComponent<MeshRenderer>(e, mr);
        m_trapMarkers.push_back(e);
        return e;
    };

    // ---- Floor 0: Ruins ----
    if (m_currentFloor == 0) {
        // Teleporter: midway, diverts to a side passage
        if (route.size() > 8) {
            size_t tpIdx = route.size() / 3;
            Cell tpCell = route[tpIdx];
            Cell dest = adjacentWalkable(route[route.size() / 2]);
            if (dest.x >= 0) {
                TrapData tp;
                tp.cell = tpCell; tp.type = TrapType::TELEPORTER;
                tp.active = true;
                tp.cooldownMax = 3.0f;
                tp.triggerMsg = "Finn steps onto a shimmering rune -- a flash of blue light teleports him across the maze!";
                tp.destCell = dest;
                m_traps.push_back(tp);
                placeMarker(tpCell, Color(0.2f, 0.4f, 0.9f, 1.0f));
            }
        }
        // Pressure plate + spike trap pair: near the end of the route
        if (route.size() > 10) {
            size_t ppIdx = route.size() * 2 / 3;
            Cell ppCell = route[ppIdx];
            Cell spikeCell = adjacentWalkable(route[ppIdx + 1]);
            if (spikeCell.x >= 0) {
                TrapData pp;
                pp.cell = ppCell; pp.type = TrapType::PRESSURE_PLATE;
                pp.active = true; pp.triggered = false;
                pp.triggerMsg = "Finn hears a soft *click* underfoot -- somewhere ahead, a mechanism grinds to life!";
                pp.linkedTrap = static_cast<i32>(m_traps.size() + 1);
                m_traps.push_back(pp);
                placeMarker(ppCell, Color(0.9f, 0.6f, 0.1f, 1.0f));

                TrapData spike;
                spike.cell = spikeCell; spike.type = TrapType::SPIKE;
                spike.active = true; spike.triggered = false;
                spike.damage = 4.0f; spike.cooldownMax = 5.0f;
                spike.triggerMsg = "Spikes jab up from the floor! Finn takes 4 damage!";
                spike.linkedTrap = -1;
                m_traps.push_back(spike);
                placeMarker(spikeCell, Color(0.7f, 0.1f, 0.1f, 1.0f));
            }
        }
        // Lever near the chest that opens a shortcut
        {
            Cell leverCell = { kExitX - 1, kExitY };
            if (leverCell.x > 0 && Walkable(leverCell.x, leverCell.y)) {
                Cell wallCell = { kExitX - 2, kExitY };
                if (wallCell.x > 0 && !Walkable(wallCell.x, wallCell.y)) {
                    TrapData lever;
                    lever.cell = leverCell; lever.type = TrapType::LEVER;
                    lever.active = true; lever.triggered = false;
                    lever.triggerMsg = "Finn pulls the rusty lever -- a hidden door grinds open!";
                    lever.linkedTrap = -1;
                    lever.leverWallCell = wallCell; lever.leverState = false;
                    m_traps.push_back(lever);
                    placeMarker(leverCell, Color(0.6f, 0.6f, 0.7f, 1.0f));
                }
            }
        }
    }

    // ---- Floor 1: Cave ----
    if (m_currentFloor == 1) {
        // Crumbling floor: a section collapses after Finn stands on it
        if (route.size() > 6) {
            size_t cfIdx = route.size() / 2;
            Cell cfCell = route[cfIdx];
            TrapData cf;
            cf.cell = cfCell; cf.type = TrapType::CRUMBLING_FLOOR;
            cf.active = true;
            cf.damage = 6.0f;
            cf.triggerMsg = "The floor gives way! Finn tumbles into a pit, scraping his arms!";
            cf.standThreshold = 1.5f;
            m_traps.push_back(cf);
            placeMarker(cfCell, Color(0.55f, 0.40f, 0.25f, 1.0f));
        }
        // Pressure plate + spike pair
        if (route.size() > 8) {
            size_t ppIdx = route.size() / 3;
            Cell ppCell = route[ppIdx];
            Cell spikeCell = adjacentWalkable(route[ppIdx + 1]);
            if (spikeCell.x >= 0) {
                TrapData pp;
                pp.cell = ppCell; pp.type = TrapType::PRESSURE_PLATE;
                pp.active = true;
                pp.triggerMsg = "A pressure plate clicks under Finn's foot -- deadly spikes erupt nearby!";
                pp.linkedTrap = static_cast<i32>(m_traps.size() + 1);
                m_traps.push_back(pp);
                placeMarker(ppCell, Color(0.9f, 0.6f, 0.1f, 1.0f));

                TrapData spike;
                spike.cell = spikeCell; spike.type = TrapType::SPIKE;
                spike.active = true;
                spike.damage = 5.0f; spike.cooldownMax = 5.0f;
                spike.triggerMsg = "Rusty spikes burst from the cave floor! Finn takes 5 damage!";
                spike.linkedTrap = -1;
                m_traps.push_back(spike);
                placeMarker(spikeCell, Color(0.7f, 0.1f, 0.1f, 1.0f));
            }
        }
    }

    // ---- Floor 2: Catacombs ----
    if (m_currentFloor == 2) {
        // Teleporter maze: two teleporters that shuffle Finn around
        if (route.size() > 10) {
            size_t tp1Idx = route.size() / 4;
            size_t tp2Idx = route.size() * 3 / 4;
            Cell tp1 = route[tp1Idx];
            Cell tp2 = route[tp2Idx];
            Cell dest1 = adjacentWalkable(route[tp2Idx]);
            Cell dest2 = adjacentWalkable(route[tp1Idx]);
            if (dest1.x >= 0 && dest2.x >= 0) {
                TrapData t1;
                t1.cell = tp1; t1.type = TrapType::TELEPORTER;
                t1.active = true;
                t1.cooldownMax = 3.0f;
                t1.triggerMsg = "A bone-white rune flares -- Finn is yanked across the catacombs!";
                t1.destCell = dest1;
                m_traps.push_back(t1);
                placeMarker(tp1, Color(0.8f, 0.3f, 0.9f, 1.0f));

                TrapData t2;
                t2.cell = tp2; t2.type = TrapType::TELEPORTER;
                t2.active = true;
                t2.cooldownMax = 3.0f;
                t2.triggerMsg = "Another rune -- Finn whirls through the darkness!";
                t2.destCell = dest2;
                m_traps.push_back(t2);
                placeMarker(tp2, Color(0.8f, 0.3f, 0.9f, 1.0f));
            }
        }
        // Spike traps along the route
        if (route.size() > 6) {
            Cell spike1 = route[route.size() / 3];
            Cell spike2 = route[route.size() * 2 / 3];
            {
                TrapData s;
                s.cell = spike1; s.type = TrapType::SPIKE;
                s.active = true;
                s.damage = 6.0f; s.cooldownMax = 4.0f;
                s.triggerMsg = "Bone spikes lash up from the darkness! Finn takes 6 damage!";
                s.linkedTrap = -1;
                m_traps.push_back(s);
            }
            placeMarker(spike1, Color(0.7f, 0.1f, 0.1f, 1.0f));
            {
                TrapData s;
                s.cell = spike2; s.type = TrapType::SPIKE;
                s.active = true;
                s.damage = 6.0f; s.cooldownMax = 4.0f;
                s.triggerMsg = "More spikes -- the catacombs are treacherous! Finn takes 6 damage!";
                s.linkedTrap = -1;
                m_traps.push_back(s);
            }
            placeMarker(spike2, Color(0.7f, 0.1f, 0.1f, 1.0f));
        }
    }

    DND_LOG_INFO("Floor %d: placed %zu traps", m_currentFloor, m_traps.size());
}

void DndDemoScene::TriggerTrap(i32 idx) {
    if (idx < 0 || idx >= static_cast<i32>(m_traps.size())) return;
    TrapData& trap = m_traps[idx];
    if (!trap.active || trap.triggered) return;

    trap.triggered = true;

    switch (trap.type) {
        case TrapType::SPIKE: {
            HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(m_rogue);
            if (hp) {
                i32 dmg = static_cast<i32>(trap.damage);
                if (dmg < 1) dmg = 1;
                hp->current -= dmg;
                if (hp->current < 0) hp->current = 0;
            }
            if (!trap.triggerMsg.empty()) PushLog(trap.triggerMsg);
            trap.cooldownTimer = trap.cooldownMax;
            break;
        }
        case TrapType::TELEPORTER: {
            Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
            if (rt && trap.destCell.x >= 0 && trap.destCell.y >= 0) {
                Vec3 dest = CellToWorld(trap.destCell.x, trap.destCell.y);
                rt->position = dest;
                // Stop any current walking and reset pathfinding
                m_walking = false;
                m_path.clear();
                // Re-grid position in fog
                Cell fc = { trap.destCell.x, trap.destCell.y };
                RevealAround(fc);
                UpdateFogVisibility();
                // Alert enemies near the teleport destination
                if (m_enemyAlerted.size() == m_enemies.size()) {
                    for (size_t i = 0; i < m_enemies.size(); ++i) {
                        if (i < m_enemyAlerted.size() &&
                            std::abs(m_enemyCells[i].x - fc.x) <= kSight &&
                            std::abs(m_enemyCells[i].y - fc.y) <= kSight) {
                            m_enemyAlerted[i] = 1;
                        }
                    }
                }
                trap.cooldownTimer = trap.cooldownMax;
            }
            if (!trap.triggerMsg.empty()) PushLog(trap.triggerMsg);
            break;
        }
        case TrapType::PRESSURE_PLATE: {
            if (!trap.triggerMsg.empty()) PushLog(trap.triggerMsg);
            if (trap.linkedTrap >= 0) TriggerTrap(trap.linkedTrap);
            // Pressure plates are one-shot
            trap.active = false;
            break;
        }
        case TrapType::LEVER: {
            if (!trap.triggerMsg.empty()) PushLog(trap.triggerMsg);
            trap.leverState = !trap.leverState;
            // Toggle the wall: create or destroy wall entity at leverWallCell
            if (trap.leverWallCell.x >= 0 && trap.leverWallCell.y >= 0) {
                bool hasWall = false;
                for (size_t i = 0; i < m_wallCells.size(); ++i) {
                    if (m_wallCells[i].x == trap.leverWallCell.x &&
                        m_wallCells[i].y == trap.leverWallCell.y) {
                        if (trap.leverState) {
                            // Lever pulled DOWN: remove the wall
                            m_entityManager.Destroy(m_walls[i]);
                            m_walls.erase(m_walls.begin() + static_cast<i64>(i));
                            m_wallCells.erase(m_wallCells.begin() + static_cast<i64>(i));
                            m_walk[trap.leverWallCell.y][trap.leverWallCell.x] = true;
                            PushLog("A stone door slides open, revealing a new passage!");
                        }
                        hasWall = true;
                        break;
                    }
                }
                if (!hasWall && !trap.leverState) {
                    // Wall doesn't exist and lever is being returned to original pos
                    // Re-create the wall if it was removed
                    // (Simple approach: just make the cell unwalkable again)
                    // For now, levers are one-shot.
                }
            }
            trap.active = false; // one-shot lever
            break;
        }
        case TrapType::CRUMBLING_FLOOR: {
            HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(m_rogue);
            if (hp) {
                i32 dmg = static_cast<i32>(trap.damage);
                if (dmg < 1) dmg = 1;
                hp->current -= dmg;
                if (hp->current < 0) hp->current = 0;
            }
            // Teleport Finn back to start
            Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
            if (rt) {
                rt->position = CellToWorld(kStartX, kStartY);
                m_walking = false;
                m_path.clear();
                Cell fc = { kStartX, kStartY };
                RevealAround(fc);
                UpdateFogVisibility();
            }
            if (!trap.triggerMsg.empty()) PushLog(trap.triggerMsg);
            trap.active = false; // one-shot
            break;
        }
    }
}

void DndDemoScene::CheckTraps(f32 dt) {
    Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    if (!rt) return;
    Cell fc = WorldToCell(rt->position);

    // Update cooldowns and crumbling-floor timers
    for (i32 i = 0; i < static_cast<i32>(m_traps.size()); ++i) {
        TrapData& trap = m_traps[i];
        if (!trap.active) continue;

        // Cooldown recovery
        if (trap.cooldownTimer > 0.0f) {
            trap.cooldownTimer -= dt;
            if (trap.cooldownTimer <= 0.0f) {
                trap.cooldownTimer = 0.0f;
                trap.triggered = false; // re-arm
            }
        }

        // Crumbling floor: track how long Finn stands on it
        if (trap.type == TrapType::CRUMBLING_FLOOR &&
            trap.cell.x == fc.x && trap.cell.y == fc.y) {
            trap.standTimer += dt;
            if (trap.standTimer >= trap.standThreshold && !trap.triggered) {
                TriggerTrap(i);
            }
        } else if (trap.type == TrapType::CRUMBLING_FLOOR) {
            trap.standTimer = 0.0f; // reset if Finn walks off
        }
    }

    // Check if Finn is standing on a trap cell
    i32 trapIdx = TrapAt(fc.x, fc.y);
    if (trapIdx < 0) return;
    TrapData& trap = m_traps[trapIdx];
    if (!trap.active || trap.triggered) return;

    switch (trap.type) {
        case TrapType::SPIKE:
            if (trap.cooldownTimer <= 0.0f) TriggerTrap(trapIdx);
            break;
        case TrapType::TELEPORTER:
            if (trap.cooldownTimer <= 0.0f) {
                TriggerTrap(trapIdx);
                // After teleport, re-check the new cell for traps
                Cell newFc = WorldToCell(rt->position);
                i32 newTrap = TrapAt(newFc.x, newFc.y);
                if (newTrap >= 0) TriggerTrap(newTrap);
            }
            break;
        default:
            break; // PRESSURE_PLATE, LEVER, CRUMBLING handled elsewhere
    }
}

Entity DndDemoScene::LivingEnemyAt(const Cell& c) const {
    for (size_t i = 0; i < m_enemies.size(); ++i) {
        if (m_enemyCells[i].x != c.x || m_enemyCells[i].y != c.y) continue;
        const HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(m_enemies[i]);
        if (hp && hp->IsAlive()) return m_enemies[i];
    }
    return Entity::None();
}

i32 DndDemoScene::EnemyIndex(Entity e) const {
    for (size_t i = 0; i < m_enemies.size(); ++i)
        if (m_enemies[i] == e) return static_cast<i32>(i);
    return -1;
}

b8 DndDemoScene::HasLineOfSight(const Cell& a, const Cell& b) const {
    // Bresenham march; sight is blocked if any cell between a and b is a wall.
    i32 x0 = a.x, y0 = a.y;
    const i32 x1 = b.x, y1 = b.y;
    const i32 dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    const i32 sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
    i32 err = dx - dy;
    while (true) {
        const b8 endpoint = (x0 == a.x && y0 == a.y) || (x0 == x1 && y0 == y1);
        if (!endpoint && !Walkable(x0, y0)) return false;   // a wall blocks the line
        if (x0 == x1 && y0 == y1) break;
        const i32 e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
    return true;
}

void DndDemoScene::CheckEnemyDetection() {
    const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    if (!rt) return;
    const Cell fc = WorldToCell(rt->position);
    const CharacterInfo* fi = m_componentRegistry.GetComponent<CharacterInfo>(m_rogue);
    const i32 stealthMod = fi
        ? fi->skills.GetBonus(Skill::STEALTH, fi->abilities, fi->proficiencyBonus)
        : CheckBonus(3, 2, ProficiencyRank::EXPERTISE);

    for (size_t i = 0; i < m_enemies.size(); ++i) {
        if (m_enemyContested[i] || m_enemyAlerted[i]) continue;   // contest is rolled once
        const HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(m_enemies[i]);
        if (!hp || !hp->IsAlive()) continue;
        const Cell ec = m_enemyCells[i];
        if (std::abs(ec.x - fc.x) + std::abs(ec.y - fc.y) > kSight) continue;
        if (!HasLineOfSight(ec, fc)) continue;

        m_enemyContested[i] = 1;
        // Reveal the goblin that now shares a sightline with Finn.
        m_explored[ec.y][ec.x] = true;

        const CharacterInfo* gi = m_componentRegistry.GetComponent<CharacterInfo>(m_enemies[i]);
        const i32 perceptMod = gi
            ? gi->skills.GetBonus(Skill::PERCEPTION, gi->abilities, gi->proficiencyBonus)
            : -1;
        // Tie keeps Finn unnoticed: the situation before the contest was "not spotted".
        const ContestResult contest = RollContest(stealthMod, perceptMod);
        const i32 stealth = contest.actor.total;
        const i32 percept = contest.opposed.total;
        const Tag* tag = m_componentRegistry.GetComponent<Tag>(m_enemies[i]);
        const char* nm = (tag && tag->name) ? tag->name : "The goblin";
        char b[176];
        if (OpposedCheckBeats(contest)) {
            m_enemyAlerted[i] = 1;
            std::snprintf(b, sizeof(b), "%s catches Finn's movement! (Stealth %d vs Perception %d) -- on guard.",
                          nm, stealth, percept);
        } else {
            std::snprintf(b, sizeof(b), "Finn melts into the shadows -- %s hasn't noticed him. (Stealth %d vs Perception %d)",
                          nm, stealth, percept);
        }
        PushLog(b);
    }
    UpdateFogVisibility();
}

b8 DndDemoScene::RepathToExit() {
    const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    if (!rt) return false;
    const Cell start = WorldToCell(rt->position);
    m_path = FindPath(kGrid, kGrid, start, { kExitX, kExitY },
                      [this](i32 x, i32 y) { return Walkable(x, y) != 0; });
    if (m_path.size() <= 1) return false;
    m_pathIndex = 0;
    m_walkT = 0.0f;
    m_walking = true;
    return true;
}

void DndDemoScene::ResolveMazeAttack() {
    const Entity foe = m_attackTarget;
    HitPoints*       hp  = m_componentRegistry.GetComponent<HitPoints>(foe);
    const ArmorClass* ac = m_componentRegistry.GetComponent<ArmorClass>(foe);
    const Tag*       ftag = m_componentRegistry.GetComponent<Tag>(foe);
    const char* fname = (ftag && ftag->name) ? ftag->name : "the goblin";
    if (!hp || !ac) return;

    // Determine enemy type
    const i32 ti = EnemyIndex(foe);
    const EnemyType etype = (ti >= 0 && static_cast<size_t>(ti) < m_enemyTypes.size())
                            ? m_enemyTypes[ti] : EnemyType::GOBLIN;
    const MonsterKind kind = (etype == EnemyType::SKELETON) ? MonsterKind::SKELETON
                           : (etype == EnemyType::BUGBEAR) ? MonsterKind::BUGBEAR
                           : MonsterKind::GOBLIN;
    const MonsterProfile profile = GetMonsterProfile(kind);
    const char* enemyLabel = profile.name;

    // Finn's strike uses the equipped weapon. Finesse (the shortsword) takes the
    // higher of Strength and Dexterity. An unseen foe grants advantage; sneak
    // attack dice are added only when the weapon is finesse or ranged.
    const CharacterInfo* info = m_componentRegistry.GetComponent<CharacterInfo>(m_rogue);
    const i32 dexMod = info ? AbilityScores::GetModifier(info->abilities.dexterity) : 3;
    const i32 strMod = info ? AbilityScores::GetModifier(info->abilities.strength) : -1;
    const i32 prof   = info ? info->proficiencyBonus : 2;
    const i32 targetAC = ac->GetTotal();
    const EquipmentSlots* eq = m_componentRegistry.GetComponent<EquipmentSlots>(m_rogue);
    const ItemData* weapon = (eq && eq->weapon && eq->weapon->type == ItemType::WEAPON)
        ? eq->weapon : nullptr;
    i32 weaponDice = 1;
    i32 weaponDie = 6;
    DamageType damageType = DamageType::PIERCING;
    b8 finesse = true;
    b8 ranged = false;
    if (weapon && weapon->weapon.damage.dice.numDice > 0 && weapon->weapon.damage.dice.numSides > 0) {
        weaponDice = weapon->weapon.damage.dice.numDice;
        weaponDie = weapon->weapon.damage.dice.numSides;
        damageType = weapon->weapon.damage.type;
        finesse = weapon->weapon.properties.isFinesse;
        ranged = weapon->weapon.properties.isRanged;
    }
    const i32 abilityMod = WeaponAbilityModifier(strMod, dexMod, finesse, ranged);
    const b8 unseen = (ti >= 0 && !m_enemyAlerted[ti]);
    const b8 sneak = unseen && (finesse || ranged);
    if (ti >= 0) m_enemyAlerted[ti] = 1;

    StrikeSpec spec;
    spec.attackBonus = abilityMod + prof;
    spec.mode = unseen ? RollMode::ADVANTAGE : RollMode::NORMAL;
    spec.targetArmorClass = targetAC;
    spec.weaponDiceCount = weaponDice;
    spec.weaponDieSize = weaponDie;
    spec.extraDiceCount = sneak ? SneakAttackDice(info ? info->level : 1) : 0;
    spec.extraDieSize = 6;
    spec.flatModifier = abilityMod;
    spec.affinity = MonsterDamageAffinity(profile, damageType);
    const StrikeResult strike = RollStrike(spec);

    const i32 roll = strike.check.natural;
    const i32 atkMod = strike.check.modifier;
    char b[256];
    if (!strike.hit) {
        std::snprintf(b, sizeof(b), "%sFinn swings: d20 %d %+d = %d vs AC %d -- miss.",
                      unseen ? "From hiding -- " : "", roll, atkMod, strike.check.total, targetAC);
        PushLog(b);
        return;  // a whiffed lunge: Finn pulls back, the exchange is a wash
    }
    const i32 dmg = strike.damage;
    hp->TakeDamage(dmg);
    const char* hitLabel = "Finn strikes";
    if (sneak && strike.critical) hitLabel = "SNEAK ATTACK CRIT";
    else if (sneak) hitLabel = "SNEAK ATTACK";
    else if (strike.critical) hitLabel = "Finn CRITS";
    std::snprintf(b, sizeof(b), "%s: d20 %d %+d = %d vs AC %d -- HIT for %d! (%s %d/%d)",
                  hitLabel, roll, atkMod, strike.check.total, targetAC, dmg, fname, hp->current, hp->maximum);
    if (spec.affinity == DamageAffinity::VULNERABLE) {
        PushLog("The blow finds a weak point -- vulnerability doubles the damage.");
    } else if (spec.affinity == DamageAffinity::RESISTANT) {
        PushLog("The creature resists, and the damage is halved.");
    } else if (spec.affinity == DamageAffinity::IMMUNE) {
        PushLog("The creature is immune. The blow does nothing.");
    }
    PushLog(b);

    ScriptSystem::NotifyEvent(m_componentRegistry, foe,
        "{\"type\":\"damage\",\"amount\":" + std::to_string(dmg) +
        ",\"hp\":" + std::to_string(hp->current) + "}");

    if (!hp->IsAlive()) {
        ScriptSystem::NotifyEvent(m_componentRegistry, foe, "{\"type\":\"death\"}");
        if (MeshRenderer* mr = m_componentRegistry.GetComponent<MeshRenderer>(foe))
            mr->visible = false;                   // the body drops out of sight

        // Death flavor varies by type
        switch (etype) {
            case EnemyType::SKELETON:
                std::snprintf(b, sizeof(b), "%s collapses into a pile of lifeless bones.", fname);
                break;
            case EnemyType::BUGBEAR:
                std::snprintf(b, sizeof(b), "%s roars one last time and falls with a thunderous crash.", fname);
                break;
            default:
                std::snprintf(b, sizeof(b), "%s crumples to the maze floor.", fname);
                break;
        }
        PushLog(b);

        // Award the experience printed on the stat block.
        {
            const i32 xpReward = profile.experience;
            Experience* exp = m_componentRegistry.GetComponent<Experience>(m_rogue);
            if (exp) {
                i32 oldLevel = exp->level;
                exp->AddXP(xpReward);
                std::snprintf(b, sizeof(b), "Finn gains %d XP (defeated a %s).", xpReward, enemyLabel);
                PushLog(b);
                if (exp->level > oldLevel) {
                    CharacterInfo* ci = m_componentRegistry.GetComponent<CharacterInfo>(m_rogue);
                    HitPoints* fhp = m_componentRegistry.GetComponent<HitPoints>(m_rogue);
                    if (ci && fhp) {
                        i32 conMod = AbilityScores::GetModifier(ci->abilities.constitution);
                        i32 oldMax = fhp->maximum;
                        ci->level = exp->level;
                        ci->maxHP = Progression::CalculateMaxHP(ci->classType, exp->level, conMod, true);
                        ci->proficiencyBonus = Progression::GetProficiencyBonus(exp->level);
                        fhp->maximum = ci->maxHP;
                        fhp->current += (fhp->maximum - oldMax);
                        std::snprintf(b, sizeof(b),
                            "Finn reaches level %d! HP %d -> %d (+%d), proficiency %+d!",
                            exp->level, oldMax, fhp->maximum, fhp->maximum - oldMax,
                            ci->proficiencyBonus);
                        PushLog(b);
                    }
                }
            }
        }

        return;                                    // dead -- no reprisal
    }

    // Still standing: the enemy attacks back with the weapon on its stat block.
    // Death saving throws are not in this layer, so Finn is left at 1 HP.
    HitPoints*        fhp = m_componentRegistry.GetComponent<HitPoints>(m_rogue);
    const ArmorClass* fac = m_componentRegistry.GetComponent<ArmorClass>(m_rogue);
    const i32 finnAC = fac ? fac->GetTotal() : 13;
    StrikeSpec reprisal;
    reprisal.attackBonus = MonsterAttackBonus(profile);
    reprisal.mode = RollMode::NORMAL;
    reprisal.targetArmorClass = finnAC;
    reprisal.weaponDiceCount = profile.melee.numDice;
    reprisal.weaponDieSize = profile.melee.dieSize;
    reprisal.flatModifier = MonsterDamageModifier(profile);
    reprisal.affinity = DamageAffinity::NORMAL;
    const StrikeResult enemyStrike = RollStrike(reprisal);
    const char* reprisalWeapon = profile.melee.name;
    if (fhp && enemyStrike.hit) {
        fhp->TakeDamage(enemyStrike.damage);
        if (fhp->current <= 0) fhp->current = 1;
        std::snprintf(b, sizeof(b), "%s lashes back with %s%s for %d! (Finn %d/%d)",
                      fname, reprisalWeapon, enemyStrike.critical ? " -- CRIT" : "",
                      enemyStrike.damage, fhp->current, fhp->maximum);
        PushLog(b);
    } else {
        std::snprintf(b, sizeof(b), "%s lashes back with %s -- Finn turns it aside.",
                      fname, reprisalWeapon);
        PushLog(b);
    }
}

void DndDemoScene::PlaceMazeItems() {
    m_itemMesh = BuildCapsuleMesh(0.18f, 0.0f, 10, 14);   // small glint sphere
    const std::vector<Cell> route = FindPath(kGrid, kGrid, { kStartX, kStartY },
        { kExitX, kExitY }, [this](i32 x, i32 y) { return Walkable(x, y) != 0; });
    if (route.size() < 6) return;

    struct Loot { const char* name; f32 frac; i32 dc; Color col; };
    static const Loot kFloorLoot[3][3] = {
        // Floor 0 (Ruins): standard starting loot
        {
            { "a silver ring",    0.05f, 10, Color(0.85f, 0.86f, 0.92f, 1.0f) },
            { "a healing potion", 0.40f, 12, Color(0.88f, 0.22f, 0.28f, 1.0f) },
            { "a jeweled dagger", 0.94f, 13, Color(0.95f, 0.80f, 0.30f, 1.0f) },
        },
        // Floor 1 (Cave): ancient remains
        {
            { "an ancient coin",  0.05f, 11, Color(0.92f, 0.84f, 0.40f, 1.0f) },
            { "a healing potion", 0.40f, 12, Color(0.88f, 0.22f, 0.28f, 1.0f) },
            { "a silver shortsword", 0.94f, 14, Color(0.82f, 0.85f, 0.90f, 1.0f) },
        },
        // Floor 2 (Catacombs): darker, richer trove
        {
            { "a dark gem",       0.05f, 12, Color(0.40f, 0.10f, 0.50f, 1.0f) },
            { "a healing potion", 0.40f, 12, Color(0.88f, 0.22f, 0.28f, 1.0f) },
            { "an enchanted dagger", 0.94f, 15, Color(0.20f, 0.60f, 0.90f, 1.0f) },
        },
    };
    const i32 floorIdx = (std::min)(m_currentFloor, 2);
    const Loot* loot = kFloorLoot[floorIdx];

    for (i32 i = 0; i < 3; ++i) {
        const Loot& L = loot[i];
        size_t idx = static_cast<size_t>(L.frac * static_cast<f32>(route.size() - 1));
        if (idx >= route.size()) idx = route.size() - 1;
        const Cell c = route[idx];
        const Vec3 w = CellToWorld(c.x, c.y);
        const Entity it = m_entityManager.Create();
        m_componentRegistry.AddComponent<Tag>(it, Tag(L.name));
        m_componentRegistry.AddComponent<Transform>(it, Transform(
            Vec3(w.x, 0.25f, w.z), Quat::Identity(), Vec3(1.0f, 1.0f, 1.0f)));
        MeshRenderer mr(m_itemMesh, L.col);
        mr.visible = false;                              // unseen until a Perception check finds it
        m_componentRegistry.AddComponent<MeshRenderer>(it, mr);
        m_items.push_back(it);
        m_itemCells.push_back(c);
        m_itemDC.push_back(L.dc);
        m_itemFound.push_back(false);
        m_itemTaken.push_back(false);
        DND_LOG_INFO("Hidden loot '%s' at cell (%d,%d), Perception DC %d.", L.name, c.x, c.y, L.dc);
    }
}

void DndDemoScene::DoPerceptionSearch() {
    const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    if (!rt) return;
    const Cell here = WorldToCell(rt->position);

    // Perception: Wisdom modifier, plus proficiency when Finn has it.
    const CharacterInfo* info = m_componentRegistry.GetComponent<CharacterInfo>(m_rogue);
    const i32 mod = info
        ? info->skills.GetBonus(Skill::PERCEPTION, info->abilities, info->proficiencyBonus)
        : CheckBonus(0, 2, ProficiencyRank::PROFICIENT);
    const D20Check search = RollD20Check(RollMode::NORMAL, mod);
    const i32 roll  = search.natural;
    const i32 total = search.total;
    char b[160];
    std::snprintf(b, sizeof(b), "Finn searches: d20 %d %+d = %d (WIS / Perception).",
                  roll, mod, total);
    PushLog(b);

    i32 revealed = 0, teased = 0;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_itemTaken[i] || m_itemFound[i]) continue;
        const i32 dist = std::abs(m_itemCells[i].x - here.x) + std::abs(m_itemCells[i].y - here.y);
        if (dist > 2) continue;                          // not close enough to notice
        if (total >= m_itemDC[i]) {
            m_itemFound[i] = true;
            if (MeshRenderer* mr = m_componentRegistry.GetComponent<MeshRenderer>(m_items[i]))
                mr->visible = true;
            const Tag* tag = m_componentRegistry.GetComponent<Tag>(m_items[i]);
            std::snprintf(b, sizeof(b), "Finn's eye catches %s half-buried in the dust! (DC %d)",
                          (tag && tag->name) ? tag->name : "something", m_itemDC[i]);
            PushLog(b);
            ++revealed;
        } else {
            ++teased;
        }
    }
    if (revealed == 0) {
        PushLog(teased > 0 ? "Something glints nearby, but Finn can't quite place it -- search again."
                           : "Nothing of interest catches Finn's eye here.");
    } else {
        PushLog("Finn can \"pick up\" what he's spotted.");
    }
}

void DndDemoScene::DoPickup() {
    const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    if (!rt) return;
    const Cell here = WorldToCell(rt->position);
    Inventory* inv = m_componentRegistry.GetComponent<Inventory>(m_rogue);

    i32 grabbed = 0, outOfReach = 0;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_itemTaken[i] || !m_itemFound[i]) continue;
        const i32 dist = std::abs(m_itemCells[i].x - here.x) + std::abs(m_itemCells[i].y - here.y);
        if (dist > 2) { ++outOfReach; continue; }
        m_itemTaken[i] = true;
        if (MeshRenderer* mr = m_componentRegistry.GetComponent<MeshRenderer>(m_items[i]))
            mr->visible = false;
        const Tag* tag = m_componentRegistry.GetComponent<Tag>(m_items[i]);
        const char* itemName = (tag && tag->name) ? tag->name : "";
        // Map loot tag names to actual ItemDatabase items
        const ItemData* actualItem = nullptr;
        if (std::strstr(itemName, "healing") || std::strstr(itemName, "potion")) {
            actualItem = ItemDatabase::GetItem("Potion of Healing");
        } else if (std::strstr(itemName, "dagger")) {
            actualItem = ItemDatabase::GetItem("Dagger");
        } else if (std::strstr(itemName, "shortsword")) {
            actualItem = ItemDatabase::GetItem("Shortsword");
        } else if (std::strstr(itemName, "ring") || std::strstr(itemName, "coin") || std::strstr(itemName, "gem")) {
            actualItem = ItemDatabase::GetItem("Club"); // reskinned trade-good worth ~50-200gp
        }
        if (actualItem && inv) {
            inv->AddItem(actualItem, 1);
        }
        char b[128];
        std::snprintf(b, sizeof(b), "Finn pockets %s.", itemName);
        PushLog(b);
        ++grabbed;
    }
    if (grabbed == 0) {
        PushLog(outOfReach > 0 ? "Finn spotted something, but it's out of reach -- move closer."
                               : "There's nothing here for Finn to pick up (try \"search\" first).");
    }
}

void DndDemoScene::BuildRig(Entity root, const Color& skin, b8 isPlayer) {
    // The placeholder cube is replaced by the rig.
    if (MeshRenderer* rc = m_componentRegistry.GetComponent<MeshRenderer>(root)) rc->visible = false;
    const Transform* rt = m_componentRegistry.GetComponent<Transform>(root);
    const Vec3 P = rt ? rt->position : Vec3(0.0f, 0.5f, 0.0f);

    auto mk = [&](Mesh* mesh, const Color& col, const char* tag) -> Entity {
        Entity e = m_entityManager.Create();
        m_componentRegistry.AddComponent<Tag>(e, Tag(tag));
        m_componentRegistry.AddComponent<Transform>(e, Transform(P, Quat::Identity(), Vec3(1, 1, 1)));
        MeshRenderer mr(mesh, col);
        mr.visible = isPlayer;          // goblins stay hidden until the fog reveals them
        m_componentRegistry.AddComponent<MeshRenderer>(e, mr);
        return e;
    };
    const Color tunic(skin.r * 0.62f, skin.g * 0.60f, skin.b * 0.78f, 1.0f); // darker clothing

    CharacterRig rig;
    rig.root = root; rig.isPlayer = isPlayer;
    rig.yaw = 0.0f; rig.phase = 0.0f; rig.lastPos = P;
    rig.head  = mk(m_headMesh, skin,  "rig.head");
    rig.torso = mk(m_cubeMesh, tunic, "rig.torso");
    rig.armL  = mk(m_limbMesh, skin,  "rig.armL");
    rig.armR  = mk(m_limbMesh, skin,  "rig.armR");
    rig.legL  = mk(m_limbMesh, tunic, "rig.legL");
    rig.legR  = mk(m_limbMesh, tunic, "rig.legR");
    m_rigs.push_back(rig);
}

void DndDemoScene::SetRigVisible(const CharacterRig& rig, b8 visible) {
    const Entity parts[6] = { rig.head, rig.torso, rig.armL, rig.armR, rig.legL, rig.legR };
    for (Entity e : parts) {
        if (MeshRenderer* mr = m_componentRegistry.GetComponent<MeshRenderer>(e)) mr->visible = visible;
    }
}

void DndDemoScene::AnimateRigs(f32 dt) {
    m_animTime += dt;
    const f32 PI = 3.14159265358979f;

    for (CharacterRig& rig : m_rigs) {
        const Transform* root = m_componentRegistry.GetComponent<Transform>(rig.root);
        if (!root) continue;
        const Vec3 P = root->position;

        // Visibility: the player is always shown; a goblin only once it is alive AND
        // standing in a cell the fog has revealed.
        b8 show = true;
        if (!rig.isPlayer) {
            const HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(rig.root);
            b8 explored = false;
            for (size_t i = 0; i < m_enemies.size(); ++i)
                if (m_enemies[i] == rig.root) { explored = m_explored[m_enemyCells[i].y][m_enemyCells[i].x]; break; }
            show = hp && hp->IsAlive() && explored;
        }
        SetRigVisible(rig, show);
        if (!show) { rig.lastPos = P; continue; }

        // Movement + facing.
        const f32 dx = P.x - rig.lastPos.x, dz = P.z - rig.lastPos.z;
        const f32 moved = std::sqrt(dx * dx + dz * dz);
        const f32 speed = (dt > 0.0f) ? moved / dt : 0.0f;
        const b8 walking = speed > 0.25f;
        const b8 attacking = (rig.root == m_lungeAttacker) &&
                             (m_lungePhase == LungePhase::OUT || m_lungePhase == LungePhase::BACK);

        f32 targetYaw = rig.yaw;
        if (attacking) {
            const f32 ax = m_lungeBump.x - P.x, az = m_lungeBump.z - P.z;
            if (ax * ax + az * az > 1e-4f) targetYaw = std::atan2(ax, az);
        } else if (walking) {
            targetYaw = std::atan2(dx, dz);
        }
        f32 dyaw = targetYaw - rig.yaw;
        while (dyaw >  PI) dyaw -= 2.0f * PI;
        while (dyaw < -PI) dyaw += 2.0f * PI;
        rig.yaw += dyaw * std::min(1.0f, dt * 12.0f);
        const f32 th = rig.yaw, ct = std::cos(th), st = std::sin(th);

        // Gait.
        if (walking) rig.phase += moved * 4.0f;
        const f32 swing = walking ? 0.6f * std::sin(rig.phase) : 0.0f;
        const f32 bob   = walking ? std::fabs(std::sin(rig.phase)) * 0.05f
                                  : std::sin(m_animTime * 2.0f + rig.phase) * 0.025f;

        const f32 gx = P.x, gz = P.z;
        const f32 baseY = P.y - 0.5f; // P.y ~ 0.5 -> feet rest on the floor

        auto setPart = [&](Entity e, const Vec3& wpos, const Quat& wrot, const Vec3& scl) {
            if (Transform* t = m_componentRegistry.GetComponent<Transform>(e)) {
                t->position = wpos; t->rotation = wrot; t->scale = scl;
            }
        };
        // Torso (slight twist) and head ride above the pelvis, yaw-aligned.
        setPart(rig.torso, Vec3(gx, baseY + 0.70f + bob, gz),
                Quat::FromEulerAngles(walking ? 0.05f * std::sin(rig.phase) : 0.0f, th, 0.0f),
                Vec3(0.42f, 0.66f, 0.28f));
        setPart(rig.head, Vec3(gx, baseY + 1.20f + bob, gz),
                Quat::FromEulerAngles(0.0f, th, 0.0f), Vec3(1.0f, 1.0f, 1.0f));

        // A limb hangs length L from a joint and swings about X; we pivot at the joint.
        auto placeLimb = [&](Entity e, f32 offX, f32 jointY, f32 s, f32 L) {
            const f32 cz = -(L * 0.5f) * std::sin(s);          // char-space Z of the limb's centre
            const f32 ly = jointY - (L * 0.5f) * std::cos(s);  // its height
            const f32 wx = offX * ct + cz * st;                // yaw-rotate (offX, cz)
            const f32 wz = -offX * st + cz * ct;
            setPart(e, Vec3(gx + wx, baseY + ly + bob, gz + wz),
                    Quat::FromEulerAngles(s, th, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
        };
        f32 armL = swing,  armR = -swing;   // arms swing opposite each other (and to the legs)
        f32 legL = -swing, legR = swing;
        if (attacking) { armR = -1.3f; armL = 0.4f; legL = 0.2f; legR = -0.2f; } // right-arm thrust
        placeLimb(rig.armL, -0.26f, 0.97f, armL, 0.54f);
        placeLimb(rig.armR,  0.26f, 0.97f, armR, 0.54f);
        placeLimb(rig.legL, -0.12f, 0.50f, legL, 0.54f);
        placeLimb(rig.legR,  0.12f, 0.50f, legR, 0.54f);

        rig.lastPos = P;
    }
}

void DndDemoScene::UpdateShadows() {
    // Up to 16 axis-aligned occluder boxes cast ray-traced spotlight shadows. We
    // rebuild them every frame from the *moving* characters (so their shadows track
    // them) plus the handful of explored walls nearest Finn (so the travelling
    // lantern throws shifting corridor shadows too).
    Vec3 mins[16], maxs[16];
    i32 n = 0;
    auto addBox = [&](const Vec3& mn, const Vec3& mx) { if (n < 16) { mins[n] = mn; maxs[n] = mx; ++n; } };

    auto addChar = [&](Entity root) {
        const Transform* t = m_componentRegistry.GetComponent<Transform>(root);
        if (!t) return;
        const Vec3 p = t->position;
        // Box generously encloses the rig (incl. swinging limbs) so parts never self-shadow.
        addBox(Vec3(p.x - 0.45f, p.y - 0.5f, p.z - 0.45f),
               Vec3(p.x + 0.45f, p.y + 0.95f, p.z + 0.45f));
    };
    addChar(m_rogue);
    for (size_t i = 0; i < m_enemies.size(); ++i) {
        const HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(m_enemies[i]);
        if (hp && hp->IsAlive() && m_explored[m_enemyCells[i].y][m_enemyCells[i].x])
            addChar(m_enemies[i]);
    }

    // Fill the remaining budget with explored walls right around Finn.
    const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    if (rt) {
        const Cell fc = WorldToCell(rt->position);
        const i32 R = 2;
        for (size_t i = 0; i < m_walls.size() && n < 16; ++i) {
            const Cell c = m_wallCells[i];
            if (!m_explored[c.y][c.x]) continue;
            if (std::abs(c.x - fc.x) > R || std::abs(c.y - fc.y) > R) continue;
            const Vec3 w = CellToWorld(c.x, c.y);
            addBox(Vec3(w.x - 1.0f, 0.0f, w.z - 1.0f), Vec3(w.x + 1.0f, 1.5f, w.z + 1.0f));
        }
    }
    Renderer::SetOccluders(mins, maxs, n);
}

void DndDemoScene::UpdateKeyLight() {
    // A travelling lantern: the key light follows the hero, lighting the patch of
    // maze around him. The sliders act as the light's OFFSET from the hero.
    const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    const Vec3 hero = rt ? rt->position : Vec3(0.0f, 0.5f, 0.0f);
    const Vec3 lightPos(hero.x + m_lightPos.x, m_lightPos.y, hero.z + m_lightPos.z);
    const Vec3 target = hero;
    Spotlight key;
    key.position  = lightPos;
    key.direction = (target - lightPos).Normalized();
    // Warm key color scaled by the intensity slider.
    key.color     = Vec3(1.00f * m_lightIntensity,
                         0.90f * m_lightIntensity,
                         0.75f * m_lightIntensity);
    key.cosInner  = 0.90f; // bright core
    key.cosOuter  = 0.72f; // ~44-degree outer cone -> a generous pool around Finn
    key.range     = 44.0f;
    Renderer::SetSpotlights(&key, 1);
}

void DndDemoScene::OnUpdate(f32 deltaTime) {
    // Pull the latest slider values into the light params, then rebuild the light.
    if (m_lightSliderX) m_lightPos.x     = m_lightSliderX->GetValue();
    if (m_lightSliderY) m_lightPos.y     = m_lightSliderY->GetValue();
    if (m_lightSliderZ) m_lightPos.z     = m_lightSliderZ->GetValue();
    if (m_lightSliderI) m_lightIntensity = m_lightSliderI->GetValue();
    UpdateKeyLight();

    // Floor descent timer: after opening the chest, wait a beat then descend.
    if (m_descentPending) {
        m_descentTimer -= deltaTime;
        if (m_descentTimer <= 0.0f) {
            DescendFloor();
        }
        // While descending, skip the rest of the update.
        return;
    }

    // The orbit camera follows the hero as he threads the maze.
    if (const Transform* hero = m_componentRegistry.GetComponent<Transform>(m_rogue))
        Engine::SetCameraFocus(Vec3(hero->position.x, 0.5f, hero->position.z));

    // Each beat the next attacker acts. Melee attackers dash in, bump (damage),
    // and run back; ranged attackers stay put and launch a projectile (a sphere
    // "fire bolt" or a capsule "arrow") that flies to the target and resolves on
    // impact. Only the scripted attacker/target pairs interact, and the HP bars
    // track every mover.
    const f32 kLungeLeg = 0.30f;  // seconds per melee leg (out, then back)
    const f32 kFlyDur   = 0.45f;  // seconds for a projectile to reach its target
    const f32 kBeatGap  = 0.55f;  // pause between attacks

    if (m_lungePhase == LungePhase::IDLE) {
        if (m_combatSystem && m_replayIndex < m_replayActions.size()) {
            m_replayTimer += deltaTime;
            if (m_replayTimer >= kBeatGap) {
                m_replayTimer = 0.0f;
                const CombatAction& a = m_replayActions[m_replayIndex];
                const Delivery d = m_replayDelivery[m_replayIndex];
                Transform* at = m_componentRegistry.GetComponent<Transform>(a.source);
                Transform* tt = m_componentRegistry.GetComponent<Transform>(a.target);
                if (!at || !tt) {
                    // Can't animate — resolve instantly.
                    m_combatSystem->QueueAction(a);
                    PushLog(a.description ? a.description : "(acts)");
                    ++m_replayIndex;
                } else if (d == Delivery::MELEE) {
                    // Melee: dash in to ~1 unit short so the cubes touch.
                    m_lungeAttacker = a.source;
                    m_lungeHome = at->position;
                    Vec3 dir = (tt->position - at->position).Normalized();
                    m_lungeBump = Vec3(tt->position.x - dir.x * 1.05f,
                                       tt->position.y - dir.y * 1.05f,
                                       tt->position.z - dir.z * 1.05f);
                    m_lungePhase = LungePhase::OUT;
                    m_lungeT = 0.0f;
                } else {
                    // Ranged: launch the matching projectile from attacker to target.
                    m_flyingProjectile = (d == Delivery::BOLT) ? m_boltProjectile : m_arrowProjectile;
                    m_lungeHome = Vec3(at->position.x, at->position.y + 0.45f, at->position.z);
                    m_lungeBump = Vec3(tt->position.x, tt->position.y + 0.45f, tt->position.z);
                    Transform* pt = m_componentRegistry.GetComponent<Transform>(m_flyingProjectile);
                    MeshRenderer* pr = m_componentRegistry.GetComponent<MeshRenderer>(m_flyingProjectile);
                    if (pt) {
                        pt->position = m_lungeHome;
                        Vec3 fdir = (m_lungeBump - m_lungeHome).Normalized();
                        pt->rotation = Quat::FromToRotation(Vec3(0.0f, 1.0f, 0.0f), fdir); // aim the capsule
                    }
                    if (pr) pr->visible = true;
                    m_lungePhase = LungePhase::FLY;
                    m_lungeT = 0.0f;
                }
            }
        }
    } else if (m_lungePhase == LungePhase::FLY) {
        // Projectile in flight: ride from launch point to impact point (constant speed).
        m_lungeT += deltaTime / kFlyDur;
        f32 e = m_lungeT; if (e > 1.0f) e = 1.0f;
        Transform* pt = m_componentRegistry.GetComponent<Transform>(m_flyingProjectile);
        if (pt) pt->position = LerpVec(m_lungeHome, m_lungeBump, e);
        if (m_lungeT >= 1.0f) {
            // Impact: resolve the attack, then hide + park the projectile.
            const CombatAction& a = m_replayActions[m_replayIndex];
            m_combatSystem->QueueAction(a);
            PushLog(a.description ? a.description : "(acts)");
            MeshRenderer* pr = m_componentRegistry.GetComponent<MeshRenderer>(m_flyingProjectile);
            if (pr) pr->visible = false;
            if (pt) pt->position = Vec3(0.0f, -100.0f, 0.0f);
            m_lungePhase = LungePhase::IDLE;
            ++m_replayIndex;
            if (m_replayIndex == m_replayActions.size()) {
                PushLog("-- the dust settles --");
            }
        }
    } else {
        // Melee lunge: interpolate the attacker out to the bump and back.
        Transform* at = m_componentRegistry.GetComponent<Transform>(m_lungeAttacker);
        m_lungeT += deltaTime / kLungeLeg;
        const f32 e = SmoothStep01(m_lungeT);
        if (m_lungePhase == LungePhase::OUT) {
            if (at) at->position = LerpVec(m_lungeHome, m_lungeBump, e);
            if (m_lungeT >= 1.0f) {
                if (m_mazeAttack) {
                    ResolveMazeAttack();             // roll Finn's strike + the goblin's reprisal
                } else {
                    const CombatAction& a = m_replayActions[m_replayIndex];
                    m_combatSystem->QueueAction(a);
                    PushLog(a.description ? a.description : "(acts)");
                }
                m_lungePhase = LungePhase::BACK;
                m_lungeT = 0.0f;
            }
        } else { // BACK
            if (at) at->position = LerpVec(m_lungeBump, m_lungeHome, e);
            if (m_lungeT >= 1.0f) {
                if (at) at->position = m_lungeHome; // land exactly home
                m_lungePhase = LungePhase::IDLE;
                if (m_mazeAttack) {
                    m_mazeAttack = false;
                    const HitPoints* thp = m_componentRegistry.GetComponent<HitPoints>(m_attackTarget);
                    if (!thp || !thp->IsAlive()) {
                        m_blockingEnemy = Entity::None();
                        // Resume whatever Finn was doing: exploring the unknown, or
                        // heading for the (now-known) chest.
                        const b8 resumed = (m_exploring && !m_chestFound) ? StartExplore()
                                                                          : RepathToExit();
                        if (resumed) PushLog("The way clears -- Finn presses on.");
                    }
                    // else: the goblin still bars the path -- the player can "attack" again.
                } else {
                    ++m_replayIndex;
                    if (m_replayIndex == m_replayActions.size()) {
                        PushLog("-- the dust settles --");
                    }
                }
            }
        }
    }

    // ---- Text command box: Enter opens it, type your intent, Enter submits, Esc cancels ----
    if (Input::IsTextInputActive()) {
        if (Input::IsKeyPressed(KeyCode::BACKSPACE)) Input::Backspace();
        if (Input::IsKeyPressed(KeyCode::ENTER)) {
            const std::string cmd = Input::GetTextInput();
            Input::EndTextInput();
            if (!cmd.empty()) Command(cmd);
        } else if (Input::IsKeyPressed(KeyCode::ESCAPE)) {
            Input::EndTextInput();
        }
    } else {
        if (Input::IsKeyPressed(KeyCode::ENTER)) Input::BeginTextInput();      // open the command box
        if (Input::IsKeyPressed(KeyCode::P)) TryStartLockPick();               // raw deterministic lock-pick
        if (Input::IsKeyPressed(KeyCode::O)) Command("Finn walks over and picks the lock on the wooden chest.");
        if (Input::IsKeyPressed(KeyCode::I)) Command("inventory");            // check pack contents
        // L key: pull a lever if Finn is adjacent to one
        if (Input::IsKeyPressed(KeyCode::L)) {
            Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
            if (rt) {
                Cell fc = WorldToCell(rt->position);
                const i32 dx[4] = { 1, -1, 0, 0 };
                const i32 dy[4] = { 0, 0, 1, -1 };
                for (i32 k = 0; k < 4; ++k) {
                    i32 idx = TrapAt(fc.x + dx[k], fc.y + dy[k]);
                    if (idx >= 0 && m_traps[idx].type == TrapType::LEVER && m_traps[idx].active) {
                        TriggerTrap(idx);
                        break;
                    }
                }
            }
        }
    }

    // The local AI DM only handles intents the rules didn't recognize; poll for its reply.
    if (m_awaitingAi) {
        AiAction act;
        if (AiBrain::Poll(act)) {
            m_awaitingAi = false;
            if (!act.ok) {
                PushLog(std::string("AI DM error (") + (act.error.empty() ? "?" : act.error) +
                        ") -- the moment passes.");
            } else {
                ExecuteAction(act, true);
            }
        }
    }
    // Lock-pick roll: once Finn has reached the chest (walked there via the maze
    // pathfinder), the player presses SPACE to make the DEX / thieves'-tools check.
    if (m_lockPhase == LockPhase::AWAIT_ROLL) {
        if (Input::IsKeyPressed(KeyCode::SPACE) && !Input::IsTextInputActive()) {
            const i32 roll  = DiceRoller::Roll(1, 20);
            const i32 total = roll + m_lockMod;
            const b8  ok    = total >= m_lockDC;
            char b[128];
            std::snprintf(b, sizeof(b), "Finn rolls a %d %+d = %d vs DC %d -- %s",
                          roll, m_lockMod, total, m_lockDC, ok ? "SUCCESS!" : "failure.");
            PushLog(b);
            if (ok) {
                m_chestOpen = true;
                m_lockPhase = LockPhase::IDLE;
                if (m_currentFloor < kMaxFloors - 1) {
                    PushLog("The lock clicks open. Inside glints a rough-hewn dungeon key.");
                    PushLog("A grinding noise -- stone stairs descend into darkness!");
                    m_descentPending = true;
                    m_descentTimer = 3.0f;
                } else {
                    PushLog("The lock clicks open; the chest creaks wide -- treasure beyond Finn's dreams!");
                    PushLog("--- VICTORY! Finn emerges from the dungeon, laden with gold! ---");
                }
            } else {
                PushLog("The pick slips -- the lock holds fast. Press SPACE to try again.");
                // stay in AWAIT_ROLL so the player can retry
            }
        }
    }

    // ---- Maze navigation: follow the current A* path one cell at a time ----
    if (m_walking) {
        Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
        if (rt && m_pathIndex + 1 < m_path.size()) {
            // At the start of each segment, check the next cell: a living goblin there
            // halts Finn one cell short so the player can decide to fight.
            if (m_walkT == 0.0f) {
                CheckEnemyDetection();   // Stealth vs Perception for any goblin now in sight
                const Entity foe = LivingEnemyAt(m_path[m_pathIndex + 1]);
                if (foe.IsValid()) {
                    m_walking = false;
                    const b8 newBlock = (foe != m_blockingEnemy); // only announce once
                    m_blockingEnemy = foe;
                    if (newBlock) {
                        const i32 ei = EnemyIndex(foe);
                        const Tag* ft = m_componentRegistry.GetComponent<Tag>(foe);
                        const char* nm = (ft && ft->name) ? ft->name : "A goblin";
                        char b[176];
                        if (ei >= 0 && m_enemyAlerted[ei])
                            std::snprintf(b, sizeof(b), "%s bars the corridor, blade raised! Type \"attack\".", nm);
                        else
                            std::snprintf(b, sizeof(b), "%s stands oblivious in the corridor -- type \"attack\" to strike from the shadows!", nm);
                        PushLog(b);
                    }
                }
            }
            // Heading to the chest to pick its lock: stop in the cell just before it
            // and kneel, rather than stepping onto the chest.
            if (m_walking && m_pickPending && m_walkT == 0.0f) {
                const Cell nxt = m_path[m_pathIndex + 1];
                if (nxt.x == kExitX && nxt.y == kExitY) {
                    m_walking = false;
                    m_pickPending = false;
                    PushLog("Finn reaches the locked chest and kneels at the lock.");
                    BeginLockRoll();
                }
            }
            if (m_walking) {
                const f32 speed = 4.0f; // world units / second
                const Vec3 a = CellToWorld(m_path[m_pathIndex].x, m_path[m_pathIndex].y);
                const Vec3 b = CellToWorld(m_path[m_pathIndex + 1].x, m_path[m_pathIndex + 1].y);
                m_walkT += deltaTime * speed / kCell;
                if (m_walkT >= 1.0f) {
                    m_walkT = 0.0f;
                    ++m_pathIndex;
                    rt->position = b;
                    // Fog: reveal the cell Finn just stepped into (and its surroundings).
                    const bool wasFound = m_chestFound;
                    RevealAround(m_path[m_pathIndex]);
                    UpdateFogVisibility();
                    if (!wasFound && m_chestFound) {
                        m_walking = false;
                        m_exploring = false;
                        if (m_currentFloor > 0) {
                            PushLog("A faint magical glow marks the way deeper into the dungeon!");
                            m_descentPending = true;
                            m_descentTimer = 2.0f;
                        } else {
                            PushLog("Finn rounds a corner -- a locked chest sits in the gloom ahead!");
                            PushLog("Type \"pick the lock\" to go for it.");
                        }
                    }
                } else {
                    rt->position = LerpVec(a, b, m_walkT);
                }
            }
        } else {
            m_walking = false;
            if (m_pickPending) {
                m_pickPending = false;
                PushLog("Finn reaches the locked chest and kneels at the lock.");
                BeginLockRoll();
            } else if (m_exploring && !m_chestFound) {
                if (!StartExplore()) {                 // reached this frontier -- head to the next
                    m_exploring = false;
                    PushLog("Finn has explored everything he can reach.");
                }
            } else if (m_exploring) {
                m_exploring = false;                   // chest found mid-walk (already announced)
            } else if (m_chestFound) {
                if (m_currentFloor > 0) {
                    PushLog("Finn reaches the passage to the next depth.");
                    m_descentPending = true;
                    m_descentTimer = 2.0f;
                } else {
                    PushLog("Finn reaches the chest.");
                }
            } else if (rt) {
                PushLog("Finn stops.");
            }
        }
    }

    // ---- Golden "turn" halo: under the active combatant, or the rogue mid-attempt ----
    m_haloTime += deltaTime;
    Entity haloActor = Entity::None();
    bool   haloActive = false;
    if (m_lockPhase != LockPhase::IDLE) {
        haloActor = m_rogue; haloActive = true;                 // it's the rogue's turn
    } else if (m_replayIndex < m_replayActions.size()) {
        haloActor = m_replayActions[m_replayIndex].source; haloActive = true;
    }
    MeshRenderer* hr = m_componentRegistry.GetComponent<MeshRenderer>(m_haloEntity);
    Transform*    ht = m_componentRegistry.GetComponent<Transform>(m_haloEntity);
    if (hr && ht) {
        const Transform* actorT = haloActive
            ? m_componentRegistry.GetComponent<Transform>(haloActor) : nullptr;
        if (actorT) {
            const f32 pulse = 1.0f + 0.10f * std::sin(m_haloTime * 4.0f);
            ht->position = Vec3(actorT->position.x, 0.03f, actorT->position.z);
            ht->scale    = Vec3(pulse, 1.0f, pulse);
            hr->visible  = true;
        } else {
            hr->visible = false;
        }
    }

    // Trap logic: check if Finn is standing on a trap, update crumbling-floor timer,
    // recover cooldowns, trigger pressure plates by proximity.
    CheckTraps(deltaTime);

    // Finally, pose every humanoid rig from its (now-updated) root position,
    // then rebuild the moving shadow occluders to match.
    AnimateRigs(deltaTime);
    UpdateShadows();
}

void DndDemoScene::SetupUI() {
    UISystem& ui = UISystem::Get();

    // Layout (pixels, top-left origin).
    const f32 panelX = 14.0f, panelY = 14.0f;
    const f32 panelW = 232.0f, pad = 14.0f;
    const f32 rowH = 46.0f;       // vertical space per slider row
    const f32 sliderH = 16.0f;
    const f32 titleH = 22.0f;
    const f32 panelH = pad + titleH + 4.0f * rowH + pad;

    UIPanel* panel = ui.CreatePanel("LightPanel");
    panel->SetPosition(Vec2(panelX, panelY));
    panel->SetSize(Vec2(panelW, panelH));
    panel->SetColor(Vec4(0.07f, 0.07f, 0.10f, 0.82f));

    // Panel title (now drawn with the real bitmap font).
    UILabel* title = ui.CreateLabel("LightTitle", "LIGHT");
    title->SetPosition(Vec2(panelX + pad, panelY + pad));
    title->SetColor(Vec4(0.95f, 0.88f, 0.55f, 1.0f));

    const f32 sliderX = panelX + pad;
    const f32 sliderW = panelW - 2.0f * pad;
    f32 rowY = panelY + pad + titleH;

    // Each row: a text label on top, the slider just below it.
    auto makeSlider = [&](const std::string& id, const std::string& label,
                          f32 mn, f32 mx, f32 val, const Vec4& knob) -> UISlider* {
        UILabel* lbl = ui.CreateLabel(id + "Label", label);
        lbl->SetPosition(Vec2(sliderX, rowY));
        lbl->SetColor(Vec4(0.80f, 0.82f, 0.88f, 1.0f));

        UISlider* s = ui.CreateSlider(id, mn, mx, val);
        s->SetPosition(Vec2(sliderX, rowY + 18.0f));
        s->SetSize(Vec2(sliderW, sliderH));
        s->SetHandleColor(knob);
        rowY += rowH;
        return s;
    };

    // Color-coded knobs: X=red, Y=green, Z=blue (axis colors), Intensity=warm.
    m_lightSliderX = makeSlider("LightX", "X",         -15.0f, 15.0f, m_lightPos.x,     Vec4(0.90f, 0.30f, 0.30f, 1.0f));
    m_lightSliderY = makeSlider("LightY", "Y",           3.0f, 20.0f, m_lightPos.y,     Vec4(0.35f, 0.85f, 0.35f, 1.0f));
    m_lightSliderZ = makeSlider("LightZ", "Z",         -12.0f, 12.0f, m_lightPos.z,     Vec4(0.35f, 0.55f, 0.95f, 1.0f));
    m_lightSliderI = makeSlider("LightI", "INTENSITY",   0.5f,  4.0f, m_lightIntensity, Vec4(0.95f, 0.85f, 0.45f, 1.0f));

    // Player "intent" trigger (a stand-in for the future type-your-action box):
    // have the rogue walk over and attempt the locked chest. ('P' does the same.)
    UIButton* lockBtn = ui.CreateButton("PickLockBtn", "PICK LOCK");
    lockBtn->SetPosition(Vec2(panelX, panelY + panelH + 12.0f));
    lockBtn->SetSize(Vec2(panelW, 30.0f));
    lockBtn->SetColor(Vec4(0.34f, 0.27f, 0.12f, 0.92f));
    lockBtn->SetCallback([this]() { this->Command("Finn walks over and picks the lock on the wooden chest."); });
}

void DndDemoScene::PushLog(const std::string& line) {
    DND_LOG_INFO("[log] %s", line.c_str());   // also persist narration to engine.log
    m_combatLog.push_back(line);
    const size_t kMaxLines = 8;
    if (m_combatLog.size() > kMaxLines) {
        m_combatLog.erase(m_combatLog.begin());
    }
}

void DndDemoScene::TryStartLockPick(i32 dc, const std::string& flavor) {
    if (m_lockPhase != LockPhase::IDLE || m_pickPending) return;   // already on it
    if (m_lungePhase != LungePhase::IDLE || m_replayIndex < m_replayActions.size()) {
        PushLog("Finn is busy in the melee -- let the round finish first.");
        return;
    }
    if (m_currentFloor > 0) {
        PushLog("There's no chest here -- the only way is deeper into the dungeon.");
        return;
    }
    if (!m_chestFound) {
        PushLog("Finn hasn't found a chest yet -- \"explore\" the maze to find it first.");
        return;
    }
    const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
    if (!rt) return;

    // Dexterity (thieves' tools). Expertise adds the proficiency bonus twice.
    const CharacterInfo* info = m_componentRegistry.GetComponent<CharacterInfo>(m_rogue);
    m_lockDexMod = info ? AbilityScores::GetModifier(info->abilities.dexterity) : 0;
    const i32 baseProf = info ? info->proficiencyBonus : 2;
    const ProficiencyRank toolRank = info ? info->tools.thievesTools : ProficiencyRank::PROFICIENT;
    m_lockMod = CheckBonus(m_lockDexMod, baseProf, toolRank);
    m_lockProf = m_lockMod - m_lockDexMod;
    m_lockDC     = dc;
    if (!flavor.empty()) PushLog(flavor);

    // If Finn is already at the chest, kneel and roll. Otherwise A*-route him there
    // through the maze (cutting through any goblins on the way -- not through walls).
    const Cell here = WorldToCell(rt->position);
    const i32 dToChest = std::abs(here.x - kExitX) + std::abs(here.y - kExitY);
    if (dToChest <= 1) {
        BeginLockRoll();
    } else if (RepathToExit()) {
        m_pickPending = true;
        PushLog("Finn picks his way through the maze toward the locked chest...");
    } else {
        PushLog("Finn can't find a route to the chest from here.");
    }
}

void DndDemoScene::BeginLockRoll() {
    m_lockPhase = LockPhase::AWAIT_ROLL;
    m_lockT     = 0.0f;
    char b[140];
    std::snprintf(b, sizeof(b),
        "DEX (Thieves' Tools) check: d20 %+d (DEX %+d, prof %+d) vs DC %d -- press SPACE to roll.",
        m_lockMod, m_lockDexMod, m_lockProf, m_lockDC);
    PushLog(b);
}

void DndDemoScene::Command(const std::string& intent) {
    if (m_awaitingAi) { PushLog("(the DM is already resolving something...)"); return; }
    if (m_lungePhase != LungePhase::IDLE || m_replayIndex < m_replayActions.size() ||
        m_lockPhase != LockPhase::IDLE) {
        PushLog("Wait -- something's already happening.");
        return;
    }
    PushLog(std::string("> ") + intent);
    AiAction local;
    if (ParseIntentLocal(intent, local)) {
        ExecuteAction(local, false);   // recognized by the rules -> instant, deterministic
    } else {
        AskAi(intent);                 // unknown -> hand off to the local LLM
    }
}

b8 DndDemoScene::ParseIntentLocal(const std::string& text, AiAction& out) {
    std::string s = text;
    for (char& c : s) c = static_cast<char>(std::tolower((unsigned char)c));
    auto has = [&](const char* kw) { return s.find(kw) != std::string::npos; };

    // Lock-pick wins first: phrases like "walk over and pick the lock" mention movement
    // only incidentally -- the intent is the chest's lock.
    if (has("lock") || has("lockpick") || (has("pick") && (has("chest") || has("door")))) {
        out.action = "pick_lock"; out.ability = "DEX"; out.skill = "Thieves' Tools"; out.dc = 15;
        out.narration = "Finn kneels by the lock and works it with practiced fingers.";
        out.ok = true; return true;
    }
    // Pick up spotted loot ("grab", "take", "pick up" -- but not "pick the lock", handled above).
    if (has("pick up") || has("pickup") || has("grab") || has("take") ||
        has("loot") || has("collect") || has("pocket") || has("scoop")) {
        out.action = "pickup"; out.ability = "DEX"; out.skill = "Sleight of Hand"; out.dc = 0;
        out.narration = "Finn reaches down for what he spotted.";
        out.ok = true; return true;
    }
    // Move / explore the maze toward the chest.
    if (has("go to") || has("move") || has("walk") || has("navigate") ||
        has("explore") || has("head") || has("run to") || has("path to")) {
        out.action = "goto"; out.target = "exit"; out.ability = ""; out.skill = "";
        out.dc = 0; out.narration = "Finn studies the corridors and picks a route.";
        out.ok = true; return true;
    }
    if (has("search") || has("look") || has("inspect") || has("examine") ||
               has("trap") || has("investigate") || has("perceive")) {
        out.action = "search"; out.ability = "WIS"; out.skill = "Perception"; out.dc = 15;
        out.narration = "Finn scans the surroundings, eyes narrowed for anything out of place.";
    } else if (has("persuade") || has("convince") || has("talk") || has("charm") ||
               has("sweet") || has("bluff") || has("intimidate") || has("deceive")) {
        out.action = "persuade"; out.ability = "CHA"; out.skill = "Persuasion"; out.dc = 13;
        out.narration = "Finn flashes a disarming smile and picks his words with care.";
    } else if (has("sneak") || has("hide") || has("stealth") || has("creep") || has("quiet")) {
        out.action = "sneak"; out.ability = "DEX"; out.skill = "Stealth"; out.dc = 14;
        out.narration = "Finn melts into the shadows, each step deliberate and silent.";
    } else if (has("drink") || has("quaff") || has("use potion") || has("use healing")) {
        out.action = "drink"; out.ability = ""; out.skill = ""; out.dc = 0;
        out.narration = "Finn unstops a potion and drinks it down.";
        out.ok = true; return true;
    } else if (has("rest") || has("short rest") || has("catch breath") || has("pause")) {
        out.action = "rest"; out.ability = ""; out.skill = ""; out.dc = 0;
        out.narration = "Finn catches his breath and tends to his wounds.";
        out.ok = true; return true;
    } else if (has("inventory") || has("inv") || has("items") || has("pack") || has("bag") || has("what have i")) {
        out.action = "inventory"; out.ability = ""; out.skill = ""; out.dc = 0;
        out.narration = "Finn checks his pack.";
        out.ok = true; return true;
    } else if (has("attack") || has("hit") || has("strike") || has("stab") ||
               has("slash") || has("shoot") || has("swing")) {
        out.action = "attack"; out.ability = "DEX"; out.skill = "Attack"; out.dc = 12;
        out.narration = "Finn darts in with a sudden, practiced strike.";
    } else {
        return false; // unrecognized -> let the LLM handle it
    }
    out.ok = true;
    return true;
}

void DndDemoScene::ExecuteAction(const AiAction& act, b8 fromAi) {
    // Movement: explore the unknown maze, or (once found) head to the chest.
    if (act.action == "goto") {
        if (!act.narration.empty()) PushLog(act.narration);
        if (m_chestFound) {
            if (RepathToExit()) {
                char b[128];
                if (m_currentFloor == 0) {
                    std::snprintf(b, sizeof(b), "Finn threads the maze toward the chest (%d steps).",
                                  static_cast<i32>(m_path.size()) - 1);
                } else {
                    std::snprintf(b, sizeof(b), "Finn follows the passage deeper (%d steps).",
                                  static_cast<i32>(m_path.size()) - 1);
                }
                PushLog(b);
            } else {
                PushLog("Finn can't find a route forward.");
            }
        } else {
            // The maze is unknown -- Finn ventures into the dark, revealing as he goes.
            if (StartExplore()) {
                m_exploring = true;
                PushLog("Finn ventures deeper into the maze, lantern held high...");
            } else {
                PushLog("There's nowhere left to explore.");
            }
        }
        return;
    }

    // Combat: a real melee attack against the goblin blocking the corridor (or the
    // nearest one in reach). The strike + reprisal resolve at the lunge's bump.
    if (act.action == "attack") {
        Entity foe = m_blockingEnemy;
        const HitPoints* bhp = foe.IsValid() ? m_componentRegistry.GetComponent<HitPoints>(foe) : nullptr;
        if (!bhp || !bhp->IsAlive()) {
            foe = Entity::None();
            const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
            f32 best = 2.8f; // about one cell of reach
            if (rt) {
                for (const Entity e : m_enemies) {
                    const HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(e);
                    const Transform* et = m_componentRegistry.GetComponent<Transform>(e);
                    if (!hp || !hp->IsAlive() || !et) continue;
                    const f32 dx = et->position.x - rt->position.x;
                    const f32 dz = et->position.z - rt->position.z;
                    const f32 d = std::sqrt(dx * dx + dz * dz);
                    if (d < best) { best = d; foe = e; }
                }
            }
        }
        const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
        const Transform* et = foe.IsValid() ? m_componentRegistry.GetComponent<Transform>(foe) : nullptr;
        if (!foe.IsValid() || !rt || !et) {
            PushLog("There's nothing in reach to strike.");
            return;
        }
        if (!act.narration.empty()) PushLog(act.narration);
        m_lungeAttacker = m_rogue;
        m_lungeHome = rt->position;
        const Vec3 dir = (et->position - rt->position).Normalized();
        m_lungeBump = Vec3(et->position.x - dir.x * 1.05f, rt->position.y,
                           et->position.z - dir.z * 1.05f);
        m_attackTarget = foe;
        m_mazeAttack = true;
        m_lungePhase = LungePhase::OUT;
        m_lungeT = 0.0f;
        return;
    }

    // Drink a potion from inventory.
    if (act.action == "drink") {
        if (!act.narration.empty()) PushLog(act.narration);
        Inventory* inv = m_componentRegistry.GetComponent<Inventory>(m_rogue);
        HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(m_rogue);
        if (inv && hp && inv->GetItemCount("Potion of Healing") > 0) {
            inv->RemoveItem("Potion of Healing", 1);
            const i32 heal = DiceRoller::Roll(2, 4) + 2;
            hp->Heal(heal);
            char b[80];
            std::snprintf(b, sizeof(b), "Finn regains %d hit points. (%d/%d)",
                          heal, hp->current, hp->maximum);
            PushLog(b);
        } else {
            PushLog("Finn has no potions to drink.");
        }
        return;
    }

    // Short rest: spend one Hit Die (d8 for rogue) to recover HP.
    if (act.action == "rest") {
        if (!act.narration.empty()) PushLog(act.narration);
        const CharacterInfo* info = m_componentRegistry.GetComponent<CharacterInfo>(m_rogue);
        HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(m_rogue);
        if (info && hp && hp->current < hp->maximum) {
            const i32 dieRoll = DiceRoller::Roll(1, 8) + AbilityScores::GetModifier(info->abilities.constitution);
            const i32 healed = (dieRoll < 1) ? 1 : dieRoll;
            hp->Heal(healed);
            char b[128];
            std::snprintf(b, sizeof(b), "Finn spends a Hit Die: regains %d HP. (%d/%d)",
                          healed, hp->current, hp->maximum);
            PushLog(b);
            if (hp->current >= hp->maximum) {
                PushLog("Finn feels fully refreshed!");
            }
        } else if (hp && hp->current >= hp->maximum) {
            PushLog("Finn is already at full health.");
        } else {
            PushLog("Finn is too worn out to rest now.");
        }
        return;
    }

    // Show inventory.
    if (act.action == "inventory") {
        Inventory* inv = m_componentRegistry.GetComponent<Inventory>(m_rogue);
        if (inv && !inv->items.empty()) {
            PushLog("Finn's pack contains:");
            for (const auto& stack : inv->items) {
                if (stack.item && !stack.IsEmpty()) {
                    char b[80];
                    std::snprintf(b, sizeof(b), "  %s x%d", stack.item->name, stack.quantity);
                    PushLog(b);
                }
            }
        } else {
            PushLog("Finn's pack is empty.");
        }
        return;
    }

    // Perception search + pickup of hidden maze loot.
    if (act.action == "search") { DoPerceptionSearch(); return; }
    if (act.action == "pickup") { DoPickup();          return; }

    char hdr[160];
    std::snprintf(hdr, sizeof(hdr), "%s: %s  [%s / %s, DC %d]",
                  fromAi ? "AI DM" : "DM", act.action.c_str(), act.ability.c_str(),
                  act.skill.c_str(), act.dc);
    PushLog(hdr);
    if (act.action.find("lock") != std::string::npos) {
        TryStartLockPick(act.dc, act.narration);   // walks over, then the player rolls
    } else {
        GenericCheck(act);                          // abstract ability check
    }
}

void DndDemoScene::AskAi(const std::string& intent) {
    const std::string sys =
        "You are the rules adjudicator for a D&D-style game. The player types what their character "
        "wants to do. Reply with ONLY these six lines, nothing before or after:\n"
        "ACTION: <pick_lock|attack|search|persuade|sneak|move|other>\n"
        "TARGET: <short target name or none>\n"
        "ABILITY: <STR|DEX|CON|INT|WIS|CHA>\n"
        "SKILL: <relevant skill or tool>\n"
        "DC: <integer 5-25 you choose>\n"
        "NARRATION: <one vivid present-tense sentence>\n"
        "Scene: a rogue named Finn picking through a stone maze, goblins lurking in the "
        "corridors, and a locked wooden chest waiting at the far exit.";
    if (AiBrain::Submit(sys, intent)) {
        m_awaitingAi = true;
        PushLog("(no rule matched -- asking the local AI DM...)");
    } else {
        PushLog("(the AI DM is unreachable -- nothing happens)");
    }
}

void DndDemoScene::GenericCheck(const AiAction& act) {
    if (!act.narration.empty()) PushLog(act.narration);
    const CharacterInfo* info = m_componentRegistry.GetComponent<CharacterInfo>(m_rogue);
    i32 mod = 0;
    if (info) {
        Skill skill;
        Ability ability;
        if (TryParseSkill(act.skill.c_str(), skill)) {
            mod = info->skills.GetBonus(skill, info->abilities, info->proficiencyBonus);
        } else if (TryParseAbility(act.ability.c_str(), ability)) {
            mod = GetAbilityModifier(info->abilities, ability);
        }
    }
    const D20Check check = RollD20Check(RollMode::NORMAL, mod);
    const i32 roll  = check.natural;
    const i32 total = check.total;
    const b8  ok    = CheckSucceeds(check, act.dc);
    char b[128];
    std::snprintf(b, sizeof(b), "Finn rolls d20: %d %+d = %d vs DC %d -- %s",
                  roll, mod, total, act.dc, ok ? "SUCCESS!" : "failure.");
    PushLog(b);
}

namespace {
// Projects a world-space point to screen pixels (top-left origin, Y down).
// Returns false if the point is at/behind the camera. Mat4 is column-major (m[col*4+row]).
bool ProjectToScreen(const Vec3& world, const Mat4& view, const Mat4& proj,
                     f32 screenW, f32 screenH, f32& outX, f32& outY) {
    auto mul = [](const Mat4& M, f32 x, f32 y, f32 z, f32 w,
                  f32& ox, f32& oy, f32& oz, f32& ow) {
        ox = M.m[0] * x + M.m[4] * y + M.m[8]  * z + M.m[12] * w;
        oy = M.m[1] * x + M.m[5] * y + M.m[9]  * z + M.m[13] * w;
        oz = M.m[2] * x + M.m[6] * y + M.m[10] * z + M.m[14] * w;
        ow = M.m[3] * x + M.m[7] * y + M.m[11] * z + M.m[15] * w;
    };
    f32 vx, vy, vz, vw;
    mul(view, world.x, world.y, world.z, 1.0f, vx, vy, vz, vw);
    f32 cx, cy, cz, cw;
    mul(proj, vx, vy, vz, vw, cx, cy, cz, cw);
    (void)cz;
    if (cw <= 0.0001f) return false; // at or behind the camera plane
    outX = (cx / cw * 0.5f + 0.5f) * screenW;
    outY = (1.0f - (cy / cw * 0.5f + 0.5f)) * screenH; // flip Y: NDC up -> screen down
    return true;
}
} // namespace

void DndDemoScene::OnRenderOverlay(f32 screenW, f32 screenH) {
    const Mat4& view = Renderer::GetViewMatrix();
    const Mat4& proj = Renderer::GetProjectionMatrix();

    // ---- Floating HP bars + name tags above Finn and every living goblin ----
    std::vector<Entity> shown;
    shown.push_back(m_rogue);
    for (size_t i = 0; i < m_enemies.size(); ++i) {
        const HitPoints* fhp = m_componentRegistry.GetComponent<HitPoints>(m_enemies[i]);
        if (fhp && fhp->IsAlive() && m_explored[m_enemyCells[i].y][m_enemyCells[i].x])
            shown.push_back(m_enemies[i]);
    }
    for (const Entity e : shown) {
        const Transform* t  = m_componentRegistry.GetComponent<Transform>(e);
        const HitPoints* hp = m_componentRegistry.GetComponent<HitPoints>(e);
        if (!t || !hp) continue;

        f32 sx, sy;
        if (!ProjectToScreen(Vec3(t->position.x, t->position.y + 1.2f, t->position.z),
                             view, proj, screenW, screenH, sx, sy)) {
            continue;
        }

        const f32 bw = 46.0f, bh = 7.0f;
        const f32 bx = sx - bw * 0.5f;
        const f32 by = sy - bh * 0.5f;
        f32 frac = (hp->maximum > 0) ? static_cast<f32>(hp->current) / static_cast<f32>(hp->maximum) : 0.0f;
        if (frac < 0.0f) frac = 0.0f;
        if (frac > 1.0f) frac = 1.0f;

        // Border, empty (dark red) track, then the colored fill (green -> red as HP falls).
        UIRenderer::DrawRect(bx - 1.0f, by - 1.0f, bw + 2.0f, bh + 2.0f, Vec4(0.0f, 0.0f, 0.0f, 0.65f));
        UIRenderer::DrawRect(bx, by, bw, bh, Vec4(0.30f, 0.05f, 0.05f, 0.90f));
        if (frac > 0.0f) {
            f32 rr = (1.0f - frac) * 2.0f; if (rr > 1.0f) rr = 1.0f;
            f32 gg = frac * 2.0f;          if (gg > 1.0f) gg = 1.0f;
            UIRenderer::DrawRect(bx, by, bw * frac, bh, Vec4(rr, gg, 0.12f, 0.95f));
        }

        char buf[24];
        std::snprintf(buf, sizeof(buf), "%d/%d", hp->current, hp->maximum);
        UIRenderer::DrawText(bx, by - 13.0f, buf, Vec4(0.95f, 0.95f, 1.0f, 1.0f), 10.0f);

        // Name tag above the bar (party greenish, enemies reddish) so they're tellable apart.
        const Tag* tag = m_componentRegistry.GetComponent<Tag>(e);
        if (tag && tag->name) {
            const f32 ns = 9.0f;
            const f32 nameW = static_cast<f32>(std::strlen(tag->name)) * ns;
            const Vec4 nameCol = (e == m_rogue) ? Vec4(0.70f, 0.92f, 0.70f, 1.0f)
                                                : Vec4(0.96f, 0.62f, 0.55f, 1.0f);
            UIRenderer::DrawText(sx - nameW * 0.5f, by - 27.0f, tag->name, nameCol, ns);
        }
    }

    // ---- Gold labels over revealed-but-unclaimed loot ----
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (!m_itemFound[i] || m_itemTaken[i]) continue;
        const Transform* it  = m_componentRegistry.GetComponent<Transform>(m_items[i]);
        const Tag*       tag = m_componentRegistry.GetComponent<Tag>(m_items[i]);
        if (!it || !tag || !tag->name) continue;
        f32 sx, sy;
        if (!ProjectToScreen(Vec3(it->position.x, it->position.y + 0.5f, it->position.z),
                             view, proj, screenW, screenH, sx, sy)) continue;
        const f32 ns = 8.0f;
        const f32 w  = static_cast<f32>(std::strlen(tag->name)) * ns;
        UIRenderer::DrawText(sx - w * 0.5f, sy - 10.0f, tag->name, Vec4(1.0f, 0.86f, 0.40f, 1.0f), ns);
    }

    // ---- Combat log (bottom-left) ----
    if (!m_combatLog.empty()) {
        const f32 lineH = 15.0f, padL = 10.0f, logW = 496.0f;
        const f32 logH = padL * 2.0f + static_cast<f32>(m_combatLog.size()) * lineH;
        const f32 logX = 14.0f;
        const f32 logY = screenH - logH - (Input::IsTextInputActive() ? 44.0f : 14.0f);
        UIRenderer::DrawRect(logX, logY, logW, logH, Vec4(0.05f, 0.05f, 0.08f, 0.78f));
        for (size_t i = 0; i < m_combatLog.size(); ++i) {
            UIRenderer::DrawText(logX + padL, logY + padL + static_cast<f32>(i) * lineH,
                                 m_combatLog[i].c_str(), Vec4(0.85f, 0.86f, 0.92f, 1.0f), 10.0f);
        }
    }

    // ---- Lock-pick roll prompt (centered, while waiting on the player's roll) ----
    if (m_lockPhase == LockPhase::AWAIT_ROLL) {
        char pr[128];
        std::snprintf(pr, sizeof(pr), "FINN PICKS THE LOCK -- press SPACE: d20 %+d  vs  DC %d",
                      m_lockMod, m_lockDC);
        UIRenderer::DrawText(screenW * 0.5f - 250.0f, 64.0f, pr, Vec4(1.0f, 0.90f, 0.45f, 1.0f), 12.0f);
    }

    // ---- Command input box (when typing) ----
    if (Input::IsTextInputActive()) {
        const f32 boxH = 28.0f;
        const f32 boxY = screenH - boxH - 6.0f;
        UIRenderer::DrawRect(8.0f, boxY, screenW - 16.0f, boxH, Vec4(0.05f, 0.05f, 0.10f, 0.92f));
        const std::string line = std::string("> ") + Input::GetTextInput() + "_";
        UIRenderer::DrawText(16.0f, boxY + 7.0f, line.c_str(), Vec4(0.95f, 0.95f, 0.70f, 1.0f), 13.0f);
    } else {
        UIRenderer::DrawText(screenW - 250.0f, screenH - 22.0f, "[Enter] type a command",
                             Vec4(0.60f, 0.60f, 0.66f, 1.0f), 10.0f);
    }

    // ---- Poll enemy script responses + send init events ----
    if (!m_enemies.empty()) {
        for (Entity e : m_enemies) {
            ScriptComponent* sc = m_componentRegistry.GetComponent<ScriptComponent>(e);
            if (!sc || !sc->isRunning) continue;

            // Send init event once when script first starts
            b8 alreadyInit = false;
            for (Entity ini : m_scriptInited) {
                if (ini.id == e.id) { alreadyInit = true; break; }
            }
            if (!alreadyInit) {
                const Tag* tag = m_componentRegistry.GetComponent<Tag>(e);
                const char* nm = (tag && tag->name) ? tag->name : "goblin";
                ScriptSystem::NotifyEvent(m_componentRegistry, e,
                    "{\"type\":\"init\",\"name\":\"" + std::string(nm) + "\"}");
                m_scriptInited.push_back(e);
            }

            std::string resp = ScriptSystem::ReadResponse(sc);
            if (!resp.empty()) {
                if (!resp.empty() && resp.back() == '\n') resp.pop_back();
                if (!resp.empty()) {
                    PushLog(resp);
                }
            }
        }
    }

    // ---- Trap warnings ----
    if (!m_traps.empty()) {
        const Transform* rt = m_componentRegistry.GetComponent<Transform>(m_rogue);
        if (rt) {
            Cell fc = WorldToCell(rt->position);
            // Crumbling floor warning
            for (const auto& trap : m_traps) {
                if (!trap.active) continue;
                if (trap.type == TrapType::CRUMBLING_FLOOR &&
                    trap.cell.x == fc.x && trap.cell.y == fc.y) {
                    f32 remain = trap.standThreshold - trap.standTimer;
                    if (remain > 0.0f && remain < 1.5f) {
                        char warn[48];
                        std::snprintf(warn, sizeof(warn), "The floor trembles! (%.1fs)", static_cast<double>(remain));
                        UIRenderer::DrawText(screenW * 0.5f - 90.0f, 100.0f, warn,
                                             Vec4(1.0f, 0.5f, 0.1f, 1.0f), 11.0f);
                    }
                }
            }
            // Lever hint: if adjacent to a lever
            const i32 dx[4] = { 1, -1, 0, 0 };
            const i32 dy[4] = { 0, 0, 1, -1 };
            for (i32 k = 0; k < 4; ++k) {
                i32 idx = TrapAt(fc.x + dx[k], fc.y + dy[k]);
                if (idx >= 0 && m_traps[idx].type == TrapType::LEVER && m_traps[idx].active) {
                    UIRenderer::DrawText(screenW * 0.5f - 70.0f, 82.0f,
                                         "Press [L] to pull the lever",
                                         Vec4(0.8f, 0.8f, 0.4f, 1.0f), 10.0f);
                    break;
                }
            }
        }
    }

    // ---- Pause indicator ----
    if (Time::GetPaused()) {
        UIRenderer::DrawText(screenW * 0.5f - 80.0f, 50.0f, "=== PAUSED (F6) ===",
                             Vec4(1.0f, 0.3f, 0.3f, 1.0f), 18.0f);
    }

    // ---- FPS (top-right) ----
    char fpsText[24];
    std::snprintf(fpsText, sizeof(fpsText), "FPS %.0f", static_cast<double>(Time::GetFPS()));
    UIRenderer::DrawText(screenW - 96.0f, 14.0f, fpsText, Vec4(0.70f, 0.92f, 0.70f, 1.0f), 11.0f);

    // ---- XP bar + level (top-left) ----
    const Experience* exp = m_componentRegistry.GetComponent<Experience>(m_rogue);
    if (exp) {
        char lvlText[24];
        std::snprintf(lvlText, sizeof(lvlText), "Level %d", exp->level);
        UIRenderer::DrawText(14.0f, 14.0f, lvlText, Vec4(0.85f, 0.86f, 0.92f, 1.0f), 13.0f);

        i32 nextLevelXP = Progression::GetExperienceRequired(exp->level + 1);
        i32 thisLevelXP = Progression::GetExperienceRequired(exp->level);
        i32 needed = nextLevelXP - thisLevelXP;
        if (needed > 0 && exp->level < 20) {
            i32 have = exp->currentXP - thisLevelXP;
            f32 frac = static_cast<f32>(have) / static_cast<f32>(needed);
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;
            const f32 barW = 120.0f, barH = 8.0f;
            const f32 barY = 36.0f;
            UIRenderer::DrawRect(14.0f, barY, barW, barH, Vec4(0.15f, 0.15f, 0.20f, 0.85f));
            UIRenderer::DrawRect(14.0f, barY, barW * frac, barH, Vec4(0.20f, 0.60f, 0.90f, 0.90f));
            char xpText[32];
            std::snprintf(xpText, sizeof(xpText), "%d / %d XP", have, needed);
            UIRenderer::DrawText(14.0f, barY - 1.0f, xpText, Vec4(0.75f, 0.80f, 0.90f, 1.0f), 7.0f);
        } else if (exp->level >= 20) {
            UIRenderer::DrawText(14.0f, 36.0f, "Max Level", Vec4(1.0f, 0.85f, 0.40f, 1.0f), 9.0f);
        }
    }
}

Entity DndDemoScene::CreateCharacter(const char* name, RaceType race, ClassType classType,
                                      const AbilityScores& abilities, const Vec3& position) {
    Entity entity = m_entityManager.Create();

    // Basic components
    m_componentRegistry.AddComponent<Tag>(entity, Tag(name));
    m_componentRegistry.AddComponent<Transform>(entity, Transform(
        position, Quat::Identity(), Vec3(1, 1, 1)));

    // Visual representation
    m_componentRegistry.AddComponent<MeshRenderer>(entity, MeshRenderer(m_cubeMesh, Color(0.2f, 0.6f, 0.2f, 1)));

    // D&D character info — auto-calculates HP, AC, proficiency.
    // Saving throws come from the class. Finn's rogue build (the only character
    // this demo creates) takes Stealth expertise, Perception, Sleight of Hand,
    // Acrobatics, and thieves' tools expertise.
    CharacterInfo info(name, race, classType, abilities);
    ApplyClassSavingThrows(classType, info.saves);
    if (classType == ClassType::ROGUE) {
        info.skills.AddExpertise(Skill::STEALTH);
        info.skills.AddProficiency(Skill::PERCEPTION);
        info.skills.AddProficiency(Skill::SLEIGHT_OF_HAND);
        info.skills.AddProficiency(Skill::ACROBATICS);
        info.tools.thievesTools = ProficiencyRank::EXPERTISE;
    }
    m_componentRegistry.AddComponent<CharacterInfo>(entity, info);

    // HitPoints
    HitPoints hp;
    hp.maximum = info.maxHP;
    hp.current = hp.maximum;
    m_componentRegistry.AddComponent<HitPoints>(entity, hp);

    // ArmorClass
    ArmorClass ac;
    ac.base = info.armorClass;
    m_componentRegistry.AddComponent<ArmorClass>(entity, ac);

    // Initiative (DEX modifier)
    Initiative init;
    init.value = AbilityScores::GetModifier(abilities.dexterity);
    m_componentRegistry.AddComponent<Initiative>(entity, init);

    // Speed based on race
    Speed speed;
    speed.walk = RaceDatabase::GetRace(race).speed;
    m_componentRegistry.AddComponent<Speed>(entity, speed);

    // Experience
    Experience xp;
    xp.currentXP = 0;
    xp.level = 1;
    m_componentRegistry.AddComponent<Experience>(entity, xp);

    // Inventory and equipment
    Inventory inventory;
    inventory.maxItems = 20;
    m_componentRegistry.AddComponent<Inventory>(entity, inventory);

    EquipmentSlots equipment;
    m_componentRegistry.AddComponent<EquipmentSlots>(entity, equipment);

    // Spellcasting for Wizard and Cleric
    if (classType == ClassType::WIZARD || classType == ClassType::CLERIC) {
        ClassData cls = ClassDatabase::GetClass(classType);
        if (cls.isSpellcaster) {
            SpellSlots slots;
            slots.maxSlots[1] = 2;  // Two 1st-level slots at level 1
            slots.slots[1] = 2;
            m_componentRegistry.AddComponent<SpellSlots>(entity, slots);

            Spellbook spellbook;
            spellbook.spellsKnown = 3;
            // Add some starter spells (defined in spellbook)
            SpellData fireBolt;
            fireBolt.name = "Fire Bolt";
            fireBolt.level = 0;
            fireBolt.school = SpellSchool::EVOCATION;
            fireBolt.castingTime = SpellCastingTime::ACTION;
            fireBolt.duration.isInstant = true;
            fireBolt.range = SpellRange(/*isSelf*/ false, /*isTouch*/ false, /*isRanged*/ true, 120);
            fireBolt.requiresVerbal = true;
            fireBolt.requiresSomatic = true;
            fireBolt.requiresAttackRoll = true;
            fireBolt.damage.dice.numDice = 1;
            fireBolt.damage.dice.numSides = 10;
            fireBolt.damage.type = DamageType::FIRE;
            spellbook.spells.push_back(fireBolt);

            SpellData cureWounds;
            cureWounds.name = "Cure Wounds";
            cureWounds.level = 1;
            cureWounds.school = SpellSchool::CONJURATION;
            cureWounds.castingTime = SpellCastingTime::ACTION;
            cureWounds.duration.isInstant = true;
            cureWounds.range = SpellRange(/*isSelf*/ false, /*isTouch*/ true, /*isRanged*/ false, 0);
            cureWounds.requiresVerbal = true;
            cureWounds.requiresSomatic = true;
            spellbook.spells.push_back(cureWounds);

            m_componentRegistry.AddComponent<Spellbook>(entity, spellbook);
        }
    }

    return entity;
}

Entity DndDemoScene::CreateEnemy(const char* name, EnemyType type, i32 level, const Vec3& position) {
    Entity entity = m_entityManager.Create();

    m_componentRegistry.AddComponent<Tag>(entity, Tag(name));
    m_componentRegistry.AddComponent<Transform>(entity, Transform(
        position, Quat::Identity(), Vec3(1, 1, 1)));

    DND_UNUSED(level); // Stat blocks are fixed CR, not a scaled fighter level.

    const MonsterKind kind = (type == EnemyType::SKELETON) ? MonsterKind::SKELETON
                           : (type == EnemyType::BUGBEAR) ? MonsterKind::BUGBEAR
                           : MonsterKind::GOBLIN;
    const MonsterProfile profile = GetMonsterProfile(kind);

    // Appearance and weapon depend on enemy type
    Color skinColor(0.8f, 0.2f, 0.1f, 1);
    switch (type) {
        case EnemyType::SKELETON:
            skinColor = Color(0.88f, 0.82f, 0.72f, 1); // bone white
            break;
        case EnemyType::BUGBEAR:
            skinColor = Color(0.38f, 0.22f, 0.08f, 1); // dark fur
            break;
        default: // GOBLIN
            skinColor = Color(0.50f, 0.68f, 0.18f, 1); // green
            break;
    }
    m_componentRegistry.AddComponent<MeshRenderer>(entity, MeshRenderer(m_cubeMesh, skinColor));

    // The class is only a placeholder so CharacterInfo can exist. HP, AC, abilities,
    // and skills come from the stat block, not from the fighter progression.
    CharacterInfo info(name, RaceType::HUMAN, ClassType::FIGHTER, profile.abilities,
                       SkillProficiencies(), /*level*/ 1, /*isNPC*/ true, /*isHostile*/ true);
    info.maxHP = profile.hitPoints;
    info.armorClass = profile.armorClass;
    info.proficiencyBonus = profile.proficiencyBonus;
    ApplyMonsterSkills(profile, info.skills);
    m_componentRegistry.AddComponent<CharacterInfo>(entity, info);

    HitPoints hp;
    hp.maximum = info.maxHP;
    hp.current = hp.maximum;
    m_componentRegistry.AddComponent<HitPoints>(entity, hp);

    ArmorClass ac;
    ac.base = info.armorClass; // single source of truth (matches CharacterInfo)
    m_componentRegistry.AddComponent<ArmorClass>(entity, ac);

    Initiative init;
    init.value = AbilityScores::GetModifier(profile.abilities.dexterity);
    m_componentRegistry.AddComponent<Initiative>(entity, init);

    Speed speed;
    speed.walk = 30.0f;
    m_componentRegistry.AddComponent<Speed>(entity, speed);

    // Enemies get a weapon matching their type. Populate the local inventory first, then add the
    // component exactly once.
    Inventory inventory;
    inventory.maxItems = 5;
    const ItemData* weapon = profile.weaponItem ? ItemDatabase::GetItem(profile.weaponItem) : nullptr;
    if (weapon) { inventory.AddItem(weapon); }
    m_componentRegistry.AddComponent<Inventory>(entity, inventory);

    // Attach an optional AI script to the enemy. If the script file exists,
    // it will be auto-started and receive combat events (damage, death, etc.).
    ScriptComponent script;
    std::strncpy(script.scriptPath, "Assets/scripts/goblin_ai.py", sizeof(script.scriptPath) - 1);
    script.scriptPath[sizeof(script.scriptPath) - 1] = '\0';
    script.language = ScriptLanguage::Python;
    script.autoStart = true;
    m_componentRegistry.AddComponent<ScriptComponent>(entity, script);

    return entity;
}

} // namespace Dnd
