#include "game/Hud.h"

#include <cstring>
#include "reversed.h"

namespace {
// memory (operator new/delete)
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// Gamebryo: a refcounted object let go
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, void* object);
// A position in the 640x480 layout to the screen's: scaled, then from the left/centre/
// right (and top/centre/bottom) as the alignment says
struct CVector2D {
    float x, y;
};
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(col::CRect*, __cdecl, ScreenRect, 0x0052D650, col::CRect* out, float x, float y, float width, float height,
        int hAlign, int vAlign);
// the layout's alignment names ("LEFT", "CENTRE", ...) to 0/1/2
GAME_FN(int, __cdecl, HAlignFromName, 0x005DAF00, const char* name);
GAME_FN(int, __cdecl, VAlignFromName, 0x005DAF20, const char* name);
// the font's justification (0 left, 1 right, 2 centre, 3)
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
// (in other areas)
GAME_FN(HUDComponent*, __thiscall, HUDTimeCycle_Construct, 0x006B1770, HUDComponent* self);
GAME_FN(HUDComponent*, __thiscall, HUDDodgeBall_Construct, 0x0067DF10, HUDComponent* self);
GAME_FN(int, __cdecl, HUDMonitor_LookUp, 0x006A7110, const char* name);   // (via 0x53CE30)
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, TaggingShape_Construct, 0x005593A0, void* shape);
GAME_FN(void, __thiscall, ButtonHistory_List_Destruct, 0x005D9C50, void* list);
GAME_FN(void, __thiscall, HUDButtonHistory_53B2C0, 0x0053B2C0, HUDComponent* self);  // Init, below
GAME_FN(void, __thiscall, HUDRadar_Construct_Call, 0x0054B280, HUDComponent* self);

// HUDComponent's own, where modes apply
GAME_FN(HUDComponent*, __thiscall, HUDComponent_Construct, 0x0053CDC0, HUDComponent* self);
GAME_FN(void, __thiscall, HUDComponent_Destruct, 0x0053CDF0, HUDComponent* self);
GAME_FN(void, __thiscall, HUDComponent_Reset, 0x0053CF00, HUDComponent* self);
GAME_FN(void, __thiscall, HUDComponent_SetVisible, 0x0053CEB0, HUDComponent* self, int visible);
GAME_FN(void, __thiscall, HUDComponent_SetAlignment, 0x0053CE00, HUDComponent* self, const char* h, const char* v);

// ---- game data ----
GAME_VAR(int32_t, g_language, 0x00C18724);             // 7: the one with bigger text (Japanese?)
GAME_VAR(float, g_fontScale, 0x00BA4E10);
GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);  // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint8_t, g_useOtherTimer, 0x020C43FD);        // (the flashing then goes by 0xA14A50)
GAME_VAR(uint32_t, g_otherTimer, 0x00A14A50);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);           // the screen over 640x480
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_VAR(float, g_fontWrapLeft, 0x0208B774);           // the font's wrap/centre limits
GAME_VAR(float, g_fontWrapCentre, 0x0208B778);
GAME_VAR(float, g_fontWrapRight, 0x0208B77C);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);           // (its money at +0x1CA0)
GAME_VAR(CRGBA, g_moneyColour, 0x00CF01A8);            // (3 bytes read, alpha 0xFF)
GAME_VAR(CRGBA, g_counterColour, 0x00CEFCB4);
GAME_ARRAY(uint32_t, g_socialDefaults, 0x00C674FC);    // [14]
GAME_ARRAY(const char* const, g_hudComponentTypeNames, 0x00919460);  // [48]
// IsType's static id
GAME_VAR(uint32_t, g_isTypeGuard, 0x00CEFC2C);
GAME_VAR(int16_t, g_isTypeId, 0x00CEFC28);
GAME_VAR(int16_t, g_nextTypeId, 0x00BF3830);

// the vtables
constexpr uintptr_t VT_IArchivableBin = 0x009191FC;
constexpr uintptr_t VT_HUDComponent = 0x0091920C;
constexpr uintptr_t VT_HUD2DSigns = 0x00918FC8;
constexpr uintptr_t VT_HUDBarber = 0x009190D0;
constexpr uintptr_t VT_HUDBigScriptText = 0x00919108;
constexpr uintptr_t VT_HUDButtonHistory = 0x00919140;
constexpr uintptr_t VT_HUDClockWarnings = 0x0091918C;
constexpr uintptr_t VT_HUDClothing = 0x009191C4;
constexpr uintptr_t VT_HUDDetention = 0x00919534;
constexpr uintptr_t VT_HUDDiary = 0x009195AC;
constexpr uintptr_t VT_HUDEntityHealth = 0x00919654;
constexpr uintptr_t VT_HUDGenericCounter = 0x00919708;
constexpr uintptr_t VT_HUDGenericText = 0x0091976C;
constexpr uintptr_t VT_HUDHealthMeter = 0x009197B0;
constexpr uintptr_t VT_HUDInventory = 0x009198B0;
constexpr uintptr_t VT_HUDMissionTimer = 0x0091993C;
constexpr uintptr_t VT_HUDMoney = 0x00919978;
constexpr uintptr_t VT_HUDMonitor = 0x009199C0;
constexpr uintptr_t VT_HUDNewspaper = 0x00919A3C;
constexpr uintptr_t VT_HUDPhoto = 0x00919A84;
constexpr uintptr_t VT_HUDRacing = 0x00919C30;
constexpr uintptr_t VT_HUDScreenFade = 0x0091A924;
constexpr uintptr_t VT_HUDSecondaryHealthBar = 0x0091A9A4;
constexpr uintptr_t VT_HUDSmallScriptText = 0x0091A9E8;
constexpr uintptr_t VT_HUDSocial = 0x0091AADC;
constexpr uintptr_t VT_HUDSocialApologize = 0x0091AB14;
constexpr uintptr_t VT_HUDSocialSmoothTalk = 0x0091AB4C;
constexpr uintptr_t VT_HUDSocialFlirt = 0x0091AB84;
constexpr uintptr_t VT_HUDSocialKiss = 0x0091ABBC;
constexpr uintptr_t VT_HUDSocialFollow = 0x0091ABF4;
constexpr uintptr_t VT_HUDSocialTaunt = 0x0091AC2C;
constexpr uintptr_t VT_HUDSocialShove = 0x0091AC64;
constexpr uintptr_t VT_HUDSocialHumiliate = 0x0091AC9C;
constexpr uintptr_t VT_HUDSocialStop = 0x0091ACD4;
constexpr uintptr_t VT_HUDSocialGift = 0x0091AD0C;
constexpr uintptr_t VT_HUDSocialDismiss = 0x0091AD44;
constexpr uintptr_t VT_HUDSocialJoin = 0x0091AD7C;
constexpr uintptr_t VT_HUDSocialGreet = 0x0091ADB4;
constexpr uintptr_t VT_HUDSocialBurn = 0x0091ADEC;
constexpr uintptr_t VT_HUDSocialErrands = 0x0091AE44;
constexpr uintptr_t VT_HUDStruggleButton = 0x0091AE80;
constexpr uintptr_t VT_HUDTaggingMeter = 0x0091AF50;
constexpr uintptr_t VT_HUDTasks = 0x0091AF9C;
constexpr uintptr_t VT_HUDTattoo = 0x0091AFD4;
constexpr uintptr_t VT_HUDText = 0x0091B00C;
constexpr uintptr_t VT_HUDTutorial = 0x0091B45C;
constexpr uintptr_t VT_HUDWeapons = 0x0091B57C;
// 2DSigns' signs
constexpr uintptr_t VT_BulletinBoard = 0x00919000;
constexpr uintptr_t VT_FortuneTeller = 0x00919010;
constexpr uintptr_t VT_Globe = 0x00919020;

re::Ptr32<void>* Vtable(uintptr_t address) {
    return reinterpret_cast<re::Ptr32<void>*>(address);
}

// A virtual call (thiscall) through the object's vtable
template <typename R, typename... A>
R VCall(HUDComponent* self, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(HUDComponent*, re::Arg<A>...)>(static_cast<void*>(self->vtable[slot]))(self, args...);
}
enum {
    SLOT_IS_VISIBLE = 4,
    SLOT_SET_VISIBLE = 5,
    SLOT_DRAW = 8,
    SLOT_DRAW_FADE_IN = 9,
    SLOT_DRAW_FADE_OUT = 10,
    SLOT_ON_SHOW = 11,
    SLOT_ON_HIDE = 12,
};

// a thiscall function of the game's by its address, where modes apply
template <uintptr_t Address>
void CallThis(HUDComponent* self) {
    reinterpret_cast<void(__thiscall*)(HUDComponent*)>(Address)(self);
}

// the timer in seconds, as the game computes it (uint32 to float, / 1000 in double)
float TimerSeconds() {
    return float(double(g_timeInMilliseconds) / 1000.0);
}
// ... and with 0.001 (a float constant widened: 0.0010000000474974513)
float TimerSecondsMul() {
    return float(double(g_timeInMilliseconds) * double(0.001f));
}

const CVector2D& Position(CVector2D& out, int16_t x, int16_t y, const HUDComponent* c) {
    return *ScreenPosition(&out, float(x), float(y), c->hAlign, c->vAlign);
}
const col::CRect& Rect(col::CRect& out, int16_t x, int16_t y, int16_t w, int16_t h, const HUDComponent* c) {
    return *ScreenRect(&out, float(x), float(y), float(w), float(h), c->hAlign, c->vAlign);
}
int16_t S16(const uint8_t* data, int offset) {
    int16_t v;
    std::memcpy(&v, data + offset, 2);
    return v;
}
CRGBA Colour(const uint8_t* data, int offset) {
    return CRGBA{data[offset], data[offset + 1], data[offset + 2], data[offset + 3]};
}
// degrees (a short) to radians, with the game's float pi widened
float Radians(int16_t degrees) {
    return float(double(degrees) * double(3.14159265358979f) / 180.0);
}
void SetEmptyRect(col::CRect& r) {
    r.left = 1000000.0f;
    r.top = -1000000.0f;
    r.right = -1000000.0f;
    r.bottom = 1000000.0f;
}
// the centre of a rect, in whole (halved) pixels of its size ("sar 1" of the truncated size)
void RectCentre(const col::CRect& r, int16_t w, int16_t h, float& cx, float& cy) {
    int halfH = int(double(float(h)) * double(g_screenScaleY)) >> 1;
    int halfW = int(double(float(w)) * double(g_screenScaleX)) >> 1;
    cx = float(double(halfW) + double(r.left));
    cy = float(double(r.top) + double(halfH));
}
} // namespace

// ---- HUDComponent ---------------------------------------------------------------------

namespace hud {

// 0x53CDC0
HUDComponent* __fastcall Construct(HUDComponent* self, re::Edx) {
    self->unknown1C = 1.0f;
    self->vtable = Vtable(VT_HUDComponent);
    self->visible = 0;
    self->flashing = 0;
    self->suppressed = 0;
    self->drawState = HUD_DRAW_SHOWN;
    self->hAlign = 0;
    self->vAlign = 0;
    self->flashStart = 0;
    return self;
}

// 0x53CDF0: (HUDComponent's destructor is IArchivableBin's)
void __fastcall Destruct(HUDComponent* self, re::Edx) {
    self->vtable = Vtable(VT_IArchivableBin);
}

// 0x53CF30: IArchivableBin's deleting destructor
HUDComponent* __fastcall IArchivableBin_Delete(HUDComponent* self, re::Edx, int flags) {
    self->vtable = Vtable(VT_IArchivableBin);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x539090 (vf3): the one id the game ever gives out here (a function-local static,
// the same for every kind: they all share this one)
uint8_t __fastcall IsType(HUDComponent*, re::Edx, int id) {
    if (!(g_isTypeGuard & 1)) {
        g_isTypeGuard |= 1;
        g_isTypeId = g_nextTypeId;
        g_nextTypeId = int16_t(g_nextTypeId + 1);
    }
    return int16_t(id) == g_isTypeId;
}

// 0x5390D0 (vf4)
uint8_t __fastcall IsVisible(HUDComponent* self, re::Edx) {
    return self->visible;
}

// 0x53CEB0 (vf5)
void __fastcall SetVisible(HUDComponent* self, re::Edx, int visible) {
    uint8_t was = self->visible;
    self->visible = uint8_t(visible);
    if (re::Flag(visible)) {
        if (!was) {
            // (a call to an empty function first, 0x49A650: a stripped debug print)
            VCall<void>(self, SLOT_ON_SHOW);
        }
    } else if (was) {
        VCall<void>(self, SLOT_ON_HIDE);
    }
    self->drawState = HUD_DRAW_SHOWN;
    if (!re::Flag(visible)) {
        self->flashing = 0;
    }
}

// 0x5390E0 (vf6)
void __fastcall SetDrawState(HUDComponent* self, re::Edx, int state) {
    self->drawState = state;
    self->visible = 1;
}

// 0x53CF00 (vf7)
void __fastcall Reset(HUDComponent* self, re::Edx) {
    VCall<void>(self, SLOT_SET_VISIBLE, 0);
    self->drawState = HUD_DRAW_SHOWN;
    self->flashing = 0;
    self->suppressed = 0;
    self->flashStart = 0;
}

// 0x53CF50: a frame's drawing, by the draw state; flashing ones every other 250 ms
// for a second. (The font's scale is 0.9 around it in language 7.)
void __fastcall Render(HUDComponent* self, re::Edx) {
    float savedScale = g_fontScale;
    if (g_language == 7) {
        g_fontScale = 0.9f;
    }
    if (VCall<uint8_t>(self, SLOT_IS_VISIBLE) && !self->suppressed) {
        bool draw = true;
        if (self->flashing) {
            uint32_t now = g_useOtherTimer ? g_otherTimer : g_timeInMilliseconds;
            uint32_t quarters = (now - self->flashStart) / 250;
            draw = (quarters & 1) != 0;
            if (quarters > 3) {
                self->flashing = 0;
                draw = true;
            }
        }
        if (draw) {
            switch (self->drawState) {
            case HUD_DRAW_FADING_IN:
                VCall<void>(self, SLOT_DRAW_FADE_IN);
                break;
            case HUD_DRAW_SHOWN:
                VCall<void>(self, SLOT_DRAW);
                break;
            case HUD_DRAW_FADING_OUT:
                VCall<void>(self, SLOT_DRAW_FADE_OUT);
                break;
            }
        }
    }
    if (g_language == 7) {
        g_fontScale = savedScale;
    }
}

// 0x53CE00: from the layout's names
void __fastcall SetAlignment(HUDComponent* self, re::Edx, const char* h, const char* v) {
    self->hAlign = HAlignFromName(h);
    self->vAlign = VAlignFromName(v);
}

// 0x53CE40: the font justified as the component is aligned, with its wrap width
// when one's given
void __fastcall SetTextJustify(HUDComponent* self, re::Edx, int wrap) {
    switch (self->hAlign) {
    case 0:
        Font_SetJustify(0);
        if (wrap > 0) {
            g_fontWrapLeft = float(wrap);
        }
        break;
    case 1:
        Font_SetJustify(2);
        if (wrap > 0) {
            g_fontWrapCentre = float(wrap);
        }
        break;
    case 2:
        Font_SetJustify(1);
        if (wrap > 0) {
            g_fontWrapRight = float(wrap);
        }
        break;
    }
}

const char* __cdecl GetHUDComponentTypeName(uint32_t type) {
    if (type > 0x2F) {
        return reinterpret_cast<const char*>(0x00903168);  // "INVALID"
    }
    return g_hudComponentTypeNames[type];
}

// CreateHUDComponent's cases: the size and the constructor (thiscall, returns it)
struct ComponentKind {
    uint32_t size;
    uintptr_t construct;
};
static const ComponentKind kComponentKinds[HUD_NUM_COMPONENT_TYPES] = {
    {0x74, 0x0053D610},   {0x3C8, 0x0053A8E0}, {0x3C8, 0x00553BF0}, {0x64, 0x005427B0},
    {0x50, 0x00540440},   {0x44, 0x0053E7F0},  {0x30, 0x006B1770},  {0x68, 0x00543B00},
    {0x2FC, 0x0053F6B0},  {0xA5C, 0x005400D0}, {0x78, 0x005469B0},  {0xE848, 0x0054B280},
    {0x180, 0x00553970},  {0x48, 0x00542B70},  {0x20, 0x0053CC40},  {0x64, 0x0053CB70},
    {0x88, 0x00543D70},   {0x168, 0x00557BB0}, {0x20, 0x0053A4B0},  {0x20, 0x0055AEC0},
    {0x19C, 0x0055DE70},  {0xEC, 0x0053B7F0},  {0x118, 0x0067DF10}, {0x40, 0x00555060},
    {0x40, 0x00555350},   {0x40, 0x005554B0},  {0x40, 0x00555730},  {0x40, 0x00555890},
    {0x40, 0x005559E0},   {0x40, 0x00555B30},  {0x40, 0x00555D50},  {0x40, 0x00555EA0},
    {0xA4, 0x00555FF0},   {0x40, 0x005563B0},  {0x40, 0x00556510},  {0x44, 0x00556670},
    {0x3C, 0x00556960},   {0x84, 0x00553C40},  {0x2E44, 0x00559A90}, {0xB8, 0x005434E0},
    {0x50, 0x0055B7A0},   {0x11C, 0x00539DC0}, {0x1CA0, 0x0055AFC0}, {0x2C, 0x00551430},
    {0x48, 0x0053E340},   {0xA4, 0x005412B0},  {0x7C, 0x00556CC0},  {0x3C, 0x0055AA20},
};

HUDComponent* __cdecl CreateHUDComponent(uint32_t type) {
    if (type > 0x2F) {
        return nullptr;
    }
    const ComponentKind& kind = kComponentKinds[type];
    void* memory = Malloc(kind.size);
    if (!memory) {
        return nullptr;
    }
    return reinterpret_cast<HUDComponent*(__thiscall*)(void*)>(kind.construct)(memory);
}

// ---- the kinds' destructors and constructors ------------------------------------------

// The deleting destructor most kinds have: their vtable back, HUDComponent's
// destructor, freed when asked
template <uintptr_t VT>
HUDComponent* __fastcall DeletingDestructor(HUDComponent* self, re::Edx, int flags) {
    self->vtable = Vtable(VT);
    HUDComponent_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// The constructors with nothing of their own
template <uintptr_t VT>
HUDComponent* __fastcall PlainConstructor(HUDComponent* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT);
    return self;
}

// ... and the ones that only Reset (their own, called directly)
template <uintptr_t VT, uintptr_t ResetAddress>
HUDComponent* __fastcall ResetConstructor(HUDComponent* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT);
    CallThis<ResetAddress>(self);
    return self;
}

// 0x53D610
HUDComponent* __fastcall HUDDetention_Construct(HUDDetention* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDDetention);
    HUDComponent_Reset(self);
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
    self->textEndTime = 0.0f;
    self->unknown6C = 0.0f;
    self->text[0] = 0;
    self->value = 0;
    self->unknown70 = 1.0f;
    return self;
}

// 0x5427B0 (no Reset)
HUDComponent* __fastcall HUDMissionTimer_Construct(HUDMissionTimer* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDMissionTimer);
    SetEmptyRect(self->rect);
    return self;
}

// 0x540440
HUDComponent* __fastcall HUDHealthMeter_Construct(HUDHealthMeter* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDHealthMeter);
    HUDComponent_Reset(self);
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
    self->unknown44 = 0.0f;
    self->unknown48 = 0.0f;
    self->unknown4C = 0.0f;
    return self;
}

// 0x53E7F0
HUDComponent* __fastcall HUDEntityHealth_Construct(HUDEntityHealth* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDEntityHealth);
    self->unknown20 = 0;
    return self;
}

// 0x543B00
HUDComponent* __fastcall HUDNewspaper_Construct(HUDNewspaper* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDNewspaper);
    SetEmptyRect(self->rect);
    SetEmptyRect(self->rect2);
    HUDComponent_Reset(self);
    self->unknown65 = 0;
    self->unknown64 = 0;
    self->unknown50 = 0;
    self->unknown4C = 10;
    return self;
}

// 0x53F6B0
HUDComponent* __fastcall HUDGenericCounter_Construct(HUDGenericCounter* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDGenericCounter);
    SetEmptyRect(self->rect);
    SetEmptyRect(self->rect2);
    CallThis<0x0053F290>(self);  // its Reset
    return self;
}

// 0x5469B0
HUDComponent* __fastcall HUDRacing_Construct(HUDRacing* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDRacing);
    SetEmptyRect(self->rect);
    HUDComponent_Reset(self);
    for (int i = 0; i < 5; i++) {
        self->unknown50[i] = 0;
    }
    self->unknown68 = 0;
    self->unknown6C = 0;
    self->unknown70 = 0;
    self->unknown74 = 0;
    return self;
}

// 0x553970
HUDComponent* __fastcall HUDSecondaryHealthBar_Construct(HUDSecondaryHealthBar* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDSecondaryHealthBar);
    SetEmptyRect(self->rect);
    SetEmptyRect(self->rect2);
    self->unknownAC = nullptr;
    self->unknownB0 = nullptr;
    self->unknownB4 = nullptr;
    CallThis<0x00551E00>(self);  // its Reset
    return self;
}

// 0x53CB70
HUDComponent* __fastcall HUDClockWarnings_Construct(HUDClockWarnings* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDClockWarnings);
    SetEmptyRect(self->rect);
    SetEmptyRect(self->rect2);
    return self;
}

// 0x543D70
HUDComponent* __fastcall HUDPhoto_Construct(HUDPhoto* self, re::Edx) {
    HUDComponent_Construct(self);
    self->unknown3C = 1.0f;
    self->vtable = Vtable(VT_HUDPhoto);
    self->unknown21 = 0;
    self->unknown24 = 0;
    self->unknown4C = 0;
    self->unknown36 = 0;
    self->unknown37 = 0;
    self->unknown38 = 0;
    self->unknown39 = 0;
    self->unknown40 = 0;
    self->unknown44 = 0;
    self->unknown2C = 0;
    self->unknown48 = 0;
    self->unknown7C = 0;
    self->unknown80 = 0;
    self->nameHash58 = HashName(reinterpret_cast<const char*>(0x00906974));
    self->nameHash5C = HashName(reinterpret_cast<const char*>(0x00906970));
    self->unknown50 = 0;
    self->unknown54 = 0;
    self->unknown20 = 1;
    return self;
}

// 0x557BB0
HUDComponent* __fastcall HUDStruggleButton_Construct(HUDStruggleButton* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDStruggleButton);
    SetEmptyRect(self->rect);
    SetEmptyRect(self->rect2);
    SetEmptyRect(self->rect3);
    CallThis<0x00557220>(self);  // its Reset
    return self;
}

// 0x55DE70
HUDComponent* __fastcall HUDWeapons_Construct(HUDWeapons* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDWeapons);
    self->unknown6C = 0;
    self->unknownBC = 0;
    self->unknown12C = 0;
    self->unknown12D = 0;
    CallThis<0x0055CC50>(self);  // its Reset
    return self;
}

// 0x53B7F0
HUDComponent* __fastcall HUDButtonHistory_Construct(HUDButtonHistory* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDButtonHistory);
    self->lists[0] = nullptr;
    self->lists[1] = nullptr;
    self->lists[2] = nullptr;
    self->buffers[0].data = nullptr;
    self->buffers[1].data = nullptr;
    self->unknown84 = nullptr;
    self->unknown8C = nullptr;
    self->unknown94 = nullptr;
    self->unknownCC = 0;
    self->unknownB4 = -1;
    self->unknownB8 = 0;
    self->unknownBC = 0;
    self->unknownB0 = 0;
    CallThis<0x0053A990>(self);  // its Reset
    HUDButtonHistory_53B2C0(self);
    return self;
}

namespace {
GAME_FN(void, __thiscall, HUDButtonHistory_53AD00, 0x0053AD00, HUDComponent* self);
// new[]'s size: count * 0x18, all ones when it overflows
uint32_t ArraySize(int32_t count) {
    uint64_t size = uint64_t(uint32_t(count)) * 0x18;
    return size > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(size);
}
} // namespace

// 0x53B2C0: its three arrays of 9
void __fastcall HUDButtonHistory_Init(HUDButtonHistory* self, re::Edx) {
    self->capacity = 9;
    self->unknown84 = Malloc(ArraySize(self->capacity));
    self->unknown8C = Malloc(ArraySize(self->capacity));
    self->unknown94 = Malloc(ArraySize(self->capacity));
    HUDButtonHistory_53AD00(self);
}

// 0x53A900: HUDButtonHistory's destructor (its deleting one is 0x53B2A0)
void __fastcall HUDButtonHistory_Destruct(HUDButtonHistory* self, re::Edx) {
    self->vtable = Vtable(VT_HUDButtonHistory);
    for (int i = 0; i < 2; i++) {
        if (self->buffers[i].data) {
            Free(self->buffers[i].data);
            self->buffers[i].data = nullptr;
        }
    }
    if (self->unknown84) {
        Free(self->unknown84);
    }
    if (self->unknown8C) {
        Free(self->unknown8C);
    }
    if (self->unknown94) {
        Free(self->unknown94);
    }
    ButtonHistory_List_Destruct(&self->lists[2]);
    ButtonHistory_List_Destruct(&self->lists[1]);
    ButtonHistory_List_Destruct(&self->lists[0]);
    HUDComponent_Destruct(self);
}

// 0x553C40
HUDComponent* __fastcall HUDSocial_Construct(HUDSocial* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDSocial);
    for (int i = 0; i < 14; i++) {
        self->unknown4C[i] = g_socialDefaults[i];
    }
    self->unknown40 = 0;
    return self;
}

// 0x555FF0: the gift menu's buttons (ids, by the slot)
HUDComponent* __fastcall HUDSocialGift_Construct(HUDComponent* self, re::Edx) {
    static const int32_t kIds[0x19] = {
        0,    0x34, 0x36, 0x35, 0x39, 0x37, 0x38, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x4C,
        0x4D, 0x4F, 0x50, 0x51, 0x43, 0x53, 0x44, 0x45, 0x42, 0x33, 0x32, 0x31,
    };
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDSocialGift);
    std::memcpy(reinterpret_cast<uint8_t*>(self) + 0x40, kIds, sizeof(kIds));
    return self;
}

// 0x556670
HUDComponent* __fastcall HUDSocialGreet_Construct(HUDComponent* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDSocialGreet);
    reinterpret_cast<uint8_t*>(self)[0x40] = 0;
    return self;
}

// 0x556CC0: (SetVisible called through the vtable's memory, not the object's)
HUDComponent* __fastcall HUDSocialErrands_Construct(HUDSocialErrands* self, re::Edx) {
    HUDComponent_Construct(self);
    for (int i = 0; i < 4; i++) {
        self->unknown4C[i] = 0.0f;
    }
    self->vtable = Vtable(VT_HUDSocialErrands);
    self->unknown49 = 0;
    self->unknown48 = 0;
    self->unknown4A = 0;
    self->unknown5C = 0;
    re::PtrCast<void(__thiscall*)(HUDComponent*, int)>(Vtable(VT_HUDSocialErrands)[SLOT_SET_VISIBLE])(self, 1);
    return self;
}

// 0x559A90: three sets of 7 tag shapes (0x1E8 bytes each)
HUDComponent* __fastcall HUDTaggingMeter_Construct(HUDComponent* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDTaggingMeter);
    uint8_t* base = reinterpret_cast<uint8_t*>(self);
    static const int kSets[3] = {0x40, 0xD98, 0x1AF0};
    for (int offset : kSets) {
        for (int i = 0; i < 7; i++) {
            TaggingShape_Construct(base + offset + i * 0x1E8);
        }
    }
    CallThis<0x005586F0>(self);  // its Reset
    return self;
}

// 0x5434E0
HUDComponent* __fastcall HUDMonitor_Construct(HUDMonitor* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDMonitor);
    self->texture = nullptr;
    self->loaded = 0;
    for (int i = 0; i < 5; i++) {
        self->lines[i].text[0] = 0;
        self->lines[i].unknown19 = 0;
    }
    return self;
}

// 0x55B7A0
HUDComponent* __fastcall HUDTutorial_Construct(HUDTutorial* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDTutorial);
    SetEmptyRect(self->rect);
    HUDComponent_Reset(self);
    self->unknown21 = 0;
    self->unknown3C = 0;
    self->unknown34 = 0;
    self->unknown38 = -1;
    return self;
}

// 0x539DC0
HUDComponent* __fastcall HUD2DSigns_Construct(HUD2DSigns* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUD2DSigns);
    self->unknown24[0] = 0.0f;
    self->bulletinBoard = Vtable(VT_BulletinBoard);
    self->unknown24[1] = 0.0f;
    self->fortuneTeller = Vtable(VT_FortuneTeller);
    self->unknown24[2] = 0.0f;
    self->globe = Vtable(VT_Globe);
    self->unknown40 = 0.0f;
    self->unknown20 = 0;
    self->texture = nullptr;
    self->unknown104 = 0;
    self->unknown114 = 0;
    self->txd = -1;
    return self;
}

// 0x53E340
HUDComponent* __fastcall HUDDiary_Construct(HUDDiary* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDDiary);
    HUDComponent_Reset(self);
    self->drawState = HUD_DRAW_SHOWN;
    self->visible = 0;
    self->txd = -1;
    return self;
}

// 0x5412B0
HUDComponent* __fastcall HUDInventory_Construct(HUDInventory* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = Vtable(VT_HUDInventory);
    HUDComponent_Reset(self);
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
    self->count = 0;
    self->last = 0;
    self->first = 0;
    return self;
}

// ---- Reset (vf7) ---------------------------------------------------------------------

// 0x5390F0 (not the base's)
void __fastcall HUD2DSigns_Reset(HUD2DSigns* self, re::Edx) {
    self->unknown40 = 0.0f;
    self->unknown20 = 0;
    self->txd = -1;
    self->texture = nullptr;
}

// 0x53D5E0
void __fastcall HUDDetention_Reset(HUDDetention* self, re::Edx) {
    HUDComponent_Reset(self);
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
    self->textEndTime = 0.0f;
    self->unknown6C = 0.0f;
    self->text[0] = 0;
    self->value = 0;
    self->unknown70 = 1.0f;
}

// 0x53E7D0: the base's and shown (ClockWarnings', EntityHealth's)
void __fastcall ResetShown(HUDComponent* self, re::Edx) {
    HUDComponent_Reset(self);
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
}

// 0x53E210
void __fastcall HUDDiary_Reset(HUDDiary* self, re::Edx) {
    HUDComponent_Reset(self);
    self->drawState = HUD_DRAW_SHOWN;
    self->visible = 0;
    self->txd = -1;
}

// 0x53F290: (hidden through the base's SetVisible, not its own)
void __fastcall HUDGenericCounter_Reset(HUDGenericCounter* self, re::Edx) {
    HUDComponent_SetVisible(self, 0);
    self->colour = g_counterColour;
    self->max = 10;
    self->value = 0;
    self->previousValue = 0;
    if (self->icon) {
        NiRelease(self->icon);
        self->icon = nullptr;
    }
    self->unknown61 = 0;
    self->unknownA4 = 0.0f;
    self->unknown63 = 0;
    self->unknown62 = 0;
    self->unknownA8 = 0.0f;
    self->unknown60 = 1;
}

// 0x540070
void __fastcall HUDGenericText_Reset(HUDGenericText* self, re::Edx) {
    HUDComponent_Reset(self);
    for (auto& line : self->lines) {
        line.startTime = 0;
        line.text[0] = 0;
        line.id = -1;
    }
}

// 0x540420
void __fastcall HUDHealthMeter_Reset(HUDHealthMeter* self, re::Edx) {
    HUDComponent_Reset(self);
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
    self->unknown44 = 0.0f;
    self->unknown48 = 0.0f;
    self->unknown4C = 0.0f;
}

// 0x540E20
void __fastcall HUDInventory_Reset(HUDInventory* self, re::Edx) {
    HUDComponent_Reset(self);
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
    self->count = 0;
    self->last = 0;
    self->first = 0;
}

// 0x541F30
void __fastcall HUDMissionTimer_Reset(HUDMissionTimer* self, re::Edx) {
    HUDComponent_Reset(self);
    self->visible = 1;
    self->unknown44 = 0;
}

// 0x542830: shows the player's money as it is (no counting)
void __fastcall HUDMoney_Reset(HUDMoney* self, re::Edx) {
    HUDComponent_Reset(self);
    self->unknown30 = 0;
    if (g_playerPed) {
        // (the money read as unsigned)
        self->shownMoney = float(*reinterpret_cast<uint32_t*>(g_playerPed + 0x1CA0));
    } else {
        self->shownMoney = 0.0f;
    }
    self->money = int(self->shownMoney);
    self->countFrom = 0.0f;
    self->countEndTime = g_timeInMilliseconds - 1;
}

// 0x5435B0
void __fastcall HUDNewspaper_Reset(HUDNewspaper* self, re::Edx) {
    HUDComponent_Reset(self);
    self->unknown65 = 0;
    self->unknown64 = 0;
    self->unknown4C = 10;
    self->unknown50 = 0;
}

// 0x546780
void __fastcall HUDRacing_Reset(HUDRacing* self, re::Edx) {
    HUDComponent_Reset(self);
    for (int i = 0; i < 5; i++) {
        self->unknown50[i] = 0;
    }
    self->unknown68 = 0;
    self->unknown6C = 0;
    self->unknown70 = 0;
    self->unknown74 = 0;
}

// 0x551450
void __fastcall HUDScreenFade_Reset(HUDScreenFade* self, re::Edx) {
    HUDComponent_Reset(self);
    self->unknown20 = 0;
    self->unknown24 = 0;
}

// 0x553A50: Big/SmallText_ForScript: hidden, gone a second ago
void __fastcall HUDScriptText_Reset(HUDScriptText* self, re::Edx) {
    HUDComponent_Reset(self);
    self->endTime = 0.0f;
    VCall<void>(self, SLOT_SET_VISIBLE, 0);
    self->text[0] = 0;
    self->numParams = 0;
    self->shown = 0;
    self->endTime = float(double(g_timeInMilliseconds) / 1000.0 - 1.0);
}

// 0x553C10
void __fastcall HUDSocial_Reset(HUDSocial* self, re::Edx) {
    HUDComponent_Reset(self);
    self->unknown40 = 0;
}

// 0x556C00 (not the base's)
void __fastcall HUDSocialErrands_Reset(HUDSocialErrands* self, re::Edx) {
    self->unknown4C[0] = 0.0f;
    self->unknown49 = 0;
    self->unknown48 = 0;
    self->unknown4C[1] = 0.0f;
    self->unknown4A = 0;
    self->unknown4C[2] = 0.0f;
    self->unknown5C = 0;
    self->unknown4C[3] = 0.0f;
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
}

// 0x557220 (not the base's)
void __fastcall HUDStruggleButton_Reset(HUDStruggleButton* self, re::Edx) {
    self->visible = 0;
    self->button = 7;
    self->unknown160 = 0;
    self->unknown161 = 1;
    std::strncpy(self->name, "NONE", 0xFF);  // (0xFF into 0x108 bytes)
    self->unknown164 = 0.0f;
    self->unknown15C = 0;
    self->unknown162 = 1;
}

// 0x5586F0 (not the base's; hidden without OnHide)
void __fastcall HUDTaggingMeter_Reset(HUDComponent* self, re::Edx) {
    uint8_t* p = reinterpret_cast<uint8_t*>(self);
    auto f = [p](int offset) -> float& { return *reinterpret_cast<float*>(p + offset); };
    f(0x284C) = 0.0f;
    self->visible = 0;
    f(0x288C) = 0.0f;
    p[0x2848] = 0;
    f(0x2890) = 0.0f;
    f(0x2894) = 0.0f;
    for (int offset = 0x2868; offset <= 0x2884; offset += 4) {
        f(offset) = 1.0f;
    }
    *reinterpret_cast<int32_t*>(p + 0x2E38) = 0;
}

namespace {
GAME_VAR(void*, g_radar, 0x00C674CC);  // the HUDRadar
GAME_FN(int, __thiscall, HUDRadar_548A80, 0x00548A80, void* radar, int blip);
GAME_FN(void, __thiscall, HUDRadar_RemoveBlip, 0x00548DE0, void* radar, int index);
} // namespace

// 0x55A570: its blip off the radar
void __fastcall HUDTasks_Reset(HUDTasks* self, re::Edx) {
    HUDComponent_Reset(self);
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
    void* radar = g_radar;
    if (self->blipType == 1) {
        HUDRadar_RemoveBlip(radar, HUDRadar_548A80(radar, self->unknown28[0]));
    } else if (self->blipType == 2) {
        HUDRadar_RemoveBlip(radar, HUDRadar_548A80(radar, self->unknown28[2]));
    }
    self->unknown24 = -1.0f;
    self->blipType = 0;
    for (int i = 0; i < 5; i++) {
        self->unknown28[i] = -1;
    }
}

// 0x55AFE0
void __fastcall HUDText_Reset(HUDText* self, re::Edx) {
    HUDComponent_Reset(self);
    for (auto& slot : self->slots) {
        slot.numParams = 0;
        slot.text[0] = 0;
    }
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
}

// 0x55B610
void __fastcall HUDTutorial_Reset(HUDTutorial* self, re::Edx) {
    HUDComponent_Reset(self);
    self->unknown21 = 0;
    self->unknown3C = 0;
    self->unknown38 = -1;
    self->unknown34 = 0;
}

// ---- Setup (vf1): from the layout ------------------------------------------------------

// 0x53A610: Big/SmallText_ForScript
void __fastcall HUDScriptText_Setup(HUDScriptText* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0xA), reinterpret_cast<const char*>(d + 0x12));
    self->colour = Colour(d, 0);
    self->font = S16(d, 8);
    CVector2D pos;
    Position(pos, S16(d, 4), S16(d, 6), self);
    self->x = pos.x;
    self->y = pos.y;
}

// 0x53C9B0
void __fastcall HUDClockWarnings_Setup(HUDClockWarnings* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x1C), reinterpret_cast<const char*>(d + 0x24));
    col::CRect r;
    self->rect = Rect(r, S16(d, 8), S16(d, 0xA), S16(d, 0xC), S16(d, 0xE), self);
    RectCentre(self->rect, S16(d, 0xC), S16(d, 0xE), self->centreX, self->centreY);
    self->unknown54 = S16(d, 0x10);
    self->unknown58 = S16(d, 0x12);
    self->rect2 = Rect(r, S16(d, 0x14), S16(d, 0x16), S16(d, 0x18), S16(d, 0x1A), self);
    self->colours[0] = Colour(d, 0);
    self->colours[1] = Colour(d, 4);
    self->colourDelta[0] = int(self->colours[1].r) - int(self->colours[0].r);
    self->colourDelta[1] = int(self->colours[1].g) - int(self->colours[0].g);
    self->colourDelta[2] = int(self->colours[1].b) - int(self->colours[0].b);
}

// 0x53E0D0 (the y scaled a second time: the game's)
void __fastcall HUDDetention_Setup(HUDDetention* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x18), reinterpret_cast<const char*>(d + 0x20));
    CVector2D pos;
    Position(pos, S16(d, 8), S16(d, 0xA), self);
    self->x = pos.x;
    self->y = pos.y;
    self->y = self->y * g_screenScaleY;
    self->width = float(S16(d, 0xC));
    self->height = float(S16(d, 0xE));
    self->angleFrom = Radians(S16(d, 0x10));
    self->angleTo = Radians(S16(d, 0x12));
    self->unknown38 = S16(d, 0x14);
    self->colours[0] = Colour(d, 0);
    self->colours[1] = Colour(d, 4);
}

// 0x540470: the same as Detention's
void __fastcall HUDHealthMeter_Setup(HUDHealthMeter* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x16), reinterpret_cast<const char*>(d + 0x1E));
    CVector2D pos;
    Position(pos, S16(d, 8), S16(d, 0xA), self);
    self->x = pos.x;
    self->y = pos.y;
    self->y = self->y * g_screenScaleY;
    self->width = float(S16(d, 0xC));
    self->height = float(S16(d, 0xE));
    self->angleFrom = Radians(S16(d, 0x10));
    self->angleTo = Radians(S16(d, 0x12));
    self->unknown38 = S16(d, 0x14);
    self->colours[0] = Colour(d, 0);
    self->colours[1] = Colour(d, 4);
}

// 0x53E3A0: (+0x20 unaligned and unscaled)
void __fastcall HUDDiary_Setup(HUDDiary* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x10), reinterpret_cast<const char*>(d + 0x18));
    self->x = float(S16(d, 0));
    self->y = float(S16(d, 2));
    CVector2D pos;
    Position(pos, S16(d, 4), S16(d, 6), self);
    self->x28 = pos.x;
    self->y28 = pos.y;
    Position(pos, S16(d, 8), S16(d, 0xA), self);
    self->x30 = pos.x;
    self->y30 = pos.y;
    self->width = float(S16(d, 0xC)) * g_screenScaleX;
    self->height = float(S16(d, 0xE)) * g_screenScaleY;
}

// 0x53E810: the record's words as they are
void __fastcall HUDEntityHealth_Setup(HUDEntityHealth* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    auto u32 = [d](int offset) {
        uint32_t v;
        std::memcpy(&v, d + offset, 4);
        return v;
    };
    self->unknown24 = u32(0);
    self->unknown28 = u32(4);
    self->unknown30 = u32(8);
    self->unknown2C = u32(0xC);
    self->unknown38 = S16(d, 0x1C);
    self->unknown34 = u32(0x10);
    self->colours[0] = Colour(d, 0x14);
    self->colours[1] = Colour(d, 0x18);
}

// 0x53F560
void __fastcall HUDGenericCounter_Setup(HUDGenericCounter* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x14), reinterpret_cast<const char*>(d + 0x1C));
    CVector2D pos;
    Position(pos, S16(d, 0), S16(d, 2), self);
    self->x30 = pos.x;
    self->y30 = pos.y;
    col::CRect r;
    self->rect = Rect(r, S16(d, 4), S16(d, 6), S16(d, 8), S16(d, 0xA), self);
    self->rect2 = Rect(r, S16(d, 0xC), S16(d, 0xE), S16(d, 0x10), S16(d, 0x12), self);
}

// 0x540360
void __fastcall HUDGenericText_Setup(HUDGenericText* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0xC), reinterpret_cast<const char*>(d + 0x14));
    self->unknown2C = S16(d, 8);
    self->unknown30 = S16(d, 0xA);
    self->colour = Colour(d, 0);
    CVector2D pos;
    Position(pos, S16(d, 4), S16(d, 6), self);
    self->x = pos.x;
    self->y = pos.y;
}

// 0x5426A0
void __fastcall HUDMissionTimer_Setup(HUDMissionTimer* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x10), reinterpret_cast<const char*>(d + 0x18));
    col::CRect r;
    self->rect = Rect(r, S16(d, 4), S16(d, 6), S16(d, 8), S16(d, 0xA), self);
    RectCentre(self->rect, S16(d, 8), S16(d, 0xA), self->centreX, self->centreY);
    self->unknown38 = S16(d, 0xC);
    self->unknown3C = S16(d, 0xE);
    self->colour = Colour(d, 0);
}

// 0x542AC0: whole pixels
void __fastcall HUDMoney_Setup(HUDMoney* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0xE), reinterpret_cast<const char*>(d + 0x16));
    CVector2D pos;
    Position(pos, S16(d, 8), S16(d, 0xA), self);
    self->x = int(pos.x);
    self->y = int(pos.y);
    std::memcpy(&self->countSeconds, d + 4, 4);
    self->unknown44 = S16(d, 0xC);
    self->colour.r = g_moneyColour.r;
    self->colour.g = g_moneyColour.g;
    self->colour.b = g_moneyColour.b;
    self->colour.a = 0xFF;
}

// 0x542E20
void __fastcall HUDMonitor_Setup(HUDMonitor* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x14), reinterpret_cast<const char*>(d + 0x1C));
    self->unknown28 = reinterpret_cast<int(__thiscall*)(HUDComponent*, const char*)>(0x0053CE30)(
        self, reinterpret_cast<const char*>(d + 0x24));
    CVector2D pos;
    for (int i = 0; i < 4; i++) {
        Position(pos, S16(d, i * 4), S16(d, i * 4 + 2), self);
        self->lines[i].x = pos.x;
        self->lines[i].y = pos.y;
    }
}

// 0x543990
void __fastcall HUDNewspaper_Setup(HUDNewspaper* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x18), reinterpret_cast<const char*>(d + 0x20));
    col::CRect r;
    self->rect = Rect(r, S16(d, 0), S16(d, 2), S16(d, 4), S16(d, 6), self);
    self->unknown40 = S16(d, 8);
    self->unknown60 = S16(d, 0xA);
    self->rect2 = Rect(r, S16(d, 0xC), S16(d, 0xE), S16(d, 0x10), S16(d, 0x12), self);
    CVector2D pos;
    Position(pos, S16(d, 0x14), S16(d, 0x16), self);
    self->x44 = pos.x;
    self->y44 = pos.y;
    self->unknown54 = 0;
    self->x58 = self->rect.left;
    self->y58 = self->rect.top;
}

// 0x5467E0
void __fastcall HUDRacing_Setup(HUDRacing* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x1C), reinterpret_cast<const char*>(d + 0x24));
    CVector2D pos;
    Position(pos, S16(d, 8), S16(d, 0xA), self);
    self->x = pos.x;
    self->y = pos.y;
    Position(pos, S16(d, 0xC), S16(d, 0xE), self);
    self->x28 = pos.x;
    self->y28 = pos.y;
    Position(pos, S16(d, 0x10), S16(d, 0x12), self);
    self->x30 = pos.x;
    self->y30 = pos.y;
    self->colours[0] = Colour(d, 0);
    self->colours[1] = Colour(d, 4);
    col::CRect r;
    self->rect = Rect(r, S16(d, 0x14), S16(d, 0x16), S16(d, 0x18), S16(d, 0x1A), self);
}

// 0x555610: the social menu's (and the tagging meter's)
void __fastcall HUDSocialBase_Setup(HUDSocialBase* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0xC), reinterpret_cast<const char*>(d + 0x14));
    self->x = float(S16(d, 0));
    self->y = float(S16(d, 2));
    self->x = self->x * g_screenScaleX;
    self->y = g_screenScaleY * self->y;
    CVector2D pos;
    Position(pos, S16(d, 4), S16(d, 6), self);
    self->iconX = pos.x;
    self->iconY = pos.y;
    self->width = float(S16(d, 8));
    self->height = float(S16(d, 0xA));
    self->width = self->width * g_screenScaleX;
    self->height = self->height * g_screenScaleY;
    self->unknown38 = float((double(self->y) - double(self->height)) * 0.5);
    self->unknown3C = float((double(self->x) - double(self->width)) * 0.5);
}

// 0x556AD0: the same, the halves in whole pixels
void __fastcall HUDSocialBurn_Setup(HUDSocialBurn* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0xC), reinterpret_cast<const char*>(d + 0x14));
    self->x = float(S16(d, 0));
    self->y = float(S16(d, 2));
    self->x = self->x * g_screenScaleX;
    self->y = g_screenScaleY * self->y;
    CVector2D pos;
    Position(pos, S16(d, 4), S16(d, 6), self);
    self->iconX = pos.x;
    self->iconY = pos.y;
    self->width = float(S16(d, 8));
    self->height = float(S16(d, 0xA));
    self->width = self->width * g_screenScaleX;
    self->height = self->height * g_screenScaleY;
    self->unknown38 = int16_t(int((double(self->y) - double(self->height)) * 0.5));
    self->unknown3A = int16_t(int((double(self->x) - double(self->width)) * 0.5));
}

// 0x5570C0
void __fastcall HUDSocialErrands_Setup(HUDSocialErrands* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 0x14), reinterpret_cast<const char*>(d + 0x1C));
    CVector2D pos;
    Position(pos, S16(d, 8), S16(d, 0xA), self);
    self->x = pos.x;
    self->y = pos.y;
    Position(pos, S16(d, 0xC), S16(d, 0xE), self);
    self->x28 = pos.x;
    self->y28 = pos.y;
    self->width = float(S16(d, 0x10));
    self->height = float(S16(d, 0x12));
    self->width = self->width * g_screenScaleX;
    self->height = self->height * g_screenScaleY;
    self->colours[0] = Colour(d, 0);
    self->colours[1] = Colour(d, 4);
}

// 0x55B630
void __fastcall HUDTutorial_Setup(HUDTutorial* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(d + 8), reinterpret_cast<const char*>(d + 0x10));
    col::CRect r;
    self->rect = Rect(r, S16(d, 0), S16(d, 2), S16(d, 4), S16(d, 6), self);
}

} // namespace hud

// ---- script text -------------------------------------------------------------------------

namespace {
// the game's 16-bit text: at most n - 1 characters, terminated (nothing for a null dst)
GAME_FN(void, __cdecl, CopyText16, 0x0068FFD0, void* dst, const void* src, int n);
} // namespace

namespace hud {

// 0x5539D0: Big/SmallText_ForScript taken down (gone a second ago)
void __fastcall HUDScriptText_Clear(HUDScriptText* self, re::Edx) {
    VCall<void>(self, SLOT_SET_VISIBLE, 0);
    self->text[0] = 0;
    self->numParams = 0;
    self->shown = 0;
    self->endTime = float(double(g_timeInMilliseconds) / 1000.0 - 1.0);
}

// 0x53A880: a line for `seconds` (the one up is taken down first)
void __fastcall HUDScriptText_Show(HUDScriptText* self, re::Edx, const char* text, float seconds) {
    if (self->shown) {
        CallThis<0x005539D0>(self);
    }
    std::strncpy(self->text, text, 0x7F);
    self->endTime = float(double(g_timeInMilliseconds) / 1000.0 + double(seconds));
    VCall<void>(self, SLOT_SET_VISIBLE, 1);
    self->shown = 1;
}

// 0x553BA0: a float param (4 bytes as they come)
void __fastcall HUDScriptText_AddFloat(HUDScriptText* self, re::Edx, float value) {
    if (self->shown) {
        CallThis<0x005539D0>(self);
    }
    if (self->numParams < 3) {
        self->params[self->numParams].type = 2;
        std::memcpy(self->params[self->numParams].value, &value, 4);
        self->numParams++;
    }
}

// 0x53A820: a 16-bit text param
void __fastcall HUDScriptText_AddText(HUDScriptText* self, re::Edx, const uint16_t* text) {
    if (self->shown) {
        CallThis<0x005539D0>(self);
    }
    if (self->numParams < 3) {
        self->params[self->numParams].type = 4;
        CopyText16(self->params[self->numParams].value, text, 0x7F);
        self->numParams++;
    }
}

// 0x55B020: Text's slot gets a float param (only while it has no text; no count check)
void __fastcall HUDText_AddFloat(HUDText* self, re::Edx, int slot, float value) {
    HUDText::Slot& s = self->slots[slot];
    if (s.text[0] == 0) {
        s.params[s.numParams].type = 2;
        std::memcpy(s.params[s.numParams].value, &value, 4);
        s.numParams++;
    }
}

// 0x55B070: ... a string param
void __fastcall HUDText_AddString(HUDText* self, re::Edx, int slot, const char* text) {
    HUDText::Slot& s = self->slots[slot];
    if (s.text[0] == 0) {
        s.params[s.numParams].type = 3;
        std::strncpy(reinterpret_cast<char*>(s.params[s.numParams].value), text, 0x7F);
        s.numParams++;
    }
}

// 0x55B0D0: ... a 16-bit text param
void __fastcall HUDText_AddText(HUDText* self, re::Edx, int slot, const uint16_t* text) {
    HUDText::Slot& s = self->slots[slot];
    if (s.text[0] == 0) {
        s.params[s.numParams].type = 4;
        CopyText16(s.params[s.numParams].value, text, 0x7F);
        s.numParams++;
    }
}

// 0x55B130: the slot's text (none: the slot freed; kept when it has one)
void __fastcall HUDText_SetText(HUDText* self, re::Edx, int slot, const char* text) {
    HUDText::Slot& s = self->slots[slot];
    if (!text || text[0] == 0) {
        s.text[0] = 0;
    } else if (s.text[0] == 0) {
        std::strncpy(s.text, text, 0x7F);
    }
}

// 0x55B180
void __fastcall HUDText_ClearAll(HUDText* self, re::Edx) {
    for (auto& slot : self->slots) {
        slot.numParams = 0;
        slot.text[0] = 0;
    }
}

// 0x5400B0
void __fastcall HUDGenericText_ClearLine(HUDGenericText* self, re::Edx, uint32_t line) {
    if (line < 5) {
        self->lines[line].startTime = 0;
        self->lines[line].text[0] = 0;
    }
}

// 0x540250: a line in the first free one, started `seconds` ago (negative: to come).
// With no id it returns the line (-1: all taken); with one, -1 whatever happens.
int __fastcall HUDGenericText_AddLine(HUDGenericText* self, re::Edx, const uint16_t* text, float seconds, int id) {
    for (int i = 0; i < 5; i++) {
        HUDGenericText::Line& line = self->lines[i];
        if (line.text[0] == 0) {
            if (id != -1) {
                line.id = id;
            }
            // (truncated through a 64-bit integer)
            line.startTime = g_timeInMilliseconds - uint32_t(int64_t(double(seconds) * -1000.0));
            CopyText16(line.text, text, 0xFF);
            return id == -1 ? i : -1;
        }
    }
    return -1;
}

// ---- small ones by kind -------------------------------------------------------------------

// 0x53D650: the detention text for 3 seconds (unless one's up and not forced)
void __fastcall HUDDetention_SetText(HUDDetention* self, re::Edx, const char* text, int value, int force) {
    if (!text) {
        return;
    }
    if (!re::Flag(force)) {
        float now = TimerSecondsMul();
        if (now < self->textEndTime) {
            return;
        }
    }
    std::strncpy(self->text, text, 0x1F);
    self->value = value;
    float now = TimerSecondsMul();
    self->textEndTime = float(double(now) + 3.0);
    // (then an empty function, 0x49A650, with 0)
}

// 0x53F120
void __fastcall HUDGenericCounter_SetA4A8(HUDGenericCounter* self, re::Edx, float a4, float a8) {
    self->unknownA4 = a4;
    self->unknownA8 = a8;
}

// 0x53F1C0: up by n, no further than the max
void __fastcall HUDGenericCounter_Add(HUDGenericCounter* self, re::Edx, int n) {
    self->previousValue = self->value;
    self->value = self->value + n;
    if (self->max < self->value) {
        self->value = self->max;
    }
}

// 0x53F1E0
void __fastcall HUDGenericCounter_SetMax(HUDGenericCounter* self, re::Edx, int max) {
    self->previousMax = self->max;
    self->max = max;
}

// 0x53F1F0
void __fastcall HUDGenericCounter_SetValue(HUDGenericCounter* self, re::Edx, int value) {
    self->previousValue = self->value;
    self->value = value;
}

} // namespace hud

namespace {
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void*, __thiscall, FindTextureAnywhere, 0x006AB310, void* missions, const char* name, const char* mask);
GAME_FN(void, __cdecl, ReleaseRef, 0x004657E0, void* object, re::Ptr32<void>* ref);  // (TrackOrCondition's)
GAME_FN(void, __thiscall, RadarSlot_Destruct, 0x0052C6D0, void* slot);
GAME_ARRAY(uint8_t, g_mapLegend, 0x00AB733C);  // [10] 8 bytes each: two flags first
GAME_VAR(uint32_t, g_radarIsTypeGuard, 0x00CF0A04);
GAME_VAR(int16_t, g_radarIsTypeId, 0x00CF0A00);

// (the index masked as the game does, top bits kept; its address wraps in 32 bits)
HUDRadarBlip* BlipAt(HUDRadar* radar, uint32_t index) {
    return reinterpret_cast<HUDRadarBlip*>(reinterpret_cast<uintptr_t>(radar->blips) + uintptr_t(uint32_t(index * 0x30u)));
}
// The blip a handle names, if it's still that one's
HUDRadarBlip* BlipFromHandle(HUDRadar* radar, uint32_t handle) {
    if (handle == 0xFFFFFFFFu) {
        return nullptr;
    }
    uint32_t index = handle & 0xF0000FFFu;
    HUDRadarBlip* blip = BlipAt(radar, index);
    if ((uint32_t(blip->generation) << 12 | index) != handle || index == 0xFFFFFFFFu) {
        return nullptr;
    }
    return blip;
}
} // namespace

namespace hud {

// 0x53F200: its icon, from HudElems (else looked for elsewhere)
void __fastcall HUDGenericCounter_SetIcon(HUDGenericCounter* self, re::Edx, const char* name, const char* mask) {
    if (self->icon) {
        NiRelease(self->icon);
        self->icon = nullptr;
    }
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(Txd_FindTxdSlot("HudElems", 0));
    self->icon = Txd_FindTexture(name, mask);
    Txd_PopCurrentTxd();
    if (!self->icon) {
        self->icon = FindTextureAnywhere(reinterpret_cast<void*>(0x020C3CA0), name, mask);   // (TheMissions)
    }
}

// 0x53F270
void __fastcall HUDGenericCounter_ReleaseIcon(HUDGenericCounter* self, re::Edx) {
    if (self->icon) {
        NiRelease(self->icon);
        self->icon = nullptr;
    }
}

// 0x5467B0
void __fastcall HUDRacing_Clear60(HUDRacing* self, re::Edx) {
    self->unknown50[4] = 0;
    self->unknown68 = 0;
    self->unknown6C = 0;
}

// 0x5467C0
void __fastcall HUDRacing_Start(HUDRacing* self, re::Edx) {
    self->unknown50[4] = int32_t(g_timeInMilliseconds);
    self->unknown68 = 0;
    self->unknown6C = 0;
    self->unknown70 = 0;
    self->unknown74 = 0;
}

// 0x546970: at most every 10 seconds
void __fastcall HUDRacing_Set6C(HUDRacing* self, re::Edx, int value) {
    if (self->unknown68 + 10000 < g_timeInMilliseconds) {
        self->unknown68 = g_timeInMilliseconds;
        self->unknown6C = value;
    }
}

// 0x546990
void __fastcall HUDRacing_Set70(HUDRacing* self, re::Edx, int value) {
    self->unknown70 = value;
    self->unknown74 = g_timeInMilliseconds;
}

// 0x545260
void __fastcall HUDPhoto_Set39(HUDPhoto* self, re::Edx, int flag, int a, int b) {
    self->unknown39 = uint8_t(flag);
    self->unknown44 = g_timeInMilliseconds;
    self->unknown7C = a;
    self->unknown80 = b;
}

// 0x545290: +0x39 for 300 ms after it's set
uint8_t __fastcall HUDPhoto_Get39(HUDPhoto* self, re::Edx) {
    if (g_timeInMilliseconds - self->unknown44 < 300) {
        return self->unknown39;
    }
    return 0;
}

// 0x542FB0: a line's text (strcpy)
void __fastcall HUDMonitor_SetLineText(HUDMonitor* self, re::Edx, int line, const char* text) {
    std::strcpy(self->lines[uint8_t(line)].text, text);
}

// 0x542FE0
void __fastcall HUDMonitor_SetLineFlag(HUDMonitor* self, re::Edx, int line, int flag) {
    self->lines[uint8_t(line)].unknown19 = uint8_t(flag);
}

// 0x53CE30
int __fastcall HUDMonitor_LookUpName(HUDComponent*, re::Edx, const char* name) {
    return HUDMonitor_LookUp(name);
}

// 0x55B6D0 (vf12)
void __fastcall HUDTutorial_OnHide(HUDTutorial* self, re::Edx) {
    self->unknown3C = 0;
}

// 0x55B720
void __fastcall HUDTutorial_Set38(HUDTutorial* self, re::Edx, int value) {
    self->unknown34 = 0;
    self->unknown38 = value < 0 ? -1 : value;
}

// 0x553C20 (vf5)
void __fastcall HUDSocial_SetVisible(HUDSocial* self, re::Edx, int visible) {
    HUDComponent_SetVisible(self, visible);
    if (!re::Flag(visible)) {
        self->unknown40 = 0;
    }
}

// 0x557270 (vf5): hidden: Reset too
void __fastcall HUDStruggleButton_SetVisible(HUDStruggleButton* self, re::Edx, int visible) {
    HUDComponent_SetVisible(self, visible);
    if (!re::Flag(visible)) {
        VCall<void>(self, 7);
    }
}

// 0x5514A0 (vf5)
void __fastcall HUDSecondaryHealthBar_SetVisible(HUDSecondaryHealthBar* self, re::Edx, int visible) {
    HUDComponent_SetVisible(self, visible);
    if (!re::Flag(visible)) {
        self->unknownB8[3] = 0;  // +0xBB
        self->unknownB8[5] = 0;  // +0xBD
    }
}

// 0x5514D0: the first label (0x40 characters at +0xBE)
void __fastcall HUDSecondaryHealthBar_SetLabel(HUDSecondaryHealthBar* self, re::Edx, const char* text) {
    self->unknownB8[0] = 0;  // +0xB8
    if (text) {
        self->unknownB8[0] = 1;
        std::strncpy(reinterpret_cast<char*>(self->unknownB8 + 6), text, 0x40);
    }
}

// 0x551500: the second (at +0xFE)
void __fastcall HUDSecondaryHealthBar_SetLabel2(HUDSecondaryHealthBar* self, re::Edx, const char* text) {
    self->unknownB8[1] = 0;  // +0xB9
    if (text) {
        self->unknownB8[1] = 1;
        std::strncpy(reinterpret_cast<char*>(self->unknownB8 + 0x46), text, 0x40);
    }
}

// 0x551590: its three refs let go
void __fastcall HUDSecondaryHealthBar_Destruct(HUDSecondaryHealthBar* self, re::Edx) {
    self->vtable = Vtable(VT_HUDSecondaryHealthBar);
    if (self->unknownB4) {
        ReleaseRef(self->unknownB4, &self->unknownB4);
    }
    if (self->unknownB0) {
        ReleaseRef(self->unknownB0, &self->unknownB0);
    }
    if (self->unknownAC) {
        ReleaseRef(self->unknownAC, &self->unknownAC);
    }
    HUDComponent_Destruct(self);
}

// 0x551DE0
HUDComponent* __fastcall HUDSecondaryHealthBar_Delete(HUDSecondaryHealthBar* self, re::Edx, int flags) {
    CallThis<0x00551590>(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// ---- the radar's blips ---------------------------------------------------------------------

// 0x548A40
void __fastcall HUDRadar_SetBlipId(HUDRadar* self, re::Edx, uint32_t handle, int id) {
    if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
        blip->id = int16_t(id);
    }
}

// 0x548A80: the handle of the blip in use with an id (index 0 never counts)
uint32_t __fastcall HUDRadar_FindBlipById(HUDRadar* self, re::Edx, int id) {
    for (int i = 0; i < 300; i++) {
        HUDRadarBlip& blip = self->blips[i];
        if ((blip.flags & 2) && blip.id == id) {
            if (i < 1) {
                return 0xFFFFFFFFu;
            }
            return uint32_t(blip.generation) << 12 | uint32_t(i);
        }
    }
    return 0xFFFFFFFFu;
}

// 0x548AD0: a blip on this entity of type 2, 6 or 13
uint8_t __fastcall HUDRadar_HasEntityBlip(HUDRadar* self, re::Edx, int entity) {
    for (uint32_t i = 0; i < 300; i++) {
        HUDRadarBlip& blip = self->blips[i];
        if (blip.entity == entity && (blip.type == 2 || blip.type == 0xD || blip.type == 6)) {
            return 1;
        }
    }
    return 0;
}

// 0x548B10 / 0x548B60 / 0x548C30: a flag bit of the blip
void __fastcall HUDRadar_SetBlipFlag8(HUDRadar* self, re::Edx, uint32_t handle, int on) {
    if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
        blip->flags ^= uint8_t((uint8_t(on) * 8) ^ blip->flags) & 8;
    }
}
void __fastcall HUDRadar_SetBlipFlag10(HUDRadar* self, re::Edx, uint32_t handle, int on) {
    if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
        blip->flags ^= uint8_t((uint8_t(on) << 4) ^ blip->flags) & 0x10;
    }
}
void __fastcall HUDRadar_SetBlipFlag4(HUDRadar* self, re::Edx, uint32_t handle, int on) {
    if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
        blip->flags ^= uint8_t((uint8_t(on) * 4) ^ blip->flags) & 4;
    }
}

// 0x548BB0
void __fastcall HUDRadar_SetBlip1C(HUDRadar* self, re::Edx, uint32_t handle, float value) {
    if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
        blip->unknown1C = value;
    }
}

// 0x548BF0
void __fastcall HUDRadar_SetBlip2E(HUDRadar* self, re::Edx, uint32_t handle, int value) {
    if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
        blip->unknown2E = uint8_t(value);
    }
}

// 0x548C80
void __fastcall HUDRadar_SetBlip28(HUDRadar* self, re::Edx, uint32_t handle, int value) {
    if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
        blip->unknown28 = value;
    }
}

// 0x54A9C0
void __fastcall HUDRadar_SetBlipPosition(HUDRadar* self, re::Edx, uint32_t handle, const float* pos) {
    if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
        float y = pos[1];
        blip->x = pos[0];
        blip->y = y;
    }
}

// 0x548DE0: nothing selected, and the blip stops flashing (the index test signed here)
void __fastcall HUDRadar_StopBlipFlashing(HUDRadar* self, re::Edx, uint32_t handle) {
    self->selectedBlip = -1;
    self->selectedUntil = 0;
    if (handle != 0xFFFFFFFFu) {
        uint32_t index = handle & 0xF0000FFFu;
        HUDRadarBlip* blip = BlipAt(self, index);
        if ((uint32_t(blip->generation) << 12 | index) == handle && int32_t(index) >= 0) {
            blip->flashUntil = 0;
        }
    }
}

// 0x54ABB0: flashes for 2.5 seconds
void __fastcall HUDRadar_FlashBlip(HUDRadar* self, re::Edx, uint32_t handle) {
    if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
        blip->flashUntil = g_timeInMilliseconds + 2500;
    }
}

// 0x54AC00: every blip of a type flashes
void __fastcall HUDRadar_FlashBlipsOfType(HUDRadar* self, re::Edx, int value) {
    // (the game compares the blip's +0x28, not its type at +0x20: tools/diffsweep, 2026-10-30)
    for (auto& blip : self->blips) {
        if (blip.unknown28 == value) {
            blip.flashUntil = g_timeInMilliseconds + 2500;
        }
    }
}

// 0x54C200: selected for 12.5 seconds, flashing
void __fastcall HUDRadar_SelectBlip(HUDRadar* self, re::Edx, uint32_t handle) {
    int32_t index = -1;
    if (handle != 0xFFFFFFFFu) {
        uint32_t i = handle & 0xF0000FFFu;
        if ((uint32_t(BlipAt(self, i)->generation) << 12 | i) == handle) {
            index = int32_t(i);
        }
    }
    self->selectedBlip = index;
    if (index >= 0) {
        self->selectedUntil = g_timeInMilliseconds + 12500;
        if (HUDRadarBlip* blip = BlipFromHandle(self, handle)) {
            blip->flashUntil = g_timeInMilliseconds + 2500;
        }
    }
}

// 0x548E30: (thiscall, the radar unused)
void __fastcall HUDRadar_SetLegendShown(HUDRadar*, re::Edx, uint32_t index, int shown) {
    if (index < 10) {
        g_mapLegend[index * 8 + 1] = uint8_t(shown);
        g_mapLegend[index * 8] = uint8_t(shown);
    }
}

// 0x548E50
re::Ptr32<void>* __fastcall HUDRadar_GetSlot3A98(HUDRadar* self, re::Edx, int index) {
    return &self->unknown3A98[index];
}

// 0x548E60: a texture slot from one of its three tables
int32_t __fastcall HUDRadar_GetTxdSlot(HUDRadar* self, re::Edx, int index, int first) {
    if (re::Flag(first)) {
        return self->unknown3A54[index];
    }
    if (self->unknownE828 == 0) {
        return self->unknown3A58[index >> 1];
    }
    return self->unknown3A5C[index >> 1];
}

// 0x548F60
uint8_t __fastcall HUDRadar_IsE734Set(HUDRadar* self, re::Edx) {
    if (self->mode == 2 || self->mode == 1) {
        return 0;
    }
    return self->unknownE734 != 0;
}

// 0x548F80
void __fastcall HUDRadar_SetFlag(HUDRadar* self, re::Edx, uint32_t bit, int on) {
    if (bit <= 0x26) {
        uint32_t mask = 1u << bit;
        if (re::Flag(on)) {
            self->unknownE82C |= mask;
        } else {
            self->unknownE82C &= ~mask;
        }
    }
}

// 0x549110 (vf3): its own id, or HUDComponent's
uint8_t __fastcall HUDRadar_IsType(HUDRadar*, re::Edx, int id) {
    if (!(g_radarIsTypeGuard & 1)) {
        g_radarIsTypeGuard |= 1;
        g_radarIsTypeId = g_nextTypeId;
        g_nextTypeId = int16_t(g_nextTypeId + 1);
    }
    if (int16_t(id) == g_radarIsTypeId) {
        return 1;
    }
    if (!(g_isTypeGuard & 1)) {
        g_isTypeGuard |= 1;
        g_isTypeId = g_nextTypeId;
        g_nextTypeId = int16_t(g_nextTypeId + 1);
    }
    return int16_t(id) == g_isTypeId;
}

// 0x5490C0: HUDRadar's destructor (its deleting one is 0x54B3D0)
void __fastcall HUDRadar_Destruct(HUDRadar* self, re::Edx) {
    self->vtable = Vtable(0x0091A810);
    for (int i = 38; i >= 0; i--) {
        RadarSlot_Destruct(&self->unknown3A98[i]);
    }
    for (int i = 13; i >= 0; i--) {
        RadarSlot_Destruct(&self->unknown3A60[i]);
    }
    HUDComponent_Destruct(self);
}

// 0x54B3D0
HUDComponent* __fastcall HUDRadar_Delete(HUDRadar* self, re::Edx, int flags) {
    CallThis<0x005490C0>(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x53B2A0
HUDComponent* __fastcall HUDButtonHistory_Delete(HUDButtonHistory* self, re::Edx, int flags) {
    CallThis<0x0053A900>(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

} // namespace hud

// ---- registration ---------------------------------------------------------------------------

#define HUD_REVERSED(name, address, fn)                                                              \
    static void* RE_CONCAT(hud_original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hud_original_, __LINE__), re::Mode::Original)

HUD_REVERSED("HUDComponent::HUDComponent", 0x0053CDC0, &hud::Construct);
HUD_REVERSED("HUDComponent::~HUDComponent", 0x0053CDF0, &hud::Destruct);
HUD_REVERSED("IArchivableBin::DeletingDestructor", 0x0053CF30, &hud::IArchivableBin_Delete);
HUD_REVERSED("HUDComponent::IsType", 0x00539090, &hud::IsType);
HUD_REVERSED("HUDComponent::SetVisible", 0x0053CEB0, &hud::SetVisible);
HUD_REVERSED("HUDComponent::SetDrawState", 0x005390E0, &hud::SetDrawState);
HUD_REVERSED("HUDComponent::Reset", 0x0053CF00, &hud::Reset);
HUD_REVERSED("HUDComponent::Render", 0x0053CF50, &hud::Render);
HUD_REVERSED("HUDComponent::SetAlignment", 0x0053CE00, &hud::SetAlignment);
HUD_REVERSED("HUDComponent::SetTextJustify", 0x0053CE40, &hud::SetTextJustify);
HUD_REVERSED("GetHUDComponentTypeName", 0x0053D010, &hud::GetHUDComponentTypeName);
HUD_REVERSED("CreateHUDComponent", 0x0053D030, &hud::CreateHUDComponent);
HUD_REVERSED("HUD2DSigns::DeletingDestructor", 0x00539210, &hud::DeletingDestructor<VT_HUD2DSigns>);
HUD_REVERSED("HUDBarber::DeletingDestructor", 0x0053A550, &hud::DeletingDestructor<VT_HUDBarber>);
HUD_REVERSED("HUDBigScriptText::DeletingDestructor", 0x0053A6A0, &hud::DeletingDestructor<VT_HUDBigScriptText>);
HUD_REVERSED("HUDClockWarnings::DeletingDestructor", 0x0053CBB0, &hud::DeletingDestructor<VT_HUDClockWarnings>);
HUD_REVERSED("HUDClothing::DeletingDestructor", 0x0053CC80, &hud::DeletingDestructor<VT_HUDClothing>);
HUD_REVERSED("HUDDetention::DeletingDestructor", 0x0053E1E0, &hud::DeletingDestructor<VT_HUDDetention>);
HUD_REVERSED("HUDDiary::DeletingDestructor", 0x0053E370, &hud::DeletingDestructor<VT_HUDDiary>);
HUD_REVERSED("HUDEntityHealth::DeletingDestructor", 0x0053E880, &hud::DeletingDestructor<VT_HUDEntityHealth>);
HUD_REVERSED("HUDGenericCounter::DeletingDestructor", 0x0053F6F0, &hud::DeletingDestructor<VT_HUDGenericCounter>);
HUD_REVERSED("HUDGenericText::DeletingDestructor", 0x005403F0, &hud::DeletingDestructor<VT_HUDGenericText>);
HUD_REVERSED("HUDHealthMeter::DeletingDestructor", 0x00540DF0, &hud::DeletingDestructor<VT_HUDHealthMeter>);
HUD_REVERSED("HUDInventory::DeletingDestructor", 0x005412F0, &hud::DeletingDestructor<VT_HUDInventory>);
HUD_REVERSED("HUDMissionTimer::DeletingDestructor", 0x005427E0, &hud::DeletingDestructor<VT_HUDMissionTimer>);
HUD_REVERSED("HUDMoney::DeletingDestructor", 0x00542B90, &hud::DeletingDestructor<VT_HUDMoney>);
HUD_REVERSED("HUDMonitor::DeletingDestructor", 0x00543000, &hud::DeletingDestructor<VT_HUDMonitor>);
HUD_REVERSED("HUDNewspaper::DeletingDestructor", 0x005435D0, &hud::DeletingDestructor<VT_HUDNewspaper>);
HUD_REVERSED("HUDPhoto::DeletingDestructor", 0x00544480, &hud::DeletingDestructor<VT_HUDPhoto>);
HUD_REVERSED("HUDRacing::DeletingDestructor", 0x00546A00, &hud::DeletingDestructor<VT_HUDRacing>);
HUD_REVERSED("HUDScreenFade::DeletingDestructor", 0x00551470, &hud::DeletingDestructor<VT_HUDScreenFade>);
HUD_REVERSED("HUDSmallScriptText::DeletingDestructor", 0x00553A20, &hud::DeletingDestructor<VT_HUDSmallScriptText>);
HUD_REVERSED("HUDSocial::DeletingDestructor", 0x00553CE0, &hud::DeletingDestructor<VT_HUDSocial>);
HUD_REVERSED("HUDSocialApologize::DeletingDestructor", 0x00555080, &hud::DeletingDestructor<VT_HUDSocialApologize>);
HUD_REVERSED("HUDSocialBurn::DeletingDestructor", 0x00556980, &hud::DeletingDestructor<VT_HUDSocialBurn>);
HUD_REVERSED("HUDSocialDismiss::DeletingDestructor", 0x005563D0, &hud::DeletingDestructor<VT_HUDSocialDismiss>);
HUD_REVERSED("HUDSocialErrands::DeletingDestructor", 0x005571F0, &hud::DeletingDestructor<VT_HUDSocialErrands>);
HUD_REVERSED("HUDSocialFlirt::DeletingDestructor", 0x005554D0, &hud::DeletingDestructor<VT_HUDSocialFlirt>);
HUD_REVERSED("HUDSocialFollow::DeletingDestructor", 0x005558B0, &hud::DeletingDestructor<VT_HUDSocialFollow>);
HUD_REVERSED("HUDSocialGift::DeletingDestructor", 0x005560D0, &hud::DeletingDestructor<VT_HUDSocialGift>);
HUD_REVERSED("HUDSocialGreet::DeletingDestructor", 0x00556690, &hud::DeletingDestructor<VT_HUDSocialGreet>);
HUD_REVERSED("HUDSocialHumiliate::DeletingDestructor", 0x00555D70, &hud::DeletingDestructor<VT_HUDSocialHumiliate>);
HUD_REVERSED("HUDSocialJoin::DeletingDestructor", 0x00556530, &hud::DeletingDestructor<VT_HUDSocialJoin>);
HUD_REVERSED("HUDSocialKiss::DeletingDestructor", 0x00555750, &hud::DeletingDestructor<VT_HUDSocialKiss>);
HUD_REVERSED("HUDSocialShove::DeletingDestructor", 0x00555B50, &hud::DeletingDestructor<VT_HUDSocialShove>);
HUD_REVERSED("HUDSocialSmoothTalk::DeletingDestructor", 0x00555370, &hud::DeletingDestructor<VT_HUDSocialSmoothTalk>);
HUD_REVERSED("HUDSocialStop::DeletingDestructor", 0x00555EC0, &hud::DeletingDestructor<VT_HUDSocialStop>);
HUD_REVERSED("HUDSocialTaunt::DeletingDestructor", 0x00555A00, &hud::DeletingDestructor<VT_HUDSocialTaunt>);
HUD_REVERSED("HUDStruggleButton::DeletingDestructor", 0x00557290, &hud::DeletingDestructor<VT_HUDStruggleButton>);
HUD_REVERSED("HUDTaggingMeter::DeletingDestructor", 0x0055A3D0, &hud::DeletingDestructor<VT_HUDTaggingMeter>);
HUD_REVERSED("HUDTasks::DeletingDestructor", 0x0055A540, &hud::DeletingDestructor<VT_HUDTasks>);
HUD_REVERSED("HUDTattoo::DeletingDestructor", 0x0055AF50, &hud::DeletingDestructor<VT_HUDTattoo>);
HUD_REVERSED("HUDText::DeletingDestructor", 0x0055B1B0, &hud::DeletingDestructor<VT_HUDText>);
HUD_REVERSED("HUDTutorial::DeletingDestructor", 0x0055B770, &hud::DeletingDestructor<VT_HUDTutorial>);
HUD_REVERSED("HUDWeapons::DeletingDestructor", 0x0055CC20, &hud::DeletingDestructor<VT_HUDWeapons>);
HUD_REVERSED("HUDClothing::HUDClothing", 0x0053CC40, &hud::PlainConstructor<VT_HUDClothing>);
HUD_REVERSED("HUDBarber::HUDBarber", 0x0053A4B0, &hud::PlainConstructor<VT_HUDBarber>);
HUD_REVERSED("HUDTattoo::HUDTattoo", 0x0055AEC0, &hud::PlainConstructor<VT_HUDTattoo>);
HUD_REVERSED("HUDText::HUDText", 0x0055AFC0, &hud::PlainConstructor<VT_HUDText>);
HUD_REVERSED("HUDScreenFade::HUDScreenFade", 0x00551430, &hud::PlainConstructor<VT_HUDScreenFade>);
HUD_REVERSED("HUDSocialApologize::HUDSocialApologize", 0x00555060, &hud::PlainConstructor<VT_HUDSocialApologize>);
HUD_REVERSED("HUDSocialSmoothTalk::HUDSocialSmoothTalk", 0x00555350, &hud::PlainConstructor<VT_HUDSocialSmoothTalk>);
HUD_REVERSED("HUDSocialFlirt::HUDSocialFlirt", 0x005554B0, &hud::PlainConstructor<VT_HUDSocialFlirt>);
HUD_REVERSED("HUDSocialKiss::HUDSocialKiss", 0x00555730, &hud::PlainConstructor<VT_HUDSocialKiss>);
HUD_REVERSED("HUDSocialFollow::HUDSocialFollow", 0x00555890, &hud::PlainConstructor<VT_HUDSocialFollow>);
HUD_REVERSED("HUDSocialTaunt::HUDSocialTaunt", 0x005559E0, &hud::PlainConstructor<VT_HUDSocialTaunt>);
HUD_REVERSED("HUDSocialShove::HUDSocialShove", 0x00555B30, &hud::PlainConstructor<VT_HUDSocialShove>);
HUD_REVERSED("HUDSocialHumiliate::HUDSocialHumiliate", 0x00555D50, &hud::PlainConstructor<VT_HUDSocialHumiliate>);
HUD_REVERSED("HUDSocialStop::HUDSocialStop", 0x00555EA0, &hud::PlainConstructor<VT_HUDSocialStop>);
HUD_REVERSED("HUDSocialDismiss::HUDSocialDismiss", 0x005563B0, &hud::PlainConstructor<VT_HUDSocialDismiss>);
HUD_REVERSED("HUDSocialJoin::HUDSocialJoin", 0x00556510, &hud::PlainConstructor<VT_HUDSocialJoin>);
HUD_REVERSED("HUDSocialBurn::HUDSocialBurn", 0x00556960, &hud::PlainConstructor<VT_HUDSocialBurn>);
HUD_REVERSED("HUDBigScriptText::HUDBigScriptText", 0x0053A8E0, (&hud::ResetConstructor<VT_HUDBigScriptText, 0x00553A50>));
HUD_REVERSED("HUDSmallScriptText::HUDSmallScriptText", 0x00553BF0, (&hud::ResetConstructor<VT_HUDSmallScriptText, 0x00553A50>));
HUD_REVERSED("HUDGenericText::HUDGenericText", 0x005400D0, (&hud::ResetConstructor<VT_HUDGenericText, 0x00540070>));
HUD_REVERSED("HUDMoney::HUDMoney", 0x00542B70, (&hud::ResetConstructor<VT_HUDMoney, 0x00542830>));
HUD_REVERSED("HUDTasks::HUDTasks", 0x0055AA20, (&hud::ResetConstructor<VT_HUDTasks, 0x0055A570>));
HUD_REVERSED("HUDDetention::HUDDetention", 0x0053D610, &hud::HUDDetention_Construct);
HUD_REVERSED("HUDMissionTimer::HUDMissionTimer", 0x005427B0, &hud::HUDMissionTimer_Construct);
HUD_REVERSED("HUDHealthMeter::HUDHealthMeter", 0x00540440, &hud::HUDHealthMeter_Construct);
HUD_REVERSED("HUDEntityHealth::HUDEntityHealth", 0x0053E7F0, &hud::HUDEntityHealth_Construct);
HUD_REVERSED("HUDNewspaper::HUDNewspaper", 0x00543B00, &hud::HUDNewspaper_Construct);
HUD_REVERSED("HUDGenericCounter::HUDGenericCounter", 0x0053F6B0, &hud::HUDGenericCounter_Construct);
HUD_REVERSED("HUDRacing::HUDRacing", 0x005469B0, &hud::HUDRacing_Construct);
HUD_REVERSED("HUDSecondaryHealthBar::HUDSecondaryHealthBar", 0x00553970, &hud::HUDSecondaryHealthBar_Construct);
HUD_REVERSED("HUDClockWarnings::HUDClockWarnings", 0x0053CB70, &hud::HUDClockWarnings_Construct);
HUD_REVERSED("HUDPhoto::HUDPhoto", 0x00543D70, &hud::HUDPhoto_Construct);
HUD_REVERSED("HUDStruggleButton::HUDStruggleButton", 0x00557BB0, &hud::HUDStruggleButton_Construct);
HUD_REVERSED("HUDWeapons::HUDWeapons", 0x0055DE70, &hud::HUDWeapons_Construct);
HUD_REVERSED("HUDButtonHistory::HUDButtonHistory", 0x0053B7F0, &hud::HUDButtonHistory_Construct);
HUD_REVERSED("HUDSocial::HUDSocial", 0x00553C40, &hud::HUDSocial_Construct);
HUD_REVERSED("HUDSocialGift::HUDSocialGift", 0x00555FF0, &hud::HUDSocialGift_Construct);
HUD_REVERSED("HUDSocialGreet::HUDSocialGreet", 0x00556670, &hud::HUDSocialGreet_Construct);
HUD_REVERSED("HUDSocialErrands::HUDSocialErrands", 0x00556CC0, &hud::HUDSocialErrands_Construct);
HUD_REVERSED("HUDTaggingMeter::HUDTaggingMeter", 0x00559A90, &hud::HUDTaggingMeter_Construct);
HUD_REVERSED("HUDMonitor::HUDMonitor", 0x005434E0, &hud::HUDMonitor_Construct);
HUD_REVERSED("HUDTutorial::HUDTutorial", 0x0055B7A0, &hud::HUDTutorial_Construct);
HUD_REVERSED("HUD2DSigns::HUD2DSigns", 0x00539DC0, &hud::HUD2DSigns_Construct);
HUD_REVERSED("HUDDiary::HUDDiary", 0x0053E340, &hud::HUDDiary_Construct);
HUD_REVERSED("HUDInventory::HUDInventory", 0x005412B0, &hud::HUDInventory_Construct);
HUD_REVERSED("HUD2DSigns::Reset", 0x005390F0, &hud::HUD2DSigns_Reset);
HUD_REVERSED("HUDDetention::Reset", 0x0053D5E0, &hud::HUDDetention_Reset);
HUD_REVERSED("HUDEntityHealth::Reset", 0x0053E7D0, &hud::ResetShown);
HUD_REVERSED("HUDDiary::Reset", 0x0053E210, &hud::HUDDiary_Reset);
HUD_REVERSED("HUDGenericCounter::Reset", 0x0053F290, &hud::HUDGenericCounter_Reset);
HUD_REVERSED("HUDGenericText::Reset", 0x00540070, &hud::HUDGenericText_Reset);
HUD_REVERSED("HUDHealthMeter::Reset", 0x00540420, &hud::HUDHealthMeter_Reset);
HUD_REVERSED("HUDInventory::Reset", 0x00540E20, &hud::HUDInventory_Reset);
HUD_REVERSED("HUDMissionTimer::Reset", 0x00541F30, &hud::HUDMissionTimer_Reset);
HUD_REVERSED("HUDMoney::Reset", 0x00542830, &hud::HUDMoney_Reset);
HUD_REVERSED("HUDNewspaper::Reset", 0x005435B0, &hud::HUDNewspaper_Reset);
HUD_REVERSED("HUDRacing::Reset", 0x00546780, &hud::HUDRacing_Reset);
HUD_REVERSED("HUDScreenFade::Reset", 0x00551450, &hud::HUDScreenFade_Reset);
HUD_REVERSED("HUDSmallScriptText::Reset", 0x00553A50, &hud::HUDScriptText_Reset);
HUD_REVERSED("HUDSocial::Reset", 0x00553C10, &hud::HUDSocial_Reset);
HUD_REVERSED("HUDSocialErrands::Reset", 0x00556C00, &hud::HUDSocialErrands_Reset);
HUD_REVERSED("HUDStruggleButton::Reset", 0x00557220, &hud::HUDStruggleButton_Reset);
HUD_REVERSED("HUDTaggingMeter::Reset", 0x005586F0, &hud::HUDTaggingMeter_Reset);
HUD_REVERSED("HUDTasks::Reset", 0x0055A570, &hud::HUDTasks_Reset);
HUD_REVERSED("HUDText::Reset", 0x0055AFE0, &hud::HUDText_Reset);
HUD_REVERSED("HUDTutorial::Reset", 0x0055B610, &hud::HUDTutorial_Reset);
HUD_REVERSED("HUDSmallScriptText::Setup", 0x0053A610, &hud::HUDScriptText_Setup);
HUD_REVERSED("HUDClockWarnings::Setup", 0x0053C9B0, &hud::HUDClockWarnings_Setup);
HUD_REVERSED("HUDDetention::Setup", 0x0053E0D0, &hud::HUDDetention_Setup);
HUD_REVERSED("HUDHealthMeter::Setup", 0x00540470, &hud::HUDHealthMeter_Setup);
HUD_REVERSED("HUDDiary::Setup", 0x0053E3A0, &hud::HUDDiary_Setup);
HUD_REVERSED("HUDEntityHealth::Setup", 0x0053E810, &hud::HUDEntityHealth_Setup);
HUD_REVERSED("HUDGenericCounter::Setup", 0x0053F560, &hud::HUDGenericCounter_Setup);
HUD_REVERSED("HUDGenericText::Setup", 0x00540360, &hud::HUDGenericText_Setup);
HUD_REVERSED("HUDMissionTimer::Setup", 0x005426A0, &hud::HUDMissionTimer_Setup);
HUD_REVERSED("HUDMoney::Setup", 0x00542AC0, &hud::HUDMoney_Setup);
HUD_REVERSED("HUDMonitor::Setup", 0x00542E20, &hud::HUDMonitor_Setup);
HUD_REVERSED("HUDNewspaper::Setup", 0x00543990, &hud::HUDNewspaper_Setup);
HUD_REVERSED("HUDRacing::Setup", 0x005467E0, &hud::HUDRacing_Setup);
HUD_REVERSED("HUDSocial::Setup", 0x00555610, &hud::HUDSocialBase_Setup);
HUD_REVERSED("HUDSocialBurn::Setup", 0x00556AD0, &hud::HUDSocialBurn_Setup);
HUD_REVERSED("HUDSocialErrands::Setup", 0x005570C0, &hud::HUDSocialErrands_Setup);
HUD_REVERSED("HUDTutorial::Setup", 0x0055B630, &hud::HUDTutorial_Setup);
HUD_REVERSED("HUDSmallScriptText::Clear", 0x005539D0, &hud::HUDScriptText_Clear);
HUD_REVERSED("HUDBigScriptText::Show", 0x0053A880, &hud::HUDScriptText_Show);
HUD_REVERSED("HUDSmallScriptText::AddFloat", 0x00553BA0, &hud::HUDScriptText_AddFloat);
HUD_REVERSED("HUDBigScriptText::AddText", 0x0053A820, &hud::HUDScriptText_AddText);
HUD_REVERSED("HUDText::AddFloat", 0x0055B020, &hud::HUDText_AddFloat);
HUD_REVERSED("HUDText::AddString", 0x0055B070, &hud::HUDText_AddString);
HUD_REVERSED("HUDText::AddText", 0x0055B0D0, &hud::HUDText_AddText);
HUD_REVERSED("HUDText::SetText", 0x0055B130, &hud::HUDText_SetText);
HUD_REVERSED("HUDText::ClearAll", 0x0055B180, &hud::HUDText_ClearAll);
HUD_REVERSED("HUDGenericText::ClearLine", 0x005400B0, &hud::HUDGenericText_ClearLine);
HUD_REVERSED("HUDGenericText::AddLine", 0x00540250, &hud::HUDGenericText_AddLine);
HUD_REVERSED("HUDDetention::SetText", 0x0053D650, &hud::HUDDetention_SetText);
HUD_REVERSED("HUDGenericCounter::SetA4A8", 0x0053F120, &hud::HUDGenericCounter_SetA4A8);
HUD_REVERSED("HUDGenericCounter::Add", 0x0053F1C0, &hud::HUDGenericCounter_Add);
HUD_REVERSED("HUDGenericCounter::SetMax", 0x0053F1E0, &hud::HUDGenericCounter_SetMax);
HUD_REVERSED("HUDGenericCounter::SetValue", 0x0053F1F0, &hud::HUDGenericCounter_SetValue);
HUD_REVERSED("HUDGenericCounter::SetIcon", 0x0053F200, &hud::HUDGenericCounter_SetIcon);
HUD_REVERSED("HUDGenericCounter::ReleaseIcon", 0x0053F270, &hud::HUDGenericCounter_ReleaseIcon);
HUD_REVERSED("HUDRacing::Clear60", 0x005467B0, &hud::HUDRacing_Clear60);
HUD_REVERSED("HUDRacing::Start", 0x005467C0, &hud::HUDRacing_Start);
HUD_REVERSED("HUDRacing::Set6C", 0x00546970, &hud::HUDRacing_Set6C);
HUD_REVERSED("HUDRacing::Set70", 0x00546990, &hud::HUDRacing_Set70);
HUD_REVERSED("HUDPhoto::Set39", 0x00545260, &hud::HUDPhoto_Set39);
HUD_REVERSED("HUDPhoto::Get39", 0x00545290, &hud::HUDPhoto_Get39);
HUD_REVERSED("HUDMonitor::SetLineText", 0x00542FB0, &hud::HUDMonitor_SetLineText);
HUD_REVERSED("HUDMonitor::SetLineFlag", 0x00542FE0, &hud::HUDMonitor_SetLineFlag);
HUD_REVERSED("HUDMonitor::LookUpName", 0x0053CE30, &hud::HUDMonitor_LookUpName);
HUD_REVERSED("HUDTutorial::OnHide", 0x0055B6D0, &hud::HUDTutorial_OnHide);
HUD_REVERSED("HUDTutorial::Set38", 0x0055B720, &hud::HUDTutorial_Set38);
HUD_REVERSED("HUDSocial::SetVisible", 0x00553C20, &hud::HUDSocial_SetVisible);
HUD_REVERSED("HUDStruggleButton::SetVisible", 0x00557270, &hud::HUDStruggleButton_SetVisible);
HUD_REVERSED("HUDSecondaryHealthBar::SetVisible", 0x005514A0, &hud::HUDSecondaryHealthBar_SetVisible);
HUD_REVERSED("HUDSecondaryHealthBar::SetLabel", 0x005514D0, &hud::HUDSecondaryHealthBar_SetLabel);
HUD_REVERSED("HUDSecondaryHealthBar::SetLabel2", 0x00551500, &hud::HUDSecondaryHealthBar_SetLabel2);
HUD_REVERSED("HUDSecondaryHealthBar::~HUDSecondaryHealthBar", 0x00551590, &hud::HUDSecondaryHealthBar_Destruct);
HUD_REVERSED("HUDSecondaryHealthBar::DeletingDestructor", 0x00551DE0, &hud::HUDSecondaryHealthBar_Delete);
HUD_REVERSED("HUDRadar::SetBlipId", 0x00548A40, &hud::HUDRadar_SetBlipId);
HUD_REVERSED("HUDRadar::FindBlipById", 0x00548A80, &hud::HUDRadar_FindBlipById);
HUD_REVERSED("HUDRadar::HasEntityBlip", 0x00548AD0, &hud::HUDRadar_HasEntityBlip);
HUD_REVERSED("HUDRadar::SetBlipFlag8", 0x00548B10, &hud::HUDRadar_SetBlipFlag8);
HUD_REVERSED("HUDRadar::SetBlipFlag10", 0x00548B60, &hud::HUDRadar_SetBlipFlag10);
HUD_REVERSED("HUDRadar::SetBlipFlag4", 0x00548C30, &hud::HUDRadar_SetBlipFlag4);
HUD_REVERSED("HUDRadar::SetBlip1C", 0x00548BB0, &hud::HUDRadar_SetBlip1C);
HUD_REVERSED("HUDRadar::SetBlip2E", 0x00548BF0, &hud::HUDRadar_SetBlip2E);
HUD_REVERSED("HUDRadar::SetBlip28", 0x00548C80, &hud::HUDRadar_SetBlip28);
HUD_REVERSED("HUDRadar::SetBlipPosition", 0x0054A9C0, &hud::HUDRadar_SetBlipPosition);
HUD_REVERSED("HUDRadar::StopBlipFlashing", 0x00548DE0, &hud::HUDRadar_StopBlipFlashing);
HUD_REVERSED("HUDRadar::FlashBlip", 0x0054ABB0, &hud::HUDRadar_FlashBlip);
HUD_REVERSED("HUDRadar::FlashBlipsOfType", 0x0054AC00, &hud::HUDRadar_FlashBlipsOfType);
HUD_REVERSED("HUDRadar::SelectBlip", 0x0054C200, &hud::HUDRadar_SelectBlip);
HUD_REVERSED("HUDRadar::SetLegendShown", 0x00548E30, &hud::HUDRadar_SetLegendShown);
HUD_REVERSED("HUDRadar::GetSlot3A98", 0x00548E50, &hud::HUDRadar_GetSlot3A98);
HUD_REVERSED("HUDRadar::GetTxdSlot", 0x00548E60, &hud::HUDRadar_GetTxdSlot);
HUD_REVERSED("HUDRadar::IsE734Set", 0x00548F60, &hud::HUDRadar_IsE734Set);
HUD_REVERSED("HUDRadar::SetFlag", 0x00548F80, &hud::HUDRadar_SetFlag);
HUD_REVERSED("HUDRadar::IsType", 0x00549110, &hud::HUDRadar_IsType);
HUD_REVERSED("HUDRadar::~HUDRadar", 0x005490C0, &hud::HUDRadar_Destruct);
HUD_REVERSED("HUDRadar::DeletingDestructor", 0x0054B3D0, &hud::HUDRadar_Delete);
HUD_REVERSED("HUDButtonHistory::DeletingDestructor", 0x0053B2A0, &hud::HUDButtonHistory_Delete);
HUD_REVERSED("HUDButtonHistory::~HUDButtonHistory", 0x0053A900, &hud::HUDButtonHistory_Destruct);
HUD_REVERSED("HUDButtonHistory::Init", 0x0053B2C0, &hud::HUDButtonHistory_Init);
