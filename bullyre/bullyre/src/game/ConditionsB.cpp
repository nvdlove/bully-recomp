// The action tree's conditions (continued): the ped's action requests and motion
// intention, health / special / sprint, hits and walls, paths and the vehicle to use.
// 0x5FD6D0..0x5FE3C0.
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cmath>
#include <cstdint>

#include "game/ActionTree.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace acttree;

namespace {

GAME_VAR(uint8_t*, buttonMap_20C88D0, 0x020C88D0);
GAME_VAR(float, wallAngleLimit_B0AAB8, 0x00B0AAB8);   // pi / 2
GAME_VAR(float, pathClearance_911E30, 0x00911E30);    // 0.3

GAME_FN(uint8_t, __cdecl, CompareFloats, 0x005FB8B0, float a, float b, int op);
GAME_FN(uint8_t, __cdecl, CompareInts, 0x005FBCB0, int32_t a, int32_t b, int op);
GAME_FN(uint8_t*, __thiscall, ActionInputs_GetRequest, 0x005F3720, uint8_t* inputs, int button);
GAME_FN(float, __thiscall, ActionRequest_SumTimes, 0x005FBD90, uint8_t* request, int index);
GAME_FN(void, __thiscall, Ped_GetForward_4154A0, 0x004154A0, uint8_t* ped, float* out);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, float* v);
GAME_FN(void, __cdecl, RotateVector2, 0x005FBFB0, float angle, float* v);
GAME_FN(uint8_t*, __cdecl, Anim_GetBlend_731C30, 0x00731C30, uint8_t* clump, int index);
GAME_FN(float, __cdecl, CycleFraction, 0x005FBE60, float at, float length, float start, float end);
GAME_FN(float, __thiscall, Ped_SpecialPerPoint_47CF10, 0x0047CF10, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(float, __cdecl, HeadingOf_433310, 0x00433310, const float* v);
GAME_FN(float, __cdecl, NormaliseAngle_432EE0, 0x00432EE0, float angle);
GAME_FN(uint8_t*, __thiscall, PedList_Nearest_48C060, 0x0048C060, uint8_t* list);
GAME_FN(uint8_t, __cdecl, CanPathTo_5E26A0, 0x005E26A0, const float* from, const float* to, uint32_t mode, uintptr_t, float clearance);
GAME_FN(uint8_t*, __cdecl, FindVehicle_4D33F0, 0x004D33F0, uint8_t* ped, float range, int, uint32_t, int);
GAME_FN(void, __cdecl, Entity_CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __cdecl, Entity_RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, uint8_t* vehicle, int32_t* out);
GAME_FN(void, __thiscall, EntityRef_Set_5D7F10, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(uint8_t*, __thiscall, Seats_GetOccupant_4CE560, 0x004CE560, uint8_t* seats, int seat);
GAME_FN(uint8_t, __thiscall, Seats_Contains_4CEA70, 0x004CEA70, uint8_t* seats, uint8_t* ped);
GAME_FN(void, __thiscall, Ped_SetGrapplePartner, 0x004744B0, uint8_t* ped, uint8_t* partner);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(uint8_t, __thiscall, Ped_CanDrive_41AA50, 0x0041AA50, uint8_t* ped);
GAME_FN(float*, __cdecl, VectorSub, 0x00414B70, float* out, const float* a, const float* b);

uint8_t* Ped(uint8_t* owner) {
    return vc::At<uint8_t*>(owner, 8);
}
uint8_t* Inputs(uint8_t* owner) {
    return vc::At<uint8_t*>(Ped(owner), 0x2E0);
}

// cos (the C runtime's)
float CrtCos(float angle) {
    return crt::Cosf(angle);
}
// sqrt (the C runtime's)
float CrtSqrt(float value) {
    return crt::Sqrtf(value);
}

// a request's state tested (kind 3: released, 2: down, else pressed this frame)
uint32_t RequestState(uint8_t* request, int32_t kind) {
    if (kind == 3) {
        return (request[0] & 3) == 0 ? 1 : 0;
    }
    if (kind == 2) {
        return (request[0] & 3) != 0 ? 1 : 0;
    }
    return request[0] & 1;
}

// 0x5FD6D0 (thiscall): ConditionActionIntentionActionRequest: a button's held time
// compared, then its state (or, +0x14 == 1, pressed before). A byte bool (its xor al, al
// paths leave eax's upper bytes; the callers test al)
uint8_t __fastcall ActionIntentionActionRequest_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* request = ActionInputs_GetRequest(Inputs(owner), vc::At<int32_t>(self, 4));
    float held = ActionRequest_SumTimes(request, vc::At<int32_t>(self, 0x14));
    if (!CompareFloats(held, vc::At<float>(self, 0xC), vc::At<int32_t>(self, 0x10))) {
        return 0;
    }
    int32_t mode = vc::At<int32_t>(self, 0x14);
    if (mode == 0) {
        return uint8_t(RequestState(request, vc::At<int32_t>(self, 8)));
    }
    if (mode == 1 && 0.0f < vc::At<float>(request, 8)) {
        return 1;
    }
    return 0;
}

// 0x5FD780 (thiscall): ConditionIntentionActionNodeRequest: the requested node is the
// owner's node
uint8_t __fastcall IntentionActionNodeRequest_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* inputs = Inputs(owner);
    uint32_t* name = vc::Call<uint32_t*>(vc::At<uint8_t*>(owner, 0x34), kNodeGetName);
    return vc::At<uint32_t>(inputs, 0x254) == *name;
}

// 0x5FD7B0 (thiscall): ConditionActionRequestButton: the same through the button map (a
// byte bool too)
uint8_t __fastcall ActionRequestButton_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int32_t button = vc::At<int32_t>(buttonMap_20C88D0, 0x18 + vc::At<int32_t>(self, 4) * 4);
    uint8_t* request = ActionInputs_GetRequest(Inputs(owner), button);
    float held = ActionRequest_SumTimes(request, vc::At<int32_t>(self, 0x14));
    if (!CompareFloats(held, vc::At<float>(self, 0xC), vc::At<int32_t>(self, 0x10))) {
        return 0;
    }
    if (vc::At<int32_t>(self, 0x14) != 0) {
        return 1;
    }
    return uint8_t(RequestState(request, vc::At<int32_t>(self, 8)));
}

// 0x5FD860 (thiscall): ConditionPedFlags
uint8_t __fastcall PedFlags_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return vc::At<int32_t>(Ped(owner), 0x1314 + vc::At<int32_t>(self, 4) * 4) != 0;
}

// 0x5FD880 (thiscall): ConditionSetPedFlags: a flag set (true)
uint8_t __fastcall SetPedFlags_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    vc::At<int32_t>(Ped(owner), 0x1314 + vc::At<int32_t>(self, 8) * 4) = self[4];
    return 1;
}

// 0x5FD8A0 (thiscall): ConditionPedFightingSystem
uint8_t __fastcall PedFightingSystem_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return vc::At<int32_t>(self, 4) == vc::At<int32_t>(Ped(owner), 0x16DC);
}

// 0x5FD8C0 (thiscall): ConditionMotionIntentionDirection: the stick within +8 radians of
// the ped's forward turned by +4
uint8_t __fastcall MotionIntentionDirection_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* inputs = Inputs(owner);
    float stick[3] = {vc::At<float>(inputs, 0), vc::At<float>(inputs, 4), vc::At<float>(inputs, 8)};
    float forward[3];
    Ped_GetForward_4154A0(Ped(owner), forward);
    forward[2] = 0.0f;
    Vector_Normalise(forward);
    RotateVector2(vc::At<float>(self, 4), forward);
    float dot = forward[1] * stick[1] + stick[0] * forward[0] + stick[2] * forward[2];
    float limit = CrtCos(vc::At<float>(self, 8));
    return double(limit) <= double(dot) ? 1 : 0;
}

// 0x5FD970 (thiscall): ConditionMotionIntentionSpeed
uint8_t __fastcall MotionIntentionSpeed_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return CompareFloats(vc::At<float>(Inputs(owner), 0x1C), vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FD9B0 (thiscall): ConditionMotionCycle: where the walk cycle is compared
uint8_t __fastcall MotionCycle_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* clump = vc::At<uint8_t*>(Ped(owner), 0x114);
    uint8_t* blend = nullptr;
    for (int i = 1; i >= 0; i--) {
        blend = Anim_GetBlend_731C30(clump, i);
        if (blend) {
            break;
        }
    }
    if (!blend) {
        return 0;
    }
    uint8_t* hierarchy = vc::At<uint8_t*>(blend, 0x18);
    if (!hierarchy) {
        return 0;
    }
    uint8_t* extra = vc::At<uint8_t*>(blend, 0x50);
    float fraction = CycleFraction(vc::At<float>(blend, 0x84), vc::At<float>(hierarchy, 0xC), vc::At<float>(extra, 0x20),
                                   vc::At<float>(extra, 0x28));
    return CompareFloats(fraction, vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// a ped's float compared (health 0x1CB8, special 0x18C0, sprint 0x18D0)
template <int Offset>
uint8_t __fastcall PedFloat_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return CompareFloats(vc::At<float>(Ped(owner), Offset), vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FDA90 (thiscall): ConditionPercentageHealth
uint8_t __fastcall PercentageHealth_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    float fraction = float(double(vc::At<float>(ped, 0x1CB8)) / double(vc::At<float>(ped, 0x1CA4)));
    return CompareFloats(fraction, vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FDAD0 (thiscall): ConditionPercentageSpecial: the special (points and charge) out
// of six points' worth, in whole percent
uint8_t __fastcall PercentageSpecial_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    float charge = vc::At<float>(ped, 0x18C0);
    int32_t points = vc::At<int32_t>(ped, 0x18C8);
    float perPoint = Ped_SpecialPerPoint_47CF10(ped);
    float pointsWorth = float(double(perPoint) * double(points));
    int32_t total = int32_t(pointsWorth + charge);
    int32_t full = int32_t(float(double(Ped_SpecialPerPoint_47CF10(Ped(owner))) * 6.0));
    int32_t percent = total / full;
    return CompareFloats(float(percent), vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FDB50 (thiscall): ConditionDamage: more than +4 lost since last time (the mark
// then moved to now)
uint8_t __fastcall Damage_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    float lost = float(double(vc::At<float>(ped, 0x1CBC)) - double(vc::At<float>(ped, 0x1CB8)));
    if (vc::At<float>(self, 4) < lost) {
        vc::At<float>(ped, 0x1CBC) = vc::At<float>(ped, 0x1CB8);
        return 1;
    }
    return 0;
}

// 0x5FDC10 (thiscall): ConditionSpecialPoints
uint8_t __fastcall SpecialPoints_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return CompareInts(vc::At<int32_t>(Ped(owner), 0x18C8), vc::At<int32_t>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FDC40 (thiscall): ConditionHitByPed: the attacker's kind (+4: not a driver or
// rider; +5: one) against +6
uint32_t __fastcall HitByPed_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* attacker = vc::At<uint8_t*>(vc::At<uint8_t*>(owner, 0x10), 4);
    if (!attacker) {
        return self[6] == 0 ? 1 : 0;
    }
    if (self[4]) {
        if (Ped_IsSeatedInVehicle(attacker) ||
            (vc::At<int32_t>(attacker, 0x1310) == 0xD && vc::At<int32_t>(attacker, 0x1E94) != 0)) {
            return self[6] == 0 ? 1 : 0;
        }
    }
    if (self[5] && !Ped_IsSeatedInVehicle(attacker) &&
        !(vc::At<int32_t>(attacker, 0x1310) == 0xD && vc::At<int32_t>(attacker, 0x1E94) != 0)) {
        return self[6] == 0 ? 1 : 0;
    }
    return self[6] == 1 ? 1 : 0;
}

// 0x5FDCC0 (thiscall): ConditionWallHitDirection: the wall's side relative to the ped's
// heading (bits: 2 front, 4 back, 8 left, 0x10 right) against the bit +4
uint8_t __fastcall WallHitDirection_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!vc::At<int32_t>(ped, 0x13A8)) {
        return 0;
    }
    float heading = vc::At<float>(ped, 0x1700);
    float away[3] = {-vc::At<float>(ped, 0x1874), -vc::At<float>(ped, 0x1878), -vc::At<float>(ped, 0x187C)};
    float wall = HeadingOf_433310(away);
    float diff = NormaliseAngle_432EE0(float(double(heading) - double(wall)));
    uint32_t bit = 1u << (vc::At<uint32_t>(self, 4) & 31);
    float size = std::fabs(diff);
    uint8_t dir = wallAngleLimit_B0AAB8 < size || size != size ? 4 : 2;
    dir |= 0.0f < diff ? 8 : 0x10;
    return (uint8_t(bit) & dir) != 0;
}

// 0x5FDDD0 (thiscall): ConditionHitFlag: the hit's flag bit (+4: only a hit with a set
// time not after now; +5: not by the ped's own target)
uint32_t __fastcall HitFlag_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* hit = vc::At<uint8_t*>(owner, 0x10);
    if (self[5] && vc::At<uint8_t*>(Ped(owner), 0x155C) == vc::At<uint8_t*>(hit, 4)) {
        return 0;
    }
    uint8_t bit = uint8_t(1u << (vc::At<uint32_t>(self, 8) & 31));
    if (self[4]) {
        float time = vc::At<float>(hit, 0x2C);
        if (-1.0f == time) {
            return 0;
        }
        if (!(0.0f >= time)) {
            return 0;
        }
        return (hit[0x68] & bit) ? 1 : 0;
    }
    return (bit & hit[0x68]) != 0 ? 1 : 0;
}

// the entity's position (its matrix's, else its placement's)
const float* EntityPos(uint8_t* entity) {
    uint8_t* matrix = vc::At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

// 0x5FDE50 (thiscall): CanPathToEntityCond: a path from the ped to its target (or the
// nearest ped); +4: a stricter mode (the mode byte goes in a dword whose other bytes are
// the owner's address)
uint8_t __fastcall CanPathToEntity_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    const float* p = EntityPos(ped);
    float from[3] = {p[0], p[1], p[2]};
    uint8_t* target = vc::At<uint8_t*>(ped, 0x544);
    if (!target) {
        target = PedList_Nearest_48C060(ped + 0x3BC);
    }
    const float* t = EntityPos(target);
    float to[3] = {t[0], t[1], t[2]};
    uint32_t mode = (uint32_t(reinterpret_cast<uintptr_t>(owner)) & 0xFFFFFF00u) | (self[4] ? 3u : 2u);
    return CanPathTo_5E26A0(from, to, mode, 0, pathClearance_911E30);
}

void SetVehicleRef(uint8_t* ped, uint8_t* vehicle) {
    re::Ptr32<uint8_t>* ref = &vc::At<uint8_t*>(ped, 0x430);
    if (*ref == vehicle) {
        return;
    }
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
    }
    *ref = vehicle;
    Entity_RegisterReference(vehicle, ref);
}

// the vehicle taken: its driver reference set (+0xA), its occupant grappled (+0xB)
void TakeVehicle(uint8_t* self, uint8_t* ped, uint8_t* vehicle) {
    if (self[0xA] && vehicle) {
        EntityRef_Set_5D7F10(&vc::At<uint8_t*>(vehicle, 0x6BC), ped);
    }
    if (self[0xB] && vehicle) {
        uint8_t* occupant = Seats_GetOccupant_4CE560(vehicle + 0x1C4, 0);
        if (occupant) {
            Ped_SetGrapplePartner(ped, occupant);
            Ped_SetGrapplePartner(occupant, ped);
        }
    }
}

// 0x5FDF10 (thiscall): ConditionTargetVehicle: a vehicle to get on (+4 range, +8 any,
// +9 search flags, +0xC not a wreck); on foot a new one is looked for, else the one
// used (or a new one every 8th frame) kept while in range; the ped's +0x430 set to it
uint8_t __fastcall TargetVehicle_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t any = self[8];
    uint8_t notAny = any == 0;
    // (the flag goes in the owner argument's slot: its other bytes stay the owner's address)
    uint32_t flagDword = (uint32_t(reinterpret_cast<uintptr_t>(owner)) & 0xFFFFFF00u) | notAny;
    int32_t kindB, kindC;
    uint8_t* vehicle;
    uint8_t* seatsOf = nullptr;
    if (vc::At<int32_t>(ped, 0x1310) == 0xD) {
        vehicle = FindVehicle_4D33F0(ped, vc::At<float>(self, 4), 1, flagDword, 0);
        if (vehicle) {
            if (self[0xC] && vehicle[0x290] == 2) {
                return 0;
            }
            uint8_t* driver = vc::At<uint8_t*>(vehicle, 0x6BC);
            if (driver && driver != ped && notAny) {
                return 0;
            }
            SetVehicleRef(ped, vehicle);
            int32_t kind;
            if (*Vehicle_GetVehicleClass(vehicle, &kind) == 3 && vc::At<int32_t>(ped, 0x13BC) == 0) {
                return 0;
            }
            if (self[0xA]) {
                EntityRef_Set_5D7F10(&vc::At<uint8_t*>(vehicle, 0x6BC), ped);
            }
            if (self[0xB]) {
                uint8_t* occupant = Seats_GetOccupant_4CE560(vehicle + 0x1C4, 0);
                if (occupant) {
                    Ped_SetGrapplePartner(ped, occupant);
                    Ped_SetGrapplePartner(occupant, ped);
                }
            }
            return 1;
        }
        vehicle = FindVehicle_4D33F0(ped, vc::At<float>(self, 4), 0, 0, self[9]);
        if (!vehicle) {
            return 0;
        }
        if (self[0xC] && vehicle[0x290] == 2) {
            return 0;
        }
        if (vc::Call<uint8_t>(vehicle, 0xD0) && !self[8]) {
            return 0;
        }
        if ((vehicle[0x108] & 0xF8) == 0x10) {
            return 0;
        }
        SetVehicleRef(ped, vehicle);
        if (vc::At<int32_t>(ped, 0x13BC) == 0) {
            int32_t kind;
            if (*Vehicle_GetVehicleClass(vehicle, &kind) != 6 && *Vehicle_GetVehicleClass(vehicle, &kindB) != 0xA &&
                *Vehicle_GetVehicleClass(vehicle, &kindC) != 7) {
                return 0;
            }
        }
        return 1;
    }
    re::Ptr32<uint8_t>* ref = &vc::At<uint8_t*>(ped, 0x430);
    vehicle = *ref;
    if (vehicle) {
        if (any) {
            goto have;
        }
        int32_t used = vc::At<int32_t>(vehicle, 0x46C);
        if (used == 1 && vc::At<uint8_t*>(vehicle, 0x6BC) && vc::At<uint8_t*>(vehicle, 0x6BC) != ped) {
            goto search;
        }
        if (used != 0) {
            goto have;
        }
        uint8_t* seats = vehicle + 0x1C4;
        if (!Seats_GetOccupant_4CE560(seats, used)) {
            goto have;
        }
        if (Seats_GetOccupant_4CE560(seats, 0) == ped) {
            goto have;
        }
    }
search:
    if (!Ped_EveryNthFrame(ped, 8) && self[0xB] == 0) {
        goto fail;
    }
    vehicle = FindVehicle_4D33F0(ped, vc::At<float>(self, 4), -1, flagDword, self[9]);
    if (!vehicle) {
        goto fail;
    }
    if (vc::At<int32_t>(vehicle, 0x46C) == 1) {
        if (ped[0x1570] == 1 && vehicle[0x290] == 2) {
            goto fail;
        }
        int32_t kind;
        if (!Ped_CanDrive_41AA50(ped) && *Vehicle_GetVehicleClass(vehicle, &kind) == 3) {
            goto fail;
        }
    }
    if (!self[8]) {
        int32_t used = vc::At<int32_t>(vehicle, 0x46C);
        if (used == 1 && vc::At<uint8_t*>(vehicle, 0x6BC) && vc::At<uint8_t*>(vehicle, 0x6BC) != ped) {
            goto fail;
        }
        if (used == 0) {
            uint8_t* seats = vehicle + 0x1C4;
            if (Seats_GetOccupant_4CE560(seats, used) && Seats_GetOccupant_4CE560(seats, 0) != ped) {
                goto fail;
            }
        }
    }
    if (*ref != vehicle) {
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
        }
        *ref = vehicle;
        Entity_RegisterReference(vehicle, ref);
    }
have:
    if (self[0xC] && vehicle[0x290] == 2) {
        return 0;
    }
    if (vc::At<int32_t>(vehicle, 0x46C) == 1) {
        seatsOf = vehicle;
        if (Seats_Contains_4CEA70(vehicle + 0x1C4, ped) || vc::At<uint8_t*>(vehicle, 0x6BC) == ped) {
            if (self[0xA]) {
                EntityRef_Set_5D7F10(&vc::At<uint8_t*>(vehicle, 0x6BC), ped);
            }
            return 1;
        }
    } else {
        if (vc::At<int32_t>(vehicle, 0x46C) == 0 && Seats_Contains_4CEA70(vehicle + 0x1C4, ped)) {
            return 1;
        }
        seatsOf = nullptr;
    }
    {
        float d[3];
        float* delta = VectorSub(d, EntityPos(vehicle), EntityPos(ped));
        float lengthSq = std::fabs(delta[0] * delta[0] + delta[1] * delta[1]);
        float length = CrtSqrt(lengthSq);
        if (!(vc::At<float>(self, 4) > length)) {
            return 0;
        }
        TakeVehicle(self, ped, seatsOf);
        return 1;
    }
fail:
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
    }
    *ref = nullptr;
    return 0;
}

} // namespace

#define CONDB_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CONDB_REVERSED("ConditionActionIntentionActionRequest::Evaluate", 0x005FD6D0, &ActionIntentionActionRequest_Evaluate);
CONDB_REVERSED("ConditionIntentionActionNodeRequest::Evaluate", 0x005FD780, &IntentionActionNodeRequest_Evaluate);
CONDB_REVERSED("ConditionActionRequestButton::Evaluate", 0x005FD7B0, &ActionRequestButton_Evaluate);
CONDB_REVERSED("ConditionPedFlags::Evaluate", 0x005FD860, &PedFlags_Evaluate);
CONDB_REVERSED("ConditionSetPedFlags::Evaluate", 0x005FD880, &SetPedFlags_Evaluate);
CONDB_REVERSED("ConditionPedFightingSystem::Evaluate", 0x005FD8A0, &PedFightingSystem_Evaluate);
CONDB_REVERSED("ConditionMotionIntentionDirection::Evaluate", 0x005FD8C0, &MotionIntentionDirection_Evaluate);
CONDB_REVERSED("ConditionMotionIntentionSpeed::Evaluate", 0x005FD970, &MotionIntentionSpeed_Evaluate);
CONDB_REVERSED("ConditionMotionCycle::Evaluate", 0x005FD9B0, &MotionCycle_Evaluate);
CONDB_REVERSED("ConditionHealth::Evaluate", 0x005FDA50, &PedFloat_Evaluate<0x1CB8>);
CONDB_REVERSED("ConditionPercentageHealth::Evaluate", 0x005FDA90, &PercentageHealth_Evaluate);
CONDB_REVERSED("ConditionPercentageSpecial::Evaluate", 0x005FDAD0, &PercentageSpecial_Evaluate);
CONDB_REVERSED("ConditionDamage::Evaluate", 0x005FDB50, &Damage_Evaluate);
CONDB_REVERSED("ConditionSpecial::Evaluate", 0x005FDB90, &PedFloat_Evaluate<0x18C0>);
CONDB_REVERSED("ConditionSprint::Evaluate", 0x005FDBD0, &PedFloat_Evaluate<0x18D0>);
CONDB_REVERSED("ConditionSpecialPoints::Evaluate", 0x005FDC10, &SpecialPoints_Evaluate);
CONDB_REVERSED("ConditionHitByPed::Evaluate", 0x005FDC40, &HitByPed_Evaluate);
CONDB_REVERSED("ConditionWallHitDirection::Evaluate", 0x005FDCC0, &WallHitDirection_Evaluate);
CONDB_REVERSED("ConditionHitFlag::Evaluate", 0x005FDDD0, &HitFlag_Evaluate);
CONDB_REVERSED("CanPathToEntityCond::Evaluate", 0x005FDE50, &CanPathToEntity_Evaluate);
CONDB_REVERSED("ConditionTargetVehicle::Evaluate", 0x005FDF10, &TargetVehicle_Evaluate);
