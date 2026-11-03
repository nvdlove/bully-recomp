// The end of the key chunk: NiFloatData's and NiPosData's GetRTTI and deleting destructors, and
// the small setters and getters the old-controller conversion (src/gamebryo/NiAnimConversion.cpp)
// calls: a float and a point interpolator's keys set (their data made when missing), the
// look-at interpolator's object and its translate, roll and scale interpolators, the transform
// interpolator's position and scale keys read, and one controller's timing copied to another.
//
// The interpolators are read by offset (their layouts are src/gamebryo/NiInterpolators.h's, the
// other chunk's): NiFloatInterpolator's data +0x10 and last index +0x14; NiPoint3Interpolator's
// +0x18 / +0x1C; NiLookAtInterpolator's object +0x10, its name +0x14, the interpolators +0x38,
// +0x3C, +0x40; NiTransformInterpolator's data +0x2C (NiTransformData: the position channel
// +0x18, the scale one +0x28, each count, keys, type, key size). A key data object (0x18 bytes):
// count +0x8, keys +0xC, type +0x10, key size +0x14 (a byte).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::animkeys {
namespace {

using sys::Decrement;
using sys::Increment;
using sys::kNullFixedString;
using sys::Virtual;

constexpr uintptr_t kNiFloatData_RTTI = 0x020E5034;
constexpr uintptr_t kNiPosData_RTTI = 0x020E4C78;
constexpr uintptr_t kNiFloatDataVtable = 0x00965AC0;
constexpr uintptr_t kNiPosDataVtable = 0x00965B08;

GAME_FN(void, __thiscall, FloatData_Destruct, 0x00812BF0, void* self);
GAME_FN(void, __thiscall, PosData_Destruct, 0x00808120, void* self);
GAME_FN(void, __thiscall, FloatData_SetAnim, 0x00812C20, void* self, void* keys, uint32_t numKeys, uint32_t type);
GAME_FN(void, __thiscall, PosData_SetAnim, 0x00808150, void* self, void* keys, uint32_t numKeys, uint32_t type);
GAME_FN(void*, __thiscall, Object_Construct, 0x00755260, void* self);   // NiObject()
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* self, NiRefObject* object);
// (NiMaterialResource::SetVariable's code: an adjusting thunk assigning the NiFixedString at +0x14)
GAME_FN(void, __thiscall, Object_AssignName14, 0x007E2D70, void* self, const re::Ptr32<char>* value);

inline void ReleaseImport(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

using re::At;

// a key data object's constructor as the setters inline it (NiObject, then the vtable and an
// empty channel)
NiRefObject* NewKeyData(uintptr_t vtable) {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    Object_Construct(memory);
    At<uint32_t>(memory, 0) = vtable;
    At<uint32_t>(memory, 0x8) = 0;
    At<void*>(memory, 0xC) = nullptr;
    At<uint32_t>(memory, 0x10) = 0;
    At<uint8_t>(memory, 0x14) = 0;
    return static_cast<NiRefObject*>(memory);
}

// an interpolator slot set: a new one held (unless it's the one there), none clears it
void SetInterpolator(re::Ptr32<NiRefObject>& slot, NiRefObject* interpolator) {
    NiRefObject* old = slot;
    if (interpolator == nullptr) {
        if (old != nullptr) {
            ReleaseImport(old);
            slot = nullptr;
        }
        return;
    }
    if (old == interpolator) {
        return;
    }
    ReleaseImport(old);
    slot = interpolator;
    Increment(&interpolator->refCount);
}

// 0x804320 NiFloatData::GetRTTI
const void* __fastcall FloatData_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<const void*>(kNiFloatData_RTTI);
}

// 0x804330 NiFloatData's deleting destructor (0x18 bytes)
void* __fastcall FloatData_DeletingDestructor(void* self, re::Edx, uint32_t flags) {
    FloatData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x804350 NiPosData::GetRTTI
const void* __fastcall PosData_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<const void*>(kNiPosData_RTTI);
}

// 0x804360 NiPosData's deleting destructor (0x18 bytes)
void* __fastcall PosData_DeletingDestructor(void* self, re::Edx, uint32_t flags) {
    PosData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x804380 NiFloatInterpolator::SetKeys(keys, count, type): a NiFloatData made when there's none,
// the keys set on it and the last index reset; no keys empty the data there is (the index kept)
void __fastcall FloatInterpolator_SetKeys(void* self, re::Edx, void* keys, uint32_t numKeys, uint32_t type) {
    re::Ptr32<NiRefObject>& data = At<NiRefObject*>(self, 0x10);
    if (numKeys == 0) {
        if (data != nullptr) {
            FloatData_SetAnim(data, nullptr, 0, 0);
        }
        return;
    }
    if (data == nullptr) {
        Pointer_Assign(&data, NewKeyData(kNiFloatDataVtable));
    }
    FloatData_SetAnim(data, keys, numKeys, type);
    At<uint32_t>(self, 0x14) = 0;
}

// 0x804400 NiLookAtInterpolator::SetLookAt(object): the object kept (not held), its name cleared
void __fastcall LookAtInterpolator_SetLookAt(void* self, re::Edx, void* lookAt) {
    At<void*>(self, 0x10) = lookAt;
    re::Ptr32<char> name = kNullFixedString;
    Object_AssignName14(self, &name);
    sys::FixedStringRelease(&name);
}

// 0x804450 NiLookAtInterpolator::SetTranslateInterpolator
void __fastcall LookAtInterpolator_SetTranslateInterpolator(void* self, re::Edx, NiRefObject* interpolator) {
    SetInterpolator(At<NiRefObject*>(self, 0x38), interpolator);
}

// 0x8044C0 NiLookAtInterpolator::SetRollInterpolator
void __fastcall LookAtInterpolator_SetRollInterpolator(void* self, re::Edx, NiRefObject* interpolator) {
    SetInterpolator(At<NiRefObject*>(self, 0x3C), interpolator);
}

// 0x804530 NiLookAtInterpolator::SetScaleInterpolator
void __fastcall LookAtInterpolator_SetScaleInterpolator(void* self, re::Edx, NiRefObject* interpolator) {
    SetInterpolator(At<NiRefObject*>(self, 0x40), interpolator);
}

// 0x8045A0 NiPoint3Interpolator::SetKeys(keys, count, type): as the float one's, a NiPosData
void __fastcall Point3Interpolator_SetKeys(void* self, re::Edx, void* keys, uint32_t numKeys, uint32_t type) {
    re::Ptr32<NiRefObject>& data = At<NiRefObject*>(self, 0x18);
    if (numKeys == 0) {
        if (data != nullptr) {
            PosData_SetAnim(data, nullptr, 0, 0);
        }
        return;
    }
    if (data == nullptr) {
        Pointer_Assign(&data, NewKeyData(kNiPosDataVtable));
    }
    PosData_SetAnim(data, keys, numKeys, type);
    At<uint32_t>(self, 0x1C) = 0;
}

// a transform data channel read out (none: zeros)
void* ReadChannel(void* self, uint32_t channel, uint32_t* numKeys, uint32_t* type, uint8_t* size) {
    void* data = At<void*>(self, 0x2C);
    if (data == nullptr) {
        *numKeys = 0;
        *type = 0;
        *size = 0;
        return nullptr;
    }
    *numKeys = At<uint32_t>(data, channel);
    *type = At<uint32_t>(data, channel + 8);
    *size = At<uint8_t>(data, channel + 0xC);
    return At<void*>(data, channel + 4);
}

// 0x804620 NiTransformInterpolator::GetPosData(count, type, key size out): the position keys
void* __fastcall TransformInterpolator_GetPosData(void* self, re::Edx, uint32_t* numKeys, uint32_t* type,
                                                  uint8_t* size) {
    return ReadChannel(self, 0x18, numKeys, type, size);
}

// 0x804670 NiTransformInterpolator::GetScaleData(count, type, key size out): the scale keys
void* __fastcall TransformInterpolator_GetScaleData(void* self, re::Edx, uint32_t* numKeys, uint32_t* type,
                                                    uint8_t* size) {
    return ReadChannel(self, 0x28, numKeys, type, size);
}

// 0x8046C0 (static) a controller's timing copied to another: the animation type (app time
// making it active, app init not), the cycle type, the direction, the frequency and the begin
// and end key times (not the phase), then the active flag itself. The source's flags byte is
// read again at each step, as the game does (they could be the same controller)
void __cdecl CopyControllerTiming(void* from, void* to) {
    uint16_t& flags = At<uint16_t>(to, 8);
    uint32_t animType = At<uint8_t>(from, 8) & 1u;
    uint32_t value = (flags & 0xFFFEu) | animType;
    flags = static_cast<uint16_t>(value);
    value = animType == 0 ? (value | 8u) : (value & 0xFFF7u);
    flags = static_cast<uint16_t>(value);
    flags = static_cast<uint16_t>(flags ^ ((static_cast<uint8_t>(value) ^ At<uint8_t>(from, 8)) & 6u));
    if (((At<uint8_t>(from, 8) >> 4) & 1u) != 0) {
        flags = static_cast<uint16_t>(flags | 0x10u);
    } else {
        flags = static_cast<uint16_t>(flags & 0xFFEFu);
    }
    At<float>(to, 0xC) = At<float>(from, 0xC);
    At<float>(to, 0x14) = At<float>(from, 0x14);
    At<float>(to, 0x18) = At<float>(from, 0x18);
    if (((At<uint8_t>(from, 8) >> 3) & 1u) != 0) {
        flags = static_cast<uint16_t>(flags | 8u);
    } else {
        flags = static_cast<uint16_t>(flags & 0xFFF7u);
    }
}

} // namespace
} // namespace ni::animkeys

// ---- registration ---------------------------------------------------------------------

#define NIANIMKEYSET_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

using namespace ni::animkeys;

NIANIMKEYSET_REVERSED("NiFloatData::GetRTTI", 0x00804320, FloatData_GetRTTI);
NIANIMKEYSET_REVERSED("NiFloatData::DeletingDestructor", 0x00804330, FloatData_DeletingDestructor);
NIANIMKEYSET_REVERSED("NiPosData::GetRTTI", 0x00804350, PosData_GetRTTI);
NIANIMKEYSET_REVERSED("NiPosData::DeletingDestructor", 0x00804360, PosData_DeletingDestructor);
NIANIMKEYSET_REVERSED("NiFloatInterpolator::SetKeys", 0x00804380, FloatInterpolator_SetKeys);
NIANIMKEYSET_REVERSED("NiLookAtInterpolator::SetLookAt", 0x00804400, LookAtInterpolator_SetLookAt);
NIANIMKEYSET_REVERSED("NiLookAtInterpolator::SetTranslateInterpolator", 0x00804450,
                      LookAtInterpolator_SetTranslateInterpolator);
NIANIMKEYSET_REVERSED("NiLookAtInterpolator::SetRollInterpolator", 0x008044C0, LookAtInterpolator_SetRollInterpolator);
NIANIMKEYSET_REVERSED("NiLookAtInterpolator::SetScaleInterpolator", 0x00804530,
                      LookAtInterpolator_SetScaleInterpolator);
NIANIMKEYSET_REVERSED("NiPoint3Interpolator::SetKeys", 0x008045A0, Point3Interpolator_SetKeys);
NIANIMKEYSET_REVERSED("NiTransformInterpolator::GetPosData", 0x00804620, TransformInterpolator_GetPosData);
NIANIMKEYSET_REVERSED("NiTransformInterpolator::GetScaleData", 0x00804670, TransformInterpolator_GetScaleData);
NIANIMKEYSET_REVERSED("NiAnimConversion::CopyControllerTiming", 0x008046C0, CopyControllerTiming);
