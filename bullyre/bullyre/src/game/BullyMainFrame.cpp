// BullyMain's life cycle (0x4045F0 to 0x404F40): its constructor and destructors,
// CreateFrame (the render steps and clicks, the mirror, the light manager), and small
// virtuals.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_FN(void, __cdecl, DebugPrint, 0x0049A650, const char* format);   // (does nothing)
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, NiDelete_836F80, 0x00836F80, void* p);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, re::Ptr32<char>* other);
GAME_FN(void, __thiscall, NiUIBaseSlot0_Destruct, 0x00402CC0, void* self);
GAME_FN(void, __thiscall, NiUIBaseSlot1_Destruct, 0x00404290, void* self);
GAME_FN(void, __thiscall, NiApplication_Destruct, 0x0082A780, void* self);
GAME_FN(void, __thiscall, BullyMain_Fn411520, 0x00411520, void* self);
GAME_FN(void, __thiscall, BullyMain_DestructBase, 0x004045F0, void* self);
GAME_FN(void, __thiscall, BullyMain_Destruct, 0x00404850, void* self);

// CreateFrame's callees
GAME_FN(void, __cdecl, ShadowManager_SetClickGenerator, 0x00757DA0, re::Ptr32<char>* name);
GAME_FN(void, __cdecl, ShadowManager_SetCamera, 0x00757DF0, void* camera);
GAME_FN(void, __cdecl, ShadowManager_SetCullingProcess, 0x00757E90, void* process);
GAME_FN(void, __thiscall, NiRenderStep_Construct, 0x007650D0, void* self);
GAME_FN(uint8_t*, __thiscall, MdBullyMainViewRenderClick_Construct, 0x005162C0, void* self);
GAME_FN(void, __thiscall, ViewRenderClick_SetProcessor, 0x007D3E40, void* self, void* process);
GAME_FN(void, __thiscall, MainClick_SetCamera, 0x00515A70, void* self, void* camera);
GAME_FN(void, __thiscall, MainClick_SetScene, 0x00893860, void* self, void* scene);
GAME_FN(void, __thiscall, MainClick_SetClouds, 0x00515AC0, void* self, void* node);
GAME_FN(void, __thiscall, MainClick_SetRenderLast, 0x00515B10, void* self, void* node);
GAME_FN(void, __thiscall, MainClick_SetRenderClose, 0x00515B60, void* self, void* node);
GAME_FN(uint8_t*, __thiscall, MdBullyMirror_Construct, 0x00518730, void* self);
GAME_FN(void, __thiscall, MdBullyMirror_Initialise, 0x00518770, void* self);
GAME_FN(void, __thiscall, MainClick_SetMirror, 0x00515BB0, void* self, void* mirror);
GAME_FN(void, __thiscall, PointerList_AddTail, 0x007843D0, void* list, re::Ptr32<uint8_t>* object);
GAME_FN(uint8_t*, __thiscall, NiRenderFrame_Construct, 0x00404510, void* self);
GAME_FN(void*, __cdecl, LightManager_Get, 0x0040F7A0);
GAME_FN(void, __thiscall, LightManager_Initialise, 0x004104C0, void* self, void* frame, char* name);
GAME_VAR(uint8_t*, ms_pClouds, 0x00BCBBB8);
GAME_VAR(uint8_t*, ms_pRenderLast, 0x00BCBBC4);
GAME_VAR(uint8_t*, ms_pRenderClose, 0x00BCBBC8);
constexpr uint32_t kNiDefaultClickRenderStepVtable = 0x008FF5B4;
constexpr uint32_t kNiTPointerListRenderClickVtable = 0x008FF508;

GAME_VAR(uint8_t, ms_bInRendererCall, 0x00BCBB40);
GAME_FN(void, __thiscall, NiTArray_SetSize, 0x0084A110, void* self, uint32_t size);
GAME_FN(void, __thiscall, NiTArray_SetAt, 0x00833C70, void* self, uint32_t index, re::Ptr32<void>* item);

GAME_FN(void, __thiscall, NiApplication_Construct, 0x00832A20, void* self, const char* name, uint32_t width,
        uint32_t height, int, int, int, int);
GAME_VAR(uint32_t, ms_windowWidth, 0x009687DC);
GAME_VAR(uint32_t, ms_windowHeight, 0x009687E0);
GAME_FN(void, __cdecl, Fn_765460, 0x00765460);
GAME_FN(void, __cdecl, Fn_411410, 0x00411410);
GAME_FN(uint8_t*, __thiscall, BullyMain_Construct, 0x00404E20, void* self);
GAME_VAR(uint8_t*, ms_pApp, 0x00BCBB48);
GAME_FN(uint8_t, __thiscall, NiApplication_Initialise, 0x00830F80, void* self);
GAME_FN(void, __cdecl, ScreenFX_Initialise, 0x0055EB70);
GAME_VAR(uint8_t, ms_bInTitleMenu_20C32EB, 0x020C32EB);
GAME_VAR(uint8_t, ms_bInTitleMenu_20C32F2, 0x020C32F2);

constexpr uint32_t kBullyMainVtable = 0x008FF62C;

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

void Assign(re::Ptr32<uint8_t>& p, uint8_t* object) {
    if (p == object) {
        return;
    }
    Release(p);
    p = object;
    if (object) {
        vc::Increment(object + 4);
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

// a NiFixedString member named
void SetName(void* member, const char* text) {
    re::Ptr32<char> name = StringTable_AddString(text);
    FixedString_Assign(static_cast<re::Ptr32<char>*>(member), &name);
    ReleaseString(&name);
}

uint8_t* NewClickRenderStep() {
    uint8_t* step = static_cast<uint8_t*>(NiNew(0x3C));
    if (!step) {
        return nullptr;
    }
    NiRenderStep_Construct(step);
    U(step, 0) = kNiDefaultClickRenderStepVtable;
    U(step, 0x38) = 0;
    U(step, 0x30) = 0;
    U(step, 0x34) = 0;
    U(step, 0x2C) = kNiTPointerListRenderClickVtable;
    return step;
}

// a member NiUIMemberSlot's destructor, inlined
void DestructMemberSlot(uint8_t* slot, uint32_t vtable, bool slot1) {
    U(slot, 0) = vtable;
    U(slot, 0x10) = 0;
    U(slot, 0x18) = 0;
    U(slot, 0x1C) = 0;
    if (slot1) {
        NiUIBaseSlot1_Destruct(slot);
    } else {
        NiUIBaseSlot0_Destruct(slot);
    }
}

} // namespace

namespace BullyMain {

// 0x4045F0 (thiscall): the destructor of BullyMain's NiApplication side: its names, arrays,
// UI slots and pointers, then NiApplication's
static void __fastcall DestructBase(uint8_t* self, re::Edx) {
    ReleaseString(reinterpret_cast<re::Ptr32<char>*>(self + 0x224));
    ReleaseString(reinterpret_cast<re::Ptr32<char>*>(self + 0x220));
    ReleaseString(reinterpret_cast<re::Ptr32<char>*>(self + 0x21C));
    ReleaseString(reinterpret_cast<re::Ptr32<char>*>(self + 0x218));
    U(self, 0x204) = 0x008FF284;
    NiFree(*reinterpret_cast<re::Ptr32<void>*>(self + 0x208));
    U(self, 0x1F4) = 0x008FF27C;
    NiFree(*reinterpret_cast<re::Ptr32<void>*>(self + 0x1F8));
    Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1E0));
    Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1DC));
    U(self, 0x1CC) = 0x008FF5AC;
    NiDelete_836F80(*reinterpret_cast<re::Ptr32<void>*>(self + 0x1D0));
    Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1C4));
    U(self, 0x1AC) = 0x008FF274;
    NiFree(*reinterpret_cast<re::Ptr32<void>*>(self + 0x1B0));
    Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x1A8));
    DestructMemberSlot(self + 0x168, 0x008FF38C, false);
    DestructMemberSlot(self + 0x148, 0x008FF38C, false);
    DestructMemberSlot(self + 0x128, 0x008FF38C, false);
    DestructMemberSlot(self + 0x108, 0x008FF5A0, true);
    Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x100));
    Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xFC));
    NiApplication_Destruct(self);
}

// 0x404850 (thiscall): BullyMain's destructor
static void __fastcall Destruct(uint8_t* self, re::Edx) {
    U(self, 0) = kBullyMainVtable;
    BullyMain_Fn411520(self);
    Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x23C));
    Release(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x238));
    BullyMain_DestructBase(self);
}

// 0x4048B0 (thiscall)
static uint8_t __fastcall GetA0(uint8_t* self, re::Edx) {
    return self[0xA0];
}

// 0x4048C0 (thiscall)
static void __fastcall Set228(uint8_t* self, re::Edx, int value) {
    self[0x228] = uint8_t(value);
}

// 0x4048D0 (thiscall)
static uint8_t __fastcall Get228(uint8_t* self, re::Edx) {
    return self[0x228];
}

// 0x4048E0 (thiscall): CreateFrame: the shadow render step, the main render click
// (+0x238, with the camera's viewport, the culler, the scene and the layers), the mirror
// (+0x23C), the main render step, the render frame (+0xC) with both steps, then the light
// manager given the frame
static uint8_t __fastcall CreateFrame(uint8_t* self, re::Edx) {
    DebugPrint("CreateFrame");
    re::Ptr32<char> name = StringTable_AddString("NiDefaultShadowClickGenerator");
    ShadowManager_SetClickGenerator(&name);
    ReleaseString(&name);
    ShadowManager_SetCamera(*reinterpret_cast<re::Ptr32<void>*>(self + 8));
    ShadowManager_SetCullingProcess(*reinterpret_cast<re::Ptr32<void>*>(self + 0xB4));

    uint8_t* shadowStep = NewClickRenderStep();
    // (a failed allocation isn't checked from here on)
    SetName(shadowStep + 0x18, "Bully Shadow Render Step");
    U(shadowStep, 8) = 0x00829710;   // its pre-processing callback
    U(shadowStep, 0x10) = 0;

    void* p = NiNew(0x88);
    re::Ptr32<uint8_t>& click = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x238);
    Assign(click, p ? MdBullyMainViewRenderClick_Construct(p) : nullptr);
    SetName(click + 0x48, "Bully Main Render Click");
    uint8_t* camera = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 8);
    U(click, 0x14) = U(camera, 0x128);   // the viewport
    U(click, 0x18) = U(camera, 0x12C);
    U(click, 0x1C) = U(camera, 0x130);
    U(click, 0x20) = U(camera, 0x134);
    U(click, 0x34) = 7;
    ViewRenderClick_SetProcessor(click, *reinterpret_cast<re::Ptr32<void>*>(self + 0xB4));
    MainClick_SetCamera(click, camera);
    MainClick_SetScene(click, *reinterpret_cast<re::Ptr32<void>*>(self + 4));
    MainClick_SetClouds(click, ms_pClouds);
    MainClick_SetRenderLast(click, ms_pRenderLast);
    MainClick_SetRenderClose(click, ms_pRenderClose);

    p = NiNew(0x20);
    re::Ptr32<uint8_t>& mirror = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x23C);
    Assign(mirror, p ? MdBullyMirror_Construct(p) : nullptr);
    MdBullyMirror_Initialise(mirror);
    MainClick_SetMirror(click, mirror);

    uint8_t* mainStep = NewClickRenderStep();
    SetName(mainStep + 0x18, "Bully Main Render Step");
    re::Ptr32<uint8_t> held = click;
    if (held) {
        vc::Increment(held + 4);
    }
    PointerList_AddTail(mainStep + 0x2C, &held);
    Release(click);

    p = NiNew(0x2C);
    re::Ptr32<uint8_t>& frame = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xC);
    Assign(frame, p ? NiRenderFrame_Construct(p) : nullptr);
    SetName(frame + 0x28, "Bully Render Frame");
    held = shadowStep;
    vc::Increment(shadowStep + 4);
    PointerList_AddTail(frame + 8, &held);
    if (vc::Decrement(shadowStep + 4) == 0) {
        vc::Call<void>(shadowStep, 0x04);
    }
    held = mainStep;
    if (mainStep) {
        vc::Increment(mainStep + 4);
    }
    PointerList_AddTail(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xC) + 8, &held);
    Release(mainStep);

    DebugPrint("LightMgr_Init");
    char* frameName = *reinterpret_cast<re::Ptr32<char>*>(self + 0xBC);   // (by value: the callee releases it)
    if (frameName != ms_nullString) {
        vc::Increment(frameName - 8);
    }
    LightManager_Initialise(LightManager_Get(), *reinterpret_cast<re::Ptr32<void>*>(self + 0xC), frameName);
    DebugPrint(nullptr);
    DebugPrint(nullptr);
    return 1;
}

// 0x404DD0 (thiscall): the shader library callback (0x746470) added to the array at
// +0x1F4 (unless in the renderer's call)
static uint8_t __fastcall AddShaderLibraryCallback(uint8_t* self, re::Edx) {
    if (ms_bInRendererCall) {
        return 1;
    }
    uint8_t* array = self + 0x1F4;
    re::Ptr32<void> callback = reinterpret_cast<void*>(0x00746470);
    uint16_t size = *reinterpret_cast<uint16_t*>(array + 0xA);
    if (*reinterpret_cast<uint16_t*>(array + 8) <= size) {
        NiTArray_SetSize(array, uint32_t(*reinterpret_cast<uint16_t*>(array + 0xE)) + size);
    }
    NiTArray_SetAt(array, size, &callback);
    return 1;
}

// 0x404E20 (thiscall): BullyMain's constructor ("Bully", the window's size)
static uint8_t* __fastcall Construct(uint8_t* self, re::Edx) {
    NiApplication_Construct(self, "Bully", ms_windowWidth, ms_windowHeight, 1, 0, 0, 0);
    U(self, 0) = kBullyMainVtable;
    U(self, 0x238) = 0;
    U(self, 0x23C) = 0;
    self[0x228] = 0;
    Fn_765460();
    Fn_411410();
    return self;
}

// 0x404E80 (thiscall): BullyMain's deleting destructor
static uint8_t* __fastcall Destroy(uint8_t* self, re::Edx, int flags) {
    BullyMain_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x240);
    }
    return self;
}

// 0x404EB0: NiApplication::Create: the one BullyMain (0xBCBB48)
static uint8_t* __cdecl Create() {
    void* p = NiNew(0x240);
    ms_pApp = p ? BullyMain_Construct(p) : nullptr;
    return ms_pApp;
}

// 0x404EE0 (a virtual, `this` passed on in ecx): NiApplication's Initialize, then the
// screen effects
static uint8_t __fastcall Initialise(uint8_t* self, re::Edx) {
    if (!NiApplication_Initialise(self)) {
        return 0;
    }
    DebugPrint("ScrFX");
    ScreenFX_Initialise();
    DebugPrint(nullptr);
    return 1;
}

// 0x404F10 (thiscall): the callback at +8 called (self, a, b, &one, 1), its result added
// to +4 and returned (NiFile::Read: the bytes read)
static int __fastcall AccumulateCallback(uint8_t* self, re::Edx, int a, int b) {
    int one = 1;
    auto callback = reinterpret_cast<int(__cdecl*)(void*, int, int, int*, int)>(U(self, 8));
    int result = callback(self, a, b, &one, 1);
    *reinterpret_cast<int*>(self + 4) += result;
    return result;
}

// 0x404F40: in the title menu (either flag)
static uint32_t __cdecl IsInTitleMenu() {
    if (!ms_bInTitleMenu_20C32EB && !ms_bInTitleMenu_20C32F2) {
        return 0;
    }
    return 1;
}

} // namespace BullyMain

// ---- registration ----------------------------------------------------------------------

#define C00_FRAME_REVERSED(name, fn, address)                                                       \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace BullyMain {
C00_FRAME_REVERSED("BullyMain::DestructBase", DestructBase, 0x004045F0);
C00_FRAME_REVERSED("BullyMain::Destruct", Destruct, 0x00404850);
C00_FRAME_REVERSED("BullyMain::GetA0", GetA0, 0x004048B0);
C00_FRAME_REVERSED("BullyMain::Set228", Set228, 0x004048C0);
C00_FRAME_REVERSED("BullyMain::Get228", Get228, 0x004048D0);
C00_FRAME_REVERSED("BullyMain::CreateFrame", CreateFrame, 0x004048E0);
C00_FRAME_REVERSED("BullyMain::AddShaderLibraryCallback", AddShaderLibraryCallback, 0x00404DD0);
C00_FRAME_REVERSED("BullyMain::Construct", Construct, 0x00404E20);
C00_FRAME_REVERSED("BullyMain::Destroy", Destroy, 0x00404E80);
C00_FRAME_REVERSED("NiApplication::Create", Create, 0x00404EB0);
C00_FRAME_REVERSED("BullyMain::Initialise", Initialise, 0x00404EE0);
C00_FRAME_REVERSED("BullyMain::AccumulateCallback", AccumulateCallback, 0x00404F10);
C00_FRAME_REVERSED("BullyMain::IsInTitleMenu", IsInTitleMenu, 0x00404F40);
} // namespace BullyMain
