// The HUD's small kinds: the barber's and the clothing shop's menus (drawn by their
// menu objects), Big_Text_For_Script's drawing (through the Text component), the clock
// warnings. 0x53A4D0..0x53CD90 (the rest of these kinds is Hud.cpp's).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Hud.h"
#include "game/Pools.h"
#include "reversed.h"

namespace {

// memory
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// the menus' destructor (a thunk to 0x5D9C50)
GAME_FN(void, __thiscall, Menu_Destruct, 0x005B1EB0, void* menu);
GAME_FN(void, __cdecl, Pad_StopRumble, 0x00738780);
// the mission manager: the player's controls locked (in a menu) or not
GAME_FN(void, __thiscall, MissionMgr_SetInMenu, 0x006A9F80, void* mgr, int inMenu);
GAME_FN(uint8_t, __thiscall, MissionMgr_Fn6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionMgr_Fn6AA8F0, 0x006AA8F0, void* mgr);
GAME_FN(void, __thiscall, MissionMgr_ClassWarning, 0x006AA030, void* mgr);
// the pause menu's frame (made around the shops' menus)
GAME_FN(void, __cdecl, PauseMenu_Create, 0x006CB550);
GAME_FN(void, __cdecl, PauseMenu_Finish, 0x006CB620);
GAME_FN(void, __thiscall, Fn_6CA7B0, 0x006CA7B0, void* object);
// the barber's menu
GAME_FN(void, __thiscall, BarberMenu_Open, 0x006CCDB0, void* menu);
GAME_FN(void, __thiscall, BarberMenu_Close, 0x006CCFD0, void* menu);
GAME_FN(void, __thiscall, BarberMenu_Draw, 0x006CD6A0, void* menu);
// the clothing shop's
GAME_FN(void, __thiscall, ClothingMenu_Open, 0x006CEF10, void* menu);
GAME_FN(void, __thiscall, ClothingMenu_Close, 0x006CF020, void* menu);
GAME_FN(void, __thiscall, ClothingMenu_Draw, 0x006CF970, void* menu);
// the shop (0xC18748): its money panel's flashing stopped
GAME_FN(void, __thiscall, CShop_ClearFlash, 0x00455190, void* shop);
// the class schedule
GAME_FN(int, __thiscall, Schedule_GetCurrent, 0x006A8CF0, void* schedule);
GAME_FN(uint8_t, __thiscall, Schedule_Test, 0x006A8BB0, void* schedule, int index, int a, int b);
GAME_FN(int, __thiscall, Schedule_GetType, 0x006A8CC0, void* schedule, int index);
GAME_FN(int, __thiscall, Schedule_Fn6A8D30, 0x006A8D30, void* schedule, int index);
GAME_FN(int, __thiscall, Schedule_Fn6A8D70, 0x006A8D70, void* schedule, int index);
// the Text component's
GAME_FN(void, __thiscall, HUDText_AddFloat, 0x0055B020, void* text, int slot, float value);
GAME_FN(void, __thiscall, HUDText_AddString, 0x0055B070, void* text, int slot, const void* string);
GAME_FN(void, __thiscall, HUDText_AddText, 0x0055B0D0, void* text, int slot, const void* text16);
GAME_FN(void, __thiscall, HUDText_SetText, 0x0055B130, void* text, int slot, const char* string);
GAME_FN(void, __thiscall, HUDScriptText_Clear, 0x005539D0, HUDScriptText* self);
GAME_FN(void, __stdcall, HUDClothing_FreezePeds, 0x0053CCF0, int freeze);

inline void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const kSchedule = reinterpret_cast<void*>(0x020C35E8);
inline void* const kShop = reinterpret_cast<void*>(0x00C18748);
inline void* const kObj20C5F70 = reinterpret_cast<void*>(0x020C5F70);
GAME_VAR(void*, g_barberMenu, 0x00CEFC34);
GAME_VAR(void*, g_clothingMenu, 0x00CEFC9C);
GAME_VAR(void*, g_hudText, 0x00C67548);             // the Text component
GAME_VAR(uint8_t, g_clockPaused, 0x00BCEB42);
GAME_VAR(uint8_t, g_pedsFrozen, 0x00C2B108);
GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);

template <typename R, typename... A>
R VCall(HUDComponent* self, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(HUDComponent*, re::Arg<A>...)>(static_cast<void*>(self->vtable[slot]))(self, args...);
}

float TimerSeconds() {
    return float(double(g_timeInMilliseconds) / 1000.0);
}

// ---- the barber ------------------------------------------------------------------------

// 0x53A4D0 (vf11 OnShow)
void __fastcall HUDBarber_OnShow(HUDComponent*, re::Edx) {
    Pad_StopRumble();
    MissionMgr_SetInMenu(kMissionMgr, 1);
    PauseMenu_Create();
    BarberMenu_Open(g_barberMenu);
    CShop_ClearFlash(kShop);
}

// 0x53A500 (vf12 OnHide)
void __fastcall HUDBarber_OnHide(HUDComponent*, re::Edx) {
    BarberMenu_Close(g_barberMenu);
    Fn_6CA7B0(kObj20C5F70);
    PauseMenu_Finish();
    MissionMgr_SetInMenu(kMissionMgr, 0);
}

// 0x53A530
void* __cdecl HUDBarber_GetMenu() {
    return g_barberMenu;
}

// 0x53A540 (vf8 Draw)
void __fastcall HUDBarber_Draw(HUDComponent*, re::Edx) {
    BarberMenu_Draw(g_barberMenu);
}

// 0x53A580: the menu replaced (the old one destroyed)
void __cdecl HUDBarber_SetMenu(void* menu) {
    void* old = g_barberMenu;
    if (old) {
        Menu_Destruct(old);
        Free(old);
    }
    g_barberMenu = menu;
}

// ---- Big_Text_For_Script ----------------------------------------------------------------

// 0x53A5C0: up and not run out
uint8_t __fastcall HUDScriptText_IsShowing(HUDScriptText* self, re::Edx) {
    if (VCall<uint8_t>(self, 4) && TimerSeconds() <= self->endTime) {
        return 1;
    }
    return 0;
}

// 0x53A6D0 (vf8 Draw): through the Text component's slot 1 (int params as floats);
// run out: hidden and cleared
void __fastcall HUDBigScriptText_Draw(HUDScriptText* self, re::Edx) {
    if (self->endTime < TimerSeconds()) {
        VCall<void>(self, 5, 0);
        HUDScriptText_Clear(self);
        return;
    }
    void* text = g_hudText;
    for (int i = 0; i < self->numParams; i++) {
        HUDTextParam& p = self->params[i];
        switch (p.type) {
        case 1: {
            int32_t v;
            std::memcpy(&v, p.value, 4);
            HUDText_AddFloat(text, 1, float(v));
            break;
        }
        case 2: {
            float v;
            std::memcpy(&v, p.value, 4);
            HUDText_AddFloat(text, 1, v);
            break;
        }
        case 3:
            HUDText_AddString(text, 1, p.value);
            break;
        case 4:
            HUDText_AddText(text, 1, p.value);
            break;
        }
    }
    HUDText_SetText(text, 1, self->text);
}

// 0x53A7C0: a string param
void __fastcall HUDScriptText_AddString(HUDScriptText* self, re::Edx, const char* string) {
    if (self->shown) {
        HUDScriptText_Clear(self);
    }
    if (self->numParams < 3) {
        self->params[self->numParams].type = 3;
        std::strncpy(reinterpret_cast<char*>(self->params[self->numParams].value), string, 0x7F);
        self->numParams++;
    }
}

// ---- the clock warnings -------------------------------------------------------------------

// 0x53C970 (vf4 IsVisible): (suppressed: whether it's visible; otherwise shown unless
// the mission manager says no or the clock is paused)
uint8_t __fastcall HUDClockWarnings_IsVisible(HUDClockWarnings* self, re::Edx) {
    if (self->suppressed) {
        return self->visible;
    }
    if (!MissionMgr_Fn6AA7E0(kMissionMgr) && !MissionMgr_Fn6AA8F0(kMissionMgr) && !g_clockPaused) {
        return 1;
    }
    return 0;
}

// 0x53CBE0 (vf8 Draw): the warning when a class is on and its time has come
void __fastcall HUDClockWarnings_Draw(HUDClockWarnings*, re::Edx) {
    int current = Schedule_GetCurrent(kSchedule);
    if (current < 0 || !Schedule_Test(kSchedule, current, 0, -1) || Schedule_GetType(kSchedule, current) != 1) {
        return;
    }
    int a = Schedule_Fn6A8D30(kSchedule, current);
    int b = Schedule_Fn6A8D70(kSchedule, current);
    if (b <= a) {
        MissionMgr_ClassWarning(kMissionMgr);
    }
}

// ---- the clothing shop -----------------------------------------------------------------------

// 0x53CC60 (vf8 Draw)
void __fastcall HUDClothing_Draw(HUDComponent*, re::Edx) {
    ClothingMenu_Draw(g_clothingMenu);
}

// 0x53CC70
void* __cdecl HUDClothing_GetMenu() {
    return g_clothingMenu;
}

// 0x53CCB0
void __cdecl HUDClothing_SetMenu(void* menu) {
    void* old = g_clothingMenu;
    if (old) {
        Menu_Destruct(old);
        Free(old);
    }
    g_clothingMenu = menu;
}

// 0x53CCF0: every ped but type 13's frozen (+0x48 cleared, +0xD0 set) or let go.
// (thiscall in its callers, ecx unused: stdcall is the same)
void __stdcall HUDClothing_FreezePeds_(int freeze) {
    uint8_t f = uint8_t(freeze);
    auto* pool = re::PtrCast<CPool<uint8_t>*>(CPools::ms_pPedPool);
    g_pedsFrozen = f;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot;
        if (!ped || *reinterpret_cast<int32_t*>(ped + 0x1310) == 0xD) {
            continue;
        }
        *reinterpret_cast<int32_t*>(ped + 0x48) = f == 0;
        *reinterpret_cast<int32_t*>(ped + 0xD0) = f;
    }
}

// 0x53CD50 (vf11 OnShow)
void __fastcall HUDClothing_OnShow(HUDComponent*, re::Edx) {
    Pad_StopRumble();
    MissionMgr_SetInMenu(kMissionMgr, 1);
    HUDClothing_FreezePeds(1);
    PauseMenu_Create();
    ClothingMenu_Open(g_clothingMenu);
    CShop_ClearFlash(kShop);
}

// 0x53CD90 (vf12 OnHide)
void __fastcall HUDClothing_OnHide(HUDComponent*, re::Edx) {
    HUDClothing_FreezePeds(0);
    ClothingMenu_Close(g_clothingMenu);
    Fn_6CA7B0(kObj20C5F70);
    PauseMenu_Finish();
    MissionMgr_SetInMenu(kMissionMgr, 0);
}

} // namespace

#define HUDKINDS_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(hudkinds_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudkinds_original_, __LINE__), \
             re::Mode::Original)

HUDKINDS_REVERSED("HUDBarber::OnShow", 0x0053A4D0, &HUDBarber_OnShow);
HUDKINDS_REVERSED("HUDBarber::OnHide", 0x0053A500, &HUDBarber_OnHide);
HUDKINDS_REVERSED("HUDBarber::GetMenu", 0x0053A530, &HUDBarber_GetMenu);
HUDKINDS_REVERSED("HUDBarber::Draw", 0x0053A540, &HUDBarber_Draw);
HUDKINDS_REVERSED("HUDBarber::SetMenu", 0x0053A580, &HUDBarber_SetMenu);
HUDKINDS_REVERSED("HUDBigScriptText::IsShowing", 0x0053A5C0, &HUDScriptText_IsShowing);
HUDKINDS_REVERSED("HUDBigScriptText::Draw", 0x0053A6D0, &HUDBigScriptText_Draw);
HUDKINDS_REVERSED("HUDBigScriptText::AddString", 0x0053A7C0, &HUDScriptText_AddString);
HUDKINDS_REVERSED("HUDClockWarnings::IsVisible", 0x0053C970, &HUDClockWarnings_IsVisible);
HUDKINDS_REVERSED("HUDClockWarnings::Draw", 0x0053CBE0, &HUDClockWarnings_Draw);
HUDKINDS_REVERSED("HUDClothing::Draw", 0x0053CC60, &HUDClothing_Draw);
HUDKINDS_REVERSED("HUDClothing::GetMenu", 0x0053CC70, &HUDClothing_GetMenu);
HUDKINDS_REVERSED("HUDClothing::SetMenu", 0x0053CCB0, &HUDClothing_SetMenu);
HUDKINDS_REVERSED("HUDClothing::FreezePeds", 0x0053CCF0, &HUDClothing_FreezePeds_);
HUDKINDS_REVERSED("HUDClothing::OnShow", 0x0053CD50, &HUDClothing_OnShow);
HUDKINDS_REVERSED("HUDClothing::OnHide", 0x0053CD90, &HUDClothing_OnHide);
