// HXD files (see AnimHxdFile.cpp): releasing an anim group (its AGR streamed out when
// nobody uses it), timing a group, and a joint's translation in an anim at a key (the
// root joints' tracks carry it in the keys, the others take the skeleton's).
// 0x6B3DA0..0x6B4147, 0x6B56B0.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

constexpr float kMilli = 0.0010000000474974513f;
constexpr float kTime16 = 1.5259021893143654e-05f;

GAME_FN(int, __cdecl, FindAgrId, 0x0052DAD0, const char* name);
GAME_FN(void, __cdecl, RemoveAgr, 0x00530880, int id);
GAME_FN(float, __thiscall, PackedKey_GetTime, 0x006B1840, const uint8_t* key);
GAME_FN(void, __thiscall, ShortKey_GetTranslation, 0x006B1990, const uint8_t* key, float* out);
GAME_FN(void, __thiscall, PackedKey_GetTranslation, 0x006B19D0, const uint8_t* key, float* out);
GAME_FN(int, __thiscall, Hxd_ReleaseGroup, 0x006B3DA0, uint8_t* hxd, uint32_t group, int keep);

inline int AnimKind(uint32_t flags) {
    return (flags & 0x200) ? 2 : 1;
}

// 0x6B3DA0: one user fewer of a group (0x1E: had none and isn't kept); the group's AGR
// is removed when that was the last and it isn't kept (unless `keep`)
int __fastcall ReleaseGroup(uint8_t* self, re::Edx, uint32_t group, int keep) {
    if (group >= At<uint32_t>(self, 0x68)) {
        return 2;
    }
    uint8_t* g = At<uint8_t*>(self, 0x6C) + group * 0x3C;
    uint16_t users = At<uint16_t>(g, 0x2C);
    if (users == 0) {
        if (!(At<uint8_t>(g, 0x24) & 2)) {
            return 0x1E;
        }
    } else {
        At<uint16_t>(g, 0x2C) = uint16_t(users - 1);
    }
    if (At<uint16_t>(g, 0x2C) == 0 && !(At<uint8_t>(g, 0x24) & 2) && keep == 0) {
        int id = FindAgrId(reinterpret_cast<const char*>(g + 4));
        if (id != -1) {
            RemoveAgr(id);
        }
    }
    return 0;
}

// 0x6B56B0: a group's +0 set to `seconds` in milliseconds (releasing it first when set)
void __fastcall SetGroupTime(uint8_t* self, re::Edx, uint32_t group, int seconds) {
    if (group >= At<uint32_t>(self, 0x68)) {
        return;
    }
    if (At<int32_t>(At<uint8_t*>(self, 0x6C) + group * 0x3C, 0) != 0) {
        Hxd_ReleaseGroup(self, group, 0);
    }
    At<int32_t>(At<uint8_t*>(self, 0x6C) + group * 0x3C, 0) = int(float(seconds) * 1000.0f);
}

inline void Copy3(float* out, const uint8_t* from) {
    out[0] = At<float>(const_cast<uint8_t*>(from), 0);
    out[1] = At<float>(const_cast<uint8_t*>(from), 4);
    out[2] = At<float>(const_cast<uint8_t*>(from), 8);
}
inline void Shorts3(float* out, const uint8_t* from) {
    out[0] = float(At<int16_t>(const_cast<uint8_t*>(from), 0)) * kMilli;
    out[1] = float(At<int16_t>(const_cast<uint8_t*>(from), 2)) * kMilli;
    out[2] = kMilli * float(At<int16_t>(const_cast<uint8_t*>(from), 4));
}

// 0x6B3E20: a joint's translation in anim `animIndex` at key `key` (< 0: the last);
// returns the key's time when the joint is the extra root (flag 0x40), else 0. Flag 0x100
// adds the anim's offset (+0x34) to the root's
float __fastcall GetTranslation(uint8_t* self, re::Edx, int animIndex, int joint, float* out, int key) {
    float time = 0.0f;
    uint8_t* anim = At<uint8_t*>(self, 0) + animIndex * 0x40;
    uint32_t flags = At<uint32_t>(anim, 0x18);
    uint8_t* track = At<uint8_t*>(anim, 0);
    if (!(flags & 8)) {
        int k;
        if (key < 0) {
            k = joint == AnimKind(flags) ? At<uint16_t>(anim, 0x2E) : At<uint16_t>(anim, 0x30);
        } else {
            k = key + joint;
        }
        uint8_t* keys = At<uint8_t*>(track, 0x10);
        if (flags & 2) {
            PackedKey_GetTranslation(keys + k * 0xC, out);
        } else if (flags & 1) {
            ShortKey_GetTranslation(keys + k * 0x14, out);
        } else {
            Copy3(out, keys + k * 0x20 + 0x14);
        }
    } else {
        uint32_t isShort = flags & 1;
        int stride = isShort ? 0xC : ((flags & 2) ? 8 : 0x14);
        uint8_t* keys = At<uint8_t*>(track, 0x10);
        int rotationBytes = At<int32_t>(track, 4) * stride;  // (the translations follow the rotations)
        if (joint == AnimKind(flags)) {
            if (!(flags & 2) && !isShort) {
                const uint8_t* p = key < 0 ? keys + rotationBytes + At<uint16_t>(anim, 0x2A) * 0x10 - 0xC
                                           : keys + rotationBytes + key * 0x10 + 4;
                Copy3(out, p);
            } else {
                const uint8_t* p = key < 0 ? keys + rotationBytes + At<uint16_t>(anim, 0x2A) * 8 - 8
                                           : keys + rotationBytes + key * 8;
                Shorts3(out, p + 2);
            }
        } else if (joint == int(At<uint32_t>(self, 0x5C)) - 1 && (flags & 0x40)) {
            uint32_t first = At<uint16_t>(anim, 0x2A);
            if (!(flags & 2) && !isShort) {
                const uint8_t* p = key < 0 ? keys + rotationBytes + (At<uint16_t>(anim, 0x2C) + first) * 0x10 - 0x10
                                           : keys + rotationBytes + (first + key) * 0x10;
                int rotationKey = At<int32_t>(const_cast<uint8_t*>(p), 0);
                time = float(At<uint16_t>(keys, rotationKey * 0x20 + 2)) * kTime16;
                Copy3(out, p + 4);
            } else {
                const uint8_t* p = key < 0 ? keys + rotationBytes + (At<uint16_t>(anim, 0x2C) + first) * 8 - 8
                                           : keys + rotationBytes + (first + key) * 8;
                uint32_t rotationKey = At<uint16_t>(const_cast<uint8_t*>(p), 0);
                if (flags & 2) {
                    time = PackedKey_GetTime(keys + rotationKey * 8);
                } else {
                    time = float(At<uint16_t>(keys, rotationKey * 0xC + 2)) * kTime16;
                }
                Shorts3(out, p + 2);
            }
        } else {
            Copy3(out, At<uint8_t*>(self, 0x70) + joint * 0xC);
        }
    }
    uint32_t f = At<uint32_t>(anim, 0x18);
    if ((f & 0x100) && joint == AnimKind(f)) {
        out[0] = out[0] + At<float>(anim, 0x34);
        out[1] = At<float>(anim, 0x38) + out[1];
        out[2] = At<float>(anim, 0x3C) + out[2];
    }
    return time;
}

} // namespace

#define AMHXDQ_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMHXDQ_REVERSED("HxdFile::ReleaseGroup", 0x006B3DA0, &ReleaseGroup);
AMHXDQ_REVERSED("HxdFile::GetTranslation", 0x006B3E20, &GetTranslation);
AMHXDQ_REVERSED("HxdFile::SetGroupTime", 0x006B56B0, &SetGroupTime);
