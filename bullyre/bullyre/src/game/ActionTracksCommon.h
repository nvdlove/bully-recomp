#pragma once

// Shared by the action tree's condition and track files of 0x601FB0..0x611DB0
// (ActionConditions*.cpp, ActionTracks*.cpp): the owner's ped, entity positions, and the
// shapes many small virtuals share (type ids, IsA, destructors).
//
// A condition: +0 vtable, its parameters from +4; vf1 Evaluate(owner) -> byte.
// A track: +0 vtable, +4 link, +8 keeps alive, +0xC start, +0x10 end, +0x14 key, its
// fields from +0x18 (ActionTree.h); vf0 IsA(id), vf1 deleting destructor, vf2
// Start(owner), vf3 Update(dt), vf4 End(), vf5 Resolve().
// The owner (the ped's action context): +0 its controller, +8 the ped.

#include <cmath>
#include <cstdint>

#include "game/ActionTree.h"
#include "game/Pool.h"
#include "reversed.h"

namespace actc {

using vc::At;

struct V3 {
    float x, y, z;
};
static_assert(sizeof(V3) == 0xC);

// CColPoint (0x28 bytes; +0xC a registered entity reference)
struct ColPoint {
    V3 pos;            // +0x0
    re::Ptr32<uint8_t> entity;   // +0xC
    V3 normal;         // +0x10
    uint8_t pad1C[8];  // +0x1C (not set by the callers here)
    float depth;       // +0x24
};
static_assert(sizeof(ColPoint) == 0x28);

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);      // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);     // CWorld::Players[0].m_pPed
GAME_VAR(uint8_t*, aiButes, 0x020C889C);       // the AI's tuning (+0x28.. engage distances)
GAME_VAR(uint8_t*, alwaysValidEntity, 0x00C2A8E8);
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);  // CModelInfo::ms_modelInfoPtrs
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);  // CPools::ms_pPedPool
GAME_VAR(float, timeStep, 0x00C1A9A4);          // CTimer::ms_fTimeStep
GAME_VAR(uint32_t, frameCounter, 0x00C1A99C);   // CTimer::m_FrameCounter
GAME_ARRAY(float, zeroVector, 0x00BCC564);
inline void* const animManager = reinterpret_cast<void*>(0x020C4910);

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);
GAME_FN(void, __thiscall, TrackBase_Destruct, 0x0061A8B0, uint8_t* track);
GAME_FN(uint8_t, __cdecl, Track_IsAStatic, 0x004437B0, int id);
GAME_FN(void, __cdecl, Entity_CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, NameKey_FromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, StringRef_Assign, 0x005772F0, uint32_t* field, uint32_t value);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, uint8_t* ped);
GAME_FN(V3*, __cdecl, VectorSub, 0x00414B70, V3* out, const float* a, const float* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, V3* v);
// value `op` reference (the tree's comparison: <, <=, ==, ...)
GAME_FN(uint8_t, __cdecl, CompareOp, 0x005FB8B0, float value, float reference, int op);

inline uint8_t* OwnerPed(uint8_t* owner) {
    return At<uint8_t*>(owner, 8);
}
// CPlaceable's position: its matrix's when it has one
inline float* Pos(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m ? reinterpret_cast<float*>(m + 0x30) : reinterpret_cast<float*>(e + 4);
}
inline bool IsPedEntity(uint8_t* e) {
    return (e[0x108] & 7) == 3;
}
// sqrt of the absolute value (the game's magnitude)
inline float SqrtAbs(float v) {
    return std::sqrt(std::fabs(v));
}
// a col point's entity reference released
inline void ReleaseColPoint(ColPoint& cp) {
    if (cp.entity) {
        // (the game tests the always-valid entity first; both ways do the same)
        Entity_CleanUpOldReference(cp.entity, &cp.entity);
    }
}

// a name key field set to none (through a temporary key)
inline void ClearName(uint8_t* self, int at) {
    uint32_t name;
    NameKey_FromValue(&name, 0);
    StringRef_Assign(&At<uint32_t>(self, at), name);
    NameKey_Destruct(&name);
}
// a name key field set from a name (the temporary's value copied raw)
inline void SetNameRaw(uint8_t* self, int at, const char* text) {
    uint32_t name;
    NameKey_Construct(&name, text);
    At<uint32_t>(self, at) = name;
    NameKey_Destruct(&name);
}
inline const char* const emptyString = reinterpret_cast<const char*>(0x008FEFBB);   // ""

template <uintptr_t Fn, typename R, typename... A>
inline R Thiscall(uint8_t* self, A... args) {
    return reinterpret_cast<R(__thiscall*)(uint8_t*, re::Arg<A>...)>(Fn)(self, args...);
}

// A class's type id from its guard/id pair (ActionTree.h), as a function
template <uintptr_t Guard, uintptr_t Id>
int16_t __cdecl TypeIdFn() {
    return acttree::TypeIdAt<Guard, Id>();
}
// IsA: its own id, else its base's (a cdecl static at BaseIsA)
template <uintptr_t Guard, uintptr_t Id, uintptr_t BaseIsA>
uint8_t __fastcall IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == acttree::TypeIdAt<Guard, Id>()) {
        return 1;
    }
    return reinterpret_cast<uint8_t(__cdecl*)(int)>(BaseIsA)(id);
}

// Deleting destructors: the vtable set, the base destructor (thiscall at Base), freed on
// flag 1
template <uintptr_t Vt, uintptr_t Base>
uint8_t* __fastcall DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = uint32_t(Vt);
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(Base)(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
// the same with name keys at the given offsets destroyed first (in that order)
template <uintptr_t Vt, uintptr_t Base, int... Keys>
uint8_t* __fastcall DeletingDtorKeys(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = uint32_t(Vt);
    (NameKey_Destruct(self + Keys), ...);
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(Base)(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// A track's destructor as its steps: a value from 0x900000 up is a vtable to set, one
// below a name key's offset to destroy; then the track base's destructor
template <uint32_t Step>
inline void DtorStep(uint8_t* self) {
    if constexpr (Step >= 0x900000) {
        At<uint32_t>(self, 0) = Step;
    } else {
        NameKey_Destruct(self + Step);
    }
}
template <uint32_t... Steps>
uint8_t* __fastcall TrackDeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    (DtorStep<Steps>(self), ...);
    TrackBase_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
template <uint32_t... Steps>
void __fastcall TrackDtor(uint8_t* self, re::Edx) {
    (DtorStep<Steps>(self), ...);
    TrackBase_Destruct(self);
}

} // namespace actc
