// A ped's social exchanges with the player (0x490490 .. 0x4911B1): the ped's reply to
// the player's move, its leanings drifting with what it saw (+0x78 13 pairs of counts),
// the authority anim for the player's mood, and resetting an exchange. See
// PedSocialMood.cpp for the object.

#include <cmath>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

namespace {

using re::At;

inline const int32_t* const PSP_moods = reinterpret_cast<const int32_t*>(0x00A349E0);        // [11][11]
inline const int32_t* const PSP_replies = reinterpret_cast<const int32_t*>(0x00A34D30);      // [mood][10]
inline const int32_t* const PSP_repliesC = reinterpret_cast<const int32_t*>(0x00A34E98);     // [mood][10]
inline const int32_t* const PSP_seenA = reinterpret_cast<const int32_t*>(0x00A341B0);        // [13][10]
inline const int32_t* const PSP_seenB = reinterpret_cast<const int32_t*>(0x00A343B8);        // [13][10]
inline void* const PSP_nodeA = reinterpret_cast<void*>(0x01B6528C);
inline void* const PSP_nodeB = reinterpret_cast<void*>(0x01B65290);
GAME_VAR(uint32_t, PSP_timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t*, PSP_playerPed, 0x00C1AEA8);
GAME_VAR(CPool<uint8_t>*, PSP_pedPool, 0x00C0F5F0);

GAME_FN(void, __thiscall, PSP_RefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(int, __cdecl, PSP_RandomRange, 0x0040DA80, int low, int high);
GAME_FN(int*, __thiscall, PSP_Ped474AA0, 0x00474AA0, void* ped, int* out);
GAME_FN(uint8_t, __thiscall, PSP_Ped474A30, 0x00474A30, void* ped);
GAME_FN(void, __thiscall, PSP_Ped474A90, 0x00474A90, void* ped, int);
GAME_FN(void, __thiscall, PSP_Ped474B30, 0x00474B30, void* ped, int);
GAME_FN(uint8_t, __thiscall, PSP_Ped44E160, 0x0044E160, void* ped);
GAME_FN(uint8_t*, __thiscall, PSP_QueueGetCurrent, 0x00470340, void* queue);
GAME_FN(uint8_t*, __thiscall, PSP_Controller5F49E0, 0x005F49E0, void* controller, int index);
GAME_FN(uint8_t, __thiscall, PSP_ControllerIsPlaying, 0x005F4800, void* controller, void* node, int, int);
// (the social object's own, through the game's address)
GAME_FN(int*, __thiscall, PSP_GetMood, 0x0048EBC0, uint8_t* self, int* out, void* other, int fresh, int* outA, int* outB);
GAME_FN(void, __thiscall, PSP_SetMood, 0x0048F1D0, uint8_t* self, void* other, int mood, int keep, int keepKept);
GAME_FN(uint8_t, __thiscall, PSP_Check6CAAC0, 0x0048E6B0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSP_Is120, 0x0048D690, uint8_t* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSP_Is124, 0x0048D6B0, uint8_t* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSP_Is128, 0x0048D6D0, uint8_t* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSP_HasBothSides, 0x0048D6F0, uint8_t* self);
GAME_FN(uint16_t, __cdecl, PSP_GetClassId, 0x0048D730);
GAME_FN(void, __thiscall, PSP_BlendHumiliation, 0x0048D940, uint8_t* self);
GAME_FN(void, __thiscall, PSP_BlendAuthority, 0x0048DA00, uint8_t* self);
GAME_FN(void, __thiscall, PSP_BlendKiss, 0x0048DAC0, uint8_t* self);
GAME_FN(void, __thiscall, PSP_ChooseAuthority, 0x0048DB80, uint8_t* self, int index);
GAME_FN(void, __thiscall, PSP_DropAuthority, 0x0048DD10, uint8_t* self);
GAME_FN(void, __thiscall, PSP_DropHumiliation, 0x0048DDB0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSP_IsKind3, 0x0048E660, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSP_WantsToFight, 0x0048FB40, uint8_t* self);
GAME_FN(void, __thiscall, PSP_ChooseKissFromButtons, 0x0048F7B0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSP_IsKind48FDD0, 0x0048FDD0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSP_IsKind48FE40, 0x0048FE40, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSP_IsKind48FEA0, 0x0048FEA0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSP_IsKind48FEF0, 0x0048FEF0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSP_IsKind48FFF0, 0x0048FFF0, uint8_t* self);
GAME_FN(void, __thiscall, PSP_ChooseAuthorityForMood, 0x00490C40, uint8_t* self, int mood, void* other);
GAME_FN(void, __thiscall, PSP_UpdateLeanings, 0x00490830, uint8_t* self);

// The mood played (as the answer 0x48F1D0 path): kept as +0xEC
void SetMoodKept(uint8_t* self, int keepKept) {
    PSP_SetMood(self, At<void*>(self, 0x70), At<int32_t>(self, 0x1FC), 0, keepKept);
    At<int32_t>(self, 0xEC) = At<int32_t>(self, 0x1FC);
}

bool MayKeepMood(uint8_t* self) {
    return (At<uint8_t>(self, 0x225) & 2) || At<int32_t>(At<uint8_t*>(self, 4), 0x141C) != 0 ||
           At<int32_t>(self, 0x1FC) > At<int32_t>(self, 0xEC);
}

// The cell of the mood nearest (a, b) (each -5..5): -1 row when there's none
void NearestCell(int mood, int rowAt, int colAt, int* bestRow, int* bestCol) {
    *bestRow = -1;
    *bestCol = -1;
    for (int row = 0; row < 11; row++) {
        for (int col = 0; col < 11; col++) {
            if (mood != PSP_moods[row * 11 + col]) {
                continue;
            }
            if (*bestRow == -1 || (row - rowAt) * (row - rowAt) + (col - colAt) * (col - colAt) <
                                      (*bestCol - colAt) * (*bestCol - colAt) + (*bestRow - rowAt) * (*bestRow - rowAt)) {
                *bestRow = row;
                *bestCol = col;
            }
        }
    }
}

// (int)(sqrt(|x|) * sign) as the game's x87 does it
int SignedRoot(int value) {
    float magnitude = std::fabs(std::fabs(static_cast<float>(value)));
    float root = static_cast<float>(std::sqrt(magnitude));
    return static_cast<int>(root * (value >= 0 ? 1 : -1));
}

} // namespace

namespace c07psp {

// 0x490490: the ped's reply (+0xE0) to the player's move (`b`, `c`: the player's moves
// answered from the reply tables)
void __fastcall ChooseReply(uint8_t* self, re::Edx, int a, int b, int c) {
    if (At<void*>(self, 0x138) == At<void*>(self, 0x70)) {
        At<int32_t>(self, 0xE0) = 0x13;
        return;
    }
    uint8_t flags = At<uint8_t>(self, 0x224);
    if (flags & 0x80) {
        At<int32_t>(self, 0xE0) = 0x15;
        return;
    }
    uint8_t bit1 = (At<uint8_t>(self, 0x226) >> 1) & 1;
    if (bit1 && (At<int32_t>(self, 0xE8) == 0x16 || At<int32_t>(self, 0xE8) == 2)) {
        PSP_RefSet(self + 0x128, At<void*>(self, 0x70));
        At<int32_t>(self, 0xE0) = 0x17;
        if (!MayKeepMood(self)) {
            return;
        }
        SetMoodKept(self, (At<uint8_t>(self, 0x226) >> 2) & 1);
        return;
    }
    if (bit1 == 1) {
        int mood = At<int32_t>(self, 0xEC);
        if (mood > 3 && mood <= 6) {
            At<int32_t>(self, 0x134) += 1;
            At<int32_t>(self, 0xE0) = 10;
            return;
        }
        if (At<int32_t>(At<uint8_t*>(self, 4), 0x141C) != 0 || At<int32_t>(self, 0xEC) > 3) {
            At<int32_t>(self, 0x134) += 1;
            At<int32_t>(self, 0xE0) = 0xC;
            return;
        }
        if (At<int32_t>(self, 0x12C) <= 3) {
            At<int32_t>(self, 0x134) += 1;
            At<int32_t>(self, 0xE0) = 0;
            return;
        }
    }
    uint8_t flags225 = At<uint8_t>(self, 0x225);
    if (flags225 & 1) {
        At<int32_t>(self, 0xE0) = 0xE;
        return;
    }
    if (static_cast<uint8_t>(b) || static_cast<uint8_t>(c)) {
        if (flags225 & 2) {
            At<uint8_t>(self, 0x225) = flags225 & 0xFD;
        }
        int r = PSP_RandomRange(0, 9);
        const int32_t* table = static_cast<uint8_t>(c) ? PSP_repliesC : PSP_replies;
        At<int32_t>(self, 0xE0) = table[r + At<int32_t>(self, 0xEC) * 10];
        return;
    }
    if (flags & 8) {
        At<int32_t>(self, 0xE0) = 0xE;
        return;
    }
    if (At<int32_t>(self, 0xE8) == 5 && At<int32_t>(self, 0xE4) != 4) {
        uint8_t* ped = At<uint8_t*>(self, 4);
        int out;
        if ((flags225 & 2) || (At<int32_t>(ped, 0x1E28) > 0 && At<int32_t>(ped, 0x1430) == 1) || *PSP_Ped474AA0(ped, &out) == 2) {
            At<uint8_t>(self, 0x225) &= 0xFD;
            At<int32_t>(self, 0xE0) = 4;
            return;
        }
    }
    int move = At<int32_t>(self, 0xE8);
    if (move == 2) {
        At<int32_t>(self, 0xE0) = 0x17;
        if (!MayKeepMood(self)) {
            return;
        }
        SetMoodKept(self, 1);
        return;
    }
    if (At<int32_t>(self, 0x12C) == -1 || (At<uint8_t>(self, 0x226) & 2)) {
        if (static_cast<uint8_t>(a)) {
            At<int32_t>(self, 0xE0) = 0x12;
            return;
        }
        if (At<void*>(self, 0x70) && At<int32_t>(self, 0xEC) >= 2 && move == 0x16) {
            At<int32_t>(self, 0xE0) = 0x17;
            SetMoodKept(self, (At<uint8_t>(self, 0x226) >> 2) & 1);
            return;
        }
    } else if (static_cast<uint8_t>(a)) {
        At<int32_t>(self, 0xE0) = 0x12;
        return;
    }
    if (move == 0x10 && At<int32_t>(self, 0x21C) == 0 && PSP_Ped474A30(At<void*>(self, 4)) == 1) {
        uint8_t* ped = At<uint8_t*>(self, 4);
        bool ready = true;
        if (At<uint8_t>(ped, 0x1570) == 1) {
            uint8_t* own = At<uint8_t*>(self, 4) + 0xB44;
            uint8_t* player = PSP_playerPed;
            ready = !PSP_Check6CAAC0(self) && !PSP_Is124(own, player) && (!PSP_Is120(own, player) || PSP_Is128(own, player));
        }
        if (ready) {
            At<uint8_t>(self, 0x225) |= 2;
            At<int32_t>(self, 0xE0) = 0x18;
            return;
        }
    }
    At<int32_t>(self, 0xE0) = 0xF;
}

// 0x490830: the leanings moved: towards the mood kept (+0x1FC for the player unless it's
// the +0x120 one), or (alone) by what was seen
void __fastcall UpdateLeanings(uint8_t* self, re::Edx) {
    uint8_t* other = At<uint8_t*>(self, 0x70);
    if (!other) {
        int sumA = 0;
        int sumB = 0;
        for (int i = 0; i < 13; i++) {
            int seen = At<int32_t>(self, 0x78 + i * 8);
            if (seen > 0) {
                int k = seen - 1 > 4 ? 4 : seen - 1;
                sumA += PSP_seenA[i * 10 + k];
                sumB += PSP_seenB[i * 10 + k];
            }
            seen = At<int32_t>(self, 0x7C + i * 8);
            if (seen > 0) {
                int k = seen - 1 > 4 ? 4 : seen - 1;
                sumA += PSP_seenA[i * 10 + 5 + k];
                sumB += PSP_seenB[i * 10 + 5 + k];
            }
        }
        At<int32_t>(self, 0x100) += SignedRoot(sumA);
        At<int32_t>(self, 0x104) += SignedRoot(sumB);
        return;
    }
    At<int32_t>(self, 0x130) = -1;
    int mood, a, b;
    PSP_GetMood(self, &mood, other, 1, &a, &b);
    other = At<uint8_t*>(self, 0x70);
    uint8_t* ped = At<uint8_t*>(self, 4);
    if ((At<int32_t>(other, 0x1310) == 13 && (!other || other != At<uint8_t*>(self, 0x120))) || At<uint8_t>(ped, 0x1570) == 2 ||
        At<int32_t>(ped, 0x141C) == 1) {
        At<int32_t>(self, 0xEC) = At<int32_t>(self, 0x1FC);
    }
    int target = At<int32_t>(self, 0xEC);
    if (mood == target) {
        return;
    }
    if (static_cast<uint32_t>(mood - 4) <= 2 && !PSP_Is120(self, other)) {
        return;
    }
    int rowAt = a + 5;
    int colAt = b + 5;
    int bestRow, bestCol;
    NearestCell(target, rowAt, colAt, &bestRow, &bestCol);
    if (bestRow == -1) {
        return;
    }
    int dA = bestRow - rowAt;
    if (At<int32_t>(At<uint8_t*>(self, 0x70), 0x1310) == 13) {
        At<int32_t>(self, 0x108) += dA;
        At<int32_t>(self, 0x10C) += bestCol - colAt;
        return;
    }
    At<int32_t>(self, 0x100) += dA / 2;
    At<int32_t>(self, 0x104) += (bestCol - colAt) / 2;
}

// 0x490AF0: the ped is busy (+0x1DD4, +0x1370 1, attacking or objective 8, or a social
// node of its second controller playing either of two nodes)
uint8_t __fastcall IsPedBusy(void*, re::Edx, uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    if (At<int32_t>(ped, 0x1DD4) != 0 || At<int32_t>(ped, 0x1370) == 1) {
        return 1;
    }
    uint8_t* current = PSP_QueueGetCurrent(ped + 0x5F0);
    if (current && At<int32_t>(current, 8) == 7) {
        return 1;
    }
    current = PSP_QueueGetCurrent(ped + 0x5F0);
    if (current && At<int32_t>(current, 8) == 8) {
        return 1;
    }
    int count = At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x2C4);
    for (int i = 0; i < count; i++) {
        uint8_t* entry = PSP_Controller5F49E0(At<uint8_t*>(ped, 0x2E0) + 0x290, i);
        if (!entry) {
            continue;
        }
        re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entry);
        uint16_t id = PSP_GetClassId();
        if (!re::PtrCast<uint8_t(__thiscall*)(void*, uint16_t)>(vtable[0])(entry, id)) {
            continue;
        }
        uint8_t* controller = entry + 0x84;
        if (!controller) {
            continue;
        }
        if (PSP_ControllerIsPlaying(controller, PSP_nodeA, 1, 1) || PSP_ControllerIsPlaying(controller, PSP_nodeB, 1, 1)) {
            return 1;
        }
    }
    return 0;
}

// 0x490BE0: the social anims blended; a kiss chosen when it wants to fight; the player's
// partner keeps it engaged
void __fastcall UpdateAnims(uint8_t* self, re::Edx) {
    PSP_BlendHumiliation(self);
    PSP_BlendAuthority(self);
    PSP_BlendKiss(self);
    if (PSP_WantsToFight(self) && At<int32_t>(self, 0x208) == -1) {
        PSP_ChooseKissFromButtons(self);
    }
    if (At<uint8_t*>(At<uint8_t*>(self, 4), 0x155C) == PSP_playerPed) {
        At<uint32_t>(self, 0x220) = PSP_timeInMs + 400;
    }
}

// 0x490C40: the authority anim for the player's mood towards the ped, by the ped's kind
void __fastcall ChooseAuthorityForMood(uint8_t* self, re::Edx, uint32_t mood, uint8_t* other) {
    if (!other) {
        other = At<uint8_t*>(self, 0x70);
        if (!other) {
            return;
        }
    }
    if (At<int32_t>(self, 0x204) != -1 || At<int32_t>(other, 0x1310) != 13) {
        return;
    }
    int index;
    if (PSP_IsKind3(self)) {
        index = mood <= 2 ? 0x15 : mood == 4 ? 0x18 : (mood == 5 || mood == 6) ? 0x17 : 0x16;
    } else if (PSP_IsKind48FDD0(self)) {
        index = mood <= 2 ? 0xD : mood == 3 ? 0xE : mood <= 6 ? 0x10 : 0xF;
    } else if (PSP_IsKind48FE40(self)) {
        index = mood == 0 ? 4 : mood <= 2 ? 7 : (mood >= 4 && mood <= 6) ? 6 : 5;
    } else if (PSP_IsKind48FEA0(self)) {
        index = mood <= 2 ? 0x11 : mood == 4 ? 0x14 : (mood == 5 || mood == 6) ? 0x13 : 0x12;
    } else if (PSP_IsKind48FEF0(self)) {
        index = mood == 0 ? 0 : mood <= 2 ? 2 : 1;
    } else if (PSP_Ped44E160(At<void*>(self, 4))) {
        index = mood <= 2 ? 0x19 : mood == 4 ? 0x1C : (mood == 5 || mood == 6) ? 0x1B : 0x1A;
    } else if (PSP_IsKind48FFF0(self)) {
        index = mood <= 2 ? 0xD : mood == 3 ? 0x1D : mood <= 6 ? 0x10 : 0x1E;
    } else {
        index = mood <= 2 ? 9 : mood == 4 ? 0xC : (mood == 5 || mood == 6) ? 0xB : 0xA;
    }
    PSP_ChooseAuthority(self, index);
}

// 0x490F90: `ped` becomes the one the ped's authority is shown to (once a second at most,
// not while it's fighting someone): the old one's anim dropped, the new one's chosen
void __fastcall SetAuthorityTarget(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped || !(At<uint32_t>(self, 0x214) + 1000 < PSP_timeInMs)) {
        return;
    }
    uint8_t* current = At<uint8_t*>(self, 0x210);
    if (ped == current) {
        return;
    }
    uint8_t* social = ped + 0xB44;
    if (At<int32_t>(ped, 0xBB0) != 0 && At<void*>(social, 0x70) != nullptr) {
        return;
    }
    if (current) {
        uint8_t* old = current + 0xB44;
        if (!PSP_HasBothSides(old)) {
            PSP_DropAuthority(old);
        }
    }
    PSP_RefSet(self + 0x210, ped);
    int mood;
    PSP_GetMood(social, &mood, At<void*>(self, 4), 0, nullptr, nullptr);
    PSP_ChooseAuthorityForMood(social, mood, At<void*>(self, 4));
    At<uint32_t>(self, 0x214) = PSP_timeInMs;
}

// 0x491040: the first ped (from the pool's end) whose +0xD44 is within low..high: that
// value (-1: none)
int __stdcall FindPedValueInRange(int low, int high) {
    CPool<uint8_t>* pool = PSP_pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (!ped) {
            continue;
        }
        int value = At<int32_t>(ped, 0xD44);
        if (value >= low && value <= high) {
            return value;
        }
    }
    return -1;
}

// 0x4910A0: the exchange ended: leanings updated, the other side dropped, the state and
// flags reset (the fight stance let go without +0x1E28)
void __fastcall EndExchange(uint8_t* self, re::Edx) {
    PSP_UpdateLeanings(self);
    PSP_RefSet(self + 0x70, nullptr);
    At<uint8_t>(self, 0x224) &= 0xE0;
    uint8_t flags = At<uint8_t>(self, 0x224);
    At<uint8_t>(self, 0x225) &= 0x7F;
    At<uint8_t>(self, 0x226) &= 0xFE;
    At<int32_t>(self, 0x6C) = 0;
    At<int32_t>(self, 0x118) = 0;
    At<int32_t>(self, 0xE4) = 0x19;
    At<int32_t>(self, 0xE0) = 0x19;
    At<int32_t>(self, 0xE8) = 0x19;
    At<int32_t>(self, 0xFC) = 0;
    if ((flags & 0x40) && !(At<int32_t>(At<uint8_t*>(self, 4), 0x1E28) > 0)) {
        PSP_Ped474A90(At<void*>(self, 4), 0);
        PSP_Ped474B30(At<void*>(self, 4), 0);
        At<uint8_t>(self, 0x224) &= 0xBF;
    }
    if ((At<uint8_t>(self, 0x225) & 2) && !PSP_Ped474A30(At<void*>(self, 4))) {
        At<uint8_t>(self, 0x225) &= 0xFD;
    }
    uint8_t flags226 = At<uint8_t>(self, 0x226);
    At<uint8_t>(self, 0x224) &= 0x7F;
    At<uint8_t>(self, 0x225) &= 0xF2;
    if ((flags226 & 2) && !(flags226 & 4)) {
        At<int32_t>(self, 0x12C) = -1;
    }
    At<uint8_t>(self, 0x226) = flags226 & 0xF9;
    At<int32_t>(self, 0x134) = 0;
    At<int32_t>(self, 0x1F4) = 0x19;
    PSP_DropHumiliation(self);
    PSP_DropAuthority(self);
    At<uint8_t>(self, 0x225) &= 0xBF;
    At<uint8_t>(self, 0x226) &= 0xEF;
    At<int32_t>(self, 0x20C) = 1900;
    At<int32_t>(self, 0xEC) = 3;
    At<int32_t>(At<uint8_t*>(self, 4), 0x1494) = 0;
}

} // namespace c07psp

#define PSP_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PSP_REVERSED("PedSocial::ChooseReply", 0x00490490, &c07psp::ChooseReply);
PSP_REVERSED("PedSocial::UpdateLeanings", 0x00490830, &c07psp::UpdateLeanings);
PSP_REVERSED("PedSocial::IsPedBusy", 0x00490AF0, &c07psp::IsPedBusy);
PSP_REVERSED("PedSocial::UpdateAnims", 0x00490BE0, &c07psp::UpdateAnims);
PSP_REVERSED("PedSocial::ChooseAuthorityForMood", 0x00490C40, &c07psp::ChooseAuthorityForMood);
PSP_REVERSED("PedSocial::SetAuthorityTarget", 0x00490F90, &c07psp::SetAuthorityTarget);
PSP_REVERSED("PedSocial::FindPedValueInRange", 0x00491040, &c07psp::FindPedValueInRange);
PSP_REVERSED("PedSocial::EndExchange", 0x004910A0, &c07psp::EndExchange);
