// CPedPopulation (PedPopulation.h; functions.csv calls it CVehicle) 0x49FCF0 .. 0x4A0B60:
// making the ambient peds - near the player, at the ambient spawners' locations, at
// the POI groups, and the ones remembered in the spawn ring (area transitions) - and
// the population's frame that drives it all.
// Fields the header leaves unnamed, as used here: +0x7820 how many peds there are,
// +0x7824 how many of them count against their type's list, +0x7888 how often the
// forced model was tried, +0x7894 the last frame's result (0 ok, -7 the ped pool full,
// -8 the forced model not loaded, -100 running).

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>

#include "game/Game.h"
#include "game/PedPopulation.h"
#include "game/Pools.h"
#include "reversed.h"
#include "runflags.h"

#include "game/Crt.h"
namespace {

using pop::At;
using Vec3 = pop::CVector;

template <typename R, typename... A>
R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}

GAME_VAR(CPedPopulation*, ThePopulation, 0x00C2C108);
GAME_VAR(void*, TheSpawners, 0x00C2C12C);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);       // CTimer's
GAME_VAR(uint32_t, frameCounter, 0x00C1A99C);   // CTimer's
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);   // CModelInfo::ms_modelInfoPtrs (a ped's: +0x4C its type, +0x50 its sex)
GAME_VAR(Vec3, vec_BCC564, 0x00BCC564);         // the starting value of positions filled in later
GAME_VAR(uint8_t*, areaTransitions, 0x009E4F4C);   // (+0x8C4: where the last transition put the player)
GAME_VAR(uint8_t*, systemEnabled, 0x00C0F5E4);  // (+0xA4, +0xA5: both on for the POI peds)
GAME_VAR(uint8_t*, obj_BCD080, 0x00BCD080);     // (+0x1D6C, +0x1D70: times reset with the population)
GAME_VAR(int32_t, val_C2D1AC, 0x00C2D1AC);
GAME_VAR(int32_t, pedType_BF4318, 0x00BF4318);  // a ped type always made with the model below
GAME_VAR(int32_t, model_BF4174, 0x00BF4174);
GAME_VAR(int32_t, pedType_BF4130, 0x00BF4130);  // a ped type made with the flag below set
GAME_VAR(uint8_t, flag_C2C7F4, 0x00C2C7F4);
GAME_VAR(uint8_t, flag_BD09EC, 0x00BD09EC);     // (with the area below: a second area whose dummies count)
GAME_VAR(int8_t, area_BD09EB, 0x00BD09EB);
GAME_VAR(uint8_t, pedProductionOff, 0x00C2C10C);
GAME_VAR(uint8_t, ms_bInFrontEnd, 0x020C5BE6);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const TheTimeSlots = reinterpret_cast<void*>(0x020C35E8);   // (0x6A8CC0: the time of day's slot)

// the population's own (PedPopulation*.cpp)
GAME_FN(uint8_t, __thiscall, Pop_CanAddPed, 0x0049E9E0, void* pop);
GAME_FN(void, __thiscall, Pop_ClearTypeBlocked, 0x0049C670, void* pop);
GAME_FN(uint8_t, __thiscall, Pop_FindSpawnPoint, 0x0049D860, void* pop, const Vec3* from, Vec3* out, int, float* value, float, float);
GAME_FN(uint8_t, __thiscall, Pop_IsInSpawnRange, 0x0049D040, void* pop, const Vec3* pos, int outOfRange, int useMin, float minDistance);
GAME_FN(void*, __thiscall, Pop_AddPedAtModel, 0x0049F6C0, void* pop, int model, const Vec3* pos, int, float value);
GAME_FN(int, __thiscall, Pop_ChoosePedType, 0x0049F610, void* pop, const Vec3* pos, uint32_t exclude);
GAME_FN(void, __thiscall, Pop_ConvertObject, 0x0049F440, void* pop, void* entity);
GAME_FN(int, __thiscall, Pop_GetTotalNumPeds, 0x0049C2F0, void* pop);
GAME_FN(int, __thiscall, Pop_GetModelForType, 0x0049C230, void* pop, int pedType, int, int);
GAME_FN(uint8_t, __thiscall, Pop_RemoveFarPed, 0x0049EB80, void* pop, int force);
GAME_FN(CPopulationInfo*, __thiscall, Pop_GetInfoAt, 0x0049EEB0, void* pop, const Vec3* pos);
GAME_FN(int, __thiscall, Pop_ChooseModelForType, 0x0049C1F0, void* pop, int pedType);
GAME_FN(uint8_t, __thiscall, Pop_IsPedPoolFull, 0x0049F400, void* pop);
GAME_FN(void, __thiscall, Pop_UpdateCurrent, 0x0049EF00, void* pop, Vec3* pos);
GAME_FN(void, __thiscall, Pop_UpdateCullDistances, 0x0049CE90, void* pop);
GAME_FN(void, __thiscall, Pop_CullObjects, 0x0049E080, void* pop, Vec3 centre);
GAME_FN(void, __thiscall, Pop_CullPeds, 0x0049F210, void* pop, Vec3 centre);
GAME_FN(void, __thiscall, Pop_UpdatePOIs, 0x0049D4C0, void* pop);
GAME_FN(uint8_t, __thiscall, Pop_UpdateForcedModel, 0x0049CDB0, void* pop);
GAME_FN(uint8_t, __thiscall, Pop_UpdateSpawnPoints, 0x0049DB90, void* pop);
GAME_FN(CPopulationInfo*, __thiscall, AreaPop_GetInfoAt, 0x0049DD10, void* area, int timeSlot, Vec3 pos);
GAME_FN(uint8_t, __thiscall, AreaPop_IsInTriggerForPedType, 0x0049DE10, void* area, int pedType, const Vec3* pos);
GAME_FN(void, __thiscall, AreaPop_DeteriorateAttitudes, 0x0049CAF0, void* area, void* ped);
GAME_FN(void, __thiscall, SpawnRing_SortByTime, 0x0049C800, void* ring);
GAME_FN(void, __thiscall, SpawnRing_49C080, 0x0049C080, void* ring);
GAME_FN(uint8_t, __thiscall, SpawnRing_49C750, 0x0049C750, void* ring);
GAME_FN(void, __thiscall, ModelList_RequestAll, 0x0049C700, void* list);
GAME_FN(uint8_t, __thiscall, ModelList_Contains, 0x0049C050, void* list, int model);
GAME_FN(void, __thiscall, ModelList_Add, 0x0049C030, void* list, int model);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, Pop_CountModel, 0x0049FDF0, void* pop, int model);
GAME_FN(void, __thiscall, Pop_UncountModel, 0x0049FE40, void* pop, int model);
GAME_FN(void, __thiscall, Pop_ConvertObjectsNear, 0x004A0280, void* pop, Vec3 pos);
GAME_FN(uint8_t, __thiscall, Pop_GenerateSpawnerPed, 0x004A0390, void* pop);
GAME_FN(uint8_t, __thiscall, Pop_GeneratePOIPeds, 0x004A04B0, void* pop);
GAME_FN(void*, __thiscall, Pop_GenerateAmbientPed, 0x004A0790, void* pop, float, float, float, float, int, int, int);
GAME_FN(void*, __thiscall, Pop_AddPedOfType, 0x004A09E0, void* pop, int pedType, Vec3 pos, int);
GAME_FN(void, __thiscall, Pop_GeneratePeds, 0x004A0B00, void* pop);
// the populations and the ped model lists
GAME_FN(int, __thiscall, PopInfo_ChoosePedType, 0x00464740, void* info);
GAME_FN(int, __thiscall, PopInfo_41A8D0, 0x0041A8D0, void* info, int pedType);
GAME_FN(int, __thiscall, PedModelsInfo_Pick, 0x00464E70, void* info, int flagA, int flagB, int);
GAME_FN(int, __thiscall, PedModelsInfo_464C50, 0x00464C50, void* info, int pedType);
GAME_FN(void, __thiscall, ModelsInfo_Used, 0x00464AB0, void* info, int model);
GAME_FN(void, __thiscall, ModelsInfo_Unused, 0x00464AF0, void* info, int model);
GAME_FN(int, __thiscall, TimeSlots_Get, 0x006A8CC0, void* slots, int);
// the spawners (SpawnerManager.cpp, Spawners.cpp)
GAME_FN(uint8_t*, __thiscall, Spawners_PickAmbientLocation, 0x004A1CE0, void* spawners);
GAME_FN(Vec3*, __thiscall, SpawnLocation_GetPosition, 0x004A1190, void* location, Vec3* out);
GAME_FN(int, __thiscall, SpawnLocation_PickModel, 0x004A11C0, void* location, void* info, int);
GAME_FN(void*, __thiscall, SpawnLocation_SpawnPed, 0x004A25D0, void* location, int model);
// a POI group's (+0x78A0)
GAME_FN(int, __thiscall, POIGroup_489700, 0x00489700, void* group);
GAME_FN(uint8_t, __thiscall, POIGroup_489870, 0x00489870, void* group, int model);
GAME_FN(uint8_t, __thiscall, POIGroup_GetPositions, 0x00489970, void* group, Vec3* pos, Vec3* target);
GAME_FN(uint8_t, __thiscall, POI_447410, 0x00447410, void* poi, void* ped, int);
// peds, streaming, the world
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);   // (+4: the load state, 1 loaded)
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(uint8_t, __cdecl, ModelInfo_IsPed, 0x0051B3F0, int model);
GAME_FN(void*, __cdecl, Ped_New, 0x004789E0, uintptr_t size);
GAME_FN(void*, __thiscall, CivilianPed_Construct, 0x0046DFF0, void* ped, int model, int, int);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041EE20, void* entity, const Vec3* pos);
GAME_FN(void, __thiscall, Placeable_SetOrientation, 0x0049F160, void* entity, const Vec3* v);
GAME_FN(float, __cdecl, FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int);
GAME_FN(uint8_t, __cdecl, IsAreaClear_48C990, 0x0048C990, const Vec3* pos, float radius, int, uintptr_t);
GAME_FN(void, __cdecl, FindNearbyEntities, 0x0045E3C0, const Vec3* pos, float radius, int, int16_t* count, int, void* out, int);
GAME_FN(Vec3*, __cdecl, VecSub, 0x00414B70, Vec3* out, const Vec3* a, const Vec3* b);   // a - b
GAME_FN(const Vec3*, __cdecl, GetPedCoors, 0x004469E0, void* ped);   // (nullptr: the player)
GAME_FN(int, __cdecl, GetRandomNumberInRange, 0x0040DA80, int min, int max);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, Fn_533030, 0x00533030, int, int);
GAME_FN(int, __thiscall, Camera_4F12F0, 0x004F12F0, void* camera);

CPedModelsInfo* PedModels(CPedPopulation* pop, int pedType) {
    return &pop->pedModels[pedType];
}

CAreaPopulationInfo* AreaHere(CPedPopulation* pop) {
    return &pop->areas[CGame::currArea];
}

// a ground height search from just above z: on the ground (a bit over it), else 0.35
// higher (the game adds its float constants as doubles)
float GroundedZ(float x, float y, float z) {
    uint8_t found = 0;
    float ground = FindGroundZ(x, y, float(z + 1.0), &found, 1);
    return found ? float(ground + double(0.01f)) : float(z + double(0.35f));
}

} // namespace

namespace population_peds {

// 0x49FCF0: a ped of pedType (of the ped type the population at pos picks) made and
// added to the world when its model is loaded; else the model is requested (nullptr)
void* __fastcall AddPedOfPedType(CPedPopulation* self, re::Edx, const Vec3* pos, int pedType) {
    CPopulationInfo* info = self->override_;
    if (info == nullptr) {
        int slot = TimeSlots_Get(TheTimeSlots, -1);
        info = AreaPop_GetInfoAt(AreaHere(self), slot, *pos);
        if (info == nullptr) {
            return nullptr;
        }
    }
    int group = PopInfo_ChoosePedType(info);
    if (group == -1 || !Pop_CanAddPed(self)) {
        return nullptr;
    }
    int model = model_BF4174;
    if (pedType != pedType_BF4318) {
        model = PedModelsInfo_464C50(PedModels(self, group), pedType);
    }
    if (model == -1) {
        return nullptr;
    }
    if (GetStreamingInfo(model)[4] != 1) {
        RequestModel(model, 0x14);
        return nullptr;
    }
    if (pedType == pedType_BF4130) {
        flag_C2C7F4 = 1;
    }
    void* memory = Ped_New(0x1E90);
    void* ped = memory ? CivilianPed_Construct(memory, model, 1, 0) : nullptr;
    flag_C2C7F4 = 0;
    World_Add(ped);   // (even a null one)
    return ped;
}

// 0x49FDF0 / 0x49FE40: a ped model counted against its type's list (or not any more)
void __fastcall CountModel(CPedPopulation* self, re::Edx, int model) {
    if (!ModelInfo_IsPed(model)) {
        return;
    }
    int pedType = At<int>(modelInfoPtrs[model], 0x4C);
    // (the game also tests the list's address against null)
    if (pedType != -1 && pedType < 12) {
        ModelsInfo_Used(PedModels(self, pedType), model);
        self->unknown7824++;
    }
}

void __fastcall UncountModel(CPedPopulation* self, re::Edx, int model) {
    if (!ModelInfo_IsPed(model)) {
        return;
    }
    int pedType = At<int>(modelInfoPtrs[model], 0x4C);
    if (pedType != -1 && pedType < 12) {
        ModelsInfo_Unused(PedModels(self, pedType), model);
        self->unknown7824--;
    }
}

// 0x49FE90: in an interior, the spawn ring's peds made again around where the area
// transition put the player (a spawn point, else 100 random tries each; 4 at most)
void __fastcall SpawnRingPedsAfterTransition(CPedPopulation* self, re::Edx) {
    Vec3 to = vec_BCC564;
    Vec3 from = *reinterpret_cast<Vec3*>(areaTransitions + 0x8C4);
    float value = 0.0f;
    int spawned = 0;
    if (CGame::currArea == 0 || self->spawnRing == nullptr) {
        return;
    }
    SpawnRing_SortByTime(self->spawnRing);
    for (int i = 0; i < self->spawnRing->size; i++) {
        int model = self->spawnRing->entries[i].model;
        if (model == -1) {
            continue;
        }
        bool place = Pop_FindSpawnPoint(self, &from, &to, 1, &value, 0.0f, 0.0f) != 0;
        for (int tries = 0; !place && tries < 100;) {
            tries++;
            to = from;
            float dx = float(Rand() / 32767.0 * 5.0 - 2.5);
            to.x = dx + from.x;
            float dy = float(Rand() / 32767.0 * 5.0 - 2.5);
            to.y = dy + from.y;
            to.z = GroundedZ(to.x, to.y, to.z);
            place = IsAreaClear_48C990(&to, 0.2f, -1, 0) != 0;
        }
        if (!place) {
            continue;
        }
        // (on the global population, not this one)
        Pop_AddPedAtModel(ThePopulation, model, &to, 1, value);
        if (++spawned >= 4) {
            i = self->spawnRing->size;
        }
    }
    SpawnRing_49C080(self->spawnRing);
}

// 0x4A00B0: a fresh start (a new area, a restart): the forced and pending models and
// the type blocks cleared, up to 3 models picked for near the player into the model
// list (the last one forced), everything listed requested. Void: the early return
// leaves eax as it was, and the one caller (0x534634) ignores it
void __fastcall ResetForArea(CPedPopulation* self, re::Edx) {
    uint8_t* player = playerPed;
    if (player == nullptr || self->modelList == nullptr) {
        return;
    }
    self->flags &= 0xF7;
    self->forcedModel = -1;
    self->forcedPedType = -1;
    self->unknown7888 = 0;
    self->pendingModel = -1;
    self->pendingTries = 0;
    self->unknown7894 = 0;
    Pop_ClearTypeBlocked(self);
    At<uint32_t>(obj_BCD080, 0x1D6C) = timeInMs + 20000;
    At<uint32_t>(obj_BCD080, 0x1D70) = timeInMs + 20000;
    if (CGame::currArea == 0) {
        Fn_533030(val_C2D1AC, 0);
    }
    float tries = float(self->modelList->count);
    if (tries > 3.0) {
        tries = 3.0f;
    }
    int count = int(tries);
    Vec3 pos = *pop::PosOf(player);
    for (; count > 0; count--) {
        int pedType = Pop_ChoosePedType(self, &pos, 15);
        if (pedType == -1) {
            continue;
        }
        int model = PedModelsInfo_Pick(PedModels(self, pedType), (self->flags >> 2) & 1, (self->flags >> 1) & 1, 0);
        if (model == -1 || !ModelInfo_IsPed(model)) {
            continue;
        }
        uint8_t* info = modelInfoPtrs[model];
        if (info == nullptr) {
            continue;
        }
        self->forcedModel = model;
        self->forcedPedType = At<int>(info, 0x4C);
        PedModelList* list = self->modelList;
        list->models[list->cursor] = model;
        list->cursor = (list->cursor + 1) % list->count;
    }
    ModelList_RequestAll(self->modelList);
    SpawnRing_49C750(self->spawnRing);   // (a tail jump in the game)
}

// 0x4A0280: this frame's 32nd of the dummy pool: those in the area within 80 of pos
// (2D) go to Pop_ConvertObject
void __fastcall ConvertObjectsNear(CPedPopulation* self, re::Edx, Vec3 pos) {
    CPool<CDummy>* pool = CPools::ms_pDummyPool;
    int part = frameCounter & 0x1F;
    int end = (part + 1) * pool->size / 32;
    for (int slot = part * pool->size / 32; slot < end; slot++) {
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* dummy = re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot;
        if (dummy == nullptr) {
            continue;
        }
        uint8_t area = At<uint8_t>(dummy, 0x111);
        if (area != CGame::currArea && area != 1 && !(flag_BD09EC && area == area_BD09EB)) {
            continue;
        }
        Vec3 d;
        VecSub(&d, pop::PosOf(dummy), &pos);
        float sq = std::fabs(float(double(d.x) * d.x + double(d.y) * d.y));
        float dist = float(std::sqrt(double(sq)));
        if (dist < 80.0f) {
            Pop_ConvertObject(self, dummy);
        }
    }
}

// 0x4A0390: a ped at one of the ambient spawners' locations (true: made, or the
// location's last ped is still about)
uint8_t __fastcall GenerateSpawnerPed(CPedPopulation* self, re::Edx) {
    if (!Pop_CanAddPed(self)) {
        return 0;
    }
    uint8_t* location = Spawners_PickAmbientLocation(TheSpawners);
    if (location == nullptr) {
        return 0;
    }
    if (At<void*>(location, 0x10) != nullptr) {
        return 1;
    }
    Vec3 pos;
    SpawnLocation_GetPosition(location, &pos);
    float minDistance = CGame::currArea == 0 ? float(self->cull[1] * 0.5) : 1.0f;
    if (!Pop_IsInSpawnRange(self, &pos, 0, 1, minDistance)) {
        return 0;
    }
    int pedType = Pop_ChoosePedType(self, &pos, 15);
    if (pedType == -1) {
        return 0;
    }
    uint8_t inTrigger = AreaPop_IsInTriggerForPedType(AreaHere(self), pedType, &pos);
    if (inTrigger && self->typeBlockedUntil[pedType] > timeInMs) {
        return 0;
    }
    int model = SpawnLocation_PickModel(location, PedModels(self, pedType), 0);
    void* ped = SpawnLocation_SpawnPed(location, model);
    if (inTrigger) {
        AreaPop_DeteriorateAttitudes(AreaHere(self), ped);
    }
    return ped != nullptr;
}

// 0x4A04B0: the POI groups, round from a random one: each ready group gets a ped at
// its place (nothing in the way) facing its target. False as soon as one can't.
uint8_t __fastcall GeneratePOIPeds(CPedPopulation* self, re::Edx) {
    if (!(self->flags & 0x10)) {
        return 0;
    }
    uint8_t spawned = 0;
    if (!systemEnabled[0xA4] || !systemEnabled[0xA5] || self->poiGroups.count == 0) {
        return 0;
    }
    uint32_t start = GetRandomNumberInRange(0, self->poiGroups.count);
    uint32_t index = start;
    do {
        uint8_t* group = self->poiGroups.groups[index];
        if (group != nullptr && !VCall<uint8_t>(group, 4)) {
            uint8_t* poi = At<uint8_t*>(group, 0x40);
            if (poi == nullptr) {
                // (not moving on: the game's loop keeps coming back to this one unless
                // it's where it started)
                continue;
            }
            int out;
            int pedType = *VCall<int*>(group, 8, &out);
            if (At<uint8_t>(poi, 0xA3)) {
                if (Pop_GetTotalNumPeds(self) > At<int8_t>(poi, 0xA1)) {
                    return 0;
                }
            } else if (!Pop_CanAddPed(self)) {
                return 0;
            }
            if (pedType < 0 || pedType >= 12) {
                if (POIGroup_489700(group) != 0) {
                    int out2;
                    pedType = *VCall<int*>(group, 8, &out2);
                } else {
                    Vec3 at = At<Vec3>(group, 0x80);
                    pedType = Pop_ChoosePedType(self, &at, 7);
                    if (pedType == -1) {
                        return 0;
                    }
                }
            }
            int model = Pop_GetModelForType(ThePopulation, pedType, At<int>(group, 0x3C), 5);
            if (model == -1 || POIGroup_489870(group, model)) {
                return 0;
            }
            Vec3 pos = vec_BCC564;
            Vec3 target = vec_BCC564;
            if (!POIGroup_GetPositions(group, &pos, &target) || !Pop_IsInSpawnRange(self, &pos, 0, 0, 10.0f)) {
                return 0;
            }
            self->nearEntities[0] = nullptr;
            self->nearEntities[1] = nullptr;
            int16_t nearCount = 0;
            FindNearbyEntities(&pos, 0.01f, 1, &nearCount, 2, self->nearEntities, 6);
            if (nearCount > 0) {
                return 0;
            }
            if (At<int>(poi, 0x6C) != 0) {
                void* ped = Pop_AddPedAtModel(self, model, &pos, 1, 0.0f);
                if (ped != nullptr && POI_447410(poi, ped, 1)) {
                    uint8_t found = 0;
                    float ground = FindGroundZ(pos.x, pos.y, float(pos.z + 1.0), &found, 1);
                    if (found) {
                        pos.z = ground;
                    }
                    Entity_SetPosition(ped, &pos);
                    Placeable_SetOrientation(ped, &target);
                    spawned = 1;
                }
            }
        }
        if (++index >= self->poiGroups.count) {
            index = 0;
        }
    } while (index != start);
    return spawned;
}

// 0x4A0790: an ambient ped at a spawn point near the player: the forced model while it
// still suits (3 tries), else a new pick. pedModel -1: picked here; checkRange -1: the
// point must be in the spawn range too. (The four cull distances passed aren't used.)
int g_generateStep = 0;   // (--doors: where the last GenerateAmbientPed stopped: 1 room, 2 a spawn point, 3 range, 4 a model, 5 made)
void* __fastcall GenerateAmbientPed(CPedPopulation* self, re::Edx, float, float, float, float, int checkRange, int pedModel,
                                    int flag) {
    g_generateStep = 1;
    if (!Pop_CanAddPed(self) && !Pop_RemoveFarPed(self, 0)) {
        return nullptr;
    }
    g_generateStep = 2;
    Vec3 pos = vec_BCC564;
    float value = 0.0f;
    // (the remaster's second pass, runflags.h: around the place the player is mapped to there)
    const float* centre = re::PopulationCentre();
    Vec3 there = centre ? Vec3{centre[0], centre[1], centre[2]} : Vec3{};
    if (!Pop_FindSpawnPoint(self, centre ? &there : pop::PosOf(playerPed), &pos, flag, &value, 0.0f, 0.0f)) {
        return nullptr;
    }
    g_generateStep = 3;
    if (checkRange == -1 && !Pop_IsInSpawnRange(self, &pos, 0, 0, 10.0f)) {
        return nullptr;
    }
    g_generateStep = 4;
    int model = pedModel;
    if (model == -1) {
        if (self->forcedModel > 0 && self->unknown7888 <= 2) {
            uint8_t* mi = modelInfoPtrs[self->forcedModel];
            // the forced model is of a sex wanted (a null model info is read anyway)
            bool suits = At<uint8_t>(mi, 0x50) ? (self->flags & 4) != 0 : (self->flags & 2) != 0;
            CPopulationInfo* info;
            if (self->forcedPedType != -1 && (info = Pop_GetInfoAt(self, &pos)) != nullptr &&
                PopInfo_41A8D0(info, self->forcedPedType) > 0 && suits) {
                model = self->forcedModel;
            } else {
                self->forcedModel = -1;
                self->forcedPedType = -1;
            }
        }
        if (self->forcedModel == -1 || self->unknown7888 > 2) {
            self->forcedModel = -1;
            self->forcedPedType = -1;
            self->unknown7888 = 0;
            int pedType = Pop_ChoosePedType(self, &pos, 15);
            if (pedType != -1) {
                model = Pop_ChooseModelForType(self, pedType);
                if (model != -1 && ModelInfo_IsPed(model)) {
                    uint8_t* mi = modelInfoPtrs[model];
                    if (mi != nullptr) {
                        self->forcedModel = model;
                        self->forcedPedType = At<int>(mi, 0x4C);
                    }
                }
            }
        }
        self->unknown7888++;
        if (model == -1) {
            return nullptr;
        }
    }
    pos.z = GroundedZ(pos.x, pos.y, pos.z);
    g_generateStep = 5;
    return Pop_AddPedAtModel(self, model, &pos, 1, value);
}

// 0x4A09E0: a ped of pedType at pos when its model is loaded; else the model goes on
// the model list and is requested (nullptr). None while the ped pool is full.
void* __fastcall AddPedOfType(CPedPopulation* self, re::Edx, int pedType, Vec3 pos, int arg) {
    if (Pop_IsPedPoolFull(self)) {
        return nullptr;
    }
    int model = PedModelsInfo_Pick(PedModels(self, pedType), (self->flags >> 2) & 1, (self->flags >> 1) & 1, 0);
    if (model == -1 || GetStreamingInfo(model)[4] == 2) {
        return nullptr;
    }
    if (GetStreamingInfo(model)[4] == 1) {
        return Pop_AddPedAtModel(self, model, &pos, arg, 0.0f);
    }
    PedModelList* list = self->modelList;
    if (!ModelList_Contains(list, model)) {
        ModelList_Add(list, model);
    }
    RequestModel(model, 0x10);
    return nullptr;
}

// 0x4A0A90 / 0x4A0AB0: a ped made or gone (counted against its type too when asked)
void __fastcall PedAdded(CPedPopulation* self, re::Edx, int model, int countType) {
    if (re::Flag(countType)) {
        Pop_CountModel(self, model);
    }
    self->unknown7820++;
}

void __fastcall PedRemoved(CPedPopulation* self, re::Edx, int model, int countType) {
    if (re::Flag(countType)) {
        Pop_UncountModel(self, model);
    }
    self->unknown7820--;
}

// 0x4A0AD0
void* __fastcall AddPedOfTypeAt(CPedPopulation* self, re::Edx, int pedType, Vec3 pos) {
    return Pop_AddPedOfType(self, pedType, pos, 4);
}

// 0x4A0B00: this frame's new peds: a spawner's, the POI groups', one near the player
void __fastcall GeneratePeds(CPedPopulation* self, re::Edx) {
    Pop_GenerateSpawnerPed(self);
    Pop_GeneratePOIPeds(self);
    int cameraFlag = Camera_4F12F0(TheCamera);
    // (the flag goes as the low byte of a dword whose upper bytes are the game's `this`)
    int flag = int(reinterpret_cast<uintptr_t>(self) & 0xFFFFFF00u) | (cameraFlag == 0 ? 1 : 0);
    Pop_GenerateAmbientPed(self, self->cull[1], self->cull[2], self->cull[4], self->cull[5], -1, -1, flag);
}

} // namespace population_peds

// ---- the remaster's two areas live at a seamless door (runflags.h) ----------------------------
// While the player is by a seamless door (or just through one), the area on the other side is
// peopled too: a second pass of the population's frame, around the place the doorway maps the
// player to there, with that area's mix (CGame::currArea made it for the pass), cull distances,
// spawn points and "in view" (CCamera made the camera through the doorway). Each ped is the pass's
// whose centre it is nearer (the areas are hundreds of metres apart): the other pass leaves it
// alone (CullPeds, RemoveFarPed), and each pass's type counts are its own peds' (the population
// keeps one set, numPeds by type: the other side's taken out for the pass, put back after it).
namespace two_areas {

GAME_FN(uint8_t*, __cdecl, TA_GetEntityInPool, 0x0044A3E0, uint32_t pool, uint32_t slot);
GAME_VAR(int32_t, TA_currArea, 0x00BD1008);

int g_pass = 0;
int g_mainStep = 0;   // (--doors: the main pass's GenerateAmbientPed step)
float g_centre[3];     // (pass 2's: the place there)
float g_ghostAt[3];    // (the two centres while there are two: which a ped is nearer)
float g_playerAt[3];

// the second pass's own of what the population keeps one of
struct GhostState {
    int area = -1;
    uint8_t spawnPoints[0xA0] = {};
    Vec3 lastFocus = {1e6f, 1e6f, 1e6f};
    int32_t numSpawnPoints = 0;
    int32_t forcedModel = -1;
    int32_t forcedPedType = -1;
    int32_t unknown7888 = 0;
};
GhostState g_ghostState;

bool NearerGhost(const uint8_t* ped) {
    const Vec3* p = pop::PosOf(const_cast<uint8_t*>(ped));
    float gx = p->x - g_ghostAt[0], gy = p->y - g_ghostAt[1];
    float px = p->x - g_playerAt[0], py = p->y - g_playerAt[1];
    return gx * gx + gy * gy < px * px + py * py;
}

// the ambient peds (+0x1570 1) nearer the place there, by their model's ped type
void CountGhostPeds(int counts[12]) {
    for (int t = 0; t < 12; t++) {
        counts[t] = 0;
    }
    uint8_t* pool = *reinterpret_cast<re::Ptr32<uint8_t>*>(0x00C0F5F0);   // (CPools::ms_pPedPool)
    if (!pool) {
        return;
    }
    int size = At<int32_t>(pool, 8);
    const uint8_t* flags = At<uint8_t*>(pool, 4);
    for (int slot = 0; slot < size; slot++) {
        uint8_t* ped = (flags[slot] & 0x80) ? nullptr : TA_GetEntityInPool(0, uint32_t(slot));
        if (!ped || At<int32_t>(ped, 0x1310) == 13 || ped[0x1570] != 1 || !NearerGhost(ped)) {
            continue;
        }
        int16_t model = At<int16_t>(ped, 0x10E);
        uint8_t* mi = model >= 0 ? modelInfoPtrs[model] : nullptr;
        int type = mi ? At<int32_t>(mi, 0x4C) : -1;
        if (type >= 0 && type < 12) {
            counts[type]++;
        }
    }
}

void SwapGhostState(CPedPopulation* self) {
    GhostState& g = g_ghostState;
    uint8_t points[0xA0];
    std::memcpy(points, self->spawnPoints, sizeof(points));
    std::memcpy(self->spawnPoints, g.spawnPoints, sizeof(points));
    std::memcpy(g.spawnPoints, points, sizeof(points));
    std::swap(self->lastFocus, g.lastFocus);
    std::swap(self->numSpawnPoints, g.numSpawnPoints);
    std::swap(self->forcedModel, g.forcedModel);
    std::swap(self->forcedPedType, g.forcedPedType);
    std::swap(self->unknown7888, g.unknown7888);
}

// the second pass: the area there culled and peopled around the place there
void GhostPass(CPedPopulation* self, int generate, int area, const float* at) {
    if (g_ghostState.area != area) {
        g_ghostState = GhostState{};
        g_ghostState.area = area;
    }
    int counts[12], saved[12];
    CountGhostPeds(counts);
    for (int t = 0; t < 12; t++) {
        saved[t] = self->pedModels[t].numPeds;
        self->pedModels[t].numPeds = counts[t];
    }
    re::Ptr32<CPopulationInfo> savedCurrent = self->current;
    float savedCull[6];
    std::memcpy(savedCull, self->cull, sizeof(savedCull));
    int32_t savedResult = self->unknown7894;
    SwapGhostState(self);
    int32_t savedArea = TA_currArea;
    TA_currArea = area;
    g_pass = 2;
    std::memcpy(g_centre, at, sizeof(g_centre));
    bool camera = re::SeamlessGhostCamera(true);
    Vec3 pos = {at[0], at[1], at[2]};
    Pop_UpdateCurrent(self, &pos);
    Pop_UpdateCullDistances(self);
    Pop_CullPeds(self, pos);
    int made = 0;
    population_peds::g_generateStep = 0;
    if (re::Flag(generate) && !pedProductionOff && !ms_bInFrontEnd && self->current && !Pop_IsPedPoolFull(self)) {
        Pop_UpdateSpawnPoints(self);
        int cameraFlag = Camera_4F12F0(TheCamera);
        int flag = int(reinterpret_cast<uintptr_t>(self) & 0xFFFFFF00u) | (cameraFlag == 0 ? 1 : 0);
        made = Pop_GenerateAmbientPed(self, self->cull[1], self->cull[2], self->cull[4], self->cull[5], -1, -1, flag) != nullptr;
    }
    if (camera) {
        re::SeamlessGhostCamera(false);
    }
    if (re::DoorReport()) {
        static uint32_t said = 0;
        int total = 0;
        for (int t = 0; t < 12; t++) {
            total += self->pedModels[t].numPeds;
        }
        if (made || timeInMs - said >= 1000) {
            said = timeInMs;
            std::fprintf(stderr,
                         "doors: second population: area %d around (%.1f, %.1f, %.1f): %d of its peds (its mix %d), %d spawn points,"
                         " cull %.0f/%.0f, camera %s%s; it stopped at step %d, result %d (the main pass: result %d, step %d; production off %d)"
                         " at %u ms\n",
                         area, at[0], at[1], at[2], total, self->current ? self->current->total : -1, self->numSpawnPoints,
                         self->cull[0], self->cull[3], camera ? "through the door" : "the main one", made ? ", one made" : "",
                         population_peds::g_generateStep, self->unknown7894, savedResult, g_mainStep, pedProductionOff, timeInMs);
        }
    }
    g_pass = 0;
    TA_currArea = savedArea;
    SwapGhostState(self);
    self->unknown7894 = savedResult;
    std::memcpy(self->cull, savedCull, sizeof(savedCull));
    self->current = savedCurrent;
    // (the main counts, with what this pass made or removed)
    for (int t = 0; t < 12; t++) {
        self->pedModels[t].numPeds = saved[t] + (self->pedModels[t].numPeds - counts[t]);
    }
}

} // namespace two_areas

int re::PopulationPass() {
    return two_areas::g_pass;
}
const float* re::PopulationCentre() {
    return two_areas::g_pass == 2 ? two_areas::g_centre : nullptr;
}
bool re::PopulationSkipsPed(const uint8_t* ped) {
    using namespace two_areas;
    if (g_pass == 0 || !ped) {
        return false;
    }
    return NearerGhost(ped) != (g_pass == 2);
}

namespace population_peds {

void MainPass(CPedPopulation* self, int generate);

// 0x4A0B60: the population's frame (CGame::Process; generate: new peds may be made). (The
// remaster's: and the second area's at a seamless door, two_areas above)
void __fastcall Update(CPedPopulation* self, re::Edx, int generate) {
    using namespace two_areas;
    int area = -1;
    float at[3];
    if (g_pass != 0 || !re::SeamlessPopulationGhost(&area, at)) {
        MainPass(self, generate);
        return;
    }
    std::memcpy(g_ghostAt, at, sizeof(g_ghostAt));
    const Vec3* player = pop::PosOf(playerPed);
    g_playerAt[0] = player->x;
    g_playerAt[1] = player->y;
    g_playerAt[2] = player->z;
    // the current area's pass, its counts without the other side's peds
    int counts[12];
    CountGhostPeds(counts);
    int taken[12];
    for (int t = 0; t < 12; t++) {
        taken[t] = std::min(counts[t], int(self->pedModels[t].numPeds));
        self->pedModels[t].numPeds -= taken[t];
    }
    g_pass = 1;
    population_peds::g_generateStep = 0;
    MainPass(self, generate);
    g_mainStep = population_peds::g_generateStep;
    g_pass = 0;
    for (int t = 0; t < 12; t++) {
        self->pedModels[t].numPeds += taken[t];
    }
    GhostPass(self, generate, area, at);
}

void MainPass(CPedPopulation* self, int generate) {
    self->unknown7894 = -100;
    const Vec3 coors = *GetPedCoors(nullptr);
    Vec3 pos = coors;
    Pop_UpdateCurrent(self, &pos);
    Pop_UpdateCullDistances(self);
    Pop_CullObjects(self, coors);
    Pop_ConvertObjectsNear(self, coors);
    Pop_CullPeds(self, coors);
    Pop_UpdatePOIs(self);
    if (!re::Flag(generate) || pedProductionOff) {
        self->unknown7894 = 0;
        return;
    }
    uint8_t full = 0;
    if (Pop_IsPedPoolFull(self)) {
        full = 1;
        self->unknown7894 = -7;
    }
    if (!Pop_UpdateForcedModel(self)) {
        self->unknown7894 = -8;
        return;
    }
    if (!full && !ms_bInFrontEnd) {
        Pop_UpdateSpawnPoints(self);
        Pop_GeneratePeds(self);
    }
}

} // namespace population_peds

#define C08P6_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P6_REVERSED("CPedPopulation::AddPedOfPedType", 0x0049FCF0, &population_peds::AddPedOfPedType);
C08P6_REVERSED("CPedPopulation::CountModel", 0x0049FDF0, &population_peds::CountModel);
C08P6_REVERSED("CPedPopulation::UncountModel", 0x0049FE40, &population_peds::UncountModel);
C08P6_REVERSED("CPedPopulation::SpawnRingPedsAfterTransition", 0x0049FE90, &population_peds::SpawnRingPedsAfterTransition);
C08P6_REVERSED("CPedPopulation::ResetForArea", 0x004A00B0, &population_peds::ResetForArea);
C08P6_REVERSED("CPedPopulation::ConvertObjectsNear", 0x004A0280, &population_peds::ConvertObjectsNear);
C08P6_REVERSED("CPedPopulation::GenerateSpawnerPed", 0x004A0390, &population_peds::GenerateSpawnerPed);
C08P6_REVERSED("CPedPopulation::GeneratePOIPeds", 0x004A04B0, &population_peds::GeneratePOIPeds);
C08P6_REVERSED("CPedPopulation::GenerateAmbientPed", 0x004A0790, &population_peds::GenerateAmbientPed);
C08P6_REVERSED("CPedPopulation::AddPedOfType", 0x004A09E0, &population_peds::AddPedOfType);
C08P6_REVERSED("CPedPopulation::PedAdded", 0x004A0A90, &population_peds::PedAdded);
C08P6_REVERSED("CPedPopulation::PedRemoved", 0x004A0AB0, &population_peds::PedRemoved);
C08P6_REVERSED("CPedPopulation::AddPedOfTypeAt", 0x004A0AD0, &population_peds::AddPedOfTypeAt);
C08P6_REVERSED("CPedPopulation::GeneratePeds", 0x004A0B00, &population_peds::GeneratePeds);
C08P6_REVERSED("CPedPopulation::Update", 0x004A0B60, &population_peds::Update);
