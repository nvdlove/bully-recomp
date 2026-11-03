// The position keys (src/gamebryo/NiAnimationKey.h): NiPosKey (content POSKEY, NOINTERP),
// NiLinPosKey, NiBezPosKey, NiTCBPosKey and NiStepPosKey. GenInterp and its clamped form, the
// coordinate frame along a path (tangent, normal, binormal, curvature: NiPathController's), each
// kind's interpolation and its derivatives, the cubic coefficients and TCB tangents, the largest
// curvature (sampled), inserting a key, comparison, saving, and each kind's Register (its table
// entries once; the per-content entries through RegisterSupportedFunctions).
//
// Main-thread float maths at the game's 24-bit x87 precision: each operation a float one, in the
// game's order.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiAnimationKey.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

GAME_VAR(uint8_t, NiPosKey_registered, 0x020E431A);
GAME_VAR(uint8_t, NiBezPosKey_registered, 0x020E432C);
GAME_VAR(uint8_t, NiTCBPosKey_registered, 0x020E4359);
GAME_VAR(uint8_t, NiLinPosKey_registered, 0x020E4357);
GAME_VAR(uint8_t, NiStepPosKey_registered, 0x020E435F);
GAME_VAR(float, NiPosKey_ms_fSampleDelta, 0x020E431C);         // 1 / ms_iSampleIterations
GAME_VAR(int32_t, NiPosKey_ms_iSampleIterations, 0x00BC8B84);
GAME_VAR(NiPoint3, NiPosKey_ms_kDefault, 0x020E4320);           // NiPoint3::ZERO's copy
GAME_VAR(NiPoint3, NiPoint3_ZERO, 0x020E2D44);

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);
GAME_FN(uint8_t, __thiscall, Point3_Equal, 0x0076BA40, const NiPoint3* self, const NiPoint3* other);
GAME_FN(float, __thiscall, Point3_Unitize, 0x00415130, NiPoint3* self);
GAME_FN(void, __cdecl, AnimationKey_SaveBinary, 0x007FFC10, void* stream, const NiAnimationKey* key);
GAME_FN(uint8_t, __cdecl, AnimationKey_Equal, 0x007FF6C0, const NiAnimationKey* key0, const NiAnimationKey* key1);
GAME_FN(uint8_t, __cdecl, AnimationKey_OkayToInsert, 0x007FF770, float time, const NiAnimationKey* keys,
        uint32_t numKeys, uint32_t* index, uint8_t size);
GAME_FN(void, __cdecl, AnimationKey_SplitBezierTangents, 0x007FFCD0, float value0, float time0, float* outTangent0,
        float value1, float time1, float* inTangent1, float time, float value, float* inTangent, float* outTangent);
// (this file's, called where modes apply)
GAME_FN(void, __cdecl, PosKey_SaveKey, 0x007F8B30, void* stream, const NiPosKey* key);
GAME_FN(NiPoint3*, __cdecl, PosKey_GenInterp, 0x007F8B50, NiPoint3* result, float time, NiPosKey* keys, KeyType type,
        uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(NiPoint3*, __cdecl, PosKey_GenInterpDefault, 0x007F8D80, NiPoint3* result, float time, NiPosKey* keys,
        KeyType type, uint32_t numKeys, uint8_t size);
GAME_FN(uint8_t, __cdecl, PosKey_Equal, 0x007F8E40, const NiAnimationKey* key0, const NiAnimationKey* key1);
GAME_FN(void, __cdecl, PosKey_RegisterSupportedFunctions, 0x007F9180, KeyContent content, KeyType type);
GAME_FN(void, __cdecl, BezPosKey_InterpD1, 0x007F9440, float t, const NiAnimationKey* key0,
        const NiAnimationKey* key1, void* result);
GAME_FN(void, __cdecl, BezPosKey_InterpD2, 0x007F9520, float t, const NiAnimationKey* key0,
        const NiAnimationKey* key1, void* result);
GAME_FN(void, __cdecl, BezPosKey_FillDerivedVals, 0x007F95C0, NiAnimationKey* keys, uint32_t numKeys, uint8_t size);
GAME_FN(void, __cdecl, BezPosKey_RegisterSupportedFunctions, 0x007F9D00, KeyContent content, KeyType type);
GAME_FN(void, __cdecl, LinPosKey_RegisterSupportedFunctions, 0x007FC290, KeyContent content, KeyType type);
GAME_FN(void, __cdecl, TCBPosKey_InterpD1, 0x007FC880, float t, const NiAnimationKey* key0,
        const NiAnimationKey* key1, void* result);
GAME_FN(void, __cdecl, TCBPosKey_InterpD2, 0x007FC960, float t, const NiAnimationKey* key0,
        const NiAnimationKey* key1, void* result);
GAME_FN(void, __thiscall, TCBPosKey_CalculateDVals, 0x007FCA00, NiTCBPosKey* self, const NiPoint3* pointIn,
        const NiPoint3* pointOut, float preLength, float nextLength);
GAME_FN(void, __cdecl, TCBPosKey_FillDerivedVals, 0x007FCC40, NiAnimationKey* keys, uint32_t numKeys, uint8_t size);
GAME_FN(void, __cdecl, TCBPosKey_SaveKey, 0x007FD270, void* stream, const NiTCBPosKey* key);
GAME_FN(void, __cdecl, TCBPosKey_RegisterSupportedFunctions, 0x007FD300, KeyContent content, KeyType type);
GAME_FN(void, __cdecl, StepPosKey_RegisterSupportedFunctions, 0x007FEC60, KeyContent content, KeyType type);

template <typename T>
inline void Set(T* table, uint32_t index, uintptr_t address) {
    table[index] = re::PtrCast<T>(address);
}

constexpr uintptr_t kNothing = 0x0049A650;      // a function doing nothing (returns)
constexpr uintptr_t kReturnZero = 0x00824EC0;   // a function returning 0
constexpr uintptr_t kNoCurvature = 0x00515A50;  // the curvature of keys without one

inline NiAnimationKey* NewKeys(uint32_t count, uint32_t size) {
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(KeyArraySize(count, size)));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    return reinterpret_cast<NiAnimationKey*>(block + 1);
}

// Insert's new array: the keys grown by one with a gap at `at` (NiNew[], the keys before and after
// it copied with memcpy_s)
inline uint8_t* GrowWithGap(const void* old, uint32_t numKeys, uint32_t at, uint32_t size) {
    uint8_t* keys = reinterpret_cast<uint8_t*>(NewKeys(numKeys + 1, size));
    crt::MemcpyS(keys, (numKeys + 1) * size, old, at * size);
    if (numKeys > at) {
        uint32_t rest = (numKeys - at) * size;
        crt::MemcpyS(keys + at * size + size, rest, static_cast<const uint8_t*>(old) + at * size, rest);
    }
    return keys;
}

// ---- NiPosKey

// 0x7F8B30 SaveKey(stream, key): the time, the position
void __cdecl SaveKey(void* stream, const NiPosKey* key) {
    AnimationKey_SaveBinary(stream, key);
    Point3_SaveBinary(&key->pos, stream);
}

// 0x7F8B50 GenInterp(result, time, keys, type, count, last index, size): one key's position; else
// the kind's Interp between the keys around `time` at the normalized time
NiPoint3* __cdecl GenInterp(NiPoint3* result, float time, NiPosKey* keys, KeyType type, uint32_t numKeys,
                            uint32_t* lastIdx, uint8_t size) {
    if (numKeys == 1) {
        *result = keys->pos;
        return result;
    }
    float t0;
    float t1;
    uint32_t next = FindKeyInterval(time, keys, numKeys, *lastIdx, size, t0, t1);
    float t = (time - t0) / (t1 - t0);
    NiPoint3 value;
    ms_interp[KeyIndex(POSKEY, type)](t, KeyAt(keys, *lastIdx, size), KeyAt(keys, next, size), &value);
    *result = value;
    return result;
}

// 0x7F8D80 GenInterpDefault(result, time, keys, type, count, size): the default without keys, the
// first or last position outside their times, else GenInterp from the first key
NiPoint3* __cdecl GenInterpDefault(NiPoint3* result, float time, NiPosKey* keys, KeyType type, uint32_t numKeys,
                                   uint8_t size) {
    if (numKeys == 0) {
        *result = NiPosKey_ms_kDefault;
        return result;
    }
    if (keys->time > time) {
        *result = keys->pos;
        return result;
    }
    NiPosKey* lastKey = KeyAt<NiPosKey>(keys, numKeys - 1, size);
    if (lastKey->time < time) {
        *result = lastKey->pos;
        return result;
    }
    uint32_t lastIdx = 0;
    PosKey_GenInterp(result, time, keys, type, numKeys, &lastIdx, size);
    return result;
}

// 0x7F8E40 Equal: the times and the positions
uint8_t __cdecl Equal(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = AnimationKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    const NiPoint3& a = static_cast<const NiPosKey*>(key0)->pos;
    const NiPoint3& b = static_cast<const NiPosKey*>(key1)->pos;
    return (b.x == a.x && b.y == a.y && b.z == a.z) ? 1 : 0;
}

// 0x7F8EA0 CoordinateFrame(t, key0, key1, type, tangent, normal, binormal, curvature): the Frenet
// frame from the first and second derivatives. Where the curvature is under 1e-6 (or unordered)
// it's 0 and the normal is a perpendicular of the tangent (read back through its pointer)
void __cdecl CoordinateFrame(float t, const NiPosKey* key0, const NiPosKey* key1, KeyType type, NiPoint3* tangent,
                             NiPoint3* normal, NiPoint3* binormal, float* curvature) {
    InterpFunction firstDerivative = ms_interpD1[KeyIndex(POSKEY, type)];
    InterpFunction secondDerivative = ms_interpD2[KeyIndex(POSKEY, type)];
    NiPoint3 d1;
    NiPoint3 d2;
    firstDerivative(t, key0, key1, &d1);
    secondDerivative(t, key0, key1, &d2);
    float squaredLength = (d1.y * d1.y + d1.x * d1.x) + d1.z * d1.z;
    float length = crt::Sqrtf(squaredLength);
    float invLength = 1.0f / length;
    tangent->x = d1.x * invLength;
    tangent->y = d1.y * invLength;
    tangent->z = d1.z * invLength;
    float cx = d2.z * d1.y - d2.y * d1.z;
    float cy = d1.z * d2.x - d2.z * d1.x;
    float cz = d1.x * d2.y - d1.y * d2.x;
    float crossLength = crt::Sqrtf((cy * cy + cx * cx) + cz * cz);
    float k = (crossLength * invLength) * invLength;
    *curvature = k;
    float absK = k < 0.0f ? -k : k;
    if (1e-6f < absK) {
        float dot = (d1.x * d2.x + d1.y * d2.y) + d1.z * d2.z;
        float px = d1.x * dot;
        float py = d1.y * dot;
        float pz = d1.z * dot;
        float ax = d2.x * squaredLength;
        float ay = d2.y * squaredLength;
        float az = d2.z * squaredLength;
        normal->x = ax - px;
        normal->y = ay - py;
        normal->z = az - pz;
    } else {
        *curvature = 0.0f;
        float absX = tangent->x < 0.0f ? -tangent->x : tangent->x;
        float absY = tangent->y < 0.0f ? -tangent->y : tangent->y;
        if (absX > 1e-6f || absY > 1e-6f) {
            normal->x = tangent->y;
            normal->y = -tangent->x;
            normal->z = 0.0f;
        } else {
            normal->z = tangent->y;
            normal->y = -tangent->z;
            normal->x = 0.0f;
        }
    }
    Point3_Unitize(normal);
    float bx = normal->z * tangent->y - tangent->z * normal->y;
    float by = normal->x * tangent->z - normal->z * tangent->x;
    float bz = tangent->x * normal->y - normal->x * tangent->y;
    binormal->x = bx;
    binormal->y = by;
    binormal->z = bz;
}

// 0x7F9180 RegisterSupportedFunctions(content, type): NOINTERP's per-content entries (the zero
// position for every interpolation, no insert)
void __cdecl RegisterSupportedFunctions(KeyContent content, KeyType type) {
    uint32_t i = KeyIndex(content, type);
    Set(ms_curvature, i, kNoCurvature);
    Set(ms_interp, i, 0x007FEA00);
    Set(ms_interpD1, i, 0x007FEA00);
    Set(ms_interpD2, i, 0x007FEA00);
    Set(ms_equal, i, 0x007F8E40);
    Set(ms_fillDerived, i, kNothing);
    Set(ms_insert, i, 0);
}

// 0x7F91E0 Register
uint32_t __cdecl Register() {
    if (NiPosKey_registered != 0) {
        return 0;
    }
    NiPosKey_registered = 1;
    Set(ms_create, 6, kReturnZero);
    Set(ms_save, 6, kNothing);
    Set(ms_copy, 6, kNothing);
    Set(ms_array, 6, kReturnZero);
    Set(ms_delete, 6, kNothing);
    ms_keySizes[6] = 0x10;
    PosKey_RegisterSupportedFunctions(POSKEY, NOINTERP);
    return 1;
}

// 0x7FEA00 Interp (and NOINTERP's derivatives, the linear and step keys' second): zero
void* __cdecl InterpZero(float, const NiAnimationKey*, const NiAnimationKey*, void* result) {
    *static_cast<NiPoint3*>(result) = NiPoint3_ZERO;
    return result;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

// ---- the cubic keys (Bezier and TCB): pos + (tangent + (a + b t) t) t, the tangent the outgoing
// one (Bezier's out, TCB's dd)

inline const NiPoint3& Tangent(const NiBezPosKey* key) {
    return key->outTangent;
}
inline const NiPoint3& Tangent(const NiTCBPosKey* key) {
    return key->dd;
}
inline NiPoint3& IncomingTangent(NiBezPosKey* key) {
    return key->inTangent;
}
inline NiPoint3& IncomingTangent(NiTCBPosKey* key) {
    return key->ds;
}
inline NiPoint3& OutgoingTangent(NiBezPosKey* key) {
    return key->outTangent;
}
inline NiPoint3& OutgoingTangent(NiTCBPosKey* key) {
    return key->dd;
}

// 0x7F9360 / 0x7FC7A0 Interp
template <typename K>
void __cdecl CubicInterp(float t, const NiAnimationKey* key0, const NiAnimationKey*, void* result) {
    auto* key = static_cast<const K*>(key0);
    const NiPoint3& out = Tangent(key);
    float x = key->b.x * t;
    float y = key->b.y * t;
    float z = key->b.z * t;
    x = key->a.x + x;
    y = key->a.y + y;
    z = key->a.z + z;
    x = x * t;
    y = y * t;
    z = z * t;
    x = out.x + x;
    y = out.y + y;
    z = out.z + z;
    x = x * t;
    y = y * t;
    z = z * t;
    auto* value = static_cast<NiPoint3*>(result);
    value->x = key->pos.x + x;
    value->y = key->pos.y + y;
    value->z = key->pos.z + z;
}

// 0x7F9440 / 0x7FC880 InterpD1: tangent + (2a + 3t b) t
template <typename K>
void __cdecl CubicInterpD1(float t, const NiAnimationKey* key0, const NiAnimationKey*, void* result) {
    auto* key = static_cast<const K*>(key0);
    const NiPoint3& out = Tangent(key);
    float t3 = t * 3.0f;
    float bx = key->b.x * t3;
    float by = key->b.y * t3;
    float bz = t3 * key->b.z;
    float ax = key->a.x * 2.0f;
    float ay = key->a.y * 2.0f;
    float az = 2.0f * key->a.z;
    float x = (ax + bx) * t;
    float y = (ay + by) * t;
    float z = (az + bz) * t;
    auto* value = static_cast<NiPoint3*>(result);
    value->x = out.x + x;
    value->y = out.y + y;
    value->z = out.z + z;
}

// 0x7F9520 / 0x7FC960 InterpD2: 2a + 6t b
template <typename K>
void __cdecl CubicInterpD2(float t, const NiAnimationKey* key0, const NiAnimationKey*, void* result) {
    auto* key = static_cast<const K*>(key0);
    float t6 = t * 6.0f;
    float bx = key->b.x * t6;
    float by = key->b.y * t6;
    float bz = t6 * key->b.z;
    float ax = key->a.x * 2.0f;
    float ay = key->a.y * 2.0f;
    float az = 2.0f * key->a.z;
    auto* value = static_cast<NiPoint3*>(result);
    value->x = ax + bx;
    value->y = by + ay;
    value->z = az + bz;
}

// the cubic coefficients of each key's segment from the tangents (the keys taken as sizeof(K)
// apart): a = 3 (p1 - p0) - (2 out0 + in1), b = (out0 + in1) - 2 (p1 - p0)
template <typename K>
void CubicCoefficients(K* keys, uint32_t numKeys) {
    uint32_t count = numKeys - 1;
    if (count == 0) {
        return;
    }
    K* key0 = keys;
    do {
        K* key1 = key0 + 1;
        const NiPoint3& out0 = OutgoingTangent(key0);
        const NiPoint3& in1 = IncomingTangent(key1);
        float ux = out0.x * 2.0f;
        float uy = out0.y * 2.0f;
        float uz = out0.z * 2.0f;
        float vx = in1.x + ux;
        float vy = in1.y + uy;
        float vz = in1.z + uz;
        float dx = key1->pos.x - key0->pos.x;
        float dy = key1->pos.y - key0->pos.y;
        float dz = key1->pos.z - key0->pos.z;
        key0->a.x = dx * 3.0f - vx;
        key0->a.y = dy * 3.0f - vy;
        key0->a.z = dz * 3.0f - vz;
        dx = key1->pos.x - key0->pos.x;
        dy = key1->pos.y - key0->pos.y;
        dz = key1->pos.z - key0->pos.z;
        float wx = in1.x + out0.x;
        float wy = in1.y + out0.y;
        float wz = in1.z + out0.z;
        key0->b.x = wx - dx * 2.0f;
        key0->b.y = wy - dy * 2.0f;
        key0->b.z = wz - dz * 2.0f;
        key0 = key1;
    } while (--count != 0);
}

// 0x7F9740 / 0x7FCF00 ComputeMaxCurvature(keys, count): |d1 x d2| / |d1|^2 sampled at
// ms_iSampleIterations + 1 points of each segment (the derivatives through the game's addresses)
template <typename K, uintptr_t D1, uintptr_t D2>
float __cdecl CubicMaxCurvature(NiAnimationKey* keys, uint32_t numKeys) {
    using Derivative = void(__cdecl*)(float, const NiAnimationKey*, const NiAnimationKey*, void*);
    float maxCurvature = 0.0f;
    uint32_t count = numKeys - 1;
    if (count == 0) {
        return maxCurvature;
    }
    K* key0 = static_cast<K*>(keys);
    do {
        K* key1 = key0 + 1;
        for (int32_t i = 0; i <= NiPosKey_ms_iSampleIterations; i++) {
            float t = float(i) * NiPosKey_ms_fSampleDelta;
            NiPoint3 d1;
            NiPoint3 d2;
            reinterpret_cast<Derivative>(D1)(t, key0, key1, &d1);
            reinterpret_cast<Derivative>(D2)(t, key0, key1, &d2);
            float cx = d1.y * d2.z - d1.z * d2.y;
            float cy = d1.z * d2.x - d1.x * d2.z;
            float cz = d1.x * d2.y - d1.y * d2.x;
            float crossLength = crt::Sqrtf((cx * cx + cy * cy) + cz * cz);
            float squaredLength = (d1.x * d1.x + d1.y * d1.y) + d1.z * d1.z;
            float curvature = crossLength / squaredLength;
            if (maxCurvature < curvature) {
                maxCurvature = curvature;
            }
        }
        key0 = key1;
    } while (--count != 0);
    return maxCurvature;
}

// ---- NiBezPosKey

// 0x7F92B0 Equal: the position key's, the incoming tangent, the outgoing one
uint8_t __cdecl BezEqual(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = PosKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    auto* a = static_cast<const NiBezPosKey*>(key0);
    auto* b = static_cast<const NiBezPosKey*>(key1);
    if (b->inTangent.x != a->inTangent.x || b->inTangent.y != a->inTangent.y || b->inTangent.z != a->inTangent.z) {
        return 0;
    }
    return Point3_Equal(&a->outTangent, &b->outTangent) != 0 ? 1 : 0;
}

// 0x7F9320 NewArray(count)
NiAnimationKey* __cdecl BezNewArray(uint32_t count) {
    return NewKeys(count, 0x40);
}

// 0x7F95C0 FillDerivedVals(keys, count, size)
void __cdecl BezFillDerivedVals(NiAnimationKey* keys, uint32_t numKeys, uint8_t) {
    CubicCoefficients(static_cast<NiBezPosKey*>(keys), numKeys);
}

// 0x7F9940 SaveBinary(stream, keys, count): each key, its tangents
void __cdecl BezSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiBezPosKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        PosKey_SaveKey(stream, key);
        Point3_SaveBinary(&key->inTangent, stream);
        Point3_SaveBinary(&key->outTangent, stream);
    }
}

// 0x7F9980 Insert(time, keys, count): a key at `time` with the curve's position there (none where
// one is), zero tangents; between two keys the curve split there (their tangents rescaled). The
// coefficients made again
uint8_t __cdecl BezInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    auto* old = static_cast<NiBezPosKey*>(*keysRef);
    uint32_t at;
    if (AnimationKey_OkayToInsert(time, old, *numKeys, &at, 0x40) == 0) {
        return 0;
    }
    auto* keys = reinterpret_cast<NiBezPosKey*>(GrowWithGap(old, *numKeys, at, 0x40));
    NiPoint3 value;
    PosKey_GenInterpDefault(&value, time, old, BEZKEY, *numKeys, 0x40);
    NiBezPosKey* key = &keys[at];
    key->time = time;
    key->pos = value;
    key->inTangent = NiPoint3_ZERO;
    key->outTangent = NiPoint3_ZERO;
    if (at != 0 && at != *numKeys) {
        NiBezPosKey* prev = &keys[at - 1];
        NiBezPosKey* next = &keys[at + 1];
        float prevTime = prev->time;
        float nextTime = next->time;
        NiPoint3 prevOut = prev->outTangent;
        NiPoint3 nextIn = next->inTangent;
        NiPoint3 newIn;
        NiPoint3 newOut;
        AnimationKey_SplitBezierTangents(prev->pos.x, prevTime, &prevOut.x, next->pos.x, nextTime, &nextIn.x, time,
                                         value.x, &newIn.x, &newOut.x);
        AnimationKey_SplitBezierTangents(prev->pos.y, prevTime, &prevOut.y, next->pos.y, nextTime, &nextIn.y, time,
                                         value.y, &newIn.y, &newOut.y);
        AnimationKey_SplitBezierTangents(prev->pos.z, prevTime, &prevOut.z, next->pos.z, nextTime, &nextIn.z, time,
                                         value.z, &newIn.z, &newOut.z);
        prev->outTangent = prevOut;
        key->inTangent = newIn;
        key->outTangent = newOut;
        next->inTangent = nextIn;
    }
    (*numKeys)++;
    if (old != nullptr) {
        NiDeleteArray(reinterpret_cast<uint8_t*>(old) - 4, 0x40);
    }
    *keysRef = keys;
    BezPosKey_FillDerivedVals(keys, *numKeys, 0x40);
    return 1;
}

// 0x7F9D00 RegisterSupportedFunctions(content, type)
void __cdecl BezRegisterSupportedFunctions(KeyContent content, KeyType type) {
    uint32_t i = KeyIndex(content, type);
    Set(ms_curvature, i, 0x007F9740);
    Set(ms_interp, i, 0x007F9360);
    Set(ms_interpD1, i, 0x007F9440);
    Set(ms_interpD2, i, 0x007F9520);
    Set(ms_equal, i, 0x007F92B0);
    Set(ms_fillDerived, i, 0x007F95C0);
    Set(ms_insert, i, 0x007F9980);
}

// 0x7F9D60 Register
uint32_t __cdecl BezRegister() {
    if (NiBezPosKey_registered != 0) {
        return 0;
    }
    NiBezPosKey_registered = 1;
    Set(ms_create, 8, 0x007F98C0);
    Set(ms_save, 8, 0x007F9940);
    Set(ms_copy, 8, 0x007F9240);
    Set(ms_array, 8, 0x007F9320);
    Set(ms_delete, 8, 0x007FD8E0);
    ms_keySizes[8] = 0x40;
    BezPosKey_RegisterSupportedFunctions(POSKEY, BEZKEY);
    return 1;
}

// ---- NiLinPosKey (Equal a lone jump to NiPosKey's; NewArray, SaveBinary shared with the step
// keys, NewArray with NiBezFloatKey too)

// 0x7FC020 NewArray(count)
NiAnimationKey* __cdecl LinNewArray(uint32_t count) {
    return NewKeys(count, 0x10);
}

// 0x7FC060 Interp: (1 - t) p0 + t p1
void* __cdecl LinInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    const NiPoint3& p0 = static_cast<const NiPosKey*>(key0)->pos;
    const NiPoint3& p1 = static_cast<const NiPosKey*>(key1)->pos;
    float s = 1.0f - t;
    float ax = p0.x * s;
    float ay = p0.y * s;
    float az = s * p0.z;
    float bx = p1.x * t;
    float by = p1.y * t;
    float bz = t * p1.z;
    auto* value = static_cast<NiPoint3*>(result);
    value->x = bx + ax;
    value->y = ay + by;
    value->z = bz + az;
    return result;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

// 0x7FC100 InterpD1: p1 - p0
void* __cdecl LinInterpD1(float, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    const NiPoint3& p0 = static_cast<const NiPosKey*>(key0)->pos;
    const NiPoint3& p1 = static_cast<const NiPosKey*>(key1)->pos;
    auto* value = static_cast<NiPoint3*>(result);
    float x = p1.x - p0.x;
    float y = p1.y - p0.y;
    float z = p1.z - p0.z;
    value->x = x;
    value->y = y;
    value->z = z;
    return result;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

// Insert for the 16-byte keys: a key at `time` with the interpolated position (none where one is)
inline uint8_t InsertPosKey(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys, KeyType type) {
    auto* old = static_cast<NiPosKey*>(*keysRef);
    uint32_t at;
    if (AnimationKey_OkayToInsert(time, old, *numKeys, &at, 0x10) == 0) {
        return 0;
    }
    auto* keys = reinterpret_cast<NiPosKey*>(GrowWithGap(old, *numKeys, at, 0x10));
    NiPoint3 value;
    PosKey_GenInterpDefault(&value, time, old, type, *numKeys, 0x10);
    keys[at].time = time;
    keys[at].pos = value;
    (*numKeys)++;
    if (old != nullptr) {
        NiDeleteArray(reinterpret_cast<uint8_t*>(old) - 4, 0x10);
    }
    *keysRef = keys;
    return 1;
}

// 0x7FC170 Insert
uint8_t __cdecl LinInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertPosKey(time, keysRef, numKeys, LINKEY);
}

// 0x7FC290 RegisterSupportedFunctions(content, type)
void __cdecl LinRegisterSupportedFunctions(KeyContent content, KeyType type) {
    uint32_t i = KeyIndex(content, type);
    Set(ms_curvature, i, kNoCurvature);
    Set(ms_interp, i, 0x007FC060);
    Set(ms_interpD1, i, 0x007FC100);
    Set(ms_interpD2, i, 0x007FEA00);
    Set(ms_equal, i, 0x007FEA20);
    Set(ms_fillDerived, i, kNothing);
    Set(ms_insert, i, 0x007FC170);
}

// 0x7FC2F0 Register
uint32_t __cdecl LinRegister() {
    if (NiLinPosKey_registered != 0) {
        return 0;
    }
    NiLinPosKey_registered = 1;
    Set(ms_create, 7, 0x007FEAB0);
    Set(ms_save, 7, 0x007FEB10);
    Set(ms_copy, 7, 0x007FBFF0);
    Set(ms_array, 7, 0x007FC020);
    Set(ms_delete, 7, 0x007FC150);
    ms_keySizes[7] = 0x10;
    LinPosKey_RegisterSupportedFunctions(POSKEY, LINKEY);
    return 1;
}

// 0x7FEB10 SaveBinary(stream, keys, count)
void __cdecl LinSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiPosKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        PosKey_SaveKey(stream, key);
    }
}

// ---- NiTCBPosKey

// 0x7FC6C0 Equal: the position key's, tension, continuity, bias, then the tangents and
// coefficients
uint8_t __cdecl TCBEqual(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = PosKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    auto* a = static_cast<const NiTCBPosKey*>(key0);
    auto* b = static_cast<const NiTCBPosKey*>(key1);
    if (b->tension != a->tension || b->continuity != a->continuity || b->bias != a->bias) {
        return 0;
    }
    if (Point3_Equal(&a->ds, &b->ds) == 0 || Point3_Equal(&a->dd, &b->dd) == 0 || Point3_Equal(&a->a, &b->a) == 0 ||
        Point3_Equal(&a->b, &b->b) == 0) {
        return 0;
    }
    return 1;
}

// 0x7FC760 NewArray(count)
NiAnimationKey* __cdecl TCBNewArray(uint32_t count) {
    return NewKeys(count, 0x4C);
}

// 0x7FCA00 CalculateDVals(previous point, next point, previous span, next span): the key's
// incoming and outgoing tangents from its tension, continuity and bias, scaled by the spans
void __fastcall TCBCalculateDVals(NiTCBPosKey* self, re::Edx, const NiPoint3* pointIn, const NiPoint3* pointOut,
                                  float preLength, float nextLength) {
    float inX = self->pos.x - pointIn->x;
    float inY = self->pos.y - pointIn->y;
    float inZ = self->pos.z - pointIn->z;
    float outX = pointOut->x - self->pos.x;
    float outY = pointOut->y - self->pos.y;
    float outZ = pointOut->z - self->pos.z;
    float onePlusBias = self->bias + 1.0f;
    float oneMinusBias = 1.0f - self->bias;
    float half = (1.0f - self->tension) * 0.5f;
    float onePlusContinuity = self->continuity + 1.0f;
    float halfPlus = onePlusContinuity * half;
    float oneMinusContinuity = 1.0f - self->continuity;
    float halfMinus = half * oneMinusContinuity;
    // ds = (p - in) halfMinus (1 + b) + (out - p) halfPlus (1 - b)
    float k1 = halfPlus * oneMinusBias;
    float t1x = outX * k1;
    float t1y = outY * k1;
    float t1z = k1 * outZ;
    float k2 = halfMinus * onePlusBias;
    float t2x = k2 * inX;
    float t2y = inY * k2;
    float t2z = k2 * inZ;
    self->ds.x = t2x + t1x;
    self->ds.y = t2y + t1y;
    self->ds.z = t2z + t1z;
    // dd = (p - in) halfPlus (1 + b) + (out - p) halfMinus (1 - b)
    float k3 = oneMinusBias * halfMinus;
    float u1x = outX * k3;
    float u1y = outY * k3;
    float u1z = k3 * outZ;
    float k4 = halfPlus * onePlusBias;
    float u2x = inX * k4;
    float u2y = inY * k4;
    float u2z = k4 * inZ;
    self->dd.x = u2x + u1x;
    self->dd.y = u2y + u1y;
    self->dd.z = u2z + u1z;
    float invSum = 2.0f / (preLength + nextLength);
    float inScale = preLength * invSum;
    self->ds.x = self->ds.x * inScale;
    self->ds.y = self->ds.y * inScale;
    self->ds.z = inScale * self->ds.z;
    float outScale = invSum * nextLength;
    self->dd.x = self->dd.x * outScale;
    self->dd.y = self->dd.y * outScale;
    self->dd.z = outScale * self->dd.z;
}

// 0x7FCC40 FillDerivedVals(keys, count, size): each key's tangents from its neighbours (the ends
// against a point mirrored through them, spans of 1), then the coefficients. The keys taken as
// 0x4C bytes apart
void __cdecl TCBFillDerivedVals(NiAnimationKey* keys, uint32_t numKeys, uint8_t) {
    auto* key = static_cast<NiTCBPosKey*>(keys);
    uint32_t last = numKeys - 1;
    if (numKeys >= 2) {
        NiPoint3 in;
        float px = key[0].pos.x * 2.0f;
        float py = key[0].pos.y * 2.0f;
        float pz = 2.0f * key[0].pos.z;
        in.x = px - key[1].pos.x;
        in.y = py - key[1].pos.y;
        in.z = pz - key[1].pos.z;
        TCBPosKey_CalculateDVals(&key[0], &in, &key[1].pos, 1.0f, 1.0f);
        for (uint32_t i = 1; i < last; i++) {
            float nextLength = key[i + 1].time - key[i].time;
            float preLength = key[i].time - key[i - 1].time;
            TCBPosKey_CalculateDVals(&key[i], &key[i - 1].pos, &key[i + 1].pos, preLength, nextLength);
        }
        NiPoint3 out;
        float qx = key[last].pos.x * 2.0f;
        float qy = key[last].pos.y * 2.0f;
        float qz = 2.0f * key[last].pos.z;
        out.x = qx - key[last - 1].pos.x;
        out.y = qy - key[last - 1].pos.y;
        out.z = qz - key[last - 1].pos.z;
        TCBPosKey_CalculateDVals(&key[last], &key[last - 1].pos, &out, 1.0f, 1.0f);
    }
    if (last != 0) {
        CubicCoefficients(key, numKeys);
    }
}

// 0x7FD0A0 Insert(time, keys, count): a key at `time` with the curve's position (none where one
// is), tension, continuity and bias 0; the tangents and coefficients made again
uint8_t __cdecl TCBInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    auto* old = static_cast<NiTCBPosKey*>(*keysRef);
    uint32_t at;
    if (AnimationKey_OkayToInsert(time, old, *numKeys, &at, 0x4C) == 0) {
        return 0;
    }
    auto* keys = reinterpret_cast<NiTCBPosKey*>(GrowWithGap(old, *numKeys, at, 0x4C));
    NiPoint3 value;
    PosKey_GenInterpDefault(&value, time, old, TCBKEY, *numKeys, 0x4C);
    NiTCBPosKey* key = &keys[at];
    key->time = time;
    key->pos = value;
    key->tension = 0.0f;
    key->continuity = 0.0f;
    key->bias = 0.0f;
    (*numKeys)++;
    if (old != nullptr) {
        NiDeleteArray(reinterpret_cast<uint8_t*>(old) - 4, 0x4C);
    }
    *keysRef = keys;
    TCBPosKey_FillDerivedVals(keys, *numKeys, 0x4C);
    return 1;
}

// 0x7FD270 SaveKey(stream, key): the position key's, tension, continuity, bias
void __cdecl TCBSaveKey(void* stream, const NiTCBPosKey* key) {
    PosKey_SaveKey(stream, key);
    ni::scene::SaveBytes(stream, &key->tension, 4);
    ni::scene::SaveBytes(stream, &key->continuity, 4);
    ni::scene::SaveBytes(stream, &key->bias, 4);
}

// 0x7FD300 RegisterSupportedFunctions(content, type)
void __cdecl TCBRegisterSupportedFunctions(KeyContent content, KeyType type) {
    uint32_t i = KeyIndex(content, type);
    Set(ms_curvature, i, 0x007FCF00);
    Set(ms_interp, i, 0x007FC7A0);
    Set(ms_interpD1, i, 0x007FC880);
    Set(ms_interpD2, i, 0x007FC960);
    Set(ms_equal, i, 0x007FC6C0);
    Set(ms_fillDerived, i, 0x007FCC40);
    Set(ms_insert, i, 0x007FD0A0);
}

// 0x7FD3C0 SaveBinary(stream, keys, count)
void __cdecl TCBSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiTCBPosKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        TCBPosKey_SaveKey(stream, key);
    }
}

// 0x7FD3F0 Register
uint32_t __cdecl TCBRegister() {
    if (NiTCBPosKey_registered != 0) {
        return 0;
    }
    NiTCBPosKey_registered = 1;
    Set(ms_create, 9, 0x007FD360);
    Set(ms_save, 9, 0x007FD3C0);
    Set(ms_copy, 9, 0x007FC640);
    Set(ms_array, 9, 0x007FC760);
    Set(ms_delete, 9, 0x007FD080);
    ms_keySizes[9] = 0x4C;
    TCBPosKey_RegisterSupportedFunctions(POSKEY, TCBKEY);
    return 1;
}

// ---- NiStepPosKey

// 0x7FEA30 Interp: the first key's position before t = 1 (an unordered t takes the second's)
void* __cdecl StepInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    const NiAnimationKey* key = t < 1.0f ? key0 : key1;
    *static_cast<NiPoint3*>(result) = static_cast<const NiPosKey*>(key)->pos;
    return result;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

// 0x7FEA60 InterpD1: p0 - p1 (the reverse of the linear key's)
void* __cdecl StepInterpD1(float, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    const NiPoint3& p0 = static_cast<const NiPosKey*>(key0)->pos;
    const NiPoint3& p1 = static_cast<const NiPosKey*>(key1)->pos;
    auto* value = static_cast<NiPoint3*>(result);
    float x = p0.x - p1.x;
    float y = p0.y - p1.y;
    float z = p0.z - p1.z;
    value->x = x;
    value->y = y;
    value->z = z;
    return result;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

// 0x7FEB40 Insert
uint8_t __cdecl StepInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertPosKey(time, keysRef, numKeys, STEPKEY);
}

// 0x7FEC60 RegisterSupportedFunctions(content, type)
void __cdecl StepRegisterSupportedFunctions(KeyContent content, KeyType type) {
    uint32_t i = KeyIndex(content, type);
    Set(ms_curvature, i, kNoCurvature);
    Set(ms_interp, i, 0x007FEA30);
    Set(ms_interpD1, i, 0x007FEA60);
    Set(ms_interpD2, i, 0x007FEA00);
    Set(ms_equal, i, 0x007FEA20);
    Set(ms_fillDerived, i, kNothing);
    Set(ms_insert, i, 0x007FEB40);
}

// 0x7FECC0 Register
uint32_t __cdecl StepRegister() {
    if (NiStepPosKey_registered != 0) {
        return 0;
    }
    NiStepPosKey_registered = 1;
    Set(ms_create, 11, 0x007FEAB0);
    Set(ms_save, 11, 0x007FEB10);
    Set(ms_copy, 11, 0x007FBFF0);
    Set(ms_array, 11, 0x007FC020);
    Set(ms_delete, 11, 0x007FC150);
    ms_keySizes[11] = 0x10;
    StepPosKey_RegisterSupportedFunctions(POSKEY, STEPKEY);
    return 1;
}

} // namespace

// 0x7FEA20: jmp 0x7F8E40 (NiLinPosKey's and NiStepPosKey's Equal are NiPosKey's)
RE_UNHOOKED("NiLinPosKey::Equal", 0x007FEA20);

} // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIPOSKEY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

using namespace ni::anim;

NIPOSKEY_REVERSED("NiPosKey::SaveKey", 0x007F8B30, SaveKey);
NIPOSKEY_REVERSED("NiPosKey::GenInterp", 0x007F8B50, GenInterp);
NIPOSKEY_REVERSED("NiPosKey::GenInterpDefault", 0x007F8D80, GenInterpDefault);
NIPOSKEY_REVERSED("NiPosKey::Equal", 0x007F8E40, Equal);
RE_UNHOOKED_FN("NiLinPosKey::Equal", 0x007FEA20, &Equal);   // (a lone jmp to it: NiStepPosKey's too)
NIPOSKEY_REVERSED("NiPosKey::CoordinateFrame", 0x007F8EA0, CoordinateFrame);
NIPOSKEY_REVERSED("NiPosKey::RegisterSupportedFunctions", 0x007F9180, RegisterSupportedFunctions);
NIPOSKEY_REVERSED("NiPosKey::Register", 0x007F91E0, Register);
NIPOSKEY_REVERSED("NiBezPosKey::Equal", 0x007F92B0, BezEqual);
NIPOSKEY_REVERSED("NiBezPosKey::NewArray", 0x007F9320, BezNewArray);
NIPOSKEY_REVERSED("NiBezPosKey::Interp", 0x007F9360, CubicInterp<NiBezPosKey>);
NIPOSKEY_REVERSED("NiBezPosKey::InterpD1", 0x007F9440, CubicInterpD1<NiBezPosKey>);
NIPOSKEY_REVERSED("NiBezPosKey::InterpD2", 0x007F9520, CubicInterpD2<NiBezPosKey>);
NIPOSKEY_REVERSED("NiBezPosKey::FillDerivedVals", 0x007F95C0, BezFillDerivedVals);
NIPOSKEY_REVERSED("NiBezPosKey::ComputeMaxCurvature", 0x007F9740,
                  (CubicMaxCurvature<NiBezPosKey, 0x007F9440, 0x007F9520>));
NIPOSKEY_REVERSED("NiBezPosKey::SaveBinary", 0x007F9940, BezSaveBinary);
NIPOSKEY_REVERSED("NiBezPosKey::Insert", 0x007F9980, BezInsert);
NIPOSKEY_REVERSED("NiBezPosKey::RegisterSupportedFunctions", 0x007F9D00, BezRegisterSupportedFunctions);
NIPOSKEY_REVERSED("NiBezPosKey::Register", 0x007F9D60, BezRegister);
NIPOSKEY_REVERSED("NiLinPosKey::NewArray", 0x007FC020, LinNewArray);
NIPOSKEY_REVERSED("NiLinPosKey::Interp", 0x007FC060, LinInterp);
NIPOSKEY_REVERSED("NiLinPosKey::InterpD1", 0x007FC100, LinInterpD1);
NIPOSKEY_REVERSED("NiLinPosKey::Insert", 0x007FC170, LinInsert);
NIPOSKEY_REVERSED("NiLinPosKey::RegisterSupportedFunctions", 0x007FC290, LinRegisterSupportedFunctions);
NIPOSKEY_REVERSED("NiLinPosKey::Register", 0x007FC2F0, LinRegister);
NIPOSKEY_REVERSED("NiTCBPosKey::Equal", 0x007FC6C0, TCBEqual);
NIPOSKEY_REVERSED("NiTCBPosKey::NewArray", 0x007FC760, TCBNewArray);
NIPOSKEY_REVERSED("NiTCBPosKey::Interp", 0x007FC7A0, CubicInterp<NiTCBPosKey>);
NIPOSKEY_REVERSED("NiTCBPosKey::InterpD1", 0x007FC880, CubicInterpD1<NiTCBPosKey>);
NIPOSKEY_REVERSED("NiTCBPosKey::InterpD2", 0x007FC960, CubicInterpD2<NiTCBPosKey>);
NIPOSKEY_REVERSED("NiTCBPosKey::CalculateDVals", 0x007FCA00, TCBCalculateDVals);
NIPOSKEY_REVERSED("NiTCBPosKey::FillDerivedVals", 0x007FCC40, TCBFillDerivedVals);
NIPOSKEY_REVERSED("NiTCBPosKey::ComputeMaxCurvature", 0x007FCF00,
                  (CubicMaxCurvature<NiTCBPosKey, 0x007FC880, 0x007FC960>));
NIPOSKEY_REVERSED("NiTCBPosKey::Insert", 0x007FD0A0, TCBInsert);
NIPOSKEY_REVERSED("NiTCBPosKey::SaveKey", 0x007FD270, TCBSaveKey);
NIPOSKEY_REVERSED("NiTCBPosKey::RegisterSupportedFunctions", 0x007FD300, TCBRegisterSupportedFunctions);
NIPOSKEY_REVERSED("NiTCBPosKey::SaveBinary", 0x007FD3C0, TCBSaveBinary);
NIPOSKEY_REVERSED("NiTCBPosKey::Register", 0x007FD3F0, TCBRegister);
NIPOSKEY_REVERSED("NiPosKey::Interp", 0x007FEA00, InterpZero);
NIPOSKEY_REVERSED("NiStepPosKey::Interp", 0x007FEA30, StepInterp);
NIPOSKEY_REVERSED("NiStepPosKey::InterpD1", 0x007FEA60, StepInterpD1);
NIPOSKEY_REVERSED("NiLinPosKey::SaveBinary", 0x007FEB10, LinSaveBinary);
NIPOSKEY_REVERSED("NiStepPosKey::Insert", 0x007FEB40, StepInsert);
NIPOSKEY_REVERSED("NiStepPosKey::RegisterSupportedFunctions", 0x007FEC60, StepRegisterSupportedFunctions);
NIPOSKEY_REVERSED("NiStepPosKey::Register", 0x007FECC0, StepRegister);
