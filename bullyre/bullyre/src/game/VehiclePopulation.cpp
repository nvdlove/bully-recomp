// The ambient vehicle population: per area (0x74 bytes each) four
// CAmbientVehicleInfo (0x1C each, from +4) read from Config\Dat\VehPop.dat, counts of
// vehicles per class (0x14 each, from +0x1D00), the vehicles counted (+0x1D60).

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace c01vehpop {

struct CVector3 {
    float x, y, z;
};

using re::At;

inline re::Ptr32<void>* const IArchivable_vftable = reinterpret_cast<re::Ptr32<void>*>(0x00900CDC);
inline const char* const readMode = reinterpret_cast<const char*>(0x008FF548);   // "r"
inline const char* const scanString = reinterpret_cast<const char*>(0x008FF970);   // "%s"

GAME_FN(int, __cdecl, File_OpenFile, 0x0042D260, const char* name, const char* mode, int);
GAME_FN(char*, __cdecl, LoadLine, 0x00429C10, int file);
GAME_FN(int, __cdecl, File_CloseFile, 0x0042D450, int file);
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(int, __cdecl, GetAreaFromName, 0x004305D0, const char* name);
GAME_FN(void, __thiscall, AmbientVehicleInfo_Parse, 0x004645F0, void* info, const char* line);
GAME_FN(uint8_t*, __cdecl, CVehicleModelInfo_Get, 0x005359D0, int model);
GAME_FN(void, __thiscall, VehicleClass_Add, 0x00464AB0, void* counts, int model);
GAME_FN(void, __thiscall, VehicleClass_Remove, 0x00464AF0, void* counts, int model);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void*, __thiscall, AmbientVehicleInfo_Construct, 0x004A7FD0, void* self);
GAME_FN(void, __thiscall, AmbientVehicleInfo_Destruct, 0x004A8000, void* self);
GAME_FN(void, __thiscall, AmbientVehicleInfo_464680, 0x00464680, void* self, int);

// 0x41A6E0 (thiscall)
void __fastcall SetField1D64(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x1D64) = 10;
}

// 0x41A6F0 (thiscall): Config\Dat\VehPop.dat: an area's name, then its four lines
void __fastcall Load(uint8_t* self, re::Edx) {
    int area = -1;
    int line = 0;
    int file = File_OpenFile("Config\\Dat\\VehPop.dat", readMode, 1);
    for (char* text = LoadLine(file); text; text = LoadLine(file)) {
        if (text[0] == '\0' || text[0] == '#') {
            continue;
        }
        if (area == -1) {
            char name[24];
            Sscanf(text, scanString, name);
            area = GetAreaFromName(name);
            continue;
        }
        AmbientVehicleInfo_Parse(self + area * 0x74 + line * 0x1C + 4, text);
        line++;
        if (line >= 4) {
            area = -1;
            line = 0;
        }
    }
    File_CloseFile(file);
}

// 0x41A7A0 (thiscall): a vehicle of the model added (its class counted)
void __fastcall AddVehicle(uint8_t* self, re::Edx, int model) {
    if (uint8_t* info = CVehicleModelInfo_Get(model)) {
        VehicleClass_Add(self + 0x1D00 + At<int32_t>(info, 0x54) * 0x14, model);
    }
    At<int32_t>(self, 0x1D60)++;
}

// 0x41A7E0 (thiscall): one removed
void __fastcall RemoveVehicle(uint8_t* self, re::Edx, int model) {
    if (uint8_t* info = CVehicleModelInfo_Get(model)) {
        VehicleClass_Remove(self + 0x1D00 + At<int32_t>(info, 0x54) * 0x14, model);
    }
    At<int32_t>(self, 0x1D60)--;
}

// 0x41A820 (thiscall): the override info (made on first use)
void __fastcall SetOverride(uint8_t* self, re::Edx, int value) {
    if (!At<void*>(self, 0x1D58)) {
        void* memory = Malloc(0x1C);
        At<void*>(self, 0x1D58) = memory ? AmbientVehicleInfo_Construct(memory) : nullptr;
    }
    AmbientVehicleInfo_464680(At<void*>(self, 0x1D58), value);
}

// 0x41A860 (thiscall): the override info deleted
void __fastcall ClearOverride(uint8_t* self, re::Edx) {
    if (void* info = At<void*>(self, 0x1D58)) {
        reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(info))[0]))(info, 1);
        At<void*>(self, 0x1D58) = nullptr;
    }
}

// 0x41A8F0 (thiscall): CAreaAmbientVehicleInfo's constructor (its vtable, then its four infos)
uint8_t* __fastcall AreaInfo_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x00900D1C;   // (CAreaAmbientVehicleInfo's vtable)
    for (int i = 0; i < 4; i++) {
        AmbientVehicleInfo_Construct(self + 4 + i * 0x1C);
    }
    return self;
}

// 0x41A920 (thiscall): CAreaAmbientVehicleInfo's destructor (its four infos, last
// first; then the base's vtable)
void __fastcall AreaInfo_Destruct(uint8_t* self, re::Edx) {
    for (int i = 3; i >= 0; i--) {
        AmbientVehicleInfo_Destruct(self + 4 + i * 0x1C);
    }
    At<re::Ptr32<void>*>(self, 0) = IArchivable_vftable;
}

GAME_VAR(uint8_t*, ms_pOtherPopulation, 0x00BCD080);   // (the ambient bikes' population)
GAME_FN(void, __thiscall, ObjectModelInfo_4647A0, 0x004647A0, void* self);
GAME_FN(void, __stdcall, VectorDestructorIterator, 0x004017C0, void* array, size_t size, int count, void(__thiscall* destructor)(void*));

// 0x41C110 (thiscall): the destructor: the override deleted, then the members
// destroyed, last first
void __fastcall Destruct(uint8_t* self, re::Edx) {
    if (void* info = At<void*>(self, 0x1D58)) {
        reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(info))[0]))(info, 1);
    }
    AmbientVehicleInfo_Destruct(self + 0x1D3C);
    for (int i = 2; i >= 0; i--) {
        ObjectModelInfo_4647A0(self + 0x1D00 + i * 0x14);
    }
    for (int area = 63; area >= 0; area--) {
        uint8_t* info = self + area * 0x74;
        VectorDestructorIterator(info + 4, 0x1C, 4, reinterpret_cast<void(__thiscall*)(void*)>(0x004A8000));
        At<re::Ptr32<void>*>(info, 0) = IArchivable_vftable;
    }
}

// 0x41C190 (thiscall): a vehicle counted (kinds 1 and 3 by class too)
void __fastcall CountVehicle(uint8_t* self, re::Edx, int model, int kind) {
    int8_t k = static_cast<int8_t>(kind);
    if (k == 1 || k == 3) {
        if (CVehicleModelInfo_Get(model)) {
            uint8_t* info = CVehicleModelInfo_Get(model);
            VehicleClass_Add(self + 0x1D00 + At<int32_t>(info, 0x54) * 0x14, model);
        }
        At<int32_t>(self, 0x1D60)++;
    }
    At<int32_t>(self, 0x1D5C)++;
}

// 0x41C1E0 (thiscall): one uncounted
void __fastcall UncountVehicle(uint8_t* self, re::Edx, int model, int kind) {
    int8_t k = static_cast<int8_t>(kind);
    if (k != 1 && k != 3) {
        At<int32_t>(self, 0x1D5C)--;
        return;
    }
    if (CVehicleModelInfo_Get(model)) {
        uint8_t* info = CVehicleModelInfo_Get(model);
        VehicleClass_Remove(self + 0x1D00 + At<int32_t>(info, 0x54) * 0x14, model);
    }
    At<int32_t>(self, 0x1D60)--;
    At<int32_t>(self, 0x1D5C)--;
}

// 0x41C240 (thiscall): the class to make a vehicle of: one short of its quota
// (+0x1D44.., for the first +0x1D50 classes) the most, not `excluded`, not `previous`
// while the other population has one; -1 for none. (Later classes compare their
// shortfall with the earlier class's count, as the game does.)
int32_t* __fastcall ChooseClass(uint8_t* self, re::Edx, int32_t* out, int previous, int excluded) {
    uint32_t classes = At<uint32_t>(self, 0x1D50);
    int32_t result = -1;
    int32_t best = 100;
    int32_t shortfall = (classes > 0 ? self[0x1D44] : 0) - At<int32_t>(self, 0x1D04);
    if (shortfall > 0 && shortfall < 100 && excluded != 0 && (previous != 0 || At<int32_t>(ms_pOtherPopulation, 0x1D04) < 1)) {
        best = At<int32_t>(self, 0x1D04);
        result = 0;
    }
    shortfall = (classes > 1 ? self[0x1D45] : 0) - At<int32_t>(self, 0x1D18);
    if (shortfall > 0 && shortfall < best && excluded != 1 && (previous != 1 || At<int32_t>(ms_pOtherPopulation, 0x1D18) < 1)) {
        best = At<int32_t>(self, 0x1D18);
        result = 1;
    }
    shortfall = (classes > 2 ? self[0x1D46] : 0) - At<int32_t>(self, 0x1D2C);
    if (shortfall > 0 && shortfall < best && excluded != 2 && (previous != 2 || At<int32_t>(ms_pOtherPopulation, 0x1D2C) < 1)) {
        result = 2;
    }
    *out = result;
    return out;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

inline re::Ptr32<void>* const CAreaAmbientVehicleInfo_vftable = reinterpret_cast<re::Ptr32<void>*>(0x00900D1C);
inline re::Ptr32<void>* const CVehicleModelsInfo_vftable = reinterpret_cast<re::Ptr32<void>*>(0x00900CF4);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_FN(void, __stdcall, VectorConstructorIterator, 0x00401240, void* array, size_t size, int count, void*(__thiscall* constructor)(void*));
GAME_FN(void, __thiscall, VehicleModelsInfo_Construct, 0x00464E00, void* self);
GAME_FN(void, __thiscall, VehicleModelsInfo_464E20, 0x00464E20, void* self, int index);
GAME_FN(void, __thiscall, VehicleModelsInfo_464E30, 0x00464E30, void* self, int index);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t*, __thiscall, Population_Construct, 0x0041EA80, uint8_t* self);
GAME_FN(void, __thiscall, Population_Load, 0x0041A6F0, uint8_t* self);
GAME_FN(void, __thiscall, Population_Destruct, 0x0041C110, uint8_t* self);
GAME_FN(void, __thiscall, AmbientVehicleInfo_4646D0, 0x004646D0, void* self, void* other);
GAME_FN(int32_t, __thiscall, Fn_6A8CC0, 0x006A8CC0, void* self, int);
GAME_FN(void, __thiscall, AreaInfo_41E8E0, 0x0041E8E0, void* self, int32_t, CVector3 pos, int32_t area, void* info);
GAME_FN(void, __thiscall, Population_UpdateCounts, 0x0041EC00, uint8_t* self, CVector3 pos);

// 0x41EA80 (thiscall): the constructor
uint8_t* __fastcall Construct(uint8_t* self, re::Edx) {
    for (int area = 0; area < 64; area++) {
        uint8_t* info = self + area * 0x74;
        At<re::Ptr32<void>*>(info, 0) = CAreaAmbientVehicleInfo_vftable;
        VectorConstructorIterator(info + 4, 0x1C, 4, reinterpret_cast<void*(__thiscall*)(void*)>(0x004A7FD0));
    }
    for (int i = 0; i < 3; i++) {
        VehicleModelsInfo_Construct(self + 0x1D00 + i * 0x14);
        At<re::Ptr32<void>*>(self, 0x1D00 + i * 0x14) = CVehicleModelsInfo_vftable;
    }
    AmbientVehicleInfo_Construct(self + 0x1D3C);
    At<int32_t>(self, 0x1D58) = 0;
    At<int32_t>(self, 0x1D5C) = 0;
    At<int32_t>(self, 0x1D60) = 0;
    At<int32_t>(self, 0x1D6C) = 0;
    At<int32_t>(self, 0x1D70) = 0;
    At<int32_t>(self, 0x1D64) = 2;
    return self;
}

// (inlined in both Init functions: the population made on first use)
uint8_t* GetPopulation() {
    if (!ms_pOtherPopulation) {
        uint8_t* memory = static_cast<uint8_t*>(Malloc(0x1D74));
        ms_pOtherPopulation = memory ? Population_Construct(memory) : nullptr;
    }
    return ms_pOtherPopulation;
}

// 0x41EB10: the population made, loaded and its classes set up
void __cdecl Init() {
    uint8_t* population = GetPopulation();
    Population_Load(population);
    for (int i = 0; i < 3; i++) {
        VehicleModelsInfo_464E20(population + 0x1D00 + i * 0x14, i);
    }
    At<float>(population, 0x1D68) = 0.0f;
}

// 0x41EB70: its classes reset
void __cdecl Reset() {
    uint8_t* population = GetPopulation();
    for (int i = 0; i < 3; i++) {
        VehicleModelsInfo_464E30(population + 0x1D00 + i * 0x14, i);
    }
    At<float>(population, 0x1D68) = 0.0f;
}

// 0x41EBD0: the population deleted
void __cdecl Shutdown() {
    if (uint8_t* population = ms_pOtherPopulation) {
        Population_Destruct(population);
        Free(population);
        ms_pOtherPopulation = nullptr;
    }
}

// 0x41EC00 (thiscall): the wanted counts for a position (the override's, or the
// current area's)
void __fastcall UpdateCounts(uint8_t* self, re::Edx, CVector3 pos) {
    if (void* other = At<void*>(self, 0x1D58)) {
        AmbientVehicleInfo_4646D0(self + 0x1D3C, other);
        return;
    }
    int32_t area = currArea;
    int32_t value = Fn_6A8CC0(reinterpret_cast<void*>(0x020C35E8), -1);
    AreaInfo_41E8E0(self + area * 0x74, value, pos, area, self + 0x1D3C);
}

// 0x41FF20 (thiscall): more vehicles wanted at a position (never in area 2)
uint8_t __fastcall WantsMoreVehicles(uint8_t* self, re::Edx, const CVector3* pos) {
    if (currArea == 2) {
        return 0;
    }
    Population_UpdateCounts(self, *pos);
    int32_t missing = At<int32_t>(self, 0x1D40) - At<int32_t>(self, 0x1D60);
    if (missing < 0) {
        missing = 0;
    }
    return missing > 0;
}

// 0x41EDF0 (thiscall, a CPool): its free slots
int __fastcall Pool_GetNoOfFreeSpaces(uint8_t* pool, re::Edx) {
    int size = At<int32_t>(pool, 8);
    int used = 0;
    for (int i = 0; i < size; i++) {
        if (!(At<uint8_t*>(pool, 4)[i] & 0x80)) {
            used++;
        }
    }
    return size - used;
}

GAME_VAR(uint8_t*, ms_pTriggerPool, 0x020C7C48);
GAME_VAR(CVector3, vecUnset, 0x00BCC564);
GAME_VAR(uint8_t*, PED_player, 0x00C1AEA8);
GAME_VAR(uint8_t*, PED_aiButes, 0x020C889C);
inline void* const triggerPoolWrapper = reinterpret_cast<void*>(0x020C7E98);
inline void* const pedTypeMgr = reinterpret_cast<void*>(0x00C2C108);
GAME_FN(int16_t, __thiscall, TriggerPool_GetSize, 0x006D70A0, void* self);
GAME_FN(uint8_t, __thiscall, PedTypeMgr_TriggerCovers_49D340, 0x0049D340, void* self, const CVector3* pos, void* trigger);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const CVector3* a, const CVector3* b);

// 0x41E8E0 (thiscall, a CAreaAmbientVehicleInfo): the counts for a time slot at a
// position into `out`: the last covering trigger's of the area (its own infos), else
// this area's; with the player driving, the first class's count scaled by the AI
// attribute (+0xA4) and the total adjusted
void __fastcall AreaInfo_GetInfo(uint8_t* self, re::Edx, int slot, CVector3 pos, int area, uint8_t* out) {
    int count = TriggerPool_GetSize(triggerPoolWrapper);
    uint8_t* found = nullptr;
    CVector3 nearest = vecUnset;
    for (int i = 0; i < count; i++) {
        uint8_t* pool = ms_pTriggerPool;
        if (i < 0 || (At<uint8_t*>(pool, 4)[i] & 0x80)) {
            continue;
        }
        uint8_t* trigger = At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * i;
        if (!trigger || static_cast<int8_t>(trigger[0x3C]) != area || At<void*>(trigger, 0x28) == nullptr) {
            continue;
        }
        if (PedTypeMgr_TriggerCovers_49D340(*reinterpret_cast<re::Ptr32<void>*>(pedTypeMgr), &pos, trigger)) {
            nearest = At<CVector3>(trigger, 4);
            Distance(&nearest, &pos);   // (computed, not used)
            found = trigger;
        }
    }
    uint8_t* info = nullptr;
    if (found) {
        info = At<uint8_t*>(found, 0x28) + 4 + slot * 0x1C;
    }
    if (!info) {
        info = self + 4 + slot * 0x1C;
    }
    AmbientVehicleInfo_4646D0(out, info);
    uint8_t* vehicle = At<uint8_t*>(PED_player, 0x1554);
    if (!vehicle || At<int32_t>(vehicle, 0x46C) != 1) {
        return;
    }
    int base = At<uint32_t>(info, 0x14) > 1 ? info[9] : 0;
    int scaled = static_cast<int>(static_cast<double>(At<float>(PED_aiButes, 0xA4)) * base);
    int change = scaled - base;
    At<int32_t>(out, 4) = At<int32_t>(info, 4) + change;
    uint8_t value = static_cast<uint8_t>((At<uint32_t>(info, 0x14) > 1 ? info[9] : 0) + change);
    if (At<uint32_t>(out, 0x14) > 1) {
        out[9] = value;
        return;
    }
    while (At<uint32_t>(out, 0x14) < 1) {
        out[8 + At<uint32_t>(out, 0x14)] = 0;
        At<uint32_t>(out, 0x14)++;
    }
    out[8 + At<uint32_t>(out, 0x14)] = value;
    At<uint32_t>(out, 0x14)++;
}

} // namespace c01vehpop

// ---- registration ----------------------------------------------------------------------

#define VEHPOP_REVERSED_C01(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01vehpop {
VEHPOP_REVERSED_C01("CVehiclePopulation::SetField1D64", 0x0041A6E0, &SetField1D64);
VEHPOP_REVERSED_C01("CVehiclePopulation::Load", 0x0041A6F0, &Load);
VEHPOP_REVERSED_C01("CVehiclePopulation::AddVehicle", 0x0041A7A0, &AddVehicle);
VEHPOP_REVERSED_C01("CVehiclePopulation::RemoveVehicle", 0x0041A7E0, &RemoveVehicle);
VEHPOP_REVERSED_C01("CVehiclePopulation::SetOverride", 0x0041A820, &SetOverride);
VEHPOP_REVERSED_C01("CVehiclePopulation::ClearOverride", 0x0041A860, &ClearOverride);
VEHPOP_REVERSED_C01("CAreaAmbientVehicleInfo::CAreaAmbientVehicleInfo", 0x0041A8F0, &AreaInfo_Construct);
VEHPOP_REVERSED_C01("CAreaAmbientVehicleInfo::~CAreaAmbientVehicleInfo", 0x0041A920, &AreaInfo_Destruct);
VEHPOP_REVERSED_C01("CVehiclePopulation::~CVehiclePopulation", 0x0041C110, &Destruct);
VEHPOP_REVERSED_C01("CVehiclePopulation::CountVehicle", 0x0041C190, &CountVehicle);
VEHPOP_REVERSED_C01("CVehiclePopulation::UncountVehicle", 0x0041C1E0, &UncountVehicle);
VEHPOP_REVERSED_C01("CVehiclePopulation::ChooseClass", 0x0041C240, &ChooseClass);
VEHPOP_REVERSED_C01("CVehiclePopulation::CVehiclePopulation", 0x0041EA80, &Construct);
VEHPOP_REVERSED_C01("CVehiclePopulation::Init", 0x0041EB10, &Init);
VEHPOP_REVERSED_C01("CVehiclePopulation::Reset", 0x0041EB70, &Reset);
VEHPOP_REVERSED_C01("CVehiclePopulation::Shutdown", 0x0041EBD0, &Shutdown);
VEHPOP_REVERSED_C01("CVehiclePopulation::UpdateCounts", 0x0041EC00, &UpdateCounts);
VEHPOP_REVERSED_C01("CVehiclePopulation::WantsMoreVehicles", 0x0041FF20, &WantsMoreVehicles);
VEHPOP_REVERSED_C01("CPool::GetNoOfFreeSpaces", 0x0041EDF0, &Pool_GetNoOfFreeSpaces);
VEHPOP_REVERSED_C01("CAreaAmbientVehicleInfo::GetInfo", 0x0041E8E0, &AreaInfo_GetInfo);
} // namespace c01vehpop
