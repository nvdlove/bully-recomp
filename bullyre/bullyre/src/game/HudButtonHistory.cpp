// HUDButtonHistory: the button-sequence minigames' HUD (a row of steps to press in
// time, "hold"/"mash" labels, a script callback on success, failure and each button)
// and the two ring buffers of recent presses it can show instead.
// 0x53A990..0x53C3A0 (its constructor, destructor and Init are Hud.cpp's).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Collision.h"
#include "game/Hud.h"
#include "reversed.h"

namespace {

// A button pressed, in a ring buffer
struct BHPress {           // (0x18)
    int32_t button;        // +0 -1: none
    uint32_t expires;      // +4 a time (0: never)
    uint8_t held;          // +8
    uint8_t unknown09[0xF];
};
static_assert(sizeof(BHPress) == 0x18);

// A step of the sequence
struct BHStep {            // (0x18)
    int32_t button;        // +0 -1: none
    int32_t unknown04;
    uint8_t hold;          // +8 held, not pressed ("FEI_HOL" over it)
    uint8_t mash;          // +9 ("FEI_CHR" over it)
    uint8_t done;          // +0xA
    uint8_t unknown0B;
    int32_t window;        // +0xC ms (the window of step i is step i/2's)
    int32_t earliest;      // +0x10 ms after the last step, -1: any time
    int32_t latest;        // +0x14
};
static_assert(sizeof(BHStep) == 0x18);

struct BHBuffer {          // (0x14)
    int32_t count;         // +0
    float seconds;         // +4 how long a press stays (0: for good)
    re::Ptr32<BHPress> presses;      // +8
    int32_t head;          // +0xC the oldest
    int32_t tail;          // +0x10 where the next goes
};
static_assert(sizeof(BHBuffer) == 0x14);

// The whole of it (Hud.h's HUDButtonHistory, with what's understood here)
struct ButtonHistory : HUDComponent {
    int32_t x, y;          // +0x20 (whole pixels)
    int32_t labelWidth;    // +0x28 (scaled)
    int32_t size;          // +0x2C a button's height (scaled; the width by its texture)
    int32_t gap;           // +0x30 (scaled)
    int32_t capacity;      // +0x34 (9)
    float seconds;         // +0x38 the buffers' press time (the layout's)
    int32_t unknown3C;     // +0x3C (100)
    int32_t unknown40;     // +0x40 (1)
    BHBuffer buffers[2];   // +0x44
    uint8_t finished;      // +0x6C
    uint8_t unknown6D;     // +0x6D
    uint8_t unknown6E[2];
    uint32_t onSuccess;    // +0x70 script callbacks (0x5D80E0: set?)
    uint32_t onFail;       // +0x74
    uint32_t onButton;     // +0x78
    int32_t current;       // +0x7C the step being waited for
    int32_t numHolds;      // +0x80
    re::Ptr32<BHStep> steps;         // +0x84
    int32_t numSteps;      // +0x88
    re::Ptr32<BHStep> done;          // +0x8C the steps done (held ones stay pressed)
    int32_t numDone;       // +0x90
    re::Ptr32<BHStep> ignored;       // +0x94 buttons that don't count
    int32_t numIgnored;    // +0x98
    uint32_t stepStart;    // +0x9C a time
    int32_t window;        // +0xA0 the current step's
    uint8_t inWindow;      // +0xA4
    uint8_t unknownA5[3];
    int32_t scriptedButton;  // +0xA8 (scripted input: the button "pressed")
    int32_t scriptedState;   // +0xAC (1 pressed, 3 held)
    uint8_t scripted;      // +0xB0 input from the script, not the pad
    uint8_t unknownB1[3];
    int32_t timeLimit;     // +0xB4 ms, -1: none
    uint32_t lastInput;    // +0xB8 a time
    uint32_t started;      // +0xBC a time
    int32_t failedButton;  // +0xC0
    uint32_t buttonMask;   // +0xC4 the sequence's buttons, by bit
    uint8_t failed;        // +0xC8
    uint8_t pressedOne;    // +0xC9
    uint8_t changed;       // +0xCA
    uint8_t failReported;  // +0xCB
    char name[0x20];       // +0xCC shown over the row (a text key)
};
static_assert(offsetof(ButtonHistory, buffers) == 0x44);
static_assert(offsetof(ButtonHistory, onSuccess) == 0x70);
static_assert(offsetof(ButtonHistory, stepStart) == 0x9C);
static_assert(offsetof(ButtonHistory, buttonMask) == 0xC4);
static_assert(offsetof(ButtonHistory, name) == 0xCC);
static_assert(sizeof(ButtonHistory) == sizeof(HUDButtonHistory));

// memory
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, HUDComponent_Reset, 0x0053CF00, HUDComponent* self);
GAME_FN(void, __thiscall, HUDComponent_SetAlignment, 0x0053CE00, HUDComponent* self, const void* h, const void* v);
struct CVector2D {
    float x, y;
};
GAME_FN(CVector2D*, __cdecl, ScreenPosition, 0x0052D4F0, CVector2D* out, float x, float y, int hAlign, int vAlign);
// a button's texture for the current pad
GAME_FN(void*, __cdecl, GetButtonTexture, 0x00514EC0, int button);
// the pad (player 0)
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int button, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int button, int pad);
// render states
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __thiscall, RGBA_Construct, 0x00510DE0, CRGBA* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const CRGBA* colour,
        const CVector2D* uv1, const CVector2D* uv2);
// text
GAME_FN(const uint16_t*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_SetJustify, 0x00688160, int justify);
GAME_FN(void, __cdecl, Font_CopyText, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(col::CRect*, __cdecl, Font_GetStringRect, 0x0068B090, col::CRect* out, float x, float y, const uint16_t* text);
GAME_FN(void, __cdecl, Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(void, __cdecl, Font_Render, 0x0068CDF0);
// a text key printed at a place (whole pixels)
GAME_FN(void, __cdecl, PrintTextKey, 0x006A6380, int x, int y, const char* key, int, int, int, int);
// script callbacks
GAME_FN(uint8_t, __thiscall, Callback_IsSet, 0x005D80E0, void* callback);
GAME_FN(void*, __thiscall, Callback_GetState, 0x005D9C20, void* callback);
GAME_FN(int, __thiscall, Callback_GetRef, 0x005D80D0, void* callback);
GAME_FN(void, __cdecl, Lua_Begin, 0x005D8270, void* state);
GAME_FN(void, __cdecl, Lua_PushRef, 0x005D8670, void* state, int ref);
GAME_FN(void, __cdecl, Lua_PushInt, 0x005D85C0, void* state, int value);
GAME_FN(void, __cdecl, Lua_PushBool, 0x005D85B0, void* state, int value);
GAME_FN(void, __cdecl, Lua_Call, 0x005D82A0, void* state, int args, int results);

// its own, where modes apply
GAME_FN(void, __thiscall, BH_RefreshButtons, 0x0053AA90, ButtonHistory* self);
GAME_FN(void, __thiscall, BH_ClearProgress, 0x0053AE10, ButtonHistory* self);
GAME_FN(int, __thiscall, BH_ReadButtons, 0x0053AF60, ButtonHistory* self, uint8_t* pressed);
GAME_FN(void, __thiscall, BH_CheckTiming, 0x0053B0F0, ButtonHistory* self, uint32_t time, int count,
        const uint8_t* pressed, BHStep* step);
GAME_FN(void, __thiscall, BH_AddPress, 0x0053B330, ButtonHistory* self, int buffer, int button, int held);
GAME_FN(void, __thiscall, BH_UpdateSteps, 0x0053B4C0, ButtonHistory* self);
GAME_FN(void, __thiscall, BH_DrawSteps, 0x0053B860, ButtonHistory* self);
GAME_FN(void, __thiscall, BH_UpdateBuffers, 0x0053C220, ButtonHistory* self);

GAME_VAR(uint32_t, g_timeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, g_language, 0x00C18724);
GAME_VAR(float, g_screenScaleX, 0x00A957A8);
GAME_VAR(float, g_screenScaleY, 0x00A957AC);
GAME_ARRAY(void*, g_buttonTextures, 0x00CEFC38);   // [25] by button (4, 5 never set)
GAME_VAR(float, g_bhFontScale, 0x00AB44D0);        // (1.0; 0.5 once drawn in language 7)
GAME_VAR(float, g_bhNameRaise, 0x00AB44CC);        // (2.0: the name's this many screen scales up)
inline void* const kTheText = reinterpret_cast<void*>(0x020C2FA4);
// the font's state
GAME_ARRAY(uint8_t, g_fontColour, 0x0208B764);
GAME_ARRAY(uint8_t, g_fontDropColour, 0x0208B780);
GAME_VAR(float, g_fontWrapCentre, 0x0208B778);
GAME_VAR(int16_t, g_fontStyle, 0x0208B788);
GAME_VAR(int16_t, g_fontEdge, 0x0208B784);
GAME_ARRAY(int32_t, g_fontStyleLoaded, 0x0208B218);

constexpr char kHoldKey[] = "FEI_HOL";   // (the game's at 0x91917C)

// new[]'s size: count * 0x18, all ones when it overflows
uint32_t ArraySize(uint32_t count) {
    uint64_t size = uint64_t(count) * 0x18;
    return size > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(size);
}

// a button texture's width for a height of `size` (its vtable's width and height)
uint32_t ButtonWidth(void* texture, int32_t size) {
    re::Ptr32<void>* vt = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texture);
    uint32_t w = re::PtrCast<uint32_t(__thiscall*)(void*)>(vt[0x48 / 4])(texture);
    uint32_t product = uint32_t(int32_t(w) * size);
    uint32_t h = re::PtrCast<uint32_t(__thiscall*)(void*)>(vt[0x4C / 4])(texture);
    return product / h;
}

// ---------------------------------------------------------------------------------------

// 0x53A990 (vf7 Reset): the buffers remade at the capacity, the textures forgotten
void __fastcall BH_Reset(ButtonHistory* self, re::Edx) {
    HUDComponent_Reset(self);
    self->finished = 0;
    self->unknown6D = 0;
    for (int b = 0; b < 2; b++) {
        BHBuffer& buf = self->buffers[b];
        if (buf.presses) {
            Free(buf.presses);
        }
        buf.count = self->capacity;
        buf.presses = static_cast<BHPress*>(Malloc(ArraySize(uint32_t(self->capacity))));
        for (int i = 0; i < buf.count; i++) {
            buf.presses[i].button = -1;
            buf.presses[i].expires = 0;
        }
        buf.head = 0;
        buf.tail = 0;
        buf.seconds = self->seconds;
    }
    for (int i = 0; i < 20; i++) {
        g_buttonTextures[i] = nullptr;
    }
}

// 0x53AA90: every button's texture for the pad in use (in the game's order)
void __fastcall BH_RefreshButtons_(ButtonHistory*, re::Edx) {
    static const int kOrder[23] = {7, 9, 6, 8, 10, 11, 14, 12, 13, 15, 0, 1, 2, 3, 16, 17, 18, 19, 20, 21, 22, 23, 24};
    for (int button : kOrder) {
        g_buttonTextures[button] = GetButtonTexture(button);
    }
}

// 0x53ABB0
void __fastcall BH_SetBufferSeconds(ButtonHistory* self, re::Edx, int buffer, float seconds) {
    self->buffers[buffer].seconds = seconds;
}

// 0x53ABD0: a buffer remade with `count` presses
void __fastcall BH_SetBufferSize(ButtonHistory* self, re::Edx, int buffer, int count) {
    BHBuffer& buf = self->buffers[buffer];
    if (buf.presses) {
        Free(buf.presses);
        buf.presses = nullptr;
    }
    buf.count = count;
    if (count > 0) {
        buf.presses = static_cast<BHPress*>(Malloc(ArraySize(uint32_t(count))));
    }
    for (int i = 0; i < buf.count; i++) {
        buf.presses[i].button = -1;
        buf.presses[i].expires = 0;
        buf.presses[i].held = 0;
    }
    buf.tail = 0;
    buf.head = 0;
}

// 0x53AC70: a buffer emptied
void __fastcall BH_ClearBuffer(ButtonHistory* self, re::Edx, int buffer) {
    BHBuffer& buf = self->buffers[buffer];
    for (int i = 0; i < buf.count; i++) {
        buf.presses[i].button = -1;
        buf.presses[i].expires = 0;
    }
}

// 0x53ACC0: the press `index` after the oldest
void __fastcall BH_GetPress(ButtonHistory* self, re::Edx, int buffer, int index, int32_t* button, uint8_t* held) {
    BHBuffer& buf = self->buffers[buffer];
    const BHPress& p = buf.presses[(buf.head + index) % buf.count];
    *button = p.button;
    *held = p.held;
}

// 0x53AD00: the sequence cleared
void __fastcall BH_ClearSteps(ButtonHistory* self, re::Edx) {
    BHStep* steps = self->steps;
    BHStep* done = self->done;
    BHStep* ignored = self->ignored;
    for (int i = 0; i < self->capacity; i++) {
        steps[i].button = -1;
        steps[i].hold = 0;
        steps[i].mash = 0;
        steps[i].done = 0;
        steps[i].window = self->window;
        steps[i].earliest = -1;
        steps[i].latest = -1;
        done[i].button = -1;
        done[i].hold = 0;
        done[i].mash = 0;
        done[i].done = 0;
        ignored[i].button = -1;
    }
    self->numSteps = 0;
    self->numHolds = 0;
    self->numDone = 0;
    self->numIgnored = 0;
    self->inWindow = 0;
    self->current = 0;
    self->pressedOne = 0;
    self->changed = 0;
    self->buttonMask = 0;
    self->name[0] = 0;
    self->lastInput = 0;
    self->unknown3C = 100;
    self->unknown40 = 1;
    self->failReported = 1;
}

// 0x53ADC0: a time limit (ms; 0 or less: none)
void __fastcall BH_SetTimeLimit(ButtonHistory* self, re::Edx, int ms) {
    self->lastInput = 0;
    self->timeLimit = ms > 0 ? ms : -1;
}

// 0x53ADF0 (too short to hook)
uint32_t* __fastcall BH_GetOnFail(ButtonHistory* self, re::Edx) {
    return &self->onFail;
}
RE_UNHOOKED_FN("HUDButtonHistory::GetOnFail", 0x0053ADF0, &BH_GetOnFail);

// 0x53AE00 (too short to hook)
uint32_t* __fastcall BH_GetOnButton(ButtonHistory* self, re::Edx) {
    return &self->onButton;
}
RE_UNHOOKED_FN("HUDButtonHistory::GetOnButton", 0x0053AE00, &BH_GetOnButton);

// 0x53AE10: back to the first step
void __fastcall BH_ClearProgress_(ButtonHistory* self, re::Edx) {
    for (int i = 0; i < self->numSteps; i++) {
        self->steps[i].done = 0;
    }
    self->current = 0;
    self->lastInput = 0;
    self->numDone = 0;
    self->inWindow = 0;
    self->pressedOne = 0;
    self->failReported = 0;
    self->scriptedButton = 0;
    self->scriptedState = 0;
}

// 0x53AE70
void __fastcall BH_SetName(ButtonHistory* self, re::Edx, const char* name) {
    std::strncpy(self->name, name, 0x1F);
    self->name[0x1F] = 0;
}

// 0x53AEA0: a step added (at most the capacity)
void __fastcall BH_AddStep(ButtonHistory* self, re::Edx, int button, int hold, int window, int earliest, int latest) {
    if (self->numSteps >= self->capacity) {
        return;
    }
    BHStep& s = self->steps[self->numSteps];
    s.window = window;
    s.earliest = earliest;
    s.button = button;
    s.hold = uint8_t(hold);
    s.latest = latest;
    self->numSteps++;
    self->buttonMask |= 1u << (button & 0x1F);
    if (re::Flag(hold)) {
        self->numHolds++;
    }
}

// 0x53AF10
void __fastcall BH_SetStepMash(ButtonHistory* self, re::Edx, int index) {
    if (index < self->numSteps) {
        self->steps[index].mash = 1;
    }
}

// 0x53AF30
void __fastcall BH_AddIgnored(ButtonHistory* self, re::Edx, int button) {
    if (self->numIgnored < self->capacity) {
        self->ignored[self->numIgnored].button = button;
        self->numIgnored++;
    }
}

// 0x53AF60: the pad's buttons down (0..15; the sticks' directions at 0x10..0x17), and
// how many. (ecx unused)
int __fastcall BH_ReadButtons_(ButtonHistory*, re::Edx, uint8_t* pressed) {
    std::memset(pressed, 0, 0x19);
    int count = 0;
    uint8_t* next = pressed;
    for (int button = 0; button < 0x14; button++) {
        float v = Pad_GetValue(button, 0);
        if (button >= 0x10) {
            int axis = button - 0x10;
            if (v > 0.5f) {
                pressed[0x10 + axis] = 1;
                count++;
            } else if (v < -0.5f) {
                pressed[0x14 + axis] = 1;
                count++;
            }
        } else {
            if (v > 0.0f) {
                *next = 1;
                count++;
            }
            next++;
        }
    }
    return count;
}

// 0x53B0E0
void __fastcall BH_SetScripted(ButtonHistory* self, re::Edx, int scripted) {
    self->scripted = uint8_t(scripted);
}

// 0x53B0F0: whether a step's press came in its time (failed set when not)
void __fastcall BH_CheckTiming_(ButtonHistory* self, re::Edx, uint32_t time, int count, const uint8_t* pressed,
                                BHStep* step) {
    int now = int(time);
    uint8_t down = pressed[step->button];
    if (step->earliest != -1) {
        if (down) {
            if (now > int(self->stepStart) + step->latest) {
                self->failed = 1;
            }
            return;
        }
        if (self->inWindow) {
            if (now >= int(self->stepStart) + step->earliest && now <= int(self->stepStart) + step->latest) {
                self->stepStart = time;
                step->done = 1;
                self->pressedOne = 1;
                return;
            }
            self->failed = 1;
            return;
        }
    } else {
        if (count == 0) {
            if (now > self->window + int(self->stepStart)) {
                self->failed = 1;
            }
            return;
        }
        if (!down) {
            self->failed = 1;
            return;
        }
        if (!step->done) {
            self->stepStart = time;
            step->done = 1;
            self->pressedOne = 1;
            return;
        }
    }
    if (now > self->window + int(self->stepStart)) {
        self->failed = 1;
    }
}

// 0x53B1D0 (vf1 Setup)
void __fastcall BH_Setup(ButtonHistory* self, re::Edx, const HUDLayoutRecord* record) {
    const uint8_t* d = record->data;
    auto s16 = [d](int offset) {
        int16_t v;
        std::memcpy(&v, d + offset, 2);
        return int(v);
    };
    HUDComponent_SetAlignment(self, d + 0x10, d + 0x18);
    CVector2D pos;
    ScreenPosition(&pos, float(s16(4)), float(s16(6)), self->hAlign, self->vAlign);
    self->x = int(pos.x);
    self->y = int(pos.y);
    self->labelWidth = s16(8);
    self->size = s16(10);
    self->size = int(float(s16(10)) * g_screenScaleY);
    self->labelWidth = int(float(self->labelWidth) * g_screenScaleX);
    self->gap = int(float(s16(0xC)) * g_screenScaleX);
    self->capacity = 9;
    std::memcpy(&self->seconds, d, 4);
}

// 0x53B330: a press into a buffer (buttons without a texture ignored)
void __fastcall BH_AddPress_(ButtonHistory* self, re::Edx, int buffer, int button, int held) {
    uint32_t now = g_timeInMilliseconds;
    BH_RefreshButtons(self);
    if (!g_buttonTextures[button]) {
        return;
    }
    BHBuffer& buf = self->buffers[buffer];
    if (buf.count <= 0) {
        return;
    }
    BHPress& p = buf.presses[buf.tail];
    p.button = button;
    p.held = uint8_t(held);
    if (buf.seconds > 0.0f) {
        // (x87: the sum rounded to a float, then truncated through 64 bits)
        float expires = float(double(float(double(buf.seconds) * 1000.0)) + double(now));
        p.expires = uint32_t(int64_t(expires));
    } else {
        p.expires = 0;
    }
    if (buf.tail == buf.head) {
        int next = (buf.head + 1) % buf.count;
        buf.head = next;
        buf.tail = next;
    } else {
        buf.tail = (buf.tail + 1) % buf.count;
    }
}

// 0x53B430: the sequence started (or stopped); a held last step is doubled past the end
void __fastcall BH_Start(ButtonHistory* self, re::Edx, int finished) {
    self->finished = uint8_t(finished);
    uint32_t now = g_timeInMilliseconds;
    self->stepStart = now;
    self->started = now;
    int n = self->numSteps;
    if (n > 0 && self->steps[n - 1].hold) {
        self->steps[n] = self->steps[n - 1];
    }
}

// 0x53B490: scripted input
void __fastcall BH_SetScriptedInput(ButtonHistory* self, re::Edx, int button, int state) {
    uint32_t now = g_timeInMilliseconds;
    if (self->scripted) {
        self->scriptedButton = button;
        self->scriptedState = state;
        self->stepStart = now;
    }
}

// 0x53B4C0: a frame of the sequence: the pad (or the script's input) against the
// current step
void __fastcall BH_UpdateSteps_(ButtonHistory* self, re::Edx) {
    uint32_t now = g_timeInMilliseconds;
    if (self->scripted) {
        BHStep* step = &self->steps[self->current];
        self->window = self->steps[self->current >> 1].window;
        if (self->scriptedButton != step->button) {
            if (now <= self->stepStart + uint32_t(self->window)) {
                return;
            }
            BH_ClearProgress(self);
            self->stepStart = now;
            self->failedButton = step->button;
            self->changed = 1;
            self->failed = 1;
            return;
        }
        self->scriptedButton = 0;
        self->failed = 0;
        step->done = 1;
        if (self->scriptedState != (step->hold ? 3 : 1)) {
            return;
        }
        self->current++;
        self->stepStart = now;
        self->changed = 1;
        self->inWindow = 0;
        self->scriptedState = 0;
        return;
    }

    uint8_t pressed[0x1C];
    int count = BH_ReadButtons(self, pressed);
    for (int i = 0; i < self->numIgnored; i++) {
        int button = self->ignored[i].button;
        if (pressed[button]) {
            pressed[button] = 0;
            count--;
        }
    }
    int current = self->current;
    BHStep* step = &self->steps[current];
    self->window = self->steps[current >> 1].window;
    self->lastInput = now;
    self->changed = 0;
    if (self->failed) {
        if ((step->earliest != -1) ? (count != 1) : (count != 0)) {
            return;
        }
        self->stepStart = now;
    }
    if (!self->inWindow && count == 1 && step->earliest != -1) {
        self->inWindow = 1;
        self->stepStart = now;
    }
    self->failed = 0;
    for (int b = 0; b < 0x14; b++) {
        if (pressed[b] && !(self->buttonMask & (1u << b))) {
            self->failed = 1;
            break;
        }
    }
    // the steps done that are held must stay down
    int held = 0;
    for (int i = 0; i < self->numDone; i++, held++) {
        if (!pressed[self->done[i].button]) {
            held = -1;
            break;
        }
    }
    if (held == -1) {
        self->failed = 1;
    }
    bool finishNow = false;
    if (current == self->numSteps && step->hold) {
        if (now >= self->stepStart + uint32_t(self->window)) {
            finishNow = true;
        } else {
            if (pressed[step->button]) {
                return;
            }
            self->failed = 1;
        }
    }
    if (!finishNow) {
        if (!self->failed) {
            BH_CheckTiming(self, now, count - held, pressed, step);
        }
        if (self->failed) {
            BH_ClearProgress(self);
            self->failedButton = step->button;
        } else {
            if (!step->done) {
                return;
            }
            if (step->hold) {
                self->stepStart = now;
                self->current++;
                self->changed = 1;
                self->done[self->numDone] = *step;
                self->numDone++;
                if (self->current == self->numSteps && step->hold) {
                    self->changed = 0;
                }
                return;
            }
            if (pressed[step->button]) {
                return;
            }
            self->current++;
            self->inWindow = 0;
        }
        self->stepStart = now;
    }
    self->changed = 1;
}

// 0x53B860: the steps' row: the buttons (dim until done), the hold/mash labels over
// them, the name over the row
void __fastcall BH_DrawSteps_(ButtonHistory* self, re::Edx) {
    int language = g_language;
    float yOffset = 0.0f;
    BHStep* steps = self->steps;
    if (language == 7) {
        yOffset = -9.0f;
    }
    int baseY = int(float(self->y) + yOffset);
    if (language == 7) {
        g_bhFontScale = 0.5f;
    }
    float holdRaise = float(g_screenScaleY * 5.0);
    float mashRaise = holdRaise;
    CRGBA colour;
    RGBA_Construct(&colour, 0xD2, 0xD2, 0xEB, 0xEB);
    Font_Begin();
    g_fontWrapCentre = 640.0f;
    std::memcpy(g_fontColour, &colour, 4);
    Font_SetScale(g_bhFontScale);
    int savedStyle = g_fontStyle;
    if (g_fontStyleLoaded[1] != 0) {
        g_fontStyle = 1;
    }
    g_fontEdge = 2;
    CRGBA drop;
    RGBA_Construct(&drop, 0, 0, 0, 0xFF);
    std::memcpy(g_fontDropColour, &drop, 4);

    // the row's width
    float total = 0.0f;
    for (int i = 0; i < self->numSteps; i++) {
        int button = steps[i].button;
        if (button != -1) {
            total = total + float(ButtonWidth(g_buttonTextures[button], self->size));
            total = float(self->gap) + total;
        }
    }
    const uint16_t* holdText = Text_Get(kTheText, kHoldKey);
    const uint16_t* mashText = Text_Get(kTheText, "FEI_CHR");
    int n = self->numSteps;
    int holdHeight = 0;
    bool anyHold = false;
    bool anyMash = false;
    if (n & 1) {
        total = total - float(self->gap) * 0.5f;
    }
    float halfTotal = 0.5f * total;
    int x = int(float(self->x) - halfTotal);
    for (int i = 0; i < n; i++) {
        if (steps[i].hold && holdText) {
            anyHold = true;
        }
        if (steps[i].mash && mashText) {
            anyMash = true;
        }
        if (!anyHold) {
            x += self->gap + self->labelWidth;
        }
    }
    col::CRect rect1 = {1000000.0f, -1000000.0f, 0.0f, 1000000.0f};  // (right 0: the game's)
    uint16_t holdCopy[0x28];
    float fx = 0.0f;
    if (anyHold && holdText) {
        Font_SetScale(g_bhFontScale);
        Font_CopyText(holdCopy, holdText, 0x28);
        float fy = float(baseY);
        fx = float(x);
        Font_GetStringRect(&rect1, fx, fy, holdCopy);
        holdHeight = int(rect1.bottom - rect1.top - 5.0f);
        fy = fy - holdRaise - float(holdHeight);
        Font_GetStringRect(&rect1, fx, fy, holdCopy);
        holdRaise = 0.0f;
    }
    if (anyMash && mashText) {
        col::CRect rect2 = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
        Font_GetStringRect(&rect2, float(x), float(baseY), mashText);
        mashRaise = rect2.bottom - rect2.top - 5.0f;
        if (!anyHold) {
            holdHeight = int(mashRaise);
        }
    }
    x = self->x;
    if (self->name[0]) {
        col::CRect rect2 = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
        Font_SetScale(g_bhFontScale);
        float scaleY = g_screenScaleY;
        fx = float(x);
        const uint16_t* name = Text_Get(kTheText, self->name);
        float ny = float(self->size + baseY) - scaleY * g_bhNameRaise;
        Font_GetStringRect(&rect2, fx, ny, name);
        // centred on its own width (the game then shifts the rect about, which nothing
        // reads)
        float halfWidth = float(std::abs(rect2.right - rect2.left) * 0.5f);
        scaleY = g_screenScaleY;
        name = Text_Get(kTheText, self->name);
        ny = float(self->size + baseY) - scaleY * g_bhNameRaise;
        Font_PrintString(fx - halfWidth, ny, name, 1);
        Font_Render();
    }

    int state8, state6, state12;
    GetRenderState(8, &state8);
    GetRenderState(6, &state6);
    GetRenderState(0xC, &state12);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xC, 0);
    BHStep* step = self->steps;
    x = int(float(self->x) - halfTotal);
    for (int i = 0; i < self->numSteps; i++) {
        // (a step of -1 isn't passed: every one after it is looked at as it)
        if (step->button == -1) {
            continue;
        }
        float savedOffset = 0.0f;
        if (g_language == 7) {
            savedOffset = yOffset;
            yOffset = float(yOffset - 3.0);
        }
        if (step->hold && holdText) {
            float y = float(baseY) - holdRaise - float(holdHeight) + yOffset;
            if (self->numSteps == 1) {
                Font_SetJustify(2);
                Font_PrintString(float(self->labelWidth) * 0.5f + float(x), y, holdCopy, 1);
            } else {
                Font_SetJustify(0);
                Font_PrintString(float(x), y, holdCopy, 1);
            }
        }
        if (step->mash && mashText) {
            float y = float(baseY) - mashRaise + yOffset;
            if (self->numSteps == 1) {
                Font_SetJustify(2);
                Font_PrintString(float(self->labelWidth) * 0.5f + float(x), y, mashText, 1);
            } else {
                Font_SetJustify(0);
                Font_PrintString(float(x), y, mashText, 1);
            }
        }
        if (g_language == 7) {
            yOffset = savedOffset;
        }
        CRGBA buttonColour;
        RGBA_Construct(&buttonColour, 0x9B, 0x9B, 0x9B, 0x9B);
        if (step->done) {
            CRGBA bright;
            RGBA_Construct(&bright, 0xFF, 0xFF, 0xFF, 0xFA);
            buttonColour = bright;
        }
        void* texture = g_buttonTextures[step->button];
        float width = float(ButtonWidth(texture, self->size));
        CVector2D uv1 = {0.0f, 0.0f};
        CVector2D uv2 = {1.0f, 1.0f};
        float left = float(x);
        DrawRectUV(left, float(baseY) + yOffset, left + width, float(self->size + baseY) + yOffset, texture,
                   &buttonColour, &uv1, &uv2);
        x = int(float(self->gap) + width + float(x));
        step++;
    }
    Font_Render();
    int16_t style = int16_t(savedStyle);
    if (g_fontStyleLoaded[style] != 0) {
        g_fontStyle = style;
        if (style != 0) {
            g_fontEdge = 0;
        }
    }
    SetRenderState(8, state8);
    SetRenderState(6, state6);
    SetRenderState(0xC, state12);
}

// 0x53C220: expired presses dropped, the buffers' ends moved past empty slots, and
// buffer 1 fed from the pad
void __fastcall BH_UpdateBuffers_(ButtonHistory* self, re::Edx) {
    uint32_t now = g_timeInMilliseconds;
    for (int b = 0; b < 2; b++) {
        BHBuffer& buf = self->buffers[b];
        if (buf.count <= 0) {
            continue;
        }
        buf.tail = buf.head;
        for (int i = 0; i < buf.count; i++) {
            BHPress& p = buf.presses[i];
            if (p.expires < now && p.expires != 0) {
                p.expires = 0;
                p.button = -1;
            }
        }
        int oldHead = buf.tail;
        BHPress* presses = buf.presses;
        for (;;) {
            if (presses[buf.head].button < 0) {
                buf.head = (buf.head + 1) % buf.count;
            }
            if (buf.head == oldHead || presses[buf.head].button >= 0) {
                break;
            }
        }
        int head = buf.head;
        if (presses[head].button >= 0) {
            int tail = oldHead;
            for (;;) {
                if (presses[tail].button >= 0) {
                    buf.tail = (tail + 1) % buf.count;
                }
                tail = buf.tail;
                if (tail == head || presses[tail].button < 0) {
                    break;
                }
            }
        }
    }
    if (self->buffers[1].count > 0 && !self->finished) {
        bool any = false;
        for (int button = 0; button < 0x14; button++) {
            if (g_buttonTextures[button] && Pad_JustPressed(button, 0)) {
                if (any) {
                    BH_AddPress(self, 1, 0x18, 0);   // (a separator)
                }
                BH_AddPress(self, 1, button, 1);
                any = true;
            }
        }
        for (int i = 0; i < self->buffers[1].count; i++) {
            BHPress& p = self->buffers[1].presses[i];
            if (p.held && Pad_GetValue(p.button, 0) == 0.0f) {
                p.held = 0;
            }
        }
    }
}

// the buffers' presses, oldest first, as a row per buffer
void DrawBufferRow(ButtonHistory* self, const BHBuffer& buf, int y, int& x, int from, int to) {
    for (int i = from; i < to; i++) {
        int button = buf.presses[i].button;
        if (button < 0) {
            continue;
        }
        void* texture = g_buttonTextures[button];
        float width = float(ButtonWidth(texture, self->size));
        CVector2D uv2 = {1.0f, 1.0f};
        CVector2D uv1 = {0.0f, 0.0f};
        float left = float(x);
        CRGBA colour;
        RGBA_Construct(&colour, 0xFF, 0xFF, 0xFF, 0x80);
        DrawRectUV(left, float(y), left + width, float(self->size + y), texture, &colour, &uv1, &uv2);
        x = int(left + width) + self->gap;
    }
}

void LabelBufferRow(ButtonHistory* self, const BHBuffer& buf, int y, int& x, int from, int to) {
    for (int i = from; i < to; i++) {
        const BHPress& p = buf.presses[i];
        if (p.button < 0) {
            continue;
        }
        if (p.held) {
            PrintTextKey(self->labelWidth / 2 + x, y - 10, kHoldKey, 3, 2, 1, 0);
        }
        x += self->labelWidth + self->gap;
    }
}

// 0x53C3A0 (vf8 Draw): the sequence (and its callbacks), or with none the buffers
void __fastcall BH_Draw(ButtonHistory* self, re::Edx) {
    if (self->numSteps > 0) {
        if (!self->finished) {
            BH_UpdateSteps(self);
            uint8_t timedOut;
            if (self->timeLimit <= 0 || int(self->lastInput) < int(self->started) + self->timeLimit) {
                timedOut = 0;
            } else {
                timedOut = 1;
                self->failed = 1;
                self->failedButton = self->steps[self->current].button;
                BH_ClearProgress(self);
                uint32_t now = g_timeInMilliseconds;
                self->stepStart = now;
                self->started = now;
                self->changed = 1;
            }
            if (!self->failed) {
                bool report = true;
                if (self->numSteps == self->current) {
                    if (!self->changed) {
                        report = false;
                    } else if (Callback_IsSet(&self->onSuccess)) {
                        void* state = Callback_GetState(&self->onSuccess);
                        Lua_Begin(state);
                        Lua_PushRef(state, Callback_GetRef(&self->onSuccess));
                        Lua_Call(state, 0, 0);
                        self->finished = 1;
                    }
                }
                if (report && self->changed && Callback_IsSet(&self->onButton)) {
                    void* state = Callback_GetState(&self->onButton);
                    Lua_Begin(state);
                    Lua_PushRef(state, Callback_GetRef(&self->onButton));
                    Lua_PushInt(state, self->failedButton);
                    Lua_Call(state, 1, 0);
                }
            } else if (self->changed && Callback_IsSet(&self->onFail) && (timedOut || !self->failReported)) {
                void* state = Callback_GetState(&self->onFail);
                Lua_Begin(state);
                Lua_PushRef(state, Callback_GetRef(&self->onFail));
                Lua_PushInt(state, self->failedButton);
                // (the game pushes the whole dword the flag byte is in: the rest is
                // uninitialised stack)
                Lua_PushBool(state, timedOut);
                Lua_Call(state, 2, 0);
                self->failReported = 1;
            }
        }
        BH_RefreshButtons(self);
        BH_DrawSteps(self);
        return;
    }

    int state8, state6, state12;
    GetRenderState(8, &state8);
    GetRenderState(6, &state6);
    GetRenderState(0xC, &state12);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xC, 0);
    BH_RefreshButtons(self);
    BH_UpdateBuffers(self);
    int y = self->y;
    for (int b = 0; b < 2; b++) {
        const BHBuffer& buf = self->buffers[b];
        int x = self->x;
        DrawBufferRow(self, buf, y, x, buf.tail, buf.count);
        DrawBufferRow(self, buf, y, x, 0, buf.tail);
        y += self->gap + self->size;
    }
    y = self->y;
    for (int b = 0; b < 2; b++) {
        const BHBuffer& buf = self->buffers[b];
        int x = self->x;
        LabelBufferRow(self, buf, y, x, buf.tail, buf.count);
        LabelBufferRow(self, buf, y, x, 0, buf.tail);
        y += self->gap + self->size;
    }
    SetRenderState(8, state8);
    SetRenderState(6, state6);
    SetRenderState(0xC, state12);
}

} // namespace

#define HUDBH_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(hudbh_original_, __LINE__);                                           \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(hudbh_original_, __LINE__), \
             re::Mode::Original)

HUDBH_REVERSED("HUDButtonHistory::Reset", 0x0053A990, &BH_Reset);
HUDBH_REVERSED("HUDButtonHistory::RefreshButtons", 0x0053AA90, &BH_RefreshButtons_);
HUDBH_REVERSED("HUDButtonHistory::SetBufferSeconds", 0x0053ABB0, &BH_SetBufferSeconds);
HUDBH_REVERSED("HUDButtonHistory::SetBufferSize", 0x0053ABD0, &BH_SetBufferSize);
HUDBH_REVERSED("HUDButtonHistory::ClearBuffer", 0x0053AC70, &BH_ClearBuffer);
HUDBH_REVERSED("HUDButtonHistory::GetPress", 0x0053ACC0, &BH_GetPress);
HUDBH_REVERSED("HUDButtonHistory::ClearSteps", 0x0053AD00, &BH_ClearSteps);
HUDBH_REVERSED("HUDButtonHistory::SetTimeLimit", 0x0053ADC0, &BH_SetTimeLimit);
HUDBH_REVERSED("HUDButtonHistory::ClearProgress", 0x0053AE10, &BH_ClearProgress_);
HUDBH_REVERSED("HUDButtonHistory::SetName", 0x0053AE70, &BH_SetName);
HUDBH_REVERSED("HUDButtonHistory::AddStep", 0x0053AEA0, &BH_AddStep);
HUDBH_REVERSED("HUDButtonHistory::SetStepMash", 0x0053AF10, &BH_SetStepMash);
HUDBH_REVERSED("HUDButtonHistory::AddIgnored", 0x0053AF30, &BH_AddIgnored);
HUDBH_REVERSED("HUDButtonHistory::ReadButtons", 0x0053AF60, &BH_ReadButtons_);
HUDBH_REVERSED("HUDButtonHistory::SetScripted", 0x0053B0E0, &BH_SetScripted);
HUDBH_REVERSED("HUDButtonHistory::CheckTiming", 0x0053B0F0, &BH_CheckTiming_);
HUDBH_REVERSED("HUDButtonHistory::Setup", 0x0053B1D0, &BH_Setup);
HUDBH_REVERSED("HUDButtonHistory::AddPress", 0x0053B330, &BH_AddPress_);
HUDBH_REVERSED("HUDButtonHistory::Start", 0x0053B430, &BH_Start);
HUDBH_REVERSED("HUDButtonHistory::SetScriptedInput", 0x0053B490, &BH_SetScriptedInput);
HUDBH_REVERSED("HUDButtonHistory::UpdateSteps", 0x0053B4C0, &BH_UpdateSteps_);
HUDBH_REVERSED("HUDButtonHistory::DrawSteps", 0x0053B860, &BH_DrawSteps_);
HUDBH_REVERSED("HUDButtonHistory::UpdateBuffers", 0x0053C220, &BH_UpdateBuffers_);
HUDBH_REVERSED("HUDButtonHistory::Draw", 0x0053C3A0, &BH_Draw);
