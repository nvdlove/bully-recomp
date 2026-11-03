// NiDepthStencilBuffer (vtable 0x95F838, 0x14 bytes): an Ni2DBuffer (another chunk's: its
// constructor 0x7D1DD0, destructor 0x7D1E00) of a depth/stencil format: the width (+8), the height
// (+0xC), the renderer's data (+0x10, a reference). Its creation (for a format, for a renderer's
// data, compatible with another buffer), its destruction, and the renderer data's assignment
// (code the compiler shares with other classes' reference at +0x10).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t NiDepthStencilBuffer_vtable = 0x0095F838;

struct NiDepthStencilBuffer {
    re::Ptr32<void> vtable;                // +0x0
    uint32_t refCount;           // +0x4
    uint32_t width;              // +0x8
    uint32_t height;             // +0xC
    re::Ptr32<NiRefObject> rendererData;   // +0x10 (its pixel format +0xC, its multisample setting +0x10)
};
static_assert(sizeof(NiDepthStencilBuffer) == 0x14);

GAME_FN(void, __thiscall, Ni2DBuffer_Construct, 0x007D1DD0, void* self);
GAME_FN(void, __thiscall, Ni2DBuffer_Destruct, 0x007D1E00, void* self);
GAME_FN(NiDepthStencilBuffer*, __cdecl, DepthStencilBuffer_Create, 0x007ABE40, uint32_t width, uint32_t height,
        void* renderer, void* format, uint32_t multisample);

// a new one (NiNew'd, constructed; null when NiNew fails)
NiDepthStencilBuffer* New() {
    NiDepthStencilBuffer* buffer = static_cast<NiDepthStencilBuffer*>(NiNew(0x14));
    if (buffer == nullptr) {
        return nullptr;
    }
    Ni2DBuffer_Construct(buffer);
    buffer->vtable = reinterpret_cast<void*>(NiDepthStencilBuffer_vtable);
    return buffer;
}

// the reference at +0x10 assigned (the old one let go before the new one is held)
void AssignRendererData(NiDepthStencilBuffer* self, NiRefObject* data) {
    NiRefObject* old = self->rendererData;
    if (old == data) {
        return;
    }
    if (old != nullptr && Decrement(&old->refCount) == 0) {
        Virtual<void>(old, 1);
    }
    self->rendererData = data;
    if (data != nullptr) {
        Increment(&data->refCount);
    }
}

NiDepthStencilBuffer* __fastcall DeletingDestructor(NiDepthStencilBuffer* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiDepthStencilBuffer_vtable);
    Ni2DBuffer_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x14);
    }
    return self;
}

// Create(width, height, (renderer), format, multisample): none unless the format is a
// depth/stencil one (15); its renderer data made (slot 20), the buffer deleted when that fails. (A
// failed NiNew is written through, as the game has it)
NiDepthStencilBuffer* __cdecl Create(uint32_t width, uint32_t height, void*, void* format, uint32_t multisample) {
    if (At<uint32_t>(format, 4) != 0xF) {
        return nullptr;
    }
    NiDepthStencilBuffer* buffer = New();
    buffer->width = width;
    buffer->height = height;
    if (!Virtual<uint8_t>(buffer, 20, format, multisample)) {   // CreateRendererData
        Virtual<void*>(buffer, 0, 1u);
        return nullptr;
    }
    return buffer;
}

// SetRendererData(data)
void __fastcall SetRendererData(NiDepthStencilBuffer* self, re::Edx, NiRefObject* data) {
    AssignRendererData(self, data);
}

// CreateCompatible(buffer, renderer): one of the buffer's size in the renderer's depth/stencil
// format for the buffer's (its slot 38), multisampled as the buffer is (0x100 without data)
NiDepthStencilBuffer* __cdecl CreateCompatible(NiDepthStencilBuffer* buffer, void* renderer) {
    if (buffer == nullptr || renderer == nullptr) {
        return nullptr;
    }
    NiRefObject* data = buffer->rendererData;
    void* bufferFormat = data != nullptr ? At<void*>(data, 0xC) : nullptr;
    void* format = Virtual<void*>(renderer, 38, bufferFormat);   // FindClosestDepthStencilFormat
    if (format == nullptr) {
        return nullptr;
    }
    data = buffer->rendererData;
    uint32_t multisample = data != nullptr ? At<uint16_t>(data, 0x10) : 0x100;
    return DepthStencilBuffer_Create(buffer->width, buffer->height, renderer, format, multisample);
}

// Create(width, height, renderer data): one over the renderer's data. (A failed NiNew is written
// through, as the game has it)
NiDepthStencilBuffer* __cdecl CreateWithData(uint32_t width, uint32_t height, NiRefObject* data) {
    NiDepthStencilBuffer* buffer = New();
    buffer->width = width;
    buffer->height = height;
    AssignRendererData(buffer, data);
    return buffer;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIDEPTHSTENCIL_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIDEPTHSTENCIL_REVERSED("NiDepthStencilBuffer::DeletingDestructor", 0x007ABE10, &ni::DeletingDestructor);
NIDEPTHSTENCIL_REVERSED("NiDepthStencilBuffer::Create", 0x007ABE40, &ni::Create);
// (the one copy of a reference-at-+0x10 setter the classes share: NiShadowManager.cpp calls it as
// NiRenderClick::SetRenderTargetGroup)
NIDEPTHSTENCIL_REVERSED("NiDepthStencilBuffer::SetRendererData", 0x007ABEF0, &ni::SetRendererData);
NIDEPTHSTENCIL_REVERSED("NiDepthStencilBuffer::CreateCompatible", 0x007ABF40, &ni::CreateCompatible);
NIDEPTHSTENCIL_REVERSED("NiDepthStencilBuffer::Create(rendererData)", 0x007ABFA0, &ni::CreateWithData);
