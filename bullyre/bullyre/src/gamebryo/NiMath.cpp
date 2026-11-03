// Gamebryo's maths types as the library has them: NiMatrix3 (row-major m[row][col], its
// columns the basis vectors), NiTransform (src/gamebryo/NiCore.h), NiPlane (a normal and a
// constant), NiFrustum (src/gamebryo/NiCamera.h) and NiFrustumPlanes (the six culling planes
// in world space), the comparisons of NiPoint3 / NiColorA, NiPoint3::UnitizeVectors.
//
// Main-thread float maths at the game's 24-bit x87 precision: each product and sum below is a
// float operation in the game's order (its grouping kept; the operands of a single add or
// multiply commute exactly). Results are written in the game's order, reading the operands
// from memory each time, so a result that aliases an operand comes out as the game's does.

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCamera.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

// NiPlane: the normal and the constant (normal . point)
struct NiPlane {
    NiPoint3 normal;   // +0x0
    float constant;    // +0xC
};
static_assert(sizeof(NiPlane) == 0x10);

// NiFrustumPlanes: near, far, left, right, top, bottom; then the active planes' bits
struct NiFrustumPlanes {
    NiPlane planes[6];   // +0x0
    uint32_t activePlanes;   // +0x60 (not touched by Set)
};
static_assert(sizeof(NiFrustumPlanes) == 0x64);

struct NiColorA {
    float r, g, b, a;
};

GAME_VAR(NiPoint3, Point3_ZERO, 0x020E2D44);
// NiFastSqrt's table: the mantissa's top bits (and the exponent's parity) to the root's mantissa
GAME_VAR(uint32_t*, FastSqrtTable, 0x020E2BFC);

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, NiPoint3* self, void* stream);
// (this chunk's, called where modes apply)
GAME_FN(void, __thiscall, Matrix3_MakeIdentity, 0x00768680, NiMatrix3* self);
GAME_FN(uint8_t, __thiscall, Matrix3_IsEqual, 0x007686A0, const NiMatrix3* self, const NiMatrix3* other);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Transpose, 0x00768C40, const NiMatrix3* self, NiMatrix3* result);
GAME_FN(NiPoint3*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(void, __thiscall, Matrix3_Snap, 0x00769050, NiMatrix3* self);
GAME_FN(void, __thiscall, Matrix3_LoadBinary, 0x007690A0, NiMatrix3* self, void* stream);
GAME_FN(void, __thiscall, Matrix3_SaveBinary, 0x00769100, NiMatrix3* self, void* stream);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
GAME_FN(NiPlane*, __thiscall, Plane_Construct, 0x00771030, NiPlane* self, const NiPoint3* normal,
        const NiPoint3* point);

inline float* E(NiMatrix3* m) {
    return &m->m[0][0];
}
inline const float* E(const NiMatrix3* m) {
    return &m->m[0][0];
}

// ---- NiMatrix3

void __fastcall Matrix3MakeIdentity(NiMatrix3* self, re::Edx) {
    float* m = E(self);
    m[0] = 1.0f;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[5] = 0.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[4] = 1.0f;
    m[8] = 1.0f;
}

// operator== (1 or 0 in the whole register)
uint32_t __fastcall Matrix3IsEqual(const NiMatrix3* self, re::Edx, const NiMatrix3* other) {
    const float* a = E(self);
    const float* b = E(other);
    for (int i = 0; i < 9; i++) {
        if (!(b[i] == a[i])) {
            return 0;
        }
    }
    return 1;
}

// SetCol(0..2): the three columns at once
NiMatrix3* __fastcall Matrix3SetCols(NiMatrix3* self, re::Edx, const NiPoint3* col0, const NiPoint3* col1,
                                     const NiPoint3* col2) {
    float* m = E(self);
    m[0] = col0->x;
    m[3] = col0->y;
    m[6] = col0->z;
    m[1] = col1->x;
    m[4] = col1->y;
    m[7] = col1->z;
    m[2] = col2->x;
    m[5] = col2->y;
    m[8] = col2->z;
    return self;
}

// MakeZRotation(angle) (the x87's fsincos: the float angle's sine and cosine, rounded to float)
void __fastcall Matrix3MakeZRotation(NiMatrix3* self, re::Edx, float angle) {
    float c = crt::Cosf(angle);
    float s = crt::Sinf(angle);
    float* m = E(self);
    m[0] = c;
    m[1] = s;
    m[2] = 0.0f;
    m[3] = -s;
    m[4] = c;
    m[5] = 0.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 1.0f;
}

// MakeRotation(angle, x, y, z): about the unit axis
void __fastcall Matrix3MakeRotation(NiMatrix3* self, re::Edx, float angle, float x, float y, float z) {
    float c = crt::Cosf(angle);
    float s = crt::Sinf(angle);
    float omc = 1.0f - c;
    float xyOmc = x * y * omc;
    float xzOmc = x * z * omc;
    float yzOmc = y * z * omc;
    float sx = s * x;
    float sy = s * y;
    float sz = s * z;
    float* m = E(self);
    m[0] = x * x * omc + c;
    m[1] = xyOmc + sz;
    m[2] = xzOmc - sy;
    m[3] = xyOmc - sz;
    m[4] = y * y * omc + c;
    m[5] = yzOmc + sx;
    m[6] = sy + xzOmc;
    m[7] = yzOmc - sx;
    m[8] = omc * (z * z) + c;
}

// operator*(matrix): written column by column, as the game does
NiMatrix3* __fastcall Matrix3Multiply(const NiMatrix3* self, re::Edx, NiMatrix3* result, const NiMatrix3* other) {
    const float* a = E(self);
    const float* b = E(other);
    float* r = E(result);
    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            r[row * 3 + col] = a[row * 3] * b[col] + a[row * 3 + 1] * b[3 + col] + a[row * 3 + 2] * b[6 + col];
        }
    }
    return result;
}

// operator*(scalar)
NiMatrix3* __fastcall Matrix3Scale(const NiMatrix3* self, re::Edx, NiMatrix3* result, float scale) {
    const float* a = E(self);
    float* r = E(result);
    for (int i = 0; i < 9; i++) {
        r[i] = a[i] * scale;
    }
    return result;
}

// operator*(point): the matrix times the column vector
NiPoint3* __fastcall Matrix3TimesPoint(const NiMatrix3* self, re::Edx, NiPoint3* result, const NiPoint3* point) {
    const float* m = E(self);
    result->x = m[0] * point->x + m[1] * point->y + m[2] * point->z;
    result->y = m[3] * point->x + m[4] * point->y + m[5] * point->z;
    result->z = m[6] * point->x + m[7] * point->y + m[8] * point->z;
    return result;
}

// operator*(point, matrix): the row vector times the matrix
NiPoint3* __cdecl PointTimesMatrix3(NiPoint3* result, const NiPoint3* point, const NiMatrix3* matrix) {
    const float* m = E(matrix);
    result->x = point->x * m[0] + point->y * m[3] + point->z * m[6];
    result->y = point->x * m[1] + point->y * m[4] + point->z * m[7];
    result->z = point->x * m[2] + point->y * m[5] + point->z * m[8];
    return result;
}

// Inverse(result): the adjoint, then divided by the determinant unless that's within 1e-6 of
// zero (then false, the adjoint left in the result)
uint8_t __fastcall Matrix3Inverse(const NiMatrix3* self, re::Edx, NiMatrix3* result) {
    const float* m = E(self);
    float* r = E(result);
    r[0] = m[8] * m[4] - m[7] * m[5];
    r[1] = m[7] * m[2] - m[1] * m[8];
    r[2] = m[1] * m[5] - m[2] * m[4];
    r[3] = m[6] * m[5] - m[3] * m[8];
    r[4] = m[0] * m[8] - m[6] * m[2];
    r[5] = m[3] * m[2] - m[5] * m[0];
    r[6] = m[3] * m[7] - m[6] * m[4];
    r[7] = m[1] * m[6] - m[7] * m[0];
    r[8] = m[0] * m[4] - m[3] * m[1];
    float det = r[0] * m[0] + m[1] * r[3] + r[6] * m[2];
    float absDet = det < 0.0f ? -det : det;
    if (absDet <= 1e-06f) {
        return 0;
    }
    float inv = 1.0f / det;
    for (int row = 0; row < 3; row++) {
        r[row * 3] = r[row * 3] * inv;
        r[row * 3 + 1] = inv * r[row * 3 + 1];
        r[row * 3 + 2] = inv * r[row * 3 + 2];
    }
    return 1;
}

// Transpose(): through a copy
NiMatrix3* __fastcall Matrix3Transpose(const NiMatrix3* self, re::Edx, NiMatrix3* result) {
    float t[9];
    std::memcpy(t, E(self), sizeof(t));
    float* r = E(result);
    r[0] = t[0];
    r[3] = t[1];
    r[6] = t[2];
    r[1] = t[3];
    r[4] = t[4];
    r[7] = t[5];
    r[2] = t[6];
    r[5] = t[7];
    r[8] = t[8];
    return result;
}

// Reorthogonalize(): Gram-Schmidt on the columns; false (the matrix part done) when a column's
// length falls under 1e-5
uint8_t __fastcall Matrix3Reorthogonalize(NiMatrix3* self, re::Edx) {
    float* m = E(self);
    float length = crt::Sqrtf(m[0] * m[0] + m[3] * m[3] + m[6] * m[6]);
    if (length < 1e-05f) {
        return 0;
    }
    float inv = 1.0f / length;
    m[0] = m[0] * inv;
    m[3] = inv * m[3];
    m[6] = m[6] * inv;

    float dot = m[0] * m[1] + m[3] * m[4] + m[6] * m[7];
    m[1] = m[1] - dot * m[0];
    m[4] = m[4] - dot * m[3];
    m[7] = m[7] - dot * m[6];
    length = crt::Sqrtf(m[1] * m[1] + m[4] * m[4] + m[7] * m[7]);
    if (length < 1e-05f) {
        return 0;
    }
    inv = 1.0f / length;
    m[1] = inv * m[1];
    m[4] = m[4] * inv;
    m[7] = inv * m[7];

    dot = m[2] * m[0] + m[5] * m[3] + m[6] * m[8];
    m[2] = m[2] - dot * m[0];
    m[5] = m[5] - dot * m[3];
    m[8] = m[8] - dot * m[6];
    dot = m[1] * m[2] + m[4] * m[5] + m[7] * m[8];
    m[2] = m[2] - dot * m[1];
    m[5] = m[5] - m[4] * dot;
    m[8] = m[8] - dot * m[7];
    length = crt::Sqrtf(m[2] * m[2] + m[5] * m[5] + m[8] * m[8]);
    if (length < 1e-05f) {
        return 0;
    }
    inv = 1.0f / length;
    m[2] = m[2] * inv;
    m[5] = m[5] * inv;
    m[8] = inv * m[8];
    return 1;
}

// TransposeTimes(result, other): this matrix's transpose times the other
NiMatrix3* __fastcall Matrix3TransposeTimes(const NiMatrix3* self, re::Edx, NiMatrix3* result,
                                            const NiMatrix3* other) {
    const float* a = E(self);
    const float* b = E(other);
    float* r = E(result);
    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            r[row * 3 + col] = a[row] * b[col] + a[3 + row] * b[3 + col] + a[6 + row] * b[6 + col];
        }
    }
    return result;
}

// Snap(): entries within 1e-8 of zero made zero
void __fastcall Matrix3Snap(NiMatrix3* self, re::Edx) {
    float* m = E(self);
    for (int i = 0; i < 9; i++) {
        float a = m[i] < 0.0f ? -m[i] : m[i];
        if (a <= 1e-08f && !(0.0f == m[i])) {
            m[i] = 0.0f;
        }
    }
}

void __fastcall Matrix3LoadBinary(NiMatrix3* self, re::Edx, void* stream) {
    float* m = E(self);
    for (int i = 0; i < 9; i++) {
        LoadBytes(stream, &m[i], 4);
    }
}

// SaveBinary: snapped first (the matrix itself changed)
void __fastcall Matrix3SaveBinary(NiMatrix3* self, re::Edx, void* stream) {
    Matrix3_Snap(self);
    float* m = E(self);
    for (int i = 0; i < 9; i++) {
        SaveBytes(stream, &m[i], 4);
    }
}

char* __fastcall Matrix3GetViewerString(const NiMatrix3* self, re::Edx, const char* prefix) {
    size_t size = std::strlen(prefix) + 0x80;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    const float* m = E(self);
    NiSprintf(text, size, "%s = ((%g,%g,%g),(%g,%g,%g),(%g,%g,%g))", prefix, double(m[0]), double(m[1]),
              double(m[2]), double(m[3]), double(m[4]), double(m[5]), double(m[6]), double(m[7]), double(m[8]));
    return text;
}

// ---- NiPoint3, NiColorA

// operator!=
uint8_t __fastcall Point3NotEqual(const NiPoint3* self, re::Edx, const NiPoint3* other) {
    bool equal = other->x == self->x && other->y == self->y && other->z == self->z;
    return equal ? 0 : 1;
}

uint8_t __fastcall ColorANotEqual(const NiColorA* self, re::Edx, const NiColorA* other) {
    bool equal = other->r == self->r && other->g == self->g && other->b == self->b && other->a == self->a;
    return equal ? 0 : 1;
}

// UnitizeVectors(points, count, stride): each scaled by 1 / NiFastSqrt(its squared length)
// (zero for a zero vector)
void __cdecl Point3UnitizeVectors(NiPoint3* points, uint32_t count, uint32_t stride) {
    uint8_t* p = reinterpret_cast<uint8_t*>(points);
    for (uint32_t i = 0; i < count; i++) {
        NiPoint3* v = reinterpret_cast<NiPoint3*>(p);
        float squared = v->x * v->x + v->y * v->y + v->z * v->z;
        uint32_t bits;
        std::memcpy(&bits, &squared, 4);
        float inv;
        if (bits == 0) {
            inv = 0.0f;
        } else {
            uint16_t exponent = uint16_t((bits >> 23) - 0x7F);
            uint32_t mantissa = bits & 0x7FFFFF;
            if (exponent & 1) {
                mantissa |= 0x800000;
            }
            int32_t rootExponent = int32_t(int16_t(exponent) >> 1) + 0x7F;
            uint32_t rootBits = (uint32_t(rootExponent) << 23) | FastSqrtTable[mantissa >> 16];
            float root;
            std::memcpy(&root, &rootBits, 4);
            inv = 1.0f / root;
        }
        v->x = v->x * inv;
        v->y = v->y * inv;
        v->z = inv * v->z;
        p += stride;
    }
}

// ---- NiTransform

void __fastcall TransformMakeIdentity(NiTransform* self, re::Edx) {
    Matrix3_MakeIdentity(&self->rotate);
    self->translate.x = Point3_ZERO.x;
    self->translate.y = Point3_ZERO.y;
    self->scale = 1.0f;
    self->translate.z = Point3_ZERO.z;
}

// Invert(result): the rotation transposed, 1/scale, the translation -R^T t / s
void __fastcall TransformInvert(const NiTransform* self, re::Edx, NiTransform* result) {
    NiMatrix3 transposed;
    std::memcpy(&result->rotate, Matrix3_Transpose(&self->rotate, &transposed), sizeof(NiMatrix3));
    result->scale = 1.0f / self->scale;
    NiPoint3 negated = {-self->translate.x, -self->translate.y, -self->translate.z};
    NiPoint3 rotated;
    NiPoint3* p = Matrix3_TimesPoint(&result->rotate, &rotated, &negated);
    float scale = result->scale;
    float x = scale * p->x;
    float y = p->y * scale;
    float z = scale * p->z;
    result->translate.x = x;
    result->translate.y = y;
    result->translate.z = z;
}

// operator!= (1 or 0 in the whole register)
uint32_t __fastcall TransformNotEqual(const NiTransform* self, re::Edx, const NiTransform* other) {
    if (Matrix3_IsEqual(&self->rotate, &other->rotate) && !Point3_NotEqual(&self->translate, &other->translate) &&
        other->scale == self->scale) {
        return 0;
    }
    return 1;
}

void __fastcall TransformLoadBinary(NiTransform* self, re::Edx, void* stream) {
    Matrix3_LoadBinary(&self->rotate, stream);
    Point3_LoadBinary(&self->translate, stream);
    LoadBytes(stream, &self->scale, 4);
}

void __fastcall TransformSaveBinary(NiTransform* self, re::Edx, void* stream) {
    Matrix3_SaveBinary(&self->rotate, stream);
    Point3_SaveBinary(&self->translate, stream);
    SaveBytes(stream, &self->scale, 4);
}

// ---- NiPlane

NiPlane* __fastcall PlaneConstruct(NiPlane* self, re::Edx) {
    self->normal.x = Point3_ZERO.x;
    self->normal.y = Point3_ZERO.y;
    self->constant = 0.0f;
    self->normal.z = Point3_ZERO.z;
    return self;
}

// NiPlane(normal, point)
NiPlane* __fastcall PlaneConstructAt(NiPlane* self, re::Edx, const NiPoint3* normal, const NiPoint3* point) {
    self->normal.x = normal->x;
    self->normal.y = normal->y;
    self->normal.z = normal->z;
    self->constant = normal->x * point->x + normal->y * point->y + normal->z * point->z;
    return self;
}

char* __fastcall PlaneGetViewerString(const NiPlane* self, re::Edx, const char* prefix) {
    size_t size = std::strlen(prefix) + 0x38;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, "(%g,%g,%g), %g", double(self->normal.x), double(self->normal.y), double(self->normal.z),
              double(self->constant));
    return text;
}

// ---- NiFrustum

NiFrustum* __fastcall FrustumConstruct(NiFrustum* self, re::Edx, int ortho) {
    self->ortho = uint8_t(ortho);
    return self;
}

NiFrustum* __fastcall FrustumConstructWith(NiFrustum* self, re::Edx, float left, float right, float top,
                                           float bottom, float nearDist, float farDist, int ortho) {
    self->left = left;
    self->ortho = uint8_t(ortho);
    self->right = right;
    self->top = top;
    self->bottom = bottom;
    self->nearDist = nearDist;
    self->farDist = farDist;
    return self;
}

char* __fastcall FrustumGetViewerString(const NiFrustum* self, re::Edx, const char* prefix) {
    size_t size = std::strlen(prefix) + 0x50;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, "%s = (N=%g,F=%g,L=%g,R=%g,T=%g,B=%g,O=%u)", prefix, double(self->nearDist),
              double(self->farDist), double(self->left), double(self->right), double(self->top),
              double(self->bottom), uint32_t(self->ortho));
    return text;
}

// ---- NiFrustumPlanes

// one plane built (NiPlane's constructor at its address, into a temporary) and copied in
inline void SetPlane(NiFrustumPlanes* self, int index, const NiPoint3& normal, const NiPoint3& point) {
    NiPlane plane;
    NiPlane* built = Plane_Construct(&plane, &normal, &point);
    self->planes[index] = *built;
}

inline NiPoint3 Scaled(float scale, const NiPoint3& v) {
    return {scale * v.x, v.y * scale, scale * v.z};
}
inline NiPoint3 Sum(const NiPoint3& a, const NiPoint3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
inline NiPoint3 Negated(const NiPoint3& v) {
    return {-v.x, -v.y, -v.z};
}

// Set(frustum, transform): the world-space planes of a camera's frustum, the camera looking
// down the rotation's first column (up the second, right the third), its normals inwards
void __fastcall FrustumPlanesSet(NiFrustumPlanes* self, re::Edx, const NiFrustum* frustum, const NiTransform* xform) {
    const float* r = E(&xform->rotate);
    NiPoint3 dir = {r[0], r[3], r[6]};
    NiPoint3 up = {r[1], r[4], r[7]};
    NiPoint3 right = {r[2], r[5], r[8]};
    NiPoint3 loc = xform->translate;

    float nearDist = frustum->nearDist;
    SetPlane(self, 0, dir, Sum(loc, Scaled(nearDist, dir)));
    float farDist = frustum->farDist;
    SetPlane(self, 1, Negated(dir), Sum(Scaled(farDist, dir), loc));

    if (frustum->ortho) {
        float left = frustum->left;
        SetPlane(self, 2, right, Sum(Scaled(left, right), loc));
        float rightDist = frustum->right;
        SetPlane(self, 3, Negated(right), Sum(Scaled(rightDist, right), loc));
        float top = frustum->top;
        SetPlane(self, 4, Negated(up), Sum(Scaled(top, up), loc));
        float bottom = frustum->bottom;
        SetPlane(self, 5, up, Sum(Scaled(bottom, up), loc));
        return;
    }

    // each side's normal: the direction and the side's axis weighted by 1/sqrt(1 + extent^2)
    float squared = frustum->left * frustum->left;
    float inv = 1.0f / crt::Sqrtf(squared + 1.0f);
    float dirScale = -frustum->left * inv;
    SetPlane(self, 2, Sum(Scaled(dirScale, dir), Scaled(inv, right)), loc);

    squared = frustum->right * frustum->right;
    inv = 1.0f / crt::Sqrtf(squared + 1.0f);
    dirScale = frustum->right * inv;
    float sideScale = -inv;
    SetPlane(self, 3, Sum(Scaled(dirScale, dir), Scaled(sideScale, right)), loc);

    squared = frustum->top * frustum->top;
    inv = 1.0f / crt::Sqrtf(squared + 1.0f);
    dirScale = inv * frustum->top;
    sideScale = -inv;
    SetPlane(self, 4, Sum(Scaled(dirScale, dir), Scaled(sideScale, up)), loc);

    squared = frustum->bottom * frustum->bottom;
    inv = 1.0f / crt::Sqrtf(squared + 1.0f);
    dirScale = -frustum->bottom * inv;
    SetPlane(self, 5, Sum(Scaled(dirScale, dir), Scaled(inv, up)), loc);
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIMATH_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIMATH_REVERSED("NiMatrix3::MakeIdentity", 0x00768680, &ni::Matrix3MakeIdentity);
NIMATH_REVERSED("NiMatrix3::operator==", 0x007686A0, &ni::Matrix3IsEqual);
NIMATH_REVERSED("NiMatrix3::SetCols", 0x00768780, &ni::Matrix3SetCols);
NIMATH_REVERSED("NiMatrix3::MakeZRotation", 0x007687D0, &ni::Matrix3MakeZRotation);
NIMATH_REVERSED("NiMatrix3::MakeRotation", 0x00768820, &ni::Matrix3MakeRotation);
NIMATH_REVERSED("NiMatrix3::operator*", 0x00768920, &ni::Matrix3Multiply);
NIMATH_REVERSED("NiMatrix3::operator*Scalar", 0x00768A10, &ni::Matrix3Scale);
NIMATH_REVERSED("NiMatrix3::operator*Point", 0x00768A70, &ni::Matrix3TimesPoint);
NIMATH_REVERSED("NiMatrix3::PointTimesMatrix", 0x00768AD0, &ni::PointTimesMatrix3);
NIMATH_REVERSED("NiMatrix3::Inverse", 0x00768B30, &ni::Matrix3Inverse);
NIMATH_REVERSED("NiMatrix3::Transpose", 0x00768C40, &ni::Matrix3Transpose);
NIMATH_REVERSED("NiMatrix3::Reorthogonalize", 0x00768CD0, &ni::Matrix3Reorthogonalize);
NIMATH_REVERSED("NiMatrix3::TransposeTimes", 0x00768F60, &ni::Matrix3TransposeTimes);
NIMATH_REVERSED("NiMatrix3::Snap", 0x00769050, &ni::Matrix3Snap);
NIMATH_REVERSED("NiMatrix3::LoadBinary", 0x007690A0, &ni::Matrix3LoadBinary);
NIMATH_REVERSED("NiMatrix3::SaveBinary", 0x00769100, &ni::Matrix3SaveBinary);
NIMATH_REVERSED("NiMatrix3::GetViewerString", 0x00769160, &ni::Matrix3GetViewerString);
NIMATH_REVERSED("NiColorA::operator!=", 0x00769390, &ni::ColorANotEqual);
NIMATH_REVERSED("NiPoint3::operator!=", 0x0076BEB0, &ni::Point3NotEqual);
NIMATH_REVERSED("NiTransform::MakeIdentity", 0x00770920, &ni::TransformMakeIdentity);
NIMATH_REVERSED("NiTransform::Invert", 0x00770950, &ni::TransformInvert);
NIMATH_REVERSED("NiTransform::operator!=", 0x007709F0, &ni::TransformNotEqual);
NIMATH_REVERSED("NiTransform::LoadBinary", 0x00770A40, &ni::TransformLoadBinary);
NIMATH_REVERSED("NiTransform::SaveBinary", 0x00770A90, &ni::TransformSaveBinary);
NIMATH_REVERSED("NiPoint3::UnitizeVectors", 0x00770B10, &ni::Point3UnitizeVectors);
NIMATH_REVERSED("NiPlane::NiPlane", 0x00770FE0, &ni::PlaneConstruct);
NIMATH_REVERSED("NiPlane::NiPlaneAt", 0x00771030, &ni::PlaneConstructAt);
NIMATH_REVERSED("NiPlane::GetViewerString", 0x00771070, &ni::PlaneGetViewerString);
NIMATH_REVERSED("NiFrustumPlanes::Set", 0x007710D0, &ni::FrustumPlanesSet);
NIMATH_REVERSED("NiFrustum::NiFrustum", 0x00771890, &ni::FrustumConstruct);
NIMATH_REVERSED("NiFrustum::NiFrustumWith", 0x007718A0, &ni::FrustumConstructWith);
NIMATH_REVERSED("NiFrustum::GetViewerString", 0x007718E0, &ni::FrustumGetViewerString);
