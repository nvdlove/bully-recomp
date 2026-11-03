// NiDynamicTexture (vtable 0x953718, 0x3C bytes): a texture the CPU writes through Lock/UnLock,
// its pixels the renderer's (its renderer data: width +0xC, height +0x10). Created through Create
// (power-of-two sizes unless the renderer's flags allow others), which has the renderer make the
// data.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiTexture.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiDynamicTexture : NiTexture {
    uint32_t width;      // +0x30
    uint32_t height;     // +0x34
    uint8_t tiled;       // +0x38
    uint8_t pad39[3];
};
static_assert(offsetof(NiDynamicTexture, width) == 0x30);
static_assert(sizeof(NiDynamicTexture) == 0x3C);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiDynamicTexture_RTTI, 0x020E2FA0);
constexpr uintptr_t NiTexture_vtable = 0x009519B8;
constexpr uintptr_t NiDynamicTexture_vtable = 0x00953718;

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);

// NiTexture's (another chunk's): onto / off the global texture list
GAME_FN(void, __thiscall, Texture_AddToList, 0x0075D7D0, NiTexture* self);
GAME_FN(void, __thiscall, Texture_RemoveFromList, 0x0075D850, NiTexture* self);
// (this file's, called where modes apply)
GAME_FN(NiDynamicTexture*, __thiscall, DynamicTexture_Construct, 0x00779200, NiDynamicTexture* self,
        uint32_t width, uint32_t height);

// vf20 Lock(pitch&): the renderer's LockDynamicTexture (slot 73)
void* __fastcall Lock(NiDynamicTexture* self, re::Edx, int* pitch) {
    *pitch = 0;
    return Virtual<void*>(NiRenderer_ms_pkRenderer, 73, self->rendererData, pitch);
}

// vf21 UnLock: the renderer's UnLockDynamicTexture (slot 74)
uint8_t __fastcall UnLock(NiDynamicTexture* self, re::Edx) {
    return Virtual<uint8_t>(NiRenderer_ms_pkRenderer, 74, self->rendererData);
}

// NiDynamicTexture(width, height): NiTexture's constructor inlined (format preferences 6, 3, 2)
NiDynamicTexture* __fastcall Construct(NiDynamicTexture* self, re::Edx, uint32_t width, uint32_t height) {
    ObjectNET_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTexture_vtable);
    self->pixelLayout = 6;
    self->alphaFormat = 3;
    self->mipMapped = 2;
    self->rendererData = nullptr;
    Texture_AddToList(self);
    self->width = width;
    self->vtable = reinterpret_cast<void*>(NiDynamicTexture_vtable);
    self->height = height;
    self->tiled = 0;
    return self;
}

const RTTI* __fastcall GetRTTI(NiDynamicTexture*, re::Edx) {
    return &NiDynamicTexture_RTTI;
}

// vf18 GetWidth, vf19 GetHeight: the renderer data's when there is one
uint32_t __fastcall GetWidth(NiDynamicTexture* self, re::Edx) {
    if (self->rendererData != nullptr) {
        return At<uint32_t>(self->rendererData, 0xC);
    }
    return self->width;
}

uint32_t __fastcall GetHeight(NiDynamicTexture* self, re::Edx) {
    if (self->rendererData != nullptr) {
        return At<uint32_t>(self->rendererData, 0x10);
    }
    return self->height;
}

// Create(width, height, prefs, tiled): none without a renderer, for sizes it can't take, or when
// it can't make the data (the texture deleted then; a failed allocation isn't checked)
NiDynamicTexture* __cdecl Create(uint32_t width, uint32_t height, const uint32_t* prefs, int tiled) {
    void* renderer = NiRenderer_ms_pkRenderer;
    if (renderer == nullptr) {
        return nullptr;
    }
    bool powersOfTwo = width != 0 && (width & (width - 1)) == 0 && height != 0 && (height & (height - 1)) == 0;
    if (!powersOfTwo && (Virtual<uint32_t>(renderer, 19) & 8) == 0 && (Virtual<uint32_t>(renderer, 19) & 4) == 0) {
        return nullptr;
    }
    void* memory = NiNew(0x3C);
    NiDynamicTexture* texture =
        memory != nullptr ? DynamicTexture_Construct(static_cast<NiDynamicTexture*>(memory), width, height) : nullptr;
    texture->pixelLayout = prefs[0];
    texture->alphaFormat = prefs[1];
    texture->mipMapped = prefs[2];
    texture->tiled = uint8_t(tiled);
    if (!Virtual<uint8_t>(renderer, 71, texture)) {   // CreateDynamicTextureRendererData
        Virtual<void*>(texture, 0, 1u);
        return nullptr;
    }
    return texture;
}

// the deleting destructor: the renderer data deleted, off the texture list
NiDynamicTexture* __fastcall DeletingDestructor(NiDynamicTexture* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiTexture_vtable);
    if (self->rendererData != nullptr) {
        Virtual<void*>(self->rendererData, 0, 1u);
    }
    Texture_RemoveFromList(self);
    ObjectNET_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3C);
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIDYNTEX_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIDYNTEX_REVERSED("NiDynamicTexture::Lock", 0x007791B0, &ni::Lock);
NIDYNTEX_REVERSED("NiDynamicTexture::UnLock", 0x007791E0, &ni::UnLock);
NIDYNTEX_REVERSED("NiDynamicTexture::NiDynamicTexture", 0x00779200, &ni::Construct);
NIDYNTEX_REVERSED("NiDynamicTexture::GetRTTI", 0x00779250, &ni::GetRTTI);
NIDYNTEX_REVERSED("NiDynamicTexture::GetWidth", 0x00779260, &ni::GetWidth);
NIDYNTEX_REVERSED("NiDynamicTexture::GetHeight", 0x00779270, &ni::GetHeight);
NIDYNTEX_REVERSED("NiDynamicTexture::Create", 0x00779280, &ni::Create);
NIDYNTEX_REVERSED("NiDynamicTexture::DeletingDestructor", 0x00779340, &ni::DeletingDestructor);
