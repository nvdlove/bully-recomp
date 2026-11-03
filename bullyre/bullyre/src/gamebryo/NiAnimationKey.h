#pragma once

// Gamebryo's animation keys (NiAnimation): a key is its time and its value, with what its kind of
// interpolation keeps besides (tangents, TCB parameters, the derived cubic coefficients). Keys
// aren't NiObjects: arrays of them are handled through per-kind function tables indexed by
// content * 6 + type, which each key class fills at start-up (its Register function, called by
// the exe's static initializers, src/game/StaticInit1.cpp), with the key size in a byte table.
// Key arrays come from NiNew[] (the count before the first key).
//
// Everything in ni::anim (the value types would clash with the NiQuaternion / NiColorA other
// files define for themselves).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni::anim {

enum KeyContent : uint32_t { FLOATKEY, POSKEY, ROTKEY, COLORKEY, TEXTKEY, BOOLKEY, NUMKEYCONTENTS };
enum KeyType : uint32_t { NOINTERP, LINKEY, BEZKEY, TCBKEY, EULERKEY, STEPKEY, NUMKEYTYPES };

struct NiQuaternion {
    float w, x, y, z;
};
static_assert(sizeof(NiQuaternion) == 0x10);

struct NiColorA {
    float r, g, b, a;
};
static_assert(sizeof(NiColorA) == 0x10);

struct NiAnimationKey {
    float time;   // +0x0
};

// ---- float keys
struct NiFloatKey : NiAnimationKey {
    float value;   // +0x4
};
static_assert(sizeof(NiFloatKey) == 8);
using NiLinFloatKey = NiFloatKey;
using NiStepFloatKey = NiFloatKey;

struct NiBezFloatKey : NiFloatKey {
    float inTangent;    // +0x8
    float outTangent;   // +0xC
};
static_assert(sizeof(NiBezFloatKey) == 0x10);

struct NiTCBFloatKey : NiFloatKey {
    float tension;      // +0x8
    float continuity;   // +0xC
    float bias;         // +0x10
    float ds;           // +0x14 (the incoming tangent, FillDerivedVals')
    float dd;           // +0x18 (the outgoing one)
};
static_assert(sizeof(NiTCBFloatKey) == 0x1C);

// ---- position keys (interpolated as a cubic: pos + (out + (a + b t) t) t)
struct NiPosKey : NiAnimationKey {
    NiPoint3 pos;   // +0x4
};
static_assert(sizeof(NiPosKey) == 0x10);
using NiLinPosKey = NiPosKey;
using NiStepPosKey = NiPosKey;

struct NiBezPosKey : NiPosKey {
    NiPoint3 inTangent;    // +0x10
    NiPoint3 outTangent;   // +0x1C
    NiPoint3 a;            // +0x28 (3 (p1 - p0) - (2 out0 + in1))
    NiPoint3 b;            // +0x34 ((out0 + in1) - 2 (p1 - p0))
};
static_assert(offsetof(NiBezPosKey, a) == 0x28);
static_assert(sizeof(NiBezPosKey) == 0x40);

struct NiTCBPosKey : NiPosKey {
    float tension;      // +0x10
    float continuity;   // +0x14
    float bias;         // +0x18
    NiPoint3 ds;        // +0x1C (incoming tangent)
    NiPoint3 dd;        // +0x28 (outgoing tangent)
    NiPoint3 a;         // +0x34
    NiPoint3 b;         // +0x40
};
static_assert(offsetof(NiTCBPosKey, ds) == 0x1C);
static_assert(offsetof(NiTCBPosKey, b) == 0x40);
static_assert(sizeof(NiTCBPosKey) == 0x4C);

// ---- rotation keys
struct NiRotKey : NiAnimationKey {
    NiQuaternion quat;   // +0x4
};
static_assert(sizeof(NiRotKey) == 0x14);
using NiLinRotKey = NiRotKey;
using NiStepRotKey = NiRotKey;

struct NiBezRotKey : NiRotKey {
    NiQuaternion intQuat;   // +0x14 (the squad intermediate)
};
static_assert(sizeof(NiBezRotKey) == 0x24);

struct NiTCBRotKey : NiRotKey {
    float tension;      // +0x14
    float continuity;   // +0x18
    float bias;         // +0x1C
    NiQuaternion a;     // +0x20
    NiQuaternion b;     // +0x30
};
static_assert(offsetof(NiTCBRotKey, a) == 0x20);
static_assert(sizeof(NiTCBRotKey) == 0x40);

// one key holding three float key sequences (x, y, z angles); its time and quaternion unused
struct NiEulerRotKey : NiRotKey {
    uint32_t numKeys[3];   // +0x14
    KeyType type[3];       // +0x20
    uint8_t size[3];       // +0x2C (the sequences' key sizes; 0 for an empty one)
    uint8_t pad2F;
    re::Ptr32<NiFloatKey> keys[3];   // +0x30
    uint32_t lastIdx[3];   // +0x3C
};
static_assert(offsetof(NiEulerRotKey, type) == 0x20);
static_assert(offsetof(NiEulerRotKey, keys) == 0x30);
static_assert(offsetof(NiEulerRotKey, lastIdx) == 0x3C);
static_assert(sizeof(NiEulerRotKey) == 0x48);

// ---- color and bool keys
struct NiColorKey : NiAnimationKey {
    NiColorA color;   // +0x4
};
static_assert(sizeof(NiColorKey) == 0x14);
using NiLinColorKey = NiColorKey;
using NiStepColorKey = NiColorKey;

struct NiBoolKey : NiAnimationKey {
    uint8_t value;   // +0x4
    uint8_t pad5[3];
};
static_assert(sizeof(NiBoolKey) == 8);
using NiStepBoolKey = NiBoolKey;

// ---- the function tables (all cdecl), [content * 6 + type]
using FillDerivedValsFunction = void(__cdecl*)(NiAnimationKey* keys, uint32_t numKeys, uint8_t size);
using InsertFunction = uint8_t(__cdecl*)(float time, re::Ptr32<NiAnimationKey>* keys, uint32_t* numKeys);
using DeleteFunction = void(__cdecl*)(NiAnimationKey* keys);
using ArrayFunction = NiAnimationKey*(__cdecl*)(uint32_t numKeys);
using CopyFunction = void(__cdecl*)(NiAnimationKey* dest, const NiAnimationKey* src);
using SaveFunction = void(__cdecl*)(void* stream, NiAnimationKey* keys, uint32_t numKeys);
using EqualFunction = uint8_t(__cdecl*)(const NiAnimationKey* key0, const NiAnimationKey* key1);
// value (and its first and second derivatives) at normalized time t between two keys (an Euler
// key's interpolates its own sequences at the real time: key0 the key, key1 null)
using InterpFunction = void(__cdecl*)(float t, const NiAnimationKey* key0, const NiAnimationKey* key1,
                                      void* result);
using CurvatureFunction = float(__cdecl*)(NiAnimationKey* keys, uint32_t numKeys);
using CreateFunction = NiAnimationKey*(__cdecl*)(void* stream, uint32_t numKeys);

GAME_ARRAY(uint8_t, ms_keySizes, 0x020E4368);                          // [36]
GAME_ARRAY(FillDerivedValsFunction, ms_fillDerived, 0x020E4390);       // [36]
GAME_ARRAY(InsertFunction, ms_insert, 0x020E4420);
GAME_ARRAY(DeleteFunction, ms_delete, 0x020E44B0);
GAME_ARRAY(ArrayFunction, ms_array, 0x020E4540);
GAME_ARRAY(CopyFunction, ms_copy, 0x020E45D0);
GAME_ARRAY(SaveFunction, ms_save, 0x020E4660);
GAME_ARRAY(EqualFunction, ms_equal, 0x020E46F0);
GAME_ARRAY(InterpFunction, ms_interpD2, 0x020E4780);
GAME_ARRAY(InterpFunction, ms_interpD1, 0x020E4810);
GAME_ARRAY(InterpFunction, ms_interp, 0x020E48A0);
GAME_ARRAY(CurvatureFunction, ms_curvature, 0x020E4930);
GAME_ARRAY(CreateFunction, ms_create, 0x020E49C0);

inline uint32_t KeyIndex(KeyContent content, KeyType type) {
    return uint32_t(content) * uint32_t(NUMKEYTYPES) + uint32_t(type);
}

// key n of an array of keys `size` bytes each
template <typename T = NiAnimationKey>
inline T* KeyAt(const void* keys, uint32_t index, uint8_t size) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(keys) + index * size);
}

// NiNew[]'s size for count keys with the count before them (-1 when it overflows)
inline uint32_t KeyArraySize(uint32_t count, uint32_t size) {
    uint64_t product = uint64_t(count) * size;
    uint32_t bytes = (product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product);
    return bytes > 0xFFFFFFFB ? 0xFFFFFFFF : bytes + 4;
}

// The search every kind's GenInterp inlines (unrolled four times in the exe, the same result):
// from key lastIdx on (from the first when time is before it), lastIdx moved past every key
// before `time`; the index of the key after it returned, the two keys' times out (t1 is 0 when
// the search starts past the last key; an unordered time stops it)
inline uint32_t FindKeyInterval(float time, const void* keys, uint32_t numKeys, uint32_t& lastIdx, uint8_t size,
                                float& t0, float& t1) {
    uint32_t last = numKeys - 1;
    t0 = KeyAt(keys, lastIdx, size)->time;
    if (time < t0) {
        lastIdx = 0;
        t0 = KeyAt(keys, 0, size)->time;
    }
    uint32_t next = lastIdx + 1;
    t1 = 0.0f;
    while (next <= last) {
        t1 = KeyAt(keys, next, size)->time;
        if (!(t1 < time)) {
            break;
        }
        lastIdx++;
        t0 = t1;
        next++;
    }
    return next;
}

} // namespace ni::anim
