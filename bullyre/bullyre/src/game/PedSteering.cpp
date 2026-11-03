// A ped's steering helper (0x14 bytes: the ped, then four registered references):
// sending the ped somewhere through its path follower (the ped's +0xDAC), giving way to
// the peds around it, and a few distance tests. The ped's move state is at its +0x2E0
// (+0x0 the direction, +0xC the facing, +0x1C the speed).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct Vec {
    float x, y, z;
};

struct CPedSteering {
    re::Ptr32<uint8_t> ped;       // +0x0
    re::Ptr32<uint8_t> refs[4];   // +0x4 (registered references; +0x8 the entity to keep near)
};
static_assert(sizeof(CPedSteering) == 0x14);

GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(void*, g_targeting, 0x00C67534);            // (vf4: the player is aiming)
inline void* const g_pathBlockers = reinterpret_cast<void*>(0x00C2AB70);

GAME_FN(Vec*, __cdecl, VecFromHeading, 0x00432DF0, Vec* out, float heading);
GAME_FN(Vec*, __cdecl, VectorSub, 0x00414B70, Vec* out, const Vec* a, const Vec* b);
GAME_FN(Vec*, __cdecl, VectorAdd, 0x004129A0, Vec* out, const Vec* a, const Vec* b);
GAME_FN(Vec*, __cdecl, VectorScale, 0x00412350, Vec* out, const Vec* v, float s);
GAME_FN(Vec*, __cdecl, Normalised, 0x00472D40, Vec* out, const Vec* v);
GAME_FN(Vec*, __cdecl, Flat2DScaled, 0x0041A520, Vec* out, const Vec* v, float scale);   // (z 0, normalised, scaled)
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const Vec* a, const Vec* b);
GAME_FN(float, __cdecl, Max, 0x00414490, float a, float b);
GAME_FN(float, __thiscall, Vec_Normalise, 0x00414E10, Vec* v);
GAME_FN(Vec*, __thiscall, Entity_GetForward, 0x004154A0, void* entity, Vec* out);
GAME_FN(float, __cdecl, Distance2D, 0x00470110, const Vec* a, const Vec* b);
GAME_FN(float, __cdecl, Fn41A4D0, 0x0041A4D0, const Vec* a, const Vec* b, float);
GAME_FN(float, __cdecl, Fn487E50, 0x00487E50, const Vec* a, const Vec* b, float radius);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(uint8_t, __thiscall, Tether_Clamp, 0x00499970, void* tether, Vec* pos, float margin);
GAME_FN(uint8_t, __thiscall, Social_48FB40, 0x0048FB40, void* social);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, Ally_GetTop, 0x00485560, void* ally);
GAME_FN(uint8_t, __thiscall, Ally_IsFollowedBy, 0x00485500, void* ally, void* ped);
GAME_FN(uint8_t, __thiscall, Fn448300, 0x00448300, void* object, void* ped);
GAME_FN(uint8_t*, __thiscall, PedTasks_GetActive, 0x00470340, void* tasks);
GAME_FN(uint8_t, __thiscall, Ped_IsGangType, 0x0041AA50, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, void* ped);
GAME_FN(int32_t*, __thiscall, ModelInfo_522380, 0x00522380, void* modelInfo, int32_t* out);
GAME_FN(uint8_t, __thiscall, PathBlockers_Blocks, 0x0046EFB0, void* blockers, void* ped, const Vec* pos, int);
GAME_FN(void, __thiscall, Path_SetTarget, 0x005E2A00, void* path, const Vec* pos, uintptr_t);
GAME_FN(void, __thiscall, Path_SetTargetWith, 0x005E2820, void* path, const Vec* pos, float a, float b);
GAME_FN(uint8_t, __thiscall, Path_Start, 0x005E4DC0, void* path);
// (its own, called where modes apply)
GAME_FN(uint8_t, __thiscall, PS_GiveWay, 0x004A4BA0, CPedSteering* self, uint32_t flags);
GAME_FN(uint8_t, __thiscall, PS_GoTo, 0x004A5000, CPedSteering* self, float x, float y, float z, uint32_t flags, float speed,
        float radius, void* entity, int flag);
GAME_FN(uint8_t, __thiscall, PS_GoToPed, 0x004A5150, CPedSteering* self, uint8_t* ped, uint32_t flags, float speed, float radius);

const Vec* EntityPos(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(entity + 4);
}
float& MoveSpeed(uint8_t* ped) {
    return At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C);
}

} // namespace

namespace c09pedsteering {

// 0x4A4710: stopped (its move speed 0)
void __fastcall Stop(CPedSteering* self, re::Edx) {
    MoveSpeed(self->ped) = 0.0f;
}

// 0x4A4720: the direction of a heading (`this` unused)
Vec* __fastcall DirectionOf(void*, re::Edx, Vec* out, float heading) {
    VecFromHeading(out, heading);
    return out;
}

// 0x4A4740: the destructor's body (the references let go, not cleared)
void __fastcall Destruct(CPedSteering* self, re::Edx) {
    self->ped = nullptr;
    for (int i = 3; i >= 0; i--) {
        if (self->refs[i]) {
            CleanUpOldReference(self->refs[i], &self->refs[i]);
        }
    }
}

// 0x4A47A0: the ped (2D) within the radius of the point; the radius grows by as much
// as the point is within half a metre of the entity at +0x8
uint32_t __fastcall IsWithin(CPedSteering* self, re::Edx, float x, float y, float z, float radius) {
    Vec point = {x, y, z};
    Vec d;
    if (self->refs[1]) {
        VectorSub(&d, &point, EntityPos(self->refs[1]));
        float dist = float(std::sqrt(std::fabs(float(double(d.y) * d.y + double(d.x) * d.x))));
        if (dist < 0.5f) {
            radius = float((0.5 - dist) + radius);
        }
    }
    VectorSub(&d, &point, EntityPos(self->ped));
    float dist = float(std::sqrt(std::fabs(float(double(d.y) * d.y + double(d.x) * d.x))));
    return radius > dist ? 1 : 0;
}

// 0x4A48A0: from the ped to the point
Vec* __fastcall ToPoint(CPedSteering* self, re::Edx, Vec* out, const Vec* point) {
    VectorSub(out, point, EntityPos(self->ped));
    return out;
}

// 0x4A48D0: the ped's +0xDD4 is 1
uint8_t __fastcall IsDD4One(CPedSteering* self, re::Edx) {
    return At<int32_t>(self->ped, 0xDD4) == 1;
}

// 0x4A48F0: the constructor's body
void __fastcall Construct(CPedSteering* self, re::Edx, uint8_t* ped) {
    self->ped = ped;
    self->refs[0] = nullptr;
    self->refs[1] = nullptr;
    self->refs[2] = nullptr;
    self->refs[3] = nullptr;
}

// 0x4A4910: sent to a point with the path follower's two extra values (no entity to
// follow); stopped when the path's blocked or can't start. (`unused` isn't read)
uint8_t __fastcall GoToWith(CPedSteering* self, re::Edx, float x, float y, float z, int unused, uint32_t flags, float speed,
                            float radius, float a, float b) {
    (void)unused;
    Vec pos = {x, y, z};
    if (PathBlockers_Blocks(g_pathBlockers, self->ped, &pos, 0)) {
        MoveSpeed(self->ped) = 0.0f;
        return 0;
    }
    uint8_t* path = self->ped + 0xDAC;
    Path_SetTargetWith(path, &pos, a, b);
    At<float>(path, 0x534) = speed;
    re::Ptr32<uint8_t>* entity = &At<uint8_t*>(path, 0x43C);
    At<uint8_t>(path, 0x553) = 0;
    At<float>(path, 0x528) = radius;
    if (*entity) {
        CleanUpOldReference(*entity, entity);
        *entity = nullptr;
    }
    At<uint8_t>(path, 0x550) = (flags >> 15) & 1;
    At<uint8_t>(path, 0x53D) = (flags >> 11) & 1;
    At<uint8_t>(path, 0x53F) = ~(flags >> 7) & 1;
    At<uint8_t>(path, 0x540) = 0;
    At<uint8_t>(path, 0x541) = (flags >> 18) & 1;
    At<uint8_t>(path, 0x552) = (flags & 0x20000) ? 3 : 2;
    if (Path_Start(path)) {
        return 1;
    }
    MoveSpeed(self->ped) = 0.0f;
    return 0;
}

// 0x4A4A10 (the ped in esi): how much right of way a ped has
int32_t __cdecl PedPriority(uint8_t* ped) {
    uint8_t* task = PedTasks_GetActive(ped + 0x5F0);
    if (task && At<int32_t>(task, 8) == 0xC) {
        return 0x78;
    }
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        return 0x14;
    }
    if (At<int32_t>(ped, 0x1378) == 0 && (At<int32_t>(ped, 0xBB0) == 0 || At<int32_t>(ped, 0xBB4) == 0)) {
        if (Ped_IsGangType(ped)) {
            return 10;
        }
        int32_t kind;   // (left as ModelInfo_522380 writes it)
        int32_t* result = ModelInfo_522380(Ped_GetModelInfo(ped), &kind);
        return *result == 4 ? 5 : 0;
    }
    return 0xF;
}

// 0x4A4AA0 (the other ped in ebx, the ped in edi): the ped gives way to the other
uint8_t __cdecl GivesWayTo(uint8_t* other, uint8_t* ped) {
    if (!other) {
        return 0;
    }
    if (At<int32_t>(other, 0x1310) == 0xD) {
        void* targeting = g_targeting;
        using Fn = uint8_t(__thiscall*)(void*);
        if (re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(targeting))[4])(targeting) && Ped_GetTarget(other) == ped) {
            return 0;
        }
    }
    if (At<int32_t>(ped, 0xBB0) != 0 && At<uint8_t*>(ped, 0xBB4) == other && (At<uint8_t>(ped, 0xD68) & 4)) {
        return 0;
    }
    if (other == Ally_GetTop(ped + 0x2F0)) {
        uint8_t* target = Ped_GetTarget(other);
        if (target && (At<uint8_t>(target, 0x108) & 7) != 3) {
            target = nullptr;
        }
        if (target != ped && At<int32_t>(ped, 0x360) != 0) {
            return 1;
        }
        return 0;
    }
    if (Ally_IsFollowedBy(ped + 0x2F0, other)) {
        return 0;
    }
    if (At<void*>(ped, 0x1D20) && Fn448300(At<void*>(ped, 0x1D20), other)) {
        return 0;
    }
    uint8_t* task = PedTasks_GetActive(ped + 0x5F0);
    if (task && At<int32_t>(task, 8) == 7) {
        return 0;
    }
    int32_t mine = PedPriority(ped);
    int32_t theirs = PedPriority(other);
    if (mine < theirs) {
        return 1;
    }
    if (mine > theirs) {
        return 0;
    }
    return At<uint16_t>(ped, 0x10C) < At<uint16_t>(other, 0x10C);
}

// 0x4A4BA0: a step aside from the first moving ped close by (within 10 m, both tests
// passed) the ped gives way to, a quarter metre (kept within its tether); the ped's
// direction (and with flag 0x20, when walking the same way, its facing eased) set, its
// speed the step's. 0: nobody to give way to
uint8_t __fastcall GiveWay(CPedSteering* self, re::Edx, uint32_t flags) {
    uint8_t* ped = self->ped;
    At<int32_t>(ped, 0x146C) = 0;
    if (At<int32_t>(ped, 0x1584) > 0) {
        return 0;
    }
    if (Social_48FB40(ped + 0xB44)) {
        return 0;
    }
    uint8_t* list = self->ped + 0x3BC;
    for (int i = 0; i < At<int32_t>(list, 4); i++) {
        uint8_t* other = At<uint8_t*>(list, 8 + i * 4);
        if (!other) {
            continue;
        }
        Vec d;
        VectorSub(&d, EntityPos(self->ped), EntityPos(other));
        float distSq = float((double(d.x) * d.x + double(d.y) * d.y) + double(d.z) * d.z);
        if (!(float(double(distSq) - 100.0) <= 0.0f)) {
            continue;
        }
        if (!(Fn41A4D0(EntityPos(self->ped), EntityPos(other), 1.0f) <= 0.0f)) {
            continue;
        }
        if (!(Fn487E50(EntityPos(self->ped), EntityPos(other), 3.0f) <= 0.0f)) {
            continue;
        }
        float speed = MoveSpeed(other);
        if (double(speed) > double(0.44f)) {
            speed = 0.44f;
        } else if (speed < 0.01f) {
            continue;
        }
        if (!GivesWayTo(other, self->ped)) {
            continue;
        }
        Vec away, pos, offset, target, dir;
        Flat2DScaled(&away, VectorSub(&d, EntityPos(self->ped), EntityPos(other)), 1.0f);
        pos = *EntityPos(self->ped);
        offset = {float(double(away.x) * 0.25), float(double(away.y) * 0.25), float(double(away.z) * 0.25)};
        VectorAdd(&target, &pos, &offset);
        Tether_Clamp(self->ped + 0x358, &target, -0.3f);
        float moved = Distance2D(&pos, &target);
        if (!(0.01f <= moved)) {
            continue;
        }
        Flat2DScaled(&dir, VectorSub(&d, &target, &pos), 1.0f);
        uint32_t same = flags & 0x20;
        uint8_t* move = At<uint8_t*>(self->ped, 0x2E0);
        if (same && DotProduct(&away, At<Vec*>(other, 0x2E0)) > 0.707f) {
            float t = float(double(timeStep) * double(0.07f));
            *reinterpret_cast<Vec*>(move) = dir;
            uint8_t* me = self->ped;
            uint8_t* myMove = At<uint8_t*>(me, 0x2E0);
            Vec a, b, forward, sum, facing;
            Vec* pa = VectorScale(&a, &away, t);
            float rest = float(1.0 - t);
            Vec* pb = VectorScale(&b, Entity_GetForward(me, &forward), rest);
            Vec* ps = VectorAdd(&sum, pb, pa);
            facing = *Normalised(&facing, ps);
            *reinterpret_cast<Vec*>(myMove + 0xC) = facing;
            At<int32_t>(self->ped, 0x146C) = 1;
        } else {
            *re::PtrCast<Vec*>(At<uint8_t*>(self->ped, 0x2E0)) = dir;
            *reinterpret_cast<Vec*>(At<uint8_t*>(self->ped, 0x2E0) + 0xC) = dir;
        }
        if (same) {
            uint8_t* myMove = At<uint8_t*>(self->ped, 0x2E0);
            float m = Max(speed, moved);
            At<float>(myMove, 0x1C) = Max(At<float>(myMove, 0x1C), m);
            return 1;
        }
        MoveSpeed(self->ped) = moved;
        return 1;
    }
    return 0;
}

// 0x4A5000: sent to a point (giving way first unless flags 0x4008/0x8000), following
// the entity; the path follower set from the flags and started (stopped when it can't
// or the way's blocked)
uint8_t __fastcall GoTo(CPedSteering* self, re::Edx, float x, float y, float z, uint32_t flags, float speed, float radius,
                        uint8_t* entity, int flag) {
    if (!(flags & 0xC008) && PS_GiveWay(self, flags)) {
        return 1;
    }
    uint8_t* ped = self->ped;
    Vec pos = {x, y, z};
    uint8_t* path = ped + 0xDAC;
    if (PathBlockers_Blocks(g_pathBlockers, ped, &pos, 0)) {
        MoveSpeed(self->ped) = 0.0f;
        return 0;
    }
    At<float>(path, 0x528) = radius;
    At<uint8_t>(path, 0x550) = (flags >> 15) & 1;
    At<uint8_t>(path, 0x53D) = (flags >> 11) & 1;
    At<uint8_t>(path, 0x53F) = ~(flags >> 7) & 1;
    At<uint8_t>(path, 0x541) = (flags >> 18) & 1;
    At<uint8_t>(path, 0x540) = ~(flags >> 16) & 1;
    At<uint8_t>(path, 0x553) = uint8_t(flag);
    re::Ptr32<uint8_t>* follow = &At<uint8_t*>(path, 0x43C);
    if (entity != *follow) {
        if (*follow) {
            CleanUpOldReference(*follow, follow);
        }
        *follow = entity;
        if (entity) {
            RegisterReference(entity, follow);
        }
    }
    Path_SetTarget(path, &pos, 0);
    At<float>(path, 0x534) = speed;
    uint8_t mode = (flags & 0x20000) ? 3 : 2;
    if (uint8_t(flag) != 0) {
        mode |= 4;
    }
    At<uint8_t>(path, 0x552) = mode;
    if (!Path_Start(path)) {
        MoveSpeed(self->ped) = 0.0f;
        return 0;
    }
    return 1;
}

// 0x4A5150: sent to another ped; when it's walking away ahead, matching its pace
// (flag 0x400; the speed from its speed and the gap past the radius, capped, the
// radius 0.7)
uint8_t __fastcall GoToPed(CPedSteering* self, re::Edx, uint8_t* other, uint32_t flags, float speed, float radius) {
    if (!other) {
        return 0;
    }
    float speedIn = speed;
    float otherSpeed = MoveSpeed(other);
    float gap = float(double(Distance2D(EntityPos(self->ped), EntityPos(other))) - radius);
    if (0.01f < otherSpeed || 0.01f == otherSpeed) {
        Vec dir = *At<Vec*>(other, 0x2E0);
        Vec_Normalise(&dir);
        Vec d;
        Vec to = *VectorSub(&d, EntityPos(other), EntityPos(self->ped));
        Vec_Normalise(&to);
        float dot = float((double(dir.y) * to.y + double(to.x) * dir.x) + double(to.z) * dir.z);
        if (dot > 0.0f) {
            flags |= 0x400;
            float want = float(dot * (double(otherSpeed) + double(0.2f) * gap));
            float cap = float(double(otherSpeed) + double(0.25f));
            cap = speedIn < cap ? cap : speedIn;
            if (0.01f > want) {
                speed = 0.01f;
            } else if (cap < want) {
                speed = cap;
            } else {
                speed = want;
            }
            radius = 0.7f;
        }
    }
    const Vec* pos = EntityPos(other);
    return PS_GoTo(self, pos->x, pos->y, pos->z, flags, speed, radius, other, 0);
}

// 0x4A5360: sent to an entity (a ped: GoToPed)
uint8_t __fastcall GoToEntity(CPedSteering* self, re::Edx, uint8_t* entity, uint32_t flags, float speed, float radius) {
    if (!entity) {
        return 0;
    }
    if ((At<uint8_t>(entity, 0x108) & 7) == 3) {
        return PS_GoToPed(self, entity, flags, speed, radius);
    }
    const Vec* pos = EntityPos(entity);
    return PS_GoTo(self, pos->x, pos->y, pos->z, flags, speed, radius, entity, 0);
}

} // namespace c09pedsteering

// (register arguments: written, not hooked)
RE_UNHOOKED("CPedSteering::PedPriority", 0x004A4A10);
RE_UNHOOKED("CPedSteering::GivesWayTo", 0x004A4AA0);

// ---- registration ---------------------------------------------------------------------

#define PEDSTEERING_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

PEDSTEERING_REVERSED("CPedSteering::Stop", 0x004A4710, &c09pedsteering::Stop);
PEDSTEERING_REVERSED("CPedSteering::DirectionOf", 0x004A4720, &c09pedsteering::DirectionOf);
PEDSTEERING_REVERSED("CPedSteering::Destruct", 0x004A4740, &c09pedsteering::Destruct);
PEDSTEERING_REVERSED("CPedSteering::IsWithin", 0x004A47A0, &c09pedsteering::IsWithin);
PEDSTEERING_REVERSED("CPedSteering::ToPoint", 0x004A48A0, &c09pedsteering::ToPoint);
PEDSTEERING_REVERSED("CPedSteering::IsDD4One", 0x004A48D0, &c09pedsteering::IsDD4One);
PEDSTEERING_REVERSED("CPedSteering::Construct", 0x004A48F0, &c09pedsteering::Construct);
PEDSTEERING_REVERSED("CPedSteering::GoToWith", 0x004A4910, &c09pedsteering::GoToWith);
PEDSTEERING_REVERSED("CPedSteering::GiveWay", 0x004A4BA0, &c09pedsteering::GiveWay);
PEDSTEERING_REVERSED("CPedSteering::GoTo", 0x004A5000, &c09pedsteering::GoTo);
PEDSTEERING_REVERSED("CPedSteering::GoToPed", 0x004A5150, &c09pedsteering::GoToPed);
PEDSTEERING_REVERSED("CPedSteering::GoToEntity", 0x004A5360, &c09pedsteering::GoToEntity);
