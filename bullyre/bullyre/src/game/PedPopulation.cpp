// The ped population: its parts, set-up, PedPop.dat, the population triggers and the
// counts by ped type (0x49C0C0 .. 0x49F610; the spawning and culling are in
// PedPopulationSpawn.cpp).

#include "game/PedPopulation.h"

#include "game/Ped.h"
#include "game/Pool.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {
GAME_VAR(CPedPopulation*, P5_population, 0x00C2C108);
GAME_VAR(int32_t, P5_currArea, 0x00BD1008);            // CGame::currArea
GAME_VAR(uint32_t, P5_timeInMs, 0x00C1A9B4);            // CTimer::m_snTimeInMilliseconds
GAME_ARRAY(uint8_t*, P5_modelInfoPtrs, 0x00C67738);     // CModelInfo::ms_modelInfoPtrs
GAME_VAR(CPool<uint8_t>*, P5_triggerPool, 0x020C7C48);  // the triggers (0x3C: their area code)
GAME_VAR(uint8_t, P5_pedProduction, 0x00C2C10C);        // (StopPedProduction's)
// the two siren models (0x49C330 takes them over any other)
GAME_VAR(int32_t, P5_sirenModelA, 0x00BF4318);
GAME_VAR(int32_t, P5_sirenModelB, 0x00BF431C);
inline void* const P5_timeSlots = reinterpret_cast<void*>(0x020C35E8);

inline re::Ptr32<void>* const P5_vtPickupObjective = reinterpret_cast<re::Ptr32<void>*>(0x00911DC4);
inline re::Ptr32<void>* const P5_vtAreaPopulationInfo = reinterpret_cast<re::Ptr32<void>*>(0x00911DD8);
inline re::Ptr32<void>* const P5_vtPedModelsInfo = reinterpret_cast<re::Ptr32<void>*>(0x00911DBC);
inline re::Ptr32<void>* const P5_vtArchivable = reinterpret_cast<re::Ptr32<void>*>(0x00900CDC);   // IArchivable
// PickupObjective's class id, and Objective's
GAME_VAR(uint32_t, P5_guardPickup, 0x00C2C124);
GAME_VAR(int16_t, P5_idPickup, 0x00C2C120);
GAME_VAR(uint32_t, P5_guardObjective, 0x00C2AFA8);
GAME_VAR(int16_t, P5_idObjective, 0x00C2AFA4);

GAME_FN(void*, __cdecl, P5_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, P5_Free, 0x005EEAB0, void* p);
inline int __cdecl P5_rand() { return crt::Rand(); }
template <typename... A>
inline int __cdecl P5_sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
// eh vector constructor / destructor iterators
GAME_FN(void, __stdcall, P5_VectorConstruct, 0x00401240, void* array, size_t size, int count, void* ctor);
GAME_FN(void, __stdcall, P5_VectorDestruct, 0x004017C0, void* array, size_t size, int count, void* dtor);
inline void* const P5_PopInfoCtor = reinterpret_cast<void*>(0x004A0C70);
inline void* const P5_PopInfoDtor = reinterpret_cast<void*>(0x004A0CA0);
inline void* const P5_TriggerArrayCtor = reinterpret_cast<void*>(0x0066BEA0);
// CFileMgr
GAME_FN(void*, __cdecl, P5_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(char*, __cdecl, P5_LoadLine, 0x00429C10, void* file);
GAME_FN(void, __cdecl, P5_CloseFile, 0x0042D450, void* file);
GAME_FN(int, __cdecl, P5_GetAreaIndex, 0x004305D0, const char* name);
// streaming
GAME_FN(uint8_t*, __cdecl, P5_GetStreamingInfo, 0x0052DD30, int model);   // +4 the load state (1 loaded, 2 requested)
GAME_FN(void, __cdecl, P5_RequestModel, 0x00532020, int model, int flags);
GAME_FN(uint8_t, __cdecl, P5_LoadModelNow, 0x00533030, int model, int);
GAME_FN(uint8_t, __cdecl, P5_IsPedModel, 0x0051B3F0, int model);
// objectives
GAME_FN(void*, __thiscall, P5_Objective_Construct, 0x0046FA90, void* self, int type, void* ped);
GAME_FN(void, __thiscall, P5_Objective_Destruct, 0x0046FAF0, void* self);
// CPopulationInfo
GAME_FN(CPopulationInfo*, __thiscall, P5_PopInfo_Construct, 0x004A0C70, void* self);
GAME_FN(void, __thiscall, P5_PopInfo_Destruct, 0x004A0CA0, CPopulationInfo* self);
GAME_FN(void, __thiscall, P5_PopInfo_Parse, 0x004645F0, CPopulationInfo* self, const char* line);
GAME_FN(void, __thiscall, P5_PopInfo_Set, 0x00464680, CPopulationInfo* self, int* counts);
// CPedModelsInfo
GAME_FN(void, __thiscall, P5_PedModels_BaseConstruct, 0x00464E00, CPedModelsInfo* self);
GAME_FN(void, __thiscall, P5_PedModels_Destruct, 0x004647A0, CPedModelsInfo* self);
GAME_FN(void, __thiscall, P5_PedModels_Setup, 0x00464E20, CPedModelsInfo* self, int pedType);
GAME_FN(void, __thiscall, P5_PedModels_Reset, 0x00464E30, CPedModelsInfo* self, int pedType);
GAME_FN(int, __thiscall, P5_PedModels_Choose, 0x00464E70, CPedModelsInfo* self, int males, int females, int);
GAME_FN(int, __thiscall, P5_PedModels_GetModel, 0x004648C0, CPedModelsInfo* self, int a, int b, int);
// the model list and ring's own
GAME_FN(PedModelList*, __thiscall, P5_ModelList_Construct, 0x0049BFD0, void* self, int size);
GAME_FN(void, __thiscall, P5_ModelList_Remove, 0x0049C030, PedModelList* self, int model);
GAME_FN(uint8_t, __thiscall, P5_ModelList_Contains, 0x0049C050, PedModelList* self, int model);
GAME_FN(void, __thiscall, P5_Ring_Clear, 0x0049C080, PedSpawnRing* self);
// peds
GAME_FN(int, __thiscall, P5_Inventory_HasWeapon, 0x0045CF40, void* inventory, int weapon);
GAME_FN(void, __thiscall, P5_Inventory_Give, 0x0045D030, void* inventory, int weapon, int, int, int);
GAME_FN(int*, __thiscall, P5_Ped_GetAttitude, 0x00475380, uint8_t* ped, int* out, int pedType);
GAME_FN(void, __thiscall, P5_Ped_SetAttitude, 0x004731D0, uint8_t* ped, int pedType, int attitude);
// triggers: whether a place is in one (in 3D; by x, y)
GAME_FN(uint8_t, __thiscall, P5_Trigger_Contains, 0x006D6FA0, uint8_t* trigger, pop::CVector* pos);
GAME_FN(uint8_t, __thiscall, P5_Trigger_Contains2D, 0x006D6EB0, uint8_t* trigger, float* pos);
GAME_FN(int, __thiscall, P5_TimeSlots_Get, 0x006A8CC0, void* self, int);
// our own
GAME_FN(PickupObjective*, __thiscall, P5_PickupObjective_Construct, 0x0049C0C0, void* self, void* ped, int pickup);
GAME_FN(void, __thiscall, P5_Area_AddTrigger, 0x0049CA20, CAreaPopulationInfo* self, uint8_t* trigger);
GAME_FN(void, __thiscall, P5_Area_RemoveTrigger, 0x0049CA80, CAreaPopulationInfo* self, uint8_t* trigger);
GAME_FN(void, __thiscall, P5_Area_SetTriggerPedType, 0x0049CBB0, CAreaPopulationInfo* self, uint8_t* trigger, int pedType);
GAME_FN(CPopulationInfo*, __thiscall, P5_Area_GetInfoAt, 0x0049DD10, CAreaPopulationInfo* self, int slot, pop::CVector pos);
GAME_FN(int, __thiscall, P5_Ring_Add, 0x0049C990, PedSpawnRing* self, int model, uint8_t* entity);
GAME_FN(void, __thiscall, P5_POIGroups_RemoveAt, 0x0049C8F0, POIGroupList* self, uint32_t index);
GAME_FN(void, __thiscall, P5_ModelList_RequestAll, 0x0049C700, PedModelList* self);
GAME_FN(CPedPopulation*, __thiscall, P5_Construct, 0x0049CBF0, void* self);
GAME_FN(void, __thiscall, P5_Destruct, 0x0049CC90, CPedPopulation* self);
GAME_FN(void, __thiscall, P5_LoadPedPop, 0x0049C3D0, CPedPopulation* self);
GAME_FN(void, __thiscall, P5_ClearTypeDisabled, 0x0049C610, CPedPopulation* self);
GAME_FN(void, __thiscall, P5_ClearTypeBlocked, 0x0049C670, CPedPopulation* self);
GAME_FN(void, __thiscall, P5_ResetPedModels, 0x0049DB10, CPedPopulation* self);
GAME_FN(int, __thiscall, P5_GetTotalNumPeds, 0x0049C2F0, CPedPopulation* self);
GAME_FN(CPopulationInfo*, __thiscall, P5_GetInfoAt, 0x0049EEB0, CPedPopulation* self, pop::CVector* pos);
GAME_FN(void, __cdecl, P5_Create, 0x0049DEC0);
GAME_FN(void, __cdecl, P5_Init, 0x0049DF80);

// vf0 of the game's classes: the deleting destructor
void DeleteObject(void* object) {
    using Fn = void*(__thiscall*)(void*, int);
    re::GameVirtual<Fn>(object, 0)(object, 1);
}

// A trigger by its index in the trigger pool (0 for a free slot)
uint8_t* TriggerAt(int index) {
    CPool<uint8_t>* pool = P5_triggerPool;
    if ((pool->flags[index] & 0x80) != 0) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * index;
}
} // namespace

// ---------------------------------------------------------------------------------
// PickupObjective

PickupObjective* PickupObjective::Construct(void* owner, int index) {
    P5_Objective_Construct(this, 0x17, owner);
    vtable = P5_vtPickupObjective;
    pickup = index;
    return this;
}

uint8_t PickupObjective::IsA(int id) {
    if (static_cast<int16_t>(id) == Objective::ClassId(P5_guardPickup, P5_idPickup)) {
        return 1;
    }
    return static_cast<int16_t>(id) == Objective::ClassId(P5_guardObjective, P5_idObjective);
}

PickupObjective* PickupObjective::Destroy(uint8_t destroyFlags) {
    vtable = P5_vtPickupObjective;
    P5_Objective_Destruct(this);
    if (destroyFlags & 1) {
        P5_Free(this);
    }
    return this;
}

// ---------------------------------------------------------------------------------
// CAreaPopulationInfo

void CAreaPopulationInfo::Destruct() {
    for (int i = 3; i >= 0; i--) {
        P5_PopInfo_Destruct(&infos[i]);
    }
    vtable = P5_vtArchivable;
}

void CAreaPopulationInfo::AddTrigger(uint8_t* trigger) {
    uint32_t n = numTriggers;
    for (uint32_t i = 0; i < n; i++) {
        if (pop::At<int32_t>(triggers[i].trigger, 0x2C) == pop::At<int32_t>(trigger, 0x2C)) {
            return;
        }
    }
    // (no check against the 40 there's room for)
    triggers[n].pedType = 0xF;
    triggers[n].trigger = trigger;
    numTriggers++;
}

void CAreaPopulationInfo::RemoveTrigger(uint8_t* trigger) {
    uint32_t n = numTriggers;
    for (uint32_t i = 0; i < n; i++) {
        if (pop::At<int32_t>(triggers[i].trigger, 0x2C) == pop::At<int32_t>(trigger, 0x2C)) {
            triggers[i] = triggers[n - 1];
            numTriggers--;
            return;
        }
    }
}

void CAreaPopulationInfo::DeteriorateAttitudes(uint8_t* ped) {
    if (!ped) {
        return;
    }
    for (int type = 0; type < 15; type++) {
        int value;
        int attitude = *P5_Ped_GetAttitude(ped, &value, type);
        if (type == pop::At<int32_t>(ped, 0x1310) || type == 0 || type == 7 || type == 8 || type == 9 ||
            attitude > 2) {
            continue;
        }
        float worse = static_cast<float>(attitude - 1);
        if (worse <= 0.0f) {
            worse = 0.0f;
        }
        P5_Ped_SetAttitude(ped, type, static_cast<int>(worse));
        pop::At<int32_t>(ped, 0x1400) = 1;
    }
}

void CAreaPopulationInfo::SetTriggerPedType(uint8_t* trigger, int pedType) {
    for (uint32_t i = 0; i < numTriggers; i++) {
        if (triggers[i].trigger == trigger) {
            triggers[i].pedType = pedType;
            return;
        }
    }
}

CPopulationInfo* CAreaPopulationInfo::GetInfoAt(int slot, pop::CVector pos) {
    uint8_t* found = nullptr;
    for (uint32_t i = 0; i < numTriggers; i++) {
        uint8_t* trigger = triggers[i].trigger;
        if (!trigger) {
            continue;
        }
        uint8_t inside;
        if ((P5_population->flags & 0x20) == 0) {
            float pos2D[2] = {pos.x, pos.y};
            inside = P5_Trigger_Contains2D(trigger, pos2D);
        } else {
            inside = P5_Trigger_Contains(trigger, &pos);
        }
        if (inside) {
            // (the game also works out the trigger's offset from the place, unused;
            // the last one it's in wins)
            found = trigger;
        }
    }
    if (found) {
        return &pop::At<CAreaPopulationInfo*>(found, 0x24)->infos[slot];
    }
    return &infos[slot];
}

uint8_t CAreaPopulationInfo::IsInTriggerForPedType(int pedType, pop::CVector* pos) {
    for (uint32_t i = 0; i < numTriggers; i++) {
        PopTriggerEntry& entry = triggers[i];
        if (!entry.trigger) {
            continue;
        }
        uint8_t inside;
        if ((P5_population->flags & 0x20) == 0) {
            float pos2D[2] = {pos->x, pos->y};
            inside = P5_Trigger_Contains2D(entry.trigger, pos2D);
        } else {
            inside = P5_Trigger_Contains(entry.trigger, pos);
        }
        if (inside && pedType == entry.pedType) {
            // (a leftover search for another ped type: it always finds one)
            for (int type = 0; type < 15; type++) {
                if (type != pedType && type != 0 && type != 7 && type != 8 && type != 9) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------------
// The model list, the spawn ring, the POI groups

void PedModelList::RequestAll() {
    for (int i = 0; i < count; i++) {
        int model = models[i];
        if (model != -1 && P5_GetStreamingInfo(model)[4] != 1) {
            P5_RequestModel(models[i], 0x10);
        }
    }
}

int PedModelList::ChooseModel(int arg) {
    int start = P5_rand() * (count - 1) / 0x7FFF;
    for (int i = start; i < count; i++) {
        int model = models[i];
        CPedPopulation* population = P5_population;
        if (!P5_IsPedModel(model)) {
            continue;
        }
        int pedType = pop::At<int32_t>(P5_modelInfoPtrs[model], 0x4C);
        if (pedType == -1 || pedType >= 12) {
            continue;
        }
        // vf0 of its ped type's CPedModelsInfo: whether it takes the model
        CPedModelsInfo* info = &population->pedModels[pedType];
        using Accepts = uint8_t(__thiscall*)(CPedModelsInfo*, int, int);
        if (re::GameVirtual<Accepts>(info, 0)(info, models[i], arg)) {
            return models[i];
        }
    }
    return -1;
}

void PedSpawnRing::SortByTime() {
    int n = size;
    for (int i = 0; n - 1 > 0 && i < n - 1; i++) {
        if (i + 1 < n) {
            int best = i;
            for (int j = i + 1; j < n; j++) {
                if (static_cast<int32_t>(entries[j].time) > static_cast<int32_t>(entries[best].time)) {
                    best = j;
                }
            }
            if (best > i) {
                PedSpawnRecord swap = entries[i];
                entries[i] = entries[best];
                entries[best] = swap;
            }
        }
        n = size;
    }
}

// 0x49C750 (thiscall): the ring kept for the entity of its first record with model 0: that
// entity's records with a model have it requested (when not loaded), every other record is
// emptied; whether there was one
static uint8_t __fastcall SpawnRing_KeepFirstEntity(PedSpawnRing* self, re::Edx) {
    uint8_t* entity = nullptr;
    for (int i = 0; i < self->size; i++) {
        if (self->entries[i].model == 0) {
            entity = self->entries[i].entity;
            break;
        }
    }
    for (int i = 0; i < self->size; i++) {
        PedSpawnRecord* record = &self->entries[i];
        int model = record->model;
        if (model > 0 && entity != nullptr && entity == record->entity) {
            if (P5_GetStreamingInfo(model)[4] != 1) {
                P5_RequestModel(self->entries[i].model, 0x10);
            }
        } else {
            record->model = -1;
            self->entries[i].entity = nullptr;
            self->entries[i].time = 0;
        }
    }
    return entity != nullptr ? 1 : 0;
}

int PedSpawnRing::Add(int model, uint8_t* entity) {
    int n = cursor;
    for (int i = 0; i < n; i++) {
        if (!entries[i].entity) {
            entries[i].model = model;
            entries[i].entity = entity;
            entries[i].time = P5_timeInMs;
            return i * static_cast<int>(sizeof(PedSpawnRecord));
        }
    }
    entries[n].model = model;
    entries[cursor].entity = entity;
    entries[cursor].time = P5_timeInMs;
    int next = cursor + 1;
    cursor = next % size;
    return next / size;
}

void POIGroupList::RemoveAt(uint32_t index) {
    for (; index < count - 1; index++) {
        groups[index] = groups[index + 1];
    }
    count--;
}

// ---------------------------------------------------------------------------------
// CPedPopulation

int CPedPopulation::ChooseModelForType(int pedType) {
    return P5_PedModels_Choose(&pedModels[pedType], (flags >> 2) & 1, (flags >> 1) & 1, 0);
}

int CPedPopulation::GetModelForType(int pedType, int a, int b) {
    if (pedType == -1) {
        pedType = P5_rand() * 11 / 0x7FFF;
    }
    return P5_PedModels_GetModel(&pedModels[pedType], a, b, 0);
}

uint8_t CPedPopulation::GiveWeapon(uint8_t* ped, int weapon) {
    if (!ped) {
        return 0;
    }
    if (P5_Inventory_HasWeapon(pop::At<void*>(ped, 0x1C4), weapon) != 0) {
        return 0;
    }
    P5_Inventory_Give(pop::At<void*>(ped, 0x1C4), weapon, 1, 1, 0);
    pop::At<int32_t>(pop::At<uint8_t*>(ped, 0x2E0), 0x18) = weapon;
    return 1;
}

int CPedPopulation::GetNumPedsOfType(int pedType) {
    return pedModels[pedType].numPeds;
}

int CPedPopulation::GetTotalNumPeds() {
    int total = 0;
    for (int i = 0; i < 12; i++) {
        total += pedModels[i].numPeds;
    }
    return total;
}

int CPedPopulation::PickPendingModel(int model) {
    int sirenA = P5_sirenModelA;
    if (pendingModel != sirenA && pendingModel != P5_sirenModelB &&
        (model == sirenA || model == P5_sirenModelB)) {
        pendingModel = model;
        pendingTries = 0;
    }
    if (pendingModel == -1 || pendingTries > 2) {
        pendingModel = -1;
        pendingTries = 0;
        if (model == -1) {
            return -1;
        }
        pendingModel = model;
    }
    if (!P5_LoadModelNow(pendingModel, 0)) {
        pendingTries++;
        return -1;
    }
    int result = pendingModel;
    pendingModel = -1;
    return result;
}

void CPedPopulation::LoadPedPop() {
    int area = -1;
    int slot = 0;
    char name[24];
    void* file = P5_OpenFile("Config\\Dat\\PedPop.dat", "r", 1);
    // the exterior cull distances (and three more)
    for (char* line = P5_LoadLine(file); line; line = P5_LoadLine(file)) {
        if (*line != '\0' && *line != '#') {
            P5_sscanf(line, "%f %f %f %f %f %f %f %i %i", &cull[0], &cull[1], &cull[2], &cull[3], &cull[4], &cull[5],
                      &interiorHeight, &unknown7874, &unknown7878);
            for (int i = 0; i < 6; i++) {
                cullExterior[i] = cull[i];
            }
            break;
        }
    }
    // the interior ones
    for (char* line = P5_LoadLine(file); line; line = P5_LoadLine(file)) {
        if (*line != '\0' && *line != '#') {
            P5_sscanf(line, "%f %f %f %f %f %f", &cullInterior[0], &cullInterior[1], &cullInterior[2],
                      &cullInterior[3], &cullInterior[4], &cullInterior[5]);
            break;
        }
    }
    // then by area: its name, and a line for each of its four populations
    for (char* line = P5_LoadLine(file); line; line = P5_LoadLine(file)) {
        if (*line == '\0' || *line == '#') {
            continue;
        }
        if (area == -1) {
            P5_sscanf(line, "%s", name);
            area = P5_GetAreaIndex(name);
        } else {
            P5_PopInfo_Parse(&areas[area].infos[slot], line);
            if (++slot > 3) {
                area = -1;
                slot = 0;
            }
        }
    }
    P5_CloseFile(file);
}

void CPedPopulation::Override(int* counts) {
    if (!override_) {
        void* memory = P5_Malloc(sizeof(CPopulationInfo));
        override_ = memory ? P5_PopInfo_Construct(memory) : nullptr;
    }
    P5_PopInfo_Set(override_, counts);
}

void CPedPopulation::SetTypeDisabled(uint32_t pedType, uint8_t disabled) {
    if (pedType < 15) {
        typeDisabled[pedType] = disabled;
    }
}

void CPedPopulation::ClearTypeDisabled() {
    for (int i = 0; i < 15; i++) {
        typeDisabled[i] = 0;
    }
}

void CPedPopulation::ClearTypeBlocked() {
    for (int i = 0; i < 15; i++) {
        typeBlockedUntil[i] = 0;
    }
}

void CPedPopulation::RevertToDefault() {
    if (override_) {
        DeleteObject(override_);
        override_ = nullptr;
    }
}

CPedPopulation* CPedPopulation::Construct() {
    for (int i = 0; i < 64; i++) {
        areas[i].vtable = P5_vtAreaPopulationInfo;
        P5_VectorConstruct(areas[i].infos, sizeof(CPopulationInfo), 4, P5_PopInfoCtor);
        P5_VectorConstruct(areas[i].triggers, sizeof(PopTriggerEntry), 40, P5_TriggerArrayCtor);
        areas[i].numTriggers = 0;
    }
    for (int i = 0; i < 12; i++) {
        P5_PedModels_BaseConstruct(&pedModels[i]);
        pedModels[i].vtable = P5_vtPedModelsInfo;
    }
    lastFocus.x = 0.0f;
    lastFocus.y = 0.0f;
    lastFocus.z = 0.0f;
    poiGroups.count = 0;
    return this;
}

void CPedPopulation::Destruct() {
    // (the override isn't cleared)
    if (override_) {
        DeleteObject(override_);
    }
    for (uint32_t n = poiGroups.count; n != 0;) {
        n--;
        if (poiGroups.groups[n]) {
            DeleteObject(poiGroups.groups[n]);
        }
    }
    poiGroups.count = 0;
    for (int i = 11; i >= 0; i--) {
        P5_PedModels_Destruct(&pedModels[i]);
    }
    for (int i = 63; i >= 0; i--) {
        areas[i].numTriggers = 0;
        P5_VectorDestruct(areas[i].infos, sizeof(CPopulationInfo), 4, P5_PopInfoDtor);
        areas[i].vtable = P5_vtArchivable;
    }
}

void CPedPopulation::Reset() {
    if (override_) {
        DeleteObject(override_);
        override_ = nullptr;
    }
    forcedModel = -1;
    forcedPedType = -1;
    pendingModel = -1;
    unknown7888 = 0;
    pendingTries = 0;
    unknown7894 = 0;
    flags = static_cast<uint8_t>((flags & 0xDE) | 0x18);
}

uint8_t CPedPopulation::UpdateForcedModel() {
    P5_ModelList_RequestAll(modelList);
    if (forcedModel <= 0 || P5_GetStreamingInfo(forcedModel)[4] == 1) {
        return 1;
    }
    if (unknown7888 == 1) {
        int model = forcedModel;
        PedModelList* list = modelList;
        if (!P5_ModelList_Contains(list, model)) {
            if (flags & 8) {
                P5_ModelList_Remove(list, model);
            } else {
                list->cursor = (list->cursor + 1) % list->count;
            }
            list = modelList;
            if (list->unknown8 % list->count == list->cursor) {
                flags ^= 8;
            }
        }
    }
    if (P5_GetStreamingInfo(forcedModel)[4] != 2) {
        P5_RequestModel(forcedModel, 0x10);
    }
    return 0;
}

void CPedPopulation::RemovePOIGroup(uint8_t* group) {
    for (uint32_t i = 0; i < poiGroups.count; i++) {
        if (poiGroups.groups[i] == group) {
            P5_POIGroups_RemoveAt(&poiGroups, i);
            return;
        }
    }
}

void CPedPopulation::ResetPedModels() {
    for (int i = 0; i < 12; i++) {
        P5_PedModels_Reset(&pedModels[i], i);
    }
    P5_ClearTypeDisabled(this);
    P5_pedProduction = 0;
}

void CPedPopulation::AddTrigger(uint8_t* trigger) {
    P5_Area_AddTrigger(&areas[pop::At<int8_t>(trigger, 0x3C)], trigger);
}

void CPedPopulation::RemoveTrigger(uint8_t* trigger) {
    P5_Area_RemoveTrigger(&areas[pop::At<int8_t>(trigger, 0x3C)], trigger);
}

void CPedPopulation::RecordSpawn(uint8_t* ped, uint8_t* entity) {
    bool hasEntity = entity && pop::At<int32_t>(entity, 0x194) != 0;
    PedSpawnRing* ring = spawnRing;
    if (!ring || !ped || !hasEntity || P5_currArea != 0) {
        return;
    }
    int model = pop::At<int16_t>(ped, 0x10E);
    int key = pop::At<int32_t>(ped, 0x1310) == 13 ? 0 : model;
    for (int i = 0; i < ring->size; i++) {
        if (key == ring->entries[i].model && entity == ring->entries[i].entity) {
            return;
        }
    }
    P5_Ring_Add(ring, model, entity);
}

void CPedPopulation::SetForcedModel(int model) {
    forcedModel = model;
    unknown7888 = 0;
    if (P5_modelInfoPtrs[model]) {
        forcedPedType = pop::At<int32_t>(P5_modelInfoPtrs[model], 0x4C);
    }
}

void CPedPopulation::BlockPedTypeOf(uint8_t* ped) {
    if (ped && pop::At<int32_t>(ped, 0x1400) != 0) {
        int type = pop::At<int32_t>(P5_modelInfoPtrs[pop::At<int16_t>(ped, 0x10E)], 0x4C);
        typeBlockedUntil[type] = P5_timeInMs + 60000;
    }
}

CPopulationInfo* CPedPopulation::GetInfoAt(pop::CVector* pos) {
    if (override_) {
        return override_;
    }
    int slot = P5_TimeSlots_Get(P5_timeSlots, -1);
    return P5_Area_GetInfoAt(&areas[P5_currArea], slot, *pos);
}

void CPedPopulation::UpdateCurrent(pop::CVector* pos) {
    CPopulationInfo* info = override_;
    if (!info) {
        int area = P5_currArea;
        int slot = P5_TimeSlots_Get(P5_timeSlots, -1);
        info = P5_Area_GetInfoAt(&areas[area], slot, *pos);
    }
    current = info;
}

void CPedPopulation::ActivateTrigger(int index) {
    if (index < 0) {
        return;
    }
    uint8_t* trigger = TriggerAt(index);
    if (trigger && pop::At<void*>(trigger, 0x24)) {
        P5_Area_AddTrigger(&areas[pop::At<int8_t>(trigger, 0x3C)], trigger);
    }
}

void CPedPopulation::DeactivateTrigger(int index) {
    if (index < 0) {
        return;
    }
    uint8_t* trigger = TriggerAt(index);
    if (trigger && pop::At<void*>(trigger, 0x24)) {
        P5_Area_RemoveTrigger(&areas[pop::At<int8_t>(trigger, 0x3C)], trigger);
    }
}

void CPedPopulation::DeteriorateAttitudeForTriggerPedType(int index, int pedType) {
    if (index < 0) {
        return;
    }
    uint8_t* trigger = TriggerAt(index);
    if (trigger) {
        P5_Area_SetTriggerPedType(&areas[pop::At<int8_t>(trigger, 0x3C)], trigger, pedType);
    }
}

int CPedPopulation::ChoosePedType(pop::CVector* pos, uint32_t exclude) {
    if (!current) {
        return -1;
    }
    CPopulationInfo* info = P5_GetInfoAt(this, pos);
    int missing = current->total - P5_GetTotalNumPeds(this);
    if (missing <= 0) {
        return -1;
    }
    int sum = 0;
    int pick = P5_rand() % missing;
    for (uint32_t type = 0; static_cast<int>(type) < 12; type++) {
        int wanted = type < info->numCounts ? info->counts[type] : 0;
        int more = wanted - pedModels[type].numPeds;
        if (more <= 0 || typeDisabled[type] != 0) {
            continue;
        }
        sum += more;
        if (sum > pick && type != exclude && (type != 7 || P5_timeInMs >= type7AllowedAt)) {
            return static_cast<int>(type);
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------------
// The static ones

namespace PedPopulation {

void __cdecl Create() {
    if (P5_population) {
        return;
    }
    void* memory = P5_Malloc(sizeof(CPedPopulation));
    P5_population = memory ? P5_Construct(memory) : nullptr;
    void* listMemory = P5_Malloc(sizeof(PedModelList));
    P5_population->modelList = listMemory ? P5_ModelList_Construct(listMemory, 5) : nullptr;
    PedSpawnRing* ring = static_cast<PedSpawnRing*>(P5_Malloc(sizeof(PedSpawnRing)));
    if (!ring) {
        P5_population->spawnRing = nullptr;
        return;
    }
    ring->entries = static_cast<PedSpawnRecord*>(P5_Malloc(21 * sizeof(PedSpawnRecord)));
    ring->size = 21;
    P5_Ring_Clear(ring);
    P5_population->spawnRing = ring;
}

void __cdecl Init() {
    P5_Create();
    P5_population->unknown7820 = 0;
    P5_population->unknown7824 = 0;
    P5_population->unknown787C = 0;
    P5_population->flags |= 4;
    P5_population->flags |= 2;
    P5_population->forcedModel = -1;
    P5_population->forcedPedType = -1;
    P5_population->flags |= 8;
    P5_population->unknown7888 = 0;
    P5_population->pendingModel = -1;
    P5_population->pendingTries = 0;
    P5_population->numSpawnPoints = 0;
    P5_population->unknown7894 = 0;
    P5_population->flags |= 0x10;
    P5_population->flags &= 0xDF;
    P5_population->type7AllowedAt = 0;
    P5_population->flags &= 0xFE;
    P5_ClearTypeDisabled(P5_population);
    P5_ClearTypeBlocked(P5_population);
    P5_ResetPedModels(P5_population);
}

void __cdecl Load() {
    P5_Create();
    CPedPopulation* population = P5_population;
    P5_LoadPedPop(population);
    for (int i = 0; i < 12; i++) {
        P5_PedModels_Setup(&population->pedModels[i], i);
    }
    P5_ClearTypeDisabled(population);
    P5_Init();
}

void __cdecl Shutdown() {
    CPedPopulation* population = P5_population;
    if (population) {
        P5_Destruct(population);
        P5_Free(population);
        P5_population = nullptr;
    }
}

} // namespace PedPopulation

// ---------------------------------------------------------------------------------

#define C08P5_REVERSED(name, address, fn) \
    REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)
#define C08P5_REVERSED_STATIC(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P5_REVERSED("PickupObjective::Construct", 0x0049C0C0, &PickupObjective::Construct);
C08P5_REVERSED("PickupObjective::IsA", 0x0049C0F0, &PickupObjective::IsA);
C08P5_REVERSED("PickupObjective::Destroy", 0x0049C160, &PickupObjective::Destroy);
C08P5_REVERSED("CAreaPopulationInfo::Destruct", 0x0049C1C0, &CAreaPopulationInfo::Destruct);
C08P5_REVERSED("CPedPopulation::ChooseModelForType", 0x0049C1F0, &CPedPopulation::ChooseModelForType);
C08P5_REVERSED("CPedPopulation::GetModelForType", 0x0049C230, &CPedPopulation::GetModelForType);
C08P5_REVERSED("CPedPopulation::GiveWeapon", 0x0049C280, &CPedPopulation::GiveWeapon);
C08P5_REVERSED("CPedPopulation::GetNumPedsOfType", 0x0049C2D0, &CPedPopulation::GetNumPedsOfType);
C08P5_REVERSED("CPedPopulation::GetTotalNumPeds", 0x0049C2F0, &CPedPopulation::GetTotalNumPeds);
C08P5_REVERSED("CPedPopulation::PickPendingModel", 0x0049C330, &CPedPopulation::PickPendingModel);
C08P5_REVERSED("CPedPopulation::LoadPedPop", 0x0049C3D0, &CPedPopulation::LoadPedPop);
C08P5_REVERSED("CPedPopulation::Override", 0x0049C5B0, &CPedPopulation::Override);
C08P5_REVERSED("CPedPopulation::SetTypeDisabled", 0x0049C5F0, &CPedPopulation::SetTypeDisabled);
C08P5_REVERSED("CPedPopulation::ClearTypeDisabled", 0x0049C610, &CPedPopulation::ClearTypeDisabled);
C08P5_REVERSED("CPedPopulation::ClearTypeBlocked", 0x0049C670, &CPedPopulation::ClearTypeBlocked);
C08P5_REVERSED("CPedPopulation::RevertToDefault", 0x0049C6D0, &CPedPopulation::RevertToDefault);
C08P5_REVERSED("PedModelList::RequestAll", 0x0049C700, &PedModelList::RequestAll);
C08P5_REVERSED_STATIC("PedSpawnRing::KeepFirstEntity", 0x0049C750, &SpawnRing_KeepFirstEntity);
C08P5_REVERSED("PedSpawnRing::SortByTime", 0x0049C800, &PedSpawnRing::SortByTime);
C08P5_REVERSED("POIGroupList::RemoveAt", 0x0049C8F0, &POIGroupList::RemoveAt);
C08P5_REVERSED("PedSpawnRing::Add", 0x0049C990, &PedSpawnRing::Add);
C08P5_REVERSED("CAreaPopulationInfo::AddTrigger", 0x0049CA20, &CAreaPopulationInfo::AddTrigger);
C08P5_REVERSED("CAreaPopulationInfo::RemoveTrigger", 0x0049CA80, &CAreaPopulationInfo::RemoveTrigger);
C08P5_REVERSED("CAreaPopulationInfo::DeteriorateAttitudes", 0x0049CAF0, &CAreaPopulationInfo::DeteriorateAttitudes);
C08P5_REVERSED("CAreaPopulationInfo::SetTriggerPedType", 0x0049CBB0, &CAreaPopulationInfo::SetTriggerPedType);
C08P5_REVERSED("CPedPopulation::Construct", 0x0049CBF0, &CPedPopulation::Construct);
C08P5_REVERSED("CPedPopulation::Destruct", 0x0049CC90, &CPedPopulation::Destruct);
C08P5_REVERSED("CPedPopulation::Reset", 0x0049CD50, &CPedPopulation::Reset);
C08P5_REVERSED("CPedPopulation::UpdateForcedModel", 0x0049CDB0, &CPedPopulation::UpdateForcedModel);
C08P5_REVERSED("CPedPopulation::RemovePOIGroup", 0x0049D480, &CPedPopulation::RemovePOIGroup);
C08P5_REVERSED("CPedPopulation::ResetPedModels", 0x0049DB10, &CPedPopulation::ResetPedModels);
C08P5_REVERSED("CPedPopulation::AddTrigger", 0x0049DB50, &CPedPopulation::AddTrigger);
C08P5_REVERSED("CPedPopulation::RemoveTrigger", 0x0049DB70, &CPedPopulation::RemoveTrigger);
C08P5_REVERSED("CPedPopulation::RecordSpawn", 0x0049DC80, &CPedPopulation::RecordSpawn);
C08P5_REVERSED("CAreaPopulationInfo::GetInfoAt", 0x0049DD10, &CAreaPopulationInfo::GetInfoAt);
C08P5_REVERSED("CAreaPopulationInfo::IsInTriggerForPedType", 0x0049DE10, &CAreaPopulationInfo::IsInTriggerForPedType);
C08P5_REVERSED_STATIC("CPedPopulation::Create", 0x0049DEC0, &PedPopulation::Create);
C08P5_REVERSED_STATIC("CPedPopulation::Init", 0x0049DF80, &PedPopulation::Init);
C08P5_REVERSED("CPedPopulation::SetForcedModel", 0x0049EE40, &CPedPopulation::SetForcedModel);
C08P5_REVERSED("CPedPopulation::BlockPedTypeOf", 0x0049EE70, &CPedPopulation::BlockPedTypeOf);
C08P5_REVERSED("CPedPopulation::GetInfoAt", 0x0049EEB0, &CPedPopulation::GetInfoAt);
C08P5_REVERSED("CPedPopulation::UpdateCurrent", 0x0049EF00, &CPedPopulation::UpdateCurrent);
C08P5_REVERSED("CPedPopulation::ActivateTrigger", 0x0049EF60, &CPedPopulation::ActivateTrigger);
C08P5_REVERSED("CPedPopulation::DeactivateTrigger", 0x0049EFA0, &CPedPopulation::DeactivateTrigger);
C08P5_REVERSED("CPedPopulation::DeteriorateAttitudeForTriggerPedType", 0x0049EFE0,
               &CPedPopulation::DeteriorateAttitudeForTriggerPedType);
C08P5_REVERSED_STATIC("CPedPopulation::Load", 0x0049F190, &PedPopulation::Load);
C08P5_REVERSED_STATIC("CPedPopulation::Shutdown", 0x0049F1E0, &PedPopulation::Shutdown);
C08P5_REVERSED("PedModelList::ChooseModel", 0x0049F0C0, &PedModelList::ChooseModel);
C08P5_REVERSED("CPedPopulation::ChoosePedType", 0x0049F610, &CPedPopulation::ChoosePedType);
