#include "game/Frontend.h"
#include "reversed.h"

namespace {
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);

// the audio manager (one object): volumes by category (1 music, 2 speech, 4 effects)
void* const g_audio = reinterpret_cast<void*>(0x00CF3DA0);
GAME_FN(int, __thiscall, Audio_GetVolume, 0x00594E60, void* audio, int category);
GAME_FN(void, __thiscall, Audio_SetVolume, 0x00594ED0, void* audio, int category, int volume);
GAME_FN(void, __thiscall, Audio_VolumeUp, 0x00594FA0, void* audio, int category);
GAME_FN(void, __thiscall, Audio_VolumeDown, 0x00594FD0, void* audio, int category);
GAME_FN(int8_t, __thiscall, Audio_GetOutput, 0x00595880, void* audio);
GAME_FN(void, __thiscall, Audio_SetOutput, 0x00595840, void* audio, int output);
// the stream player (the music option's sample)
void* const g_streamPlayer = reinterpret_cast<void*>(0x00CF4324);
GAME_FN(void, __thiscall, Stream_Stop, 0x005913D0, void* player, int channel, int fadeMs);
// the menu's input and sounds
GAME_FN(uint8_t, __cdecl, FE_PadPressed, 0x006A3170, int pad);
GAME_FN(void, __cdecl, FE_PlaySound, 0x006A30D0, int sound, int unknown);
// the pads' rumble
GAME_FN(int, __cdecl, GetPlayerPadIndex, 0x00738070);
GAME_FN(void, __cdecl, Pad_738700, 0x00738700, int unknown);
GAME_FN(void, __cdecl, Pad_StopRumble, 0x00738780);
GAME_FN(void, __thiscall, Pad_Rumble, 0x0050B710, void* pad, int a, int b, int c);
GAME_ARRAY(uint8_t, g_pads, 0x020CECF0);  // 0x914 bytes each
// the settings (one block at 0xC18700) and what reads them
void* const g_settings = reinterpret_cast<void*>(0x00C18700);
GAME_FN(void, __thiscall, Settings_SetDefaults, 0x004546B0, void* settings);
GAME_VAR(uint8_t, g_vibration, 0x00C18700);
GAME_VAR(int32_t, g_brightness, 0x00C18708);
GAME_VAR(uint8_t, g_subtitles, 0x00C1870C);
GAME_VAR(uint8_t, g_audioOutput, 0x00C18713);
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(float, g_screenOffsetX, 0x00C1872C);
GAME_VAR(float, g_screenOffsetY, 0x00C18730);
GAME_VAR(uint8_t, g_invertCamera, 0x00C18736);
GAME_ARRAY(float, g_screenCorners, 0x020C3000);  // [16]: 8 x/y pairs, all the same point
// the text (the language reloaded)
void* const g_text = reinterpret_cast<void*>(0x020C2FA4);
GAME_FN(void, __thiscall, Text_Reload, 0x0068FDF0, void* text);
// the options screen, and its option lists refreshed
GAME_VAR(void*, g_optionsScreen, 0x020C329C);
GAME_FN(void, __thiscall, FEOptions_InitOptions, 0x006965D0, void* screen);
GAME_FN(void, __thiscall, FEOptions_696620, 0x00696620, void* screen);
// the input edges the menus read
GAME_VAR(uint8_t, g_menuLeft, 0x020C3280);
GAME_VAR(uint8_t, g_menuRight, 0x020C327C);
GAME_VAR(uint32_t, g_otherTimer, 0x00A14A50);
// the pause menu's state
GAME_VAR(uint8_t*, g_uberCompletion, 0x020CE75C);  // (+0x264: what the menu does next)
GAME_VAR(uint8_t, g_specialEntitiesCreated, 0x00CA3984);
GAME_VAR(uint8_t, g_menu20C32F6, 0x020C32F6);
GAME_VAR(uint8_t, g_menu20C32EF, 0x020C32EF);
GAME_VAR(uint8_t, g_quitRequested, 0x020C32ED);
GAME_VAR(void*, g_radar, 0x00C674CC);
GAME_FN(void, __thiscall, HUDRadar_SetMapMode, 0x0054DEF0, void* radar, int mode);
GAME_FN(void, __cdecl, Game_42FA20, 0x0042FA20);
// the photo album
void* const g_album = reinterpret_cast<void*>(0x00AB4688);
GAME_FN(void, __thiscall, Album_CreateTexture, 0x00543F60, void* album);
GAME_FN(void, __thiscall, Album_Clear, 0x00546260, void* album);
GAME_FN(void, __thiscall, Album_ReleaseTexture, 0x00543FA0, void* album);
// the missions/tasks
void* const g_missionMgr = reinterpret_cast<void*>(0x020C3CA0);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetTaskFlags, 0x006AA7C0, void* mgr, int task);
GAME_FN(void, __thiscall, FEDisplaySettings_692BC0, 0x00692BC0, void* screen);
GAME_FN(int, __cdecl, CallBrightnessLevel, 0x00696E00);

constexpr uintptr_t VT_IArchivable = 0x00900CDC;
constexpr uintptr_t VT_FESelectionListWindow = 0x009404B8;
constexpr uintptr_t VT_FEScrollingSelectionListWindow = 0x0094021C;

re::Ptr32<void>* Vtable(uintptr_t address) {
    return reinterpret_cast<re::Ptr32<void>*>(address);
}
template <typename R, typename P, typename... A>
R VCall(P self, int slot, A... args) {
    void* object = self;   // (a pointer, or a Ptr32 field on x64)
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(object, args...);
}
template <uintptr_t Address, typename T>
void CallThis(T* self) {
    reinterpret_cast<void(__thiscall*)(T*)>(Address)(self);
}
re::Ptr32<void>* VtableOf(void* object) {
    return *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
}
void SetVtable(void* object, uintptr_t vtable) {
    *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object) = Vtable(vtable);
}
enum { OPTION_APPLY = 3, OPTION_STEP = 7 };
} // namespace

namespace fe {

// ---- COption kinds -----------------------------------------------------------------------

// 0x697610
void __fastcall Vibration_Init(COptionValue* self, re::Edx) {
    self->value = int8_t(g_vibration);
    self->unknown10 = g_otherTimer - 1;
}

// 0x696890: on: a short rumble on the player's pad. Changed: 1
uint8_t __fastcall Vibration_Change(COptionValue* self, re::Edx, int value) {
    int8_t was = self->value;
    self->value = int8_t(value);
    g_vibration = uint8_t(value);
    VCall<void>(self, OPTION_APPLY);
    if (was == 0 && self->value != 0) {
        int pad = GetPlayerPadIndex();
        Pad_738700(0);
        Pad_Rumble(g_pads + uint32_t(pad * 0x914), 0xFA, 0x96, 1);
    }
    return was != self->value;
}

// 0x696900: off: the rumble stopped
void __fastcall Vibration_Apply(COptionValue* self, re::Edx) {
    if (self->value == 0) {
        Pad_StopRumble();
    }
}

// 0x696910
void __fastcall Vibration_Save(COptionValue* self, re::Edx) {
    self->saved = int8_t(g_vibration);
}

// 0x696920
void __fastcall Vibration_Restore(COptionValue* self, re::Edx) {
    self->value = self->saved;
    g_vibration = uint8_t(self->saved);
    VCall<void>(self, OPTION_APPLY);
}

// 0x696940 (vf2): the music sample stopped
void __fastcall MusicVolume_StopPreview(CVolumeOption* self, re::Edx) {
    if (self->previewing) {
        Stream_Stop(g_streamPlayer, 6, 500);
        self->previewing = 0;
    }
}

// 0x696A60
void __fastcall MusicVolume_Save(CVolumeOption* self, re::Edx) {
    self->savedVolume = Audio_GetVolume(g_audio, 1);
}

// 0x696A80
void __fastcall MusicVolume_Restore(CVolumeOption* self, re::Edx) {
    if (self->previewing) {
        Stream_Stop(g_streamPlayer, 6, 500);
        self->previewing = 0;
    }
    Audio_SetVolume(g_audio, 1, self->savedVolume);
}

// A volume a step up or down; changed: 1 (0x696AB0 effects, 0x696BB0 speech)
static uint8_t StepVolume(int category, int up) {
    int was = Audio_GetVolume(g_audio, category);
    if (re::Flag(up)) {
        Audio_VolumeUp(g_audio, category);
    } else {
        Audio_VolumeDown(g_audio, category);
    }
    return was != Audio_GetVolume(g_audio, category);
}
uint8_t __fastcall SFXVolume_Step(CVolumeOption*, re::Edx, int up) {
    return StepVolume(4, up);
}
uint8_t __fastcall SpeechVolume_Step(CVolumeOption*, re::Edx, int up) {
    return StepVolume(2, up);
}

// Left/right: a step, with the menu's sound when it changed (0x696B00, 0x696C00)
static void VolumeInput(CVolumeOption* self, int downSound, int upSound) {
    if (g_menuLeft && FE_PadPressed(0)) {
        if (VCall<uint8_t>(self, OPTION_STEP, 0)) {
            FE_PlaySound(downSound, 0);
        }
        return;
    }
    if (g_menuRight && FE_PadPressed(0) && VCall<uint8_t>(self, OPTION_STEP, 1)) {
        FE_PlaySound(upSound, 0);
    }
}
void __fastcall SFXVolume_Input(CVolumeOption* self, re::Edx) {
    VolumeInput(self, 5, 6);
}
void __fastcall SpeechVolume_Input(CVolumeOption* self, re::Edx) {
    VolumeInput(self, 7, 8);
}

// 0x696B70 / 0x696B90, 0x696C70 / 0x696C90
void __fastcall SFXVolume_Save(CVolumeOption* self, re::Edx) {
    self->savedVolume = Audio_GetVolume(g_audio, 4);
}
void __fastcall SFXVolume_Restore(CVolumeOption* self, re::Edx) {
    Audio_SetVolume(g_audio, 4, self->savedVolume);
}
void __fastcall SpeechVolume_Save(CVolumeOption* self, re::Edx) {
    self->savedVolume = Audio_GetVolume(g_audio, 2);
}
void __fastcall SpeechVolume_Restore(CVolumeOption* self, re::Edx) {
    Audio_SetVolume(g_audio, 2, self->savedVolume);
}

// 0x696CB0: (0, 1, 2 kept as they are; anything else too)
void __fastcall AudioOutput_Init(COptionValue* self, re::Edx) {
    int8_t output = Audio_GetOutput(g_audio);
    self->value = output;
    switch (output) {
    case 0: self->value = 0; break;
    case 1: self->value = 1; break;
    case 2: self->value = 2; break;
    }
}

// 0x696D70
void __fastcall AudioOutput_Save(COptionValue* self, re::Edx) {
    int8_t output = Audio_GetOutput(g_audio);
    self->saved = output;
    switch (output) {
    case 0: self->saved = 0; break;
    case 1: self->saved = 1; break;
    case 2: self->saved = 2; break;
    }
}

// 0x696DB0: (an unknown value becomes 1)
void __fastcall AudioOutput_Restore(COptionValue* self, re::Edx) {
    self->value = self->saved;
    int output = 1;
    if (self->saved == 0) {
        output = 0;
    } else if (self->saved == 2) {
        output = 2;
    }
    Audio_SetOutput(g_audio, output);
    g_audioOutput = uint8_t(output);
    VCall<void>(self, OPTION_APPLY);
}

// 0x696E00: the brightness setting (150 .. 280) as a level 0 .. 10, rounded
int __cdecl BrightnessLevel() {
    float f = float((double(g_brightness) - 150.0) / 130.0);
    // (x87 compares: below 0 is 0, above 1 is 1, a NaN stays)
    if (f < 0.0f) {
        f = 0.0f;
    } else if (1.0f < f) {
        f = 1.0f;
    }
    float tenths = float(double(f) * 10.0);
    return int(double(tenths) + 0.5);
}

// 0x696E60
void __fastcall Brightness_Init(CBrightnessOption* self, re::Edx) {
    self->level = uint16_t(CallBrightnessLevel());
    self->changed = 0;
}

// 0x696F60
void __fastcall Brightness_Save(CBrightnessOption* self, re::Edx) {
    self->savedLevel = uint16_t(CallBrightnessLevel());
}

// 0x696F70
void __fastcall Brightness_Restore(CBrightnessOption* self, re::Edx) {
    self->level = self->savedLevel;
    g_brightness = int(double(self->savedLevel) / 10.0 * 130.0 + 150.5);
}

// 0x696FA0
void __fastcall AdjustScreenPos_Init(CAdjustScreenPosOption* self, re::Edx) {
    self->adjusting = 0;
}

// 0x697160: the offset tried, on all the corners' points
void __fastcall AdjustScreenPos_Apply(CAdjustScreenPosOption* self, re::Edx) {
    float x = self->x + g_screenOffsetX;
    float y = self->y + g_screenOffsetY;
    g_screenCorners[0] = x;
    for (int i = 1; i < 16; i += 2) {
        g_screenCorners[i] = y;
    }
    for (int i = 2; i < 16; i += 2) {
        g_screenCorners[i] = x;
    }
}

// 0x6971F0
void __fastcall AdjustScreenPos_Save(CAdjustScreenPosOption* self, re::Edx) {
    self->savedX = g_screenOffsetX;
    self->savedY = g_screenOffsetY;
}

// 0x697210
void __fastcall AdjustScreenPos_Restore(CAdjustScreenPosOption* self, re::Edx) {
    g_screenOffsetX = self->savedX;
    g_screenOffsetY = self->savedY;
    VCall<void>(self, OPTION_APPLY);
}

// On/off settings kept in a byte (subtitles 0x697230.., invert camera 0x697400..)
template <uintptr_t Setting>
void __fastcall Toggle_Init(COptionValue* self, re::Edx) {
    self->value = *reinterpret_cast<int8_t*>(Setting);
}
template <uintptr_t Setting>
uint8_t __fastcall Toggle_Change(COptionValue* self, re::Edx, int value) {
    int8_t was = self->value;
    self->value = int8_t(value);
    *reinterpret_cast<uint8_t*>(Setting) = uint8_t(value);
    return was != self->value;
}
template <uintptr_t Setting>
void __fastcall Toggle_Save(COptionValue* self, re::Edx) {
    self->saved = *reinterpret_cast<int8_t*>(Setting);
}
template <uintptr_t Setting>
void __fastcall Toggle_Restore(COptionValue* self, re::Edx) {
    *reinterpret_cast<int8_t*>(Setting) = self->saved;
    self->value = self->saved;
}

// 0x697280
void __fastcall Language_Init(COptionValue* self, re::Edx) {
    self->value = int8_t(g_language);
}

// 0x697290: the next or previous of the 8 (not past the ends: 0 then)
uint8_t __fastcall Language_Change(COptionValue* self, re::Edx, int up) {
    uint8_t changed = 1;
    if (re::Flag(up)) {
        self->value = int8_t(self->value + 1);
        if (self->value >= 8) {
            self->value = int8_t(self->value - 1);
            VCall<void>(self, OPTION_APPLY);
            return 0;
        }
    } else {
        self->value = int8_t(self->value - 1);
        if (self->value < 0) {
            self->value = int8_t(self->value + 1);
            changed = 0;
        }
    }
    VCall<void>(self, OPTION_APPLY);
    return changed;
}

// 0x6972E0: the text in it, the options' texts again
void __fastcall Language_Apply(COptionValue* self, re::Edx) {
    g_language = self->value;
    Text_Reload(g_text);
    FEOptions_InitOptions(g_optionsScreen);
}

// 0x697300
void __fastcall Language_Save(COptionValue* self, re::Edx) {
    self->saved = int8_t(g_language);
}

// 0x697310
void __fastcall Language_Restore(COptionValue* self, re::Edx) {
    self->value = self->saved;
    VCall<void>(self, OPTION_APPLY);
}

// 0x697320 (vf2): the settings' defaults, output 1 and the volumes 8
void __fastcall ResetToDefaults_Do(CResetToDefaultsOption* self, re::Edx) {
    if (self->unknown0C == 0) {
        Settings_SetDefaults(g_settings);
        Audio_SetOutput(g_audio, 1);
        Audio_SetVolume(g_audio, 4, 8);
        Audio_SetVolume(g_audio, 1, 8);
        Audio_SetVolume(g_audio, 2, 8);
        FEOptions_696620(g_optionsScreen);
    }
}

// ---- the screens -------------------------------------------------------------------------

// 0x692840 / 0x692880: FEDisplaySettings' row, down / up through 4
void __fastcall DisplaySettings_Next(uint8_t* self, re::Edx) {
    int32_t& row = *reinterpret_cast<int32_t*>(self + 0xC);
    switch (row) {
    case 0: row = 1; break;
    case 1: row = 2; break;
    case 2: row = 3; break;
    case 3: row = 0; break;
    }
}
void __fastcall DisplaySettings_Previous(uint8_t* self, re::Edx) {
    int32_t& row = *reinterpret_cast<int32_t*>(self + 0xC);
    switch (row) {
    case 0: row = 3; break;
    case 1: row = 0; break;
    case 2: row = 1; break;
    case 3: row = 2; break;
    }
}

// 0x692CB0 (vf3)
void __fastcall DisplaySettings_OnEnter(uint8_t* self, re::Edx) {
    *reinterpret_cast<int32_t*>(self + 0x10) = 0;
    *reinterpret_cast<int32_t*>(self + 0xC) = 0;
    FEDisplaySettings_692BC0(self);
}

// 0x693710 (FERestart's and FEExitGameToMenu's vf3)
void __fastcall Confirm_OnEnter(uint8_t* self, re::Edx) {
    *reinterpret_cast<int32_t*>(self + 0xC) = 1;
}

// 0x6939E0: FEGeography (and the like): the radar shown
void __fastcall Geography_OnEnter(void*, re::Edx) {
    g_specialEntitiesCreated = 0;
    g_menu20C32F6 = 1;
    g_menu20C32EF = 0;
    void* radar = g_radar;
    re::PtrCast<void(__thiscall*)(void*, int)>(VtableOf(radar)[5])(radar, 1);
}

// 0x6955C0
void __fastcall Load_OnEnter(uint8_t* self, re::Edx) {
    *reinterpret_cast<int32_t*>(self + 0x24) = 0;
    *reinterpret_cast<int32_t*>(self + 0x1C) = 0;
    *reinterpret_cast<int32_t*>(self + 0x20) = 1;
    self[0x28] = 0;
    self[0x29] = 0;
}

// 0x6955E0 (FELoad's and FESave's vf4)
void __fastcall SaveLoad_OnLeave(void*, re::Edx) {
    *reinterpret_cast<int32_t*>(g_uberCompletion + 0x264) = 6;
}

// 0x696000 / 0x696010: FEMap
void __fastcall Map_OnEnter(void*, re::Edx) {
    HUDRadar_SetMapMode(g_radar, 1);
}
void __fastcall Map_OnLeave(void*, re::Edx) {
    HUDRadar_SetMapMode(g_radar, 0);
}

// 0x693BE0: FEInventory's first shown row follows the selection
void __fastcall Inventory_ScrollToSelection(uint8_t* self, re::Edx) {
    uint8_t first = self[0x3C];
    int32_t selected = *reinterpret_cast<int32_t*>(self + 0x34);
    int32_t shown = *reinterpret_cast<int32_t*>(self + 0x2C);
    if (selected < int32_t(first)) {
        self[0x3C] = uint8_t(first - 1);
    } else if (shown + int32_t(first) - 1 < selected) {
        self[0x3C] = uint8_t(first + 1);
    }
}

// 0x69C850 (vf4)
void __fastcall Stats_OnLeave(uint8_t* self, re::Edx) {
    re::Ptr32<void>& items = *reinterpret_cast<re::Ptr32<void>*>(self + 0x410);
    if (items) {
        Free(items);
        items = nullptr;
    }
}

// 0x6965D0: every option of the three lists (at +0xD8, 10 each; counts at +0x150) Init
void __fastcall Options_InitOptions(uint8_t* self, re::Edx) {
    for (int list = 0; list < 3; list++) {
        int count = self[0x150 + list];
        re::Ptr32<COption>* options = reinterpret_cast<re::Ptr32<COption>*>(self + 0xD8 + list * 0x28);
        for (int i = 0; i < count; i++) {
            VCall<void>(options[i], 0);
        }
    }
}

namespace {
GAME_FN(void, __thiscall, SelectionList_ResetCall, 0x0069B8F0, FESelectionListWindow* list);
} // namespace

// 0x697480 (vf3)
void __fastcall Options_OnEnter(uint8_t* self, re::Edx) {
    *reinterpret_cast<int32_t*>(self + 0x154) = 0;
    self[0x158] = 0;
    CallThis<0x006965D0>(self);
    SelectionList_ResetCall(reinterpret_cast<FESelectionListWindow*>(self + 0x16C));
}

// 0x698840 (vf3)
void __fastcall PhotoAlbum_OnEnter(uint8_t* self, re::Edx) {
    *reinterpret_cast<int32_t*>(self + 0x16C) = 1;
    Album_CreateTexture(g_album);
    self[0x169] = 0;
    *reinterpret_cast<int32_t*>(self + 0x170) = 0;
    *reinterpret_cast<int32_t*>(self + 0x174) = 0;
    *reinterpret_cast<int32_t*>(self + 0x178) = 0;
    *reinterpret_cast<int32_t*>(self + 0x164) = 0;
    self[0x168] = 1;
}

// 0x698880 (vf4)
void __fastcall PhotoAlbum_OnLeave(void*, re::Edx) {
    Album_Clear(g_album);
    Album_ReleaseTexture(g_album);
    *reinterpret_cast<int32_t*>(g_uberCompletion + 0x264) = 6;
}

// 0x6998B0 (vf3)
void __fastcall QuitGame_OnEnter(void*, re::Edx) {
    Game_42FA20();
    g_quitRequested = 1;
}

// 0x69E940: FETasks::CAvailableTask (vf2): its task's flag 0x40 toggled
void __fastcall AvailableTask_Toggle(FEListItem* self, re::Edx) {
    uint8_t* flags = MissionMgr_GetTaskFlags(g_missionMgr, self->id);
    flags[4] ^= 0x40;
}

// 0x69D2B0: FEStatus::CStatusItem (vf0): its extent, 420 high
void __fastcall StatusItem_GetExtent(FEListItem*, re::Edx, float* rect, float, float width) {
    rect[0] = 0.0f;
    rect[2] = width;
    rect[1] = 0.0f;
    rect[3] = 420.0f;  // (0x940610)
}

// ---- selection lists ----------------------------------------------------------------------

// 0x69B8F0
void __fastcall SelectionList_Reset(FESelectionListWindow* self, re::Edx) {
    self->selected = 0;
    self->firstVisible = 0;
    self->unknown6D = 0;
}

// 0x69BAE0
void __fastcall SelectionList_Clear(FESelectionListWindow* self, re::Edx) {
    self->count = 0;
    self->selected = 0;
    self->firstVisible = 0;
    self->unknown6D = 0;
}

// 0x69B900: returns its index
int __fastcall SelectionList_Add(FESelectionListWindow* self, re::Edx, uint32_t entry) {
    self->entries[self->count] = entry;
    self->count++;
    return int(self->count) - 1;
}

// 0x69BF30: an entry by its text key's name
int __fastcall SelectionList_AddByName(FESelectionListWindow* self, re::Edx, const char* name) {
    uint32_t hash = HashName(name);
    self->entries[self->count] = hash;
    self->count++;
    return int(self->count) - 1;
}

// 0x69B920: down, round to the top; the shown ones follow
int8_t __fastcall SelectionList_Next(FESelectionListWindow* self, re::Edx) {
    self->selected = int8_t(self->selected + 1);
    if (int(self->selected) >= int(self->count)) {
        self->selected = 0;
        self->firstVisible = 0;
        return 0;
    }
    if (int(self->visibleCount) + int(self->firstVisible) - 1 < int(self->selected)) {
        self->firstVisible++;
    }
    return self->selected;
}

// 0x69B960: up, round to the bottom
int8_t __fastcall SelectionList_Previous(FESelectionListWindow* self, re::Edx) {
    self->selected = int8_t(self->selected - 1);
    if (self->selected < 0) {
        int8_t last = int8_t(self->count - 1);
        self->selected = last;
        self->firstVisible = uint8_t(self->count - uint8_t(self->visibleCount));
        return last;
    }
    if (int(self->selected) < int(self->firstVisible)) {
        self->firstVisible--;
    }
    return self->selected;
}

// 0x69B9A0
void __fastcall SelectionList_Select(FESelectionListWindow* self, re::Edx, int index) {
    self->selected = int8_t(index);
}

// 0x69B8E0: FESelectionListWindow's destructor
void __fastcall SelectionList_Destruct(FESelectionListWindow* self, re::Edx) {
    SetVtable(self, VT_FESelectionListWindow);
}

// 0x69BAC0
void* __fastcall SelectionList_Delete(FESelectionListWindow* self, re::Edx, int flags) {
    SetVtable(self, VT_FESelectionListWindow);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x69AB40: FEScrollingSelectionListWindow's destructor
void __fastcall ScrollingList_Destruct(FEScrollingSelectionListWindow* self, re::Edx) {
    SetVtable(self, VT_FEScrollingSelectionListWindow);
}

// 0x69ABB0
void* __fastcall ScrollingList_Delete(FEScrollingSelectionListWindow* self, re::Edx, int flags) {
    SetVtable(self, VT_FEScrollingSelectionListWindow);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x69AB50: back to the top, the first selectable item selected
void __fastcall ScrollingList_Reset(FEScrollingSelectionListWindow* self, re::Edx) {
    self->unknown4C[3] = 0.0f;
    self->unknown4C[0] = 0.0f;
    self->unknown4C[1] = 0.0f;
    self->unknown4C[2] = 0.0f;
    self->unknown48 = 0;
    self->selected = -1;
    for (int i = 0; i < self->count; i++) {
        FEListItem* item = VCall<FEListItem*>(self, 1, i);
        if (item->selectable) {
            self->selected = i;
            break;
        }
    }
    self->unknown60 = 0.0f;
    self->unknown64 = 0;
}

namespace {
GAME_FN(void, __thiscall, ScrollingList_ResetCall, 0x0069AB50, FEScrollingSelectionListWindow* list);
} // namespace

// 0x69ABD0: emptied
void __fastcall ScrollingList_Clear(FEScrollingSelectionListWindow* self, re::Edx) {
    self->totalHeight = -1.0f;
    self->count = 0;
    self->unknown08 = 0;
    ScrollingList_ResetCall(self);
}

// 0x69B1C0: its constructor (the argument unused)
FEScrollingSelectionListWindow* __fastcall ScrollingList_Construct(FEScrollingSelectionListWindow* self, re::Edx, int) {
    SetVtable(self, VT_FEScrollingSelectionListWindow);
    self->unknown24 = 0.0f;
    self->unknown28 = 0.0f;
    self->unknown20 = 0;
    self->unknown30 = 0.0f;
    self->unknown34 = 0.0f;
    self->unknown2C = 0;
    self->count = 0;
    self->totalHeight = -1.0f;
    self->unknown08 = 0;
    ScrollingList_ResetCall(self);
    return self;
}

// 0x693A10: IArchivable's deleting destructor (the screens without their own)
void* __fastcall IArchivable_Delete(void* self, re::Edx, int flags) {
    SetVtable(self, VT_IArchivable);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// The lists' template kinds (FESelectionListWindowT<n>, FEScrollingSelectionListWindowT<n>):
// their vtable, the base's destructor, freed when asked
template <uintptr_t VT, uintptr_t BaseDestruct>
void* __fastcall ListT_Delete(void* self, re::Edx, int flags) {
    SetVtable(self, VT);
    CallThis<BaseDestruct>(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// A screen's deleting destructor: its lists' destructors (inlined: their vtable, then
// the base's), IArchivable's vtable, freed when asked
struct ScreenList {
    uint32_t offset;
    uintptr_t vtable;
    uintptr_t baseDestruct;
};
static void DestructScreen(void* self, uintptr_t vtable, const ScreenList* lists, int count) {
    SetVtable(self, vtable);
    for (int i = 0; i < count; i++) {
        void* list = static_cast<uint8_t*>(self) + lists[i].offset;
        SetVtable(list, lists[i].vtable);
        reinterpret_cast<void(__thiscall*)(void*)>(lists[i].baseDestruct)(list);
    }
    SetVtable(self, VT_IArchivable);
}
constexpr uintptr_t SEL = 0x0069B8E0;   // FESelectionListWindow's destructor
constexpr uintptr_t SCROLL = 0x0069AB40;  // FEScrollingSelectionListWindow's

#define FE_SCREEN_DELETE(Name, VT, ...)                                              \
    void* __fastcall Name##_Delete(void* self, void*, int flags) {                   \
        static const ScreenList kLists[] = {__VA_ARGS__};                            \
        DestructScreen(self, VT, kLists, int(sizeof(kLists) / sizeof(kLists[0])));   \
        if (flags & 1) {                                                             \
            Free(self);                                                              \
        }                                                                            \
        return self;                                                                 \
    }

// 0x6926D0, 0x6949D0, 0x6981E0, 0x69CB80, 0x69E140, 0x69F310, 0x6A02F0
FE_SCREEN_DELETE(FEControls, 0x0093F3B4, {0x54C, 0x0093F318, SCROLL}, {0x150, 0x0093F308, SCROLL}, {0xD8, 0x0093F300, SEL})
FE_SCREEN_DELETE(FEInventory, 0x0093F9B4, {0x40, 0x0093F9AC, SEL})
FE_SCREEN_DELETE(FEOptions, 0x0093FD04, {0x16C, 0x0093FCFC, SEL})
FE_SCREEN_DELETE(FEStats, 0x0094052C, {0x114, 0x0094051C, SCROLL}, {0x44, 0x00940514, SEL})
FE_SCREEN_DELETE(FEStatus, 0x00940628, {0xC, 0x00940618, SCROLL})
FE_SCREEN_DELETE(FETasks, 0x0094085C, {0x7C, 0x0094084C, SCROLL})
FE_SCREEN_DELETE(FEUpgrades, 0x0094095C, {0x130, 0x0094094C, SCROLL}, {0xB8, 0x0093F300, SEL})

} // namespace fe


// ---- registration ---------------------------------------------------------------------------

#define FE_REVERSED(name, address, fn)                                                               \
    static void* RE_CONCAT(fe_original_, __LINE__);                                                 \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(fe_original_, __LINE__), re::Mode::Original)

FE_REVERSED("CVibrationOption::Init", 0x00697610, (&fe::Vibration_Init));
FE_REVERSED("CVibrationOption::Change", 0x00696890, (&fe::Vibration_Change));
FE_REVERSED("CVibrationOption::Apply", 0x00696900, (&fe::Vibration_Apply));
FE_REVERSED("CVibrationOption::Save", 0x00696910, (&fe::Vibration_Save));
FE_REVERSED("CVibrationOption::Restore", 0x00696920, (&fe::Vibration_Restore));
FE_REVERSED("CMusicVolumeOption::StopPreview", 0x00696940, (&fe::MusicVolume_StopPreview));
FE_REVERSED("CMusicVolumeOption::Save", 0x00696A60, (&fe::MusicVolume_Save));
FE_REVERSED("CMusicVolumeOption::Restore", 0x00696A80, (&fe::MusicVolume_Restore));
FE_REVERSED("CSFXVolumeOption::Step", 0x00696AB0, (&fe::SFXVolume_Step));
FE_REVERSED("CSFXVolumeOption::HandleInput", 0x00696B00, (&fe::SFXVolume_Input));
FE_REVERSED("CSFXVolumeOption::Save", 0x00696B70, (&fe::SFXVolume_Save));
FE_REVERSED("CSFXVolumeOption::Restore", 0x00696B90, (&fe::SFXVolume_Restore));
FE_REVERSED("CSpeechVolumeOption::Step", 0x00696BB0, (&fe::SpeechVolume_Step));
FE_REVERSED("CSpeechVolumeOption::HandleInput", 0x00696C00, (&fe::SpeechVolume_Input));
FE_REVERSED("CSpeechVolumeOption::Save", 0x00696C70, (&fe::SpeechVolume_Save));
FE_REVERSED("CSpeechVolumeOption::Restore", 0x00696C90, (&fe::SpeechVolume_Restore));
FE_REVERSED("CAudioOutputOption::Init", 0x00696CB0, (&fe::AudioOutput_Init));
FE_REVERSED("CAudioOutputOption::Save", 0x00696D70, (&fe::AudioOutput_Save));
FE_REVERSED("CAudioOutputOption::Restore", 0x00696DB0, (&fe::AudioOutput_Restore));
FE_REVERSED("CBrightnessOption::GetLevel", 0x00696E00, (&fe::BrightnessLevel));
FE_REVERSED("CBrightnessOption::Init", 0x00696E60, (&fe::Brightness_Init));
FE_REVERSED("CBrightnessOption::Save", 0x00696F60, (&fe::Brightness_Save));
FE_REVERSED("CBrightnessOption::Restore", 0x00696F70, (&fe::Brightness_Restore));
FE_REVERSED("CAdjustScreenPosOption::Init", 0x00696FA0, (&fe::AdjustScreenPos_Init));
FE_REVERSED("CAdjustScreenPosOption::Apply", 0x00697160, (&fe::AdjustScreenPos_Apply));
FE_REVERSED("CAdjustScreenPosOption::Save", 0x006971F0, (&fe::AdjustScreenPos_Save));
FE_REVERSED("CAdjustScreenPosOption::Restore", 0x00697210, (&fe::AdjustScreenPos_Restore));
FE_REVERSED("CSubtitlesOption::Init", 0x00697230, (&fe::Toggle_Init<0x00C1870C>));
FE_REVERSED("CSubtitlesOption::Change", 0x00697240, (&fe::Toggle_Change<0x00C1870C>));
FE_REVERSED("CSubtitlesOption::Save", 0x00697260, (&fe::Toggle_Save<0x00C1870C>));
FE_REVERSED("CSubtitlesOption::Restore", 0x00697270, (&fe::Toggle_Restore<0x00C1870C>));
FE_REVERSED("CInvertCameraControlOption::Init", 0x00697400, (&fe::Toggle_Init<0x00C18736>));
FE_REVERSED("CInvertCameraControlOption::Change", 0x00697410, (&fe::Toggle_Change<0x00C18736>));
FE_REVERSED("CInvertCameraControlOption::Save", 0x00697430, (&fe::Toggle_Save<0x00C18736>));
FE_REVERSED("CInvertCameraControlOption::Restore", 0x00697440, (&fe::Toggle_Restore<0x00C18736>));
FE_REVERSED("CLanguageOption::Init", 0x00697280, (&fe::Language_Init));
FE_REVERSED("CLanguageOption::Change", 0x00697290, (&fe::Language_Change));
FE_REVERSED("CLanguageOption::Apply", 0x006972E0, (&fe::Language_Apply));
FE_REVERSED("CLanguageOption::Save", 0x00697300, (&fe::Language_Save));
FE_REVERSED("CLanguageOption::Restore", 0x00697310, (&fe::Language_Restore));
FE_REVERSED("CResetToDefaultsOption::Do", 0x00697320, (&fe::ResetToDefaults_Do));
FE_REVERSED("FEDisplaySettings::NextRow", 0x00692840, (&fe::DisplaySettings_Next));
FE_REVERSED("FEDisplaySettings::PreviousRow", 0x00692880, (&fe::DisplaySettings_Previous));
FE_REVERSED("FEDisplaySettings::OnEnter", 0x00692CB0, (&fe::DisplaySettings_OnEnter));
FE_REVERSED("FERestart::OnEnter", 0x00693710, (&fe::Confirm_OnEnter));
FE_REVERSED("FEGeography::OnEnter", 0x006939E0, (&fe::Geography_OnEnter));
FE_REVERSED("FELoad::OnEnter", 0x006955C0, (&fe::Load_OnEnter));
FE_REVERSED("FrontEndScreen::SaveLoad_OnLeave", 0x006955E0, (&fe::SaveLoad_OnLeave));
FE_REVERSED("FEMap::OnEnter", 0x00696000, (&fe::Map_OnEnter));
FE_REVERSED("FEMap::OnLeave", 0x00696010, (&fe::Map_OnLeave));
FE_REVERSED("FEInventory::ScrollToSelection", 0x00693BE0, (&fe::Inventory_ScrollToSelection));
FE_REVERSED("FEStats::OnLeave", 0x0069C850, (&fe::Stats_OnLeave));
FE_REVERSED("FEOptions::InitOptions", 0x006965D0, (&fe::Options_InitOptions));
FE_REVERSED("FEOptions::OnEnter", 0x00697480, (&fe::Options_OnEnter));
FE_REVERSED("FEPhotoAlbum::OnEnter", 0x00698840, (&fe::PhotoAlbum_OnEnter));
FE_REVERSED("FEPhotoAlbum::OnLeave", 0x00698880, (&fe::PhotoAlbum_OnLeave));
FE_REVERSED("FEQuitGame::OnEnter", 0x006998B0, (&fe::QuitGame_OnEnter));
FE_REVERSED("FETasks::CAvailableTask::Toggle", 0x0069E940, (&fe::AvailableTask_Toggle));
FE_REVERSED("FEStatus::CStatusItem::GetExtent", 0x0069D2B0, (&fe::StatusItem_GetExtent));
FE_REVERSED("FESelectionListWindow::Reset", 0x0069B8F0, (&fe::SelectionList_Reset));
FE_REVERSED("FESelectionListWindow::Clear", 0x0069BAE0, (&fe::SelectionList_Clear));
FE_REVERSED("FESelectionListWindow::Add", 0x0069B900, (&fe::SelectionList_Add));
FE_REVERSED("FESelectionListWindow::AddByName", 0x0069BF30, (&fe::SelectionList_AddByName));
FE_REVERSED("FESelectionListWindow::Next", 0x0069B920, (&fe::SelectionList_Next));
FE_REVERSED("FESelectionListWindow::Previous", 0x0069B960, (&fe::SelectionList_Previous));
FE_REVERSED("FESelectionListWindow::Select", 0x0069B9A0, (&fe::SelectionList_Select));
FE_REVERSED("FESelectionListWindow::~FESelectionListWindow", 0x0069B8E0, (&fe::SelectionList_Destruct));
FE_REVERSED("FESelectionListWindow::DeletingDestructor", 0x0069BAC0, (&fe::SelectionList_Delete));
FE_REVERSED("FEScrollingSelectionListWindow::~FEScrollingSelectionListWindow", 0x0069AB40, (&fe::ScrollingList_Destruct));
FE_REVERSED("FEScrollingSelectionListWindow::DeletingDestructor", 0x0069ABB0, (&fe::ScrollingList_Delete));
FE_REVERSED("FEScrollingSelectionListWindow::Reset", 0x0069AB50, (&fe::ScrollingList_Reset));
FE_REVERSED("FEScrollingSelectionListWindow::Clear", 0x0069ABD0, (&fe::ScrollingList_Clear));
FE_REVERSED("FEScrollingSelectionListWindow::FEScrollingSelectionListWindow", 0x0069B1C0, (&fe::ScrollingList_Construct));
FE_REVERSED("IArchivable::DeletingDestructor", 0x00693A10, (&fe::IArchivable_Delete));
FE_REVERSED("FEControls::DeletingDestructor", 0x006926D0, (&fe::FEControls_Delete));
FE_REVERSED("FEInventory::DeletingDestructor", 0x006949D0, (&fe::FEInventory_Delete));
FE_REVERSED("FEOptions::DeletingDestructor", 0x006981E0, (&fe::FEOptions_Delete));
FE_REVERSED("FEStats::DeletingDestructor", 0x0069CB80, (&fe::FEStats_Delete));
FE_REVERSED("FEStatus::DeletingDestructor", 0x0069E140, (&fe::FEStatus_Delete));
FE_REVERSED("FETasks::DeletingDestructor", 0x0069F310, (&fe::FETasks_Delete));
FE_REVERSED("FEUpgrades::DeletingDestructor", 0x006A02F0, (&fe::FEUpgrades_Delete));
FE_REVERSED("FEScrollingSelectionListWindowT<9>::DeletingDestructor", 0x006A2A00, (&fe::ListT_Delete<0x00941004, fe::SCROLL>));
FE_REVERSED("FEScrollingSelectionListWindowT<0x12>::DeletingDestructor", 0x00691650, (&fe::ListT_Delete<0x0093F318, fe::SCROLL>));
FE_REVERSED("FEScrollingSelectionListWindowT<0x1D>::DeletingDestructor", 0x00691620, (&fe::ListT_Delete<0x0093F308, fe::SCROLL>));
FE_REVERSED("FEScrollingSelectionListWindowT<0x32>::DeletingDestructor", 0x0069EC40, (&fe::ListT_Delete<0x0094084C, fe::SCROLL>));
FE_REVERSED("FEScrollingSelectionListWindowT<0x40>::DeletingDestructor", 0x0069FFF0, (&fe::ListT_Delete<0x0094094C, fe::SCROLL>));
FE_REVERSED("FEScrollingSelectionListWindowT<0x71>::DeletingDestructor", 0x0069D470, (&fe::ListT_Delete<0x00940618, fe::SCROLL>));
FE_REVERSED("FEScrollingSelectionListWindowT<0xA5>::DeletingDestructor", 0x0069C950, (&fe::ListT_Delete<0x0094051C, fe::SCROLL>));
FE_REVERSED("FESelectionListWindowT<1>::DeletingDestructor", 0x006915F0, (&fe::ListT_Delete<0x0093F300, fe::SEL>));
FE_REVERSED("FESelectionListWindowT<2>::DeletingDestructor", 0x00697450, (&fe::ListT_Delete<0x0093FCFC, fe::SEL>));
FE_REVERSED("FESelectionListWindowT<3>::DeletingDestructor", 0x00693EE0, (&fe::ListT_Delete<0x0093F9AC, fe::SEL>));
FE_REVERSED("FESelectionListWindowT<0xC>::DeletingDestructor", 0x0069C920, (&fe::ListT_Delete<0x00940514, fe::SEL>));
