// The display settings (0x404F60 to 0x406140): the registry (HKCU\Software\Rockstar Games\
// Bully Scholarship Edition: ADAPTER, then per adapter RESW, RESH, AA, VS, SHA, WIN), the
// adapter's modes and multisample levels, changing the mode, activating and deactivating
// the application, the frame limiter. Also XACT's engine creation, inlined here.
//
// 0x405580 to 0x405FA4 is patched in place by bullygfx (windowed mode): the functions in
// it are written but not hooked.

#include <cstdint>
#include <cstdio>

#include "game/VCall.h"
#include "reversed.h"
#include "runflags.h"

#include "game/Crt.h"
#include "game/XactShim.h"
namespace {

// the game's import slots used here
constexpr uintptr_t kRegQueryValueExA = 0x008FC000;
constexpr uintptr_t kRegOpenKeyExA = 0x008FC004;
constexpr uintptr_t kRegSetValueExA = 0x008FC008;
constexpr uintptr_t kRegOpenKeyA = 0x008FC00C;
constexpr uintptr_t kRegCreateKeyExA = 0x008FC010;
constexpr uintptr_t kRegCloseKey = 0x008FC014;
constexpr uintptr_t kQueryPerformanceCounter = 0x008FC040;
constexpr uintptr_t kQueryPerformanceFrequency = 0x008FC044;
constexpr uintptr_t kWaitForSingleObject = 0x008FC048;
constexpr uintptr_t kReleaseMutex = 0x008FC04C;
constexpr uintptr_t kSleep = 0x008FC16C;
constexpr uintptr_t kChangeDisplaySettingsA = 0x008FC20C;
constexpr uintptr_t kTranslateMessage = 0x008FC210;
constexpr uintptr_t kDispatchMessageA = 0x008FC214;
constexpr uintptr_t kPeekMessageA = 0x008FC218;
constexpr uintptr_t kGetMonitorInfoA = 0x008FC21C;
constexpr uintptr_t kMoveWindow = 0x008FC220;
constexpr uintptr_t kShowWindow = 0x008FC224;
constexpr uintptr_t kSetWindowPos = 0x008FC228;
constexpr uintptr_t kCoCreateInstance = 0x008FC32C;

using HKEY_ = void*;
constexpr uintptr_t HKEY_CURRENT_USER_ = 0x80000001;
constexpr uintptr_t HKEY_LOCAL_MACHINE_ = 0x80000002;

// the settings: the current mode at 0xBCBC60, a pending one at 0xBCBC78
struct VideoMode {
    int32_t width;          // +0x00 RESW
    int32_t height;         // +0x04 RESH
    int32_t antialiasing;   // +0x08 AA (the multisample level)
    uint32_t vsync;         // +0x0C VS (a bool in the low byte)
    uint32_t shaders;       // +0x10 SHA (0..3: off, low, medium, high)
    uint32_t windowed;      // +0x14 WIN (a bool in the low byte)
};
static_assert(sizeof(VideoMode) == 0x18);
GAME_VAR(VideoMode, ms_mode, 0x00BCBC60);

// the mode windowed: its WIN setting, or always in bullyre_x64 (runflags.h)
static bool Windowed() {
    return uint8_t(ms_mode.windowed) != 0 || re::ForceWindowed();
}
GAME_VAR(VideoMode, ms_pendingMode, 0x00BCBC78);
GAME_VAR(uint32_t, ms_adapter, 0x00BCBC24);
GAME_ARRAY(const char*, ms_registryNames, 0x009D3B84);   // key, ADAPTER, RESW, RESH, AA, WIN, SHA, VS

GAME_VAR(int32_t, ms_pixelShader30Slots, 0x00BCBC18);
GAME_VAR(uint32_t, ms_adapterCount, 0x00BCBC1C);
GAME_VAR(uint32_t, ms_vendor, 0x00BCBC20);   // 2 ATI, 1 NVIDIA, 0 other
GAME_VAR(uint64_t, ms_videoMemory, 0x00BCBC28);
GAME_VAR(uint64_t, ms_availableMemory, 0x00BCBC30);
GAME_VAR(uint8_t, ms_bShadersOn, 0x00BCBC38);
GAME_VAR(uint8_t, ms_bAppActive, 0x00BCBC39);
GAME_VAR(uint8_t, ms_bAudioPaused, 0x00BCBC3A);
GAME_VAR(uint8_t, ms_bGameStarted, 0x00BCBC3B);
GAME_VAR(uint32_t, ms_shaderQuality, 0x00BCBC3C);
GAME_VAR(int32_t, ms_pauseCountdown, 0x00BCBC44);
GAME_VAR(int32_t, ms_fpsLimit, 0x00BCBC48);
GAME_VAR(uint8_t, ms_bPausedByFocus, 0x00BCBC4C);
GAME_VAR(int64_t, ms_lastFrameTime, 0x00BCBC50);
GAME_VAR(int64_t, ms_performanceFrequency, 0x00BCBC58);
GAME_ARRAY(int32_t, ms_modes, 0x00BCBC90);   // width, height pairs
GAME_VAR(int32_t, ms_numModes, 0x00BCC090);
GAME_ARRAY(int32_t, ms_multisampleLevels, 0x00BCC098);
GAME_VAR(int32_t, ms_numMultisampleLevels, 0x00BCC0D8);
GAME_VAR(uint8_t, ms_bHardwareOk, 0x009D3BA4);
GAME_VAR(uint8_t, ms_bFirstSettingsRead, 0x009D3BA5);
GAME_VAR(uint8_t, ms_bOwnsMutex, 0x00BCC114);
GAME_VAR(void*, ms_mutex, 0x020DCDAC);
GAME_VAR(void*, ms_window, 0x00BD77FC);   // RsGlobal's
inline void* const ms_message = reinterpret_cast<void*>(0x00BCC0DC);
inline void* const ms_inactiveMessage = reinterpret_cast<void*>(0x00BCC0F8);

GAME_VAR(void*, ms_pD3D, 0x020E6D04);   // IDirect3D9
GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
GAME_VAR(uint8_t*, ms_pApp, 0x00BCBB48);
GAME_VAR(uint8_t, ms_bWidescreen, 0x00C1870E);
GAME_VAR(float, ms_fade, 0x00C3CD28);
GAME_VAR(uint8_t, ms_bUserStart_20C32E9, 0x020C32E9);
GAME_VAR(uint8_t, ms_bUserStart_20C32EA, 0x020C32EA);
GAME_VAR(uint8_t, ms_bInTitleMenu_20C32EB, 0x020C32EB);
GAME_VAR(uint8_t, ms_bInTitleMenu_20C32F2, 0x020C32F2);
GAME_VAR(uint8_t, ms_bLoaded_BF3818, 0x00BF3818);
GAME_VAR(uint8_t, ms_bPaused_C1A998, 0x00C1A998);
GAME_VAR(uint8_t, ms_bFlag_CF68E8, 0x00CF68E8);
GAME_VAR(uint8_t, ms_bAudioInitialised, 0x00CF68D8);
GAME_VAR(uint8_t, ms_bAudio_CF3DD5, 0x00CF3DD5);
GAME_VAR(uint32_t, ms_audio_CF68DC, 0x00CF68DC);
GAME_VAR(uint32_t, ms_audio_CF68E4, 0x00CF68E4);
GAME_VAR(int32_t, ms_gameState_BA6894, 0x00BA6894);
GAME_VAR(void*, ms_pPlayerPed_C1AEA8, 0x00C1AEA8);
GAME_ARRAY(float, ms_screenScales, 0x00A957A8);   // 8: x, 1, 1/x, 1, y, 1, 1/y, 1

template <typename... A>
inline int __cdecl sprintf_(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
inline void* __cdecl memset_(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
inline int64_t __stdcall alldiv(int64_t a, int64_t b) { return crt::Alldiv(a, b); }
inline void __cdecl exit_(int code) { crt::Exit(code); }
GAME_FN(long, __stdcall, DirectDrawCreateEx_, 0x0074646A, void* guid, void** dd, const void* iid, void* outer);   // (the import: a native out-pointer)
GAME_FN(int, __cdecl, Renderer_GetDeviceId, 0x00878D10);
GAME_FN(uint8_t, __cdecl, RegReadBool, 0x00405080, HKEY_ key, const char* name, uint8_t* out);
GAME_FN(uint8_t, __cdecl, RegReadInt, 0x004050E0, HKEY_ key, const char* name, uint32_t* out);
GAME_FN(void, __cdecl, ReadAdapter, 0x00405140);
GAME_FN(uint8_t, __cdecl, ReadModeSettings, 0x004051D0);
GAME_FN(void, __cdecl, WriteSettings, 0x00405330);
GAME_FN(void, __cdecl, ReadVideoMemory, 0x004054D0);
GAME_FN(uint8_t, __cdecl, ChangeMode, 0x00405580, const VideoMode* mode);
GAME_FN(void, __cdecl, EnumerateModes, 0x00405770);
GAME_FN(void, __cdecl, DefaultSettings, 0x00405A00);
GAME_FN(uint8_t, __cdecl, IsModeListed, 0x00405A70, const VideoMode* mode);
GAME_FN(uint8_t, __cdecl, AreSettingsValid, 0x00405AB0);
GAME_FN(uint8_t, __cdecl, WaitForDevice, 0x00405B40, uint32_t tries);
GAME_FN(void, __cdecl, GetDisplaySettings, 0x00405BA0, VideoMode* out);
GAME_FN(uint8_t, __cdecl, ApplyVideoMode_, 0x00405F60);
GAME_FN(uint32_t, __cdecl, IsInFrontEnd, 0x00405050);
GAME_FN(void, __cdecl, Fn_517650, 0x00517650);
GAME_FN(void, __cdecl, CallRenderer_C8, 0x004013A0);
GAME_FN(void, __thiscall, Renderer_Fn8806C0, 0x008806C0, void* renderer);
GAME_FN(void*, __cdecl, ShaderLibrary_Get, 0x00756620);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_FN(void, __cdecl, ShaderConstant_Set, 0x00756C70, void* constant, int size, const void* data);
GAME_FN(void, __cdecl, ShowSystemMessage, 0x0043B0F0, const char* key);
GAME_FN(void, __thiscall, Audio_Resume, 0x00595130, void* self);
GAME_FN(void, __thiscall, Audio_Pause, 0x00595100, void* self);
inline void* const ms_audio = reinterpret_cast<void*>(0x00CF3DA0);
GAME_FN(void, __cdecl, Fn_738B10, 0x00738B10);
GAME_FN(void, __cdecl, Fn_4096F0, 0x004096F0);
GAME_FN(void, __cdecl, Fn_4094B0, 0x004094B0, int);
GAME_FN(void, __cdecl, Fn_4096C0, 0x004096C0);
GAME_FN(void, __cdecl, Fn_409750, 0x00409750);
GAME_FN(int, __thiscall, NiDX9Renderer_Recreate, 0x00880E60, void* renderer, int width, int height, int, uint32_t bpp,
        int, int, int vsync, int, uint32_t multisample, int, int);
GAME_FN(void, __cdecl, Fn_55F130, 0x0055F130);
GAME_FN(void, __cdecl, Fn_514360, 0x00514360);
GAME_FN(void*, __cdecl, PauseMenu_Fn6C3D50, 0x006C3D50);
GAME_FN(void*, __cdecl, PauseMenu_Get, 0x006CA7C0);
GAME_FN(void, __cdecl, PauseMenu_Create, 0x006CB550);
GAME_FN(void, __cdecl, PauseMenu_Finish, 0x006CB620);
GAME_FN(void, __thiscall, PauseMenu_Fn6CC7C0, 0x006CC7C0, void* self, void* arg);
GAME_FN(void, __thiscall, PauseMenu_Fn6CCB20, 0x006CCB20, void* self, void* arg);

// IDirect3D9 (stdcall)
template <typename R, typename... A>
R D3D(int offset, A... args) {
    void* fn = re::ComVtable(ms_pD3D)[offset / 4];   // (IDirect3D9's: native slots)
    return reinterpret_cast<R(__stdcall*)(void*, re::Arg<A>...)>(fn)(ms_pD3D, args...);
}

int32_t ChangeDisplaySettingsReset() {
    return vc::Import<int32_t>(kChangeDisplaySettingsA, nullptr, 0);
}

} // namespace

namespace DisplaySettings {

// 0x404F60 (stdcall): XACTCreateEngine. The game's CoCreateInstances XACT 3.1's auditioning,
// debug or retail engine (flags 1: auditioning, 2: debug; debug too when
// HKLM\Software\Microsoft\XACT's DebugEngine says so; the retail one when the debug one
// isn't there). Ours is XACT's interface over FAudio (XactShim.cpp) whatever the flags: XACT
// has no 64-bit runtime, and the other two engines were for the authoring tools.
static long __stdcall XactCreateEngine(uint32_t, re::Ptr32<void>* engine) {
    return xact::CreateEngine(engine);
}

// 0x405050: in the front end (either start flag or either title menu flag)
static uint32_t __cdecl IsInFrontEnd_() {
    if (!ms_bUserStart_20C32E9 && !ms_bUserStart_20C32EA && !ms_bInTitleMenu_20C32EB && !ms_bInTitleMenu_20C32F2) {
        return 0;
    }
    return 1;
}

// 0x405080: a DWORD value as a bool
static uint8_t __cdecl RegReadBool_(HKEY_ key, const char* name, uint8_t* out) {
    uint32_t size = 4;
    uint32_t type;
    int32_t value;
    if (vc::Import<long>(kRegQueryValueExA, key, name, nullptr, &type, &value, &size) == 0 && type == 4 && size == 4) {
        *out = value != 0;
        return 1;
    }
    return 0;
}

// 0x4050E0: a DWORD value
static uint8_t __cdecl RegReadInt_(HKEY_ key, const char* name, uint32_t* out) {
    uint32_t size = 4;
    uint32_t type;
    uint32_t value;
    if (vc::Import<long>(kRegQueryValueExA, key, name, nullptr, &type, &value, &size) == 0 && type == 4 && size == 4) {
        *out = value;
        return 1;
    }
    return 0;
}

// 0x405140: the adapter (0 when not stored)
static void __cdecl ReadAdapter_() {
    char path[1024];
    sprintf_(path, "%s", ms_registryNames[0], ms_registryNames[1]);
    HKEY_ key;
    if (vc::Import<long>(kRegOpenKeyA, HKEY_CURRENT_USER_, path, &key) != 0) {
        ms_adapter = 0;
        return;
    }
    uint32_t adapter;
    ms_adapter = RegReadInt(key, ms_registryNames[1], &adapter) ? adapter : 0;
    vc::Import<long>(kRegCloseKey, key);
}

// 0x4051D0: the adapter's mode settings (all six, or nothing). The windowed flag read is
// thrown away: its low byte is cleared before it's stored (the game's)
static uint8_t __cdecl ReadModeSettings_() {
    char path[1024];
    sprintf_(path, "%s\\A%d", ms_registryNames[0], ms_adapter);
    HKEY_ key;
    if (vc::Import<long>(kRegOpenKeyA, HKEY_CURRENT_USER_, path, &key) != 0) {
        return 0;
    }
    // (the bools are written as bytes into uninitialised dwords: 0 here)
    uint32_t width, height, aa, vsync = 0, shaders, windowed = 0;
    if (RegReadInt(key, ms_registryNames[2], &width) && RegReadInt(key, ms_registryNames[3], &height) &&
        RegReadInt(key, ms_registryNames[4], &aa) && RegReadBool(key, ms_registryNames[7], reinterpret_cast<uint8_t*>(&vsync)) &&
        RegReadInt(key, ms_registryNames[6], &shaders) &&
        RegReadBool(key, ms_registryNames[5], reinterpret_cast<uint8_t*>(&windowed))) {
        ms_mode.width = int32_t(width);
        ms_mode.vsync = vsync;
        ms_mode.height = int32_t(height);
        ms_mode.antialiasing = int32_t(aa);
        ms_mode.shaders = shaders;
        ms_mode.windowed = windowed & 0xFFFFFF00u;
        vc::Import<long>(kRegCloseKey, key);
        return 1;
    }
    vc::Import<long>(kRegCloseKey, key);
    return 0;
}

// 0x405330: the adapter and its mode settings saved
static void __cdecl WriteSettings_() {
    HKEY_ key;
    uint32_t value;
    if (vc::Import<long>(kRegCreateKeyExA, HKEY_CURRENT_USER_, ms_registryNames[0], 0, nullptr, 0, 0x20006, nullptr, &key,
                         nullptr) == 0) {
        value = ms_adapter;
        vc::Import<long>(kRegSetValueExA, key, ms_registryNames[1], 0, 4, &value, 4);
        vc::Import<long>(kRegCloseKey, key);
    }
    char path[1024];
    sprintf_(path, "%s\\A%d", ms_registryNames[0], ms_adapter);
    if (vc::Import<long>(kRegCreateKeyExA, HKEY_CURRENT_USER_, path, 0, nullptr, 0, 0x20006, nullptr, &key, nullptr) != 0) {
        return;
    }
    value = uint32_t(ms_mode.width);
    vc::Import<long>(kRegSetValueExA, key, ms_registryNames[2], 0, 4, &value, 4);
    value = uint32_t(ms_mode.height);
    vc::Import<long>(kRegSetValueExA, key, ms_registryNames[3], 0, 4, &value, 4);
    value = uint32_t(ms_mode.antialiasing);
    vc::Import<long>(kRegSetValueExA, key, ms_registryNames[4], 0, 4, &value, 4);
    value = uint8_t(ms_mode.vsync);
    vc::Import<long>(kRegSetValueExA, key, ms_registryNames[7], 0, 4, &value, 4);
    value = ms_mode.shaders;
    vc::Import<long>(kRegSetValueExA, key, ms_registryNames[6], 0, 4, &value, 4);
    value = uint8_t(ms_mode.windowed);
    vc::Import<long>(kRegSetValueExA, key, ms_registryNames[5], 0, 4, &value, 4);
    vc::Import<long>(kRegCloseKey, key);
}

// 0x4054D0: the video memory, once (DirectDraw 7's caps, +0x3C; at most 0x7FFFFFFF)
static void __cdecl ReadVideoMemory_() {
    if (ms_videoMemory != 0) {
        return;
    }
    uint32_t caps[0x17C / 4];
    memset_(caps, 0, 0x17C);
    caps[0] = 0x17C;
    void* dd = nullptr;
    long hr = DirectDrawCreateEx_(nullptr, &dd, reinterpret_cast<const void*>(0x009A1FF4), nullptr);
    if (hr < 0 || !dd) {
        return;
    }
    void* const* vtable = re::ComVtable(dd);   // (DirectDraw's: native slots)
    re::PtrCast<long(__stdcall*)(void*, void*, void*)>(vtable[0x2C / 4])(dd, caps, nullptr);   // GetCaps
    re::PtrCast<unsigned long(__stdcall*)(void*)>(vtable[2])(dd);                             // Release
    ms_videoMemory = caps[0x3C / 4];
    if (ms_videoMemory > 0x7FFFFFFF) {
        ms_videoMemory = 0x7FFFFFFF;
    }
}

// ---- 0x405580 to 0x405F60: patched in place by bullygfx, not hooked ----

// 0x405580: the display mode changed to `mode` (32 bits), for this session
[[maybe_unused]] static uint8_t __cdecl ChangeMode_(const VideoMode* mode) {
    uint8_t devmode[0x9C] = {};
    *reinterpret_cast<uint16_t*>(devmode + 0x24) = 0x9C;       // dmSize
    *reinterpret_cast<uint32_t*>(devmode + 0x28) = 0x1C0000;   // dmFields
    *reinterpret_cast<uint32_t*>(devmode + 0x68) = 0x20;       // dmBitsPerPel
    *reinterpret_cast<uint32_t*>(devmode + 0x6C) = uint32_t(mode->width);
    *reinterpret_cast<uint32_t*>(devmode + 0x70) = uint32_t(mode->height);
    // (the rest of the DEVMODE is uninitialised stack in the game)
    return vc::Import<int32_t>(kChangeDisplaySettingsA, devmode, 4) == 0;
}
RE_UNHOOKED_FN("DisplaySettings::ChangeMode", 0x00405580, &ChangeMode_);

// 0x4055D0: full screen in the current mode, or back to the desktop's
[[maybe_unused]] static uint8_t __cdecl SetFullScreen(int fullScreen) {
    if (re::Flag(fullScreen) && !Windowed()) {
        return ChangeMode(&ms_mode);
    }
    ChangeDisplaySettingsReset();
    return 1;
}
RE_UNHOOKED_FN("DisplaySettings::SetFullScreen", 0x004055D0, &SetFullScreen);

// 0x405600: a mode to change to (at the frame's end)
[[maybe_unused]] static void __cdecl SetPendingMode(const VideoMode* mode) {
    ms_pendingMode = *mode;
}
RE_UNHOOKED_FN("DisplaySettings::SetPendingMode", 0x00405600, &SetPendingMode);

// 0x405640: the shader quality (0..3) set and saved; the shaders reloaded when on
[[maybe_unused]] static void __cdecl SetShaderQuality(uint32_t quality) {
    if (quality >= 4) {
        return;
    }
    ms_mode.shaders = quality;
    WriteSettings();
    if (ms_mode.shaders != 0 && ms_bShadersOn) {
        Fn_517650();
        CallRenderer_C8();
    }
}
RE_UNHOOKED_FN("DisplaySettings::SetShaderQuality", 0x00405640, &SetShaderQuality);

// 0x405670: a mode change pending
[[maybe_unused]] static uint8_t __cdecl IsModeChangePending() {
    return ms_pendingMode.antialiasing != 0;
}
RE_UNHOOKED_FN("DisplaySettings::IsModeChangePending", 0x00405670, &IsModeChangePending);

static void PumpMessages(void* message) {
#if defined(_WIN64)
    // (the game's buffer is x86's MSG, 0x1C bytes; x64's is 0x30: a local one, the game's unused)
    alignas(8) uint8_t local[0x30];
    message = local;
#endif
    while (vc::Import<int>(kPeekMessageA, message, nullptr, 0, 0, 1)) {
        vc::Import<int>(kTranslateMessage, message);
        vc::Import<long>(kDispatchMessageA, message);
    }
}

// 0x405680: the window's messages handled
[[maybe_unused]] static void __cdecl ProcessMessages() {
    PumpMessages(ms_message);
}
RE_UNHOOKED_FN("DisplaySettings::ProcessMessages", 0x00405680, &ProcessMessages);

static void __cdecl SetAppActive(int active);

// 0x4056D0: the messages handled until the application is active again
[[maybe_unused]] static void __cdecl WaitUntilActive() {
    if (re::KeepActive() && !ms_bAppActive) {
        SetAppActive(1);
    }
    do {
        PumpMessages(ms_inactiveMessage);
    } while (!ms_bAppActive);
}
RE_UNHOOKED_FN("DisplaySettings::WaitUntilActive", 0x004056D0, &WaitUntilActive);

// 0x405730, 0x405740, 0x405750, 0x405760
[[maybe_unused]] static int32_t __cdecl GetPixelShader30Slots() {
    return ms_pixelShader30Slots;
}
RE_UNHOOKED_FN("DisplaySettings::GetPixelShader30Slots", 0x00405730, &GetPixelShader30Slots);
[[maybe_unused]] static uint8_t __cdecl IsAppActive() {
    return ms_bAppActive;
}
RE_UNHOOKED_FN("DisplaySettings::IsAppActive", 0x00405740, &IsAppActive);
[[maybe_unused]] static int32_t* __cdecl GetModes() {
    return ms_modes;
}
RE_UNHOOKED_FN("DisplaySettings::GetModes", 0x00405750, &GetModes);
[[maybe_unused]] static int32_t* __cdecl GetMultisampleLevels() {
    return ms_multisampleLevels;
}
RE_UNHOOKED_FN("DisplaySettings::GetMultisampleLevels", 0x00405760, &GetMultisampleLevels);

// 0x405770: the adapter's capabilities: shader model 3 (else not supported), its modes
// (640x480, then each X8R8G8B8 one of 800x600 and up, once), its multisample levels (1,
// then 2..16 those supported for both X8R8G8B8 and D24X8), its vendor
[[maybe_unused]] static void __cdecl EnumerateModes_() {
    ms_numModes = 0;
    ms_numMultisampleLevels = 0;
    ms_bHardwareOk = 1;
    ms_adapterCount = 0;
    Renderer_GetDeviceId();
    ms_adapterCount = D3D<uint32_t>(0x10);   // GetAdapterCount
    uint8_t caps[0x130];
    D3D<long>(0x38, ms_adapter, 1, caps);   // GetDeviceCaps (HAL)
    if (*reinterpret_cast<uint16_t*>(caps + 0xCC) < 0x300) {   // PixelShaderVersion
        ms_bHardwareOk = 0;
    }
    if (*reinterpret_cast<uint16_t*>(caps + 0xC4) < 0x300) {   // VertexShaderVersion
        ms_bHardwareOk = 0;
    }
    ms_pixelShader30Slots = *reinterpret_cast<int32_t*>(caps + 0x12C);
    uint32_t count = D3D<uint32_t>(0x18, ms_adapter, 0x16);   // GetAdapterModeCount
    if (count == 0) {
        ms_bHardwareOk = 0;
    } else {
        ms_modes[ms_numModes * 2] = 640;
        ms_modes[ms_numModes * 2 + 1] = 480;
        ms_numModes++;
        for (uint32_t i = 0; i < count; i++) {
            int32_t mode[4];
            if (D3D<long>(0x1C, ms_adapter, 0x16, i, mode) != 0) {   // EnumAdapterModes
                continue;
            }
            if (mode[0] < 800 || mode[1] < 600) {
                continue;
            }
            bool listed = false;
            for (int j = 0; j < ms_numModes; j++) {
                if (ms_modes[j * 2] == mode[0] && ms_modes[j * 2 + 1] == mode[1]) {
                    listed = true;
                    break;
                }
            }
            if (!listed) {
                ms_modes[ms_numModes * 2] = mode[0];
                ms_modes[ms_numModes * 2 + 1] = mode[1];
                ms_numModes++;
            }
        }
    }
    if (ms_numModes == 0) {
        ms_bHardwareOk = 0;
    }
    ms_multisampleLevels[ms_numMultisampleLevels++] = 1;
    // CheckDeviceMultiSampleType(adapter, HAL, format, windowed 0, type, null)
    long colour = D3D<long>(0x2C, ms_adapter, 1, 0x16, 0, 1, nullptr);
    long depth = D3D<long>(0x2C, ms_adapter, 1, 0x4D, 0, 1, nullptr);
    if (colour >= 0 && depth >= 0) {
        for (int level = 2; level <= 16; level++) {
            colour = D3D<long>(0x2C, ms_adapter, 1, 0x16, 0, level, nullptr);
            depth = D3D<long>(0x2C, ms_adapter, 1, 0x4D, 0, level, nullptr);
            if (colour >= 0 && depth >= 0) {
                ms_multisampleLevels[ms_numMultisampleLevels++] = level;
            }
        }
    }
    uint8_t identifier[0x448];
    D3D<long>(0x14, ms_adapter, 0, identifier);   // GetAdapterIdentifier
    uint32_t vendor = *reinterpret_cast<uint32_t*>(identifier + 0x428);
    ms_vendor = vendor == 0x1002 ? 2 : (vendor == 0x10DE ? 1 : 0);
}
RE_UNHOOKED_FN("DisplaySettings::EnumerateModes", 0x00405770, &EnumerateModes_);

// 0x405A00: the defaults (800x600, full screen, no multisampling, vsync or shaders),
// when the hardware is supported. (The monitor's info is asked for and not used)
[[maybe_unused]] static void __cdecl DefaultSettings_() {
    if (!ms_bHardwareOk) {
        return;
    }
    uint32_t info[10];
    info[0] = 0x28;
    void* monitor = D3D<void*>(0x3C, ms_adapter);   // GetAdapterMonitor
    vc::Import<int>(kGetMonitorInfoA, monitor, info);
    reinterpret_cast<uint8_t&>(ms_mode.windowed) = 0;
    ms_mode.width = 800;
    ms_mode.height = 600;
    ms_mode.antialiasing = 1;
    reinterpret_cast<uint8_t&>(ms_mode.vsync) = 0;
    ms_mode.shaders = 0;
}
RE_UNHOOKED_FN("DisplaySettings::DefaultSettings", 0x00405A00, &DefaultSettings_);

// 0x405A70: the mode's size among the adapter's
[[maybe_unused]] static uint8_t __cdecl IsModeListed_(const VideoMode* mode) {
    for (int i = 0; i < ms_numModes; i++) {
        if (ms_modes[i * 2] == mode->width && ms_modes[i * 2 + 1] == mode->height) {
            return 1;
        }
    }
    return 0;
}
RE_UNHOOKED_FN("DisplaySettings::IsModeListed", 0x00405A70, &IsModeListed_);

// 0x405AB0: the settings usable: the size listed (or windowed), the multisample level
// supported, the shader quality at most 3 (2 with under 1000 shader slots), and the aspect
// ratio between 1.25 and 1.8
[[maybe_unused]] static uint8_t __cdecl AreSettingsValid_() {
    if (!Windowed() && !IsModeListed(&ms_mode)) {
        return 0;
    }
    for (int i = 0; i < ms_numMultisampleLevels; i++) {
        if (ms_multisampleLevels[i] != ms_mode.antialiasing) {
            continue;
        }
        int32_t maxQuality = ms_pixelShader30Slots < 1000 ? 2 : 3;
        int32_t quality = int32_t(ms_mode.shaders);
        if (quality < 0 || quality > maxQuality) {
            return 0;
        }
        uint32_t ratio = uint32_t(ms_mode.width * 1000 / ms_mode.height - 1250);
        return ratio <= (re::UltraWide() ? 3560u - 1250u : 0x226u);   // (32:9 at most, runflags.h)
    }
    return 0;
}
RE_UNHOOKED_FN("DisplaySettings::AreSettingsValid", 0x00405AB0, &AreSettingsValid_);

// 0x405B40: waits (up to `tries` half seconds) for the device to be usable again; the
// renderer's 0x8806C0 each time
[[maybe_unused]] static uint8_t __cdecl WaitForDevice_(uint32_t tries) {
    uint8_t* renderer = ms_pRenderer;
    if (!renderer) {
        return 1;
    }
    while (true) {
        if (!renderer[0x4F8]) {
            void* device = *reinterpret_cast<re::Ptr32<void>*>(renderer + 0xA4);
            // IDirect3DDevice9::TestCooperativeLevel
            if (reinterpret_cast<long(__stdcall*)(void*)>(static_cast<void*>(re::ComVtable(device)[3]))(device) == 0) {
                break;
            }
        }
        if (tries == 0) {
            break;
        }
        Renderer_Fn8806C0(renderer);
        vc::Import<void>(kSleep, 500);
        tries--;
    }
    return tries != 0;
}
RE_UNHOOKED_FN("DisplaySettings::WaitForDevice", 0x00405B40, &WaitForDevice_);

// 0x405BA0: the settings (read, checked and defaulted the first time); returns `out`
[[maybe_unused]] static VideoMode* __cdecl GetDisplaySettings_(VideoMode* out) {
    if (ms_bFirstSettingsRead) {
        ms_bFirstSettingsRead = 0;
        ReadAdapter();
        ReadVideoMemory();
        EnumerateModes();
        if (!ReadModeSettings() || !AreSettingsValid()) {
            DefaultSettings();
            WriteSettings();
        }
        ms_bShadersOn = ms_mode.shaders != 0;
        ms_shaderQuality = ms_mode.shaders;
    }
    *out = ms_mode;
    return out;
}
RE_UNHOOKED_FN("DisplaySettings::GetDisplaySettings", 0x00405BA0, &GetDisplaySettings_);

// 0x405C30: the video memory available at start (the device's, less what DirectDraw
// reported, or all of it), once
[[maybe_unused]] static void __cdecl ReadAvailableMemory() {
    if (ms_availableMemory != 0) {
        return;
    }
    void* device = *reinterpret_cast<re::Ptr32<void>*>(ms_pRenderer + 0xA4);
    // IDirect3DDevice9::GetAvailableTextureMem
    uint32_t available = reinterpret_cast<uint32_t(__stdcall*)(void*)>(static_cast<void*>(re::ComVtable(device)[4]))(device);
    int64_t rest = int64_t(available) - int64_t(ms_videoMemory);
    ms_availableMemory = rest > 0 ? uint64_t(rest) : available;
}
RE_UNHOOKED_FN("DisplaySettings::ReadAvailableMemory", 0x00405C30, &ReadAvailableMemory);

// 0x405C90: on ATI cards, the shaders' gFracBias set to 1/128
[[maybe_unused]] static void __cdecl SetAtiFracBias() {
    if (ms_vendor != 2 || !ShaderLibrary_Get()) {
        return;
    }
    float bias = 0.0078125f;
    re::Ptr32<char> name = StringTable_AddString("gFracBias");
    ShaderConstant_Set(&name, 4, &bias);
    if (name != ms_nullString) {
        uint16_t length = *reinterpret_cast<uint16_t*>(name - 2);
        if (vc::Decrement(name - 8) == 1) {
            StringTable_Release(ms_pStringTable, &name, length);
        }
    }
}
RE_UNHOOKED_FN("DisplaySettings::SetAtiFracBias", 0x00405C90, &SetAtiFracBias);

// 0x405D10: the shader binaries' directories for the quality (2 entries; 0 when the
// buffer's too small)
[[maybe_unused]] static int __cdecl GetShaderDirectories(re::Ptr32<const char>* out, int size) {
    VideoMode mode;
    GetDisplaySettings(&mode);
    if (size < 4) {
        return 0;
    }
    out[1] = "ShaderBinaries\\Shaders";
    switch (mode.shaders) {
    case 0:
        out[0] = "ShaderBinaries\\Off";
        break;
    case 1:
        out[0] = "ShaderBinaries\\Low";
        break;
    case 2:
        out[0] = "ShaderBinaries\\Med";
        break;
    case 3:
        out[0] = ms_pixelShader30Slots > 1000 ? "ShaderBinaries\\High" : "ShaderBinaries\\Med";
        break;
    default:
        out[0] = nullptr;
        break;
    }
    return 2;
}
RE_UNHOOKED_FN("DisplaySettings::GetShaderDirectories", 0x00405D10, &GetShaderDirectories);

// 0x405DB0: the application activated or deactivated: the display mode, the window,
// the device, the audio, the game mutex
[[maybe_unused]] static void __cdecl SetAppActive(int active) {
    uint8_t on = uint8_t(active) || re::KeepActive();
    if (on && !Windowed()) {
        ChangeMode(&ms_mode);
    } else {
        ChangeDisplaySettingsReset();
    }
    if (ms_bAppActive == on) {
        return;
    }
    ms_bAppActive = on;
    if (on) {
        ms_bPausedByFocus = 0;
        if (ms_bOwnsMutex) {
            vc::Import<int>(kReleaseMutex, ms_mutex);
            ms_bOwnsMutex = 0;
        }
        ms_bPaused_C1A998 = 0;
        ms_bFlag_CF68E8 = 1;
        vc::Import<int>(kShowWindow, ms_window, 9);   // SW_RESTORE
        vc::Import<int>(kMoveWindow, ms_window, 0, 0, ms_mode.width, ms_mode.height, 0);
        if (!WaitForDevice(100)) {
            ShowSystemMessage("SYS_DEVICE_LOST");
            exit_(-1);
        }
        if (ms_pRenderer && ms_bGameStarted && (!IsInFrontEnd() || ms_gameState_BA6894 != 3)) {
            ms_pauseCountdown = 2;
        }
        if (ms_bAudioInitialised && ms_bAudioPaused) {
            Audio_Resume(ms_audio);
            ms_bAudioPaused = 0;
        }
        vc::Import<int>(kQueryPerformanceCounter, &ms_lastFrameTime);
        return;
    }
    if (vc::Import<uint32_t>(kWaitForSingleObject, ms_mutex, 0xFFFFFFFFu) == 0) {
        ms_bOwnsMutex = 1;
    }
    ms_bPaused_C1A998 = 1;
    ms_audio_CF68E4 = ms_audio_CF68DC;
    if (ms_bAudioInitialised && !ms_bAudio_CF3DD5) {
        Audio_Pause(ms_audio);
        ms_bAudioPaused = 1;
    }
    Fn_738B10();
    if (re::ForceWindowed()) {
        return;   // (kept on the desktop)
    }
    vc::Import<int>(kShowWindow, ms_window, 6);   // SW_MINIMIZE
}
RE_UNHOOKED_FN("DisplaySettings::SetAppActive", 0x00405DB0, &SetAppActive);

// 0x405F60: ApplyVideoMode: the window resized, the renderer recreated in the current
// mode, the settings saved; the screen scales for 4:3 (0.75) or wider (1)
[[maybe_unused]] static uint8_t __cdecl ApplyVideoMode() {
    Fn_738B10();
    int32_t width = ms_mode.width;
    int32_t height = ms_mode.height;
    uint8_t* window = *reinterpret_cast<re::Ptr32<uint8_t>*>(ms_pApp + 0x2C);
    uint32_t bpp = *reinterpret_cast<uint32_t*>(window + 4);
    uint32_t multisample = ms_mode.antialiasing <= 1 ? 0 : (uint32_t(ms_mode.antialiasing) | 0x80000000u);
    if (!vc::Import<int>(kSetWindowPos, ms_window, nullptr, 0, 0, width, height, 0)) {
        return 0;
    }
    Fn_4096F0();
    Fn_4094B0(7);
    Fn_4096C0();
    Fn_409750();
    uint8_t* renderer = ms_pRenderer;
    if (*reinterpret_cast<int32_t*>(renderer + 0x94) == 3) {
        uint8_t ok = vc::Call<uint8_t>(renderer, 0x150);
        *reinterpret_cast<int32_t*>(renderer + 0x94) = 0;
        if (ok) {
            (*reinterpret_cast<int32_t*>(renderer + 0x9C))++;
        }
    }
    int result = NiDX9Renderer_Recreate(ms_pRenderer, width, height, 0, bpp, 2, 0, uint8_t(ms_mode.vsync) != 0, 0,
                                        multisample, 1, 0);
    if (result == 0) {
        if (!WaitForDevice(10)) {
            return 0;
        }
    } else if (uint32_t(result - 1) > 1) {
        return 0;
    }
    WriteSettings();
    vc::Call<void>(ms_pApp, 0xF4);
    if (ms_bGameStarted) {
        ms_pauseCountdown = 1;
    }
    ms_bWidescreen = 1;
    VideoMode mode;
    GetDisplaySettings(&mode);
    float scale = 1.0f;
    if (mode.height == 0 || double(mode.width) / double(mode.height) >= 1.5) {
        if (ms_bWidescreen) {
            scale = 0.75f;
        }
    } else {
        ms_bWidescreen = 0;
    }
    scale = float(scale / re::WiderThan169());
    ms_screenScales[0] = scale;
    ms_screenScales[1] = 1.0f;
    ms_screenScales[2] = float(1.0 / scale);
    ms_screenScales[3] = 1.0f;
    ms_screenScales[4] = scale;
    ms_screenScales[5] = 1.0f;
    ms_screenScales[6] = float(1.0 / scale);
    ms_screenScales[7] = 1.0f;
    Fn_55F130();
    Fn_514360();
    return 1;
}
RE_UNHOOKED_FN("DisplaySettings::ApplyVideoMode", 0x00405F60, &ApplyVideoMode);

// ---- hookable again ----

// 0x406140: the frame's end: the frame limiter (fpsLimit, or 31, frames a second), the
// start of the game noted, the pause menu brought up a couple of frames after losing and
// regaining focus, and a pending mode change applied (or undone when it fails)
static void __cdecl EndFrame() {
    if (ms_performanceFrequency == 0) {
        vc::Import<int>(kQueryPerformanceFrequency, &ms_performanceFrequency);
    }
    int64_t now;
    vc::Import<int>(kQueryPerformanceCounter, &now);
    int64_t fps = ms_fpsLimit > 0 ? ms_fpsLimit : 31;
    int64_t next = alldiv(ms_performanceFrequency, fps) + ms_lastFrameTime;
    while (now < next) {
        vc::Import<void>(kSleep, 1);
        vc::Import<int>(kQueryPerformanceCounter, &now);
    }
    float fade = ms_fade;
    uint8_t e9 = ms_bUserStart_20C32E9;
    uint8_t ea = ms_bUserStart_20C32EA;
    uint8_t eb = ms_bInTitleMenu_20C32EB;
    ms_lastFrameTime = now;
    if (!ms_bGameStarted && 255.0 > fade && !e9 && !ea && !eb && !ms_bInTitleMenu_20C32F2 && ms_bLoaded_BF3818) {
        ms_bGameStarted = 1;
    }
    if (ms_pauseCountdown != 0 && !e9 && !ea && !eb && !ms_bInTitleMenu_20C32F2 && --ms_pauseCountdown == 0) {
        // (NaN counts as faded in)
        if (255.0 > fade || fade != fade) {
            void* player = ms_pPlayerPed_C1AEA8;
            void* current = PauseMenu_Fn6C3D50();
            if (current) {
                if (!ms_bPausedByFocus) {
                    void* menu = PauseMenu_Get();
                    if (menu) {
                        PauseMenu_Fn6CC7C0(menu, current);
                    } else {
                        PauseMenu_Create();
                        PauseMenu_Fn6CC7C0(PauseMenu_Get(), current);
                        PauseMenu_Finish();
                    }
                    ms_bPausedByFocus = 1;
                }
                ms_pauseCountdown = 1;
            } else if (player) {
                void* menu = PauseMenu_Get();
                if (menu) {
                    PauseMenu_Fn6CCB20(menu, player);
                } else {
                    PauseMenu_Create();
                    PauseMenu_Fn6CCB20(PauseMenu_Get(), player);
                    PauseMenu_Finish();
                }
                if (ms_pauseCountdown == 0) {
                    ms_bPausedByFocus = 0;
                }
            } else {
                ms_pauseCountdown = 1;
            }
        } else {
            ms_pauseCountdown = 1;
        }
    }
    if (ms_pendingMode.antialiasing == 0) {
        return;
    }
    VideoMode saved = ms_mode;
    ms_mode = ms_pendingMode;
    if (ApplyVideoMode_()) {
        WriteSettings();
        if (!Windowed()) {
            ChangeMode(&ms_mode);
        } else {
            ChangeDisplaySettingsReset();
        }
    } else {
        ms_mode = saved;
    }
    ms_pendingMode.height = 0;
    ms_pendingMode.antialiasing = 0;
    ms_pendingMode.width = 0;
    ms_pendingMode.vsync = 0;   // (the game's upper three bytes are stack garbage)
    ms_pendingMode.shaders = 0;
    ms_pendingMode.windowed = saved.windowed & 0xFFFFFF00u;
}

} // namespace DisplaySettings

float re::WiderThan169() {
    if (!re::UltraWide() || !ms_bWidescreen || ms_mode.height <= 0) {
        return 1.0f;
    }
    double wider = double(ms_mode.width) / double(ms_mode.height) / (16.0 / 9.0);
    return wider > 1.0 ? float(wider) : 1.0f;
}

// ---- registration ----------------------------------------------------------------------

#define C00_DISPLAY_REVERSED(name, fn, address)                                                     \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace DisplaySettings {
C00_DISPLAY_REVERSED("XACTCreateEngine", XactCreateEngine, 0x00404F60);
C00_DISPLAY_REVERSED("DisplaySettings::IsInFrontEnd", IsInFrontEnd_, 0x00405050);
C00_DISPLAY_REVERSED("DisplaySettings::RegReadBool", RegReadBool_, 0x00405080);
C00_DISPLAY_REVERSED("DisplaySettings::RegReadInt", RegReadInt_, 0x004050E0);
C00_DISPLAY_REVERSED("DisplaySettings::ReadAdapter", ReadAdapter_, 0x00405140);
C00_DISPLAY_REVERSED("DisplaySettings::ReadModeSettings", ReadModeSettings_, 0x004051D0);
C00_DISPLAY_REVERSED("DisplaySettings::WriteSettings", WriteSettings_, 0x00405330);
C00_DISPLAY_REVERSED("DisplaySettings::ReadVideoMemory", ReadVideoMemory_, 0x004054D0);
C00_DISPLAY_REVERSED("DisplaySettings::EndFrame", EndFrame, 0x00406140);
} // namespace DisplaySettings
