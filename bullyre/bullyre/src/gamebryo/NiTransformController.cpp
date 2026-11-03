// NiTransformController's slots of this chunk (vtable 0x966840, a NiSingleInterpController
// driving its target's local transform; its Update and the rest are the next chunk's), and
// NiBlendAccumTransformInterpolator's constructor, accumulation item, RTTI and destructor slot
// (vtable 0x966798, 0x58 bytes).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::Stream_ReadLinkID;
using scene::Stream_ResolveLinkID;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBlendAccumTransformInterpolator_RTTI, 0x020E55B0);

constexpr uintptr_t NiBlendAccumTransformInterpolator_vtable = 0x00966798;
constexpr uintptr_t AccumArrayItem_Construct_address = 0x008103F0;   // (this file's, the array's constructor)

GAME_VAR(NiMatrix3, kMatrixIdentity, 0x00BC75FC);

GAME_FN(void*, __cdecl, NiAllocArray, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __thiscall, Quaternion_FromRotation, 0x00776040, NiQuaternion* self, const NiMatrix3* m);
GAME_FN(NiInterpolator*, __thiscall, TransformInterpolator_ConstructWithData, 0x00805280, void* self, NiObject* data);
// (the pose by value: translation, rotation (w, x, y, z), scale)
GAME_FN(NiInterpolator*, __thiscall, TransformInterpolator_ConstructWithPose, 0x00805320, void* self, float tx,
        float ty, float tz, float rw, float rx, float ry, float rz, float scale);

inline NiInterpolator* ConstructWithPose(void* self, const NiQuatTransform& p) {
    return TransformInterpolator_ConstructWithPose(self, p.translate.x, p.translate.y, p.translate.z, p.rotate.w,
                                                   p.rotate.x, p.rotate.y, p.rotate.z, p.scale);
}
// NiSingleInterpController's, NiBlendInterpolator's (the next chunk's)
GAME_FN(void, __thiscall, SingleInterp_LoadBinaryImpl, 0x00825240, void* self, void* stream);
GAME_FN(void, __thiscall, SingleInterp_LinkObjectImpl, 0x008252A0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, SingleInterp_IsEqualImpl, 0x00825300, void* self, void* other);
GAME_FN(void, __thiscall, BlendInterpolator_Construct, 0x00827320, void* self, uint8_t managerControlled,
        float weightThreshold, uint8_t arraySize);
GAME_FN(void, __thiscall, BlendAccumTransformInterpolator_Destruct, 0x00821060, void* self);

// ---- NiTransformController

// 0x810260: InterpolatorIsCorrectType(interpolator, index) (slot 39; NiMultiTargetTransform
// Controller's too): one that gives transforms (slot 29)
uint8_t __fastcall TC_InterpolatorIsCorrectType(NiSingleInterpController*, re::Edx, NiInterpolator* interpolator,
                                                uint16_t) {
    return Virtual<uint8_t>(interpolator, 29);
}

// 0x810270: IsEqual
uint8_t __fastcall TC_IsEqual(NiSingleInterpController* self, re::Edx, NiSingleInterpController* other) {
    return SingleInterp_IsEqualImpl(self, other) != 0 ? 1 : 0;
}

// 0x8102C0: CreatePoseInterpolator(index) (slot 35): a transform interpolator posed at the
// target's local transform
NiInterpolator* __fastcall TC_CreatePoseInterpolator(NiSingleInterpController* self, re::Edx, uint16_t) {
    NiAVObject* target = static_cast<NiAVObject*>(self->target);
    NiQuaternion q;
    Quaternion_FromRotation(&q, &target->local.rotate);
    NiQuatTransform pose;
    pose.scale = target->local.scale;
    pose.translate = target->local.translate;
    pose.rotate = q;
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        return nullptr;
    }
    return ConstructWithPose(memory, pose);
}

// 0x810350: LoadBinary (an older file's data link read)
void __fastcall TC_LoadBinary(NiSingleInterpController* self, re::Edx, void* stream) {
    SingleInterp_LoadBinaryImpl(self, stream);
    if (StreamVersion(stream) < 0x0A010068) {
        Stream_ReadLinkID(stream);
    }
}

// 0x810370: LinkObject: an older file's NiTransformData (a keyframe controller's) made into a
// NiTransformInterpolator (collapsed)
void __fastcall TC_LinkObject(NiSingleInterpController* self, re::Edx, void* stream) {
    SingleInterp_LinkObjectImpl(self, stream);
    if (StreamVersion(stream) < 0x0A010068) {
        NiObject* data = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        void* memory = NiNew(0x3C);
        NiInterpolator* made = memory != nullptr ? TransformInterpolator_ConstructWithData(memory, data) : nullptr;
        AssignReleasingFirst(self->interpolator, made);
        Virtual<void>(self->interpolator, 30);   // Collapse
    }
}

// ---- NiBlendAccumTransformInterpolator

// 0x810290: AccumArrayItem::ClearValues (the parts invalid, the rotation identity)
void __fastcall Item_ClearValues(AccumArrayItem* self, re::Edx) {
    self->lastTime = kInvalidFloat;
    self->delta.translate.x = kInvalidFloat;
    self->delta.rotate.x = kInvalidFloat;
    self->delta.scale = kInvalidFloat;
    self->start.translate.x = kInvalidFloat;
    self->start.rotate.x = kInvalidFloat;
    self->start.scale = kInvalidFloat;
    self->rotation = kMatrixIdentity;
}

// 0x8103F0: AccumArrayItem() (both transforms invalid, then cleared)
AccumArrayItem* __fastcall Item_Construct(AccumArrayItem* self, re::Edx) {
    self->delta.translate = kInvalidTranslate;
    self->delta.rotate = kInvalidRotate;
    self->delta.scale = kInvalidFloat;
    self->start.translate = kInvalidTranslate;
    self->start.rotate = kInvalidRotate;
    self->start.scale = kInvalidFloat;
    Item_ClearValues(self, nullptr);
    return self;
}

// 0x8104B0: NiBlendAccumTransformInterpolator(managerControlled, weightThreshold, arraySize):
// an accumulation item per array slot (NiAllocArray'd, no count before them)
void* __fastcall BlendAccum_Construct(uint8_t* self, re::Edx, uint8_t managerControlled, float weightThreshold,
                                      uint8_t arraySize) {
    BlendInterpolator_Construct(self, managerControlled, weightThreshold, arraySize);
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(NiBlendAccumTransformInterpolator_vtable);
    NiQuatTransform& value = *reinterpret_cast<NiQuatTransform*>(self + 0x30);
    value.translate = kInvalidTranslate;
    value.rotate = kInvalidRotate;
    value.scale = kInvalidFloat;
    uint8_t count = self[0xD];
    re::Ptr32<AccumArrayItem>& items = *reinterpret_cast<re::Ptr32<AccumArrayItem>*>(self + 0x50);
    items = nullptr;
    self[0x54] = 0;
    if (count != 0) {
        void* memory = NiAllocArray(uint32_t(count) * 0x68);
        if (memory != nullptr) {
            VectorConstruct(memory, 0x68, count, reinterpret_cast<void*>(AccumArrayItem_Construct_address));
            items = static_cast<AccumArrayItem*>(memory);
            return self;
        }
        items = nullptr;
    }
    return self;
}

const RTTI* __fastcall BlendAccum_GetRTTI(void*, re::Edx) {
    return &NiBlendAccumTransformInterpolator_RTTI;
}

// 0x810590: the deleting destructor
void* __fastcall BlendAccum_DeletingDestructor(void* self, re::Edx, uint32_t flags) {
    BlendAccumTransformInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x58);
    }
    return self;
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_XFCTLR_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC30_XFCTLR_REVERSED("NiTransformController::InterpolatorIsCorrectType", 0x00810260,
                      &ni::anim::TC_InterpolatorIsCorrectType);
NIC30_XFCTLR_REVERSED("NiTransformController::IsEqual", 0x00810270, &ni::anim::TC_IsEqual);
NIC30_XFCTLR_REVERSED("NiBlendAccumTransformInterpolator::AccumArrayItem::ClearValues", 0x00810290,
                      &ni::anim::Item_ClearValues);
NIC30_XFCTLR_REVERSED("NiTransformController::CreatePoseInterpolator", 0x008102C0,
                      &ni::anim::TC_CreatePoseInterpolator);
NIC30_XFCTLR_REVERSED("NiTransformController::LoadBinary", 0x00810350, &ni::anim::TC_LoadBinary);
NIC30_XFCTLR_REVERSED("NiTransformController::LinkObject", 0x00810370, &ni::anim::TC_LinkObject);
NIC30_XFCTLR_REVERSED("NiBlendAccumTransformInterpolator::AccumArrayItem::AccumArrayItem", 0x008103F0,
                      &ni::anim::Item_Construct);
NIC30_XFCTLR_REVERSED("NiBlendAccumTransformInterpolator::NiBlendAccumTransformInterpolator", 0x008104B0,
                      &ni::anim::BlendAccum_Construct);
NIC30_XFCTLR_REVERSED("NiBlendAccumTransformInterpolator::GetRTTI", 0x00810580, &ni::anim::BlendAccum_GetRTTI);
NIC30_XFCTLR_REVERSED("NiBlendAccumTransformInterpolator::DeletingDestructor", 0x00810590,
                      &ni::anim::BlendAccum_DeletingDestructor);
