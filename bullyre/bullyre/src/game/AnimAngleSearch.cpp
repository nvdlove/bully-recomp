// AM_ channels: where a rotation angle reaches a target (the angle at time t plus `delta`,
// wrapped into 0..360) along the key chains of one channel, for the six key kinds (short,
// packed and float keys at two strides each; keys chain to their previous key by index,
// joints 35 and 71 anchor the two chains searched), and the channel-level dispatch.
// 0x6B7240..0x6B8F13, 0x6B90A0..0x6B928B.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

struct CVector {
    float x, y, z;
};

constexpr float kTime9 = 0.001956947147846222f;
constexpr float kTime16 = 1.5259021893143654e-05f;

GAME_VAR(float, as_minDelta, 0x00BB6124);

GAME_FN(void, __thiscall, NiQuat_ToAngleAxis, 0x00775F50, const float* q, float* angle, CVector* axis);
GAME_FN(void, __thiscall, PackedKey_GetQuat, 0x006B1870, const uint8_t* key, float* out);
GAME_FN(void, __cdecl, ShortKey_GetAngle, 0x006B4AD0, const uint8_t* key, float* angle);
GAME_FN(void, __cdecl, FloatKey_GetAngle, 0x006B5280, const uint8_t* key, float* angle);
GAME_FN(float, __cdecl, PackedAngleAt, 0x006B4B60, const uint8_t* k1, const uint8_t* k2, float t);
GAME_FN(float, __cdecl, PackedTimeAt, 0x006B4D50, const uint8_t* k1, const uint8_t* k2, float angle);
GAME_FN(float, __cdecl, ShortAngleAt, 0x006B4F60, const uint8_t* k1, const uint8_t* k2, float t);
GAME_FN(float, __cdecl, ShortTimeAt, 0x006B50E0, const uint8_t* k1, const uint8_t* k2, float angle);
GAME_FN(float, __cdecl, FloatAngleAt, 0x006B52E0, const uint8_t* k1, const uint8_t* k2, float t);
GAME_FN(float, __cdecl, FloatTimeAt, 0x006B5460, const uint8_t* k1, const uint8_t* k2, float angle);
GAME_FN(int32_t, __thiscall, Channel_FindKey, 0x006B2E00, uint8_t* channel, int32_t base, float t);

struct PackedKeys {
    static uint32_t Previous(const uint8_t* k) { return *reinterpret_cast<const uint32_t*>(k) & 0x7FF; }
    static float Time(const uint8_t* k) {
        return float((*reinterpret_cast<const uint32_t*>(k) >> 11) & 0x1FF) * kTime9;
    }
    static void Angle(const uint8_t* k, float* angle) {
        float q[4];
        CVector axis;
        PackedKey_GetQuat(k, q);
        NiQuat_ToAngleAxis(q, angle, &axis);
        if (axis.z < 0.0f) {
            *angle = 360.0f - *angle;
        }
    }
    static float AngleAt(const uint8_t* a, const uint8_t* b, float t) { return PackedAngleAt(a, b, t); }
    static float TimeAt(const uint8_t* a, const uint8_t* b, float x) { return PackedTimeAt(a, b, x); }
};
struct ShortKeys {
    static uint32_t Previous(const uint8_t* k) { return *reinterpret_cast<const uint16_t*>(k); }
    static float Time(const uint8_t* k) { return float(*reinterpret_cast<const uint16_t*>(k + 2)) * kTime16; }
    static void Angle(const uint8_t* k, float* angle) { ShortKey_GetAngle(k, angle); }
    static float AngleAt(const uint8_t* a, const uint8_t* b, float t) { return ShortAngleAt(a, b, t); }
    static float TimeAt(const uint8_t* a, const uint8_t* b, float x) { return ShortTimeAt(a, b, x); }
};
struct FloatKeys {
    static uint32_t Previous(const uint8_t* k) { return *reinterpret_cast<const uint16_t*>(k); }
    static float Time(const uint8_t* k) { return float(*reinterpret_cast<const uint16_t*>(k + 2)) * kTime16; }
    static void Angle(const uint8_t* k, float* angle) { FloatKey_GetAngle(k, angle); }
    static float AngleAt(const uint8_t* a, const uint8_t* b, float t) { return FloatAngleAt(a, b, t); }
    static float TimeAt(const uint8_t* a, const uint8_t* b, float x) { return FloatTimeAt(a, b, x); }
};

// the first non-zero angle found marks its side found; from 190 up it counts as 360
template <typename Keys>
inline void NoteAngle(const uint8_t* key, bool& found, float* out, float* scratch) {
    Keys::Angle(key, scratch);
    float a = *scratch;
    if (std::fabs(a) >= 0.001f) {
        found = true;
        if (a >= 190.0f) {
            *out = 360.0f;
        }
    }
}

// keys (stride Stride), `last` the key reached; returns the time the angle at t plus delta
// is reached (-1: never), the angle at t in *angleOut; *first / *second the chains' first
// non-zero angles (set to 360 near the wrap)
template <typename Keys, int Stride>
float __cdecl FindAngleTime(uint8_t* keys, uint32_t last, float delta, float t, float* angleOut, float* first,
                            float* second) {
    if (as_minDelta > std::fabs(delta)) {
        return t;
    }
    auto previous = [&](const uint8_t* k) { return keys + Keys::Previous(k) * Stride; };
    uint8_t* anchorA = keys + 35 * Stride;
    uint8_t* anchorB = keys + 71 * Stride;
    uint8_t* cur = keys + last * Stride;
    uint8_t* lo = anchorA;
    uint8_t* hi = anchorB;
    Keys::Angle(anchorA, first);
    Keys::Angle(cur, second);
    bool firstFound = std::fabs(*first) >= 0.001f;
    bool secondFound = std::fabs(*second) >= 0.001f;
    bool found = false;
    if (t >= Keys::Time(anchorA)) {
        found = Keys::Time(anchorB) > t;
    }
    if (!(found && secondFound)) {
        if (t == 1.0f) {
            lo = previous(cur);
            hi = cur;
            found = true;
        }
        while (!(found && secondFound) && cur != keys) {
            if (!found) {
                uint8_t* p = previous(cur);
                if (Keys::Time(p) <= t) {
                    found = true;
                    hi = cur;
                    lo = p;
                }
            }
            if (!secondFound) {
                float scratch;
                NoteAngle<Keys>(cur, secondFound, second, &scratch);
            }
            cur = previous(cur);
        }
    }
    float angle = Keys::AngleAt(lo, hi, t);
    *angleOut = angle;
    angle = angle + delta;
    while (!(angle < 360.0f)) {
        angle = angle - 360.0f;
    }
    while (angle < 0.0f) {
        angle = angle + 360.0f;
    }
    float result = -1.0f;
    bool searchForward = true;
    if (lo != keys) {
        uint8_t* next = hi;
        do {
            if (result != -1.0f) {
                searchForward = false;
                break;
            }
            result = Keys::TimeAt(lo, next, angle);
            next = lo;
            lo = previous(lo);
        } while (lo != keys);
        if (searchForward && result != -1.0f) {
            searchForward = false;
        }
    }
    if (searchForward) {
        uint32_t index = uint32_t(int(hi - keys) / Stride);
        uint8_t* k = hi;
        do {
            uint8_t* p = previous(k);
            if (p == hi) {
                result = Keys::TimeAt(hi, k, angle);
                hi = k;
                if (!firstFound) {
                    float scratch;
                    NoteAngle<Keys>(p, firstFound, first, &scratch);
                }
            }
            index++;
            k += Stride;
        } while (index <= last && result == -1.0f);
    }
    if (!firstFound) {
        uint8_t* chain = anchorB;
        uint32_t index = uint32_t(int(anchorB - keys) / Stride);
        uint8_t* k = anchorB;
        do {
            if (previous(k) == chain) {
                chain = k;
                if (!firstFound) {
                    float scratch;  // (the game uses the delta's slot)
                    NoteAngle<Keys>(k, firstFound, first, &scratch);
                }
            }
            index++;
            k += Stride;
        } while (index <= last && !firstFound);
    }
    return result;
}

// 0x6B90A0: the channel's (+0 the track, +8 the owner, +0xC the duration) time where the
// angle at time t (seconds) plus delta is reached, searching up to `until` (-1: the end)
float __fastcall Channel_FindAngleTime(uint8_t* self, re::Edx, float delta, float t, float until, float* angleOut,
                                       float* first, float* second) {
    uint8_t* track = *reinterpret_cast<re::Ptr32<uint8_t>*>(self);
    if (track == nullptr) {
        *angleOut = 0.0f;
        *first = 0.0f;
        *second = 0.0f;
        return 0.0f;
    }
    float duration = *reinterpret_cast<float*>(self + 0xC);
    t = t / duration;
    if (1.0f < t) {
        t = 1.0f;
    }
    if (until == -1.0f) {
        until = 1.0f;
    } else {
        until = until / duration;
        if (1.0f < until) {
            until = 1.0f;
        }
    }
    uint8_t* owner = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 8);
    int32_t last = Channel_FindKey(self, int32_t(*reinterpret_cast<uint32_t*>(owner + 0x5C)) - 1, until);
    float result = -1.0f;
    uint8_t* keys = *reinterpret_cast<re::Ptr32<uint8_t>*>(track + 0x10);
    using Search = float(__cdecl*)(uint8_t*, uint32_t, float, float, float*, float*, float*);
    Search search = nullptr;
    switch (**reinterpret_cast<re::Ptr32<int32_t>*>(track)) {
    case 1001:
        search = reinterpret_cast<Search>(0x006B7240);
        break;
    case 1002:
        search = reinterpret_cast<Search>(0x006B76A0);
        break;
    case 1000:
        search = reinterpret_cast<Search>(0x006B7C00);
        break;
    case 1003:
        search = reinterpret_cast<Search>(0x006B80B0);
        break;
    case 1004:
        search = reinterpret_cast<Search>(0x006B8510);
        break;
    case 999:
        search = reinterpret_cast<Search>(0x006B8AA0);
        break;
    }
    if (search) {
        result = search(keys, uint32_t(last), delta, t, angleOut, first, second);
    }
    return *reinterpret_cast<float*>(self + 0xC) * result;
}

} // namespace

#define AMANGSEARCH_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMANGSEARCH_REVERSED("AM_ShortKeys12::FindAngleTime", 0x006B7240, (&FindAngleTime<ShortKeys, 0xC>));
AMANGSEARCH_REVERSED("AM_PackedKeys8::FindAngleTime", 0x006B76A0, (&FindAngleTime<PackedKeys, 8>));
AMANGSEARCH_REVERSED("AM_FloatKeys20::FindAngleTime", 0x006B7C00, (&FindAngleTime<FloatKeys, 0x14>));
AMANGSEARCH_REVERSED("AM_ShortKeys20::FindAngleTime", 0x006B80B0, (&FindAngleTime<ShortKeys, 0x14>));
AMANGSEARCH_REVERSED("AM_PackedKeys12::FindAngleTime", 0x006B8510, (&FindAngleTime<PackedKeys, 0xC>));
AMANGSEARCH_REVERSED("AM_FloatKeys32::FindAngleTime", 0x006B8AA0, (&FindAngleTime<FloatKeys, 0x20>));
AMANGSEARCH_REVERSED("AM_Channel::FindAngleTime", 0x006B90A0, &Channel_FindAngleTime);
