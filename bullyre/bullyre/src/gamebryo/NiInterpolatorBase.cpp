// The interpolators' bases (src/gamebryo/NiInterpolators.h): NiKeyBasedInterpolator (its
// construction and destruction, a key by index, the active time range over its channels' keys,
// viewer strings), NiInterpolator (construction, destruction, the time range and sequence
// interpolator defaults, viewer strings) and NiQuatTransform's arithmetic and streaming (the
// blend interpolators' accumulation runs on it).
//
// Main-thread float maths at the game's 24-bit x87 precision, in the game's order.

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
using scene::GetViewerStringFloat;
using scene::Object_Destruct;
using scene::Object_GetViewerStrings;
using scene::Point3_GetViewerString;
using scene::RefObject_Construct;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiKeyBasedInterpolator_RTTI, 0x020E56B4);
GAME_VAR(RTTI, NiInterpolator_RTTI, 0x020E56F8);

constexpr uintptr_t NiKeyBasedInterpolator_vtable = 0x00968138;
constexpr uintptr_t NiInterpolator_vtable = 0x00968250;

GAME_VAR(NiMatrix3, kMatrixIdentity, 0x00BC75FC);

GAME_FN(NiObject*, __thiscall, Object_Clone, 0x00755540, void* self);
GAME_FN(void, __thiscall, NiPoint3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, NiPoint3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Float4_LoadBinary, 0x007625D0, void* self, void* stream);   // (NiColorA's)
GAME_FN(void, __thiscall, Quaternion_SaveBinary, 0x00776310, const NiQuaternion* self, void* stream);
GAME_FN(char*, __thiscall, Quaternion_GetViewerString, 0x007763B0, const NiQuaternion* self, const char* prefix);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Multiply, 0x00775C40, const NiQuaternion* self, NiQuaternion* result,
        const NiQuaternion* other);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_UnitInverse, 0x00775CD0, NiQuaternion* result, const NiQuaternion* q);
GAME_FN(void, __thiscall, Quaternion_Normalize, 0x00776290, NiQuaternion* self);
GAME_FN(void, __thiscall, Quaternion_ToRotation, 0x00413C50, const NiQuaternion* self, NiMatrix3* m);
GAME_FN(NiPoint3*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, Interpolator_Construct, 0x00825DA0, void* self);
GAME_FN(void, __thiscall, Interpolator_Destruct, 0x00825DC0, void* self);
GAME_FN(void, __thiscall, Interpolator_GetViewerStrings, 0x00825E50, void* self, NiTArray<char*>* strings);

// ---- NiKeyBasedInterpolator

// 0x8253B0: NiKeyBasedInterpolator()
NiKeyBasedInterpolator* __fastcall KeyBased_Construct(NiKeyBasedInterpolator* self, re::Edx) {
    Interpolator_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiKeyBasedInterpolator_vtable);
    return self;
}

// 0x8253D0: ~NiKeyBasedInterpolator
void __fastcall KeyBased_Destruct(NiKeyBasedInterpolator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiKeyBasedInterpolator_vtable);
    Interpolator_Destruct(self);
}

// 0x825410
const RTTI* __fastcall KeyBased_GetRTTI(NiKeyBasedInterpolator*, re::Edx) {
    return &NiKeyBasedInterpolator_RTTI;
}

// 0x825420: the deleting destructor
NiKeyBasedInterpolator* __fastcall KeyBased_DeletingDestructor(NiKeyBasedInterpolator* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiKeyBasedInterpolator_vtable);
    Interpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xC);
    }
    return self;
}

// 0x825450: GetKeyAt(index, channel): the channel's key array (slot 41) at index keys of its
// stride (slot 42)
NiAnimationKey* __fastcall KeyBased_GetKeyAt(NiKeyBasedInterpolator* self, re::Edx, uint32_t index, uint16_t channel) {
    uint8_t stride = Virtual<uint8_t>(self, 42, channel);
    auto* keys = Virtual<uint8_t*>(self, 41, channel);
    return reinterpret_cast<NiAnimationKey*>(keys + uint32_t(stride) * index);
}

// 0x825490: GetActiveTimeRange(&begin, &end) (slot 31): the earliest first key and the latest last
// key over the channels; none: 0 and 0. (An empty channel's "last key" is read from before its
// array, as the game has it)
void __fastcall KeyBased_GetActiveTimeRange(NiKeyBasedInterpolator* self, re::Edx, float* begin, float* end) {
    *begin = 3.40282347e+38f;
    *end = kInvalidFloat;
    for (uint32_t channel = 0; channel < Virtual<uint16_t>(self, 37); channel++) {
        uint16_t c = uint16_t(channel);
        Virtual<uint8_t>(self, 42, c);   // (its stride, unused)
        auto* first = Virtual<NiAnimationKey*>(self, 41, c);
        uint32_t lastIndex = Virtual<uint32_t>(self, 38, c) - 1;
        uint8_t stride = Virtual<uint8_t>(self, 42, c);
        auto* keys = Virtual<uint8_t*>(self, 41, c);
        auto* last = reinterpret_cast<NiAnimationKey*>(keys + uint32_t(stride) * lastIndex);
        if (first == nullptr || last == nullptr) {
            continue;
        }
        if (*begin > first->time) {
            *begin = first->time;
        }
        if (*end < last->time) {
            *end = last->time;
        }
    }
    if (*begin == 3.40282347e+38f && *end == kInvalidFloat) {
        *begin = 0.0f;
        *end = 0.0f;
    }
}

// 0x8255C0: GetViewerStrings
void __fastcall KeyBased_GetViewerStrings(NiKeyBasedInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    Interpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiKeyBasedInterpolator_RTTI.name));
}

// ---- NiQuatTransform

// 0x825610: Invert(dest): the scale's reciprocal, the rotation's inverse and the translation
// negated, each only when valid (the translation not rotated: a delta's undoing)
void __fastcall QuatTransform_Invert(const NiQuatTransform* self, re::Edx, NiQuatTransform* dest) {
    if (self->scale == kInvalidFloat) {
        dest->scale = kInvalidFloat;
    } else {
        dest->scale = 1.0f / self->scale;
    }
    if (self->rotate.x == kInvalidFloat) {
        dest->rotate.x = kInvalidFloat;
    } else {
        NiQuaternion inverse;
        dest->rotate = *Quaternion_UnitInverse(&inverse, &self->rotate);
    }
    if (self->translate.x == kInvalidFloat) {
        dest->translate.x = kInvalidFloat;
        return;
    }
    dest->translate.x = -self->translate.x;
    dest->translate.y = -self->translate.y;
    dest->translate.z = -self->translate.z;
}

// 0x8256E0: HierInvert(dest): the transform's true inverse (the negated translation rotated back
// and scaled by the reciprocal), each part only when valid
void __fastcall QuatTransform_HierInvert(const NiQuatTransform* self, re::Edx, NiQuatTransform* dest) {
    float inverseScale = 1.0f;
    if (self->scale == kInvalidFloat) {
        dest->scale = kInvalidFloat;
    } else {
        inverseScale = 1.0f / self->scale;
        dest->scale = inverseScale;
    }
    NiMatrix3 m = kMatrixIdentity;
    if (self->rotate.x == kInvalidFloat) {
        dest->rotate.x = kInvalidFloat;
    } else {
        NiQuaternion inverse;
        dest->rotate = *Quaternion_UnitInverse(&inverse, &self->rotate);
        Quaternion_ToRotation(&dest->rotate, &m);
    }
    if (self->translate.x == kInvalidFloat) {
        dest->translate.x = kInvalidFloat;
        return;
    }
    NiPoint3 negated = {-self->translate.x, -self->translate.y, -self->translate.z};
    NiPoint3 rotated;
    const NiPoint3* p = Matrix3_TimesPoint(&m, &rotated, &negated);
    float x = p->x * inverseScale;
    float y = p->y * inverseScale;
    float z = inverseScale * p->z;
    dest->translate.x = x;
    dest->translate.y = y;
    dest->translate.z = z;
}

// 0x825820: operator*(other): the other applied in this one's frame (scales multiplied, rotations
// multiplied and normalized, the other's translation rotated by this rotation and scaled by the
// product scale onto this translation); a part invalid when either's is
NiQuatTransform* __fastcall QuatTransform_Multiply(const NiQuatTransform* self, re::Edx, NiQuatTransform* result,
                                                   const NiQuatTransform* other) {
    result->scale = kInvalidFloat;
    float s = 1.0f;
    result->translate = kInvalidTranslate;
    result->rotate = kInvalidRotate;
    if (self->scale != kInvalidFloat && other->scale != kInvalidFloat) {
        s = other->scale * self->scale;
        result->scale = s;
    } else {
        result->scale = kInvalidFloat;
    }
    NiMatrix3 m = kMatrixIdentity;
    if (self->rotate.x != kInvalidFloat && other->rotate.x != kInvalidFloat) {
        NiQuaternion q;
        Quaternion_Multiply(&self->rotate, &q, &other->rotate);
        Quaternion_Normalize(&q);
        result->rotate = q;
        Quaternion_ToRotation(&self->rotate, &m);
    } else {
        result->rotate.x = kInvalidFloat;
    }
    if (self->translate.x != kInvalidFloat && other->translate.x != kInvalidFloat) {
        NiPoint3 rotated;
        const NiPoint3* p = Matrix3_TimesPoint(&m, &rotated, &other->translate);
        float vx = s * p->x;
        float vy = p->y * s;
        float vz = s * p->z;
        result->translate.x = self->translate.x + vx;
        result->translate.y = self->translate.y + vy;
        result->translate.z = self->translate.z + vz;
    } else {
        result->translate.x = kInvalidFloat;
    }
    return result;
}

// 0x8259F0: the other concatenated part by part (scales multiplied, rotations multiplied and
// normalized, translations added, unrotated); a part invalid when either's is. (Its Gamebryo
// name isn't known)
NiQuatTransform* __fastcall QuatTransform_Concatenate(const NiQuatTransform* self, re::Edx, NiQuatTransform* result,
                                                      const NiQuatTransform* other) {
    result->scale = kInvalidFloat;
    result->translate = kInvalidTranslate;
    result->rotate = kInvalidRotate;
    if (self->scale != kInvalidFloat && other->scale != kInvalidFloat) {
        result->scale = other->scale * self->scale;
    } else {
        result->scale = kInvalidFloat;
    }
    if (self->rotate.x != kInvalidFloat && other->rotate.x != kInvalidFloat) {
        NiQuaternion q;
        Quaternion_Multiply(&self->rotate, &q, &other->rotate);
        Quaternion_Normalize(&q);
        result->rotate = q;
    } else {
        result->rotate.x = kInvalidFloat;
    }
    if (self->translate.x != kInvalidFloat && other->translate.x != kInvalidFloat) {
        result->translate.x = other->translate.x + self->translate.x;
        result->translate.y = other->translate.y + self->translate.y;
        result->translate.z = other->translate.z + self->translate.z;
    } else {
        result->translate.x = kInvalidFloat;
    }
    return result;
}

// 0x825B50: LoadBinary (before 10.1.0.110 a byte per part says whether it's valid)
void __fastcall QuatTransform_LoadBinary(NiQuatTransform* self, re::Edx, void* stream) {
    NiPoint3_LoadBinary(&self->translate, stream);
    Float4_LoadBinary(&self->rotate, stream);
    scene::LoadBytes(stream, &self->scale, 4);
    if (scene::StreamVersion(stream) >= 0x0A01006E) {
        return;
    }
    uint8_t valid = 0;
    scene::LoadBytes(stream, &valid, 1);
    if (valid == 0) {
        self->translate.x = kInvalidFloat;
    }
    scene::LoadBytes(stream, &valid, 1);
    if (valid == 0) {
        self->rotate.x = kInvalidFloat;
    }
    scene::LoadBytes(stream, &valid, 1);
    if (valid == 0) {
        self->scale = kInvalidFloat;
    }
}

// 0x825C50: SaveBinary
void __fastcall QuatTransform_SaveBinary(const NiQuatTransform* self, re::Edx, void* stream) {
    NiPoint3_SaveBinary(&self->translate, stream);
    Quaternion_SaveBinary(&self->rotate, stream);
    scene::SaveBytes(stream, &self->scale, 4);
}

// 0x825CA0: GetViewerStrings(strings)
void __fastcall QuatTransform_GetViewerStrings(const NiQuatTransform* self, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, GetViewerString("NiQuatTransform"));
    AddViewerString(strings, Point3_GetViewerString(&self->translate, "m_kTranslate"));
    AddViewerString(strings, Quaternion_GetViewerString(&self->rotate, "m_kRotate"));
    AddViewerString(strings, GetViewerStringFloat("m_fScale", self->scale));
}

// ---- NiInterpolator

// 0x825DA0: NiInterpolator() (no update yet)
NiInterpolator* __fastcall Interpolator_ConstructImpl(NiInterpolator* self, re::Edx) {
    RefObject_Construct(self);
    self->lastTime = kInvalidFloat;
    self->vtable = reinterpret_cast<void*>(NiInterpolator_vtable);
    return self;
}

// 0x825DC0: ~NiInterpolator
void __fastcall Interpolator_DestructImpl(NiInterpolator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiInterpolator_vtable);
    Object_Destruct(self);
}

// 0x825DD0: GetActiveTimeRange(&begin, &end) (slot 31): 0 and 0 (the blend interpolators' too)
void __fastcall Interpolator_GetActiveTimeRange(NiInterpolator*, re::Edx, float* begin, float* end) {
    *begin = 0.0f;
    *end = 0.0f;
}

// 0x825DF0: GetSequenceInterpolator(start, end) (slot 33): a clone, its slot 35 called (the
// blend interpolators' too)
NiInterpolator* __fastcall Interpolator_GetSequenceInterpolator(NiInterpolator* self, re::Edx, float, float) {
    auto* clone = static_cast<NiInterpolator*>(Object_Clone(self));
    Virtual<uint8_t>(clone, 35);
    return clone;
}

// 0x825E10
const RTTI* __fastcall Interpolator_GetRTTI(NiInterpolator*, re::Edx) {
    return &NiInterpolator_RTTI;
}

// 0x825E20: the deleting destructor
NiInterpolator* __fastcall Interpolator_DeletingDestructor(NiInterpolator* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiInterpolator_vtable);
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xC);
    }
    return self;
}

// 0x825E50: GetViewerStrings
void __fastcall Interpolator_GetViewerStringsImpl(NiInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiInterpolator_RTTI.name));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC32_INTERPBASE_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (lone jmps: NiObject's CopyMembers, its empty SaveBinary / LinkObject, NiInterpolator's IsEqual)
RE_UNHOOKED("NiKeyBasedInterpolator::CopyMembers (thunk)", 0x008253E0);
RE_UNHOOKED("NiInterpolator::SaveBinary (thunk)", 0x008253F0);
RE_UNHOOKED("NiKeyBasedInterpolator::IsEqual (thunk)", 0x00825400);

NIC32_INTERPBASE_REVERSED("NiKeyBasedInterpolator::NiKeyBasedInterpolator", 0x008253B0,
                          &ni::anim::KeyBased_Construct);
NIC32_INTERPBASE_REVERSED("NiKeyBasedInterpolator::~NiKeyBasedInterpolator", 0x008253D0,
                          &ni::anim::KeyBased_Destruct);
NIC32_INTERPBASE_REVERSED("NiKeyBasedInterpolator::GetRTTI", 0x00825410, &ni::anim::KeyBased_GetRTTI);
NIC32_INTERPBASE_REVERSED("NiKeyBasedInterpolator::DeletingDestructor", 0x00825420,
                          &ni::anim::KeyBased_DeletingDestructor);
NIC32_INTERPBASE_REVERSED("NiKeyBasedInterpolator::GetKeyAt", 0x00825450, &ni::anim::KeyBased_GetKeyAt);
NIC32_INTERPBASE_REVERSED("NiKeyBasedInterpolator::GetActiveTimeRange", 0x00825490,
                          &ni::anim::KeyBased_GetActiveTimeRange);
NIC32_INTERPBASE_REVERSED("NiKeyBasedInterpolator::GetViewerStrings", 0x008255C0,
                          &ni::anim::KeyBased_GetViewerStrings);
NIC32_INTERPBASE_REVERSED("NiQuatTransform::Invert", 0x00825610, &ni::anim::QuatTransform_Invert);
NIC32_INTERPBASE_REVERSED("NiQuatTransform::HierInvert", 0x008256E0, &ni::anim::QuatTransform_HierInvert);
NIC32_INTERPBASE_REVERSED("NiQuatTransform::operator*", 0x00825820, &ni::anim::QuatTransform_Multiply);
NIC32_INTERPBASE_REVERSED("NiQuatTransform::Concatenate", 0x008259F0, &ni::anim::QuatTransform_Concatenate);
NIC32_INTERPBASE_REVERSED("NiQuatTransform::LoadBinary", 0x00825B50, &ni::anim::QuatTransform_LoadBinary);
NIC32_INTERPBASE_REVERSED("NiQuatTransform::SaveBinary", 0x00825C50, &ni::anim::QuatTransform_SaveBinary);
NIC32_INTERPBASE_REVERSED("NiQuatTransform::GetViewerStrings", 0x00825CA0,
                          &ni::anim::QuatTransform_GetViewerStrings);
NIC32_INTERPBASE_REVERSED("NiInterpolator::NiInterpolator", 0x00825DA0, &ni::anim::Interpolator_ConstructImpl);
NIC32_INTERPBASE_REVERSED("NiInterpolator::~NiInterpolator", 0x00825DC0, &ni::anim::Interpolator_DestructImpl);
NIC32_INTERPBASE_REVERSED("NiInterpolator::GetActiveTimeRange", 0x00825DD0,
                          &ni::anim::Interpolator_GetActiveTimeRange);
NIC32_INTERPBASE_REVERSED("NiInterpolator::GetSequenceInterpolator", 0x00825DF0,
                          &ni::anim::Interpolator_GetSequenceInterpolator);
NIC32_INTERPBASE_REVERSED("NiInterpolator::GetRTTI", 0x00825E10, &ni::anim::Interpolator_GetRTTI);
NIC32_INTERPBASE_REVERSED("NiInterpolator::DeletingDestructor", 0x00825E20,
                          &ni::anim::Interpolator_DeletingDestructor);
NIC32_INTERPBASE_REVERSED("NiInterpolator::GetViewerStrings", 0x00825E50,
                          &ni::anim::Interpolator_GetViewerStringsImpl);
