#pragma once

// What the frontend screens' files (FEAttractControls.cpp, FEDisplayInventory.cpp,
// FELoadOptions.cpp, FEPhotoSave.cpp, FEListWindows.cpp, FEStatsTasks.cpp) share: the
// game functions they call and small helpers. The screens' own layouts are raw offsets
// (see each file's notes); Frontend.h has the list windows' structs.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace fescr {

using re::At;
inline uint8_t* Ptr(void* base, int offset) {
    return static_cast<uint8_t*>(base) + offset;
}

// obj's virtual at byte offset `offset` (thiscall)
template <typename R, typename... A>
inline R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}
template <typename R, typename... A>
inline R ThisCall(uintptr_t address, void* self, A... args) {
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(address)(self, args...);
}
inline void SetVtable(void* object, uintptr_t vtable) {
    *reinterpret_cast<uint32_t*>(object) = vtable;
}

// ---- the menus' input ----------------------------------------------------------------------
GAME_VAR(uint8_t, g_menuLeft, 0x020C3280);
GAME_VAR(uint8_t, g_menuRight, 0x020C327C);
GAME_VAR(uint8_t, g_menuUp, 0x020C3278);
GAME_VAR(uint8_t, g_menuDown, 0x020C3274);
GAME_VAR(uint8_t, g_menuActive, 0x020C32EF);  // cleared: the menu closes
void* const kUserInput = reinterpret_cast<void*>(0x00A45A30);
GAME_FN(uint8_t, __thiscall, Input_AcceptPressed, 0x0050E1C0, void* input, int pad);
GAME_FN(uint8_t, __thiscall, Input_BackPressed, 0x0050E200, void* input, int pad);
GAME_FN(uint8_t, __thiscall, Input_AcceptDown, 0x0050E180, void* input, int pad);
GAME_FN(uint8_t, __cdecl, FE_PadPressed, 0x006A3170, int pad);   // (the repeat timer allows)
GAME_FN(void, __cdecl, FE_PadClear, 0x006A3120, int pad);
GAME_FN(void, __cdecl, FE_PlaySound, 0x006A30D0, int sound, int unknown);
GAME_FN(void, __cdecl, FE_PlaySelect, 0x006A2360);
GAME_FN(int, __cdecl, GetActivePad, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_IsXInput, 0x00738500, int pad);
GAME_FN(uint8_t, __cdecl, Pad_ButtonJustUp, 0x00738940, int button, int pad);

// ---- drawing --------------------------------------------------------------------------------
// (x, y, the text, then a text style's colour, font, justification and a flag)
GAME_FN(void, __cdecl, FE_DrawText, 0x006A5450, int x, int y, const uint16_t* text, int colour, int font, int justify,
        int flag);
GAME_ARRAY(float, g_lastTextRect, 0x00BA6898);  // FE_DrawText's extent: left, top, right, bottom
GAME_FN(void, __cdecl, FE_DrawHighlight, 0x006A4790, float left, float right, float y);
GAME_FN(void, __cdecl, FE_DrawWindow, 0x006A4730, float centreX, float width, float y, void* colour, float height);
GAME_FN(void, __cdecl, FE_DrawTitle, 0x006A6510, const char* key);
GAME_FN(void, __thiscall, Screen_Load, 0x006A7150, void* screen, void* archive);
GAME_FN(void, __thiscall, TextStyle_Load, 0x006A48A0, void* style, void* archive, const char* name);
GAME_FN(void, __cdecl, Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(float*, __cdecl, ScreenPosition, 0x0052D4F0, float* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const void* colour, const float* uv1, const float* uv2);
GAME_FN(void*, __cdecl, GetButtonTexture, 0x005145B0, int button, uint8_t* isKey, int pad, int player);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_ARRAY(uint8_t, g_fontDropColour, 0x0208B780);
GAME_VAR(int16_t, g_fontDrop, 0x0208B784);

// ---- text -----------------------------------------------------------------------------------
void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(const uint16_t*, __thiscall, Text_GetByHash, 0x0068FF10, void* text, uint32_t hash);
GAME_FN(void, __cdecl, AsciiToWide, 0x00690090, const char* src, uint16_t* dst);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
template <typename... A>
inline int __cdecl SprintfS(char* buffer, size_t size, const char* format, A... rest) { return crt::SprintfS(buffer, size, format, rest...); }
inline int __cdecl WcsCpyS(uint16_t* dst, int size, const uint16_t* src) { return crt::WcscpyS((wchar_t*)dst, (size_t)size, (const wchar_t*)src); }
inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

// ---- the list windows (0x69AB40.. FEScrollingSelectionListWindow, 0x69B8E0.. FESelectionListWindow)
GAME_FN(void, __thiscall, SelList_Reset, 0x0069B8F0, void* list);
GAME_FN(void, __thiscall, SelList_Clear, 0x0069BAE0, void* list);
GAME_FN(int, __thiscall, SelList_AddByName, 0x0069BF30, void* list, const char* name);
GAME_FN(int8_t, __thiscall, SelList_Next, 0x0069B920, void* list);
GAME_FN(int8_t, __thiscall, SelList_Previous, 0x0069B960, void* list);
GAME_FN(void, __thiscall, SelList_Select, 0x0069B9A0, void* list, int index);
GAME_FN(void, __thiscall, SelList_Load, 0x0069BAF0, void* list, void* archive);
GAME_FN(void, __thiscall, SelList_Draw, 0x0069C410, void* list);
GAME_FN(void, __thiscall, ScrollList_Reset, 0x0069AB50, void* list);
GAME_FN(void, __thiscall, ScrollList_Clear, 0x0069ABD0, void* list);
GAME_FN(void, __thiscall, ScrollList_Load, 0x0069ABF0, void* list, void* archive);
GAME_FN(int, __thiscall, ScrollList_Next, 0x0069B830, void* list);
GAME_FN(int, __thiscall, ScrollList_Previous, 0x0069B890, void* list);
GAME_FN(void, __thiscall, ScrollList_Draw, 0x0069B6B0, void* list, float x, float y, int);
inline int8_t SelList_Selected(void* list) {
    return At<int8_t>(list, 0x68);
}

// A scrolling list's item added (inlined everywhere): items at +0x68 of the list
inline void ScrollList_AddItem(void* list, void* item) {
    int32_t& count = At<int32_t>(list, 4);
    At<void*>(list, 0x68 + count * 4) = item;
    if (At<uint8_t>(item, 4) != 0) {
        At<int32_t>(list, 8) += 1;
        if (At<int32_t>(list, 0xC) < 0) {
            At<int32_t>(list, 0xC) = count;
        }
    }
    count += 1;
}

// ---- the game's state the menus touch -------------------------------------------------------
GAME_VAR(void*, g_radar, 0x00C674CC);
GAME_VAR(uint8_t, g_specialEntitiesCreated, 0x00CA3984);
GAME_VAR(uint8_t, g_menu20C32F6, 0x020C32F6);
void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
GAME_VAR(uint8_t*, g_player, 0x00C1AEA8);
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(uint8_t, g_widescreen, 0x00C1870E);
GAME_VAR(uint32_t, g_timeInMs, 0x00C1A9B4);

} // namespace fescr
