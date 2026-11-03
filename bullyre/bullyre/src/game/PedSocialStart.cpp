// A ped's social state (the ped's +0xB44), part 1: starting a social exchange with
// someone (0x492AE0: the reaction worked out from the ped type tables, the models'
// ranges, what each already did to the other), responding to being hit (0x494290), and
// the hurt check (0x492AA0). See also PedSocialize.cpp.
//
// The reaction is looked up in an 11x11 grid (0xA349E0) by two scores each clamped
// to -5..5: the first from how the ped types like each other, the second from how
// they fear (or respect) each other (the names are guesses).

#include <cstddef>
#include <cstdint>

#include "game/Ped.h"
#include "game/Pools.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct PedSocial {
    re::Ptr32<CEntity> entity;              // +0x0 its ped as an entity (+0x14 matrix, +4 position)
    re::Ptr32<CPed> ped;                    // +0x4 its ped (this is the ped's +0xB44)
    uint8_t unknown8_[0x64];
    int32_t state;                // +0x6C (0 none, 1 started, 2, 3, 4 engaged by another, 5, 6)
    re::Ptr32<CPed> partner;                // +0x70 who it's with (registered)
    re::Ptr32<CPed> lastPartner;            // +0x74
    int32_t counts[26];           // +0x78 how often each kind of action happened (cleared together)
    int32_t action;               // +0xE0 the last kind done
    int32_t lastAction;           // +0xE4
    int32_t response;             // +0xE8
    int32_t reaction;             // +0xEC from the grid (7: refused)
    uint32_t startTime;           // +0xF0 (CTimer ms)
    uint32_t time;                // +0xF4 (CTimer ms)
    uint8_t unknownF8_[0x4];
    uint32_t unknownFC;           // +0xFC (CTimer ms)
    int32_t unknown100;           // +0x100 added to the first score
    int32_t unknown104;           // +0x104 added to the second
    int32_t unknown108;           // +0x108
    int32_t unknown10C;           // +0x10C
    int32_t unknown110;           // +0x110 (with flags224 & 0x20, towards lastPartner)
    int32_t unknown114;           // +0x114
    int32_t unknown118;           // +0x118 the model difference (0x522380)
    re::Ptr32<CPed> ref11C;                 // +0x11C (registered)
    re::Ptr32<CPed> ref120;                 // +0x120 (registered)
    re::Ptr32<CPed> ref124;                 // +0x124 (registered)
    re::Ptr32<CPed> ref128;                 // +0x128 (registered)
    int32_t forcedReaction;       // +0x12C (-1: none; used with lastPartner)
    int32_t unknown130;           // +0x130 (-1: none)
    uint8_t unknown134_[0x4];
    re::Ptr32<CPed> ref138;                 // +0x138 (registered)
    uint8_t unknown13C_[0xB8];
    int32_t unknown1F4;           // +0x1F4 (0x19: none)
    uint8_t unknown1F8_[0x4];
    int32_t unknown1FC;           // +0x1FC
    int32_t unknown200;           // +0x200 (-1: none)
    uint8_t unknown204_[0x20];
    uint8_t flags224;             // +0x224
    uint8_t flags225;             // +0x225 (0x80: started by the other)
    uint8_t flags226;             // +0x226
    uint8_t unknown227_[5];
};
static_assert(offsetof(PedSocial, state) == 0x6C);
static_assert(offsetof(PedSocial, counts) == 0x78);
static_assert(offsetof(PedSocial, action) == 0xE0);
static_assert(offsetof(PedSocial, reaction) == 0xEC);
static_assert(offsetof(PedSocial, unknownFC) == 0xFC);
static_assert(offsetof(PedSocial, unknown118) == 0x118);
static_assert(offsetof(PedSocial, ref11C) == 0x11C);
static_assert(offsetof(PedSocial, forcedReaction) == 0x12C);
static_assert(offsetof(PedSocial, ref138) == 0x138);
static_assert(offsetof(PedSocial, unknown1F4) == 0x1F4);
static_assert(offsetof(PedSocial, unknown200) == 0x200);
static_assert(offsetof(PedSocial, flags224) == 0x224);
static_assert(sizeof(PedSocial) == 0x22C); // (the ped's +0xD70 follows)

constexpr int PEDTYPE_PLAYER = 13;

// a CPed field Ped.h doesn't name yet
template <typename T>
re::Field<T>& PedField(const void* p, uint32_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uintptr_t>(p) + offset);
}
PedSocial* SocialOf(CPed* ped) {
    return reinterpret_cast<PedSocial*>(reinterpret_cast<uint8_t*>(ped) + 0xB44);
}
int16_t ModelOf(const CEntity* entity) {
    return PedField<int16_t>(entity, 0x10E);
}

GAME_VAR(uint32_t, SS_timeMs, 0x00C1A9B4);          // CTimer::m_snTimeInMilliseconds
GAME_VAR(CPed*, SS_playerPed, 0x00C1AEA8);          // FindPlayerPed
GAME_VAR(uint8_t, SS_chapter, 0x00A147E0);
// models that never socialise (the ped models the game keeps in these)
GAME_VAR(int32_t, SS_modelBF42CC, 0x00BF42CC);
GAME_VAR(int32_t, SS_modelBF42D0, 0x00BF42D0);
GAME_VAR(int32_t, SS_modelBF42D4, 0x00BF42D4);
GAME_VAR(int32_t, SS_modelBF42D8, 0x00BF42D8);
GAME_VAR(int32_t, SS_modelBF42DC, 0x00BF42DC);
// [ped type][ped type]: the base scores
inline int32_t (*const SS_likes)[15] = reinterpret_cast<int32_t (*)[15]>(0x00A33A68);
inline int32_t (*const SS_fears)[15] = reinterpret_cast<int32_t (*)[15]>(0x00A33E28);
// [26 kinds][5 times]: what an action done n times adds to each score
inline int32_t (*const SS_actionLikes)[5] = reinterpret_cast<int32_t (*)[5]>(0x00A341B0);
inline int32_t (*const SS_actionFears)[5] = reinterpret_cast<int32_t (*)[5]>(0x00A343B8);
// [model][lo, hi]: the ranges each model's scores are kept in
inline int8_t (*const SS_likesRange)[2] = reinterpret_cast<int8_t (*)[2]>(0x00A345C0);
inline int8_t (*const SS_fearsRange)[2] = reinterpret_cast<int8_t (*)[2]>(0x00A347D0);
// [likes + 5][fears + 5]: the reaction
inline int32_t (*const SS_reactionGrid)[11] = reinterpret_cast<int32_t (*)[11]>(0x00A349E0);
// the gift requirements (entries of 0x14 bytes from +2: +0 the kind)
inline void* const SS_giftRequirements = reinterpret_cast<void*>(0x020C3698);
inline uint8_t* const SS_giftEntries = reinterpret_cast<uint8_t*>(0x020C369A);
inline void* const SS_missionMgr = reinterpret_cast<void*>(0x020C3CA0);
// hashed names (the input actions; a mission)
inline void* const SS_input1B65444 = reinterpret_cast<void*>(0x01B65444);
inline void* const SS_input1B65448 = reinterpret_cast<void*>(0x01B65448);
inline void* const SS_mission1B6547C = reinterpret_cast<void*>(0x01B6547C);

// the social's own
GAME_FN(uint8_t, __thiscall, SS_Social48D470, 0x0048D470, void* pedA64, CPed* other);
GAME_FN(uint8_t, __thiscall, SS_Social48D560, 0x0048D560, PedSocial* self);
GAME_FN(uint8_t, __thiscall, SS_Social48D580, 0x0048D580, PedSocial* self);
GAME_FN(uint8_t, __thiscall, SS_Social48D5C0, 0x0048D5C0, PedSocial* self);
GAME_FN(uint8_t, __thiscall, SS_Social48D5F0, 0x0048D5F0, PedSocial* self);
GAME_FN(uint8_t, __thiscall, SS_Social48D620, 0x0048D620, PedSocial* self);
GAME_FN(uint8_t, __thiscall, SS_Social48D690, 0x0048D690, PedSocial* self, CPed* ped);
GAME_FN(int, __thiscall, SS_Social48D770, 0x0048D770, PedSocial* self, int value);
GAME_FN(void, __thiscall, SS_Social48D7F0, 0x0048D7F0, PedSocial* self);
GAME_FN(uint8_t, __thiscall, SS_Social48DFB0, 0x0048DFB0, PedSocial* self);
GAME_FN(uint8_t, __thiscall, SS_Social48E130, 0x0048E130, PedSocial* self);
GAME_FN(uint8_t, __thiscall, SS_Social48E2B0, 0x0048E2B0, PedSocial* self);
GAME_FN(int*, __thiscall, SS_Social48EBC0, 0x0048EBC0, PedSocial* self, int* out, CPed* ped, int a, uintptr_t b, uintptr_t c);
GAME_FN(void, __thiscall, SS_Social48F1D0, 0x0048F1D0, PedSocial* self, CPed* other, int value, int a, int b);
GAME_FN(void, __thiscall, SS_Social48F380, 0x0048F380, PedSocial* self);
GAME_FN(uint8_t, __thiscall, SS_Social490AF0, 0x00490AF0, PedSocial* self, CPed* other);
GAME_FN(void, __thiscall, SS_Social490C40, 0x00490C40, PedSocial* self, int reaction, uintptr_t a);
GAME_FN(void, __thiscall, SS_Social4910A0, 0x004910A0, PedSocial* self);   // stops it
GAME_FN(void, __thiscall, SS_Social4911C0, 0x004911C0, PedSocial* self, int started, int a, int b);
GAME_FN(void, __thiscall, SS_Social4925E0, 0x004925E0, PedSocial* self, CPed* ped, int a, int b, int c);
GAME_FN(void, __thiscall, SS_Social492680, 0x00492680, PedSocial* self);
GAME_FN(void, __thiscall, SS_Social492720, 0x00492720, PedSocial* self, int a, CPed* ped);
// registered pointers
GAME_FN(void, __thiscall, SS_RefAssign, 0x005D7F10, re::Ptr32<CPed>* slot, CPed* entity);
GAME_FN(void, __thiscall, SS_RefAssign472CA0, 0x00472CA0, re::Ptr32<CPed>* slot, CPed* entity);
GAME_FN(void, __thiscall, SS_RefCopy, 0x00448230, re::Ptr32<CPed>* slot, re::Ptr32<CPed>* from);
// peds
GAME_FN(uint8_t, __thiscall, SS_Ped465A60, 0x00465A60, CPed* ped, int chance);
GAME_FN(uint8_t, __thiscall, SS_Ped4686C0, 0x004686C0, CPed* ped);
GAME_FN(CPed*, __cdecl, SS_AsPed, 0x00468E10, CEntity* entity);
GAME_FN(uint8_t, __thiscall, SS_PedWantsGift, 0x00474A30, CPed* ped);
GAME_FN(uint8_t, __thiscall, SS_Ped474B80, 0x00474B80, CPed* ped);
GAME_FN(uint8_t*, __thiscall, SS_PedGetIntelligence408, 0x00474BE0, CPed* ped);   // +4 the attacker, +0x38 a time
GAME_FN(int*, __thiscall, SS_PedGetAttitude, 0x00475380, CPed* ped, int* out, int pedType);
GAME_FN(uint8_t, __thiscall, SS_Ped4765C0, 0x004765C0, CPed* ped);
GAME_FN(uint8_t*, __thiscall, SS_PedGetModelInfo, 0x00477DC0, CPed* ped);   // +0x50 female
GAME_FN(CPed*, __thiscall, SS_Ped497020, 0x00497020, CPed* ped);
GAME_FN(CEntity*, __thiscall, SS_PedGetTarget, 0x00497100, CPed* ped);
GAME_FN(void, __thiscall, SS_Ped497520, 0x00497520, CPed* ped, uintptr_t a);
GAME_FN(CPed*, __cdecl, SS_FindPedNear, 0x004D3370, CPed* ped, float radius);
GAME_FN(CEntity*, __thiscall, SS_Targeting4D7780, 0x004D7780, void* targeting);
GAME_FN(void, __thiscall, SS_ModelInfo522380, 0x00522380, void* modelInfo, int* out);
// the objectives
GAME_FN(void*, __thiscall, SS_Objectives470340, 0x00470340, void* queue);
GAME_FN(void*, __thiscall, SS_Objectives485900, 0x00485900, void* queue);
GAME_FN(int, __cdecl, SS_Objective48E6E0, 0x0048E6E0, void* objective);
GAME_FN(CPed*, __thiscall, SS_Objective8B6120, 0x008B6120, void* objective);   // its +0x14
// the rest
GAME_FN(CPed*, __thiscall, SS_PedPoolGetAt, 0x00415DE0, void* pool, int slot);
inline void* __cdecl SS_Memset(void* p, int value, size_t n) { return crt::Memset(p, value, n); }
GAME_FN(int, __cdecl, SS_RandomInt, 0x0040DA80, int min, int max);
GAME_FN(const float*, __cdecl, SS_FindPlayerCoors, 0x00446900, ped::CVector* out);
GAME_FN(float, __cdecl, SS_Fn487E50, 0x00487E50, const void* position, const ped::CVector* to, float radius);
GAME_FN(uint8_t, __cdecl, SS_InNIS, 0x005BB380);
GAME_FN(uint8_t, __thiscall, SS_Input5F4800, 0x005F4800, void* input, void* action, int a, int b);
GAME_FN(int, __thiscall, SS_GiftFind, 0x006A9C70, void* gifts, CPed* ped);
GAME_FN(int, __thiscall, SS_MissionFind, 0x006A9FD0, void* mgr, void* name);
GAME_FN(uint8_t*, __thiscall, SS_MissionGetStats, 0x006AA7C0, void* mgr, int id);   // +2 successes, +4 flags

bool IsExemptModel(int model) {
    return model == SS_modelBF42DC || model == SS_modelBF42D4 || model == SS_modelBF42D0 ||
           model == SS_modelBF42CC || model == SS_modelBF42D8;
}
int Clamp5(int value) {
    if (value > 5) {
        return 5;
    }
    return value < -5 ? -5 : value;
}
bool IsFemale(CPed* ped) {
    return SS_PedGetModelInfo(ped)[0x50] != 0;
}

// 0x492AA0: hurt below half its health, it tells the player (unless it's busy with
// something up to 12)
void __fastcall CheckHurt(PedSocial* self, re::Edx) {
    if (self->unknown200 == -1 || self->unknown200 > 12) {
        // (x87: the quotient at double precision, against the double 0.5)
        if (double(self->ped->health) / double(self->ped->maxHealth) < 0.5) {
            SS_Social492720(self, 1, SS_playerPed);
        }
    }
}

// (0x492AE0's two exits that give up and let the other take it back)
uint8_t Bail(PedSocial* self) {
    self->flags225 |= 1;
    SS_RefCopy(&self->ref120, &self->partner);
    SS_RefCopy(&self->ref124, &self->partner);
    SS_Social492680(self);
    return 0;
}

// 0x492AE0: starts a social exchange with `partner` (or picks someone: the one it
// was with, or someone within 4 m when the player presses the talk inputs), works out
// its reaction (+0xEC) and, with a ped that isn't the player, the other's too. 1: it
// started (or was going on already).
uint8_t __fastcall StartSocial(PedSocial* self, re::Edx) {
    CPed* const owner = self->ped;
    if (IsExemptModel(owner->modelIndex)) {
        return 0;
    }
    int const state = self->state;
    CPed* target = self->partner;
    if (state == 4 || state == 3 || state == 6) {
        return 0;
    }
    if (PedField<int32_t>(owner, 0x1370) == 1) {
        return 0;
    }
    if (state == 0 && !(self->flags224 & 0x10) && self->time + 10000 > SS_timeMs) {
        return 0;
    }

    if (!target) {
        if (state != 0) {
            SS_Social4910A0(self);
        }
        if ((self->flags224 & 0x10) && (target = self->lastPartner) != nullptr) {
            if (PedField<int32_t>(target, 0x1370) == 1) {
                return 0;
            }
            if (target->pedType == PEDTYPE_PLAYER) {
                if (target->unknown1E91 != 0 || PedField<int32_t>(target, 0x1DD4) != 0) {
                    return 0;
                }
            } else {
                if (SS_Social490AF0(self, target)) {
                    return 0;
                }
                CPed* busyWith = SocialOf(target)->partner;
                if (busyWith && busyWith != owner) {
                    return 0;
                }
            }
            if (!(self->flags224 & 3) && !(self->flags226 & 2) && !SS_Social48D470(owner->unknownA64, target)) {
                return 0;
            }
            if (SS_Ped497020(owner) == target && (self->flags226 & 2) && self->unknownFC + 200 > SS_timeMs) {
                SS_Ped497520(owner, 0);
            }
        } else {
            if (!SS_Ped465A60(owner, 8)) {
                return 0;
            }
            if (SS_InNIS() && SS_Ped4765C0(owner)) {
                return 0;
            }
            if (PedField<int32_t>(owner, 0x1DD4) != 0) {
                return 0;
            }
            if (!SS_Objective48E6E0(SS_Objectives470340(&owner->objectives))) {
                return 0;
            }
            void* input = owner->intelligence + 0x290;
            if (!SS_Input5F4800(input, SS_input1B65448, 1, 1) && !SS_Input5F4800(input, SS_input1B65444, 1, 1)) {
                return 0;
            }
            target = SS_FindPedNear(owner, 4.0f);
            if (!target) {
                // (the state again: stopping it above may have changed it)
                int now = self->state;
                return now == 2 || now == 1 || now == 5;
            }
            if (PedField<int32_t>(target, 0x1370) == 1 || PedField<int32_t>(target, 0x1DD4) == 0) {
                return 0;
            }
            if (owner->seatState == 1 &&
                (PedField<uint8_t>(owner, 0x1BE) == 6 || PedField<uint8_t>(target, 0x1BE) == 6)) {
                return 0;
            }
            if (target->pedType == PEDTYPE_PLAYER) {
                if (target->unknown1E91 != 0) {
                    return 0;
                }
                if (!SS_Ped4686C0(owner)) {
                    return 0;
                }
            } else {
                if (target->seatState == 2) {
                    return 0;
                }
                if (!SS_Objective48E6E0(SS_Objectives470340(&target->objectives))) {
                    return 0;
                }
                CPed* busyWith = SocialOf(target)->partner;
                if (busyWith && busyWith != owner) {
                    return 0;
                }
                if (SocialOf(target)->flags224 & 0x10) {
                    return 0;
                }
                if (!SS_Input5F4800(target->intelligence + 0x290, SS_input1B65448, 1, 1)) {
                    return 0;
                }
            }
        }
    } else if (state == 2 || state == 1) {
        return 1;
    }

    int tmp;
    if (target->pedType == PEDTYPE_PLAYER) {
        // ---- with the player: only this one's reaction
        int const targetType = PEDTYPE_PLAYER;
        int likes = SS_likes[owner->pedType][PEDTYPE_PLAYER];
        int fears = SS_fears[owner->pedType][PEDTYPE_PLAYER];
        int const attitude = (*SS_PedGetAttitude(owner, &tmp, targetType) - 2) * 3;
        int theirLikes = SS_likes[PEDTYPE_PLAYER][owner->pedType];
        SS_PedGetAttitude(target, &tmp, owner->pedType);   // (the result isn't used)
        int bonusLikes;
        int bonusFears;
        if (owner->pedType == 9) {
            bonusLikes = 0;
            bonusFears = self->unknown104;
        } else {
            bonusLikes = 1 - SS_chapter / 3;
            bonusFears = 0;
        }
        uint8_t started = 0;
        if (self->state == 0) {
            if (!(self->flags224 & 0x10)) {
                if ((self->flags226 & 8) && SocialOf(target)->ref120 == owner) {
                    return 0;
                }
                // (not while another ped is at the player)
                CPool<CPed>* pool = CPools::ms_pPedPool;
                for (int slot = pool->size; slot != 0;) {
                    slot--;
                    CPed* ped = SS_PedPoolGetAt(CPools::ms_pPedPool, slot);
                    if (ped && ped != target && ped != owner && !ped->unknown1C98 && SocialOf(ped)->state != 0 &&
                        SocialOf(ped)->partner == target) {
                        return 0;
                    }
                }
                if (!(self->flags226 & 2)) {
                    int kind = self->forcedReaction;
                    if (kind == -1 || target != self->lastPartner) {
                        kind = *SS_Social48EBC0(self, &tmp, target, 1, 0, 0);
                    }
                    if (kind == 4 || kind == 5) {
                        if (!self->ref138 || target != self->ref138) {
                            return 0;
                        }
                    } else {
                        if (kind == 3 || kind >= 6) {
                            return 0;
                        }
                        if (*SS_PedGetAttitude(owner, &tmp, targetType) >= 2) {
                            return 0;
                        }
                        int roll = SS_RandomInt(1, 100);
                        if (kind <= 3 ? roll < 25 : roll < 50) {
                            self->time = SS_timeMs;
                            return 0;
                        }
                    }
                }
            }
            self->startTime = SS_timeMs;
            SS_Memset(self->counts, 0, sizeof(self->counts));
            self->flags224 &= ~4;
            started = 1;
        }
        uint8_t reactionSet = 0;
        self->time = SS_timeMs;
        SS_RefAssign472CA0(&self->partner, target);
        int const forced = self->forcedReaction;
        self->state = 1;
        uint8_t done;
        if (forced != -1 && target == self->lastPartner) {
            self->reaction = forced;
            if (self->unknown1F4 == 0x19) {
                if (!started) {
                    goto checkFlag;
                }
                if ((forced == 4 || forced == 5) && self->ref138 && target == self->ref138) {
                    return Bail(self);
                }
            }
            goto checkStarted;
        }
        {
            if (IsFemale(owner)) {
                likes /= 2;
            }
            if (owner->seatState == 1) {
                int16_t held = ModelOf(owner->weapon);
                if (held != -1 && ModelOf(target->weapon) == -1) {
                    theirLikes -= 1;
                }
                if (ModelOf(target->weapon) != -1 && held == -1) {
                    likes -= 1;
                }
            }
            int mine;
            int theirs;
            SS_ModelInfo522380(SS_PedGetModelInfo(owner), &mine);
            SS_ModelInfo522380(SS_PedGetModelInfo(self->partner), &theirs);
            int const difference = mine - theirs;
            theirLikes -= difference;
            likes += difference;
            self->unknown118 = difference;
            int sign;
            if (self->flags225 & 0x20) {
                sign = -1;
            } else {
                sign = SS_Social48D770(self, likes) >= 0 ? 1 : -1;
            }
            int const bonusLikes0 = bonusLikes;
            int const bonusFears0 = bonusFears;
            for (int kind = 0; kind < 26; kind++) {
                if (kind == 0 && SS_Social48DFB0(self) && self->counts[0] > 0) {
                    // (the first kind counted from 1 here, not 0)
                    int n = self->counts[0];
                    bonusLikes += SS_actionLikes[0][n > 4 ? 4 : n] * sign;
                    bonusFears += SS_actionFears[0][n > 4 ? 4 : n];
                } else if (self->counts[kind] > 0) {
                    int n = self->counts[kind] - 1;
                    bonusLikes += SS_actionLikes[kind][n > 4 ? 4 : n] * sign;
                    bonusFears += SS_actionFears[kind][n > 4 ? 4 : n];
                }
            }
            int x = likes + self->unknown108 - (theirLikes > 0 ? 0 : theirLikes) / 2 - attitude + sign + bonusLikes;
            int y = fears + self->unknown10C + bonusFears + attitude;

            int const model = owner->modelIndex;
            int const xlo = SS_Social48E130(self) ? -5 : SS_likesRange[model][0];
            int const ylo = ((self->flags225 & 0x10) && !(self->flags225 & 0x20)) ? 0 : SS_fearsRange[model][0];
            if (!(self->flags224 & 0x40) &&
                (SS_PedWantsGift(owner) || SS_Ped474B80(owner) || owner->unknown1E27 != 0)) {
                if (x > 0) {
                    x = 0;
                }
                int const xhi = SS_likesRange[model][1];
                int const low = xhi < x ? xhi : x;
                if (xlo > low) {
                    x = xlo;
                } else if (xhi < x) {
                    x = xhi;
                }
                if (y < 2) {
                    y = 2;
                }
                int const capped = y > 5 ? 5 : y;
                if (ylo > capped) {
                    y = ylo;
                } else if (y > 5) {
                    y = 5;
                }
            } else {
                int gift = SS_GiftFind(SS_giftRequirements, owner);
                if (owner->seatState == 1 && gift != -1) {
                    uint16_t kind = *reinterpret_cast<uint16_t*>(SS_giftEntries + gift * 0x14);
                    if (kind == 2 || kind == 1 || kind == 0) {
                        y -= 3;
                    }
                }
                int const xhi = SS_likesRange[model][1];
                int low = xhi < x ? xhi : x;
                if (xlo > low) {
                    x = xlo;
                } else if (xhi < x) {
                    x = xhi;
                }
                int const yhi = SS_fearsRange[model][1];
                low = yhi < y ? yhi : y;
                if (ylo > low) {
                    y = ylo;
                } else if (yhi < y) {
                    y = yhi;
                }
            }
            bool const towardsLast = target == self->lastPartner && (self->flags224 & 0x20);
            if (!towardsLast) {
                int type = owner->pedType;
                if (type == 9 || type == 10 || type == 8 || type == 7) {
                    x = 0;
                    if (y > 3) {
                        y = 3;
                    } else if (y < 0) {
                        y = 0;
                    }
                }
            }
            if (towardsLast) {
                x += self->unknown110;
                y += self->unknown114;
            }
            x = Clamp5(x);
            y = Clamp5(y);
            int reaction = SS_reactionGrid[x + 5][y + 5];
            self->reaction = reaction;
            if (reaction == 3 && SS_Social48D690(self, target)) {
                int x2 = Clamp5(x + sign * 2);
                y = Clamp5(y - 2);
                reaction = SS_reactionGrid[x2 + 5][y + 5];
                self->reaction = reaction;
            }
            if (self->unknown1F4 == 0x19 && started && (reaction == 4 || reaction == 5) && self->ref138 &&
                target == self->ref138) {
                return Bail(self);
            }
            if (reaction >= 4 && reaction <= 6 && !SS_Social48D690(self, target)) {
                if (started && !(self->flags226 & 2)) {
                    reactionSet = 1;
                    self->unknown1FC = reaction;
                }
                self->reaction = 7;
                goto checkStarted;
            }
            int const fixed = self->unknown130;
            if (fixed == -1 || bonusLikes != bonusLikes0 || bonusFears != bonusFears0) {
                goto checkStarted;
            }
            if (!started && !(self->flags224 & 2)) {
                goto notDone;
            }
            if (!(self->flags226 & 2)) {
                self->reaction = fixed;
                reactionSet = 1;
                self->unknown1FC = fixed;
            }
        }
    checkStarted:
        if (started) {
            goto isDone;
        }
    checkFlag:
        if (self->flags224 & 2) {
            goto isDone;
        }
    notDone:
        done = 0;
        goto told;
    isDone:
        done = 1;
        SS_Social492720(self, self->reaction > 3 ? 1 : 0, nullptr);
        SS_Social490C40(self, self->reaction, 0);
    told:
        if (started && !(self->flags226 & 2) && !reactionSet) {
            self->unknown1FC = self->reaction;
        }
        {
            uint8_t const b = (SS_Social48D620(self) && (self->flags224 & 4)) ? 1 : 0;
            uint8_t const a = ((self->flags226 & 0x10) || (SS_Social48D5F0(self) && (self->flags224 & 4))) ? 1 : 0;
            // (the first argument is pushed as the whole stack word the byte lives in)
            SS_Social4911C0(self, done, a, b);
        }
        self->flags224 &= ~2;
        self->flags226 &= ~0x10;
        self->lastAction = self->action;
        if (SS_Social48D5C0(self)) {
            SS_RefAssign472CA0(&SocialOf(self->partner)->ref11C, owner);
        }
        if (SS_Social48D580(self)) {
            SS_RefAssign472CA0(&SocialOf(self->partner)->ref120, owner);
        }
        if (SS_Social48D560(self)) {
            SS_RefAssign472CA0(&SocialOf(self->partner)->ref124, owner);
        }
        SocialOf(self->partner)->counts[self->action]++;
        PedSocial* other = SocialOf(self->partner);
        other->flags225 = uint8_t((other->flags225 & 0x7F) | (done << 7));
        self->flags226 = uint8_t(self->flags226 ^ ((self->flags226 ^ done) & 1));
        return 1;
    }

    // ---- with another ped: both reactions
    PedSocial* const theirs = SocialOf(target);
    int likes = SS_likes[owner->pedType][target->pedType];
    int fears = SS_fears[owner->pedType][target->pedType];
    int const attitude = (*SS_PedGetAttitude(owner, &tmp, target->pedType) - 2) * 3;
    int theirFears = SS_fears[target->pedType][owner->pedType];
    int theirLikes = SS_likes[target->pedType][owner->pedType];
    int const theirAttitude = (*SS_PedGetAttitude(target, &tmp, owner->pedType) - 2) * 3;
    int bonusLikes = self->unknown100;
    int bonusFears = self->unknown104;
    int theirBonusLikes = theirs->unknown100;
    int theirBonusFears = theirs->unknown104;
    uint8_t started = 0;
    if (self->state == 0) {
        if (!(self->flags224 & 0x10) && !(self->flags226 & 2)) {
            int kind = self->forcedReaction;
            if (kind == -1 || target != self->lastPartner) {
                kind = *SS_Social48EBC0(self, &tmp, target, 1, 0, 0);
            }
            if (kind == 4 || kind == 5) {
                if (!self->ref138 || target != self->ref138) {
                    return 0;
                }
            } else {
                if (kind == 6) {
                    return 0;
                }
                if (kind == 3) {
                    if (SS_RandomInt(1, 100) < 60 || target == self->ref138) {
                        self->time = SS_timeMs;
                        return 0;
                    }
                    goto roll25;
                }
                if (kind > 3) {
                    if (SS_RandomInt(1, 100) < 50) {
                        self->time = SS_timeMs;
                        return 0;
                    }
                } else {
                roll25:
                    if (SS_RandomInt(1, 100) < 25) {
                        self->time = SS_timeMs;
                        return 0;
                    }
                }
                if (kind >= 3) {
                    // (near the player, not before the mission 0x1B6547C names is passed)
                    ped::CVector player;
                    SS_FindPlayerCoors(&player);
                    uint8_t* matrix = PedField<uint8_t*>(self->entity, 0x14);
                    const void* position = matrix ? static_cast<const void*>(matrix + 0x30)
                                                   : static_cast<const void*>(re::PtrCast<uint8_t*>(self->entity) + 4);
                    if (SS_Fn487E50(position, &player, 3.0f) <= 0.0f) {
                        int id = SS_MissionFind(SS_missionMgr, SS_mission1B6547C);
                        if (*reinterpret_cast<uint16_t*>(SS_MissionGetStats(SS_missionMgr, id) + 2) == 0 &&
                            !(SS_MissionGetStats(SS_missionMgr, id)[4] & 8)) {
                            return 0;
                        }
                    }
                }
            }
        }
        self->startTime = SS_timeMs;
        theirs->startTime = SS_timeMs;
        SS_Memset(self->counts, 0, sizeof(self->counts));
        SS_Memset(theirs->counts, 0, sizeof(theirs->counts));
        self->flags224 &= ~4;
        theirs->flags224 &= ~4;
        started = 1;
    }
    self->time = SS_timeMs;
    theirs->time = SS_timeMs;
    SS_RefAssign472CA0(&self->partner, target);
    self->state = 1;
    theirs->state = 4;
    SS_RefAssign472CA0(&theirs->partner, owner);
    if (IsFemale(target) && !IsFemale(owner)) {
        likes -= 1;
        theirLikes /= 2;
        fears += 2;
    } else if (!IsFemale(target) && IsFemale(owner)) {
        likes /= 2;
        theirLikes -= 1;
        theirFears += 2;
    }
    if (owner->seatState == 1) {
        int16_t held = ModelOf(owner->weapon);
        if (held != -1 && ModelOf(target->weapon) == -1) {
            theirLikes -= 1;
        }
        if (ModelOf(target->weapon) != -1 && held == -1) {
            likes -= 1;
        }
    }
    {
        int mine;
        int other;
        SS_ModelInfo522380(SS_PedGetModelInfo(owner), &mine);
        SS_ModelInfo522380(SS_PedGetModelInfo(self->partner), &other);
        self->unknown118 = mine - other;
        SocialOf(self->partner)->unknown118 = -(mine - other);
    }
    likes += self->unknown118;
    theirLikes -= self->unknown118;
    int const sign = SS_Social48D770(self, likes) >= 0 ? 1 : -1;
    int const theirSign = SS_Social48D770(SocialOf(self->partner), theirLikes) >= 0 ? 1 : -1;
    for (int kind = 0; kind < 26; kind++) {
        if (self->counts[kind] > 0) {
            int n = self->counts[kind] - 1;
            if (n > 4) {
                n = 4;
            }
            bonusLikes += SS_actionLikes[kind][n] * sign;
            bonusFears += SS_actionFears[kind][n];
            theirBonusFears += SS_actionFears[kind][n] / 2;
        }
        if (theirs->counts[kind] > 0) {
            int n = theirs->counts[kind] - 1;
            if (n > 4) {
                n = 4;
            }
            theirBonusLikes += SS_actionLikes[kind][n] * theirSign;
            theirBonusFears += SS_actionFears[kind][n];
            bonusFears += SS_actionFears[kind][n] / 2;
        }
    }
    theirFears += theirBonusFears;
    int theirX = theirLikes + theirSign + theirBonusLikes;
    likes += sign - (theirX > 0 ? 0 : theirX) / 2 - attitude + bonusLikes;
    fears += bonusFears + attitude;

    int const ownType = owner->pedType;
    int const targetType = target->pedType;
    int y;
    if (ownType != targetType) {
        int const model = owner->modelIndex;
        int const xhi = SS_likesRange[model][1];
        int low = xhi < likes ? xhi : likes;
        int const xlo = SS_likesRange[model][0];
        if (xlo > low) {
            likes = xlo;
        } else if (xhi < likes) {
            likes = xhi;
        }
        int const yhi = SS_fearsRange[model][1];
        low = yhi < fears ? yhi : fears;
        int const ylo = SS_fearsRange[model][0];
        if (ylo > low) {
            y = ylo;
        } else {
            y = fears;
            if (yhi < y) {
                y = yhi;
            }
        }
    } else {
        y = fears;
    }
    if ((ownType == 9 || ownType == 10 || ownType == 8 || ownType == 7) &&
        ((targetType >= 1 && targetType <= 6) || (targetType >= 11 && targetType <= 13))) {
        likes = 0;
        if (y > 3) {
            y = 3;
        } else if (y < 0) {
            y = 0;
        }
    }
    if (target == self->lastPartner && (self->flags224 & 0x20)) {
        likes += self->unknown110;
        y += self->unknown114;
    }
    likes = Clamp5(likes);
    y = Clamp5(y);
    int const forced = self->forcedReaction;
    if (forced != -1 && target == self->lastPartner) {
        self->reaction = forced;
        if (started && (forced == 4 || forced == 5) && self->ref138 && target == self->ref138) {
            return Bail(self);
        }
    } else {
        int reaction = SS_reactionGrid[likes + 5][y + 5];
        self->reaction = reaction;
        if (reaction == 3 && SS_Social48D690(self, target)) {
            likes = Clamp5(likes + sign * 2);
            y = Clamp5(y - 2);
            reaction = SS_reactionGrid[likes + 5][y + 5];
            self->reaction = reaction;
        }
        if (started && (reaction == 4 || reaction == 5) && self->ref138 && target == self->ref138) {
            return Bail(self);
        }
        if (reaction >= 3 && reaction <= 6 && !SS_Social48D690(self, target)) {
            self->reaction = 7;
        }
    }
    {
        uint8_t const b = ((self->flags226 & 0x10) || SS_Social48D580(SocialOf(self->partner))) ? 1 : 0;
        uint8_t const c = SS_Social48D5C0(SocialOf(self->partner));
        // (the first argument is pushed as the whole stack word the byte lives in)
        SS_Social4911C0(self, started, b, c);
    }
    self->lastAction = self->action;
    self->flags226 &= ~0x10;
    if (SS_Social48D5C0(self)) {
        SS_RefAssign472CA0(&theirs->ref11C, owner);
    }
    if (SS_Social48D580(self)) {
        SS_RefAssign472CA0(&theirs->ref120, owner);
    }
    if (SS_Social48D560(self)) {
        SS_RefAssign472CA0(&theirs->ref124, owner);
    }
    theirs->counts[self->action]++;
    if (theirs->forcedReaction != -1 && owner == theirs->lastPartner) {
        theirs->reaction = theirs->forcedReaction;
    } else {
        // the other's, with this one's action counted
        int const kind = self->action;
        int const n = theirs->counts[kind];
        int x = theirX;
        int yy = theirFears;
        if (n == 1) {
            x += SS_actionLikes[kind][0] * theirSign;
            yy += SS_actionFears[kind][0];
        } else if (n <= 5 && n > 1) {
            x += (SS_actionLikes[kind][n - 1] - SS_actionLikes[kind][n - 2]) * theirSign;
            yy += SS_actionFears[kind][n - 1] - SS_actionFears[kind][n - 2];
        }
        yy += theirAttitude;
        x += -theirAttitude - (likes > 0 ? 0 : likes) / 2;
        theirX = x;
        if (owner == theirs->lastPartner && (theirs->flags224 & 0x20)) {
            x += theirs->unknown110;
            yy += theirs->unknown114;
        } else {
            int const tType = target->pedType;
            if ((tType == 9 || tType == 10) &&
                ((owner->pedType >= 1 && owner->pedType <= 6) || (owner->pedType >= 11 && owner->pedType <= 13))) {
                x = 0;
                if (yy > 3) {
                    yy = 3;
                } else if (yy < 0) {
                    yy = 0;
                }
                goto theirGrid;
            }
            if (owner->pedType != tType) {
                int const model = target->modelIndex;
                int const xhi = SS_likesRange[model][1];
                int low = xhi < x ? xhi : x;
                int const xlo = SS_likesRange[model][0];
                if (xlo > low) {
                    x = xlo;
                } else {
                    x = theirX;
                    if (xhi < x) {
                        x = xhi;
                    }
                }
                int const yhi = SS_fearsRange[model][1];
                low = yhi < yy ? yhi : yy;
                int const ylo = SS_fearsRange[model][0];
                if (ylo > low) {
                    yy = ylo;
                } else if (yhi < yy) {
                    yy = yhi;
                }
            }
        }
        x = Clamp5(x);
        yy = Clamp5(yy);
    theirGrid:
        int reaction = SS_reactionGrid[x + 5][yy + 5];
        theirs->reaction = reaction;
        if (reaction == 3) {
            if (!SS_Social48D690(theirs, owner)) {
                goto theirsDone;
            }
            int x2 = Clamp5(x + theirSign * 2);
            yy = Clamp5(yy - 2);
            reaction = SS_reactionGrid[x2 + 5][yy + 5];
            theirs->reaction = reaction;
        }
        if (reaction >= 4 && reaction <= 6 && !SS_Social48D690(theirs, owner)) {
            theirs->reaction = 7;
        }
    }
theirsDone:
    {
        uint8_t const c = SS_Social48D5C0(self);
        uint8_t const b = SS_Social48D580(self);
        SS_Social4911C0(theirs, 0, b, c);
    }
    theirs->flags225 = uint8_t((started << 7) | (theirs->flags225 & 0x7F));
    self->flags226 = uint8_t(self->flags226 ^ ((self->flags226 ^ started) & 1));
    theirs->lastAction = theirs->action;
    if (SS_Social48D5C0(theirs)) {
        SS_RefAssign472CA0(&self->ref11C, target);
    }
    if (SS_Social48D580(theirs)) {
        SS_RefAssign472CA0(&self->ref120, target);
    }
    if (SS_Social48D560(theirs)) {
        SS_RefAssign472CA0(&self->ref124, target);
    }
    self->counts[theirs->action]++;
    return 1;
}

// 0x494290: hit by someone (within 20 of the hit's time +0x38): the player (or whoever
// the player's objective is about) gets remembered, and a ped starts a social
// exchange with reaction 0x19. 1: it started one.
uint8_t __fastcall RespondToHit(PedSocial* self, re::Edx) {
    CPed* const owner = self->ped;
    if (IsExemptModel(owner->modelIndex)) {
        return 0;
    }
    if (SS_Social48E2B0(self) && !(*reinterpret_cast<float*>(SS_PedGetIntelligence408(owner) + 0x38) >= 20.0f) &&
        PedField<int32_t>(self->ped, 0x1370) != 1 && PedField<int32_t>(self->ped, 0x1350) != 1) {
        CEntity* entity = SS_PedGetTarget(self->ped);
        if (!entity) {
            entity = SS_Targeting4D7780(self->ped->targeting);
        }
        CPed* attacker = SS_AsPed(entity);
        if (!attacker) {
            return 0;
        }
        if (attacker->pedType != PEDTYPE_PLAYER) {
            SS_RefAssign472CA0(&self->ref120, attacker);
            SS_RefAssign472CA0(&self->ref124, attacker);
            SS_RefAssign(&self->ref128, nullptr);
            return 0;
        }
        if (void* objective = SS_Objectives485900(&self->ped->objectives)) {
            CPed* player = SS_playerPed;
            if (SS_Objective8B6120(objective) == player) {
                SS_Social48D7F0(self);
            }
            SS_RefAssign472CA0(&self->ref120, SS_Objective8B6120(objective));
            SS_RefAssign472CA0(&self->ref124, SS_Objective8B6120(objective));
            SS_RefAssign(&self->ref128, nullptr);
            return 0;
        }
        if (attacker == self->lastPartner && self->forcedReaction != -1) {
            self->flags226 |= 4;
        }
        SS_Social48F380(self);
        SS_Social4910A0(self);
        int kind = 0;   // (the game's slot holds `this` until 0x48EBC0 writes it)
        SS_Social48EBC0(self, &kind, attacker, 1, 0, 0);
        if (SS_Social48D690(self, attacker)) {
            SS_RefAssign472CA0(&self->ref120, attacker);
            SS_RefAssign472CA0(&self->ref124, attacker);
            SS_RefAssign(&self->ref128, nullptr);
            SS_Social48D7F0(self);
            return 0;
        }
        self->unknown1FC = kind;
        SS_RefAssign472CA0(&self->ref120, attacker);
        SS_RefAssign472CA0(&self->ref124, attacker);
        SS_RefAssign(&self->ref128, nullptr);
        int type = self->ped->pedType;
        int value;
        if (type == 9 || type == 10) {
            value = 3;
        } else if (kind == 0 || kind == 1 || kind == 2) {
            value = 0;
        } else if (kind == 4 || kind == 5 || kind == 6) {
            value = 4;
        } else if (kind == 3 || kind == 7 || kind == 8) {
            value = 2;
        } else {
            goto noValue;
        }
        SS_Social48F1D0(self, attacker, value, 1, 0);
    noValue:
        SS_Social4925E0(self, attacker, 0, 0x19, 0);
        self->flags226 |= 2;
        self->response = 0x19;
        return 1;
    }
    uint8_t* hit = SS_PedGetIntelligence408(self->ped);
    if (*reinterpret_cast<re::Ptr32<CPed>*>(hit + 4) == SS_playerPed) {
        SS_Social48D7F0(self);
    }
    SS_RefAssign(&self->ref120, *reinterpret_cast<re::Ptr32<CPed>*>(SS_PedGetIntelligence408(self->ped) + 4));
    SS_RefAssign(&self->ref124, *reinterpret_cast<re::Ptr32<CPed>*>(SS_PedGetIntelligence408(self->ped) + 4));
    SS_RefAssign(&self->ref128, nullptr);
    return 0;
}

} // namespace

#define C08P1_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P1_REVERSED("PedSocial::CheckHurt", 0x00492AA0, &CheckHurt);
C08P1_REVERSED("PedSocial::StartSocial", 0x00492AE0, &StartSocial);
C08P1_REVERSED("PedSocial::RespondToHit", 0x00494290, &RespondToHit);
