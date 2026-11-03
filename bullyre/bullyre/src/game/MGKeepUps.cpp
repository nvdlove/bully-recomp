// Keep-ups (MGKeepUps, vtable 0x948B28, minigame kind 0x11): the script's sequences of
// timed actions (the right stick pushed one of eight ways, or a button), each shown as an
// arrow that fades in before its window and glows while it's open; the player's input in
// the window succeeds it, a miss fails it (0x702180..0x7035F0, with the MGKeepUps
// natives NativesC2.cpp lacks).
//
// The minigame: five sequences of up to 30 actions from +0xC4 (0xC each: the action, its
// window's start and end, in the sequence's time), +0x7CC the sequences' lengths, +0x7E0 /
// +0x7E4 the script's success and failure callbacks, +0x7E8 the sequence running (-1:
// none), +0x7EC the action, +0x7F0 failed, +0x7F1 hit, +0x7F2 the next one just started,
// +0x7F4 the glow's fade, +0x7F8 / +0x7F9 the callbacks armed, +0x7FC the time, +0x800 when
// it was hit. Actions: 0..7 the right stick (up / down / left / right, then the same on
// the other axis pair), 8 none (the button).
// The x87 runs at 24-bit precision here (the main thread): float throughout.

#include <cstdint>

#include "game/SchoolClasses.h"
#include "game/ScriptApi.h"
#include "reversed.h"

namespace {

using school::At;

struct KeepUpsAction {
    int32_t action;   // +0x0
    float start;      // +0x4
    float end;        // +0x8
};
static_assert(sizeof(KeepUpsAction) == 0xC);

constexpr int kKeepUpsActions = 0xC4;
constexpr int kKeepUpsLengths = 0x7CC;
constexpr int kKeepUpsHitFunc = 0x7E0;
constexpr int kKeepUpsMissFunc = 0x7E4;
constexpr int kKeepUpsSeq = 0x7E8;
constexpr int kKeepUpsIndex = 0x7EC;
constexpr int kKeepUpsFailed = 0x7F0;
constexpr int kKeepUpsHit = 0x7F1;
constexpr int kKeepUpsNext = 0x7F2;
constexpr int kKeepUpsGlow = 0x7F4;
constexpr int kKeepUpsMissArmed = 0x7F8;
constexpr int kKeepUpsHitArmed = 0x7F9;
constexpr int kKeepUpsTime = 0x7FC;
constexpr int kKeepUpsHitTime = 0x800;

GAME_VAR(uint8_t*, kuCurrentMinigame, 0x020C8704);
GAME_VAR(float, kuTickRate, 0x00BC6B58);
GAME_VAR(float, kuTimeStep, 0x00C1A9A4);
GAME_VAR(float, kuScreenScaleX, 0x00A957A8);
GAME_VAR(float, kuScreenScaleY, 0x00A957AC);
GAME_VAR(uint32_t, kuColour, 0x020C83C8);       // the arrows' (and the stick icons')
GAME_VAR(uint32_t, kuFailColour, 0x020C83CC);
inline const uint8_t* const kuColourBytes = reinterpret_cast<const uint8_t*>(0x020C83C8);

GAME_FN(void, __cdecl, KU_Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t, __cdecl, KU_ScriptsReady, 0x0042E430);
GAME_FN(float, __cdecl, KU_Pad_GetAxis, 0x00738980, int control, int pad, float deadZone);
GAME_FN(void, __cdecl, KU_GetRenderState, 0x00519290, int state, int32_t* out);
GAME_FN(void, __cdecl, KU_SetRenderState, 0x00519790, int state, int value);
GAME_FN(float*, __cdecl, KU_ScreenPosition, 0x0052D4F0, float* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void*, __cdecl, KU_GetButtonTexture, 0x00514EC0, int button);
GAME_FN(void, __cdecl, KU_DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, KU_DrawQuadUV, 0x0052CCC0, float x1, float y1, float x2, float y2, float x3, float y3,
        float x4, float y4, void* texture, const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void*, __thiscall, KU_Minigame_GetTexture, 0x006D7CB0, void* mg, int index);
GAME_FN(void, __thiscall, KU_Minigame_EndSounds, 0x00704DE0, void* mg);
GAME_FN(void, __thiscall, KU_Minigame_Construct, 0x007059D0, void* mg);
GAME_FN(void, __thiscall, KU_Minigame_Destruct, 0x00705A50, void* mg);
GAME_FN(void, __thiscall, KU_Minigame_LoadTextures, 0x00704D30, void* mg, int count,
        const school::ClassTextureEntry* entries, const char* txd);
GAME_FN(void, __thiscall, KU_Popup_Draw, 0x00706000, void* popup);
GAME_FN(uint8_t, __thiscall, KU_Ref_IsSet, 0x005D80E0, uint32_t* ref);
GAME_FN(lua_State*, __thiscall, KU_Ref_GetState, 0x005D9C20, uint32_t* ref);
GAME_FN(int32_t, __thiscall, KU_Ref_GetId, 0x005D80D0, uint32_t* ref);
GAME_FN(void, __thiscall, KU_Ref_Clear, 0x005D9C50, uint32_t* ref);
GAME_FN(void, __cdecl, KU_Event_Begin, 0x005D8270, lua_State* L);
GAME_FN(void, __cdecl, KU_Lua_GetRef, 0x005D8670, lua_State* L, int ref);
GAME_FN(void, __cdecl, KU_Lua_Call, 0x005D82A0, lua_State* L, int args, int results);
// (this file's)
GAME_FN(uint8_t, __thiscall, KU_FireHit, 0x00702180, void* mg, int32_t action);
GAME_FN(uint8_t, __thiscall, KU_FireMiss, 0x00702220, void* mg, int32_t action);
GAME_FN(uint8_t, __thiscall, KU_ActionDown, 0x007024B0, void* mg, int32_t action);
GAME_FN(void, __thiscall, KU_DrawAction, 0x007025F0, void* mg, int32_t action, float x, float y, uint32_t colour,
        float scale, int glow, int success);
GAME_FN(void, __thiscall, KU_AddAction, 0x007033C0, void* mg, int seq, int32_t action, float start, float duration);

inline KeepUpsAction& ActionAt(uint8_t* mg, int seq, int index) {
    return reinterpret_cast<KeepUpsAction*>(mg + kKeepUpsActions)[index + seq * 30];
}
inline KeepUpsAction& CurrentAction(uint8_t* mg) {
    return ActionAt(mg, At<int32_t>(mg, kKeepUpsSeq), At<int32_t>(mg, kKeepUpsIndex));
}
// the script callback run with one number
void CallWithInt(uint32_t* ref, int32_t value) {
    if (KU_Ref_IsSet(ref) == 0) {
        return;
    }
    lua_State* L = KU_Ref_GetState(ref);
    KU_Event_Begin(L);
    KU_Lua_GetRef(L, KU_Ref_GetId(ref));
    Script::PushInt(L, value);
    KU_Lua_Call(L, 1, 0);
}

// 0x702180: the hit callback (the action's index), once, when the current action is this
uint8_t __fastcall KeepUpsFireHit(uint8_t* mg, re::Edx, int32_t action) {
    if (At<int32_t>(mg, kKeepUpsSeq) < 0 || CurrentAction(mg).action != action || mg[kKeepUpsHitArmed] == 0) {
        return 0;
    }
    mg[kKeepUpsHitArmed] = 0;
    CallWithInt(&At<uint32_t>(mg, kKeepUpsHitFunc), At<int32_t>(mg, kKeepUpsIndex));
    return 1;
}

// 0x702220: the miss callback, once: for the previous action before the new one's time
// starts counting, else for the current one
uint8_t __fastcall KeepUpsFireMiss(uint8_t* mg, re::Edx, int32_t action) {
    int seq = At<int32_t>(mg, kKeepUpsSeq);
    if (seq < 0) {
        return 0;
    }
    int index = At<int32_t>(mg, kKeepUpsIndex);
    if (At<float>(mg, kKeepUpsTime) == 0.0f) {
        if (index == 0 || ActionAt(mg, seq, index - 1).action != action || mg[kKeepUpsMissArmed] == 0) {
            return 0;
        }
        mg[kKeepUpsMissArmed] = 0;
        uint32_t* ref = &At<uint32_t>(mg, kKeepUpsMissFunc);
        if (KU_Ref_IsSet(ref) != 0) {
            lua_State* L = KU_Ref_GetState(ref);
            KU_Event_Begin(L);
            KU_Lua_GetRef(L, KU_Ref_GetId(ref));
            Script::PushInt(L, At<int32_t>(mg, kKeepUpsIndex) - 1);
            KU_Lua_Call(L, 1, 0);
        }
        return 1;
    }
    if (ActionAt(mg, seq, index).action != action || mg[kKeepUpsMissArmed] == 0) {
        return 0;
    }
    mg[kKeepUpsMissArmed] = 0;
    CallWithInt(&At<uint32_t>(mg, kKeepUpsMissFunc), At<int32_t>(mg, kKeepUpsIndex));
    return 1;
}

// 0x702350: the constructor
uint8_t* __fastcall KeepUpsConstruct(uint8_t* mg, re::Edx) {
    KU_Minigame_Construct(mg);
    At<uint32_t>(mg, 0) = 0x00948B28;
    At<uint32_t>(mg, kKeepUpsHitFunc) = 0;
    At<uint32_t>(mg, kKeepUpsMissFunc) = 0;
    At<int32_t>(mg, 0x38) = 4;
    At<int32_t>(mg, 0x3C) = 0x11;
    return mg;
}

// 0x702380 (virtual 4): starts (no sequence), the textures
void __fastcall KeepUpsStart(uint8_t* mg, re::Edx) {
    At<float>(mg, kKeepUpsTime) = 0.0f;
    mg[5] = 0;
    mg[4] = 1;
    At<int32_t>(mg, kKeepUpsSeq) = -1;
    At<int32_t>(mg, kKeepUpsIndex) = -1;
    for (int i = 0; i < 5; i++) {
        At<int32_t>(mg, kKeepUpsLengths + i * 4) = 0;
    }
    At<float>(mg, 0x804) = 1.0f;   // the speed scale
    mg[kKeepUpsMissArmed] = 0;
    mg[kKeepUpsHitArmed] = 0;
    const char* const button = school::GameString(0x00945270);   // HRbutton_x
    const school::ClassTextureEntry textures[8] = {
            {0, school::GameString(0x009451EC), button},   // HRbutton_L3
            {1, school::GameString(0x009451C8), button},   // HRbutton_R3
            {2, school::GameString(0x009451B8), button},   // HRbutton_L3a
            {3, school::GameString(0x009451A8), button},   // HRbutton_R3a
            {4, school::GameString(0x00945174), school::GameString(0x00945160)},   // HRanalog_arrow
            {5, school::GameString(0x00945150), school::GameString(0x00945140)},   // HRbutton_glow
            {6, school::GameString(0x00945130), school::GameString(0x0094511C)},   // HRbutton_sucess
            {7, school::GameString(0x009450FC), school::GameString(0x009450EC)},   // HRbutton_fail
    };
    KU_Minigame_LoadTextures(mg, 8, textures, school::GameString(0x00948B44));   // MG_KpUps
}

// 0x7024A0 (virtual 5): ends
void __fastcall KeepUpsEnd(uint8_t* mg, re::Edx) {
    mg[4] = 0;
    mg[5] = 0;
    KU_Minigame_EndSounds(mg);
}

// 0x7024B0 (ecx unused): the action's input held: the right stick past 0.3 that way (8:
// always)
uint8_t __fastcall KeepUpsActionDown(void*, re::Edx, int32_t action) {
    static const int kAxis[8] = {0x11, 0x11, 0x10, 0x10, 0x13, 0x13, 0x12, 0x12};
    if (uint32_t(action) > 7) {
        return action == 8 ? 1 : 0;
    }
    float value = KU_Pad_GetAxis(kAxis[action], 0, 0.3f);
    if ((action & 1) == 0) {
        return value > 0.3f ? 1 : 0;
    }
    return value < -0.3f ? 1 : 0;
}

// 0x7025B0 (virtual 0): the deleting destructor
uint8_t* __fastcall KeepUpsDeletingDestructor(uint8_t* mg, re::Edx, int flags) {
    At<uint32_t>(mg, 0) = 0x00948B28;
    KU_Ref_Clear(&At<uint32_t>(mg, kKeepUpsMissFunc));
    KU_Ref_Clear(&At<uint32_t>(mg, kKeepUpsHitFunc));
    KU_Minigame_Destruct(mg);
    if ((flags & 1) != 0) {
        KU_Free(mg);
    }
    return mg;
}

// 0x7025F0: an action's arrow (the stick's direction: the arrow texture turned, off the
// stick icon by 44 x 30) and, when asked, the glow or fail ring on it
void __fastcall KeepUpsDrawAction(uint8_t* mg, re::Edx, int32_t action, float x, float y, uint32_t colour, float scale,
                                  int glow, int success) {
    if (action == 8) {
        return;
    }
    int32_t saved;
    KU_GetRenderState(9, &saved);
    KU_SetRenderState(9, 2);
    float sx = kuScreenScaleX;
    float sy = kuScreenScaleY;
    float ox = 0.0f;
    float oy = 0.0f;
    if (uint32_t(action) <= 3) {
        ox = -(scale * 44.0f * sx);
        oy = scale * 30.0f * sy;
    } else if (uint32_t(action - 4) <= 3) {
        ox = scale * 44.0f * sx;
        oy = scale * 30.0f * sy;
    }
    x = ox + x;
    y = oy + y;
    const uint8_t* rgba = reinterpret_cast<const uint8_t*>(&colour);
    if (uint32_t(action) <= 7) {
        float uv1[2] = {0.0f, 0.0f};
        float uv2[2] = {1.0f, 1.0f};
        float h = scale * 64.0f * 0.5f;
        float a = sy * h;   // the half height
        float c = sx * h;   // the half width
        // the offset along the arrow (scaled for the first four, 32 pixels for the rest)
        float b = action <= 3 ? sy * (scale * 32.0f) : sy * 32.0f;
        float d = action <= 3 ? sx * (scale * 32.0f) : sx * 32.0f;
        void* texture = KU_Minigame_GetTexture(mg, 4);
        switch (action & 3) {
        case 0: {   // up
            float bottom = (y + a) - b;
            float right = x + c;
            float left = x - c;
            float top = (y - a) - b;
            KU_DrawQuadUV(left, top, right, top, left, bottom, right, bottom, texture, rgba, uv1, uv2);
            break;
        }
        case 1: {   // down
            float y1 = (y - a) + b;
            float left = x - c;
            float right = c + x;
            float y0 = b + (a + y);
            KU_DrawQuadUV(right, y0, left, y0, right, y1, left, y1, texture, rgba, uv1, uv2);
            break;
        }
        case 2: {   // left
            float y0 = y - a;
            float x1 = (x + c) - d;
            float y1 = a + y;
            float x0 = (x - c) - d;
            KU_DrawQuadUV(x0, y1, x0, y0, x1, y1, x1, y0, texture, rgba, uv1, uv2);
            break;
        }
        default: {   // right
            float y1 = y + a;
            float x1 = (x - c) + d;
            float y0 = y - a;
            float x0 = d + (c + x);
            KU_DrawQuadUV(x0, y0, x0, y1, x1, y0, x1, y1, texture, rgba, uv1, uv2);
            break;
        }
        }
    }
    if (re::Flag(glow)) {
        float uv2[2] = {1.0f, 1.0f};
        float uv1[2] = {0.0f, 0.0f};
        re::Ptr32<void>* textures = At<re::Ptr32<void>*>(mg, 0x1C);
        void* texture;
        if (re::Flag(success)) {
            texture = textures != nullptr ? textures[5] : nullptr;
        } else {
            texture = textures != nullptr ? textures[7] : nullptr;
        }
        float g = scale * 56.0f * 0.5f;
        float gy = sy * g;
        float gx = sx * g;
        KU_DrawRectUV(x - gx, y - gy, x + gx, y + gy, texture, rgba, uv1, uv2);
    }
    KU_SetRenderState(9, saved);
}

// 0x702D50 (virtual 1): the sequence's tick: the time on; past the action's window the
// next one (the sequence succeeds after its last); in it, the action's input hits it
// (once), a button press outside the stick actions fails it; then the callbacks
void __fastcall KeepUpsUpdate(uint8_t* mg, re::Edx) {
    int seq = At<int32_t>(mg, kKeepUpsSeq);
    if (seq <= -1 || seq >= 5) {
        return;
    }
    float step = kuTickRate * kuTimeStep;
    int index = At<int32_t>(mg, kKeepUpsIndex);
    float time = step + At<float>(mg, kKeepUpsTime);
    At<float>(mg, kKeepUpsTime) = time;
    KeepUpsAction& action = ActionAt(mg, seq, index);
    if (!(action.end >= time)) {
        uint8_t failed = mg[kKeepUpsFailed];
        At<float>(mg, kKeepUpsTime) = 0.0f;
        At<float>(mg, kKeepUpsHitTime) = 0.0f;
        int next = index + 1;
        mg[kKeepUpsHit] = 0;
        mg[kKeepUpsHitArmed] = 0;
        At<int32_t>(mg, kKeepUpsIndex) = next;
        mg[kKeepUpsFailed] = 0;
        if (failed != 0) {
            mg[kKeepUpsNext] = 0;
        } else {
            mg[kKeepUpsNext] = 1;
            mg[kKeepUpsMissArmed] = 1;
        }
        if (next >= At<int32_t>(mg, kKeepUpsLengths + seq * 4)) {
            mg[6] = 1;
            At<int32_t>(mg, kKeepUpsSeq) = -1;
            mg[4] = 0;
        }
    } else if (At<float>(mg, kKeepUpsHitTime) == 0.0f && mg[kKeepUpsFailed] == 0) {
        if (KU_ActionDown(mg, action.action) != 0) {
            mg[kKeepUpsHit] = 1;
            At<float>(mg, kKeepUpsHitTime) = At<float>(mg, kKeepUpsTime);
            mg[kKeepUpsNext] = 0;
            mg[kKeepUpsMissArmed] = 0;
            mg[kKeepUpsHitArmed] = 0;
        }
    } else if (mg[kKeepUpsNext] == 0 && mg[kKeepUpsFailed] == 0 && uint32_t(action.action) <= 8) {
        At<float>(mg, kKeepUpsHitTime) = 0.0f;
        mg[kKeepUpsFailed] = 1;
        mg[kKeepUpsNext] = 0;
        mg[kKeepUpsHitArmed] = 1;
    }
    KU_FireHit(mg, CurrentAction(mg).action);
    KU_FireMiss(mg, CurrentAction(mg).action);
}

// 0x702F30 (virtual 2): the stick icons, then the current action's arrow (glowing in its
// window; before it, the previous one fading out over the first half and the current
// fading in)
void __fastcall KeepUpsDraw(uint8_t* mg, re::Edx) {
    if (mg[0xA0] != 0) {
        KU_Popup_Draw(mg + 0x50);
        return;
    }
    if (mg[5] == 0) {
        return;
    }
    float sx = kuScreenScaleX;
    float sy = kuScreenScaleY;
    float pos[2];
    KU_ScreenPosition(pos, 0.0f, -98.0f, 1, 2);
    if (At<int32_t>(mg, kKeepUpsSeq) < 0) {
        return;
    }
    // (the double constants: 22.4, 24 and 35.2 computed in float, a little off)
    float halfH = float(double(sy) * 22.40000033378601);
    float down = float(double(sy) * 24.00000035762787);
    float halfW = float(22.40000033378601 * double(sx));
    float apart = float(double(sx) * 35.200000524520874);
    float uv1[2] = {0.0f, 0.0f};
    float uv2[2] = {1.0f, 1.0f};
    void* left = KU_GetButtonTexture(0x19);
    KU_DrawRectUV((pos[0] - halfW) - apart, down + (pos[1] - halfH), (pos[0] + halfW) - apart,
                  (pos[1] + halfH) + down, left, kuColourBytes, uv1, uv2);
    void* right = KU_GetButtonTexture(0x1A);
    KU_DrawRectUV((pos[0] - halfW) + apart, down + (pos[1] - halfH), (pos[0] + halfW) + apart,
                  (pos[1] + halfH) + down, right, kuColourBytes, uv1, uv2);
    KeepUpsAction& action = CurrentAction(mg);
    float time = At<float>(mg, kKeepUpsTime);
    if (action.start < time && action.end > time) {
        uint32_t colour = mg[kKeepUpsFailed] != 0 ? kuFailColour : kuColour;
        KU_DrawAction(mg, action.action, pos[0], pos[1], colour, 0.8f, mg[kKeepUpsFailed], 1);
    } else if (At<int32_t>(mg, kKeepUpsIndex) > 0 && !(action.start < time)) {
        float half = float(double(action.start) * 0.5);
        if (At<float>(mg, kKeepUpsTime) < half) {
            float fade = 1.0f - At<float>(mg, kKeepUpsTime) / half;
            int alpha = int(float(double(fade) * 255.0));   // (__ftol2)
            if (mg[kKeepUpsNext] != 0) {
                At<float>(mg, kKeepUpsGlow) = half;
                mg[kKeepUpsNext] = 0;
            }
            uint32_t colour = (kuFailColour & 0x00FFFFFF) | (uint32_t(uint8_t(alpha)) << 24);
            int glow = At<float>(mg, kKeepUpsGlow) == 0.0f ? 1 : 0;
            KU_DrawAction(mg, (&action)[-1].action, pos[0], pos[1], colour, 0.8f, glow, 1);
        }
        KeepUpsAction& now = CurrentAction(mg);
        float share = At<float>(mg, kKeepUpsTime) / now.start;
        uint8_t alpha = uint8_t(int32_t(share * float(int32_t(kuColourBytes[3]))));
        uint32_t colour = (kuColour & 0x00FFFFFF) | (uint32_t(alpha) << 24);
        KU_DrawAction(mg, now.action, pos[0], pos[1], colour, 0.8f, 0, 0);
    }
    if (mg[kKeepUpsNext] != 0) {
        mg[kKeepUpsNext] = 0;
        At<float>(mg, kKeepUpsGlow) = 0.5f;
    }
    if (At<float>(mg, kKeepUpsGlow) != 0.0f) {
        float step = kuTickRate * kuTimeStep;
        float remaining = At<float>(mg, kKeepUpsGlow) - step;
        At<float>(mg, kKeepUpsGlow) = remaining;
        if (remaining <= 0.0f) {
            At<float>(mg, kKeepUpsGlow) = 0.0f;
        }
    }
}

// 0x7033C0: an action added to a sequence (no bounds check)
void __fastcall KeepUpsAddAction(uint8_t* mg, re::Edx, int seq, int32_t action, float start, float duration) {
    int count = At<int32_t>(mg, kKeepUpsLengths + seq * 4);
    ActionAt(mg, seq, count).action = action;
    ActionAt(mg, seq, At<int32_t>(mg, kKeepUpsLengths + seq * 4)).start = start;
    float end = start + duration;
    ActionAt(mg, seq, At<int32_t>(mg, kKeepUpsLengths + seq * 4)).end = end;
    At<int32_t>(mg, kKeepUpsLengths + seq * 4) += 1;
}

// 0x703430: the action the player is on (the previous one until the current one's window,
// or while it's open and not failed; the current one once failed); -1 for none
int32_t __fastcall KeepUpsGetLastAction(uint8_t* mg, re::Edx) {
    int seq = At<int32_t>(mg, kKeepUpsSeq);
    if (seq < 0) {
        return -1;
    }
    int index = At<int32_t>(mg, kKeepUpsIndex);
    float time = At<float>(mg, kKeepUpsTime);
    if (index > 0) {
        KeepUpsAction& a = ActionAt(mg, seq, index);
        if (a.start > time) {
            return (&a)[-1].action;
        }
        if (a.end > time && mg[kKeepUpsFailed] == 0) {
            return (&a)[-1].action;
        }
    }
    KeepUpsAction& a = ActionAt(mg, seq, index);
    if (a.end > At<float>(mg, kKeepUpsTime) && mg[kKeepUpsFailed] != 0) {
        return a.action;
    }
    return -1;
}

// MGKeepUpsAddAction(seq, action, start, duration)
int __cdecl Native_MGKeepUpsAddAction(lua_State* L) {
    int seq = Script::GetInt(L, 0);
    int action = Script::GetInt(L, 1);
    float start = Script::GetFloat(L, 2);
    float duration = Script::GetFloat(L, 3);
    KU_AddAction(kuCurrentMinigame, seq, action, start, duration);
    return 0;
}

// MGKeepUpsStartSeq(seq) (spins until 0x42E430 says so)
int __cdecl Native_MGKeepUpsStartSeq(lua_State* L) {
    int seq = Script::GetInt(L, 0);
    while (KU_ScriptsReady() == 0) {
    }
    uint8_t* mg = kuCurrentMinigame;
    At<float>(mg, kKeepUpsTime) = 0.0f;
    At<int32_t>(mg, kKeepUpsSeq) = seq;
    At<float>(mg, kKeepUpsHitTime) = 0.0f;
    At<int32_t>(mg, kKeepUpsIndex) = 0;
    mg[kKeepUpsFailed] = 0;
    At<float>(mg, kKeepUpsGlow) = 0.0f;
    mg[kKeepUpsHit] = 0;
    mg[kKeepUpsNext] = 0;
    return 0;
}

// MGKeepUpsSetFuncs(hit, miss)
int __cdecl Native_MGKeepUpsSetFuncs(lua_State* L) {
    Script::StoreFunctionRef(L, 0, kuCurrentMinigame + kKeepUpsHitFunc);
    Script::StoreFunctionRef(L, 1, kuCurrentMinigame + kKeepUpsMissFunc);
    return 0;
}

} // namespace

#define KEEPUPS44_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

KEEPUPS44_REVERSED("MGKeepUps::FireHit_702180", 0x00702180, &KeepUpsFireHit);
KEEPUPS44_REVERSED("MGKeepUps::FireMiss_702220", 0x00702220, &KeepUpsFireMiss);
KEEPUPS44_REVERSED("MGKeepUps::MGKeepUps_702350", 0x00702350, &KeepUpsConstruct);
KEEPUPS44_REVERSED("MGKeepUps::Start_702380", 0x00702380, &KeepUpsStart);
KEEPUPS44_REVERSED("MGKeepUps::End_7024A0", 0x007024A0, &KeepUpsEnd);
KEEPUPS44_REVERSED("MGKeepUps::ActionDown_7024B0", 0x007024B0, &KeepUpsActionDown);
KEEPUPS44_REVERSED("MGKeepUps::DeletingDestructor_7025B0", 0x007025B0, &KeepUpsDeletingDestructor);
KEEPUPS44_REVERSED("MGKeepUps::DrawAction_7025F0", 0x007025F0, &KeepUpsDrawAction);
KEEPUPS44_REVERSED("MGKeepUps::Update_702D50", 0x00702D50, &KeepUpsUpdate);
KEEPUPS44_REVERSED("MGKeepUps::Draw_702F30", 0x00702F30, &KeepUpsDraw);
KEEPUPS44_REVERSED("MGKeepUps::AddAction_7033C0", 0x007033C0, &KeepUpsAddAction);
KEEPUPS44_REVERSED("MGKeepUps::GetLastAction_703430", 0x00703430, &KeepUpsGetLastAction);
KEEPUPS44_REVERSED("Native_MGKeepUpsAddAction", 0x007034F0, &Native_MGKeepUpsAddAction);
KEEPUPS44_REVERSED("Native_MGKeepUpsStartSeq", 0x00703550, &Native_MGKeepUpsStartSeq);
KEEPUPS44_REVERSED("Native_MGKeepUpsSetFuncs", 0x007035F0, &Native_MGKeepUpsSetFuncs);
