// The arcade cabinet minigames (MGArcade, vtable 0x948220): a 640x480 virtual screen of
// layers of sprites, all driven by the script (0x6FD060..0x700C70, with its MGArcade_
// natives; NativesC2.cpp has the few others). The script creates layers, adds sprites to
// them and sets their fields; each tick the sprites move (velocity, acceleration, spin,
// growth), the collision callbacks run for each pair that touches, then the sprite, layer
// and screen callbacks. Textures come from TXD\Arcade\<name>.nft, sounds from
// MINIGAME\<name>.bnk; sprites and layers refer to them by index.
//
// The screen sits at +0xC8 of the minigame; the "rastery" effect (+0xC4) draws a noise
// raster over everything. The x87 runs at 24-bit precision here (the main thread): float
// throughout.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/SchoolClasses.h"
#include "game/ScriptApi.h"
#include "game/TrackMath.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using school::At;

// ---- the structures ---------------------------------------------------------------------------

struct ArcadeSprite {
    float scaleX, scaleY;            // +0x00
    float width, height;             // +0x08 (16 x 16)
    float offsetX, offsetY;          // +0x10 the drawn rect from the position
    float x, y;                      // +0x18
    float velX, velY;                // +0x20
    float accX, accY;                // +0x28
    float collW, collH;              // +0x30 (collH 0: a circle, collW across)
    float rot;                       // +0x38 radians
    float rotSpeed;                  // +0x3C
    float rotAcc;                    // +0x40
    float scaleSpeedX, scaleSpeedY;  // +0x44
    uint8_t colour[4];               // +0x4C
    int32_t texture;                 // +0x50 the minigame's texture index (-1: a plain rect)
    uint8_t visible;                 // +0x54
    uint8_t flipX;                   // +0x55
    uint8_t flipY;                   // +0x56
    uint8_t additive;                // +0x57
    uint8_t unknown58;               // +0x58
    uint8_t unknown59;               // +0x59
    uint8_t unknown5A[2];
    uint32_t updateFunc;             // +0x5C script function references
    uint32_t unknown60;              // +0x60
    uint32_t unknown64;              // +0x64
    uint32_t collFunc;               // +0x68
    int32_t font;                    // +0x6C (a short is read)
    char text[0x10];                 // +0x70 a text key (empty: a sprite)
    int32_t paramCount;              // +0x80
    float params[2];                 // +0x84 the text's numbers
    int8_t scoreTable;               // +0x8C >= 0: shows a high score name's letters instead
    int8_t scoreSlot;                // +0x8D
    int8_t scoreChar;                // +0x8E (< 0: the whole name)
    uint8_t unknown8F;
};
static_assert(offsetof(ArcadeSprite, colour) == 0x4C);
static_assert(offsetof(ArcadeSprite, updateFunc) == 0x5C);
static_assert(offsetof(ArcadeSprite, collFunc) == 0x68);
static_assert(offsetof(ArcadeSprite, text) == 0x70);
static_assert(offsetof(ArcadeSprite, scoreTable) == 0x8C);
static_assert(sizeof(ArcadeSprite) == 0x90);

constexpr int kArcadeMaxSprites = 200;

struct ArcadeLayer {
    int32_t id;                        // +0x0
    re::Ptr32<ArcadeLayer> next;                 // +0x4
    ArcadeSprite sprites[kArcadeMaxSprites];   // +0x8
    int32_t spriteCount;               // +0x7088
    float scaleX, scaleY;              // +0x708C
    float width, height;               // +0x7094 (640 x 480)
    float offsetX, offsetY;            // +0x709C
    float x, y;                        // +0x70A4
    uint8_t colour[4];                 // +0x70AC
    int32_t texture;                   // +0x70B0
    float unknown70B4;                 // +0x70B4
    float unknown70B8;                 // +0x70B8
    re::Ptr32<void> unknown70BC;                 // +0x70BC (freed with the layer)
    uint32_t updateFunc;               // +0x70C0
};
static_assert(offsetof(ArcadeLayer, spriteCount) == 0x7088);
static_assert(offsetof(ArcadeLayer, colour) == 0x70AC);
static_assert(offsetof(ArcadeLayer, updateFunc) == 0x70C0);
static_assert(sizeof(ArcadeLayer) == 0x70C4);

// a line of text drawn this frame (MGArcade_DrawText)
struct ArcadeText {
    char key[0x10];      // +0x0
    float x, y;          // +0x10
    int32_t paramCount;  // +0x18
    float params[2];     // +0x1C
};
static_assert(sizeof(ArcadeText) == 0x24);

struct ArcadeScreen {
    float scaleX, scaleY;          // +0x00
    float width, height;           // +0x08 (640 x 480)
    float offsetX, offsetY;        // +0x10 (-320, -240)
    float x, y;                    // +0x18
    uint8_t background[4];         // +0x20
    uint8_t colour[4];             // +0x24 the screen texture's
    int32_t texture;               // +0x28
    uint8_t borderColour[4];       // +0x2C the side borders'
    int32_t leftTexture;           // +0x30
    int32_t rightTexture;          // +0x34
    uint8_t fitScreen;             // +0x38 scale the size to 640x480
    uint8_t noWidescreen;          // +0x39
    uint8_t unknown3A[2];
    re::Ptr32<ArcadeLayer> layers;           // +0x3C
    float centreX, centreY;        // +0x40 (ComputeRect's results from here)
    float drawScaleX, drawScaleY;  // +0x48
    float drawWidth, drawHeight;   // +0x50
    float left, top;               // +0x58
    float right, bottom;           // +0x60
    uint32_t updateFunc;           // +0x68
    int32_t font;                  // +0x6C (a short is read)
    float fontScale;               // +0x70
    int32_t textCount;             // +0x74
    ArcadeText texts[32];          // +0x78
};
static_assert(offsetof(ArcadeScreen, layers) == 0x3C);
static_assert(offsetof(ArcadeScreen, updateFunc) == 0x68);
static_assert(sizeof(ArcadeScreen) == 0x4F8);

// the minigame's own fields (the Minigame base before +0xC4)
constexpr int kArcadeRastery = 0xC4;
constexpr int kArcadeCancelConfirm = 0xC5;
constexpr int kArcadeScreen = 0xC8;
constexpr int kArcadeTexCount = 0x5C0;
constexpr int kArcadeTxdSlot = 0x5C4;
constexpr int kArcadeTexCapacity = 0x5C8;
constexpr int kArcadeTextures = 0x5CC;
constexpr int kArcadeSoundCount = 0x5D0;
constexpr int kArcadeSoundBank = 0x5D4;
constexpr int kArcadeSoundCapacity = 0x5D8;
constexpr int kArcadeSounds = 0x5DC;

// ---- the game's state and calls -------------------------------------------------------------

GAME_VAR(uint8_t*, arcCurrentMinigame, 0x020C8704);
GAME_VAR(void*, arcRaster, 0x020C8394);              // the rastery effect's noise
GAME_VAR(void*, arcIm2DRaster, 0x020DCCC8);
GAME_ARRAY(uint32_t, arcIm2DColours, 0x020DCCF4);    // [4]
GAME_VAR(uint8_t*, arcAudioPausedFlag, 0x00CF4374);
GAME_VAR(uint8_t*, arcAmbiences, 0x00CF4278);
GAME_VAR(float, arcTickRate, 0x00BC68CC);
GAME_VAR(uint8_t, arcWidescreen, 0x00C1870E);
GAME_VAR(int32_t, arcLanguage, 0x00C18724);
GAME_VAR(float, arcScreenScaleX, 0x00A957A8);
GAME_VAR(float, arcScreenScaleY, 0x00A957AC);
GAME_VAR(float, arcTimeStep, 0x00C1A9A4);
GAME_VAR(int32_t, arcUserStartResponse, 0x020C87AC);
inline void* const arcSoundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const arcSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const arcLurStore = reinterpret_cast<void*>(0x00D02850);
inline void* const arcTheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_ARRAY(uint8_t, arcFontColour, 0x0208B764);
GAME_VAR(int16_t, arcFontShadow, 0x0208B784);
GAME_VAR(int16_t, arcFontStyle, 0x0208B788);
GAME_ARRAY(void*, arcFontStyleTexture, 0x0208B218);

GAME_FN(void*, __cdecl, Arc_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Arc_Free, 0x005EEAB0, void* p);
inline int __cdecl Arc_Rand() { return crt::Rand(); }
inline char* __cdecl Arc_Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
template <typename... A>
inline int __cdecl Arc_Snprintf(char* buffer, size_t size, const char* format, A... rest) { return crt::Snprintf(buffer, size, format, rest...); }
inline void* __cdecl Arc_Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(void, __cdecl, Arc_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, Arc_PopMemoryTag, 0x005EF100);
GAME_FN(void, __cdecl, Arc_PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, Arc_PopTxdMemoryTag, 0x0052E710);
GAME_FN(float, __cdecl, Arc_LimitAngle, 0x00432EE0, float angle);
GAME_FN(void, __thiscall, Arc_Vector2D_Normalise, 0x00443730, float* v);
// raster and render states (the RW emulation)
GAME_FN(void*, __cdecl, Arc_RasterCreate, 0x00519F60, int width, int height, int depth, int flags);
GAME_FN(uint8_t*, __cdecl, Arc_RasterLock, 0x0051A250, void* raster, int level, int mode);
GAME_FN(void, __cdecl, Arc_RasterUnlock, 0x0051A290, void* raster);
GAME_FN(void, __cdecl, Arc_RasterDestroy, 0x00519090, void* raster);
GAME_FN(void, __cdecl, Arc_SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, Arc_Im2DSetUVs, 0x0052AC90, float, float, float, float, float, float, float, float);
GAME_FN(void, __cdecl, Arc_Im2DDrawQuad, 0x00739820, float, float, float, float, float, float, float, float, float,
        float, float, float);
// 2D drawing (rect: left, bottom, right, top)
GAME_FN(void, __cdecl, Arc_DrawRect, 0x0052C470, const float* rect, const uint8_t* colour);
GAME_FN(void, __cdecl, Arc_DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, Arc_DrawQuad, 0x0052C270, float x1, float y1, float x2, float y2, float x3, float y3, float x4,
        float y4, const uint8_t* colour);
GAME_FN(void, __cdecl, Arc_DrawQuadUV, 0x0052CCC0, float x1, float y1, float x2, float y2, float x3, float y3,
        float x4, float y4, void* texture, const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(uint8_t*, __thiscall, Arc_RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
// the font
GAME_FN(void, __cdecl, Arc_Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, Arc_Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Arc_Font_SetScaleXY, 0x006880B0, float x, float y);
GAME_FN(void, __cdecl, Arc_Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Arc_Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(void, __cdecl, Arc_Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Arc_Font_Render, 0x0068CDF0);
GAME_FN(const uint16_t*, __thiscall, Arc_Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Arc_GetHighScore, 0x007055A0, int table, int slot, int32_t* score, re::Ptr32<uint16_t>* name);
// txds and sounds
GAME_FN(void, __cdecl, Arc_NiRelease, 0x005F0F90, void* object);
GAME_FN(void, __cdecl, Arc_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(int, __cdecl, Arc_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, Arc_LoadTxdFile, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Arc_TxdAddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Arc_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Arc_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Arc_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, Arc_PopCurrentTxd, 0x005F0AA0);
GAME_FN(int32_t, __thiscall, Arc_SoundBanks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(void, __thiscall, Arc_SoundBanks_Load, 0x0059A3E0, void* banks, int32_t bank, int, int);
GAME_FN(void, __thiscall, Arc_SoundBanks_Unload, 0x0059A420, void* banks, int32_t bank, int, int);
GAME_FN(int32_t, __thiscall, Arc_Sounds_GetId, 0x00595B80, void* sounds, const char* name);
// the pad, the Minigame base's popup and sounds
GAME_FN(uint8_t, __cdecl, Arc_Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(int, __thiscall, Arc_Popup_Result, 0x00705080, void* popup);
GAME_FN(void, __thiscall, Arc_Popup_Show, 0x00705220, void* popup, int kind, const char* title, const char* buttons,
        int, int);
GAME_FN(void, __thiscall, Arc_Popup_Draw, 0x00706000, void* popup);
GAME_FN(void, __thiscall, Arc_Minigame_EndSounds, 0x00704DE0, void* mg);
GAME_FN(void, __thiscall, Arc_Minigame_EndTextures, 0x00704FD0, void* mg);
GAME_FN(void, __thiscall, Arc_Minigame_Construct, 0x007059D0, void* mg);
GAME_FN(void, __thiscall, Arc_Minigame_Destruct, 0x00705A50, void* mg);
// script function references and the Lua calls
GAME_FN(uint8_t, __thiscall, Arc_Ref_IsSet, 0x005D80E0, uint32_t* ref);
GAME_FN(lua_State*, __thiscall, Arc_Ref_GetState, 0x005D9C20, uint32_t* ref);
GAME_FN(int32_t, __thiscall, Arc_Ref_GetId, 0x005D80D0, uint32_t* ref);
GAME_FN(int32_t, __thiscall, Arc_Ref_GetScript, 0x005D80B0, uint32_t* ref);
GAME_FN(int32_t, __thiscall, Arc_Ref_GetThread, 0x005D80C0, uint32_t* ref);
GAME_FN(void, __thiscall, Arc_Ref_Clear, 0x005D9C50, uint32_t* ref);
GAME_FN(void*, __thiscall, Arc_Lur_GetCurrentOrNull, 0x005DB960, void* store);
GAME_FN(void*, __thiscall, Arc_Lur_FindScript, 0x005DB8F0, void* store, int32_t script);
GAME_FN(void, __thiscall, Arc_Lur_SetCurrent, 0x005DBDA0, void* store, void* script);
GAME_FN(int32_t, __thiscall, Arc_Script_GetThread, 0x005D8BC0, void* script);
GAME_FN(void, __thiscall, Arc_Script_SetThread, 0x005D8BF0, void* script, int32_t thread);
GAME_FN(void, __cdecl, Arc_Event_Begin, 0x005D8270, lua_State* L);
GAME_FN(void, __cdecl, Arc_Lua_GetRef, 0x005D8670, lua_State* L, int ref);
GAME_FN(void, __cdecl, Arc_Lua_Call, 0x005D82A0, lua_State* L, int args, int results);
// (this file's)
GAME_FN(void, __thiscall, Arc_Screen_ComputeRect, 0x006FD530, ArcadeScreen* screen);
GAME_FN(void, __thiscall, Arc_Sprite_Update, 0x006FD670, ArcadeSprite* sprite, float dt, int32_t layer);
GAME_FN(void, __thiscall, Arc_Sprite_Draw, 0x006FD700, ArcadeSprite* sprite, float x, float y, float sx, float sy);
GAME_FN(uint8_t, __thiscall, Arc_Sprite_Overlaps, 0x006FDE50, ArcadeSprite* sprite, ArcadeSprite* other);
GAME_FN(ArcadeScreen*, __thiscall, Arc_Screen_Construct, 0x006FE370, ArcadeScreen* screen);
GAME_FN(void, __thiscall, Arc_Screen_Destruct, 0x006FE470, ArcadeScreen* screen);
GAME_FN(void, __thiscall, Arc_Layer_Update, 0x006FE570, ArcadeLayer* layer, float dt);
GAME_FN(void, __thiscall, Arc_Layer_Draw, 0x006FE8C0, ArcadeLayer* layer, float x, float y, float sx, float sy);
GAME_FN(void, __thiscall, Arc_Layer_Destruct, 0x006FD3B0, ArcadeLayer* layer);
GAME_FN(void, __thiscall, Arc_Screen_Update, 0x006FEB30, ArcadeScreen* screen);
GAME_FN(void, __thiscall, Arc_Screen_Draw, 0x006FEBC0, ArcadeScreen* screen);
GAME_FN(void, __thiscall, Arc_UnloadTextures, 0x006FD0E0, void* mg);
GAME_FN(void, __thiscall, Arc_UnloadSounds, 0x006FD300, void* mg);
GAME_FN(int, __thiscall, Arc_FindTextureIndex, 0x006FD170, void* mg, const char* name, const char* mask);
GAME_FN(int32_t, __thiscall, Arc_Screen_AddLayer, 0x006FF230, ArcadeScreen* screen, ArcadeLayer* layer);
GAME_FN(void, __thiscall, Arc_Screen_AddText, 0x006FF290, ArcadeScreen* screen, float x, float y, const char* key,
        float p1, float p2);
GAME_FN(void, __thiscall, Arc_Destruct, 0x006FE510, void* mg);
GAME_FN(ArcadeSprite*, __thiscall, Arc_Sprite_Construct, 0x007005C0, ArcadeSprite* sprite);
GAME_FN(ArcadeLayer*, __thiscall, Arc_Layer_Construct, 0x00700670, ArcadeLayer* layer);

inline ArcadeScreen* ScreenOf(uint8_t* mg) {
    return reinterpret_cast<ArcadeScreen*>(mg + kArcadeScreen);
}
inline void* ArcTexture(int index) {
    return At<re::Ptr32<void>*>(arcCurrentMinigame, kArcadeTextures)[index];
}
// the running minigame's layer of that id (nullptr for none: the natives then write
// through it regardless, as the game does)
ArcadeLayer* FindLayer(int id) {
    ArcadeLayer* layer = ScreenOf(arcCurrentMinigame)->layers;
    while (layer != nullptr && layer->id != id) {
        layer = layer->next;
    }
    return layer;
}
inline ArcadeSprite& SpriteOf(ArcadeLayer* layer, int index) {
    return *reinterpret_cast<ArcadeSprite*>(reinterpret_cast<uint8_t*>(layer) + 8 + index * 0x90);
}
// a sprite's or the screen's font set up the way the game does (the shadow off twice)
void ArcSetFont(int32_t font) {
    Arc_Font_Reset();
    arcFontShadow = 0;
    int16_t style = int16_t(font);
    if (arcFontStyleTexture[style] != nullptr) {
        arcFontStyle = style;
        if (style != 0) {
            arcFontShadow = 0;
        }
    }
}

// a script function reference called inside its own script (its thread made current
// for the call); the pushes come from `push`
template <typename Push>
void CallRefInItsScript(uint32_t* ref, int args, Push push) {
    void* current = Arc_Lur_GetCurrentOrNull(arcLurStore);
    void* script = Arc_Lur_FindScript(arcLurStore, Arc_Ref_GetScript(ref));
    if (script == nullptr) {
        return;
    }
    int32_t saved = current != nullptr ? Arc_Script_GetThread(current) : -1;
    Arc_Script_SetThread(script, Arc_Ref_GetThread(ref));
    Arc_Lur_SetCurrent(arcLurStore, script);
    lua_State* L = Arc_Ref_GetState(ref);
    Arc_Event_Begin(L);
    Arc_Lua_GetRef(L, Arc_Ref_GetId(ref));
    push(L);
    Arc_Lua_Call(L, args, 0);
    Arc_Script_SetThread(script, saved);
    Arc_Lur_SetCurrent(arcLurStore, current);
}

// ---- the minigame -----------------------------------------------------------------------------

// 0x6FD060 (virtual 4): starts: the noise raster (64x64, random grey)
void __fastcall ArcStart(uint8_t* mg, re::Edx) {
    mg[4] = 1;
    mg[5] = 0;
    mg[kArcadeCancelConfirm] = 1;
    *arcAudioPausedFlag = 1;
    arcAmbiences[0xA8] = 1;
    arcRaster = Arc_RasterCreate(0x40, 0x40, 0x20, 0x504);
    uint8_t* pixel = Arc_RasterLock(arcRaster, 0, 1);
    for (int i = 0; i < 0x1000; i++) {
        uint8_t v = uint8_t(Arc_Rand());
        pixel[3] = v;
        pixel[2] = v;
        pixel[1] = v;
        pixel[0] = v;
        pixel += 4;
    }
    Arc_RasterUnlock(arcRaster);
}

// 0x6FD0E0: releases the textures and the txd
void __fastcall ArcUnloadTextures(uint8_t* mg, re::Edx) {
    auto& textures = At<re::Ptr32<void>*>(mg, kArcadeTextures);
    if (textures == nullptr) {
        return;
    }
    for (int i = 0; i < At<int32_t>(mg, kArcadeTexCount); i++) {
        Arc_NiRelease(At<re::Ptr32<void>*>(mg, kArcadeTextures)[i]);
    }
    Arc_Free(textures);
    int32_t slot = At<int32_t>(mg, kArcadeTxdSlot);
    if (slot != -1) {
        Arc_RemoveTxdSlot(slot);
    }
    textures = nullptr;
    At<int32_t>(mg, kArcadeTexCount) = 0;
    At<int32_t>(mg, kArcadeTxdSlot) = -1;
}

// 0x6FD170: a texture of the arcade's txd added to the table: its index (-1: none; no
// capacity check)
int __fastcall ArcFindTexture(uint8_t* mg, re::Edx, const char* name, const char* mask) {
    if (At<re::Ptr32<void>*>(mg, kArcadeTextures) == nullptr || At<int32_t>(mg, kArcadeTxdSlot) == -1) {
        return -1;
    }
    Arc_PushCurrentTxd();
    Arc_SetCurrentTxd(At<int32_t>(mg, kArcadeTxdSlot));
    void* texture = Arc_FindTexture(name, mask);
    Arc_PopCurrentTxd();
    if (texture == nullptr) {
        return -1;
    }
    int32_t index = At<int32_t>(mg, kArcadeTexCount);
    At<int32_t>(mg, kArcadeTexCount) = index + 1;
    At<re::Ptr32<void>*>(mg, kArcadeTextures)[index] = texture;
    return index;
}

// 0x6FD1E0: loads the sound bank MINIGAME\<name>.bnk (a 256-byte buffer, unchecked)
void __fastcall ArcLoadSounds(uint8_t* mg, re::Edx, const char* name) {
    char path[0x100];
    std::memcpy(path, school::GameString(0x009481E8), 10);   // "MINIGAME\"
    Arc_Memset(path + 10, 0, 0xF6);
    std::strcat(path, name);
    std::strcat(path, school::GameString(0x009481E0));   // ".bnk"
    int32_t bank = Arc_SoundBanks_Find(arcSoundBanks, path);
    At<int32_t>(mg, kArcadeSoundBank) = bank;
    Arc_SoundBanks_Load(arcSoundBanks, bank, 1, 1);
    At<int32_t>(mg, kArcadeSoundCapacity) = 200;
    int32_t* ids = static_cast<int32_t*>(Arc_Malloc(800));
    At<int32_t*>(mg, kArcadeSounds) = ids;
    if (ids == nullptr) {
        At<int32_t>(mg, kArcadeSoundBank) = 0;
        return;
    }
    At<int32_t>(mg, kArcadeSoundCount) = 0;
}

// 0x6FD300: unloads the sound bank
void __fastcall ArcUnloadSounds(uint8_t* mg, re::Edx) {
    if (At<int32_t*>(mg, kArcadeSounds) == nullptr) {
        return;
    }
    Arc_SoundBanks_Unload(arcSoundBanks, At<int32_t>(mg, kArcadeSoundBank), 1, 1);
    Arc_Free(At<int32_t*>(mg, kArcadeSounds));
    At<int32_t*>(mg, kArcadeSounds) = nullptr;
    At<int32_t>(mg, kArcadeSoundCount) = 0;
    At<int32_t>(mg, kArcadeSoundBank) = 0;
}

// 0x6FD350: a sound's index in the table (added when new; no capacity check; -1 for no name)
int __fastcall ArcGetSoundId(uint8_t* mg, re::Edx, const char* name) {
    if (name == nullptr) {
        return -1;
    }
    int32_t id = Arc_Sounds_GetId(arcSounds, name);
    int32_t count = At<int32_t>(mg, kArcadeSoundCount);
    int32_t* ids = At<int32_t*>(mg, kArcadeSounds);
    for (int i = 0; i < count; i++) {
        if (id == ids[i]) {
            return i;
        }
    }
    At<int32_t>(mg, kArcadeSoundCount) = count + 1;
    At<int32_t*>(mg, kArcadeSounds)[count] = id;
    return count;
}

// 0x6FD3B0: the layer's destructor (its sprites' references from the last)
void __fastcall ArcLayerDestruct(ArcadeLayer* layer, re::Edx) {
    if (layer->unknown70BC != nullptr) {
        Arc_Free(layer->unknown70BC);
    }
    Arc_Ref_Clear(&layer->updateFunc);
    for (int i = kArcadeMaxSprites - 1; i >= 0; i--) {
        ArcadeSprite& s = layer->sprites[i];
        Arc_Ref_Clear(&s.collFunc);
        Arc_Ref_Clear(&s.unknown64);
        Arc_Ref_Clear(&s.unknown60);
        Arc_Ref_Clear(&s.updateFunc);
    }
}

// the end shared by 0x6FD410 and the destructor (the noise raster, the pause flags, the
// textures and sounds)
void ArcEndCommon(uint8_t* mg) {
    if (arcRaster != nullptr) {
        Arc_RasterDestroy(arcRaster);
    }
    *arcAudioPausedFlag = 0;
    arcAmbiences[0xA8] = 0;
    arcRaster = nullptr;
    mg[4] = 0;
    mg[5] = 0;
    Arc_UnloadTextures(mg);
    Arc_UnloadSounds(mg);
}

// 0x6FD410 (virtual 5): ends
void __fastcall ArcEnd(uint8_t* mg, re::Edx) {
    ArcEndCommon(mg);
    Arc_Minigame_EndSounds(mg);
    Arc_Minigame_EndTextures(mg);
}

// 0x6FD470: loads TXD\Arcade\<name>.nft into a new "agscrtex" slot, and a 200-texture table
void __fastcall ArcLoadTextures(uint8_t* mg, re::Edx, const char* name) {
    char path[0x80];
    Arc_PushMemoryTag(8);
    Arc_Snprintf(path, 0x80, school::GameString(0x00948200), name);   // "TXD\Arcade\%s.nft"
    int slot = Arc_AddTxdSlot(school::GameString(0x009481F4));       // "agscrtex"
    At<int32_t>(mg, kArcadeTxdSlot) = slot;
    Arc_LoadTxdFile(slot, path);
    Arc_TxdAddRef(At<int32_t>(mg, kArcadeTxdSlot));
    At<int32_t>(mg, kArcadeTexCapacity) = 200;
    re::Ptr32<void>* textures = static_cast<re::Ptr32<void>*>(Arc_Malloc(800));
    At<re::Ptr32<void>*>(mg, kArcadeTextures) = textures;
    if (textures == nullptr) {
        if (At<int32_t>(mg, kArcadeTxdSlot) != -1) {
            Arc_RemoveTxdSlot(At<int32_t>(mg, kArcadeTxdSlot));
        }
        At<int32_t>(mg, kArcadeTxdSlot) = -1;
    }
    At<int32_t>(mg, kArcadeTexCount) = 0;
    Arc_PopMemoryTag();
}

// 0x6FD530: the screen's rect on the real screen (fitted to 640x480 when asked, x scaled
// for widescreen; only y follows screenScaleY)
void __fastcall ArcScreenComputeRect(ArcadeScreen* s, re::Edx) {
    float sx = 1.0f;
    float sy = 1.0f;
    if (s->fitScreen != 0) {
        sx = 640.0f / s->width;
        sy = 480.0f / s->height;
    }
    if (arcWidescreen != 0 && s->noWidescreen == 0) {
        sx = arcScreenScaleX * sx;
    }
    sx = s->scaleX * sx;
    sy = s->scaleY * sy;
    float left = s->offsetX * sx + (s->x + 320.0f);
    float top = s->offsetY * sy + (s->y * arcScreenScaleY + 240.0f);
    float w = s->width * sx;
    float h = s->height * sy;
    float cx = w * 0.5f + left;
    float cy = h * 0.5f + top;
    s->centreX = cx;
    s->centreY = cy;
    s->drawScaleX = sx;
    s->drawScaleY = sy;
    s->drawWidth = w;
    s->drawHeight = h;
    s->left = left;
    s->right = w + left;
    s->bottom = h + top;
    s->top = top;
}

// 0x6FD670: a tick of a sprite's motion (the layer id argument unused)
void __fastcall ArcSpriteUpdate(ArcadeSprite* s, re::Edx, float dt, int32_t) {
    float ax = dt * s->accX;
    float ay = s->accY * dt;
    s->velX = s->velX + ax;
    s->velY = ay + s->velY;
    float dx = s->velX * dt;
    float dy = s->velY * dt;
    s->x = s->x + dx;
    s->y = s->y + dy;
    float spin = s->rotAcc * dt + s->rotSpeed;
    s->rotSpeed = spin;
    s->rot = spin * dt + s->rot;
    float gx = s->scaleSpeedX * dt;
    float gy = dt * s->scaleSpeedY;
    s->scaleX = s->scaleX + gx;
    s->scaleY = gy + s->scaleY;
}

// the texture UVs of a flipped sprite
inline void FlipUVs(const ArcadeSprite* s, float* uv1, float* uv2) {
    uv2[0] = s->flipX != 0 ? 0.0f : 1.0f;
    uv2[1] = s->flipY != 0 ? 0.0f : 1.0f;
    uv1[0] = s->flipX != 0 ? 1.0f : 0.0f;
    uv1[1] = s->flipY != 0 ? 1.0f : 0.0f;
}

// 0x6FD700: draws a sprite at (x, y) + its position scaled: its text, or its texture (a
// plain rect without one), rotated when it has an angle. An upright one draws when any of
// it is on the screen, a rotated one only when all of it is.
void __fastcall ArcSpriteDraw(ArcadeSprite* s, re::Edx, float x, float y, float sx, float sy) {
    if (s->visible == 0) {
        return;
    }
    if (s->additive != 0) {
        Arc_SetRenderState(10, 2);
        Arc_SetRenderState(11, 2);
    } else {
        Arc_SetRenderState(10, 5);
        Arc_SetRenderState(11, 6);
    }
    float px = s->x * sx;
    float py = s->y * sy;
    if (s->text[0] == 0 && s->scoreTable < 0) {
        if (s->rot == 0.0f) {
            float ssx = sx * s->scaleX;
            float ssy = sy * s->scaleY;
            px = s->offsetX * ssx + px;
            py = s->offsetY * ssy + py;
            px = x + px;
            py = y + py;
            float rect[4];
            rect[0] = px;
            rect[3] = py;
            float right = ssx * s->width + px;
            float bottom = ssy * s->height + py;
            rect[2] = right;
            rect[1] = bottom;
            if (!(640.0f < px) && !(right < 0.0f) && !(py > 480.0f) && !(bottom < 0.0f)) {
                if (s->texture != -1) {
                    float uv1[2];
                    float uv2[2];
                    FlipUVs(s, uv1, uv2);
                    Arc_DrawRectUV(px, py, right, bottom, ArcTexture(s->texture), s->colour, uv1, uv2);
                } else {
                    Arc_DrawRect(rect, s->colour);
                }
            }
        } else {
            float maxY = -3.4028235e+38f;
            float minY = 3.4028235e+38f;
            float minX = 3.4028235e+38f;
            float maxX = -3.4028235e+38f;
            float corner[8];
            corner[0] = s->offsetX * s->scaleX;
            corner[1] = s->offsetY * s->scaleY;
            float w = s->width * s->scaleX;
            corner[2] = w + corner[0];
            corner[3] = corner[1];
            corner[4] = corner[0];
            corner[5] = s->height * s->scaleY + corner[1];
            corner[6] = corner[2];
            corner[7] = corner[5];
            float sinA = trk::CrtSin(s->rot);
            float cosA = trk::CrtCos(s->rot);
            float out[8];
            for (int i = 0; i < 8; i += 2) {
                float rx = corner[i] * cosA - sinA * corner[i + 1];
                out[i] = rx * sx;
                float ry = corner[i] * sinA + cosA * corner[i + 1];
                out[i + 1] = ry * sy;
                out[i] = out[i] + px;
                out[i + 1] = out[i + 1] + py;
                out[i] = out[i] + x;
                out[i + 1] = out[i + 1] + y;
                if (maxX < out[i]) {
                    maxX = out[i];
                }
                if (minX > out[i]) {
                    minX = out[i];
                }
                if (maxY < out[i + 1]) {
                    maxY = out[i + 1];
                }
                if (minY > out[i + 1]) {
                    minY = out[i + 1];
                }
            }
            if (!(640.0f < maxX) && !(0.0f > minX) && !(maxY > 480.0f) && !(0.0f > minY)) {
                if (s->texture != -1) {
                    float uv1[2];
                    float uv2[2];
                    FlipUVs(s, uv1, uv2);
                    Arc_DrawQuadUV(out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
                                   ArcTexture(s->texture), s->colour, uv1, uv2);
                } else {
                    Arc_DrawQuad(out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], s->colour);
                }
            }
        }
    } else {
        uint16_t buffer[0x80];
        re::Ptr32<uint16_t> text = buffer;
        float ssx = sx * s->scaleX;
        float ssy = sy * s->scaleY;
        px = s->offsetX * ssx + px;
        py = s->offsetY * ssy + py;
        px = x + px;
        py = y + py;
        ArcSetFont(s->font);
        if (arcLanguage == 7 || arcLanguage == 6) {
            Arc_Font_SetSizePreset(2);
        } else {
            float scale = s->scaleX * sx;
            Arc_Font_SetScaleXY(scale, scale);
        }
        arcFontColour[0] = s->colour[0];
        arcFontColour[1] = s->colour[1];
        arcFontColour[2] = s->colour[2];
        arcFontColour[3] = s->colour[3];
        if (s->scoreTable >= 0) {
            Arc_GetHighScore(s->scoreTable, s->scoreSlot, nullptr, &text);
            if (s->scoreChar >= 0) {
                buffer[0] = text[s->scoreChar];
                buffer[1] = 0;
                text = buffer;
            }
        } else {
            for (int i = 0; i < s->paramCount; i++) {
                Arc_Font_AddNumber(s->params[i]);
            }
            Arc_Font_Format(buffer, Arc_Text_Get(arcTheText, s->text), 0x80);
        }
        Arc_Font_PrintString(px, py, text, 1);
        Arc_Font_Render();
    }
    Arc_SetRenderState(10, 5);
    Arc_SetRenderState(11, 6);
}

// 0x6FDE50: two sprites' collision shapes touch: circles when both have no collision
// height (across collW times the larger scale), else boxes (a circle-shaped one boxed)
uint8_t __fastcall ArcSpriteOverlaps(ArcadeSprite* a, re::Edx, ArcadeSprite* b) {
    float aCollW = a->collW;
    float aCollH = a->collH;
    float bCollW = b->collW;
    float bCollH = b->collH;
    if (aCollH == 0.0f && bCollH == 0.0f) {
        float aw = a->width * a->scaleX;
        float ah = a->height * a->scaleY;
        float aHalfW = aw * 0.5f;
        float aHalfH = ah * 0.5f;
        float aox = a->offsetX * a->scaleX;
        float aoy = a->offsetY * a->scaleY;
        float aLeft = aox + a->x;
        float aTop = a->y + aoy;
        float acx = aLeft + aHalfW;
        float acy = aTop + aHalfH;
        float bw = b->width * b->scaleX;
        float bh = b->height * b->scaleY;
        float bHalfW = bw * 0.5f;
        float bHalfH = bh * 0.5f;
        float box = b->offsetX * b->scaleX;
        float boy = b->offsetY * b->scaleY;
        float bLeft = box + b->x;
        float bTop = boy + b->y;
        float bcx = bLeft + bHalfW;
        float bcy = bTop + bHalfH;
        float aScale = a->scaleX > a->scaleY ? a->scaleX : a->scaleY;
        float aRadius = aCollW * aScale * 0.5f;
        float bScale = b->scaleX > b->scaleY ? b->scaleX : b->scaleY;
        float bRadius = bCollW * bScale * 0.5f;
        float dx = acx - bcx;
        float dy = acy - bcy;
        float reach = bRadius + aRadius;
        float distance2 = dx * dx + dy * dy;
        float reach2 = reach * reach;
        return !(reach2 < distance2) ? 1 : 0;
    }
    float aw = a->width * a->scaleX;
    float ah = a->height * a->scaleY;
    float aHalfW = aw * 0.5f;
    float aHalfH = ah * 0.5f;
    float aox = a->offsetX * a->scaleX;
    float aoy = a->offsetY * a->scaleY;
    float aLeft = aox + a->x;
    float aTop = aoy + a->y;
    float acx = aLeft + aHalfW;
    float acy = aTop + aHalfH;
    if (aCollH == 0.0f) {
        float scale = a->scaleY < a->scaleX ? a->scaleX : a->scaleY;
        aCollW = scale * aCollW;
        aCollH = aCollW;
    } else {
        aCollW = a->scaleX * aCollW;
        aCollH = aCollH * a->scaleY;
    }
    float aBoxHalfW = aCollW * 0.5f;
    float aBoxHalfH = aCollH * 0.5f;
    float aMinX = acx - aBoxHalfW;
    float aMinY = acy - aBoxHalfH;
    float aMaxX = acx + aBoxHalfW;
    float aMaxY = acy + aBoxHalfH;
    float bw = b->width * b->scaleX;
    float bh = b->height * b->scaleY;
    float bHalfW = bw * 0.5f;
    float bHalfH = bh * 0.5f;
    float box = b->offsetX * b->scaleX;
    float boy = b->offsetY * b->scaleY;
    float bLeft = box + b->x;
    float bTop = boy + b->y;
    float bcx = bLeft + bHalfW;
    float bcy = bTop + bHalfH;
    if (bCollH == 0.0f) {
        float scale = b->scaleX > b->scaleY ? b->scaleX : b->scaleY;
        bCollW = scale * bCollW;
        bCollH = bCollW;
    } else {
        bCollW = b->scaleX * bCollW;
        bCollH = bCollH * b->scaleY;
    }
    float bBoxHalfW = bCollW * 0.5f;
    float bBoxHalfH = bCollH * 0.5f;
    float bMinX = bcx - bBoxHalfW;
    float bMinY = bcy - bBoxHalfH;
    float bMaxX = bcx + bBoxHalfW;
    float bMaxY = bcy + bBoxHalfH;
    if (bMinX > aMaxX || bMinY > aMaxY || aMinX > bMaxX || aMinY > bMaxY) {
        return 0;
    }
    return 1;
}

// 0x6FE370: the screen's constructor (and its rect)
ArcadeScreen* __fastcall ArcScreenConstruct(ArcadeScreen* s, re::Edx) {
    s->scaleX = 1.0f;
    s->scaleY = 1.0f;
    s->width = 640.0f;
    s->height = 480.0f;
    s->offsetX = -320.0f;
    s->offsetY = -240.0f;
    s->x = 0.0f;
    s->y = 0.0f;
    Arc_RGBA_Set(s->background, 0, 0, 0, 0xFF);
    Arc_RGBA_Set(s->colour, 0x80, 0x80, 0x80, 0xFF);
    s->texture = -1;
    Arc_RGBA_Set(s->borderColour, 0x80, 0x80, 0x80, 0xFF);
    s->leftTexture = -1;
    s->rightTexture = -1;
    s->fitScreen = 1;
    s->noWidescreen = 0;
    s->layers = nullptr;
    s->left = 1000000.0f;
    s->bottom = 1000000.0f;
    s->updateFunc = 0;
    s->right = -1000000.0f;
    s->top = -1000000.0f;
    // (the texts' array constructed, then cleared)
    for (int i = 0; i < 32; i++) {
        s->texts[i].key[0] = 0;
        s->texts[i].paramCount = 0;
    }
    s->font = 0;
    s->fontScale = 1.0f;
    for (int i = 0; i < 32; i++) {
        s->texts[i].key[0] = 0;
        s->texts[i].paramCount = 0;
    }
    s->textCount = 0;
    Arc_Screen_ComputeRect(s);
    return s;
}

// 0x6FE470: the screen's destructor (its layers freed)
void __fastcall ArcScreenDestruct(ArcadeScreen* s, re::Edx) {
    ArcadeLayer* layer = s->layers;
    while (layer != nullptr) {
        ArcadeLayer* next = layer->next;
        Arc_Layer_Destruct(layer);
        Arc_Free(layer);
        layer = next;
    }
    Arc_Ref_Clear(&s->updateFunc);
}

// 0x6FE4B0: the constructor
uint8_t* __fastcall ArcConstruct(uint8_t* mg, re::Edx) {
    Arc_Minigame_Construct(mg);
    At<uint32_t>(mg, 0) = 0x00948220;
    Arc_Screen_Construct(ScreenOf(mg));
    mg[kArcadeRastery] = 0;
    At<int32_t>(mg, kArcadeTexCount) = 0;
    At<re::Ptr32<void>*>(mg, kArcadeTextures) = nullptr;
    At<int32_t>(mg, kArcadeSoundCount) = 0;
    At<int32_t>(mg, kArcadeSoundBank) = 0;
    At<int32_t*>(mg, kArcadeSounds) = nullptr;
    At<int32_t>(mg, 0x38) = 4;
    At<int32_t>(mg, 0x3C) = 0xF;
    At<int32_t>(mg, kArcadeTxdSlot) = -1;
    return mg;
}

// 0x6FE510: the destructor
void __fastcall ArcDestruct(uint8_t* mg, re::Edx) {
    At<uint32_t>(mg, 0) = 0x00948220;
    ArcEndCommon(mg);
    Arc_Screen_Destruct(ScreenOf(mg));
    Arc_Minigame_Destruct(mg);
}

// 0x6FE570: a layer's tick: each sprite moves, its collision callback runs for each other
// sprite it touches (layer, index, layer, other), its update callback (dt, layer, index);
// then the layer's (dt, layer: its thread's id saved from the layer's script itself)
void __fastcall ArcLayerUpdate(ArcadeLayer* layer, re::Edx, float dt) {
    for (int i = 0; i < layer->spriteCount; i++) {
        ArcadeSprite* s = &layer->sprites[i];
        Arc_Sprite_Update(s, dt, layer->id);
        if (Arc_Ref_IsSet(&s->collFunc) != 0) {
            lua_State* L = Arc_Ref_GetState(&s->collFunc);
            for (int j = 0; j < layer->spriteCount; j++) {
                if (j == i || Arc_Sprite_Overlaps(s, &layer->sprites[j]) == 0) {
                    continue;
                }
                void* current = Arc_Lur_GetCurrentOrNull(arcLurStore);
                void* script = Arc_Lur_FindScript(arcLurStore, Arc_Ref_GetScript(&s->collFunc));
                if (script == nullptr) {
                    continue;
                }
                int32_t saved = current != nullptr ? Arc_Script_GetThread(current) : -1;
                Arc_Script_SetThread(script, Arc_Ref_GetThread(&s->collFunc));
                Arc_Lur_SetCurrent(arcLurStore, script);
                Arc_Event_Begin(L);
                Arc_Lua_GetRef(L, Arc_Ref_GetId(&s->collFunc));
                Script::PushInt(L, layer->id);
                Script::PushInt(L, i);
                Script::PushInt(L, layer->id);
                Script::PushInt(L, j);
                Arc_Lua_Call(L, 4, 0);
                Arc_Script_SetThread(script, saved);
                Arc_Lur_SetCurrent(arcLurStore, current);
            }
        }
        if (Arc_Ref_IsSet(&s->updateFunc) != 0) {
            CallRefInItsScript(&s->updateFunc, 3, [&](lua_State* L) {
                Script::PushFloat(L, dt);
                Script::PushInt(L, layer->id);
                Script::PushInt(L, i);
            });
        }
    }
    uint32_t* ref = &layer->updateFunc;
    if (Arc_Ref_IsSet(ref) == 0) {
        return;
    }
    void* current = Arc_Lur_GetCurrentOrNull(arcLurStore);
    void* script = Arc_Lur_FindScript(arcLurStore, Arc_Ref_GetScript(ref));
    if (script == nullptr) {
        return;
    }
    int32_t saved = Arc_Script_GetThread(script);
    Arc_Script_SetThread(script, Arc_Ref_GetThread(ref));
    Arc_Lur_SetCurrent(arcLurStore, script);
    lua_State* L = Arc_Ref_GetState(ref);
    Arc_Event_Begin(L);
    Arc_Lua_GetRef(L, Arc_Ref_GetId(ref));
    Script::PushFloat(L, dt);
    Script::PushInt(L, layer->id);
    Arc_Lua_Call(L, 2, 0);
    Arc_Script_SetThread(script, saved);
    Arc_Lur_SetCurrent(arcLurStore, current);
}

// 0x6FE8C0: draws a layer (its rect, then its sprites) at (x, y) scaled
void __fastcall ArcLayerDraw(ArcadeLayer* layer, re::Edx, float x, float y, float sx, float sy) {
    float px = layer->x * sx;
    float py = layer->y * sy;
    float ssx = sx * layer->scaleX;
    float ssy = sy * layer->scaleY;
    px = layer->offsetX * ssx + px;
    py = layer->offsetY * ssy + py;
    px = x + px;
    py = y + py;
    float right = ssx * layer->width + px;
    float bottom = ssy * layer->height + py;
    float rect[4] = {px, bottom, right, py};
    if (640.0f < px || py > 480.0f || 0.0f > right || 0.0f > bottom) {
        return;
    }
    if (layer->texture != -1) {
        float uv1[2] = {0.0f, 0.0f};
        float uv2[2] = {1.0f, 1.0f};
        Arc_DrawRectUV(px, py, right, bottom, ArcTexture(layer->texture), layer->colour, uv1, uv2);
    } else {
        Arc_DrawRect(rect, layer->colour);
    }
    float baseX = px - layer->offsetX * ssx;
    float baseY = py - layer->offsetY * ssy;
    for (int i = 0; i < layer->spriteCount; i++) {
        Arc_Sprite_Draw(&layer->sprites[i], baseX, baseY, ssx, ssy);
    }
}

// 0x6FEB10 (virtual 0): the deleting destructor
uint8_t* __fastcall ArcDeletingDestructor(uint8_t* mg, re::Edx, int flags) {
    Arc_Destruct(mg);
    if ((flags & 1) != 0) {
        Arc_Free(mg);
    }
    return mg;
}

// 0x6FEB30: the screen's tick: its layers, then its callback (dt)
void __fastcall ArcScreenUpdate(ArcadeScreen* s, re::Edx) {
    float dt = arcTickRate * arcTimeStep;
    for (ArcadeLayer* layer = s->layers; layer != nullptr; layer = layer->next) {
        Arc_Layer_Update(layer, dt);
    }
    if (Arc_Ref_IsSet(&s->updateFunc) != 0) {
        lua_State* L = Arc_Ref_GetState(&s->updateFunc);
        Arc_Event_Begin(L);
        Arc_Lua_GetRef(L, Arc_Ref_GetId(&s->updateFunc));
        Script::PushFloat(L, dt);
        Arc_Lua_Call(L, 1, 0);
    }
}

// 0x6FEBC0: draws the screen: its background and texture, the layers, this frame's texts
// (then cleared), and the side borders where it doesn't reach the edges
void __fastcall ArcScreenDraw(ArcadeScreen* s, re::Edx) {
    Arc_Screen_ComputeRect(s);
    float rect[4] = {s->left, s->top, s->right, s->bottom};
    Arc_DrawRect(rect, s->background);
    if (s->texture != -1) {
        float uv1[2] = {0.0f, 0.0f};
        float uv2[2] = {1.0f, 1.0f};
        Arc_DrawRectUV(rect[0], rect[1], rect[2], rect[3], ArcTexture(s->texture), s->colour, uv1, uv2);
    }
    for (ArcadeLayer* layer = s->layers; layer != nullptr; layer = layer->next) {
        Arc_Layer_Draw(layer, s->centreX, s->centreY, s->drawScaleX, s->drawScaleY);
    }
    for (int i = 0; i < s->textCount; i++) {
        ArcadeText& t = s->texts[i];
        ArcSetFont(s->font);
        float scaleY = s->fontScale * s->drawScaleY;
        float scaleX = s->drawScaleX * s->fontScale;
        Arc_Font_SetScaleXY(scaleX, scaleY);
        uint8_t grey[4];
        uint8_t* colour = Arc_RGBA_Set(grey, 0x80, 0x80, 0x80, 0xFF);
        arcFontColour[0] = colour[0];
        arcFontColour[1] = colour[1];
        arcFontColour[2] = colour[2];
        arcFontColour[3] = colour[3];
        for (int k = 0; k < t.paramCount; k++) {
            Arc_Font_AddNumber(t.params[k]);
        }
        uint16_t buffer[0x80];
        Arc_Font_Format(buffer, Arc_Text_Get(arcTheText, t.key), 0x80);
        float ty = t.y * s->drawScaleY + s->centreY;
        float tx = t.x * s->drawScaleX + s->centreX;
        Arc_Font_PrintString(tx, ty, buffer, 1);
    }
    Arc_Font_Render();
    for (int i = 0; i < 32; i++) {
        s->texts[i].key[0] = 0;
        s->texts[i].paramCount = 0;
    }
    s->textCount = 0;
    float uv1[2] = {0.0f, 1.0f};
    float uv2[2] = {1.0f, 0.0f};
    if (rect[0] > 0.0f) {
        float border[4] = {0.0f, 480.0f, rect[0], 0.0f};
        if (s->leftTexture == -1) {
            Arc_DrawRect(border, s->borderColour);
        } else {
            Arc_DrawRectUV(0.0f, 480.0f, rect[0], 0.0f, ArcTexture(s->leftTexture), s->borderColour, uv1, uv2);
        }
    }
    if (rect[2] < 640.0f) {
        float border[4] = {rect[2], 480.0f, 640.0f, 0.0f};
        if (s->rightTexture == -1) {
            Arc_DrawRect(border, s->borderColour);
        } else {
            Arc_DrawRectUV(rect[2], 480.0f, 640.0f, 0.0f, ArcTexture(s->rightTexture), s->borderColour, uv1, uv2);
        }
    }
}

// 0x6FEFC0 (virtual 1): the tick: the exit popup, or the start button (the exit question
// when cancelling is allowed, else quits), or the screen's tick
void __fastcall ArcUpdate(uint8_t* mg, re::Edx) {
    if (mg[4] == 0) {
        return;
    }
    Arc_Screen_ComputeRect(ScreenOf(mg));
    void* popup = mg + 0x50;
    if (mg[0xA0] != 0 && At<int32_t>(mg, 0x50) == 1) {
        if (Arc_Popup_Result(popup) == 2) {
            mg[4] = 0;
        }
        return;
    }
    if (arcUserStartResponse != 0) {
        return;
    }
    if (Arc_Pad_JustDown(8, 0) == 0) {
        Arc_Screen_Update(ScreenOf(mg));
        return;
    }
    if (mg[kArcadeCancelConfirm] == 0) {
        mg[4] = 0;
        return;
    }
    Arc_Popup_Show(popup, 1, school::GameString(0x00944690), school::GameString(0x00941D14), 7, 8);   // EXIT_QUESTION
}

// 0x6FF050 (virtual 2): draws the screen, then the rastery effect: fresh noise (every
// other row darker), drawn over the screen in grey
void __fastcall ArcDraw(uint8_t* mg, re::Edx) {
    if (mg[0xA0] != 0) {
        Arc_Popup_Draw(mg + 0x50);
        return;
    }
    Arc_SetRenderState(6, 0);
    Arc_SetRenderState(8, 0);
    Arc_SetRenderState(0xC, 1);
    Arc_SetRenderState(10, 5);
    Arc_SetRenderState(11, 6);
    Arc_SetRenderState(9, 1);
    if (mg[5] == 0) {
        return;
    }
    Arc_Screen_Draw(ScreenOf(mg));
    if (mg[kArcadeRastery] == 0) {
        return;
    }
    uint8_t* pixel = Arc_RasterLock(arcRaster, 0, 1);
    for (int row = 0; row < 0x40; row++) {
        int odd = row & 1;
        for (int i = 0; i < 0x40; i++) {
            uint8_t v = uint8_t(Arc_Rand());
            pixel[2] = v;
            pixel[1] = v;
            pixel[0] = v;
            pixel[3] = uint8_t((odd != 0 ? 0 : 0xFF) & 0x1F);
            pixel[3] = uint8_t(pixel[3] + (Arc_Rand() & 3));
            pixel += 4;
        }
    }
    Arc_RasterUnlock(arcRaster);
    Arc_SetRenderState(10, 5);
    Arc_SetRenderState(11, 6);
    Arc_SetRenderState(9, 1);
    Arc_SetRenderState(2, 1);
    Arc_SetRenderState(1, static_cast<int>(re::PtrCast<uintptr_t>(arcRaster)));
    arcIm2DRaster = arcRaster;
    Arc_Im2DSetUVs(0.0f, 0.0f, 160.0f, 0.0f, 0.0f, 224.0f, 160.0f, 224.0f);
    Arc_SetRenderState(0xC, 1);
    uint32_t grey = 0xFF404040;
    arcIm2DColours[0] = grey;
    arcIm2DColours[1] = grey;
    arcIm2DColours[2] = grey;
    arcIm2DColours[3] = grey;
    Arc_Im2DDrawQuad(0.0f, 0.0f, 0.0f, 639.0f, 0.0f, 0.0f, 0.0f, 447.0f, 0.0f, 639.0f, 447.0f, 0.0f);
}

// 0x6FF230: links a layer at the end of the screen's list, its id one above the highest
// (the first keeps its own id); -1 for none
int32_t __fastcall ArcScreenAddLayer(ArcadeScreen* s, re::Edx, ArcadeLayer* layer) {
    if (layer == nullptr) {
        return -1;
    }
    ArcadeLayer* last = s->layers;
    if (last == nullptr) {
        s->layers = layer;
        return layer->id;
    }
    int32_t highest = 0;
    for (;;) {
        if (last->id > highest) {
            highest = last->id;
        }
        if (last->next == nullptr) {
            break;
        }
        last = last->next;
    }
    layer->id = highest + 1;
    last->next = layer;
    return layer->id;
}

// 0x6FF290: queues a line of text for this frame (no bounds check)
void __fastcall ArcScreenAddText(ArcadeScreen* s, re::Edx, float x, float y, const char* key, float p1, float p2) {
    ArcadeText& t = s->texts[s->textCount];
    Arc_Strncpy(t.key, key, 0x10);
    t.x = x;
    t.key[0xF] = 0;
    t.y = y;
    t.paramCount = 0;
    ArcadeText& a = s->texts[s->textCount];
    a.params[a.paramCount] = p1;
    a.paramCount += 1;
    ArcadeText& b = s->texts[s->textCount];
    b.params[b.paramCount] = p2;
    b.paramCount += 1;
    s->textCount += 1;
}

// 0x7005C0: a sprite's constructor
ArcadeSprite* __fastcall ArcSpriteConstruct(ArcadeSprite* s, re::Edx) {
    s->scaleX = 1.0f;
    s->scaleY = 1.0f;
    s->width = 16.0f;
    s->height = 16.0f;
    s->offsetX = 0.0f;
    s->offsetY = 0.0f;
    s->x = 0.0f;
    s->y = 0.0f;
    s->velX = 0.0f;
    s->velY = 0.0f;
    s->accX = 0.0f;
    s->accY = 0.0f;
    s->collH = 0.0f;
    s->collW = 16.0f;
    s->rot = 0.0f;
    s->rotSpeed = 0.0f;
    s->rotAcc = 0.0f;
    s->scaleSpeedX = 0.0f;
    s->scaleSpeedY = 0.0f;
    Arc_RGBA_Set(s->colour, 0x80, 0x80, 0x80, 0xFF);
    s->visible = 0;
    s->flipX = 0;
    s->flipY = 0;
    s->additive = 0;
    s->unknown58 = 0;
    s->unknown59 = 0;
    s->texture = -1;
    s->updateFunc = 0;
    s->unknown60 = 0;
    s->unknown64 = 0;
    s->collFunc = 0;
    s->font = 0;
    s->text[0] = 0;
    s->paramCount = 0;
    s->params[0] = 0.0f;
    s->params[1] = 0.0f;
    s->scoreTable = -1;
    return s;
}

// 0x700670: a layer's constructor
ArcadeLayer* __fastcall ArcLayerConstruct(ArcadeLayer* layer, re::Edx) {
    layer->id = 0;
    layer->next = nullptr;
    for (int i = 0; i < kArcadeMaxSprites; i++) {
        Arc_Sprite_Construct(&layer->sprites[i]);
    }
    layer->spriteCount = 0;
    layer->scaleX = 1.0f;
    layer->scaleY = 1.0f;
    layer->width = 640.0f;
    layer->height = 480.0f;
    layer->offsetX = 0.0f;
    layer->offsetY = 0.0f;
    layer->x = 0.0f;
    layer->y = 0.0f;
    Arc_RGBA_Set(layer->colour, 0x80, 0x80, 0x80, 0xFF);
    layer->texture = -1;
    layer->unknown70B4 = 0.0f;
    layer->unknown70B8 = 0.0f;
    layer->unknown70BC = nullptr;
    layer->updateFunc = 0;
    return layer;
}

// ---- the script natives -----------------------------------------------------------------------

// MGArcade_GetTextureID(name [, mask])
int __cdecl Native_MGArcade_GetTextureID(lua_State* L) {
    const char* mask = nullptr;
    if (Script::GetArgCount(L) >= 2) {
        mask = Script::GetString(L, 1);
    }
    uint8_t* mg = arcCurrentMinigame;
    Script::PushInt(L, Arc_FindTextureIndex(mg, Script::GetString(L, 0), mask));
    return 1;
}

// MGArcade_SetFont(font [, scale = 1])
int __cdecl Native_MGArcade_SetFont(lua_State* L) {
    float scale = 1.0f;
    if (Script::GetArgCount(L) >= 2) {
        scale = Script::GetFloat(L, 1);
    }
    ArcadeScreen* s = ScreenOf(arcCurrentMinigame);
    int font = Script::GetInt(L, 0);
    s->fontScale = scale;
    s->font = font;
    return 0;
}

// MGArcade_DrawText(x, y, key [, p1 [, p2]])
int __cdecl Native_MGArcade_DrawText(lua_State* L) {
    float p1 = 0.0f;
    float p2 = 0.0f;
    if (Script::GetArgCount(L) >= 4) {
        p1 = Script::GetFloat(L, 3);
        if (Script::GetArgCount(L) >= 5) {
            p2 = Script::GetFloat(L, 4);
        }
    }
    uint8_t* mg = arcCurrentMinigame;
    const char* key = Script::GetString(L, 2);
    float y = Script::GetFloat(L, 1);
    float x = Script::GetFloat(L, 0);
    Arc_Screen_AddText(ScreenOf(mg), x, y, key, p1, p2);
    return 0;
}

// a layer's two floats (layer, a, b)
template <int kOffset>
int __cdecl Native_ArcLayerSetPair(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    float a = Script::GetFloat(L, 1);
    float b = Script::GetFloat(L, 2);
    At<float>(layer, kOffset) = a;
    At<float>(layer, kOffset + 4) = b;
    return 0;
}

// MGArcade_Layer_SetTexture(layer, texture)
int __cdecl Native_MGArcade_Layer_SetTexture(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    layer->texture = Script::GetInt(L, 1);
    return 0;
}

// MGArcade_Layer_AddSprite(layer [, update [, collision]]): the new sprite's index (-1 when
// full: the callbacks then go just before the first sprite)
int __cdecl Native_MGArcade_Layer_AddSprite(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = layer->spriteCount;
    if (index < kArcadeMaxSprites) {
        layer->spriteCount = index + 1;
    } else {
        index = -1;
    }
    if (Script::GetArgCount(L) >= 2) {
        ArcadeLayer* again = FindLayer(Script::GetInt(L, 0));
        Script::StoreFunctionRef(L, 1, reinterpret_cast<uint8_t*>(again) + index * 0x90 + 0x64);
    }
    if (Script::GetArgCount(L) >= 3) {
        ArcadeLayer* again = FindLayer(Script::GetInt(L, 0));
        Script::StoreFunctionRef(L, 2, reinterpret_cast<uint8_t*>(again) + index * 0x90 + 0x70);
    }
    Script::PushInt(L, index);
    return 1;
}

// MGArcade_Sprite_SetSize(layer, sprite, w, h): also centres it and sizes its collision box
int __cdecl Native_MGArcade_Sprite_SetSize(lua_State* L) {
    float w = Script::GetFloat(L, 2);
    float h = Script::GetFloat(L, 3);
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    ArcadeSprite& a = SpriteOf(layer, Script::GetInt(L, 1));
    a.width = w;
    a.height = h;
    layer = FindLayer(Script::GetInt(L, 0));
    ArcadeSprite& b = SpriteOf(layer, Script::GetInt(L, 1));
    b.offsetX = -w * 0.5f;
    b.offsetY = -h * 0.5f;
    layer = FindLayer(Script::GetInt(L, 0));
    ArcadeSprite& c = SpriteOf(layer, Script::GetInt(L, 1));
    c.collW = w;
    c.collH = h;
    return 0;
}

// MGArcade_Sprite_SetOffset(layer, sprite, x, y) (its floats read first)
int __cdecl Native_MGArcade_Sprite_SetOffset(lua_State* L) {
    float x = Script::GetFloat(L, 2);
    float y = Script::GetFloat(L, 3);
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    ArcadeSprite& s = SpriteOf(layer, Script::GetInt(L, 1));
    s.offsetX = x;
    s.offsetY = y;
    return 0;
}

// MGArcade_Sprite_SetCollSize(layer, sprite, w [, h = 0])
int __cdecl Native_MGArcade_Sprite_SetCollSize(lua_State* L) {
    float h = 0.0f;
    if (Script::GetArgCount(L) >= 4) {
        h = Script::GetFloat(L, 3);
    }
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    ArcadeSprite& s = SpriteOf(layer, Script::GetInt(L, 1));
    s.collW = Script::GetFloat(L, 2);
    s.collH = h;
    return 0;
}

// a sprite's two floats (layer, sprite, a, b)
template <int kOffset>
int __cdecl Native_ArcSpriteSetPair(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    ArcadeSprite& s = SpriteOf(layer, Script::GetInt(L, 1));
    float a = Script::GetFloat(L, 2);
    float b = Script::GetFloat(L, 3);
    At<float>(&s, kOffset) = a;
    At<float>(&s, kOffset + 4) = b;
    return 0;
}

// a sprite's two floats pushed (layer, sprite)
template <int kOffset>
int __cdecl Native_ArcSpriteGetPair(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    ArcadeSprite& s = SpriteOf(layer, Script::GetInt(L, 1));
    float a = At<float>(&s, kOffset);
    float b = At<float>(&s, kOffset + 4);
    Script::PushFloat(L, a);
    Script::PushFloat(L, b);
    return 2;
}

// MGArcade_Sprite_SetRot(layer, sprite, degrees) (wrapped to the angle range)
int __cdecl Native_MGArcade_Sprite_SetRot(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    float degrees = Script::GetFloat(L, 2);
    float radians = degrees * 3.14159274f / 180.0f;
    SpriteOf(layer, index).rot = Arc_LimitAngle(radians);
    return 0;
}

// MGArcade_Sprite_SetRotSpeed (and MGArcade_Sprite_SetRotAcc: the same function)
int __cdecl Native_MGArcade_Sprite_SetRotSpeed(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    float degrees = Script::GetFloat(L, 2);
    SpriteOf(layer, index).rotSpeed = degrees * 3.14159274f / 180.0f;
    return 0;
}

// a sprite's angle pushed in degrees
template <int kOffset>
int __cdecl Native_ArcSpriteGetDegrees(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    ArcadeSprite& s = SpriteOf(layer, Script::GetInt(L, 1));
    float degrees = At<float>(&s, kOffset) * 180.0f / 3.14159274f;
    Script::PushFloat(L, degrees);
    return 1;
}

// a sprite's int (layer, sprite, value)
template <int kOffset>
int __cdecl Native_ArcSpriteSetInt(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    int value = Script::GetInt(L, 2);
    At<int32_t>(&SpriteOf(layer, index), kOffset) = value;
    return 0;
}

// a sprite's flag (layer, sprite, on)
template <int kOffset>
int __cdecl Native_ArcSpriteSetFlag(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    uint8_t value = Script::GetBool(L, 2);
    At<uint8_t>(&SpriteOf(layer, index), kOffset) = value;
    return 0;
}

// MGArcade_Sprite_GetTexture(layer, sprite)
int __cdecl Native_MGArcade_Sprite_GetTexture(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    Script::PushInt(L, SpriteOf(layer, Script::GetInt(L, 1)).texture);
    return 1;
}

// MGArcade_Sprite_SetFlip(layer, sprite, x, y)
int __cdecl Native_MGArcade_Sprite_SetFlip(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    SpriteOf(layer, index).flipX = Script::GetBool(L, 2);
    layer = FindLayer(Script::GetInt(L, 0));
    index = Script::GetInt(L, 1);
    SpriteOf(layer, index).flipY = Script::GetBool(L, 3);
    return 0;
}

// a sprite's callback (layer, sprite, function)
template <int kOffset>
int __cdecl Native_ArcSpriteSetFunc(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    Script::StoreFunctionRef(L, 2, &At<uint32_t>(&SpriteOf(layer, index), kOffset));
    return 0;
}

// MGArcade_Sprite_SetText(layer, sprite, key) (16 characters, not terminated when longer)
int __cdecl Native_MGArcade_Sprite_SetText(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    Arc_Strncpy(SpriteOf(layer, index).text, Script::GetString(L, 2), 0x10);
    return 0;
}

// MGArcade_Sprite_AddTextParam(layer, sprite, value): its index (-1 past two)
int __cdecl Native_MGArcade_Sprite_AddTextParam(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    float value = Script::GetFloat(L, 2);
    ArcadeSprite& s = SpriteOf(layer, index);
    if (s.paramCount >= 2) {
        Script::PushInt(L, -1);
        return 1;
    }
    s.params[s.paramCount] = value;
    s.paramCount += 1;
    Script::PushInt(L, s.paramCount - 1);
    return 1;
}

// MGArcade_Sprite_SetTextParam(layer, sprite, param, value) (0 or 1, else ignored)
int __cdecl Native_MGArcade_Sprite_SetTextParam(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    float value = Script::GetFloat(L, 3);
    uint32_t param = uint32_t(Script::GetInt(L, 2));
    if (param <= 1) {
        SpriteOf(layer, index).params[param] = value;
    }
    return 0;
}

// MGArcade_Sprite_SetTextToScoreName(layer, sprite, table, slot [, letter = -1])
int __cdecl Native_MGArcade_Sprite_SetTextToScoreName(lua_State* L) {
    int letter = -1;
    if (Script::GetArgCount(L) >= 5) {
        letter = Script::GetInt(L, 4);
    }
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    int slot = Script::GetInt(L, 3);
    ArcadeSprite& s = SpriteOf(layer, index);
    int table = Script::GetInt(L, 2);
    s.scoreTable = int8_t(table);
    s.scoreSlot = int8_t(slot);
    s.scoreChar = int8_t(letter);
    return 0;
}

// MGArcade_InitScreen(r, g, b [, leftTexture [, rightTexture [, update]]])
int __cdecl Native_MGArcade_InitScreen(lua_State* L) {
    uint8_t* mg = arcCurrentMinigame;
    int b = Script::GetInt(L, 2);
    int g = Script::GetInt(L, 1);
    int r = Script::GetInt(L, 0);
    uint8_t colour[4];
    uint8_t* set = Arc_RGBA_Set(colour, r, g, b, 0xFF);
    ArcadeScreen* s = ScreenOf(mg);
    s->background[0] = set[0];
    s->background[1] = set[1];
    s->background[2] = set[2];
    s->background[3] = set[3];
    if (Script::GetArgCount(L) >= 4) {
        ScreenOf(arcCurrentMinigame)->leftTexture = Script::GetInt(L, 3);
    }
    if (Script::GetArgCount(L) >= 5) {
        ScreenOf(arcCurrentMinigame)->rightTexture = Script::GetInt(L, 4);
    }
    if (Script::GetArgCount(L) >= 6) {
        Script::StoreFunctionRef(L, 5, &ScreenOf(arcCurrentMinigame)->updateFunc);
    }
    return 0;
}

// MGArcade_CreateLayer(w, h [, ?, update]): the new layer's id (centred on its position)
int __cdecl Native_MGArcade_CreateLayer(lua_State* L) {
    Arc_PushTxdMemoryTag();
    uint8_t* mg = arcCurrentMinigame;
    void* memory = Arc_Malloc(sizeof(ArcadeLayer));
    ArcadeLayer* created = memory != nullptr ? Arc_Layer_Construct(static_cast<ArcadeLayer*>(memory)) : nullptr;
    int32_t id = Arc_Screen_AddLayer(ScreenOf(mg), created);
    Arc_PopTxdMemoryTag();
    float w = Script::GetFloat(L, 0);
    float h = Script::GetFloat(L, 1);
    ArcadeLayer* layer = FindLayer(id);
    layer->width = w;
    layer->height = h;
    layer = FindLayer(id);
    layer->offsetX = -w * 0.5f;
    layer->offsetY = -h * 0.5f;
    Script::GetArgCount(L);
    if (Script::GetArgCount(L) >= 4) {
        Script::StoreFunctionRef(L, 3, &FindLayer(id)->updateFunc);
    }
    Script::PushInt(L, id);
    return 1;
}

// MGArcade_Layer_SetCol(layer, r, g, b, a)
int __cdecl Native_MGArcade_Layer_SetCol(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int a = Script::GetInt(L, 4);
    int b = Script::GetInt(L, 3);
    int g = Script::GetInt(L, 2);
    int r = Script::GetInt(L, 1);
    uint8_t colour[4];
    uint8_t* set = Arc_RGBA_Set(colour, r, g, b, a);
    layer->colour[0] = set[0];
    layer->colour[3] = set[3];
    layer->colour[1] = set[1];
    layer->colour[2] = set[2];
    return 0;
}

// MGArcade_Sprite_SetCol(layer, sprite, r, g, b, a)
int __cdecl Native_MGArcade_Sprite_SetCol(lua_State* L) {
    ArcadeLayer* layer = FindLayer(Script::GetInt(L, 0));
    int index = Script::GetInt(L, 1);
    int a = Script::GetInt(L, 5);
    int b = Script::GetInt(L, 4);
    int g = Script::GetInt(L, 3);
    int r = Script::GetInt(L, 2);
    uint8_t colour[4];
    uint8_t* set = Arc_RGBA_Set(colour, r, g, b, a);
    ArcadeSprite& s = SpriteOf(layer, index);
    s.colour[1] = set[1];
    s.colour[0] = set[0];
    s.colour[3] = set[3];
    s.colour[2] = set[2];
    return 0;
}

// MGArcade_Tan(radians) (the CRT's x87 tan)
int __cdecl Native_MGArcade_Tan(lua_State* L) {
    float angle = Script::GetFloat(L, 0);
    float result = crt::Tanf(angle);
    Script::PushFloat(L, result);
    return 1;
}

// MGArcade_Sqrt(x) (of its absolute value)
int __cdecl Native_MGArcade_Sqrt(lua_State* L) {
    float value = std::fabs(Script::GetFloat(L, 0));
    float result = crt::Sqrtf(value);
    Script::PushFloat(L, result);
    return 1;
}

// MGArcade_GetDistanceBetweenSprites(layerA, spriteA, layerB, spriteB)
int __cdecl Native_MGArcade_GetDistanceBetweenSprites(lua_State* L) {
    ArcadeLayer* layerB = FindLayer(Script::GetInt(L, 2));
    int indexB = Script::GetInt(L, 3);
    ArcadeLayer* layerA = FindLayer(Script::GetInt(L, 0));
    const ArcadeSprite& from = SpriteOf(layerB, indexB);
    const ArcadeSprite& to = SpriteOf(layerA, Script::GetInt(L, 1));
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float sum = dy * dy + dx * dx;
    float distance = crt::Sqrtf(std::fabs(sum));
    Script::PushFloat(L, distance);
    return 1;
}

// MGArcade_GetOrientationBetweenSprites(layerA, spriteA, layerB, spriteB): the unit vector
// from A to B
int __cdecl Native_MGArcade_GetOrientationBetweenSprites(lua_State* L) {
    ArcadeLayer* layerA = FindLayer(Script::GetInt(L, 0));
    int indexA = Script::GetInt(L, 1);
    ArcadeLayer* layerB = FindLayer(Script::GetInt(L, 2));
    const ArcadeSprite& from = SpriteOf(layerA, indexA);
    const ArcadeSprite& to = SpriteOf(layerB, Script::GetInt(L, 3));
    float v[2] = {to.x - from.x, to.y - from.y};
    Arc_Vector2D_Normalise(v);
    Script::PushFloat(L, v[0]);
    Script::PushFloat(L, v[1]);
    return 2;
}

} // namespace

#define ARCADE44_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ARCADE44_REVERSED("MGArcade::Start_6FD060", 0x006FD060, &ArcStart);
ARCADE44_REVERSED("MGArcade::UnloadTextures_6FD0E0", 0x006FD0E0, &ArcUnloadTextures);
ARCADE44_REVERSED("MGArcade::FindTexture_6FD170", 0x006FD170, &ArcFindTexture);
ARCADE44_REVERSED("MGArcade::LoadSounds_6FD1E0", 0x006FD1E0, &ArcLoadSounds);
ARCADE44_REVERSED("MGArcade::UnloadSounds_6FD300", 0x006FD300, &ArcUnloadSounds);
ARCADE44_REVERSED("MGArcade::GetSoundId_6FD350", 0x006FD350, &ArcGetSoundId);
ARCADE44_REVERSED("ArcadeLayer::~ArcadeLayer_6FD3B0", 0x006FD3B0, &ArcLayerDestruct);
ARCADE44_REVERSED("MGArcade::End_6FD410", 0x006FD410, &ArcEnd);
// (0x6FD41A is inside 0x6FD410, after its first instructions: eax, ebx and the flags live)
RE_UNHOOKED("MGArcade::End_6FD410 (inner label)", 0x006FD41A);
ARCADE44_REVERSED("MGArcade::LoadTextures_6FD470", 0x006FD470, &ArcLoadTextures);
ARCADE44_REVERSED("ArcadeScreen::ComputeRect_6FD530", 0x006FD530, &ArcScreenComputeRect);
ARCADE44_REVERSED("ArcadeSprite::Update_6FD670", 0x006FD670, &ArcSpriteUpdate);
ARCADE44_REVERSED("ArcadeSprite::Draw_6FD700", 0x006FD700, &ArcSpriteDraw);
ARCADE44_REVERSED("ArcadeSprite::Overlaps_6FDE50", 0x006FDE50, &ArcSpriteOverlaps);
ARCADE44_REVERSED("ArcadeScreen::ArcadeScreen_6FE370", 0x006FE370, &ArcScreenConstruct);
ARCADE44_REVERSED("ArcadeScreen::~ArcadeScreen_6FE470", 0x006FE470, &ArcScreenDestruct);
ARCADE44_REVERSED("MGArcade::MGArcade_6FE4B0", 0x006FE4B0, &ArcConstruct);
ARCADE44_REVERSED("MGArcade::~MGArcade_6FE510", 0x006FE510, &ArcDestruct);
ARCADE44_REVERSED("ArcadeLayer::Update_6FE570", 0x006FE570, &ArcLayerUpdate);
ARCADE44_REVERSED("ArcadeLayer::Draw_6FE8C0", 0x006FE8C0, &ArcLayerDraw);
ARCADE44_REVERSED("MGArcade::DeletingDestructor_6FEB10", 0x006FEB10, &ArcDeletingDestructor);
ARCADE44_REVERSED("ArcadeScreen::Update_6FEB30", 0x006FEB30, &ArcScreenUpdate);
ARCADE44_REVERSED("ArcadeScreen::Draw_6FEBC0", 0x006FEBC0, &ArcScreenDraw);
ARCADE44_REVERSED("MGArcade::Update_6FEFC0", 0x006FEFC0, &ArcUpdate);
ARCADE44_REVERSED("MGArcade::Draw_6FF050", 0x006FF050, &ArcDraw);
ARCADE44_REVERSED("ArcadeScreen::AddLayer_6FF230", 0x006FF230, &ArcScreenAddLayer);
ARCADE44_REVERSED("ArcadeScreen::AddText_6FF290", 0x006FF290, &ArcScreenAddText);
ARCADE44_REVERSED("ArcadeSprite::ArcadeSprite_7005C0", 0x007005C0, &ArcSpriteConstruct);
ARCADE44_REVERSED("ArcadeLayer::ArcadeLayer_700670", 0x00700670, &ArcLayerConstruct);
ARCADE44_REVERSED("Native_MGArcade_GetTextureID", 0x006FF340, &Native_MGArcade_GetTextureID);
ARCADE44_REVERSED("Native_MGArcade_SetFont", 0x006FF410, &Native_MGArcade_SetFont);
ARCADE44_REVERSED("Native_MGArcade_DrawText", 0x006FF470, &Native_MGArcade_DrawText);
ARCADE44_REVERSED("Native_MGArcade_Layer_SetOffset", 0x006FF510, &Native_ArcLayerSetPair<0x709C>);
ARCADE44_REVERSED("Native_MGArcade_Layer_SetPos", 0x006FF580, &Native_ArcLayerSetPair<0x70A4>);
ARCADE44_REVERSED("Native_MGArcade_Layer_SetScale", 0x006FF5F0, &Native_ArcLayerSetPair<0x708C>);
ARCADE44_REVERSED("Native_MGArcade_Layer_SetTexture", 0x006FF660, &Native_MGArcade_Layer_SetTexture);
ARCADE44_REVERSED("Native_MGArcade_Layer_AddSprite", 0x006FF6B0, &Native_MGArcade_Layer_AddSprite);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetSize", 0x006FF7B0, &Native_MGArcade_Sprite_SetSize);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetOffset", 0x006FF8D0, &Native_MGArcade_Sprite_SetOffset);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetCollSize", 0x006FF950, &Native_MGArcade_Sprite_SetCollSize);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetPos", 0x006FF9D0, &Native_ArcSpriteSetPair<0x18>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetVel", 0x006FFA50, &Native_ArcSpriteSetPair<0x20>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetAcc", 0x006FFAD0, &Native_ArcSpriteSetPair<0x28>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetRot", 0x006FFB50, &Native_MGArcade_Sprite_SetRot);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetRotSpeed", 0x006FFBD0, &Native_MGArcade_Sprite_SetRotSpeed);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetScale", 0x006FFC40, &Native_ArcSpriteSetPair<0x00>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetScaleSpeed", 0x006FFCC0, &Native_ArcSpriteSetPair<0x44>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetTexture", 0x006FFD40, &Native_ArcSpriteSetInt<0x50>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_GetTexture", 0x006FFDA0, &Native_MGArcade_Sprite_GetTexture);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetVisible", 0x006FFDF0, &Native_ArcSpriteSetFlag<0x54>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetAdditive", 0x006FFE50, &Native_ArcSpriteSetFlag<0x57>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetFlip", 0x006FFEB0, &Native_MGArcade_Sprite_SetFlip);
ARCADE44_REVERSED("Native_MGArcade_Sprite_GetPos", 0x006FFF50, &Native_ArcSpriteGetPair<0x18>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_GetVel", 0x006FFFD0, &Native_ArcSpriteGetPair<0x20>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_GetRot", 0x00700050, &Native_ArcSpriteGetDegrees<0x38>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_GetRotSpeed", 0x007000C0, &Native_ArcSpriteGetDegrees<0x3C>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_GetScale", 0x00700130, &Native_ArcSpriteGetPair<0x00>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_GetScaleSpeed", 0x007001B0, &Native_ArcSpriteGetPair<0x44>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetUpdateFunc", 0x00700230, &Native_ArcSpriteSetFunc<0x5C>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetCollFunc", 0x00700280, &Native_ArcSpriteSetFunc<0x68>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetText", 0x007002D0, &Native_MGArcade_Sprite_SetText);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetFont", 0x00700330, &Native_ArcSpriteSetInt<0x6C>);
ARCADE44_REVERSED("Native_MGArcade_Sprite_AddTextParam", 0x00700390, &Native_MGArcade_Sprite_AddTextParam);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetTextParam", 0x00700440, &Native_MGArcade_Sprite_SetTextParam);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetTextToScoreName", 0x007004B0, &Native_MGArcade_Sprite_SetTextToScoreName);
ARCADE44_REVERSED("Native_MGArcade_InitScreen", 0x00700730, &Native_MGArcade_InitScreen);
ARCADE44_REVERSED("Native_MGArcade_CreateLayer", 0x00700810, &Native_MGArcade_CreateLayer);
ARCADE44_REVERSED("Native_MGArcade_Layer_SetCol", 0x00700940, &Native_MGArcade_Layer_SetCol);
ARCADE44_REVERSED("Native_MGArcade_Sprite_SetCol", 0x007009E0, &Native_MGArcade_Sprite_SetCol);
ARCADE44_REVERSED("Native_MGArcade_Tan", 0x00700B00, &Native_MGArcade_Tan);
ARCADE44_REVERSED("Native_MGArcade_Sqrt", 0x00700B40, &Native_MGArcade_Sqrt);
ARCADE44_REVERSED("Native_MGArcade_GetDistanceBetweenSprites", 0x00700B90, &Native_MGArcade_GetDistanceBetweenSprites);
ARCADE44_REVERSED("Native_MGArcade_GetOrientationBetweenSprites", 0x00700C70,
                  &Native_MGArcade_GetOrientationBetweenSprites);
