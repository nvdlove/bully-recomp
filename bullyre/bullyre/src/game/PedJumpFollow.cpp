// Followers jumping after their leader (0x488CB0 .. 0x489290): the jump objective (type
// 0x1A, 0x34 bytes), the line tests it uses, and the ally's jump watcher (PedAlly +0x30,
// CPed +0x320) that sees the leader take off and land and queues the same jump.
//
// The watcher: +0 the ped, +4 the leader watched (registered), +8 a jump seen, +9 the
// leader was in the air, +0xC where it took off, +0x18 where it landed, +0x24 its heading
// then, +0x28 its intelligence's +0x1C then.
// The jump objective: the objective's (0x10), +0x10 where to take off, +0x1C the heading,
// +0x20 a speed, +0x24 a flag (1), +0x28 where to land.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

struct V3 {
    float x, y, z;
};

using re::At;

V3* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<V3*>(matrix + 0x30) : &At<V3>(entity, 4);
}

inline re::Ptr32<void>* const PJF_vtJumpObjective = reinterpret_cast<re::Ptr32<void>*>(0x0090F9E4);
inline void* const PJF_isJumpObjective = reinterpret_cast<void*>(0x004890E0);   // (a predicate for the queue)
GAME_VAR(float, PJF_lineTestSetting, 0x00AEB940);

GAME_FN(void*, __thiscall, PJF_ObjectiveConstruct, 0x0046FA90, void* self, int type, void* ped);
GAME_FN(void, __thiscall, PJF_ObjectiveDestruct, 0x0046FAF0, void* self);
GAME_FN(void, __cdecl, PJF_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, PJF_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(V3*, __cdecl, PJF_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
GAME_FN(uint8_t, __cdecl, PJF_TestLine, 0x005EB1B0, const V3* from, const V3* to, uint8_t* blocked, int, int);
GAME_FN(float, __cdecl, PJF_InRange41A4D0, 0x0041A4D0, const V3* a, const V3* b, float range);
GAME_FN(float, __cdecl, PJF_DistanceToLine, 0x00569330, const V3* a, const V3* b, const V3* point);
GAME_FN(void, __thiscall, PJF_RefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(uint8_t, __thiscall, PJF_PedInAir, 0x00476D70, void* ped);
GAME_FN(float, __cdecl, PJF_DistanceSquaredLess, 0x00487E50, const V3* a, const V3* b, float r);
GAME_FN(void, __thiscall, PJF_QueueRemoveIf, 0x00470400, void* queue, void* predicate);
GAME_FN(void, __thiscall, PJF_QueueRemoveIf470470, 0x00470470, void* queue, void* predicate);
GAME_FN(uint8_t, __thiscall, PJF_QueueAdd, 0x00471390, void* queue, void* objective);
// (this file's, through the game's address)
GAME_FN(uint8_t*, __thiscall, PJF_JumpConstruct, 0x00488FC0, void* self, void* ped, const V3* from, float heading, float speed,
        const V3* to);
GAME_FN(uint8_t, __cdecl, PJF_CanJump, 0x00489040, const V3* from, const V3* to);
GAME_FN(void*, __thiscall, PJF_FindJump, 0x00489250, void* queue);

} // namespace

namespace c07pjf {

// 0x488CB0: a ped's block of 0x90 flags (+0x20, +0x28, ... set, the rest clear)
void __fastcall PedFlagsReset(uint32_t* self, re::Edx) {
    static const uint16_t set[] = {0x20, 0x28, 0x54, 0xA0, 0xA4, 0xAC, 0xFC, 0x13C, 0x144, 0x188, 0x1A8, 0x1BC, 0x1D4, 0x214};
    for (int i = 0; i < 0x90; i++) {
        self[i] = 0;
    }
    for (uint16_t offset : set) {
        self[offset / 4] = 1;
    }
}

// 0x488FC0: the jump objective
uint8_t* __fastcall JumpConstruct(uint8_t* self, re::Edx, void* ped, const V3* from, float heading, float speed, const V3* to) {
    PJF_ObjectiveConstruct(self, 0x1A, ped);
    At<re::Ptr32<void>*>(self, 0) = PJF_vtJumpObjective;
    At<V3>(self, 0x10) = {0.0f, 0.0f, 0.0f};
    At<V3>(self, 0x28) = {0.0f, 0.0f, 0.0f};
    At<V3>(self, 0x10) = *from;
    At<float>(self, 0x1C) = heading;
    At<float>(self, 0x20) = speed;
    At<uint8_t>(self, 0x24) = 1;
    At<V3>(self, 0x28) = *to;
    return self;
}

// 0x489040 (cdecl): the jump from `from` to `to` can be made: not more than 0.5 up, and
// the line clear (with 0xAEB940 zero meanwhile)
uint8_t __cdecl CanJump(const V3* from, const V3* to) {
    V3 d;
    PJF_VectorSub(&d, to, from);
    if (0.5f < d.z || std::isnan(d.z)) {
        return 0;
    }
    float saved = PJF_lineTestSetting;
    PJF_lineTestSetting = 0.0f;
    uint8_t blocked = 0;
    if (PJF_TestLine(from, to, &blocked, 0, 3) && !blocked) {
        PJF_lineTestSetting = saved;
        return 1;
    }
    PJF_lineTestSetting = saved;
    return 0;
}

// 0x489100 (cdecl): `b` reachable from `a`: level and within 0.5, or not more than 0.5 up
// from `via` and the line clear
uint8_t __cdecl CanReach(const V3* a, const V3* via, const V3* b) {
    float dz = std::fabs(a->z - b->z);
    if (0.5f > dz && PJF_InRange41A4D0(a, b, 0.5f) < 0.0f) {
        return 1;
    }
    V3 d;
    PJF_VectorSub(&d, b, via);
    if (0.5f < d.z || std::isnan(d.z)) {
        return 0;
    }
    uint8_t blocked = 0;
    if (PJF_TestLine(a, b, &blocked, 0, 0) && !blocked) {
        return 1;
    }
    return 0;
}

// 0x4891B0 (vf0): the jump objective's deleting destructor
uint8_t* __fastcall JumpDeletingDestructor(uint8_t* self, re::Edx, int flags) {
    PJF_ObjectiveDestruct(self);
    if (flags & 1) {
        PJF_Free(self);
    }
    return self;
}

// 0x4891D0 (cdecl): a ped around `ped` stands within 0.5 of the line a..b
uint8_t __cdecl IsLineBlockedByPed(uint8_t* ped, const V3* a, const V3* b) {
    for (int i = 0; i < At<int32_t>(ped, 0x3C0); i++) {
        uint8_t* other = At<uint8_t*>(ped, 0x3C4 + i * 4);
        if (other && other != ped && PJF_DistanceToLine(a, b, EntityPos(other)) < 0.5f) {
            return 1;
        }
    }
    return 0;
}

// 0x489250 (the objective queue): its jump objective, queued or current
void* __fastcall FindJump(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x58); i++) {
        uint8_t* objective = At<uint8_t*>(self, 8 + i * 4);
        if (objective && At<int32_t>(objective, 8) == 0x1A) {
            return objective;
        }
    }
    uint8_t* current = At<uint8_t*>(self, 0x5C);
    if (current && At<int32_t>(current, 8) == 0x1A) {
        return current;
    }
    return nullptr;
}

// 0x489290: the watcher's step for the leader: a jump the leader made (in the air, then
// landed 2 away) is queued for this ped unless it can just walk there; a queued jump is
// dropped once it can
void __fastcall WatchLeader(uint8_t* self, re::Edx, void* leader) {
    if (At<void*>(self, 4) != leader) {
        PJF_RefSet(self + 4, leader);
        At<uint8_t>(self, 8) = 0;
        At<uint8_t>(self, 9) = 0;
    }
    uint8_t* watched = At<uint8_t*>(self, 4);
    if (!watched) {
        return;
    }
    V3* pos = EntityPos(watched);
    uint8_t wasInAir = At<uint8_t>(self, 9);
    uint8_t inAir = PJF_PedInAir(watched);
    At<uint8_t>(self, 9) = inAir;
    if (!wasInAir) {
        if (inAir) {
            uint8_t* leaderPed = At<uint8_t*>(self, 4);
            At<uint8_t>(self, 8) = 1;
            At<V3>(self, 0xC) = *pos;
            float heading;
            if (At<uint8_t*>(leaderPed, 0x14)) {
                uint8_t* matrix = At<uint8_t*>(leaderPed, 0x14);
                heading = static_cast<float>(std::atan2(-At<float>(matrix, 0x10), At<float>(matrix, 0x14)));
            } else {
                heading = At<float>(leaderPed, 0x10);
            }
            At<float>(self, 0x24) = heading;
            At<float>(self, 0x28) = At<float>(At<uint8_t*>(leaderPed, 0x2E0), 0x1C);
        }
    } else if (!inAir) {
        At<V3>(self, 0x18) = *pos;
    }
    if (At<uint8_t>(self, 8)) {
        if (inAir) {
            return;
        }
        if (PJF_DistanceSquaredLess(&At<V3>(self, 0x18), pos, 2.0f) < 0.0f) {
            return;
        }
        if (At<uint8_t>(self, 8) && !PJF_CanJump(&At<V3>(self, 0xC), pos)) {
            PJF_QueueRemoveIf(At<uint8_t*>(self, 0) + 0x5F0, PJF_isJumpObjective);
            void* memory = PJF_Malloc(0x34);
            void* jump = memory ? PJF_JumpConstruct(memory, At<void*>(self, 0), &At<V3>(self, 0xC), At<float>(self, 0x24),
                                                    At<float>(self, 0x28), pos)
                                : nullptr;
            PJF_QueueAdd(At<uint8_t*>(self, 0) + 0x5F0, jump);
        }
    }
    At<uint8_t>(self, 8) = 0;
    if (PJF_PedInAir(At<void*>(self, 0)) || PJF_PedInAir(At<void*>(self, 4))) {
        return;
    }
    uint8_t* ped = At<uint8_t*>(self, 0);
    if (PJF_FindJump(ped + 0x5F0) && PJF_CanJump(EntityPos(ped), pos)) {
        PJF_QueueRemoveIf470470(At<uint8_t*>(self, 0) + 0x5F0, PJF_isJumpObjective);
    }
}

} // namespace c07pjf

#define PJF_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PJF_REVERSED("CPed::FlagsReset488CB0", 0x00488CB0, &c07pjf::PedFlagsReset);
PJF_REVERSED("JumpObjective::Construct", 0x00488FC0, &c07pjf::JumpConstruct);
PJF_REVERSED("JumpObjective::CanJump", 0x00489040, &c07pjf::CanJump);
PJF_REVERSED("JumpObjective::CanReach", 0x00489100, &c07pjf::CanReach);
PJF_REVERSED("JumpObjective::DeletingDestructor", 0x004891B0, &c07pjf::JumpDeletingDestructor);
PJF_REVERSED("JumpObjective::IsLineBlockedByPed", 0x004891D0, &c07pjf::IsLineBlockedByPed);
PJF_REVERSED("ObjectiveQueue::FindJump", 0x00489250, &c07pjf::FindJump);
PJF_REVERSED("AllyJumpWatcher::WatchLeader", 0x00489290, &c07pjf::WatchLeader);
