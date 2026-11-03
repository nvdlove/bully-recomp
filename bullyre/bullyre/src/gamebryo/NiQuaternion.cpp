// NiQuaternion (w, x, y, z: 0x10 bytes): construction, negation, scaling, angle-axis and rotation
// matrix conversion, normalization (exact and the fast one Slerp uses), snapping, streaming, the
// viewer string, and the fast Slerp (a counter-warped lerp, then the fast normalize). Also
// NiPoint3::LoadBinary and NiPoint3::GetViewerString, which sit with it.
//
// Main-thread float maths at the game's 24-bit x87 precision: each sum and product is a float
// operation in the game's order. The double constants these use are all exact floats.

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiQuaternion {
    float w, x, y, z;
};
static_assert(sizeof(NiQuaternion) == 0x10);

// the fast normalize's iteration constants (made at start-up: ISQRT_NEIGHBORHOOD's)
GAME_VAR(float, Quaternion_IsqrtA, 0x020E2D3C);
GAME_VAR(float, Quaternion_IsqrtB, 0x020E2D40);
GAME_ARRAY(const int32_t, Quaternion_Next, 0x00BC8A74);   // {1, 2, 0}

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// (this file's, called where modes apply)
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Construct, 0x00775B90, NiQuaternion* self, float w, float x,
        float y, float z);
GAME_FN(void, __thiscall, Quaternion_Snap, 0x007761F0, NiQuaternion* self);
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Times, 0x00776410, const NiQuaternion* self, NiQuaternion* result,
        float scale);
GAME_FN(void, __thiscall, Quaternion_FastNormalize, 0x007764D0, NiQuaternion* self);
GAME_FN(float, __cdecl, Quaternion_CounterWarp, 0x007765B0, float t, float cosAlpha);

// NiQuaternion(w, x, y, z)
NiQuaternion* __fastcall Construct(NiQuaternion* self, re::Edx, float w, float x, float y, float z) {
    self->w = w;
    self->x = x;
    self->y = y;
    self->z = z;
    return self;
}

// operator-() const
NiQuaternion* __fastcall Negate(const NiQuaternion* self, re::Edx, NiQuaternion* result) {
    result->w = -self->w;
    result->x = -self->x;
    result->y = -self->y;
    result->z = -self->z;
    return result;
}

// ToAngleAxis(angle, axis): none for a rotation under 0.001 of axis length
void __fastcall ToAngleAxis(const NiQuaternion* self, re::Edx, float* angle, NiPoint3* axis) {
    float lengthSq = self->x * self->x;
    lengthSq = lengthSq + self->y * self->y;
    lengthSq = lengthSq + self->z * self->z;
    float length = float(crt::Sqrt(lengthSq));
    if (length < 0.001f) {
        *angle = 0.0f;
        axis->x = 0.0f;
        axis->y = 0.0f;
        axis->z = 0.0f;
        return;
    }
    float w = self->w;
    float half;
    if (!(w > -1.0f)) {
        half = 3.14159274f;
    } else if (!(1.0f > w)) {
        half = 0.0f;
    } else {
        half = float(crt::Acos(w));
    }
    *angle = half + half;
    float inverse = 1.0f / length;
    axis->x = self->x * inverse;
    axis->y = inverse * self->y;
    axis->z = inverse * self->z;
}

// FromRotation(matrix): from the trace when it's positive, else from the largest diagonal entry
void __fastcall FromRotation(NiQuaternion* self, re::Edx, const NiMatrix3* rotation) {
    const float* m = &rotation->m[0][0];
    float trace = m[0] + m[4];
    trace = trace + m[8];
    if (trace > 0.0f) {
        float root = float(crt::Sqrt(trace + 1.0f));
        self->w = root * 0.5f;
        float r = 0.5f / root;
        self->x = (m[7] - m[5]) * r;
        self->y = (m[2] - m[6]) * r;
        self->z = (m[3] - m[1]) * r;
        return;
    }
    int i = m[0] < m[4] ? 1 : 0;
    if (m[i * 4] < m[8]) {
        i = 2;
    }
    int j = Quaternion_Next[i];
    int k = Quaternion_Next[j];
    float sum = m[i * 4] - m[j * 4];
    sum = sum - m[k * 4];
    float root = float(crt::Sqrt(sum + 1.0f));
    float* q[3] = {&self->x, &self->y, &self->z};
    *q[i] = root * 0.5f;
    float r = 0.5f / root;
    self->w = (m[k * 3 + j] - m[j * 3 + k]) * r;
    *q[j] = (m[i * 3 + j] + m[j * 3 + i]) * r;
    *q[k] = (m[i * 3 + k] + m[k * 3 + i]) * r;
}

// Snap: components within 1e-8 of zero made 0 (x, y, z, then w; an exact 0 isn't rewritten)
void __fastcall Snap(NiQuaternion* self, re::Edx) {
    const float epsilon = 1e-8f;
    float* c[4] = {&self->x, &self->y, &self->z, &self->w};
    for (float* p : c) {
        float magnitude = *p < 0.0f ? -*p : *p;
        if (magnitude <= epsilon && *p != 0.0f) {   // (NaN: unordered, left alone)
            *p = 0.0f;
        }
    }
}

// Normalize: scaled by the inverse length (operator* through its address)
void __fastcall Normalize(NiQuaternion* self, re::Edx) {
    float lengthSq = self->x * self->x;
    lengthSq = lengthSq + self->w * self->w;
    lengthSq = lengthSq + self->y * self->y;
    lengthSq = lengthSq + self->z * self->z;
    float length = float(crt::Sqrt(lengthSq));
    float inverse = 1.0f / length;
    NiQuaternion scaled;
    const NiQuaternion* result = Quaternion_Times(self, &scaled, inverse);
    *self = *result;
}

// SaveBinary: snapped first
void __fastcall SaveBinary(NiQuaternion* self, re::Edx, void* stream) {
    Quaternion_Snap(self);
    SaveBytes(stream, &self->w, 4);
    SaveBytes(stream, &self->x, 4);
    SaveBytes(stream, &self->y, 4);
    SaveBytes(stream, &self->z, 4);
}

char* __fastcall GetViewerString(const NiQuaternion* self, re::Edx, const char* prefix) {
    size_t size = std::strlen(prefix) + 0x80;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, "%s: (w=%g,x=%g,y=%g,z=%g)", prefix, double(self->w), double(self->x), double(self->y),
              double(self->z));
    return text;
}

// operator*(float) const
NiQuaternion* __fastcall Times(const NiQuaternion* self, re::Edx, NiQuaternion* result, float scale) {
    float z = self->z * scale;
    float y = self->y * scale;
    float x = self->x * scale;
    float w = scale * self->w;
    return Quaternion_Construct(result, w, x, y, z);
}

// FastNormalize: one or two Newton steps of 1/sqrt from a linear guess, by how far the squared
// length is from 1
void __fastcall FastNormalize(NiQuaternion* self, re::Edx) {
    const float neighborhood = 0.959065974f;
    float s = self->x * self->x;
    s = s + self->y * self->y;
    s = s + self->z * self->z;
    s = s + self->w * self->w;
    float a = Quaternion_IsqrtA;
    float b = Quaternion_IsqrtB;
    float k = (s - neighborhood) * b;
    k = k + a;
    if (s <= 0.915211976f) {
        float u = k * k;
        u = u * s;
        float v = (u - neighborhood) * b;
        v = v + a;
        k = k * v;
        if (s <= 0.652119696f) {
            u = k * k;
            u = s * u;
            v = (u - neighborhood) * b;
            v = a + v;
            k = k * v;
        }
    }
    self->x = self->x * k;
    self->y = k * self->y;
    self->z = self->z * k;
    self->w = k * self->w;
}

// CounterWarp(t, cos): the lerp parameter bent so the fast Slerp's angle runs evenly
float __cdecl CounterWarp(float t, float cosAlpha) {
    float a = 1.0f - cosAlpha * 0.822796881f;
    a = a * a;
    float factor = a * 0.585492194f;
    float k = (t + t) - 3.0f;
    k = (t * factor) * k;
    float result = (1.0f + k) + factor;
    return result * t;
}

// Slerp(t, p, q): the lerp with the counter-warped t, normalized (fast)
NiQuaternion* __cdecl Slerp(NiQuaternion* result, float t, const NiQuaternion* p, const NiQuaternion* q) {
    float cosAlpha = p->w * q->w;
    cosAlpha = cosAlpha + p->x * q->x;
    cosAlpha = cosAlpha + p->y * q->y;
    cosAlpha = cosAlpha + p->z * q->z;
    float k;
    if (t <= 0.5f) {
        k = Quaternion_CounterWarp(t, cosAlpha);
    } else {
        float warped = Quaternion_CounterWarp(1.0f - t, cosAlpha);
        k = 1.0f - warped;
    }
    float pz = p->z;
    float py = p->y;
    float px = p->x;
    float pw = p->w;
    float z = pz + (q->z - pz) * k;
    float y = (q->y - py) * k + py;
    float x = (q->x - px) * k + px;
    float w = pw + k * (q->w - pw);
    Quaternion_Construct(result, w, x, y, z);
    Quaternion_FastNormalize(result);
    return result;
}

// ---- NiPoint3

void __fastcall Point3LoadBinary(NiPoint3* self, re::Edx, void* stream) {
    LoadBytes(stream, &self->x, 4);
    LoadBytes(stream, &self->y, 4);
    LoadBytes(stream, &self->z, 4);
}

char* __fastcall Point3GetViewerString(const NiPoint3* self, re::Edx, const char* prefix) {
    size_t size = std::strlen(prefix) + 0x40;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, "%s = (%g,%g,%g)", prefix, double(self->x), double(self->y), double(self->z));
    return text;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIQUAT_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIQUAT_REVERSED("NiQuaternion::NiQuaternion", 0x00775B90, &ni::Construct);
NIQUAT_REVERSED("NiQuaternion::operator-", 0x00775C10, &ni::Negate);
NIQUAT_REVERSED("NiQuaternion::ToAngleAxis", 0x00775F50, &ni::ToAngleAxis);
NIQUAT_REVERSED("NiQuaternion::FromRotation", 0x00776040, &ni::FromRotation);
NIQUAT_REVERSED("NiQuaternion::Snap", 0x007761F0, &ni::Snap);
NIQUAT_REVERSED("NiQuaternion::Normalize", 0x00776290, &ni::Normalize);
NIQUAT_REVERSED("NiQuaternion::SaveBinary", 0x00776310, &ni::SaveBinary);
NIQUAT_REVERSED("NiQuaternion::GetViewerString", 0x007763B0, &ni::GetViewerString);
NIQUAT_REVERSED("NiQuaternion::operator*", 0x00776410, &ni::Times);
NIQUAT_REVERSED("NiQuaternion::FastNormalize", 0x007764D0, &ni::FastNormalize);
NIQUAT_REVERSED("NiQuaternion::CounterWarp", 0x007765B0, &ni::CounterWarp);
NIQUAT_REVERSED("NiQuaternion::Slerp", 0x007766E0, &ni::Slerp);
NIQUAT_REVERSED("NiPoint3::LoadBinary", 0x007768A0, &ni::Point3LoadBinary);
NIQUAT_REVERSED("NiPoint3::GetViewerString", 0x00776920, &ni::Point3GetViewerString);
