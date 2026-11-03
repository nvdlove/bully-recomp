// NiPointLight and NiSpotLight (src/gamebryo/NiPointLight.h): construction, cloning, streaming
// (the attenuations; the spot angles, whose cosines are made on load), comparison, viewer
// strings, the world direction (the world rotation's first column). NiLight's own code (the
// bases called here) is another chunk's, at its addresses.

#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiPointLight.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiPointLight_RTTI, 0x020E2CD0);
GAME_VAR(RTTI, NiSpotLight_RTTI, 0x020E2C9C);

constexpr uintptr_t NiPointLight_vtable = 0x00952FF0;
constexpr uintptr_t NiSpotLight_vtable = 0x00952F20;

inline uint8_t& EffectType(void* light) {
    return At<uint8_t>(light, 0xC5);
}
inline uint32_t& RevisionID(void* light) {
    return At<uint32_t>(light, 0xD0);
}

// NiLight's (the other chunk's)
GAME_FN(void, __thiscall, Light_Destruct, 0x00773820, void* self);
GAME_FN(void, __thiscall, Light_CopyMembers, 0x00773840, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Light_Construct, 0x00773900, void* self);
GAME_FN(void, __thiscall, Light_LoadBinary, 0x007739E0, void* self, void* stream);
GAME_FN(void, __thiscall, Light_SaveBinary, 0x00773A50, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Light_IsEqual, 0x00773AC0, void* self, void* other);
GAME_FN(void, __thiscall, Light_GetViewerStrings, 0x00773B40, void* self, NiTArray<char*>* strings);
// (this chunk's, called where modes apply)
GAME_FN(void, __thiscall, AVObject_UpdateWorldData, 0x00779EB0, void* self);
GAME_FN(NiPointLight*, __thiscall, PointLight_Construct, 0x007751A0, NiPointLight* self);
GAME_FN(void, __thiscall, PointLight_CopyMembers, 0x007751D0, NiPointLight* self, NiPointLight* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, PointLight_IsEqual, 0x00775260, NiPointLight* self, NiPointLight* other);
GAME_FN(void, __thiscall, PointLight_UpdateWorldData, 0x00775300, NiPointLight* self);
GAME_FN(void, __thiscall, PointLight_LoadBinary, 0x007753A0, NiPointLight* self, void* stream);
GAME_FN(void, __thiscall, PointLight_SaveBinary, 0x00775430, NiPointLight* self, void* stream);
GAME_FN(void, __thiscall, PointLight_GetViewerStrings, 0x007754C0, NiPointLight* self, NiTArray<char*>* strings);
GAME_FN(NiSpotLight*, __thiscall, SpotLight_Construct, 0x00774D60, NiSpotLight* self);
GAME_FN(void, __thiscall, SpotLight_CopyMembers, 0x00774C60, NiSpotLight* self, NiSpotLight* dest, void* cloning);

// ---- NiPointLight

NiPointLight* __fastcall PointLightConstruct(NiPointLight* self, re::Edx) {
    Light_Construct(self);
    self->atten0 = 0.0f;
    self->vtable = reinterpret_cast<void*>(NiPointLight_vtable);
    EffectType(self) = 2;
    self->atten1 = 1.0f;
    self->atten2 = 0.0f;
    return self;
}

// the deleting destructor (NiLight's destructor: the point light has nothing of its own)
NiPointLight* __fastcall PointLightDestruct(NiPointLight* self, re::Edx, uint32_t flags) {
    Light_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x130);
    }
    return self;
}

const RTTI* __fastcall PointLightGetRTTI(NiPointLight*, re::Edx) {
    return &NiPointLight_RTTI;
}

// vf30 UpdateWorldData: the light changed
void __fastcall PointLightUpdateWorldData(NiPointLight* self, re::Edx) {
    AVObject_UpdateWorldData(self);
    RevisionID(self) += 1;
}

void __fastcall PointLightCopyMembers(NiPointLight* self, re::Edx, NiPointLight* dest, void* cloning) {
    Light_CopyMembers(self, dest, cloning);
    dest->atten0 = self->atten0;
    dest->atten1 = self->atten1;
    dest->atten2 = self->atten2;
}

// vf3 CreateClone: the constructor and CopyMembers inlined (a failed allocation isn't checked
// past the constructor: the copy writes through null, as the game's does)
NiPointLight* __fastcall PointLightCreateClone(NiPointLight* self, re::Edx, void* cloning) {
    NiPointLight* clone = static_cast<NiPointLight*>(NiNew(0x130));
    if (clone != nullptr) {
        Light_Construct(clone);
        clone->atten0 = 0.0f;
        clone->vtable = reinterpret_cast<void*>(NiPointLight_vtable);
        EffectType(clone) = 2;
        clone->atten1 = 1.0f;
        clone->atten2 = 0.0f;
    } else {
        clone = nullptr;
    }
    Light_CopyMembers(self, clone, cloning);
    clone->atten0 = self->atten0;
    clone->atten1 = self->atten1;
    clone->atten2 = self->atten2;
    return clone;
}

void __fastcall PointLightLoadBinary(NiPointLight* self, re::Edx, void* stream) {
    Light_LoadBinary(self, stream);
    LoadBytes(stream, &self->atten0, 4);
    LoadBytes(stream, &self->atten1, 4);
    LoadBytes(stream, &self->atten2, 4);
}

void __fastcall PointLightSaveBinary(NiPointLight* self, re::Edx, void* stream) {
    Light_SaveBinary(self, stream);
    SaveBytes(stream, &self->atten0, 4);
    SaveBytes(stream, &self->atten1, 4);
    SaveBytes(stream, &self->atten2, 4);
}

uint8_t __fastcall PointLightIsEqual(NiPointLight* self, re::Edx, NiPointLight* other) {
    if (!Light_IsEqual(self, other)) {
        return 0;
    }
    if (self->atten0 != other->atten0 || self->atten1 != other->atten1 || self->atten2 != other->atten2) {
        return 0;
    }
    return 1;
}

void __fastcall PointLightGetViewerStrings(NiPointLight* self, re::Edx, NiTArray<char*>* strings) {
    Light_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiPointLight_RTTI.name));
    AddViewerString(strings, GetViewerStringFloat("m_fAtten0", self->atten0));
    AddViewerString(strings, GetViewerStringFloat("m_fAtten1", self->atten1));
    AddViewerString(strings, GetViewerStringFloat("m_fAtten2", self->atten2));
}

// ---- NiSpotLight

NiSpotLight* __fastcall SpotLightConstruct(NiSpotLight* self, re::Edx) {
    PointLight_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiSpotLight_vtable);
    self->worldDir.x = 1.0f;
    EffectType(self) = 4;
    self->worldDir.y = 0.0f;
    self->worldDir.z = 0.0f;
    self->spotExponent = 1.0f;
    self->cosOuterSpotAngle = 1.0f;
    self->cosInnerSpotAngle = 1.0f;
    self->outerSpotAngle = 0.0f;
    self->innerSpotAngle = 0.0f;
    return self;
}

NiSpotLight* __fastcall SpotLightDestruct(NiSpotLight* self, re::Edx, uint32_t flags) {
    Light_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x150);
    }
    return self;
}

const RTTI* __fastcall SpotLightGetRTTI(NiSpotLight*, re::Edx) {
    return &NiSpotLight_RTTI;
}

// vf30 UpdateWorldData: the direction from the world rotation (the revision bumped once more)
void __fastcall SpotLightUpdateWorldData(NiSpotLight* self, re::Edx) {
    PointLight_UpdateWorldData(self);
    self->worldDir.x = self->world.rotate.m[0][0];
    self->worldDir.y = self->world.rotate.m[1][0];
    self->worldDir.z = self->world.rotate.m[2][0];
    RevisionID(self) += 1;
}

void __fastcall SpotLightCopyMembers(NiSpotLight* self, re::Edx, NiSpotLight* dest, void* cloning) {
    PointLight_CopyMembers(self, dest, cloning);
    dest->worldDir = self->worldDir;
    dest->outerSpotAngle = self->outerSpotAngle;
    dest->innerSpotAngle = self->innerSpotAngle;
    dest->spotExponent = self->spotExponent;
    dest->cosOuterSpotAngle = self->cosOuterSpotAngle;
    dest->cosInnerSpotAngle = self->cosInnerSpotAngle;
}

// vf3 CreateClone (CopyMembers runs on a failed allocation too, with a null clone)
NiSpotLight* __fastcall SpotLightCreateClone(NiSpotLight* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x150);
    NiSpotLight* clone = memory != nullptr ? SpotLight_Construct(static_cast<NiSpotLight*>(memory)) : nullptr;
    SpotLight_CopyMembers(self, clone, cloning);
    return clone;
}

// the cosine of an angle in degrees, as the loader makes it (float steps; pi is the float's)
inline float CosDegrees(float degrees) {
    float radians = degrees * 3.14159274f / 180.0f;
    return float(crt::Cos(radians));
}

// vf4: the inner angle from 20.2.0.5 on; the cosines made, the revision bumped twice
void __fastcall SpotLightLoadBinary(NiSpotLight* self, re::Edx, void* stream) {
    PointLight_LoadBinary(self, stream);
    LoadBytes(stream, &self->outerSpotAngle, 4);
    if (StreamVersion(stream) >= 0x14020005) {
        LoadBytes(stream, &self->innerSpotAngle, 4);
    }
    LoadBytes(stream, &self->spotExponent, 4);
    self->cosOuterSpotAngle = CosDegrees(self->outerSpotAngle);
    float cosInner = CosDegrees(self->innerSpotAngle);
    RevisionID(self) += 2;
    self->cosInnerSpotAngle = cosInner;
}

void __fastcall SpotLightSaveBinary(NiSpotLight* self, re::Edx, void* stream) {
    PointLight_SaveBinary(self, stream);
    SaveBytes(stream, &self->outerSpotAngle, 4);
    SaveBytes(stream, &self->innerSpotAngle, 4);
    SaveBytes(stream, &self->spotExponent, 4);
}

// vf8: outer angle, exponent, inner angle
uint8_t __fastcall SpotLightIsEqual(NiSpotLight* self, re::Edx, NiSpotLight* other) {
    if (!PointLight_IsEqual(self, other)) {
        return 0;
    }
    if (self->outerSpotAngle != other->outerSpotAngle || self->spotExponent != other->spotExponent ||
        self->innerSpotAngle != other->innerSpotAngle) {
        return 0;
    }
    return 1;
}

void __fastcall SpotLightGetViewerStrings(NiSpotLight* self, re::Edx, NiTArray<char*>* strings) {
    PointLight_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiSpotLight_RTTI.name));
    AddViewerString(strings, Point3_GetViewerString(&self->worldDir, "m_kWorldDir"));
    AddViewerString(strings, GetViewerStringFloat("m_fOuterSpotAngle", self->outerSpotAngle));
    AddViewerString(strings, GetViewerStringFloat("m_fInnerSpotAngle", self->innerSpotAngle));
    AddViewerString(strings, GetViewerStringFloat("m_fSpotExponent", self->spotExponent));
}

} // namespace

// the base's LinkObject / RegisterStreamables and NiAmbientLight's LoadBinary: lone jmps
RE_UNHOOKED("NiSpotLight::LinkObject", 0x00774CE0);            // jmp 0x7D54C0
RE_UNHOOKED("NiSpotLight::RegisterStreamables", 0x00774CF0);   // jmp 0x7D54D0
RE_UNHOOKED("NiAmbientLight::LoadBinary", 0x007841D0);         // jmp 0x7739E0 (NiLight's)

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPOINTLIGHT_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIPOINTLIGHT_REVERSED("NiSpotLight::CopyMembers", 0x00774C60, &ni::SpotLightCopyMembers);
NIPOINTLIGHT_REVERSED("NiSpotLight::IsEqual", 0x00774D00, &ni::SpotLightIsEqual);
NIPOINTLIGHT_REVERSED("NiSpotLight::NiSpotLight", 0x00774D60, &ni::SpotLightConstruct);
NIPOINTLIGHT_REVERSED("NiSpotLight::GetRTTI", 0x00774DD0, &ni::SpotLightGetRTTI);
NIPOINTLIGHT_REVERSED("NiSpotLight::~NiSpotLight", 0x00774DE0, &ni::SpotLightDestruct);
NIPOINTLIGHT_REVERSED("NiSpotLight::UpdateWorldData", 0x00774E10, &ni::SpotLightUpdateWorldData);
NIPOINTLIGHT_REVERSED("NiSpotLight::CreateClone", 0x00774E40, &ni::SpotLightCreateClone);
NIPOINTLIGHT_REVERSED("NiSpotLight::LoadBinary", 0x00774EB0, &ni::SpotLightLoadBinary);
NIPOINTLIGHT_REVERSED("NiSpotLight::SaveBinary", 0x00774FC0, &ni::SpotLightSaveBinary);
NIPOINTLIGHT_REVERSED("NiSpotLight::GetViewerStrings", 0x00775050, &ni::SpotLightGetViewerStrings);
NIPOINTLIGHT_REVERSED("NiPointLight::NiPointLight", 0x007751A0, &ni::PointLightConstruct);
NIPOINTLIGHT_REVERSED("NiPointLight::CopyMembers", 0x007751D0, &ni::PointLightCopyMembers);
NIPOINTLIGHT_REVERSED("NiPointLight::IsEqual", 0x00775260, &ni::PointLightIsEqual);
NIPOINTLIGHT_REVERSED("NiPointLight::GetRTTI", 0x007752C0, &ni::PointLightGetRTTI);
NIPOINTLIGHT_REVERSED("NiPointLight::~NiPointLight", 0x007752D0, &ni::PointLightDestruct);
NIPOINTLIGHT_REVERSED("NiPointLight::UpdateWorldData", 0x00775300, &ni::PointLightUpdateWorldData);
NIPOINTLIGHT_REVERSED("NiPointLight::CreateClone", 0x00775320, &ni::PointLightCreateClone);
NIPOINTLIGHT_REVERSED("NiPointLight::LoadBinary", 0x007753A0, &ni::PointLightLoadBinary);
NIPOINTLIGHT_REVERSED("NiPointLight::SaveBinary", 0x00775430, &ni::PointLightSaveBinary);
NIPOINTLIGHT_REVERSED("NiPointLight::GetViewerStrings", 0x007754C0, &ni::PointLightGetViewerStrings);
