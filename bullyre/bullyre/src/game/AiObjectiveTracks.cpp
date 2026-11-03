// The action tree's AI tracks that hand a ped objectives or send it somewhere with its
// steering (investigate an area, wander, move to / away from a point, remove the
// current objective, ...). 0x621700..0x62A5xx (the objective ones; the path, bike,
// combat, locomotion and social tracks are in AiPathTracks.cpp, AiBikeTracks.cpp,
// AiCombatTracks.cpp, AiLocoTracks.cpp and AiSocialTracks.cpp).
// The ped: +0x5F0 its objective queue (an objective: +0 vtable, vf4 (+0x10) its target
// position, +8 its type, +0x10 its speed), +0x654 its steering (PedSteering.cpp),
// +0x1D30 the stimulus it reacts to (+0xC its position).

#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

using namespace actc;

namespace {

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(uint8_t*, __thiscall, WanderObjective_Construct, 0x0046FC80, void* self, uint8_t* ped, float speed);
GAME_FN(uint8_t*, __thiscall, MoveToPosObjective_Construct, 0x004705B0, void* self, uint8_t* ped, const float* pos,
        float speed, float radius, int a, int b);
GAME_FN(uint8_t, __thiscall, Queue_Prepend, 0x00471470, uint8_t* queue, uint8_t* objective);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(void, __thiscall, Queue_RemoveFirst, 0x004702C0, uint8_t* queue);
// CPedSteering (the ped's +0x654): sent to a point (flags, speed, radius, the entity
// followed); GoToWith: the path follower's two extra values instead
GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
GAME_FN(uint8_t, __thiscall, Steering_GoToWith, 0x004A4910, uint8_t* steering, float x, float y, float z, int unused,
        uint32_t flags, float speed, float radius, float a, float b);
GAME_FN(void, __thiscall, Steering_Stop, 0x004A4710, uint8_t* steering);
GAME_FN(uint8_t, __thiscall, Steering_IsDD4One, 0x004A48D0, uint8_t* steering);
GAME_FN(uint32_t, __thiscall, Steering_IsWithin, 0x004A47A0, uint8_t* steering, float x, float y, float z, float radius);
GAME_FN(void, __thiscall, Queue_SetDefault, 0x00470300, uint8_t* queue, uint8_t* objective);
GAME_FN(uint8_t, __thiscall, Objective_Get18, 0x0046FB90, uint8_t* objective);
GAME_FN(uint8_t, __thiscall, Ped_HasWeapon, 0x00474BF0, uint8_t* ped);
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int min, int max);
GAME_FN(int32_t*, __thiscall, Ped_GetAttitude, 0x00475380, uint8_t* ped, int32_t* out, int32_t pedType);
GAME_FN(uint8_t, __thiscall, Social_CanTalkTo, 0x0048D470, uint8_t* social, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Sight_CanSee, 0x0048D1F0, uint8_t* sight, uint8_t* entity);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget, 0x004D5AD0, uint8_t* targeting);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(V3*, __cdecl, Vector_FromAngle, 0x00432DF0, V3* out, float angle);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(V3*, __cdecl, Matrix_Multiply3x3, 0x0041C490, V3* out, const void* m, const void* v);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(uint8_t, __thiscall, Seats_HasSeat, 0x004CE4F0, uint8_t* seats, uint32_t seat);
GAME_FN(uint8_t, __thiscall, Seats_IsSeatTaken, 0x004CE510, uint8_t* seats, uint32_t seat, int orEntering);
GAME_FN(uint8_t*, __thiscall, Seats_GetInfo, 0x004CE4C0, uint8_t* seats, int seat);
GAME_FN(uint8_t, __cdecl, Walkable_IsOnMesh, 0x005EB7D0, const float* point, int a, uintptr_t b);
// a vehicle's path target (PathFollower.cpp): a new point; where to drive next; reached
GAME_FN(void, __thiscall, PathTarget_Set, 0x00417B90, uint8_t* target, const float* point);
GAME_FN(void, __thiscall, PathTarget_Next, 0x004184F0, uint8_t* target, uint8_t* vehicle, V3* out);
GAME_FN(uint32_t, __thiscall, PathTarget_IsDirect, 0x00417BE0, uint8_t* target);
// the ambient schedules (PedSchedules.cpp): a route start's position, object, validity;
// its schedule active
GAME_FN(V3*, __thiscall, RouteStart_GetPosition, 0x0046E3D0, uint8_t* start, V3* out);
GAME_FN(uint8_t*, __thiscall, RouteStart_GetObject, 0x0046E420, uint8_t* start);
GAME_FN(uint8_t, __thiscall, RouteStart_IsValid, 0x0046E390, uint8_t* start);
GAME_FN(uint8_t, __thiscall, Schedules_IsStartActive, 0x0046E2C0, void* manager, uint8_t* start);
GAME_FN(void, __thiscall, Doors_AddPush, 0x004E9CB0, void* doors, uint8_t* ped, uint8_t* door);
GAME_FN(void, __thiscall, Population_Remember, 0x0049DC80, void* population, uint8_t* ped, uint8_t* entity);
GAME_FN(void, __thiscall, DockTrack_Arrive, 0x0061B3E0, uint8_t* self, uint8_t* ped);
GAME_VAR(void*, schedules, 0x00C2AB68);
GAME_VAR(void*, population, 0x00C2C108);
GAME_VAR(int32_t, dockModel, 0x00BF436C);   // (a model id armed peds dock slowly)
GAME_VAR(int32_t, greetKeyA, 0x01B652B0);
GAME_VAR(int32_t, greetKeyB, 0x01B652B4);
inline void* const doorManager = reinterpret_cast<void*>(0x00C3B808);

uint8_t* Queue(uint8_t* ped) {
    return ped + 0x5F0;
}
uint8_t* Steering(uint8_t* ped) {
    return ped + 0x654;
}
// the time in ms as the game's x87 has it (the unsigned value, exact)
double NowExact() {
    return double(timeInMs);
}

// 0x621700 (thiscall): InvestigateAreaTrack::Update: state +0x30: 0 walk to the spot
// (+0x38) once, then 1 keep walking there (flags 0x14, speed +0x20, radius +0x1C); when
// that ends 2: wait until +0x34 (now + the wait +0x24)
uint8_t __fastcall InvestigateAreaTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x2C));
    int32_t state = At<int32_t>(self, 0x30);
    if (state == 0) {
        Steering_GoTo(Steering(ped), At<float>(self, 0x38), At<float>(self, 0x3C), At<float>(self, 0x40), 2, 0.0f,
                      0.3f, nullptr, 0);
        At<int32_t>(self, 0x30) = 1;
    } else if (state != 1) {
        if (state != 2) {
            return 0;
        }
        return NowExact() < double(At<float>(self, 0x34)) ? 1 : 0;
    }
    if (!Steering_GoTo(Steering(ped), At<float>(self, 0x38), At<float>(self, 0x3C), At<float>(self, 0x40), 0x14,
                       At<float>(self, 0x20), At<float>(self, 0x1C), nullptr, 0)) {
        At<int32_t>(self, 0x30) = 2;
        At<float>(self, 0x34) = float(NowExact() + At<float>(self, 0x24));
    }
    return 1;
}

// 0x621810 (thiscall): AddWanderObjectiveTrack::Start: a wander objective (speed 0.3)
// put first in the ped's queue (a failed allocation queues null)
uint8_t __fastcall AddWanderObjectiveTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    void* mem = Malloc(0x14);
    uint8_t* objective = mem ? WanderObjective_Construct(mem, ped, 0.3f) : nullptr;
    Queue_Prepend(Queue(ped), objective);
    return 0;
}

// 0x621870 (thiscall): RemoveObjectiveTrack::Start: the current objective removed when
// it's of the type +0x18
uint8_t __fastcall RemoveObjectiveTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* queue = Queue(OwnerPed(owner));
    uint8_t* current = Queue_GetCurrent(queue);
    if (current && At<int32_t>(current, 8) == At<int32_t>(self, 0x18)) {
        Queue_RemoveFirst(queue);
    }
    return 0;
}

// 0x6218B0 (thiscall): MoveAwayFromStimulusTrack::Update: while the ped's stimulus is
// nearer than +0x18 (flat distance), it walks to it with the rest of the distance as the
// path's value (a quirk of the name: GoToWith's extra values), speed +0x1C
uint8_t __fastcall MoveAwayFromStimulusTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x20);
    uint8_t* stimulus = At<uint8_t*>(OwnerPed(owner), 0x1D30);
    if (!stimulus) {
        return 0;
    }
    V3 at = {At<float>(stimulus, 0xC), At<float>(stimulus, 0x10), At<float>(stimulus, 0x14)};
    float speed = At<float>(self, 0x1C);
    uint8_t* ped = OwnerPed(owner);
    V3 d;
    V3* r = VectorSub(&d, &at.x, Pos(ped));
    double range = At<float>(self, 0x18);
    float flat = r->x * r->x + r->y * r->y;
    float dist = SqrtAbs(flat);
    float left = float(range - dist);
    if (left < 0.0f) {
        return 0;
    }
    return Steering_GoToWith(Steering(ped), at.x, at.y, at.z, 0, 0x14, speed, 0.3f, left, 20.0f);
}

// 0x6219C0 (thiscall): MoveToCurrentPlayerPositionTrack::Start: a move-to objective to
// where the player is now (speed and radius 0.3, then its speed +0x1C) put first in the
// queue (a failed allocation writes through null, as the game does)
uint8_t __fastcall MoveToCurrentPlayerPositionTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    uint8_t* ped = OwnerPed(owner);
    float* pos = Pos(playerPed);
    float speed = At<float>(self, 0x1C);
    void* mem = Malloc(0x34);
    uint8_t* objective = mem ? MoveToPosObjective_Construct(mem, ped, pos, 0.3f, 0.3f, 0, 0) : nullptr;
    At<float>(objective, 0x10) = speed;
    Queue_Prepend(Queue(ped), objective);
    return 0;
}

// 0x621A40 (thiscall): MoveToCurrentPlayerPositionTrack::Update: while the current
// objective is a move-to (type 0), the ped walks to its target (flags 0x14, its speed,
// radius 0.3); removed when that's done
uint8_t __fastcall MoveToCurrentPlayerPositionTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x2C));
    uint8_t* queue = Queue(ped);
    uint8_t* current = Queue_GetCurrent(queue);
    if (!current || At<int32_t>(current, 8) != 0) {
        return 0;
    }
    float speed = At<float>(current, 0x10);
    float* to = vc::Call<float*>(current, 0x10);
    if (Steering_GoTo(Steering(ped), to[0], to[1], to[2], 0x14, speed, 0.3f, nullptr, 0)) {
        return 1;
    }
    Queue_RemoveFirst(queue);
    return 0;
}

// 0x621DE0 (thiscall): FollowPathTrack::Resolve: a current follow-path objective (types
// 0xC, 0xE) removed
void __fastcall FollowPathTrack_Resolve(uint8_t* self, re::Edx) {
    uint8_t* queue = Queue(OwnerPed(At<uint8_t*>(self, 0x18)));
    uint8_t* current = Queue_GetCurrent(queue);
    if (current && (At<int32_t>(current, 8) == 0xC || At<int32_t>(current, 8) == 0xE)) {
        Queue_RemoveFirst(queue);
    }
}

// 0x621E20 (thiscall): FleeOnPathOnBikeTrack::Resolve: a current flee-on-path objective
// (type 0xD) removed
void __fastcall FleeOnPathOnBikeTrack_Resolve(uint8_t* self, re::Edx) {
    uint8_t* queue = Queue(OwnerPed(At<uint8_t*>(self, 0x18)));
    uint8_t* current = Queue_GetCurrent(queue);
    if (current && At<int32_t>(current, 8) == 0xD) {
        Queue_RemoveFirst(queue);
    }
}

// 0x624980 (thiscall): DockTrack::Start: the ped's route start (+0x1D1C) and its
// schedule needed (else 0); +0x14EC cleared; the walking speed +0x24: 1.5 when +0x18,
// else 0.7 for a schedule with flag 0x20 or by its chance (+0xFF percent), 0.3 for an
// armed ped of the dock model and otherwise
uint8_t __fastcall DockTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = OwnerPed(owner);
    uint8_t* start = At<uint8_t*>(ped, 0x1D1C);
    if (!start) {
        return 0;
    }
    uint8_t* schedule = At<uint8_t*>(start, 8);
    if (!schedule) {
        return 0;
    }
    if (At<int32_t>(ped, 0x14EC) != 0) {
        At<int32_t>(ped, 0x14EC) = 0;
    }
    int chance = int(int8_t(At<uint8_t*>(start, 8)[0xFF]));
    Queue_GetCurrent(Queue(ped));
    if (self[0x18] != 0) {
        At<float>(self, 0x24) = 1.5f;
        return 1;
    }
    if (!Ped_HasWeapon(ped) || int32_t(At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E)) != dockModel) {
        if ((schedule[0xFC] & 0x20) != 0 || (chance != 0 && RandomInt(0, 100) < chance)) {
            At<float>(self, 0x24) = 0.7f;
            return 1;
        }
    }
    At<float>(self, 0x24) = 0.3f;
    return 1;
}

// 0x624A60 (thiscall): DockTrack::Update: the ped goes to its route start (+0x20; its
// object's door pushed open, or the route valid) while the schedule is active (unless
// +0x18; flag 0x20: speed 1), flags 0x1C at +0x24, radius 0.75; there (+0x28 set) it
// docks (0x61B3E0) and hides (+0x78 1) unless +0x1DE7. The start is dropped when its
// object's taken, its schedule's over, or the steering gives up.
uint8_t __fastcall DockTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    At<uint8_t*>(self, 0x20) = At<uint8_t*>(ped, 0x1D1C);
    V3 pos;
    RouteStart_GetPosition(At<uint8_t*>(self, 0x20), &pos);
    uint8_t* object = RouteStart_GetObject(At<uint8_t*>(self, 0x20));
    V3 d;
    VectorSub(&d, &pos.x, Pos(ped));
    d.z = 0.0f;
    Vector_Normalise(&d);   // (not used)
    if (object) {
        if (At<int32_t>(object, 0x180) != 0) {
            At<uint8_t*>(ped, 0x1D1C) = nullptr;
            return 0;
        }
        Doors_AddPush(doorManager, ped, object);
        Population_Remember(population, ped, object);
    } else if (!RouteStart_IsValid(At<uint8_t*>(self, 0x20))) {
        return 0;
    }
    if (self[0x18] == 0) {
        if (!Schedules_IsStartActive(schedules, At<uint8_t*>(self, 0x20))) {
            At<uint8_t*>(ped, 0x1D1C) = nullptr;
            return 0;
        }
        if ((At<uint8_t*>(At<uint8_t*>(self, 0x20), 8)[0xFC] & 0x20) != 0) {
            At<float>(self, 0x24) = 1.0f;
        }
    }
    if (self[0x28] != 0) {
        DockTrack_Arrive(self, ped);
        if (ped[0x1DE7] == 0) {
            At<int32_t>(ped, 0x78) = 1;
        }
        return 1;
    }
    uint8_t* steering = Steering(ped);
    uint8_t going = Steering_GoTo(steering, pos.x, pos.y, pos.z, 0x1C, At<float>(self, 0x24), 0.75f, nullptr, 0);
    if (Steering_IsDD4One(steering)) {
        At<uint8_t*>(ped, 0x1D1C) = nullptr;
        return 0;
    }
    if (!going && uint8_t(Steering_IsWithin(steering, pos.x, pos.y, pos.z, 0.75f))) {
        DockTrack_Arrive(self, ped);
        At<int32_t>(ped, 0x1338) = 1;
        self[0x28] = 1;
    }
    return 1;
}

// 0x624C20 (thiscall): GreetTrack::Update: targeted by the player while its input holds
// one of the two greet keys, the ped requests action 0xB or 0xA; else the first of the
// peds around it (+0x3BC's list) it may talk to and isn't hostile to (attitude 2), 2..4
// away and seen, is greeted (0xA below attitude 2, 0xB above). A ped past 4 away ends
// the search; one that doesn't qualify also skips the next entry (as the game does).
uint8_t __fastcall GreetTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* player = playerPed;
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (Targeting_GetTarget(At<uint8_t*>(player, 0x1C8)) == ped) {
        uint8_t* held = At<uint8_t*>(At<uint8_t*>(player, 0x2E0), 0x268);
        if (held) {
            if (*vc::Call<int32_t*>(held, 8) == greetKeyA) {
                ActionInputs_SetRequest(At<uint8_t*>(ped, 0x2E0), 0xB, 1, 0.0f);
                return 1;
            }
            if (*vc::Call<int32_t*>(held, 8) == greetKeyB) {
                ActionInputs_SetRequest(At<uint8_t*>(ped, 0x2E0), 0xA, 1, 0.0f);
                return 1;
            }
        }
    }
    const float* pp = Pos(ped);
    V3 at = {pp[0], pp[1], pp[2]};
    int32_t count = At<int32_t>(ped, 0x3C0);
    for (int32_t i = 0; i < count; i++) {
        uint8_t* other = At<uint8_t*>(ped, 0x3C4 + i * 4);
        if (!other) {
            continue;
        }
        if (Social_CanTalkTo(ped + 0xA64, other)) {
            int32_t attitude;
            Ped_GetAttitude(ped, &attitude, At<int32_t>(other, 0x1310));
            if (attitude != 2) {
                V3 d;
                V3* r = VectorSub(&d, Pos(other), &at.x);
                float dist = SqrtAbs(r->x * r->x + r->y * r->y);
                if (dist > 2.0f) {
                    if (!(dist < 4.0f)) {
                        return 1;
                    }
                    if (Sight_CanSee(ped + 0xA64, other)) {
                        if (attitude < 2) {
                            ActionInputs_SetRequest(At<uint8_t*>(ped, 0x2E0), 0xA, 1, 0.0f);
                        }
                        if (attitude > 2) {
                            ActionInputs_SetRequest(At<uint8_t*>(ped, 0x2E0), 0xB, 1, 0.0f);
                        }
                        return 1;
                    }
                }
            }
        }
        i++;
    }
    return 1;
}

// 0x624E30 (thiscall): TeacherTrack::Update: the ped goes to the player (flags 0x1A,
// speed 1, radius 0.3) while it can
uint8_t __fastcall TeacherTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    const float* to = Pos(playerPed);
    return Steering_GoTo(Steering(ped), to[0], to[1], to[2], 0x1A, 1.0f, 0.3f, nullptr, 0);
}

// 0x624E90 (thiscall): MoveToVehicleTrack::Update: the ped goes to the vehicle of its
// enter-vehicle objective (type 5; else its +0x430): a car's first free seat's door
// (the seat's offset through the car's matrix; an offset of zero: the seat's position
// with x tripled, as the game does) with flags 0x1C, else the vehicle itself with flags
// 4; speed 1, radius 1.2, following the vehicle when its vf4 says so
uint8_t __fastcall MoveToVehicleTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* current = Queue_GetCurrent(Queue(ped));
    if (current && At<int32_t>(current, 8) != 5) {
        current = nullptr;
    }
    uint32_t flags = 0x1C;
    uint8_t* vehicle = current ? At<uint8_t*>(current, 0x10) : At<uint8_t*>(ped, 0x430);
    if (!vehicle) {
        return 0;
    }
    const float* vp = Pos(vehicle);
    V3 to = {vp[0], vp[1], vp[2]};
    bool door = false;
    if (At<int32_t>(vehicle, 0x46C) == 0) {
        uint8_t* seats = vehicle + 0x1C4;
        for (uint32_t seat = 0; seat < 4; seat++) {
            if (Seats_HasSeat(seats, seat) && !Seats_IsSeatTaken(seats, seat, 1)) {
                uint8_t* info = Seats_GetInfo(seats, int(seat));
                V3 offset = {At<float>(info, 0x24), At<float>(info, 0x28), At<float>(info, 0x2C)};
                if (offset.x == zeroVector[0] && offset.y == zeroVector[1] && offset.z == zeroVector[2]) {
                    info = Seats_GetInfo(seats, int(seat));
                    offset = {At<float>(info, 0x18), At<float>(info, 0x1C), At<float>(info, 0x20)};
                    offset.x = offset.x * 3.0f;
                }
                const float* base = Pos(vehicle);
                V3 turned, sum;
                V3* t = Matrix_Multiply3x3(&turned, Entity_GetMatrix(vehicle), &offset);
                V3* r = VectorAdd(&sum, base, &t->x);
                to = *r;
                door = true;
                break;
            }
        }
    }
    if (!door) {
        flags = 4;
    }
    uint8_t follow = vc::Call<uint8_t>(vehicle, 0x10);
    uint8_t going =
        Steering_GoTo(Steering(ped), to.x, to.y, to.z, uint16_t(flags), 1.0f, 1.2f, follow ? vehicle : nullptr, 0);
    return going != 0 ? 1 : 0;
}

// 0x625070 (thiscall): MoveObjectiveTrack's step: for a move objective (types 0, 1,
// 0x18) the path target (+0x1C) aimed at its target and driven along (from the ped's
// vehicle); the ped sent there (flags 0x10, 0x14 when direct; its speed eased down to
// 0.05 inside the arrival radius + 3 (ped +0x51C when +0x520, else 3), radius 0.3); off
// the walkable meshes it stops. Inside the radius the objective's done (removed and
// the steering stopped: 0)
uint8_t __fastcall MoveObjectiveTrack_Step(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* queue = Queue(ped);
    uint8_t* current = Queue_GetCurrent(queue);
    if (!current) {
        return 0;
    }
    int32_t type = At<int32_t>(current, 8);
    if (type < 0 || (type > 1 && type != 0x18)) {
        return 0;
    }
    V3 to = {0.0f, 0.0f, 0.0f};
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    uint8_t* target = self + 0x1C;
    PathTarget_Set(target, vc::Call<float*>(current, 0x10));
    PathTarget_Next(target, vehicle, &to);
    uint32_t flags = 0x10;
    if (uint8_t(PathTarget_IsDirect(target))) {
        flags = 0x14;
    }
    float radius = ped[0x520] != 0 ? At<float>(ped, 0x51C) : 3.0f;
    V3 d;
    float* goal = vc::Call<float*>(current, 0x10);
    V3* r = VectorSub(&d, goal, Pos(ped));
    float distSq = r->x * r->x + r->y * r->y;
    float outer = radius + 3.0f;
    float speed = At<float>(current, 0x10);
    if (outer > distSq) {
        float eased = (distSq - radius) / (outer - radius) * speed;
        speed = eased + 0.05f;
    }
    if (!Walkable_IsOnMesh(&to.x, 0, 0)) {
        float* move = At<float*>(ped, 0x2E0);
        move[7] = 0.0f;
        move = At<float*>(ped, 0x2E0);
        move[0] = zeroVector[0];
        move[1] = zeroVector[1];
        move[2] = zeroVector[2];
        return 1;
    }
    uint8_t* steering = Steering(ped);
    Steering_GoTo(steering, to.x, to.y, to.z, uint16_t(flags), speed, 0.3f, nullptr, 0);
    float radiusSq = radius * radius;
    if (radiusSq > distSq) {
        Queue_RemoveFirst(queue);
        Steering_Stop(steering);
        return 0;
    }
    return 1;
}

// 0x625260 (thiscall): FaceObjectiveTrack::Update: for a face objective (types 2..4) the
// ped stands facing its heading (vf4); done (removed, 0) once facing within 1 - sin 20
// degrees (a type 4 without its +0x18 never finishes)
uint8_t __fastcall FaceObjectiveTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    uint8_t* queue = Queue(ped);
    uint8_t* current = Queue_GetCurrent(queue);
    if (!current || uint32_t(At<int32_t>(current, 8) - 2) > 2) {
        return 0;
    }
    float heading = vc::Call<float>(current, 0x10);
    V3 dir;
    Vector_FromAngle(&dir, heading);
    float* move = At<float*>(ped, 0x2E0);
    move[0] = dir.x;
    move[1] = dir.y;
    move[2] = dir.z;
    move = At<float*>(ped, 0x2E0);
    move[3] = dir.x;
    move[4] = dir.y;
    move[5] = dir.z;
    At<float*>(ped, 0x2E0)[7] = 0.0f;
    if (At<int32_t>(current, 8) == 4 && !Objective_Get18(current)) {
        return 1;
    }
    V3 fwd;
    Placeable_GetForward(ped, &fwd);
    fwd.z = 0.0f;
    Vector_Normalise(&fwd);
    float dot = fwd.x * dir.x + dir.y * fwd.y + dir.z * fwd.z;
    float s = trk::CrtSin(0.34906587f);
    if (!(dot > 1.0f - s)) {
        return 1;
    }
    Queue_RemoveFirst(queue);
    return 0;
}

} // namespace

#define AIOBJTRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AIOBJTRK_REVERSED("InvestigateAreaTrack::Update", 0x00621700, &InvestigateAreaTrack_Update);
AIOBJTRK_REVERSED("AddWanderObjectiveTrack::Start", 0x00621810, &AddWanderObjectiveTrack_Start);
AIOBJTRK_REVERSED("RemoveObjectiveTrack::Start", 0x00621870, &RemoveObjectiveTrack_Start);
AIOBJTRK_REVERSED("MoveAwayFromStimulusTrack::Update", 0x006218B0, &MoveAwayFromStimulusTrack_Update);
AIOBJTRK_REVERSED("MoveToCurrentPlayerPositionTrack::Start", 0x006219C0, &MoveToCurrentPlayerPositionTrack_Start);
AIOBJTRK_REVERSED("MoveToCurrentPlayerPositionTrack::Update", 0x00621A40, &MoveToCurrentPlayerPositionTrack_Update);
AIOBJTRK_REVERSED("FollowPathTrack::Resolve", 0x00621DE0, &FollowPathTrack_Resolve);
AIOBJTRK_REVERSED("FleeOnPathOnBikeTrack::Resolve", 0x00621E20, &FleeOnPathOnBikeTrack_Resolve);
AIOBJTRK_REVERSED("DockTrack::Start", 0x00624980, &DockTrack_Start);
AIOBJTRK_REVERSED("DockTrack::Update", 0x00624A60, &DockTrack_Update);
AIOBJTRK_REVERSED("GreetTrack::Update", 0x00624C20, &GreetTrack_Update);
AIOBJTRK_REVERSED("TeacherTrack::Update", 0x00624E30, &TeacherTrack_Update);
AIOBJTRK_REVERSED("MoveToVehicleTrack::Update", 0x00624E90, &MoveToVehicleTrack_Update);
AIOBJTRK_REVERSED("MoveObjectiveTrack::Step", 0x00625070, &MoveObjectiveTrack_Step);
AIOBJTRK_REVERSED("FaceObjectiveTrack::Update", 0x00625260, &FaceObjectiveTrack_Update);
