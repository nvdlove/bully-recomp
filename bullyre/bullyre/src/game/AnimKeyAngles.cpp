// AM_ keys' rotation angles (degrees, from the rotation's angle-axis: 360 - angle when the
// axis points down) between two keys of a track: the angle at a time, and the time an
// angle is reached (-1: not between them). Angles near 0 next to ones from 190 up count as
// 360 (the wrap). For the three key kinds: packed (0x6B4B60 / 0x6B4D50), 16-bit
// (0x6B4F60 / 0x6B50E0) and float (0x6B52E0 / 0x6B5460). 0x6B4B60..0x6B55F0.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

struct CVector {
    float x, y, z;
};

constexpr float kTime9 = 0.001956947147846222f;
constexpr float kTime16 = 1.5259021893143654e-05f;

GAME_FN(void, __thiscall, NiQuat_ToAngleAxis, 0x00775F50, const float* q, float* angle, CVector* axis);
GAME_FN(void, __thiscall, PackedKey_GetQuat, 0x006B1870, const uint8_t* key, float* out);
GAME_FN(void, __cdecl, ShortKey_GetAngle, 0x006B4AD0, const uint8_t* key, float* angle);
GAME_FN(void, __cdecl, FloatKey_GetAngle, 0x006B5280, const uint8_t* key, float* angle);
GAME_FN(float, __cdecl, LinearMap, 0x00432B60, float x, float y0, float x0, float y1, float x1);

struct PackedKeys {
    static float Time(const uint8_t* key) {
        return float((*reinterpret_cast<const uint32_t*>(key) >> 11) & 0x1FF) * kTime9;
    }
    static void Angle(const uint8_t* key, float* angle) {
        float q[4];
        CVector axis;
        PackedKey_GetQuat(key, q);
        NiQuat_ToAngleAxis(q, angle, &axis);
        if (axis.z < 0.0f) {
            *angle = 360.0f - *angle;
        }
    }
};
struct ShortKeys {
    static float Time(const uint8_t* key) {
        return float(*reinterpret_cast<const uint16_t*>(key + 2)) * kTime16;
    }
    static void Angle(const uint8_t* key, float* angle) {
        ShortKey_GetAngle(key, angle);
    }
};
struct FloatKeys {
    static float Time(const uint8_t* key) {
        return float(*reinterpret_cast<const uint16_t*>(key + 2)) * kTime16;
    }
    static void Angle(const uint8_t* key, float* angle) {
        FloatKey_GetAngle(key, angle);
    }
};

// the two angles across the 0/360 wrap
inline void Unwrap(float& a1, float& a2) {
    if (std::fabs(a1) < 0.001f && std::fabs(a2) >= 190.0f) {
        a1 = 360.0f;
    }
    if (std::fabs(a2) < 0.001f && std::fabs(a1) >= 190.0f) {
        a2 = 360.0f;
    }
}

// the angle at time t (the first key's before it, and past the second key by more than
// 1e-5 too)
template <typename Keys>
float __cdecl AngleAt(const uint8_t* k1, const uint8_t* k2, float t) {
    float t1 = Keys::Time(k1);
    float t2 = Keys::Time(k2);
    float a1, a2;
    Keys::Angle(k1, &a1);
    Keys::Angle(k2, &a2);
    Unwrap(a1, a2);
    if (!(t >= t1)) {
        return a1;
    }
    if (!(t2 > t)) {
        float d = std::fabs(t - t2);
        if (!(d <= 1e-05f)) {
            return a1;
        }
    }
    return LinearMap(t, a1, t1, a2, t2);
}

// the time the angle's reached (-1: not between the keys' angles)
template <typename Keys>
float __cdecl TimeAt(const uint8_t* k1, const uint8_t* k2, float angle) {
    float t1 = Keys::Time(k1);
    float t2 = Keys::Time(k2);
    float a1, a2;
    Keys::Angle(k1, &a1);
    Keys::Angle(k2, &a2);
    Unwrap(a1, a2);
    if (a2 > a1) {
        if (angle >= a1 && angle <= a2) {
            return LinearMap(angle, t1, a1, t2, a2);
        }
    } else if (angle >= a2 && angle <= a1) {
        return LinearMap(angle, t1, a1, t2, a2);
    }
    return -1.0f;
}

// 0x6B5280: a float key's rotation angle (the quaternion at +4)
void __cdecl FloatKeyAngle(const uint8_t* key, float* angle) {
    float q[4];
    for (int i = 0; i < 4; i++) {
        q[i] = reinterpret_cast<const float*>(key + 4)[i];
    }
    CVector axis;
    NiQuat_ToAngleAxis(q, angle, &axis);
    if (axis.z < 0.0f) {
        *angle = 360.0f - *angle;
    }
}

} // namespace

#define AMANGLES_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMANGLES_REVERSED("AM_PackedKey::AngleAt", 0x006B4B60, &AngleAt<PackedKeys>);
AMANGLES_REVERSED("AM_PackedKey::TimeAtAngle", 0x006B4D50, &TimeAt<PackedKeys>);
AMANGLES_REVERSED("AM_ShortKey::AngleAt", 0x006B4F60, &AngleAt<ShortKeys>);
AMANGLES_REVERSED("AM_ShortKey::TimeAtAngle", 0x006B50E0, &TimeAt<ShortKeys>);
AMANGLES_REVERSED("AM_FloatKey::GetAngle", 0x006B5280, &FloatKeyAngle);
AMANGLES_REVERSED("AM_FloatKey::AngleAt", 0x006B52E0, &AngleAt<FloatKeys>);
AMANGLES_REVERSED("AM_FloatKey::TimeAtAngle", 0x006B5460, &TimeAt<FloatKeys>);
