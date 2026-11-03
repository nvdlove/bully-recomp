// The extra data controllers of this chunk (src/gamebryo/NiAnimControllers.h): an interpolated
// value written into the target's extra data of a name. NiFloatsExtraDataPoint3Controller
// (vtable 0x966990, 0x48: three floats from an index of a NiFloatsExtraData, from a point),
// NiFloatsExtraDataController (vtable 0x966A58, 0x48: one of them, from a float),
// NiFloatExtraDataController (vtable 0x966BD0, 0x40: a NiFloatExtraData's value) and
// NiColorExtraDataController (vtable 0x966ED0, 0x40: a NiColorExtraData's colour). Their Update,
// pose interpolators, streaming, cloning, comparison, IDs and viewer strings (NiExtraDataController's
// own are the next chunks'); and the colour controller's blend interpolator and type check.
//
// An Update's float interpolator writes its value over the time argument's slot, as the game's
// code passes it (here: a local starting from the time's bits).
//
// Main-thread float maths at the game's 24-bit x87 precision.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiAnimControllers.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::LoadBytes;
using scene::SaveBytes;
using scene::Stream_ReadLinkID;
using scene::Stream_ResolveLinkID;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiFloatsExtraDataPoint3Controller_RTTI, 0x020E4F64);
GAME_VAR(RTTI, NiFloatsExtraDataController_RTTI, 0x020E4F98);
GAME_VAR(RTTI, NiFloatExtraDataController_RTTI, 0x020E5000);
GAME_VAR(RTTI, NiColorExtraDataController_RTTI, 0x020E5138);
GAME_VAR(uint32_t, kNiPoint3InterpolatorClassIndex, 0x020E4CB4);
GAME_VAR(uint32_t, kNiFloatInterpolatorClassIndex, 0x020E4FD4);
GAME_VAR(uint32_t, kNiColorInterpolatorClassIndex, 0x020E510C);

constexpr uintptr_t NiFloatsExtraDataPoint3Controller_vtable = 0x00966990;
constexpr uintptr_t NiFloatsExtraDataController_vtable = 0x00966A58;
constexpr uintptr_t NiFloatExtraDataController_vtable = 0x00966BD0;
constexpr uintptr_t NiColorExtraDataController_vtable = 0x00966ED0;
constexpr uint32_t kVersion_10_1_0_104 = 0x0A010068;

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(char*, __cdecl, GetViewerStringInt, 0x007856A0, const char* prefix, int32_t value);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(uint8_t, __thiscall, TimeController_DontDoUpdate, 0x0077D890, void* self, float time);
// NiFloatsExtraData's and NiColorExtraData's accessors (a value past the size: 0, not set)
GAME_FN(float, __thiscall, FloatsExtraData_GetValue, 0x0076C9D0, NiObject* self, uint32_t index);
GAME_FN(uint8_t, __thiscall, FloatsExtraData_SetValue, 0x0076C9B0, NiObject* self, uint32_t index, float value);
// (the colour, point and pose by value: their floats)
GAME_FN(void, __thiscall, ColorExtraData_SetValue, 0x007691F0, NiObject* self, float r, float g, float b, float a);
// NiExtraDataController's and NiSingleInterpController's (the next chunks')
GAME_FN(void, __thiscall, ExtraDataController_Construct, 0x00828090, void* self);
GAME_FN(void, __thiscall, ExtraDataController_Destruct, 0x008280C0, void* self);
GAME_FN(void, __thiscall, ExtraDataController_LoadBinary, 0x00827F90, void* self, void* stream);
GAME_FN(void, __thiscall, ExtraDataController_SaveBinary, 0x00827F70, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, ExtraDataController_IsEqual, 0x00827FD0, void* self, void* other);
GAME_FN(void, __thiscall, ExtraDataController_CopyMembers, 0x00828220, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, ExtraDataController_GetViewerStrings, 0x00828300, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, SingleInterp_LinkObject, 0x00824DD0, void* self, void* stream);
// the interpolators made (NiPoint3Interpolator: src/gamebryo/NiKeyInterpolators.cpp; the float
// and colour ones: this chunk's; the blend colour one: the next chunk's)
GAME_FN(NiInterpolator*, __thiscall, Point3Interpolator_ConstructPosed, 0x008085A0, void* self, float x, float y,
        float z);
GAME_FN(void, __thiscall, Point3Interpolator_SetPoseValue_x64, 0x00808680, void* self, re::Float3);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Point3Interpolator_SetPoseValue(void* a0, float a1, float a2, float a3) {
    return Point3Interpolator_SetPoseValue_x64(a0, re::Float3{a1, a2, a3});
}
GAME_FN(NiInterpolator*, __thiscall, FloatInterpolator_ConstructPosed, 0x008122B0, void* self, float value);
GAME_FN(void, __thiscall, FloatInterpolator_SetPoseValue, 0x00812420, void* self, float value);
GAME_FN(NiInterpolator*, __thiscall, ColorInterpolator_ConstructWithData, 0x00818F50, void* self, NiObject* data);
GAME_FN(NiInterpolator*, __thiscall, ColorInterpolator_ConstructPosed, 0x00818FC0, void* self, float r, float g,
        float b, float a);
GAME_FN(void, __thiscall, ColorInterpolator_SetPoseValue_x64, 0x00819040, void* self, re::Floats<4>);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void ColorInterpolator_SetPoseValue(void* a0, float a1, float a2, float a3, float a4) {
    return ColorInterpolator_SetPoseValue_x64(a0, re::Floats<4>{{a1, a2, a3, a4}});
}
GAME_FN(NiInterpolator*, __thiscall, BlendColorInterpolator_Construct, 0x00820610, void* self, int managerControlled,
        float weightThreshold, int arraySize);

// the Update gate the interpolator controllers share: manager controlled: the scaled time
// invalid (the manager sets it); else nothing when DontDoUpdate holds, unless the interpolator
// always updates (slot 36)
inline bool PassesGate(NiSingleInterpController* self, float time) {
    if (((uint8_t(self->flags) >> 5) & 1) != 0) {
        self->scaledTime = kInvalidFloat;
        return true;
    }
    if (!TimeController_DontDoUpdate(self, time)) {
        return true;
    }
    if (self->interpolator == nullptr) {
        return false;
    }
    return Virtual<uint8_t>(self->interpolator, 36) != 0;
}

// an interpolator of a class (by its class index), else null
inline NiInterpolator* KindOrNull(NiInterpolator* interpolator, uint32_t classIndex) {
    if (interpolator == nullptr) {
        return nullptr;
    }
    return IsKindOf(interpolator, classIndex) ? interpolator : nullptr;
}

inline NiPoint3 FloatsPoint(const NiFloatsExtraDataController* self) {
    NiObject* data = self->extraData;
    NiPoint3 point;
    point.x = FloatsExtraData_GetValue(data, uint32_t(self->floatsExtraDataIndex));
    point.y = FloatsExtraData_GetValue(data, uint32_t(self->floatsExtraDataIndex + 1));
    point.z = FloatsExtraData_GetValue(data, uint32_t(self->floatsExtraDataIndex + 2));
    return point;
}

inline NiColorA ExtraColor(const NiExtraDataController* self) {
    return *reinterpret_cast<const NiColorA*>(re::PtrCast<const uint8_t*>(self->extraData) + 0xC);
}

// a floats controller as the constructor makes it (CreateObject and CreateClone inline it)
inline void ConstructFloats(NiFloatsExtraDataController* self, uintptr_t vtable) {
    ExtraDataController_Construct(self);
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(vtable);
    self->floatsExtraDataIndex = -1;
    self->ctlrID = sys::kNullFixedString;
}

// a floats controller's clone (a failed allocation still copied onto null; the index then set
// through null, as the game has it)
inline NiFloatsExtraDataController* CloneFloats(NiFloatsExtraDataController* self, void* cloning, uintptr_t vtable) {
    NiFloatsExtraDataController* clone = static_cast<NiFloatsExtraDataController*>(NiNew(0x48));
    if (clone != nullptr) {
        ConstructFloats(clone, vtable);
    }
    ExtraDataController_CopyMembers(self, clone, cloning);
    clone->floatsExtraDataIndex = self->floatsExtraDataIndex;
    return clone;
}

inline NiObject* CreateFloats(uintptr_t vtable) {
    void* memory = NiNew(0x48);
    if (memory == nullptr) {
        return nullptr;
    }
    ConstructFloats(static_cast<NiFloatsExtraDataController*>(memory), vtable);
    return static_cast<NiObject*>(memory);
}

// a float or colour controller's clone (as above)
inline NiExtraDataController* CloneSimple(NiExtraDataController* self, void* cloning, uintptr_t vtable) {
    void* memory = NiNew(0x40);
    if (memory == nullptr) {
        ExtraDataController_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    ExtraDataController_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(vtable);
    ExtraDataController_CopyMembers(self, memory, cloning);
    return static_cast<NiExtraDataController*>(memory);
}

inline NiObject* CreateSimple(uintptr_t vtable) {
    void* memory = NiNew(0x40);
    if (memory == nullptr) {
        return nullptr;
    }
    ExtraDataController_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(vtable);
    return static_cast<NiObject*>(memory);
}

// the floats controllers' deleting destructor (the ID let go)
inline NiFloatsExtraDataController* DeleteFloats(NiFloatsExtraDataController* self, uint32_t flags, uintptr_t vtable) {
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(vtable);
    sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->ctlrID));
    ExtraDataController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x48);
    }
    return self;
}

inline void FloatsViewerStrings(NiFloatsExtraDataController* self, NiTArray<char*>* strings, const RTTI& rtti) {
    ExtraDataController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(rtti.name));
    AddViewerString(strings, GetViewerStringInt("m_iFloatsExtraDataIndex", self->floatsExtraDataIndex));
}

// ---- NiFloatsExtraDataController and NiFloatsExtraDataPoint3Controller (shared slots)

// 0x811AB0: IsEqual (the index too)
uint8_t __fastcall Floats_IsEqual(NiFloatsExtraDataController* self, re::Edx, NiFloatsExtraDataController* other) {
    if (!ExtraDataController_IsEqual(self, other)) {
        return 0;
    }
    return self->floatsExtraDataIndex == other->floatsExtraDataIndex ? 1 : 0;
}

// 0x8120A0: SaveBinary
void __fastcall Floats_SaveBinary(NiFloatsExtraDataController* self, re::Edx, void* stream) {
    ExtraDataController_SaveBinary(self, stream);
    SaveBytes(stream, &self->floatsExtraDataIndex, 4);
}

// 0x812140: GetCtlrID (slot 34): "name[index]", made the first time
const char* __fastcall Floats_GetCtlrID(NiFloatsExtraDataController* self, re::Edx) {
    if (self->ctlrID != sys::kNullFixedString) {
        return self->ctlrID;
    }
    FixedString_AssignText(&self->ctlrID, nullptr);
    const char* name = self->extraDataName;
    if (name != nullptr) {
        size_t size = std::strlen(name) + 0xF;   // (inlined)
        char* buffer = static_cast<char*>(NiMalloc(uint32_t(size)));
        NiSprintf(buffer, size, "%s[%d]", name, self->floatsExtraDataIndex);
        FixedString_AssignText(&self->ctlrID, buffer);
        NiFree(buffer);
    }
    return self->ctlrID;
}

// ---- NiFloatsExtraDataPoint3Controller

// 0x811AE0: CreatePoseInterpolator(index) (slot 35): a point interpolator posed at the three floats
NiInterpolator* __fastcall P3_CreatePoseInterpolator(NiFloatsExtraDataController* self, re::Edx, uint16_t) {
    NiPoint3 point = FloatsPoint(self);
    void* memory = NiNew(0x20);
    if (memory == nullptr) {
        return nullptr;
    }
    return Point3Interpolator_ConstructPosed(memory, point.x, point.y, point.z);
}

const RTTI* __fastcall P3_GetRTTI(NiFloatsExtraDataController*, re::Edx) {
    return &NiFloatsExtraDataPoint3Controller_RTTI;
}

// 0x811B70: SynchronizePoseInterpolator(interpolator, index) (slot 36): the point interpolator
// posed at the three floats (another kind posed through null, as the game has it)
void __fastcall P3_SynchronizePoseInterpolator(NiFloatsExtraDataController* self, re::Edx, NiInterpolator* interpolator,
                                               uint16_t) {
    NiInterpolator* point = KindOrNull(interpolator, kNiPoint3InterpolatorClassIndex);
    NiPoint3 value = FloatsPoint(self);
    Point3Interpolator_SetPoseValue(point, value.x, value.y, value.z);
}

// 0x811C10: Update(time) (slot 20): the point into the three floats
void __fastcall P3_Update(NiFloatsExtraDataController* self, re::Edx, float time) {
    if (!PassesGate(self, time) || self->interpolator == nullptr) {
        return;
    }
    NiPoint3 value;   // (not set first, as the game has it)
    if (!Virtual<uint8_t>(self->interpolator, 20, self->scaledTime, self->target, &value)) {
        return;
    }
    NiObject* data = self->extraData;
    if (data == nullptr) {
        return;
    }
    FloatsExtraData_SetValue(data, uint32_t(self->floatsExtraDataIndex), value.x);
    FloatsExtraData_SetValue(data, uint32_t(self->floatsExtraDataIndex + 1), value.y);
    FloatsExtraData_SetValue(data, uint32_t(self->floatsExtraDataIndex + 2), value.z);
}

// 0x811CD0: CreateClone
NiFloatsExtraDataController* __fastcall P3_CreateClone(NiFloatsExtraDataController* self, re::Edx, void* cloning) {
    return CloneFloats(self, cloning, NiFloatsExtraDataPoint3Controller_vtable);
}

// 0x811D20: CreateObject
NiObject* __cdecl P3_CreateObject() {
    return CreateFloats(NiFloatsExtraDataPoint3Controller_vtable);
}

// 0x811D60: LoadBinary (the index)
void __fastcall P3_LoadBinary(NiFloatsExtraDataController* self, re::Edx, void* stream) {
    ExtraDataController_LoadBinary(self, stream);
    LoadBytes(stream, &self->floatsExtraDataIndex, 4);
}

// 0x811DA0: the deleting destructor
NiFloatsExtraDataController* __fastcall P3_DeletingDestructor(NiFloatsExtraDataController* self, re::Edx,
                                                             uint32_t flags) {
    return DeleteFloats(self, flags, NiFloatsExtraDataPoint3Controller_vtable);
}

// 0x811E00: GetViewerStrings
void __fastcall P3_GetViewerStrings(NiFloatsExtraDataController* self, re::Edx, NiTArray<char*>* strings) {
    FloatsViewerStrings(self, strings, NiFloatsExtraDataPoint3Controller_RTTI);
}

// ---- NiFloatsExtraDataController

// 0x811E90: CreatePoseInterpolator(index) (slot 35): a float interpolator posed at the float
NiInterpolator* __fastcall FS_CreatePoseInterpolator(NiFloatsExtraDataController* self, re::Edx, uint16_t) {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    float value = FloatsExtraData_GetValue(self->extraData, uint32_t(self->floatsExtraDataIndex));
    return FloatInterpolator_ConstructPosed(memory, value);
}

const RTTI* __fastcall FS_GetRTTI(NiFloatsExtraDataController*, re::Edx) {
    return &NiFloatsExtraDataController_RTTI;
}

// 0x811EE0: SynchronizePoseInterpolator(interpolator, index) (slot 36)
void __fastcall FS_SynchronizePoseInterpolator(NiFloatsExtraDataController* self, re::Edx, NiInterpolator* interpolator,
                                               uint16_t) {
    NiInterpolator* floatInterpolator = KindOrNull(interpolator, kNiFloatInterpolatorClassIndex);
    float value = FloatsExtraData_GetValue(self->extraData, uint32_t(self->floatsExtraDataIndex));
    FloatInterpolator_SetPoseValue(floatInterpolator, value);
}

// 0x811F40: Update(time) (slot 20): the float into its index
void __fastcall FS_Update(NiFloatsExtraDataController* self, re::Edx, float time) {
    if (!PassesGate(self, time) || self->interpolator == nullptr) {
        return;
    }
    float value = time;
    if (!Virtual<uint8_t>(self->interpolator, 22, self->scaledTime, self->target, &value)) {
        return;
    }
    if (self->extraData != nullptr) {
        FloatsExtraData_SetValue(self->extraData, uint32_t(self->floatsExtraDataIndex), value);
    }
}

// 0x811FC0: CreateClone
NiFloatsExtraDataController* __fastcall FS_CreateClone(NiFloatsExtraDataController* self, re::Edx, void* cloning) {
    return CloneFloats(self, cloning, NiFloatsExtraDataController_vtable);
}

// 0x812010: CreateObject
NiObject* __cdecl FS_CreateObject() {
    return CreateFloats(NiFloatsExtraDataController_vtable);
}

// 0x812050: LoadBinary (the index; an older file's data link read)
void __fastcall FS_LoadBinary(NiFloatsExtraDataController* self, re::Edx, void* stream) {
    ExtraDataController_LoadBinary(self, stream);
    LoadBytes(stream, &self->floatsExtraDataIndex, 4);
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        Stream_ReadLinkID(stream);
    }
}

// 0x8120E0: the deleting destructor
NiFloatsExtraDataController* __fastcall FS_DeletingDestructor(NiFloatsExtraDataController* self, re::Edx,
                                                             uint32_t flags) {
    return DeleteFloats(self, flags, NiFloatsExtraDataController_vtable);
}

// 0x8121C0: GetViewerStrings
void __fastcall FS_GetViewerStrings(NiFloatsExtraDataController* self, re::Edx, NiTArray<char*>* strings) {
    FloatsViewerStrings(self, strings, NiFloatsExtraDataController_RTTI);
}

// ---- NiFloatExtraDataController

// 0x812980: CreateObject
NiObject* __cdecl FE_CreateObject() {
    return CreateSimple(NiFloatExtraDataController_vtable);
}

const RTTI* __fastcall FE_GetRTTI(NiExtraDataController*, re::Edx) {
    return &NiFloatExtraDataController_RTTI;
}

// 0x8129E0: CreatePoseInterpolator(index) (slot 35): a float interpolator posed at the value
NiInterpolator* __fastcall FE_CreatePoseInterpolator(NiExtraDataController* self, re::Edx, uint16_t) {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    return FloatInterpolator_ConstructPosed(memory, static_cast<NiFloatExtraData*>(self->extraData)->value);
}

// 0x812A20: CreateClone
NiExtraDataController* __fastcall FE_CreateClone(NiExtraDataController* self, re::Edx, void* cloning) {
    return CloneSimple(self, cloning, NiFloatExtraDataController_vtable);
}

// 0x812A70: LoadBinary (NiFloatExtraDataController's and NiColorExtraDataController's): an
// older file's data link read
void __fastcall FE_LoadBinary(NiExtraDataController* self, re::Edx, void* stream) {
    ExtraDataController_LoadBinary(self, stream);
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        Stream_ReadLinkID(stream);
    }
}

// 0x812A90: SynchronizePoseInterpolator(interpolator, index) (slot 36)
void __fastcall FE_SynchronizePoseInterpolator(NiExtraDataController* self, re::Edx, NiInterpolator* interpolator,
                                               uint16_t) {
    NiInterpolator* floatInterpolator = KindOrNull(interpolator, kNiFloatInterpolatorClassIndex);
    FloatInterpolator_SetPoseValue(floatInterpolator, static_cast<NiFloatExtraData*>(self->extraData)->value);
}

// 0x812AE0: Update(time) (slot 20): the extra data's value
void __fastcall FE_Update(NiExtraDataController* self, re::Edx, float time) {
    if (!PassesGate(self, time) || self->interpolator == nullptr) {
        return;
    }
    float value = time;
    if (!Virtual<uint8_t>(self->interpolator, 22, self->scaledTime, self->target, &value)) {
        return;
    }
    if (self->extraData != nullptr) {
        static_cast<NiFloatExtraData*>(self->extraData)->value = value;
    }
}

// 0x812B60: GetViewerStrings
void __fastcall FE_GetViewerStrings(NiExtraDataController* self, re::Edx, NiTArray<char*>* strings) {
    ExtraDataController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiFloatExtraDataController_RTTI.name));
}

// ---- NiColorExtraDataController

// 0x8196A0: CreateBlendInterpolator(index, managerControlled, accumulate, weightThreshold,
// arraySize) (slot 37): a NiBlendColorInterpolator
NiInterpolator* __fastcall CE_CreateBlendInterpolator(NiExtraDataController*, re::Edx, uint16_t, int managerControlled,
                                                      int, float weightThreshold, int arraySize) {
    void* memory = NiNew(0x44);
    if (memory == nullptr) {
        return nullptr;
    }
    return BlendColorInterpolator_Construct(memory, managerControlled, weightThreshold, arraySize);
}

// 0x8196D0: InterpolatorIsCorrectType(interpolator, index) (slot 39): one that gives colours
// (slot 28)
uint8_t __fastcall CE_InterpolatorIsCorrectType(NiExtraDataController*, re::Edx, NiInterpolator* interpolator, uint16_t) {
    return Virtual<uint8_t>(interpolator, 28);
}

// 0x8196E0: CreateObject
NiObject* __cdecl CE_CreateObject() {
    return CreateSimple(NiColorExtraDataController_vtable);
}

const RTTI* __fastcall CE_GetRTTI(NiExtraDataController*, re::Edx) {
    return &NiColorExtraDataController_RTTI;
}

// 0x819720: CreatePoseInterpolator(index) (slot 35): a colour interpolator posed at the colour
NiInterpolator* __fastcall CE_CreatePoseInterpolator(NiExtraDataController* self, re::Edx, uint16_t) {
    void* memory = NiNew(0x24);
    if (memory == nullptr) {
        return nullptr;
    }
    NiColorA color = ExtraColor(self);
    return ColorInterpolator_ConstructPosed(memory, color.r, color.g, color.b, color.a);
}

// 0x819770: CreateClone
NiExtraDataController* __fastcall CE_CreateClone(NiExtraDataController* self, re::Edx, void* cloning) {
    return CloneSimple(self, cloning, NiColorExtraDataController_vtable);
}

// 0x8197C0: LinkObject: an older file's NiColorData made into a NiColorInterpolator (collapsed)
void __fastcall CE_LinkObject(NiExtraDataController* self, re::Edx, void* stream) {
    SingleInterp_LinkObject(self, stream);
    if (StreamVersion(stream) >= kVersion_10_1_0_104) {
        return;
    }
    NiObject* data = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
    void* memory = NiNew(0x24);
    NiInterpolator* made = memory != nullptr ? ColorInterpolator_ConstructWithData(memory, data) : nullptr;
    AssignReleasingFirst(self->interpolator, made);
    Virtual<void>(self->interpolator, 30);   // Collapse
}

// 0x819840: SynchronizePoseInterpolator(interpolator, index) (slot 36)
void __fastcall CE_SynchronizePoseInterpolator(NiExtraDataController* self, re::Edx, NiInterpolator* interpolator,
                                               uint16_t) {
    NiInterpolator* color = KindOrNull(interpolator, kNiColorInterpolatorClassIndex);
    NiColorA value = ExtraColor(self);
    ColorInterpolator_SetPoseValue(color, value.r, value.g, value.b, value.a);
}

// 0x8198B0: Update(time) (slot 20): the extra data's colour (from black, nothing alpha)
void __fastcall CE_Update(NiExtraDataController* self, re::Edx, float time) {
    if (!PassesGate(self, time) || self->interpolator == nullptr) {
        return;
    }
    NiColorA value = {0.0f, 0.0f, 0.0f, 0.0f};
    if (!Virtual<uint8_t>(self->interpolator, 19, self->scaledTime, self->target, &value)) {
        return;
    }
    if (self->extraData != nullptr) {
        ColorExtraData_SetValue(self->extraData, value.r, value.g, value.b, value.a);
    }
}

// 0x819960: GetViewerStrings
void __fastcall CE_GetViewerStrings(NiExtraDataController* self, re::Edx, NiTArray<char*>* strings) {
    ExtraDataController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiColorExtraDataController_RTTI.name));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_EXTRACTLR_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (lone jmp thunks into NiExtraDataController's slots, the next chunks')
RE_UNHOOKED("NiExtraDataController::RegisterStreamables thunk", 0x00811AA0);
RE_UNHOOKED("NiFloatExtraDataController::SaveBinary thunk", 0x008129B0);
RE_UNHOOKED("NiFloatExtraDataController::IsEqual thunk", 0x008129C0);

NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::IsEqual", 0x00811AB0, &ni::anim::Floats_IsEqual);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::SaveBinary", 0x008120A0, &ni::anim::Floats_SaveBinary);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::GetCtlrID", 0x00812140, &ni::anim::Floats_GetCtlrID);

NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataPoint3Controller::CreatePoseInterpolator", 0x00811AE0,
                         &ni::anim::P3_CreatePoseInterpolator);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataPoint3Controller::GetRTTI", 0x00811B60, &ni::anim::P3_GetRTTI);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataPoint3Controller::SynchronizePoseInterpolator", 0x00811B70,
                         &ni::anim::P3_SynchronizePoseInterpolator);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataPoint3Controller::Update", 0x00811C10, &ni::anim::P3_Update);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataPoint3Controller::CreateClone", 0x00811CD0, &ni::anim::P3_CreateClone);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataPoint3Controller::CreateObject", 0x00811D20, &ni::anim::P3_CreateObject);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataPoint3Controller::LoadBinary", 0x00811D60, &ni::anim::P3_LoadBinary);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataPoint3Controller::DeletingDestructor", 0x00811DA0,
                         &ni::anim::P3_DeletingDestructor);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataPoint3Controller::GetViewerStrings", 0x00811E00,
                         &ni::anim::P3_GetViewerStrings);

NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::CreatePoseInterpolator", 0x00811E90,
                         &ni::anim::FS_CreatePoseInterpolator);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::GetRTTI", 0x00811ED0, &ni::anim::FS_GetRTTI);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::SynchronizePoseInterpolator", 0x00811EE0,
                         &ni::anim::FS_SynchronizePoseInterpolator);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::Update", 0x00811F40, &ni::anim::FS_Update);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::CreateClone", 0x00811FC0, &ni::anim::FS_CreateClone);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::CreateObject", 0x00812010, &ni::anim::FS_CreateObject);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::LoadBinary", 0x00812050, &ni::anim::FS_LoadBinary);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::DeletingDestructor", 0x008120E0,
                         &ni::anim::FS_DeletingDestructor);
NIC31_EXTRACTLR_REVERSED("NiFloatsExtraDataController::GetViewerStrings", 0x008121C0, &ni::anim::FS_GetViewerStrings);

NIC31_EXTRACTLR_REVERSED("NiFloatExtraDataController::CreateObject", 0x00812980, &ni::anim::FE_CreateObject);
NIC31_EXTRACTLR_REVERSED("NiFloatExtraDataController::GetRTTI", 0x008129D0, &ni::anim::FE_GetRTTI);
NIC31_EXTRACTLR_REVERSED("NiFloatExtraDataController::CreatePoseInterpolator", 0x008129E0,
                         &ni::anim::FE_CreatePoseInterpolator);
NIC31_EXTRACTLR_REVERSED("NiFloatExtraDataController::CreateClone", 0x00812A20, &ni::anim::FE_CreateClone);
NIC31_EXTRACTLR_REVERSED("NiFloatExtraDataController::LoadBinary", 0x00812A70, &ni::anim::FE_LoadBinary);
NIC31_EXTRACTLR_REVERSED("NiFloatExtraDataController::SynchronizePoseInterpolator", 0x00812A90,
                         &ni::anim::FE_SynchronizePoseInterpolator);
NIC31_EXTRACTLR_REVERSED("NiFloatExtraDataController::Update", 0x00812AE0, &ni::anim::FE_Update);
NIC31_EXTRACTLR_REVERSED("NiFloatExtraDataController::GetViewerStrings", 0x00812B60, &ni::anim::FE_GetViewerStrings);

NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::CreateBlendInterpolator", 0x008196A0,
                         &ni::anim::CE_CreateBlendInterpolator);
NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::InterpolatorIsCorrectType", 0x008196D0,
                         &ni::anim::CE_InterpolatorIsCorrectType);
NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::CreateObject", 0x008196E0, &ni::anim::CE_CreateObject);
NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::GetRTTI", 0x00819710, &ni::anim::CE_GetRTTI);
NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::CreatePoseInterpolator", 0x00819720,
                         &ni::anim::CE_CreatePoseInterpolator);
NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::CreateClone", 0x00819770, &ni::anim::CE_CreateClone);
NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::LinkObject", 0x008197C0, &ni::anim::CE_LinkObject);
NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::SynchronizePoseInterpolator", 0x00819840,
                         &ni::anim::CE_SynchronizePoseInterpolator);
NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::Update", 0x008198B0, &ni::anim::CE_Update);
NIC31_EXTRACTLR_REVERSED("NiColorExtraDataController::GetViewerStrings", 0x00819960, &ni::anim::CE_GetViewerStrings);
