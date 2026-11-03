// NiRenderTargetGroup (vtable 0x951920, 0x34 bytes): up to 8 colour buffers and a depth/stencil
// buffer the renderer draws into together, with the renderer's own data for the group.
//
// Layout: NiObject (8), the buffers (+0x8, Ni2DBufferPtr[8]), their count (+0x28), the depth
// buffer (+0x2C, NiDepthStencilBufferPtr), the renderer data (+0x30, deleted through its slot 0).
// An Ni2DBuffer: width +0x8, height +0xC, renderer data +0x10 (its pixel format +0xC).
//
// Virtual slots: 18/19 GetWidth/GetHeight(i), 20/21 the depth buffer's, 22 GetPixelFormat(i),
// 23 GetDepthStencilPixelFormat, 25 AttachBuffer, 26 AttachDepthStencilBuffer, 27 GetBuffer,
// 28 GetDepthStencilBuffer, 30 SetRendererData, 31/32 the buffers' renderer data.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {

struct NiRenderTargetGroup : NiObject {
    re::Ptr32<NiObject> buffers[8];      // +0x8
    uint32_t count;            // +0x28
    re::Ptr32<NiObject> depthStencil;    // +0x2C
    re::Ptr32<void> rendererData;        // +0x30
};
static_assert(offsetof(NiRenderTargetGroup, buffers) == 0x8);
static_assert(offsetof(NiRenderTargetGroup, count) == 0x28);
static_assert(offsetof(NiRenderTargetGroup, depthStencil) == 0x2C);
static_assert(sizeof(NiRenderTargetGroup) == 0x34);

namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiRenderTargetGroup_RTTI, 0x020E24AC);
constexpr uintptr_t NiRenderTargetGroup_vtable = 0x00951920;

using Group = NiRenderTargetGroup;

GAME_FN(NiObject*, __cdecl, DepthStencilBuffer_CreateCompatible, 0x007ABF40, NiObject* buffer, void* renderer);
// (this chunk's, called where modes apply)
GAME_FN(Group*, __thiscall, Group_Construct, 0x0075D410, void* self);
GAME_FN(void, __thiscall, Group_Destructor, 0x0075D460, Group* self);
GAME_FN(Group*, __cdecl, Group_CreateWithDepth, 0x0075D670, NiObject* buffer, void* renderer, NiObject* depth);

// vf30 SetRendererData: the old data deleted
void __fastcall SetRendererData(Group* self, re::Edx, void* data) {
    if (self->rendererData != nullptr) {
        Virtual<void>(self->rendererData, 0, 1);
    }
    self->rendererData = data;
}

// vf18 / vf19
uint32_t __fastcall GetWidth(Group* self, re::Edx, uint32_t index) {
    if (index < self->count && self->buffers[index] != nullptr) {
        return At<uint32_t>(self->buffers[index], 8);
    }
    return 0;
}

uint32_t __fastcall GetHeight(Group* self, re::Edx, uint32_t index) {
    if (index < self->count && self->buffers[index] != nullptr) {
        return At<uint32_t>(self->buffers[index], 0xC);
    }
    return 0;
}

// vf20 / vf21
uint32_t __fastcall GetDepthStencilWidth(Group* self, re::Edx) {
    return self->depthStencil != nullptr ? At<uint32_t>(self->depthStencil, 8) : 0;
}

uint32_t __fastcall GetDepthStencilHeight(Group* self, re::Edx) {
    return self->depthStencil != nullptr ? At<uint32_t>(self->depthStencil, 0xC) : 0;
}

// vf27
NiObject* __fastcall GetBuffer(Group* self, re::Edx, uint32_t index) {
    return index < self->count ? self->buffers[index] : nullptr;
}

// vf28 (4 bytes: not hooked)
NiObject* __fastcall GetDepthStencilBuffer(Group* self, re::Edx) {
    return self->depthStencil;
}

Group* __fastcall Construct(Group* self, re::Edx) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiRenderTargetGroup_vtable);
    for (re::Ptr32<NiObject>& buffer : self->buffers) {
        buffer = nullptr;
    }
    self->count = 0;
    self->depthStencil = nullptr;
    self->rendererData = nullptr;
    return self;
}

const RTTI* __fastcall GetRTTI(Group*, re::Edx) {
    return &NiRenderTargetGroup_RTTI;
}

void __fastcall Destruct(Group* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiRenderTargetGroup_vtable);
    if (self->rendererData != nullptr) {
        Virtual<void>(self->rendererData, 0, 1);
    }
    Release(self->depthStencil);
    for (int i = 7; i >= 0; i--) {
        Release(self->buffers[i]);
    }
    Object_Destruct(self);
}

// vf22: the buffer's pixel format (from its renderer data)
void* __fastcall GetPixelFormat(Group* self, re::Edx, uint32_t index) {
    if (index < self->count && self->buffers[index] != nullptr) {
        void* data = At<void*>(self->buffers[index], 0x10);
        if (data != nullptr) {
            return At<void*>(data, 0xC);
        }
    }
    return nullptr;
}

// vf23
void* __fastcall GetDepthStencilPixelFormat(Group* self, re::Edx) {
    if (self->depthStencil != nullptr) {
        void* data = At<void*>(self->depthStencil, 0x10);
        if (data != nullptr) {
            return At<void*>(data, 0xC);
        }
    }
    return nullptr;
}

// vf25 AttachBuffer(pkBuffer, i): the renderer data dropped first
uint8_t __fastcall AttachBuffer(Group* self, re::Edx, NiObject* buffer, uint32_t index) {
    Virtual<void>(self, 30, static_cast<void*>(nullptr));
    if (index >= self->count) {
        return 0;
    }
    NiObject* old = self->buffers[index];
    if (old != buffer) {
        Release(old);
        self->buffers[index] = buffer;
        AddRef(buffer);
    }
    return 1;
}

// vf26
uint8_t __fastcall AttachDepthStencilBuffer(Group* self, re::Edx, NiObject* depth) {
    Virtual<void>(self, 30, static_cast<void*>(nullptr));
    NiObject* old = self->depthStencil;
    if (old != depth) {
        Release(old);
        self->depthStencil = depth;
        AddRef(depth);
    }
    return 1;
}

// vf31 / vf32
void* __fastcall GetBufferRendererData(Group* self, re::Edx, uint32_t index) {
    if (index < self->count && self->buffers[index] != nullptr) {
        return At<void*>(self->buffers[index], 0x10);
    }
    return nullptr;
}

void* __fastcall GetDepthStencilRendererData(Group* self, re::Edx) {
    return self->depthStencil != nullptr ? At<void*>(self->depthStencil, 0x10) : nullptr;
}

// Create(uiNumBuffers, pkRenderer): empty, for up to the renderer's maximum (8 at most). (When
// the allocation fails, the count is still written: at address 0x28)
Group* __cdecl Create(uint32_t count, void* renderer) {
    if (renderer == nullptr) {
        return nullptr;
    }
    uint32_t max = Virtual<uint32_t>(renderer, 39);   // GetMaxBuffersPerRenderTargetGroup
    if (max < count || count > 8) {
        return nullptr;
    }
    void* p = NiNew(0x34);
    Group* group = p != nullptr ? Group_Construct(p) : nullptr;
    reinterpret_cast<volatile uint32_t*>(reinterpret_cast<uintptr_t>(group) + 0x28)[0] = count;
    return group;
}

// Create(pkBuffer, pkRenderer, pkDepthBuffer): one buffer (and the depth buffer, when the
// renderer takes the pair)
Group* __cdecl CreateWithDepth(NiObject* buffer, void* renderer, NiObject* depth) {
    if (renderer == nullptr) {
        return nullptr;
    }
    if (buffer == nullptr || Virtual<uint32_t>(renderer, 39) < 1) {
        return nullptr;
    }
    if (depth != nullptr && !Virtual<uint8_t>(renderer, 28, buffer, depth)) {   // IsDepthBufferCompatible
        return nullptr;
    }
    void* p = NiNew(0x34);
    Group* group = p != nullptr ? Group_Construct(p) : nullptr;
    re::Ptr32<void>* vtable = VtableOf(group);
    group->count = 1;
    re::PtrCast<uint8_t(__thiscall*)(Group*, NiObject*, uint32_t)>(vtable[25])(group, buffer, 0);
    if (depth != nullptr) {
        Virtual<uint8_t>(group, 26, depth);
    }
    return group;
}

// Create(pkBuffer, pkRenderer, bUseDefaultDepthBuffer, bCreateCompatibleDepthBuffer)
Group* __cdecl CreateForBuffer(NiObject* buffer, void* renderer, int useDefault, int createCompatible) {
    NiObject* depth = nullptr;
    if (uint8_t(useDefault) != 0) {
        depth = Virtual<NiObject*>(renderer, 31);   // GetDefaultDepthStencilBuffer
        if (depth != nullptr) {
            if (Virtual<uint8_t>(renderer, 28, buffer, depth)) {
                return Group_CreateWithDepth(buffer, renderer, depth);
            }
            depth = nullptr;
        }
    }
    if (uint8_t(createCompatible) != 0) {
        depth = DepthStencilBuffer_CreateCompatible(buffer, renderer);
    }
    return Group_CreateWithDepth(buffer, renderer, depth);
}

// vf9 GetViewerStrings
void __fastcall GetViewerStrings(Group* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiRenderTargetGroup_RTTI.name));
}

// vf0
Group* __fastcall DeletingDestructor(Group* self, re::Edx, uint32_t flags) {
    Group_Destructor(self);
    if (flags & 1) {
        NiDelete(self, 0x34);
    }
    return self;
}

} // namespace

// 0x0075D400: [8b 41 2c c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_0075D400(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x2C);
}
RE_UNHOOKED_FN("NiRenderTargetGroup::GetDepthStencilBuffer", 0x0075D400, &Tiny_0075D400);   // 4 bytes

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIRTG_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIRTG_REVERSED("NiRenderTargetGroup::SetRendererData", 0x0075D350, &ni::SetRendererData);
NIRTG_REVERSED("NiRenderTargetGroup::GetWidth", 0x0075D380, &ni::GetWidth);
NIRTG_REVERSED("NiRenderTargetGroup::GetHeight", 0x0075D3A0, &ni::GetHeight);
NIRTG_REVERSED("NiRenderTargetGroup::GetDepthStencilWidth", 0x0075D3C0, &ni::GetDepthStencilWidth);
NIRTG_REVERSED("NiRenderTargetGroup::GetDepthStencilHeight", 0x0075D3D0, &ni::GetDepthStencilHeight);
NIRTG_REVERSED("NiRenderTargetGroup::GetBuffer", 0x0075D3E0, &ni::GetBuffer);
NIRTG_REVERSED("NiRenderTargetGroup::NiRenderTargetGroup", 0x0075D410, &ni::Construct);
NIRTG_REVERSED("NiRenderTargetGroup::GetRTTI", 0x0075D450, &ni::GetRTTI);
NIRTG_REVERSED("NiRenderTargetGroup::~NiRenderTargetGroup", 0x0075D460, &ni::Destruct);
NIRTG_REVERSED("NiRenderTargetGroup::GetPixelFormat", 0x0075D4E0, &ni::GetPixelFormat);
NIRTG_REVERSED("NiRenderTargetGroup::GetDepthStencilPixelFormat", 0x0075D510, &ni::GetDepthStencilPixelFormat);
NIRTG_REVERSED("NiRenderTargetGroup::AttachBuffer", 0x0075D530, &ni::AttachBuffer);
NIRTG_REVERSED("NiRenderTargetGroup::AttachDepthStencilBuffer", 0x0075D5A0, &ni::AttachDepthStencilBuffer);
NIRTG_REVERSED("NiRenderTargetGroup::GetBufferRendererData", 0x0075D5F0, &ni::GetBufferRendererData);
NIRTG_REVERSED("NiRenderTargetGroup::GetDepthStencilRendererData", 0x0075D610, &ni::GetDepthStencilRendererData);
NIRTG_REVERSED("NiRenderTargetGroup::Create", 0x0075D620, &ni::Create);
NIRTG_REVERSED("NiRenderTargetGroup::CreateWithDepth", 0x0075D670, &ni::CreateWithDepth);
NIRTG_REVERSED("NiRenderTargetGroup::DeletingDestructor", 0x0075D700, &ni::DeletingDestructor);
NIRTG_REVERSED("NiRenderTargetGroup::CreateForBuffer", 0x0075D720, &ni::CreateForBuffer);
NIRTG_REVERSED("NiRenderTargetGroup::GetViewerStrings", 0x0075D780, &ni::GetViewerStrings);
