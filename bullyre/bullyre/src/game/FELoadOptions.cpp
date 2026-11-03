// The frontend's load screen (FELoad), map, message and options screens (FEOptions and
// the COption kinds' input and drawing; their Init/Save/Restore are Frontend.cpp's).
// 0x695490..0x6986E8.
//
// FELoad: +0xC / +0x10 the file table's left and right x, +0x14 its y, +0x18 the rows'
//   spacing (ints), +0x1C the state (0 choosing a slot, 1 / 2 / 4 yes-no questions,
//   3 a message), +0x20 the yes/no choice (0 yes), +0x24 the slot, +0x28 the table shown,
//   +0x29 a load started, +0x2C / +0x48 / +0x64 text styles, +0x80 a position.
// FEOptions: +0xC.. its options (vibration, music, SFX, speech volumes, audio output,
//   brightness, screen position, subtitles, language, invert camera, reset), +0xD8 [3][10]
//   the tabs' options (audio, display, misc), +0x150 [3] their counts, +0x154 the row,
//   +0x158 changing the row's option, +0x15C.. the layout, +0x16C the tabs.
// COption (Frontend.h): +4 its kind (1 left/right steps, 2 its own input).

#include <cmath>

#include "game/FEScreensCommon.h"
#include "game/Frontend.h"

using namespace fescr;

namespace {

constexpr uintptr_t VT_FELoad = 0x0093FA84;
constexpr uintptr_t VT_FEMap = 0x0093FB7C;
constexpr uintptr_t VT_FEMessage = 0x0093FBB0;
constexpr uintptr_t VT_FEOptions = 0x0093FD04;
constexpr uintptr_t VT_SelListT2 = 0x0093FCFC;

void* const kWindowColour = reinterpret_cast<void*>(0x020C33E4);

// the save system (memory card emulation)
GAME_FN(int32_t, __cdecl, SaveSys_GetState, 0x00735370);
GAME_FN(const char*, __cdecl, SaveSys_GetSlotInfo, 0x007354B0, int slot);
GAME_FN(void, __cdecl, SaveSys_Load, 0x007353E0, int slot);
GAME_FN(int32_t, __cdecl, SaveSys_FirstUsedSlot, 0x007373D0, int);
GAME_FN(void, __cdecl, SaveSys_DrawMessage, 0x00735660, const void* position, const void* style);
GAME_FN(uint8_t, __cdecl, SaveSys_7368C0, 0x007368C0, int);
GAME_FN(uint8_t, __cdecl, SaveSys_736E70, 0x00736E70, int);
GAME_VAR(int32_t, g_lastSaveState, 0x00BA6168);
GAME_VAR(float, g_fileRowSpacing, 0x00BA6184);
GAME_ARRAY(const char*, g_loadStyleNames, 0x00BA6158);  // [4]
GAME_VAR(uint8_t*, g_uberCompletion, 0x020CE75C);        // (+0x264: what the menu does next)
GAME_VAR(int32_t, g_loadingGame, 0x020CE760);
GAME_ARRAY(uint8_t, g_menuColours, 0x020C331C);          // [n][4]
GAME_VAR(uint8_t, g_menuFlagsByte, 0x0208B78A);
GAME_VAR(float, g_fontWrapX, 0x0208B778);
GAME_VAR(int16_t, g_fontSizeIndex, 0x0208B788);
GAME_ARRAY(float, g_fontLineHeights, 0x00BA4E8C);
GAME_VAR(float, g_fontScaleY, 0x0208B770);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_FN(void, __cdecl, FE_DrawTextKey, 0x006A6380, int x, int y, const char* key, int font, int justify, int, int);
GAME_FN(void, __cdecl, FE_DrawKeyStyled, 0x006A6CC0, int x, int y, const char* key, const void* style);
GAME_FN(void, __cdecl, FE_DrawMessageStyled, 0x006A6D00, const void* position, const char* key, const void* style);
GAME_FN(void, __cdecl, FE_DrawSaveInfo, 0x006A5540, const char* info, float left, float right, float y1, float y2,
        int colour, float scaleX, float scaleY, int highlighted);
GAME_FN(void, __thiscall, Position_Load, 0x006A4940, void* position, void* archive, const char* name);
GAME_FN(float, __thiscall, Position_GetY, 0x006A2790, const void* position);
GAME_FN(void, __cdecl, FE_ExitToMainMenu, 0x006A4250);
GAME_FN(void, __cdecl, Font_SetScale, 0x006880B0, float x, float y);
GAME_FN(void, __cdecl, Font_GetExtent, 0x00688EA0, float* rect, uintptr_t);
GAME_FN(uint8_t, __cdecl, Pad_ButtonJustDown, 0x00738900, int button, int pad);
GAME_FN(void, __thiscall, Load_DrawYesNo, 0x006958A0, void* self, float y);
GAME_FN(void, __thiscall, Load_DrawFiles, 0x00695990, void* self);

// the map and message screens
GAME_FN(void, __thiscall, Radar_SetMapMode, 0x0054DEF0, void* radar, int mode);
GAME_FN(void, __cdecl, Monitor_5EE1D0, 0x005EE1D0);
GAME_VAR(uint32_t, g_mapBackground, 0x020C2FF8);
GAME_FN(void, __cdecl, FE_DrawBackground, 0x006A2FF0, uint32_t which);
GAME_FN(uint8_t, __cdecl, Screen_BeginOverlay, 0x0043AB30, int, int, int, int, int, int, int, int, int, int, int, int,
        int);
GAME_FN(void, __cdecl, Screen_EndOverlay, 0x0043ACE0, int);
GAME_FN(void, __cdecl, Render_4094B0, 0x004094B0, int);
GAME_FN(void, __cdecl, Font_68A250, 0x0068A250);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const float* rect, const uint8_t* colour);
GAME_FN(void, __cdecl, ShowHudText3, 0x0068F580, const char* text, float);

// the options
GAME_VAR(uint8_t*, g_optionsScreen, 0x020C329C);
void* const kAudio = reinterpret_cast<void*>(0x00CF3DA0);
GAME_FN(int, __thiscall, Audio_GetVolume, 0x00594E60, void* audio, int category);
GAME_FN(void, __thiscall, Audio_VolumeUp, 0x00594FA0, void* audio, int category);
GAME_FN(void, __thiscall, Audio_VolumeDown, 0x00594FD0, void* audio, int category);
GAME_FN(void, __thiscall, Audio_SetOutput, 0x00595840, void* audio, int output);
void* const kStreamPlayer = reinterpret_cast<void*>(0x00CF4324);
GAME_FN(void, __thiscall, Stream_PlayByName, 0x00591330, void* player, const char* name, float volume, int loop,
        int channel);
GAME_VAR(uint8_t, g_audioOutput, 0x00C18713);
GAME_VAR(int32_t, g_brightness, 0x00C18708);
GAME_VAR(float, g_screenOffsetX, 0x00C1872C);
GAME_VAR(float, g_screenOffsetY, 0x00C18730);
GAME_ARRAY(float, g_barSize, 0x020C3040);            // "Bar Width", "Bar Height"
GAME_ARRAY(uint32_t, g_onOffHashes, 0x020C3048);     // on, off
GAME_ARRAY(uint32_t, g_outputHashes, 0x020C3050);    // [3]
GAME_ARRAY(uint32_t, g_languageHashes, 0x020C3064);  // [8]
GAME_VAR(float, g_resetHiliteX, 0x020C3084);
GAME_ARRAY(float, g_screenPosSaved, 0x020C3000);
GAME_FN(void, __cdecl, FE_PlayError, 0x006A2390);
GAME_FN(void, __cdecl, Pad_StopRumble, 0x00738780);
GAME_FN(void, __thiscall, Option_DrawLabel, 0x00696820, void* option, float x, float y, float dataX, int selected,
        int changing);
GAME_FN(void, __thiscall, SelList_Construct, 0x0069C3A0, void* list, void* entries, int count);

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

void UseMenuDrop() {
    g_fontDropColour[0] = g_menuColours[0];
    g_fontDropColour[3] = g_menuColours[3];
    g_fontDropColour[1] = g_menuColours[1];
    g_fontDropColour[2] = g_menuColours[2];
    g_fontDrop = 1;
}

// an option's colour and flag: changing 8, selected 3, otherwise 11
int OptionColour(int selected, int changing) {
    if (!re::Flag(changing)) {
        return re::Flag(selected) ? 3 : 0xB;
    }
    return 8;
}
int OptionFlag(int selected, int changing) {
    return (re::Flag(selected) || re::Flag(changing)) ? 1 : 0;
}

// the option at the current row of the current tab
uint8_t* CurrentOption(uint8_t* screen) {
    int8_t tab = SelList_Selected(screen + 0x16C);
    return At<uint8_t*>(screen, 0xD8 + (At<int32_t>(screen, 0x154) + tab * 10) * 4);
}

} // namespace

namespace felo {

// ---- FELoad --------------------------------------------------------------------------------

// 0x695490 (not in the function list): its three text styles and a position
void* __fastcall Load_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FELoad);
    InitStyle(self + 0x2C);
    InitStyle(self + 0x48);
    InitStyle(self + 0x64);
    At<int32_t>(self, 0x80) = 0;
    At<float>(self, 0x84) = 0.0f;
    At<float>(self, 0x88) = 0.0f;
    return self;
}

// 0x695520 (vf1): Load
void __fastcall Load_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    VCall<void>(archive, 0x38, self + 0xC, "Filetable XPos");
    VCall<void>(archive, 0x38, self + 0x10, "Filetable Right XPos ");
    VCall<void>(archive, 0x38, self + 0x14, "Filetable YPos");
    VCall<void>(archive, 0x38, self + 0x18, "Filetable Spacing");
    TextStyle_Load(self + 0x48, archive, g_loadStyleNames[0]);
    TextStyle_Load(self + 0x2C, archive, g_loadStyleNames[1]);
    At<uint8_t>(self, 0x46) = 1;
    TextStyle_Load(self + 0x64, archive, g_loadStyleNames[2]);
    Position_Load(self + 0x80, archive, g_loadStyleNames[3]);
}

// yes/no by up/down (the same in the questions' states)
static void YesNoInput(uint8_t* self) {
    if (g_menuUp != 0 && FE_PadPressed(0) != 0) {
        At<int32_t>(self, 0x20) = 0;
        return;
    }
    if (g_menuDown != 0 && FE_PadPressed(0) != 0) {
        At<int32_t>(self, 0x20) = 1;
    }
}

// 0x6955F0 (vf5)
void __fastcall Load_Process(uint8_t* self, re::Edx) {
    switch (At<int32_t>(self, 0x1C)) {
    case 0:
        // choosing a slot: accept on one with a save asks to load it
        if (Pad_ButtonJustDown(6, 0) != 0) {
            FE_PlaySelect();
            return;
        }
        if (Input_BackPressed(kUserInput, 0) != 0) {
            FE_PlaySelect();
            g_menuActive = 0;
            return;
        }
        if (At<uint8_t>(self, 0x28) == 0) {
            return;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0 && *SaveSys_GetSlotInfo(At<int32_t>(self, 0x24)) != 0) {
            FE_PlaySelect();
            At<int32_t>(self, 0x1C) = 1;
        }
        if (Pad_ButtonJustDown(8, 0) != 0) {
            return;
        }
        if (g_menuUp != 0 && FE_PadPressed(0) != 0) {
            At<int32_t>(self, 0x24) -= 1;
            if (At<int32_t>(self, 0x24) < 0) {
                At<int32_t>(self, 0x24) = 5;
            }
            FE_PadClear(0);
            return;
        }
        if (g_menuDown != 0 && FE_PadPressed(0) != 0) {
            At<int32_t>(self, 0x24) += 1;
            if (At<int32_t>(self, 0x24) >= 6) {
                At<int32_t>(self, 0x24) = 0;
            }
            FE_PadClear(0);
        }
        return;
    case 1:
        // load this one?
        if (Input_BackPressed(kUserInput, 0) != 0) {
            FE_PlaySelect();
            At<int32_t>(self, 0x1C) = 0;
            At<int32_t>(self, 0x20) = 1;
            return;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0) {
            FE_PlaySelect();
            if (At<int32_t>(self, 0x20) == 0) {
                SaveSys_Load(At<int32_t>(self, 0x24));
                At<uint8_t>(self, 0x29) = 1;
            }
            At<int32_t>(self, 0x1C) = 0;
            At<int32_t>(self, 0x20) = 1;
            return;
        }
        break;
    case 2:
        if (Input_BackPressed(kUserInput, 0) != 0) {
            FE_PlaySelect();
            At<int32_t>(self, 0x1C) = 0;
            At<int32_t>(self, 0x20) = 1;
            return;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0) {
            FE_PlaySelect();
            if (At<int32_t>(self, 0x20) == 0) {
                At<int32_t>(g_uberCompletion, 0x264) = 9;
                SaveSys_Load(At<int32_t>(self, 0x24));
            }
            At<int32_t>(self, 0x1C) = 0;
            At<int32_t>(self, 0x20) = 1;
            return;
        }
        break;
    case 3:
        if (Input_AcceptDown(kUserInput, 0) == 0) {
            return;
        }
        FE_PlaySelect();
        At<int32_t>(g_uberCompletion, 0x264) = 1;
        At<int32_t>(self, 0x1C) = 0;
        At<int32_t>(self, 0x20) = 1;
        return;
    case 4:
        // retry? (no, or back: the menu closes)
        if (Input_BackPressed(kUserInput, 0) != 0 ||
            (Input_AcceptDown(kUserInput, 0) != 0 && At<int32_t>(self, 0x20) == 1)) {
            FE_PlaySelect();
            g_menuActive = 0;
            return;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0 && At<int32_t>(self, 0x20) == 0) {
            FE_PlaySelect();
            if (SaveSys_736E70(0) != 0) {
                return;
            }
            At<int32_t>(g_uberCompletion, 0x264) = 6;
            VCall<void>(self, 0xC);
            return;
        }
        break;
    default:
        return;
    }
    YesNoInput(self);
}

// 0x6958A0: the highlight, yes and no from y down (the highlight's style moved for
// language 7 while it's drawn)
void __fastcall Load_DrawYesNoImpl(uint8_t* self, re::Edx, float yesY) {
    float lineHeight = g_fontLineHeights[g_fontSizeIndex] * g_fontScaleY;
    float noY = lineHeight + float(2.0 + double(yesY));
    float savedDx = At<float>(self, 0x34);
    if (g_language == 7) {
        At<float>(self, 0x34) = -7.0f;
    }
    float highlightY = At<int32_t>(self, 0x20) == 1 ? noY : yesY;
    FE_DrawKeyStyled(0x140, int(highlightY), "FESA_HILITE", self + 0x2C);
    if (g_language == 7) {
        At<float>(self, 0x34) = savedDx;
    }
    FE_DrawKeyStyled(0x140, int(yesY), "YES", At<int32_t>(self, 0x20) == 0 ? self + 0x2C : self + 0x48);
    FE_DrawKeyStyled(0x140, int(noY), "NO", At<int32_t>(self, 0x20) == 1 ? self + 0x2C : self + 0x48);
}

// 0x695990: the six slots: a save's details, or the slot's name and "not present"
void __fastcall Load_DrawFilesImpl(uint8_t* self, re::Edx) {
    static const char* const kSlots[6] = {"FEM_SL1", "FEM_SL2", "FEM_SL3", "FEM_SL4", "FEM_SL5", "FEM_SL6"};
    if (g_language == 7) {
        g_fileRowSpacing = 17.5f;
    }
    float scaleY = float(double(g_screenScaleY) * 0.5);
    for (int slot = 0; slot < 6; slot++) {
        const char* info = SaveSys_GetSlotInfo(slot);
        int32_t colour = At<int32_t>(self, 0x5C);
        float y = float(At<int32_t>(self, 0x18) * slot + At<int32_t>(self, 0x14));
        if (slot == At<int32_t>(self, 0x24)) {
            colour = At<int32_t>(self, 0x40);
        }
        if (*info != 0) {
            FE_DrawSaveInfo(info, float(At<int32_t>(self, 0xC)), float(At<int32_t>(self, 0x10)), y,
                            g_fileRowSpacing + y, colour, 0.39f, scaleY, colour == At<int32_t>(self, 0x40));
            continue;
        }
        float rect[4] = {1e6f, -1e6f, -1e6f, 1e6f};
        Font_Reset();
        g_fontColour[0] = g_menuColours[colour * 4];
        g_fontColour[1] = g_menuColours[colour * 4 + 1];
        g_fontColour[2] = g_menuColours[colour * 4 + 2];
        g_fontColour[3] = g_menuColours[colour * 4 + 3];
        if (slot == At<int32_t>(self, 0x24)) {
            g_fontDropColour[0] = g_menuColours[0];
            g_fontDropColour[1] = g_menuColours[1];
            g_fontDropColour[2] = g_menuColours[2];
            g_fontDropColour[3] = g_menuColours[3];
            g_fontDrop = 1;
        } else {
            g_fontDrop = 0;
        }
        Font_SetJustify(0);
        Font_SetScale(0.39f, scaleY);
        float x = float(At<int32_t>(self, 0xC));
        Font_PrintString(x, y, Text_Get(kTheText, kSlots[slot]), 1);
        Font_GetExtent(rect, 0);
        FE_DrawHighlight(std::fabs(rect[2] - rect[0]) + x, float(At<int32_t>(self, 0x10)), y);
        Font_PrintString(x, g_fileRowSpacing + y, Text_Get(kTheText, "FEM_NOT_PRESENT"), 1);
        Font_GetExtent(rect, 0);
        FE_DrawHighlight(std::fabs(rect[2] - rect[0]) + x, float(At<int32_t>(self, 0x10)), g_fileRowSpacing + y);
    }
}

// 0x695C60 (vf6): by the save system's state: the table, questions, messages
void __fastcall Load_Draw(uint8_t* self, re::Edx) {
    UseMenuDrop();
    FE_DrawTextKey(0x140, 87, "FEM_LOAD_HEADER", 8, 2, 1, 1);
    g_menuFlagsByte &= 0xEF;
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    At<uint8_t>(self, 0x28) = 0;
    if (g_lastSaveState != SaveSys_GetState()) {
        At<int32_t>(self, 0x20) = 1;
        g_lastSaveState = SaveSys_GetState();
    }
    int32_t state = SaveSys_GetState();
    if (state == 1) {
        g_loadingGame = 1;
        At<int32_t>(g_uberCompletion, 0x264) = 7;
        At<int32_t>(self, 0x24) = SaveSys_FirstUsedSlot(0);
        if (At<uint8_t>(self, 0x29) != 0) {
            At<int32_t>(g_uberCompletion, 0x264) = 6;
            g_menuActive = 0;
            FE_ExitToMainMenu();
        }
        FE_DrawTitle(nullptr);
        return;
    }
    if (state == 7) {
        const char* question;
        switch (At<int32_t>(self, 0x1C)) {
        case 4:
            At<int32_t>(self, 0x1C) = 0;
            [[fallthrough]];
        case 0:
            At<uint8_t>(self, 0x28) = 1;
            Load_DrawFiles(self);
            FE_DrawTitle("FELO_CONTROLS");
            return;
        case 1:
            question = "FESZ_QL";
            break;
        case 2:
            question = "FESZ_QD";
            break;
        default:
            return;
        }
        g_fontWrapX = 400.0f;
        FE_DrawKeyStyled(0x140, int(Position_GetY(self + 0x80)), question, self + 0x48);
        Load_DrawYesNo(self, float(double(g_lastTextRect[3]) + 10.0));
        FE_DrawTitle("MC_YESNO");
        return;
    }
    if (state == 0x17) {
        At<int32_t>(self, 0x1C) = 3;
        FE_DrawMessageStyled(self + 0x80, "RECRE_S", self + 0x64);
        FE_DrawTitle("FE_CONTINUE");
        return;
    }
    At<int32_t>(self, 0x1C) = 0;
    At<uint8_t>(g_uberCompletion, 0x269) = 1;
    switch (SaveSys_GetState()) {
    case 2:
    case 4:
    case 6:
    case 0x11:
    case 0x15:
        SaveSys_DrawMessage(self + 0x80, self + 0x64);
        FE_DrawTitle(nullptr);
        return;
    case 5:
    case 0x19:
        At<int32_t>(self, 0x1C) = 4;
        SaveSys_DrawMessage(self + 0x80, self + 0x64);
        g_fontWrapX = 400.0f;
        FE_DrawKeyStyled(0x140, int(float(double(g_lastTextRect[3]) + 30.0)), "RETRY_Q", self + 0x48);
        Load_DrawYesNo(self, float(double(g_lastTextRect[3]) + 10.0));
        FE_DrawTitle("MC_YESNO");
        if (SaveSys_7368C0(0) != 0) {
            At<int32_t>(g_uberCompletion, 0x264) = 6;
            VCall<void>(self, 0xC);
        }
        return;
    }
}

// ---- FEMap / FEMessage ----------------------------------------------------------------------

// 0x695FE0 / 0x696140 (not in the function list): the constructors (the vtable only)
void* __fastcall Map_Construct(void* self, re::Edx) {
    SetVtable(self, VT_FEMap);
    return self;
}
void* __fastcall Message_Construct(void* self, re::Edx) {
    SetVtable(self, VT_FEMessage);
    return self;
}

// 0x696020 (vf5): back: the radar's mode 1, accept: mode 2 (not when it's that already)
void __fastcall Map_Process(void*, re::Edx) {
    uint8_t* radar = static_cast<uint8_t*>(g_radar);
    if (Input_BackPressed(kUserInput, 0) != 0) {
        if (At<int32_t>(radar, 0xE788) == 1) {
            return;
        }
        FE_PlaySelect();
        Radar_SetMapMode(radar, 1);
        Monitor_5EE1D0();
        return;
    }
    if (Input_AcceptDown(kUserInput, 0) != 0 && At<int32_t>(radar, 0xE788) != 2) {
        FE_PlaySelect();
        Radar_SetMapMode(radar, 2);
        Monitor_5EE1D0();
    }
}

// 0x696090 (vf6): the background (unless the radar draws its own), two windows, the title
// (not in mode 2), the radar's map
void __fastcall Map_Draw(void*, re::Edx) {
    uint8_t* radar = static_cast<uint8_t*>(g_radar);
    if (At<uint8_t>(radar, 0xE7E8) == 0) {
        FE_DrawBackground(g_mapBackground);
    }
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    FE_DrawWindow(320.0f, 440.0f, 394.0f, kWindowColour, 2.0f);
    if (At<int32_t>(radar, 0xE788) != 2) {
        FE_DrawTitle("FE_SELECT_Z");
    }
    VCall<void>(radar, 0x20);
}

// 0x696150: the message and whether it has a black backdrop
void __fastcall Message_Set(uint8_t* self, re::Edx, const char* text, int backdrop) {
    Strncpy(reinterpret_cast<char*>(self + 0xC), text, 0xFF);
    At<uint8_t>(self, 0x10B) = uint8_t(backdrop);
}

// 0x696180 (vf6): drawn over the game on its own
void __fastcall Message_Draw(uint8_t* self, re::Edx) {
    int backdrop = At<uint8_t>(self, 0x10B) != 0 ? 0xFF : 0;
    if (Screen_BeginOverlay(0, 0, 0, 0, 0, 0, backdrop, 0, 0, 0, 0x280, 0x1E0, 0) == 0) {
        return;
    }
    Render_4094B0(7);
    Font_68A250();
    Monitor_5EE1D0();
    uint8_t white[4];
    RGBA_Construct(white, 255, 255, 255, 255);
    if (At<uint8_t>(self, 0x10B) != 0) {
        float rect[4] = {0.0f, 480.0f, 640.0f, 0.0f};
        uint8_t black[4];
        DrawRect(rect, RGBA_Construct(black, 0, 0, 0, 255));
    }
    ShowHudText3(reinterpret_cast<const char*>(self + 0xC), 1.0f);
    Font_Render();
    Screen_EndOverlay(0);
}

// ---- FEOptions ------------------------------------------------------------------------------

// 0x696270 (vf1): Load
void __fastcall Options_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    VCall<void>(archive, 0x28, self + 0x15C, "Text Start XPos");
    VCall<void>(archive, 0x28, self + 0x160, "Text Start YPos");
    VCall<void>(archive, 0x28, self + 0x164, "Data XOffset");
    VCall<void>(archive, 0x28, self + 0x168, "Text Spacing");
    VCall<void>(archive, 0x28, &g_barSize[0], "Bar Width");
    VCall<void>(archive, 0x28, &g_barSize[1], "Bar Height");
    void* tabs = self + 0x16C;
    SelList_Load(tabs, archive);
    SelList_Clear(tabs);
    SelList_AddByName(tabs, "FEO_AUD");
    SelList_AddByName(tabs, "FEO_DIS");
    SelList_AddByName(tabs, "FEO_MSC");
}

// 0x696340 (vf4): leaving mid-change restores the option
void __fastcall Options_OnLeave(uint8_t* self, re::Edx) {
    Pad_StopRumble();
    if (At<uint8_t>(self, 0x158) == 0) {
        return;
    }
    uint8_t* option = CurrentOption(self);
    if (option != nullptr) {
        VCall<void>(option, 0x14);
    }
}

// 0x696380 (vf5): changing: back restores, accept applies, kind 1 steps by left/right,
// kind 2 handles its own input; otherwise rows by up/down, tabs by left/right, accept
// starts changing (Save)
void __fastcall Options_Process(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x158) != 0) {
        uint8_t* option = CurrentOption(self);
        if (Input_BackPressed(kUserInput, 0) != 0) {
            VCall<void>(option, 0x14);
            FE_PlaySelect();
            At<uint8_t>(self, 0x158) = 0;
        }
        if (Input_AcceptDown(kUserInput, 0) != 0) {
            VCall<void>(option, 8);
            FE_PlaySelect();
            At<uint8_t>(self, 0x158) = 0;
        }
        uint8_t kind = At<uint8_t>(option, 4);
        if (kind != 1) {
            if (kind == 2) {
                VCall<void>(option, 0x18);
            }
            return;
        }
        int up;
        if (g_menuLeft != 0 && FE_PadPressed(0) != 0) {
            up = 0;
        } else if (g_menuRight != 0 && FE_PadPressed(0) != 0) {
            up = 1;
        } else {
            return;
        }
        if (VCall<uint8_t>(option, 0x18, up) != 0) {
            FE_PadClear(0);
        }
        return;
    }
    uint32_t rows = At<uint8_t>(self, 0x150 + SelList_Selected(self + 0x16C));
    if (Input_BackPressed(kUserInput, 0) != 0) {
        FE_PlaySelect();
        g_menuActive = 0;
        return;
    }
    if (Input_AcceptDown(kUserInput, 0) != 0) {
        uint8_t* option = CurrentOption(self);
        FE_PlaySelect();
        VCall<void>(option, 0x10);
        At<uint8_t>(self, 0x158) = 1;
        return;
    }
    bool moved = false;
    if (g_menuUp != 0 && FE_PadPressed(0) != 0) {
        At<int32_t>(self, 0x154) -= 1;
        if (At<int32_t>(self, 0x154) < 0) {
            At<int32_t>(self, 0x154) = int32_t(rows) - 1;
        }
        moved = true;
    } else if (g_menuDown != 0 && FE_PadPressed(0) != 0) {
        At<int32_t>(self, 0x154) += 1;
        if (At<int32_t>(self, 0x154) >= int32_t(rows)) {
            At<int32_t>(self, 0x154) = 0;
        }
        moved = true;
    }
    if (moved) {
        FE_PadClear(0);
    }
    void* tabs = self + 0x16C;
    bool changed = false;
    if (g_menuRight != 0 && FE_PadPressed(0) != 0) {
        int8_t before = SelList_Selected(tabs);
        changed = before != SelList_Next(tabs);
    } else if (g_menuLeft != 0 && FE_PadPressed(0) != 0) {
        int8_t before = SelList_Selected(tabs);
        changed = before != SelList_Previous(tabs);
    } else {
        return;
    }
    if (changed) {
        FE_PadClear(0);
        At<int32_t>(self, 0x154) = 0;
    }
}

// 0x696620: every option of the three tabs Init then Apply
void __fastcall Options_InitAndApply(uint8_t* self, re::Edx) {
    for (int tab = 0; tab < 3; tab++) {
        for (uint32_t i = 0; i < At<uint8_t>(self, 0x150 + tab); i++) {
            uint8_t* option = At<uint8_t*>(self, 0xD8 + (tab * 10 + int(i)) * 4);
            VCall<void>(option, 0);
            VCall<void>(option, 0xC);
        }
    }
}

// 0x696690 (vf6): the tab's options (the reset one alone while it asks), the tabs, the title
void __fastcall Options_Draw(uint8_t* self, re::Edx) {
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    uint8_t* reset = self + 0xC8;
    if (CurrentOption(self) == reset && At<uint8_t>(self, 0x158) != 0) {
        VCall<void>(reset, 4, At<float>(self, 0x15C), 0.0f, At<float>(self, 0x164), 1, 1);
    } else {
        uint32_t rows = At<uint8_t>(self, 0x150 + SelList_Selected(self + 0x16C));
        for (int row = 0; row < int(rows); row++) {
            int8_t tab = SelList_Selected(self + 0x16C);
            float y = float(row) * At<float>(self, 0x168) + At<float>(self, 0x160);
            int selected = row == At<int32_t>(self, 0x154) ? 1 : 0;
            int changing = (selected != 0 && At<uint8_t>(self, 0x158) != 0) ? 1 : 0;
            uint8_t* option = At<uint8_t*>(self, 0xD8 + (row + tab * 10) * 4);
            VCall<void>(option, 4, At<float>(self, 0x15C), y, At<float>(self, 0x164), selected, changing);
        }
    }
    SelList_Draw(self + 0x16C);
    FE_DrawTitle(At<uint8_t>(self, 0x158) == 0 ? "FEO_CONTROLS" : "FEO_CHGCTRL");
}

// 0x696820: an option's label
void __fastcall Option_DrawLabelImpl(COption* self, re::Edx, float x, float y, float, int selected, int changing) {
    const uint16_t* text = Text_GetByHash(kTheText, self->nameKey);
    FE_DrawText(int(x), int(y), text, OptionColour(selected, changing), 0, 1, OptionFlag(selected, changing));
}

// The options' Draw (vf1): the label, the value right-aligned at x + dataX, the highlight
// between them
static void DrawWithValue(COption* self, float x, float y, float dataX, int selected, int changing,
                          const uint16_t* value) {
    Option_DrawLabel(self, x, y, dataX, selected, changing);
    float labelLeft = g_lastTextRect[0];
    float labelRight = g_lastTextRect[2];
    int colour = OptionColour(selected, changing);
    int flag = OptionFlag(selected, changing);
    float valueX = x + dataX;
    FE_DrawText(int(valueX), int(y), value, colour, 1, 1, flag);
    float valueWidth = std::fabs(g_lastTextRect[2] - g_lastTextRect[0]);
    FE_DrawHighlight(std::fabs(labelRight - labelLeft) + x, valueX - valueWidth, y);
}

// 0x6974B0: on/off (vibration, subtitles, invert camera)
void __fastcall Toggle_Draw(COptionValue* self, re::Edx, float x, float y, float dataX, int selected, int changing) {
    DrawWithValue(self, x, y, dataX, selected, changing,
                  Text_GetByHash(kTheText, g_onOffHashes[self->value == 0 ? 1 : 0]));
}

// 0x697630 / 0x6977B0 / 0x697930: "volume/10" (music 1, effects 4, speech 2)
template <int Category>
void __fastcall Volume_Draw(COption* self, re::Edx, float x, float y, float dataX, int selected, int changing) {
    char text[12];
    uint16_t wide[12];
    Sprintf(text, "%d/%d", Audio_GetVolume(kAudio, Category), 10);
    AsciiToWide(text, wide);
    DrawWithValue(self, x, y, dataX, selected, changing, wide);
}

// 0x697AB0: the audio output
void __fastcall AudioOutput_Draw(COptionValue* self, re::Edx, float x, float y, float dataX, int selected,
                                 int changing) {
    DrawWithValue(self, x, y, dataX, selected, changing, Text_GetByHash(kTheText, g_outputHashes[self->value]));
}

// 0x697C10: "level/10"
void __fastcall Brightness_Draw(CBrightnessOption* self, re::Edx, float x, float y, float dataX, int selected,
                                int changing) {
    char text[12];
    uint16_t wide[12];
    Sprintf(text, "%d/%d", uint32_t(self->level), 10);
    AsciiToWide(text, wide);
    DrawWithValue(self, x, y, dataX, selected, changing, wide);
}

// 0x697D90: the label alone (the highlight to x + dataX)
void __fastcall AdjustScreenPos_Draw(COption* self, re::Edx, float x, float y, float dataX, int selected, int changing) {
    const uint16_t* text = Text_GetByHash(kTheText, self->nameKey);
    FE_DrawText(int(x), int(y), text, OptionColour(selected, changing), 0, 1, OptionFlag(selected, changing));
    FE_DrawHighlight(x + std::fabs(g_lastTextRect[2] - g_lastTextRect[0]), x + dataX, y);
}

// 0x697E70: the language's name
void __fastcall Language_Draw(COptionValue* self, re::Edx, float x, float y, float dataX, int selected, int changing) {
    DrawWithValue(self, x, y, dataX, selected, changing, Text_GetByHash(kTheText, g_languageHashes[self->value]));
}

// 0x697FD0: asking (it's the option being changed): the question, yes / no with the
// highlight; otherwise the label
void __fastcall ResetToDefaults_Draw(CResetToDefaultsOption* self, re::Edx, float x, float y, float dataX, int selected,
                                     int changing) {
    uint8_t* screen = g_optionsScreen;
    if (CurrentOption(screen) == reinterpret_cast<uint8_t*>(self) && At<uint8_t>(screen, 0x158) != 0) {
        g_fontWrapX = 530.0f;
        FE_DrawTextKey(0x140, 0x96, "FEO_RESETDEF", 0xB, 2, 3, 0);
        const float ys[2] = {260.0f, 280.0f};
        static const char* const kKeys[2] = {"FEM_YES", "FEM_NO"};
        for (int i = 0; i < 2; i++) {
            int colour;
            bool chosen = i == self->unknown0C;
            if (chosen) {
                UseMenuDrop();
                colour = 3;
            } else {
                colour = 0xB;
                g_fontDrop = 0;
            }
            int yInt = int(ys[i]);
            FE_DrawTextKey(0x140, yInt, kKeys[i], colour, 2, 3, chosen ? 1 : 0);
            if (chosen) {
                if (g_language == 7) {
                    g_resetHiliteX = -7.0f;
                }
                FE_DrawTextKey(int(double(g_resetHiliteX) + 320.0), yInt, "FESA_HILITE", colour, 2, 3, 1);
            }
        }
        return;
    }
    Option_DrawLabel(self, x, y, dataX, selected, changing);
    FE_DrawHighlight(x + std::fabs(g_lastTextRect[2] - g_lastTextRect[0]), x + dataX, y);
}

// 0x696960 (vf6): the music's sample while it's changed; left/right step the volume (a
// sound when it changed)
void __fastcall MusicVolume_HandleInput(CVolumeOption* self, re::Edx) {
    uint8_t* screen = g_optionsScreen;
    if (self->previewing == 0 && CurrentOption(screen) == reinterpret_cast<uint8_t*>(self) &&
        At<uint8_t>(screen, 0x158) != 0) {
        Stream_PlayByName(kStreamPlayer, "MS_RunningLow.rsm", 1.0f, 1, 6);
        self->previewing = 1;
    }
    if (g_menuLeft != 0 && FE_PadPressed(0) != 0) {
        int before = Audio_GetVolume(kAudio, 1);
        Audio_VolumeDown(kAudio, 1);
        if (before != Audio_GetVolume(kAudio, 1)) {
            FE_PlaySound(9, 0);
        }
        return;
    }
    if (g_menuRight != 0 && FE_PadPressed(0) != 0) {
        int before = Audio_GetVolume(kAudio, 1);
        Audio_VolumeUp(kAudio, 1);
        if (before != Audio_GetVolume(kAudio, 1)) {
            FE_PlaySound(10, 0);
        }
    }
}

// 0x696CF0 (vf7 Step): 0 .. 2, applied at once; changed (false at the ends)
uint8_t __fastcall AudioOutput_Step(COptionValue* self, re::Edx, int up) {
    uint8_t changed = 1;
    if (!re::Flag(up)) {
        self->value = int8_t(self->value - 1);
        if (self->value < 0) {
            changed = 0;
            self->value = int8_t(self->value + 1);
        }
    } else {
        self->value = int8_t(self->value + 1);
        if (self->value >= 3) {
            changed = 0;
            self->value = int8_t(self->value - 1);
        }
    }
    uint8_t output = 1;
    if (self->value == 0) {
        output = 0;
    } else if (self->value != 1 && self->value == 2) {
        output = 2;
    }
    Audio_SetOutput(kAudio, output);
    g_audioOutput = output;
    VCall<void>(self, 0xC);
    return changed;
}

// 0x696E80 (vf6): the level by left/right (the setting 150.5 .. 280.5, truncated); at an
// end: the error sound (once)
void __fastcall Brightness_HandleInput(CBrightnessOption* self, re::Edx) {
    if (g_menuLeft != 0 && FE_PadPressed(0) != 0) {
        if (self->level != 0) {
            FE_PadClear(0);
            self->level = uint16_t(self->level - 1);
            g_brightness = int32_t(float(float(self->level) / 10.0f) * 130.0f + 150.5f);
            self->changed = 1;
            return;
        }
    } else {
        if (g_menuRight == 0 || FE_PadPressed(0) == 0) {
            self->changed = 0;
            return;
        }
        if (self->level < 10) {
            FE_PadClear(0);
            self->level = uint16_t(self->level + 1);
            g_brightness = int32_t(float(float(self->level) / 10.0f) * 130.0f + 150.5f);
            self->changed = 1;
            return;
        }
    }
    if (self->changed != 0) {
        FE_PlayError();
    }
    self->changed = 0;
}

// 0x696FB0 (vf6): the offsets by 2 (x, within the option's +-100) and 1 (y, +-50); past a
// limit: clamped with the error sound (once); then applied (vf3)
void __fastcall AdjustScreenPos_HandleInput(CAdjustScreenPosOption* self, re::Edx) {
    float limitX = self->x < 100.0f ? self->x : 100.0f;
    float limitY = self->y < 50.0f ? self->y : 50.0f;
    auto ok = [self]() {
        FE_PlaySelect();
        self->adjusting = 1;
    };
    auto clamp = [self](float& offset, float value) {
        if (self->adjusting != 0) {
            FE_PlayError();
            self->adjusting = 0;
        }
        offset = value;
    };
    if (g_menuLeft != 0) {
        g_screenOffsetX = float(double(g_screenOffsetX) - 2.0);
        float low = -limitX;
        if (low > g_screenOffsetX) {
            clamp(g_screenOffsetX, low);
        } else {
            ok();
        }
    } else if (g_menuRight != 0) {
        g_screenOffsetX = float(double(g_screenOffsetX) + 2.0);
        if (limitX < g_screenOffsetX) {
            clamp(g_screenOffsetX, limitX);
        } else {
            ok();
        }
    } else if (g_menuUp != 0) {
        g_screenOffsetY = float(double(g_screenOffsetY) - 1.0);
        float low = -limitY;
        if (low > g_screenOffsetY) {
            clamp(g_screenOffsetY, low);
        } else {
            ok();
        }
    } else if (g_menuDown != 0) {
        g_screenOffsetY = float(double(g_screenOffsetY) + 1.0);
        if (limitY < g_screenOffsetY) {
            clamp(g_screenOffsetY, limitY);
        } else {
            ok();
        }
    } else {
        self->adjusting = 0;
    }
    VCall<void>(self, 0xC);
}

// 0x697380 (vf6): up or down flips yes/no
void __fastcall ResetToDefaults_HandleInput(CResetToDefaultsOption* self, re::Edx) {
    if ((g_menuUp != 0 && FE_PadPressed(0) != 0) || (g_menuDown != 0 && FE_PadPressed(0) != 0)) {
        self->unknown0C = (self->unknown0C + 1) % 2;
        FE_PadClear(0);
    }
}

// the options (0x10 or 0x14 bytes: vtable, kind, name hash) made in place
static void MakeOption(uint8_t* option, uintptr_t vtable, uint8_t kind, const char* name) {
    SetVtable(option, vtable);
    At<uint8_t>(option, 4) = kind;
    At<uint32_t>(option, 8) = 0;
    At<uint32_t>(option, 8) = HashName(name);
}

// 0x698220 (not in the function list): FEOptions' constructor: its options and tabs
void* __fastcall Options_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEOptions);
    MakeOption(self + 0xC, 0x0093FEEC, 1, "FEC_VIB");
    MakeOption(self + 0x20, 0x0093FD24, 2, "VOLMSC");
    At<uint8_t>(self, 0x30) = 0;
    MakeOption(self + 0x34, 0x0093FD4C, 2, "VOLSFX");
    MakeOption(self + 0x44, 0x0093FD78, 2, "VOLSPEECH");
    MakeOption(self + 0x54, 0x0093FDA8, 1, "AUDOP");
    MakeOption(self + 0x64, 0x0093FDD0, 2, "DISPB");
    MakeOption(self + 0x78, 0x0093FDF8, 2, "DISPASP");
    At<float>(self, 0x88) = g_screenPosSaved[0];
    At<float>(self, 0x8C) = g_screenPosSaved[1];
    MakeOption(self + 0x98, 0x0093FE20, 1, "DISPST");
    MakeOption(self + 0xA8, 0x0093FE48, 1, "DISPL");
    MakeOption(self + 0xB8, 0x0093FEC0, 1, "FEO_FPINV");
    MakeOption(self + 0xC8, 0x0093FE70, 2, "DISDEF");
    SelList_Construct(self + 0x16C, self + 0x16C + 0x70, 3);
    SetVtable(self + 0x16C, VT_SelListT2);
    At<uint16_t>(self, 0x150) = 0;
    At<uint8_t>(self, 0x152) = 0;
    auto add = [self](int tab, uint8_t* option) {
        uint8_t& count = At<uint8_t>(self, 0x150 + tab);
        At<uint8_t*>(self, 0xD8 + (tab * 10 + count) * 4) = option;
        count = uint8_t(count + 1);
    };
    add(0, self + 0x20);
    add(0, self + 0x34);
    add(0, self + 0x44);
    add(0, self + 0x54);
    add(1, self + 0x64);
    add(1, self + 0x98);
    add(2, self + 0xB8);
    add(2, self + 0xC);
    add(2, self + 0xC8);
    return self;
}

} // namespace felo

// 0x695FF0 (FEBiology's and others' vf1): a lone jmp to FrontEndScreen's Load
RE_UNHOOKED("FrontEndScreen::Load thunk (jmp 0x6A7150)", 0x00695FF0);

#define FELO_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(felo_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(felo_original_, __LINE__), \
             re::Mode::Original)

FELO_REVERSED("FELoad::FELoad", 0x00695490, &felo::Load_Construct);
FELO_REVERSED("FELoad::Load", 0x00695520, &felo::Load_Load);
FELO_REVERSED("FELoad::Process", 0x006955F0, &felo::Load_Process);
FELO_REVERSED("FELoad::DrawYesNo", 0x006958A0, &felo::Load_DrawYesNoImpl);
FELO_REVERSED("FELoad::DrawFiles", 0x00695990, &felo::Load_DrawFilesImpl);
FELO_REVERSED("FELoad::Draw", 0x00695C60, &felo::Load_Draw);
FELO_REVERSED("FEMap::FEMap", 0x00695FE0, &felo::Map_Construct);
FELO_REVERSED("FEMap::Process", 0x00696020, &felo::Map_Process);
FELO_REVERSED("FEMap::Draw", 0x00696090, &felo::Map_Draw);
FELO_REVERSED("FEMessage::FEMessage", 0x00696140, &felo::Message_Construct);
FELO_REVERSED("FEMessage::Set", 0x00696150, &felo::Message_Set);
FELO_REVERSED("FEMessage::Draw", 0x00696180, &felo::Message_Draw);
FELO_REVERSED("FEOptions::Load", 0x00696270, &felo::Options_Load);
FELO_REVERSED("FEOptions::OnLeave", 0x00696340, &felo::Options_OnLeave);
FELO_REVERSED("FEOptions::Process", 0x00696380, &felo::Options_Process);
FELO_REVERSED("FEOptions::InitAndApply", 0x00696620, &felo::Options_InitAndApply);
FELO_REVERSED("FEOptions::Draw", 0x00696690, &felo::Options_Draw);
FELO_REVERSED("COption::DrawLabel", 0x00696820, &felo::Option_DrawLabelImpl);
FELO_REVERSED("CMusicVolumeOption::HandleInput", 0x00696960, &felo::MusicVolume_HandleInput);
FELO_REVERSED("CAudioOutputOption::Step", 0x00696CF0, &felo::AudioOutput_Step);
FELO_REVERSED("CBrightnessOption::HandleInput", 0x00696E80, &felo::Brightness_HandleInput);
FELO_REVERSED("CAdjustScreenPosOption::HandleInput", 0x00696FB0, &felo::AdjustScreenPos_HandleInput);
FELO_REVERSED("CResetToDefaultsOption::HandleInput", 0x00697380, &felo::ResetToDefaults_HandleInput);
FELO_REVERSED("CToggleOption::Draw", 0x006974B0, &felo::Toggle_Draw);
FELO_REVERSED("CMusicVolumeOption::Draw", 0x00697630, &felo::Volume_Draw<1>);
FELO_REVERSED("CSFXVolumeOption::Draw", 0x006977B0, &felo::Volume_Draw<4>);
FELO_REVERSED("CSpeechVolumeOption::Draw", 0x00697930, &felo::Volume_Draw<2>);
FELO_REVERSED("CAudioOutputOption::Draw", 0x00697AB0, &felo::AudioOutput_Draw);
FELO_REVERSED("CBrightnessOption::Draw", 0x00697C10, &felo::Brightness_Draw);
FELO_REVERSED("CAdjustScreenPosOption::Draw", 0x00697D90, &felo::AdjustScreenPos_Draw);
FELO_REVERSED("CLanguageOption::Draw", 0x00697E70, &felo::Language_Draw);
FELO_REVERSED("CResetToDefaultsOption::Draw", 0x00697FD0, &felo::ResetToDefaults_Draw);
FELO_REVERSED("FEOptions::FEOptions", 0x00698220, &felo::Options_Construct);
