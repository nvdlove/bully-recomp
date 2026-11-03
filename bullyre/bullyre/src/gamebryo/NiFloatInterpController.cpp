// The small controller classes at the end of the animation library (src/gamebryo/NiInterpolators.h):
// NiFloatInterpController's own slots (vtable 0x9682F8, 0x38: a NiSingleInterpController driving a
// float), NiPoint3InterpController (vtable 0x968518, 0x38: one driving a point), NiFloatController's
// streaming and cloning (vtable 0x965EC8, 0x3C: the old controller with its own NiFloatData,
// NiRollController's base: src/gamebryo/NiRollController.cpp) and NiTextKey's helpers (a time and
// a NiFixedString: NiTextKeyExtraData's keys).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiAnimControllers.h"
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
GAME_VAR(RTTI, NiFloatInterpController_RTTI, 0x020E573C);
GAME_VAR(RTTI, NiPoint3InterpController_RTTI, 0x020E57D8);
GAME_VAR(RTTI, NiFloatController_RTTI, 0x020E5770);
GAME_VAR(uint32_t, ms_NiFloatInterpolatorClassIndex, 0x020E4FD4);    // (the RTTI's +8)
GAME_VAR(uint32_t, ms_NiPoint3InterpolatorClassIndex, 0x020E4CB4);

constexpr uintptr_t NiFloatInterpController_vtable = 0x009682F8;
constexpr uintptr_t NiPoint3InterpController_vtable = 0x00968518;
constexpr uintptr_t NiFloatController_vtable = 0x00965EC8;

// NiTimeController's
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
GAME_FN(void, __thiscall, TimeController_ProcessClone, 0x0077DEE0, void* self, void* cloning);
GAME_FN(void, __thiscall, TimeController_LinkObject, 0x0077DF10, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TimeController_RegisterStreamables, 0x0077DF70, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_SaveBinary, 0x0077E2F0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TimeController_IsEqual, 0x0077DB90, void* self, void* other);
GAME_FN(void, __thiscall, TimeController_CopyMembers, 0x0077E100, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, TimeController_LoadBinary, 0x0077E1F0, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_GetViewerStrings, 0x0077E550, void* self, NiTArray<char*>* strings);
// NiSingleInterpController's
GAME_FN(void, __thiscall, SingleInterp_Construct, 0x008250C0, void* self);
GAME_FN(void, __thiscall, SingleInterp_GetViewerStrings, 0x00825350, void* self, NiTArray<char*>* strings);
// the interpolators these pose
GAME_FN(void, __thiscall, FloatInterpolator_SetPoseValue, 0x00812420, void* self, float pose);
// (the NiPoint3 taken by value: its three floats, the same bytes)
GAME_FN(NiInterpolator*, __thiscall, Point3Interpolator_ConstructWithPose, 0x008085A0, void* self, float x, float y,
        float z);
GAME_FN(void, __thiscall, Point3Interpolator_SetPoseValue_x64, 0x00808680, void* self, re::Float3);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Point3Interpolator_SetPoseValue(void* a0, float a1, float a2, float a3) {
    return Point3Interpolator_SetPoseValue_x64(a0, re::Float3{a1, a2, a3});
}

// ---- NiFloatInterpController

// 0x825F30: NiFloatInterpController()
NiSingleInterpController* __fastcall FloatInterp_Construct(NiSingleInterpController* self, re::Edx) {
    SingleInterp_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiFloatInterpController_vtable);
    return self;
}

// 0x825F50
const RTTI* __fastcall FloatInterp_GetRTTI(NiSingleInterpController*, re::Edx) {
    return &NiFloatInterpController_RTTI;
}

// 0x825F60: the deleting destructor (NiFloatInterpController's and NiPoint3InterpController's:
// ~NiSingleInterpController inlined, the interpolator let go)
NiSingleInterpController* __fastcall SingleInterp_DeletingDestructor(NiSingleInterpController* self, re::Edx,
                                                                     uint32_t flags) {
    ReleaseRef(self->interpolator);
    TimeController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x38);
    }
    return self;
}

// 0x825FB0: SynchronizePoseInterpolator(interp, index) (slot 36): the float interpolator (a
// NiFloatInterpolator: anything else is null here, called through all the same) posed at the
// target's value (slot 41 writes it over the interpolator argument's slot)
void __fastcall FloatInterp_SynchronizePoseInterpolator(NiSingleInterpController* self, re::Edx,
                                                        NiInterpolator* interpolator, uint16_t) {
    NiInterpolator* floatInterp = nullptr;
    if (interpolator != nullptr && IsKindOf(interpolator, ms_NiFloatInterpolatorClassIndex)) {
        floatInterp = interpolator;
    }
    uint32_t bits = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpolator));
    float pose;
    std::memcpy(&pose, &bits, 4);
    Virtual<void>(self, 41, &pose);
    FloatInterpolator_SetPoseValue(floatInterp, pose);
}

// 0x826010: GetViewerStrings
void __fastcall FloatInterp_GetViewerStrings(NiSingleInterpController* self, re::Edx, NiTArray<char*>* strings) {
    SingleInterp_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiFloatInterpController_RTTI.name));
}

// ---- NiPoint3InterpController

// 0x827DB0: InterpolatorIsCorrectType(interp, index) (slot 39): one giving a point (slot 27)
uint8_t __fastcall Point3Interp_InterpolatorIsCorrectType(NiSingleInterpController*, re::Edx,
                                                          NiInterpolator* interpolator, uint16_t) {
    return Virtual<uint8_t>(interpolator, 27);
}

// 0x827DF0: CreatePoseInterpolator(index) (slot 35): a NiPoint3Interpolator posed at the target's
// point (slot 41 fills it)
NiInterpolator* __fastcall Point3Interp_CreatePoseInterpolator(NiSingleInterpController* self, re::Edx, uint16_t) {
    NiPoint3 pose;   // (only what slot 41 writes)
    Virtual<void>(self, 41, &pose);
    void* memory = NiNew(0x20);
    if (memory == nullptr) {
        return nullptr;
    }
    return Point3Interpolator_ConstructWithPose(memory, pose.x, pose.y, pose.z);
}

// 0x827E40: NiPoint3InterpController()
NiSingleInterpController* __fastcall Point3Interp_Construct(NiSingleInterpController* self, re::Edx) {
    SingleInterp_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiPoint3InterpController_vtable);
    return self;
}

// 0x827E60
const RTTI* __fastcall Point3Interp_GetRTTI(NiSingleInterpController*, re::Edx) {
    return &NiPoint3InterpController_RTTI;
}

// 0x827E70: SynchronizePoseInterpolator(interp, index) (slot 36): the point interpolator (a
// NiPoint3Interpolator, else null and called all the same) posed at the target's point
void __fastcall Point3Interp_SynchronizePoseInterpolator(NiSingleInterpController* self, re::Edx,
                                                         NiInterpolator* interpolator, uint16_t) {
    NiInterpolator* pointInterp = nullptr;
    if (interpolator != nullptr && IsKindOf(interpolator, ms_NiPoint3InterpolatorClassIndex)) {
        pointInterp = interpolator;
    }
    NiPoint3 pose;   // (only what slot 41 writes)
    Virtual<void>(self, 41, &pose);
    Point3Interpolator_SetPoseValue(pointInterp, pose.x, pose.y, pose.z);
}

// 0x827EF0: GetViewerStrings
void __fastcall Point3Interp_GetViewerStrings(NiSingleInterpController* self, re::Edx, NiTArray<char*>* strings) {
    SingleInterp_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiPoint3InterpController_RTTI.name));
}

// ---- NiTextKey

// 0x826060: RegisterStreamables(stream): the text registered with the stream (its slot 8,
// RegisterFixedString)
uint8_t __fastcall TextKey_RegisterStreamables(NiTextKey* self, re::Edx, void* stream) {
    return Virtual<uint8_t>(stream, 8, &self->text);
}

// 0x826080: ~NiTextKey (the text let go)
void __fastcall TextKey_Destruct(NiTextKey* self, re::Edx) {
    scene::FixedStringRelease(&self->text);
}

// 0x8260C0: SetText(text)
void __fastcall TextKey_SetText(NiTextKey* self, re::Edx, const re::Ptr32<const char>* text) {
    scene::FixedString_Assign(&self->text, text);
}

// 0x8260D0: LoadBinary: the time, then the text (inline before 20.1.0.1, by index from it)
void __fastcall TextKey_LoadBinary(NiTextKey* self, re::Edx, void* stream) {
    scene::LoadBytes(stream, &self->time, 4);
    if (scene::StreamVersion(stream) < 0x14010001) {
        scene::Stream_LoadInlineFixedString(stream, &self->text);
        return;
    }
    scene::Stream_LoadFixedString(stream, &self->text);
}

// 0x826130: SaveBinary
void __fastcall TextKey_SaveBinary(NiTextKey* self, re::Edx, void* stream) {
    scene::SaveBytes(stream, &self->time, 4);
    scene::Stream_SaveFixedString(stream, &self->text);
}

// ---- NiFloatController

// 0x826170: LoadBinary (the data's link)
void __fastcall FloatCtlr_LoadBinary(NiFloatController* self, re::Edx, void* stream) {
    TimeController_LoadBinary(self, stream);
    scene::Stream_ReadLinkID(stream);
}

// 0x826190: ~NiFloatController (the data let go, then the member pointer's own release of what
// is left: nothing)
void __fastcall FloatCtlr_Destruct(NiFloatController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiFloatController_vtable);
    if (self->data != nullptr) {
        ReleaseRef(self->data);
        self->data = nullptr;
    }
    ReleaseRef(self->data);
    TimeController_Destruct(self);
}

// 0x8261F0: GetAnim(numKeys&, type&, size&): the data's keys (none without data)
NiAnimationKey* __fastcall FloatCtlr_GetAnim(NiFloatController* self, re::Edx, uint32_t* numKeys, KeyType* type,
                                             uint8_t* size) {
    auto* data = re::PtrCast<NiKeyChannelData*>(self->data);
    if (data != nullptr) {
        *numKeys = data->numKeys;
        *type = data->type;
        *size = data->size;
        return data->keys;
    }
    *numKeys = 0;
    *reinterpret_cast<uint32_t*>(type) = 0;
    *size = 0;
    return nullptr;
}

// 0x826240: ProcessClone (slot 11; NiRollController's too): the data's as well
void __fastcall FloatCtlr_ProcessClone(NiFloatController* self, re::Edx, void* cloning) {
    TimeController_ProcessClone(self, cloning);
    if (self->data != nullptr) {
        Virtual<void>(self->data, 11, cloning);
    }
}

// 0x826270: LinkObject (the data resolved)
void __fastcall FloatCtlr_LinkObject(NiFloatController* self, re::Edx, void* stream) {
    TimeController_LinkObject(self, stream);
    auto* data = static_cast<NiObject*>(scene::Stream_ResolveLinkID(stream));
    AssignReleasingFirst(self->data, data);
}

// 0x8262D0: RegisterStreamables (the data's, not checked for null)
uint8_t __fastcall FloatCtlr_RegisterStreamables(NiFloatController* self, re::Edx, void* stream) {
    if (!TimeController_RegisterStreamables(self, stream)) {
        return 0;
    }
    Virtual<uint8_t>(self->data, 6, stream);
    return 1;
}

// 0x826300: SaveBinary (the data's link)
void __fastcall FloatCtlr_SaveBinary(NiFloatController* self, re::Edx, void* stream) {
    TimeController_SaveBinary(self, stream);
    Virtual<void>(stream, 12, self->data);   // SaveLinkID
}

// 0x826320: IsEqual (the data compared, not checked for null)
uint8_t __fastcall FloatCtlr_IsEqual(NiFloatController* self, re::Edx, NiFloatController* other) {
    if (!TimeController_IsEqual(self, other)) {
        return 0;
    }
    return Virtual<uint8_t>(self->data, 8, other->data) != 0 ? 1 : 0;
}

// 0x826350: CopyMembers (the data cloned through the cloning: its slot 3)
void __fastcall FloatCtlr_CopyMembers(NiFloatController* self, re::Edx, NiFloatController* dest, void* cloning) {
    TimeController_CopyMembers(self, dest, cloning);
    if (self->data != nullptr) {
        auto* data = Virtual<NiObject*>(self->data, 3, cloning);
        AssignReleasingFirst(dest->data, data);
    }
}

// 0x8263B0: GetViewerStrings
void __fastcall FloatCtlr_GetViewerStrings(NiFloatController* self, re::Edx, NiTArray<char*>* strings) {
    TimeController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiFloatController_RTTI.name));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define C33_FLOATCTLR_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// lone jmps: NiFloatInterpController's RegisterStreamables to NiSingleInterpController's
// (0x8252B0); NiSingleInterpController's CopyMembers, SaveBinary and IsEqual (0x8251B0, 0x8252E0,
// 0x825300) as the interp controllers' slots
RE_UNHOOKED("NiFloatInterpController::RegisterStreamables (thunk)", 0x00825F20);
RE_UNHOOKED("NiSingleInterpController::CopyMembers (thunk 0x827DC0)", 0x00827DC0);
RE_UNHOOKED("NiSingleInterpController::SaveBinary (thunk 0x827DD0)", 0x00827DD0);
RE_UNHOOKED("NiSingleInterpController::IsEqual (thunk 0x827DE0)", 0x00827DE0);

C33_FLOATCTLR_REVERSED("NiFloatInterpController::NiFloatInterpController", 0x00825F30,
                       &ni::anim::FloatInterp_Construct);
C33_FLOATCTLR_REVERSED("NiFloatInterpController::GetRTTI", 0x00825F50, &ni::anim::FloatInterp_GetRTTI);
C33_FLOATCTLR_REVERSED("NiFloatInterpController::DeletingDestructor", 0x00825F60,
                       &ni::anim::SingleInterp_DeletingDestructor);
C33_FLOATCTLR_REVERSED("NiFloatInterpController::SynchronizePoseInterpolator", 0x00825FB0,
                       &ni::anim::FloatInterp_SynchronizePoseInterpolator);
C33_FLOATCTLR_REVERSED("NiFloatInterpController::GetViewerStrings", 0x00826010,
                       &ni::anim::FloatInterp_GetViewerStrings);
C33_FLOATCTLR_REVERSED("NiTextKey::RegisterStreamables", 0x00826060, &ni::anim::TextKey_RegisterStreamables);
C33_FLOATCTLR_REVERSED("NiTextKey::~NiTextKey", 0x00826080, &ni::anim::TextKey_Destruct);
C33_FLOATCTLR_REVERSED("NiTextKey::SetText", 0x008260C0, &ni::anim::TextKey_SetText);
C33_FLOATCTLR_REVERSED("NiTextKey::LoadBinary", 0x008260D0, &ni::anim::TextKey_LoadBinary);
C33_FLOATCTLR_REVERSED("NiTextKey::SaveBinary", 0x00826130, &ni::anim::TextKey_SaveBinary);
C33_FLOATCTLR_REVERSED("NiFloatController::LoadBinary", 0x00826170, &ni::anim::FloatCtlr_LoadBinary);
C33_FLOATCTLR_REVERSED("NiFloatController::~NiFloatController", 0x00826190, &ni::anim::FloatCtlr_Destruct);
C33_FLOATCTLR_REVERSED("NiFloatController::GetAnim", 0x008261F0, &ni::anim::FloatCtlr_GetAnim);
C33_FLOATCTLR_REVERSED("NiFloatController::ProcessClone", 0x00826240, &ni::anim::FloatCtlr_ProcessClone);
C33_FLOATCTLR_REVERSED("NiFloatController::LinkObject", 0x00826270, &ni::anim::FloatCtlr_LinkObject);
C33_FLOATCTLR_REVERSED("NiFloatController::RegisterStreamables", 0x008262D0,
                       &ni::anim::FloatCtlr_RegisterStreamables);
C33_FLOATCTLR_REVERSED("NiFloatController::SaveBinary", 0x00826300, &ni::anim::FloatCtlr_SaveBinary);
C33_FLOATCTLR_REVERSED("NiFloatController::IsEqual", 0x00826320, &ni::anim::FloatCtlr_IsEqual);
C33_FLOATCTLR_REVERSED("NiFloatController::CopyMembers", 0x00826350, &ni::anim::FloatCtlr_CopyMembers);
C33_FLOATCTLR_REVERSED("NiFloatController::GetViewerStrings", 0x008263B0, &ni::anim::FloatCtlr_GetViewerStrings);
C33_FLOATCTLR_REVERSED("NiPoint3InterpController::InterpolatorIsCorrectType", 0x00827DB0,
                       &ni::anim::Point3Interp_InterpolatorIsCorrectType);
C33_FLOATCTLR_REVERSED("NiPoint3InterpController::CreatePoseInterpolator", 0x00827DF0,
                       &ni::anim::Point3Interp_CreatePoseInterpolator);
C33_FLOATCTLR_REVERSED("NiPoint3InterpController::NiPoint3InterpController", 0x00827E40,
                       &ni::anim::Point3Interp_Construct);
C33_FLOATCTLR_REVERSED("NiPoint3InterpController::GetRTTI", 0x00827E60, &ni::anim::Point3Interp_GetRTTI);
C33_FLOATCTLR_REVERSED("NiPoint3InterpController::SynchronizePoseInterpolator", 0x00827E70,
                       &ni::anim::Point3Interp_SynchronizePoseInterpolator);
C33_FLOATCTLR_REVERSED("NiPoint3InterpController::GetViewerStrings", 0x00827EF0,
                       &ni::anim::Point3Interp_GetViewerStrings);
