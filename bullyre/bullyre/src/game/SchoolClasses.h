#pragma once

// What the school classes' minigames of 0x6ECF80..0x6FC790 share (ClassEnglishPlay.cpp,
// ClassGeography*.cpp, ClassMath*.cpp, ClassMusic*.cpp): the Minigame base's fields, its
// popup, the HUD font and drawing calls, the pad, the sounds and the script callback.
//
// Every class starts with the Minigame base (0x705xxx): +0x4 active, +0x5 HUD shown,
// +0x6 success, +0x1C the textures, +0x4C the sound ids, +0x50 the popup (+0x50 its kind:
// 1 the exit question, 2 the info box; +0xA0 showing). The x87 runs at 24-bit precision
// here (the main thread): float throughout.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/TrackMath.h"
#include "reversed.h"

#include "game/Crt.h"
namespace school {

using re::At;
inline const char* GameString(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}
inline uint8_t* GameColour(uintptr_t address) {
    return reinterpret_cast<uint8_t*>(address);
}

// ---- the game's state ---------------------------------------------------------------------
GAME_VAR(float, screenScaleX, 0x00A957A8);
GAME_VAR(float, screenScaleY, 0x00A957AC);
GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, language, 0x00C18724);
GAME_VAR(uint8_t, widescreen, 0x00C1870E);
GAME_VAR(uint8_t*, currentMinigame, 0x020C8704);
GAME_VAR(int32_t, userStartResponse, 0x020C87AC);
GAME_VAR(uint8_t, radarHidden, 0x00A459FC);
GAME_VAR(uint8_t*, musicChannel, 0x00CF4288);     // the class's music (a stream channel)
inline void* const sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const camera = reinterpret_cast<void*>(0x00A14810);
inline void* const theText = reinterpret_cast<void*>(0x020C2FA4);

// ---- the C runtime, memory, txds ---------------------------------------------------------------
inline int __cdecl Rand() { return crt::Rand(); }   // (the game's rand: its seed is shared)
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Streaming_RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, Streaming_LoadAllRequestedModels, 0x00533EE0, int);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);

// the CRT's x87 sqrt (0x85C500; its argument and result on the FPU stack)
inline float CrtSqrt(float x) {
    return crt::Sqrtf(x);
}
// the distance the classes measure: sqrt(|dx*dx + dy*dy|), each step in float
inline float Distance2D(float dx, float dy) {
    float sum = dx * dx + dy * dy;
    return CrtSqrt(std::fabs(sum));
}
// a texture's size (its virtuals +0x48, +0x4C)
inline uint32_t TexWidth(void* texture) {
    return reinterpret_cast<uint32_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture))[0x48 / 4]))(texture);
}
inline uint32_t TexHeight(void* texture) {
    return reinterpret_cast<uint32_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture))[0x4C / 4]))(texture);
}

// ---- the font -------------------------------------------------------------------------------
GAME_ARRAY(uint8_t, fontColour, 0x0208B764);
GAME_ARRAY(uint8_t, fontShadowColour, 0x0208B780);
GAME_VAR(int16_t, fontShadow, 0x0208B784);
GAME_VAR(int16_t, fontStyle, 0x0208B788);
GAME_VAR(uint8_t, fontFlags, 0x0208B78A);           // 0x20 outline
GAME_ARRAY(void*, fontStyleTexture, 0x0208B218);    // [10] (non-null: the style is loaded)
GAME_FN(void, __cdecl, Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(int, __cdecl, WStrLen, 0x00690020, const uint16_t* s);

inline void SetFontColour(const uint8_t* colour) {
    fontColour[0] = colour[0];
    fontColour[1] = colour[1];
    fontColour[2] = colour[2];
    fontColour[3] = colour[3];
}
inline void SetFontShadowColour(const uint8_t* colour) {
    fontShadowColour[0] = colour[0];
    fontShadowColour[1] = colour[1];
    fontShadowColour[2] = colour[2];
    fontShadowColour[3] = colour[3];
}
inline const uint16_t* GetText(const char* key) {
    return Text_Get(theText, key);
}

// ---- drawing --------------------------------------------------------------------------------
GAME_FN(float*, __cdecl, ScreenPosition, 0x0052D4F0, float* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
// (rect: left, bottom, right, top)
GAME_FN(void, __cdecl, DrawSprite, 0x0052D610, const float* rect, void* texture, const uint8_t* colour,
        const float* uv1, const float* uv2);
GAME_FN(uint8_t*, __thiscall, RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void*, __cdecl, GetButtonTexture, 0x00514EC0, int button);   // (virtuals +0x48 width, +0x4C height)

// ---- the pad --------------------------------------------------------------------------------
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustUp, 0x00738940, int control, int pad);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, int duration, int strength, int);

// ---- sounds and the music -------------------------------------------------------------------
GAME_FN(uint8_t*, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, uint32_t id, const float* pos, int, uintptr_t, int,
        int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(int32_t, __thiscall, Channel_IsPlaying, 0x005A6D60, void* channel);
GAME_FN(void, __thiscall, Channel_Pause, 0x005A6E40, void* channel);
GAME_FN(int32_t, __thiscall, Channel_IsPaused, 0x005A6D80, void* channel);
GAME_FN(void, __thiscall, Channel_Resume, 0x005A6E70, void* channel);

// a minigame's sound (its ids at +0x4C), the way the classes play them
inline uint8_t* PlayClassSound(void* mg, int sound) {
    return Sounds_Play(sounds, At<uint32_t*>(mg, 0x4C)[sound], nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}

// ---- the Minigame base ----------------------------------------------------------------------
GAME_FN(int, __thiscall, Popup_Result, 0x00705080, void* popup);
GAME_FN(void, __thiscall, Popup_Show, 0x00705220, void* popup, int kind, const char* title, const char* buttons,
        int, int);
GAME_FN(void, __thiscall, Popup_Draw, 0x00706000, void* popup);
GAME_FN(void, __thiscall, Minigame_Success, 0x00705790, void* mg);
GAME_FN(void, __thiscall, Minigame_EndTextures, 0x00704FD0, void* mg);
GAME_FN(void, __thiscall, Minigame_EndSounds, 0x00704DE0, void* mg);
GAME_FN(void, __thiscall, Camera_RestoreFov, 0x00454DE0, void* camera);
GAME_FN(void, __thiscall, Camera_SetFov, 0x00454DB0, void* camera, float fov);
GAME_FN(void, __thiscall, Minigame_Construct, 0x007059D0, void* mg);
GAME_FN(void, __thiscall, Minigame_Destruct, 0x00705A50, void* mg);
GAME_FN(void*, __thiscall, Minigame_GetTexture, 0x006D7CB0, void* mg, int index);   // (nullptr without one)
GAME_FN(void, __thiscall, Minigame_SetTimerShown, 0x007058A0, void* minigame, float time);
// a Minigame's texture table entry, and its sounds'
struct ClassTextureEntry {
    int32_t id;
    re::Ptr32<const char> name;
    re::Ptr32<const char> mask;
};
static_assert(sizeof(ClassTextureEntry) == 0xC);
struct ClassSoundEntry {
    int32_t id;
    re::Ptr32<const char> name;
};
static_assert(sizeof(ClassSoundEntry) == 0x8);
GAME_FN(void, __thiscall, Minigame_LoadTextures, 0x00704D30, void* mg, int count, const ClassTextureEntry* entries,
        const char* txd);
GAME_FN(void, __thiscall, Minigame_LoadSounds, 0x00704E90, void* mg, int count, const ClassSoundEntry* entries,
        const char* bank);
// a texture set released and its txd removed (`self` unused)
GAME_FN(void, __thiscall, Minigame_UnloadTextureSet, 0x00704E40, void* mg, re::Ptr32<void>* textures, int count, int txdSlot);

// ---- the script callback (a function reference) ---------------------------------------------
GAME_FN(uint8_t, __thiscall, ScriptRef_IsSet, 0x005D80E0, uint32_t* ref);
GAME_FN(void*, __thiscall, ScriptRef_GetState, 0x005D9C20, uint32_t* ref);
GAME_FN(int32_t, __thiscall, ScriptRef_GetId, 0x005D80D0, uint32_t* ref);
GAME_FN(void, __cdecl, Event_Begin, 0x005D8270, void* state);
GAME_FN(void, __cdecl, Lua_GetRef, 0x005D8670, void* state, int ref);
GAME_FN(void, __cdecl, Lua_Call, 0x005D82A0, void* state, int args, int results);
GAME_FN(void, __thiscall, ScriptRef_Clear, 0x005D9C50, uint32_t* ref);

// the trigger function run (when one is set)
inline void RunScriptRef(uint32_t* ref) {
    if (ScriptRef_IsSet(ref) != 0) {
        void* state = ScriptRef_GetState(ref);
        Event_Begin(state);
        Lua_GetRef(state, ScriptRef_GetId(ref));
        Lua_Call(state, 0, 0);
    }
}

} // namespace school
