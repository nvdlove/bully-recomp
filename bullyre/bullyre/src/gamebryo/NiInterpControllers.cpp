// The single-interpolator controllers of this part of the library (NiSingleInterpController's
// kinds, src/gamebryo/NiInterpolators.h): NiVisController (vtable 0x965B50, 0x38: the target's
// app-culled flag from a bool), NiTextureTransformController (vtable 0x965CB0, 0x50: one member
// of a texturing map's transform from a float), NiMaterialColorController (vtable 0x9663F8, 0x3C:
// a NiMaterialProperty colour from a point), NiLightDimmerController (vtable 0x9665C8, 0x38: a
// light's dimmer) and NiLightColorController (vtable 0x966670, 0x3C: its ambient, or diffuse and
// specular, colour). Their Update, target values, streaming (older files' data links turned into
// interpolators), cloning, comparison, IDs and viewer strings; the slots several share.
//
// An Update's interpolator writes its value over the time argument's slot, as the game's code
// passes it (here: a local starting from the time's bits).
//
// Main-thread float maths at the game's 24-bit x87 precision.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDynamicEffect.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiProperties.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiTexturingProperty.h"
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
GAME_VAR(RTTI, NiVisController_RTTI, 0x020E4AD8);
GAME_VAR(RTTI, NiTextureTransformController_RTTI, 0x020E4B40);
GAME_VAR(RTTI, NiMaterialColorController_RTTI, 0x020E4DC0);
GAME_VAR(RTTI, NiLightDimmerController_RTTI, 0x020E4E5C);
GAME_VAR(RTTI, NiLightColorController_RTTI, 0x020E4E90);
GAME_VAR(RTTI, NiLight_RTTI, 0x020E2C68);
GAME_VAR(uint32_t, kNiLightClassIndex, 0x020E2C70);
GAME_VAR(NiPoint3, kBlack, 0x020E25E8);   // NiColor::BLACK

constexpr uintptr_t NiVisController_vtable = 0x00965B50;
constexpr uintptr_t NiTextureTransformController_vtable = 0x00965CB0;
constexpr uintptr_t NiMaterialColorController_vtable = 0x009663F8;
constexpr uintptr_t NiLightDimmerController_vtable = 0x009665C8;
constexpr uintptr_t NiLightColorController_vtable = 0x00966670;

constexpr uint32_t kVersion_10_1_0_104 = 0x0A010068;   // (before: the data linked, not an interpolator)

// NiTextureTransform's start (the map's transform; src/gamebryo/NiTextureTransform.cpp)
struct TextureTransform {
    float translate[2];   // +0x0
    float rotate;         // +0x8
    float scale[2];       // +0xC
    float center[2];      // +0x14
    uint8_t dirty;        // +0x1C
};

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(NiObject*, __cdecl, NiDynamicCast, 0x00401FF0, const RTTI* rtti, NiObject* object);
GAME_FN(void*, __thiscall, TextureTransform_Construct, 0x0077ED00, void* self, const float* translate, float rotate,
        const float* scale, const float* center, uint32_t method);
GAME_FN(void, __thiscall, TextureTransform_SetScale, 0x007D0E70, void* self, const float* scale);
GAME_FN(uint16_t, __thiscall, TexturingProperty_GetShaderMapCount, 0x00516B50, void* self);
GAME_FN(void, __thiscall, Light_SetAmbientColor, 0x0040DAB0, NiLight* self, const NiPoint3* color);
GAME_FN(void, __thiscall, Light_SetDiffuseColor, 0x0040DAE0, NiLight* self, const NiPoint3* color);
GAME_FN(void, __thiscall, Light_SetSpecularColor, 0x0040DB10, NiLight* self, const NiPoint3* color);
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
GAME_FN(void, __thiscall, TimeController_SetTarget, 0x0077E050, void* self, NiObjectNET* target);
GAME_FN(uint8_t, __thiscall, TimeController_DontDoUpdate, 0x0077D890, void* self, float time);
// the interpolators made from older files' data (NiBoolInterpolator, NiFloatInterpolator: the
// next chunk's; NiPoint3Interpolator: src/gamebryo/NiKeyInterpolators.cpp)
GAME_FN(NiInterpolator*, __thiscall, BoolInterpolator_Construct, 0x0081CF70, void* self, NiObject* data);
GAME_FN(NiInterpolator*, __thiscall, FloatInterpolator_Construct, 0x00812260, void* self, NiObject* data);
GAME_FN(NiInterpolator*, __thiscall, Point3Interpolator_Construct, 0x00808540, void* self, NiObject* data);
// the bases' (NiSingleInterpController through its thunks; the kinds' constructors and viewer
// strings)
GAME_FN(void, __thiscall, SingleInterp_LoadBinary, 0x00825F10, void* self, void* stream);
GAME_FN(void, __thiscall, SingleInterp_LinkObject, 0x00824DD0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, SingleInterp_RegisterStreamables, 0x00825F20, void* self, void* stream);
GAME_FN(void, __thiscall, SingleInterp_CopyMembers, 0x00827DC0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, SingleInterp_SaveBinary, 0x00827DD0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, SingleInterp_IsEqual, 0x00827DE0, void* self, void* other);
GAME_FN(void, __thiscall, BoolInterpController_Construct, 0x00824DE0, void* self);
GAME_FN(void, __thiscall, BoolInterpController_GetViewerStrings, 0x00824E70, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, FloatInterpController_Construct, 0x00825F30, void* self);
GAME_FN(void, __thiscall, FloatInterpController_GetViewerStrings, 0x00826010, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Point3InterpController_Construct, 0x00827E40, void* self);
GAME_FN(void, __thiscall, Point3InterpController_GetViewerStrings, 0x00827EF0, void* self,
        NiTArray<char*>* strings);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, TextureTransformController_GetMap, 0x00806270, NiTextureTransformController* self);
GAME_FN(NiTextureTransformController*, __thiscall, TextureTransformController_Construct, 0x008066E0, void* self,
        NiObjectNET* target, NiTexturingMap* map, uint32_t member);
GAME_FN(void, __thiscall, TextureTransform_SetTranslate, 0x00806160, void* self, const float* translate);
GAME_FN(void, __thiscall, TextureTransform_SetRotate, 0x008061B0, void* self, float rotate);

// a float the interpolator writes over the time's slot (its bits as they were when it doesn't)
inline float TimeSlot(float time) {
    return time;
}

// the old data link of an older file made into an interpolator (collapsed), as the
// LinkObjects have it
inline void InterpolatorFromOldData(NiSingleInterpController* self, NiInterpolator* made) {
    AssignReleasingFirst(self->interpolator, made);
    Virtual<void>(self->interpolator, 30);   // Collapse
}

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

// a colour component clamped to 0..1 (a NaN kept)
inline float Clamp01(float x) {
    if (x < 0.0f) {
        return 0.0f;
    }
    if (x > 1.0f) {
        return 1.0f;
    }
    return x;
}

// ---- NiVisController

// 0x804F10: GetTargetBoolValue(value&) (slot 41): not app-culled
void __fastcall Vis_GetTargetBoolValue(NiSingleInterpController* self, re::Edx, uint8_t* value) {
    *value = uint8_t(~static_cast<NiAVObject*>(self->target)->flags & 1);
}

// 0x804F30: LoadBinary (NiVisController's and NiAlphaController's): an older file's data link read
void __fastcall Vis_LoadBinary(NiSingleInterpController* self, re::Edx, void* stream) {
    SingleInterp_LoadBinary(self, stream);
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        Stream_ReadLinkID(stream);
    }
}

// 0x804F50: LinkObject: an older file's NiBoolData made into a NiBoolInterpolator
void __fastcall Vis_LinkObject(NiSingleInterpController* self, re::Edx, void* stream) {
    SingleInterp_LinkObject(self, stream);
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        NiObject* data = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        void* memory = NiNew(0x18);
        NiInterpolator* made = memory != nullptr ? BoolInterpolator_Construct(memory, data) : nullptr;
        InterpolatorFromOldData(self, made);
    }
}

// 0x804FD0: Update(time) (slot 20): the target app-culled when the bool is false
void __fastcall Vis_Update(NiSingleInterpController* self, re::Edx, float time) {
    if (!PassesGate(self, time)) {
        return;
    }
    if (self->interpolator == nullptr) {
        return;
    }
    uint8_t value;
    std::memcpy(&value, &time, 1);
    if (!Virtual<uint8_t>(self->interpolator, 23, self->scaledTime, self->target, &value)) {
        return;
    }
    NiAVObject* target = static_cast<NiAVObject*>(self->target);
    if (value == 0) {
        target->flags |= 1;
    } else {
        target->flags &= 0xFFFE;
    }
}

const RTTI* __fastcall Vis_GetRTTI(NiSingleInterpController*, re::Edx) {
    return &NiVisController_RTTI;
}

// 0x805070: CreateClone (a failed allocation still copied onto null, as the game has it)
NiSingleInterpController* __fastcall Vis_CreateClone(NiSingleInterpController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x38);
    if (memory == nullptr) {
        SingleInterp_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    BoolInterpController_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(NiVisController_vtable);
    SingleInterp_CopyMembers(self, memory, cloning);
    return static_cast<NiSingleInterpController*>(memory);
}

// 0x8050C0: CreateObject
NiObject* __cdecl Vis_CreateObject() {
    void* memory = NiNew(0x38);
    if (memory == nullptr) {
        return nullptr;
    }
    BoolInterpController_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(NiVisController_vtable);
    return static_cast<NiObject*>(memory);
}

// 0x8050F0: GetViewerStrings
void __fastcall Vis_GetViewerStrings(NiSingleInterpController* self, re::Edx, NiTArray<char*>* strings) {
    BoolInterpController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiVisController_RTTI.name));
}

// ---- NiTextureTransformController (and two NiTextureTransform setters)

// 0x806100: SetTarget (slot 21): the map found again for another target
void __fastcall TexTransform_SetTarget(NiTextureTransformController* self, re::Edx, NiObjectNET* target) {
    if (self->target != target) {
        self->map = nullptr;
    }
    TimeController_SetTarget(self, target);
}

// 0x806120: IsEqual
uint8_t __fastcall TexTransform_IsEqual(NiTextureTransformController* self, re::Edx,
                                        NiTextureTransformController* other) {
    if (!SingleInterp_IsEqual(self, other)) {
        return 0;
    }
    if (other->unknown38 != self->unknown38 || other->shaderMap != self->shaderMap ||
        other->mapIndex != self->mapIndex || other->member != self->member) {
        return 0;
    }
    return 1;
}

// 0x806160: NiTextureTransform::SetTranslate(point): dirty unless unchanged and clean
void __fastcall TextureTransform_SetTranslateImpl(TextureTransform* self, re::Edx, const float* translate) {
    if (self->dirty == 0 && translate[0] == self->translate[0] && translate[1] == self->translate[1]) {
        self->dirty = 0;
    } else {
        self->dirty = 1;
    }
    self->translate[0] = translate[0];
    self->translate[1] = translate[1];
}

// 0x8061B0: NiTextureTransform::SetRotate(angle)
void __fastcall TextureTransform_SetRotateImpl(TextureTransform* self, re::Edx, float rotate) {
    if (self->dirty == 0 && rotate == self->rotate) {
        self->rotate = rotate;
        self->dirty = 0;
        return;
    }
    self->rotate = rotate;
    self->dirty = 1;
}

// 0x8061F0: LinkObject (NiTextureTransformController's, the float extra data controllers' and
// NiAlphaController's): an older file's NiFloatData made into a NiFloatInterpolator
void __fastcall FloatController_LinkObject(NiSingleInterpController* self, re::Edx, void* stream) {
    SingleInterp_LinkObject(self, stream);
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        NiObject* data = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        void* memory = NiNew(0x18);
        NiInterpolator* made = memory != nullptr ? FloatInterpolator_Construct(memory, data) : nullptr;
        InterpolatorFromOldData(self, made);
    }
}

// 0x806270: GetMap: the target's map (the property's maps or shader maps at the index), kept
uint8_t __fastcall TexTransform_GetMap(NiTextureTransformController* self, re::Edx) {
    if (self->map != nullptr) {
        return 1;
    }
    NiTexturingProperty* property = re::PtrCast<NiTexturingProperty*>(self->target);
    if (property == nullptr) {
        return 0;
    }
    uint32_t index = self->mapIndex;
    if (self->shaderMap == 0) {
        self->map = property->maps.base[index];
        return self->map != nullptr ? 1 : 0;
    }
    NiTArray<NiTexturingMap*>* shaderMaps = property->shaderMaps;
    if (shaderMaps != nullptr && index < shaderMaps->size) {
        self->map = shaderMaps->base[index];
        return self->map != nullptr ? 1 : 0;
    }
    self->map = nullptr;
    return 0;
}

const RTTI* __fastcall TexTransform_GetRTTI(NiTextureTransformController*, re::Edx) {
    return &NiTextureTransformController_RTTI;
}

// 0x8062E0: Update(time) (slot 20): the member set on the map's transform (one made when the
// map has none: identity, centred, the plain method)
void __fastcall TexTransform_Update(NiTextureTransformController* self, re::Edx, float time) {
    if (!PassesGate(self, time)) {
        return;
    }
    if (self->interpolator == nullptr) {
        return;
    }
    float value = TimeSlot(time);
    if (!Virtual<uint8_t>(self->interpolator, 22, self->scaledTime, self->target, &value)) {
        return;
    }
    if (!TextureTransformController_GetMap(self)) {
        return;
    }
    TextureTransform* transform = static_cast<TextureTransform*>(self->map->transform);
    if (transform == nullptr) {
        void* memory = NiNew(0x48);
        if (memory != nullptr) {
            float center[2] = {0.5f, 0.5f};
            float scale[2] = {1.0f, 1.0f};
            float translate[2] = {0.0f, 0.0f};
            transform = static_cast<TextureTransform*>(
                TextureTransform_Construct(memory, translate, 0.0f, scale, center, 2));
        } else {
            transform = nullptr;
        }
        self->map->transform = transform;
    }
    float pair[2];
    switch (self->member) {
    case 0:
        pair[0] = value;
        pair[1] = transform->translate[1];
        TextureTransform_SetTranslate(transform, pair);
        break;
    case 1:
        pair[0] = transform->translate[0];
        pair[1] = value;
        TextureTransform_SetTranslate(transform, pair);
        break;
    case 2:
        TextureTransform_SetRotate(transform, value);
        break;
    case 3:
        pair[0] = value;
        pair[1] = transform->scale[1];
        TextureTransform_SetScale(transform, pair);
        break;
    case 4:
        pair[0] = transform->scale[0];
        pair[1] = value;
        TextureTransform_SetScale(transform, pair);
        break;
    default:
        break;
    }
}

// 0x8064A0: GetTargetFloatValue(value&) (slot 41): the member (0 when the map has no transform)
void __fastcall TexTransform_GetTargetFloatValue(NiTextureTransformController* self, re::Edx, float* value) {
    if (!TextureTransformController_GetMap(self)) {
        return;
    }
    const TextureTransform* transform = static_cast<const TextureTransform*>(self->map->transform);
    if (transform == nullptr) {
        *value = 0.0f;
        return;
    }
    switch (self->member) {
    case 0:
        *value = transform->translate[0];
        break;
    case 1:
        *value = transform->translate[1];
        break;
    case 2:
        *value = transform->rotate;
        break;
    case 3:
        *value = transform->scale[0];
        break;
    case 4:
        *value = transform->scale[1];
        break;
    default:
        break;
    }
}

// 0x806580: LoadBinary (before 20.2.0.5 the map indices past 5 moved up by 2)
void __fastcall TexTransform_LoadBinary(NiTextureTransformController* self, re::Edx, void* stream) {
    SingleInterp_LoadBinary(self, stream);
    uint8_t shaderMap;
    LoadBytes(stream, &shaderMap, 1);
    self->shaderMap = shaderMap != 0 ? 1 : 0;
    LoadBytes(stream, &self->mapIndex, 4);
    if (StreamVersion(stream) < 0x14020005 && self->mapIndex > 5) {
        self->mapIndex += 2;
    }
    uint32_t member;
    LoadBytes(stream, &member, 4);
    self->member = member;
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        Stream_ReadLinkID(stream);
    }
}

// 0x806650: SaveBinary
void __fastcall TexTransform_SaveBinary(NiTextureTransformController* self, re::Edx, void* stream) {
    SingleInterp_SaveBinary(self, stream);
    uint8_t shaderMap = self->shaderMap;
    SaveBytes(stream, &shaderMap, 1);
    SaveBytes(stream, &self->mapIndex, 4);
    SaveBytes(stream, &self->member, 4);
}

// 0x8066E0: NiTextureTransformController(target, map, member): the map's index found in the
// target's maps, else its shader maps
NiTextureTransformController* __fastcall TexTransform_Construct(NiTextureTransformController* self, re::Edx,
                                                                NiObjectNET* target, NiTexturingMap* map,
                                                                uint32_t member) {
    FloatInterpController_Construct(self);
    self->member = member;
    self->vtable = reinterpret_cast<void*>(NiTextureTransformController_vtable);
    self->ctlrID = nullptr;
    self->unknown38 = 0;
    self->shaderMap = 0;
    self->mapIndex = 0;
    if (self->target != target) {
        self->map = nullptr;
    }
    TimeController_SetTarget(self, target);
    self->map = map;
    if (map == nullptr) {
        NiFree(self->ctlrID);
        self->ctlrID = nullptr;
        return self;
    }
    NiTexturingProperty* property = re::PtrCast<NiTexturingProperty*>(self->target);
    if (property == nullptr) {
        return self;
    }
    for (uint32_t i = 0; i < property->maps.size; i++) {
        if (property->maps.base[i] == map) {
            self->shaderMap = 0;
            self->mapIndex = i;
            NiFree(self->ctlrID);
            self->ctlrID = nullptr;
            TextureTransformController_GetMap(self);
            return self;
        }
    }
    uint32_t count = TexturingProperty_GetShaderMapCount(property);
    if (count == 0) {
        return self;
    }
    uint32_t i = 0;
    for (;;) {
        NiTArray<NiTexturingMap*>* shaderMaps = property->shaderMaps;
        NiTexturingMap* m = (shaderMaps != nullptr && i < shaderMaps->size) ? shaderMaps->base[i] : nullptr;
        if (m == map) {
            break;
        }
        i++;
        if (i >= count) {
            return self;
        }
    }
    self->shaderMap = 1;
    self->mapIndex = i;
    NiFree(self->ctlrID);
    self->ctlrID = nullptr;
    // (GetMap inlined: the map was just set, so this keeps it)
    if (self->map == nullptr && self->target != nullptr) {
        NiTexturingProperty* p = re::PtrCast<NiTexturingProperty*>(self->target);
        uint32_t index = self->mapIndex;
        if (self->shaderMap != 0) {
            NiTArray<NiTexturingMap*>* shaderMaps = p->shaderMaps;
            self->map = (shaderMaps != nullptr && index < shaderMaps->size) ? shaderMaps->base[index] : nullptr;
        } else {
            self->map = p->maps.base[index];
        }
    }
    return self;
}

// 0x806840: the deleting destructor
NiTextureTransformController* __fastcall TexTransform_DeletingDestructor(NiTextureTransformController* self, re::Edx,
                                                                         uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiTextureTransformController_vtable);
    NiFree(self->ctlrID);
    self->ctlrID = nullptr;
    ReleaseRef(self->interpolator);
    TimeController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x50);
    }
    return self;
}

// 0x8068A0: GetCtlrID (slot 34): "shader-index-member" (an unknown member: the buffer as
// allocated)
const char* __fastcall TexTransform_GetCtlrID(NiTextureTransformController* self, re::Edx) {
    if (self->ctlrID != nullptr) {
        return self->ctlrID;
    }
    char* id = static_cast<char*>(NiMalloc(0x32));
    self->ctlrID = id;
    static const char* const kFormats[5] = {"%d-%d-TT_TRANSLATE_U", "%d-%d-TT_TRANSLATE_V", "%d-%d-TT_ROTATE",
                                            "%d-%d-TT_SCALE_U", "%d-%d-TT_SCALE_V"};
    if (self->member <= 4) {
        NiSprintf(id, 0x32, kFormats[self->member], int(self->shaderMap), self->mapIndex);
    }
    return self->ctlrID;
}

// 0x806980: CreateClone (no null check, as the game has it)
NiTextureTransformController* __fastcall TexTransform_CreateClone(NiTextureTransformController* self, re::Edx,
                                                                  void* cloning) {
    void* memory = NiNew(0x50);
    NiTextureTransformController* clone =
        memory != nullptr ? TextureTransformController_Construct(memory, nullptr, nullptr, 0) : nullptr;
    SingleInterp_CopyMembers(self, clone, cloning);
    clone->unknown38 = self->unknown38;
    clone->shaderMap = self->shaderMap;
    clone->mapIndex = self->mapIndex;
    clone->member = self->member;
    return clone;
}

// 0x8069E0: CreateObject
NiObject* __cdecl TexTransform_CreateObject() {
    void* memory = NiNew(0x50);
    if (memory == nullptr) {
        return nullptr;
    }
    return TextureTransformController_Construct(memory, nullptr, nullptr, 0);
}

// 0x806A00: GetViewerStrings
void __fastcall TexTransform_GetViewerStrings(NiTextureTransformController* self, re::Edx, NiTArray<char*>* strings) {
    FloatInterpController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTextureTransformController_RTTI.name));
}

// ---- slots several share

// 0x80D690: RegisterStreamables (NiTextureTransformController's, NiMaterialColorController's and
// the light controllers')
uint8_t __fastcall Shared_RegisterStreamables(NiSingleInterpController* self, re::Edx, void* stream) {
    return SingleInterp_RegisterStreamables(self, stream) != 0 ? 1 : 0;
}

// 0x80F420: InterpTargetIsCorrectType(target) (slot 40, the light controllers'): a NiLight
uint8_t __fastcall Light_InterpTargetIsCorrectType(NiSingleInterpController*, re::Edx, NiObjectNET* target) {
    if (target == nullptr) {
        return 0;
    }
    return IsKindOf(target, kNiLightClassIndex);
}

// 0x80F5E0: LoadBinary (NiMaterialColorController's and NiLightColorController's): the field
void __fastcall Color_LoadBinary(NiColorInterpController* self, re::Edx, void* stream) {
    SingleInterp_LoadBinary(self, stream);
    LoadBytes(stream, &self->field, 2);
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        Stream_ReadLinkID(stream);
    }
}

// 0x80F6E0: SaveBinary (the same two)
void __fastcall Color_SaveBinary(NiColorInterpController* self, re::Edx, void* stream) {
    SingleInterp_SaveBinary(self, stream);
    SaveBytes(stream, &self->field, 2);
}

// ---- NiMaterialColorController

// the material colour the field names (none past self-illumination)
inline NiPoint3* MaterialColor(NiMaterialProperty* material, uint16_t field) {
    switch (field & 7) {
    case 0:
        return &material->ambient;
    case 1:
        return &material->diffuse;
    case 2:
        return &material->specular;
    case 3:
        return &material->emissive;
    default:
        return nullptr;
    }
}

// 0x80D6B0: GetTargetPoint3Value(value&) (slot 41): the colour (0 past the four)
void __fastcall Material_GetTargetPoint3Value(NiColorInterpController* self, re::Edx, NiPoint3* value) {
    NiMaterialProperty* material = re::PtrCast<NiMaterialProperty*>(self->target);
    NiPoint3 color{0.0f, 0.0f, 0.0f};
    switch (self->field & 7) {
    case 0:
    case 1:
    case 2:
    case 3:
        color = *MaterialColor(material, self->field);
        break;
    default:
        break;
    }
    *value = color;
}

// 0x80D740: GetCtlrID (slot 34)
const char* __fastcall Material_GetCtlrID(NiColorInterpController* self, re::Edx) {
    switch (self->field & 7) {
    case 0:
        return "AMB";
    case 1:
        return "DIFF";
    case 2:
        return "SPEC";
    case 3:
        return "SELF_ILLUM";
    default:
        return nullptr;
    }
}

// 0x80D780: LinkObject: an older file's NiPosData made into a NiPoint3Interpolator
void __fastcall Material_LinkObject(NiColorInterpController* self, re::Edx, void* stream) {
    SingleInterp_LinkObject(self, stream);
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        NiObject* data = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        void* memory = NiNew(0x20);
        NiInterpolator* made = memory != nullptr ? Point3Interpolator_Construct(memory, data) : nullptr;
        InterpolatorFromOldData(self, made);
    }
}

// 0x80D800: IsEqual (the field's colour bits too)
uint8_t __fastcall Material_IsEqual(NiColorInterpController* self, re::Edx, NiColorInterpController* other) {
    if (!SingleInterp_IsEqual(self, other)) {
        return 0;
    }
    return ((other->field ^ self->field) & 7) == 0 ? 1 : 0;
}

const RTTI* __fastcall Material_GetRTTI(NiColorInterpController*, re::Edx) {
    return &NiMaterialColorController_RTTI;
}

// 0x80D840: Update(time) (slot 20): the colour set (each component clamped to 0..1), the
// material's revision moved on
void __fastcall Material_Update(NiColorInterpController* self, re::Edx, float time) {
    if (!PassesGate(self, time)) {
        return;
    }
    if (self->interpolator == nullptr) {
        return;
    }
    NiPoint3 value;
    if (!Virtual<uint8_t>(self->interpolator, 20, self->scaledTime, self->target, &value)) {
        return;
    }
    NiMaterialProperty* material = re::PtrCast<NiMaterialProperty*>(self->target);
    NiPoint3 c{Clamp01(value.x), Clamp01(value.y), Clamp01(value.z)};
    NiPoint3* color = MaterialColor(material, self->field);
    if (color != nullptr) {
        material->revisionID++;
        *color = c;
    }
}

// 0x80D9F0: the deleting destructor
NiColorInterpController* __fastcall Material_DeletingDestructor(NiColorInterpController* self, re::Edx,
                                                                uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiMaterialColorController_vtable);
    ReleaseRef(self->interpolator);
    TimeController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3C);
    }
    return self;
}

// the colour controllers' CreateClone (no null check after the copy, as the game has it)
inline NiColorInterpController* ColorCreateClone(NiColorInterpController* self, void* cloning, uintptr_t vtable) {
    void* memory = NiNew(0x3C);
    NiColorInterpController* clone = nullptr;
    if (memory != nullptr) {
        Point3InterpController_Construct(memory);
        clone = static_cast<NiColorInterpController*>(memory);
        clone->vtable = reinterpret_cast<void*>(vtable);
        clone->field = 0;
    }
    SingleInterp_CopyMembers(self, clone, cloning);
    clone->field = self->field;
    return clone;
}

inline NiObject* ColorCreateObject(uintptr_t vtable) {
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        return nullptr;
    }
    Point3InterpController_Construct(memory);
    NiColorInterpController* self = static_cast<NiColorInterpController*>(memory);
    self->vtable = reinterpret_cast<void*>(vtable);
    self->field = 0;
    return self;
}

// 0x80DA40: CreateClone
NiColorInterpController* __fastcall Material_CreateClone(NiColorInterpController* self, re::Edx, void* cloning) {
    return ColorCreateClone(self, cloning, NiMaterialColorController_vtable);
}

// 0x80DA90: CreateObject
NiObject* __cdecl Material_CreateObject() {
    return ColorCreateObject(NiMaterialColorController_vtable);
}

// 0x80DAC0: GetViewerStrings
void __fastcall Material_GetViewerStrings(NiColorInterpController* self, re::Edx, NiTArray<char*>* strings) {
    Point3InterpController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiMaterialColorController_RTTI.name));
}

// ---- NiLightDimmerController

// 0x80F160: IsEqual
uint8_t __fastcall Dimmer_IsEqual(NiSingleInterpController* self, re::Edx, NiSingleInterpController* other) {
    return SingleInterp_IsEqual(self, other) != 0 ? 1 : 0;
}

// 0x80F180: GetTargetFloatValue(value&) (slot 41): the dimmer
void __fastcall Dimmer_GetTargetFloatValue(NiSingleInterpController* self, re::Edx, float* value) {
    if (self->target != nullptr) {
        *value = re::PtrCast<NiLight*>(self->target)->dimmer;
    }
}

const RTTI* __fastcall Dimmer_GetRTTI(NiSingleInterpController*, re::Edx) {
    return &NiLightDimmerController_RTTI;
}

// 0x80F1B0: Update(time) (slot 20): the light's dimmer set, its revision moved on
void __fastcall Dimmer_Update(NiSingleInterpController* self, re::Edx, float time) {
    if (!PassesGate(self, time)) {
        return;
    }
    if (self->interpolator == nullptr) {
        return;
    }
    float value = TimeSlot(time);
    if (!Virtual<uint8_t>(self->interpolator, 22, self->scaledTime, self->target, &value)) {
        return;
    }
    if (self->target == nullptr) {
        return;
    }
    NiLight* light = reinterpret_cast<NiLight*>(NiDynamicCast(&NiLight_RTTI, self->target));
    light->dimmer = value;
    light->revisionID++;
}

// 0x80F240: the deleting destructor
NiSingleInterpController* __fastcall Dimmer_DeletingDestructor(NiSingleInterpController* self, re::Edx,
                                                               uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiLightDimmerController_vtable);
    ReleaseRef(self->interpolator);
    TimeController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x38);
    }
    return self;
}

// 0x80F290: CreateClone (a failed allocation still copied onto null)
NiSingleInterpController* __fastcall Dimmer_CreateClone(NiSingleInterpController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x38);
    if (memory == nullptr) {
        SingleInterp_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    FloatInterpController_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(NiLightDimmerController_vtable);
    SingleInterp_CopyMembers(self, memory, cloning);
    return static_cast<NiSingleInterpController*>(memory);
}

// 0x80F2E0: CreateObject
NiObject* __cdecl Dimmer_CreateObject() {
    void* memory = NiNew(0x38);
    if (memory == nullptr) {
        return nullptr;
    }
    FloatInterpController_Construct(memory);
    *static_cast<re::Ptr32<void>*>(memory) = reinterpret_cast<void*>(NiLightDimmerController_vtable);
    return static_cast<NiObject*>(memory);
}

// 0x80F310: GetViewerStrings
void __fastcall Dimmer_GetViewerStrings(NiSingleInterpController* self, re::Edx, NiTArray<char*>* strings) {
    FloatInterpController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiLightDimmerController_RTTI.name));
}

// ---- NiLightColorController

// 0x80F360: GetTargetPoint3Value(value&) (slot 41): the ambient colour, or the diffuse one
void __fastcall LightColor_GetTargetPoint3Value(NiColorInterpController* self, re::Edx, NiPoint3* value) {
    NiLight* light = re::PtrCast<NiLight*>(self->target);
    if (light == nullptr) {
        return;
    }
    *value = (self->field & 1) != 0 ? light->ambient : light->diffuse;
}

// 0x80F3C0: GetCtlrID (slot 34)
const char* __fastcall LightColor_GetCtlrID(NiColorInterpController* self, re::Edx) {
    return (self->field & 1) != 0 ? "Ambient" : "Diffuse";
}

// 0x80F3E0: IsEqual (the ambient bit too)
uint8_t __fastcall LightColor_IsEqual(NiColorInterpController* self, re::Edx, NiColorInterpController* other) {
    if (!SingleInterp_IsEqual(self, other)) {
        return 0;
    }
    return uint8_t((~uint8_t(other->field) ^ uint8_t(self->field)) & 1);
}

const RTTI* __fastcall LightColor_GetRTTI(NiColorInterpController*, re::Edx) {
    return &NiLightColorController_RTTI;
}

// 0x80F460: Update(time) (slot 20): ambient: the ambient colour set and the others black; else
// the diffuse and specular ones set and the ambient black (each component clamped to 0..1)
void __fastcall LightColor_Update(NiColorInterpController* self, re::Edx, float time) {
    if (!PassesGate(self, time)) {
        return;
    }
    if (self->interpolator == nullptr) {
        return;
    }
    NiPoint3 value;
    if (!Virtual<uint8_t>(self->interpolator, 20, self->scaledTime, self->target, &value)) {
        return;
    }
    NiPoint3 c{Clamp01(value.x), Clamp01(value.y), Clamp01(value.z)};
    if (self->target == nullptr) {
        return;
    }
    NiLight* light = reinterpret_cast<NiLight*>(NiDynamicCast(&NiLight_RTTI, self->target));
    if ((self->field & 1) != 0) {
        Light_SetAmbientColor(light, &c);
        Light_SetDiffuseColor(light, &kBlack);
        Light_SetSpecularColor(light, &kBlack);
        return;
    }
    Light_SetAmbientColor(light, &kBlack);
    Light_SetDiffuseColor(light, &c);
    Light_SetSpecularColor(light, &c);
}

// 0x80F630: LinkObject: an older file's NiPosData (held meanwhile) made into a
// NiPoint3Interpolator
void __fastcall LightColor_LinkObject(NiColorInterpController* self, re::Edx, void* stream) {
    SingleInterp_LinkObject(self, stream);
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        NiObject* data = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        AddRefImport(data);
        void* memory = NiNew(0x20);
        NiInterpolator* made = memory != nullptr ? Point3Interpolator_Construct(memory, data) : nullptr;
        InterpolatorFromOldData(self, made);
        ReleaseRef(data);
    }
}

// 0x80F720: the deleting destructor
NiColorInterpController* __fastcall LightColor_DeletingDestructor(NiColorInterpController* self, re::Edx,
                                                                  uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiLightColorController_vtable);
    ReleaseRef(self->interpolator);
    TimeController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3C);
    }
    return self;
}

// 0x80F770: CreateClone
NiColorInterpController* __fastcall LightColor_CreateClone(NiColorInterpController* self, re::Edx, void* cloning) {
    return ColorCreateClone(self, cloning, NiLightColorController_vtable);
}

// 0x80F7C0: CreateObject
NiObject* __cdecl LightColor_CreateObject() {
    return ColorCreateObject(NiLightColorController_vtable);
}

// 0x80F7F0: GetViewerStrings
void __fastcall LightColor_GetViewerStrings(NiColorInterpController* self, re::Edx, NiTArray<char*>* strings) {
    Point3InterpController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiLightColorController_RTTI.name));
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_INTERPCTLR_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// lone jmps to NiSingleInterpController's thunks (the next chunk's): SaveBinary (0x827DD0) and
// IsEqual (0x827DE0) as NiVisController's / NiAlphaController's slots 7 and 8, LoadBinary
// (0x825F10) and LinkObject (0x824DD0) as NiLightDimmerController's slots 4 and 5
RE_UNHOOKED("NiVisController::SaveBinary (thunk)", 0x00804EF0);
RE_UNHOOKED("NiVisController::IsEqual (thunk)", 0x00804F00);
RE_UNHOOKED("NiLightDimmerController::LoadBinary (thunk)", 0x0080F140);
RE_UNHOOKED("NiLightDimmerController::LinkObject (thunk)", 0x0080F150);

NIC30_INTERPCTLR_REVERSED("NiVisController::GetTargetBoolValue", 0x00804F10, &ni::anim::Vis_GetTargetBoolValue);
NIC30_INTERPCTLR_REVERSED("NiVisController::LoadBinary", 0x00804F30, &ni::anim::Vis_LoadBinary);
NIC30_INTERPCTLR_REVERSED("NiVisController::LinkObject", 0x00804F50, &ni::anim::Vis_LinkObject);
NIC30_INTERPCTLR_REVERSED("NiVisController::Update", 0x00804FD0, &ni::anim::Vis_Update);
NIC30_INTERPCTLR_REVERSED("NiVisController::GetRTTI", 0x00805060, &ni::anim::Vis_GetRTTI);
NIC30_INTERPCTLR_REVERSED("NiVisController::CreateClone", 0x00805070, &ni::anim::Vis_CreateClone);
NIC30_INTERPCTLR_REVERSED("NiVisController::CreateObject", 0x008050C0, &ni::anim::Vis_CreateObject);
NIC30_INTERPCTLR_REVERSED("NiVisController::GetViewerStrings", 0x008050F0, &ni::anim::Vis_GetViewerStrings);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::SetTarget", 0x00806100, &ni::anim::TexTransform_SetTarget);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::IsEqual", 0x00806120, &ni::anim::TexTransform_IsEqual);
NIC30_INTERPCTLR_REVERSED("NiTextureTransform::SetTranslate", 0x00806160,
                          &ni::anim::TextureTransform_SetTranslateImpl);
NIC30_INTERPCTLR_REVERSED("NiTextureTransform::SetRotate", 0x008061B0, &ni::anim::TextureTransform_SetRotateImpl);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::LinkObject", 0x008061F0,
                          &ni::anim::FloatController_LinkObject);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::GetMap", 0x00806270, &ni::anim::TexTransform_GetMap);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::GetRTTI", 0x008062D0, &ni::anim::TexTransform_GetRTTI);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::Update", 0x008062E0, &ni::anim::TexTransform_Update);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::GetTargetFloatValue", 0x008064A0,
                          &ni::anim::TexTransform_GetTargetFloatValue);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::LoadBinary", 0x00806580,
                          &ni::anim::TexTransform_LoadBinary);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::SaveBinary", 0x00806650,
                          &ni::anim::TexTransform_SaveBinary);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::NiTextureTransformController", 0x008066E0,
                          &ni::anim::TexTransform_Construct);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::DeletingDestructor", 0x00806840,
                          &ni::anim::TexTransform_DeletingDestructor);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::GetCtlrID", 0x008068A0, &ni::anim::TexTransform_GetCtlrID);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::CreateClone", 0x00806980,
                          &ni::anim::TexTransform_CreateClone);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::CreateObject", 0x008069E0,
                          &ni::anim::TexTransform_CreateObject);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::GetViewerStrings", 0x00806A00,
                          &ni::anim::TexTransform_GetViewerStrings);
NIC30_INTERPCTLR_REVERSED("NiTextureTransformController::RegisterStreamables", 0x0080D690,
                          &ni::anim::Shared_RegisterStreamables);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::GetTargetPoint3Value", 0x0080D6B0,
                          &ni::anim::Material_GetTargetPoint3Value);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::GetCtlrID", 0x0080D740, &ni::anim::Material_GetCtlrID);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::LinkObject", 0x0080D780, &ni::anim::Material_LinkObject);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::IsEqual", 0x0080D800, &ni::anim::Material_IsEqual);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::GetRTTI", 0x0080D830, &ni::anim::Material_GetRTTI);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::Update", 0x0080D840, &ni::anim::Material_Update);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::DeletingDestructor", 0x0080D9F0,
                          &ni::anim::Material_DeletingDestructor);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::CreateClone", 0x0080DA40, &ni::anim::Material_CreateClone);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::CreateObject", 0x0080DA90, &ni::anim::Material_CreateObject);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::GetViewerStrings", 0x0080DAC0,
                          &ni::anim::Material_GetViewerStrings);
NIC30_INTERPCTLR_REVERSED("NiLightDimmerController::IsEqual", 0x0080F160, &ni::anim::Dimmer_IsEqual);
NIC30_INTERPCTLR_REVERSED("NiLightDimmerController::GetTargetFloatValue", 0x0080F180,
                          &ni::anim::Dimmer_GetTargetFloatValue);
NIC30_INTERPCTLR_REVERSED("NiLightDimmerController::GetRTTI", 0x0080F1A0, &ni::anim::Dimmer_GetRTTI);
NIC30_INTERPCTLR_REVERSED("NiLightDimmerController::Update", 0x0080F1B0, &ni::anim::Dimmer_Update);
NIC30_INTERPCTLR_REVERSED("NiLightDimmerController::DeletingDestructor", 0x0080F240,
                          &ni::anim::Dimmer_DeletingDestructor);
NIC30_INTERPCTLR_REVERSED("NiLightDimmerController::CreateClone", 0x0080F290, &ni::anim::Dimmer_CreateClone);
NIC30_INTERPCTLR_REVERSED("NiLightDimmerController::CreateObject", 0x0080F2E0, &ni::anim::Dimmer_CreateObject);
NIC30_INTERPCTLR_REVERSED("NiLightDimmerController::GetViewerStrings", 0x0080F310,
                          &ni::anim::Dimmer_GetViewerStrings);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::GetTargetPoint3Value", 0x0080F360,
                          &ni::anim::LightColor_GetTargetPoint3Value);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::GetCtlrID", 0x0080F3C0, &ni::anim::LightColor_GetCtlrID);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::IsEqual", 0x0080F3E0, &ni::anim::LightColor_IsEqual);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::GetRTTI", 0x0080F410, &ni::anim::LightColor_GetRTTI);
NIC30_INTERPCTLR_REVERSED("NiLightDimmerController::InterpTargetIsCorrectType", 0x0080F420,
                          &ni::anim::Light_InterpTargetIsCorrectType);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::Update", 0x0080F460, &ni::anim::LightColor_Update);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::LoadBinary", 0x0080F5E0, &ni::anim::Color_LoadBinary);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::LinkObject", 0x0080F630, &ni::anim::LightColor_LinkObject);
NIC30_INTERPCTLR_REVERSED("NiMaterialColorController::SaveBinary", 0x0080F6E0, &ni::anim::Color_SaveBinary);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::DeletingDestructor", 0x0080F720,
                          &ni::anim::LightColor_DeletingDestructor);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::CreateClone", 0x0080F770, &ni::anim::LightColor_CreateClone);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::CreateObject", 0x0080F7C0, &ni::anim::LightColor_CreateObject);
NIC30_INTERPCTLR_REVERSED("NiLightColorController::GetViewerStrings", 0x0080F7F0,
                          &ni::anim::LightColor_GetViewerStrings);
