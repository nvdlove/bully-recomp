// The action tree's path-following AI tracks (PathTrack: FollowPath and FleeOnPathOnBike,
// along a patrol path with the node actions and script callbacks of their objective),
// the cheering crowd round a fight (CheerFightTrack), and MoveObjectiveTrack's Update.
// 0x621AD0..0x62EF80 (the path track's own helpers of 0x61AE20..0x61B350 are in
// TracksAICombat.cpp).
// A path track: +0x18 the owner, +0x1C wait until (ms), +0x20 the node, +0x24 the one
// before, +0x28 the patrol path, +0x2C the node's next action, +0x30 its loop mode, +0x34
// the direction, +0x38.. (saved to the objective), +0x44 the node headed for (+0x48 at,
// +0x54 the direction through it), +0x64 doing the node's actions.
// The follow-path / flee-on-path objective's data (+0x10): +0 the path, +4 the node,
// +8.. , +0x14 the loop mode, +0x18 the direction, +0x1C / +0x20 / +0x24 script
// callbacks (reached / node / actions done), +0x28 wait until.
// A cheer-fight track: +0x18 / +0x1C the near / far distances, +0x20 / +0x24 the least /
// most speed, +0x28 the owner, +0x2C the fight, +0x38 the state, +0x3C may move,
// +0x40 / +0x44 the distances in use, +0x48 the speed, +0x4C from the ped to the fight
// (+0x58 its length), +0x5C going, +0x5D a spot picked, +0x60 the spot, +0x6C a timer.

#include <cstdint>

#include "game/ActionTracksCommon.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

GAME_VAR(void*, patrolPaths, 0x00D13CB4);
inline void* const lurStore = reinterpret_cast<void*>(0x00D02850);

inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(uint8_t*, __thiscall, PatrolPath_Get, 0x005ED3A0, void* pool, int32_t id);
GAME_FN(uint32_t, __thiscall, Path_GetNumNodes, 0x005EC670, uint8_t* path);
GAME_FN(float*, __thiscall, Path_GetNode, 0x005EC8C0, uint8_t* path, int32_t node);
GAME_FN(void, __thiscall, Segment_Set, 0x004D5A60, void* segment, float sx, float sy, float sz, float ex, float ey,
        float ez);
GAME_FN(V3*, __thiscall, Segment_PointAhead, 0x005E76C0, const void* segment, V3* out, const float* p, float radius,
        float* t);
GAME_FN(V3*, __thiscall, Steering_DirectionOf, 0x004A4720, uint8_t* steering, V3* out, float heading);
GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
GAME_FN(void, __thiscall, Steering_Stop, 0x004A4710, uint8_t* steering);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(void, __thiscall, Queue_RemoveFirst, 0x004702C0, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, Objectives_FindFollowPath, 0x005CF7D0, uint8_t* queue);
GAME_FN(uint8_t*, __thiscall, Objectives_FindFleeOnPath, 0x0062A800, uint8_t* queue);
GAME_FN(uint8_t, __thiscall, Events_IsOn, 0x005D80E0, uint8_t* events);
GAME_FN(void*, __thiscall, Events_New, 0x005D9C20, uint8_t* events);
GAME_FN(void, __cdecl, Event_Begin, 0x005D8270, void* event);
GAME_FN(int32_t, __thiscall, Events_GetId, 0x005D80D0, uint8_t* events);
GAME_FN(void, __cdecl, Event_SetId, 0x005D8670, void* event, int32_t id);
GAME_FN(void, __cdecl, Event_AddInt, 0x005D85C0, void* event, int32_t value);
GAME_FN(void, __cdecl, Event_Send, 0x005D82A0, void* event, int kind, int arg);
GAME_FN(int, __thiscall, ScriptRef_GetScript, 0x005D80B0, uint8_t* ref);
GAME_FN(void*, __thiscall, Lur_GetCurrentOrNull, 0x005DB960, void* store);
GAME_FN(void*, __thiscall, Lur_FindScript, 0x005DB8F0, void* store, int script);
GAME_FN(void, __thiscall, Lur_SetCurrent, 0x005DBDA0, void* store, void* script);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(float, __thiscall, Ped_Get478180, 0x00478180, uint8_t* ped, float value);
GAME_FN(void, __thiscall, Ped_SetMoveTarget, 0x00474A00, uint8_t* ped, const float* pos);
GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(uint8_t*, __thiscall, Objective_GetTarget, 0x008B6120, uint8_t* objective);
GAME_FN(uint8_t, __thiscall, Tether_IsInside, 0x004998F0, uint8_t* tether, float margin);
GAME_FN(uint8_t, __thiscall, Tether_IsPosAllowed, 0x00499840, uint8_t* tether, const float* pos, float range);
GAME_FN(uint8_t*, __thiscall, TetherTarget_GetEntity, 0x004995A0, uint8_t* target);
GAME_FN(V3*, __thiscall, TetherTarget_GetPosition, 0x004995B0, uint8_t* target, V3* out);
GAME_FN(float, __thiscall, Vector_Magnitude2D, 0x004246B0, const float* v);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(V3*, __cdecl, FlatDirection, 0x0041A520, V3* out, const V3* v, float length);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const V3* v);
GAME_FN(V3*, __cdecl, Vector_FromAngle, 0x00432DF0, V3* out, float angle);
GAME_FN(uint8_t, __cdecl, Walk_TestLine, 0x005EB1B0, const float* from, const float* to, uint8_t* blocked, int a, int b);
GAME_FN(float, __cdecl, DistanceBeyond, 0x0041A4D0, const float* a, const float* b, float range);
GAME_FN(void, __thiscall, Timer_Reset, 0x0045BD10, uint8_t* timer);
GAME_FN(void, __thiscall, Countdown_StartIfStopped, 0x0045BD50, uint8_t* timer, uint32_t ms);
GAME_FN(uint8_t, __thiscall, Timer_IsDone, 0x0045BDD0, uint8_t* timer);
// the tracks' own, by address (so their modes apply)
GAME_FN(uint8_t, __thiscall, PathTrack_Step, 0x0061AE20, uint8_t* self, int32_t* index);
GAME_FN(uint8_t, __thiscall, PathTrack_IsStop, 0x0061AEC0, uint8_t* self, int32_t index);
GAME_FN(uint8_t, __thiscall, PathTrack_PastNode, 0x0061AF40, uint8_t* self, uint8_t* ped, float distance);
GAME_FN(void, __thiscall, PathTrack_SetNode, 0x0061B100, uint8_t* self, int32_t previous, int32_t index);
GAME_FN(void, __thiscall, PathTrack_Aim, 0x00621B90, uint8_t* self, V3* out, uint8_t* ped, float scale);
GAME_FN(void, __thiscall, FollowPathTrack_SetHeading, 0x00621D60, uint8_t* self, const float* degrees);
GAME_FN(void, __thiscall, PathTrack_Load, 0x0062C340, uint8_t* self);
GAME_FN(void, __thiscall, PathTrack_Save, 0x0062C390, uint8_t* self);
GAME_FN(void, __thiscall, FollowPathTrack_FaceNode, 0x0062C460, uint8_t* self);
GAME_FN(void, __thiscall, PathTrack_NotifyNode, 0x0062E0C0, uint8_t* self, int32_t node);
GAME_FN(void, __thiscall, FollowPathTrack_NotifyActionsDone, 0x0062E1A0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PathTrack_Advance, 0x0062E750, uint8_t* self, uint8_t* ped);
GAME_FN(void, __thiscall, PathTrack_CatchUp, 0x0062E7A0, uint8_t* self, int32_t node);
GAME_FN(uint8_t, __thiscall, FollowPathTrack_NextAction, 0x0062E8E0, uint8_t* self);
GAME_FN(void, __thiscall, PathTrack_CheckPassed, 0x0062EC00, uint8_t* self);
GAME_FN(uint8_t, __thiscall, CheerFightTrack_IsSpotClear, 0x00621AF0, uint8_t* self, const float* pos);
GAME_FN(uint8_t, __thiscall, CheerFightTrack_FindFight, 0x0062BE90, uint8_t* self);
GAME_FN(void, __thiscall, CheerFightTrack_SetState, 0x0062C050, uint8_t* self, int32_t state);
GAME_FN(uint8_t, __thiscall, CheerFightTrack_PickSpot, 0x0062C0E0, uint8_t* self);
GAME_FN(void, __thiscall, CheerFightTrack_Step, 0x0062DF40, uint8_t* self);
GAME_FN(uint8_t, __thiscall, MoveObjectiveTrack_Step, 0x00625070, uint8_t* self);
GAME_FN(uint8_t, __thiscall, FleeOnRoadOnBikeTrack_Ride, 0x00621F10, uint8_t* self);

constexpr double kPiD = 3.1415927410125732;

uint8_t* PedOf(uint8_t* self, int at) {
    return OwnerPed(At<uint8_t*>(self, at));
}
uint8_t* Path(uint8_t* self) {
    return PatrolPath_Get(patrolPaths, At<int32_t>(self, 0x28));
}
// the follow-path (or flee-on-path) objective's data (+0x10 of it), null without one
uint8_t* PathData(uint8_t* self) {
    uint8_t* queue = PedOf(self, 0x18) + 0x5F0;
    uint8_t* objective = Objectives_FindFollowPath(queue);
    if (!objective) {
        objective = Objectives_FindFleeOnPath(queue);
        if (!objective) {
            return nullptr;
        }
    }
    return objective + 0x10;
}
float FlatLength(const V3& d) {
    return SqrtAbs(d.x * d.x + d.y * d.y);
}
// the ped arrived at its node: within its radius (+0x51C; 3x on a vehicle unless it's at
// the path's last node of a one-way path)
float ArriveScale(uint8_t* self, uint8_t* ped, uint8_t* path) {
    if (!Ped_IsSeatedInVehicle(ped)) {
        return 1.0f;
    }
    uint32_t node = At<uint32_t>(self, 0x20);
    if (node < Path_GetNumNodes(path) - 1 || At<int32_t>(self, 0x30) != 0) {
        return 3.0f;
    }
    return 1.0f;
}
// a callback of the objective's script (the ref at the data's +at): the ped's id, the
// path, then the extra values, sent as kind
void Notify(uint8_t* self, int at, int kind, int32_t a, int32_t b, bool extra, bool useFound) {
    uint8_t* data = PathData(self);
    if (!data) {
        return;
    }
    uint8_t* ref = data + at;
    if (!Events_IsOn(ref)) {
        return;
    }
    void* saved = Lur_GetCurrentOrNull(lurStore);
    void* script = Lur_FindScript(lurStore, ScriptRef_GetScript(ref));
    if (!script) {
        return;
    }
    if (useFound) {
        Lur_SetCurrent(lurStore, script);
    }
    void* event = Events_New(ref);
    Event_Begin(event);
    Event_SetId(event, Events_GetId(ref));
    Event_AddInt(event, vc::Call<int32_t>(PedOf(self, 0x18), 0xAC));
    Event_AddInt(event, At<int32_t>(self, 0x28));
    Event_AddInt(event, a);
    if (extra) {
        Event_AddInt(event, b);
    }
    Event_Send(event, kind, 0);
    Lur_SetCurrent(lurStore, saved);
}

// 0x621AD0 (thiscall): CheerFightTrack::End: the ped's cheering (+0x136C) and facing
// freed
uint8_t __fastcall CheerFightTrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(PedOf(self, 0x28), 0x136C) = 0;
    At<int32_t>(PedOf(self, 0x28), 0x1340) = 0;
    return 1;
}

// 0x621AF0 (thiscall): a cheering spot's clear: a walkable straight line from the fight
// (+0x2C) and not within 0.7 of a ped around (the scanner's list; their +0x1E18 spots)
uint8_t __fastcall CheerFightTrack_IsSpotClearImpl(uint8_t* self, re::Edx, const float* pos) {
    uint8_t blocked = 0;
    if (!Walk_TestLine(&At<float>(self, 0x2C), pos, &blocked, 0, 0) || blocked) {
        return 0;
    }
    uint8_t* ped = PedOf(self, 0x28);
    int32_t count = At<int32_t>(ped, 0x3C0);
    for (int32_t i = 0; i < count; i++) {
        uint8_t* other = At<uint8_t*>(ped, 0x3C4 + i * 4);
        if (other && DistanceBeyond(pos, &At<float>(other, 0x1E18), 0.7f) < 0.0f) {
            return 0;
        }
    }
    return 1;
}

// 0x621B90 (thiscall): the point to steer for along the path: the node itself at a
// stop, else ahead on the leg from the node before (3 + 3 (12 on a kind-1 vehicle) *
// scale ahead of the ped); past the leg's end the next node's taken
void __fastcall PathTrack_AimImpl(uint8_t* self, re::Edx, V3* out, uint8_t* ped, float scale) {
    uint8_t* path = Path(self);
    if (!path) {
        return;
    }
    int32_t* node = &At<int32_t>(self, 0x20);
    const float* n = Path_GetNode(path, *node);
    V3 at = {n[0], n[1], n[2]};
    if (At<int32_t>(self, 0x24) == -1) {
        if (PathTrack_IsStop(self, *node)) {
            *out = at;
            return;
        }
        PathTrack_SetNode(self, -1, *node);
        At<int32_t>(self, 0x24) = *node;
        PathTrack_Step(self, node);
        n = Path_GetNode(path, *node);
        at = {n[0], n[1], n[2]};
    }
    const float* b = Path_GetNode(path, At<int32_t>(self, 0x24));
    uint8_t segment[0x20];
    Segment_Set(segment, b[0], b[1], b[2], at.x, at.y, at.z);
    const float* pp = Pos(ped);
    V3 from = {pp[0], pp[1], pp[2]};
    float t = 0.0f;
    float ahead = 3.0f;
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (vehicle && At<int32_t>(vehicle, 0x46C) == 1) {
        ahead = 12.0f;
    }
    V3 p = from;
    float radius = ahead * scale + 3.0f;
    V3 r;
    *out = *Segment_PointAhead(segment, &r, &p.x, radius, &t);
    if (t >= 1.0f) {
        if (!PathTrack_IsStop(self, *node)) {
            At<int32_t>(self, 0x24) = *node;
            PathTrack_Step(self, node);
        }
    }
}

// 0x621D60 (thiscall): the ped turned to a heading (degrees): standing, facing it
void __fastcall FollowPathTrack_SetHeadingImpl(uint8_t* self, re::Edx, const float* degrees) {
    uint8_t* ped = PedOf(self, 0x18);
    float* move = At<float*>(ped, 0x2E0);
    float product = float(double(*degrees) * kPiD);
    float radians = float(double(product) / 180.0);
    V3 d;
    V3* r = Steering_DirectionOf(ped + 0x654, &d, radians);
    move[0] = r->x;
    move[1] = r->y;
    move[2] = r->z;
    move = At<float*>(ped, 0x2E0);
    move[3] = move[0];
    move[4] = move[1];
    move[5] = move[2];
    At<float*>(ped, 0x2E0)[7] = 0.0f;
}

// 0x62BE90 (thiscall): the fight (+0x2C): an attack objective's target, the ped's
// target, a fight stimulus (kinds 5, 7, 0x17: its second ped, its first, else its
// spot; the ped targeting it) or the target again
uint8_t __fastcall CheerFightTrack_FindFightImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = PedOf(self, 0x28);
    auto setFight = [&](const float* p) {
        At<float>(self, 0x2C) = p[0];
        At<float>(self, 0x30) = p[1];
        At<float>(self, 0x34) = p[2];
        return uint8_t(1);
    };
    uint8_t* current = Queue_GetCurrent(ped + 0x5F0);
    if (current && At<int32_t>(current, 8) == 7) {
        if (Objective_GetTarget(current)) {
            return setFight(Pos(Objective_GetTarget(current)));
        }
        if (Ped_GetTarget(ped)) {
            return setFight(Pos(Ped_GetTarget(ped)));
        }
    }
    int32_t* stimulus = At<int32_t*>(ped, 0x1D30);
    if (stimulus && (stimulus[0] == 5 || stimulus[0] == 7 || stimulus[0] == 0x17)) {
        uint8_t* first = reinterpret_cast<uint8_t*>(stimulus[1]);
        if (first && !IsPedEntity(first)) {
            first = nullptr;
        }
        uint8_t* second = reinterpret_cast<uint8_t*>(stimulus[2]);
        if (second && IsPedEntity(second) && ped != second) {
            Ped_SetTargetEntity(ped, second);
            return setFight(Pos(second));
        }
        if (first && ped != first) {
            Ped_SetTargetEntity(ped, first);
            return setFight(Pos(first));
        }
        Ped_SetTargetEntity(ped, nullptr);
        return setFight(reinterpret_cast<const float*>(&stimulus[3]));
    }
    if (Ped_GetTarget(ped)) {
        return setFight(Pos(Ped_GetTarget(ped)));
    }
    return 0;
}

// 0x62C050 (thiscall): the state: 0 standing (the timer cleared), 1 going (a spot to
// pick from where the ped is, a random speed)
void __fastcall CheerFightTrack_SetStateImpl(uint8_t* self, re::Edx, int32_t state) {
    At<int32_t>(self, 0x38) = state;
    if (state == 0) {
        Timer_Reset(self + 0x6C);
        return;
    }
    if (state != 1) {
        return;
    }
    self[0x5D] = 1;
    const float* p = Pos(PedOf(self, 0x28));
    float most = At<float>(self, 0x24);
    At<float>(self, 0x60) = p[0];
    float least = At<float>(self, 0x20);
    At<float>(self, 0x64) = p[1];
    At<float>(self, 0x68) = p[2];
    float q = float(GameRand()) / 32767.0f;
    At<float>(self, 0x48) = q * (most - least) + least;
}

// 0x62C0E0 (thiscall): a cheering spot (+0x60): a random one round the fight on the
// ped's side (the distances' sum out), or the last one pulled to between 0.75 of the
// far distance either side of the near one; up to 8 tries for a clear one
uint8_t __fastcall CheerFightTrack_PickSpotImpl(uint8_t* self, re::Edx) {
    const float* fight = &At<float>(self, 0x2C);
    V3 d;
    V3* r = VectorSub(&d, Pos(PedOf(self, 0x28)), fight);
    V3 away = {r->x, r->y, 0.0f};
    Vector_Normalise(&away);
    float out = At<float>(self, 0x44) + At<float>(self, 0x40);
    float* spot = &At<float>(self, 0x60);
    for (int i = 0; i < 8; i++) {
        if (self[0x5D] == 0) {
            spot[0] = fight[0];
            spot[1] = fight[1];
            self[0x5D] = 1;
            spot[2] = fight[2];
            float heading = HeadingFromVector(&away);
            float q = float(GameRand()) / 32767.0f;
            float turn = q * 3.14159274f - 1.57079637f;
            float angle = turn + heading;
            V3 dir;
            V3* v = Vector_FromAngle(&dir, angle);
            V3 off = {out * v->x, v->y * out, out * v->z};
            spot[0] = spot[0] + off.x;
            spot[1] = spot[1] + off.y;
            spot[2] = spot[2] + off.z;
        } else {
            V3 e;
            VectorSub(&e, fight, spot);
            e.z = 0.0f;
            float len = SqrtAbs(e.y * e.y + e.x * e.x + 0.0f * 0.0f);
            float band = At<float>(self, 0x44) * 0.75f;
            float lo = At<float>(self, 0x40) - band;
            float hi = band + At<float>(self, 0x40);
            if (lo > len) {
                len = lo;
            } else if (hi < len) {
                len = hi;
            }
            V3 f;
            V3* g = VectorSub(&f, spot, fight);
            V3 dir = *g;
            Vector_Normalise(&dir);
            V3 off = {dir.x * len, dir.y * len, len * dir.z};
            V3 sum;
            V3* s = VectorAdd(&sum, fight, &off.x);
            spot[0] = s->x;
            spot[1] = s->y;
            spot[2] = s->z;
        }
        if (!CheerFightTrack_IsSpotClear(self, spot)) {
            self[0x5D] = 0;
        } else if (self[0x5D] != 0) {
            break;
        }
    }
    return self[0x5D];
}

// 0x62C340 (thiscall): the path track's place from the objective
void __fastcall PathTrack_LoadImpl(uint8_t* self, re::Edx) {
    uint8_t* data = PathData(self);
    if (!data) {
        return;
    }
    At<int32_t>(self, 0x28) = At<int32_t>(data, 0);
    At<int32_t>(self, 0x20) = At<int32_t>(data, 4);
    At<int32_t>(self, 0x30) = At<int32_t>(data, 0x14);
    At<int32_t>(self, 0x34) = At<int32_t>(data, 0x18);
    At<int32_t>(self, 0x1C) = At<int32_t>(data, 0x28);
}

// 0x62C390 (thiscall): and back to the objective
void __fastcall PathTrack_SaveImpl(uint8_t* self, re::Edx) {
    uint8_t* data = PathData(self);
    if (!data) {
        return;
    }
    At<int32_t>(data, 0) = At<int32_t>(self, 0x28);
    At<int32_t>(data, 4) = At<int32_t>(self, 0x20);
    At<int32_t>(data, 8) = At<int32_t>(self, 0x38);
    At<int32_t>(data, 0xC) = At<int32_t>(self, 0x3C);
    At<int32_t>(data, 0x10) = At<int32_t>(self, 0x40);
    At<int32_t>(data, 0x14) = At<int32_t>(self, 0x30);
    At<int32_t>(data, 0x18) = At<int32_t>(self, 0x34);
    At<int32_t>(data, 0x28) = At<int32_t>(self, 0x1C);
}

// 0x62C3F0 / 0x62C4F0 (thiscall): FollowPath / FleeOnPathOnBike tracks' Start: reset,
// loaded from the objective; going while its path exists
template <bool FollowPath>
uint8_t __fastcall PathTrack_StartOnPath(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    At<int32_t>(self, 0x28) = -1;
    At<int32_t>(self, 0x24) = -1;
    At<int32_t>(self, 0x44) = -1;
    At<int32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x2C) = 0;
    At<int32_t>(self, 0x30) = 1;
    At<int32_t>(self, 0x34) = 1;
    At<int32_t>(self, 0x1C) = 0;
    if constexpr (FollowPath) {
        At<int32_t>(self, 0x64) = 0;
    }
    PathTrack_Load(self);
    return PatrolPath_Get(patrolPaths, At<int32_t>(self, 0x28)) != nullptr ? 1 : 0;
}

// 0x62C440 (thiscall): FollowPathTrack::End: saved, no wait
uint8_t __fastcall FollowPathTrack_End(uint8_t* self, re::Edx) {
    PathTrack_Save(self);
    At<int32_t>(self, 0x1C) = 0;
    return 1;
}

// 0x62C460 (thiscall): facing the node's heading (+0xC; +180 on a vehicle, else -90)
void __fastcall FollowPathTrack_FaceNodeImpl(uint8_t* self, re::Edx) {
    uint8_t* path = Path(self);
    if (!path) {
        return;
    }
    const float* n = Path_GetNode(path, At<int32_t>(self, 0x20));
    V3 h = {n[3], n[4], n[5]};
    if (At<uint8_t*>(PedOf(self, 0x18), 0x1554)) {
        h.x = h.x + 180.0f;
    } else {
        h.x = h.x - 90.0f;
    }
    FollowPathTrack_SetHeading(self, &h.x);
}

// 0x62C540 (thiscall): FleeOnPathOnBikeTrack::End: saved
uint8_t __fastcall FleeOnPathOnBikeTrack_End(uint8_t* self, re::Edx) {
    PathTrack_Save(self);
    return 1;
}

// 0x62C550 (thiscall): FleeOnRoadOnBikeTrack::Update: the ride (its steering's result)
uint8_t __fastcall FleeOnRoadOnBikeTrack_Update(uint8_t* self, re::Edx, float) {
    return FleeOnRoadOnBikeTrack_Ride(self);
}

// 0x62C560 (thiscall): MoveObjectiveTrack::Update: on a bike (kind 1) the vehicle's
// step; else to a move objective's target (0, 1, 0x18) at its speed (through 0x478180)
// and radius (flags 0x1C, 0x2001C with its +0x18); there it's done (removed, 0)
uint8_t __fastcall MoveObjectiveTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = PedOf(self, 0x18);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    if (vehicle && At<int32_t>(vehicle, 0x46C) == 1) {
        return MoveObjectiveTrack_Step(self);
    }
    uint8_t* queue = ped + 0x5F0;
    uint8_t* current = Queue_GetCurrent(queue);
    if (!current) {
        return 0;
    }
    int32_t type = At<int32_t>(current, 8);
    if (type < 0 || (type > 1 && type != 0x18)) {
        return 0;
    }
    float speed = Ped_Get478180(ped, At<float>(current, 0x10));
    uint32_t flags = current[0x18] != 0 ? 0x2001C : 0x1C;
    float radius = At<float>(current, 0x14);
    float* to = vc::Call<float*>(current, 0x10);
    if (Steering_GoTo(ped + 0x654, to[0], to[1], to[2], flags, speed, radius, nullptr, 0)) {
        return 1;
    }
    Queue_RemoveFirst(queue);
    return 0;
}

// 0x62DEB0 (thiscall): CheerFightTrack::Start: going (state 1) from where the ped is
uint8_t __fastcall CheerFightTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    At<int32_t>(OwnerPed(owner), 0x136C) = 0;
    At<float>(self, 0x48) = 1.0f;
    self[0x5C] = 0;
    At<int32_t>(self, 0x38) = 1;
    self[0x5D] = 1;
    const float* p = Pos(PedOf(self, 0x28));
    float most = At<float>(self, 0x24);
    At<float>(self, 0x60) = p[0];
    float least = At<float>(self, 0x20);
    At<float>(self, 0x64) = p[1];
    At<float>(self, 0x68) = p[2];
    float q = float(GameRand()) / 32767.0f;
    At<float>(self, 0x48) = q * (most - least) + least;
    return 1;
}

// 0x62DF40 (thiscall): the state step: 1 go to a clear spot (flags 0x214; standing when
// there), or at the fight without one; 0 stand: too far off (or the spot taken, checked
// every 8th frame) go again, too close for 5 s go again
void __fastcall CheerFightTrack_StepImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = PedOf(self, 0x28);
    int32_t state = At<int32_t>(self, 0x38);
    if (state == 1) {
        if (CheerFightTrack_PickSpot(self)) {
            float* spot = &At<float>(self, 0x60);
            Ped_SetMoveTarget(ped, spot);
            self[0x5C] = Steering_GoTo(ped + 0x654, spot[0], spot[1], spot[2], 0x214, At<float>(self, 0x48), 0.3f,
                                       nullptr, 0);
            if (self[0x5C] == 0) {
                At<int32_t>(self, 0x38) = 0;
                Timer_Reset(self + 0x6C);
            }
            return;
        }
        float radius = At<float>(self, 0x44) + At<float>(self, 0x40);
        self[0x5C] = Steering_GoTo(ped + 0x654, At<float>(self, 0x2C), At<float>(self, 0x30), At<float>(self, 0x34),
                                   0x214, At<float>(self, 0x48), radius, nullptr, 0);
        return;
    }
    if (state != 0) {
        return;
    }
    Steering_Stop(ped + 0x654);
    uint8_t move = self[0x3C];
    self[0x5C] = 0;
    bool reset = true;
    if (move) {
        if (At<float>(self, 0x44) + At<float>(self, 0x40) < At<float>(self, 0x58)) {
            CheerFightTrack_SetState(self, 1);
            return;
        }
        if (At<float>(self, 0x40) - At<float>(self, 0x44) > At<float>(self, 0x58)) {
            reset = false;
            Countdown_StartIfStopped(self + 0x6C, 5000);
            if (Timer_IsDone(self + 0x6C)) {
                CheerFightTrack_SetState(self, 1);
                return;
            }
        }
    }
    if (reset) {
        Timer_Reset(self + 0x6C);
    }
    if (self[0x3C] != 0 && Ped_EveryNthFrame(ped, 8) && !CheerFightTrack_IsSpotClear(self, &At<float>(self, 0x60))) {
        CheerFightTrack_SetState(self, 1);
    }
}

// 0x62E0C0 (thiscall): the objective's script told the node was reached (callback
// +0x20: the ped, the path, the node)
void __fastcall PathTrack_NotifyNodeImpl(uint8_t* self, re::Edx, int32_t node) {
    Notify(self, 0x20, 3, node, 0, false, false);
}

// 0x62E1A0 (thiscall): the objective's script told the node's actions are done
// (callback +0x24: the ped, the path, the node, the action)
void __fastcall FollowPathTrack_NotifyActionsDoneImpl(uint8_t* self, re::Edx) {
    Notify(self, 0x24, 4, At<int32_t>(self, 0x20), At<int32_t>(self, 0x2C), true, false);
}

// 0x62E5A0 (thiscall): CheerFightTrack::Update: the distances from +0x18 / +0x1C; may
// move unless stationary; the fight found (else done); standing: facing it, cheering
uint8_t __fastcall CheerFightTrack_Update(uint8_t* self, re::Edx, float) {
    At<float>(self, 0x40) = At<float>(self, 0x18);
    At<float>(self, 0x44) = At<float>(self, 0x1C);
    uint8_t* ped = PedOf(self, 0x28);
    self[0x3C] = ped[0x4E4] != 0 ? 0 : 1;
    if (!CheerFightTrack_FindFight(self)) {
        return 0;
    }
    V3 d;
    V3* r = VectorSub(&d, &At<float>(self, 0x2C), Pos(ped));
    At<float>(self, 0x4C) = r->x;
    At<float>(self, 0x50) = r->y;
    At<float>(self, 0x54) = r->z;
    At<float>(self, 0x58) = SqrtAbs(At<float>(self, 0x4C) * At<float>(self, 0x4C) +
                                    At<float>(self, 0x50) * At<float>(self, 0x50));
    if (self[0x3C] == 0) {
        At<int32_t>(self, 0x38) = 0;
        Timer_Reset(self + 0x6C);
    }
    CheerFightTrack_Step(self);
    V3 dir = {At<float>(self, 0x4C), At<float>(self, 0x50), At<float>(self, 0x54)};
    Vector_Normalise(&dir);
    float* move = At<float*>(ped, 0x2E0);
    move[3] = dir.x;
    move[4] = dir.y;
    move[5] = dir.z;
    if (self[0x5C] == 0) {
        At<int32_t>(PedOf(self, 0x28), 0x136C) = 1;
        At<int32_t>(PedOf(self, 0x28), 0x1340) = 0;
        move = At<float*>(ped, 0x2E0);
        move[0] = dir.x;
        move[1] = dir.y;
        move[2] = dir.z;
        At<float*>(ped, 0x2E0)[7] = 0.0f;
        return 1;
    }
    At<int32_t>(PedOf(self, 0x28), 0x136C) = 0;
    At<int32_t>(PedOf(self, 0x28), 0x1340) = 1;
    return 1;
}

// 0x62E750 (thiscall): on to the next node (the reached one told to the script); past
// the end the node's 0 and the track resolves again (vf5); the argument isn't read
uint8_t __fastcall PathTrack_AdvanceImpl(uint8_t* self, re::Edx, uint8_t*) {
    int32_t reached = At<int32_t>(self, 0x20);
    PathTrack_NotifyNode(self, reached);
    At<int32_t>(self, 0x24) = At<int32_t>(self, 0x20);
    if (PathTrack_Step(self, &At<int32_t>(self, 0x20))) {
        PathTrack_SetNode(self, reached, At<int32_t>(self, 0x20));
        return 1;
    }
    At<int32_t>(self, 0x20) = 0;
    vc::Call<void>(self, acttree::kTrackResolve);
    return 0;
}

// 0x62E7A0 (thiscall): the node headed for (+0x44) moved on to node (each passed one told
// to the script), then the script's reached callback (+0x1C) for node
void __fastcall PathTrack_CatchUpImpl(uint8_t* self, re::Edx, int32_t node) {
    while (At<int32_t>(self, 0x44) != node) {
        if (At<int32_t>(self, 0x44) == -1) {
            break;
        }
        PathTrack_CatchUp(self, At<int32_t>(self, 0x44));
        PathTrack_NotifyNode(self, At<int32_t>(self, 0x44));
        int32_t at = At<int32_t>(self, 0x44);
        int32_t next = at;
        if (!PathTrack_Step(self, &next)) {
            At<int32_t>(self, 0x44) = -1;
        } else {
            PathTrack_SetNode(self, at, next);
        }
    }
    Notify(self, 0x1C, 3, node, 0, false, true);
}

// 0x62E8E0 (thiscall): the node's next action (+0x2C of its list, +0x20): 0 face a
// heading, 1 stand; each waits its time (+0x10 s), then the script's told; after the
// last, on to the next node
uint8_t __fastcall FollowPathTrack_NextActionImpl(uint8_t* self, re::Edx) {
    uint8_t* path = Path(self);
    if (!path) {
        return 0;
    }
    uint8_t* actions = At<uint8_t*>(Path_GetNode(path, At<int32_t>(self, 0x20)), 0x20);
    if (actions && At<uint32_t>(self, 0x2C) < At<uint32_t>(actions, 8)) {
        uint8_t* action = At<re::Ptr32<uint8_t>*>(actions, 0)[At<uint32_t>(self, 0x2C)];
        int32_t kind = At<int32_t>(action, 0);
        if (kind == 0) {
            V3 h = {At<float>(action, 4), At<float>(action, 8), At<float>(action, 0xC)};
            h.x = h.x - 90.0f;
            FollowPathTrack_SetHeading(self, &h.x);
        } else if (kind == 1) {
            At<float*>(PedOf(self, 0x18), 0x2E0)[7] = 0.0f;
        }
        float wait = At<float>(action, 0x10) * -1000.0f;
        At<int32_t>(self, 0x2C) += 1;
        At<uint32_t>(self, 0x1C) = timeInMs - uint32_t(int32_t(int64_t(wait)));
        FollowPathTrack_NotifyActionsDone(self);
        return 1;
    }
    At<int32_t>(self, 0x64) = 0;
    At<int32_t>(self, 0x1C) = 0;
    At<int32_t>(self, 0x2C) = 0;
    return PathTrack_Advance(self, PedOf(self, 0x18));
}

// 0x62E9F0 (thiscall): FleeOnPathOnBikeTrack::Update: the objective's next node kept;
// at the node (or past it) on to the next; the ride (vf6)
uint8_t __fastcall FleeOnPathOnBikeTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* path = Path(self);
    if (!path) {
        return 0;
    }
    uint8_t* ped = PedOf(self, 0x18);
    const float* n = Path_GetNode(path, At<int32_t>(self, 0x20));
    V3 d;
    V3* r = VectorSub(&d, n, Pos(ped));
    float dist = SqrtAbs(r->x * r->x + r->y * r->y);
    uint8_t* current = Queue_GetCurrent(ped + 0x5F0);
    if (current && At<int32_t>(current, 8) == 0xD) {
        uint32_t node = At<uint32_t>(self, 0x20);
        At<uint32_t>(current, 0x14) = node;
        if (node < Path_GetNumNodes(path) - 1) {
            At<uint32_t>(current, 0x14) = node + 1;
        }
    }
    float scale = ArriveScale(self, ped, path);
    if (!(At<float>(ped, 0x51C) * scale > dist) && !PathTrack_PastNode(self, ped, dist)) {
        return vc::Call<uint8_t>(self, 0x18);
    }
    PathTrack_CatchUp(self, At<int32_t>(self, 0x20));
    if (PathTrack_Advance(self, ped)) {
        return vc::Call<uint8_t>(self, 0x18);
    }
    return 0;
}

// 0x62EC00 (thiscall): the node headed for passed (the ped beyond it along its
// direction): caught up to and on to the one after
void __fastcall PathTrack_CheckPassedImpl(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x44) == -1) {
        return;
    }
    const float* pp = Pos(PedOf(self, 0x18));
    V3 from = {pp[0], pp[1], pp[2]};
    V3 d;
    VectorSub(&d, &At<float>(self, 0x48), &from.x);
    float along = At<float>(self, 0x58) * d.y + d.x * At<float>(self, 0x54) + At<float>(self, 0x5C) * d.z;
    if (!(along <= 0.0f)) {
        return;
    }
    PathTrack_CatchUp(self, At<int32_t>(self, 0x44));
    PathTrack_NotifyNode(self, At<int32_t>(self, 0x44));
    int32_t at = At<int32_t>(self, 0x44);
    int32_t next = at;
    if (!PathTrack_Step(self, &next)) {
        At<int32_t>(self, 0x44) = -1;
        return;
    }
    PathTrack_SetNode(self, at, next);
}

// 0x62ECD0 (thiscall): FollowPathTrack::Update: waiting (+0x1C) until due (then the next
// action, or on); at the node (or past it): its actions, its wait (facing its heading),
// or on to the next; the move (vf6). A bike at the one-way path's end stops once close
// for its speed.
uint8_t __fastcall FollowPathTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = PedOf(self, 0x18);
    uint32_t until = At<uint32_t>(self, 0x1C);
    if (until != 0) {
        if (timeInMs <= until) {
            return 1;
        }
        At<int32_t>(self, 0x1C) = 0;
        if (At<int32_t>(self, 0x64) == 1) {
            return FollowPathTrack_NextAction(self);
        }
        if (!PathTrack_Advance(self, ped)) {
            return 0;
        }
    }
    uint8_t* path = Path(self);
    if (!path) {
        return 0;
    }
    float* node = Path_GetNode(path, At<int32_t>(self, 0x20));
    V3 d;
    V3* r = VectorSub(&d, node, Pos(ped));
    float dist = SqrtAbs(r->x * r->x + r->y * r->y);
    float scale = ArriveScale(self, ped, path);
    bool arrived = At<float>(ped, 0x51C) * scale > dist || PathTrack_PastNode(self, ped, dist);
    if (arrived) {
        PathTrack_CatchUp(self, At<int32_t>(self, 0x20));
        if (At<int32_t>(node, 0x20) != 0) {
            At<int32_t>(self, 0x64) = 1;
            return FollowPathTrack_NextAction(self);
        }
        float wait = At<float>(node, 0x1C);
        if (wait > 0.0f) {
            At<uint32_t>(self, 0x1C) = timeInMs - uint32_t(int32_t(int64_t(wait * -1000.0f)));
            FollowPathTrack_FaceNode(self);
            return 1;
        }
        if (!PathTrack_Advance(self, ped)) {
            if (At<uint8_t*>(ped, 0x1554)) {
                FollowPathTrack_FaceNode(self);
            }
            return 0;
        }
    }
    uint8_t going = vc::Call<uint8_t>(self, 0x18);
    uint8_t* bike = At<uint8_t*>(ped, 0x1554);
    if (bike && At<uint32_t>(self, 0x20) >= Path_GetNumNodes(path) - 1 && At<int32_t>(self, 0x30) == 0) {
        double far = dist;
        float speed = Vector_Magnitude2D(&At<float>(bike, 0x124));
        float stop = speed / 0.3f * 17.0f + 1.0f;
        if (double(stop) > far) {
            float* move = At<float*>(ped, 0x2E0);
            move[0] = 0.0f;
            move[1] = 0.0f;
            move[2] = 0.0f;
            At<float*>(ped, 0x2E0)[7] = 0.0f;
        }
    }
    return going;
}

// 0x62EF80 (thiscall): FollowPathTrack's move (vf6): along the path (its speed from the
// objective's +0x3C (0.3) through 0x478180, flags 0x20C1C); outside its tether with the
// point outside too it stops, facing where the tether is
uint8_t __fastcall FollowPathTrack_Move(uint8_t* self, re::Edx) {
    uint8_t* ped = PedOf(self, 0x18);
    if (!Path(self)) {
        return 0;
    }
    float value = 0.3f;
    uint8_t* current = Queue_GetCurrent(ped + 0x5F0);
    if (current && At<int32_t>(current, 8) == 0xC) {
        value = At<float>(current, 0x3C);
    }
    float speed = Ped_Get478180(ped, value);
    V3 goal = {0.0f, 0.0f, 0.0f};
    PathTrack_Aim(self, &goal, ped, speed);
    if (At<int32_t>(ped, 0x360) != 0 && !Tether_IsInside(ped + 0x358, 0.0f) &&
        !Tether_IsPosAllowed(ped + 0x358, &goal.x, 0.0f)) {
        Steering_Stop(ped + 0x654);
        if (TetherTarget_GetEntity(ped + 0x35C)) {
            const float* from = Pos(ped);
            V3 at, d, dir;
            TetherTarget_GetPosition(ped + 0x35C, &at);
            VectorSub(&d, &at.x, from);
            FlatDirection(&dir, &d, 1.0f);
            float* move = At<float*>(ped, 0x2E0);
            move[0] = dir.x;
            move[1] = dir.y;
            move[2] = dir.z;
            move = At<float*>(ped, 0x2E0);
            move[3] = dir.x;
            move[4] = dir.y;
            move[5] = dir.z;
        }
        return 1;
    }
    PathTrack_CheckPassed(self);
    return Steering_GoTo(ped + 0x654, goal.x, goal.y, goal.z, 0x20C1C, speed, 0.3f, nullptr, 0);
}

} // namespace

#define AIPATHTRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AIPATHTRK_REVERSED("CheerFightTrack::End", 0x00621AD0, &CheerFightTrack_End);
AIPATHTRK_REVERSED("CheerFightTrack::IsSpotClear", 0x00621AF0, &CheerFightTrack_IsSpotClearImpl);
AIPATHTRK_REVERSED("PathTrack::Aim", 0x00621B90, &PathTrack_AimImpl);
AIPATHTRK_REVERSED("FollowPathTrack::SetHeading", 0x00621D60, &FollowPathTrack_SetHeadingImpl);
AIPATHTRK_REVERSED("CheerFightTrack::FindFight", 0x0062BE90, &CheerFightTrack_FindFightImpl);
AIPATHTRK_REVERSED("CheerFightTrack::SetState", 0x0062C050, &CheerFightTrack_SetStateImpl);
AIPATHTRK_REVERSED("CheerFightTrack::PickSpot", 0x0062C0E0, &CheerFightTrack_PickSpotImpl);
AIPATHTRK_REVERSED("PathTrack::Load", 0x0062C340, &PathTrack_LoadImpl);
AIPATHTRK_REVERSED("PathTrack::Save", 0x0062C390, &PathTrack_SaveImpl);
AIPATHTRK_REVERSED("FollowPathTrack::Start", 0x0062C3F0, &PathTrack_StartOnPath<true>);
AIPATHTRK_REVERSED("FollowPathTrack::End", 0x0062C440, &FollowPathTrack_End);
AIPATHTRK_REVERSED("FollowPathTrack::FaceNode", 0x0062C460, &FollowPathTrack_FaceNodeImpl);
AIPATHTRK_REVERSED("FleeOnPathOnBikeTrack::Start", 0x0062C4F0, &PathTrack_StartOnPath<false>);
AIPATHTRK_REVERSED("FleeOnPathOnBikeTrack::End", 0x0062C540, &FleeOnPathOnBikeTrack_End);
AIPATHTRK_REVERSED("FleeOnRoadOnBikeTrack::Update", 0x0062C550, &FleeOnRoadOnBikeTrack_Update);
AIPATHTRK_REVERSED("MoveObjectiveTrack::Update", 0x0062C560, &MoveObjectiveTrack_Update);
AIPATHTRK_REVERSED("CheerFightTrack::Start", 0x0062DEB0, &CheerFightTrack_Start);
AIPATHTRK_REVERSED("CheerFightTrack::Step", 0x0062DF40, &CheerFightTrack_StepImpl);
AIPATHTRK_REVERSED("PathTrack::NotifyNode", 0x0062E0C0, &PathTrack_NotifyNodeImpl);
AIPATHTRK_REVERSED("FollowPathTrack::NotifyActionsDone", 0x0062E1A0, &FollowPathTrack_NotifyActionsDoneImpl);
AIPATHTRK_REVERSED("CheerFightTrack::Update", 0x0062E5A0, &CheerFightTrack_Update);
AIPATHTRK_REVERSED("PathTrack::Advance", 0x0062E750, &PathTrack_AdvanceImpl);
AIPATHTRK_REVERSED("PathTrack::CatchUp", 0x0062E7A0, &PathTrack_CatchUpImpl);
AIPATHTRK_REVERSED("FollowPathTrack::NextAction", 0x0062E8E0, &FollowPathTrack_NextActionImpl);
AIPATHTRK_REVERSED("FleeOnPathOnBikeTrack::Update", 0x0062E9F0, &FleeOnPathOnBikeTrack_Update);
AIPATHTRK_REVERSED("PathTrack::CheckPassed", 0x0062EC00, &PathTrack_CheckPassedImpl);
AIPATHTRK_REVERSED("FollowPathTrack::Update", 0x0062ECD0, &FollowPathTrack_Update);
AIPATHTRK_REVERSED("FollowPathTrack::Move", 0x0062EF80, &FollowPathTrack_Move);
