// Script natives 0x5C2C20..0x5C4150: objects (create, delete, area tests, pickups in
// triggers, vulnerability) and the first prop-anim ones (path following, health bars,
// sockets). Each is `int __cdecl Native_X(lua_State* L)`, returning how many results
// it pushed; the helper 0x5C35E0 keeps its own convention.

#include <cstdint>

#include "game/NativesMPP.h"

namespace {

GAME_FN(void, __cdecl, DeleteObjectByHandle, 0x005C2A10, uint16_t handle);
GAME_FN(uint8_t, __cdecl, IsEntityInArea, 0x005C2A70, int is3D, int handle, int kind, float x, float y, float z,
        float rx, float ry, float rz, int a10, void* rotation, int a12, int a13, int a14, int a15, int a16);
GAME_FN(float*, __cdecl, GetAreaEntityPos, 0x005C2970, float* out, int handle, int kind);
GAME_FN(uint8_t, __cdecl, FindObjectsInArea, 0x005C25A0, void* list, float x, float y, float z, float rx, float ry,
        float rz);
GAME_FN(int, __cdecl, CreateObjectAt, 0x005C35E0, lua_State* L, int model, float x, float y, float z);

GAME_FN(void, __thiscall, Script_AddObject, 0x005D97D0, void* script, int handle);
GAME_FN(void, __thiscall, Script_RemoveObject, 0x005D97F0, void* script, int handle);
GAME_FN(uint8_t*, __thiscall, ObjectPool_GetAtRef, 0x005C1470, void* pool, int handle);
GAME_FN(uint8_t*, __cdecl, O_GetVehicle, 0x0044A580, int handle);
GAME_FN(int, __thiscall, O_Pool_ChainSize, 0x00435BB0, void* pool);
GAME_FN(uint8_t*, __thiscall, O_Pool_ChainAt, 0x00435BD0, void* pool, int slot, int);
GAME_FN(uint8_t, __thiscall, Trigger_ContainsPoint, 0x006D6FA0, void* trigger, const float* point);
GAME_FN(void, __thiscall, Butes_Resolve, 0x007282A0, void* butes);
GAME_FN(void, __thiscall, ProjectileData_Resolve, 0x00728430, void* data);
GAME_FN(int, __thiscall, ProjectileImpacts_Count, 0x00561060, void* impacts, int type, void* trigger);
inline void* const projectileImpacts = reinterpret_cast<void*>(0x00CF1F60);
GAME_FN(void, __thiscall, Pickup_Collect, 0x00444A10, void* pickup, int);
GAME_FN(void, __cdecl, O_World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, O_World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __cdecl, O_Entity_45E690, 0x0045E690, void* entity);
GAME_FN(int, __cdecl, O_GetModelIndex, 0x0051C330, const char* name);
GAME_FN(uint8_t*, __cdecl, GetObjectInfo, 0x004D0CA0, int model);
GAME_FN(void, __thiscall, CopyInt, 0x005FA880, uint32_t* dst, const void* src);
GAME_FN(uint32_t*, __thiscall, O_NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, ButesPath_Construct, 0x005FA8C0, void* path, int count, const uint32_t* parts);
GAME_FN(uint32_t, __cdecl, Config_GetHash, 0x0072AEA0, const void* path, const void* type);
GAME_FN(void*, __cdecl, O_GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, O_RequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __thiscall, Script_AddModel, 0x005D9450, void* script, int id);
GAME_FN(void, __cdecl, O_LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);
GAME_FN(void, __cdecl, O_LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void*, __cdecl, O_Object_New, 0x004CFCD0, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, Object_Construct, 0x004D0580, void* object, int model, int);
GAME_VAR(float, g_groundZThreshold, 0x00A14A8C);   // -20: at or below it, the ground's
GAME_FN(float, __cdecl, FindGroundZ2D, 0x004634A0, float x, float y, int);
GAME_FN(float, __thiscall, Entity_DistanceToBase, 0x00466110, void* entity);
GAME_FN(void, __thiscall, O_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Entity_SetOrientation, 0x0046A690, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, O_Entity_443AA0, 0x00443AA0, void* entity);
GAME_FN(void, __thiscall, O_Entity_465760, 0x00465760, void* entity);
GAME_FN(void, __thiscall, SetupBigBuilding, 0x00467BC0, void* entity);
GAME_FN(void*, __thiscall, Routes_Get, 0x006D6450, void* routes, int index);
GAME_FN(float*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(float*, __thiscall, Route_Next, 0x006D59F0, void* route);
inline void* const o_routes = reinterpret_cast<void*>(0x00BC5E7C);
GAME_FN(void, __cdecl, O_Free, 0x005EEAB0, void* p);

// prop anims
GAME_FN(uint8_t*, __thiscall, PropAnims_Get, 0x006D7B10, void* wrapper, int id);
GAME_FN(uint8_t*, __cdecl, O_GetObject, 0x0044A5B0, int handle);
GAME_FN(uint8_t*, __cdecl, O_GetPropAnim, 0x0044A5E0, int handle);
GAME_FN(uint8_t*, __cdecl, GetEntityOfKind, 0x0044A660, int handle, int kind);
GAME_VAR(uint8_t*, g_healthBar, 0x00C674D0);
GAME_FN(void, __thiscall, HealthBar_SetTarget, 0x005538C0, void* bar, void* target);
GAME_FN(void, __thiscall, HealthBar_SetName, 0x005514D0, void* bar, const char* name);
GAME_FN(void, __thiscall, PropAnim_PlayAnim, 0x004EA450, void* prop, int anim, int);
GAME_VAR(void*, g_patrolPathPool, 0x00D13CB4);
GAME_FN(void*, __thiscall, PatrolPaths_Get, 0x005ED4F0, void* pool, int id);
inline void* const pathFollowers = reinterpret_cast<void*>(0x00D13730);
GAME_FN(void, __thiscall, PathFollowers_Start, 0x005EC130, void* self, void* entity, void* path, int flag);
GAME_FN(uint8_t, __thiscall, PathFollowers_Has, 0x005EBC70, void* self, void* entity);
GAME_FN(void*, __thiscall, PathFollowers_Callback, 0x005EBA30, void* self, void* entity);
GAME_FN(void, __thiscall, PathFollowers_Reset, 0x005EBCB0, void* self, void* entity);
GAME_FN(void, __thiscall, PathFollowers_SetSpeed, 0x005EBD50, void* self, void* entity, float speed);
GAME_FN(float, __thiscall, PathFollowers_GetSpeed, 0x005EBD00, void* self, void* entity);
GAME_FN(void, __thiscall, PathFollowers_Stop, 0x005EC0E0, void* self, void* entity);
GAME_FN(void, __thiscall, PathFollowers_SetSoundLoop, 0x005EBE70, void* self, void* entity, const char* name);
GAME_FN(uint8_t*, __cdecl, FindNearestObjectOfTypeInRange, 0x004607A0, int model, const float* pos, float range, int);
GAME_FN(uint8_t, __thiscall, Entity_512760, 0x00512760, void* entity);
GAME_FN(void, __cdecl, SpawnDestructionEffects, 0x0046AAF0, void* entity);
GAME_FN(uint32_t, __cdecl, O_HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t, __cdecl, O_GetNodeTransform, 0x00467D80, void* entity, uint32_t node, float* transform, int flag);

void* CurrentScript() {
    return M_Scripts_GetCurrent(m_lurStore);
}

// The pickups (0x48 bytes each, 0xC0A100..0xC0F590)
inline uint8_t* const pickups = reinterpret_cast<uint8_t*>(0x00C0A100);
inline uint8_t* const pickupsEnd = reinterpret_cast<uint8_t*>(0x00C0F590);

// ---- objects

int __cdecl Native_ObjectDelete(lua_State* L) {
    uint16_t handle = uint16_t(Script::GetInt(L, 0));
    DeleteObjectByHandle(handle);
    Script_RemoveObject(CurrentScript(), handle);
    return 0;
}

// (object, x, y, z, radius, marker, [heading])
int __cdecl Native_ObjectIsInAreaXYZ(lua_State* L) {
    uint16_t handle = uint16_t(Script::GetInt(L, 0));
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    float radius = Script::GetFloat(L, 4);
    int marker = Script::GetInt(L, 5);
    float heading = 0.0f;
    if (Script::GetArgCount(L) >= 7) {
        heading = float(double(Script::GetFloat(L, 6)) * 3.1415927410125732 / 180.0);
    }
    float rotation[3] = {0.0f, 0.0f, heading};
    uint8_t in = IsEntityInArea(1, handle, 0, x, y, z, radius, radius, radius, marker, rotation, -1, -1, 0, 0, 1);
    Script::PushBool(L, in);
    return 1;
}

// (object, handle, kind, radius, marker, [a = -1], [flag], [heading]): around an
// entity (a ped's vehicle when in one)
int __cdecl Native_ObjectIsInAreaObject(lua_State* L) {
    int object = Script::GetInt(L, 0);
    uint16_t handle = uint16_t(Script::GetInt(L, 1));
    int kind = Script::GetInt(L, 2);
    float radius = Script::GetFloat(L, 3);
    int marker = Script::GetInt(L, 4);
    uint8_t result = 0;
    int a = -1;
    uint8_t flag = 0;
    if (Script::GetArgCount(L) >= 6) {
        a = Script::GetInt(L, 5);
    }
    if (Script::GetArgCount(L) >= 7) {
        flag = Script::GetBool(L, 6);
    }
    float heading = 0.0f;
    if (Script::GetArgCount(L) >= 8) {
        heading = float(double(Script::GetFloat(L, 7)) * 3.1415927410125732 / 180.0);
    }
    uint8_t* e = nullptr;
    bool found = true;
    switch (kind) {
    case 0:
        e = ObjectPool_GetAtRef(m_objectPool, handle);
        break;
    case 1:
        e = O_GetVehicle(handle);
        break;
    case 2:
    case 3:
        e = kind == 2 ? M_GetPedFromHandle(handle, 2) : static_cast<uint8_t*>(m_playerPed);
        if (e == nullptr) {
            found = false;
            break;
        }
        if (At<uint8_t*>(e, 0x1554) != nullptr) {
            e = At<uint8_t*>(e, 0x1554);
        }
        break;
    default:
        found = false;
        break;
    }
    if (found && e != nullptr) {
        float pos[3];
        GetAreaEntityPos(pos, handle, kind);
        float rotation[3] = {0.0f, 0.0f, heading};
        result = IsEntityInArea(1, object, 0, pos[0], pos[1], pos[2], radius, radius, radius, marker, rotation, a, -1,
                                flag, 0, 1);
    }
    Script::PushBool(L, result);
    return 1;
}

// (model, vulnerable): bit 6 of +0x209 on every object of the model (slots walked down)
int __cdecl Native_ObjectToggleVulnerability(lua_State* L) {
    int model = Script::GetInt(L, 0);
    uint8_t on = Script::GetBool(L, 1);
    CPool<uint8_t>* pool = m_objectPool;
    int i = pool->size;
    if (pool->Next() != nullptr) {
        i += O_Pool_ChainSize(pool->Next());
    }
    while (i != 0) {
        i--;
        uint8_t* object = nullptr;
        if (i < pool->size) {
            object = pool->objects + pool->elemSize * i;
        } else if (pool->Next() != nullptr) {
            object = O_Pool_ChainAt(pool->Next(), i - pool->size, 1);
        }
        if ((pool->flags[i] & 0x80) == 0 && object != nullptr && At<int16_t>(object, 0x10E) == model) {
            uint8_t& flags = At<uint8_t>(object, 0x209);
            flags ^= (uint8_t(on << 6) ^ flags) & 0x40;
            pool = m_objectPool;
        }
    }
    return 0;
}

// (trigger, model): its projectile types' impacts in the trigger (two of them, +0x150
// and +0x152 of the model's projectile data; the "none" test, -1 against a u16, never
// skips)
int __cdecl Native_ObjectNumProjectileImpacts(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    int model = Script::GetInt(L, 1);
    uint8_t* trigger = PoolSlot(m_triggerPool, slot);
    uint8_t* butes = At<uint8_t*>(m_modelInfoPtrs[model], 0x18);
    int count = 0;
    if (butes != nullptr) {
        Butes_Resolve(butes);
        if (At<void*>(butes, 0x80) != nullptr) {
            Butes_Resolve(butes);
            uint8_t* data = At<uint8_t*>(butes, 0x80);
            ProjectileData_Resolve(data);
            uint16_t type = At<uint16_t>(data, 0x150);
            if (int(type) != -1) {
                count = ProjectileImpacts_Count(projectileImpacts, type, trigger);
            }
            Butes_Resolve(butes);
            data = At<uint8_t*>(butes, 0x80);
            ProjectileData_Resolve(data);
            type = At<uint16_t>(data, 0x152);
            if (int(type) != -1) {
                count += ProjectileImpacts_Count(projectileImpacts, type, trigger);
            }
        }
    }
    Script::PushInt(L, count);
    return 1;
}

// (the slot's object, flags unchecked: its virtual +0x78)
int __cdecl Native_ObjectIsDestroyed(lua_State* L) {
    uint8_t destroyed = 0;
    int slot = uint16_t(Script::GetInt(L, 0)) >> 8;
    CPool<uint8_t>* pool = m_objectPool;
    uint8_t* object = nullptr;
    if (slot < pool->size) {
        object = pool->objects + pool->elemSize * slot;
    } else if (pool->Next() != nullptr) {
        object = O_Pool_ChainAt(pool->Next(), slot - pool->size, 1);
    }
    if (object != nullptr) {
        destroyed = VCall<uint8_t>(object, 0x78);
    }
    Script::PushBool(L, destroyed);
    return 1;
}

// Counts the objects and projectiles of the model inside the trigger
int CountInTrigger(CPool<uint8_t>* (*getPool)(), int model, uint8_t* trigger) {
    CPool<uint8_t>* pool = getPool();
    int n = pool->size;
    if (pool->Next() != nullptr) {
        n += O_Pool_ChainSize(pool->Next());
    }
    int count = 0;
    for (int i = 0; i < n; i++) {
        uint8_t* object = nullptr;
        if (i < pool->size) {
            object = pool->objects + pool->elemSize * i;
        } else if (pool->Next() != nullptr) {
            object = O_Pool_ChainAt(pool->Next(), i - pool->size, 1);
        }
        if ((pool->flags[i] & 0x80) == 0 && object != nullptr && At<int16_t>(object, 0x10E) == model) {
            if (Trigger_ContainsPoint(trigger, EntityPos(object)) != 0) {
                count++;
            }
            pool = getPool();
        }
    }
    return count;
}

// (model, trigger) -> how many
int __cdecl Native_ObjectTypeIsInTrigger(lua_State* L) {
    int model = Script::GetInt(L, 0);
    uint8_t* trigger = PoolSlot(m_triggerPool, Script::GetInt(L, 1));
    int count = CountInTrigger([]() -> CPool<uint8_t>* { return m_objectPool; }, model, trigger);
    count += CountInTrigger([]() -> CPool<uint8_t>* { return m_projectilePool; }, model, trigger);
    Script::PushInt(L, count);
    return 1;
}

// (trigger, [model]): collects the pickups (of the model) in it and deletes the
// projectiles (of the model) in it. (The projectile count is the pool's ChainSize, taken
// again each step; at most 256 are deleted, unchecked)
int __cdecl Native_ObjectRemovePickupsInTrigger(lua_State* L) {
    uint8_t* trigger = PoolSlot(m_triggerPool, Script::GetInt(L, 0));
    int found = 0;
    int model = -1;
    if (lua::gettop(L) > 1) {
        model = Script::GetInt(L, 1);
    }
    for (uint8_t* p = pickups; p < pickupsEnd; p += 0x48) {
        if ((At<uint8_t>(p, 0x46) & 8) != 0 && (model == -1 || model == At<int32_t>(p, 0x30))
            && Trigger_ContainsPoint(trigger, reinterpret_cast<float*>(p + 8)) != 0) {
            Pickup_Collect(p, 1);
        }
    }
    uint8_t* toDelete[256];
    CPool<uint8_t>* pool = m_projectilePool;
    for (int i = 0; i < O_Pool_ChainSize(pool); i++) {
        uint8_t* object = nullptr;
        if (i < pool->size) {
            object = pool->objects + pool->elemSize * i;
        } else if (pool->Next() != nullptr) {
            object = O_Pool_ChainAt(pool->Next(), i - pool->size, 1);
        }
        if ((pool->flags[i] & 0x80) == 0 && object != nullptr
            && (model == -1 || model == At<int16_t>(object, 0x10E))) {
            if (Trigger_ContainsPoint(trigger, EntityPos(object)) != 0) {
                toDelete[found] = object;
                found++;
            }
        }
    }
    for (int i = 0; i < found; i++) {
        uint8_t* object = toDelete[i];
        O_World_Remove(object);
        O_Entity_45E690(object);
        if (object != nullptr) {
            VCall<void>(object, 0, 1);
        }
    }
    return 0;
}

// (model name, list name): the object info's pickup list (+0x2C) from the config path
// (0x1B653A0, 0x1B6538C, name)
int __cdecl Native_ObjectTypeSetPickupListOverride(lua_State* L) {
    uint8_t* info = GetObjectInfo(O_GetModelIndex(Script::GetString(L, 0)));
    uint32_t parts[3];
    CopyInt(&parts[0], reinterpret_cast<void*>(0x01B653A0));
    CopyInt(&parts[1], reinterpret_cast<void*>(0x01B6538C));
    O_NameKey_Construct(&parts[2], Script::GetString(L, 1));
    uint8_t path[8];
    ButesPath_Construct(path, 3, parts);
    At<uint32_t>(info, 0x2C) = Config_GetHash(path, reinterpret_cast<void*>(0x01B65390));
    return 0;
}

// 0x5C35E0: creates an object of the model at (x, y, z) (loading the model first; z at
// or below -20 is on the ground), added to the world; its handle
int __cdecl CreateObjectAtImpl(lua_State*, int modelArg, float x, float y, float z) {
    int model = uint16_t(modelArg);
    if (At<uint8_t>(O_GetStreamingInfo(model), 4) != 1) {
        O_RequestModel(model, 0x16);
        Script_AddModel(CurrentScript(), model);
        O_LoadingScreen("CreateObject()", "CStreaming::LoadAllRequestedModels()", 0);
        O_LoadAllRequestedModels(0);
    }
    uint8_t* mi = m_modelInfoPtrs[model];
    void* memory = O_Object_New(0x22C);
    uint8_t* object = memory != nullptr ? Object_Construct(memory, model, 0) : nullptr;
    At<uint8_t>(object, 0x208) = 2;
    float groundZ = z;
    if (g_groundZThreshold >= z) {
        groundZ = FindGroundZ2D(x, y, 1);
    }
    O_SetPosition(object, x, y, Entity_DistanceToBase(object) + groundZ);
    float angle = float(3.1415927410125732 * 0.0 / 180.0);
    Entity_SetOrientation(object, angle, angle, angle);
    O_Entity_443AA0(object);
    O_Entity_465760(object);
    uint8_t type = At<uint8_t>(mi, 8);
    if ((type == 1 || type == 3) && (At<uint8_t>(mi, 0x28) & 0x10) != 0) {
        SetupBigBuilding(object);
    }
    O_World_Add(object);
    return PoolHandle(m_objectPool, object);
}

// (model name or index, x, y, z) -> its handle (the script's)
int __cdecl Native_ObjectCreateXYZ(lua_State* L) {
    int model;
    if (Script::IsString(L, 0) != 0) {
        model = O_GetModelIndex(Script::GetString(L, 0));
    } else {
        model = Script::GetInt(L, 0);
    }
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    uint16_t handle = uint16_t(CreateObjectAt(L, model, x, y, z));
    Script_AddObject(CurrentScript(), handle);
    Script::PushInt(L, handle);
    return 1;
}

// (model, route, [point number]): at the route's first point, or its n-th
int __cdecl Native_ObjectCreatePoint(lua_State* L) {
    bool numbered = false;
    int n = 0;   // (uninitialised in the game when not given; unused then)
    if (lua::gettop(L) == 3) {
        numbered = true;
        n = Script::GetInt(L, 2);
    }
    void* route = Routes_Get(o_routes, uint16_t(Script::GetInt(L, 1)));
    float* p = Route_First(route);
    if (numbered) {
        for (int i = 1; i < n;) {
            float* next = Route_Next(route);
            if (next == nullptr) {
                break;
            }
            i++;
            p = next;
        }
    }
    float pos[3] = {p[0], p[1], p[2]};
    int model = uint16_t(Script::GetInt(L, 0));
    uint16_t handle = uint16_t(CreateObjectAt(L, model, pos[0], pos[1], pos[2]));
    Script_AddObject(CurrentScript(), handle);
    Script::PushInt(L, handle);
    return 1;
}

// (x, y, z, radius) -> (found, handles...)
int __cdecl Native_ObjectFindInArea(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    float radius = Script::GetFloat(L, 3);
    struct {
        re::Ptr32<uint32_t> data;
        uint32_t capacity;
        uint32_t count;
    } list = {nullptr, 0, 0};
    uint8_t found = FindObjectsInArea(&list, x, y, z, radius, radius, radius);
    Script::PushBool(L, found);
    uint32_t count = list.count;
    uint32_t* data = list.data;
    for (uint32_t i = 0; i < count; i++) {
        Script::PushInt(L, int(data[i]));
    }
    if (data != nullptr) {
        O_Free(data);
    }
    return int(count) + 1;
}

// ---- prop anims

// (prop) -> who destroyed it (+0x238), or (handle, kind): an object's (+0x224) or a
// prop anim's
int __cdecl Native_PAnimDestroyedByPed(lua_State* L) {
    int who = 0;
    if (Script::GetArgCount(L) == 1) {
        uint8_t* prop = PropAnims_Get(m_triggerPoolWrapper, Script::GetInt(L, 0));
        if (prop != nullptr) {
            Script::PushInt(L, At<int32_t>(prop, 0x238));
            return 1;
        }
    } else {
        int handle = Script::GetInt(L, 0);
        int kind = Script::GetInt(L, 1);
        if (kind == 0) {
            uint8_t* object = O_GetObject(handle);
            if (object != nullptr) {
                Script::PushInt(L, At<int32_t>(object, 0x224));
                return 1;
            }
        } else if (kind == 5) {
            uint8_t* prop = O_GetPropAnim(handle);
            if (prop != nullptr) {
                who = At<int32_t>(prop, 0x238);
            }
        }
    }
    Script::PushInt(L, who);
    return 1;
}

// ([prop]): the health bar hidden (the prop's anim 3 played)
int __cdecl Native_PAnimHideHealthBar(lua_State* L) {
    uint8_t* bar = g_healthBar;
    HealthBar_SetTarget(bar, nullptr);
    VCall<void>(bar, 0x14, 0);
    if (lua::gettop(L) == 1) {
        uint8_t* prop = PropAnims_Get(m_triggerPoolWrapper, Script::GetInt(L, 0));
        if (prop != nullptr) {
            PropAnim_PlayAnim(prop, 3, 0);
        }
    }
    return 0;
}

// (prop | handle, kind, path, flag, [callback])
int __cdecl Native_PAnimFollowPath(lua_State* L) {
    Script::IsBoolean(L, 2);
    int handle = Script::GetInt(L, 0);
    int i = 1;
    uint8_t* e;
    if (Script::IsNumber(L, 2) != 0) {
        int kind = Script::GetInt(L, 1);
        i = 2;
        e = GetEntityOfKind(handle, kind);
    } else {
        e = PropAnims_Get(m_triggerPoolWrapper, handle);
    }
    if (e != nullptr) {
        void* path = PatrolPaths_Get(g_patrolPathPool, Script::GetInt(L, i));
        i++;
        uint8_t flag = Script::GetBool(L, i);
        i++;
        PathFollowers_Start(pathFollowers, e, path, flag);
        if (lua::gettop(L) > i && PathFollowers_Has(pathFollowers, e) != 0) {
            Script::StoreFunctionRef(L, i, PathFollowers_Callback(pathFollowers, e));
        }
    }
    return 0;
}

// (prop | handle, kind). (With two arguments, a missing entity isn't checked)
int __cdecl Native_PAnimFollowPathReset(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    if (lua::gettop(L) == 2) {
        PathFollowers_Reset(pathFollowers, GetEntityOfKind(handle, Script::GetInt(L, 1)));
        return 0;
    }
    uint8_t* prop = PropAnims_Get(m_triggerPoolWrapper, handle);
    if (prop != nullptr) {
        PathFollowers_Reset(pathFollowers, prop);
    }
    return 0;
}

int __cdecl Native_PAnimSetPathFollowSpeed(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int i = 1;
    uint8_t* e;
    if (lua::gettop(L) == 3) {
        int kind = Script::GetInt(L, 1);
        i = 2;
        e = GetEntityOfKind(handle, kind);
    } else {
        e = PropAnims_Get(m_triggerPoolWrapper, handle);
    }
    if (e != nullptr) {
        PathFollowers_SetSpeed(pathFollowers, e, Script::GetFloat(L, i));
    }
    return 0;
}

int __cdecl Native_PAnimGetPathFollowSpeed(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* e;
    if (lua::gettop(L) == 2) {
        e = GetEntityOfKind(handle, Script::GetInt(L, 1));
    } else {
        e = PropAnims_Get(m_triggerPoolWrapper, handle);
        if (e == nullptr) {
            return 0;
        }
    }
    Script::PushFloat(L, PathFollowers_GetSpeed(pathFollowers, e));
    return 1;
}

int __cdecl Native_PAnimStopFollowPath(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    if (lua::gettop(L) == 2) {
        PathFollowers_Stop(pathFollowers, GetEntityOfKind(handle, Script::GetInt(L, 1)));
        return 0;
    }
    uint8_t* prop = PropAnims_Get(m_triggerPoolWrapper, handle);
    if (prop != nullptr) {
        PathFollowers_Stop(pathFollowers, prop);
    }
    return 0;
}

// (model name or index, x, y, z): the nearest such object within 1 (a light) broken
int __cdecl Native_PAnimBreakLight(lua_State* L) {
    int model;
    if (Script::IsString(L, 0) != 0) {
        model = O_GetModelIndex(Script::GetString(L, 0));
    } else {
        model = Script::GetInt(L, 0);
    }
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    if (model != -1) {
        float pos[3] = {x, y, z};
        uint8_t* light = FindNearestObjectOfTypeInRange(model, pos, 1.0f, 1);
        if (Entity_512760(light) != 0 && At<int32_t>(light, 0xCC) == 0) {
            SpawnDestructionEffects(light);
        }
    }
    return 0;
}

// (prop | handle, kind, sound name, [off])
int __cdecl Native_PAnimFollowPathSoundLoop(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int i = 1;
    uint8_t* e;
    if (Script::GetArgCount(L) >= 2 && Script::IsNumber(L, 0) != 0 && Script::IsNumber(L, 1) != 0) {
        int kind = Script::GetInt(L, 1);
        i = 2;
        e = GetEntityOfKind(handle, kind);
    } else {
        e = PropAnims_Get(m_triggerPoolWrapper, handle);
    }
    if (e != nullptr) {
        const char* name = Script::GetString(L, i);
        i++;
        if (Script::GetArgCount(L) == i + 1 && Script::GetBool(L, i) != 0) {
            name = nullptr;
        }
        PathFollowers_SetSoundLoop(pathFollowers, e, name);
    }
    return 0;
}

// (prop, show, [name], [flag]): hidden plays the prop's anim 3
int __cdecl Native_PAnimShowHealthBar(lua_State* L) {
    uint8_t* prop = PropAnims_Get(m_triggerPoolWrapper, Script::GetInt(L, 0));
    if (prop == nullptr) {
        return 0;
    }
    if (Script::GetBool(L, 1) == 0) {
        PropAnim_PlayAnim(prop, 3, 1);
        return 0;
    }
    uint8_t* bar = g_healthBar;
    HealthBar_SetTarget(bar, prop);
    VCall<void>(bar, 0x14, 1);
    const char* name = nullptr;
    uint8_t flag = 0;
    int n = lua::gettop(L);
    if (n >= 3) {
        name = Script::GetString(L, 2);
    }
    if (n >= 4) {
        flag = Script::GetBool(L, 3);
    }
    HealthBar_SetName(bar, name);
    At<uint8_t>(bar, 0xBA) = flag;
    if (flag == 0) {
        At<uint8_t>(bar, 0xBB) = 0;
    }
    return 0;
}

// (prop, socket) -> x, y, z (the prop's position when there's no such socket; a missing
// prop reads through null)
int __cdecl Native_PAnimGetSocketXYZ(lua_State* L) {
    uint8_t* prop = PropAnims_Get(m_triggerPoolWrapper, Script::GetInt(L, 0));
    float* p = EntityPos(prop);
    float pos[3] = {p[0], p[1], p[2]};
    uint32_t socket = O_HashName(Script::GetString(L, 1));
    float transform[16];
    transform[12] = 1.0f;
    if (O_GetNodeTransform(prop, socket, transform, 1) != 0) {
        pos[0] = transform[9];
        pos[1] = transform[10];
        pos[2] = transform[11];
    }
    Script::PushFloat(L, pos[0]);
    Script::PushFloat(L, pos[1]);
    Script::PushFloat(L, pos[2]);
    return 3;
}

} // namespace

#define OBJECTS_NATIVE(name, address)                                                  \
    static decltype(&Native_##name) Original_ON_##name;                                  \
    REVERSED("Native_" #name, address, RE_BODY(&Native_##name), nullptr, \
             reinterpret_cast<void**>(&Original_ON_##name), re::Mode::Original)

OBJECTS_NATIVE(ObjectDelete, 0x005C2C20);
OBJECTS_NATIVE(ObjectIsInAreaXYZ, 0x005C2C60);
OBJECTS_NATIVE(ObjectIsInAreaObject, 0x005C2D70);
OBJECTS_NATIVE(ObjectToggleVulnerability, 0x005C2F80);
OBJECTS_NATIVE(ObjectNumProjectileImpacts, 0x005C3030);
OBJECTS_NATIVE(ObjectIsDestroyed, 0x005C3110);
OBJECTS_NATIVE(ObjectTypeIsInTrigger, 0x005C3180);
OBJECTS_NATIVE(ObjectRemovePickupsInTrigger, 0x005C3310);
OBJECTS_NATIVE(ObjectTypeSetPickupListOverride, 0x005C3540);
static void* Original_ON_CreateObjectAt;
REVERSED("ScriptNatives::CreateObjectAt_5C35E0", 0x005C35E0, RE_BODY(&CreateObjectAtImpl), nullptr,
         &Original_ON_CreateObjectAt, re::Mode::Original);
OBJECTS_NATIVE(ObjectCreateXYZ, 0x005C3750);
OBJECTS_NATIVE(ObjectCreatePoint, 0x005C3800);
OBJECTS_NATIVE(ObjectFindInArea, 0x005C38F0);
OBJECTS_NATIVE(PAnimDestroyedByPed, 0x005C3AB0);
OBJECTS_NATIVE(PAnimHideHealthBar, 0x005C3B70);
OBJECTS_NATIVE(PAnimFollowPath, 0x005C3C90);
OBJECTS_NATIVE(PAnimFollowPathReset, 0x005C3D60);
OBJECTS_NATIVE(PAnimSetPathFollowSpeed, 0x005C3DC0);
OBJECTS_NATIVE(PAnimGetPathFollowSpeed, 0x005C3E40);
OBJECTS_NATIVE(PAnimStopFollowPath, 0x005C3EB0);
OBJECTS_NATIVE(PAnimBreakLight, 0x005C3F10);
OBJECTS_NATIVE(PAnimFollowPathSoundLoop, 0x005C3FD0);
OBJECTS_NATIVE(PAnimShowHealthBar, 0x005C4080);
OBJECTS_NATIVE(PAnimGetSocketXYZ, 0x005C4150);
