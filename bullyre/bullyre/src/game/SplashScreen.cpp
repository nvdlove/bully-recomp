// The script's splash screen (SplashScreenDisplay): a full-screen texture from its own
// dictionary TXD\<name>.NFT. 0x458140..0x4582B0.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_VAR(int32_t, splashTxd, 0x00A149B8);   // -1: none
GAME_VAR(void*, splashTexture, 0x00C192C0);

GAME_FN(void, __cdecl, Txd_RemoveRef, 0x005F1160, int slot);
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, void* object);
GAME_FN(void, __cdecl, Txd_RemoveTxd, 0x005F1830, int slot);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, Txd_LoadTxdFile, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
template <typename... A>
inline int __cdecl Sprintf(char* out, const char* format, A... rest) { return crt::Sprintf(out, format, rest...); }
GAME_FN(void, __cdecl, Fn_5EE1D0, 0x005EE1D0);   // (the 2D render states)
GAME_FN(void, __cdecl, RS_SetRenderState, 0x00519790, int state, int value);
GAME_FN(uint8_t*, __thiscall, CRGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawSprite, 0x0052C8F0, float x, float y, float width, float height, void* texture,
        const uint8_t* colour, const float* uv0, const float* uv1);

// ours, through the game
GAME_FN(void, __cdecl, SetRenderStatesCall, 0x00458180);

} // namespace

namespace splash {

// 0x458140: SplashScreenDisable: the texture and its dictionary let go
void __cdecl Disable() {
    if (splashTxd == -1) {
        return;
    }
    Txd_RemoveRef(splashTxd);
    NiRelease(splashTexture);
    splashTexture = nullptr;
    Txd_RemoveTxd(splashTxd);
    splashTxd = -1;
}

// 0x458180: the render states for it
void __cdecl SetRenderStates() {
    Fn_5EE1D0();
    RS_SetRenderState(6, 0);
    RS_SetRenderState(8, 0);
    RS_SetRenderState(10, 5);
    RS_SetRenderState(11, 6);
    RS_SetRenderState(12, 1);
    RS_SetRenderState(9, 2);
    RS_SetRenderState(2, 3);
}

// 0x4581D0: SplashScreenDisplay: the old one let go (as Disable, inlined), the
// dictionary loaded (a slot made for it if there's none) and the texture found in it
void __cdecl Display(const char* txd, const char* texture) {
    if (splashTxd != -1) {
        Txd_RemoveRef(splashTxd);
        NiRelease(splashTexture);
        splashTexture = nullptr;
        Txd_RemoveTxd(splashTxd);
        splashTxd = -1;
    }
    if (!txd || !texture) {
        return;
    }
    splashTxd = Txd_FindTxdSlot(txd, 0);
    if (splashTxd == -1) {
        splashTxd = Txd_AddTxdSlot(txd);
    }
    char path[256];
    Sprintf(path, "TXD/%s.NFT", txd);
    Txd_LoadTxdFile(splashTxd, path);
    Txd_AddRef(splashTxd);
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(splashTxd);
    splashTexture = Txd_FindTexture(texture, nullptr);
    Txd_PopCurrentTxd();
}

// 0x4582B0: drawn over the whole 640x480 screen, white
void __cdecl Render() {
    if (!splashTexture) {
        return;
    }
    SetRenderStatesCall();
    float uv1[2] = {1.0f, 1.0f};
    float uv0[2] = {0.0f, 0.0f};
    uint8_t colour[4];
    const uint8_t* rgba = CRGBA_Construct(colour, 0xFF, 0xFF, 0xFF, 0xFF);
    DrawSprite(0.0f, 0.0f, 640.0f, 480.0f, splashTexture, rgba, uv0, uv1);
}

} // namespace splash

// ---- registration ---------------------------------------------------------------------

#define C04D_SPLASH_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04D_SPLASH_REVERSED("SplashScreen::Disable", 0x00458140, &splash::Disable);
C04D_SPLASH_REVERSED("SplashScreen::SetRenderStates", 0x00458180, &splash::SetRenderStates);
C04D_SPLASH_REVERSED("SplashScreen::Display", 0x004581D0, &splash::Display);
C04D_SPLASH_REVERSED("SplashScreen::Render", 0x004582B0, &splash::Render);
