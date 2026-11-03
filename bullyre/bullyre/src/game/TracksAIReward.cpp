// The action tree's tracks of 0x61C760..0x61DB40: the player's reward (picked by
// RewardPlayerTrack: mission or shop clothing, weapons, ammo or items; given by
// GivePlayerRewardTrack), grapple target setters, the day, seek entity position,
// idle, controller, scripted ambient and the wrestle track's Start.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

// The reward being offered (0xC1AFA8)
struct Reward {
    int32_t hash;       // +0x0 model / clothing hash (-1: none)
    int32_t a;          // +0x4
    int32_t slot;       // +0x8 clothing slot (6: a whole outfit)
    uint8_t clothing;   // +0xC
    uint8_t outfit;     // +0xD
    uint8_t ammo;       // +0xE
    uint8_t countStat;  // +0xF
};
static_assert(sizeof(Reward) == 0x10);
inline Reward& reward = *reinterpret_cast<Reward*>(0x00C1AFA8);

// a reward choice: the mission it needs (done or under way; "" none), its value, and
// for clothing its name, a and slot
struct RewardEntry {
    re::Ptr32<const char> mission;  // +0x0
    int32_t value;        // +0x4
    re::Ptr32<const char> item;     // +0x8
    int32_t a;            // +0xC
    int32_t slot;         // +0x10
};
static_assert(sizeof(RewardEntry) == 0x14);

struct ClothingItem {
    uint32_t hash;
    uint8_t slot;
    uint8_t pad[3];
};

constexpr uint32_t kVtSeekEntityPositionTrack = 0x00933634;
inline void* const missionManager = reinterpret_cast<void*>(0x020C3CA0);
inline void* const outfits = reinterpret_cast<void*>(0x020C5F70);
inline const char* S(uintptr_t a) {
    return reinterpret_cast<const char*>(a);
}
GAME_VAR(int32_t, firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, lastWeapon, 0x00A136B4);
GAME_VAR(int32_t, firstItem, 0x00A136B8);
GAME_VAR(int32_t, lastItem, 0x00A136BC);
GAME_VAR(int32_t, rewardAmmoWeapon, 0x00BF4434);
GAME_VAR(int32_t, rewardItemA, 0x00BF427C);
GAME_VAR(int32_t, rewardItemB, 0x00BF4278);
GAME_VAR(uint8_t*, hudInventory, 0x00C67554);
GAME_VAR(uint8_t*, hudWeapons, 0x00C674F0);
GAME_VAR(int32_t, currentDay, 0x020C3640);
GAME_VAR(uint8_t, playerSeeking, 0x00C2B10A);
GAME_VAR(int32_t, controllerLockA, 0x00C2B120);
GAME_VAR(int32_t, controllerLockB, 0x00C2B124);
GAME_VAR(float, combatStepRate, 0x00B0C0B0);
GAME_VAR(uint32_t, getOnRhsGuard, 0x01B65674);
inline uint32_t* const getOnRhs = reinterpret_cast<uint32_t*>(0x01B65670);   // "GetOnRHS"
GAME_FN(void, __thiscall, TR_NameKey_Destruct, 0x0049A650, uint32_t* key);
inline const float* const zeroVec = reinterpret_cast<const float*>(0x00BCC564);

inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(int, __cdecl, RandomRange, 0x0040DA80, int least, int most);
GAME_FN(uint32_t*, __thiscall, NameKey_ConstructFrom, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(int32_t, __thiscall, Missions_Find, 0x006A9FD0, void* manager, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, Missions_GetTaskFlags, 0x006AA7C0, void* manager, int32_t mission);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, ClothingItem_FromHash, 0x006CA4F0, ClothingItem* item, uint32_t hash, int slot);
GAME_FN(uint8_t, __thiscall, Clothing_PlayerOwns, 0x006CA850, void* outfits, const ClothingItem* item);
GAME_FN(uint8_t*, __thiscall, Clothing_FindOutfit, 0x006CAA90, void* outfits, uint32_t hash);
GAME_FN(uint8_t, __thiscall, Clothing_OwnsOutfit, 0x006CB880, void* outfits, uint8_t* outfit);
GAME_FN(void, __thiscall, Clothing_GivePlayer, 0x006CB6C0, void* outfits, const ClothingItem* item, int);
GAME_FN(void, __thiscall, Clothing_GivePlayerOutfit, 0x006CB7E0, void* outfits, uint8_t* outfit, int);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int32_t model);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int32_t model, int flags);
GAME_FN(uint8_t, __cdecl, Items_Has, 0x00541E90, int32_t item);
GAME_FN(uint8_t, __cdecl, Items_IsType3, 0x00541AC0, int32_t item);
GAME_FN(void, __thiscall, HUDInventory_ShowText, 0x00541170, uint8_t* hud, int32_t a, int32_t hash);
GAME_FN(void, __cdecl, IncrementStat, 0x00458480, int stat, int amount);
GAME_FN(void, __thiscall, WeaponInventory_AddAmmo, 0x0045CED0, uint8_t* inventory, int32_t weapon, int amount, int,
        int);
GAME_FN(void, __thiscall, WeaponInventory_GiveWeapon, 0x0045CE00, uint8_t* inventory, int32_t weapon, int, int);
GAME_FN(void, __thiscall, HUDWeapons_Select, 0x0055C840, uint8_t* hud, int32_t weapon);
GAME_FN(void, __cdecl, Items_Add, 0x00541C30, int32_t item, int, int, int);
GAME_FN(void, __thiscall, GrappleTarget_Reset, 0x005F55A0, uint8_t* target);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnimAt, 0x006B1FA0, uint8_t* blend, int32_t slot);
GAME_FN(int16_t, __cdecl, AnimSlot_TypeId, 0x006075F0);
GAME_FN(float, __thiscall, RangeMap_Get, 0x00608630, uint8_t* map, float value);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412A10, void* dst, const void* src);
GAME_FN(void, __thiscall, Matrix_ToNiTransform, 0x00412AC0, const void* matrix, void* out);
GAME_FN(void, __thiscall, Matrix_UpdateAttached, 0x00413A70, void* matrix);
GAME_FN(void, __thiscall, Matrix_SetRotateZ, 0x00413220, void* matrix, float angle);
GAME_FN(uint8_t*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrixPtr, 0x008B6120, uint8_t* entity);
GAME_FN(uint8_t*, __thiscall, Ped_GetMatrix, 0x004734D0, uint8_t* ped);
GAME_FN(uint8_t*, __cdecl, Frame_GetLTM, 0x006C9A90, uint8_t* frame);
GAME_FN(uint8_t, __cdecl, Entity_GetNodeTransform, 0x00467D80, uint8_t* entity, uint32_t hash, void* out, int);
inline void __cdecl AtExit(void* fn) { crt::Atexit((void(__cdecl*)())fn); }
GAME_FN(uint8_t*, __thiscall, Seats_GetDoor, 0x004CE4C0, uint8_t* seats, int side);
GAME_FN(uint8_t, __cdecl, VectorEqual, 0x00417870, const void* a, const void* b);
GAME_FN(V3*, __cdecl, TransformPoint, 0x005E17D0, V3* out, const void* matrix, const V3* v);
GAME_FN(uint8_t, __thiscall, Ped_IsInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(float, __cdecl, RandomFloat, 0x0041A5B0, float least, float most);
GAME_FN(void, __thiscall, ActionInputs_Clear, 0x005F3A40, uint8_t* inputs);
GAME_FN(void, __thiscall, ScriptedAmbient_Stop, 0x004475D0, uint8_t* ambient, uint8_t* ped);

void ResetReward(bool countStat) {
    reward.hash = -1;
    reward.a = 0xF;
    reward.slot = 6;
    reward.clothing = 0;
    reward.outfit = 0;
    reward.ammo = 0;
    if (countStat) {
        reward.countStat = 1;
    }
}
int RandomIndex(int span) {
    int r = GameRand();
    return r * span / 32767;
}
uint32_t Length(const char* s) {
    uint32_t n = 0;
    while (s[n] != 0) {
        n++;
    }
    return n;
}
// the entry's mission allows it: none named (checked on check), else its task started
// or done (the key made from keyName)
bool MissionAllows(const char* keyName, const char* check, bool* none = nullptr) {
    uint32_t key;
    NameKey_ConstructFrom(&key, keyName);
    int32_t id = Missions_Find(missionManager, &key);
    bool ok;
    if (Length(check) == 0) {
        ok = true;
        if (none) {
            *none = true;
        }
    } else if (id == -1) {
        ok = false;
    } else if (At<uint16_t>(Missions_GetTaskFlags(missionManager, id), 2) > 0) {
        ok = true;
    } else {
        ok = (Missions_GetTaskFlags(missionManager, id)[4] & 8) != 0;
    }
    NameKey_Destruct(&key);
    return ok;
}
// the clothing (outfit when its slot is 6) already owned
bool OwnsClothing(uint32_t hash, int32_t slot) {
    if (slot < 6) {
        reward.outfit = 0;
        ClothingItem item;
        ClothingItem_FromHash(&item, hash, slot);
        return Clothing_PlayerOwns(outfits, &item) != 0;
    }
    reward.outfit = 1;
    uint8_t* outfit = Clothing_FindOutfit(outfits, hash);
    if (outfit[0x50] & 1) {
        return true;
    }
    return Clothing_OwnsOutfit(outfits, outfit) != 0;
}

// 0x61C760 (thiscall): RewardPlayerTrack::Start: the reward picked: a mission's special
// outfit, else a shop's clothes, else a weapon, charm or item (10 tries), or (a fifth of
// the time) a candy
uint8_t __fastcall RewardPlayerTrack_Start(uint8_t*, re::Edx, uint8_t*) {
    ResetReward(true);
    int q = RandomIndex(99);
    int kind = 0;
    if (uint32_t(q - 50) <= 29) {
        kind = 1;
    } else if (uint32_t(q - 30) <= 19) {
        kind = 2;
    } else if (uint32_t(q - 80) <= 19) {
        kind = 3;
    }
    if (kind == 3) {
        RewardEntry candy[2] = {{S(0x009334F0), rewardItemA, nullptr, 0, 0},    // "1_02C"
                                {S(0x008FEFBB), rewardItemB, nullptr, 0, 0}};   // ""
        reward.hash = candy[RandomRange(0, 2)].value;
        return 1;
    }
    if (kind == 0) {
        // (their +4 isn't set)
        RewardEntry specials[3] = {{S(0x00933628), 0, S(0x00933618), 0, 0},       // "1_11x1": SP_Zorromask
                                   {S(0x00933628), 0, S(0x0093360C), 0, 0},       // SP_Werewolf
                                   {S(0x00933628), 0, S(0x00933600), 0xF, 6}};    // Ninja_RED
        bool picked = false;
        int idx = 0;
        for (int i = 0; i < 3; i++) {
            idx = RandomIndex(2);
            // (the key is the loop's entry's, the check the random one's)
            if (MissionAllows(specials[i].mission, specials[idx].mission)) {
                picked = true;
                break;
            }
        }
        if (picked && idx != -1) {
            RewardEntry& e = specials[idx];
            uint32_t hash = HashName(e.item);
            reward.hash = int32_t(hash);
            reward.slot = e.slot;
            reward.clothing = 1;
            reward.a = e.a;
            if (!OwnsClothing(hash, e.slot)) {
                reward.countStat = 0;
                return 1;
            }
            ResetReward(true);
        }
        kind = 1;
    }
    if (kind == 1) {
        const char* none = S(0x008FEFBB);
        const char* b = S(0x00921998);   // "1_B"
        RewardEntry clothes[18] = {
            {none, 0, S(0x009335F4), 0, 1}, {none, 0, S(0x009335E8), 0, 1}, {none, 0, S(0x009335DC), 0, 4},
            {none, 0, S(0x009335D0), 0, 1}, {none, 0, S(0x009335C4), 0, 1}, {b, 0, S(0x009335B8), 0, 4},
            {b, 0, S(0x009335AC), 0, 5},    {b, 0, S(0x009335A0), 0, 1},    {b, 0, S(0x00933594), 0, 1},
            {b, 0, S(0x00933588), 0, 4},    {b, 0, S(0x0093357C), 0, 5},    {b, 0, S(0x00933570), 0, 5},
            {b, 0, S(0x00933560), 0, 5},    {b, 0, S(0x00933554), 0, 1},    {b, 0, S(0x00933548), 0, 4},
            {b, 0, S(0x0093353C), 0, 5},    {b, 0, S(0x00933530), 0, 5},    {b, 0, S(0x00933524), 0, 4},
        };
        bool picked = false;
        int idx = 0;
        for (int i = 0; i < 18; i++) {
            idx = RandomIndex(17);
            if (MissionAllows(clothes[i].mission, clothes[idx].mission)) {
                picked = true;
                break;
            }
        }
        if (picked && idx != -1) {
            RewardEntry& e = clothes[idx];
            uint32_t hash = HashName(e.item);
            reward.a = e.a;
            reward.hash = int32_t(hash);
            reward.slot = e.slot;
            reward.clothing = 1;
            if (!OwnsClothing(hash, e.slot)) {
                return 1;
            }
            ResetReward(false);
        }
    }
    // weapons (k 0), charms / chemistry (k 1), ammo (k 2: needs its mission)
    RewardEntry table[11];
    int32_t found = -1;
    int count = 0;
    do {
        count++;
        if (found != -1) {
            break;
        }
        int k = RandomIndex(2);
        if (k == 0) {
            const char* none = S(0x008FEFBB);
            const int32_t models[11] = {At<int32_t>(reinterpret_cast<void*>(0x00BF4158), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF4154), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF4150), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF414C), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF4148), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF4428), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF4344), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF43F4), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF43F4), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF43F4), 0),
                                        At<int32_t>(reinterpret_cast<void*>(0x00BF43F4), 0)};
            for (int i = 0; i < 11; i++) {
                table[i].mission = none;
                table[i].value = models[i];
            }
            int idx = RandomIndex(10);
            found = MissionAllows(table[idx].mission, table[idx].mission) ? idx : -1;
        } else if (k == 1) {
            table[0] = {S(0x00933628), At<int32_t>(reinterpret_cast<void*>(0x00BF43E8), 0), nullptr, 0, 0};
            table[1] = {S(0x00933628), At<int32_t>(reinterpret_cast<void*>(0x00BF4364), 0), nullptr, 0, 0};
            table[2] = {S(0x00933504), At<int32_t>(reinterpret_cast<void*>(0x00BF4448), 0), nullptr, 0, 0};
            table[3] = {S(0x009334F8), At<int32_t>(reinterpret_cast<void*>(0x00BF442C), 0), nullptr, 0, 0};
            table[4] = {S(0x00933518), At<int32_t>(reinterpret_cast<void*>(0x00BF4384), 0), nullptr, 0, 0};
            int idx = RandomIndex(4);
            found = MissionAllows(table[idx].mission, table[idx].mission) ? idx : -1;
        } else if (k == 2) {
            table[0] = {S(0x009219A4), At<int32_t>(reinterpret_cast<void*>(0x00BF4434), 0), nullptr, 0, 0};
            table[1] = {S(0x0090A374), At<int32_t>(reinterpret_cast<void*>(0x00BF435C), 0), nullptr, 0, 0};
            table[2] = {S(0x00933510), At<int32_t>(reinterpret_cast<void*>(0x00BF4358), 0), nullptr, 0, 0};
            int idx = RandomIndex(2);
            if (MissionAllows(table[idx].mission, table[idx].mission)) {
                reward.ammo = 1;
                found = idx;
            } else {
                found = -1;
            }
        }
    } while (count < 10);
    if (found == -1) {
        return 1;
    }
    reward.hash = table[found].value;
    return 1;
}

// 0x61D1A0 (thiscall): RewardPlayerTrack::Update: the rewarded model streamed in
uint8_t __fastcall RewardPlayerTrack_Update(uint8_t*, re::Edx, float) {
    if (reward.clothing == 0 && reward.hash != -1) {
        if (GetStreamingInfo(reward.hash)[4] != 2 && GetStreamingInfo(reward.hash)[4] != 1) {
            RequestModel(reward.hash, 0);
        }
    }
    return 1;
}

// 0x61D1F0 (thiscall): GivePlayerRewardTrack::Start: the reward given (without one, or
// its model not loaded: an item the player has, of type 3); the reward stat counted
uint8_t __fastcall GivePlayerRewardTrack_Start(uint8_t*, re::Edx, uint8_t*) {
    uint8_t* player = playerPed;
    int32_t id = reward.hash;
    bool pick = id == -1;
    if (!pick && reward.clothing == 0 && GetStreamingInfo(id)[4] != 1) {
        pick = true;
    }
    if (pick) {
        int32_t item = RandomIndex(0x4F);
        bool found = false;
        for (int tries = 1; !found; tries++) {
            for (; item < 0x50; item++) {
                if (Items_Has(item) && Items_IsType3(item)) {
                    reward.hash = item;
                    found = true;
                    break;
                }
            }
            if (!found && tries >= 100) {
                break;
            }
        }
        id = reward.hash;
    }
    if (id == -1) {
        return 1;
    }
    if (reward.clothing != 0) {
        if (reward.outfit != 0) {
            uint8_t* outfit = Clothing_FindOutfit(outfits, id);
            if (!outfit) {
                return 1;
            }
            outfit[0x50] |= 8;
            Clothing_GivePlayerOutfit(outfits, outfit, 1);
        } else {
            ClothingItem item;
            ClothingItem_FromHash(&item, id, reward.slot);
            if (Clothing_PlayerOwns(outfits, &item)) {
                HUDInventory_ShowText(hudInventory, reward.a, reward.hash);
            } else {
                Clothing_GivePlayer(outfits, &item, 1);
            }
        }
        if (reward.countStat != 0) {
            IncrementStat(0x42, 1);
        }
    } else if (id >= firstWeapon && id <= lastWeapon) {
        if (reward.ammo != 0) {
            WeaponInventory_AddAmmo(At<uint8_t*>(playerPed, 0x1C4), id, id != rewardAmmoWeapon ? 1 : 12, 1, 1);
        } else {
            WeaponInventory_GiveWeapon(At<uint8_t*>(player, 0x1C4), id, 1, 1);
            At<int32_t>(At<uint8_t*>(player, 0x2E0), 0x18) = reward.hash;
            HUDWeapons_Select(hudWeapons, reward.hash);
        }
    } else if (id >= firstItem && id <= lastItem) {
        if (id == rewardItemA) {
            IncrementStat(0x24, 1);
            id = reward.hash;
        } else if (id == rewardItemB) {
            IncrementStat(0x26, 1);
            id = reward.hash;
        }
        Items_Add(id, 1, 1, 1);
    } else {
        return 1;
    }
    IncrementStat(0xD0, 1);
    return 1;
}

// 0x61D400 / 0x61D410 / 0x61D470 (thiscall): field initialisers
void __fastcall InitFields_61D400(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<int32_t>(self, 0x18) = -1;
}
void __fastcall InitFields_61D410(uint8_t* self, re::Edx) {
    self[8] = 1;
    self[0x18] = 0;
}
void __fastcall InitFields_61D470(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<int32_t>(self, 0x18) = 0;
}

// 0x61D420 (thiscall): SetTargetAsGrappleTargetTrack's two targets reset (vf6)
void __fastcall SetTargetAsGrappleTargetTrack_Reset(uint8_t* self, re::Edx) {
    GrappleTarget_Reset(self + 0x1C);
    GrappleTarget_Reset(self + 0x2C);
}
// 0x61D440 (thiscall): their node set (vf7, each target's own +0x1C virtual)
void __fastcall SetTargetAsGrappleTargetTrack_SetNode(uint8_t* self, re::Edx, uint8_t* node) {
    vc::Call<void>(self + 0x1C, 0x1C, node);
    vc::Call<void>(self + 0x2C, 0x1C, node);
}

// 0x61D480 (thiscall): ChangeDayTrack::Start: the day (+0x18; out of 0..4: 0)
uint8_t __fastcall ChangeDayTrack_Start(uint8_t* self, re::Edx, uint8_t*) {
    int32_t day = At<int32_t>(self, 0x18);
    currentDay = day;
    if (day > 4 || day < 0) {
        currentDay = 0;
    }
    return 1;
}

// 0x61D4A0 (thiscall): SeekEntityPositionTrack's deleting destructor
uint8_t* __fastcall SeekEntityPositionTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtSeekEntityPositionTrack;
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x34);
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
    }
    NameKey_Destruct(self + 0x30);
    TrackBase_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x61D4F0 (cdecl): the entity's move animation's (slot 1, else 0; of the move type)
// time for walking the flat distance to target at the step rate (-1: none; a mapped
// time at its end held at least; ~0 is 0)
float __cdecl SeekEntityPosition_MoveTime(uint8_t* entity, const float* target, float least) {
    const float* p = Pos(entity);
    float dx = target[0] - p[0];
    float dy = target[1] - p[1];
    float distance = std::sqrt(std::fabs(dx * dx + dy * dy));
    uint8_t* anim = nullptr;
    uint8_t* a = Blend_GetAnimAt(At<uint8_t*>(entity, 0x114), 1);
    if (a && vc::Call<uint8_t>(a, 0, int(AnimSlot_TypeId()))) {
        anim = a;
    } else {
        uint8_t* b = Blend_GetAnimAt(At<uint8_t*>(entity, 0x114), 0);
        if (b) {
            anim = vc::Call<uint8_t>(b, 0, int(AnimSlot_TypeId())) ? b : nullptr;
        }
    }
    float step = combatStepRate * timeStep;
    if (!anim || !At<uint8_t*>(anim, 0x4C)) {
        return -1.0f;
    }
    if (std::fabs(step) <= 1e-5f) {
        return -1.0f;
    }
    float t = RangeMap_Get(At<uint8_t*>(anim, 0x4C), distance / step);
    if (At<float>(At<uint8_t*>(anim, 0x4C), 0x24) == t && !(least < t)) {
        t = least;
    }
    if (std::fabs(t) <= 1e-5f) {
        return 0.0f;
    }
    return t;
}

// a CMatrix with its attached transform (+0x40)
struct AttachedMatrix {
    float m[16];
    re::Ptr32<void> attach;
};
static_assert(sizeof(AttachedMatrix) == 0x44);

// 0x61D690 (thiscall): SeekEntityPositionTrack's goal transform: by mode +0x1C: the
// ped's own (0xB), its vehicle's (0xE), else the entity's (+0x34, its frame's or just
// its position); getting on (3) a vehicle: beside its door (the side named "GetOnRHS" by
// +0x30) facing along it; else its node +0x38
void __fastcall SeekEntityPositionTrack_GetGoal(uint8_t* self, re::Edx, uint8_t* out) {
    AttachedMatrix m;
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    bool done = false;
    if (ped) {
        if (At<int32_t>(self, 0x1C) == 0xB) {
            const uint8_t* t = Ped_GetMatrix(ped);
            for (int i = 0; i < 0x34; i++) {
                out[i] = t[i];
            }
            done = true;
        } else if (At<int32_t>(self, 0x1C) == 0xE) {
            uint8_t* vehicle = At<uint8_t*>(ped, 0x1D24);
            if (vehicle) {
                Matrix_Copy(&m, Placeable_GetMatrix(vehicle));
                Matrix_ToNiTransform(&m, out);
            }
            done = true;
        }
    }
    if (!done) {
        uint8_t* entity = At<uint8_t*>(self, 0x34);
        uint8_t* object = At<uint8_t*>(entity, 0x18);
        if (object && At<uint8_t*>(object, 4) && Frame_GetLTM(At<uint8_t*>(object, 4))) {
            object = At<uint8_t*>(At<uint8_t*>(self, 0x34), 0x18);
            // (null when there's no frame: the game copies from address 0)
            const uint8_t* ltm = object && At<uint8_t*>(object, 4) ? Frame_GetLTM(At<uint8_t*>(object, 4)) : nullptr;
            for (int i = 0; i < 0x34; i++) {
                out[i] = ltm[i];
            }
        } else if (At<uint8_t*>(self, 0x34)) {
            const float* p = Pos(At<uint8_t*>(self, 0x34));
            At<float>(out, 0x24) = p[0];
            At<float>(out, 0x28) = p[1];
            At<float>(out, 0x2C) = p[2];
        }
    }
    uint8_t* entity = At<uint8_t*>(self, 0x34);
    if (At<int32_t>(self, 0x1C) == 3 && vc::Call<uint8_t>(entity, 0x14)) {
        if (At<uint32_t>(self, 0x38) != 0 &&
            Entity_GetNodeTransform(At<uint8_t*>(self, 0x34), At<uint32_t>(self, 0x38), out, 1)) {
            return;
        }
        Matrix_Copy(&m, Entity_GetMatrixPtr(At<uint8_t*>(self, 0x34)));
        m.attach = out;
        Matrix_UpdateAttached(&m);
        if (!(getOnRhsGuard & 1)) {
            getOnRhsGuard |= 1;
            NameKey_ConstructFrom(getOnRhs, S(0x00932278));
            AtExit(reinterpret_cast<void*>(0x008F5D40));
        }
        uint8_t* seats = entity + 0x1C4;
        V3 door;
        float angle;
        int side = At<uint32_t>(self, 0x30) == *getOnRhs ? 1 : 0;
        uint8_t* d = Seats_GetDoor(seats, side);
        door = {At<float>(d, 0x24), At<float>(d, 0x28), At<float>(d, 0x2C)};
        if (VectorEqual(&door, zeroVec)) {
            d = Seats_GetDoor(seats, side);
            door = {At<float>(d, 0x18), At<float>(d, 0x1C), At<float>(d, 0x20)};
        }
        angle = side ? 1.57079637f : 4.71238899f;
        V3 w;
        V3* at = TransformPoint(&w, &m, &door);
        V3 pos = *at;
        Matrix_SetRotateZ(&m, angle);
        // (the position's fourth word: whatever the stack held)
        m.m[12] = pos.x;
        m.m[13] = pos.y;
        m.m[14] = pos.z;
        Matrix_UpdateAttached(&m);
        return;
    }
    if (At<uint32_t>(self, 0x38) != 0) {
        Entity_GetNodeTransform(At<uint8_t*>(self, 0x34), At<uint32_t>(self, 0x38), out, 1);
    }
}

// 0x61D970 (thiscall): SeekEntityPositionTrack::End: the player's seek flag cleared; the
// ped's +0x146C / +0x1514 when +0x41
uint8_t __fastcall SeekEntityPositionTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x3C));
    if (ped == playerPed || At<uint8_t*>(self, 0x34) == playerPed) {
        playerSeeking = 0;
    }
    if (self[0x41] != 0) {
        At<int32_t>(ped, 0x146C) = 0;
        At<int32_t>(ped, 0x1514) = 0;
    }
    self[0x41] = 0;
    return 1;
}

// 0x61D9B0 (thiscall): IdleTrack::Update: while there's a ped
uint8_t __fastcall IdleTrack_Update(uint8_t* self, re::Edx, float) {
    return OwnerPed(At<uint8_t*>(self, 0x18)) != nullptr ? 1 : 0;
}
// 0x61D9C0 (thiscall): ControllerTrack::Start: its pad's lock (+0x1C) taken by the ped;
// the ped's heading kept (+0x3C)
uint8_t __fastcall ControllerTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    if (self[0x1C] != 0) {
        int32_t pad = At<int32_t>(self, 0x18);
        if (pad == 0) {
            controllerLockA = At<int32_t>(owner, 8);
        } else if (pad == 1) {
            controllerLockB = At<int32_t>(owner, 8);
        }
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    uint8_t* matrix = At<uint8_t*>(ped, 0x14);
    if (matrix) {
        At<float>(self, 0x3C) = trk::CrtAtan2(-At<float>(matrix, 0x10), At<float>(matrix, 0x14));
        return 1;
    }
    At<float>(self, 0x3C) = At<float>(ped, 0x10);
    return 1;
}
// 0x61DA40 (thiscall): IdleTrack::Start: a stopped bike's rider (state 1 or 0xB) idles
// 6..9 s; the ped's input cleared
uint8_t __fastcall IdleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 1;
    }
    if (ped[0x1570] == 1 && Ped_IsInVehicle(ped)) {
        uint8_t* bike = At<uint8_t*>(ped, 0x1554);
        if (bike && (bike[0x2DA] == 1 || bike[0x2DA] == 0xB) && 0.0f == At<float>(bike, 0x294)) {
            At<uint8_t*>(ped, 0x1554)[0x2DA] = 1;
            uint8_t* b = At<uint8_t*>(ped, 0x1554);
            b[0x2E4] = uint8_t(int32_t(RandomFloat(6.0f, 9.0f)));
            At<uint8_t*>(ped, 0x1554)[0x2D9] = 0;
        }
    }
    ActionInputs_Clear(At<uint8_t*>(ped, 0x2E0));
    return 1;
}
// 0x61DAF0 (thiscall): ScriptedAmbientTrack::End: the ped's scripted ambient (unless in
// state 3) stopped
uint8_t __fastcall ScriptedAmbientTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x18));
    if (ped && At<uint8_t*>(ped, 0x1D24) && At<int32_t>(At<uint8_t*>(ped, 0x1D24), 0x70) != 3) {
        ScriptedAmbient_Stop(At<uint8_t*>(ped, 0x1D24), ped);
    }
    return 1;
}
// 0x61DB20 (thiscall): WrestleTrack::Start: the ped wrestling (+0x1340; eax is left 1)
uint8_t __fastcall WrestleTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x40) = owner;
    At<int32_t>(self, 0x50) = 0;
    At<int32_t>(OwnerPed(owner), 0x1340) = 1;
    return 1;
}

// 0x8F5D40 (exit): the "GetOnRHS" key's destructor
void __cdecl GetOnRhs_Exit() {
    TR_NameKey_Destruct(getOnRhs);
}

} // namespace

#define TRREWARD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

TRREWARD_REVERSED("RewardPlayerTrack::Start", 0x0061C760, &RewardPlayerTrack_Start);
TRREWARD_REVERSED("RewardTracks::GetOnRhsExit", 0x008F5D40, &GetOnRhs_Exit);
TRREWARD_REVERSED("RewardPlayerTrack::Update", 0x0061D1A0, &RewardPlayerTrack_Update);
TRREWARD_REVERSED("GivePlayerRewardTrack::Start", 0x0061D1F0, &GivePlayerRewardTrack_Start);
TRREWARD_REVERSED("AITrack::InitFields_61D400", 0x0061D400, &InitFields_61D400);
TRREWARD_REVERSED("AITrack::InitFields_61D410", 0x0061D410, &InitFields_61D410);
TRREWARD_REVERSED("SetTargetAsGrappleTargetTrack::Reset", 0x0061D420, &SetTargetAsGrappleTargetTrack_Reset);
TRREWARD_REVERSED("SetTargetAsGrappleTargetTrack::SetNode", 0x0061D440, &SetTargetAsGrappleTargetTrack_SetNode);
TRREWARD_REVERSED("AITrack::InitFields_61D470", 0x0061D470, &InitFields_61D470);
TRREWARD_REVERSED("ChangeDayTrack::Start", 0x0061D480, &ChangeDayTrack_Start);
TRREWARD_REVERSED("SeekEntityPositionTrack::DeletingDestruct", 0x0061D4A0, &SeekEntityPositionTrack_DeletingDestruct);
TRREWARD_REVERSED("SeekEntityPositionTrack::MoveTime_61D4F0", 0x0061D4F0, &SeekEntityPosition_MoveTime);
TRREWARD_REVERSED("SeekEntityPositionTrack::GetGoal", 0x0061D690, &SeekEntityPositionTrack_GetGoal);
TRREWARD_REVERSED("SeekEntityPositionTrack::End", 0x0061D970, &SeekEntityPositionTrack_End);
TRREWARD_REVERSED("IdleTrack::Update", 0x0061D9B0, &IdleTrack_Update);
TRREWARD_REVERSED("ControllerTrack::Start", 0x0061D9C0, &ControllerTrack_Start);
TRREWARD_REVERSED("IdleTrack::Start", 0x0061DA40, &IdleTrack_Start);
TRREWARD_REVERSED("ScriptedAmbientTrack::End", 0x0061DAF0, &ScriptedAmbientTrack_End);
TRREWARD_REVERSED("WrestleTrack::Start", 0x0061DB20, &WrestleTrack_Start);
