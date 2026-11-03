// The combination lock (MGLock, vtable 0x948CF0, minigame kind 10): the right stick
// turns the dial (its angle smoothed over the last eight frames); the dial's notch
// clicks, and stopping on each of the three numbers in turn (right, left, right: the
// second needs a whole turn first) opens it (0x703630..0x7047FF).
//
// The minigame: +0xC4 the dial texture, +0xC8 the three numbers (16, 30, 36), +0xD4 the
// numbers done, +0xD8 the dial's angle, +0x124 the stick is turning it, +0x128 the
// stick's (smoothed) angle, +0x12C last frame's, +0x130 where the turn started, +0x134
// the turn's rate, +0x138 / +0x13C / +0x140 the turn's time, turns and total, +0x144 /
// +0x148 the stick's length now and before, +0x14C its rate, +0x150 the dial's spin (the
// start spins it), +0x154 the turning since the last number, +0x158 the last eight stick
// angles (+0x178 the next slot), +0x17C the shackle's height, +0x180 the lock's scale,
// +0x184 the opening's time, +0x188 the saved 0xC3CD70.
// The x87 runs at 24-bit precision here (the main thread): float throughout (the
// double constants here are floats widened).

#include <cmath>
#include <cstdint>

#include "game/SchoolClasses.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using school::At;

GAME_VAR(float, lockTwoPi, 0x00BC6BFC);
GAME_VAR(float, lockTickRate, 0x00BC6C00);           // (0.02: the tick in seconds)
GAME_VAR(float, lockArrowX, 0x00BC6C04);              // the arrow's offsets: even numbers
GAME_VAR(float, lockArrowY, 0x00BC6C08);
GAME_VAR(float, lockArrowX2, 0x00BC6C0C);             // odd ones
GAME_VAR(float, lockArrowY2, 0x00BC6C10);
GAME_VAR(float, lockStartSpin, 0x00BC6C18);
GAME_VAR(float, lockNotch, 0x00BC6C1C);               // an angle per notch (40 of them)
GAME_VAR(float, lockMaxTurn, 0x00BC6C2C);             // turning further resets the numbers
GAME_VAR(int32_t, lockLastNotch, 0x00BC6C48);         // (-1: none yet)
GAME_VAR(int32_t, lockSpinNotch, 0x020C83D4);
GAME_VAR(float, lockDialOffset, 0x020C83D8);
GAME_VAR(uint32_t, lockColour, 0x020C83DC);
inline const uint8_t* const lockArrowColour = reinterpret_cast<const uint8_t*>(0x020C83E4);
GAME_VAR(int32_t, lockSavedGlobal, 0x00C3CD70);
inline void* const lockButtonMap = reinterpret_cast<void*>(0x00A14810);
inline void* const lockKeys = reinterpret_cast<void*>(0x00BC6B98);
inline void* const lockSounds = reinterpret_cast<void*>(0x00CF3DA0);
GAME_VAR(float, lockScreenScaleX, 0x00A957A8);
GAME_VAR(float, lockScreenScaleY, 0x00A957AC);
GAME_VAR(float, lockTimeStep, 0x00C1A9A4);

GAME_FN(void, __cdecl, Lock_Free, 0x005EEAB0, void* p);
inline int __cdecl Lock_Rand() { return crt::Rand(); }
GAME_FN(float, __cdecl, Lock_LimitAngle, 0x00432EE0, float angle);
GAME_FN(void, __cdecl, Lock_SetTextColour, 0x00424480, const uint8_t* rgba);
GAME_FN(uint8_t*, __thiscall, Lock_RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(uint8_t, __cdecl, Lock_Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(float, __cdecl, Lock_Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(void, __cdecl, Lock_Pad_Rumble, 0x00738680, int pad, int duration, int strength, int);
GAME_FN(void, __thiscall, Lock_ButtonMap_SetKeys, 0x00454D10, void* map, void* keys);
GAME_FN(void, __thiscall, Lock_ButtonMap_RestoreKeys, 0x00454D60, void* map);
GAME_FN(uint8_t*, __thiscall, Lock_Sounds_Play, 0x0059B8A0, void* sounds, uint32_t id, const float* pos, int, uintptr_t,
        int, int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __cdecl, Lock_GetRenderState, 0x00519290, int state, int32_t* out);
GAME_FN(void, __cdecl, Lock_SetRenderState, 0x00519790, int state, int value);
GAME_FN(float*, __cdecl, Lock_ScreenPosition, 0x0052D4F0, float* out, float x, float y, int hAlign, int vAlign);
GAME_FN(void, __cdecl, Lock_DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, Lock_DrawQuadUV, 0x0052CCC0, float x1, float y1, float x2, float y2, float x3, float y3,
        float x4, float y4, void* texture, const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __thiscall, Lock_Minigame_EndSounds, 0x00704DE0, void* mg);
GAME_FN(void, __thiscall, Lock_Minigame_EndTextures, 0x00704FD0, void* mg);
GAME_FN(void, __thiscall, Lock_Minigame_Construct, 0x007059D0, void* mg);
GAME_FN(void, __thiscall, Lock_Minigame_Destruct, 0x00705A50, void* mg);
GAME_FN(void, __thiscall, Lock_Minigame_LoadTextures, 0x00704D30, void* mg, int count,
        const school::ClassTextureEntry* entries, const char* txd);
GAME_FN(void, __thiscall, Lock_Minigame_LoadSounds, 0x00704E90, void* mg, int count,
        const school::ClassSoundEntry* entries, const char* bank);
// (this file's)
GAME_FN(float, __thiscall, Lock_ResetHistory, 0x00703630, void* mg, float angle);
GAME_FN(float, __thiscall, Lock_SmoothAngle, 0x00703850, void* mg, float angle);

inline void LockPlay(uint8_t* mg, int sound) {
    Lock_Sounds_Play(lockSounds, At<uint32_t*>(mg, 0x4C)[sound], nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}
inline void* LockTexture(uint8_t* mg, int index) {
    re::Ptr32<void>* textures = At<re::Ptr32<void>*>(mg, 0x1C);
    return textures != nullptr ? textures[index] : nullptr;
}
// the history's slot index wrapped to 0..7 (the compiler's modulo)
inline int Wrap8(int i) {
    if (i >= 8) {
        i = i - int(uint32_t(i) >> 3) * 8;
    }
    if (i < 0) {
        i = i + int(uint32_t(-1 - i) >> 3) * 8 + 8;
    }
    return i;
}
// a difference of two angles brought within half a turn
inline float WrapDelta(float d, float pi, float twoPi) {
    if (std::fabs(d) > pi) {
        d = d < 0.0f ? d + twoPi : d - twoPi;
    }
    return d;
}

// 0x703630: the angle history filled with one angle (the angle left on the x87 stack:
// the callers pop it)
float __fastcall LockResetHistory(uint8_t* mg, re::Edx, float angle) {
    for (int i = 0; i < 8; i++) {
        At<float>(mg, 0x158 + i * 4) = angle;
    }
    At<int32_t>(mg, 0x178) = 0;
    return angle;
}

// 0x703680 (virtual 4): starts: the dial spun to a random angle, the textures and sounds,
// the controls swapped for the lock's
void __fastcall LockStart(uint8_t* mg, re::Edx) {
    mg[4] = 1;
    mg[5] = 0;
    for (int i = 0; i < 8; i++) {
        At<float>(mg, 0x158 + i * 4) = 0.0f;
    }
    At<int32_t>(mg, 0x178) = 0;
    const school::ClassTextureEntry textures[4] = {
            {0, school::GameString(0x00948D88), school::GameString(0x00948D7C)},   // LockDial
            {1, school::GameString(0x00948D70), school::GameString(0x00948D64)},   // LockBody
            {2, school::GameString(0x00948D5C), school::GameString(0x00948D50)},   // LockArm
            {3, school::GameString(0x00948D44), school::GameString(0x00948D34)},   // LockArrowR
    };
    Lock_Minigame_LoadTextures(mg, 4, textures, school::GameString(0x00948D94));   // MG_Lock
    const school::ClassSoundEntry sounds[3] = {
            {0, school::GameString(0x00948D20)},   // LckNumClick
            {1, school::GameString(0x00948D14)},   // Tumbler1
            {2, school::GameString(0x00948D0C)},   // LckOpen
    };
    Lock_Minigame_LoadSounds(mg, 3, sounds, school::GameString(0x00948D2C));   // LckPick
    At<int32_t>(mg, 0xC4) = 0;
    mg[0x124] = 0;
    At<int32_t>(mg, 0xC8) = 0x10;
    At<int32_t>(mg, 0xCC) = 0x1E;
    At<int32_t>(mg, 0xD0) = 0x24;
    At<int32_t>(mg, 0xD4) = 0;
    float r = float(Lock_Rand());
    At<float>(mg, 0xD8) = r / 32767.0f * 6.28318548f - 3.14159274f;
    At<float>(mg, 0x150) = lockStartSpin;
    At<float>(mg, 0x154) = 0.0f;
    At<float>(mg, 0x17C) = -96.0f;
    At<float>(mg, 0x180) = 0.0f;
    At<float>(mg, 0x184) = 0.0f;
    At<int32_t>(mg, 0x188) = lockSavedGlobal;
    lockSavedGlobal = 0;
    Lock_ButtonMap_SetKeys(lockButtonMap, lockKeys);
}

// 0x703820 (virtual 5): ends (the controls restored)
void __fastcall LockEnd(uint8_t* mg, re::Edx) {
    mg[4] = 0;
    mg[5] = 0;
    Lock_Minigame_EndSounds(mg);
    Lock_Minigame_EndTextures(mg);
    lockSavedGlobal = At<int32_t>(mg, 0x188);
    Lock_ButtonMap_RestoreKeys(lockButtonMap);
}

// 0x703850: an angle into the history; the smoothed angle back (halfway from the oldest
// towards the oldest plus the average step)
float __fastcall LockSmoothAngle(uint8_t* mg, re::Edx, float angle) {
    float* history = &At<float>(mg, 0x158);
    int32_t& next = At<int32_t>(mg, 0x178);
    history[next] = angle;
    next += 1;
    if (next >= 8) {
        next = 0;
    }
    const float pi = 3.14159274f;
    float twoPi = lockTwoPi;
    int oldest = next;
    float sum = 0.0f;
    int i = oldest;
    while (i != Wrap8(oldest - 1)) {
        int n = Wrap8(i + 1);
        float d = WrapDelta(history[n] - history[i], pi, twoPi);
        sum = d + sum;
        i = Wrap8(i + 1);
    }
    float average = sum * 0.125f;
    float last = WrapDelta(history[Wrap8(oldest - 1)] - history[oldest], pi, twoPi);
    float r = last + average;
    r = r * 0.5f;
    r = r + history[oldest];
    if (std::fabs(r) > pi) {
        if (r < 0.0f) {
            return twoPi + r;
        }
        return r - twoPi;
    }
    return r;
}

// 0x703A40: the constructor
uint8_t* __fastcall LockConstruct(uint8_t* mg, re::Edx) {
    Lock_Minigame_Construct(mg);
    At<uint32_t>(mg, 0) = 0x00948CF0;
    At<int32_t>(mg, 0x38) = 4;
    At<int32_t>(mg, 0x3C) = 10;
    return mg;
}

// 0x703A60 (virtual 0): the deleting destructor
uint8_t* __fastcall LockDeletingDestructor(uint8_t* mg, re::Edx, int flags) {
    At<uint32_t>(mg, 0) = 0x00948CF0;
    Lock_Minigame_Destruct(mg);
    if ((flags & 1) != 0) {
        Lock_Free(mg);
    }
    return mg;
}

// 0x703A90 (virtual 2): the shackle, the body, the dial (turned) and, while the lock is
// at its full size, the arrow showing the way to turn
void __fastcall LockDraw(uint8_t* mg, re::Edx) {
    if (mg[5] == 0) {
        return;
    }
    int32_t saved;
    Lock_GetRenderState(9, &saved);
    Lock_SetRenderState(9, 2);
    float k = At<float>(mg, 0x180);
    float sx = lockScreenScaleX;
    float sy = lockScreenScaleY;
    uint32_t colour = lockColour;
    const uint8_t* rgba = reinterpret_cast<const uint8_t*>(&colour);
    float pos[2];
    Lock_ScreenPosition(pos, -130.0f, 80.0f, 2, 1);
    float uv2[2] = {1.0f, 1.0f};
    float uv1[2] = {0.0f, 0.0f};
    // the shackle (texture 2), raised by +0x17C
    float zero = k * 0.0f * sx;
    float armY = At<float>(mg, 0x17C) * k + pos[1];
    float armX = zero + pos[0];
    float armH = sy * 85.0f * k;
    float armW = k * (85.0f * sx);
    Lock_DrawRectUV(armX - armW, armY - armH, armW + armX, armH + armY, LockTexture(mg, 2), rgba, uv1, uv2);
    // the body (texture 1)
    float bodyY = -6.0f * k + pos[1];
    float bodyX = pos[0] + zero;
    float bodyH = sy * 87.5f * k;
    float bodyW = k * (87.5f * sx);
    Lock_DrawRectUV(bodyX - bodyW, bodyY - bodyH, bodyW + bodyX, bodyH + bodyY, LockTexture(mg, 1), rgba, uv1, uv2);
    // the dial (its texture by +0xC4), a square turned by the dial's angle
    float angle = At<float>(mg, 0xD8) + lockDialOffset;
    float s = trk::CrtSin(angle);
    float s2 = s * 1.41421354f;
    float c = trk::CrtCos(angle);
    float c2 = c * 1.41421354f;
    float big = k * 64.0f;
    float a = s2 * big;
    float b = big * c2;
    float quadUV1[2] = {0.0f, 0.0f};
    float quadUV2[2] = {1.0f, 1.0f};
    Lock_DrawQuadUV(pos[0] + sx * -a, pos[1] + sy * -b, b * sx + pos[0], -a * sy + pos[1], -b * sx + pos[0],
                    a * sy + pos[1], a * sx + pos[0], b * sy + pos[1], LockTexture(mg, At<int32_t>(mg, 0xC4)), rgba,
                    quadUV1, quadUV2);
    if (0.5f == At<float>(mg, 0x180) && At<int32_t>(mg, 0xD4) < 3) {
        float k2 = At<float>(mg, 0x180);
        float arrowUV1[2];
        float arrowUV2[2];
        float ox;
        float oy;
        if ((At<int32_t>(mg, 0xD4) & 1) != 0) {
            // (mirrored: turn the other way)
            arrowUV1[0] = 1.0f;
            arrowUV1[1] = 0.0f;
            arrowUV2[0] = 0.0f;
            arrowUV2[1] = 1.0f;
            ox = lockArrowX2;
            oy = lockArrowY2;
        } else {
            arrowUV1[0] = 0.0f;
            arrowUV1[1] = 0.0f;
            arrowUV2[0] = 1.0f;
            arrowUV2[1] = 1.0f;
            ox = lockArrowX;
            oy = lockArrowY;
        }
        float halfH = sy * 32.0f * k2;
        float y = oy * k2 + pos[1];
        float halfW = 32.0f * sx * k2;
        float x = sx * (ox * k2) + pos[0];
        Lock_DrawRectUV(x - halfW, y - halfH, x + halfW, y + halfH, LockTexture(mg, 3), lockArrowColour, arrowUV1,
                        arrowUV2);
    }
    Lock_SetRenderState(9, saved);
}

// 0x703FF0 (virtual 1): the tick (see the file's head)
void __fastcall LockUpdate(uint8_t* mg, re::Edx) {
    float& openTime = At<float>(mg, 0x184);
    if (openTime != 0.0f) {
        // opening: the lock shrinks, then the shackle rises; then it ends
        float t = openTime;
        if (0.2f < t) {
            float q = (t - 0.2f) / 0.3f;
            At<float>(mg, 0x17C) = q * (25.0f * q) - 121.0f;
        } else {
            float q = t / 0.2f;
            q = q * q;
            At<float>(mg, 0x180) = q * 0.5f;
        }
        float step = lockTickRate * lockTimeStep;
        float left = openTime - step;
        openTime = left;
        if (!(left <= 0.0f)) {
            return;
        }
        openTime = 0.0f;
        vc::Call<void>(mg, 0x14);
        return;
    }
    if (Lock_Pad_JustDown(9, 0) != 0) {
        vc::Call<void>(mg, 0x14);
        return;
    }
    float& dial = At<float>(mg, 0xD8);
    float& spin = At<float>(mg, 0x150);
    if (0.0f < spin) {
        // the start's spin: the dial slows down, clicking each notch
        float a = dial - spin * lockTimeStep;
        dial = a;
        dial = Lock_LimitAngle(a);
        float damp = trk::CrtPow(0.9f, lockTimeStep);
        float s = damp * spin;
        spin = s;
        if (s < 0.0001f) {
            spin = 0.0f;
        }
        At<float>(mg, 0x180) = (1.0f - spin / lockStartSpin) * 0.5f;
        float d = dial;
        if (d < 0.0f) {
            d = d + 6.28318548f;
        }
        int notch = int(d / lockNotch);   // (__ftol2)
        if (notch != lockSpinNotch) {
            LockPlay(mg, 0);
            lockSpinNotch = notch;
        }
        return;
    }
    At<float>(mg, 0x180) = 0.5f;
    float x = Lock_Pad_GetValue(0x12, 0);
    float y = Lock_Pad_GetValue(0x13, 0);
    float& stick = At<float>(mg, 0x128);
    float& lastStick = At<float>(mg, 0x12C);
    float& rate = At<float>(mg, 0x134);
    float& turned = At<float>(mg, 0x154);
    int32_t& stage = At<int32_t>(mg, 0xD4);
    lastStick = stick;
    float length2 = x * x + y * y;
    if (double(length2) > 0.809999942779541) {
        float angle = trk::CrtAtan2(x, y);
        stick = angle;
        if (mg[0x124] == 0) {
            Lock_ResetHistory(mg, angle);
            lastStick = stick;
            At<float>(mg, 0x130) = stick;
            At<float>(mg, 0x138) = 0.0f;
            At<float>(mg, 0x13C) = 0.0f;
            At<float>(mg, 0x140) = 0.0f;
        }
        float before = stick;
        float smoothed = Lock_SmoothAngle(mg, stick);
        stick = smoothed;
        if (std::fabs(smoothed - before) > 1.57079637f) {
            uint8_t colour[4];
            Lock_SetTextColour(Lock_RGBA_Set(colour, 0x80, 0x40, 0x80, 0xFF));
        }
        float lastRate = rate;
        float twoPi = lockTwoPi;
        float d = stick - lastStick;
        if (3.14159274f < std::fabs(d)) {
            d = 0.0f > d ? d + twoPi : d - twoPi;
        }
        float tick = lockTickRate * lockTimeStep;
        rate = d / tick;
        if (stage == 0) {
            turned = 0.0f;
        } else {
            float r = rate;
            bool add;
            if ((stage & 1) != 0) {
                add = r < 0.0f;
            } else {
                add = r > 0.0f;
            }
            turned = add ? std::fabs(d) + turned : turned - std::fabs(d);
        }
        if ((lastRate < 0.0f && 0.0f < rate) || (lastRate > 0.0f && 0.0f > rate) || rate == 0.0f) {
            mg[0x124] = 1;
            At<float>(mg, 0x130) = stick;
            At<float>(mg, 0x138) = 0.0f;
            At<float>(mg, 0x13C) = 0.0f;
            At<float>(mg, 0x140) = 0.0f;
        } else {
            float dt = lockTimeStep * lockTickRate;
            At<float>(mg, 0x138) = dt + At<float>(mg, 0x138);
            float total = std::fabs(d) + At<float>(mg, 0x140);
            At<float>(mg, 0x140) = total;
            At<float>(mg, 0x13C) = total / twoPi;
            float a = d + dial;
            dial = a;
            if (std::fabs(a) > 3.14159274f) {
                dial = a < 0.0f ? twoPi + a : a - twoPi;
            }
            mg[0x124] = 1;
        }
    } else {
        mg[0x124] = 0;
        stick = 0.0f;
        rate = 0.0f;
        At<float>(mg, 0x130) = 0.0f;
        At<float>(mg, 0x138) = 0.0f;
    }
    // the notch under the dial's mark, and whether this tick's move passed the number
    float d = dial;
    if (d < 0.0f) {
        d = d + 6.28318548f;
    }
    int notch = int(d / lockNotch);   // (__ftol2)
    int last = lockLastNotch;
    if (last < 0) {
        last = notch;
        lockLastNotch = notch;
    }
    int moved = notch - last;
    if (moved > 30) {
        moved -= 40;
    } else if (moved < -30) {
        moved += 40;
    }
    uint8_t passed = 0;
    int32_t target = At<int32_t>(mg, 0xC8 + stage * 4);
    if (moved > 0) {
        if ((stage & 1) != 0 && turned < -6.28318548f) {
            if ((target > notch - moved && target < notch) || (target > last && target < moved + last)) {
                passed = 1;
            }
        }
    } else if (moved < 0 && (stage & 1) == 0) {
        if ((target < notch - moved && target > notch) || (target < last && target > moved + last)) {
            passed = 1;
        }
    }
    if (notch != last) {
        LockPlay(mg, 0);
        lockLastNotch = notch;
    }
    if (lockMaxTurn < turned) {
        turned = 0.0f;
        stage = 0;
    }
    if (mg[0x124] != 0 && (notch == At<int32_t>(mg, 0xC8 + stage * 4) || passed != 0) && stage < 3 &&
        (stage != 1 || turned < -6.28318548f)) {
        LockPlay(mg, 1);
        Lock_Pad_Rumble(0, 0x96, 0x32, 1);
        turned = 0.0f;
        if (stage < 3) {
            stage += 1;
        }
    }
    float sx2 = Lock_Pad_GetValue(0x12, 0);
    float sy2 = Lock_Pad_GetValue(0x13, 0);
    At<float>(mg, 0x148) = At<float>(mg, 0x144);
    float sum = sx2 * sx2 + sy2 * sy2;
    float length = school::CrtSqrt(std::fabs(sum));
    At<float>(mg, 0x144) = length;
    float tick = lockTickRate * lockTimeStep;
    At<float>(mg, 0x14C) = (length - At<float>(mg, 0x148)) / tick;
    if (stage >= 3) {
        openTime = 0.5f;
        mg[6] = 1;
        LockPlay(mg, 2);
    }
}

} // namespace

#define LOCK44_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

LOCK44_REVERSED("MGLock::ResetHistory_703630", 0x00703630, &LockResetHistory);
LOCK44_REVERSED("MGLock::Start_703680", 0x00703680, &LockStart);
LOCK44_REVERSED("MGLock::End_703820", 0x00703820, &LockEnd);
LOCK44_REVERSED("MGLock::SmoothAngle_703850", 0x00703850, &LockSmoothAngle);
LOCK44_REVERSED("MGLock::MGLock_703A40", 0x00703A40, &LockConstruct);
LOCK44_REVERSED("MGLock::DeletingDestructor_703A60", 0x00703A60, &LockDeletingDestructor);
LOCK44_REVERSED("MGLock::Draw_703A90", 0x00703A90, &LockDraw);
LOCK44_REVERSED("MGLock::Update_703FF0", 0x00703FF0, &LockUpdate);
