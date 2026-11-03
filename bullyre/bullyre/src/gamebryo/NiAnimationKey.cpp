// NiAnimationKey's statics (src/gamebryo/NiAnimationKey.h): the time comparison and save every
// kind's start with, the insert position search, copying the keys of a time range (an Euler key
// recursing into its three sequences), making sure there are keys at both ends of a range, and
// the Bezier tangent split Insert uses.
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

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Construct, 0x00775B90, NiQuaternion* self, float w, float x, float y,
        float z);
// (this file's and the Euler key's, called where modes apply)
GAME_FN(void, __thiscall, EulerRotKey_Initialize, 0x007FF6E0, NiEulerRotKey* self, NiFloatKey* keys0,
        uint32_t numKeys0, KeyType type0, NiFloatKey* keys1, uint32_t numKeys1, KeyType type1, NiFloatKey* keys2,
        uint32_t numKeys2, KeyType type2);
GAME_FN(void, __cdecl, AnimationKey_CopySequence, 0x007FF7D0, KeyContent content, KeyType type,
        NiAnimationKey* keys, uint32_t numKeys, float startTime, float endTime, re::Ptr32<NiAnimationKey>* newKeys,
        uint32_t* newNumKeys);
GAME_FN(void, __cdecl, AnimationKey_GuaranteeKeyAtStartAndEnd, 0x007FFAF0, KeyContent content, KeyType type,
        re::Ptr32<NiAnimationKey>* keys, uint32_t* numKeys, float startTime, float endTime);

// 0x7FF6C0 Equal(key0, key1): the times
uint8_t __cdecl Equal(const NiAnimationKey* key0, const NiAnimationKey* key1) {
    return key1->time == key0->time ? 1 : 0;
}

// 0x7FF770 OkayToInsert(time, keys, count, index out, size): the index of the first key not before
// `time`; false when that key is at `time` exactly (or unordered with it)
uint8_t __cdecl OkayToInsert(float time, const NiAnimationKey* keys, uint32_t numKeys, uint32_t* index,
                             uint8_t size) {
    *index = 0;
    if (numKeys == 0) {
        return 1;
    }
    do {
        float keyTime = KeyAt(keys, *index, size)->time;
        if (!(keyTime < time)) {
            return keyTime > time ? 1 : 0;
        }
        (*index)++;
    } while (*index < numKeys);
    return 1;
}

// 0x7FF7D0 CopySequence(content, type, keys, count, start, end, new keys out, new count out): the
// keys from start to end copied into a new array (their times made relative to start); none (a
// null array) when there are none. An Euler key: its three sequences copied, into one new Euler
// key. (Counting stops at the first key after the end; a key before the start doesn't count.)
void __cdecl CopySequence(KeyContent content, KeyType type, NiAnimationKey* keys, uint32_t numKeys, float startTime,
                          float endTime, re::Ptr32<NiAnimationKey>* newKeys, uint32_t* newNumKeys) {
    if (type == EULERKEY) {
        auto* euler = static_cast<NiEulerRotKey*>(keys);
        re::Ptr32<NiAnimationKey> axisKeys[3];
        uint32_t axisCounts[3];
        KeyType axisTypes[3];
        for (uint32_t i = 0; i < 3; i++) {
            axisTypes[i] = euler->type[i];
            AnimationKey_CopySequence(FLOATKEY, euler->type[i], euler->keys[i], euler->numKeys[i], startTime, endTime,
                                      &axisKeys[i], &axisCounts[i]);
        }
        // a one-key NiNew[] array, the key constructed in place (its quaternion zero)
        uint32_t* block = static_cast<uint32_t*>(NiNewArray(0x4C));
        NiEulerRotKey* key = nullptr;
        if (block != nullptr) {
            key = reinterpret_cast<NiEulerRotKey*>(block + 1);
            *block = 1;
            Quaternion_Construct(&key->quat, 0.0f, 0.0f, 0.0f, 0.0f);
            key->numKeys[0] = 0;
            key->numKeys[1] = 0;
            key->numKeys[2] = 0;
            key->keys[0] = nullptr;
            key->keys[1] = nullptr;
            key->keys[2] = nullptr;
            key->type[0] = NOINTERP;
            key->type[1] = NOINTERP;
            key->type[2] = NOINTERP;
            key->lastIdx[0] = 0;
            key->lastIdx[1] = 0;
            key->lastIdx[2] = 0;
            key->size[0] = 0;
            key->size[1] = 0;
            key->size[2] = 0;
        }
        *newKeys = key;
        *newNumKeys = 1;
        // (on a failed allocation the original goes on with a null key too)
        EulerRotKey_Initialize(static_cast<NiEulerRotKey*>(*newKeys), static_cast<NiFloatKey*>(axisKeys[0]),
                               axisCounts[0], axisTypes[0], static_cast<NiFloatKey*>(axisKeys[1]), axisCounts[1],
                               axisTypes[1], static_cast<NiFloatKey*>(axisKeys[2]), axisCounts[2], axisTypes[2]);
        return;
    }

    uint32_t index = KeyIndex(content, type);
    *newNumKeys = 0;
    uint8_t size = ms_keySizes[index];
    for (uint32_t i = 0; i < numKeys; i++) {
        float time = KeyAt(keys, i, size)->time;
        if (!(time < startTime)) {
            if (time > endTime) {
                break;
            }
            (*newNumKeys)++;
        }
    }
    if (*newNumKeys == 0) {
        *newKeys = nullptr;
        return;
    }
    *newKeys = ms_array[index](*newNumKeys);
    CopyFunction copy = ms_copy[index];
    *newNumKeys = 0;
    for (uint32_t i = 0; i < numKeys; i++) {
        NiAnimationKey* key = KeyAt(keys, i, size);
        if (key->time < startTime) {
            continue;
        }
        if (endTime < key->time) {
            return;
        }
        NiAnimationKey* dest = KeyAt(*newKeys, *newNumKeys, size);
        copy(dest, key);
        dest->time = key->time - startTime;
        (*newNumKeys)++;
    }
}

// 0x7FFAF0 GuaranteeKeyAtStartAndEnd(content, type, keys, count, start, end): keys inserted at start
// and end (each kind's Insert keeps an existing one), except when the one key there is at start
// already. An Euler key: each sequence with keys done, its last index reset and its key size
// refreshed
void __cdecl GuaranteeKeyAtStartAndEnd(KeyContent content, KeyType type, re::Ptr32<NiAnimationKey>* keys, uint32_t* numKeys,
                                       float startTime, float endTime) {
    if (type == EULERKEY) {
        auto* euler = static_cast<NiEulerRotKey*>(*keys);
        for (uint32_t i = 0; i < 3; i++) {
            uint32_t count = euler->numKeys[i];
            re::Ptr32<NiAnimationKey> axisKeys = euler->keys[i];
            KeyType axisType = euler->type[i];
            if (count != 0) {
                AnimationKey_GuaranteeKeyAtStartAndEnd(FLOATKEY, axisType, &axisKeys, &count, startTime, endTime);
            }
            euler->keys[i] = static_cast<NiFloatKey*>(axisKeys);
            euler->numKeys[i] = count;
            euler->type[i] = axisType;
            euler->lastIdx[i] = 0;
            euler->size[i] = ms_keySizes[axisType];
        }
        return;
    }
    InsertFunction insert = ms_insert[KeyIndex(content, type)];
    if (*numKeys == 1 && startTime == (*keys)->time) {
        return;
    }
    insert(startTime, keys, numKeys);
    insert(endTime, keys, numKeys);
}

// 0x7FFC10 SaveBinary(stream, key): the time
void __cdecl SaveBinary(void* stream, const NiAnimationKey* key) {
    ni::scene::SaveBytes(stream, &key->time, 4);
}

// 0x7FFCD0 (one axis of a Bezier curve split where a key is inserted): the previous key's value
// and time and its outgoing tangent, the next key's value and time and its incoming tangent, the
// new key's time and value; the new key's tangents out, the two neighbours' scaled to their
// shorter spans. The slope of the Hermite segment at the split, s = (time - t0) / (t1 - t0)
void __cdecl SplitBezierTangents(float value0, float time0, float* outTangent0, float value1, float time1,
                                 float* inTangent1, float time, float value, float* inTangent, float* outTangent) {
    (void)value;
    float delta = value1 - value0;
    float span = time1 - time0;
    float before = time - time0;
    float after = time1 - time;
    float s = before / span;
    float p = (*outTangent0 - delta * 2.0f) + *inTangent1;    // the cubic coefficient
    float q = (delta * 3.0f - *outTangent0 * 2.0f) - *inTangent1;   // the quadratic one
    float slope = ((2.0f * q + (p * 3.0f) * s) * s + *outTangent0) / span;
    *inTangent = slope;
    *outTangent = slope * after;
    *inTangent = before * *inTangent;
    *outTangent0 = s * *outTangent0;
    *inTangent1 = (after / span) * *inTangent1;
}

} // namespace
} // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIANIMKEY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIANIMKEY_REVERSED("NiAnimationKey::Equal", 0x007FF6C0, ni::anim::Equal);
NIANIMKEY_REVERSED("NiAnimationKey::OkayToInsert", 0x007FF770, ni::anim::OkayToInsert);
NIANIMKEY_REVERSED("NiAnimationKey::CopySequence", 0x007FF7D0, ni::anim::CopySequence);
NIANIMKEY_REVERSED("NiAnimationKey::GuaranteeKeyAtStartAndEnd", 0x007FFAF0, ni::anim::GuaranteeKeyAtStartAndEnd);
NIANIMKEY_REVERSED("NiAnimationKey::SaveBinary", 0x007FFC10, ni::anim::SaveBinary);
NIANIMKEY_REVERSED("NiAnimationKey::SplitBezierTangents", 0x007FFCD0, ni::anim::SplitBezierTangents);
