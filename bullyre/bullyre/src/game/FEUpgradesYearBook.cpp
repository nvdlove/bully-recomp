// The pause menu's upgrades screen (FEUpgrades), the reload warning (FEWarnReload) and
// the yearbook (FEYearBook), with the tasks' title item. 0x69F740..0x6A2317.
//
// A text style (0x1C bytes): +0 justification, +4 font preset (3), +8.. +0x10 floats,
// +0x14 colour index (2), +0x18 a flag (1), +0x19 / +0x1A.
// FEUpgrades (0x2A0 bytes): +0xC "HeadingStyle", +0x28 "DataStyle", +0x44 [0x48] what's
//   unlocked (saved), +0x8C [11] the headings' hashes, +0xB8 the tabs
//   (FESelectionListWindowT<2>: 0 fighting, 1 classes), +0x130 the list
//   (FEScrollingSelectionListWindowT<0x40>, its items at +0x198), +0x298 the items
//   (0x8C bytes each), +0x29C their count.
// CUpgradeItem (0x8C): +0 vtable, +4 selectable, +8 kind (0 an upgrade, 1 a heading),
//   +0xA its text [65].
// FEYearBook (0x430): +0xC its text style, +0x28 / +0x40 the page arrows (sprites),
//   +0x58 the first entry shown (a multiple of 12), +0x5C the "unknown" photo,
//   +0x60 [61] the students: the photo, then the three masks of the player's yearbook
//   bits that mean "met" (0x64, 0x68, 0x6C).

#include <cmath>

#include "game/FEScreensCommon.h"

#include "game/Crt.h"
#include "reversed.h"
namespace feyb {
using namespace fescr;

namespace {

constexpr uintptr_t VT_FEUpgrades = 0x0094095C;
constexpr uintptr_t VT_CUpgradeItem = 0x009408EC;
constexpr uintptr_t VT_FEWarnReload = 0x00940998;
constexpr uintptr_t VT_FEYearBook = 0x009409C8;
constexpr uintptr_t VT_SelListT2 = 0x0093F300;
constexpr uintptr_t VT_ScrollListT40 = 0x0094094C;

// the frontend (this range's neighbours)
GAME_FN(void, __cdecl, YB_SetTextStyle, 0x006A3FC0, int colour, int justify, int font, int dropShadow);
GAME_FN(void, __cdecl, YB_RequestSound0, 0x006A2360);
GAME_FN(uint8_t, __cdecl, YB_PadRepeatOk, 0x006A3170, int pad);
GAME_FN(void, __cdecl, YB_PadClear, 0x006A3120, int pad);
GAME_FN(void*, __cdecl, YB_GetTexture, 0x006A2600, int index);
GAME_FN(void, __cdecl, YB_DrawSprite, 0x006A42A0, void* sprite, void* texture, const uint8_t* colour,
        const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, YB_DrawCentredText, 0x006A6460, int x, int y, const uint16_t* text, void* style);
GAME_FN(void, __cdecl, YB_DrawControls, 0x006A6510, const char* key);
GAME_FN(void, __cdecl, YB_DrawUpgradeText, 0x006A6680, float x, float y, const uint16_t* text, void* style);
GAME_FN(void, __thiscall, YB_SpriteLoad, 0x006A49C0, void* sprite, void* archive, const char* name);
GAME_FN(void, __thiscall, YB_YearBookOnEnter, 0x006A07D0, void* self);
GAME_FN(void, __thiscall, YB_GetMasksOf, 0x006A21D0, void* self, int model, int32_t* a, int32_t* b, int32_t* c);
GAME_FN(void, __thiscall, YB_YearBookFrame, 0x006A0E70, void* self, int dim);
GAME_FN(void, __thiscall, YB_YearBookPage, 0x006A1390, void* self);
GAME_FN(void, __thiscall, YB_UpgradesRefresh, 0x0069F800, void* self);
GAME_FN(void, __thiscall, YB_UpgradesFillList, 0x006A0190, void* self);
GAME_FN(int8_t, __thiscall, YB_SelListSelected, 0x00899B00, void* list);
GAME_FN(uint8_t, __thiscall, YB_ScrollUp, 0x0069B5A0, void* list);
GAME_FN(uint8_t, __thiscall, YB_ScrollDown, 0x0069B480, void* list);

// missions
GAME_FN(uint32_t*, __thiscall, YB_NameKey, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, YB_NameKeyDestruct, 0x0049A650, uint32_t* key);
GAME_FN(int32_t, __thiscall, YB_MissionFind, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, YB_MissionFlags, 0x006AA7C0, void* mgr, int32_t mission);

// the upgrades' names and the classes' grades
GAME_FN(const char*, __cdecl, YB_UpgradeHeading, 0x005DAE40, int index);
GAME_FN(const char*, __cdecl, YB_UpgradeName, 0x005DAE00, int index);
GAME_FN(uint32_t, __cdecl, YB_HashString, 0x005DAE20, const char* name);
GAME_VAR(int32_t, g_gradeC1B02C, 0x00C1B02C);
GAME_VAR(int32_t, g_gradeC1B030, 0x00C1B030);
GAME_VAR(int32_t, g_gradeC1B034, 0x00C1B034);
GAME_VAR(int32_t, g_gradeC1B038, 0x00C1B038);
GAME_VAR(int32_t, g_gradeC1B03C, 0x00C1B03C);
GAME_VAR(int32_t, g_gradeC1B040, 0x00C1B040);
GAME_VAR(int32_t, g_gradeC1B044, 0x00C1B044);
GAME_VAR(int32_t, g_gradeC1B048, 0x00C1B048);
GAME_VAR(int32_t, g_gradeC1B04C, 0x00C1B04C);
GAME_VAR(int32_t, g_gradeC1B050, 0x00C1B050);

// the upgrade screen's styles for its items
GAME_VAR(int32_t*, g_upgradeHeadingStyle, 0x020C30B0);  // (the DataStyle, for headings)
GAME_VAR(int32_t*, g_upgradeItemStyle, 0x020C30B4);     // (the HeadingStyle, for upgrades)
GAME_VAR(uint32_t, g_taskTitleHash, 0x020C30A0);

// fonts and drawing
GAME_FN(void*, __cdecl, YB_GetStringRect, 0x0068B090, float* rect, float x, float y, const uint16_t* text);
GAME_VAR(float, g_fontWrapX, 0x0208B774);
GAME_VAR(int16_t, g_fontIndex, 0x0208B788);
GAME_VAR(float, g_fontScaleY, 0x0208B770);
GAME_ARRAY(float, g_fontHeights, 0x00BA4E8C);
GAME_VAR(float, g_yearbookNameDy, 0x00BA6880);  // (-6; 0 from the first Japanese draw on)
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_ARRAY(uint8_t, g_warnColour, 0x020C3328);
GAME_FN(uint8_t*, __thiscall, YB_RGBA, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, YB_SetRenderState, 0x00519790, int state, int value);
GAME_FN(float*, __cdecl, YB_ScreenRect, 0x0052D650, float* out, float left, float top, float right, float bottom,
        int hAlign, int vAlign);
GAME_FN(void*, __cdecl, YB_HudTexture, 0x00514EC0, int index);
GAME_FN(uint8_t, __cdecl, YB_PadJustDown, 0x00738900, int button, int pad);

// the yearbook's textures
GAME_FN(int, __cdecl, YB_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(int, __cdecl, YB_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, YB_RequestModel, 0x00532020, int id, uint32_t flags);
GAME_FN(void, __cdecl, YB_LoadAllRequested, 0x00533EE0, int onlyPriority);
GAME_FN(void, __cdecl, YB_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, YB_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, YB_SetCurrentTxd, 0x005F1100, int txd);
GAME_FN(void*, __cdecl, YB_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, YB_TextureDestroy, 0x005F0F90, void* texture);
GAME_VAR(void*, g_yearbookHud, 0x00C1AA04);
GAME_FN(void, __thiscall, YB_HudSetJimmy, 0x0045BE70, void* hud, int32_t a, int32_t b, int32_t c);
GAME_FN(uint32_t, __thiscall, YB_PlayerYearbookBits, 0x004466F0, void* playerInfo, int which);
void* const kPlayerInfo = reinterpret_cast<void*>(0x00C1AEA8);
GAME_ARRAY(uint32_t, g_studentNameHashes, 0x020C30B8);  // [61]
GAME_ARRAY(uint8_t*, g_modelInfos, 0x00C67738);
GAME_VAR(const char*, g_yearbookStyleName, 0x00BA6858);
GAME_VAR(const char*, g_yearbookLeftArrowName, 0x00BA685C);
GAME_VAR(const char*, g_yearbookRightArrowName, 0x00BA6860);

GAME_FN(void, __stdcall, YB_VectorConstruct, 0x00401240, void* array, size_t size, int count, void* ctor);
GAME_FN(void, __thiscall, SelList_Construct, 0x0069C3A0, void* list, void* entries, int count);
GAME_FN(void, __thiscall, ScrollList_Construct, 0x0069B1C0, void* list, int unused);
inline void* __cdecl YB_Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(uint8_t, __cdecl, YB_SaveCheck, 0x00733320, int, int, int value, int expected);
GAME_FN(uint8_t, __cdecl, YB_SaveCheckBlock, 0x00733620, const char* name, void* data, int size);

constexpr int kStudents = 61;

// (the game's _ftol: truncation)
inline int Ftol(float value) {
    return static_cast<int>(value);
}

// a mission's task done (its flags' +2 set)
bool TaskDone(const char* name) {
    uint32_t key;
    YB_NameKey(&key, name);
    int32_t mission = YB_MissionFind(kMissionMgr, &key);
    bool done = *reinterpret_cast<uint16_t*>(YB_MissionFlags(kMissionMgr, mission) + 2) >= 1;
    YB_NameKeyDestruct(&key);
    return done;
}

// the item's text measured: the rect's height added to the extent (GetExtent's tail)
void AddTextHeight(float* extent, float x, const uint16_t* text) {
    float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    YB_GetStringRect(rect, x, 0.0f, text);
    float height = std::fabs(rect[3] - rect[1]);
    extent[3] = height + 0.0f + extent[3];
}

// a text style's defaults (the screens' constructors)
void InitTextStyle(uint8_t* style) {
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

} // namespace

// ---- the tasks' title --------------------------------------------------------------------------

// 0x69F740 (vf0): GetExtent(rect, x, width)
void __fastcall TaskTitle_GetExtent(void*, re::Edx, float* rect, float x, float width) {
    rect[0] = x;
    rect[2] = x + width;
    rect[1] = 0.0f;
    rect[3] = 0.0f;
    YB_SetTextStyle(0xB, 0, 3, 0);
    AddTextHeight(rect, x, Text_GetByHash(kTheText, g_taskTitleHash));
}

// ---- FEUpgrades --------------------------------------------------------------------------------

// 0x69F7F0 (not in the function list): CUpgradeItem's constructor
void* __fastcall UpgradeItem_Construct(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 4) = 1;
    SetVtable(self, VT_CUpgradeItem);
    return self;
}

// 0x69F800: what's unlocked (the fighting moves from missions, the classes' by grade)
void __fastcall Upgrades_Refresh(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 0x44) = TaskDone("1_06_01");
    At<uint8_t>(self, 0x45) = TaskDone("1_06_02");
    At<uint8_t>(self, 0x46) = TaskDone("1_06_03");
    At<uint8_t>(self, 0x47) = TaskDone("1_06_04");
    At<uint8_t>(self, 0x48) = TaskDone("1_06_07");
    At<uint8_t>(self, 0x49) = TaskDone("1_06_08");
    struct Grades {
        int offset;
        int32_t* grade;
    };
    const Grades grades[] = {{0x4A, &g_gradeC1B044}, {0x5A, &g_gradeC1B030}, {0x7D, &g_gradeC1B050},
                             {0x5F, &g_gradeC1B02C}, {0x64, &g_gradeC1B048}, {0x69, &g_gradeC1B04C},
                             {0x82, &g_gradeC1B034}, {0x73, &g_gradeC1B040}, {0x78, &g_gradeC1B038},
                             {0x6E, &g_gradeC1B03C}};
    for (const Grades& g : grades) {
        for (int i = 0; i < 5; ++i) {
            At<uint8_t>(self, g.offset + i) = *g.grade >= i + 1;
        }
    }
}

// 0x69FC00 (vf4): OnLeave: the items freed
void __fastcall Upgrades_OnLeave(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x298) != nullptr) {
        Free(At<void*>(self, 0x298));
        At<void*>(self, 0x298) = nullptr;
    }
}

// 0x69FC30: how many fighting upgrades are unlocked (the six missions, and the gym
// class's grade up to 5)
int __cdecl Upgrades_CountFighting() {
    int count = TaskDone("1_06_01");
    count += TaskDone("1_06_02");
    count += TaskDone("1_06_03");
    count += TaskDone("1_06_04");
    count += TaskDone("1_06_07");
    count += TaskDone("1_06_08");
    int32_t grade = g_gradeC1B044;
    return count + (grade >= 5) + (grade >= 4) + (grade >= 3) + (grade >= 2) + (grade >= 1);
}

// 0x69FDE0 (vf6): Draw
void __fastcall Upgrades_Draw(uint8_t* self, re::Edx) {
    FE_DrawWindow(320.0f, 440.0f, 110.0f, reinterpret_cast<void*>(0x020C33E4), 2.0f);
    FE_DrawWindow(320.0f, 440.0f, 394.0f, reinterpret_cast<void*>(0x020C33E4), 2.0f);
    SelList_Draw(self + 0xB8);
    ScrollList_Draw(self + 0x130, 0.0f, 0.0f, 0);
    YB_DrawControls("FEU_CONTROLS");
}

// 0x69FE90: the save block (version 1, the screen's size, the flags' size, the flags)
void __fastcall Upgrades_Save(uint8_t* self, re::Edx, int32_t* block) {
    block[0] = 1;
    block[1] = 0x94;
    block[2] = 0x48;
    for (int i = 0; i < 0x48; ++i) {
        At<uint8_t>(block, 0xC + i) = At<uint8_t>(self, 0x44 + i);
    }
}

// 0x69FED0: loaded back, when the block checks out
void __fastcall Upgrades_Load(uint8_t* self, re::Edx, int32_t* block) {
    if (YB_SaveCheck(0, 0, block[0], 1) && YB_SaveCheck(0, 0, block[1], 0x94) &&
        YB_SaveCheck(0, 0, block[2], 0x48) && YB_SaveCheckBlock("UpgradesSaveData", block + 0x15, 0x10)) {
        for (int i = 0; i < 0x48; ++i) {
            At<uint8_t>(self, 0x44 + i) = At<uint8_t>(block, 0xC + i);
        }
    }
}

// 0x69FF50 (CUpgradeItem vf1): Draw(x, y, width, ...)
void __fastcall UpgradeItem_Draw(uint8_t* self, re::Edx, float x, float y, float width, int, int) {
    uint8_t kind = At<uint8_t>(self, 8);
    if (kind == 0) {
        YB_DrawUpgradeText(x, y, reinterpret_cast<uint16_t*>(self + 0xA), g_upgradeItemStyle);
        return;
    }
    if (kind == 1) {
        int32_t* style = g_upgradeHeadingStyle;
        YB_SetTextStyle(style[5], style[0], style[1], 0);
        g_fontWrapX = x + width;
        Font_PrintString(x + 5.0f, y, reinterpret_cast<uint16_t*>(self + 0xA), 1);
        Font_Render();
    }
}

// 0x6A0020 (not in the function list): FEUpgrades' constructor
void* __fastcall Upgrades_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEUpgrades);
    InitTextStyle(self + 0xC);
    InitTextStyle(self + 0x28);
    SelList_Construct(self + 0xB8, self + 0xB8 + 0x70, 2);
    SetVtable(self + 0xB8, VT_SelListT2);
    ScrollList_Construct(self + 0x130, 0x40);
    SetVtable(self + 0x130, VT_ScrollListT40);
    YB_Memset(self + 0x44, 0, 0x48);
    g_upgradeHeadingStyle = reinterpret_cast<int32_t*>(self + 0x28);
    g_upgradeItemStyle = reinterpret_cast<int32_t*>(self + 0xC);
    return self;
}

// 0x6A00E0 (vf1): Load
void __fastcall Upgrades_LoadLayout(uint8_t* self, re::Edx, void* archive) {
    struct StringBuffer {
        re::Ptr32<char> text;
        int32_t size;
        char buffer[32];
    } name;
    name.text = name.buffer;
    name.size = 0x1F;
    name.buffer[0] = 0;
    Screen_Load(self, archive);
    TextStyle_Load(self + 0xC, archive, "HeadingStyle");
    TextStyle_Load(self + 0x28, archive, "DataStyle");
    for (int i = 0; i < 11; ++i) {
        const char* key = YB_UpgradeHeading(i);
        VCall<void>(archive, 0x24, &name, 0x1F, key);
        At<uint32_t>(self, 0x8C + i * 4) = YB_HashString(name.text);
    }
    SelList_Load(self + 0xB8, archive);
    ScrollList_Load(self + 0x130, archive);
}

// 0x6A0190: the list refilled for the tab (fighting: 13 rows, upgrades 0..; classes: 54
// rows, upgrades 22..), the headings (hashes +0x8C..) between them
void __fastcall Upgrades_FillList(uint8_t* self, re::Edx) {
    uint8_t* list = self + 0x130;
    ScrollList_Clear(list);
    At<int32_t>(self, 0x29C) = 0;
    int tab = YB_SelListSelected(self + 0xB8);
    int upgrade;
    int heading;
    int last;
    if (tab == 0) {
        upgrade = 0;
        heading = 0;
        last = 0xC;
    } else if (tab == 1) {
        upgrade = 0x16;
        heading = 2;
        last = 0x35;
    } else {
        return;
    }
    int32_t* nextHeading = reinterpret_cast<int32_t*>(self + 0x8C + heading * 4);
    for (int rows = last + 1; rows != 0; --rows) {
        uint8_t* item = At<uint8_t*>(self, 0x298) + At<int32_t>(self, 0x29C) * 0x8C;
        At<int32_t>(self, 0x29C) += 1;
        uint16_t* text = reinterpret_cast<uint16_t*>(item + 0xA);
        if (upgrade == *nextHeading) {
            At<uint8_t>(item, 4) = 0;
            At<uint8_t>(item, 8) = 0;
            Font_Format(text, Text_Get(kTheText, YB_UpgradeHeading(heading)), 0x3F);
            ++heading;
            ++nextHeading;
        } else {
            At<uint8_t>(item, 4) = 1;
            At<uint8_t>(item, 8) = 1;
            if (At<uint8_t>(self, 0x44 + upgrade) == 0) {
                Font_Format(text, Text_Get(kTheText, "LOCKED"), 0x3F);
            } else {
                Font_Format(text, Text_Get(kTheText, YB_UpgradeName(upgrade)), 0x3F);
            }
            ++upgrade;
        }
        ScrollList_AddItem(list, item);
    }
}

// 0x6A0340 (vf3): OnEnter: the tabs, what's unlocked, 64 items (a null block isn't
// checked by the list's fill)
void __fastcall Upgrades_OnEnter(uint8_t* self, re::Edx) {
    uint8_t* tabs = self + 0xB8;
    SelList_Clear(tabs);
    SelList_AddByName(tabs, "FIGHTING");
    SelList_AddByName(tabs, "CLASSES");
    YB_UpgradesRefresh(self);
    void* items = Malloc(0x2300);
    if (items != nullptr) {
        YB_VectorConstruct(items, 0x8C, 0x40, reinterpret_cast<void*>(0x0069F7F0));
        At<void*>(self, 0x298) = items;
    } else {
        At<void*>(self, 0x298) = nullptr;
    }
    YB_UpgradesFillList(self);
}

// 0x6A03C0 (vf5): Process
void __fastcall Upgrades_Process(uint8_t* self, re::Edx) {
    if (Input_BackPressed(kUserInput, 0)) {
        YB_RequestSound0();
        g_menuActive = 0;
        return;
    }
    uint8_t* tabs = self + 0xB8;
    bool changed = false;
    bool tried = false;
    if (g_menuRight != 0 && YB_PadRepeatOk(0)) {
        int before = YB_SelListSelected(tabs);
        changed = before != SelList_Next(tabs);
        tried = true;
    } else if (g_menuLeft != 0 && YB_PadRepeatOk(0)) {
        int before = YB_SelListSelected(tabs);
        changed = before != SelList_Previous(tabs);
        tried = true;
    }
    if (tried && changed) {
        YB_UpgradesFillList(self);
        YB_PadClear(0);
    }
    if (g_menuUp != 0 && YB_ScrollUp(self + 0x130)) {
        YB_RequestSound0();
        return;
    }
    if (g_menuDown != 0 && YB_ScrollDown(self + 0x130)) {
        YB_RequestSound0();
    }
}

// 0x6A04B0 (CUpgradeItem vf0): GetExtent(rect, x, width)
void __fastcall UpgradeItem_GetExtent(uint8_t* self, re::Edx, float* rect, float x, float width) {
    rect[0] = x;
    float right = x + width;
    rect[2] = right;
    rect[1] = 0.0f;
    rect[3] = 0.0f;
    int32_t* style = At<uint8_t>(self, 8) != 0 ? g_upgradeHeadingStyle : g_upgradeItemStyle;
    YB_SetTextStyle(style[5], style[0], style[1], 0);
    float r[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    g_fontWrapX = right;
    YB_GetStringRect(r, x + 5.0f, 0.0f, reinterpret_cast<uint16_t*>(self + 0xA));
    float height = std::fabs(r[3] - r[1]);
    rect[3] = height + 0.0f + rect[3];
}

// ---- FEWarnReload ------------------------------------------------------------------------------

// 0x6A0580 (not in the function list): the constructor
void* __fastcall WarnReload_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEWarnReload);
    InitTextStyle(self + 0xC);
    return self;
}

// 0x6A05C0 (vf1): Load
void __fastcall WarnReload_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    TextStyle_Load(self + 0xC, archive, "WarningStyle");
}

// 0x6A05F0 (vf3): OnEnter: the font reset (a lone jmp)
RE_UNHOOKED("FEWarnReload::OnEnter", 0x006A05F0);

// 0x6A0600 (vf5): Process: button 7 polled on the pads until one has it (nothing done
// with it)
void __fastcall WarnReload_Process(void*, re::Edx) {
    if (!YB_PadJustDown(7, 0) && !YB_PadJustDown(7, 1) && !YB_PadJustDown(7, 2)) {
        YB_PadJustDown(7, 3);
    }
}

// 0x6A0640 (vf6): Draw: the warning, centred 100 above the middle
void __fastcall WarnReload_Draw(void*, re::Edx) {
    float position[2];
    const float* at = ScreenPosition(position, g_screenScaleX * 0.0f, -100.0f, 1, 1);
    float x = at[0];
    float y = at[1];
    g_fontColour[0] = g_warnColour[0];
    g_fontColour[1] = g_warnColour[1];
    g_fontColour[2] = g_warnColour[2];
    g_fontColour[3] = g_warnColour[3];
    Font_SetJustify(2);
    Font_SetSizePreset(4);
    uint16_t text[86];
    Font_Format(text, Text_Get(kTheText, "FE_WARN_RELOAD"), 0x55);
    Font_PrintString(x, y, text, 1);
    Font_Render();
}

// ---- FEYearBook --------------------------------------------------------------------------------

// 0x6A0720 (not in the function list): the constructor
void* __fastcall YearBook_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEYearBook);
    InitTextStyle(self + 0xC);
    for (int sprite = 0x28; sprite <= 0x40; sprite += 0x18) {
        At<int32_t>(self, sprite) = 0;
        At<float>(self, sprite + 4) = 0.0f;
        At<float>(self, sprite + 8) = 0.0f;
        At<float>(self, sprite + 0xC) = 0.0f;
        At<float>(self, sprite + 0x10) = 0.0f;
        At<int32_t>(self, sprite + 0x14) = 1;
    }
    return self;
}

// 0x6A0780 (vf1): Load
void __fastcall YearBook_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    TextStyle_Load(self + 0xC, archive, g_yearbookStyleName);
    const char* left = g_yearbookLeftArrowName;
    At<int32_t>(self, 0x20) = 0;
    YB_SpriteLoad(self + 0x28, archive, left);
    YB_SpriteLoad(self + 0x40, archive, g_yearbookRightArrowName);
}

// 0x6A07D0 (vf3's body): the photos loaded from "Yearbook", each student's masks (bit i of
// the 96), Jimmy's masks given to the yearbook HUD
void __fastcall YearBook_OnEnter(uint8_t* self, re::Edx) {
    static const char* const kNames[kStudents] = {
        "Algernon", "Angie",   "Beatrice", "Bif",     "Bo",      "Bryce",  "Bucky",  "Casey",        "Chad",
        "Christy",  "Constantinos", "Cornelius", "Damon", "Dan", "Darby",  "Davis",  "Donald",       "Earnest",
        "Ethan",    "Eunice",  "Fatty",    "Gary",    "Gloria",  "Gord",   "Gordon", "Hal",          "Ivan",
        "Jimmy",    "Johnny",  "Juri",     "Justin",  "Karen",   "Kirby",  "Lance",  "Lefty",        "Lola",
        "Lucky",    "Luis",    "Mandy",    "Melody",  "Melvin",  "Norton", "Parker", "Peanut",       "Pedro",
        "Peter",    "Pinky",   "Ray",      "Ricky",   "Russell", "Sheldon", "Tad",   "Ted",          "Thad",
        "Tom",      "Trent",   "Trevor",   "Troy",    "Vance",   "Wade",   "Zoe"};
    int txd = YB_FindTxdSlot("Yearbook", 0);
    if (txd == -1) {
        txd = YB_AddTxdSlot("Yearbook");
    }
    YB_RequestModel(txd + 0x2FA8, 9);
    YB_LoadAllRequested(1);
    YB_PushCurrentTxd();
    YB_SetCurrentTxd(txd);
    for (int i = 0; i < kStudents; ++i) {
        int32_t* masks = reinterpret_cast<int32_t*>(self + 0x64 + i * 0x10);
        masks[0] = 0;
        masks[1] = 0;
        masks[2] = 0;
        if (i < 0x20) {
            masks[0] = 1 << i;
        } else if (i < 0x40) {
            masks[1] = 1 << (i - 0x20);
        } else if (i < 0x60) {
            masks[2] = 1 << (i - 0x40);
        }
    }
    for (int i = 0; i < kStudents; ++i) {
        At<void*>(self, 0x60 + i * 0x10) = YB_FindTexture(kNames[i], nullptr);
    }
    At<void*>(self, 0x5C) = YB_FindTexture("unknown", nullptr);
    At<int32_t>(self, 0x58) = 0;
    YB_HudSetJimmy(g_yearbookHud, At<int32_t>(self, 0x214), At<int32_t>(self, 0x218), At<int32_t>(self, 0x21C));
    YB_PopCurrentTxd();
}

// 0x6A0CD0 (vf4): OnLeave: the photos released
void __fastcall YearBook_OnLeave(uint8_t* self, re::Edx) {
    re::Ptr32<void>* photo = reinterpret_cast<re::Ptr32<void>*>(self + 0x60);
    for (int i = kStudents; i != 0; --i, photo += 4) {
        if (*photo != nullptr) {
            YB_TextureDestroy(*photo);
            *photo = nullptr;
        }
    }
    if (At<void*>(self, 0x5C) != nullptr) {
        YB_TextureDestroy(At<void*>(self, 0x5C));
        At<void*>(self, 0x5C) = nullptr;
    }
}

// 0x6A0D20 (vf5): Process: back, or a page of 12 left/right (clamped as floats)
void __fastcall YearBook_Process(uint8_t* self, re::Edx) {
    if (!YB_PadRepeatOk(0)) {
        return;
    }
    if (Input_BackPressed(kUserInput, 0)) {
        YB_RequestSound0();
        g_menuActive = 0;
    }
    int total = kStudents;
    do {
        ++total;
    } while (total % 12 != 0);
    auto clampPage = [&](int page) {
        float value = static_cast<float>(page);
        float last = static_cast<float>(total - 12);
        float result = value < 0.0f ? 0.0f : (last < value ? last : value);
        return Ftol(result);
    };
    if (g_menuLeft != 0) {
        At<int32_t>(self, 0x58) += -12;
        At<int32_t>(self, 0x58) = clampPage(At<int32_t>(self, 0x58));
        YB_PadClear(0);
    }
    if (g_menuRight != 0) {
        At<int32_t>(self, 0x58) += 12;
        At<int32_t>(self, 0x58) = clampPage(At<int32_t>(self, 0x58));
        YB_PadClear(0);
    }
}

// 0x6A0E60 (vf3): OnEnter (a lone jmp to 0x6A07D0)
RE_UNHOOKED("FEYearBook::OnEnter", 0x006A0E60);

// 0x6A0E70: the book's frame (textures 6 / 5, in halves, mirrored), and the arrows while
// there's a page that way. (`dim` sets the alpha of a colour nothing uses.)
void __fastcall YearBook_DrawFrame(uint8_t* self, re::Edx, int dim) {
    uint8_t unused[4];
    YB_RGBA(unused, 0xFF, 0xFF, 0xFF, 0xFF);
    if (re::Flag(dim)) {
        unused[3] = 0x20;
    }
    YB_SetRenderState(2, 3);
    float rect[4];
    YB_ScreenRect(rect, 0.0f, 10.0f, 470.0f, 320.0f, 1, 1);
    float width = std::fabs(rect[2] - rect[0]);
    float half = width * 0.5f;
    float edge = g_screenScaleY * 16.0f;
    float uvA[2];
    float uvB[2];
    uint8_t colour[4];
    auto quad = [&](int texture, float x1, float y1, float x2, float y2) {
        uint8_t* c = YB_RGBA(colour, 0xFF, 0xFF, 0xFF, 0xFF);
        DrawRectUV(x1, y1, x2, y2, YB_GetTexture(texture), c, uvA, uvB);
    };
    // top
    uvA[0] = 1.0f, uvA[1] = 1.0f, uvB[0] = 0.0f, uvB[1] = 0.0f;
    quad(6, rect[0], rect[1], rect[0] + half, rect[1] + edge);
    float full = half + half;
    uvA[0] = 0.0f, uvA[1] = 1.0f, uvB[0] = 1.0f, uvB[1] = 0.0f;
    quad(6, rect[0] + half, rect[1], full + rect[0], rect[1] + edge);
    // the sides
    uvA[0] = 1.0f, uvA[1] = 1.0f, uvB[0] = 0.0f, uvB[1] = 0.0f;
    quad(5, rect[0], rect[1] + edge, rect[0] + half, rect[3] - edge);
    uvA[0] = 0.0f, uvA[1] = 0.0f, uvB[0] = 1.0f, uvB[1] = 1.0f;
    quad(5, rect[0] + half, rect[1] + edge, full + rect[0], rect[3] - edge);
    // bottom
    uvA[0] = 1.0f, uvA[1] = 0.0f, uvB[0] = 0.0f, uvB[1] = 1.0f;
    quad(6, rect[0], rect[3] - edge, rect[0] + half, rect[3]);
    uvA[0] = 0.0f, uvA[1] = 0.0f, uvB[0] = 1.0f, uvB[1] = 1.0f;
    quad(6, rect[0] + half, rect[3] - edge, full + rect[0], rect[3]);
    if (At<int32_t>(self, 0x58) != 0) {
        uvA[0] = 0.0f, uvA[1] = 0.0f, uvB[0] = 1.0f, uvB[1] = 1.0f;
        uint8_t* c = YB_RGBA(colour, 0xFF, 0xFF, 0xFF, 0xFF);
        YB_DrawSprite(self + 0x28, YB_HudTexture(0), c, uvA, uvB);
    }
    if (At<int32_t>(self, 0x58) < 0x31) {
        uvA[0] = 0.0f, uvA[1] = 0.0f, uvB[0] = 1.0f, uvB[1] = 1.0f;
        uint8_t* c = YB_RGBA(colour, 0xFF, 0xFF, 0xFF, 0xFF);
        YB_DrawSprite(self + 0x40, YB_HudTexture(1), c, uvA, uvB);
    }
}

// 0x6A1350: a model is one of the yearbook's students (its name's hash in the table)
uint8_t __cdecl YearBook_IsStudent(int model) {
    uint8_t* info = g_modelInfos[model];
    if (info != nullptr) {
        uint32_t hash = HashName(reinterpret_cast<const char*>(info + 0x98));
        for (uint32_t* entry = g_studentNameHashes; entry < g_studentNameHashes + kStudents; ++entry) {
            if (*entry == hash) {
                return 1;
            }
        }
    }
    return 0;
}

// 0x6A1390: the page: 12 photos (two pages of 3 rows of 2), each on a card, the photo or
// "unknown" until the player's yearbook bits have one of its masks, the name under it;
// then "page n of 6"
void __fastcall YearBook_DrawPage(uint8_t* self, re::Edx) {
    float scaleX = g_screenScaleX;
    float scaleY = g_screenScaleY;
    float r[4];
    YB_ScreenRect(r, 0.0f, 5.0f, 470.0f, 308.0f, 1, 1);
    float half = std::fabs(r[2] - r[0]) * 0.5f;
    YB_SetTextStyle(At<int32_t>(self, 0x20), At<int32_t>(self, 0xC), At<int32_t>(self, 0x10), 0);
    float lineHeight = g_fontHeights[g_fontIndex] * g_fontScaleY;
    if (g_language == 7) {
        lineHeight = lineHeight + lineHeight;
    }
    float margin42 = scaleX * 42.0f;
    float left = r[0] + margin42;
    float top = scaleY * 10.0f + r[1];
    float bottom = r[3] + scaleY * 0.0f;
    float margin34 = scaleX * -34.0f;
    float right = (r[0] + half) + margin34;
    float height = std::fabs(bottom - top);
    float cellHeight = (height - (lineHeight + 0.0f) * 3.0f) / 3.0f;
    float cellWidth = cellHeight * scaleX;
    float rowStep = (height - cellHeight * 3.0f) / 3.0f + cellHeight;
    if (g_language == 7) {
        g_yearbookNameDy = 0.0f;
    }
    float insetX = scaleX * 5.0f;
    float insetY = scaleY * 5.0f;
    int student = At<int32_t>(self, 0x58);
    uint8_t colour[4];
    float uv1[2];
    float uv2[2];

    // one photo: its card, then (a student there) the photo and the name
    auto photo = [&](int index, float x1, float x2, float yTop, float yBottom, float dx, float dy, float nameX) {
        uint8_t* entry = self + 0x60 + index * 0x10;
        void* texture;
        if ((YB_PlayerYearbookBits(kPlayerInfo, 0x14) != 0 &&
             (At<uint32_t>(entry, 4) & YB_PlayerYearbookBits(kPlayerInfo, 0x14)) != 0) ||
            (YB_PlayerYearbookBits(kPlayerInfo, 0x15) != 0 &&
             (At<uint32_t>(entry, 8) & YB_PlayerYearbookBits(kPlayerInfo, 0x15)) != 0) ||
            (YB_PlayerYearbookBits(kPlayerInfo, 0x16) != 0 &&
             (At<uint32_t>(entry, 0xC) & YB_PlayerYearbookBits(kPlayerInfo, 0x16)) != 0)) {
            texture = At<void*>(entry, 0);
        } else {
            texture = At<void*>(self, 0x5C);
        }
        uv2[0] = 1.0f, uv2[1] = 1.0f, uv1[0] = 0.0f, uv1[1] = 0.0f;
        uint8_t* c = YB_RGBA(colour, 0xFF, 0xFF, 0xFF, 0xFF);
        DrawRectUV((insetX + x1) + dx, (insetY + yTop) + dy, (x2 - insetX) + dx, (yBottom - insetY) + dy, texture, c,
                   uv1, uv2);
        uint32_t hash = g_studentNameHashes[index];
        const uint16_t* name = Text_GetByHash(kTheText, hash);
        if (hash == HashName("N_Constantinos")) {
            name = Text_GetByHash(kTheText, HashName("N_Const_Short"));
        }
        int y = Ftol((g_yearbookNameDy + yBottom) + dy);
        int x = Ftol((nameX + 0.0f) + dx);
        YB_DrawCentredText(x, y, name, self + 0xC);
    };

    // one page's three rows, its two columns from `x0` and to `x1`
    auto page = [&](float x0, float x1) {
        float leftRight = cellWidth + x0;
        float rightLeft = x1 - cellWidth;
        for (int row = 0; row < 3; ++row, student += 2) {
            float fRow = static_cast<float>(row);
            float yTop = fRow * rowStep + top;
            float yBottom = cellHeight + yTop;
            float dy = fRow * -5.0f;
            float yBottomCard = yBottom + dy;
            float dx = fRow * 0.0f;
            float yTopCard = yTop + dy;
            uv2[0] = 1.0f, uv2[1] = 1.0f, uv1[0] = 0.0f, uv1[1] = 0.0f;
            uint8_t* c = YB_RGBA(colour, 0xFF, 0xFF, 0xFF, 0x80);
            DrawRectUV(dx + x0, yTopCard, leftRight + dx, yBottomCard, YB_GetTexture(4), c, uv1, uv2);
            if (student < kStudents) {
                photo(student, x0, leftRight, yTop, yBottom, dx, dy, cellWidth * 0.5f + x0);
            }
            uv2[0] = 1.0f, uv2[1] = 1.0f, uv1[0] = 0.0f, uv1[1] = 0.0f;
            c = YB_RGBA(colour, 0xFF, 0xFF, 0xFF, 0x80);
            DrawRectUV(rightLeft + dx, yTopCard, x1 + dx, yBottomCard, YB_GetTexture(4), c, uv1, uv2);
            if (student + 1 < kStudents) {
                photo(student + 1, rightLeft, x1, yTop, yBottom, dx, dy, x1 - cellWidth * 0.5f);
            }
        }
    };
    page(left, right);
    float left2 = (r[0] - margin34) + half;
    float right2 = half + ((r[0] + half) - margin42);
    page(left2, right2);

    Font_AddNumber(static_cast<float>(At<int32_t>(self, 0x58) / 12 + 1));
    Font_AddNumber(6.0f);
    uint16_t text[0x18];
    Font_Format(text, Text_Get(kTheText, "FEYB_PAGENUM"), 0x18);
    float x = scaleX * 200.0f + 320.0f;
    FE_DrawText(Ftol(x), 0x180, text, 0, 1, 1, 0);
}

// 0x6A21D0: a model's student masks (unchanged when the model has no info; zero when it
// isn't a student)
void __fastcall YearBook_GetMasks(uint8_t* self, re::Edx, int model, int32_t* a, int32_t* b, int32_t* c) {
    uint8_t* info = g_modelInfos[model];
    if (info == nullptr) {
        return;
    }
    uint32_t hash = HashName(reinterpret_cast<const char*>(info + 0x98));
    for (int i = 0; i < kStudents; ++i) {
        if (g_studentNameHashes[i] == hash) {
            uint8_t* entry = self + i * 0x10;
            *a = At<int32_t>(entry, 0x64);
            *b = At<int32_t>(entry, 0x68);
            *c = At<int32_t>(entry, 0x6C);
            return;
        }
    }
    *a = 0;
    *b = 0;
    *c = 0;
}

// 0x6A2250: the same from a yearbook made on the stack for it (its OnEnter loads the
// photos, which are never released: they leak a reference each call)
void __cdecl YearBook_MasksOfModel(int model, int32_t* a, int32_t* b, int32_t* c) {
    alignas(4) uint8_t book[0x430];
    YearBook_Construct(book, nullptr);
    YB_YearBookOnEnter(book);
    YB_GetMasksOf(book, model, a, b, c);
}

// 0x6A2300 (vf6): Draw
void __fastcall YearBook_Draw(uint8_t* self, re::Edx) {
    YB_YearBookFrame(self, 1);
    YB_DrawControls("FEYB_CONTROLS");
    YB_YearBookPage(self);
}

} // namespace feyb

#define FEYB_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(feyb_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(feyb_original_, __LINE__), \
             re::Mode::Original)

FEYB_REVERSED("FETasks::CTaskTitle::GetExtent", 0x0069F740, &feyb::TaskTitle_GetExtent);
FEYB_REVERSED("FEUpgrades::CUpgradeItem::CUpgradeItem", 0x0069F7F0, &feyb::UpgradeItem_Construct);
FEYB_REVERSED("FEUpgrades::Refresh", 0x0069F800, &feyb::Upgrades_Refresh);
FEYB_REVERSED("FEUpgrades::OnLeave", 0x0069FC00, &feyb::Upgrades_OnLeave);
FEYB_REVERSED("FEUpgrades::CountFighting", 0x0069FC30, &feyb::Upgrades_CountFighting);
FEYB_REVERSED("FEUpgrades::Draw", 0x0069FDE0, &feyb::Upgrades_Draw);
FEYB_REVERSED("FEUpgrades::Save", 0x0069FE90, &feyb::Upgrades_Save);
FEYB_REVERSED("FEUpgrades::LoadSave", 0x0069FED0, &feyb::Upgrades_Load);
FEYB_REVERSED("FEUpgrades::CUpgradeItem::Draw", 0x0069FF50, &feyb::UpgradeItem_Draw);
FEYB_REVERSED("FEUpgrades::FEUpgrades", 0x006A0020, &feyb::Upgrades_Construct);
FEYB_REVERSED("FEUpgrades::Load", 0x006A00E0, &feyb::Upgrades_LoadLayout);
FEYB_REVERSED("FEUpgrades::FillList", 0x006A0190, &feyb::Upgrades_FillList);
FEYB_REVERSED("FEUpgrades::OnEnter", 0x006A0340, &feyb::Upgrades_OnEnter);
FEYB_REVERSED("FEUpgrades::Process", 0x006A03C0, &feyb::Upgrades_Process);
FEYB_REVERSED("FEUpgrades::CUpgradeItem::GetExtent", 0x006A04B0, &feyb::UpgradeItem_GetExtent);
FEYB_REVERSED("FEWarnReload::FEWarnReload", 0x006A0580, &feyb::WarnReload_Construct);
FEYB_REVERSED("FEWarnReload::Load", 0x006A05C0, &feyb::WarnReload_Load);
FEYB_REVERSED("FEWarnReload::Process", 0x006A0600, &feyb::WarnReload_Process);
FEYB_REVERSED("FEWarnReload::Draw", 0x006A0640, &feyb::WarnReload_Draw);
FEYB_REVERSED("FEYearBook::FEYearBook", 0x006A0720, &feyb::YearBook_Construct);
FEYB_REVERSED("FEYearBook::Load", 0x006A0780, &feyb::YearBook_Load);
FEYB_REVERSED("FEYearBook::LoadPhotos", 0x006A07D0, &feyb::YearBook_OnEnter);
FEYB_REVERSED("FEYearBook::OnLeave", 0x006A0CD0, &feyb::YearBook_OnLeave);
FEYB_REVERSED("FEYearBook::Process", 0x006A0D20, &feyb::YearBook_Process);
FEYB_REVERSED("FEYearBook::DrawFrame", 0x006A0E70, &feyb::YearBook_DrawFrame);
FEYB_REVERSED("FEYearBook::IsStudent", 0x006A1350, &feyb::YearBook_IsStudent);
FEYB_REVERSED("FEYearBook::DrawPage", 0x006A1390, &feyb::YearBook_DrawPage);
FEYB_REVERSED("FEYearBook::GetMasks", 0x006A21D0, &feyb::YearBook_GetMasks);
FEYB_REVERSED("FEYearBook::MasksOfModel", 0x006A2250, &feyb::YearBook_MasksOfModel);
FEYB_REVERSED("FEYearBook::Draw", 0x006A2300, &feyb::YearBook_Draw);
