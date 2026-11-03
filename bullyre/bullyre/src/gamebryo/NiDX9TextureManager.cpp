// NiDX9TextureManager (vtable 0x99D8F8, 6 slots, an NiRefObject; 0x10 bytes): the DX9 renderer's
// texture side. +0x8 the D3D device (held: AddRef'd), +0xC the renderer. Slots: 0 the destructor,
// 1 DeleteThis, 2 (returns 0), 3 PrepareTextureForRendering, 4 PrecacheTexture, 5
// GetFormatFromTexture (Gamebryo 2.x names; there they're plain methods).
//
// The source textures' renderer data is made on demand here (NiDX9SourceTextureData::Create)
// under the renderer's precache lock (its +0x70).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9RendererData.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kTextureManagerVtable = 0x0099D8F8;
constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uint32_t kTextureManagerSize = 0x10;
constexpr uint32_t D3DFMT_X8R8G8B8 = 0x16;
GAME_VAR(volatile long, TextureManager_RefObjectCount, 0x020E2168);   // NiRefObject::ms_uiObjects
GAME_VAR(uint8_t, TextureManager_NiSourceTextureRTTI, 0x020E2A10);   // NiSourceTexture::ms_RTTI
GAME_VAR(uint32_t, TextureManager_NiSourceTextureClassBit, 0x020E2A18);   // (its +8)

GAME_FN(uint8_t*, __cdecl, TextureManager_IsKindOf, 0x00401FF0, void* rtti, void* object);
GAME_FN(uint8_t*, __cdecl, TextureManager_CreateSourceTextureData, 0x008968F0, void* texture, void* renderer);

inline ni::sys::NiCriticalSection& PrecacheLock() {
    return At<ni::sys::NiCriticalSection>(Renderer(), ni::dx9::kPrecacheLock);
}

// the texture's renderer data, a source texture's made when it has none (`made` set then); null
// when it has none and isn't a source texture. Under the precache lock.
uint8_t* TextureManager_GetOrCreateData(uint8_t* self, uint8_t* texture, uint8_t* made) {
    ni::sys::NiCriticalSection& lock = PrecacheLock();
    ni::sys::Lock(lock);
    uint8_t* data = At<uint8_t*>(texture, 0x24);
    if (data == nullptr) {
        uint8_t* source = TextureManager_IsKindOf(&TextureManager_NiSourceTextureRTTI, texture);
        if (source == nullptr) {
            ni::sys::Unlock(lock);
            return nullptr;
        }
        data = TextureManager_CreateSourceTextureData(source, At<void*>(self, 0xC));
        if (made != nullptr) {
            *made = 1;
        }
    }
    ni::sys::Unlock(lock);
    return data;
}

} // namespace

// 0x89A2E0 (thiscall, slot 2): six arguments, 0 (5 bytes: xor eax, eax / ret 0x18)
uint32_t __fastcall TextureManager_Slot2(void*, re::Edx, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    return 0;
}

// 0x89A2F0 (thiscall): NiDX9TextureManager::NiDX9TextureManager (the renderer's device held)
uint8_t* __fastcall NiDX9TextureManager_Construct(uint8_t* self, re::Edx, uint8_t* renderer) {
    At<uint32_t>(self, 0) = kRefObjectVtable;
    At<uint32_t>(self, 4) = 0;
    ni::sys::Increment(const_cast<long*>(&TextureManager_RefObjectCount));
    At<uint32_t>(self, 0) = kTextureManagerVtable;
    At<uint8_t*>(self, 0xC) = renderer;
    void* device = At<void*>(renderer, ni::dx9::kDevice);
    At<void*>(self, 8) = device;
    Com<unsigned long>(device, 1);   // AddRef
    return self;
}

// 0x89A330 (thiscall, slot 0): the deleting destructor (the device let go)
uint8_t* __fastcall TextureManager_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kTextureManagerVtable;
    Com<unsigned long>(At<void*>(self, 8), 2);   // Release
    At<void*>(self, 8) = nullptr;
    At<uint32_t>(self, 0) = kRefObjectVtable;
    ni::sys::Decrement(const_cast<long*>(&TextureManager_RefObjectCount));
    if ((flags & 1) != 0) {
        NiDelete(self, kTextureManagerSize);
    }
    return self;
}

// 0x89A380 (thiscall, slot 3): NiDX9TextureManager::PrepareTextureForRendering (the texture's D3D
// texture: its renderer data made when it has none (`changed`); a size not a power of two
// (`nonPow2`); data marked for update (+0x62 bit 0) updated (slot 19) when it has no D3D texture
// yet (`changed`) or the texture isn't static (+0x40 0); `mipmap` when it has more than one level)
void* __fastcall TextureManager_PrepareTextureForRendering(uint8_t* self, re::Edx, uint8_t* texture, uint8_t* changed,
                                                           uint8_t* mipmap, uint8_t* nonPow2) {
    *changed = 0;
    *mipmap = 0;
    *nonPow2 = 0;
    if (texture == nullptr) {
        return nullptr;
    }
    uint8_t* data = TextureManager_GetOrCreateData(self, texture, changed);
    if (data == nullptr) {
        return nullptr;
    }
    // (read under the lock in the game; nothing else between)
    uint32_t width = At<uint32_t>(data, 0xC);
    uint32_t height = At<uint32_t>(data, 0x10);
    if (width == 0 || (width & (width - 1)) != 0 || height == 0 || (height & (height - 1)) != 0) {
        *nonPow2 = 1;
    }
    if ((At<uint8_t>(data, 0x62) & 1) != 0) {
        if (At<void*>(data, 0x5C) == nullptr) {
            Virtual<void>(data, 19);
            *changed = 1;
        } else if (At<uint8_t>(texture, 0x40) == 0) {
            Virtual<void>(data, 19);
        }
    }
    *mipmap = At<uint16_t>(data, 0x60) > 1 ? 1 : 0;
    return At<void*>(data, 0x5C);
}

// 0x89A4A0 (thiscall, slot 4): NiDX9TextureManager::PrecacheTexture (the renderer data made when
// it has none; a rendered (+0x62 bit 1) or dynamic (bit 2) texture's: whether it has its D3D
// texture; else true. The two flags unused.)
uint8_t __fastcall TextureManager_PrecacheTexture(uint8_t* self, re::Edx, uint8_t* texture, uint8_t, uint8_t) {
    if (texture == nullptr) {
        return 0;
    }
    uint8_t* data = TextureManager_GetOrCreateData(self, texture, nullptr);
    if (data == nullptr) {
        return 0;
    }
    uint8_t flags = At<uint8_t>(data, 0x62);
    if (((flags >> 1) & 1) != 0) {
        return At<void*>(data, 0x5C) != nullptr ? 1 : 0;
    }
    if (((flags >> 2) & 1) != 0) {
        return At<void*>(data, 0x5C) != nullptr ? 1 : 0;
    }
    return 1;
}

// 0x89A580 (thiscall, slot 5): NiDX9TextureManager::GetFormatFromTexture (the renderer data's
// D3D format (+0x24); a source texture without data has it made by the renderer (slot 67,
// CreateSourceTextureRendererData) first; else X8R8G8B8)
uint32_t __fastcall TextureManager_GetFormatFromTexture(uint8_t* self, re::Edx, uint8_t* texture) {
    if (At<uint8_t*>(texture, 0x24) != nullptr) {
        return At<uint32_t>(At<uint8_t*>(texture, 0x24), 0x24);
    }
    if (ni::dx9::HasClassBit(texture, TextureManager_NiSourceTextureClassBit)) {
        if (Virtual<uint8_t>(At<void*>(self, 0xC), 67, texture) != 0 && At<uint8_t*>(texture, 0x24) != nullptr) {
            return At<uint32_t>(At<uint8_t*>(texture, 0x24), 0x24);
        }
    }
    return D3DFMT_X8R8G8B8;
}

#define C40O_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40O_REVERSED("NiDX9TextureManager::vf2", 0x0089A2E0, TextureManager_Slot2);
C40O_REVERSED("NiDX9TextureManager::NiDX9TextureManager", 0x0089A2F0, NiDX9TextureManager_Construct);
C40O_REVERSED("NiDX9TextureManager::DeletingDestructor", 0x0089A330, TextureManager_DeletingDestructor);
C40O_REVERSED("NiDX9TextureManager::PrepareTextureForRendering", 0x0089A380,
              TextureManager_PrepareTextureForRendering);
C40O_REVERSED("NiDX9TextureManager::PrecacheTexture", 0x0089A4A0, TextureManager_PrecacheTexture);
C40O_REVERSED("NiDX9TextureManager::GetFormatFromTexture", 0x0089A580, TextureManager_GetFormatFromTexture);

} // namespace ni::c40
