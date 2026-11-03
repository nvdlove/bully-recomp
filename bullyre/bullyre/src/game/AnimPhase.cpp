// The animation phase helpers of the motion tracks (cyclic phases in 0..1 of a cycle
// length): remaps, the blend of two cycles' phases, the mapping of a phase between two
// gaits by their four foot keys, and the signed angle of a heading in the ground plane.
// Pure x87 float code: written in float in the game's operation order (the main thread
// runs at 24-bit precision). 0x72E7E0..0x72F500.

#include <cstdint>

#include "game/TrackMath.h"
#include "reversed.h"

namespace {

struct ApVec {
    float x, y, z;
};
static_assert(sizeof(ApVec) == 0xC);

GAME_VAR(int32_t, apPiecewiseGaits, 0x020C9024);
GAME_FN(float, __cdecl, Ap_Remap, 0x0072EFB0, float v, float a, float b, float c, float d);
GAME_FN(float, __cdecl, Ap_MapLinear, 0x0072EF20, float phase, float offset, float a3, float a4, float a5, float key, float a7, float a8,
        float a9, float target);
GAME_FN(float, __cdecl, Ap_MapPiecewise, 0x0072F010, float phase, float offset, float a3, float key1, float key2, float key3, float a7,
        float target1, float target2, float target3);
GAME_FN(float, __thiscall, Ap_Normalise, 0x00414E10, ApVec* v);
GAME_FN(float, __cdecl, Ap_SafeAcos, 0x00413D60, float x);

bool Unordered(float a, float b) {
    return a != a || b != b;
}

} // namespace

namespace animphase {

// 0x72E7E0: the phase `t` of the way from phase `from` to phase `to` of a cycle of
// `length` (both as times), the first half ramping from `from`, the second on to the
// end of the cycle
float __cdecl BlendPhase(float t, float length, float from, float to) {
    float start = from / length;
    float span = to / length - start;
    if (span < 0.0f) {
        span = span + 1.0f;
    }
    float blended;
    if (!(t > 0.5f)) {
        blended = (t + t) * span;
    } else {
        float h = float(t - 0.5);
        blended = (h + h) * (1.0f - span) + span;
    }
    float phase = blended + start;
    if (1.0f < phase) {
        return (phase - 1.0f) * length;
    }
    return phase * length;
}

// 0x72EF20: a phase mapped through one key: below `key` scaled onto `target`, above it
// from `target` on to 1; then offset, wrapped
float __cdecl MapLinear(float phase, float offset, float, float, float, float key, float, float, float, float target) {
    float mapped;
    if (key <= phase) {
        float above = phase - key;
        float rest = 1.0f - key;
        float f = above / rest;
        mapped = f * (1.0f - target) + target;
    } else {
        float f = phase / key;
        mapped = f * target;
    }
    float result = mapped + offset;
    if (1.0f < result) {
        result = result - 1.0f;
    }
    return result;
}

// 0x72EFB0: v from [a, b] onto [c, d]
float __cdecl Remap(float v, float a, float b, float c, float d) {
    float f = (v - a) / (b - a);
    return f * (d - c) + c;
}

// 0x72F010: a phase mapped through three keys onto three targets (piecewise linear);
// then offset, wrapped
float __cdecl MapPiecewise(float phase, float offset, float, float key1, float key2, float key3, float, float target1, float target2,
                           float target3) {
    float mapped;
    if (key1 >= phase) {
        // (the game's literal "- 0" and "+ 0")
        float f = (phase - 0.0f) / (key1 - 0.0f);
        mapped = f * (target1 - 0.0f) + 0.0f;
    } else if (!Unordered(key1, phase) && key2 >= phase) {
        mapped = Ap_Remap(phase, key1, key2, target1, target2);
    } else if (key2 < phase && key3 >= phase) {
        mapped = Ap_Remap(phase, key2, key3, target2, target3);
    } else {
        float above = phase - key3;
        float rest = 1.0f - key3;
        float f = above / rest;
        mapped = f * (1.0f - target3) + target3;
    }
    float result = mapped + offset;
    if (1.0f < result) {
        result = float(result - 1.0);
    }
    return result;
}

// 0x72F140: a time in a cycle of `length` (its four foot keys) mapped onto the matching
// time in a cycle of `otherLength` (its four keys); keys past their cycle are clamped (not NaNs),
// a time past the cycle is returned as it is
float __cdecl MapGait(float time, float length, float key0, float key1, float key2, float key3, float otherLength, float other0,
                      float other1, float other2, float other3) {
    if (key0 > length) key0 = length;
    if (key1 > length) key1 = length;
    if (key2 > length) key2 = length;
    if (key3 > length) key3 = length;
    if (other0 > otherLength) other0 = otherLength;
    if (other1 > otherLength) other1 = otherLength;
    if (other2 > otherLength) other2 = otherLength;
    if (other3 > otherLength) other3 = otherLength;
    if (!(time <= length)) {
        return time;
    }
    float start = key0 / length;
    float t = time / length;
    float phase = t - start;
    float k1 = key1 / length;
    k1 = k1 - start;
    float k2 = key2 / length;
    k2 = k2 - start;
    float k3 = key3 / length;
    k3 = k3 - start;
    if (phase < 0.0f) phase = phase + 1.0f;
    if (k1 < 0.0f) k1 = k1 + 1.0f;
    if (k2 < 0.0f) k2 = k2 + 1.0f;
    if (k3 < 0.0f) k3 = k3 + 1.0f;
    float otherStart = other0 / otherLength;
    float o1 = other1 / otherLength;
    o1 = o1 - otherStart;
    float o2 = other2 / otherLength;
    o2 = o2 - otherStart;
    float o3 = other3 / otherLength;
    o3 = o3 - otherStart;
    if (o1 < 0.0f) o1 = float(o1 + 1.0);
    if (o2 < 0.0f) o2 = float(o2 + 1.0);
    if (o3 < 0.0f) o3 = float(o3 + 1.0);
    float mapped;
    if (apPiecewiseGaits != 0) {
        mapped = Ap_MapPiecewise(phase, otherStart, start, k1, k2, k3, otherStart, o1, o2, o3);
    } else {
        mapped = Ap_MapLinear(phase, otherStart, start, k1, k2, k3, otherStart, o1, o2, o3);
    }
    return mapped * otherLength;
}

// 0x72F3F0 (stdcall, two vectors by value): the angle (0..2 pi) from heading `dir` to
// `to` in the ground plane (to normalised in place; its normal (y, -x, 0) gives the side)
float __stdcall HeadingAngle(float dirX, float dirY, float, float toX, float toY, float toZ) {
    ApVec to{toX, toY, toZ};
    Ap_Normalise(&to);
    ApVec side{to.y, -to.x, 0.0f};
    Ap_Normalise(&side);
    float cosAngle = dirY * to.y + dirX * to.x;
    float sideDot = dirY * side.y + dirX * side.x;
    if (cosAngle < -1.0f) {
        cosAngle = -1.0f;
    } else if (1.0f < cosAngle) {
        cosAngle = 1.0f;
    }
    float angle = Ap_SafeAcos(cosAngle);
    float off = float(sideDot - 0.0);
    off = off < 0.0f ? -off : off;
    if (off > 0.0001 && 0.0f > sideDot) {
        return float(6.2831854820251465 - angle);
    }
    return angle;
}

// 0x72F500: v from [a, b] onto [c, d], clamped (c at or below a, d at or above b)
float __cdecl RemapClamped(float v, float a, float b, float c, float d) {
    if (b <= v) {
        return d;
    }
    float above = v - a;
    if (!(above > 0.0f)) {
        return c;
    }
    float span = d - c;
    float range = b - a;
    return span / range * above + c;
}

} // namespace animphase

#define ANIMPHASE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ANIMPHASE_REVERSED("AnimPhase::BlendPhase", 0x0072E7E0, &animphase::BlendPhase);
ANIMPHASE_REVERSED("AnimPhase::MapLinear", 0x0072EF20, &animphase::MapLinear);
ANIMPHASE_REVERSED("AnimPhase::Remap", 0x0072EFB0, &animphase::Remap);
ANIMPHASE_REVERSED("AnimPhase::MapPiecewise", 0x0072F010, &animphase::MapPiecewise);
ANIMPHASE_REVERSED("AnimPhase::MapGait", 0x0072F140, &animphase::MapGait);
ANIMPHASE_REVERSED("AnimPhase::HeadingAngle", 0x0072F3F0, &animphase::HeadingAngle);
// register hazard: callers rely on ecx (0x6081D5: fstp [ecx+0x1C]) and edx (0x60B51D:
// mov ecx,[edx+0x38]) surviving this call; the game's version leaves them alone
ANIMPHASE_REVERSED("AnimPhase::RemapClamped", 0x0072F500, &animphase::RemapClamped);
