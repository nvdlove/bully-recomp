// AM_MotionTrack: the locomotion animation track that picks the walk/run cycle by the
// turn between the ped's movement direction and its heading (up to 10 cycles keyed by
// angle, each with four foot keys), keeps its phase matched to the cycle it blends from,
// fires the foot events, and blends towards the next cycle. Its blocks come from the
// motion pool (0x20C9028). 0x72FFB0..0x732530.
//
// The track (AM_AnimTrack, 0x6B2850 its Update): +0x18 its anim, +0x1C time, +0x24
// speed, +0x28 mode, +0x2C a second anim player (+0x30 time, +0x38 speed, +0x3C mode,
// +0x40 its blend), +0x48 the ped, +0x4C the locomotion track, +0x50 the cycle in use,
// +0x54 the cycles (LocomotionAnimationNewTrack: +0x18 name, +0x1C angle, +0x20..+0x2C
// the foot keys), +0x7C started, +0x80 the last time, +0x88 a phase to blend in from,
// +0x8C the move speed, +0x90 the movement direction, +0x9C the heading.

#include <cstdint>

#include "game/ActionTree.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct MtVec {
    float x, y, z;
};
static_assert(sizeof(MtVec) == 0xC);

struct MtPool {
    int32_t used;       // +0x0
    int32_t numFree;    // +0x4
    int32_t size;       // +0x8
    int32_t count;      // +0xC
    int32_t total;      // +0x10
    re::Ptr32<re::Ptr32<uint8_t>> free;     // +0x14
    re::Ptr32<uint8_t> blocks;    // +0x18
};
static_assert(sizeof(MtPool) == 0x1C);

GAME_VAR(MtPool*, mtPool, 0x020C9028);
GAME_ARRAY(float, mtZero, 0x00BCC564);
GAME_VAR(float, mtSpeedScale, 0x00BC724C);     // 3
GAME_VAR(float, mtTurnRate, 0x00BC7254);       // 300
GAME_VAR(float, mtTurnSign, 0x00BC72D4);       // -1
GAME_VAR(float, mtTwoPi, 0x00BC7258);
GAME_VAR(float, mtStartEpsilon, 0x00BC733C);   // 0.06
GAME_VAR(int32_t, mtScaleBySpeed, 0x00BC7338);
GAME_VAR(int32_t, mtNoSpeedWhenBlended, 0x020C905C);
constexpr uint32_t kVtMotionTrack = 0x0094D2F8;
constexpr uint32_t kVtLocomotionTrack = 0x00930EC4;
constexpr uint32_t kVtLocomotionAnimationNewTrack = 0x00930EDC;

GAME_FN(void, __thiscall, Mt_AnimTrackReset, 0x006B8F30, uint8_t* track);
GAME_FN(uint8_t*, __thiscall, Mt_AnimTrackCopyFrom, 0x006B4670, uint8_t* track, uint8_t* other);
GAME_FN(uint8_t, __thiscall, Mt_AnimTrackUpdate, 0x006B2850, uint8_t* track, float dt);
GAME_FN(uint8_t*, __thiscall, Mt_Construct, 0x007315C0, uint8_t* track);
GAME_FN(uint8_t*, __thiscall, Mt_CopyFrom, 0x00730090, uint8_t* track, uint8_t* other);
GAME_FN(void, __thiscall, Mt_Destruct, 0x0072FFB0, uint8_t* track);
GAME_FN(void, __thiscall, Mt_UpdateDirection, 0x00730260, uint8_t* track, float dt);
GAME_FN(float, __stdcall, Mt_Travel, 0x00730490, uint8_t* anim, uint8_t* dict);
GAME_FN(uint8_t*, __cdecl, Mt_GetBlend, 0x00731C30, uint8_t* model, int slot);
GAME_FN(float, __stdcall, Mt_HeadingAngle_x64, 0x0072F3F0, float, float, float, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline float Mt_HeadingAngle(MtVec a0, MtVec a1) {
    return Mt_HeadingAngle_x64(re::FloatAt(a0, 0), re::FloatAt(a0, 1), re::FloatAt(a0, 2), re::FloatAt(a1, 0), re::FloatAt(a1, 1), re::FloatAt(a1, 2));
}
GAME_FN(float, __cdecl, Mt_MapGait, 0x0072F140, float time, float length, float key0, float key1, float key2, float key3,
        float otherLength, float other0, float other1, float other2, float other3);
GAME_FN(float, __cdecl, Mt_BlendPhase, 0x0072E7E0, float t, float length, float from, float to);
GAME_FN(float, __cdecl, Mt_RemapClamped, 0x0072F500, float v, float a, float b, float c, float d);
GAME_FN(void, __thiscall, Mt_TrackBaseConstruct, 0x0061A880, uint8_t* track);
GAME_FN(void, __thiscall, Mt_KeyDefault, 0x0066BEA0, uint32_t* key);
GAME_FN(void*, __cdecl, Mt_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(float, __thiscall, Mt_Normalise, 0x00414E10, MtVec* v);
GAME_FN(float, __cdecl, Mt_SafeAcos, 0x00413D60, float x);
GAME_FN(void, __cdecl, Mt_RotateVector2, 0x005FBFB0, float angle, float* v);
GAME_FN(MtVec*, __thiscall, Mt_GetForward, 0x004154A0, uint8_t* placeable, MtVec* out);
GAME_FN(int, __thiscall, Mt_BoneTravel, 0x00607500, uint8_t* dict, int bone, float* out);
GAME_FN(uint8_t*, __thiscall, Mt_SlotsGet, 0x006B1FA0, uint8_t* model, int slot);
GAME_FN(int32_t, __cdecl, Mt_AnimSlotTypeId, 0x006075F0);
GAME_FN(int32_t, __cdecl, Mt_IndexOf, 0x00607770, const uint32_t* name, uint8_t* dict);
GAME_FN(int, __thiscall, Mt_HxdAddGroupRef, 0x006B1C60, uint8_t* hxd, uint32_t group);
GAME_FN(int, __thiscall, Mt_HxdReleaseGroup, 0x006B3DA0, uint8_t* hxd, uint32_t group, int keep);
GAME_FN(int32_t, __thiscall, Mt_PedGetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(void, __thiscall, Mt_SetEventA, 0x005F3770, uint8_t* intention, int set);
GAME_FN(void, __thiscall, Mt_SetEventB, 0x005F37A0, uint8_t* intention, int set);
GAME_FN(uint8_t, __cdecl, Mt_NearlyEqual, 0x00432A60, float a, float b, float epsilon);
GAME_FN(void, __thiscall, Mt_PlayerSetAnim, 0x006B5600, uint8_t* player, uint8_t* anim);

int16_t MotionTrackId() {
    return acttree::TypeIdAt<0x01B65254, 0x01B65250>();
}
// a vtable call's thiscall IsA with an id
uint8_t IsA(uint8_t* object, uint32_t id) {
    return vc::Call<uint8_t>(object, 0, id);
}
// the ped's motion intention (the ped's vf16, +0x14)
uint8_t* Intention(uint8_t* self) {
    return At<uint8_t*>(vc::Call<uint8_t*>(At<uint8_t*>(self, 0x48), 0x40), 0x14);
}
// a cycle's anim in the dictionary (index clamped at 0; past the end: none)
uint8_t* CycleAnim(uint8_t* cycle, uint8_t* dict) {
    int32_t index = Mt_IndexOf(reinterpret_cast<uint32_t*>(cycle + 0x18), dict);
    if (index < 0) {
        index = 0;
    }
    return uint32_t(index) < At<uint32_t>(dict, 4) ? At<uint8_t*>(dict, 0) + index * 0x40 : nullptr;
}
// a new track's block copied raw (the game copies into null when out of memory)
void CopyDwords(void* to, const void* from, int count) {
    uint32_t* d = static_cast<uint32_t*>(to);
    const uint32_t* s = static_cast<const uint32_t*>(from);
    for (int i = 0; i < count; i++) {
        d[i] = s[i];
    }
}
uint8_t* NewCycle() {
    uint8_t* cycle = static_cast<uint8_t*>(Mt_Malloc(0x34));
    if (cycle == nullptr) {
        return nullptr;
    }
    Mt_TrackBaseConstruct(cycle);
    At<uint32_t>(cycle, 0) = kVtLocomotionAnimationNewTrack;
    Mt_KeyDefault(&At<uint32_t>(cycle, 0x18));
    return cycle;
}

} // namespace

namespace motiontrack {

// 0x72FFB0 (thiscall): the destructor's body (its tracks deleted)
void __fastcall Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtMotionTrack;
    Mt_AnimTrackReset(self);
    for (int i = 0; i < 10; i++) {
        uint8_t* cycle = At<uint8_t*>(self, 0x54 + 4 * i);
        if (cycle == At<uint8_t*>(self, 0x50)) {
            At<uint8_t*>(self, 0x50) = nullptr;
        }
        cycle = At<uint8_t*>(self, 0x54 + 4 * i);
        if (cycle != nullptr) {
            vc::Call<void>(cycle, 0x4, 1);
        }
    }
    if (At<uint8_t*>(self, 0x50) != nullptr) {
        vc::Call<void>(At<uint8_t*>(self, 0x50), 0x4, 1);
    }
    if (At<uint8_t*>(self, 0x4C) != nullptr) {
        vc::Call<void>(At<uint8_t*>(self, 0x4C), 0x4, 1);
    }
}

// 0x730020 (vf0): IsA (its class or AM_AnimTrack's)
uint8_t __fastcall IsATrack(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == MotionTrackId()) {
        return 1;
    }
    return int16_t(id) == acttree::TypeIdAt<0x020C48BC, 0x020C48B8>();
}

// 0x730090 (thiscall): a copy of another (its tracks copied into new blocks)
uint8_t* __fastcall CopyFrom(uint8_t* self, re::Edx, uint8_t* other) {
    Mt_AnimTrackCopyFrom(self, other);
    At<uint32_t>(self, 0x48) = At<uint32_t>(other, 0x48);
    At<uint8_t*>(self, 0x4C) = nullptr;
    if (At<uint8_t*>(other, 0x4C) != nullptr) {
        uint8_t* loco = static_cast<uint8_t*>(Mt_Malloc(0xA4));
        if (loco != nullptr) {
            Mt_TrackBaseConstruct(loco);
            At<uint32_t>(loco, 0) = kVtLocomotionTrack;
            At<float>(loco, 0x94) = 0.0f;
            At<float>(loco, 0x98) = 0.0f;
            At<float>(loco, 0x9C) = 0.0f;
        }
        CopyDwords(loco, At<uint8_t*>(other, 0x4C), 0x29);
        At<uint8_t*>(self, 0x4C) = loco;
    }
    At<uint8_t*>(self, 0x50) = nullptr;
    for (int i = 0; i < 10; i++) {
        uint8_t* from = At<uint8_t*>(other, 0x54 + 4 * i);
        if (from == nullptr) {
            At<uint8_t*>(self, 0x54 + 4 * i) = nullptr;
            continue;
        }
        uint8_t* cycle = NewCycle();
        CopyDwords(cycle, from, 0xD);
        At<uint8_t*>(self, 0x54 + 4 * i) = cycle;
        if (At<uint8_t*>(other, 0x50) == from) {
            At<uint8_t*>(self, 0x50) = cycle;
        }
    }
    if (At<uint8_t*>(self, 0x50) == nullptr && At<uint8_t*>(other, 0x50) != nullptr) {
        uint8_t* cycle = NewCycle();
        CopyDwords(cycle, At<uint8_t*>(other, 0x50), 0xD);
        At<uint8_t*>(self, 0x50) = cycle;
    }
    At<uint8_t>(self, 0x7C) = At<uint8_t>(other, 0x7C);
    At<float>(self, 0x80) = At<float>(other, 0x80);
    At<float>(self, 0x84) = At<float>(other, 0x84);
    At<float>(self, 0x8C) = At<float>(other, 0x8C);
    At<uint32_t>(self, 0x90) = At<uint32_t>(other, 0x90);
    At<uint32_t>(self, 0x94) = At<uint32_t>(other, 0x94);
    At<uint32_t>(self, 0x98) = At<uint32_t>(other, 0x98);
    At<uint32_t>(self, 0x9C) = At<uint32_t>(other, 0x9C);
    At<uint32_t>(self, 0xA0) = At<uint32_t>(other, 0xA0);
    At<uint32_t>(self, 0xA4) = At<uint32_t>(other, 0xA4);
    return self;
}

// 0x730260 (thiscall): the move speed and direction from the ped's intention (the
// direction turned towards the wanted one at 300 * dt radians at most), the heading
void __fastcall UpdateDirection(uint8_t* self, re::Edx, float dt) {
    uint8_t* ped = At<uint8_t*>(self, 0x48);
    At<float>(self, 0x8C) = At<float>(At<uint8_t*>(vc::Call<uint8_t*>(ped, 0x40), 0x14), 0x20);
    const float* wantedRaw = At<float*>(At<uint8_t*>(self, 0x48), 0x2E0);
    MtVec wanted{wantedRaw[0], wantedRaw[1], 0.0f};
    Mt_Normalise(&wanted);
    if (0.0f > At<float>(self, 0x8C)) {
        At<float>(self, 0x8C) = At<float>(At<uint8_t*>(At<uint8_t*>(self, 0x48), 0x2E0), 0x1C) * mtSpeedScale;
    }
    MtVec& dir = At<MtVec>(self, 0x90);
    bool turn = false;
    float maxTurn = dt;
    if (dir.x != mtZero[0] || dir.y != mtZero[1] || dir.z != mtZero[2]) {
        MtVec side{dir.y, -dir.x, 0.0f};
        Mt_Normalise(&side);
        float cosAngle = dir.y * wanted.y + dir.x * wanted.x;
        if (cosAngle < -1.0f) {
            cosAngle = -1.0f;
        } else if (1.0f < cosAngle) {
            cosAngle = 1.0f;
        }
        maxTurn = mtTurnRate * dt;
        float angle = Mt_SafeAcos(cosAngle);
        turn = maxTurn < angle;
    }
    if (turn) {
        MtVec side{dir.y, -dir.x, 0.0f};
        Mt_Normalise(&side);
        float sideDot = side.y * wanted.y + side.x * wanted.x;
        float off = float(sideDot - 0.0);
        off = off < 0.0f ? -off : off;
        if (off > 0.0001 && 0.0f < sideDot) {
            maxTurn = mtTurnSign * maxTurn;
        }
        Mt_RotateVector2(maxTurn, &dir.x);
    } else {
        dir = wanted;
    }
    MtVec forward;
    At<MtVec>(self, 0x9C) = *Mt_GetForward(At<uint8_t*>(self, 0x48), &forward);
}

// 0x730490 (stdcall; the caller's ecx unused): an anim's travel speed (its root bone's
// ground travel over its length)
float __stdcall Travel(uint8_t* anim, uint8_t* dict) {
    float length = At<float>(anim, 0xC);
    float off = float(length - 0.0);
    off = off < 0.0f ? -off : off;
    if (!(off > 1e-5f || off != off)) {
        return 0.0f;
    }
    float travel[3];
    travel[0] = 0.0f;
    travel[1] = 0.0f;
    Mt_BoneTravel(dict, At<uint16_t>(anim, 0x32), travel);
    float lengthSq = travel[1] * travel[1] + travel[0] * travel[0];
    lengthSq = lengthSq < 0.0f ? -lengthSq : lengthSq;
    float magnitude = crt::Sqrtf(lengthSq);
    return magnitude / length;
}

// 0x7315C0 (thiscall): the constructor's body
uint8_t* __fastcall Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 4) = 0;
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 0.0f;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x28) = 0;
    At<float>(self, 0x24) = 1.0f;
    At<uint32_t>(self, 0x2C) = 0;
    At<uint32_t>(self, 0x3C) = 0;
    At<float>(self, 0x30) = 0.0f;
    At<float>(self, 0x34) = 0.0f;
    At<float>(self, 0x38) = 1.0f;
    At<uint32_t>(self, 0) = kVtMotionTrack;
    for (int at = 0x90; at <= 0xA4; at += 4) {
        At<float>(self, at) = 0.0f;
    }
    At<uint32_t>(self, 0x48) = 0;
    for (int at = 0x54; at <= 0x78; at += 4) {
        At<uint32_t>(self, at) = 0;
    }
    At<float>(self, 0x8C) = 0.0f;
    At<uint32_t>(self, 0x50) = 0;
    At<uint32_t>(self, 0x4C) = 0;
    At<uint32_t>(self, 0x90) = *reinterpret_cast<uint32_t*>(&mtZero[0]);
    At<uint32_t>(self, 0x94) = *reinterpret_cast<uint32_t*>(&mtZero[1]);
    At<uint32_t>(self, 0x98) = *reinterpret_cast<uint32_t*>(&mtZero[2]);
    At<uint32_t>(self, 0x9C) = *reinterpret_cast<uint32_t*>(&mtZero[0]);
    At<uint32_t>(self, 0xA0) = *reinterpret_cast<uint32_t*>(&mtZero[1]);
    At<float>(self, 0x80) = -1.0f;
    At<float>(self, 0x88) = -1.0f;
    At<uint32_t>(self, 0xA4) = *reinterpret_cast<uint32_t*>(&mtZero[2]);
    At<uint8_t>(self, 0x7C) = 0;
    return self;
}

// 0x7316B0 (vf1): deleted (back to the motion pool when it's one of its blocks)
void __fastcall Release(uint8_t* self, re::Edx) {
    if (self == nullptr) {
        return;
    }
    Mt_Destruct(self);
    MtPool* pool = mtPool;
    if (self >= pool->blocks && self < pool->blocks + pool->total) {
        pool->used += -1;
        pool->free[pool->numFree] = self;
        pool->numFree += 1;
    }
}

// 0x7316F0 (vf2): a copy from the motion pool (the game copies into null when the pool
// is empty)
uint8_t* __fastcall Clone(uint8_t* self, re::Edx) {
    MtPool* pool = mtPool;
    bool fits = pool->size >= 0xA8;
    uint8_t* copy = nullptr;
    if (pool->numFree > 0 && fits) {
        pool->used += 1;
        pool->numFree += -1;
        uint8_t* block = pool->free[pool->numFree];
        if (block != nullptr) {
            copy = Mt_Construct(block);
        }
    }
    Mt_CopyFrom(copy, self);
    return copy;
}

// 0x731C30 (cdecl): the motion track blending in a model's slot: the slot's motion track
// unless the slot's action track is weighted more (the motion track's weight under 0.5)
uint8_t* __cdecl GetBlend(uint8_t* model, int slot) {
    uint8_t* actionTrack = nullptr;
    uint8_t* link = At<uint8_t*>(model, 0x34 + slot * 0xC);
    if (link != nullptr) {
        uint8_t* track = link - 4;
        if (track != nullptr) {
            uint8_t* vtable = At<uint8_t*>(track, 0);
            int32_t id = Mt_AnimSlotTypeId();
            uint8_t isMotion = re::PtrCast<uint8_t(__thiscall*)(uint8_t*, int32_t)>(At<void*>(vtable, 0))(track, id);
            actionTrack = isMotion ? track : nullptr;
        }
    }
    uint8_t* anim = Mt_SlotsGet(model, slot);
    if (anim != nullptr && IsA(anim, uint16_t(MotionTrackId())) && (actionTrack == nullptr || 0.5f <= At<float>(anim, 0xC))) {
        return anim;
    }
    return actionTrack;
}

// 0x731CE0 (vf3): Update
uint8_t __fastcall Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* model = At<uint8_t*>(At<uint8_t*>(self, 0x48), 0x114);
    uint8_t* dict = At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10);
    // the track this one blends from (a started one in slot 1, else 0)
    uint8_t* source = nullptr;
    for (int slot = 1; slot >= 0 && source == nullptr; slot--) {
        uint8_t* blend = Mt_GetBlend(model, slot);
        if (blend != nullptr && At<uint8_t>(blend, 0x7C) != 0) {
            source = blend;
        }
    }
    uint8_t* s0 = Mt_SlotsGet(model, 0);
    bool steer = s0 != nullptr && IsA(s0, uint16_t(MotionTrackId()));
    if (!steer) {
        uint8_t* s1 = Mt_SlotsGet(model, 1);
        if (s1 != nullptr) {
            uint8_t* vtable = At<uint8_t*>(s1, 0);
            int32_t id = Mt_AnimSlotTypeId();
            steer = re::PtrCast<uint8_t(__thiscall*)(uint8_t*, int32_t)>(At<void*>(vtable, 0))(s1, id) != 0;
        }
    }
    if (steer) {
        Mt_UpdateDirection(self, dt);
    }
    if (source != nullptr) {
        At<uint32_t>(self, 0x90) = At<uint32_t>(source, 0x90);
        At<uint32_t>(self, 0x94) = At<uint32_t>(source, 0x94);
        At<uint32_t>(self, 0x98) = At<uint32_t>(source, 0x98);
    }
    At<float>(self, 0x98) = 0.0f;
    Mt_Normalise(&At<MtVec>(self, 0x90));
    float angle = Mt_HeadingAngle(At<MtVec>(self, 0x90), At<MtVec>(self, 0x9C));
    uint8_t* first = At<uint8_t*>(self, 0x54);
    if (first == nullptr) {
        return 1;
    }
    // the cycles around the angle: the last at or under it, the first above it (else
    // wrapping round to the first)
    uint8_t* low = first;
    uint8_t* next = nullptr;
    bool any = false;
    for (int i = 1; i < 10 && next == nullptr; i++) {
        uint8_t* cycle = At<uint8_t*>(self, 0x54 + 4 * i);
        if (cycle == nullptr) {
            continue;
        }
        any = true;
        if (At<float>(cycle, 0x1C) <= angle) {
            low = cycle;
        } else {
            next = cycle;
        }
    }
    if (any && next == nullptr) {
        next = first;
    }
    float time = At<float>(self, 0x1C);
    float lastTime = At<float>(self, 0x80);
    uint8_t* anim = CycleAnim(low, dict);
    float length = At<float>(anim, 0xC);
    float key0 = At<float>(low, 0x20);
    float key1 = At<float>(low, 0x24);
    float key2 = At<float>(low, 0x28);
    float key3 = At<float>(low, 0x2C);
    // the cycle the time is in (the source's, else this one's last, else the new one)
    float fromLength = length, from0 = key0, from1 = key1, from2 = key2, from3 = key3;
    uint8_t* fromCycle = nullptr;
    if (source != nullptr && At<uint8_t*>(source, 0x18) != nullptr) {
        time = At<float>(source, 0x84);
        fromLength = At<float>(At<uint8_t*>(source, 0x18), 0xC);
        fromCycle = At<uint8_t*>(source, 0x50);   // (not checked: the game reads through it)
    } else if (At<uint8_t*>(self, 0x50) != nullptr && At<uint8_t*>(self, 0x18) != nullptr) {
        time = At<float>(self, 0x1C);
        fromLength = At<float>(At<uint8_t*>(self, 0x18), 0xC);
        fromCycle = At<uint8_t*>(self, 0x50);
    }
    if (fromCycle != nullptr || (source != nullptr && At<uint8_t*>(source, 0x18) != nullptr)) {
        from0 = At<float>(fromCycle, 0x20);
        from1 = At<float>(fromCycle, 0x24);
        from2 = At<float>(fromCycle, 0x28);
        from3 = At<float>(fromCycle, 0x2C);
    }
    uint8_t own = source == nullptr || source == self;
    float newTime = Mt_MapGait(time, fromLength, from0, from1, from2, from3, length, key0, key1, key2, key3);
    if (lastTime >= 0.0f && own) {
        At<float>(self, 0x80) = Mt_MapGait(lastTime, fromLength, from0, from1, from2, from3, length, key0, key1, key2, key3);
    }
    if (At<uint8_t>(self, 0x7C) == 0 && !own) {
        float last = At<float>(self, 0x80);
        if (0.0f > last) {
            if (Mt_NearlyEqual(newTime, key0, mtStartEpsilon)) {
                At<uint8_t>(self, 0x7C) = 1;
            }
            if (Mt_NearlyEqual(newTime, key2, mtStartEpsilon)) {
                At<uint8_t>(self, 0x7C) = 1;
            }
        } else {
            if (At<float>(self, 0x80) <= key0 && key0 <= newTime) {
                At<uint8_t>(self, 0x7C) = 1;
            }
            if (At<float>(self, 0x80) <= key2 && key2 <= newTime) {
                At<uint8_t>(self, 0x7C) = 1;
            }
        }
    }
    if (At<uint8_t>(self, 0x7C) == 0) {
        At<uint8_t>(self, 0x7C) = 1;
    }
    if (own && 0.0f <= At<float>(self, 0x88)) {
        newTime = Mt_BlendPhase(At<float>(self, 0x88), length, key0, key2);
        At<float>(self, 0x88) = -1.0f;
    }
    // the foot events: a foot key passed since the last time (wrapping round)
    if (0.0f < At<float>(self, 0x80) && own) {
        float last = At<float>(self, 0x80);
        if ((last <= key0 && newTime >= key0) || (last > newTime && (last <= key0 || newTime >= key0))) {
            Mt_SetEventA(Intention(self), 1);
        }
        last = At<float>(self, 0x80);
        if ((last <= key2 && newTime >= key2) || (last > newTime && (last <= key2 || newTime >= key2))) {
            Mt_SetEventB(Intention(self), 1);
        }
    }
    At<uint8_t*>(self, 0x50) = low;
    if (At<uint8_t*>(anim, 8) != nullptr) {
        Mt_HxdAddGroupRef(At<uint8_t*>(anim, 8), At<uint32_t>(anim, 0x20));
    }
    uint8_t* oldAnim = At<uint8_t*>(self, 0x18);
    if (oldAnim != nullptr && At<uint8_t*>(oldAnim, 8) != nullptr) {
        Mt_HxdReleaseGroup(At<uint8_t*>(oldAnim, 8), At<uint32_t>(oldAnim, 0x20), 0);
    }
    At<uint8_t*>(self, 0x18) = anim;
    At<float>(self, 0x1C) = newTime;
    At<int32_t>(self, 0x28) = 7;
    float speed = float(double(Mt_PedGetStat(At<uint8_t*>(self, 0x48), 0x14)) / 100.0);
    float moveSpeed = At<float>(At<uint8_t*>(vc::Call<uint8_t*>(At<uint8_t*>(self, 0x48), 0x40), 0x14), 0x1C);
    At<float>(self, 0x8C) = moveSpeed;
    if (own && mtScaleBySpeed != 0) {
        if (0.0f <= moveSpeed) {
            float scale = 1.0f;
            float travel = Mt_Travel(anim, dict);
            if (travel > 0.0f) {
                float ratio = At<float>(self, 0x8C) / travel;
                At<float>(self, 0x24) = ratio * speed;
            } else {
                At<float>(self, 0x24) = scale * speed;
            }
        } else {
            At<float>(self, 0x24) = speed;
        }
    }
    if (next != nullptr) {
        uint8_t* nextAnim = CycleAnim(next, dict);
        float nextTime = Mt_MapGait(time, fromLength, from0, from1, from2, from3, At<float>(nextAnim, 0xC), At<float>(next, 0x20),
                                    At<float>(next, 0x24), At<float>(next, 0x28), At<float>(next, 0x2C));
        Mt_PlayerSetAnim(self + 0x2C, nextAnim);
        At<float>(self, 0x30) = nextTime;
        At<int32_t>(self, 0x3C) = 7;
        float lowAngle = At<float>(low, 0x1C);
        float nextAngle = At<float>(next, 0x1C);
        if (nextAngle < lowAngle) {
            nextAngle = nextAngle + mtTwoPi;
        }
        float w = Mt_RemapClamped(angle, lowAngle, nextAngle, 0.0f, 1.0f);
        if (w < 0.0f) {
            At<float>(self, 0x40) = 0.0f;
        } else if (1.0f < w) {
            At<float>(self, 0x40) = 1.0f;
        } else {
            At<float>(self, 0x40) = w;
        }
        At<float>(self, 0x38) = At<float>(self, 0x24);
    } else {
        At<float>(self, 0x40) = 0.0f;
    }
    At<float>(self, 0x80) = At<float>(self, 0x1C);
    if (!own && mtNoSpeedWhenBlended != 0) {
        At<float>(self, 0x24) = 0.0f;
        At<float>(self, 0x38) = 0.0f;
    }
    return Mt_AnimTrackUpdate(self, dt);
}

} // namespace motiontrack

#define MOTIONTRACK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

MOTIONTRACK_REVERSED("AM_MotionTrack::Destruct", 0x0072FFB0, &motiontrack::Destruct);
MOTIONTRACK_REVERSED("AM_MotionTrack::IsA", 0x00730020, &motiontrack::IsATrack);
MOTIONTRACK_REVERSED("AM_MotionTrack::CopyFrom", 0x00730090, &motiontrack::CopyFrom);
MOTIONTRACK_REVERSED("AM_MotionTrack::UpdateDirection", 0x00730260, &motiontrack::UpdateDirection);
MOTIONTRACK_REVERSED("AM_MotionTrack::Travel", 0x00730490, &motiontrack::Travel);
MOTIONTRACK_REVERSED("AM_MotionTrack::Construct", 0x007315C0, &motiontrack::Construct);
MOTIONTRACK_REVERSED("AM_MotionTrack::Release", 0x007316B0, &motiontrack::Release);
MOTIONTRACK_REVERSED("AM_MotionTrack::Clone", 0x007316F0, &motiontrack::Clone);
MOTIONTRACK_REVERSED("AM_MotionTrack::GetBlend", 0x00731C30, &motiontrack::GetBlend);
MOTIONTRACK_REVERSED("AM_MotionTrack::Update", 0x00731CE0, &motiontrack::Update);
