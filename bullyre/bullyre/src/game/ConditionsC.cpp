// The action tree's conditions (continued): the vehicle to get on (car / bike), what the
// ped's vehicle is, its owner, its wheels on the ground and where things are relative to
// each other. 0x5FE3C0..0x5FF030.
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

GAME_VAR(double, zero_9009A0, 0x009009A0);         // 0.0
GAME_VAR(double, groundLimit_900AD0, 0x00900AD0);  // 0.8f widened

GAME_FN(uint8_t*, __cdecl, FindVehicle_4D33F0c, 0x004D33F0, uint8_t* ped, float range, int, uint32_t, int);
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
GAME_FN(uint32_t, __thiscall, Ped_TryGrabVehicle_479B50, 0x00479B50, uint8_t* ped);
GAME_FN(int32_t*, __thiscall, Ped_GetAttitude_475380, 0x00475380, uint8_t* ped, int32_t* out, int32_t pedType);
GAME_FN(uint8_t*, __thiscall, AnimBlend_GetCurrent_6B1F50, 0x006B1F50, uint8_t* blend, int);
GAME_FN(int32_t, __cdecl, AnimIdOf_607770, 0x00607770, const uint32_t* name, uint8_t* animSet);
GAME_FN(uint8_t*, __thiscall, Ped_GetFightTarget_497100, 0x00497100, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget_4D5AD0, 0x004D5AD0, uint8_t* targeting);
GAME_FN(uint8_t*, __thiscall, Targeting_GetLookTarget_4D7780, 0x004D7780, uint8_t* targeting);
GAME_FN(float*, __thiscall, Physical_GetUp_473730, 0x00473730, uint8_t* entity, float* out);
GAME_FN(float*, __thiscall, Entity_GetForward_4154A0, 0x004154A0, uint8_t* entity, float* out);
GAME_FN(float*, __cdecl, CrossProduct_414730, 0x00414730, float* out, const float* a, const float* b);

uint8_t* Ped(uint8_t* owner) {
    return vc::At<uint8_t*>(owner, 8);
}

// sqrt (the C runtime's)
float CrtSqrtC(float value) {
    return crt::Sqrtf(value);
}

const float* EntityPos(uint8_t* entity) {
    uint8_t* matrix = vc::At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

void SetRef(re::Ptr32<uint8_t>* ref, uint8_t* entity) {
    if (*ref == entity) {
        return;
    }
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
    }
    *ref = entity;
    Entity_RegisterReference(entity, ref);
}

// the same where the new one may be null (no reference registered then)
void SetRefMaybeNull(re::Ptr32<uint8_t>* ref, uint8_t* entity) {
    if (*ref == entity) {
        return;
    }
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
    }
    *ref = entity;
    if (entity) {
        Entity_RegisterReference(entity, ref);
    }
}

void ReleaseRef(re::Ptr32<uint8_t>* ref) {
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
    }
    *ref = nullptr;
}

// |the 2D distance| from b to a, through the x87 and the CRT's sqrt
float Distance2D(const float* delta) {
    float lengthSq = std::fabs(delta[0] * delta[0] + delta[1] * delta[1]);
    return CrtSqrtC(lengthSq);
}

bool IsWreck(uint8_t* vehicle) {
    return (vehicle[0x108] & 0xF8) == 0x10;
}

// 0x5FE3C0 (thiscall): ConditionTargetVehicleCar: a car to get in (+4 range, +8 any,
// +9 search flags); on foot a new one is looked for (only a car, bus or other of the
// classes 6 / 0xA / 7 for a ped without a licence), else the one used is kept (or a
// new one looked for every 8th frame) while within range
uint8_t __fastcall TargetVehicleCar_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    re::Ptr32<uint8_t>* ref = &vc::At<uint8_t*>(ped, 0x430);
    uint8_t* vehicle;
    if (vc::At<int32_t>(ped, 0x1310) == 0xD) {
        vehicle = FindVehicle_4D33F0c(ped, vc::At<float>(self, 4), 0, 0, self[9]);
        if (!vehicle) {
            return 0;
        }
        if (vc::Call<uint8_t>(vehicle, 0xD0) && !self[8]) {
            return 0;
        }
        if (IsWreck(vehicle)) {
            return 0;
        }
        SetRef(ref, vehicle);
        if (vc::At<int32_t>(ped, 0x13BC) == 0) {
            int32_t kindA, kindB, kindC;
            if (*Vehicle_GetVehicleClass(vehicle, &kindA) != 6 && *Vehicle_GetVehicleClass(vehicle, &kindB) != 0xA &&
                *Vehicle_GetVehicleClass(vehicle, &kindC) != 7) {
                return 0;
            }
        }
        return 1;
    }
    vehicle = *ref;
    bool occupied = vehicle && Seats_GetOccupant_4CE560(vehicle + 0x1C4, 0) != nullptr;
    if (vehicle) {
        if (vc::At<int32_t>(vehicle, 0x46C) != 0) {
            goto distance;
        }
        if (Seats_GetOccupant_4CE560(vehicle + 0x1C4, 0) == ped) {
            return 1;
        }
        if (!occupied || self[8]) {
            goto distance;
        }
    }
    // (every 8th frame: its result's upper bytes are 0 here, so the flag's dword is clean)
    if (!Ped_EveryNthFrame(ped, 8)) {
        goto fail;
    }
    vehicle = FindVehicle_4D33F0c(ped, vc::At<float>(self, 4), 0, self[8] == 0 ? 1u : 0u, self[9]);
    if (!vehicle) {
        goto fail;
    }
    if (occupied && !self[8]) {
        goto fail;
    }
    if (IsWreck(vehicle)) {
        goto fail;
    }
    SetRef(ref, vehicle);
distance: {
        float d[3];
        float* delta = VectorSub(d, EntityPos(vehicle), EntityPos(ped));
        if (vc::At<int32_t>(vehicle, 0x46C) != 0) {
            return 0;
        }
        float length = Distance2D(delta);
        return vc::At<float>(self, 4) > length ? 1 : 0;
    }
fail:
    ReleaseRef(ref);
    return 0;
}

// 0x5FE630 (thiscall): ConditionTargetVehicleBike: a bike to get on (+4 range, +8 any,
// +9 search flags, +0xA take it: its rider reference set, +0xB grab its rider, +0xC not a
// wreck); on foot a new one, else the one used (or a new one every 8th frame)
uint8_t __fastcall TargetVehicleBike_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t notAny = self[8] == 0;
    // (the flag goes in the owner argument's slot: its other bytes stay the owner's address)
    uint32_t flagDword = (uint32_t(reinterpret_cast<uintptr_t>(owner)) & 0xFFFFFF00u) | notAny;
    re::Ptr32<uint8_t>* ref = &vc::At<uint8_t*>(ped, 0x430);
    uint8_t* vehicle;
    if (vc::At<int32_t>(ped, 0x1310) == 0xD) {
        vehicle = FindVehicle_4D33F0c(ped, vc::At<float>(self, 4), 1, flagDword, 0);
        if (!vehicle) {
            return 0;
        }
        if (self[0xC] && vehicle[0x290] == 2) {
            return 0;
        }
        uint8_t* rider = vc::At<uint8_t*>(vehicle, 0x6BC);
        if (rider && rider != ped && notAny) {
            return 0;
        }
        SetRef(ref, vehicle);
        int32_t kind;
        if (*Vehicle_GetVehicleClass(vehicle, &kind) == 3 && vc::At<int32_t>(ped, 0x13BC) == 0) {
            return 0;
        }
        if (self[0xA]) {
            EntityRef_Set_5D7F10(&vc::At<uint8_t*>(vehicle, 0x6BC), ped);
        }
        if (self[0xB]) {
            uint8_t* occupant = Seats_GetOccupant_4CE560(vehicle + 0x1C4, 0);
            if (occupant && occupant != ped) {
                Ped_SetGrapplePartner(ped, occupant);
                Ped_SetGrapplePartner(occupant, ped);
            }
        }
        return 1;
    }
    if (self[0xB]) {
        vehicle = nullptr;
    } else {
        vehicle = *ref;
        if (vehicle && vc::At<int32_t>(vehicle, 0x46C) != 1) {
            goto distance;
        }
    }
    if (vehicle) {
        if (Seats_Contains_4CEA70(vehicle + 0x1C4, ped) || vc::At<uint8_t*>(vehicle, 0x6BC) == ped) {
            if (self[0xA]) {
                EntityRef_Set_5D7F10(&vc::At<uint8_t*>(vehicle, 0x6BC), ped);
            }
            return 1;
        }
        if (!vc::At<uint8_t*>(vehicle, 0x6BC) || self[8]) {
            goto distance;
        }
    }
    if (!Ped_EveryNthFrame(ped, 8) && self[0xB] == 0) {
        vehicle = nullptr;
    } else {
        vehicle = FindVehicle_4D33F0c(ped, vc::At<float>(self, 4), 1, flagDword, self[9]);
        if (vehicle && vc::At<int32_t>(vehicle, 0x46C) == 1) {
            int32_t kind;
            if (ped[0x1570] == 1 && vehicle[0x290] == 2) {
                vehicle = nullptr;
            } else if (!Ped_CanDrive_41AA50(ped) && *Vehicle_GetVehicleClass(vehicle, &kind) == 3) {
                vehicle = nullptr;
            }
        }
    }
    if (!vehicle) {
        goto fail;
    }
    if (vc::At<uint8_t*>(vehicle, 0x6BC) && !self[8]) {
        goto fail;
    }
    SetRef(ref, vehicle);
distance:
    if (self[0xC] && vehicle[0x290] == 2) {
        return 0;
    }
    {
        float d[3];
        float* delta = VectorSub(d, EntityPos(vehicle), EntityPos(ped));
        if (vc::At<int32_t>(vehicle, 0x46C) != 1) {
            return 0;
        }
        float length = Distance2D(delta);
        if (!(vc::At<float>(self, 4) > length)) {
            return 0;
        }
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
fail:
    ReleaseRef(ref);
    return 0;
}

// 0x5FE970 (thiscall): ConditionAttemptAnchorToVehicle: the ped's grab of its vehicle
uint32_t __fastcall AttemptAnchorToVehicle_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Ped_TryGrabVehicle_479B50(Ped(owner));
}

// 0x5FE980 (thiscall): ConditionTargetVehicleIs: the ped's vehicle is of the class +4
uint8_t __fastcall TargetVehicleIs_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* vehicle = vc::At<uint8_t*>(Ped(owner), 0x430);
    if (!vehicle || (vehicle[0x108] & 7) != 2) {
        return 0;
    }
    int32_t kind;
    return *Vehicle_GetVehicleClass(vehicle, &kind) == vc::At<int32_t>(self, 4);
}

// 0x5FE9C0 (thiscall): ConditionTargetVehicleIsLastVehicleUsed: the last vehicle used
// made the target (true if there is one)
uint8_t __fastcall TargetVehicleIsLastVehicleUsed_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!vc::At<uint8_t*>(ped, 0x1560)) {
        return 0;
    }
    re::Ptr32<uint8_t>* ref = &vc::At<uint8_t*>(ped, 0x430);
    if (*ref != vc::At<uint8_t*>(ped, 0x1560)) {
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
        }
        uint8_t* last = vc::At<uint8_t*>(ped, 0x1560);
        *ref = last;
        if (last) {
            Entity_RegisterReference(last, ref);
        }
    }
    return 1;
}

// 0x5FEA20 (thiscall): ConditionTargetVehicleOwnerDifferentFaction: the vehicle's
// driver (someone else) and its owner (+0x358) not of the ped's faction, the driver not
// hated (attitude 4) either; true with no vehicle
uint8_t __fastcall TargetVehicleOwnerDifferentFaction_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!vc::At<uint8_t*>(ped, 0x430)) {
        return 1;
    }
    if (Seats_GetOccupant_4CE560(vc::At<uint8_t*>(ped, 0x430) + 0x1C4, 0) &&
        Seats_GetOccupant_4CE560(vc::At<uint8_t*>(ped, 0x430) + 0x1C4, 0) != ped) {
        uint8_t* driver = Seats_GetOccupant_4CE560(vc::At<uint8_t*>(ped, 0x430) + 0x1C4, 0);
        if (vc::At<int32_t>(driver, 0x1310) == vc::At<int32_t>(ped, 0x1310)) {
            return 0;
        }
        driver = Seats_GetOccupant_4CE560(vc::At<uint8_t*>(ped, 0x430) + 0x1C4, 0);
        int32_t attitude;
        if (*Ped_GetAttitude_475380(ped, &attitude, vc::At<int32_t>(driver, 0x1310)) == 4) {
            return 0;
        }
    }
    uint8_t* vehicleOwner = vc::At<uint8_t*>(vc::At<uint8_t*>(ped, 0x430), 0x358);
    if (vehicleOwner && vehicleOwner != ped && vc::At<int32_t>(vehicleOwner, 0x1310) == vc::At<int32_t>(ped, 0x1310)) {
        return 0;
    }
    return 1;
}

// 0x5FEAE0 (thiscall): ConditionTargetVehicleIsAlreadyOccupied: someone else drives it
uint8_t __fastcall TargetVehicleIsAlreadyOccupied_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!vc::At<uint8_t*>(ped, 0x430)) {
        return 0;
    }
    if (!Seats_GetOccupant_4CE560(vc::At<uint8_t*>(ped, 0x430) + 0x1C4, 0)) {
        return 0;
    }
    return Seats_GetOccupant_4CE560(vc::At<uint8_t*>(ped, 0x430) + 0x1C4, 0) != ped;
}

// 0x5FEB30 (thiscall): ConditionTargetVehicleIsPlaying: the bike's current animation
// is the one named at +4 (in the bike's anim set)
uint8_t __fastcall TargetVehicleIsPlaying_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* vehicle = vc::At<uint8_t*>(ped, 0x430);
    if (!vehicle || vc::At<int32_t>(vehicle, 0x46C) != 1) {
        return 0;
    }
    uint8_t* blend = vehicle + 0x47C;
    if (!blend) {
        return 0;
    }
    uint8_t* current = AnimBlend_GetCurrent_6B1F50(blend, 0);
    if (!current) {
        return 0;
    }
    uint8_t* animSet = vc::At<uint8_t*>(vc::At<uint8_t*>(blend, 0x94), 0x10);
    if (!animSet) {
        return 0;
    }
    uint8_t* anim = vc::At<uint8_t*>(current, 0x18);
    int32_t id = anim ? int32_t(vc::At<uint16_t>(anim, 0x32)) : -1;
    return id == AnimIdOf_607770(reinterpret_cast<uint32_t*>(self + 4), animSet);
}

// 0x5FEBB0 (thiscall): ConditionTargetMyTargetsVehicle: the fight target's vehicle
// (a ped's +0x1554) made the ped's target vehicle
uint8_t __fastcall TargetMyTargetsVehicle_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* target = Ped_GetFightTarget_497100(ped);
    if (!target || (target[0x108] & 7) != 3 || !vc::At<uint8_t*>(target, 0x1554)) {
        return 0;
    }
    re::Ptr32<uint8_t>* ref = &vc::At<uint8_t*>(ped, 0x430);
    if (*ref != vc::At<uint8_t*>(target, 0x1554)) {
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
        }
        uint8_t* vehicle = vc::At<uint8_t*>(target, 0x1554);
        *ref = vehicle;
        if (vehicle) {
            Entity_RegisterReference(vehicle, ref);
        }
    }
    return 1;
}

// the entity a RelativePosition side names: 0 the target (or the looked-at one),
// 1 the ped's vehicle, 2 the ped, 3 the ped's +0x438
uint8_t* RelativeSubject(uint8_t* ped, int32_t kind) {
    switch (kind) {
    case 0: {
        uint8_t* entity = Targeting_GetTarget_4D5AD0(vc::At<uint8_t*>(ped, 0x1C8));
        if (!entity) {
            entity = Targeting_GetLookTarget_4D7780(vc::At<uint8_t*>(ped, 0x1C8));
        }
        return entity;
    }
    case 1:
        return vc::At<uint8_t*>(ped, 0x430);
    case 2:
        return ped;
    case 3:
        return vc::At<uint8_t*>(ped, 0x438);
    default:
        return nullptr;
    }
}

// 0x5FEC30 (thiscall): ConditionRelativePosition: the side +4 relative to the side +0xC:
// op +8 0 / 1 on the right / left of it (its forward crossed with up), 2 / 3 above /
// below it (its up)
uint8_t __fastcall RelativePosition_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* a = RelativeSubject(ped, vc::At<int32_t>(self, 4));
    uint8_t* b = RelativeSubject(ped, vc::At<int32_t>(self, 0xC));
    if (!a || !b) {
        return 0;
    }
    int32_t op = vc::At<int32_t>(self, 8);
    float dot;
    if (op != 0 && op != 1) {
        float d[3];
        VectorSub(d, EntityPos(a), EntityPos(b));
        float upBuf[3];
        float* up = Physical_GetUp_473730(b, upBuf);
        dot = up[1] * d[1] + up[0] * d[0] + up[2] * d[2];
        if (op == 2) {
            return dot > 0.0f ? 1 : 0;
        }
        if (op == 3) {
            return dot <= 0.0f ? 1 : 0;
        }
        return 0;
    }
    float fwdA[3], fwdB[3];
    float x = Entity_GetForward_4154A0(b, fwdA)[0];
    float y = Entity_GetForward_4154A0(b, fwdB)[1];
    float flat[3] = {x, y, 0.0f};
    float upZ[3] = {0.0f, 0.0f, 1.0f};
    float side[3];
    CrossProduct_414730(side, flat, upZ);
    float d[3];
    VectorSub(d, EntityPos(a), EntityPos(b));
    dot = side[1] * d[1] + d[0] * side[0] + float(double(side[2]) * zero_9009A0);
    if (op == 0) {
        return dot > 0.0f ? 1 : 0;
    }
    return dot <= 0.0f ? 1 : 0;
}

// the count of wheels against the op +4: 0 none, 1..3 at least that many, 4 all
uint8_t WheelsMatch(int32_t op, int32_t count) {
    switch (op) {
    case 0:
        return count == 0;
    case 1:
        return count >= 1;
    case 2:
        return count >= 2;
    case 3:
        return count >= 3;
    case 4:
        return count >= 4;
    default:
        return 0;
    }
}

// 0x5FEEB0 (thiscall): ConditionVehicleWheelsOnGround: the wheels of the ped's vehicle
// (the one it is on, else its target) touching (a bike's two compressions under 0.8,
// or under 1 with its flag 0x10; a car's four under 1)
uint8_t __fastcall VehicleWheelsOnGround_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = Ped(owner);
    uint8_t* vehicle = vc::At<uint8_t*>(ped, 0x1554);
    if (!vehicle) {
        vehicle = vc::At<uint8_t*>(ped, 0x430);
        if (!vehicle) {
            return 0;
        }
    }
    int32_t kind = vc::At<int32_t>(vehicle, 0x46C);
    int32_t count = 0;
    if (kind == 1) {
        double limit = groundLimit_900AD0;
        bool loose = (vehicle[0x1BC] & 0x10) != 0;
        float front = vc::At<float>(vehicle, 0x634);
        if (limit > front || (loose && 1.0f > vc::At<float>(vehicle, 0x634))) {
            count = 1;
        }
        float rear = vc::At<float>(vehicle, 0x638);
        if (rear < limit || (loose && 1.0f > vc::At<float>(vehicle, 0x638))) {
            count += 1;
        }
    } else if (kind == 0) {
        for (int i = 0; i < 4; i++) {
            if (1.0f > vc::At<float>(vehicle, 0x774 + i * 4)) {
                count += 1;
            }
        }
    } else {
        return 0;
    }
    return WheelsMatch(vc::At<int32_t>(self, 4), count);
}

} // namespace

#define CONDC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CONDC_REVERSED("ConditionTargetVehicleCar::Evaluate", 0x005FE3C0, &TargetVehicleCar_Evaluate);
CONDC_REVERSED("ConditionTargetVehicleBike::Evaluate", 0x005FE630, &TargetVehicleBike_Evaluate);
CONDC_REVERSED("ConditionAttemptAnchorToVehicle::Evaluate", 0x005FE970, &AttemptAnchorToVehicle_Evaluate);
CONDC_REVERSED("ConditionTargetVehicleIs::Evaluate", 0x005FE980, &TargetVehicleIs_Evaluate);
CONDC_REVERSED("ConditionTargetVehicleIsLastVehicleUsed::Evaluate", 0x005FE9C0, &TargetVehicleIsLastVehicleUsed_Evaluate);
CONDC_REVERSED("ConditionTargetVehicleOwnerDifferentFaction::Evaluate", 0x005FEA20,
               &TargetVehicleOwnerDifferentFaction_Evaluate);
CONDC_REVERSED("ConditionTargetVehicleIsAlreadyOccupied::Evaluate", 0x005FEAE0, &TargetVehicleIsAlreadyOccupied_Evaluate);
CONDC_REVERSED("ConditionTargetVehicleIsPlaying::Evaluate", 0x005FEB30, &TargetVehicleIsPlaying_Evaluate);
CONDC_REVERSED("ConditionTargetMyTargetsVehicle::Evaluate", 0x005FEBB0, &TargetMyTargetsVehicle_Evaluate);
CONDC_REVERSED("ConditionRelativePosition::Evaluate", 0x005FEC30, &RelativePosition_Evaluate);
CONDC_REVERSED("ConditionVehicleWheelsOnGround::Evaluate", 0x005FEEB0, &VehicleWheelsOnGround_Evaluate);
