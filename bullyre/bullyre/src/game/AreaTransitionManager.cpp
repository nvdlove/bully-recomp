// AreaTransitionManager: moving the player between areas (interiors) through doors:
// the transition's target position/heading/area, the props on the routes of the
// current area, the door models near the target streamed in.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01atm {

struct CVector {
    float x, y, z;
};

using re::At;

// The manager (0x8F0 bytes; the fields by offset, most of them unnamed yet):
//   +0 vtable (0x900A90), +8 route props counted, +0x10 [100] {trigger, route},
//   +0x330 [100] a flag per route trigger, +0x394 a target (door) and +0x398,
//   +0x39C [300] the objects made along routes, +0x84C their count,
//   +0x864 the transition's state (4: fading, +0x8EC of +0x8E0 steps),
//   +0x86C a transition requested, +0x874 its position set: +0x878 position,
//   +0x884 heading, +0x888 area, +0x88C a direction given (+0x890), +0x8C4 the
//   requested position, +0x8D0 the player's position then.
struct AreaTransitionManager;
inline re::Ptr32<void>* const AreaTransitionManager_vftable = reinterpret_cast<re::Ptr32<void>*>(0x00900A90);

GAME_VAR(uint8_t*, PED_player, 0x00C1AEA8);   // CWorld::Players[0].m_pPed
GAME_VAR(int32_t, currArea, 0x00BD1008);      // CGame::currArea
GAME_VAR(CVector, vecUnset, 0x00BCC564);
GAME_VAR(uint16_t, ms_nCurrentScanCode, 0x00C1AE88);
GAME_ARRAY(uint32_t*, ms_aSectorObjectLists, 0x00C1B17C);   // CWorld::ms_aSectors[36 * 36] (0x14 each) .lists[1]
GAME_VAR(uint32_t*, ms_pPtrNodes, 0x00C0F788);
GAME_VAR(uint8_t, lockButtonInputsExcept, 0x00C1AF8C);
GAME_VAR(uint8_t*, ms_pTriggerPool, 0x020C7C48);   // a CPool (objects +0, flags +4, element size +0xC)

inline void* const theRoutes = reinterpret_cast<void*>(0x00BC5E7C);
inline void* const triggerPoolWrapper = reinterpret_cast<void*>(0x020C7E98);
inline void* const userInputManager = reinterpret_cast<void*>(0x00A45A30);
inline void* const theAnimManager = reinterpret_cast<void*>(0x020C4910);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(void, __thiscall, UserInputManager_SetEnabled3, 0x0050DFD0, void* self, int enabled);
GAME_FN(void, __thiscall, CPed_ClearPartner, 0x00477470, void* ped);
GAME_FN(void, __thiscall, ActionController_Stop, 0x005F40A0, void* controller);
GAME_FN(void, __thiscall, PedIntelligence_5F3A40, 0x005F3A40, void* intelligence);
GAME_FN(void, __thiscall, CPed_UpdateMoveDirection, 0x00474570, void* ped);
GAME_FN(void, __cdecl, DeleteObject_4454F0, 0x004454F0, void* object, int);
GAME_FN(uint32_t, __thiscall, Routes_GetCount, 0x006D62F0, void* routes);
GAME_FN(uint8_t*, __thiscall, Routes_Get, 0x006D6450, void* routes, uint32_t index);
GAME_FN(int, __thiscall, Route_GetNumPoints, 0x006D5950, void* route);
GAME_FN(uint8_t*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(uint8_t*, __thiscall, Route_Next, 0x006D59F0, void* route);
GAME_FN(uint8_t, __cdecl, RoutePointIsDone_443A10, 0x00443A10, int model, void* point);
GAME_FN(void*, __cdecl, CreateRouteObject_443FA0_x64, 0x00443FA0, int, float, float, float, int model, void* info);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void* CreateRouteObject_443FA0(int a0, CVector a1, int a2, void* a3) {
    return CreateRouteObject_443FA0_x64(a0, a1.x, a1.y, a1.z, a2, a3);
}
GAME_FN(int16_t, __thiscall, TriggerPool_GetSize, 0x006D70A0, void* self);
GAME_FN(void*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, int slot);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(void, __thiscall, Effects_668150, 0x00668150, void* self);
GAME_FN(void, __thiscall, Fn_454240, 0x00454240, void* self, int area);
GAME_FN(void, __thiscall, Fn_561040, 0x00561040, void* self);
GAME_FN(void, __thiscall, Camera_Init, 0x004F3720, void* camera);
GAME_VAR(void*, ms_pEffects_B6F1D4, 0x00B6F1D4);
GAME_FN(void, __thiscall, VehicleSeats_RemoveDriver, 0x004CE960, void* seats);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(void, __cdecl, FindSafePlace_48B510, 0x0048B510, CVector* pos, float* heading, uintptr_t);
GAME_FN(void*, __thiscall, Clump_GetAnim_6B1F50, 0x006B1F50, void* clump, int);
GAME_FN(void, __thiscall, AnimManager_6C0D40, 0x006C0D40, void* self, void* clump, uintptr_t, void* anim, int, float, float);
GAME_FN(void, __thiscall, AnimManager_6C0E60, 0x006C0E60, void* self, void* clump, void* anim, int, float);
GAME_FN(void, __thiscall, ATM_RequestTransition, 0x00415790, AreaTransitionManager* self, const CVector* pos, float heading,
        const int32_t* area, const CVector* direction);

// the placeable's position and heading (inlined CPlaceable accessors)
CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}
float HeadingOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    if (matrix) {
        return static_cast<float>(std::atan2(-static_cast<double>(At<float>(matrix, 0x10)), static_cast<double>(At<float>(matrix, 0x14))));
    }
    return At<float>(placeable, 0x10);
}

// 0x415290 (thiscall)
void __fastcall ClearTarget(AreaTransitionManager* self, re::Edx) {
    At<int32_t>(self, 0x394) = 0;
    At<int32_t>(self, 0x398) = 0;
    At<int32_t>(self, 0x864) = 0;
}

// 0x4152B0 (thiscall): the door being used (the input manager told when there is one)
void __fastcall SetTarget(AreaTransitionManager* self, re::Edx, int target) {
    if (target != 0) {
        UserInputManager_SetEnabled3(userInputManager, 0);
    }
    int old = At<int32_t>(self, 0x394);
    At<int32_t>(self, 0x394) = target;
    if (old == 0) {
        At<int32_t>(self, 0x398) = 0;
    }
}

// 0x4152F0 (thiscall)
int __fastcall GetTarget(AreaTransitionManager* self, re::Edx) {
    return At<int32_t>(self, 0x394);
}

// 0x415300 (thiscall): how far the fade is, while fading (state 4)
uint8_t __fastcall GetFadeProgress(AreaTransitionManager* self, re::Edx, float* progress) {
    if (At<int32_t>(self, 0x864) == 4) {
        *progress = static_cast<float>(static_cast<double>(At<int32_t>(self, 0x8EC)) / At<int32_t>(self, 0x8E0));
        return 1;
    }
    return 0;
}

// 0x415330: the player freed from whatever it was doing (partner, actions, input lock)
void __cdecl ResetPlayer() {
    uint8_t* ped = PED_player;
    CPed_ClearPartner(ped);
    ActionController_Stop(At<uint8_t*>(ped, 0x2E0) + 0x258);
    PedIntelligence_5F3A40(At<uint8_t*>(ped, 0x2E0));
    CPed_UpdateMoveDirection(ped);
    At<int32_t>(ped, 0x13AC) = 0;
    At<int32_t>(ped, 0x13A8) = 0;
    lockButtonInputsExcept = 0;
}

// 0x415380 (thiscall): a transition going on or requested
uint32_t __fastcall IsBusy(AreaTransitionManager* self, re::Edx) {
    if (At<int32_t>(self, 0x864) == 0 && At<uint8_t>(self, 0x86C) == 0 && At<int32_t>(self, 0x868) == 0 &&
        At<uint8_t>(self, 0x86D) == 0) {
        return 0;
    }
    return 1;
}

// 0x4153B0 (thiscall): the area of the route a trigger is on (0xFF: none)
uint8_t __fastcall GetTriggerRouteArea(AreaTransitionManager* self, re::Edx, void* prop) {
    for (int i = 0; i < At<int32_t>(self, 8); i++) {
        if (At<void*>(self, 0x10 + i * 8) == prop) {
            return At<uint8_t>(At<uint8_t*>(self, 0x14 + i * 8), 0xC);
        }
    }
    return 0xFF;
}

// 0x4153F0 (a static; ecx unused): the kinds in the set
uint8_t __fastcall IsTransitionKind(void*, re::Edx, int kind) {
    switch (static_cast<int8_t>(kind)) {
    case 0x1A:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x21:
    case 0x22:
    case 0x27:
    case 0x2C:
    case 0x2E:
    case 0x32:
    case 0x38:
        return 1;
    default:
        return 0;
    }
}

// 0x415520 (thiscall): the constructor
AreaTransitionManager* __fastcall Construct(AreaTransitionManager* self, re::Edx) {
    At<re::Ptr32<void>*>(self, 0) = AreaTransitionManager_vftable;
    At<int32_t>(self, 4) = 0;
    for (int i = 0; i < 100; i++) {
        At<int32_t>(self, 0x10 + i * 8) = 0;
        At<int32_t>(self, 0x14 + i * 8) = 0;
    }
    At<int32_t>(self, 0x84C) = 0;
    At<float>(self, 0x850) = 0.0f;
    At<int32_t>(self, 0x854) = 0;
    At<uint8_t>(self, 0x85F) = 0;
    At<uint8_t>(self, 0x860) = 0;
    At<uint8_t>(self, 0x86C) = 0;
    At<uint8_t>(self, 0x86D) = 0;
    At<uint8_t>(self, 0x86F) = 0;
    At<uint8_t>(self, 0x870) = 0;
    At<uint8_t>(self, 0x874) = 0;
    for (uint32_t offset = 0x878; offset <= 0x884; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    At<int32_t>(self, 0x888) = 0;
    At<uint8_t>(self, 0x88C) = 0;
    At<float>(self, 0x890) = 0.0f;
    At<float>(self, 0x894) = 0.0f;
    At<float>(self, 0x898) = 0.0f;
    At<uint8_t>(self, 0x89C) = 0;
    for (uint32_t offset = 0x8A0; offset <= 0x8AC; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    At<int32_t>(self, 0x8B0) = 0;
    At<uint8_t>(self, 0x8B4) = 0;
    for (uint32_t offset = 0x8B8; offset <= 0x8D8; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    At<uint8_t>(self, 0x85C) = 1;
    At<uint8_t>(self, 0x85D) = 0;
    At<uint8_t>(self, 0x85E) = 0;
    return self;
}

// 0x415630 (thiscall): the objects along the current area's routes made again
void __fastcall CreateRouteObjects(AreaTransitionManager* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x84C); i++) {
        DeleteObject_4454F0(At<void*>(self, 0x39C + i * 4), 1);
    }
    At<uint32_t>(self, 0x84C) = 0;
    for (uint32_t r = 0; r < Routes_GetCount(theRoutes); r++) {
        uint8_t* route = Routes_Get(theRoutes, r);
        if (!route || Route_GetNumPoints(route) == 0 || static_cast<int8_t>(route[0xC]) != currArea) {
            continue;
        }
        uint8_t* point = Route_First(route);
        while (point) {
            uint8_t* next = Route_Next(route);
            uint16_t model = At<uint16_t>(point, 0x20);
            if (model != 0) {
                uint8_t* info = At<uint8_t*>(point, 0x18);
                if (info[0x18] != 0 || info[0x2C] != 0 || !RoutePointIsDone_443A10(model, point)) {
                    void* object = CreateRouteObject_443FA0(At<int32_t>(route, 8), *reinterpret_cast<CVector*>(point), model, info);
                    At<void*>(self, 0x39C + At<uint32_t>(self, 0x84C) * 4) = object;
                    At<uint32_t>(self, 0x84C)++;
                }
            }
            point = next;
        }
    }
}

// 0x415790 (thiscall): a transition to `pos` (only when none is going on)
void __fastcall RequestTransition(AreaTransitionManager* self, re::Edx, const CVector* pos, float heading, const int32_t* area,
                                  const CVector* direction) {
    uint8_t* ped = PED_player;
    if (At<int32_t>(self, 0x864) != 0 || At<uint8_t>(self, 0x86C) != 0 || At<int32_t>(self, 0x868) != 0 ||
        At<uint8_t>(self, 0x86D) != 0) {
        return;
    }
    CVector target = *pos;
    At<CVector>(self, 0x8C4) = target;
    At<CVector>(self, 0x8D0) = *PositionOf(ped);
    At<float>(self, 0x884) = heading;
    At<CVector>(self, 0x878) = target;
    At<uint8_t>(self, 0x874) = 1;
    At<int32_t>(self, 0x888) = *area;
    At<uint8_t>(self, 0x88C) = direction != nullptr;
    if (direction) {
        At<CVector>(self, 0x890) = *direction;
    }
    At<uint8_t>(self, 0x86C) = 1;
}

// 0x415880 (thiscall): the same without a direction
void __fastcall RequestTransitionNoDirection(AreaTransitionManager* self, re::Edx, const CVector* pos, float heading, const int32_t* area) {
    ATM_RequestTransition(self, pos, heading, area, nullptr);
}

// 0x4158A0 (a static; ecx unused): into another area now (the effects, the world's
// area state, the camera)
void __fastcall SetArea(void*, re::Edx, const int32_t* area) {
    if (currArea != *area) {
        Effects_668150(ms_pEffects_B6F1D4);
    }
    Fn_454240(reinterpret_cast<void*>(0x00C13FD8), *area);
    Fn_561040(reinterpret_cast<void*>(0x00CF1F60));
    Camera_Init(TheCamera);
    currArea = *area;
}

// 0x4158F0: the player's vehicle (in an interior) put 3 units behind the player
void __cdecl PlaceVehicleBehindPlayer() {
    uint8_t* ped = PED_player;
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (!vehicle || currArea == 0) {
        return;
    }
    VehicleSeats_RemoveDriver(vehicle + 0x1C4);
    CVector pos = *PositionOf(ped);
    CVector forward;
    CVector* f = Placeable_GetForward(ped, &forward);
    CVector back = {-f->x, -f->y, 0.0f};
    Vector_Normalise(&back);
    back.x = static_cast<float>(back.x * 3.0);
    back.y = static_cast<float>(back.y * 3.0);
    back.z = static_cast<float>(back.z * 3.0);
    pos.x = back.x + pos.x;
    pos.y = back.y + pos.y;
    pos.z = pos.z + back.z;
    float heading = HeadingOf(vehicle);
    FindSafePlace_48B510(&pos, &heading, 0);
    float vehicleHeading = HeadingOf(vehicle);
    // vf 0xD4: teleport (x, y, z, heading)
    auto teleport = reinterpret_cast<void(__thiscall*)(void*, float, float, float, float)>(static_cast<void*>(At<re::Ptr32<void>*>(vehicle, 0)[0xD4 / 4]));
    teleport(vehicle, pos.x, pos.y, pos.z, vehicleHeading);
    void* clump = At<void*>(vehicle, 0x114);
    void* anim = Clump_GetAnim_6B1F50(clump, 0);
    if (anim) {
        AnimManager_6C0D40(theAnimManager, At<void*>(vehicle, 0x114), 0, anim, 0, 1.0f, -1.0f);
        AnimManager_6C0E60(theAnimManager, At<void*>(vehicle, 0x114), anim, 0, 1.0f);
    }
}

// 0x415AD0 (thiscall): the door models around the transition's target requested
// (the objects of kind 6 in the sectors within 49 units, each once by scan code)
void __fastcall RequestDoorModels(AreaTransitionManager* self, re::Edx) {
    float x = At<float>(self, 0x878);
    float y = At<float>(self, 0x87C);
    float xMax = static_cast<float>(x + 49.0);
    float yMin = static_cast<float>(y - 49.0);
    float yMax = static_cast<float>(y + 49.0);
    float xMin = static_cast<float>(x - 49.0);
    auto low = [](float v) {
        float sector = static_cast<float>(static_cast<int>(v / 50.0 + 18.0));
        return static_cast<int>(sector > 0.0f ? sector : 0.0f);
    };
    auto high = [](float v) {
        float sector = static_cast<float>(static_cast<int>(v / 50.0 + 18.0));
        return static_cast<int>(sector < 35.0f ? sector : 35.0f);
    };
    int minX = low(xMin);
    int minY = low(yMin);
    int maxX = high(xMax);
    int maxY = high(yMax);
    if (ms_nCurrentScanCode < 0xFFFF) {
        ms_nCurrentScanCode++;
    } else {
        ClearScanCodes();
        ms_nCurrentScanCode = 1;
    }
    uint16_t scanCode = ms_nCurrentScanCode;
    for (int sy = minY; sy <= maxY; sy++) {
        for (int sx = minX; sx <= maxX; sx++) {
            uint32_t* node = ms_aSectorObjectLists[(sy * 36 + sx) * 5];
            while (node) {
                uint8_t* entity = static_cast<uint8_t*>(GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF));
                node = (*node >> 18) == 0x3FFF ? nullptr : ms_pPtrNodes + (*node >> 18);
                if (!entity || (entity[0x108] & 7) != 6 || At<uint16_t>(entity, 0x10A) == scanCode) {
                    continue;
                }
                At<uint16_t>(entity, 0x10A) = scanCode;
                if (At<int32_t>(entity, 0x228) != 0 || At<uint16_t>(entity, 0x22C) == 0xFFFF) {
                    continue;
                }
                int id = At<int16_t>(entity, 0x22C) + 0x5D41;
                if (GetStreamingInfo(id)[4] == 2 || GetStreamingInfo(id)[4] == 1) {
                    GetStreamingInfo(id);   // (called again for nothing)
                } else {
                    RequestModel(id, 0x13);
                }
            }
        }
    }
}

// 0x415D80 (thiscall)
void __fastcall ClearFadeState(AreaTransitionManager* self, re::Edx) {
    At<uint8_t>(self, 0x89C) = 0;
    At<float>(self, 0x8A0) = 0.0f;
    At<float>(self, 0x8AC) = 0.0f;
    At<float>(self, 0x8A4) = 0.0f;
    At<float>(self, 0x8A8) = 0.0f;
    At<int32_t>(self, 0x8B0) = 0;
    At<uint8_t>(self, 0x8B4) = 0;
    At<uint8_t>(self, 0x86F) = 0;
}

// 0x416000 (thiscall, vf0): the scalar deleting destructor
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
AreaTransitionManager* __fastcall Destroy(AreaTransitionManager* self, re::Edx, uint8_t flags) {
    At<re::Ptr32<void>*>(self, 0) = AreaTransitionManager_vftable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x416020 (thiscall): back to no transition
void __fastcall Reset(AreaTransitionManager* self, re::Edx) {
    At<int32_t>(self, 0x864) = 0;
    At<int32_t>(self, 8) = 0;
    At<CVector>(self, 0x8C4) = vecUnset;
    At<uint8_t>(self, 0x85C) = 1;
    At<uint8_t>(self, 0x85D) = 0;
    At<uint8_t>(self, 0x86C) = 0;
    At<uint8_t>(self, 0x86D) = 0;
    At<uint8_t>(self, 0x870) = 0;
    At<uint8_t>(self, 0x85F) = 0;
    At<uint8_t>(self, 0x860) = 0;
    At<int32_t>(self, 0x8EC) = 0;
    At<float>(self, 0x8AC) = 0.0f;
    At<float>(self, 0x8A0) = 0.0f;
    At<float>(self, 0x8A4) = 0.0f;
    At<uint8_t>(self, 0x89C) = 0;
    At<float>(self, 0x8A8) = 0.0f;
    At<int32_t>(self, 0x8B0) = 0;
    At<uint8_t>(self, 0x8B4) = 0;
    At<uint8_t>(self, 0x86F) = 0;
    At<uint8_t>(self, 0x8DC) = 0;
    At<int32_t>(self, 0x8E0) = 0xE;
    At<int32_t>(self, 0x8E4) = 0;
    At<int32_t>(self, 0x8E8) = 0xB4;
}

// 0x416100 (thiscall): a transition to `pos` with the player's heading, in the
// current area
void __fastcall RequestTransitionHere(AreaTransitionManager* self, re::Edx, const CVector* pos) {
    float heading = HeadingOf(PED_player);
    ATM_RequestTransition(self, pos, heading, &currArea, nullptr);
}

// 0x416150 (thiscall): the triggers on routes collected: a route with flag 1 takes its
// first prop (flagged 0), one with flag 2 all of them (flagged 1)
void __fastcall CollectRouteTriggers(AreaTransitionManager* self, re::Edx) {
    At<int32_t>(self, 8) = 0;
    for (uint32_t r = 0; r < Routes_GetCount(theRoutes); r++) {
        uint8_t* route = Routes_Get(theRoutes, r);
        if (!route) {
            continue;
        }
        uint32_t flags = At<uint32_t>(route, 0x18);
        if (!(flags & 1) && !(flags & 2)) {
            continue;
        }
        int count = TriggerPool_GetSize(triggerPoolWrapper);
        for (int slot = 0; slot < count; slot++) {
            uint8_t* pool = ms_pTriggerPool;
            if (slot < 0 || (At<uint8_t*>(pool, 4)[slot] & 0x80)) {
                continue;
            }
            uint8_t* prop = At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * slot;
            if (!prop || At<int32_t>(prop, 0x2C) != At<int32_t>(route, 8)) {
                continue;
            }
            int n = At<int32_t>(self, 8);
            At<uint8_t>(self, 0x330 + n) = (flags & 1) ? 0 : 1;
            At<uint8_t*>(self, 0x10 + n * 8) = prop;
            At<uint8_t*>(self, 0x14 + n * 8) = route;
            At<int32_t>(self, 8) = n + 1;
            if (flags & 1) {
                break;
            }
        }
    }
}

} // namespace c01atm

// ---- registration ----------------------------------------------------------------------

#define ATM_REVERSED_C01(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01atm {
ATM_REVERSED_C01("AreaTransitionManager::ClearTarget", 0x00415290, &ClearTarget);
ATM_REVERSED_C01("AreaTransitionManager::SetTarget", 0x004152B0, &SetTarget);
ATM_REVERSED_C01("AreaTransitionManager::GetTarget", 0x004152F0, &GetTarget);
ATM_REVERSED_C01("AreaTransitionManager::GetFadeProgress", 0x00415300, &GetFadeProgress);
ATM_REVERSED_C01("AreaTransitionManager::ResetPlayer", 0x00415330, &ResetPlayer);
ATM_REVERSED_C01("AreaTransitionManager::IsBusy", 0x00415380, &IsBusy);
ATM_REVERSED_C01("AreaTransitionManager::GetTriggerRouteArea", 0x004153B0, &GetTriggerRouteArea);
ATM_REVERSED_C01("AreaTransitionManager::IsTransitionKind", 0x004153F0, &IsTransitionKind);
ATM_REVERSED_C01("AreaTransitionManager::AreaTransitionManager", 0x00415520, &Construct);
ATM_REVERSED_C01("AreaTransitionManager::CreateRouteObjects", 0x00415630, &CreateRouteObjects);
ATM_REVERSED_C01("AreaTransitionManager::RequestTransition", 0x00415790, &RequestTransition);
ATM_REVERSED_C01("AreaTransitionManager::RequestTransition (no direction)", 0x00415880, &RequestTransitionNoDirection);
ATM_REVERSED_C01("AreaTransitionManager::SetArea", 0x004158A0, &SetArea);
ATM_REVERSED_C01("AreaTransitionManager::PlaceVehicleBehindPlayer", 0x004158F0, &PlaceVehicleBehindPlayer);
ATM_REVERSED_C01("AreaTransitionManager::RequestDoorModels", 0x00415AD0, &RequestDoorModels);
ATM_REVERSED_C01("AreaTransitionManager::ClearFadeState", 0x00415D80, &ClearFadeState);
ATM_REVERSED_C01("AreaTransitionManager::~AreaTransitionManager", 0x00416000, &Destroy);
ATM_REVERSED_C01("AreaTransitionManager::Reset", 0x00416020, &Reset);
ATM_REVERSED_C01("AreaTransitionManager::RequestTransitionHere", 0x00416100, &RequestTransitionHere);
ATM_REVERSED_C01("AreaTransitionManager::CollectRouteTriggers", 0x00416150, &CollectRouteTriggers);
} // namespace c01atm
