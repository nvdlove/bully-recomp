// The AM_ key kinds' codecs (registered by type at 0x6C05D2: interpolate, read, write,
// size): 999 float keys with translations (0x20), 1000 float keys (0x14) + separate
// translation keys (0x10), 1001 short keys (0xC) + separate 8-byte translations, 1002
// packed keys (8) + separate 8-byte translations, 1003 short keys with translations
// (0x14), 1004 packed keys with translations (0xC). The keys' interpolation goes through
// two slerps taking their arguments in registers (0x6BB7F0, 0x6BB940; written here as
// helpers, not hooked). The track being sampled: its anim at 0x20C4904, the joint at
// 0x20C4908, the rest translations at 0x20C4900. 0x6BB570..0x6BCBFF.

#include <cstdint>
#include <cstring>

#include "game/FileLoader.h"
#include "game/TrackMath.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;
template <typename T>
inline T Get(const void* p, int offset) {
    T v;
    std::memcpy(&v, static_cast<const uint8_t*>(p) + offset, sizeof(T));
    return v;
}

// a key decoded for interpolating: its time, rotation (x, y, z, w), translation
struct Sample {
    float time;
    float q[4];
    float t[3];
};
static_assert(sizeof(Sample) == 0x20);

constexpr float kTime9 = 0.001956947147846222f;
constexpr float kTime16 = 1.5259021893143654e-05f;
constexpr float kQuat10 = 0.0009775171056389809f;
constexpr float kShort = 3.0518509447574615e-05f;
constexpr float kMilli = 0.0010000000474974513f;

GAME_VAR(uint8_t*, codec_anim, 0x020C4904);
GAME_VAR(int32_t, codec_joint, 0x020C4908);
GAME_VAR(float*, codec_restTranslations, 0x020C4900);

GAME_FN(int, __cdecl, DataStream_Read, 0x0042E730, void* stream, void* to, int size);
GAME_FN(uint32_t, __cdecl, File_WriteItems, 0x0042D110, const void* buffer, uint32_t size, uint32_t count, void* file);
inline void* __cdecl Memcpy(void* dst, const void* src, size_t n) { return crt::Memcpy(dst, src, n); }
GAME_FN(void*, __cdecl, Stream_Write, 0x006BB670, FileStream* stream, const void* from, uint32_t size);
GAME_FN(void, __thiscall, PackedKey_GetQuatXYZW, 0x006BB6E0, const uint8_t* key, float* out);
GAME_FN(void, __thiscall, PackedKey_GetTranslation, 0x006B19D0, const uint8_t* key, float* out);
GAME_FN(uint8_t, __cdecl, Anim_IsKind, 0x006B2DD0, uint8_t* anim, int kind);

inline float CrtAcos(float x) {
    return crt::Acosf(x);
}

// the anim's translation keys past its rotation keys (the extra root's count added)
inline uint32_t MoveCount() {
    uint8_t* anim = codec_anim;
    if (At<uint8_t>(anim, 0x18) & 0x40) {
        return uint32_t(At<uint16_t>(anim, 0x2C)) + At<uint16_t>(anim, 0x2A);
    }
    return At<uint16_t>(anim, 0x2A);
}

// 0x6BB570 / 0x6BB650 / 0x6BB660: the keys' size
template <int Stride>
uint32_t __cdecl KeysSize(uint8_t* track) {
    return At<uint32_t>(track, 4) * Stride;
}
// 0x6BB580 / 0x6BB5C0 / 0x6BB610: with the translation keys
template <int Stride, int MoveStride>
uint32_t __cdecl KeysAndMovesSize(uint8_t* track) {
    return MoveCount() * MoveStride + At<uint32_t>(track, 4) * Stride;
}

// 0x6BBAC0 / 0x6BBF50 / 0x6BC1E0: reads the keys (the track, or null when short)
template <int Stride>
uint8_t* __cdecl ReadKeys(void* stream, uint8_t* track) {
    int read = DataStream_Read(stream, At<void*>(track, 0x10), int(At<uint32_t>(track, 4) * Stride));
    return read != 0 ? track : nullptr;
}
// 0x6BBC50 / 0x6BBD30 / 0x6BBE50
template <int Stride, int MoveStride>
uint8_t* __cdecl ReadKeysAndMoves(void* stream, uint8_t* track) {
    if (DataStream_Read(stream, At<void*>(track, 0x10), int(At<uint32_t>(track, 4) * Stride)) == 0) {
        return nullptr;
    }
    uint32_t moves = MoveCount();
    uint8_t* at = At<uint8_t*>(track, 0x10) + At<uint32_t>(track, 4) * Stride;
    if (DataStream_Read(stream, at, int(moves * MoveStride)) == 0) {
        return nullptr;
    }
    return track;
}

// 0x6BBAF0 / 0x6BBF80 / 0x6BC210: writes the keys (1: done)
template <int Stride>
int __cdecl WriteKeys(uint8_t* track, FileStream* stream) {
    return Stream_Write(stream, At<void*>(track, 0x10), At<uint32_t>(track, 4) * Stride) != nullptr ? 1 : 0;
}
// 0x6BBCC0 / 0x6BBDC0 / 0x6BBED0
template <int Stride, int MoveStride>
int __cdecl WriteKeysAndMoves(uint8_t* track, FileStream* stream) {
    if (Stream_Write(stream, At<void*>(track, 0x10), At<uint32_t>(track, 4) * Stride) == nullptr) {
        return 0;
    }
    uint32_t moves = MoveCount();
    uint8_t* at = At<uint8_t*>(track, 0x10) + At<uint32_t>(track, 4) * Stride;
    return Stream_Write(stream, at, moves * MoveStride) != nullptr ? 1 : 0;
}

// 0x6BB670: writes to a stream (the stream, or null when short or not writable)
FileStream* __cdecl StreamWrite(FileStream* stream, const void* from, uint32_t size) {
    if (stream == nullptr) {
        return nullptr;
    }
    if (stream->file) {
        return File_WriteItems(from, size, 1, stream->file) == size ? stream : nullptr;
    }
    if (stream->data) {
        Memcpy(stream->data + stream->position, from, size);
        stream->position += size;
        return stream;
    }
    return nullptr;
}

inline void OrBits(float& f, uint32_t bits) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    u |= bits;
    std::memcpy(&f, &u, 4);
}

// 0x6BB6E0: a packed key's rotation as x, y, z, w
void __fastcall PackedQuatXYZW(const uint32_t* key, re::Edx, float* out) {
    uint32_t w0 = key[0];
    uint32_t w1 = key[1];
    float q[4];
    q[0] = float((w0 >> 21) & 0x3FF) * kQuat10;
    OrBits(q[0], (w0 & 0xFFF00000) << 11);
    q[1] = float(w1 & 0x3FF) * kQuat10;
    OrBits(q[1], w0 & 0x80000000);
    q[2] = float((w1 >> 11) & 0x3FF) * kQuat10;
    OrBits(q[2], (w1 & 0xFFFFFC00) << 21);
    q[3] = float(w1 >> 22) * kQuat10;
    OrBits(q[3], (w1 & 0xFFE00000) << 10);
    std::memcpy(out, q, 16);
}

// the slerp weights (w for the first-named key, u for the second)
inline void SlerpWeights(float cosine, float& w, float& u) {
    if (cosine < 0.999f) {
        float theta = CrtAcos(cosine);
        float recip = 1.0f / trk::CrtSin(theta);
        w = trk::CrtSin(theta * w) * recip;
        u = trk::CrtSin(theta * u) * recip;
    }
}

// 0x6BB7F0 (esi = a, edi = b, ebx = out, t on the stack): the rotation between b (at its
// time) and a, a's rotation flipped onto b's side
void SlerpRotation(Sample* a, const Sample* b, uint8_t* out, float t) {
    float cosine = ((a->q[1] * b->q[1] + a->q[0] * b->q[0]) + a->q[2] * b->q[2]) + a->q[3] * b->q[3];
    float u = (t - b->time) / (a->time - b->time);
    if (cosine < 0.0f) {
        cosine = -cosine;
        a->q[0] = a->q[0] * -1.0f;
        a->q[1] = a->q[1] * -1.0f;
        a->q[2] = a->q[2] * -1.0f;
        a->q[3] = -1.0f * a->q[3];
    }
    float w = 1.0f - u;
    SlerpWeights(cosine, w, u);
    At<float>(out, 0xC) = b->q[0] * w + u * a->q[0];
    At<float>(out, 0x10) = b->q[1] * w + a->q[1] * u;
    At<float>(out, 0x14) = b->q[2] * w + a->q[2] * u;
    At<float>(out, 8) = w * b->q[3] + a->q[3] * u;
}
RE_UNHOOKED_FN("AM::SlerpRotation", 0x006BB7F0, &SlerpRotation);

// 0x6BB940 (esi = a, edi = b, ebx = out, t): the rotation (slerp, b flipped) and the
// translation (lerp) from a to b
void SlerpSample(const Sample* a, Sample* b, uint8_t* out, float t) {
    float cosine = ((b->q[1] * a->q[1] + b->q[0] * a->q[0]) + b->q[2] * a->q[2]) + b->q[3] * a->q[3];
    float u = (t - a->time) / (b->time - a->time);
    At<float>(out, 0x18) = (b->t[0] - a->t[0]) * u + a->t[0];
    At<float>(out, 0x1C) = (b->t[1] - a->t[1]) * u + a->t[1];
    At<float>(out, 0x20) = (b->t[2] - a->t[2]) * u + a->t[2];
    if (cosine < 0.0f) {
        cosine = -cosine;
        b->q[0] = b->q[0] * -1.0f;
        b->q[1] = b->q[1] * -1.0f;
        b->q[2] = b->q[2] * -1.0f;
        b->q[3] = -1.0f * b->q[3];
    }
    float w = 1.0f - u;
    SlerpWeights(cosine, w, u);
    At<float>(out, 0xC) = a->q[0] * w + u * b->q[0];
    At<float>(out, 0x10) = a->q[1] * w + b->q[1] * u;
    At<float>(out, 0x14) = a->q[2] * w + b->q[2] * u;
    At<float>(out, 8) = w * a->q[3] + b->q[3] * u;
}
RE_UNHOOKED_FN("AM::SlerpSample", 0x006BB940, &SlerpSample);

inline void DecodeFloatKey(const uint8_t* k, Sample& s, bool translation) {
    s.q[0] = Get<float>(k, 8);
    s.q[1] = Get<float>(k, 0xC);
    s.q[2] = Get<float>(k, 0x10);
    s.q[3] = Get<float>(k, 4);
    if (translation) {
        s.t[0] = Get<float>(k, 0x14);
        s.t[1] = Get<float>(k, 0x18);
        s.t[2] = Get<float>(k, 0x1C);
    }
}
inline void DecodeShortKey(const uint8_t* k, Sample& s) {
    s.q[0] = float(Get<int16_t>(k, 4)) * kShort;
    s.q[1] = float(Get<int16_t>(k, 6)) * kShort;
    s.q[2] = float(Get<int16_t>(k, 8)) * kShort;
    s.q[3] = float(Get<int16_t>(k, 0xA)) * kShort;
}
inline void DecodeShorts3(const uint8_t* p, float* out) {
    out[0] = float(Get<int16_t>(p, 0)) * kMilli;
    out[1] = float(Get<int16_t>(p, 2)) * kMilli;
    out[2] = float(Get<int16_t>(p, 4)) * kMilli;
}
inline float Time16(const uint8_t* k) {
    return float(Get<uint16_t>(k, 2)) * kTime16;
}
inline float Time9(const uint8_t* k) {
    return float((Get<uint32_t>(k, 0) >> 11) & 0x1FF) * kTime9;
}
// the anim's root offset (flag 0x100) on both translations when the joint is its root
inline void AddRootOffset(Sample& a, Sample& b) {
    uint8_t* anim = codec_anim;
    if (Anim_IsKind(codec_anim, codec_joint) && (At<uint32_t>(anim, 0x18) & 0x100)) {
        for (int i = 0; i < 3; i++) {
            a.t[i] = a.t[i] + At<float>(anim, 0x34 + i * 4);
            b.t[i] = b.t[i] + At<float>(anim, 0x34 + i * 4);
        }
    }
}
// rotation only, the joint's rest translation
inline void RotationOnly(Sample& a, Sample& b, uint8_t* out, float t) {
    SlerpRotation(&b, &a, out, t);
    const float* rest = codec_restTranslations + codec_joint * 3;
    At<float>(out, 0x18) = rest[0];
    At<float>(out, 0x1C) = rest[1];
    At<float>(out, 0x20) = rest[2];
}

// 0x6BBB20: 999 (float keys with translations)
void __cdecl Interp999(uint8_t* out, const uint8_t* k1, const uint8_t* k2, float t) {
    Sample a, b;
    a.time = Time16(k1);
    if (t < a.time) {
        a.time = t;
    }
    DecodeFloatKey(k1, a, true);
    b.time = Time16(k2);
    if (b.time < t) {
        b.time = t;
    }
    DecodeFloatKey(k2, b, true);
    SlerpSample(&a, &b, out, t);
}

// 0x6BBFB0: 1003 (short keys with translations)
void __cdecl Interp1003(uint8_t* out, const uint8_t* k1, const uint8_t* k2, float t) {
    Sample a, b;
    a.time = Time16(k1);
    if (t < a.time) {
        a.time = t;
    }
    DecodeShortKey(k1, a);
    DecodeShorts3(k1 + 0xC, a.t);
    b.time = Time16(k2);
    if (b.time < t) {
        b.time = t;
    }
    DecodeShortKey(k2, b);
    DecodeShorts3(k2 + 0xC, b.t);
    AddRootOffset(a, b);
    SlerpSample(&a, &b, out, t);
}

// 0x6BC240: 1004 (packed keys with translations)
void __cdecl Interp1004(uint8_t* out, const uint8_t* k1, const uint8_t* k2, float t) {
    Sample a, b;
    a.time = Time9(k1);
    if (t < a.time) {
        a.time = t;
    }
    PackedKey_GetQuatXYZW(k1, a.q);
    PackedKey_GetTranslation(k1, a.t);
    b.time = Time9(k2);
    if (t > b.time) {
        b.time = t;
    }
    PackedKey_GetQuatXYZW(k2, b.q);
    PackedKey_GetTranslation(k2, b.t);
    AddRootOffset(a, b);
    SlerpSample(&a, &b, out, t);
}

// 0x6BC3C0: 1000 (float keys; the translations from the key after the rotations naming
// this key)
void __cdecl Interp1000(uint8_t* out, const uint8_t* k1, const uint8_t* k2, float t) {
    Sample a, b;
    a.time = Time16(k1);
    if (t < a.time) {
        a.time = t;
    }
    DecodeFloatKey(k1, a, false);
    b.time = Time16(k2);
    if (b.time < t) {
        b.time = t;
    }
    DecodeFloatKey(k2, b, false);
    bool kind = Anim_IsKind(codec_anim, codec_joint) != 0;
    uint8_t* anim = codec_anim;
    float* rest = codec_restTranslations;
    if (!kind && (!(At<uint8_t>(anim, 0x18) & 0x40) || rest != nullptr)) {
        RotationOnly(a, b, out, t);
        return;
    }
    uint8_t* track = At<uint8_t*>(anim, 0);
    uint8_t* data = At<uint8_t*>(track, 0x10);
    uint8_t* moves = data + At<uint32_t>(track, 4) * 0x14;
    if (rest == nullptr) {
        moves += At<uint16_t>(anim, 0x2A) * 0x10;
    }
    uint32_t index = uint32_t(k1 - data) / 0x14;
    uint32_t i = 0;
    while (Get<uint32_t>(moves, i * 0x10) != index) {
        i++;
    }
    const uint8_t* m = moves + i * 0x10;
    a.t[0] = Get<float>(m, 4);
    a.t[1] = Get<float>(m, 8);
    a.t[2] = Get<float>(m, 0xC);
    b.t[0] = Get<float>(m, 0x14);
    b.t[1] = Get<float>(m, 0x18);
    b.t[2] = Get<float>(m, 0x1C);
    SlerpSample(&a, &b, out, t);
}

// 1001 / 1002 (short / packed keys with 8-byte translation keys naming them)
template <bool Packed>
void InterpWithMoves(uint8_t* out, const uint8_t* k1, const uint8_t* k2, float t) {
    constexpr uint32_t stride = Packed ? 8 : 0xC;
    Sample a, b;
    if constexpr (Packed) {
        a.time = Time9(k1);
        PackedKey_GetQuatXYZW(k1, a.q);
        if (t < a.time) {
            a.time = t;
        }
        b.time = Time9(k2);
        if (b.time < t) {
            b.time = t;
        }
        PackedKey_GetQuatXYZW(k2, b.q);
    } else {
        a.time = Time16(k1);
        if (t < a.time) {
            a.time = t;
        }
        DecodeShortKey(k1, a);
        b.time = Time16(k2);
        if (b.time < t) {
            b.time = t;
        }
        DecodeShortKey(k2, b);
    }
    bool kind = Anim_IsKind(codec_anim, codec_joint) != 0;
    float* rest = codec_restTranslations;
    if (!kind && rest != nullptr) {
        uint8_t* anim = codec_anim;
        if ((At<uint32_t>(anim, 0x18) & 0x200) && codec_joint == 1) {
            // the anim's own offset for both
            for (int i = 0; i < 3; i++) {
                a.t[i] = At<float>(anim, 0x34 + i * 4);
                b.t[i] = At<float>(anim, 0x34 + i * 4);
            }
            SlerpSample(&a, &b, out, t);
            return;
        }
        RotationOnly(a, b, out, t);
        return;
    }
    uint8_t* anim = codec_anim;
    uint8_t* track = At<uint8_t*>(anim, 0);
    uint8_t* data = At<uint8_t*>(track, 0x10);
    uint8_t* moves = data + At<uint32_t>(track, 4) * stride;
    if (rest == nullptr) {
        moves += At<uint16_t>(anim, 0x2A) * 8;
    }
    uint32_t index = uint32_t(k1 - data) / stride;
    uint32_t i = 0;
    while (Get<uint16_t>(moves, i * 8) != index) {
        i++;
    }
    const uint8_t* m = moves + i * 8;
    DecodeShorts3(m + 2, a.t);
    DecodeShorts3(m + 0xA, b.t);
    uint32_t flags = At<uint32_t>(anim, 0x18);
    if (!(flags & 0x200) && (flags & 0x100)) {
        for (int j = 0; j < 3; j++) {
            a.t[j] = At<float>(anim, 0x34 + j * 4) + a.t[j];
            b.t[j] = b.t[j] + At<float>(anim, 0x34 + j * 4);
        }
    }
    SlerpSample(&a, &b, out, t);
}

// 0x6BC5F0: 1001
void __cdecl Interp1001(uint8_t* out, const uint8_t* k1, const uint8_t* k2, float t) {
    InterpWithMoves<false>(out, k1, k2, t);
}
// 0x6BC930: 1002
void __cdecl Interp1002(uint8_t* out, const uint8_t* k1, const uint8_t* k2, float t) {
    InterpWithMoves<true>(out, k1, k2, t);
}

} // namespace

#define AMCODEC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMCODEC_REVERSED("AM_Keys999::Size", 0x006BB570, &KeysSize<0x20>);
AMCODEC_REVERSED("AM_Keys1000::Size", 0x006BB580, (&KeysAndMovesSize<0x14, 0x10>));
AMCODEC_REVERSED("AM_Keys1001::Size", 0x006BB5C0, (&KeysAndMovesSize<0xC, 8>));
AMCODEC_REVERSED("AM_Keys1002::Size", 0x006BB610, (&KeysAndMovesSize<8, 8>));
AMCODEC_REVERSED("AM_Keys1003::Size", 0x006BB650, &KeysSize<0x14>);
AMCODEC_REVERSED("AM_Keys1004::Size", 0x006BB660, &KeysSize<0xC>);
AMCODEC_REVERSED("AM::Stream_Write", 0x006BB670, &StreamWrite);
AMCODEC_REVERSED("AM_PackedKey::GetQuatXYZW", 0x006BB6E0, &PackedQuatXYZW);
AMCODEC_REVERSED("AM_Keys999::Read", 0x006BBAC0, &ReadKeys<0x20>);
AMCODEC_REVERSED("AM_Keys999::Write", 0x006BBAF0, &WriteKeys<0x20>);
AMCODEC_REVERSED("AM_Keys999::Interpolate", 0x006BBB20, &Interp999);
AMCODEC_REVERSED("AM_Keys1000::Read", 0x006BBC50, (&ReadKeysAndMoves<0x14, 0x10>));
AMCODEC_REVERSED("AM_Keys1000::Write", 0x006BBCC0, (&WriteKeysAndMoves<0x14, 0x10>));
AMCODEC_REVERSED("AM_Keys1001::Read", 0x006BBD30, (&ReadKeysAndMoves<0xC, 8>));
AMCODEC_REVERSED("AM_Keys1001::Write", 0x006BBDC0, (&WriteKeysAndMoves<0xC, 8>));
AMCODEC_REVERSED("AM_Keys1002::Read", 0x006BBE50, (&ReadKeysAndMoves<8, 8>));
AMCODEC_REVERSED("AM_Keys1002::Write", 0x006BBED0, (&WriteKeysAndMoves<8, 8>));
AMCODEC_REVERSED("AM_Keys1003::Read", 0x006BBF50, &ReadKeys<0x14>);
AMCODEC_REVERSED("AM_Keys1003::Write", 0x006BBF80, &WriteKeys<0x14>);
AMCODEC_REVERSED("AM_Keys1003::Interpolate", 0x006BBFB0, &Interp1003);
AMCODEC_REVERSED("AM_Keys1004::Read", 0x006BC1E0, &ReadKeys<0xC>);
AMCODEC_REVERSED("AM_Keys1004::Write", 0x006BC210, &WriteKeys<0xC>);
AMCODEC_REVERSED("AM_Keys1004::Interpolate", 0x006BC240, &Interp1004);
AMCODEC_REVERSED("AM_Keys1000::Interpolate", 0x006BC3C0, &Interp1000);
AMCODEC_REVERSED("AM_Keys1001::Interpolate", 0x006BC5F0, &Interp1001);
AMCODEC_REVERSED("AM_Keys1002::Interpolate", 0x006BC930, &Interp1002);
