// The action tree's bike tracks of 0x659A00..0x65A961: coasting, speeding up, sprint,
// braking, a set speed, hopping, the first-person passenger's getting on / off, picking a
// bike up, freezing a bike, blending one to a position, the AI's steering, and landing
// the ped on the ground after getting off. (Bike fields: BikeTracks2.cpp.)

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"
#include "reversed.h"

using namespace actc;

namespace {

GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(uint8_t*, __thiscall, Seats_GetOccupant, 0x004CE560, uint8_t* seats, uint32_t seat);
GAME_FN(uint8_t*, __thiscall, Seats_GetInfo, 0x004CE4C0, uint8_t* seats, int seat);
GAME_FN(uint8_t, __cdecl, Pad_ButtonJustUp, 0x00738940, int button, int pad);
GAME_FN(void, __thiscall, Bike_AddUnknown778, 0x004BCCB0, uint8_t* bike, float value);
GAME_FN(void, __thiscall, Bike_Jump, 0x004B9C20, uint8_t* bike, float strength);
GAME_FN(uint8_t, __thiscall, Bike_IsUpright, 0x005FD680, uint8_t* bike);
GAME_FN(float, __thiscall, Bike_GetSteerLock, 0x004BCA60, uint8_t* bike);
GAME_FN(V3*, __thiscall, Vehicle_GetDriverMoveDirection, 0x004CCC00, uint8_t* vehicle, V3* out);
GAME_FN(uint8_t, __thiscall, Steer_Apply, 0x006595E0, uint8_t* self);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, Objectives_Clear, 0x00470350, uint8_t* queue);
GAME_FN(void, __thiscall, Ped_SetAlpha, 0x004732A0, uint8_t* ped, int alpha, int b);
GAME_FN(void, __thiscall, Ped_SetVehicle, 0x00476180, uint8_t* ped, uint8_t* vehicle);
GAME_FN(void, __thiscall, Camera_SetShot, 0x004F2C20, void* camera, int shot, int a, int instant, int b, int c);
GAME_FN(void, __thiscall, Camera_ResetController, 0x004F2B60, void* camera);
GAME_FN(void, __thiscall, HUDWeapons_Select, 0x0055C840, void* hud, int weapon);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(float, __cdecl, FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int flag);
GAME_FN(void, __thiscall, Placeable_SetPosition, 0x0041C4E0, uint8_t* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Physical_SetPositionV, 0x0041EE20, uint8_t* entity, const V3* pos);
GAME_FN(void, __thiscall, Entity_443AA0, 0x00443AA0, uint8_t* entity);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00465760, uint8_t* entity);
GAME_FN(void, __thiscall, Physical_RemoveAndAdd, 0x00469470, uint8_t* physical);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void*, __thiscall, ElementTable_FindById, 0x00465580, void* table, uint32_t id, int* outIndex);
GAME_FN(void*, __thiscall, Nodes_FindByName, 0x004A6BB0, void* nodes, const char* name, uintptr_t arg);
GAME_FN(int, __thiscall, AnimManager_GetNodePosition, 0x006C1190, void* manager, uint8_t* anim, int bone, V3* out,
        void* node);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, uint8_t* entity);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, uint8_t* entity);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, uint8_t* matrix, float angle);
GAME_FN(float, __thiscall, Entity_GetHeading, 0x00415470, uint8_t* entity);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);

GAME_VAR(uint8_t*, bikeButes_20C8900, 0x020C8900);
GAME_VAR(int32_t, cameraMode_C3CCF8, 0x00C3CCF8);
GAME_VAR(int32_t, camera_C3CE0C, 0x00C3CE0C);
GAME_VAR(int32_t, camera_C3CED8, 0x00C3CED8);
GAME_VAR(uint8_t, camera_C3CE8C, 0x00C3CE8C);
GAME_VAR(uint8_t*, cameraTarget_C3CE90, 0x00C3CE90);
GAME_VAR(void*, hudWeapons, 0x00C674F0);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline const char* const getOnRhs = reinterpret_cast<const char*>(0x00932278);   // "GetOnRHS"
inline const char* const rearSeat = reinterpret_cast<const char*>(0x0093C0C8);   // "RearSeat"

inline void Engine(uint8_t* vehicle, int on) {
    vc::Call<void>(vehicle, 0x1C, on);
}
inline void Status(uint8_t* vehicle, int status) {
    vc::Call<void>(vehicle, 0x18, status);
}
inline V3 Zero() {
    return *reinterpret_cast<V3*>(zeroVector);
}
// the ped's bike (+0x1554), when it is one
inline uint8_t* PedBike(uint8_t* owner) {
    uint8_t* bike = At<uint8_t*>(OwnerPed(owner), 0x1554);
    return bike && At<int32_t>(bike, 0x46C) == 1 ? bike : nullptr;
}
// the rider's stick (+0 its direction, +0x1C its push) and the bike's speeds zeroed
inline void StopRider(uint8_t* ped, uint8_t* bike) {
    uint8_t* inputs = At<uint8_t*>(ped, 0x2E0);
    At<V3>(inputs, 0) = Zero();
    At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) = 0.0f;
    At<V3>(bike, 0x124) = Zero();
    At<V3>(bike, 0x130) = Zero();
}
// an entity reference field set (the old one let go)
inline void SetEntityRef(re::Ptr32<uint8_t>* ref, uint8_t* entity) {
    if (entity != *ref) {
        if (*ref) {
            Entity_CleanUpOldReference(*ref, ref);
        }
        *ref = entity;
        RegisterReference(entity, ref);
    }
}
// the first-person camera given back
inline void ResetCameraShot() {
    Camera_SetShot(theCamera, 1, 0, 1, 0, 0);
}

// 0x659A00 (thiscall): CoastTrack::Update: the pedal speed capped at the top speed (times
// +0x1C) and at the forward speed plus +0x18
uint8_t __fastcall Coast_Update(uint8_t* self, re::Edx, float) {
    uint8_t* bike = PedBike(At<uint8_t*>(self, 0x20));
    if (!bike) {
        return 1;
    }
    V3 f;
    V3* forward = Placeable_GetForward(bike, &f);
    float along = At<float>(bike, 0x128) * forward->y + At<float>(bike, 0x124) * forward->x +
                  At<float>(bike, 0x12C) * forward->z;
    float cap = At<float>(At<uint8_t*>(bike, 0x2A4), 0x84) * At<float>(self, 0x1C);
    if (cap < At<float>(bike, 0x69C)) {
        At<float>(bike, 0x69C) = cap;
    }
    float limit = At<float>(self, 0x18) + along;
    if (At<float>(bike, 0x69C) > limit) {
        At<float>(bike, 0x69C) = limit;
    }
    return 1;
}

// 0x659AD0 (thiscall): SpeedUpTrack::Update: pedalling at the top speed (times +0x1C),
// the player's sprint tapped on, the pedal input blended from the sprint
uint8_t __fastcall SpeedUp_Update(uint8_t* self, re::Edx, float) {
    uint8_t* bike = PedBike(At<uint8_t*>(self, 0x20));
    if (!bike) {
        return 1;
    }
    At<float>(bike, 0x69C) = At<float>(At<uint8_t*>(bike, 0x2A4), 0x84) * At<float>(self, 0x1C);
    if (Seats_GetOccupant(bike + 0x1C4, 0) == playerPed && Pad_ButtonJustUp(7, 0)) {
        Bike_AddUnknown778(bike, At<float>(bikeButes_20C8900, 0x2C));
    }
    uint8_t* butes = bikeButes_20C8900;
    float input = (1.0f - At<float>(butes, 0x28)) * At<float>(bike, 0x778) + At<float>(self, 0x1C) * At<float>(butes, 0x28);
    At<float>(bike, 0x294) = input * At<float>(bike, 0x29C);
    return 1;
}

// 0x659B80 (thiscall): SprintTrack::Start: a sprint of +0x18 added
uint8_t __fastcall Sprint_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* bike = PedBike(owner);
    if (bike) {
        Bike_AddUnknown778(bike, At<float>(self, 0x18));
    }
    return 1;
}

// 0x659BC0 (thiscall): BrakeTrack::Update: a bike's pedal speed cut by +0x18 (not below
// 0), a full brake stopping an AI rider's stick; an automobile braked for 50 ms
uint8_t __fastcall Brake_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (!vehicle) {
        return 1;
    }
    if (At<int32_t>(vehicle, 0x46C) != 1) {
        At<uint32_t>(vehicle, 0x800) = timeInMs;
        vehicle[0x2DB] = 0xC;
        At<uint32_t>(vehicle, 0x2DC) = timeInMs + 0x32;
        At<float>(vehicle, 0x298) = At<float>(self, 0x18);
        return 1;
    }
    float speed = (1.0f - At<float>(self, 0x18)) * At<float>(vehicle, 0x69C);
    if (speed < 0.0f) {
        speed = 0.0f;
    }
    At<float>(vehicle, 0x69C) = speed;
    if (!(1.0f > At<float>(self, 0x18) || At<float>(self, 0x18) != At<float>(self, 0x18)) &&
        At<int32_t>(ped, 0x1310) != 0xD) {
        uint8_t* inputs = At<uint8_t*>(ped, 0x2E0);
        At<V3>(inputs, 0) = Zero();
        At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) = 0.0f;
    }
    At<float>(vehicle, 0x298) = At<float>(self, 0x18);
    return 1;
}

// 0x659CC0 (thiscall): SetDesiredSpeedTrack::Update: the pedal speed set (+0x18), pedalling
// its way, the brake off
uint8_t __fastcall SetDesiredSpeed_Update(uint8_t* self, re::Edx, float) {
    uint8_t* bike = PedBike(At<uint8_t*>(self, 0x1C));
    if (!bike) {
        return 1;
    }
    At<float>(bike, 0x69C) = At<float>(self, 0x18);
    int32_t sign = 0.0f < At<float>(self, 0x18) ? 1 : -1;
    At<float>(bike, 0x294) = At<float>(bike, 0x29C) * float(sign);
    At<float>(bike, 0x298) = 0.0f;
    return 1;
}

// 0x659D20 (thiscall): HopTrack::Start: the ped's bike jumps (+0x18)
uint8_t __fastcall Hop_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* bike = At<uint8_t*>(OwnerPed(owner), 0x1554);
    if (bike) {
        Bike_Jump(bike, At<float>(self, 0x18));
    }
    return 1;
}

// 0x659D50 (thiscall): FPPassengerGetOnTrack::Start: the player's vehicle kept (+0x1E94);
// without one the ped's objectives cleared, shown and the camera given back
uint8_t __fastcall FPPassengerGetOn_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x430);
    if (vehicle) {
        if (At<int32_t>(ped, 0x1310) == 0xD) {
            re::Ptr32<uint8_t>* ref = &At<uint8_t*>(ped, 0x1E94);
            *ref = vehicle;
            RegisterReference(vehicle, ref);
            At<int32_t>(ped, 0x20) = 0;
        }
        return 1;
    }
    Objectives_Clear(ped + 0x5F0);
    At<uint8_t*>(ped, 0x1E94) = nullptr;
    At<int32_t>(ped, 0x20) = 1;
    Ped_SetAlpha(ped, 0xFF, 0);
    ResetCameraShot();
    At<int32_t>(ped, 0x1318) = 0;
    return 0;
}

// 0x659DF0 (thiscall): FPPassengerGetOnTrack::Update: the player still has the vehicle
uint8_t __fastcall FPPassengerGetOn_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (At<int32_t>(ped, 0x1310) == 0xD && At<uint8_t*>(ped, 0x1E94)) {
        return 1;
    }
    return 0;
}

// 0x659E20 (thiscall): PickUpBikeTrack::Start: the ped's free bike stopped and flagged
// taken
uint8_t __fastcall PickUpBike_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* bike = At<uint8_t*>(OwnerPed(owner), 0x430);
    if (bike && At<int32_t>(bike, 0x46C) == 1 && !(bike[0x6B8] & 8)) {
        bike[0x6B8] = uint8_t(bike[0x6B8] | 1);
        Status(bike, 7);
        Engine(bike, 0);
    }
    return 1;
}

// 0x659E70 (thiscall): PickUpBikeTrack::Update: until the bike is upright
uint8_t __fastcall PickUpBike_Update(uint8_t* self, re::Edx, float) {
    uint8_t* bike = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x1C)), 0x430);
    if (!bike) {
        return 0;
    }
    return Bike_IsUpright(bike) == 0 ? 1 : 0;
}

// 0x659EA0 (thiscall): PickUpBikeTrack::End: the bike's taken flag cleared
uint8_t __fastcall PickUpBike_End(uint8_t* self, re::Edx) {
    uint8_t* bike = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x1C)), 0x430);
    if (bike) {
        bike[0x6B8] = uint8_t(bike[0x6B8] & 0xFE);
    }
    return 1;
}

// 0x659EC0 (thiscall): FreezeBikeTrack::Update: the frozen bike (+0x1C) held still
uint8_t __fastcall FreezeBike_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* bike = At<uint8_t*>(self, 0x1C);
    if (bike && At<int32_t>(bike, 0x46C) == 1) {
        At<float>(bike, 0x69C) = 0.0f;
        StopRider(ped, bike);
    }
    return 1;
}

// the engine of a bike reference (+At) off
template <int At_>
uint8_t __fastcall BikeEngineOff_End(uint8_t* self, re::Edx) {
    uint8_t* bike = At<uint8_t*>(self, At_);
    if (bike && At<int32_t>(bike, 0x46C) == 1) {
        Engine(bike, 0);
    }
    return 1;
}

// 0x659FA0 (thiscall): SteerTrack::Update: the player's bike steered by the stick
// (0x6595E0); else the steer from the rider's direction (its asin, within the lock), a
// bike's eased from its lean, an AI braking into sharp turns and when standing; the
// owner's lean (+0x38 of the owner) the lean over the lock
uint8_t __fastcall Steer_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x24));
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (!vehicle) {
        return 1;
    }
    uint8_t* bike = nullptr;
    if (At<int32_t>(vehicle, 0x46C) == 1) {
        bike = vehicle;
        if (At<int32_t>(ped, 0x1310) == 0xD) {
            return Steer_Apply(self);
        }
    }
    V3 direction;
    Vehicle_GetDriverMoveDirection(vehicle, &direction);
    float steer = -trk::CrtAsin(direction.x);
    float lockDegrees = bike ? Bike_GetSteerLock(bike) : At<float>(At<uint8_t*>(vehicle, 0x2A4), 0x9C);
    float lock = lockDegrees * 3.14159274f / 180.0f;
    float low = -lock;
    if (!(steer >= low || steer != steer)) {
        steer = low;
    }
    if (steer > lock) {
        steer = lock;
    }
    if (bike) {
        float lean = At<float>(bike, 0x2A0);
        float rate = trk::CrtPow(0.7f, timeStep);
        steer = lean + (steer - lean) * rate;
    }
    vc::Call<void>(vehicle, 0xE0, steer);
    if (At<int32_t>(ped, 0x1310) != 0xD) {
        float push = At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C);
        if (std::fabs(steer) > 0.1f && 0.0f == At<float>(vehicle, 0x294)) {
            vehicle[0x33A] = uint8_t(vehicle[0x33A] & 0xFB);
            At<float>(vehicle, 0x294) = At<float>(vehicle, 0x29C) * 0.5f;
            At<float>(vehicle, 0x298) = 0.0f;
        }
        if (0.0f == push) {
            At<float>(vehicle, 0x298) = 1.0f;
        }
    }
    float* lean = At<float*>(At<uint8_t*>(self, 0x24), 0x38);
    *lean = std::fabs(At<float>(vehicle, 0x2A0) / lock);
    return 1;
}

// 0x65A160 (thiscall): FPPassengerGetOffTrack::Start: the player's passenger state
// cleared: objectives, shown, the camera and its target given back, no weapon
uint8_t __fastcall FPPassengerGetOff_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        At<uint8_t*>(ped, 0x1E94) = nullptr;
        Objectives_Clear(ped + 0x5F0);
        At<int32_t>(ped, 0x20) = 1;
        Ped_SetAlpha(ped, 0xFF, 0);
        ResetCameraShot();
        camera_C3CE8C = 0;
        if (cameraTarget_C3CE90) {
            Entity_CleanUpOldReference(cameraTarget_C3CE90, &cameraTarget_C3CE90);
            cameraTarget_C3CE90 = nullptr;
        }
        At<int32_t>(ped, 0x1318) = 0;
        At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18) = -1;
        HUDWeapons_Select(hudWeapons, -1);
    }
    return 1;
}

// 0x65A210 (thiscall): FreezeBikeTrack::Start: the ped's bike kept (+0x1C), its engine on
uint8_t __fastcall FreezeBike_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* bike = PedBike(owner);
    if (bike) {
        re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x1C);
        SetEntityRef(ref, bike);
        Engine(*ref, 1);
    }
    return 1;
}

// the ground under (x, y) looked for from z (again from z2); false when there is none
bool FindGround(float x, float y, float z, float z2, float& ground) {
    uint8_t found = 0;
    ground = FindGroundZ(x, y, z, &found, 1);
    if (!found) {
        found = 0;
        ground = FindGroundZ(x, y, z2, &found, 1);
    }
    return found != 0;
}

// 0x65A270 (thiscall): GetOffVehicleTrack::End: off the vehicle (+0x30): an automobile's
// seat left (its door by the seat +0x34), a bike's rider reference cleared (+0x1B: its
// engine on); +0x1C / +0x1D its flags; +0x24 the ped put on the ground under it
uint8_t __fastcall GetOffVehicle_End(uint8_t* self, re::Edx) {
    uint8_t* vehicle = At<uint8_t*>(self, 0x30);
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x2C));
    if (vehicle) {
        int32_t kind = At<int32_t>(vehicle, 0x46C);
        if (kind == 0) {
            Ped_SetVehicle(ped, nullptr);
            int32_t seat = At<int32_t>(self, 0x34);
            if (seat != -1) {
                uint8_t* info = Seats_GetInfo(vehicle + 0x1C4, seat);
                if (info) {
                    vc::Call<void>(vehicle, 0xB8, At<int32_t>(info, 4), At<int32_t>(info, 8), 0.0f);
                }
            }
        } else {
            if (kind == 1) {
                SetRef(&At<uint8_t*>(vehicle, 0x6BC), nullptr);
            }
            if (self[0x1B] != 0) {
                Engine(vehicle, 1);
            }
        }
        if (self[0x1C] != 0) {
            At<int32_t>(vehicle, 0x20) = 0;
        }
        if (self[0x1D] != 0) {
            At<int32_t>(vehicle, 0xC0) = 1;
        }
        if (At<int32_t>(self, 0x20) < 0) {
            vehicle[0x454] = 0;
        }
    }
    if (self[0x24] != 0) {
        V3 pos = *reinterpret_cast<V3*>(Pos(ped));
        float ground;
        if (FindGround(pos.x, pos.y, pos.z + 2.5f, pos.z + 5.0f, ground)) {
            Placeable_SetPosition(ped, pos.x, pos.y, ground + 0.05f);
            Entity_443AA0(ped);
            Entity_UpdateRwFrame(ped);
            Physical_RemoveAndAdd(ped);
        }
        At<float>(ped, 0x12C) = 0.0f;
    }
    ped[0x1558] = 0;
    return 1;
}

// 0x65A430 (thiscall): FPPassengerGetOnTrack::End: the player shown, the camera given
// back (unless in modes 3 / 4), and put on the ground under the vehicle's rear seat (or
// its right-hand get-on point)
uint8_t __fastcall FPPassengerGetOn_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (At<int32_t>(ped, 0x1310) != 0xD) {
        return 1;
    }
    Ped_SetAlpha(ped, 0xFF, 0);
    if (cameraMode_C3CCF8 != 3 && cameraMode_C3CCF8 != 4) {
        camera_C3CE0C = 1;
        camera_C3CED8 = 0;
        ResetCameraShot();
        Camera_ResetController(theCamera);
    }
    uint8_t* vehicle = At<uint8_t*>(ped, 0x430);
    if (!vehicle || !At<uint8_t*>(vehicle, 0x114)) {
        return 1;
    }
    void* nodes = At<void*>(At<uint8_t*>(vehicle, 0x114), 0x94);
    if (!nodes || !ElementTable_FindById(nodes, HashName(getOnRhs), nullptr)) {
        return 1;
    }
    void* node = Nodes_FindByName(At<void*>(At<uint8_t*>(vehicle, 0x114), 0x94), rearSeat, 0);
    if (!node) {
        node = Nodes_FindByName(At<void*>(At<uint8_t*>(vehicle, 0x114), 0x94), getOnRhs, 0);
        if (!node) {
            return 1;
        }
    }
    V3 pos{0.0f, 0.0f, 0.0f};
    AnimManager_GetNodePosition(animManager, At<uint8_t*>(vehicle, 0x114), -1, &pos, node);
    float x = pos.x;
    float y = pos.y;
    float z = pos.z + 5.0f;
    uint8_t found = 0;
    float ground = FindGroundZ(x, y, z, &found, 1);
    if (!found) {
        // (nudged, the flag not reset)
        x = x + 0.01f;
        y = 0.01f + y;
        ground = FindGroundZ(x, y, z, &found, 1);
    }
    if (found) {
        pos.z = ground + 0.13f;
        Physical_SetPositionV(ped, &pos);
        Entity_443AA0(ped);
        Entity_UpdateRwFrame(ped);
    }
    At<float>(ped, 0x12C) = 0.0f;
    return 1;
}

// 0x65A620 (thiscall): BlendBikeToPosTrack::Start: the ped's bike kept (+0x40), its start
// position (+0x34); +0x18 no steps: moved at once to +0x1C and turned by +0x28, stopped
uint8_t __fastcall BlendBikeToPos_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* bike = PedBike(owner);
    if (!bike) {
        return 1;
    }
    At<V3>(self, 0x34) = *reinterpret_cast<V3*>(Pos(bike));
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x40);
    SetEntityRef(ref, bike);
    if (At<int32_t>(self, 0x18) != 0) {
        return 1;
    }
    At<float>(*ref, 0x69C) = 0.0f;
    uint8_t* inputs = At<uint8_t*>(ped, 0x2E0);
    At<V3>(inputs, 0) = Zero();
    At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) = 0.0f;
    At<V3>(*ref, 0x124) = Zero();
    At<V3>(*ref, 0x130) = Zero();
    World_Remove(*ref);
    Placeable_SetPosition(*ref, At<float>(self, 0x1C), At<float>(self, 0x20), At<float>(self, 0x24));
    float heading;
    uint8_t* matrix = At<uint8_t*>(bike, 0x14);
    if (matrix) {
        heading = trk::CrtAtan2(-At<float>(matrix, 0x10), At<float>(matrix, 0x14));
    } else {
        heading = At<float>(bike, 0x10);
    }
    uint8_t* target = *ref;
    heading = At<float>(self, 0x28) + heading;
    if (At<uint8_t*>(target, 0x14)) {
        Matrix_SetRotateZOnly(At<uint8_t*>(target, 0x14), heading);
    }
    At<float>(target, 0x10) = heading;
    World_Add(*ref);
    return 1;
}

// 0x65A7D0 (thiscall): BlendBikeToPosTrack::Update: one step (+0x30 of +0x18) of the way
// from the start to +0x1C and of the turn +0x28, held still; past the last its engine off
uint8_t __fastcall BlendBikeToPos_Update(uint8_t* self, re::Edx, float) {
    int32_t steps = At<int32_t>(self, 0x18);
    if (steps <= 0) {
        return 1;
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x2C));
    uint8_t* bike = At<uint8_t*>(self, 0x40);
    if (!bike || At<int32_t>(bike, 0x46C) != 1) {
        return 1;
    }
    At<int32_t>(self, 0x30) += 1;
    if (At<int32_t>(self, 0x30) > steps) {
        Engine(bike, 0);
        return 1;
    }
    Engine(bike, 1);
    At<float>(bike, 0x69C) = 0.0f;
    StopRider(ped, bike);
    World_Remove(bike);
    V3 target{At<float>(self, 0x1C), At<float>(self, 0x20), At<float>(self, 0x24)};
    float t = float(At<int32_t>(self, 0x30)) / float(At<int32_t>(self, 0x18));
    V3 d;
    VectorSub(&d, &target.x, &At<float>(self, 0x34));
    V3 step{d.x * t, d.y * t, d.z * t};
    V3 sum;
    V3* pos = VectorAdd(&sum, &At<float>(self, 0x34), &step.x);
    Physical_SetPositionV(bike, pos);
    float heading = Entity_GetHeading(bike) + At<float>(self, 0x28) / float(At<int32_t>(self, 0x18));
    if (At<uint8_t*>(bike, 0x14)) {
        Matrix_SetRotateZOnly(At<uint8_t*>(bike, 0x14), heading);
    }
    At<float>(bike, 0x10) = heading;
    World_Add(bike);
    return 1;
}

} // namespace

#define BIKETRK3_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

BIKETRK3_REVERSED("CoastTrack::Update", 0x00659A00, &Coast_Update);
BIKETRK3_REVERSED("SpeedUpTrack::Update", 0x00659AD0, &SpeedUp_Update);
BIKETRK3_REVERSED("SprintTrack::Start", 0x00659B80, &Sprint_Start);
BIKETRK3_REVERSED("BrakeTrack::Update", 0x00659BC0, &Brake_Update);
BIKETRK3_REVERSED("SetDesiredSpeedTrack::Update", 0x00659CC0, &SetDesiredSpeed_Update);
BIKETRK3_REVERSED("HopTrack::Start", 0x00659D20, &Hop_Start);
BIKETRK3_REVERSED("FPPassengerGetOnTrack::Start", 0x00659D50, &FPPassengerGetOn_Start);
BIKETRK3_REVERSED("FPPassengerGetOnTrack::Update", 0x00659DF0, &FPPassengerGetOn_Update);
BIKETRK3_REVERSED("PickUpBikeTrack::Start", 0x00659E20, &PickUpBike_Start);
BIKETRK3_REVERSED("PickUpBikeTrack::Update", 0x00659E70, &PickUpBike_Update);
BIKETRK3_REVERSED("PickUpBikeTrack::End", 0x00659EA0, &PickUpBike_End);
BIKETRK3_REVERSED("FreezeBikeTrack::Update", 0x00659EC0, &FreezeBike_Update);
BIKETRK3_REVERSED("FreezeBikeTrack::End", 0x00659F60, &BikeEngineOff_End<0x1C>);
BIKETRK3_REVERSED("BlendBikeToPosTrack::End", 0x00659F80, &BikeEngineOff_End<0x40>);
BIKETRK3_REVERSED("SteerTrack::Update", 0x00659FA0, &Steer_Update);
BIKETRK3_REVERSED("FPPassengerGetOffTrack::Start", 0x0065A160, &FPPassengerGetOff_Start);
BIKETRK3_REVERSED("FreezeBikeTrack::Start", 0x0065A210, &FreezeBike_Start);
BIKETRK3_REVERSED("GetOffVehicleTrack::End", 0x0065A270, &GetOffVehicle_End);
BIKETRK3_REVERSED("FPPassengerGetOnTrack::End", 0x0065A430, &FPPassengerGetOn_End);
BIKETRK3_REVERSED("BlendBikeToPosTrack::Start", 0x0065A620, &BlendBikeToPos_Start);
BIKETRK3_REVERSED("BlendBikeToPosTrack::Update", 0x0065A7D0, &BlendBikeToPos_Update);
