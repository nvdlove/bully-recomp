// The blend interpolators' kinds (src/gamebryo/NiBlendInterpolator.h) of this part of the library:
// NiBlendTransformInterpolator, NiBlendQuaternionInterpolator, NiBlendPoint3Interpolator,
// NiBlendFloatInterpolator, NiBlendColorInterpolator and NiBlendBoolInterpolator. Each one's
// Update: one interpolator's value passed through (StoreSingleValue), or the weighted blend of all
// of them after the weights are normalized (BlendValues); construction, cloning, streaming,
// comparison, viewer strings. (NiBlendInterpolator's own slots are the next chunk's.)
//
// Main-thread float maths at the game's 24-bit x87 precision: every operation rounded to float,
// in the game's order. An interpolator's Update writes a value the blends read whether or not it
// wrote it; the locals are left as the game leaves them where it matters (said so below).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiBlendInterpolator.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::GetViewerStringFloat;
using scene::Point3_GetViewerString;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBlendTransformInterpolator_RTTI, 0x020E5478);
GAME_VAR(RTTI, NiBlendQuaternionInterpolator_RTTI, 0x020E54AC);
GAME_VAR(RTTI, NiBlendPoint3Interpolator_RTTI, 0x020E54E0);
GAME_VAR(RTTI, NiBlendFloatInterpolator_RTTI, 0x020E5514);
GAME_VAR(RTTI, NiBlendColorInterpolator_RTTI, 0x020E5548);
GAME_VAR(RTTI, NiBlendBoolInterpolator_RTTI, 0x020E557C);

constexpr uintptr_t NiBlendTransformInterpolator_vtable = 0x00967940;
constexpr uintptr_t NiBlendQuaternionInterpolator_vtable = 0x009679E8;
constexpr uintptr_t NiBlendPoint3Interpolator_vtable = 0x00967A90;
constexpr uintptr_t NiBlendFloatInterpolator_vtable = 0x00967B38;
constexpr uintptr_t NiBlendColorInterpolator_vtable = 0x00967BE0;
constexpr uintptr_t NiBlendBoolInterpolator_vtable = 0x00967C88;

GAME_VAR(NiPoint3, kPoint3Zero, 0x020E2D44);       // NiPoint3::ZERO
GAME_VAR(NiPoint3, kInvalidPoint, 0x00BC8C48);     // NiPoint3Interpolator's invalid value
GAME_VAR(NiColorA, kInvalidColor, 0x00BC8C54);     // NiColorInterpolator's
GAME_VAR(uint8_t, kInvalidBool, 0x00968240);

GAME_FN(char*, __cdecl, GetViewerStringByte, 0x00785600, const char* prefix, uint8_t value);
GAME_FN(void, __thiscall, NiPoint3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, NiPoint3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
// (NiColorA's four floats: the quaternion is read with it too)
GAME_FN(void, __thiscall, Float4_LoadBinary, 0x007625D0, void* self, void* stream);
GAME_FN(void, __thiscall, ColorA_SaveBinary, 0x00762670, const NiColorA* self, void* stream);
GAME_FN(uint8_t, __thiscall, ColorA_NotEqual, 0x00769390, const NiColorA* self, const NiColorA* other);
GAME_FN(char*, __thiscall, ColorA_GetViewerString, 0x00762770, const NiColorA* self, const char* prefix);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Construct, 0x00775B90, NiQuaternion* self, float w, float x, float y,
        float z);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Negate, 0x00775C10, const NiQuaternion* self, NiQuaternion* result);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Times, 0x00776410, const NiQuaternion* self, NiQuaternion* result,
        float scale);
GAME_FN(void, __thiscall, Quaternion_Normalize, 0x00776290, NiQuaternion* self);
GAME_FN(void, __thiscall, NiQuaternion_SaveBinary, 0x00776310, const NiQuaternion* self, void* stream);
GAME_FN(uint8_t, __thiscall, Quaternion_NotEqual, 0x006B1C10, const NiQuaternion* self, const NiQuaternion* other);
GAME_FN(char*, __thiscall, Quaternion_GetViewerString, 0x007763B0, const NiQuaternion* self, const char* prefix);
GAME_FN(void, __thiscall, QuatTransform_LoadBinary, 0x00825B50, NiQuatTransform* self, void* stream);
// NiBlendInterpolator's (the next chunk's)
GAME_FN(void, __thiscall, BlendInterpolator_Construct, 0x00827320, void* self, uint8_t managerControlled,
        float weightThreshold, uint8_t arraySize);
GAME_FN(void, __thiscall, BlendInterpolator_DefaultConstruct, 0x00826400, void* self);
GAME_FN(void, __thiscall, BlendInterpolator_Destruct, 0x00826A30, void* self);
GAME_FN(void, __thiscall, BlendInterpolator_CopyMembers, 0x00827510, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, BlendInterpolator_LoadBinary, 0x00827670, void* self, void* stream);
GAME_FN(void, __thiscall, BlendInterpolator_SaveBinary, 0x00827130, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, BlendInterpolator_IsEqual, 0x00826960, void* self, void* other);
GAME_FN(void, __thiscall, BlendInterpolator_GetViewerStrings, 0x00827B90, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, BlendInterpolator_ComputeNormalizedWeights, 0x00826C10, void* self);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, BlendTransform_StoreSingleValue, 0x0081F150, NiBlendInterpolator* self, float time,
        NiObjectNET* target, NiQuatTransform* value);
GAME_FN(uint8_t, __thiscall, BlendTransform_BlendValues, 0x0081F1D0, NiBlendInterpolator* self, float time,
        NiObjectNET* target, NiQuatTransform* value);
GAME_FN(uint8_t, __thiscall, BlendQuaternion_StoreSingleValue, 0x0081F850, NiBlendQuaternionInterpolator* self,
        float time, NiObjectNET* target, NiQuaternion* value);
GAME_FN(uint8_t, __thiscall, BlendQuaternion_BlendValues, 0x0081F940, NiBlendQuaternionInterpolator* self,
        float time, NiObjectNET* target, NiQuaternion* value);
GAME_FN(uint8_t, __thiscall, BlendPoint3_StoreSingleValue, 0x0081FDF0, NiBlendPoint3Interpolator* self, float time,
        NiObjectNET* target, NiPoint3* value);
GAME_FN(uint8_t, __thiscall, BlendPoint3_BlendValues, 0x0081FE90, NiBlendPoint3Interpolator* self, float time,
        NiObjectNET* target, NiPoint3* value);
GAME_FN(uint8_t, __thiscall, BlendFloat_StoreSingleValue, 0x008202F0, NiBlendFloatInterpolator* self, float time,
        NiObjectNET* target, float* value);
GAME_FN(uint8_t, __thiscall, BlendFloat_BlendValues, 0x00820360, NiBlendFloatInterpolator* self, float time,
        NiObjectNET* target, float* value);
GAME_FN(uint8_t, __thiscall, BlendColor_StoreSingleValue, 0x00820790, NiBlendColorInterpolator* self, float time,
        NiObjectNET* target, NiColorA* value);
GAME_FN(uint8_t, __thiscall, BlendColor_BlendValues, 0x00820840, NiBlendColorInterpolator* self, float time,
        NiObjectNET* target, NiColorA* value);
GAME_FN(uint8_t, __thiscall, BlendBool_StoreSingleValue, 0x00820D10, NiBlendBoolInterpolator* self, float time,
        NiObjectNET* target, uint8_t* value);
GAME_FN(uint8_t, __thiscall, BlendBool_BlendValues, 0x00820D80, NiBlendBoolInterpolator* self, float time,
        NiObjectNET* target, uint8_t* value);

// the Update every kind has: the only interpolator's value passed through, or the blend of all
// once the weights are normalized; none (false) without interpolators
template <typename Self, typename T>
inline uint8_t BlendUpdate(Self* self, float time, NiObjectNET* target, T* value,
                           uint8_t(__thiscall* single)(Self*, float, NiObjectNET*, T*),
                           uint8_t(__thiscall* blend)(Self*, float, NiObjectNET*, T*)) {
    uint8_t result = 0;
    if (self->interpCount == 1) {
        result = single(self, time, target, value);
        self->lastTime = time;
        return result;
    }
    if (self->interpCount != 0) {
        BlendInterpolator_ComputeNormalizedWeights(self);
        result = blend(self, time, target, value);
    }
    self->lastTime = time;
    return result;
}

// the time a StoreSingleValue updates at (the only one's when manager controlled)
inline float SingleTime(const NiBlendInterpolator* self, float time) {
    return (self->flags & 1) != 0 ? self->singleTime : time;
}

// GetUpdateTimeForItem as the blends inline it: no time for a zero weight; the item's own time
// when manager controlled, and none when that's invalid
inline bool ItemTime(const NiBlendInterpolator* self, const InterpArrayItem& item, float& time) {
    if (item.normalizedWeight == 0.0f) {
        return false;
    }
    if ((self->flags & 1) != 0) {
        time = item.updateTime;
    }
    return time != kInvalidFloat;
}

inline void InvalidateTransform(NiQuatTransform* value) {
    value->translate.x = kInvalidFloat;
    value->rotate.x = kInvalidFloat;
    value->scale = kInvalidFloat;
}

inline bool TransformInvalid(const NiQuatTransform& value) {
    return value.scale == kInvalidFloat && value.rotate.x == kInvalidFloat && value.translate.x == kInvalidFloat;
}

// ---- NiBlendTransformInterpolator

// 0x81EFB0: NiBlendTransformInterpolator(manager controlled, weight threshold, array size)
NiBlendInterpolator* __fastcall Transform_Construct(NiBlendInterpolator* self, re::Edx, uint8_t managerControlled,
                                                    float weightThreshold, uint8_t arraySize) {
    BlendInterpolator_Construct(self, managerControlled, weightThreshold, arraySize);
    self->vtable = reinterpret_cast<void*>(NiBlendTransformInterpolator_vtable);
    return self;
}

// 0x81EFE0: NiBlendTransformInterpolator()
NiBlendInterpolator* __fastcall Transform_DefaultConstruct(NiBlendInterpolator* self, re::Edx) {
    BlendInterpolator_DefaultConstruct(self);
    self->vtable = reinterpret_cast<void*>(NiBlendTransformInterpolator_vtable);
    return self;
}

// 0x81F000: CreateObject
NiObject* __cdecl Transform_CreateObject() {
    void* memory = NiNew(0x30);
    if (memory == nullptr) {
        return nullptr;
    }
    BlendInterpolator_DefaultConstruct(memory);
    static_cast<NiBlendInterpolator*>(memory)->vtable = reinterpret_cast<void*>(NiBlendTransformInterpolator_vtable);
    return static_cast<NiObject*>(memory);
}

// 0x81F040: IsEqual (the base's)
uint8_t __fastcall Transform_IsEqual(NiBlendInterpolator* self, re::Edx, NiBlendInterpolator* other) {
    return BlendInterpolator_IsEqual(self, other) != 0 ? 1 : 0;
}

// 0x81F060
const RTTI* __fastcall Transform_GetRTTI(NiBlendInterpolator*, re::Edx) {
    return &NiBlendTransformInterpolator_RTTI;
}

// 0x81F070: CreateClone (a failed allocation still copied onto null)
NiBlendInterpolator* __fastcall Transform_CreateClone(NiBlendInterpolator* self, re::Edx, void* cloning) {
    auto* clone = static_cast<NiBlendInterpolator*>(NiNew(0x30));
    if (clone == nullptr) {
        BlendInterpolator_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    BlendInterpolator_DefaultConstruct(clone);
    clone->vtable = reinterpret_cast<void*>(NiBlendTransformInterpolator_vtable);
    BlendInterpolator_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x81F0D0: LoadBinary (before 10.1.0.110 a transform value follows, read and dropped)
void __fastcall Transform_LoadBinary(NiBlendInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_LoadBinary(self, stream);
    if (scene::StreamVersion(stream) < 0x0A01006E) {
        NiQuatTransform dropped;
        dropped.translate = kInvalidTranslate;
        dropped.rotate = kInvalidRotate;
        dropped.scale = kInvalidFloat;
        QuatTransform_LoadBinary(&dropped, stream);
    }
}

// 0x81F150: StoreSingleValue(time, target, transform&): the only interpolator's (slot 18); an
// invalid time or a failed update gives the invalid transform
uint8_t __fastcall Transform_StoreSingleValue(NiBlendInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                              NiQuatTransform* value) {
    time = SingleTime(self, time);
    if (time == kInvalidFloat) {
        InvalidateTransform(value);
        return 0;
    }
    if (Virtual<uint8_t>(self->singleInterpolator, 18, time, target, value)) {
        return 1;
    }
    InvalidateTransform(value);
    return 0;
}

// 0x81F1D0: BlendValues(time, target, transform&): the translations and scales averaged by weight
// (the weights of the ones without renormalized away), the rotations summed weighted (each turned
// to the first's hemisphere) and normalized
uint8_t __fastcall Transform_BlendValues(NiBlendInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                         NiQuatTransform* value) {
    float transWeight = 1.0f;
    float scaleWeight = 1.0f;
    NiPoint3 translate = kPoint3Zero;
    NiQuaternion rotate;
    Quaternion_Construct(&rotate, 0.0f, 0.0f, 0.0f, 0.0f);
    float scale = 0.0f;
    uint8_t transChanged = 0;
    uint8_t rotChanged = 0;
    uint8_t scaleChanged = 0;
    uint8_t firstRotation = 1;
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem& item = self->interpArray[i];
        if (item.interpolator == nullptr || !(item.normalizedWeight > 0.0f)) {
            continue;
        }
        float updateTime = time;
        if (!ItemTime(self, item, updateTime)) {
            transWeight -= item.normalizedWeight;
            scaleWeight -= item.normalizedWeight;
            continue;
        }
        NiQuatTransform t;
        t.translate = kInvalidTranslate;
        t.rotate = kInvalidRotate;
        t.scale = kInvalidFloat;
        if (!Virtual<uint8_t>(item.interpolator, 18, updateTime, target, &t)) {
            transWeight -= item.normalizedWeight;
            scaleWeight -= item.normalizedWeight;
            continue;
        }
        float w = item.normalizedWeight;
        if (t.translate.x != kInvalidFloat) {
            transChanged = 1;
            float px = t.translate.x * w;
            float py = t.translate.y * w;
            float pz = t.translate.z * w;
            translate.x = px + translate.x;
            translate.y = translate.y + py;
            translate.z = translate.z + pz;
        } else {
            transWeight -= item.normalizedWeight;
        }
        if (t.rotate.x != kInvalidFloat) {
            NiQuaternion r = t.rotate;
            if (!firstRotation) {
                float dot = ((r.x * rotate.x + r.w * rotate.w) + r.y * rotate.y) + r.z * rotate.z;
                if (dot < 0.0f) {
                    NiQuaternion negated;
                    r = *Quaternion_Negate(&r, &negated);
                }
            } else {
                firstRotation = 0;
            }
            NiQuaternion weighted;
            r = *Quaternion_Times(&r, &weighted, item.normalizedWeight);
            rotate.w = r.w + rotate.w;
            rotChanged = 1;
            rotate.x = rotate.x + r.x;
            rotate.y = rotate.y + r.y;
            rotate.z = rotate.z + r.z;
        }
        if (t.scale != kInvalidFloat) {
            scale = t.scale * item.normalizedWeight + scale;
            scaleChanged = 1;
        } else {
            scaleWeight -= item.normalizedWeight;
        }
    }
    InvalidateTransform(value);
    if (transChanged) {
        float inv = 1.0f / transWeight;
        translate.x = inv * translate.x;
        value->translate.x = translate.x;
        translate.y = translate.y * inv;
        value->translate.y = translate.y;
        translate.z = inv * translate.z;
        value->translate.z = translate.z;
    }
    if (rotChanged) {
        Quaternion_Normalize(&rotate);
        value->rotate = rotate;
    }
    if (scaleChanged) {
        value->scale = scale / scaleWeight;
    }
    return TransformInvalid(*value) ? 0 : 1;
}

// 0x81F620: Update(time, target, transform&) (slot 18)
uint8_t __fastcall Transform_Update(NiBlendInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                    NiQuatTransform* value) {
    return BlendUpdate(self, time, target, value, BlendTransform_StoreSingleValue, BlendTransform_BlendValues);
}

// 0x81F680: GetViewerStrings
void __fastcall Transform_GetViewerStrings(NiBlendInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BlendInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBlendTransformInterpolator_RTTI.name));
}

// ---- InterpolatorIsCorrectType (slot 39) of each: the interpolator gives the kind's value

// 0x81F6D0: a quaternion (slot 26)
uint8_t __fastcall Quaternion_InterpolatorIsCorrectType(void*, re::Edx, NiInterpolator* interpolator) {
    return Virtual<uint8_t>(interpolator, 26);
}

// 0x81FCF0: a point (slot 27)
uint8_t __fastcall Point3_InterpolatorIsCorrectType(void*, re::Edx, NiInterpolator* interpolator) {
    return Virtual<uint8_t>(interpolator, 27);
}

// 0x820220: a float (slot 25)
uint8_t __fastcall Float_InterpolatorIsCorrectType(void*, re::Edx, NiInterpolator* interpolator) {
    return Virtual<uint8_t>(interpolator, 25);
}

// 0x820660: a colour (slot 28)
uint8_t __fastcall Color_InterpolatorIsCorrectType(void*, re::Edx, NiInterpolator* interpolator) {
    return Virtual<uint8_t>(interpolator, 28);
}

// 0x820C10: a bool (slot 24)
uint8_t __fastcall Bool_InterpolatorIsCorrectType(void*, re::Edx, NiInterpolator* interpolator) {
    return Virtual<uint8_t>(interpolator, 24);
}

// 0x821080: a transform (slot 29; NiBlendAccumTransformInterpolator's too)
uint8_t __fastcall Transform_InterpolatorIsCorrectType(void*, re::Edx, NiInterpolator* interpolator) {
    return Virtual<uint8_t>(interpolator, 29);
}

// ---- NiBlendQuaternionInterpolator

inline void QuaternionInitMembers(NiBlendQuaternionInterpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiBlendQuaternionInterpolator_vtable);
    self->value = kInvalidQuaternion;
}

// 0x81F6E0: CreateObject
NiObject* __cdecl Quaternion_CreateObject() {
    auto* self = static_cast<NiBlendQuaternionInterpolator*>(NiNew(0x40));
    if (self == nullptr) {
        return nullptr;
    }
    BlendInterpolator_DefaultConstruct(self);
    QuaternionInitMembers(self);
    return self;
}

// 0x81F730: LoadBinary
void __fastcall Quaternion_LoadBinary(NiBlendQuaternionInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_LoadBinary(self, stream);
    Float4_LoadBinary(&self->value, stream);
}

// 0x81F750: SaveBinary
void __fastcall Quaternion_SaveBinary(NiBlendQuaternionInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_SaveBinary(self, stream);
    NiQuaternion_SaveBinary(&self->value, stream);
}

// 0x81F770
const RTTI* __fastcall Quaternion_GetRTTI(NiBlendQuaternionInterpolator*, re::Edx) {
    return &NiBlendQuaternionInterpolator_RTTI;
}

// 0x81F780: the deleting destructor (NiBlendPoint3Interpolator's too: both 0x40 bytes)
NiBlendInterpolator* __fastcall Blend40_DeletingDestructor(NiBlendInterpolator* self, re::Edx, uint32_t flags) {
    BlendInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x40);
    }
    return self;
}

// 0x81F7A0: CreateClone (its value copied after; a failed allocation still copied onto null)
NiBlendQuaternionInterpolator* __fastcall Quaternion_CreateClone(NiBlendQuaternionInterpolator* self, re::Edx,
                                                                 void* cloning) {
    auto* clone = static_cast<NiBlendQuaternionInterpolator*>(NiNew(0x40));
    if (clone != nullptr) {
        BlendInterpolator_DefaultConstruct(clone);
        QuaternionInitMembers(clone);
    } else {
        clone = nullptr;
    }
    BlendInterpolator_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    return clone;
}

// 0x81F820: IsEqual
uint8_t __fastcall Quaternion_IsEqual(NiBlendQuaternionInterpolator* self, re::Edx,
                                      NiBlendQuaternionInterpolator* other) {
    if (!BlendInterpolator_IsEqual(self, other)) {
        return 0;
    }
    return Quaternion_NotEqual(&self->value, &other->value) == 0 ? 1 : 0;
}

// 0x81F850: StoreSingleValue(time, target, quaternion&) (into the value; the invalid one when the
// time is invalid or the update fails)
uint8_t __fastcall Quaternion_StoreSingleValue(NiBlendQuaternionInterpolator* self, re::Edx, float time,
                                               NiObjectNET* target, NiQuaternion* value) {
    time = SingleTime(self, time);
    if (time == kInvalidFloat) {
        *value = kInvalidQuaternion;
        self->value = *value;
        return 0;
    }
    if (!Virtual<uint8_t>(self->singleInterpolator, 21, time, target, &self->value)) {
        *value = kInvalidQuaternion;
        self->value = *value;
        return 0;
    }
    *value = self->value;
    return 1;
}

// 0x81F940: BlendValues(time, target, quaternion&): each rotation, turned to the running result's
// hemisphere, blended in by weight (the result scaled by the weight so far, the new one by its
// own, summed and normalized)
uint8_t __fastcall Quaternion_BlendValues(NiBlendQuaternionInterpolator* self, re::Edx, float time,
                                          NiObjectNET* target, NiQuaternion* value) {
    self->value = {0.0f, 0.0f, 0.0f, 0.0f};
    float totalWeight = 0.0f;
    uint8_t modified = 0;
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem& item = self->interpArray[i];
        if (item.interpolator == nullptr || !(item.normalizedWeight > 0.0f)) {
            continue;
        }
        float updateTime = time;
        if (!ItemTime(self, item, updateTime)) {
            continue;
        }
        NiQuaternion q;   // (left as it is: the update writes it)
        if (!Virtual<uint8_t>(item.interpolator, 21, updateTime, target, &q)) {
            continue;
        }
        if (totalWeight > 0.0f) {
            float dot = ((self->value.x * q.x + self->value.w * q.w) + self->value.y * q.y) + self->value.z * q.z;
            if (dot < 0.0f) {
                NiQuaternion negated;
                q = *Quaternion_Negate(&q, &negated);
            }
        }
        NiQuaternion weighted;
        q = *Quaternion_Times(&q, &weighted, self->interpArray[i].normalizedWeight);
        NiQuaternion scaled;
        self->value = *Quaternion_Times(&self->value, &scaled, totalWeight);
        float vx = self->value.x;
        float vy = self->value.y;
        float vz = self->value.z;
        self->value.w = q.w + self->value.w;
        self->value.x = q.x + vx;
        self->value.y = q.y + vy;
        self->value.z = q.z + vz;
        Quaternion_Normalize(&self->value);
        modified = 1;
        totalWeight = self->interpArray[i].normalizedWeight + totalWeight;
    }
    if (modified) {
        *value = self->value;
        return 1;
    }
    self->value = kInvalidQuaternion;
    *value = self->value;
    return 0;
}

// 0x81FBB0: Update(time, target, quaternion&) (slot 21)
uint8_t __fastcall Quaternion_Update(NiBlendQuaternionInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                     NiQuaternion* value) {
    return BlendUpdate(self, time, target, value, BlendQuaternion_StoreSingleValue, BlendQuaternion_BlendValues);
}

// 0x81FC10: GetViewerStrings
void __fastcall Quaternion_GetViewerStrings(NiBlendQuaternionInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BlendInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBlendQuaternionInterpolator_RTTI.name));
    AddViewerString(strings, Quaternion_GetViewerString(&self->value, "m_kQuaternionValue"));
}

// ---- NiBlendPoint3Interpolator

inline void Point3InitMembers(NiBlendPoint3Interpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiBlendPoint3Interpolator_vtable);
    self->value = kInvalidPoint;
    self->validValue = 0;
}

// 0x81FCA0: NiBlendPoint3Interpolator(manager controlled, weight threshold, array size)
NiBlendPoint3Interpolator* __fastcall Point3_Construct(NiBlendPoint3Interpolator* self, re::Edx,
                                                       uint8_t managerControlled, float weightThreshold,
                                                       uint8_t arraySize) {
    BlendInterpolator_Construct(self, managerControlled, weightThreshold, arraySize);
    Point3InitMembers(self);
    return self;
}

// 0x81FD00: CreateObject
NiObject* __cdecl Point3_CreateObject() {
    auto* self = static_cast<NiBlendPoint3Interpolator*>(NiNew(0x40));
    if (self == nullptr) {
        return nullptr;
    }
    BlendInterpolator_DefaultConstruct(self);
    Point3InitMembers(self);
    return self;
}

// 0x81FD50: SaveBinary
void __fastcall Point3_SaveBinary(NiBlendPoint3Interpolator* self, re::Edx, void* stream) {
    BlendInterpolator_SaveBinary(self, stream);
    NiPoint3_SaveBinary(&self->value, stream);
}

// 0x81FD70
const RTTI* __fastcall Point3_GetRTTI(NiBlendPoint3Interpolator*, re::Edx) {
    return &NiBlendPoint3Interpolator_RTTI;
}

// 0x81FD80: CreateClone (a failed allocation still copied onto null)
NiBlendPoint3Interpolator* __fastcall Point3_CreateClone(NiBlendPoint3Interpolator* self, re::Edx, void* cloning) {
    auto* clone = static_cast<NiBlendPoint3Interpolator*>(NiNew(0x40));
    if (clone != nullptr) {
        BlendInterpolator_DefaultConstruct(clone);
        Point3InitMembers(clone);
    } else {
        clone = nullptr;
    }
    BlendInterpolator_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    clone->validValue = self->validValue;
    return clone;
}

// 0x81FDF0: StoreSingleValue(time, target, point&) (an invalid time: nothing written)
uint8_t __fastcall Point3_StoreSingleValue(NiBlendPoint3Interpolator* self, re::Edx, float time, NiObjectNET* target,
                                           NiPoint3* value) {
    time = SingleTime(self, time);
    if (time == kInvalidFloat) {
        return 0;
    }
    if (Virtual<uint8_t>(self->singleInterpolator, 20, time, target, &self->value)) {
        *value = self->value;
        return 1;
    }
    self->value = kInvalidPoint;
    *value = self->value;
    return 0;
}

// 0x81FE90: BlendValues(time, target, point&): the points averaged by weight (the weights of the
// ones without renormalized away)
uint8_t __fastcall Point3_BlendValues(NiBlendPoint3Interpolator* self, re::Edx, float time, NiObjectNET* target,
                                      NiPoint3* value) {
    float totalWeight = 1.0f;
    self->validValue = 0;
    self->value = kPoint3Zero;
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem& item = self->interpArray[i];
        if (item.interpolator == nullptr || !(item.normalizedWeight > 0.0f)) {
            continue;
        }
        float updateTime = time;
        if (!ItemTime(self, item, updateTime)) {
            totalWeight -= item.normalizedWeight;
            continue;
        }
        NiPoint3 p;   // (left as it is: the update writes it)
        if (!Virtual<uint8_t>(item.interpolator, 20, updateTime, target, &p)) {
            totalWeight -= self->interpArray[i].normalizedWeight;
            continue;
        }
        float w = self->interpArray[i].normalizedWeight;
        float px = p.x * w;
        float py = p.y * w;
        float pz = w * p.z;
        self->value.x = self->value.x + px;
        self->value.y = py + self->value.y;
        self->value.z = self->value.z + pz;
        self->validValue = 1;
    }
    if (!self->validValue) {
        self->value = kInvalidPoint;
        *value = self->value;
        return 0;
    }
    float inv = 1.0f / totalWeight;
    self->value.x = self->value.x * inv;
    self->value.y = self->value.y * inv;
    self->value.z = inv * self->value.z;
    *value = self->value;
    return 1;
}

// 0x820060: Update(time, target, point&) (slot 20)
uint8_t __fastcall Point3_Update(NiBlendPoint3Interpolator* self, re::Edx, float time, NiObjectNET* target,
                                 NiPoint3* value) {
    return BlendUpdate(self, time, target, value, BlendPoint3_StoreSingleValue, BlendPoint3_BlendValues);
}

// 0x8200C0: LoadBinary (valid once it isn't the invalid point)
void __fastcall Point3_LoadBinary(NiBlendPoint3Interpolator* self, re::Edx, void* stream) {
    BlendInterpolator_LoadBinary(self, stream);
    NiPoint3_LoadBinary(&self->value, stream);
    if (kInvalidPoint.x != self->value.x || kInvalidPoint.y != self->value.y || kInvalidPoint.z != self->value.z) {
        self->validValue = 1;
    }
}

// 0x820120: IsEqual
uint8_t __fastcall Point3_IsEqual(NiBlendPoint3Interpolator* self, re::Edx, NiBlendPoint3Interpolator* other) {
    if (!BlendInterpolator_IsEqual(self, other)) {
        return 0;
    }
    return Point3_NotEqual(&self->value, &other->value) == 0 ? 1 : 0;
}

// 0x820150: GetViewerStrings
void __fastcall Point3_GetViewerStrings(NiBlendPoint3Interpolator* self, re::Edx, NiTArray<char*>* strings) {
    BlendInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBlendPoint3Interpolator_RTTI.name));
    AddViewerString(strings, Point3_GetViewerString(&self->value, "m_kPoint3Value"));
}

// ---- NiBlendFloatInterpolator

inline void FloatInitMembers(NiBlendFloatInterpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiBlendFloatInterpolator_vtable);
    self->value = kInvalidFloat;
}

// 0x8201E0: NiBlendFloatInterpolator(manager controlled, weight threshold, array size)
NiBlendFloatInterpolator* __fastcall Float_Construct(NiBlendFloatInterpolator* self, re::Edx, uint8_t managerControlled,
                                                     float weightThreshold, uint8_t arraySize) {
    BlendInterpolator_Construct(self, managerControlled, weightThreshold, arraySize);
    FloatInitMembers(self);
    return self;
}

// 0x820230: CreateObject
NiObject* __cdecl Float_CreateObject() {
    auto* self = static_cast<NiBlendFloatInterpolator*>(NiNew(0x34));
    if (self == nullptr) {
        return nullptr;
    }
    BlendInterpolator_DefaultConstruct(self);
    FloatInitMembers(self);
    return self;
}

// 0x820260: IsEqual
uint8_t __fastcall Float_IsEqual(NiBlendFloatInterpolator* self, re::Edx, NiBlendFloatInterpolator* other) {
    if (!BlendInterpolator_IsEqual(self, other)) {
        return 0;
    }
    return other->value == self->value ? 1 : 0;
}

// 0x820290
const RTTI* __fastcall Float_GetRTTI(NiBlendFloatInterpolator*, re::Edx) {
    return &NiBlendFloatInterpolator_RTTI;
}

// 0x8202A0: CreateClone (a failed allocation still copied onto null)
NiBlendFloatInterpolator* __fastcall Float_CreateClone(NiBlendFloatInterpolator* self, re::Edx, void* cloning) {
    auto* clone = static_cast<NiBlendFloatInterpolator*>(NiNew(0x34));
    if (clone != nullptr) {
        BlendInterpolator_DefaultConstruct(clone);
        FloatInitMembers(clone);
    } else {
        clone = nullptr;
    }
    BlendInterpolator_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    return clone;
}

// 0x8202F0: StoreSingleValue(time, target, float&) (an invalid time: nothing written)
uint8_t __fastcall Float_StoreSingleValue(NiBlendFloatInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                          float* value) {
    time = SingleTime(self, time);
    if (time == kInvalidFloat) {
        return 0;
    }
    if (Virtual<uint8_t>(self->singleInterpolator, 22, time, target, &self->value)) {
        *value = self->value;
        return 1;
    }
    self->value = kInvalidFloat;
    *value = kInvalidFloat;
    return 0;
}

// 0x820360: BlendValues(time, target, float&): the weighted average (divided whether or not any
// was blended in)
uint8_t __fastcall Float_BlendValues(NiBlendFloatInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                     float* value) {
    self->value = 0.0f;
    float totalWeight = 1.0f;
    uint8_t modified = 0;
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem& item = self->interpArray[i];
        if (item.interpolator == nullptr || !(item.normalizedWeight > 0.0f)) {
            continue;
        }
        float updateTime = time;
        if (!ItemTime(self, item, updateTime)) {
            totalWeight -= item.normalizedWeight;
            continue;
        }
        float f;   // (left as it is: the update writes it)
        if (!Virtual<uint8_t>(item.interpolator, 22, updateTime, target, &f)) {
            totalWeight -= self->interpArray[i].normalizedWeight;
            continue;
        }
        modified = 1;
        self->value = self->interpArray[i].normalizedWeight * f + self->value;
    }
    float result = self->value / totalWeight;
    self->value = result;
    if (!modified) {
        *value = kInvalidFloat;
        self->value = kInvalidFloat;
        return 0;
    }
    *value = result;
    return 1;
}

// 0x8204A0: Update(time, target, float&) (slot 22)
uint8_t __fastcall Float_Update(NiBlendFloatInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                float* value) {
    return BlendUpdate(self, time, target, value, BlendFloat_StoreSingleValue, BlendFloat_BlendValues);
}

// 0x820500: LoadBinary
void __fastcall Float_LoadBinary(NiBlendFloatInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_LoadBinary(self, stream);
    scene::LoadBytes(stream, &self->value, 4);
}

// 0x820540: SaveBinary
void __fastcall Float_SaveBinary(NiBlendFloatInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_SaveBinary(self, stream);
    scene::SaveBytes(stream, &self->value, 4);
}

// 0x820580: GetViewerStrings
void __fastcall Float_GetViewerStrings(NiBlendFloatInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BlendInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBlendFloatInterpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringFloat("m_fFloatValue", self->value));
}

// ---- NiBlendColorInterpolator

inline void ColorInitMembers(NiBlendColorInterpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiBlendColorInterpolator_vtable);
    self->value = kInvalidColor;
    self->validValue = 0;
}

// 0x820610: NiBlendColorInterpolator(manager controlled, weight threshold, array size)
NiBlendColorInterpolator* __fastcall Color_Construct(NiBlendColorInterpolator* self, re::Edx, uint8_t managerControlled,
                                                     float weightThreshold, uint8_t arraySize) {
    BlendInterpolator_Construct(self, managerControlled, weightThreshold, arraySize);
    ColorInitMembers(self);
    return self;
}

// 0x820670: CreateObject
NiObject* __cdecl Color_CreateObject() {
    auto* self = static_cast<NiBlendColorInterpolator*>(NiNew(0x44));
    if (self == nullptr) {
        return nullptr;
    }
    BlendInterpolator_DefaultConstruct(self);
    ColorInitMembers(self);
    return self;
}

// 0x8206C0: SaveBinary
void __fastcall Color_SaveBinary(NiBlendColorInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_SaveBinary(self, stream);
    ColorA_SaveBinary(&self->value, stream);
}

// 0x8206E0
const RTTI* __fastcall Color_GetRTTI(NiBlendColorInterpolator*, re::Edx) {
    return &NiBlendColorInterpolator_RTTI;
}

// 0x8206F0: the deleting destructor
NiBlendColorInterpolator* __fastcall Color_DeletingDestructor(NiBlendColorInterpolator* self, re::Edx,
                                                              uint32_t flags) {
    BlendInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x44);
    }
    return self;
}

// 0x820710: CreateClone (a failed allocation still copied onto null)
NiBlendColorInterpolator* __fastcall Color_CreateClone(NiBlendColorInterpolator* self, re::Edx, void* cloning) {
    auto* clone = static_cast<NiBlendColorInterpolator*>(NiNew(0x44));
    if (clone != nullptr) {
        BlendInterpolator_DefaultConstruct(clone);
        ColorInitMembers(clone);
    } else {
        clone = nullptr;
    }
    BlendInterpolator_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    clone->validValue = self->validValue;
    return clone;
}

// 0x820790: StoreSingleValue(time, target, colour&) (an invalid time: nothing written)
uint8_t __fastcall Color_StoreSingleValue(NiBlendColorInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                          NiColorA* value) {
    time = SingleTime(self, time);
    if (time == kInvalidFloat) {
        return 0;
    }
    if (Virtual<uint8_t>(self->singleInterpolator, 19, time, target, &self->value)) {
        *value = self->value;
        return 1;
    }
    *value = kInvalidColor;
    self->value = *value;
    return 0;
}

// 0x820840: BlendValues(time, target, colour&): the weighted average (each component divided by
// the total weight)
uint8_t __fastcall Color_BlendValues(NiBlendColorInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                     NiColorA* value) {
    NiColorA zero = {0.0f, 0.0f, 0.0f, 0.0f};
    float totalWeight = 1.0f;
    self->value = zero;
    self->validValue = 0;
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem& item = self->interpArray[i];
        if (item.interpolator == nullptr || !(item.normalizedWeight > 0.0f)) {
            continue;
        }
        NiColorA c = {0.0f, 0.0f, 0.0f, 0.0f};
        float updateTime = time;
        if (!ItemTime(self, item, updateTime)) {
            totalWeight -= item.normalizedWeight;
            continue;
        }
        if (!Virtual<uint8_t>(item.interpolator, 19, updateTime, target, &c)) {
            totalWeight -= self->interpArray[i].normalizedWeight;
            continue;
        }
        float w = self->interpArray[i].normalizedWeight;
        float r = c.r * w;
        float g = c.g * w;
        float b = c.b * w;
        float a = w * c.a;
        self->value.r = self->value.r + r;
        self->value.g = self->value.g + g;
        self->value.b = self->value.b + b;
        self->value.a = self->value.a + a;
        self->validValue = 1;
    }
    if (!self->validValue) {
        self->value = kInvalidColor;
        *value = self->value;
        return 0;
    }
    self->value.r = self->value.r / totalWeight;
    self->value.g = self->value.g / totalWeight;
    self->value.b = self->value.b / totalWeight;
    self->value.a = self->value.a / totalWeight;
    *value = self->value;
    return 1;
}

// 0x820A70: Update(time, target, colour&) (slot 19)
uint8_t __fastcall Color_Update(NiBlendColorInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                NiColorA* value) {
    return BlendUpdate(self, time, target, value, BlendColor_StoreSingleValue, BlendColor_BlendValues);
}

// 0x820AD0: LoadBinary (valid once it isn't the invalid colour)
void __fastcall Color_LoadBinary(NiBlendColorInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_LoadBinary(self, stream);
    Float4_LoadBinary(&self->value, stream);
    if (ColorA_NotEqual(&self->value, &kInvalidColor)) {
        self->validValue = 1;
    }
}

// 0x820B10: IsEqual
uint8_t __fastcall Color_IsEqual(NiBlendColorInterpolator* self, re::Edx, NiBlendColorInterpolator* other) {
    if (!BlendInterpolator_IsEqual(self, other)) {
        return 0;
    }
    return ColorA_NotEqual(&self->value, &other->value) == 0 ? 1 : 0;
}

// 0x820B40: GetViewerStrings
void __fastcall Color_GetViewerStrings(NiBlendColorInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BlendInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBlendColorInterpolator_RTTI.name));
    AddViewerString(strings, ColorA_GetViewerString(&self->value, "m_kColorValue"));
}

// ---- NiBlendBoolInterpolator

inline void BoolInitMembers(NiBlendBoolInterpolator* self) {
    self->vtable = reinterpret_cast<void*>(NiBlendBoolInterpolator_vtable);
    self->value = kInvalidBool;
}

// 0x820BD0: NiBlendBoolInterpolator(manager controlled, weight threshold, array size)
NiBlendBoolInterpolator* __fastcall Bool_Construct(NiBlendBoolInterpolator* self, re::Edx, uint8_t managerControlled,
                                                   float weightThreshold, uint8_t arraySize) {
    BlendInterpolator_Construct(self, managerControlled, weightThreshold, arraySize);
    BoolInitMembers(self);
    return self;
}

// 0x820C20: CreateObject
NiObject* __cdecl Bool_CreateObject() {
    auto* self = static_cast<NiBlendBoolInterpolator*>(NiNew(0x34));
    if (self == nullptr) {
        return nullptr;
    }
    BlendInterpolator_DefaultConstruct(self);
    BoolInitMembers(self);
    return self;
}

// 0x820C60: IsEqual
uint8_t __fastcall Bool_IsEqual(NiBlendBoolInterpolator* self, re::Edx, NiBlendBoolInterpolator* other) {
    if (!BlendInterpolator_IsEqual(self, other)) {
        return 0;
    }
    return self->value == other->value ? 1 : 0;
}

// 0x820C90
const RTTI* __fastcall Bool_GetRTTI(NiBlendBoolInterpolator*, re::Edx) {
    return &NiBlendBoolInterpolator_RTTI;
}

// 0x820CA0: the deleting destructor (NiBlendFloatInterpolator's too: both 0x34 bytes)
NiBlendInterpolator* __fastcall Blend34_DeletingDestructor(NiBlendInterpolator* self, re::Edx, uint32_t flags) {
    BlendInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x34);
    }
    return self;
}

// 0x820CC0: CreateClone (a failed allocation still copied onto null)
NiBlendBoolInterpolator* __fastcall Bool_CreateClone(NiBlendBoolInterpolator* self, re::Edx, void* cloning) {
    auto* clone = static_cast<NiBlendBoolInterpolator*>(NiNew(0x34));
    if (clone != nullptr) {
        BlendInterpolator_DefaultConstruct(clone);
        BoolInitMembers(clone);
    } else {
        clone = nullptr;
    }
    BlendInterpolator_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    return clone;
}

// 0x820D10: StoreSingleValue(time, target, bool&) (an invalid time: nothing written)
uint8_t __fastcall Bool_StoreSingleValue(NiBlendBoolInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                         uint8_t* value) {
    time = SingleTime(self, time);
    if (time == kInvalidFloat) {
        return 0;
    }
    if (Virtual<uint8_t>(self->singleInterpolator, 23, time, target, value)) {
        self->value = *value;
        return 1;
    }
    *value = 0;
    self->value = kInvalidBool;
    return 0;
}

// 0x820D80: BlendValues(time, target, bool&): true when the weighted average is at least a half
// (a NaN one too)
uint8_t __fastcall Bool_BlendValues(NiBlendBoolInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                    uint8_t* value) {
    float sum = 0.0f;
    float totalWeight = 1.0f;
    uint8_t modified = 0;
    // (the update writes the bool over the target argument's slot: its low byte before)
    uint8_t b = uint8_t(reinterpret_cast<uintptr_t>(target));
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem& item = self->interpArray[i];
        if (item.interpolator == nullptr || !(item.normalizedWeight > 0.0f)) {
            continue;
        }
        float updateTime = time;
        if (!ItemTime(self, item, updateTime)) {
            totalWeight -= item.normalizedWeight;
            continue;
        }
        if (!Virtual<uint8_t>(item.interpolator, 23, updateTime, target, &b)) {
            totalWeight -= self->interpArray[i].normalizedWeight;
            continue;
        }
        modified = 1;
        sum = self->interpArray[i].normalizedWeight * float(int32_t(b)) + sum;
    }
    if (!modified) {
        *value = 0;
        self->value = kInvalidBool;
        return 0;
    }
    float average = sum / totalWeight;
    if (average < 0.5f) {
        self->value = 0;
        *value = 0;
        return 1;
    }
    self->value = 1;
    *value = 1;
    return 1;
}

// 0x820EF0: Update(time, target, bool&) (slot 23)
uint8_t __fastcall Bool_Update(NiBlendBoolInterpolator* self, re::Edx, float time, NiObjectNET* target,
                               uint8_t* value) {
    return BlendUpdate(self, time, target, value, BlendBool_StoreSingleValue, BlendBool_BlendValues);
}

// 0x820F50: LoadBinary
void __fastcall Bool_LoadBinary(NiBlendBoolInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_LoadBinary(self, stream);
    scene::LoadBytes(stream, &self->value, 1);
}

// 0x820F90: SaveBinary
void __fastcall Bool_SaveBinary(NiBlendBoolInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_SaveBinary(self, stream);
    scene::SaveBytes(stream, &self->value, 1);
}

// 0x820FD0: GetViewerStrings
void __fastcall Bool_GetViewerStrings(NiBlendBoolInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    BlendInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBlendBoolInterpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringByte("m_bBoolValue", self->value));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC32_BLEND_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (lone jmps to NiBlendInterpolator's SaveBinary, CopyMembers and RegisterStreamables)
RE_UNHOOKED("NiBlendTransformInterpolator::SaveBinary (thunk)", 0x0081F030);
RE_UNHOOKED("NiBlendTransformInterpolator::CopyMembers (thunk)", 0x0081F0C0);
RE_UNHOOKED("NiBlendInterpolator::RegisterStreamables (thunk)", 0x00820C50);

NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::NiBlendTransformInterpolator", 0x0081EFB0,
                     &ni::anim::Transform_Construct);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::NiBlendTransformInterpolator()", 0x0081EFE0,
                     &ni::anim::Transform_DefaultConstruct);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::CreateObject", 0x0081F000, &ni::anim::Transform_CreateObject);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::IsEqual", 0x0081F040, &ni::anim::Transform_IsEqual);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::GetRTTI", 0x0081F060, &ni::anim::Transform_GetRTTI);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::CreateClone", 0x0081F070, &ni::anim::Transform_CreateClone);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::LoadBinary", 0x0081F0D0, &ni::anim::Transform_LoadBinary);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::StoreSingleValue", 0x0081F150,
                     &ni::anim::Transform_StoreSingleValue);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::BlendValues", 0x0081F1D0, &ni::anim::Transform_BlendValues);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::Update", 0x0081F620, &ni::anim::Transform_Update);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::GetViewerStrings", 0x0081F680,
                     &ni::anim::Transform_GetViewerStrings);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::InterpolatorIsCorrectType", 0x0081F6D0,
                     &ni::anim::Quaternion_InterpolatorIsCorrectType);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::CreateObject", 0x0081F6E0,
                     &ni::anim::Quaternion_CreateObject);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::LoadBinary", 0x0081F730, &ni::anim::Quaternion_LoadBinary);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::SaveBinary", 0x0081F750, &ni::anim::Quaternion_SaveBinary);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::GetRTTI", 0x0081F770, &ni::anim::Quaternion_GetRTTI);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::DeletingDestructor", 0x0081F780,
                     &ni::anim::Blend40_DeletingDestructor);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::CreateClone", 0x0081F7A0, &ni::anim::Quaternion_CreateClone);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::IsEqual", 0x0081F820, &ni::anim::Quaternion_IsEqual);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::StoreSingleValue", 0x0081F850,
                     &ni::anim::Quaternion_StoreSingleValue);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::BlendValues", 0x0081F940, &ni::anim::Quaternion_BlendValues);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::Update", 0x0081FBB0, &ni::anim::Quaternion_Update);
NIC32_BLEND_REVERSED("NiBlendQuaternionInterpolator::GetViewerStrings", 0x0081FC10,
                     &ni::anim::Quaternion_GetViewerStrings);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::NiBlendPoint3Interpolator", 0x0081FCA0,
                     &ni::anim::Point3_Construct);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::InterpolatorIsCorrectType", 0x0081FCF0,
                     &ni::anim::Point3_InterpolatorIsCorrectType);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::CreateObject", 0x0081FD00, &ni::anim::Point3_CreateObject);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::SaveBinary", 0x0081FD50, &ni::anim::Point3_SaveBinary);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::GetRTTI", 0x0081FD70, &ni::anim::Point3_GetRTTI);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::CreateClone", 0x0081FD80, &ni::anim::Point3_CreateClone);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::StoreSingleValue", 0x0081FDF0,
                     &ni::anim::Point3_StoreSingleValue);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::BlendValues", 0x0081FE90, &ni::anim::Point3_BlendValues);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::Update", 0x00820060, &ni::anim::Point3_Update);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::LoadBinary", 0x008200C0, &ni::anim::Point3_LoadBinary);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::IsEqual", 0x00820120, &ni::anim::Point3_IsEqual);
NIC32_BLEND_REVERSED("NiBlendPoint3Interpolator::GetViewerStrings", 0x00820150,
                     &ni::anim::Point3_GetViewerStrings);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::NiBlendFloatInterpolator", 0x008201E0, &ni::anim::Float_Construct);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::InterpolatorIsCorrectType", 0x00820220,
                     &ni::anim::Float_InterpolatorIsCorrectType);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::CreateObject", 0x00820230, &ni::anim::Float_CreateObject);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::IsEqual", 0x00820260, &ni::anim::Float_IsEqual);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::GetRTTI", 0x00820290, &ni::anim::Float_GetRTTI);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::CreateClone", 0x008202A0, &ni::anim::Float_CreateClone);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::StoreSingleValue", 0x008202F0, &ni::anim::Float_StoreSingleValue);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::BlendValues", 0x00820360, &ni::anim::Float_BlendValues);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::Update", 0x008204A0, &ni::anim::Float_Update);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::LoadBinary", 0x00820500, &ni::anim::Float_LoadBinary);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::SaveBinary", 0x00820540, &ni::anim::Float_SaveBinary);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::GetViewerStrings", 0x00820580, &ni::anim::Float_GetViewerStrings);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::NiBlendColorInterpolator", 0x00820610, &ni::anim::Color_Construct);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::InterpolatorIsCorrectType", 0x00820660,
                     &ni::anim::Color_InterpolatorIsCorrectType);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::CreateObject", 0x00820670, &ni::anim::Color_CreateObject);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::SaveBinary", 0x008206C0, &ni::anim::Color_SaveBinary);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::GetRTTI", 0x008206E0, &ni::anim::Color_GetRTTI);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::DeletingDestructor", 0x008206F0,
                     &ni::anim::Color_DeletingDestructor);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::CreateClone", 0x00820710, &ni::anim::Color_CreateClone);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::StoreSingleValue", 0x00820790, &ni::anim::Color_StoreSingleValue);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::BlendValues", 0x00820840, &ni::anim::Color_BlendValues);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::Update", 0x00820A70, &ni::anim::Color_Update);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::LoadBinary", 0x00820AD0, &ni::anim::Color_LoadBinary);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::IsEqual", 0x00820B10, &ni::anim::Color_IsEqual);
NIC32_BLEND_REVERSED("NiBlendColorInterpolator::GetViewerStrings", 0x00820B40, &ni::anim::Color_GetViewerStrings);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::NiBlendBoolInterpolator", 0x00820BD0, &ni::anim::Bool_Construct);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::InterpolatorIsCorrectType", 0x00820C10,
                     &ni::anim::Bool_InterpolatorIsCorrectType);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::CreateObject", 0x00820C20, &ni::anim::Bool_CreateObject);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::IsEqual", 0x00820C60, &ni::anim::Bool_IsEqual);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::GetRTTI", 0x00820C90, &ni::anim::Bool_GetRTTI);
NIC32_BLEND_REVERSED("NiBlendFloatInterpolator::DeletingDestructor", 0x00820CA0,
                     &ni::anim::Blend34_DeletingDestructor);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::CreateClone", 0x00820CC0, &ni::anim::Bool_CreateClone);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::StoreSingleValue", 0x00820D10, &ni::anim::Bool_StoreSingleValue);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::BlendValues", 0x00820D80, &ni::anim::Bool_BlendValues);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::Update", 0x00820EF0, &ni::anim::Bool_Update);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::LoadBinary", 0x00820F50, &ni::anim::Bool_LoadBinary);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::SaveBinary", 0x00820F90, &ni::anim::Bool_SaveBinary);
NIC32_BLEND_REVERSED("NiBlendBoolInterpolator::GetViewerStrings", 0x00820FD0, &ni::anim::Bool_GetViewerStrings);
NIC32_BLEND_REVERSED("NiBlendTransformInterpolator::InterpolatorIsCorrectType", 0x00821080,
                     &ni::anim::Transform_InterpolatorIsCorrectType);
