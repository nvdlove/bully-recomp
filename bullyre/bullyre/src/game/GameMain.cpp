// The game's main function (WinMain calls it after the start-up): world.dat loaded, then
// round and round: the title screen or the game, the frames (Idle) until a return to the
// main menu, the "SUCCES_LOAD" frames and the loading screen, the restart. The splash's
// frame (RenderSplash(1)) and one loading screen are inlined in it several times (see
// LoadingScreen.cpp). Also the model index list (Config\Extra\MIListF.bin).

#include <cmath>
#include <cstdint>

#include <windows.h>

#include "game/LoadingScreen.h"
#include "reversed.h"

namespace {

template <typename F>
F Import(uintptr_t slot) {
    // (the slot is the game's 4 bytes: read as such on x64)
    return reinterpret_cast<F>(static_cast<void*>(*reinterpret_cast<re::Ptr32<void>*>(slot)));
}
constexpr uintptr_t IMP_QueryPerformanceCounter = 0x008FC040;
constexpr uintptr_t IMP_InterlockedDecrement = 0x008FC170;
constexpr uintptr_t IMP_InterlockedIncrement = 0x008FC174;

template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

GAME_VAR(uint8_t, ms_bWorld2, 0x00BF3810);            // Config\Extra\world2.dat
GAME_VAR(uint8_t, ms_bSplashOnly, 0x00BF3818);
GAME_VAR(void*, ms_splashTexture, 0x00BF3E08);         // the splash sprite's texture
GAME_VAR(int32_t, ms_splashTxdSlot, 0x00A1360C);
GAME_VAR(uint8_t, ms_bLoadingScreenNew, 0x00BF382A);
GAME_VAR(uint8_t*, ms_pCompletion, 0x020CE75C);        // (+0x270: a reload wanted)
GAME_VAR(int32_t, ms_shouldMovePlayer, 0x00BD0FD4);
GAME_VAR(uint8_t, ms_bFirstStart_BD0FDE, 0x00BD0FDE);
GAME_VAR(uint8_t, ms_bInTitleMenu, 0x020C32F2);
GAME_VAR(uint8_t, ms_bEnsureSpecialEntities, 0x00CA3984);
GAME_VAR(uint8_t, ms_bReturnToMain, 0x020C32EC);
GAME_VAR(uint8_t, ms_bFlag_20C32EE, 0x020C32EE);
GAME_VAR(uint8_t, ms_bRestarted, 0x00CFB35C);
GAME_VAR(uint8_t, ms_bFlag_9D3450, 0x009D3450);
GAME_VAR(float, ms_loadingProgress, 0x00BF3834);
GAME_VAR(float, ms_progressAngle, 0x00A13634);
GAME_VAR(int32_t, ms_loadingLastTime, 0x00BF3E50);
GAME_ARRAY(uint8_t, ms_fadeColour, 0x00C3CD2C);        // r, g, b, a
GAME_VAR(void*, ms_pRenderer, 0x020E2380);
GAME_VAR(void*, ms_camera, 0x00C1AE7C);
GAME_VAR(int32_t, ms_fovOverrideOn, 0x00C3CDD4);
GAME_VAR(float, ms_fovOverride, 0x00C3CDD0);
GAME_VAR(float, ms_fov, 0x00C3CD04);
GAME_VAR(float, ms_screenScaleX, 0x00A957A8);
GAME_VAR(float, ms_screenScaleY, 0x00A957AC);
GAME_VAR(uint8_t, ms_bFontLoaded, 0x0208B264);
GAME_VAR(uint16_t, ms_font_208B784, 0x0208B784);
GAME_ARRAY(uint8_t, ms_fontColour, 0x0208B764);
GAME_ARRAY(uint8_t, ms_loadingTextColour, 0x020C333C);
GAME_VAR(int32_t*, ms_pRenderTarget_BCBBAC, 0x00BCBBAC);   // an NiPointer (+4: the references)
GAME_ARRAY(int32_t, ms_modelIndexList, 0x00BF3E54);         // 429 words (-1: none)
GAME_VAR(uint8_t, ms_bDebugSettingsLocked, 0x00BF3816);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const ms_controllerMgr = reinterpret_cast<void*>(0x020C3CA0);

GAME_FN(int64_t, __cdecl, GetPerformanceFrequency, 0x0045B7F0);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void, __cdecl, Game_LoadLevel, 0x0042EFE0, const char* path);
GAME_FN(void, __cdecl, Txd_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(void, __cdecl, Fn_42E710, 0x0042E710);
GAME_FN(void, __thiscall, Fn_561040, 0x00561040, void* self);
GAME_FN(void, __cdecl, Fn_45B960, 0x0045B960, int);
GAME_FN(void, __cdecl, Fn_5EFA70, 0x005EFA70);
GAME_FN(void, __cdecl, Fn_6A2620, 0x006A2620);
GAME_FN(void, __cdecl, Fn_6A2B00, 0x006A2B00);
GAME_FN(void, __cdecl, Fn_6A2A30, 0x006A2A30);
GAME_FN(void, __thiscall, Camera_Fade, 0x004F1AD0, void* camera, float seconds, int direction, int);
GAME_FN(uint32_t, __cdecl, App_IsReturningToMain, 0x0043A6B0);
GAME_FN(void, __cdecl, Fn_405680, 0x00405680);
GAME_FN(uint8_t, __cdecl, Fn_405740, 0x00405740);
GAME_FN(void, __cdecl, Fn_406140, 0x00406140);
GAME_FN(void, __cdecl, Fn_5F0FF0, 0x005F0FF0);
GAME_FN(void, __cdecl, Fn_5EFAA0, 0x005EFAA0);
GAME_FN(void, __cdecl, Fn_5EFB50, 0x005EFB50);
GAME_FN(void, __cdecl, Fn_4098D0, 0x004098D0);
GAME_FN(void, __cdecl, Fn_4096F0, 0x004096F0);
GAME_FN(float, __thiscall, Camera_GetAspectRatio, 0x004F1360, void* camera);
GAME_FN(void, __cdecl, Camera_SetView, 0x005EF8A0, void* camera, uintptr_t, float tanHalfFov, float aspect);
GAME_FN(void, __cdecl, Camera_Fn_5F2EB0, 0x005F2EB0, void* camera);
GAME_FN(int, __cdecl, Camera_BeginUpdate, 0x006C86A0, void* camera);
GAME_FN(void, __cdecl, Camera_EndUpdate, 0x006C86C0, void* camera);
GAME_FN(void, __cdecl, Fn_409860, 0x00409860);
GAME_FN(void, __cdecl, Fn_6A2E00, 0x006A2E00);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Fn_6A6D00, 0x006A6D00, void* position, const char* key, void* style);
GAME_FN(void, __cdecl, Fn_6A6510, 0x006A6510, uintptr_t);
GAME_FN(void, __cdecl, Fn_4096C0, 0x004096C0);
GAME_FN(void, __cdecl, Fn_409750, 0x00409750);
GAME_FN(void, __cdecl, Fn_4097D0, 0x004097D0);
GAME_FN(void, __cdecl, Fn_42E6B0, 0x0042E6B0, int);
GAME_FN(void, __cdecl, Fn_6A2D50, 0x006A2D50);
GAME_FN(void, __cdecl, Fn_430330, 0x00430330);
GAME_FN(void, __cdecl, Fn_45B800, 0x0045B800);
GAME_FN(void, __cdecl, Fn_533880, 0x00533880);
GAME_FN(void, __cdecl, Fn_42FE70, 0x0042FE70);
GAME_FN(void, __thiscall, Key_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, Key_Destruct, 0x0049A650, uint32_t* key);   // (just a ret)
GAME_FN(int, __thiscall, Controllers_Find, 0x006A9FD0, void* mgr, const uint32_t* key);
GAME_FN(uint8_t*, __thiscall, Controllers_Get, 0x006AA660, void* mgr, int index);
GAME_FN(void*, __cdecl, Fn_411370, 0x00411370);
// the loading screen's parts (see LoadingScreen.cpp)
GAME_FN(uint8_t*, __cdecl, Loading_GetSplash, 0x0043BEE0, const char* name);
GAME_FN(void, __cdecl, Loading_RenderSplash, 0x0043C060, int, int);
GAME_FN(void, __cdecl, Fn_4056D0, 0x004056D0);
GAME_FN(float, __cdecl, Screen_GetWidth, 0x006A29C0);
GAME_FN(float, __cdecl, Screen_GetHeight, 0x006A29E0);
GAME_FN(uint8_t, __cdecl, Fn_407430, 0x00407430);
GAME_FN(void, __cdecl, Fn_4094B0, 0x004094B0, int);
GAME_FN(void, __cdecl, Fn_409520, 0x00409520, int, int);
GAME_FN(void, __cdecl, Fn_68A250, 0x0068A250);
GAME_FN(void, __cdecl, Fn_5EE1D0, 0x005EE1D0);
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __thiscall, Sprite_Draw, 0x0052C6F0, void* sprite, const float* rect, const uint8_t* colour);
GAME_FN(void, __cdecl, DrawCircle, 0x0052D090, const float* centre, float radius, const uint8_t* colour, int segments);
GAME_FN(void, __cdecl, DrawArc, 0x0052D200, const float* centre, float radius, const uint8_t* colour, int segments, float start, float end);
GAME_FN(void, __cdecl, Font_SetStyle, 0x00688050, int);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(const char*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Text_CopyToWide, 0x00688ED0, uint16_t* out, const char* text, int size);
GAME_FN(void, __cdecl, AsciiToWide, 0x00690090, const char* text, uint16_t* out);
GAME_FN(float*, __cdecl, ScreenPosition, 0x0052D4F0, float* out, float x, float y, int, int);
GAME_FN(void, __cdecl, EmptyPrint, 0x0049A650, float x, float y, const uint16_t* text);   // (just a ret)
// the file reader's
GAME_FN(void*, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(void, __cdecl, File_Close, 0x0042D450, void* file);
// (ours, called where modes apply)
GAME_FN(void, __cdecl, Game_Idle, 0x0043C960);

int32_t NowMs() {
    LARGE_INTEGER counter;
    if (!Import<BOOL(WINAPI*)(LARGE_INTEGER*)>(IMP_QueryPerformanceCounter)(&counter)) {
        return 1;
    }
    double ticks = double(counter.QuadPart);
    return int32_t(int64_t(ticks / (double(GetPerformanceFrequency()) / 1000.0)));
}

// the frame's camera, cleared to black (App::BeginCameraUpdate, inlined)
bool BeginCamera() {
    float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    VCall<void>(ms_pRenderer, 0x58, black);
    Fn_4098D0();
    Fn_4096F0();
    float fov = ms_fovOverrideOn == 0 ? ms_fov : ms_fovOverride;
    float aspect = Camera_GetAspectRatio(TheCamera);
    float half = float(double(fov) * 0.008726646502812704);
    float tanHalf = float(std::tan(double(half)));   // (0x85C600: the CRT's x87 tan)
    Camera_SetView(ms_camera, 0, tanHalf, aspect);
    Camera_Fn_5F2EB0(ms_camera);
    return Camera_BeginUpdate(ms_camera) != 0;
}

void EndCamera() {   // (App::EndLoadingScreenFrame(0), inlined)
    Camera_EndUpdate(ms_camera);
    Fn_4096C0();
    Fn_409750();
    Fn_4097D0();
    Fn_42E6B0(0);
}

// App::DrawLoadingScreen(_, "LoadIconStart", nullptr) (0x43C4F0) as inlined here, from
// after its timing (the camera set-up and the frame's end inlined too)
void LoadingScreenInlined() {
    uint8_t* sprite = nullptr;
    if (const char* name = LoadingScreen::LoadingScreenName(nullptr)) {
        sprite = Loading_GetSplash(name);
    }
    Fn_4056D0();
    Screen_GetHeight();
    Screen_GetWidth();
    if (!BeginCamera()) {
        return;
    }
    if (!Fn_407430()) {
        Fn_4094B0(7);
        Fn_4096C0();
        Fn_409520(7, 1);
    }
    if (ms_bFontLoaded) {
        Fn_68A250();
    }
    Fn_5EE1D0();
    float w = Screen_GetWidth() * ms_screenScaleX;
    float h = Screen_GetHeight();
    float rect[4] = {0.0f, h, w, 0.0f};
    uint8_t white[4];
    RGBA_Construct(white, 0xFF, 0xFF, 0xFF, 0xFF);
    SetRenderState(2, 3);
    Sprite_Draw(sprite, rect, white);
    if (ms_bFontLoaded) {
        uint8_t ring[4];
        uint8_t fill[4];
        RGBA_Construct(ring, 0x31, 0x41, 0x41, 0xFF);
        RGBA_Construct(fill, 0xDF, 0xC5, 0x42, 0xFF);
        float centreY = float(double(Screen_GetHeight()) - 30.0 - 25.0);
        float centre[2] = {float(double(w) * 0.5), centreY};
        float progress = float(double(ms_loadingProgress) / 1350.0);
        if (1.0f < progress) {
            progress = 1.0f;
        }
        if (1.0f < ms_loadingProgress) {
            uint8_t black[4];
            RGBA_Construct(black, 0, 0, 0, 0xFF);
            DrawCircle(centre, 27.0f, black, 0x48);
            DrawCircle(centre, 25.0f, ring, 0x48);
            float angle = ms_progressAngle * progress;
            DrawArc(centre, 25.0f, fill, 0x48, 0.0f, angle);
        }
        uint16_t text[80];
        Font_Begin();
        Font_SetJustify(1);
        Font_SetStyle(1);
        Font_SetJustify(0);
        Font_SetStyle(1);
        AsciiToWide("Loading", text);
        EmptyPrint(40.0f, float(double(Screen_GetHeight()) - 70.0), text);
        float y = float(double(ms_screenScaleY) * -50.0);
        for (int i = 0; i < 4; i++) {
            ms_fontColour[i] = ms_loadingTextColour[i];   // (0x424480, inlined)
        }
        float x = float(double(ms_screenScaleX) * 70.0);
        ms_font_208B784 = 1;
        float out[2];
        ScreenPosition(out, x, y, 0, 2);
        uint16_t loading[512];
        Text_CopyToWide(loading, Text_Get(TheText, "RARC_LOADING"), 0x200);
        Font_PrintString(out[0], out[1], loading, 1);
        AsciiToWide("LoadIconStart", text);
        EmptyPrint(40.0f, float(double(Screen_GetHeight()) - 50.0), text);
        if (ms_bFontLoaded) {
            int32_t* target = ms_pRenderTarget_BCBBAC;
            void* renderer = ms_pRenderer;
            if (target) {
                Import<LONG(WINAPI*)(volatile LONG*)>(IMP_InterlockedIncrement)(reinterpret_cast<volatile LONG*>(target + 1));
            }
            bool changed = VCall<int32_t*>(renderer, 0x78) != target;
            if (target) {
                if (Import<LONG(WINAPI*)(volatile LONG*)>(IMP_InterlockedDecrement)(reinterpret_cast<volatile LONG*>(target + 1)) == 0) {
                    VCall<void>(target, 4);   // DeleteThis
                }
            }
            if (changed) {
                Fn_4096C0();
                Fn_409520(7, 1);
            }
            Font_Render();
        }
    }
    EndCamera();
}

// the model one of the list's entries (by index)
uint32_t IsListed(int model, const int16_t* entries, int count) {
    for (int i = 0; i < count; i++) {
        if (model == ms_modelIndexList[entries[i]]) {
            return 1;
        }
    }
    return 0;
}

void FadeFromBlack(int direction, int unknown) {
    ms_fadeColour[0] = 0;
    ms_fadeColour[1] = 0;
    ms_fadeColour[2] = 0;
    ms_fadeColour[3] = 0xFF;
    Camera_Fade(TheCamera, 0.0f, direction, unknown);
}

} // namespace

namespace GameMain {

// 0x43CF20: the game's main function
static void __cdecl Main() {
    PushMemoryTag(0x21);
    PopMemoryTag();
    Game_LoadLevel(ms_bWorld2 ? "Config\\Extra\\world2.dat" : "Config\\world.dat");
    ms_bSplashOnly = 1;
    ms_splashTexture = nullptr;
    if (ms_splashTxdSlot != -1) {
        Txd_RemoveTxdSlot(ms_splashTxdSlot);
    }
    ms_splashTxdSlot = -1;
    ms_bLoadingScreenNew = 1;
    bool restarted = false;
    for (;;) {
        LoadingScreen::RenderSplashInlined(false);
        if (ms_pCompletion[0x270]) {
            ms_shouldMovePlayer = 2;
        }
        LoadingScreen::RenderSplashInlined(true);
        Fn_42E710();
        Fn_561040(reinterpret_cast<void*>(0x00CF1F60));
        Fn_45B960(0);
        Fn_5EFA70();
        if (ms_pCompletion[0x270] || (ms_bFirstStart_BD0FDE != 1 && restarted)) {
            Fn_6A2620();
        } else {
            ms_bInTitleMenu = 1;
            Fn_6A2B00();
            Fn_6A2A30();
            ms_bEnsureSpecialEntities = 1;
            Camera_Fade(TheCamera, 0.0f, 0, 0);
            ms_bFirstStart_BD0FDE = 0;
        }
        while (!App_IsReturningToMain()) {
            Fn_405680();
            if (!Fn_405740()) {
                Fn_45B960(0);
            } else {
                Fn_406140();
                Fn_5F0FF0();
                Game_Idle();
                Fn_5EFAA0();
            }
        }
        Fn_5EFB50();
        uint8_t reload = ms_pCompletion[0x270];
        FadeFromBlack(1, 1);
        LoadingScreen::RenderSplashInlined(true);
        for (int i = 2; i != 0; i--) {
            if (BeginCamera()) {
                Fn_6A2B00();
                Fn_409860();
                if (reload) {
                    Fn_6A2E00();
                    Font_Begin();
                    struct {
                        int32_t unknown0;   // +0 (1)
                        float x;            // +4
                        float y;            // +8
                    } position = {1, 0.0f, 124.0f};
                    struct {
                        int32_t unknown0;      // +0 (2)
                        int32_t unknown4;      // +4 (2)
                        float unknown8;        // +8
                        float unknownC;        // +0xC
                        float unknown10;       // +0x10
                        int32_t unknown14;     // +0x14 (1)
                        uint8_t unknown18[4];  // +0x18 (three zero bytes; the fourth unset)
                    } style;
                    style.unknown0 = 2;
                    style.unknown4 = 2;
                    style.unknown8 = 0.0f;
                    style.unknownC = 0.0f;
                    style.unknown10 = 0.0f;
                    style.unknown14 = 1;
                    style.unknown18[0] = 0;
                    style.unknown18[1] = 0;
                    style.unknown18[2] = 0;
                    ms_font_208B784 = 0;
                    Fn_6A6D00(&position, "SUCCES_LOAD", &style);
                    Fn_6A6510(0);
                }
                Loading_RenderSplash(0, 0);
                EndCamera();
                Fn_6A2D50();
            }
        }
        int32_t now = NowMs();
        if (now - ms_loadingLastTime >= 40) {
            ms_loadingLastTime = now;
            if (ms_bSplashOnly) {
                LoadingScreen::RenderSplashInlined(true);
            } else if (!ms_bLoadingScreenNew) {
                LoadingScreenInlined();
            }
        }
        Fn_430330();
        Fn_45B800();
        if (!ms_bReturnToMain) {
            return;
        }
        LoadingScreen::RenderSplashInlined(false);
        uint8_t flagEE = ms_bFlag_20C32EE;
        restarted = true;
        Fn_533880();
        Fn_42FE70();
        ms_loadingProgress = 0.0f;
        ms_bReturnToMain = 0;
        ms_bFlag_20C32EE = 0;
        ms_bRestarted = 1;
        uint32_t key;
        Key_Construct(&key, "1_01");
        int index = Controllers_Find(ms_controllerMgr, &key);
        Key_Destruct(&key);
        if (index != -1 && !ms_bFirstStart_BD0FDE && !flagEE) {
            if (ms_bFlag_9D3450) {
                void* object = Fn_411370();
                VCall<void>(object, 4, reinterpret_cast<void*>(0x020C3220), 2);
                ms_shouldMovePlayer = 2;
                Controllers_Get(ms_controllerMgr, index)[0x33] = 1;
            } else {
                ms_shouldMovePlayer = 0;
            }
            Fn_42E710();
        }
        FadeFromBlack(1, 0);
    }
}

// 0x43EA60: the model index list: read from Config\Extra\MIListF.bin while the debug
// settings are locked (its size, then that many bytes at the list's last word), else all -1
static void __cdecl LoadModelIndexList() {
    if (ms_bDebugSettingsLocked) {
        char path[] = "Config\\Extra\\MIListF.bin";
        void* file = File_Open(path, "r", 1);
        int32_t size;
        File_Read(file, &size, 4);
        File_Read(file, &ms_modelIndexList[428], size);
        File_Close(file);
        return;
    }
    for (int i = 428; i >= 0; i--) {
        ms_modelIndexList[i] = -1;
    }
}


// 0x43F3A0: is the model one of these list entries (in this order: 14 of them). A byte
// bool (xor al, al / mov al, 1: eax's upper bytes are the model's), unlike the others
static uint8_t __cdecl IsModelInList14(int model) {
    static const int16_t entries[] = {424, 420, 425, 413, 411, 412, 410, 409, 408, 407, 406, 405, 404, 403};
    return uint8_t(IsListed(model, entries, sizeof(entries) / sizeof(entries[0])));
}

// 0x43F420: is the model one of these list entries (in this order: 25 of them)
static uint32_t __cdecl IsModelInList25(int model) {
    static const int16_t entries[] = {259, 258, 257, 256, 255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 237, 236, 235};
    return IsListed(model, entries, sizeof(entries) / sizeof(entries[0]));
}

// 0x43F520: is the model one of these list entries (in this order: 30 of them)
static uint32_t __cdecl IsModelInList30(int model) {
    static const int16_t entries[] = {234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, 207, 206, 205};
    return IsListed(model, entries, sizeof(entries) / sizeof(entries[0]));
}

// 0x43F660: is the model one of these list entries (in this order: 3 of them)
static uint32_t __cdecl IsModelInList3(int model) {
    static const int16_t entries[] = {382, 417, 310};
    return IsListed(model, entries, sizeof(entries) / sizeof(entries[0]));
}

} // namespace GameMain

// ---- registration ----------------------------------------------------------------------

#define GAMEMAIN_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace GameMain {
GAMEMAIN_REVERSED("Game::Main", 0x0043CF20, &Main);
GAMEMAIN_REVERSED("Game::LoadModelIndexList", 0x0043EA60, &LoadModelIndexList);
GAMEMAIN_REVERSED("ModelIndexList::IsModelInList14", 0x0043F3A0, &IsModelInList14);
GAMEMAIN_REVERSED("ModelIndexList::IsModelInList25", 0x0043F420, &IsModelInList25);
GAMEMAIN_REVERSED("ModelIndexList::IsModelInList30", 0x0043F520, &IsModelInList30);
GAMEMAIN_REVERSED("ModelIndexList::IsModelInList3", 0x0043F660, &IsModelInList3);
} // namespace GameMain
