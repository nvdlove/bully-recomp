// A ped's social state (at the ped's +0xB44): who it's socialising with, what each did
// to the other (counted by kind), and the decisions the socialize tracks make from
// that (SocializeTrack, SocializeActionCompleteTrack, ConditionSocialActionInterrupted).
// The player's own is used too, with `other` the ped it's engaging.

#include <cstddef>
#include <cstdint>

#include "game/Ped.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct PedSocial {
    uint8_t unknown0[4];          // +0x0
    re::Ptr32<CPed> ped;                    // +0x4 its ped (this is the ped's +0xB44)
    uint8_t unknown8_[0x64];
    int32_t state;                // +0x6C (2 socialising, 3 an action, 5 done, 6 ...)
    re::Ptr32<CPed> other;                  // +0x70 who it's with (registered)
    re::Ptr32<CPed> unknown74;              // +0x74
    int32_t counts[26];           // +0x78 how often each kind happened (cleared together)
    int32_t action;               // +0xE0 (0x19: none)
    int32_t lastAction;           // +0xE4
    int32_t response;             // +0xE8
    int32_t unknownEC;            // +0xEC
    uint32_t startTime;           // +0xF0 (CTimer ms)
    uint32_t time;                // +0xF4 (CTimer ms)
    uint32_t unknownF8;           // +0xF8 (CTimer ms)
    uint8_t unknownFC_[0x1C];
    int32_t unknown118;           // +0x118
    uint8_t unknown11C_[4];
    re::Ptr32<CPed> ref120;                 // +0x120 (registered)
    re::Ptr32<CPed> ref124;                 // +0x124 (registered)
    re::Ptr32<CPed> ref128;                 // +0x128 (registered)
    int32_t unknown12C;           // +0x12C (-1: none)
    uint8_t unknown130_[4];
    int32_t unknown134;           // +0x134
    re::Ptr32<CPed> ref138;                 // +0x138 (registered)
    uint8_t unknown13C_[4];
    uint8_t unknown140;           // +0x140
    uint8_t unknown141_[0x15];
    int8_t stimulusTypes[25];     // +0x156 (0: none)
    uint8_t unknown16F_[0x85];
    int32_t stimulusIndex;        // +0x1F4 the one of stimulusTypes that came
    int8_t stimulusType;          // +0x1F8
    uint8_t unknown1F9_[3];
    int32_t unknown1FC;           // +0x1FC
    uint8_t unknown200_[0xC];
    int32_t duration;             // +0x20C (ms)
    uint8_t unknown210_[0x14];
    uint8_t flags224;             // +0x224
    uint8_t flags225;             // +0x225
    uint8_t flags226;             // +0x226
    uint8_t unknown227_[5];
};
static_assert(offsetof(PedSocial, state) == 0x6C);
static_assert(offsetof(PedSocial, counts) == 0x78);
static_assert(offsetof(PedSocial, action) == 0xE0);
static_assert(offsetof(PedSocial, startTime) == 0xF0);
static_assert(offsetof(PedSocial, unknown118) == 0x118);
static_assert(offsetof(PedSocial, ref120) == 0x120);
static_assert(offsetof(PedSocial, unknown12C) == 0x12C);
static_assert(offsetof(PedSocial, ref138) == 0x138);
static_assert(offsetof(PedSocial, unknown140) == 0x140);
static_assert(offsetof(PedSocial, stimulusTypes) == 0x156);
static_assert(offsetof(PedSocial, stimulusIndex) == 0x1F4);
static_assert(offsetof(PedSocial, unknown1FC) == 0x1FC);
static_assert(offsetof(PedSocial, duration) == 0x20C);
static_assert(offsetof(PedSocial, flags224) == 0x224);
static_assert(sizeof(PedSocial) == 0x22C); // (the ped's +0xD70 follows)

constexpr int PEDTYPE_PLAYER = 13;

GAME_VAR(uint32_t, g_timeMs, 0x00C1A9B4);           // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint32_t, g_previousTimeMs, 0x00C1A9AC);   // the previous frame's
GAME_VAR(CPed*, g_playerPed, 0x00C1AEA8);
GAME_ARRAY(int32_t, g_socialThreshold, 0x00A33DEC); // [ped type]
GAME_ARRAY(int32_t, g_socialScore, 0x00A341B0);     // [26 kinds][5 times]
GAME_ARRAY(int32_t, g_socialCost, 0x00A343B8);      // [26 kinds][5 times]
// the stimulus managers
GAME_VAR(void*, g_stimulusMgr, 0x00C2C1A0);
GAME_VAR(void*, g_stimulusMgr2, 0x00C2C108);
GAME_VAR(void*, g_mgrC0F5E4, 0x00C0F5E4);
// the input actions (UserInput)
inline void* const g_input = reinterpret_cast<void*>(0x020C3CA0);
inline const void* const g_inputName1B65468 = reinterpret_cast<const void*>(0x01B65468);
inline const void* const g_inputName1B6544C = reinterpret_cast<const void*>(0x01B6544C);
// a class id given out on first use (0x48D730, inlined in 0x495590)
GAME_VAR(uint32_t, g_classIdGuard, 0x00C2C03C);
GAME_VAR(uint16_t, g_classId48D730, 0x00C2C038);
GAME_VAR(uint16_t, g_nextClassId, 0x00BF3830);

// the social's own
GAME_FN(void*, __thiscall, Social_FindStimulus, 0x0048D8C0, PedSocial* self, int type, re::Ptr32<void>* inOut);
GAME_FN(uint8_t, __thiscall, Social_48D580, 0x0048D580, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48D5C0, 0x0048D5C0, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48D5F0, 0x0048D5F0, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48D620, 0x0048D620, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48D650, 0x0048D650, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48D690, 0x0048D690, PedSocial* self, CPed* ped);
GAME_FN(uint8_t, __thiscall, Social_48D6F0, 0x0048D6F0, PedSocial* self);
GAME_FN(int, __thiscall, Social_48D770, 0x0048D770, PedSocial* self, int value);
GAME_FN(uint8_t, __thiscall, Social_48E130, 0x0048E130, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48E1F0, 0x0048E1F0, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48E2B0, 0x0048E2B0, PedSocial* self);
GAME_FN(int*, __thiscall, Social_48EBC0, 0x0048EBC0, PedSocial* self, int* out, CPed* ped, int a, void* b, void* c);
GAME_FN(void, __thiscall, Social_48F1D0, 0x0048F1D0, PedSocial* self, CPed* other, int value, int a, int b);
GAME_FN(void, __thiscall, Social_ClearStimulus, 0x0048F380, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48FB40, 0x0048FB40, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48FD00, 0x0048FD00, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_48FD80, 0x0048FD80, PedSocial* self);
GAME_FN(uint8_t, __thiscall, Social_490AF0, 0x00490AF0, PedSocial* self, CPed* other);
GAME_FN(void, __thiscall, Social_490C40, 0x00490C40, PedSocial* self, int value, uintptr_t a);
GAME_FN(void, __thiscall, Social_Reset, 0x004910A0, PedSocial* self);
GAME_FN(void, __thiscall, Social_4925E0, 0x004925E0, PedSocial* self, CPed* ped, int a, int b, int c);
GAME_FN(void, __thiscall, Social_EndAction, 0x00492680, PedSocial* self);
GAME_FN(void, __thiscall, Social_492720, 0x00492720, PedSocial* self, int a, uintptr_t b);
GAME_FN(uint8_t, __thiscall, Social_RespondToPlayer, 0x00494580, PedSocial* self, int fromInterrupt);
GAME_FN(uint8_t, __thiscall, Social_EngageWithPlayer, 0x00494FF0, PedSocial* self, CPed* target);
// registered pointers
GAME_FN(void, __thiscall, RegisteredPointerAssign, 0x005D7F10, re::Ptr32<CPed>* slot, CPed* entity);
GAME_FN(void, __thiscall, RegisteredPointerCopy, 0x00448230, re::Ptr32<CPed>* slot, re::Ptr32<CPed>* from);
GAME_FN(void, __thiscall, RegisteredPointerAssign472CA0, 0x00472CA0, re::Ptr32<CPed>* slot, CPed* entity);
// peds
GAME_FN(CPed*, __thiscall, Ped_46ECE0, 0x0046ECE0, CPed* ped);
GAME_FN(void, __thiscall, Ped_TakeMoney, 0x00473100, CPed* ped, uint32_t amount);
GAME_FN(void, __thiscall, Ped_GiveMoney, 0x00475320, CPed* ped, uint32_t amount);
GAME_FN(uint8_t, __thiscall, Ped_WantsGift, 0x00474A30, CPed* ped);
GAME_FN(uint8_t, __thiscall, Ped_Has0GiftRequirement, 0x00474B80, CPed* ped);
GAME_FN(int, __thiscall, Ped_GetStat, 0x00477060, CPed* ped, int stat);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, CPed* ped);
GAME_FN(int, __thiscall, Ped_497020, 0x00497020, CPed* ped);
GAME_FN(void, __thiscall, Ped_497520, 0x00497520, CPed* ped, uintptr_t a);
GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, CPed* ped, CPed* target);
GAME_FN(void, __thiscall, Ped_ClearStimulus, 0x0048D710, CPed* ped, void* stimulus);
GAME_FN(uint8_t, __thiscall, Ped_41AA50, 0x0041AA50, CPed* ped);
GAME_FN(uint8_t, __thiscall, Ped_44E3D0, 0x0044E3D0, CPed* ped);
GAME_FN(float, __thiscall, Ped_GetHeading415470, 0x00415470, CPed* ped);
GAME_FN(void, __thiscall, PedGroup_48C480, 0x0048C480, void* group, CPed* ped);
GAME_FN(void, __thiscall, Targeting_4D79E0, 0x004D79E0, void* targeting, CPed* ped);
GAME_FN(void, __thiscall, Intelligence_5F3720, 0x005F3720, void* intelligence, int value);
GAME_FN(uint8_t*, __thiscall, List_GetAt, 0x005F49E0, void* list, int index);
GAME_FN(void, __thiscall, ModelInfo_522380, 0x00522380, void* modelInfo, int* out);
// objectives
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void*, __thiscall, FleeObjective_Construct, 0x0048E700, void* mem, CPed* ped, CPed* from);
GAME_FN(void*, __thiscall, AttackPedObjective_Construct, 0x00470D60, void* mem, CPed* ped, CPed* target, int value);
GAME_FN(uint8_t, __thiscall, ObjectiveQueue_Prepend, 0x00471470, void* queue, void* objective);
// stimuli
GAME_FN(void*, __thiscall, StimulusMgr_Find, 0x004A6150, void* mgr, CPed* ped, int type, int a);
GAME_FN(CPed*, __thiscall, Stimulus_GetSource, 0x004A5CD0, void* stimulus);
GAME_FN(void, __thiscall, StimulusMgr_Add, 0x004A6670, void* mgr, int type, CPed* from, CPed* to,
        const ped::CVector* pos, float a, float b);
GAME_FN(uint8_t, __thiscall, StimulusMgr2_49DAA0, 0x0049DAA0, void* mgr, CPed* ped);
GAME_FN(uint8_t, __cdecl, ConditionIsSpeechBusy, 0x005A2900, CPed* ped, int a, int b);
// the rest
GAME_FN(int, __cdecl, Random, 0x0040DA80, int min, int max);
inline void* __cdecl Memset(void* dest, int value, size_t size) { return crt::Memset(dest, value, size); }
GAME_FN(int, __thiscall, Input_GetActionId, 0x006A9FD0, void* input, const void* name);
GAME_FN(uint8_t*, __thiscall, Input_GetAction, 0x006AA7C0, void* input, int id);
GAME_FN(uint8_t, __thiscall, Input_5F4800, 0x005F4800, void* self, const void* name, int a, int b);
GAME_FN(uint16_t, __cdecl, ClassId48D730, 0x0048D730);
GAME_FN(ped::CVector*, __cdecl, Vector_4129A0, 0x004129A0, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);   // a + b
GAME_FN(ped::CVector*, __cdecl, Vector_Copy412350, 0x00412350, ped::CVector* out, const ped::CVector* in, float scale);
GAME_FN(uint8_t*, __thiscall, Mgr_448E00_x64, 0x00448E00, void* mgr, float radius, float, float, float, float heading);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* Mgr_448E00(void* a0, float a1, ped::CVector a2, float a3) {
    return Mgr_448E00_x64(a0, a1, a2.x, a2.y, a2.z, a3);
}
GAME_FN(void, __thiscall, Obj_447410, 0x00447410, void* self, CPed* ped, int a);
GAME_FN(void, __thiscall, Obj_489A00, 0x00489A00, void* self, CPed* ped, const ped::CVector* pos);

using re::At;

PedSocial* SocialOf(CPed* ped) {
    return reinterpret_cast<PedSocial*>(reinterpret_cast<uint8_t*>(ped) + 0xB44);
}

const ped::CVector* PositionOf(CPed* ped) {
    return ped->matrix ? reinterpret_cast<const ped::CVector*>(ped->matrix + 0x30) : &ped->position;
}

// the ped's current stimulus (+0x1D30), marked handled (+0x2A) and dropped
re::Ptr32<void>& CurrentStimulus(CPed* ped) {
    return At<void*>(ped, 0x1D30);
}

int Clamp4(int value) {
    return value > 4 ? 4 : value;
}

// ---------------------------------------------------------------------------------

// 0x494580 (thiscall): the player (`other`) did something to this ped: count it and
// decide the response. 1: nothing came
uint8_t __fastcall RespondToPlayer(PedSocial* self, re::Edx, int fromInterrupt) {
    CPed* other = self->other;
    if (!other || other->pedType != PEDTYPE_PLAYER) {
        return 1;
    }
    re::Ptr32<void> stimulus = CurrentStimulus(self->ped);
    for (int i = 0; i < 25; i++) {
        int8_t type = self->stimulusTypes[i];
        if (type != 0 && Social_FindStimulus(self, type, &stimulus)) {
            self->stimulusIndex = i;
            self->stimulusType = self->stimulusTypes[i];
            At<uint8_t>(stimulus, 0x2A) = 1;
            if (CurrentStimulus(self->ped) == stimulus) {
                CurrentStimulus(self->ped) = nullptr;
            }
            if (Ped_497020(self->ped)) {
                Ped_497520(self->ped, 0);
            }
            self->time = g_timeMs;
            self->state = 5;
            return 0;
        }
    }

    bool resetRefs = false, keep124 = false, flag31 = false, flag20 = false, flag9 = false,
         flag1C = false, flag38 = false;
    bool interrupt = re::Flag(fromInterrupt);
    if (Social_FindStimulus(self, 0x37, &stimulus) || Social_FindStimulus(self, 0xC, &stimulus)) {
        self->counts[6]++;
        resetRefs = true;
        keep124 = true;
    } else if (Social_FindStimulus(self, 0x31, &stimulus)) {
        self->counts[9]++;
        At<int32_t>(self->ped, 0x1400) = 0;
        flag31 = true;
        At<int32_t>(self->ped, 0x145C) = 0;
        if (Social_48E130(self)) {
            // the others it knows of (+0x3BC: +4 count, +8 them) of its own type too
            uint8_t* list = self->ped->unknown3BC;
            for (int i = 0; i < *reinterpret_cast<int32_t*>(list + 4); i++) {
                CPed* each = reinterpret_cast<re::Ptr32<CPed>*>(list + 8)[i];
                if (each && each->pedType == self->ped->pedType) {
                    At<int32_t>(each, 0x1400) = 0;
                    At<int32_t>(each, 0x145C) = 0;
                }
            }
        }
    } else if (Social_FindStimulus(self, 9, &stimulus)) {
        int pedType = self->ped->pedType;
        resetRefs = true;
        if (pedType == 0 || pedType == 8 || pedType == 7 || !Social_48E130(self) || self->unknownEC > 2 ||
            (self->ref120 != g_playerPed && SocialOf(g_playerPed)->ref120 != self->ped)) {
            self->counts[7]++;
        } else {
            self->counts[8]++;
            At<int32_t>(self->ped, 0x1400) = 0;
            At<int32_t>(self->ped, 0x145C) = 0;
            self->flags225 |= 0x20;
            flag9 = true;
        }
    } else if (!interrupt && Social_FindStimulus(self, 0xA, &stimulus)) {
        if (Ped_GetModelInfo(self->ped)[0x50] != 0 && !Social_48D690(self, self->other)) {
            int kind = 0; // (always written)
            ModelInfo_522380(Ped_GetModelInfo(self->ped), &kind);
            if (kind == 1 || kind == 3) {
                int id = Input_GetActionId(g_input, g_inputName1B65468);
                if (*reinterpret_cast<uint16_t*>(Input_GetAction(g_input, id) + 2) != 0 ||
                    (Input_GetAction(g_input, id)[4] & 8) != 0) {
                    self->flags225 |= 0x40;
                    self->counts[15]++;
                }
            }
        }
        self->counts[18]++;
    } else if (Social_FindStimulus(self, 0x1C, &stimulus)) {
        self->counts[5]++;
        flag1C = true;
    } else if (!interrupt && Social_FindStimulus(self, 0x1F, &stimulus)) {
        self->counts[1]++;
        resetRefs = true;
    } else if (!interrupt && Social_FindStimulus(self, 0x39, &stimulus)) {
        self->counts[16]++;
    } else if (!interrupt && Social_FindStimulus(self, 0x23, &stimulus)) {
        self->counts[15]++;
    } else if (!interrupt && Social_FindStimulus(self, 0x20, &stimulus)) {
        flag20 = true;
        if (Social_48E1F0(self)) {
            self->counts[2]++;
            At<int32_t>(self->ped, 0x1400) = 0;
            At<int32_t>(self->ped, 0x145C) = 0;
            self->flags225 |= 0x10;
        } else {
            self->counts[22]++;
        }
    } else if (!interrupt && Social_FindStimulus(self, 0x22, &stimulus)) {
        self->counts[17]++;
        resetRefs = true;
    } else if (Social_FindStimulus(self, 0x38, &stimulus)) {
        self->counts[3]++;
        flag38 = true;
    } else {
        return 1;
    }

    At<uint8_t>(stimulus, 0x2A) = 1;
    if (CurrentStimulus(self->ped) == stimulus) {
        CurrentStimulus(self->ped) = nullptr;
    }
    other = self->other;
    bool was124 = self->ref124 == other;
    self->time = g_timeMs;
    self->state = 5;

    if (resetRefs) {
        RegisteredPointerAssign(&self->ref120, other);
        RegisteredPointerAssign(&self->ref128, nullptr);
        if (Social_48E130(self) && flag9) {
            self->response = 8;
            return 0;
        }
        if (keep124 || flag31) {
            RegisteredPointerAssign(&self->ref124, self->other);
            self->response = 0;
            At<uint8_t>(self->other, 0x5D8) = 1;
            if (At<int32_t>(self->ped, 0x1464) != 0) {
                goto action13;
            }
            if (!Social_48E2B0(self)) {
                if (Ped_GetStat(self->ped, 7) > 0 && (self->flags225 & 8) && self->ped->seatState == 1 &&
                    At<int32_t>(self->ped, 0x141C) == 0) {
                    goto action14;
                }
                goto action13;
            }
        } else {
            self->response = 9;
        }
        uint8_t flags = self->flags224;
        if ((flags & 0x40) || ((self->flags226 & 2) && (self->unknown12C < 4 || At<int32_t>(self->ped, 0x141C) != 0))) {
            self->flags224 = flags | 0x80;
            Social_EndAction(self);
            return 0;
        }
    } else if (flag1C) {
        self->response = 5;
        if ((self->flags224 & 0x40) || (self->flags225 & 2) ||
            (self->ped->unknown1E28 > 0 && At<int32_t>(self->ped, 0x1430) == 1)) {
            Social_EndAction(self);
            return 0;
        }
    } else if (flag20) {
        RegisteredPointerAssign(&self->ref128, other);
        self->response = Social_48E1F0(self) ? 2 : 0x16;
        return 0;
    } else if (flag31) {
        RegisteredPointerAssign(&self->ref120, other);
        RegisteredPointerAssign(&self->ref124, self->other);
        if (Ped_GetStat(self->ped, 7) > 0 && (self->flags225 & 8) && self->ped->seatState == 1 &&
            At<int32_t>(self->ped, 0x141C) == 0) {
            goto action14;
        }
        if (Ped_GetStat(self->ped, 7) > 0) {
            goto action13;
        }
        goto ended;
    } else {
        self->response = flag38 ? 3 : 0x10;
    }

    {
        if (self->flags225 & 0x30) {
            return 0;
        }
        // how well it's going: what each did, scored by how often (the 5th time on the same)
        int sign = Social_48D770(self, 0) >= 0 ? 1 : -1;
        int score = 0, cost = 0;
        for (int kind = 0; kind < 26; kind++) {
            int count = self->counts[kind];
            if (count > 0) {
                int n = Clamp4(count - 1);
                if (kind == 8) {
                    score -= g_socialScore[8 * 5 + Clamp4(self->counts[8] - 1)];
                } else {
                    score += g_socialScore[kind * 5 + n] * sign;
                }
                cost += g_socialCost[kind * 5 + n];
            }
        }
        const int threshold = g_socialThreshold[self->ped->pedType];
        if (!(self->flags225 & 4) && !flag20) {
            if (self->unknown12C != -1) {
                return 0;
            }
            if (Social_48D5F0(self) || Social_48D620(self)) {
                bool likes = (score - cost >= threshold && Social_48D580(self) && !Social_48D5C0(self) &&
                              self->ref120 == self->other) ||
                             (keep124 && was124);
                if (likes && self->unknownEC <= 1) {
                    int stat = Ped_GetStat(self->ped, 0xE);
                    if (Random(0, 100) < stat + 10) {
                        self->flags224 |= 0x80;
                        Social_EndAction(self);
                        return 0;
                    }
                }
            }
        }
        if (self->unknown12C != -1) {
            return 0;
        }
        if (!((self->ref124 == self->other && self->unknownEC <= 1) || (self->flags224 & 0x40) ||
              (self->flags225 & 2) || Social_48FB40(self) || self->lastAction == 0x12 ||
              cost >= int(g_timeMs - self->startTime) / 3000 - g_socialThreshold[self->ped->pedType])) {
            if (Random(0, 100) < 0x21) {
                goto ended;
            }
        }
        if (self->unknown12C != -1 || (self->flags224 & 0x40) || (self->flags225 & 2) || Social_48FB40(self)) {
            return 0;
        }
        uint32_t start = self->startTime;
        uint32_t now = g_timeMs;
        if (start + 30000 >= now) {
            if (Social_48D580(self)) {
                return 0;
            }
            if (cost >= int(now - start) / 2000 - g_socialThreshold[self->ped->pedType]) {
                return 0;
            }
            if (Random(0, 100) >= 0x42) {
                return 0;
            }
        }
        if (!Social_48D580(self) && cost > 0 && cost > score) {
            self->flags224 |= 8;
            Social_EndAction(self);
            return 0;
        }
    }
ended:
    self->flags225 |= 1;
    Social_EndAction(self);
    return 0;
action13:
    self->action = 0x13;
    self->lastAction = 0x13;
    self->state = 3;
    return 0;
action14:
    self->action = 0x14;
    self->lastAction = 0x14;
    self->state = 3;
    return 0;
}

// 0x494FF0 (thiscall): the player's: start socialising with `target` (none: its
// partner, or its target), unless that one is busy with someone else. Returns 0
uint8_t __fastcall EngageWithPlayer(PedSocial* self, re::Edx, CPed* target) {
    CPed* ped = self->ped;
    if (ped->pedType != PEDTYPE_PLAYER || ped->unknown1E91 != 0) {
        return 0;
    }
    CPed* who = target;
    if (!who) {
        who = ped->partner;
        if (!who) {
            who = Ped_46ECE0(ped);
        }
    }
    RegisteredPointerAssign(&self->other, who);
    if (!self->other) {
        return 0;
    }
    PedSocial* its = SocialOf(self->other);
    its->unknownF8 = g_timeMs;
    CPed* other = self->other;
    if (At<int32_t>(other, 0x1428) != 0) {
        return 0;
    }
    its = SocialOf(other);
    if (its->state != 0 && its->other == self->ped) {
        return 0;
    }
    if ((its->flags224 & 0x10) && its->unknown74 != self->ped) {
        return 0;
    }
    if (Social_490AF0(self, other)) {
        return 0;
    }
    if (!Ped_WantsGift(self->other)) {
        Ped_Has0GiftRequirement(self->other);
    }
    Intelligence_5F3720(self->ped->intelligence, 1);
    int value = 0; // (always written; the game uses its argument's stack slot)
    Social_48EBC0(SocialOf(self->other), &value, self->ped, 1, nullptr, nullptr);
    its = SocialOf(self->other);
    if (Social_48D6F0(its)) {
        Social_Reset(its);
    }
    its = SocialOf(self->other);
    if (its->unknown12C != -1 && its->unknown74 == self->ped) {
        its->unknownEC = its->unknown12C;
    } else {
        its->unknownEC = value;
    }
    SocialOf(self->other)->startTime = g_timeMs;
    Memset(self->counts, 0, sizeof(self->counts));
    Memset(SocialOf(self->other)->counts, 0, sizeof(self->counts));
    SocialOf(self->other)->flags224 &= 0xFB;
    SocialOf(self->other)->time = g_timeMs;
    SocialOf(self->other)->state = 2;
    RegisteredPointerAssign472CA0(&SocialOf(self->other)->other, self->ped);
    SocialOf(self->other)->flags224 |= 2;
    SocialOf(self->other)->flags225 |= 0x80;
    SocialOf(self->other)->duration = 0xED8;
    its = SocialOf(self->other);
    Social_492720(its, its->unknownEC > 3 ? 1 : 0, 0);
    its = SocialOf(self->other);
    Social_490C40(its, its->unknownEC, 0);
    its = SocialOf(self->other);
    if (its->unknownEC >= 4 && its->unknownEC <= 6 && !Social_48D690(its, self->ped)) {
        its->unknownEC = 7;
    }
    its = SocialOf(self->other);
    its->unknown1FC = its->unknownEC;
    return 0;
}

// 0x495220 (thiscall): the player's: it bumped into `ped` (BUMPSHOVE): engage it, and
// tell it (stimulus 0x38) unless it's already been told
void __fastcall PlayerBumped(PedSocial* self, re::Edx, CPed* ped) {
    if (self->ped->pedType != PEDTYPE_PLAYER || !ped) {
        return;
    }
    Social_EngageWithPlayer(self, ped);
    ped::CVector position = *PositionOf(self->ped);
    if (!StimulusMgr2_49DAA0(g_stimulusMgr2, ped)) {
        StimulusMgr_Add(g_stimulusMgr, 0x38, self->ped, ped, &position, -1.0f, -1.0f);
    }
}

// An entry of the intelligence's list (+0x290) wants the input (0x1B6544C): the
// socialising gives way
uint8_t EntryTakesInput(uint8_t* entry, uint32_t classId) {
    auto isA = reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entry))[0]));
    if (!isA(entry, classId)) {
        return 0;
    }
    void* input = entry + 0x84;
    return input && Input_5F4800(input, g_inputName1B6544C, 1, 1);
}

// 0x4952B0 (thiscall, SocializeTrack vf3): socialising goes on. 1: it carries on
uint8_t __fastcall UpdateSocialize(PedSocial* self) {
    int state = self->state;
    self->flags224 |= 4;
    CPed* other = self->other;
    if (state != 0 && other && other->pedType != PEDTYPE_PLAYER && state != 3) {
        // (the count is the other's list's, the entries this ped's own)
        int count = At<int32_t>(other->intelligence, 0x2C4);
        for (int i = 0; i < count; i++) {
            uint8_t* entry = List_GetAt(self->ped->intelligence + 0x290, i);
            if (entry) {
                // (the game pushes eax whole: the id with the entry's upper half above it)
                uint32_t classId = ClassId48D730();
                if (EntryTakesInput(entry, classId)) {
                    return 1;
                }
            }
        }
    }
    if (self->state != 4 && self->state != 2) {
        return 0;
    }
    int limit = self->duration;
    if (self->ped->partner) {
        self->time = g_timeMs;
    } else {
        // (the time the player spends talking doesn't count)
        CPed* player = self->other;
        if (player && player->pedType == PEDTYPE_PLAYER && ConditionIsSpeechBusy(player, -1, -1)) {
            self->time += g_timeMs - g_previousTimeMs;
        }
    }
    if (Social_48FB40(self)) {
        limit = 30000;
    }
    other = self->other;
    if (!other) {
        Social_Reset(self);
        return 0;
    }
    int pedType = other->pedType;
    uint32_t now = g_timeMs;
    if (pedType != PEDTYPE_PLAYER) {
        PedSocial* its = SocialOf(other);
        if ((its->state == 0 || !its->other || (its->other != self->ped && self->unknown12C == -1)) &&
            uint32_t(self->duration / 2 + self->time) < now) {
            self->flags225 |= 1;
            Social_EndAction(self);
            return 0;
        }
        return 1;
    }
    if (uint32_t(self->time + limit) >= now) {
        return Social_RespondToPlayer(self, 0);
    }
    uint8_t flags = self->flags226;
    if (flags & 2) {
        if (self->unknown134 != 2) {
            if (self->state == 2) {
                self->time = now;
                self->state = 5;
            }
            return 0;
        }
        int value = self->unknown12C;
        bool agreed;
        if (value != -1) {
            agreed = value >= 4 && value <= 6;
        } else {
            agreed = self->unknownEC >= 4 && self->unknownEC <= 6;
        }
        if (agreed) {
            self->flags225 |= 1;
            self->flags226 = flags & 0xFD;
        } else {
            self->flags224 |= 0x80;
        }
        Social_EndAction(self);
        return 0;
    }
    if (self->state == 2 && self->unknown12C != -1) {
        self->time = now;
        self->state = 5;
        return 0;
    }
    if (flags & 8) {
        self->flags225 |= 1;
        self->time = now;
        self->state = 5;
        return 0;
    }
    if (self->flags224 & 0x40) {
        self->state = 3;
        self->action = 6;
        self->lastAction = 6;
        return 0;
    }
    self->flags225 |= 1;
    Social_EndAction(self);
    return 0;
}

// the stimulus handled, and the ped's current one dropped if it's that one
void HandleStimulus(CPed* ped, void* stimulus) {
    At<uint8_t>(stimulus, 0x2A) = 1;
    Ped_ClearStimulus(ped, stimulus);
}

// 0x495590 (thiscall, SocializeActionCompleteTrack vf3): an action finished: what
// comes of it (fleeing, a fight, money, walking together, another round). 1: carry on
uint8_t __fastcall UpdateActionComplete(PedSocial* self) {
    uint8_t flags = self->flags224;
    self->flags225 &= 0x7F;
    self->flags226 &= 0xFE;
    const bool wasUpdating = ((flags >> 2) & 1) != 0;
    self->flags224 = flags | 4;

    re::Ptr32<void> stimulus = nullptr;
    bool handled = false, resetRefs = false, flag37 = false, flag31 = false, flag38 = false, flag1C = false;
    bool bored = false;
    int action = 0;
    CPed* other = nullptr;
    PedSocial* its = nullptr;
    void* objective = nullptr;
    int sign1 = 0, sign2 = 0, ownScore = 0, otherScore = 0, ownCost = 0, otherCost = 0;
    uint32_t now = 0;

    if (self->state != 0) {
        int count = At<int32_t>(self->ped->intelligence, 0x2C4);
        for (int i = 0; i < count; i++) {
            uint8_t* entry = List_GetAt(self->ped->intelligence + 0x290, i);
            if (entry) {
                // (0x48D730 inlined)
                if (!(g_classIdGuard & 1)) {
                    g_classIdGuard |= 1;
                    g_classId48D730 = g_nextClassId;
                    g_nextClassId = uint16_t(g_nextClassId + 1);
                }
                if (EntryTakesInput(entry, g_classId48D730)) {
                    return 1;
                }
            }
        }
        if (At<int32_t>(self->ped, 0x1494) != 0 && !wasUpdating) {
            return 1;
        }
    }

    stimulus = CurrentStimulus(self->ped);
    if (self->other && self->other->pedType == PEDTYPE_PLAYER) {
        if (Social_FindStimulus(self, 0x37, &stimulus)) {
            self->counts[6]++;
            handled = resetRefs = flag37 = true;
        } else if (Social_FindStimulus(self, 9, &stimulus)) {
            self->counts[7]++;
            handled = resetRefs = true;
        } else if (Social_FindStimulus(self, 0x35, &stimulus)) {
            handled = true;
        } else if (Social_FindStimulus(self, 0x1C, &stimulus)) {
            self->counts[5]++;
            handled = flag1C = true;
        } else if (Social_FindStimulus(self, 0x1F, &stimulus)) {
            self->counts[1]++;
            handled = resetRefs = true;
        } else if (Social_FindStimulus(self, 0x22, &stimulus)) {
            self->counts[17]++;
            handled = resetRefs = true;
        } else if (Social_FindStimulus(self, 0x31, &stimulus)) {
            self->counts[9]++;
            handled = flag31 = resetRefs = true;
        } else if (Social_FindStimulus(self, 0x38, &stimulus)) {
            self->counts[3]++;
            handled = flag38 = true;
            self->response = 3;
        }
        if (resetRefs) {
            RegisteredPointerAssign(&self->ref128, nullptr);
            RegisteredPointerAssign(&self->ref120, self->other);
            if (flag37 || flag31) {
                RegisteredPointerAssign(&self->ref124, self->other);
                self->response = 0;
                At<uint8_t>(self->other, 0x5D8) = 1;
                if (!Social_48E2B0(self) && self->action != 0x15 && self->action != 0x14 && self->action != 0x13) {
                    RegisteredPointerCopy(&self->ref124, &self->other);
                    if (Ped_GetStat(self->ped, 7) > 0 && (self->flags225 & 8) && self->ped->seatState == 1 &&
                        At<int32_t>(self->ped, 0x141C) == 0) {
                        self->action = 0x14;
                        self->lastAction = 0x14;
                    } else {
                        self->action = 0x13;
                        self->lastAction = 0x13;
                    }
                    self->state = 3;
                    return 0;
                }
            } else {
                self->response = 9;
            }
        }
    }

    action = self->action;
    if (action == 3 || action == 9 || action == 0xE || action == 0x17) {
        if (handled) {
            At<uint8_t>(stimulus, 0x2A) = 1;
            if (CurrentStimulus(self->ped) == stimulus) {
                CurrentStimulus(self->ped) = nullptr;
            }
        }
        if (resetRefs) {
            if (self->action == 9 || self->action == 0x17) {
                goto refuse;
            }
            goto finished;
        }
        if (flag38) {
            goto finished;
        }
        if (flag1C) {
            self->response = 5;
            if ((self->flags224 & 0x40) || (self->flags225 & 2) ||
                (self->ped->unknown1E28 > 0 && At<int32_t>(self->ped, 0x1430) == 1)) {
                goto giveUp;
            }
        }
        if (At<int32_t>(self->ped, 0x141C) != 0) {
            if (self->action != 0x17) {
                goto noRef;
            }
            if (self->other && self->other->pedType == PEDTYPE_PLAYER) {
                RegisteredPointerCopy(&self->ref128, &self->other);
                Social_48F1D0(self, self->other, self->unknown1FC, 0, (self->flags226 >> 2) & 1);
                self->flags226 &= 0xFD;
                self->unknownEC = self->unknown1FC;
                goto respond;
            }
        }
        if (self->action == 0x17) {
            RegisteredPointerAssign(&self->ref128, self->other);
        }
    noRef:
        if (!(self->flags225 & 0x10)) {
            Social_48F1D0(self, self->other, self->unknown1FC, 0, 1);
            self->unknownEC = self->unknown1FC;
        }
        if (self->other) {
            Social_ClearStimulus(SocialOf(self->other));
        }
        {
            CPed* ped = self->ped;
            void* current = CurrentStimulus(ped);
            if (current && At<CPed*>(current, 8) == ped && self->other && At<CPed*>(current, 4) == self->other) {
                CurrentStimulus(ped) = nullptr;
                At<uint8_t>(current, 0x2A) = 1;
            }
        }
        goto reset;
    }

    if (action == 0x13) {
        if (!self->other) {
            goto respond;
        }
        self->time = g_timeMs;
        RegisteredPointerCopy(&self->ref138, &self->other);
        objective = Malloc(0x14);
        objective = objective ? FleeObjective_Construct(objective, self->ped, self->other) : nullptr;
        goto prepend;
    }
    if (action == 0x14) {
        if (!self->other) {
            goto respond;
        }
        // (they go off together)
        self->time = g_timeMs;
        Social_ClearStimulus(SocialOf(self->other));
        Social_ClearStimulus(self);
        PedGroup_48C480(self->ped->unknown3BC, self->other);
        Ped_SetTargetEntity(self->ped, self->other);
        Targeting_4D79E0(self->ped->targeting, self->other);
        Social_Reset(self);
        return 0;
    }
    if (action == 0x15 && self->other) {
        objective = Malloc(0x1C);
        objective = objective ? AttackPedObjective_Construct(objective, self->ped, self->other, 2) : nullptr;
        goto prepend;
    }
    if (action == 5 && self->other) {
        // money to the other (by how it went)
        uint32_t amount = 0;
        if (self->unknownEC == 6) {
            amount = 100;
        } else if (self->unknownEC == 5) {
            amount = 500;
        } else if (self->unknownEC == 4) {
            amount = 1000;
        }
        if (amount) {
            Ped_TakeMoney(self->ped, amount);
            Ped_GiveMoney(self->other, amount);
        }
        self->time = g_timeMs;
        objective = Malloc(0x14);
        objective = objective ? FleeObjective_Construct(objective, self->ped, self->other) : nullptr;
        ObjectiveQueue_Prepend(&self->ped->objectives, objective);
        Social_ClearStimulus(SocialOf(self->other));
        Social_ClearStimulus(self);
        other = self->other;
        if (other->pedType != PEDTYPE_PLAYER && SocialOf(other)->other == self->ped) {
            SocialOf(other)->flags225 |= 1;
            Social_EndAction(SocialOf(self->other));
            Social_Reset(self);
            return 0;
        }
        goto reset;
    }
    if ((action == 0x10 || action == 0xF) && self->other) {
        uint8_t its44E3D0 = Ped_44E3D0(self->other);
        CPed* ped = self->ped;
        if ((Ped_44E3D0(ped) == its44E3D0 || ped->pedType == self->other->pedType) &&
            self->other->pedType != PEDTYPE_PLAYER) {
            // halfway between them, facing the ped's way
            const ped::CVector* otherPos = PositionOf(self->other);
            const ped::CVector* ownPos = PositionOf(self->ped);
            ped::CVector half, middle;
            Vector_Copy412350(&middle, Vector_4129A0(&half, ownPos, otherPos), 0.5f);   // (the 0.5 is the scale's: the game leaves it on the stack across the add)
            float heading = Ped_GetHeading415470(self->ped);
            uint8_t* made = Mgr_448E00(g_mgrC0F5E4, 6.0f, middle, heading);
            At<int32_t>(made, 0x70) = 2;
            Obj_447410(made, self->other, 1);
            Obj_447410(made, self->ped, 1);
            Obj_489A00(At<void*>(made, 0x6C), self->other, PositionOf(self->other));
            Obj_489A00(At<void*>(made, 0x6C), self->ped, PositionOf(self->ped));
            Social_ClearStimulus(SocialOf(self->other));
            Social_ClearStimulus(self);
            Social_Reset(SocialOf(self->other));
            Social_Reset(self);
            return 0;
        }
    }
    if (action == 0x10 && self->other && self->other->pedType == PEDTYPE_PLAYER) {
        if (handled) {
            HandleStimulus(self->ped, stimulus);
        }
        if (resetRefs || flag38) {
            goto finished;
        }
        if (flag1C) {
            self->response = 5;
            if ((self->flags224 & 0x40) || (self->flags225 & 2) ||
                (self->ped->unknown1E28 > 0 && At<int32_t>(self->ped, 0x1430) == 1)) {
                goto giveUp;
            }
        }
        self->flags225 |= 1;
        Social_EndAction(self);
        if (self->other->pedType == PEDTYPE_PLAYER) {
            return 0;
        }
        SocialOf(self->other)->flags225 |= 1;
        Social_EndAction(SocialOf(self->other));
        return 0;
    }
    if (action == 6) {
        if (handled) {
            HandleStimulus(self->ped, stimulus);
        }
        if (resetRefs) {
            goto refuse;
        }
        if (flag38) {
            goto finished;
        }
        if ((self->flags224 & 1) && self->other && self->other->pedType == PEDTYPE_PLAYER) {
            goto reset;
        }
        self->action = 9;
        self->lastAction = 9;
        other = self->other;
        self->state = 3;
        if (!other || other->pedType == PEDTYPE_PLAYER) {
            return 0;
        }
        SocialOf(other)->flags225 |= 1;
        Social_EndAction(SocialOf(self->other));
        return 0;
    }
    if (action == 4 && (At<int32_t>(self->ped, 0x1464) == 0 || self->unknown140 != 0 || self->ped->seatState == 2)) {
        if (self->other) {
            Social_ClearStimulus(SocialOf(self->other));
        }
        Social_ClearStimulus(self);
        Social_Reset(self);
        return 0;
    }
    flags = self->flags226;
    if ((flags & 2) && self->unknown134 == 3) {
        int value = self->unknown12C;
        bool agreed;
        if (value != -1) {
            agreed = value >= 4 && value <= 6;
        } else {
            agreed = self->unknownEC >= 4 && self->unknownEC <= 6;
        }
        if (!agreed) {
            goto refuse;
        }
        self->flags225 |= 1;
        self->flags226 = flags & 0xFD;
        Social_EndAction(self);
        return 0;
    }
    if ((self->flags224 & 1) && self->other && self->other->pedType == PEDTYPE_PLAYER) {
        if (self->ped->seatState != 1 || Ped_41AA50(self->ped) || Social_RespondToPlayer(self, 0)) {
            goto reset;
        }
        self->flags224 &= 0xFE;
        return 0;
    }

respond:
    other = self->other;
    if (!other) {
        goto clear;
    }
    if (self->unknown12C == -1) {
        CPed* itsOther = SocialOf(other)->other;
        if ((itsOther && itsOther != self->ped) || other->unknown1C98 != 0) {
            self->flags225 |= 1;
            Social_EndAction(self);
            return 0;
        }
    }
    if (other->pedType == PEDTYPE_PLAYER) {
        self->time = g_timeMs;
        self->state = self->state != 3 ? 2 : 5;
        return 0;
    }
    if (self->state == 1) {
        self->time = g_timeMs;
        self->state = 2;
        SocialOf(other)->state = 3;
        return 0;
    }
    if (self->state != 3) {
        return 0;
    }
    if (self->unknown12C == -1 &&
        (self->startTime + 30000 < g_timeMs || (SocialOf(other)->flags224 & 1) || (self->flags224 & 1))) {
        Social_ClearStimulus(SocialOf(other));
        Social_ClearStimulus(self);
        Social_Reset(SocialOf(self->other));
        Social_Reset(self);
        return 0;
    }

    // how it went, from each side: its own actions scored, and the cost of both (the
    // other's at half)
    sign1 = Social_48D770(self, 0) >= 0 ? 1 : -1;
    sign2 = Social_48D770(SocialOf(self->other), 0) >= 0 ? 1 : -1;
    its = SocialOf(self->other);
    for (int kind = 0; kind < 26; kind++) {
        int count = self->counts[kind];
        if (count > 0) {
            int n = Clamp4(count - 1);
            ownScore += g_socialScore[kind * 5 + n] * sign1;
            ownCost += g_socialCost[kind * 5 + n];
            otherCost += g_socialCost[kind * 5 + n] / 2;
        }
        count = its->counts[kind];
        if (count > 0) {
            int n = Clamp4(count - 1);
            otherScore += g_socialScore[kind * 5 + n] * sign2;
            otherCost += g_socialCost[kind * 5 + n];
            ownCost += g_socialCost[kind * 5 + n] / 2;
        }
    }
    // it went well enough for this one
    if (!(self->flags225 & 4) && self->unknown12C == -1 &&
        (Social_48D580(self) || Social_48D580(SocialOf(self->other))) && self->ref120 == self->other &&
        ownScore - ownCost >= g_socialThreshold[self->ped->pedType] && self->unknownEC <= 0 && self->unknown118 <= 0) {
        int stat = Ped_GetStat(self->ped, 0xE);
        if (Random(0, 100) < stat) {
            self->flags224 |= 0x80;
            Social_EndAction(self);
            its = SocialOf(self->other);
            if (its->unknownEC <= 3) {
                its->flags224 |= 0x80;
            }
            Social_EndAction(SocialOf(self->other));
            return 0;
        }
    }
    // for the other
    other = self->other;
    its = SocialOf(other);
    if (!(its->flags225 & 4) && its->unknown12C == -1 && (Social_48D580(self) || Social_48D580(its)) &&
        its->ref120 == self->ped && otherScore - otherCost >= g_socialThreshold[other->pedType] &&
        its->unknownEC <= 0 && its->unknown118 <= 0) {
        int stat = Ped_GetStat(self->other, 0xE);
        if (Random(0, 100) < stat) {
            SocialOf(self->other)->flags224 |= 0x80;
            Social_EndAction(SocialOf(self->other));
            if (self->unknownEC <= 3) {
                self->flags224 |= 0x80;
            }
            Social_EndAction(self);
            return 0;
        }
    }
    // one of them has had enough
    if (self->unknown12C == -1 && self->action != 0x12) {
        if (Social_48D5C0(self) && Random(0, 100) < 0x19) {
            goto bothEnd;
        }
        now = g_timeMs;
        if (ownCost < int(now - self->startTime) / 3000 - g_socialThreshold[self->ped->pedType]) {
            if (Random(0, 100) < 0x21) {
                goto bothEnd;
            }
            now = g_timeMs;
        }
    } else {
        now = g_timeMs;
    }
    if (SocialOf(self->other)->unknown12C == -1 && self->action != 0x12) {
        if (Social_48D5C0(SocialOf(self->other))) {
            if (Random(0, 100) < 0x19) {
                goto bothEnd;
            }
            now = g_timeMs;
        }
        if (otherCost < int(now - self->startTime) / 3000 - g_socialThreshold[self->other->pedType]) {
            if (Random(0, 100) < 0x21) {
                goto bothEnd;
            }
            now = g_timeMs;
        }
    }
    // another round; bored of it after a while
    if (!Social_48D580(self) && !Social_48D580(SocialOf(self->other)) && ownCost > 2 && otherCost > 2) {
        if (ownCost < int(now - self->startTime) / 2000 - g_socialThreshold[self->ped->pedType]) {
            if (Random(0, 100) < 0x4B) {
                bored = true;
            } else {
                now = g_timeMs;
            }
        }
        if (!bored && otherCost < int(now - self->startTime) / 2000 - g_socialThreshold[self->other->pedType] &&
            Random(0, 100) < 0x4B) {
            bored = true;
        }
        if (bored) {
            self->flags224 |= 8;
            SocialOf(self->other)->flags224 |= 8;
        }
    }
    self->state = 6;
    SocialOf(self->other)->state = 5;
    return 0;

bothEnd:
    self->flags225 |= 1;
    Social_EndAction(self);
    SocialOf(self->other)->flags225 |= 1;
    Social_EndAction(SocialOf(self->other));
    return 0;
prepend:
    ObjectiveQueue_Prepend(&self->ped->objectives, objective);
    Social_ClearStimulus(SocialOf(self->other));
clear:
    Social_ClearStimulus(self);
    Social_Reset(self);
    return 0;
refuse:
    self->flags224 |= 0x80;
    Social_EndAction(self);
    return 0;
finished:
    self->flags225 &= 0xFE;
    self->time = g_timeMs;
    self->state = 5;
    return 0;
giveUp:
    self->flags225 &= 0xFE;
    Social_EndAction(self);
    return 0;
reset:
    Social_Reset(self);
    return 0;
}

// 0x4967A0 (thiscall, ConditionSocialActionInterrupted vf1): the player hit one of the
// two (stimulus 10, else 9): the one hit fights the player and the other stops. 1: not
// interrupted
uint8_t __fastcall CheckInterrupted(PedSocial* self) {
    CPed* other = self->other;
    CPed* target = nullptr;
    CPed* second = nullptr;
    void* stimulus = nullptr;
    CPed* source = nullptr;
    if (other && other->pedType == PEDTYPE_PLAYER && Social_48D650(self)) {
        uint8_t result = Social_RespondToPlayer(self, 1);
        if (!result) {
            self->flags224 &= 0xFE;
            return result;
        }
        goto notInterrupted;
    }
    if (Social_48FD00(self)) {
        target = other;
        second = self->ped;
    } else if (Social_48FD80(self)) {
        target = self->ped;
        second = other;
    } else {
        goto notInterrupted;
    }
    if (!target) {
        goto notInterrupted;
    }
    stimulus = StimulusMgr_Find(g_stimulusMgr, g_playerPed, 10, -1);
    if (stimulus) {
        source = Stimulus_GetSource(stimulus);
        if (source == target) {
            if (second) {
                PedSocial* its = SocialOf(second);
                Social_ClearStimulus(its);
                Social_Reset(its);
                Social_4925E0(its, g_playerPed, 0, 0x19, 0);
            }
            SocialOf(target)->flags225 |= 1;
            Social_EndAction(SocialOf(target));
            return 0;
        }
        if (!second || source != second) {
            goto notInterrupted;
        }
    } else {
        stimulus = StimulusMgr_Find(g_stimulusMgr, g_playerPed, 9, -1);
        if (!stimulus) {
            goto notInterrupted;
        }
        source = Stimulus_GetSource(stimulus);
        if (!second || source != second) {
            goto notInterrupted;
        }
    }
    {
        PedSocial* its = SocialOf(second);
        Social_ClearStimulus(its);
        Social_Reset(its);
        Social_4925E0(its, g_playerPed, 0, 0x19, 0);
        Social_EngageWithPlayer(SocialOf(g_playerPed), second);
        SocialOf(target)->flags225 |= 1;
        Social_EndAction(SocialOf(target));
        return 0;
    }
notInterrupted:
    At<int32_t>(self->ped, 0x1494) = 1;
    return 1;
}

} // namespace

#define C08P2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P2_REVERSED("PedSocial::RespondToPlayer", 0x00494580, &RespondToPlayer);
C08P2_REVERSED("PedSocial::EngageWithPlayer", 0x00494FF0, &EngageWithPlayer);
C08P2_REVERSED("PedSocial::PlayerBumped", 0x00495220, &PlayerBumped);
C08P2_REVERSED("PedSocial::UpdateSocialize", 0x004952B0, &UpdateSocialize);
C08P2_REVERSED("PedSocial::UpdateActionComplete", 0x00495590, &UpdateActionComplete);
C08P2_REVERSED("PedSocial::CheckInterrupted", 0x004967A0, &CheckInterrupted);
