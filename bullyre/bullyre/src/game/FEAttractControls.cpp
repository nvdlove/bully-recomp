// The frontend's attract mode, background and biology screens, and the controls screen
// (FEControls: its key maps for DirectInput and XInput pads and the other control
// options). 0x6905F0..0x692832.
//
// FEControls (0x8C0 bytes):
//   +0x0C..0x20 its window (zeroed)       +0xD8  FESelectionListWindowT<1>: the two tabs
//   +0x24  [25] the DI actions' text hashes ("<button>_D")
//   +0x88  [16] the XInput actions' ("<button>_X")
//   +0xC8  the selection   +0xCC listening for a key   +0xD0 the pad listened to (-1)
//   +0xD4  the value column's x (from "Data XOffset")   +0x108 / +0x10C colours
//   +0x150 the DI list (FEScrollingSelectionListWindowT<0x1D>), its 25 DIButtonMaps at +0x22C
//   +0x54C the XInput list (<0x12>), its 16 XButtonMaps at +0x5FC
//   +0x7FC CEnableXInput  +0x81C CMouseSensitivityOption  +0x83C CCamControlSteeringOption
//   +0x85C CLockMovementDirOption  +0x87C / +0x89C CResetKeyMapOption (XInput / DI)
//   +0x8BC skip the next accept
// A control option (0x20 bytes): +0 vtable, +4 selectable, +8 its text's hash, +0xC
// listening, +0x10 / +0x14 colours (selected / not), +0x18 the value's x offset, +0x1C its
// value (an action index, a flag or a float). Its vtable: 0 GetExtent, 1 Draw, 2 GetHeight,
// 3 DrawValue, 4 GetValueText(out, size, selected), 5 HandleInput(pad) -> handled,
// 6 IsXInput (the key maps).

#include <cmath>

#include "game/FEScreensCommon.h"

using namespace fescr;

namespace {

constexpr uintptr_t VT_FEAttractMode = 0x0093F1E4;
constexpr uintptr_t VT_FEBackground = 0x0093F230;
constexpr uintptr_t VT_FEBiology = 0x0093F250;
constexpr uintptr_t VT_FEControls = 0x0093F3B4;
constexpr uintptr_t VT_SelListT1 = 0x0093F300;
constexpr uintptr_t VT_ScrollListT1D = 0x0093F308;
constexpr uintptr_t VT_ScrollListT12 = 0x0093F318;
constexpr uintptr_t VT_DIButtonMap = 0x0093F47C;
constexpr uintptr_t VT_XButtonMap = 0x0093F49C;
constexpr uintptr_t VT_CEnableXInput = 0x0093F328;
constexpr uintptr_t VT_CMouseSensitivity = 0x0093F344;
constexpr uintptr_t VT_CCamControlSteering = 0x0093F360;
constexpr uintptr_t VT_CLockMovementDir = 0x0093F37C;
constexpr uintptr_t VT_CResetKeyMap = 0x0093F398;

// the settings (keys and the control flags)
void* const kButtonMap = reinterpret_cast<void*>(0x00A14810);
GAME_ARRAY(int32_t, g_xinputKeys, 0x00A14810);          // [16]
GAME_VAR(float, g_mouseSensitivityDefault, 0x00A1491C);
GAME_VAR(float, g_mouseSensitivity, 0x00A14920);         // (0: the default)
GAME_VAR(uint8_t, g_mouseSteerVehicle, 0x00A14925);
GAME_VAR(uint8_t, g_mouseSteerFoot, 0x00A14926);
GAME_VAR(int32_t, g_xinputPad, 0x020CECE8);
GAME_VAR(int32_t, g_dinputPad, 0x00BC7490);
GAME_FN(const int32_t*, __thiscall, ButtonMap_GetKeys, 0x00454CD0, void* map, int saved);
GAME_FN(void, __thiscall, ButtonMap_ResetX, 0x004549E0, void* map);
GAME_FN(void, __thiscall, ButtonMap_SetDefaultKeys, 0x00454E00, void* map);
GAME_FN(void, __thiscall, ButtonMap_Save, 0x00454B70, void* map);
GAME_FN(int32_t, __cdecl, Pad_PollKey, 0x007384B0, int pad);
GAME_FN(void, __cdecl, Pad_KeysSwapped, 0x00738540, int pad, int32_t oldKey, int32_t newKey);
GAME_FN(int32_t, __cdecl, Pad_BindingKind, 0x007384D0, int32_t pad, int32_t key, int32_t* a, int32_t* button);
GAME_FN(const char*, __cdecl, ButtonName, 0x005DAE60, uint32_t button);
GAME_FN(void, __cdecl, Pad_Reset738660, 0x00738660);

// attract mode's movie and music
GAME_FN(void*, __cdecl, MoviePlayer_Get, 0x00411370);
void* const kStreamPlayer = reinterpret_cast<void*>(0x00CF4324);
GAME_FN(void, __thiscall, Stream_Stop, 0x005913D0, void* player, int channel, int fadeMs);
GAME_FN(void, __thiscall, Stream_PlayByName, 0x00591330, void* player, const char* name, float volume, int loop,
        int channel);
void* const kAttractMovie = reinterpret_cast<void*>(0x020C3220);
GAME_ARRAY(uint8_t, g_attractTipColour, 0x020C333C);
GAME_ARRAY(uint8_t, g_attractTipDrop, 0x020C331C);
void* const kBackdropColour = reinterpret_cast<void*>(0x020C2FC0);
void* const kIconColour = reinterpret_cast<void*>(0x020C2FC4);
void* const kWindowColour = reinterpret_cast<void*>(0x020C33E4);

// biology
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* key);
GAME_FN(int32_t, __thiscall, MissionMgr_Find, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetTaskFlags, 0x006AA7C0, void* mgr, int32_t mission);
GAME_FN(void, __thiscall, MissionMgr_6AD290, 0x006AD290, void* mgr, int);
GAME_FN(void, __thiscall, MissionMgr_Start, 0x006AD100, void* mgr, int32_t mission, int);
GAME_FN(void, __thiscall, Ped_474C10, 0x00474C10, void* ped, int, int);

GAME_FN(void, __thiscall, SelList_Construct, 0x0069C3A0, void* list, void* entries, int count);
GAME_FN(void, __thiscall, ScrollList_Construct, 0x0069B1C0, void* list, int unused);
GAME_FN(float, __thiscall, ControlOption_DrawValueText, 0x00691780, void* option, float x, float y, int selected);

// the text key's text (a pad button name etc.) into `out`
void TextInto(uint16_t* out, int size, const char* key) {
    WcsCpyS(out, size, Text_Get(kTheText, key));
}

// a control option's item initialised and added to a list (FEControls' OnEnter, inlined)
void SetupOption(uint8_t* screen, void* list, int itemOffset, uint32_t hash) {
    uint8_t* item = screen + itemOffset;
    At<float>(item, 0x18) = At<float>(screen, 0xD4);
    At<uint32_t>(item, 8) = hash;
    At<uint32_t>(item, 0x10) = At<uint32_t>(screen, 0x108);
    At<uint32_t>(item, 0x14) = At<uint32_t>(screen, 0x10C);
    At<uint8_t>(item, 0xC) = 0;
    ScrollList_AddItem(list, item);
}

} // namespace

namespace feac {

// ---- attract mode ----------------------------------------------------------------------------

// 0x6905F0 (not in the function list): FEAttractMode's constructor (its text style at +0x10)
void* __fastcall AttractMode_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEAttractMode);
    At<float>(self, 0x18) = 0.0f;
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 0.0f;
    At<int32_t>(self, 0x10) = 0;
    At<int32_t>(self, 0x14) = 3;
    At<int32_t>(self, 0x24) = 2;
    At<uint8_t>(self, 0x28) = 1;
    At<uint8_t>(self, 0x29) = 0;
    At<uint8_t>(self, 0x2A) = 0;
    return self;
}

// 0x690630 (vf1): Load
void __fastcall AttractMode_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    TextStyle_Load(self + 0x10, archive, "TextStyle");
}

// 0x690660 (vf3): the music stopped, the attract movie played
void __fastcall AttractMode_OnEnter(void*, re::Edx) {
    Font_Reset();
    Stream_Stop(kStreamPlayer, 6, 500);
    void* movie = MoviePlayer_Get();
    VCall<void>(movie, 8, kAttractMovie, 0);
}

// 0x690690 (vf4): the movie stopped, the menu music back
void __fastcall AttractMode_OnLeave(void*, re::Edx) {
    Font_Reset();
    void* movie = MoviePlayer_Get();
    VCall<void>(movie, 0xC);
    Stream_PlayByName(kStreamPlayer, "MS_Gobble.rsm", 0.4f, 1, 6);
}

// 0x6906D0 (vf5): any of buttons 4 .. 13 released on any of the 4 pads ends it
void __fastcall AttractMode_Process(void*, re::Edx) {
    for (int pad = 0; pad < 4; pad++) {
        for (int button = 4; button <= 0xD; button++) {
            if (Pad_ButtonJustUp(button, pad) != 0) {
                g_menuActive = 0;
                Pad_Reset738660();
                return;
            }
        }
    }
}

// 0x6909E0: the whole screen filled (640 x 480, the colour at 0x20C2FC0)
void __cdecl AttractMode_DrawBackground() {
    float uv2[2] = {1.0f, 1.0f};
    float uv1[2] = {0.0f, 0.0f};
    DrawRectUV(0.0f, 0.0f, 640.0f, 480.0f, nullptr, kBackdropColour, uv1, uv2);
}

// 0x690A30: the tip ("FE_ATTRACT_TIP") in the bottom right
void __fastcall AttractMode_DrawTip(void*, re::Edx) {
    float width = g_widescreen != 0 ? 853.0f : 640.0f;
    float negative = -width;
    float x = g_language == 7 ? float(double(negative) * 0.08281250298023224) : float(double(negative) * 0.05999999865889549);
    float position[2];
    float* at = ScreenPosition(position, x, -45.0f, 2, 2);
    float px = at[0];
    float py = at[1];
    g_fontColour[0] = g_attractTipColour[0];
    g_fontColour[1] = g_attractTipColour[1];
    g_fontColour[2] = g_attractTipColour[2];
    g_fontColour[3] = g_attractTipColour[3];
    Font_SetJustify(1);
    Font_SetSizePreset(g_language == 7 ? 1 : 2);
    g_fontDropColour[0] = g_attractTipDrop[0];
    g_fontDropColour[2] = g_attractTipDrop[2];
    g_fontDropColour[1] = g_attractTipDrop[1];
    g_fontDropColour[3] = g_attractTipDrop[3];
    g_fontDrop = 1;
    uint16_t text[0x40];
    Font_Format(text, Text_Get(kTheText, "FE_ATTRACT_TIP"), 0x40);
    Font_PrintString(px, py, text, 1);
    Font_Render();
}

// 0x690B80 (vf6): the movie while it plays (then its frame and the tip); over: the menu closes
void __fastcall AttractMode_Draw(void* self, re::Edx) {
    AttractMode_DrawBackground();
    if (VCall<int>(MoviePlayer_Get(), 0x10) == 1) {
        VCall<void>(MoviePlayer_Get(), 0x14);
        AttractMode_DrawTip(self, nullptr);
        return;
    }
    g_menuActive = 0;
}

// 0x690BC0 (not in the function list): FEBackground's constructor
void* __fastcall Background_Construct(void* self, re::Edx) {
    SetVtable(self, VT_FEBackground);
    return self;
}

// 0x690BD0 (not in the function list): FEBiology's constructor (its text style at +0xC)
void* __fastcall Biology_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEBiology);
    At<float>(self, 0x14) = 0.0f;
    At<float>(self, 0x18) = 0.0f;
    At<float>(self, 0x1C) = 0.0f;
    At<int32_t>(self, 0xC) = 0;
    At<int32_t>(self, 0x10) = 3;
    At<int32_t>(self, 0x20) = 2;
    At<uint8_t>(self, 0x24) = 1;
    At<uint8_t>(self, 0x25) = 0;
    At<uint8_t>(self, 0x26) = 0;
    return self;
}

// 0x690C10 (vf3): the biology class started from the menu (C_Biology_1)
void __fastcall Biology_OnEnter(void*, re::Edx) {
    g_specialEntitiesCreated = 0;
    g_menu20C32F6 = 1;
    g_menuActive = 0;
    VCall<void>(g_radar, 0x14, 1);
    uint32_t key;
    NameKey_Construct(&key, "C_Biology_1");
    int32_t mission = MissionMgr_Find(kMissionMgr, &key);
    MissionMgr_GetTaskFlags(kMissionMgr, mission)[4] |= 2;
    MissionMgr_6AD290(kMissionMgr, 1);
    MissionMgr_Start(kMissionMgr, mission, 0);
    Ped_474C10(g_player, 0, 0);
    NameKey_Destruct(&key);
}

// ---- the control options ------------------------------------------------------------------

// 0x690CA0 (vf0): {0, 0, width, its height}
void __fastcall Option_GetExtent(uint8_t* self, re::Edx, float* rect, float, float width) {
    rect[0] = 0.0f;
    rect[2] = width;
    rect[1] = 0.0f;
    rect[3] = VCall<float>(self, 8);
}

// 0x690CD0 / 0x6910B0 / 0x691310 (vf2): their heights
float __fastcall KeyMap_GetHeight(void*, re::Edx) {
    return 20.0f;
}
float __fastcall Toggle_GetHeight(void*, re::Edx) {
    return 25.0f;
}
float __fastcall Reset_GetHeight(void*, re::Edx) {
    return 15.0f;
}

// the key a map entry was set to, swapped with any other action that had it (DI: 25
// actions in the saved table; XInput: 16 in the live one)
static void AssignKey(int32_t* keys, int count, int action, int32_t key, int pad) {
    int32_t old = keys[action];
    keys[action] = key;
    for (int i = 0; i < count; i++) {
        if (keys[i] == key && i != action) {
            keys[i] = old;
            break;
        }
    }
    Pad_KeysSwapped(pad, old, key);
}

// 0x690CE0 (vf5): DI: a pressed key assigned (pad 0; the argument unused)
uint8_t __fastcall DIButtonMap_HandleInput(uint8_t* self, re::Edx, int) {
    int32_t key = Pad_PollKey(0);
    if (key == -1) {
        return 0;
    }
    int32_t* keys = const_cast<int32_t*>(ButtonMap_GetKeys(kButtonMap, 1));
    AssignKey(keys, 0x19, At<int32_t>(self, 0x1C), key, 0);
    return 1;
}

// 0x690D60 (vf4): DI: the key's name ("Button_Listening" while listening)
void __fastcall DIButtonMap_GetValueText(uint8_t* self, re::Edx, uint16_t* out, int size, int selected) {
    const int32_t* keys = ButtonMap_GetKeys(kButtonMap, 1);
    char key[0x40];  // (left as it was for an unknown binding kind)
    if (re::Flag(selected) && At<uint8_t>(self, 0xC) != 0) {
        SprintfS(key, 0x40, "Button_Listening");
    } else {
        int32_t code = keys[At<int32_t>(self, 0x1C)];
        if (code >= 0x10A) {
            int32_t unknown = 0;
            int32_t button = 0;
            static const char* const kKinds[] = {"DIBUTTON_JOYAXIS_NEG", "DIBUTTON_JOYAXIS_POS", "DIBUTTON_JOYPOV_UP",
                                                 "DIBUTTON_JOYPOV_DOWN", "DIBUTTON_JOYPOV_LEFT",
                                                 "DIBUTTON_JOYPOV_RIGHT", "DIBUTTON_JOYBUTTON"};
            uint32_t kind = uint32_t(Pad_BindingKind(0, code, &unknown, &button));
            if (kind <= 6) {
                SprintfS(key, 0x40, kKinds[kind]);
            }
            Font_AddNumber(float(button + 1));
        } else {
            SprintfS(key, 0x40, "DIBUTTON_%d", code);
        }
    }
    TextInto(out, size, key);
}

// 0x690EB0 (vf5): XInput: a pressed button assigned
uint8_t __fastcall XButtonMap_HandleInput(uint8_t* self, re::Edx, int pad) {
    int32_t key = Pad_PollKey(pad);
    if (key == -1) {
        return 0;
    }
    AssignKey(g_xinputKeys, 0x10, At<int32_t>(self, 0x1C), key, pad);
    return 1;
}

// 0x690F30 (vf4): XInput: "XBUTTON_%04x"
void __fastcall XButtonMap_GetValueText(uint8_t* self, re::Edx, uint16_t* out, int size, int selected) {
    char key[0x40];
    if (re::Flag(selected) && At<uint8_t>(self, 0xC) != 0) {
        SprintfS(key, 0x40, "Button_Listening");
    } else {
        SprintfS(key, 0x40, "XBUTTON_%04x", g_xinputKeys[At<int32_t>(self, 0x1C)]);
    }
    TextInto(out, size, key);
}

// A yes/no option toggled by left or right; accept stores it (`Store`), back just leaves
template <typename Store>
static uint8_t ToggleInput(uint8_t* self, Store store) {
    if (g_menuLeft != 0 && FE_PadPressed(0) != 0) {
        At<uint8_t>(self, 0x1C) = At<uint8_t>(self, 0x1C) == 0;
        FE_PadClear(0);
    } else if (g_menuRight != 0 && FE_PadPressed(0) != 0) {
        At<uint8_t>(self, 0x1C) = At<uint8_t>(self, 0x1C) == 0;
        FE_PadClear(0);
    }
    if (Input_AcceptPressed(kUserInput, 0) != 0) {
        store(At<uint8_t>(self, 0x1C));
        FE_PlaySelect();
        return 1;
    }
    if (Input_BackPressed(kUserInput, 0) != 0) {
        FE_PlaySelect();
        return 1;
    }
    return 0;
}

// 0x690FA0 (vf5): XInput on (for the pad this was entered from) or off; back: as it is
uint8_t __fastcall EnableXInput_HandleInput(uint8_t* self, re::Edx, int pad) {
    if ((g_menuLeft != 0 || g_menuRight != 0) && FE_PadPressed(0) != 0) {
        At<uint8_t>(self, 0x1C) = At<uint8_t>(self, 0x1C) == 0;
        FE_PadClear(0);
    }
    if (Input_AcceptPressed(kUserInput, 0) != 0) {
        if (At<uint8_t>(self, 0x1C) != 0) {
            g_xinputPad = pad;
            g_dinputPad = 0;
        } else {
            g_xinputPad = 0;
            g_dinputPad = pad;
        }
        FE_PlaySelect();
        return 1;
    }
    if (Input_BackPressed(kUserInput, 0) != 0) {
        At<uint8_t>(self, 0x1C) = Pad_IsXInput(GetActivePad());
        FE_PlaySelect();
        return 1;
    }
    return 0;
}

// yes/no texts (0x691060: yes when set; 0x691220: yes when clear)
template <bool YesWhenSet>
void __fastcall YesNo_GetValueText(uint8_t* self, re::Edx, uint16_t* out, int size, int) {
    char key[0x40];
    bool set = At<uint8_t>(self, 0x1C) != 0;
    SprintfS(key, 0x40, set == YesWhenSet ? "Enable_Xinput_Yes" : "Enable_Xinput_No");
    TextInto(out, size, key);
}

// 0x6910C0 (vf4): the sensitivity as "%1.1f"
void __fastcall MouseSensitivity_GetValueText(uint8_t* self, re::Edx, uint16_t* out, int, int) {
    char text[0x40];
    SprintfS(text, 0x40, "%1.1f", double(At<float>(self, 0x1C)));
    AsciiToWide(text, out);
}

// 0x691100 / 0x691190 (vf5): mouse steering on foot / in vehicles
uint8_t __fastcall CamControlSteering_HandleInput(uint8_t* self, re::Edx, int) {
    return ToggleInput(self, [](uint8_t value) { g_mouseSteerVehicle = value; });
}
uint8_t __fastcall LockMovementDir_HandleInput(uint8_t* self, re::Edx, int) {
    return ToggleInput(self, [](uint8_t value) { g_mouseSteerFoot = value; });
}

// 0x691270 (vf5): accept: the keys reset (+0x1C set: the XInput ones)
uint8_t __fastcall ResetKeyMap_HandleInput(uint8_t* self, re::Edx, int) {
    if (Input_AcceptPressed(kUserInput, 0) != 0) {
        if (At<uint8_t>(self, 0x1C) != 0) {
            ButtonMap_ResetX(kButtonMap);
        } else {
            ButtonMap_SetDefaultKeys(kButtonMap);
        }
        FE_PlaySelect();
        return 1;
    }
    if (Input_BackPressed(kUserInput, 0) != 0) {
        FE_PlaySelect();
        return 1;
    }
    return 0;
}

// 0x6912D0 (vf4): empty, or the confirmation while listening
void __fastcall ResetKeyMap_GetValueText(uint8_t* self, re::Edx, uint16_t* out, int size, int) {
    out[0] = 0;
    if (At<uint8_t>(self, 0xC) != 0) {
        WcsCpyS(out, size, Text_Get(kTheText, "Reset_Mapping_Confirm"));
    }
}

// 0x691320 (vf4): FEControls' OnLeave: the keys saved (a jmp with ecx set)
void __fastcall Controls_OnLeave(void*, re::Edx) {
    ButtonMap_Save(kButtonMap);
}

// 0x691330: the list of the tab shown (XInput's for tab 1)
void* __fastcall Controls_CurrentList(uint8_t* self, re::Edx) {
    return SelList_Selected(self + 0xD8) != 0 ? self + 0x54C : self + 0x150;
}

// the selected option of the tab shown
static uint8_t* SelectedOption(uint8_t* self) {
    void* list = SelList_Selected(self + 0xD8) != 0 ? self + 0x54C : self + 0x150;
    void* same = SelList_Selected(self + 0xD8) != 0 ? self + 0x54C : self + 0x150;
    int32_t selected = At<int32_t>(same, 0xC);
    return VCall<uint8_t*>(list, 4, selected);
}

// 0x691350 (vf5): listening: the option takes the input until it's handled; otherwise
// back closes, left/right switch tabs (the pad must be one), up/down move, accept listens
void __fastcall Controls_Process(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0xCC) != 0) {
        uint8_t* option = SelectedOption(self);
        if (option == nullptr) {
            return;
        }
        if (VCall<uint8_t>(option, 0x14, At<int32_t>(self, 0xD0)) != 0) {
            At<uint8_t>(self, 0xCC) = 0;
            At<uint8_t>(option, 0xC) = 0;
            FE_PadClear(0);
        }
        return;
    }
    if (Input_BackPressed(kUserInput, 0) != 0) {
        FE_PlaySelect();
        g_menuActive = 0;
    }
    void* tabs = self + 0xD8;
    SelList_Selected(tabs);
    bool moved = false;
    if (g_menuLeft != 0) {
        if (FE_PadPressed(0) != 0 && At<int32_t>(self, 0xD0) != -1) {
            int8_t before = SelList_Selected(tabs);
            moved = before != SelList_Previous(tabs);
        }
    } else if (g_menuRight != 0) {
        if (FE_PadPressed(0) != 0 && At<int32_t>(self, 0xD0) != -1) {
            int8_t before = SelList_Selected(tabs);
            moved = before != SelList_Next(tabs);
        }
    } else if (g_menuUp != 0) {
        if (FE_PadPressed(0) != 0) {
            At<int32_t>(self, 0xC8) = ScrollList_Previous(Controls_CurrentList(self, nullptr));
            moved = true;
        }
    } else if (g_menuDown != 0) {
        if (FE_PadPressed(0) != 0) {
            At<int32_t>(self, 0xC8) = ScrollList_Next(Controls_CurrentList(self, nullptr));
            moved = true;
        }
    }
    if (moved) {
        FE_PadClear(0);
    }
    if (Input_AcceptPressed(kUserInput, 0) == 0) {
        return;
    }
    if (At<uint8_t>(self, 0x8BC) != 0) {
        At<uint8_t>(self, 0x8BC) = 0;
        return;
    }
    At<uint8_t>(self, 0xCC) = 1;
    uint8_t* option = SelectedOption(self);
    if (option != nullptr) {
        At<uint8_t>(option, 0xC) = 1;
    }
    FE_PlaySelect();
}

// 0x691560 (vf6): the window, the tab's list, the tabs, the title
void __fastcall Controls_Draw(uint8_t* self, re::Edx) {
    FE_DrawWindow(320.0f, 440.0f, 110.0f, kWindowColour, 2.0f);
    void* list = SelList_Selected(self + 0xD8) != 0 ? self + 0x54C : self + 0x150;
    ScrollList_Draw(list, 0.0f, 0.0f, 0);
    SelList_Draw(self + 0xD8);
    FE_DrawTitle("FEO_CONTROLS");
}

// 0x691680 (vf1): the label, its value (vf3) and the highlight between them
void __fastcall Option_Draw(uint8_t* self, re::Edx, float x, float y, int, int, int selected) {
    uint32_t hash = At<uint32_t>(self, 8);
    if (hash == 0) {
        return;
    }
    int colour = re::Flag(selected) ? At<int32_t>(self, 0x10) : At<int32_t>(self, 0x14);
    const uint16_t* text = Text_GetByHash(kTheText, hash);
    FE_DrawText(int(x), int(y), text, colour, 0, 0, selected);
    float left = g_lastTextRect[0];
    float right = g_lastTextRect[2];
    float valueX = x + At<float>(self, 0x18);
    float width = VCall<float>(self, 0xC, valueX, y, selected);
    float end = (x + At<float>(self, 0x18)) - width;
    float labelWidth = std::fabs(right - left);
    FE_DrawHighlight(x + labelWidth, end, y);
}

// 0x691780 (vf3): the value's text right-aligned at x; its width
float __fastcall Option_DrawValueText(uint8_t* self, re::Edx, float x, float y, int selected) {
    uint16_t text[0x200];
    VCall<void>(self, 0x10, text, 0x200, selected);
    int colour = re::Flag(selected) ? At<int32_t>(self, 0x10) : At<int32_t>(self, 0x14);
    if (At<uint8_t>(self, 0xC) != 0 && re::Flag(selected)) {
        colour = At<int32_t>(self, 0x14);
    }
    FE_DrawText(int(x), int(y), text, colour, 1, 0, selected);
    float width = g_lastTextRect[2] - g_lastTextRect[0];
    return std::fabs(width);
}

// the button's icon right-aligned at x (as high as the option); its width + 5
static float DrawIcon(uint8_t* self, void* texture, float x, float y) {
    // (x87 at 24 bits: each step rounded to float though kept in a double)
    float scale = g_screenScaleX;
    uint32_t texWidth = VCall<uint32_t>(texture, 0x48);
    float scaled = float(texWidth) * VCall<float>(self, 8);
    uint32_t texHeight = VCall<uint32_t>(texture, 0x4C);
    float width = scaled / float(texHeight) * scale;
    float uv2[2] = {1.0f, 1.0f};
    float uv1[2] = {0.0f, 0.0f};
    float height = VCall<float>(self, 8);
    DrawRectUV(x - width, y, x, y + height, texture, kIconColour, uv1, uv2);
    return width + 5.0f;
}

// 0x691840 (vf3): the key maps' values: a pad button's icon; a keyboard key's icon then its
// name to the left (neither while listening on the selected one)
float __fastcall KeyMap_DrawValue(uint8_t* self, re::Edx, float x, float y, int selected) {
    uint8_t isKey = 0;
    // (the byte flag passed as a dword: the game's upper bits are whatever eax held)
    int xinput = VCall<uint8_t>(self, 0x18);
    void* texture = GetButtonTexture(At<int32_t>(self, 0x1C), &isKey, xinput, 1);
    bool listening = re::Flag(selected) && At<uint8_t>(self, 0xC) != 0;
    if (!listening && texture != nullptr && isKey == 0) {
        return DrawIcon(self, texture, x, y);
    }
    float iconWidth = 0.0f;
    if (!listening && texture != nullptr && isKey != 0) {
        iconWidth = DrawIcon(self, texture, x, y);
    }
    return ControlOption_DrawValueText(self, x - iconWidth, y, selected) + iconWidth;
}

// 0x691AB0 (vf5): the sensitivity by 0.1 (0.4 .. 5); accept stores it (as the custom one
// when that's set), back takes the stored one again
uint8_t __fastcall MouseSensitivity_HandleInput(uint8_t* self, re::Edx, int) {
    float& value = At<float>(self, 0x1C);
    bool changed = false;
    float next = 0.0f;
    if (g_menuLeft != 0 && FE_PadPressed(0) != 0) {
        next = float(double(value) - 0.10000000149011612);
        if (!(double(next) > 0.4000000059604645)) {
            next = 0.4f;
        }
        changed = true;
    } else if (g_menuRight != 0 && FE_PadPressed(0) != 0) {
        next = float(double(value) + 0.10000000149011612);
        if (!(next < 5.0f)) {
            next = 5.0f;
        }
        changed = true;
    }
    if (changed) {
        value = next;
        FE_PadClear(0);
    }
    if (Input_AcceptPressed(kUserInput, 0) != 0) {
        if (0.0f < g_mouseSensitivity) {
            g_mouseSensitivity = value;
        } else {
            g_mouseSensitivityDefault = value;
        }
        FE_PlaySelect();
        return 1;
    }
    if (Input_BackPressed(kUserInput, 0) != 0) {
        value = g_mouseSensitivity > 0.0f ? g_mouseSensitivity : g_mouseSensitivityDefault;
        FE_PlaySelect();
        return 1;
    }
    return 0;
}

// 0x691BE0 (vf1): Load: the lists, the tabs' labels, the value column, the actions' texts
void __fastcall Controls_Load(uint8_t* self, re::Edx, void* archive) {
    Screen_Load(self, archive);
    ScrollList_Load(self + 0x150, archive);
    ScrollList_Load(self + 0x54C, archive);
    SelList_Load(self + 0xD8, archive);
    SelList_Clear(self + 0xD8);
    SelList_AddByName(self + 0xD8, "Label_DirectInput");
    SelList_AddByName(self + 0xD8, "Label_XInput");
    VCall<void>(archive, 0x28, self + 0xD4, "Data XOffset");
    struct {
        re::Ptr32<char> data;
        int32_t size;
        char buffer[0x10];
    } value;
    value.data = value.buffer;
    value.size = 0x10;
    value.buffer[0] = 0;
    for (int i = 0; i < 0x19; i++) {
        char key[0x74];
        char name[0x80];
        Sprintf(key, "%s", ButtonName(uint32_t(i)));
        VCall<void>(archive, 0x24, &value, 0x10, key);
        SprintfS(name, 0x80, "%s_D", value.data);
        At<uint32_t>(self, 0x24 + i * 4) = HashName(name);
        if (i < 0x10) {
            SprintfS(name, 0x80, "%s_X", value.data);
            At<uint32_t>(self, 0x88 + i * 4) = HashName(name);
        }
    }
}

// 0x691D20 (vf3): OnEnter: both lists filled (DI: reset, mouse, steering, the 25 keys;
// XInput: reset, enable, the 16 buttons), the XInput tab first for an XInput pad, the
// first XInput pad the one listened to
void __fastcall Controls_OnEnter(uint8_t* self, re::Edx) {
    SelList_Reset(self + 0xD8);
    void* di = self + 0x150;
    void* xi = self + 0x54C;
    ScrollList_Clear(di);
    ScrollList_Clear(xi);
    At<uint8_t>(self, 0x8B8) = 0;
    SetupOption(self, di, 0x89C, HashName("Reset_Dinput"));
    uint32_t hash = HashName("FEO_MOUSE");
    At<float>(self, 0x838) = g_mouseSensitivity > 0.0f ? g_mouseSensitivity : g_mouseSensitivityDefault;
    SetupOption(self, di, 0x81C, hash);
    hash = HashName("FEO_MOUSESTEERFOOT");
    At<uint8_t>(self, 0x878) = g_mouseSteerFoot;
    SetupOption(self, di, 0x85C, hash);
    hash = HashName("FEO_MOUSESTEERVEH");
    At<uint8_t>(self, 0x858) = g_mouseSteerVehicle;
    SetupOption(self, di, 0x83C, hash);
    for (int i = 0; i < 0x19; i++) {
        At<int32_t>(self, 0x22C + i * 0x20 + 0x1C) = i;
        SetupOption(self, di, 0x22C + i * 0x20, At<uint32_t>(self, 0x24 + i * 4));
    }
    hash = HashName("Reset_Xinput");
    At<uint8_t>(self, 0x898) = 1;
    SetupOption(self, xi, 0x87C, hash);
    hash = HashName("Enable_Xinput");
    At<uint8_t>(self, 0x818) = Pad_IsXInput(GetActivePad());
    SetupOption(self, xi, 0x7FC, hash);
    for (int i = 0; i < 0x10; i++) {
        At<int32_t>(self, 0x5FC + i * 0x20 + 0x1C) = i;
        SetupOption(self, xi, 0x5FC + i * 0x20, At<uint32_t>(self, 0x88 + i * 4));
    }
    if (Pad_IsXInput(GetActivePad()) != 0) {
        SelList_Select(self + 0xD8, 1);
    }
    At<int32_t>(self, 0xD0) = -1;
    for (int pad = 0; pad < 4; pad++) {
        if (Pad_IsXInput(pad) != 0) {
            At<int32_t>(self, 0xD0) = pad;
            At<uint8_t>(self, 0x8BC) = 1;
            return;
        }
    }
    At<uint8_t>(self, 0x8BC) = 1;
}

// 0x692730 (not in the function list): FEControls' constructor
void* __fastcall Controls_Construct(uint8_t* self, re::Edx) {
    SetVtable(self, VT_FEControls);
    At<float>(self, 0x10) = 0.0f;
    At<float>(self, 0x14) = 0.0f;
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 0.0f;
    At<int32_t>(self, 0xC) = 0;
    At<int32_t>(self, 0x18) = 0;
    SelList_Construct(self + 0xD8, self + 0xD8 + 0x70, 2);
    SetVtable(self + 0xD8, VT_SelListT1);
    ScrollList_Construct(self + 0x150, 0x1D);
    SetVtable(self + 0x150, VT_ScrollListT1D);
    for (int i = 0; i < 0x19; i++) {
        At<uint8_t>(self, 0x22C + i * 0x20 + 4) = 1;
        SetVtable(self + 0x22C + i * 0x20, VT_DIButtonMap);
    }
    ScrollList_Construct(self + 0x54C, 0x12);
    SetVtable(self + 0x54C, VT_ScrollListT12);
    for (int i = 0; i < 0x10; i++) {
        At<uint8_t>(self, 0x5FC + i * 0x20 + 4) = 1;
        SetVtable(self + 0x5FC + i * 0x20, VT_XButtonMap);
    }
    At<uint8_t>(self, 0x800) = 1;
    SetVtable(self + 0x7FC, VT_CEnableXInput);
    At<uint8_t>(self, 0x820) = 1;
    SetVtable(self + 0x81C, VT_CMouseSensitivity);
    At<uint8_t>(self, 0x840) = 1;
    SetVtable(self + 0x83C, VT_CCamControlSteering);
    At<uint8_t>(self, 0x860) = 1;
    SetVtable(self + 0x85C, VT_CLockMovementDir);
    At<uint8_t>(self, 0x880) = 1;
    SetVtable(self + 0x87C, VT_CResetKeyMap);
    SetVtable(self + 0x89C, VT_CResetKeyMap);
    At<uint8_t>(self, 0x8A0) = 1;
    return self;
}

} // namespace feac

#define FEAC_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(feac_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(feac_original_, __LINE__), \
             re::Mode::Original)

FEAC_REVERSED("FEAttractMode::FEAttractMode", 0x006905F0, &feac::AttractMode_Construct);
FEAC_REVERSED("FEAttractMode::Load", 0x00690630, &feac::AttractMode_Load);
FEAC_REVERSED("FEAttractMode::OnEnter", 0x00690660, &feac::AttractMode_OnEnter);
FEAC_REVERSED("FEAttractMode::OnLeave", 0x00690690, &feac::AttractMode_OnLeave);
FEAC_REVERSED("FEAttractMode::Process", 0x006906D0, &feac::AttractMode_Process);
FEAC_REVERSED("FEAttractMode::DrawBackground", 0x006909E0, &feac::AttractMode_DrawBackground);
FEAC_REVERSED("FEAttractMode::DrawTip", 0x00690A30, &feac::AttractMode_DrawTip);
FEAC_REVERSED("FEAttractMode::Draw", 0x00690B80, &feac::AttractMode_Draw);
FEAC_REVERSED("FEBackground::FEBackground", 0x00690BC0, &feac::Background_Construct);
FEAC_REVERSED("FEBiology::FEBiology", 0x00690BD0, &feac::Biology_Construct);
FEAC_REVERSED("FEBiology::OnEnter", 0x00690C10, &feac::Biology_OnEnter);
FEAC_REVERSED("FEControls::CControlOption::GetExtent", 0x00690CA0, &feac::Option_GetExtent);
FEAC_REVERSED("FEControls::CButtonMap::GetHeight", 0x00690CD0, &feac::KeyMap_GetHeight);
FEAC_REVERSED("FEControls::DIButtonMap::HandleInput", 0x00690CE0, &feac::DIButtonMap_HandleInput);
FEAC_REVERSED("FEControls::DIButtonMap::GetValueText", 0x00690D60, &feac::DIButtonMap_GetValueText);
FEAC_REVERSED("FEControls::XButtonMap::HandleInput", 0x00690EB0, &feac::XButtonMap_HandleInput);
FEAC_REVERSED("FEControls::XButtonMap::GetValueText", 0x00690F30, &feac::XButtonMap_GetValueText);
FEAC_REVERSED("FEControls::CEnableXInput::HandleInput", 0x00690FA0, &feac::EnableXInput_HandleInput);
FEAC_REVERSED("FEControls::CEnableXInput::GetValueText", 0x00691060, &feac::YesNo_GetValueText<true>);
FEAC_REVERSED("FEControls::CEnableXInput::GetHeight", 0x006910B0, &feac::Toggle_GetHeight);
FEAC_REVERSED("FEControls::CMouseSensitivityOption::GetValueText", 0x006910C0, &feac::MouseSensitivity_GetValueText);
FEAC_REVERSED("FEControls::CCamControlSteeringOption::HandleInput", 0x00691100, &feac::CamControlSteering_HandleInput);
FEAC_REVERSED("FEControls::CLockMovementDirOption::HandleInput", 0x00691190, &feac::LockMovementDir_HandleInput);
FEAC_REVERSED("FEControls::CLockMovementDirOption::GetValueText", 0x00691220, &feac::YesNo_GetValueText<false>);
FEAC_REVERSED("FEControls::CResetKeyMapOption::HandleInput", 0x00691270, &feac::ResetKeyMap_HandleInput);
FEAC_REVERSED("FEControls::CResetKeyMapOption::GetValueText", 0x006912D0, &feac::ResetKeyMap_GetValueText);
FEAC_REVERSED("FEControls::CResetKeyMapOption::GetHeight", 0x00691310, &feac::Reset_GetHeight);
FEAC_REVERSED("FEControls::OnLeave", 0x00691320, &feac::Controls_OnLeave);
FEAC_REVERSED("FEControls::CurrentList", 0x00691330, &feac::Controls_CurrentList);
FEAC_REVERSED("FEControls::Process", 0x00691350, &feac::Controls_Process);
FEAC_REVERSED("FEControls::Draw", 0x00691560, &feac::Controls_Draw);
FEAC_REVERSED("FEControls::CControlOption::Draw", 0x00691680, &feac::Option_Draw);
FEAC_REVERSED("FEControls::CControlOption::DrawValueText", 0x00691780, &feac::Option_DrawValueText);
FEAC_REVERSED("FEControls::CButtonMap::DrawValue", 0x00691840, &feac::KeyMap_DrawValue);
FEAC_REVERSED("FEControls::CMouseSensitivityOption::HandleInput", 0x00691AB0, &feac::MouseSensitivity_HandleInput);
FEAC_REVERSED("FEControls::Load", 0x00691BE0, &feac::Controls_Load);
FEAC_REVERSED("FEControls::OnEnter", 0x00691D20, &feac::Controls_OnEnter);
FEAC_REVERSED("FEControls::FEControls", 0x00692730, &feac::Controls_Construct);
