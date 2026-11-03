// NiTexture and NiRenderedTexture (src/gamebryo/NiTexture.h): the global texture list, the
// destructors, NiRenderedTexture::Create. Also NiShadowCubeMap's setup (a cube map rendered by
// six shadow render clicks, one per face: Initialize / the release of it all).

#include <cstdint>

#include <windows.h>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiTexture.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTexture_RTTI, 0x020E2184);
GAME_VAR(RTTI, NiRenderedTexture_RTTI, 0x020E24E0);

constexpr uintptr_t NiTexture_vtable = 0x009519B8;
constexpr uintptr_t NiRenderedTexture_vtable = 0x00951A08;

// the list and its lock (a critical section with its owner and depth)
GAME_VAR(NiTexture*, NiTexture_ms_pkHead, 0x020E2178);
GAME_VAR(NiTexture*, NiTexture_ms_pkTail, 0x020E217C);
GAME_VAR(CRITICAL_SECTION, NiTexture_ms_kCritSec, 0x020E21B8);
GAME_VAR(DWORD, NiTexture_ms_kCritSecOwner, 0x020E21D0);
GAME_VAR(uint32_t, NiTexture_ms_kCritSecDepth, 0x020E21D4);

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
GAME_VAR(uint8_t*, NiShadowManager_ms_pkShadowManager, 0x020E2410);

GAME_FN(NiObject*, __cdecl, Ni2DBuffer_Create, 0x007D1E40, uint32_t width, uint32_t height);
GAME_FN(NiObject*, __cdecl, RenderedCubeMap_Create, 0x007D3170, uint32_t size, void* renderer, uintptr_t format);
GAME_FN(NiObject*, __thiscall, ShadowRenderClick_Construct, 0x00518D90, void* self);
GAME_FN(NiObject*, __thiscall, Camera_Construct, 0x00763C20, void* self);
GAME_FN(NiObject*, __thiscall, RenderView_Construct, 0x007561A0, void* self, NiObject* camera, void* culler,
        int alwaysUseCameraViewport);
GAME_FN(void, __thiscall, RenderViewList_AddTail, 0x007843D0, void* list, const re::Ptr32<NiObject>* view);
GAME_FN(NiObject*, __cdecl, ShadowManager_GetDepthStencilBuffer, 0x00758180, NiObject* buffer);
// (this chunk's, called where modes apply)
GAME_FN(void, __thiscall, Texture_AddToList, 0x0075D7D0, NiTexture* self);
GAME_FN(void, __thiscall, Texture_RemoveFromList, 0x0075D850, NiTexture* self);
GAME_FN(NiRenderedTexture*, __thiscall, RenderedTexture_Construct, 0x0075D8E0, void* self);
GAME_FN(NiObject*, __cdecl, RenderTargetGroup_CreateWithDepth, 0x0075D670, NiObject* buffer, void* renderer,
        NiObject* depth);
GAME_FN(void, __cdecl, ShadowCubeMap_Release, 0x00760C00, void* map);

inline void Lock() {
    EnterCriticalSection(re::GameSection(&NiTexture_ms_kCritSec));
    DWORD thread = GetCurrentThreadId();
    NiTexture_ms_kCritSecDepth += 1;
    NiTexture_ms_kCritSecOwner = thread;
}

inline void Unlock() {
    NiTexture_ms_kCritSecDepth -= 1;
    if (NiTexture_ms_kCritSecDepth == 0) {
        NiTexture_ms_kCritSecOwner = 0;
    }
    LeaveCriticalSection(re::GameSection(&NiTexture_ms_kCritSec));
}

// AddToTextureList: at the tail
void __fastcall AddToTextureList(NiTexture* self, re::Edx) {
    Lock();
    if (NiTexture_ms_pkHead == nullptr) {
        NiTexture_ms_pkHead = self;
    }
    if (NiTexture_ms_pkTail != nullptr) {
        NiTexture_ms_pkTail->next = self;
        self->prev = NiTexture_ms_pkTail;
    } else {
        self->prev = nullptr;
    }
    NiTexture_ms_pkTail = self;
    self->next = nullptr;
    Unlock();
}

void __fastcall RemoveFromTextureList(NiTexture* self, re::Edx) {
    Lock();
    if (NiTexture_ms_pkHead == self) {
        NiTexture_ms_pkHead = self->next;
    }
    if (NiTexture_ms_pkTail == self) {
        NiTexture_ms_pkTail = self->prev;
    }
    if (self->prev != nullptr) {
        self->prev->next = self->next;
    }
    if (self->next != nullptr) {
        self->next->prev = self->prev;
    }
    Unlock();
}

const RTTI* __fastcall TextureGetRTTI(NiTexture*, re::Edx) {
    return &NiTexture_RTTI;
}

const RTTI* __fastcall RenderedTextureGetRTTI(NiTexture*, re::Edx) {
    return &NiRenderedTexture_RTTI;
}

// NiRenderedTexture::GetWidth / GetHeight (its slots 18, 19)
uint32_t __fastcall RenderedTextureGetWidth(NiRenderedTexture* self, re::Edx) {
    return self->buffer != nullptr ? At<uint32_t>(self->buffer, 8) : 0;
}

uint32_t __fastcall RenderedTextureGetHeight(NiRenderedTexture* self, re::Edx) {
    return self->buffer != nullptr ? At<uint32_t>(self->buffer, 0xC) : 0;
}

NiRenderedTexture* __fastcall RenderedTextureConstruct(NiRenderedTexture* self, re::Edx) {
    ObjectNET_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTexture_vtable);
    self->pixelLayout = 6;
    self->alphaFormat = 3;
    self->mipMapped = 2;
    self->rendererData = nullptr;
    Texture_AddToList(self);
    self->vtable = reinterpret_cast<void*>(NiRenderedTexture_vtable);
    self->buffer = nullptr;
    return self;
}

// NiRenderedTexture::Create(uiWidth, uiHeight, pkRenderer, kPrefs, ...): power-of-two sizes
// unless the renderer can do others (its flags 8 or 4); null when the renderer can't make it
NiRenderedTexture* __cdecl RenderedTextureCreate(uint32_t width, uint32_t height, void* renderer,
                                                 const uint32_t* prefs, uint32_t arg) {
    if (renderer == nullptr) {
        return nullptr;
    }
    bool powersOfTwo = width != 0 && (width & (width - 1)) == 0 && height != 0 && (height & (height - 1)) == 0;
    if (!powersOfTwo) {
        if ((Virtual<uint8_t>(renderer, 19) & 8) == 0 && (Virtual<uint8_t>(renderer, 19) & 4) == 0) {
            return nullptr;
        }
    }
    void* p = NiNew(0x34);
    NiRenderedTexture* texture = p != nullptr ? RenderedTexture_Construct(p) : nullptr;
    texture->pixelLayout = prefs[0];
    texture->alphaFormat = prefs[1];
    texture->mipMapped = prefs[2];
    NiObject* buffer = Ni2DBuffer_Create(width, height);
    NiObject* old = texture->buffer;
    if (old != buffer) {
        Release(old);
        texture->buffer = buffer;
        AddRef(buffer);
    }
    if (!Virtual<uint8_t>(renderer, 68, texture, arg)) {   // CreateRenderedTextureRendererData
        Virtual<void>(texture, 0, 1);
        return nullptr;
    }
    return texture;
}

NiTexture* __fastcall TextureDeletingDestructor(NiTexture* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiTexture_vtable);
    if (self->rendererData != nullptr) {
        Virtual<void>(self->rendererData, 0, 1);
    }
    Texture_RemoveFromList(self);
    ObjectNET_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x30);
    }
    return self;
}

NiRenderedTexture* __fastcall RenderedTextureDeletingDestructor(NiRenderedTexture* self, re::Edx, uint32_t flags) {
    Release(self->buffer);
    self->vtable = reinterpret_cast<void*>(NiTexture_vtable);
    if (self->rendererData != nullptr) {
        Virtual<void>(self->rendererData, 0, 1);
    }
    Texture_RemoveFromList(self);
    ObjectNET_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x34);
    }
    return self;
}

// ---- NiShadowCubeMap's setup. The map: flags +0x4 (NiTexturingProperty::Map's: clamp mode
// bits 12-13, filter 8-11), the cube map texture +0x8, a reference +0xC, the six faces' render
// clicks +0x10 (NiShadowRenderClick, 0x88 bytes: its material swap processor +0xC, render
// target group +0x10, valid flags +0x34, render views +0x4C)

struct ShadowCubeMap {
    uint32_t unknown0;
    uint16_t flags;          // +0x4
    uint16_t unknown6;
    re::Ptr32<NiObject> texture;       // +0x8
    re::Ptr32<NiObject> unknownC;      // +0xC
    re::Ptr32<NiObject> clicks[6];     // +0x10
};
static_assert(offsetof(ShadowCubeMap, clicks) == 0x10);

void __cdecl ShadowCubeMapRelease(ShadowCubeMap* map) {
    for (re::Ptr32<NiObject>& click : map->clicks) {
        if (click != nullptr) {
            NiObject* c = click;
            Release(c);
            click = nullptr;
        }
    }
    map->flags = 0;
    if (map->texture != nullptr) {
        Release(map->texture);
        map->texture = nullptr;
    }
    if (map->unknownC != nullptr) {
        Release(map->unknownC);
        map->unknownC = nullptr;
    }
}

// Initialize(map, size, format, clamp mode, filter mode): the cube map and a click per face
// (each with a camera's render view and a render target group on its face); everything
// released when a part fails
uint8_t __cdecl ShadowCubeMapInitialize(ShadowCubeMap* map, uint32_t size, uint32_t format, uint32_t clamp,
                                        uint32_t filter) {
    ShadowCubeMap_Release(map);
    map->flags = uint16_t((map->flags & 0xFFF1) | 1);
    void* renderer = NiRenderer_ms_pkRenderer;
    NiObject* cube = RenderedCubeMap_Create(size, renderer, format);
    NiObject* old = map->texture;
    if (old != cube) {
        Release(old);
        map->texture = cube;
        AddRef(cube);
    }
    if (map->texture == nullptr) {
        ShadowCubeMap_Release(map);
        return 0;
    }
    for (uint8_t face = 0; face < 6; face++) {
        void* p = NiNew(0x88);
        NiObject* click = p != nullptr ? ShadowRenderClick_Construct(p) : nullptr;
        At<uint32_t>(click, 0x34) = 7;
        // the shadow manager's material swap processor
        NiObject* processor = At<NiObject*>(NiShadowManager_ms_pkShadowManager, 0xD4);
        AddRef(processor);
        re::Ptr32<NiObject>& slot = At<NiObject*>(click, 0xC);
        if (slot != processor) {
            Release(slot);
            slot = processor;
            AddRef(processor);
        }
        Release(processor);
        // a render view of a new camera, with the shadow manager's culler
        void* q = NiNew(0x3C);
        NiObject* view = nullptr;
        if (q != nullptr) {
            void* c = NiNew(0x13C);
            NiObject* camera = c != nullptr ? Camera_Construct(c) : nullptr;
            view = RenderView_Construct(q, camera, At<void*>(NiShadowManager_ms_pkShadowManager, 0xC4), 1);
        }
        re::Ptr32<NiObject> held = view;
        AddRef(view);
        RenderViewList_AddTail(static_cast<uint8_t*>(static_cast<void*>(click)) + 0x4C, &held);
        Release(view);
        // the face's render target group
        NiObject* buffer = At<NiObject*>(cube, 0x38 + face * 4);
        NiObject* depth = ShadowManager_GetDepthStencilBuffer(buffer);
        if (depth == nullptr) {
            ShadowCubeMap_Release(map);
            return 0;
        }
        NiObject* group = RenderTargetGroup_CreateWithDepth(buffer, renderer, depth);
        re::Ptr32<NiObject>& groupSlot = At<NiObject*>(click, 0x10);
        if (groupSlot != group) {
            Release(groupSlot);
            groupSlot = group;
            if (group == nullptr) {
                ShadowCubeMap_Release(map);
                return 0;
            }
            AddRef(group);
        }
        if (group == nullptr) {
            ShadowCubeMap_Release(map);
            return 0;
        }
        re::Ptr32<NiObject>& clickSlot = map->clicks[face];
        if (clickSlot != click) {
            Release(clickSlot);
            clickSlot = click;
            AddRef(click);
        }
    }
    uint16_t flags = uint16_t((map->flags & 0xCFFF) | uint16_t(clamp << 12));
    map->flags = uint16_t((flags & 0xF0FF) | uint16_t(uint8_t(filter) << 8));
    return 1;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NITEX_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NITEX_REVERSED("NiTexture::AddToTextureList", 0x0075D7D0, &ni::AddToTextureList);
NITEX_REVERSED("NiTexture::RemoveFromTextureList", 0x0075D850, &ni::RemoveFromTextureList);
NITEX_REVERSED("NiTexture::GetRTTI", 0x0075D8D0, &ni::TextureGetRTTI);
NITEX_REVERSED("NiRenderedTexture::NiRenderedTexture", 0x0075D8E0, &ni::RenderedTextureConstruct);
NITEX_REVERSED("NiRenderedTexture::GetRTTI", 0x0075D930, &ni::RenderedTextureGetRTTI);
NITEX_REVERSED("NiRenderedTexture::GetWidth", 0x0075D940, &ni::RenderedTextureGetWidth);
NITEX_REVERSED("NiRenderedTexture::GetHeight", 0x0075D950, &ni::RenderedTextureGetHeight);
NITEX_REVERSED("NiRenderedTexture::Create", 0x0075D960, &ni::RenderedTextureCreate);
NITEX_REVERSED("NiTexture::DeletingDestructor", 0x0075DA60, &ni::TextureDeletingDestructor);
NITEX_REVERSED("NiRenderedTexture::DeletingDestructor", 0x0075DAA0, &ni::RenderedTextureDeletingDestructor);
NITEX_REVERSED("NiShadowCubeMap::Release", 0x00760C00, &ni::ShadowCubeMapRelease);
NITEX_REVERSED("NiShadowCubeMap::Initialize", 0x00760CA0, &ni::ShadowCubeMapInitialize);
