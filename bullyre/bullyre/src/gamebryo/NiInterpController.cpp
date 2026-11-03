// The interpolator-driven controllers' bases of this part of the library (src/gamebryo/
// NiInterpolators.h): NiInterpController (vtable 0x967FA8, 0x34: the time controller with the
// manager-controlled flag streamed), NiSingleInterpController (vtable 0x968070, 0x38: one
// interpolator, its slots), NiBoolInterpController (vtable 0x967EE0) and NiFloatInterpController's
// pose and blend interpolators, and NiAlphaController (vtable 0x967D30, 0x38: a NiMaterialProperty's
// alpha from a float).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiProperties.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::GetViewerStringBool;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiAlphaController_RTTI, 0x020E55E4);
GAME_VAR(RTTI, NiBoolInterpController_RTTI, 0x020E5618);
GAME_VAR(RTTI, NiInterpController_RTTI, 0x020E564C);
GAME_VAR(RTTI, NiSingleInterpController_RTTI, 0x020E5680);
GAME_VAR(uint32_t, ms_NiMaterialPropertyClassIndex, 0x020E2D0C);   // (the RTTI's +8)
GAME_VAR(uint32_t, ms_NiBoolInterpolatorClassIndex, 0x020E53E4);

constexpr uintptr_t NiAlphaController_vtable = 0x00967D30;
constexpr uintptr_t NiBoolInterpController_vtable = 0x00967EE0;
constexpr uintptr_t NiInterpController_vtable = 0x00967FA8;
constexpr uintptr_t NiSingleInterpController_vtable = 0x00968070;

GAME_FN(NiObject*, __thiscall, Object_CloneWith, 0x007553B0, void* self, void* cloning);   // NiObject::Clone(cloning)
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
// NiTimeController's (and the thunks to them the code calls)
GAME_FN(NiTimeController*, __thiscall, TimeController_Construct, 0x0077DFA0, void* self);
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
GAME_FN(void, __thiscall, TimeController_ProcessClone, 0x0077DEE0, void* self, void* cloning);
GAME_FN(void, __thiscall, TimeController_LoadBinary, 0x0077E1F0, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_GetViewerStrings, 0x0077E550, void* self, NiTArray<char*>* strings);
GAME_FN(uint8_t, __thiscall, TimeController_DontDoUpdate, 0x0077D890, void* self, float time);
GAME_FN(void, __thiscall, InterpController_CopyMembers, 0x00824ED0, void* self, void* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, InterpController_RegisterStreamables, 0x00824EE0, void* self, void* stream);
GAME_FN(void, __thiscall, InterpController_SaveBinary, 0x00824EF0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, InterpController_IsEqual, 0x00824F00, void* self, void* other);
// the interpolators these make (their files')
GAME_FN(NiInterpolator*, __thiscall, BoolInterpolator_ConstructWithPose, 0x0081CFC0, void* self, uint32_t pose);
GAME_FN(NiInterpolator*, __thiscall, BlendBoolInterpolator_Construct, 0x00820BD0, void* self,
        uint32_t managerControlled, float weightThreshold, uint32_t arraySize);
GAME_FN(NiInterpolator*, __thiscall, FloatInterpolator_ConstructWithPose, 0x008122B0, void* self, float pose);
GAME_FN(NiInterpolator*, __thiscall, BlendFloatInterpolator_Construct, 0x008201E0, void* self,
        uint32_t managerControlled, float weightThreshold, uint32_t arraySize);
GAME_FN(void, __thiscall, BoolInterpolator_SetPoseValue, 0x0081D110, void* self, uint32_t pose);
GAME_FN(void, __thiscall, FloatInterpController_Construct, 0x00825F30, void* self);
GAME_FN(void, __thiscall, FloatInterpController_GetViewerStrings, 0x00826010, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, SingleInterp_CopyMembersThunk, 0x00827DC0, void* self, void* dest, void* cloning);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, InterpController_Construct, 0x00824F10, void* self);
GAME_FN(void, __thiscall, InterpController_ResetTimeExtrema, 0x00824F60, void* self);
GAME_FN(void, __thiscall, InterpController_LoadBinary, 0x00824F90, void* self, void* stream);
GAME_FN(void, __thiscall, InterpController_GetViewerStrings, 0x00825000, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, SingleInterp_Construct, 0x008250C0, void* self);
GAME_FN(void, __thiscall, SingleInterp_GetViewerStrings, 0x00825350, void* self, NiTArray<char*>* strings);

// a pointer compared as the IsEqual have it: both there and equal (slot 8), or both missing
inline uint8_t ObjectsEqual(NiObject* mine, NiObject* other) {
    if (mine != nullptr) {
        if (other == nullptr) {
            return 0;
        }
    } else {
        if (other != nullptr) {
            return 0;
        }
        return 1;
    }
    return Virtual<uint8_t>(mine, 8, other) ? 1 : 0;
}

// ---- NiAlphaController

// 0x822550: GetTargetFloatValue(value&) (slot 41): the material's alpha
void __fastcall Alpha_GetTargetFloatValue(NiSingleInterpController* self, re::Edx, float* value) {
    *value = re::PtrCast<NiMaterialProperty*>(self->target)->alpha;
}

// 0x822560: InterpTargetIsCorrectType(target) (slot 40; NiMaterialColorController's too): a
// NiMaterialProperty
uint8_t __fastcall Alpha_InterpTargetIsCorrectType(NiSingleInterpController*, re::Edx, NiObjectNET* target) {
    if (target == nullptr) {
        return 0;
    }
    return IsKindOf(target, ms_NiMaterialPropertyClassIndex);
}

// 0x8225A0: Update(time) (slot 20): the material's alpha set, its revision moved on.
// Manager controlled: the scaled time invalid (the manager sets it); else nothing when
// DontDoUpdate holds unless the interpolator always updates. (The interpolator writes its value
// over the time argument's slot)
void __fastcall Alpha_Update(NiSingleInterpController* self, re::Edx, float time) {
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
    float value = time;
    if (!Virtual<uint8_t>(self->interpolator, 22, self->scaledTime, self->target, &value)) {
        return;
    }
    auto* material = re::PtrCast<NiMaterialProperty*>(self->target);
    material->revisionID++;
    material->alpha = value;
}

// 0x822620
const RTTI* __fastcall Alpha_GetRTTI(NiSingleInterpController*, re::Edx) {
    return &NiAlphaController_RTTI;
}

// 0x822630: CreateClone (a failed allocation still copied onto null)
NiSingleInterpController* __fastcall Alpha_CreateClone(NiSingleInterpController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x38);
    if (memory == nullptr) {
        SingleInterp_CopyMembersThunk(self, nullptr, cloning);
        return nullptr;
    }
    FloatInterpController_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(NiAlphaController_vtable);
    SingleInterp_CopyMembersThunk(self, memory, cloning);
    return static_cast<NiSingleInterpController*>(memory);
}

// 0x822680: CreateObject
NiObject* __cdecl Alpha_CreateObject() {
    void* memory = NiNew(0x38);
    if (memory == nullptr) {
        return nullptr;
    }
    FloatInterpController_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(NiAlphaController_vtable);
    return static_cast<NiObject*>(memory);
}

// 0x8226B0: GetViewerStrings
void __fastcall Alpha_GetViewerStrings(NiSingleInterpController* self, re::Edx, NiTArray<char*>* strings) {
    FloatInterpController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiAlphaController_RTTI.name));
}

// ---- NiBoolInterpController

// 0x824D50: CreatePoseInterpolator(index) (slot 35): a NiBoolInterpolator posed at the target's
// value (slot 41; it writes a byte over a slot holding this controller, the whole slot passed)
NiInterpolator* __fastcall Bool_CreatePoseInterpolator(NiSingleInterpController* self, re::Edx, uint16_t) {
    uint32_t pose = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self));
    Virtual<void>(self, 41, &pose);
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    return BoolInterpolator_ConstructWithPose(memory, pose);
}

// 0x824D90: CreateBlendInterpolator(index, manager controlled, accumulate, weight threshold,
// array size) (slot 37): a NiBlendBoolInterpolator
NiInterpolator* __fastcall Bool_CreateBlendInterpolator(NiSingleInterpController*, re::Edx, uint32_t,
                                                        uint32_t managerControlled, uint32_t, float weightThreshold,
                                                        uint32_t arraySize) {
    void* memory = NiNew(0x34);
    if (memory == nullptr) {
        return nullptr;
    }
    return BlendBoolInterpolator_Construct(memory, managerControlled, weightThreshold, arraySize);
}

// 0x824DC0: InterpolatorIsCorrectType(interp, index) (slot 39): one giving a bool (slot 24)
uint8_t __fastcall Bool_InterpolatorIsCorrectType(NiSingleInterpController*, re::Edx, NiInterpolator* interpolator,
                                                  uint16_t) {
    return Virtual<uint8_t>(interpolator, 24);
}

// 0x824DE0: NiBoolInterpController()
NiSingleInterpController* __fastcall BoolInterp_Construct(NiSingleInterpController* self, re::Edx) {
    SingleInterp_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiBoolInterpController_vtable);
    return self;
}

// 0x824E00
const RTTI* __fastcall BoolInterp_GetRTTI(NiSingleInterpController*, re::Edx) {
    return &NiBoolInterpController_RTTI;
}

// 0x824E10: SynchronizePoseInterpolator(interp, index) (slot 36): the bool interpolator (a
// NiBoolInterpolator: anything else is null here, called through all the same) posed at the
// target's value (written over the interpolator argument's slot, the whole slot passed)
void __fastcall Bool_SynchronizePoseInterpolator(NiSingleInterpController* self, re::Edx, NiInterpolator* interpolator,
                                                 uint16_t) {
    NiInterpolator* boolInterp = nullptr;
    if (interpolator != nullptr && IsKindOf(interpolator, ms_NiBoolInterpolatorClassIndex)) {
        boolInterp = interpolator;
    }
    uint32_t pose = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpolator));
    Virtual<void>(self, 41, &pose);
    BoolInterpolator_SetPoseValue(boolInterp, pose);
}

// 0x824E70: GetViewerStrings
void __fastcall BoolInterp_GetViewerStrings(NiSingleInterpController* self, re::Edx, NiTArray<char*>* strings) {
    SingleInterp_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBoolInterpController_RTTI.name));
}

// ---- NiInterpController

// 0x824F10: NiInterpController() (not manager controlled)
NiTimeController* __fastcall Interp_Construct(NiTimeController* self, re::Edx) {
    TimeController_Construct(self);
    self->flags &= 0xFFDF;
    self->vtable = reinterpret_cast<void*>(NiInterpController_vtable);
    return self;
}

// 0x824F30
const RTTI* __fastcall Interp_GetRTTI(NiTimeController*, re::Edx) {
    return &NiInterpController_RTTI;
}

// 0x824F40: the deleting destructor
NiTimeController* __fastcall Interp_DeletingDestructor(NiTimeController* self, re::Edx, uint32_t flags) {
    TimeController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x34);
    }
    return self;
}

// 0x824F60: ResetTimeExtrema (slot 33): 0 and 0 unless the manager controls it
void __fastcall Interp_ResetTimeExtrema(NiTimeController* self, re::Edx) {
    if (((uint8_t(self->flags) >> 5) & 1) == 0) {
        self->loKeyTime = 0.0f;
        self->hiKeyTime = 0.0f;
    }
}

// 0x824F90: LoadBinary (from 10.1.0.104 to before 10.1.0.109 the manager-controlled flag follows)
void __fastcall Interp_LoadBinary(NiTimeController* self, re::Edx, void* stream) {
    TimeController_LoadBinary(self, stream);
    uint32_t version = scene::StreamVersion(stream);
    if (version >= 0x0A010068 && version < 0x0A01006D) {
        uint8_t managerControlled = 0;
        scene::LoadBytes(stream, &managerControlled, 1);
        if (managerControlled != 0) {
            self->flags |= 0x20;
        } else {
            self->flags &= 0xFFDF;
        }
    }
}

// 0x825000: GetViewerStrings
void __fastcall Interp_GetViewerStrings(NiTimeController* self, re::Edx, NiTArray<char*>* strings) {
    TimeController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiInterpController_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("IsManagerControlled", (uint8_t(self->flags) >> 5) & 1));
}

// ---- NiSingleInterpController

// 0x8250A0: GetInterpolatorIndex(id) (slot 30): 0
uint16_t __fastcall Single_GetInterpolatorIndex(NiSingleInterpController*, re::Edx, const char*) {
    return 0;
}

// 0x8250B0: TargetIsRequiredType (slot 27): the target as InterpTargetIsCorrectType (slot 40) has it
uint8_t __fastcall Single_TargetIsRequiredType(NiSingleInterpController* self, re::Edx) {
    return Virtual<uint8_t>(self, 40, self->target);
}

// 0x8250C0: NiSingleInterpController() (no interpolator)
NiSingleInterpController* __fastcall Single_Construct(NiSingleInterpController* self, re::Edx) {
    InterpController_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiSingleInterpController_vtable);
    self->interpolator = nullptr;
    return self;
}

// 0x8250E0
const RTTI* __fastcall Single_GetRTTI(NiSingleInterpController*, re::Edx) {
    return &NiSingleInterpController_RTTI;
}

// 0x8250F0: GetInterpolator(index) (slot 31)
NiInterpolator* __fastcall Single_GetInterpolator(NiSingleInterpController* self, re::Edx, uint16_t) {
    return self->interpolator;
}

// 0x825100: SetInterpolator(interp, index) (slot 32)
void __fastcall Single_SetInterpolator(NiSingleInterpController* self, re::Edx, NiInterpolator* interpolator, uint16_t) {
    AssignReleasingFirst(self->interpolator, interpolator);
}

// 0x825150: GuaranteeTimeRange(start, end) (slot 38): the interpolator's
void __fastcall Single_GuaranteeTimeRange(NiSingleInterpController* self, re::Edx, float start, float end) {
    if (self->interpolator != nullptr) {
        Virtual<void>(self->interpolator, 32, start, end);
    }
}

// 0x825180: ResetTimeExtrema (slot 33): the interpolator's active range, unless the manager
// controls it
void __fastcall Single_ResetTimeExtrema(NiSingleInterpController* self, re::Edx) {
    if (((uint8_t(self->flags) >> 5) & 1) != 0) {
        return;
    }
    InterpController_ResetTimeExtrema(self);
    if (self->interpolator != nullptr) {
        Virtual<void>(self->interpolator, 31, &self->loKeyTime, &self->hiKeyTime);
    }
}

// 0x8251B0: CopyMembers (the interpolator cloned through the cloning)
void __fastcall Single_CopyMembers(NiSingleInterpController* self, re::Edx, NiSingleInterpController* dest,
                                   void* cloning) {
    InterpController_CopyMembers(self, dest, cloning);
    if (self->interpolator != nullptr) {
        AssignReleasingFirst(dest->interpolator, Object_CloneWith(self->interpolator, cloning));
    }
}

// 0x825210: ProcessClone (slot 11): the interpolator's too
void __fastcall Single_ProcessClone(NiSingleInterpController* self, re::Edx, void* cloning) {
    TimeController_ProcessClone(self, cloning);
    if (self->interpolator != nullptr) {
        Virtual<void>(self->interpolator, 11, cloning);
    }
}

// 0x825240: LoadBinary (the interpolator's link from 10.1.0.104 on)
void __fastcall Single_LoadBinary(NiSingleInterpController* self, re::Edx, void* stream) {
    InterpController_LoadBinary(self, stream);
    if (scene::StreamVersion(stream) >= 0x0A010068) {
        AssignReleasingFirst(self->interpolator, Stream_ReadLinkIDObject(stream));
    }
}

// 0x8252B0: RegisterStreamables
uint8_t __fastcall Single_RegisterStreamables(NiSingleInterpController* self, re::Edx, void* stream) {
    if (!InterpController_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->interpolator != nullptr) {
        Virtual<uint8_t>(self->interpolator, 6, stream);
    }
    return 1;
}

// 0x8252E0: SaveBinary
void __fastcall Single_SaveBinary(NiSingleInterpController* self, re::Edx, void* stream) {
    InterpController_SaveBinary(self, stream);
    Virtual<void>(stream, 12, self->interpolator);   // SaveLinkID
}

// 0x825300: IsEqual
uint8_t __fastcall Single_IsEqual(NiSingleInterpController* self, re::Edx, NiSingleInterpController* other) {
    if (!InterpController_IsEqual(self, other)) {
        return 0;
    }
    return ObjectsEqual(self->interpolator, other->interpolator);
}

// 0x825350: GetViewerStrings (the interpolator's after)
void __fastcall Single_GetViewerStrings(NiSingleInterpController* self, re::Edx, NiTArray<char*>* strings) {
    InterpController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiSingleInterpController_RTTI.name));
    if (self->interpolator != nullptr) {
        Virtual<void>(self->interpolator, 9, strings);
    }
}

// ---- NiFloatInterpController

// 0x825EA0: CreatePoseInterpolator(index) (slot 35): a NiFloatInterpolator posed at the target's
// value (slot 41 writes it over a slot holding this controller)
NiInterpolator* __fastcall Float_CreatePoseInterpolator(NiSingleInterpController* self, re::Edx, uint16_t) {
    uint32_t bits = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self));
    float pose;
    static_assert(sizeof(pose) == sizeof(bits));
    std::memcpy(&pose, &bits, 4);
    Virtual<void>(self, 41, &pose);
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    return FloatInterpolator_ConstructWithPose(memory, pose);
}

// 0x825EE0: CreateBlendInterpolator(index, manager controlled, accumulate, weight threshold,
// array size) (slot 37): a NiBlendFloatInterpolator
NiInterpolator* __fastcall Float_CreateBlendInterpolator(NiSingleInterpController*, re::Edx, uint32_t,
                                                         uint32_t managerControlled, uint32_t, float weightThreshold,
                                                         uint32_t arraySize) {
    void* memory = NiNew(0x34);
    if (memory == nullptr) {
        return nullptr;
    }
    return BlendFloatInterpolator_Construct(memory, managerControlled, weightThreshold, arraySize);
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC32_INTERPCTLR_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (lone jmps: NiAlphaController's RegisterStreamables is NiSingleInterpController's; the bool /
// float / point interp controllers' LoadBinary and LinkObject are NiSingleInterpController's;
// NiInterpController's CopyMembers, RegisterStreamables, SaveBinary, IsEqual and LinkObject are
// NiTimeController's)
RE_UNHOOKED("NiAlphaController::RegisterStreamables (thunk)", 0x00822540);
RE_UNHOOKED("NiBoolInterpController::LinkObject (thunk)", 0x00824DD0);
RE_UNHOOKED("NiInterpController::CopyMembers (thunk)", 0x00824ED0);
RE_UNHOOKED("NiInterpController::RegisterStreamables (thunk)", 0x00824EE0);
RE_UNHOOKED("NiInterpController::SaveBinary (thunk)", 0x00824EF0);
RE_UNHOOKED("NiInterpController::IsEqual (thunk)", 0x00824F00);
RE_UNHOOKED("NiInterpController::LinkObject (thunk)", 0x00824F80);
RE_UNHOOKED("NiSingleInterpController::LinkObject (thunk)", 0x008252A0);
RE_UNHOOKED("NiFloatInterpController::LoadBinary (thunk)", 0x00825F10);
// (3 bytes: "return 0", folded for every such slot: NiInterpController::GetCtlrID, NiObject's
// GetBlockAllocationSize and many of the game's own)
// 0x00824EC0: [33 c0 c3 cc cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __cdecl Tiny_00824EC0() {
    return 0;
}
RE_UNHOOKED_FN("NiInterpController::GetCtlrID", 0x00824EC0, &Tiny_00824EC0);

NIC32_INTERPCTLR_REVERSED("NiAlphaController::GetTargetFloatValue", 0x00822550,
                          &ni::anim::Alpha_GetTargetFloatValue);
NIC32_INTERPCTLR_REVERSED("NiAlphaController::InterpTargetIsCorrectType", 0x00822560,
                          &ni::anim::Alpha_InterpTargetIsCorrectType);
NIC32_INTERPCTLR_REVERSED("NiAlphaController::Update", 0x008225A0, &ni::anim::Alpha_Update);
NIC32_INTERPCTLR_REVERSED("NiAlphaController::GetRTTI", 0x00822620, &ni::anim::Alpha_GetRTTI);
NIC32_INTERPCTLR_REVERSED("NiAlphaController::CreateClone", 0x00822630, &ni::anim::Alpha_CreateClone);
NIC32_INTERPCTLR_REVERSED("NiAlphaController::CreateObject", 0x00822680, &ni::anim::Alpha_CreateObject);
NIC32_INTERPCTLR_REVERSED("NiAlphaController::GetViewerStrings", 0x008226B0, &ni::anim::Alpha_GetViewerStrings);
NIC32_INTERPCTLR_REVERSED("NiBoolInterpController::CreatePoseInterpolator", 0x00824D50,
                          &ni::anim::Bool_CreatePoseInterpolator);
NIC32_INTERPCTLR_REVERSED("NiBoolInterpController::CreateBlendInterpolator", 0x00824D90,
                          &ni::anim::Bool_CreateBlendInterpolator);
NIC32_INTERPCTLR_REVERSED("NiBoolInterpController::InterpolatorIsCorrectType", 0x00824DC0,
                          &ni::anim::Bool_InterpolatorIsCorrectType);
NIC32_INTERPCTLR_REVERSED("NiBoolInterpController::NiBoolInterpController", 0x00824DE0,
                          &ni::anim::BoolInterp_Construct);
NIC32_INTERPCTLR_REVERSED("NiBoolInterpController::GetRTTI", 0x00824E00, &ni::anim::BoolInterp_GetRTTI);
NIC32_INTERPCTLR_REVERSED("NiBoolInterpController::SynchronizePoseInterpolator", 0x00824E10,
                          &ni::anim::Bool_SynchronizePoseInterpolator);
NIC32_INTERPCTLR_REVERSED("NiBoolInterpController::GetViewerStrings", 0x00824E70,
                          &ni::anim::BoolInterp_GetViewerStrings);
NIC32_INTERPCTLR_REVERSED("NiInterpController::NiInterpController", 0x00824F10, &ni::anim::Interp_Construct);
NIC32_INTERPCTLR_REVERSED("NiInterpController::GetRTTI", 0x00824F30, &ni::anim::Interp_GetRTTI);
NIC32_INTERPCTLR_REVERSED("NiInterpController::DeletingDestructor", 0x00824F40,
                          &ni::anim::Interp_DeletingDestructor);
NIC32_INTERPCTLR_REVERSED("NiInterpController::ResetTimeExtrema", 0x00824F60, &ni::anim::Interp_ResetTimeExtrema);
NIC32_INTERPCTLR_REVERSED("NiInterpController::LoadBinary", 0x00824F90, &ni::anim::Interp_LoadBinary);
NIC32_INTERPCTLR_REVERSED("NiInterpController::GetViewerStrings", 0x00825000, &ni::anim::Interp_GetViewerStrings);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::GetInterpolatorIndex", 0x008250A0,
                          &ni::anim::Single_GetInterpolatorIndex);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::TargetIsRequiredType", 0x008250B0,
                          &ni::anim::Single_TargetIsRequiredType);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::NiSingleInterpController", 0x008250C0,
                          &ni::anim::Single_Construct);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::GetRTTI", 0x008250E0, &ni::anim::Single_GetRTTI);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::GetInterpolator", 0x008250F0,
                          &ni::anim::Single_GetInterpolator);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::SetInterpolator", 0x00825100,
                          &ni::anim::Single_SetInterpolator);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::GuaranteeTimeRange", 0x00825150,
                          &ni::anim::Single_GuaranteeTimeRange);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::ResetTimeExtrema", 0x00825180,
                          &ni::anim::Single_ResetTimeExtrema);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::CopyMembers", 0x008251B0, &ni::anim::Single_CopyMembers);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::ProcessClone", 0x00825210, &ni::anim::Single_ProcessClone);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::LoadBinary", 0x00825240, &ni::anim::Single_LoadBinary);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::RegisterStreamables", 0x008252B0,
                          &ni::anim::Single_RegisterStreamables);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::SaveBinary", 0x008252E0, &ni::anim::Single_SaveBinary);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::IsEqual", 0x00825300, &ni::anim::Single_IsEqual);
NIC32_INTERPCTLR_REVERSED("NiSingleInterpController::GetViewerStrings", 0x00825350,
                          &ni::anim::Single_GetViewerStrings);
NIC32_INTERPCTLR_REVERSED("NiFloatInterpController::CreatePoseInterpolator", 0x00825EA0,
                          &ni::anim::Float_CreatePoseInterpolator);
NIC32_INTERPCTLR_REVERSED("NiFloatInterpController::CreateBlendInterpolator", 0x00825EE0,
                          &ni::anim::Float_CreateBlendInterpolator);
