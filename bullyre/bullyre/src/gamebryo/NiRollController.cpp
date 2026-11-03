// NiFloatController (vtable 0x965EC8, 0x3C bytes: an old-style controller with its own
// NiFloatData keys) and NiRollController (vtable 0x965F38): the target's rotation turned about
// its z axis by the keys' value each update. Plus the TargetIsRequiredType slot the old
// transform controllers share (a NiAVObject target).
//
// Main-thread float maths at the game's 24-bit x87 precision.

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

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiFloatController_RTTI, 0x020E5770);
GAME_VAR(RTTI, NiRollController_RTTI, 0x020E4C10);
GAME_VAR(uint32_t, kNiAVObjectClassIndex, 0x020E251C);

constexpr uintptr_t NiRollController_vtable = 0x00965F38;

GAME_FN(float, __cdecl, FloatKey_GenInterp, 0x007F8300, float time, NiAnimationKey* keys, KeyType type,
        uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(void, __thiscall, Matrix3_MakeZRotation, 0x007687D0, NiMatrix3* self, float angle);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Multiply, 0x00768920, const NiMatrix3* self, NiMatrix3* out,
        const NiMatrix3* other);
GAME_FN(NiTimeController*, __thiscall, TimeController_Construct, 0x0077DFA0, void* self);
GAME_FN(uint8_t, __thiscall, TimeController_DontDoUpdate, 0x0077D890, void* self, float time);
GAME_FN(void, __thiscall, TimeController_GetViewerStrings, 0x0077E550, void* self, NiTArray<char*>* strings);
// NiFloatController's (the next chunk's): its destructor, its keys, CopyMembers
GAME_FN(void, __thiscall, FloatController_Destruct, 0x00826190, void* self);
GAME_FN(NiAnimationKey*, __thiscall, FloatController_GetAnim, 0x008261F0, void* self, uint32_t* numKeys,
        KeyType* type, uint8_t* size);
GAME_FN(void, __thiscall, FloatController_CopyMembers, 0x00826350, void* self, void* dest, void* cloning);

const RTTI* __fastcall FloatController_GetRTTI(NiFloatController*, re::Edx) {
    return &NiFloatController_RTTI;
}

const RTTI* __fastcall Roll_GetRTTI(NiFloatController*, re::Edx) {
    return &NiRollController_RTTI;
}

// 0x807740: the deleting destructor (NiFloatController's and NiRollController's)
NiFloatController* __fastcall FloatController_DeletingDestructor(NiFloatController* self, re::Edx, uint32_t flags) {
    FloatController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3C);
    }
    return self;
}

// 0x807760: TargetIsRequiredType (slot 27): a NiAVObject (NiRollController's, NiPathController's,
// NiMultiTargetTransformController's, NiLookAtController's)
uint8_t __fastcall TargetIsAVObject(NiTimeController* self, re::Edx) {
    if (self->target == nullptr) {
        return 0;
    }
    return IsKindOf(self->target, kNiAVObjectClassIndex);
}

// 0x807790: NiRollController::Update(time) (slot 20): the target's rotation times a rotation
// about z by minus the keys' value at the scaled time
void __fastcall Roll_Update(NiFloatController* self, re::Edx, float time) {
    if (self->target == nullptr || TimeController_DontDoUpdate(self, time)) {
        return;
    }
    uint32_t numKeys;
    KeyType type;
    uint8_t size;
    NiAnimationKey* keys = FloatController_GetAnim(self, &numKeys, &type, &size);
    float roll = FloatKey_GenInterp(self->scaledTime, keys, type, numKeys, &self->lastIdx, size);
    NiAVObject* target = static_cast<NiAVObject*>(self->target);
    NiMatrix3 rotation = target->local.rotate;
    NiMatrix3 z;
    Matrix3_MakeZRotation(&z, -roll);
    NiMatrix3 product;
    rotation = *Matrix3_Multiply(&rotation, &product, &z);
    target->local.rotate = rotation;
}

// 0x807860: NiRollController::CreateClone (a failed allocation still copied onto null, as the
// game has it)
NiFloatController* __fastcall Roll_CreateClone(NiFloatController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        FloatController_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiFloatController* clone = static_cast<NiFloatController*>(memory);
    TimeController_Construct(clone);
    clone->data = nullptr;
    clone->lastIdx = 0;
    clone->vtable = reinterpret_cast<void*>(NiRollController_vtable);
    FloatController_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x8078C0: NiRollController::CreateObject
NiObject* __cdecl Roll_CreateObject() {
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        return nullptr;
    }
    NiFloatController* self = static_cast<NiFloatController*>(memory);
    TimeController_Construct(self);
    self->data = nullptr;
    self->lastIdx = 0;
    self->vtable = reinterpret_cast<void*>(NiRollController_vtable);
    return self;
}

// 0x807900: NiRollController::GetViewerStrings (the time controller's, then its name)
void __fastcall Roll_GetViewerStrings(NiFloatController* self, re::Edx, NiTArray<char*>* strings) {
    TimeController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiRollController_RTTI.name));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_ROLL_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// NiRollController's slots 4..8: lone jmps to NiFloatController's LoadBinary (0x826170),
// LinkObject (0x826270), RegisterStreamables (0x8262D0), SaveBinary (0x826300), IsEqual
// (0x826320), the next chunk's
RE_UNHOOKED("NiRollController::LoadBinary (thunk)", 0x008076D0);
RE_UNHOOKED("NiRollController::LinkObject (thunk)", 0x008076E0);
RE_UNHOOKED("NiRollController::RegisterStreamables (thunk)", 0x008076F0);
RE_UNHOOKED("NiRollController::SaveBinary (thunk)", 0x00807700);
RE_UNHOOKED("NiRollController::IsEqual (thunk)", 0x00807710);

NIC30_ROLL_REVERSED("NiFloatController::GetRTTI", 0x00807720, &ni::anim::FloatController_GetRTTI);
NIC30_ROLL_REVERSED("NiRollController::GetRTTI", 0x00807730, &ni::anim::Roll_GetRTTI);
NIC30_ROLL_REVERSED("NiFloatController::DeletingDestructor", 0x00807740,
                    &ni::anim::FloatController_DeletingDestructor);
NIC30_ROLL_REVERSED("NiRollController::TargetIsRequiredType", 0x00807760, &ni::anim::TargetIsAVObject);
NIC30_ROLL_REVERSED("NiRollController::Update", 0x00807790, &ni::anim::Roll_Update);
NIC30_ROLL_REVERSED("NiRollController::CreateClone", 0x00807860, &ni::anim::Roll_CreateClone);
NIC30_ROLL_REVERSED("NiRollController::CreateObject", 0x008078C0, &ni::anim::Roll_CreateObject);
NIC30_ROLL_REVERSED("NiRollController::GetViewerStrings", 0x00807900, &ni::anim::Roll_GetViewerStrings);
