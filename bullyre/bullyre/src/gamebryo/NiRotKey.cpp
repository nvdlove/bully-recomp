// The rotation keys (src/gamebryo/NiAnimationKey.h): NiRotKey (content ROTKEY, NOINTERP),
// NiLinRotKey, NiBezRotKey, NiTCBRotKey, NiEulerRotKey (three float key sequences, one per axis)
// and NiStepRotKey. GenInterp, the quaternions made to take the short way and their w clamped,
// the squad intermediates (Bezier) and the TCB A/B quaternions, the Euler key's angles made into
// a quaternion, comparison, copying, saving, the arrays (their keys constructed), and each kind's
// Register (its table entries, once).
//
// Main-thread float maths at the game's 24-bit x87 precision: each operation a float one, in the
// game's order (the Euler key's fsincos through the C runtime's sin and cos, rounded).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiAnimationKey.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

GAME_VAR(uint8_t, NiRotKey_registered, 0x020E432D);
GAME_VAR(uint8_t, NiBezRotKey_registered, 0x020E4340);
GAME_VAR(uint8_t, NiEulerRotKey_registered, 0x020E4354);
GAME_VAR(uint8_t, NiLinRotKey_registered, 0x020E4358);
GAME_VAR(uint8_t, NiTCBRotKey_registered, 0x020E435A);
GAME_VAR(uint8_t, NiStepRotKey_registered, 0x020E4360);

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Construct, 0x00775B90, NiQuaternion* self, float w, float x, float y,
        float z);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Add, 0x00775BB0, const NiQuaternion* self, NiQuaternion* result,
        const NiQuaternion* other);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Subtract, 0x00775BE0, const NiQuaternion* self, NiQuaternion* result,
        const NiQuaternion* other);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Negate, 0x00775C10, const NiQuaternion* self, NiQuaternion* result);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Multiply, 0x00775C40, const NiQuaternion* self, NiQuaternion* result,
        const NiQuaternion* other);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_UnitInverse, 0x00775CD0, NiQuaternion* result, const NiQuaternion* q);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_Log, 0x00775D80, NiQuaternion* result, const NiQuaternion* q);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_Intermediate, 0x00775E40, NiQuaternion* result, const NiQuaternion* q0,
        const NiQuaternion* q1, const NiQuaternion* q2);
GAME_FN(void, __thiscall, Quaternion_SaveBinary, 0x00776310, const NiQuaternion* self, void* stream);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_ScalarTimes, 0x00776470, NiQuaternion* result, float scale,
        const NiQuaternion* q);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_Exp, 0x00776610, NiQuaternion* result, const NiQuaternion* q);
GAME_FN(void, __cdecl, AnimationKey_SaveBinary, 0x007FFC10, void* stream, const NiAnimationKey* key);
GAME_FN(uint8_t, __cdecl, AnimationKey_Equal, 0x007FF6C0, const NiAnimationKey* key0, const NiAnimationKey* key1);
GAME_FN(float, __cdecl, FloatKey_GenInterp, 0x007F8300, float time, NiFloatKey* keys, KeyType type,
        uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
// (this file's, called where modes apply)
GAME_FN(void, __cdecl, RotKey_SaveKey, 0x007F9E20, void* stream, const NiRotKey* key);
GAME_FN(uint8_t, __thiscall, Quaternion_Equal, 0x007F9E40, const NiQuaternion* self, const NiQuaternion* other);
GAME_FN(void, __cdecl, RotKey_FillDerivedVals, 0x007FA1F0, NiAnimationKey* keys, uint32_t numKeys, uint8_t size);
GAME_FN(uint8_t, __cdecl, RotKey_Equal, 0x007FB0E0, const NiAnimationKey* key0, const NiAnimationKey* key1);
GAME_FN(void, __thiscall, EulerRotKey_CleanUp, 0x007FB1F0, NiEulerRotKey* self);
GAME_FN(void, __cdecl, EulerRotKey_SaveKey, 0x007FB720, void* stream, const NiEulerRotKey* key);
GAME_FN(void, __thiscall, TCBRotKey_CalculateDVals, 0x007FD510, NiTCBRotKey* self, const NiTCBRotKey* prev,
        const NiTCBRotKey* next);
GAME_FN(void, __cdecl, TCBRotKey_SaveKey, 0x007FDAE0, void* stream, const NiTCBRotKey* key);

constexpr uintptr_t kRotKeyConstruct = 0x007FC360;
constexpr uintptr_t kEulerRotKeyConstruct = 0x007FB190;
constexpr uintptr_t kNothing = 0x0049A650;      // a function doing nothing (returns)
constexpr uintptr_t kReturnZero = 0x00824EC0;   // a function returning 0

template <typename T>
inline void Set(T* table, uint32_t index, uintptr_t address) {
    table[index] = re::PtrCast<T>(address);
}

// NiNew[] of count keys, the count before them, each constructed (the vector constructor)
inline NiAnimationKey* NewConstructedKeys(uint32_t count, uint32_t size, uintptr_t construct) {
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(KeyArraySize(count, size)));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, size, count, reinterpret_cast<void*>(construct));
    return reinterpret_cast<NiAnimationKey*>(block + 1);
}

// ---- NiRotKey

// 0x7F9E20 SaveKey(stream, key): the time, the quaternion
void __cdecl SaveKey(void* stream, const NiRotKey* key) {
    AnimationKey_SaveBinary(stream, key);
    Quaternion_SaveBinary(&key->quat, stream);
}

// 0x7F9E40 NiQuaternion::operator==(other) (NiColorA's too: four floats)
uint8_t __fastcall QuaternionEqual(const NiQuaternion* self, re::Edx, const NiQuaternion* other) {
    return (other->w == self->w && other->x == self->x && other->y == self->y && other->z == self->z) ? 1 : 0;
}

// 0x7F9E90 GenInterp(result, time, keys, type, count, last index, size): one key's quaternion; an
// Euler key interpolates its own sequences at the real time; else the kind's Interp between the
// keys around `time`
NiQuaternion* __cdecl GenInterp(NiQuaternion* result, float time, NiRotKey* keys, KeyType type, uint32_t numKeys,
                                uint32_t* lastIdx, uint8_t size) {
    if (numKeys == 1 && type != EULERKEY) {
        *result = keys->quat;
        return result;
    }
    NiQuaternion value;
    if (type == EULERKEY) {
        ms_interp[KeyIndex(ROTKEY, type)](time, keys, nullptr, &value);
        *result = value;
        return result;
    }
    float t0;
    float t1;
    uint32_t next = FindKeyInterval(time, keys, numKeys, *lastIdx, size, t0, t1);
    float t = (time - t0) / (t1 - t0);
    ms_interp[KeyIndex(ROTKEY, type)](t, KeyAt(keys, *lastIdx, size), KeyAt(keys, next, size), &value);
    *result = value;
    return result;
}

// 0x7FA1F0 FillDerivedVals(keys, count, size): each quaternion negated where it's more than 90
// degrees from the one before (so the interpolation takes the short way), then every w clamped to
// [-1, 1] (an unordered one left)
void __cdecl FillDerivedVals(NiAnimationKey* keys, uint32_t numKeys, uint8_t size) {
    uint32_t count = numKeys - 1;
    if (count != 0) {
        uint32_t i = 0;
        do {
            NiQuaternion* q0 = &KeyAt<NiRotKey>(keys, i, size)->quat;
            NiQuaternion* q1 = &KeyAt<NiRotKey>(keys, i + 1, size)->quat;
            float dot = ((q1->x * q0->x + q0->w * q1->w) + q1->y * q0->y) + q1->z * q0->z;
            if (dot < 0.0f) {
                NiQuaternion negated;
                *q1 = *Quaternion_Negate(q1, &negated);
            }
            i++;
        } while (--count != 0);
    }
    for (uint32_t i = 0; i < numKeys; i++) {
        float& w = KeyAt<NiRotKey>(keys, i, size)->quat.w;
        if (w < -1.0f) {
            w = -1.0f;
        } else if (w > 1.0f) {
            w = 1.0f;
        }
    }
}

// 0x7FA3F0 Register: ROTKEY/NOINTERP's entries (Interp the game's 0x7F9DC0)
uint32_t __cdecl Register() {
    if (NiRotKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 12, 0);
    Set(ms_interpD1, 12, 0);
    Set(ms_interpD2, 12, 0);
    Set(ms_insert, 12, 0);
    NiRotKey_registered = 1;
    Set(ms_create, 12, kReturnZero);
    Set(ms_save, 12, kNothing);
    Set(ms_copy, 12, kNothing);
    Set(ms_array, 12, kReturnZero);
    Set(ms_delete, 12, kNothing);
    ms_keySizes[12] = 0x14;
    Set(ms_interp, 12, 0x007F9DC0);
    Set(ms_equal, 12, 0x007FB0E0);
    Set(ms_fillDerived, 12, 0x007FA1F0);
    return 1;
}

// 0x7FB0E0 Equal: the times and the quaternions (NiColorKey's too)
uint8_t __cdecl Equal(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = AnimationKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    return Quaternion_Equal(&static_cast<const NiRotKey*>(key0)->quat, &static_cast<const NiRotKey*>(key1)->quat);
}

// 0x7FC360 NiRotKey(): the quaternion all zero
NiRotKey* __fastcall Construct(NiRotKey* self, re::Edx) {
    Quaternion_Construct(&self->quat, 0.0f, 0.0f, 0.0f, 0.0f);
    return self;
}

// ---- NiBezRotKey

// 0x7FA520 FillDerivedVals(keys, count, size): NiRotKey's, then each key's squad intermediate
// (the ends against themselves). The keys taken as 0x24 bytes apart
void __cdecl BezFillDerivedVals(NiAnimationKey* keys, uint32_t numKeys, uint8_t size) {
    if (numKeys < 2) {
        return;
    }
    RotKey_FillDerivedVals(keys, numKeys, size);
    auto* key = static_cast<NiBezRotKey*>(keys);
    NiQuaternion value;
    key[0].intQuat = *Quaternion_Intermediate(&value, &key[0].quat, &key[0].quat, &key[1].quat);
    uint32_t last = numKeys - 1;
    for (uint32_t i = 1; i < last; i++) {
        key[i].intQuat = *Quaternion_Intermediate(&value, &key[i - 1].quat, &key[i].quat, &key[i + 1].quat);
    }
    key[last].intQuat = *Quaternion_Intermediate(&value, &key[last - 1].quat, &key[last].quat, &key[last].quat);
}

// 0x7FA600 Equal: NiRotKey's, the intermediates
uint8_t __cdecl BezEqual(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = RotKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    return Quaternion_Equal(&static_cast<const NiBezRotKey*>(key0)->intQuat,
                            &static_cast<const NiBezRotKey*>(key1)->intQuat);
}

// 0x7FA630 SaveBinary(stream, keys, count)
void __cdecl BezSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiBezRotKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        RotKey_SaveKey(stream, key);
    }
}

// 0x7FA660 NewArray(count): each key's quaternion zeroed (NiRotKey's constructor)
NiAnimationKey* __cdecl BezNewArray(uint32_t count) {
    return NewConstructedKeys(count, 0x24, kRotKeyConstruct);
}

// 0x7FA8A0 Register
uint32_t __cdecl BezRegister() {
    if (NiBezRotKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 14, 0);
    Set(ms_interpD1, 14, 0);
    Set(ms_interpD2, 14, 0);
    NiBezRotKey_registered = 1;
    Set(ms_create, 14, 0x007FA830);
    Set(ms_save, 14, 0x007FA630);
    Set(ms_copy, 14, 0x007FA480);
    Set(ms_array, 14, 0x007FA660);
    Set(ms_delete, 14, 0x007FA6B0);
    ms_keySizes[14] = 0x24;
    Set(ms_interp, 14, 0x007FA4D0);
    Set(ms_equal, 14, 0x007FA600);
    Set(ms_fillDerived, 14, 0x007FA520);
    Set(ms_insert, 14, 0x007FA6D0);
    return 1;
}

// ---- NiEulerRotKey

// 0x7FB190 NiEulerRotKey(): no sequences
NiEulerRotKey* __fastcall EulerConstruct(NiEulerRotKey* self, re::Edx) {
    Quaternion_Construct(&self->quat, 0.0f, 0.0f, 0.0f, 0.0f);
    self->numKeys[0] = 0;
    self->numKeys[1] = 0;
    self->numKeys[2] = 0;
    self->keys[0] = nullptr;
    self->keys[1] = nullptr;
    self->keys[2] = nullptr;
    self->type[0] = NOINTERP;
    self->type[1] = NOINTERP;
    self->type[2] = NOINTERP;
    self->lastIdx[0] = 0;
    self->lastIdx[1] = 0;
    self->lastIdx[2] = 0;
    self->size[0] = 0;
    self->size[1] = 0;
    self->size[2] = 0;
    return self;
}

// 0x7FB1F0 CleanUp: each sequence's keys deleted (its float kind's Delete), counts and types zeroed
void __fastcall EulerCleanUp(NiEulerRotKey* self, re::Edx) {
    for (uint32_t i = 0; i < 3; i++) {
        if (self->keys[i] != nullptr) {
            ms_delete[self->type[i]](self->keys[i]);
        }
        self->keys[i] = nullptr;
        self->numKeys[i] = 0;
        self->type[i] = NOINTERP;
    }
}

// 0x7FB230 Interp(time, key, -, result): each axis's angle at `time` (0 without keys), the
// quaternion of the three half-angle rotations
void __cdecl EulerInterp(float time, const NiAnimationKey* key0, const NiAnimationKey*, void* result) {
    auto* key = const_cast<NiEulerRotKey*>(static_cast<const NiEulerRotKey*>(key0));
    float angles[3];
    for (uint32_t i = 0; i < 3; i++) {
        if (key->numKeys[i] == 0) {
            angles[i] = 0.0f;
        } else {
            angles[i] = FloatKey_GenInterp(time, key->keys[i], key->type[i], key->numKeys[i], &key->lastIdx[i],
                                           key->size[i]);
        }
    }
    float half0 = angles[0] * 0.5f;
    float c0 = crt::Cosf(half0);
    float s0 = crt::Sinf(half0);
    float half1 = angles[1] * 0.5f;
    float c1 = crt::Cosf(half1);
    float s1 = crt::Sinf(half1);
    float half2 = angles[2] * 0.5f;
    float c2 = crt::Cosf(half2);
    float s2 = crt::Sinf(half2);
    float s1s2 = s2 * s1;
    float c1c2 = c2 * c1;
    auto* q = static_cast<NiQuaternion*>(result);
    q->w = c0 * c1c2 + s0 * s1s2;
    q->x = s0 * c1c2 - c0 * s1s2;
    float c1s2 = s2 * c1;
    float s1c2 = s1 * c2;
    q->y = c0 * s1c2 + s0 * c1s2;
    q->z = c0 * c1s2 - s0 * s1c2;
}

// 0x7FB340 Equal: the counts, types and sizes of each sequence, then their keys (their float
// kind's Equal)
uint8_t __cdecl EulerEqual(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    auto* a = static_cast<const NiEulerRotKey*>(key0);
    auto* b = static_cast<const NiEulerRotKey*>(key1);
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t count = a->numKeys[i];
        if (count != b->numKeys[i] || a->type[i] != b->type[i] || a->size[i] != b->size[i]) {
            return 0;
        }
        if (count != 0) {
            EqualFunction equal = ms_equal[a->type[i]];
            for (uint32_t j = 0; j < a->numKeys[i]; j++) {
                if (equal(KeyAt(a->keys[i], j, a->size[i]), KeyAt(b->keys[i], j, b->size[i])) == 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// 0x7FB410 Copy(dest, src): each sequence's count and type, then a new array of its keys (copied
// at the destination's key size: it isn't copied)
void __cdecl EulerCopy(NiAnimationKey* destKey, const NiAnimationKey* srcKey) {
    auto* dest = static_cast<NiEulerRotKey*>(destKey);
    auto* src = static_cast<const NiEulerRotKey*>(srcKey);
    for (uint32_t i = 0; i < 3; i++) {
        dest->numKeys[i] = src->numKeys[i];
        dest->type[i] = src->type[i];
        uint32_t count = src->numKeys[i];
        if (count == 0) {
            continue;
        }
        dest->keys[i] = static_cast<NiFloatKey*>(ms_array[src->type[i]](count));
        CopyFunction copy = ms_copy[src->type[i]];
        for (uint32_t j = 0; j < count; j++) {
            copy(KeyAt(dest->keys[i], j, dest->size[i]), KeyAt(src->keys[i], j, src->size[i]));
        }
    }
}

// 0x7FB4D0 NewArray(count): each key constructed
NiAnimationKey* __cdecl EulerNewArray(uint32_t count) {
    return NewConstructedKeys(count, 0x48, kEulerRotKeyConstruct);
}

// 0x7FB540 FillDerivedVals(keys, count, size): the first key's sequences' (their float kinds')
void __cdecl EulerFillDerivedVals(NiAnimationKey* keys, uint32_t, uint8_t) {
    auto* key = static_cast<NiEulerRotKey*>(keys);
    for (uint32_t i = 0; i < 3; i++) {
        if (key->keys[i] != nullptr) {
            ms_fillDerived[key->type[i]](key->keys[i], key->numKeys[i], key->size[i]);
        }
    }
}

// 0x7FB720 SaveKey(stream, key): each sequence's count, then (when it has keys) its type and keys
void __cdecl EulerSaveKey(void* stream, const NiEulerRotKey* key) {
    for (uint32_t i = 0; i < 3; i++) {
        ni::scene::SaveBytes(stream, &key->numKeys[i], 4);
        if (key->numKeys[i] != 0) {
            ni::scene::SaveBytes(stream, &key->type[i], 4);
            ms_save[key->type[i]](stream, key->keys[i], key->numKeys[i]);
        }
    }
}

// 0x7FB820 SaveBinary(stream, keys, count)
void __cdecl EulerSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiEulerRotKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        EulerRotKey_SaveKey(stream, key);
    }
}

// 0x7FB850 Register
uint32_t __cdecl EulerRegister() {
    if (NiEulerRotKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 16, 0);
    Set(ms_interpD1, 16, 0);
    Set(ms_interpD2, 16, 0);
    NiEulerRotKey_registered = 1;
    Set(ms_create, 16, 0x007FB7B0);
    Set(ms_save, 16, 0x007FB820);
    Set(ms_copy, 16, 0x007FB410);
    Set(ms_array, 16, 0x007FB4D0);
    Set(ms_delete, 16, 0x007FB520);
    ms_keySizes[16] = 0x48;
    Set(ms_interp, 16, 0x007FB230);
    Set(ms_equal, 16, 0x007FB340);
    Set(ms_fillDerived, 16, 0x007FB540);
    Set(ms_insert, 16, 0x007FB580);
    return 1;
}

// 0x7FF6E0 Initialize(keys, count, type for x, y and z): the old sequences deleted, the new ones
// taken (their last indices reset, their key sizes from the types; 0 for an empty one)
void __fastcall EulerInitialize(NiEulerRotKey* self, re::Edx, NiFloatKey* keys0, uint32_t numKeys0, KeyType type0,
                                NiFloatKey* keys1, uint32_t numKeys1, KeyType type1, NiFloatKey* keys2,
                                uint32_t numKeys2, KeyType type2) {
    EulerRotKey_CleanUp(self);
    self->numKeys[0] = numKeys0;
    self->type[0] = type0;
    self->keys[0] = keys0;
    self->keys[1] = keys1;
    self->numKeys[1] = numKeys1;
    self->numKeys[2] = numKeys2;
    self->type[1] = type1;
    self->type[2] = type2;
    self->keys[2] = keys2;
    self->lastIdx[0] = 0;
    self->lastIdx[1] = 0;
    self->lastIdx[2] = 0;
    for (uint32_t i = 0; i < 3; i++) {
        self->size[i] = self->numKeys[i] == 0 ? 0 : ms_keySizes[self->type[i]];
    }
}

// ---- NiLinRotKey (its functions shared with NiStepRotKey's)

// 0x7FC5B0 Register
uint32_t __cdecl LinRegister() {
    if (NiLinRotKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 13, 0);
    Set(ms_interpD1, 13, 0);
    Set(ms_interpD2, 13, 0);
    NiLinRotKey_registered = 1;
    Set(ms_create, 13, 0x007FC3E0);
    Set(ms_save, 13, 0x007FEDB0);
    Set(ms_copy, 13, 0x007FB8E0);
    Set(ms_array, 13, 0x007FED20);
    Set(ms_delete, 13, 0x007FEDE0);
    ms_keySizes[13] = 0x14;
    Set(ms_interp, 13, 0x007FC390);
    Set(ms_equal, 13, 0x007FE630);
    Set(ms_fillDerived, 13, 0x007FC350);
    Set(ms_insert, 13, 0x007FC450);
    return 1;
}

// 0x7FED20 NewArray(count)
NiAnimationKey* __cdecl LinNewArray(uint32_t count) {
    return NewConstructedKeys(count, 0x14, kRotKeyConstruct);
}

// 0x7FEDB0 SaveBinary(stream, keys, count)
void __cdecl LinSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiRotKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        RotKey_SaveKey(stream, key);
    }
}

// ---- NiTCBRotKey

// 0x7FD510 CalculateDVals(previous key, next key): the key's A and B quaternions from the logs of
// the rotations to and from its neighbours, weighted by tension, continuity, bias and the spans
void __fastcall TCBCalculateDVals(NiTCBRotKey* self, re::Edx, const NiTCBRotKey* prev, const NiTCBRotKey* next) {
    NiQuaternion inverse;
    NiQuaternion product;
    NiQuaternion logIn;
    NiQuaternion logOut;
    Quaternion_Log(&logIn, Quaternion_Multiply(Quaternion_UnitInverse(&inverse, &prev->quat), &product, &self->quat));
    Quaternion_Log(&logOut, Quaternion_Multiply(Quaternion_UnitInverse(&product, &self->quat), &inverse, &next->quat));
    float invDuration = 1.0f / (next->time - prev->time);
    float minusTension = 1.0f - self->tension;
    float minusContinuity = 1.0f - self->continuity;
    float plusContinuity = self->continuity + 1.0f;
    float minusBias = 1.0f - self->bias;
    float plusBias = 1.0f + self->bias;

    // A = q exp(((s2 logOut + s1 logIn) - logOut) / 2)
    float adjust = (self->time - prev->time) * invDuration;
    float k = adjust * minusTension;
    float s1 = (k * plusContinuity) * plusBias;
    NiQuaternion t1;
    Quaternion_ScalarTimes(&t1, s1, &logIn);
    float s2 = (minusContinuity * k) * minusBias;
    NiQuaternion t2;
    Quaternion_ScalarTimes(&t2, s2, &logOut);
    NiQuaternion sum;
    Quaternion_Add(&t2, &sum, &t1);
    NiQuaternion difference;
    Quaternion_Subtract(&sum, &difference, &logOut);
    NiQuaternion half;
    NiQuaternion exp;
    Quaternion_Exp(&exp, Quaternion_ScalarTimes(&half, 0.5f, &difference));
    NiQuaternion a;
    self->a = *Quaternion_Multiply(&self->quat, &a, &exp);

    // B = q exp((logIn - (s4 logOut + s3 logIn)) / 2)
    adjust = (next->time - self->time) * invDuration;
    k = adjust * minusTension;
    float s3 = (k * minusContinuity) * plusBias;
    NiQuaternion u1;
    Quaternion_ScalarTimes(&u1, s3, &logIn);
    float s4 = (plusContinuity * k) * minusBias;
    NiQuaternion u2;
    Quaternion_ScalarTimes(&u2, s4, &logOut);
    Quaternion_Add(&u2, &sum, &u1);
    Quaternion_Subtract(&logIn, &difference, &sum);
    Quaternion_Exp(&exp, Quaternion_ScalarTimes(&half, 0.5f, &difference));
    // (the product written over its operand, as the game passes them)
    self->b = *Quaternion_Multiply(&self->quat, &exp, &exp);
}

// 0x7FD790 FillDerivedVals(keys, count, size): NiRotKey's, then each key's A and B from its
// neighbours (the ends against themselves). The keys taken as 0x40 bytes apart
void __cdecl TCBFillDerivedVals(NiAnimationKey* keys, uint32_t numKeys, uint8_t size) {
    if (numKeys < 2) {
        return;
    }
    RotKey_FillDerivedVals(keys, numKeys, size);
    auto* key = static_cast<NiTCBRotKey*>(keys);
    TCBRotKey_CalculateDVals(&key[0], &key[0], &key[1]);
    uint32_t last = numKeys - 1;
    for (uint32_t i = 1; i < last; i++) {
        TCBRotKey_CalculateDVals(&key[i], &key[i - 1], &key[i + 1]);
    }
    TCBRotKey_CalculateDVals(&key[last], &key[last - 1], &key[last]);
}

// 0x7FD810 Equal: NiRotKey's, tension, continuity, bias, A, B
uint8_t __cdecl TCBEqual(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = RotKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    auto* a = static_cast<const NiTCBRotKey*>(key0);
    auto* b = static_cast<const NiTCBRotKey*>(key1);
    if (b->tension != a->tension || b->continuity != a->continuity || b->bias != a->bias) {
        return 0;
    }
    if (Quaternion_Equal(&a->a, &b->a) == 0 || Quaternion_Equal(&a->b, &b->b) == 0) {
        return 0;
    }
    return 1;
}

// 0x7FD890 NewArray(count)
NiAnimationKey* __cdecl TCBNewArray(uint32_t count) {
    return NewConstructedKeys(count, 0x40, kRotKeyConstruct);
}

// 0x7FDAE0 SaveKey(stream, key): NiRotKey's, tension, continuity, bias
void __cdecl TCBSaveKey(void* stream, const NiTCBRotKey* key) {
    RotKey_SaveKey(stream, key);
    ni::scene::SaveBytes(stream, &key->tension, 4);
    ni::scene::SaveBytes(stream, &key->continuity, 4);
    ni::scene::SaveBytes(stream, &key->bias, 4);
}

// 0x7FDBE0 SaveBinary(stream, keys, count)
void __cdecl TCBSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiTCBRotKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        TCBRotKey_SaveKey(stream, key);
    }
}

// 0x7FDC10 Register
uint32_t __cdecl TCBRegister() {
    if (NiTCBRotKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 15, 0);
    Set(ms_interpD1, 15, 0);
    Set(ms_interpD2, 15, 0);
    NiTCBRotKey_registered = 1;
    Set(ms_create, 15, 0x007FDB70);
    Set(ms_save, 15, 0x007FDBE0);
    Set(ms_copy, 15, 0x007FD450);
    Set(ms_array, 15, 0x007FD890);
    Set(ms_delete, 15, 0x007FD8E0);
    ms_keySizes[15] = 0x40;
    Set(ms_interp, 15, 0x007FD4C0);
    Set(ms_equal, 15, 0x007FD810);
    Set(ms_fillDerived, 15, 0x007FD790);
    Set(ms_insert, 15, 0x007FD900);
    return 1;
}

// ---- NiStepRotKey

// 0x7FEF60 Register
uint32_t __cdecl StepRegister() {
    if (NiStepRotKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 17, 0);
    Set(ms_interpD1, 17, 0);
    Set(ms_interpD2, 17, 0);
    NiStepRotKey_registered = 1;
    Set(ms_create, 17, 0x007FC3E0);
    Set(ms_save, 17, 0x007FEDB0);
    Set(ms_copy, 17, 0x007FB8E0);
    Set(ms_array, 17, 0x007FED20);
    Set(ms_delete, 17, 0x007FEDE0);
    ms_keySizes[17] = 0x14;
    Set(ms_interp, 17, 0x007FED70);
    Set(ms_equal, 17, 0x007FE630);
    Set(ms_fillDerived, 17, 0x007FC350);
    Set(ms_insert, 17, 0x007FEE00);
    return 1;
}

} // namespace
} // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIROTKEY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

using namespace ni::anim;

NIROTKEY_REVERSED("NiRotKey::SaveKey", 0x007F9E20, SaveKey);
NIROTKEY_REVERSED("NiQuaternion::operator==", 0x007F9E40, QuaternionEqual);
NIROTKEY_REVERSED("NiRotKey::GenInterp", 0x007F9E90, GenInterp);
NIROTKEY_REVERSED("NiRotKey::FillDerivedVals", 0x007FA1F0, FillDerivedVals);
NIROTKEY_REVERSED("NiRotKey::Register", 0x007FA3F0, Register);
NIROTKEY_REVERSED("NiBezRotKey::FillDerivedVals", 0x007FA520, BezFillDerivedVals);
NIROTKEY_REVERSED("NiBezRotKey::Equal", 0x007FA600, BezEqual);
NIROTKEY_REVERSED("NiBezRotKey::SaveBinary", 0x007FA630, BezSaveBinary);
NIROTKEY_REVERSED("NiBezRotKey::NewArray", 0x007FA660, BezNewArray);
NIROTKEY_REVERSED("NiBezRotKey::Register", 0x007FA8A0, BezRegister);
NIROTKEY_REVERSED("NiRotKey::Equal", 0x007FB0E0, Equal);
// (lone jmps to the two above: NiLinRotKey's and NiStepRotKey's FillDerivedVals; NiLinColorKey's,
// NiStepColorKey's, NiLinRotKey's and NiStepRotKey's Equal)
RE_UNHOOKED_FN("NiLinRotKey::FillDerivedVals", 0x007FC350, &FillDerivedVals);
RE_UNHOOKED_FN("NiLinColorKey::Equal", 0x007FE630, &Equal);
NIROTKEY_REVERSED("NiEulerRotKey::NiEulerRotKey", 0x007FB190, EulerConstruct);
NIROTKEY_REVERSED("NiEulerRotKey::CleanUp", 0x007FB1F0, EulerCleanUp);
NIROTKEY_REVERSED("NiEulerRotKey::Interp", 0x007FB230, EulerInterp);
NIROTKEY_REVERSED("NiEulerRotKey::Equal", 0x007FB340, EulerEqual);
NIROTKEY_REVERSED("NiEulerRotKey::Copy", 0x007FB410, EulerCopy);
NIROTKEY_REVERSED("NiEulerRotKey::NewArray", 0x007FB4D0, EulerNewArray);
NIROTKEY_REVERSED("NiEulerRotKey::FillDerivedVals", 0x007FB540, EulerFillDerivedVals);
NIROTKEY_REVERSED("NiEulerRotKey::SaveKey", 0x007FB720, EulerSaveKey);
NIROTKEY_REVERSED("NiEulerRotKey::SaveBinary", 0x007FB820, EulerSaveBinary);
NIROTKEY_REVERSED("NiEulerRotKey::Register", 0x007FB850, EulerRegister);
NIROTKEY_REVERSED("NiRotKey::NiRotKey", 0x007FC360, Construct);
NIROTKEY_REVERSED("NiLinRotKey::Register", 0x007FC5B0, LinRegister);
NIROTKEY_REVERSED("NiTCBRotKey::CalculateDVals", 0x007FD510, TCBCalculateDVals);
NIROTKEY_REVERSED("NiTCBRotKey::FillDerivedVals", 0x007FD790, TCBFillDerivedVals);
NIROTKEY_REVERSED("NiTCBRotKey::Equal", 0x007FD810, TCBEqual);
NIROTKEY_REVERSED("NiTCBRotKey::NewArray", 0x007FD890, TCBNewArray);
NIROTKEY_REVERSED("NiTCBRotKey::SaveKey", 0x007FDAE0, TCBSaveKey);
NIROTKEY_REVERSED("NiTCBRotKey::SaveBinary", 0x007FDBE0, TCBSaveBinary);
NIROTKEY_REVERSED("NiTCBRotKey::Register", 0x007FDC10, TCBRegister);
NIROTKEY_REVERSED("NiLinRotKey::NewArray", 0x007FED20, LinNewArray);
NIROTKEY_REVERSED("NiLinRotKey::SaveBinary", 0x007FEDB0, LinSaveBinary);
NIROTKEY_REVERSED("NiStepRotKey::Register", 0x007FEF60, StepRegister);
NIROTKEY_REVERSED("NiEulerRotKey::Initialize", 0x007FF6E0, EulerInitialize);
