// NiBSplineInterpolator's own slots (src/gamebryo/NiBSplineData.h: vtable 0x9686B0, 0x1C bytes):
// the time range, the control point data and the basis its kinds evaluate (the kinds are the other
// chunk's, src/gamebryo/NiBSplineInterpolators.cpp). Plus the input system's semantic-to-control
// mapping (NiInputSystem, src/gamebryo/NiInput.h), which sits between them and NiApplication.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiBSplineData.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::GetViewerStringFloat;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBSplineInterpolator_RTTI, 0x020E5840);
GAME_VAR(float, kBSplineStartTime, 0x009686AC);   // FLT_MAX
GAME_VAR(float, kBSplineEndTime, 0x009009BC);     // -FLT_MAX

constexpr uintptr_t NiBSplineInterpolator_vtable = 0x009686B0;
constexpr float kMaxFloat = 3.40282347e+38f;

GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(NiObject*, __thiscall, Object_Clone, 0x007553B0, void* self, void* cloning);
GAME_FN(NiBSplineBasis3*, __thiscall, BasisData_GetDegree3Basis, 0x0077C280, NiBSplineBasisData* self);
// NiInterpolator's (and the thunks to NiObject's the code calls)
GAME_FN(void, __thiscall, Interpolator_Construct, 0x00825DA0, void* self);
GAME_FN(void, __thiscall, Interpolator_Destruct, 0x00825DC0, void* self);
GAME_FN(void, __thiscall, Interpolator_LoadBinary, 0x007F79A0, void* self, void* stream);
GAME_FN(void, __thiscall, Interpolator_SaveBinary, 0x00806A50, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Interpolator_RegisterStreamables, 0x00807310, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Interpolator_IsEqual, 0x007769C0, void* self, void* other);
GAME_FN(void, __thiscall, Interpolator_CopyMembers, 0x007F7990, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Interpolator_GetViewerStrings, 0x00825E50, void* self, NiTArray<char*>* strings);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, BSplineInterpolator_Destruct, 0x008285C0, void* self);

// 0x828390: GetActiveTimeRange(&begin, &end) (slot 31): 0 and 0 when the values passed in are
// FLT_MAX and -FLT_MAX (the game tests the arguments, not its own times), else its own times
void __fastcall BSpline_GetActiveTimeRange(NiBSplineInterpolator* self, re::Edx, float* begin, float* end) {
    if (*begin == kMaxFloat && *end == kInvalidFloat) {
        *begin = 0.0f;
        *end = 0.0f;
        return;
    }
    *begin = self->startTime;
    *end = self->endTime;
}

// 0x8283E0: the channel's control point storage in bytes (slot 43): dimension (slot 39) times
// count (slot 38), shorts when compressed (slot 42), else floats
uint32_t __fastcall BSpline_GetAllocatedSize(NiBSplineInterpolator* self, re::Edx, uint32_t channel) {
    if (Virtual<uint8_t>(self, 42)) {
        uint32_t dimension = Virtual<uint32_t>(self, 39, channel);
        uint32_t count = Virtual<uint32_t>(self, 38, channel);
        return dimension * count * 2;
    }
    uint32_t dimension = Virtual<uint32_t>(self, 39, channel);
    uint32_t count = Virtual<uint32_t>(self, 38, channel);
    return dimension * count * 4;
}

// 0x828440: GetControlPointCount(channel) (slot 38): the basis's count for a channel that has
// control points (its handle, slot 44, isn't 0xFFFF), else 0
uint32_t __fastcall BSpline_GetControlPointCount(NiBSplineInterpolator* self, re::Edx, uint32_t channel) {
    uint16_t channels = Virtual<uint16_t>(self, 37);
    if (static_cast<uint16_t>(channel) < channels && self->data != nullptr &&
        Virtual<uint32_t>(self, 44, channel) != kInvalidBSplineHandle) {
        return static_cast<uint32_t>(BasisData_GetDegree3Basis(self->basisData)->numControlPoints);
    }
    return 0;
}

// 0x828490: RegisterStreamables (the data's and the basis's)
uint8_t __fastcall BSpline_RegisterStreamables(NiBSplineInterpolator* self, re::Edx, void* stream) {
    if (!Interpolator_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->data != nullptr) {
        Virtual<uint8_t>(self->data, 6, stream);
    }
    if (self->basisData != nullptr) {
        Virtual<uint8_t>(self->basisData, 6, stream);
    }
    return 1;
}

// a pointer compared as this IsEqual has it: false when only one is there; else whether equal
inline bool DataPairDiffers(NiObject* mine, NiObject* other) {
    if (mine != nullptr) {
        if (other == nullptr) {
            return true;
        }
    } else {
        if (other != nullptr) {
            return true;
        }
        return false;
    }
    return !Virtual<uint8_t>(mine, 8, other);
}

// 0x8284D0: IsEqual. The game's is inverted: true when both times are equal; otherwise the data
// and basis compared (calls made) and false whatever they give
uint8_t __fastcall BSpline_IsEqual(NiBSplineInterpolator* self, re::Edx, NiBSplineInterpolator* other) {
    if (!Interpolator_IsEqual(self, other)) {
        return 0;
    }
    if (self->startTime == other->startTime && self->endTime == other->endTime) {
        return 1;
    }
    if (DataPairDiffers(self->data, other->data)) {
        return 0;
    }
    NiObject* mine = self->basisData;
    NiObject* theirs = other->basisData;
    if (mine != nullptr && theirs != nullptr) {
        Virtual<uint8_t>(mine, 8, theirs);
    }
    return 0;
}

// 0x828560: NiBSplineInterpolator(data, basis data) (both held)
NiBSplineInterpolator* __fastcall BSpline_Construct(NiBSplineInterpolator* self, re::Edx, NiBSplineData* data,
                                                    NiBSplineBasisData* basisData) {
    Interpolator_Construct(self);
    self->startTime = kBSplineStartTime;
    self->endTime = kBSplineEndTime;
    self->vtable = reinterpret_cast<void*>(NiBSplineInterpolator_vtable);
    self->data = data;
    AddRefImport(data);
    self->basisData = basisData;
    AddRefImport(basisData);
    return self;
}

// 0x8285B0
const RTTI* __fastcall BSpline_GetRTTI(NiBSplineInterpolator*, re::Edx) {
    return &NiBSplineInterpolator_RTTI;
}

// 0x8285C0: ~NiBSplineInterpolator
void __fastcall BSpline_Destruct(NiBSplineInterpolator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiBSplineInterpolator_vtable);
    ReleaseRef(self->basisData);
    ReleaseRef(self->data);
    Interpolator_Destruct(self);
}

// 0x828610: CopyMembers: the times, the data shared, the basis cloned through the cloning
void __fastcall BSpline_CopyMembers(NiBSplineInterpolator* self, re::Edx, NiBSplineInterpolator* dest, void* cloning) {
    Interpolator_CopyMembers(self, dest, cloning);
    dest->endTime = self->endTime;
    dest->startTime = self->startTime;
    AssignReleasingFirst(dest->data, self->data);
    if (self->basisData != nullptr) {
        auto* basis = static_cast<NiBSplineBasisData*>(Object_Clone(self->basisData, cloning));
        AssignReleasingFirst(dest->basisData, basis);
    }
}

// 0x8286C0: LoadBinary: the times, the data's and the basis's links (objects read now)
void __fastcall BSpline_LoadBinary(NiBSplineInterpolator* self, re::Edx, void* stream) {
    Interpolator_LoadBinary(self, stream);
    scene::LoadBytes(stream, &self->startTime, 4);
    scene::LoadBytes(stream, &self->endTime, 4);
    AssignReleasingFirst(self->data, static_cast<NiBSplineData*>(Stream_ReadLinkIDObject(stream)));
    AssignReleasingFirst(self->basisData, static_cast<NiBSplineBasisData*>(Stream_ReadLinkIDObject(stream)));
}

// 0x8287A0: SaveBinary
void __fastcall BSpline_SaveBinary(NiBSplineInterpolator* self, re::Edx, void* stream) {
    Interpolator_SaveBinary(self, stream);
    scene::SaveBytes(stream, &self->startTime, 4);
    scene::SaveBytes(stream, &self->endTime, 4);
    Virtual<void>(stream, 12, self->data);        // SaveLinkID
    Virtual<void>(stream, 12, self->basisData);
}

// 0x828820: the deleting destructor
NiBSplineInterpolator* __fastcall BSpline_DeletingDestructor(NiBSplineInterpolator* self, re::Edx, uint32_t flags) {
    BSplineInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// 0x828840: GetViewerStrings (the data's and the basis's after)
void __fastcall BSpline_GetViewerStrings(NiBSplineInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    Interpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBSplineInterpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringFloat("m_fStartTime", self->startTime));
    AddViewerString(strings, GetViewerStringFloat("m_fEndTime", self->endTime));
    if (self->data != nullptr) {
        Virtual<void>(self->data, 9, strings);
    }
    if (self->basisData != nullptr) {
        Virtual<void>(self->basisData, 9, strings);
    }
}

}   // namespace
}   // namespace ni::anim

// ---- the input system's semantics (NiAction's: 0x10000000 game pad, 0x20000000 keyboard,
// 0x40000000 mouse) to the device's control numbers. Gamebryo's names, a guess

namespace ni::input {
namespace {

// 0x828930: a game pad semantic: the sticks' axes 0x10000001..4 (0x10001, 0x10002, 0x20001,
// 0x20002), 0x10000100 the first button (0) and 0x10000101..F the rest (1..15); -1 otherwise
uint32_t __cdecl GamePadSemanticToDeviceControl(uint32_t semantic) {
    if (semantic <= 0x10000100) {
        if (semantic == 0x10000100) {
            return 0;
        }
        switch (semantic) {
        case 0x10000001:
            return 0x10001;
        case 0x10000002:
            return 0x10002;
        case 0x10000003:
            return 0x20001;
        case 0x10000004:
            return 0x20002;
        default:
            return 0xFFFFFFFF;
        }
    }
    if (semantic >= 0x10000101 && semantic <= 0x1000010F) {
        return semantic - 0x10000100;
    }
    return 0xFFFFFFFF;
}

// 0x828A30: a mouse semantic: the axes 0x40000001..3 (0..2), the buttons 0x40001000..7 (0..7);
// -1 otherwise
uint32_t __cdecl MouseSemanticToDeviceControl(uint32_t semantic) {
    if (semantic <= 0x40001000) {
        if (semantic == 0x40001000 || semantic == 0x40000001) {
            return 0;
        }
        if (semantic == 0x40000002) {
            return 1;
        }
        if (semantic == 0x40000003) {
            return 2;
        }
        return 0xFFFFFFFF;
    }
    if (semantic >= 0x40001001 && semantic <= 0x40001007) {
        return semantic - 0x40001000;
    }
    return 0xFFFFFFFF;
}

GAME_FN(uint32_t, __cdecl, GamePadSemanticToDeviceControl_, 0x00828930, uint32_t semantic);
GAME_FN(uint32_t, __cdecl, MouseSemanticToDeviceControl_, 0x00828A30, uint32_t semantic);

// 0x828AB0: SemanticToDeviceControl: by the semantic's kind; a keyboard one is its key (not the
// modifier ones, 0x2001xxxx)
uint32_t __cdecl SemanticToDeviceControl(uint32_t semantic) {
    if ((semantic & 0x10000000) != 0) {
        return GamePadSemanticToDeviceControl_(semantic);
    }
    if ((semantic & 0x40000000) != 0) {
        return MouseSemanticToDeviceControl_(semantic);
    }
    if ((semantic & 0x20000000) != 0 && (semantic & 0x20010000) != 0x20010000) {
        return semantic & 0xDFFFFFFF;
    }
    return 0xFFFFFFFF;
}

}   // namespace
}   // namespace ni::input

// ---- registration ---------------------------------------------------------------------

#define C33_BSPLINEBASE_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::GetActiveTimeRange", 0x00828390,
                         &ni::anim::BSpline_GetActiveTimeRange);
// (3 bytes: "return false", folded for many slots: NiBSplineInterpolator's 13 and 24..29 and 36,
// TriShape's, the game's own)
// 0x008283D0: [32 c0 c3 cc cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __cdecl Tiny_008283D0() {
    return 0;
}
RE_UNHOOKED_FN("NiObject::ReturnFalse (folded, 0x8283D0)", 0x008283D0, &Tiny_008283D0);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::GetAllocatedSize", 0x008283E0,
                         &ni::anim::BSpline_GetAllocatedSize);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::GetControlPointCount", 0x00828440,
                         &ni::anim::BSpline_GetControlPointCount);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::RegisterStreamables", 0x00828490,
                         &ni::anim::BSpline_RegisterStreamables);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::IsEqual", 0x008284D0, &ni::anim::BSpline_IsEqual);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::NiBSplineInterpolator", 0x00828560,
                         &ni::anim::BSpline_Construct);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::GetRTTI", 0x008285B0, &ni::anim::BSpline_GetRTTI);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::~NiBSplineInterpolator", 0x008285C0,
                         &ni::anim::BSpline_Destruct);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::CopyMembers", 0x00828610, &ni::anim::BSpline_CopyMembers);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::LoadBinary", 0x008286C0, &ni::anim::BSpline_LoadBinary);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::SaveBinary", 0x008287A0, &ni::anim::BSpline_SaveBinary);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::DeletingDestructor", 0x00828820,
                         &ni::anim::BSpline_DeletingDestructor);
C33_BSPLINEBASE_REVERSED("NiBSplineInterpolator::GetViewerStrings", 0x00828840,
                         &ni::anim::BSpline_GetViewerStrings);
C33_BSPLINEBASE_REVERSED("NiInputSystem::GamePadSemanticToDeviceControl", 0x00828930,
                         &ni::input::GamePadSemanticToDeviceControl);
C33_BSPLINEBASE_REVERSED("NiInputSystem::MouseSemanticToDeviceControl", 0x00828A30,
                         &ni::input::MouseSemanticToDeviceControl);
C33_BSPLINEBASE_REVERSED("NiInputSystem::SemanticToDeviceControl", 0x00828AB0,
                         &ni::input::SemanticToDeviceControl);
