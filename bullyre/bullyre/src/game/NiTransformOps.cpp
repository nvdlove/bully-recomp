// Gamebryo NiTransform (rotation, translation, scale: 0x34 bytes) combined by mode:
// 0 replace, 1 this * other, 2 other * this; rotations about an axis in degrees and
// translations built into one and combined. And CColPoint's constructor, the trigger
// pool's GetAt, next to them in the exe.

#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace c01nitrans {

struct CVector {
    float x, y, z;
};
struct NiTransform {
    float rotate[9];      // +0 (column-major)
    CVector translate;    // +0x24
    float scale;          // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

// NiTransform::operator* (this * other into `out`)
GAME_FN(NiTransform*, __thiscall, NiTransform_Multiply, 0x00406830, const NiTransform* self, NiTransform* out, const NiTransform* other);
GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, NiTransform* self);
GAME_FN(void, __thiscall, NiMatrix3_MakeRotation, 0x00768820, float* self, float angle, float x, float y, float z);
GAME_FN(float, __thiscall, Vector_NormaliseSafe, 0x00415130, CVector* v);
GAME_FN(void, __cdecl, Transform_PreMultiply, 0x00415E00, NiTransform* out, const NiTransform* a, const NiTransform* b);
GAME_FN(NiTransform*, __cdecl, Transform_Combine, 0x00415E30, NiTransform* out, const NiTransform* in, int mode);

// 0x415E00: out = b * a
void __cdecl PreMultiply(NiTransform* out, const NiTransform* a, const NiTransform* b) {
    NiTransform result;
    *out = *NiTransform_Multiply(b, &result, a);
}

// 0x415E30: `in` into `out` by mode (0 replace, 1 out * in, 2 in * out)
NiTransform* __cdecl Combine(NiTransform* out, const NiTransform* in, int mode) {
    if (mode == 0) {
        *out = *in;
        return out;
    }
    if (mode == 1) {
        NiTransform result;
        *out = *NiTransform_Multiply(out, &result, in);
        return out;
    }
    if (mode == 2) {
        Transform_PreMultiply(out, out, in);
        return out;
    }
    return out;
}

// 0x415EB0: a rotation of `degrees` (negated) about `axis` combined in
NiTransform* __cdecl CombineRotation(NiTransform* out, const CVector* axis, float degrees, int mode) {
    NiTransform rotation;
    rotation.scale = 1.0f;
    NiTransform_MakeIdentity(&rotation);
    CVector unit = *axis;
    Vector_NormaliseSafe(&unit);
    float radians = static_cast<float>(-(degrees * 3.1415927410125732 / 180.0));
    NiMatrix3_MakeRotation(rotation.rotate, radians, unit.x, unit.y, unit.z);
    Transform_Combine(out, &rotation, mode);
    return out;
}

// 0x415F50: a translation combined in
NiTransform* __cdecl CombineTranslation(NiTransform* out, const CVector* translation, int mode) {
    NiTransform move;
    move.scale = 1.0f;
    NiTransform_MakeIdentity(&move);
    move.translate = *translation;
    Transform_Combine(out, &move, mode);
    return out;
}

// GTA's CColPoint, what's set of it here (0x28 bytes at least)
struct CColPoint {
    CVector point;       // +0
    uint32_t unknownC;   // +0xC
    CVector normal;      // +0x10
    uint8_t unknown1C[8];
    float depth;         // +0x24
};

// 0x415FA0 (thiscall): the constructor (no depth yet: FLT_MAX)
CColPoint* __fastcall ColPoint_Construct(CColPoint* self, re::Edx) {
    self->point = {0.0f, 0.0f, 0.0f};
    self->unknownC = 0;
    self->normal = {0.0f, 0.0f, 0.0f};
    self->depth = 3.4028234663852886e+38f;
    return self;
}

// The trigger pool (a CPool: objects +0, flags +4, element size +0xC)
GAME_VAR(uint8_t*, ms_pTriggerPool, 0x020C7C48);

// 0x415FD0 (thiscall, `this` unused: its callers pass the trigger manager): a trigger by slot
// (nullptr below 0 or for a free slot)
uint8_t* __fastcall TriggerAt(void*, re::Edx, int slot) {
    if (slot < 0) {
        return nullptr;
    }
    uint8_t* pool = ms_pTriggerPool;
    uint8_t* flags = *reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4);
    if (flags[slot] & 0x80) {
        return nullptr;
    }
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(pool) + *reinterpret_cast<int32_t*>(pool + 0xC) * slot;
}

} // namespace c01nitrans

// ---- registration ----------------------------------------------------------------------

#define NITRANS_REVERSED_C01(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01nitrans {
NITRANS_REVERSED_C01("NiTransformOps::PreMultiply", 0x00415E00, &PreMultiply);
NITRANS_REVERSED_C01("NiTransformOps::Combine", 0x00415E30, &Combine);
NITRANS_REVERSED_C01("NiTransformOps::CombineRotation", 0x00415EB0, &CombineRotation);
NITRANS_REVERSED_C01("NiTransformOps::CombineTranslation", 0x00415F50, &CombineTranslation);
NITRANS_REVERSED_C01("CColPoint::CColPoint", 0x00415FA0, &ColPoint_Construct);
NITRANS_REVERSED_C01("CTriggerPool::GetAt", 0x00415FD0, &TriggerAt);
} // namespace c01nitrans
