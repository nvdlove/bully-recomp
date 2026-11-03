// Chemistry class (ClassChem; also the shop's "SHOP" game type): a rhythm minigame. The
// scripts add sequences of timed actions (ClassChemAddAction: button presses, stick
// turns, triggers held) and start one; each action's window [start, end] must see its
// input, scored by how centred the press is. A "scrolly" bar shows the actions coming
// (0x6E8110..0x6EB820 with five of its script natives; the rest are NativesC2.cpp's).
//
// Actions: 0..13 buttons and d-pad (0 cross, 1 circle, 2 square, 3 triangle, 4 L1, 5 L2,
// 6 L3, 7 R1, 8 R2, 9 R3, 10 up, 11 down, 12 left, 13 right), 14..21 the sticks pushed
// (14/15 left up/down, 16/17 left/right, 18..21 the right stick), 22..27 the left stick
// turned (22/24 anticlockwise, 23/25 clockwise, 26/27 a quarter), 28..33 the right stick
// turned likewise, 34 none. The x87 runs at 24-bit precision here (the main thread).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/ScriptApi.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct ChemAction {
    int32_t action;           // +0x0
    float start;              // +0x4 seconds into the sequence
    float end;                // +0x8
    float unknownC;           // +0xC
    float unknown10;          // +0x10
};
static_assert(sizeof(ChemAction) == 0x14);

// a stick's turning, followed by its angle (+0xCDC, +0xCFC)
struct ChemStick {
    uint8_t active;           // +0x0 pushed past the dead zone
    uint8_t unknown1[3];
    float angle;              // +0x4 atan2
    float lastAngle;          // +0x8
    float startAngle;         // +0xC
    float speed;              // +0x10 radians a tick
    float time;               // +0x14
    float turns;              // +0x18 turned, in turns (signed)
    float turned;             // +0x1C radians
};
static_assert(sizeof(ChemStick) == 0x20);

struct ClassChem {
    re::Ptr32<re::Ptr32<void>> vtable;                     // +0x0
    uint8_t active;                    // +0x4
    uint8_t hud;                       // +0x5
    uint8_t success;                   // +0x6
    uint8_t unknown7[0x15];
    re::Ptr32<re::Ptr32<void>> textures;                   // +0x1C
    uint8_t unknown20[0x18];
    int32_t unknown38;                 // +0x38 (1)
    int32_t unknown3C;                 // +0x3C (5)
    uint8_t unknown40[0xC];
    re::Ptr32<uint32_t> sounds;                  // +0x4C
    int32_t popupKind;                 // +0x50 the popup (1 and 3 quit questions, 2 another)
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;              // +0xA0
    uint8_t unknownA1[0x23];
    int32_t gameType;                  // +0xC4 1 "CHEM", 2 "SHOP"
    ChemAction actions[5][30];         // +0xC8 the sequences
    int32_t actionCount[5];            // +0xC80
    int32_t sequence;                  // +0xC94 running (-1 none)
    int32_t index;                     // +0xC98 the action due
    uint8_t completed;                 // +0xC9C its input done
    uint8_t justStarted;               // +0xC9D
    uint8_t missed;                    // +0xC9E
    uint8_t unknownC9F;
    float flash;                       // +0xCA0 the hit flash fading
    uint8_t justFailed;                // +0xCA4
    uint8_t justFinished;              // +0xCA5
    uint8_t unknownCA6[2];
    float time;                        // +0xCA8 into the action
    float pressTime;                   // +0xCAC
    float performance;                 // +0xCB0
    float pulse;                       // +0xCB4 the button's size pulsing (1..1.2)
    int32_t enabled[6];                // +0xCB8 the input groups (face, shoulders, sticks
                                       //   clicked, d-pad, stick turns, sticks pushed)
    uint8_t scrollyVisible;            // +0xCD0
    uint8_t scrollyOnly;               // +0xCD1
    uint8_t unknownCD2[2];
    float speedScale;                  // +0xCD4
    float pixPerSec;                   // +0xCD8
    ChemStick sticks[2];               // +0xCDC
};
static_assert(offsetof(ClassChem, gameType) == 0xC4);
static_assert(offsetof(ClassChem, actionCount) == 0xC80);
static_assert(offsetof(ClassChem, flash) == 0xCA0);
static_assert(offsetof(ClassChem, enabled) == 0xCB8);
static_assert(offsetof(ClassChem, sticks) == 0xCDC);
static_assert(sizeof(ClassChem) == 0xD1C);

GAME_VAR(float, chemScreenScaleX, 0x00A957A8);
GAME_VAR(float, chemScreenScaleY, 0x00A957AC);
GAME_VAR(float, chemTimeStep, 0x00C1A9A4);
GAME_VAR(uint8_t*, chemCurrentMinigame, 0x020C8704);
GAME_VAR(int32_t, chemUserStartResponse, 0x020C87AC);
GAME_VAR(float, chemSpin1, 0x020C8284);        // the turn arrows' spins
GAME_VAR(float, chemSpin2, 0x020C8288);
GAME_VAR(float, chemPulseTime, 0x020C828C);
GAME_VAR(float, chemRate, 0x00BC64C8);          // (0.02: the time step's seconds)
GAME_VAR(float, chemTwoPi, 0x00BC64C4);
GAME_VAR(float, chemSpinRate1, 0x00BC64D4);
GAME_VAR(float, chemSpinRate2, 0x00BC64D8);
GAME_VAR(float, chemScrollyHalfHeight, 0x00BC64CC);
GAME_VAR(float, chemScrollyPixPerSec, 0x00BC6458);
GAME_ARRAY(int32_t, chemPadButtons, 0x00A14810);   // the pad's buttons' masks, by game button
GAME_ARRAY(uint8_t, chemColourScrolly, 0x020C82A4);
GAME_ARRAY(uint8_t, chemColourWaiting, 0x020C82A8);
GAME_ARRAY(uint8_t, chemColourDone, 0x020C82AC);
GAME_ARRAY(uint8_t, chemColourFlash, 0x020C82B0);
inline const uint8_t* const chemColourBar = reinterpret_cast<const uint8_t*>(0x020C8290);
inline const uint8_t* const chemColourBarEdge = reinterpret_cast<const uint8_t*>(0x020C8294);
inline const uint8_t* const chemColourMark = reinterpret_cast<const uint8_t*>(0x020C8298);
inline const uint8_t* const chemColourLine = reinterpret_cast<const uint8_t*>(0x020C829C);
inline void* const chemCamera = reinterpret_cast<void*>(0x00A14810);
inline void* const chemCameraSettings = reinterpret_cast<void*>(0x00BC6460);

inline const char* GameString(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}
inline float Sqrt(float v) {
    return float(std::sqrt(double(v)));
}

struct ChemTextureEntry {
    int32_t id;
    re::Ptr32<const char> name;
    re::Ptr32<const char> mask;
};
static_assert(sizeof(ChemTextureEntry) == 0xC);

GAME_FN(void, __thiscall, Minigame_Construct, 0x007059D0, void* self);
GAME_FN(void, __thiscall, Minigame_Destruct, 0x00705A50, void* self);
GAME_FN(void, __thiscall, Minigame_EndSounds, 0x00704DE0, void* self);
GAME_FN(void, __thiscall, Minigame_LoadTextures, 0x00704D30, void* self, int count, const ChemTextureEntry* entries,
        const char* txd);
GAME_FN(void*, __thiscall, Minigame_GetTexture, 0x006D7CB0, void* self, int index);
GAME_FN(int, __thiscall, Popup_Result, 0x00705080, void* popup);
GAME_FN(void, __thiscall, Popup_Draw, 0x00706000, void* popup);
GAME_FN(void, __thiscall, Camera_PushSettings, 0x00454D10, void* camera, void* settings);
GAME_FN(void, __thiscall, Camera_PopSettings, 0x00454D60, void* camera);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(float, __cdecl, Pad_GetAxis, 0x00738AD0, int axis, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(int32_t, __cdecl, Pad_GetActive, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_IsXInput, 0x00738500, int32_t pad);
GAME_FN(void*, __cdecl, Hud_GetButtonTexture, 0x00514EC0, int which);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(float*, __cdecl, ScreenPosition, 0x0052D4F0, float* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, DrawQuadUV, 0x0052CCC0, float x1, float y1, float x2, float y2, float x3, float y3, float x4,
        float y4, void* texture, const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, DrawRect, 0x0052C470, const float* rect, const uint8_t* colour);
GAME_FN(void, __cdecl, DrawRect4, 0x0052C530, const float* rect, const uint8_t* topLeft, const uint8_t* topRight,
        const uint8_t* bottomLeft, const uint8_t* bottomRight);
GAME_FN(uint8_t, __cdecl, Streaming_IsReady, 0x0042E430);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }

// the class's own (through the game's addresses)
GAME_FN(int, __stdcall, Chem_MapButton, 0x006E8680, int action);
GAME_FN(int, __stdcall, Chem_PromptIndex, 0x006E8760, int action);
GAME_FN(int, __stdcall, Chem_GlowFor, 0x006E8990, int action);
GAME_FN(void*, __thiscall, Chem_GetButtonTexture, 0x006E8960, ClassChem* self, int action);
GAME_FN(uint8_t, __thiscall, Chem_IsActionPressed, 0x006E89E0, ClassChem* self, int action);
GAME_FN(uint8_t, __thiscall, Chem_OtherPressed, 0x006E9C60, ClassChem* self, int action);
GAME_FN(uint8_t, __thiscall, Chem_AnyPressed, 0x006E9CB0, ClassChem* self);
GAME_FN(void, __thiscall, Chem_UpdatePulse, 0x006E9CE0, ClassChem* self);
GAME_FN(void, __thiscall, Chem_EndGame, 0x006E8110, ClassChem* self, int success);
GAME_FN(void, __thiscall, Chem_DrawButton, 0x006E8CA0, ClassChem* self, int action, float x, float y,
        uint32_t colour, float scale, int highlight, int withGlow);
GAME_FN(void, __thiscall, Chem_AddAction, 0x006EB350, void* self, int sequence, int action, float start,
        float duration, float a, float b);

inline ChemAction& Current(ClassChem* chem) {
    return chem->actions[chem->sequence][chem->index];
}
inline uint32_t Colour(const uint8_t* rgba) {
    uint32_t c;
    std::memcpy(&c, rgba, 4);
    return c;
}

// ---- the game's own --------------------------------------------------------------------------

// 0x6E8110: the sequence over (its virtual +0x14 ends the game)
void __fastcall ChemEndGame(ClassChem* chem, re::Edx, int success) {
    chem->success = uint8_t(success);
    chem->sequence = -1;
    vc::Call<void>(chem, 0x14);
}

// 0x6E8130: ClassChemSetActiveActions's: which input groups count (4: none of the
// buttons; 5: the d-pad and sticks clicked off; 6: only the sticks pushed (as 1, 2 and
// 3 with more off): 1 no shoulders, 2 no clicks or d-pad, 3 no turns)
void __fastcall ChemSetActiveActions(ClassChem* chem, re::Edx, int set) {
    for (int i = 0; i < 5; ++i) {
        chem->enabled[i] = 1;
    }
    chem->enabled[5] = 0;
    switch (set) {
    case 4:
        chem->enabled[0] = 0;
        chem->enabled[1] = 0;
        chem->enabled[2] = 0;
        chem->enabled[3] = 0;
        return;
    case 5:
        chem->enabled[3] = 0;
        chem->enabled[2] = 0;
        return;
    case 6:
        chem->enabled[0] = 0;
        chem->enabled[5] = 1;
        [[fallthrough]];
    case 1:
        chem->enabled[1] = 0;
        [[fallthrough]];
    case 2:
        chem->enabled[2] = 0;
        chem->enabled[3] = 0;
        [[fallthrough]];
    case 3:
        chem->enabled[4] = 0;
        return;
    }
}

// 0x6E81E0
ClassChem* __fastcall ChemConstruct(ClassChem* chem, re::Edx) {
    Minigame_Construct(chem);
    chem->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00944FEC);
    chem->unknown38 = 1;
    chem->unknown3C = 5;
    chem->gameType = 0;
    return chem;
}

// 0x6E8210 (virtual +0x10): the game starts: no sequence, the scrolly at 128 pixels a
// second, every group but the pushed sticks on; the buttons' textures ("MG_Chem"); the
// pulse and spins reset; the camera's settings pushed
void __fastcall ChemStart(ClassChem* chem, re::Edx) {
    chem->time = 0.0f;
    chem->active = 1;
    chem->hud = 0;
    chem->sequence = -1;
    chem->index = -1;
    for (int i = 0; i < 5; ++i) {
        chem->actionCount[i] = 0;
    }
    chem->pixPerSec = 128.0f;
    chem->sticks[0].active = 0;
    chem->sticks[1].active = 0;
    chem->speedScale = 1.0f;
    chem->justFailed = 0;
    chem->pulse = 1.0f;
    chem->justFinished = 0;
    chem->scrollyVisible = 1;
    chem->scrollyOnly = 0;
    for (int i = 0; i < 5; ++i) {
        chem->enabled[i] = 1;
    }
    chem->enabled[5] = 0;
    const uintptr_t x = 0x00945270;    // "HRbutton_x"
    const uintptr_t lr1 = 0x00945204;  // "HRbutton_LR1_x"
    const uintptr_t lr2 = 0x00945260;  // "HRbutton_LR2_x"
    const uintptr_t dpad = 0x00945114; // "dpad_x"
    const uintptr_t none = 0x008FEFBB; // ""
    const ChemTextureEntry textures[31] = {
        {0, GameString(0x00945250), GameString(x)},       // HRbutton_cross
        {1, GameString(0x00945240), GameString(x)},       // HRbutton_circl
        {2, GameString(0x00945230), GameString(x)},       // HRbutton_squa
        {3, GameString(0x00945220), GameString(x)},       // HRbutton_tri
        {5, GameString(0x00945214), GameString(lr1)},     // HRbutton_L1
        {4, GameString(0x009451F8), GameString(lr2)},     // HRbutton_L2
        {6, GameString(0x009451EC), GameString(x)},       // HRbutton_L3
        {8, GameString(0x009451E0), GameString(lr1)},     // HRbutton_R1
        {7, GameString(0x009451D4), GameString(lr2)},     // HRbutton_R2
        {9, GameString(0x009451C8), GameString(x)},       // HRbutton_R3
        {0xE, GameString(0x009451B8), GameString(x)},     // HRbutton_L3a
        {0xF, GameString(0x009451A8), GameString(x)},     // HRbutton_R3a
        {0x10, GameString(0x00945198), GameString(0x00945184)},   // HRbutton_rotate
        {0x11, GameString(0x00945174), GameString(0x00945160)},   // HRanalog_arrow
        {0x12, GameString(0x00945150), GameString(0x00945140)},   // HRbutton_glow
        {0x13, GameString(0x00945130), GameString(0x0094511C)},   // HRbutton_sucess
        {0x14, GameString(0x009450FC), GameString(0x009450EC)},   // HRbutton_fail
        {0x15, GameString(0x009450D8), GameString(0x009450C4)},   // HRbutton_LR1_glow
        {0x16, GameString(0x009450B0), GameString(0x0094509C)},   // HRbutton_LR2_glow
        {0xA, GameString(0x00945094), GameString(dpad)},  // dpad_up
        {0xB, GameString(0x00945088), GameString(dpad)},  // dpad_dwn
        {0xC, GameString(0x0094507C), GameString(dpad)},  // dpad_lef
        {0xD, GameString(0x00945070), GameString(dpad)},  // dpad_rig
        {0x17, GameString(0x00945064), GameString(none)}, // HRbutton_W
        {0x18, GameString(0x00945058), GameString(none)}, // HRbutton_A
        {0x19, GameString(0x0094504C), GameString(none)}, // HRbutton_S
        {0x1A, GameString(0x00945040), GameString(none)}, // HRbutton_D
        {0x1B, GameString(0x00945034), GameString(none)}, // HRbutton_Q
        {0x1C, GameString(0x00945028), GameString(none)}, // HRbutton_E
        {0x1D, GameString(0x00945018), GameString(none)}, // HRbutton_LMB
        {0x1E, GameString(0x00945008), GameString(none)}, // HRbutton_RMB
    };
    Minigame_LoadTextures(chem, 31, textures, GameString(0x0094510C));   // "MG_Chem"
    chemPulseTime = 0.3f;
    chemSpin1 = 0.0f;
    chemSpin2 = 0.0f;
    Camera_PushSettings(chemCamera, chemCameraSettings);
}

// 0x6E8660 (virtual +0x14): the game ends
void __fastcall ChemEnd(ClassChem* chem, re::Edx) {
    chem->active = 0;
    chem->hud = 0;
    Minigame_EndSounds(chem);
    Camera_PopSettings(chemCamera);
}

// 0x6E8680: an action's pad button (the game's button numbering; -1 for the sticks).
// (Callers rely on ecx surviving it: see the registration.)
int __stdcall ChemMapButton(int action) {
    static constexpr int kButton[16] = {7, 8, 6, 9, 0xA, 0xB, 0xE, 0xC, 0xD, 0xF, 2, 3, 0, 1, 0x19, 0x1A};
    if (uint32_t(action) > 0xF) {
        return -1;
    }
    return kButton[action];
}

// 0x6E8760: the prompt texture for an action's button: by the pad's mapping on an XInput
// pad (its mask's bit: 0x1000 cross .. 0x8000 triangle, 1..0x40 d-pad and sticks clicked,
// 0x100..0x800 shoulders; else 9), else the keyboard and mouse prompts (W A S D Q E LMB
// RMB) for the shoulders and triggers; the action itself otherwise
int __stdcall ChemPromptIndex(int action) {
    int button = Chem_MapButton(action);
    if (button == -1) {
        return action;
    }
    if (Pad_IsXInput(Pad_GetActive()) != 0) {
        switch (chemPadButtons[button]) {
        case 1:
            return 10;
        case 2:
            return 11;
        case 4:
            return 12;
        case 8:
        case 0x10:
        case 0x20:
            return 13;
        case 0x40:
            return 6;
        case 0x100:
            return 5;
        case 0x200:
            return 8;
        case 0x400:
            return 4;
        case 0x800:
            return 7;
        case 0x1000:
            return 0;
        case 0x2000:
            return 1;
        case 0x4000:
            return 2;
        case 0x8000:
            return 3;
        default:
            return 9;
        }
    }
    switch (button) {
    case 6:
        return 0x18;
    case 7:
        return 0x19;
    case 8:
        return 0x1A;
    case 9:
        return 0x17;
    case 10:
        return 0x1D;
    case 11:
        return 0x1B;
    case 12:
        return 0x1E;
    case 13:
        return 0x1C;
    }
    return action;
}

// 0x6E8960: an action's texture: the HUD's prompt for a mapped button, else the
// minigame's own. (The game reads ecx back after 0x6E8680.)
void* __fastcall ChemGetButtonTexture(ClassChem* chem, re::Edx, int action) {
    int button = Chem_MapButton(action);
    if (button != -1) {
        return Hud_GetButtonTexture(button);
    }
    return chem->textures != nullptr ? chem->textures[action] : nullptr;
}

// 0x6E8990: the glow texture for an action's prompt (the shoulders' and triggers' own)
int __stdcall ChemGlowFor(int action) {
    switch (Chem_PromptIndex(action)) {
    case 4:
    case 7:
        return 0x16;
    case 5:
    case 8:
        return 0x15;
    default:
        return 0x12;
    }
}

// 0x6E89E0: an action's input this frame, in its group if that is on
uint8_t __fastcall ChemIsActionPressed(ClassChem* chem, re::Edx, int action) {
    if (chem->enabled[0] != 0) {
        static constexpr int kFace[4] = {7, 8, 6, 9};
        if (uint32_t(action) <= 3 && Pad_JustDown(kFace[action], 0) != 0) {
            return 1;
        }
    }
    if (chem->enabled[1] != 0) {
        int button = -1;
        switch (action) {
        case 4:
            button = 0xA;
            break;
        case 5:
            button = 0xB;
            break;
        case 7:
            button = 0xC;
            break;
        case 8:
            button = 0xD;
            break;
        }
        if (button != -1 && Pad_JustDown(button, 0) != 0) {
            return 1;
        }
    }
    if (chem->enabled[2] != 0) {
        int button = action == 6 ? 0xE : action == 9 ? 0xF : -1;
        if (button != -1 && Pad_JustDown(button, 0) != 0) {
            return 1;
        }
    }
    if (chem->enabled[3] != 0) {
        static constexpr int kDpad[4] = {2, 3, 0, 1};
        if (action >= 10 && action <= 13 && Pad_JustDown(kDpad[action - 10], 0) != 0) {
            return 1;
        }
    }
    if (chem->enabled[4] != 0) {
        switch (action) {
        case 0x16:
        case 0x18:
        case 0x1A:
            if (chem->sticks[0].active != 0 && 0.25f > chem->sticks[0].turns) {
                return 1;
            }
            break;
        case 0x17:
        case 0x19:
        case 0x1B:
            if (chem->sticks[0].active != 0 && -0.25f < chem->sticks[0].turns) {
                return 1;
            }
            break;
        case 0x1C:
        case 0x1E:
        case 0x20:
            if (chem->sticks[1].active != 0 && 0.25f > chem->sticks[1].turns) {
                return 1;
            }
            break;
        case 0x1D:
        case 0x1F:
        case 0x21:
            if (chem->sticks[1].active != 0 && -0.25f < chem->sticks[1].turns) {
                return 1;
            }
            break;
        }
    }
    if (chem->enabled[5] != 0 && action >= 0xE && action <= 0x15) {
        static constexpr int kAxis[8] = {0x11, 0x11, 0x10, 0x10, 0x13, 0x13, 0x12, 0x12};
        float v = Pad_GetAxis(kAxis[action - 0xE], 0);
        if ((action & 1) == 0 ? v > 0.7f : v < -0.7f) {
            return 1;
        }
    }
    return action == 0x22 ? 1 : 0;
}

// 0x6E8C70 (virtual +0): the deleting destructor
ClassChem* __fastcall ChemDelete(ClassChem* chem, re::Edx, int flags) {
    chem->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00944FEC);
    Minigame_Destruct(chem);
    if ((flags & 1) != 0) {
        Free(chem);
    }
    return chem;
}
// ---- drawing a button --------------------------------------------------------------------------

// a texture across [x1, x2] x [y1, y2], its whole image
inline void DrawWhole(float x1, float y1, float x2, float y2, void* texture, const uint8_t* colour) {
    const float uv1[2] = {0.0f, 0.0f};
    const float uv2[2] = {1.0f, 1.0f};
    DrawRectUV(x1, y1, x2, y2, texture, colour, uv1, uv2);
}
// a quad with the whole image
inline void DrawWholeQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
                          void* texture, const uint8_t* colour) {
    const float uv1[2] = {0.0f, 0.0f};
    const float uv2[2] = {1.0f, 1.0f};
    DrawQuadUV(x1, y1, x2, y2, x3, y3, x4, y4, texture, colour, uv1, uv2);
}
// a corner of the stick arrow's texture (the texture's (0.5, 0)..(1, `v2`))
inline void DrawArrowCorner(float x1, float y1, float x2, float y2, void* texture, const uint8_t* colour,
                            float v2) {
    const float uv1[2] = {0.5f, 0.0f};
    const float uv2[2] = {1.0f, v2};
    DrawRectUV(x1, y1, x2, y2, texture, colour, uv1, uv2);
}

// 0x6E8CA0: an action's prompt at (x, y), `scale` of its 64 pixels (times the pulse):
// the button (in the shop, coloured between waiting and done by the pulse), fading over
// the screen's last and first 40 pixels; a stick push's arrow, a stick turn's arrow
// spinning; with `highlight`, the glow over it (or, without `withGlow`, the fail mark)
void __fastcall ChemDrawButton(ClassChem* chem, re::Edx, int action, float x, float y, uint32_t colourIn, float scale,
                              int highlight, int withGlow) {
    if (action == 0x22) {
        return;
    }
    int state9;
    int stateC;
    GetRenderState(9, &state9);
    GetRenderState(0xC, &stateC);
    SetRenderState(9, 2);
    SetRenderState(0xC, 1);
    float scaleX = chemScreenScaleX;
    float scaleY = chemScreenScaleY;
    scale = chem->pulse * scale;
    uint8_t colour[4];
    std::memcpy(colour, &colourIn, 4);
    if (chem->gameType == 2 && uint32_t(action) <= 0xD) {
        float t = (chem->pulse - 1.0f) / 0.20000004768371582f;
        const uint8_t* a = chemColourWaiting;
        const uint8_t* b = chemColourDone;
        colour[0] = uint8_t(int(t * float(int(b[0]) - int(a[0])) + float(a[0])));
        colour[1] = uint8_t(int(float(int(b[1]) - int(a[1])) * t + float(a[1])));
        colour[2] = uint8_t(int(float(int(b[2]) - int(a[2])) * t + float(a[2])));
        colour[3] = uint8_t(int(t * float(int(b[3]) - int(a[3])) + float(a[3])));
    }
    if (x > 600.0f) {
        float q = (x - 600.0f) / 40.0f;
        colour[3] = uint8_t(int((1.0f - q) * float(colour[3])));
    } else if (40.0f > x) {
        colour[3] = uint8_t(int(x / 40.0f * float(colour[3])));
    }
    int drawn = 0;
    float half = scale * 64.0f * 0.5f;
    if (uint32_t(action) <= 0xD) {
        float hh = scaleY * half;
        float hw = scaleX * half;
        DrawWhole(x - hw, y - hh, x + hw, y + hh, Chem_GetButtonTexture(chem, action), colour);
        drawn = action;
    } else if ((action >= 0x16 && action <= 0x1B) || uint32_t(action - 0xE) <= 3) {
        void* stick = Hud_GetButtonTexture(0x19);
        float hh = scaleY * half;
        float hw = half * scaleX;
        DrawWhole(x - hw, y - hh, x + hw, y + hh, stick, colour);
        drawn = 0xE;
    } else if ((action >= 0x1C && action <= 0x21) || uint32_t(action - 0x12) <= 3) {
        float hh = scaleY * half;
        float hw = scaleX * half;
        DrawWhole(x - hw, y - hh, x + hw, y + hh, Chem_GetButtonTexture(chem, 0xF), colour);
        drawn = 0xF;
    }

    switch (action) {
    case 0x16:
    case 0x1C:
    case 0x17:
    case 0x1D: {   // the turn arrow, spinning (anticlockwise or clockwise)
        bool anticlockwise = action == 0x16 || action == 0x1C;
        float spin = anticlockwise ? chemSpin1 : chemSpin2;
        float s = trk::CrtSin(spin) * 1.4142135f;
        float c = trk::CrtCos(spin) * 1.4142135f;
        float s32 = scale * 32.0f;
        float a = s * s32;
        float cs = s32 * c;
        float v[8];
        void* texture;
        if (anticlockwise) {
            const float w[8] = {a, -cs, -cs, -a, cs, a, -a, cs};
            std::memcpy(v, w, sizeof(v));
            texture = chem->textures != nullptr ? chem->textures[0x10] : nullptr;
        } else {
            const float w[8] = {-a, -cs, cs, -a, -cs, a, a, cs};
            std::memcpy(v, w, sizeof(v));
            texture = Chem_GetButtonTexture(chem, 0x10);
        }
        DrawWholeQuad(x + v[0] * scaleX, y + v[1] * scaleY, x + v[2] * scaleX, y + v[3] * scaleY,
                      x + v[4] * scaleX, y + v[5] * scaleY, x + v[6] * scaleX, y + v[7] * scaleY, texture, colour);
        break;
    }
    case 0x18:
    case 0x1E:     // a half turn's arrow, left and right halves; a quarter's, the top's
    case 0x19:
    case 0x1F:
    case 0x1A:
    case 0x20:
    case 0x1B:
    case 0x21: {
        void* texture = Chem_GetButtonTexture(chem, 0x10);
        float hh = scaleY * half;
        float hw = scaleX * half;
        bool left = action == 0x18 || action == 0x1E || action == 0x1A || action == 0x20;
        bool quarter = action == 0x1A || action == 0x1B || action == 0x20 || action == 0x21;
        DrawArrowCorner(x, y - hh, left ? x - hw : hw + x, quarter ? y : y + hh, texture, colour,
                        quarter ? 0.5f : 1.0f);
        break;
    }
    case 0xE:
    case 0x12: {   // pushed up: the arrow above
        float hh = scaleY * half;
        float sy20 = scaleY * 20.0f;
        float a = (y + hh) - sy20;
        float hw = half * scaleX;
        float c = x + hw;
        float b = x - hw;
        float d = (y - hh) - sy20;
        DrawWholeQuad(b, d, c, d, b, a, c, a, Chem_GetButtonTexture(chem, 0x11), colour);
        break;
    }
    case 0xF:
    case 0x13: {   // down
        float hh = scaleY * half;
        float sy20 = scaleY * 20.0f;
        float a = (y - hh) + sy20;
        float hw = half * scaleX;
        float b = x - hw;
        float c = hw + x;
        float d = (y + hh) + sy20;
        DrawWholeQuad(c, d, b, d, c, a, b, a, Chem_GetButtonTexture(chem, 0x11), colour);
        break;
    }
    case 0x10:
    case 0x14: {   // left
        float hh = scaleY * half;
        float p = y - hh;
        float hw = half * scaleX;
        float sx20 = scaleX * 20.0f;
        float q = (x + hw) - sx20;
        float r = hh + y;
        float s = (x - hw) - sx20;
        DrawWholeQuad(s, r, s, p, q, r, q, p, Chem_GetButtonTexture(chem, 0x11), colour);
        break;
    }
    case 0x11:
    case 0x15: {   // right
        float hh = scaleY * half;
        float a = y + hh;
        float hw = half * scaleX;
        float sx20 = scaleX * 20.0f;
        float b = (x - hw) + sx20;
        float c = y - hh;
        float d = (x + hw) + sx20;
        DrawWholeQuad(d, c, d, a, b, c, b, a, Chem_GetButtonTexture(chem, 0x11), colour);
        break;
    }
    }

    if (re::Flag(highlight) && chem->gameType != 2) {
        void* glow = re::Flag(withGlow) ? Chem_GetButtonTexture(chem, Chem_GlowFor(drawn))
                                        : Chem_GetButtonTexture(chem, 0x14);
        float hh = scaleY * half;
        float hw = half * scaleX;
        DrawWhole(x - hw, y - hh, x + hw, y + hh, glow, colour);
    }
    SetRenderState(9, state9);
    SetRenderState(0xC, stateC);
}

// 0x6E9C60: any other button (of the first 22 actions) pressed instead
uint8_t __fastcall ChemOtherPressed(ClassChem* chem, re::Edx, int action) {
    if (action == 0x22) {
        return 0;
    }
    for (int i = 0; i < 0x22; ++i) {
        if (i != action && i < 0x16 && Chem_IsActionPressed(chem, i) != 0) {
            return 1;
        }
    }
    return 0;
}

// 0x6E9CB0: any of the first 22 actions pressed
uint8_t __fastcall ChemAnyPressed(ClassChem* chem, re::Edx) {
    for (int i = 0; i < 0x16; ++i) {
        if (Chem_IsActionPressed(chem, i) != 0) {
            return 1;
        }
    }
    return 0;
}

// 0x6E9CE0: the arrows spin; in the shop the button pulses (a beat of 0.3: growing over
// the first fifth, easing back) while the same button is due again
void __fastcall ChemUpdatePulse(ClassChem* chem, re::Edx) {
    float step = chemTimeStep;
    float rs = chemRate * step;
    chemPulseTime = chemPulseTime - rs;
    if (chemPulseTime <= 0.0f) {
        chemPulseTime = 0.3f;
    }
    chemSpin1 = step * chemSpinRate1 + chemSpin1;
    chemSpin2 = chemSpinRate2 * step + chemSpin2;
    if (chem->gameType != 2 || chem->sequence < 0) {
        return;
    }
    int seq = chem->sequence;
    int index = chem->index;
    bool pulse = false;
    bool decided = false;
    if (index > 0) {
        const ChemAction& a = chem->actions[seq][index];
        if (a.action == chem->actions[seq][index - 1].action && a.start > chem->time) {
            pulse = a.action >= 0 && a.action < 0x16;
            decided = true;
        }
    }
    if (!decided && index < chem->actionCount[seq]) {
        const ChemAction& a = chem->actions[seq][index];
        if (a.action == chem->actions[seq][index + 1].action && a.start < chem->time) {
            pulse = a.action >= 0 && a.action < 0x16;
        }
    }
    if (!pulse) {
        chemPulseTime = 0.3f;
        chem->pulse = 1.0f;
        return;
    }
    float f = 1.0f - chemPulseTime / 0.3f;
    float g;
    if (0.2f > f) {
        float q = f / 0.2f;
        g = q * q;
    } else {
        float q = float(double(f - 0.2f) / 0.7999999970197678);   // (1 - 0.2f, a double)
        g = 1.0f - q * q;
    }
    chem->pulse = g * 0.20000004768371582f + 1.0f;
}

// a stick's turning this tick from its axes (a, b): past the dead zone its angle, the
// turn since the last tick (wrapped into (-pi, pi]) adding to the turned total
void FollowStick(ChemStick& stick, float a, float b) {
    if (stick.active != 0) {
        stick.lastAngle = stick.angle;
    }
    float m = b * b + a * a;
    if (!(m > 0.25f)) {
        stick.active = 0;
        stick.angle = 0.0f;
        stick.speed = 0.0f;
        stick.startAngle = 0.0f;
        stick.time = 0.0f;
        return;
    }
    float angle = trk::CrtAtan2(a, b);
    stick.angle = angle;
    if (stick.active == 0) {
        stick.lastAngle = angle;
        stick.startAngle = angle;
        stick.time = 0.0f;
        stick.turns = 0.0f;
        stick.turned = 0.0f;
    }
    float d = angle - stick.lastAngle;
    float magnitude = std::fabs(d);
    if (magnitude > 3.1415927f) {
        d = d < 0.0f ? d + chemTwoPi : d - chemTwoPi;
    }
    float rate = chemRate;
    float tick = rate * chemTimeStep;
    stick.speed = d / tick;
    stick.active = 1;
    float tick2 = rate * chemTimeStep;
    stick.time = tick2 + stick.time;
    float turned = d + stick.turned;
    stick.turned = turned;
    stick.turns = turned / chemTwoPi;
}

// a miss: the performance halved toward -1
inline void Miss(ClassChem* chem, float lessOne) {
    chem->performance = lessOne * 0.5f;
    chem->justStarted = 0;
    chem->justFinished = 0;
    chem->missed = 1;
    chem->justFailed = 1;
}

// 0x6E9E90 (virtual +4): the popups (the quit question's yes ends it); then the sticks
// followed, the time on, and the action due judged: a press before its window (but in
// the shop) a miss; its window passed with its input not done a miss (the next action
// due, the sequence over after the last); a press in the window scores how centred it
// is, another button a miss; a stick turn done once turned far enough
void __fastcall ChemUpdate(ClassChem* chem, re::Edx) {
    void* popup = &chem->popupKind;
    if (chem->popupShowing != 0) {
        if (chem->popupKind == 1) {
            if (Popup_Result(popup) == 2) {
                chem->active = 0;
            }
            return;
        }
        if (chem->popupShowing != 0) {
            if (chem->popupKind == 2) {
                Popup_Result(popup);
                return;
            }
            if (chem->popupShowing != 0 && chem->popupKind == 3) {
                if (Popup_Result(popup) == 2) {
                    chem->active = 0;
                }
                return;
            }
        }
    }
    if (chemUserStartResponse != 0 || chem->sequence < 0) {
        return;
    }
    float leftX = Pad_GetValue(0x10, 0);
    float leftY = Pad_GetValue(0x11, 0);
    float rightX = Pad_GetValue(0x12, 0);
    float rightY = Pad_GetValue(0x13, 0);
    FollowStick(chem->sticks[0], leftX, leftY);
    FollowStick(chem->sticks[1], rightX, rightY);

    float tick = chemRate * chemTimeStep;
    float t = tick + chem->time;
    chem->time = t;
    ChemAction& due = Current(chem);
    if (!(due.start < t)) {   // before its window
        if (chem->gameType != 2 && Chem_AnyPressed(chem) != 0) {
            float lessOne = chem->performance - 1.0f;
            chem->completed = 0;
            Miss(chem, lessOne);
        }
        return;
    }
    if (!(due.end >= t)) {   // its window passed
        int index = chem->index + 1;
        chem->time = 0.0f;
        chem->pressTime = 0.0f;
        chem->justStarted = 0;
        chem->justFinished = 0;
        chem->index = index;
        uint8_t wasCompleted = chem->completed;
        chem->completed = 0;
        if (wasCompleted != 0) {
            chem->missed = 0;
        } else {
            chem->missed = 1;
            float lessOne = chem->performance - 1.0f;
            chem->justFailed = 1;
            chem->performance = lessOne * 0.5f;
        }
        if (index >= chem->actionCount[chem->sequence]) {
            Chem_EndGame(chem, 1);
        }
        return;
    }
    if (chem->pressTime == 0.0f && chem->completed == 0) {
        if (Chem_IsActionPressed(chem, due.action) != 0) {
            ChemAction& now = Current(chem);
            chem->pressTime = chem->time;
            chem->missed = 0;
            chem->justFailed = 0;
            chem->justFinished = 0;
            chem->justStarted = 1;
            float f = (chem->time - now.start) / (now.end - now.start);
            float off = f + f - 1.0f;
            float away = std::fabs(off);
            float sum = 1.0f - away + chem->performance;
            chem->performance = sum * 0.5f;
            return;
        }
        if (Chem_OtherPressed(chem, Current(chem).action) != 0) {
            Miss(chem, chem->performance - 1.0f);
        }
        return;
    }
    if (Chem_OtherPressed(chem, due.action) != 0) {
        chem->pressTime = 0.0f;
        float lessOne = chem->performance - 1.0f;
        chem->completed = 0;
        Miss(chem, lessOne);
        return;
    }
    if (chem->missed != 0 || chem->completed != 0) {
        return;
    }
    float leftTurns = chem->sticks[0].turns;
    float rightTurns = chem->sticks[1].turns;
    bool done;
    switch (Current(chem).action) {
    case 0x16:
    case 0x18:
        done = leftTurns < -0.4f;
        break;
    case 0x17:
    case 0x19:
        done = leftTurns > 0.4f;
        break;
    case 0x1A:
        done = leftTurns < -0.2f;
        break;
    case 0x1B:
        done = leftTurns > 0.2f;
        break;
    case 0x1C:
    case 0x1E:
        done = rightTurns < -0.4f;
        break;
    case 0x1D:
    case 0x1F:
        done = rightTurns > 0.4f;
        break;
    case 0x20:
        done = rightTurns < -0.2f;
        break;
    case 0x21:
        done = rightTurns > 0.2f;
        break;
    default:
        done = uint32_t(Current(chem).action) <= 0x22;   // the buttons (and none): done
        break;
    }
    if (done) {
        chem->pressTime = 0.0f;
        chem->missed = 0;
        chem->completed = 1;
        chem->justFinished = 1;
    }
}

inline uint32_t WithAlpha(const uint8_t* rgba, int alpha) {
    uint8_t c[4] = {rgba[0], rgba[1], rgba[2], uint8_t(alpha)};
    return Colour(c);
}
inline float ScrollyScale(const ClassChem* chem) {
    return chem->scrollyOnly != 0 ? 0.6f : 0.5f;
}
// a rect {x1, y1, x2, y2} of the scrolly
inline void ScrollyRect(float x1, float y1, float x2, float y2, const uint8_t* colour) {
    float rect[4] = {x1, y1, x2, y2};
    DrawRect(rect, colour);
}
inline void ScrollyGradient(float x1, float y1, float x2, float y2, const uint8_t* a, const uint8_t* b) {
    float rect[4] = {x1, y1, x2, y2};
    DrawRect4(rect, a, b, a, b);
}

// 0x6EA5B0 (virtual +8): the popup over all; else (with the HUD) the pulse and spins on,
// and with a sequence running: the action due, large, above the middle (glowing once
// done; in its lead-in the one before fading out, the flash's glow behind a miss); then
// the scrolly: a bar the due action's window wide with its edges and ends, the due action
// on it and those after to the screen's right edge, those before leftwards fading
void __fastcall ChemRender(ClassChem* chem, re::Edx) {
    if (chem->popupShowing != 0) {
        Popup_Draw(&chem->popupKind);
        return;
    }
    if (chem->hud == 0) {
        return;
    }
    Chem_UpdatePulse(chem);
    float scaleX = chemScreenScaleX;
    float scaleY = chemScreenScaleY;
    float pos[2];
    ScreenPosition(pos, 0.0f, -98.0f, 1, 2);
    if (chem->sequence < 0) {
        return;
    }
    if (chem->scrollyOnly == 0) {
        int index = chem->index;
        ChemAction* actions = chem->actions[chem->sequence];
        ChemAction& due = actions[index];
        float time = chem->time;
        if (due.start < time && due.end > chem->time) {   // in its window
            uint8_t completed = chem->completed;
            uint32_t colour = Colour(completed != 0 ? chemColourDone : chemColourWaiting);
            if (chem->gameType == 2 && index > 0 && due.action == actions[index - 1].action) {
                Chem_DrawButton(chem, due.action, pos[0], pos[1], WithAlpha(chemColourDone, 0xFF), 1.0f, 1, 1);
            } else {
                Chem_DrawButton(chem, due.action, pos[0], pos[1], colour, 1.0f, completed, 1);
            }
        } else if (chem->gameType != 2) {   // the one before fading over the lead-in's first half
            if (index > 0) {
                float half = due.start * 0.5f;
                if (chem->time < half) {
                    int alpha = int((1.0f - chem->time / half) * 255.0f);
                    if (chem->missed != 0) {
                        chem->flash = half;
                        chem->missed = 0;
                    }
                    int highlight = chem->flash == 0.0f ? 1 : 0;
                    Chem_DrawButton(chem, actions[index - 1].action, pos[0], pos[1],
                                    WithAlpha(chemColourDone, alpha), 1.0f, highlight, 1);
                }
            }
        } else if (index > 0 && due.start > chem->time) {   // the shop's: held while repeated
            int previous = actions[index - 1].action;
            bool same = due.action == previous;
            if (same || due.start * 0.5f > chem->time) {
                float flashTo = 0.0f;
                int alpha = 0xFF;
                if (!same) {
                    float half = due.start * 0.5f;
                    flashTo = half;
                    alpha = int((1.0f - chem->time / half) * 255.0f);
                } else if (chem->missed != 0) {
                    float half = due.start * 0.5f;
                    flashTo = half;
                    alpha = half > chem->time ? int((1.0f - chem->time / half) * 255.0f) : 0;
                }
                if (chem->missed != 0) {
                    chem->flash = flashTo;
                    chem->missed = 0;
                }
                int highlight = chem->flash == 0.0f ? 1 : 0;
                Chem_DrawButton(chem, previous, pos[0], pos[1], WithAlpha(chemColourDone, alpha), 1.0f, highlight, 1);
            }
        }
        if (chem->missed != 0) {
            chem->missed = 0;
            chem->flash = 0.5f;
        }
        if (chem->flash != 0.0f) {   // the flash's glow fading
            float tick = chemRate * chemTimeStep;
            float flash = chem->flash - tick;
            chem->flash = flash;
            if (flash <= 0.0f) {
                chem->flash = 0.0f;
            }
            uint8_t colour[4];
            std::memcpy(colour, chemColourFlash, 4);
            colour[3] = uint8_t(int((chem->flash + chem->flash) * 255.0f));
            float hh = scaleY * 32.0f;
            float hw = 32.0f * scaleX;
            DrawWhole(pos[0] - hw, pos[1] - hh, pos[0] + hw, pos[1] + hh, Minigame_GetTexture(chem, 0x14), colour);
        }
    }
    if (chem->scrollyVisible == 0 && chem->scrollyOnly == 0) {
        return;
    }

    // the scrolly
    float pixPerSec = chem->pixPerSec * chem->speedScale;
    chemScrollyPixPerSec = pixPerSec;
    ChemAction& due = Current(chem);
    float startS = due.start / chem->speedScale;
    float endS = due.end / chem->speedScale;
    int stateC;
    int state7;
    GetRenderState(0xC, &stateC);
    GetRenderState(7, &state7);
    SetRenderState(0xC, 1);
    SetRenderState(7, 2);
    float at[2];
    ScreenPosition(at, 0.0f, chem->scrollyOnly != 0 ? -120.0f : -198.0f, 1, 2);
    float duration = endS - startS;
    {
        float w = duration * chemScrollyPixPerSec * 0.5f;
        float hh = chemScrollyHalfHeight * scaleY;
        ScrollyRect(at[0] - w, at[1] + hh, w + at[0], at[1] - hh, chemColourBar);
    }
    {   // the edges fading out, the window's width again each side
        float wide = chemScrollyPixPerSec * duration;
        float hh = chemScrollyHalfHeight * scaleY;
        float w = wide * 0.5f;
        ScrollyGradient(at[0] - wide, at[1] + hh, (at[0] - w) - 1.0f, at[1] - hh, chemColourBarEdge, chemColourBar);
    }
    {
        float wide = chemScrollyPixPerSec * duration;
        float w = 0.5f * wide;
        float hh = chemScrollyHalfHeight * scaleY;
        ScrollyGradient((w + at[0]) + 1.0f, at[1] + hh, wide + at[0], at[1] - hh, chemColourBar, chemColourBarEdge);
    }
    {   // the window's ends
        float left = at[0] - chemScrollyPixPerSec * duration * 0.5f;
        float hh = chemScrollyHalfHeight * scaleY;
        ScrollyRect(left - 2.0f, hh + at[1], left + 2.0f, at[1] - hh, chemColourMark);
    }
    {
        float right = chemScrollyPixPerSec * duration * 0.5f + at[0];
        float hh = chemScrollyHalfHeight * scaleY;
        ScrollyRect(right - 2.0f, hh + at[1], right + 2.0f, at[1] - hh, chemColourMark);
    }
    {   // the lines along its top and bottom
        float w = chemScrollyPixPerSec * duration * 0.5f;
        float top = at[1] - chemScrollyHalfHeight * scaleY;
        ScrollyRect(at[0] - w, top + 2.0f, w + at[0], top - 1.0f, chemColourLine);
    }
    {
        float w = chemScrollyPixPerSec * duration * 0.5f;
        float bottom = chemScrollyHalfHeight * scaleY + at[1];
        ScrollyRect(at[0] - w, bottom + 1.0f, w + at[0], bottom - 2.0f, chemColourLine);
    }
    SetRenderState(0xC, stateC);
    SetRenderState(7, state7);

    // the due action and those after
    uint8_t completed = chem->completed;
    uint8_t colour[4];
    std::memcpy(colour, chemColourScrolly, 4);
    if (completed != 0) {
        ChemAction& now = Current(chem);
        if (now.start < chem->time && now.end > chem->time) {
            colour[3] = 0xFF;
        }
    }
    float x = ((endS - chem->time) + duration * -0.5f) * chemScrollyPixPerSec + at[0];
    if (x < 640.0f) {
        Chem_DrawButton(chem, Current(chem).action, x, at[1], Colour(colour), ScrollyScale(chem), completed, 1);
        int i = chem->index;
        while (i < chem->actionCount[chem->sequence] - 1 && x < 640.0f) {
            ++i;
            x = chem->actions[chem->sequence][i].end * chemScrollyPixPerSec + x;
            if (x < 640.0f) {
                Chem_DrawButton(chem, chem->actions[chem->sequence][i].action, x, at[1], Colour(chemColourScrolly),
                                ScrollyScale(chem), 0, 0);
            }
        }
    }

    // those before, leftwards (the one just before fading in over the lead-in's first half)
    int index = chem->index;
    if (index <= 0) {
        return;
    }
    std::memcpy(colour, chemColourScrolly, 4);
    ChemAction& now = Current(chem);
    float half = now.start * 0.5f;
    if (chem->time < half) {
        int alpha = chemColourScrolly[3];
        float q = 1.0f - chem->time / half;
        colour[3] = uint8_t(int((255.0f - float(alpha)) * q + float(alpha)));
    }
    x = ((now.end - now.start) * -0.5f - chem->time) * chemScrollyPixPerSec + at[0];
    if (!(x >= 0.0f)) {
        return;
    }
    int highlight = chem->flash == 0.0f ? 1 : 0;
    ChemAction* actions = chem->actions[chem->sequence];
    Chem_DrawButton(chem, actions[index - 1].action, x, at[1], Colour(colour), ScrollyScale(chem), highlight, 1);
    int i = chem->index;
    while (i > 1 && x >= 0.0f) {
        --i;
        actions = chem->actions[chem->sequence];
        x = x - actions[i].end * chemScrollyPixPerSec;
        if (x >= 0.0f) {
            Chem_DrawButton(chem, actions[i - 1].action, x, at[1], Colour(chemColourScrolly), ScrollyScale(chem), 0, 0);
        }
    }
}

// 0x6EB350: an action added to a sequence: due `start` seconds after the last, its
// window `duration` long
void __fastcall ChemAddAction(ClassChem* chem, re::Edx, int sequence, int action, float start, float duration, float a,
                             float b) {
    ChemAction& entry = chem->actions[sequence][chem->actionCount[sequence]];
    entry.action = action;
    entry.start = start;
    entry.end = start + duration;
    entry.unknownC = a;
    entry.unknown10 = a + b;
    chem->actionCount[sequence] += 1;
}

// 0x6EB3F0: the action just failed is `action` (the one before while the time is 0),
// once
uint8_t __fastcall ChemGetActionJustFailed(ClassChem* chem, re::Edx, int action) {
    if (chem->sequence < 0) {
        return 0;
    }
    const ChemAction* entry;
    if (chem->time == 0.0f) {
        if (chem->index == 0) {
            return 0;
        }
        entry = &chem->actions[chem->sequence][chem->index - 1];
    } else {
        entry = &Current(chem);
    }
    if (entry->action == action && chem->justFailed != 0) {
        chem->justFailed = 0;
        return 1;
    }
    return 0;
}

// 0x6EB460: the last action shown: the one before in the lead-in (or in the window
// while not yet done), the due one once done in its window; else -1
int __fastcall ChemGetLastAction(ClassChem* chem, re::Edx) {
    int sequence = chem->sequence;
    if (sequence < 0) {
        return -1;
    }
    int index = chem->index;
    if (index > 0) {
        const ChemAction& due = chem->actions[sequence][index];
        if (due.start > chem->time) {
            return chem->actions[sequence][index - 1].action;
        }
        if (due.end > chem->time && chem->completed == 0) {
            return chem->actions[sequence][index - 1].action;
        }
    }
    const ChemAction& due = chem->actions[sequence][index];
    if (due.end > chem->time && chem->completed != 0) {
        return due.action;
    }
    return -1;
}

inline ClassChem* CurrentChem() {
    return re::PtrCast<ClassChem*>(chemCurrentMinigame);
}

// 0x6EB520: ClassChemAddAction(sequence, action, start, duration)
int __cdecl Native_ClassChemAddAction(lua_State* L) {
    int sequence = Script::GetInt(L, 0);
    int action = Script::GetInt(L, 1);
    float start = Script::GetFloat(L, 2);
    float duration = Script::GetFloat(L, 3);
    Chem_AddAction(chemCurrentMinigame, sequence, action, start, duration, 0.0f, 0.0f);
    return 0;
}

// 0x6EB590: ClassChemStartSeq(sequence) (once the streaming is done: it waits)
int __cdecl Native_ClassChemStartSeq(lua_State* L) {
    int sequence = Script::GetInt(L, 0);
    while (Streaming_IsReady() == 0) {
    }
    ClassChem* chem = CurrentChem();
    chem->time = 0.0f;
    chem->sequence = sequence;
    chem->pressTime = 0.0f;
    chem->index = 0;
    chem->performance = 0.0f;
    chem->completed = 0;
    chem->justStarted = 0;
    chem->flash = 0.0f;
    chem->missed = 0;
    return 0;
}

// the due action is `action` and the flag set: cleared, true
inline uint8_t TakeDueFlag(ClassChem* chem, int action, uint8_t& flag) {
    if (chem->sequence >= 0 && Current(chem).action == action && flag != 0) {
        flag = 0;
        return 1;
    }
    return 0;
}

// 0x6EB5F0: ClassChemGetActionJustStarted(action)
int __cdecl Native_ClassChemGetActionJustStarted(lua_State* L) {
    int action = Script::GetInt(L, 0);
    ClassChem* chem = CurrentChem();
    Script::PushBool(L, TakeDueFlag(chem, action, chem->justStarted));
    return 1;
}

// 0x6EB660: ClassChemGetActionJustFinished(action)
int __cdecl Native_ClassChemGetActionJustFinished(lua_State* L) {
    int action = Script::GetInt(L, 0);
    ClassChem* chem = CurrentChem();
    Script::PushBool(L, TakeDueFlag(chem, action, chem->justFinished));
    return 1;
}

// 0x6EB820: ClassChemSetGameType("CHEM" or "SHOP"; anything else 0)
int __cdecl Native_ClassChemSetGameType(lua_State* L) {
    ClassChem* chem = CurrentChem();
    const char* type = Script::GetString(L, 0);
    if (Stricmp(type, GameString(0x00920C88)) == 0) {   // "CHEM"
        chem->gameType = 1;
        return 0;
    }
    chem->gameType = Stricmp(type, GameString(0x0092BEB4)) != 0 ? 0 : 2;   // "SHOP"
    return 0;
}

}  // namespace

// Untested: the game's code runs until each is checked.
#define CLASSCHEM42_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CLASSCHEM42_REVERSED("ClassChem::EndGame", 0x006E8110, &ChemEndGame);
CLASSCHEM42_REVERSED("ClassChem::SetActiveActions", 0x006E8130, &ChemSetActiveActions);
CLASSCHEM42_REVERSED("ClassChem::ClassChem", 0x006E81E0, &ChemConstruct);
CLASSCHEM42_REVERSED("ClassChem::Start", 0x006E8210, &ChemStart);
CLASSCHEM42_REVERSED("ClassChem::End", 0x006E8660, &ChemEnd);
// Register hazard (regcheck2): the game's 0x6E8960 reads ecx (this) and edx back after
// calling it at 0x6E8965 (the game's leaves both alone); ours is only called from C++
// while 0x6E8960 is hooked too, so keep the two in the same mode.
CLASSCHEM42_REVERSED("ClassChem::MapButton", 0x006E8680, &ChemMapButton);
CLASSCHEM42_REVERSED("ClassChem::PromptIndex", 0x006E8760, &ChemPromptIndex);
CLASSCHEM42_REVERSED("ClassChem::GetButtonTexture", 0x006E8960, &ChemGetButtonTexture);
CLASSCHEM42_REVERSED("ClassChem::GlowFor", 0x006E8990, &ChemGlowFor);
CLASSCHEM42_REVERSED("ClassChem::IsActionPressed", 0x006E89E0, &ChemIsActionPressed);
CLASSCHEM42_REVERSED("ClassChem::Delete", 0x006E8C70, &ChemDelete);
CLASSCHEM42_REVERSED("ClassChem::DrawButton", 0x006E8CA0, &ChemDrawButton);
CLASSCHEM42_REVERSED("ClassChem::OtherPressed", 0x006E9C60, &ChemOtherPressed);
CLASSCHEM42_REVERSED("ClassChem::AnyPressed", 0x006E9CB0, &ChemAnyPressed);
CLASSCHEM42_REVERSED("ClassChem::UpdatePulse", 0x006E9CE0, &ChemUpdatePulse);
CLASSCHEM42_REVERSED("ClassChem::Update", 0x006E9E90, &ChemUpdate);
CLASSCHEM42_REVERSED("ClassChem::Render", 0x006EA5B0, &ChemRender);
CLASSCHEM42_REVERSED("ClassChem::AddAction", 0x006EB350, &ChemAddAction);
CLASSCHEM42_REVERSED("ClassChem::GetActionJustFailed", 0x006EB3F0, &ChemGetActionJustFailed);
CLASSCHEM42_REVERSED("ClassChem::GetLastAction", 0x006EB460, &ChemGetLastAction);
CLASSCHEM42_REVERSED("Native_ClassChemAddAction", 0x006EB520, &Native_ClassChemAddAction);
CLASSCHEM42_REVERSED("Native_ClassChemStartSeq", 0x006EB590, &Native_ClassChemStartSeq);
CLASSCHEM42_REVERSED("Native_ClassChemGetActionJustStarted", 0x006EB5F0, &Native_ClassChemGetActionJustStarted);
CLASSCHEM42_REVERSED("Native_ClassChemGetActionJustFinished", 0x006EB660, &Native_ClassChemGetActionJustFinished);
CLASSCHEM42_REVERSED("Native_ClassChemSetGameType", 0x006EB820, &Native_ClassChemSetGameType);
