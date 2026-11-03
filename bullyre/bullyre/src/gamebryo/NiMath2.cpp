// The rest of the library's maths: NiACos, NiPlane's constructor, a scaled vector array added
// in place, the fast square root's mantissa table (NiMath's _SDMInit), NiQuaternion's algebra
// (sum, difference, product, unit inverse, Log, Exp, the squad Intermediate, scalar times
// quaternion), and NiRTTI's constructor (the class list and indices Bully adds).
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's
// order. The CRT's x87 acos / sqrt / sin / cos are the C runtime's here (crt::), rounded to float
// as the game stores them.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {
namespace {

struct NiQuaternion {
    float w, x, y, z;
};
static_assert(sizeof(NiQuaternion) == 0x10);

struct NiPlane {
    NiPoint3 normal;   // +0x0
    float constant;    // +0xC
};

struct NiRTTI {
    re::Ptr32<const char> name;      // +0x0
    re::Ptr32<const NiRTTI> base;    // +0x4
    uint32_t index;        // +0x8 (Bully's: the classes in construction order)
    uint32_t bits[9];      // +0xC (the indices of the classes this one is a kind of)
    re::Ptr32<NiRTTI> next;          // +0x30
};
static_assert(sizeof(NiRTTI) == 0x34);

GAME_VAR(NiRTTI*, NiRTTI_ms_pkHead, 0x020E2814);
GAME_VAR(uint32_t, NiRTTI_ms_uiNextIndex, 0x020E281C);
GAME_VAR(uint32_t*, NiMath_ms_puiSqrtTable, 0x020E2BFC);
GAME_VAR(double, kMinusOne, 0x00900550);   // -1.0
GAME_VAR(float, kPi, 0x009524C0);
GAME_VAR(float, kPiQ, 0x00953150);
GAME_VAR(float, kQuatEpsilon, 0x0095314C);  // 0.001
GAME_VAR(float, kMinusQuarter, 0x0091AF48);  // -0.25

GAME_FN(NiQuaternion*, __thiscall, Quaternion_Construct, 0x00775B90, NiQuaternion* self, float w, float x,
        float y, float z);
// (this file's, called where modes apply)
GAME_FN(NiQuaternion*, __thiscall, Quaternion_Multiply, 0x00775C40, const NiQuaternion* self,
        NiQuaternion* result, const NiQuaternion* other);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_Log, 0x00775D80, NiQuaternion* result, const NiQuaternion* q);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_ScalarTimes, 0x00776470, NiQuaternion* result, float scale,
        const NiQuaternion* q);
GAME_FN(NiQuaternion*, __cdecl, Quaternion_Exp, 0x00776610, NiQuaternion* result, const NiQuaternion* q);
GAME_FN(uint32_t*, __cdecl, Math_CreateSqrtTable, 0x00776810);

inline float FromBits(uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, 4);
    return value;
}
inline uint32_t ToBits(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

// acos clamped to [-1, 1] as the library inlines it (at -1 and below: pi, at 1 and above: 0)
inline float ClampedACos(float value, float pi) {
    if (!(double(value) > kMinusOne)) {
        return pi;
    }
    if (!(1.0f > value)) {
        return 0.0f;
    }
    return float(crt::Acos(value));
}

} // namespace

// 0x765410: NiRTTI::NiRTTI(name, base): indexed and linked at the head of the class list (the
// kind-of bits filled in later)
NiRTTI* __fastcall NiRTTI_Construct(NiRTTI* self, re::Edx, const char* name, const NiRTTI* base) {
    self->name = name;
    self->base = base;
    self->index = NiRTTI_ms_uiNextIndex;
    NiRTTI_ms_uiNextIndex++;
    for (uint32_t& word : self->bits) {
        word = 0;
    }
    self->next = NiRTTI_ms_pkHead;
    NiRTTI_ms_pkHead = self;
    return self;
}

// 0x768740: NiACos
float __cdecl NiACos(float value) {
    return ClampedACos(value, kPi);
}

// 0x770AE0: the fast square root's table made (NiMath's _SDMInit)
void __cdecl NiMath_SDMInit() {
    NiMath_ms_puiSqrtTable = Math_CreateSqrtTable();
}

// 0x770AF0: the table freed (NiMath's _SDMShutdown, from NiMainSDM::Shutdown)
void __cdecl NiMath_SDMShutdown() {
    NiFree(NiMath_ms_puiSqrtTable);
    NiMath_ms_puiSqrtTable = nullptr;
}

// 0x770E40: the vectors added to, each by its source scaled (four at a time, then the rest)
void __cdecl NiPoint3_AddScaledArray(NiPoint3* destination, float scale, const NiPoint3* source, int32_t count) {
    for (uint32_t i = 0; i < uint32_t(count); i++) {
        float x = source[i].x * scale;
        float y = source[i].y * scale;
        float z = source[i].z * scale;
        destination[i].x = destination[i].x + x;
        destination[i].y = destination[i].y + y;
        destination[i].z = destination[i].z + z;
    }
}

// 0x771010: NiPlane::NiPlane(normal, constant)
NiPlane* __fastcall NiPlane_Construct(NiPlane* self, re::Edx, const NiPoint3* normal, float constant) {
    self->normal.x = normal->x;
    self->normal.y = normal->y;
    self->constant = constant;
    self->normal.z = normal->z;
    return self;
}

// 0x775BB0: NiQuaternion::operator+
NiQuaternion* __fastcall NiQuaternion_Add(const NiQuaternion* self, re::Edx, NiQuaternion* result,
                                          const NiQuaternion* other) {
    result->w = other->w + self->w;
    result->x = other->x + self->x;
    result->y = other->y + self->y;
    result->z = other->z + self->z;
    return result;
}

// 0x775BE0: NiQuaternion::operator- (the difference)
NiQuaternion* __fastcall NiQuaternion_Subtract(const NiQuaternion* self, re::Edx, NiQuaternion* result,
                                               const NiQuaternion* other) {
    result->w = self->w - other->w;
    result->x = self->x - other->x;
    result->y = self->y - other->y;
    result->z = self->z - other->z;
    return result;
}

// 0x775C40: NiQuaternion::operator* (the product, this one first)
NiQuaternion* __fastcall NiQuaternion_Multiply(const NiQuaternion* self, re::Edx, NiQuaternion* result,
                                               const NiQuaternion* other) {
    const NiQuaternion& a = *self;
    const NiQuaternion& b = *other;
    float w = b.w * a.w - b.x * a.x;
    w = w - b.y * a.y;
    result->w = w - b.z * a.z;
    float x = a.x * b.w + b.x * a.w;
    x = x + b.z * a.y;
    result->x = x - a.z * b.y;
    float y = a.y * b.w + b.y * a.w;
    y = y + b.x * a.z;
    result->y = y - b.z * a.x;
    float z = a.z * b.w + b.z * a.w;
    z = z + a.x * b.y;
    result->z = z - b.x * a.y;
    return result;
}

// 0x775CD0: NiQuaternion::UnitInverse (the conjugate)
NiQuaternion* __cdecl NiQuaternion_UnitInverse(NiQuaternion* result, const NiQuaternion* q) {
    result->w = q->w;
    result->x = -q->x;
    result->y = -q->y;
    result->z = -q->z;
    return result;
}

// 0x775D80: NiQuaternion::Log: (0, axis * angle / sin(angle)), the axis as it is under 0.001
NiQuaternion* __cdecl NiQuaternion_Log(NiQuaternion* result, const NiQuaternion* q) {
    float angle = ClampedACos(q->w, kPiQ);
    float sine = float(crt::Sin(angle));
    float coefficient;
    if (!(std::fabs(sine) < kQuatEpsilon)) {
        coefficient = angle / sine;
    } else {
        coefficient = 1.0f;
    }
    result->w = 0.0f;
    result->x = coefficient * q->x;
    result->y = q->y * coefficient;
    result->z = coefficient * q->z;
    return result;
}

// 0x775E40: NiQuaternion::Intermediate(q0, q1, q2): q1 exp(-(log(q1* q2) + log(q1* q0)) / 4)
NiQuaternion* __cdecl NiQuaternion_Intermediate(NiQuaternion* result, const NiQuaternion* q0,
                                                const NiQuaternion* q1, const NiQuaternion* q2) {
    NiQuaternion inverse = {q1->w, -q1->x, -q1->y, -q1->z};
    NiQuaternion product2;
    NiQuaternion log2;
    const NiQuaternion* a = Quaternion_Log(&log2, Quaternion_Multiply(&inverse, &product2, q2));
    NiQuaternion product0;
    NiQuaternion log0;
    const NiQuaternion* b = Quaternion_Log(&log0, Quaternion_Multiply(&inverse, &product0, q0));
    NiQuaternion sum;
    sum.x = a->x + b->x;
    sum.y = a->y + b->y;
    sum.z = a->z + b->z;
    sum.w = a->w + b->w;
    NiQuaternion scaled;
    NiQuaternion exponential;
    Quaternion_Exp(&exponential, Quaternion_ScalarTimes(&scaled, kMinusQuarter, &sum));
    Quaternion_Multiply(q1, result, &exponential);
    return result;
}

// 0x776470: operator*(scalar, quaternion)
NiQuaternion* __cdecl NiQuaternion_ScalarTimes(NiQuaternion* result, float scale, const NiQuaternion* q) {
    float z = scale * q->z;
    float y = q->y * scale;
    float x = q->x * scale;
    float w = scale * q->w;
    Quaternion_Construct(result, w, x, y, z);
    return result;
}

// 0x776610: NiQuaternion::Exp: (cos(angle), axis * sin(angle) / angle) for the angle the
// vector's length, the vector as it is under 0.001
NiQuaternion* __cdecl NiQuaternion_Exp(NiQuaternion* result, const NiQuaternion* q) {
    float lengthSq = q->x * q->x + q->y * q->y;
    lengthSq = lengthSq + q->z * q->z;
    float angle = float(crt::Sqrt(lengthSq));
    float cosine = float(crt::Cos(angle));
    float sine = float(crt::Sin(angle));
    float coefficient;
    if (!(kQuatEpsilon > std::fabs(sine))) {
        coefficient = sine / angle;
    } else {
        coefficient = 1.0f;
    }
    float z = coefficient * q->z;
    float y = coefficient * q->y;
    float x = coefficient * q->x;
    Quaternion_Construct(result, cosine, x, y, z);
    return result;
}

// 0x776810: the fast square root's table: the mantissas of the square roots of [1, 2) and
// [2, 4), 128 steps each (NiMalloc'd)
uint32_t* __cdecl NiMath_CreateSqrtTable() {
    uint32_t* table = static_cast<uint32_t*>(NiMalloc(0x400));
    for (uint32_t i = 0; i < 0x80; i++) {
        float low = float(crt::Sqrt(FromBits((i | 0x3F80) << 16)));
        table[i] = ToBits(low) & 0x7FFFFF;
        float high = float(crt::Sqrt(FromBits((i | 0x4000) << 16)));
        table[i + 0x80] = ToBits(high) & 0x7FFFFF;
    }
    return table;
}

#define NIMATH2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIMATH2_REVERSED("NiRTTI::NiRTTI", 0x00765410, NiRTTI_Construct);
NIMATH2_REVERSED("NiACos", 0x00768740, NiACos);
NIMATH2_REVERSED("NiMath::_SDMInit", 0x00770AE0, NiMath_SDMInit);
// (never hooked in the 32-bit game: a body at its address for the 64-bit build)
RE_UNHOOKED_FN("NiMath::_SDMShutdown", 0x00770AF0, &NiMath_SDMShutdown);
NIMATH2_REVERSED("NiPoint3::AddScaledArray", 0x00770E40, NiPoint3_AddScaledArray);
NIMATH2_REVERSED("NiPlane::NiPlaneWith", 0x00771010, NiPlane_Construct);
NIMATH2_REVERSED("NiQuaternion::operator+", 0x00775BB0, NiQuaternion_Add);
NIMATH2_REVERSED("NiQuaternion::operator-(quat)", 0x00775BE0, NiQuaternion_Subtract);
NIMATH2_REVERSED("NiQuaternion::operator*(quat)", 0x00775C40, NiQuaternion_Multiply);
NIMATH2_REVERSED("NiQuaternion::UnitInverse", 0x00775CD0, NiQuaternion_UnitInverse);
NIMATH2_REVERSED("NiQuaternion::Log", 0x00775D80, NiQuaternion_Log);
NIMATH2_REVERSED("NiQuaternion::Intermediate", 0x00775E40, NiQuaternion_Intermediate);
NIMATH2_REVERSED("NiQuaternion::operator*(scalar,quat)", 0x00776470, NiQuaternion_ScalarTimes);
NIMATH2_REVERSED("NiQuaternion::Exp", 0x00776610, NiQuaternion_Exp);
NIMATH2_REVERSED("NiMath::CreateSqrtTable", 0x00776810, NiMath_CreateSqrtTable);

} // namespace ni
