// A ped's social exchange as it runs (0x4911C0 .. 0x492A58): the reply chosen each
// step, the exchange kept up or ended, starting one, and the humiliation anim for the
// player. See PedSocialMood.cpp for the object.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct V3 {
    float x, y, z;
};

V3* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<V3*>(matrix + 0x30) : &At<V3>(entity, 4);
}

inline const int32_t* const PSX_replies = reinterpret_cast<const int32_t*>(0x00A34D30);      // [mood][10]
inline const int32_t* const PSX_repliesC = reinterpret_cast<const int32_t*>(0x00A34E98);     // [mood][10]
inline const int32_t* const PSX_repliesD = reinterpret_cast<const int32_t*>(0x00A34BC8);     // [mood][10]
inline void* const PSX_nodeA = reinterpret_cast<void*>(0x01B65444);
inline void* const PSX_nodeB = reinterpret_cast<void*>(0x01B65448);
inline void* const PSX_nodeC = reinterpret_cast<void*>(0x01B65450);
inline void* const PSX_nodeD = reinterpret_cast<void*>(0x01B6555C);
inline void* const PSX_nodeE = reinterpret_cast<void*>(0x01B65344);
inline void* const PSX_nodeF = reinterpret_cast<void*>(0x01B6544C);
GAME_VAR(uint32_t, PSX_timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t*, PSX_playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t, PSX_cutsceneRunning, 0x020C5BE4);
GAME_VAR(int32_t, PSX_specialModel1, 0x00BF3F20);
GAME_VAR(int32_t, PSX_specialModel2, 0x00BF3F1C);

GAME_FN(void, __thiscall, PSX_RefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(int, __cdecl, PSX_RandomRange, 0x0040DA80, int low, int high);
inline int __cdecl PSX_Rand() { return crt::Rand(); }
GAME_FN(float, __cdecl, PSX_Max, 0x00414490, float a, float b);
GAME_FN(V3*, __cdecl, PSX_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
GAME_FN(float, __cdecl, PSX_DistanceSquared, 0x00472D00, const V3* a, const V3* b);
GAME_FN(uint8_t*, __thiscall, PSX_PedGetModelInfo, 0x00477DC0, void* ped);
GAME_FN(int*, __thiscall, PSX_ModelInfo522380, 0x00522380, void* info, int* out);
GAME_FN(int, __thiscall, PSX_PedGetStat, 0x00477060, void* ped, int stat);
GAME_FN(int*, __thiscall, PSX_Ped474AA0, 0x00474AA0, void* ped, int* out);
GAME_FN(uint8_t, __thiscall, PSX_Ped474A30, 0x00474A30, void* ped);
GAME_FN(void, __thiscall, PSX_Ped474A90, 0x00474A90, void* ped, int);
GAME_FN(void, __thiscall, PSX_Ped474B30, 0x00474B30, void* ped, int);
GAME_FN(void, __thiscall, PSX_Ped47D1B0, 0x0047D1B0, void* ped, int);
GAME_FN(uint8_t, __thiscall, PSX_PedIsGangType, 0x0041AA50, void* ped);
GAME_FN(uint8_t, __thiscall, PSX_Ped4765C0, 0x004765C0, void* ped);
GAME_FN(void, __thiscall, PSX_Ped475320, 0x00475320, void* ped, int);
GAME_FN(void, __thiscall, PSX_Ped473100, 0x00473100, void* ped, int);
GAME_FN(uint8_t, __cdecl, PSX_Global5BB380, 0x005BB380);
GAME_FN(uint8_t*, __thiscall, PSX_Ref773DB0, 0x00773DB0, void* ref);
GAME_FN(uint8_t*, __thiscall, PSX_Controller5F49E0, 0x005F49E0, void* controller, int index);
GAME_FN(uint8_t, __thiscall, PSX_ControllerIsPlaying, 0x005F4800, void* controller, void* node, int, int);
// (the social object's own, through the game's address)
GAME_FN(void, __thiscall, PSX_SetMood, 0x0048F1D0, uint8_t* self, void* other, int mood, int keep, int keepKept);
GAME_FN(void*, __thiscall, PSX_FindStimulus, 0x0048D8C0, uint8_t* self, int type, re::Ptr32<void>* cached);
GAME_FN(uint8_t, __thiscall, PSX_Check6CAAC0, 0x0048E6B0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSX_Is120, 0x0048D690, uint8_t* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSX_Is124, 0x0048D6B0, uint8_t* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSX_Is128, 0x0048D6D0, uint8_t* self, void* ped);
GAME_FN(uint8_t, __thiscall, PSX_OtherMoveIsFriendly, 0x0048D580, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSX_OtherMoveIsA, 0x0048D5C0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSX_MoveIsFriendly, 0x0048D5F0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PSX_MoveIsA, 0x0048D620, uint8_t* self);
GAME_FN(uint16_t, __cdecl, PSX_GetClassId, 0x0048D730);
GAME_FN(uint8_t, __cdecl, PSX_Button1, 0x0048DEF0);
GAME_FN(uint8_t, __cdecl, PSX_Button4, 0x0048E130);
GAME_FN(uint8_t, __cdecl, PSX_Button5, 0x0048E1F0);
GAME_FN(void, __thiscall, PSX_EndPending, 0x0048F380, uint8_t* self);
GAME_FN(void, __thiscall, PSX_ChooseHumiliation, 0x0048F720, uint8_t* self, int index);
GAME_FN(void, __thiscall, PSX_ChooseAnswer, 0x00490120, uint8_t* self, int a, int b, int c);
GAME_FN(void, __thiscall, PSX_ChooseReply, 0x00490490, uint8_t* self, int a, int b, int c);
GAME_FN(uint8_t, __thiscall, PSX_IsPedBusy, 0x00490AF0, uint8_t* self, void* ped);
GAME_FN(int, __stdcall, PSX_FindPedValueInRange, 0x00491040, int low, int high);
GAME_FN(void, __thiscall, PSX_EndExchange, 0x004910A0, uint8_t* self);
GAME_FN(void, __thiscall, PSX_ChooseStep, 0x004911C0, uint8_t* self, int a, int b, int c);

void* Ped(uint8_t* self) {
    return At<void*>(self, 4);
}

bool IsStudentType(int type) {
    return type == 0 || type == 8 || type == 7;
}

bool HasFlag50(void* ped) {
    return At<uint8_t>(PSX_PedGetModelInfo(ped), 0x50) != 0;
}

// The fight stance taken (0x11)
void StartFight(uint8_t* self) {
    At<int32_t>(self, 0xE0) = 0x11;
    PSX_Ped474B30(Ped(self), 0x16);
    PSX_Ped474A90(Ped(self), 1);
    PSX_Ped47D1B0(Ped(self), 0);
    At<uint8_t>(self, 0x224) |= 0x40;
}

void PlayMoodKept(uint8_t* self, int keepKept) {
    PSX_SetMood(self, At<void*>(self, 0x70), At<int32_t>(self, 0x1FC), 0, keepKept);
    At<int32_t>(self, 0xEC) = At<int32_t>(self, 0x1FC);
}

// The game's 0x4911C0 up to its common end (true: go on to it; false: it returned)
bool ChooseStepBody(uint8_t* self, uint8_t a, uint8_t b, uint8_t c) {
    uint8_t* other = At<uint8_t*>(self, 0x70);
    At<int32_t>(self, 0x20C) = 1900;
    bool isPlayer = false;
    if (other) {
        if (At<int32_t>(other, 0x1310) == 13) {
            At<uint32_t>(self, 0x220) = PSX_timeInMs;
            int queued = At<int32_t>(self, 0x1F4);
            isPlayer = true;
            if (queued != 0x19) {
                At<int32_t>(self, 0xE0) = queued;
                if (queued == 0x11) {
                    PSX_Ped474B30(Ped(self), 0x16);
                    PSX_Ped474A90(Ped(self), 1);
                    PSX_Ped47D1B0(Ped(self), 0);
                    At<uint8_t>(self, 0x224) |= 0x40;
                }
                At<int32_t>(self, 0x1F4) = 0x19;
                return false;
            }
            re::Ptr32<void> pending = At<void*>(Ped(self), 0x1D30);
            for (int i = 0; i < 0x19; i++) {
                int8_t type = At<int8_t>(self, 0x156 + i);
                if (type && PSX_FindStimulus(self, type, &pending)) {
                    At<int32_t>(self, 0xE0) = i;
                    At<uint8_t>(pending, 0x2A) = 1;
                    if (At<void*>(Ped(self), 0x1D30) == pending) {
                        At<void*>(Ped(self), 0x1D30) = nullptr;
                    }
                    return false;
                }
            }
            if (PSX_FindStimulus(self, 0x31, &pending)) {
                At<int32_t>(self, 0xE0) = 0x13;
                return false;
            }
        } else if (At<int32_t>(self, 0x1F4) != 0x19) {
            if (At<int32_t>(self, 0xE0) != 0x11) {
                At<int32_t>(self, 0xE0) = At<int32_t>(self, 0x1F4);
            }
            At<int32_t>(self, 0x1F4) = 0x19;
            return false;
        }
    }
    if (At<void*>(self, 0x138) == At<void*>(self, 0x70)) {
        At<int32_t>(self, 0xE0) = 0x13;
        return false;
    }
    if (c) {
        int theirs = At<int32_t>(self, 0xE4);
        if (theirs == 1 || theirs == 3 || theirs == 8 || theirs == 9 || theirs == 7 || PSX_OtherMoveIsFriendly(self)) {
            b = 1;
        }
    }
    int low = 0;
    int high = 9;
    bool markA = false;
    bool markB = false;
    if (HasFlag50(Ped(self))) {
        low = 1;
        markA = true;
        markB = true;
    }
    int type = At<int32_t>(Ped(self), 0x1310);
    if (type == 8 || type == 0 || type == 9 || type == 0xA || type == 7) {
        low += 2;
        markA = true;
        markB = true;
    }
    if (type == 1 || type == 6) {
        markA = true;
    }
    if (At<int32_t>(self, 0x118) != 0) {
        low += 1;
    }
    other = At<uint8_t*>(self, 0x70);
    if (other) {
        if (HasFlag50(other)) {
            low += 1;
            if (!HasFlag50(Ped(self))) {
                low += 1;
                markB = true;
            }
        }
        other = At<uint8_t*>(self, 0x70);
        if (IsStudentType(At<int32_t>(other, 0x1310))) {
            low += 2;
            markA = true;
            markB = true;
        }
    }
    At<uint8_t>(self, 0x225) &= 0xF3;
    if (!other || !IsStudentType(At<int32_t>(other, 0x1310))) {
        uint8_t* nearby = At<uint8_t*>(self, 4) + 0x3BC;
        for (int i = 0; i < At<int32_t>(nearby, 4); i++) {
            uint8_t* ped = At<uint8_t*>(nearby, 8 + i * 4);
            if (!ped || !IsStudentType(At<int32_t>(ped, 0x1310))) {
                continue;
            }
            At<uint8_t>(self, 0x225) |= 8;
            V3 d;
            V3* diff = PSX_VectorSub(&d, EntityPos(ped), EntityPos(Ped(self)));
            float distanceSq = diff->x * diff->x + diff->y * diff->y + diff->z * diff->z;
            if (distanceSq < 16.0f) {
                At<uint8_t>(self, 0x225) |= 4;
                break;
            }
        }
    }
    if (isPlayer) {
        int model = At<int16_t>(Ped(self), 0x10E);
        bool answer = At<int32_t>(self, 0x1FC) <= 1 && (model == PSX_specialModel1 || model == PSX_specialModel2);
        if (!answer) {
            int mood = At<int32_t>(self, 0xEC);
            if ((mood == 0 || (At<int32_t>(Ped(self), 0x1310) != 5 && mood == 1)) && !markA && !markB &&
                !(At<uint8_t>(self, 0x225) & 4)) {
                answer = true;
            }
        }
        if (answer) {
            At<uint8_t>(self, 0x226) |= 8;
            PSX_ChooseAnswer(self, a, b, c);
            return false;
        }
        At<uint8_t>(self, 0x226) &= 0xF7;
        int out;
        if (!HasFlag50(Ped(self)) && *PSX_Ped474AA0(Ped(self), &out) == 2 && At<int32_t>(self, 0xEC) >= 7) {
            PSX_ChooseReply(self, a, b, c);
            return false;
        }
    }
    if (low > 0 && !c && !b) {
        low += 4;
    }
    if (low > 9) {
        low = 9;
    }
    bool raise;
    if (b == 1 && !c) {
        raise = true;
    } else if (!At<void*>(self, 0x70)) {
        raise = false;
    } else if (!isPlayer) {
        raise = false;
        goto clamp_done;
    } else {
        int theirs = At<int32_t>(self, 0xE4);
        raise = theirs == 0 || theirs == 1 || theirs == 6 || theirs == 0x15 || theirs == 8 || theirs == 7 || theirs == 0x11;
    }
    if (raise) {
        float f = static_cast<float>(low);
        if (f < 6.0) {
            f = 6.0f;
        }
        high = static_cast<int>(f);
    }
    if (isPlayer) {
        int seen = high - (At<int32_t>(self, 0x98) + At<int32_t>(self, 0x94) + At<int32_t>(self, 0x90)) * 3;
        float f = static_cast<float>(seen);
        if (f < 0.0) {
            f = 0.0f;
        }
        high = static_cast<int>(f);
        if (high < low) {
            low = high;
        }
    }
clamp_done:
    uint8_t flags = At<uint8_t>(self, 0x224);
    if (flags & 0x80) {
        At<int32_t>(self, 0xE0) = 0x15;
        return true;
    }
    uint8_t bit1 = (At<uint8_t>(self, 0x226) >> 1) & 1;
    if (bit1 && isPlayer && (At<int32_t>(self, 0xE8) == 0x16 || At<int32_t>(self, 0xE8) == 2)) {
        PSX_RefSet(self + 0x128, At<void*>(self, 0x70));
        if (PSX_Button5()) {
            At<int32_t>(self, 0xE0) = 0xE;
        } else {
            At<int32_t>(self, 0xE0) = PSX_Button1() ? 3 : 9;
        }
        if (!(At<uint8_t>(self, 0x225) & 2) && At<int32_t>(Ped(self), 0x141C) == 0 && At<int32_t>(self, 0x1FC) <= At<int32_t>(self, 0xEC)) {
            return true;
        }
        PlayMoodKept(self, (At<uint8_t>(self, 0x226) >> 2) & 1);
        return true;
    }
    if (bit1 == 1) {
        int mood = At<int32_t>(self, 0xEC);
        if (mood > 3 && mood <= 6) {
            At<int32_t>(self, 0x134) += 1;
            At<int32_t>(self, 0xE0) = 0xA;
            return true;
        }
        if (At<int32_t>(Ped(self), 0x141C) != 0 || At<int32_t>(self, 0xEC) > 3) {
            At<int32_t>(self, 0x134) += 1;
            At<int32_t>(self, 0xE0) = 0xC;
            return true;
        }
        if (At<int32_t>(self, 0x12C) <= 3) {
            At<int32_t>(self, 0x134) += 1;
            At<int32_t>(self, 0xE0) = 0;
            return true;
        }
    }
    uint8_t flags225 = At<uint8_t>(self, 0x225);
    auto randomReply = [&](bool fromC) {
        int r = PSX_RandomRange(low, high);
        const int32_t* table = fromC ? PSX_repliesC : PSX_replies;
        At<int32_t>(self, 0xE0) = table[r + At<int32_t>(self, 0xEC) * 10];
    };
    if (flags225 & 1) {
        int pedType = At<int32_t>(Ped(self), 0x1310);
        if (((pedType >= 1 && pedType <= 6) || pedType == 0xB || pedType == 0xD) &&
            (At<int32_t>(self, 0xEC) <= 1 || (At<int32_t>(self, 0xEC) == 2 && PSX_OtherMoveIsFriendly(self)))) {
            At<int32_t>(self, 0xE0) = 9;
            return true;
        }
        int mood = At<int32_t>(self, 0xEC);
        if (mood <= 3) {
            At<int32_t>(self, 0xE0) = 3;
            return true;
        }
        if (mood > 6) {
            At<int32_t>(self, 0xE0) = 0xE;
            return true;
        }
        void* who = At<void*>(self, 0x70);
        if (PSX_Is120(self, who) && !PSX_Is128(self, who)) {
            At<int32_t>(self, 0xE0) = 0x13;
            return true;
        }
        At<int32_t>(self, 0xE0) = 0xE;
        return true;
    }
    if (b) {
        if ((flags & 0x40) && isPlayer && At<int32_t>(self, 0xE8) == 0) {
            At<int32_t>(self, 0xE0) = PSX_Button4() ? 0x17 : 0x15;
            return true;
        }
        uint8_t bit225 = (flags225 >> 1) & 1;
        if (bit225 && isPlayer) {
            At<uint8_t>(self, 0x225) = flags225 & 0xFD;
            randomReply(c != 0);
            return true;
        }
        uint8_t* who = At<uint8_t*>(self, 0x70);
        if (who && At<int32_t>(self, 0xE8) == 8 && At<int32_t>(self, 0xEC) <= 3) {
            At<int32_t>(self, 0xE0) = 0x17;
            if (!isPlayer) {
                return true;
            }
            if (!bit225 && At<int32_t>(Ped(self), 0x141C) == 0) {
                return true;
            }
            PSX_SetMood(self, who, At<int32_t>(self, 0x1FC), 0, 1);
            At<int32_t>(self, 0xEC) = At<int32_t>(self, 0x1FC);
            return true;
        }
        if (PSX_PedIsGangType(Ped(self)) || (At<int32_t>(self, 0x12C) != -1 && !bit1)) {
            randomReply(c != 0);
            return true;
        }
        int theirs = At<int32_t>(self, 0xE4);
        if (theirs == 0x14 || theirs == 5) {
            At<int32_t>(self, 0xE0) = 0x13;
            return true;
        }
        int chance = 0;
        int mood = At<int32_t>(self, 0xEC);
        if (mood == 6 || mood == 5 || mood == 4) {
            float least = mood == 6 ? 10.0f : mood == 5 ? 25.0f : 40.0f;
            float stat = static_cast<float>(PSX_PedGetStat(Ped(self), 7));
            chance = static_cast<int>(PSX_Max(least, stat));
        }
        if (PSX_RandomRange(0, 100) >= chance) {
            randomReply(c != 0);
            return true;
        }
        if ((At<uint8_t>(self, 0x225) & 8) && PSX_PedGetStat(Ped(self), 7) > 0 && At<uint8_t>(Ped(self), 0x1570) == 1 &&
            At<int32_t>(Ped(self), 0x141C) == 0) {
            At<int32_t>(self, 0xE0) = 0x14;
            return true;
        }
        randomReply(false);
        return true;
    }
    if (flags & 8) {
        At<int32_t>(self, 0xE0) = 0xE;
        return true;
    }
    uint8_t bit6 = (flags >> 6) & 1;
    if (bit6) {
        uint8_t* who = At<uint8_t*>(self, 0x70);
        if (who) {
            if (At<int32_t>(self, 0xE8) == 0x16) {
                if (!isPlayer) {
                    At<int32_t>(self, 0xE0) = 6;
                    return true;
                }
                if (At<int32_t>(Ped(self), 0x141C) == 0 && !PSX_Button4()) {
                    At<int32_t>(self, 0xE0) = 6;
                    return true;
                }
                At<int32_t>(self, 0xE0) = 0x17;
                if (At<int32_t>(Ped(self), 0x141C) == 0) {
                    return true;
                }
                PlayMoodKept(self, 1);
                return true;
            }
            if (!isPlayer && At<int32_t>(who, 0xC28) == 0x16) {
                At<int32_t>(self, 0xE0) = 6;
                return true;
            }
        }
        if (!isPlayer) {
            goto after_moves;
        }
        int move = At<int32_t>(self, 0xE8);
        if (move == 8 || move == 2) {
            if (At<int32_t>(Ped(self), 0x141C) == 0 && !PSX_Button4()) {
                return true;
            }
            At<int32_t>(self, 0xE0) = 0x17;
            if (At<int32_t>(Ped(self), 0x141C) == 0) {
                return true;
            }
            PSX_SetMood(self, At<void*>(self, 0x70), At<int32_t>(self, 0x1FC), 0, 1);
            At<int32_t>(self, 0xEC) = At<int32_t>(self, 0x1FC);
            return true;
        }
    }
    if (!isPlayer) {
        goto after_moves;
    }
    if (At<int32_t>(self, 0xE8) == 5 && At<int32_t>(self, 0xE4) != 4) {
        uint8_t* ped = At<uint8_t*>(self, 4);
        int out;
        if ((flags225 & 2) || (At<int32_t>(ped, 0x1E28) > 0 && At<int32_t>(ped, 0x1430) == 1) || *PSX_Ped474AA0(ped, &out) == 2) {
            At<uint8_t>(self, 0x225) &= 0xFD;
            At<int32_t>(self, 0xE0) = 4;
            return true;
        }
    }
    if (At<int32_t>(self, 0xE8) == 2) {
        At<int32_t>(self, 0xE0) = 0x17;
        if (!(At<uint8_t>(self, 0x225) & 2) && At<int32_t>(Ped(self), 0x141C) == 0 && At<int32_t>(self, 0x1FC) <= At<int32_t>(self, 0xEC)) {
            return true;
        }
        PlayMoodKept(self, 1);
        return true;
    }
after_moves:
    if (At<int32_t>(self, 0x12C) == -1 && !(At<uint8_t>(self, 0x224) & 1) && !a && !markA && !(At<uint8_t>(self, 0x224) & 0x40) &&
        isPlayer && !PSX_Ped474A30(Ped(self)) && At<int32_t>(self, 0xEC) == 0 && PSX_PedGetStat(Ped(self), 0xE) > 0xF &&
        (PSX_OtherMoveIsFriendly(self) || PSX_OtherMoveIsA(self)) && (PSX_Rand() & 1)) {
        StartFight(self);
        return true;
    }
    int kept = At<int32_t>(self, 0x12C);
    if (kept == -1 && !a && !markB) {
        uint8_t* who = At<uint8_t*>(self, 0x70);
        int mood = At<int32_t>(self, 0xEC);
        if (who && (mood == 0 || mood == 1) &&
            ((!isPlayer && At<int32_t>(who, 0xC28) == 0x16) || At<int32_t>(self, 0xE8) == 0x16) &&
            (PSX_OtherMoveIsFriendly(self) || PSX_OtherMoveIsA(self))) {
            bool calm;
            if (isPlayer) {
                calm = !PSX_MoveIsFriendly(self) && !PSX_MoveIsA(self);
            } else {
                uint8_t* theirs = who + 0xB44;
                calm = !PSX_OtherMoveIsFriendly(theirs) && !PSX_OtherMoveIsA(theirs);
            }
            if (calm) {
                if (At<int32_t>(Ped(self), 0x141C) != 0) {
                    At<int32_t>(self, 0xE0) = 0x17;
                } else if (PSX_PedGetStat(Ped(self), 0xE) > 0x19 || !PSX_Button1()) {
                    At<int32_t>(self, 0xE0) = 6;
                } else {
                    At<int32_t>(self, 0xE0) = 0x17;
                }
                return true;
            }
        }
    }
    if (kept == -1 || (At<uint8_t>(self, 0x226) & 2)) {
        uint8_t* who = At<uint8_t*>(self, 0x70);
        if (!a && who && At<int32_t>(self, 0xEC) >= 2) {
            bool agree = false;
            if (At<int32_t>(self, 0xE8) == 0x16) {
                agree = true;
                if (isPlayer && !PSX_Button1() && (PSX_OtherMoveIsA(self) || PSX_OtherMoveIsFriendly(self))) {
                    At<int32_t>(self, 0xE0) = 9;
                    PlayMoodKept(self, (At<uint8_t>(self, 0x226) >> 2) & 1);
                    return true;
                }
            } else if (!isPlayer && At<int32_t>(who, 0xC28) == 0x16) {
                agree = true;
            }
            if (agree) {
                At<int32_t>(self, 0xE0) = 0x17;
                if (!isPlayer) {
                    return true;
                }
                PlayMoodKept(self, (At<uint8_t>(self, 0x226) >> 2) & 1);
                return true;
            }
        }
    }
    if (!c && At<int32_t>(self, 0x21C) == 0 && isPlayer && !(At<uint8_t>(self, 0x224) & 0x40) && At<int32_t>(self, 0xEC) >= 3 &&
        PSX_Ped474A30(Ped(self)) == 1) {
        uint8_t* partner = At<uint8_t*>(At<uint8_t*>(self, 0x70), 0x2F8);
        uint8_t* ped = At<uint8_t*>(self, 4);
        if (!partner || partner == ped || At<uint8_t>(ped, 0x1570) == 2) {
            int out;
            bool ready = true;
            if (*PSX_Ped474AA0(Ped(self), &out) == 2 || *PSX_Ped474AA0(Ped(self), &out) == 1) {
                if (At<uint8_t>(At<uint8_t*>(self, 4), 0x1570) == 1) {
                    uint8_t* player = PSX_playerPed;
                    ready = !PSX_Check6CAAC0(self) && !PSX_Is124(self, player) && (!PSX_Is120(self, player) || PSX_Is128(self, player));
                }
            }
            if (ready) {
                At<uint8_t>(self, 0x225) |= 2;
                At<int32_t>(self, 0xE0) = 0x18;
                return true;
            }
        }
    }
    if (a && At<int32_t>(self, 0xEC) >= 4 && !c) {
        uint8_t* who = At<uint8_t*>(self, 0x70);
        if ((At<uint8_t>(self, 0x225) & 0x40) && At<int32_t>(self, 0xEC) >= 7 && !PSX_Check6CAAC0(self) && !PSX_Is124(self, who) &&
            (!PSX_Is120(self, who) || PSX_Is128(self, who))) {
            At<int32_t>(self, 0xE0) = 0xF;
        } else {
            At<int32_t>(self, 0xE0) = 0x12;
        }
        return true;
    }
    for (int tries = 0;;) {
        int r = PSX_RandomRange(low, high);
        const int32_t* table = c ? PSX_repliesC : PSX_repliesD;
        At<int32_t>(self, 0xE0) = table[r + At<int32_t>(self, 0xEC) * 10];
        tries++;
        if (At<int32_t>(self, 0xE0) != At<int32_t>(self, 0xE4) || tries >= 3) {
            break;
        }
    }
    return true;
}

} // namespace

namespace c07psx {

// 0x4911C0: the ped's step in the exchange: its reply (+0xE0) chosen from the queued one,
// the stimuli, its mood, the other side's move and chance; a threat (0xF) that can't be
// made against the player becomes 0x10
void __fastcall ChooseStep(uint8_t* self, re::Edx, int a, int b, int c) {
    if (!ChooseStepBody(self, static_cast<uint8_t>(a), static_cast<uint8_t>(b), static_cast<uint8_t>(c))) {
        return;
    }
    if (At<int32_t>(self, 0xE0) != 0xF) {
        return;
    }
    uint8_t* other = At<uint8_t*>(self, 0x70);
    if (other && At<int32_t>(other, 0x1310) == 13) {
        if (At<uint8_t>(At<uint8_t*>(self, 4), 0x1570) == 1) {
            uint8_t* who = At<uint8_t*>(self, 0x70);
            if (PSX_Check6CAAC0(self) || (PSX_Is120(self, who) && !PSX_Is128(self, who)) || PSX_Is124(self, who)) {
                At<int32_t>(self, 0xE0) = 0x10;
                return;
            }
        }
        if (At<int32_t>(At<uint8_t*>(self, 4), 0x1310) != 9) {
            return;
        }
    }
    At<int32_t>(self, 0xE0) = 0x10;
}

// 0x4921D0: the exchange kept up or ended (the other side gone, fighting, too far or
// too long, the player's moves over)
void __fastcall Update(uint8_t* self, re::Edx) {
    auto endBoth = [&]() {
        PSX_EndPending(self);
        PSX_EndExchange(self);
    };
    auto endTheirsToo = [&](uint8_t* other) {
        if (At<void*>(other, 0xBB4) == At<void*>(self, 4)) {
            PSX_EndPending(other + 0xB44);
        }
        endBoth();
    };
    if (At<void*>(self, 0x6C) == nullptr) {
        if ((At<uint8_t>(self, 0x224) & 0x10) && At<void*>(self, 0x74) &&
            (PSX_IsPedBusy(self, At<void*>(self, 4)) || At<uint8_t>(At<uint8_t*>(self, 0x74), 0x1C98))) {
            At<uint8_t>(self, 0x224) &= 0xEF;
        }
        return;
    }
    if (At<void*>(self, 0x70) &&
        ((PSX_Ped4765C0(At<void*>(self, 0)) && PSX_Global5BB380()) || PSX_cutsceneRunning || At<int32_t>(At<uint8_t*>(self, 4), 0x1370) == 1)) {
        uint8_t* ped = At<uint8_t*>(self, 4);
        uint8_t* pending = At<uint8_t*>(ped, 0x1D30);
        if (pending && At<uint8_t*>(pending, 8) == ped && At<void*>(self, 0x70) && At<void*>(pending, 4) == At<void*>(self, 0x70)) {
            At<uint8_t*>(ped, 0x1D30) = nullptr;
            At<uint8_t>(pending, 0x2A) = 1;
        }
        PSX_EndExchange(self);
        return;
    }
    uint8_t* other = At<uint8_t*>(self, 0x70);
    bool checkBusy = other == nullptr;
    if (other) {
        if (At<int32_t>(other, 0x1370) == 1 || At<uint8_t>(other, 0x1C98) || At<int32_t>(other, 0x1DD4) != 0 ||
            ((At<uint8_t>(other, 0xD68) & 0x10) && At<void*>(other, 0xBB8) != At<void*>(self, 4))) {
            endBoth();
            return;
        }
        uint8_t* theirController = At<uint8_t*>(other, 0x2E0) + 0x290;
        uint8_t* ped = At<uint8_t*>(self, 4);
        float limit = At<int32_t>(ped, 0x141C) != 0 ? 500.0f : 200.0f;
        if (!(At<uint8_t>(self, 0x224) & 0x10)) {
            float distanceSq = PSX_DistanceSquared(EntityPos(ped), EntityPos(other));
            if (limit < distanceSq || At<uint32_t>(self, 0xF4) + 45000 < PSX_timeInMs) {
                endTheirsToo(At<uint8_t*>(self, 0x70));
                return;
            }
        }
        other = At<uint8_t*>(self, 0x70);
        int type = At<int32_t>(other, 0x1310);
        bool playerCheck = false;
        if (type == 13) {
            playerCheck = true;
        } else if (At<uint32_t>(self, 0xF0) + 500 < PSX_timeInMs && (At<uint8_t>(self, 0x224) & 4)) {
            if (At<int32_t>(theirController, 0x34) > 0 && !PSX_ControllerIsPlaying(theirController, PSX_nodeA, 1, 1) &&
                !PSX_ControllerIsPlaying(theirController, PSX_nodeB, 1, 1) && !PSX_ControllerIsPlaying(theirController, PSX_nodeC, 1, 1)) {
                endTheirsToo(At<uint8_t*>(self, 0x70));
                return;
            }
        } else if (type == 13) {
            playerCheck = true;
        }
        if (playerCheck && At<uint8_t>(PSX_Ref773DB0(self + 0x70), 0x1E91)) {
            endTheirsToo(At<uint8_t*>(self, 0x70));
            return;
        }
        checkBusy = true;
    }
    if (checkBusy) {
        if (PSX_IsPedBusy(self, At<void*>(self, 4))) {
            uint8_t* who = At<uint8_t*>(self, 0x70);
            if (!who) {
                endBoth();
                return;
            }
            endTheirsToo(who);
            return;
        }
        if (!(At<uint32_t>(self, 0xF0) + 500 < PSX_timeInMs) && !(At<uint8_t>(self, 0x224) & 4)) {
            return;
        }
        uint8_t* controller = At<uint8_t*>(At<uint8_t*>(self, 4), 0x2E0) + 0x290;
        if (PSX_ControllerIsPlaying(controller, PSX_nodeA, 1, 1) || PSX_ControllerIsPlaying(controller, PSX_nodeB, 1, 1) ||
            PSX_ControllerIsPlaying(controller, PSX_nodeD, 1, 1)) {
            return;
        }
        if (PSX_ControllerIsPlaying(controller, PSX_nodeE, 1, 1) && At<void*>(self, 0x70) &&
            At<int32_t>(At<uint8_t*>(self, 0x70), 0x1310) == 13) {
            return;
        }
        int count = At<int32_t>(controller, 0x34);
        if (count <= 0) {
            return;
        }
        for (int i = 0; i < count; i++) {
            uint8_t* entry = PSX_Controller5F49E0(At<uint8_t*>(At<uint8_t*>(self, 4), 0x2E0) + 0x290, i);
            if (!entry) {
                continue;
            }
            re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entry);
            uint16_t id = PSX_GetClassId();
            if (!re::PtrCast<uint8_t(__thiscall*)(void*, uint16_t)>(vtable[0])(entry, id)) {
                continue;
            }
            uint8_t* entryController = entry + 0x84;
            if (!entryController) {
                continue;
            }
            if (PSX_ControllerIsPlaying(entryController, PSX_nodeF, 1, 1) || PSX_ControllerIsPlaying(entryController, PSX_nodeB, 1, 1) ||
                PSX_ControllerIsPlaying(entryController, PSX_nodeA, 1, 1)) {
                return;
            }
        }
        uint8_t* who = At<uint8_t*>(self, 0x70);
        if (!who) {
            endBoth();
            return;
        }
        endTheirsToo(who);
    }
}

// 0x492590: an exchange between two seated non-players ended
void __fastcall EndSeated(uint8_t* self, re::Edx) {
    uint8_t* other = At<uint8_t*>(self, 0x70);
    uint8_t* ped = At<uint8_t*>(self, 4);
    if (At<void*>(self, 0x6C) && other && At<uint8_t>(ped, 0x1570) == 1 && At<uint8_t>(other, 0x1570) == 1 &&
        At<int32_t>(other, 0x1310) != 13 && At<int32_t>(At<uint8_t*>(self, 4), 0x1310) != 13) {
        PSX_EndPending(self);
        PSX_EndExchange(self);
    }
}

// 0x4925E0: an exchange with `ped` started (the one running ended first with `restart`);
// +0x1F4 the queued reply
void __fastcall Start(uint8_t* self, re::Edx, void* ped, int restart, int queued, int flag) {
    if (static_cast<uint8_t>(restart) && At<void*>(self, 0x6C) && At<void*>(self, 0x70)) {
        PSX_EndExchange(self);
    }
    if (At<void*>(self, 0x74) != ped) {
        PSX_RefSet(self + 0x74, ped);
        At<uint8_t>(self, 0x224) &= 0xDF;
        At<int32_t>(self, 0x110) = 0;
        At<int32_t>(self, 0x114) = 0;
    }
    At<uint8_t>(self, 0x224) |= 0x10;
    uint8_t flags224 = At<uint8_t>(self, 0x224);
    At<uint32_t>(self, 0xFC) = PSX_timeInMs;
    uint8_t bits = static_cast<uint8_t>(static_cast<uint8_t>(flag) << 4);
    At<uint8_t>(self, 0x226) ^= (bits ^ At<uint8_t>(self, 0x226)) & 0x10;
    At<uint8_t>(self, 0x224) = static_cast<uint8_t>(((flags224 ^ static_cast<uint8_t>(restart)) & 1) ^ flags224);
    At<int32_t>(self, 0x1F4) = queued;
}

// 0x492680: the ped answers (the player backing off from a fight settles it), then the
// step chosen
void __fastcall Answer(uint8_t* self, re::Edx) {
    uint8_t* other = At<uint8_t*>(self, 0x70);
    if ((At<uint8_t>(self, 0x224) & 0x40) && other && At<int32_t>(other, 0x1310) == 13 && At<int32_t>(self, 0xE8) == 5) {
        PSX_Ped475320(At<void*>(self, 4), 100);
        PSX_Ped473100(At<void*>(self, 0x70), 100);
        PSX_Ped474A90(At<void*>(self, 4), 0);
        PSX_Ped474B30(At<void*>(self, 4), 0);
        PSX_RefSet(self + 0x128, At<void*>(self, 0x70));
        At<uint8_t>(self, 0x224) &= 0xBF;
        At<uint8_t>(self, 0x225) |= 1;
    }
    At<int32_t>(self, 0x6C) = 3;
    PSX_ChooseStep(self, 0, 0, 0);
    At<int32_t>(self, 0xE4) = At<int32_t>(self, 0xE0);
}

// 0x492720: a humiliation anim for the ped by its clique and the target's (the player:
// picked at random unless another ped shows one of the clique's already)
void __fastcall ChooseHumiliationFor(uint8_t* self, re::Edx, int forPlayer, uint8_t* target) {
    if (!target) {
        if (At<int32_t>(self, 0x200) != -1) {
            return;
        }
        target = At<uint8_t*>(self, 0x70);
        if (!target) {
            return;
        }
    }
    int clique;
    PSX_ModelInfo522380(PSX_PedGetModelInfo(At<void*>(self, 4)), &clique);
    float r = static_cast<float>(PSX_Rand() / 32767.0 * (1.0f - 0.0f) + 0.0f);
    auto pick2 = [&](int low, int high) {
        int shown = PSX_FindPedValueInRange(low, high);
        if (shown != -1) {
            PSX_ChooseHumiliation(self, shown);
        } else {
            PSX_ChooseHumiliation(self, 0.5f > r ? low : high);
        }
    };
    auto pick3 = [&](int low, int high) {
        int shown = PSX_FindPedValueInRange(low, high);
        if (shown != -1) {
            PSX_ChooseHumiliation(self, shown);
        } else if (r < 0.33000001311302185f) {
            PSX_ChooseHumiliation(self, low);
        } else if (r < 0.6600000262260437f) {
            PSX_ChooseHumiliation(self, low + 1);
        } else {
            PSX_ChooseHumiliation(self, high);
        }
    };
    if (At<int32_t>(target, 0x1310) == 13) {
        if (static_cast<uint8_t>(forPlayer)) {
            switch (clique) {
            case 0:
                pick2(8, 9);
                break;
            case 1:
                pick3(0, 2);
                break;
            case 2:
                pick3(5, 7);
                break;
            case 3:
                pick2(3, 4);
                break;
            case 4:
                pick3(10, 12);
                break;
            default:
                break;
            }
            return;
        }
        if (clique == 1) {
            PSX_ChooseHumiliation(self, 0x11);
        } else if (clique == 2) {
            PSX_ChooseHumiliation(self, 0x14);
        } else if (clique == 4) {
            PSX_ChooseHumiliation(self, 0xE);
        }
        return;
    }
    if (static_cast<uint8_t>(forPlayer)) {
        return;
    }
    int theirs;
    PSX_ModelInfo522380(PSX_PedGetModelInfo(target), &theirs);
    const int* choices = nullptr;
    static const int clique1[] = {0x10, 0x11, 0x12, 0x11};
    static const int clique2[] = {0x13, 0x14, 0x15, 0x14};
    static const int clique4[] = {0xD, 0xE, 0xF, 0xE};
    if (clique == 1) {
        choices = clique1;
    } else if (clique == 2) {
        choices = clique2;
    } else if (clique == 4) {
        choices = clique4;
    }
    if (choices && static_cast<uint32_t>(theirs) <= 3) {
        PSX_ChooseHumiliation(self, choices[theirs]);
    }
}

} // namespace c07psx

#define PSX_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PSX_REVERSED("PedSocial::ChooseStep", 0x004911C0, &c07psx::ChooseStep);
PSX_REVERSED("PedSocial::Update", 0x004921D0, &c07psx::Update);
PSX_REVERSED("PedSocial::EndSeated", 0x00492590, &c07psx::EndSeated);
PSX_REVERSED("PedSocial::Start", 0x004925E0, &c07psx::Start);
PSX_REVERSED("PedSocial::Answer", 0x00492680, &c07psx::Answer);
PSX_REVERSED("PedSocial::ChooseHumiliationFor", 0x00492720, &c07psx::ChooseHumiliationFor);
