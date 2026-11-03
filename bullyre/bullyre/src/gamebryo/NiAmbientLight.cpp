// NiAmbientLight (vtable 0x954C68, 0x124 bytes): an NiLight of type 0 with nothing of its own:
// its constructor, RTTI, deleting destructor, CreateClone and viewer strings (NiLight's, other
// chunks'). Its CreateObject is NiCreators.cpp's.

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
GAME_VAR(RTTI, NiAmbientLight_RTTI, 0x020E3480);

constexpr uintptr_t NiAmbientLight_vtable = 0x00954C68;

GAME_FN(void, __thiscall, Light_Destruct, 0x00773820, void* self);
GAME_FN(void, __thiscall, Light_CopyMembers, 0x00773840, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Light_Construct, 0x00773900, void* self);
GAME_FN(void, __thiscall, Light_GetViewerStrings, 0x00773B40, void* self, NiTArray<char*>* strings);

} // namespace

// 0x784180: the constructor (type 0)
NiLight* __fastcall NiAmbientLight_Construct(NiLight* self, re::Edx) {
    Light_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiAmbientLight_vtable);
    self->effectType = 0;
    return self;
}

// 0x7841E0: GetRTTI (slot 2)
const RTTI* __fastcall NiAmbientLight_GetRTTI(NiLight*, re::Edx) {
    return &NiAmbientLight_RTTI;
}

// 0x7841F0: the deleting destructor (slot 0)
NiLight* __fastcall NiAmbientLight_DeletingDestructor(NiLight* self, re::Edx, uint32_t flags) {
    Light_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiLight));
    }
    return self;
}

// 0x784220: CreateClone (slot 3); NiLight's CopyMembers on a failed NiNew too, as the game has it
NiLight* __fastcall NiAmbientLight_CreateClone(NiLight* self, re::Edx, void* cloning) {
    NiLight* clone = static_cast<NiLight*>(NiNew(sizeof(NiLight)));
    if (clone != nullptr) {
        Light_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiAmbientLight_vtable);
        clone->effectType = 0;
        Light_CopyMembers(self, clone, cloning);
        return clone;
    }
    Light_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

// 0x784280: GetViewerStrings (slot 9)
void __fastcall NiAmbientLight_GetViewerStrings(NiLight* self, re::Edx, NiTArray<char*>* strings) {
    Light_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiAmbientLight_RTTI.name));
}

#define NIAMBIENT_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIAMBIENT_REVERSED("NiAmbientLight::NiAmbientLight", 0x00784180, NiAmbientLight_Construct);
NIAMBIENT_REVERSED("NiAmbientLight::GetRTTI", 0x007841E0, NiAmbientLight_GetRTTI);
NIAMBIENT_REVERSED("NiAmbientLight::DeletingDestructor", 0x007841F0, NiAmbientLight_DeletingDestructor);
NIAMBIENT_REVERSED("NiAmbientLight::CreateClone", 0x00784220, NiAmbientLight_CreateClone);
NIAMBIENT_REVERSED("NiAmbientLight::GetViewerStrings", 0x00784280, NiAmbientLight_GetViewerStrings);

} // namespace ni
