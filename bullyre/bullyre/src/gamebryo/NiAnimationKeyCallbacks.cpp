// The animation keys' table functions the Register functions store (src/gamebryo/NiAnimationKey.h:
// the per content * 6 + type tables at 0x20E4390..): each kind's Copy, Interp, Insert,
// CreateFromStream (the keys of a stream) and DeleteArray, with the loaders, the clamped
// GenInterpDefault of the float, rotation and colour keys and the two float curves the float
// kinds share (the linear and the cubic one). Many are shared by several kinds (folded), noted
// at each. Also NiQuaternion::Squad (the Bezier and TCB rotation keys').
//
// Main-thread float maths at the game's 24-bit x87 precision: each operation a float one, in the
// game's order. Where the game copies a float through the x87 stack (fld / fstp) it comes out
// the same but for a signalling NaN, made quiet: X87Copy does that; dword moves stay moves.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiAnimationKey.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

GAME_VAR(float, NiFloatKey_ms_fDefault, 0x020E4314);
GAME_VAR(NiQuaternion, NiRotKey_ms_kDefault, 0x020E4330);
GAME_VAR(NiColorA, NiColorKey_ms_kDefault, 0x020E4344);
GAME_VAR(NiColorA, NiColorA_Default, 0x00BC75D4);   // (0, 0, 0, 1)

constexpr uintptr_t kRotKeyConstruct = 0x007FC360;     // NiRotKey::NiRotKey
constexpr uintptr_t kColorKeyConstruct = 0x007FE610;   // NiColorKey::NiColorKey
constexpr uintptr_t kEulerRotKeyConstruct = 0x007FB190;

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(uint8_t, __cdecl, AnimationKey_OkayToInsert, 0x007FF770, float time, const NiAnimationKey* keys,
        uint32_t numKeys, uint32_t* index, uint8_t size);
GAME_FN(void, __cdecl, AnimationKey_SplitBezierTangents, 0x007FFCD0, float value0, float time0, float* outTangent0,
        float value1, float time1, float* inTangent1, float time, float value, float* inTangent, float* outTangent);
GAME_FN(float, __cdecl, FloatKey_GenInterp, 0x007F8300, float time, NiFloatKey* keys, KeyType type,
        uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(NiQuaternion*, __cdecl, RotKey_GenInterp, 0x007F9E90, NiQuaternion* result, float time, NiRotKey* keys,
        KeyType type, uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(NiColorA*, __cdecl, ColorKey_GenInterp, 0x007FADC0, NiColorA* result, float time, NiAnimationKey* keys,
        KeyType type, uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(void, __cdecl, RotKey_FillDerivedVals, 0x007FA1F0, NiAnimationKey* keys, uint32_t numKeys, uint8_t size);
GAME_FN(void, __cdecl, BezRotKey_FillDerivedVals, 0x007FA520, NiAnimationKey* keys, uint32_t numKeys, uint8_t size);
GAME_FN(void, __cdecl, TCBRotKey_FillDerivedVals, 0x007FD790, NiAnimationKey* keys, uint32_t numKeys, uint8_t size);
GAME_FN(void, __cdecl, TCBFloatKey_FillDerivedVals, 0x007FDEF0, NiAnimationKey* keys, uint32_t numKeys,
        uint8_t size);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Construct, 0x00775B90, NiQuaternion* self, float w, float x, float y,
        float z);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_Slerp, 0x007766E0, NiQuaternion* result, float t, const NiQuaternion* p,
        const NiQuaternion* q);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Float4_LoadBinary, 0x007625D0, void* self, void* stream);   // NiColorA::LoadBinary

// this file's, through their addresses
GAME_FN(NiQuaternion*, __cdecl, Quaternion_Squad_, 0x00775D00, NiQuaternion* result, float t, const NiQuaternion* p,
        const NiQuaternion* a, const NiQuaternion* b, const NiQuaternion* q);
GAME_FN(float, __cdecl, FloatKey_GenInterpDefault_, 0x007F8500, float time, NiFloatKey* keys, KeyType type,
        uint32_t numKeys, uint8_t size);
GAME_FN(NiQuaternion*, __cdecl, RotKey_GenInterpDefault_, 0x007FA120, NiQuaternion* result, float time,
        NiRotKey* keys, KeyType type, uint32_t numKeys, uint8_t size);
GAME_FN(NiColorA*, __cdecl, ColorKey_GenInterpDefault_, 0x007FB010, NiColorA* result, float time,
        NiColorKey* keys, KeyType type, uint32_t numKeys, uint8_t size);
GAME_FN(float, __cdecl, LinearInterp_, 0x007FFC40, float t, float value0, float value1);
GAME_FN(float, __cdecl, CubicInterp_, 0x007FFC60, float t, float value0, float outTangent0, float value1,
        float inTangent1);
GAME_FN(void, __thiscall, AnimationKey_LoadBinary_, 0x007FFBE0, NiAnimationKey* self, void* stream);
GAME_FN(void, __thiscall, FloatKey_LoadBinary_, 0x007F8610, NiFloatKey* self, void* stream);
GAME_FN(void, __thiscall, PosKey_LoadBinary_, 0x007F8B10, NiPosKey* self, void* stream);
GAME_FN(void, __thiscall, RotKey_LoadBinary_, 0x007F9E00, NiRotKey* self, void* stream);
GAME_FN(void, __thiscall, BoolKey_LoadBinary_, 0x007FACB0, NiBoolKey* self, void* stream);
GAME_FN(void, __thiscall, TCBFloatKey_LoadBinary_, 0x007FE150, NiTCBFloatKey* self, void* stream);
GAME_FN(void, __thiscall, TCBPosKey_LoadBinary_, 0x007FD1E0, NiTCBPosKey* self, void* stream);
GAME_FN(void, __thiscall, TCBRotKey_LoadBinary_, 0x007FDA50, NiTCBRotKey* self, void* stream);
GAME_FN(void, __thiscall, EulerRotKey_LoadBinary_, 0x007FB640, NiEulerRotKey* self, void* stream);

// a float copied through the x87 stack (fld / fstp): a signalling NaN comes out quiet
inline void X87Copy(void* to, const void* from) {
    uint32_t bits;
    std::memcpy(&bits, from, 4);
    if ((bits & 0x7F800000u) == 0x7F800000u && (bits & 0x007FFFFFu) != 0) {
        bits |= 0x00400000u;
    }
    std::memcpy(to, &bits, 4);
}
inline float X87(float value) {
    float out;
    X87Copy(&out, &value);
    return out;
}

// dwords moved
inline void Move(void* to, const void* from, uint32_t bytes) {
    std::memcpy(to, from, bytes);
}

// NiNew[] of count keys `size` bytes each, the count before them (no constructor). The address
// arithmetic stays when it fails (the game's code then writes through null)
inline uint8_t* NewKeys(uint32_t count, uint32_t size) {
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(KeyArraySize(count, size)));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    return reinterpret_cast<uint8_t*>(block + 1);
}

// ... each constructed (the vector constructor)
inline uint8_t* NewConstructedKeys(uint32_t count, uint32_t size, uintptr_t construct) {
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(KeyArraySize(count, size)));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, size, count, reinterpret_cast<void*>(construct));
    return reinterpret_cast<uint8_t*>(block + 1);
}

// the keys copied into a new array one longer, a gap at `at`
inline void CopyWithGap(uint8_t* keys, const void* old, uint32_t numKeys, uint32_t at, uint32_t size) {
    crt::MemcpyS(keys, (numKeys + 1) * size, old, at * size);
    if (numKeys > at) {
        uint32_t rest = (numKeys - at) * size;
        crt::MemcpyS(keys + at * size + size, rest, static_cast<const uint8_t*>(old) + at * size, rest);
    }
}

inline void DeleteKeys(NiAnimationKey* keys, uint32_t size) {
    if (keys != nullptr) {
        NiDeleteArray(reinterpret_cast<uint8_t*>(keys) - 4, size);
    }
}

// the key at an address (the arrays' keys `size` apart, from a null base when allocation failed)
template <typename T>
inline T* KeyAtAddress(uint8_t* base, uint32_t index, uint32_t size) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) + index * size);
}

// ---- the shared pieces

// 0x7FFBE0 NiAnimationKey::LoadBinary(stream): the time
void __fastcall AnimationKeyLoadBinary(NiAnimationKey* self, re::Edx, void* stream) {
    ni::scene::LoadBytes(stream, &self->time, 4);
}

// 0x7FFC40 (t, a, b): (1 - t) a + t b
float __cdecl LinearInterp(float t, float value0, float value1) {
    float s = 1.0f - t;
    float a = s * value0;
    float b = t * value1;
    return a + b;
}

// 0x7FFC60 (t, p0, out0, p1, in1): the Hermite cubic p0 + (out0 + (A + B t) t) t, with
// A = 3 (p1 - p0) - (2 out0 + in1) and B = (out0 + in1) - 2 (p1 - p0)
float __cdecl CubicInterp(float t, float value0, float outTangent0, float value1, float inTangent1) {
    float diff = value1 - value0;
    float sum = outTangent0 + inTangent1;
    float b = sum - diff * 2.0f;
    float bt = b * t;
    float a = diff * 3.0f - (inTangent1 + outTangent0 * 2.0f);
    float r = bt + a;
    r = r * t;
    r = outTangent0 + r;
    r = t * r;
    return value0 + r;
}

// 0x775D00 NiQuaternion::Squad(t, p, a, b, q): Slerp(2t (1 - t), Slerp(t, p, q), Slerp(t, a, b))
NiQuaternion* __cdecl Squad(NiQuaternion* result, float t, const NiQuaternion* p, const NiQuaternion* a,
                            const NiQuaternion* b, const NiQuaternion* q) {
    NiQuaternion ab;
    NiQuaternion pq;
    Quaternion_Slerp(&ab, t, a, b);
    Quaternion_Slerp(&pq, t, p, q);
    float s = (1.0f - t) * (t + t);
    Quaternion_Slerp(result, s, &pq, &ab);
    return result;
}

// ---- float keys

// 0x7F8500 NiFloatKey::GenInterpDefault(time, keys, type, count, size): the default without keys,
// the first or last value outside them, else GenInterp from the first key
float __cdecl FloatGenInterpDefault(float time, NiFloatKey* keys, KeyType type, uint32_t numKeys, uint8_t size) {
    if (numKeys == 0) {
        return NiFloatKey_ms_fDefault;
    }
    if (keys->time > time) {
        return keys->value;
    }
    NiFloatKey* last = KeyAt<NiFloatKey>(keys, numKeys - 1, size);
    if (last->time < time) {
        return last->value;
    }
    uint32_t lastIdx = 0;   // (the game's: the count's argument slot)
    return FloatKey_GenInterp(time, keys, type, numKeys, &lastIdx, size);
}

// 0x7F8610 NiFloatKey::LoadBinary(stream): the time, the value
void __fastcall FloatLoadBinary(NiFloatKey* self, re::Edx, void* stream) {
    AnimationKey_LoadBinary_(self, stream);
    ni::scene::LoadBytes(stream, &self->value, 4);
}

// 0x7FE150 NiTCBFloatKey::LoadBinary(stream): the float key's, tension, continuity, bias
void __fastcall TCBFloatLoadBinary(NiTCBFloatKey* self, re::Edx, void* stream) {
    FloatKey_LoadBinary_(self, stream);
    ni::scene::LoadBytes(stream, &self->tension, 4);
    ni::scene::LoadBytes(stream, &self->continuity, 4);
    ni::scene::LoadBytes(stream, &self->bias, 4);
}

// 0x7FBD00 NiLinFloatKey::Copy (NiStepFloatKey's too)
void __cdecl LinFloatCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    X87Copy(&dest->time, &src->time);
    X87Copy(&static_cast<NiFloatKey*>(dest)->value, &static_cast<const NiFloatKey*>(src)->value);
}

// 0x7F86E0 NiBezFloatKey::Copy
void __cdecl BezFloatCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    auto* to = static_cast<NiBezFloatKey*>(dest);
    auto* from = static_cast<const NiBezFloatKey*>(src);
    X87Copy(&to->time, &from->time);
    X87Copy(&to->value, &from->value);
    X87Copy(&to->inTangent, &from->inTangent);
    X87Copy(&to->outTangent, &from->outTangent);
}

// 0x7FDD20 NiTCBFloatKey::Copy
void __cdecl TCBFloatCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    const float* from = reinterpret_cast<const float*>(src);
    float* to = reinterpret_cast<float*>(dest);
    for (int i = 0; i < 7; i++) {
        X87Copy(&to[i], &from[i]);
    }
}

// 0x7FBD20 NiLinFloatKey::Interp
void __cdecl LinFloatInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    float value1 = static_cast<const NiFloatKey*>(key1)->value;
    float value0 = static_cast<const NiFloatKey*>(key0)->value;
    *static_cast<float*>(result) = LinearInterp_(t, value0, value1);
}

// 0x7F8700 NiBezFloatKey::Interp: the cubic with key 0's outgoing and key 1's incoming tangents
void __cdecl BezFloatInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    auto* k0 = static_cast<const NiBezFloatKey*>(key0);
    auto* k1 = static_cast<const NiBezFloatKey*>(key1);
    *static_cast<float*>(result) = CubicInterp_(t, k0->value, k0->outTangent, k1->value, k1->inTangent);
}

// 0x7FDD60 NiTCBFloatKey::Interp: the cubic with the derived tangents
void __cdecl TCBFloatInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    auto* k0 = static_cast<const NiTCBFloatKey*>(key0);
    auto* k1 = static_cast<const NiTCBFloatKey*>(key1);
    *static_cast<float*>(result) = CubicInterp_(t, k0->value, k0->dd, k1->value, k1->ds);
}

// 0x7FE830 NiStepFloatKey::Interp: key 0's value before t = 1, key 1's from there
void __cdecl StepFloatInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    const NiAnimationKey* key = 1.0f > t ? key0 : key1;
    X87Copy(result, &static_cast<const NiFloatKey*>(key)->value);
}

// Insert for the 8-byte keys (linear, step): a key at `time` with the interpolated value (none
// where one is)
inline uint8_t InsertFloatKey(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys, KeyType type) {
    auto* old = static_cast<NiFloatKey*>(*keysRef);
    uint32_t at;
    if (AnimationKey_OkayToInsert(time, old, *numKeys, &at, 8) == 0) {
        return 0;
    }
    uint8_t* keys = NewKeys(*numKeys + 1, 8);
    CopyWithGap(keys, old, *numKeys, at, 8);
    float value = FloatKey_GenInterpDefault_(time, old, type, *numKeys, 8);
    auto* key = KeyAtAddress<NiFloatKey>(keys, at, 8);
    key->time = time;
    key->value = value;
    (*numKeys)++;
    DeleteKeys(old, 8);
    *keysRef = reinterpret_cast<NiAnimationKey*>(keys);
    return 1;
}

// 0x7FBE50 NiLinFloatKey::Insert
uint8_t __cdecl LinFloatInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertFloatKey(time, keysRef, numKeys, LINKEY);
}

// 0x7FE860 NiStepFloatKey::Insert
uint8_t __cdecl StepFloatInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertFloatKey(time, keysRef, numKeys, STEPKEY);
}

// 0x7F8760 NiBezFloatKey::Insert: the curve's value there, zero tangents; between two keys the
// curve split there (the neighbours' tangents rescaled)
uint8_t __cdecl BezFloatInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    auto* old = static_cast<NiBezFloatKey*>(*keysRef);
    uint32_t at;
    if (AnimationKey_OkayToInsert(time, old, *numKeys, &at, 0x10) == 0) {
        return 0;
    }
    uint8_t* keys = NewKeys(*numKeys + 1, 0x10);
    CopyWithGap(keys, old, *numKeys, at, 0x10);
    float value = FloatKey_GenInterpDefault_(time, old, BEZKEY, *numKeys, 0x10);
    auto* key = KeyAtAddress<NiBezFloatKey>(keys, at, 0x10);
    key->time = time;
    key->value = value;
    key->inTangent = 0.0f;
    key->outTangent = 0.0f;
    if (at != 0 && at != *numKeys) {
        auto* prev = KeyAtAddress<NiBezFloatKey>(keys, at - 1, 0x10);
        auto* next = KeyAtAddress<NiBezFloatKey>(keys, at + 1, 0x10);
        float prevOut = prev->outTangent;
        float nextIn = next->inTangent;
        float newIn;
        float newOut;
        AnimationKey_SplitBezierTangents(prev->value, prev->time, &prevOut, next->value, next->time, &nextIn, time,
                                         value, &newIn, &newOut);
        prev->outTangent = prevOut;
        key->inTangent = newIn;
        key->outTangent = newOut;
        next->inTangent = nextIn;
    }
    (*numKeys)++;
    DeleteKeys(old, 0x10);
    *keysRef = reinterpret_cast<NiAnimationKey*>(keys);
    return 1;
}

// 0x7FE000 NiTCBFloatKey::Insert: the interpolated value, zero tension, continuity and bias;
// the tangents made again
uint8_t __cdecl TCBFloatInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    auto* old = static_cast<NiTCBFloatKey*>(*keysRef);
    uint32_t at;
    if (AnimationKey_OkayToInsert(time, old, *numKeys, &at, 0x1C) == 0) {
        return 0;
    }
    uint8_t* keys = NewKeys(*numKeys + 1, 0x1C);
    CopyWithGap(keys, old, *numKeys, at, 0x1C);
    float value = FloatKey_GenInterpDefault_(time, old, TCBKEY, *numKeys, 0x1C);
    auto* key = KeyAtAddress<NiTCBFloatKey>(keys, at, 0x1C);
    key->time = time;
    key->value = value;
    key->tension = 0.0f;
    key->continuity = 0.0f;
    key->bias = 0.0f;
    (*numKeys)++;
    DeleteKeys(old, 0x1C);
    *keysRef = reinterpret_cast<NiAnimationKey*>(keys);
    TCBFloatKey_FillDerivedVals(reinterpret_cast<NiAnimationKey*>(keys), *numKeys, 0x1C);
    return 1;
}

// 0x7FBDA0 NiLinFloatKey::CreateFromStream (NiStepFloatKey's too)
NiAnimationKey* __cdecl LinFloatCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewKeys(numKeys, 8);
    for (uint32_t i = 0; i < numKeys; i++) {
        FloatKey_LoadBinary_(KeyAtAddress<NiFloatKey>(keys, i, 8), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// 0x7F8940 NiBezFloatKey::CreateFromStream: each key, its tangents
NiAnimationKey* __cdecl BezFloatCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewKeys(numKeys, 0x10);
    for (uint32_t i = 0; i < numKeys; i++) {
        auto* key = KeyAtAddress<NiBezFloatKey>(keys, i, 0x10);
        FloatKey_LoadBinary_(key, stream);
        ni::scene::LoadBytes(stream, &key->inTangent, 4);
        ni::scene::LoadBytes(stream, &key->outTangent, 4);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// 0x7FE270 NiTCBFloatKey::CreateFromStream
NiAnimationKey* __cdecl TCBFloatCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewKeys(numKeys, 0x1C);
    for (uint32_t i = 0; i < numKeys; i++) {
        TCBFloatKey_LoadBinary_(KeyAtAddress<NiTCBFloatKey>(keys, i, 0x1C), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// the arrays freed (NiDelete[]: no destructors), by key size
// 0x7FBE30 (8: the linear and step float keys', NiStepBoolKey's)
void __cdecl DeleteArray8(NiAnimationKey* keys) {
    DeleteKeys(keys, 8);
}
// 0x7FC150 (0x10: NiBezFloatKey's, the linear and step position keys')
void __cdecl DeleteArray10(NiAnimationKey* keys) {
    DeleteKeys(keys, 0x10);
}
// 0x7FEDE0 (0x14: the linear and step rotation and colour keys')
void __cdecl DeleteArray14(NiAnimationKey* keys) {
    DeleteKeys(keys, 0x14);
}
// 0x7FDFE0 (0x1C: NiTCBFloatKey's)
void __cdecl DeleteArray1C(NiAnimationKey* keys) {
    DeleteKeys(keys, 0x1C);
}
// 0x7FA6B0 (0x24: NiBezRotKey's)
void __cdecl DeleteArray24(NiAnimationKey* keys) {
    DeleteKeys(keys, 0x24);
}
// 0x7FD8E0 (0x40: NiBezPosKey's, NiTCBRotKey's)
void __cdecl DeleteArray40(NiAnimationKey* keys) {
    DeleteKeys(keys, 0x40);
}
// 0x7FB520 (0x48: NiEulerRotKey's; their sequences not let go)
void __cdecl DeleteArray48(NiAnimationKey* keys) {
    DeleteKeys(keys, 0x48);
}
// 0x7FD080 (0x4C: NiTCBPosKey's)
void __cdecl DeleteArray4C(NiAnimationKey* keys) {
    DeleteKeys(keys, 0x4C);
}

// ---- position keys

// 0x7F8B10 NiPosKey::LoadBinary(stream): the time, the position
void __fastcall PosLoadBinary(NiPosKey* self, re::Edx, void* stream) {
    AnimationKey_LoadBinary_(self, stream);
    Point3_LoadBinary(&self->pos, stream);
}

// 0x7FD1E0 NiTCBPosKey::LoadBinary(stream): the position key's, tension, continuity, bias
void __fastcall TCBPosLoadBinary(NiTCBPosKey* self, re::Edx, void* stream) {
    PosKey_LoadBinary_(self, stream);
    ni::scene::LoadBytes(stream, &self->tension, 4);
    ni::scene::LoadBytes(stream, &self->continuity, 4);
    ni::scene::LoadBytes(stream, &self->bias, 4);
}

// 0x7FBFF0 NiLinPosKey::Copy (NiStepPosKey's too)
void __cdecl LinPosCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    X87Copy(&dest->time, &src->time);
    Move(&static_cast<NiPosKey*>(dest)->pos, &static_cast<const NiPosKey*>(src)->pos, 0xC);
}

// 0x7F9240 NiBezPosKey::Copy
void __cdecl BezPosCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    X87Copy(&dest->time, &src->time);
    Move(reinterpret_cast<uint8_t*>(dest) + 4, reinterpret_cast<const uint8_t*>(src) + 4, 0x3C);
}

// 0x7FC640 NiTCBPosKey::Copy
void __cdecl TCBPosCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    auto* to = static_cast<NiTCBPosKey*>(dest);
    auto* from = static_cast<const NiTCBPosKey*>(src);
    X87Copy(&to->time, &from->time);
    Move(&to->pos, &from->pos, 0xC);
    X87Copy(&to->tension, &from->tension);
    X87Copy(&to->continuity, &from->continuity);
    X87Copy(&to->bias, &from->bias);
    Move(&to->ds, &from->ds, 0x30);
}

// 0x7FEAB0 NiLinPosKey::CreateFromStream (NiStepPosKey's too)
NiAnimationKey* __cdecl LinPosCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewKeys(numKeys, 0x10);
    for (uint32_t i = 0; i < numKeys; i++) {
        PosKey_LoadBinary_(KeyAtAddress<NiPosKey>(keys, i, 0x10), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// 0x7F98C0 NiBezPosKey::CreateFromStream: each key, its tangents
NiAnimationKey* __cdecl BezPosCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewKeys(numKeys, 0x40);
    for (uint32_t i = 0; i < numKeys; i++) {
        auto* key = KeyAtAddress<NiBezPosKey>(keys, i, 0x40);
        PosKey_LoadBinary_(key, stream);
        Point3_LoadBinary(&key->inTangent, stream);
        Point3_LoadBinary(&key->outTangent, stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// 0x7FD360 NiTCBPosKey::CreateFromStream
NiAnimationKey* __cdecl TCBPosCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewKeys(numKeys, 0x4C);
    for (uint32_t i = 0; i < numKeys; i++) {
        TCBPosKey_LoadBinary_(KeyAtAddress<NiTCBPosKey>(keys, i, 0x4C), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// ---- rotation keys

// 0x7F9E00 NiRotKey::LoadBinary(stream): the time, the quaternion (NiColorKey's too: four floats)
void __fastcall RotLoadBinary(NiRotKey* self, re::Edx, void* stream) {
    AnimationKey_LoadBinary_(self, stream);
    Float4_LoadBinary(&self->quat, stream);
}

// 0x7FDA50 NiTCBRotKey::LoadBinary(stream): the rotation key's, tension, continuity, bias
void __fastcall TCBRotLoadBinary(NiTCBRotKey* self, re::Edx, void* stream) {
    RotKey_LoadBinary_(self, stream);
    ni::scene::LoadBytes(stream, &self->tension, 4);
    ni::scene::LoadBytes(stream, &self->continuity, 4);
    ni::scene::LoadBytes(stream, &self->bias, 4);
}

// 0x7FB640 NiEulerRotKey::LoadBinary(stream): (before 10.1.0.104 a word not kept) each axis's
// count, and when it has keys their type and the keys (the type's create function)
void __fastcall EulerLoadBinary(NiEulerRotKey* self, re::Edx, void* stream) {
    if (ni::scene::StreamVersion(stream) < 0x0A010068) {
        uint32_t unused;
        ni::scene::LoadBytes(stream, &unused, 4);
    }
    for (uint32_t i = 0; i < 3; i++) {
        ni::scene::LoadBytes(stream, &self->numKeys[i], 4);
        if (self->numKeys[i] != 0) {
            uint32_t type;
            ni::scene::LoadBytes(stream, &type, 4);
            self->type[i] = KeyType(type);
            self->size[i] = ms_keySizes[type];
            self->keys[i] = static_cast<NiFloatKey*>(ms_create[type](stream, self->numKeys[i]));
        }
    }
}

// 0x7FA120 NiRotKey::GenInterpDefault(result, time, keys, type, count, size): the default without
// keys, the first or last quaternion outside them, else GenInterp from the first key
NiQuaternion* __cdecl RotGenInterpDefault(NiQuaternion* result, float time, NiRotKey* keys, KeyType type,
                                          uint32_t numKeys, uint8_t size) {
    if (numKeys == 0) {
        *result = NiRotKey_ms_kDefault;
        return result;
    }
    if (keys->time > time) {
        *result = keys->quat;
        return result;
    }
    NiRotKey* last = KeyAt<NiRotKey>(keys, numKeys - 1, size);
    if (last->time < time) {
        *result = last->quat;
        return result;
    }
    uint32_t lastIdx = 0;
    RotKey_GenInterp(result, time, keys, type, numKeys, &lastIdx, size);
    return result;
}

// 0x7F9DC0 NiRotKey::Interp (no interpolation): the identity
void __cdecl RotInterp(float, const NiAnimationKey*, const NiAnimationKey*, void* result) {
    NiQuaternion identity;
    NiQuaternion* q = Quaternion_Construct(&identity, 1.0f, 0.0f, 0.0f, 0.0f);
    Move(result, q, 0x10);
}

// 0x7FB8E0 NiLinRotKey::Copy (folded: NiStepRotKey's, NiLinColorKey's, NiStepColorKey's)
void __cdecl LinRotCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    X87Copy(&dest->time, &src->time);
    Move(reinterpret_cast<uint8_t*>(dest) + 4, reinterpret_cast<const uint8_t*>(src) + 4, 0x10);
}

// 0x7FA480 NiBezRotKey::Copy
void __cdecl BezRotCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    X87Copy(&dest->time, &src->time);
    Move(reinterpret_cast<uint8_t*>(dest) + 4, reinterpret_cast<const uint8_t*>(src) + 4, 0x20);
}

// 0x7FD450 NiTCBRotKey::Copy
void __cdecl TCBRotCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    auto* to = static_cast<NiTCBRotKey*>(dest);
    auto* from = static_cast<const NiTCBRotKey*>(src);
    X87Copy(&to->time, &from->time);
    Move(&to->quat, &from->quat, 0x10);
    X87Copy(&to->tension, &from->tension);
    X87Copy(&to->continuity, &from->continuity);
    X87Copy(&to->bias, &from->bias);
    Move(&to->a, &from->a, 0x20);
}

// 0x7FC390 NiLinRotKey::Interp: Slerp
void __cdecl LinRotInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    NiQuaternion value;
    NiQuaternion* q = Quaternion_Slerp(&value, t, &static_cast<const NiRotKey*>(key0)->quat,
                                       &static_cast<const NiRotKey*>(key1)->quat);
    Move(result, q, 0x10);
}

// 0x7FA4D0 NiBezRotKey::Interp: Squad through the intermediates
void __cdecl BezRotInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    auto* k0 = static_cast<const NiBezRotKey*>(key0);
    auto* k1 = static_cast<const NiBezRotKey*>(key1);
    NiQuaternion value;
    NiQuaternion* q = Quaternion_Squad_(&value, t, &k0->quat, &k0->intQuat, &k1->intQuat, &k1->quat);
    Move(result, q, 0x10);
}

// 0x7FD4C0 NiTCBRotKey::Interp: Squad through key 0's A and key 1's B
void __cdecl TCBRotInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    auto* k0 = static_cast<const NiTCBRotKey*>(key0);
    auto* k1 = static_cast<const NiTCBRotKey*>(key1);
    NiQuaternion value;
    NiQuaternion* q = Quaternion_Squad_(&value, t, &k0->quat, &k0->a, &k1->b, &k1->quat);
    Move(result, q, 0x10);
}

// 0x7FED70 NiStepRotKey::Interp (NiStepColorKey's too): key 0's four floats before t = 1, key 1's
// from there
void __cdecl StepRotInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    const NiAnimationKey* key = 1.0f > t ? key0 : key1;
    Move(result, reinterpret_cast<const uint8_t*>(key) + 4, 0x10);
}

// Insert for the rotation keys: a key at `time` (the array's keys constructed) with the
// interpolated quaternion, TCB's parameters 0; the derived values made again
template <uint32_t Size, KeyType Type>
inline uint8_t InsertRotKey(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys,
                            void(__cdecl* fill)(NiAnimationKey*, uint32_t, uint8_t)) {
    auto* old = static_cast<NiRotKey*>(*keysRef);
    uint32_t at;
    if (AnimationKey_OkayToInsert(time, old, *numKeys, &at, uint8_t(Size)) == 0) {
        return 0;
    }
    uint8_t* keys = NewConstructedKeys(*numKeys + 1, Size, kRotKeyConstruct);
    CopyWithGap(keys, old, *numKeys, at, Size);
    NiQuaternion value;
    RotKey_GenInterpDefault_(&value, time, old, Type, *numKeys, uint8_t(Size));
    auto* key = KeyAtAddress<NiRotKey>(keys, at, Size);
    key->time = time;
    Move(&key->quat, &value, 0x10);
    if constexpr (Type == TCBKEY) {
        auto* tcb = static_cast<NiTCBRotKey*>(key);
        tcb->tension = 0.0f;
        tcb->continuity = 0.0f;
        tcb->bias = 0.0f;
    }
    (*numKeys)++;
    DeleteKeys(old, Size);
    *keysRef = reinterpret_cast<NiAnimationKey*>(keys);
    fill(reinterpret_cast<NiAnimationKey*>(keys), *numKeys, uint8_t(Size));
    return 1;
}

// 0x7FC450 NiLinRotKey::Insert
uint8_t __cdecl LinRotInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertRotKey<0x14, LINKEY>(time, keysRef, numKeys, RotKey_FillDerivedVals);
}

// 0x7FEE00 NiStepRotKey::Insert
uint8_t __cdecl StepRotInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertRotKey<0x14, STEPKEY>(time, keysRef, numKeys, RotKey_FillDerivedVals);
}

// 0x7FA6D0 NiBezRotKey::Insert
uint8_t __cdecl BezRotInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertRotKey<0x24, BEZKEY>(time, keysRef, numKeys, BezRotKey_FillDerivedVals);
}

// 0x7FD900 NiTCBRotKey::Insert
uint8_t __cdecl TCBRotInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertRotKey<0x40, TCBKEY>(time, keysRef, numKeys, TCBRotKey_FillDerivedVals);
}

// 0x7FB580 NiEulerRotKey::Insert: (no key yet: one made) a key inserted into each axis's sequence
// that has keys (its type's Insert); true when one was
uint8_t __cdecl EulerInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    if (*numKeys == 0) {
        uint32_t* block = static_cast<uint32_t*>(NiNewArray(0x4C));
        NiAnimationKey* keys = nullptr;
        if (block != nullptr) {
            *block = 1;
            VectorConstruct(block + 1, 0x48, 1, reinterpret_cast<void*>(kEulerRotKeyConstruct));
            keys = reinterpret_cast<NiAnimationKey*>(block + 1);
        }
        *keysRef = keys;
        *numKeys = 1;
    }
    auto* key = static_cast<NiEulerRotKey*>(*keysRef);
    uint8_t inserted = 0;
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t count = key->numKeys[i];
        if (count == 0) {
            continue;
        }
        re::Ptr32<NiAnimationKey> sequence = key->keys[i];
        if (ms_insert[key->type[i]](time, &sequence, &count)) {
            inserted = 1;
        }
        key->numKeys[i] = count;
        key->keys[i] = static_cast<NiFloatKey*>(sequence);
        key->lastIdx[i] = 0;
    }
    return inserted != 0 ? 1 : 0;
}

// 0x7FC3E0 NiLinRotKey::CreateFromStream (NiStepRotKey's too)
NiAnimationKey* __cdecl LinRotCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewConstructedKeys(numKeys, 0x14, kRotKeyConstruct);
    for (uint32_t i = 0; i < numKeys; i++) {
        RotKey_LoadBinary_(KeyAtAddress<NiRotKey>(keys, i, 0x14), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// 0x7FA830 NiBezRotKey::CreateFromStream (the intermediates FillDerivedVals')
NiAnimationKey* __cdecl BezRotCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewConstructedKeys(numKeys, 0x24, kRotKeyConstruct);
    for (uint32_t i = 0; i < numKeys; i++) {
        RotKey_LoadBinary_(KeyAtAddress<NiRotKey>(keys, i, 0x24), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// 0x7FDB70 NiTCBRotKey::CreateFromStream
NiAnimationKey* __cdecl TCBRotCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewConstructedKeys(numKeys, 0x40, kRotKeyConstruct);
    for (uint32_t i = 0; i < numKeys; i++) {
        TCBRotKey_LoadBinary_(KeyAtAddress<NiTCBRotKey>(keys, i, 0x40), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// 0x7FB7B0 NiEulerRotKey::CreateFromStream
NiAnimationKey* __cdecl EulerCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewConstructedKeys(numKeys, 0x48, kEulerRotKeyConstruct);
    for (uint32_t i = 0; i < numKeys; i++) {
        EulerRotKey_LoadBinary_(KeyAtAddress<NiEulerRotKey>(keys, i, 0x48), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// ---- colour keys

// 0x7FB010 NiColorKey::GenInterpDefault(result, time, keys, type, count, size)
NiColorA* __cdecl ColorGenInterpDefault(NiColorA* result, float time, NiColorKey* keys, KeyType type,
                                        uint32_t numKeys, uint8_t size) {
    if (numKeys == 0) {
        *result = NiColorKey_ms_kDefault;
        return result;
    }
    if (keys->time > time) {
        *result = keys->color;
        return result;
    }
    NiColorKey* last = KeyAt<NiColorKey>(keys, numKeys - 1, size);
    if (last->time < time) {
        *result = last->color;
        return result;
    }
    uint32_t lastIdx = 0;
    ColorKey_GenInterp(result, time, keys, type, numKeys, &lastIdx, size);
    return result;
}

// 0x7FAD90 NiColorKey::Interp (no interpolation): NiColorA's default
void __cdecl ColorInterp(float, const NiAnimationKey*, const NiAnimationKey*, void* result) {
    Move(result, &NiColorA_Default, 0x10);
}

// Insert for the colour keys (linear, step): the interpolated colour
inline uint8_t InsertColorKey(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys, KeyType type) {
    auto* old = static_cast<NiColorKey*>(*keysRef);
    uint32_t at;
    if (AnimationKey_OkayToInsert(time, old, *numKeys, &at, 0x14) == 0) {
        return 0;
    }
    uint8_t* keys = NewConstructedKeys(*numKeys + 1, 0x14, kColorKeyConstruct);
    CopyWithGap(keys, old, *numKeys, at, 0x14);
    NiColorA value;
    ColorKey_GenInterpDefault_(&value, time, old, type, *numKeys, 0x14);
    auto* key = KeyAtAddress<NiColorKey>(keys, at, 0x14);
    key->time = time;
    Move(&key->color, &value, 0x10);
    (*numKeys)++;
    DeleteKeys(old, 0x14);
    *keysRef = reinterpret_cast<NiAnimationKey*>(keys);
    return 1;
}

// 0x7FBB20 NiLinColorKey::Insert
uint8_t __cdecl LinColorInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertColorKey(time, keysRef, numKeys, LINKEY);
}

// 0x7FE640 NiStepColorKey::Insert
uint8_t __cdecl StepColorInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    return InsertColorKey(time, keysRef, numKeys, STEPKEY);
}

// 0x7FBA80 NiLinColorKey::CreateFromStream (NiStepColorKey's too)
NiAnimationKey* __cdecl LinColorCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewConstructedKeys(numKeys, 0x14, kColorKeyConstruct);
    for (uint32_t i = 0; i < numKeys; i++) {
        RotKey_LoadBinary_(KeyAtAddress<NiRotKey>(keys, i, 0x14), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

// ---- bool keys

// 0x7FACB0 NiBoolKey::LoadBinary(stream): the time, a byte (any non-zero true)
void __fastcall BoolLoadBinary(NiBoolKey* self, re::Edx, void* stream) {
    ni::scene::LoadBytes(stream, &self->time, 4);
    uint8_t value;   // (the game's: the stream argument's slot)
    ni::scene::LoadBytes(stream, &value, 1);
    self->value = value != 0 ? 1 : 0;
}

// 0x7FE390 NiStepBoolKey::Copy
void __cdecl StepBoolCopy(NiAnimationKey* dest, const NiAnimationKey* src) {
    X87Copy(&dest->time, &src->time);
    static_cast<NiBoolKey*>(dest)->value = static_cast<const NiBoolKey*>(src)->value;
}

// 0x7FE3C0 NiStepBoolKey::Interp: key 0's value before t = 1, key 1's from there
void __cdecl StepBoolInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    const NiAnimationKey* key = 1.0f > t ? key0 : key1;
    *static_cast<uint8_t*>(result) = static_cast<const NiBoolKey*>(key)->value;
}

// 0x7FE520 NiStepBoolKey::CreateFromStream
NiAnimationKey* __cdecl StepBoolCreate(void* stream, uint32_t numKeys) {
    uint8_t* keys = NewKeys(numKeys, 8);
    for (uint32_t i = 0; i < numKeys; i++) {
        BoolKey_LoadBinary_(KeyAtAddress<NiBoolKey>(keys, i, 8), stream);
    }
    return reinterpret_cast<NiAnimationKey*>(keys);
}

} // namespace
} // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIKEYCB_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace kc = ni::anim;

NIKEYCB_REVERSED("NiQuaternion::Squad", 0x00775D00, kc::Squad);
NIKEYCB_REVERSED("NiFloatKey::GenInterpDefault", 0x007F8500, kc::FloatGenInterpDefault);
NIKEYCB_REVERSED("NiFloatKey::LoadBinary", 0x007F8610, kc::FloatLoadBinary);
NIKEYCB_REVERSED("NiBezFloatKey::Copy", 0x007F86E0, kc::BezFloatCopy);
NIKEYCB_REVERSED("NiBezFloatKey::Interp", 0x007F8700, kc::BezFloatInterp);
NIKEYCB_REVERSED("NiBezFloatKey::Insert", 0x007F8760, kc::BezFloatInsert);
NIKEYCB_REVERSED("NiBezFloatKey::CreateFromStream", 0x007F8940, kc::BezFloatCreate);
NIKEYCB_REVERSED("NiPosKey::LoadBinary", 0x007F8B10, kc::PosLoadBinary);
NIKEYCB_REVERSED("NiBezPosKey::Copy", 0x007F9240, kc::BezPosCopy);
NIKEYCB_REVERSED("NiBezPosKey::CreateFromStream", 0x007F98C0, kc::BezPosCreate);
NIKEYCB_REVERSED("NiRotKey::Interp", 0x007F9DC0, kc::RotInterp);
// (folded: NiColorKey::LoadBinary)
NIKEYCB_REVERSED("NiRotKey::LoadBinary", 0x007F9E00, kc::RotLoadBinary);
NIKEYCB_REVERSED("NiRotKey::GenInterpDefault", 0x007FA120, kc::RotGenInterpDefault);
NIKEYCB_REVERSED("NiBezRotKey::Copy", 0x007FA480, kc::BezRotCopy);
NIKEYCB_REVERSED("NiBezRotKey::Interp", 0x007FA4D0, kc::BezRotInterp);
NIKEYCB_REVERSED("NiBezRotKey::DeleteArray", 0x007FA6B0, kc::DeleteArray24);
NIKEYCB_REVERSED("NiBezRotKey::Insert", 0x007FA6D0, kc::BezRotInsert);
NIKEYCB_REVERSED("NiBezRotKey::CreateFromStream", 0x007FA830, kc::BezRotCreate);
NIKEYCB_REVERSED("NiBoolKey::LoadBinary", 0x007FACB0, kc::BoolLoadBinary);
NIKEYCB_REVERSED("NiColorKey::Interp", 0x007FAD90, kc::ColorInterp);
NIKEYCB_REVERSED("NiColorKey::GenInterpDefault", 0x007FB010, kc::ColorGenInterpDefault);
NIKEYCB_REVERSED("NiEulerRotKey::DeleteArray", 0x007FB520, kc::DeleteArray48);
NIKEYCB_REVERSED("NiEulerRotKey::Insert", 0x007FB580, kc::EulerInsert);
NIKEYCB_REVERSED("NiEulerRotKey::LoadBinary", 0x007FB640, kc::EulerLoadBinary);
NIKEYCB_REVERSED("NiEulerRotKey::CreateFromStream", 0x007FB7B0, kc::EulerCreate);
// (folded: NiStepRotKey's, NiLinColorKey's, NiStepColorKey's Copy)
NIKEYCB_REVERSED("NiLinRotKey::Copy", 0x007FB8E0, kc::LinRotCopy);
// (folded: NiStepColorKey's)
NIKEYCB_REVERSED("NiLinColorKey::CreateFromStream", 0x007FBA80, kc::LinColorCreate);
NIKEYCB_REVERSED("NiLinColorKey::Insert", 0x007FBB20, kc::LinColorInsert);
// (folded: NiStepFloatKey's)
NIKEYCB_REVERSED("NiLinFloatKey::Copy", 0x007FBD00, kc::LinFloatCopy);
NIKEYCB_REVERSED("NiLinFloatKey::Interp", 0x007FBD20, kc::LinFloatInterp);
// (folded: NiStepFloatKey's)
NIKEYCB_REVERSED("NiLinFloatKey::CreateFromStream", 0x007FBDA0, kc::LinFloatCreate);
// (folded: NiStepFloatKey's, NiStepBoolKey's)
NIKEYCB_REVERSED("NiLinFloatKey::DeleteArray", 0x007FBE30, kc::DeleteArray8);
NIKEYCB_REVERSED("NiLinFloatKey::Insert", 0x007FBE50, kc::LinFloatInsert);
// (folded: NiStepPosKey's)
NIKEYCB_REVERSED("NiLinPosKey::Copy", 0x007FBFF0, kc::LinPosCopy);
// (folded: NiBezFloatKey's, NiStepPosKey's)
NIKEYCB_REVERSED("NiLinPosKey::DeleteArray", 0x007FC150, kc::DeleteArray10);
// (a lone jmp to NiRotKey::FillDerivedVals: NiStepRotKey's too)
// NiLinRotKey::FillDerivedVals (0x007FC350): registered in NiRotKey.cpp with its target's body
NIKEYCB_REVERSED("NiLinRotKey::Interp", 0x007FC390, kc::LinRotInterp);
// (folded: NiStepRotKey's)
NIKEYCB_REVERSED("NiLinRotKey::CreateFromStream", 0x007FC3E0, kc::LinRotCreate);
NIKEYCB_REVERSED("NiLinRotKey::Insert", 0x007FC450, kc::LinRotInsert);
NIKEYCB_REVERSED("NiTCBPosKey::Copy", 0x007FC640, kc::TCBPosCopy);
NIKEYCB_REVERSED("NiTCBPosKey::DeleteArray", 0x007FD080, kc::DeleteArray4C);
NIKEYCB_REVERSED("NiTCBPosKey::LoadBinary", 0x007FD1E0, kc::TCBPosLoadBinary);
NIKEYCB_REVERSED("NiTCBPosKey::CreateFromStream", 0x007FD360, kc::TCBPosCreate);
NIKEYCB_REVERSED("NiTCBRotKey::Copy", 0x007FD450, kc::TCBRotCopy);
NIKEYCB_REVERSED("NiTCBRotKey::Interp", 0x007FD4C0, kc::TCBRotInterp);
// (folded: NiTCBRotKey's)
NIKEYCB_REVERSED("NiBezPosKey::DeleteArray", 0x007FD8E0, kc::DeleteArray40);
NIKEYCB_REVERSED("NiTCBRotKey::Insert", 0x007FD900, kc::TCBRotInsert);
NIKEYCB_REVERSED("NiTCBRotKey::LoadBinary", 0x007FDA50, kc::TCBRotLoadBinary);
NIKEYCB_REVERSED("NiTCBRotKey::CreateFromStream", 0x007FDB70, kc::TCBRotCreate);
NIKEYCB_REVERSED("NiTCBFloatKey::Copy", 0x007FDD20, kc::TCBFloatCopy);
NIKEYCB_REVERSED("NiTCBFloatKey::Interp", 0x007FDD60, kc::TCBFloatInterp);
NIKEYCB_REVERSED("NiTCBFloatKey::DeleteArray", 0x007FDFE0, kc::DeleteArray1C);
NIKEYCB_REVERSED("NiTCBFloatKey::Insert", 0x007FE000, kc::TCBFloatInsert);
NIKEYCB_REVERSED("NiTCBFloatKey::LoadBinary", 0x007FE150, kc::TCBFloatLoadBinary);
NIKEYCB_REVERSED("NiTCBFloatKey::CreateFromStream", 0x007FE270, kc::TCBFloatCreate);
NIKEYCB_REVERSED("NiStepBoolKey::Copy", 0x007FE390, kc::StepBoolCopy);
// (a lone jmp to NiBoolKey::Equal)
// NiStepBoolKey::Equal (0x007FE3B0): registered in NiColorKey.cpp with its target's body
NIKEYCB_REVERSED("NiStepBoolKey::Interp", 0x007FE3C0, kc::StepBoolInterp);
NIKEYCB_REVERSED("NiStepBoolKey::CreateFromStream", 0x007FE520, kc::StepBoolCreate);
// (a lone jmp to NiRotKey::Equal: NiLinColorKey's, NiStepColorKey's, NiLinRotKey's, NiStepRotKey's
// Equal)
// NiLinColorKey::Equal (0x007FE630): registered in NiRotKey.cpp with its target's body
NIKEYCB_REVERSED("NiStepColorKey::Insert", 0x007FE640, kc::StepColorInsert);
// (a lone jmp to NiFloatKey::Equal: NiStepFloatKey's too)
// NiLinFloatKey::Equal (0x007FE820): registered in NiFloatKey.cpp with its target's body
NIKEYCB_REVERSED("NiStepFloatKey::Interp", 0x007FE830, kc::StepFloatInterp);
NIKEYCB_REVERSED("NiStepFloatKey::Insert", 0x007FE860, kc::StepFloatInsert);
// (folded: NiStepPosKey's)
NIKEYCB_REVERSED("NiLinPosKey::CreateFromStream", 0x007FEAB0, kc::LinPosCreate);
// (folded: NiStepColorKey's)
NIKEYCB_REVERSED("NiStepRotKey::Interp", 0x007FED70, kc::StepRotInterp);
// (folded: NiLinRotKey's, NiLinColorKey's, NiStepColorKey's)
NIKEYCB_REVERSED("NiStepRotKey::DeleteArray", 0x007FEDE0, kc::DeleteArray14);
NIKEYCB_REVERSED("NiStepRotKey::Insert", 0x007FEE00, kc::StepRotInsert);
NIKEYCB_REVERSED("NiAnimationKey::LoadBinary", 0x007FFBE0, kc::AnimationKeyLoadBinary);
NIKEYCB_REVERSED("NiFloatKey::LinearInterp", 0x007FFC40, kc::LinearInterp);
NIKEYCB_REVERSED("NiFloatKey::CubicInterp", 0x007FFC60, kc::CubicInterp);
