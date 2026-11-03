// The HUD's task line (HUD_TASKS: the current main / secondary mission or objective,
// cycled with the d-pad), the tattoo parlour hooks, script text (HUD_TEXT's slots),
// the tutorial box, and the weapon selector (its wheel, the list of what can be
// picked, the fade). 0x55A400..0x55C5C3 (the constructors, Setup and Reset are
// Hud.cpp's).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct CVector2D {
    float x, y;
};

GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(uint8_t, g_widescreenText, 0x00C1870E);
GAME_VAR(void*, g_radar, 0x00C674CC);
GAME_VAR(void*, g_tattooParlour, 0x00CF0A40);
GAME_VAR(float, g_fontScale, 0x00BA4E10);
GAME_VAR(uint32_t, g_fontColour, 0x0208B764);
GAME_VAR(uint32_t, g_fontDropColour, 0x0208B780);
GAME_VAR(int16_t, g_fontShadow, 0x0208B784);
GAME_VAR(int16_t, g_fontEdge, 0x0208B788);
GAME_VAR(uint8_t, g_fontFlags, 0x0208B78A);
GAME_ARRAY(float, g_fontStyleHeights, 0x0208B240);
GAME_ARRAY(uint8_t, g_fontSlots, 0x0208B818);  // [8] 0x38 bytes: the print slots (+0 colour)
// the task line's font block (0x208B838: +0xC x, +0x10 y, +0x14 style, +0x18 colour)
GAME_VAR(float, g_taskX, 0x0208B844);
GAME_VAR(float, g_taskY, 0x0208B848);
GAME_VAR(int32_t, g_taskStyle, 0x0208B84C);
GAME_VAR(uint32_t, g_taskColour, 0x0208B850);
GAME_VAR(int32_t, g_taskEdge, 0x0208B228);
// the subtitle block (0x208B800: +0xC x, +0x10 y)
GAME_VAR(float, g_subtitleX, 0x0208B80C);
GAME_VAR(float, g_subtitleY, 0x0208B810);
// the tutorial block (0x208B870: +0xC x, +0x10 y; the minimum right edges)
GAME_VAR(int32_t, g_tutorialEdge, 0x0208B21C);
GAME_VAR(float, g_tutorialX, 0x0208B87C);
GAME_VAR(float, g_tutorialY, 0x0208B880);
GAME_VAR(float, g_tutorialMinRight, 0x0208B898);
GAME_VAR(float, g_tutorialMinRightWide, 0x0208B89C);
GAME_VAR(float, g_tutorialScaleX, 0x00AB81BC);
GAME_VAR(float, g_tutorialScaleY, 0x00AB81B8);
GAME_VAR(float, g_tutorialSeconds, 0x00AB8198);
GAME_VAR(float, g_taskSeconds, 0x00AB8038);
GAME_VAR(uint8_t, g_cutsceneRunning, 0x00C1A99A);
GAME_VAR(HUDComponent*, g_hudC674E4, 0x00C674E4);
GAME_VAR(uint8_t, g_showingCompletion, 0x020C870A);
GAME_VAR(uint8_t, g_objectivesWrap, 0x020C476E);
GAME_VAR(uint8_t*, g_gameMode, 0x020C8704);
GAME_VAR(uint32_t, g_outfitHash, 0x01B6548C);
GAME_ARRAY(const char*, g_tutorialTips, 0x00AB8078);
GAME_VAR(void*, g_weaponWheelBack, 0x00C66FD4);
GAME_VAR(void*, g_weaponWheelEmpty, 0x00C66F68);
GAME_VAR(float, g_hudAlpha, 0x00A64880);
GAME_VAR(uint32_t, g_wheelColourFar, 0x00CF0A5C);
GAME_VAR(uint32_t, g_wheelColourNear, 0x00CF0A58);
// weapon models
GAME_VAR(int32_t, g_weaponNone, 0x00BF4330);
GAME_VAR(int32_t, g_weaponUnequip, 0x00BF432C);
GAME_VAR(int32_t, g_weaponOutfit1, 0x00BF444C);
GAME_VAR(int32_t, g_weaponOutfit2, 0x00BF44D8);
GAME_VAR(int32_t, g_weaponOutfit3, 0x00BF4444);
GAME_VAR(int32_t, g_weaponOutfit4, 0x00BF4438);
GAME_VAR(int32_t, g_weaponMode1, 0x00BF4430);
GAME_VAR(int32_t, g_weaponMode2, 0x00BF433C);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
inline void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const kObjectives = reinterpret_cast<void*>(0x020C4420);
inline void* const kClothing = reinterpret_cast<void*>(0x020C5F70);
inline void* const kTaskFont = reinterpret_cast<void*>(0x0208B838);
inline void* const kSubtitleFont = reinterpret_cast<void*>(0x0208B800);
inline void* const kTutorialFont = reinterpret_cast<void*>(0x0208B870);

GAME_FN(CRGBA*, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const col::CRect* rect, const CRGBA* colour);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
GAME_FN(void, __thiscall, Sprite_Draw, 0x0052C6F0, void* sprite, const col::CRect* rect, const CRGBA* colour);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __thiscall, Font_UseBlock, 0x00689400, void* block);
GAME_FN(void, __cdecl, Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Font_SetScaleXY, 0x006880B0, float x, float y);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Font_AddString, 0x006880F0, const uint16_t* text);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(void, __cdecl, Font_GetTextRect, 0x0068B090, float* rect, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_PrintInSlot, 0x0068D610, int slot, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int control, int pad);
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetInfo, 0x006AA660, void* mgr, int id);   // +0x20 name key, +0x25, +0x28 kind
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetStats, 0x006AA7C0, void* mgr, int id);  // +4 flags
GAME_FN(uint8_t, __thiscall, MissionMgr_6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionMgr_InClass, 0x006AA840, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionMgr_6AA890, 0x006AA890, void* mgr);
GAME_FN(int, __thiscall, MissionMgr_Count, 0x006AA140, void* mgr);
GAME_FN(int, __thiscall, MissionMgr_GetIcon, 0x006AB0A0, void* mgr, int id);
GAME_FN(void, __thiscall, MissionMgr_6A9F80, 0x006A9F80, void* mgr, int);
GAME_FN(uint8_t, __thiscall, Objectives_Exists, 0x006AE090, void* objectives, int id);
GAME_FN(uint8_t*, __thiscall, Objectives_Get, 0x006AE080, void* objectives, int id);  // +0x1D done
GAME_FN(void, __thiscall, Objectives_Rewind, 0x006AE0E0, void* objectives);
GAME_FN(void, __thiscall, Objectives_SeekTo, 0x006AE0F0, void* objectives, int id);
GAME_FN(int, __thiscall, Objectives_Next, 0x006AE100, void* objectives, int32_t* id);
GAME_FN(void, __thiscall, Objective_GetText, 0x006ADF90, void* objective, uint16_t* out, int size);
GAME_FN(int, __thiscall, Radar_FindBlipById, 0x00548A80, void* radar, int id);
GAME_FN(void, __thiscall, Radar_StopBlipFlashing, 0x00548DE0, void* radar, int handle);
GAME_FN(void, __thiscall, Radar_SelectBlip, 0x0054C200, void* radar, int handle);
GAME_FN(void, __thiscall, Radar_FlashDoorBlips, 0x0054C290, void* radar);
GAME_FN(void*, __thiscall, Radar_GetSlot3A98, 0x00548E50, void* radar, int index);
GAME_FN(uint8_t, __cdecl, IsPaused_5142D0, 0x005142D0);
GAME_FN(uint8_t, __cdecl, IsInCutscene_705500, 0x00705500);
GAME_FN(uint8_t, __thiscall, Controls_50DF50, 0x0050DF50, void* controls);
GAME_FN(void, __cdecl, Fn_738780, 0x00738780);
GAME_FN(void, __cdecl, Fn_6CB550, 0x006CB550);
GAME_FN(void, __cdecl, Fn_6CB620, 0x006CB620);
GAME_FN(void, __thiscall, Tattoo_6CFF80, 0x006CFF80, void* parlour);
GAME_FN(void, __thiscall, Tattoo_6D00A0, 0x006D00A0, void* parlour);
GAME_FN(void, __thiscall, Tattoo_Draw, 0x006D06A0, void* parlour);
GAME_FN(void, __thiscall, Tattoo_Destruct, 0x005B1EB0, void* parlour);
GAME_FN(void, __thiscall, Fn_455190, 0x00455190, void* self);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, Tutorial_59C150, 0x0059C150, void* self, int);
GAME_FN(uint8_t, __thiscall, Clothing_IsWearingOutfit, 0x006CA9F0, void* clothing, uint32_t hash);
GAME_FN(int, __thiscall, Inventory_Has, 0x0045CF40, void* inventory, int weapon);
GAME_FN(uint8_t*, __cdecl, GetWeaponInfo, 0x0051B450, int weapon);
GAME_FN(void, __thiscall, WeaponInfo_7282A0, 0x007282A0, void* info);
GAME_FN(void*, __cdecl, GetWeaponIcon, 0x00541210, int weapon);
// these, where modes apply
GAME_FN(uint8_t, __thiscall, Tasks_IsMainAvailable_, 0x0055A400, void*, int id);
GAME_FN(uint8_t, __thiscall, Tasks_IsSecondaryAvailable_, 0x0055A460, void*, int id);
GAME_FN(void, __thiscall, Tasks_DrawTaskText_, 0x0055A5E0, void*, const uint16_t* text);
GAME_FN(void, __thiscall, Tasks_DrawObjective_, 0x0055A690, void* self);
GAME_FN(void, __thiscall, Tasks_SetMain_, 0x0055A6E0, void* self, int id);
GAME_FN(void, __thiscall, Tasks_SetSecondary_, 0x0055A760, void* self, int id);
GAME_FN(void, __thiscall, Tasks_SetObjective_, 0x0055A7E0, void* self, int id);
GAME_FN(void, __thiscall, Tasks_NextSecondary_, 0x0055A840, void* self);
GAME_FN(void, __thiscall, Tasks_NextObjective_, 0x0055A8B0, void* self);
GAME_FN(void, __thiscall, Tasks_NextMain_, 0x0055A970, void* self);
GAME_FN(void, __thiscall, Tasks_DrawLine_, 0x0055AA40, void* self, int icon, const uint16_t* text);
GAME_FN(void, __thiscall, Tasks_DrawMain_, 0x0055ACD0, void* self);
GAME_FN(void, __thiscall, Tasks_DrawSecondary_, 0x0055AD10, void* self);
GAME_FN(void, __thiscall, Tasks_Clear, 0x0055A4F0, void* self);
GAME_FN(uint8_t, __thiscall, Weapons_CanSelect_, 0x0055BA70, void* self, void* ped, int weapon);
GAME_FN(void, __thiscall, Weapons_SetFade_, 0x0055BC90, void* self, int fadeOut);

// the task's blip off the radar, the line hidden (inlined in several of these)
void ClearTaskBlip(HUDTasks* self) {
    void* radar = g_radar;
    int id;
    if (self->blipType == 1) {
        id = self->unknown28[0];
    } else if (self->blipType == 2) {
        id = self->unknown28[2];
    } else {
        return;
    }
    Radar_StopBlipFlashing(radar, Radar_FindBlipById(radar, id));
}

void UseTaskColour() {
    std::memcpy(&g_fontColour, &g_taskColour, 4);
}

} // namespace

namespace hudtasks {

// 0x55A400: a main mission that can be shown (kind 0, not hidden, unlocked, not flagged 0x40)
uint8_t __fastcall IsMainAvailable(void*, re::Edx, int id) {
    if (id < 0) {
        return 0;
    }
    if (*reinterpret_cast<int32_t*>(MissionMgr_GetInfo(kMissionMgr, id) + 0x28) != 0) {
        return 0;
    }
    if (MissionMgr_GetInfo(kMissionMgr, id)[0x25] != 0) {
        return 0;
    }
    if (MissionMgr_GetStats(kMissionMgr, id)[4] & 0x40) {
        return 0;
    }
    return (MissionMgr_GetStats(kMissionMgr, id)[4] & 1) ? 1 : 0;
}

// 0x55A4F0 (thiscall): the blip being flashed stopped (+0x20: 1 the main mission's, +0x28;
// 2 the objective's, +0x30), none after, +0x24 back to -1
void __fastcall ClearFlashingBlip(uint8_t* self, re::Edx) {
    void* radar = g_radar;
    int32_t kind = At<int32_t>(self, 0x20);
    if (kind == 1 || kind == 2) {
        int id = kind == 1 ? At<int32_t>(self, 0x28) : At<int32_t>(self, 0x30);
        Radar_StopBlipFlashing(radar, Radar_FindBlipById(radar, id));
    }
    At<int32_t>(self, 0x20) = 0;
    At<float>(self, 0x24) = -1.0f;   // (0x9009AC)
}

// 0x55A460: a secondary one (kind 1 or 3), outside class and 0x6AA890
uint8_t __fastcall IsSecondaryAvailable(void*, re::Edx, int id) {
    if (id < 0) {
        return 0;
    }
    if (*reinterpret_cast<int32_t*>(MissionMgr_GetInfo(kMissionMgr, id) + 0x28) != 1 &&
        *reinterpret_cast<int32_t*>(MissionMgr_GetInfo(kMissionMgr, id) + 0x28) != 3) {
        return 0;
    }
    if (MissionMgr_GetInfo(kMissionMgr, id)[0x25] != 0) {
        return 0;
    }
    if (MissionMgr_GetStats(kMissionMgr, id)[4] & 0x40) {
        return 0;
    }
    if (!(MissionMgr_GetStats(kMissionMgr, id)[4] & 1)) {
        return 0;
    }
    if (MissionMgr_InClass(kMissionMgr)) {
        return 0;
    }
    return MissionMgr_6AA890(kMissionMgr) ? 0 : 1;
}

// 0x55A5E0 (`self` unused): a line of task text in the task font, shadowed
void __fastcall DrawTaskText(void*, re::Edx, const uint16_t* text) {
    Font_UseBlock(kTaskFont);
    float scaled = float(double(g_fontStyleHeights[g_taskStyle]) * 3.0);
    int32_t shadow = int32_t(double(scaled) + 0.5);
    float y = float(double(g_taskY) + 40.0);
    g_fontFlags |= 0x20;
    UseTaskColour();
    g_fontShadow = int16_t(shadow);
    Font_PrintString(g_taskX, y, text, 1);
    g_fontFlags &= 0xDF;
}

// 0x55A690: the current objective's text
void __fastcall DrawObjective(HUDTasks* self, re::Edx) {
    uint16_t text[0x200];
    int id = self->unknown28[4];
    if (id < 0) {
        return;
    }
    if (uint8_t* objective = Objectives_Get(kObjectives, id)) {
        Objective_GetText(objective, text, 0x1FF);
        Tasks_DrawTaskText_(self, text);
    }
}

// 0x55A6E0: show a main mission (its blip selected)
void __fastcall SetMain(HUDTasks* self, re::Edx, int id) {
    ClearTaskBlip(self);
    self->unknown24 = -1.0f;
    self->blipType = 1;
    self->unknown28[0] = id;
    self->unknown28[1] = MissionMgr_GetIcon(kMissionMgr, id);
    void* radar = g_radar;
    int handle = Radar_FindBlipById(radar, id);
    if (handle >= 0) {
        Radar_SelectBlip(radar, handle);
    }
    self->unknown24 = 1.5f;
}

// 0x55A760: show a secondary one
void __fastcall SetSecondary(HUDTasks* self, re::Edx, int id) {
    ClearTaskBlip(self);
    self->unknown24 = -1.0f;
    self->blipType = 2;
    self->unknown28[2] = id;
    self->unknown28[3] = MissionMgr_GetIcon(kMissionMgr, id);
    void* radar = g_radar;
    int handle = Radar_FindBlipById(radar, id);
    if (handle >= 0) {
        Radar_SelectBlip(radar, handle);
    }
    self->unknown24 = 1.5f;
}

// 0x55A7E0: show an objective (the door blips flash)
void __fastcall SetObjective(HUDTasks* self, re::Edx, int id) {
    ClearTaskBlip(self);
    self->unknown24 = 1.5f;
    self->blipType = 3;
    self->unknown28[4] = id;
    Radar_FlashDoorBlips(g_radar);
}

// 0x55A840: the next secondary mission that can be shown (the current one first)
void __fastcall NextSecondary(HUDTasks* self, re::Edx) {
    if (self->blipType != 2 && Tasks_IsSecondaryAvailable_(self, self->unknown28[2])) {
        Tasks_SetSecondary_(self, self->unknown28[2]);
        return;
    }
    int count = MissionMgr_Count(kMissionMgr);
    for (int i = 1; i <= count; i++) {
        int id = self->unknown28[2] + i;
        if (count <= id) {
            id -= count;
        }
        if (Tasks_IsSecondaryAvailable_(self, id)) {
            Tasks_SetSecondary_(self, id);
            return;
        }
    }
}

// 0x55A8B0: the next objective (the current one first; from the start again when
// the list wraps)
void __fastcall NextObjective(HUDTasks* self, re::Edx) {
    int32_t& id = self->unknown28[4];
    if (self->blipType != 3 && id >= 0 && Objectives_Exists(kObjectives, id) && !Objectives_Get(kObjectives, id)[0x1D]) {
        Tasks_SetObjective_(self, id);
        return;
    }
    if (id < 0) {
        Objectives_Rewind(kObjectives);
    } else {
        Objectives_SeekTo(kObjectives, id);
        Objectives_Next(kObjectives, &id);
    }
    if (Objectives_Next(kObjectives, &id) ||
        (g_objectivesWrap > 0 && (Objectives_Rewind(kObjectives), Objectives_Next(kObjectives, &id)))) {
        Tasks_SetObjective_(self, id);
        return;
    }
    id = -1;
    Tasks_Clear(self);
}

// 0x55A970: the next main mission that can be shown
void __fastcall NextMain(HUDTasks* self, re::Edx) {
    if (self->blipType != 1 && Tasks_IsMainAvailable_(self, self->unknown28[0])) {
        Tasks_SetMain_(self, self->unknown28[0]);
        return;
    }
    int count = MissionMgr_Count(kMissionMgr);
    for (int i = 1; i <= count; i++) {
        int id = self->unknown28[0] + i;
        if (count <= id) {
            id -= count;
        }
        if (id >= 0 && *reinterpret_cast<int32_t*>(MissionMgr_GetInfo(kMissionMgr, id) + 0x28) == 0 &&
            MissionMgr_GetInfo(kMissionMgr, id)[0x25] == 0 && !(MissionMgr_GetStats(kMissionMgr, id)[4] & 0x40) &&
            (MissionMgr_GetStats(kMissionMgr, id)[4] & 1)) {
            Tasks_SetMain_(self, id);
            return;
        }
    }
}

// 0x55AA40: a mission's line, centred, with its radar icon (-1: none) before it
void __fastcall DrawLine(HUDTasks* self, re::Edx, int icon, const uint16_t* text) {
    (void)self;
    float iconW = float(double(g_screenScaleX) * 32.0);
    float iconH = float(32.0 * g_screenScaleY);
    Font_UseBlock(kTaskFont);
    Font_SetJustify(0);
    float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    Font_GetTextRect(rect, 0.0f, 0.0f, text);
    float y = float(double(g_taskY) + 40.0);
    if (icon != -1) {
        void* sprite = Radar_GetSlot3A98(g_radar, icon);
        double gap = 6.0 + iconW;
        float width = std::fabs(float(rect[2] - rect[0]));
        float x0 = float(320.0 - double(float(gap + width)) * 0.5);
        float y0 = float(double(y) - 2.0);
        col::CRect box = {x0, float(y0 + double(iconH)), float(double(x0) + iconW), y0};
        CRGBA white;
        RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
        Sprite_Draw(sprite, &box, &white);
        float scaled = float(double(g_fontStyleHeights[g_taskStyle]) * 3.0);
        int32_t shadow = int32_t(double(scaled) + 0.5);
        g_fontFlags |= 0x20;
        g_fontShadow = int16_t(shadow);
        UseTaskColour();
        Font_PrintString(float(double(x0) + gap), y, text, 1);
        g_fontFlags &= 0xDF;
        return;
    }
    float scaled = float(double(g_fontStyleHeights[g_taskStyle]) * 3.0);
    int32_t shadow = int32_t(double(scaled) + 0.5);
    g_fontFlags |= 0x20;
    UseTaskColour();
    g_fontShadow = int16_t(shadow);
    float width = std::fabs(float(rect[2] - rect[0]));
    Font_PrintString(float(320.0 - 0.5 * width), y, text, 1);
    g_fontFlags &= 0xDF;
}

// 0x55ACD0: the main mission's line
void __fastcall DrawMain(HUDTasks* self, re::Edx) {
    if (self->unknown28[0] >= 0) {
        uint8_t* info = MissionMgr_GetInfo(kMissionMgr, self->unknown28[0]);
        Tasks_DrawLine_(self, self->unknown28[1], Text_Get(kTheText, *reinterpret_cast<re::Ptr32<const char>*>(info + 0x20)));
    }
}

// 0x55AD10: the secondary mission's line
void __fastcall DrawSecondary(HUDTasks* self, re::Edx) {
    if (self->unknown28[2] >= 0) {
        uint8_t* info = MissionMgr_GetInfo(kMissionMgr, self->unknown28[2]);
        Tasks_DrawLine_(self, self->unknown28[3], Text_Get(kTheText, *reinterpret_cast<re::Ptr32<const char>*>(info + 0x20)));
    }
}

// 0x55AD50 (vf8 Draw): the d-pad cycles (right: secondaries, left: missions or
// objectives), the chosen line shown for its 1.5 seconds
void __fastcall Draw(HUDTasks* self, re::Edx) {
    if (IsPaused_5142D0() || IsInCutscene_705500()) {
        ClearTaskBlip(self);
        self->blipType = 0;
        self->unknown24 = -1.0f;
        return;
    }
    if (!MissionMgr_6AA7E0(kMissionMgr)) {
        self->unknown28[4] = -1;
    }
    if (self->blipType == 3) {
        int id = self->unknown28[4];
        if (id < 0 || !Objectives_Exists(kObjectives, id) || Objectives_Get(kObjectives, id)[0x1D]) {
            self->unknown28[4] = -1;
            self->blipType = 0;
        }
    }
    if (Controls_50DF50(reinterpret_cast<void*>(0x00A45A30))) {
        if (Pad_JustPressed(0, 0)) {
            Tasks_NextSecondary_(self);
        } else if (Pad_JustPressed(1, 0)) {
            if (MissionMgr_6AA7E0(kMissionMgr)) {
                Tasks_NextObjective_(self);
            } else {
                Tasks_NextMain_(self);
            }
        }
    }
    if (!(0.0f < self->unknown24)) {
        return;
    }
    int type = self->blipType;
    float step = float(double(g_taskSeconds) * g_timeStep);
    self->unknown24 = float(double(self->unknown24) - step);
    if (type == 1) {
        Tasks_DrawMain_(self);
    } else if (type == 2) {
        Tasks_DrawSecondary_(self);
    } else if (type == 3) {
        Tasks_DrawObjective_(self);
    }
    if (0.0f <= self->unknown24) {
        return;
    }
    Tasks_Clear(self);
}

// 0x55AEE0: the tattoo parlour
void* __cdecl GetTattooParlour() {
    return g_tattooParlour;
}

// 0x55AEF0 (HUDTattoo's vf11 OnShow)
void __fastcall Tattoo_OnShow(HUDComponent*, re::Edx) {
    Fn_738780();
    MissionMgr_6A9F80(kMissionMgr, 1);
    Fn_6CB550();
    Tattoo_6CFF80(g_tattooParlour);
    Fn_455190(reinterpret_cast<void*>(0x00C18748));
}

// 0x55AF20 (HUDTattoo's vf12 OnHide)
void __fastcall Tattoo_OnHide(HUDComponent*, re::Edx) {
    Tattoo_6D00A0(g_tattooParlour);
    Fn_6CB620();
    MissionMgr_6A9F80(kMissionMgr, 0);
}

// 0x55AF40 (HUDTattoo's vf8 Draw): the parlour draws itself
void __fastcall Tattoo_Draw_(HUDComponent*, re::Edx) {
    Tattoo_Draw(g_tattooParlour);
}

// 0x55AF80: a new parlour (the old one destroyed)
void __cdecl SetTattooParlour(void* parlour) {
    void* old = g_tattooParlour;
    if (old) {
        Tattoo_Destruct(old);
        OperatorDelete(old);
    }
    g_tattooParlour = parlour;
}

// 0x55B1E0 (HUDText's vf8 Draw): each slot with text, formatted with its params, in
// its print slot (0 the subtitle, above the box; 1 the task line; 3 hidden by the
// completion screen), then cleared
void __fastcall Text_Draw(HUDText* self, re::Edx) {
    uint16_t line[0x200];
    for (int i = 0; i < 8; i++) {
        HUDText::Slot& slot = self->slots[i];
        if (slot.text[0] == '\0') {
            continue;
        }
        Font_Begin();
        uint8_t* slotColour = g_fontSlots + i * 0x38;
        uint8_t saved[4];
        std::memcpy(saved, slotColour, 4);
        for (int p = 0; p < slot.numParams; p++) {
            HUDTextParam& param = slot.params[p];
            switch (param.type) {
            case 1:
                Font_AddNumber(float(*reinterpret_cast<int32_t*>(param.value)));
                break;
            case 2:
                Font_AddNumber(*reinterpret_cast<float*>(param.value));
                break;
            case 3:
                Font_AddString(Text_Get(kTheText, reinterpret_cast<const char*>(param.value)));
                break;
            case 4: {
                Font_AddString(reinterpret_cast<const uint16_t*>(param.value));
                CRGBA red;
                RGBA_Construct(&red, 0xA0, 0x30, 0x30, 0xFF);
                std::memcpy(&g_fontColour, &red, 4);
                std::memcpy(slotColour, &red, 4);
                break;
            }
            default:
                break;
            }
        }
        Font_FormatText(line, Text_Get(kTheText, slot.text), 0x200);
        if (i == 0) {
            if (!g_cutsceneRunning &&
                !reinterpret_cast<uint8_t(__thiscall*)(HUDComponent*)>(static_cast<void*>(g_hudC674E4->vtable[4]))(g_hudC674E4)) {
                Font_UseBlock(kSubtitleFont);
                float x = g_subtitleX;
                float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
                Font_GetTextRect(rect, x, g_subtitleY, line);
                float height = float(rect[3] - rect[1]);
                height = std::fabs(height);
                Font_PrintString(x, float(430.0 - double(height)), line, 1);
            }
        } else if (i == 1) {
            if (!IsInCutscene_705500() || (IsInCutscene_705500() && g_showingCompletion)) {
                Font_UseBlock(kTaskFont);
                if (g_taskEdge != 0) {
                    g_fontEdge = 4;
                    g_fontShadow = 0;
                }
                float x = g_taskX;
                float y = g_taskY;
                float scaled = float(double(g_fontStyleHeights[g_taskStyle]) * 3.0);
                float savedScale = g_fontScale;
                if (g_language == 7) {
                    g_fontScale = 0.9f;
                    Font_SetSizePreset(1);
                }
                int32_t shadow = int32_t(double(scaled) + 0.5);
                g_fontFlags |= 0x20;
                g_fontShadow = int16_t(shadow);
                UseTaskColour();
                Font_PrintString(x, y, line, 1);
                g_fontFlags &= 0xDF;
                if (g_language == 7) {
                    g_fontScale = savedScale;
                }
            }
        } else if (i == 3) {
            if (!IsInCutscene_705500() || (IsInCutscene_705500() && g_showingCompletion)) {
                Font_PrintInSlot(3, line, 0);
            }
        } else {
            Font_PrintInSlot(i, line, 0);
        }
        Font_Render();
        std::memcpy(slotColour, saved, 4);
        slot.numParams = 0;
        slot.text[0] = '\0';
    }
}

// 0x55B5F0: +0x2C with the widescreen text, else +0x28
float __fastcall PickForWidescreen(uint8_t* self, re::Edx) {
    return g_widescreenText ? At<float>(self, 0x2C) : At<float>(self, 0x28);
}

// 0x55B6E0: the tutorial's text key (empty: hidden)
void __fastcall Tutorial_SetText(HUDTutorial* self, re::Edx, const char* key) {
    if (key && key[0] != '\0') {
        std::strncpy(reinterpret_cast<char*>(self) + 0x21, key, 0x10);
        Tutorial_59C150(reinterpret_cast<void*>(0x00CF3DA0), 0x1D);
        return;
    }
    reinterpret_cast<void(__thiscall*)(HUDComponent*)>(static_cast<void*>(self->vtable[7]))(self);
}

// 0x55B740: a random tip
const char* __cdecl RandomTip() {
    int r = GameRand();
    return g_tutorialTips[(r * 0x47) / 0x7FFF];
}

// 0x55B7F0 (HUDTutorial's vf8 Draw): the text in a dark box, gone after its time
void __fastcall Tutorial_Draw(HUDTutorial* self, re::Edx) {
    if (self->unknown38 > 0) {
        int64_t elapsed = int64_t(double(g_tutorialSeconds) * g_timeStep * 1000.0);
        self->unknown34 += int32_t(elapsed);
        if (self->unknown34 > self->unknown38) {
            reinterpret_cast<void(__thiscall*)(HUDComponent*)>(static_cast<void*>(self->vtable[7]))(self);
            return;
        }
    }
    const char* key = reinterpret_cast<const char*>(self) + 0x21;
    if (key[0] == '\0') {
        return;
    }
    if (g_language == 7 || g_language == 6) {
        g_tutorialScaleX = 0.5f;
        g_tutorialScaleY = 0.5f;
    }
    CRGBA box;
    RGBA_Construct(&box, 0x10, 0x10, 0x10, 0x40);
    Font_Begin();
    Font_UseBlock(kTutorialFont);
    if (g_tutorialEdge != 0) {
        g_fontEdge = 1;
        g_fontShadow = 0;
    }
    float scaleY = g_screenScaleY * g_tutorialScaleY;
    float scaleX = g_screenScaleX * g_tutorialScaleX;
    Font_SetScaleXY(scaleX, scaleY);
    uint16_t line[0x200];
    Font_FormatText(line, Text_Get(kTheText, key), 0x200);
    float x = g_tutorialX * g_screenScaleX;
    float y = g_tutorialY;
    float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
    Font_GetTextRect(rect, x, y, line);
    float minRight = g_widescreenText ? g_tutorialMinRightWide : g_tutorialMinRight;
    if (rect[2] < minRight) {
        rect[2] = g_widescreenText ? g_tutorialMinRightWide : g_tutorialMinRight;
    }
    rect[1] = float(double(rect[1]) - 2.0);
    rect[0] = float(double(rect[0]) - 4.0);
    rect[2] = float(double(rect[2]) + 4.0);
    rect[3] = float(4.0 + double(rect[3]));
    DrawRect(reinterpret_cast<col::CRect*>(rect), &box);
    g_fontShadow = 2;
    CRGBA black;
    RGBA_Construct(&black, 0, 0, 0, 0xFF);
    std::memcpy(&g_fontDropColour, &black, 4);
    Font_PrintString(x, y, line, 1);
    Font_Render();
}

// 0x55BA70 (HUDWeapons): a weapon the ped can switch to now (-1: none)
uint8_t __fastcall Weapons_CanSelect(uint8_t* self, re::Edx, uint8_t* ped, int weapon) {
    if (!self[0x15C] && weapon == g_weaponNone) {
        return 0;
    }
    if (self[0x15D] && weapon == g_weaponUnequip) {
        return 1;
    }
    if (Clothing_IsWearingOutfit(kClothing, g_outfitHash) &&
        (weapon == g_weaponOutfit1 || weapon == g_weaponOutfit2 || weapon == g_weaponOutfit3 || weapon == g_weaponOutfit4)) {
        return 0;
    }
    if (weapon == g_weaponMode1 || weapon == g_weaponMode2) {
        uint8_t* mode = g_gameMode;
        if (mode && mode[4]) {
            int32_t kind = *reinterpret_cast<int32_t*>(mode + 0x3C);
            if (kind == 0xE || kind == 0xB) {
                return 0;
            }
        }
    }
    void* inventory = *reinterpret_cast<re::Ptr32<void>*>(ped + 0x1C4);
    if (!Inventory_Has(inventory, weapon)) {
        if (weapon != -1) {
            return 0;
        }
    } else if (weapon != -1) {
        int ammo = weapon;
        if (GetWeaponInfo(weapon)) {
            uint8_t* info = GetWeaponInfo(weapon);
            WeaponInfo_7282A0(info);
            ammo = *reinterpret_cast<int32_t*>(info + 0x84);
        }
        if (ammo <= 0) {
            ammo = weapon;
        }
        int kind = GetWeaponInfo(ammo) ? *reinterpret_cast<int32_t*>(GetWeaponInfo(ammo) + 0x34) : 1;
        if (kind != 1 && kind != 0xFF && kind != -1 && !Inventory_Has(inventory, ammo)) {
            return 0;
        }
    }
    uint8_t* vehicle = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x1554);
    if (vehicle && *reinterpret_cast<int32_t*>(vehicle + 0x46C) == 1 && weapon != -1) {
        return GetWeaponInfo(weapon)[0x29];
    }
    return 1;
}

// 0x55BBD0 (`self` unused): the weapon's info +0x64 is set
uint8_t __fastcall Weapons_Has64(void*, re::Edx, int weapon) {
    if (weapon != -1 && *reinterpret_cast<int32_t*>(GetWeaponInfo(weapon) + 0x64) != 0) {
        return 1;
    }
    return 0;
}

// 0x55BC00 (`self` unused): the player's weapon to switch to (-1 when not carried)
void __fastcall Weapons_SetPlayerWeapon(void*, re::Edx, int weapon) {
    uint8_t* player = g_playerPed;
    if (!Inventory_Has(*reinterpret_cast<re::Ptr32<void>*>(player + 0x1C4), weapon)) {
        weapon = -1;
    }
    uint8_t* current = *reinterpret_cast<re::Ptr32<uint8_t>*>(player + 0x1D0);
    if (int32_t(*reinterpret_cast<int16_t*>(current + 0x10E)) != weapon) {
        *reinterpret_cast<int32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(player + 0x2E0) + 0x18) = weapon;
    }
}

// 0x55BC40 (`self` unused): the weapon's info +0x48 (0: none)
int32_t __fastcall Weapons_Info48(void*, re::Edx, int weapon) {
    if (weapon == -1) {
        return 0;
    }
    uint8_t* info = GetWeaponInfo(weapon);
    return info ? *reinterpret_cast<int32_t*>(info + 0x48) : 0;
}

// 0x55BC70
void __fastcall Weapons_Set13C(uint8_t* self, re::Edx, int value) {
    self[0x13C] = uint8_t(value);
}

// 0x55BC80
void __fastcall Weapons_Clear12C(uint8_t* self, re::Edx) {
    self[0x12C] = 0;
}

// 0x55BC90: fade in (0: held for 3 seconds) or out (1), at a speed for the distance
void __fastcall Weapons_SetFade(uint8_t* self, re::Edx, int fadeOut) {
    float current = At<float>(self, 0x1C);
    At<float>(self, 0x16C) = current;
    uint32_t now = g_timeInMilliseconds;
    if (fadeOut == 0) {
        if (1.0f == At<float>(self, 0x160)) {
            At<uint32_t>(self, 0x170) = now + 3000;
            return;
        }
        At<float>(self, 0x160) = 1.0f;
        At<float>(self, 0x168) = 3.0f;
        float distance = std::fabs(float(1.0 - double(current)));
        At<uint32_t>(self, 0x170) = now + 3000;
        At<uint32_t>(self, 0x164) = uint32_t(int64_t(double(distance) / 3.0 * 1000.0 + double(now)));
        return;
    }
    if (fadeOut == 1 && !(0.0f == At<float>(self, 0x160))) {
        At<float>(self, 0x160) = 0.0f;
        At<float>(self, 0x168) = 0.5f;
        float distance = std::fabs(float(0.0 - double(current)));
        At<uint32_t>(self, 0x164) = uint32_t(int64_t((double(distance) + distance) * 1000.0 + double(now)));
    }
}

// 0x55BE00: the fade moved towards its target (fading out once the hold's over)
void __fastcall Weapons_UpdateFade(uint8_t* self, re::Edx) {
    uint32_t now = g_timeInMilliseconds;
    if (At<uint32_t>(self, 0x170) < now) {
        Weapons_SetFade_(self, 1);
    }
    uint32_t end = At<uint32_t>(self, 0x164);
    if (end > now) {
        double diff = double(At<float>(self, 0x160)) - At<float>(self, 0x16C);
        float target = At<float>(self, 0x160);
        double left = double(end - now);
        float span = std::fabs(float(diff));
        float q = float(left / (double(span) / At<float>(self, 0x168) * 1000.0));
        At<float>(self, 0x1C) = float(double(target) - diff * q);
        return;
    }
    At<float>(self, 0x1C) = At<float>(self, 0x160);
}

// 0x55BE90: the weapon wheel: five slots round an arc (the middle one the current),
// turning towards place, fading at the ends and with the HUD
void __fastcall Weapons_DrawWheel(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0xC8) == 0) {
        return;
    }
    float ease = float(std::pow(0.800000011920929, double(g_timeStep)));
    uint32_t end = At<uint32_t>(self, 0x148);
    float& angle = At<float>(self, 0x11C);
    angle = float((2.35619455575943 - double(angle)) * ease + angle);
    At<float>(self, 0x120) = 130.0f;
    uint32_t now = g_timeInMilliseconds;
    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    float fade = 1.0f;
    if (now < end) {
        if (!(double(now) < double(end) - 250.0 || double(now) == double(end) - 250.0)) {
            float u = float(1.0 - double(end - now) / 250.0);
            float uu = u * u;
            fade = float(1.0 - double(uu));
        }
    }
    float radius = float(double(sx) * 45.0);
    for (int i = 0; i < 5; i++) {
        float a = float(double(i - 2) * 0.4712389298772379 + angle);
        float x = radius;
        float offset = std::fabs(float(double(a) - 2.35619455575943));
        float t = float(double(offset) / 0.9424778597544758);
        if (!(1.0f > t)) {
            t = 1.0f;
        }
        float w = float(1.0 - double(t));
        uint32_t from = g_wheelColourFar;
        uint32_t to = g_wheelColourNear;
        auto channel = [w](uint32_t f, uint32_t tt, int shift) {
            int32_t fc = int32_t(f >> shift & 0xFF);
            int32_t tc = int32_t(tt >> shift & 0xFF);
            return uint8_t(int32_t(double(fc) + double(w) * double(tc - fc)));
        };
        CRGBA colour;
        colour.r = channel(from, to, 0);
        colour.g = uint8_t(int32_t(double(int32_t(to >> 8 & 0xFF) - int32_t(from >> 8 & 0xFF)) * w +
                                   double(int32_t(from >> 8 & 0xFF))));
        colour.b = uint8_t(int32_t(double(int32_t(to >> 16 & 0xFF) - int32_t(from >> 16 & 0xFF)) * w +
                                   double(int32_t(from >> 16 & 0xFF))));
        uint8_t alpha = uint8_t(int32_t(double(int32_t(to >> 24) - int32_t(from >> 24)) * w + double(int32_t(from >> 24))));
        float hud = At<float>(self, 0x1C) * g_hudAlpha;
        alpha = uint8_t(int32_t(double(alpha) * hud));
        if (i != 2) {
            alpha = uint8_t(int32_t(double(alpha) * fade));
        }
        colour.a = alpha;
        float size = float(double(w) * 0.699999988079071 + 0.30000001192092896);
        float s = float(std::sin(double(a)));
        x = float(double(s) * At<float>(self, 0x120) * sx + x);
        float c = float(std::cos(double(a)));
        int weapon = At<int32_t>(self, 0x20 + At<int32_t>(self, 0xF4 + i * 8) * 4);
        float y = float(double(c) * At<float>(self, 0x120) * sy + 438.0);
        CVector2D uv2 = {1.0f, 1.0f};
        CVector2D uv1 = {0.0f, 0.0f};
        double half = double(size) * 32.0;
        double hh = double(sy) * half;
        double hw = half * sx;
        float y2 = float(y + hh);
        float x2 = float(x + hw);
        float y1 = float(y - hh);
        float x1 = float(x - hw);
        DrawRectUV(x1, y1, x2, y2, g_weaponWheelBack, &colour, &uv1, &uv2);
        void* icon;
        CVector2D iconUv2 = {1.0f, 1.0f};
        CVector2D iconUv1 = {0.0f, 0.0f};
        if (weapon == -1) {
            icon = g_weaponWheelEmpty;
        } else {
            icon = GetWeaponIcon(weapon);
        }
        DrawRectUV(x1, y1, x2, y2, icon, &colour, &iconUv1, &iconUv2);
    }
}

// 0x55C340: the wheel's list refreshed: which weapons can be picked, the current one
// (the player's, else the last one or the first that can), the slots either side
void __fastcall Weapons_Refresh(uint8_t* self, re::Edx) {
    float button = Pad_GetValue(10, 0);
    At<float>(self, 0x120) = 0.0f;
    self[0x15C] = button <= 0.0f;
    At<int32_t>(self, 0xC8) = 0;
    uint32_t count = At<uint32_t>(self, 0x6C);
    int32_t* list = &At<int32_t>(self, 0x20);
    uint8_t* flags = self + 0xCC;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x6C); i++) {
        if (Weapons_CanSelect_(self, g_playerPed, list[i])) {
            flags[i] = 1;
            At<int32_t>(self, 0xC8) += 1;
        } else {
            flags[i] = 0;
        }
    }
    int32_t& current = At<int32_t>(self, 0xC0);
    int32_t& index = At<int32_t>(self, 0xC4);
    if (At<int32_t>(self, 0xC8) == 0) {
        current = -1;
        index = -1;
        return;
    }
    auto firstSelectable = [&]() {
        for (uint32_t i = 0; i < At<uint32_t>(self, 0x6C); i++) {
            if (Weapons_CanSelect_(self, g_playerPed, list[i])) {
                index = int32_t(i);
                current = list[i];
                return;
            }
        }
    };
    int held = int32_t(*reinterpret_cast<int16_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(g_playerPed + 0x1D0) + 0x10E));
    if (held == -1) {
        if (index == -1 || !Weapons_CanSelect_(self, g_playerPed, list[index])) {
            firstSelectable();
        } else {
            current = list[index];
        }
    } else {
        count = At<uint32_t>(self, 0x6C);
        uint32_t i = 0;
        for (; i < count; i++) {
            if (held == list[i]) {
                index = int32_t(i);
                current = held;
                break;
            }
        }
        if (count <= i && !Weapons_CanSelect_(self, g_playerPed, current)) {
            firstSelectable();
        }
    }
    if (At<int32_t>(self, 0xC8) != 0) {
        for (int s = 0; s < 5; s++) {
            At<int32_t>(self, 0xF4 + s * 8) = index;
        }
    }
    if (At<int32_t>(self, 0xC8) > 1) {
        int32_t back = index;
        for (int s = 1; s >= 0; s--) {
            do {
                back--;
                if (back < 0) {
                    back = At<int32_t>(self, 0x6C) - 1;
                }
            } while (flags[back] == 0);
            At<int32_t>(self, 0xF4 + s * 8) = back;
        }
        uint32_t forward = uint32_t(index);
        for (int s = 3; s < 5; s++) {
            do {
                forward++;
                if (At<uint32_t>(self, 0x6C) - 1u < forward) {
                    forward = 0;
                }
            } while (flags[forward] == 0);
            At<uint32_t>(self, 0xF4 + s * 8) = forward;
        }
    }
    At<float>(self, 0x11C) = 2.356194496154785f;
}

} // namespace hudtasks

#define HUDTASKS_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(hudtasks_original_, __LINE__);                                       \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudtasks_original_, __LINE__), \
             re::Mode::Original)

HUDTASKS_REVERSED("HUDTasks::IsMainAvailable", 0x0055A400, &hudtasks::IsMainAvailable);
HUDTASKS_REVERSED("HUDTasks::IsSecondaryAvailable", 0x0055A460, &hudtasks::IsSecondaryAvailable);
HUDTASKS_REVERSED("HUDTasks::ClearFlashingBlip", 0x0055A4F0, &hudtasks::ClearFlashingBlip);
HUDTASKS_REVERSED("HUDTasks::DrawTaskText", 0x0055A5E0, &hudtasks::DrawTaskText);
HUDTASKS_REVERSED("HUDTasks::DrawObjective", 0x0055A690, &hudtasks::DrawObjective);
HUDTASKS_REVERSED("HUDTasks::SetMain", 0x0055A6E0, &hudtasks::SetMain);
HUDTASKS_REVERSED("HUDTasks::SetSecondary", 0x0055A760, &hudtasks::SetSecondary);
HUDTASKS_REVERSED("HUDTasks::SetObjective", 0x0055A7E0, &hudtasks::SetObjective);
HUDTASKS_REVERSED("HUDTasks::NextSecondary", 0x0055A840, &hudtasks::NextSecondary);
HUDTASKS_REVERSED("HUDTasks::NextObjective", 0x0055A8B0, &hudtasks::NextObjective);
HUDTASKS_REVERSED("HUDTasks::NextMain", 0x0055A970, &hudtasks::NextMain);
HUDTASKS_REVERSED("HUDTasks::DrawLine", 0x0055AA40, &hudtasks::DrawLine);
HUDTASKS_REVERSED("HUDTasks::DrawMain", 0x0055ACD0, &hudtasks::DrawMain);
HUDTASKS_REVERSED("HUDTasks::DrawSecondary", 0x0055AD10, &hudtasks::DrawSecondary);
HUDTASKS_REVERSED("HUDTasks::Draw", 0x0055AD50, &hudtasks::Draw);
HUDTASKS_REVERSED("HUDTattoo::GetParlour", 0x0055AEE0, &hudtasks::GetTattooParlour);
HUDTASKS_REVERSED("HUDTattoo::OnShow", 0x0055AEF0, &hudtasks::Tattoo_OnShow);
HUDTASKS_REVERSED("HUDTattoo::OnHide", 0x0055AF20, &hudtasks::Tattoo_OnHide);
HUDTASKS_REVERSED("HUDTattoo::Draw", 0x0055AF40, &hudtasks::Tattoo_Draw_);
HUDTASKS_REVERSED("HUDTattoo::SetParlour", 0x0055AF80, &hudtasks::SetTattooParlour);
HUDTASKS_REVERSED("HUDText::Draw", 0x0055B1E0, &hudtasks::Text_Draw);
HUDTASKS_REVERSED("HUD::PickForWidescreen", 0x0055B5F0, &hudtasks::PickForWidescreen);
HUDTASKS_REVERSED("HUDTutorial::SetText", 0x0055B6E0, &hudtasks::Tutorial_SetText);
HUDTASKS_REVERSED("HUDTutorial::RandomTip", 0x0055B740, &hudtasks::RandomTip);
HUDTASKS_REVERSED("HUDTutorial::Draw", 0x0055B7F0, &hudtasks::Tutorial_Draw);
HUDTASKS_REVERSED("HUDWeapons::CanSelect", 0x0055BA70, &hudtasks::Weapons_CanSelect);
HUDTASKS_REVERSED("HUDWeapons::Has64", 0x0055BBD0, &hudtasks::Weapons_Has64);
HUDTASKS_REVERSED("HUDWeapons::SetPlayerWeapon", 0x0055BC00, &hudtasks::Weapons_SetPlayerWeapon);
HUDTASKS_REVERSED("HUDWeapons::Info48", 0x0055BC40, &hudtasks::Weapons_Info48);
HUDTASKS_REVERSED("HUDWeapons::Set13C", 0x0055BC70, &hudtasks::Weapons_Set13C);
HUDTASKS_REVERSED("HUDWeapons::Clear12C", 0x0055BC80, &hudtasks::Weapons_Clear12C);
HUDTASKS_REVERSED("HUDWeapons::SetFade", 0x0055BC90, &hudtasks::Weapons_SetFade);
HUDTASKS_REVERSED("HUDWeapons::UpdateFade", 0x0055BE00, &hudtasks::Weapons_UpdateFade);
HUDTASKS_REVERSED("HUDWeapons::DrawWheel", 0x0055BE90, &hudtasks::Weapons_DrawWheel);
HUDTASKS_REVERSED("HUDWeapons::Refresh", 0x0055C340, &hudtasks::Weapons_Refresh);
