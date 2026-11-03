// The color and bool keys (src/gamebryo/NiAnimationKey.h): NiColorKey (content COLORKEY,
// NOINTERP), NiLinColorKey, NiStepColorKey, NiBoolKey (content BOOLKEY, NOINTERP) and
// NiStepBoolKey. GenInterp (and the bool's clamped form), the linear color blend, comparison,
// saving, inserting a bool key, the arrays, and each kind's Register (its table entries, once).
//
// Main-thread float maths at the game's 24-bit x87 precision: each operation a float one, in the
// game's order.

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

GAME_VAR(uint8_t, NiBoolKey_registered, 0x020E4342);
GAME_VAR(uint8_t, NiColorKey_registered, 0x020E4343);
GAME_VAR(uint8_t, NiLinColorKey_registered, 0x020E4355);
GAME_VAR(uint8_t, NiStepBoolKey_registered, 0x020E435C);
GAME_VAR(uint8_t, NiStepColorKey_registered, 0x020E435D);
GAME_VAR(uint8_t, NiBoolKey_ms_bDefault, 0x020E4341);

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(void, __thiscall, ColorA_SaveBinary, 0x00762670, const NiColorA* self, void* stream);
GAME_FN(void, __cdecl, AnimationKey_SaveBinary, 0x007FFC10, void* stream, const NiAnimationKey* key);
GAME_FN(uint8_t, __cdecl, AnimationKey_Equal, 0x007FF6C0, const NiAnimationKey* key0, const NiAnimationKey* key1);
GAME_FN(uint8_t, __cdecl, AnimationKey_OkayToInsert, 0x007FF770, float time, const NiAnimationKey* keys,
        uint32_t numKeys, uint32_t* index, uint8_t size);
// (this file's, called where modes apply)
GAME_FN(void, __cdecl, BoolKey_RegisterSupportedFunctions, 0x007FA970, KeyContent content, KeyType type);
GAME_FN(uint8_t, __cdecl, BoolKey_GenInterp, 0x007FA9D0, float time, NiBoolKey* keys, KeyType type,
        uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(uint8_t, __cdecl, BoolKey_GenInterpDefault, 0x007FABD0, float time, NiBoolKey* keys, KeyType type,
        uint32_t numKeys, uint8_t size);
GAME_FN(void, __cdecl, BoolKey_SaveKey, 0x007FAD20, void* stream, const NiBoolKey* key);
GAME_FN(void, __cdecl, ColorKey_SaveKey, 0x007FAD70, void* stream, const NiColorKey* key);

constexpr uintptr_t kColorKeyConstruct = 0x007FE610;
constexpr uintptr_t kNothing = 0x0049A650;      // a function doing nothing (returns)
constexpr uintptr_t kReturnZero = 0x00824EC0;   // a function returning 0
constexpr uintptr_t kNoCurvature = 0x00515A50;  // the curvature of keys without one

template <typename T>
inline void Set(T* table, uint32_t index, uintptr_t address) {
    table[index] = re::PtrCast<T>(address);
}

// ---- NiBoolKey

// 0x7FA930 Equal: the times and the values (as bytes)
uint8_t __cdecl BoolEqual(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    uint8_t equal = AnimationKey_Equal(key0, key1);
    if (equal == 0) {
        return equal;
    }
    return static_cast<const NiBoolKey*>(key0)->value == static_cast<const NiBoolKey*>(key1)->value ? 1 : 0;
}

// 0x7FA960 Interp (and its derivatives): the default bool
void __cdecl BoolInterp(float, const NiAnimationKey*, const NiAnimationKey*, void* result) {
    *static_cast<uint8_t*>(result) = NiBoolKey_ms_bDefault;
}

// 0x7FA970 RegisterSupportedFunctions(content, type): NOINTERP's per-content entries
void __cdecl BoolRegisterSupportedFunctions(KeyContent content, KeyType type) {
    uint32_t i = KeyIndex(content, type);
    Set(ms_curvature, i, kNoCurvature);
    Set(ms_interp, i, 0x007FA960);
    Set(ms_interpD1, i, 0x007FA960);
    Set(ms_interpD2, i, 0x007FA960);
    Set(ms_equal, i, 0x007FA930);
    Set(ms_fillDerived, i, kNothing);
    Set(ms_insert, i, 0);
}

// 0x7FA9D0 GenInterp(time, keys, type, count, last index, size): one key's value; else the
// kind's Interp between the keys around `time`, into a slot holding the normalized time (the low
// byte of that read back)
uint8_t __cdecl BoolGenInterp(float time, NiBoolKey* keys, KeyType type, uint32_t numKeys, uint32_t* lastIdx,
                              uint8_t size) {
    if (numKeys == 1) {
        return keys->value;
    }
    float t0;
    float t1;
    uint32_t next = FindKeyInterval(time, keys, numKeys, *lastIdx, size, t0, t1);
    float slot = (time - t0) / (t1 - t0);
    ms_interp[KeyIndex(BOOLKEY, type)](slot, KeyAt(keys, *lastIdx, size), KeyAt(keys, next, size), &slot);
    uint8_t value;
    std::memcpy(&value, &slot, 1);
    return value;
}

// 0x7FABD0 GenInterpDefault(time, keys, type, count, size): the default without keys, the first
// or last value outside their times, else GenInterp from the first key
uint8_t __cdecl BoolGenInterpDefault(float time, NiBoolKey* keys, KeyType type, uint32_t numKeys, uint8_t size) {
    if (numKeys == 0) {
        return NiBoolKey_ms_bDefault;
    }
    if (keys->time > time) {
        return keys->value;
    }
    NiBoolKey* last = KeyAt<NiBoolKey>(keys, numKeys - 1, size);
    if (last->time < time) {
        return last->value;
    }
    uint32_t lastIdx = 0;
    return BoolKey_GenInterp(time, keys, type, numKeys, &lastIdx, size);
}

// 0x7FAC50 Register
uint32_t __cdecl BoolRegister() {
    if (NiBoolKey_registered != 0) {
        return 0;
    }
    NiBoolKey_registered = 1;
    Set(ms_create, 30, kReturnZero);
    Set(ms_save, 30, kNothing);
    Set(ms_copy, 30, kNothing);
    Set(ms_array, 30, kReturnZero);
    Set(ms_delete, 30, kNothing);
    ms_keySizes[30] = 8;
    BoolKey_RegisterSupportedFunctions(BOOLKEY, NOINTERP);
    return 1;
}

// 0x7FAD20 SaveKey(stream, key): the time, the value as a 0 / 1 byte
void __cdecl BoolSaveKey(void* stream, const NiBoolKey* key) {
    AnimationKey_SaveBinary(stream, key);
    uint8_t value = key->value != 0 ? 1 : 0;
    ni::scene::SaveBytes(stream, &value, 1);
}

// ---- NiStepBoolKey

// 0x7FE3E0 SaveBinary(stream, keys, count)
void __cdecl StepBoolSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiBoolKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        BoolKey_SaveKey(stream, key);
    }
}

// 0x7FE410 Insert(time, keys, count): a key at `time` with the value there (none where one is;
// only the value's byte written)
uint8_t __cdecl StepBoolInsert(float time, re::Ptr32<NiAnimationKey>* keysRef, uint32_t* numKeys) {
    auto* old = static_cast<NiBoolKey*>(*keysRef);
    uint32_t at;
    if (AnimationKey_OkayToInsert(time, old, *numKeys, &at, 8) == 0) {
        return 0;
    }
    uint32_t count = *numKeys + 1;
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(KeyArraySize(count, 8)));
    NiBoolKey* keys = nullptr;
    if (block != nullptr) {
        *block = count;
        keys = reinterpret_cast<NiBoolKey*>(block + 1);
    }
    crt::MemcpyS(keys, *numKeys * 8 + 8, old, at * 8);
    if (*numKeys > at) {
        uint32_t rest = (*numKeys - at) * 8;
        crt::MemcpyS(&keys[at + 1], rest, &old[at], rest);
    }
    uint8_t value = BoolKey_GenInterpDefault(time, old, STEPKEY, *numKeys, 8);
    keys[at].time = time;
    keys[at].value = value;
    (*numKeys)++;
    if (old != nullptr) {
        NiDeleteArray(reinterpret_cast<uint8_t*>(old) - 4, 8);
    }
    *keysRef = keys;
    return 1;
}

// 0x7FE580 Register
uint32_t __cdecl StepBoolRegister() {
    if (NiStepBoolKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 35, 0);
    Set(ms_interpD1, 35, 0);
    Set(ms_interpD2, 35, 0);
    NiStepBoolKey_registered = 1;
    Set(ms_create, 35, 0x007FE520);
    Set(ms_save, 35, 0x007FE3E0);
    Set(ms_copy, 35, 0x007FE390);
    Set(ms_array, 35, 0x007FBD60);
    Set(ms_delete, 35, 0x007FBE30);
    ms_keySizes[35] = 8;
    Set(ms_interp, 35, 0x007FE3C0);
    Set(ms_equal, 35, 0x007FE3B0);
    Set(ms_fillDerived, 35, kNothing);
    Set(ms_insert, 35, 0x007FE410);
    return 1;
}

// ---- NiColorKey

// 0x7FAD70 SaveKey(stream, key): the time, the color
void __cdecl ColorSaveKey(void* stream, const NiColorKey* key) {
    AnimationKey_SaveBinary(stream, key);
    ColorA_SaveBinary(&key->color, stream);
}

// 0x7FADC0 GenInterp(result, time, keys, type, count, last index, size): one key's color; else the
// kind's Interp between the keys around `time` (into a zeroed color)
NiColorA* __cdecl ColorGenInterp(NiColorA* result, float time, NiColorKey* keys, KeyType type, uint32_t numKeys,
                                 uint32_t* lastIdx, uint8_t size) {
    if (numKeys == 1) {
        *result = keys->color;
        return result;
    }
    float t0;
    float t1;
    uint32_t next = FindKeyInterval(time, keys, numKeys, *lastIdx, size, t0, t1);
    float t = (time - t0) / (t1 - t0);
    NiColorA value = {0.0f, 0.0f, 0.0f, 0.0f};
    ms_interp[KeyIndex(COLORKEY, type)](t, KeyAt(keys, *lastIdx, size), KeyAt(keys, next, size), &value);
    *result = value;
    return result;
}

// 0x7FB110 Register: COLORKEY/NOINTERP's entries (Interp the game's 0x7FAD90)
uint32_t __cdecl ColorRegister() {
    if (NiColorKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 18, 0);
    Set(ms_interpD1, 18, 0);
    Set(ms_interpD2, 18, 0);
    Set(ms_fillDerived, 18, 0);
    Set(ms_insert, 18, 0);
    NiColorKey_registered = 1;
    Set(ms_create, 18, kReturnZero);
    Set(ms_save, 18, kNothing);
    Set(ms_copy, 18, kNothing);
    Set(ms_array, 18, kReturnZero);
    Set(ms_delete, 18, kNothing);
    ms_keySizes[18] = 0x14;
    Set(ms_interp, 18, 0x007FAD90);
    Set(ms_equal, 18, 0x007FB0E0);
    return 1;
}

// 0x7FE610 NiColorKey(): the color all zero
NiColorKey* __fastcall ColorConstruct(NiColorKey* self, re::Edx) {
    self->color.r = 0.0f;
    self->color.g = 0.0f;
    self->color.b = 0.0f;
    self->color.a = 0.0f;
    return self;
}

// ---- NiLinColorKey (its functions shared with NiStepColorKey's)

// 0x7FB910 NewArray(count): each key's color zeroed
NiAnimationKey* __cdecl LinColorNewArray(uint32_t count) {
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(KeyArraySize(count, 0x14)));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, 0x14, count, reinterpret_cast<void*>(kColorKeyConstruct));
    return reinterpret_cast<NiAnimationKey*>(block + 1);
}

// 0x7FB960 Interp: (1 - t) c0 + t c1
void __cdecl LinColorInterp(float t, const NiAnimationKey* key0, const NiAnimationKey* key1, void* result) {
    NiColorA c0 = static_cast<const NiColorKey*>(key0)->color;
    NiColorA c1 = static_cast<const NiColorKey*>(key1)->color;
    float s = 1.0f - t;
    float ar = c0.r * s;
    float ag = c0.g * s;
    float ab = c0.b * s;
    float aa = s * c0.a;
    float br = c1.r * t;
    float bg = c1.g * t;
    float bb = c1.b * t;
    float ba = t * c1.a;
    auto* color = static_cast<NiColorA*>(result);
    color->r = br + ar;
    color->g = bg + ag;
    color->b = bb + ab;
    color->a = ba + aa;
}

// 0x7FBAF0 SaveBinary(stream, keys, count)
void __cdecl LinColorSaveBinary(void* stream, NiAnimationKey* keys, uint32_t numKeys) {
    auto* key = static_cast<NiColorKey*>(keys);
    for (; numKeys != 0; numKeys--, key++) {
        ColorKey_SaveKey(stream, key);
    }
}

// 0x7FBC70 Register
uint32_t __cdecl LinColorRegister() {
    if (NiLinColorKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 19, 0);
    Set(ms_interpD1, 19, 0);
    Set(ms_interpD2, 19, 0);
    Set(ms_fillDerived, 19, 0);
    NiLinColorKey_registered = 1;
    Set(ms_create, 19, 0x007FBA80);
    Set(ms_save, 19, 0x007FBAF0);
    Set(ms_copy, 19, 0x007FB8E0);
    Set(ms_array, 19, 0x007FB910);
    Set(ms_delete, 19, 0x007FEDE0);
    ms_keySizes[19] = 0x14;
    Set(ms_interp, 19, 0x007FB960);
    Set(ms_equal, 19, 0x007FE630);
    Set(ms_insert, 19, 0x007FBB20);
    return 1;
}

// ---- NiStepColorKey

// 0x7FE790 Register
uint32_t __cdecl StepColorRegister() {
    if (NiStepColorKey_registered != 0) {
        return 0;
    }
    Set(ms_curvature, 23, 0);
    Set(ms_interpD1, 23, 0);
    Set(ms_interpD2, 23, 0);
    Set(ms_fillDerived, 23, 0);
    NiStepColorKey_registered = 1;
    Set(ms_create, 23, 0x007FBA80);
    Set(ms_save, 23, 0x007FBAF0);
    Set(ms_copy, 23, 0x007FB8E0);
    Set(ms_array, 23, 0x007FB910);
    Set(ms_delete, 23, 0x007FEDE0);
    ms_keySizes[23] = 0x14;
    Set(ms_interp, 23, 0x007FED70);
    Set(ms_equal, 23, 0x007FE630);
    Set(ms_insert, 23, 0x007FE640);
    return 1;
}

} // namespace
} // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NICOLORKEY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

using namespace ni::anim;

NICOLORKEY_REVERSED("NiBoolKey::Equal", 0x007FA930, BoolEqual);
RE_UNHOOKED_FN("NiStepBoolKey::Equal", 0x007FE3B0, &BoolEqual);   // (a lone jmp to it)
NICOLORKEY_REVERSED("NiBoolKey::Interp", 0x007FA960, BoolInterp);
NICOLORKEY_REVERSED("NiBoolKey::RegisterSupportedFunctions", 0x007FA970, BoolRegisterSupportedFunctions);
NICOLORKEY_REVERSED("NiBoolKey::GenInterp", 0x007FA9D0, BoolGenInterp);
NICOLORKEY_REVERSED("NiBoolKey::GenInterpDefault", 0x007FABD0, BoolGenInterpDefault);
NICOLORKEY_REVERSED("NiBoolKey::Register", 0x007FAC50, BoolRegister);
NICOLORKEY_REVERSED("NiBoolKey::SaveKey", 0x007FAD20, BoolSaveKey);
NICOLORKEY_REVERSED("NiColorKey::SaveKey", 0x007FAD70, ColorSaveKey);
NICOLORKEY_REVERSED("NiColorKey::GenInterp", 0x007FADC0, ColorGenInterp);
NICOLORKEY_REVERSED("NiColorKey::Register", 0x007FB110, ColorRegister);
NICOLORKEY_REVERSED("NiLinColorKey::NewArray", 0x007FB910, LinColorNewArray);
NICOLORKEY_REVERSED("NiLinColorKey::Interp", 0x007FB960, LinColorInterp);
NICOLORKEY_REVERSED("NiLinColorKey::SaveBinary", 0x007FBAF0, LinColorSaveBinary);
NICOLORKEY_REVERSED("NiLinColorKey::Register", 0x007FBC70, LinColorRegister);
NICOLORKEY_REVERSED("NiStepBoolKey::SaveBinary", 0x007FE3E0, StepBoolSaveBinary);
NICOLORKEY_REVERSED("NiStepBoolKey::Insert", 0x007FE410, StepBoolInsert);
NICOLORKEY_REVERSED("NiStepBoolKey::Register", 0x007FE580, StepBoolRegister);
NICOLORKEY_REVERSED("NiColorKey::NiColorKey", 0x007FE610, ColorConstruct);
NICOLORKEY_REVERSED("NiStepColorKey::Register", 0x007FE790, StepColorRegister);
