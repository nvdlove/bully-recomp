// The stream factories (CreateObject: what NiStream::RegisterLoader is given) and default
// constructors the first passes left for classes written elsewhere: NiSkinData
// (src/gamebryo/NiSkinData.cpp) and its BoneData, NiScreenTexture (NiScreenTexture.cpp),
// NiScreenLODData (NiScreenLODData.cpp), NiPersistentSrcTextureRendererData
// (NiPersistentSrcTextureRendererData.cpp).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using ni::scene::At;
using ni::scene::RefObject_Construct;

constexpr uintptr_t NiSkinData_vtable = 0x00961168;
constexpr uintptr_t NiScreenTexture_vtable = 0x009611B0;
constexpr uintptr_t TextureRendererData_vtable = 0x00961358;   // NiTexture::RendererData
constexpr uintptr_t NiPersistentSrcTextureRendererData_vtable = 0x009613A0;

GAME_FN(void*, __thiscall, ScreenLODData_Construct, 0x007DC040, void* self);
GAME_FN(void*, __thiscall, PixelFormat_Construct, 0x0076B280, void* self);
// (this file's, called where modes apply)
GAME_FN(void*, __thiscall, PersistentData_Construct, 0x007DD480, void* self);

// NiSkinData::BoneData(): no vertex weights
void* __fastcall BoneDataConstruct(uint8_t* self, re::Edx) {
    At<void*>(self, 0x44) = nullptr;
    At<uint16_t>(self, 0x48) = 0;
    return self;
}

// NiSkinData's factory: no partition, no bones (the bone count left as allocated, as the game has it)
void* __cdecl SkinDataCreateObject() {
    uint8_t* object = static_cast<uint8_t*>(NiNew(0x48));
    if (object == nullptr) {
        return nullptr;
    }
    RefObject_Construct(object);
    *reinterpret_cast<re::Ptr32<void>*>(object) = reinterpret_cast<void*>(NiSkinData_vtable);
    At<void*>(object, 0x8) = nullptr;
    At<void*>(object, 0x40) = nullptr;
    return object;
}

// NiScreenTexture's factory: everything empty
void* __cdecl ScreenTextureCreateObject() {
    uint8_t* object = static_cast<uint8_t*>(NiNew(0x20));
    if (object == nullptr) {
        return nullptr;
    }
    RefObject_Construct(object);
    *reinterpret_cast<re::Ptr32<void>*>(object) = reinterpret_cast<void*>(NiScreenTexture_vtable);
    At<uint32_t>(object, 0x8) = 0;
    At<uint32_t>(object, 0xC) = 0;
    At<uint32_t>(object, 0x10) = 0;
    At<uint32_t>(object, 0x14) = 0;
    At<uint16_t>(object, 0x18) = 0;
    At<uint32_t>(object, 0x1C) = 0;
    return object;
}

void* __cdecl ScreenLODDataCreateObject() {
    void* memory = NiNew(0x30);
    if (memory == nullptr) {
        return nullptr;
    }
    return ScreenLODData_Construct(memory);
}

// NiPersistentSrcTextureRendererData(): NiTexture::RendererData's (an NiObject and its pixel
// format), then everything empty
void* __fastcall PersistentDataConstruct(uint8_t* self, re::Edx) {
    RefObject_Construct(self);
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(TextureRendererData_vtable);
    PixelFormat_Construct(self + 0x14);
    for (int offset = 0x58; offset <= 0x84; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    *reinterpret_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(NiPersistentSrcTextureRendererData_vtable);
    At<void*>(self, 0x88) = nullptr;
    At<void*>(self, 0x8) = nullptr;
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0x10) = 0;
    return self;
}

void* __cdecl PersistentDataCreateObject() {
    void* memory = NiNew(0x8C);
    if (memory == nullptr) {
        return nullptr;
    }
    return PersistentData_Construct(memory);
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIFACTORIES_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIFACTORIES_REVERSED("NiSkinData::BoneData::BoneData", 0x007DB100, ni::BoneDataConstruct);
NIFACTORIES_REVERSED("NiSkinData::CreateObject", 0x007DB2E0, ni::SkinDataCreateObject);
NIFACTORIES_REVERSED("NiScreenTexture::CreateObject", 0x007DBDC0, ni::ScreenTextureCreateObject);
NIFACTORIES_REVERSED("NiScreenLODData::CreateObject", 0x007DC2A0, ni::ScreenLODDataCreateObject);
NIFACTORIES_REVERSED("NiPersistentSrcTextureRendererData::NiPersistentSrcTextureRendererData", 0x007DD480,
                     ni::PersistentDataConstruct);
NIFACTORIES_REVERSED("NiPersistentSrcTextureRendererData::CreateObject", 0x007DD580, ni::PersistentDataCreateObject);
