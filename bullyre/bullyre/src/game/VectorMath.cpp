// The game's vector and matrix helpers (GTA's CVector / CMatrix functions, out of line).
// A matrix is 4 rows of 4 floats: right, forward, up, position (the 4th column mostly
// unused), then the attached matrix at +0x40.
// (The game does these in x87 extended precision; ours round each float operation.)

#include <cmath>
#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace c01math {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

struct CMatrix {
    float m[16];      // +0
    re::Ptr32<void> attached;   // +0x40
};
static_assert(sizeof(CMatrix) == 0x44);

GAME_FN(CVector*, __cdecl, CrossProduct, 0x00414730, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, RecipSqrt, 0x00414C20, float numerator, float value);
GAME_FN(void, __thiscall, Matrix4_Multiply, 0x004120E0, float* self, const float* other);
GAME_FN(void, __thiscall, Matrix_SetRotateXOnly, 0x00412B60, CMatrix* self, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateYOnly, 0x00412BC0, CMatrix* self, float angle);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, CMatrix* self, float angle);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline float* TransformVector(float* out, const CMatrix* matrix, const CVector* v) { return re::CallVec16<0x004146B0, float*>(out, matrix, v); }
// (0x00414C80 stores 16 bytes through `out`: called through re::CallVec16)
static inline float* TransformVector2(float* out, const CMatrix* matrix, const CVector* v) { return re::CallVec16<0x00414C80, float*>(out, matrix, v); }

// the CRT's _CIsin / _CIcos (on the float widened to double)
float Sin(float a) {
    return static_cast<float>(std::sin(static_cast<double>(a)));
}
float Cos(float a) {
    return static_cast<float>(std::cos(static_cast<double>(a)));
}

// 0x4120E0 (thiscall): this = this * other (4x4)
void __fastcall Matrix4Multiply(float* self, re::Edx, const float* other) {
    float result[16];
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            result[r * 4 + c] = other[12 + c] * self[r * 4 + 3] + other[8 + c] * self[r * 4 + 2] +
                                other[4 + c] * self[r * 4 + 1] + other[c] * self[r * 4];
        }
    }
    std::memcpy(self, result, sizeof(result));
}

// 0x412320: out = s * v
CVector* __cdecl ScaleVector(CVector* out, float s, const CVector* v) {
    out->x = s * v->x;
    out->y = v->y * s;
    out->z = s * v->z;
    return out;
}

// 0x412350: out = v * s
CVector* __cdecl ScaleVector2(CVector* out, const CVector* v, float s) {
    out->x = s * v->x;
    out->y = v->y * s;
    out->z = s * v->z;
    return out;
}

// 0x412380: a unit vector from spherical angles (the elevation from the xy plane, the
// heading around z)
void __cdecl FromSpherical(float elevation, float heading, CVector* out) {
    float polar = static_cast<float>(1.5707963705062866 - elevation);   // (pi / 2 as a float, in double)
    float sinPolar = Sin(polar);
    out->x = Cos(heading) * sinPolar;
    out->y = Sin(heading) * sinPolar;
    out->z = Cos(polar);
}

// The Catmull-Rom basis (0x9E3380)
GAME_ARRAY(const float, catmullRomBasis, 0x009E3380);

// 0x412400: a Catmull-Rom spline through p0..p3 at t (between p1 and p2)
CVector* __cdecl CatmullRom(CVector* out, const CVector* p0, const CVector* p1, const CVector* p2, const CVector* p3, float t) {
    // (the points' w column is uninitialised stack in the game; it only reaches the
    // unused 4th column)
    float points[16] = {};
    const CVector* p[4] = {p0, p1, p2, p3};
    for (int i = 0; i < 4; i++) {
        points[i * 4] = p[i]->x;
        points[i * 4 + 1] = p[i]->y;
        points[i * 4 + 2] = p[i]->z;
    }
    float basis[16];
    std::memcpy(basis, catmullRomBasis, sizeof(basis));
    Matrix4_Multiply(basis, points);
    float powers[16] = {};
    powers[0] = 1.0f;
    powers[1] = t;
    powers[2] = t * t;
    powers[3] = t * t * t;
    Matrix4_Multiply(powers, basis);
    out->x = powers[0];
    out->y = powers[1];
    out->z = powers[2];
    return out;
}

// 0x412530: `v` (horizontal) turned by `heading` around z, then tilted by `pitch`
// towards z, normalised
CVector* __cdecl RotateHeadingPitch(CVector* out, CVector v, float heading, float pitch) {
    float cosHeading = Cos(heading);
    float cosPitch = Cos(pitch);
    float sinHeading = Sin(heading);
    float sinPitch = Sin(pitch);
    CVector up = {0.0f, 0.0f, 1.0f};
    CVector tmp;
    const CVector* side = CrossProduct(&tmp, &v, &up);
    CVector sinPart = {sinHeading * side->x, side->y * sinHeading, sinHeading * side->z};
    CVector cosPart = {cosHeading * v.x, v.y * cosHeading, cosHeading * v.z};
    CVector turned = *VectorAdd(&tmp, &cosPart, &sinPart);
    up = {0.0f, 0.0f, 1.0f};
    CVector axis = *CrossProduct(&tmp, &turned, &up);
    const CVector* tilt = CrossProduct(&tmp, &axis, &turned);
    sinPart = {sinPitch * tilt->x, tilt->y * sinPitch, sinPitch * tilt->z};
    cosPart = {cosPitch * turned.x, turned.y * cosPitch, cosPitch * turned.z};
    VectorAdd(out, &cosPart, &sinPart);
    Vector_Normalise(out);
    return out;
}

// 0x412720 (thiscall): the rotation and position copied (not the 4th column); returns this
CMatrix* __fastcall Matrix_CopyOnlyMatrix(CMatrix* self, re::Edx, const CMatrix* other) {
    for (int r = 0; r < 4; r++) {
        self->m[r * 4] = other->m[r * 4];
        self->m[r * 4 + 1] = other->m[r * 4 + 1];
        self->m[r * 4 + 2] = other->m[r * 4 + 2];
    }
    return self;
}

// 0x412770 (thiscall): identity rotation, position zero
void __fastcall Matrix_SetUnity(CMatrix* self, re::Edx) {
    self->m[0] = 1.0f;
    self->m[1] = 0.0f;
    self->m[2] = 0.0f;
    self->m[4] = 0.0f;
    self->m[6] = 0.0f;
    self->m[8] = 0.0f;
    self->m[9] = 0.0f;
    self->m[12] = 0.0f;
    self->m[13] = 0.0f;
    self->m[14] = 0.0f;
    self->m[5] = 1.0f;
    self->m[10] = 1.0f;
}

// 0x4127A0 (thiscall): identity rotation (the position kept)
void __fastcall Matrix_ResetOrientation(CMatrix* self, re::Edx) {
    self->m[0] = 1.0f;
    self->m[1] = 0.0f;
    self->m[2] = 0.0f;
    self->m[4] = 0.0f;
    self->m[6] = 0.0f;
    self->m[8] = 0.0f;
    self->m[9] = 0.0f;
    self->m[5] = 1.0f;
    self->m[10] = 1.0f;
}

// 0x4127C0 (thiscall): identity rotation at a position (4 floats copied)
void __fastcall Matrix_SetTranslate(CMatrix* self, re::Edx, const float* position) {
    Matrix_ResetOrientation(self, nullptr);
    self->m[12] = position[0];
    self->m[13] = position[1];
    self->m[14] = position[2];
    self->m[15] = position[3];
}

// 0x412800 (thiscall): this = other * this (3x4: other's position through this)
void __fastcall Matrix_PreMultiply(CMatrix* self, re::Edx, const float* other) {
    float m[16];
    std::memcpy(m, self->m, sizeof(m));
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            self->m[r * 4 + c] = other[r * 4 + 2] * m[8 + c] + other[r * 4] * m[c] + other[r * 4 + 1] * m[4 + c];
        }
    }
    for (int c = 0; c < 3; c++) {
        self->m[12 + c] = other[14] * m[8 + c] + other[12] * m[c] + other[13] * m[4 + c] + m[12 + c];
    }
}

// 0x4129A0: out = a + b
CVector* __cdecl Add(CVector* out, const CVector* a, const CVector* b) {
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
    return out;
}

// 0x4129D0 (thiscall): operator= (through a copy on the stack; the self-assignment
// check is against that copy, so it always copies)
void __fastcall Vector_Assign(CVector* self, re::Edx, const CVector* other) {
    CVector copy = *other;
    *self = copy;
}

// 0x412A10 (thiscall): the copy constructor (no attached matrix)
CMatrix* __fastcall Matrix_CopyConstruct(CMatrix* self, re::Edx, const CMatrix* other) {
    self->attached = nullptr;
    return Matrix_CopyOnlyMatrix(self, nullptr, other);
}

// 0x412A70 (thiscall): from Gamebryo's transform (a column-major 3x3, then the
// position)
void __fastcall Matrix_FromNiTransform(CMatrix* self, re::Edx, const float* ni) {
    self->m[0] = ni[0];
    self->m[1] = ni[3];
    self->m[2] = ni[6];
    self->m[4] = ni[1];
    self->m[5] = ni[4];
    self->m[6] = ni[7];
    self->m[8] = ni[2];
    self->m[9] = ni[5];
    self->m[10] = ni[8];
    self->m[12] = ni[9];
    self->m[13] = ni[10];
    self->m[14] = ni[11];
}

// 0x412AC0 (thiscall): to Gamebryo's transform
void __fastcall Matrix_ToNiTransform(const CMatrix* self, re::Edx, float* ni) {
    ni[0] = self->m[0];
    ni[3] = self->m[1];
    ni[6] = self->m[2];
    ni[1] = self->m[4];
    ni[4] = self->m[5];
    ni[7] = self->m[6];
    ni[2] = self->m[8];
    ni[5] = self->m[9];
    ni[8] = self->m[10];
    ni[9] = self->m[12];
    ni[10] = self->m[13];
    ni[11] = self->m[14];
}

// 0x412B60 (thiscall)
void __fastcall SetRotateXOnly(CMatrix* self, re::Edx, float angle) {
    float c = Cos(angle);
    float s = Sin(angle);
    self->m[0] = 1.0f;
    self->m[1] = 0.0f;
    self->m[2] = 0.0f;
    self->m[4] = 0.0f;
    self->m[5] = c;
    self->m[6] = s;
    self->m[8] = 0.0f;
    self->m[9] = -s;
    self->m[10] = c;
}

// 0x412BC0 (thiscall)
void __fastcall SetRotateYOnly(CMatrix* self, re::Edx, float angle) {
    float c = Cos(angle);
    float s = Sin(angle);
    self->m[0] = c;
    self->m[1] = 0.0f;
    self->m[2] = -s;
    self->m[4] = 0.0f;
    self->m[5] = 1.0f;
    self->m[6] = 0.0f;
    self->m[9] = 0.0f;
    self->m[8] = s;
    self->m[10] = c;
}

// 0x412C20 (thiscall): (0 exactly skips the sin and cos)
void __fastcall SetRotateZOnly(CMatrix* self, re::Edx, float angle) {
    float c, s;
    if (angle == 0.0f) {
        c = 1.0f;
        s = 0.0f;
    } else {
        c = Cos(angle);
        s = Sin(angle);
    }
    self->m[0] = c;
    self->m[1] = s;
    self->m[2] = 0.0f;
    self->m[4] = -s;
    self->m[5] = c;
    self->m[6] = 0.0f;
    self->m[8] = 0.0f;
    self->m[9] = 0.0f;
    self->m[10] = 1.0f;
}

// 0x412CA0, 0x412CC0, 0x412CE0 (thiscall): the rotation, the position zeroed
void __fastcall SetRotateX(CMatrix* self, re::Edx, float angle) {
    Matrix_SetRotateXOnly(self, angle);
    self->m[12] = 0.0f;
    self->m[13] = 0.0f;
    self->m[14] = 0.0f;
}
void __fastcall SetRotateY(CMatrix* self, re::Edx, float angle) {
    Matrix_SetRotateYOnly(self, angle);
    self->m[12] = 0.0f;
    self->m[13] = 0.0f;
    self->m[14] = 0.0f;
}
void __fastcall SetRotateZ(CMatrix* self, re::Edx, float angle) {
    Matrix_SetRotateZOnly(self, angle);
    self->m[12] = 0.0f;
    self->m[13] = 0.0f;
    self->m[14] = 0.0f;
}

// 0x414470: the smaller (b when equal)
float __cdecl Min(float a, float b) {
    return a < b ? a : b;
}

// 0x414490: the larger (b when equal)
float __cdecl Max(float a, float b) {
    return b < a ? a : b;
}

// 0x414640
float __cdecl DotProduct(const CVector* a, const CVector* b) {
    return a->y * b->y + a->x * b->x + a->z * b->z;
}

// 0x414670 (thiscall): this += v, the result copied to out
CVector* __fastcall Vector_AddTo(CVector* self, re::Edx, CVector* out, const CVector* v) {
    self->x = v->x + self->x;
    self->y = v->y + self->y;
    float z = v->z + self->z;
    self->z = z;
    out->x = self->x;
    out->y = self->y;
    out->z = z;
    return out;
}

// 0x4146B0: the direction through the matrix (row vector times the rotation; the game
// also writes out[3], from uninitialised stack: left alone here)
float* __cdecl TransformVectorImpl(float* out, const CMatrix* m, const CVector* v) {
    float x = m->m[4] * v->y + m->m[0] * v->x + m->m[8] * v->z;
    float y = m->m[1] * v->x + m->m[5] * v->y + m->m[9] * v->z;
    float z = m->m[2] * v->x + m->m[6] * v->y + m->m[10] * v->z;
    out[0] = x;
    out[1] = y;
    out[2] = z;
    return out;
}

// 0x414730: out = a x b
CVector* __cdecl Cross(CVector* out, const CVector* a, const CVector* b) {
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - b->z * a->x;
    out->z = b->y * a->x - a->y * b->x;
    return out;
}

// 0x414770: the rotation times the column vector (the inverse rotation)
CVector* __cdecl InverseTransformVector(CVector* out, const CVector* v, const CMatrix* m) {
    float x = m->m[2] * v->z + v->x * m->m[0] + m->m[1] * v->y;
    float y = m->m[6] * v->z + m->m[4] * v->x + m->m[5] * v->y;
    float z = m->m[10] * v->z + m->m[8] * v->x + m->m[9] * v->y;
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}

// 0x414B10: `count` points (16 bytes apart) through the matrix
void __cdecl TransformPoints(float* out, int count, const CMatrix* m, const uint8_t* in) {
    for (; count > 0; count--) {
        TransformVector(out, m, reinterpret_cast<const CVector*>(in));
        in += 0x10;
        out[0] += m->m[12];
        out[1] += m->m[13];
        out[2] = m->m[14] + out[2];
        out += 4;
    }
}

// 0x414B70: out = a - b
CVector* __cdecl Subtract(CVector* out, const CVector* a, const CVector* b) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
    return out;
}

// 0x414BA0: out = a - b (a read first: out may be a)
CVector* __cdecl Subtract2(CVector* out, const CVector* a, const CVector* b) {
    float z = a->z;
    float y = a->y;
    out->x = a->x - b->x;
    out->y = y - b->y;
    out->z = z - b->z;
    return out;
}

// 0x414C20: numerator / sqrt(|value|) (+-FLT_MAX by the numerator's sign for 0)
float __cdecl RecipSqrtImpl(float numerator, float value) {
    float root = static_cast<float>(std::sqrt(static_cast<double>(std::fabs(value))));
    if (root == 0.0f) {
        if (numerator < 0.0f) {
            return -3.4028234663852886e+38f;
        }
        return 3.4028234663852886e+38f;
    }
    return numerator / root;
}

// 0x414C80: as 0x4146B0 (the game's other copy; out[3] from uninitialised stack too)
float* __cdecl TransformVector2Impl(float* out, const CMatrix* m, const CVector* v) {
    float x = v->y * m->m[4] + v->x * m->m[0] + v->z * m->m[8];
    float y = m->m[5] * v->y + m->m[1] * v->x + m->m[9] * v->z;
    float z = m->m[6] * v->y + m->m[2] * v->x + v->z * m->m[10];
    out[0] = x;
    out[1] = y;
    out[2] = z;
    return out;
}

// 0x414D20: a point into the matrix's space (the inverse of a rigid transform)
CVector* __cdecl InverseTransformPoint(CVector* out, const CMatrix* m, const CVector* p) {
    float dx = p->x - m->m[12];
    float dy = p->y - m->m[13];
    float dz = p->z - m->m[14];
    out->x = dy * m->m[1] + dx * m->m[0] + dz * m->m[2];
    out->y = m->m[6] * dz + m->m[4] * dx + m->m[5] * dy;
    out->z = dz * m->m[10] + m->m[9] * dy + m->m[8] * dx;
    return out;
}

// 0x414DB0: `count` points (`stride` bytes apart) through the matrix
void __cdecl TransformPointsStrided(float* out, int count, const CMatrix* m, const uint8_t* in, int stride) {
    for (; count > 0; count--) {
        TransformVector2(out, m, reinterpret_cast<const CVector*>(in));
        in += stride;
        out[0] += m->m[12];
        out[1] = m->m[13] + out[1];
        out[2] = m->m[14] + out[2];
        out += 4;
    }
}

// 0x414E10 (thiscall): normalised; returns 1 / the length
float __fastcall Normalise(CVector* self, re::Edx) {
    float lengthSq = self->y * self->y + self->x * self->x + self->z * self->z;
    float scale = RecipSqrt(1.0f, lengthSq);
    self->x = scale * self->x;
    self->y = scale * self->y;
    self->z = self->z * scale;
    return scale;
}

} // namespace c01math

// ---- registration ----------------------------------------------------------------------

#define MATH_REVERSED_C01(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01math {
MATH_REVERSED_C01("CMatrix4::Multiply", 0x004120E0, &Matrix4Multiply);
MATH_REVERSED_C01("CVector::Scale (s, v)", 0x00412320, &ScaleVector);
MATH_REVERSED_C01("CVector::Scale (v, s)", 0x00412350, &ScaleVector2);
MATH_REVERSED_C01("CVector::FromSpherical", 0x00412380, &FromSpherical);
MATH_REVERSED_C01("Math::CatmullRom", 0x00412400, &CatmullRom);
MATH_REVERSED_C01("CVector::RotateHeadingPitch", 0x00412530, &RotateHeadingPitch);
MATH_REVERSED_C01("CMatrix::CopyOnlyMatrix", 0x00412720, &Matrix_CopyOnlyMatrix);
MATH_REVERSED_C01("CMatrix::SetUnity", 0x00412770, &Matrix_SetUnity);
MATH_REVERSED_C01("CMatrix::ResetOrientation", 0x004127A0, &Matrix_ResetOrientation);
MATH_REVERSED_C01("CMatrix::SetTranslate", 0x004127C0, &Matrix_SetTranslate);
MATH_REVERSED_C01("CMatrix::PreMultiply", 0x00412800, &Matrix_PreMultiply);
MATH_REVERSED_C01("CVector::Add", 0x004129A0, &Add);
MATH_REVERSED_C01("CVector::operator=", 0x004129D0, &Vector_Assign);
MATH_REVERSED_C01("CMatrix::CMatrix (copy)", 0x00412A10, &Matrix_CopyConstruct);
MATH_REVERSED_C01("CMatrix::FromNiTransform", 0x00412A70, &Matrix_FromNiTransform);
MATH_REVERSED_C01("CMatrix::ToNiTransform", 0x00412AC0, &Matrix_ToNiTransform);
MATH_REVERSED_C01("CMatrix::SetRotateXOnly", 0x00412B60, &SetRotateXOnly);
MATH_REVERSED_C01("CMatrix::SetRotateYOnly", 0x00412BC0, &SetRotateYOnly);
MATH_REVERSED_C01("CMatrix::SetRotateZOnly", 0x00412C20, &SetRotateZOnly);
MATH_REVERSED_C01("CMatrix::SetRotateX", 0x00412CA0, &SetRotateX);
MATH_REVERSED_C01("CMatrix::SetRotateY", 0x00412CC0, &SetRotateY);
MATH_REVERSED_C01("CMatrix::SetRotateZ", 0x00412CE0, &SetRotateZ);
MATH_REVERSED_C01("Math::Min", 0x00414470, &Min);
MATH_REVERSED_C01("Math::Max", 0x00414490, &Max);
MATH_REVERSED_C01("CVector::DotProduct", 0x00414640, &DotProduct);
MATH_REVERSED_C01("CVector::AddTo", 0x00414670, &Vector_AddTo);
MATH_REVERSED_C01("CMatrix::TransformVector", 0x004146B0, &TransformVectorImpl);
MATH_REVERSED_C01("CVector::CrossProduct", 0x00414730, &Cross);
MATH_REVERSED_C01("CMatrix::InverseTransformVector", 0x00414770, &InverseTransformVector);
MATH_REVERSED_C01("CMatrix::TransformPoints", 0x00414B10, &TransformPoints);
MATH_REVERSED_C01("CVector::Subtract", 0x00414B70, &Subtract);
MATH_REVERSED_C01("CVector::Subtract (2)", 0x00414BA0, &Subtract2);
MATH_REVERSED_C01("Math::RecipSqrt", 0x00414C20, &RecipSqrtImpl);
MATH_REVERSED_C01("CMatrix::TransformVector (2)", 0x00414C80, &TransformVector2Impl);
MATH_REVERSED_C01("CMatrix::InverseTransformPoint", 0x00414D20, &InverseTransformPoint);
MATH_REVERSED_C01("CMatrix::TransformPointsStrided", 0x00414DB0, &TransformPointsStrided);
MATH_REVERSED_C01("CVector::Normalise", 0x00414E10, &Normalise);
} // namespace c01math
