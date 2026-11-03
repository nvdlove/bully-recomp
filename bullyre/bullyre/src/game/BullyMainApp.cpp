// The start of the exe (0x401020 on): BullyMain (the game's NiApplication), the shader
// loading and the per-frame shader constants, BullyNiDX9Renderer's small virtuals, and
// the Gamebryo helpers inlined here (NiFixedString, NiPointer, the list allocator).

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// NiGlobalStringTable: a string handle points 8 bytes past its header (+0 refcount,
// +6 length); the null handle is the value at 0x951028
GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);

inline int __cdecl strncpy_s_(char* dst, size_t size, const char* src, size_t count) { return crt::StrncpyS(dst, size, src, count); }
GAME_FN(void, __cdecl, SetAppActive, 0x00405DB0, int active);
GAME_FN(uint8_t, __thiscall, NiApplication_OnWindowMessage, 0x00830D90, void* self, void* message);

// the shader library
GAME_VAR(uint8_t, ms_bNoShaderPreload, 0x00BCBB52);
GAME_FN(void*, __cdecl, ShaderLibrary_Get, 0x00756620);
GAME_FN(uint32_t, __thiscall, NameList_GetCount, 0x007564C0, void* list);
GAME_FN(const char*, __thiscall, NameList_Get, 0x007564E0, void* list, uint32_t index);
GAME_FN(void, __cdecl, LoadingScreen_Message, 0x0043C4F0, const char* title, const char* text, uintptr_t);

GAME_VAR(uint8_t, ms_bInRendererCall, 0x00BCBB40);
GAME_VAR(void*, ms_pRenderer_BCBB48, 0x00BCBB48);

// global shader constants (NiShaderConstantMap entries)
GAME_FN(uint8_t, __cdecl, ShaderConstant_Get, 0x00756C10, void* constant, void* a, void* b);
GAME_FN(void, __cdecl, ShaderConstant_Add, 0x00756A60, void* constant, int type, int size, const void* data);
GAME_FN(void, __cdecl, ShaderConstant_Set, 0x00756C70, void* constant, int size, const void* data);
inline void* const kFogColour = reinterpret_cast<void*>(0x00BCC4C0);
inline void* const kFogRange = reinterpret_cast<void*>(0x00BCC4C4);
inline void* const kTime = reinterpret_cast<void*>(0x00BCC538);
inline void* const kSinCos = reinterpret_cast<void*>(0x00BCC53C);
inline void* const kColourA = reinterpret_cast<void*>(0x00BCC540);
inline void* const kColourB = reinterpret_cast<void*>(0x00BCC544);
GAME_VAR(uint8_t, ms_bAddFogConstants, 0x009D355D);
GAME_ARRAY(uint8_t, ms_timecycColours, 0x020C47F8);   // 0x20C47F8..0x20C4800: three RGB colours
GAME_VAR(float, ms_timeMicroseconds, 0x00C1A9B0);

GAME_ARRAY(char, ms_levelName, 0x020E2280);   // 0x100

GAME_FN(void, __cdecl, NiDX9Renderer_Fn885E80, 0x00885E80, void* self, int a, int b);
GAME_VAR(uint8_t, ms_bRendererFlag_BCB804, 0x00BCB804);

// NiTPointerAllocator's free list (a node is 0xC: next, then the data)
GAME_VAR(void*, ms_pFreeList, 0x020E2458);
GAME_VAR(void*, ms_pBlockList, 0x020E2454);
GAME_VAR(uint32_t, ms_blockSize, 0x00BC75AC);
inline void* const ms_freeListLock = reinterpret_cast<void*>(0x020E245C);   // CRITICAL_SECTION
GAME_VAR(uint32_t, ms_freeListLockOwner, 0x020E2474);
GAME_VAR(int32_t, ms_freeListLockCount, 0x020E2478);
GAME_FN(void*, __cdecl, NiMalloc, 0x007524D0, size_t size);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);

} // namespace

namespace BullyMain {

// 0x401020: strncpy_s, returning the destination
static char* __cdecl CopyString(char* dst, size_t size, const char* src, size_t count) {
    strncpy_s_(dst, size, src, count);
    return dst;
}

// 0x401050 (thiscall): NiFixedString from a C string
static re::Ptr32<char>* __fastcall FixedString_Construct(re::Ptr32<char>* self, re::Edx, const char* s) {
    *self = s ? StringTable_AddString(s) : static_cast<char*>(ms_nullString);
    return self;
}

// 0x401080 (thiscall): NiFixedString's copy constructor
static re::Ptr32<char>* __fastcall FixedString_Copy(re::Ptr32<char>* self, re::Edx, const re::Ptr32<char>* other) {
    if (*other != ms_nullString) {
        vc::Increment(*other - 8);
    }
    *self = *other;
    return self;
}

// 0x4010B0, 0x4010C0: BullyNiDX9Renderer virtuals that jump to NiDX9Renderer's
RE_UNHOOKED("BullyNiDX9Renderer::vf101", 0x004010B0);
RE_UNHOOKED("BullyNiDX9Renderer::vf96", 0x004010C0);

// 0x4010D0 (thiscall): the window messages BullyMain handles itself (the message's +4
// is its id, +8 its wParam), the rest go to NiApplication
static uint8_t __fastcall OnWindowMessage(void* self, re::Edx, uint8_t* message) {
    uint32_t id = *reinterpret_cast<uint32_t*>(message + 4);
    int32_t wParam = *reinterpret_cast<int32_t*>(message + 8);
    switch (id) {
    case 6:   // WM_ACTIVATE
        if (wParam != 0) {
            SetAppActive(1);
            return 1;
        }
        SetAppActive(0);
        return 1;
    case 8:   // WM_KILLFOCUS
        SetAppActive(0);
        return 1;
    case 0x10:   // WM_CLOSE (ignored)
        return 1;
    case 0x104:   // WM_SYSKEYDOWN: Alt alone is swallowed
        if (wParam == 0x12) {
            return 1;
        }
        break;
    }
    return NiApplication_OnWindowMessage(self, message);
}

// 0x401240 (stdcall): the compiler's `vector constructor iterator'
static void __stdcall ConstructArray(uint8_t* p, uint32_t size, int count, void(__thiscall* ctor)(void*)) {
    while (--count >= 0) {
        ctor(p);
        p += size;
    }
}

// 0x401280: BullyMain::LoadAllShaders, the BullyDef and MdWater ones of every library
// (a loading screen message every fourth)
static void __cdecl LoadAllShaders() {
    if (ms_bNoShaderPreload) {
        return;
    }
    void* factory = ShaderLibrary_Get();
    uint32_t sinceMessage = 0;
    void* library = vc::Call<void*>(factory, 0x38);   // the first library
    while (library) {
        void* names = vc::Call<void*>(library, 0x14);
        for (uint32_t i = 0; i < NameList_GetCount(names); i++) {
            const char* name = NameList_Get(names, i);
            if (strncmp(name, "BullyDef", 8) != 0 && strncmp(name, "MdWater", 7) != 0) {
                continue;
            }
            if (sinceMessage > 3) {
                char text[256];
                sprintf(text, "Loading: %s\n", name);
                LoadingScreen_Message("BullyMain::LoadAllShaders", text, 0);
                sinceMessage = 0;
            }
            void* shader = vc::Call<void*>(factory, 0x08, name, -1, 1);
            if (shader && !vc::Call<uint8_t>(shader, 0x0C)) {
                vc::Call<void>(shader, 0x10);
            }
            sinceMessage++;
        }
        library = vc::Call<void*>(factory, 0x3C);   // the next
    }
}

// 0x4013A0: the renderer's vtable[50], flagged as in progress
static void __cdecl CallRenderer_C8() {
    ms_bInRendererCall = 1;
    vc::Call<void>(ms_pRenderer_BCBB48, 0xC8);
    ms_bInRendererCall = 0;
}

static void AddConstantIfMissing(void* constant) {
    uint32_t a = 0, b = 0;
    if (!ShaderConstant_Get(constant, &b, &a)) {
        float zero[4] = {0, 0, 0, 0};
        ShaderConstant_Add(constant, 7, 0x10, zero);
    }
}

// 0x4013C0: the fog constants: the colour from the timecycle, the range from the object
// in esi (+0x1BC start, +0x1B8 end). Takes esi: not hookable
[[maybe_unused]] static void UpdateFogConstants(uint8_t* fog) {
    if (ms_bAddFogConstants) {
        AddConstantIfMissing(kFogColour);
        AddConstantIfMissing(kFogRange);
        ms_bAddFogConstants = 0;
    }
    float v[4];
    v[0] = float(ms_timecycColours[6] / 255.0);
    v[1] = float(ms_timecycColours[7] / 255.0);
    v[2] = float(ms_timecycColours[8] / 255.0);
    v[3] = 1.0f;
    ShaderConstant_Set(kFogColour, 0x10, v);
    float end = *reinterpret_cast<float*>(fog + 0x1B8);
    v[0] = *reinterpret_cast<float*>(fog + 0x1BC);
    v[1] = end;
    v[2] = end;
    v[3] = 1.0f / end;
    ShaderConstant_Set(kFogRange, 0x10, v);
}
RE_UNHOOKED_FN("BullyMain::UpdateFogConstants", 0x004013C0, &UpdateFogConstants);

// 0x401520: the time constants (seconds, and wrapped at 1, 120 and 240; its sine and
// cosine) and two timecycle colours
static void __cdecl UpdateTimeConstants() {
    // (the constant is 1e-6 rounded to float, stored as a double)
    float t = float(double(ms_timeMicroseconds) * double(1e-06f));
    AddConstantIfMissing(kTime);
    AddConstantIfMissing(kSinCos);
    AddConstantIfMissing(kColourA);
    AddConstantIfMissing(kColourB);
    float v[4];
    v[0] = t;
    v[1] = float(std::fmod(double(t), 1.0));
    v[2] = float(std::fmod(double(t), 120.0));
    v[3] = float(std::fmod(double(t), 240.0));
    ShaderConstant_Set(kTime, 0x10, v);
    float s = float(std::sin(double(t)));
    float c = float(std::cos(double(t)));
    v[0] = s;
    v[1] = c;
    v[2] = float(s * 0.5 + 1.0);
    v[3] = float(c * 0.5 + 1.0);
    ShaderConstant_Set(kSinCos, 0x10, v);
    v[0] = float(ms_timecycColours[0] / 255.0);
    v[1] = float(ms_timecycColours[1] / 255.0);
    v[2] = float(ms_timecycColours[2] / 255.0);
    v[3] = 1.0f;
    ShaderConstant_Set(kColourA, 0x10, v);
    v[0] = float(ms_timecycColours[3] / 255.0);
    v[1] = float(ms_timecycColours[4] / 255.0);
    v[2] = float(ms_timecycColours[5] / 255.0);
    v[3] = 1.0f;
    ShaderConstant_Set(kColourB, 0x10, v);
}

// 0x401790: the current level's name (0x20E2280, 0x100)
static void __cdecl SetLevelName(const char* name) {
    if (name) {
        strncpy_s_(ms_levelName, 0x100, name, 0xFF);
        return;
    }
    ms_levelName[0] = 0;
}

// 0x4017C0 (stdcall): the compiler's `vector destructor iterator' (last to first)
static void __stdcall DestructArray(uint8_t* p, uint32_t size, int count, void(__thiscall* dtor)(void*)) {
    p += size * count;
    while (--count >= 0) {
        p -= size;
        dtor(p);
    }
}

} // namespace BullyMain

// ---- BullyNiDX9Renderer's small virtuals (thiscall) ----
namespace BullyNiDX9Renderer {

static uint8_t* Settings(uint8_t* self) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x6B4);
}

// 0x401800
static uint32_t __fastcall Get408(uint8_t* self, re::Edx) {
    return *reinterpret_cast<uint32_t*>(self + 0x408);
}

// 0x401810: the settings' +0xF4 flag
static uint8_t __fastcall GetSettingsFlagF4(uint8_t* self, re::Edx) {
    return *reinterpret_cast<int32_t*>(Settings(self) + 0xF4) != 0;
}

// 0x401830
static uint8_t __fastcall SetSettingsFlagF4(uint8_t* self, re::Edx, int value) {
    *reinterpret_cast<int32_t*>(Settings(self) + 0xF4) = re::Flag(value) ? 1 : 0;
    return 1;
}

// 0x401850
static uint32_t __fastcall Get684(uint8_t* self, re::Edx) {
    return *reinterpret_cast<uint32_t*>(self + 0x684);
}

// 0x401860: the depth clear value
static void __fastcall SetDepthClear(uint8_t* self, re::Edx, float value) {
    *reinterpret_cast<float*>(self + 0x400) = value;
}

// 0x401870
static float __fastcall GetDepthClear(uint8_t* self, re::Edx) {
    return *reinterpret_cast<float*>(self + 0x400);
}

static uint32_t ToByte(double v) {
    return uint32_t(int32_t(v)) & 0xFF;   // (fistp, truncating)
}

// 0x401880: the background colour from RGB, alpha 0xFF
static void __fastcall SetBackgroundColourRGB(uint8_t* self, re::Edx, const float* rgb) {
    uint32_t c = (ToByte(rgb[0] * 255.0) | 0xFFFFFF00u) << 8;
    c = (c | ToByte(rgb[1] * 255.0)) << 8;
    c |= ToByte(rgb[2] * 255.0);
    *reinterpret_cast<uint32_t*>(self + 0x3FC) = c;
}

// 0x401930: the background colour from RGBA
static void __fastcall SetBackgroundColourRGBA(uint8_t* self, re::Edx, const float* rgba) {
    uint32_t a = ToByte(rgba[3] * 255.0);
    uint32_t r = ToByte(rgba[0] * 255.0);
    uint32_t g = ToByte(rgba[1] * 255.0);
    uint32_t b = ToByte(rgba[2] * 255.0);
    *reinterpret_cast<uint32_t*>(self + 0x3FC) = a << 24 | r << 16 | g << 8 | b;
}

// 0x401A00
static void __fastcall GetBackgroundColour(uint8_t* self, re::Edx, float* rgba) {
    const double scale = double(1.0f / 255.0f);
    rgba[3] = float(self[0x3FF] * scale);
    rgba[0] = float(self[0x3FE] * scale);
    rgba[1] = float(self[0x3FD] * scale);
    rgba[2] = float(self[0x3FC] * scale);
}

// 0x401A60: the stencil clear value
static void __fastcall SetStencilClear(uint8_t* self, re::Edx, uint32_t value) {
    *reinterpret_cast<uint32_t*>(self + 0x404) = value;
}

// 0x401A70
static uint32_t __fastcall GetStencilClear(uint8_t* self, re::Edx) {
    return *reinterpret_cast<uint32_t*>(self + 0x404);
}

// 0x401A80: the settings' +0x88
static float __fastcall GetSettings88(uint8_t* self, re::Edx) {
    return *reinterpret_cast<float*>(Settings(self) + 0x88);
}

// 0x401A90: +0x88 (at least 1e-5) and +0x84 = 1/value - 1
static void __fastcall SetSettings88(uint8_t* self, re::Edx, float value) {
    uint8_t* s = Settings(self);
    if (value <= 0.0f) {
        value = 1e-05f;
    }
    *reinterpret_cast<float*>(s + 0x88) = value;
    *reinterpret_cast<float*>(s + 0x84) = float(1.0 / value - 1.0);
}

// 0x401AD0
static void __fastcall Fn401AD0(uint8_t* self, re::Edx, int a, int b) {
    NiDX9Renderer_Fn885E80(self, a, b);
}

// 0x401AF0
static void __fastcall SetFlagBCB804(uint8_t*, re::Edx, int value) {
    ms_bRendererFlag_BCB804 = uint8_t(value);
}

// 0x401B00: the class's RTTI
static void* __fastcall GetRTTI(uint8_t*, re::Edx) {
    return reinterpret_cast<void*>(0x00BCBBCC);
}

} // namespace BullyNiDX9Renderer

namespace BullyMain {

// 0x401B10 (thiscall): NiPointer<T>'s constructor (the refcount at +4)
static re::Ptr32<void>* __fastcall NiPointer_Construct(re::Ptr32<void>* self, re::Edx, uint8_t* object) {
    *self = object;
    if (object) {
        vc::Increment(object + 4);
    }
    return self;
}

// 0x401B30: a list node back to NiTPointerAllocator's free list
// (thiscall, the allocator in ecx unused; pops its argument)
static void __fastcall FreeListNode(void*, re::Edx, re::Ptr32<void>* node) {
    vc::Import<void>(vc::kEnterCriticalSection, ms_freeListLock);
    ms_freeListLockOwner = vc::Import<uint32_t>(vc::kGetCurrentThreadId);
    ms_freeListLockCount++;
    node[1] = nullptr;
    node[0] = ms_pFreeList;
    ms_freeListLockCount--;
    ms_pFreeList = node;
    if (ms_freeListLockCount == 0) {
        ms_freeListLockOwner = 0;
    }
    vc::Import<void>(vc::kLeaveCriticalSection, ms_freeListLock);
}

// 0x401B90: a new block of free list nodes (its first node links the blocks)
static void __cdecl AllocateFreeListBlock() {
    uint64_t bytes = uint64_t(ms_blockSize) * 0xC;
    uint8_t* block = static_cast<uint8_t*>(NiMalloc((bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes)));
    ms_pFreeList = block;
    *reinterpret_cast<re::Ptr32<void>*>(block + ms_blockSize * 0xC - 0xC) = nullptr;
    uint32_t i = 1;
    uint32_t offset = 0xC;
    uint32_t last;
    do {
        uint8_t* list = static_cast<uint8_t*>(ms_pFreeList);
        *reinterpret_cast<re::Ptr32<void>*>(list + offset) = list + offset + 0xC;
        last = i;
        i++;
        offset += 0xC;
    } while (last < ms_blockSize - 2);
    uint8_t* list = static_cast<uint8_t*>(ms_pFreeList);
    *reinterpret_cast<re::Ptr32<void>*>(list) = ms_pBlockList;   // (null when it's the first)
    ms_pBlockList = list;
    ms_pFreeList = list + 0xC;
}

// 0x401C30, 0x401C60 (thiscall): NiTListBase<NiTPointerAllocator, NiPointer<...>>'s
// deleting destructors
static void* __fastcall ClickListBase_Destroy(uint32_t* self, re::Edx, int flags) {
    *self = 0x008FF21C;
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}
static void* __fastcall StepListBase_Destroy(uint32_t* self, re::Edx, int flags) {
    *self = 0x008FF21C;
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x401C90: NiFixedString's destructor
static void __cdecl FixedString_Destruct(re::Ptr32<char>* handle) {
    char* s = *handle;
    if (s == ms_nullString) {
        return;
    }
    uint16_t length = *reinterpret_cast<uint16_t*>(s - 2);
    if (vc::Decrement(s - 8) == 1) {
        StringTable_Release(ms_pStringTable, handle, length);
    }
}

// 0x401CD0 (thiscall): NiFixedString's assignment (returns this, as operator= does: some
// callers return it)
static re::Ptr32<char>* __fastcall FixedString_Assign(re::Ptr32<char>* self, re::Edx, const re::Ptr32<char>* other) {
    char* s = *other;
    if (*self == s) {
        return self;
    }
    if (s != ms_nullString) {
        vc::Increment(s - 8);
    }
    char* old = *self;
    if (old != ms_nullString) {
        uint16_t length = *reinterpret_cast<uint16_t*>(old - 2);
        if (vc::Decrement(old - 8) == 1) {
            StringTable_Release(ms_pStringTable, self, length);
        }
    }
    *self = s;
    return self;
}

// 0x401D30 (thiscall): an entry of the array at +0xC8 (count at +0xCE), null past it
static void* __fastcall GetArrayC8(uint8_t* self, re::Edx, uint32_t index) {
    if (*reinterpret_cast<uint16_t*>(self + 0xCE) <= index) {
        return nullptr;
    }
    return (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self + 0xC8))[index];
}

} // namespace BullyMain

// ---- registration ----------------------------------------------------------------------

#define C00_APP_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace BullyMain {
C00_APP_REVERSED("BullyMain::CopyString", CopyString, 0x00401020);
C00_APP_REVERSED("NiFixedString::Construct", FixedString_Construct, 0x00401050);
C00_APP_REVERSED("NiFixedString::Copy", FixedString_Copy, 0x00401080);
C00_APP_REVERSED("BullyMain::OnWindowMessage", OnWindowMessage, 0x004010D0);
C00_APP_REVERSED("VectorConstructorIterator", ConstructArray, 0x00401240);
C00_APP_REVERSED("BullyMain::LoadAllShaders", LoadAllShaders, 0x00401280);
C00_APP_REVERSED("BullyMain::CallRenderer_C8", CallRenderer_C8, 0x004013A0);
C00_APP_REVERSED("BullyMain::UpdateTimeConstants", UpdateTimeConstants, 0x00401520);
C00_APP_REVERSED("BullyMain::SetLevelName", SetLevelName, 0x00401790);
C00_APP_REVERSED("VectorDestructorIterator", DestructArray, 0x004017C0);
C00_APP_REVERSED("NiPointer::Construct", NiPointer_Construct, 0x00401B10);
C00_APP_REVERSED("NiTPointerAllocator::FreeNode", FreeListNode, 0x00401B30);
C00_APP_REVERSED("NiTPointerAllocator::AllocateBlock", AllocateFreeListBlock, 0x00401B90);
C00_APP_REVERSED("NiTListBase<NiRenderClick>::Destroy", ClickListBase_Destroy, 0x00401C30);
C00_APP_REVERSED("NiTListBase<NiRenderStep>::Destroy", StepListBase_Destroy, 0x00401C60);
C00_APP_REVERSED("NiFixedString::Destruct", FixedString_Destruct, 0x00401C90);
C00_APP_REVERSED("NiFixedString::Assign", FixedString_Assign, 0x00401CD0);
C00_APP_REVERSED("BullyMain::GetArrayC8", GetArrayC8, 0x00401D30);
} // namespace BullyMain

namespace BullyNiDX9Renderer {
C00_APP_REVERSED("BullyNiDX9Renderer::Get408", Get408, 0x00401800);
C00_APP_REVERSED("BullyNiDX9Renderer::GetSettingsFlagF4", GetSettingsFlagF4, 0x00401810);
C00_APP_REVERSED("BullyNiDX9Renderer::SetSettingsFlagF4", SetSettingsFlagF4, 0x00401830);
C00_APP_REVERSED("BullyNiDX9Renderer::Get684", Get684, 0x00401850);
C00_APP_REVERSED("BullyNiDX9Renderer::SetDepthClear", SetDepthClear, 0x00401860);
C00_APP_REVERSED("BullyNiDX9Renderer::GetDepthClear", GetDepthClear, 0x00401870);
C00_APP_REVERSED("BullyNiDX9Renderer::SetBackgroundColourRGB", SetBackgroundColourRGB, 0x00401880);
C00_APP_REVERSED("BullyNiDX9Renderer::SetBackgroundColourRGBA", SetBackgroundColourRGBA, 0x00401930);
C00_APP_REVERSED("BullyNiDX9Renderer::GetBackgroundColour", GetBackgroundColour, 0x00401A00);
C00_APP_REVERSED("BullyNiDX9Renderer::SetStencilClear", SetStencilClear, 0x00401A60);
C00_APP_REVERSED("BullyNiDX9Renderer::GetStencilClear", GetStencilClear, 0x00401A70);
C00_APP_REVERSED("BullyNiDX9Renderer::GetSettings88", GetSettings88, 0x00401A80);
C00_APP_REVERSED("BullyNiDX9Renderer::SetSettings88", SetSettings88, 0x00401A90);
C00_APP_REVERSED("BullyNiDX9Renderer::Fn401AD0", Fn401AD0, 0x00401AD0);
C00_APP_REVERSED("BullyNiDX9Renderer::SetFlagBCB804", SetFlagBCB804, 0x00401AF0);
C00_APP_REVERSED("BullyNiDX9Renderer::GetRTTI", GetRTTI, 0x00401B00);
} // namespace BullyNiDX9Renderer
