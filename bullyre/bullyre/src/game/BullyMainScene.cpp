// BullyMain's scene (0x4035E0 to 0x404090): CreateScene (the root node and its render
// layers, the camera), the per-frame scene update and render, and the global shader list.

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_FN(void, __cdecl, DebugPrint, 0x0049A650, const char* format);   // (does nothing)
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);

GAME_VAR(void*, ms_pShadowManager, 0x020E2410);
GAME_FN(void, __cdecl, ShadowManager_Fn757340, 0x00757340, int);
GAME_FN(void, __cdecl, ShadowManager_Fn7573A0, 0x007573A0, int, int);
GAME_FN(void*, __thiscall, MdBullyMaterialSwapProcessor_Construct, 0x005185C0, void* self);
GAME_FN(void, __cdecl, ShadowManager_SetProcessor, 0x00402AE0, void* processor, int index);
GAME_VAR(void*, ms_pShadowClickFactory, 0x020E23B8);
constexpr uint32_t kMdShadowRenderClickFactoryVtable = 0x008FF348;
inline void* const ms_renderer_20E25E8 = reinterpret_cast<void*>(0x020E25E8);
GAME_FN(uint8_t*, __thiscall, NiNode_Construct, 0x00760780, void* self, int childCount);
GAME_FN(void, __thiscall, NiObjectNET_SetName, 0x0077C3A0, void* self, re::Ptr32<char>* name);
GAME_FN(uint8_t*, __thiscall, NiCamera_Construct, 0x00763C20, void* self);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E040, void* object);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E240, void* object);

GAME_VAR(uint8_t*, ms_pLights, 0x00BCBBB4);
GAME_VAR(uint8_t*, ms_pClouds, 0x00BCBBB8);
GAME_VAR(uint8_t*, ms_pLitObjects, 0x00BCBBBC);
GAME_VAR(uint8_t*, ms_pUnlitObjects, 0x00BCBBC0);
GAME_VAR(uint8_t*, ms_pRenderLast, 0x00BCBBC4);
GAME_VAR(uint8_t*, ms_pRenderClose, 0x00BCBBC8);

GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, void* self);
GAME_FN(void*, __cdecl, Fn_40F7A0, 0x0040F7A0);
GAME_FN(void, __thiscall, Fn_410BF0, 0x00410BF0, void* self);
GAME_FN(void, __stdcall, UpdateChildren, 0x004026B0, void* node, int flag);

// the shader list's callees (the C runtime's, called where they are)
inline FILE* __cdecl fopen_(const char* name, const char* mode) { return crt::Fopen(name, mode); }
inline char* __cdecl fgets_(char* buffer, int size, FILE* file) { return crt::Fgets(buffer, size, file); }
inline int __cdecl fclose_(FILE* file) { return crt::Fclose(file); }
inline void* __cdecl memset_(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
inline char* __cdecl strchr_(const char* s, int c) { return crt::Strchr(s, c); }
inline int __cdecl strncmp_(const char* a, const char* b, size_t n) { return crt::Strncmp(a, b, n); }
constexpr uintptr_t kGlobalMemoryStatus = 0x008FC03C;
GAME_FN(void, __cdecl, Shaders_LoadHashed, 0x00517D60, const char* name);
GAME_FN(void, __cdecl, Shaders_Load, 0x00517CE0, const char* name);
GAME_FN(void, __cdecl, ScreenFX_LoadShaders, 0x0055FD90);
GAME_FN(int, __cdecl, FormatString, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __cdecl, FixPath, 0x00752C20, char* path);
GAME_FN(uint8_t, __cdecl, FileExists, 0x007533C0, const char* path, int);
GAME_FN(void, __cdecl, ShaderInclude_Load, 0x00407580, const char* fxl, const char* fxb, uintptr_t, uintptr_t);
GAME_VAR(const char*, ms_shaderDirectory, 0x009D3454);
GAME_VAR(const char*, ms_shaderBinaryDirectory, 0x009D3BA8);

// the frame's render
GAME_VAR(uint8_t*, ms_pCamera, 0x00C3CCF4);
GAME_FN(void, __cdecl, UpdateTimeConstants, 0x00401520);
GAME_VAR(uint8_t, ms_bResetBounds_20C5BE6, 0x020C5BE6);
GAME_FN(uint8_t, __thiscall, Fn_50DF50, 0x0050DF50, void* self);
GAME_FN(void, __cdecl, ResetBounds, 0x00402760, void* object);
GAME_FN(void, __cdecl, ShadowManager_SetCamera, 0x00757DF0, void* camera);
GAME_FN(void, __thiscall, Culler_SetCamera, 0x00515A70, void* self, void* camera);
GAME_FN(void, __cdecl, Fn_5218A0, 0x005218A0);
GAME_FN(void, __thiscall, RenderFrame_GatherStatistics, 0x00402B90, void* self, void* count, void* a, void* b);

void Release(uint8_t* object) {
    if (vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

void Assign(re::Ptr32<uint8_t>& p, uint8_t* object) {
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

uint8_t* NewNode() {
    void* p = NiNew(0xF4);
    return p ? NiNode_Construct(p, 0) : nullptr;
}

void SetName(void* object, const char* text) {
    re::Ptr32<char> name = StringTable_AddString(text);
    NiObjectNET_SetName(object, &name);
    ReleaseString(&name);
}

void UpdateAll(void* object) {
    AVObject_Update(object, 0.0f, 1);
    AVObject_UpdateEffects(object);
    AVObject_UpdateProperties(object);
}

// 0x4013C0 takes the camera in esi
__declspec(noinline) void UpdateFogConstants(uint8_t* camera) {
#if defined(_M_IX86)
    __asm {
        mov esi, camera
        mov eax, 0x004013C0
        call eax
    }
#else
    re::Ours<void (*)(uint8_t*)>(0x004013C0)(camera);
#endif
}

} // namespace

namespace BullyMain {

// 0x4035E0 (thiscall): CreateScene: the shadow manager's material swap processors, the
// root ("Bully Root", +4) with its layers (Lights, Lit Objects and Un-lit Objects under
// it; Clouds, Render Close and Render Last on their own), the camera (+8)
static uint8_t __fastcall CreateScene(uint8_t* self, re::Edx) {
    DebugPrint("CreateScene");
    void* factory = ms_pShadowClickFactory;
    if (ms_pShadowManager) {
        ShadowManager_Fn757340(3);
        ShadowManager_Fn7573A0(1, 0);
        void* p = NiNew(0x10);
        void* processor = p ? MdBullyMaterialSwapProcessor_Construct(p) : nullptr;
        ShadowManager_SetProcessor(processor, 0);
        ShadowManager_SetProcessor(processor, 1);
        ShadowManager_SetProcessor(processor, 2);
        factory = NiNew(4);
        if (factory) {
            *reinterpret_cast<uint32_t*>(factory) = kMdShadowRenderClickFactoryVtable;
        } else {
            factory = nullptr;
        }
        if (ms_pShadowClickFactory) {
            NiFreeSized(ms_pShadowClickFactory, 4);
        }
    }
    ms_pShadowClickFactory = factory;
    vc::Call<void>(*reinterpret_cast<re::Ptr32<void>*>(self + 0x24), 0x5C, ms_renderer_20E25E8);

    re::Ptr32<uint8_t>& root = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 4);
    Assign(root, NewNode());
    SetName(root, "Bully Root");
    Assign(ms_pLights, NewNode());
    SetName(ms_pLights, "Lights");
    vc::Call<void>(root, 0x88, ms_pLights, 0);   // AttachChild
    Assign(ms_pClouds, NewNode());
    SetName(ms_pClouds, "Clouds");
    Assign(ms_pLitObjects, NewNode());
    SetName(ms_pLitObjects, "Lit Objects");
    vc::Call<void>(root, 0x88, ms_pLitObjects, 0);
    Assign(ms_pUnlitObjects, NewNode());
    SetName(ms_pUnlitObjects, "Un-lit Objects");
    vc::Call<void>(root, 0x88, ms_pUnlitObjects, 0);
    Assign(ms_pRenderClose, NewNode());
    SetName(ms_pRenderClose, "Render Close");
    Assign(ms_pRenderLast, NewNode());
    SetName(ms_pRenderLast, "Render Last");
    void* p = NiNew(0x13C);
    Assign(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 8), p ? NiCamera_Construct(p) : nullptr);
    UpdateAll(root);
    UpdateAll(ms_pClouds);
    UpdateAll(ms_pRenderLast);
    DebugPrint(nullptr);   // (the stack's leftovers)
    return 1;
}

// 0x403C10 (thiscall): NiTPointerListBase<NiRenderClick>'s deleting destructor
static void* __fastcall ClickPointerList_Destroy(uint32_t* self, re::Edx, int flags) {
    *self = 0x008FF244;
    NiTPointerList_RemoveAll(self);
    *self = 0x008FF21C;
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x403C40 (thiscall): the frame's scene update: vtable[39] and [9], 0x410BF0, vtable[62],
// then each layer's children updated (Lit Objects with the flag)
static void __fastcall UpdateScene(uint8_t* self, re::Edx) {
    vc::Call<void>(self, 0x9C);
    vc::Call<void>(self, 0x24);
    Fn_410BF0(Fn_40F7A0());
    vc::Call<void>(self, 0xF8);
    UpdateChildren(ms_pLights, 0);
    UpdateChildren(ms_pClouds, 0);
    UpdateChildren(ms_pLitObjects, 1);
    UpdateChildren(ms_pUnlitObjects, 0);
    UpdateChildren(ms_pRenderClose, 0);
    UpdateChildren(ms_pRenderLast, 0);
}

// 0x403CD0 (stdcall; a virtual not using `this`): the shaders named in
// GlobalShaderList.txt (or the debug one): BullyDef_ and Md ones (with a hash suffix
// of 8 hex digits, or not), the screen effects once, and Bully ones from their .fxl
// and .fxb files. (Every line asks for the memory status and ignores it)
static void __stdcall LoadGlobalShaderList(int debug) {
    FILE* file = fopen_(re::Flag(debug) ? "GlobalDebugShaderList.txt" : "GlobalShaderList.txt", "r");
    if (!file) {
        return;
    }
    bool screenFxLoaded = false;
    char line[128];
    line[0] = 0;
    memset_(line + 1, 0, 0x7F);
    uint8_t memoryStatus[0x20];
    char fxl[260];
    char fxb[260];
    while (fgets_(line, 0x80, file)) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        *strchr_(line, '.') = 0;
        vc::Import<void>(kGlobalMemoryStatus, memoryStatus);
        if (strncmp_(line, "BullyDef_", 9) == 0 || (line[0] == 'M' && line[1] == 'd')) {
            int length = int(strlen(line));
            bool hashed = length > 8;
            for (int i = 0; hashed && i < 8; i++) {
                if (!strchr_("ABCDEF0123456789", line[length - 8 + i])) {
                    hashed = false;
                }
            }
            if (hashed) {
                Shaders_LoadHashed(line);
            } else {
                Shaders_Load(line);
            }
        } else if (strncmp_(line, "ScrFX_", 6) == 0) {
            if (!screenFxLoaded) {
                ScreenFX_LoadShaders();
                screenFxLoaded = true;
            }
        } else if (strncmp_(line, "Bully", 5) == 0) {
            FormatString(fxl, 0x104, "%s\\%s.fxl", ms_shaderDirectory, line);
            FixPath(fxl);
            if (FileExists(fxl, 0)) {
                FormatString(fxb, 0x104, "%s\\%s.fxb", ms_shaderBinaryDirectory, line);
                FixPath(fxb);
                ShaderInclude_Load(fxl, fxb, 0, 0);
            }
        }
    }
    fclose_(file);
}

// 0x403F10 (thiscall): the frame's render: the camera (0xC3CCF4) taken and updated, the
// fog and time constants, the layers' bounds reset (unless 0x50DF50 says not), the
// shadow manager and culler given the camera, then the render frame's steps drawn with
// their callbacks, and its statistics
static void __fastcall RenderFrame(uint8_t* self, re::Edx) {
    re::Ptr32<uint8_t>& camera = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 8);
    Assign(camera, ms_pCamera);
    AVObject_Update(camera, 0.0f, 1);
    UpdateFogConstants(ms_pCamera);
    UpdateTimeConstants();
    if (ms_bResetBounds_20C5BE6 || !Fn_50DF50(reinterpret_cast<void*>(0x00A45A30))) {
        ResetBounds(ms_pClouds);
        ResetBounds(ms_pLitObjects);
        ResetBounds(ms_pUnlitObjects);
        ResetBounds(ms_pRenderLast);
        ResetBounds(ms_pRenderClose);
    }
    if (camera) {
        if (ms_pShadowManager) {
            ShadowManager_SetCamera(camera);
        }
        Culler_SetCamera(*reinterpret_cast<re::Ptr32<void>*>(self + 0x238), camera);
    }
    // NiRenderFrame: +0xC its steps, +0x18/+0x20 the pre-render callback and its data,
    // +0x1C/+0x24 the post-render ones. A step: +8/+0x10 pre, +0xC/+0x14 post, +0x1C active
    using Callback = uint8_t(__cdecl*)(void*, void*);
    uint8_t* frame = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xC);
    Callback pre = re::GameFnAt<Callback>(frame + 0x18);
    if (pre && !pre(frame, *reinterpret_cast<re::Ptr32<void>*>(frame + 0x20))) {
        return;
    }
    for (uint8_t* node = *reinterpret_cast<re::Ptr32<uint8_t>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xC) + 0xC); node;) {
        uint8_t* step = *reinterpret_cast<re::Ptr32<uint8_t>*>(node + 8);
        node = *reinterpret_cast<re::Ptr32<uint8_t>*>(node);
        if (!step[0x1C]) {
            continue;
        }
        Callback stepPre = re::GameFnAt<Callback>(step + 8);
        if (stepPre && !stepPre(step, *reinterpret_cast<re::Ptr32<void>*>(step + 0x10))) {
            continue;
        }
        vc::Call<void>(step, 0x20);   // Render
        Callback stepPost = re::GameFnAt<Callback>(step + 0xC);
        if (stepPost) {
            stepPost(step, *reinterpret_cast<re::Ptr32<void>*>(step + 0x14));
        }
    }
    Fn_5218A0();
    frame = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xC);
    Callback post = re::GameFnAt<Callback>(frame + 0x1C);
    if (post && !post(frame, *reinterpret_cast<re::Ptr32<void>*>(frame + 0x24))) {
        return;
    }
    RenderFrame_GatherStatistics(frame, self + 0x6C, self + 0x70, self + 0x74);
}

} // namespace BullyMain

// ---- registration ----------------------------------------------------------------------

#define C00_SCENE_REVERSED(name, fn, address)                                                       \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace BullyMain {
C00_SCENE_REVERSED("BullyMain::CreateScene", CreateScene, 0x004035E0);
C00_SCENE_REVERSED("NiTPointerListBase<NiRenderClick>::Destroy", ClickPointerList_Destroy, 0x00403C10);
C00_SCENE_REVERSED("BullyMain::UpdateScene", UpdateScene, 0x00403C40);
C00_SCENE_REVERSED("BullyMain::LoadGlobalShaderList", LoadGlobalShaderList, 0x00403CD0);
C00_SCENE_REVERSED("BullyMain::RenderFrame", RenderFrame, 0x00403F10);
} // namespace BullyMain
