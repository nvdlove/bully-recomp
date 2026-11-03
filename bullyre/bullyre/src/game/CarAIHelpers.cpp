// Small helpers next to CCarAI in the exe: vector ops, random numbers, an angle
// falloff, positions from entities and Gamebryo frames.
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace c01math2 {

struct CVector {
    float x, y, z;
};
struct CMatrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(CMatrix) == 0x44);
using re::At;

GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const CMatrix* in, CMatrix* out);
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, CMatrix* self, const CMatrix* other);
GAME_FN(void*, __cdecl, RwCameraGetFrame, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, RwFrameGetLTM, 0x006C9A90, void* frame);

// 0x41A440 (thiscall, CPhysical): the move speed set
void __fastcall SetMoveSpeed(uint8_t* self, re::Edx, const CVector* speed) {
    At<float>(self, 0x124) = speed->x;
    At<float>(self, 0x128) = speed->y;
    At<float>(self, 0x12C) = speed->z;
}

// 0x41A470: x clamped to [low, high] (low checked first)
float __cdecl Clamp(float x, float low, float high) {
    if (low > x) {
        return low;
    }
    if (high < x) {
        return high;
    }
    return x;
}

// 0x41A4A0 (thiscall): /= (by the inverse)
CVector* __fastcall Vector_Divide(CVector* self, re::Edx, float divisor) {
    float inverse = 1.0f / divisor;
    self->x = inverse * self->x;
    self->y = self->y * inverse;
    self->z = inverse * self->z;
    return self;
}

// 0x41A4D0: the flat distance squared from b to a, less radius squared
float __cdecl FlatDistanceSqrMinusRadiusSqr(const CVector* a, const CVector* b, float radius) {
    CVector d;
    VectorSub(&d, a, b);
    float sum = static_cast<float>(static_cast<double>(d.x) * d.x + static_cast<double>(d.y) * d.y + 0.0 * 0.0);
    return static_cast<float>(sum - static_cast<double>(radius) * radius);
}

// 0x41A520: out = v flattened, normalised, scaled to `length`
CVector* __cdecl FlatDirection(CVector* out, const CVector* v, float length) {
    out->x = v->x;
    out->y = v->y;
    out->z = 0.0f;
    Vector_Normalise(out);
    out->x = length * out->x;
    out->y = out->y * length;
    out->z = length * out->z;
    return out;
}

// 0x41A570: out = a x b
CVector* __cdecl CrossProduct2(CVector* out, const CVector* a, const CVector* b) {
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = b->y * a->x - b->x * a->y;
    return out;
}

// 0x41A5B0: a random float in [min, max]
float __cdecl RandomFloat(float min, float max) {
    int r = Rand();
    return static_cast<float>(r / 32767.0 * (static_cast<double>(max) - min) + min);
}

// 0x41A5E0: a coin toss
uint8_t __cdecl RandomBool() {
    return Rand() < 0x3FFF;
}

// 0x41A600: 1 within `inner` of angle 0, easing to `minimum` at `outer` (the angle
// wrapped into -pi..pi first)
float __cdecl AngleFalloff(float angle, float inner, float outer, float minimum) {
    const float twoPi = 6.2831854820251465f;
    while (angle < -3.1415927410125732) {
        angle = angle + twoPi;
    }
    while (angle > 3.1415927410125732) {
        angle = angle - twoPi;
    }
    angle = std::fabs(angle);
    float f = static_cast<float>(angle - inner);
    if (f < 0.0) {
        f = 0.0f;
    }
    double range = static_cast<double>(outer) - inner;
    float result = static_cast<float>(1.0 - (f / range) * (1.0 - minimum));
    if (!(f > range)) {
        return result;
    }
    return minimum;
}

// 0x41A890: out = the inverse of a rigid matrix (no attached matrix)
CMatrix* __cdecl Invert(CMatrix* out, const CMatrix* in) {
    CMatrix inverse;
    inverse.attached = nullptr;
    Matrix_Invert(in, &inverse);
    Matrix_CopyConstruct(out, &inverse);
    return out;
}

// 0x41A8D0 (thiscall): a byte of a counted array (+8 bytes, +0x14 count; 0 past it)
uint32_t __fastcall ByteArray_Get(uint8_t* self, re::Edx, uint32_t index) {
    if (At<uint32_t>(self, 0x14) <= index) {
        return 0;
    }
    return self[8 + index];
}

// 0x41A950: the distance from b to a
float __cdecl Distance(const CVector* a, const CVector* b) {
    CVector d;
    const CVector* p = VectorSub(&d, a, b);
    float sum = static_cast<float>(static_cast<double>(p->y) * p->y + static_cast<double>(p->x) * p->x + static_cast<double>(p->z) * p->z);
    return static_cast<float>(std::sqrt(static_cast<double>(std::fabs(sum))));
}

// 0x41A9A0: a point through a matrix (out[3], the game's uninitialised stack, left)
float* __cdecl TransformPoint(float* out, const CMatrix* m, const CVector* point) {
    CVector v = *point;
    out[0] = static_cast<float>(static_cast<double>(m->m[4]) * v.y + static_cast<double>(m->m[0]) * v.x +
                                static_cast<double>(m->m[8]) * v.z + m->m[12]);
    out[1] = static_cast<float>(static_cast<double>(m->m[5]) * v.y + static_cast<double>(m->m[1]) * v.x +
                                static_cast<double>(m->m[9]) * v.z + m->m[13]);
    out[2] = static_cast<float>(static_cast<double>(m->m[6]) * v.y + static_cast<double>(m->m[2]) * v.x +
                                static_cast<double>(m->m[10]) * v.z + m->m[14]);
    return out;
}

// 0x41AA50 (thiscall, CPed): its state (+0x1310) is 0, 7 or 8
uint32_t __fastcall Ped_IsStateIdle(uint8_t* self, re::Edx) {
    int32_t state = At<int32_t>(self, 0x1310);
    if (state != 0 && state != 8 && state != 7) {
        return 0;
    }
    return 1;
}

// 0x41AA70 (thiscall): the camera's frame's z column (its LTM's rotation column 2)
CVector* __fastcall Camera_GetUp(uint8_t* self, re::Edx, CVector* out) {
    const float* ltm = RwFrameGetLTM(RwCameraGetFrame(At<void*>(self, 0x8C)));
    CVector up = {ltm[2], ltm[5], ltm[8]};
    *out = up;
    return out;
}

// 0x41AAC0 (thiscall): the camera's frame's position
float* __fastcall Camera_GetPosition(uint8_t* self, re::Edx) {
    return RwFrameGetLTM(RwCameraGetFrame(At<void*>(self, 0x8C))) + 9;
}

// 0x41AAE0: an entity's position copied
CVector* __cdecl GetEntityPosition(CVector* out, uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    if (matrix) {
        *out = At<CVector>(matrix, 0x30);
        return out;
    }
    *out = At<CVector>(entity, 4);
    return out;
}

} // namespace c01math2

// ---- registration ----------------------------------------------------------------------

#define MATH2_REVERSED_C01(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01math2 {
MATH2_REVERSED_C01("CPhysical::SetMoveSpeed", 0x0041A440, &SetMoveSpeed);
MATH2_REVERSED_C01("Math::Clamp", 0x0041A470, &Clamp);
MATH2_REVERSED_C01("CVector::operator/=", 0x0041A4A0, &Vector_Divide);
MATH2_REVERSED_C01("Math::FlatDistanceSqrMinusRadiusSqr", 0x0041A4D0, &FlatDistanceSqrMinusRadiusSqr);
MATH2_REVERSED_C01("CVector::FlatDirection", 0x0041A520, &FlatDirection);
MATH2_REVERSED_C01("CVector::CrossProduct (2)", 0x0041A570, &CrossProduct2);
MATH2_REVERSED_C01("CGeneral::GetRandomNumberInRange", 0x0041A5B0, &RandomFloat);
MATH2_REVERSED_C01("CGeneral::GetRandomTrueFalse", 0x0041A5E0, &RandomBool);
MATH2_REVERSED_C01("Math::AngleFalloff", 0x0041A600, &AngleFalloff);
MATH2_REVERSED_C01("CMatrix::Invert (copy)", 0x0041A890, &Invert);
MATH2_REVERSED_C01("ByteArray::Get", 0x0041A8D0, &ByteArray_Get);
MATH2_REVERSED_C01("CVector::Distance", 0x0041A950, &Distance);
MATH2_REVERSED_C01("CMatrix::TransformPoint", 0x0041A9A0, &TransformPoint);
MATH2_REVERSED_C01("CPed::IsStateIdle", 0x0041AA50, &Ped_IsStateIdle);
MATH2_REVERSED_C01("CCamera::GetUp", 0x0041AA70, &Camera_GetUp);
MATH2_REVERSED_C01("CCamera::GetPosition", 0x0041AAC0, &Camera_GetPosition);
MATH2_REVERSED_C01("CEntity::GetPositionCopy", 0x0041AAE0, &GetEntityPosition);
} // namespace c01math2
