// The loading screen's drawing (App.cpp has its state): the splash picture (the loading
// screens "loadsc%02d" / "wloads%02d" from TXD\ls or TXD\lsw), the fading pictures over it
// ("fade%02d" from fade.nft, one every 5 seconds), the progress circle and the
// "Loading" texts. The frame's time is the performance counter in ms, and each screen is
// drawn at most every 40 ms.

#include <cstdint>

#include <windows.h>

#include "game/LoadingScreen.h"
#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

template <typename F>
F Import(uintptr_t slot) {
    // (the slot is the game's 4 bytes: read as such on x64)
    return reinterpret_cast<F>(static_cast<void*>(*reinterpret_cast<re::Ptr32<void>*>(slot)));
}
constexpr uintptr_t IMP_QueryPerformanceCounter = 0x008FC040;
constexpr uintptr_t IMP_InterlockedDecrement = 0x008FC170;

template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

GAME_VAR(uint8_t, ms_bWidescreen, 0x00C1870E);
GAME_VAR(CPool<uint8_t>*, ms_pTxdPool, 0x01B64654);   // (a TxdDef's +0: its dictionary)
GAME_VAR(void*, ms_pRenderer, 0x020E2380);
GAME_VAR(float, ms_screenScaleX, 0x00A957A8);
GAME_VAR(float, ms_screenScaleY, 0x00A957AC);
GAME_VAR(float, ms_fadeAlpha, 0x00C3CD28);            // TheCamera's fade
GAME_ARRAY(uint8_t, ms_fadeColour, 0x00C3CD2C);       // r, g, b, a
GAME_VAR(uint32_t, ms_splashStartTime, 0x00BF3840);
GAME_VAR(int32_t, ms_loadingAlpha_C18708, 0x00C18708);
GAME_VAR(uint32_t, ms_spriteColour, 0x020DCD14);      // the next sprite's colour
GAME_VAR(uint8_t, ms_bUserStart, 0x020C32E9);
GAME_VAR(uint8_t, ms_bFlag_20C32EA, 0x020C32EA);
GAME_VAR(uint8_t, ms_bFlag_20C32EB, 0x020C32EB);
GAME_VAR(uint8_t, ms_bFlag_20C32F2, 0x020C32F2);
GAME_VAR(uint8_t, ms_bSkipSplash_BF381B, 0x00BF381B);
GAME_VAR(uint8_t, ms_bLoadingScreenPaused, 0x00BF3828);
GAME_VAR(uint8_t, ms_bSplashOnly_BF3818, 0x00BF3818);
GAME_VAR(uint8_t, ms_bLoadingScreenNew, 0x00BF382A);
GAME_VAR(float, ms_loadingProgress, 0x00BF3834);      // (of 1350)
GAME_VAR(float, ms_progressAngle, 0x00A13634);
GAME_VAR(int32_t, ms_splashLastTime, 0x00BF3E4C);
GAME_VAR(int32_t, ms_loadingLastTime, 0x00BF3E50);
// the fading pictures
GAME_VAR(void*, ms_fadeDictionary, 0x00BF3E28);
GAME_VAR(int32_t, ms_bShowLoadingText, 0x00BF3E2C);
GAME_VAR(int32_t, ms_fadeIndex, 0x00BF3E30);           // 0-4
GAME_VAR(uint8_t, ms_bFadeStarted, 0x00BF3E34);
GAME_VAR(int32_t*, ms_fadeTexture, 0x00BF3E38);         // NiTexture (+4: its references)
GAME_VAR(int32_t, ms_fadeTimeLeft, 0x00BF3E3C);         // ms
GAME_VAR(int32_t, ms_fadeLastTime, 0x00BF3E40);
GAME_VAR(int32_t, ms_fadeTxdSlot, 0x00A13698);
GAME_ARRAY(uint8_t, ms_fadeInColour, 0x00BF3E44);       // (0, 0, 0, 0 once made)
GAME_VAR(uint32_t, ms_fadeInColourGuard, 0x00BF3E48);
// the splash
GAME_VAR(void*, ms_splashNames, 0x00BF3844);            // the names already loaded
GAME_VAR(int32_t, ms_splashTxdSlot, 0x00A1360C);
inline uint8_t* const ms_splashSprite = reinterpret_cast<uint8_t*>(0x00BF3E08);   // CSprite2d
// the loading screen's name
GAME_ARRAY(char, ms_loadingScreenName, 0x00BF3E10);
GAME_VAR(uint32_t, ms_loadingScreenNameTime, 0x00BF3E20);
GAME_VAR(uint32_t, ms_loadingScreenNameGuard, 0x00BF3E24);
GAME_VAR(int32_t, ms_loadingScreenIndex, 0x00BF3E0C);
GAME_ARRAY(int32_t, ms_loadingScreenOrder, 0x00A13610);   // 9
GAME_VAR(uint8_t, ms_bNextLoadingScreen, 0x00A13672);
// the font's
GAME_VAR(uint8_t, ms_bFontLoaded, 0x0208B264);
GAME_VAR(int32_t, ms_font_208B234, 0x0208B234);
GAME_VAR(uint16_t, ms_font_208B788, 0x0208B788);
GAME_VAR(uint16_t, ms_font_208B784, 0x0208B784);
GAME_ARRAY(uint8_t, ms_fontDropColour, 0x0208B780);
GAME_ARRAY(uint8_t, ms_fontColour, 0x0208B764);
GAME_ARRAY(uint8_t, ms_hudDropColour, 0x020C331C);
GAME_ARRAY(uint8_t, ms_hudTextColour, 0x020C3328);
inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const ms_loadingTextStyle = reinterpret_cast<void*>(0x020C333C);

GAME_FN(int64_t, __cdecl, GetPerformanceFrequency, 0x0045B7F0);
GAME_FN(uint32_t, __cdecl, GetTimeInMilliseconds, 0x0045BCA0);
template <typename... A>
inline int __cdecl Sprintf(char* out, const char* format, A... rest) { return crt::Sprintf(out, format, rest...); }
inline int __cdecl Atexit(void* fn) { return crt::Atexit((void(__cdecl*)())fn); }
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(uint8_t, __cdecl, Txd_LoadTxd, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_RemoveTxd, 0x005F1830, int slot);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, PopTxdMemoryTag, 0x0052E710);
GAME_FN(int32_t*, __cdecl, Dictionary_FindTexture, 0x005F1610, void* dict, const char* name);
GAME_FN(void*, __cdecl, App_GetTxdDictionary, 0x0043B9D0, int slot);
GAME_FN(void*, __cdecl, NiMemObject_New, 0x00752460, size_t size);
GAME_FN(void*, __thiscall, NameSet_Construct, 0x007740D0, void* set, int size);
GAME_FN(uint8_t, __cdecl, NameSet_Contains, 0x00773DC0, void* set, const char* name);
GAME_FN(void, __thiscall, NameSet_Add, 0x007745E0, void* set, const char* name);
GAME_FN(void, __thiscall, Sprite_Delete, 0x0052C6D0, void* sprite);
GAME_FN(void, __thiscall, Sprite_SetTexture, 0x0052C230, void* sprite, const char* name);
GAME_FN(void, __thiscall, Sprite_Draw, 0x0052C6F0, void* sprite, const float* rect, const uint8_t* colour);
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __thiscall, Camera_Fade, 0x004F1AD0, void* camera, float seconds, int direction, int);
GAME_FN(void, __thiscall, Camera_Fn_4F2090, 0x004F2090, void* camera);
GAME_FN(void, __cdecl, Fn_45B960, 0x0045B960, int);
GAME_FN(int, __cdecl, Fn_55B740, 0x0055B740);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetStyle, 0x00688050, int);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(const char*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Text_CopyToWide, 0x00688ED0, uint16_t* out, const char* text, int size);
GAME_FN(void, __cdecl, AsciiToWide, 0x00690090, const char* text, uint16_t* out);
GAME_FN(float*, __cdecl, ScreenPosition, 0x0052D4F0, float* out, float x, float y, int, int);
GAME_FN(void, __cdecl, EmptyPrint, 0x0049A650, float x, float y, const uint16_t* text);   // (just a ret: a print left out)
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, SetSpriteTexture, 0x00738C00, void* texture);
GAME_FN(void, __cdecl, DrawScreenQuad, 0x007395B0, float, float, float, float, float, float);
GAME_FN(void, __cdecl, DrawCircle, 0x0052D090, const float* centre, float radius, const uint8_t* colour, int segments);
GAME_FN(void, __cdecl, DrawArc, 0x0052D200, const float* centre, float radius, const uint8_t* colour, int segments, float start, float end);
GAME_FN(float, __cdecl, MinF, 0x00414470, float a, float b);
GAME_FN(float, __cdecl, MaxF, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, Screen_GetWidth, 0x006A29C0);    // 640, or the widescreen's
GAME_FN(float, __cdecl, Screen_GetHeight, 0x006A29E0);   // 480
GAME_FN(uint8_t, __cdecl, Fn_404F40, 0x00404F40);
GAME_FN(void, __cdecl, Fn_409860, 0x00409860);
GAME_FN(void, __cdecl, Fn_5EE1D0, 0x005EE1D0);
GAME_FN(void, __cdecl, Fn_738780, 0x00738780);
GAME_FN(void, __cdecl, Fn_68D6C0, 0x0068D6C0);
GAME_FN(void, __cdecl, Fn_4098D0, 0x004098D0);
GAME_FN(void, __cdecl, Fn_4056D0, 0x004056D0);
GAME_FN(uint8_t, __cdecl, Fn_407430, 0x00407430);
GAME_FN(void, __cdecl, Fn_4094B0, 0x004094B0, int);
GAME_FN(void, __cdecl, Fn_4096C0, 0x004096C0);
GAME_FN(void, __cdecl, Fn_409520, 0x00409520, int, int);
GAME_FN(void, __cdecl, Fn_68A250, 0x0068A250);
GAME_FN(void, __cdecl, Fn_424480, 0x00424480, void*);
GAME_FN(int32_t*, __cdecl, Fn_407BB0, 0x00407BB0, void* out);
GAME_FN(void, __thiscall, Pointer_Release, 0x007549D0, void* pointer);
GAME_FN(void, __cdecl, App_EndLoadingScreenFrame, 0x0043ACE0, int);
// (ours, called where modes apply)
GAME_FN(void, __cdecl, App_SetClipRect, 0x0043A6D0, float left, float top, float right, float bottom);
GAME_FN(uint8_t, __cdecl, App_BeginCameraUpdate, 0x0043AB30, int, int, int, int, int, int, int, int, int, int, int, int, int);
GAME_FN(uint8_t*, __cdecl, Loading_GetSplash, 0x0043BEE0, const char* name);
GAME_FN(void, __cdecl, Loading_RenderSplash, 0x0043C060, int, int);

// the performance counter in ms (1 when there's none)
int32_t NowMs() {
    LARGE_INTEGER counter;
    if (!Import<BOOL(WINAPI*)(LARGE_INTEGER*)>(IMP_QueryPerformanceCounter)(&counter)) {
        return 1;
    }
    double ticks = double(counter.QuadPart);
    return int32_t(int64_t(ticks / (double(GetPerformanceFrequency()) / 1000.0)));   // (_ftol's low word)
}

// an NiPointer's reference dropped (deleted at none)
void Release(int32_t* object) {
    if (Import<LONG(WINAPI*)(volatile LONG*)>(IMP_InterlockedDecrement)(reinterpret_cast<volatile LONG*>(object + 1)) == 0) {
        VCall<void>(object, 4);   // DeleteThis
    }
}

void* TxdDictionary(int slot) {
    CPool<uint8_t>* pool = ms_pTxdPool;
    uint8_t* def = (pool->flags[slot] & 0x80) ? nullptr : pool->objects + pool->elemSize * slot;
    return *reinterpret_cast<re::Ptr32<void>*>(def);   // (a free slot reads through nullptr, as the game)
}

void Set2DRenderStates() {
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
}

} // namespace

namespace LoadingScreen {

// 0x43AD80: the loading screen's texture name: `name` (in esi) when given, else the next
// of the nine every 3 seconds (or when asked), "wloads%02d" in widescreen, "loadsc%02d" not.
// (esi: not hooked; called directly)
char* LoadingScreenName(const char* name) {
    uint32_t seconds = GetTimeInMilliseconds() / 1000;
    if (!(ms_loadingScreenNameGuard & 1)) {
        ms_loadingScreenNameGuard |= 1;
        ms_loadingScreenNameTime = seconds;
    }
    if (name) {
        char* out = ms_loadingScreenName;
        while ((*out++ = *name++) != 0) {
        }
        ms_loadingScreenNameTime = 0;
        return ms_loadingScreenName;
    }
    if (seconds - ms_loadingScreenNameTime > 3 || ms_bNextLoadingScreen == 1) {
        ms_bNextLoadingScreen = 0;
        ms_loadingScreenNameTime = seconds;
        Sprintf(ms_loadingScreenName, ms_bWidescreen ? "wloads%02d" : "loadsc%02d",
                ms_loadingScreenOrder[ms_loadingScreenIndex]);
        if (++ms_loadingScreenIndex >= 9) {
            ms_loadingScreenIndex = 0;
        }
    }
    return ms_loadingScreenName;
}

// 0x43BA00: the fading pictures (the colour's alpha 0xFF: they run; else they stop) and
// the "Loading" text, then the sprite colour (white with a picture) and texture for the
// frame's quad. (The colour in edi: not hooked; called directly)
void DrawFade(uint8_t* colour) {
    int32_t now = NowMs();
    if (colour[3] == 0xFF) {
        int32_t elapsed = now - ms_fadeLastTime;
        ms_fadeLastTime = now;
        if (elapsed < 0) {
            ms_fadeTimeLeft = 0;
        } else {
            ms_fadeTimeLeft -= elapsed;
        }
        if (!ms_fadeTexture && !ms_bFadeStarted) {
            ms_fadeIndex = (ms_fadeIndex + 1) % 5;
            ms_fadeTimeLeft = 5000;
            ms_bFadeStarted = 1;
        } else if (ms_fadeTimeLeft < 1) {
            ms_fadeIndex = (ms_fadeIndex + 1) % 5;
            ms_fadeTimeLeft = 5000;
            ms_bShowLoadingText = Fn_55B740();
            char name[128];
            Sprintf(name, ms_bWidescreen ? "wfade%02d" : "fade%02d", ms_fadeIndex + 1);
            if (ms_fadeTexture) {
                Release(ms_fadeTexture);
                ms_fadeTexture = nullptr;
            }
            if (ms_fadeTxdSlot == -1) {
                ms_fadeTxdSlot = Txd_AddTxdSlot("fade");
            }
            ms_fadeDictionary = TxdDictionary(ms_fadeTxdSlot);
            bool found = true;
            if (!ms_fadeDictionary) {
                char path[128];
                PushTxdMemoryTag();
                Sprintf(path, ms_bWidescreen ? "TXD\\lsw\\fade.nft" : "TXD\\ls\\fade.nft");
                Txd_LoadTxd(ms_fadeTxdSlot, path);
                Txd_AddRef(ms_fadeTxdSlot);
                PopTxdMemoryTag();
                ms_fadeDictionary = App_GetTxdDictionary(ms_fadeTxdSlot);
                found = ms_fadeDictionary != nullptr;
            }
            if (found) {
                ms_fadeTexture = Dictionary_FindTexture(ms_fadeDictionary, name);
            }
        }
    } else {
        ms_bShowLoadingText = 0;
        if (ms_fadeTexture) {
            Release(ms_fadeTexture);
        }
        ms_fadeTexture = nullptr;
        ms_bFadeStarted = 0;
    }

    if (ms_bFontLoaded && ms_bShowLoadingText) {
        Font_Begin();
        if (ms_font_208B234 != 0) {
            ms_font_208B788 = 7;
            ms_font_208B784 = 0;
        }
        Font_SetStyle(2);
        float scaleX = ms_screenScaleX;
        float scaleY = ms_screenScaleY;
        for (int i = 0; i < 4; i++) {
            ms_fontDropColour[i] = ms_hudDropColour[i];
        }
        for (int i = 0; i < 4; i++) {
            ms_fontColour[i] = ms_hudTextColour[i];
        }
        ms_font_208B784 = 1;
        Font_SetJustify(2);
        float y = float(double(scaleY) * -160.0);
        float x = float(double(scaleX) * 0.0);
        float out[2];
        float* pos = ScreenPosition(out, x, y, 1, 1);
        float px = pos[0];
        float py = pos[1];
        uint16_t text[512];
        Text_CopyToWide(text, Text_Get(TheText, "RARC_LOADING"), 0x200);
        Font_PrintString(px, py, text, 1);
    }

    if (ms_fadeTexture) {
        colour[0] = 0xFF;
        colour[1] = 0xFF;
        colour[2] = 0xFF;
        uint32_t width = VCall<uint32_t>(ms_fadeTexture, 0x48);
        uint32_t height = VCall<uint32_t>(ms_fadeTexture, 0x4C);
        App_SetClipRect(0.0f, float(height), float(width), 0.0f);
    }
    ms_spriteColour = *reinterpret_cast<uint32_t*>(colour);
    SetSpriteTexture(ms_fadeTexture);
}

// 0x43BE40: the splash's start: black, faded in, the fading pictures stopped
static void __cdecl StartSplash(int start) {
    ms_splashStartTime = 0;
    if (!re::Flag(start)) {
        return;
    }
    ms_fadeColour[0] = 0;
    ms_fadeColour[1] = 0;
    ms_fadeColour[2] = 0;
    ms_fadeColour[3] = 0xFF;
    Camera_Fade(TheCamera, 0.0f, 0, 0);
    Fn_45B960(1);
    Camera_Fn_4F2090(TheCamera);
    if (!(ms_fadeInColourGuard & 1)) {
        ms_fadeInColourGuard |= 1;
        ms_fadeInColour[0] = 0;
        ms_fadeInColour[1] = 0;
        ms_fadeInColour[2] = 0;
        ms_fadeInColour[3] = 0;
        Atexit(reinterpret_cast<void*>(0x008F1DE0));   // (an empty destructor)
    }
    DrawFade(ms_fadeInColour);
}

// 0x43BEE0: the splash sprite with the picture (loaded from TXD\ls\<name>.nft, or lsw,
// into the "splash" slot unless the dictionary has it; each name remembered once)
static uint8_t* __cdecl GetSplash(const char* name) {
    if (!ms_splashNames) {
        void* memory = NiMemObject_New(4);
        ms_splashNames = memory ? NameSet_Construct(memory, 0x100) : nullptr;
    } else if (NameSet_Contains(ms_splashNames, name)) {
        return ms_splashSprite;
    }
    NameSet_Add(ms_splashNames, name);
    PushTxdMemoryTag();
    if (!name) {
        PopTxdMemoryTag();
        return ms_splashSprite;
    }
    if (ms_splashTxdSlot == -1) {
        ms_splashTxdSlot = Txd_AddTxdSlot("splash");
    }
    void* dict = TxdDictionary(ms_splashTxdSlot);
    if (!dict || !Dictionary_FindTexture(dict, name)) {
        char path[128];
        Sprintf(path, ms_bWidescreen ? "TXD\\lsw\\%s.nft" : "TXD\\ls\\%s.nft", name);
        Sprite_Delete(ms_splashSprite);
        if (dict) {
            Txd_RemoveTxd(ms_splashTxdSlot);
        }
        Txd_LoadTxd(ms_splashTxdSlot, path);
        Txd_AddRef(ms_splashTxdSlot);
        Txd_PushCurrentTxd();
        Txd_SetCurrentTxd(ms_splashTxdSlot);
        Sprite_SetTexture(ms_splashSprite, name);
        Txd_PopCurrentTxd();
    }
    PopTxdMemoryTag();
    return ms_splashSprite;
}

// RenderSplash's frame while the camera fades (the fade not 0): the fading pictures at
// the fade's colour, the alpha from the fade and 0xC18708
static void FadeFrame(float fade, bool fromLoading, bool calls404F40) {
    uint8_t colour[4];
    if (fade != 0.0f) {   // (NaN too)
        Fn_738780();
    }
    Loading_GetSplash(nullptr);
    int32_t fadeAlpha = int32_t(int64_t(ms_fadeAlpha));
    float level = float(ms_loadingAlpha_C18708);
    if (!(level <= 255.0f)) {
        level = 255.0f;
    }
    level = float(int32_t(int64_t(level)));
    if (level <= 150.0f) {
        level = 150.0f;
    }
    int32_t loadAlpha = int32_t(int64_t(level));
    bool frontend = calls404F40 ? Fn_404F40() != 0 : (ms_bFlag_20C32EB || ms_bFlag_20C32F2);   // (0x404F40's test)
    if (fadeAlpha != 0xFF && (ms_bUserStart || ms_bFlag_20C32EA || frontend)) {
        fadeAlpha = 0;
    }
    int32_t range = fadeAlpha - loadAlpha + 0xFF;
    uint8_t alpha;
    if (range == 0) {
        alpha = 0;
        colour[2] = 0;
        colour[1] = 0;
        colour[0] = 0;
    } else {
        colour[0] = uint8_t(int32_t(ms_fadeColour[0]) * fadeAlpha / range);
        colour[1] = uint8_t(int32_t(ms_fadeColour[1]) * fadeAlpha / range);
        colour[2] = uint8_t(int32_t(ms_fadeColour[2]) * fadeAlpha / range);
        float value = float(0xFF - (0xFF - fadeAlpha) * loadAlpha / 0xFF);
        if (value < 0.0f) {
            value = 0.0f;
        }
        alpha = uint8_t(int32_t(value));   // (truncated)
    }
    float value = float(alpha);
    if (value < 0.0f) {
        value = 0.0f;
    }
    alpha = uint8_t(int32_t(value));
    colour[3] = alpha > 0xFA ? 0xFF : alpha;
    Fn_409860();
    Fn_5EE1D0();
    Set2DRenderStates();
    DrawFade(colour);
    DrawScreenQuad(0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 0.0f);
    Font_Render();
    if (fromLoading) {
        Fn_68D6C0();
        Fn_4098D0();
    }
}

// 0x43C060: the splash (the start-up's legal screens): while the camera fades, the fading
// pictures at the fade's colour and alpha; else a black quad whose alpha follows 0xC18708.
// (`fromLoading`: at most every 40 ms, and not in the splash's first second)
static void __cdecl RenderSplash(int fromLoading, int) {
    if (ms_bSkipSplash_BF381B == 1) {
        ms_bSkipSplash_BF381B = 0;
        return;
    }
    if (ms_bLoadingScreenPaused) {
        return;
    }
    int32_t now = NowMs();
    if (now - ms_splashLastTime < 40 && re::Flag(fromLoading)) {
        return;
    }
    ms_splashLastTime = now;
    if (re::Flag(fromLoading)) {
        Camera_Fn_4F2090(TheCamera);
        if (ms_splashStartTime != 0 && uint32_t(now - int32_t(ms_splashStartTime)) < 1000) {
            return;
        }
    }
    float fade = ms_fadeAlpha;
    if (!(fade == 0.0f)) {
        FadeFrame(fade, re::Flag(fromLoading), true);
        return;
    }
    if (ms_loadingAlpha_C18708 >= 0xFF || re::Flag(fromLoading) || ms_bUserStart || ms_bFlag_20C32EA || Fn_404F40()) {
        return;
    }
    Loading_GetSplash(nullptr);
    uint8_t colour[4];
    colour[0] = 0;
    colour[1] = 0;
    colour[2] = 0;
    float level = MinF(255.0f, float(ms_loadingAlpha_C18708));
    level = MaxF(float(int32_t(int64_t(level))), 150.0f);
    int32_t loadAlpha = int32_t(int64_t(level));
    float value = MaxF(0.0f, float(0xFF - loadAlpha * 0xFF / 0xFF));
    colour[3] = uint8_t(int32_t(value));   // (truncated)
    Fn_409860();
    Fn_5EE1D0();
    Set2DRenderStates();
    SetSpriteTexture(nullptr);
    ms_spriteColour = *reinterpret_cast<uint32_t*>(colour);
    DrawScreenQuad(0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 0.0f);
}

// RenderSplash(1, 0) as 0x43CF20 has it inlined
void RenderSplashInlined(bool calls404F40) {
    if (ms_bSkipSplash_BF381B == 1) {
        ms_bSkipSplash_BF381B = 0;
        return;
    }
    if (ms_bLoadingScreenPaused) {
        return;
    }
    int32_t now = NowMs();
    if (now - ms_splashLastTime < 40) {
        return;
    }
    ms_splashLastTime = now;
    Camera_Fn_4F2090(TheCamera);
    if (ms_splashStartTime != 0 && uint32_t(now - int32_t(ms_splashStartTime)) < 1000) {
        return;
    }
    float fade = ms_fadeAlpha;
    if (!(fade == 0.0f)) {
        FadeFrame(fade, true, calls404F40);
    }
}

// 0x43C4F0: the loading screen (GTA's LoadingScreen): at most every 40 ms, the splash
// picture (`screen`: a new one), the progress circle once the progress is past 1, and
// "Loading"/`message` (their prints are empty stubs now) and the RARC_LOADING text
static void __cdecl DrawLoadingScreen(const char*, const char* message, const char* screen) {
    int32_t now = NowMs();
    if (now - ms_loadingLastTime < 40) {
        return;
    }
    ms_loadingLastTime = now;
    if (ms_bSplashOnly_BF3818) {
        Loading_RenderSplash(1, 0);
        return;
    }
    if (ms_bLoadingScreenNew && !screen) {
        return;
    }
    uint8_t* sprite = nullptr;
    if (screen) {
        ms_bLoadingScreenNew = 0;
    }
    if (const char* name = LoadingScreenName(screen)) {
        sprite = Loading_GetSplash(name);
    }
    Fn_4056D0();
    int32_t height = int32_t(int64_t(Screen_GetHeight()));
    int32_t width = int32_t(int64_t(Screen_GetWidth()));
    if (!App_BeginCameraUpdate(0, 0, 0, 0, 0, 0, 0xFF, 0, 0, 0, width, height, 1)) {
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
        Fn_424480(ms_loadingTextStyle);
        float y = float(double(ms_screenScaleY) * -50.0);
        ms_font_208B784 = 1;
        float x = float(double(ms_screenScaleX) * 70.0);
        float out[2];
        ScreenPosition(out, x, y, 0, 2);
        uint16_t loading[512];
        Text_CopyToWide(loading, Text_Get(TheText, "RARC_LOADING"), 0x200);
        Font_PrintString(out[0], out[1], loading, 1);
        if (message) {
            AsciiToWide(message, text);
            EmptyPrint(40.0f, float(double(Screen_GetHeight()) - 50.0), text);
        }
        if (ms_bFontLoaded) {
            void* renderer = ms_pRenderer;
            uint8_t pointer[4];
            int32_t target = *Fn_407BB0(pointer);
            bool changed = VCall<int32_t>(renderer, 0x78) != target;
            Pointer_Release(pointer);
            if (changed) {
                Fn_4096C0();
                Fn_409520(7, 1);
            }
            Font_Render();
        }
    }
    App_EndLoadingScreenFrame(1);
}

} // namespace LoadingScreen

RE_UNHOOKED("App::LoadingScreenName", 0x0043AD80);   // (the name in esi)
RE_UNHOOKED("App::DrawFade", 0x0043BA00);            // (the colour in edi)

namespace LoadingScreen {
// 0x8F1DE0 (exit): the fade-in colour's destructor (empty)
void __cdecl FadeInColour_Exit() {}
} // namespace LoadingScreen

// ---- registration ----------------------------------------------------------------------

#define LOADING_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace LoadingScreen {
RE_UNHOOKED_FN("LoadingScreen::FadeInColourExit", 0x008F1DE0, &FadeInColour_Exit);   // (a lone ret: no room for a hook)
LOADING_REVERSED("App::StartSplash", 0x0043BE40, &StartSplash);
LOADING_REVERSED("App::GetSplash", 0x0043BEE0, &GetSplash);
LOADING_REVERSED("App::RenderSplash", 0x0043C060, &RenderSplash);
LOADING_REVERSED("App::DrawLoadingScreen", 0x0043C4F0, &DrawLoadingScreen);
} // namespace LoadingScreen
