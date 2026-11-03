// NiBlendAccumTransformInterpolator's slots of this part of the library (src/gamebryo/
// NiBlendInterpolator.h; its constructor, accumulation item and destructor slot are
// src/gamebryo/NiTransformController.cpp's): the transform accumulated over an animation's loops
// for its root. Each blended interpolator's item keeps the transform at the last update (the
// delta), the start of the current accumulation step and the rotation that turns the steps into
// the accumulated frame; Update moves every item on, then blends the steps into the accumulated
// value and hands out the value composed with the current deltas.
//
// Main-thread float maths at the game's 24-bit x87 precision, in the game's order; the
// NiQuatTransform arithmetic is through its functions (src/gamebryo/NiInterpolatorBase.cpp).

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
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBlendAccumTransformInterpolator_RTTI, 0x020E55B0);

constexpr uintptr_t NiBlendAccumTransformInterpolator_vtable = 0x00966798;
constexpr uintptr_t AccumArrayItem_Construct_address = 0x008103F0;

GAME_VAR(NiPoint3, kPoint3Zero, 0x020E2D44);             // NiPoint3::ZERO
GAME_VAR(NiQuaternion, kQuaternionIdentity, 0x00BC8A80);  // NiQuaternion::IDENTITY
GAME_VAR(NiMatrix3, kMatrixIdentity, 0x00BC75FC);
GAME_VAR(uint8_t, kInvalidIndex, 0x009683CC);              // NiBlendInterpolator::INVALID_INDEX

GAME_FN(void*, __cdecl, NiAllocArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __thiscall, Matrix3_LoadBinary, 0x007690A0, NiMatrix3* self, void* stream);
GAME_FN(void, __thiscall, Matrix3_SaveBinary, 0x00769100, const NiMatrix3* self, void* stream);
GAME_FN(NiPoint3*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Construct, 0x00775B90, NiQuaternion* self, float w, float x, float y,
        float z);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Negate, 0x00775C10, const NiQuaternion* self, NiQuaternion* result);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Multiply, 0x00775C40, const NiQuaternion* self, NiQuaternion* result,
        const NiQuaternion* other);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_UnitInverse, 0x00775CD0, NiQuaternion* result, const NiQuaternion* q);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Times, 0x00776410, const NiQuaternion* self, NiQuaternion* result,
        float scale);
GAME_FN(void, __thiscall, Quaternion_Normalize, 0x00776290, NiQuaternion* self);
GAME_FN(void, __thiscall, Quaternion_ToRotation, 0x00413C50, const NiQuaternion* self, NiMatrix3* m);
// NiQuatTransform's
GAME_FN(NiQuatTransform*, __thiscall, QuatTransform_Construct, 0x00805180, NiQuatTransform* self);   // (invalid)
GAME_FN(uint8_t, __thiscall, QuatTransform_IsTransformInvalid, 0x008051D0, const NiQuatTransform* self);
GAME_FN(uint8_t, __thiscall, QuatTransform_Equal, 0x00805210, const NiQuatTransform* self,
        const NiQuatTransform* other);
GAME_FN(void, __thiscall, QuatTransform_Invert, 0x00825610, const NiQuatTransform* self, NiQuatTransform* dest);
GAME_FN(void, __thiscall, QuatTransform_HierInvert, 0x008256E0, const NiQuatTransform* self, NiQuatTransform* dest);
GAME_FN(NiQuatTransform*, __thiscall, QuatTransform_Multiply, 0x00825820, const NiQuatTransform* self,
        NiQuatTransform* result, const NiQuatTransform* other);
GAME_FN(NiQuatTransform*, __thiscall, QuatTransform_Concatenate, 0x008259F0, const NiQuatTransform* self,
        NiQuatTransform* result, const NiQuatTransform* other);
GAME_FN(void, __thiscall, QuatTransform_LoadBinary, 0x00825B50, NiQuatTransform* self, void* stream);
GAME_FN(void, __thiscall, QuatTransform_SaveBinary, 0x00825C50, const NiQuatTransform* self, void* stream);
GAME_FN(void, __thiscall, QuatTransform_GetViewerStrings, 0x00825CA0, const NiQuatTransform* self,
        NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Item_ClearValues, 0x00810290, AccumArrayItem* self);
// NiBlendInterpolator's (the next chunk's)
GAME_FN(void, __thiscall, BlendInterpolator_DefaultConstruct, 0x00826400, void* self);
GAME_FN(void, __thiscall, BlendInterpolator_Destruct, 0x00826A30, void* self);
GAME_FN(uint8_t, __thiscall, BlendInterpolator_AddInterpInfo, 0x00826470, void* self, NiInterpolator* interpolator,
        float weight, int priority, float easeSpinner);
GAME_FN(re::Ptr32<NiInterpolator>*, __thiscall, BlendInterpolator_RemoveInterpInfo, 0x00826AB0, void* self,
        re::Ptr32<NiInterpolator>* result, uint8_t index);
GAME_FN(uint8_t, __thiscall, BlendInterpolator_ReallocateArrays, 0x00827400, void* self);
GAME_FN(void, __thiscall, BlendInterpolator_CopyMembers, 0x00827510, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, BlendInterpolator_LoadBinary, 0x00827670, void* self, void* stream);
GAME_FN(void, __thiscall, BlendInterpolator_SaveBinary, 0x00827130, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, BlendInterpolator_IsEqual, 0x00826960, void* self, void* other);
GAME_FN(void, __thiscall, BlendInterpolator_GetViewerStrings, 0x00827B90, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, BlendInterpolator_ComputeNormalizedWeights, 0x00826C10, void* self);
// (this file's, called where modes apply)
GAME_FN(NiBlendAccumTransformInterpolator*, __thiscall, BlendAccum_DefaultConstruct, 0x00821090, void* self);
GAME_FN(void, __thiscall, BlendAccum_AccumulateSingle, 0x00821410, NiBlendAccumTransformInterpolator* self,
        float time, NiObjectNET* target);
GAME_FN(uint8_t, __thiscall, BlendAccum_StoreSingleValue, 0x00821680, NiBlendAccumTransformInterpolator* self,
        float time, NiObjectNET* target, NiQuatTransform* value);
GAME_FN(uint8_t, __thiscall, BlendAccum_BlendValues, 0x00821830, NiBlendAccumTransformInterpolator* self, float time,
        NiObjectNET* target, NiQuatTransform* value);
GAME_FN(void, __thiscall, BlendAccum_AccumulateItem, 0x00821DC0, NiBlendAccumTransformInterpolator* self,
        uint8_t index, float time, NiObjectNET* target);
GAME_FN(void, __thiscall, BlendAccum_CopyMembers, 0x00822290, NiBlendAccumTransformInterpolator* self,
        NiBlendAccumTransformInterpolator* dest, void* cloning);

inline bool TransformInvalid(const NiQuatTransform& t) {
    return t.scale == kInvalidFloat && t.rotate.x == kInvalidFloat && t.translate.x == kInvalidFloat;
}

// AccumArrayItem::ClearValues as these inline it
inline void ClearItem(AccumArrayItem& item) {
    item.lastTime = kInvalidFloat;
    item.delta.translate.x = kInvalidFloat;
    item.delta.rotate.x = kInvalidFloat;
    item.delta.scale = kInvalidFloat;
    item.start.translate.x = kInvalidFloat;
    item.start.rotate.x = kInvalidFloat;
    item.start.scale = kInvalidFloat;
    item.rotation = kMatrixIdentity;
}

// an accumulation array (NiAllocArray'd at count * 0x68, -1 when that overflows; no count before
// it), each item constructed
inline AccumArrayItem* NewItems(uint8_t count) {
    uint64_t product = uint64_t(count) * 0x68;
    void* memory = NiAllocArray((product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product));
    if (memory == nullptr) {
        return nullptr;
    }
    VectorConstruct(memory, 0x68, count, reinterpret_cast<void*>(AccumArrayItem_Construct_address));
    return static_cast<AccumArrayItem*>(memory);
}

// a quaternion, or the identity when invalid
inline NiQuaternion RotationOr(const NiQuaternion& q) {
    if (q.x != kInvalidFloat) {
        return q;
    }
    return kQuaternionIdentity;
}

// the inverse of a rotation, or the identity when invalid
inline NiQuaternion InverseOr(const NiQuaternion& q) {
    NiQuaternion result = kQuaternionIdentity;
    if (q.x != kInvalidFloat) {
        NiQuaternion inverse;
        result = *Quaternion_UnitInverse(&inverse, &q);
    }
    return result;
}

// the rotation from a step's frame into the accumulated one (the value's rotation times the
// inverse of the reference) into the item's matrix
inline void SetItemRotation(AccumArrayItem& item, const NiQuaternion& value, const NiQuaternion& reference) {
    NiQuaternion a = RotationOr(value);
    NiQuaternion b = InverseOr(reference);
    NiQuaternion r;
    Quaternion_Multiply(&a, &r, &b);
    Quaternion_ToRotation(&r, &item.rotation);
}

// 0x821060: ~NiBlendAccumTransformInterpolator (the items freed, then the base's)
void __fastcall BlendAccum_Destruct(NiBlendAccumTransformInterpolator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiBlendAccumTransformInterpolator_vtable);
    NiFreeArray(self->accumArray, 0x68);
    BlendInterpolator_Destruct(self);
}

// 0x821090: NiBlendAccumTransformInterpolator() (no items)
NiBlendAccumTransformInterpolator* __fastcall BlendAccum_DefaultConstructImpl(NiBlendAccumTransformInterpolator* self,
                                                                             re::Edx) {
    BlendInterpolator_DefaultConstruct(self);
    self->vtable = reinterpret_cast<void*>(NiBlendAccumTransformInterpolator_vtable);
    self->value.translate = kInvalidTranslate;
    self->value.rotate = kInvalidRotate;
    self->value.scale = kInvalidFloat;
    self->accumArray = nullptr;
    self->reset = 0;
    return self;
}

// 0x8210F0: AddInterpInfo(interp, weight, priority, ease spinner) (slot 37): the new item cleared;
// the first makes the accumulation start again; the second ends the single one's run: its delta
// composed into the value and its rotation set up (its values cleared when it had none)
uint8_t __fastcall BlendAccum_AddInterpInfo(NiBlendAccumTransformInterpolator* self, re::Edx,
                                            NiInterpolator* interpolator, float weight, int priority,
                                            float easeSpinner) {
    uint8_t previous = self->singleIdx;
    uint8_t index = BlendInterpolator_AddInterpInfo(self, interpolator, weight, priority, easeSpinner);
    if (index == kInvalidIndex) {
        return index;
    }
    ClearItem(self->accumArray[index]);
    if (self->interpCount == 1) {
        self->reset = 1;
        return index;
    }
    if (self->interpCount != 2) {
        return index;
    }
    AccumArrayItem& item = self->accumArray[previous];
    if (QuatTransform_IsTransformInvalid(&item.delta)) {
        Item_ClearValues(&item);
        return index;
    }
    if (!QuatTransform_IsTransformInvalid(&self->value)) {
        NiQuatTransform composed;
        self->value = *QuatTransform_Multiply(&self->value, &composed, &item.delta);
    }
    SetItemRotation(item, self->value.rotate, item.delta.rotate);
    return index;
}

// 0x8212B0: RemoveInterpInfo(index) (slot 38): the only one going: its delta composed into a
// valid value first; one left after: the accumulation starts again from it
re::Ptr32<NiInterpolator>* __fastcall BlendAccum_RemoveInterpInfo(NiBlendAccumTransformInterpolator* self, re::Edx,
                                                        re::Ptr32<NiInterpolator>* result, uint8_t index) {
    if (self->interpCount == 1 && index == self->singleIdx && !TransformInvalid(self->value)) {
        AccumArrayItem& item = self->accumArray[self->singleIdx];
        if (!QuatTransform_IsTransformInvalid(&item.delta)) {
            NiQuatTransform composed;
            self->value = *QuatTransform_Multiply(&self->value, &composed, &item.delta);
        }
    }
    BlendInterpolator_RemoveInterpInfo(self, result, index);
    if (self->interpCount == 1 && *result != nullptr) {
        self->reset = 1;
        ClearItem(self->accumArray[self->singleIdx]);
    }
    return result;
}

// 0x821390: GetAccumulatedTransform(transform&) (slot 41): none while invalid; with one
// interpolator, its delta composed in
uint8_t __fastcall BlendAccum_GetAccumulatedTransform(NiBlendAccumTransformInterpolator* self, re::Edx,
                                                      NiQuatTransform* transform) {
    if (TransformInvalid(self->value)) {
        return 0;
    }
    const NiQuatTransform* source = &self->value;
    NiQuatTransform composed;
    if (self->interpCount == 1) {
        source = QuatTransform_Multiply(&self->value, &composed, &self->accumArray[self->singleIdx].delta);
    }
    *transform = *source;
    return 1;
}

// 0x821410: the only interpolator moved on to the time: when the time went back (a loop), the
// loop's whole motion (the end of the active range over its start) composed into a valid value
// first; its delta then the transform at the time
void __fastcall BlendAccum_AccumulateSingleImpl(NiBlendAccumTransformInterpolator* self, re::Edx, float time,
                                                NiObjectNET* target) {
    AccumArrayItem& item = self->accumArray[self->singleIdx];
    NiInterpolator* interpolator = self->interpArray[self->singleIdx].interpolator;
    if (time != item.lastTime) {
        if (item.lastTime > time) {
            if (TransformInvalid(item.start)) {
                float end;
                float begin;
                Virtual<void>(interpolator, 31, &begin, &end);   // GetActiveTimeRange
                NiQuatTransform atBegin;
                QuatTransform_Construct(&atBegin);
                Virtual<uint8_t>(interpolator, 18, begin, target, &atBegin);
                NiQuatTransform atEnd;
                QuatTransform_Construct(&atEnd);
                Virtual<uint8_t>(interpolator, 18, end, target, &atEnd);
                NiQuatTransform inverse;
                QuatTransform_Construct(&inverse);
                QuatTransform_HierInvert(&atBegin, &inverse);
                NiQuatTransform loop;
                item.start = *QuatTransform_Multiply(&atEnd, &loop, &inverse);
            }
            if (!TransformInvalid(self->value)) {
                NiQuatTransform composed;
                self->value = *QuatTransform_Multiply(&self->value, &composed, &item.start);
            }
        }
        Virtual<uint8_t>(interpolator, 18, time, target, &item.delta);
    }
    item.lastTime = time;
}

// 0x821680: StoreSingleValue(time, target, transform&): the only one moved on; a fresh
// accumulation (or an invalid value) starts from the inverse of its delta; the value composed
// with the delta handed out
uint8_t __fastcall BlendAccum_StoreSingleValueImpl(NiBlendAccumTransformInterpolator* self, re::Edx, float time,
                                                   NiObjectNET* target, NiQuatTransform* value) {
    if ((self->flags & 1) != 0) {
        time = self->singleTime;
    }
    if (time == kInvalidFloat) {
        return 0;
    }
    BlendAccum_AccumulateSingle(self, time, target);
    if (self->reset != 0 || TransformInvalid(self->value)) {
        NiQuatTransform inverse;
        inverse.translate = kInvalidTranslate;
        inverse.rotate = kInvalidRotate;
        inverse.scale = kInvalidFloat;
        QuatTransform_HierInvert(&self->accumArray[self->singleIdx].delta, &inverse);
        const NiQuatTransform* source = &inverse;
        NiQuatTransform composed;
        if (!TransformInvalid(self->value)) {
            source = QuatTransform_Multiply(&self->value, &composed, &inverse);
        }
        self->value = *source;
        self->reset = 0;
    }
    if (TransformInvalid(self->value)) {
        return 0;
    }
    NiQuatTransform composed;
    *value = *QuatTransform_Multiply(&self->value, &composed, &self->accumArray[self->singleIdx].delta);
    return 1;
}

// 0x821830: BlendValues(time, target, transform&): the items' steps (their deltas when the value
// was invalid and is taken as the identity, else their starts) blended as
// NiBlendTransformInterpolator blends (translations and scales by weight, rotations summed and
// normalized), the blend concatenated onto the value, which is handed out
uint8_t __fastcall BlendAccum_BlendValuesImpl(NiBlendAccumTransformInterpolator* self, re::Edx, float, NiObjectNET*,
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
    uint8_t useDeltas = 0;
    if (TransformInvalid(self->value)) {
        self->value.translate = kPoint3Zero;
        self->value.rotate = kQuaternionIdentity;
        self->value.scale = 1.0f;
        useDeltas = 1;
    }
    uint8_t firstRotation = 1;
    if (self->arraySize == 0) {
        return 0;
    }
    for (uint8_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem& item = self->interpArray[i];
        AccumArrayItem& accum = self->accumArray[i];
        if (item.interpolator == nullptr || !(item.normalizedWeight > 0.0f)) {
            continue;
        }
        NiQuatTransform t = useDeltas ? accum.delta : accum.start;
        float w = item.normalizedWeight;
        if (t.translate.x != kInvalidFloat) {
            transChanged = 1;
            float px = t.translate.x * w;
            float py = t.translate.y * w;
            float pz = w * t.translate.z;
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
            rotate.x = rotate.x + r.x;
            rotChanged = 1;
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
    if (!transChanged && !rotChanged && !scaleChanged) {
        return 0;
    }
    float invTrans = 1.0f / transWeight;
    NiQuatTransform blended;
    blended.translate = kInvalidTranslate;
    blended.rotate = kInvalidRotate;
    float invScale = 1.0f / scaleWeight;
    blended.scale = kInvalidFloat;
    if (transChanged) {
        translate.x = translate.x * invTrans;
        blended.translate.x = translate.x;
        translate.y = translate.y * invTrans;
        blended.translate.y = translate.y;
        translate.z = invTrans * translate.z;
        blended.translate.z = translate.z;
    }
    if (rotChanged) {
        Quaternion_Normalize(&rotate);
        blended.rotate = rotate;
    }
    if (scaleChanged) {
        blended.scale = invScale * scale;
    }
    NiQuatTransform concatenated;
    self->value = *QuatTransform_Concatenate(&self->value, &concatenated, &blended);
    if (TransformInvalid(self->value)) {
        return 0;
    }
    *value = self->value;
    return 1;
}

// 0x821D20: ReallocateArrays (slot 40): the items too, the old ones copied over
uint8_t __fastcall BlendAccum_ReallocateArrays(NiBlendAccumTransformInterpolator* self, re::Edx) {
    uint8_t oldSize = self->arraySize;
    uint8_t result = BlendInterpolator_ReallocateArrays(self);
    if (!result) {
        return result;
    }
    AccumArrayItem* items = NewItems(self->arraySize);
    for (uint32_t i = 0; i < oldSize; i++) {
        items[i] = self->accumArray[i];
    }
    NiFreeArray(self->accumArray, 0x68);
    self->accumArray = items;
    return result;
}

// 0x821DC0: one item moved on to the time (Update's, with several interpolators). Without an
// interpolator, or at the same time: its start the identity. A fresh accumulation (or no delta
// yet): the delta taken now and the rotation set up from it. Then the step since the last update:
// forwards, the transform now over the last delta; when the time went back (a loop), the rest of
// the loop (its end over the last delta) then the start of the next (now over its beginning,
// with the rotation set up again from the beginning), concatenated. A step's translation turned
// into the accumulated frame; the transform now kept as the delta.
void __fastcall BlendAccum_AccumulateItemImpl(NiBlendAccumTransformInterpolator* self, re::Edx, uint8_t index,
                                              float time, NiObjectNET* target) {
    NiInterpolator* interpolator = self->interpArray[index].interpolator;
    AccumArrayItem& item = self->accumArray[index];
    if (interpolator == nullptr || time == item.lastTime) {
        item.start.translate = kPoint3Zero;
        item.start.rotate = kQuaternionIdentity;
        item.start.scale = 1.0f;
        item.lastTime = time;
        return;
    }
    if (self->reset != 0 || QuatTransform_IsTransformInvalid(&item.delta)) {
        self->reset = 0;
        Virtual<uint8_t>(interpolator, 18, time, target, &item.delta);
        SetItemRotation(item, self->value.rotate, item.delta.rotate);
    }
    NiQuatTransform now;
    if (item.lastTime > time) {
        float begin;
        float end;
        Virtual<void>(interpolator, 31, &begin, &end);   // GetActiveTimeRange
        NiQuatTransform atEnd;
        QuatTransform_Construct(&atEnd);
        Virtual<uint8_t>(interpolator, 18, end, target, &atEnd);
        NiQuatTransform atBegin;
        QuatTransform_Construct(&atBegin);
        Virtual<uint8_t>(interpolator, 18, begin, target, &atBegin);
        QuatTransform_Construct(&now);
        Virtual<uint8_t>(interpolator, 18, time, target, &now);
        NiQuatTransform inverse;
        QuatTransform_Construct(&inverse);
        QuatTransform_Invert(&item.delta, &inverse);
        NiQuatTransform step;
        item.start = *QuatTransform_Concatenate(&inverse, &step, &atEnd);
        if (item.start.translate.x != kInvalidFloat) {
            NiPoint3 turned;
            item.start.translate = *Matrix3_TimesPoint(&item.rotation, &turned, &item.start.translate);
        }
        SetItemRotation(item, self->value.rotate, atBegin.rotate);
        QuatTransform_Invert(&atBegin, &inverse);
        item.delta = *QuatTransform_Concatenate(&inverse, &step, &now);
        if (item.delta.translate.x != kInvalidFloat) {
            NiPoint3 turned;
            item.delta.translate = *Matrix3_TimesPoint(&item.rotation, &turned, &item.delta.translate);
        }
        item.start = *QuatTransform_Concatenate(&item.start, &step, &item.delta);
    } else {
        QuatTransform_Construct(&now);
        Virtual<uint8_t>(interpolator, 18, time, target, &now);
        NiQuatTransform inverse;
        QuatTransform_Construct(&inverse);
        QuatTransform_Invert(&item.delta, &inverse);
        NiQuatTransform step;
        item.start = *QuatTransform_Concatenate(&inverse, &step, &now);
        if (item.start.translate.x != kInvalidFloat) {
            NiPoint3 turned;
            item.start.translate = *Matrix3_TimesPoint(&item.rotation, &turned, &item.start.translate);
        }
    }
    item.lastTime = time;
    item.delta = now;
}

// 0x822290: CopyMembers (the items made anew for the destination and copied, the one its
// constructor made not freed, as the game has it; the value copied)
void __fastcall BlendAccum_CopyMembersImpl(NiBlendAccumTransformInterpolator* self, re::Edx,
                                           NiBlendAccumTransformInterpolator* dest, void* cloning) {
    BlendInterpolator_CopyMembers(self, dest, cloning);
    dest->accumArray = NewItems(self->arraySize);
    for (uint8_t i = 0; i < self->arraySize; i++) {
        dest->accumArray[i] = self->accumArray[i];
    }
    dest->value = self->value;
}

// 0x822320: LoadBinary (the items made anew, each one's delta, start and rotation read; then the
// value)
void __fastcall BlendAccum_LoadBinary(NiBlendAccumTransformInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_LoadBinary(self, stream);
    self->accumArray = NewItems(self->arraySize);
    for (uint8_t i = 0; i < self->arraySize; i++) {
        AccumArrayItem& item = self->accumArray[i];
        QuatTransform_LoadBinary(&item.delta, stream);
        QuatTransform_LoadBinary(&item.start, stream);
        Matrix3_LoadBinary(&item.rotation, stream);
    }
    QuatTransform_LoadBinary(&self->value, stream);
}

// 0x8215F0: SaveBinary
void __fastcall BlendAccum_SaveBinary(NiBlendAccumTransformInterpolator* self, re::Edx, void* stream) {
    BlendInterpolator_SaveBinary(self, stream);
    for (uint8_t i = 0; i < self->arraySize; i++) {
        AccumArrayItem& item = self->accumArray[i];
        QuatTransform_SaveBinary(&item.delta, stream);
        QuatTransform_SaveBinary(&item.start, stream);
        Matrix3_SaveBinary(&item.rotation, stream);
    }
    QuatTransform_SaveBinary(&self->value, stream);
}

// 0x8215D0: CreateObject
NiObject* __cdecl BlendAccum_CreateObject() {
    void* memory = NiNew(0x58);
    if (memory == nullptr) {
        return nullptr;
    }
    return BlendAccum_DefaultConstruct(memory);
}

// 0x821650: IsEqual (the values)
uint8_t __fastcall BlendAccum_IsEqual(NiBlendAccumTransformInterpolator* self, re::Edx,
                                      NiBlendAccumTransformInterpolator* other) {
    if (!BlendInterpolator_IsEqual(self, other)) {
        return 0;
    }
    return QuatTransform_Equal(&self->value, &other->value) != 0 ? 1 : 0;
}

// 0x8223B0: Update(time, target, transform&) (slot 18): the only one's value through
// StoreSingleValue; else the weights normalized, each item with a weight and a valid time moved
// on, then blended
uint8_t __fastcall BlendAccum_Update(NiBlendAccumTransformInterpolator* self, re::Edx, float time, NiObjectNET* target,
                                     NiQuatTransform* value) {
    uint8_t result = 0;
    if (self->interpCount == 1) {
        result = BlendAccum_StoreSingleValue(self, time, target, value);
        self->lastTime = time;
        return result;
    }
    if (self->interpCount != 0) {
        BlendInterpolator_ComputeNormalizedWeights(self);
        for (uint8_t i = 0; i < self->arraySize; i++) {
            float updateTime = time;
            InterpArrayItem& item = self->interpArray[i];
            if (item.interpolator == nullptr || item.normalizedWeight == 0.0f) {
                continue;
            }
            if ((self->flags & 1) != 0) {
                updateTime = item.updateTime;
            }
            if (updateTime != kInvalidFloat) {
                BlendAccum_AccumulateItem(self, i, updateTime, target);
            }
        }
        result = BlendAccum_BlendValues(self, time, target, value);
    }
    self->lastTime = time;
    return result;
}

// 0x822490: CreateClone (a failed allocation still copied onto null)
NiBlendAccumTransformInterpolator* __fastcall BlendAccum_CreateClone(NiBlendAccumTransformInterpolator* self, re::Edx,
                                                                     void* cloning) {
    void* memory = NiNew(0x58);
    if (memory == nullptr) {
        BlendAccum_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiBlendAccumTransformInterpolator* clone = BlendAccum_DefaultConstruct(memory);
    BlendAccum_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x8224E0: GetViewerStrings
void __fastcall BlendAccum_GetViewerStrings(NiBlendAccumTransformInterpolator* self, re::Edx,
                                            NiTArray<char*>* strings) {
    BlendInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBlendAccumTransformInterpolator_RTTI.name));
    QuatTransform_GetViewerStrings(&self->value, strings);
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC32_ACCUM_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::~NiBlendAccumTransformInterpolator", 0x00821060,
                     &ni::anim::BlendAccum_Destruct);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::NiBlendAccumTransformInterpolator()", 0x00821090,
                     &ni::anim::BlendAccum_DefaultConstructImpl);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::AddInterpInfo", 0x008210F0,
                     &ni::anim::BlendAccum_AddInterpInfo);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::RemoveInterpInfo", 0x008212B0,
                     &ni::anim::BlendAccum_RemoveInterpInfo);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::GetAccumulatedTransform", 0x00821390,
                     &ni::anim::BlendAccum_GetAccumulatedTransform);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::AccumulateSingle", 0x00821410,
                     &ni::anim::BlendAccum_AccumulateSingleImpl);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::CreateObject", 0x008215D0,
                     &ni::anim::BlendAccum_CreateObject);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::SaveBinary", 0x008215F0, &ni::anim::BlendAccum_SaveBinary);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::IsEqual", 0x00821650, &ni::anim::BlendAccum_IsEqual);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::StoreSingleValue", 0x00821680,
                     &ni::anim::BlendAccum_StoreSingleValueImpl);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::BlendValues", 0x00821830,
                     &ni::anim::BlendAccum_BlendValuesImpl);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::ReallocateArrays", 0x00821D20,
                     &ni::anim::BlendAccum_ReallocateArrays);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::AccumulateItem", 0x00821DC0,
                     &ni::anim::BlendAccum_AccumulateItemImpl);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::CopyMembers", 0x00822290,
                     &ni::anim::BlendAccum_CopyMembersImpl);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::LoadBinary", 0x00822320, &ni::anim::BlendAccum_LoadBinary);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::Update", 0x008223B0, &ni::anim::BlendAccum_Update);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::CreateClone", 0x00822490, &ni::anim::BlendAccum_CreateClone);
NIC32_ACCUM_REVERSED("NiBlendAccumTransformInterpolator::GetViewerStrings", 0x008224E0,
                     &ni::anim::BlendAccum_GetViewerStrings);
