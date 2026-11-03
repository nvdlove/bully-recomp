// NiLookAtInterpolator (vtable 0x9664A8, 0x44 bytes) and the older NiLookAtController (vtable
// 0x966558, 0x3C): the rotation turning the target's chosen axis (x, y or z; flipped or not)
// toward another object, the world up (UNIT_Z; UNIT_Y when looking along z) kept as close as it
// can be, taken back into the target's parent's space. The interpolator also takes its
// translation, a roll about the axis and its scale from three interpolators of its own, and
// finds the object again by name after cloning (SetUpDependencies / ResolveDependencies).
// Update, the interpolator queries, streaming, cloning, comparison, viewer strings.
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's
// order.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::GetViewerStringPointer;
using scene::LoadBytes;
using scene::SaveBytes;
using scene::Stream_LoadFixedString;
using scene::Stream_LoadInlineFixedString;
using scene::Stream_ReadLinkID;
using scene::Stream_ResolveLinkID;
using scene::Stream_SaveFixedString;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiLookAtInterpolator_RTTI, 0x020E4DF4);
GAME_VAR(RTTI, NiLookAtController_RTTI, 0x020E4E28);

constexpr uintptr_t NiLookAtInterpolator_vtable = 0x009664A8;
constexpr uintptr_t NiLookAtController_vtable = 0x00966558;

GAME_VAR(NiMatrix3, kMatrixIdentity, 0x00BC75FC);   // NiMatrix3::IDENTITY
GAME_VAR(NiPoint3, kUnitY, 0x00BC8A9C);             // NiPoint3::UNIT_Y
GAME_VAR(NiPoint3, kUnitZ, 0x00BC8AA8);             // NiPoint3::UNIT_Z
constexpr float kMaxFloat = 3.40282347e+38f;

GAME_FN(void, __thiscall, Matrix3_MakeIdentity, 0x00768680, NiMatrix3* self);
GAME_FN(NiPoint3*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_TransposeTimes, 0x00768F60, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(void, __thiscall, Matrix3_MakeZRotation, 0x007687D0, NiMatrix3* self, float angle);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Multiply, 0x00768920, const NiMatrix3* self, NiMatrix3* out,
        const NiMatrix3* other);
GAME_FN(float, __thiscall, Point3_Unitize, 0x00415130, NiPoint3* self);
GAME_FN(void, __thiscall, Quaternion_FromRotation, 0x00776040, NiQuaternion* self, const NiMatrix3* m);
GAME_FN(uint8_t, __thiscall, QuatTransform_Equal, 0x00805210, const NiQuatTransform* self,
        const NiQuatTransform* other);
GAME_FN(void, __thiscall, QuatTransform_LoadBinary, 0x00825B50, NiQuatTransform* self, void* stream);
GAME_FN(void, __thiscall, QuatTransform_SaveBinary, 0x00825C50, NiQuatTransform* self, void* stream);
GAME_FN(void, __thiscall, QuatTransform_GetViewerStrings, 0x00825CA0, NiQuatTransform* self,
        NiTArray<char*>* strings);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(NiObject*, __thiscall, Object_Clone, 0x00755540, void* self);
GAME_FN(void, __thiscall, Object_ProcessClone, 0x007553F0, void* self, void* cloning);
GAME_FN(void, __thiscall, Object_SaveBinaryNone, 0x00806A50, void* self, void* stream);   // (the jmp to 0x8A20A0)
GAME_FN(uint8_t, __thiscall, Object_RegisterStreamablesThunk, 0x00807310, void* self, void* stream);
// NiInterpolator's (the next chunk's; its LoadBinary and CopyMembers the key chunk's)
GAME_FN(void, __thiscall, Interpolator_Construct, 0x00825DA0, void* self);
GAME_FN(void, __thiscall, Interpolator_Destruct, 0x00825DC0, void* self);
GAME_FN(NiInterpolator*, __thiscall, Interpolator_GetSequenceInterpolator, 0x00825DF0, void* self, float start,
        float end);
GAME_FN(void, __thiscall, Interpolator_GetViewerStrings, 0x00825E50, void* self, NiTArray<char*>* strings);
GAME_FN(uint8_t, __thiscall, Interpolator_IsEqual, 0x007769C0, void* self, void* other);
GAME_FN(void, __thiscall, Interpolator_LoadBinary, 0x007F79A0, void* self, void* stream);
GAME_FN(void, __thiscall, Interpolator_CopyMembers, 0x007F7990, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, LookAtInterpolator_SetLookAt, 0x00804400, NiLookAtInterpolator* self, NiAVObject* lookAt);
// NiTimeController's
GAME_FN(NiTimeController*, __thiscall, TimeController_Construct, 0x0077DFA0, void* self);
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
GAME_FN(void, __thiscall, TimeController_ProcessClone, 0x0077DEE0, void* self, void* cloning);
GAME_FN(void, __thiscall, TimeController_LinkObject, 0x0077DF10, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TimeController_RegisterStreamables, 0x0077DF70, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TimeController_IsEqual, 0x0077DB90, void* self, void* other);
GAME_FN(void, __thiscall, TimeController_CopyMembers, 0x0077E100, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, TimeController_LoadBinary, 0x0077E1F0, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_SaveBinary, 0x0077E2F0, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_GetViewerStrings, 0x0077E550, void* self, NiTArray<char*>* strings);
// (this file's, called where modes apply)
GAME_FN(NiLookAtInterpolator*, __thiscall, LookAtInterpolator_Construct, 0x0080DE20, void* self, NiAVObject* lookAt,
        uint16_t axis, uint8_t flip);
GAME_FN(void, __thiscall, LookAtInterpolator_Destruct, 0x0080E7B0, NiLookAtInterpolator* self);
GAME_FN(void, __thiscall, LookAtInterpolator_CopyMembers, 0x0080E870, NiLookAtInterpolator* self,
        NiLookAtInterpolator* dest, void* cloning);

// the clone of an object in the cloning's map (its buckets at +0x8, their count at +0x4; an
// item: next, key, value)
inline const re::Ptr32<void>* FindClone(void* cloning, const void* object) {
    void* map = *static_cast<re::Ptr32<void>*>(cloning);
    re::Ptr32<void>* buckets = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reinterpret_cast<uintptr_t>(map) + 8);
    uint32_t count = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(map) + 4);
    for (re::Ptr32<void>* item = static_cast<re::Ptr32<void>*>(buckets[reinterpret_cast<uintptr_t>(object) % count]); item != nullptr;
         item = static_cast<re::Ptr32<void>*>(item[0])) {
        if (item[1] == object) {
            return &item[2];
        }
    }
    return nullptr;
}

// the look-at rotation in the target's parent's space (the target at its local position `pos`):
// false (identity) when the object is too close. `bothPoles`: the interpolator's (UNIT_Y up when
// looking along +z or -z); the controller only checks +z. An unknown axis (3) leaves the matrix
// as the game's stack had it (here: zeros)
bool LookAtRotation(const NiPoint3& lookAtPos, NiAVObject* target, const NiPoint3& pos, uint16_t flags,
                    bool bothPoles, NiMatrix3& rot) {
    NiAVObject* parent = target->parent;
    NiMatrix3 parentRotate;
    NiPoint3 worldPos;
    if (parent != nullptr) {
        parentRotate = parent->world.rotate;
        float scale = parent->world.scale;
        NiPoint3 rotated;
        const NiPoint3* r = Matrix3_TimesPoint(&parentRotate, &rotated, &pos);
        NiPoint3 scaled{r->x * scale, r->y * scale, scale * r->z};
        worldPos.x = parent->world.translate.x + scaled.x;
        worldPos.y = parent->world.translate.y + scaled.y;
        worldPos.z = parent->world.translate.z + scaled.z;
    } else {
        parentRotate = kMatrixIdentity;
        worldPos = pos;
    }
    NiPoint3 d{lookAtPos.x - worldPos.x, lookAtPos.y - worldPos.y, lookAtPos.z - worldPos.z};
    float lengthSquared = (d.x * d.x + d.y * d.y) + d.z * d.z;
    if (lengthSquared < 0.001f) {
        Matrix3_MakeIdentity(&rot);
        return false;
    }
    Point3_Unitize(&d);
    NiPoint3 up;
    float along;
    bool nearPole = d.z < 1.001f && d.z > 0.999f;
    if (bothPoles && !nearPole) {
        nearPole = d.z > -1.001f && d.z < -0.999f;
    }
    if (nearPole) {
        up = kUnitY;
        along = d.y;
    } else {
        up = kUnitZ;
        along = d.z;
    }
    NiPoint3 projection{d.x * along, d.y * along, d.z * along};
    up.x = up.x - projection.x;
    up.y = up.y - projection.y;
    up.z = up.z - projection.z;
    Point3_Unitize(&up);
    if ((flags & 1) == 0) {
        d.x = -d.x;
        d.y = -d.y;
        d.z = -d.z;
    }
    NiPoint3 c;
    c.x = up.y * d.z - up.z * d.y;
    c.y = up.z * d.x - up.x * d.z;
    c.z = up.x * d.y - up.y * d.x;
    NiMatrix3 m{};
    switch ((uint8_t(flags) >> 1) & 3) {
    case 0:   // x the direction, y up, z -(up x direction)
        m.m[0][0] = d.x;
        m.m[1][0] = d.y;
        m.m[2][0] = d.z;
        m.m[0][1] = up.x;
        m.m[1][1] = up.y;
        m.m[2][1] = up.z;
        m.m[0][2] = -c.x;
        m.m[1][2] = -c.y;
        m.m[2][2] = -c.z;
        break;
    case 1:   // x the cross, y the direction, z -up
        m.m[0][0] = c.x;
        m.m[1][0] = c.y;
        m.m[2][0] = c.z;
        m.m[0][1] = d.x;
        m.m[1][1] = d.y;
        m.m[2][1] = d.z;
        m.m[0][2] = -up.x;
        m.m[1][2] = -up.y;
        m.m[2][2] = -up.z;
        break;
    case 2:   // x the cross, y up, z the direction
        m.m[0][0] = c.x;
        m.m[1][0] = c.y;
        m.m[2][0] = c.z;
        m.m[0][1] = up.x;
        m.m[1][1] = up.y;
        m.m[2][1] = up.z;
        m.m[0][2] = d.x;
        m.m[1][2] = d.y;
        m.m[2][2] = d.z;
        break;
    default:
        break;
    }
    NiMatrix3 result;
    rot = *Matrix3_TransposeTimes(&parentRotate, &result, &m);
    return true;
}

// ---- NiLookAtInterpolator

inline void InvalidateValue(NiLookAtInterpolator* self) {
    self->value.translate.x = kInvalidFloat;
    self->value.rotate.x = kInvalidFloat;
    self->value.scale = kInvalidFloat;
}

// 0x80DB10: Collapse (slot 30): each interpolator's
void __fastcall LA_Collapse(NiLookAtInterpolator* self, re::Edx) {
    for (int i = 0; i < 3; i++) {
        if (self->interps[i] != nullptr) {
            Virtual<void>(self->interps[i], 30);
        }
    }
}

// 0x80DB40: GetActiveTimeRange(&begin, &end) (slot 31): over the interpolators' ranges that
// aren't empty; none: 0 and 0
void __fastcall LA_GetActiveTimeRange(NiLookAtInterpolator* self, re::Edx, float* begin, float* end) {
    *begin = kMaxFloat;
    *end = kInvalidFloat;
    for (int i = 0; i < 3; i++) {
        if (self->interps[i] == nullptr) {
            continue;
        }
        float b;
        float e;
        Virtual<void>(self->interps[i], 31, &b, &e);
        if (e != b) {
            if (b < *begin) {
                *begin = b;
            }
            if (*end < e) {
                *end = e;
            }
        }
    }
    if (*begin == kMaxFloat && *end == kInvalidFloat) {
        *begin = 0.0f;
        *end = 0.0f;
    }
}

// 0x80DBF0: GuaranteeTimeRange(start, end) (slot 32)
void __fastcall LA_GuaranteeTimeRange(NiLookAtInterpolator* self, re::Edx, float start, float end) {
    for (int i = 0; i < 3; i++) {
        if (self->interps[i] != nullptr) {
            Virtual<void>(self->interps[i], 32, start, end);
        }
    }
}

// 0x80DC30: GetSequenceInterpolator(start, end) (slot 33): the clone's interpolators the
// sequence copies of these
NiInterpolator* __fastcall LA_GetSequenceInterpolator(NiLookAtInterpolator* self, re::Edx, float start, float end) {
    NiLookAtInterpolator* clone =
        static_cast<NiLookAtInterpolator*>(Interpolator_GetSequenceInterpolator(self, start, end));
    for (int i = 0; i < 3; i++) {
        if (self->interps[i] != nullptr) {
            NiInterpolator* sequence = Virtual<NiInterpolator*>(self->interps[i], 33, start, end);
            AssignReleasingFirst(clone->interps[i], sequence);
        }
    }
    return clone;
}

// 0x80DCE0: ResolveDependencies(palette) (slot 34): the object found by name when missing
uint8_t __fastcall LA_ResolveDependencies(NiLookAtInterpolator* self, re::Edx, void* palette) {
    if (self->lookAt == nullptr) {
        NiAVObject* found = Virtual<NiAVObject*>(palette, 18, self->lookAtName);
        if (found == nullptr) {
            return 0;
        }
        self->lookAt = found;
    }
    return 1;
}

// 0x80DD10: RegisterStreamables
uint8_t __fastcall LA_RegisterStreamables(NiLookAtInterpolator* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamablesThunk(self, stream)) {
        return 0;
    }
    for (int i = 0; i < 3; i++) {
        if (self->interps[i] != nullptr && !Virtual<uint8_t>(self->interps[i], 6, stream)) {
            return 0;
        }
    }
    Virtual<void>(stream, 8, &self->lookAtName);   // RegisterFixedString
    return 1;
}

// 0x80DD70: IsEqual
uint8_t __fastcall LA_IsEqual(NiLookAtInterpolator* self, re::Edx, NiLookAtInterpolator* other) {
    if (!Interpolator_IsEqual(self, other) || self->flags != other->flags) {
        return 0;
    }
    NiAVObject* lookAt = self->lookAt;
    if (lookAt != nullptr) {
        if (other->lookAt == nullptr) {
            return 0;
        }
    } else if (other->lookAt != nullptr) {
        return 0;
    }
    if (lookAt != nullptr && other->lookAt != nullptr && !Virtual<uint8_t>(lookAt, 8, other->lookAt)) {
        return 0;
    }
    if (!QuatTransform_Equal(&self->value, &other->value)) {
        return 0;
    }
    for (int i = 0; i < 3; i++) {
        NiInterpolator* mine = self->interps[i];
        NiInterpolator* theirs = other->interps[i];
        if (mine == nullptr) {
            if (theirs != nullptr) {
                return 0;
            }
        } else {
            if (theirs == nullptr || !Virtual<uint8_t>(mine, 8, theirs)) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x80DE20: NiLookAtInterpolator(lookAt, axis, flip)
NiLookAtInterpolator* __fastcall LA_Construct(NiLookAtInterpolator* self, re::Edx, NiAVObject* lookAt, uint16_t axis,
                                              uint8_t flip) {
    Interpolator_Construct(self);
    self->lookAt = lookAt;
    self->vtable = reinterpret_cast<void*>(NiLookAtInterpolator_vtable);
    self->flags = 0;
    self->lookAtName = scene::NullFixedString;
    self->value.translate = kInvalidTranslate;
    self->value.rotate = kInvalidRotate;
    self->value.scale = kInvalidFloat;
    self->interps[0] = nullptr;
    self->interps[1] = nullptr;
    self->interps[2] = nullptr;
    if (flip != 0) {
        self->flags |= 1;
    } else {
        self->flags &= 0xFFFE;
    }
    self->flags = uint16_t((self->flags & 0xFFF9) | uint16_t(axis * 2));
    for (int i = 0; i < 3; i++) {
        if (self->interps[i] != nullptr) {
            ReleaseRef(self->interps[i]);
            self->interps[i] = nullptr;
        }
    }
    return self;
}

const RTTI* __fastcall LA_GetRTTI(NiLookAtInterpolator*, re::Edx) {
    return &NiLookAtInterpolator_RTTI;
}

// 0x80DF40: Update(time, target, transform&) (slot 18): the translation (its interpolator's, or
// the target's), the look-at rotation (rolled by minus its interpolator's value), the scale
// (its interpolator's); any interpolator failing, or no object to look at: invalid
uint8_t __fastcall LA_Update(NiLookAtInterpolator* self, re::Edx, float time, NiObjectNET* target,
                             NiQuatTransform* value) {
    NiAVObject* lookAt = self->lookAt;
    if (lookAt == nullptr) {
        value->translate.x = kInvalidFloat;
        value->rotate.x = kInvalidFloat;
        value->scale = kInvalidFloat;
        InvalidateValue(self);
        return 0;
    }
    NiPoint3 lookAtPos = lookAt->world.translate;
    NiAVObject* object = static_cast<NiAVObject*>(target);
    NiPoint3 pos;
    if (self->interps[0] != nullptr) {
        if (!Virtual<uint8_t>(self->interps[0], 20, time, target, &pos)) {
            InvalidateValue(self);
            return 0;
        }
        self->value.translate = pos;
    } else {
        pos = object->local.translate;
    }
    NiMatrix3 rot;
    if (LookAtRotation(lookAtPos, object, pos, self->flags, true, rot) && self->interps[1] != nullptr) {
        float roll;
        if (!Virtual<uint8_t>(self->interps[1], 22, time, target, &roll)) {
            InvalidateValue(self);
            return 0;
        }
        NiMatrix3 z;
        Matrix3_MakeZRotation(&z, -roll);
        NiMatrix3 product;
        rot = *Matrix3_Multiply(&rot, &product, &z);
    }
    NiQuaternion q;
    Quaternion_FromRotation(&q, &rot);
    self->value.rotate = q;
    if (self->interps[2] != nullptr) {
        float scale;
        if (!Virtual<uint8_t>(self->interps[2], 22, time, target, &scale)) {
            InvalidateValue(self);
            return 0;
        }
        self->value.scale = scale;
    }
    if (self->value.scale == kInvalidFloat && self->value.rotate.x == kInvalidFloat &&
        self->value.translate.x == kInvalidFloat) {
        value->translate.x = kInvalidFloat;
        value->rotate.x = kInvalidFloat;
        value->scale = kInvalidFloat;
        return 0;
    }
    *value = self->value;
    self->lastTime = time;
    return 1;
}

// 0x80E510: SetUpDependencies (slot 35): the object's name kept and the pointer dropped (found
// again by ResolveDependencies); a nameless object kept, the name cleared
uint8_t __fastcall LA_SetUpDependencies(NiLookAtInterpolator* self, re::Edx) {
    const char* name = self->lookAt->name;
    if (name == nullptr) {
        re::Ptr32<char> none = sys::kNullFixedString;
        FixedString_Assign(&self->lookAtName, re::PtrCast<const re::Ptr32<const char>*>(&none));
        sys::FixedStringRelease(&none);
        return 0;
    }
    re::Ptr32<char> handle = sys::GlobalStringTable_AddString(name);
    FixedString_Assign(&self->lookAtName, re::PtrCast<const re::Ptr32<const char>*>(&handle));
    sys::FixedStringRelease(&handle);
    self->lookAt = nullptr;
    return 1;
}

// 0x80E5D0: ProcessClone (slot 11): the clone's object the object's clone (or the same object)
void __fastcall LA_ProcessClone(NiLookAtInterpolator* self, re::Edx, void* cloning) {
    Object_ProcessClone(self, cloning);
    const re::Ptr32<void>* found = FindClone(cloning, self);
    NiLookAtInterpolator* clone = found != nullptr ? static_cast<NiLookAtInterpolator*>(*found) : nullptr;
    NiAVObject* lookAt = self->lookAt;
    const re::Ptr32<void>* mapped = FindClone(cloning, lookAt);
    clone->lookAt = mapped != nullptr ? static_cast<NiAVObject*>(*mapped) : lookAt;
}

// 0x80E640: CreateObject
NiObject* __cdecl LA_CreateObject() {
    void* memory = NiNew(0x44);
    if (memory == nullptr) {
        return nullptr;
    }
    return LookAtInterpolator_Construct(memory, nullptr, 0, 0);
}

// 0x80E660: LoadBinary (the object and the interpolators read as resolved links)
void __fastcall LA_LoadBinary(NiLookAtInterpolator* self, re::Edx, void* stream) {
    Interpolator_LoadBinary(self, stream);
    LoadBytes(stream, &self->flags, 2);
    self->lookAt = static_cast<NiAVObject*>(Stream_ReadLinkIDObject(stream));
    if (StreamVersion(stream) < 0x14010001) {
        Stream_LoadInlineFixedString(stream, &self->lookAtName);
    } else {
        Stream_LoadFixedString(stream, &self->lookAtName);
    }
    QuatTransform_LoadBinary(&self->value, stream);
    for (int i = 0; i < 3; i++) {
        NiObject* interp = Stream_ReadLinkIDObject(stream);
        AssignReleasingFirst(self->interps[i], interp);
    }
}

// 0x80E730: SaveBinary
void __fastcall LA_SaveBinary(NiLookAtInterpolator* self, re::Edx, void* stream) {
    Object_SaveBinaryNone(self, stream);
    SaveBytes(stream, &self->flags, 2);
    Virtual<void>(stream, 12, self->lookAt);   // SaveLinkID
    Stream_SaveFixedString(stream, &self->lookAtName);
    QuatTransform_SaveBinary(&self->value, stream);
    for (int i = 0; i < 3; i++) {
        Virtual<void>(stream, 12, self->interps[i]);
    }
}

// 0x80E7B0: ~NiLookAtInterpolator
void __fastcall LA_Destruct(NiLookAtInterpolator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiLookAtInterpolator_vtable);
    re::Ptr32<char> none = sys::kNullFixedString;
    FixedString_Assign(&self->lookAtName, re::PtrCast<const re::Ptr32<const char>*>(&none));
    sys::FixedStringRelease(&none);
    for (int i = 2; i >= 0; i--) {
        ReleaseRef(self->interps[i]);
    }
    sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->lookAtName));
    Interpolator_Destruct(self);
}

// 0x80E870: CopyMembers (the interpolators cloned)
void __fastcall LA_CopyMembers(NiLookAtInterpolator* self, re::Edx, NiLookAtInterpolator* dest, void* cloning) {
    Interpolator_CopyMembers(self, dest, cloning);
    uint16_t flags = self->flags;
    dest->flags = flags;
    flags = (self->flags & 1) != 0 ? uint16_t(flags | 1) : uint16_t(flags & 0xFFFE);
    dest->flags = flags;
    dest->flags ^= uint16_t((uint8_t(flags) ^ uint8_t(self->flags)) & 6);
    LookAtInterpolator_SetLookAt(dest, self->lookAt);
    dest->value = self->value;
    for (int i = 0; i < 3; i++) {
        if (self->interps[i] != nullptr) {
            NiObject* clone = Object_Clone(self->interps[i]);
            AssignReleasingFirst(dest->interps[i], clone);
        }
    }
}

// 0x80E930: the deleting destructor
NiLookAtInterpolator* __fastcall LA_DeletingDestructor(NiLookAtInterpolator* self, re::Edx, uint32_t flags) {
    LookAtInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x44);
    }
    return self;
}

// 0x80E950: CreateClone (a failed allocation still copied onto null)
NiLookAtInterpolator* __fastcall LA_CreateClone(NiLookAtInterpolator* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x44);
    if (memory == nullptr) {
        LookAtInterpolator_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiLookAtInterpolator* clone = LookAtInterpolator_Construct(memory, nullptr, 0, 0);
    LookAtInterpolator_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x80E9A0: GetViewerStrings
void __fastcall LA_GetViewerStrings(NiLookAtInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    Interpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiLookAtInterpolator_RTTI.name));
    AddViewerString(strings, GetViewerStringPointer("m_pkLookAt", self->lookAt));
    QuatTransform_GetViewerStrings(&self->value, strings);
    for (int i = 0; i < 3; i++) {
        if (self->interps[i] != nullptr) {
            Virtual<void>(self->interps[i], 9, strings);
        }
    }
}

// ---- NiLookAtController

// 0x80EA60: LinkObject
void __fastcall LC_LinkObject(NiLookAtController* self, re::Edx, void* stream) {
    TimeController_LinkObject(self, stream);
    self->lookAt = static_cast<NiAVObject*>(Stream_ResolveLinkID(stream));
}

// 0x80EA80: RegisterStreamables
uint8_t __fastcall LC_RegisterStreamables(NiLookAtController* self, re::Edx, void* stream) {
    if (!TimeController_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->lookAt != nullptr) {
        Virtual<uint8_t>(self->lookAt, 6, stream);
    }
    return 1;
}

// 0x80EAB0: IsEqual
uint8_t __fastcall LC_IsEqual(NiLookAtController* self, re::Edx, NiLookAtController* other) {
    if (!TimeController_IsEqual(self, other)) {
        return 0;
    }
    NiAVObject* lookAt = self->lookAt;
    if (lookAt != nullptr) {
        if (other->lookAt == nullptr) {
            return 0;
        }
    } else {
        if (other->lookAt != nullptr) {
            return 0;
        }
        return 1;
    }
    return Virtual<uint8_t>(lookAt, 8, other->lookAt) ? 1 : 0;
}

// 0x80EB00: Update(time) (slot 20): while active, the target's rotation the look-at one (the
// time itself unused)
void __fastcall LC_Update(NiLookAtController* self, re::Edx, float) {
    NiAVObject* lookAt = self->lookAt;
    if (lookAt == nullptr || ((uint8_t(self->flags) >> 3) & 1) == 0) {
        return;
    }
    NiAVObject* target = static_cast<NiAVObject*>(self->target);
    NiMatrix3 rot;
    LookAtRotation(lookAt->world.translate, target, target->local.translate, self->lookFlags, false, rot);
    target->local.rotate = rot;
}

const RTTI* __fastcall LC_GetRTTI(NiLookAtController*, re::Edx) {
    return &NiLookAtController_RTTI;
}

// 0x80EEF0: the deleting destructor
NiLookAtController* __fastcall LC_DeletingDestructor(NiLookAtController* self, re::Edx, uint32_t flags) {
    TimeController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3C);
    }
    return self;
}

inline void LCInit(NiLookAtController* self) {
    self->flags &= 0xFFBF;   // (no scaled time)
    self->vtable = reinterpret_cast<void*>(NiLookAtController_vtable);
    self->lookAt = nullptr;
    self->lookFlags = 0;
}

// 0x80EF10: CreateClone (no null check after the copy, as the game has it)
NiLookAtController* __fastcall LC_CreateClone(NiLookAtController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x3C);
    NiLookAtController* clone = nullptr;
    if (memory != nullptr) {
        TimeController_Construct(memory);
        clone = static_cast<NiLookAtController*>(memory);
        LCInit(clone);
    }
    TimeController_CopyMembers(self, clone, cloning);
    uint16_t flags = self->lookFlags;
    clone->lookFlags = flags;
    flags = (self->lookFlags & 1) != 0 ? uint16_t(flags | 1) : uint16_t(flags & 0xFFFE);
    clone->lookFlags = flags;
    clone->lookFlags ^= uint16_t((uint8_t(flags) ^ uint8_t(self->lookFlags)) & 6);
    clone->lookAt = self->lookAt;
    return clone;
}

// 0x80EF90: ProcessClone (slot 11): the clone's object the object's clone (or the same object)
void __fastcall LC_ProcessClone(NiLookAtController* self, re::Edx, void* cloning) {
    TimeController_ProcessClone(self, cloning);
    const re::Ptr32<void>* found = FindClone(cloning, self);
    NiLookAtController* clone = found != nullptr ? static_cast<NiLookAtController*>(*found) : nullptr;
    NiAVObject* lookAt = self->lookAt;
    if (lookAt != nullptr) {
        const re::Ptr32<void>* mapped = FindClone(cloning, lookAt);
        clone->lookAt = mapped != nullptr ? static_cast<NiAVObject*>(*mapped) : lookAt;
    }
}

// 0x80F010: CreateObject
NiObject* __cdecl LC_CreateObject() {
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        return nullptr;
    }
    TimeController_Construct(memory);
    NiLookAtController* self = static_cast<NiLookAtController*>(memory);
    LCInit(self);
    return self;
}

// 0x80F050: LoadBinary
void __fastcall LC_LoadBinary(NiLookAtController* self, re::Edx, void* stream) {
    TimeController_LoadBinary(self, stream);
    LoadBytes(stream, &self->lookFlags, 2);
    Stream_ReadLinkID(stream);
}

// 0x80F0A0: SaveBinary
void __fastcall LC_SaveBinary(NiLookAtController* self, re::Edx, void* stream) {
    TimeController_SaveBinary(self, stream);
    SaveBytes(stream, &self->lookFlags, 2);
    Virtual<void>(stream, 12, self->lookAt);   // SaveLinkID
}

// 0x80F0F0: GetViewerStrings
void __fastcall LC_GetViewerStrings(NiLookAtController* self, re::Edx, NiTArray<char*>* strings) {
    TimeController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiLookAtController_RTTI.name));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_LOOKAT_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::Collapse", 0x0080DB10, &ni::anim::LA_Collapse);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::GetActiveTimeRange", 0x0080DB40, &ni::anim::LA_GetActiveTimeRange);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::GuaranteeTimeRange", 0x0080DBF0, &ni::anim::LA_GuaranteeTimeRange);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::GetSequenceInterpolator", 0x0080DC30,
                      &ni::anim::LA_GetSequenceInterpolator);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::ResolveDependencies", 0x0080DCE0, &ni::anim::LA_ResolveDependencies);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::RegisterStreamables", 0x0080DD10, &ni::anim::LA_RegisterStreamables);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::IsEqual", 0x0080DD70, &ni::anim::LA_IsEqual);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::NiLookAtInterpolator", 0x0080DE20, &ni::anim::LA_Construct);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::GetRTTI", 0x0080DF30, &ni::anim::LA_GetRTTI);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::Update", 0x0080DF40, &ni::anim::LA_Update);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::SetUpDependencies", 0x0080E510, &ni::anim::LA_SetUpDependencies);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::ProcessClone", 0x0080E5D0, &ni::anim::LA_ProcessClone);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::CreateObject", 0x0080E640, &ni::anim::LA_CreateObject);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::LoadBinary", 0x0080E660, &ni::anim::LA_LoadBinary);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::SaveBinary", 0x0080E730, &ni::anim::LA_SaveBinary);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::~NiLookAtInterpolator", 0x0080E7B0, &ni::anim::LA_Destruct);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::CopyMembers", 0x0080E870, &ni::anim::LA_CopyMembers);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::DeletingDestructor", 0x0080E930, &ni::anim::LA_DeletingDestructor);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::CreateClone", 0x0080E950, &ni::anim::LA_CreateClone);
NIC30_LOOKAT_REVERSED("NiLookAtInterpolator::GetViewerStrings", 0x0080E9A0, &ni::anim::LA_GetViewerStrings);
NIC30_LOOKAT_REVERSED("NiLookAtController::LinkObject", 0x0080EA60, &ni::anim::LC_LinkObject);
NIC30_LOOKAT_REVERSED("NiLookAtController::RegisterStreamables", 0x0080EA80, &ni::anim::LC_RegisterStreamables);
NIC30_LOOKAT_REVERSED("NiLookAtController::IsEqual", 0x0080EAB0, &ni::anim::LC_IsEqual);
NIC30_LOOKAT_REVERSED("NiLookAtController::Update", 0x0080EB00, &ni::anim::LC_Update);
NIC30_LOOKAT_REVERSED("NiLookAtController::GetRTTI", 0x0080EEE0, &ni::anim::LC_GetRTTI);
NIC30_LOOKAT_REVERSED("NiLookAtController::DeletingDestructor", 0x0080EEF0, &ni::anim::LC_DeletingDestructor);
NIC30_LOOKAT_REVERSED("NiLookAtController::CreateClone", 0x0080EF10, &ni::anim::LC_CreateClone);
NIC30_LOOKAT_REVERSED("NiLookAtController::ProcessClone", 0x0080EF90, &ni::anim::LC_ProcessClone);
NIC30_LOOKAT_REVERSED("NiLookAtController::CreateObject", 0x0080F010, &ni::anim::LC_CreateObject);
NIC30_LOOKAT_REVERSED("NiLookAtController::LoadBinary", 0x0080F050, &ni::anim::LC_LoadBinary);
NIC30_LOOKAT_REVERSED("NiLookAtController::SaveBinary", 0x0080F0A0, &ni::anim::LC_SaveBinary);
NIC30_LOOKAT_REVERSED("NiLookAtController::GetViewerStrings", 0x0080F0F0, &ni::anim::LC_GetViewerStrings);
