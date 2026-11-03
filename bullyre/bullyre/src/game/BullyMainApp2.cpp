// BullyMain, continued (0x401D50 to 0x402960): creating the renderer, shutting down,
// the shadow render targets, MdShaderConstantManager (the shader constant cache that
// goes to the D3D9 device in ranges), the OpenMP scene update, the node light list.

#include <cstdint>

#include "game/VCall.h"
#include "game/HeapCheck.h"
#include "log.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_ARRAY(char, ms_levelName, 0x020E2280);
inline int __cdecl strncpy_s_(char* dst, size_t size, const char* src, size_t count) { return crt::StrncpyS(dst, size, src, count); }
GAME_FN(void, __cdecl, DebugPrint, 0x0049A650, const char* format);   // (does nothing)
GAME_FN(int, __cdecl, Renderer_GetDeviceId, 0x00878D10);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __thiscall, NiDX9Renderer_Construct, 0x0087FAA0, void* self);
GAME_FN(uint8_t, __thiscall, NiDX9Renderer_Initialise, 0x0087FE00, void* self, uint32_t, uint32_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
GAME_FN(void, __cdecl, RendererData_Unregister, 0x00759EE0, const char* name);
GAME_FN(void, __cdecl, RendererData_Register, 0x00759EC0, const char* name, void* create);
constexpr uintptr_t kBullyNiDX9RendererVtable = 0x008FEFE4;
inline void* const ms_rendererLock = reinterpret_cast<void*>(0x020E6D4C);   // CRITICAL_SECTION
GAME_VAR(uint32_t, ms_rendererLockOwner, 0x020E6D64);
GAME_VAR(int32_t, ms_rendererLockCount, 0x020E6D68);

// BullyMain's smart pointers (NiPointer<NiRefObject>: the refcount at +4)
GAME_VAR(uint8_t*, ms_pRenderTargetAC, 0x00BCBBAC);
GAME_VAR(uint8_t*, ms_pRenderTargetB0, 0x00BCBBB0);
GAME_ARRAY(uint8_t*, ms_pointersB4, 0x00BCBBB4);   // B4, B8, BC, C0, C4, C8
GAME_VAR(uint8_t*, ms_pClickBC, 0x00BCBBBC);
GAME_VAR(uint8_t*, ms_pClickC0, 0x00BCBBC0);
GAME_VAR(uint8_t*, ms_pClickC4, 0x00BCBBC4);
GAME_VAR(uint8_t*, ms_pClickC8, 0x00BCBBC8);
GAME_FN(void, __cdecl, Fn_518270, 0x00518270);
GAME_FN(void, __cdecl, Fn_738BE0, 0x00738BE0);
GAME_FN(void, __thiscall, NiApplication_Terminate, 0x00831C30, void* self);

GAME_VAR(void*, ms_pRenderer, 0x020E2380);
GAME_FN(uint8_t*, __cdecl, RenderedTexture_Create, 0x0075D960, uint32_t width, uint32_t height, void* renderer, void* format, int flags);
GAME_FN(uint8_t*, __cdecl, RenderTargetGroup_Create, 0x0075D720, void* buffer, void* renderer, int, int);

GAME_FN(void*, __cdecl, NiAlignedMalloc, 0x00752340, size_t size, size_t align);
GAME_FN(void*, __cdecl, NiMalloc, 0x00752300, size_t size);
inline void* __cdecl memset_(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __thiscall, NiDX9ShaderConstantManager_Destruct, 0x00886EB0, void* self);
GAME_FN(uint8_t, __thiscall, RendererSettings_Fn882550, 0x00882550, void* settings);
constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uint32_t kMdShaderConstantManagerVtable = 0x008FF338;
inline void* const ms_numRefObjects = reinterpret_cast<void*>(0x020E2168);

// OpenMP (vcomp)
GAME_FN(void, __cdecl, vcomp_for_static_simple_init, 0x00877BDE, int first, int last, int step, int ascending, int* lower, int* upper);
GAME_FN(void, __cdecl, vcomp_for_static_end, 0x00877BD8);
GAME_FN(void, __cdecl, vcomp_fork, 0x00877BE4, int ifval, int argCount, void* fn, ...);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E240, void* object);
GAME_FN(void, __cdecl, Fn_40AAA0, 0x0040AAA0, void* object);

GAME_VAR(uint32_t, ms_NiNodeRTTIBit, 0x020E2550);
GAME_FN(void, __thiscall, LightList_Add, 0x007843D0, void* list, re::Ptr32<uint8_t>* light);

GAME_VAR(void*, ms_pFreeList, 0x020E2458);
inline void* const ms_freeListLock = reinterpret_cast<void*>(0x020E245C);
GAME_VAR(uint32_t, ms_freeListLockOwner, 0x020E2474);
GAME_VAR(int32_t, ms_freeListLockCount, 0x020E2478);
GAME_FN(void, __cdecl, AllocateFreeListBlock, 0x00401B90);

// NiRefObject's DecRefCount: DeleteThis (vtable[1]) at zero
void Release(uint8_t* object) {
    if (vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

// NiPointer's release: the pointer cleared
void ReleasePointer(re::Ptr32<uint8_t>& p) {
    if (p) {
        Release(p);
        p = nullptr;
    }
}

// NiPointer's assignment
void AssignPointer(re::Ptr32<uint8_t>& p, uint8_t* object) {
    if (p == object) {
        return;
    }
    if (p) {
        Release(p);
    }
    p = object;
    if (object) {
        vc::Increment(object + 4);
    }
}

} // namespace

namespace BullyMain {

// NiObject's IsKindOf through the RTTI's class bits (vtable[2] is GetRTTI)
static uint8_t* IsKindOfBit(uint8_t* object, uint32_t bit) {
    uint8_t* rtti = vc::Call<uint8_t*>(object, 0x08);
    uint32_t bits = *reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4);
    return (bits & (1u << (bit & 0x1F))) ? object : nullptr;
}

// 0x401D50: NiDX9Renderer::Create for BullyNiDX9Renderer (its arguments passed through
// to the renderer's initialisation, under the renderer's lock)
static void* __cdecl CreateRenderer(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6,
                                    uint32_t a7, uint32_t a8, uint32_t a9, uint32_t a10, uint32_t a11, uint32_t a12,
                                    uint32_t a13, uint32_t a14) {
    if (Renderer_GetDeviceId() == -1) {
        strncpy_s_(ms_levelName, 0x100, "Creation failed: Could not initialize DirectX9", 0xFF);
        DebugPrint("NiDX9Renderer::Create> Invalid device ID specified - FAILING\n");
        return nullptr;
    }
    void* renderer = NiNew(0x930);
    if (renderer) {
        NiDX9Renderer_Construct(renderer);
        *reinterpret_cast<uint32_t*>(renderer) = kBullyNiDX9RendererVtable;
    } else {
        renderer = nullptr;
    }
    vc::Import<void>(vc::kEnterCriticalSection, ms_rendererLock);
    ms_rendererLockOwner = vc::Import<uint32_t>(vc::kGetCurrentThreadId);
    ms_rendererLockCount++;
    uint8_t ok = NiDX9Renderer_Initialise(renderer, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    if (--ms_rendererLockCount == 0) {
        ms_rendererLockOwner = 0;
    }
    vc::Import<void>(vc::kLeaveCriticalSection, ms_rendererLock);
    if (!ok) {
        if (renderer) {
            vc::Call<void>(renderer, 0x00, 1);   // deleting destructor
        }
        return nullptr;
    }
    RendererData_Unregister("NiPersistentSrcTextureRendererData");
    RendererData_Register("NiPersistentSrcTextureRendererData", reinterpret_cast<void*>(0x00886790));
    return renderer;
}

// 0x401E70 (thiscall): BullyMain's Terminate: its pointers released, then
// NiApplication's, then vtable[60]
static void __fastcall Terminate(uint8_t* self, re::Edx) {
    ReleasePointer(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x238));
    for (int i = 0; i < 6; i++) {
        ReleasePointer(ms_pointersB4[i]);
    }
    ReleasePointer(ms_pRenderTargetB0);
    ReleasePointer(ms_pRenderTargetAC);
    Fn_518270();
    Fn_738BE0();
    NiApplication_Terminate(self);
    vc::Call<void>(self, 0xF0);
}

// 0x401FF0: `object` when it's of the class whose RTTI bit is at +8 of `rtti`
static uint8_t* __cdecl IsKindOf(uint8_t* rttiInfo, uint8_t* object) {
    if (!object) {
        return nullptr;
    }
    return IsKindOfBit(object, *reinterpret_cast<uint32_t*>(rttiInfo + 8));
}

// 0x402030 (a virtual; `this` unused): the render targets again, for the renderer's
// current back buffer size (format 2,2,2; 0x100)
static void __cdecl RecreateRenderTargets() {
    int32_t format[3] = {2, 2, 2};
    uint8_t* buffer = vc::Call<uint8_t*>(ms_pRenderer, 0x80);
    ReleasePointer(ms_pRenderTargetB0);
    ReleasePointer(ms_pRenderTargetAC);
    AssignPointer(ms_pRenderTargetB0, RenderedTexture_Create(*reinterpret_cast<uint32_t*>(buffer + 8),
                                                             *reinterpret_cast<uint32_t*>(buffer + 0xC), ms_pRenderer, format,
                                                             0x100));
    AssignPointer(ms_pRenderTargetAC, RenderTargetGroup_Create(buffer, ms_pRenderer, 1, 0));
}

} // namespace BullyMain

// ---- MdShaderConstantManager (0x8C bytes) ----
// +0x08/+0x0C vertex float constants (current, shadow), +0x10/+0x14 pixel floats,
// +0x30..+0x3C int constants, +0x58..+0x64 bool constants, +0x18.. the dirty ranges
// (first, end) of each, +0x28.. the counts, +0x80 the D3D device, +0x84 the renderer,
// +0x88 "no vertex constants: software"
namespace MdShaderConstantManager {

static uint32_t& U(uint8_t* self, int offset) {
    return *reinterpret_cast<uint32_t*>(self + offset);
}

// 0x402160 (thiscall): the deleting destructor
static void* __fastcall Destroy(uint8_t* self, re::Edx, int flags) {
    NiDX9ShaderConstantManager_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x8C);
    }
    return self;
}

static void* ZeroedAligned(size_t size) {
    void* p = NiAlignedMalloc(size, 0x10);
    memset_(p, 0, size);
    return p;
}

static void* Zeroed(size_t size) {
    void* p = NiMalloc(size);
    memset_(p, 0, size);
    return p;
}

// 0x402190 (thiscall): the constructor, sized from the device caps (D3DCAPS9: +0xC5
// the vertex shader major version, +0xC8 its constant count, +0xCD the pixel shader's
// major version, +0xFC/+0x10C the flow control depths)
static uint8_t* __fastcall Construct(uint8_t* self, re::Edx, uint8_t* renderer, uint8_t* caps) {
    U(self, 0) = kNiRefObjectVtable;
    U(self, 4) = 0;
    vc::Increment(ms_numRefObjects);
    U(self, 0) = kMdShaderConstantManagerVtable;
    U(self, 0x84) = reinterpret_cast<uintptr_t>(renderer);
    for (int offset = 0x08; offset <= 0x7C; offset += 4) {
        U(self, offset) = 0;
    }
    if (renderer) {
        U(self, 0x80) = *reinterpret_cast<uint32_t*>(renderer + 0xA4);
    }
    // (with no renderer +0x80 isn't set: the constructor's caller's value stays)
    uint8_t* device = reinterpret_cast<uint8_t*>(U(self, 0x80));
    if (device) {
        // IUnknown::AddRef (stdcall)
        reinterpret_cast<unsigned long(__stdcall*)(void*)>(static_cast<void*>(re::ComVtable(device)[1]))(device);
    }
    U(self, 0x28) = *reinterpret_cast<uint32_t*>(caps + 0xC8);
    switch (caps[0xC5]) {
    case 1:
        U(self, 0x78) = 0;
        U(self, 0x50) = 0;
        break;
    case 2:
        U(self, 0x7C) = *reinterpret_cast<int32_t*>(caps + 0xFC) != 0 ? 0x10 : 0;
        U(self, 0x50) = 0x10;
        break;
    case 3:
        U(self, 0x78) = 0x10;
        U(self, 0x50) = 0x10;
        break;
    }
    // (the renderer is read here even when it's null)
    if (U(self, 0x28) == 0 && renderer[0x3F0]) {
        self[0x88] = 1;
        U(self, 0x28) = 0x2000;
    } else {
        self[0x88] = 0;
    }
    switch (caps[0xCD]) {
    case 1:
        U(self, 0x2C) = 8;
        U(self, 0x7C) = 0;
        U(self, 0x54) = 0;
        break;
    case 2:
        U(self, 0x2C) = 0x20;
        U(self, 0x7C) = *reinterpret_cast<int32_t*>(caps + 0x10C) != 0 ? 0x10 : 0;
        U(self, 0x54) = 0x10;
        break;
    case 3:
        U(self, 0x2C) = 0xE0;
        U(self, 0x7C) = 0x10;
        U(self, 0x54) = 0x10;
        break;
    }
    U(self, 0x20) = U(self, 0x2C);
    U(self, 0x40) = U(self, 0x50);
    U(self, 0x48) = U(self, 0x54);
    U(self, 0x18) = U(self, 0x28);
    U(self, 0x68) = U(self, 0x78);
    U(self, 0x70) = U(self, 0x7C);
    if (U(self, 0x28)) {
        size_t size = U(self, 0x28) * 0x10;
        U(self, 0x08) = reinterpret_cast<uintptr_t>(ZeroedAligned(size));
        U(self, 0x0C) = reinterpret_cast<uintptr_t>(ZeroedAligned(size));
    }
    if (U(self, 0x2C)) {
        size_t size = U(self, 0x2C) * 0x10;
        U(self, 0x10) = reinterpret_cast<uintptr_t>(ZeroedAligned(size));
        U(self, 0x14) = reinterpret_cast<uintptr_t>(ZeroedAligned(size));
    }
    if (U(self, 0x78)) {
        U(self, 0x58) = reinterpret_cast<uintptr_t>(Zeroed(U(self, 0x78) * 4));
        U(self, 0x5C) = reinterpret_cast<uintptr_t>(Zeroed(U(self, 0x78) * 4));
    }
    if (U(self, 0x7C)) {
        U(self, 0x60) = reinterpret_cast<uintptr_t>(Zeroed(U(self, 0x7C) * 4));
        U(self, 0x64) = reinterpret_cast<uintptr_t>(Zeroed(U(self, 0x7C) * 4));
    }
    if (U(self, 0x50)) {
        size_t size = U(self, 0x50) * 0x10;
        U(self, 0x30) = reinterpret_cast<uintptr_t>(ZeroedAligned(size));
        U(self, 0x34) = reinterpret_cast<uintptr_t>(ZeroedAligned(size));
    }
    if (U(self, 0x54)) {
        size_t size = U(self, 0x54) * 0x10;
        U(self, 0x38) = reinterpret_cast<uintptr_t>(ZeroedAligned(size));
        U(self, 0x3C) = reinterpret_cast<uintptr_t>(ZeroedAligned(size));
    }
    return self;
}

// a range of constants to the device (IDirect3DDevice9's Set*ShaderConstant*, stdcall)
static void Upload(uint8_t* self, int method, int firstOffset, int dataOffset, int stride) {
    uint32_t first = U(self, firstOffset);
    uint32_t end = U(self, firstOffset + 4);
    if (first < end) {
        void* device = reinterpret_cast<void*>(U(self, 0x80));
        void* fn = re::ComVtable(device)[method / 4];   // (D3D9's: native slots)
        reinterpret_cast<long(__stdcall*)(void*, uint32_t, const void*, uint32_t)>(fn)(
            device, first, reinterpret_cast<uint8_t*>(U(self, dataOffset)) + first * stride, end - first);
    }
}

// 0x402490 (thiscall): the dirty ranges to the device, then reset (first = count, end = 0)
static void __fastcall Commit(uint8_t* self, re::Edx) {
    if (self[0x88]) {
        uint8_t* renderer = reinterpret_cast<uint8_t*>(U(self, 0x84));
        if (!RendererSettings_Fn882550(*reinterpret_cast<re::Ptr32<void>*>(renderer + 0x6B4))) {
            U(self, 0x18) = U(self, 0x28);
            U(self, 0x1C) = 0;
        }
    }
    Upload(self, 0x178, 0x18, 0x08, 0x10);   // SetVertexShaderConstantF
    Upload(self, 0x1B4, 0x20, 0x10, 0x10);   // SetPixelShaderConstantF
    Upload(self, 0x188, 0x68, 0x58, 4);      // SetVertexShaderConstantB
    Upload(self, 0x1C4, 0x70, 0x60, 4);      // SetPixelShaderConstantB
    Upload(self, 0x180, 0x40, 0x30, 0x10);   // SetVertexShaderConstantI
    Upload(self, 0x1BC, 0x48, 0x38, 0x10);   // SetPixelShaderConstantI
    U(self, 0x18) = U(self, 0x28);
    U(self, 0x20) = U(self, 0x2C);
    U(self, 0x40) = U(self, 0x50);
    U(self, 0x1C) = 0;
    U(self, 0x24) = 0;
    U(self, 0x44) = 0;
    U(self, 0x4C) = 0;
    U(self, 0x6C) = 0;
    U(self, 0x74) = 0;
    U(self, 0x48) = U(self, 0x54);
    U(self, 0x68) = U(self, 0x78);
    U(self, 0x70) = U(self, 0x7C);
}

} // namespace MdShaderConstantManager

namespace BullyMain {

// 0x4025F0 (thiscall)
static uint32_t __fastcall Get23C(uint8_t* self, re::Edx) {
    return *reinterpret_cast<uint32_t*>(self + 0x23C);
}

// 0x402600
static uint32_t __cdecl GetRenderer9C() {
    return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(ms_pRenderer) + 0x9C);
}

// 0x402610: the OpenMP body of UpdateChildren: the node's children in this thread's
// share updated (time 0), their properties, and 0x40AAA0 when *flag
static void __cdecl UpdateChildren_Body(int count, uint8_t* node, const char* flag) {
    if (count <= 0) {
        return;
    }
    int lower, upper;
    vcomp_for_static_simple_init(0, count - 1, 1, 1, &lower, &upper);
    for (int i = lower; i <= upper; i++) {
        if (uint32_t(i) < *reinterpret_cast<uint16_t*>(node + 0xCE)) {
            void* child = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(node + 0xC8))[i];
            if (child) {
                AVObject_Update(child, 0.0f, 0);
                AVObject_UpdateProperties(child);
                if (*flag) {
                    Fn_40AAA0(child);
                }
            }
        }
    }
    vcomp_for_static_end();
}

// 0x4026B0 (stdcall): a node's children updated in parallel (count from +0xD0)
static void __stdcall UpdateChildren(uint8_t* node, char flag) {
    uint16_t count = *reinterpret_cast<uint16_t*>(node + 0xD0);
    // (testing, bullyre.ini "heap_check": a child in two slots, or one whose parent isn't this
    // node, would be updated by two OpenMP threads at once: the 2026-10-30 soaks' effect
    // states destructed twice. Reported once per node.)
    if (heapcheck::g_validateFrees) {
        static uint8_t* reported[64];
        static int reportedCount = 0;
        bool seen = false;
        for (int i = 0; i < reportedCount; i++) {
            seen |= reported[i] == node;
        }
        if (!seen) {
            uint16_t size = *reinterpret_cast<uint16_t*>(node + 0xCE);
            re::Ptr32<uint8_t>* children = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(node + 0xC8);
            bool bad = false;
            for (uint32_t i = 0; i < size && !bad; i++) {
                uint8_t* child = children[i];
                if (child == nullptr) {
                    continue;
                }
                if (*reinterpret_cast<re::Ptr32<uint8_t>*>(child + 0x18) != node) {
                    re::Log("heap_check: node %p: child %p at slot %u has parent %p", static_cast<void*>(node),
                            static_cast<void*>(child), i, static_cast<void*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(child + 0x18)));
                    bad = true;
                }
                for (uint32_t j = i + 1; j < size; j++) {
                    if (children[j] == child) {
                        re::Log("heap_check: node %p: child %p in slots %u and %u (size %u, used %u)", static_cast<void*>(node),
                                static_cast<void*>(child), i, j, unsigned(size), unsigned(count));
                        bad = true;
                    }
                }
            }
            if (bad && reportedCount < 64) {
                reported[reportedCount++] = node;
            }
        }
    }
    if (count) {
        vcomp_fork(1, 3, reinterpret_cast<void*>(0x00402610), int(count), node, &flag);
    }
}

// 0x4026E0: the render click at 0xBCBBBC's vtable[34] (object, 0)
static void __cdecl RenderClickBC(void* object) {
    vc::Call<void>(ms_pClickBC, 0x88, object, 0);
}

// 0x402700: the same with the click chosen by the flags
static void __cdecl RenderClickChosen(void* object, int second, int third) {
    if (re::Flag(third)) {
        vc::Call<void>(ms_pClickC8, 0x88, object, 0);
        return;
    }
    if (re::Flag(second)) {
        vc::Call<void>(ms_pClickC4, 0x88, object, 0);
        return;
    }
    vc::Call<void>(ms_pClickC0, 0x88, object, 0);
}

// 0x402760: an object's bound (+0x1C..+0x28: 0,0,0 and radius 10000), and its children's
// when it's a node
static void __cdecl ResetBounds(uint8_t* object) {
    float* bound = reinterpret_cast<float*>(object + 0x1C);
    bound[0] = 0.0f;
    bound[1] = 0.0f;
    bound[2] = 0.0f;
    bound[3] = 10000.0f;
    uint8_t* node = IsKindOfBit(object, ms_NiNodeRTTIBit);
    if (!node) {
        return;
    }
    for (uint32_t i = 0; i < *reinterpret_cast<uint16_t*>(node + 0xCE); i++) {
        uint8_t* child = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(node + 0xC8))[i];
        if (child) {
            reinterpret_cast<void(__cdecl*)(uint8_t*)>(0x00402760)(child);
        }
    }
}

// 0x402810 (thiscall): the NiPointer at +0x1C set (a null one is ignored)
static void __fastcall SetPointer1C(uint8_t* self, re::Edx, uint8_t* object) {
    if (!object) {
        return;
    }
    re::Ptr32<uint8_t>& p = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C);
    if (p == object) {
        return;
    }
    if (p) {
        Release(p);
    }
    p = object;
    vc::Increment(object + 4);
}

// 0x402860: two debug prints (compiled out; the second passes the caller's stack)
static void __cdecl FinishLoadShaders() {
    DebugPrint("FinishLoadShaders");
    DebugPrint(nullptr);
}

// 0x402880 (thiscall): the entry (0x10 each at +0xD4, count +0xDC) whose +4 is
// `key` made current (+0xE0; -1 when none)
static uint8_t __fastcall SelectEntry(uint8_t* self, re::Edx, uint32_t key) {
    uint32_t count = *reinterpret_cast<uint32_t*>(self + 0xDC);
    uint8_t* entries = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xD4);
    for (uint32_t i = 0; i < count; i++) {
        if (*reinterpret_cast<uint32_t*>(entries + i * 0x10 + 4) == key) {
            *reinterpret_cast<uint32_t*>(self + 0xE0) = i;
            return 1;
        }
    }
    *reinterpret_cast<int32_t*>(self + 0xE0) = -1;
    return 0;
}

// 0x4028D0 (thiscall): the current entry's +4 (0 when none)
static uint32_t __fastcall GetSelectedEntry(uint8_t* self, re::Edx) {
    uint32_t index = *reinterpret_cast<uint32_t*>(self + 0xE0);
    if (*reinterpret_cast<uint32_t*>(self + 0xDC) <= index) {
        return 0;
    }
    return *reinterpret_cast<uint32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xD4) + index * 0x10 + 4);
}

// 0x4028F0: a node from NiTPointerAllocator's free list (a new block when empty), zeroed
static uint32_t* __cdecl AllocateListNode() {
    vc::Import<void>(vc::kEnterCriticalSection, ms_freeListLock);
    ms_freeListLockOwner = vc::Import<uint32_t>(vc::kGetCurrentThreadId);
    ms_freeListLockCount++;
    if (!ms_pFreeList) {
        AllocateFreeListBlock();
    }
    uint32_t* node = static_cast<uint32_t*>(ms_pFreeList);
    ms_pFreeList = *reinterpret_cast<re::Ptr32<void>*>(node);
    node[2] = 0;
    node[0] = 0;
    node[1] = 0;
    if (--ms_freeListLockCount == 0) {
        ms_freeListLockOwner = 0;
    }
    vc::Import<void>(vc::kLeaveCriticalSection, ms_freeListLock);
    return node;
}

// 0x402960 (thiscall): a light added to the list at +0x2C (held by a temporary NiPointer)
static void __fastcall AddLight(uint8_t* self, re::Edx, uint8_t* light) {
    re::Ptr32<uint8_t> held = light;
    if (light) {
        vc::Increment(light + 4);
    }
    LightList_Add(self + 0x2C, &held);
    if (light) {
        Release(light);
    }
}

} // namespace BullyMain

// ---- registration ----------------------------------------------------------------------

#define C00_APP2_REVERSED(name, fn, address)                                                        \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace BullyMain {
C00_APP2_REVERSED("BullyMain::CreateRenderer", CreateRenderer, 0x00401D50);
C00_APP2_REVERSED("BullyMain::Terminate", Terminate, 0x00401E70);
C00_APP2_REVERSED("NiObject::IsKindOf", IsKindOf, 0x00401FF0);
C00_APP2_REVERSED("BullyMain::RecreateRenderTargets", RecreateRenderTargets, 0x00402030);
C00_APP2_REVERSED("BullyMain::Get23C", Get23C, 0x004025F0);
C00_APP2_REVERSED("BullyMain::GetRenderer9C", GetRenderer9C, 0x00402600);
C00_APP2_REVERSED("BullyMain::UpdateChildren_Body", UpdateChildren_Body, 0x00402610);
C00_APP2_REVERSED("BullyMain::UpdateChildren", UpdateChildren, 0x004026B0);
C00_APP2_REVERSED("BullyMain::RenderClickBC", RenderClickBC, 0x004026E0);
C00_APP2_REVERSED("BullyMain::RenderClickChosen", RenderClickChosen, 0x00402700);
C00_APP2_REVERSED("BullyMain::ResetBounds", ResetBounds, 0x00402760);
C00_APP2_REVERSED("BullyNiDX9Renderer::SetPointer1C", SetPointer1C, 0x00402810);
C00_APP2_REVERSED("BullyMain::FinishLoadShaders", FinishLoadShaders, 0x00402860);
C00_APP2_REVERSED("BullyMain::SelectEntry", SelectEntry, 0x00402880);
C00_APP2_REVERSED("BullyMain::GetSelectedEntry", GetSelectedEntry, 0x004028D0);
C00_APP2_REVERSED("NiTPointerAllocator::AllocateNode", AllocateListNode, 0x004028F0);
C00_APP2_REVERSED("BullyMain::AddLight", AddLight, 0x00402960);
} // namespace BullyMain

namespace MdShaderConstantManager {
C00_APP2_REVERSED("MdShaderConstantManager::Destroy", Destroy, 0x00402160);
C00_APP2_REVERSED("MdShaderConstantManager::Construct", Construct, 0x00402190);
C00_APP2_REVERSED("MdShaderConstantManager::Commit", Commit, 0x00402490);
} // namespace MdShaderConstantManager
