// More of the application's top level (see App.cpp): the Gamebryo application's creation
// (the window class, the renderer), the event handler, the frame's camera set-up, the
// loading icon, the credits lines' drawing, the accessibility shortcut keys (the DirectX
// samples' AllowShortcutKeys), the error box.

#include <cmath>
#include <cstdint>

#include <windows.h>

#include "reversed.h"
#include "runflags.h"

#include "game/Crt.h"
namespace {

// Win32 through the game's import slots (read at call time)
template <typename F>
F Import(uintptr_t slot) {
    // (the slot is the game's 4 bytes: read as such on x64)
    return reinterpret_cast<F>(static_cast<void*>(*reinterpret_cast<re::Ptr32<void>*>(slot)));
}
constexpr uintptr_t IMP_GetStockObject = 0x008FC034;
constexpr uintptr_t IMP_QueryPerformanceCounter = 0x008FC040;
constexpr uintptr_t IMP_MultiByteToWideChar = 0x008FC074;
constexpr uintptr_t IMP_SystemParametersInfoA = 0x008FC230;
constexpr uintptr_t IMP_MessageBoxW = 0x008FC234;
constexpr uintptr_t IMP_LoadIconA = 0x008FC23C;
constexpr uintptr_t IMP_LoadCursorA = 0x008FC258;
constexpr uintptr_t IMP_RegisterClassA = 0x008FC25C;

using re::At;
// a virtual function of a thiscall object, by its byte offset in the vtable
template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

// FEDisplaySettings' (0x405BA0 fills it)
struct DisplaySettings {
    int32_t width;         // +0
    int32_t height;        // +4
    uint8_t unknown8[0xC]; // +8
    uint8_t windowed;      // +0x14
    uint8_t unknown15[3];  // +0x15
};
static_assert(sizeof(DisplaySettings) == 0x18);

// a credits line (0x18), at 0xBF3860 (see App.cpp)
struct CreditLine {
    re::Ptr32<const uint16_t> text;   // +0
    float x;                // +4
    float y;                // +8
    uint8_t colour[4];      // +0xC
    float scale;            // +0x10
    int32_t justify;        // +0x14
};
static_assert(sizeof(CreditLine) == 0x18);
GAME_ARRAY(CreditLine, ms_creditLines, 0x00BF3860);
GAME_VAR(int32_t, ms_numCreditLines, 0x00BF381C);

GAME_VAR(uint8_t, ms_bWidescreen, 0x00C1870E);            // (16:9 unless the display is narrower than 1.5)
GAME_VAR(void*, ms_camera, 0x00C1AE7C);
GAME_VAR(void*, ms_pRenderer, 0x020E2380);                 // NiDX9Renderer
GAME_VAR(int32_t*, ms_pApplication, 0x00BF3838);           // the NiApplication
GAME_VAR(int32_t, ms_appCmdLine_BF383C, 0x00BF383C);
GAME_VAR(int32_t, ms_cmdLine_20E2168, 0x020E2168);
GAME_VAR(HINSTANCE, ms_hInstance, 0x00BD77B4);
GAME_VAR(int32_t, ms_windowArg_BD77B0, 0x00BD77B0);
GAME_VAR(HWND, ms_hWnd, 0x00BD77FC);                        // RsGlobal's window
GAME_VAR(int32_t, ms_unknown20E5878, 0x020E5878);
GAME_VAR(uint8_t, ms_bShaderModelOk, 0x009D3BA4);
inline const char* const ms_windowClassName = reinterpret_cast<const char*>(0x00BC8D70);   // "Gamebryo Application"
inline void* const ms_wndProc = reinterpret_cast<void*>(0x0082A010);
inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline uint8_t* const TheCameraMatrix = reinterpret_cast<uint8_t*>(0x00C3CCB0);
GAME_VAR(void*, ms_cameraNode, 0x00C3CCF0);
GAME_VAR(int32_t, ms_fovOverrideOn, 0x00C3CDD4);
GAME_VAR(float, ms_fovOverride, 0x00C3CDD0);
GAME_VAR(float, ms_fov, 0x00C3CD04);
GAME_VAR(float, ms_fadeAlpha, 0x00C3CD28);
GAME_ARRAY(uint8_t, ms_fadeColour, 0x00C3CD2C);             // r, g, b, a
GAME_VAR(int32_t, ms_widescreenSetting, 0x00C3CD0C);
GAME_VAR(int32_t, ms_widescreenSetting2, 0x00C3CD10);
GAME_VAR(uint32_t, ms_splashStartTime, 0x00BF3840);         // (ms of the performance counter)
GAME_VAR(float, ms_screenScaleX, 0x00A957A8);
GAME_VAR(float, ms_screenScaleY, 0x00A957AC);
GAME_VAR(float, ms_timeStep, 0x00C1A9A4);
GAME_ARRAY(float, ms_clipRect, 0x020DCD04);                 // left, top, right, bottom
GAME_VAR(float, ms_loadingIconTime, 0x00A13654);
GAME_VAR(float, ms_loadingIconSpeed, 0x00A13638);
GAME_VAR(void*, ms_loadingIconTexture, 0x00C66FF0);
GAME_VAR(uint8_t, ms_bInputIgnoredDisplay, 0x00BF381A);
GAME_VAR(uint8_t, ms_bUserStart, 0x020C32E9);
GAME_VAR(uint8_t, ms_bFlag_20C32EA, 0x020C32EA);
GAME_VAR(uint8_t, ms_bFlag_20C32EB, 0x020C32EB);
GAME_VAR(uint8_t, ms_bFlag_20C32F2, 0x020C32F2);
GAME_VAR(uint8_t, ms_bClearToWhite_CE754D, 0x00CE754D);
GAME_ARRAY(uint8_t, ms_skyColour, 0x020C47F8);              // r, g, b
GAME_ARRAY(uint8_t, ms_clearColour, 0x00BF3E00);            // r, g, b
GAME_ARRAY(uint8_t, ms_fontColour, 0x0208B764);             // r, g, b, a
GAME_VAR(uint8_t, ms_bPlayerHas140C, 0x00C67561);
GAME_VAR(uint8_t*, ms_pPlayerPed, 0x00C1AEA8);              // CWorld::Players[0].ped
inline void* const ms_controllerMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const obj_A45A30 = reinterpret_cast<void*>(0x00A45A30);
GAME_VAR(void*, ms_pObj_9E4F4C, 0x009E4F4C);
// the accessibility settings at start-up (restored when leaving)
GAME_VAR(STICKYKEYS, ms_startupStickyKeys, 0x00A135E4);
GAME_VAR(TOGGLEKEYS, ms_startupToggleKeys, 0x00A135EC);
GAME_VAR(FILTERKEYS, ms_startupFilterKeys, 0x00A135F4);

GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(const char*, __thiscall, Text_GetByHash, 0x0068FF10, void* text, uint32_t hash);
GAME_FN(const char*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int64_t, __cdecl, GetPerformanceFrequency, 0x0045B7F0);
GAME_FN(void, __thiscall, Camera_Fade, 0x004F1AD0, void* camera, float seconds, int direction, int);
GAME_FN(int, __thiscall, Camera_GetFadeState, 0x004F12F0, void* camera);
GAME_FN(float, __thiscall, Camera_GetAspectRatio, 0x004F1360, void* camera);
GAME_FN(void, __thiscall, Camera_Fn_4F1BE0, 0x004F1BE0, void* camera);
GAME_FN(void, __thiscall, Matrix_FromNode, 0x00412A70, void* matrix, void* node);
GAME_FN(void, __cdecl, Camera_SetView, 0x005EF8A0, void* camera, uintptr_t, float tanHalfFov, float aspect);
GAME_FN(void, __cdecl, Camera_Fn_5F2EB0, 0x005F2EB0, void* camera);
GAME_FN(void, __cdecl, Camera_SetClearColour, 0x006CA7D0, void* camera, const uint8_t* colour, int);
GAME_FN(int, __cdecl, Camera_BeginUpdate, 0x006C86A0, void* camera);
GAME_FN(void, __cdecl, Fn_4098D0, 0x004098D0);
GAME_FN(void, __cdecl, Fn_4096F0, 0x004096F0);
GAME_FN(uint8_t, __cdecl, Fn_538C70, 0x00538C70);
GAME_FN(uint8_t, __thiscall, Fn_50DF50, 0x0050DF50, void* self);
GAME_FN(uint8_t, __thiscall, Fn_415380, 0x00415380, void* self);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int pad, int);
GAME_FN(void, __cdecl, GetDisplaySettings, 0x00405BA0, DisplaySettings* settings);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(float, __cdecl, Clamp, 0x0041A470, float value, float low, float high);
GAME_FN(void, __cdecl, DrawSprite, 0x0052D610, const float* rect, void* texture, const uint8_t* colour, const float* uv0, const float* uv1);
GAME_FN(void, __cdecl, DrawQuad, 0x0052C270, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, const uint8_t* colour);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(void*, __thiscall, Controllers_Get0, 0x006AA680, void* mgr);
GAME_FN(void*, __thiscall, Controllers_Get1, 0x006AA690, void* mgr);
GAME_FN(void, __thiscall, Controller_Reset, 0x006ABF20, void* controller);
GAME_FN(void, __cdecl, Fn_514EF0, 0x00514EF0);
GAME_FN(void, __cdecl, Fn_4582B0, 0x004582B0);
GAME_FN(void, __cdecl, Fn_411670, 0x00411670);
GAME_FN(void, __cdecl, Fn_5EE630, 0x005EE630);
GAME_FN(void, __thiscall, Fn_753840, 0x00753840, void* self, void* p);
GAME_FN(void, __cdecl, Fn_7536F0, 0x007536F0, void* p);
GAME_FN(void, __cdecl, Fn_828B00, 0x00828B00, uintptr_t p);
GAME_FN(void, __cdecl, EmptyFunction, 0x0049A650);   // (just a ret)
GAME_FN(void, __cdecl, Fn_411F60, 0x00411F60);
GAME_FN(void, __cdecl, Fn_738090, 0x00738090);
GAME_FN(void, __thiscall, Fn_4546B0, 0x004546B0, void* self);
GAME_FN(void, __cdecl, Fn_7351C0, 0x007351C0);
GAME_FN(void, __cdecl, Fn_7379A0, 0x007379A0);
GAME_FN(void, __thiscall, Fn_753850, 0x00753850, void* self);
GAME_FN(int32_t*, __cdecl, CreateApplication, 0x00404EB0);
GAME_FN(void, __cdecl, Fn_753750, 0x00753750);
GAME_FN(void, __cdecl, Fn_829EE0, 0x00829EE0);
GAME_FN(void, __cdecl, Fn_405C30, 0x00405C30);
GAME_FN(void, __cdecl, Fn_410C00, 0x00410C00, int, uintptr_t);
GAME_FN(void, __cdecl, Fn_617C30, 0x00617C30);
GAME_FN(void, __cdecl, Fn_5F7080, 0x005F7080);
GAME_FN(void, __cdecl, Fn_72BE80, 0x0072BE80);
GAME_FN(void, __cdecl, Fn_55EF80, 0x0055EF80);
GAME_FN(uint8_t, __cdecl, Game_Initialise_42E970, 0x0042E970);
inline void __cdecl Exit(int code) { crt::Exit(code); }
GAME_FN(int, __cdecl, App_CanRender, 0x0043AD10);
GAME_FN(void, __cdecl, Fn_42ED70, 0x0042ED70);
GAME_FN(void, __cdecl, Fn_42E450, 0x0042E450);
GAME_FN(void, __cdecl, Fn_42E4C0, 0x0042E4C0);
// (ours, called where modes apply)
GAME_FN(void, __cdecl, App_UpdateInputIgnored, 0x0043A8E0);
GAME_FN(void, __cdecl, App_ShowError, 0x0043B060, uint32_t hash);
GAME_FN(int, __cdecl, App_Create, 0x0043B6D0);

// the camera's tan(fov / 2): 0x85C600 is the CRT's x87 tan (its argument in st0)
float TanHalfFov() {
    float fov = ms_fovOverrideOn == 0 ? ms_fov : ms_fovOverride;
    float half = float(double(fov) * 0.008726646502812704);   // (pi / 360)
    return float(std::tan(double(half)));
}

// (0 when negative; NaN stays)
float NotNegative(float value) {
    return value < 0.0f ? 0.0f : value;
}

} // namespace

namespace App2 {

// 0x43A6D0: the sprites' clip rectangle (half a pixel in from each edge, none negative)
static void __cdecl SetClipRect(float left, float top, float right, float bottom) {
    float l = NotNegative(float(double(left) + 0.5));
    float t = NotNegative(float(double(top) + 0.5));
    float r = NotNegative(float(double(right) - 0.5));
    float b = NotNegative(float(double(bottom) - 0.5));
    ms_clipRect[0] = l;
    ms_clipRect[1] = t;
    ms_clipRect[2] = r;
    ms_clipRect[3] = b;
}

// 0x43A8E0: while the input-ignored display is up (not in widescreen, no fade), any of
// the pads 0x10-0x13 moved restarts the loading icon
static void __cdecl UpdateInputIgnored() {
    bool show = false;
    if (!Fn_50DF50(obj_A45A30) && !Fn_415380(ms_pObj_9E4F4C) && ms_widescreenSetting == 0 &&
        Camera_GetFadeState(TheCamera) != 1 && Camera_GetFadeState(TheCamera) != 1) {
        show = true;
    }
    if (!ms_bInputIgnoredDisplay || !show) {
        return;
    }
    bool moved = false;
    for (int pad = 0; pad < 0x14; pad++) {
        if (pad == 0x10 || pad == 0x12 || pad == 0x11 || pad == 0x13) {
            if (!moved) {
                moved = Pad_GetValue(pad, 0) != 0.0f;   // (NaN: moved)
            }
        }
    }
    if (moved) {
        ms_loadingIconTime = 0.0f;
    }
}

// 0x43A990: waits for the word at 0xBF382C to change (another thread's)
static void __cdecl WaitForChange() {
    volatile uint32_t* value = reinterpret_cast<volatile uint32_t*>(0x00BF382C);
    uint32_t start = *value;
    while (*value == start) {
    }
}

// 0x43A9A0: the Windows accessibility shortcut keys (sticky, toggle, filter keys) back to
// how they were, or their hotkeys off while playing (unless the feature itself is on)
static void __cdecl AllowShortcutKeys(int allow) {
    auto spi = Import<BOOL(WINAPI*)(UINT, UINT, PVOID, UINT)>(IMP_SystemParametersInfoA);
    if (re::Flag(allow)) {
        spi(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &ms_startupStickyKeys, 0);
        spi(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &ms_startupToggleKeys, 0);
        spi(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &ms_startupFilterKeys, 0);
        return;
    }
    STICKYKEYS sticky = ms_startupStickyKeys;
    if (!(sticky.dwFlags & SKF_STICKYKEYSON)) {
        sticky.dwFlags &= ~(SKF_HOTKEYACTIVE | SKF_CONFIRMHOTKEY);
        spi(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &sticky, 0);
    }
    TOGGLEKEYS toggle = ms_startupToggleKeys;
    if (!(toggle.dwFlags & TKF_TOGGLEKEYSON)) {
        toggle.dwFlags &= ~(TKF_HOTKEYACTIVE | TKF_CONFIRMHOTKEY);
        spi(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &toggle, 0);
    }
    FILTERKEYS filter = ms_startupFilterKeys;
    if (!(filter.dwFlags & FKF_FILTERKEYSON)) {
        filter.dwFlags &= ~(FKF_HOTKEYACTIVE | FKF_CONFIRMHOTKEY);
        spi(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &filter, 0);
    }
}

// 0x43AA90: the display mode changed: 16:9 unless the display is narrower than 1.5, and
// everything that depends on the screen reset
static void __cdecl OnDisplayChange(int p) {
    uint8_t object[8];
    DisplaySettings settings;
    Fn_411670();
    ms_bWidescreen = 1;
    GetDisplaySettings(&settings);
    if (settings.height != 0 && double(settings.width) / double(settings.height) < 1.5) {
        ms_bWidescreen = 0;
    }
    Fn_5EE630();
    Fn_753840(object, reinterpret_cast<void*>(0x00A13650));
    Fn_7536F0(object);
    Fn_828B00(p);
    EmptyFunction();
    EmptyFunction();
    Fn_411F60();
    Fn_738090();
    Fn_4546B0(reinterpret_cast<void*>(0x00C18700));
    Fn_7351C0();
    Fn_7379A0();
    Fn_753850(object);
}

// 0x43AB30: the frame's camera, cleared to black (4:3 when asked: the eighth argument;
// the callers push 13 words)
static uint8_t __cdecl BeginCameraUpdate(int, int, int, int, int, int, int, int fourByThree, int, int, int, int, int) {
    float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    VCall<void>(ms_pRenderer, 0x58, black);   // the background colour
    Fn_4098D0();
    Fn_4096F0();
    float tanHalf = TanHalfFov();
    float aspect = re::Flag(fourByThree) ? 1.3333334f : Camera_GetAspectRatio(TheCamera);
    Camera_SetView(ms_camera, 0, tanHalf, aspect);
    Camera_Fn_5F2EB0(ms_camera);
    return Camera_BeginUpdate(ms_camera) != 0;
}

// 0x43AC10: once faded out, the splash's start time and a fade in from black
static uint8_t __cdecl StartSplashFade() {
    ms_splashStartTime = 0;
    if (!(ms_fadeAlpha < 0.0001f)) {
        return 0;
    }
    ms_splashStartTime = 0;
    LARGE_INTEGER counter;
    if (Import<BOOL(WINAPI*)(LARGE_INTEGER*)>(IMP_QueryPerformanceCounter)(&counter)) {
        double ticks = double(counter.QuadPart);
        ms_splashStartTime = uint32_t(int64_t(ticks / (double(GetPerformanceFrequency()) / 1000.0)));
    }
    ms_fadeColour[0] = 0;
    ms_fadeColour[1] = 0;
    ms_fadeColour[2] = 0;
    ms_fadeColour[3] = 0xFF;
    Camera_Fade(TheCamera, 0.0f, 1, 0);
    return 1;
}

// 0x43AE60: the application's per-frame steps (vf 0x2C, 0x30, 0x34, 0x48) while the
// renderer's device is in state 1, 2 or 4
static void __cdecl ApplicationFrame() {
    int state = At<int32_t>(ms_pRenderer, 0x94);
    if (state != 1 && state != 2 && state != 4) {
        return;
    }
    VCall<void>(ms_pApplication, 0x2C);
    VCall<void>(ms_pApplication, 0x30);
    VCall<void>(ms_pApplication, 0x34);
    VCall<void>(ms_pApplication, 0x48);
}

// 0x43AF90: the 2D render states, the controllers reset, the letterbox (widescreen set
// but not a 16:9 display), and the frame's end steps
static void __cdecl Begin2D() {
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
    Controller_Reset(Controllers_Get0(ms_controllerMgr));
    Controller_Reset(Controllers_Get1(ms_controllerMgr));
    if ((ms_widescreenSetting == 1 || ms_widescreenSetting != ms_widescreenSetting2) && !ms_bWidescreen) {
        Camera_Fn_4F1BE0(TheCamera);
    }
    ms_bPlayerHas140C = At<int32_t>(ms_pPlayerPed, 0x140C) != 0;
    Fn_514EF0();
    Font_Render();
    Fn_4582B0();
}

// 0x43B060: a message box with the text of the hash, captioned SYS_ERROR (UTF-8)
static void __cdecl ShowError(uint32_t hash) {
    WCHAR caption[512];
    WCHAR text[512];
    const char* message = Text_GetByHash(TheText, hash);
    const char* title = Text_Get(TheText, "SYS_ERROR");
    auto toWide = Import<int(WINAPI*)(UINT, DWORD, LPCSTR, int, LPWSTR, int)>(IMP_MultiByteToWideChar);
    toWide(CP_UTF8, 0, message, -1, text, 512);
    toWide(CP_UTF8, 0, title, -1, caption, 512);
    Import<int(WINAPI*)(HWND, LPCWSTR, LPCWSTR, UINT)>(IMP_MessageBoxW)(ms_hWnd, text, caption, 0);
}

// 0x43B0F0: the same by the text's key
static void __cdecl ShowErrorKey(const char* key) {
    App_ShowError(HashName(key));
}

// 0x43B140: the credits lines over a black box (the fade's alpha)
static void __cdecl DrawCreditLines() {
    if (ms_numCreditLines == 0) {
        return;
    }
    uint8_t colour[4] = {0, 0, 0, uint8_t(int32_t(ms_fadeAlpha))};   // (truncated)
    float w = float(double(ms_screenScaleX) * 853.0);
    float h = float(double(ms_screenScaleY) * 480.0);
    DrawQuad(0.0f, 0.0f, 0.0f, h, w, 0.0f, w, h, colour);
    Font_Begin();
    for (int i = 0; i < ms_numCreditLines; i++) {
        CreditLine& line = ms_creditLines[i];
        ms_fontColour[0] = line.colour[0];
        ms_fontColour[1] = line.colour[1];
        ms_fontColour[2] = line.colour[2];
        ms_fontColour[3] = line.colour[3];
        Font_SetScale(line.scale);
        Font_SetJustify(line.justify);
        Font_PrintString(line.x, line.y, line.text, 1);
    }
    Font_Render();
}

// 0x43B310: the loading icon (bottom left), fading out from 0.1 s to 0.4 s after input
static void __cdecl DrawLoadingIcon() {
    App_UpdateInputIgnored();
    if (!(ms_loadingIconTime < 0.4f) || ms_bUserStart || ms_bFlag_20C32EA || ms_bFlag_20C32EB || ms_bFlag_20C32F2) {
        return;
    }
    int saved8, saved6, savedC, savedA, savedB;
    GetRenderState(8, &saved8);
    GetRenderState(6, &saved6);
    GetRenderState(0xC, &savedC);
    GetRenderState(0xA, &savedA);
    GetRenderState(0xB, &savedB);
    SetRenderState(8, 0);
    SetRenderState(6, 0);
    SetRenderState(0xC, 1);
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
    float alpha = 1.0f;
    float time = ms_loadingIconTime;
    if (time >= 0.1f) {
        float t = Clamp(float((double(time) - 0.10000000149011612) / 0.30000000447034836), 0.0f, 1.0f);
        alpha = 1.0f - t * t;
    }
    uint8_t colour[4];
    RGBA_Construct(colour, 0xFF, 0xFF, 0xFF, uint8_t(int32_t(double(alpha) * 192.0)));   // (truncated)
    float top = float(double(ms_screenScaleY) * -26.0);
    float rect[4];
    rect[1] = float(double(top) + 480.0);
    rect[0] = float(double(ms_screenScaleX) * 35.0);
    float uv1[2] = {1.0f, 1.0f};
    float uv0[2] = {0.0f, 0.0f};
    float size = float(double(ms_screenScaleY) * 32.0);
    rect[3] = rect[1] - size;
    size = float(double(ms_screenScaleX) * 32.0);
    rect[2] = rect[0] + size;
    DrawSprite(rect, ms_loadingIconTexture, colour, uv0, uv1);
    SetRenderState(8, saved8);
    SetRenderState(6, saved6);
    SetRenderState(0xC, savedC);
    SetRenderState(0xA, savedA);
    SetRenderState(0xB, savedB);
    float step = ms_loadingIconSpeed * ms_timeStep;
    ms_loadingIconTime = step + ms_loadingIconTime;
}

// 0x43B580: the frame's camera for the world: cleared to the sky's colour (white while
// 0xCE754D is set and 0x538C70 says no), the camera's matrix from its node
static uint8_t __cdecl BeginWorldCameraUpdate() {
    float first[4] = {1.0f, 1.0f, 0.0f, 1.0f};
    VCall<void>(ms_pRenderer, 0x58, first);   // the background colour (then black)
    float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    VCall<void>(ms_pRenderer, 0x58, black);
    Fn_4098D0();
    Fn_4096F0();
    float tanHalf = TanHalfFov();
    uint8_t r = ms_skyColour[0];
    uint8_t g = ms_skyColour[1];
    uint8_t b = ms_skyColour[2];
    if (ms_bClearToWhite_CE754D && !Fn_538C70()) {
        r = g = b = 0xFF;
    }
    ms_clearColour[1] = g;
    ms_clearColour[0] = r;
    ms_clearColour[2] = b;
    Camera_SetView(ms_camera, 0, tanHalf, Camera_GetAspectRatio(TheCamera));
    Camera_Fn_5F2EB0(ms_camera);
    Camera_SetClearColour(ms_camera, ms_clearColour, 2);
    if (Camera_BeginUpdate(ms_camera) == 0) {
        return 0;
    }
    Matrix_FromNode(TheCameraMatrix, ms_cameraNode);
    return 1;
}

// 0x43B6D0: the Gamebryo application: made, its window class registered, its renderer's
// window made at the display's size, initialised, then the game's start-up
static int __cdecl CreateApp() {
    PushMemoryTag(0x1D);
    ms_appCmdLine_BF383C = ms_cmdLine_20E2168;
    ms_pApplication = CreateApplication();
    if (!ms_pApplication) {
        App_ShowError(HashName("SYS_APP_CREATE_ERROR"));
        Fn_753750();
        return 0;
    }
    ms_unknown20E5878 = 0;
    Fn_829EE0();
    WNDCLASSA wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = reinterpret_cast<WNDPROC>(ms_wndProc);
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = ms_hInstance;
    wc.hIcon = Import<HICON(WINAPI*)(HINSTANCE, LPCSTR)>(IMP_LoadIconA)(ms_hInstance, MAKEINTRESOURCEA(0x65));
    wc.hCursor = Import<HCURSOR(WINAPI*)(HINSTANCE, LPCSTR)>(IMP_LoadCursorA)(nullptr, MAKEINTRESOURCEA(0x7F00));
    wc.hbrBackground = static_cast<HBRUSH>(Import<HGDIOBJ(WINAPI*)(int)>(IMP_GetStockObject)(0));
    wc.lpszClassName = ms_windowClassName;
    int32_t menu = ms_pApplication[0xC];
    wc.lpszMenuName = menu != 0 ? reinterpret_cast<LPCSTR>(uintptr_t(uint16_t(menu))) : nullptr;
    Import<ATOM(WINAPI*)(const WNDCLASSA*)>(IMP_RegisterClassA)(&wc);
    DisplaySettings settings;
    GetDisplaySettings(&settings);
    const char* error = "SYS_SHADER_MODEL_ERROR";
    if (ms_bShaderModelOk != 0) {
        At<int32_t>(reinterpret_cast<void*>(ms_pApplication[0xB]), 0x1C) = settings.height;
        At<int32_t>(reinterpret_cast<void*>(ms_pApplication[0xB]), 0x14) = settings.width;
        At<uint8_t>(ms_pApplication, 0x34) = settings.windowed == 0 && !re::ForceWindowed();
        void* window = reinterpret_cast<void*>(ms_pApplication[0xB]);
        ms_hWnd = VCall<HWND>(window, 0, ms_windowArg_BD77B0, 0);
        if (VCall<uint8_t>(ms_pApplication, 8)) {
            PopMemoryTag();
            Fn_405C30();
            Fn_410C00(9, 0);
            Fn_617C30();
            Fn_5F7080();
            Fn_72BE80();
            PushMemoryTag(0x1D);
            Fn_55EF80();
            PopMemoryTag();
            return Game_Initialise_42E970();
        }
        error = "SYS_INIT_FAILED";
    }
    App_ShowError(HashName(error));
    Exit(-1);
    return 0;
}

// 0x43B8D0: the application's events (GTA's AppEventHandler): 0 the camera's view (4:3,
// the argument), 9 can render, 0x15 start-up, 0x16/0x18/0x19 the game's; 2 for the rest
static int __cdecl EventHandler(int event, int param) {
    switch (event) {
    case 0:
        Camera_SetView(ms_camera, param, TanHalfFov(), 1.3333334f);
        return 1;
    case 9:
        return App_CanRender() != 0;
    case 0x15:
        return App_Create() != 0;
    case 0x16:
        Fn_42ED70();
        return 1;
    case 0x18:
        Fn_42E450();
        return 1;
    case 0x19:
        Fn_42E4C0();
        return 1;
    default:
        return 2;
    }
}

} // namespace App2

// ---- registration ----------------------------------------------------------------------

#define APP2_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace App2 {
APP2_REVERSED("App::SetClipRect", 0x0043A6D0, &SetClipRect);
APP2_REVERSED("App::UpdateInputIgnored", 0x0043A8E0, &UpdateInputIgnored);
APP2_REVERSED("App::WaitForChange", 0x0043A990, &WaitForChange);
APP2_REVERSED("App::AllowShortcutKeys", 0x0043A9A0, &AllowShortcutKeys);
APP2_REVERSED("App::OnDisplayChange", 0x0043AA90, &OnDisplayChange);
APP2_REVERSED("App::BeginCameraUpdate", 0x0043AB30, &BeginCameraUpdate);
APP2_REVERSED("App::StartSplashFade", 0x0043AC10, &StartSplashFade);
APP2_REVERSED("App::ApplicationFrame", 0x0043AE60, &ApplicationFrame);
APP2_REVERSED("App::Begin2D", 0x0043AF90, &Begin2D);
APP2_REVERSED("App::ShowError", 0x0043B060, &ShowError);
APP2_REVERSED("App::ShowErrorKey", 0x0043B0F0, &ShowErrorKey);
APP2_REVERSED("App::DrawCreditLines", 0x0043B140, &DrawCreditLines);
APP2_REVERSED("App::DrawLoadingIcon", 0x0043B310, &DrawLoadingIcon);
APP2_REVERSED("App::BeginWorldCameraUpdate", 0x0043B580, &BeginWorldCameraUpdate);
APP2_REVERSED("App::CreateApp", 0x0043B6D0, &CreateApp);
APP2_REVERSED("App::EventHandler", 0x0043B8D0, &EventHandler);
} // namespace App2
