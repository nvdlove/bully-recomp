// Small AM_ animation helpers: an offset clamped to a length range, a skeleton's joint
// transforms filled from its source, the model's bounding sphere moved onto its root
// joint (the atomics' spheres), a 16-bit key's rotation angle, and an attachment's
// "has an offset" test. 0x6B43D0..0x6B45ED, 0x6B4AD0, 0x6B5650, 0x6BCC00, 0x6BD240..0x6BD2A6.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

constexpr float kShort = 3.0518509447574615e-05f;

GAME_VAR(int32_t, am_clampOffsets, 0x00BB6248);
GAME_VAR(uint8_t, am_flag20C5BE5, 0x020C5BE5);
GAME_VAR(uint8_t, am_flag20C5BE6, 0x020C5BE6);

GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(void, __cdecl, Rw_ForAllAtomics, 0x006C7380, void* clump, void* callback, void* data);
GAME_FN(float*, __cdecl, Atomic_GetBoundingSphere, 0x006C6D30, void* atomic);
GAME_FN(void*, __cdecl, ReturnFirst_824EC0, 0x00824EC0, void* atomic);  // (the game's shared no-op)
GAME_FN(void, __thiscall, NiQuat_ToAngleAxis, 0x00775F50, const float* q, float* angle, CVector* axis);
inline void* const kLargestSphereCallback = reinterpret_cast<void*>(0x006BCC00);
inline void* const kSetSphereCallback = reinterpret_cast<void*>(0x006BD270);

// 0x6B43D0: to - from, its length kept within [minScale, maxScale] * scale when the
// clamp's on (0xBB6248)
CVector* __cdecl ClampedOffset(CVector* out, const CVector* from, const CVector* to, float maxScale, float minScale,
                               float scale) {
    out->x = to->x - from->x;
    out->y = to->y - from->y;
    float z = to->z - from->z;
    out->z = z;
    float length = std::sqrt(std::fabs((out->x * out->x + out->y * out->y) + z * z));
    Vector_Normalise(out);
    if (am_clampOffsets != 0) {
        float most = maxScale * scale;
        if (length > most) {
            length = most;
        }
        float least = scale * minScale;
        if (least > length) {
            length = least;
        }
    }
    out->x = out->x * length;
    out->y = out->y * length;
    out->z = length * out->z;
    return out;
}

// 0x6B44B0: fills a skeleton's joint transforms (0x34 bytes each) from its source (+0x1C:
// the converter at +0x3C, the joints' stride at +0x24, the joints at +0x4C)
void __cdecl FillJointTransforms(uint8_t* skeleton) {
    uint32_t buffer[13];
    reinterpret_cast<float&>(buffer[12]) = 1.0f;  // (kept from call to call unless written)
    uint8_t* source = At<uint8_t*>(skeleton, 0x1C);
    using Convert = void(__cdecl*)(void* out, const void* in);
    Convert convert = At<Convert>(source, 0x3C);
    int32_t stride = At<int32_t>(source, 0x24);
    uint8_t* in = source + 0x4C;
    uint32_t* out = At<uint32_t*>(skeleton, 8);
    for (int i = 0; i < At<int32_t>(skeleton, 4); i++) {
        convert(buffer, in);
        in += stride;
        for (int w = 0; w < 13; w++) {
            out[w] = buffer[w];
        }
        out += 13;
    }
}

// 0x6BCC00: Rw_ForAllAtomics callback: keeps the atomic's bounding sphere when it's the
// largest so far
void* __cdecl LargestSphereCallback(void* atomic, re::Ptr32<float>* largest) {
    float* current = *largest;
    float* sphere = Atomic_GetBoundingSphere(atomic);
    if (current[3] < sphere[3]) {
        *largest = sphere;
    }
    return atomic;
}

// 0x6BD270: Rw_ForAllAtomics callback: sets the atomic's sphere (+0x38) when it has a frame
void* __cdecl SetSphereCallback(uint8_t* atomic, re::Ptr32<float>* sphere) {
    const float* s = *sphere;
    ReturnFirst_824EC0(atomic);
    if (At<void*>(atomic, 0x14) != nullptr) {
        At<float>(atomic, 0x38) = s[0];
        At<float>(atomic, 0x3C) = s[1];
        At<float>(atomic, 0x40) = s[2];
        At<float>(atomic, 0x44) = s[3];
    }
    return atomic;
}

// 0x6B4530: (flag 0x10 of +0x98) the model's largest bounding sphere centred on its root
// joint (2 in some cutscene state), grown by half once (flag 0x20), for every atomic
void __fastcall CentreSphereOnRoot(uint8_t* self, re::Edx) {
    if (!(At<uint8_t>(self, 0x98) & 0x10)) {
        return;
    }
    float none[4];
    none[3] = 0.0f;
    float* sphere = none;
    Rw_ForAllAtomics(At<void*>(self, 0x10), kLargestSphereCallback, &sphere);
    if (sphere == none) {
        return;
    }
    int joint = 1;
    if (am_flag20C5BE6 == 1 && am_flag20C5BE5 == 0) {
        joint = 2;
    }
    const float* pos = reinterpret_cast<const float*>(At<uint8_t*>(At<uint8_t*>(self, 0x14), 8) + joint * 0x34 + 0x24);
    sphere[0] = pos[0];
    sphere[1] = pos[1];
    sphere[2] = pos[2];
    if (At<uint8_t>(self, 0x98) & 0x20) {
        sphere[3] = sphere[3] * 1.5f;
        At<uint32_t>(self, 0x98) &= ~0x20u;
    }
    Rw_ForAllAtomics(At<void*>(self, 0x10), kSetSphereCallback, &sphere);
}

// 0x6B4AD0: a 16-bit key's rotation angle (in 0..360 by the axis's z)
void __cdecl ShortKey_GetAngle(const uint8_t* key, float* angle) {
    float q[4];  // w first
    q[1] = float(*reinterpret_cast<const int16_t*>(key + 4)) * kShort;
    q[2] = float(*reinterpret_cast<const int16_t*>(key + 6)) * kShort;
    q[3] = float(*reinterpret_cast<const int16_t*>(key + 8)) * kShort;
    q[0] = kShort * float(*reinterpret_cast<const int16_t*>(key + 0xA));
    CVector axis;
    NiQuat_ToAngleAxis(q, angle, &axis);
    if (axis.z < 0.0f) {
        *angle = 360.0f - *angle;
    }
}

// 0x6B5650: whether an attachment has an offset (+0x14..0x1C non-zero, or +0x30 and +0x20
// both set)
uint32_t __fastcall HasOffset(uint8_t* self, re::Edx) {
    float x = At<float>(self, 0x14);
    float y = At<float>(self, 0x18);
    float z = At<float>(self, 0x1C);
    if (0.0f == x && 0.0f == y && 0.0f == z) {
        if (At<int32_t>(self, 0x30) == 0 || At<int32_t>(self, 0x20) == 0) {
            return 0;
        }
    }
    return 1;
}

// 0x6BD240: whether two handles hold the same thing (both empty counts)
uint32_t __fastcall Handle_Equals(re::Ptr32<void>* self, re::Edx, re::Ptr32<void>* other) {
    uint32_t* a = static_cast<uint32_t*>(*self);
    if (a == nullptr) {
        return *other == nullptr ? 1 : 0;
    }
    uint32_t* b = static_cast<uint32_t*>(*other);
    if (b == nullptr) {
        return 0;
    }
    return *a == *b ? 1 : 0;
}

} // namespace

#define AMMISC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMMISC_REVERSED("AM::ClampedOffset", 0x006B43D0, &ClampedOffset);
AMMISC_REVERSED("AM_Skeleton::FillJointTransforms", 0x006B44B0, &FillJointTransforms);
AMMISC_REVERSED("AM_Model::CentreSphereOnRoot", 0x006B4530, &CentreSphereOnRoot);
AMMISC_REVERSED("AM_ShortKey::GetAngle", 0x006B4AD0, &ShortKey_GetAngle);
AMMISC_REVERSED("AM_Attachment::HasOffset", 0x006B5650, &HasOffset);
AMMISC_REVERSED("AM::LargestSphereCallback", 0x006BCC00, &LargestSphereCallback);
AMMISC_REVERSED("AM::Handle_Equals", 0x006BD240, &Handle_Equals);
AMMISC_REVERSED("AM::SetSphereCallback", 0x006BD270, &SetSphereCallback);
