// Script natives for peds (0x5CD5D0..0x5D0C80): allies, emotions, objectives (attack,
// cover, use a prop, follow a path, guard), combat zones, stimuli, creating peds.
// Each is `int __cdecl Native_X(lua_State* L)`, returning how many results it pushed;
// registered under its script name (lua_natives.csv).
//
// The ped's parts reached here by offset: +0x1C8 the targeting system, +0x2F0 the ally
// chain, +0x3BC the scanner, +0x5F0 the objective manager, +0x668 the combat slots,
// +0xA64 the head look / AI, +0xB44 the social part.

#include <cstddef>
#include <cstring>

#include "game/ScriptNatives.h"
#include "reversed.h"

namespace {

// ---- the ped's parts ------------------------------------------------------------------

// 0x48BE90: the scanner (ped + 0x3BC) gets a point to aim for (a CVector by value)
GAME_FN(void, __thiscall, snpScanner_PushPoint_x64, 0x0048BE90, void* scanner, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void snpScanner_PushPoint(void* a0, SnVec a1) {
    return snpScanner_PushPoint_x64(a0, re::FloatAt(a1, 0), re::FloatAt(a1, 1), re::FloatAt(a1, 2));
}
// 0x4860F0: the ally chain (ped + 0x2F0) takes a follower
GAME_FN(void, __thiscall, snpAlly_AddFollower, 0x004860F0, void* ally, uint8_t* follower, int flag);
// 0x4859D0: the ally chain drops a follower
GAME_FN(uint8_t, __thiscall, snpAlly_RemoveFollower, 0x004859D0, void* ally, uint8_t* follower);
// 0x48EBC0: the social part's (ped + 0xB44) emotion towards a ped, into *out
GAME_FN(int32_t*, __thiscall, snpSocial_GetEmotion, 0x0048EBC0, void* social, int32_t* out, uint8_t* ped, int, uintptr_t, uintptr_t);
// 0x48F1D0: sets it
GAME_FN(void, __thiscall, snpSocial_SetEmotion, 0x0048F1D0, void* social, uint8_t* ped, int emotion, uint8_t flag, int);
// 0x4925E0: starts socializing with a ped
GAME_FN(void, __thiscall, snpSocial_Start, 0x004925E0, void* social, uint8_t* ped, uint8_t flag, int kind, int);
// 0x474B30 / 0x474A90 / 0x47D1B0: the ped's required gift, its "needs a gift" flag, paid
GAME_FN(void, __thiscall, snpPed_SetRequiredGift, 0x00474B30, void* ped, int model);
GAME_FN(void, __thiscall, snpPed_SetNeedsGift, 0x00474A90, void* ped, int on);
GAME_FN(void, __thiscall, snpPed_SetPaid, 0x0047D1B0, void* ped, int paid);
// 0x4744B0: the ped's grapple partner
GAME_FN(void, __thiscall, snpPed_SetPartner, 0x004744B0, void* ped, uint8_t* partner);
// 0x48D020: the head's position (ped + 0xA64), into *out
GAME_FN(SnVec*, __thiscall, snpPed_GetHeadPos, 0x0048D020, void* part, SnVec* out);
// 0x487BC0 / 0x487B70 / 0x486D50: the combat slots (ped + 0x668)
GAME_FN(uint8_t, __thiscall, snpCombat_JoinTarget, 0x00487BC0, void* combat, uint8_t* target, int list, int dontCommit);
GAME_FN(void, __thiscall, snpCombat_LeaveCurrentTarget, 0x00487B70, void* combat);
GAME_FN(void, __thiscall, snpCombat_ClearTargetMark, 0x00486D50, void* combat);
// 0x4DAB30 / 0x4DAB70: the ped's accessory container (ped + 0x1D4): count, one of them
GAME_FN(int, __thiscall, snpAccessories_Count, 0x004DAB30, void* container);
GAME_FN(uint8_t*, __thiscall, snpAccessories_Get, 0x004DAB70, void* container, int i);
// 0x50E940: CBaseModelInfo::MatchesName
GAME_FN(uint8_t, __thiscall, snpModelInfo_MatchesName, 0x0050E940, void* mi, const char* name);
// 0x4146B0: out = the matrix's rotation times v
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline SnVec* snpMultiply3x3(SnVec* out, const float* matrix, const SnVec* v) { return re::CallVec16<0x004146B0, SnVec*>(out, matrix, v); }

// The targeting system (ped + 0x1C8)
GAME_FN(void*, __thiscall, snpTargeting_Classify, 0x004D5AE0, void* system, uint8_t* entity);
GAME_FN(float, __thiscall, snpTargeting_Score, 0x004D7AF0, void* system, void* kind, uint8_t* entity, int zoomed);
GAME_FN(uint8_t, __thiscall, snpTargeting_IsVisible, 0x004D6E20, void* system, uint8_t* entity, int);
// 0x4F1250: TheCamera's zoom
inline void* const snpTheCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_FN(float, __thiscall, snpCamera_GetZoom, 0x004F1250, void* camera);

// 0x4D33F0: the nearest vehicle to attack (CProjectileInfo::FindVehicleTarget)
GAME_FN(uint8_t*, __cdecl, snpFindVehicleTarget, 0x004D33F0, uint8_t* ped, float range, int, int, int);

// The triggers: the names (TriggerNames, 0x20C7E98) and the pool (0x20C7C48)
inline void* const snpTriggerNames = reinterpret_cast<void*>(0x020C7E98);
GAME_FN(int, __thiscall, snpTriggerNames_Find, 0x006D79C0, void* names, void* userdata);
GAME_VAR(CPool<uint8_t>*, snpTriggerPool, 0x020C7C48);

// ---- objectives (each: Malloc, the constructor in place, then the manager's Add) ------

GAME_FN(void*, __thiscall, snpAttackVehicleObjective_Construct, 0x00470DB0, void* self, uint8_t* ped, uint8_t* vehicle);
GAME_FN(void*, __thiscall, snpRooftopAttackerObjective_Construct, 0x0046FE70, void* self, uint8_t* ped);
GAME_FN(void*, __thiscall, snpCoverObjective_Construct, 0x004710B0, void* self, uint8_t* ped, uint8_t* victim, int value, int value1C);
GAME_FN(void*, __thiscall, snpUsePropObjective_Construct, 0x00471810, void* self, uint8_t* ped, uint8_t* prop);

inline void snpAddObjective(uint8_t* ped, void* objective) {
    snObjectives_Add(ped + 0x5F0, objective);
}

// ---- the natives ------------------------------------------------------------------------

// (the ped handle 0 is the player here)
int __cdecl Native_PedSetProjDest(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = snGetPed(handle, handle != 0 ? SN_HANDLE_PED : SN_HANDLE_PLAYER);
    SnVec point;
    point.x = Script::GetFloat(L, 1);
    point.y = Script::GetFloat(L, 2);
    point.z = Script::GetFloat(L, 3);
    snpScanner_PushPoint(ped + 0x3BC, point);
    return 0;
}

int __cdecl Native_PedRecruitAlly(lua_State* L) {
    uint8_t* leader = snPedArg(L, 0);
    uint8_t* ally = snPedArg(L, 1);
    snpAlly_AddFollower(leader + 0x2F0, ally, 1);
    if (lua::gettop(L) == 3) {
        int32_t stay = Script::GetBool(L, 2);
        snAt<int32_t>(leader, 0x14EC) = stay;
        snAt<int32_t>(ally, 0x14EC) = stay;
    }
    return 0;
}

int __cdecl Native_PedDismissAlly(lua_State* L) {
    uint8_t* leader = snPedArg(L, 0);
    uint8_t* ally = snPedArg(L, 1);
    if (leader != nullptr && ally != nullptr) {
        snpAlly_RemoveFollower(leader + 0x2F0, ally);
    }
    if (lua::gettop(L) == 3) {
        int32_t stay = Script::GetBool(L, 2);
        if (leader != nullptr) {
            snAt<int32_t>(leader, 0x14EC) = stay;
        }
        if (ally != nullptr) {
            snAt<int32_t>(ally, 0x14EC) = stay;
        }
    } else {
        if (leader != nullptr) {
            snAt<int32_t>(leader, 0x14EC) = 0;
        }
        if (ally != nullptr) {
            snAt<int32_t>(ally, 0x14EC) = 0;
        }
    }
    return 0;
}

int __cdecl Native_PedGetEmotionTowardsPed(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    uint8_t* other = snPedArg(L, 1);
    int32_t emotion;
    Script::PushInt(L, *snpSocial_GetEmotion(ped + 0xB44, &emotion, other, 0, 0, 0));
    return 1;
}

int __cdecl Native_PedSetEmotionTowardsPed(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    uint8_t* other = snPedArg(L, 1);
    int emotion = Script::GetInt(L, 2);
    uint8_t flag = 0;
    if (lua::gettop(L) > 3) {
        flag = Script::GetBool(L, 3);
    }
    snpSocial_SetEmotion(ped + 0xB44, other, emotion, flag, 0);
    return 0;
}

int __cdecl Native_PedSetWantsToSocializeWithPed(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    uint8_t* other = snPedArg(L, 1);
    uint8_t flag = Script::GetBoolOpt(L, 2, 0);
    int kind = Script::GetIntOpt(L, 3, 0x19);
    snpSocial_Start(ped + 0xB44, other, flag, kind, 0);
    return 0;
}

int __cdecl Native_PedSetRequiredGift(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    int model = Script::GetInt(L, 1);
    snAt<int32_t>(ped, 0x1428) = Script::GetBoolOpt(L, 2, 0);
    snAt<int32_t>(ped, 0x1430) = Script::GetBoolOpt(L, 3, 0);
    snpPed_SetRequiredGift(ped, model);
    snpPed_SetNeedsGift(ped, 1);
    snpPed_SetPaid(ped, 0);
    return 0;
}

// (a failed allocation still hands nullptr to the objective manager)
int __cdecl Native_PedAttackCar(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    uint8_t* vehicle = snPoolAt(snVehiclePool, Script::GetInt(L, 1));
    if (vehicle != nullptr) {
        void* mem = snMalloc(0x14);
        snpAddObjective(ped, mem != nullptr ? snpAttackVehicleObjective_Construct(mem, ped, vehicle) : nullptr);
    }
    return 0;
}

int __cdecl Native_PedAttackNearestCar(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    uint8_t* vehicle = snpFindVehicleTarget(ped, *reinterpret_cast<const float*>(0x00900BEC), -1, 1, 0); // (2.6)
    if (vehicle != nullptr) {
        void* mem = snMalloc(0x14);
        snpAddObjective(ped, mem != nullptr ? snpAttackVehicleObjective_Construct(mem, ped, vehicle) : nullptr);
    }
    return 0;
}

int __cdecl Native_PedGetHeadPos(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    SnVec pos;
    if (handle != -1) {
        uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
        Script::PushVector(L, reinterpret_cast<ScriptVector*>(snpPed_GetHeadPos(ped + 0xA64, &pos)));
        return 3;
    }
    pos.x = pos.y = pos.z = 0.0f;
    Script::PushVector(L, reinterpret_cast<ScriptVector*>(&pos));
    return 3;
}

// A prop by model name among the ped's accessories ("Any": any accessory at all). The
// ped handle 0 is the player
int __cdecl Native_PedIsPropAttached(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = snPlayerPed;
    if (handle != 0) {
        ped = snGetPed(handle, SN_HANDLE_PED);
    }
    uint8_t attached = 0;
    if (ped != nullptr) {
        int count = snpAccessories_Count(snAt<void*>(ped, 0x1D4));
        char name[256];
        strcpy(name, Script::GetString(L, 1));
        if (snStricmp(name, "Any") == 0 && (attached = count != 0) != 0) {
            // any accessory
        } else {
            for (int i = 0; i < count; i++) {
                uint8_t* accessory = snpAccessories_Get(snAt<void*>(ped, 0x1D4), i);
                int16_t model = snAt<int16_t>(accessory, 0x10E);
                if (model != -1 && snModelInfoPtrs[model] != nullptr &&
                    snpModelInfo_MatchesName(snModelInfoPtrs[model], name)) {
                    attached = 1;
                    break;
                }
            }
        }
    }
    Script::PushBool(L, attached);
    return 1;
}

int __cdecl Native_PedRooftopAttacker(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        void* mem = snMalloc(0x10);
        snpAddObjective(ped, mem != nullptr ? snpRooftopAttackerObjective_Construct(mem, ped) : nullptr);
    }
    return 0;
}

// (the second ped is looked up and not used)
int __cdecl Native_PedAutoCover(lua_State* L) {
    uint8_t* ped = nullptr;
    if (Script::IsNumber(L, 0)) {
        ped = snPedArg(L, 0);
    }
    if (Script::IsNumber(L, 1)) {
        snPedArg(L, 1);
    }
    if (ped != nullptr) {
        void* mem = snMalloc(0x6C);
        snpAddObjective(ped, mem != nullptr ? snpCoverObjective_Construct(mem, ped, nullptr, 3, 0) : nullptr);
    }
    return 0;
}

int __cdecl Native_PedGetOffsetInWorldCoords(lua_State* L) {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    uint8_t* ped = nullptr;
    if (Script::IsNumber(L, 0)) {
        ped = snPedArg(L, 0);
    }
    if (ped != nullptr) {
        SnVec offset;
        offset.x = Script::GetFloat(L, 1);
        offset.y = Script::GetFloat(L, 2);
        offset.z = Script::GetFloat(L, 3);
        const float* matrix = snEntity_GetMatrix(ped);
        SnVec world;
        snpMultiply3x3(&world, matrix, &offset);
        const float* pos = snEntityPos(ped);
        x = pos[0] + world.x;
        y = pos[1] + world.y;
        z = pos[2] + world.z;
    }
    Script::PushFloat(L, x);
    Script::PushFloat(L, y);
    Script::PushFloat(L, z);
    return 3;
}

// The prop: a trigger's (argument 3 true: the trigger named by the userdata, its entity
// at +0x1C) or the object whose +0x228 is the userdata (the game's pool walk tests the
// slot's own flag, then looks the object up as if `slot` were a handle: slot 0 when its
// flag byte equals `slot`, else nullptr, which it then reads through: a crash)
int __cdecl Native_PedUseProp(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    void* userdata = Script::GetUserdata(L, 1);
    uint8_t isTrigger = Script::GetBoolOpt(L, 2, 0);
    uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
    uint8_t* prop = nullptr;
    if (isTrigger) {
        int slot = snpTriggerNames_Find(snpTriggerNames, userdata);
        if (slot == -1) {
            return 0;
        }
        CPool<uint8_t>* pool = snpTriggerPool;
        if ((pool->flags[slot] & 0x80) != 0) {
            return 0;
        }
        uint8_t* trigger = pool->objects + pool->elemSize * slot;
        if (trigger == nullptr) {
            return 0;
        }
        prop = snAt<uint8_t*>(trigger, 0x1C);
    } else {
        CPool<uint8_t>* pool = snObjectPool;
        int i = 0;
        for (; i < pool->size; i++) {
            if ((pool->flags[i] & 0x80) == 0) {
                uint8_t* object = snPoolAt(pool, i);
                if (snAt<void*>(object, 0x228) == userdata) {
                    prop = object;
                    break;
                }
            }
        }
        if (i >= pool->size) {
            return 0;
        }
    }
    if (prop != nullptr) {
        void* mem = snMalloc(0x14);
        snpAddObjective(ped, mem != nullptr ? snpUsePropObjective_Construct(mem, ped, prop) : nullptr);
    }
    return 0;
}

// (argument 2 nil: no partner)
int __cdecl Native_PedSetGrappleTarget(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int partnerHandle = Script::GetInt(L, 1);
    uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
    if (Script::IsNil(L, 1)) {
        snpPed_SetPartner(ped, nullptr);
        return 0;
    }
    snpPed_SetPartner(ped, snGetPed(partnerHandle, SN_HANDLE_PED));
    return 0;
}

// List 0..2, else the first of the three that takes the ped
int __cdecl Native_PedJoinCombatZone(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int targetHandle = Script::GetInt(L, 1);
    uint8_t joined = 0;
    uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
    uint8_t* target = snGetPed(targetHandle, SN_HANDLE_PED);
    int list = Script::GetInt(L, 2);
    if (ped != nullptr && target != nullptr) {
        if (list >= 0 && list <= 2) {
            joined = snpCombat_JoinTarget(ped + 0x668, target, list, 0);
        } else {
            for (int i = 0; i < 3; i++) {
                joined = snpCombat_JoinTarget(ped + 0x668, target, i, 0);
                if (joined) {
                    break;
                }
            }
        }
    }
    Script::PushBool(L, joined);
    return 1;
}

int __cdecl Native_PedLeaveCombatZone(lua_State* L) {
    uint8_t* combat = snPedArg(L, 0) + 0x668;
    snpCombat_LeaveCurrentTarget(combat);
    snpCombat_ClearTargetMark(combat);
    return 0;
}

// Targetable: visible to the ped's targeting, scored above "none" (-1e11), classified
int __cdecl Native_PedIsTargetable(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int targetHandle = Script::GetInt(L, 1);
    uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
    uint8_t* target = snGetPed(targetHandle, SN_HANDLE_PED);
    if (ped == nullptr || target == nullptr) {
        Script::PushBool(L, 0);
        return 1;
    }
    float zoom = snpCamera_GetZoom(snpTheCamera);
    void* kind = snpTargeting_Classify(snAt<void*>(ped, 0x1C8), target);
    float score = snpTargeting_Score(snAt<void*>(ped, 0x1C8), kind, target, zoom != 0.0f);
    uint8_t targetable = 0;
    if (snpTargeting_IsVisible(snAt<void*>(ped, 0x1C8), target, 0) &&
        score != *reinterpret_cast<const float*>(0x00928A78) && kind != nullptr) {
        targetable = 1;
    }
    Script::PushBool(L, targetable);
    return 1;
}

// ---- effects, stimuli, boxes -----------------------------------------------------------

// 0x576ED0: a name's hash; 0x465580: the model's effect set (model info + 0x94) finds it
GAME_FN(uint32_t, __cdecl, snpHashName, 0x00576ED0, const char* name);
GAME_FN(int32_t, __thiscall, snpEffectSet_Find, 0x00465580, void* set, uint32_t hash, int32_t* out);
// 0x667B60: the effect library (0xB6F1E8) looks a name up
GAME_VAR(void*, snpEffectLibrary, 0x00B6F1E8);
GAME_FN(uint16_t, __thiscall, snpEffectLibrary_Find, 0x00667B60, void* library, const char* name);
// 0x468770 / 0x468820: the ped's effect keeper (ped + 0x2E4) starts / stops one
GAME_FN(void, __thiscall, snpEffectKeeper_Start, 0x00468770, void* keeper, int effect, int32_t bone);
GAME_FN(void, __thiscall, snpEffectKeeper_Stop, 0x00468820, void* keeper, int effect, int32_t bone);

// 0x4A6670: the stimuli (0xC2C1A0) take a new one
GAME_VAR(void*, snpStimuli, 0x00C2C1A0);
GAME_FN(void, __thiscall, snpStimuli_Add, 0x004A6670, void* stimuli, int type, uint8_t* from, uint8_t* to, const SnVec* pos, float a, float b);

// 0x564830: a box from its corners (CColBox, 0x20 bytes); 0x565C20: it holds a point
struct SnpColBox {
    float min[4];
    float max[4];
};
static_assert(sizeof(SnpColBox) == 0x20);
GAME_FN(void, __thiscall, snpColBox_Set, 0x00564830, SnpColBox* box, const SnVec* min, const SnVec* max);
GAME_FN(uint8_t, __thiscall, snpColBox_Contains, 0x00565C20, const SnpColBox* box, const SnVec* point);
// 0x46DD20: CPlaceable::GetMatrix; 0x414D20: the point in the matrix's space
GAME_FN(float*, __thiscall, snpPlaceable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(SnVec*, __cdecl, snpInverseTransformPoint, 0x00414D20, SnVec* out, const float* matrix, const SnVec* point);

GAME_FN(void*, __thiscall, snpBikeBrawlObjective_Construct, 0x0046FF00, void* self, uint8_t* ped);
// 0x471470: the objective manager puts one in front
GAME_FN(uint8_t, __thiscall, snpObjectives_Prepend, 0x00471470, void* manager, void* objective);

// The ped population (0xC2C108) and the path blockers (0xC2AB70)
GAME_VAR(uint8_t*, snpPedPopulation, 0x00C2C108);
GAME_FN(uint8_t, __thiscall, snpPopulation_FindSpawnPoint, 0x0049D860, void* pop, const SnVec* from, SnVec* out, int, int32_t* out2, float, float);
GAME_FN(uint8_t, __thiscall, snpPopulation_IsInSpawnRange, 0x0049D040, void* pop, const SnVec* pos, int outOfRange, int useMin, float minDistance);
GAME_FN(uint8_t, __thiscall, snpPopulation_IsPedPoolFull, 0x0049F400, void* pop);
inline void* const snpPathBlockers = reinterpret_cast<void*>(0x00C2AB70);
GAME_FN(uint8_t, __thiscall, snpPathBlockers_Blocks, 0x0046EFB0, void* blockers, uint8_t* ped, const SnVec* pos, int);
// 0x4636B0: CWorld::FindGroundZ
GAME_FN(float, __cdecl, snpFindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int flags);
// the population's "area clear of peds" reference point (0xBCC564)
GAME_VAR(SnVec, snpAreaClearPoint, 0x00BCC564);

// 0x6A9A20: the gift a ped wants (the gifts, 0x20C3698); 0x474B40: the ped's gift index
inline void* const snpGifts = reinterpret_cast<void*>(0x020C3698);
GAME_FN(int, __thiscall, snpGifts_For, 0x006A9A20, void* gifts, uint8_t* ped);
GAME_FN(void, __thiscall, snpPed_SetGiftIndex, 0x00474B40, void* ped, int index);

int __cdecl Native_PedAttachEffect(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    const char* boneName = Script::GetString(L, 1);
    const char* effectName = Script::GetString(L, 2);
    uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
    void* set = snAt<void*>(snAt<uint8_t*>(ped, 0x114), 0x94);
    int32_t bone;
    snpEffectSet_Find(set, snpHashName(boneName), &bone);
    uint16_t effect = snpEffectLibrary_Find(snpEffectLibrary, effectName);
    snpEffectKeeper_Start(snAt<void*>(ped, 0x2E4), effect, bone);
    return 0;
}

int __cdecl Native_PedDetachAndKillEffect(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    const char* boneName = Script::GetString(L, 1);
    const char* effectName = Script::GetString(L, 2);
    uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
    void* set = snAt<void*>(snAt<uint8_t*>(ped, 0x114), 0x94);
    int32_t bone;
    snpEffectSet_Find(set, snpHashName(boneName), &bone);
    uint16_t effect = snpEffectLibrary_Find(snpEffectLibrary, effectName);
    snpEffectKeeper_Stop(snAt<void*>(ped, 0x2E4), effect, bone);
    return 0;
}

// Not while either is in state 0xD, or the first is mid-conversation (+0xBB0 social
// state, +0xBB4 partner) with someone other than the second. Handle 0: the player
int __cdecl Native_PedCreateStimulus(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* from = snGetPed(handle, handle != 0 ? SN_HANDLE_PED : SN_HANDLE_PLAYER);
    int toHandle = Script::GetInt(L, 1);
    uint8_t* to = snGetPed(toHandle, toHandle != 0 ? SN_HANDLE_PED : SN_HANDLE_PLAYER);
    int type = Script::GetInt(L, 2);
    if (snAt<int32_t>(from, 0x1310) == 0xD || snAt<int32_t>(to, 0x1310) == 0xD || snAt<int32_t>(from, 0xBB0) == 0 ||
        snAt<int32_t>(from, 0xBB4) == 0 || snAt<uint8_t*>(to, 0xBB4) == from) {
        const float* p = snEntityPos(from);
        SnVec pos = {p[0], p[1], p[2]};
        snpStimuli_Add(snpStimuli, type, from, to, &pos, -1.0f, -1.0f);
    }
    return 0;
}

// The box: x1, x2, y1, y2, z1, z2, in the first ped's space
int __cdecl Native_PedIsPedInBox(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    uint8_t* other = snPedArg(L, 1);
    float x1 = Script::GetFloat(L, 2);
    float x2 = Script::GetFloat(L, 3);
    float y1 = Script::GetFloat(L, 4);
    float y2 = Script::GetFloat(L, 5);
    float z1 = Script::GetFloat(L, 6);
    float z2 = Script::GetFloat(L, 7);
    SnVec min = {x1, y1, z1};
    SnVec max = {x2, y2, z2};
    SnpColBox box;
    snpColBox_Set(&box, &min, &max);
    const float* matrix = snpPlaceable_GetMatrix(ped);
    const float* p = snEntityPos(other);
    SnVec pos = {p[0], p[1], p[2]};
    SnVec local;
    snpInverseTransformPoint(&local, matrix, &pos);
    Script::PushBool(L, snpColBox_Contains(&box, &local));
    return 1;
}

// (in front of the queue, unlike the others)
int __cdecl Native_PedBikeBrawl(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        void* mem = snMalloc(0x10);
        snpObjectives_Prepend(ped + 0x5F0, mem != nullptr ? snpBikeBrawlObjective_Construct(mem, ped) : nullptr);
    }
    return 0;
}

// A spawn point near the player (for this ped), on the ground, in spawn range and not
// blocked; else (9999, 9999, 9999)
int __cdecl Native_PedFindRandomSpawnPosition(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    float none = *reinterpret_cast<const float*>(0x0090D1C8); // (9999)
    SnVec fallback = {none, none, none};
    SnVec pos = snpAreaClearPoint;
    const float* p = snEntityPos(snPlayerPed);
    SnVec from = {p[0], p[1], p[2]};
    int32_t unused = 0;
    if (snpPopulation_FindSpawnPoint(snpPedPopulation, &from, &pos, 1, &unused, 0.0f, 0.0f)) {
        uint8_t found = 0;
        float groundZ = snpFindGroundZ(pos.x, pos.y, float(pos.z + 1.0), &found, 1);
        if (found) {
            pos.z = groundZ;
        }
        if (snpPopulation_IsInSpawnRange(snpPedPopulation, &pos, 0, 0, 10.0f) &&
            !snpPathBlockers_Blocks(snpPathBlockers, ped, &pos, 1)) {
            Script::PushVector(L, reinterpret_cast<ScriptVector*>(&pos));
            return 3;
        }
    }
    Script::PushVector(L, reinterpret_cast<ScriptVector*>(&fallback));
    return 3;
}

int __cdecl Native_PedEnableGiftRequirement(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        if (Script::GetBool(L, 1)) {
            snpPed_SetGiftIndex(ped, snpGifts_For(snpGifts, ped));
            return 0;
        }
        snpPed_SetGiftIndex(ped, -1);
    }
    return 0;
}

// 0x5CF7D0 (thiscall on the objective manager, ped + 0x5F0): its follow-path objective
// (type 0xC), queued or current, nullptr for none
struct SnpObjectives {
    uint8_t unknown0[8];      // +0
    re::Ptr32<uint8_t> queued[20];      // +8
    uint32_t count;           // +0x58
    re::Ptr32<uint8_t> current;         // +0x5C
};
static_assert(offsetof(SnpObjectives, count) == 0x58 && offsetof(SnpObjectives, current) == 0x5C);

uint8_t* __fastcall Objectives_FindFollowPath(SnpObjectives* self, re::Edx) {
    for (uint32_t i = 0; i < self->count; i++) {
        uint8_t* objective = self->queued[i];
        if (objective != nullptr && snAt<int32_t>(objective, 8) == 0xC) {
            return objective;
        }
    }
    uint8_t* current = self->current;
    if (current == nullptr || snAt<int32_t>(current, 8) != 0xC) {
        current = nullptr;
    }
    return current;
}

// ---- creating peds ----------------------------------------------------------------------

GAME_FN(uint8_t*, __cdecl, snpGetStreamingInfo, 0x0052DD30, int model);
GAME_FN(void, __cdecl, snpRequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __cdecl, snpLoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __cdecl, snpLoadingScreenMessage, 0x0043C4F0, const char* a, const char* b, uintptr_t);
// 0x5DB960: the running script (checked); 0x5D9450 / 0x5D9630: it keeps a model / a ped
GAME_FN(uint8_t*, __thiscall, snpCurrentScriptChecked, 0x005DB960, void* store);
GAME_FN(void, __thiscall, snpScript_AddModel, 0x005D9450, void* script, int model);
GAME_FN(void, __thiscall, snpScript_AddPed, 0x005D9630, void* script, int handle);
GAME_FN(void*, __cdecl, snpPedPool_New, 0x004789E0, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, snpCivilianPed_Construct, 0x0046DFF0, void* self, int model, int, int);
GAME_FN(void, __thiscall, snpEntity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, snpPlaceable_SetOrientation, 0x0046A690, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, snpPed_SetHeading, 0x00475730, void* ped, float heading);
GAME_FN(void, __cdecl, snpWorld_Add, 0x0045D6B0, void* entity);
GAME_FN(int, __cdecl, snpCreatePed, 0x005CF810, lua_State* L, int model, float x, float y, float z, float heading, int cull);

// 0x5CF810: a civilian ped of the model at x, y, z (on the ground below z + 1 when there
// is ground there), its handle (the ped's virtual +0xAC). The model is loaded at once
// when it isn't, and kept by the running script. `cull`: the population may make room
// first (L is unused; a full ped pool gives nullptr, which the game uses anyway)
int __cdecl CreatePed(lua_State*, int model, float x, float y, float z, float heading, int cull) {
    if (uint8_t(cull) != 0 && snpPedPopulation != nullptr) {
        snpPopulation_IsPedPoolFull(snpPedPopulation);
    }
    if (snpGetStreamingInfo(model)[4] == 1) {
        if ((snpGetStreamingInfo(model)[5] & 2) != 0 && snpCurrentScriptChecked(snLurStore) != nullptr) {
            snpScript_AddModel(snCurrentScript(snLurStore), model);
        }
    } else {
        snpRequestModel(model, 0x16);
        if (snpCurrentScriptChecked(snLurStore) != nullptr) {
            snpScript_AddModel(snCurrentScript(snLurStore), model);
        }
        snpLoadingScreenMessage("CreatePed()", "CStreaming::LoadAllRequestedModels()", 0);
        snpLoadAllRequestedModels(0);
    }
    void* mem = snpPedPool_New(0x1E90);
    uint8_t* ped = mem != nullptr ? snpCivilianPed_Construct(mem, model, 2, 0) : nullptr;
    uint8_t found = 0;
    float groundZ = snpFindGroundZ(x, y, float(z + 1.0), &found, 1);
    if (found) {
        z = float(groundZ + double(0.01f));
    }
    snpEntity_SetPosition(ped, x, y, z);
    if (heading == 0.0f) {
        snpPlaceable_SetOrientation(ped, 0.0f, 0.0f, 0.0f);
    } else {
        snpPed_SetHeading(ped, heading);
    }
    snpWorld_Add(ped);
    return snVCall<int>(ped, 0xAC);
}

// The routes (0xBC5E7C): one by id, its first point, the next one (nullptr past the last)
inline void* const snpRoutes = reinterpret_cast<void*>(0x00BC5E7C);
GAME_FN(uint8_t*, __thiscall, snpRoutes_Get, 0x006D6450, void* routes, int id);
GAME_FN(uint8_t*, __thiscall, snpRoute_First, 0x006D59B0, void* route);
GAME_FN(uint8_t*, __thiscall, snpRoute_Next, 0x006D59F0, void* route);

// A ped at a route's point (argument 3: its index, 1-based, stopping at the last), its
// heading from the point's (+0x1C, degrees, 0 = +x)
int __cdecl Native_PedCreatePoint(lua_State* L) {
    int count = lua::gettop(L);
    uint32_t index = 0; // (only read with three arguments)
    if (count == 3) {
        index = uint16_t(Script::GetInt(L, 2));
    }
    int model = Script::GetInt(L, 0);
    uint8_t* route = snpRoutes_Get(snpRoutes, Script::GetInt(L, 1));
    uint8_t* point = snpRoute_First(route);
    if (count == 3) {
        for (int i = 1; i < int(index); i++) {
            uint8_t* next = snpRoute_Next(route);
            if (next == nullptr) {
                break;
            }
            point = next;
        }
    }
    float x = snAt<float>(point, 0);
    float y = snAt<float>(point, 4);
    float z = snAt<float>(point, 8);
    uint8_t found = 0;
    float groundZ = snpFindGroundZ(x, y, float(z + 1.0), &found, 1);
    if (found) {
        z = groundZ;
    }
    z = float(z + double(0.01f));
    float heading = float((double(snAt<float>(point, 0x1C)) - 90.0) * double(3.1415927f) / 180.0);
    int handle = snpCreatePed(L, model, x, y, z, heading, 1);
    if (handle != -1) {
        snpScript_AddPed(snCurrentScript(snLurStore), handle);
    }
    Script::PushInt(L, handle);
    return 1;
}

int __cdecl Native_PedCreateXYZ(lua_State* L) {
    int model = Script::GetInt(L, 0);
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    int handle = snpCreatePed(L, model, x, y, z, 0.0f, 1);
    if (handle != -1) {
        snpScript_AddPed(snCurrentScript(snLurStore), handle);
    }
    Script::PushInt(L, handle);
    return 1;
}

// ---- paths ------------------------------------------------------------------------------

// The patrol paths (0xD13CB4)
GAME_VAR(void*, snpPatrolPaths, 0x00D13CB4);
GAME_FN(uint8_t*, __thiscall, snpPatrolPath_Get, 0x005ED3A0, void* owner, int id);
GAME_FN(uint32_t, __thiscall, snpPath_GetNumNodes, 0x005EC670, void* path);
GAME_FN(uint8_t*, __thiscall, snpObjectives_FindFollowPath, 0x005CF7D0, void* manager);
GAME_FN(void, __thiscall, snpActionController_Stop, 0x005F40A0, void* controller);
GAME_FN(void, __thiscall, snpObjectives_Clear, 0x00470350, void* manager);
GAME_FN(void, __thiscall, snpObjectives_MoveToFront, 0x00471D30, void* manager, void* objective);
GAME_FN(void*, __thiscall, snpFollowPathObjective_Construct, 0x0046FD70, void* self, uint8_t* ped);
// 0x5C62E0: a speed argument (0..4: walk .. sprint, -1; else `def`)
GAME_FN(float, __cdecl, snpGetSpeedOpt, 0x005C62E0, lua_State* L, int i, float def);
GAME_FN(uint8_t*, __thiscall, snpLurStore_GetScript, 0x005DBD50, void* store, lua_State* L);

// Arguments: ped, path, mode (a one-node path only with mode 3), speed, then the
// callbacks at 5, 7, 8 and the start node at 6. Without a start node an objective on
// the same path is resumed. (A failed allocation is then written through)
int __cdecl Native_PedFollowPath(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int pathId = Script::GetInt(L, 1);
    int mode = Script::GetInt(L, 2);
    uint8_t* path = snpPatrolPath_Get(snpPatrolPaths, pathId);
    if (path == nullptr || snpPath_GetNumNodes(path) == 0 || (snpPath_GetNumNodes(path) == 1 && mode != 3)) {
        return 0;
    }
    bool hasStart = lua::gettop(L) >= 6;
    uint8_t* manager = ped + 0x5F0;
    uint8_t* objective = nullptr;
    if (!hasStart) {
        objective = snpObjectives_FindFollowPath(manager);
        if (objective != nullptr && snAt<int32_t>(objective, 0x10) != pathId) {
            objective = nullptr;
        }
    }
    if (objective == nullptr) {
        snpActionController_Stop(snAt<uint8_t*>(ped, 0x2E0) + 0x290);
        snpObjectives_Clear(manager);
        void* mem = snMalloc(0x40);
        objective = mem != nullptr ? static_cast<uint8_t*>(snpFollowPathObjective_Construct(mem, ped)) : nullptr;
        snObjectives_Add(manager, objective);
        snAt<int32_t>(objective, 0x10) = pathId;
        if (hasStart) {
            int start = Script::GetInt(L, 5);
            if (start >= 0) {
                snpPath_GetNumNodes(path); // (the result is unused)
            }
            snAt<int32_t>(objective, 0x14) = start;
        } else {
            snAt<int32_t>(objective, 0x14) = 0;
        }
    }
    snpObjectives_MoveToFront(manager, objective);
    snAt<int32_t>(objective, 0x24) = mode;
    snAt<float>(objective, 0x3C) = snpGetSpeedOpt(L, 3, *reinterpret_cast<const float*>(0x00911E30));
    snpLurStore_GetScript(snLurStore, L);
    Script::StoreFunctionRefOpt(L, 4, objective + 0x2C);
    Script::StoreFunctionRefOpt(L, 6, objective + 0x30);
    Script::StoreFunctionRefOpt(L, 7, objective + 0x34);
    return 0;
}

int __cdecl Native_PedResumePath(lua_State* L) {
    if (Script::IsNumber(L, 0)) {
        uint8_t* ped = snPedArg(L, 0);
        if (ped != nullptr) {
            uint8_t* objective = snpObjectives_FindFollowPath(ped + 0x5F0);
            if (objective != nullptr) {
                snpObjectives_MoveToFront(ped + 0x5F0, objective);
            }
        }
    }
    return 0;
}

GAME_FN(void, __cdecl, snpCleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);

// The hit record (the ped's virtual +0x40, its +0x10): cleared, its attacker released
int __cdecl Native_PedClearHitRecord(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        uint8_t* record = snAt<uint8_t*>(snVCall<uint8_t*>(ped, 0x40), 0x10);
        snAt<uint8_t>(record, 0x68) = 0;
        snAt<int32_t>(record, 0x64) = 0;
        snAt<uint8_t>(record, 0x34) = 0;
        record = snAt<uint8_t*>(snVCall<uint8_t*>(ped, 0x40), 0x10);
        re::Ptr32<void>* attacker = &snAt<void*>(record, 4);
        if (*attacker != nullptr) {
            snpCleanUpOldReference(*attacker, attacker);
            *attacker = nullptr;
        }
    }
    return 0;
}

// ---- targets, guards, stealth ------------------------------------------------------------

GAME_FN(void, __thiscall, snpPed_Fn498B10, 0x00498B10, void* ped, int);
GAME_FN(void, __thiscall, snpPed_SetTarget, 0x00497E70, void* ped, void* target, int kind, const SnVec* aimPoint);
// 0x4EA3C0: a trigger's entity
GAME_FN(uint8_t*, __cdecl, snpTrigger_GetEntity, 0x004EA3C0, void* trigger);
// 0x471300: an entity reference with a name (ped + 0x438)
GAME_FN(void, __thiscall, snpEntityRef_Set, 0x00471300, void* ref, void* entity, uint32_t name);
GAME_FN(void*, __thiscall, snpNameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, snpNameKey_Destruct, 0x0049A650, uint32_t* self);   // (empty)
GAME_FN(void, __cdecl, snpRegisterReference, 0x004657D0, void* entity, re::Ptr32<void>* ref);

// Argument 2: a trigger slot (0: no target), then strings (a name for the prop anim's
// reference) and numbers (the target kind) in any order
int __cdecl Native_PedTargetPAnim(lua_State* L) {
    int kind = 0;
    uint8_t* ped = snPedArg(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int slot = Script::GetInt(L, 1);
    if (slot == 0) {
        snpPed_Fn498B10(ped, 0);
        re::Ptr32<void>* ref = &snAt<void*>(ped, 0x438);
        if (*ref != nullptr) {
            snpCleanUpOldReference(*ref, ref);
            *ref = nullptr;
        }
        snAt<int32_t>(ped, 0x43C) = 0;
        return 0;
    }
    uint8_t* trigger = nullptr;
    if (slot >= 0) {
        CPool<uint8_t>* pool = snpTriggerPool;
        if ((pool->flags[slot] & 0x80) == 0) {
            trigger = pool->objects + pool->elemSize * slot;
        }
    }
    uint8_t* entity = snpTrigger_GetEntity(trigger);
    uint32_t name = 0xFFFFFFFF;
    for (int i = 2; i < lua::gettop(L); i++) {
        if (Script::IsString(L, i)) {
            uint32_t key;
            snpNameKey_Construct(&key, Script::GetString(L, i));
            name = key;
            snpNameKey_Destruct(&key);
        } else if (Script::IsNumber(L, i)) {
            kind = Script::GetInt(L, i);
        }
    }
    if (entity != nullptr && (entity[0x108] & 7) == 6) {
        snpEntityRef_Set(ped + 0x438, entity, name);
    }
    SnVec aim = {0.0f, 0.0f, 0.0f};
    snpPed_SetTarget(ped, entity, kind, &aim);
    return 0;
}

// The ped to guard (+0x1E08, a registered reference)
int __cdecl Native_PedGuardPed(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    uint8_t* other = snPedArg(L, 1);
    if (ped != nullptr) {
        re::Ptr32<void>* ref = &snAt<void*>(ped, 0x1E08);
        if (other != *ref) {
            if (*ref != nullptr) {
                snpCleanUpOldReference(*ref, ref);
            }
            *ref = other;
            if (other != nullptr) {
                snpRegisterReference(other, ref);
            }
        }
    }
    return 0;
}

int __cdecl Native_PedClearGuard(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        re::Ptr32<void>* ref = &snAt<void*>(ped, 0x1E08);
        if (*ref != nullptr) {
            snpCleanUpOldReference(*ref, ref);
            *ref = nullptr;
        }
    }
    return 0;
}

// The radar (0xC674CC)
GAME_VAR(void*, snpRadar, 0x00C674CC);
GAME_FN(void, __thiscall, snpRadar_AddEntityFlag, 0x0054A890, void* radar, int type, int handle);
GAME_FN(int32_t, __thiscall, snpRadar_AddEntityBlip, 0x0054A5B0, void* radar, int type, int handle, uint32_t color, int display, int sprite, int);
GAME_FN(void, __thiscall, snpRadar_SetBlipSize, 0x00548BF0, void* radar, int32_t blip, int size);

// The stealth mission marks: +0xE4, +0x1398 (seen or not), the radar's flags 6, 2, 0xD
// on the ped and a blip (sprite 2, size 5; shown only when `seen`)
void snpMarkStealthPed(uint8_t* ped, uint8_t seen) {
    snAt<int32_t>(ped, 0xE4) = 1;
    snAt<int32_t>(ped, 0x1398) = seen;
    int handle = snPoolHandle(snPedPool, ped);
    void* radar = snpRadar;
    snpRadar_AddEntityFlag(radar, 6, handle);
    snpRadar_AddEntityFlag(radar, 2, handle);
    snpRadar_AddEntityFlag(radar, 0xD, handle);
    int32_t blip = snpRadar_AddEntityBlip(radar, 2, handle, 0, seen != 0 ? 3 : 0, 2, 0);
    snpRadar_SetBlipSize(radar, blip, 5);
}

// 0x5D01E0 (cdecl)
void __cdecl SetStealthMissionPed(uint8_t* ped, int seen) {
    snpMarkStealthPed(ped, uint8_t(seen));
}

GAME_FN(void, __cdecl, snpSetStealthMissionPed, 0x005D01E0, uint8_t* ped, int seen);

int __cdecl Native_PedSetIsStealthMissionPed(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        snpSetStealthMissionPed(ped, Script::GetBoolOpt(L, 1, 1));
    }
    return 0;
}

// 0x5D80E0: a callback holds a live reference
GAME_FN(uint8_t, __thiscall, snpCallback_IsSet, 0x005D80E0, void* callback);

// Marked as a seen stealth ped (inlined 0x5D01E0), the behaviour (default 1) and two
// callbacks
int __cdecl Native_PedSetStealthBehavior(lua_State* L) {
    uint8_t* ped = snPedArg(L, 0);
    if (ped != nullptr) {
        snpMarkStealthPed(ped, 1);
        int behaviour = Script::GetIntOpt(L, 1, -1);
        snAt<int32_t>(ped, 0x18DC) = behaviour != -1 ? behaviour : 1;
        Script::StoreFunctionRefOpt(L, 2, ped + 0x18E0);
        snpCallback_IsSet(ped + 0x18E0); // (the result is unused)
        Script::StoreFunctionRefOpt(L, 3, ped + 0x18E4);
    }
    return 0;
}

GAME_FN(uint8_t*, __cdecl, snpGetVehicle, 0x0044A580, int handle);
GAME_FN(void, __thiscall, snpBike_SetRider, 0x004BDF80, void* bike, uint8_t* ped);
GAME_FN(void, __thiscall, snpPed_EnterVehicle, 0x00476640, void* ped, uint8_t* vehicle);
GAME_FN(int32_t*, __thiscall, snpVehicle_GetClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(void, __thiscall, snpPed_PlayAction, 0x004740F0, void* ped, const char* node, const char* file);

// Onto a bike (+0x46C vehicle type 1), riding the motorcycle / scooter / bike action
int __cdecl Native_PedPutOnBike(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int bikeHandle = Script::GetInt(L, 1);
    uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
    uint8_t* bike = snpGetVehicle(bikeHandle);
    if (ped == nullptr || bike == nullptr || snAt<int32_t>(bike, 0x46C) != 1) {
        return 0;
    }
    snpBike_SetRider(bike, ped);
    snpPed_EnterVehicle(ped, bike);
    const float* pos = snEntityPos(bike);
    snpEntity_SetPosition(ped, pos[0], pos[1], pos[2]);
    int32_t kind;
    if (*snpVehicle_GetClass(bike, &kind) == 3) {
        snpPed_PlayAction(ped, "/Global/Vehicles/Motorcycle/Vehicles_Ride", "Act/Vehicles.act");
        return 0;
    }
    if (*snpVehicle_GetClass(bike, &kind) == 8) {
        snpPed_PlayAction(ped, "/Global/Vehicles/Scooter/Vehicles_Ride", "Act/Vehicles.act");
        return 0;
    }
    snpPed_PlayAction(ped, "/Global/Vehicles/Bikes/Vehicles_Ride", "Act/Vehicles.act");
    return 0;
}

GAME_FN(uint8_t, __thiscall, snpPopulation_RemoveFarPed, 0x0049EB80, void* pop, int force);
GAME_FN(float, __cdecl, snpFindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int flags);

// 0x5D04C0: CreatePed's twin for a given heading: room is made in a full ped pool, the
// model is always kept by the script when it has to be loaded, and the ground is looked
// for with FindGroundZFor3DCoord. Returns the handle
int __cdecl CreatePedWithAdjustedHeading(lua_State*, int model, float x, float y, float z, float heading) {
    if (snpPedPopulation != nullptr) {
        CPool<uint8_t>* pool = snPedPool;
        int used = 0;
        for (int i = 0; i < pool->size; i++) {
            if ((pool->flags[i] & 0x80) == 0) {
                used++;
            }
        }
        if (pool->size - used < 1) {
            snpPopulation_RemoveFarPed(snpPedPopulation, 0);
        }
    }
    if (snpGetStreamingInfo(model)[4] != 1) {
        snpRequestModel(model, 0x16);
        snpScript_AddModel(snCurrentScript(snLurStore), model);
        snpLoadingScreenMessage("CreatePedWithAdjustedHeading()", "CStreaming::LoadAllRequestedModels()", 0);
        snpLoadAllRequestedModels(0);
    }
    void* mem = snpPedPool_New(0x1E90);
    uint8_t* ped = mem != nullptr ? snpCivilianPed_Construct(mem, model, 2, 0) : nullptr;
    uint8_t found = 0;
    float groundZ = snpFindGroundZFor3DCoord(x, y, float(z + 1.0), &found, 1);
    if (found) {
        z = float(groundZ + double(0.01f));
    }
    snpEntity_SetPosition(ped, x, y, z);
    if (heading == 0.0f) {
        snpPlaceable_SetOrientation(ped, 0.0f, 0.0f, 0.0f);
    } else {
        snpPed_SetHeading(ped, heading);
    }
    snpWorld_Add(ped);
    return snVCall<int>(ped, 0xAC);
}

GAME_FN(int, __cdecl, snpCreatePedWithAdjustedHeading, 0x005D04C0, lua_State* L, int model, float x, float y, float z, float heading);

// PedCreatePoint's twin (the route id taken as 16 bits; the ground left to 0x5D04C0)
int __cdecl Native_PedCreatePointWithAdjustedHeading(lua_State* L) {
    int count = lua::gettop(L);
    uint32_t index = 0; // (only read with three arguments)
    if (count == 3) {
        index = uint16_t(Script::GetInt(L, 2));
    }
    int model = Script::GetInt(L, 0);
    uint8_t* route = snpRoutes_Get(snpRoutes, uint16_t(Script::GetInt(L, 1)));
    uint8_t* point = snpRoute_First(route);
    if (count == 3) {
        for (int i = 1; i < int(index); i++) {
            uint8_t* next = snpRoute_Next(route);
            if (next == nullptr) {
                break;
            }
            point = next;
        }
    }
    float heading = float((double(snAt<float>(point, 0x1C)) - 90.0) * double(3.1415927f) / 180.0);
    int handle = snpCreatePedWithAdjustedHeading(L, model, snAt<float>(point, 0), snAt<float>(point, 4), snAt<float>(point, 8), heading);
    if (handle != -1) {
        snpScript_AddPed(snCurrentScript(snLurStore), handle);
    }
    Script::PushInt(L, handle);
    return 1;
}

// The POIs (0xC0F5E4)
GAME_VAR(void*, snpPOIManager, 0x00C0F5E4);
GAME_FN(uint8_t*, __thiscall, snpPOIManager_Get, 0x00448DC0, void* manager, int index);
GAME_FN(uint8_t, __thiscall, snpPOIManager_Fn448730_x64, 0x00448730, void* manager, uint8_t* poi, float, float, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t snpPOIManager_Fn448730(void* a0, uint8_t* a1, SnVec a2) {
    return snpPOIManager_Fn448730_x64(a0, a1, re::FloatAt(a2, 0), re::FloatAt(a2, 1), re::FloatAt(a2, 2));
}
GAME_FN(uint8_t, __thiscall, snpPOI_AddPed, 0x00447410, void* poi, uint8_t* ped, int flag);
GAME_FN(void, __thiscall, snpPOI_SetActivation, 0x004478F0, void* poi, int active);
GAME_FN(int, __thiscall, snpPopulation_GetTotalNumPeds, 0x0049C2F0, void* pop);
GAME_FN(uint8_t, __thiscall, snpPopulation_CanAddPed, 0x0049E9E0, void* pop);
// the models a POI ped can't be (0xBF42CC..0xBF42DC)
GAME_VAR(int32_t, snpPOIExcludedModel0, 0x00BF42DC);
GAME_VAR(int32_t, snpPOIExcludedModel1, 0x00BF42D4);
GAME_VAR(int32_t, snpPOIExcludedModel2, 0x00BF42D0);
GAME_VAR(int32_t, snpPOIExcludedModel3, 0x00BF42CC);

// CColPoint (as far as it's set here)
struct SnpColPoint {
    SnVec pos;              // +0
    re::Ptr32<void> entity;           // +0xC (a registered reference)
    SnVec normal;           // +0x10
    uint8_t unknown1C[8];   // +0x1C
    float depth;            // +0x24
};
static_assert(sizeof(SnpColPoint) == 0x28);
GAME_FN(uint8_t, __cdecl, snpProcessVerticalLine, 0x004629C0, const SnVec* pos, float zStop, SnpColPoint* colPoint, re::Ptr32<void>* entity, int flags, int);
GAME_FN(float, __cdecl, snpAngleToHeading, 0x004333A0, float degrees);
GAME_FN(void, __cdecl, snpRwObject_SetVisible, 0x005F2B00, void* object, int visible);
GAME_FN(void, __cdecl, snpSetModelNotNeededAmbient, 0x0052FE40, int model);

// A ped of the model at a POI (+ an offset, arguments 3..5), facing the POI's heading
// (+0x68), when the population allows it (the POI's own limit at +0xA1 when +0xA3 is
// set), there are 4 free ped slots, no other ped of the model (bar the excluded ones and
// +0x1C98 set) exists, the POI accepts it (0x448730, from the player's position) and
// nothing is over the spot. The handle, or -1
int __cdecl Native_PedCreatePOIPoint(lua_State* L) {
    uint8_t ok = 1;
    int model = Script::GetInt(L, 0);
    int poiIndex = Script::GetInt(L, 1);
    int handle = -1;
    uint8_t* poi = snpPOIManager_Get(snpPOIManager, poiIndex);
    void* pop = snpPedPopulation;
    if (pop != nullptr) {
        if (poi != nullptr && snAt<uint8_t>(poi, 0xA3) != 0) {
            int limit = snAt<int8_t>(poi, 0xA1);
            if (snpPopulation_GetTotalNumPeds(pop) > limit) {
                ok = 0;
            }
        } else if (!snpPopulation_CanAddPed(pop)) {
            ok = 0;
        }
    }
    CPool<uint8_t>* pool = snPedPool;
    int used = 0;
    for (int i = 0; i < pool->size; i++) {
        if ((pool->flags[i] & 0x80) == 0) {
            used++;
        }
    }
    if (pool->size - used < 4) {
        ok = 0;
    }
    bool found = false;
    for (int i = pool->size; i != 0;) {
        i--;
        if ((pool->flags[i] & 0x80) != 0) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * i;
        if (ped == nullptr || snAt<uint8_t>(ped, 0x1C98) != 0) {
            continue;
        }
        int pedModel = snAt<int16_t>(ped, 0x10E);
        if (pedModel == snpPOIExcludedModel0 || pedModel == snpPOIExcludedModel1 || pedModel == snpPOIExcludedModel2 ||
            pedModel == snpPOIExcludedModel3) {
            continue;
        }
        if (pedModel == model) {
            found = true;
            break;
        }
    }
    if (found) {
        ok = 0;
    } else if (ok && poi != nullptr) {
        const float* p = snEntityPos(snPlayerPed);
        SnVec playerPos = {p[0], p[1], p[2]};
        SnVec zero = {0.0f, 0.0f, 0.0f};
        SnVec offset;
        Script::GetVectorOpt(reinterpret_cast<ScriptVector*>(&offset), L, 2, reinterpret_cast<ScriptVector*>(&zero));
        SnVec pos = snpAreaClearPoint; // (overwritten)
        const float* poiPos = snEntityPos(poi);
        pos.x = poiPos[0] + offset.x;
        pos.y = poiPos[1] + offset.y;
        pos.z = poiPos[2] + offset.z;
        SnpColPoint colPoint;
        colPoint.pos = {0.0f, 0.0f, 0.0f};
        colPoint.entity = nullptr;
        colPoint.normal = {0.0f, 0.0f, 0.0f};
        colPoint.depth = *reinterpret_cast<const float*>(0x009009B8); // (FLT_MAX)
        uint8_t accepted = snpPOIManager_Fn448730(snpPOIManager, poi, playerPos);
        re::Ptr32<void> hitEntity;
        if (!snpProcessVerticalLine(&pos, -100.0f, &colPoint, &hitEntity, 4, 0) && accepted) {
            float heading = snpAngleToHeading(snAt<float>(poi, 0x68));
            handle = snpCreatePed(L, model, pos.x, pos.y, pos.z, heading, 0);
            if (handle == -1) {
                ok = 0;
            } else {
                snpScript_AddPed(snCurrentScript(snLurStore), handle);
                uint8_t* ped = snGetPed(handle, SN_HANDLE_PED);
                snpRwObject_SetVisible(snAt<void*>(ped, 0x18), 1);
                snpPOI_AddPed(poi, ped, 1);
            }
        } else {
            ok = 0;
            int kind = snAt<int32_t>(poi, 0x70);
            if (kind > 2 && kind < 0x10) {
                snpPOI_SetActivation(poi, 0);
            }
        }
        if (colPoint.entity != nullptr) {
            snpCleanUpOldReference(colPoint.entity, &colPoint.entity);
        }
    }
    snpSetModelNotNeededAmbient(model);
    Script::PushInt(L, ok ? handle : -1);
    return 1;
}

// An array of handles the game grows (data, capacity, count)
struct SnpHandleArray {
    re::Ptr32<int32_t> data;      // +0
    uint32_t capacity;  // +4
    uint32_t count;     // +8
};
static_assert(sizeof(SnpHandleArray) == 0xC);
GAME_FN(void, __thiscall, snpHandleArray_Reserve, 0x006DB480, SnpHandleArray* self, uint32_t capacity, int);

GAME_FN(uint8_t, __thiscall, snpEntity_IsInArea2D, 0x0046D9D0, void* entity, float x1, float y1, float x2, float y2);

// 0x5CF9B0: the handles of the peds within x +- rx, y +- ry (and "z +- rz", tested as
// either bound, so always) that aren't +0x78 or +0x1338 flagged; any found
uint8_t __cdecl FindPedsInArea(SnpHandleArray* out, float x, float y, float z, float rx, float ry, float rz) {
    if (out->data != nullptr) {
        snFree(out->data);
        out->data = nullptr;
    }
    out->capacity = 0;
    out->count = 0;
    float x1 = x - rx, y1 = y - ry, z1 = z - rz;
    float x2 = x + rx, y2 = y + ry, z2 = z + rz;
    uint8_t found = 0;
    for (int i = 0; i < snPedPool->size; i++) {
        CPool<uint8_t>* pool = snPedPool;
        if ((pool->flags[i] & 0x80) != 0) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * i;
        if (ped == nullptr || snAt<int32_t>(ped, 0x78) != 0 || snAt<int32_t>(ped, 0x1338) != 0) {
            continue;
        }
        if (!snpEntity_IsInArea2D(ped, x1, y1, x2, y2)) {
            continue;
        }
        float pz = snEntityPos(ped)[2];
        if (!(z1 > pz) || !(z2 < pz)) {
            int handle = snVCall<int>(ped, 0xAC);
            if (out->count == out->capacity) {
                snpHandleArray_Reserve(out, out->capacity != 0 ? out->capacity * 2 : 1, 0);
            }
            out->data[out->count] = handle;
            out->count++;
            found = 1;
        }
    }
    return found;
}

GAME_FN(uint8_t, __cdecl, snpFindPedsInArea, 0x005CF9B0, SnpHandleArray* out, float x, float y, float z, float rx, float ry, float rz);

// Any found, then their handles
int __cdecl Native_PedFindInAreaXYZ(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    float r = Script::GetFloat(L, 3);
    SnpHandleArray peds = {nullptr, 0, 0};
    Script::PushBool(L, snpFindPedsInArea(&peds, x, y, z, r, r, r));
    uint32_t count = peds.count;
    for (uint32_t i = 0; i < count; i++) {
        Script::PushInt(L, peds.data[i]);
    }
    if (peds.data != nullptr) {
        snFree(peds.data);
    }
    return int(count + 1);
}

// 0x5C2970: the position of a script handle's entity
GAME_FN(SnVec*, __cdecl, snpGetHandlePosition, 0x005C2970, SnVec* out, int handle, int kind);

// Around a ped (the game passes its y as the z too)
int __cdecl Native_PedFindInAreaObject(lua_State* L) {
    if (snPedArg(L, 0) == nullptr) {
        return 0;
    }
    int handle = Script::GetInt(L, 0);
    float r = Script::GetFloat(L, 1);
    SnpHandleArray peds = {nullptr, 0, 0};
    SnVec pos;
    snpGetHandlePosition(&pos, handle, SN_HANDLE_PED);
    Script::PushBool(L, snpFindPedsInArea(&peds, pos.x, pos.y, pos.y, r, r, r));
    uint32_t count = peds.count;
    for (uint32_t i = 0; i < count; i++) {
        Script::PushInt(L, peds.data[i]);
    }
    if (peds.data != nullptr) {
        snFree(peds.data);
    }
    return int(count + 1);
}

// The butes (tree names, 0x1B652E8 the root, 0x1B652EC the group)
GAME_FN(void, __thiscall, snpName_Copy, 0x005FA880, uint32_t* self, const uint32_t* from);
GAME_FN(void, __thiscall, snpPath_Build, 0x005FA8C0, uint32_t* path, int count, const uint32_t* names);
GAME_FN(void*, __cdecl, snpButes_Find, 0x0072AEA0, const uint32_t* path, const uint32_t* root);

// 0x5D0C80: the butes of a name under the group at 0x1B652EC
void* __cdecl FindGroupButes(const uint32_t* name) {
    uint32_t names[2];
    uint32_t path[2];
    snpName_Copy(&names[0], reinterpret_cast<const uint32_t*>(0x01B652EC));
    snpName_Copy(&names[1], name);
    snpPath_Build(path, 2, names);
    void* found = snpButes_Find(path, reinterpret_cast<const uint32_t*>(0x01B652E8));
    for (int i = 1; i >= 0; i--) {
        snpNameKey_Destruct(&names[i]);
    }
    return found;
}

} // namespace

#define SNP_REVERSED(name, address)       \
    static void* Original_Native_##name; \
    REVERSED("Native_" #name, address, RE_BODY(Native_##name), nullptr, &Original_Native_##name, re::Mode::Original)

SNP_REVERSED(PedSetProjDest, 0x005CD5D0);
SNP_REVERSED(PedRecruitAlly, 0x005CD6B0);
SNP_REVERSED(PedDismissAlly, 0x005CD730);
SNP_REVERSED(PedGetEmotionTowardsPed, 0x005CDBD0);
SNP_REVERSED(PedSetEmotionTowardsPed, 0x005CDC40);
SNP_REVERSED(PedSetWantsToSocializeWithPed, 0x005CDCD0);
SNP_REVERSED(PedSetRequiredGift, 0x005CDD80);
SNP_REVERSED(PedAttackCar, 0x005CDEB0);
SNP_REVERSED(PedAttackNearestCar, 0x005CDF40);
SNP_REVERSED(PedGetHeadPos, 0x005CE090);
SNP_REVERSED(PedIsPropAttached, 0x005CE240);
SNP_REVERSED(PedRooftopAttacker, 0x005CE3B0);
SNP_REVERSED(PedAutoCover, 0x005CE4E0);
SNP_REVERSED(PedGetOffsetInWorldCoords, 0x005CE660);
SNP_REVERSED(PedUseProp, 0x005CE8F0);
SNP_REVERSED(PedSetGrappleTarget, 0x005CEA10);
SNP_REVERSED(PedJoinCombatZone, 0x005CEA80);
SNP_REVERSED(PedLeaveCombatZone, 0x005CEB40);
SNP_REVERSED(PedIsTargetable, 0x005CEC50);
SNP_REVERSED(PedAttachEffect, 0x005CEE60);
SNP_REVERSED(PedDetachAndKillEffect, 0x005CEEF0);
SNP_REVERSED(PedCreateStimulus, 0x005CEF80);
SNP_REVERSED(PedIsPedInBox, 0x005CF270);
SNP_REVERSED(PedBikeBrawl, 0x005CF3A0);
SNP_REVERSED(PedFindRandomSpawnPosition, 0x005CF5B0);
SNP_REVERSED(PedEnableGiftRequirement, 0x005CF730);
static void* Original_Objectives_FindFollowPath;
REVERSED("ObjectiveManager::FindFollowPath", 0x005CF7D0, RE_BODY(Objectives_FindFollowPath), nullptr,
         &Original_Objectives_FindFollowPath, re::Mode::Original);
static void* Original_CreatePed;
REVERSED("Script::CreatePed", 0x005CF810, RE_BODY(CreatePed), nullptr, &Original_CreatePed, re::Mode::Original);
SNP_REVERSED(PedCreatePoint, 0x005CFB30);
SNP_REVERSED(PedCreateXYZ, 0x005CFCB0);
SNP_REVERSED(PedFollowPath, 0x005CFD50);
SNP_REVERSED(PedResumePath, 0x005CFEE0);
SNP_REVERSED(PedClearHitRecord, 0x005CFF40);
SNP_REVERSED(PedTargetPAnim, 0x005CFFB0);
SNP_REVERSED(PedGuardPed, 0x005D0110);
SNP_REVERSED(PedClearGuard, 0x005D0190);
static void* Original_SetStealthMissionPed;
REVERSED("Script::SetStealthMissionPed", 0x005D01E0, RE_BODY(SetStealthMissionPed), nullptr,
         &Original_SetStealthMissionPed, re::Mode::Original);
SNP_REVERSED(PedSetIsStealthMissionPed, 0x005D0270);
SNP_REVERSED(PedSetStealthBehavior, 0x005D02C0);
SNP_REVERSED(PedPutOnBike, 0x005D03C0);
static void* Original_CreatePedWithAdjustedHeading;
REVERSED("Script::CreatePedWithAdjustedHeading", 0x005D04C0, RE_BODY(CreatePedWithAdjustedHeading), nullptr,
         &Original_CreatePedWithAdjustedHeading, re::Mode::Original);
SNP_REVERSED(PedCreatePointWithAdjustedHeading, 0x005D0640);
SNP_REVERSED(PedCreatePOIPoint, 0x005D0770);
static void* Original_FindPedsInArea;
REVERSED("Script::FindPedsInArea", 0x005CF9B0, RE_BODY(FindPedsInArea), nullptr, &Original_FindPedsInArea,
         re::Mode::Original);
SNP_REVERSED(PedFindInAreaXYZ, 0x005D0AC0);
SNP_REVERSED(PedFindInAreaObject, 0x005D0B90);
static void* Original_FindGroupButes;
REVERSED("Script::FindGroupButes_5D0C80", 0x005D0C80, RE_BODY(FindGroupButes), nullptr, &Original_FindGroupButes,
         re::Mode::Original);
