// The frontend's stats (FEStats), status / report card (FEStatus), story mode (FEStoryMode)
// and tasks (FETasks) screens and their list items. 0x69C7F0..0x69F736.
//
// FEStats: +0xC / +0x28 heading and stat styles (their addresses kept at 0x20C308C /
//   0x20C3088 for the items), +0x44 the tabs (FESelectionListWindowT<0xC>), +0xE4 [12] each
//   tab's first stats-layout row, +0x114 the list (<0xA5>), +0x410 the items (0xA5 of 0x4C:
//   +8 the layout row, +0xC its value's text), +0x414 their count.
// FEStatus: +0xC the list (<0x71>), +0x238 the report card item (CStatusItem: +0x10 the
//   completion, +0x14 [10] the classes' names, +0x3C [6] (faction, name), +0x6C [10] the
//   classes' mission keys; a CCompletedHeader at +8 of it), +0x2CC a header, +0x2D8 [0x6E]
//   completed tasks (0xC: +8 the mission), +0x800 their count, +0x804 "none".
// FETasks: +0xC / +0x28 / +0x44 / +0x60 the chapter, season, completion and header styles,
//   +0x7C the list (<0x32>), +0x1AC the completion, +0x1B0 / +0x1C0 titles, +0x1D0 the task
//   title, +0x1DC [50] available tasks (0x10: +8 the mission, +0xC its blip), +0x4FC their
//   count, +0x500 [10] objectives (0xC: +8 the objective), +0x578 their count, +0x57C /
//   +0x588 / +0x594 "none" items.

#include <cmath>

#include "game/FEScreensCommon.h"
#include "game/Frontend.h"

using namespace fescr;

namespace {

constexpr uintptr_t VT_FEStats = 0x0094052C;
constexpr uintptr_t VT_SelListTC = 0x00940514;
constexpr uintptr_t VT_ScrollListTA5 = 0x0094051C;
constexpr uintptr_t VT_CStatItem = 0x009404F8;
constexpr uintptr_t VT_FEStatus = 0x00940628;
constexpr uintptr_t VT_ScrollListT71 = 0x00940618;
constexpr uintptr_t VT_CStatusItem = 0x00940608;
constexpr uintptr_t VT_CCompletedHeader = 0x00940578;
constexpr uintptr_t VT_CCompletedTask = 0x00940584;
constexpr uintptr_t VT_CNoCompletedTasks = 0x00940590;
constexpr uintptr_t VT_FEStoryMode = 0x00940784;
constexpr uintptr_t VT_FETasks = 0x0094085C;
constexpr uintptr_t VT_ScrollListT32 = 0x0094084C;
constexpr uintptr_t VT_CTitle = 0x009407B8;
constexpr uintptr_t VT_CTaskTitle = 0x009407C8;
constexpr uintptr_t VT_CAvailableTask = 0x009407E8;
constexpr uintptr_t VT_CObjective = 0x009407F8;
constexpr uintptr_t VT_CNoItems = 0x009407D8;

void* const kWindowColour = reinterpret_cast<void*>(0x020C33E4);

// the stats layout (8-byte rows: +0 id, +2 kind, +4 name hash)
uint8_t* const kLayoutItems = reinterpret_cast<uint8_t*>(0x00C193E8);
GAME_VAR(uint16_t, g_layoutCount, 0x00C1A748);
GAME_FN(uint8_t, __thiscall, Layout_IsHeader, 0x00458420, void* row);
GAME_FN(uint8_t, __thiscall, Layout_IsTitle, 0x00458430, void* row);
GAME_FN(uint8_t, __thiscall, Layout_IsStat, 0x00458440, void* row);
GAME_FN(void, __thiscall, Stats_ShowItem, 0x0045AFD0, void* row, uint16_t* out, int size);
GAME_VAR(uint8_t*, g_statStyle, 0x020C3088);
GAME_VAR(uint8_t*, g_headingStyle, 0x020C308C);
void* const kButtonMap = reinterpret_cast<void*>(0x00A14810);
GAME_FN(const int32_t*, __thiscall, ButtonMap_GetKeys, 0x00454CD0, void* map, int saved);

// the missions
GAME_FN(int32_t, __thiscall, MissionMgr_GetCount, 0x006AA140, void* mgr);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetTaskFlags, 0x006AA7C0, void* mgr, int32_t mission);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetInfo, 0x006AA660, void* mgr, int32_t mission);
GAME_FN(void, __thiscall, MissionMgr_6AAF10, 0x006AAF10, void* mgr, int32_t mission);
GAME_FN(int32_t, __thiscall, MissionMgr_GetBlip, 0x006AB0A0, void* mgr, int32_t mission);
GAME_FN(int32_t*, __thiscall, MissionMgr_GetCurrent, 0x006AA680, void* mgr);
GAME_FN(uint8_t, __thiscall, Mission_IsRunning, 0x006AA3F0, void* mission);
GAME_FN(uint8_t*, __thiscall, MissionMgr_FindByKey, 0x006AAF20, void* mgr, const void* key, uintptr_t);
GAME_FN(uint8_t, __thiscall, MissionMgr_IsCompleted, 0x006AB020, void* mgr, int32_t mission);
GAME_FN(int32_t, __thiscall, MissionMgr_Find, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* key);
void* const kObjectives = reinterpret_cast<void*>(0x020C4420);
GAME_FN(void*, __thiscall, Objectives_Get, 0x006AE080, void* mgr, int32_t id);
GAME_FN(void, __thiscall, Objective_GetText, 0x006ADF90, void* objective, uint16_t* out, int size);
GAME_FN(void, __thiscall, Objectives_BeginIterate, 0x006AE0E0, void* mgr);
GAME_FN(void*, __thiscall, Objectives_NextPrimary, 0x006AE100, void* mgr, int32_t* out);   // (the objective, or nullptr: all of eax counts)
GAME_FN(void*, __thiscall, Objectives_NextSecondary, 0x006AE150, void* mgr, int32_t* out);

// the player's grades and respect (a thiscall on the object at 0xC1AEA8)
void* const kPlayerStats = reinterpret_cast<void*>(0x00C1AEA8);
GAME_ARRAY(int32_t, g_grades, 0x00C1B02C);
GAME_FN(uint8_t, __thiscall, Player_GradeImproving, 0x00446710, void* stats, int cls);
GAME_FN(int32_t, __thiscall, Player_GetRespect, 0x00446780, void* stats, int faction);
GAME_VAR(void*, g_gradeStarTexture, 0x00C66F54);
GAME_VAR(void*, g_gradeNextTexture, 0x00C66F58);
GAME_VAR(float, g_completion, 0x020C3304);
GAME_VAR(uint32_t, g_hashCompletion, 0x020C3094);
GAME_VAR(uint32_t, g_hashTaskTitle, 0x020C30A0);
GAME_VAR(uint32_t, g_hashNoCompleted, 0x020C30A4);
GAME_VAR(uint32_t, g_statusBackground, 0x020C3090);
GAME_VAR(uint32_t, g_bandColour, 0x020C3348);
GAME_VAR(uint32_t, g_colourGrade, 0x020C3328);
GAME_VAR(uint32_t, g_colourOutline, 0x020C333C);
GAME_VAR(uint32_t, g_colourMenu0, 0x020C331C);
GAME_VAR(uint8_t, g_chapter, 0x00A147E0);
GAME_VAR(float, g_fontRightEdge, 0x0208B774);
GAME_VAR(float, g_fontWrapX, 0x0208B778);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_ARRAY(uint8_t, g_menuColours, 0x020C331C);

// drawing
GAME_FN(void, __cdecl, FE_SetTextStyle, 0x006A3FC0, int colour, int font, int justify, int);
GAME_FN(float, __cdecl, Style_LineHeight, 0x006A5430, const void* style);
GAME_FN(void, __cdecl, FE_DrawStyled, 0x006A6680, float x, float y, const uint16_t* text, const void* style);
GAME_FN(void, __cdecl, FE_DrawTextStyled, 0x006A6460, int x, int y, const uint16_t* text, const void* style);
GAME_FN(void, __cdecl, FE_DrawTextKey, 0x006A6380, int x, int y, const char* key, int font, int justify, int, int);
GAME_FN(void, __cdecl, FE_DrawFrameRect, 0x006A4420, float left, float top, float right, float bottom, float width,
        const void* inner, const void* border);
GAME_FN(void, __cdecl, FE_DrawBackground, 0x006A2FF0, uint32_t which);
GAME_FN(void, __cdecl, FE_PlayMove, 0x006A2380);
GAME_FN(void, __cdecl, FE_PlayBlocked, 0x006A23A0);
GAME_FN(void, __cdecl, FE_6A2620, 0x006A2620);
GAME_FN(void, __cdecl, Font_Measure, 0x0068B090, float* rect, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, Font_GetExtent, 0x00688EA0, float* rect, uintptr_t);
GAME_FN(void, __cdecl, Font_AddText, 0x006880F0, const uint16_t* text);
GAME_FN(void, __cdecl, Font_ShowCentredMessage, 0x006881B0, const char* key, float time, float scale);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const float* rect, const void* colour);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void*, __thiscall, Radar_GetBlipSprite, 0x00548E50, void* radar, int32_t blip);
GAME_FN(void, __thiscall, Sprite_Draw, 0x0052C6F0, void* sprite, const float* rect, const void* colour);
GAME_FN(void, __thiscall, SelList_Construct, 0x0069C3A0, void* list, void* entries, int count);
GAME_FN(void, __thiscall, ScrollList_Construct, 0x0069B1C0, void* list, int unused);
GAME_FN(int, __thiscall, SelList_Add, 0x0069B900, void* list, uint32_t entry);
GAME_FN(uint8_t, __thiscall, ScrollList_NudgeUp, 0x0069B5A0, void* list);
GAME_FN(uint8_t, __thiscall, ScrollList_NudgeDown, 0x0069B480, void* list);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, int size, int count, void* ctor);
GAME_FN(void, __thiscall, Stats_Rebuild, 0x0069CA20, void* screen);
GAME_FN(void, __thiscall, Status_DrawBand, 0x0069D610, void* item, float x, float y, const char* key);
GAME_FN(void, __thiscall, Status_DrawGrade, 0x0069D690, void* item, float x, float y, int cls);
GAME_FN(void, __thiscall, Tasks_DrawCompletion, 0x0069E860, void* screen);
GAME_FN(void, __thiscall, Tasks_BuildTasks, 0x0069EE70, void* screen);
GAME_FN(void, __thiscall, Tasks_BuildObjectives, 0x0069F120, void* screen);
GAME_FN(void, __thiscall, Camera_Fade, 0x004F1AD0, void* camera, float time, int direction, int);
void* const kCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_FN(void, __cdecl, SaveSys_737AB0, 0x00737AB0);
GAME_FN(void, __thiscall, Sound_597610, 0x00597610, void* audio);
GAME_FN(void*, __cdecl, MoviePlayer_Get, 0x00411370);
GAME_FN(void, __cdecl, Game_42E710, 0x0042E710);
GAME_FN(void, __cdecl, Game_42FA20, 0x0042FA20);
GAME_FN(void, __thiscall, Ped_474C10, 0x00474C10, void* ped, int, int);
GAME_VAR(uint8_t*, g_uberCompletion, 0x020CE75C);
GAME_VAR(uint8_t, g_storyIntro, 0x009D3450);
GAME_VAR(int32_t, g_movePlayer, 0x00BD0FD4);
GAME_VAR(uint8_t, g_gameReady, 0x00BD0FCB);
void* const kAudio = reinterpret_cast<void*>(0x00CF3DA0);
void* const kAttractMovie = reinterpret_cast<void*>(0x020C3220);

void InitStyle(uint8_t* style) {
    At<int32_t>(style, 0) = 0;
    At<int32_t>(style, 4) = 3;
    At<float>(style, 8) = 0.0f;
    At<float>(style, 0xC) = 0.0f;
    At<float>(style, 0x10) = 0.0f;
    At<int32_t>(style, 0x14) = 2;
    At<uint8_t>(style, 0x18) = 1;
    At<uint8_t>(style, 0x19) = 0;
    At<uint8_t>(style, 0x1A) = 0;
}

// a text style on the stack (font, justification, colour; flags 1, 0, 0 or 0, 0, 0)
struct LocalStyle {
    int32_t font, justify;
    float dx, dy, unknown10;
    int32_t colour;
    uint8_t flags[3];
};
LocalStyle MakeStyle(int font, int justify, int colour, uint8_t flag) {
    LocalStyle style = {font, justify, 0.0f, 0.0f, 0.0f, colour, {flag, 0, 0}};
    return style;
}

// the style's colour, font and justification set
void UseStyle(const uint8_t* style) {
    FE_SetTextStyle(At<int32_t>(const_cast<uint8_t*>(style), 0x14), At<int32_t>(const_cast<uint8_t*>(style), 0),
                    At<int32_t>(const_cast<uint8_t*>(style), 4), 0);
}

// between the status window's bars (110 .. 394)
bool Visible(float top, float bottom) {
    return !(110.0f > top) && !(394.0f < bottom);
}

void InitExtent(float* rect) {
    rect[0] = 1e6f;
    rect[1] = -1e6f;
    rect[2] = -1e6f;
    rect[3] = 1e6f;
}

} // namespace

namespace fest {

// ---- FEStats ---------------------------------------------------------------------------------

// 0x69C7F0 (vf1)
void __fastcall Stats_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    TextStyle_Load(self + 0xC, archive, "HeadingStyle");
    TextStyle_Load(self + 0x28, archive, "StatStyle");
    SelList_Load(self + 0x44, archive);
    ScrollList_Load(self + 0x114, archive);
}

// 0x69C840 (not in the function list; the items' constructor for the vector)
void* __fastcall StatItem_Construct(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 4) = 1;
    SetVtable(self, VT_CStatItem);
    return self;
}

// 0x69C880 (vf6)
void __fastcall Stats_Draw(uint8_t* self, re::Edx) {
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    FE_DrawWindow(320.0f, 440.0f, 394.0f, kWindowColour, 2.0f);
    SelList_Draw(self + 0x44);
    ScrollList_Draw(self + 0x114, 0.0f, 0.0f, 0);
    FE_DrawTitle("FEST_CONTROLS");
}

// 0x69C980 (not in the function list): FEStats' constructor
void* __fastcall Stats_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEStats);
    InitStyle(self + 0xC);
    InitStyle(self + 0x28);
    SelList_Construct(self + 0x44, self + 0x44 + 0x70, 0xC);
    SetVtable(self + 0x44, VT_SelListTC);
    ScrollList_Construct(self + 0x114, 0xA5);
    SetVtable(self + 0x114, VT_ScrollListTA5);
    g_statStyle = self + 0x28;
    g_headingStyle = self + 0xC;
    return self;
}

// 0x69CA20: the tab's stats listed (from its row to the next header; row id 0x31 only
// for a pad or joystick bindings)
int __fastcall Stats_RebuildImpl(uint8_t* self, re::Edx) {
    void* list = self + 0x114;
    ScrollList_Clear(list);
    At<int32_t>(self, 0x414) = 0;
    int8_t tab = SelList_Selected(self + 0x44);
    int result = 0;
    for (int row = At<int32_t>(self, 0xE4 + tab * 4) + 1; row < int(g_layoutCount); row++) {
        uint8_t* item = kLayoutItems + row * 8;
        uint8_t header = Layout_IsHeader(item);
        result = header;
        if (header != 0) {
            return result;
        }
        if (At<int16_t>(item, 0) == 0x31 && Pad_IsXInput(GetActivePad()) == 0 &&
            ButtonMap_GetKeys(kButtonMap, 0)[17] < 0x10A && ButtonMap_GetKeys(kButtonMap, 0)[21] < 0x10A) {
            continue;
        }
        if (Layout_IsTitle(item) == 0 && Layout_IsStat(item) == 0) {
            continue;
        }
        int32_t count = At<int32_t>(self, 0x414);
        if (count >= 0xA5) {
            continue;
        }
        uint8_t* statItem = At<uint8_t*>(self, 0x410) + count * 0x4C;
        At<int32_t>(self, 0x414) = count + 1;
        At<uint8_t*>(statItem, 8) = item;
        if (Layout_IsHeader(item) == 0 && Layout_IsTitle(item) == 0) {
            At<uint8_t>(statItem, 4) = 0;
            Stats_ShowItem(item, reinterpret_cast<uint16_t*>(statItem + 0xC), 0x1F);
        } else {
            At<uint8_t>(statItem, 4) = 1;
        }
        ScrollList_AddItem(list, statItem);
    }
    return result;
}

// 0x69CBD0 (vf3): the items made, each tab's first row found, the first listed
void __fastcall Stats_OnEnter(uint8_t* self, re::Edx) {
    SelList_Clear(self + 0x44);
    void* memory = Malloc(0x30FC);
    if (memory != nullptr) {
        VectorConstruct(memory, 0x4C, 0xA5, reinterpret_cast<void*>(0x0069C840));
    }
    At<void*>(self, 0x410) = memory;
    for (int row = 0; row < int(g_layoutCount); row++) {
        uint8_t* item = kLayoutItems + row * 8;
        if (Layout_IsHeader(item) != 0) {
            int8_t tab = int8_t(SelList_Add(self + 0x44, At<uint32_t>(item, 4)));
            At<int32_t>(self, 0xE4 + tab * 4) = row;
        }
    }
    Stats_Rebuild(self);
}

// 0x69CC60 (vf5): tabs by left/right, scrolling by up/down
void __fastcall Stats_Process(uint8_t* self, re::Edx) {
    if (Input_BackPressed(kUserInput, 0) != 0) {
        FE_PlaySelect();
        g_menuActive = 0;
        return;
    }
    void* tabs = self + 0x44;
    bool changed = false;
    if (g_menuRight != 0 && FE_PadPressed(0) != 0) {
        int8_t before = SelList_Selected(tabs);
        changed = before != SelList_Next(tabs);
    } else if (g_menuLeft != 0 && FE_PadPressed(0) != 0) {
        int8_t before = SelList_Selected(tabs);
        changed = before != SelList_Previous(tabs);
    }
    if (changed) {
        Stats_Rebuild(self);
        FE_PadClear(0);
    }
    if (g_menuUp != 0 && ScrollList_NudgeUp(self + 0x114) != 0) {
        FE_PlaySelect();
        return;
    }
    if (g_menuDown != 0 && ScrollList_NudgeDown(self + 0x114) != 0) {
        FE_PlaySelect();
    }
}

// 0x69CD40 (vf0): a stat's extent (its name's height)
void __fastcall StatItem_GetExtent(uint8_t* self, re::Edx, float* rect, float x, float width) {
    rect[0] = x;
    rect[2] = x + width;
    rect[1] = 0.0f;
    rect[3] = 0.0f;
    uint8_t* row = At<uint8_t*>(self, 8);
    UseStyle(Layout_IsTitle(row) != 0 ? g_headingStyle : g_statStyle);
    float measured[4];
    InitExtent(measured);
    g_fontRightEdge = x + width;
    Font_Measure(measured, x + 5.0f, 0.0f, Text_GetByHash(kTheText, At<uint32_t>(row, 4)));
    rect[3] = std::fabs(measured[3] - measured[1]) + 0.0f + rect[3];
}

// 0x69CE30 (vf1): a title in the heading style, or a stat's name and value with the
// highlight between
void __fastcall StatItem_Draw(uint8_t* self, re::Edx, float x, float y, float width, float, int) {
    g_fontDropColour[0] = g_menuColours[0];
    g_fontDropColour[1] = g_menuColours[1];
    g_fontDropColour[2] = g_menuColours[2];
    g_fontDropColour[3] = g_menuColours[3];
    g_fontDrop = 1;
    At<uint8_t>(g_headingStyle, 0x1A) = 1;
    uint8_t* row = At<uint8_t*>(self, 8);
    if (Layout_IsTitle(row) != 0) {
        FE_DrawStyled(x, y, Text_GetByHash(kTheText, At<uint32_t>(row, 4)), g_headingStyle);
        return;
    }
    if (Layout_IsStat(row) == 0) {
        return;
    }
    UseStyle(g_statStyle);
    float right = x + width;
    g_fontRightEdge = right;
    Font_PrintString(x + 5.0f, y, Text_GetByHash(kTheText, At<uint32_t>(row, 4)), 1);
    float name[4];
    InitExtent(name);
    Font_GetExtent(name, 0);
    Font_SetJustify(1);
    Font_PrintString(right - 5.0f, y, reinterpret_cast<uint16_t*>(self + 0xC), 1);
    float value[4];
    InitExtent(value);
    Font_GetExtent(value, 0);
    Font_Render();
    FE_DrawHighlight(5.0f + x + std::fabs(name[2] - name[0]), right - std::fabs(value[2] - value[0]), y);
}

// ---- FEStatus ------------------------------------------------------------------------------------

// 0x69D040 (vf1)
void __fastcall Status_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    ScrollList_Load(self + 0xC, archive);
}

// 0x69D060 (vf5): scrolling only
void __fastcall Status_Process(uint8_t* self, re::Edx) {
    if (g_menuUp != 0 && ScrollList_NudgeUp(self + 0xC) != 0) {
        FE_PlaySelect();
        return;
    }
    if (g_menuDown != 0 && ScrollList_NudgeDown(self + 0xC) != 0) {
        FE_PlaySelect();
    }
}

// 0x69D0A0 (vf6)
void __fastcall Status_Draw(uint8_t* self, re::Edx) {
    FE_DrawBackground(g_statusBackground);
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    FE_DrawWindow(320.0f, 440.0f, 394.0f, kWindowColour, 2.0f);
    ScrollList_Draw(self + 0xC, 0.0f, 0.0f, 1);
    FE_DrawTitle("FE_SELECT_C");
}

// 0x69D140: the report card item: its classes, factions and the classes' missions
void* __fastcall StatusItem_Construct(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 4) = 1;
    SetVtable(self, VT_CStatusItem);
    At<uint8_t>(self, 0xC) = 1;
    SetVtable(self + 8, VT_CCompletedHeader);
    At<uint32_t>(self, 0x14) = HashName("CN_ART");
    At<uint32_t>(self, 0x18) = HashName("CN_CHEM");
    At<uint32_t>(self, 0x1C) = HashName("CN_ENG");
    At<uint32_t>(self, 0x2C) = HashName("CN_GYM");
    At<uint32_t>(self, 0x30) = HashName("CN_PHOTO");
    At<uint32_t>(self, 0x34) = HashName("CN_SHOP");
    At<uint32_t>(self, 0x20) = HashName("CN_MAT");
    At<uint32_t>(self, 0x24) = HashName("CN_BIO");
    At<uint32_t>(self, 0x28) = HashName("CN_GEO");
    At<uint32_t>(self, 0x38) = HashName("CN_MUS");
    At<int32_t>(self, 0x3C) = 0xB;
    At<uint32_t>(self, 0x40) = HashName("STATUS_BULLIES");
    At<int32_t>(self, 0x44) = 1;
    At<uint32_t>(self, 0x48) = HashName("STATUS_NERDS");
    At<int32_t>(self, 0x4C) = 2;
    At<uint32_t>(self, 0x50) = HashName("STATUS_JOCKS");
    At<int32_t>(self, 0x54) = 5;
    At<uint32_t>(self, 0x58) = HashName("STATUS_PREPPIES");
    At<int32_t>(self, 0x5C) = 4;
    At<uint32_t>(self, 0x60) = HashName("STATUS_GREASERS");
    At<int32_t>(self, 0x64) = 3;
    At<uint32_t>(self, 0x68) = HashName("STATUS_DROPOUTS");
    At<uint32_t>(self, 0x6C) = 0x00C1A778;
    At<uint32_t>(self, 0x70) = 0x00C1A77C;
    At<uint32_t>(self, 0x74) = 0x00C1A770;
    At<uint32_t>(self, 0x84) = 0x00C1A768;
    At<uint32_t>(self, 0x88) = 0x00C1A76C;
    At<uint32_t>(self, 0x8C) = 0x00C1A774;
    At<uint32_t>(self, 0x78) = 0x00C1A784;
    At<uint32_t>(self, 0x80) = 0x00C1A78C;
    At<uint32_t>(self, 0x90) = 0x00C1A788;
    At<uint32_t>(self, 0x7C) = 0x00C1A780;
    return self;
}

// 0x69D2D0 (vf0) / 0x69D340 (vf1): a completed task's name
void __fastcall CompletedTask_GetExtent(uint8_t* self, re::Edx, float* rect, float x, float width) {
    FE_SetTextStyle(3, 0, 1, 0);
    g_fontRightEdge = x + width;
    uint8_t* info = MissionMgr_GetInfo(kMissionMgr, At<int32_t>(self, 8));
    Font_Measure(rect, x, 0.0f, Text_Get(kTheText, At<const char*>(info, 0x20)));
    rect[3] = rect[3] + 0.0f;
}
void __fastcall CompletedTask_Draw(uint8_t* self, re::Edx, float x, float y, float width, float, int) {
    uint8_t* info = MissionMgr_GetInfo(kMissionMgr, At<int32_t>(self, 8));
    FE_SetTextStyle(3, 0, 1, 0);
    g_fontRightEdge = x + width;
    Font_PrintString(x, y, Text_Get(kTheText, At<const char*>(info, 0x20)), 1);
    Font_Render();
}

// 0x69D3B0 (vf0) / 0x69D410 (vf1): "no completed tasks"
void __fastcall NoCompleted_GetExtent(void*, re::Edx, float* rect, float x, float width) {
    FE_SetTextStyle(0xB, 0, 1, 0);
    g_fontRightEdge = x + width;
    Font_Measure(rect, x, 0.0f, Text_GetByHash(kTheText, g_hashNoCompleted));
    rect[3] = 0.0f + rect[3];
}
void __fastcall NoCompleted_Draw(void*, re::Edx, float x, float y, float width, float, int) {
    FE_SetTextStyle(0xB, 0, 1, 0);
    g_fontRightEdge = x + width;
    Font_PrintString(x, y, Text_GetByHash(kTheText, g_hashNoCompleted), 1);
    Font_Render();
}

// 0x69D4A0 (not in the function list): FEStatus' constructor
void* __fastcall Status_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEStatus);
    ScrollList_Construct(self + 0xC, 0x71);
    SetVtable(self + 0xC, VT_ScrollListT71);
    StatusItem_Construct(self + 0x238, nullptr);
    At<uint8_t>(self, 0x2D0) = 1;
    SetVtable(self + 0x2CC, VT_CCompletedHeader);
    for (int i = 0; i < 0x6E; i++) {
        At<uint8_t>(self, 0x2D8 + i * 0xC + 4) = 1;
        SetVtable(self + 0x2D8 + i * 0xC, VT_CCompletedTask);
    }
    At<uint8_t>(self, 0x808) = 1;
    SetVtable(self + 0x804, VT_CNoCompletedTasks);
    return self;
}

// 0x69D520 (vf3): the report card, then the completed tasks (or "none")
void __fastcall Status_OnEnter(uint8_t* self, re::Edx) {
    void* list = self + 0xC;
    At<int32_t>(self, 0x800) = 0;
    ScrollList_Clear(list);
    At<float>(self, 0x248) = g_completion;
    ScrollList_AddItem(list, self + 0x238);
    for (int mission = 0; mission < MissionMgr_GetCount(kMissionMgr); mission++) {
        if (MissionMgr_IsCompleted(kMissionMgr, mission) == 0) {
            continue;
        }
        uint8_t* item = self + 0x2D8 + At<int32_t>(self, 0x800) * 0xC;
        At<int32_t>(self, 0x800) += 1;
        At<int32_t>(item, 8) = mission;
        ScrollList_AddItem(list, item);
    }
    if (At<int32_t>(self, 0x800) < 1) {
        ScrollList_AddItem(list, self + 0x804);
    }
}

// 0x69D610 (ecx unused): a darkened band across the window with a key's text at (x, y - 1)
void __fastcall StatusItem_DrawBandImpl(void*, re::Edx, float x, float y, const char* key) {
    uint8_t colour[4];
    RGBA_Construct(colour, 0x40, 0x40, 0x40, 0x80);
    float rect[4] = {100.0f, float(double(y) + 18.0), 540.0f, y};
    DrawRect(rect, colour);
    FE_DrawTextKey(int(x), int(float(double(y) - 1.0)), key, 3, 2, 1, 1);
}

// 0x69D690: a class's name and its grade as 5 cells (stars up to the grade, the next one
// flashing when it's improving, marks otherwise; the mark at the grade highlighted while
// the class's mission is open)
void __fastcall StatusItem_DrawGradeImpl(uint8_t* self, re::Edx, float x, float y, int cls) {
    bool open = false;
    uint8_t* mission = MissionMgr_FindByKey(kMissionMgr, At<void*>(self, 0x6C + cls * 4), 0);
    if (cls == 6 || cls == 7) {
        open = mission != nullptr;
    } else {
        open = mission != nullptr && At<uint8_t>(mission, 0x2C) == 0;
    }
    float scaleX = g_screenScaleX;
    float scaleY = g_screenScaleY;
    FE_DrawText(int(x), int(y), Text_GetByHash(kTheText, At<uint32_t>(self, 0x14 + cls * 4)), 8, 0, 1, 1);
    float width = float(double(scaleX) * 100.0);
    uint32_t outline = g_colourOutline;
    uint32_t grade = g_colourGrade;
    uint32_t shadow = g_colourMenu0;
    float height = float(double(scaleY) * 18.0);
    float top = float(double(y) + 22.0);
    float cell = width / 5.0f;
    float bottom = top + height;
    double right = double(x + width);
    // the shadow frame and dividers (one pixel down and right)
    uint8_t clear[4];
    RGBA_Construct(clear, 0, 0, 0, 0);
    FE_DrawFrameRect(x + 1.0f, top + 1.0f, float(right) + 1.0f, bottom + 1.0f, 2.0f, clear, &shadow);
    for (int i = 1; i <= 4; i++) {
        float cx = float(i) * cell + x;
        float rect[4] = {(cx - 1.0f) + 1.0f, top + 1.0f, cx + 1.0f + 1.0f, bottom + 1.0f};
        DrawRect(rect, &shadow);
    }
    RGBA_Construct(clear, 0, 0, 0, 0);
    FE_DrawFrameRect(x, top, float(right), bottom, 2.0f, clear, &grade);
    for (int i = 1; i <= 4; i++) {
        float cx = float(i) * cell + x;
        float rect[4] = {cx - 1.0f, top, cx + 1.0f, bottom};
        DrawRect(rect, &grade);
    }
    double firstCentre = double(cell * 0.5f + x);
    float cy = float(double(height * 0.5f + top) + 0.0);
    float halfWidth = scaleX * 16.0f * 0.5f;
    float halfHeight = scaleY * 16.0f * 0.5f;
    for (int i = 0; i < 5; i++) {
        float cx = float(double(float(i) * cell + float(firstCentre)) + 0.0);
        if (i < g_grades[cls]) {
            float uv2[2] = {1.0f, 1.0f};
            float uv1[2] = {0.0f, 0.0f};
            uint8_t white[4];
            RGBA_Construct(white, 255, 255, 255, 255);
            DrawRectUV(cx - halfWidth, cy - halfHeight, cx + halfWidth, cy + halfHeight, g_gradeStarTexture, white, uv1,
                       uv2);
        } else if (g_grades[cls] == i && Player_GradeImproving(kPlayerStats, cls) != 0) {
            float uv2[2] = {1.0f, 1.0f};
            float uv1[2] = {0.0f, 0.0f};
            uint8_t white[4];
            RGBA_Construct(white, 255, 255, 255, 255);
            DrawRectUV(cx - halfWidth, cy - halfHeight, cx + halfWidth, cy + halfHeight, g_gradeNextTexture, white, uv1,
                       uv2);
        } else {
            uint32_t colour = grade;
            if (open && g_grades[cls] == i) {
                colour = outline;
            }
            float left = cx - 5.0f;
            float markShadow[4] = {left + 1.0f, (cy - 1.0f) + 1.0f, cx + 5.0f + 1.0f, cy + 1.0f + 1.0f};
            DrawRect(markShadow, &shadow);
            float mark[4] = {left, cy - 1.0f, cx + 5.0f, cy + 1.0f};
            DrawRect(mark, &colour);
        }
    }
}

// 0x69DC40 (vf1): the report card: its heading, the periods, the classes' grades, the
// respect for each faction, then the completed tasks' header below
void __fastcall StatusItem_Draw(uint8_t* self, re::Edx, float x, float y, float width, float height, int selected) {
    float top = float(double(y) - 108.0);
    uint32_t band = g_bandColour;
    float heading = top + 108.0f;
    if (Visible(heading, heading)) {
        Status_DrawBand(self, 320.0f, heading, "REPORT_CARD");
    }
    double periods = double(top) + 132.0;
    if (Visible(float(periods), float(periods))) {
        int yInt = int(periods);
        FE_DrawTextKey(0xCD, yInt, "PER_MORN", 3, 2, 1, 1);
        float lineTop = float(periods + 21.0);
        float lineBottom = float(2.0 + (periods + 21.0));
        float morning[4] = {102.0f, lineTop, 308.0f, lineBottom};
        DrawRect(morning, &band);
        FE_DrawTextKey(0x1B3, yInt, "PER_ANOON", 3, 2, 1, 1);
        float afternoon[4] = {332.0f, lineTop, 538.0f, lineBottom};
        DrawRect(afternoon, &band);
    }
    double grades = periods + 24.0;
    static const int kMorning[5] = {1, 0, 8, 4, 5};
    static const int kAfternoon[5] = {2, 6, 7, 9, 3};
    for (int i = 0; i < 5; i++) {
        float row = float(double(i) * 42.0 + grades);
        if (Visible(row, row)) {
            Status_DrawGrade(self, 110.0f, row, kMorning[i]);
        }
    }
    for (int i = 0; i < 5; i++) {
        float row = float(double(i) * 42.0 + grades);
        if (Visible(row, row)) {
            Status_DrawGrade(self, 340.0f, row, kAfternoon[i]);
        }
    }
    LocalStyle bandStyle = MakeStyle(2, 1, 3, 1);
    float bandHeight = Style_LineHeight(&bandStyle);
    double respectTop = double(top + 376.0f);
    if (Visible(float(respectTop), float(respectTop) + bandHeight)) {
        Status_DrawBand(self, 320.0f, float(respectTop), "RESPECT");
    }
    bandStyle = MakeStyle(0, 1, 0xB, 1);
    float lineHeight = Style_LineHeight(&bandStyle);
    double respectRows = double(top + 400.0f);
    for (int i = 0; i < 6; i++) {
        float row = float(double(i % 3) * 18.0 + respectRows);
        if (!(110.0f <= row) || !(row + lineHeight <= 394.0f)) {
            continue;
        }
        float nameX = i < 3 ? 110.0f : 340.0f;
        float valueX = float(double(nameX) + 190.0);
        int yInt = int(row);
        FE_DrawText(int(nameX), yInt, Text_GetByHash(kTheText, At<uint32_t>(self, 0x3C + i * 8 + 4)), 3, 0, 1, 0);
        int32_t respect = Player_GetRespect(kPlayerStats, At<int32_t>(self, 0x3C + i * 8));
        int colour = 6;
        if (respect < 0x32) {
            colour = 5;
        } else if (respect < 0x4B) {
            colour = 7;
        }
        char text[16];
        uint16_t wide[16];
        Sprintf(text, "%d%%", respect);
        AsciiToWide(text, wide);
        FE_DrawText(int(valueX), yInt, wide, colour, 1, 1, 1);
    }
    float next = float(float(respectTop + 24.0 + 9.0) + 54.0);
    VCall<void>(self + 8, 4, x, next, width, height, selected);
}

// 0x69E180 (vf0): the completion line and "completed tasks"' heights
void __fastcall CompletedHeader_GetExtent(uint8_t* self, re::Edx, float* rect, float x, float width) {
    float measured[4];
    InitExtent(measured);
    Font_AddNumber(At<float>(self, 8));
    uint16_t text[0x200];
    Font_Format(text, Text_GetByHash(kTheText, g_hashCompletion), 0x1FF);
    rect[0] = x;
    rect[2] = x + width;
    rect[1] = 0.0f;
    rect[3] = 0.0f;
    FE_SetTextStyle(3, 2, 1, 0);
    g_fontWrapX = width;
    Font_Measure(measured, x, 0.0f, text);
    rect[3] = std::fabs(measured[3] - measured[1]) + 0.0f + rect[3];
    FE_SetTextStyle(3, 0, 3, 0);
    Font_Measure(measured, x, 0.0f, Text_Get(kTheText, "FE_COMP_TASKS"));
    rect[3] = std::fabs(measured[3] - measured[1]) + rect[3];
}

// 0x69E2D0 (vf1): the completion (centred) and "completed tasks" below, inside the window
void __fastcall CompletedHeader_Draw(uint8_t* self, re::Edx, float x, float y, float width, float, int) {
    LocalStyle style = MakeStyle(2, 1, 3, 1);
    float lineHeight = Style_LineHeight(&style);
    if (!(110.0f > y) && !(394.0f < y + lineHeight)) {
        Font_AddNumber(At<float>(self, 8));
        uint16_t text[0x200];
        Font_Format(text, Text_GetByHash(kTheText, g_hashCompletion), 0x1FF);
        FE_SetTextStyle(3, 2, 1, 0);
        Font_PrintString(width * 0.5f + x, y, text, 1);
    }
    float extent[4];
    InitExtent(extent);
    Font_GetExtent(extent, 0);
    Font_Render();
    style = MakeStyle(0, 3, 3, 1);
    lineHeight = Style_LineHeight(&style);
    float below = std::fabs(extent[3] - extent[1]) + y + 0.0f;
    if (110.0f < below && below + lineHeight <= 394.0f) {
        LocalStyle heading = MakeStyle(0, 3, 3, 0);
        FE_DrawStyled(x, below, Text_Get(kTheText, "FE_COMP_TASKS"), &heading);
    }
}

// ---- FEStoryMode --------------------------------------------------------------------------------

// 0x69E500 (not in the function list)
void* __fastcall StoryMode_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEStoryMode);
    InitStyle(self + 0xC);
    return self;
}

// 0x69E540 (vf1)
void __fastcall StoryMode_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    TextStyle_Load(self + 0xC, archive, "TextStyle");
}

// 0x69E570 (vf3): back into the story: the fade, the intro (the first time: mission 1_01
// started, its intro message), the menu closed
void __fastcall StoryMode_OnEnter(void*, re::Edx) {
    Camera_Fade(kCamera, 0.0f, 1, 0);
    if (At<uint8_t>(g_uberCompletion, 0x270) == 0) {
        SaveSys_737AB0();
    }
    if (At<uint8_t>(g_uberCompletion, 0x270) == 0 && g_storyIntro != 0) {
        Sound_597610(kAudio);
        VCall<void>(MoviePlayer_Get(), 4, kAttractMovie, 2);
        uint32_t key;
        NameKey_Construct(&key, "1_01");
        int32_t mission = MissionMgr_Find(kMissionMgr, &key);
        NameKey_Destruct(&key);
        if (mission != -1) {
            g_movePlayer = 2;
            Game_42E710();
            At<uint8_t>(MissionMgr_GetInfo(kMissionMgr, mission), 0x33) = 1;
            uint8_t* info = MissionMgr_GetInfo(kMissionMgr, mission);
            const char* intro = At<const char*>(info, 0x20);
            if (At<uint8_t>(info, 0x24) != 0 && intro != nullptr && intro[0] != 0) {
                Font_ShowCentredMessage(intro, 5.0f, 2.0f);
            }
        }
    }
    g_specialEntitiesCreated = 0;
    g_menu20C32F6 = 1;
    g_menuActive = 0;
    FE_6A2620();
    g_gameReady = At<uint8_t>(g_uberCompletion, 0x270) == 0 ? 1 : 0;
    Game_42FA20();
    VCall<void>(g_radar, 0x14, 1);
    Ped_474C10(g_player, 1, 0);
}

// ---- FETasks ------------------------------------------------------------------------------------

// 0x69E6C0 (vf1)
void __fastcall Tasks_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    TextStyle_Load(self + 0xC, archive, "ChapterStyle");
    TextStyle_Load(self + 0x28, archive, "SeasonStyle");
    TextStyle_Load(self + 0x44, archive, "GameCompletionStyle");
    TextStyle_Load(self + 0x60, archive, "TasksHeaderStyle");
    ScrollList_Load(self + 0x7C, archive);
}

// 0x69E720 (vf5): on a mission: scrolling; otherwise accept toggles the task, up/down move
// (not while the list scrolls)
void __fastcall Tasks_Process(uint8_t* self, re::Edx) {
    void* list = self + 0x7C;
    if (Mission_IsRunning(MissionMgr_GetCurrent(kMissionMgr)) != 0) {
        if (g_menuUp != 0 && ScrollList_NudgeUp(list) != 0) {
            FE_PlaySelect();
            return;
        }
        if (g_menuDown != 0 && ScrollList_NudgeDown(list) != 0) {
            FE_PlaySelect();
        }
        return;
    }
    if (Input_AcceptDown(kUserInput, 0) != 0 && At<int32_t>(self, 0x4FC) > 0) {
        FE_PlaySelect();
        int32_t selected = At<int32_t>(list, 0xC);
        void* item = VCall<void*>(list, 4, selected);
        VCall<void>(item, 8);
        return;
    }
    int32_t before;
    int32_t after;
    if (g_menuUp != 0 && FE_PadPressed(0) != 0) {
        if (At<uint8_t>(list, 0x48) != 0 || 0.0f < At<float>(list, 0x60)) {
            FE_PlayBlocked();
            return;
        }
        before = At<int32_t>(list, 0xC);
        after = ScrollList_Previous(list);
    } else if (g_menuDown != 0 && FE_PadPressed(0) != 0) {
        if (At<uint8_t>(list, 0x48) != 0 || 0.0f < At<float>(list, 0x60)) {
            FE_PlayBlocked();
            return;
        }
        before = At<int32_t>(list, 0xC);
        after = ScrollList_Next(list);
    } else {
        return;
    }
    if (before != after) {
        FE_PlayMove();
        FE_PadClear(0);
    }
}

// 0x69E860: the game's completion
void __fastcall Tasks_DrawCompletionImpl(uint8_t* self, re::Edx) {
    Font_AddNumber(At<float>(self, 0x1AC));
    uint16_t text[0x200];
    Font_Format(text, Text_GetByHash(kTheText, g_hashCompletion), 0x1FF);
    FE_DrawTextStyled(0x140, 0x6C, text, self + 0x44);
}

// 0x69E8C0 (vf0): an available task's name
void __fastcall AvailableTask_GetExtent(uint8_t* self, re::Edx, float* rect, float x, float width) {
    FE_SetTextStyle(0xB, 0, 1, 0);
    g_fontRightEdge = x + width;
    uint8_t* info = MissionMgr_GetInfo(kMissionMgr, At<int32_t>(self, 8));
    Font_Measure(rect, x + 5.0f, 0.0f, Text_Get(kTheText, At<const char*>(info, 0x20)));
    rect[3] = rect[3] + 0.0f;
}

// 0x69E960 (vf0) / 0x69EA00 (vf1): an objective's text (done ones in colour 11)
void __fastcall Objective_GetExtent(uint8_t* self, re::Edx, float* rect, float x, float width) {
    FE_SetTextStyle(0xB, 0, 1, 0);
    g_fontRightEdge = x + width;
    void* objective = Objectives_Get(kObjectives, At<int32_t>(self, 8));
    uint16_t text[0x200];
    Objective_GetText(objective, text, 0x1FF);
    Font_Measure(rect, x + 5.0f, 0.0f, text);
    rect[3] = rect[3] + 0.0f;
}
void __fastcall Objective_Draw(uint8_t* self, re::Edx, float x, float y, float width, float, int) {
    void* objective = Objectives_Get(kObjectives, At<int32_t>(self, 8));
    uint16_t text[0x200];
    Objective_GetText(objective, text, 0x1FF);
    FE_SetTextStyle(At<uint8_t>(objective, 0x1D) != 0 ? 0xB : 3, 0, 1, 0);
    g_fontRightEdge = x + width;
    Font_PrintString(x + 5.0f, y, text, 1);
    Font_Render();
}

// 0x69EAA0 (vf1): a title in its style
void __fastcall Title_Draw(uint8_t* self, re::Edx, float x, float y, float, float, int) {
    FE_DrawStyled(x, y, Text_GetByHash(kTheText, At<uint32_t>(self, 0xC)), At<void*>(self, 8));
}

// 0x69EAD0 (vf1): the current mission's title
void __fastcall TaskTitle_Draw(uint8_t* self, re::Edx, float x, float y, float, float, int) {
    int32_t* current = MissionMgr_GetCurrent(kMissionMgr);
    uint8_t* info = MissionMgr_GetInfo(kMissionMgr, *current);
    Font_AddText(Text_Get(kTheText, At<const char*>(info, 0x20)));
    uint16_t text[0x200];
    Font_Format(text, Text_GetByHash(kTheText, g_hashTaskTitle), 0x1FF);
    FE_DrawStyled(x, y, text, At<void*>(self, 8));
}

// 0x69EB60 (vf0) / 0x69EBD0 (vf1): "no items" texts
void __fastcall NoItems_GetExtent(uint8_t* self, re::Edx, float* rect, float x, float width) {
    FE_SetTextStyle(0xB, 0, 1, 0);
    g_fontRightEdge = x + width;
    Font_Measure(rect, x + 5.0f, 0.0f, Text_GetByHash(kTheText, At<uint32_t>(self, 8)));
    rect[3] = rect[3] + 0.0f;
}
void __fastcall NoItems_Draw(uint8_t* self, re::Edx, float x, float y, float width, float, int) {
    FE_SetTextStyle(0xB, 0, 1, 0);
    g_fontRightEdge = x + width;
    Font_PrintString(x + 5.0f, y, Text_GetByHash(kTheText, At<uint32_t>(self, 8)), 1);
    Font_Render();
}

// 0x69EC70 (not in the function list): FETasks' constructor
void* __fastcall Tasks_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FETasks);
    InitStyle(self + 0xC);
    InitStyle(self + 0x28);
    InitStyle(self + 0x44);
    InitStyle(self + 0x60);
    ScrollList_Construct(self + 0x7C, 0x32);
    SetVtable(self + 0x7C, VT_ScrollListT32);
    SetVtable(self + 0x1B0, VT_CTitle);
    At<uint8_t>(self, 0x1B4) = 0;
    SetVtable(self + 0x1C0, VT_CTitle);
    At<uint8_t>(self, 0x1C4) = 0;
    At<uint8_t>(self, 0x1D4) = 0;
    SetVtable(self + 0x1D0, VT_CTaskTitle);
    for (int i = 0; i < 0x32; i++) {
        At<uint8_t>(self, 0x1DC + i * 0x10 + 4) = 1;
        SetVtable(self + 0x1DC + i * 0x10, VT_CAvailableTask);
    }
    for (int i = 0; i < 10; i++) {
        At<uint8_t>(self, 0x504 + i * 0xC) = 1;
        SetVtable(self + 0x500 + i * 0xC, VT_CObjective);
    }
    At<uint8_t>(self, 0x580) = 0;
    SetVtable(self + 0x57C, VT_CNoItems);
    SetVtable(self + 0x588, VT_CNoItems);
    At<uint8_t>(self, 0x58C) = 0;
    SetVtable(self + 0x594, VT_CNoItems);
    At<uint8_t>(self, 0x598) = 0;
    At<uint32_t>(self, 0x1BC) = *reinterpret_cast<uint32_t*>(0x020C3098);
    At<uint8_t*>(self, 0x1B8) = self + 0x60;
    At<uint8_t*>(self, 0x1C8) = self + 0x60;
    At<uint32_t>(self, 0x1CC) = *reinterpret_cast<uint32_t*>(0x020C309C);
    At<uint8_t*>(self, 0x1D8) = self + 0x60;
    At<uint32_t>(self, 0x590) = *reinterpret_cast<uint32_t*>(0x020C30A8);
    At<uint32_t>(self, 0x584) = *reinterpret_cast<uint32_t*>(0x020C30A4);
    At<uint32_t>(self, 0x59C) = *reinterpret_cast<uint32_t*>(0x020C30AC);
    return self;
}

// an available task item added for a mission
static void AddTask(uint8_t* self, int32_t mission) {
    int32_t blip = MissionMgr_GetBlip(kMissionMgr, mission);
    uint8_t* item = self + 0x1DC + At<int32_t>(self, 0x4FC) * 0x10;
    At<int32_t>(self, 0x4FC) += 1;
    At<int32_t>(item, 8) = mission;
    At<int32_t>(item, 0xC) = blip;
    ScrollList_AddItem(self + 0x7C, item);
}

// 0x69EE70: the unlocked tasks (kinds 0 and 3, newest first, with their blips), then the
// kind-1 ones, each under its title (or "none")
void __fastcall Tasks_BuildTasksImpl(uint8_t* self, re::Edx) {
    int32_t tasks[100];
    int32_t others[100];
    int taskCount = 0;
    int otherCount = 0;
    for (int mission = 0; mission < MissionMgr_GetCount(kMissionMgr); mission++) {
        if ((MissionMgr_GetTaskFlags(kMissionMgr, mission)[4] & 1) == 0 ||
            At<uint8_t>(MissionMgr_GetInfo(kMissionMgr, mission), 0x25) != 0) {
            continue;
        }
        if (At<int32_t>(MissionMgr_GetInfo(kMissionMgr, mission), 0x28) == 1) {
            others[otherCount++] = mission;
        } else if (At<int32_t>(MissionMgr_GetInfo(kMissionMgr, mission), 0x28) == 0 ||
                   At<int32_t>(MissionMgr_GetInfo(kMissionMgr, mission), 0x28) == 3) {
            MissionMgr_6AAF10(kMissionMgr, mission);
            tasks[taskCount++] = mission;
        }
    }
    void* list = self + 0x7C;
    ScrollList_AddItem(list, self + 0x1B0);
    for (int i = taskCount - 1; i >= 0; i--) {
        AddTask(self, tasks[i]);
    }
    if (taskCount < 1) {
        ScrollList_AddItem(list, self + 0x57C);
    }
    ScrollList_AddItem(list, self + 0x1C0);
    for (int i = 0; i < otherCount; i++) {
        AddTask(self, others[i]);
    }
    if (otherCount > 0) {
        return;
    }
    ScrollList_AddItem(list, self + 0x588);
}

// 0x69F120: on a mission: its title and objectives (primary, then secondary; or "none")
void __fastcall Tasks_BuildObjectivesImpl(uint8_t* self, re::Edx) {
    void* list = self + 0x7C;
    ScrollList_AddItem(list, self + 0x1D0);
    int32_t objective = int32_t(reinterpret_cast<uintptr_t>(self));  // (the out's initial value: the screen)
    auto add = [&]() {
        uint8_t* item = self + 0x500 + At<int32_t>(self, 0x578) * 0xC;
        At<int32_t>(self, 0x578) += 1;
        At<int32_t>(item, 8) = objective;
        ScrollList_AddItem(list, item);
    };
    Objectives_BeginIterate(kObjectives);
    while (Objectives_NextPrimary(kObjectives, &objective) != nullptr) {
        add();
    }
    Objectives_BeginIterate(kObjectives);
    while (Objectives_NextSecondary(kObjectives, &objective) != nullptr) {
        add();
    }
    if (At<int32_t>(list, 4) < 1) {
        ScrollList_AddItem(list, self + 0x594);
    }
}

// 0x69F2C0 (vf3)
void __fastcall Tasks_OnEnter(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x4FC) = 0;
    At<int32_t>(self, 0x578) = 0;
    At<float>(self, 0x1AC) = g_completion;
    ScrollList_Clear(self + 0x7C);
    if (Mission_IsRunning(MissionMgr_GetCurrent(kMissionMgr)) != 0) {
        Tasks_BuildObjectives(self);
        return;
    }
    Tasks_BuildTasks(self);
}

// 0x69F350 (vf6): the chapter's background, the windows, the completion, the list
void __fastcall Tasks_Draw(uint8_t* self, re::Edx) {
    char chapter[24];
    Sprintf(chapter, "CHAPTER%d", int(g_chapter) + 1);
    FE_DrawBackground(HashName(chapter));
    FE_DrawWindow(320.0f, 440.0f, 130.0f, kWindowColour, 2.0f);
    FE_DrawWindow(320.0f, 440.0f, 394.0f, kWindowColour, 2.0f);
    if (Mission_IsRunning(MissionMgr_GetCurrent(kMissionMgr)) == 0) {
        Tasks_DrawCompletion(self);
        ScrollList_Draw(self + 0x7C, 0.0f, 0.0f, 0);
        if (At<int32_t>(self, 0x4FC) > 0) {
            FE_DrawTitle("FE_TASK_TOG");
            return;
        }
    } else {
        Tasks_DrawCompletion(self);
        ScrollList_Draw(self + 0x7C, 0.0f, 0.0f, 0);
    }
    FE_DrawTitle("FE_SELECT_C");
}

// 0x69F470 (vf1): an available task: its blip (not when hidden), its name, the highlight
void __fastcall AvailableTask_Draw(uint8_t* self, re::Edx, float x, float y, float width, float, int selected) {
    uint8_t* info = MissionMgr_GetInfo(kMissionMgr, At<int32_t>(self, 8));
    uint8_t* flags = MissionMgr_GetTaskFlags(kMissionMgr, At<int32_t>(self, 8));
    float iconWidth = float(double(g_screenScaleX) * 20.0);
    float iconHeight = float(double(g_screenScaleY) * 20.0);
    float iconX = (((x + width) - iconWidth) + 0.0f) - 5.0f;
    if ((flags[4] & 0x40) == 0 && At<int32_t>(self, 0xC) != -1) {
        void* sprite = Radar_GetBlipSprite(g_radar, At<int32_t>(self, 0xC));
        float top = float(double(y) - 2.0);
        float rect[4] = {iconX, top + iconHeight, iconWidth + iconX, top};
        uint8_t white[4];
        RGBA_Construct(white, 255, 255, 255, 255);
        Sprite_Draw(sprite, rect, white);
    }
    FE_SetTextStyle(re::Flag(selected) ? 3 : 0xB, 0, 1, 0);
    g_fontRightEdge = iconX;
    if (re::Flag(selected)) {
        uint8_t shadow[4];
        RGBA_Construct(shadow, 0, 0, 0, 0xA0);
        g_fontDropColour[0] = g_menuColours[0];
        g_fontDrop = 1;
        g_fontDropColour[1] = g_menuColours[1];
        g_fontDropColour[2] = g_menuColours[2];
        g_fontDropColour[3] = g_menuColours[3];
    }
    Font_PrintString(x + 5.0f, y, Text_Get(kTheText, At<const char*>(info, 0x20)), 1);
    float extent[4];
    InitExtent(extent);
    Font_GetExtent(extent, 0);
    Font_Render();
    FE_DrawHighlight(x + 5.0f + std::fabs(extent[2] - extent[0]), iconX - 4.0f, y);
}

// 0x69F690 (vf0): a title's extent
void __fastcall Title_GetExtent(uint8_t* self, re::Edx, float* rect, float x, float width) {
    rect[0] = x;
    rect[2] = x + width;
    rect[1] = 0.0f;
    rect[3] = 0.0f;
    FE_SetTextStyle(0xB, 0, 3, 0);
    float measured[4];
    InitExtent(measured);
    Font_Measure(measured, x, 0.0f, Text_GetByHash(kTheText, At<uint32_t>(self, 0xC)));
    rect[3] = std::fabs(measured[3] - measured[1]) + 0.0f + rect[3];
}

} // namespace fest

#define FEST_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(fest_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(fest_original_, __LINE__), \
             re::Mode::Original)

FEST_REVERSED("FEStats::Load", 0x0069C7F0, &fest::Stats_Load);
FEST_REVERSED("FEStats::CStatItem::CStatItem", 0x0069C840, &fest::StatItem_Construct);
FEST_REVERSED("FEStats::Draw", 0x0069C880, &fest::Stats_Draw);
FEST_REVERSED("FEStats::FEStats", 0x0069C980, &fest::Stats_Construct);
FEST_REVERSED("FEStats::Rebuild", 0x0069CA20, &fest::Stats_RebuildImpl);
FEST_REVERSED("FEStats::OnEnter", 0x0069CBD0, &fest::Stats_OnEnter);
FEST_REVERSED("FEStats::Process", 0x0069CC60, &fest::Stats_Process);
FEST_REVERSED("FEStats::CStatItem::GetExtent", 0x0069CD40, &fest::StatItem_GetExtent);
FEST_REVERSED("FEStats::CStatItem::Draw", 0x0069CE30, &fest::StatItem_Draw);
FEST_REVERSED("FEStatus::Load", 0x0069D040, &fest::Status_Load);
FEST_REVERSED("FEStatus::Process", 0x0069D060, &fest::Status_Process);
FEST_REVERSED("FEStatus::Draw", 0x0069D0A0, &fest::Status_Draw);
FEST_REVERSED("FEStatus::CStatusItem::CStatusItem", 0x0069D140, &fest::StatusItem_Construct);
FEST_REVERSED("FEStatus::CCompletedTask::GetExtent", 0x0069D2D0, &fest::CompletedTask_GetExtent);
FEST_REVERSED("FEStatus::CCompletedTask::Draw", 0x0069D340, &fest::CompletedTask_Draw);
FEST_REVERSED("FEStatus::CNoCompletedTasks::GetExtent", 0x0069D3B0, &fest::NoCompleted_GetExtent);
FEST_REVERSED("FEStatus::CNoCompletedTasks::Draw", 0x0069D410, &fest::NoCompleted_Draw);
FEST_REVERSED("FEStatus::FEStatus", 0x0069D4A0, &fest::Status_Construct);
FEST_REVERSED("FEStatus::OnEnter", 0x0069D520, &fest::Status_OnEnter);
FEST_REVERSED("FEStatus::CStatusItem::DrawBand", 0x0069D610, &fest::StatusItem_DrawBandImpl);
FEST_REVERSED("FEStatus::CStatusItem::DrawGrade", 0x0069D690, &fest::StatusItem_DrawGradeImpl);
FEST_REVERSED("FEStatus::CStatusItem::Draw", 0x0069DC40, &fest::StatusItem_Draw);
FEST_REVERSED("FEStatus::CCompletedHeader::GetExtent", 0x0069E180, &fest::CompletedHeader_GetExtent);
FEST_REVERSED("FEStatus::CCompletedHeader::Draw", 0x0069E2D0, &fest::CompletedHeader_Draw);
FEST_REVERSED("FEStoryMode::FEStoryMode", 0x0069E500, &fest::StoryMode_Construct);
FEST_REVERSED("FEStoryMode::Load", 0x0069E540, &fest::StoryMode_Load);
FEST_REVERSED("FEStoryMode::OnEnter", 0x0069E570, &fest::StoryMode_OnEnter);
FEST_REVERSED("FETasks::Load", 0x0069E6C0, &fest::Tasks_Load);
FEST_REVERSED("FETasks::Process", 0x0069E720, &fest::Tasks_Process);
FEST_REVERSED("FETasks::DrawCompletion", 0x0069E860, &fest::Tasks_DrawCompletionImpl);
FEST_REVERSED("FETasks::CAvailableTask::GetExtent", 0x0069E8C0, &fest::AvailableTask_GetExtent);
FEST_REVERSED("FETasks::CObjective::GetExtent", 0x0069E960, &fest::Objective_GetExtent);
FEST_REVERSED("FETasks::CObjective::Draw", 0x0069EA00, &fest::Objective_Draw);
FEST_REVERSED("FETasks::CTitle::Draw", 0x0069EAA0, &fest::Title_Draw);
FEST_REVERSED("FETasks::CTaskTitle::Draw", 0x0069EAD0, &fest::TaskTitle_Draw);
FEST_REVERSED("FETasks::CNoItems::GetExtent", 0x0069EB60, &fest::NoItems_GetExtent);
FEST_REVERSED("FETasks::CNoItems::Draw", 0x0069EBD0, &fest::NoItems_Draw);
FEST_REVERSED("FETasks::FETasks", 0x0069EC70, &fest::Tasks_Construct);
FEST_REVERSED("FETasks::BuildTasks", 0x0069EE70, &fest::Tasks_BuildTasksImpl);
FEST_REVERSED("FETasks::BuildObjectives", 0x0069F120, &fest::Tasks_BuildObjectivesImpl);
FEST_REVERSED("FETasks::OnEnter", 0x0069F2C0, &fest::Tasks_OnEnter);
FEST_REVERSED("FETasks::Draw", 0x0069F350, &fest::Tasks_Draw);
FEST_REVERSED("FETasks::CAvailableTask::Draw", 0x0069F470, &fest::AvailableTask_Draw);
FEST_REVERSED("FETasks::CTitle::GetExtent", 0x0069F690, &fest::Title_GetExtent);
