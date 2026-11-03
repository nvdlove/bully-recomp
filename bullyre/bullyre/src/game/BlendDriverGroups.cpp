// The animation blend drivers (BlendDriverBasic: a weight blended in and out over time;
// JointConstraint, FloorMotionDriver, ReachDriver built on it) and the driver groups a
// ped's constraint set keeps them in (8 refcounted drivers per group; the joint and IK
// groups blend their drivers' targets). 0x72E7A0..0x730800.
//
// BlendDriverBasic: +0 vtable, +4 refs, +8 group, +0xC enabled, +0x10, +0x14 blend-in
// time, +0x18 blend-out time, +0x1C time in the blend, +0x20 full weight, +0x24 weight,
// +0x28 state (0 off, 1 on, 2 blending in, 3 blending out).

#include <cstdint>

#include "game/ActionTree.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct BdgVec {
    float x, y, z;
};
struct BdgQuat {
    float x, y, z, w;
};
static_assert(sizeof(BdgQuat) == 0x10);

// a driver group: 8 drivers
struct BdgGroup {
    uint32_t vtable;        // +0x0
    re::Ptr32<uint8_t> model;         // +0x4
    re::Ptr32<uint8_t> drivers[8];    // +0x8
    uint8_t updated;        // +0x28
    uint8_t unknown29;      // +0x29
    uint8_t active;         // +0x2A
    uint8_t pad2B;
    BdgQuat targetRot;      // +0x2C
    BdgVec target;          // +0x3C
    BdgVec lastTarget;      // +0x48
    uint32_t lastTime;      // +0x54
    int32_t unknown58;      // +0x58
};
static_assert(offsetof(BdgGroup, target) == 0x3C);
static_assert(sizeof(BdgGroup) == 0x5C);

// a fixed block pool: `count` blocks of `size` bytes, a free list
struct BdgPool {
    int32_t unknown0;   // +0x0
    int32_t numFree;    // +0x4
    int32_t size;       // +0x8
    int32_t count;      // +0xC
    int32_t total;      // +0x10
    re::Ptr32<re::Ptr32<uint8_t>> free;     // +0x14
    re::Ptr32<uint8_t> blocks;    // +0x18
};
static_assert(sizeof(BdgPool) == 0x1C);

GAME_VAR(int32_t, bdgSwapApplyAll, 0x020C9044);
GAME_VAR(int32_t, bdgNoIk, 0x020C9018);
GAME_VAR(uint32_t, bdgTimeInMs, 0x00C1A9B4);
GAME_ARRAY(float, bdgNoTarget, 0x00BC7268);   // (FLT_MAX x3: no target yet)
GAME_ARRAY(uint32_t, bdgZero, 0x00BCC564);
GAME_VAR(BdgPool*, bdgMotionPool, 0x020C9028);
inline void* const bdgAnimManager = reinterpret_cast<void*>(0x020C4910);
constexpr uint32_t kVtBlendDriverBasic = 0x0094D22C;
constexpr uint32_t kVtJointConstraint = 0x0094D28C;
constexpr uint32_t kVtSimpleRefCounted = 0x00905B3C;
constexpr uint32_t kVtFloorMotionDriver = 0x00905B4C;
constexpr uint32_t kVtReachDriver = 0x00905BAC;
constexpr uint32_t kVtDriverGroup = 0x0094D2EC;

GAME_FN(void, __thiscall, Bdg_Update, 0x0072E8E0, uint8_t* driver, float dt);
GAME_FN(void, __thiscall, Bdg_ApplyBlend, 0x0072E9E0, uint8_t* constraint);
GAME_FN(void, __thiscall, Bdg_GroupAdd, 0x0072EB10, BdgGroup* group, uint8_t* driver);
GAME_FN(void, __thiscall, Bdg_GroupRemove, 0x0072EB70, BdgGroup* group, uint8_t* driver);
GAME_FN(void, __thiscall, Bdg_GroupClear, 0x0072EBC0, BdgGroup* group);
GAME_FN(void, __thiscall, Bdg_GroupReset, 0x0072EC10, BdgGroup* group);
GAME_FN(BdgPool*, __thiscall, Bdg_PoolConstruct, 0x0072F5A0, BdgPool* pool, int size, int count);
GAME_FN(uint8_t, __cdecl, Bdg_BlendDriverIsA, 0x0044A120, int id);
GAME_FN(uint8_t*, __cdecl, Bdg_Hierarchy, 0x005EE1A0, void* clump);
GAME_FN(uint8_t*, __cdecl, Bdg_Matrices, 0x006C7710, void* hierarchy);
GAME_FN(void, __thiscall, Bdg_AnimUpdate, 0x006C1910, void* manager, uint8_t* model, float dt, int a, uintptr_t b);
GAME_FN(void, __thiscall, Bdg_AnimDetach, 0x006C2300, void* manager, uint8_t* owner, uint8_t* model);
GAME_FN(int, __thiscall, Bdg_AnimRemove, 0x006C2C00, void* manager, uint8_t* model, int keep);
GAME_FN(void, __thiscall, Bdg_ModelTeardown, 0x006B5A80, uint8_t* model);
GAME_FN(void, __cdecl, Bdg_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Bdg_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __thiscall, Bdg_MatrixCopy, 0x00412720, uint8_t* matrix, const uint8_t* from);
GAME_FN(void, __thiscall, Bdg_MatrixUpdateRW, 0x00413A70, uint8_t* matrix);
GAME_FN(void, __thiscall, Bdg_MatrixFromNi, 0x00412A70, uint8_t* matrix, const void* transform);
GAME_FN(void, __thiscall, Bdg_QuatToNi, 0x00413E60, const BdgQuat* q, void* transform);
GAME_FN(void, __thiscall, Bdg_QuatSet, 0x00465620, BdgQuat* q, const void* transform);
GAME_FN(void, __thiscall, Bdg_QuatNormalise, 0x006B1EE0, BdgQuat* q);
GAME_FN(void, __thiscall, Bdg_QuatSlerp, 0x004143C0, BdgQuat* self, const BdgQuat* a, const BdgQuat* b, float t);
GAME_FN(uint8_t*, __thiscall, Bdg_SlotsGet, 0x006B1FA0, uint8_t* model, int slot);
GAME_FN(uint8_t*, __thiscall, Bdg_AgrGetEntry, 0x00450B80, void* group, uint32_t index);
GAME_FN(float, __thiscall, Bdg_Magnitude, 0x0040DFB0, const BdgVec* v);
GAME_FN(BdgVec*, __cdecl, Bdg_VectorSub, 0x00414B70, BdgVec* out, const BdgVec* a, const BdgVec* b);
GAME_FN(void, __thiscall, Bdg_MoveTargetSet, 0x00472D70, uint8_t* target, const BdgVec* pos, float time);
GAME_FN(void, __thiscall, Bdg_SolveIk, 0x006BA380, uint8_t* model, uint8_t* effector, float a);
GAME_FN(void, __thiscall, Bdg_UpdateAttachments, 0x006B93A0, uint8_t* model, float dt, int flag);

constexpr float kOneSixtieth = 0.0166666675f;

// the joint matrices of a constraint's clump
uint8_t* JointMatrices(uint8_t* self) {
    return Bdg_Matrices(Bdg_Hierarchy(At<void*>(self, 0x30)));
}
// a driver released from a group slot (deleted with its last reference)
void ReleaseSlot(re::Ptr32<uint8_t>& slot) {
    uint8_t* driver = slot;
    At<int32_t>(driver, 4) += -1;
    At<uint8_t*>(driver, 8) = nullptr;
    if (At<int32_t>(driver, 4) <= 0) {
        vc::Call<void>(driver, 0x4, 1);
    }
    slot = nullptr;
}

} // namespace

namespace bdg {

// 0x72E7A0: JointConstraint's class id
int16_t __cdecl JointConstraint_TypeId() {
    return acttree::TypeIdAt<0x020C9040, 0x020C903C>();
}

// 0x72E8A0 (vf20): the state set; the time in the blend from the weight
void __fastcall SetState(uint8_t* self, re::Edx, int state) {
    At<int32_t>(self, 0x28) = state;
    if (state == 2) {
        At<float>(self, 0x1C) = At<float>(self, 0x24) / At<float>(self, 0x20) * At<float>(self, 0x14);
        return;
    }
    if (state == 3) {
        At<float>(self, 0x1C) = (1.0f - At<float>(self, 0x24) / At<float>(self, 0x20)) * At<float>(self, 0x18);
        return;
    }
    At<float>(self, 0x1C) = 0.0f;
}

// 0x72E8E0 (vf3): the weight blended in or out (blends under a frame are instant)
void __fastcall Update(uint8_t* self, re::Edx, float dt) {
    int state = At<int32_t>(self, 0x28);
    if (state == 2) {
        float time = At<float>(self, 0x14);
        At<float>(self, 0x24) = kOneSixtieth <= time ? At<float>(self, 0x1C) / time * At<float>(self, 0x20) : At<float>(self, 0x20);
        float elapsed = dt + At<float>(self, 0x1C);
        At<float>(self, 0x1C) = elapsed;
        if (At<float>(self, 0x14) <= elapsed) {
            At<int32_t>(self, 0x28) = 1;
            At<float>(self, 0x1C) = At<float>(self, 0x14);
        }
    } else if (state == 3) {
        float time = At<float>(self, 0x18);
        float weight;
        if (kOneSixtieth <= time) {
            weight = (1.0f - At<float>(self, 0x1C) / time) * At<float>(self, 0x20);
        } else {
            weight = 0.0f;
            At<int32_t>(self, 0x28) = 0;
        }
        At<float>(self, 0x24) = weight;
        float elapsed = dt + At<float>(self, 0x1C);
        At<float>(self, 0x1C) = elapsed;
        if (At<float>(self, 0x18) <= elapsed) {
            At<int32_t>(self, 0x28) = 0;
            At<float>(self, 0x1C) = At<float>(self, 0x18);
        }
    } else if (state == 1) {
        At<float>(self, 0x24) = At<float>(self, 0x20);
    } else {
        At<float>(self, 0x24) = 0.0f;
    }
}

// 0x72E9B0 (JointConstraint vf8): its weight, 0 without a skeleton
float __fastcall JointConstraint_GetWeight(uint8_t* self, re::Edx) {
    uint8_t* hierarchy = Bdg_Hierarchy(At<void*>(self, 0x30));
    if (hierarchy != nullptr && At<uint32_t*>(hierarchy, 0x1C) != nullptr && *At<uint32_t*>(hierarchy, 0x1C) != 0) {
        return At<float>(self, 0x24);
    }
    return 0.0f;
}

// 0x72E9E0: the joint's translation blended from its bind pose by +0x78 (z only unless
// 0x20C9044) (the game's vf21 0x72F800 relies on ecx surviving this)
void __fastcall JointConstraint_ApplyBlend(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x7C) == 0) {
        return;
    }
    bool all = bdgSwapApplyAll != 0;
    uint8_t* m = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x2C), 0x14), 8);
    float k = At<float>(self, 0x78);
    At<float>(m, 0x60) = (At<float>(m, 0x60) - At<float>(m, 0x2C)) * k + At<float>(m, 0x2C);
    if (all) {
        At<float>(m, 0x58) = (At<float>(m, 0x58) - At<float>(m, 0x24)) * At<float>(self, 0x78) + At<float>(m, 0x24);
        At<float>(m, 0x5C) = (At<float>(m, 0x5C) - At<float>(m, 0x28)) * At<float>(self, 0x78) + At<float>(m, 0x28);
    }
}

// 0x72EA30 (vf6): the joint's position
BdgVec* __fastcall JointConstraint_GetPosition(uint8_t* self, re::Edx, BdgVec* out) {
    uint8_t* m = JointMatrices(self);
    *out = At<BdgVec>(m, 0x58);
    return out;
}

// 0x72EA60 (vf7): its matrix set
void __fastcall JointConstraint_SetMatrix(uint8_t* self, re::Edx, const uint8_t* matrix) {
    Bdg_MatrixCopy(self + 0x34, matrix);
    Bdg_MatrixUpdateRW(self + 0x34);
}

// 0x72EA80 (vf3): its model animated, then the weight
void __fastcall JointConstraint_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* model = At<uint8_t*>(self, 0x2C);
    // (the game tests the model after reading it)
    if (At<uint32_t>(model, 0x9C) == 0 && (At<uint8_t>(model, 0x98) & 2) == 0 && model != nullptr && At<uint32_t>(self, 0x30) != 0) {
        Bdg_AnimUpdate(bdgAnimManager, model, dt, 1, 0);
    }
    Bdg_Update(self, dt);
}

// 0x72EAD0 (FloorMotionDriver vf2): on at full weight
void __fastcall FloorMotionDriver_Start(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x28) = 1;
    At<float>(self, 0x20) = 1.0f;
    At<int32_t>(self, 0x10) = 0;
    At<float>(self, 0x24) = 1.0f;
}

// 0x72EAF0 (ReachDriver vf6): its target
BdgVec* __fastcall ReachDriver_GetTarget(uint8_t* self, re::Edx, BdgVec* out) {
    *out = At<BdgVec>(self, 0x34);
    return out;
}

// 0x72EB10: a driver added (once; a reference taken, vf22 told when the group exists,
// vf17 started)
void __fastcall Group_Add(BdgGroup* self, re::Edx, uint8_t* driver) {
    for (int i = 0; i < 8; i++) {
        if (self->drivers[i] == driver) {
            return;
        }
    }
    for (int i = 0; i < 8; i++) {
        if (self->drivers[i] == nullptr) {
            self->drivers[i] = driver;
            At<int32_t>(driver, 4) += 1;
            At<BdgGroup*>(driver, 8) = self;
            if (self != nullptr) {
                vc::Call<void>(driver, 0x58);
            }
            vc::Call<void>(driver, 0x44);
            return;
        }
    }
}

// 0x72EB70: a driver removed
void __fastcall Group_Remove(BdgGroup* self, re::Edx, uint8_t* driver) {
    if (driver == nullptr) {
        return;
    }
    for (int i = 0; i < 8; i++) {
        if (self->drivers[i] != nullptr && self->drivers[i] == driver) {
            ReleaseSlot(self->drivers[i]);
        }
    }
}

// 0x72EBC0: every driver removed
void __fastcall Group_Clear(BdgGroup* self, re::Edx) {
    for (int i = 0; i < 8; i++) {
        if (self->drivers[i] != nullptr) {
            ReleaseSlot(self->drivers[i]);
        }
    }
}

// 0x72EC10: reset (each driver's vf21(0))
void __fastcall Group_Reset(BdgGroup* self, re::Edx) {
    self->unknown29 = 0;
    self->updated = 0;
    self->unknown58 = 0;
    for (int i = 0; i < 8; i++) {
        if (self->drivers[i] != nullptr) {
            vc::Call<void>(self->drivers[i], 0x54, 0);
        }
    }
}

// 0x72EC50: the target's velocity since the last frame (zero before there's one, or
// for a step under 1e-5)
BdgVec* __fastcall Group_TargetVelocity(BdgGroup* self, re::Edx, BdgVec* out, float dt) {
    if (bdgNoTarget[0] != self->lastTarget.x || bdgNoTarget[1] != self->lastTarget.y || bdgNoTarget[2] != self->lastTarget.z) {
        float step = float(dt - 0.0);
        step = step < 0.0f ? -step : step;
        if (step > 1e-5f || step != step) {
            BdgVec d;
            Bdg_VectorSub(&d, &self->target, &self->lastTarget);
            float inv = 1.0f / dt;
            out->x = d.x * inv;
            out->y = d.y * inv;
            out->z = d.z * inv;
            return out;
        }
    }
    out->x = *reinterpret_cast<float*>(&bdgZero[0]);
    out->y = *reinterpret_cast<float*>(&bdgZero[1]);
    out->z = *reinterpret_cast<float*>(&bdgZero[2]);
    return out;
}

// 0x72ED40 (JointBlendDriverGroup vf1): its joint's transform (0x34 bytes)
uint32_t* __fastcall JointGroup_GetTransform(uint8_t* self, re::Edx, uint32_t* out) {
    const uint32_t* from = reinterpret_cast<const uint32_t*>(
        At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 4), 0x14), 8) + At<int32_t>(self, 0x60) * 0x34);
    for (int i = 0; i < 13; i++) {
        out[i] = from[i];
    }
    return out;
}

// 0x72ED70 (IKBlendDriverGroup vf1): its effector's joint's transform
uint32_t* __fastcall IkGroup_GetTransform(uint8_t* self, re::Edx, uint32_t* out) {
    int joint = At<int32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x5C), 0), 0x24) + 1;
    const uint32_t* from =
        reinterpret_cast<const uint32_t*>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 4), 0x14), 8) + joint * 0x34);
    for (int i = 0; i < 13; i++) {
        out[i] = from[i];
    }
    return out;
}

// 0x72EDA0: the constraint set's foot effectors from its ped
void __fastcall Constraints_SetEffectors(uint8_t* self, re::Edx) {
    uint8_t* ped = At<uint8_t*>(self, 0x1FC);
    if (ped == nullptr) {
        return;
    }
    uint8_t* model = At<uint8_t*>(self, 0x1F8);
    if (model == nullptr) {
        return;
    }
    At<uint8_t*>(self, 0x13C) = model;
    At<uint8_t*>(self, 0x19C) = model;
    At<uint32_t>(self, 0x194) = At<uint32_t>(ped, 0x16E8);
    At<uint32_t>(self, 0x1F4) = At<uint32_t>(ped, 0x16EC);
}

// 0x72EDE0: the five groups reset; the ped's IK flag from the groups' flags
void __fastcall Constraints_Reset(uint8_t* self, re::Edx) {
    Bdg_GroupReset(reinterpret_cast<BdgGroup*>(self));
    Bdg_GroupReset(reinterpret_cast<BdgGroup*>(self + 0x68));
    Bdg_GroupReset(reinterpret_cast<BdgGroup*>(self + 0xD0));
    Bdg_GroupReset(reinterpret_cast<BdgGroup*>(self + 0x138));
    Bdg_GroupReset(reinterpret_cast<BdgGroup*>(self + 0x198));
    At<uint8_t>(At<uint8_t*>(self, 0x1FC), 0x1624) = 0;
    if (At<uint8_t>(self, 0xFA) != 0) {
        At<uint8_t>(At<uint8_t*>(self, 0x1FC), 0x1624) = 1;
        return;
    }
    if (At<uint8_t>(self, 0x92) != 0) {
        At<uint8_t>(At<uint8_t*>(self, 0x1FC), 0x1624) = 1;
    }
}

// 0x72EE50: a driver added to its group by kind (-1: the first; 0, 0x16, 0x1E their
// own; any other the third)
void __fastcall Constraints_Add(uint8_t* self, re::Edx, uint8_t* driver, int kind) {
    int at;
    if (kind == -1) {
        at = 0;
    } else if (kind == 0) {
        at = 0x68;
    } else if (kind == 0x16) {
        at = 0x198;
    } else if (kind == 0x1E) {
        at = 0x138;
    } else {
        at = 0xD0;
    }
    Bdg_GroupAdd(reinterpret_cast<BdgGroup*>(self + at), driver);
}

// 0x72EEB0: a driver removed from the four kind groups
void __fastcall Constraints_Remove(uint8_t* self, re::Edx, uint8_t* driver) {
    Bdg_GroupRemove(reinterpret_cast<BdgGroup*>(self + 0x68), driver);
    Bdg_GroupRemove(reinterpret_cast<BdgGroup*>(self + 0xD0), driver);
    Bdg_GroupRemove(reinterpret_cast<BdgGroup*>(self + 0x138), driver);
    Bdg_GroupRemove(reinterpret_cast<BdgGroup*>(self + 0x198), driver);
}

// 0x72EEF0: the four kind groups cleared
void __fastcall Constraints_Clear(uint8_t* self, re::Edx) {
    Bdg_GroupClear(reinterpret_cast<BdgGroup*>(self + 0x68));
    Bdg_GroupClear(reinterpret_cast<BdgGroup*>(self + 0xD0));
    Bdg_GroupClear(reinterpret_cast<BdgGroup*>(self + 0x138));
    Bdg_GroupClear(reinterpret_cast<BdgGroup*>(self + 0x198));
}

// 0x72F4F0: +0x84 from +0x1C
void __fastcall Node_Latch(uint8_t* self, re::Edx) {
    At<float>(self, 0x84) = At<float>(self, 0x1C);
}

// 0x72F5A0 (thiscall): a fixed pool of `count` blocks of `size` (all free)
BdgPool* __fastcall Pool_Construct(BdgPool* self, re::Edx, int size, int count) {
    self->size = size;
    self->total = size * count;
    self->count = count;
    uint64_t bytes = uint64_t(uint32_t(count)) * 4;
    self->free = static_cast<re::Ptr32<uint8_t>*>(Bdg_Malloc((bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes)));
    self->blocks = static_cast<uint8_t*>(Bdg_Malloc(uint32_t(self->total)));
    self->unknown0 = 0;
    self->numFree = 0;
    for (int i = 0; i < self->count; i++) {
        self->free[self->numFree] = self->blocks + self->size * i;
        self->numFree += 1;
    }
    return self;
}

// 0x72F620 (thiscall): BlendDriverBasic's constructor (blends of 0.1 s, blending in)
uint8_t* __fastcall Construct(uint8_t* self, re::Edx) {
    At<float>(self, 0x14) = 0.1f;
    At<float>(self, 0x18) = 0.1f;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 0) = kVtBlendDriverBasic;
    At<float>(self, 0x1C) = 0.0f;
    At<uint32_t>(self, 0x10) = 0;
    At<int32_t>(self, 0x28) = 2;
    At<float>(self, 0x20) = 1.0f;
    At<float>(self, 0x24) = 0.0f;
    return self;
}

// 0x72F660 (vf0): IsA
uint8_t __fastcall IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == acttree::TypeIdAt<0x00C0F68C, 0x00C0F688>()) {
        return 1;
    }
    return Bdg_BlendDriverIsA(id);
}

// 0x72F6B0: JointConstraint's destructor (its model detached, removed and deleted)
void __fastcall JointConstraint_Destruct(uint8_t* self, re::Edx) {
    uint8_t* model = At<uint8_t*>(self, 0x2C);
    At<uint32_t>(self, 0) = kVtJointConstraint;
    if (model != nullptr) {
        uint8_t* owner = At<uint8_t*>(model, 0x9C);
        if (owner != nullptr && At<uint32_t>(owner, 0x94) != 0) {
            Bdg_AnimDetach(bdgAnimManager, owner, model);
        }
        Bdg_AnimRemove(bdgAnimManager, At<uint8_t*>(self, 0x2C), 0);
    }
    model = At<uint8_t*>(self, 0x2C);
    if (model != nullptr) {
        Bdg_ModelTeardown(model);
        Bdg_Free(model);
    }
    At<uint32_t>(self, 0) = kVtSimpleRefCounted;
}

// 0x72F710 (JointConstraint vf0): IsA
uint8_t __fastcall JointConstraint_IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == acttree::TypeIdAt<0x020C9040, 0x020C903C>()) {
        return 1;
    }
    if (int16_t(id) == acttree::TypeIdAt<0x00C0F68C, 0x00C0F688>()) {
        return 1;
    }
    return Bdg_BlendDriverIsA(id);
}

// 0x72F790 (vf22, added to a group): the blend from the model's travel over the anim's
// length
void __fastcall JointConstraint_Attached(uint8_t* self, re::Edx) {
    uint8_t* group = At<uint8_t*>(self, 8);
    if (group == nullptr) {
        return;
    }
    uint8_t* model = At<uint8_t*>(group, 4);
    if (At<uint8_t>(self, 0x7C) == 0 || model == nullptr) {
        return;
    }
    uint8_t* slot = Bdg_SlotsGet(model, 0);
    uint8_t* anim = At<uint8_t*>(slot, 0x18);
    if (anim == nullptr) {
        return;
    }
    uint8_t* data = At<uint8_t*>(model, 0x94);
    float length = At<float>(Bdg_AgrGetEntry(At<void*>(data, 0x10), At<uint32_t>(anim, 0x20)), 0x28);
    if (!(0.0f == length)) {
        At<float>(self, 0x78) = Bdg_Magnitude(reinterpret_cast<BdgVec*>(At<uint8_t*>(data, 0xA8) + 0xC)) / length;
    }
}

// 0x72F800 (vf21): enabled (the blend applied when switched on)
void __fastcall JointConstraint_SetEnabled(uint8_t* self, re::Edx, int enabled) {
    if (At<uint8_t>(self, 0xC) == 0 && re::Flag(enabled)) {
        Bdg_ApplyBlend(self);
    }
    At<uint8_t>(self, 0xC) = uint8_t(enabled);
}

// 0x72F830 (thiscall): FloorMotionDriver's constructor (its ped)
uint8_t* __fastcall FloorMotionDriver_Construct(uint8_t* self, re::Edx, uint8_t* ped) {
    At<float>(self, 0x14) = 0.1f;
    At<float>(self, 0x18) = 0.1f;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<float>(self, 0x1C) = 0.0f;
    At<int32_t>(self, 0x28) = 2;
    At<float>(self, 0x20) = 1.0f;
    At<uint32_t>(self, 0) = kVtFloorMotionDriver;
    At<uint8_t*>(self, 0x2C) = ped;
    At<float>(self, 0x24) = 0.0f;
    return self;
}

// 0x72F870 (vf5): its rotation
BdgQuat* __fastcall FloorMotionDriver_GetRotation(uint8_t* self, re::Edx, BdgQuat* out) {
    *out = At<BdgQuat>(self, 0x30);
    return out;
}

// 0x72F890 (vf6): its ped's position
BdgVec* __fastcall FloorMotionDriver_GetPosition(uint8_t* self, re::Edx, BdgVec* out) {
    uint8_t* ped = At<uint8_t*>(self, 0x2C);
    uint8_t* m = At<uint8_t*>(ped, 0x14);
    const BdgVec* pos = m != nullptr ? reinterpret_cast<BdgVec*>(m + 0x30) : reinterpret_cast<BdgVec*>(ped + 4);
    *out = *pos;
    return out;
}

// 0x72F8C0 (thiscall): ReachDriver's constructor (its ped; target 0)
uint8_t* __fastcall ReachDriver_Construct(uint8_t* self, re::Edx, uint8_t* ped) {
    At<float>(self, 0x14) = 0.1f;
    At<float>(self, 0x18) = 0.1f;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<float>(self, 0x1C) = 0.0f;
    At<int32_t>(self, 0x28) = 2;
    At<float>(self, 0x20) = 1.0f;
    At<uint32_t>(self, 0) = kVtReachDriver;
    At<uint8_t*>(self, 0x2C) = ped;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x34) = 0.0f;
    At<float>(self, 0x38) = 0.0f;
    At<float>(self, 0x3C) = 0.0f;
    return self;
}

// 0x72F910 (vf2): reset (target zero, rotation identity)
void __fastcall ReachDriver_Reset(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x10) = 0;
    At<uint32_t>(self, 0x34) = bdgZero[0];
    At<uint32_t>(self, 0x38) = bdgZero[1];
    At<uint32_t>(self, 0x3C) = bdgZero[2];
    At<BdgQuat>(self, 0x40) = BdgQuat{0.0f, 0.0f, 0.0f, 1.0f};
}

// 0x72F970 (vf5): its rotation
BdgQuat* __fastcall ReachDriver_GetRotation(uint8_t* self, re::Edx, BdgQuat* out) {
    *out = At<BdgQuat>(self, 0x40);
    return out;
}

// 0x72F990: the group updated once a frame (the last target kept); drivers whose vf19
// says they're done released
void __fastcall Group_Update(BdgGroup* self, re::Edx, float dt) {
    if (self->updated != 0) {
        return;
    }
    if (self->active != 0) {
        self->lastTarget = self->target;
        self->lastTime = bdgTimeInMs;
    } else {
        self->lastTarget.x = bdgNoTarget[0];
        self->lastTarget.y = bdgNoTarget[1];
        self->lastTarget.z = bdgNoTarget[2];
        self->lastTime = 0;
    }
    self->updated = 1;
    for (int i = 0; i < 8; i++) {
        if (self->drivers[i] != nullptr) {
            vc::Call<void>(self->drivers[i], 0xC, dt);
        }
    }
    for (int i = 0; i < 8; i++) {
        uint8_t* driver = self->drivers[i];
        if (driver != nullptr && vc::Call<int>(driver, 0x4C) == 0) {
            ReleaseSlot(self->drivers[i]);
        }
    }
    self->active = 0;
    for (int i = 0; i < 8; i++) {
        if (self->drivers[i] != nullptr) {
            self->active = 1;
            return;
        }
    }
}

// 0x72FA80: the drivers of a kind blended by weight: the first weighted one's rotation
// and position, the next slerped and added in; returns the largest weight
float __fastcall Group_Blend(BdgGroup* self, re::Edx, int kind, BdgVec* outPos, BdgQuat* outRot) {
    float maxWeight = 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < 8; i++) {
        uint8_t* driver = self->drivers[i];
        if (driver != nullptr && vc::Call<int>(driver, 0x2C) == kind) {
            float w = vc::Call<float>(driver, 0x20);
            sum = w + sum;
            if (maxWeight < w) {
                maxWeight = w;
            }
        }
    }
    float total = 0.0f;
    uint8_t found = 0;
    int i = 0;
    for (; i < 8 && !found; i++) {
        uint8_t* driver = self->drivers[i];
        if (driver == nullptr || vc::Call<int>(driver, 0x2C) != kind) {
            continue;
        }
        float w = vc::Call<float>(driver, 0x20);
        if (!(w > 0.0f)) {
            continue;
        }
        total = w / sum;
        BdgQuat q;
        *outRot = *vc::Call<BdgQuat*>(driver, 0x14, &q);
        BdgVec v;
        const BdgVec* p = vc::Call<BdgVec*>(driver, 0x18, &v);
        found = 1;
        float x = total * p->x;
        float y = p->y * total;
        float z = total * p->z;
        outPos->x = x;
        outPos->y = y;
        outPos->z = z;
    }
    if (!found) {
        return maxWeight;
    }
    for (; i < 8; i++) {
        uint8_t* driver = self->drivers[i];
        if (driver == nullptr || vc::Call<int>(driver, 0x2C) != kind) {
            continue;
        }
        float f = vc::Call<float>(driver, 0x20) / sum;
        total = f + total;
        BdgQuat q;
        vc::Call<BdgQuat*>(driver, 0x14, &q);
        float t = f / total;
        BdgQuat from = *outRot;
        Bdg_QuatSlerp(outRot, &from, &q, t);
        float lengthSq = outRot->x * outRot->x + outRot->y * outRot->y;
        lengthSq = lengthSq + outRot->z * outRot->z;
        lengthSq = lengthSq + outRot->w * outRot->w;
        float length = crt::Sqrtf(lengthSq);
        float inv = 1.0f / length;
        outRot->x = outRot->x * inv;
        outRot->y = outRot->y * inv;
        outRot->z = outRot->z * inv;
        outRot->w = inv * outRot->w;
        BdgVec v;
        const BdgVec* p = vc::Call<BdgVec*>(driver, 0x18, &v);
        float x = p->x * f;
        float y = p->y * f;
        float z = f * p->z;
        outPos->x = outPos->x + x;
        outPos->y = outPos->y + y;
        outPos->z = z + outPos->z;
    }
    return maxWeight;
}

// 0x72FD60 (IKBlendDriverGroup vf0): the IK effector on (its target and rotation set,
// solved) or off
void __fastcall IkGroup_Apply(uint8_t* self, re::Edx, int on) {
    uint8_t* effector = At<uint8_t*>(self, 0x5C);
    if (effector == nullptr) {
        return;
    }
    if (re::Flag(on) && bdgNoIk == 0) {
        At<uint32_t>(effector, 0x38) |= 1;
        At<float>(effector, 0x2C) = 0.0f;
        At<float>(effector, 0x28) = 0.0f;
        BdgVec target = At<BdgVec>(self, 0x3C);
        Bdg_MoveTargetSet(At<uint8_t*>(self, 0x5C), &target, 0.0f);
        BdgQuat q = At<BdgQuat>(self, 0x2C);
        uint8_t* e = At<uint8_t*>(self, 0x5C);
        // (stored w first)
        At<float>(e, 0x10) = q.w;
        At<float>(e, 0x14) = q.x;
        At<float>(e, 0x18) = q.y;
        At<float>(e, 0x1C) = q.z;
        Bdg_SolveIk(At<uint8_t*>(self, 4), At<uint8_t*>(self, 0x5C), 0.0f);
        Bdg_UpdateAttachments(At<uint8_t*>(self, 4), 0.0f, 1);
        return;
    }
    At<uint32_t>(effector, 0x38) &= ~1u;
    At<float>(effector, 0x2C) = 0.0f;
    At<float>(effector, 0x28) = 0.0f;
}

// 0x72FE60 (thiscall): the constraint set's destructor (the kind groups cleared, then
// every group as its base)
void __fastcall Constraints_Destruct(uint8_t* self, re::Edx) {
    Bdg_GroupClear(reinterpret_cast<BdgGroup*>(self + 0x68));
    Bdg_GroupClear(reinterpret_cast<BdgGroup*>(self + 0xD0));
    Bdg_GroupClear(reinterpret_cast<BdgGroup*>(self + 0x138));
    Bdg_GroupClear(reinterpret_cast<BdgGroup*>(self + 0x198));
    static const int kGroups[5] = {0x198, 0x138, 0xD0, 0x68, 0};
    for (int at : kGroups) {
        At<uint32_t>(self, at) = kVtDriverGroup;
        Bdg_GroupClear(reinterpret_cast<BdgGroup*>(self + at));
    }
}

// 0x72FEE0 (thiscall): the constraint set's model and ped
void __fastcall Constraints_Init(uint8_t* self, re::Edx, uint8_t* model, uint8_t* ped) {
    At<uint8_t*>(self, 0x1F8) = model;
    At<uint8_t*>(self, 0x1FC) = ped;
    At<uint8_t*>(self, 0x5C) = ped;
    At<uint8_t*>(self, 0xC4) = ped;
    At<uint8_t*>(self, 0x12C) = ped;
    At<uint8_t*>(self, 4) = model;
    At<uint8_t*>(self, 0x6C) = model;
    At<uint8_t*>(self, 0xD4) = model;
    At<uint32_t>(self, 0x60) = 0;
    At<uint32_t>(self, 0xC8) = 0;
    At<uint32_t>(self, 0x130) = 1;
    At<uint8_t>(self, 0x64) = 0;
    At<uint8_t>(self, 0xCC) = 1;
    At<uint8_t>(self, 0x134) = 1;
    if (ped != nullptr && model != nullptr) {
        At<uint8_t*>(self, 0x13C) = model;
        At<uint8_t*>(self, 0x19C) = model;
        At<uint32_t>(self, 0x194) = At<uint32_t>(ped, 0x16E8);
        At<uint32_t>(self, 0x1F4) = At<uint32_t>(ped, 0x16EC);
    }
}

// 0x72FF70: the motion pool made once (240 blocks of 0xA8)
void __cdecl MotionPool_Init() {
    if (bdgMotionPool != nullptr) {
        return;
    }
    BdgPool* pool = static_cast<BdgPool*>(Bdg_Malloc(sizeof(BdgPool)));
    bdgMotionPool = pool != nullptr ? Bdg_PoolConstruct(pool, 0xA8, 0xF0) : nullptr;
}

// 0x730730 (cdecl): a matrix from a rotation and a position (the matrix keeps a pointer
// to this function's stack transform: the game's)
uint8_t* __cdecl MatrixFromQuatPos(uint8_t* out, BdgQuat q, const BdgVec* pos) {
    struct {
        uint32_t before;   // (read into +0x3C: the game's uninitialised stack, zero here)
        float m[12];
        float scale;
    } t{};
    t.scale = 1.0f;
    At<void*>(out, 0x40) = t.m;
    Bdg_MatrixFromNi(out, t.m);
    Bdg_QuatToNi(&q, At<void*>(out, 0x40));
    Bdg_MatrixFromNi(out, At<void*>(out, 0x40));
    At<float>(out, 0x30) = pos->x;
    At<float>(out, 0x34) = pos->y;
    At<float>(out, 0x38) = pos->z;
    At<uint32_t>(out, 0x3C) = t.before;
    return out;
}

// 0x7307D0 (JointConstraint vf4): its matrix (attached to the joint's transform)
uint8_t* __fastcall JointConstraint_GetMatrix(uint8_t* self, re::Edx, uint8_t* out) {
    uint8_t* transform = JointMatrices(self) + 0x34;
    At<uint8_t*>(out, 0x40) = transform;
    Bdg_MatrixFromNi(out, transform);
    return out;
}

// 0x730800 (JointConstraint vf5): its rotation
BdgQuat* __fastcall JointConstraint_GetRotation(uint8_t* self, re::Edx, BdgQuat* out) {
    uint8_t* transform = JointMatrices(self) + 0x34;
    Bdg_QuatSet(out, transform);
    Bdg_QuatNormalise(out);
    return out;
}

} // namespace bdg

RE_UNHOOKED("AM_MotionTrack::vf4", 0x0072F590);   // (jmp 0x6B99F0)

#define BDG_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

BDG_REVERSED("JointConstraint::TypeId", 0x0072E7A0, &bdg::JointConstraint_TypeId);
BDG_REVERSED("BlendDriverBasic::SetState", 0x0072E8A0, &bdg::SetState);
BDG_REVERSED("BlendDriverBasic::Update", 0x0072E8E0, &bdg::Update);
BDG_REVERSED("JointConstraint::GetWeight", 0x0072E9B0, &bdg::JointConstraint_GetWeight);
// callers rely on ecx: the game's 0x72F800 (JointConstraint::SetEnabled) uses ecx after
// this: turn on together with 0x72F800
BDG_REVERSED("JointConstraint::ApplyBlend", 0x0072E9E0, &bdg::JointConstraint_ApplyBlend);
BDG_REVERSED("JointConstraint::GetPosition", 0x0072EA30, &bdg::JointConstraint_GetPosition);
BDG_REVERSED("JointConstraint::SetMatrix", 0x0072EA60, &bdg::JointConstraint_SetMatrix);
BDG_REVERSED("JointConstraint::Update", 0x0072EA80, &bdg::JointConstraint_Update);
BDG_REVERSED("FloorMotionDriver::Start", 0x0072EAD0, &bdg::FloorMotionDriver_Start);
BDG_REVERSED("ReachDriver::GetTarget", 0x0072EAF0, &bdg::ReachDriver_GetTarget);
BDG_REVERSED("BlendDriverGroup::Add", 0x0072EB10, &bdg::Group_Add);
BDG_REVERSED("BlendDriverGroup::Remove", 0x0072EB70, &bdg::Group_Remove);
BDG_REVERSED("BlendDriverGroup::Clear", 0x0072EBC0, &bdg::Group_Clear);
BDG_REVERSED("BlendDriverGroup::Reset", 0x0072EC10, &bdg::Group_Reset);
BDG_REVERSED("BlendDriverGroup::TargetVelocity", 0x0072EC50, &bdg::Group_TargetVelocity);
BDG_REVERSED("JointBlendDriverGroup::GetTransform", 0x0072ED40, &bdg::JointGroup_GetTransform);
BDG_REVERSED("IKBlendDriverGroup::GetTransform", 0x0072ED70, &bdg::IkGroup_GetTransform);
BDG_REVERSED("PedConstraints::SetEffectors", 0x0072EDA0, &bdg::Constraints_SetEffectors);
BDG_REVERSED("PedConstraints::Reset", 0x0072EDE0, &bdg::Constraints_Reset);
BDG_REVERSED("PedConstraints::Add", 0x0072EE50, &bdg::Constraints_Add);
BDG_REVERSED("PedConstraints::Remove", 0x0072EEB0, &bdg::Constraints_Remove);
BDG_REVERSED("PedConstraints::Clear", 0x0072EEF0, &bdg::Constraints_Clear);
BDG_REVERSED("AnimNode::Latch84", 0x0072F4F0, &bdg::Node_Latch);
BDG_REVERSED("FixedPool::Construct", 0x0072F5A0, &bdg::Pool_Construct);
BDG_REVERSED("BlendDriverBasic::Construct", 0x0072F620, &bdg::Construct);
BDG_REVERSED("BlendDriverBasic::IsA", 0x0072F660, &bdg::IsA);
BDG_REVERSED("JointConstraint::Destruct", 0x0072F6B0, &bdg::JointConstraint_Destruct);
BDG_REVERSED("JointConstraint::IsA", 0x0072F710, &bdg::JointConstraint_IsA);
BDG_REVERSED("JointConstraint::Attached", 0x0072F790, &bdg::JointConstraint_Attached);
BDG_REVERSED("JointConstraint::SetEnabled", 0x0072F800, &bdg::JointConstraint_SetEnabled);
BDG_REVERSED("FloorMotionDriver::Construct", 0x0072F830, &bdg::FloorMotionDriver_Construct);
BDG_REVERSED("FloorMotionDriver::GetRotation", 0x0072F870, &bdg::FloorMotionDriver_GetRotation);
BDG_REVERSED("FloorMotionDriver::GetPosition", 0x0072F890, &bdg::FloorMotionDriver_GetPosition);
BDG_REVERSED("ReachDriver::Construct", 0x0072F8C0, &bdg::ReachDriver_Construct);
BDG_REVERSED("ReachDriver::Reset", 0x0072F910, &bdg::ReachDriver_Reset);
BDG_REVERSED("ReachDriver::GetRotation", 0x0072F970, &bdg::ReachDriver_GetRotation);
BDG_REVERSED("BlendDriverGroup::Update", 0x0072F990, &bdg::Group_Update);
BDG_REVERSED("BlendDriverGroup::Blend", 0x0072FA80, &bdg::Group_Blend);
BDG_REVERSED("IKBlendDriverGroup::Apply", 0x0072FD60, &bdg::IkGroup_Apply);
BDG_REVERSED("PedConstraints::Destruct", 0x0072FE60, &bdg::Constraints_Destruct);
BDG_REVERSED("PedConstraints::Init", 0x0072FEE0, &bdg::Constraints_Init);
BDG_REVERSED("MotionPool::Init", 0x0072FF70, &bdg::MotionPool_Init);
BDG_REVERSED("CMatrix::FromQuatPos", 0x00730730, &bdg::MatrixFromQuatPos);
BDG_REVERSED("JointConstraint::GetMatrix", 0x007307D0, &bdg::JointConstraint_GetMatrix);
BDG_REVERSED("JointConstraint::GetRotation", 0x00730800, &bdg::JointConstraint_GetRotation);
