// The action tree's conditions: the base (IsA, the compare helpers), the condition lists
// (a node's conditions, evaluated with their and / not / or joints), the group condition,
// the conditions' deleting destructors (one shape with a name key at +4, one without)
// and their field initialisers, and the first evaluators. 0x5FB7F0..0x5FC770.
//
// A condition: +0 vtable {IsA, Evaluate(owner), Joint, DeletingDestruct, ...}; the
// owner is the ped's action context (+0 its controller, +8 the ped, +0x10 its hit state).
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cstdint>

#include "game/ActionTree.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace acttree;

namespace {

// the condition list (a node's conditions): the first inline, the rest in an array
struct ConditionList {
    re::Ptr32<uint8_t> first;         // +0
    re::Ptr32<re::Ptr32<uint8_t>> rest;         // +4
    uint32_t restCapacity;  // +8
    uint32_t count;         // +0xC
};
static_assert(sizeof(ConditionList) == 0x10);
uint8_t* ListElem(ConditionList* list, uint32_t i) {
    return i < 1 ? (&list->first)[i] : list->rest[i - 1];
}

GAME_VAR(uint32_t, conditionIdGuard, 0x01B64A90);
GAME_VAR(int16_t, conditionId, 0x01B64A8C);
GAME_VAR(uint32_t, trackOrConditionIdGuard, 0x00C0F5B4);
GAME_VAR(int16_t, trackOrConditionId, 0x00C0F5B0);
GAME_VAR(float, compareEpsilon, 0x008FF230);   // 1e-5
GAME_VAR(int32_t, cameraMode_C3CCF8, 0x00C3CCF8);   // 2: first person
GAME_VAR(uint8_t, playerPhysicalState_C1AF4C, 0x00C1AF4C);
inline void* const outfits_20C5F70 = reinterpret_cast<void*>(0x020C5F70);
inline void* const missionManager_20C3CA0 = reinterpret_cast<void*>(0x020C3CA0);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);
GAME_FN(uint32_t*, __thiscall, NameKey_Default, 0x0066BEA0, uint32_t* key);
GAME_FN(void, __thiscall, NameKey_FromString, 0x005FA890, uint32_t* key, const char* name);
GAME_FN(uint32_t*, __thiscall, NameKey_FromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(void, __thiscall, StringRef_Assign, 0x005772F0, uint32_t* field, uint32_t value);
GAME_FN(uint8_t, __cdecl, NearlyEqual, 0x00432A60, float a, float b, float epsilon);
GAME_FN(uint8_t, __cdecl, CompareFloats, 0x005FB8B0, float a, float b, int op);
GAME_FN(void, __thiscall, ConditionList_ReserveRest, 0x005FBA30, ConditionList* list, uint32_t count);
GAME_FN(void, __thiscall, ConditionList_Grow, 0x005FBCC0, ConditionList* list);
GAME_FN(uint8_t, __thiscall, ConditionList_Evaluate, 0x005FBB10, ConditionList* list, uint8_t* owner);
GAME_FN(uint8_t, __thiscall, Outfits_IsWorn_6CA9F0, 0x006CA9F0, void* outfits, int32_t outfit);
GAME_FN(uint8_t, __cdecl, Clock_IsDayTime_425FE0, 0x00425FE0);
GAME_FN(uint8_t, __thiscall, MissionManager_IsActive_6AA8F0, 0x006AA8F0, void* manager);
GAME_FN(uint8_t*, __thiscall, TrackOrCondition_Construct, 0x0061A600, uint8_t* self);

// the conditions' deleting destructors
template <uint32_t Vt>
uint8_t* __fastcall ConditionDeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    vc::At<uint32_t>(self, 0) = Vt;
    if (flags & 1) {
        Free(self);
    }
    return self;
}
template <uint32_t Vt>
uint8_t* __fastcall ConditionWithKeyDeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    vc::At<uint32_t>(self, 0) = Vt;
    NameKey_Destruct(self + 4);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

uint8_t ConditionIsA(int id) {
    if (int16_t(id) == TypeId(conditionIdGuard, conditionId)) {
        return 1;
    }
    return int16_t(id) == TypeId(trackOrConditionIdGuard, trackOrConditionId);
}

// 0x5FB7F0 (thiscall) / 0x5FC700 (cdecl): Condition::IsA
uint8_t __fastcall Condition_IsA(uint8_t*, re::Edx, int id) {
    return ConditionIsA(id);
}
uint8_t __cdecl Condition_IsAStatic(int id) {
    return ConditionIsA(id);
}

// 0x5FB8B0 (cdecl): a float compared: 0 about equal, 1 not, 2 less, 3 greater, 4 less or
// about equal, 5 greater or about equal
uint8_t __cdecl CompareFloatsImpl(float a, float b, int op) {
    switch (op) {
    case 0:
        return NearlyEqual(a, b, compareEpsilon) ? 1 : 0;
    case 1:
        return NearlyEqual(a, b, compareEpsilon) ? 0 : 1;
    case 2:
        return b > a ? 1 : 0;
    case 3:
        return b < a ? 1 : 0;
    case 4:
        if (b > a) {
            return 1;
        }
        return NearlyEqual(a, b, compareEpsilon) ? 1 : 0;
    case 5:
        if (b < a) {
            return 1;
        }
        return NearlyEqual(a, b, compareEpsilon) ? 1 : 0;
    default:
        return 0;
    }
}

// 0x5FB9B0 (cdecl): an int compared (0 ==, 1 !=, 2 <, 3 >, 4 <=, 5 >=)
uint8_t __cdecl CompareInts(int32_t a, int32_t b, int op) {
    switch (op) {
    case 0: return a == b;
    case 1: return a != b;
    case 2: return a < b;
    case 3: return a > b;
    case 4: return a <= b;
    case 5: return a >= b;
    default: return 0;
    }
}
// 0x5FBCB0: a lone jump to CompareInts
RE_UNHOOKED_FN("ConditionScriptSaveData::CompareInts", 0x005FBCB0, &CompareInts);

// 0x5FBA30 (thiscall): the rest's array grown to `count`
void __fastcall ConditionList_ReserveRestImpl(ConditionList* self, re::Edx, uint32_t count) {
    if (count <= self->restCapacity) {
        return;
    }
    uint64_t bytes = uint64_t(count) * 4;
    re::Ptr32<uint8_t>* rest = static_cast<re::Ptr32<uint8_t>*>(Malloc(bytes > 0xFFFFFFFFu ? size_t(-1) : size_t(bytes)));
    if (self->rest) {
        for (uint32_t i = 0; self->count - 1 != 0 && i < self->count - 1; i++) {
            rest[i] = self->rest[i];
        }
        Free(self->rest);
    }
    self->rest = rest;
    self->restCapacity = count;
}

// 0x5FBAA0 (thiscall): ConditionActionNodePlayTime: the controller's time compared
uint8_t __fastcall ActionNodePlayTime_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return CompareFloats(vc::At<float>(vc::At<uint8_t*>(owner, 0), 8), vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FBAD0 (thiscall): the list's constructor (the first slot left)
ConditionList* __fastcall ConditionList_Construct(ConditionList* self, re::Edx) {
    self->rest = nullptr;
    self->restCapacity = 0;
    self->count = 0;
    return self;
}

// 0x5FBAE0 (thiscall): the list's destructor
void __fastcall ConditionList_Destruct(ConditionList* self, re::Edx) {
    self->count = 0;
    if (self->rest) {
        Free(self->rest);
        self->rest = nullptr;
    }
}

// 0x5FBB10 (thiscall): all the conditions evaluated in order, each joined to the result
// by the one before's joint (0 and, 1 and not, 2 or: the rest of an or skipped once it
// holds); a failure ends it unless an or follows
uint8_t __fastcall ConditionList_EvaluateImpl(ConditionList* self, re::Edx, uint8_t* owner) {
    uint32_t count = self->count;
    if (count == 0) {
        return 1;
    }
    if (count == 1) {
        return vc::Call<uint8_t>(self->first, 4, owner);
    }
    int32_t joint = 0;
    uint8_t orHolds = 0;
    uint8_t result = 1;
    for (uint32_t i = 0; i < count;) {
        uint8_t* condition = ListElem(self, i);
        uint8_t value = vc::Call<uint8_t>(condition, 4, owner);
        if (!orHolds) {
            if (joint == 0) {
                result = value;
            } else if (joint == 1) {
                result = value == 0;
            } else if (joint == 2) {
                orHolds = 1;
            }
        }
        joint = vc::Call<int32_t>(condition, 8);
        if (orHolds == 1 && joint != 0) {
            orHolds = 0;
        } else if (!result) {
            if (i + 1 >= count) {
                break;
            }
            if (vc::Call<int32_t>(self->rest[i], 8) != 2) {
                break;
            }
            i++;
        }
        i++;
    }
    return result;
}

// 0x5FBC50 (thiscall): ConditionGroupCondition::Evaluate: its list's (none: true)
uint8_t __fastcall GroupCondition_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    ConditionList* list = vc::At<ConditionList*>(self, 4);
    if (list) {
        return ConditionList_Evaluate(list, owner);
    }
    return 1;
}

// 0x5FBC70 (thiscall)
uint32_t __fastcall GroupCondition_NumConditions(uint8_t* self, re::Edx) {
    return vc::At<ConditionList*>(self, 4)->count;
}

// 0x5FBC80 (thiscall)
uint8_t* __fastcall GroupCondition_GetCondition(uint8_t* self, re::Edx, int i) {
    ConditionList* list = vc::At<ConditionList*>(self, 4);
    if (i < 0 || i >= int(list->count)) {
        return nullptr;
    }
    return ListElem(list, uint32_t(i));
}

// 0x5FBCC0 (thiscall): the rest grown to the next step
void __fastcall ConditionList_GrowImpl(ConditionList* self, re::Edx) {
    uint32_t capacity = self->restCapacity;
    if (capacity < 2) {
        ConditionList_ReserveRest(self, 2);
    } else if (capacity < 4) {
        ConditionList_ReserveRest(self, 4);
    } else if (capacity < 6) {
        ConditionList_ReserveRest(self, 6);
    } else if (capacity < 0xC) {
        ConditionList_ReserveRest(self, 0xC);
    } else if (capacity < 0x23) {
        ConditionList_ReserveRest(self, 0x23);
    } else {
        ConditionList_ReserveRest(self, capacity * 2);
    }
}

// 0x5FBD30 (thiscall): room for `count` (the first is inline)
void __fastcall ConditionList_Reserve(ConditionList* self, re::Edx, uint32_t count) {
    if (count > 1) {
        ConditionList_ReserveRest(self, count - 1);
    }
}

// 0x5FBD50 (thiscall)
void __fastcall ConditionList_Add(ConditionList* self, re::Edx, uint8_t* condition) {
    if (self->count == self->restCapacity + 1) {
        ConditionList_Grow(self);
    }
    if (self->count < 1) {
        (&self->first)[self->count] = condition;
        self->count++;
        return;
    }
    self->rest[self->count - 1] = condition;
    self->count++;
}

// 0x5FBD90 (thiscall on an action request): its times summed up to `index` (0 or 1;
// else FLT_MAX), adding stopping at FLT_MAX
float __fastcall ActionRequest_SumTimes(uint8_t* self, re::Edx, int index) {
    if (index >= 2) {
        return 3.4028235e38f;
    }
    float sum = 0.0f;
    for (int i = 0; i <= index; i++) {
        if (double(sum) < 3.4028234663852886e38) {
            sum = float(double(sum) + double(vc::At<float>(self, 4 + i * 4)));
        }
    }
    return sum;
}

// 0x5FBE60 (cdecl): where `at` is in a cycle of `length` (0..1) relative to `start`, as a
// fraction of the part before `end` (0..0.5) or after it (0.5..1); 0 out of the cycle
float __cdecl CycleFraction(float at, float length, float start, float end) {
    if (!(at >= 0.0f)) {
        return 0.0f;
    }
    if (!(length >= at)) {
        return 0.0f;
    }
    float s = float(double(start) / double(length));
    float a = float(double(at) / double(length));
    float x = float(double(a) - double(s));
    float e = float(double(end) / double(length));
    float y = float(double(e) - double(s));
    if (x < 0.0f) {
        x = float(double(x) + 1.0);
    }
    if (y < 0.0f) {
        y = float(double(y) + 1.0);
    }
    if (x < y) {
        return float(double(x) / double(y) * 0.5);
    }
    float over = x - y;
    float rest = 1.0f - y;
    float part = over / rest;
    return part * 0.5f + 0.5f;
}

// 0x5FBF40 / 0x5FBF60 / 0x5FBFA0 / 0x5FC020 (thiscall): conditions' fields
void __fastcall Fields_5FBF40(uint8_t* self, re::Edx) {
    vc::At<float>(self, 0xC) = 0.0f;
    vc::At<int32_t>(self, 4) = 0;
    vc::At<int32_t>(self, 8) = 1;
    vc::At<int32_t>(self, 0x10) = 5;
    vc::At<int32_t>(self, 0x14) = 0;
}
void __fastcall Fields_5FBF60(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 7;
    vc::At<float>(self, 0xC) = 0.0f;
    vc::At<int32_t>(self, 8) = 1;
    vc::At<int32_t>(self, 0x10) = 5;
    vc::At<int32_t>(self, 0x14) = 0;
}
void __fastcall Fields_5FBFA0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 0.0f;
    vc::At<float>(self, 8) = 0.785f;
}
void __fastcall Fields_5FC020(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 8) = 5;
    vc::At<float>(self, 4) = 0.0f;
}

// 0x5FBFB0 (cdecl): a 2D vector rotated by the angle
void __cdecl RotateVector2(float angle, float* v) {
    float x = v[0];
    float y = v[1];
    float c = crt::Cosf(angle);
    float s = crt::Sinf(angle);
    v[0] = c * x - s * y;
    v[1] = c * y + s * x;
}

// 0x5FC030 (thiscall): ConditionParametricTime: the owner's parametric value compared
uint8_t __fastcall ParametricTime_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return CompareFloats(*vc::At<float*>(owner, 0x38), vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

uint8_t* HitState(uint8_t* owner) {
    return vc::At<uint8_t*>(owner, 0x10);
}

// 0x5FC060 (thiscall): ConditionDamagePending: the pending damage compared
uint8_t __fastcall DamagePending_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return CompareFloats(vc::At<float>(HitState(owner), 0x38), vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FC0A0 (thiscall): ConditionCollisionFlag: the hit state's flag is this one
uint8_t __fastcall CollisionFlag_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int32_t flag = vc::At<int32_t>(self, 4);
    if (!flag) {
        return 0;
    }
    return vc::At<int32_t>(HitState(owner), 0x64) == flag;
}

// 0x5FC0C0 / 0x5FC0F0 (thiscall): fields
void __fastcall Fields_5FC0C0(uint8_t* self, re::Edx) {
    self[6] = 1;
    self[4] = 0;
    self[5] = 0;
}
void __fastcall Fields_5FC0F0(uint8_t* self, re::Edx) {
    self[4] = 1;
    vc::At<int32_t>(self, 8) = 0;
    self[5] = 0;
}

// 0x5FC0D0 (thiscall): ConditionPedIsInOutfit
uint8_t __fastcall PedIsInOutfit_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return Outfits_IsWorn_6CA9F0(outfits_20C5F70, vc::At<int32_t>(self, 4));
}

// 0x5FC100 (thiscall): ConditionHitFlagClear: the hit flags cleared (true)
uint8_t __fastcall HitFlagClear_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* hit = HitState(owner);
    hit[0x68] = 0;
    vc::At<int32_t>(hit, 0x64) = 0;
    hit[0x34] = 0;
    return 1;
}

// 0x5FC120 (thiscall): ConditionActionNodePlayTimePast: the controller's time past the
// hit state's +0x50
uint32_t __fastcall PlayTimePast_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return vc::At<float>(HitState(owner), 0x50) <= vc::At<float>(vc::At<uint8_t*>(owner, 0), 8) ? 1 : 0;
}

// 0x5FC150 (thiscall): ConditionFirstPerson
uint8_t __fastcall FirstPerson_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return cameraMode_C3CCF8 == 2;
}

// 0x5FC180 .. 0x5FC6E0 (thiscall): fields
void __fastcall Fields_5FC180(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 2.0f;
    self[8] = 0;
    self[9] = 0;
}
void __fastcall Fields_5FC1A0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 2.0f;
    self[8] = 0;
    self[9] = 0;
    self[0xA] = 0;
    self[0xB] = 0;
    self[0xC] = 0;
}
void __fastcall Fields_5FC1C0(uint8_t* self, re::Edx) {
    NameKey_FromString(&vc::At<uint32_t>(self, 4), "");
}
void __fastcall Fields_5FC1D0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
    vc::At<int32_t>(self, 8) = 0;
    vc::At<int32_t>(self, 0xC) = 2;
}
void __fastcall Fields_5FC1E0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 1.0f;
    self[0x10] = 0;
    vc::At<int32_t>(self, 0x14) = 0;
    vc::At<float>(self, 8) = 0.0f;
    vc::At<float>(self, 0xC) = 1.57f;
}
void __fastcall Fields_5FC200(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 1.0f;
    vc::At<float>(self, 8) = 0.0f;
    vc::At<float>(self, 0xC) = 1.57f;
}
void __fastcall Fields_5FC220(uint8_t* self, re::Edx) {
    self[4] = 0;
    vc::At<float>(self, 8) = 3.14159f;
    vc::At<float>(self, 0xC) = 1.57f;
}
void __fastcall Fields_5FC240(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 0x1C) = 0;
    vc::At<float>(self, 4) = -1.0f;
    vc::At<float>(self, 8) = 1.0f;
    vc::At<float>(self, 0x10) = 1.0f;
    vc::At<float>(self, 0x18) = 1.0f;
    vc::At<float>(self, 0xC) = -1.0f;
    vc::At<float>(self, 0x14) = -1.0f;
}
void __fastcall Fields_5FC290(uint8_t* self, re::Edx) {
    NameKey_FromString(&vc::At<uint32_t>(self, 4), "");
    self[8] = 1;
}

// 0x5FC2E0 (thiscall): ConditionIsPlaying's destructor
void __fastcall IsPlaying_Destruct(uint8_t* self, re::Edx) {
    vc::At<uint32_t>(self, 0) = 0x0092E5B8;
    NameKey_Destruct(self + 4);
}

// 0x5FC310 (thiscall): ConditionIsDayTime
uint8_t __fastcall IsDayTime_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return Clock_IsDayTime_425FE0();
}

void __fastcall Fields_5FC320(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 8) = 4;
    vc::At<float>(self, 4) = -0.8f;
}

void ClearKey(uint8_t* self, int at) {
    uint32_t key;
    NameKey_FromValue(&key, 0);
    StringRef_Assign(&vc::At<uint32_t>(self, at), key);
    NameKey_Destruct(&key);
}
void __fastcall Fields_5FC3D0(uint8_t* self, re::Edx) {
    ClearKey(self, 4);
}

// 0x5FC450 (thiscall): ConditionNearPoint's constructor
uint8_t* __fastcall NearPoint_Construct(uint8_t* self, re::Edx) {
    TrackOrCondition_Construct(self);
    vc::At<uint32_t>(self, 0) = 0x0092E658;
    NameKey_Default(&vc::At<uint32_t>(self, 4));
    vc::At<int32_t>(self, 0xC) = -1;
    return self;
}

void __fastcall Fields_5FC480(uint8_t* self, re::Edx) {
    ClearKey(self, 4);
    vc::At<float>(self, 8) = 0.0f;
}
void __fastcall Fields_5FC4B0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0xC;
}
void __fastcall Fields_5FC4E0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 12.0f;
}
void __fastcall Fields_5FC4F0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0x10;
}

// 0x5FC500 / 0x5FC510 (thiscall): the player's physical state is fatigued / tired
uint32_t __fastcall IsPlayerFatigued_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return playerPhysicalState_C1AF4C == 1 ? 1 : 0;
}
uint32_t __fastcall IsPlayerTired_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return playerPhysicalState_C1AF4C == 2 ? 1 : 0;
}

void __fastcall Fields_5FC520(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 0.0f;
    vc::At<int32_t>(self, 0xC) = 0;
    self[9] = 0;
    self[8] = 0;
    self[0x10] = 0;
    vc::At<int32_t>(self, 0x14) = 0;
}

// 0x5FC560 (thiscall): ConditionHitTime: the hit's time (or its other time, +0xC)
uint8_t __fastcall HitTime_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float time = self[0xC] ? vc::At<float>(HitState(owner), 0x30) : vc::At<float>(HitState(owner), 0x2C);
    return CompareFloats(time, vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FC5A0 (thiscall): ConditionHitStunTime
uint8_t __fastcall HitStunTime_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return CompareFloats(vc::At<float>(HitState(owner), 0x4C), vc::At<float>(self, 4), vc::At<int32_t>(self, 8));
}

// 0x5FC5D0 (thiscall): ConditionHitStunFinished: the controller's time past the stun
uint32_t __fastcall HitStunFinished_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return vc::At<float>(HitState(owner), 0x4C) <= vc::At<float>(vc::At<uint8_t*>(owner, 0), 8) ? 1 : 0;
}

void __fastcall Fields_5FC600(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 0xC) = 5;
    vc::At<float>(self, 4) = 0.0f;
    self[8] = 0;
}
void __fastcall Fields_5FC640(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0x22;
}
void __fastcall Fields_5FC670(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0x57;
    vc::At<float>(self, 8) = 0.0f;
    vc::At<float>(self, 0xC) = 0.0f;
}
void __fastcall Fields_5FC680(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 200;
}
void __fastcall Fields_5FC690(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 1;
}

// 0x5FC6A0 (thiscall): ConditionMissionActive
uint8_t __fastcall MissionActive_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return MissionManager_IsActive_6AA8F0(missionManager_20C3CA0);
}

void __fastcall Fields_5FC6B0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 8) = 4;
    vc::At<float>(self, 4) = 0.0f;
}
void __fastcall Fields_5FC6C0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 0.0f;
    vc::At<int32_t>(self, 8) = 4;
    self[0xC] = 0;
    self[0xD] = 0;
    self[0xE] = 0;
    vc::At<int32_t>(self, 0x10) = 0;
}
void __fastcall Fields_5FC6E0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 0.0f;
    self[0xC] = 0;
    vc::At<float>(self, 8) = 0.0f;
    self[0xD] = 0;
}

} // namespace

#define CONDCORE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)
#define CONDDTOR(name, address, vt) CONDCORE_REVERSED(name, address, &ConditionDeletingDestruct<vt>)
#define CONDDTOR_KEY(name, address, vt) CONDCORE_REVERSED(name, address, &ConditionWithKeyDeletingDestruct<vt>)

CONDCORE_REVERSED("Condition::IsA", 0x005FB7F0, &Condition_IsA);
CONDCORE_REVERSED("Condition::CompareFloats", 0x005FB8B0, &CompareFloatsImpl);
CONDCORE_REVERSED("Condition::CompareInts", 0x005FB9B0, &CompareInts);
CONDCORE_REVERSED("ConditionList::ReserveRest", 0x005FBA30, &ConditionList_ReserveRestImpl);
CONDCORE_REVERSED("ConditionActionNodePlayTime::Evaluate", 0x005FBAA0, &ActionNodePlayTime_Evaluate);
CONDCORE_REVERSED("ConditionList::Construct", 0x005FBAD0, &ConditionList_Construct);
CONDCORE_REVERSED("ConditionList::Destruct", 0x005FBAE0, &ConditionList_Destruct);
CONDCORE_REVERSED("ConditionList::Evaluate", 0x005FBB10, &ConditionList_EvaluateImpl);
CONDCORE_REVERSED("ConditionGroupCondition::Evaluate", 0x005FBC50, &GroupCondition_Evaluate);
CONDCORE_REVERSED("ConditionGroupCondition::NumConditions", 0x005FBC70, &GroupCondition_NumConditions);
CONDCORE_REVERSED("ConditionGroupCondition::GetCondition", 0x005FBC80, &GroupCondition_GetCondition);
CONDCORE_REVERSED("ConditionList::Grow", 0x005FBCC0, &ConditionList_GrowImpl);
CONDCORE_REVERSED("ConditionList::Reserve", 0x005FBD30, &ConditionList_Reserve);
CONDCORE_REVERSED("ConditionList::Add", 0x005FBD50, &ConditionList_Add);
CONDCORE_REVERSED("ActionRequest::SumTimes", 0x005FBD90, &ActionRequest_SumTimes);
CONDCORE_REVERSED("Condition::CycleFraction", 0x005FBE60, &CycleFraction);
CONDCORE_REVERSED("Condition::Fields_5FBF40", 0x005FBF40, &Fields_5FBF40);
CONDCORE_REVERSED("Condition::Fields_5FBF60", 0x005FBF60, &Fields_5FBF60);
CONDCORE_REVERSED("Condition::Fields_5FBFA0", 0x005FBFA0, &Fields_5FBFA0);
CONDCORE_REVERSED("Condition::RotateVector2", 0x005FBFB0, &RotateVector2);
CONDCORE_REVERSED("Condition::Fields_5FC020", 0x005FC020, &Fields_5FC020);
CONDCORE_REVERSED("ConditionParametricTime::Evaluate", 0x005FC030, &ParametricTime_Evaluate);
CONDCORE_REVERSED("ConditionDamagePending::Evaluate", 0x005FC060, &DamagePending_Evaluate);
CONDCORE_REVERSED("ConditionCollisionFlag::Evaluate", 0x005FC0A0, &CollisionFlag_Evaluate);
CONDCORE_REVERSED("Condition::Fields_5FC0C0", 0x005FC0C0, &Fields_5FC0C0);
CONDCORE_REVERSED("ConditionPedIsInOutfit::Evaluate", 0x005FC0D0, &PedIsInOutfit_Evaluate);
CONDCORE_REVERSED("Condition::Fields_5FC0F0", 0x005FC0F0, &Fields_5FC0F0);
CONDCORE_REVERSED("ConditionHitFlagClear::Evaluate", 0x005FC100, &HitFlagClear_Evaluate);
CONDCORE_REVERSED("ConditionActionNodePlayTimePast::Evaluate", 0x005FC120, &PlayTimePast_Evaluate);
CONDCORE_REVERSED("ConditionFirstPerson::Evaluate", 0x005FC150, &FirstPerson_Evaluate);
CONDCORE_REVERSED("Condition::Fields_5FC180", 0x005FC180, &Fields_5FC180);
CONDCORE_REVERSED("Condition::Fields_5FC1A0", 0x005FC1A0, &Fields_5FC1A0);
CONDCORE_REVERSED("Condition::Fields_5FC1C0", 0x005FC1C0, &Fields_5FC1C0);
CONDCORE_REVERSED("Condition::Fields_5FC1D0", 0x005FC1D0, &Fields_5FC1D0);
CONDCORE_REVERSED("Condition::Fields_5FC1E0", 0x005FC1E0, &Fields_5FC1E0);
CONDCORE_REVERSED("Condition::Fields_5FC200", 0x005FC200, &Fields_5FC200);
CONDCORE_REVERSED("Condition::Fields_5FC220", 0x005FC220, &Fields_5FC220);
CONDCORE_REVERSED("Condition::Fields_5FC240", 0x005FC240, &Fields_5FC240);
CONDCORE_REVERSED("Condition::Fields_5FC290", 0x005FC290, &Fields_5FC290);
CONDCORE_REVERSED("ConditionIsPlaying::Destruct", 0x005FC2E0, &IsPlaying_Destruct);
CONDCORE_REVERSED("ConditionIsDayTime::Evaluate", 0x005FC310, &IsDayTime_Evaluate);
CONDCORE_REVERSED("Condition::Fields_5FC320", 0x005FC320, &Fields_5FC320);
CONDCORE_REVERSED("Condition::Fields_5FC3D0", 0x005FC3D0, &Fields_5FC3D0);
CONDCORE_REVERSED("ConditionNearPoint::Construct", 0x005FC450, &NearPoint_Construct);
CONDCORE_REVERSED("Condition::Fields_5FC480", 0x005FC480, &Fields_5FC480);
CONDCORE_REVERSED("Condition::Fields_5FC4B0", 0x005FC4B0, &Fields_5FC4B0);
CONDCORE_REVERSED("Condition::Fields_5FC4E0", 0x005FC4E0, &Fields_5FC4E0);
CONDCORE_REVERSED("Condition::Fields_5FC4F0", 0x005FC4F0, &Fields_5FC4F0);
CONDCORE_REVERSED("ConditionIsPlayerFatigued::Evaluate", 0x005FC500, &IsPlayerFatigued_Evaluate);
CONDCORE_REVERSED("ConditionIsPlayerTired::Evaluate", 0x005FC510, &IsPlayerTired_Evaluate);
CONDCORE_REVERSED("Condition::Fields_5FC520", 0x005FC520, &Fields_5FC520);
CONDCORE_REVERSED("ConditionHitTime::Evaluate", 0x005FC560, &HitTime_Evaluate);
CONDCORE_REVERSED("ConditionHitStunTime::Evaluate", 0x005FC5A0, &HitStunTime_Evaluate);
CONDCORE_REVERSED("ConditionHitStunFinished::Evaluate", 0x005FC5D0, &HitStunFinished_Evaluate);
CONDCORE_REVERSED("Condition::Fields_5FC600", 0x005FC600, &Fields_5FC600);
CONDCORE_REVERSED("Condition::Fields_5FC640", 0x005FC640, &Fields_5FC640);
CONDCORE_REVERSED("Condition::Fields_5FC670", 0x005FC670, &Fields_5FC670);
CONDCORE_REVERSED("Condition::Fields_5FC680", 0x005FC680, &Fields_5FC680);
CONDCORE_REVERSED("Condition::Fields_5FC690", 0x005FC690, &Fields_5FC690);
CONDCORE_REVERSED("ConditionMissionActive::Evaluate", 0x005FC6A0, &MissionActive_Evaluate);
CONDCORE_REVERSED("Condition::Fields_5FC6B0", 0x005FC6B0, &Fields_5FC6B0);
CONDCORE_REVERSED("Condition::Fields_5FC6C0", 0x005FC6C0, &Fields_5FC6C0);
CONDCORE_REVERSED("Condition::Fields_5FC6E0", 0x005FC6E0, &Fields_5FC6E0);
CONDCORE_REVERSED("Condition::IsAStatic", 0x005FC700, &Condition_IsAStatic);

// the deleting destructors
CONDDTOR_KEY("ConditionPrint::DeletingDestruct", 0x005FB860, 0x0092E12C);
CONDDTOR("ConditionGroupCondition::DeletingDestruct", 0x005FB890, 0x0092E140);
CONDDTOR("CanPathToEntityCond::DeletingDestruct", 0x005FC160, 0x0092E588);
CONDDTOR("ConditionTargetHealth::DeletingDestruct", 0x005FC270, 0x0092E5A4);
CONDDTOR_KEY("ConditionIsPlaying::DeletingDestruct", 0x005FC2B0, 0x0092E5B8);
CONDDTOR("ConditionMePlayingDefaultNeutralState::DeletingDestruct", 0x005FC2F0, 0x0092E5CC);
CONDDTOR("ConditionLiquidLevel::DeletingDestruct", 0x005FC340, 0x0092E5E0);
CONDDTOR("ConditionLiquidType::DeletingDestruct", 0x005FC360, 0x0092E5F4);
CONDDTOR("ConditionSurfaceHardness::DeletingDestruct", 0x005FC380, 0x0092E608);
CONDDTOR_KEY("ConditionInTriggerVolume::DeletingDestruct", 0x005FC3A0, 0x0092E61C);
CONDDTOR_KEY("ConditionPlayerInTriggerVolume::DeletingDestruct", 0x005FC400, 0x0092E630);
CONDDTOR("ConditionInTriggerVolumeOfType::DeletingDestruct", 0x005FC430, 0x0092E644);
CONDDTOR("ConditionPedType::DeletingDestruct", 0x005FC4C0, 0x0092E66C);
CONDDTOR("ConditionObjective::DeletingDestruct", 0x005FC620, 0x0092E680);
CONDDTOR("ConditionObjectiveOfType::DeletingDestruct", 0x005FC650, 0x0092E694);
CONDDTOR("ConditionWeightedRandom::DeletingDestruct", 0x005FC7C0, 0x0092E6A8);
CONDDTOR("ConditionConditionGroup::DeletingDestruct", 0x005FC970, 0x0092E6BC);
CONDDTOR_KEY("ConditionPedModelID::DeletingDestruct", 0x005FC9B0, 0x0092E6DC);
CONDDTOR_KEY("ConditionOnMission::DeletingDestruct", 0x005FCA70, 0x0092E6F0);
CONDDTOR_KEY("ConditionAnimationGroupLoaded::DeletingDestruct", 0x005FCB70, 0x0092E704);
CONDDTOR_KEY("ConditionHasActionNode::DeletingDestruct", 0x005FCC80, 0x0092E790);
CONDDTOR("ConditionHasSweptSphereCollision::DeletingDestruct", 0x005FCD20, 0x0092E7CC);
CONDDTOR("ConditionTargetProxyProp::DeletingDestruct", 0x005FCE70, 0x0092E804);
CONDDTOR("ConditionCanPropSetUsageState::DeletingDestruct", 0x005FCE90, 0x0092E818);
CONDDTOR("PedGroundDistCond::DeletingDestruct", 0x005FD1A0, 0x0092E84C);
CONDDTOR("PedVelocityCond::DeletingDestruct", 0x005FD1E0, 0x0092E860);
CONDDTOR("ConditionHasRangedWeapon::DeletingDestruct", 0x005FD200, 0x0092E874);
CONDDTOR_KEY("ConditionHasTXDLoaded::DeletingDestruct", 0x005FD220, 0x0092E888);
CONDDTOR("ConditionCutsceneSoundLoaded::DeletingDestruct", 0x005FD590, 0x0092E718);
CONDDTOR("ConditionIsMusicStreamLoaded::DeletingDestruct", 0x005FD5B0, 0x0092E72C);
CONDDTOR_KEY("ConditionIsSoundBankLoaded::DeletingDestruct", 0x005FD5D0, 0x0092E740);
CONDDTOR("ConditionIsSpeechBusy::DeletingDestruct", 0x005FD600, 0x0092E754);
CONDDTOR("ConditionIsSpeechStreamFree::DeletingDestruct", 0x005FD620, 0x0092E768);
CONDDTOR("ConditionIsSpeechPreloaded::DeletingDestruct", 0x005FD640, 0x0092E77C);
CONDDTOR("ConditionHasSpeechEvent::DeletingDestruct", 0x005FD660, 0x0092E7A4);
CONDDTOR_KEY("ConditionNearPoint::DeletingDestruct", 0x005FFB40, 0x0092E658);
