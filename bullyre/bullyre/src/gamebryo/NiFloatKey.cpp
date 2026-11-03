// The float keys (src/gamebryo/NiAnimationKey.h): NiFloatKey (content FLOATKEY, NOINTERP),
// NiLinFloatKey, NiBezFloatKey, NiTCBFloatKey and NiStepFloatKey. Interpolation between the two
// keys around a time (GenInterp), comparison, saving, the TCB keys' tangents, the array
// allocation the kinds share, and each kind's Register (its table entries, once).
//
// Main-thread float maths at the game's 24-bit x87 precision: each operation a float one, in the
// game's order.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiAnimationKey.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

GAME_VAR(uint8_t, NiFloatKey_registered, 0x020E4318);
GAME_VAR(uint8_t, NiBezFloatKey_registered, 0x020E4319);
GAME_VAR(uint8_t, NiTCBFloatKey_registered, 0x020E435B);
GAME_VAR(uint8_t, NiLinFloatKey_registered, 0x020E4356);
GAME_VAR(uint8_t, NiStepFloatKey_registered, 0x020E435E);

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, AnimationKey_SaveBinary, 0x007FFC10, void* stream, const NiAnimationKey* key);
GAME_FN(uint8_t, __cdecl, AnimationKey_Equal, 0x007FF6C0, const NiAnimationKey* key0, const NiAnimationKey* key1);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __cdecl, FloatKey_Equal, 0x007F82C0, const NiAnimationKey* key0, const NiAnimationKey* key1);
GAME_FN(void, __cdecl, FloatKey_SaveKey, 0x007F8650, void* stream, const NiFloatKey* key);
GAME_FN(void, __cdecl, TCBFloatKey_CalculateDVals, 0x007FDDF0, NiTCBFloatKey* key, float prevValue,
        float nextValue, float preLength, float nextLength);
GAME_FN(void, __cdecl, TCBFloatKey_SaveKey, 0x007FE1E0, void* stream, const NiTCBFloatKey* key);

// a table entry set to the game's function at `address` (0: none)
template <typename T>
inline void Set(T* table, uint32_t index, uintptr_t address) {
    table[index] = re::PtrCast<T>(address);
}

constexpr uintptr_t kNothing = 0x0049A650;      // a function doing nothing (returns)
constexpr uintptr_t kReturnZero = 0x00824EC0;   // a function returning 0

// NiNew[] of count keys `size` bytes each, the count before them (no constructor)
inline NiAnimationKey* NewKeys(uint32_t count, uint32_t size) {
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(KeyArraySize(count, size)));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    return reinterpret_cast<NiAnimationKey*>(block + 1);
}

// ---- NiFloatKey

// 0x7F82C0 Equal: the times and the values
uint8_t __cdecl Equal(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = AnimationKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    return static_cast<const NiFloatKey*>(key1)->value == static_cast<const NiFloatKey*>(key0)->value ? 1 : 0;
}

// 0x7F8300 GenInterp(time, keys, type, count, last index, size): one key's value; else the kind's
// Interp between the keys around `time` at the normalized time (its result slot starts as that)
float __cdecl GenInterp(float time, NiFloatKey* keys, KeyType type, uint32_t numKeys, uint32_t* lastIdx,
                        uint8_t size) {
    if (numKeys == 1) {
        return keys->value;
    }
    float t0;
    float t1;
    uint32_t next = FindKeyInterval(time, keys, numKeys, *lastIdx, size, t0, t1);
    float result = (time - t0) / (t1 - t0);
    ms_interp[KeyIndex(FLOATKEY, type)](result, KeyAt(keys, *lastIdx, size), KeyAt(keys, next, size), &result);
    return result;
}

// 0x7F8580 Register: FLOATKEY/NOINTERP's entries (no interpolation: Interp does nothing, Create
// and NewArray return null). 0 when it ran before
uint32_t __cdecl Register() {
    if (NiFloatKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 0, 0);
    Set(ms_interpD1, 0, 0);
    Set(ms_interpD2, 0, 0);
    Set(ms_insert, 0, 0);
    NiFloatKey_registered = 1;
    Set(ms_create, 0, kReturnZero);
    Set(ms_save, 0, kNothing);
    Set(ms_copy, 0, kNothing);
    Set(ms_array, 0, kReturnZero);
    Set(ms_delete, 0, kNothing);
    ms_keySizes[0] = 8;
    Set(ms_interp, 0, kNothing);
    Set(ms_equal, 0, 0x007F82C0);
    Set(ms_fillDerived, 0, kNothing);
    return 1;
}

// 0x7F8650 SaveKey(stream, key): the time, the value
void __cdecl SaveKey(void* stream, const NiFloatKey* key) {
    AnimationKey_SaveBinary(stream, key);
    ni::scene::SaveBytes(stream, &key->value, 4);
}

// ---- NiBezFloatKey

// 0x7F8690 Equal: the float key's, then the tangents
uint8_t __cdecl BezEqual(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = FloatKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    auto* a = static_cast<const NiBezFloatKey*>(key0);
    auto* b = static_cast<const NiBezFloatKey*>(key1);
    return (b->inTangent == a->inTangent && b->outTangent == a->outTangent) ? 1 : 0;
}

// 0x7F8A00 SaveBinary(stream, keys, count): each key, then its tangents
void __cdecl BezSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiBezFloatKey*>(keys);
    for (uint32_t i = 0; i < numKeys; i++, key++) {
        FloatKey_SaveKey(stream, key);
        ni::scene::SaveBytes(stream, &key->inTangent, 4);
        ni::scene::SaveBytes(stream, &key->outTangent, 4);
    }
}

// 0x7F8A80 Register
uint32_t __cdecl BezRegister() {
    if (NiBezFloatKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 2, 0);
    Set(ms_interpD1, 2, 0);
    Set(ms_interpD2, 2, 0);
    NiBezFloatKey_registered = 1;
    Set(ms_create, 2, 0x007F8940);
    Set(ms_save, 2, 0x007F8A00);
    Set(ms_copy, 2, 0x007F86E0);
    Set(ms_array, 2, 0x007FC020);
    Set(ms_delete, 2, 0x007FC150);
    ms_keySizes[2] = 0x10;
    Set(ms_interp, 2, 0x007F8700);
    Set(ms_equal, 2, 0x007F8690);
    Set(ms_fillDerived, 2, kNothing);
    Set(ms_insert, 2, 0x007F8760);
    return 1;
}

// ---- NiLinFloatKey (its functions shared with NiStepFloatKey's; NewArray with NiStepBoolKey's)

// 0x7FBD60 NewArray(count)
NiAnimationKey* __cdecl LinNewArray(uint32_t count) {
    return NewKeys(count, 8);
}

// 0x7FBE00 SaveBinary(stream, keys, count)
void __cdecl LinSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiFloatKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        FloatKey_SaveKey(stream, key);
    }
}

// 0x7FBF60 Register
uint32_t __cdecl LinRegister() {
    if (NiLinFloatKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 1, 0);
    Set(ms_interpD1, 1, 0);
    Set(ms_interpD2, 1, 0);
    NiLinFloatKey_registered = 1;
    Set(ms_create, 1, 0x007FBDA0);
    Set(ms_save, 1, 0x007FBE00);
    Set(ms_copy, 1, 0x007FBD00);
    Set(ms_array, 1, 0x007FBD60);
    Set(ms_delete, 1, 0x007FBE30);
    ms_keySizes[1] = 8;
    Set(ms_interp, 1, 0x007FBD20);
    Set(ms_equal, 1, 0x007FE820);
    Set(ms_fillDerived, 1, kNothing);
    Set(ms_insert, 1, 0x007FBE50);
    return 1;
}

// ---- NiTCBFloatKey

// 0x7FDCA0 Equal: the float key's, then tension, continuity, bias and the derived tangents
uint8_t __cdecl TCBEqual(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = FloatKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    auto* a = static_cast<const NiTCBFloatKey*>(key0);
    auto* b = static_cast<const NiTCBFloatKey*>(key1);
    return (b->tension == a->tension && b->continuity == a->continuity && b->bias == a->bias && b->ds == a->ds &&
            b->dd == a->dd)
               ? 1
               : 0;
}

// 0x7FDDB0 NewArray(count)
NiAnimationKey* __cdecl TCBNewArray(uint32_t count) {
    return NewKeys(count, 0x1C);
}

// 0x7FDDF0 CalculateDVals(key, previous value, next value, previous span, next span): the key's
// incoming and outgoing tangents from its tension, continuity and bias, scaled by the spans.
// (The game's FillDerivedVals keeps values in ecx and edx across its calls of this.)
void __cdecl TCBCalculateDVals(NiTCBFloatKey* key, float prevValue, float nextValue, float preLength,
                               float nextLength) {
    float diffIn = key->value - prevValue;
    float diffOut = nextValue - key->value;
    float onePlusBias = key->bias + 1.0f;
    float oneMinusBias = 1.0f - key->bias;
    float half = (1.0f - key->tension) * 0.5f;
    float onePlusContinuity = key->continuity + 1.0f;
    float halfPlus = onePlusContinuity * half;
    float oneMinusContinuity = 1.0f - key->continuity;
    float halfMinus = half * oneMinusContinuity;
    float invSum = 2.0f / (preLength + nextLength);
    float in = (halfPlus * oneMinusBias) * diffOut + (halfMinus * onePlusBias) * diffIn;
    key->ds = (preLength * invSum) * in;
    float out = (halfPlus * onePlusBias) * diffIn + diffOut * (oneMinusBias * halfMinus);
    key->dd = out * (nextLength * invSum);
}

// 0x7FDEF0 FillDerivedVals(keys, count, size): each key's tangents from its neighbours (the ends
// against a point mirrored through them, spans of 1). The keys taken as 0x1C bytes apart
void __cdecl TCBFillDerivedVals(NiAnimationKey* keys, uint32_t numKeys, uint8_t) {
    if (numKeys < 2) {
        return;
    }
    auto* key = static_cast<NiTCBFloatKey*>(keys);
    float first = (key[0].value + key[0].value) - key[1].value;
    TCBFloatKey_CalculateDVals(&key[0], first, key[1].value, 1.0f, 1.0f);
    uint32_t last = numKeys - 1;
    if (last > 1) {
        for (uint32_t i = 1; i < last; i++) {
            float nextLength = key[i + 1].time - key[i].time;
            float preLength = key[i].time - key[i - 1].time;
            TCBFloatKey_CalculateDVals(&key[i], key[i - 1].value, key[i + 1].value, preLength, nextLength);
        }
    }
    float end = (key[last].value + key[last].value) - key[last - 1].value;
    TCBFloatKey_CalculateDVals(&key[last], key[last - 1].value, end, 1.0f, 1.0f);
}

// 0x7FE1E0 SaveKey(stream, key): the float key's, then tension, continuity, bias
void __cdecl TCBSaveKey(void* stream, const NiTCBFloatKey* key) {
    FloatKey_SaveKey(stream, key);
    ni::scene::SaveBytes(stream, &key->tension, 4);
    ni::scene::SaveBytes(stream, &key->continuity, 4);
    ni::scene::SaveBytes(stream, &key->bias, 4);
}

// 0x7FE2D0 SaveBinary(stream, keys, count)
void __cdecl TCBSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiTCBFloatKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        TCBFloatKey_SaveKey(stream, key);
    }
}

// 0x7FE300 Register
uint32_t __cdecl TCBRegister() {
    if (NiTCBFloatKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 3, 0);
    Set(ms_interpD1, 3, 0);
    Set(ms_interpD2, 3, 0);
    NiTCBFloatKey_registered = 1;
    Set(ms_create, 3, 0x007FE270);
    Set(ms_save, 3, 0x007FE2D0);
    Set(ms_copy, 3, 0x007FDD20);
    Set(ms_array, 3, 0x007FDDB0);
    Set(ms_delete, 3, 0x007FDFE0);
    ms_keySizes[3] = 0x1C;
    Set(ms_interp, 3, 0x007FDD60);
    Set(ms_equal, 3, 0x007FDCA0);
    Set(ms_fillDerived, 3, 0x007FDEF0);
    Set(ms_insert, 3, 0x007FE000);
    return 1;
}

// ---- NiStepFloatKey

// 0x7FE970 Register
uint32_t __cdecl StepRegister() {
    if (NiStepFloatKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 5, 0);
    Set(ms_interpD1, 5, 0);
    Set(ms_interpD2, 5, 0);
    NiStepFloatKey_registered = 1;
    Set(ms_create, 5, 0x007FBDA0);
    Set(ms_save, 5, 0x007FBE00);
    Set(ms_copy, 5, 0x007FBD00);
    Set(ms_array, 5, 0x007FBD60);
    Set(ms_delete, 5, 0x007FBE30);
    ms_keySizes[5] = 8;
    Set(ms_interp, 5, 0x007FE830);
    Set(ms_equal, 5, 0x007FE820);
    Set(ms_fillDerived, 5, kNothing);
    Set(ms_insert, 5, 0x007FE860);
    return 1;
}

} // namespace
} // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIFLOATKEY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIFLOATKEY_REVERSED("NiFloatKey::Equal", 0x007F82C0, ni::anim::Equal);
RE_UNHOOKED_FN("NiLinFloatKey::Equal", 0x007FE820, &ni::anim::Equal);   // (a lone jmp to it: NiStepFloatKey's too)
NIFLOATKEY_REVERSED("NiFloatKey::GenInterp", 0x007F8300, ni::anim::GenInterp);
NIFLOATKEY_REVERSED("NiFloatKey::Register", 0x007F8580, ni::anim::Register);
NIFLOATKEY_REVERSED("NiFloatKey::SaveKey", 0x007F8650, ni::anim::SaveKey);
NIFLOATKEY_REVERSED("NiBezFloatKey::Equal", 0x007F8690, ni::anim::BezEqual);
NIFLOATKEY_REVERSED("NiBezFloatKey::SaveBinary", 0x007F8A00, ni::anim::BezSaveBinary);
NIFLOATKEY_REVERSED("NiBezFloatKey::Register", 0x007F8A80, ni::anim::BezRegister);
NIFLOATKEY_REVERSED("NiLinFloatKey::NewArray", 0x007FBD60, ni::anim::LinNewArray);
NIFLOATKEY_REVERSED("NiLinFloatKey::SaveBinary", 0x007FBE00, ni::anim::LinSaveBinary);
NIFLOATKEY_REVERSED("NiLinFloatKey::Register", 0x007FBF60, ni::anim::LinRegister);
NIFLOATKEY_REVERSED("NiTCBFloatKey::Equal", 0x007FDCA0, ni::anim::TCBEqual);
NIFLOATKEY_REVERSED("NiTCBFloatKey::NewArray", 0x007FDDB0, ni::anim::TCBNewArray);
// callers rely on ecx/edx: the original FillDerivedVals (0x7FDEF0) keeps the key and the array
// in them across its calls: turn on together with NiTCBFloatKey::FillDerivedVals
NIFLOATKEY_REVERSED("NiTCBFloatKey::CalculateDVals", 0x007FDDF0, ni::anim::TCBCalculateDVals);
NIFLOATKEY_REVERSED("NiTCBFloatKey::FillDerivedVals", 0x007FDEF0, ni::anim::TCBFillDerivedVals);
NIFLOATKEY_REVERSED("NiTCBFloatKey::SaveKey", 0x007FE1E0, ni::anim::TCBSaveKey);
NIFLOATKEY_REVERSED("NiTCBFloatKey::SaveBinary", 0x007FE2D0, ni::anim::TCBSaveBinary);
NIFLOATKEY_REVERSED("NiTCBFloatKey::Register", 0x007FE300, ni::anim::TCBRegister);
NIFLOATKEY_REVERSED("NiStepFloatKey::Register", 0x007FE970, ni::anim::StepRegister);
