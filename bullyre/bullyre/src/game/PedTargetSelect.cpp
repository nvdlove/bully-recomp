// A ped's choice of whom to attack (CPed, 0x496990..0x499490): the target it has
// (+0x1DBC, a registered pointer, how long it's had it +0x1DC0), whether it may attack
// someone (range, attitude, the player's state), the attack objectives in its queue,
// and the weighted random pick among the peds around.
//
// The fields (not in Ped.h's CPed):
//   +0x1DB4 int      the target's kind (0 .. 3, see 0x496AF0; 0 and 2 need range checks)
//   +0x1DB8 float    time spent on the current target (reset when it changes)
//   +0x1DBC CEntity* the ped that picked a fight with this one (registered)
//   +0x1DC0 float    time since then
//   +0x1DC4 uint32   when to look for a new target (CTimer ms)
//   +0x1DC8 CVector  the point on the target to aim at (in its space)
//   +0x1DE4 uint8    look for a target on the next update
//   +0x1DE5 uint8    (set while the objective's target is tried)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Ped.h"
#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// a field at an offset (for the ones no header names)
template <typename T>
re::Field<T>& F(const void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uintptr_t>(base) + offset);
}
uint8_t EntityType(const void* entity) {
    return F<uint8_t>(entity, 0x108) & 7;
}
// (inlined everywhere) the matrix's position, or the placeable's
const ped::CVector* EntityPos(const void* entity) {
    uint8_t* matrix = F<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const ped::CVector*>(matrix + 0x30) : &F<ped::CVector>(entity, 4);
}

// A pick of peds by weight (on 0x499110's stack, for 0x4884A0)
struct TargetChoice {
    int32_t count;     // +0
    float total;       // +4 the weights' sum
    struct {
        float weight;
        re::Ptr32<CPed> ped;
    } entries[16];     // +8
};
static_assert(sizeof(TargetChoice) == 0x88);

GAME_VAR(CPool<CPed>*, PT_pedPool, 0x00C0F5F0);          // CPools::ms_pPedPool
GAME_VAR(uint8_t*, PT_aibutes, 0x020C889C);              // the AI tuning (+0x64 .. +0x7C read here)
GAME_VAR(uint32_t, PT_timeInMs, 0x00C1A9B4);             // CTimer::m_snTimeInMilliseconds
GAME_VAR(float, PT_timeStep, 0x00A35020);
GAME_VAR(float, PT_timeScaleC1A9A4, 0x00C1A9A4);
GAME_VAR(ped::CVector, PT_defaultAimPoint, 0x00BCC564);
GAME_VAR(int32_t, PT_modelBF42DC, 0x00BF42DC);           // a model never attacked while +0x1DE5
inline void* const PT_objC2AB70 = reinterpret_cast<void*>(0x00C2AB70);
inline void* const PT_conversations = reinterpret_cast<void*>(0x00B9DAB8);

GAME_FN(Objective*, __thiscall, PT_QueueGetCurrent, 0x00470340, ObjectiveQueue* queue);
GAME_FN(uint8_t, __thiscall, PT_QueueRemoveFirst, 0x004702C0, ObjectiveQueue* queue);
GAME_FN(uint8_t, __thiscall, PT_QueueRemoveAt, 0x00470200, ObjectiveQueue* queue, int index);
GAME_FN(uint8_t, __thiscall, PT_QueueInsert, 0x00470180, ObjectiveQueue* queue, Objective* objective, uint32_t index);
GAME_FN(uint8_t, __thiscall, PT_QueuePrepend, 0x00471470, ObjectiveQueue* queue, Objective* objective);
GAME_FN(void, __thiscall, PT_ObjectiveAddRef, 0x0046FB00, Objective* objective);
GAME_FN(void, __thiscall, PT_ObjectiveRelease, 0x0046FB10, Objective* objective);   // (only the count)
GAME_FN(Objective*, __thiscall, PT_AttackPedConstruct, 0x00470D60, void* mem, CPed* ped, CEntity* target, int value);
// (a shared "+0x14" getter: an attack objective's target) and "+0x1C" (a break-prop's prop)
GAME_FN(CEntity*, __thiscall, PT_Get14, 0x008B6120, void* object);
GAME_FN(CEntity*, __thiscall, PT_Get1C, 0x0077E8E0, void* object);
GAME_FN(uint8_t, __thiscall, PT_BallIsTimeUp, 0x004711D0, Objective* objective);
GAME_FN(uint8_t, __thiscall, PT_BallIsTimeUpF, 0x004711F0, Objective* objective);
GAME_FN(void, __thiscall, PT_Fn48C500, 0x0048C500, void* object3BC, CEntity* entity);
GAME_FN(uint8_t, __thiscall, PT_Fn48C080, 0x0048C080, void* object3BC, CEntity* entity);
GAME_FN(CEntity*, __thiscall, PT_TargetingGetA0, 0x004D5AD0, void* targeting);
GAME_FN(void, __thiscall, PT_Targeting4D79E0, 0x004D79E0, void* targeting, CEntity* entity);
GAME_FN(float*, __thiscall, PT_GetMatrix, 0x0046DD20, CEntity* entity);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline float* PT_MultiplyPoint(float* out, const float* matrix, const ped::CVector* v) { return re::CallVec16<0x0041A9A0, float*>(out, matrix, v); }
GAME_FN(uint8_t, __thiscall, PT_Fn485590, 0x00485590, void* object2F0, CEntity* other);
GAME_FN(int*, __thiscall, PT_GetAttitude, 0x00475380, CPed* ped, int* out, int pedType);
GAME_FN(uint8_t, __thiscall, PT_Fn41AA50, 0x0041AA50, CPed* ped);
GAME_FN(void*, __cdecl, PT_Malloc, 0x005EEAA0, size_t size);
GAME_FN(uint8_t, __cdecl, PT_Fn5BB380, 0x005BB380);
GAME_FN(uint8_t, __thiscall, PT_Fn4765C0, 0x004765C0, CPed* ped);
GAME_FN(uint8_t, __thiscall, PT_Fn4CE690, 0x004CE690, CPed* ped);
GAME_FN(uint8_t, __thiscall, PT_Inventory486BE0, 0x00486BE0, void* inventory);
GAME_FN(void, __thiscall, PT_Inventory486F50, 0x00486F50, void* inventory, CPed* ped);
GAME_FN(void, __thiscall, PT_Inventory487B70, 0x00487B70, void* inventory);
GAME_FN(void, __thiscall, PT_Inventory486D50, 0x00486D50, void* inventory);
GAME_FN(uint8_t*, __cdecl, PT_GetWeaponInfo, 0x0051B450, int model);
GAME_FN(uint8_t, __thiscall, PT_Fn46EFB0, 0x0046EFB0, void* object, CPed* ped, const ped::CVector* pos, int flag);
GAME_FN(uint8_t, __thiscall, PT_InConversation, 0x00683F40, void* conversations, CEntity* ped);
GAME_FN(float, __cdecl, PT_DistanceSqMinus, 0x00487E50, const ped::CVector* a, const ped::CVector* b, float range);
GAME_FN(CEntity*, __thiscall, PT_GetTarget, 0x00497100, CPed* ped);
GAME_FN(uint8_t, __thiscall, PT_HasWeapon, 0x00474BF0, CPed* ped);
GAME_FN(float, __thiscall, PT_Weapon4D96B0, 0x004D96B0, CEntity* weapon);
GAME_FN(uint8_t, __thiscall, PT_Fn499840, 0x00499840, void* object358, const ped::CVector* pos, float range);
GAME_FN(uint8_t, __thiscall, PT_Fn48D470, 0x0048D470, void* objectA64, CEntity* entity);
GAME_FN(uint8_t, __thiscall, PT_Is1D24Kind3, 0x00473220, CEntity* ped);
GAME_FN(void, __cdecl, PT_RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, PT_CleanUpReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
inline int __cdecl PT_rand() { return crt::Rand(); }
GAME_FN(float*, __thiscall, PT_GetPositionPtr, 0x00415460, CPed* ped);
GAME_FN(ped::CVector*, __cdecl, PT_VectorSub, 0x00414B70, ped::CVector* out, const float* a, const ped::CVector* b);
GAME_FN(float, __cdecl, PT_Distance, 0x0041A950, const float* a, const ped::CVector* b);
GAME_FN(CPed*, __thiscall, PT_ChoicePick, 0x004884A0, TargetChoice* choice);
GAME_FN(void, __thiscall, PT_SetTargetEntity, 0x00498B10, CPed* ped, CEntity* target);
GAME_FN(float, __cdecl, PT_SafeAcos, 0x00413D60, float cosine);
GAME_FN(float, __cdecl, PT_LimitAngle, 0x00432EE0, float angle);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __cdecl, PT_IsKind0Or2, 0x00496AB0, int kind);
GAME_FN(uint8_t, __cdecl, PT_IsKind2Or3, 0x00496AD0, int kind);
GAME_FN(uint8_t, __thiscall, PT_DropAttackOn, 0x00496C00, CPed* ped, CEntity* target);
GAME_FN(CEntity*, __thiscall, PT_GetObjectiveTarget, 0x00496CB0, CPed* ped, int* kind);
GAME_FN(uint8_t, __thiscall, PT_IsAlly, 0x00496F30, CPed* ped, CEntity* other);
GAME_FN(uint8_t, __thiscall, PT_MayPick, 0x00496FB0, CPed* ped, CEntity* other);
GAME_FN(void, __thiscall, PT_Fn497030, 0x00497030, CPed* ped);
GAME_FN(uint8_t, __thiscall, PT_CanAttack, 0x004971D0, CPed* ped, CEntity* target, int kind, int checkRange, int notIfDown);
GAME_FN(void, __thiscall, PT_SetTarget, 0x00497E70, CPed* ped, CEntity* target, int kind, const ped::CVector* aimPoint);
GAME_FN(uint8_t, __thiscall, PT_TryObjectiveTarget, 0x00498A00, CPed* ped);
GAME_FN(uint8_t, __thiscall, PT_TryAttacker, 0x00498A80, CPed* ped);
GAME_FN(uint8_t, __thiscall, PT_PickNearby, 0x00499110, CPed* ped);
GAME_FN(CEntity*, __thiscall, PT_ChooseTarget, 0x004993F0, CPed* ped);

re::Ptr32<CEntity>& Attacker(CPed* ped) {
    return F<CEntity*>(ped, 0x1DBC);
}

// ---------------------------------------------------------------------------------
// Helpers (cdecl)

// 0x4969A0: the angle between a and b (b negated when `negate`) within `degrees`
uint8_t __cdecl IsWithinAngle(const ped::CVector* a, const ped::CVector* b, float degrees, int negate) {
    float radians = float(double(degrees) * 3.1415927410125732 / 180.0);
    float dot;
    if (re::Flag(negate)) {
        ped::CVector n = {-b->x, -b->y, -b->z};
        dot = float(double(a->y) * n.y + double(a->x) * n.x + double(a->z) * n.z);
    } else {
        dot = float(double(a->y) * b->y + double(a->x) * b->x + double(a->z) * b->z);
    }
    float angle = PT_SafeAcos(dot);
    angle = std::fabs(angle);
    return radians >= PT_LimitAngle(angle);
}

// 0x496AB0: the kinds that need the range checks
uint8_t __cdecl IsKind0Or2(int kind) {
    return kind == 0 || kind == 2;
}

// 0x496AD0
uint8_t __cdecl IsKind2Or3(int kind) {
    return kind >= 2 && kind <= 3;
}

// 0x496AF0: the kind from two flags; returns out
int* __cdecl MakeKind(int* out, int first, int second) {
    if (re::Flag(first)) {
        *out = re::Flag(second) ? 2 : 3;
    } else {
        *out = re::Flag(second) ? 0 : 1;
    }
    return out;
}

// 0x496B30: the objective if it's of type 0x16
Objective* __cdecl AsType16(Objective* objective) {
    if (!objective) {
        return nullptr;
    }
    return objective->type == 0x16 ? objective : nullptr;
}

// ---------------------------------------------------------------------------------
// CPed (thiscall, as __fastcall with the unused edx)

// 0x496990
float __fastcall GetAttackerTime(CPed* self) {
    return F<float>(self, 0x1DC0);
}

// 0x496BB0
void __fastcall ResetTarget(CPed* self) {
    F<float>(self, 0x1DB8) = 0.0f;
    F<uint32_t>(self, 0x1DC4) = 0;
    F<float>(self, 0x1DC0) = 0.0f;
    F<int32_t>(self, 0x1DB4) = 0;
    F<uint8_t>(self, 0x1DE4) = 1;
    F<uint8_t>(self, 0x1DE5) = 0;
    F<ped::CVector>(self, 0x1DC8) = PT_defaultAimPoint;
}

// 0x496C00: the current objective dropped if it attacks the ped (7) or breaks it (8)
uint8_t __fastcall DropAttackOn(CPed* self, re::Edx, CEntity* target) {
    ObjectiveQueue* queue = &self->objectives;
    Objective* current = PT_QueueGetCurrent(queue);
    if (!target || EntityType(target) != 3 || !current) {
        return 0;
    }
    if (current->type == OBJECTIVE_ATTACK_PED && PT_Get14(current) == target) {
        PT_QueueRemoveFirst(queue);
        return 1;
    }
    if (current->type == OBJECTIVE_BREAK_PROP && PT_Get1C(current) == target) {
        PT_QueueRemoveFirst(queue);
        return 1;
    }
    return 0;
}

// 0x496C70: whom the current objective attacks (AttackPed, or Cover's)
CEntity* __fastcall GetAttackObjectiveTarget(CPed* self) {
    Objective* current = PT_QueueGetCurrent(&self->objectives);
    CEntity* target = nullptr;
    if (!current) {
        return nullptr;
    }
    if (current->type == OBJECTIVE_ATTACK_PED) {
        target = PT_Get14(current);
    }
    if (!target && current->type == OBJECTIVE_COVER) {
        return PT_Get14(current);
    }
    return target;
}

// 0x496CB0: what the current objective is about, and its kind (3 unless the objective
// says). A ball objective whose time is up is dropped (nullptr)
CEntity* __fastcall GetObjectiveTarget(CPed* self, re::Edx, int* kind) {
    ObjectiveQueue* queue = &self->objectives;
    Objective* current = PT_QueueGetCurrent(queue);
    CEntity* result = nullptr;
    if (!current) {
        return nullptr;
    }
    *kind = 3;
    if (current->type == OBJECTIVE_ATTACK_PED) {
        result = PT_Get14(current);
        *kind = F<int32_t>(current, 0x10);
    }
    if (current->type == OBJECTIVE_ATTACK_VEHICLE) {
        result = F<CEntity*>(current, 0x10);
    }
    if (current->type == OBJECTIVE_COVER) {
        result = PT_Get14(current);
        *kind = F<int32_t>(current, 0x10);
    }
    if (current->type == OBJECTIVE_BREAK_PROP) {
        result = PT_Get1C(current);
        *kind = F<int32_t>(current, 0x20);
    }
    int type = current->type;
    if (type == OBJECTIVE_FACE_ENTITY) {
        result = F<CEntity*>(current, 0x14);
    }
    if (type == 0x16) {
        result = F<CEntity*>(current, 0x10);
    }
    if (type == OBJECTIVE_PLAY_CATCH) {
        if (PT_BallIsTimeUp(current)) {
            PT_QueueRemoveFirst(queue);
            return nullptr;
        }
        result = F<CEntity*>(current, 0x10);
    }
    if (current->type == OBJECTIVE_PASS_BALL) {
        if (PT_BallIsTimeUpF(current)) {
            PT_QueueRemoveFirst(queue);
            return nullptr;
        }
        result = F<CEntity*>(current, 0x10);
    }
    if (current->type == OBJECTIVE_RECEIVE_BALL) {
        if (PT_BallIsTimeUpF(current)) {
            PT_QueueRemoveFirst(queue);
            return nullptr;
        }
        result = F<CEntity*>(current, 0x10);
    }
    return result;
}

// 0x496D80: the queue's attack objectives (AttackPed, Cover): the one on `keep` is moved
// to the place of the last of them; returns the first one on someone else (told to
// +0x3BC), nullptr when the one on `keep` is already last
CEntity* __fastcall ReorderAttackObjectives(CPed* self, re::Edx, CEntity* keep) {
    ObjectiveQueue* queue = &self->objectives;
    uint32_t count = queue->count;
    if (count == 0) {
        return nullptr;
    }
    bool haveFirst = false;
    CEntity* first = nullptr;
    uint32_t keepIndex = 0xFFFFFFFF;
    uint32_t lastIndex = 0xFFFFFFFF;
    if (int(count) <= 0) {
        return first;
    }
    for (uint32_t i = 0; int(i) < int(count); i++) {
        Objective* objective = i < queue->count ? queue->entries[i] : nullptr;
        if (!objective) {
            continue;
        }
        CEntity* target = nullptr;
        if (objective->type == OBJECTIVE_ATTACK_PED) {
            target = PT_Get14(objective);
        }
        if (!target) {
            if (objective->type != OBJECTIVE_COVER) {
                continue;
            }
            target = PT_Get14(objective);
            if (!target) {
                continue;
            }
        }
        lastIndex = i;
        if (target == keep) {
            keepIndex = i;
        } else if (!haveFirst) {
            first = target;
            haveFirst = true;
        }
    }
    if (keepIndex != 0xFFFFFFFF) {
        if (keepIndex == lastIndex) {
            return nullptr;
        }
        Objective* objective = keepIndex < queue->count ? queue->entries[keepIndex] : nullptr;
        PT_ObjectiveAddRef(objective);
        PT_QueueRemoveAt(queue, int(keepIndex));
        PT_QueueInsert(queue, objective, lastIndex);
        PT_ObjectiveRelease(objective);
    }
    if (first) {
        PT_Fn48C500(self->unknown3BC, first);
    }
    return first;
}

// 0x496EC0: the aim point (+0x1DC8) on +0x538's entity, or the targeting's, in the
// world (zero without one)
ped::CVector* __fastcall GetAimPoint(CPed* self, re::Edx, ped::CVector* out) {
    CEntity* entity = self->unknown538;
    if (!entity) {
        entity = PT_TargetingGetA0(self->targeting);
    }
    // (MultiplyPoint writes a fourth float: the game's is its stack's)
    float point[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (entity) {
        PT_MultiplyPoint(point, PT_GetMatrix(entity), &F<ped::CVector>(self, 0x1DC8));
    }
    out->x = point[0];
    out->y = point[1];
    out->z = point[2];
    return out;
}

// 0x496F30: an ally (with +0x14BC set): a friend of +0x2F0's (0x485590),
// the same ped type, or one it has attitude 4 towards (never ped type 6)
uint8_t __fastcall IsAlly(CPed* self, re::Edx, CEntity* other) {
    if (!other || EntityType(other) != 3) {
        return 0;
    }
    CPed* otherPed = reinterpret_cast<CPed*>(other);
    if (F<int32_t>(self, 0x14BC) == 0 || otherPed->pedType == 6) {
        return 0;
    }
    if (PT_Fn485590(&self->unknown2F0, other)) {
        return 1;
    }
    if (self->pedType == otherPed->pedType) {
        return 1;
    }
    int attitude;
    PT_GetAttitude(self, &attitude, otherPed->pedType);
    return attitude == 4;
}

// 0x496FB0: another ped, standing, that this one dislikes (attitude <= 0), or (0x41AA50)
// one with enough trouble points
uint8_t __fastcall MayPick(CPed* self, re::Edx, CEntity* other) {
    if (!other || other == reinterpret_cast<CEntity*>(self) || EntityType(other) != 3) {
        return 0;
    }
    CPed* otherPed = reinterpret_cast<CPed*>(other);
    if (otherPed->unknown1C98 != 0) {
        return 0;
    }
    int attitude;
    PT_GetAttitude(self, &attitude, otherPed->pedType);
    if (attitude <= 0) {
        return 1;
    }
    if (!PT_Fn41AA50(self)) {
        return 0;
    }
    return otherPed->punishment >= F<int32_t>(PT_aibutes, 0x6C) ? 1 : 0;
}

// 0x497020
CEntity* __fastcall GetAttacker(CPed* self) {
    return Attacker(self);
}

// 0x497030: attacked recently (within 5 s) by someone it's not friends with while its
// current objective attacks someone else in a ranged kind: attack the attacker first
void __fastcall RespondToAttacker(CPed* self) {
    CEntity* attacker = Attacker(self);
    if (!attacker || F<float>(self, 0x1DC0) > 5.0f) {
        return;
    }
    if (PT_IsAlly(self, attacker)) {
        return;
    }
    ObjectiveQueue* queue = &self->objectives;
    Objective* current = PT_QueueGetCurrent(queue);
    if (!current || current->type != OBJECTIVE_ATTACK_PED) {
        return;
    }
    if (!PT_Get14(current)) {
        return;
    }
    CEntity* stored = Attacker(self);
    if (PT_Get14(current) == stored) {
        return;
    }
    if (PT_IsKind2Or3(F<int32_t>(current, 0x10))) {
        return;
    }
    void* mem = PT_Malloc(0x1C);
    if (mem) {
        PT_QueuePrepend(queue, PT_AttackPedConstruct(mem, self, Attacker(self), 0));
        return;
    }
    PT_QueuePrepend(queue, nullptr);
}

// 0x497180: a ped it isn't friends with and dislikes (attitude <= 0)
uint8_t __fastcall Dislikes(CPed* self, re::Edx, CEntity* other) {
    if (PT_IsAlly(self, other)) {
        return 0;
    }
    if (!other || EntityType(other) != 3) {
        return 0;
    }
    int attitude;
    PT_GetAttitude(self, &attitude, reinterpret_cast<CPed*>(other)->pedType);
    return attitude <= 0;
}

// 0x4971D0: whether it may attack `target` in this kind (the player's own limits, the
// target's state and conversations, range for kinds 0 and 2)
uint8_t __fastcall CanAttack(CPed* self, re::Edx, CEntity* target, int kind, int checkRange, int notIfDown) {
    CPed* targetPed = (target && EntityType(target) == 3) ? reinterpret_cast<CPed*>(target) : nullptr;
    if (PT_Fn5BB380() && PT_Fn4765C0(self)) {
        return 0;
    }
    if (self->pedType == 13) {
        if (F<int32_t>(self, 0x1474) != 0 || self->unknown1470 != 0) {
            return 0;
        }
        if (PT_Fn4CE690(self)) {
            if (!PT_Inventory486BE0(self->inventory)) {
                return 0;
            }
            int model = F<int16_t>(self->weapon, 0x10E);
            if (model != -1) {
                uint8_t* info = PT_GetWeaponInfo(model);
                if (!info || info[0x29] == 0) {
                    return 0;
                }
            }
        }
    }
    if (!target || target == reinterpret_cast<CEntity*>(self)) {
        return 0;
    }
    if (targetPed && targetPed->unknown1C98 != 0 && re::Flag(notIfDown)) {
        return 0;
    }
    int type = self->pedType;
    if (type != 0 && type != 8 && type != 7) {
        if (PT_Fn46EFB0(PT_objC2AB70, self, EntityPos(target), 0)) {
            return 0;
        }
    }
    if (PT_InConversation(PT_conversations, reinterpret_cast<CEntity*>(self))) {
        return 0;
    }
    if (targetPed) {
        if (PT_InConversation(PT_conversations, target)) {
            return 0;
        }
        if (targetPed->pedType == 13 && F<uint8_t>(targetPed, 0x1E90) == 0 && PT_Fn41AA50(self)) {
            return 0;
        }
    }
    if (F<uint8_t>(self, 0x1DE5) != 0) {
        if (targetPed && targetPed->modelIndex == PT_modelBF42DC) {
            return 0;
        }
        if (self->pedType != 13) {
            goto rangeChecks;
        }
        if (targetPed) {
            if (PT_Fn4CE690(self)) {
                return 0;
            }
            if (PT_Fn4CE690(targetPed)) {
                return 0;
            }
        }
    }
    if (self->pedType == 13) {
        return 1;
    }
rangeChecks:
    if (!PT_IsKind0Or2(kind)) {
        return 1;
    }
    if (targetPed) {
        if (PT_Fn48C080(self->unknown3BC, target)) {
            return 0;
        }
        if (F<int32_t>(self, 0x1360) == 1) {
            return 0;
        }
    }
    uint8_t* aibutes = PT_aibutes;
    if (re::Flag(checkRange)) {
        if (PT_DistanceSqMinus(EntityPos(self), EntityPos(target), F<float>(aibutes, 0x68)) > 0.0f) {
            return 0;
        }
        aibutes = PT_aibutes;
    }
    if (targetPed && PT_Fn41AA50(self) && targetPed->punishment < F<int32_t>(aibutes, 0x6C)) {
        return 0;
    }
    float reach = F<float>(aibutes, 0x70);
    if (target == PT_GetTarget(self)) {
        reach = F<float>(PT_aibutes, 0x74);
    }
    if (PT_HasWeapon(self)) {
        CEntity* weapon = self->weapon;
        if (!(PT_Weapon4D96B0(weapon) == 0.0f)) {
            reach = F<float>(weapon, 0x120) + reach;
        }
    }
    if (!PT_Fn499840(&F<uint8_t>(self, 0x358), EntityPos(target), reach)) {
        return 0;
    }
    if (PT_IsKind2Or3(kind) || !re::Flag(checkRange) || PT_Fn48D470(self->unknownA64, target)) {
        return 1;
    }
    if (target != PT_GetTarget(self)) {
        return 0;
    }
    // (kept on the current target until +0x1DB8 reaches the tuning's +0x64)
    return !(F<float>(PT_aibutes, 0x64) <= F<float>(self, 0x1DB8)) ? 1 : 0;
}

// 0x497520: the attacker (registered), its time from 0
void __fastcall SetAttacker(CPed* self, re::Edx, CEntity* attacker) {
    re::Ptr32<CEntity>* slot = &Attacker(self);
    if (attacker != *slot) {
        if (*slot) {
            PT_CleanUpReference(*slot, slot);
        }
        *slot = attacker;
        if (attacker) {
            PT_RegisterReference(attacker, slot);
        }
    }
    F<float>(self, 0x1DC0) = 0.0f;
}

// 0x497E70: the target (not a ped of kind 3's +0x1D24 for peds other than the player),
// its kind and aim point; the next look in the tuning's +0x78 .. +0x7C seconds
void __fastcall SetTarget(CPed* self, re::Edx, CEntity* target, int kind, const ped::CVector* aimPoint) {
    if (target && self->pedType != 13 && EntityType(target) == 3 && PT_Is1D24Kind3(target)) {
        return;
    }
    if (target != PT_GetTarget(self)) {
        CEntity* old = PT_GetTarget(self);
        if (old && EntityType(old) == 3) {
            PT_Inventory486F50(reinterpret_cast<CPed*>(old)->inventory, self);
        }
        PT_Inventory487B70(self->inventory);
        PT_Inventory486D50(self->inventory);
        F<float>(self, 0x1DB8) = 0.0f;
    }
    PT_Fn48C500(self->unknown3BC, target);
    PT_Targeting4D79E0(self->targeting, target);
    F<int32_t>(self, 0x1DB4) = kind;
    F<ped::CVector>(self, 0x1DC8) = *aimPoint;
    F<uint32_t>(self, 0x1DC4) = PT_timeInMs;
    float low = F<float>(PT_aibutes, 0x78);
    float high = F<float>(PT_aibutes, 0x7C);
    int r = PT_rand();
    float seconds = float(double(r) / 32767.0 * (double(high) - double(low)) + double(low));
    F<uint32_t>(self, 0x1DC4) += uint32_t(int(double(seconds) * 1000.0));
}

// 0x497F90: CanAttack, not if the target is down
uint8_t __fastcall CanAttackStanding(CPed* self, re::Edx, CEntity* target, int kind, int checkRange) {
    return PT_CanAttack(self, target, kind, checkRange, 1);
}

// 0x497FB0: SetTarget aiming at the target's origin
void __fastcall SetTargetAtOrigin(CPed* self, re::Edx, CEntity* target, int kind) {
    ped::CVector origin = {0.0f, 0.0f, 0.0f};
    PT_SetTarget(self, target, kind, &origin);
}

// 0x498A00: the current objective's target, if it may be attacked (+0x1DE5 set meanwhile)
uint8_t __fastcall TryObjectiveTarget(CPed* self) {
    int kind;
    CEntity* target = PT_GetObjectiveTarget(self, &kind);
    if (!target) {
        return 0;
    }
    uint8_t saved = F<uint8_t>(self, 0x1DE5);
    F<uint8_t>(self, 0x1DE5) = 1;
    if (PT_CanAttack(self, target, kind, 0, 1)) {
        ped::CVector origin = {0.0f, 0.0f, 0.0f};
        PT_SetTarget(self, target, kind, &origin);
        F<uint8_t>(self, 0x1DE5) = saved;
        return 1;
    }
    F<uint8_t>(self, 0x1DE5) = saved;
    return 0;
}

// 0x498A80: the attacker of the last 5 s, if it may be picked and attacked
uint8_t __fastcall TryAttacker(CPed* self) {
    if (!Attacker(self)) {
        return 0;
    }
    if (!(F<float>(self, 0x1DC0) < 5.0f)) {
        return 0;
    }
    if (!PT_MayPick(self, Attacker(self))) {
        return 0;
    }
    if (!PT_CanAttack(self, Attacker(self), 0, 0, 1)) {
        return 0;
    }
    ped::CVector origin = {0.0f, 0.0f, 0.0f};
    PT_SetTarget(self, Attacker(self), 0, &origin);
    return 1;
}

// 0x499110: (with peds in +0x3C0's list) a ped within 100 it may pick, at random by
// 1 / (distance + 0.01), halved for each of the list's standing peds already on it
uint8_t __fastcall PickNearby(CPed* self) {
    if (F<int32_t>(self, 0x3C0) <= 0) {
        return 0;
    }
    CPool<CPed>* pool = PT_pedPool;
    TargetChoice choice;
    choice.total = 0.0f;
    choice.count = 0;
    if (pool->size <= 0) {
        return 0;
    }
    for (int i = 0; i < pool->size; i++) {
        if (pool->flags[i] & 0x80) {
            continue;
        }
        CPed* ped = reinterpret_cast<CPed*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * i);
        if (!ped) {
            continue;
        }
        if (!PT_MayPick(self, reinterpret_cast<CEntity*>(ped))) {
            continue;
        }
        ped::CVector d;
        PT_VectorSub(&d, PT_GetPositionPtr(self), EntityPos(ped));
        float lengthSq = float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z);
        float over = float(double(lengthSq) - 10000.0);
        if (over > 0.0f) {
            continue;
        }
        float distance = PT_Distance(PT_GetPositionPtr(self), EntityPos(ped));
        float weight = float(1.0 / (double(distance) + 0.009999999776482582));
        for (int j = 0; j < F<int32_t>(self, 0x3C0); j++) {
            CPed* other = F<CPed*>(self, 0x3C4 + j * 4);
            if (other && other->unknown1C98 == 0 && PT_GetTarget(other) == reinterpret_cast<CEntity*>(ped)) {
                weight = float(double(weight) * 0.5);
            }
        }
        if (!(weight <= 0.0f) && choice.count < 16) {
            choice.entries[choice.count].weight = weight;
            choice.entries[choice.count].ped = ped;
            choice.count++;
            choice.total = weight + choice.total;
        }
    }
    if (choice.count <= 0) {
        return 0;
    }
    CPed* picked = PT_ChoicePick(&choice);
    int notCover = 1;
    Objective* current = PT_QueueGetCurrent(&self->objectives);
    if (current && current->type == OBJECTIVE_COVER) {
        notCover = 0;
    }
    if (!PT_CanAttack(self, reinterpret_cast<CEntity*>(picked), 0, notCover, 1)) {
        return 0;
    }
    ped::CVector origin = {0.0f, 0.0f, 0.0f};
    PT_SetTarget(self, reinterpret_cast<CEntity*>(picked), 0, &origin);
    return 1;
}

// 0x4993F0: a new target (the objective's, the attacker, one nearby) unless the current
// one is kept (a kind other than 0 and 2, or not yet time to look); the target after
CEntity* __fastcall ChooseTarget(CPed* self) {
    if (self->pedType != 13) {
        PT_Fn497030(self);
    }
    if (PT_TryObjectiveTarget(self)) {
        return PT_GetTarget(self);
    }
    if (PT_GetTarget(self)) {
        int kind = F<int32_t>(self, 0x1DB4);
        if ((kind != 0 && kind != 2) || F<uint32_t>(self, 0x1DC4) > PT_timeInMs) {
            return PT_GetTarget(self);
        }
    }
    if (PT_TryAttacker(self)) {
        return PT_GetTarget(self);
    }
    PT_PickNearby(self);
    return PT_GetTarget(self);
}

// 0x499470: ChooseTarget when asked (+0x1DE4)
CEntity* __fastcall ChooseTargetIfAsked(CPed* self) {
    if (F<uint8_t>(self, 0x1DE4) != 0) {
        F<uint8_t>(self, 0x1DE4) = 0;
        PT_ChooseTarget(self);
    }
    return PT_GetTarget(self);
}

// 0x499490: the timers step; a target it may no longer attack is dropped (and its
// objective), then a new one chosen. (Nothing for the player or with +0x13E4: there the
// game returns whatever eax held; ours nullptr)
CEntity* __fastcall UpdateTarget(CPed* self) {
    F<uint8_t>(self, 0x1DE4) = 1;
    if (Attacker(self)) {
        float step = PT_timeStep * PT_timeScaleC1A9A4;
        F<float>(self, 0x1DC0) = step + F<float>(self, 0x1DC0);
    } else {
        F<float>(self, 0x1DC0) = 0.0f;
    }
    if (self->pedType == 13 || F<int32_t>(self, 0x13E4) != 0) {
        return nullptr;
    }
    if (PT_GetTarget(self)) {
        if (PT_Fn48D470(self->unknownA64, PT_GetTarget(self))) {
            F<float>(self, 0x1DB8) = 0.0f;
        } else {
            float step = PT_timeStep * PT_timeScaleC1A9A4;
            F<float>(self, 0x1DB8) = step + F<float>(self, 0x1DB8);
        }
        int kind = F<int32_t>(self, 0x1DB4);
        if (!PT_CanAttack(self, PT_GetTarget(self), kind, 1, 1)) {
            PT_DropAttackOn(self, PT_GetTarget(self));
            PT_SetTargetEntity(self, nullptr);
            PT_Targeting4D79E0(self->targeting, nullptr);
        }
    }
    if (F<uint8_t>(self, 0x1DE4) != 0) {
        F<uint8_t>(self, 0x1DE4) = 0;
        PT_ChooseTarget(self);
    }
    return PT_GetTarget(self);
}

} // namespace

#define C08P3_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P3_REVERSED("PedTarget::IsWithinAngle", 0x004969A0, &IsWithinAngle);
C08P3_REVERSED("PedTarget::IsKind0Or2", 0x00496AB0, &IsKind0Or2);
C08P3_REVERSED("PedTarget::IsKind2Or3", 0x00496AD0, &IsKind2Or3);
C08P3_REVERSED("PedTarget::MakeKind", 0x00496AF0, &MakeKind);
C08P3_REVERSED("PedTarget::AsType16", 0x00496B30, &AsType16);
C08P3_REVERSED("CPed::GetAttackerTime", 0x00496990, &GetAttackerTime);
C08P3_REVERSED("CPed::ResetTarget", 0x00496BB0, &ResetTarget);
C08P3_REVERSED("CPed::DropAttackOn", 0x00496C00, &DropAttackOn);
C08P3_REVERSED("CPed::GetAttackObjectiveTarget", 0x00496C70, &GetAttackObjectiveTarget);
C08P3_REVERSED("CPed::GetObjectiveTarget", 0x00496CB0, &GetObjectiveTarget);
C08P3_REVERSED("CPed::ReorderAttackObjectives", 0x00496D80, &ReorderAttackObjectives);
C08P3_REVERSED("CPed::GetAimPoint", 0x00496EC0, &GetAimPoint);
C08P3_REVERSED("CPed::IsAlly", 0x00496F30, &IsAlly);
C08P3_REVERSED("CPed::MayPick", 0x00496FB0, &MayPick);
C08P3_REVERSED("CPed::GetAttacker", 0x00497020, &GetAttacker);
C08P3_REVERSED("CPed::RespondToAttacker", 0x00497030, &RespondToAttacker);
C08P3_REVERSED("CPed::Dislikes", 0x00497180, &Dislikes);
C08P3_REVERSED("CPed::CanAttack", 0x004971D0, &CanAttack);
C08P3_REVERSED("CPed::SetAttacker", 0x00497520, &SetAttacker);
C08P3_REVERSED("CPed::SetTarget", 0x00497E70, &SetTarget);
C08P3_REVERSED("CPed::CanAttackStanding", 0x00497F90, &CanAttackStanding);
C08P3_REVERSED("CPed::SetTargetAtOrigin", 0x00497FB0, &SetTargetAtOrigin);
C08P3_REVERSED("CPed::TryObjectiveTarget", 0x00498A00, &TryObjectiveTarget);
C08P3_REVERSED("CPed::TryAttacker", 0x00498A80, &TryAttacker);
C08P3_REVERSED("CPed::PickNearby", 0x00499110, &PickNearby);
C08P3_REVERSED("CPed::ChooseTarget", 0x004993F0, &ChooseTarget);
C08P3_REVERSED("CPed::ChooseTargetIfAsked", 0x00499470, &ChooseTargetIfAsked);
C08P3_REVERSED("CPed::UpdateTarget", 0x00499490, &UpdateTarget);
