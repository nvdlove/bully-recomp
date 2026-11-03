// HUDWeapons (the weapon selector, 0x55C5D0..0x55DEA0): the ring round the current
// weapon's icon (the ammo left as a filled arc, a tick per round), cycling through the
// weapons with the pad (the icons' ring of 5 slots turning), the aiming overlays (the
// scope's mask, the camera's viewfinder), and the setup/reset. (The constructor and
// the destructor are Hud.cpp's; 0x55BA70..0x55C340 are the neighbouring pass's.)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Hud.h"
#include "reversed.h"

namespace {

struct CVector2D {
    float x, y;
};

// HUDWeapons, with what's understood here (Hud.h has it as unknowns)
struct Weapons : HUDComponent {
    int32_t weapons[19];      // +0x20 the ones the selector cycles through (-1: empty hands)
    uint32_t numWeapons;      // +0x6C
    int32_t items[19];        // +0x70 the other kind (the weapon info's +0x64 == 2)
    uint32_t numItems;        // +0xBC
    int32_t selected;         // +0xC0 the weapon picked (-1: none)
    int32_t index;            // +0xC4 its index in weapons (-1: none)
    int32_t numUsable;        // +0xC8 (the selector only turns with more than one)
    uint8_t usable[0x28];     // +0xCC by index into weapons
    struct Slot {             // +0xF4 the icons shown round the selector, 5 of them
        int32_t index;        // +0 into weapons
        float unknown4;       // +4
    } slots[5];
    float angle;              // +0x11C the ring's turn (3/4 pi when reset; +-0.471 a step)
    float ringRadius;         // +0x120 (130 while the wheel is up)
    float unknown124;         // +0x124
    float photoZoom;          // +0x128 (passed to the camera's viewfinder; 1 when set up)
    uint8_t initialised;      // +0x12C
    uint8_t unknown12D;       // +0x12D
    uint8_t unknown12E[2];
    float x130, y130;         // +0x130 (from the layout)
    int32_t overlay;          // +0x138 1: the scope's mask, 2: the camera's viewfinder
    uint8_t hidden;           // +0x13C only the ring is drawn
    uint8_t selecting;        // +0x13D the player is cycling through the weapons
    uint8_t unknown13E[2];
    uint32_t unequipTime;     // +0x140
    uint8_t pendingSelect;    // +0x144 the choice is applied at selectTime
    uint8_t cycled;           // +0x145
    uint8_t unknown146[2];
    uint32_t ringTime;        // +0x148 the ring is shown until this time (it pops for 250 ms before)
    uint32_t selectTime;      // +0x14C
    uint32_t unknown150;      // +0x150
    int32_t toggleWeapon;     // +0x154 the weapon when the toggle buttons went down
    uint32_t toggleTime;      // +0x158 until when a second button makes it a toggle
    uint8_t unknown15C;       // +0x15C
    uint8_t unknown15D;       // +0x15D
    uint8_t unknown15E[2];
    float fadeTarget;        // +0x160 (HudTasksWeapons.cpp: the fade)
    uint32_t fadeEnd;         // +0x164
    float fadeSeconds;       // +0x168
    float fadeFrom;          // +0x16C
    uint32_t hideTime;        // +0x170
    float x174, y174;         // +0x174 (the layout's, placed)
    float w17C, h17C;         // +0x17C (h scaled)
    float w184, h184;         // +0x184 (h scaled)
    float x18C, y18C;         // +0x18C (as they are)
    uint8_t unknown194[8];
};
static_assert(offsetof(Weapons, numWeapons) == 0x6C);
static_assert(offsetof(Weapons, numItems) == 0xBC);
static_assert(offsetof(Weapons, usable) == 0xCC);
static_assert(offsetof(Weapons, slots) == 0xF4);
static_assert(offsetof(Weapons, angle) == 0x11C);
static_assert(offsetof(Weapons, photoZoom) == 0x128);
static_assert(offsetof(Weapons, overlay) == 0x138);
static_assert(offsetof(Weapons, ringTime) == 0x148);
static_assert(offsetof(Weapons, toggleTime) == 0x158);
static_assert(offsetof(Weapons, hideTime) == 0x170);
static_assert(offsetof(Weapons, x18C) == 0x18C);
static_assert(sizeof(Weapons) == sizeof(HUDWeapons));

// ---- the game's ----
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawSprite, 0x0052D610, const col::CRect* rect, void* texture, const CRGBA* colour,
        const CVector2D* uv0, const CVector2D* uv1);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const col::CRect* rect, const CRGBA* colour);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const CRGBA* colour, const CVector2D* uv0, const CVector2D* uv1);
// a ring (part of one) from angle `from` to `to`, in `segments` pieces
GAME_FN(void, __cdecl, DrawRing, 0x0052D390, const CVector2D* centre, float inner, float outer, const CRGBA* colour,
        int segments, float from, float to);
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void, __thiscall, HUDComponent_SetAlignment, 0x0053CE00, HUDComponent* self, const char* h, const char* v);
GAME_FN(void, __thiscall, HUDComponent_Reset, 0x0053CF00, HUDComponent* self);
GAME_FN(void, __thiscall, HUDPhoto_DrawViewfinder, 0x00545450, void* photo, float zoom);
GAME_FN(void*, __cdecl, GetItemIcon, 0x00541210, int item);
GAME_FN(uint8_t*, __cdecl, GetWeaponInfo, 0x0051B450, int weapon);  // (+0x34 the most rounds, +0x64 the kind, +0x84 its ammo's)
GAME_FN(void, __thiscall, WeaponInfo_Request, 0x007282A0, uint8_t* info);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);  // (+4 the load state)
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(int, __thiscall, Inventory_GetCount, 0x0045CF40, void* inventory, int weapon);
GAME_FN(float, __thiscall, PlayerPed_Get49AD40, 0x0049AD40, void* ped);
GAME_FN(uint8_t, __thiscall, UserInput_50DF50, 0x0050DF50, void* input);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int control, int pad);
GAME_FN(void, __thiscall, FrontendSound_Play, 0x0059C150, void* sounds, int sound);
GAME_FN(void*, __cdecl, FindPlayerVehicle_446970, 0x00446970, uintptr_t);
GAME_FN(int*, __thiscall, Entity_GetType_4CAF30, 0x004CAF30, void* entity, int* out);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, uint32_t* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, uint32_t value);
// HUDWeapons' own (HudTasksWeapons.cpp's, and these, where modes apply)
GAME_FN(uint8_t, __thiscall, Weapons_CanSelect, 0x0055BA70, Weapons* self, void* ped, int weapon);
GAME_FN(void, __thiscall, Weapons_SetPlayerWeapon, 0x0055BC00, Weapons* self, int weapon);
GAME_FN(void, __thiscall, Weapons_SetFade, 0x0055BC90, Weapons* self, int);
GAME_FN(void, __thiscall, Weapons_UpdateFade, 0x0055BE00, Weapons* self);
GAME_FN(void, __thiscall, Weapons_DrawWheel, 0x0055BE90, Weapons* self);
GAME_FN(void, __thiscall, Weapons_Refresh, 0x0055C340, Weapons* self);
GAME_FN(void, __thiscall, Weapons_Select, 0x0055C840, Weapons* self, int weapon);
GAME_FN(void, __thiscall, Weapons_ApplySelection, 0x0055C930, Weapons* self);
GAME_FN(void, __thiscall, Weapons_Add, 0x0055C9B0, Weapons* self, int weapon, int kind);
GAME_FN(void, __thiscall, Weapons_DrawRing, 0x0055CDE0, Weapons* self);
GAME_FN(void, __thiscall, Weapons_HandleInput, 0x0055D8E0, Weapons* self);

// ---- game data ----
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);         // +0x1C4 the inventory, +0x1D0 the weapon (+0x10E the model)
GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(float, g_hudAlpha, 0x00A64880);             // the HUD's fade
GAME_VAR(HUDComponent*, g_hudC674B0, 0x00C674B0);    // (while its +0x15 is set, only a selection is drawn)
GAME_VAR(void*, g_hudPhoto, 0x00C674E0);
GAME_VAR(void*, g_scopeMask, 0x00C66F74);            // textures
GAME_VAR(void*, g_ringBack, 0x00C66FD4);
GAME_VAR(void*, g_emptyHandsIcon, 0x00C66F68);
GAME_VAR(CRGBA, g_iconColour, 0x00CF0A58);
GAME_VAR(CRGBA, g_iconColourDim, 0x00CF0A5C);
GAME_VAR(CRGBA, g_backColour, 0x00CF0A60);
GAME_VAR(CRGBA, g_backColourDim, 0x00CF0A64);
GAME_VAR(float, g_pulsePhase, 0x00CF0A44);
GAME_VAR(int32_t, g_modelUnequip, 0x00BF432C);
GAME_VAR(int32_t, g_model_BF4330, 0x00BF4330);       // what the toggle puts away / takes out
GAME_VAR(int32_t, g_model_BF444C, 0x00BF444C);       // (two weapons whose ring shows a level, not rounds)
GAME_VAR(int32_t, g_model_BF44D8, 0x00BF44D8);
GAME_VAR(int32_t, g_firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, g_lastWeapon, 0x00A136B4);
GAME_VAR(uint8_t, g_lockButtons, 0x00C1AF8C);        // only the buttons whose flag is set work
GAME_VAR(uint8_t, g_allowButton11, 0x00C1AF98);
GAME_VAR(uint8_t, g_allowButton13, 0x00C1AF9A);
GAME_VAR(uint8_t, g_allowButton24, 0x00C1AFA5);
inline void* const kUserInput = reinterpret_cast<void*>(0x00A45A30);
inline void* const kFrontendSounds = reinterpret_cast<void*>(0x00CF3DA0);

constexpr double kPi = 3.1415927410125732;          // (floats' pi, as the game's doubles)
constexpr double kMinusTwoPi = -6.2831854820251465;

uint8_t* PlayerWeapon(uint8_t* ped) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x1D0);
}
int16_t CurrentWeapon(uint8_t* ped) {
    return *reinterpret_cast<int16_t*>(PlayerWeapon(ped) + 0x10E);
}
void* Inventory(uint8_t* ped) {
    return *reinterpret_cast<re::Ptr32<void>*>(ped + 0x1C4);
}
int32_t& RequestedWeapon(uint8_t* ped) {
    return *reinterpret_cast<int32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x2E0) + 0x18);
}

// an alpha times the component's and the HUD's fade (the game's x87: exact, cut off)
uint8_t Faded(const Weapons* self, uint32_t alpha) {
    float scale = self->unknown1C * g_hudAlpha;
    return uint8_t(int(double(scale) * double(alpha)));
}

// the timer as a float, as the game converts it (unsigned)
float TimerFloat(uint32_t t) {
    return float(double(t));
}

// ---- 0x55C5D0 (thiscall): the aiming overlay: the scope's mask (a 480x480 texture in
// four mirrored quarters in the middle of the screen, black bars at the sides) or the
// camera's viewfinder
void __fastcall Weapons_DrawOverlay(Weapons* self, re::Edx) {
    if (self->overlay == 1) {
        int w = int(double(g_screenScaleX) * 480.0);
        int h = int(double(g_screenScaleY) * 480.0);
        void* texture = g_scopeMask;
        float wf = float(w);
        float x0 = float((640.0 - double(wf)) * 0.5);
        float hf = float(h);
        float y0 = float((480.0 - double(hf)) * 0.5);
        float xMid = float(double(x0) + double(wf) * 0.5);
        float xRight = float(double(x0) + double(wf));
        float yMid = float(double(y0) + double(hf) * 0.5);
        float yBottom = float(double(y0) + double(hf));
        col::CRect quarters[4] = {
            {xMid, yMid, x0, y0},
            {xMid, yMid, x0, yBottom},
            {xMid, yMid, xRight, y0},
            {xMid, yMid, xRight, yBottom},
        };
        CVector2D uv0 = {0.0f, 0.0f};
        CVector2D uv1 = {1.0f, 1.0f};
        CRGBA white;
        RGBA_Construct(&white, 0xFF, 0xFF, 0xFF, 0xFF);
        for (const col::CRect& quarter : quarters) {
            DrawSprite(&quarter, texture, &white, &uv0, &uv1);
        }
        col::CRect left = {0.0f, 480.0f, x0, 0.0f};
        col::CRect right = {xRight, 480.0f, 640.0f, 0.0f};
        CRGBA black;
        RGBA_Construct(&black, 0, 0, 0, 0x6B);
        DrawRect(&left, &black);
        DrawRect(&right, &black);
    }
    if (self->overlay == 2) {
        HUDPhoto_DrawViewfinder(g_hudPhoto, self->photoZoom);
    }
}

// ---- 0x55C840 (thiscall): picks a weapon (dropped when the player has none of it,
// unless it's the empty hands) and finds its index among the usable ones
void __fastcall Weapons_Select_(Weapons* self, re::Edx, int weapon) {
    self->selected = weapon;
    if (g_playerPed) {
        if (Inventory_GetCount(Inventory(g_playerPed), weapon) == 0 && self->selected != g_modelUnequip) {
            self->selected = -1;
        }
    }
    self->index = -1;
    for (uint32_t i = 0; i < self->numWeapons; i++) {
        int w = self->weapons[i];
        if (Weapons_CanSelect(self, g_playerPed, w) && w == self->selected) {
            self->index = int32_t(i);
            return;
        }
    }
}

// ---- 0x55C8D0 (thiscall): puts the weapon away (for 4 seconds on the timer)
void __fastcall Weapons_Unequip(Weapons* self, re::Edx) {
    self->selected = -1;
    uint8_t* ped = g_playerPed;
    Inventory_GetCount(Inventory(ped), -1);
    if (CurrentWeapon(ped) != -1) {
        RequestedWeapon(ped) = -1;
    }
    Weapons_Select(self, -1);
    self->unequipTime = g_timeInMilliseconds + 4000;
}

// ---- 0x55C930 (thiscall): asks the player for the selected weapon (-1 when he has
// none of it)
void __fastcall Weapons_ApplySelection_(Weapons* self, re::Edx) {
    if (g_playerPed) {
        if (Inventory_GetCount(Inventory(g_playerPed), self->selected) == 0 && self->selected != g_modelUnequip) {
            self->selected = -1;
        }
    }
    int weapon = self->selected;
    uint8_t* ped = g_playerPed;
    if (Inventory_GetCount(Inventory(ped), weapon) == 0) {
        weapon = -1;
    }
    if (CurrentWeapon(ped) != weapon) {
        RequestedWeapon(ped) = weapon;
    }
}

// ---- 0x55C9B0 (thiscall): adds a weapon to its list (kind 1: the selector's, its model
// requested; 2: the others), once
void __fastcall Weapons_Add_(Weapons* self, re::Edx, int weapon, int kind) {
    if (kind == 1) {
        int count = int32_t(self->numWeapons);
        for (int i = 0; i < count; i++) {
            if (self->weapons[i] == weapon) {
                return;
            }
        }
        self->weapons[count] = weapon;
        self->numWeapons++;
        if (weapon != -1 && *reinterpret_cast<int32_t*>(GetWeaponInfo(weapon) + 0x64) != 0 &&
            GetStreamingInfo(weapon)[4] != 1) {
            RequestModel(weapon, 1);
        }
    } else if (kind == 2) {
        int count = int32_t(self->numItems);
        for (int i = 0; i < count; i++) {
            if (self->items[i] == weapon) {
                return;
            }
        }
        self->items[count] = weapon;
        self->numItems++;
    }
}

// ---- 0x55CA60 (thiscall): stops cycling (applying the choice when asked)
void __fastcall Weapons_StopSelecting(Weapons* self, re::Edx, int apply) {
    if (self->selecting) {
        if (re::Flag(apply)) {
            Weapons_Select(self, self->selected);
            Weapons_ApplySelection(self);
        }
        self->selecting = 0;
    }
}

// ---- 0x55CAA0 (vf1 Setup)
void __fastcall Weapons_Setup(Weapons* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    auto S = [d](int offset) { return *reinterpret_cast<const int16_t*>(d + offset); };
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x22), reinterpret_cast<const char*>(d + 0x2A));
    CVector2D p;
    CVector2D* r = ScreenPosition(&p, float(S(8)), float(S(0xA)), self->hAlign, self->vAlign);
    self->x174 = r->x;
    self->y174 = r->y;
    self->w17C = float(S(0xC));
    self->h17C = float(S(0xE));
    self->h17C = g_screenScaleY * self->h17C;
    r = ScreenPosition(&p, float(S(0x10)), float(S(0x12)), self->hAlign, self->vAlign);
    self->x130 = r->x;
    self->y130 = r->y;
    self->w184 = float(S(0x14));
    self->h184 = float(S(0x16));
    self->h184 = g_screenScaleY * self->h184;
    self->x18C = float(S(0x18));
    self->y18C = float(S(0x1A));
}

// ---- 0x55CC50 (vf7 Reset)
void __fastcall Weapons_Reset(Weapons* self, re::Edx) {
    HUDComponent_Reset(self);
    reinterpret_cast<void(__thiscall*)(HUDComponent*, int)>(static_cast<void*>(self->vtable[5]))(self, 1);  // SetVisible
    Weapons_Select(self, -1);
    self->angle = 2.356194496154785f;  // (3/4 pi)
    self->ringRadius = 0.0f;
    self->overlay = 0;
    self->unknown124 = 1.0f;
    self->unequipTime = 0;
    self->fadeTarget = 1.0f;
    self->ringTime = 0;
    self->selectTime = 0;
    self->fadeSeconds = 0.0f;
    self->pendingSelect = 0;
    self->selecting = 0;
    self->cycled = 0;
    self->index = 0;
    self->fadeFrom = 1.0f;
    self->selected = -1;
    self->numUsable = 0;
    self->toggleWeapon = -1;
    self->toggleTime = 0;
    self->unknown15D = 0;
    self->unknown15C = 1;
    self->hidden = 0;
    self->fadeEnd = 0;
    Weapons_SetFade(self, 0);
}

// ---- 0x55CD20 (thiscall): the lists filled once (the empty hands, then every weapon
// with a kind)
void __fastcall Weapons_Init(Weapons* self, re::Edx) {
    if (self->initialised) {
        self->photoZoom = 1.0f;
        return;
    }
    self->initialised = 1;
    self->ringTime = 0;
    self->selectTime = 0;
    self->pendingSelect = 0;
    Weapons_Select(self, -1);
    int count = int32_t(self->numWeapons);
    int i = 0;
    for (; i < count; i++) {
        if (self->weapons[i] == -1) {
            break;
        }
    }
    if (i >= count) {
        self->weapons[count] = -1;
        self->numWeapons++;
    }
    for (int w = g_firstWeapon; w <= g_lastWeapon; w++) {
        if (*reinterpret_cast<int32_t*>(GetWeaponInfo(w) + 0x64) != 0) {
            Weapons_Add(self, w, *reinterpret_cast<int32_t*>(GetWeaponInfo(w) + 0x64));
        }
    }
    self->photoZoom = 1.0f;
}

// ---- 0x55CDE0 (thiscall): the ring round the weapon's icon: its back, the rounds
// left as a filled arc (or the level for the two weapons that have one), a tick per
// round, the icon on its back while the selector is up. Out of ammo: puts it away.
void __fastcall Weapons_DrawRing_(Weapons* self, re::Edx) {
    if (g_hudC674B0->suppressed && !self->selecting) {
        return;
    }
    uint8_t* ped = g_playerPed;
    uint8_t* pedWeapon = PlayerWeapon(ped);
    int weapon = *reinterpret_cast<int16_t*>(pedWeapon + 0x10E);
    CRGBA back = g_backColour;
    CRGBA icon = g_iconColour;
    uint8_t bright = 1;
    uint8_t found = 0;
    if (weapon == -1 && self->selected != -1 && !self->selecting) {
        weapon = self->selected;
        icon = g_iconColourDim;
        back = g_backColourDim;
        bright = 0;
    }
    for (uint32_t i = 0; i < self->numWeapons; i++) {
        if (weapon == self->weapons[i]) {
            found = 1;
        }
    }
    for (uint32_t i = 0; i < self->numItems; i++) {
        if (weapon == self->items[i]) {
            found = 1;
        }
    }
    if (!found) {
        weapon = self->selected;
        icon = g_iconColourDim;
        back = g_backColourDim;
        bright = 0;
    }
    if (weapon == 0) {
        return;
    }

    float sx = g_screenScaleX;
    float sy = g_screenScaleY;
    float s = float(std::sin(2.356194496154785));
    double X = double(s) * 130.0 * double(sx) + 45.0 * double(sx);
    float xPos = float(X - double(sx) * 32.0);
    float c = float(std::cos(2.356194496154785));
    uint32_t now = g_timeInMilliseconds;
    double Y = double(c) * 130.0 * double(sy) + 438.0;
    float yPos = float(Y - double(sy) * 32.0);
    float fade = self->unknown1C * g_hudAlpha;
    icon.a = uint8_t(int(double(icon.a) * double(fade)));
    back.a = uint8_t(int(double(fade) * double(back.a)));

    int count;      // rounds left
    int max;        // the most
    float frac;     // the arc filled
    int ammo = weapon;
    if (weapon == -1) {
        max = 1;
        count = 1;
        goto noRounds;
    }
    if (GetWeaponInfo(weapon)) {
        uint8_t* info = GetWeaponInfo(weapon);
        WeaponInfo_Request(info);
        ammo = *reinterpret_cast<int32_t*>(info + 0x84);
    }
    if (ammo <= 0) {
        ammo = weapon;
    }
    max = GetWeaponInfo(ammo) ? *reinterpret_cast<int32_t*>(GetWeaponInfo(ammo) + 0x34) : 1;
    if (max == 1 || max == 0xFF || max == -1) {
        count = 1;
        max = 1;
        goto noRounds;
    }
    count = Inventory_GetCount(Inventory(ped), ammo);
    frac = float(double(count) / double(max));
    goto haveFrac;
noRounds:
    frac = 1.0f;
    if (weapon != g_modelUnequip && weapon != g_model_BF444C && weapon != g_model_BF44D8) {
        goto icon;
    }
haveFrac:
    if (UserInput_50DF50(kUserInput) && (Pad_GetValue(0xC, 0) != 0.0f || Pad_GetValue(0xA, 0) != 0.0f)) {
        Weapons_SetFade(self, 0);
    }
    if (weapon == g_modelUnequip) {
        frac = PlayerPed_Get49AD40(ped);
    } else if (weapon == g_model_BF444C || weapon == g_model_BF44D8) {
        float v = *reinterpret_cast<float*>(pedWeapon + 0x128);
        frac = v < 0.0f ? 0.0f : (1.0f < v ? 1.0f : v);
    }
    {
        float nowF = TimerFloat(now);
        if (double(self->ringTime) - 250.0 > double(nowF)) {
            goto icon;
        }
        CRGBA ringBack, tickColour, emptyColour, fullColour, fullColour1;
        RGBA_Construct(&ringBack, 0x10, 0x10, 0x10, 0x80);
        RGBA_Construct(&tickColour, 0x10, 0x10, 0x10, 0x80);
        RGBA_Construct(&emptyColour, 0x32, 0x32, 0x32, 0x80);
        RGBA_Construct(&fullColour, 0xC0, 0xC0, 0x64, 0x80);
        RGBA_Construct(&fullColour1, 0xC0, 0xC0, 0x64, 0x80);
        CVector2D centre = {float(X), float(Y)};
        uint32_t ringTime = self->ringTime;
        // the ring pops out in the 250 ms before ringTime
        float radius = 35.0f;
        if (now < ringTime) {
            radius = 31.0f;
            if (double(nowF) > double(ringTime) - 250.0) {
                float t = float(1.0 - double(ringTime - now) / 250.0);
                radius = float(double(t) * double(t) * 4.0 + 31.0);
            }
        }
        CRGBA colour = ringBack;
        colour.a = Faded(self, bright ? ringBack.a : uint32_t(ringBack.a >> 1));
        DrawRing(&centre, 29.0f, float(double(radius) + 2.0), &colour, 0x40, 3.1415927f, -3.1415927f);
        if (frac < 1.0f) {
            colour = emptyColour;
            colour.a = Faded(self, bright ? emptyColour.a : uint32_t(emptyColour.a >> 1));
            DrawRing(&centre, 31.0f, radius, &colour, 0x40, float(double(frac) * kMinusTwoPi + kPi), -3.1415927f);
        }
        if (0.0f < frac) {
            colour = fullColour;
            if (count == 1 && max == 1 && frac == 1.0f) {
                colour = fullColour1;
            }
            uint8_t a = colour.a;
            if (!bright) {
                a >>= 1;
            }
            colour.a = Faded(self, a);
            if (weapon == g_model_BF444C || weapon == g_model_BF44D8) {
                CRGBA red;
                RGBA_Construct(&red, 200, 0x20, 0x20, 0x80);
                colour = red;
                if (frac < 1.0f) {
                    g_pulsePhase = 0.0f;
                } else {
                    // full: pulsing
                    float step = float(double(g_timeStep) * 0.20000000298023224);
                    g_pulsePhase = step + g_pulsePhase;
                    float pulse = float(std::sin(double(g_pulsePhase)));
                    float v = float(double(pulse) * 64.0 + 128.0);
                    float scale = self->unknown1C * g_hudAlpha;
                    colour.a = uint8_t(int(double(v) * double(scale)));
                }
            }
            DrawRing(&centre, 31.0f, radius, &colour, 0x40, 3.1415927f, float(double(frac) * kMinusTwoPi + kPi));
        }
        float step = float(kMinusTwoPi / double(max));
        if (count != 1 || max != 1) {
            colour = tickColour;
            colour.a = Faded(self, bright ? tickColour.a : uint32_t(tickColour.a >> 1));
            for (int i = 0; i < max; i++) {
                double a = double(i) * double(step) + kPi;
                float to = float(a - 0.04363323375582695);
                float from = float(a + 0.04363323375582695);
                DrawRing(&centre, 31.0f, radius, &colour, 3, from, to);
            }
        }
    }
icon:
    if (now > self->ringTime && self->numUsable > 1) {
        CVector2D uv0 = {0.0f, 0.0f};
        CVector2D uv1 = {1.0f, 1.0f};
        float bottom = float(double(yPos) + double(sy) * 64.0);
        float right = float(double(xPos) + double(sx) * 64.0);
        DrawRectUV(xPos, yPos, right, bottom, g_ringBack, &back, &uv0, &uv1);
        uv1 = {1.0f, 1.0f};
        uv0 = {0.0f, 0.0f};
        void* texture = weapon == -1 ? static_cast<void*>(g_emptyHandsIcon) : GetItemIcon(weapon);
        DrawRectUV(xPos, yPos, right, bottom, texture, &icon, &uv0, &uv1);
    }
    if (count == 0) {
        uint8_t* player = g_playerPed;
        int current = CurrentWeapon(player);
        if (current != g_modelUnequip) {
            if (current != -1) {
                Inventory_GetCount(Inventory(player), -1);
                if (CurrentWeapon(player) != -1) {
                    RequestedWeapon(player) = -1;
                }
            }
            Weapons_Select(self, -1);
            self->index = 0;
        }
    }
}

// ---- 0x55D8E0 (thiscall): the pad: the two cycle buttons turn the selector (skipping
// what the player can't use), both at once (or the third) within 200 ms toggles the
// weapon away / back; the choice is applied when the selector closes
void __fastcall Weapons_HandleInput_(Weapons* self, re::Edx) {
    uint32_t now = g_timeInMilliseconds;
    uint8_t held11 = 0, held13 = 0, pressed11 = 0, pressed13 = 0, held24 = 0, pressed24 = 0;
    if (UserInput_50DF50(kUserInput)) {
        if (!g_lockButtons || g_allowButton11) {
            held11 = Pad_GetValue(0xB, 0) != 0.0f;
            pressed11 = Pad_JustPressed(0xB, 0);
        }
        if (!g_lockButtons || g_allowButton13) {
            held13 = Pad_GetValue(0xD, 0) != 0.0f;
            pressed13 = Pad_JustPressed(0xD, 0);
        }
        if (!g_lockButtons || g_allowButton24) {
            held24 = Pad_GetValue(0x18, 0) != 0.0f;
            pressed24 = Pad_JustPressed(0x18, 0);
        }
        if ((pressed11 || pressed13 || pressed24) && self->toggleTime < now) {
            self->toggleWeapon = CurrentWeapon(g_playerPed);
            self->toggleTime = uint32_t(int64_t(double(now) + 200.0));
        }
        if ((held24 || (held11 && held13)) && self->toggleTime >= now) {
            if (self->toggleWeapon == g_model_BF4330) {
                Weapons_SetPlayerWeapon(self, -1);
                Weapons_Select(self, -1);
                Weapons_SetFade(self, 0);
            } else if (*reinterpret_cast<int32_t*>(g_playerPed + 0x1554) == 0) {
                Weapons_SetPlayerWeapon(self, g_model_BF4330);
                Weapons_Select(self, g_model_BF4330);
                Weapons_SetFade(self, 0);
            }
            self->toggleTime = 0;
        } else if (pressed11) {
            if (!self->selecting) {
                Weapons_Refresh(self);
            }
            if (self->numUsable > 1) {
                FrontendSound_Play(kFrontendSounds, 0x16);
                do {
                    self->index++;
                    if (uint32_t(self->index) > self->numWeapons - 1) {
                        self->index = 0;
                    }
                } while (!Weapons_CanSelect(self, g_playerPed, self->weapons[self->index]));
                self->selected = self->weapons[self->index];
                // the icons' ring turns one on
                self->slots[0].index = self->slots[1].index;
                self->slots[1].index = self->slots[2].index;
                self->slots[2].index = self->slots[3].index;
                uint32_t i = uint32_t(self->slots[4].index);
                self->slots[3].index = int32_t(i);
                do {
                    i++;
                    if (i > self->numWeapons - 1) {
                        i = 0;
                    }
                } while (!self->usable[i]);
                self->angle = float(double(self->angle) + 0.4712389298772379);
                self->slots[4].index = int32_t(i);
                self->cycled = 1;
                self->selecting = 1;
                float nowF = TimerFloat(now);
                self->pendingSelect = 1;
                self->ringTime = uint32_t(int64_t(double(nowF) + 500.0));
                self->selectTime = uint32_t(int64_t(double(nowF) + 0.0));
                Weapons_SetFade(self, 0);
            }
        } else if (pressed13) {
            if (!self->selecting) {
                Weapons_Refresh(self);
            }
            if (self->numUsable > 1) {
                FrontendSound_Play(kFrontendSounds, 0x17);
                do {
                    if (--self->index < 0) {
                        self->index = int32_t(self->numWeapons - 1);
                    }
                } while (!Weapons_CanSelect(self, g_playerPed, self->weapons[self->index]));
                self->selected = self->weapons[self->index];
                // the icons' ring turns one back
                self->slots[4].index = self->slots[3].index;
                self->slots[3].index = self->slots[2].index;
                self->slots[2].index = self->slots[1].index;
                int32_t i = self->slots[0].index;
                self->slots[1].index = i;
                do {
                    if (--i < 0) {
                        i = int32_t(self->numWeapons - 1);
                    }
                } while (!self->usable[i]);
                self->slots[0].index = i;
                self->angle = float(double(self->angle) - 0.4712389298772379);
                self->cycled = 1;
                self->selecting = 1;
                float nowF = TimerFloat(now);
                self->pendingSelect = 1;
                self->ringTime = uint32_t(int64_t(double(nowF) + 500.0));
                self->selectTime = uint32_t(int64_t(double(nowF) + 0.0));
                Weapons_SetFade(self, 0);
            }
        }
    }
    if (now > self->selectTime && self->pendingSelect) {
        self->pendingSelect = 0;
        if (self->selecting && !self->cycled && CurrentWeapon(g_playerPed) != -1) {
            Weapons_SetPlayerWeapon(self, -1);
        } else {
            Weapons_Select(self, self->selected);
            Weapons_ApplySelection(self);
        }
    }
    if (now > self->ringTime) {
        self->selecting = 0;
        self->cycled = 0;
    }
}

// ---- 0x55DEA0 (vf8 Draw): not in some vehicles (types 6, 7, 10); the render states
// for the HUD's sprites around it
void __fastcall Weapons_Draw(Weapons* self, re::Edx) {
    if (!reinterpret_cast<uint8_t(__thiscall*)(HUDComponent*)>(static_cast<void*>(g_hudC674B0->vtable[4]))(g_hudC674B0)) {
        return;
    }
    void* vehicle = FindPlayerVehicle_446970(0);
    int type;
    if (vehicle && (*Entity_GetType_4CAF30(vehicle, &type) == 7 || *Entity_GetType_4CAF30(vehicle, &type) == 6 ||
                    *Entity_GetType_4CAF30(vehicle, &type) == 10)) {
        return;
    }
    uint32_t state8, state6, stateC;
    GetRenderState(8, &state8);
    GetRenderState(6, &state6);
    GetRenderState(0xC, &stateC);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    Weapons_UpdateFade(self);
    if (!self->hidden) {
        uint8_t* ped = g_playerPed;
        if (!ped[0x16CC]) {
            Weapons_HandleInput(self);
        }
        Weapons_DrawRing(self);
        if (self->selecting && !ped[0x16CC]) {
            Weapons_SetFade(self, 0);
            // (the game calls it with ecx as 0x55BC90 leaves it: the component)
            Weapons_DrawWheel(self);
        } else {
            self->selecting = 0;
        }
    } else {
        Weapons_DrawRing(self);
    }
    SetRenderState(8, state8);
    SetRenderState(6, state6);
    SetRenderState(0xC, stateC);
}

} // namespace

#define HUDWHEEL_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(hudwheel_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudwheel_original_, __LINE__), \
             re::Mode::Original)

HUDWHEEL_REVERSED("HUDWeapons::DrawOverlay", 0x0055C5D0, &Weapons_DrawOverlay);
HUDWHEEL_REVERSED("HUDWeapons::Select", 0x0055C840, &Weapons_Select_);
HUDWHEEL_REVERSED("HUDWeapons::Unequip", 0x0055C8D0, &Weapons_Unequip);
HUDWHEEL_REVERSED("HUDWeapons::ApplySelection", 0x0055C930, &Weapons_ApplySelection_);
HUDWHEEL_REVERSED("HUDWeapons::Add", 0x0055C9B0, &Weapons_Add_);
HUDWHEEL_REVERSED("HUDWeapons::StopSelecting", 0x0055CA60, &Weapons_StopSelecting);
HUDWHEEL_REVERSED("HUDWeapons::Setup", 0x0055CAA0, &Weapons_Setup);
HUDWHEEL_REVERSED("HUDWeapons::Reset", 0x0055CC50, &Weapons_Reset);
HUDWHEEL_REVERSED("HUDWeapons::Init", 0x0055CD20, &Weapons_Init);
HUDWHEEL_REVERSED("HUDWeapons::DrawRing", 0x0055CDE0, &Weapons_DrawRing_);
HUDWHEEL_REVERSED("HUDWeapons::HandleInput", 0x0055D8E0, &Weapons_HandleInput_);
HUDWHEEL_REVERSED("HUDWeapons::Draw", 0x0055DEA0, &Weapons_Draw);
