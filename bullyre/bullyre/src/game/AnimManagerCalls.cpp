// The AM_ anim manager's (0x20C4910) small calls: a model's track slots (setting, playing,
// stopping), an HXD's anim groups (use counts, keeping, unloading), the file names by kind,
// gathering the tracks' root motion, and copying joint records. The manager's own pointer
// is unused by them. 0x6BCC30..0x6BD5C3.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

constexpr uintptr_t kDebugBreak = 0x008FC05C;
GAME_VAR(int32_t, mgr_breakEnabled, 0x00BB6458);
GAME_VAR(int32_t, mgr_breakArmed, 0x020C48FC);
GAME_VAR(uint8_t*, mgr_breakModel, 0x020C48F8);

GAME_FN(void, __thiscall, Model_AddToSlot, 0x006B9290, uint8_t* model, int slot, void* object);
GAME_FN(void, __thiscall, Track_PlayAnim, 0x006B66D0, uint8_t* track, uint8_t* hxd, uint32_t animIndex, int flags,
        float blend, int player, float rate, float fade);
GAME_FN(int, __thiscall, Hxd_AddGroupRef, 0x006B1C60, uint8_t* hxd, uint32_t group);
GAME_FN(int, __thiscall, Hxd_ReleaseGroup, 0x006B3DA0, uint8_t* hxd, uint32_t group, int keep);
GAME_FN(int, __thiscall, Hxd_UnloadGroup, 0x006B1C90, uint8_t* hxd, uint32_t group);
inline char* __cdecl StrUpr(char* s) { return crt::Strupr(s); }   // _strupr
GAME_FN(void, __thiscall, Model_EmptyList, 0x006B5AB0, uint8_t* model, int list);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, Rw_ForAllAtomics, 0x006C7380, void* clump, void* callback, void* data);
inline void* const kSetSphereCallback = reinterpret_cast<void*>(0x006BD270);

// 0x6BCC30: puts `track` in its slot (+0x44) on the model (the slot's old track released)
// and plays anim `animIndex` of the model's HXD on it
int __fastcall Mgr_PlayOnTrack(void*, re::Edx, uint8_t* model, uint32_t animIndex, uint8_t* track, int flags, float blend,
                               int player, float rate, float fade) {
    int32_t slot = At<int32_t>(track, 0x44);
    uint8_t* old = At<uint8_t*>(model, 0x18 + slot * 4);
    uint8_t* hxd = At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10);
    if (old != track) {
        if (old == nullptr) {
            At<int32_t>(model, 0x88) += 1;
        } else {
            if (At<void*>(old, 0x18) != nullptr && ((At<uint32_t>(old, 0x28) >> 2) & 1)) {
                Model_AddToSlot(model, slot, nullptr);
            }
            vc::Call<void>(old, 4);
            At<uint8_t*>(model, 0x18 + slot * 4) = nullptr;
        }
        At<uint8_t*>(model, 0x18 + slot * 4) = track;
    }
    Track_PlayAnim(track, hxd, animIndex, flags, blend, player, rate, fade);
    return 0;
}

// 0x6BCCD0: puts `track` in slot `slot` (the old one released)
int __fastcall Mgr_SetSlot(void*, re::Edx, uint8_t* model, uint8_t* track, int slot) {
    Model_AddToSlot(model, slot, nullptr);
    uint8_t* old = At<uint8_t*>(model, 0x18 + slot * 4);
    if (old) {
        vc::Call<void>(old, 4);
        At<uint8_t*>(model, 0x18 + slot * 4) = nullptr;
    } else {
        At<int32_t>(model, 0x88) += 1;
    }
    At<int32_t>(track, 0x44) = slot;
    At<uint8_t*>(model, 0x18 + slot * 4) = track;
    return 0;
}

// 0x6BCD30: copies the joint records (0x24 bytes at +0x4C, stride +0x24) of the smaller
// count (+0x2C) from `from` into `to`
void __cdecl CopyJointRecords(uint8_t* to, uint8_t* from) {
    float a = float(At<int32_t>(to, 0x2C));
    float b = float(At<int32_t>(from, 0x2C));
    float fewer = (b < a || b == a) ? b : a;
    uint32_t count = uint32_t(int64_t(fewer));
    for (uint32_t i = 0; i < count; i++) {
        std::memcpy(to + At<int32_t>(to, 0x24) * i + 0x4C, from + At<int32_t>(from, 0x24) * i + 0x4C, 0x24);
    }
}

// 0x6BD010: one more user of an HXD's group (0x16: no HXD)
int __fastcall Mgr_AddGroupRef(void*, re::Edx, uint8_t* hxd, uint32_t group) {
    if (hxd == nullptr) {
        return 0x16;
    }
    return Hxd_AddGroupRef(hxd, group);
}

// 0x6BD030
int __fastcall Mgr_ReleaseGroup(void*, re::Edx, uint8_t* hxd, uint32_t group, int keep) {
    if (hxd == nullptr) {
        return 0x16;
    }
    return Hxd_ReleaseGroup(hxd, group, keep);
}

// 0x6BD060: keeps a group loaded (flag 2) or not
int __fastcall Mgr_KeepGroup(void*, re::Edx, uint8_t* hxd, uint32_t group, int keep) {
    if (hxd == nullptr) {
        return 0x16;
    }
    if (group >= At<uint32_t>(hxd, 0x68)) {
        return 2;
    }
    uint16_t& flags = At<uint16_t>(At<uint8_t*>(hxd, 0x6C) + group * 0x3C, 0x24);
    if (keep != 0) {
        flags |= 2;
    } else {
        flags &= 0xFFFD;
    }
    return 0;
}

// 0x6BD0B0: CAnimManager::RemoveAgr (unloads a group's anims)
int __fastcall Mgr_RemoveAgr(void*, re::Edx, uint8_t* hxd, uint32_t group) {
    if (hxd == nullptr) {
        return 0x16;
    }
    return Hxd_UnloadGroup(hxd, group);
}

// 0x6BD0D0: a file's base name by kind (0 .MXD, 2 .HXD, 4 .ANM, 5 .DFF; upper case, the
// kind's extension taken off; 0x18: wrong kind or extension, 0x19: too long)
int __fastcall Mgr_BaseName(void*, re::Edx, const char* name, char* out, uint32_t kind) {
    char buffer[0x80];
    std::strcpy(buffer, name);
    StrUpr(buffer);
    size_t length = std::strlen(buffer);
    char extension[5];
    uint32_t limit;
    switch (kind) {
    case 0:
        std::memcpy(extension, ".MXD", 5);
        limit = 0x40;
        break;
    case 2:
        std::memcpy(extension, ".HXD", 5);
        limit = 0x40;
        break;
    case 4:
        std::memcpy(extension, ".ANM", 5);
        limit = 0x20;
        break;
    case 5:
        std::memcpy(extension, ".DFF", 5);
        limit = 0x40;
        break;
    default:
        return 0x18;
    }
    char* dot = buffer + length - 4;
    if (*dot == '.') {
        if (std::strcmp(dot, extension) != 0) {
            return 0x18;
        }
        *dot = 0;
    }
    if (std::strlen(buffer) >= limit - 1) {
        return 0x19;
    }
    std::strcpy(out, buffer);
    return 0;
}

// 0x6BD2B0: the model's root motion from its tracks (slot 0's, the others blended in by
// their weight sets' first weight), handed to +0xA4 when dt > 0
uint8_t __fastcall Mgr_GatherRootMotion(void*, re::Edx, uint8_t* model, float dt) {
    CVector total{0.0f, 0.0f, 0.0f};
    uint8_t* hxd = At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10);
    float rotation[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    if (mgr_breakEnabled != 0 && mgr_breakArmed != 0 && mgr_breakModel == model) {
        vc::Import<void>(kDebugBreak);
    }
    for (uint32_t slot = 0; slot < 7; slot++) {
        uint8_t* track = At<uint8_t*>(model, 0x18 + slot * 4);
        CVector motion{0.0f, 0.0f, 0.0f};
        if (track == nullptr) {
            continue;
        }
        vc::Call<uint8_t>(track, 0x14, model, &motion, rotation);
        Model_EmptyList(model, int(slot));
        if (slot == 0) {
            total = motion;
            continue;
        }
        uint8_t* set = At<uint8_t*>(track, 8);
        if (set == nullptr) {
            if (At<uint32_t>(hxd, 0xC) == 0) {
                continue;
            }
            set = At<uint8_t*>(hxd, 8);
            if (set == nullptr) {
                continue;
            }
        }
        float weight = At<uint32_t>(set, 0x24) != 0 ? *At<float*>(set, 0x20) : -1.0f;
        float rest = 1.0f - weight;
        CVector kept{total.x * rest, total.y * rest, rest * total.z};
        CVector added{motion.x * weight, motion.y * weight, weight * motion.z};
        CVector sum;
        total = *VectorAdd(&sum, &added, &kept);
    }
    if (0.0f < dt) {
        CVector* out = At<CVector*>(model, 0xA4);
        if (out) {
            *out = total;
        }
    }
    At<uint32_t>(model, 0x98) |= 0x2000;
    return 1;
}

// 0x6BD460: every atomic of the model takes the bounding sphere
void __fastcall Mgr_SetSpheres(void*, re::Edx, uint8_t* model, float* sphere) {
    Rw_ForAllAtomics(At<void*>(model, 0x10), kSetSphereCallback, &sphere);
}

// 0x6BD480: stops a model's track (fading out over `fade`: -1 its weight set's time, the
// first slot at once); stopped at once it's reset, released and its slot emptied
int __fastcall Mgr_StopTrack(void*, re::Edx, uint8_t* model, uint8_t* track, float fade) {
    if (model == nullptr || track == nullptr) {
        return 0x16;
    }
    uint32_t slot = 0;
    while (At<uint8_t*>(model, 0x18 + slot * 4) != track) {
        slot++;
        if (slot >= 7) {
            return 0;
        }
    }
    if (slot == 0) {
        fade = 0.0f;
    } else if (fade == -1.0f && At<void*>(track, 8) != nullptr) {
        fade = At<float>(At<uint8_t*>(track, 8), 0x28);
    }
    if (!(std::fabs(fade) < 0.001f)) {
        At<float>(track, 0x10) = fade;
        At<float>(track, 0x14) = fade;
        return 0;
    }
    At<float>(track, 0xC) = 1.0f;
    At<float>(track, 0x10) = 0.0f;
    At<float>(track, 0x14) = 0.0f;
    if (slot > 0) {
        Model_AddToSlot(model, int(slot), nullptr);
    }
    At<void*>(track, 8) = nullptr;
    for (int base = 0x18; base <= 0x2C; base += 0x14) {
        uint8_t* anim = At<uint8_t*>(track, base);
        if (anim && At<void*>(anim, 8)) {
            Hxd_ReleaseGroup(At<uint8_t*>(anim, 8), At<uint32_t>(anim, 0x20), 0);
        }
        At<float>(track, base + 4) = 0.0f;
        At<void*>(track, base) = nullptr;
        At<float>(track, base + 8) = 0.0f;
        At<uint32_t>(track, base + 0x10) = 0;
        At<float>(track, base + 0xC) = 1.0f;
    }
    At<float>(track, 0x40) = 0.0f;
    At<float>(track, 0x14) = 0.0f;
    At<float>(track, 0x10) = 0.0f;
    vc::Call<void>(track, 4);
    At<void*>(model, 0x18 + slot * 4) = nullptr;
    At<int32_t>(model, 0x88) -= 1;
    return 0;
}

} // namespace

#define AMMGR_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMMGR_REVERSED("AM_Manager::PlayOnTrack", 0x006BCC30, &Mgr_PlayOnTrack);
AMMGR_REVERSED("AM_Manager::SetSlot", 0x006BCCD0, &Mgr_SetSlot);
AMMGR_REVERSED("AM_Manager::CopyJointRecords", 0x006BCD30, &CopyJointRecords);
AMMGR_REVERSED("AM_Manager::AddGroupRef", 0x006BD010, &Mgr_AddGroupRef);
AMMGR_REVERSED("AM_Manager::ReleaseGroup", 0x006BD030, &Mgr_ReleaseGroup);
AMMGR_REVERSED("AM_Manager::KeepGroup", 0x006BD060, &Mgr_KeepGroup);
AMMGR_REVERSED("CAnimManager::RemoveAgr_6BD0B0", 0x006BD0B0, &Mgr_RemoveAgr);
AMMGR_REVERSED("AM_Manager::BaseName", 0x006BD0D0, &Mgr_BaseName);
AMMGR_REVERSED("AM_Manager::GatherRootMotion", 0x006BD2B0, &Mgr_GatherRootMotion);
AMMGR_REVERSED("AM_Manager::SetSpheres", 0x006BD460, &Mgr_SetSpheres);
AMMGR_REVERSED("AM_Manager::StopTrack", 0x006BD480, &Mgr_StopTrack);
