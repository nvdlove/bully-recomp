// The frontend's manager: its screens (24, at 0x20C3288; the current one at 0xBA6890),
// opening and closing the pause menu, the title (start) menu and the map/"other" menu
// each frame, the menus' sounds and key-repeat timers, their textures, and a screen
// position's anchoring. 0x6A2320..0x6A4332.
//
// A position (0x14 bytes): +0 anchor (0..8: TOPLEFT, TOP, TOPRIGHT, LEFT, CENTER, RIGHT,
// BOTTOMLEFT, BOTTOM, BOTTOMRIGHT), +4 x, +8 y, +0xC width, +0x10 height (in 640x480).
// A screen's vtable: 3 OnEnter, 4 OnLeave, 6 Draw; +4 its title's hash.

#include <cstring>

#include "game/FEScreensCommon.h"

#include "game/Crt.h"
#include "reversed.h"
namespace femgr {
using namespace fescr;

namespace {

// the screens
GAME_ARRAY(void*, g_screens, 0x020C3288);          // [24]
GAME_VAR(int32_t, g_screen, 0x00BA6890);           // the current one, -1: none
GAME_VAR(int32_t, g_menuKind, 0x00BA6894);         // (1 map/other, 3 title, 0 pause, 4 opened on a screen)
GAME_VAR(void*, g_background, 0x020C32AC);         // (drawn under the screens)
GAME_VAR(void*, g_screen8, 0x020C32A8);
GAME_VAR(uint8_t*, g_screen4, 0x020C3298);          // (+0xCC: busy, the menus can't close)
GAME_ARRAY(int32_t, g_titleScreenIds, 0x00BA6920);  // [3] the title menu's tabs
GAME_ARRAY(int32_t, g_otherScreenIds, 0x00BA692C);  // [3] the map menu's tabs
GAME_VAR(int32_t, g_pauseFirstScreen, 0x00BA68B0);  // (the pause menu's first item's screen)
void* const kTitleTabs = reinterpret_cast<void*>(0x020C3508);
void* const kOtherTabs = reinterpret_cast<void*>(0x020C3488);
void* const kStartList = reinterpret_cast<void*>(0x020C33F8);
GAME_VAR(int32_t, g_startListCount, 0x020C33FC);
GAME_VAR(int32_t, g_startListSelectable, 0x020C3400);
GAME_VAR(int32_t, g_startListSelected, 0x020C3404);
GAME_ARRAY(void*, g_startListItems, 0x020C3460);
GAME_ARRAY(uint8_t, g_startMenuItems, 0x00BA68A8);  // [10] CStartMenuItem (0xC: vtable, selectable, screen)

// the state
GAME_VAR(uint8_t, g_frontendOn, 0x00BA688D);
GAME_VAR(uint8_t, g_pauseMenu, 0x020C32E9);    // the pause menu is up
GAME_VAR(uint8_t, g_otherMenu, 0x020C32EA);    // the map menu is up
GAME_VAR(uint8_t, g_inTitle, 0x020C32EB);      // the title menu is up
GAME_VAR(uint8_t, g_returnToMain, 0x020C32EC);
GAME_VAR(uint8_t, g_texturesLoaded, 0x020C32F0);
GAME_VAR(uint8_t, g_storyStarted, 0x020C32F1);
GAME_VAR(uint8_t, g_titleRequested, 0x020C32F2);
GAME_VAR(uint8_t, g_ownsMissionText, 0x020C32F3);  // (false: loaded "Frontend" text itself)
GAME_VAR(uint8_t, g_leaveTitle, 0x020C32F6);
GAME_VAR(uint8_t, g_menu20C32F7, 0x020C32F7);
GAME_VAR(uint8_t, g_rampsLoaded, 0x020C32FB);
GAME_VAR(float, g_completion, 0x020C3304);
GAME_VAR(uint32_t, g_titleTimeout, 0x020C32FC);
GAME_VAR(int32_t, g_missionTextLoaded, 0x020C2FB4);
GAME_VAR(uint8_t, g_photographyOnSave, 0x00C1B172);
GAME_VAR(uint8_t, g_inputDisabledA45A4F, 0x00A45A4F);
GAME_VAR(int32_t, g_menuAllowed20C87AC, 0x020C87AC);
GAME_VAR(uint8_t, g_windowFocusBD0FCB, 0x00BD0FCB);
GAME_VAR(int32_t, g_currArea, 0x00BD1008);
GAME_VAR(int32_t, g_noPauseC3CD0C, 0x00C3CD0C);
GAME_VAR(uint8_t, g_blockC1A998, 0x00C1A998);
GAME_VAR(uint8_t, g_blockC1A999, 0x00C1A999);
GAME_VAR(uint8_t, g_blockC1A99A, 0x00C1A99A);
// where the player was when the title menu opened
GAME_VAR(int32_t, g_savedArea, 0x020C336C);
GAME_ARRAY(uint32_t, g_savedPosition, 0x020C3310);
GAME_VAR(float, g_savedHeading, 0x020C3284);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);

// sounds and key repeat
GAME_VAR(int32_t, g_sound, 0x020C3300);         // the sound asked for (0: from the input)
GAME_ARRAY(uint8_t, g_soundPending, 0x020C31B4);  // [pad]
GAME_ARRAY(uint8_t, g_repeatSet, 0x020C31B0);     // [pad]
GAME_ARRAY(uint32_t, g_repeatTime, 0x020C31CC);   // [4]
GAME_ARRAY(uint8_t, g_soundsOn, 0x020C3270);      // [pad]
GAME_VAR(uint8_t, g_repeatFast, 0x020C31C8);
GAME_VAR(uint16_t, g_repeatFirstDelay, 0x020C32F4);
GAME_VAR(uint16_t, g_repeatDelay, 0x020C32F8);
GAME_VAR(uint32_t, g_frontendTime, 0x00A14A50);
GAME_ARRAY(uint8_t, g_padsActive, 0x020C3354);    // [4] 0x10 apart
void* const kAudio = reinterpret_cast<void*>(0x00CF3DA0);
GAME_FN(void, __thiscall, FM_PlaySound, 0x0059C150, void* audio, int sound);

// the textures
GAME_ARRAY(void*, g_feTextures, 0x020C31E8);  // [14] checkmark .. period, then the backgrounds
GAME_VAR(void*, g_rampCol, 0x020C31DC);
GAME_VAR(void*, g_rampGen, 0x020C31E0);
GAME_VAR(void*, g_rampStart, 0x020C31E4);
GAME_VAR(void*, g_logo, 0x020C321C);
GAME_FN(void, __cdecl, FM_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, FM_PopMemoryTag, 0x005EF100);
GAME_FN(void, __cdecl, FM_PushTxdMemoryTag, 0x0052E700);
GAME_FN(void, __cdecl, FM_PopTxdMemoryTag, 0x0052E710);
GAME_FN(void, __cdecl, FM_TidyUpMemory, 0x004302A0, int, int);
GAME_FN(int, __cdecl, FM_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(int, __cdecl, FM_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(uint8_t, __cdecl, FM_LoadTxd, 0x005F2A80, int slot, const char* file);
GAME_FN(void, __cdecl, FM_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, FM_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void, __cdecl, FM_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, FM_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void*, __cdecl, FM_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, FM_TextureDestroy, 0x005F0F90, void* texture);
GAME_FN(void, __cdecl, FM_RemoveTxd, 0x005F1830, int slot);
GAME_FN(void, __thiscall, FM_TextLoadMission, 0x0068FDA0, void* text, const char* name);
GAME_FN(void, __thiscall, FM_TextUnloadMission, 0x0068F550, void* text);

// drawing
GAME_FN(void, __cdecl, FM_SetRenderState, 0x00519790, int state, int value);
GAME_FN(uint8_t*, __thiscall, FM_RGBA, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, FM_RenderStates, 0x006A2450);
GAME_FN(float, __thiscall, FM_PosLeft, 0x006A2800, void* position);
GAME_FN(float, __thiscall, FM_PosTop, 0x006A2890, void* position);
GAME_ARRAY(uint8_t, g_messageColour, 0x020C333C);
GAME_ARRAY(uint8_t, g_feColours, 0x020C331C);  // [12]
GAME_ARRAY(float, g_feFloats, 0x0208B240);     // [9]
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(uint16_t, g_fontDropShadow, 0x0208B784);

// the frames drawn black while the menus switch
GAME_FN(uint8_t, __cdecl, FM_BeginFrame, 0x0043AB30, int, int, int, int, int, int, int, int, int, int, int, int,
        int);
GAME_FN(void, __cdecl, FM_Render4094B0, 0x004094B0, int);
GAME_FN(void, __cdecl, FM_EndFrame, 0x0043ACE0, int);

// the game around it
GAME_FN(void, __thiscall, FM_StreamStop, 0x005913D0, void* player, int channel, int fadeMs);
GAME_FN(void, __thiscall, FM_StreamPlayByName, 0x00591330, void* player, const char* name, float volume, int loop,
        int channel);
void* const kStreamPlayer = reinterpret_cast<void*>(0x00CF4324);
GAME_FN(void, __thiscall, FM_RadarRemoveBlip, 0x00548DE0, void* radar, int index);
GAME_FN(float, __cdecl, FM_GameCompletion, 0x006A98C0);
GAME_FN(void, __cdecl, FM_Pad738560, 0x00738560);
GAME_FN(void, __cdecl, FM_EndUserPause, 0x0045B820);
GAME_FN(void, __cdecl, FM_StartUserPause, 0x0045B810);
GAME_FN(void, __cdecl, FM_GenerateBoundingBoxes, 0x0045B960, int);
GAME_FN(void, __cdecl, FM_PadsUpdate, 0x00738610);
GAME_FN(void, __cdecl, FM_PadStopRumble, 0x00738780);
GAME_FN(void, __cdecl, FM_Nothing49A650, 0x0049A650, int);  // (an empty function)
void* const kCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_FN(int, __thiscall, FM_CameraFadeState, 0x004F12F0, void* camera);
GAME_FN(uint8_t, __cdecl, FM_Pad7387E0, 0x007387E0);
GAME_FN(uint8_t, __cdecl, FM_Pad738800, 0x00738800);
GAME_FN(uint8_t, __cdecl, FM_Pad738820, 0x00738820);
GAME_FN(uint8_t, __cdecl, FM_Fn42E430, 0x0042E430);
GAME_FN(uint8_t, __cdecl, FM_PadJustDown, 0x00738900, int button, int pad);
GAME_FN(void, __thiscall, FM_ActionInputsClear, 0x005F3A40, void* inputs);

// the frontend's own
GAME_FN(void, __cdecl, FM_LoadTextures, 0x006A2B00);
GAME_FN(void, __cdecl, FM_UnloadTextures, 0x006A2D50);
GAME_FN(void, __cdecl, FM_OtherMenuOpen, 0x006A2570);
GAME_FN(void, __cdecl, FM_FillTitleTabs, 0x006A24B0);
GAME_FN(void, __cdecl, FM_FillOtherTabs, 0x006A2500);
GAME_FN(void, __cdecl, FM_FillStartList, 0x006A3390);
GAME_FN(void, __cdecl, FM_TitleMenuOpen, 0x006A3580);
GAME_FN(void, __cdecl, FM_PauseMenuOpen, 0x006A35E0);
GAME_FN(void, __cdecl, FM_PlayMenuSound, 0x006A31B0);
GAME_FN(const char*, __cdecl, FM_ScreenName, 0x006A6D70, int index);
GAME_FN(const char*, __cdecl, FM_ColourName, 0x006A6D90, int index);
GAME_FN(const char*, __cdecl, FM_FloatName, 0x006A6DB0, int index);
GAME_FN(void, __thiscall, FM_SelListAdd, 0x0069B900, void* list, uint32_t hash);
GAME_FN(void, __thiscall, FM_SelListLoadNamed, 0x0069BC80, void* list, void* archive, const char* name);
GAME_FN(void*, __thiscall, FM_MissionCurrentSlot, 0x006AA680, void* mgr);
GAME_FN(uint8_t, __thiscall, FM_MissionSlotActive, 0x006AA3F0, void* slot);
GAME_FN(uint32_t*, __thiscall, FM_NameKey, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, FM_NameKeyDestruct, 0x0049A650, uint32_t* key);
GAME_FN(int32_t, __thiscall, FM_MissionFind, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, FM_MissionFlags, 0x006AA7C0, void* mgr, int32_t mission);

// the data file
GAME_FN(void*, __cdecl, FM_ReaderOpen, 0x005B07E0, const char* path, int32_t* out, int);
GAME_FN(uint8_t, __thiscall, FM_ReaderSection, 0x005B0720, void* reader, void* object, const char* name);
GAME_FN(void, __cdecl, FM_ReaderClose, 0x005B0860, void* reader);
template <typename... A>
inline int __cdecl FM_Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline int __cdecl FM_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
constexpr uintptr_t VT_FrontEnd = 0x00940F04;

// a screen's virtual
void ScreenCall(void* screen, int offset) {
    VCall<void>(screen, offset);
}

// the menu time a pad's key repeats at
uint32_t RepeatDue() {
    uint16_t delay = g_repeatFast != 0 ? g_repeatDelay : g_repeatFirstDelay;
    return uint32_t(delay) + g_frontendTime;
}

// two frames cleared (the menus switching); with `restoreStates`, the render states
// 6 and 8 set back when the first one began a frame and the pause menu is down
void ClearFrames(bool restoreStates) {
    FM_BeginFrame(0, 0, 0, 0, 0, 0, 0xFF, 0, 0, 0, 0x280, 0x1E0, 0);
    FM_Render4094B0(7);
    FM_EndFrame(0);
    uint8_t began = FM_BeginFrame(0, 0, 0, 0, 0, 0, 0xFF, 0, 0, 0, 0x280, 0x1E0, 0);
    FM_Render4094B0(7);
    if (restoreStates && began == 1 && g_pauseMenu == 0) {
        FM_SetRenderState(6, 1);
        FM_SetRenderState(8, 1);
    }
    FM_EndFrame(0);
}

// the game resumed after a menu closed (the tail most closings share)
void ResumeGame() {
    FM_EndUserPause();
    FM_GenerateBoundingBoxes(0);
    FM_PadsUpdate();
}

void ClearPlayerInputs() {
    FM_ActionInputsClear(At<void*>(g_playerPed, 0x2E0));
}

bool Screen4Busy() {
    return g_screen == 4 && At<uint8_t>(g_screen4, 0xCC) != 0;
}

} // namespace

// 0x6A2320: the current screen's title hash (0: none)
uint32_t __cdecl CurrentScreenTitle() {
    if (g_screen == -1) {
        return 0;
    }
    return At<uint32_t>(g_screens[g_screen], 4);
}

// 0x6A2340: a sound asked for (played by 0x6A31B0)
void __cdecl RequestSound(int sound) {
    g_sound = sound;
    g_soundPending[0] = 1;
}

// 0x6A2360: the default sound asked for
void __cdecl RequestSoundDefault() {
    g_sound = 0;
    g_soundPending[0] = 1;
}

// 0x6A2380: the menu sounds on (8 bytes)
void __cdecl SoundsOn() {
    g_soundsOn[0] = 1;
}

// 0x6A2390: the menu sounds on, nothing pending
void __cdecl SoundsOnNoPending() {
    g_soundsOn[0] = 1;
    g_soundPending[0] = 0;
}

// 0x6A23A0: the menu sounds off (8 bytes)
void __cdecl SoundsOff() {
    g_soundsOn[0] = 0;
}

// 0x6A23B0: the screens' layouts read from Config\hdt\Frontend.HDT (Frontenj / Frontenr
// for Japanese / Russian)
void __cdecl LoadLayouts() {
    int32_t out = 0;
    const char* path;
    if (g_language == 7) {
        path = "Config\\hdt\\Frontenj.HDT";
    } else if (g_language == 6) {
        path = "Config\\hdt\\Frontenr.HDT";
    } else {
        path = "Config\\hdt\\Frontend.HDT";
    }
    void* reader = FM_ReaderOpen(path, &out, 1);
    uintptr_t frontEnd = VT_FrontEnd;  // (a FrontEnd on the stack: its Load reads the colours)
    if (reader == nullptr) {
        return;
    }
    FM_ReaderSection(reader, &frontEnd, "FrontEnd");
    for (int i = 0; i < 0x18; ++i) {
        FM_ReaderSection(reader, g_screens[i], FM_ScreenName(i));
    }
    FM_ReaderClose(reader);
}

// 0x6A2450: the render states the frontend draws with
void __cdecl SetRenderStates() {
    Font_Reset();
    FM_SetRenderState(6, 0);
    FM_SetRenderState(8, 0);
    FM_SetRenderState(0xA, 5);
    FM_SetRenderState(0xB, 6);
    FM_SetRenderState(0xC, 1);
    FM_SetRenderState(9, 2);
    FM_SetRenderState(2, 3);
    FM_SetRenderState(7, 2);
}

// 0x6A24B0: the title menu's tabs
void __cdecl FillTitleTabs() {
    SelList_Clear(kTitleTabs);
    At<uint8_t>(kTitleTabs, 0x2C) = 0;
    At<uint8_t>(kTitleTabs, 0x2D) = 1;
    At<uint8_t>(kTitleTabs, 0x6C) = 1;
    for (int32_t* id = g_titleScreenIds; id < g_titleScreenIds + 3; ++id) {
        FM_SelListAdd(kTitleTabs, At<uint32_t>(g_screens[*id], 4));
    }
    SelList_Reset(kTitleTabs);
}

// 0x6A2500: the map menu's tabs (the tasks' tab says "current task" while one runs)
void __cdecl FillOtherTabs() {
    SelList_Clear(kOtherTabs);
    for (int32_t* id = g_otherScreenIds; id < g_otherScreenIds + 3; ++id) {
        uint32_t hash = At<uint32_t>(g_screens[*id], 4);
        if (*id == 0xB) {
            void* slot = FM_MissionCurrentSlot(kMissionMgr);
            hash = HashName(FM_MissionSlotActive(slot) ? "FE_CURR_TASK" : "FE_AVAIL_TASKS");
        }
        FM_SelListAdd(kOtherTabs, hash);
    }
    SelList_Reset(kOtherTabs);
}

// 0x6A2570: the map menu opened on its first tab
void __cdecl OtherMenuOpen() {
    FM_FillOtherTabs();
    g_screen = g_otherScreenIds[0];
    g_menuActive = 1;
    g_menuKind = 1;
    FM_RadarRemoveBlip(g_radar, -1);
    g_completion = FM_GameCompletion();
}

// 0x6A25B0: a screen entered (the current one left first); true when it changed
uint8_t __cdecl SetScreen(int index) {
    int32_t old = g_screen;
    if (index != -1) {
        if (old != -1) {
            ScreenCall(g_screens[old], 0x10);
        }
        g_screen = index;
        ScreenCall(g_screens[index], 0xC);
    }
    return old != index;
}

// 0x6A2600: a frontend texture by index (0 checkmark, 1 controller, 2 photo frame, 3
// film, 4 yearbook frame, 5 tile, 6 cap, 7 bracket, 8 period, 9.. backgrounds, logo)
void* __cdecl GetTexture(uint32_t index) {
    if (index > 0xD) {
        return nullptr;
    }
    return g_feTextures[index];
}

// 0x6A2620: the story's started (8 bytes)
void __cdecl SetStoryStarted() {
    g_storyStarted = 1;
}

// 0x6A2630: an anchor from its name (LEFT when none matches)
int __cdecl AnchorFromName(const char* name) {
    static const char* const kNames[] = {"TOPLEFT", "TOP",        "TOPRIGHT", "LEFT",       "CENTER",
                                         "RIGHT",   "BOTTOMLEFT", "BOTTOM",   "BOTTOMRIGHT"};
    for (int i = 0; i < 9; ++i) {
        if (FM_Stricmp(name, kNames[i]) == 0) {
            return i;
        }
    }
    return 3;
}

// 0x6A2730: a position's x (its column's origin: 0, 320 or 640). (An anchor past 8 adds
// the bits of `self` as a float: the slot the pushed ecx left.)
float __fastcall Position_GetX(uint8_t* self) {
    float origin;
    switch (At<uint32_t>(self, 0)) {
    case 0: case 3: case 6: origin = 0.0f; break;
    case 1: case 4: case 7: origin = 320.0f; break;
    case 2: case 5: case 8: origin = 640.0f; break;
    default: std::memcpy(&origin, &self, 4); break;
    }
    return At<float>(self, 4) * g_screenScaleX + origin;
}

// 0x6A2790: its y (its row's origin: 0, 240 or 480; the same quirk past 8)
float __fastcall Position_GetY(uint8_t* self) {
    float origin;
    switch (At<uint32_t>(self, 0)) {
    case 0: case 1: case 2: origin = 0.0f; break;
    case 3: case 4: case 5: origin = 240.0f; break;
    case 6: case 7: case 8: origin = 480.0f; break;
    default: std::memcpy(&origin, &self, 4); break;
    }
    return At<float>(self, 8) + origin;
}

// 0x6A2800: its left edge (the width scaled, aligned in its column)
float __fastcall Position_GetLeft(uint8_t* self) {
    float width = At<float>(self, 0xC) * g_screenScaleX;
    float offset;
    switch (At<uint32_t>(self, 0)) {
    case 0: case 3: case 6: offset = 0.0f; break;
    case 1: case 4: case 7: offset = (640.0f - width) * 0.5f; break;
    case 2: case 5: case 8: offset = 640.0f - width; break;
    default: offset = width; break;
    }
    return At<float>(self, 4) + offset;
}

// 0x6A2890: its top edge
float __fastcall Position_GetTop(uint8_t* self) {
    float height = At<float>(self, 0x10) * g_screenScaleY;
    float offset;
    switch (At<uint32_t>(self, 0)) {
    case 0: case 1: case 2: offset = 0.0f; break;
    case 3: case 4: case 5: offset = (480.0f - height) * 0.5f; break;
    case 6: case 7: case 8: offset = 480.0f - height; break;
    default: offset = height; break;
    }
    return At<float>(self, 8) + offset;
}

// 0x6A2920: its width, scaled
float __fastcall Position_GetWidth(uint8_t* self) {
    return At<float>(self, 0xC) * g_screenScaleX;
}

// 0x6A2940: its height, scaled
float __fastcall Position_GetHeight(uint8_t* self) {
    return At<float>(self, 0x10) * g_screenScaleY;
}

// 0x6A2960: a cycling option's name added (16 bytes each from +0x10, count at +0x40)
void __fastcall NameList_Add(uint8_t* self, re::Edx, const char* name) {
    Strncpy(reinterpret_cast<char*>(self + (At<int32_t>(self, 0x40) + 1) * 0x10), name, 0xF);
    At<int32_t>(self, 0x40) += 1;
}

// 0x6A2990: the next name (wrapping)
void __fastcall NameList_Next(uint8_t* self) {
    At<int32_t>(self, 0x44) += 1;
    if (At<int32_t>(self, 0x44) >= At<int32_t>(self, 0x40)) {
        At<int32_t>(self, 0x44) = 0;
    }
}

// 0x6A29B0: the previous one (wrapping)
void __fastcall NameList_Previous(uint8_t* self) {
    At<int32_t>(self, 0x44) += -1;
    if (At<int32_t>(self, 0x44) < 0) {
        At<int32_t>(self, 0x44) = At<int32_t>(self, 0x40) - 1;
    }
}

// 0x6A29C0: the screen's width in menu units (853 widescreen)
float __cdecl ScreenWidth() {
    return g_widescreen != 0 ? 853.0f : 640.0f;
}

// 0x6A29E0: and its height
float __cdecl ScreenHeight() {
    return 480.0f;
}

// 0x6A29F0 (FEScrollingSelectionListWindowT vf1/vf2): an item
void* __fastcall ScrollList_GetItem(uint8_t* self, re::Edx, int index) {
    return At<void*>(self, 0x68 + index * 4);
}

// 0x6A2A30: the colour ramps' textures (once)
void __cdecl LoadRamps() {
    FM_PushMemoryTag(0x17);
    if (g_rampsLoaded == 0) {
        g_rampsLoaded = 1;
        FM_PushCurrentTxd();
        int slot = FM_FindTxdSlot("ColorRamp", 0);
        if (slot == -1) {
            slot = FM_AddTxdSlot("ColorRamp");
        }
        FM_PushTxdMemoryTag();
        FM_TidyUpMemory(0, 1);
        FM_LoadTxd(slot, "TXD/colramp.TXD");
        FM_PopTxdMemoryTag();
        FM_AddRef(slot);
        FM_SetCurrentTxd(slot);
        g_rampCol = FM_FindTexture("FE_colramps", nullptr);
        g_rampGen = FM_FindTexture("FE_genramps", nullptr);
        g_rampStart = FM_FindTexture("FE_startramps", nullptr);
        FM_PopCurrentTxd();
        g_texturesLoaded = 1;
        FM_TidyUpMemory(0, 1);
    }
    FM_PopMemoryTag();
}

// 0x6A2B00: the frontend's textures (fe1 in the title menu: the logo; fe2 in game: the
// icons), and the "Frontend" text unless a mission's text is in
void __cdecl LoadTextures() {
    FM_PushMemoryTag(0x17);
    if (g_texturesLoaded == 0) {
        g_ownsMissionText = g_missionTextLoaded != 0;
        if (g_ownsMissionText == 0) {
            FM_TextLoadMission(kTheText, "Frontend");
        }
        FM_PushCurrentTxd();
        FM_PushTxdMemoryTag();
        FM_TidyUpMemory(0, 1);
        bool title = g_inTitle != 0 || g_titleRequested != 0;
        int slot;
        const char* file;
        if (title) {
            slot = FM_FindTxdSlot("FrontEnd1", 0);
            if (slot == -1) {
                slot = FM_AddTxdSlot("FrontEnd1");
            }
            file = "TXD/fe1.TXD";
        } else {
            slot = FM_FindTxdSlot("FrontEnd2", 0);
            if (slot == -1) {
                slot = FM_AddTxdSlot("FrontEnd2");
            }
            file = "TXD/fe2.TXD";
        }
        FM_LoadTxd(slot, file);
        FM_PopTxdMemoryTag();
        FM_AddRef(slot);
        FM_SetCurrentTxd(slot);
        if (g_inTitle != 0 || g_titleRequested != 0) {
            if (g_language == 2) {
                g_logo = FM_FindTexture("Bully_Logo_Ger", nullptr);
            } else if (g_language == 7) {
                g_logo = FM_FindTexture("Bully_Logo_JP", nullptr);
            } else {
                g_logo = FM_FindTexture("Bully_Logo", nullptr);
            }
        } else {
            g_feTextures[0] = FM_FindTexture("HUD_checkmark", "HUD_checkmark_x");
            g_feTextures[1] = FM_FindTexture("fe_controller", "fe_controllersh");
            g_feTextures[2] = FM_FindTexture("FE_photoframe", "FE_photoframe_x");
            g_feTextures[3] = FM_FindTexture("FE_film", "FE_film_x");
            g_feTextures[6] = FM_FindTexture("FE_yrbook_cap", nullptr);
            g_feTextures[4] = FM_FindTexture("FE_yrbook_frame", nullptr);
            g_feTextures[5] = FM_FindTexture("FE_yrbook_tile", nullptr);
            g_feTextures[7] = FM_FindTexture("FE_bracket", "FE_bracket_x");
            g_feTextures[8] = FM_FindTexture("Period", "Period_x");
        }
        g_feTextures[9] = FM_FindTexture("Background01", nullptr);
        g_feTextures[10] = FM_FindTexture("Background02", nullptr);
        g_feTextures[11] = FM_FindTexture("Background04", nullptr);
        g_feTextures[12] = FM_FindTexture("Background03", nullptr);
        FM_PopCurrentTxd();
        g_texturesLoaded = 1;
        FM_TidyUpMemory(0, 1);
    }
    FM_PopMemoryTag();
}

// 0x6A2D50: and released (the text too, once the story's started)
void __cdecl UnloadTextures() {
    FM_PushMemoryTag(0x17);
    if (g_texturesLoaded != 0) {
        if (g_ownsMissionText == 0 && g_storyStarted != 0) {
            FM_TextUnloadMission(kTheText);
        }
        bool title = g_inTitle != 0 || g_titleRequested != 0;
        int slot = FM_FindTxdSlot(title ? "FrontEnd1" : "FrontEnd2", 0);
        if (g_inTitle == 0 && g_titleRequested == 0) {
            for (re::Ptr32<void>* texture = g_feTextures; texture < g_feTextures + 12; ++texture) {
                FM_TextureDestroy(*texture);
            }
        }
        FM_RemoveTxd(slot);
        g_texturesLoaded = 0;
    }
    FM_PopMemoryTag();
}

// 0x6A2E00: the backdrop: the background screen drawn, then the four background
// quarters (white; a black colour made and not used)
void __cdecl DrawBackdrop() {
    FM_RenderStates();
    if (g_inTitle != 0) {
        FM_SetRenderState(0x14, 1);
    }
    ScreenCall(g_background, 0x18);
    void* quarter1 = g_feTextures[9];
    void* quarter2 = g_feTextures[10];
    void* quarter3 = g_feTextures[11];
    void* quarter4 = g_feTextures[12];
    uint8_t white[4];
    uint8_t black[4];
    FM_RGBA(white, 0xFF, 0xFF, 0xFF, 0xFF);
    FM_RGBA(black, 0, 0, 0, 0xFF);
    float width = g_widescreen != 0 ? 853.0f : 640.0f;
    width = g_screenScaleX * width;
    float half = width * 0.5f;
    float uv1[2] = {0.0f, 0.0f};
    float uv2[2] = {1.0f, 1.0f};
    DrawRectUV(0.0f, 0.0f, half, 240.0f, quarter1, white, uv1, uv2);
    DrawRectUV(half, 0.0f, width, 240.0f, quarter2, white, uv1, uv2);
    DrawRectUV(0.0f, 240.0f, half, 480.0f, quarter3, white, uv1, uv2);
    DrawRectUV(half, 240.0f, width, 480.0f, quarter4, white, uv1, uv2);
}

// 0x6A2FF0: a message (by hash) in the middle, 190 above the centre
void __cdecl DrawMessage(uint32_t hash) {
    g_fontColour[0] = g_messageColour[0];
    g_fontColour[1] = g_messageColour[1];
    g_fontColour[2] = g_messageColour[2];
    g_fontColour[3] = g_messageColour[3];
    Font_SetSizePreset(4);
    Font_SetJustify(2);
    float y = g_screenScaleY * -190.0f;
    g_fontDropShadow = 1;
    float position[2];
    ScreenPosition(position, g_screenScaleX * 0.0f, y, 1, 1);
    uint16_t text[64];
    Font_Format(text, Text_GetByHash(kTheText, hash), 0x40);
    Font_PrintString(position[0], position[1], text, 1);
}

// 0x6A30D0: a sound for a pad (pad 0's asked for), its key-repeat timer restarted
void __cdecl PlaySoundForPad(int sound, int pad) {
    if (pad == 0) {
        g_sound = sound;
        g_soundPending[0] = 1;
    }
    g_repeatSet[pad] = 1;
    g_repeatTime[pad] = RepeatDue();
}

// 0x6A3120: a pad's key repeat restarted (pad 0: the default sound too)
void __cdecl ClearPad(int pad) {
    if (pad == 0) {
        g_sound = pad;
        g_soundPending[0] = 1;
    }
    g_repeatSet[pad] = 1;
    g_repeatTime[pad] = RepeatDue();
}

// 0x6A3170: a pad's input may repeat (its timer just set, or run out)
uint8_t __cdecl PadRepeatReady(int pad) {
    uint32_t time = g_repeatTime[pad];
    uint32_t due = RepeatDue();
    if (time == due || g_frontendTime >= time) {
        return 1;
    }
    return 0;
}

// 0x6A31B0: the menu sound played for the first active pad (asked for, or from the input:
// 1 left/up, 2 right/down, 3 back, 4 other); else the repeat tick when its timer's due
void __cdecl PlayMenuSound() {
    int pad = 0;
    for (int i = 0; i < 4; ++i) {
        if (g_padsActive[i * 0x10] != 0) {
            pad = i;
            break;
        }
    }
    if (g_soundsOn[pad] == 0) {
        return;
    }
    if (g_soundPending[pad] != 0) {
        g_soundPending[pad] = 0;
        int sound = g_sound;
        if (sound == 0) {
            if (g_menuLeft != 0 || g_menuUp != 0) {
                sound = 1;
            } else if (g_menuRight != 0 || g_menuDown != 0) {
                sound = 2;
            } else {
                sound = Input_BackPressed(kUserInput, 0) ? 3 : 4;
            }
            g_sound = sound;
        }
        static const int kSounds[14] = {0xC, 0xD, 1, 0, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13, 0x18, 0x19, 0, 0x1A};
        if (uint32_t(sound - 1) <= 0xD) {
            FM_PlaySound(kAudio, kSounds[sound - 1]);
        }
        return;
    }
    uint32_t time = g_repeatTime[0];
    uint32_t due = RepeatDue();
    if (time == due || g_frontendTime >= time) {
        FM_PlaySound(kAudio, 0x14);
        g_repeatSet[pad] = 0;
    }
}

// 0x6A3390: the pause menu's list (the photography item once its class is passed, item 6
// only with 0xC1B172 set)
void __cdecl FillStartList() {
    ScrollList_Clear(kStartList);
    uint8_t allow6 = g_photographyOnSave;
    uint32_t key;
    FM_NameKey(&key, "C_Photography_1");
    int32_t mission = FM_MissionFind(kMissionMgr, &key);
    FM_NameKeyDestruct(&key);
    bool photography = *reinterpret_cast<uint16_t*>(FM_MissionFlags(kMissionMgr, mission) + 2) > 0;
    for (int i = 0; i < 10; ++i) {
        uint8_t* item = g_startMenuItems + i * 0xC;
        int32_t id = At<int32_t>(item, 8);
        if ((id == 6 && allow6 == 0) || (id == 0xE && !photography)) {
            continue;
        }
        g_startListItems[g_startListCount] = item;
        if (At<uint8_t>(item, 4) != 0) {
            g_startListSelectable += 1;
            if (g_startListSelected < 0) {
                g_startListSelected = g_startListCount;
            }
        }
        g_startListCount += 1;
    }
    ScrollList_Reset(kStartList);
}

// 0x6A3580: the title menu opened
void __cdecl TitleMenuOpen() {
    FM_FillTitleTabs();
    void* radar = g_radar;
    g_menuKind = 3;
    g_completion = FM_GameCompletion();
    FM_RadarRemoveBlip(radar, -1);
    FM_StreamPlayByName(kStreamPlayer, "MS_Gobble.rsm", 0.4f, 1, 6);
    FM_Pad738560();
    g_titleTimeout = g_frontendTime + 60000;
}

// 0x6A35E0: the pause menu opened
void __cdecl PauseMenuOpen() {
    FM_FillStartList();
    g_menuKind = 0;
    FM_RadarRemoveBlip(g_radar, -1);
    g_completion = FM_GameCompletion();
}

// 0x6A3610: each frame: the title menu left when asked, the menus opened and closed by
// start (5) / select (4) / back, the title menu opened when asked
void __cdecl UpdateMenus() {
    if (g_frontendOn == 0) {
        return;
    }
    if (g_inTitle != 0 && g_leaveTitle != 0) {
        FM_UnloadTextures();
        if (g_inTitle != 0) {
            FM_StreamStop(kStreamPlayer, 6, 500);
        }
        g_leaveTitle = 0;
        g_inTitle = 0;
        ClearFrames(true);
        if (g_inTitle == 0) {
            if (g_menuActive != 0) {
                ScreenCall(g_screens[g_screen], 0x10);
                g_screen = -1;
            }
            g_menuActive = 0;
            ScreenCall(g_background, 0x10);
            ResumeGame();
        }
    }
    if (g_inputDisabledA45A4F == 0 && g_menuAllowed20C87AC == 0) {
        return;
    }
    if (FM_CameraFadeState(kCamera) == 2 || FM_CameraFadeState(kCamera) == 1) {
        return;
    }

    // the window lost focus (or a pad's gone): the menus open
    bool focusLost = g_windowFocusBD0FCB != 0 &&
                     (FM_Pad7387E0() || FM_Pad738800() || !FM_Pad738820());
    if (focusLost && (g_inputDisabledA45A4F != 0 || g_menuAllowed20C87AC != 0)) {
        if (g_pauseMenu != 0) {
            if (g_menuActive != 0 && g_screen != 8 && g_screen != 0x15) {
                FM_PauseMenuOpen();
                ScreenCall(g_screens[g_screen], 0x10);
                g_menuActive = 0;
            }
            return;
        }
        if (g_inTitle != 0) {
            if (g_menuActive != 0 && g_screen != 0x14) {
                FM_TitleMenuOpen();
            }
            return;
        }
        if (g_otherMenu != 0) {
            if (g_menuActive != 0) {
                ScreenCall(g_screens[g_screen], 0x10);
                g_menuActive = 0;
            }
            g_otherMenu = 0;
        } else {
            FM_LoadTextures();
        }
        if (g_titleRequested == 0) {
            FM_PauseMenuOpen();
            g_pauseMenu = 1;
        } else {
            FM_TitleMenuOpen();
            g_inTitle = 1;
            g_titleRequested = 0;
        }
        ClearFrames(false);
        if (g_inTitle != 0) {
            g_screen = g_titleScreenIds[0];
        } else if (g_screen != 0x15) {
            g_screen = g_pauseFirstScreen;
        }
        ScreenCall(g_background, 0xC);
        FM_PadStopRumble();
        FM_StartUserPause();
        return;
    }

    // the title menu asked for
    if (g_titleRequested == 1) {
        g_titleRequested = 0;
        if (g_storyStarted != 0) {
            g_savedArea = g_currArea;
            uint8_t* ped = g_playerPed;
            uint8_t* matrix = At<uint8_t*>(ped, 0x14);
            uint32_t* position = reinterpret_cast<uint32_t*>(matrix != nullptr ? matrix + 0x30 : ped + 4);
            g_savedPosition[0] = position[0];
            g_savedPosition[1] = position[1];
            g_savedPosition[2] = position[2];
            g_savedHeading = At<float>(ped, 0x1700);
            FM_UnloadTextures();
        }
        g_inTitle = 1;
        FM_LoadTextures();
        FM_TitleMenuOpen();
        ClearFrames(false);
        if (g_pauseMenu != 0) {
            if (g_storyStarted == 0) {
                FM_UnloadTextures();
            }
            g_menu20C32F7 = 1;
            g_pauseMenu = 0;
        }
        if (g_inTitle != 0 && g_storyStarted == 0) {
            g_screen = g_titleScreenIds[0];
            ScreenCall(g_background, 0xC);
        }
        FM_PadStopRumble();
        FM_StartUserPause();
        return;
    }

    if (g_pauseMenu != 0 && (g_screen == 8 || g_screen == 0x15)) {
        return;
    }
    uint8_t back = Input_BackPressed(kUserInput, 0);

    // start: the pause menu toggled
    bool toggle = false;
    bool check = true;
    if (g_noPauseC3CD0C != 0 && g_pauseMenu == 0) {
        check = false;
    }
    if (check) {
        bool start = FM_PadJustDown(5, 0) && (g_storyStarted != 0 || !FM_Fn42E430());
        if (start || (back != 0 && g_pauseMenu != 0 && g_menuActive == 0)) {
            if (g_otherMenu == 0) {
                if (g_pauseMenu != 0) {
                    toggle = true;
                } else if (g_blockC1A99A == 0 && g_blockC1A999 == 0 && g_blockC1A998 == 0) {
                    toggle = true;
                }
            }
        }
    }
    if (!toggle && g_returnToMain != 0 && g_pauseMenu != 0) {
        toggle = true;
    }
    if (toggle && g_inTitle == 0 && !Screen4Busy()) {
        FM_PlaySound(kAudio, 0);
        g_soundsOn[0] = 0;
        g_pauseMenu = g_pauseMenu == 0;
        ClearFrames(true);
        if (g_pauseMenu == 0) {
            FM_PlayMenuSound();
            if (g_menuActive != 0) {
                ScreenCall(g_screens[g_screen], 0x10);
                g_screen = -1;
            }
            g_menuActive = 0;
            ScreenCall(g_background, 0x10);
            ClearPlayerInputs();
            FM_UnloadTextures();
            ResumeGame();
        } else if (g_otherMenu != 0) {
            if (g_menuActive != 0) {
                ScreenCall(g_screens[g_screen], 0x10);
            }
            FM_OtherMenuOpen();
            g_otherMenu = 1;
            ScreenCall(g_screens[g_screen], 0xC);
        } else {
            FM_LoadTextures();
            FM_PauseMenuOpen();
            ScreenCall(g_background, 0xC);
            FM_PadStopRumble();
            FM_StartUserPause();
            FM_Nothing49A650(1);
        }
    }

    // select (or back in the map menu): the map menu toggled
    void* radar = g_radar;
    if (g_noPauseC3CD0C == 0 || g_otherMenu != 0) {
        bool select = FM_PadJustDown(4, 0) != 0;
        uint8_t other = g_otherMenu;
        if ((select && (g_storyStarted != 0 || !FM_Fn42E430())) ||
            (back != 0 && other != 0 && At<int32_t>(radar, 0xE788) != 2)) {
            if (g_pauseMenu == 0 &&
                (other != 0 || (g_blockC1A99A == 0 && g_blockC1A999 == 0 && g_blockC1A998 == 0)) &&
                g_inTitle == 0 && !Screen4Busy()) {
                FM_PlaySound(kAudio, 0);
                g_soundsOn[0] = 0;
                g_otherMenu = g_otherMenu == 0;
                if (g_otherMenu == 0) {
                    FM_PlayMenuSound();
                    void* screen = g_screens[g_screen];
                    g_menuActive = 0;
                    ScreenCall(screen, 0x10);
                    g_screen = -1;
                    ScreenCall(g_background, 0x10);
                    ClearPlayerInputs();
                    FM_UnloadTextures();
                    ResumeGame();
                } else {
                    FM_LoadTextures();
                    FM_OtherMenuOpen();
                    ScreenCall(g_background, 0xC);
                    ScreenCall(g_screens[g_screen], 0xC);
                    FM_PadStopRumble();
                    FM_StartUserPause();
                }
            }
        }
    }

    // start in the map menu: over to the pause menu
    if (FM_PadJustDown(5, 0) && g_otherMenu != 0) {
        FM_PlaySound(kAudio, 0);
        void* screen = g_screens[g_screen];
        g_soundsOn[0] = 0;
        g_otherMenu = 0;
        ScreenCall(screen, 0x10);
        g_screen = -1;
        g_menuActive = 0;
        FM_PauseMenuOpen();
        g_pauseMenu = 1;
        g_screen = g_pauseFirstScreen;
    }

    // select in the pause menu: over to the map menu
    if (!FM_PadJustDown(4, 0) || g_pauseMenu == 0 || Screen4Busy()) {
        return;
    }
    FM_PlaySound(kAudio, 0);
    g_soundsOn[0] = 0;
    g_pauseMenu = 0;
    if (g_menuActive != 0) {
        ScreenCall(g_screens[g_screen], 0x10);
    }
    FM_OtherMenuOpen();
    g_otherMenu = 1;
    ScreenCall(g_screens[g_screen], 0xC);
}

// 0x6A4080 (FrontEnd vf1): Load: the 12 colours ("(r, g, b, a)"; an unparsed one takes
// whatever was on the stack), 9 floats, the key-repeat delays, the pause menu's list, the
// title and map menus' tabs
void __fastcall FrontEnd_Load(void*, re::Edx, void* archive) {
    struct StringBuffer {
        re::Ptr32<char> text;
        int32_t size;
        char buffer[32];
    } name;
    name.text = name.buffer;
    name.size = 0x1F;
    name.buffer[0] = 0;
    int r, g, b, a;  // (uninitialised, as in the game, when the text doesn't parse)
    for (int i = 0; i < 12; ++i) {
        VCall<void>(archive, 0x24, &name, 0x1F, FM_ColourName(i));
        FM_Sscanf(name.text, "(%d, %d, %d, %d)", &r, &g, &b, &a);
        uint8_t colour[4];
        uint8_t* c = FM_RGBA(colour, r, g, b, a);
        g_feColours[i * 4 + 0] = c[0];
        g_feColours[i * 4 + 1] = c[1];
        g_feColours[i * 4 + 2] = c[2];
        g_feColours[i * 4 + 3] = c[3];
    }
    float number;  // (a failed read keeps the previous one; the first: uninitialised)
    for (int i = 0; i < 9; ++i) {
        float& value = number;
        VCall<void>(archive, 0x28, &value, FM_FloatName(i));
        g_feFloats[i] = value;
    }
    // (read into the argument's slot: a failed read keeps the archive pointer's bits)
    int32_t value = int32_t(reinterpret_cast<uintptr_t>(archive));
    VCall<void>(archive, 0x38, &value, "TimeInputDelayedFirst");
    g_repeatFirstDelay = uint16_t(value);
    VCall<void>(archive, 0x38, &value, "TimeInputDelayed");
    g_repeatDelay = uint16_t(value);
    g_repeatTime[0] = 0;
    g_repeatTime[1] = 0;
    g_repeatTime[2] = 0;
    g_repeatTime[3] = 0;
    ScrollList_Load(kStartList, archive);
    FM_SelListLoadNamed(kTitleTabs, archive, "Title");
    SelList_Load(kOtherTabs, archive);
}

// 0x6A41F0: the pause menu opened straight on screen 8 (the save screen)
void __cdecl OpenOnScreen8() {
    FM_PlaySound(kAudio, 0);
    g_soundsOn[0] = 0;
    FM_LoadTextures();
    g_screen = 8;
    ScreenCall(g_screen8, 0xC);
    ScreenCall(g_background, 0xC);
    g_menuKind = 4;
    FM_PadStopRumble();
    FM_StartUserPause();
    g_pauseMenu = 1;
    g_menuActive = 1;
}

// 0x6A4250: and closed
void __cdecl CloseMenus() {
    void* screen = g_screens[g_screen];
    g_menuActive = 0;
    g_pauseMenu = 0;
    ScreenCall(screen, 0x10);
    g_screen = -1;
    ScreenCall(g_background, 0x10);
    FM_UnloadTextures();
    FM_EndUserPause();
    FM_GenerateBoundingBoxes(0);
}

// 0x6A42A0: a texture drawn at a position
void __cdecl DrawAtPosition(void* position, void* texture, const uint8_t* colour, const float* uv1,
                            const float* uv2) {
    float left = FM_PosLeft(position);
    float top = FM_PosTop(position);
    float height = At<float>(position, 0x10) * g_screenScaleY;
    float bottom = height + top;
    float width = At<float>(position, 0xC) * g_screenScaleX;
    float right = width + left;
    DrawRectUV(left, top, right, bottom, texture, colour, uv1, uv2);
}

} // namespace femgr

#define FEMGR_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(femgr_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(femgr_original_, __LINE__), \
             re::Mode::Original)

FEMGR_REVERSED("FrontEnd::CurrentScreenTitle", 0x006A2320, &femgr::CurrentScreenTitle);
FEMGR_REVERSED("FrontEnd::RequestSound", 0x006A2340, &femgr::RequestSound);
FEMGR_REVERSED("FrontEnd::RequestSoundDefault", 0x006A2360, &femgr::RequestSoundDefault);
FEMGR_REVERSED("FrontEnd::SoundsOn", 0x006A2380, &femgr::SoundsOn);
FEMGR_REVERSED("FrontEnd::SoundsOnNoPending", 0x006A2390, &femgr::SoundsOnNoPending);
FEMGR_REVERSED("FrontEnd::SoundsOff", 0x006A23A0, &femgr::SoundsOff);
FEMGR_REVERSED("FrontEnd::LoadLayouts", 0x006A23B0, &femgr::LoadLayouts);
FEMGR_REVERSED("FrontEnd::SetRenderStates", 0x006A2450, &femgr::SetRenderStates);
FEMGR_REVERSED("FrontEnd::FillTitleTabs", 0x006A24B0, &femgr::FillTitleTabs);
FEMGR_REVERSED("FrontEnd::FillOtherTabs", 0x006A2500, &femgr::FillOtherTabs);
FEMGR_REVERSED("FrontEnd::OtherMenuOpen", 0x006A2570, &femgr::OtherMenuOpen);
FEMGR_REVERSED("FrontEnd::SetScreen", 0x006A25B0, &femgr::SetScreen);
FEMGR_REVERSED("FrontEnd::GetTexture", 0x006A2600, &femgr::GetTexture);
FEMGR_REVERSED("FrontEnd::SetStoryStarted", 0x006A2620, &femgr::SetStoryStarted);
FEMGR_REVERSED("FrontEnd::AnchorFromName", 0x006A2630, &femgr::AnchorFromName);
FEMGR_REVERSED("FEPosition::GetX", 0x006A2730, &femgr::Position_GetX);
FEMGR_REVERSED("FEPosition::GetY", 0x006A2790, &femgr::Position_GetY);
FEMGR_REVERSED("FEPosition::GetLeft", 0x006A2800, &femgr::Position_GetLeft);  // (0x6A42A0 keeps edx across it)
FEMGR_REVERSED("FEPosition::GetTop", 0x006A2890, &femgr::Position_GetTop);    // (0x6A42A0 keeps edx across it)
FEMGR_REVERSED("FEPosition::GetWidth", 0x006A2920, &femgr::Position_GetWidth);
FEMGR_REVERSED("FEPosition::GetHeight", 0x006A2940, &femgr::Position_GetHeight);
FEMGR_REVERSED("FENameList::Add", 0x006A2960, &femgr::NameList_Add);
FEMGR_REVERSED("FENameList::Next", 0x006A2990, &femgr::NameList_Next);
FEMGR_REVERSED("FENameList::Previous", 0x006A29B0, &femgr::NameList_Previous);
FEMGR_REVERSED("FrontEnd::ScreenWidth", 0x006A29C0, &femgr::ScreenWidth);
FEMGR_REVERSED("FrontEnd::ScreenHeight", 0x006A29E0, &femgr::ScreenHeight);
FEMGR_REVERSED("FEScrollingSelectionListWindowT::GetItem", 0x006A29F0, &femgr::ScrollList_GetItem);
FEMGR_REVERSED("FrontEnd::LoadRamps", 0x006A2A30, &femgr::LoadRamps);
FEMGR_REVERSED("FrontEnd::LoadTextures", 0x006A2B00, &femgr::LoadTextures);
FEMGR_REVERSED("FrontEnd::UnloadTextures", 0x006A2D50, &femgr::UnloadTextures);
FEMGR_REVERSED("FrontEnd::DrawBackdrop", 0x006A2E00, &femgr::DrawBackdrop);
FEMGR_REVERSED("FrontEnd::DrawMessage", 0x006A2FF0, &femgr::DrawMessage);
FEMGR_REVERSED("FrontEnd::PlaySoundForPad", 0x006A30D0, &femgr::PlaySoundForPad);
FEMGR_REVERSED("FrontEnd::ClearPad", 0x006A3120, &femgr::ClearPad);  // (regcheck: 0x6A5F20 keeps edx across it)
FEMGR_REVERSED("FrontEnd::PadRepeatReady", 0x006A3170, &femgr::PadRepeatReady);
FEMGR_REVERSED("FrontEnd::PlayMenuSound", 0x006A31B0, &femgr::PlayMenuSound);
FEMGR_REVERSED("FrontEnd::FillStartList", 0x006A3390, &femgr::FillStartList);
FEMGR_REVERSED("FrontEnd::TitleMenuOpen", 0x006A3580, &femgr::TitleMenuOpen);
FEMGR_REVERSED("FrontEnd::PauseMenuOpen", 0x006A35E0, &femgr::PauseMenuOpen);
FEMGR_REVERSED("FrontEnd::UpdateMenus", 0x006A3610, &femgr::UpdateMenus);
FEMGR_REVERSED("FrontEnd::Load", 0x006A4080, &femgr::FrontEnd_Load);
FEMGR_REVERSED("FrontEnd::OpenOnScreen8", 0x006A41F0, &femgr::OpenOnScreen8);
FEMGR_REVERSED("FrontEnd::CloseMenus", 0x006A4250, &femgr::CloseMenus);
FEMGR_REVERSED("FrontEnd::DrawAtPosition", 0x006A42A0, &femgr::DrawAtPosition);
