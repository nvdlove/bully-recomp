// The truancy system (0xC0F78C, 0x158 bytes): the prefect/orderly who goes after the
// player for trespassing, being late, skipping class, curfew and misconduct. Its
// creation and reset, the crime checks, the orderly's vehicle spawned on the road near
// its target (0x44EAE0) and the rule table (0x44F2B0).

#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// The system's fields
//   +0x0  uint8     +0x1 uint8     +0x2 uint8 (the rule set: matched against a rule's +0x21)
//   +0x4, +0x14..+0x20 int32 (0 on reset)
//   +0x8  int32 the orderly's model ("Orderly01")      +0xC int32
//   +0x10 CPed* the ped it's after (a registered reference)
//   +0x28 int32 the number of rules                     +0x2C int32 (0x2E on reset)
//   +0x30, +0x34, +0x38 int32 (100, 200, 300)
//   +0x3C int32[64] per-crime values                    +0x13C float, +0x140 float
//   +0x144..+0x154 the five crimes' name hashes
GAME_VAR(uint8_t*, ts_system, 0x00C0F78C);
GAME_VAR(uint8_t*, ts_playerPed, 0x00C1AEA8);   // FindPlayerPed
GAME_VAR(int32_t, ts_currArea, 0x00BD1008);     // CGame::currArea
GAME_VAR(uint32_t, ts_timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t, ts_clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, ts_clockMinutes, 0x00BCEB4A);
GAME_ARRAY(uint8_t*, ts_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, ts_orderlyModelA, 0x00BF4170);   // the orderly for cars
GAME_VAR(int32_t, ts_orderlyModelB, 0x00BF4174);   // for bikes
// the models a rule's vehicle can't be
GAME_VAR(int32_t, ts_excludedModel1, 0x00BF42D4);
GAME_VAR(int32_t, ts_excludedModel2, 0x00BF42D0);
GAME_VAR(int32_t, ts_excludedModel3, 0x00BF42CC);
GAME_VAR(int32_t, ts_excludedModel4, 0x00BF42DC);

inline const char* const ts_trespassing = reinterpret_cast<const char*>(0x00906450);
inline const char* const ts_lateForClass = reinterpret_cast<const char*>(0x00906440);
inline const char* const ts_skippingClass = reinterpret_cast<const char*>(0x00906430);
inline const char* const ts_curfew = reinterpret_cast<const char*>(0x00906428);
inline const char* const ts_misconduct = reinterpret_cast<const char*>(0x0090641C);
inline const char* const ts_orderly01 = reinterpret_cast<const char*>(0x0090645C);
inline const char* const ts_statMaleOld = reinterpret_cast<const char*>(0x00906400);     // "STAT_CV_MALE_OLD"
inline const char* const ts_statFemaleOld = reinterpret_cast<const char*>(0x009063EC);   // "STAT_CV_FEMALE_OLD"
inline const char* const ts_crimeWeaponViolence = reinterpret_cast<const char*>(0x00906468);
inline void* const ts_rootName = reinterpret_cast<void*>(0x01B652F8);   // the tables' root (a name)
inline void* const ts_tableType = reinterpret_cast<void*>(0x01B652FC);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, Ref_Set, 0x005D7F10, void* ref, void* entity);
// the config's names and paths (a name: 4 bytes, a path: 8)
GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(void, __thiscall, Name_FromText, 0x005FA910, void* self, const char* text);
GAME_FN(void, __thiscall, Name_Destruct, 0x0049A650, void* self);   // (empty)
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* self, int count, const void* names);
GAME_FN(uint32_t, __cdecl, Config_GetHash, 0x0072AEA0, const void* path, void* type);
GAME_FN(int32_t, __cdecl, Config_Count, 0x0072B7D0, const void* path);
GAME_FN(uint8_t*, __cdecl, Config_GetAt, 0x0072B810, const void* path, int index, void* type);
GAME_FN(int32_t, __cdecl, ModelIndexFromName, 0x0051C2E0, const char* name, int32_t, int32_t);
// peds
GAME_FN(uint8_t*, __thiscall, Ped_Fn477DC0, 0x00477DC0, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_Fn473220, 0x00473220, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_Fn41AA50, 0x0041AA50, void* ped);
GAME_FN(int32_t*, __thiscall, Fn522380, 0x00522380, void* self, int32_t* out);
GAME_FN(uint8_t, __thiscall, Clock_Fn6A8D20, 0x006A8D20, void* self);
GAME_FN(uint8_t, __cdecl, Fn425E40, 0x00425E40, int, int, int, int);
GAME_FN(uint8_t, __thiscall, Zones_Fn46F190, 0x0046F190, void* self, void* ped, const CVector* pos);
GAME_FN(void, __thiscall, Crimes_Report, 0x0053D650, void* self, const char* name, int32_t amount, int);
GAME_VAR(void*, ts_crimes, 0x00C674A0);
// the orderly's vehicle
GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void, __cdecl, Vehicle_Fn41A340, 0x0041A340, void* vehicle, void* ped, float, float);
GAME_FN(void*, __thiscall, Objective_Construct, 0x00470D60, void* self, void* ped, void* target, int);
GAME_FN(void, __thiscall, Objectives_Add, 0x00471390, void* self, void* objective);
GAME_FN(CVector*, __thiscall, Camera_Fn41AA70, 0x0041AA70, void* self, CVector* out);
GAME_FN(uint8_t, __thiscall, Paths_FindNodeNear, 0x00440870, void* self, float x, float y, float z, float dirX,
        float dirY, float dirZ, float maxDist, float, int, CVector* pos, re::Ptr32<void>* nodeA, re::Ptr32<void>* nodeB, float* t, int,
        int);
GAME_FN(float, __cdecl, World_FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int);
GAME_FN(void, __cdecl, World_FindObjectsKindaColliding, 0x0045E3C0, const CVector* pos, float radius, int,
        int16_t* count, int, uintptr_t, int);
GAME_FN(void, __cdecl, Vector_Sub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x004246B0, const CVector* self);
GAME_FN(void*, __thiscall, Population_AddPed, 0x0049F560, void* self, int type, int32_t model);
GAME_VAR(void*, ts_population, 0x00C2C108);
GAME_FN(void*, __cdecl, Vehicle_New, 0x004CC490, size_t size);
GAME_FN(uint8_t*, __thiscall, Bike_Construct, 0x004C6F70, void* self, int model, int createdBy);
GAME_FN(uint8_t*, __thiscall, Automobile_Construct, 0x004B7B80, void* self, int model, int createdBy);
GAME_FN(void, __cdecl, Fn5F2B00, 0x005F2B00, void*, int);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(void, __thiscall, ModelInfo_ChooseColours, 0x00535060, void* self, uint8_t* colour1, uint8_t* colour2);
GAME_FN(int32_t, __thiscall, Node_Fn43F8A0, 0x0043F8A0, void* node);
GAME_FN(void, __thiscall, AutoPilot_Fn5AB1A0, 0x005AB1A0, void* self, uintptr_t);
GAME_FN(void, __cdecl, Fn41BF30, 0x0041BF30, void* vehicle);
GAME_FN(CVector*, __thiscall, Node_GetPosition, 0x00417C00, void* node, CVector* out);
GAME_FN(float, __cdecl, Max, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, Min, 0x00414470, float a, float b);
GAME_FN(void*, __thiscall, AutoPilot_Fn4178E0, 0x004178E0, void* self);
GAME_FN(void*, __thiscall, AutoPilot_Fn417900, 0x00417900, void* self);
GAME_FN(void, __thiscall, Node_Fn43F970, 0x0043F970, void* node, float* x, float* y, int lane);
GAME_FN(void, __thiscall, Node_Fn4403C0, 0x004403C0, void* node, CVector* out, float x, float y, int lane, float);
GAME_FN(float, __cdecl, Curve_Length, 0x00428DA0, const CVector* a, const CVector* b, float, float, float, float);
GAME_FN(void, __cdecl, Curve_Point, 0x00428E70, const CVector* a, const CVector* b, const CVector* dirA,
        const CVector* dirB, float progress, uint32_t duration, CVector* pos, CVector* speed);
GAME_FN(void, __thiscall, Physical_SetMoveSpeed, 0x0041A440, void* self, const CVector* speed);
GAME_FN(float, __cdecl, Vector_Heading, 0x00433310, const CVector* v);
GAME_FN(void, __thiscall, Ped_Fn49A580, 0x0049A580, void* ped, void* vehicle, int);
GAME_FN(void, __thiscall, Ped_Fn47D0D0, 0x0047D0D0, void* ped, void* vehicle);
// (ours, through the game)
GAME_FN(void, __cdecl, Truancy_CreateFn, 0x0044E450);
GAME_FN(void, __cdecl, Truancy_ResetFn, 0x0044E6A0);
GAME_FN(void, __thiscall, Truancy_InitCrimeValuesFn, 0x0044E230, void* self);
GAME_FN(uint8_t, __thiscall, Ped_IsTruancyAgeFn, 0x0044E3D0, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsOldCivilianFn, 0x0044E160, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_Fn44E400Fn, 0x0044E400, void* ped);
GAME_FN(uint8_t, __stdcall, Rule_MatchesFn, 0x0044E990, uint8_t* rule, void* ped);
GAME_FN(void, __thiscall, Truancy_SendVehicleFn, 0x0044E780, void* self, void* vehicle);

const CVector& EntityPosition(uint8_t* entity) {
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x14);
    return *reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : entity + 4);
}

// the ped's kind (+0x1310): 1..6, 11 and 13 are the ones truancy is about
bool TruancyKind(int32_t kind) {
    return (kind > 0 && kind < 7) || kind == 0xB || kind == 0xD;
}

// 0x44E160 (thiscall, a ped): an old civilian (its stats' name)
uint8_t __fastcall Ped_IsOldCivilian(void* ped) {
    uint32_t& guard = *reinterpret_cast<uint32_t*>(0x00C0F7B0);
    uint32_t& maleOld = *reinterpret_cast<uint32_t*>(0x00C0F7AC);
    uint32_t& femaleOld = *reinterpret_cast<uint32_t*>(0x00C0F7A8);
    if (!(guard & 1)) {
        guard |= 1;
        maleOld = HashName(ts_statMaleOld);
    }
    if (!(guard & 2)) {
        guard |= 2;
        femaleOld = HashName(ts_statFemaleOld);
    }
    uint8_t* info = Ped_Fn477DC0(ped);
    if (info) {
        uint8_t* stats = At<uint8_t*>(info, 0x54);
        if (stats) {
            uint32_t name = At<uint32_t>(stats, 0x18);
            if (name == maleOld || name == femaleOld) {
                return 1;
            }
        }
    }
    return 0;
}

// 0x44E1E0 (thiscall): +0x34 and +0x38 from a triple's last two
void __fastcall Truancy_SetTimes(uint8_t* self, re::Edx, const int32_t* values) {
    At<int32_t>(self, 0x34) = values[1];
    At<int32_t>(self, 0x38) = values[2];
}

// 0x44E200 (thiscall): a crime's value
void __fastcall Truancy_SetCrimeValue(uint8_t* self, re::Edx, int index, int32_t value) {
    At<int32_t>(self, 0x3C + index * 4) = value;
}

// 0x44E210 (thiscall): the defaults
void __fastcall Truancy_SetDefaultTimes(uint8_t* self) {
    At<int32_t>(self, 0x30) = 100;
    At<int32_t>(self, 0x34) = 200;
    At<int32_t>(self, 0x38) = 300;
}

// 0x44E230 (thiscall): the crimes' values (0: 7, 2..19: 0, 35: 8, the rest 15)
void __fastcall Truancy_InitCrimeValues(uint8_t* self) {
    int32_t* values = &At<int32_t>(self, 0x3C);
    for (int i = 0; i < 64; i++) {
        if (uint32_t(i - 2) <= 0x11) {
            values[i] = 0;
        } else if (i == 0) {
            values[0] = 7;
        } else if (i == 0x23) {
            values[0x23] = 8;
        } else {
            values[i] = 15;
        }
    }
}

// 0x44E280 (thiscall): a ped to go after (the player always; else one of a higher
// +0x1D40 than the current one)
void __fastcall Truancy_SetTarget(uint8_t* self, re::Edx, uint8_t* ped) {
    re::Ptr32<uint8_t>& target = At<uint8_t*>(self, 0x10);
    if (ts_playerPed == target) {
        return;
    }
    if (At<int32_t>(ped, 0x14E8) == 0 || Ped_Fn473220(ped)) {
        return;
    }
    if (ts_playerPed != ped && target && At<int32_t>(ped, 0x1D40) <= At<int32_t>(target, 0x1D40)) {
        return;
    }
    Ref_Set(&target, ped);
}

// 0x44E2D0 (stdcall; a thiscall that ignores ecx): an amount limited so the ped's
// +0x1D40 stays within `*limit`
void __fastcall Truancy_ClampAmount(void*, re::Edx, uint8_t* ped, int32_t* amount, const int32_t* limit) {
    int32_t have = At<int32_t>(ped, 0x1D40);
    int32_t max = *limit;
    if (have > max) {
        *amount = 0;
        return;
    }
    if (*amount + have > max) {
        *amount = max - have;
    }
}

// 0x44E310 (thiscall)
void __fastcall Truancy_Set13C(uint8_t* self, re::Edx, float value) {
    At<float>(self, 0x13C) = value;
}

// 0x44E360: how close the clock is to 2:00 (0 at 2:00, 1 at 6:00 and 22:00; 0 when
// the clock's check fails)
float __cdecl Truancy_CurfewFactor() {
    if (!Clock_Fn6A8D20(reinterpret_cast<void*>(0x020C35E8))) {
        return 0.0f;
    }
    int minutes = ts_clockHours * 60 + ts_clockMinutes;
    int left = minutes <= 120 ? 120 - minutes : 1560 - minutes;
    return float(1.0 - left / 240.0);
}

// 0x44E3C0 (thiscall): +0x2C unless -1
void __fastcall Truancy_Set2C(uint8_t* self, re::Edx, int32_t value) {
    if (value != -1) {
        At<int32_t>(self, 0x2C) = value;
    }
}

// 0x44E3D0 (thiscall, a ped): truancy applies to it (its kind)
int32_t __fastcall Ped_IsTruancyAge(uint8_t* ped) {
    return TruancyKind(At<int32_t>(ped, 0x1310)) ? 1 : 0;
}

// 0x44E400 (thiscall, a ped)
int32_t __fastcall Ped_Fn44E400(uint8_t* ped) {
    uint8_t* info = Ped_Fn477DC0(ped);
    if (!TruancyKind(At<int32_t>(ped, 0x1310)) || !info) {
        return 0;
    }
    int32_t out;   // (uninitialised: 0x522380 fills it)
    if (*Fn522380(info, &out) == 0) {
        return 1;
    }
    return 0;
}

// 0x44E450: the system made (once), with its crimes' name hashes
void __cdecl Truancy_Create() {
    if (ts_system) {
        return;
    }
    uint8_t* system = static_cast<uint8_t*>(Malloc(0x158));
    if (system) {
        At<int32_t>(system, 0x10) = 0;
        ts_system = system;
    } else {
        ts_system = nullptr;
    }
    const char* const names[5] = {ts_trespassing, ts_lateForClass, ts_skippingClass, ts_curfew, ts_misconduct};
    const int offsets[5] = {0x150, 0x144, 0x148, 0x14C, 0x154};
    alignas(8) uint8_t parts[5][2][4];   // {root, crime} each
    alignas(8) uint8_t paths[5][8];
    for (int i = 0; i < 5; i++) {
        Name_Copy(parts[i][0], ts_rootName);
        Name_FromText(parts[i][1], names[i]);
        Path_Build(paths[i], 2, parts[i]);
        At<uint32_t>(ts_system, offsets[i]) = Config_GetHash(paths[i], ts_tableType);
    }
    for (int i = 4; i >= 0; i--) {
        Name_Destruct(parts[i][1]);
        Name_Destruct(parts[i][0]);
    }
}

// 0x44E6A0: made if needed, then reset
void __cdecl Truancy_Reset() {
    Truancy_CreateFn();
    At<uint8_t>(ts_system, 2) = 0;
    At<int32_t>(ts_system, 4) = 0;
    At<int32_t>(ts_system, 0x14) = 0;
    At<int32_t>(ts_system, 0x18) = 0;
    At<int32_t>(ts_system, 0x1C) = 0;
    At<int32_t>(ts_system, 0x20) = 0;
    Ref_Set(ts_system + 0x10, nullptr);
    At<float>(ts_system, 0x13C) = 1.0f;
    At<uint8_t>(ts_system, 0) = 0;
    At<float>(ts_system, 0x140) = 0.0f;
    At<uint8_t>(ts_system, 1) = 0;
    At<int32_t>(ts_system, 8) = ModelIndexFromName(ts_orderly01, *reinterpret_cast<int32_t*>(0x00A136C8),
                                                   *reinterpret_cast<int32_t*>(0x00A136CC));
    At<int32_t>(ts_system, 0xC) = 0;
    At<int32_t>(ts_system, 0x30) = 100;
    At<int32_t>(ts_system, 0x34) = 200;
    At<int32_t>(ts_system, 0x38) = 300;
    Truancy_InitCrimeValuesFn(ts_system);
    At<int32_t>(ts_system, 0x2C) = 0x2E;
}

// 0x44EAD0: the same (0x44E450 then 0x44E6A0)
void __cdecl Truancy_CreateAndReset() {
    Truancy_CreateFn();
    Truancy_ResetFn();
}

// 0x44E780 (thiscall): the vehicle's driver sent after the target (a car: 0x41A340; a
// bike: an attack objective)
void __fastcall Truancy_SendVehicle(uint8_t* self, re::Edx, uint8_t* vehicle) {
    if (!vehicle) {
        return;
    }
    uint8_t* seats = vehicle + 0x1C4;
    if (!Seats_GetOccupant(seats, 0)) {
        return;
    }
    int32_t kind = At<int32_t>(vehicle, 0x46C);
    At<uint8_t>(vehicle, 0x2EE) |= 1;
    if (kind == 0) {
        Vehicle_Fn41A340(vehicle, At<void*>(ts_system, 0x10), 6.0f, 15.0f);
        At<uint8_t>(vehicle, 0x36D) = 1;
        return;
    }
    if (kind == 1) {
        void* mem = Malloc(0x1C);
        void* objective = nullptr;
        if (mem) {
            void* target = At<void*>(self, 0x10);
            objective = Objective_Construct(mem, Seats_GetOccupant(seats, 0), target, 2);
        }
        uint8_t* driver = static_cast<uint8_t*>(Seats_GetOccupant(seats, 0));
        Objectives_Add(driver + 0x5F0, objective);
    }
    At<uint8_t>(vehicle, 0x36D) = 1;
}

// 0x44E850 (stdcall; a thiscall that ignores ecx): a crime's amount limited (as
// 0x44E2D0, by the rule's +0x1C), reported for kind-13 peds, and given to the ped (vf45)
void __fastcall Truancy_Punish(void*, re::Edx, uint8_t* ped, int32_t* amount, uint8_t* rule, int report, int weaponViolence) {
    if (rule) {
        int32_t max = At<int32_t>(rule, 0x1C);
        int32_t have = At<int32_t>(ped, 0x1D40);
        if (have > max) {
            *amount = 0;
        } else if (*amount + have > max) {
            *amount = max - have;
        }
    }
    if (uint8_t(report) && At<int32_t>(ped, 0x1310) == 0xD) {
        if (uint8_t(weaponViolence)) {
            Crimes_Report(ts_crimes, ts_crimeWeaponViolence, *amount, 1);
        } else if (rule) {
            Crimes_Report(ts_crimes, At<const char*>(rule, 0x2C), *amount, 1);
        }
    }
    int32_t value = *amount;
    if (value > 0) {
        vc::Call<void>(ped, 0xB4, value);
    }
}

// 0x44E8E0 (stdcall; a thiscall that ignores ecx): the ped is somewhere it shouldn't be
uint8_t __fastcall Truancy_IsOutOfBounds(void*, re::Edx, uint8_t* ped) {
    if (!TruancyKind(At<int32_t>(ped, 0x1310)) || At<int32_t>(ped, 0x14E8) == 0) {
        return 0;
    }
    if (At<uint8_t>(Ped_Fn477DC0(ped), 0x50) && ts_currArea == 0xE) {
        return 1;
    }
    if (!At<uint8_t>(Ped_Fn477DC0(ped), 0x50) && ts_currArea == 0x23) {
        return 1;
    }
    bool check = Fn425E40(7, 0x13, 0, 0) != 0;
    int32_t area = ts_currArea;
    if ((!check && area == 2) || area == 0x26 || area == 0x17) {
        return 1;
    }
    if (Zones_Fn46F190(reinterpret_cast<void*>(0x00C2AB70), ped, &EntityPosition(ped))) {
        return 1;
    }
    return 0;
}

// 0x44E990 (stdcall; a thiscall that ignores ecx): the rule applies to the ped (no
// ped: the rule is of kind 0)
uint8_t __stdcall Rule_Matches(uint8_t* rule, uint8_t* ped) {
    if (!ped) {
        return At<int32_t>(rule, 0x24) == 0;
    }
    switch (At<int32_t>(rule, 0x24)) {
    case 0:
        return 1;
    case 1:
        if (At<uint8_t>(Ped_Fn477DC0(ped), 0x50)) {
            return 0;
        }
        return Ped_IsTruancyAgeFn(ped) ? 1 : 0;
    case 2:
        if (!At<uint8_t>(Ped_Fn477DC0(ped), 0x50)) {
            return 0;
        }
        return Ped_IsTruancyAgeFn(ped) ? 1 : 0;
    case 3: {
        int32_t kind = At<int32_t>(ped, 0x1310);
        return (kind == 9 || kind == 0xA) ? 1 : 0;
    }
    case 4:
        return Ped_Fn41AA50(ped);
    case 5:
        return Ped_IsOldCivilianFn(ped);
    case 6:
        return Ped_Fn44E400Fn(ped);
    default:
        return 0;
    }
}

// 0x44EAE0 (thiscall): an orderly's vehicle made on a road near the player (at least 55
// from the target, nothing in the way), driven along it towards the target
void __fastcall Truancy_SpawnVehicle(uint8_t* self, re::Edx, int modelIndex) {
    uint8_t* target = At<uint8_t*>(self, 0x10);
    if (!target || modelIndex == -1) {
        return;
    }
    CVector targetPos = EntityPosition(target);
    float t = 0.0f;
    CVector nodePos = *reinterpret_cast<const CVector*>(0x00BCC564);
    re::Ptr32<void> nodeA = nullptr;
    re::Ptr32<void> nodeB = nullptr;
    int16_t colliding = 0;
    CVector dirBuffer;
    CVector* dir = Camera_Fn41AA70(reinterpret_cast<void*>(0x00C3CC68), &dirBuffer);
    CVector dirFlat = {dir->x, dir->y, 0.0f};
    const CVector& playerPos = EntityPosition(ts_playerPed);
    if (!Paths_FindNodeNear(reinterpret_cast<void*>(0x00BF6228), playerPos.x, playerPos.y, playerPos.z, dirFlat.x,
                            dirFlat.y, dirFlat.z, 65.0f, 1.0f, 0, &nodePos, &nodeA, &nodeB, &t, 0, 1)) {
        return;
    }
    float lanesA = float(At<uint8_t>(nodeA, 0x16) & 0xF);
    float lanesB = float(At<uint8_t>(nodeB, 0x16) & 0xF);
    float lanes = lanesB <= lanesA ? lanesB : lanesA;
    if (int(lanes) == 0) {
        return;
    }
    uint8_t found;
    float groundZ = World_FindGroundZ(nodePos.x, nodePos.y, float(nodePos.z + 2.0), &found, 1);
    if (!found) {
        return;
    }
    uint8_t* col = At<uint8_t*>(ts_modelInfoPtrs[modelIndex], 0xC);
    float spawnZ = float(At<float>(col, 0x18) * -1.0 + groundZ);   // on the ground by its bounding box
    nodePos.z = spawnZ;
    World_FindObjectsKindaColliding(&nodePos, 10.0f, 1, &colliding, 2, 0, 6);
    CVector toTarget;
    Vector_Sub(&toTarget, &nodePos, &targetPos);
    float dist = Vector_Magnitude(&toTarget);
    if (colliding != 0 || dist < 55.0) {
        return;
    }
    bool bike = At<int32_t>(ts_modelInfoPtrs[modelIndex], 0x50) == 1;
    uint8_t* driver = static_cast<uint8_t*>(Population_AddPed(ts_population, 7, bike ? ts_orderlyModelB : ts_orderlyModelA));
    if (!driver) {
        return;
    }
    uint8_t* vehicle;
    if (At<int32_t>(ts_modelInfoPtrs[modelIndex], 0x50) == 1) {
        void* mem = Vehicle_New(0x7A4);
        if (!mem) {
            return;
        }
        vehicle = Bike_Construct(mem, modelIndex, 1);
    } else {
        void* mem = Vehicle_New(0x8C0);
        if (!mem) {
            return;
        }
        vehicle = Automobile_Construct(mem, modelIndex, 1);
    }
    if (!vehicle) {
        return;
    }
    Fn5F2B00(At<void*>(vehicle, 0x18), 1);
    World_Add(vehicle);
    ModelInfo_ChooseColours(ts_modelInfoPtrs[At<int16_t>(vehicle, 0x10E)], vehicle + 0x334, vehicle + 0x335);
    uint8_t* autoPilot = vehicle + 0x2A8;
    At<int32_t>(autoPilot, 8) = 0;
    At<uint8_t>(vehicle, 0x2D7) = 0;
    At<uint8_t>(autoPilot, 0x30) = 0;
    At<int32_t>(autoPilot, 0) = Node_Fn43F8A0(nodeA);
    AutoPilot_Fn5AB1A0(autoPilot, Node_Fn43F8A0(nodeB));
    At<float>(autoPilot, 0x38) = 5.0f;
    At<uint8_t>(autoPilot, 0x3C) = 5;
    At<uint8_t>(autoPilot, 0x33) = 0;
    At<uint8_t>(autoPilot, 0x31) = 2;
    Fn41BF30(vehicle);
    if (At<int32_t>(vehicle, 0x46C) == 0) {
        // a car: put part way along the link (by its length), moving at its speed
        uint8_t* carCol = At<uint8_t*>(ts_modelInfoPtrs[At<int16_t>(vehicle, 0x10E)], 0xC);
        float length = At<float>(carCol, 0x24) - At<float>(carCol, 0x14);
        float halfLength = float(length * 0.5 + 1.0);
        CVector bufferB, bufferA, link;
        CVector* posB = Node_GetPosition(nodeB, &bufferB);
        CVector* posA = Node_GetPosition(nodeA, &bufferA);
        Vector_Sub(&link, posA, posB);
        float linkLength = Vector_Magnitude(&link);
        float along;
        if (halfLength <= linkLength * 0.5) {
            double r = double(halfLength) / linkLength;
            float oneMinusR = float(1.0 - r);
            along = Min(Max(t, float(r)), oneMinusR);
        } else {
            along = 0.5f;
        }
        t = along;
        void* next = AutoPilot_Fn4178E0(autoPilot);
        void* current = AutoPilot_Fn417900(autoPilot);
        float dirAX, dirAY, dirBX, dirBY;
        Node_Fn43F970(current, &dirAX, &dirAY, int8_t(At<uint8_t>(autoPilot, 0x2D)));
        Node_Fn43F970(next, &dirBX, &dirBY, int8_t(At<uint8_t>(autoPilot, 0x2E)));
        CVector laneA, laneB;
        Node_Fn4403C0(current, &laneA, dirAX, dirAY, int8_t(At<uint8_t>(autoPilot, 0x2F)), 5.0f);
        Node_Fn4403C0(next, &laneB, dirBX, dirBY, int8_t(At<uint8_t>(autoPilot, 0x30)), 5.0f);
        float curveLength = Curve_Length(&laneA, &laneB, dirAX, dirAY, dirBX, dirBY);
        double msPerUnit = (1.0 / At<float>(autoPilot, 0x38)) * 1000.0;
        uint32_t duration = uint32_t(int64_t(curveLength * msPerUnit));
        At<uint32_t>(autoPilot, 0x10) = duration;
        float durationF = float(double(duration));
        uint32_t start = uint32_t(int64_t(double(ts_timeInMs) - durationF * (t + 0.5)));
        At<uint32_t>(autoPilot, 0xC) = start;
        uint32_t elapsed = ts_timeInMs - start;
        CVector dirA = {dirAX, dirAY, 0.0f};
        CVector dirB = {dirBX, dirBY, 0.0f};
        CVector speed = {0.0f, 0.0f, 0.0f};
        float progress = float((1.0 / durationF) * double(elapsed));
        Curve_Point(&laneA, &laneB, &dirA, &dirB, progress, duration, &nodePos, &speed);
        const double perFrame = *reinterpret_cast<const double*>(0x00900DD0);   // 1/30
        CVector moveSpeed = {float(speed.x * perFrame), float(speed.y * perFrame), float(speed.z * perFrame)};
        Physical_SetMoveSpeed(vehicle, &moveSpeed);
    }
    nodePos.z = spawnZ;
    CVector bufferA2, bufferB2, heading;
    CVector* posA = Node_GetPosition(nodeA, &bufferA2);
    CVector* posB = Node_GetPosition(nodeB, &bufferB2);
    Vector_Sub(&heading, posB, posA);
    float angle = Vector_Heading(&heading);
    vc::Call<void>(vehicle, 0xD4, nodePos.x, nodePos.y, nodePos.z, angle);   // placed, facing along the link
    Fn5F2B00(At<void*>(vehicle, 0x18), 1);
    int status;
    if (At<int32_t>(vehicle, 0x46C) == 0) {
        Ped_Fn49A580(driver, vehicle, 1);
        status = 2;
    } else {
        Ped_Fn47D0D0(driver, vehicle);
        status = 3;
    }
    vc::Call<void>(vehicle, 0x18, status);   // SetStatus
    Truancy_SendVehicleFn(self, vehicle);
}

// 0x44F2B0 (thiscall): the first of the config's rules that fits the incident (the
// system's rule set, the incident's +0 and +0x28, then its vehicle's or ped's kind)
uint8_t* __fastcall Truancy_FindRule(uint8_t* self, re::Edx, uint8_t* incident) {
    alignas(8) uint8_t path[8];
    Path_Build(path, 1, ts_rootName);
    At<int32_t>(self, 0x28) = Config_Count(path);
    for (int i = 0; i < At<int32_t>(self, 0x28); i++) {
        uint8_t* rule = Config_GetAt(path, i, ts_tableType);
        if (At<uint8_t>(self, 2) != At<uint8_t>(rule, 0x21) || At<int32_t>(incident, 0) != At<int32_t>(rule, 0x18) ||
            At<uint8_t>(incident, 0x28) != At<uint8_t>(rule, 0x20)) {
            continue;
        }
        uint8_t* entity = At<uint8_t*>(incident, 8);
        if (entity && (At<uint8_t>(entity, 0x108) & 7) == 2) {   // a vehicle
            return rule;
        }
        uint8_t* ped = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(0x004A5CD0)(incident);
        if (ped) {
            int32_t model = At<int16_t>(ped, 0x10E);
            if (model == ts_excludedModel1 || model == ts_excludedModel2 || model == ts_excludedModel3 ||
                model == ts_excludedModel4) {
                continue;
            }
        } else {
            ped = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(0x004A5CB0)(incident);
            if (!ped) {
                continue;
            }
        }
        if (Rule_MatchesFn(rule, ped)) {
            return rule;
        }
    }
    return nullptr;
}

#define C04A_TRU(name, address, fn)                                                            \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04A_TRU("CPed::IsOldCivilian", 0x0044E160, &Ped_IsOldCivilian);
C04A_TRU("CTruancy::SetTimes", 0x0044E1E0, &Truancy_SetTimes);
C04A_TRU("CTruancy::SetCrimeValue", 0x0044E200, &Truancy_SetCrimeValue);
C04A_TRU("CTruancy::SetDefaultTimes", 0x0044E210, &Truancy_SetDefaultTimes);
C04A_TRU("CTruancy::InitCrimeValues", 0x0044E230, &Truancy_InitCrimeValues);
C04A_TRU("CTruancy::SetTarget", 0x0044E280, &Truancy_SetTarget);
C04A_TRU("CTruancy::ClampAmount", 0x0044E2D0, &Truancy_ClampAmount);
C04A_TRU("CTruancy::Set13C", 0x0044E310, &Truancy_Set13C);
C04A_TRU("CTruancy::CurfewFactor", 0x0044E360, &Truancy_CurfewFactor);
C04A_TRU("CTruancy::Set2C", 0x0044E3C0, &Truancy_Set2C);
C04A_TRU("CPed::IsTruancyAge", 0x0044E3D0, &Ped_IsTruancyAge);
C04A_TRU("CPed::Fn44E400", 0x0044E400, &Ped_Fn44E400);
C04A_TRU("CTruancy::Create", 0x0044E450, &Truancy_Create);
C04A_TRU("CTruancy::Reset", 0x0044E6A0, &Truancy_Reset);
C04A_TRU("CTruancy::SendVehicle", 0x0044E780, &Truancy_SendVehicle);
C04A_TRU("CTruancy::Punish", 0x0044E850, &Truancy_Punish);
C04A_TRU("CTruancy::IsOutOfBounds", 0x0044E8E0, &Truancy_IsOutOfBounds);
C04A_TRU("CTruancy::RuleMatches", 0x0044E990, &Rule_Matches);
C04A_TRU("CTruancy::CreateAndReset", 0x0044EAD0, &Truancy_CreateAndReset);
C04A_TRU("CTruancy::SpawnVehicle", 0x0044EAE0, &Truancy_SpawnVehicle);
C04A_TRU("CTruancy::FindRule", 0x0044F2B0, &Truancy_FindRule);

} // namespace
