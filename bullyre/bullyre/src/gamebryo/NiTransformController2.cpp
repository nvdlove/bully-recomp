// NiTransformController's slots of this chunk (vtable 0x966840, 0x38 bytes: a
// NiSingleInterpController driving its target's local transform from a transform interpolator;
// the rest is src/gamebryo/NiTransformController.cpp): the constructor, Update, the pose and
// blend interpolators it makes, cloning, creation and the viewer strings.
//
// Main-thread float maths at the game's 24-bit x87 precision.

#include <cmath>
#include <cstddef>
#include <cstdint>

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
GAME_VAR(RTTI, NiTransformController_RTTI, 0x020E4EF8);
GAME_VAR(uint32_t, kNiAVObjectClassIndex, 0x020E251C);
GAME_VAR(uint32_t, kNiTransformInterpolatorClassIndex, 0x020E4B14);

constexpr uintptr_t NiTransformController_vtable = 0x00966840;

GAME_FN(void, __thiscall, Quaternion_FromRotation, 0x00776040, NiQuaternion* self, const NiMatrix3* m);
GAME_FN(void, __thiscall, Quaternion_ToRotation, 0x00413C50, const NiQuaternion* self, NiMatrix3* m);
GAME_FN(uint8_t, __thiscall, TimeController_DontDoUpdate, 0x0077D890, void* self, float time);
// NiSingleInterpController's (the next chunks')
GAME_FN(void, __thiscall, SingleInterp_Construct, 0x008250C0, void* self);
GAME_FN(void, __thiscall, SingleInterp_CopyMembers, 0x008251B0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, SingleInterp_GetViewerStrings, 0x00825350, void* self, NiTArray<char*>* strings);
// the blend interpolators made (NiBlendTransformInterpolator: the next chunk's; the accumulating one:
// src/gamebryo/NiTransformController.cpp)
GAME_FN(NiInterpolator*, __thiscall, BlendTransformInterpolator_Construct, 0x0081EFB0, void* self,
        int managerControlled, float weightThreshold, int arraySize);
GAME_FN(NiInterpolator*, __thiscall, BlendAccumTransformInterpolator_Construct, 0x008104B0, void* self,
        int managerControlled, float weightThreshold, int arraySize);
// NiTransformInterpolator::SetPoseValue (the pose by value: translation, rotation (w, x, y, z), scale)
GAME_FN(void, __thiscall, TransformInterpolator_SetPoseValue_x64, 0x008105B0, void* self, re::Floats<8>);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void TransformInterpolator_SetPoseValue(void* a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8) {
    return TransformInterpolator_SetPoseValue_x64(a0, re::Floats<8>{{a1, a2, a3, a4, a5, a6, a7, a8}});
}

// 0x810660: NiTransformController()
NiSingleInterpController* __fastcall TC_Construct(NiSingleInterpController* self, re::Edx) {
    SingleInterp_Construct(self);
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(NiTransformController_vtable);
    return self;
}

const RTTI* __fastcall TC_GetRTTI(NiSingleInterpController*, re::Edx) {
    return &NiTransformController_RTTI;
}

// 0x810690: SynchronizePoseInterpolator(interpolator, index) (slot 36): the transform interpolator
// posed at the target's local transform (a null one, or another kind, posed through null as the
// game has it)
void __fastcall TC_SynchronizePoseInterpolator(NiSingleInterpController* self, re::Edx, NiInterpolator* interpolator,
                                               uint16_t) {
    NiInterpolator* transform = nullptr;
    if (interpolator != nullptr && IsKindOf(interpolator, kNiTransformInterpolatorClassIndex)) {
        transform = interpolator;
    }
    NiAVObject* target = static_cast<NiAVObject*>(self->target);
    NiQuaternion q;
    Quaternion_FromRotation(&q, &target->local.rotate);
    NiQuatTransform pose;
    pose.scale = target->local.scale;
    pose.translate = target->local.translate;
    pose.rotate = q;
    TransformInterpolator_SetPoseValue(transform, pose.translate.x, pose.translate.y, pose.translate.z, pose.rotate.w,
                                       pose.rotate.x, pose.rotate.y, pose.rotate.z, pose.scale);
}

// 0x810740: CreateBlendInterpolator(index, managerControlled, accumulate, weightThreshold,
// arraySize) (slot 37): an accumulating blend transform interpolator or a plain one
NiInterpolator* __fastcall TC_CreateBlendInterpolator(NiSingleInterpController*, re::Edx, uint16_t,
                                                      int managerControlled, int accumulate, float weightThreshold,
                                                      int arraySize) {
    if (re::Flag(accumulate)) {
        void* memory = NiNew(0x58);
        if (memory == nullptr) {
            return nullptr;
        }
        return BlendAccumTransformInterpolator_Construct(memory, managerControlled, weightThreshold, arraySize);
    }
    void* memory = NiNew(0x30);
    if (memory == nullptr) {
        return nullptr;
    }
    return BlendTransformInterpolator_Construct(memory, managerControlled, weightThreshold, arraySize);
}

// 0x8107A0: Update(time) (slot 20): the target's local translation, rotation and scale (its
// absolute value) set from the parts the interpolator gives
void __fastcall TC_Update(NiSingleInterpController* self, re::Edx, float time) {
    if (((uint8_t(self->flags) >> 5) & 1) != 0) {
        self->scaledTime = kInvalidFloat;
    } else if (TimeController_DontDoUpdate(self, time)) {
        if (self->interpolator == nullptr || !Virtual<uint8_t>(self->interpolator, 36)) {
            return;
        }
    }
    if (self->interpolator == nullptr) {
        return;
    }
    NiQuatTransform value;
    value.translate = kInvalidTranslate;
    value.rotate = kInvalidRotate;
    value.scale = kInvalidFloat;
    if (!Virtual<uint8_t>(self->interpolator, 18, self->scaledTime, self->target, &value)) {
        return;
    }
    NiAVObject* target = static_cast<NiAVObject*>(self->target);
    if (value.translate.x != kInvalidFloat) {
        target->local.translate = value.translate;
    }
    if (value.rotate.x != kInvalidFloat) {
        Quaternion_ToRotation(&value.rotate, &target->local.rotate);
    }
    if (value.scale != kInvalidFloat) {
        target->local.scale = std::fabs(value.scale);
    }
}

// 0x8108E0: InterpTargetIsCorrectType(target) (slot 40): a NiAVObject
uint8_t __fastcall TC_InterpTargetIsCorrectType(NiSingleInterpController*, re::Edx, NiObject* target) {
    if (target == nullptr) {
        return 0;
    }
    return IsKindOf(target, kNiAVObjectClassIndex);
}

// 0x810920: CreateClone (a failed allocation still copied onto null, as the game has it)
NiSingleInterpController* __fastcall TC_CreateClone(NiSingleInterpController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x38);
    if (memory == nullptr) {
        SingleInterp_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    SingleInterp_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(NiTransformController_vtable);
    SingleInterp_CopyMembers(self, memory, cloning);
    return static_cast<NiSingleInterpController*>(memory);
}

// 0x810970: CreateObject (NiTransformController's and NiKeyframeController's loader)
NiObject* __cdecl TC_CreateObject() {
    void* memory = NiNew(0x38);
    if (memory == nullptr) {
        return nullptr;
    }
    SingleInterp_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(NiTransformController_vtable);
    return static_cast<NiObject*>(memory);
}

// 0x8109A0: GetViewerStrings
void __fastcall TC_GetViewerStrings(NiSingleInterpController* self, re::Edx, NiTArray<char*>* strings) {
    SingleInterp_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTransformController_RTTI.name));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_XFCTLR_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC31_XFCTLR_REVERSED("NiTransformController::NiTransformController", 0x00810660, &ni::anim::TC_Construct);
NIC31_XFCTLR_REVERSED("NiTransformController::GetRTTI", 0x00810680, &ni::anim::TC_GetRTTI);
NIC31_XFCTLR_REVERSED("NiTransformController::SynchronizePoseInterpolator", 0x00810690,
                      &ni::anim::TC_SynchronizePoseInterpolator);
NIC31_XFCTLR_REVERSED("NiTransformController::CreateBlendInterpolator", 0x00810740,
                      &ni::anim::TC_CreateBlendInterpolator);
NIC31_XFCTLR_REVERSED("NiTransformController::Update", 0x008107A0, &ni::anim::TC_Update);
NIC31_XFCTLR_REVERSED("NiTransformController::InterpTargetIsCorrectType", 0x008108E0,
                      &ni::anim::TC_InterpTargetIsCorrectType);
NIC31_XFCTLR_REVERSED("NiTransformController::CreateClone", 0x00810920, &ni::anim::TC_CreateClone);
NIC31_XFCTLR_REVERSED("NiTransformController::CreateObject", 0x00810970, &ni::anim::TC_CreateObject);
NIC31_XFCTLR_REVERSED("NiTransformController::GetViewerStrings", 0x008109A0, &ni::anim::TC_GetViewerStrings);
