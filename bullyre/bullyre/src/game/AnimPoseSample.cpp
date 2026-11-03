// The AM_ pose sampler (the manager at 0x20C4910; its own pointer unused here): a pose
// (+0 the track, +4 the time sampled, +0x24 the record stride, +0x2C the joint count,
// +0x3C..0x48 the key kind's codec, +0x40 its blend, +0x44 its interpolation; records of
// 0x24 bytes at +0x4C: the two keys around the time, then w, x, y, z and a translation)
// sampled from an anim at a time, the key pairs around a time found per joint, and two
// poses blended. 0x6BCDC0..0x6BD006, 0x6BD5D0..0x6BE3A7.

#include <cmath>
#include <cstdint>
#include <cstring>

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

constexpr float kShort = 3.0518509447574615e-05f;
constexpr float kMilli = 0.0010000000474974513f;

GAME_VAR(uint8_t*, ps_anim, 0x020C4904);
GAME_VAR(int32_t, ps_joint, 0x020C4908);
GAME_VAR(uint8_t*, ps_restTranslations, 0x020C4900);

GAME_FN(void, __cdecl, CopyJointRecords, 0x006BCD30, uint8_t* to, uint8_t* from);
GAME_FN(void, __cdecl, SeekPacked, 0x006BD5D0, uint8_t* pose, uint32_t stride, uint32_t time);
GAME_FN(void, __cdecl, Seek16, 0x006BD7C0, uint8_t* pose, uint32_t stride, uint32_t time);
GAME_FN(int, __cdecl, Anim_GetKind, 0x006B1A70, uint8_t* anim);
GAME_FN(void, __thiscall, FloatKey_GetQuat, 0x006B1820, const uint8_t* key, float* out);
GAME_FN(void, __thiscall, PackedKey_GetQuat, 0x006B1870, const uint8_t* key, float* out);
GAME_FN(void, __thiscall, ShortKey_GetQuat, 0x006B1940, const uint8_t* key, float* out);
GAME_FN(void, __thiscall, ShortKey_GetTranslation, 0x006B1990, const uint8_t* key, float* out);
GAME_FN(void, __thiscall, PackedKey_GetTranslation, 0x006B19D0, const uint8_t* key, float* out);
GAME_FN(void, __thiscall, Vector_Add, 0x00415200, float* v, const float* add);
inline void* __cdecl Memcpy(void* dst, const void* src, size_t n) { return crt::Memcpy(dst, src, n); }

inline uint8_t* Record(uint8_t* pose, uint32_t i) {
    return pose + 0x4C + At<int32_t>(pose, 0x24) * i;
}

using Blend = void(__cdecl*)(uint8_t* out, uint8_t* a, uint8_t* b, float t);

// 0x6BCDC0: out = a blended towards b by t (per joint by a weight set's weights times t)
void __fastcall Pose_Blend(void*, re::Edx, uint8_t* out, uint8_t* a, uint8_t* b, float t, uint8_t* weights) {
    uint32_t count = At<uint32_t>(out, 0x2C);
    if (weights == nullptr) {
        if (t == 1.0f) {
            CopyJointRecords(out, b);
            return;
        }
        if (std::fabs(t) < 0.001f) {
            CopyJointRecords(out, a);
            return;
        }
        for (uint32_t i = 0; i < count; i++) {
            At<Blend>(out, 0x40)(Record(out, i), Record(a, i), Record(b, i), t);
        }
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        float w = i < At<uint32_t>(weights, 0x24) ? At<float*>(weights, 0x20)[i] : -1.0f;
        if (!(t == 1.0f)) {
            w = w * t;
        }
        if (w == 1.0f) {
            std::memcpy(Record(out, i), Record(b, i), 0x24);
        } else if (std::fabs(w) < 0.001f) {
            std::memcpy(Record(out, i), Record(a, i), 0x24);
        }
        if (std::fabs(w) >= 0.001f) {
            At<Blend>(out, 0x40)(Record(out, i), Record(a, i), Record(b, i), w);
        }
    }
}

// 0x6BD5D0 / 0x6BD7C0: each joint's key pair around `time` (the first `count` keys are
// the joints' first keys, the next `count` their second; later keys chain back by index to
// their joint's first). Packed: bits 0..10 the previous key, the time bits 11..19 compared
// as (key >> 4) & 0xFF80; 16-bit: the previous key and the time as shorts
template <bool Packed>
void __cdecl SeekKeys(uint8_t* pose, uint32_t stride, uint32_t time) {
    uint8_t* track = At<uint8_t*>(pose, 0);
    uint32_t recordStride = At<uint32_t>(At<uint8_t*>(track, 0), 4);
    uint8_t* data = At<uint8_t*>(track, 0x10);
    uint32_t count = At<uint32_t>(pose, 0x2C);
    auto previous = [&](const uint8_t* k) -> uint32_t {
        return Packed ? (Get<uint32_t>(k, 0) & 0x7FF) : Get<uint16_t>(k, 0);
    };
    auto keyTime = [&](const uint8_t* k) -> uint32_t {
        return Packed ? ((Get<uint32_t>(k, 0) >> 4) & 0xFF80) : Get<uint16_t>(k, 2);
    };
    auto setBit = [](uint32_t* bits, uint32_t i) {
        if (int32_t(i) >= 32) {
            bits += i >> 5;
            i -= (i >> 5) << 5;
        }
        *bits |= 1u << i;
    };
    uint32_t found[2] = {0, 0};
    uint32_t all[2] = {0, 0};
    if (count == 0) {
        return;
    }
    uint8_t* record = pose + 0x4C;
    uint8_t* first = data;
    uint8_t* second = data + count * stride;
    for (uint32_t i = 0; i < count; i++) {
        if (keyTime(second) >= time) {
            At<uint8_t*>(record, 4) = second;
            At<uint8_t*>(record, 0) = first;
            setBit(found, i);
        }
        record += recordStride;
        second += stride;
        first += stride;
        setBit(all, i);
    }
    if (found[0] == all[0] && found[1] == all[1]) {
        return;
    }
    uint8_t* k = At<uint8_t*>(track, 0x10) + count * stride * 2;
    while (keyTime(k) < time) {
        k += stride;
    }
    while (found[0] != all[0] || found[1] != all[1]) {
        uint8_t* keys = At<uint8_t*>(track, 0x10);
        uint8_t* before = keys + previous(k) * stride;
        if (keyTime(k) >= time && keyTime(before) < time) {
            uint8_t* chain = before;
            while (previous(chain) >= count) {
                chain = keys + previous(chain) * stride;
            }
            uint32_t joint = previous(chain);
            uint8_t* r = pose + 0x4C + joint * At<uint32_t>(pose, 0x24);
            At<uint8_t*>(r, 0) = before;
            At<uint8_t*>(r, 4) = k;
            setBit(found, previous(chain));
        }
        k += stride;
    }
}

inline void RestTranslation(uint8_t* record, uint32_t i) {
    const uint32_t* rest = reinterpret_cast<const uint32_t*>(ps_restTranslations + i * 0xC);
    At<uint32_t>(record, 0x18) = rest[0];
    At<uint32_t>(record, 0x1C) = rest[1];
    At<uint32_t>(record, 0x20) = rest[2];
}
inline void Shorts3(float* out, const uint8_t* p) {
    out[0] = float(Get<int16_t>(p, 0)) * kMilli;
    out[1] = float(Get<int16_t>(p, 2)) * kMilli;
    out[2] = kMilli * float(Get<int16_t>(p, 4));
}

// the root's translation from the first translation key (after the keys), the anim's
// offset added (flag 0x100); the extra root's from its own first one
void FirstMoves(uint8_t* pose, uint8_t* anim, uint8_t* track, uint32_t stride, uint8_t* hxd) {
    float* root = reinterpret_cast<float*>(Record(pose, uint32_t(Anim_GetKind(anim))) + 0x18);
    const uint8_t* move = At<uint8_t*>(track, 0x10) + At<uint32_t>(track, 4) * stride;
    root[0] = float(Get<int16_t>(move, 2)) * kMilli;
    root[1] = float(Get<int16_t>(move, 4)) * kMilli;
    root[2] = float(Get<int16_t>(move, 6)) * kMilli;
    if (At<uint32_t>(anim, 0x18) & 0x100) {
        root[0] = root[0] + At<float>(anim, 0x34);
        root[1] = At<float>(anim, 0x38) + root[1];
        root[2] = At<float>(anim, 0x3C) + root[2];
    }
    if (At<uint8_t>(anim, 0x18) & 0x40) {
        float* extra = reinterpret_cast<float*>(Record(pose, At<uint32_t>(hxd, 0x5C) - 1) + 0x18);
        const uint8_t* m = At<uint8_t*>(track, 0x10) + At<uint32_t>(track, 4) * stride + At<uint16_t>(anim, 0x2A) * 8;
        Shorts3(extra, m + 2);
    }
}

// 0x6BD970: samples `anim` at `time` (seconds, clamped to its length) into `pose` for the
// model's hierarchy; at time 0 straight from the first keys
void __fastcall Pose_SampleAll(void*, re::Edx, uint8_t* pose, uint8_t* anim, float time, uint8_t* hierarchy) {
    uint8_t* hxd = At<uint8_t*>(hierarchy, 0x10);
    if (At<float>(anim, 0xC) < time) {
        time = At<float>(anim, 0xC);
    }
    At<uint32_t>(pose, 0x2C) = At<uint32_t>(hxd, 0x5C);
    At<float>(pose, 4) = 0.0f;
    At<uint8_t*>(pose, 0) = At<uint8_t*>(anim, 0);
    if (At<uint8_t*>(anim, 0) == nullptr) {
        return;
    }
    uint8_t* header = At<uint8_t*>(At<uint8_t*>(anim, 0), 0);   // (anim[0]: its keys; [0]: the header)
    At<uint32_t>(pose, 0x24) = At<uint32_t>(header, 4);
    for (int i = 0; i < 4; i++) {
        At<uint32_t>(pose, 0x3C + i * 4) = At<uint32_t>(header, 0xC + i * 4);
    }
    uint8_t* rest = At<uint8_t*>(hierarchy, 0xA8);
    if (rest == nullptr || At<uint32_t>(hierarchy, 0xA4) != At<uint32_t>(hxd, 0x5C)) {
        rest = At<uint8_t*>(hxd, 0x70);
    }
    ps_restTranslations = rest;
    int32_t kind = At<int32_t>(header, 0);
    uint32_t count = At<uint32_t>(pose, 0x2C);
    uint8_t* track = At<uint8_t*>(pose, 0);
    uint32_t stride;
    switch (kind) {
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
        stride = At<uint32_t>(header, 4);
        break;
    }
    if (!(std::fabs(time) < 0.001f)) {
        float t = time / At<float>(anim, 0xC);
        At<float>(pose, 4) = t;
        ps_anim = anim;
        if (At<uint8_t>(anim, 0x18) & 2) {
            int64_t units = int64_t(float(double(t) * 511.0));
            SeekPacked(pose, stride, uint32_t(units) << 7);
        } else {
            float magnitude = std::fabs(t);
            int units = int(float(double(magnitude) * 65535.0));
            Seek16(pose, stride, uint16_t(units));
        }
        uint32_t recordStride = At<uint32_t>(header, 4);
        uint32_t flags = At<uint32_t>(anim, 0x18);
        uint32_t joints = count;
        if ((flags & 0x40) && (flags & 8)) {
            joints--;
        }
        using Interp = void(__cdecl*)(uint8_t*, uint8_t*, uint8_t*, float, uint32_t);
        uint8_t* r = pose + 0x4C;
        for (uint32_t j = 0; j < joints; j++) {
            ps_joint = int32_t(j);
            At<Interp>(pose, 0x44)(r, At<uint8_t*>(r, 0), At<uint8_t*>(r, 4), At<float>(pose, 4),
                                   At<uint32_t>(At<uint8_t*>(pose, 0), 0x14));
            r += recordStride;
        }
        flags = At<uint32_t>(anim, 0x18);
        if ((flags & 0x40) && (flags & 8)) {
            ps_restTranslations = nullptr;
            At<Interp>(pose, 0x44)(r, At<uint8_t*>(r, 0), At<uint8_t*>(r, 4), At<float>(pose, 4),
                                   At<uint32_t>(At<uint8_t*>(pose, 0), 0x14));
        }
        return;
    }
    uint8_t* data = At<uint8_t*>(track, 0x10);
    switch (kind) {
    case 1001:
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* r = Record(pose, i);
            const uint8_t* k = data + i * stride;
            At<float>(r, 0xC) = float(Get<int16_t>(k, 4)) * kShort;
            At<float>(r, 0x10) = float(Get<int16_t>(k, 6)) * kShort;
            At<float>(r, 0x14) = float(Get<int16_t>(k, 8)) * kShort;
            At<float>(r, 8) = float(Get<int16_t>(k, 0xA)) * kShort;
            RestTranslation(r, i);
        }
        FirstMoves(pose, anim, track, stride, hxd);
        return;
    case 1002:
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* r = Record(pose, i);
            PackedKey_GetQuat(At<uint8_t*>(track, 0x10) + i * stride, reinterpret_cast<float*>(r + 8));
            RestTranslation(r, i);
        }
        FirstMoves(pose, anim, track, stride, hxd);
        return;
    case 1000: {
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* r = Record(pose, i);
            const uint8_t* k = At<uint8_t*>(track, 0x10) + i * stride;
            for (int w = 0; w < 4; w++) {
                At<uint32_t>(r, 8 + w * 4) = Get<uint32_t>(k, 4 + w * 4);
            }
            RestTranslation(r, i);
        }
        const uint8_t* move = At<uint8_t*>(track, 0x10) + At<uint32_t>(track, 4) * stride + 4;
        uint8_t* root = Record(pose, uint32_t(Anim_GetKind(anim))) + 0x18;
        std::memcpy(root, move, 0xC);
        if (At<uint8_t>(anim, 0x18) & 0x40) {
            const uint8_t* m = At<uint8_t*>(track, 0x10) + At<uint32_t>(track, 4) * stride +
                               At<uint16_t>(anim, 0x2A) * 0x10 + 4;
            std::memcpy(Record(pose, At<uint32_t>(hxd, 0x5C) - 1) + 0x18, m, 0xC);
        }
        return;
    }
    case 1003:
    case 1004:
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* r = Record(pose, i);
            const uint8_t* k = At<uint8_t*>(track, 0x10) + i * stride;
            if (kind == 1003) {
                ShortKey_GetQuat(k, reinterpret_cast<float*>(r + 8));
                ShortKey_GetTranslation(k, reinterpret_cast<float*>(r + 0x18));
            } else {
                PackedKey_GetQuat(k, reinterpret_cast<float*>(r + 8));
                PackedKey_GetTranslation(k, reinterpret_cast<float*>(r + 0x18));
            }
        }
        if (At<uint32_t>(anim, 0x18) & 0x100) {
            float* root = reinterpret_cast<float*>(Record(pose, uint32_t(Anim_GetKind(anim))) + 0x18);
            Vector_Add(root, reinterpret_cast<float*>(anim + 0x34));
        }
        return;
    case 999:
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* r = Record(pose, i);
            const uint8_t* k = At<uint8_t*>(track, 0x10) + i * stride;
            FloatKey_GetQuat(k, reinterpret_cast<float*>(r + 8));
            At<uint32_t>(r, 0x18) = Get<uint32_t>(k, 0x14);
            At<uint32_t>(r, 0x1C) = Get<uint32_t>(k, 0x18);
            At<uint32_t>(r, 0x20) = Get<uint32_t>(k, 0x1C);
        }
        return;
    default:
        Memcpy(pose + 0x4C, At<void*>(track, 0x10), stride * count);
        return;
    }
}

} // namespace

#define AMPOSE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMPOSE_REVERSED("AM_Pose::Blend", 0x006BCDC0, &Pose_Blend);
AMPOSE_REVERSED("AM_Pose::SeekPackedKeys", 0x006BD5D0, &SeekKeys<true>);
AMPOSE_REVERSED("AM_Pose::Seek16BitKeys", 0x006BD7C0, &SeekKeys<false>);
AMPOSE_REVERSED("AM_Pose::Sample", 0x006BD970, &Pose_SampleAll);
