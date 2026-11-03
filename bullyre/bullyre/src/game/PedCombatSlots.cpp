// A ped's combat slots (CPed +0x668): who attacks whom. Every ped keeps three lists of
// the peds attacking it (by range: close, middle, far; at most +8 / +0xC / +0x10 of
// them, 10 places each), and four lists of slots for the peds it considers attacking
// itself, filled from the peds around it (CPed +0x3C0 count, +0x3C4 the peds). Also the
// cached "should I wait" answers of the last 8 targets (+0x35C). 0x486260 .. 0x487D40.
//
// +0 the ped, +4 the list it's in on its target (4: none), +8 / +0xC / +0x10 the lists'
// sizes, +0x14 / +0x40 / +0x6C the three attacker lists (a count and 10 registered
// peds), +0x98..0x9A whether each list may be joined, +0x9C an entity (registered) with
// a timer at +0xA0, +0xBC / +0xC0 slots held until then, +0xC4 the slot lists' counts,
// +0xD4 the four slot lists (0x94 each: 6 slots of 0x18 and a count at +0x90), +0x324
// the chosen slot, +0x340 / +0x344 / +0x348 a ped remembered for 15 seconds, +0x354 a
// registered pointer with a flag at +0x358.
//
// A slot (0x18): +0 the ped, +4 the one it last had, +8 a group (a ped's +0x714) to pick
// from (0: any), +0xC a flag, +0x10 its list, +0x14 when it was taken.

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct V3 {
    float x, y, z;
};

using re::At;

V3* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<V3*>(matrix + 0x30) : &At<V3>(entity, 4);
}

bool IsPed(uint8_t* entity) {
    return (At<uint8_t>(entity, 0x108) & 7) == 3;
}

// The ped types (+0x1310) that count as students (0, 7, 8)
bool IsStudentType(int type) {
    return type == 0 || type == 8 || type == 7;
}

GAME_VAR(uint32_t, PCS_timeInMs, 0x00C1A9B4);          // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint8_t*, PCS_aiButes, 0x020C889C);           // the AI butes (+0x18.. the combat numbers)
inline void* const PCS_nodeTaunt = reinterpret_cast<void*>(0x01B652C4);
inline void* const PCS_nodeWait = reinterpret_cast<void*>(0x01B654F0);

GAME_FN(void, __cdecl, PCS_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, PCS_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __thiscall, PCS_RefSet, 0x005D7F10, void* ref, void* entity);
inline int __cdecl PCS_Rand() { return crt::Rand(); }
GAME_FN(uint8_t*, __cdecl, PCS_GetWeaponInfo, 0x0051B450, int model);
GAME_FN(void, __thiscall, PCS_WeaponInfo7282A0, 0x007282A0, void* info);
GAME_FN(int, __thiscall, PCS_InventoryGetAmmo, 0x0045CF40, void* inventory, int model);
GAME_FN(void, __thiscall, PCS_InventoryReset, 0x007DB6B0, void* inventory);
GAME_FN(uint8_t*, __thiscall, PCS_InventoryNext, 0x0045CDB0, void* inventory);
GAME_FN(float, __cdecl, PCS_InRange41A4D0, 0x0041A4D0, const V3* a, const V3* b, float range);
GAME_FN(float, __cdecl, PCS_Distance2D, 0x00470110, const V3* a, const V3* b);
GAME_FN(float, __cdecl, PCS_Distance41A950, 0x0041A950, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, PCS_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
GAME_FN(float, __thiscall, PCS_Normalise, 0x00414E10, V3* v);
GAME_FN(V3*, __thiscall, PCS_GetForward, 0x004154A0, void* entity, V3* out);
GAME_FN(uint8_t, __thiscall, PCS_Obj48D470, 0x0048D470, void* self, void* ped);
GAME_FN(uint8_t, __thiscall, PCS_PedHasWeapon, 0x00474BF0, void* ped);
GAME_FN(float, __thiscall, PCS_Weapon4D96B0, 0x004D96B0, void* weapon);
GAME_FN(uint8_t*, __thiscall, PCS_PedGetTarget, 0x00497100, void* ped);
GAME_FN(void, __thiscall, PCS_PedSetTarget, 0x00498B10, void* ped, void* target);
GAME_FN(uint8_t, __thiscall, PCS_Ped4473E0, 0x004473E0, void* ped);
GAME_FN(void, __thiscall, PCS_PedSetStat, 0x004770F0, void* ped, int stat, int value);
GAME_FN(void, __thiscall, PCS_Timer45BD70, 0x0045BD70, void* timer, float seconds);
GAME_FN(void, __thiscall, PCS_Timer45BD10, 0x0045BD10, void* timer);
GAME_FN(uint8_t, __thiscall, PCS_Timer45BDD0, 0x0045BDD0, void* timer);
GAME_FN(uint8_t, __thiscall, PCS_Tether499840, 0x00499840, void* tether, const V3* pos, float);
GAME_FN(void*, __thiscall, PCS_Tether499600, 0x00499600, void* tetherTarget);
GAME_FN(uint8_t, __thiscall, PCS_RouteIsValid, 0x005E7AF0, void* route);
GAME_FN(uint8_t, __cdecl, PCS_IsPointClear5EAE60, 0x005EAE60, const V3* pos, float radius, uintptr_t);
GAME_FN(uint8_t, __thiscall, PCS_ControllerIsPlaying, 0x005F4800, void* controller, void* node, int, int);
// (this file's, through the game's address)
GAME_FN(int, __thiscall, PCS_GetAmmo, 0x00486280, uint8_t* self, int model);
GAME_FN(void, __thiscall, PCS_MemoryTick, 0x00486340, uint8_t* memory);
GAME_FN(uint8_t, __thiscall, PCS_MemoryFind, 0x004863B0, uint8_t* memory, const void* key, uint8_t* out);
GAME_FN(uint8_t, __thiscall, PCS_MemoryAdd, 0x00486410, uint8_t* memory, const void* key, const uint8_t* value);
GAME_FN(uint8_t, __thiscall, PCS_ShouldDisplace, 0x00486510, uint8_t* self, uint8_t* target, int list, uint8_t* other);
GAME_FN(uint8_t, __thiscall, PCS_IsThreat, 0x004866F0, uint8_t* self, uint8_t* ped);
GAME_FN(void, __thiscall, PCS_InitList, 0x00486830, uint8_t* self, int list, int count);
GAME_FN(uint8_t, __thiscall, PCS_IsInSlots, 0x00486930, uint8_t* self, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, PCS_FindSlot, 0x004869A0, uint8_t* self, uint8_t* ped);
GAME_FN(void, __thiscall, PCS_RemoveFromSlots, 0x00486A30, uint8_t* self, uint8_t* ped);
GAME_FN(void, __thiscall, PCS_ReleaseSlots, 0x00486AB0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PCS_HasUsableWeapon, 0x00486BE0, uint8_t* self);
GAME_FN(void, __thiscall, PCS_RefListAdd, 0x00486DC0, uint8_t* list, uint8_t* ped);
GAME_FN(void, __thiscall, PCS_ResetSlots, 0x00486E20, uint8_t* self);
GAME_FN(void, __thiscall, PCS_UpdateSlots, 0x00486F90, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PCS_ShouldWait, 0x004876B0, uint8_t* self, uint8_t* target, float, float maxHeight);
GAME_FN(void, __thiscall, PCS_LeaveTarget, 0x00487A30, uint8_t* self, uint8_t* target, int keepList);
GAME_FN(uint8_t, __thiscall, PCS_JoinTarget, 0x00487BC0, uint8_t* self, uint8_t* target, int list, int dontCommit);

uint8_t* Combat(uint8_t* ped) {
    return ped + 0x668;
}

uint8_t* SlotList(uint8_t* self, int list) {
    return self + 0xD4 + list * 0x94;
}

int& SlotCount(uint8_t* self, int list) {
    return At<int32_t>(self, 0xC4 + list * 4);
}

// A registered-pointer list: +0 the count, +4 the entries
bool RefListContains(uint8_t* list, uint8_t* ped) {
    int count = At<int32_t>(list, 0);
    for (int i = 0; i < count; i++) {
        if (At<uint8_t*>(list, 4 + i * 4) == ped) {
            return true;
        }
    }
    return false;
}

// (inlined): a slot's registered pointer set
void SetRef(re::Ptr32<uint8_t>* ref, uint8_t* entity) {
    if (entity != *ref) {
        if (*ref) {
            PCS_CleanUpOldReference(*ref, ref);
        }
        *ref = entity;
        if (entity) {
            PCS_RegisterReference(entity, ref);
        }
    }
}

// The ped's weapon in hand is unloaded (0x4D96B0 zero), or it has none
bool WeaponIdle(uint8_t* ped) {
    return !PCS_PedHasWeapon(ped) || PCS_Weapon4D96B0(At<void*>(ped, 0x1D0)) == 0.0f;
}

// The first empty slot of a list (nullptr: none)
uint8_t* FreeSlot(uint8_t* self, int list) {
    for (int i = 0; i < SlotCount(self, list); i++) {
        uint8_t* slot = SlotList(self, list) + i * 0x18;
        if (At<uint8_t*>(slot, 0) == nullptr) {
            return slot;
        }
    }
    return nullptr;
}

void TakeSlot(uint8_t* slot, uint8_t* ped) {
    At<uint8_t*>(slot, 0) = ped;
    At<uint8_t*>(slot, 4) = ped;
    At<uint32_t>(slot, 0x14) = PCS_timeInMs;
}

} // namespace

namespace c07pcs {

// 0x486260: whether a list may be joined
void __fastcall SetListOpen(uint8_t* self, re::Edx, int list, uint8_t open) {
    At<uint8_t>(self, 0x98 + list) = open;
}

// 0x486280: the ammo for a weapon model (its ammo model's, when it has one; 1 for the
// one in hand without ammo)
int __fastcall GetAmmo(uint8_t* self, re::Edx, int model) {
    if (model < 1) {
        return 0;
    }
    uint8_t* info = PCS_GetWeaponInfo(model);
    if (!info) {
        return 0;
    }
    PCS_WeaponInfo7282A0(info);
    int ammoModel = At<int32_t>(info, 0x84);
    uint8_t* ped = At<uint8_t*>(self, 0);
    if (ammoModel > 0) {
        return PCS_InventoryGetAmmo(At<void*>(ped, 0x1C4), ammoModel);
    }
    int ammo = PCS_InventoryGetAmmo(At<void*>(ped, 0x1C4), model);
    if (ammo == 0 && At<int16_t>(At<uint8_t*>(At<uint8_t*>(self, 0), 0x1D0), 0x10E) == model) {
        ammo = 1;
    }
    return ammo;
}

// 0x486300
uint8_t __fastcall HasRemembered(uint8_t* self, re::Edx) {
    return At<uint8_t>(self, 0x344);
}

// 0x486310: the remembered ped forgotten
void __fastcall Forget(uint8_t* self, re::Edx, uint8_t* ped) {
    if (ped == At<uint8_t*>(self, 0x340) && At<uint8_t>(self, 0x344)) {
        At<uint8_t>(self, 0x344) = 0;
        At<uint8_t*>(self, 0x340) = nullptr;
    }
}

// 0x486340 (the answer memory): a frame older (entries count down to free)
void __fastcall MemoryTick(uint8_t* self, re::Edx) {
    for (int i = 0; i < 8; i++) {
        int32_t& age = At<int32_t>(self, i * 0x14);
        if (age > 0) {
            age--;
        }
    }
}

// 0x4863B0: the answer remembered for a key (a dword and two floats)
uint8_t __fastcall MemoryFind(uint8_t* self, re::Edx, const uint32_t* key, uint8_t* out) {
    const float* keyF = reinterpret_cast<const float*>(key);
    for (int i = 0; i < 8; i++) {
        uint8_t* entry = self + i * 0x14;
        if (At<int32_t>(entry, 0) > 0 && At<uint32_t>(entry, 4) == key[0] && keyF[1] == At<float>(entry, 8) &&
            keyF[2] == At<float>(entry, 0xC)) {
            *out = At<uint8_t>(entry, 0x10);
            return 1;
        }
    }
    return 0;
}

// 0x486410: an answer remembered (in a free entry or the key's own) for 8..14 frames
uint8_t __fastcall MemoryAdd(uint8_t* self, re::Edx, const uint32_t* key, const uint8_t* value) {
    const float* keyF = reinterpret_cast<const float*>(key);
    for (int i = 0; i < 8; i++) {
        uint8_t* entry = self + i * 0x14;
        if (At<int32_t>(entry, 0) <= 0 ||
            (At<uint32_t>(entry, 4) == key[0] && keyF[1] == At<float>(entry, 8) && keyF[2] == At<float>(entry, 0xC))) {
            At<int32_t>(entry, 0) = PCS_Rand() * 7 / 0x7FFF + 8;
            At<uint32_t>(entry, 4) = key[0];
            At<float>(entry, 8) = keyF[1];
            At<float>(entry, 0xC) = keyF[2];
            At<uint8_t>(entry, 0x10) = *value;
            return 1;
        }
    }
    return 0;
}

// 0x4864B0 (cdecl): the entity if it's a vehicle without +0x46C
uint8_t* __cdecl AsFreeVehicle(uint8_t* entity) {
    if (entity && (At<uint8_t>(entity, 0x108) & 7) == 2) {
        return At<int32_t>(entity, 0x46C) != 0 ? nullptr : entity;
    }
    return nullptr;
}

// 0x4864E0: attacking, or any of its own attacker lists has someone
uint8_t __fastcall HasAttackers(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 4) != 4) {
        return 1;
    }
    for (int list = 0; list < 3; list++) {
        if (At<int32_t>(self, 0x14 + list * 0x2C) > 0) {
            return 1;
        }
    }
    return 0;
}

// 0x486510: whether this ped should take `other`'s place in `target`'s list (other's
// the target's own target: no; a student against a non-student: yes; else by range and
// who's nearer)
uint8_t __fastcall ShouldDisplace(uint8_t* self, re::Edx, uint8_t* target, int list, uint8_t* other) {
    if (At<uint8_t>(target, 0x9AC) && other == At<uint8_t*>(target, 0x9A8)) {
        return 0;
    }
    uint8_t* ped = At<uint8_t*>(self, 0);
    if (IsStudentType(At<int32_t>(ped, 0x1310)) && !IsStudentType(At<int32_t>(other, 0x1310))) {
        return 1;
    }
    float range = 0.0f;
    switch (list) {
    case 0:
        range = At<float>(PCS_aiButes, 0x2C);
        break;
    case 1:
        range = At<float>(PCS_aiButes, 0x30);
        break;
    case 2:
        range = At<float>(PCS_aiButes, 0x34);
        break;
    }
    bool selfOutside = PCS_InRange41A4D0(EntityPos(target), EntityPos(ped), range) < 0.0f;
    if (PCS_InRange41A4D0(EntityPos(target), EntityPos(other), range) < 0.0f) {
        return 0;
    }
    if (selfOutside) {
        return 1;
    }
    float mine = PCS_Distance2D(EntityPos(target), EntityPos(At<uint8_t*>(self, 0)));
    double limit = static_cast<double>(mine) + 1.0;
    double theirs = PCS_Distance2D(EntityPos(target), EntityPos(other));
    return theirs > limit ? 1 : 0;
}

// 0x4866C0: `ped` is the entity at +0x9C (or there's none)
uint8_t __fastcall IsCurrentEntity(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    uint8_t* current = At<uint8_t*>(self, 0x9C);
    if (!current) {
        return 1;
    }
    return current == ped ? 1 : 0;
}

// 0x4866F0: `ped` is a threat: near enough (butes +0x54), facing this one (+0x58), and
// either of the two sees the other (0x48D470 of +0xA64)
uint8_t __fastcall IsThreat(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    uint8_t* owner = At<uint8_t*>(self, 0);
    if (At<int32_t>(owner, 0x1380) != 0 || At<int32_t>(ped, 0x139C) != 0) {
        return 0;
    }
    V3 toOwner;
    PCS_VectorSub(&toOwner, EntityPos(owner), EntityPos(ped));
    toOwner.z = 0.0f;
    float distance = static_cast<float>(std::sqrt(std::fabs(toOwner.x * toOwner.x + toOwner.y * toOwner.y)));
    if (At<float>(PCS_aiButes, 0x54) < distance) {
        return 0;
    }
    PCS_Normalise(&toOwner);
    V3 forward;
    PCS_GetForward(ped, &forward);
    forward.z = 0.0f;
    PCS_Normalise(&forward);
    float facing = forward.y * toOwner.y + forward.x * toOwner.x + forward.z * toOwner.z;
    float limit = At<float>(PCS_aiButes, 0x58);
    if (limit > facing) {
        return 0;
    }
    if (PCS_Obj48D470(At<uint8_t*>(self, 0) + 0xA64, ped) || PCS_Obj48D470(ped + 0xA64, At<uint8_t*>(self, 0))) {
        return 1;
    }
    return 0;
}

// 0x486830: a slot list set up with `count` empty slots
void __fastcall InitList(uint8_t* self, re::Edx, int list, int count) {
    uint8_t* slots = SlotList(self, list);
    At<int32_t>(slots, 0x90) = 0;
    SlotCount(self, list) = count;
    for (int i = 0; i < SlotCount(self, list); i++) {
        uint8_t* slot = slots + At<int32_t>(slots, 0x90) * 0x18;
        At<uint32_t>(slot, 0) = 0;
        At<uint32_t>(slot, 4) = 0;
        At<uint32_t>(slot, 8) = 0;
        // (the game writes the flag's byte 0 and 3 bytes of uninitialised stack)
        At<uint32_t>(slot, 0xC) = 0;
        At<int32_t>(slot, 0x10) = list;
        At<uint32_t>(slot, 0x14) = 0;
        At<int32_t>(slots, 0x90)++;
    }
}

// 0x4868B0: `ped` holds a slot without its flag
uint8_t __fastcall HoldsUnflaggedSlot(uint8_t* self, re::Edx, uint8_t* ped) {
    for (int list = 0; list < 4; list++) {
        for (int i = 0; i < SlotCount(self, list); i++) {
            uint8_t* slot = SlotList(self, list) + i * 0x18;
            if (At<uint8_t*>(slot, 0) == ped && At<uint8_t>(slot, 0xC) == 0) {
                return 1;
            }
        }
    }
    if (At<uint8_t*>(self, 0x324) == ped && At<uint8_t>(self, 0x330) == 0) {
        return 1;
    }
    return 0;
}

// 0x486930: `ped` holds a slot
uint8_t __fastcall IsInSlots(uint8_t* self, re::Edx, uint8_t* ped) {
    for (int list = 0; list < 4; list++) {
        for (int i = 0; i < SlotCount(self, list); i++) {
            if (At<uint8_t*>(SlotList(self, list) + i * 0x18, 0) == ped) {
                return 1;
            }
        }
    }
    return At<uint8_t*>(self, 0x324) == ped ? 1 : 0;
}

// 0x4869A0: the slot `ped` holds (nullptr: none)
uint8_t* __fastcall FindSlot(uint8_t* self, re::Edx, uint8_t* ped) {
    for (int list = 0; list < 4; list++) {
        for (int i = 0; i < SlotCount(self, list); i++) {
            uint8_t* slot = SlotList(self, list) + i * 0x18;
            if (At<uint8_t*>(slot, 0) == ped) {
                return slot;
            }
        }
    }
    return At<uint8_t*>(self, 0x324) == ped ? self + 0x324 : nullptr;
}

// 0x486A30: `ped`'s slots emptied
void __fastcall RemoveFromSlots(uint8_t* self, re::Edx, uint8_t* ped) {
    for (int list = 0; list < 4; list++) {
        for (int i = 0; i < SlotCount(self, list); i++) {
            uint8_t* slot = SlotList(self, list) + i * 0x18;
            if (At<uint8_t*>(slot, 0) == ped) {
                At<uint32_t>(slot, 0) = 0;
                At<uint32_t>(slot, 4) = 0;
                At<uint32_t>(slot, 8) = 0;
                At<uint32_t>(slot, 0x14) = 0;
            }
        }
    }
    if (At<uint8_t*>(self, 0x324) == ped) {
        At<uint32_t>(self, 0x324) = 0;
        At<uint32_t>(self, 0x328) = 0;
        At<uint32_t>(self, 0x32C) = 0;
        At<uint32_t>(self, 0x338) = 0;
    }
}

// 0x486AB0: every slot's flag cleared
void __fastcall ReleaseSlots(uint8_t* self, re::Edx) {
    for (int list = 0; list < 4; list++) {
        for (int i = 0; i < SlotCount(self, list); i++) {
            At<uint8_t>(SlotList(self, list) + i * 0x18, 0xC) = 0;
        }
    }
    At<uint8_t>(self, 0x330) = 0;
    At<uint8_t>(self, 0xBC) = 0;
}

// 0x486B10: every slot's flag set, until `ms` from now (0: the butes' +0x4C seconds, 5
// when not positive)
void __fastcall HoldSlots(uint8_t* self, re::Edx, int ms) {
    for (int list = 0; list < 4; list++) {
        for (int i = 0; i < SlotCount(self, list); i++) {
            At<uint8_t>(SlotList(self, list) + i * 0x18, 0xC) = 1;
        }
    }
    At<uint8_t>(self, 0x330) = 1;
    At<uint8_t>(self, 0xBC) = 1;
    if (ms != 0) {
        At<uint32_t>(self, 0xC0) = PCS_timeInMs + ms;
        return;
    }
    float seconds = At<float>(PCS_aiButes, 0x4C);
    if (!(0.0f < seconds)) {
        seconds = 5.0f;
    }
    At<uint32_t>(self, 0xC0) = PCS_timeInMs - static_cast<uint32_t>(static_cast<int64_t>(seconds * -1000.0));
}

// 0x486BE0: the weapon in hand is loaded (0x4D96B0) and has ammo
uint8_t __fastcall HasUsableWeapon(uint8_t* self, re::Edx) {
    if (PCS_PedHasWeapon(At<void*>(self, 0))) {
        uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(self, 0), 0x1D0);
        if (PCS_Weapon4D96B0(weapon) != 0.0f && PCS_GetAmmo(self, At<int16_t>(weapon, 0x10E)) > 0) {
            return 1;
        }
    }
    return 0;
}

// 0x486C30: that, or any weapon in the inventory flagged +0x68 and not +0x69 with ammo
uint8_t __fastcall HasAnyUsableWeapon(uint8_t* self, re::Edx) {
    if (PCS_PedHasWeapon(At<void*>(self, 0))) {
        uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(self, 0), 0x1D0);
        if (PCS_Weapon4D96B0(weapon) != 0.0f && PCS_GetAmmo(self, At<int16_t>(weapon, 0x10E)) > 0) {
            return 1;
        }
    }
    PCS_InventoryReset(At<void*>(At<uint8_t*>(self, 0), 0x1C4));
    for (;;) {
        uint8_t* item = PCS_InventoryNext(At<void*>(At<uint8_t*>(self, 0), 0x1C4));
        if (!item) {
            return 0;
        }
        int16_t model = At<int16_t>(item, 2);
        if (model < 1) {
            continue;
        }
        uint8_t* info = PCS_GetWeaponInfo(model);
        if (!info || !At<uint8_t>(info, 0x68) || At<uint8_t>(info, 0x69)) {
            continue;
        }
        if (PCS_GetAmmo(self, At<int16_t>(item, 2)) > 0) {
            return 1;
        }
    }
}

// 0x486CE0: the first inventory weapon flagged +0x68 with ammo chosen (the
// intelligence's +0x18)
uint8_t __fastcall ChooseWeapon(uint8_t* self, re::Edx) {
    PCS_InventoryReset(At<void*>(At<uint8_t*>(self, 0), 0x1C4));
    for (;;) {
        uint8_t* item = PCS_InventoryNext(At<void*>(At<uint8_t*>(self, 0), 0x1C4));
        if (!item) {
            return 0;
        }
        int16_t model = At<int16_t>(item, 2);
        if (model < 1) {
            continue;
        }
        uint8_t* info = PCS_GetWeaponInfo(model);
        if (!info || !At<uint8_t>(info, 0x68)) {
            continue;
        }
        if (PCS_GetAmmo(self, At<int16_t>(item, 2)) < 1) {
            continue;
        }
        At<int32_t>(At<uint8_t*>(At<uint8_t*>(self, 0), 0x2E0), 0x18) = At<int16_t>(item, 2);
        return 1;
    }
}

// 0x486D50: the target's +0x9A8 mark on this ped cleared
void __fastcall ClearTargetMark(uint8_t* self, re::Edx) {
    uint8_t* target = PCS_PedGetTarget(At<void*>(self, 0));
    if (target && IsPed(target) && At<uint8_t*>(self, 0) == At<uint8_t*>(target, 0x9A8) && At<uint8_t>(target, 0x9AC)) {
        At<uint8_t>(target, 0x9AC) = 0;
        At<uint8_t*>(target, 0x9A8) = nullptr;
    }
}

// 0x486D90: a ped remembered for 15 seconds
void __fastcall Remember(uint8_t* self, re::Edx, uint8_t* ped) {
    At<uint8_t>(self, 0x344) = 1;
    At<uint8_t*>(self, 0x340) = ped;
    At<uint32_t>(self, 0x348) = PCS_timeInMs + 15000;
}

// 0x486DC0 (a registered-pointer list): `ped` in the first empty place, or appended
void __fastcall RefListAdd(uint8_t* self, re::Edx, uint8_t* ped) {
    int count = At<int32_t>(self, 0);
    int index = 0;
    for (; index < count; index++) {
        if (At<uint8_t*>(self, 4 + index * 4) == nullptr) {
            break;
        }
    }
    if (index >= count) {
        index = count;
        At<int32_t>(self, 0) = count + 1;
    }
    SetRef(&At<uint8_t*>(self, 4 + index * 4), ped);
}

// 0x486E20: the slot lists set up from the butes, the chosen slot and memory cleared
void __fastcall ResetSlots(uint8_t* self, re::Edx) {
    PCS_InitList(self, 0, At<int32_t>(PCS_aiButes, 0x3C));
    PCS_InitList(self, 1, At<int32_t>(PCS_aiButes, 0x40));
    PCS_InitList(self, 2, At<int32_t>(PCS_aiButes, 0x44));
    PCS_InitList(self, 3, At<int32_t>(PCS_aiButes, 0x48));
    At<uint32_t>(self, 0x324) = 0;
    At<uint32_t>(self, 0x328) = 0;
    At<uint32_t>(self, 0x32C) = 0;
    At<uint8_t>(self, 0x330) = 0;
    At<uint32_t>(self, 0x338) = 0;
    At<uint32_t>(self, 0x334) = 0;
    At<uint8_t>(self, 0x344) = 0;
    At<uint32_t>(self, 0x340) = 0;
    At<uint8_t>(self, 0x33C) = 0;
}

// 0x486EA0: reset (no target list, the list sizes from the butes, all lists open)
void __fastcall Reset(uint8_t* self, re::Edx) {
    PCS_ResetSlots(self);
    At<int32_t>(self, 4) = 4;
    At<int32_t>(self, 8) = At<int32_t>(PCS_aiButes, 0x18);
    At<int32_t>(self, 0xC) = At<int32_t>(PCS_aiButes, 0x1C);
    At<int32_t>(self, 0x10) = At<int32_t>(PCS_aiButes, 0x20);
    At<uint16_t>(self, 0x98) = 0x0101;
    At<uint8_t>(self, 0x9A) = 1;
    At<uint8_t>(self, 0xB0) = 0;
    At<uint32_t>(self, 0xB4) = 0;
    At<uint32_t>(self, 0xB8) = 0;
    At<uint32_t>(self, 0xAC) = 0;
}

// 0x486F00: the entity at +0x9C, with its timer started
void __fastcall SetEntity(uint8_t* self, re::Edx, uint8_t* entity, float seconds) {
    SetRef(&At<uint8_t*>(self, 0x9C), entity);
    PCS_Timer45BD70(self + 0xA0, seconds);
}

// 0x486F50: the entity at +0x9C dropped (when it's `entity`)
void __fastcall ClearEntity(uint8_t* self, re::Edx, uint8_t* entity) {
    re::Ptr32<uint8_t>& current = At<uint8_t*>(self, 0x9C);
    if (current != entity) {
        return;
    }
    if (current) {
        PCS_CleanUpOldReference(current, &current);
        current = nullptr;
    }
    PCS_Timer45BD10(self + 0xA0);
}

// The empty slot of one list filled from the peds around, starting at a random one of
// them (0 or 1): `accept` decides for a ped that holds no slot and targets this one (its
// the slot's last: that cleared instead)
template <typename Accept>
void FillSlot(uint8_t* self, uint8_t* slot, Accept accept) {
    int start = PCS_Rand() / 0x7FFF;
    uint8_t* owner = At<uint8_t*>(self, 0);
    for (int i = start; i < At<int32_t>(owner, 0x3C0); i++, owner = At<uint8_t*>(self, 0)) {
        uint8_t* ped = At<uint8_t*>(owner, 0x3C4 + i * 4);
        if (!ped || PCS_IsInSlots(self, ped) || PCS_PedGetTarget(ped) != At<uint8_t*>(self, 0)) {
            continue;
        }
        if (At<uint8_t*>(slot, 4) == ped) {
            At<uint8_t*>(slot, 4) = nullptr;
            continue;
        }
        if (accept(ped)) {
            TakeSlot(slot, ped);
            break;
        }
    }
    At<uint32_t>(slot, 8) = 0;
}

// 0x486F90: the slots kept up: this ped's slot on its target given up after the butes'
// time, the chosen slot (+0x324) to the nearest threat targeting it, and each list's
// first empty slot filled
void __fastcall UpdateSlots(uint8_t* self, re::Edx) {
    uint8_t* target = PCS_PedGetTarget(At<void*>(self, 0));
    if (target && IsPed(target)) {
        uint8_t* owner = At<uint8_t*>(self, 0);
        uint8_t* slot = PCS_FindSlot(Combat(target), owner);
        if (slot) {
            uint32_t since = At<uint32_t>(owner, 0x550);
            if (!(since > At<uint32_t>(slot, 0x14))) {
                since = At<uint32_t>(slot, 0x14);
            }
            float limit = At<int32_t>(slot, 0x10) == 3 ? At<float>(PCS_aiButes, 0x50) : At<float>(PCS_aiButes, 0x4C);
            if (limit > 0.0f) {
                double elapsed = static_cast<double>(PCS_timeInMs - since);
                if (static_cast<double>(limit) * 1000.0 < elapsed) {
                    At<uint32_t>(slot, 0) = 0;
                    At<uint32_t>(slot, 4) = 0;
                    At<uint32_t>(slot, 8) = 0;
                    At<uint32_t>(slot, 0x14) = 0;
                }
            }
        }
    }

    // the chosen slot
    {
        float bestDistance = 0.0f;
        bool threatened = false;
        uint8_t* best = nullptr;
        uint8_t* owner = At<uint8_t*>(self, 0);
        for (int i = 0; i < At<int32_t>(owner, 0x3C0); i++, owner = At<uint8_t*>(self, 0)) {
            uint8_t* ped = At<uint8_t*>(owner, 0x3C4 + i * 4);
            if (!ped) {
                continue;
            }
            if (PCS_IsThreat(Combat(ped), owner)) {
                threatened = true;
            }
            uint8_t* me = At<uint8_t*>(self, 0);
            if (PCS_PedGetTarget(ped) != me || !PCS_IsThreat(Combat(ped), me) || !WeaponIdle(ped)) {
                continue;
            }
            float distance = PCS_Distance41A950(EntityPos(ped), EntityPos(At<uint8_t*>(self, 0)));
            if (!best || bestDistance > distance) {
                bestDistance = distance;
                best = ped;
            }
            if (!PCS_IsInSlots(self, ped)) {
                At<uint8_t*>(self, 0x324) = ped;
                At<uint8_t*>(self, 0x328) = ped;
                At<uint32_t>(self, 0x338) = PCS_timeInMs;
                break;
            }
        }
        if (!threatened) {
            if (At<uint8_t*>(self, 0x324)) {
                At<uint32_t>(self, 0x324) = 0;
                At<uint32_t>(self, 0x328) = 0;
                At<uint32_t>(self, 0x32C) = 0;
                At<uint32_t>(self, 0x338) = 0;
            }
        } else if (!At<uint8_t*>(self, 0x324) && best) {
            PCS_RemoveFromSlots(self, best);
            At<uint8_t*>(self, 0x324) = best;
            At<uint8_t*>(self, 0x328) = best;
            At<uint32_t>(self, 0x338) = PCS_timeInMs;
        }
    }

    // list 2: students
    if (uint8_t* slot = FreeSlot(self, 2)) {
        FillSlot(self, slot, [](uint8_t* ped) { return IsStudentType(At<int32_t>(ped, 0x1310)); });
    }
    // list 3: of the slot's group, in a vehicle
    if (uint8_t* slot = FreeSlot(self, 3)) {
        FillSlot(self, slot, [slot](uint8_t* ped) {
            uint8_t* group = At<uint8_t*>(slot, 8);
            return (group == At<uint8_t*>(ped, 0x714) || !group) && At<void*>(ped, 0x1554) != nullptr;
        });
    }
    // list 0 (the chosen slot when that's full and the chosen one empty): of the slot's
    // group, attacking it (its lists 0 or 1), unarmed
    uint8_t* slot = FreeSlot(self, 0);
    if (!slot && !At<uint8_t*>(self, 0x324)) {
        slot = self + 0x324;
    }
    if (slot) {
        FillSlot(self, slot, [self, slot](uint8_t* ped) {
            uint8_t* group = At<uint8_t*>(slot, 8);
            if (group != At<uint8_t*>(ped, 0x714) && group) {
                return false;
            }
            if (!RefListContains(self + 0x14, ped) && !RefListContains(self + 0x40, ped)) {
                return false;
            }
            return WeaponIdle(ped);
        });
    }
    // list 1: of the slot's group, attacking it, armed
    slot = FreeSlot(self, 1);
    if (!slot) {
        return;
    }
    FillSlot(self, slot, [self, slot](uint8_t* ped) {
        uint8_t* group = At<uint8_t*>(slot, 8);
        if (group != At<uint8_t*>(ped, 0x714) && group) {
            return false;
        }
        if (!RefListContains(self + 0x14, ped) && !RefListContains(self + 0x40, ped) &&
            !RefListContains(self + 0x6C, ped)) {
            return false;
        }
        return PCS_PedHasWeapon(ped) && !(PCS_Weapon4D96B0(At<void*>(ped, 0x1D0)) == 0.0f);
    });
}

// 0x4876B0: whether to hold back from `target` rather than go in (its tether elsewhere,
// taunting, too far above or below, no way there, ...); +0x358 remembers the wait
uint8_t __fastcall ShouldWait(uint8_t* self, re::Edx, uint8_t* target, float, float maxHeight) {
    uint8_t result = 0;
    V3 diff;
    PCS_VectorSub(&diff, EntityPos(target), EntityPos(At<uint8_t*>(self, 0)));
    if (At<uint8_t*>(self, 0x354) != target) {
        PCS_RefSet(self + 0x354, target);
        At<uint8_t>(self, 0x358) = 0;
    }
    uint8_t* vehicle = At<uint8_t*>(target, 0x1758);
    if (vehicle && (At<uint8_t>(vehicle, 0x108) & 7) == 2 && At<int32_t>(vehicle, 0x46C) == 0) {
        return 1;
    }
    uint8_t* owner = At<uint8_t*>(self, 0);
    if (At<int32_t>(owner, 0x360) != 0 && !PCS_Tether499840(owner + 0x358, EntityPos(target), 0.0f)) {
        if (!PCS_Tether499600(At<uint8_t*>(self, 0) + 0x35C)) {
            return 1;
        }
        if (PCS_Tether499600(At<uint8_t*>(self, 0) + 0x35C) &&
            PCS_Tether499600(At<uint8_t*>(self, 0) + 0x35C) != target) {
            return 1;
        }
    }
    if (At<uint8_t>(target, 0x16CC) &&
        (PCS_ControllerIsPlaying(At<uint8_t*>(target, 0x2E0) + 0x258, PCS_nodeTaunt, 1, 1) ||
         PCS_Ped4473E0(At<void*>(self, 0)))) {
        At<uint8_t>(self, 0x358) = 0;
        result = 1;
    }
    float height = std::fabs(diff.z);
    if (maxHeight < height) {
        if (PCS_ControllerIsPlaying(At<uint8_t*>(target, 0x2E0) + 0x258, PCS_nodeWait, 1, 1) ||
            At<int32_t>(target, 0x13DC) != 0) {
            At<uint8_t>(self, 0x358) = 1;
            result = 1;
        }
    } else {
        At<uint8_t>(self, 0x358) = 0;
    }
    V3 pos = *EntityPos(target);
    if (!PCS_RouteIsValid(target + 0x1268) && !PCS_IsPointClear5EAE60(&pos, 0.4f, 0)) {
        result = 1;
    }
    if (At<uint8_t>(self, 0x358)) {
        result = 1;
    }
    return result;
}

// 0x4878A0: +0x350's +0x9B4 reference dropped
void __fastcall Clear350Ref(uint8_t* self, re::Edx) {
    uint8_t* other = At<uint8_t*>(self, 0x350);
    if (other) {
        re::Ptr32<uint8_t>& ref = At<uint8_t*>(other, 0x9B4);
        if (ref) {
            PCS_CleanUpOldReference(ref, &ref);
            ref = nullptr;
        }
    }
}

// 0x4878D0: a step: the entity's timer, the slots, the memory, the holds; stat 8 from
// the health (+20 when the target's +0x98C is this ped), 0..100
void __fastcall Update(uint8_t* self, re::Edx) {
    if (At<uint8_t>(At<uint8_t*>(self, 0), 0x1E25)) {
        return;
    }
    if (PCS_Timer45BDD0(self + 0xA0)) {
        re::Ptr32<uint8_t>& entity = At<uint8_t*>(self, 0x9C);
        if (entity) {
            PCS_CleanUpOldReference(entity, &entity);
            entity = nullptr;
        }
        PCS_Timer45BD10(self + 0xA0);
    }
    PCS_UpdateSlots(self);
    PCS_MemoryTick(self + 0x35C);
    if (At<uint8_t*>(self, 0x354) == nullptr && At<uint8_t>(self, 0x358)) {
        At<uint8_t>(self, 0x358) = 0;
    }
    if (At<uint8_t>(self, 0xBC) && At<uint32_t>(self, 0xC0) > 0 && At<uint32_t>(self, 0xC0) < PCS_timeInMs) {
        PCS_ReleaseSlots(self);
    }
    if (At<uint8_t>(self, 0x344) && At<uint32_t>(self, 0x348) <= PCS_timeInMs) {
        At<uint8_t>(self, 0x344) = 0;
        At<uint8_t*>(self, 0x340) = nullptr;
    }
    uint8_t* ped = At<uint8_t*>(self, 0);
    int value = static_cast<int>(At<float>(ped, 0x1CB4));
    uint8_t* target = PCS_PedGetTarget(ped);
    if (target && IsPed(target) && At<uint8_t*>(target, 0x98C) == At<uint8_t*>(self, 0)) {
        value += 20;
    }
    float clamped = static_cast<float>(value);
    if (clamped < 0.0f) {
        clamped = 0.0f;
    } else if (!(clamped < 100.0 || clamped == 100.0)) {
        clamped = 100.0f;
    }
    PCS_PedSetStat(At<void*>(self, 0), 8, static_cast<int>(clamped));
}

// 0x487A30: off `target`'s attacker lists (all but `keepList`) and slots
void __fastcall LeaveTarget(uint8_t* self, re::Edx, uint8_t* target, int keepList) {
    if (!target) {
        return;
    }
    if (At<int32_t>(self, 4) != keepList) {
        At<int32_t>(self, 4) = 4;
    }
    PCS_RemoveFromSlots(Combat(target), At<uint8_t*>(self, 0));
    for (int list = 0; list < 3; list++) {
        if (list == keepList) {
            continue;
        }
        uint8_t* attackers = target + 0x67C + list * 0x2C;
        int32_t& count = At<int32_t>(attackers, 0);
        for (int i = 0; i < count; i++) {
            if (At<uint8_t*>(attackers, 4 + i * 4) == At<uint8_t*>(self, 0)) {
                // (the last one moved into its place; the last place keeps its reference)
                count--;
                SetRef(&At<uint8_t*>(attackers, 4 + i * 4), At<uint8_t*>(attackers, 4 + count * 4));
                break;
            }
        }
    }
}

// 0x487AE0: ShouldWait, remembered for a few frames per (target, a, b)
uint8_t __fastcall ShouldWaitCached(uint8_t* self, re::Edx, uint8_t* target, float a, float b) {
    struct {
        re::Ptr32<uint8_t> target;
        float a, b;
    } key = {target, a, b};
    uint8_t answer;
    if (PCS_MemoryFind(self + 0x35C, &key, &answer)) {
        return answer;
    }
    answer = PCS_ShouldWait(self, target, a, b);
    PCS_MemoryAdd(self + 0x35C, &key, &answer);
    return answer;
}

// 0x487B70: off the current target's lists
void __fastcall LeaveCurrentTarget(uint8_t* self, re::Edx) {
    uint8_t* target = PCS_PedGetTarget(At<void*>(self, 0));
    if (!target) {
        PCS_LeaveTarget(self, nullptr, 4);
        return;
    }
    PCS_LeaveTarget(self, IsPed(target) ? target : nullptr, 4);
}

// 0x487BC0: onto `target`'s attacker list (open, not full or with someone this one
// should displace); with `dontCommit` only asked
uint8_t __fastcall JoinTarget(uint8_t* self, re::Edx, uint8_t* target, int list, int dontCommit) {
    if (list >= 0 && list <= 2 && At<uint8_t>(self, 0x98 + list) == 0) {
        return 0;
    }
    if (PCS_HasUsableWeapon(Combat(At<uint8_t*>(self, 0))) && list != 2) {
        return 0;
    }
    bool commit = !(dontCommit & 1);
    if (commit && PCS_PedGetTarget(At<void*>(self, 0)) != target) {
        PCS_PedSetTarget(At<void*>(self, 0), target);
    }
    uint8_t* attackers = target + 0x67C + list * 0x2C;
    if (RefListContains(attackers, At<uint8_t*>(self, 0))) {
        return 1;
    }
    int limit = At<int32_t>(target, 0x670 + list * 4);
    uint8_t* displaced = nullptr;
    if (At<int32_t>(attackers, 0) >= limit) {
        for (int i = 0; i < At<int32_t>(attackers, 0); i++) {
            uint8_t* other = At<uint8_t*>(attackers, 4 + i * 4);
            if (other && PCS_ShouldDisplace(self, target, list, other)) {
                displaced = other;
                break;
            }
        }
        if (!displaced && At<int32_t>(attackers, 0) >= limit) {
            return 0;
        }
    }
    if (commit) {
        if (displaced) {
            PCS_LeaveTarget(Combat(displaced), target, 4);
        }
        PCS_LeaveTarget(self, target, list);
        PCS_RefListAdd(attackers, At<uint8_t*>(self, 0));
        At<int32_t>(self, 4) = list;
    }
    return 1;
}

// 0x487D40: onto the current target's nearest list its distance allows
uint8_t __fastcall JoinCurrentTarget(uint8_t* self, re::Edx) {
    uint8_t* target = PCS_PedGetTarget(At<void*>(self, 0));
    if (!target || !IsPed(target)) {
        return 0;
    }
    float distance = PCS_Distance2D(EntityPos(At<uint8_t*>(self, 0)), EntityPos(target));
    if (distance <= At<float>(PCS_aiButes, 0x2C) && PCS_JoinTarget(self, target, 0, 0)) {
        return 1;
    }
    if (distance <= At<float>(PCS_aiButes, 0x30) && PCS_JoinTarget(self, target, 1, 0)) {
        return 1;
    }
    if (distance <= At<float>(PCS_aiButes, 0x34) && PCS_JoinTarget(self, target, 2, 0)) {
        return 1;
    }
    return 0;
}

} // namespace c07pcs

#define PCS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PCS_REVERSED("PedCombat::SetListOpen", 0x00486260, &c07pcs::SetListOpen);
PCS_REVERSED("PedCombat::GetAmmo", 0x00486280, &c07pcs::GetAmmo);
PCS_REVERSED("PedCombat::HasRemembered", 0x00486300, &c07pcs::HasRemembered);
PCS_REVERSED("PedCombat::Forget", 0x00486310, &c07pcs::Forget);
PCS_REVERSED("AnswerMemory::Tick", 0x00486340, &c07pcs::MemoryTick);
PCS_REVERSED("AnswerMemory::Find", 0x004863B0, &c07pcs::MemoryFind);
PCS_REVERSED("AnswerMemory::Add", 0x00486410, &c07pcs::MemoryAdd);
PCS_REVERSED("PedCombat::AsFreeVehicle", 0x004864B0, &c07pcs::AsFreeVehicle);
PCS_REVERSED("PedCombat::HasAttackers", 0x004864E0, &c07pcs::HasAttackers);
PCS_REVERSED("PedCombat::ShouldDisplace", 0x00486510, &c07pcs::ShouldDisplace);
PCS_REVERSED("PedCombat::IsCurrentEntity", 0x004866C0, &c07pcs::IsCurrentEntity);
PCS_REVERSED("PedCombat::IsThreat", 0x004866F0, &c07pcs::IsThreat);
PCS_REVERSED("PedCombat::InitList", 0x00486830, &c07pcs::InitList);
PCS_REVERSED("PedCombat::HoldsUnflaggedSlot", 0x004868B0, &c07pcs::HoldsUnflaggedSlot);
PCS_REVERSED("PedCombat::IsInSlots", 0x00486930, &c07pcs::IsInSlots);
PCS_REVERSED("PedCombat::FindSlot", 0x004869A0, &c07pcs::FindSlot);
PCS_REVERSED("PedCombat::RemoveFromSlots", 0x00486A30, &c07pcs::RemoveFromSlots);
PCS_REVERSED("PedCombat::ReleaseSlots", 0x00486AB0, &c07pcs::ReleaseSlots);
PCS_REVERSED("PedCombat::HoldSlots", 0x00486B10, &c07pcs::HoldSlots);
PCS_REVERSED("PedCombat::HasUsableWeapon", 0x00486BE0, &c07pcs::HasUsableWeapon);
PCS_REVERSED("PedCombat::HasAnyUsableWeapon", 0x00486C30, &c07pcs::HasAnyUsableWeapon);
PCS_REVERSED("PedCombat::ChooseWeapon", 0x00486CE0, &c07pcs::ChooseWeapon);
PCS_REVERSED("PedCombat::ClearTargetMark", 0x00486D50, &c07pcs::ClearTargetMark);
PCS_REVERSED("PedCombat::Remember", 0x00486D90, &c07pcs::Remember);
PCS_REVERSED("RefList::Add", 0x00486DC0, &c07pcs::RefListAdd);
PCS_REVERSED("PedCombat::ResetSlots", 0x00486E20, &c07pcs::ResetSlots);
PCS_REVERSED("PedCombat::Reset", 0x00486EA0, &c07pcs::Reset);
PCS_REVERSED("PedCombat::SetEntity", 0x00486F00, &c07pcs::SetEntity);
PCS_REVERSED("PedCombat::ClearEntity", 0x00486F50, &c07pcs::ClearEntity);
PCS_REVERSED("PedCombat::UpdateSlots", 0x00486F90, &c07pcs::UpdateSlots);
PCS_REVERSED("PedCombat::ShouldWait", 0x004876B0, &c07pcs::ShouldWait);
PCS_REVERSED("PedCombat::Clear350Ref", 0x004878A0, &c07pcs::Clear350Ref);
PCS_REVERSED("PedCombat::Update", 0x004878D0, &c07pcs::Update);
PCS_REVERSED("PedCombat::LeaveTarget", 0x00487A30, &c07pcs::LeaveTarget);
PCS_REVERSED("PedCombat::ShouldWaitCached", 0x00487AE0, &c07pcs::ShouldWaitCached);
PCS_REVERSED("PedCombat::LeaveCurrentTarget", 0x00487B70, &c07pcs::LeaveCurrentTarget);
PCS_REVERSED("PedCombat::JoinTarget", 0x00487BC0, &c07pcs::JoinTarget);
PCS_REVERSED("PedCombat::JoinCurrentTarget", 0x00487D40, &c07pcs::JoinCurrentTarget);
