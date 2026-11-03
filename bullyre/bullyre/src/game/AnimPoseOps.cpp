// The AM_ key codecs' shared pose-entry operations (every codec's slots +0xC, +0x10 and
// +0x18; see AnimManagerSetup.cpp): a pose entry is +0 / +4 its keys, +8 the rotation
// quaternion (w, x, y, z), +0x18 the translation. An entry made into a transform (rotation
// matrix, translation), two entries interpolated (slerp, or linear when nearly the same),
// and two combined (quaternion product, translations added). 0x6C7740..0x6C7B76.

#include <cstdint>

#include "game/TrackMath.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// 0x6C7740: out = a combined with b (the quaternion product, the translations summed),
// written as the game does in case out is one of them
void __cdecl CombineEntries(float* out, const float* a, const float* b) {
    // (the entries' floats from +8: a[2..5] the quaternion, a[6..8] the translation)
    out[2] = a[2] * b[2] - ((b[4] * a[4] + b[3] * a[3]) + b[5] * a[5]);
    out[3] = b[5] * a[4] - a[5] * b[4];
    out[4] = a[5] * b[3] - b[5] * a[3];
    out[5] = b[4] * a[3] - b[3] * a[4];
    out[3] = a[2] * b[3] + out[3];
    out[4] = a[2] * b[4] + out[4];
    out[5] = b[5] * a[2] + out[5];
    out[3] = b[2] * a[3] + out[3];
    out[4] = b[2] * a[4] + out[4];
    out[5] = a[5] * b[2] + out[5];
    float x = b[6] + a[6];
    float y = a[7] + b[7];
    float z = a[8] + b[8];
    out[6] = x;
    out[7] = y;
    out[8] = z;
}

// 0x6C7820: an entry made into a transform (the rotation matrix of its quaternion, its
// translation; 12 floats)
void __cdecl EntryToTransform(float* out, const float* entry) {
    float x = entry[3];
    float y = entry[4];
    float z = entry[5];
    float w = entry[2];
    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float zy = z * y;
    float zx = z * x;
    float yx = y * x;
    float xw = x * w;
    float yw = y * w;
    float zw = w * z;
    out[0] = 1.0f - (zz + yy) * 2.0f;
    out[3] = (zw + yx) * 2.0f;
    out[6] = (zx - yw) * 2.0f;
    out[1] = (yx - zw) * 2.0f;
    out[4] = 1.0f - (zz + xx) * 2.0f;
    out[7] = (xw + zy) * 2.0f;
    out[2] = (yw + zx) * 2.0f;
    out[5] = (zy - xw) * 2.0f;
    out[8] = 1.0f - (xx + yy) * 2.0f;
    out[9] = entry[6];
    out[10] = entry[7];
    out[11] = entry[8];
}

// 0x6C7990 (the quaternions and translations in esi / ecx and eax / edx: not hookable):
// the translations lerped, the quaternions slerped (b's flipped to a's side; lerped past a
// dot of 0.999), with the CRT's x87 acos and sin
void Interpolate(const float* aQuat, const float* bQuat, const float* aMove, const float* bMove, float* outQuat,
                 float* outMove, float t) {
    outMove[0] = (bMove[0] - aMove[0]) * t + aMove[0];
    outMove[1] = (bMove[1] - aMove[1]) * t + aMove[1];
    outMove[2] = (bMove[2] - aMove[2]) * t + aMove[2];
    float dot = ((bQuat[2] * aQuat[2] + bQuat[1] * aQuat[1]) + bQuat[3] * aQuat[3]) + aQuat[0] * bQuat[0];
    float q[4] = {bQuat[0], bQuat[1], bQuat[2], bQuat[3]};
    if (dot < 0.0f) {
        dot = -dot;
        q[1] = -q[1];
        q[2] = -q[2];
        q[3] = -q[3];
        q[0] = -q[0];
    }
    float s0 = 1.0f - t;
    float s1;
    if (!(dot >= 0.9990000128746033f)) {
        float theta = crt::Acosf(dot);
        float inverse = crt::Sinf(theta);
        inverse = 1.0f / inverse;
        s0 = theta * s0;
        s0 = crt::Sinf(s0);
        s0 = s0 * inverse;
        s1 = theta * t;
        s1 = crt::Sinf(s1);
        s1 = s1 * inverse;
    } else {
        s1 = t;
    }
    outQuat[1] = s0 * aQuat[1] + q[1] * s1;
    outQuat[2] = aQuat[2] * s0 + q[2] * s1;
    outQuat[3] = aQuat[3] * s0 + q[3] * s1;
    outQuat[0] = s0 * aQuat[0] + s1 * q[0];
}

// 0x6C7B40: two entries interpolated into out
void __cdecl InterpolateEntries(float* out, const float* a, const float* b, float t) {
    Interpolate(a + 2, b + 2, a + 6, b + 6, out + 2, out + 6, t);
}

} // namespace

RE_UNHOOKED("AM_Pose::Interpolate", 0x006C7990);

#define AMPOSEOPS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMPOSEOPS_REVERSED("AM_Pose::CombineEntries", 0x006C7740, &CombineEntries);
AMPOSEOPS_REVERSED("AM_Pose::EntryToTransform", 0x006C7820, &EntryToTransform);
AMPOSEOPS_REVERSED("AM_Pose::InterpolateEntries", 0x006C7B40, &InterpolateEntries);
