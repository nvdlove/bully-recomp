// More of CMatrix (GTA's): rotations by Euler angles, from quaternions, orientation from
// an axis, the inverse and the product; and Gamebryo's NiQuaternion bits next to them.
// (VectorMath.cpp has the rest; x87 extended precision in the game, floats here.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01matrix {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

struct CMatrix {
    float m[16];      // +0 right, forward, up, position (4 floats each)
    re::Ptr32<void> attached;   // +0x40 the Gamebryo transform it updates
};
static_assert(sizeof(CMatrix) == 0x44);

// Gamebryo's NiQuaternion: w first
struct NiQuaternion {
    float w, x, y, z;
};
static_assert(sizeof(NiQuaternion) == 0x10);

GAME_FN(void, __thiscall, Matrix_ToNiTransform, 0x00412AC0, const CMatrix* self, float* ni);
GAME_FN(float, __cdecl, ACos, 0x00413D60, float x);
GAME_FN(NiQuaternion*, __thiscall, NiQuaternion_Construct, 0x00775B90, NiQuaternion* self, float w, float x, float y, float z);
GAME_FN(void, __thiscall, NiQuaternion_ToRotation, 0x00413C50, const NiQuaternion* self, float* rotation);

float Sin(float a) {
    return static_cast<float>(std::sin(static_cast<double>(a)));
}
float Cos(float a) {
    return static_cast<float>(std::cos(static_cast<double>(a)));
}
float Sqrt(float a) {
    return static_cast<float>(std::sqrt(static_cast<double>(a)));
}

// The rotation of Euler angles (x, y, z) as 0x412D00 and 0x413320 build it
struct Rotation {
    float r[9];
};
Rotation EulerRotation(const CVector* angles) {
    float cx = Cos(angles->x);
    float sx = Sin(angles->x);
    float cy = Cos(angles->y);
    float sy = Sin(angles->y);
    float cz = Cos(angles->z);
    float sz = Sin(angles->z);
    Rotation rot;
    rot.r[0] = cy * cz - sy * sx * sz;
    rot.r[1] = sz * cy + cz * sx * sy;
    rot.r[2] = -cx * sy;
    rot.r[3] = -sz * cx;
    rot.r[4] = cz * cx;
    rot.r[5] = sx;
    rot.r[6] = sx * sz * cy + sy * cz;
    rot.r[7] = sy * sz - cz * sx * cy;
    rot.r[8] = cx * cy;
    return rot;
}

// 0x412D00 (thiscall): the rotation of Euler angles, the position zeroed
void __fastcall SetRotate(CMatrix* self, re::Edx, const CVector* angles) {
    Rotation rot = EulerRotation(angles);
    self->m[0] = rot.r[0];
    self->m[1] = rot.r[1];
    self->m[2] = rot.r[2];
    self->m[4] = rot.r[3];
    self->m[5] = rot.r[4];
    self->m[6] = rot.r[5];
    self->m[8] = rot.r[6];
    self->m[9] = rot.r[7];
    self->m[10] = rot.r[8];
    self->m[12] = 0.0f;
    self->m[13] = 0.0f;
    self->m[14] = 0.0f;
}

// 0x412E40 (thiscall): the rotation of a quaternion (x, y, z, w); the rows' 4th floats
// of forward and up get w (the game copies 16-byte rows; the right row's is kept)
void __fastcall SetRotateQuat(CMatrix* self, re::Edx, const float* quat) {
    float x = quat[0], y = quat[1], z = quat[2], w = quat[3];
    float y2 = y + y;
    float z2 = z + z;
    float xx2 = (x + x) * x;
    float wx2 = w * (x + x);
    self->m[0] = 1.0f - (y * y2 + z2 * z);
    self->m[1] = y2 * x + w * z2;
    self->m[2] = z2 * x - w * y2;
    self->m[8] = z2 * x + w * y2;
    self->m[9] = z2 * y - wx2;
    self->m[10] = 1.0f - (xx2 + y * y2);
    self->m[11] = w;
    self->m[4] = y2 * x - w * z2;
    self->m[5] = 1.0f - (z2 * z + xx2);
    self->m[6] = z2 * y + wx2;
    self->m[7] = w;
}

// 0x413020 (thiscall): rotated about x (the position too)
void __fastcall RotateX(CMatrix* self, re::Edx, float angle) {
    float c = Cos(angle);
    float s = Sin(angle);
    for (int r = 0; r < 4; r++) {
        float y = self->m[r * 4 + 1];
        float z = self->m[r * 4 + 2];
        self->m[r * 4 + 1] = c * y - s * z;
        self->m[r * 4 + 2] = z * c + y * s;
    }
}

// 0x413120 (thiscall): rotated about y
void __fastcall RotateY(CMatrix* self, re::Edx, float angle) {
    float c = Cos(angle);
    float s = Sin(angle);
    for (int r = 0; r < 4; r++) {
        float x = self->m[r * 4];
        float z = self->m[r * 4 + 2];
        self->m[r * 4] = s * z + x * c;
        self->m[r * 4 + 2] = z * c - x * s;
    }
}

// 0x413220 (thiscall): rotated about z
void __fastcall RotateZ(CMatrix* self, re::Edx, float angle) {
    float c = Cos(angle);
    float s = Sin(angle);
    for (int r = 0; r < 4; r++) {
        float x = self->m[r * 4];
        float y = self->m[r * 4 + 1];
        self->m[r * 4] = c * x - y * s;
        self->m[r * 4 + 1] = c * y + x * s;
    }
}

// 0x413320 (thiscall): rotated by Euler angles (every row, the position too)
void __fastcall Rotate(CMatrix* self, re::Edx, const CVector* angles) {
    Rotation rot = EulerRotation(angles);
    for (int r = 0; r < 4; r++) {
        float x = self->m[r * 4];
        float y = self->m[r * 4 + 1];
        float z = self->m[r * 4 + 2];
        self->m[r * 4] = rot.r[3] * y + rot.r[0] * x + rot.r[6] * z;
        self->m[r * 4 + 1] = z * rot.r[7] + x * rot.r[1] + rot.r[4] * y;
        self->m[r * 4 + 2] = z * rot.r[8] + x * rot.r[2] + y * rot.r[5];
    }
}

// 0x413600 (thiscall): up set to (x, y, z), right and forward rebuilt from the old
// forward (the 4th float, a vector's padding, unused)
void __fastcall SetUp(CMatrix* self, re::Edx, float x, float y, float z, float) {
    float rx = z * self->m[5] - y * self->m[6];
    float ry = x * self->m[6] - z * self->m[4];
    float rz = y * self->m[4] - x * self->m[5];
    self->m[0] = rx;
    self->m[1] = ry;
    self->m[2] = rz;
    self->m[4] = rz * y - ry * z;
    self->m[5] = z * rx - x * rz;
    self->m[6] = x * ry - y * rx;
    self->m[8] = x;
    self->m[9] = y;
    self->m[10] = z;
}

// 0x4136C0 (thiscall): up set to (x, y, z), forward and right rebuilt from the old
// right
void __fastcall SetUpKeepRight(CMatrix* self, re::Edx, float x, float y, float z, float) {
    float fx = y * self->m[2] - z * self->m[1];
    float fy = self->m[0] * z - x * self->m[2];
    float fz = self->m[1] * x - self->m[0] * y;
    self->m[0] = fy * z - fz * y;
    self->m[1] = x * fz - z * fx;
    self->m[2] = y * fx - x * fy;
    self->m[4] = fx;
    self->m[5] = fy;
    self->m[6] = fz;
    self->m[8] = x;
    self->m[9] = y;
    self->m[10] = z;
}

// 0x413770: the inverse of a rigid transform
CMatrix* __cdecl Invert(const CMatrix* in, CMatrix* out) {
    float r0 = in->m[0], r1 = in->m[1], r2 = in->m[2];
    float f0 = in->m[4], f1 = in->m[5], f2 = in->m[6];
    float u0 = in->m[8], u1 = in->m[9], u2 = in->m[10];
    float px = in->m[12], py = in->m[13], pz = in->m[14];
    out->m[0] = r0;
    out->m[1] = f0;
    out->m[2] = u0;
    out->m[4] = r1;
    out->m[5] = f1;
    out->m[6] = u1;
    out->m[8] = r2;
    out->m[9] = f2;
    out->m[10] = u2;
    out->m[12] = (px * r0 + 0.0f + py * r1 + pz * r2) * -1.0f;
    out->m[13] = (pz * f2 + py * f1 + px * f0 + 0.0f) * -1.0f;
    out->m[14] = (u1 * py + px * u0 + 0.0f + u2 * pz) * -1.0f;
    return out;
}

// 0x4138D0: out = a * b (b's rows through a; no attached matrix)
CMatrix* __cdecl Multiply(CMatrix* out, const CMatrix* a, const CMatrix* b) {
    const float* p = a->m;
    const float* q = b->m;
    out->attached = nullptr;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 3; c++) {
            out->m[r * 4 + c] = p[8 + c] * q[r * 4 + 2] + p[c] * q[r * 4] + p[4 + c] * q[r * 4 + 1];
        }
    }
    out->m[12] += p[12];
    out->m[13] += p[13];
    out->m[14] += p[14];
    return out;
}

// 0x413A20 (thiscall): this *= s, the result copied to out
CVector* __fastcall Vector_Scale(CVector* self, re::Edx, CVector* out, float s) {
    float x = self->x;
    self->x = s * x;
    float y = self->y;
    self->y = y * s;
    float z = self->z;
    self->z = z * s;
    out->x = s * x;
    out->y = y * s;
    out->z = z * s;
    return out;
}

// 0x413A70 (thiscall): GTA's UpdateRW: the attached Gamebryo transform set from this
void __fastcall UpdateRW(CMatrix* self, re::Edx) {
    if (self->attached) {
        Matrix_ToNiTransform(self, static_cast<float*>(self->attached));
    }
}

// 0x413A80 (thiscall): re-orthonormalised: up = right x forward, right = forward x
// up, forward = up x right (each normalised but the last)
void __fastcall Reorthogonalise(CMatrix* self, re::Edx) {
    float* m = self->m;
    float ux = m[6] * m[1] - m[5] * m[2];
    float uy = m[2] * m[4] - m[0] * m[6];
    float uz = m[5] * m[0] - m[4] * m[1];
    float scale = 1.0f / Sqrt(std::fabs(uy * uy + ux * ux + uz * uz));
    ux = scale * ux;
    uy = uy * scale;
    uz = scale * uz;
    float rx = uz * m[5] - uy * m[6];
    float ry = ux * m[6] - m[4] * uz;
    float rz = m[4] * uy - ux * m[5];
    scale = 1.0f / Sqrt(std::fabs(ry * ry + rx * rx + rz * rz));
    rx = scale * rx;
    ry = ry * scale;
    rz = scale * rz;
    m[0] = rx;
    m[1] = ry;
    m[2] = rz;
    m[4] = uy * rz - uz * ry;
    m[5] = uz * rx - ux * rz;
    m[6] = ry * ux - uy * rx;
    m[8] = ux;
    m[9] = uy;
    m[10] = uz;
}

// 0x413C50 (thiscall): NiQuaternion::ToRotation (a column-major NiMatrix3)
void __fastcall ToRotation(const NiQuaternion* self, re::Edx, float* out) {
    float y2 = self->y * 2.0f;
    float z2 = self->z * 2.0f;
    float xw2 = self->x * 2.0f * self->w;
    float xx2 = self->x * self->x * 2.0f;
    out[0] = 1.0f - (y2 * self->y + z2 * self->z);
    out[3] = z2 * self->w + y2 * self->x;
    out[6] = z2 * self->x - y2 * self->w;
    out[1] = y2 * self->x - z2 * self->w;
    out[4] = 1.0f - (xx2 + z2 * self->z);
    out[7] = xw2 + self->y * z2;
    out[2] = y2 * self->w + z2 * self->x;
    out[5] = self->y * z2 - xw2;
    out[8] = 1.0f - (y2 * self->y + xx2);
}

// 0x413D60: acos clamped to [-1, 1]
float __cdecl ACosClamped(float x) {
    if (x >= 1.0f) {
        return 0.0f;
    }
    if (x <= -1.0f) {
        return 3.1415927410125732f;
    }
    return static_cast<float>(std::acos(static_cast<double>(x)));
}

// 0x413DD0: the angle between two quaternions (x, y, z, w) and 1 / its sine (0 for no
// angle), for slerping
void __cdecl SlerpAngle(const float* a, const float* b, float* angle, float* invSin) {
    float dot = a[1] * b[1] + a[0] * b[0] + a[2] * b[2] + a[3] * b[3];
    if (1.0f < dot) {
        dot = 1.0f;
    }
    float theta = ACos(dot);
    *angle = theta;
    if (theta == 0.0f) {
        *invSin = 0.0f;
        return;
    }
    *invSin = 1.0f / Sin(theta);
}

// 0x413E60 (thiscall): a quaternion (x, y, z, w) to a Gamebryo transform's rotation,
// its scale 1 (the translation left)
void __fastcall QuatToNiTransform(const float* self, re::Edx, float* transform) {
    NiQuaternion quat;
    NiQuaternion* q = NiQuaternion_Construct(&quat, self[3], self[0], self[1], self[2]);
    NiQuaternion_ToRotation(q, transform);
    transform[12] = 1.0f;
}

} // namespace c01matrix

// ---- registration ----------------------------------------------------------------------

#define MATRIX_REVERSED_C01(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01matrix {
MATRIX_REVERSED_C01("CMatrix::SetRotate (angles)", 0x00412D00, &SetRotate);
MATRIX_REVERSED_C01("CMatrix::SetRotate (quaternion)", 0x00412E40, &SetRotateQuat);
MATRIX_REVERSED_C01("CMatrix::RotateX", 0x00413020, &RotateX);
MATRIX_REVERSED_C01("CMatrix::RotateY", 0x00413120, &RotateY);
MATRIX_REVERSED_C01("CMatrix::RotateZ", 0x00413220, &RotateZ);
MATRIX_REVERSED_C01("CMatrix::Rotate (angles)", 0x00413320, &Rotate);
MATRIX_REVERSED_C01("CMatrix::SetUp", 0x00413600, &SetUp);
MATRIX_REVERSED_C01("CMatrix::SetUpKeepRight", 0x004136C0, &SetUpKeepRight);
MATRIX_REVERSED_C01("CMatrix::Invert", 0x00413770, &Invert);
MATRIX_REVERSED_C01("CMatrix::operator*", 0x004138D0, &Multiply);
MATRIX_REVERSED_C01("CVector::Scale", 0x00413A20, &Vector_Scale);
MATRIX_REVERSED_C01("CMatrix::UpdateRW", 0x00413A70, &UpdateRW);
MATRIX_REVERSED_C01("CMatrix::Reorthogonalise", 0x00413A80, &Reorthogonalise);
MATRIX_REVERSED_C01("NiQuaternion::ToRotation", 0x00413C50, &ToRotation);
MATRIX_REVERSED_C01("Math::ACosClamped", 0x00413D60, &ACosClamped);
MATRIX_REVERSED_C01("CQuaternion::SlerpAngle", 0x00413DD0, &SlerpAngle);
MATRIX_REVERSED_C01("CQuaternion::ToNiTransform", 0x00413E60, &QuatToNiTransform);
} // namespace c01matrix
