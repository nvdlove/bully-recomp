// The AM_ animation system's keys (Rockstar Vancouver's anim library): decoding the
// compressed kinds, finding the key pair around a time, and the sockets' defaults
// (AM_Socket, vtable 0x941F14; AM_SocketOffSet's constructor). 0x6B17C0..0x6B2EA3.
//
// A track's keys (the track: +4 count, +0x10 the keys; the kind at **track):
//   999 (0x20 bytes), 1000 / 1003 (0x14: a time then a float quaternion),
//   1001 / 1004 (0xC: packed in three words), 1002 (8: 16-bit shorts).
// The packed keys: bits 0..10 of the first word the previous key's index, 11..19 the
// time (/ 511), the rest the quaternion's 10-bit parts with their sign bits beside.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace {

using re::At;

constexpr float kTime9 = 0.001956947147846222f;    // 1 / 511
constexpr float kQuat10 = 0.0009775171056389809f;  // 1 / 1023
constexpr float kShort = 3.0518509447574615e-05f;  // 1 / 32767
constexpr float kTime16 = 1.5259021893143654e-05f; // 1 / 65535
static_assert(double(kTime9) == 0.001956947147846222 && double(kQuat10) == 0.0009775171056389809);
static_assert(double(kShort) == 3.0518509447574615e-05 && double(kTime16) == 1.5259021893143654e-05);

GAME_ARRAY(float, am_zeroVector, 0x00BCC564);     // (0, 0, 0)
GAME_ARRAY(float, am_identityQuat, 0x020C48A8);   // x, y, z, w

GAME_FN(void*, __cdecl, RW_FrameFirstObject, 0x006C7700, void* frame);
GAME_FN(void*, __cdecl, RW_ForAllChildren, 0x006C9B50, void* frame, void* callback, void* data);
GAME_FN(void*, __cdecl, Rw_GetFrame, 0x006C7370, void* object);
GAME_FN(int, __cdecl, DataStream_Read, 0x0042E730, void* stream, void* to, int size);
GAME_FN(float, __cdecl, SafeAcos, 0x00413D60, float x);
// this file's
GAME_FN(int32_t, __cdecl, FindKeyPacked, 0x006B29E0, uint8_t* track, int, int32_t base, int32_t first, int32_t stride,
        float t);
GAME_FN(int32_t, __cdecl, FindKey16, 0x006B2B00, uint8_t* track, int, int32_t base, int32_t first, int32_t stride,
        float t);
inline void* const kFirstObjectCallback = reinterpret_cast<void*>(0x006B1A90);

// the packed parts' sign bits go straight into the float's
inline void OrBits(float& f, uint32_t bits) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    u |= bits;
    std::memcpy(&f, &u, 4);
}

// 0x6B17C0: AM_Socket's constructor
void* __fastcall Socket_Construct(uint8_t* self, re::Edx) {
    At<void*>(self, 0) = reinterpret_cast<void*>(0x00941F14);
    return self;
}

// 0x6B17D0: AM_Socket::GetPosition (none: the zero vector)
float* __fastcall Socket_GetPosition(void*, re::Edx) {
    return am_zeroVector;
}

// 0x6B17E0: AM_Socket::GetRotation (none: the identity)
float* __fastcall Socket_GetRotation(void*, re::Edx) {
    return am_identityQuat;
}

// 0x6B1DD0: AM_Socket's vtable slot 3 (three zeroes)
void __fastcall Socket_GetZeroes(void*, re::Edx, float* a, float* b, float* c) {
    *a = 0.0f;
    *b = 0.0f;
    *c = 0.0f;
}

// 0x6B2D00: AM_SocketOffSet's constructor (the base's offsets copied in)
void* __fastcall SocketOffSet_Construct(uint8_t* self, re::Edx) {
    At<void*>(self, 0) = reinterpret_cast<void*>(0x00941F5C);
    for (int i = 0; i < 3; i++) {
        At<float>(self, 0xC + i * 4) = am_zeroVector[i];
    }
    for (int i = 0; i < 4; i++) {
        At<float>(self, 0x18 + i * 4) = am_identityQuat[i];
    }
    return self;
}

// 0x6B1820: a float key's quaternion (at +4)
void __fastcall FloatKey_GetQuat(uint8_t* key, re::Edx, float* out) {
    for (int i = 0; i < 4; i++) {
        out[i] = At<float>(key, 4 + i * 4);
    }
}

// 0x6B1840: a packed key's time
float __fastcall PackedKey_GetTime(uint32_t* key, re::Edx) {
    return float((key[0] >> 11) & 0x1FF) * kTime9;
}

// 0x6B1870: a packed key's quaternion (x, y, z, w in out[1..3], out[0])
void __fastcall PackedKey_GetQuat(uint32_t* key, re::Edx, float* out) {
    out[1] = float((key[0] >> 21) & 0x3FF) * kQuat10;
    OrBits(out[1], (key[0] & 0xFFF00000) << 11);
    out[2] = float(key[1] & 0x3FF) * kQuat10;
    OrBits(out[2], key[0] & 0x80000000);
    out[3] = float((key[1] >> 11) & 0x3FF) * kQuat10;
    OrBits(out[3], (key[1] & 0xFFFFFC00) << 21);
    out[0] = float(key[1] >> 22) * kQuat10;
    OrBits(out[0], (key[1] & 0xFFE00000) << 10);
}

// 0x6B1940: a 16-bit key's quaternion (shorts at +4..+0xA, w last)
void __fastcall ShortKey_GetQuat(uint8_t* key, re::Edx, float* out) {
    out[1] = float(At<int16_t>(key, 4)) * kShort;
    out[2] = float(At<int16_t>(key, 6)) * kShort;
    out[3] = float(At<int16_t>(key, 8)) * kShort;
    out[0] = kShort * float(At<int16_t>(key, 0xA));
}

// 0x6B1990: a 16-bit key's translation (shorts at +0xC, in thousandths)
void __fastcall ShortKey_GetTranslation(uint8_t* key, re::Edx, float* out) {
    constexpr float kMilli = 0.0010000000474974513f;
    out[0] = float(At<int16_t>(key, 0xC)) * kMilli;
    out[1] = float(At<int16_t>(key, 0xE)) * kMilli;
    out[2] = kMilli * float(At<int16_t>(key, 0x10));
}

// 0x6B19D0: a packed key's translation (the third word: 10, 10 and 9 bits, hundredths)
void __fastcall PackedKey_GetTranslation(uint8_t* key, re::Edx, float* out) {
    constexpr float kCenti = 0.009999999776482582f;
    uint32_t w = At<uint32_t>(key, 8);
    out[0] = float(w & 0x3FF) * kCenti;
    OrBits(out[0], (w & 0xFFFFFC00) << 21);
    out[1] = float((w >> 11) & 0x3FF) * kCenti;
    OrBits(out[1], (w & 0xFFE00000) << 10);
    out[2] = float((w >> 22) & 0x1FF) * kCenti;
    OrBits(out[2], w & 0x80000000);
}

// 0x6B1A70: an anim's kind (flag 0x200 of +0x18: 2, else 1)
int __cdecl Anim_GetKind(uint8_t* anim) {
    return (At<uint32_t>(anim, 0x18) & 0x200) ? 2 : 1;
}

// 0x6B2DD0
uint8_t __cdecl Anim_IsKind(uint8_t* anim, int kind) {
    return kind == ((At<uint32_t>(anim, 0x18) & 0x200) ? 2 : 1) ? 1 : 0;
}

// 0x6B1A90: the children callback below (stops at the first with an object)
void* __cdecl FirstObjectCallback(void* frame, re::Ptr32<void>* out) {
    void* object = RW_FrameFirstObject(frame);
    if (object == nullptr) {
        RW_ForAllChildren(frame, kFirstObjectCallback, out);
        return frame;
    }
    *out = object;
    return nullptr;
}

// 0x6B1AD0: the first object under an object's frame
void* __cdecl FindFirstObjectBelow(void* object) {
    void* found = nullptr;
    if (Rw_GetFrame(object) == nullptr) {
        return nullptr;
    }
    RW_ForAllChildren(Rw_GetFrame(object), kFirstObjectCallback, &found);
    return found;
}

// 0x6B1C10: whether two quaternions differ (1) or are equal (0)
uint32_t __fastcall Quat_NotEqual(const float* self, re::Edx, const float* other) {
    for (int i = 0; i < 4; i++) {
        if (!(other[i] == self[i])) {
            return 1;
        }
    }
    return 0;
}

// 0x6B1EE0: normalises a quaternion
void __fastcall Quat_Normalise(float* q, re::Edx) {
    float lengthSq = ((q[1] * q[1] + q[0] * q[0]) + q[2] * q[2]) + q[3] * q[3];
    float length = std::sqrt(lengthSq);
    float recip = 1.0f / length;
    q[0] = q[0] * recip;
    q[1] = recip * q[1];
    q[2] = q[2] * recip;
    q[3] = recip * q[3];
}

// 0x6B20F0: the angle opposite side c of a triangle with sides a, b, c (the cosine rule)
float __cdecl TriangleAngle(float a, float b, float c) {
    return SafeAcos(((a * a + b * b) - c * c) / (b * (a + a)));
}

// 0x6B2D50: whether two points are within `tolerance` on every axis
uint32_t __cdecl Vec_WithinTolerance(const float* a, const float* b, float tolerance) {
    if (std::fabs(a[0] - b[0]) < tolerance && std::fabs(a[1] - b[1]) < tolerance &&
        std::fabs(a[2] - b[2]) < tolerance) {
        return 1;
    }
    return 0;
}

// 0x6B2BF0: the stream when `size` bytes were read into `to`, else null
void* __cdecl Stream_ReadExactly(void* stream, void* to, int size) {
    return DataStream_Read(stream, to, size) == size ? stream : nullptr;
}

// 0x6B29E0 / 0x6B2B00: from key `first`, the index of the key starting the span that
// holds time t (keys chain to the previous one by index; a key and its previous one's
// times around t end the search). The previous key's time is read at a fixed stride
// (8 / 4), the chain compared at the given one (as the game does)
template <bool Packed>
int32_t FindKeyImpl(uint8_t* track, int32_t base, int32_t first, int32_t stride, float t) {
    int32_t result = base + first;
    int32_t index = first * 2;
    uint8_t* data = At<uint8_t*>(track, 0x10);
    uint8_t* end = At<uint32_t>(track, 4) * stride + data;
    uint8_t* cur = result * stride + data;
    bool found = false;
    for (uint8_t* key = index * stride + data; key < end && !found; key += stride, index++) {
        uint32_t previous = Packed ? (At<uint32_t>(key, 0) & 0x7FF) : At<uint16_t>(key, 0);
        if (previous * stride + At<uint8_t*>(track, 0x10) != cur) {
            continue;
        }
        result = index;
        cur = key;
        if (!(t < 1.0f)) {
            continue;
        }
        float previousTime, keyTime;
        if constexpr (Packed) {
            previousTime = float((At<uint32_t>(At<uint8_t*>(track, 0x10), previous * 8) >> 11) & 0x1FF) * kTime9;
        } else {
            previousTime = float(At<uint16_t>(At<uint8_t*>(track, 0x10), previous * 4 + 2)) * kTime16;
        }
        if (!(previousTime <= t)) {
            found = false;
            continue;
        }
        if constexpr (Packed) {
            keyTime = float((At<uint32_t>(key, 0) >> 11) & 0x1FF) * kTime9;
        } else {
            keyTime = float(At<uint16_t>(key, 2)) * kTime16;
        }
        found = keyTime > t;
    }
    return result;
}

// 0x6B29E0
int32_t __cdecl FindKeyPackedImpl(uint8_t* track, int, int32_t base, int32_t first, int32_t stride, float t) {
    return FindKeyImpl<true>(track, base, first, stride, t);
}

// 0x6B2B00
int32_t __cdecl FindKey16Impl(uint8_t* track, int, int32_t base, int32_t first, int32_t stride, float t) {
    return FindKeyImpl<false>(track, base, first, stride, t);
}

// 0x6B2E00: an anim channel's key at time t (the channel: +0 its track, +8 the
// owner's first key at +0x5C, +0x18 flag 2: packed keys); -1 for an unknown kind
int32_t __fastcall Channel_FindKey(uint8_t* self, re::Edx, int32_t base, float t) {
    uint8_t* track = At<uint8_t*>(self, 0);
    if (track == nullptr) {
        return 0;
    }
    int32_t first = At<int32_t>(At<uint8_t*>(self, 8), 0x5C);
    int32_t stride;
    switch (**reinterpret_cast<re::Ptr32<int32_t>*>(track)) {
    case 1001:
        stride = 0xC;
        break;
    case 1002:
        stride = 8;
        break;
    case 1000:
        stride = 0x14;
        break;
    case 1003:
        stride = 0x14;
        break;
    case 1004:
        stride = 0xC;
        break;
    case 999:
        stride = 0x20;
        break;
    default:
        return -1;
    }
    if (At<uint8_t>(self, 0x18) & 2) {
        return FindKeyPacked(track, At<int32_t>(track, 0x10), base, first, stride, t);
    }
    return FindKey16(track, At<int32_t>(track, 0x10), base, first, stride, t);
}

} // namespace

#define AMKEYS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMKEYS_REVERSED("AM_Socket::AM_Socket", 0x006B17C0, &Socket_Construct);
AMKEYS_REVERSED("AM_Socket::GetPosition", 0x006B17D0, &Socket_GetPosition);
AMKEYS_REVERSED("AM_Socket::GetRotation", 0x006B17E0, &Socket_GetRotation);
AMKEYS_REVERSED("AM_FloatKey::GetQuat", 0x006B1820, &FloatKey_GetQuat);
AMKEYS_REVERSED("AM_PackedKey::GetTime", 0x006B1840, &PackedKey_GetTime);
AMKEYS_REVERSED("AM_PackedKey::GetQuat", 0x006B1870, &PackedKey_GetQuat);
AMKEYS_REVERSED("AM_ShortKey::GetQuat", 0x006B1940, &ShortKey_GetQuat);
AMKEYS_REVERSED("AM_ShortKey::GetTranslation", 0x006B1990, &ShortKey_GetTranslation);
AMKEYS_REVERSED("AM_PackedKey::GetTranslation", 0x006B19D0, &PackedKey_GetTranslation);
AMKEYS_REVERSED("AM_Anim::GetKind", 0x006B1A70, &Anim_GetKind);
AMKEYS_REVERSED("AM::FirstObjectCallback", 0x006B1A90, &FirstObjectCallback);
AMKEYS_REVERSED("AM::FindFirstObjectBelow", 0x006B1AD0, &FindFirstObjectBelow);
AMKEYS_REVERSED("AM_Quat::NotEqual", 0x006B1C10, &Quat_NotEqual);
AMKEYS_REVERSED("AM_Socket::GetZeroes", 0x006B1DD0, &Socket_GetZeroes);
AMKEYS_REVERSED("AM_Quat::Normalise", 0x006B1EE0, &Quat_Normalise);
AMKEYS_REVERSED("AM::TriangleAngle", 0x006B20F0, &TriangleAngle);
AMKEYS_REVERSED("AM::FindKeyPacked", 0x006B29E0, &FindKeyPackedImpl);
AMKEYS_REVERSED("AM::FindKey16", 0x006B2B00, &FindKey16Impl);
AMKEYS_REVERSED("AM::Stream_ReadExactly", 0x006B2BF0, &Stream_ReadExactly);
AMKEYS_REVERSED("AM_SocketOffSet::AM_SocketOffSet", 0x006B2D00, &SocketOffSet_Construct);
AMKEYS_REVERSED("AM::Vec_WithinTolerance", 0x006B2D50, &Vec_WithinTolerance);
AMKEYS_REVERSED("AM_Anim::IsKind", 0x006B2DD0, &Anim_IsKind);
AMKEYS_REVERSED("AM_Channel::FindKey", 0x006B2E00, &Channel_FindKey);
