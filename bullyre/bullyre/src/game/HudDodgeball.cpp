// HUDDodgeBall (HUD kind 22, 0x118 bytes, vtable 0x93E760; the one at *0xC674F8): the
// dodgeball game's message line and, while the game's forced pause is on, its controls
// help. 0x67DF10..0x67E1D0.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

struct CVector2D {
    float x, y;
};

// the layout's positions, in screen pixels
struct HUDDodgeBall : HUDComponent {
    int32_t x20;           // +0x20 (the layout's 1st position)
    int32_t x24;           // +0x24 (its 2nd)
    int32_t y28;           // +0x28 (1st)
    int32_t y2C;           // +0x2C (2nd)
    int32_t unknown30;     // +0x30 (the layout's 5th short)
    int32_t unknown34;     // +0x34 (its 6th, times the screen's y scale)
    int32_t unknown38;     // +0x38 (its 7th)
    int32_t y3C;           // +0x3C (a 3rd position's y)
    int32_t x40, y44;      // +0x40 (a 4th)
    int32_t messageX;      // +0x48
    int32_t messageY;      // +0x4C
    uint16_t message[100]; // +0x50 16-bit text (empty: none)
};
static_assert(offsetof(HUDDodgeBall, message) == 0x50);
static_assert(sizeof(HUDDodgeBall) == 0x118);

inline void* const kVtable = reinterpret_cast<void*>(0x0093E760);
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(HUDComponent*, __thiscall, HUDComponent_Construct, 0x0053CDC0, HUDComponent* self);
GAME_FN(void, __thiscall, HUDComponent_Destruct, 0x0053CDF0, HUDComponent* self);
GAME_FN(void, __thiscall, HUDComponent_SetAlignment, 0x0053CE00, HUDComponent* self, const char* h, const char* v);
GAME_FN(void, __cdecl, CopyText16, 0x0068FFD0, void* dst, const void* src, int n);
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
GAME_FN(col::CRect*, __cdecl, ScreenRect, 0x0052D650, col::CRect* out, float x, float y, float width, float height,
        int hAlign, int vAlign);
GAME_FN(CRGBA*, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const col::CRect* rect, const CRGBA* colour);
GAME_FN(void, __cdecl, SetRenderState_519790, 0x00519790, int state, int value);
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetStyleScale, 0x00688050, int style);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_FormatText, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
GAME_FN(void, __cdecl, HUD_SetPrompt, 0x005143D0, const char* text);
GAME_FN(void, __cdecl, HUD_SetPromptTimed, 0x00514420, const char* text, float time);
// the game (DodgeballGame.cpp)
GAME_FN(uint32_t*, __thiscall, Dodgeball_GetFlagsFn, 0x0067B0E0, uint8_t* game, uint32_t* out);
GAME_FN(uint32_t, __thiscall, Dodgeball_GetWinnerFn, 0x0067B1E0, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_ForcePauseFn, 0x0067B620, uint8_t* game, uint32_t paused);
GAME_FN(void, __thiscall, Dodgeball_TimesFn, 0x0067B480, uint8_t* game, uint32_t* out, int);

GAME_VAR(uint8_t*, dodgeballGame, 0x01B89150);
GAME_VAR(uint8_t*, tutorial_C67540, 0x00C67540);
GAME_VAR(float, screenScaleY, 0x00A957AC);
GAME_VAR(int32_t, language, 0x00C18724);
GAME_ARRAY(uint8_t, fontColour, 0x0208B764);
GAME_VAR(float, fontWrap_208B774, 0x0208B774);
GAME_VAR(int16_t, fontEdge_208B784, 0x0208B784);
GAME_VAR(int16_t, fontShadow_208B786, 0x0208B786);
GAME_VAR(int16_t, fontStyle_208B788, 0x0208B788);
GAME_ARRAY(int32_t, fontStyleLoaded, 0x0208B218);

void SetColour(const CRGBA& c) {
    std::memcpy(fontColour, &c, 4);
}

// a layout position (at shorts `x`, `y`, aligned by the names at `names`) to pixels
void Place(HUDDodgeBall* self, const uint8_t* data, int names, float x, float y, int32_t* outX, int32_t* outY) {
    HUDComponent_SetAlignment(self, reinterpret_cast<const char*>(data + names),
                              reinterpret_cast<const char*>(data + names + 8));
    CVector2D p;
    CVector2D* at = ScreenPosition(&p, x, y, self->hAlign, self->vAlign);
    float px = at->x;
    float py = at->y;
    if (outX) {
        *outX = int32_t(px);
    }
    *outY = int32_t(py);
}

// 0x67DF10 (thiscall)
HUDDodgeBall* __fastcall Construct(HUDDodgeBall* self, re::Edx) {
    HUDComponent_Construct(self);
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(kVtable);
    self->message[0] = 0;
    return self;
}

// 0x67DF30 (thiscall): the message (up to 99 characters; none: cleared)
void __fastcall SetText(HUDDodgeBall* self, re::Edx, const uint16_t* text) {
    if (text) {
        CopyText16(self->message, text, 0x63);
        return;
    }
    self->message[0] = 0;
}

// 0x67DF60 (vf0, thiscall): deleting destructor
HUDDodgeBall* __fastcall DeletingDestruct(HUDDodgeBall* self, re::Edx, uint8_t flags) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(kVtable);
    HUDComponent_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x67DF90 (vf1, thiscall): set up from the layout: shorts 0..11 (positions and values),
// alignment names from +0x18 (8 bytes each, two per position)
void __fastcall Setup(HUDDodgeBall* self, re::Edx, HUDLayoutRecord* record) {
    const uint8_t* data = record->data;
    const int16_t* s = reinterpret_cast<const int16_t*>(data);
    Place(self, data, 0x18, float(int32_t(s[0])), float(int32_t(s[1])), &self->x20, &self->y28);
    Place(self, data, 0x28, float(int32_t(s[2])), float(int32_t(s[3])), &self->x24, &self->y2C);
    self->unknown30 = s[4];
    self->unknown34 = int32_t(float(int32_t(s[5])) * screenScaleY);
    self->unknown38 = s[6];
    Place(self, data, 0x38, 0.0f, float(int32_t(s[7])), nullptr, &self->y3C);
    Place(self, data, 0x48, float(int32_t(s[8])), float(int32_t(s[9])), &self->x40, &self->y44);
    Place(self, data, 0x58, float(int32_t(s[10])), float(int32_t(s[11])), &self->messageX, &self->messageY);
}

// the controls help over a shaded box in the middle of the screen
void DrawHelp() {
    CRGBA heading;
    RGBA_Construct(&heading, 0xE6, 0xE6, 0xC8, 0xFF);
    CRGBA body;
    RGBA_Construct(&body, 0xC8, 0xC8, 0x64, 0xFF);
    col::CRect box;
    float* b = reinterpret_cast<float*>(&box);
    b[0] = 1000000.0f;   // (the rect's defaults, overwritten)
    b[3] = 1000000.0f;
    b[1] = -1000000.0f;
    b[2] = -1000000.0f;
    col::CRect screen;
    box = *ScreenRect(&screen, 0.0f, 0.0f, 500.0f, 375.0f, 1, 1);
    CVector2D centre;
    ScreenPosition(&centre, 0.0f, 0.0f, 1, 1);
    CRGBA shade;
    DrawRect(&box, RGBA_Construct(&shade, 0x10, 0x10, 0x10, 0x6E));
    Font_Begin();
    if (fontStyleLoaded[0]) {
        fontStyle_208B788 = 0;
    }
    Font_SetStyleScale(3);
    SetColour(heading);
    fontShadow_208B786 = 3;
    Font_PrintString(320.0f, 70.0f, Text_Get(kTheText, "DBH_Title"), 1);
    if (fontStyleLoaded[1]) {
        fontStyle_208B788 = 1;
        fontEdge_208B784 = 0;
    }
    Font_SetScale((language == 7 || language == 6) ? 0.5f : 1.0f);
    SetColour(body);
    uint16_t line[0x100];
    auto print = [&](const char* key, float x, float y) {
        Font_FormatText(line, Text_Get(kTheText, key), 0x100);
        Font_PrintString(x, y, line, 1);
    };
    print("DBH_TUT1", centre.x, centre.y - 120.0f);
    print("DBH_TUT2", centre.x, centre.y - 100.0f);
    print("DBH_TUT3", centre.x, centre.y - 80.0f);
    print("DBH_TUT4", centre.x, centre.y - 60.0f);
    print("DBH_TUT5", centre.x, centre.y - 40.0f);
    Font_Render();
    SetColour(heading);
    print("DBH_OFFENSE", centre.x - 100.0f, centre.y + 40.0f);
    SetColour(body);
    fontShadow_208B786 = 0;
    print("DBH_THROW", centre.x - 150.0f, centre.y + 70.0f);
    print("DBH_JUMP", centre.x - 150.0f, centre.y + 100.0f);
    print("DBH_PASS", centre.x - 150.0f, centre.y + 130.0f);
    Font_Render();
    fontWrap_208B774 = 640.0f;
    fontShadow_208B786 = 3;
    SetColour(heading);
    print("DBH_DEFENSE", centre.x + 100.0f, centre.y + 40.0f);
    SetColour(body);
    fontShadow_208B786 = 0;
    print("DBH_CATCH", centre.x + 40.0f, centre.y + 70.0f);
    print("DBH_DODGE", centre.x + 40.0f, centre.y + 100.0f);
    print("DBH_SWITCH", centre.x + 40.0f, centre.y + 130.0f);
    Font_Render();
    HUD_SetPromptTimed("DBH_BACK", 1.0f);
}

// 0x67E1D0 (vf8, thiscall): drawn: while the game's forced pause (0x400) is on, the
// controls help (a tutorial object answering its vfunc 4 ends the pause instead);
// otherwise the "DBH_Text1" prompt until there is a winner (then no prompt); the message
// in its colour at its place. Render states 6 / 8 / 12 are turned off while drawing and
// back on only when not paused (as the game's). The game's times are read and unused.
void __fastcall Draw(HUDDodgeBall* self, re::Edx) {
    CRGBA messageColour;
    RGBA_Construct(&messageColour, 0x73, 0x20, 0x20, 0xFF);
    SetRenderState_519790(6, 0);
    SetRenderState_519790(8, 0);
    SetRenderState_519790(0xC, 0);
    uint32_t flags;
    if (*Dodgeball_GetFlagsFn(dodgeballGame, &flags) & 0x400) {
        if (tutorial_C67540 && vc::Call<uint8_t>(tutorial_C67540, 0x10)) {
            Dodgeball_ForcePauseFn(dodgeballGame, 0);
        } else {
            DrawHelp();
        }
    } else if (Dodgeball_GetWinnerFn(dodgeballGame) == 0xFFFFFFFFu) {
        HUD_SetPromptTimed("DBH_Text1", 1.0f);
    } else {
        HUD_SetPrompt(nullptr);
    }
    uint32_t times[2];
    Dodgeball_TimesFn(dodgeballGame, times, 2);
    Font_Begin();
    Font_SetJustify(2);
    Font_SetStyleScale(1);
    if (!((*Dodgeball_GetFlagsFn(dodgeballGame, &flags) >> 10) & 1)) {
        if (self->message[0]) {
            SetColour(messageColour);
            Font_SetStyleScale(4);
            Font_PrintString(float(self->messageX), float(self->messageY), self->message, 1);
        }
        SetRenderState_519790(0xC, 1);
        SetRenderState_519790(6, 1);
        SetRenderState_519790(8, 1);
    }
}

} // namespace

#define HUDDODGE35_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

HUDDODGE35_REVERSED("HUDDodgeBall::HUDDodgeBall", 0x0067DF10, &Construct);
HUDDODGE35_REVERSED("HUDDodgeBall::SetText", 0x0067DF30, &SetText);
HUDDODGE35_REVERSED("HUDDodgeBall::DeletingDestructor", 0x0067DF60, &DeletingDestruct);
HUDDODGE35_REVERSED("HUDDodgeBall::Setup", 0x0067DF90, &Setup);
HUDDODGE35_REVERSED("HUDDodgeBall::Draw", 0x0067E1D0, &Draw);
// vtable slot 2 (the base's empty one-argument function): ret 4, 3 bytes
// 0x008A20A0: [c2 04 00 cc cc cc cc cc] too short to hook: the 64-bit dispatch's body
static void __cdecl Tiny_008A20A0() {}
RE_UNHOOKED_FN("HUDComponent::Empty_8A20A0", 0x008A20A0, &Tiny_008A20A0);
