// CQuaternion (x, y, z, w): from Euler angles and axis-angle, to Euler angles, slerp.
// (x87 extended precision in the game; the floats it stores are kept as floats here.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01quat {

struct CVector {
    float x, y, z;
};
struct CQuaternion {
    float x, y, z, w;
};
static_assert(sizeof(CQuaternion) == 0x10);

GAME_FN(void, __thiscall, Quat_ToNiTransform, 0x00413E60, const CQuaternion* self, float* transform);
GAME_FN(void, __cdecl, Quat_SlerpAngle, 0x00413DD0, const CQuaternion* a, const CQuaternion* b, float* angle, float* invSin);
GAME_FN(void, __thiscall, Quat_Slerp, 0x00414210, CQuaternion* self, const CQuaternion* a, const CQuaternion* b, float angle,
        float invSin, float t);

float Sin(float a) {
    return static_cast<float>(std::sin(static_cast<double>(a)));
}
float Cos(float a) {
    return static_cast<float>(std::cos(static_cast<double>(a)));
}
// the CRT's _CIatan2 (y, x)
float ATan2(float y, float x) {
    return static_cast<float>(std::atan2(static_cast<double>(y), static_cast<double>(x)));
}

// 0x413ED0 (thiscall): from Euler angles (x, y, z)
void __fastcall Set(CQuaternion* self, re::Edx, const CVector* angles) {
    float hx = static_cast<float>(angles->x * 0.5);
    float hy = static_cast<float>(angles->y * 0.5);
    float hz = static_cast<float>(angles->z * 0.5);
    float cx = Cos(hx);
    float cy = Cos(hy);
    float cz = Cos(hz);
    float sx = Sin(hx);
    float sy = Sin(hy);
    float sz = Sin(hz);
    float cxcy = cx * cy;
    float sxsy = sx * sy;
    float sxcy = sx * cy;
    float cxsy = cx * sy;
    self->w = sz * sxsy + cz * cxcy;
    self->x = cxcy * sz - cz * sxsy;
    self->y = sxcy * cz + cxsy * sz;
    self->z = cxsy * cz - sxcy * sz;
}

// 0x414020 (thiscall): a rotation of `angle` about `axis` (a unit vector)
void __fastcall SetAxisAngle(CQuaternion* self, re::Edx, const CVector* axis, float angle) {
    float half = static_cast<float>(angle * 0.5);
    float s = Sin(half);
    CVector a = *axis;
    self->x = s * a.x;
    self->y = a.y * s;
    self->z = s * a.z;
    self->w = Cos(half);
}

// 0x4140C0 (thiscall): to Euler angles (x, y, z), each in [0, 2pi)
void __fastcall ToEuler(const CQuaternion* self, re::Edx, float* angles) {
    float transform[13];   // the NiTransform's rotation (column-major) and scale
    Quat_ToNiTransform(self, transform);
    const float* r = transform;
    const double zero = 3.1415927410125732 * 0.0 / 180.0;   // (a degree conversion of 0)
    const double twoPi = 6.2831854820251465;
    float z = ATan2(r[3], r[4]);
    if (z < zero) {
        z = static_cast<float>(z + twoPi);
    }
    float sinZ = Sin(z);
    float cosZ = Cos(z);
    float x = ATan2(-r[5], r[3] * sinZ + r[4] * cosZ);
    if (x < zero) {
        x = static_cast<float>(x + twoPi);
    }
    float a = -(r[6] * cosZ - r[7] * sinZ);
    float b = r[0] * cosZ - r[1] * sinZ;
    float y = ATan2(a, b);
    if (y < zero) {
        y = static_cast<float>(y + twoPi);
    }
    angles[0] = x;
    angles[1] = y;
    angles[2] = z;
}

// 0x414210 (thiscall): the slerp of a and b at t, with their angle and 1 / its sine
// (0x413DD0). (The game returns b for any angle from 1 up: its check is on the angle,
// not t; past pi/2 it goes the short way round.)
void __fastcall Slerp(CQuaternion* self, re::Edx, const CQuaternion* a, const CQuaternion* b, float angle, float invSin, float t) {
    if (angle == 0.0f) {
        *self = *a;
        return;
    }
    if (angle >= 1.0f) {
        *self = *b;
        return;
    }
    float sa, sb;
    if (1.5707963705062866f < angle) {
        float other = static_cast<float>(3.1415927410125732 - angle);
        sa = Sin(static_cast<float>((1.0f - t) * other)) * invSin;
        sb = -Sin(other * t) * invSin;
    } else {
        sa = Sin(static_cast<float>((1.0f - t) * angle)) * invSin;
        sb = Sin(angle * t) * invSin;
    }
    float bx = b->x * sb, by = b->y * sb, bz = b->z * sb, bw = b->w * sb;
    float ax = a->x * sa, ay = a->y * sa, az = a->z * sa, aw = a->w * sa;
    self->x = ax + bx;
    self->y = ay + by;
    self->z = az + bz;
    self->w = aw + bw;
}

// 0x4143C0 (thiscall): the slerp of a and b at t
void __fastcall SlerpT(CQuaternion* self, re::Edx, const CQuaternion* a, const CQuaternion* b, float t) {
    float angle, invSin;
    Quat_SlerpAngle(a, b, &angle, &invSin);
    Quat_Slerp(self, a, b, angle, invSin, t);
}

} // namespace c01quat

// ---- registration ----------------------------------------------------------------------

#define QUAT_REVERSED_C01(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01quat {
QUAT_REVERSED_C01("CQuaternion::Set (angles)", 0x00413ED0, &Set);
QUAT_REVERSED_C01("CQuaternion::Set (axis, angle)", 0x00414020, &SetAxisAngle);
QUAT_REVERSED_C01("CQuaternion::Get (angles)", 0x004140C0, &ToEuler);
QUAT_REVERSED_C01("CQuaternion::Slerp (angle)", 0x00414210, &Slerp);
QUAT_REVERSED_C01("CQuaternion::Slerp", 0x004143C0, &SlerpT);
} // namespace c01quat
