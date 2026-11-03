// The AM_ manager's track calls on a model (0x20C4910's methods; most don't use it): a
// slot's mask track set (slot 0 always the skeleton's first weight set, no fade), a slot's
// track found or made, a track's players paused / resumed / given a rate / a time, a
// player's time read, a track started through 0x6BCC30, blend nodes added to and taken off
// a model (the manager's pool at +0xE8, the model's list at +0xD8), and the used
// hierarchy objects and HXDs found by their name's hash. Results are error codes (0 fine).
// 0x6C0B20..0x6C1180, 0x6C17A0..0x6C1838.
//
// A track (AM_AnimTrack, 0x48): +8 its mask, +0xC its weight, +0x10 / +0x14 its fade,
// +0x18 two players (0x14 each: +0 the anim, +4 the time, +8 last frame's, +0xC the rate,
// +0x10 flags, 1 paused), +0x3C / +0x40, +0x44 its slot.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

GAME_FN(uint8_t*, __thiscall, AmApi_WeightSet, 0x00607480, void* hxd, uint32_t index);
GAME_FN(uint8_t*, __cdecl, AmApi_NewTrack, 0x006B8FB0);
GAME_FN(int, __stdcall, AmApi_FindTrack, 0x006C0CD0, uint8_t* model, re::Ptr32<uint8_t>* track, int player);
GAME_FN(int, __thiscall, AmApi_StartTrack, 0x006BCC30, void* manager, uint8_t* model, void* anim,
        uint8_t* track, int a, float weight, int b, float c, float d);
GAME_FN(void, __thiscall, AmApi_BlendNodeInit, 0x006BE840, void* node);
GAME_FN(void*, __thiscall, AmApi_GetBone, 0x00464EA0, void* hierarchy, uint32_t bone);
GAME_FN(void, __thiscall, AmApi_ListMoveFrom, 0x006B2CA0, void* self, re::Ptr32<uint8_t>* before, void* source,
        re::Ptr32<uint8_t>* link);
GAME_FN(uint32_t, __thiscall, AmApi_HandleEquals, 0x006BD240, re::Ptr32<uint8_t>* self, re::Ptr32<uint8_t>* other);
GAME_FN(uint32_t, __cdecl, AmApi_HashName, 0x00576ED0, const char* name);

// 0x6C0B20 (the manager unused): slot's mask track set (-1: the first free slot) with its
// fade (-1: the mask's; under 0.001: none, full weight) and weight (-1: 0; else 0..1).
// Slot 0 always gets the skeleton's first weight set and no fade. 1: no slot
int __fastcall SetMaskTrack(void*, re::Edx, uint8_t* model, uint8_t* mask, re::Ptr32<uint8_t>* out, float fade, float weight,
                            int slot) {
    if (model == nullptr || mask == nullptr || uint32_t(slot + 1) > 7) {
        return 0x16;
    }
    if (slot == -1 && At<uint32_t>(model, 0x88) >= 7) {
        return 0xE;
    }
    if (weight == -1.0f) {
        weight = 0.0f;
    } else if (weight < 0.0f) {
        return 2;
    } else if (1.0f < weight) {
        return 2;
    }
    uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
    if (hierarchy == nullptr) {
        return 0xB;
    }
    void* hxd = At<void*>(hierarchy, 0x10);
    if (hxd == nullptr) {
        return 0x12;
    }
    uint32_t i;
    if (slot == -1) {
        i = 0;
    } else {
        i = uint32_t(slot);
        if (i >= 7) {
            if (out) {
                *out = nullptr;
            }
            return 1;
        }
    }
    for (; i < 7; i++) {
        if (At<uint8_t*>(model, 0x18 + i * 4) == nullptr || slot != -1) {
            break;
        }
    }
    if (i >= 7) {
        if (out) {
            *out = nullptr;
        }
        return 1;
    }
    if (i == 0) {
        mask = AmApi_WeightSet(hxd, 0);
        fade = 0.0f;
    }
    uint8_t* track = At<uint8_t*>(model, 0x18 + i * 4);
    if (track == nullptr) {
        track = AmApi_NewTrack();
        At<uint8_t*>(model, 0x18 + i * 4) = track;
        At<uint32_t>(track, 0x44) = i;
        At<uint32_t>(model, 0x88) += 1;
    }
    At<uint8_t*>(track, 8) = mask;
    if (fade == -1.0f) {
        fade = At<float>(mask, 0x28);
    }
    float fadeTime;
    if (std::fabs(fade) < 0.001f) {
        At<float>(track, 0xC) = 1.0f;
        fadeTime = 0.0f;
    } else {
        At<float>(track, 0xC) = weight;
        fadeTime = -fade;
    }
    At<float>(track, 0x10) = fadeTime;
    At<float>(track, 0x14) = fadeTime;
    if (out) {
        *out = At<uint8_t*>(model, 0x18 + i * 4);
    }
    return 0;
}

// 0x6C0CD0 (stdcall; the manager methods pass their ecx along unused): the track to use (*track, else the first slot's, else a new one in slot 0):
// 0x24 a given track still fading in; 4 when the player index is over 2
int __stdcall FindTrack(uint8_t* model, re::Ptr32<uint8_t>* track, int player) {
    if (*track == nullptr) {
        uint32_t i = 0;
        for (; i < 7; i++) {
            if (At<uint8_t*>(model, 0x18 + i * 4) != nullptr) {
                break;
            }
        }
        if (i < 7) {
            *track = At<uint8_t*>(model, 0x18 + i * 4);
        } else {
            uint8_t* made = AmApi_NewTrack();
            At<uint8_t*>(model, 0x18) = made;
            *track = made;
            At<uint32_t>(model, 0x88) += 1;
        }
    } else if (0.0f < At<float>(*track, 0x10)) {
        return 0x24;
    }
    return player <= 2 ? 0 : 4;
}

// 0x6C0D40 (thiscall): a track (null: the first slot's, else a new one in slot 0)
// started through 0x6BCC30 at weight 1; when that fails, +0x3C / +0x40 cleared.
// Returns 0x6BCC30's result
int __fastcall StartTrack(void* self, re::Edx, uint8_t* model, void* anim, uint8_t* track, int a, float c, float d) {
    if (track == nullptr) {
        uint32_t i = 0;
        for (; i < 7; i++) {
            if (At<uint8_t*>(model, 0x18 + i * 4) != nullptr) {
                break;
            }
        }
        if (i < 7) {
            track = At<uint8_t*>(model, 0x18 + i * 4);
        } else {
            track = AmApi_NewTrack();
            At<uint32_t>(model, 0x88) += 1;
            At<uint8_t*>(model, 0x18) = track;
        }
    }
    int started = AmApi_StartTrack(self, model, anim, track, a, 1.0f, 0, c, d);
    if (started == 0) {
        At<uint32_t>(track, 0x3C) = 0;
        At<float>(track, 0x40) = 0.0f;
    }
    return started;
}

// 0x6C0DC0 (the manager unused): a player resumed (-1: both), the lookup's result ignored
int __fastcall ResumePlayer(void*, re::Edx, uint8_t* model, re::Ptr32<uint8_t> track, int player) {
    AmApi_FindTrack(model, &track, player);
    if (player == -1) {
        At<uint32_t>(track, 0x28) &= ~1u;
        At<uint32_t>(track, 0x3C) &= ~1u;
        return 0;
    }
    At<uint32_t>(track, 0x28 + player * 0x14) &= ~1u;
    return 0;
}

// 0x6C0E10 (the manager unused): a player paused (-1: both)
int __fastcall PausePlayer(void*, re::Edx, uint8_t* model, re::Ptr32<uint8_t> track, int player) {
    int result = AmApi_FindTrack(model, &track, player);
    if (result != 0) {
        return result;
    }
    if (player == -1) {
        At<uint32_t>(track, 0x28) |= 1;
        At<uint32_t>(track, 0x3C) |= 1;
        return 0;
    }
    At<uint32_t>(track, 0x28 + player * 0x14) |= 1;
    return 0;
}

// 0x6C0E60 (the manager unused): a player's rate
int __fastcall SetPlayerRate(void*, re::Edx, uint8_t* model, re::Ptr32<uint8_t> track, int player, float rate) {
    int result = AmApi_FindTrack(model, &track, player);
    if (result != 0) {
        return result;
    }
    At<float>(track, 0x24 + player * 0x14) = rate;
    return 0;
}

// 0x6C0E90 (the manager unused; two more arguments unused): a player's time, clamped to
// its anim (2 when clamped; 0x1E no anim)
int __fastcall SetPlayerTime(void*, re::Edx, uint8_t* model, re::Ptr32<uint8_t> track, int player, float time, int, int) {
    if (track == nullptr) {
        int result = AmApi_FindTrack(model, &track, player);
        if (result != 0) {
            return result;
        }
    }
    uint8_t* p = track + 0x18 + player * 0x14;
    uint8_t* anim = At<uint8_t*>(p, 0);
    if (anim == nullptr) {
        return 0x1E;
    }
    if (time < 0.0f) {
        At<float>(p, 4) = 0.0f;
        At<float>(p, 8) = 0.0f;
        return 2;
    }
    if (At<float>(anim, 0xC) < time) {
        float end = At<float>(anim, 0xC);
        At<float>(p, 4) = end;
        At<float>(p, 8) = end;
        return 2;
    }
    At<float>(p, 4) = time;
    At<float>(p, 8) = time;
    return 0;
}

// 0x6C0F30 (the manager unused): a player's time (0x1E no anim)
int __fastcall GetPlayerTime(void*, re::Edx, uint8_t* model, re::Ptr32<uint8_t> track, int player, float* out) {
    if (track == nullptr) {
        int result = AmApi_FindTrack(model, &track, player);
        if (result != 0) {
            return result;
        }
    }
    uint8_t* p = track + player * 0x14;
    if (At<void*>(p, 0x18) == nullptr) {
        return 0x1E;
    }
    *out = At<float>(p, 0x1C);
    return 0;
}

// 0x6C0F80 (thiscall): a blend node taken from the manager's pool for a socket (its record
// at the HXD's +0x74, 0x30 each), aimed at the joint whose id is the record's +0x24 plus 1,
// and put in the model's list. 0x10: none left
int __fastcall AddBlendNode(uint8_t* self, re::Edx, uint8_t* model, int socket, re::Ptr32<uint8_t>* out) {
    if (model == nullptr) {
        return 0x16;
    }
    uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
    if (hierarchy == nullptr) {
        return 0xB;
    }
    if (At<void*>(hierarchy, 0x10) == nullptr) {
        return 0x12;
    }
    re::Ptr32<uint8_t> link = At<uint8_t*>(self, 0xEC);
    if (link == nullptr) {
        return 0x10;
    }
    AmApi_BlendNodeInit(At<void*>(link, 0));
    At<uint8_t*>(At<uint8_t*>(link, 0), 0x34) = link;
    uint8_t* node = At<uint8_t*>(link, 0);
    At<uint8_t*>(node, 0) = At<uint8_t*>(At<uint8_t*>(hierarchy, 0x10), 0x74) + socket * 0x30;
    uint32_t count = At<uint32_t>(hierarchy, 0x9C);
    if (count != 0) {
        uint32_t id = At<uint32_t>(At<uint8_t*>(node, 0), 0x24) + 1;
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* joint;
            if (i < At<uint32_t>(hierarchy, 0x9C)) {
                joint = At<uint8_t*>(hierarchy, 0xA0) + i * 0x28;
            } else {
                // (never reached: the loop stops at the joints' count) the HXD's sockets
                uint8_t* hxd = At<uint8_t*>(hierarchy, 0x10);
                joint = nullptr;
                if (hxd) {
                    uint32_t k = i - At<uint32_t>(hierarchy, 0x9C);
                    if (k < At<uint32_t>(hxd, 0x5C)) {
                        uint8_t* s = At<uint8_t*>(hxd, 0x60) + k * 0xC;
                        if (s != nullptr && reinterpret_cast<uintptr_t>(s) + 4 != 0) {
                            joint = s;
                        }
                    }
                }
            }
            if (At<uint32_t>(joint, 8) == id) {
                At<void*>(node, 0x30) = AmApi_GetBone(hierarchy, i);
                break;
            }
        }
    }
    re::Ptr32<uint8_t> before = nullptr;
    AmApi_ListMoveFrom(model + 0xD8, &before, self + 0xE8, &link);
    if (out) {
        *out = node;
    }
    return 0;
}

// 0x6C10E0 (thiscall): a blend node taken off the model and given back (0x1D: the model
// has none, or the node isn't linked)
int __fastcall RemoveBlendNode(uint8_t* self, re::Edx, uint8_t* model, uint8_t* node) {
    if (model == nullptr || node == nullptr) {
        return 0x16;
    }
    uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
    if (hierarchy == nullptr) {
        return 0xB;
    }
    if (At<void*>(hierarchy, 0x10) == nullptr) {
        return 0x12;
    }
    if (At<uint32_t>(model, 0xF0) == 0) {
        return 0x1D;
    }
    re::Ptr32<uint8_t> link = At<uint8_t*>(node, 0x34);
    re::Ptr32<uint8_t> none = nullptr;
    if (AmApi_HandleEquals(&link, &none) != 0) {
        return 0x1D;
    }
    re::Ptr32<uint8_t> before = nullptr;
    AmApi_ListMoveFrom(self + 0xE8, &before, model + 0xD8, &link);
    return 0;
}

// the used objects of a list whose +offset is the hash (or the name's): 1 found
template <int List, int HashAt>
int FindByHash(uint8_t* self, uint32_t hash, const char* name, re::Ptr32<void>* out) {
    if (hash == 0) {
        if (name == nullptr) {
            return 0;
        }
        hash = AmApi_HashName(name);
    }
    for (uint8_t* link = At<uint8_t*>(self, List + 4); link; link = At<uint8_t*>(link, 8)) {
        void* object = At<void*>(link, 0);
        if (At<uint32_t>(object, HashAt) == hash) {
            if (out) {
                *out = object;
            }
            return 1;
        }
    }
    return 0;
}

// 0x6C17A0 (thiscall): a used hierarchy object by its hash (+0x14)
int __fastcall FindHierarchy(uint8_t* self, re::Edx, uint32_t hash, const char* name, re::Ptr32<void>* out) {
    return FindByHash<0, 0x14>(self, hash, name, out);
}

// 0x6C17F0 (thiscall): a used HXD by its hash (+0x18)
int __fastcall FindHxd(uint8_t* self, re::Edx, uint32_t hash, const char* name, re::Ptr32<void>* out) {
    return FindByHash<0x5C, 0x18>(self, hash, name, out);
}

} // namespace

#define AMAPI_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMAPI_REVERSED("AM_Manager::SetMaskTrack", 0x006C0B20, &SetMaskTrack);
AMAPI_REVERSED("AM_Manager::FindTrack", 0x006C0CD0, &FindTrack);
AMAPI_REVERSED("AM_Manager::StartTrack", 0x006C0D40, &StartTrack);
AMAPI_REVERSED("AM_Manager::ResumePlayer", 0x006C0DC0, &ResumePlayer);
AMAPI_REVERSED("AM_Manager::PausePlayer", 0x006C0E10, &PausePlayer);
AMAPI_REVERSED("AM_Manager::SetPlayerRate", 0x006C0E60, &SetPlayerRate);
AMAPI_REVERSED("AM_Manager::SetPlayerTime", 0x006C0E90, &SetPlayerTime);
AMAPI_REVERSED("AM_Manager::GetPlayerTime", 0x006C0F30, &GetPlayerTime);
AMAPI_REVERSED("AM_Manager::AddBlendNode", 0x006C0F80, &AddBlendNode);
AMAPI_REVERSED("AM_Manager::RemoveBlendNode", 0x006C10E0, &RemoveBlendNode);
AMAPI_REVERSED("AM_Manager::FindHierarchy", 0x006C17A0, &FindHierarchy);
AMAPI_REVERSED("AM_Manager::FindHxd", 0x006C17F0, &FindHxd);
