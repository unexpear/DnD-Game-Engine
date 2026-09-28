#include "scene/scene_serializer.h"
#include "scene/scene.h"
#include "core/json.h"
#include "core/log.h"
#include "ecs/entity.h"
#include "ecs/component.h"
#include "ecs/components/transform.h"
#include "ecs/components/tag.h"
#include "ecs/components/mesh_renderer.h"
#include "ecs/components/rigidbody.h"
#include "ecs/components/camera.h"
#include "ecs/components/light.h"
#include "audio/audio_source.h"
#include "audio/audio_clip.h"
#include "ecs/components/script.h"
#include "game/combat_stats.h"
#include "assets/asset_manager.h"
#include <cstdio>
#include <unordered_map>
#include <unordered_set>

namespace Dnd {

// ==========================================
// Serialization helpers
// ==========================================

static JsonValue Vec3ToJson(const Vec3& v) {
    JsonValue arr = JsonValue::Array();
    arr.arrVal.push_back(JsonValue(v.x));
    arr.arrVal.push_back(JsonValue(v.y));
    arr.arrVal.push_back(JsonValue(v.z));
    return arr;
}

static Vec3 Vec3FromJson(const JsonValue& arr) {
    Vec3 v;
    if (arr.type == JsonType::Array && arr.arrVal.size() >= 3) {
        v.x = static_cast<f32>(arr.arrVal[0].AsNumber());
        v.y = static_cast<f32>(arr.arrVal[1].AsNumber());
        v.z = static_cast<f32>(arr.arrVal[2].AsNumber());
    }
    return v;
}

static JsonValue QuatToJson(const Quat& q) {
    JsonValue arr = JsonValue::Array();
    arr.arrVal.push_back(JsonValue(q.x));
    arr.arrVal.push_back(JsonValue(q.y));
    arr.arrVal.push_back(JsonValue(q.z));
    arr.arrVal.push_back(JsonValue(q.w));
    return arr;
}

static Quat QuatFromJson(const JsonValue& arr) {
    Quat q;
    if (arr.type == JsonType::Array && arr.arrVal.size() >= 4) {
        q.x = static_cast<f32>(arr.arrVal[0].AsNumber());
        q.y = static_cast<f32>(arr.arrVal[1].AsNumber());
        q.z = static_cast<f32>(arr.arrVal[2].AsNumber());
        q.w = static_cast<f32>(arr.arrVal[3].AsNumber());
    }
    return q;
}

static JsonValue ColorToJson(const Color& c) {
    JsonValue arr = JsonValue::Array();
    arr.arrVal.push_back(JsonValue(c.r));
    arr.arrVal.push_back(JsonValue(c.g));
    arr.arrVal.push_back(JsonValue(c.b));
    arr.arrVal.push_back(JsonValue(c.a));
    return arr;
}

static Color ColorFromJson(const JsonValue& arr) {
    Color c;
    if (arr.type == JsonType::Array && arr.arrVal.size() >= 4) {
        c.r = static_cast<f32>(arr.arrVal[0].AsNumber());
        c.g = static_cast<f32>(arr.arrVal[1].AsNumber());
        c.b = static_cast<f32>(arr.arrVal[2].AsNumber());
        c.a = static_cast<f32>(arr.arrVal[3].AsNumber());
    }
    return c;
}

// ==========================================
// Component serializers
// ==========================================

static JsonValue SerializeTransform(const Transform& t) {
    JsonValue obj = JsonValue::Object();
    obj["position"] = Vec3ToJson(t.position);
    obj["rotation"] = QuatToJson(t.rotation);
    obj["scale"] = Vec3ToJson(t.scale);
    obj["parent"] = JsonValue(static_cast<int>(t.parent.IsValid() ? Entity::GetIndex(t.parent.id) : -1));
    return obj;
}

static JsonValue SerializeTag(const Tag& t) {
    JsonValue obj = JsonValue::Object();
    obj["name"] = JsonValue(t.name);
    return obj;
}

static JsonValue SerializeMeshRenderer(const MeshRenderer& mr) {
    JsonValue obj = JsonValue::Object();
    std::string meshPath;
    if (mr.mesh) {
        AssetHandle handle = AssetManager::Get().FindAssetByData(const_cast<Mesh*>(mr.mesh));
        if (handle.IsValid()) {
            const char* path = AssetManager::Get().GetAssetPath(handle);
            if (path) meshPath = path;
        }
    }
    obj["mesh"] = JsonValue(meshPath);
    obj["color"] = ColorToJson(mr.color);
    obj["visible"] = JsonValue(mr.visible);
    obj["shininess"] = JsonValue(mr.shininess);
    const char* texPath = mr.diffuseTexture.IsValid()
        ? AssetManager::Get().GetAssetPath(mr.diffuseTexture)
        : "";
    obj["diffuseTexture"] = JsonValue(texPath ? texPath : "");
    return obj;
}

static JsonValue SerializeRigidBody(const RigidBody& rb) {
    JsonValue obj = JsonValue::Object();
    obj["mass"] = JsonValue(rb.mass);
    obj["velocity"] = Vec3ToJson(rb.velocity);
    obj["acceleration"] = Vec3ToJson(rb.acceleration);
    obj["gravity"] = Vec3ToJson(rb.gravity);
    obj["damping"] = JsonValue(rb.damping);
    obj["isStatic"] = JsonValue(rb.isStatic);
    return obj;
}

static JsonValue SerializeECSCamera(const ECSCamera& cam) {
    JsonValue obj = JsonValue::Object();
    obj["fov"] = JsonValue(cam.fov);
    obj["nearPlane"] = JsonValue(cam.nearPlane);
    obj["farPlane"] = JsonValue(cam.farPlane);
    obj["aspectRatio"] = JsonValue(cam.aspectRatio);
    obj["isActive"] = JsonValue(cam.isActive);
    obj["isOrthographic"] = JsonValue(cam.isOrthographic);
    return obj;
}

static JsonValue SerializeLight(const Light& l) {
    JsonValue obj = JsonValue::Object();
    obj["type"] = JsonValue(static_cast<int>(l.type));
    JsonValue col = JsonValue::Array();
    col.arrVal.push_back(JsonValue(l.color.x));
    col.arrVal.push_back(JsonValue(l.color.y));
    col.arrVal.push_back(JsonValue(l.color.z));
    obj["color"] = col;
    obj["intensity"] = JsonValue(l.intensity);
    obj["range"] = JsonValue(l.range);
    obj["spotAngle"] = JsonValue(l.spotAngle);
    return obj;
}

static JsonValue SerializeAudioSource(const AudioSource& as) {
    JsonValue obj = JsonValue::Object();
    if (as.clip)
        obj["clipPath"] = JsonValue(as.clip->GetFilePath());
    obj["volume"] = JsonValue(as.volume);
    obj["pitch"] = JsonValue(as.pitch);
    obj["state"] = JsonValue(static_cast<int>(as.state));
    obj["isLooping"] = JsonValue(as.isLooping);
    obj["is3D"] = JsonValue(as.is3D);
    obj["minDistance"] = JsonValue(as.minDistance);
    obj["maxDistance"] = JsonValue(as.maxDistance);
    return obj;
}

static JsonValue SerializeScriptComponent(const ScriptComponent& sc) {
    JsonValue obj = JsonValue::Object();
    obj["scriptPath"] = JsonValue(sc.scriptPath);
    obj["language"] = JsonValue(static_cast<int>(sc.language));
    obj["autoStart"] = JsonValue(sc.autoStart);
    return obj;
}

static JsonValue SerializeHitPoints(const HitPoints& hp) {
    JsonValue obj = JsonValue::Object();
    obj["current"] = JsonValue(hp.current);
    obj["maximum"] = JsonValue(hp.maximum);
    obj["temporary"] = JsonValue(hp.temporary);
    return obj;
}

static JsonValue SerializeArmorClass(const ArmorClass& ac) {
    JsonValue obj = JsonValue::Object();
    obj["base"] = JsonValue(ac.base);
    JsonValue bonuses = JsonValue::Array();
    for (i32 bonus : ac.bonuses) bonuses.arrVal.push_back(JsonValue(bonus));
    obj["bonuses"] = bonuses;
    return obj;
}

static JsonValue SerializeInitiative(const Initiative& init) {
    JsonValue obj = JsonValue::Object();
    obj["value"] = JsonValue(init.value);
    obj["hasAdvantage"] = JsonValue(init.hasAdvantage);
    return obj;
}

static JsonValue SerializeSpeed(const Speed& s) {
    JsonValue obj = JsonValue::Object();
    obj["walk"] = JsonValue(s.walk);
    obj["fly"] = JsonValue(s.fly);
    obj["swim"] = JsonValue(s.swim);
    obj["climb"] = JsonValue(s.climb);
    return obj;
}

// ==========================================
// Deserialization helpers
// ==========================================

template<typename T>
static void DeserializeComponent(ComponentRegistry& cr, Entity entity, const JsonValue& components, const char* key, T (*fromJson)(const JsonValue&)) {
    const JsonValue* comp = components.Get(key);
    if (comp) {
        cr.AddComponent<T>(entity, fromJson(*comp));
    }
}

static Transform DeserializeTransform(const JsonValue& j) {
    Transform t;
    if (j.Has("position")) t.position = Vec3FromJson(j["position"]);
    if (j.Has("rotation")) t.rotation = QuatFromJson(j["rotation"]);
    if (j.Has("scale")) t.scale = Vec3FromJson(j["scale"]);
    // parent is set in a second pass
    return t;
}

static Tag DeserializeTag(const JsonValue& j) {
    std::string name = j.GetString("name", "Unnamed");
    return Tag(name.c_str());
}

static MeshRenderer DeserializeMeshRenderer(const JsonValue& j) {
    MeshRenderer mr;
    std::string meshPath = j.GetString("mesh", "");
    if (!meshPath.empty()) {
        AssetHandle h = AssetManager::Get().LoadMesh(meshPath.c_str());
        mr.mesh = AssetManager::Get().GetMesh(h);
    }
    if (j.Has("color")) mr.color = ColorFromJson(j["color"]);
    mr.visible = j.GetBool("visible", true);
    mr.shininess = static_cast<f32>(j.GetNumber("shininess", 32.0f));
    std::string texPath = j.GetString("diffuseTexture", "");
    if (!texPath.empty()) {
        mr.diffuseTexture = AssetManager::Get().LoadTexture(texPath.c_str());
    }
    return mr;
}

static RigidBody DeserializeRigidBody(const JsonValue& j) {
    RigidBody rb;
    rb.mass = static_cast<f32>(j.GetNumber("mass", 1.0f));
    if (j.Has("velocity")) rb.velocity = Vec3FromJson(j["velocity"]);
    if (j.Has("acceleration")) rb.acceleration = Vec3FromJson(j["acceleration"]);
    if (j.Has("gravity")) rb.gravity = Vec3FromJson(j["gravity"]);
    rb.damping = static_cast<f32>(j.GetNumber("damping", 0.0f));
    rb.isStatic = j.GetBool("isStatic", false);
    return rb;
}

static ECSCamera DeserializeECSCamera(const JsonValue& j) {
    ECSCamera cam;
    cam.fov = static_cast<f32>(j.GetNumber("fov", 60.0f));
    cam.nearPlane = static_cast<f32>(j.GetNumber("nearPlane", 0.1f));
    cam.farPlane = static_cast<f32>(j.GetNumber("farPlane", 1000.0f));
    cam.aspectRatio = static_cast<f32>(j.GetNumber("aspectRatio", 16.0f / 9.0f));
    cam.isActive = j.GetBool("isActive", false);
    cam.isOrthographic = j.GetBool("isOrthographic", false);
    return cam;
}

static Light DeserializeLight(const JsonValue& j) {
    Light l;
    l.type = static_cast<LightType>(j.GetInt("type", 0));
    if (j.Has("color")) {
        const JsonValue& col = j["color"];
        if (col.type == JsonType::Array && col.arrVal.size() >= 3) {
            l.color.x = static_cast<f32>(col.arrVal[0].AsNumber());
            l.color.y = static_cast<f32>(col.arrVal[1].AsNumber());
            l.color.z = static_cast<f32>(col.arrVal[2].AsNumber());
        }
    }
    l.intensity = static_cast<f32>(j.GetNumber("intensity", 1.0f));
    l.range = static_cast<f32>(j.GetNumber("range", 50.0f));
    l.spotAngle = static_cast<f32>(j.GetNumber("spotAngle", 45.0f));
    return l;
}

static AudioSource DeserializeAudioSource(const JsonValue& j) {
    AudioSource as;
    as.volume = j.GetNumber("volume", 1.0);
    as.pitch = j.GetNumber("pitch", 1.0);
    as.state = static_cast<AudioSourceState>(j.GetInt("state", 0));
    as.isLooping = j.GetBool("isLooping", false);
    as.is3D = j.GetBool("is3D", false);
    as.minDistance = j.GetNumber("minDistance", 1.0);
    as.maxDistance = j.GetNumber("maxDistance", 50.0);
    // clip is loaded separately; stored path not resolved here
    return as;
}

static ScriptComponent DeserializeScriptComponent(const JsonValue& j) {
    ScriptComponent sc;
    std::string path = j.GetString("scriptPath", "");
    if (path.size() < sizeof(sc.scriptPath)) {
        std::memcpy(sc.scriptPath, path.c_str(), path.size() + 1);
    }
    sc.language = static_cast<ScriptLanguage>(j.GetInt("language", 0));
    sc.autoStart = j.GetBool("autoStart", false);
    return sc;
}

static HitPoints DeserializeHitPoints(const JsonValue& j) {
    i32 cur = j.GetInt("current", 10);
    i32 max = j.GetInt("maximum", 10);
    i32 tmp = j.GetInt("temporary", 0);
    return HitPoints(cur, max, tmp);
}

static ArmorClass DeserializeArmorClass(const JsonValue& j) {
    ArmorClass ac;
    ac.base = j.GetInt("base", 10);
    const JsonValue* bonuses = j.Get("bonuses");
    if (bonuses && bonuses->type == JsonType::Array) {
        for (const auto& b : bonuses->arrVal) {
            ac.bonuses.push_back(b.AsInt());
        }
    }
    return ac;
}

static Initiative DeserializeInitiative(const JsonValue& j) {
    i32 val = j.GetInt("value", 0);
    bool adv = j.GetBool("hasAdvantage", false);
    return Initiative(val, adv);
}

static Speed DeserializeSpeed(const JsonValue& j) {
    Speed s;
    s.walk = static_cast<f32>(j.GetNumber("walk", 30.0f));
    s.fly = static_cast<f32>(j.GetNumber("fly", 0.0f));
    s.swim = static_cast<f32>(j.GetNumber("swim", 0.0f));
    s.climb = static_cast<f32>(j.GetNumber("climb", 0.0f));
    return s;
}

// ==========================================
// Save
// ==========================================

bool SceneSerializer::Save(Scene& scene, const char* path) {
    EntityManager& em = scene.GetEntityManager();
    ComponentRegistry& cr = scene.GetComponentRegistry();

    JsonValue root = JsonValue::Object();
    root["name"] = JsonValue(scene.GetName());

    // Collect all entity indices that have any component
    struct EntityInfo {
        u32 entityIdx;
        u16 generation;
    };
    std::vector<EntityInfo> aliveEntities;

    em.ForEachAlive([&](Entity e) {
        EntityInfo info;
        info.entityIdx = Entity::GetIndex(e.id);
        info.generation = Entity::GetGeneration(e.id);
        aliveEntities.push_back(info);
    });

    JsonValue entitiesArr = JsonValue::Array();

    for (const auto& info : aliveEntities) {
        Entity e(Entity::EncodeId(info.entityIdx, info.generation));

        JsonValue entityObj = JsonValue::Object();
        entityObj["id"] = JsonValue(static_cast<int>(info.entityIdx));
        entityObj["generation"] = JsonValue(static_cast<int>(info.generation));

        // Tag name for readability
        Tag* tag = cr.GetComponent<Tag>(e);
        if (tag && tag->name) {
            entityObj["name"] = JsonValue(tag->name);
        } else {
            entityObj["name"] = JsonValue("");
        }

        JsonValue components = JsonValue::Object();

        Transform* t = cr.GetComponent<Transform>(e);
        if (t) components["Transform"] = SerializeTransform(*t);

        if (tag) components["Tag"] = SerializeTag(*tag);

        MeshRenderer* mr = cr.GetComponent<MeshRenderer>(e);
        if (mr) components["MeshRenderer"] = SerializeMeshRenderer(*mr);

        RigidBody* rb = cr.GetComponent<RigidBody>(e);
        if (rb) components["RigidBody"] = SerializeRigidBody(*rb);

        ECSCamera* cam = cr.GetComponent<ECSCamera>(e);
        if (cam) components["ECSCamera"] = SerializeECSCamera(*cam);

        Light* l = cr.GetComponent<Light>(e);
        if (l) components["Light"] = SerializeLight(*l);

        AudioSource* as = cr.GetComponent<AudioSource>(e);
        if (as) components["AudioSource"] = SerializeAudioSource(*as);

        ScriptComponent* sc = cr.GetComponent<ScriptComponent>(e);
        if (sc) components["ScriptComponent"] = SerializeScriptComponent(*sc);

        // Game stat components
        HitPoints* hp = cr.GetComponent<HitPoints>(e);
        if (hp) components["HitPoints"] = SerializeHitPoints(*hp);

        ArmorClass* ac = cr.GetComponent<ArmorClass>(e);
        if (ac) components["ArmorClass"] = SerializeArmorClass(*ac);

        Initiative* init = cr.GetComponent<Initiative>(e);
        if (init) components["Initiative"] = SerializeInitiative(*init);

        Speed* speed = cr.GetComponent<Speed>(e);
        if (speed) components["Speed"] = SerializeSpeed(*speed);

        entityObj["components"] = components;
        entitiesArr.arrVal.push_back(entityObj);
    }

    root["entities"] = entitiesArr;

    // Write to file
    FILE* f = fopen(path, "w");
    if (!f) {
        DND_LOG_ERROR("SceneSerializer::Save: could not open '%s' for writing", path);
        return false;
    }

    std::string json = JsonWriter::Write(root, true);
    fwrite(json.c_str(), 1, json.size(), f);
    fclose(f);

    DND_LOG_INFO("SceneSerializer::Save: saved %zu entities to '%s'",
                 aliveEntities.size(), path);
    return true;
}

// ==========================================
// Load
// ==========================================

bool SceneSerializer::Load(Scene& scene, const char* path) {
    JsonValue root = JsonParser::ParseFile(path);
    if (root.IsNull()) {
        DND_LOG_ERROR("SceneSerializer::Load: failed to parse '%s'", path);
        return false;
    }

    JsonValue* entitiesVal = root.Get("entities");
    if (!entitiesVal || entitiesVal->type != JsonType::Array) {
        DND_LOG_ERROR("SceneSerializer::Load: no entities array in '%s'", path);
        return false;
    }

    // Clear existing entities and components
    scene.GetEntityManager().Clear();
    scene.GetComponentRegistry().Clear();

    EntityManager& em = scene.GetEntityManager();
    ComponentRegistry& cr = scene.GetComponentRegistry();

    // Pass 1: Create all entities and build old-index -> new-Entity mapping
    std::unordered_map<u32, Entity> indexToEntity;
    std::vector<JsonValue*> entityList;
    std::vector<u32> originalIndices;

    for (auto& entityVal : entitiesVal->arrVal) {
        if (entityVal.type != JsonType::Object) continue;
        u32 origIdx = static_cast<u32>(entityVal.GetInt("id", 0));
        Entity e = em.Create();
        indexToEntity[origIdx] = e;
        entityList.push_back(&entityVal);
        originalIndices.push_back(origIdx);
    }

    // Pass 2: Add components with resolved parent references
    std::vector<std::pair<Entity, Entity>> parentAssignments; // (child, parent or None)

    for (size_t i = 0; i < entityList.size(); i++) {
        const JsonValue& entityVal = *entityList[i];
        u32 origIdx = originalIndices[i];
        Entity e = indexToEntity[origIdx];

        const JsonValue* nameVal = entityVal.Get("name");
        std::string tagName = nameVal ? nameVal->AsString("") : "";
        Tag tag(tagName.c_str());
        cr.AddComponent<Tag>(e, tag);

        const JsonValue* comps = entityVal.Get("components");
        if (!comps) continue;

        // Transform
        const JsonValue* t = comps->Get("Transform");
        if (t) {
            Transform tf = DeserializeTransform(*t);
            int parentIdx = t->GetInt("parent", -1);
            if (parentIdx >= 0) {
                auto it = indexToEntity.find(static_cast<u32>(parentIdx));
                if (it != indexToEntity.end()) {
                    tf.parent = it->second;
                }
            }
            cr.AddComponent<Transform>(e, tf);
        }

        // MeshRenderer
        const JsonValue* mr = comps->Get("MeshRenderer");
        if (mr) {
            cr.AddComponent<MeshRenderer>(e, DeserializeMeshRenderer(*mr));
        }

        // RigidBody
        const JsonValue* rb = comps->Get("RigidBody");
        if (rb) {
            cr.AddComponent<RigidBody>(e, DeserializeRigidBody(*rb));
        }

        // ECSCamera
        const JsonValue* cam = comps->Get("ECSCamera");
        if (cam) {
            cr.AddComponent<ECSCamera>(e, DeserializeECSCamera(*cam));
        }

        // Light
        const JsonValue* l = comps->Get("Light");
        if (l) {
            cr.AddComponent<Light>(e, DeserializeLight(*l));
        }

        // AudioSource
        const JsonValue* as = comps->Get("AudioSource");
        if (as) {
            cr.AddComponent<AudioSource>(e, DeserializeAudioSource(*as));
        }

        // ScriptComponent
        const JsonValue* sc = comps->Get("ScriptComponent");
        if (sc) {
            cr.AddComponent<ScriptComponent>(e, DeserializeScriptComponent(*sc));
        }

        // Game stat components
        const JsonValue* hp = comps->Get("HitPoints");
        if (hp) {
            cr.AddComponent<HitPoints>(e, DeserializeHitPoints(*hp));
        }

        const JsonValue* ac = comps->Get("ArmorClass");
        if (ac) {
            cr.AddComponent<ArmorClass>(e, DeserializeArmorClass(*ac));
        }

        const JsonValue* init = comps->Get("Initiative");
        if (init) {
            cr.AddComponent<Initiative>(e, DeserializeInitiative(*init));
        }

        const JsonValue* speed = comps->Get("Speed");
        if (speed) {
            cr.AddComponent<Speed>(e, DeserializeSpeed(*speed));
        }
    }

    DND_LOG_INFO("SceneSerializer::Load: loaded %zu entities from '%s'",
                 entityList.size(), path);
    return true;
}

} // namespace Dnd
