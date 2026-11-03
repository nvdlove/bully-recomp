// Triggers (0x6D6510..0x6D7CA0): Trigger, a perimeter (its shape) placed and rotated in
// the world with a height, maybe a door (a prop anim, or the door manager's index) and
// area population / ambient vehicle infos; and the TriggerManager (0x20C7E98), which
// keeps the (trigger, entity) pairs inside script triggers and loads / removes an
// area's triggers.
//
// Trigger (0x40): +0 vtable 0x94327C, +4 the position, +0x10 a vector, +0x1C the door
// prop anim, +0x20 the perimeter (shape), +0x24 CAreaPopulationInfo (0x94), +0x28 the
// ambient vehicle info (0x74), +0x2C the name hash (the door manager's index), +0x30
// the script id, +0x34 the height scale (top = perimeter +8 * this + z; bottom = z -
// perimeter +0xC), +0x38 the heading (radians), +0x3C a byte, +0x3D the area, +0x3E a
// tag, +0x3F flags (1, 2, 4 "players only", 8 a door name).
// TriggerManager: +4 100 pairs (trigger, entity) inside, count +0x324; +0x328 10 pairs
// entered this frame, count +0x378.

#include <cstdint>

#include "game/Pool.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using AnyPool = CPool<uint8_t>;

using re::At;

struct CVec {
    float x, y, z;
};

GAME_VAR(AnyPool*, tr_triggerPool, 0x020C7C48);
GAME_VAR(AnyPool*, tr_perimeterPool, 0x020C7C4C);
GAME_VAR(int32_t, tr_scriptTriggerCount0, 0x00CFB188);
GAME_VAR(int32_t, tr_scriptTriggerCount1, 0x00CFB2AC);
GAME_VAR(void*, tr_pedPopulation, 0x00C2C108);
inline void* const tr_pedExcluder = reinterpret_cast<void*>(0x00C2AB70);
inline void* const tr_doors = reinterpret_cast<void*>(0x00C3B808);
inline void* const tr_soundBank = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const tr_scripts = reinterpret_cast<void*>(0x00D02850);
inline void* const tr_triggers = reinterpret_cast<void*>(0x020C7E98);
constexpr uintptr_t VT_Trigger = 0x0094327C;
constexpr uintptr_t VT_IArchivable = 0x00900CDC;
constexpr uintptr_t VT_TriggerManager = 0x00943A44;
constexpr uintptr_t VT_AreaPopulationInfo = 0x00911DD8;
constexpr uintptr_t VT_AreaAmbientVehicleInfo = 0x00900D1C;
// the field names
GAME_VAR(const char*, tr_nName, 0x00BC5EA0);
GAME_VAR(const char*, tr_nByte3C, 0x00BC5EA8);
GAME_VAR(const char*, tr_nFlag2, 0x00BC5EB0);
GAME_VAR(const char*, tr_nPosition, 0x00BC5EB4);
GAME_VAR(const char*, tr_nVector, 0x00BC5EB8);
GAME_VAR(const char*, tr_nHeading, 0x00BC5EBC);
GAME_VAR(const char*, tr_nScale, 0x00BC5EC0);
GAME_VAR(const char*, tr_nFlag1, 0x00BC5EC4);
GAME_VAR(const char*, tr_nHasPopulationA, 0x00BC5EC8);
GAME_VAR(const char*, tr_nHasPopulationB, 0x00BC5ECC);
GAME_VAR(const char*, tr_nPopulation, 0x00BC5ED0);
GAME_VAR(const char*, tr_nUnknown18, 0x00BC5ED4);
GAME_VAR(const char*, tr_nUnusedByte, 0x00BC5ED8);
GAME_VAR(const char*, tr_nHasVehiclesA, 0x00BC5EDC);
GAME_VAR(const char*, tr_nHasVehiclesB, 0x00BC5EE0);
GAME_VAR(const char*, tr_nVehicles, 0x00BC5EE4);
GAME_VAR(const char*, tr_nTriggerCount, 0x00BC5EEC);
GAME_VAR(const char*, tr_nPerimeter, 0x00BC5EF0);
GAME_VAR(const char*, tr_nTrigger, 0x00BC5EF4);
GAME_VAR(uint8_t, tr_currentArea, 0x020C7E94);

GAME_FN(uint8_t, __thiscall, TR_Door_IsLocked, 0x004EA3A0, void* door);
GAME_FN(void, __thiscall, TR_Door_SetLocked, 0x004EA3B0, void* door, int locked);
GAME_FN(uint8_t, __thiscall, TR_Doors_GetState0, 0x004E99A0, void* doors, int32_t index);
GAME_FN(void, __thiscall, TR_Doors_SetState0, 0x004E99F0, void* doors, int32_t index, int value);
GAME_FN(uint8_t, __thiscall, TR_Doors_GetState1, 0x004E9A70, void* doors, int32_t index);
GAME_FN(void, __thiscall, TR_Doors_SetState1, 0x004E9AC0, void* doors, int32_t index, int value);
GAME_FN(uint8_t, __thiscall, TR_Doors_GetState2, 0x004E9B40, void* doors, int32_t index);
GAME_FN(void, __thiscall, TR_Doors_SetState2, 0x004E9B90, void* doors, int32_t index, int value);
GAME_FN(void, __thiscall, TR_PropAnim_Open, 0x004EED10, void* prop, int, uintptr_t, int);
GAME_FN(void, __thiscall, TR_PropAnim_Close, 0x004EEEC0, void* prop, uintptr_t);
GAME_FN(void, __cdecl, TR_Door_SetPathableToPeds, 0x005EB060, void* door, int pathable);
GAME_FN(void, __thiscall, TR_AreaPopulationInfo_Copy, 0x006D6880, void* info, const void* from);
GAME_FN(void*, __cdecl, TR_AreaPopulationInfo_New, 0x004A0FA0, uintptr_t size);
GAME_FN(void, __thiscall, TR_PopulationInfo_Construct, 0x004A0C70, void* info);
GAME_FN(void, __thiscall, TR_PopulationInfo_Destruct, 0x004A0CA0, void* info);
GAME_FN(void*, __cdecl, TR_AmbientVehicleInfo_New, 0x004A8280, uint32_t size);
GAME_FN(void, __thiscall, TR_AmbientVehicleInfo_Construct, 0x004A7FD0, void* info);
GAME_FN(void, __thiscall, TR_AmbientVehicleInfo_Destruct, 0x004A8000, void* info);
GAME_FN(void, __thiscall, TR_ModelsInfo_Copy, 0x004646D0, void* info, const void* from);
GAME_FN(void, __thiscall, TR_Triggers_RemoveTrigger, 0x006D74D0, void* manager, void* trigger);
GAME_FN(void, __cdecl, TR_Fn5B5A30, 0x005B5A30, void* trigger);
GAME_FN(void*, __thiscall, TR_TriggerPool_New, 0x007305B0, AnyPool* pool);
GAME_FN(int32_t, __cdecl, TR_HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t, __cdecl, TR_IsDoorName, 0x006D67A0, const char* name);
GAME_FN(uint8_t, __thiscall, TR_Archive_ReadSection, 0x005B0720, void* archive, void* loader, const char* name);
GAME_FN(uint8_t, __thiscall, TR_Perimeter_IsPointInside, 0x006D4D00, void* perimeter, const float* point);
GAME_FN(uint8_t, __thiscall, TR_Perimeter_IsWithinDistance, 0x006D4EA0, void* perimeter, const float* point, float distance);
GAME_FN(uint8_t, __thiscall, TR_Perimeter_KeepWithin, 0x006D50C0, void* perimeter, float* position, float margin);
GAME_FN(float, __thiscall, TR_Perimeter_GetMaxDistance, 0x006D53D0, void* perimeter, const float* position);
GAME_FN(float*, __stdcall, TR_Vector2_Zero, 0x006D4CE0, float* v);
GAME_FN(void*, __cdecl, TR_Perimeter_New, 0x006D4CF0, uint32_t size);
GAME_FN(void*, __thiscall, TR_Perimeter_Construct, 0x006D4B30, void* perimeter, int32_t id);
GAME_FN(CVec*, __thiscall, TR_Entity_GetPosition, 0x00415460, void* entity);
GAME_FN(uint8_t, __cdecl, TR_ScriptTriggers_IsActive, 0x005B53E0, int index, int kind);
GAME_FN(uint8_t, __cdecl, TR_ScriptTriggers_IsInstant, 0x005B55A0, int index, int kind);
GAME_FN(uint8_t*, __cdecl, TR_ScriptTriggers_Get, 0x005B5360, int index, int kind);
GAME_FN(void, __cdecl, TR_ScriptTriggers_OnInside, 0x005B22A0, void* trigger, void* entity);
GAME_FN(void, __cdecl, TR_ScriptTriggers_OnEnter, 0x005B2810, void* trigger, void* entity);
GAME_FN(void, __cdecl, TR_ScriptTriggers_OnExit, 0x005B28D0, void* trigger, void* entity);
GAME_FN(void, __cdecl, TR_ScriptTriggers_OnEnterInstant, 0x005B20D0, void* trigger, void* entity);
GAME_FN(void, __thiscall, TR_FixedStringRegistry_Construct, 0x004113D0, void* registry);
GAME_FN(void, __cdecl, TR_Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t*, __thiscall, TR_Scripts_Current, 0x005DB960, void* scripts);
GAME_FN(void, __thiscall, TR_PedPopulation_AddTrigger, 0x0049DB50, void* population, void* trigger);
GAME_FN(void, __thiscall, TR_PedPopulation_RemoveTrigger, 0x0049DB70, void* population, void* trigger);
GAME_FN(void, __thiscall, TR_PedExcluder_AddZone, 0x0046ED20, void* excluder, void* trigger);
GAME_FN(void, __thiscall, TR_PedExcluder_RemoveZone, 0x0046ED70, void* excluder, void* trigger);
GAME_FN(void, __thiscall, TR_SoundBank_SetTrigger, 0x00595370, void* bank, int32_t hash, int32_t index);
GAME_FN(void, __thiscall, TR_Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(void, __cdecl, TR_World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, TR_Fn45E690, 0x0045E690, void* entity);
// (this file's)
GAME_FN(CVec*, __thiscall, TR_WorldToLocal, 0x006D6710, void* trigger, CVec* out, const CVec* world);
GAME_FN(CVec*, __thiscall, TR_LocalToWorld, 0x006D66A0, void* trigger, CVec* out, const CVec* local);
GAME_FN(float*, __thiscall, TR_LocalToWorld2D, 0x006D6980, void* trigger, float* out, const float* local);
GAME_FN(float*, __thiscall, TR_WorldToLocal2D, 0x006D69F0, void* trigger, float* out, const float* world);
GAME_FN(uint8_t, __thiscall, TR_Trigger_Contains, 0x006D6FA0, void* trigger, const CVec* point);
GAME_FN(uint8_t, __thiscall, TR_Trigger_IsWithinDistance2D, 0x006D6EE0, void* trigger, const float* point, float distance);
GAME_FN(void, __thiscall, TR_Trigger_FreeAreaInfo, 0x006D6510, void* trigger);
GAME_FN(void*, __thiscall, TR_Trigger_SetPopulation, 0x006D6AF0, void* trigger, const void* info);
GAME_FN(void, __thiscall, TR_Trigger_SetVehicles, 0x006D6B40, void* trigger, const void* info);
GAME_FN(void*, __cdecl, TR_Trigger_New, 0x006D6AE0, uint32_t size);
GAME_FN(void*, __thiscall, TR_Trigger_Construct, 0x006D6A70, void* trigger, int32_t id);

uint8_t* PoolSlot(AnyPool* pool, int slot) {
    if ((pool->flags[slot] & 0x80) != 0) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * slot;
}

void PoolRelease(AnyPool* pool, const void* object) {
    int slot = int32_t(reinterpret_cast<uintptr_t>(object) - re::PtrCast<uintptr_t>(pool->objects)) / pool->elemSize;
    pool->flags[slot] |= 0x80;
    if (slot < At<int32_t>(pool, 0x14)) {
        At<int32_t>(pool, 0x14) = slot;
    }
}

// a (trigger, entity) pair list: `count` at `countOffset`, the pairs at `pairsOffset`
struct Pair {   // (the game's layout: 8)
    re::Ptr32<void> trigger;
    re::Ptr32<void> entity;
};
static_assert(sizeof(Pair) == 8);
Pair* Pairs(uint8_t* manager, int offset) {
    return reinterpret_cast<Pair*>(manager + offset);
}

// removes the pair at `index` (the rest shifted down)
void RemovePairAt(uint8_t* manager, int pairsOffset, int countOffset, uint32_t index) {
    Pair* pairs = Pairs(manager, pairsOffset);
    uint32_t& count = At<uint32_t>(manager, countOffset);
    for (uint32_t j = index; j < count - 1; j++) {
        pairs[j] = pairs[j + 1];
    }
    count--;
}

// removes every pair whose trigger (field 0) or entity (1) is `value`
void RemovePairsWith(uint8_t* manager, int pairsOffset, int countOffset, int field, void* value) {
    uint32_t i = 0;
    while (i < At<uint32_t>(manager, countOffset)) {
        Pair& pair = Pairs(manager, pairsOffset)[i];
        if ((field == 0 ? pair.trigger : pair.entity) == value) {
            RemovePairAt(manager, pairsOffset, countOffset, i);
        } else {
            i++;
        }
    }
}

int32_t CurrentScriptId() {
    uint8_t* script = TR_Scripts_Current(tr_scripts);
    return script != nullptr ? At<int32_t>(script, 0x44) : -2;
}

// a trigger (and its door prop) removed from the world and deleted
void DeleteTrigger(uint8_t* trigger, bool releaseSound) {
    if (At<void*>(trigger, 0x24) != nullptr) {
        TR_PedPopulation_RemoveTrigger(tr_pedPopulation, trigger);
    }
    TR_Trigger_FreeAreaInfo(trigger);
    uint8_t* prop = At<uint8_t*>(trigger, 0x1C);
    if (prop != nullptr) {
        TR_Entity_DeleteRwObject(prop);
        vc::Call<void>(prop, 0xC);
        TR_World_Remove(prop);
        TR_Fn45E690(prop);
        vc::Call<void>(prop, 0, 1);
        At<void*>(trigger, 0x1C) = nullptr;
    }
    TR_PedExcluder_RemoveZone(tr_pedExcluder, trigger);
    if (releaseSound) {
        TR_SoundBank_SetTrigger(tr_soundBank, At<int32_t>(trigger, 0x2C), -1);
    }
    vc::Call<void>(trigger, 0, 1);
}

} // namespace

#define TRIGGERS_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// ---------------------------------------------------------------------------------------
// Trigger

// 0x6D6510 (thiscall): its population and vehicle infos deleted
void __fastcall Trigger_FreeAreaInfo(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x24) != nullptr) {
        vc::Call<void>(At<void*>(self, 0x24), 0, 1);
        At<void*>(self, 0x24) = nullptr;
    }
    if (At<void*>(self, 0x28) != nullptr) {
        vc::Call<void>(At<void*>(self, 0x28), 0, 1);
        At<void*>(self, 0x28) = nullptr;
    }
}
TRIGGERS_REVERSED("Trigger::FreeAreaInfo", 0x006D6510, &Trigger_FreeAreaInfo);

// 0x6D6550 (thiscall): the door's locked (its prop, or the door manager's state 0)
uint8_t __fastcall Trigger_IsDoorLocked(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x1C) != nullptr) {
        return TR_Door_IsLocked(At<void*>(self, 0x1C));
    }
    return TR_Doors_GetState0(tr_doors, At<int32_t>(self, 0x2C));
}
TRIGGERS_REVERSED("Trigger::IsDoorLocked", 0x006D6550, &Trigger_IsDoorLocked);

// 0x6D6570 (thiscall)
void __fastcall Trigger_SetDoorLocked(uint8_t* self, re::Edx, int locked) {
    if (At<void*>(self, 0x1C) != nullptr) {
        TR_Door_SetLocked(At<void*>(self, 0x1C), locked);
        return;
    }
    TR_Doors_SetState0(tr_doors, At<int32_t>(self, 0x2C), locked);
}
TRIGGERS_REVERSED("Trigger::SetDoorLocked", 0x006D6570, &Trigger_SetDoorLocked);

// 0x6D65A0 (thiscall): the door's state 1 (the prop's +0x180)
uint8_t __fastcall Trigger_GetDoorState1(uint8_t* self, re::Edx) {
    uint8_t* prop = At<uint8_t*>(self, 0x1C);
    if (prop != nullptr) {
        return At<uint32_t>(prop, 0x180) != 0 ? 1 : 0;
    }
    return TR_Doors_GetState1(tr_doors, At<int32_t>(self, 0x2C));
}
TRIGGERS_REVERSED("Trigger::GetDoorState1", 0x006D65A0, &Trigger_GetDoorState1);

// 0x6D65D0 (thiscall): (with a prop: not pathable to peds while set)
void __fastcall Trigger_SetDoorState1(uint8_t* self, re::Edx, int value) {
    uint8_t* prop = At<uint8_t*>(self, 0x1C);
    if (prop != nullptr) {
        uint8_t flag = uint8_t(value);
        At<uint32_t>(prop, 0x180) = flag;
        TR_Door_SetPathableToPeds(At<void*>(self, 0x1C), flag == 0 ? 1 : 0);
        return;
    }
    TR_Doors_SetState1(tr_doors, At<int32_t>(self, 0x2C), value);
}
TRIGGERS_REVERSED("Trigger::SetDoorState1", 0x006D65D0, &Trigger_SetDoorState1);

// 0x6D6620 (thiscall): the door's open (the prop's +0x184)
uint8_t __fastcall Trigger_IsDoorOpen(uint8_t* self, re::Edx) {
    uint8_t* prop = At<uint8_t*>(self, 0x1C);
    if (prop != nullptr) {
        return At<uint32_t>(prop, 0x184) != 0 ? 1 : 0;
    }
    return TR_Doors_GetState2(tr_doors, At<int32_t>(self, 0x2C));
}
TRIGGERS_REVERSED("Trigger::IsDoorOpen", 0x006D6620, &Trigger_IsDoorOpen);

// 0x6D6650 (thiscall)
void __fastcall Trigger_SetDoorOpen(uint8_t* self, re::Edx, int open) {
    void* prop = At<void*>(self, 0x1C);
    if (prop != nullptr) {
        if (re::Flag(open)) {
            TR_PropAnim_Open(prop, 0, 0, 0);
        } else {
            TR_PropAnim_Close(prop, 0);
        }
        return;
    }
    TR_Doors_SetState2(tr_doors, At<int32_t>(self, 0x2C), open);
}
TRIGGERS_REVERSED("Trigger::SetDoorOpen", 0x006D6650, &Trigger_SetDoorOpen);

// 0x6D66A0 (thiscall): a local point to the world (turned by the heading, moved to the
// position; z kept)
CVec* __fastcall Trigger_LocalToWorld(uint8_t* self, re::Edx, CVec* out, const CVec* local) {
    float heading = At<float>(self, 0x38);
    float s = trk::CrtSin(heading);
    float c = trk::CrtCos(heading);
    out->x = (local->x * c + local->y * s) + At<float>(self, 4);
    out->y = (c * local->y - s * local->x) + At<float>(self, 8);
    out->z = local->z;
    return out;
}
TRIGGERS_REVERSED("Trigger::LocalToWorld", 0x006D66A0, &Trigger_LocalToWorld);

// 0x6D6710 (thiscall): a world point to local (z kept)
CVec* __fastcall Trigger_WorldToLocal(uint8_t* self, re::Edx, CVec* out, const CVec* world) {
    float dx = world->x - At<float>(self, 4);
    float dy = world->y - At<float>(self, 8);
    float heading = -At<float>(self, 0x38);
    float s = trk::CrtSin(heading);
    float c = trk::CrtCos(heading);
    out->x = dx * c + dy * s;
    out->y = dy * c - s * dx;
    out->z = world->z;
    return out;
}
TRIGGERS_REVERSED("Trigger::WorldToLocal", 0x006D6710, &Trigger_WorldToLocal);

// 0x6D67A0 (cdecl): a door's name ("DT_..." or "_DT_...", longer than 3)
uint8_t __cdecl Trigger_IsDoorName(const char* name) {
    uint32_t length = 0;
    while (name[length] != 0) {
        length++;
    }
    if (length <= 3) {
        return 0;
    }
    if (name[0] == 'D' && name[1] == 'T' && name[2] == '_') {
        return 1;
    }
    if (name[0] == '_' && name[1] == 'D' && name[2] == 'T' && name[3] == '_') {
        return 1;
    }
    return 0;
}
TRIGGERS_REVERSED("Trigger::IsDoorName", 0x006D67A0, &Trigger_IsDoorName);

// 0x6D6800 (thiscall): the top's height
float __fastcall Trigger_GetTop(uint8_t* self, re::Edx) {
    return At<float>(At<uint8_t*>(self, 0x20), 8) * At<float>(self, 0x34) + At<float>(self, 0xC);
}
TRIGGERS_REVERSED("Trigger::GetTop", 0x006D6800, &Trigger_GetTop);

// 0x6D6820 (thiscall): the destructor
void __fastcall Trigger_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = VT_Trigger;
    At<void*>(self, 0x20) = nullptr;
    if (At<void*>(self, 0x24) != nullptr) {
        vc::Call<void>(At<void*>(self, 0x24), 0, 1);
        At<void*>(self, 0x24) = nullptr;
    }
    if (At<void*>(self, 0x28) != nullptr) {
        vc::Call<void>(At<void*>(self, 0x28), 0, 1);
        At<void*>(self, 0x28) = nullptr;
    }
    TR_Triggers_RemoveTrigger(tr_triggers, self);
    TR_Fn5B5A30(self);
    At<uint32_t>(self, 0) = VT_IArchivable;
}
TRIGGERS_REVERSED("Trigger::~Trigger", 0x006D6820, &Trigger_Destruct);

// 0x6D6880 (thiscall): CAreaPopulationInfo's copy (its 4 population infos: the models
// and two ints)
uint8_t* __fastcall AreaPopulationInfo_Assign(uint8_t* self, re::Edx, const uint8_t* from) {
    for (int i = 0; i < 4; i++) {
        int base = 4 + i * 0x24;
        TR_ModelsInfo_Copy(self + base, from + base);
        At<uint32_t>(self, base + 0x1C) = At<uint32_t>(const_cast<uint8_t*>(from), base + 0x1C);
        At<uint32_t>(self, base + 0x20) = At<uint32_t>(const_cast<uint8_t*>(from), base + 0x20);
    }
    return self;
}
TRIGGERS_REVERSED("CAreaPopulationInfo::operator=", 0x006D6880, &AreaPopulationInfo_Assign);

// 0x6D68D0 (thiscall): keeps a position inside the trigger: z between its bottom and
// top, then within `margin` of its perimeter (in its frame)
uint8_t __fastcall Trigger_KeepWithin(uint8_t* self, re::Edx, CVec* position, float margin) {
    uint8_t* shape = At<uint8_t*>(self, 0x20);
    float bottom = At<float>(self, 0xC) - At<float>(shape, 0xC);
    float top = At<float>(shape, 8) * At<float>(self, 0x34) + At<float>(self, 0xC);
    float z = position->z;
    if (bottom > z) {
        z = bottom;
    } else if (top < z) {
        z = top;
    }
    position->z = z;
    CVec local;
    *position = *TR_WorldToLocal(self, &local, position);
    TR_Perimeter_KeepWithin(At<void*>(self, 0x20), reinterpret_cast<float*>(position), margin);
    *position = *TR_LocalToWorld(self, &local, position);
    return 1;
}
TRIGGERS_REVERSED("Trigger::KeepWithin", 0x006D68D0, &Trigger_KeepWithin);

// 0x6D6980 (thiscall): a local 2D point to the world
float* __fastcall Trigger_LocalToWorld2D(uint8_t* self, re::Edx, float* out, const float* local) {
    float heading = At<float>(self, 0x38);
    float s = trk::CrtSin(heading);
    float c = trk::CrtCos(heading);
    out[0] = (local[0] * c + local[1] * s) + At<float>(self, 4);
    out[1] = (c * local[1] - s * local[0]) + At<float>(self, 8);
    return out;
}
TRIGGERS_REVERSED("Trigger::LocalToWorld2D", 0x006D6980, &Trigger_LocalToWorld2D);

// 0x6D69F0 (thiscall): a world 2D point to local
float* __fastcall Trigger_WorldToLocal2D(uint8_t* self, re::Edx, float* out, const float* world) {
    float dx = world[0] - At<float>(self, 4);
    float dy = world[1] - At<float>(self, 8);
    float heading = -At<float>(self, 0x38);
    float c = trk::CrtCos(heading);
    float s = trk::CrtSin(heading);
    out[0] = dy * s + dx * c;
    out[1] = c * dy - s * dx;
    return out;
}
TRIGGERS_REVERSED("Trigger::WorldToLocal2D", 0x006D69F0, &Trigger_WorldToLocal2D);

// 0x6D6A70 (thiscall): the constructor
uint8_t* __fastcall Trigger_Construct(uint8_t* self, re::Edx, int32_t id) {
    At<uint32_t>(self, 0) = VT_Trigger;
    for (int i = 0; i < 6; i++) {
        At<float>(self, 4 + i * 4) = 0.0f;
    }
    At<uint8_t>(self, 0x3F) = uint8_t((At<uint8_t>(self, 0x3F) & 0xF3) | 0x10);
    At<int32_t>(self, 0x30) = id;
    At<uint8_t>(self, 0x3E) = 0xFF;
    if (At<void*>(self, 0x24) != nullptr) {
        vc::Call<void>(At<void*>(self, 0x24), 0, 1);
        At<void*>(self, 0x24) = nullptr;
    }
    if (At<void*>(self, 0x28) != nullptr) {
        vc::Call<void>(At<void*>(self, 0x28), 0, 1);
        At<void*>(self, 0x28) = nullptr;
    }
    At<void*>(self, 0x1C) = nullptr;
    return self;
}
TRIGGERS_REVERSED("Trigger::Trigger", 0x006D6A70, &Trigger_Construct);

// 0x6D6AE0 (cdecl): operator new (the trigger pool)
void* __cdecl Trigger_New(uint32_t) {
    return TR_TriggerPool_New(tr_triggerPool);
}
TRIGGERS_REVERSED("Trigger::operator new", 0x006D6AE0, &Trigger_New);

// 0x6D6AF0 (thiscall): its population info: a new one copied from `from`
void* __fastcall Trigger_SetPopulation(uint8_t* self, re::Edx, const void* from) {
    uint8_t* info = static_cast<uint8_t*>(TR_AreaPopulationInfo_New(0x94));
    if (info != nullptr) {
        At<uint32_t>(info, 0) = VT_AreaPopulationInfo;
        for (int i = 0; i < 4; i++) {
            TR_PopulationInfo_Construct(info + 4 + i * 0x24);
        }
    }
    At<void*>(self, 0x24) = info;
    return reinterpret_cast<void*(__thiscall*)(void*, const void*)>(0x006D6880)(info, from);
}
TRIGGERS_REVERSED("Trigger::SetPopulation", 0x006D6AF0, &Trigger_SetPopulation);

// 0x6D6B40 (thiscall): its ambient vehicle info: a new one copied from `from`
void __fastcall Trigger_SetVehicles(uint8_t* self, re::Edx, const uint8_t* from) {
    uint8_t* info = static_cast<uint8_t*>(TR_AmbientVehicleInfo_New(0x74));
    if (info != nullptr) {
        At<uint32_t>(info, 0) = VT_AreaAmbientVehicleInfo;
        for (int i = 0; i < 4; i++) {
            TR_AmbientVehicleInfo_Construct(info + 4 + i * 0x1C);
        }
    }
    At<void*>(self, 0x28) = info;
    for (int i = 0; i < 4; i++) {
        TR_ModelsInfo_Copy(info + 4 + i * 0x1C, from + 4 + i * 0x1C);
    }
}
TRIGGERS_REVERSED("Trigger::SetVehicles", 0x006D6B40, &Trigger_SetVehicles);

// 0x6D6BB0 (thiscall, slot 1): loads the trigger (its heading in degrees, wrapped once
// into 0..360)
void __fastcall Trigger_Load(uint8_t* self, re::Edx, void* archive) {
    struct {
        re::Ptr32<char> ptr;
        int32_t size;
        char buffer[0x24];
    } name;
    name.ptr = name.buffer;
    name.size = 0x21;
    name.buffer[0] = 0;
    vc::Call<void>(archive, 0x24, &name, 0x20, tr_nName);
    At<uint8_t>(self, 0x3D) = tr_currentArea;
    At<int32_t>(self, 0x2C) = TR_HashName(name.ptr);
    uint8_t door = TR_IsDoorName(name.ptr);
    At<uint8_t>(self, 0x3F) ^= uint8_t((uint8_t(door << 3) ^ At<uint8_t>(self, 0x3F)) & 8);
    int32_t value;
    vc::Call<uint8_t>(archive, 0x38, &value, tr_nByte3C);
    At<uint8_t>(self, 0x3C) = uint8_t(value);
    vc::Call<void>(archive, 0x20, self + 4, tr_nPosition);
    vc::Call<void>(archive, 0x20, self + 0x10, tr_nVector);
    float heading;
    vc::Call<void>(archive, 0x28, &heading, tr_nHeading);
    if (heading < 0.0f) {
        heading = float(double(heading) + 360.0);
    } else if (360.0f < heading) {
        heading = float(double(heading) - 360.0);
    }
    // (degrees to radians with the float pi, through double constants)
    heading = float(float(double(heading) * 3.1415927410125732) / 180.0);
    At<float>(self, 0x38) = heading;
    float scale;
    vc::Call<void>(archive, 0x28, &scale, tr_nScale);
    At<float>(self, 0x34) = scale;
    uint8_t flag;
    vc::Call<void>(archive, 0x3C, &flag, tr_nFlag1);
    At<uint8_t>(self, 0x3F) ^= uint8_t((At<uint8_t>(self, 0x3F) ^ flag) & 1);
    vc::Call<void>(archive, 0x3C, &flag, tr_nFlag2);
    At<uint8_t>(self, 0x3F) ^= uint8_t((uint8_t(flag + flag) ^ At<uint8_t>(self, 0x3F)) & 2);
    uint8_t unused18[0x10];
    vc::Call<void>(archive, 0x18, unused18, tr_nUnknown18);
    uint8_t unusedByte;
    vc::Call<void>(archive, 0x3C, &unusedByte, tr_nUnusedByte);
    uint8_t populationA = 0;
    uint8_t populationB = 0;
    vc::Call<void>(archive, 0x3C, &populationA, tr_nHasPopulationA);
    vc::Call<void>(archive, 0x3C, &populationB, tr_nHasPopulationB);
    uint8_t info[0x94];
    if (populationA != 0 || populationB != 0) {
        At<uint32_t>(info, 0) = VT_AreaPopulationInfo;
        for (int i = 0; i < 4; i++) {
            TR_PopulationInfo_Construct(info + 4 + i * 0x24);
        }
        TR_Archive_ReadSection(archive, info, tr_nPopulation);
        TR_Trigger_SetPopulation(self, info);
        for (int i = 3; i >= 0; i--) {
            TR_PopulationInfo_Destruct(info + 4 + i * 0x24);
        }
        At<uint32_t>(info, 0) = VT_IArchivable;
    }
    uint8_t vehiclesA = 0;
    uint8_t vehiclesB = 0;
    vc::Call<void>(archive, 0x3C, &vehiclesA, tr_nHasVehiclesA);
    vc::Call<void>(archive, 0x3C, &vehiclesB, tr_nHasVehiclesB);
    if (vehiclesA != 0 || vehiclesB != 0) {
        At<uint32_t>(info, 0) = VT_AreaAmbientVehicleInfo;
        for (int i = 0; i < 4; i++) {
            TR_AmbientVehicleInfo_Construct(info + 4 + i * 0x1C);
        }
        TR_Archive_ReadSection(archive, info, tr_nVehicles);
        TR_Trigger_SetVehicles(self, info);
        for (int i = 3; i >= 0; i--) {
            TR_AmbientVehicleInfo_Destruct(info + 4 + i * 0x1C);
        }
    }
}
TRIGGERS_REVERSED("Trigger::Load", 0x006D6BB0, &Trigger_Load);

// 0x6D6EB0 (thiscall): the 2D point's inside the perimeter
uint8_t __fastcall Trigger_ContainsXY(uint8_t* self, re::Edx, const float* point) {
    float local[2];
    return TR_Perimeter_IsPointInside(At<void*>(self, 0x20), TR_WorldToLocal2D(self, local, point));
}
TRIGGERS_REVERSED("Trigger::ContainsXY", 0x006D6EB0, &Trigger_ContainsXY);

// 0x6D6EE0 (thiscall): the 2D point's within `distance` of the perimeter
uint8_t __fastcall Trigger_IsWithinDistance2D(uint8_t* self, re::Edx, const float* point, float distance) {
    float local[2];
    return TR_Perimeter_IsWithinDistance(At<void*>(self, 0x20), TR_WorldToLocal2D(self, local, point), distance);
}
TRIGGERS_REVERSED("Trigger::IsWithinDistance2D", 0x006D6EE0, &Trigger_IsWithinDistance2D);

// 0x6D6F10 (thiscall): its origin in the world (2D)
float* __fastcall Trigger_GetCentre(uint8_t* self, re::Edx, float* out) {
    float local[2];
    return TR_LocalToWorld2D(self, out, TR_Vector2_Zero(local));
}
TRIGGERS_REVERSED("Trigger::GetCentre", 0x006D6F10, &Trigger_GetCentre);

// 0x6D6F40 (thiscall): the perimeter's farthest point from a 2D point
float __fastcall Trigger_GetMaxDistance(uint8_t* self, re::Edx, const float* point) {
    float local[2];
    return TR_Perimeter_GetMaxDistance(At<void*>(self, 0x20), TR_WorldToLocal2D(self, local, point));
}
TRIGGERS_REVERSED("Trigger::GetMaxDistance", 0x006D6F40, &Trigger_GetMaxDistance);

// 0x6D6F70 (cdecl): operator delete
void __cdecl Trigger_Delete(void* trigger) {
    PoolRelease(tr_triggerPool, trigger);
}
TRIGGERS_REVERSED("Trigger::operator delete", 0x006D6F70, &Trigger_Delete);

// 0x6D6FA0 (thiscall): the point's inside: between the bottom and top, and inside the
// perimeter
uint32_t __fastcall Trigger_Contains(uint8_t* self, re::Edx, const CVec* point) {
    uint8_t* shape = At<uint8_t*>(self, 0x20);
    if (shape == nullptr) {
        return 0;
    }
    if (!(At<float>(self, 0xC) - At<float>(shape, 0xC) < point->z)) {
        return 0;
    }
    if (!(At<float>(shape, 8) * At<float>(self, 0x34) + At<float>(self, 0xC) > point->z)) {
        return 0;
    }
    float flat[2] = {point->x, point->y};
    float local[2];
    return TR_Perimeter_IsPointInside(shape, TR_WorldToLocal2D(self, local, flat)) != 0 ? 1 : 0;
}
TRIGGERS_REVERSED("Trigger::Contains", 0x006D6FA0, &Trigger_Contains);

// 0x6D7020 (thiscall): between the bottom and top, and within `distance` of the
// perimeter
uint8_t __fastcall Trigger_IsWithinDistance(uint8_t* self, re::Edx, const CVec* point, float distance) {
    uint8_t* shape = At<uint8_t*>(self, 0x20);
    if (shape == nullptr) {
        return 0;
    }
    if (!(At<float>(self, 0xC) - At<float>(shape, 0xC) < point->z)) {
        return 0;
    }
    if (!(At<float>(shape, 8) * At<float>(self, 0x34) + At<float>(self, 0xC) > point->z)) {
        return 0;
    }
    float flat[2] = {point->x, point->y};
    return TR_Trigger_IsWithinDistance2D(self, flat, distance) != 0 ? 1 : 0;
}
TRIGGERS_REVERSED("Trigger::IsWithinDistance", 0x006D7020, &Trigger_IsWithinDistance);

// 0x6D70A0 (cdecl): the trigger pool's size (in ax; eax's top half is the pool pointer's)
uint32_t __cdecl Triggers_GetPoolSize() {
    uintptr_t pool = re::PtrCast<uintptr_t>(tr_triggerPool);
    return (uint32_t(pool) & 0xFFFF0000) | *reinterpret_cast<uint16_t*>(pool + 8);
}
TRIGGERS_REVERSED("Triggers::GetPoolSize", 0x006D70A0, &Triggers_GetPoolSize);

// ---------------------------------------------------------------------------------------
// TriggerManager (0x20C7E98)

namespace {
// one kind of script trigger for an entity: entering / staying / leaving
void UpdateScriptTriggers(uint8_t* self, uint8_t* entity, int kind, int32_t& countVar) {
    int32_t count = countVar;
    for (int32_t i = 0; i < count; i++) {
        if (TR_ScriptTriggers_IsActive(i, kind) == 0) {
            count = countVar;
            continue;
        }
        uint8_t* trigger = TR_ScriptTriggers_Get(i, kind);
        if ((At<uint8_t>(trigger, 0x3F) & 4) != 0 && At<int32_t>(entity, 0x1310) != 0xD) {
            continue;   // (the count not re-read)
        }
        uint8_t inside = TR_Trigger_Contains(trigger, TR_Entity_GetPosition(entity));
        uint32_t n = At<uint32_t>(self, 0x324);
        Pair* pairs = Pairs(self, 4);
        uint32_t j = 0;
        if (inside != 0) {
            for (; j < n; j++) {
                if (pairs[j].trigger == trigger && pairs[j].entity == entity) {
                    TR_ScriptTriggers_OnInside(trigger, entity);
                    break;
                }
            }
            if (j >= At<uint32_t>(self, 0x324) && At<uint32_t>(self, 0x324) != 100) {
                TR_ScriptTriggers_OnEnter(trigger, entity);
                TR_ScriptTriggers_OnInside(trigger, entity);
                uint32_t at = At<uint32_t>(self, 0x324);
                pairs[at].trigger = trigger;
                pairs[at].entity = entity;
                At<uint32_t>(self, 0x324) += 1;
            }
        } else {
            for (; j < n; j++) {
                if (pairs[j].trigger == trigger && pairs[j].entity == entity) {
                    TR_ScriptTriggers_OnExit(trigger, entity);
                    RemovePairAt(self, 4, 0x324, j);
                    break;
                }
            }
        }
        count = countVar;
    }
}

// one kind of the "instant" script triggers for an entity: entering this frame
void UpdateInstantTriggers(uint8_t* self, uint8_t* entity, int kind, int32_t& countVar) {
    for (int32_t i = 0; i < countVar; i++) {
        if (TR_ScriptTriggers_IsInstant(i, kind) == 0) {
            continue;
        }
        uint8_t* trigger = TR_ScriptTriggers_Get(i, kind);
        if (TR_Trigger_Contains(trigger, TR_Entity_GetPosition(entity)) == 0) {
            continue;
        }
        uint32_t n = At<uint32_t>(self, 0x378);
        Pair* pairs = Pairs(self, 0x328);
        uint32_t j = 0;
        while (j < n && !(pairs[j].trigger == trigger && pairs[j].entity == entity)) {
            j++;
        }
        if (j >= n && n != 10) {
            TR_ScriptTriggers_OnEnterInstant(trigger, entity);
            uint32_t at = At<uint32_t>(self, 0x378);
            pairs[at].trigger = trigger;
            pairs[at].entity = entity;
            At<uint32_t>(self, 0x378) += 1;
        }
    }
}
} // namespace

// 0x6D70B0 (thiscall): an entity against the script triggers (players only for flag 4
// ones: type 0xD)
void __fastcall TriggerManager_UpdateEntity(uint8_t* self, re::Edx, uint8_t* entity) {
    UpdateScriptTriggers(self, entity, 0, tr_scriptTriggerCount0);
    UpdateScriptTriggers(self, entity, 1, tr_scriptTriggerCount1);
}
TRIGGERS_REVERSED("TriggerManager::UpdateEntity", 0x006D70B0, &TriggerManager_UpdateEntity);

// 0x6D7370 (thiscall): an entity against the instant script triggers
void __fastcall TriggerManager_UpdateEntityInstant(uint8_t* self, re::Edx, uint8_t* entity) {
    UpdateInstantTriggers(self, entity, 0, tr_scriptTriggerCount0);
    UpdateInstantTriggers(self, entity, 1, tr_scriptTriggerCount1);
}
TRIGGERS_REVERSED("TriggerManager::UpdateEntityInstant", 0x006D7370, &TriggerManager_UpdateEntityInstant);

// 0x6D74D0 (thiscall): a trigger's pairs forgotten
void __fastcall TriggerManager_RemoveTrigger(uint8_t* self, re::Edx, void* trigger) {
    RemovePairsWith(self, 4, 0x324, 0, trigger);
    RemovePairsWith(self, 0x328, 0x378, 0, trigger);
}
TRIGGERS_REVERSED("TriggerManager::RemoveTrigger", 0x006D74D0, &TriggerManager_RemoveTrigger);

// 0x6D75A0 (thiscall): an entity's "inside" pairs forgotten
void __fastcall TriggerManager_RemoveEntity(uint8_t* self, re::Edx, void* entity) {
    RemovePairsWith(self, 4, 0x324, 1, entity);
}
TRIGGERS_REVERSED("TriggerManager::RemoveEntity", 0x006D75A0, &TriggerManager_RemoveEntity);

// 0x6D7610 (thiscall): an entity's "entered" pairs forgotten
void __fastcall TriggerManager_RemoveEntityInstant(uint8_t* self, re::Edx, void* entity) {
    RemovePairsWith(self, 0x328, 0x378, 1, entity);
}
TRIGGERS_REVERSED("TriggerManager::RemoveEntityInstant", 0x006D7610, &TriggerManager_RemoveEntityInstant);

// 0x6D7680 (thiscall): the constructor
uint8_t* __fastcall TriggerManager_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = VT_TriggerManager;
    TR_FixedStringRegistry_Construct(self + 4);
    for (int offset = 0x328; offset <= 0x378; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    return self;
}
TRIGGERS_REVERSED("TriggerManager::TriggerManager", 0x006D7680, &TriggerManager_Construct);

// 0x6D7720 (thiscall): the deleting destructor
void* __fastcall TriggerManager_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = VT_TriggerManager;
    if ((flags & 1) != 0) {
        TR_Free(self);
    }
    return self;
}
TRIGGERS_REVERSED("TriggerManager::DeletingDestructor", 0x006D7720, &TriggerManager_DeletingDestructor);

// 0x6D7740 (thiscall; ecx unused): deletes an area's triggers and perimeters (the
// current script's, or all)
void __fastcall TriggerManager_RemoveArea(void*, re::Edx, int32_t area, int all) {
    AnyPool* pool = tr_triggerPool;
    AnyPool* perimeters = tr_perimeterPool;
    int32_t size = pool->size;
    int32_t perimeterCount = perimeters->size;
    int32_t id = CurrentScriptId();
    for (int32_t slot = 0; slot < size; slot++) {
        uint8_t* trigger = PoolSlot(pool, slot);
        if (trigger == nullptr || At<int8_t>(trigger, 0x3D) != area) {
            continue;
        }
        if (At<int32_t>(trigger, 0x30) != id && !re::Flag(all)) {
            continue;
        }
        DeleteTrigger(trigger, true);
    }
    for (int32_t slot = 0; slot < perimeterCount; slot++) {
        uint8_t* perimeter = PoolSlot(perimeters, slot);
        if (perimeter == nullptr || At<int8_t>(perimeter, 0x14) != area) {
            continue;
        }
        if (At<int32_t>(perimeter, 0x18) == id || re::Flag(all)) {
            vc::Call<void>(perimeter, 0, 1);
        }
    }
}
TRIGGERS_REVERSED("TriggerManager::RemoveArea", 0x006D7740, &TriggerManager_RemoveArea);

// 0x6D78B0 (thiscall; ecx unused): deletes the triggers (from slot 1) and perimeters
// with a tag
void __fastcall TriggerManager_RemoveTagged(void*, re::Edx, int tag) {
    AnyPool* pool = tr_triggerPool;
    int32_t size = pool->size;
    AnyPool* perimeters = tr_perimeterPool;
    int32_t perimeterCount = perimeters->size;
    for (int32_t slot = 1; slot < size; slot++) {
        uint8_t* trigger = PoolSlot(pool, slot);
        if (trigger == nullptr || At<uint8_t>(trigger, 0x3E) != uint8_t(tag)) {
            continue;
        }
        DeleteTrigger(trigger, false);
    }
    for (int32_t slot = 0; slot < perimeterCount; slot++) {
        uint8_t* perimeter = PoolSlot(perimeters, slot);
        if (perimeter != nullptr && At<uint8_t>(perimeter, 0x1C) == uint8_t(tag)) {
            vc::Call<void>(perimeter, 0, 1);
        }
    }
}
TRIGGERS_REVERSED("TriggerManager::RemoveTagged", 0x006D78B0, &TriggerManager_RemoveTagged);

namespace {
int32_t FindTriggerByHash(int32_t hash) {
    AnyPool* pool = tr_triggerPool;
    int32_t size = pool->size;
    for (int32_t slot = 0; slot < size; slot++) {
        uint8_t* trigger = PoolSlot(pool, slot);
        if (trigger != nullptr && At<int32_t>(trigger, 0x2C) == hash) {
            return slot;
        }
    }
    return -1;
}
} // namespace

// 0x6D79C0 (thiscall; ecx unused): a trigger's slot by hash (-1: none)
int32_t __fastcall TriggerManager_FindByHash(void*, re::Edx, int32_t hash) {
    return FindTriggerByHash(hash);
}
TRIGGERS_REVERSED("TriggerManager::FindByHash", 0x006D79C0, &TriggerManager_FindByHash);

// 0x6D7A10 (thiscall; ecx unused): a trigger's slot by name (-1: none)
int32_t __fastcall TriggerManager_FindByName(void*, re::Edx, const char* name) {
    return FindTriggerByHash(TR_HashName(name));
}
TRIGGERS_REVERSED("TriggerManager::FindByName", 0x006D7A10, &TriggerManager_FindByName);

// 0x6D7A60 (thiscall; ecx unused): a trigger's slot (-1: none; a free slot compares as
// null)
int32_t __fastcall TriggerManager_FindIndex(void*, re::Edx, void* trigger) {
    AnyPool* pool = tr_triggerPool;
    int32_t size = pool->size;
    for (int32_t slot = 0; slot < size; slot++) {
        if (trigger == PoolSlot(pool, slot)) {
            return slot;
        }
    }
    return -1;
}
TRIGGERS_REVERSED("TriggerManager::FindIndex", 0x006D7A60, &TriggerManager_FindIndex);

// 0x6D7AB0 (thiscall; ecx unused): the point's in one of an area's triggers (from slot 1)
uint8_t __fastcall TriggerManager_IsInAreaTrigger(void*, re::Edx, int area, const CVec* point) {
    AnyPool* pool = tr_triggerPool;
    int32_t size = pool->size;
    for (int32_t slot = 1; slot < size; slot++) {
        uint8_t* trigger = PoolSlot(pool, slot);
        if (trigger != nullptr && At<uint8_t>(trigger, 0x3D) == uint8_t(area) &&
            TR_Trigger_Contains(trigger, point) != 0) {
            return 1;
        }
    }
    return 0;
}
TRIGGERS_REVERSED("TriggerManager::IsInAreaTrigger", 0x006D7AB0, &TriggerManager_IsInAreaTrigger);

// 0x6D7B10 (thiscall; ecx unused): a trigger's door prop by slot
void* __fastcall TriggerManager_GetDoorProp(void*, re::Edx, int32_t slot) {
    if (slot < 0) {
        return nullptr;
    }
    uint8_t* trigger = PoolSlot(tr_triggerPool, slot);
    if (trigger == nullptr) {
        return nullptr;
    }
    return At<void*>(trigger, 0x1C);
}
TRIGGERS_REVERSED("TriggerManager::GetDoorProp", 0x006D7B10, &TriggerManager_GetDoorProp);

// 0x6D7B40 (thiscall; ecx unused): loads `count` perimeters, each with its triggers (the
// current script's); a perimeter without triggers is deleted
void __fastcall TriggerManager_Load(void*, re::Edx, void* archive, int32_t count, int) {
    int32_t id = CurrentScriptId();
    for (int32_t i = count; i > 0; i--) {
        void* memory = TR_Perimeter_New(0x30);
        uint8_t* perimeter = memory != nullptr ? static_cast<uint8_t*>(TR_Perimeter_Construct(memory, id)) : nullptr;
        TR_Archive_ReadSection(archive, perimeter, tr_nPerimeter);
        int32_t triggers;
        if (vc::Call<uint8_t>(archive, 0x38, &triggers, tr_nTriggerCount) == 0) {
            triggers = 0;
        }
        if (triggers == 0) {
            if (perimeter != nullptr) {
                vc::Call<void>(perimeter, 0, 1);
            }
            perimeter = nullptr;
            continue;
        }
        for (int32_t j = triggers; j > 0; j--) {
            void* triggerMemory = TR_Trigger_New(0x40);
            uint8_t* trigger =
                triggerMemory != nullptr ? static_cast<uint8_t*>(TR_Trigger_Construct(triggerMemory, id)) : nullptr;
            TR_Archive_ReadSection(archive, trigger, tr_nTrigger);
            At<uint8_t*>(trigger, 0x20) = perimeter;
            if (At<void*>(trigger, 0x24) != nullptr) {
                TR_PedPopulation_AddTrigger(tr_pedPopulation, trigger);
            }
            TR_PedExcluder_AddZone(tr_pedExcluder, trigger);
            int32_t hash = At<int32_t>(trigger, 0x2C);
            TR_SoundBank_SetTrigger(tr_soundBank, hash, FindTriggerByHash(hash));
        }
    }
}
TRIGGERS_REVERSED("TriggerManager::Load", 0x006D7B40, &TriggerManager_Load);
