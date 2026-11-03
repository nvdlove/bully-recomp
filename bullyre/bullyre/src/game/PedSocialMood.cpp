// A ped's social exchanges, continued (0x48E2F0 .. 0x48F1C8): its set-up and
// destruction, the flee objective (type 0x16), and how it feels about another ped: two
// scores (-5..5) from the ped-type tables, the ratings between them, the clique, weapons,
// the model's limits, and the table of moods they index.
//
// The object (CPed +0xB44 for the ped's own): +4 the ped, +0x24 14 action handles, +0x6C /
// +0x70 / +0x74 the other side, +0x100 / +0x104 random leanings, +0x108 / +0x10C from the
// rating to the player, +0x110 / +0x114 adjustments for +0x74, +0x120.. registered peds,
// +0x12C / +0xEC the mood kept, +0x170 26 action handles, +0x200..0x208 the social anims,
// +0x224..0x226 flags.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

inline void* const PSM_input = reinterpret_cast<void*>(0x020C3CA0);
inline uint8_t* const PSM_gifts = reinterpret_cast<uint8_t*>(0x020C3698);   // the gift list (0x14 each, +2 a kind)
inline const int32_t* const PSM_typeTableA = reinterpret_cast<const int32_t*>(0x00A33A68);   // [15][15]
inline const int32_t* const PSM_typeTableB = reinterpret_cast<const int32_t*>(0x00A33E28);   // [15][15]
inline const int8_t* const PSM_modelRangeA = reinterpret_cast<const int8_t*>(0x00A345C0);    // per model: low, high
inline const int8_t* const PSM_modelRangeB = reinterpret_cast<const int8_t*>(0x00A347D0);
inline const int32_t* const PSM_moods = reinterpret_cast<const int32_t*>(0x00A34AD0);        // [a * 11 + b], a, b in -5..5
inline re::Ptr32<void>* const PSM_vtFleeObjective = reinterpret_cast<re::Ptr32<void>*>(0x00911894);
GAME_VAR(uint8_t, PSM_chapter, 0x00A147E0);
GAME_VAR(uint32_t, PSM_timeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, PSM_excludedModel, 0x00BF4020);
GAME_VAR(int32_t, PSM_specialModel1, 0x00BF3F20);
GAME_VAR(int32_t, PSM_specialModel2, 0x00BF3F1C);
GAME_VAR(int32_t, PSM_control1, 0x00A33A54);
GAME_VAR(int32_t, PSM_control2, 0x00A33A58);
GAME_VAR(int32_t, PSM_control3, 0x00A33A5C);
GAME_VAR(int32_t, PSM_control4, 0x00A33A60);
GAME_VAR(uint16_t, PSM_nextClassId, 0x00BF3830);
GAME_VAR(uint32_t, PSM_fleeIdMade, 0x00C2C044);
GAME_VAR(uint16_t, PSM_fleeId, 0x00C2C040);
GAME_VAR(uint32_t, PSM_objectiveIdMade, 0x00C2AFA8);
GAME_VAR(uint16_t, PSM_objectiveId, 0x00C2AFA4);

GAME_FN(int, __thiscall, PSM_InputFindControl, 0x006A9FD0, void* input, void* name);
GAME_FN(uint8_t*, __thiscall, PSM_InputGetControl, 0x006AA7C0, void* input, int id);
GAME_FN(void*, __thiscall, PSM_InputDevice6AA680, 0x006AA680, void* input);
GAME_FN(void*, __thiscall, PSM_InputDevice6AA690, 0x006AA690, void* input);
GAME_FN(uint8_t, __thiscall, PSM_DeviceActive, 0x006AA3F0, void* device);
GAME_FN(uint8_t, __thiscall, PSM_DevicePressed, 0x006AA410, void* device, int id);
GAME_FN(uint8_t*, __thiscall, PSM_PedGetModelInfo, 0x00477DC0, void* ped);
GAME_FN(int*, __thiscall, PSM_ModelInfo522380, 0x00522380, void* info, int* out);
GAME_FN(uint8_t, __thiscall, PSM_Ped44E160, 0x0044E160, void* ped);
GAME_FN(uint8_t, __thiscall, PSM_Obj6CAAC0, 0x006CAAC0, void* self);
GAME_FN(void*, __thiscall, PSM_ObjectiveConstruct, 0x0046FA90, void* self, int type, void* ped);
GAME_FN(void, __thiscall, PSM_ObjectiveDestruct, 0x0046FAF0, void* self);
GAME_FN(void, __cdecl, PSM_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PSM_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, PSM_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, PSM_HandleRelease, 0x005D9C50, void* handle);
GAME_FN(void, __thiscall, PSM_RefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(int, __thiscall, PSM_GiftsFind6A9C70, 0x006A9C70, void* gifts, void* ped);
GAME_FN(int, __thiscall, PSM_Gifts6A9A20, 0x006A9A20, void* gifts, void* ped);
inline int __cdecl PSM_Rand() { return crt::Rand(); }
GAME_FN(int*, __thiscall, PSM_PedRating, 0x00475380, void* ped, int* out, int pedType);
inline void* __cdecl PSM_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(uint8_t, __thiscall, PSM_Ped474A30, 0x00474A30, void* ped);
GAME_FN(uint8_t, __thiscall, PSM_Ped474B80, 0x00474B80, void* ped);
// (the social object's own, through the game's address)
GAME_FN(void, __thiscall, PSM_DropHumiliation, 0x0048DDB0, void* self);
GAME_FN(void, __thiscall, PSM_DropAuthority, 0x0048DD10, void* self);
GAME_FN(void, __thiscall, PSM_DropKiss, 0x0048DE50, void* self);
GAME_FN(int, __thiscall, PSM_Score, 0x0048D770, void* self, int base);
GAME_FN(uint8_t, __cdecl, PSM_Button4, 0x0048E130);

// A control held or pressed (as 0x48DEF0)
bool ControlDown(int32_t control) {
    if (At<uint16_t>(PSM_InputGetControl(PSM_input, control), 2) > 0 || (PSM_InputGetControl(PSM_input, control)[4] & 8)) {
        return true;
    }
    if (PSM_DeviceActive(PSM_InputDevice6AA680(PSM_input)) && PSM_DevicePressed(PSM_InputDevice6AA680(PSM_input), control)) {
        return true;
    }
    return PSM_DeviceActive(PSM_InputDevice6AA690(PSM_input)) && PSM_DevicePressed(PSM_InputDevice6AA690(PSM_input), control);
}

void ReleaseHandles(uint8_t* self, uint32_t first, int count) {
    for (int i = 0; i < count; i++) {
        PSM_HandleRelease(self + first + i * 4);
    }
}

void ReleaseHandlesBackwards(uint8_t* self, uint32_t first, int count) {
    for (int i = count - 1; i >= 0; i--) {
        PSM_HandleRelease(self + first + i * 4);
    }
}

int Clamp5(int value) {
    if (value > 5) {
        return 5;
    }
    if (value < -5) {
        return -5;
    }
    return value;
}

// value clamped into low..high, low winning
int ClampLowFirst(int value, int low, int high) {
    int limited = high < value ? high : value;
    if (low > limited) {
        return low;
    }
    return high < value ? high : value;
}

} // namespace

namespace c07psm {

// 0x48E2F0: the ped's max health (+0x1CA4) scaled by the strongest of four controls held
// (x2, x1.75, x1.5, x1.25)
float __fastcall ScaledMaxHealth(uint8_t* self, re::Edx) {
    if (PSM_control1 == -1) {
        PSM_control1 = PSM_InputFindControl(PSM_input, reinterpret_cast<void*>(0x01B65468));
    }
    if (PSM_control2 == -1) {
        PSM_control2 = PSM_InputFindControl(PSM_input, reinterpret_cast<void*>(0x01B6546C));
    }
    if (PSM_control3 == -1) {
        PSM_control3 = PSM_InputFindControl(PSM_input, reinterpret_cast<void*>(0x01B65470));
    }
    if (PSM_control4 == -1) {
        PSM_control4 = PSM_InputFindControl(PSM_input, reinterpret_cast<void*>(0x01B65474));
    }
    float health = At<float>(At<uint8_t*>(self, 4), 0x1CA4);
    if (ControlDown(PSM_control4)) {
        return health + health;
    }
    if (ControlDown(PSM_control3)) {
        return static_cast<float>(health * 1.75);
    }
    if (ControlDown(PSM_control2)) {
        return static_cast<float>(health * 1.5);
    }
    if (ControlDown(PSM_control1)) {
        return static_cast<float>(health * 1.25);
    }
    return At<float>(At<uint8_t*>(self, 4), 0x1CA4);
}

// 0x48E650
void __fastcall Count21C(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x21C) += 1;
}

// 0x48E660: the ped's model is of kind 3 (0x522380) and not 0x44E160
uint32_t __fastcall IsKind3(uint8_t* self, re::Edx) {
    int out;
    if (*PSM_ModelInfo522380(PSM_PedGetModelInfo(At<void*>(self, 4)), &out) == 3 && !PSM_Ped44E160(At<void*>(self, 4))) {
        return 1;
    }
    return 0;
}

// 0x48E6A0: the ped's 0x44E160
uint8_t __fastcall Ped44E160(uint8_t* self, re::Edx) {
    return PSM_Ped44E160(At<void*>(self, 4));
}

// 0x48E6B0: 0x20C5F70's 0x6CAAC0, but never for the excluded model
uint8_t __fastcall Check6CAAC0(uint8_t* self, re::Edx) {
    uint8_t* ped = At<uint8_t*>(self, 4);
    if (ped && At<int16_t>(ped, 0x10E) == PSM_excludedModel) {
        return 0;
    }
    return PSM_Obj6CAAC0(reinterpret_cast<void*>(0x020C5F70));
}

// 0x48E6E0 (cdecl): the objective if it's of type 0xA
uint8_t* __cdecl AsType10(uint8_t* objective) {
    if (!objective) {
        return objective;
    }
    return At<int32_t>(objective, 8) == 0xA ? objective : nullptr;
}

// 0x48E700: the flee objective (from `from`, registered at +0x10)
uint8_t* __fastcall FleeConstruct(uint8_t* self, re::Edx, void* ped, void* from) {
    PSM_ObjectiveConstruct(self, 0x16, ped);
    At<re::Ptr32<void>*>(self, 0) = PSM_vtFleeObjective;
    At<void*>(self, 0x10) = from;
    if (from) {
        PSM_RegisterReference(from, self + 0x10);
    }
    return self;
}

// 0x48E740 (vf1): the flee objective's IsA
uint8_t __fastcall FleeIsA(uint8_t*, re::Edx, uint16_t id) {
    if (!(PSM_fleeIdMade & 1)) {
        uint16_t next = PSM_nextClassId;
        PSM_fleeIdMade |= 1;
        PSM_fleeId = next;
        PSM_nextClassId = static_cast<uint16_t>(next + 1);
    }
    if (id == PSM_fleeId) {
        return 1;
    }
    if (!(PSM_objectiveIdMade & 1)) {
        uint16_t next = PSM_nextClassId;
        PSM_objectiveIdMade |= 1;
        PSM_objectiveId = next;
        PSM_nextClassId = static_cast<uint16_t>(next + 1);
    }
    return id == PSM_objectiveId ? 1 : 0;
}

// 0x48E7B0 (vf0): the flee objective's deleting destructor
uint8_t* __fastcall FleeDeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<re::Ptr32<void>*>(self, 0) = PSM_vtFleeObjective;
    re::Ptr32<void>& from = At<void*>(self, 0x10);
    if (from) {
        PSM_CleanUpOldReference(from, &from);
    }
    PSM_ObjectiveDestruct(self);
    if (flags & 1) {
        PSM_Free(self);
    }
    return self;
}

// 0x48E7F0: the destructor (handles released, anims dropped, references cleared)
void __fastcall Destruct(uint8_t* self, re::Edx) {
    ReleaseHandles(self, 0x170, 26);
    ReleaseHandles(self, 0x24, 14);
    PSM_DropHumiliation(self);
    PSM_DropAuthority(self);
    PSM_DropKiss(self);
    PSM_RefSet(self + 0x210, nullptr);
    ReleaseHandlesBackwards(self, 0x170, 26);
    PSM_RefSet(self + 0x138, nullptr);
    PSM_RefSet(self + 0x128, nullptr);
    PSM_RefSet(self + 0x124, nullptr);
    PSM_RefSet(self + 0x120, nullptr);
    PSM_RefSet(self + 0x11C, nullptr);
    PSM_RefSet(self + 0x74, nullptr);
    PSM_RefSet(self + 0x70, nullptr);
    ReleaseHandlesBackwards(self, 0x24, 14);
}

// 0x48E8E0: set up for a ped: random leanings (none for one in the gift lists; type 9
// leans on the second only), the ratings to the player, the rest cleared
void __fastcall Init(uint8_t* self, re::Edx, void* ped) {
    At<void*>(self, 4) = ped;
    if (PSM_GiftsFind6A9C70(PSM_gifts, ped) == -1 && PSM_Gifts6A9A20(PSM_gifts, At<void*>(self, 4)) == -1) {
        At<int32_t>(self, 0x100) = PSM_Rand() * 2 / 0x7FFF - 1;
        At<int32_t>(self, 0x104) = PSM_Rand() * 2 / 0x7FFF - 1;
    } else {
        At<int32_t>(self, 0x100) = 0;
        At<int32_t>(self, 0x104) = 0;
    }
    int rating;
    At<int32_t>(self, 0x108) = 2 - *PSM_PedRating(At<void*>(self, 4), &rating, 0xD);
    At<int32_t>(self, 0x10C) = *PSM_PedRating(At<void*>(self, 4), &rating, 0xD) - 2;
    if (At<int32_t>(At<uint8_t*>(self, 4), 0x1310) == 9) {
        At<int32_t>(self, 0x100) = 0;
        At<int32_t>(self, 0x104) = PSM_Rand() * 4 / 0x7FFF - 1;
    }
    At<uint8_t>(self, 0x224) &= 4;
    At<uint8_t>(self, 0x225) &= 0xF0;
    At<uint8_t>(self, 0x226) &= 0xEF;
    At<int32_t>(self, 0x110) = 0;
    At<int32_t>(self, 0x114) = 0;
    At<int32_t>(self, 0x118) = 0;
    At<int32_t>(self, 0xE4) = 0x19;
    At<int32_t>(self, 0xE0) = 0x19;
    At<int32_t>(self, 0xE8) = 0x19;
    At<int32_t>(self, 0xFC) = 0;
    At<int32_t>(self, 0x12C) = -1;
    At<int32_t>(self, 0x130) = -1;
    At<int32_t>(self, 0xEC) = 3;
    PSM_Memset(self + 0x78, 0, 0x68);
    for (uint32_t offset = 0x13C; offset <= 0x150; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    At<uint16_t>(self, 0x154) = 0;
    for (uint32_t offset = 0x156; offset <= 0x16A; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    At<uint16_t>(self, 0x16E) = 0;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<uint16_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x16) = 0;
    At<uint32_t>(self, 0x1A) = 0;
    At<uint32_t>(self, 0x1E) = 0;
    At<uint16_t>(self, 0x22) = 0;
    At<uint32_t>(self, 0x5C) = 0;
    At<uint32_t>(self, 0x60) = 0;
    At<uint32_t>(self, 0x64) = 0;
    At<uint16_t>(self, 0x68) = 0;
    for (uint32_t offset = 0x1D8; offset <= 0x1EC; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    At<uint16_t>(self, 0x1F0) = 0;
    At<uint8_t>(self, 0x226) &= 0xF8;
    At<uint8_t>(self, 0x225) &= 0xF;
    uint8_t flags = At<uint8_t>(self, 0x226);
    At<int32_t>(self, 0x134) = 0;
    At<int32_t>(self, 0x1F4) = 0x19;
    At<int32_t>(self, 0x1FC) = 3;
    At<int32_t>(self, 0x200) = -1;
    At<int32_t>(self, 0x204) = -1;
    At<int32_t>(self, 0x208) = -1;
    At<int32_t>(self, 0x20C) = 1900;
    At<int32_t>(self, 0x214) = 0;
    At<int32_t>(self, 0xF8) = 0;
    At<int32_t>(self, 0x218) = 0;
    At<int32_t>(self, 0x21C) = 0;
    At<int32_t>(self, 0x220) = 0;
    int model = At<int16_t>(At<uint8_t*>(self, 4), 0x10E);
    if (model != PSM_specialModel1 && model != PSM_specialModel2) {
        At<uint8_t>(self, 0x226) = flags & 0xF7;
        return;
    }
    At<uint8_t>(self, 0x226) = flags | 8;
}

// 0x48EBA0: less than half a second since +0xF8
uint32_t __fastcall IsRecent(uint8_t* self, re::Edx) {
    return PSM_timeInMs < At<uint32_t>(self, 0xF8) + 500 ? 1 : 0;
}

// 0x48EBC0: the ped's mood towards `other` (the kept one when nothing changed); the two
// scores out when asked
int* __fastcall GetMood(uint8_t* self, re::Edx, int* out, uint8_t* other, int fresh, int* outA, int* outB) {
    bool isFresh = re::Flag(fresh);
    if (!isFresh && At<void*>(self, 0x6C) && At<uint8_t*>(self, 0x70) == other) {
        int kept = At<int32_t>(self, 0x12C);
        if (kept == -1 || At<uint8_t*>(self, 0x74) != other) {
            kept = At<int32_t>(self, 0xEC);
        }
        *out = kept;
        return out;
    }
    if (!other) {
        *out = 3;
        return out;
    }
    if (!isFresh && !outA && !outB && other == At<uint8_t*>(self, 0x74) && At<int32_t>(self, 0x12C) != -1) {
        *out = At<int32_t>(self, 0x12C);
        return out;
    }
    uint8_t* ped = At<uint8_t*>(self, 4);
    int myType = At<int32_t>(ped, 0x1310);
    int otherType = At<int32_t>(other, 0x1310);
    int a = PSM_typeTableA[myType * 15 + otherType];
    int b = PSM_typeTableB[myType * 15 + otherType];
    int rating;
    int rated = (*PSM_PedRating(At<void*>(self, 4), &rating, otherType) - 2) * 3;
    int otherA = PSM_typeTableA[otherType * 15 + At<int32_t>(At<uint8_t*>(self, 4), 0x1310)];
    PSM_PedRating(other, &rating, At<int32_t>(At<uint8_t*>(self, 4), 0x1310));
    int leanA, leanB, otherLean;
    otherType = At<int32_t>(other, 0x1310);
    if (otherType == 13) {
        if (At<int32_t>(At<uint8_t*>(self, 4), 0x1310) == 9) {
            leanA = 0;
            leanB = At<int32_t>(self, 0x104);
            otherLean = 0;
        } else {
            leanA = 1 - PSM_chapter / 3;
            leanB = 0;
            otherLean = 0;
        }
    } else {
        leanA = At<int32_t>(self, 0x100);
        leanB = At<int32_t>(self, 0x104);
        otherLean = At<int32_t>(other, 0xC44);
    }
    if (otherType == 13) {
        if (At<uint8_t>(PSM_PedGetModelInfo(At<void*>(self, 4)), 0x50)) {
            a = a / 2;
        }
    } else if (At<uint8_t>(PSM_PedGetModelInfo(other), 0x50) && !At<uint8_t>(PSM_PedGetModelInfo(At<void*>(self, 4)), 0x50)) {
        otherA = otherA / 2;
        a = a - 1;
        b = b + 2;
    } else if (!At<uint8_t>(PSM_PedGetModelInfo(other), 0x50) && At<uint8_t>(PSM_PedGetModelInfo(At<void*>(self, 4)), 0x50)) {
        otherA = otherA - 1;
        a = a / 2;
    }
    ped = At<uint8_t*>(self, 4);
    if (At<uint8_t>(ped, 0x1570) == 1) {
        int16_t mine = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E);
        if (mine != -1 && At<int16_t>(At<uint8_t*>(other, 0x1D0), 0x10E) == -1) {
            otherA = otherA - 1;
        }
        if (At<int16_t>(At<uint8_t*>(other, 0x1D0), 0x10E) != -1 && mine == -1) {
            a = a - 1;
        }
    }
    int cliqueMine, cliqueOther;
    PSM_ModelInfo522380(PSM_PedGetModelInfo(At<void*>(self, 4)), &cliqueMine);
    PSM_ModelInfo522380(PSM_PedGetModelInfo(other), &cliqueOther);
    a += cliqueMine - cliqueOther;
    otherA -= cliqueMine - cliqueOther;
    int signMine;
    if ((At<uint8_t>(self, 0x225) & 0x20) && At<int32_t>(other, 0x1310) == 13) {
        signMine = -1;
    } else {
        signMine = PSM_Score(self, a) >= 0 ? 1 : -1;
    }
    int signOther;
    if (At<int32_t>(other, 0x1310) == 13) {
        signOther = 1;
    } else {
        signOther = PSM_Score(other + 0xB44, otherA) >= 0 ? 1 : -1;
    }
    b += leanB;
    otherA += otherLean;
    a += signMine + leanA;
    otherType = At<int32_t>(other, 0x1310);
    if (otherType != 13) {
        otherA += signOther;
    }
    int hostile = otherA > 0 ? 0 : otherA;
    b += rated;
    a += -rated - hostile / 2;
    ped = At<uint8_t*>(self, 4);
    if (otherType == 13) {
        a += At<int32_t>(self, 0x108);
        b += At<int32_t>(self, 0x10C);
        int model = At<int16_t>(ped, 0x10E);
        int lowA = PSM_Button4() ? -5 : PSM_modelRangeA[model * 2];
        int lowB;
        if ((At<uint8_t>(self, 0x225) & 0x10) && !(At<uint8_t>(self, 0x225) & 0x20)) {
            lowB = 0;
        } else {
            lowB = PSM_modelRangeB[model * 2];
        }
        int highA = PSM_modelRangeA[model * 2 + 1];
        if (!(At<uint8_t>(self, 0x224) & 0x40) &&
            (PSM_Ped474A30(At<void*>(self, 4)) || PSM_Ped474B80(At<void*>(self, 4)) || At<uint8_t>(At<uint8_t*>(self, 4), 0x1E27))) {
            if (a > 0) {
                a = 0;
            }
            a = ClampLowFirst(a, lowA, highA);
            int limited;
            if (b < 2) {
                b = 2;
                limited = 2;
            } else {
                limited = b > 5 ? 5 : b;
            }
            if (lowB > limited) {
                b = lowB;
            } else if (b > 5) {
                b = 5;
            }
        } else {
            int gift = PSM_GiftsFind6A9C70(PSM_gifts, At<void*>(self, 4));
            if (At<uint8_t>(At<uint8_t*>(self, 4), 0x1570) == 1 && gift != -1) {
                uint16_t kind = At<uint16_t>(PSM_gifts, gift * 0x14 + 2);
                if (kind == 2 || kind == 1 || kind == 0) {
                    b -= 3;
                }
            }
            a = ClampLowFirst(a, lowA, highA);
            b = ClampLowFirst(b, lowB, PSM_modelRangeB[model * 2 + 1]);
        }
    } else if (At<int32_t>(ped, 0x1310) != otherType) {
        int model = At<int16_t>(ped, 0x10E);
        a = ClampLowFirst(a, PSM_modelRangeA[model * 2], PSM_modelRangeA[model * 2 + 1]);
        b = ClampLowFirst(b, PSM_modelRangeB[model * 2], PSM_modelRangeB[model * 2 + 1]);
    }
    uint8_t* kept = At<uint8_t*>(self, 0x74);
    bool adjusted = other == kept && (At<uint8_t>(self, 0x224) & 0x20);
    if (!adjusted) {
        int mine = At<int32_t>(At<uint8_t*>(self, 4), 0x1310);
        int theirs = At<int32_t>(other, 0x1310);
        if ((mine == 9 || mine == 10 || mine == 8 || mine == 7) && ((theirs >= 1 && theirs <= 6) || (theirs >= 0xB && theirs <= 0xD))) {
            a = 0;
            if (b > 3) {
                b = 3;
            } else if (b < 0) {
                b = 0;
            }
        }
    }
    if (other == kept && (At<uint8_t>(self, 0x224) & 0x20)) {
        a += At<int32_t>(self, 0x110);
        b += At<int32_t>(self, 0x114);
    }
    a = Clamp5(a);
    b = Clamp5(b);
    if (outA) {
        *outA = a;
    }
    if (outB) {
        *outB = b;
    }
    int mood = PSM_moods[a * 11 + b];
    if (mood == 3 && other == At<uint8_t*>(self, 0x120)) {
        int a2 = Clamp5(a + signMine * 2);
        b = Clamp5(b - 2);
        mood = PSM_moods[a2 * 11 + b];
    }
    *out = mood;
    return out;
}

} // namespace c07psm

#define PSM_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PSM_REVERSED("PedSocial::ScaledMaxHealth", 0x0048E2F0, &c07psm::ScaledMaxHealth);
PSM_REVERSED("PedSocial::Count21C", 0x0048E650, &c07psm::Count21C);
PSM_REVERSED("PedSocial::IsKind3", 0x0048E660, &c07psm::IsKind3);
PSM_REVERSED("PedSocial::Ped44E160", 0x0048E6A0, &c07psm::Ped44E160);
PSM_REVERSED("PedSocial::Check6CAAC0", 0x0048E6B0, &c07psm::Check6CAAC0);
PSM_REVERSED("PedSocial::AsType10", 0x0048E6E0, &c07psm::AsType10);
PSM_REVERSED("FleeObjective::Construct", 0x0048E700, &c07psm::FleeConstruct);
PSM_REVERSED("FleeObjective::IsA", 0x0048E740, &c07psm::FleeIsA);
PSM_REVERSED("FleeObjective::DeletingDestructor", 0x0048E7B0, &c07psm::FleeDeletingDestructor);
PSM_REVERSED("PedSocial::Destruct", 0x0048E7F0, &c07psm::Destruct);
PSM_REVERSED("PedSocial::Init", 0x0048E8E0, &c07psm::Init);
PSM_REVERSED("PedSocial::IsRecent", 0x0048EBA0, &c07psm::IsRecent);
PSM_REVERSED("PedSocial::GetMood", 0x0048EBC0, &c07psm::GetMood);
