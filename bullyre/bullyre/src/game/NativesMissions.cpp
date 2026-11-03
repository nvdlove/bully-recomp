// Script natives 0x5B8F10..0x5C2A70 and their helpers: missions (fail, succeed, start,
// counts), objectives and the fixed timer, persistent entities, projectiles, the photo
// natives (what's in the camera's frame), fires, faction respect, and the area checks
// the object natives use. Each native is `int __cdecl Native_X(lua_State* L)`
// returning how many results it pushed (see ScriptApi.h); helpers keep their own
// conventions.

#include <cmath>
#include <cstdint>

#include "game/NativesMPP.h"
#include "reversed.h"

namespace {

// ---- the game's functions

// the mission manager (0x20C3CA0)
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetCurrent, 0x006AA710, void* mgr);
GAME_FN(uint8_t*, __thiscall, Missions_GetCurrent, 0x006AA680, void* mgr);
GAME_FN(uint8_t*, __thiscall, Missions_GetCurrent2, 0x006AA690, void* mgr);
GAME_FN(uint8_t, __thiscall, Mission_6AA430, 0x006AA430, void* mission);
GAME_FN(uint8_t, __thiscall, MissionSlot_IsActive, 0x006AA3F0, void* slot);
// (the flags are bytes pushed as dwords)
GAME_FN(uint8_t, __thiscall, Mission_End, 0x006AA300, void* mission, int, int, int, int, int, int, int);
GAME_FN(void, __thiscall, Mission_Succeed, 0x006ABF40, void* mission, int, int, int);
GAME_FN(int, __thiscall, Missions_Find, 0x006A9FD0, void* mgr, const uint32_t* key);
// a mission's record: +0 attempts (u16), +2 successes (u16), +4 flags
GAME_FN(uint8_t*, __thiscall, Missions_GetRecord, 0x006AA7C0, void* mgr, int index);
GAME_FN(void, __thiscall, Missions_6AD290, 0x006AD290, void* mgr, int);
GAME_FN(void, __thiscall, Missions_Start, 0x006AD100, void* mgr, int index, int flag);
GAME_FN(void, __thiscall, Missions_6AA150, 0x006AA150, void* mgr);
GAME_FN(uint8_t, __thiscall, Missions_IsRunning, 0x006AA940, void* mgr, int index);
GAME_FN(void, __cdecl, ShowMessage_7074E0, 0x007074E0, const char* name, int, float, const char*, uintptr_t, int);
// a name's key (thiscall on 4 bytes; its destructor, 0x49A650, does nothing)
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(uint32_t, __cdecl, MM_HashName, 0x00576ED0, const char* name);

// animation groups
GAME_FN(int, __cdecl, AnimGroups_GetStreamingId, 0x0052DAD0, const char* name);
GAME_FN(void*, __cdecl, GetAgrGroupOf, 0x0052DB50, int id, uint32_t* index);
GAME_FN(void, __cdecl, MM_RequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __cdecl, MM_LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);
GAME_FN(void, __cdecl, MM_LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __thiscall, AgrManager_6BD010, 0x006BD010, void* manager, void* group, uint32_t index);
GAME_FN(void, __thiscall, Script_AddAnimGroup, 0x005D9550, void* script, int id);
GAME_FN(void, __thiscall, Script_RemoveAnimGroup, 0x005D95A0, void* script, int id);

// the camera's photo
GAME_VAR(void*, g_photoHud, 0x00C674E0);
GAME_FN(void, __thiscall, Photo_SetValid, 0x00545260, void* photo, int valid, uint32_t a, uint32_t b);
GAME_FN(void, __cdecl, DetonateProjectiles_4D4500, 0x004D4500, float x, float y, float z, float radius, int type, void* ped);
GAME_FN(void, __thiscall, Gifts_AddRequirement, 0x006A99E0, void* gifts, void* requirement);
GAME_FN(void, __thiscall, PlayerInfo_SetFactionRespect, 0x00446760, void* info, int faction, int respect);
GAME_FN(void, __cdecl, E3Dodgeball_CleanObjects1, 0x00444BB0);
GAME_FN(void, __cdecl, ClearAllProjectiles, 0x004D4A90);

// fires (0xBD0CF8)
inline void* const fireManager = reinterpret_cast<void*>(0x00BD0CF8);
GAME_FN(int, __thiscall, Fires_Create, 0x0042DB60, void* mgr, float x, float y, float z, void* trigger, uintptr_t, const char* name);
GAME_FN(void, __thiscall, Fires_42D820, 0x0042D820, void* mgr, int fire, float, float);
GAME_FN(void, __thiscall, Fires_42D860, 0x0042D860, void* mgr, int fire, float, float);

// pools by handle
GAME_FN(uint8_t*, __thiscall, ObjectPool_GetAtRef, 0x005C1470, void* pool, int handle);
GAME_FN(uint8_t*, __thiscall, ObjectPool_436B90, 0x00436B90, void* pool, int handle);
GAME_FN(uint8_t*, __thiscall, Pool_GetAtRef, 0x00425AD0, void* pool, int handle);
GAME_FN(uint8_t*, __thiscall, Pool_GetAtSlot, 0x00415DE0, void* pool, int slot);
GAME_FN(uint8_t*, __cdecl, MM_GetPropAnim, 0x0044A5E0, int handle);
GAME_FN(uint8_t*, __cdecl, MM_GetVehicle, 0x0044A580, int handle);
GAME_FN(int, __thiscall, Pool_ChainSize, 0x00435BB0, void* pool);
GAME_FN(uint8_t*, __thiscall, Pool_ChainAt, 0x00435BD0, void* pool, int slot, int);

// targeting / visibility
GAME_FN(int, __thiscall, TS_GetEntityKind, 0x004D5AE0, void* ts, void* entity);
GAME_FN(float, __thiscall, TS_ScoreEntity, 0x004D7AF0, void* ts, int kind, void* entity, int);
GAME_FN(uint8_t, __thiscall, TS_IsEntityVisible, 0x004D6E20, void* ts, void* entity, int);
GAME_FN(float, __thiscall, TS_ScorePoint, 0x004D70A0, void* ts, int, float x, float y, float z, float, float);
GAME_VAR(int16_t, g_frameCounter16, 0x00C1AE88);
GAME_VAR(void*, g_camera, 0x00C3CCF4);
GAME_FN(void*, __cdecl, CameraObject, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, Frame_GetTransform, 0x006C9A90, void* frame);
GAME_FN(uint8_t, __cdecl, IsLineOfSightClear, 0x004617D0, const float* from, const float* to, int flags);
GAME_FN(float*, __cdecl, VectorAdd, 0x004129A0, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, CrossProduct, 0x00414730, float* out, const float* a, const float* b);
GAME_FN(float, __thiscall, Normalise, 0x00414E10, float* v);

// ped models
GAME_FN(uint8_t*, __cdecl, AddPedModel, 0x0051C960, int id, const char* name, int special);
GAME_FN(uint8_t*, __cdecl, RedefinePedModel, 0x0051B360, int id, const char* name, int special);
GAME_FN(void, __thiscall, ModelInfo_SetTexDictionary, 0x0050E900, void* mi, const char* name, int);
GAME_FN(void, __thiscall, PedMI_522190, 0x00522190, void* mi, const char*);
GAME_FN(void, __thiscall, PedMI_5221C0, 0x005221C0, void* mi, const char*);
GAME_FN(void, __thiscall, PedMI_5221F0, 0x005221F0, void* mi, const char*);
GAME_FN(int, __cdecl, Fn_488B70, 0x00488B70, const char*);
GAME_FN(void, __thiscall, PedMI_522370, 0x00522370, void* mi, int);
GAME_FN(void, __thiscall, ModelInfo_SetColModel, 0x0050EB30, void* mi, void* colModel, int owns);
GAME_FN(int, __cdecl, Fn_488B10, 0x00488B10, const char*);
GAME_VAR(void*, g_pedStatsLike, 0x00C2C084);
GAME_FN(int, __thiscall, Fn_49A0F0, 0x0049A0F0, void* object, const char*);
GAME_FN(void, __thiscall, PedMI_5226D0, 0x005226D0, void* mi, const char*, const char*);
GAME_FN(void, __thiscall, PedMI_5227D0, 0x005227D0, void* mi, const char*, const char*);
GAME_ARRAY(void*, g_tempColModels, 0x00CF3120);   // [0] no collision, [3] a ped's

// projectiles
GAME_FN(uint8_t*, __cdecl, AddProjectile, 0x004D4D90, uintptr_t creator, int type, ScriptVector pos, ScriptVector dir, uintptr_t,
        float radius, float, uintptr_t, int, int, ScriptVector, ScriptVector, uintptr_t, int);
GAME_FN(void, __thiscall, Placeable_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Entity_443AA0, 0x00443AA0, void* entity);
GAME_FN(void, __thiscall, Entity_465760, 0x00465760, void* entity);

// objectives (0x20C4420) and the mission timer (0xBF4508)
inline void* const objectives = reinterpret_cast<void*>(0x020C4420);
GAME_FN(uint8_t, __thiscall, Objectives_Add, 0x006AE270, void* self, const char* name, int);
GAME_FN(void, __thiscall, Objectives_SetReminderTime, 0x006AE580, void* self, float time);
GAME_FN(void, __thiscall, Objectives_SetParamFloat, 0x006AE3D0, void* self, int id, int param, float value);
GAME_FN(void, __thiscall, Objectives_SetParamInt, 0x006AE390, void* self, int id, int param, int value);
GAME_FN(void, __thiscall, Objectives_SetParamString, 0x006AE410, void* self, int id, int param, const char* value);
inline void* const missionTimer = reinterpret_cast<void*>(0x00BF4508);
GAME_FN(void, __thiscall, MissionTimer_Stop, 0x0043F750, void* self, int);
GAME_FN(void, __thiscall, MissionTimer_Start, 0x0043F690, void* self, int seconds);
GAME_VAR(uint8_t, g_clockSeconds, 0x00BCEB48);
GAME_VAR(uint8_t, g_clockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t, g_clockHours, 0x00BCEB4B);

// persistent entities
GAME_FN(int, __cdecl, MM_GetModelIndex, 0x0051C330, const char* name);
GAME_FN(float*, __cdecl, CombineTranslation, 0x00415F50, float* out, const float* translation, int mode);
GAME_FN(uint8_t*, __cdecl, CreateTriggerObject_44CE70, 0x0044CE70, int model, int area, float* transform, float, uintptr_t, int, int);
GAME_FN(void, __thiscall, Matrix_SetRotateZ, 0x00412C20, void* m, float angle);
GAME_FN(void, __thiscall, Matrix_RotateX_413020, 0x00413020, void* m, float angle);
GAME_FN(void*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void*, __cdecl, Dummy_New, 0x004653A0, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, DummyObject_Construct, 0x004CF200, void* dummy);
GAME_FN(void*, __cdecl, Object_New, 0x004CFCD0, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, Object_ConstructFromDummy, 0x004D05E0, void* object, void* dummy);
GAME_FN(void*, __cdecl, Building_New, 0x004CED70, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, Building_Construct, 0x004CECB0, void* building);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(uint8_t*, __cdecl, GetColModel, 0x0051AF50, void* entity);
GAME_FN(void*, __cdecl, ColStore_GetBoundingBox, 0x0056F0F0, int slot);
GAME_FN(void, __thiscall, Rect_Extend, 0x00414410, void* rect, void* other);
GAME_FN(uint8_t, __cdecl, IsSpecialModel, 0x0043F3A0, int model);
GAME_FN(float, __thiscall, MI_GetLargestLodDistance, 0x00527420, void* mi);
GAME_FN(void, __thiscall, Entity_466260, 0x00466260, void* entity);
GAME_FN(void, __cdecl, Entity_45E690, 0x0045E690, void* entity);

// sockets
GAME_FN(uint8_t*, __cdecl, Object_GetProp, 0x004EA3C0, void* object);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, void* entity, uint32_t node, float* transform, int flag);
GAME_VAR(float, g_socketStep, 0x00AE9B80);   // 0.02
GAME_VAR(float, ms_timeStep, 0x00C1A9A4);

// misc
inline void* const g_modelNameTable = reinterpret_cast<void*>(0x00BCCE80);
GAME_FN(void, __thiscall, ModelNames_4186E0, 0x004186E0, void* table, const char* name);
GAME_FN(int, __thiscall, ModelNames_418950, 0x00418950, void* table, const char* a, const char* b);
GAME_FN(void, __thiscall, ClothingItem_FromHash, 0x006CA4F0, void* item, uint32_t hash, int slot);
GAME_VAR(uint8_t, g_nisFlag, 0x00CFB340);
inline void* const g_routes = reinterpret_cast<void*>(0x00BC5E7C);
GAME_FN(int, __thiscall, Routes_Find, 0x006D63C0, void* routes, const char* name);
GAME_FN(void*, __thiscall, Routes_Get, 0x006D6450, void* routes, int index);
GAME_FN(float*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(uint16_t, __cdecl, Object_GetHandle16, 0x0044CAF0, void* object);
GAME_FN(void, __thiscall, List_Reserve, 0x006DB480, void* list, uint32_t n, int);
GAME_FN(uint8_t, __thiscall, Entity_IsInArea2D, 0x0046D9D0, void* entity, float x1, float y1, float x2, float y2);
GAME_FN(uint8_t, __thiscall, Entity_IsInArea3D, 0x0046DAB0, void* entity, float x1, float y1, float z1, float x2,
        float y2, float z2);
GAME_FN(void, __cdecl, DebugDrawArea_52A600, 0x0052A600, int, int, int, float*, float, void*, int, int, int, int, int,
        int, int);
GAME_VAR(uint8_t, g_cutsceneRunning, 0x020C5BE4);
GAME_ARRAY(uint8_t*, g_cutsceneObjects, 0x020C5B68);
GAME_FN(uint8_t, __cdecl, PhotoTargetVisible, 0x005C16E0, int handle, int type, int flag);
GAME_FN(uint8_t*, __cdecl, PhotoGetEntity, 0x005C15D0, int handle, int type, int flag);

GAME_VAR(int32_t, g_photoIndex, 0x00CFB358);
GAME_VAR(int32_t, g_photoCount, 0x00CFB354);
GAME_VAR(int32_t, g_photoType, 0x00CFB370);

GAME_FN(void, __cdecl, M_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, M_Malloc, 0x005EEAA0, uint32_t size);

// A growable array of handles (+0 data, +4 capacity, +8 count)
struct HandleList {
    re::Ptr32<uint32_t> data;
    uint32_t capacity;
    uint32_t count;
};
static_assert(sizeof(HandleList) == 0xC);

// A script entity lookup by kind (ped kinds: its vehicle when in one)
uint8_t* EntityOrVehicle(int handle, int type) {
    uint8_t* e = nullptr;
    switch (type) {
    case 0:
        return ObjectPool_GetAtRef(m_objectPool, handle);
    case 1:
        return MM_GetVehicle(handle);
    case 2:
        e = M_GetPedFromHandle(handle, 2);
        break;
    case 3:
        e = m_playerPed;
        break;
    default:
        return nullptr;
    }
    if (e == nullptr) {
        return nullptr;
    }
    uint8_t* vehicle = At<uint8_t*>(e, 0x1554);
    return vehicle != nullptr ? vehicle : e;
}

// ---- 0x5B8F10.. (helpers the natives' registrars use; not in lua_natives.csv)

// 0x5B8F10: (name): the model name table's 0x4186E0
int __cdecl ModelNames_Add(lua_State* L) {
    ModelNames_4186E0(g_modelNameTable, Script::GetString(L, 0));
    return 0;
}

// 0x5B8F30: (a, b) -> the model name table's 0x418950
int __cdecl ModelNames_Lookup(lua_State* L) {
    const char* a = Script::GetString(L, 0);
    const char* b = Script::GetString(L, 1);
    Script::PushInt(L, ModelNames_418950(g_modelNameTable, a, b));
    return 1;
}

// 0x5BA870 (thiscall): a clothing item from (hash, slot); returns it
void* __fastcall ClothingItem_Construct(void* self, re::Edx, uint32_t hash, int slot) {
    ClothingItem_FromHash(self, hash, slot);
    return self;
}

// 0x5BA890 (thiscall): the selected entry (+0x178C) of the 0x54-byte ones at +0xC10
uint8_t* __fastcall Shop_GetSelected(uint8_t* self, re::Edx) {
    int index = At<int32_t>(self, 0x178C);
    if (index == -1) {
        return nullptr;
    }
    return self + index * 0x54 + 0xC10;
}

// 0x5BB380
uint8_t __cdecl Nis_IsSet() {
    return g_nisFlag;
}

// 0x5BB390
void __cdecl Nis_Clear() {
    g_nisFlag = 0;
}

// 0x5BFE10: a route's first point (zero for an unknown name)
float* __cdecl Route_GetStart(float* out, const char* name) {
    int index = Routes_Find(g_routes, name);
    if (index == -1) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
        return out;
    }
    float* p = Route_First(Routes_Get(g_routes, index));
    out[0] = p[0];
    out[1] = p[1];
    out[2] = p[2];
    return out;
}

// ---- natives

// (trigger, socket name, radius): a projectile (the ball) is within its model's bounding
// sphere + radius of the prop's socket, looking one step ahead along its path
int __cdecl Native_HasBallTouchedSocketRadius(lua_State* L) {
    uint8_t* trigger = PoolSlot(m_triggerPool, Script::GetInt(L, 0));
    uint8_t* prop = Object_GetProp(trigger);
    uint32_t socket;
    NameKey_Construct(&socket, Script::GetString(L, 1));
    float radius = Script::GetFloat(L, 2);
    float transform[16];   // an NiTransform: rotation, translation (+0x24), scale (+0x30)
    transform[12] = 1.0f;
    if (GetNodeTransform(prop, socket, transform, 1) == 0) {
        Script::PushBool(L, 0);
        return 1;
    }
    float socketPos[3] = {transform[9], transform[10], transform[11]};
    CPool<uint8_t>* pool = m_projectilePool;
    int n = pool->size;
    if (pool->Next() != nullptr) {
        n += Pool_ChainSize(pool->Next());
    }
    uint8_t hit = 0;
    for (int i = 0; i < n; i++) {
        uint8_t* ball = nullptr;
        if (i < pool->size) {
            ball = pool->objects + pool->elemSize * i;
        } else if (pool->Next() != nullptr) {
            ball = Pool_ChainAt(pool->Next(), i - pool->size, 1);
        }
        // (the first pool's flags, even for chained slots)
        if ((pool->flags[i] & 0x80) != 0 || ball == nullptr) {
            continue;
        }
        float* p = EntityPos(ball);
        float p0[3] = {p[0], p[1], p[2]};
        float v[3] = {At<float>(ball, 0x124), At<float>(ball, 0x128), At<float>(ball, 0x12C)};
        float gravity = VCall<float>(ball, 0x8C);
        float t = g_socketStep * ms_timeStep;
        float t2 = t * t;
        v[2] = v[2] + (t * v[2] - gravity * 0.5f * t2);
        float p1[3];
        VectorAdd(p1, p0, v);
        float toSocket[3];
        VectorSub(toSocket, socketPos, p0);
        float d[3];
        VectorSub(d, p1, p0);
        float c1[3];
        CrossProduct(c1, toSocket, d);
        float normal[3];
        CrossProduct(normal, d, c1);
        Normalise(normal);
        float dist = normal[0] * toSocket[0] + normal[1] * toSocket[1] + normal[2] * toSocket[2];
        dist = -dist;
        float w[3] = {dist * normal[0], dist * normal[1], dist * normal[2]};
        float closest[3];
        VectorAdd(closest, socketPos, w);
        float len = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
        len = std::sqrt(std::fabs(len));
        float r;
        if (len < 0.01) {
            r = 0.0f;
        } else {
            r = (closest[1] - p0[1]) / d[1];
            if (r < 0.0f) {
                r = 0.0f;
            } else if (r > 1.0f) {
                r = 1.0f;
            }
        }
        float s[3] = {v[0] * r, v[1] * r, r * v[2]};
        float at[3];
        float diff[3];
        VectorSub(diff, VectorAdd(at, p0, s), socketPos);
        float* sphere = At<float*>(At<uint8_t*>(At<uint8_t*>(m_modelInfoPtrs[At<int16_t>(ball, 0x10E)], 0xC), 0x38), 0);
        float reach = sphere[3] + radius;
        float distance = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];
        distance = std::sqrt(std::fabs(distance));
        if (reach > distance) {
            hit = 1;
            break;
        }
    }
    Script::PushBool(L, hit);
    return 1;
}

// (model name or index, x, y, z, heading, area, [x rotation]) -> (handle, kind): a
// building, an object (models with a col slot, made from a dummy) or a model type 10
// thing (0x44CE70); -1, -1 for an unknown model. (A null model info reads through null)
int __cdecl Native_CreatePersistentEntity(lua_State* L) {
    int model;
    if (Script::IsNumber(L, 0) != 0) {
        model = Script::GetInt(L, 0);
    } else {
        model = MM_GetModelIndex(Script::GetString(L, 0));
    }
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    float heading = float(double(Script::GetFloat(L, 4)) * 3.1415927410125732 / 180.0);
    int area = int(Script::GetFloat(L, 5));
    float rotX = 0.0f;
    if (Script::GetArgCount(L) == 7) {
        rotX = Script::GetFloat(L, 6);
    }
    if (model == -1) {
        Script::PushInt(L, -1);
        Script::PushInt(L, -1);
        return 2;
    }
    uint8_t* mi = m_modelInfoPtrs[model];
    uint8_t* entity = nullptr;
    if (mi != nullptr) {
        if (At<uint8_t>(mi, 8) == 10) {
            float transform[16];
            transform[12] = 1.0f;
            float pos[3] = {x, y, z};
            CombineTranslation(transform, pos, 0);
            entity = CreateTriggerObject_44CE70(model, area, transform, 0.0f, 0, 0, 0);
            if (At<void*>(entity, 0x14) != nullptr) {
                Matrix_SetRotateZ(At<void*>(entity, 0x14), heading);
            }
            At<float>(entity, 0x10) = heading;
        } else {
            if (At<int16_t>(mi, 0x12) != -1) {
                void* memory = Dummy_New(0x128);
                uint8_t* dummy = memory != nullptr ? DummyObject_Construct(memory) : nullptr;
                VCall<void>(dummy, 0x24, model);
                memory = Object_New(0x22C);
                entity = memory != nullptr ? Object_ConstructFromDummy(memory, dummy) : nullptr;
                At<uint8_t>(entity, 0x208) = 2;
                World_Remove(dummy);
                VCall<void>(dummy, 0, 1);
            } else {
                void* memory = Building_New(0x120);
                entity = memory != nullptr ? Building_Construct(memory) : nullptr;
                VCall<void>(entity, 0x24, model);
            }
            if (At<void*>(entity, 0x14) != nullptr) {
                Matrix_SetRotateZ(At<void*>(entity, 0x14), heading);
            }
            At<float>(entity, 0x10) = heading;
            Placeable_SetPosition(entity, x, y, z);
            At<float>(entity, 0x11C) = 0.0f;
            At<uint8_t>(entity, 0x111) = uint8_t(area);
            if (At<uint8_t>(mi, 8) == 4) {
                VCall<void>(entity, 0x28, 1, 1);
            }
            if (At<int16_t>(mi, 0x12) != -1) {
                if (At<void*>(mi, 0xC) == nullptr) {
                    ModelInfo_SetColModel(mi, g_tempColModels[0], 0);
                }
                int slot = At<uint16_t>(GetColModel(entity), 0x30);
                uint8_t rect[16];
                void* bounds = VCall<void*>(entity, 0x2C, static_cast<void*>(rect));
                Rect_Extend(ColStore_GetBoundingBox(slot), bounds);
            } else {
                if (At<void*>(mi, 0xC) == nullptr) {
                    ModelInfo_SetColModel(mi, g_tempColModels[0], 0);
                }
                if (IsSpecialModel(At<int16_t>(entity, 0x10E)) != 0) {
                    At<int32_t>(entity, 0xC8) = 1;
                }
                uint8_t type = At<uint8_t>(mi, 8);
                if (type == 1 || type == 3) {
                    if ((At<uint32_t>(mi, 0x28) & 0x800) != 0) {
                        At<int32_t>(entity, 0xA0) = 1;
                    }
                    if (MI_GetLargestLodDistance(mi) < 2.0f) {
                        At<int32_t>(entity, 0x48) = 0;
                    }
                }
                Entity_466260(entity);
            }
        }
        if (rotX != 0.0f) {
            float angle = float(double(rotX) * 3.1415927410125732 / 180.0);
            Matrix_RotateX_413020(Entity_GetMatrix(entity), angle);
        }
        World_Add(entity);
    }
    int handle = -1;
    int kind = -1;
    switch (At<uint8_t>(entity, 0x108) & 7) {
    case 1:
        kind = HANDLE_BUILDING;
        handle = PoolHandle(m_buildingPool, entity);
        break;
    case 4:
        handle = PoolHandle(m_objectPool, entity);
        kind = HANDLE_OBJECT;
        break;
    case 6:
        kind = HANDLE_PROPANIM;
        handle = PoolHandle(m_propAnimPool, entity);
        break;
    }
    Script::PushInt(L, handle);
    Script::PushInt(L, kind);
    return 2;
}

// ([fade = 1], [a = 1], [message]): when the current mission is the running one and
// 0x6AA430 allows, it's ended first (its +0x15 set)
int __cdecl Native_MissionFail(lua_State* L) {
    uint8_t fade = 1;
    if (Script::GetArgCount(L) >= 1) {
        fade = Script::GetBool(L, 0);
    }
    uint8_t second = 1;
    const char* message = nullptr;
    if (Script::GetArgCount(L) >= 2) {
        second = Script::GetBool(L, 1);
    }
    if (Script::GetArgCount(L) >= 3) {
        message = Script::GetString(L, 2);
    }
    uint8_t* current = MissionMgr_GetCurrent(missionMgr);
    if (current == Missions_GetCurrent(missionMgr)) {
        if (Mission_6AA430(Missions_GetCurrent2(missionMgr)) != 0) {
            Mission_End(Missions_GetCurrent2(missionMgr), 0, 0, 0, fade, 0, 0, 0);
            At<uint8_t>(Missions_GetCurrent2(missionMgr), 0x15) = 1;
        }
    }
    Mission_End(MissionMgr_GetCurrent(missionMgr), 0, 0, 0, fade, second,
                int(reinterpret_cast<uintptr_t>(message)), 0);
    return 0;
}

// ([a = 1], [b = 1], [show = 1], [time], [text]): "M_PASS" shown when asked
int __cdecl Native_MissionSucceed(lua_State* L) {
    uint8_t a = Script::GetBoolOpt(L, 0, 1);
    uint8_t b = Script::GetBoolOpt(L, 1, 1);
    Mission_Succeed(MissionMgr_GetCurrent(missionMgr), 0, a, b);
    float time = 0.0f;
    uint8_t show = 1;
    const char* text = nullptr;
    if (Script::GetArgCount(L) >= 3) {
        show = Script::GetBool(L, 2);
    }
    if (Script::GetArgCount(L) >= 4) {
        time = Script::GetFloat(L, 3);
        if (Script::GetArgCount(L) >= 5) {
            text = Script::GetString(L, 4);
        }
    }
    if (show != 0) {
        ShowMessage_7074E0("M_PASS", 1, time, text, 0, 0);
    }
    return 0;
}

int MissionIndex(lua_State* L, uint32_t* key) {
    NameKey_Construct(key, Script::GetString(L, 0));
    return Missions_Find(missionMgr, key);
}

int __cdecl Native_StartMission(lua_State* L) {
    uint32_t key;
    Missions_Start(missionMgr, MissionIndex(L, &key), 0);
    return 0;
}

// (name, [flag = 0])
int __cdecl Native_ForceStartMission(lua_State* L) {
    uint32_t key;
    NameKey_Construct(&key, Script::GetString(L, 0));
    uint8_t flag = Script::GetBoolOpt(L, 1, 0);
    int index = Missions_Find(missionMgr, &key);
    Missions_GetRecord(missionMgr, index)[4] |= 2;
    Missions_6AD290(missionMgr, 1);
    Missions_Start(missionMgr, index, flag);
    return 0;
}

// (index): a running mission slot is ended first
int __cdecl Native_ForceStartMissionIndex(lua_State* L) {
    int index = Script::GetInt(L, 0);
    if (MissionSlot_IsActive(Missions_GetCurrent(missionMgr)) != 0) {
        Mission_End(Missions_GetCurrent(missionMgr), 1, 0, 0, 1, 0, 0, 0);
    }
    Missions_GetRecord(missionMgr, index)[4] |= 2;
    Missions_6AD290(missionMgr, 1);
    Missions_Start(missionMgr, index, 0);
    return 0;
}

int __cdecl Native_ForceMissionAvailable(lua_State* L) {
    uint32_t key;
    int index = MissionIndex(L, &key);
    if (index != -1) {
        Missions_GetRecord(missionMgr, index)[4] |= 2;
        Missions_6AD290(missionMgr, 1);
    }
    return 0;
}

int __cdecl Native_GetMissionAttemptCount(lua_State* L) {
    uint32_t key;
    Script::PushInt(L, At<uint16_t>(Missions_GetRecord(missionMgr, MissionIndex(L, &key)), 0));
    return 1;
}

int __cdecl Native_GetMissionSuccessCount(lua_State* L) {
    uint32_t key;
    Script::PushInt(L, At<uint16_t>(Missions_GetRecord(missionMgr, MissionIndex(L, &key)), 2));
    return 1;
}

int __cdecl Native_SetMissionSuccessCount(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    int count = Script::GetInt(L, 1);
    uint32_t key;
    NameKey_Construct(&key, name);
    int index = Missions_Find(missionMgr, &key);
    At<uint16_t>(Missions_GetRecord(missionMgr, index), 2) = uint16_t(count);
    return 0;
}

int __cdecl Native_MissionSuccessCountInc(lua_State* L) {
    uint32_t key;
    int index = MissionIndex(L, &key);
    int count = At<uint16_t>(Missions_GetRecord(missionMgr, index), 2) + 1;
    At<uint16_t>(Missions_GetRecord(missionMgr, index), 2) = uint16_t(count);
    Missions_6AA150(missionMgr);
    return 0;
}

int __cdecl Native_MissionAttemptCountInc(lua_State* L) {
    uint32_t key;
    int index = MissionIndex(L, &key);
    int count = At<uint16_t>(Missions_GetRecord(missionMgr, index), 0) + 1;
    At<uint16_t>(Missions_GetRecord(missionMgr, index), 0) = uint16_t(count);
    return 0;
}

// (its success count is non-zero)
int __cdecl Native_IsMissionCompleated(lua_State* L) {
    uint32_t key;
    Script::PushBool(L, At<uint16_t>(Missions_GetRecord(missionMgr, MissionIndex(L, &key)), 2) != 0);
    return 1;
}

// (flag 1 set and not running)
int __cdecl Native_IsMissionAvailable(lua_State* L) {
    uint32_t key;
    int index = MissionIndex(L, &key);
    uint8_t available = 0;
    if (index >= 0 && (Missions_GetRecord(missionMgr, index)[4] & 1) != 0) {
        if (Missions_IsRunning(missionMgr, index) == 0) {
            available = 1;
        }
    }
    Script::PushBool(L, available);
    return 1;
}

int __cdecl Native_MissionForceCompleted(lua_State* L) {
    uint32_t key;
    Missions_GetRecord(missionMgr, MissionIndex(L, &key))[4] |= 8;
    return 0;
}

// Loads the group now (a loading screen's worth) and adds it to the script's
int __cdecl Native_LoadAnimationGroup(lua_State* L) {
    int id = AnimGroups_GetStreamingId(Script::GetString(L, 0));
    if (id >= 0) {
        uint32_t index;
        void* group = GetAgrGroupOf(id, &index);
        MM_RequestModel(id, 4);
        MM_LoadingScreen("cmdLoadAnimationGroup()", "CStreaming::LoadAllRequestedModels()", 0);
        MM_LoadAllRequestedModels(0);
        AgrManager_6BD010(reinterpret_cast<void*>(0x020C4910), group, index);
        Script_AddAnimGroup(M_Scripts_GetCurrent(m_lurStore), id);
    }
    return 0;
}

int __cdecl Native_UnLoadAnimationGroup(lua_State* L) {
    int id = AnimGroups_GetStreamingId(Script::GetString(L, 0));
    if (id >= 0) {
        Script_RemoveAnimGroup(M_Scripts_GetCurrent(m_lurStore), id);
    }
    return 0;
}

// (valid, [name a], [name b])
int __cdecl Native_PhotoSetValid(lua_State* L) {
    uint32_t a = 0;
    uint32_t b = 0;
    if (Script::GetArgCount(L) >= 2) {
        a = MM_HashName(Script::GetString(L, 1));
    }
    if (Script::GetArgCount(L) >= 3) {
        b = MM_HashName(Script::GetString(L, 2));
    }
    uint8_t valid = Script::GetBool(L, 0);
    Photo_SetValid(g_photoHud, valid != 0, a, b);
    return 0;
}

// (x, y, z, radius, type, ped handle: 0 the player, -1 none)
int __cdecl Native_DetonateProjectiles(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    float radius = Script::GetFloat(L, 3);
    int type = int(Script::GetFloat(L, 4));
    int handle = int(Script::GetFloat(L, 5));
    uint8_t* ped = nullptr;
    if (handle != -1) {
        ped = M_GetPedFromHandle(handle, handle != 0 ? 2 : 3);
    }
    DetonateProjectiles_4D4500(x, y, z, radius, type, ped);
    return 0;
}

// (model, a, [name], [name], flag, b, [name]): a gift requirement
int __cdecl Native_PedModelCreateGiftRequirement(lua_State* L) {
    struct {
        uint8_t b5;        // +0
        uint8_t flag4;     // +1
        int16_t a1;        // +2
        int32_t model;     // +4
        uint32_t hash2;    // +8
        uint32_t hash3;    // +0xC
        uint32_t hash6;    // +0x10
    } req;
    static_assert(sizeof(req) == 0x14);
    req.model = Script::GetInt(L, 0);
    req.a1 = int16_t(Script::GetInt(L, 1));
    req.hash2 = Script::IsString(L, 2) != 0 ? MM_HashName(Script::GetString(L, 2)) : 0;
    req.hash3 = Script::IsString(L, 3) != 0 ? MM_HashName(Script::GetString(L, 3)) : 0;
    req.flag4 = Script::GetBool(L, 4);
    req.b5 = uint8_t(Script::GetInt(L, 5));
    if (lua::gettop(L) > 6 && Script::IsString(L, 6) != 0) {
        req.hash6 = MM_HashName(Script::GetString(L, 6));
    } else {
        req.hash6 = 0;
    }
    Gifts_AddRequirement(reinterpret_cast<void*>(0x020C3698), &req);
    return 0;
}

// (the current mission slot's +0x1E, when 0x6AA430 allows)
int __cdecl Native_MissionAllowConcurrentMissions(lua_State* L) {
    if (Mission_6AA430(Missions_GetCurrent(missionMgr)) != 0) {
        uint8_t allow = Script::GetBool(L, 0);
        At<uint8_t>(Missions_GetCurrent(missionMgr), 0x1E) = allow;
    }
    return 0;
}

// (faction, respect clamped to 0..100)
int __cdecl Native_SetFactionRespect(lua_State* L) {
    int faction = Script::GetInt(L, 0);
    float respect = float(Script::GetInt(L, 1));
    if (respect < 0.0f) {
        respect = 0.0f;
    } else if (respect > 100.0) {
        respect = 100.0f;
    }
    PlayerInfo_SetFactionRespect(m_playerInfo, faction, int(respect));
    return 0;
}

// 0x5C14B0
void __cdecl ClearDodgeballsAndProjectiles() {
    E3Dodgeball_CleanObjects1();
    ClearAllProjectiles();
}

// (trigger, a (ms), b, c, d, [name]) -> the fire's id
int __cdecl Native_FireCreate(lua_State* L) {
    int slot = Script::GetInt(L, 0);
    int a = Script::GetInt(L, 1);
    int b = Script::GetInt(L, 2);
    int c = Script::GetInt(L, 3);
    int d = Script::GetInt(L, 4);
    const char* name = Script::GetArgCount(L) == 6 ? Script::GetString(L, 5) : nullptr;
    uint8_t* trigger = PoolSlot(m_triggerPool, slot);
    int fire = Fires_Create(fireManager, At<float>(trigger, 4), At<float>(trigger, 8), At<float>(trigger, 0xC),
                            trigger, 0, name);
    Fires_42D820(fireManager, fire, float(double(a) * 0.001), float(b));
    Fires_42D860(fireManager, fire, float(c), float(d));
    Script::PushInt(L, fire);
    return 1;
}

// 0x5C15D0: an entity by (handle, kind); `bySlot`: the handle is a pool slot
uint8_t* __cdecl GetEntityByKind(int handle, int kind, int bySlot) {
    if (handle == -1) {
        return nullptr;
    }
    switch (kind) {
    case 0:
        if (re::Flag(bySlot)) {
            return ObjectPool_436B90(m_objectPool, handle);
        }
        return ObjectPool_GetAtRef(m_objectPool, handle);
    case 1:
        if (re::Flag(bySlot)) {
            return Pool_GetAtSlot(m_vehiclePool, handle);
        }
        return Pool_GetAtRef(m_vehiclePool, handle);
    case 2:
        if (re::Flag(bySlot)) {
            return Pool_GetAtSlot(m_pedPool, handle);
        }
        return M_GetPedFromHandle(handle, kind);
    case 3:
        return m_playerPed;
    case 5:
        if (re::Flag(bySlot)) {
            return Pool_GetAtSlot(m_propAnimPool, handle);
        }
        return MM_GetPropAnim(handle);
    }
    return nullptr;
}

// 0x5C1680: how many slots the kind's pool has
int __cdecl GetKindPoolSize(int kind) {
    switch (kind) {
    case 0:
        return m_objectPool->size;
    case 1:
        return m_vehiclePool->size;
    case 2:
        return m_pedPool->size;
    case 3:
        return 1;
    case 5:
        return m_propAnimPool->size;
    }
    return 0;
}

// 0x5C16E0: the entity is a photo target: the targeting system scores it, it's visible
// and a line from the camera to half its height is clear (both marked as seen this
// frame, +0x10A)
uint8_t __cdecl IsPhotoTargetVisible(int handle, int kind, int bySlot) {
    uint8_t* e = PhotoGetEntity(handle, kind, bySlot);
    if (e == nullptr) {
        return 0;
    }
    void* ts = At<void*>(m_playerPed, 0x1C8);
    int targetKind = TS_GetEntityKind(ts, e);
    float score = TS_ScoreEntity(At<void*>(m_playerPed, 0x1C8), targetKind, e, 1);
    if (TS_IsEntityVisible(At<void*>(m_playerPed, 0x1C8), e, 0) != 0 && !(-99999997952.0f == score) && targetKind != 0) {
        At<int16_t>(m_playerPed, 0x10A) = int16_t(g_frameCounter16 + 1);
        At<int16_t>(e, 0x10A) = int16_t(g_frameCounter16 + 1);
        float half = float(VCall<float>(e, 0x6C) * 0.5);
        float offset[3] = {0.0f, 0.0f, half};
        float target[3];
        VectorAdd(target, EntityPos(e), offset);
        uint8_t* transform = Frame_GetTransform(CameraObject(g_camera));
        if (IsLineOfSightClear(reinterpret_cast<float*>(transform + 0x24), target, 0x4B) != 0) {
            return 1;
        }
    }
    return 0;
}

// (handle, kind) or (x, y, z)
int __cdecl Native_PhotoTargetInFrame(lua_State* L) {
    uint8_t result = 0;
    if (Script::GetArgCount(L) == 2) {
        int handle = Script::GetInt(L, 0);
        int kind = Script::GetInt(L, 1);
        result = PhotoTargetVisible(handle, kind, 0);
    } else if (Script::GetArgCount(L) == 3) {
        float x = float(Script::GetInt(L, 0));
        float y = float(Script::GetInt(L, 1));
        float z = float(double(Script::GetInt(L, 2)) + 0.30000001192092896);
        float point[3] = {x, y, z};
        result = 1;
        if (TS_ScorePoint(At<void*>(m_playerPed, 0x1C8), 6, x, y, z, 0.3f, 0.3f) == -99999997952.0f) {
            result = 0;
        }
        At<int16_t>(m_playerPed, 0x10A) = int16_t(g_frameCounter16 + 1);
        uint8_t* transform = Frame_GetTransform(CameraObject(g_camera));
        uint8_t clear = IsLineOfSightClear(reinterpret_cast<float*>(transform + 0x24), point, 0x4B);
        if (clear == 0) {
            result = clear;
        }
    }
    Script::PushBool(L, result);
    return 1;
}

// -> (handle, kind) of the next photo target, walking objects, vehicles, peds, the
// player and prop anims; -1, -1 at the end. (A ped's is its handle; a null ped reads
// through null)
int __cdecl Native_PhotoGetEntityNext(lua_State* L) {
    for (;;) {
        int type = g_photoType;
        if (g_photoIndex >= g_photoCount) {
            g_photoIndex = 0;
            if (type == 0) {
                type = 1;
                g_photoType = type;
            } else if (type == 1) {
                type = 2;
                g_photoType = type;
            } else if (type == 2) {
                type = 3;
                g_photoType = type;
            } else if (type == 3) {
                type = 5;
                g_photoType = type;
            } else if (type == 5) {
                type = -1;
                g_photoType = type;
            }
            int count = 0;
            switch (type) {
            case 0:
                count = m_objectPool->size;
                break;
            case 1:
                count = m_vehiclePool->size;
                break;
            case 2:
                count = m_pedPool->size;
                break;
            case 3:
                count = 1;
                break;
            case 5:
                count = m_propAnimPool->size;
                break;
            }
            g_photoCount = count;
        }
        if (type == -1) {
            Script::PushInt(L, -1);
            Script::PushInt(L, -1);
            return 2;
        }
        if (PhotoTargetVisible(g_photoIndex, type, 1) != 0) {
            break;
        }
        g_photoIndex++;
    }
    int handle;
    if (g_photoType == 2) {
        uint8_t* ped = g_photoIndex == -1 ? nullptr : Pool_GetAtSlot(m_pedPool, g_photoIndex);
        handle = VCall<int>(ped, 0xAC);
    } else {
        handle = g_photoIndex;
    }
    Script::PushInt(L, handle);
    Script::PushInt(L, g_photoType);
    g_photoIndex++;
    return 2;
}

// (model, name, txd, a, b, c, d, e, f, g, h, i, j, k, l, m): defines a ped model.
// (A model info of another type is left null and written through; e's pointer is
// stored as a short)
int __cdecl Native_SetupPedObject(lua_State* L) {
    const char* name = Script::GetString(L, 1);
    const char* txd = Script::GetString(L, 2);
    int i3 = Script::GetInt(L, 3);
    const char* s4 = Script::GetString(L, 4);
    const char* s5 = Script::GetString(L, 5);
    const char* s6 = Script::GetString(L, 6);
    const char* s7 = Script::GetString(L, 7);
    const char* s8 = Script::GetString(L, 8);
    const char* s9 = Script::GetString(L, 9);
    const char* s10 = Script::GetString(L, 10);
    int i11 = Script::GetInt(L, 11);
    const char* s12 = Script::GetString(L, 12);
    const char* s13 = Script::GetString(L, 13);
    const char* s14 = Script::GetString(L, 14);
    const char* s15 = Script::GetString(L, 15);
    int model = Script::GetInt(L, 0);
    uint8_t* mi = nullptr;
    if (m_modelInfoPtrs[model] == nullptr) {
        mi = AddPedModel(model, name, 0);
    } else if (At<uint8_t>(m_modelInfoPtrs[model], 8) == 8) {
        mi = RedefinePedModel(model, name, 0);
    }
    At<uint8_t>(mi, 0x78) = uint8_t(model);
    ModelInfo_SetTexDictionary(mi, txd, 0);
    VCall<void>(mi, 0x18, s7);
    PedMI_522190(mi, s8);
    PedMI_5221C0(mi, s9);
    PedMI_5221F0(mi, s10);
    Fn_488B70(s4);
    PedMI_522370(mi, Fn_488B70(s4));
    ModelInfo_SetColModel(mi, g_tempColModels[3], 0);
    At<int32_t>(mi, 0x4C) = Fn_488B10(s5);
    At<uint8_t>(mi, 0x50) = i3 != 0;
    At<int32_t>(mi, 0x54) = Fn_49A0F0(g_pedStatsLike, s6);
    At<int16_t>(mi, 0x5C) = int16_t(i11);
    At<int32_t>(mi, 0x60) = 0;
    PedMI_5226D0(mi, s12, s13);
    PedMI_5227D0(mi, s14, s15);
    return 0;
}

// (type, x, y, z, vx, vy, vz, [radius = 0.01]) -> its id (+0x238)
int __cdecl Native_CreateProjectile(lua_State* L) {
    int type = Script::GetInt(L, 0);
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    float vx = Script::GetFloat(L, 4);
    float vy = Script::GetFloat(L, 5);
    float vz = Script::GetFloat(L, 6);
    ScriptVector zero = {0.0f, 0.0f, 0.0f};
    float radius = Script::GetFloatOpt(L, 7, 0.0099999998f);
    ScriptVector pos = {x, y, z};
    ScriptVector up = {0.0f, 0.0f, 1.0f};
    uint8_t* projectile = AddProjectile(0, type, pos, up, 0, radius, 0.0f, 0, 0, 0, zero, zero, 0, 0);
    Placeable_SetPosition(projectile, x, y, z);
    At<float>(projectile, 0x124) = vx;
    At<float>(projectile, 0x128) = vy;
    At<float>(projectile, 0x12C) = vz;
    At<float>(projectile, 0x130) = 0.0f;
    At<float>(projectile, 0x134) = 0.0f;
    At<float>(projectile, 0x138) = 0.0f;
    Entity_443AA0(projectile);
    Entity_465760(projectile);
    VCall<void>(projectile, 0xC);
    VCall<void>(projectile, 8);
    Script::PushInt(L, At<int32_t>(projectile, 0x238));
    return 1;
}

// (name, [a], [reminder time]) -> the objective's id
int __cdecl Native_MissionObjectiveAdd(lua_State* L) {
    const char* name = Script::GetString(L, 0);
    uint8_t a = 0;
    if (lua::gettop(L) >= 2) {
        a = uint8_t(Script::GetInt(L, 1));
    }
    uint8_t id = Objectives_Add(objectives, name, a);
    if (lua::gettop(L) >= 3) {
        Objectives_SetReminderTime(objectives, Script::GetFloat(L, 2));
    }
    Script::PushInt(L, id);
    return 1;
}

// (id, param, value: number, else string). (The game tests IsNumber twice: the int
// branch is never taken)
int __cdecl Native_MissionObjectiveUpdateParam(lua_State* L) {
    uint8_t id = uint8_t(Script::GetInt(L, 0));
    uint8_t param = uint8_t(Script::GetInt(L, 1));
    if (Script::IsNumber(L, 2) != 0) {
        Objectives_SetParamFloat(objectives, id, param, Script::GetFloat(L, 2));
        return 0;
    }
    if (Script::IsNumber(L, 2) != 0) {
        Objectives_SetParamInt(objectives, id, param, Script::GetInt(L, 2));
        return 0;
    }
    if (Script::IsString(L, 2) != 0) {
        Objectives_SetParamString(objectives, id, param, Script::GetString(L, 2));
    }
    return 0;
}

// (hour, minute, second): the timer runs to that time of day. (In 16 bits: past times
// wrap with 2 * target + 0x5180 - now, the day's 86400 truncated)
int __cdecl Native_MissionTimerStartFixed(lua_State* L) {
    uint16_t hours = uint16_t(Script::GetInt(L, 0));
    uint16_t minutes = uint16_t(Script::GetInt(L, 1));
    uint16_t seconds = uint16_t(Script::GetInt(L, 2));
    uint16_t now = uint16_t((uint16_t(g_clockHours * 60 + g_clockMinutes)) * 60 + g_clockSeconds);
    uint16_t target = uint16_t(seconds + (minutes + hours * 60) * 60);
    uint16_t left;
    if (target < now) {
        left = uint16_t(target * 2 + 0x5180 - now);
    } else {
        left = uint16_t(target - now);
    }
    MissionTimer_Stop(missionTimer, 1);
    MissionTimer_Start(missionTimer, left);
    return 0;
}

// 0x5C25A0: the handles of the objects and projectiles (not attached, +0x78) in a box
// into the list. (The z test is (zmin <= z || z <= zmax): any z passes)
uint8_t __cdecl FindObjectsInArea(HandleList* list, float x, float y, float z, float rx, float ry, float rz) {
    if (list->data != nullptr) {
        M_Free(list->data);
        list->data = nullptr;
    }
    list->capacity = 0;
    list->count = 0;
    float x1 = x - rx;
    float y1 = y - ry;
    float z1 = z - rz;
    float x2 = x + rx;
    float y2 = y + ry;
    float z2 = z + rz;
    uint8_t found = 0;
    for (int pass = 0; pass < 2; pass++) {
        CPool<uint8_t>* pool = pass == 0 ? m_objectPool : m_projectilePool;
        int n = pool->size;
        if (pool->Next() != nullptr) {
            n += Pool_ChainSize(pool->Next());
        }
        for (int i = 0; i < n; i++) {
            pool = pass == 0 ? m_objectPool : m_projectilePool;
            uint8_t* object = nullptr;
            if (i < pool->size) {
                object = pool->objects + pool->elemSize * i;
            } else if (pool->Next() != nullptr) {
                object = Pool_ChainAt(pool->Next(), i - pool->size, 1);
            }
            if ((pool->flags[i] & 0x80) != 0 || object == nullptr || At<int32_t>(object, 0x78) != 0) {
                continue;
            }
            if (Entity_IsInArea2D(object, x1, y1, x2, y2) == 0) {
                continue;
            }
            float oz = EntityPos(object)[2];
            if (!(z1 > oz) || !(oz > z2)) {
                uint16_t handle = Object_GetHandle16(object);
                if (list->count == list->capacity) {
                    uint32_t capacity = list->capacity != 0 ? list->capacity * 2 : 1;
                    if (pass == 0) {
                        List_Reserve(list, capacity, 0);
                    } else {
                        // (inlined here)
                        uint32_t* data = nullptr;
                        list->capacity = capacity;
                        if (capacity != 0) {
                            uint64_t bytes = uint64_t(capacity) * 4;
                            data = static_cast<uint32_t*>(M_Malloc(bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : uint32_t(bytes)));
                        }
                        if (list->data != nullptr) {
                            for (uint32_t k = 0; k < list->count; k++) {
                                data[k] = list->data[k];
                            }
                            M_Free(list->data);
                        }
                        list->data = data;
                    }
                }
                list->data[list->count] = handle;
                list->count++;
                found = 1;
            }
        }
    }
    return found;
}

// 0x5C28D0: an entity by (handle, kind); a ped's vehicle when in one (none at all
// with noVehicle); an object in a cutscene from the cutscene's list when asked
uint8_t* __cdecl GetAreaEntity(int handle, int kind, int noVehicle, int cutscene) {
    uint8_t* ped;
    switch (kind) {
    case 0:
        if (g_cutsceneRunning != 0 && re::Flag(cutscene)) {
            return g_cutsceneObjects[handle];
        }
        return ObjectPool_GetAtRef(m_objectPool, handle);
    case 1:
        return MM_GetVehicle(handle);
    case 2:
        ped = M_GetPedFromHandle(handle, 2);
        break;
    case 3:
        ped = m_playerPed;
        break;
    default:
        return nullptr;
    }
    if (ped == nullptr || re::Flag(noVehicle)) {
        return nullptr;
    }
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    return vehicle != nullptr ? vehicle : ped;
}

// 0x5C2970: the position of an entity by (handle, kind) (a ped's vehicle's when in
// one; an unknown kind reads through null)
float* __cdecl GetAreaEntityPos(float* out, int handle, int kind) {
    uint8_t* e = EntityOrVehicle(handle, kind);
    float* p = EntityPos(e);
    out[0] = p[0];
    out[1] = p[1];
    out[2] = p[2];
    return out;
}

// 0x5C2A10: deletes the object of a (16-bit) handle: its slot's, flags unchecked
void __cdecl DeleteObjectByHandle(uint16_t handle) {
    int slot = handle >> 8;
    CPool<uint8_t>* pool = m_objectPool;
    uint8_t* object;
    if (slot < pool->size) {
        object = pool->objects + pool->elemSize * slot;
    } else {
        if (pool->Next() == nullptr) {
            return;
        }
        object = Pool_ChainAt(pool->Next(), slot - pool->size, 1);
    }
    if (object != nullptr) {
        World_Remove(object);
        Entity_45E690(object);
        VCall<void>(object, 0, 1);
    }
}

// 0x5C2A70: an entity by (handle, kind) is in the box (x, y[, z]) +- (rx, ry[, rz]);
// with a marker drawn (0x52A600, `rotation` a vector) when a10 or a12 asks
uint8_t __cdecl IsEntityInArea(int is3D, int handle, int kind, float x, float y, float z, float rx, float ry,
                               float rz, int a10, void* rotation, int a12, int a13, int a14, int a15, int a16) {
    uint8_t* e = EntityOrVehicle(handle, kind);
    float x1 = x - rx;
    float y1 = y - ry;
    float x2 = x + rx;
    float y2 = y + ry;
    uint8_t in;
    if (re::Flag(is3D)) {
        float z2 = z + rz;
        float z1 = z - rz;
        in = Entity_IsInArea3D(e, x1, y1, z1, x2, y2, z2);
    } else {
        in = Entity_IsInArea2D(e, x1, y1, x2, y2);
    }
    if (a10 != 0 || a12 != -1) {
        float centre[3] = {x, y, z};
        DebugDrawArea_52A600(a10, a12, a14, centre, 2.0f, rotation, a13, 0, 0x72, 0xBC, a15, a16, 5);
    }
    return in == 1;
}

} // namespace

#define MISSIONS_NATIVE(name, address)                                                 \
    static decltype(&Native_##name) Original_MN_##name;                                  \
    REVERSED("Native_" #name, address, RE_BODY(&Native_##name), nullptr, \
             reinterpret_cast<void**>(&Original_MN_##name), re::Mode::Original)
#define MISSIONS_REVERSED(name, address, fn)                                        \
    static void* RE_CONCAT(Original_MN_, address);                                  \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &RE_CONCAT(Original_MN_, address), \
             re::Mode::Original)

MISSIONS_REVERSED("ScriptNatives::ModelNamesAdd_5B8F10", 0x005B8F10, ModelNames_Add);
MISSIONS_REVERSED("ScriptNatives::ModelNamesLookup_5B8F30", 0x005B8F30, ModelNames_Lookup);
MISSIONS_REVERSED("ClothingItem::Construct_5BA870", 0x005BA870, ClothingItem_Construct);
MISSIONS_REVERSED("Shop::GetSelected_5BA890", 0x005BA890, Shop_GetSelected);
MISSIONS_REVERSED("Nis::IsSet_5BB380", 0x005BB380, Nis_IsSet);
MISSIONS_REVERSED("Nis::Clear_5BB390", 0x005BB390, Nis_Clear);
MISSIONS_NATIVE(HasBallTouchedSocketRadius, 0x005BCD70);
MISSIONS_NATIVE(CreatePersistentEntity, 0x005BD1F0);
MISSIONS_REVERSED("Routes::GetStart_5BFE10", 0x005BFE10, Route_GetStart);
MISSIONS_NATIVE(MissionFail, 0x005C0060);
MISSIONS_NATIVE(MissionSucceed, 0x005C0160);
MISSIONS_NATIVE(StartMission, 0x005C0230);
MISSIONS_NATIVE(ForceStartMission, 0x005C0280);
MISSIONS_NATIVE(ForceStartMissionIndex, 0x005C0300);
MISSIONS_NATIVE(ForceMissionAvailable, 0x005C03A0);
MISSIONS_NATIVE(GetMissionAttemptCount, 0x005C0470);
MISSIONS_NATIVE(GetMissionSuccessCount, 0x005C04D0);
MISSIONS_NATIVE(SetMissionSuccessCount, 0x005C0530);
MISSIONS_NATIVE(MissionSuccessCountInc, 0x005C0610);
MISSIONS_NATIVE(MissionAttemptCountInc, 0x005C0680);
MISSIONS_NATIVE(IsMissionCompleated, 0x005C06E0);
MISSIONS_NATIVE(IsMissionAvailable, 0x005C0740);
MISSIONS_NATIVE(MissionForceCompleted, 0x005C08A0);
MISSIONS_NATIVE(LoadAnimationGroup, 0x005C0A00);
MISSIONS_NATIVE(UnLoadAnimationGroup, 0x005C0A80);
MISSIONS_NATIVE(PhotoSetValid, 0x005C0C20);
MISSIONS_NATIVE(DetonateProjectiles, 0x005C0DD0);
MISSIONS_NATIVE(PedModelCreateGiftRequirement, 0x005C0EA0);
MISSIONS_NATIVE(MissionAllowConcurrentMissions, 0x005C1090);
MISSIONS_NATIVE(SetFactionRespect, 0x005C1290);
MISSIONS_REVERSED("ScriptNatives::ClearDodgeballsAndProjectiles_5C14B0", 0x005C14B0, ClearDodgeballsAndProjectiles);
MISSIONS_NATIVE(FireCreate, 0x005C14C0);
MISSIONS_REVERSED("ScriptNatives::GetEntityByKind_5C15D0", 0x005C15D0, GetEntityByKind);
MISSIONS_REVERSED("ScriptNatives::GetKindPoolSize_5C1680", 0x005C1680, GetKindPoolSize);
MISSIONS_REVERSED("ScriptNatives::IsPhotoTargetVisible_5C16E0", 0x005C16E0, IsPhotoTargetVisible);
MISSIONS_NATIVE(PhotoTargetInFrame, 0x005C1820);
MISSIONS_NATIVE(PhotoGetEntityNext, 0x005C19B0);
MISSIONS_NATIVE(SetupPedObject, 0x005C1BA0);
MISSIONS_NATIVE(CreateProjectile, 0x005C1D70);
MISSIONS_NATIVE(MissionObjectiveAdd, 0x005C1F40);
MISSIONS_NATIVE(MissionObjectiveUpdateParam, 0x005C1FC0);
MISSIONS_NATIVE(MissionTimerStartFixed, 0x005C2150);
MISSIONS_REVERSED("ScriptNatives::FindObjectsInArea_5C25A0", 0x005C25A0, FindObjectsInArea);
MISSIONS_REVERSED("ScriptNatives::GetAreaEntity_5C28D0", 0x005C28D0, GetAreaEntity);
MISSIONS_REVERSED("ScriptNatives::GetAreaEntityPos_5C2970", 0x005C2970, GetAreaEntityPos);
MISSIONS_REVERSED("ScriptNatives::DeleteObjectByHandle_5C2A10", 0x005C2A10, DeleteObjectByHandle);
MISSIONS_REVERSED("ScriptNatives::IsEntityInArea_5C2A70", 0x005C2A70, IsEntityInArea);
