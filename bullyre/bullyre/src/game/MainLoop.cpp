// The top level: WinMain, the frame (GTA's Idle: the game's process, the world's
// rendering, the 2D and the HUD, the loading screen when there's no rendering) and the
// debug settings' table ("name" -> variable).

#include <cfloat>
#include <xmmintrin.h>
#include <cmath>
#include <cstdint>

#include <windows.h>

#include "log.h"
#include "reversed.h"
#include "runflags.h"

#include "game/Crt.h"
namespace {

template <typename F>
F Import(uintptr_t slot) {
    // (the slot is the game's 4 bytes: read as such on x64)
    return reinterpret_cast<F>(static_cast<void*>(*reinterpret_cast<re::Ptr32<void>*>(slot)));
}
constexpr uintptr_t IMP_GetModuleFileNameA = 0x008FC07C;
constexpr uintptr_t IMP_GetTickCount = 0x008FC078;
constexpr uintptr_t IMP_Sleep = 0x008FC16C;
constexpr uintptr_t IMP_SystemParametersInfoA = 0x008FC230;

using re::At;
template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

// a debug setting: its name and variable (the table ends with a null variable)
struct DebugSetting {
    re::Ptr32<const char> name;
    re::Ptr32<int32_t> variable;
};
GAME_ARRAY(DebugSetting, ms_debugSettings, 0x00A136E8);
GAME_VAR(uint8_t, ms_bDebugSettingsLocked, 0x00BF3816);

GAME_VAR(HINSTANCE, ms_hInstance, 0x00BD77B4);
GAME_VAR(int32_t, ms_cmdShow, 0x00BD77B0);
GAME_VAR(STICKYKEYS, ms_startupStickyKeys, 0x00A135E4);
GAME_VAR(TOGGLEKEYS, ms_startupToggleKeys, 0x00A135EC);
GAME_VAR(FILTERKEYS, ms_startupFilterKeys, 0x00A135F4);
GAME_VAR(uint8_t, ms_bSkipMoviesASAP, 0x00BCBB50);
GAME_VAR(uint8_t, ms_bAppCreated, 0x00BF3829);
GAME_VAR(int32_t*, ms_pApplication, 0x00BF3838);
GAME_VAR(uint32_t, ms_initialObjectCount, 0x00BF383C);
GAME_VAR(uint32_t, ms_objectCount, 0x020E2168);   // NiObject's count

GAME_VAR(uint8_t*, ms_pCompletion, 0x020CE75C);   // (+0x270: a reload wanted)
GAME_VAR(uint8_t, ms_bToLoad, 0x00BD0FD0);
GAME_VAR(uint8_t, ms_bReady, 0x00BD0FCB);
GAME_VAR(int32_t, ms_numPointLights, 0x00C92668);
GAME_VAR(int32_t, ms_lastChannelRead, 0x00CA3920);  // CStreaming's (-1: none)
GAME_VAR(uint8_t, ms_bReturnToMain, 0x020C32EC);
GAME_VAR(uint8_t*, ms_pExitString, 0x00C674E0);     // (+0x2C: set while exiting)
GAME_VAR(uint8_t, ms_bUserStart, 0x020C32E9);
GAME_VAR(uint8_t, ms_bFlag_20C32EA, 0x020C32EA);
GAME_VAR(uint8_t, ms_bFlag_20C32EB, 0x020C32EB);
GAME_VAR(uint8_t, ms_bFlag_20C32EE, 0x020C32EE);
GAME_VAR(uint8_t, ms_bFlag_20C32F2, 0x020C32F2);
GAME_ARRAY(uint8_t, ms_fadeColour, 0x00C3CD2C);     // r, g, b, a
GAME_VAR(int32_t, ms_cameraMode_C3CCF8, 0x00C3CCF8);
GAME_VAR(int16_t, ms_farClip_20C4820, 0x020C4820);
GAME_VAR(int16_t, ms_fogStart_20C4822, 0x020C4822);
GAME_VAR(void*, ms_camera, 0x00C1AE7C);
GAME_VAR(int32_t, g_rwCamera, 0x00C3CCF4);
GAME_VAR(int32_t*, ms_pOverlay_20C8704, 0x020C8704);   // (drawn before and after the 2D)
GAME_VAR(void*, ms_pEffectsB6F1EC, 0x00B6F1EC);
GAME_VAR(void*, ms_pEffects, 0x00B6F1D4);
GAME_VAR(uint8_t*, ms_pPlayerPed, 0x00C1AEA8);
GAME_VAR(int32_t, ms_bSniper_CF1070, 0x00CF1070);
GAME_VAR(int32_t, ms_sniper_CF1074, 0x00CF1074);
GAME_VAR(void*, ms_pHud_C674B4, 0x00C674B4);
GAME_VAR(int32_t, ms_pending_CA064C, 0x00CA064C);
GAME_VAR(uint8_t, ms_bPlayerDown_20C87A8, 0x020C87A8);
GAME_VAR(uint8_t, ms_bFirstFrameDrawn, 0x00BA688C);
GAME_VAR(uint8_t, ms_bPlayerHas140C, 0x00C67561);
GAME_VAR(int32_t, ms_fovOverrideOn, 0x00C3CDD4);
GAME_VAR(float, ms_fovOverride, 0x00C3CDD0);
GAME_VAR(float, ms_fov, 0x00C3CD04);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);
inline int32_t* ChannelTries(int channel) {   // CStreaming::ms_channel[n].tries
    return reinterpret_cast<int32_t*>(0x00CA38E0 + channel * 0x38);
}

inline int __cdecl IsLower(int c) { return crt::Islower(c); }
inline char* __cdecl StrRChr(char* s, int c) { return crt::Strrchr((const char*)s, c); }
inline int __cdecl StrICmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline void __cdecl Srand(unsigned seed) { crt::Srand((unsigned int)seed); }
GAME_FN(int, __cdecl, Snprintf_402E00, 0x00402E00, char* out, size_t size, const char* format, ...);
GAME_FN(void, __cdecl, SetGamePath, 0x0082A5C0, const char* path);
GAME_FN(void, __cdecl, ParseCommandLine, 0x00464490, const char* line, int* argc, re::Ptr32<char>* argv);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int, __cdecl, RsEventHandler, 0x00410C00, int event, uintptr_t param);
GAME_FN(void, __cdecl, Fn_5EE580, 0x005EE580);
GAME_FN(void, __cdecl, Fn_5EE5A0, 0x005EE5A0);
GAME_FN(void, __cdecl, Fn_45B8A0, 0x0045B8A0);
GAME_FN(void, __thiscall, Text_Load, 0x0068F4E0, void* text);
GAME_FN(void, __thiscall, Camera_Init_4F3000, 0x004F3000, void* camera);
GAME_FN(void, __cdecl, Fn_42EDD0, 0x0042EDD0);
GAME_FN(void, __cdecl, Fn_42E4D0, 0x0042E4D0);
GAME_FN(void, __thiscall, App_Fn_82A000, 0x0082A000, void* app);
GAME_FN(void, __cdecl, Fn_753750, 0x00753750);
GAME_FN(void, __thiscall, Fn_5DBB40, 0x005DBB40, void* self, int);
GAME_FN(void, __cdecl, Fn_734EE0, 0x00734EE0, void*);
GAME_FN(void, __cdecl, Fn_68A250, 0x0068A250);
GAME_FN(void, __cdecl, Game_Process_42FEB0, 0x0042FEB0);
GAME_FN(void, __thiscall, Fn_597610, 0x00597610, void* self);
GAME_FN(void, __cdecl, Fn_4D0110, 0x004D0110);
GAME_FN(uint8_t, __cdecl, Fn_404F40, 0x00404F40);
GAME_FN(int, __thiscall, Camera_GetFadeState, 0x004F12F0, void* camera);
GAME_FN(uint8_t, __thiscall, Fn_524290, 0x00524290, void* self);
GAME_FN(void, __cdecl, Fn_465890, 0x00465890);
GAME_FN(void, __cdecl, Fn_453A00, 0x00453A00);
GAME_FN(uint8_t, __cdecl, Fn_405740, 0x00405740);
GAME_FN(void, __cdecl, Fn_451530, 0x00451530);
GAME_FN(void, __cdecl, Camera_SetFarClip, 0x006C8780, void* camera, float distance);
GAME_FN(void, __cdecl, Camera_Fn_6C8300, 0x006C8300, void* camera, int);
GAME_FN(void, __cdecl, Fn_527470, 0x00527470);
GAME_FN(void, __cdecl, Fn_5EE1D0, 0x005EE1D0);
GAME_FN(void, __thiscall, Camera_Fn_41AAC0, 0x0041AAC0, void* camera);
GAME_FN(void, __cdecl, App_Fn_43AED0, 0x0043AED0);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __thiscall, Fn_578150, 0x00578150, void* self, uintptr_t camera);
GAME_FN(void, __thiscall, Effects_Render, 0x00668780, void* effects, int);
GAME_FN(void, __thiscall, Fn_5387D0, 0x005387D0, void* self);
GAME_FN(int, __thiscall, Camera_Fn_4F1220, 0x004F1220, void* camera);
GAME_FN(void, __thiscall, Fn_72D520, 0x0072D520, void* self, int);
GAME_FN(void, __cdecl, Fn_52AC70, 0x0052AC70, int);
GAME_FN(void, __cdecl, Fn_5251C0, 0x005251C0);
GAME_FN(void, __cdecl, EmptyFunction, 0x0049A650);   // (just a ret)
GAME_FN(void, __cdecl, Fn_527800, 0x00527800);
GAME_FN(void, __thiscall, Hud_Fn_53CF50, 0x0053CF50, void* hud);
GAME_FN(void, __cdecl, Fn_513020, 0x00513020);
GAME_FN(void, __cdecl, Fn_4E8CE0, 0x004E8CE0);
GAME_FN(void, __cdecl, Fn_452EC0, 0x00452EC0);
GAME_FN(void, __thiscall, Fn_513780, 0x00513780, void* self);
GAME_FN(void, __cdecl, Fn_528FE0, 0x00528FE0);
GAME_FN(void, __cdecl, Fn_452B20, 0x00452B20, uint8_t* ped);
GAME_FN(void, __cdecl, Fn_404090, 0x00404090);
GAME_FN(void, __cdecl, Fn_404120, 0x00404120);
GAME_FN(void, __cdecl, App_Fn_43AEB0, 0x0043AEB0);
GAME_FN(void, __cdecl, Fn_55E2C0, 0x0055E2C0);
GAME_FN(void, __cdecl, Fn_409810, 0x00409810, int);
GAME_FN(void, __thiscall, Fn_546340, 0x00546340, void* self);
GAME_FN(void, __cdecl, Fn_4D9A60, 0x004D9A60);
GAME_FN(void, __cdecl, App_Fn_43AF10, 0x0043AF10);
GAME_FN(uint8_t, __cdecl, Fn_8283D0, 0x008283D0, int);
GAME_FN(uint8_t, __cdecl, Fn_407430, 0x00407430);
GAME_FN(void, __cdecl, Fn_407550, 0x00407550, uintptr_t camera);
GAME_FN(void, __cdecl, Fn_706600, 0x00706600);
GAME_FN(void, __cdecl, Fn_707510, 0x00707510);
GAME_FN(void, __thiscall, Fn_456E30, 0x00456E30, void* self);
GAME_FN(void, __cdecl, Fn_4096F0, 0x004096F0);
GAME_FN(void, __cdecl, Fn_4094B0, 0x004094B0, int);
GAME_FN(float, __thiscall, Camera_GetAspectRatio, 0x004F1360, void* camera);
GAME_FN(void, __cdecl, Camera_SetView, 0x005EF8A0, void* camera, uintptr_t, float tanHalfFov, float aspect);
GAME_FN(void, __cdecl, Camera_Fn_5F2EB0, 0x005F2EB0, void* camera);
GAME_FN(int, __cdecl, Camera_BeginUpdate, 0x006C86A0, void* camera);
GAME_FN(void, __cdecl, Fn_514EF0, 0x00514EF0);
GAME_FN(void, __cdecl, App_Fn_43B030, 0x0043B030);
GAME_FN(void, __cdecl, App_FrameEnd, 0x0043B870);
GAME_FN(void, __cdecl, App_EndLoadingScreenFrame, 0x0043ACE0, int);
GAME_FN(void, __thiscall, Fn_6C0A00, 0x006C0A00, void* self);
GAME_FN(void, __thiscall, Fn_545190, 0x00545190, void* self);
GAME_FN(void, __cdecl, Fn_45B960, 0x0045B960, int);
// (ours, called where modes apply)
GAME_FN(void, __cdecl, App_AllowShortcutKeys, 0x0043A9A0, int allow);
GAME_FN(void, __cdecl, App_OnDisplayChange, 0x0043AA90, const char* cmdLine);
GAME_FN(int, __cdecl, App_Create, 0x0043B6D0);
GAME_FN(void, __cdecl, Game_Initialise, 0x0043CF20);
GAME_FN(uint8_t, __cdecl, App_BeginWorldCameraUpdate, 0x0043B580, int, int);
GAME_FN(void, __cdecl, App_Begin2D, 0x0043AF90);
GAME_FN(void, __cdecl, App_ApplicationFrame, 0x0043AE60);
GAME_FN(void, __cdecl, App_RenderSplash, 0x0043C060, int, int);
GAME_FN(void, __cdecl, App_DrawCreditLines, 0x0043B140);
GAME_FN(uint8_t, __cdecl, StringsDiffer, 0x0043EA00, const char* a, const char* b);
GAME_FN(int32_t*, __cdecl, DebugSetting_Find, 0x0043F330, const char* name);

void Set2DRenderStates() {
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
}

bool NoUserStart() {
    return !ms_bUserStart && !ms_bFlag_20C32EA && !Fn_404F40();
}

} // namespace

namespace MainLoop {

// 0x43C960: the frame (GTA's Idle)
static void __cdecl Idle() {
    re::RunScheduled();   // (a test run's teleports: runflags.h)
    // (testing: "trap_nan = 2" also unmasks x87's invalid operation each frame, the device's
    // creation having reset it: the first NaN made anywhere on this thread stops the game)
    // ("= 1": SSE's only, kept unmasked here too in case the game's start-up reset it)
    static const int trapNan = re::IniInt("trap_nan", 0);
    if (trapNan >= 1 && trapNan <= 3 && (_mm_getcsr() & 0x680u)) {
        _mm_setcsr((_mm_getcsr() & ~0x3Fu) & ~0x680u);   // (invalid, divide by zero, overflow)
        re::Log("trap_nan: SSE invalid was masked again; unmasked");
    }
    static bool watching = false;
    if (trapNan == 3 && !watching) {
        watching = true;   // (TheCamera's position z: dllmain's handler arms DR0 on it)
        ULONG_PTR address = 0x00C3CC68 + 0x38;
        RaiseException(0xE0BADCA7, 0, 1, &address);
    }
    if (trapNan == 2) {
        _clearfp();
#if defined(_M_IX86)
        unsigned int x87 = 0;
        __control87_2(0, _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW, &x87, nullptr);
#else
        _controlfp(0, _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW);   // (no x87 state: SSE's)
#endif
    }
    if (ms_pCompletion[0x270]) {
        if (ms_bToLoad) {
            PushMemoryTag(0x23);
            Fn_5DBB40(reinterpret_cast<void*>(0x00D02850), 0);
            PopMemoryTag();
            Fn_734EE0(reinterpret_cast<void*>(0x020C9068));
            ms_pCompletion[0x270] = 0;
            ms_bToLoad = 0;
            ms_bReady = 1;
        }
    } else if (ms_bToLoad) {
        ms_bToLoad = 0;
        ms_bReady = 1;
    }
    Fn_68A250();
    ms_numPointLights = 0;
    PushMemoryTag(0x1A);
    Game_Process_42FEB0();
    PopMemoryTag();
    Fn_597610(reinterpret_cast<void*>(0x00CF3DA0));
    Fn_4D0110();
    if ((ms_lastChannelRead != -1 && *ChannelTries(ms_lastChannelRead) >= 3000) || ms_bReturnToMain) {
        return;
    }
    PushMemoryTag(0x1E);
    bool exiting = At<int32_t>(ms_pExitString, 0x2C) != 0;
    bool render;
    if (ms_bUserStart || ms_bFlag_20C32EA || Fn_404F40() || exiting) {
        render = false;
    } else if (Camera_GetFadeState(TheCamera) == 1 && ms_fadeColour[3] == 0) {
        render = false;
    } else {
        render = true;
    }
    uint8_t flag524290 = 0;
    if (render) {
        flag524290 = Fn_524290(reinterpret_cast<void*>(0x00C92D70));
    }
    Fn_465890();
    if (!ms_bFlag_20C32EB && !ms_bFlag_20C32F2) {
        Fn_453A00();
    }
    bool cameraMode2 = ms_cameraMode_C3CCF8 == 2;
    if (Fn_405740()) {
        if (render) {
            Fn_451530();
            Camera_SetFarClip(ms_camera, float(int32_t(ms_farClip_20C4820)));
            Camera_Fn_6C8300(ms_camera, ms_fogStart_20C4822);
            if (!App_BeginWorldCameraUpdate(flag524290, 0)) {
                App_EndLoadingScreenFrame(0);
                goto end;
            }
            Fn_527470();
            Fn_5EE1D0();
            Camera_Fn_41AAC0(TheCamera);
            App_Fn_43AED0();
            if (ms_pOverlay_20C8704) {
                Set2DRenderStates();
                VCall<void>(ms_pOverlay_20C8704, 0xC);
            }
            if (NoUserStart()) {
                Fn_578150(ms_pEffectsB6F1EC, g_rwCamera);
                Effects_Render(ms_pEffects, 0);
            }
            Fn_5387D0(reinterpret_cast<void*>(0x00CE7580));
            if (ms_pPlayerPed) {
                int n = Camera_Fn_4F1220(TheCamera);
                if (cameraMode2 && n > 0) {
                    ms_bSniper_CF1070 = 1;
                    ms_sniper_CF1074 = n;
                } else {
                    ms_bSniper_CF1070 = 0;
                }
            }
            Fn_72D520(reinterpret_cast<void*>(0x020C8A00), 0);
            Fn_52AC70(0);
            Fn_5251C0();
            EmptyFunction();
            Fn_527800();
            Hud_Fn_53CF50(ms_pHud_C674B4);
            Fn_513020();
            Fn_4E8CE0();
            Fn_452EC0();
            Fn_513780(reinterpret_cast<void*>(0x00C66B78));
            if (ms_pending_CA064C != 0) {
                Fn_528FE0();
                ms_pending_CA064C = 0;
            }
            uint8_t* ped = ms_pPlayerPed;
            if (ped[0x1DE7] < 0xFF) {
                Fn_5EE1D0();
                SetRenderState(0xE, 1);
                Fn_452B20(ped);
                SetRenderState(0xE, 0);
            }
            App_ApplicationFrame();
            Fn_404090();
            Fn_404120();
            App_Fn_43AEB0();
            Fn_55E2C0();
            Fn_409810(4);
            Fn_546340(ms_pExitString);
            if (cameraMode2 && ms_pPlayerPed) {
                Fn_4D9A60();
            }
            if (NoUserStart()) {
                App_Fn_43AF10();
            }
            if (!Fn_8283D0(0x16)) {
                App_Begin2D();
            }
            if (ms_pOverlay_20C8704) {
                Set2DRenderStates();
                VCall<void>(ms_pOverlay_20C8704, 8);
            }
            if (NoUserStart() && Fn_407430()) {
                Fn_409810(4);
                Fn_407550(g_rwCamera);
                Effects_Render(ms_pEffects, 1);
            }
            Fn_706600();
            if (!ms_bPlayerDown_20C87A8) {
                Fn_707510();
            }
            Fn_456E30(reinterpret_cast<void*>(0x00C18748));
            Fn_5EE1D0();
            if (!ms_bFirstFrameDrawn) {
                ms_bFirstFrameDrawn = 1;
            }
        } else {
            if (!Fn_407430()) {
                Fn_4096F0();
                Fn_4094B0(7);
            }
            float fov = ms_fovOverrideOn == 0 ? ms_fov : ms_fovOverride;
            float aspect = Camera_GetAspectRatio(TheCamera);
            float half = float(double(fov) * 0.008726646502812704);
            float tanHalf = float(std::tan(double(half)));   // (0x85C600: the CRT's x87 tan)
            Camera_SetView(ms_camera, 0, tanHalf, aspect);
            Camera_Fn_5F2EB0(ms_camera);
            Camera_BeginUpdate(ms_camera);
            if (exiting) {
                Set2DRenderStates();
                ms_bPlayerHas140C = At<int32_t>(ms_pPlayerPed, 0x140C) != 0;
                Fn_514EF0();
            }
        }
    }
    if (Fn_405740()) {
        App_Fn_43B030();
        if (ms_bFlag_20C32EE) {
            PopMemoryTag();
            return;
        }
        App_RenderSplash(0, 0);
        App_FrameEnd();
        App_DrawCreditLines();
        App_EndLoadingScreenFrame(0);
    }
end:
    Fn_6C0A00(reinterpret_cast<void*>(0x020C4910));
    Fn_545190(ms_pExitString);
    Fn_45B960(0);
    PopMemoryTag();
}

// 0x43E7D0 (stdcall): WinMain: the accessibility settings saved (and their hotkeys off),
// the game's folder, "--skipMoviesASAP", the application made and initialised, the
// game's initialisation, the main loop (the application's vf3), and the shutdown
static int __stdcall WinMain(HINSTANCE instance, HINSTANCE, LPSTR cmdLine, int cmdShow) {
    ms_hInstance = instance;
    ms_cmdShow = cmdShow;
    auto spi = Import<BOOL(WINAPI*)(UINT, UINT, PVOID, UINT)>(IMP_SystemParametersInfoA);
    spi(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &ms_startupStickyKeys, 0);
    spi(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &ms_startupToggleKeys, 0);
    spi(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &ms_startupFilterKeys, 0);
    App_AllowShortcutKeys(0);
    Fn_5EE580();
    char path[512];
    if (!Import<DWORD(WINAPI*)(HMODULE, LPSTR, DWORD)>(IMP_GetModuleFileNameA)(nullptr, path, 0x200)) {
        return 0;
    }
    StrRChr(path, '\\')[1] = '\0';
    SetGamePath(path);
    int argc = 0;
    re::Ptr32<char> argv[16];
    ParseCommandLine(cmdLine, &argc, argv);
    for (int i = 0; i < argc; i++) {
        if (StrICmp(argv[i], "--skipMoviesASAP") == 0) {
            ms_bSkipMoviesASAP = 1;
        }
    }
    App_OnDisplayChange(cmdLine);
    PushMemoryTag(1);
    if (!RsEventHandler(0x18, 0)) {
        return 0;
    }
    Srand(Import<DWORD(WINAPI*)()>(IMP_GetTickCount)());
    Fn_45B8A0();
    Text_Load(TheText);
    if (!ms_bAppCreated) {
        App_Create();
        ms_bAppCreated = 1;
    }
    Camera_Init_4F3000(TheCamera);
    Fn_42EDD0();
    Game_Initialise();
    Fn_42E4D0();
    RsEventHandler(0x16, 0);
    RsEventHandler(0x19, 0);
    VCall<void>(ms_pApplication, 0xC);   // the main loop
    App_Fn_82A000(ms_pApplication);
    if (ms_pApplication) {
        VCall<void>(ms_pApplication, 0, 1);   // deleted
    }
    Fn_753750();
    spi(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &ms_startupStickyKeys, 0);
    spi(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &ms_startupToggleKeys, 0);
    spi(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &ms_startupFilterKeys, 0);
    char message[256];
    uint32_t count = ms_objectCount;
    Snprintf_402E00(message, 0x100, "\nGamebryo NiObject counts:  initial = %u, final = %u. ", ms_initialObjectCount, count);
    if (count > ms_initialObjectCount) {
        Snprintf_402E00(message, 0x100, "Application is leaking %u objects\n\n", count - ms_initialObjectCount);
    }
    Import<void(WINAPI*)(DWORD)>(IMP_Sleep)(250);
    Fn_5EE5A0();
    return 0;
}

// 0x43EA00: do the names differ (ASCII case ignored)
static uint8_t __cdecl StringsDifferImpl(const char* a, const char* b) {
    while (*a) {
        char ca = *a;
        if (IsLower(ca)) {
            ca -= 0x20;
        }
        char cb = *b;
        if (IsLower(*b)) {
            cb -= 0x20;
        }
        if (ca != cb) {
            return 1;
        }
        a++;
        b++;
    }
    return *b != 0;
}

// 0x43F330: a debug setting's variable by its name (nullptr: none)
static int32_t* __cdecl FindDebugSetting(const char* name) {
    for (DebugSetting* s = ms_debugSettings;; s++) {
        if (!StringsDiffer(name, s->name)) {
            return s->variable;
        }
        if (!s->variable) {
            return nullptr;
        }
    }
}

// 0x43F370: a debug setting set (unless they're locked)
static void __cdecl SetDebugSetting(const char* name, int32_t value) {
    if (ms_bDebugSettingsLocked) {
        return;
    }
    if (int32_t* variable = DebugSetting_Find(name)) {
        *variable = value;
    }
}

} // namespace MainLoop

// ---- registration ----------------------------------------------------------------------

#define MAINLOOP_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace MainLoop {
MAINLOOP_REVERSED("Game::Idle", 0x0043C960, &Idle);
MAINLOOP_REVERSED("WinMain", 0x0043E7D0, &WinMain);
MAINLOOP_REVERSED("StringsDiffer", 0x0043EA00, &StringsDifferImpl);
MAINLOOP_REVERSED("DebugSettings::Find", 0x0043F330, &FindDebugSetting);
MAINLOOP_REVERSED("DebugSettings::Set", 0x0043F370, &SetDebugSetting);
} // namespace MainLoop
