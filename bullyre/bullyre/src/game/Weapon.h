#pragma once

// Weapons: the ped's inventory (CWeaponInventory, its items in the weapon item pool),
// the weapon objects (CWeapon), the container of a ped's weapon objects, projectiles
// and prop anims. A weapon's id is its model index: its CWeaponInfo hangs off the
// model info (+0x18).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

struct CPed;
struct CWeaponInventory;

// The weapon's data (the model info's +0x18; a default one at 0x20C88D8)
struct CWeaponInfo {
    uint8_t unknown00[0x18];
    int32_t unknown18;       // +0x18 (1: kept in the inventory)
    uint8_t unknown1C[0x34 - 0x1C];
    int32_t maxAmmo;         // +0x34 (-1: not counted)
    uint8_t unknown38[0x64 - 0x38];
    int32_t unknown64;       // +0x64 (non-zero: kept in the inventory)
    uint8_t unknown68[0x84 - 0x68];
    int32_t ammoWeapon;      // +0x84 the weapon its ammo is, given with it (<= 0: none)
};
static_assert(offsetof(CWeaponInfo, unknown18) == 0x18);
static_assert(offsetof(CWeaponInfo, maxAmmo) == 0x34);
static_assert(offsetof(CWeaponInfo, unknown64) == 0x64);
static_assert(offsetof(CWeaponInfo, ammoWeapon) == 0x84);

// A weapon a ped carries (the weapon item pool, 0xC0F654, 0x28 bytes a slot)
struct CWeaponItem {
    int16_t ammo;                  // +0
    int16_t weaponId;              // +2 (model index)
    uint32_t unknown04[8];         // +4 (zeroed by 0x4E8D70)
    re::Ptr32<CWeaponInventory> inventory;   // +0x24

    // 0x45C9E0: ammo, clamped to 0..the info's max; the player's stats and HUD told
    void SetAmmo(int ammo, int notify, int extra);
    // 0x45CB60
    CWeaponItem* Constructor(int weaponId, int ammo, CWeaponInventory* owner, int notify);
};
static_assert(offsetof(CWeaponItem, inventory) == 0x24);
static_assert(sizeof(CWeaponItem) == 0x28);

// SaveItems' copy of an item (0x24 bytes: without its inventory)
struct CWeaponItemCopy {
    int16_t ammo;
    int16_t weaponId;
    uint32_t unknown04[8];
};
static_assert(sizeof(CWeaponItemCopy) == 0x24);

// A ped's weapons (vtable 0x90AE44). The items aren't kept here: they're the weapon
// item pool's slots whose inventory is this one.
struct CWeaponInventory {
    re::Ptr32<void> vtable;                  // +0
    re::Ptr32<CPed> ped;                     // +4
    re::Ptr32<CWeaponItemCopy> saved;        // +8 SaveItems' copy
    int32_t numSaved;              // +0xC (-1: none)
    int32_t nextSlot;              // +0x10 where GetNextItem goes on from

    // 0x45C990
    CWeaponInventory* Constructor();
    // 0x45D370 (vtable slot 0)
    CWeaponInventory* ScalarDeletingDestructor(int flags);
    // 0x45C9B0
    void FreeSaved();
    // 0x45CBA0
    int GetNumItems();
    // 0x45CBE0: a copy of every item, for RestoreItems
    void SaveItems();
    // 0x45CC90: the pool slot's item if it's one of ours
    CWeaponItem* GetItemInSlot(int slot);
    // 0x45CCC0: the pool slot of our item of the weapon, -1 if none
    int FindItem(int weaponId);
    // 0x45CD10: a new item (for weapons kept in the inventory)
    void AddItem(int weaponId, int ammo, int notify, int extra);
    // 0x45CDB0: from nextSlot on (nullptr at the end, and back to the start)
    CWeaponItem* GetNextItem();
    // 0x45CE00: the weapon, and its ammo weapon with that one's max ammo
    void GiveWeapon(int weaponId, int notify, int extra);
    // 0x45CED0
    void AddAmmoOrItem(int weaponId, int ammo, int notify, int extra);
    // 0x45CF40: the weapon's ammo (1 for three weapons always there)
    int GetAmmo(int weaponId);
    // 0x45CFC0
    void Save(int32_t* data);
    // 0x45D030: adds (or with a negative count, takes) ammo; at 0 the item goes
    void AddAmmo(int weaponId, int ammo, int notify, int extra);
    // 0x45D0C0
    void RemoveAmmo(int weaponId, int ammo);
    // 0x45D190: every item gone
    void Clear();
    // 0x45D200: SaveItems' copy back
    void RestoreItems();
    // 0x45D240: the ammo set; returns the item's slot (-1: none)
    int SetAmmo(int weaponId, int ammo, int notify);
    // 0x45D2C0
    void Load(int32_t* data);
};
static_assert(offsetof(CWeaponInventory, nextSlot) == 0x10);

// A weapon object: the entity a ped holds (vtable 0x913AB4, 0x298 bytes, made with
// the game's Malloc). It can hold weapons of its own (attachments).
struct CWeapon {
    re::Ptr32<re::Ptr32<void>> vtable;                 // +0
    uint8_t unknown04[0x18 - 0x4];
    re::Ptr32<void> rwObject;                // +0x18
    uint8_t unknown1C[0xD4 - 0x1C];
    int32_t unknownD4;             // +0xD4 (1)
    uint8_t unknownD8[0x10E - 0xD8];
    int16_t modelIndex;            // +0x10E (-1: none)
    uint8_t unknown110[0x114 - 0x110];
    re::Ptr32<uint8_t> animData;             // +0x114 (&ownAnimData, or the holder's)
    uint8_t unknown118[0x120 - 0x118];
    float unknown120;              // +0x120 (-1)
    float unknown124;              // +0x124 (-1)
    float unknown128;              // +0x128
    int32_t weaponId;              // +0x12C
    uint8_t effectProxy[0x138 - 0x130];  // +0x130 (0x668E90 / 0x668EB0)
    int32_t boneA;                 // +0x138 the holder's bone it's attached to
    int32_t boneB;                 // +0x13C
    int32_t unknown140;            // +0x140
    int32_t attachHandle;          // +0x144 the anim manager's attachment (0: none)
    uint8_t ownAnimData[0x24C - 0x148];  // +0x148 (0x6BA240 / 0x6B5A80)
    re::Ptr32<uint8_t> holderAnimData;       // +0x24C
    re::Ptr32<uint8_t> effects;              // +0x250 3 effects of 0x1C (new[]: the count before)
    re::Ptr32<re::Ptr32<CWeapon>> children;            // +0x254 (grown by 0x6DB480)
    int32_t childCapacity;         // +0x258
    int32_t numChildren;           // +0x25C
    uint8_t unknown260[0x264 - 0x260];  // +0x260 (0x669FD0 / 0x49A650)
    re::Ptr32<CWeaponInfo> info;             // +0x264
    int32_t unknown268;            // +0x268
    uint32_t unknown26C[8];        // +0x26C
    float scale[3];                // +0x28C

    // 0x4DA760
    CWeapon* Constructor();
    // 0x4D9A90: the destructor's body
    void Destruct();
    // 0x4DA400 (vtable slot 0)
    CWeapon* ScalarDeletingDestructor(int flags);
    // 0x4D9410
    void Reset();
    // 0x4D9440
    uint8_t IsUnknown268InRange();
    // 0x4D9460: to the holder's bone, through the anim manager
    void Attach();
    // 0x4D9510
    void Detach();
    // 0x4D9540
    void StopEffect(int index);
    // 0x4D9580
    void FadeEffect(int index);
    // 0x4D95E0: the first effect that's on (stopped, returned)
    uint8_t* TakeActiveEffect();
    // 0x4D9650
    void SetEffectsVisible(int visible);
    // 0x4D96B0: the info's +0x68 (0 without one)
    float GetInfoByte68AsFloat();
    // 0x4D96D0
    uint8_t GetInfoByte68();
    // 0x4D96F0
    uint8_t InfoHasNext();
    // 0x4D9970
    void SetScale(const float* scale);
    // 0x4D9B10
    void DetachAndClearModel();
    // 0x4D9B60
    void SetHolder(uint8_t* holderAnimData, int boneA, int boneB);
    // 0x4D9BB0 (vtable slot 18)
    void Render();
    // 0x4D9D70 (vtable slot 17)
    void PreRender();
    // 0x4DA090: the child, detached and taken out
    CWeapon* TakeChild(int index);
    // 0x4DA0F0
    void DeleteChildren();
    // 0x4DA3B0
    void ChildrenCall466160(int value);
    // 0x4DA490
    void DeleteEffects();
    // 0x4DA600: the child at the index, the array grown to it (nullptr past 100)
    CWeapon* GetChild(uint32_t index);
    // 0x4DA840
    void SetWeapon(int weaponId, int value268);
    // 0x4DA8E0
    void ClearWeapon(int keepNext);
    // 0x4DA990
    void ClearWeaponKeepNext();
};
static_assert(offsetof(CWeapon, rwObject) == 0x18);
static_assert(offsetof(CWeapon, unknownD4) == 0xD4);
static_assert(offsetof(CWeapon, modelIndex) == 0x10E);
static_assert(offsetof(CWeapon, animData) == 0x114);
static_assert(offsetof(CWeapon, weaponId) == 0x12C);
static_assert(offsetof(CWeapon, attachHandle) == 0x144);
static_assert(offsetof(CWeapon, ownAnimData) == 0x148);
static_assert(offsetof(CWeapon, holderAnimData) == 0x24C);
static_assert(offsetof(CWeapon, numChildren) == 0x25C);
static_assert(offsetof(CWeapon, info) == 0x264);
static_assert(offsetof(CWeapon, unknown26C) == 0x26C);
static_assert(offsetof(CWeapon, scale) == 0x28C);
static_assert(sizeof(CWeapon) == 0x298);

// A ped's weapon objects (the pool at 0xC0F63C, 0x20 bytes; the ped's +0x1D4): five
// slots, and a default one the slots may point at (not owned)
struct CWeaponAccessoryContainer {
    re::Ptr32<void> skeleton;                // +0 (what bone names are looked up in)
    re::Ptr32<CWeapon> defaultWeapon;        // +4
    int32_t current;               // +8 (-1: none)
    re::Ptr32<CWeapon> slots[5];             // +0xC

    // 0x4DA9A0
    CWeaponAccessoryContainer* Constructor();
    // 0x4DA9C0
    void SetDefault(CWeapon* weapon);
    // 0x4DA9E0: every slot's weapon deleted (not the default one)
    void DeleteAll();
    // 0x4DAA20: the slot emptied; returns its weapon (nullptr: none, or the default)
    CWeapon* Take(int slot);
    // 0x4DAA50: the slot holding a weapon on the bone, -1 if none
    int FindByBone(const uint32_t* boneHash);
    // 0x4DAAA0: the slot holding the model, -1 if none
    int FindByModel(int modelIndex);
    // 0x4DAAE0: into the first empty slot; returns it (5: none)
    int Add(CWeapon* weapon);
    // 0x4DAB10
    uint8_t HasFreeSlot();
    // 0x4DAB30
    int GetNumWeapons();
    // 0x4DAB70
    CWeapon* GetSlot(uint32_t slot);
    // 0x4DAB90: vtable slot 17 of each
    void PreRenderAll();
    // 0x4DABC0: vtable slot 18 of each
    void RenderAll();
    // 0x4DABF0
    void CallAll466160(int value);
    // 0x4DAC30: each weapon dropped with the chance (0..1)
    void DropRandomly(float chance);
};
static_assert(offsetof(CWeaponAccessoryContainer, slots) == 0xC);
static_assert(sizeof(CWeaponAccessoryContainer) == 0x20);
