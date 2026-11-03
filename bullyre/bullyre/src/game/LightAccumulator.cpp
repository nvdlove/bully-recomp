// The light accumulator (the one at 0xD140B8): what's left of it: colour slots, a few
// allocations and frames made at start-up, and a count of lights added (up to 2); plus
// 0x5EE1D0, the render states set at the start of a frame's rendering. 0x5EE1D0..0x5EE570.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct LaColour {
    float r, g, b, a;
};

struct LightAccumulator {
    LaColour colours[8];   // +0x0 (the third, +0x20, isn't set by the constructor)
    re::Ptr32<void> alloc80;         // +0x80
    re::Ptr32<void> alloc84;         // +0x84
    re::Ptr32<void> alloc88;         // +0x88
    re::Ptr32<void> alloc8C;         // +0x8C
    re::Ptr32<void> allocs90[2];     // +0x90
    int32_t count;         // +0x98
};
static_assert(offsetof(LightAccumulator, count) == 0x98);

GAME_FN(void*, __cdecl, LA_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, LA_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, LA_FrameCreate, 0x006CA210);
GAME_FN(void*, __cdecl, LA_FrameRotate, 0x006CA1E0, void* frame, const float* axis, float angle, int combine);
GAME_FN(void, __cdecl, LA_SetRenderState, 0x00519790, int state, uint32_t value);
GAME_FN(void, __cdecl, LA_SetAlphaToCoverage, 0x00407460, int on);
GAME_VAR(uint8_t*, la_renderer, 0x020E2380);
GAME_VAR(uint8_t, la_fogRed, 0x020C47FE);
GAME_VAR(uint8_t, la_fogGreen, 0x020C47FF);
GAME_VAR(uint8_t, la_fogBlue, 0x020C4800);

// 0x5EE1D0: the render states and the eight samplers' states set up
void __cdecl SetDefaultRenderStates() {
    LA_SetRenderState(2, 1);
    LA_SetRenderState(5, 1);
    LA_SetRenderState(6, 1);
    LA_SetRenderState(8, 1);
    LA_SetRenderState(7, 2);
    LA_SetRenderState(9, 2);
    LA_SetRenderState(0xC, 0);
    LA_SetRenderState(0xA, 5);
    LA_SetRenderState(0xB, 6);
    LA_SetRenderState(0xE, 0);
    uint32_t colour = uint32_t(la_fogRed) | uint32_t(la_fogGreen) << 8 | uint32_t(la_fogBlue) << 16 | 0xFF000000u;
    LA_SetRenderState(0xF, colour);
    LA_SetRenderState(0x10, 1);
    if (la_renderer) {
        using Fn = void(__thiscall*)(void*);
        re::PtrCast<Fn>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(la_renderer))[2])(la_renderer);
    }
    LA_SetAlphaToCoverage(0);
    LA_SetRenderState(2, 1);
    LA_SetRenderState(3, 1);
    LA_SetRenderState(4, 1);
    // the D3D device's SetSamplerState
    void* device = *reinterpret_cast<re::Ptr32<void>*>(la_renderer + 0xA4);
    using SetSampler = long(__stdcall*)(void*, uint32_t, uint32_t, uint32_t);
    for (uint32_t sampler = 0; sampler < 8; sampler++) {
        re::PtrCast<SetSampler>(re::ComVtable(device)[0x114 / 4])(device, sampler, 6, 3);
        re::PtrCast<SetSampler>(re::ComVtable(device)[0x114 / 4])(device, sampler, 10, 4);
        re::PtrCast<SetSampler>(re::ComVtable(device)[0x114 / 4])(device, sampler, 5, 2);
        re::PtrCast<SetSampler>(re::ComVtable(device)[0x114 / 4])(device, sampler, 7, 2);
        re::PtrCast<SetSampler>(re::ComVtable(device)[0x114 / 4])(device, sampler, 1, 1);
        re::PtrCast<SetSampler>(re::ComVtable(device)[0x114 / 4])(device, sampler, 2, 1);
        re::PtrCast<SetSampler>(re::ComVtable(device)[0x114 / 4])(device, sampler, 3, 1);
    }
}

// 0x5EE330 (thiscall): the constructor: the colours (but the third) 0,0,0,1, the rest zero
LightAccumulator* __fastcall Construct(LightAccumulator* self) {
    for (int i = 0; i < 8; i++) {
        if (i == 2) {
            continue;
        }
        self->colours[i] = {0.0f, 0.0f, 0.0f, 1.0f};
    }
    self->alloc80 = nullptr;
    self->alloc84 = nullptr;
    self->alloc88 = nullptr;
    self->alloc8C = nullptr;
    self->count = 0;
    self->allocs90[0] = nullptr;
    self->allocs90[1] = nullptr;
    return self;
}

// 0x5EE3F0 (thiscall, one argument unused): byte allocations kept and frames made (only
// the first used: turned 160 degrees about (1,1,0); the others are left behind)
void __fastcall Init(LightAccumulator* self, re::Edx, void*) {
    self->alloc80 = LA_Malloc(1);
    self->alloc84 = LA_Malloc(1);
    void* frame = LA_FrameCreate();
    float axis[3] = {1.0f, 1.0f, 0.0f};
    LA_FrameRotate(frame, axis, 160.0f, 1);
    self->alloc88 = LA_Malloc(1);
    LA_FrameCreate();
    self->alloc8C = LA_Malloc(1);
    LA_FrameCreate();
    for (int i = 0; i < 2; i++) {
        void* p = LA_Malloc(1);
        LA_FrameCreate();
        self->allocs90[i] = p;
    }
}

// 0x5EE4A0 (thiscall): the allocations freed
void __fastcall Shutdown(LightAccumulator* self) {
    if (self->alloc80) {
        LA_Free(self->alloc80);
        self->alloc80 = nullptr;
    }
    if (self->alloc84) {
        LA_Free(self->alloc84);
        self->alloc84 = nullptr;
    }
    if (self->alloc88) {
        LA_Free(self->alloc88);
        self->alloc88 = nullptr;
    }
    if (self->alloc8C) {
        LA_Free(self->alloc8C);
        self->alloc8C = nullptr;
    }
    for (int i = 0; i < 2; i++) {
        if (self->allocs90[i]) {
            LA_Free(self->allocs90[i]);
            self->allocs90[i] = nullptr;
        }
    }
}

// 0x5EE540 (thiscall): a light added (only counted, up to 2; the direction and colour
// unused)
void __fastcall Add(LightAccumulator* self, re::Edx, const float*, const float*) {
    if (self->count < 2) {
        self->count++;
    }
}

// 0x5EE560 (thiscall): the count reset
void __fastcall Reset(LightAccumulator* self) {
    self->count = 0;
}

// 0x5EE570: a flag's bit 4 (callers pass an object in ecx, unused)
uint32_t __fastcall Flag4(void*, re::Edx, uint32_t flags) {
    return flags & 4;
}

#define LIGHTACC_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(LightAccOriginal_, __LINE__);                                          \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(LightAccOriginal_, __LINE__), \
             re::Mode::Original)

LIGHTACC_REVERSED("RenderStates::SetDefaults", 0x005EE1D0, &SetDefaultRenderStates);
LIGHTACC_REVERSED("LightAccumulator::Construct", 0x005EE330, &Construct);
LIGHTACC_REVERSED("LightAccumulator::Init", 0x005EE3F0, &Init);
LIGHTACC_REVERSED("LightAccumulator::Shutdown", 0x005EE4A0, &Shutdown);
LIGHTACC_REVERSED("LightAccumulator::Add", 0x005EE540, &Add);
LIGHTACC_REVERSED("LightAccumulator::Reset", 0x005EE560, &Reset);
LIGHTACC_REVERSED("LightAccumulator::Flag4", 0x005EE570, &Flag4);

} // namespace
