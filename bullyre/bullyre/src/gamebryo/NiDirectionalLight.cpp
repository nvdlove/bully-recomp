// NiDirectionalLight (vtable 0x960910, 0x130 bytes): an NiLight (type 3) with its world
// direction (+0x124: the world rotation's first column, kept by UpdateWorldData). Its slots
// 5..8 are lone jumps to NiLight's (shared with NiPointLight and NiAmbientLight). Also
// NiPoint3::Cross, which the linker put after it.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDynamicEffect.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiDirectionalLight_RTTI, 0x020E39AC);

constexpr uintptr_t kVtable = 0x00960910;

struct NiDirectionalLight : NiLight {
    NiPoint3 worldDir;   // +0x124 (1, 0, 0)
};
static_assert(offsetof(NiDirectionalLight, worldDir) == 0x124);
static_assert(sizeof(NiDirectionalLight) == 0x130);

GAME_FN(NiLight*, __thiscall, Light_Construct, 0x00773900, void* self);
GAME_FN(void, __thiscall, Light_CopyMembers, 0x00773840, NiLight* self, NiLight* dest, void* cloning);
GAME_FN(void, __thiscall, Light_GetViewerStrings, 0x00773B40, NiLight* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, AVObject_UpdateWorldData, 0x00779EB0, NiAVObject* self);
// (this file's, called where modes apply)
GAME_FN(NiDirectionalLight*, __thiscall, DirectionalLight_Construct, 0x007D5500, void* self);

// 0x7D5500
NiDirectionalLight* __fastcall ConstructImpl(NiDirectionalLight* self, re::Edx) {
    Light_Construct(self);
    self->vtable = reinterpret_cast<void*>(kVtable);
    self->worldDir.x = 1.0f;
    self->effectType = 3;
    self->worldDir.y = 0.0f;
    self->worldDir.z = 0.0f;
    return self;
}

// 0x7D5550 vf2
const RTTI* __fastcall GetRTTIImpl(NiDirectionalLight*, re::Edx) {
    return &NiDirectionalLight_RTTI;
}

// 0x7D5560 vf30 UpdateWorldData: NiAVObject's, then the direction and a new revision
void __fastcall UpdateWorldDataImpl(NiDirectionalLight* self, re::Edx) {
    AVObject_UpdateWorldData(self);
    self->worldDir.x = self->world.rotate.m[0][0];
    self->worldDir.y = self->world.rotate.m[1][0];
    self->worldDir.z = self->world.rotate.m[2][0];
    self->revisionID++;
}

// 0x7D5590 vf3 CreateClone(cloning) (no allocation check)
NiDirectionalLight* __fastcall CreateCloneImpl(NiDirectionalLight* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x130);
    NiDirectionalLight* clone = memory != nullptr ? DirectionalLight_Construct(memory) : nullptr;
    Light_CopyMembers(self, clone, cloning);
    clone->worldDir = self->worldDir;
    return clone;
}

// 0x7D5610 vf9 GetViewerStrings
void __fastcall GetViewerStringsImpl(NiDirectionalLight* self, re::Edx, NiTArray<char*>* strings) {
    Light_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiDirectionalLight_RTTI.name));
}

// 0x7D5660 NiPoint3::Cross(other) (the result by value)
NiPoint3* __fastcall Point3_CrossImpl(const NiPoint3* self, re::Edx, NiPoint3* result, const NiPoint3* other) {
    result->x = other->z * self->y - other->y * self->z;
    result->y = self->z * other->x - self->x * other->z;
    result->z = other->y * self->x - other->x * self->y;
    return result;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIDIRLIGHT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

// lone jumps to NiLight's slots (NiPointLight, NiAmbientLight and NiDirectionalLight's vf5..8)
RE_UNHOOKED("NiLight::LinkObject (jump)", 0x007D54C0);           // -> 0x7738D0
RE_UNHOOKED("NiLight::RegisterStreamables (jump)", 0x007D54D0);  // -> 0x7738E0
RE_UNHOOKED("NiLight::SaveBinary (jump)", 0x007D54E0);           // -> 0x773A50
RE_UNHOOKED("NiLight::IsEqual (jump)", 0x007D54F0);              // -> 0x773AC0
NIDIRLIGHT_REVERSED("NiDirectionalLight::NiDirectionalLight", 0x007D5500, ni::ConstructImpl);
NIDIRLIGHT_REVERSED("NiDirectionalLight::GetRTTI", 0x007D5550, ni::GetRTTIImpl);
NIDIRLIGHT_REVERSED("NiDirectionalLight::UpdateWorldData", 0x007D5560, ni::UpdateWorldDataImpl);
NIDIRLIGHT_REVERSED("NiDirectionalLight::CreateClone", 0x007D5590, ni::CreateCloneImpl);
NIDIRLIGHT_REVERSED("NiDirectionalLight::GetViewerStrings", 0x007D5610, ni::GetViewerStringsImpl);
NIDIRLIGHT_REVERSED("NiPoint3::Cross", 0x007D5660, ni::Point3_CrossImpl);
