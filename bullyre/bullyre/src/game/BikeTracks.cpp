// The action tree's vehicle tracks of 0x658880..0x659220: the vehicle animation (its
// speed from the bike's), flagging a vehicle for an action, getting on / off; the bike
// tracks' field defaults and destructors (the New functions call the defaults after
// construction).
//
// A vehicle: +0x124 its move speed, +0x130 its turn speed, +0x1C4 its seats, +0x46C its
// kind (0 an automobile, 1 a bike); vf 0x18 its status, vf 0x1C its engine.

#include <cstdint>

#include "game/ActionFactories.h"
#include "game/ActionTracksCommon.h"
#include "reversed.h"

using namespace actc;
namespace af = actfac;

namespace {

GAME_FN(uint8_t, __thiscall, AnimationTrack_Start, 0x006124C0, uint8_t* self, uint8_t* owner);
GAME_FN(uint8_t, __thiscall, AnimationTrack_Update, 0x0060AF60, uint8_t* self, float dt);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, uint8_t* blend, int32_t slot);
GAME_FN(void, __thiscall, AnimManager_SetSpeed, 0x006C0E60, void* manager, uint8_t* blend, uint8_t* anim, int, float speed);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget, 0x004D5AD0, uint8_t* targeting);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(void, __thiscall, Queue_RemoveFirst, 0x004702C0, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, Seats_GetOccupant, 0x004CE560, uint8_t* seats, uint32_t seat);
GAME_FN(uint8_t*, __thiscall, Seats_GetFrontPassenger, 0x004CE5F0, uint8_t* seats);
GAME_FN(uint8_t*, __thiscall, Seats_GetInfo, 0x004CE4C0, uint8_t* seats, int seat);
GAME_FN(void, __thiscall, Seats_RemoveDriver, 0x004CE960, uint8_t* seats);
GAME_FN(uint8_t, __thiscall, Seats_IsInside, 0x004CE6D0, uint8_t* seats, uint8_t* ped);
GAME_FN(int32_t, __thiscall, Seats_GetSeatIndex, 0x004CE720, uint8_t* seats, uint8_t* ped);
GAME_FN(void, __thiscall, Seats_Remove, 0x004CE900, uint8_t* seats, uint8_t* ped);
GAME_FN(void, __thiscall, Bike_SetRider, 0x004BDF80, uint8_t* bike, uint8_t* ped);
GAME_FN(void, __thiscall, Bike_AddUnknown778, 0x004BCCB0, uint8_t* bike, float value);
GAME_FN(void, __thiscall, Ped_EnterVehicle, 0x00476640, uint8_t* ped, uint8_t* vehicle);
GAME_FN(void, __thiscall, Ped_WarpIntoFreeSeat, 0x0049A580, uint8_t* ped, uint8_t* vehicle, int front);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(void, __thiscall, AutoPilot_ModifySpeed, 0x00417C50, uint8_t* autoPilot, float speed);
GAME_FN(void, __thiscall, EventMgr_Send, 0x004A6670, void* self, int event, uint8_t* ped, uint8_t* other, const V3* pos,
        float, float);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);

GAME_VAR(void*, events, 0x00C2C1A0);
GAME_VAR(uint8_t*, bikeButes_20C8900, 0x020C8900);

inline void Engine(uint8_t* vehicle, int on) {
    vc::Call<void>(vehicle, 0x1C, on);
}
inline void Status(uint8_t* vehicle, int status) {
    vc::Call<void>(vehicle, 0x18, status);
}
inline V3 Zero() {
    return *reinterpret_cast<V3*>(zeroVector);
}

// 0x6588A0 (thiscall): VehicleAnimationTrack::Start: the owner kept, the base's start
uint8_t __fastcall VehicleAnimation_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x40) = owner;
    return AnimationTrack_Start(self, owner);
}

// 0x658A90 (thiscall): VehicleAnimationTrack::Update: the base's update, then on a bike
// the anim's speed (+0x30) scaled by the mode (+0x58: 1 the bike's forward speed, 2 its
// pedalling, 3 both, over its butes' top speed); +0x5C the ped's anim too
uint8_t __fastcall VehicleAnimation_Update(uint8_t* self, re::Edx, float dt) {
    if (!At<uint8_t*>(self, 0x4C)) {
        return 0;
    }
    uint8_t running = AnimationTrack_Update(self, dt);
    uint8_t* bike = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x40)), 0x1554);
    if (!bike || At<int32_t>(bike, 0x46C) != 1) {
        return running;
    }
    V3 f;
    V3* forward = Placeable_GetForward(bike, &f);
    float along = At<float>(bike, 0x128) * forward->y + At<float>(bike, 0x124) * forward->x +
                  At<float>(bike, 0x12C) * forward->z;
    float pedal = At<float>(bike, 0x69C);
    float inverse = 1.0f / At<float>(At<uint8_t*>(bike, 0x2A4), 0x84);
    float scale;
    switch (At<int32_t>(self, 0x58)) {
    case 1:
        scale = inverse * along;
        break;
    case 2:
        scale = inverse * pedal;
        break;
    case 3:
        scale = pedal * along * inverse;
        break;
    default:
        scale = 1.0f;
        break;
    }
    float speed = At<float>(self, 0x30) * scale;
    uint8_t* anim = Blend_GetAnim(At<uint8_t*>(self, 0x4C), 0);
    AnimManager_SetSpeed(animManager, At<uint8_t*>(self, 0x4C), anim, 0, speed);
    if (self[0x5C] != 0) {
        uint8_t* blend = At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x40)), 0x114);
        uint8_t* pedAnim = Blend_GetAnim(blend, 0);
        AnimManager_SetSpeed(animManager, blend, pedAnim, 0, speed);
    }
    return running;
}

// 0x658BD0 (thiscall): VehicleFlagForActionTrack::Start: the vehicle (+0x18: 1 the ped's
// bike, 2 its vehicle, 3 its target when a vehicle) flagged (+0x1C: 1 stopped for the
// action (a bike halted), 2 the ped's +0x1558 cleared and +0x20 set)
uint8_t __fastcall VehicleFlagForAction_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* vehicle;
    switch (At<int32_t>(self, 0x18)) {
    case 1:
        vehicle = At<uint8_t*>(ped, 0x1554);
        break;
    case 2:
        vehicle = At<uint8_t*>(ped, 0x430);
        break;
    case 3: {
        uint8_t* target = Targeting_GetTarget(At<uint8_t*>(ped, 0x1C8));
        if (!target || (target[0x108] & 7) != 2) {
            return 0;
        }
        vehicle = target;
        break;
    }
    default:
        return 0;
    }
    if (!vehicle) {
        return 0;
    }
    switch (At<int32_t>(self, 0x1C)) {
    case 1:
        if (At<int32_t>(vehicle, 0x46C) == 0) {
            Engine(vehicle, 0);
            if (At<int32_t>(ped, 0x1310) == 0xD) {
                Status(vehicle, 7);
            }
            vehicle[0x2DA] = 0xB;
            return 1;
        }
        if (At<int32_t>(vehicle, 0x46C) == 1) {
            Status(vehicle, 7);
            Engine(vehicle, 0);
            vehicle[0x2DA] = 0xB;
            At<V3>(vehicle, 0x124) = Zero();
            At<float>(vehicle, 0x298) = 1.0f;
            return 1;
        }
        return 0;
    case 2:
        ped[0x1558] = 0;
        At<int32_t>(ped, 0x20) = 1;
        return 1;
    default:
        return 0;
    }
}

// 0x658CF0 (thiscall): GetOnVehicleTrack::Start: the ped's vehicle stopped (a free bike
// flagged taken)
uint8_t __fastcall GetOnVehicle_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* vehicle = At<uint8_t*>(OwnerPed(owner), 0x430);
    if (!vehicle) {
        return 1;
    }
    int32_t kind = At<int32_t>(vehicle, 0x46C);
    if (kind == 1 && !(vehicle[0x6B8] & 8)) {
        vehicle[0x6B8] = uint8_t(vehicle[0x6B8] | 1);
        Status(vehicle, 7);
        Engine(vehicle, 0);
        return 1;
    }
    if (kind == 0) {
        Engine(vehicle, 0);
        At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x20) = 0;
        if (At<int32_t>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x1310) == 0xD) {
            Status(vehicle, 7);
        }
    }
    return 1;
}

// 0x658D80 (thiscall): GetOnVehicleTrack::End: the ped put on its vehicle: a free bike
// ridden (the player's pedalling on at once with the pad held), else into a free seat
// (+0x18: 0 the driver's, 1 the front passenger's) and the driver told
uint8_t __fastcall GetOnVehicle_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    uint8_t* vehicle = At<uint8_t*>(ped, 0x430);
    if (!vehicle) {
        return 1;
    }
    uint8_t* queue = ped + 0x5F0;
    uint8_t* objective = Queue_GetCurrent(queue);
    if (objective && At<int32_t>(objective, 8) == 5) {
        Queue_RemoveFirst(queue);
    }
    int32_t kind = At<int32_t>(vehicle, 0x46C);
    if (kind == 1) {
        if (vehicle[0x6B8] & 8) {
            return 0;
        }
        if (Seats_GetOccupant(vehicle + 0x1C4, 0)) {
            return 0;
        }
        Engine(vehicle, 0);
        Bike_SetRider(vehicle, ped);
        Ped_EnterVehicle(ped, vehicle);
        At<float>(vehicle, 0x294) = At<float>(vehicle, 0x29C) * 0.0f;
        vehicle[0x6B8] = uint8_t(vehicle[0x6B8] & 0xFE);
        At<float>(vehicle, 0x69C) = 0.0f;
        At<float>(vehicle, 0x6A0) = 1.0f;
        if (At<int32_t>(ped, 0x1310) == 0xD && Pad_GetValue(7, 0) > 0.0f) {
            Bike_AddUnknown778(vehicle, At<float>(bikeButes_20C8900, 0x2C));
        }
        return 1;
    }
    if (kind != 0) {
        return 1;
    }
    int32_t seat = At<int32_t>(self, 0x18);
    if (seat == 0) {
        if (Seats_GetOccupant(vehicle + 0x1C4, 0)) {
            return 0;
        }
    } else if (seat == 1) {
        uint8_t* expected = At<uint8_t*>(vehicle, 0x28C);
        if (Seats_GetFrontPassenger(vehicle + 0x1C4) == expected) {
            return 0;
        }
    }
    Engine(vehicle, 0);
    // (the flag goes on the stack as a dword whose upper bytes are stale)
    Ped_WarpIntoFreeSeat(ped, vehicle, seat == 0 ? 1 : 0);
    At<float>(vehicle, 0x294) = At<float>(vehicle, 0x29C) * 0.0f;
    if (At<int32_t>(ped, 0x1310) != 0xD) {
        AutoPilot_ModifySpeed(vehicle + 0x2A8, 0.0f);
        vehicle[0x2DB] = 1;
        At<uint32_t>(vehicle, 0x2DC) = timeInMs + 200;
    }
    uint8_t* seats = vehicle + 0x1C4;
    uint8_t* driver = At<uint8_t*>(vehicle, 0x358);
    if (Seats_GetOccupant(seats, 0) != driver && At<uint8_t*>(ped, 0x1560) != vehicle) {
        V3 pos = *reinterpret_cast<V3*>(Pos(ped));
        EventMgr_Send(events, 0x14, driver, ped, &pos, -1.0f, -1.0f);
    }
    uint8_t* info = Seats_GetInfo(seats, At<int32_t>(ped, 0x1E10));
    if (info) {
        vc::Call<void>(vehicle, 0xB8, At<int32_t>(info, 4), At<int32_t>(info, 8), 0.0f);
    }
    return 1;
}

// 0x658FC0 (thiscall): GetOffVehicleTrack::Start: the ped's bike (its +0x450 rider
// reference and +0x454 state from +0x20) let go: a bike without its rider (braking
// (+0x19: halved) or left), an automobile's seat freed (+0x34 which); an automobile
// nearly stopped is stopped; the ped's own speeds zeroed
uint8_t __fastcall GetOffVehicle_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    At<int32_t>(ped, 0x20) = 1;
    if (vehicle) {
        int32_t state = At<int32_t>(self, 0x20);
        if (state > 0) {
            SetRef(&At<uint8_t*>(vehicle, 0x450), ped);
            vehicle[0x454] = uint8_t(state);
        } else if (state < 0) {
            SetRef(&At<uint8_t*>(vehicle, 0x450), ped);
            vehicle[0x454] = 3;
        }
    }
    At<int32_t>(self, 0x34) = -1;
    if (vehicle) {
        SetRef(&At<uint8_t*>(self, 0x30), vehicle);
        vehicle[0x2E4] = 0;
        int32_t kind = At<int32_t>(vehicle, 0x46C);
        if (kind == 1) {
            Seats_RemoveDriver(vehicle + 0x1C4);
            if (self[0x19] == 0) {
                vehicle[0x6B8] = uint8_t(vehicle[0x6B8] | 2);
            } else {
                At<float>(vehicle, 0x124) = At<float>(vehicle, 0x124) * 0.5f;
                At<float>(vehicle, 0x128) = At<float>(vehicle, 0x128) * 0.5f;
                At<float>(vehicle, 0x12C) = 0.5f * At<float>(vehicle, 0x12C);
            }
        } else if (kind == 0) {
            uint8_t* seats = vehicle + 0x1C4;
            if (Seats_IsInside(seats, ped)) {
                At<int32_t>(self, 0x34) = Seats_GetSeatIndex(seats, ped);
                Seats_Remove(seats, ped);
            }
        }
        bool stopCheck = true;
        if (At<int32_t>(ped, 0x1310) != 0xD) {
            if (At<int32_t>(vehicle, 0x46C) != 0) {
                stopCheck = false;
            } else {
                AutoPilot_ModifySpeed(vehicle + 0x2A8, 0.0f);
                vehicle[0x2DB] = 1;
                At<uint32_t>(vehicle, 0x2DC) = timeInMs + 500;
            }
        }
        if (stopCheck && At<int32_t>(vehicle, 0x46C) == 0) {
            float x = At<float>(vehicle, 0x124);
            float y = At<float>(vehicle, 0x128);
            float speedSq = y * y + x * x;
            if (speedSq < 0.0016f) {
                At<V3>(vehicle, 0x124) = Zero();
                At<V3>(vehicle, 0x130) = Zero();
                Status(vehicle, 4);
            }
        }
    }
    At<V3>(ped, 0x124) = V3{0.0f, 0.0f, 0.0f};
    At<V3>(ped, 0x130) = V3{0.0f, 0.0f, 0.0f};
    return 1;
}

// 0x6591F0 (thiscall): GetOffVehicleTrack::Update: a let-go bike (+0x20 below 0) kept
// in state 3 at least
uint8_t __fastcall GetOffVehicle_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x2C));
    if (!ped) {
        return 0;
    }
    uint8_t* bike = At<uint8_t*>(ped, 0x1554);
    if (bike && At<int32_t>(self, 0x20) < 0 && int8_t(bike[0x454]) < 3) {
        bike[0x454] = 3;
    }
    return 1;
}

// 0x658940 (thiscall): PedalTrack::Start: the owner kept
uint8_t __fastcall Pedal_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x34) = owner;
    return 1;
}

} // namespace

// (variadic: the template arguments' commas)
#define BIKETRK_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

using af::Construct;
using af::Fields;
using af::Vt;
using af::Call;
using af::F32;
using af::I32;
using af::U8;
using af::TrackBaseDtor;

// field defaults (thiscall, after construction)
BIKETRK_REVERSED("VehicleAnimationTrack::InitFields", 0x00658880, &Fields<Call(0x00607660), I32(0x58, 0), U8(0x5C, 1), U8(0x8, 0)>);
BIKETRK_REVERSED("VehicleFlagForActionTrack::InitFields", 0x006588B0, &Fields<I32(0x18, 0), I32(0x1C, 0)>);
BIKETRK_REVERSED("GetOffVehicleTrack::InitFields", 0x006588E0, &Fields<U8(0x8, 0), U8(0x18, 0), U8(0x19, 0), U8(0x1A, 1), U8(0x1B, 0), U8(0x1C, 0), U8(0x1D, 0), U8(0x24, 1), I32(0x20, 0)>);
BIKETRK_REVERSED("PedalTrack::InitFields", 0x00658900, &Fields<F32(0x18, 1.0f), U8(0x30, 0), F32(0x1C, 1.0f), U8(0x8, 0), F32(0x20, 0.523598790f), F32(0x24, 0.523598790f), F32(0x28, 0.800000012f), F32(0x2C, 1.0f)>);
BIKETRK_REVERSED("SteerTrack::InitFields", 0x00658950, &Fields<U8(0x8, 0), F32(0x18, 0.610865235f), F32(0x1C, 0.200000003f), F32(0x20, 2.0f)>);
BIKETRK_REVERSED("CoastTrack::InitFields", 0x00658980, &Fields<U8(0x8, 0), F32(0x18, 0.0199999996f), F32(0x1C, 1.0f)>);
BIKETRK_REVERSED("SpeedUpTrack::InitFields", 0x006589B0, &Fields<U8(0x8, 0), F32(0x1C, 1.0f), F32(0x18, 0.00999999978f)>);
BIKETRK_REVERSED("SetDesiredSpeedTrack::InitFields", 0x00658A00, &Fields<U8(0x8, 0), F32(0x18, 0.200000003f)>);
BIKETRK_REVERSED("HopTrack::InitFields", 0x00658A20, &Fields<U8(0x8, 0), F32(0x18, 0.100000001f)>);
BIKETRK_REVERSED("BlendBikeToPosTrack::InitFields", 0x00658A70, &Fields<F32(0x1C, 0.0f), U8(0x8, 0), F32(0x20, 0.0f), I32(0x18, 0), F32(0x24, 0.0f), F32(0x28, 0.0f)>);

// destructors (thiscall)
BIKETRK_REVERSED("VehicleFlagForActionTrack::Destruct", 0x006588C0, &Fields<Vt(0x00934BE0), TrackBaseDtor>);
BIKETRK_REVERSED("GetOnVehicleTrack::Destruct", 0x006588D0, &Fields<Vt(0x00934BF8), TrackBaseDtor>);
BIKETRK_REVERSED("PedalTrack::Destruct", 0x00658930, &Fields<Vt(0x00934F18), TrackBaseDtor>);
BIKETRK_REVERSED("SteerTrack::Destruct", 0x00658970, &Fields<Vt(0x00934F30), TrackBaseDtor>);
BIKETRK_REVERSED("CoastTrack::Destruct", 0x006589A0, &Fields<Vt(0x00934F48), TrackBaseDtor>);
BIKETRK_REVERSED("SpeedUpTrack::Destruct", 0x006589D0, &Fields<Vt(0x00934F60), TrackBaseDtor>);
BIKETRK_REVERSED("SprintTrack::Destruct", 0x006589E0, &Fields<Vt(0x00934F78), TrackBaseDtor>);
BIKETRK_REVERSED("BrakeTrack::Destruct", 0x006589F0, &Fields<Vt(0x00934F90), TrackBaseDtor>);
BIKETRK_REVERSED("SetDesiredSpeedTrack::Destruct", 0x00658A10, &Fields<Vt(0x00934FA8), TrackBaseDtor>);
BIKETRK_REVERSED("HopTrack::Destruct", 0x00658A30, &Fields<Vt(0x00934FC0), TrackBaseDtor>);
BIKETRK_REVERSED("FPPassengerGetOnTrack::Destruct", 0x00658A40, &Fields<Vt(0x00934C10), TrackBaseDtor>);
BIKETRK_REVERSED("FPPassengerGetOffTrack::Destruct", 0x00658A50, &Fields<Vt(0x00934C28), TrackBaseDtor>);
BIKETRK_REVERSED("PickUpBikeTrack::Destruct", 0x00658A60, &Fields<Vt(0x009352E0), TrackBaseDtor>);

// the tracks' virtuals
BIKETRK_REVERSED("VehicleAnimationTrack::Start", 0x006588A0, &VehicleAnimation_Start);
BIKETRK_REVERSED("PedalTrack::Start", 0x00658940, &Pedal_Start);
BIKETRK_REVERSED("VehicleAnimationTrack::Update", 0x00658A90, &VehicleAnimation_Update);
BIKETRK_REVERSED("VehicleFlagForActionTrack::Start", 0x00658BD0, &VehicleFlagForAction_Start);
BIKETRK_REVERSED("GetOnVehicleTrack::Start", 0x00658CF0, &GetOnVehicle_Start);
BIKETRK_REVERSED("GetOnVehicleTrack::End", 0x00658D80, &GetOnVehicle_End);
BIKETRK_REVERSED("GetOffVehicleTrack::Start", 0x00658FC0, &GetOffVehicle_Start);
BIKETRK_REVERSED("GetOffVehicleTrack::Update", 0x006591F0, &GetOffVehicle_Update);
