// 0x404090 to 0x4045D0: BullyMain's small frame helpers and shader system set-up, and
// Gamebryo's render frame and render step classes (NiRenderFrame, NiDefaultClickRenderStep),
// the NiUI signal slots' unsubscribing, and the shadow map arrays' deleting destructor.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_VAR(uint8_t*, ms_pApp, 0x00BCBB48);   // BullyMain
GAME_FN(void, __cdecl, Fn_409810, 0x00409810, int);
GAME_FN(void, __thiscall, Culler_Fn516170, 0x00516170, void* self);
GAME_FN(void, __thiscall, ClearChildren18, 0x00402E30, void* node);
GAME_FN(void, __thiscall, BullyMain_Fn4040C0, 0x004040C0, void* self);
GAME_FN(void*, __cdecl, Fn_40F7A0, 0x0040F7A0);
GAME_FN(void, __thiscall, Fn_40EC50, 0x0040EC50, void* self);
GAME_FN(void, __thiscall, Fn_518720, 0x00518720, void* self);
GAME_VAR(uint8_t*, ms_pClouds, 0x00BCBBB8);
GAME_VAR(uint8_t*, ms_pLitObjects, 0x00BCBBBC);
GAME_VAR(uint8_t*, ms_pUnlitObjects, 0x00BCBBC0);
GAME_VAR(uint8_t*, ms_pRenderLast, 0x00BCBBC4);
GAME_VAR(uint8_t*, ms_pRenderClose, 0x00BCBBC8);

GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, void* self);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, size_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __thiscall, UIBaseSlot0_Unsubscribe, 0x00404130, void* self, void* other);
GAME_FN(void, __thiscall, NiUIBaseSlot0_Destruct, 0x00402CC0, void* self);
GAME_FN(void, __thiscall, NiUIBaseSlot1_Destruct, 0x00404290, void* self);
GAME_FN(void, __thiscall, NiShadowMap_DestructMember, 0x00402C00, void* owner);
GAME_FN(void, __thiscall, NiDefaultClickRenderStep_Destruct, 0x004043A0, void* self);
GAME_FN(void, __thiscall, NiRenderFrame_Destruct, 0x00404570, void* self);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_VAR(char*, ms_renderFrameDefaultName, 0x020E2810);
inline void* const ms_numRefObjects = reinterpret_cast<void*>(0x020E2168);

// the shader system
GAME_FN(void, __cdecl, DebugPrint, 0x0049A650, const char* format);   // (does nothing)
GAME_FN(void, __cdecl, ShaderFactory_SetErrorCallback, 0x007566B0, void* callback);
GAME_FN(uint32_t, __cdecl, GetShaderSettings, 0x00405D10, void* settings, int);
GAME_VAR(uint8_t, ms_bInRendererCall, 0x00BCBB40);
GAME_VAR(uint8_t, ms_bLoadGlobalShaders, 0x00BCBB41);
GAME_VAR(uint8_t, ms_bLoadGlobalDebugShaders, 0x00BCBB42);
GAME_FN(void*, __cdecl, ShaderLibrary_Get, 0x00756620);
GAME_FN(void, __cdecl, Fn_756650, 0x00756650);
GAME_FN(void, __cdecl, Fn_756630, 0x00756630);
GAME_FN(void*, __cdecl, ShaderProgramPath_Create, 0x00829900, const char* path);
GAME_FN(void, __cdecl, ShaderFactory_AddProgramPath, 0x007566E0, void* path);
GAME_VAR(const char*, ms_shaderPath_9D3428, 0x009D3428);
GAME_VAR(const char*, ms_shaderPath_9D342C, 0x009D342C);
GAME_FN(void, __stdcall, LoadGlobalShaderList, 0x00403CD0, int debug);
GAME_FN(void, __cdecl, MaterialLibrary_Fn405C90, 0x00405C90);

constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uint32_t kNiShadowMapVtable = 0x008FF368;

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

void ReleaseString(re::Ptr32<char>* handle) {
    char* s = *handle;
    if (s == ms_nullString) {
        return;
    }
    uint16_t length = *reinterpret_cast<uint16_t*>(s - 2);
    if (vc::Decrement(s - 8) == 1) {
        StringTable_Release(ms_pStringTable, handle, length);
    }
}

uint32_t& U(void* p, int offset) {
    return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + offset);
}

} // namespace

namespace BullyMain {

// 0x404090: 0x409810(4) around the culler's 0x516170 (when there is one)
static void __cdecl CullerFn() {
    uint8_t* app = ms_pApp;
    if (!app) {
        return;
    }
    Fn_409810(4);
    void* culler = *reinterpret_cast<re::Ptr32<void>*>(app + 0x238);
    if (culler) {
        Culler_Fn516170(culler);
    }
    Fn_409810(4);
}

// 0x4040C0 (thiscall): the layers' children's +0x18 cleared, then 0x40EC50 and 0x518720
static void __fastcall ClearLayers(uint8_t* self, re::Edx) {
    ClearChildren18(ms_pClouds);
    ClearChildren18(ms_pLitObjects);
    ClearChildren18(ms_pUnlitObjects);
    ClearChildren18(ms_pRenderClose);
    ClearChildren18(ms_pRenderLast);
    Fn_40EC50(Fn_40F7A0());
    Fn_518720(*reinterpret_cast<re::Ptr32<void>*>(self + 0x23C));
}

// 0x404120: the same on the application, when there is one
static void __cdecl ClearAppLayers() {
    if (ms_pApp) {
        BullyMain_Fn4040C0(ms_pApp);
    }
}

// 0x404420 (thiscall): CreateShaderSys: the shader factory's error callback, program
// paths and libraries (vtable[51], [52]), then the global shader lists
static uint8_t __fastcall CreateShaderSystem(uint8_t* self, re::Edx) {
    uint8_t settings[0x10];
    DebugPrint("CreateShaderSys");
    ShaderFactory_SetErrorCallback(reinterpret_cast<void*>(0x00824EC0));
    uint32_t shaderSettings = GetShaderSettings(settings, 4);
    if (ms_bInRendererCall) {
        vc::Call<void>(ShaderLibrary_Get(), 0x2C, "NiD3DXEffectShaderLib");
        Fn_756650();
        Fn_756630();
    }
    ShaderFactory_AddProgramPath(ShaderProgramPath_Create("Shaders/Common"));
    ShaderFactory_AddProgramPath(ShaderProgramPath_Create(ms_shaderPath_9D3428));
    if (!vc::Call<uint8_t>(self, 0xCC, ms_shaderPath_9D342C) ||
        !vc::Call<uint8_t>(self, 0xD0, shaderSettings, settings)) {
        DebugPrint(nullptr);
        return 0;
    }
    if (ms_bLoadGlobalShaders) {
        LoadGlobalShaderList(0);
    }
    if (ms_bLoadGlobalDebugShaders) {
        LoadGlobalShaderList(1);
    }
    MaterialLibrary_Fn405C90();
    DebugPrint(nullptr);
    return 1;
}

} // namespace BullyMain

namespace NiUISlot {

// 0x404130 (thiscall): `other` taken off this one's list (the last moved into its place),
// then this one off `other`'s (slots and signals keep each other's lists)
static void __fastcall Unsubscribe(uint8_t* self, re::Edx, uint8_t* other) {
    uint32_t count = U(self, 0xC);
    re::Ptr32<uint8_t>* list = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 4);
    for (uint32_t i = 0; i < count; i++) {
        if (list[i] == other) {
            U(self, 0xC)--;
            list = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 4);
            list[i] = list[U(self, 0xC)];
            UIBaseSlot0_Unsubscribe(other, self);
            return;
        }
    }
}

// 0x404270 (thiscall): NiUIBaseSlot0's deleting destructor
static uint8_t* __fastcall Slot0_Destroy(uint8_t* self, re::Edx, int flags) {
    NiUIBaseSlot0_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x404290 (thiscall): NiUIBaseSlot1's destructor: off every signal it's on
static void __fastcall Slot1_Destruct(uint8_t* self, re::Edx) {
    U(self, 0) = 0x008FF594;
    while (U(self, 0xC) != 0) {
        uint8_t* signal = **reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 4);
        uint32_t count = U(signal, 0xC);
        re::Ptr32<uint8_t>* slots = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(signal + 4);
        for (uint32_t i = 0; i < count; i++) {
            if (slots[i] != self) {
                continue;
            }
            U(signal, 0xC)--;
            slots = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(signal + 4);
            slots[i] = slots[U(signal, 0xC)];
            re::Ptr32<uint8_t>* mine = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 4);
            for (uint32_t j = 0; j < U(self, 0xC); j++) {
                if (mine[j] == signal) {
                    U(self, 0xC)--;
                    mine = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 4);
                    mine[j] = mine[U(self, 0xC)];
                    UIBaseSlot0_Unsubscribe(signal, self);
                    break;
                }
            }
            break;
        }
        // (a signal not listing the slot loops for ever: the game's)
    }
    NiFree(*reinterpret_cast<re::Ptr32<void>*>(self + 4));
}

// 0x404330 (thiscall): NiUIBaseSlot1's deleting destructor
static uint8_t* __fastcall Slot1_Destroy(uint8_t* self, re::Edx, int flags) {
    NiUIBaseSlot1_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x404350 (thiscall): NiUIMemberSlot1<NiSample>'s deleting destructor
static uint8_t* __fastcall MemberSlot1_Destroy(uint8_t* self, re::Edx, int flags) {
    U(self, 0x10) = 0;
    U(self, 0x1C) = 0;
    U(self, 0) = 0x008FF5A0;
    U(self, 0x18) = 0;
    NiUIBaseSlot1_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x20);
    }
    return self;
}

} // namespace NiUISlot

namespace NiRenderSteps {

// 0x404180 (thiscall): NiTPointerListBase<NiRenderStep>'s deleting destructor
static void* __fastcall StepPointerList_Destroy(uint32_t* self, re::Edx, int flags) {
    *self = 0x008FF264;
    NiTPointerList_RemoveAll(self);
    *self = 0x008FF254;
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x4041B0 (thiscall): the vector deleting destructor of the objects (0x18) that hold an
// NiShadowMap at +8
static uint8_t* __fastcall ShadowMapHolder_Destroy(uint8_t* self, re::Edx, int flags) {
    if (!(flags & 2)) {
        NiShadowMap_DestructMember(self);
        if (flags & 1) {
            NiFreeSized(self, 0x18);
        }
        return self;
    }
    uint8_t* block = self - 4;
    int count = *reinterpret_cast<int*>(block);
    for (int i = count - 1; i >= 0; i--) {
        uint8_t* map = self + i * 0x18 + 8;
        U(map, 0) = kNiShadowMapVtable;
        Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(map + 0xC));
        Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(map + 8));
    }
    if (flags & 1) {
        NiFreeArray(block, 0x18);
    }
    return block;
}

// 0x404390: NiDefaultClickRenderStep's RTTI
static void* __cdecl DefaultClickRenderStep_GetRTTI() {
    return reinterpret_cast<void*>(0x020E27DC);
}

// 0x4043A0 (thiscall): NiDefaultClickRenderStep's destructor: its click list (+0x2C)
// emptied, its name (+0x18) released, NiRefObject's
static void __fastcall DefaultClickRenderStep_Destruct(uint8_t* self, re::Edx) {
    uint32_t* list = reinterpret_cast<uint32_t*>(self + 0x2C);
    *list = 0x008FF244;
    NiTPointerList_RemoveAll(list);
    *list = 0x008FF21C;
    ReleaseString(reinterpret_cast<re::Ptr32<char>*>(self + 0x18));
    U(self, 0) = kNiRefObjectVtable;
    vc::Decrement(ms_numRefObjects);
}

// 0x404400 (thiscall): its deleting destructor
static uint8_t* __fastcall DefaultClickRenderStep_Destroy(uint8_t* self, re::Edx, int flags) {
    NiDefaultClickRenderStep_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x3C);
    }
    return self;
}

// 0x404510 (thiscall): NiRenderFrame's constructor (its name the default one)
static uint8_t* __fastcall RenderFrame_Construct(uint8_t* self, re::Edx) {
    U(self, 0) = kNiRefObjectVtable;
    U(self, 4) = 0;
    vc::Increment(ms_numRefObjects);
    U(self, 0) = 0x008FF61C;
    U(self, 0x14) = 0;
    U(self, 0xC) = 0;
    U(self, 0x10) = 0;
    U(self, 8) = 0x008FF584;   // NiTPointerList<NiPointer<NiRenderStep>>
    U(self, 0x18) = 0;
    U(self, 0x1C) = 0;
    U(self, 0x20) = 0;
    U(self, 0x24) = 0;
    if (ms_renderFrameDefaultName != ms_nullString) {
        vc::Increment(ms_renderFrameDefaultName - 8);
    }
    *reinterpret_cast<re::Ptr32<char>*>(self + 0x28) = ms_renderFrameDefaultName;
    return self;
}

// 0x404570 (thiscall): NiRenderFrame's destructor
static void __fastcall RenderFrame_Destruct(uint8_t* self, re::Edx) {
    ReleaseString(reinterpret_cast<re::Ptr32<char>*>(self + 0x28));
    uint32_t* list = reinterpret_cast<uint32_t*>(self + 8);
    *list = 0x008FF264;
    NiTPointerList_RemoveAll(list);
    *list = 0x008FF254;
    U(self, 0) = kNiRefObjectVtable;
    vc::Decrement(ms_numRefObjects);
}

// 0x4045D0 (thiscall): its deleting destructor
static uint8_t* __fastcall RenderFrame_Destroy(uint8_t* self, re::Edx, int flags) {
    NiRenderFrame_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x2C);
    }
    return self;
}

} // namespace NiRenderSteps

// ---- registration ----------------------------------------------------------------------

#define C00_STEPS_REVERSED(name, fn, address)                                                       \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace BullyMain {
C00_STEPS_REVERSED("BullyMain::CullerFn", CullerFn, 0x00404090);
C00_STEPS_REVERSED("BullyMain::ClearLayers", ClearLayers, 0x004040C0);
C00_STEPS_REVERSED("BullyMain::ClearAppLayers", ClearAppLayers, 0x00404120);
C00_STEPS_REVERSED("BullyMain::CreateShaderSystem", CreateShaderSystem, 0x00404420);
} // namespace BullyMain
namespace NiUISlot {
C00_STEPS_REVERSED("NiUIBaseSlot0::Unsubscribe", Unsubscribe, 0x00404130);
C00_STEPS_REVERSED("NiUIBaseSlot0::Destroy", Slot0_Destroy, 0x00404270);
C00_STEPS_REVERSED("NiUIBaseSlot1::Destruct", Slot1_Destruct, 0x00404290);
C00_STEPS_REVERSED("NiUIBaseSlot1::Destroy", Slot1_Destroy, 0x00404330);
C00_STEPS_REVERSED("NiUIMemberSlot1<NiSample>::Destroy", MemberSlot1_Destroy, 0x00404350);
} // namespace NiUISlot
namespace NiRenderSteps {
C00_STEPS_REVERSED("NiTPointerListBase<NiRenderStep>::Destroy", StepPointerList_Destroy, 0x00404180);
C00_STEPS_REVERSED("ShadowMapHolder::Destroy", ShadowMapHolder_Destroy, 0x004041B0);
C00_STEPS_REVERSED("NiDefaultClickRenderStep::GetRTTI", DefaultClickRenderStep_GetRTTI, 0x00404390);
C00_STEPS_REVERSED("NiDefaultClickRenderStep::Destruct", DefaultClickRenderStep_Destruct, 0x004043A0);
C00_STEPS_REVERSED("NiDefaultClickRenderStep::Destroy", DefaultClickRenderStep_Destroy, 0x00404400);
C00_STEPS_REVERSED("NiRenderFrame::Construct", RenderFrame_Construct, 0x00404510);
C00_STEPS_REVERSED("NiRenderFrame::Destruct", RenderFrame_Destruct, 0x00404570);
C00_STEPS_REVERSED("NiRenderFrame::Destroy", RenderFrame_Destroy, 0x004045D0);
} // namespace NiRenderSteps
