// NiFlipController (vtable 0x966C78, 0x54 bytes: src/gamebryo/NiAnimControllers.h): a
// NiFloatInterpController flipping a NiTexturingProperty's map (or shader map) through a set of
// textures, the float its index. Construction, Update, streaming (an older file's start time and
// step made into step float keys), cloning, comparison, the texture array, its ID; and
// NiMultiTargetTransformController::AddInterpolatorTarget, which sits between it and the
// sequences in the exe.
//
// Main-thread float maths at the game's 24-bit x87 precision.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiAnimControllers.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiTexturingProperty.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::LoadBytes;
using scene::SaveBytes;
using scene::Stream_GetNumberOfLinkIDs;
using scene::Stream_ReadMultipleLinkIDs;
using scene::Stream_ResolveLinkID;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiFlipController_RTTI, 0x020E5068);
GAME_VAR(uint32_t, kNiTexturingPropertyClassIndex, 0x020E28C4);

constexpr uintptr_t NiFlipController_vtable = 0x00966C78;
constexpr uintptr_t TextureArray_vtable = 0x009893C0;   // NiTObjectArray<NiSourceTexturePtr>
constexpr uintptr_t NiFloatData_vtable = 0x00965AC0;
constexpr uint32_t kVersion_10_1_0_104 = 0x0A010068;
constexpr uint32_t kShaderMapIndex = 0x400;             // an affected map from here on: a shader map's

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void*, __cdecl, NiAllocArray, 0x007524D0, uint32_t size);
GAME_FN(re::Ptr32<NiObject>*, __cdecl, NiNewPointerArray, 0x00856120, uint32_t count);
GAME_FN(void, __cdecl, NiDeletePointerArray, 0x00836F80, void* p);
GAME_FN(void, __thiscall, TextureArray_SetSize, 0x00856170, NiTArray<NiObject*>* self, uint32_t size);
GAME_FN(void, __thiscall, TextureArray_SetAt, 0x0040E800, NiTArray<NiObject*>* self, uint32_t index,
        const re::Ptr32<NiObject>* value);
GAME_FN(void, __thiscall, Object_Construct, 0x00755260, void* self);
// NiStream::ReadMultipleLinkIDs (its count returned)
GAME_FN(uint32_t, __thiscall, Stream_ReadMultipleLinkIDsCount, 0x0075A870, void* stream);
GAME_FN(uint8_t, __thiscall, TimeController_DontDoUpdate, 0x0077D890, void* self, float time);
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
// NiTexturingProperty::Map::SetTexture, NiTexturingProperty::GetShaderMap(index)
GAME_FN(void, __thiscall, TexturingMap_SetTexture, 0x00829350, NiTexturingMap* self, NiObject* texture);
GAME_FN(NiTexturingMap*, __thiscall, TexturingProperty_GetShaderMap, 0x0074A7E0, NiTexturingProperty* self,
        uint32_t index);
// NiSingleInterpController's / NiFloatInterpController's (the next chunks')
GAME_FN(void, __thiscall, FloatInterpController_Construct, 0x00825F30, void* self);
GAME_FN(void, __thiscall, SingleInterp_LoadBinary, 0x00825F10, void* self, void* stream);
GAME_FN(void, __thiscall, SingleInterp_LinkObject, 0x00824DD0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, SingleInterp_RegisterStreamables, 0x00825F20, void* self, void* stream);
GAME_FN(void, __thiscall, SingleInterp_CopyMembers, 0x00827DC0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, SingleInterp_SaveBinary, 0x00827DD0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, SingleInterp_IsEqual, 0x00827DE0, void* self, void* other);
// the interpolators made (src/gamebryo/NiFloatInterpolator.cpp; the blend float one: the next
// chunk's) and NiFloatData::ReplaceAnim
GAME_FN(NiInterpolator*, __thiscall, FloatInterpolator_ConstructWithData, 0x00812260, void* self, NiObject* data);
GAME_FN(NiInterpolator*, __thiscall, BlendFloatInterpolator_Construct, 0x008201E0, void* self, int managerControlled,
        float weightThreshold, int arraySize);
GAME_FN(void, __thiscall, FloatData_ReplaceAnim, 0x00812D30, NiObject* self, NiAnimationKey* keys, uint32_t numKeys,
        KeyType type);
// (this file's, called where modes apply)
GAME_FN(NiFlipController*, __thiscall, FlipController_Construct, 0x00813590, void* self);
GAME_FN(void, __thiscall, FlipController_Destruct, 0x008134B0, NiFlipController* self);
GAME_FN(void, __thiscall, FlipController_SetTexture, 0x00813600, NiFlipController* self, NiObject* texture,
        uint32_t index);
GAME_FN(void, __thiscall, FlipController_CopyMembers, 0x00813680, NiFlipController* self, NiFlipController* dest,
        void* cloning);

// the Update gate the interpolator controllers share (src/gamebryo/NiExtraDataControllers.cpp)
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

// ---- NiFlipController

// 0x813010: GetTargetFloatValue(value&) (slot 41): the index shown
void __fastcall Flip_GetTargetFloatValue(NiFlipController* self, re::Edx, float* value) {
    *value = float(self->index);
}

// 0x813030: IsEqual (the affected map too)
uint8_t __fastcall Flip_IsEqual(NiFlipController* self, re::Edx, NiFlipController* other) {
    if (!SingleInterp_IsEqual(self, other)) {
        return 0;
    }
    return self->affectedMap == other->affectedMap ? 1 : 0;
}

// 0x813060: CreateBlendInterpolator(index, managerControlled, accumulate, weightThreshold,
// arraySize) (slot 37): a NiBlendFloatInterpolator that only takes the highest weight (flag 2;
// set through null when the allocation fails, as the game has it)
NiInterpolator* __fastcall Flip_CreateBlendInterpolator(NiFlipController*, re::Edx, uint16_t, int managerControlled, int,
                                                        float weightThreshold, int arraySize) {
    void* memory = NiNew(0x34);
    NiInterpolator* blend = nullptr;
    if (memory != nullptr) {
        blend = BlendFloatInterpolator_Construct(memory, managerControlled, weightThreshold, arraySize);
    }
    *reinterpret_cast<volatile uint8_t*>(reinterpret_cast<uintptr_t>(blend) + 0xC) |= 2;
    return blend;
}

// 0x8130A0: RegisterStreamables: the textures too
uint8_t __fastcall Flip_RegisterStreamables(NiFlipController* self, re::Edx, void* stream) {
    if (!SingleInterp_RegisterStreamables(self, stream)) {
        return 0;
    }
    uint32_t count = self->textures.size;
    for (uint32_t i = 0; i < count; i++) {
        NiObject* texture = self->textures.base[i];
        if (texture != nullptr) {
            Virtual<uint8_t>(texture, 6, stream);
        }
    }
    return 1;
}

// 0x8130F0: Update(time) (slot 20): the index from the float (+0.01, cut toward zero, kept to the
// last texture), its texture set on the map (or the shader map past 0x400)
void __fastcall Flip_Update(NiFlipController* self, re::Edx, float time) {
    if (!PassesGate(self, time) || self->interpolator == nullptr) {
        return;
    }
    float value;   // (not set first, as the game has it)
    if (!Virtual<uint8_t>(self->interpolator, 22, self->scaledTime, self->target, &value)) {
        return;
    }
    uint32_t count = self->textures.size;
    self->index = uint32_t(int64_t(value + 0.01f));
    if (self->index >= count) {
        self->index = count - 1;
    }
    NiTexturingProperty* property = re::PtrCast<NiTexturingProperty*>(self->target);
    if (property == nullptr) {
        return;
    }
    uint32_t map = self->affectedMap;
    if (map < property->maps.size && property->maps.base[map] != nullptr) {
        TexturingMap_SetTexture(property->maps.base[map], self->textures.base[self->index]);
        return;
    }
    uint32_t shaderIndex = map - kShaderMapIndex;
    NiTArray<NiTexturingMap*>* shaderMaps = property->shaderMaps;
    if (shaderMaps == nullptr || shaderIndex >= shaderMaps->size) {
        return;
    }
    if (TexturingProperty_GetShaderMap(property, shaderIndex) == nullptr) {
        return;
    }
    NiObject* texture = self->textures.base[self->index];
    TexturingMap_SetTexture(TexturingProperty_GetShaderMap(property, shaderIndex), texture);
}

// 0x813220: LoadBinary: the affected map; from 10.1.0.104 the texture links; before, the start
// time and step made into step float keys (one per texture and one more), the time extrema set,
// and a float interpolator over them (collapsed)
void __fastcall Flip_LoadBinary(NiFlipController* self, re::Edx, void* stream) {
    SingleInterp_LoadBinary(self, stream);
    LoadBytes(stream, &self->affectedMap, 4);
    if (StreamVersion(stream) >= kVersion_10_1_0_104) {
        Stream_ReadMultipleLinkIDs(stream);
        return;
    }
    float start;
    LoadBytes(stream, &start, 4);
    float delta;
    LoadBytes(stream, &delta, 4);
    uint32_t count = Stream_ReadMultipleLinkIDsCount(stream);
    NiObject* data = static_cast<NiObject*>(NiNew(0x18));
    if (data != nullptr) {
        Object_Construct(data);
        NiKeyChannelData* d = static_cast<NiKeyChannelData*>(data);
        d->vtable = reinterpret_cast<void*>(NiFloatData_vtable);
        d->numKeys = 0;
        d->keys = nullptr;
        d->type = NOINTERP;
        d->size = 0;
    }
    if (count != 0) {
        uint32_t numKeys = count + 1;
        uint32_t* block = static_cast<uint32_t*>(NiAllocArray(KeyArraySize(numKeys, sizeof(NiFloatKey))));
        NiFloatKey* keys = nullptr;
        if (block != nullptr) {
            *block = numKeys;
            keys = reinterpret_cast<NiFloatKey*>(block + 1);
        }
        float t = start;
        for (uint32_t i = 0; i < count; i++) {
            keys[i].time = t;
            keys[i].value = float(i);
            t = t + delta;
        }
        keys[count].time = t;
        keys[count].value = float(count - 1);
        FloatData_ReplaceAnim(data, keys, numKeys, STEPKEY);
        self->loKeyTime = start;
        self->hiKeyTime = t;
    }
    void* memory = NiNew(0x18);
    NiInterpolator* made = memory != nullptr ? FloatInterpolator_ConstructWithData(memory, data) : nullptr;
    AssignReleasingFirst(self->interpolator, made);
    Virtual<void>(self->interpolator, 30);   // Collapse
}

// 0x813420: SaveBinary (the affected map, the texture links)
void __fastcall Flip_SaveBinary(NiFlipController* self, re::Edx, void* stream) {
    SingleInterp_SaveBinary(self, stream);
    SaveBytes(stream, &self->affectedMap, 4);
    uint32_t count = self->textures.size;
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(stream, 12, self->textures.base[i]);   // SaveLinkID
    }
}

// 0x8134B0: ~NiFlipController (the ID freed, the texture array's storage freed as it is)
void __fastcall Flip_Destruct(NiFlipController* self, re::Edx) {
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(NiFlipController_vtable);
    NiFree(self->ctlrID);
    self->ctlrID = nullptr;
    self->textures.vtable = reinterpret_cast<void*>(TextureArray_vtable);
    NiDeletePointerArray(self->textures.base);
    ReleaseRef(self->interpolator);
    TimeController_Destruct(self);
}

const RTTI* __fastcall Flip_GetRTTI(NiFlipController*, re::Edx) {
    return &NiFlipController_RTTI;
}

// 0x813520: InterpTargetIsCorrectType(target) (slot 40; NiTextureTransformController's too): a
// NiTexturingProperty
uint8_t __fastcall Flip_InterpTargetIsCorrectType(NiFlipController*, re::Edx, NiObject* target) {
    if (target == nullptr) {
        return 0;
    }
    return IsKindOf(target, kNiTexturingPropertyClassIndex);
}

// 0x813560: GetCtlrID (slot 34): the affected map's number, made the first time
const char* __fastcall Flip_GetCtlrID(NiFlipController* self, re::Edx) {
    if (self->ctlrID == nullptr) {
        self->ctlrID = static_cast<char*>(NiMalloc(0xF));
        NiSprintf(self->ctlrID, 0xF, "%d", self->affectedMap);
    }
    return self->ctlrID;
}

// 0x813590: NiFlipController() (room for one texture)
NiFlipController* __fastcall Flip_Construct(NiFlipController* self, re::Edx) {
    FloatInterpController_Construct(self);
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(NiFlipController_vtable);
    self->textures.vtable = reinterpret_cast<void*>(TextureArray_vtable);
    self->textures.maxSize = 1;
    self->textures.growBy = 1;
    self->textures.size = 0;
    self->textures.esize = 0;
    self->textures.base = NiNewPointerArray(1);
    self->textures.vtable = reinterpret_cast<void*>(TextureArray_vtable);
    self->index = 0;
    self->affectedMap = 0;
    self->ctlrID = nullptr;
    return self;
}

// 0x8135E0: the deleting destructor
NiFlipController* __fastcall Flip_DeletingDestructor(NiFlipController* self, re::Edx, uint32_t flags) {
    FlipController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x54);
    }
    return self;
}

// 0x813600: SetTexture(texture, index): the array grown to take it
void __fastcall Flip_SetTexture(NiFlipController* self, re::Edx, NiObject* texture, uint32_t index) {
    if (self->textures.size <= index) {
        TextureArray_SetSize(&self->textures, index + 1);
    }
    re::Ptr32<NiObject> held = texture;
    AddRefImport(texture);
    if (index >= self->textures.maxSize) {
        TextureArray_SetSize(&self->textures, self->textures.growBy + index);
    }
    TextureArray_SetAt(&self->textures, index, &held);
    ReleaseRef(texture);
}

// 0x813680: CopyMembers(dest, cloning): the affected map, the textures shared
void __fastcall Flip_CopyMembers(NiFlipController* self, re::Edx, NiFlipController* dest, void* cloning) {
    SingleInterp_CopyMembers(self, dest, cloning);
    dest->affectedMap = self->affectedMap;
    for (uint32_t i = 0; i < self->textures.size; i++) {
        NiObject* texture = i < self->textures.size ? self->textures.base[i] : nullptr;
        FlipController_SetTexture(dest, texture, i);
    }
}

// 0x8136D0: CreateObject
NiObject* __cdecl Flip_CreateObject() {
    void* memory = NiNew(0x54);
    if (memory == nullptr) {
        return nullptr;
    }
    return FlipController_Construct(memory);
}

// 0x8136F0: LinkObject: with a target, the textures from their links
void __fastcall Flip_LinkObject(NiFlipController* self, re::Edx, void* stream) {
    SingleInterp_LinkObject(self, stream);
    if (self->target == nullptr) {
        return;
    }
    uint32_t count = Stream_GetNumberOfLinkIDs(stream);
    TextureArray_SetSize(&self->textures, count);
    for (uint32_t i = 0; i < count; i++) {
        NiObject* texture = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        re::Ptr32<NiObject> held = texture;
        AddRefImport(texture);
        TextureArray_SetAt(&self->textures, i, &held);
        ReleaseRef(texture);
    }
}

// 0x813780: CreateClone (a failed allocation still copied onto null, as the game has it)
NiFlipController* __fastcall Flip_CreateClone(NiFlipController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x54);
    if (memory == nullptr) {
        FlipController_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiFlipController* clone = FlipController_Construct(memory);
    FlipController_CopyMembers(self, clone, cloning);
    return clone;
}

// ---- NiMultiTargetTransformController

// 0x813800: AddInterpolatorTarget(target): the first free target slot taken (the target marked
// never rigid, 0x80); its index, or 0xFFFF when full
uint16_t __fastcall MT_AddInterpolatorTarget(NiMultiTargetTransformController* self, re::Edx, NiAVObject* target) {
    for (uint16_t i = 0; i < self->numInterps; i++) {
        if (self->targets[i] == nullptr) {
            self->targets[i] = target;
            target->flags |= 0x80;
            return i;
        }
    }
    return 0xFFFF;
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_FLIP_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC31_FLIP_REVERSED("NiFlipController::GetTargetFloatValue", 0x00813010, &ni::anim::Flip_GetTargetFloatValue);
NIC31_FLIP_REVERSED("NiFlipController::IsEqual", 0x00813030, &ni::anim::Flip_IsEqual);
NIC31_FLIP_REVERSED("NiFlipController::CreateBlendInterpolator", 0x00813060, &ni::anim::Flip_CreateBlendInterpolator);
NIC31_FLIP_REVERSED("NiFlipController::RegisterStreamables", 0x008130A0, &ni::anim::Flip_RegisterStreamables);
NIC31_FLIP_REVERSED("NiFlipController::Update", 0x008130F0, &ni::anim::Flip_Update);
NIC31_FLIP_REVERSED("NiFlipController::LoadBinary", 0x00813220, &ni::anim::Flip_LoadBinary);
NIC31_FLIP_REVERSED("NiFlipController::SaveBinary", 0x00813420, &ni::anim::Flip_SaveBinary);
NIC31_FLIP_REVERSED("NiFlipController::~NiFlipController", 0x008134B0, &ni::anim::Flip_Destruct);
NIC31_FLIP_REVERSED("NiFlipController::GetRTTI", 0x00813510, &ni::anim::Flip_GetRTTI);
NIC31_FLIP_REVERSED("NiFlipController::InterpTargetIsCorrectType", 0x00813520,
                    &ni::anim::Flip_InterpTargetIsCorrectType);
NIC31_FLIP_REVERSED("NiFlipController::GetCtlrID", 0x00813560, &ni::anim::Flip_GetCtlrID);
NIC31_FLIP_REVERSED("NiFlipController::NiFlipController", 0x00813590, &ni::anim::Flip_Construct);
NIC31_FLIP_REVERSED("NiFlipController::DeletingDestructor", 0x008135E0, &ni::anim::Flip_DeletingDestructor);
NIC31_FLIP_REVERSED("NiFlipController::SetTexture", 0x00813600, &ni::anim::Flip_SetTexture);
NIC31_FLIP_REVERSED("NiFlipController::CopyMembers", 0x00813680, &ni::anim::Flip_CopyMembers);
NIC31_FLIP_REVERSED("NiFlipController::CreateObject", 0x008136D0, &ni::anim::Flip_CreateObject);
NIC31_FLIP_REVERSED("NiFlipController::LinkObject", 0x008136F0, &ni::anim::Flip_LinkObject);
NIC31_FLIP_REVERSED("NiFlipController::CreateClone", 0x00813780, &ni::anim::Flip_CreateClone);
NIC31_FLIP_REVERSED("NiMultiTargetTransformController::AddInterpolatorTarget", 0x00813800,
                    &ni::anim::MT_AddInterpolatorTarget);
