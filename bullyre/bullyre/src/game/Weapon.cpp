#include "game/Weapon.h"
#include "game/HeapCheck.h"

#include "game/Pools.h"
#include "reversed.h"

// CWeaponInventory and its items (the weapon item pool's)

#define WEAPON_REVERSED(name, address, fn) REVERSED(name, address, fn, nullptr, nullptr, re::Mode::Original)

namespace {

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// the compiler's "eh vector constructor iterator"
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void(__thiscall* ctor)(void*));
GAME_FN(CWeaponInfo*, __cdecl, GetWeaponInfo, 0x0051B450, int weaponId);
GAME_FN(void, __thiscall, WeaponInfo_Request, 0x007282A0, CWeaponInfo* info);   // (loads what it needs)
GAME_FN(void, __cdecl, Stats_Add, 0x00458480, int stat, int amount);
GAME_FN(void, __thiscall, AmmoChanged, 0x00541110, void* self, int weaponId, int oldAmmo, int newAmmo, int extra);
GAME_FN(void, __thiscall, HUDWeapons_Update, 0x0055C340, void* self);
GAME_FN(void, __thiscall, Ped_RemoveUnequip, 0x0049AD00, CPed* ped);
GAME_FN(uint8_t, __cdecl, IsDebugInfiniteAmmo, 0x00425340);  // reads 0xBCEAF2
GAME_FN(uint8_t, __cdecl, Save_CheckValue, 0x00733320, int, int, int value, int expected);
GAME_FN(uint8_t, __cdecl, Save_CheckBlock, 0x00733620, const char* name, void* data, int size);
// (the item's own, called where modes apply)
GAME_FN(void, __thiscall, Item_SetAmmo, 0x0045C9E0, CWeaponItem* item, int ammo, int notify, int extra);
GAME_FN(CWeaponItem*, __thiscall, Item_Construct, 0x0045CB60, CWeaponItem* item, int weaponId, int ammo, CWeaponInventory* owner, int notify);
GAME_FN(void, __thiscall, Item_ClearUnknown04, 0x004E8D70, void* p);
GAME_FN(CWeaponItem*, __thiscall, ItemPool_New, 0x0045CAD0, void* pool);
GAME_FN(void, __cdecl, Item_Delete, 0x0045CF90, CWeaponItem* item);
GAME_FN(void, __thiscall, ItemCopy_Construct, 0x00449890, void* copy);
GAME_FN(CWeaponItem*, __thiscall, ItemPool_GetAt, 0x00415DE0, void* pool, int slot);
GAME_FN(void, __thiscall, Inventory_FreeSaved, 0x0045C9B0, CWeaponInventory* self);
GAME_FN(int, __thiscall, Inventory_GetNumItems, 0x0045CBA0, CWeaponInventory* self);
GAME_FN(int, __thiscall, Inventory_FindItem, 0x0045CCC0, CWeaponInventory* self, int weaponId);
GAME_FN(void, __thiscall, Inventory_AddItem, 0x0045CD10, CWeaponInventory* self, int weaponId, int ammo, int notify, int extra);
GAME_FN(CWeaponItem*, __thiscall, Inventory_GetNextItem, 0x0045CDB0, CWeaponInventory* self);
GAME_FN(void, __thiscall, Inventory_AddAmmo, 0x0045D030, CWeaponInventory* self, int weaponId, int ammo, int notify, int extra);
GAME_FN(void, __thiscall, Inventory_Clear, 0x0045D190, CWeaponInventory* self);

GAME_VAR(void*, gAmmoListener, 0x00C67554);
GAME_VAR(void*, gHUDWeapons, 0x00C674F0);
// weapons whose pick-ups count in the player's stats (and the stat for each)
GAME_VAR(int32_t, gStatWeapon6D, 0x00BF4448);
GAME_VAR(int32_t, gStatWeapon70, 0x00BF4358);
GAME_VAR(int32_t, gStatWeapon73, 0x00BF4434);
GAME_VAR(int32_t, gStatWeapon76, 0x00BF435C);
// GetAmmo's three always-there weapons, and the one whose running out unequips
GAME_VAR(int32_t, gAlwaysWeapon1, 0x00BF4108);
GAME_VAR(int32_t, gAlwaysWeapon2, 0x00BF4100);
GAME_VAR(int32_t, gAlwaysWeapon3, 0x00BF4298);
GAME_VAR(int32_t, gUnequipWeapon, 0x00BF432C);

inline void* const vtableWeaponInventory = reinterpret_cast<void*>(0x0090AE44);
inline const char* const weaponSaveDataName = reinterpret_cast<const char*>(0x0090AE48);  // "WeaponSaveData"

CPool<CWeaponItem>* ItemPool() {
    return re::PtrCast<CPool<CWeaponItem>*>(CPools::ms_pWeaponItemPool);
}

// (the ped's type, +0x1310: 0xD the player)
bool IsPlayerPed(CPed* ped) {
    return *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(ped) + 0x1310) == 0xD;
}

CWeaponInventory* PedInventory(CPed* ped) {
    return *reinterpret_cast<re::Ptr32<CWeaponInventory>*>(reinterpret_cast<uint8_t*>(ped) + 0x1C4);
}

// (inlined: the pool's Delete)
void DeleteItem(CPool<CWeaponItem>* pool, CWeaponItem* item) {
    int slot = int((reinterpret_cast<uint8_t*>(item) - re::PtrCast<uint8_t*>(pool->objects)) / pool->elemSize);
    pool->flags[slot] |= 0x80;
    if (slot < pool->lastUsed) {
        pool->lastUsed = slot;
    }
}

// (inlined: the slot's item, nullptr for a free one)
CWeaponItem* ItemAt(int slot) {
    CPool<CWeaponItem>* pool = ItemPool();
    if (pool->flags[slot] & 0x80) {
        return nullptr;
    }
    return reinterpret_cast<CWeaponItem*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
}

} // namespace

// CWeaponItem

void CWeaponItem::SetAmmo(int newAmmo, int notify, int extra) {
    CWeaponInfo* info = GetWeaponInfo(weaponId);
    int oldAmmo = ammo;
    // (in floats: 0 below 0, the max above it)
    float value = float(newAmmo);
    float max = float(info->maxAmmo);
    float clamped;
    if (value < 0.0f) {
        clamped = 0.0f;
    } else if (max < value) {
        clamped = max;
    } else {
        clamped = value;
    }
    ammo = int16_t(int(clamped));
    if (!inventory || !inventory->ped || !IsPlayerPed(inventory->ped)) {
        return;
    }
    int gained = ammo - oldAmmo;
    if (gained > 0) {
        int id = weaponId;
        if (id == gStatWeapon6D) {
            Stats_Add(0x6D, gained);
        } else if (id == gStatWeapon70) {
            Stats_Add(0x70, gained);
        } else if (id == gStatWeapon73) {
            Stats_Add(0x73, gained);
        } else if (id == gStatWeapon76) {
            Stats_Add(0x76, gained);
        }
    }
    if (re::Flag(notify)) {
        AmmoChanged(gAmmoListener, weaponId, oldAmmo, ammo, extra);
    }
    HUDWeapons_Update(gHUDWeapons);
}
WEAPON_REVERSED("CWeaponItem::SetAmmo", 0x0045C9E0, re::MemberAddress(&CWeaponItem::SetAmmo));

CWeaponItem* CWeaponItem::Constructor(int id, int newAmmo, CWeaponInventory* owner, int notify) {
    Item_ClearUnknown04(unknown04);
    weaponId = int16_t(id);
    inventory = owner;
    ammo = 0;
    Item_SetAmmo(this, newAmmo, notify, 0);
    return this;
}
WEAPON_REVERSED("CWeaponItem::CWeaponItem", 0x0045CB60, re::MemberAddress(&CWeaponItem::Constructor));

// 0x4E8D70: the 8 dwords at +4 (a struct of its own, used elsewhere too)
static void __fastcall ClearUnknown04(uint32_t* p) {
    for (int i = 0; i < 8; i++) {
        p[i] = 0;
    }
}
WEAPON_REVERSED("CWeaponItem::ClearUnknown04", 0x004E8D70, &ClearUnknown04);

// CWeaponInventory

CWeaponInventory* CWeaponInventory::Constructor() {
    vtable = vtableWeaponInventory;
    saved = nullptr;
    numSaved = -1;
    return this;
}
WEAPON_REVERSED("CWeaponInventory::CWeaponInventory", 0x0045C990, re::MemberAddress(&CWeaponInventory::Constructor));

void CWeaponInventory::FreeSaved() {
    if (saved) {
        Free(saved);
        saved = nullptr;
        numSaved = -1;
    }
}
WEAPON_REVERSED("CWeaponInventory::FreeSaved", 0x0045C9B0, re::MemberAddress(&CWeaponInventory::FreeSaved));

CWeaponInventory* CWeaponInventory::ScalarDeletingDestructor(int flags) {
    vtable = vtableWeaponInventory;
    Inventory_Clear(this);
    if (saved) {
        Free(saved);
        saved = nullptr;
        numSaved = -1;
    }
    if (flags & 1) {
        Free(this);
    }
    return this;
}
WEAPON_REVERSED("CWeaponInventory::~CWeaponInventory", 0x0045D370, re::MemberAddress(&CWeaponInventory::ScalarDeletingDestructor));

int CWeaponInventory::GetNumItems() {
    CPool<CWeaponItem>* pool = ItemPool();
    int count = 0;
    for (int slot = 0; slot < pool->size; slot++) {
        if (!(pool->flags[slot] & 0x80)) {
            CWeaponItem* item = reinterpret_cast<CWeaponItem*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
            if (item && item->inventory == this) {
                count++;
            }
        }
    }
    return count;
}
WEAPON_REVERSED("CWeaponInventory::GetNumItems", 0x0045CBA0, re::MemberAddress(&CWeaponInventory::GetNumItems));

void CWeaponInventory::SaveItems() {
    if (saved) {
        Free(saved);
        saved = nullptr;
        numSaved = -1;
    }
    int count = Inventory_GetNumItems(this);
    numSaved = count;
    uint64_t bytes = uint64_t(uint32_t(count)) * sizeof(CWeaponItemCopy);
    void* array = Malloc(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : size_t(bytes));
    if (array) {
        VectorConstruct(array, sizeof(CWeaponItemCopy), count, reinterpret_cast<void(__thiscall*)(void*)>(ItemCopy_Construct));
    } else {
        array = nullptr;
    }
    saved = static_cast<CWeaponItemCopy*>(array);
    CPool<CWeaponItem>* pool = ItemPool();
    int size = pool->size;
    int copied = 0;
    for (int slot = 0; slot < size; slot++) {
        if (!(pool->flags[slot] & 0x80)) {
            CWeaponItem* item = reinterpret_cast<CWeaponItem*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
            if (item && item->inventory == this) {
                *reinterpret_cast<CWeaponItemCopy*>(re::PtrCast<uint8_t*>(saved) + copied) = *reinterpret_cast<CWeaponItemCopy*>(item);
                copied += sizeof(CWeaponItemCopy);
            }
        }
    }
}
WEAPON_REVERSED("CWeaponInventory::SaveItems", 0x0045CBE0, re::MemberAddress(&CWeaponInventory::SaveItems));

CWeaponItem* CWeaponInventory::GetItemInSlot(int slot) {
    CPool<CWeaponItem>* pool = ItemPool();
    if (!(pool->flags[slot] & 0x80)) {
        CWeaponItem* item = reinterpret_cast<CWeaponItem*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
        if (item && item->inventory == this) {
            return item;
        }
    }
    return nullptr;
}
WEAPON_REVERSED("CWeaponInventory::GetItemInSlot", 0x0045CC90, re::MemberAddress(&CWeaponInventory::GetItemInSlot));

int CWeaponInventory::FindItem(int weaponId) {
    CPool<CWeaponItem>* pool = ItemPool();
    int size = pool->size;
    for (int slot = 0; slot < size; slot++) {
        if (!(pool->flags[slot] & 0x80)) {
            CWeaponItem* item = reinterpret_cast<CWeaponItem*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
            if (item && item->weaponId == weaponId && item->inventory == this) {
                return slot;
            }
        }
    }
    return -1;
}
WEAPON_REVERSED("CWeaponInventory::FindItem", 0x0045CCC0, re::MemberAddress(&CWeaponInventory::FindItem));

void CWeaponInventory::AddItem(int weaponId, int ammo, int notify, int extra) {
    CWeaponInfo* info = GetWeaponInfo(weaponId);
    if (info->unknown64 == 0 && info->unknown18 != 1) {
        // (not kept: the player's pick-up still told)
        if (re::Flag(notify) && IsPlayerPed(ped)) {
            AmmoChanged(gAmmoListener, weaponId, 0, 1, extra);
        }
        return;
    }
    CWeaponItem* item = ItemPool_New(CPools::ms_pWeaponItemPool);
    if (!item) {
        return;
    }
    // (the item's constructor, inlined; its inventory is the ped's)
    CWeaponInventory* owner = PedInventory(ped);
    Item_ClearUnknown04(item->unknown04);
    item->weaponId = int16_t(weaponId);
    item->ammo = 0;
    item->inventory = owner;
    Item_SetAmmo(item, ammo, notify, 0);
}
WEAPON_REVERSED("CWeaponInventory::AddItem", 0x0045CD10, re::MemberAddress(&CWeaponInventory::AddItem));

CWeaponItem* CWeaponInventory::GetNextItem() {
    CPool<CWeaponItem>* pool = ItemPool();
    int size = pool->size;
    for (int slot = nextSlot; slot < size; slot++) {
        if (!(pool->flags[slot] & 0x80)) {
            CWeaponItem* item = reinterpret_cast<CWeaponItem*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
            if (item && item->inventory == this) {
                nextSlot = slot + 1;
                return item;
            }
        }
    }
    nextSlot = 0;
    return nullptr;
}
WEAPON_REVERSED("CWeaponInventory::GetNextItem", 0x0045CDB0, re::MemberAddress(&CWeaponInventory::GetNextItem));

void CWeaponInventory::GiveWeapon(int weaponId, int notify, int extra) {
    CWeaponInfo* info = GetWeaponInfo(weaponId);
    if (Inventory_FindItem(this, weaponId) == -1 || re::Flag(extra)) {
        int notifyPlayer = (IsPlayerPed(ped) && re::Flag(notify)) ? 1 : 0;
        Inventory_AddItem(this, weaponId, 1, notifyPlayer, extra);
    }
    WeaponInfo_Request(info);
    int ammoWeapon = info->ammoWeapon;
    if (ammoWeapon <= 0) {
        return;
    }
    int ammo = GetWeaponInfo(ammoWeapon)->maxAmmo;
    int slot = Inventory_FindItem(this, ammoWeapon);
    if (slot == -1) {
        Inventory_AddItem(this, ammoWeapon, ammo, extra, extra);
        return;
    }
    // (a free slot would give a null item: FindItem never returns one)
    Item_SetAmmo(ItemAt(slot), ammo, notify, extra);
}
WEAPON_REVERSED("CWeaponInventory::GiveWeapon", 0x0045CE00, re::MemberAddress(&CWeaponInventory::GiveWeapon));

void CWeaponInventory::AddAmmoOrItem(int weaponId, int ammo, int notify, int extra) {
    int slot = Inventory_FindItem(this, weaponId);
    if (slot == -1) {
        Inventory_AddItem(this, weaponId, ammo, notify, extra);
        return;
    }
    CWeaponItem* item = ItemAt(slot);
    Item_SetAmmo(item, item->ammo + ammo, notify, extra);
}
WEAPON_REVERSED("CWeaponInventory::AddAmmoOrItem", 0x0045CED0, re::MemberAddress(&CWeaponInventory::AddAmmoOrItem));

int CWeaponInventory::GetAmmo(int weaponId) {
    if (weaponId == gAlwaysWeapon1 || weaponId == gAlwaysWeapon2 || weaponId == gAlwaysWeapon3) {
        return 1;
    }
    int slot = Inventory_FindItem(this, weaponId);
    if (slot == -1) {
        return 0;
    }
    return ItemPool_GetAt(CPools::ms_pWeaponItemPool, slot)->ammo;
}
WEAPON_REVERSED("CWeaponInventory::GetAmmo", 0x0045CF40, re::MemberAddress(&CWeaponInventory::GetAmmo));

void CWeaponInventory::Save(int32_t* data) {
    // version 1, 0x144 bytes, 30 items
    data[0] = 1;
    data[1] = 0x144;
    data[2] = 30;
    nextSlot = 0;
    int count = 0;
    CWeaponItem* item = Inventory_GetNextItem(this);
    int32_t* out = data + 3;
    while (item) {
        if (count >= 30) {
            break;
        }
        out[0] = item->weaponId;
        count++;
        out[1] = item->ammo;
        out += 2;
        item = Inventory_GetNextItem(this);
    }
    data[count * 2 + 3] = -1;
    data[count * 2 + 4] = 0;
}
WEAPON_REVERSED("CWeaponInventory::Save", 0x0045CFC0, re::MemberAddress(&CWeaponInventory::Save));

void CWeaponInventory::AddAmmo(int weaponId, int ammo, int notify, int extra) {
    if (weaponId == -1) {
        return;
    }
    int slot = Inventory_FindItem(this, weaponId);
    if (slot == -1) {
        Inventory_AddItem(this, weaponId, ammo, notify, 0);
        return;
    }
    if (IsDebugInfiniteAmmo() && ammo < 0) {
        return;
    }
    CWeaponItem* item = ItemAt(slot);
    Item_SetAmmo(item, item->ammo + ammo, notify, extra);
    if (item->ammo <= 0) {
        Item_Delete(item);
    }
}
WEAPON_REVERSED("CWeaponInventory::AddAmmo", 0x0045D030, re::MemberAddress(&CWeaponInventory::AddAmmo));

void CWeaponInventory::RemoveAmmo(int weaponId, int ammo) {
    int slot = Inventory_FindItem(this, weaponId);
    if (slot == -1) {
        return;
    }
    if (IsDebugInfiniteAmmo() && ammo > 0) {
        return;
    }
    if (GetWeaponInfo(weaponId) && GetWeaponInfo(weaponId)->maxAmmo == -1) {
        return;
    }
    CWeaponItem* item = ItemAt(slot);
    Item_SetAmmo(item, item->ammo - ammo, 1, 0);
    if (IsPlayerPed(ped) && weaponId == gUnequipWeapon) {
        if (item->ammo <= 0) {
            Ped_RemoveUnequip(ped);
        }
        return;
    }
    if (item->ammo <= 0) {
        DeleteItem(ItemPool(), item);
    }
}
WEAPON_REVERSED("CWeaponInventory::RemoveAmmo", 0x0045D0C0, re::MemberAddress(&CWeaponInventory::RemoveAmmo));

void CWeaponInventory::Clear() {
    CPool<CWeaponItem>* pool = ItemPool();
    int size = pool->size;
    for (int slot = 0; slot < size; slot++) {
        if (!(pool->flags[slot] & 0x80)) {
            CWeaponItem* item = reinterpret_cast<CWeaponItem*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
            if (item && item->inventory == this) {
                DeleteItem(pool, item);
                pool = ItemPool();
            }
        }
    }
    nextSlot = 0;
}
WEAPON_REVERSED("CWeaponInventory::Clear", 0x0045D190, re::MemberAddress(&CWeaponInventory::Clear));

void CWeaponInventory::RestoreItems() {
    // (no snapshot (none taken, or deleted: numSaved -1): the game's unsigned loop ran 2^32 times
    // over the null array and stopped at its first read; the inventory is left as it is)
    if (saved == nullptr) {
        return;
    }
    Inventory_Clear(this);
    for (uint32_t i = 0; i < uint32_t(numSaved); i++) {
        Inventory_AddAmmo(this, saved[i].weaponId, saved[i].ammo, 0, 0);
    }
}
WEAPON_REVERSED("CWeaponInventory::RestoreItems", 0x0045D200, re::MemberAddress(&CWeaponInventory::RestoreItems));

int CWeaponInventory::SetAmmo(int weaponId, int ammo, int notify) {
    int slot = Inventory_FindItem(this, weaponId);
    if (slot == -1) {
        return -1;
    }
    CWeaponItem* item = ItemAt(slot);
    Item_SetAmmo(item, ammo, notify, 0);
    if (item->ammo <= 0) {
        DeleteItem(ItemPool(), item);
    }
    return slot;
}
WEAPON_REVERSED("CWeaponInventory::SetAmmo", 0x0045D240, re::MemberAddress(&CWeaponInventory::SetAmmo));

void CWeaponInventory::Load(int32_t* data) {
    Inventory_Clear(this);
    if (!Save_CheckValue(0, 0, data[0], 1) || !Save_CheckValue(0, 0, data[1], 0x144) ||
        !Save_CheckValue(0, 0, data[2], 30) || !Save_CheckBlock(weaponSaveDataName, data + 0x41, 0x10)) {
        return;
    }
    int32_t* in = data + 3;
    for (int i = 0; i < 30; i++) {
        if (in[0] == -1) {
            return;
        }
        CWeaponItem* item = ItemPool_New(CPools::ms_pWeaponItemPool);
        if (item) {
            Item_Construct(item, in[0], in[1], this, 0);
        }
        in += 2;
    }
}
WEAPON_REVERSED("CWeaponInventory::Load", 0x0045D2C0, re::MemberAddress(&CWeaponInventory::Load));

// CWeapon

namespace {

GAME_FN(void, __thiscall, AnimManager_Detach, 0x006C2200, void* manager, uint8_t* holderAnimData, uintptr_t handle);
GAME_FN(void, __thiscall, AnimManager_Attach, 0x006C2070, void* manager, uint8_t* holderAnimData, int bone, void* frame,
        int32_t* handle, void* node, uint8_t* animData, uintptr_t flag);
inline void* const theAnimManager = reinterpret_cast<void*>(0x020C4910);
GAME_FN(void*, __thiscall, Skeleton_GetNode, 0x00464EA0, void* skeleton, int bone);
GAME_FN(void*, __cdecl, GetFrameOfObject, 0x006C7370, void* rwObject);
GAME_FN(void, __thiscall, Effect_Stop, 0x00668E40, void* effect);
GAME_FN(void, __thiscall, Effect_Remove, 0x00668BA0, void* effect);
GAME_FN(void, __thiscall, Effect_SetAlpha_x64, 0x00668C40, void* effect, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Effect_SetAlpha(void* a0, float a1) {
    return Effect_SetAlpha_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, Effect_FadeOut, 0x00668D00, void* effect, float time);
GAME_FN(void, __thiscall, Effect_SetVisible, 0x00668CD0, void* effect, int visible);
GAME_FN(void, __thiscall, Effect_Destruct, 0x00668DC0, void* effect);
GAME_FN(void, __thiscall, EffectProxy_Construct, 0x00668E90, void* proxy);
GAME_FN(void, __thiscall, EffectProxy_Destruct, 0x00668EB0, void* proxy);
GAME_FN(void, __thiscall, AnimData_Construct, 0x006BA240, void* data);
GAME_FN(void, __thiscall, AnimData_Destruct, 0x006B5A80, void* data);
GAME_FN(void, __thiscall, Unknown260_Construct, 0x00669FD0, void* p);
GAME_FN(void, __thiscall, Unknown260_Destruct, 0x0049A650, void* p);
GAME_FN(void, __thiscall, Entity_Construct, 0x004656A0, void* entity);
GAME_FN(void, __thiscall, Entity_Destruct, 0x00467E60, void* entity);
GAME_FN(void, __thiscall, Entity_PreRender, 0x00466CC0, void* entity);
GAME_FN(void, __thiscall, Entity_Render, 0x00466D40, void* entity);
GAME_FN(void, __thiscall, Entity_Call466160, 0x00466160, void* entity, int value);
GAME_FN(void, __thiscall, MemoryTag_Set, 0x005EE570, void* self, int tag);
inline void* const memoryTags = reinterpret_cast<void*>(0x00D140B8);
GAME_FN(void, __thiscall, PtrArray_Grow, 0x006DB480, void* array, int capacity, int flag);
GAME_FN(uint8_t, __cdecl, RequestWeaponModels, 0x004D9790, int weaponId, int flags);   // all loaded?
GAME_FN(void, __cdecl, AddWeaponModelRefs, 0x004DA680, int weaponId, int remove);
GAME_FN(int, __cdecl, NextWeaponModel, 0x0051AE20, void* next, int first, int last);
GAME_VAR(int32_t, gFirstWeaponModel, 0x00A136B0);
GAME_VAR(int32_t, gLastWeaponModel, 0x00A136B4);
// (CWeapon's own, called where modes apply)
GAME_FN(void, __thiscall, Weapon_Destruct, 0x004D9A90, CWeapon* self);
GAME_FN(void, __thiscall, Weapon_ClearWeapon, 0x004DA8E0, CWeapon* self, int keepNext);
GAME_FN(void, __thiscall, Weapon_SetScale, 0x004D9970, CWeapon* self, const float* scale);
GAME_FN(void, __thiscall, Weapon_DeleteEffects, 0x004DA490, CWeapon* self);
GAME_FN(void, __thiscall, Weapon_DeleteChildren, 0x004DA0F0, CWeapon* self);
GAME_FN(void, __thiscall, Weapon_DetachAndClearModel, 0x004D9B10, CWeapon* self);
GAME_FN(void, __thiscall, Weapon_SetWeapon, 0x004DA840, CWeapon* self, int weaponId, int value268);
// the scaling callback SetScale installs (animData +4, with the weapon at +8)
inline void* const weaponScaleCallback = reinterpret_cast<void*>(0x004D9240);

inline void* const vtableWeapon = reinterpret_cast<void*>(0x00913AB4);

// the vtable's slots
void CallDeletingDestructor(void* object, int flags) {
    using Fn = void*(__thiscall*)(void*, int);
    re::PtrCast<Fn>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[0])(object, flags);
}
void CallSetModelIndex(CWeapon* weapon, int modelIndex, int flag) {
    using Fn = void(__thiscall*)(void*, int, int);
    re::PtrCast<Fn>(weapon->vtable[8])(weapon, modelIndex, flag);
}
void CallSlot(void* object, int slot) {
    using Fn = void(__thiscall*)(void*);
    re::PtrCast<Fn>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[slot])(object);
}

uint8_t* Effect(CWeapon* weapon, int index) {
    return weapon->effects + index * 0x1C;
}

// (inlined everywhere: the anim manager's attachment let go)
void DetachInline(CWeapon* weapon) {
    if (weapon->holderAnimData && weapon->attachHandle) {
        AnimManager_Detach(theAnimManager, weapon->holderAnimData, weapon->attachHandle);
        weapon->attachHandle = 0;
    }
}

} // namespace

CWeapon* CWeapon::Constructor() {
    Entity_Construct(this);
    vtable = static_cast<re::Ptr32<void>*>(vtableWeapon);
    EffectProxy_Construct(effectProxy);
    AnimData_Construct(ownAnimData);
    children = nullptr;
    childCapacity = 0;
    numChildren = 0;
    Unknown260_Construct(unknown260);
    Item_ClearUnknown04(unknown26C);
    scale[0] = 0.0f;
    scale[1] = 0.0f;
    scale[2] = 0.0f;
    unknown128 = 0.0f;
    attachHandle = 0;
    animData = ownAnimData;
    unknown120 = -1.0f;
    unknownD4 = 1;
    unknown124 = -1.0f;
    effects = nullptr;
    info = nullptr;
    Item_ClearUnknown04(unknown26C);
    float one[3] = {1.0f, 1.0f, 1.0f};
    unknown140 = 0;
    Weapon_SetScale(this, one);
    return this;
}
WEAPON_REVERSED("CWeapon::CWeapon", 0x004DA760, re::MemberAddress(&CWeapon::Constructor));

void CWeapon::Destruct() {
    unknown120 = -1.0f;
    unknown124 = -1.0f;
    vtable = static_cast<re::Ptr32<void>*>(vtableWeapon);
    if (modelIndex != -1) {
        Weapon_ClearWeapon(this, 0);
    }
    animData = nullptr;
    Unknown260_Destruct(unknown260);
    if (children) {
        Free(children);
        children = nullptr;
    }
    AnimData_Destruct(ownAnimData);
    EffectProxy_Destruct(effectProxy);
    Entity_Destruct(this);
}
WEAPON_REVERSED("CWeapon::~CWeapon", 0x004D9A90, re::MemberAddress(&CWeapon::Destruct));

CWeapon* CWeapon::ScalarDeletingDestructor(int flags) {
    Weapon_Destruct(this);
    if (flags & 1) {
        Free(this);
    }
    return this;
}
WEAPON_REVERSED("CWeapon::ScalarDeletingDestructor", 0x004DA400, re::MemberAddress(&CWeapon::ScalarDeletingDestructor));

void CWeapon::Reset() {
    heapcheck::AssertLive(this, "CWeapon::Reset");   // (testing)
    unknown120 = -1.0f;
    unknown124 = -1.0f;
    if (modelIndex != -1) {
        Weapon_ClearWeapon(this, 0);
    }
}
WEAPON_REVERSED("CWeapon::Reset", 0x004D9410, re::MemberAddress(&CWeapon::Reset));

uint8_t CWeapon::IsUnknown268InRange() {
    int32_t value = *reinterpret_cast<int32_t*>(re::PtrCast<uint8_t*>(info) + 0x98);
    return value > 0 && uint32_t(value) <= uint32_t(unknown268);
}
WEAPON_REVERSED("CWeapon::IsUnknown268InRange", 0x004D9440, re::MemberAddress(&CWeapon::IsUnknown268InRange));

void CWeapon::Attach() {
    if (attachHandle != 0 || !rwObject) {
        return;
    }
    void* node = nullptr;
    int bone = boneB;
    if (bone != -1) {
        void* skeleton = *reinterpret_cast<re::Ptr32<void>*>(animData + 0x94);
        if (skeleton && *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(skeleton) + 0x9C) > uint32_t(bone)) {
            node = Skeleton_GetNode(skeleton, bone);
        }
    }
    if (*reinterpret_cast<re::Ptr32<void>*>(animData + 0x94)) {
        AnimManager_Attach(theAnimManager, holderAnimData, boneA, nullptr, &attachHandle, node, animData, 0);
    } else {
        void* frame = GetFrameOfObject(rwObject);
        AnimManager_Attach(theAnimManager, holderAnimData, boneA, frame, &attachHandle, nullptr, nullptr, 0);
    }
}
WEAPON_REVERSED("CWeapon::Attach", 0x004D9460, re::MemberAddress(&CWeapon::Attach));

void CWeapon::Detach() {
    DetachInline(this);
}
WEAPON_REVERSED("CWeapon::Detach", 0x004D9510, re::MemberAddress(&CWeapon::Detach));

void CWeapon::StopEffect(int index) {
    if (!effects || *reinterpret_cast<int32_t*>(Effect(this, index) + 4) == 0) {
        return;
    }
    Effect_Stop(Effect(this, index));
    Effect_Remove(Effect(this, index));
}
WEAPON_REVERSED("CWeapon::StopEffect", 0x004D9540, re::MemberAddress(&CWeapon::StopEffect));

void CWeapon::FadeEffect(int index) {
    if (!effects || *reinterpret_cast<int32_t*>(Effect(this, index) + 4) == 0) {
        return;
    }
    Effect_SetAlpha(Effect(this, index), 0.0f);
    Effect_Stop(Effect(this, index));
    Effect_FadeOut(Effect(this, index), 0.5f);
}
WEAPON_REVERSED("CWeapon::FadeEffect", 0x004D9580, re::MemberAddress(&CWeapon::FadeEffect));

uint8_t* CWeapon::TakeActiveEffect() {
    if (!effects) {
        return nullptr;
    }
    for (int i = 0; i < 3; i++) {
        uint8_t* effect = Effect(this, i);
        if (effect[0x15] != 0 && *reinterpret_cast<int32_t*>(effect + 0x18) != 0xFF &&
            *reinterpret_cast<int32_t*>(effect + 4) != 0) {
            Effect_Stop(effect);
            *reinterpret_cast<int32_t*>(Effect(this, i) + 0x10) = -1;
            return Effect(this, i);
        }
    }
    return nullptr;
}
WEAPON_REVERSED("CWeapon::TakeActiveEffect", 0x004D95E0, re::MemberAddress(&CWeapon::TakeActiveEffect));

void CWeapon::SetEffectsVisible(int visible) {
    if (!effects) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        uint8_t* effect = Effect(this, i);
        if (*reinterpret_cast<int32_t*>(effect + 4) != 0) {
            Effect_SetVisible(effect, (re::Flag(visible) && effect[0x14] != 0) ? 1 : 0);
        }
    }
}
WEAPON_REVERSED("CWeapon::SetEffectsVisible", 0x004D9650, re::MemberAddress(&CWeapon::SetEffectsVisible));

float CWeapon::GetInfoByte68AsFloat() {
    if (info) {
        return float(int32_t(re::PtrCast<uint8_t*>(info)[0x68]));
    }
    return 0.0f;
}
WEAPON_REVERSED("CWeapon::GetInfoByte68AsFloat", 0x004D96B0, re::MemberAddress(&CWeapon::GetInfoByte68AsFloat));

uint8_t CWeapon::GetInfoByte68() {
    if (!info) {
        return 0;
    }
    return re::PtrCast<uint8_t*>(info)[0x68];
}
WEAPON_REVERSED("CWeapon::GetInfoByte68", 0x004D96D0, re::MemberAddress(&CWeapon::GetInfoByte68));

uint8_t CWeapon::InfoHasNext() {
    // (no null check)
    return *reinterpret_cast<int32_t*>(re::PtrCast<uint8_t*>(info) + 0x9C) != 0;
}
WEAPON_REVERSED("CWeapon::InfoHasNext", 0x004D96F0, re::MemberAddress(&CWeapon::InfoHasNext));

void CWeapon::SetScale(const float* newScale) {
    scale[0] = newScale[0];
    scale[1] = newScale[1];
    scale[2] = newScale[2];
    if (newScale[0] == 1.0f && newScale[1] == 1.0f && newScale[2] == 1.0f) {
        *reinterpret_cast<re::Ptr32<void>*>(animData + 4) = nullptr;
        *reinterpret_cast<re::Ptr32<void>*>(animData + 8) = nullptr;
        return;
    }
    *reinterpret_cast<re::Ptr32<void>*>(animData + 4) = weaponScaleCallback;
    *reinterpret_cast<re::Ptr32<void>*>(animData + 8) = this;
}
WEAPON_REVERSED("CWeapon::SetScale", 0x004D9970, re::MemberAddress(&CWeapon::SetScale));

void CWeapon::DetachAndClearModel() {
    if (!rwObject) {
        return;
    }
    DetachInline(this);
    if (modelIndex != -1) {
        CallSetModelIndex(this, -1, 1);
    }
}
WEAPON_REVERSED("CWeapon::DetachAndClearModel", 0x004D9B10, re::MemberAddress(&CWeapon::DetachAndClearModel));

void CWeapon::SetHolder(uint8_t* newHolderAnimData, int newBoneA, int newBoneB) {
    DetachInline(this);
    holderAnimData = newHolderAnimData;
    boneA = newBoneA;
    boneB = newBoneB;
}
WEAPON_REVERSED("CWeapon::SetHolder", 0x004D9B60, re::MemberAddress(&CWeapon::SetHolder));

void CWeapon::Render() {
    if (!rwObject) {
        return;
    }
    MemoryTag_Set(memoryTags, 7);
    Entity_Render(this);
    for (uint32_t i = 0; i < uint32_t(numChildren); i++) {
        if (children[i]) {
            CallSlot(children[i], 18);
        }
    }
}
WEAPON_REVERSED("CWeapon::Render", 0x004D9BB0, re::MemberAddress(&CWeapon::Render));

void CWeapon::PreRender() {
    if (!rwObject) {
        return;
    }
    Entity_PreRender(this);
    for (uint32_t i = 0; i < uint32_t(numChildren); i++) {
        if (children[i]) {
            CallSlot(children[i], 17);
        }
    }
}
WEAPON_REVERSED("CWeapon::PreRender", 0x004D9D70, re::MemberAddress(&CWeapon::PreRender));

CWeapon* CWeapon::TakeChild(int index) {
    heapcheck::CheckIndex(this, children, index, numChildren, "CWeapon::TakeChild");   // (testing)
    CWeapon* child = children[index];
    DetachInline(child);
    children[index] = nullptr;
    return child;
}
WEAPON_REVERSED("CWeapon::TakeChild", 0x004DA090, re::MemberAddress(&CWeapon::TakeChild));

void CWeapon::DeleteChildren() {
    // (testing, bullyre.ini "heap_check": the 2026-10-30 soaks read a freed child array here)
    if (heapcheck::g_validateFrees) {
        heapcheck::AssertLive(this, "CWeapon::DeleteChildren (this)");
        heapcheck::AssertLive(children, "CWeapon::DeleteChildren (the child array)");
        for (uint32_t i = 0; i < uint32_t(numChildren) && children != nullptr; i++) {
            heapcheck::AssertLive(children[i], "CWeapon::DeleteChildren (a child)");
        }
    }
    for (uint32_t i = 0; i < uint32_t(numChildren); i++) {
        CWeapon* child = children[i];
        if (!child) {
            continue;
        }
        // (the start of its destructor, repeated here)
        child->unknown120 = -1.0f;
        child->unknown124 = -1.0f;
        if (child->modelIndex != -1) {
            Weapon_ClearWeapon(child, 0);
        }
        if (children[i]) {
            CallDeletingDestructor(children[i], 1);
        }
        children[i] = nullptr;
    }
    if (children) {
        Free(children);
        children = nullptr;
    }
    numChildren = 0;
    childCapacity = 0;
}
WEAPON_REVERSED("CWeapon::DeleteChildren", 0x004DA0F0, re::MemberAddress(&CWeapon::DeleteChildren));

void CWeapon::ChildrenCall466160(int value) {
    int count = numChildren;
    for (int i = 0; i < count; i++) {
        if (children[i]) {
            Entity_Call466160(children[i], value);
        }
    }
}
WEAPON_REVERSED("CWeapon::ChildrenCall466160", 0x004DA3B0, re::MemberAddress(&CWeapon::ChildrenCall466160));

void CWeapon::DeleteEffects() {
    if (!effects) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        if (*reinterpret_cast<int32_t*>(Effect(this, i) + 4) != 0) {
            Effect_Stop(Effect(this, i));
            Effect_Remove(Effect(this, i));
        }
    }
    if (effects) {
        // (delete[]: the count before the array, destroyed from the last)
        int32_t* block = re::PtrCast<int32_t*>(effects) - 1;
        int count = *block;
        uint8_t* effect = effects + count * 0x1C;
        for (int i = count - 1; i >= 0; i--) {
            effect -= 0x1C;
            Effect_Destruct(effect);
        }
        Free(block);
    }
    effects = nullptr;
}
WEAPON_REVERSED("CWeapon::DeleteEffects", 0x004DA490, re::MemberAddress(&CWeapon::DeleteEffects));

CWeapon* CWeapon::GetChild(uint32_t index) {
    if (index > 100) {
        return nullptr;
    }
    if (numChildren <= int32_t(index)) {
        int toAdd = int(index) - numChildren + 1;
        do {
            if (childCapacity == numChildren) {
                int capacity = numChildren;
                capacity = uint32_t(capacity) > 0 ? capacity * 2 : 1;
                PtrArray_Grow(&children, capacity, 0);
            }
            heapcheck::CheckIndex(this, children, numChildren, childCapacity, "CWeapon::GetChild (growing)");
            children[numChildren] = nullptr;
            numChildren++;
        } while (--toAdd != 0);
    }
    heapcheck::CheckIndex(this, children, int(index), numChildren, "CWeapon::GetChild");   // (testing)
    return children[index];
}
WEAPON_REVERSED("CWeapon::GetChild", 0x004DA600, re::MemberAddress(&CWeapon::GetChild));

void CWeapon::SetWeapon(int id, int value268) {
    if (id != -1) {
        if (!RequestWeaponModels(id, 0)) {
            id = -1;
        } else {
            AddWeaponModelRefs(id, 0);
        }
    }
    unknown128 = 0.0f;
    weaponId = id;
    unknown140 = 0;
    unknown120 = -1.0f;
    unknown124 = -1.0f;
    Item_ClearUnknown04(unknown26C);
    unknown268 = value268;
    info = GetWeaponInfo(id);
    if (rwObject) {
        Weapon_DetachAndClearModel(this);
    }
    if (id != -1) {
        CallSetModelIndex(this, id, 1);
    }
}
WEAPON_REVERSED("CWeapon::SetWeapon", 0x004DA840, re::MemberAddress(&CWeapon::SetWeapon));

void CWeapon::ClearWeapon(int keepNext) {
    int oldModel = modelIndex;
    CWeaponInfo* oldInfo = info;
    Weapon_DeleteEffects(this);
    Weapon_DeleteChildren(this);
    if (rwObject) {
        DetachInline(this);
        if (modelIndex != -1) {
            CallSetModelIndex(this, -1, 1);
        }
    }
    if (oldModel == -1) {
        return;
    }
    if (re::Flag(keepNext)) {
        void* next = *reinterpret_cast<re::Ptr32<void>*>(reinterpret_cast<uint8_t*>(oldInfo) + 0x9C);
        Weapon_SetWeapon(this, NextWeaponModel(next, gFirstWeaponModel, gLastWeaponModel), 0);
    }
    AddWeaponModelRefs(oldModel, 1);
}
WEAPON_REVERSED("CWeapon::ClearWeapon", 0x004DA8E0, re::MemberAddress(&CWeapon::ClearWeapon));

void CWeapon::ClearWeaponKeepNext() {
    Weapon_ClearWeapon(this, 1);
}
WEAPON_REVERSED("CWeapon::ClearWeaponKeepNext", 0x004DA990, re::MemberAddress(&CWeapon::ClearWeaponKeepNext));

// The first-person weapons list (10 at 0xC3A94C, the count at 0xC3A974)

namespace {
GAME_ARRAY(void*, gFirstPersonList, 0x00C3A94C);
GAME_VAR(uint32_t, gFirstPersonCount, 0x00C3A974);
} // namespace

// 0x4D99E0 (no bounds check)
static void __cdecl FirstPersonList_Add(void* object) {
    gFirstPersonList[gFirstPersonCount] = object;
    gFirstPersonCount++;
}
WEAPON_REVERSED("FirstPersonList::Add", 0x004D99E0, &FirstPersonList_Add);

// 0x4D9A00
static void __cdecl FirstPersonList_Remove(void* object) {
    uint32_t count = gFirstPersonCount;
    if (count == 0) {
        return;
    }
    uint32_t i = 0;
    while (gFirstPersonList[i] != object) {
        i++;
        if (i >= count) {
            return;
        }
    }
    if (i == 0xFFFFFFFFu) {
        return;
    }
    while (i < gFirstPersonCount - 1) {
        gFirstPersonList[i] = gFirstPersonList[i + 1];
        i++;
    }
    gFirstPersonCount = gFirstPersonCount - 1;
}
WEAPON_REVERSED("FirstPersonList::Remove", 0x004D9A00, &FirstPersonList_Remove);

// 0x4D9A60: vtable slot 5 of each
static void __cdecl FirstPersonList_RenderAll() {
    for (uint32_t i = 0; i < gFirstPersonCount; i++) {
        CallSlot(gFirstPersonList[i], 5);
    }
}
WEAPON_REVERSED("FirstPersonList::RenderAll", 0x004D9A60, &FirstPersonList_RenderAll);

// CWeaponAccessoryContainer

namespace {
GAME_FN(int, __cdecl, Skeleton_FindBone, 0x00655620, void* skeleton, uint32_t boneHash);
GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* generator);   // 0..1
inline void* const randomGenerator = reinterpret_cast<void*>(0x00CF356C);
inline void* const weaponManager = reinterpret_cast<void*>(0x00C3AEF8);
GAME_FN(void, __thiscall, WeaponManager_DeleteWeapon, 0x004DAF40, void* self, CWeapon* weapon);
GAME_FN(void, __thiscall, Weapon_Drop, 0x004DA190, CWeapon* self, int flag);
GAME_FN(uint8_t, __thiscall, Container_HasFreeSlot, 0x004DAB10, CWeaponAccessoryContainer* self);
GAME_FN(int, __thiscall, Container_Add, 0x004DAAE0, CWeaponAccessoryContainer* self, CWeapon* weapon);
GAME_FN(CWeapon*, __thiscall, Weapon_Construct, 0x004DA760, void* memory);
GAME_FN(void, __thiscall, Weapon_SetHolder, 0x004D9B60, CWeapon* self, uint8_t* holderAnimData, int boneA, int boneB);
GAME_FN(void, __thiscall, Weapon_Attach, 0x004D9460, CWeapon* self);
GAME_FN(const char*, __thiscall, BoneNames_Get, 0x004A75B0, void* table, int bone);
inline void* const boneNames = reinterpret_cast<void*>(0x00C3A978);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, Skeleton_GetBoneIndex, 0x00465580, void* skeleton, uint32_t hash, int32_t* index);

// (a live weapon of the container's own, not its default one)
bool IsOwnWeapon(CWeaponAccessoryContainer* container, CWeapon* weapon) {
    return weapon && weapon != container->defaultWeapon;
}
} // namespace

CWeaponAccessoryContainer* CWeaponAccessoryContainer::Constructor() {
    defaultWeapon = nullptr;
    skeleton = nullptr;
    for (int i = 0; i < 5; i++) {
        slots[i] = nullptr;
    }
    current = -1;
    return this;
}
WEAPON_REVERSED("CWeaponAccessoryContainer::CWeaponAccessoryContainer", 0x004DA9A0,
                re::MemberAddress(&CWeaponAccessoryContainer::Constructor));

void CWeaponAccessoryContainer::SetDefault(CWeapon* weapon) {
    slots[0] = weapon;
    current = 0;
    defaultWeapon = weapon;
}
WEAPON_REVERSED("CWeaponAccessoryContainer::SetDefault", 0x004DA9C0, re::MemberAddress(&CWeaponAccessoryContainer::SetDefault));

void CWeaponAccessoryContainer::DeleteAll() {
    for (int i = 0; i < 5; i++) {
        if (IsOwnWeapon(this, slots[i])) {
            CallDeletingDestructor(slots[i], 1);
        }
        slots[i] = nullptr;
    }
}
WEAPON_REVERSED("CWeaponAccessoryContainer::DeleteAll", 0x004DA9E0, re::MemberAddress(&CWeaponAccessoryContainer::DeleteAll));

CWeapon* CWeaponAccessoryContainer::Take(int slot) {
    CWeapon* weapon = slots[slot];
    if (!IsOwnWeapon(this, weapon)) {
        return nullptr;
    }
    slots[slot] = nullptr;
    return weapon;
}
WEAPON_REVERSED("CWeaponAccessoryContainer::Take", 0x004DAA20, re::MemberAddress(&CWeaponAccessoryContainer::Take));

int CWeaponAccessoryContainer::FindByBone(const uint32_t* boneHash) {
    int bone = Skeleton_FindBone(skeleton, *boneHash);
    if (bone == -1) {
        return -1;
    }
    for (int i = 0; i < 5; i++) {
        CWeapon* weapon = slots[i];
        if (IsOwnWeapon(this, weapon) && weapon->boneA == bone) {
            return i;
        }
    }
    return -1;
}
WEAPON_REVERSED("CWeaponAccessoryContainer::FindByBone", 0x004DAA50, re::MemberAddress(&CWeaponAccessoryContainer::FindByBone));

int CWeaponAccessoryContainer::FindByModel(int model) {
    for (int i = 0; i < 5; i++) {
        CWeapon* weapon = slots[i];
        if (IsOwnWeapon(this, weapon) && weapon->modelIndex == model) {
            return i;
        }
    }
    return -1;
}
WEAPON_REVERSED("CWeaponAccessoryContainer::FindByModel", 0x004DAAA0, re::MemberAddress(&CWeaponAccessoryContainer::FindByModel));

int CWeaponAccessoryContainer::Add(CWeapon* weapon) {
    int i = 0;
    for (; i < 5; i++) {
        if (!slots[i]) {
            slots[i] = weapon;
            return i;
        }
    }
    return i;
}
WEAPON_REVERSED("CWeaponAccessoryContainer::Add", 0x004DAAE0, re::MemberAddress(&CWeaponAccessoryContainer::Add));

uint8_t CWeaponAccessoryContainer::HasFreeSlot() {
    for (int i = 0; i < 5; i++) {
        if (!slots[i]) {
            return 1;
        }
    }
    return 0;
}
WEAPON_REVERSED("CWeaponAccessoryContainer::HasFreeSlot", 0x004DAB10, re::MemberAddress(&CWeaponAccessoryContainer::HasFreeSlot));

int CWeaponAccessoryContainer::GetNumWeapons() {
    int count = 0;
    for (int i = 0; i < 5; i++) {
        if (slots[i]) {
            count++;
        }
    }
    return count;
}
WEAPON_REVERSED("CWeaponAccessoryContainer::GetNumWeapons", 0x004DAB30, re::MemberAddress(&CWeaponAccessoryContainer::GetNumWeapons));

CWeapon* CWeaponAccessoryContainer::GetSlot(uint32_t slot) {
    if (slot > 4) {
        return nullptr;
    }
    return slots[slot];
}
WEAPON_REVERSED("CWeaponAccessoryContainer::GetSlot", 0x004DAB70, re::MemberAddress(&CWeaponAccessoryContainer::GetSlot));

void CWeaponAccessoryContainer::PreRenderAll() {
    // (the default one too)
    for (int i = 0; i < 5; i++) {
        if (slots[i]) {
            CallSlot(slots[i], 17);
        }
    }
}
WEAPON_REVERSED("CWeaponAccessoryContainer::PreRenderAll", 0x004DAB90, re::MemberAddress(&CWeaponAccessoryContainer::PreRenderAll));

void CWeaponAccessoryContainer::RenderAll() {
    for (int i = 0; i < 5; i++) {
        if (IsOwnWeapon(this, slots[i])) {
            CallSlot(slots[i], 18);
        }
    }
}
WEAPON_REVERSED("CWeaponAccessoryContainer::RenderAll", 0x004DABC0, re::MemberAddress(&CWeaponAccessoryContainer::RenderAll));

void CWeaponAccessoryContainer::CallAll466160(int value) {
    for (int i = 0; i < 5; i++) {
        if (IsOwnWeapon(this, slots[i])) {
            Entity_Call466160(slots[i], value);
        }
    }
}
WEAPON_REVERSED("CWeaponAccessoryContainer::CallAll466160", 0x004DABF0, re::MemberAddress(&CWeaponAccessoryContainer::CallAll466160));

void CWeaponAccessoryContainer::DropRandomly(float chance) {
    if (chance < 0.0f) {
        return;
    }
    if (1.0f < chance) {
        chance = 1.0f;
    } else if (chance <= 0.0f) {
        return;
    }
    for (int i = 0; i < 5; i++) {
        if (IsOwnWeapon(this, slots[i]) && Random_GetFloat(randomGenerator) <= chance) {
            WeaponManager_DeleteWeapon(weaponManager, slots[i]);
            slots[i] = nullptr;
            if (i == current) {
                current = -1;
            }
        }
    }
}
WEAPON_REVERSED("CWeaponAccessoryContainer::DropRandomly", 0x004DAC30, re::MemberAddress(&CWeaponAccessoryContainer::DropRandomly));

// The weapon manager (0xC3AEF8; its methods don't use it)

// 0x4DAE50: a new weapon into the ped's container, on two bones; its slot (-1: full)
static int __fastcall WeaponManager_GiveAccessory(void* self, re::Edx edx, uint8_t* ped, int weaponId, int boneA, int boneB) {
    (void)self;
    (void)edx;
    CWeaponAccessoryContainer* container = *reinterpret_cast<re::Ptr32<CWeaponAccessoryContainer>*>(ped + 0x1D4);
    if (!Container_HasFreeSlot(container)) {
        return -1;
    }
    void* memory = Malloc(sizeof(CWeapon));
    CWeapon* weapon = memory ? Weapon_Construct(memory) : nullptr;
    uint8_t* pedAnimData = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x114);
    // (the bone ids become the skeleton's indices, in the arguments; one not found
    // stays the id)
    void* skeleton = *reinterpret_cast<re::Ptr32<void>*>(pedAnimData + 0x94);
    Skeleton_GetBoneIndex(skeleton, HashName(BoneNames_Get(boneNames, boneA)), &boneA);
    Weapon_SetWeapon(weapon, weaponId, 0);
    skeleton = *reinterpret_cast<re::Ptr32<void>*>(weapon->animData + 0x94);
    // (into the ped argument's slot: a bone it doesn't find leaves the ped's address)
    int32_t pedArg = int32_t(reinterpret_cast<uintptr_t>(ped));
    Skeleton_GetBoneIndex(skeleton, HashName(BoneNames_Get(boneNames, boneB)), &pedArg);
    Weapon_SetHolder(weapon, *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x114), boneA, pedArg);
    Weapon_Attach(weapon);
    return Container_Add(container, weapon);
}
WEAPON_REVERSED("CWeaponManager::GiveAccessory", 0x004DAE50, RE_BODY(&WeaponManager_GiveAccessory));

// 0x4DAF40
static void __fastcall WeaponManager_DeleteWeaponImpl(void* self, re::Edx edx, CWeapon* weapon) {
    (void)self;
    (void)edx;
    if (weapon->modelIndex != -1) {
        Weapon_Drop(weapon, 1);
    }
    CallDeletingDestructor(weapon, 1);
}
WEAPON_REVERSED("CWeaponManager::DeleteWeapon", 0x004DAF40, RE_BODY(&WeaponManager_DeleteWeaponImpl));

// Projectiles

namespace {
// what the model infos' +0x12 indexes (0x38 bytes each; the object data)
inline uint8_t* const objectInfos = reinterpret_cast<uint8_t*>(0x00C35200);
// the projectile instances (0x34 bytes each, up to 0xC3A6BC; see ProjectileInstance)
inline uint8_t* const projectileInstances = reinterpret_cast<uint8_t*>(0x00C3A038);
inline uint8_t* const projectileInstancesEnd = reinterpret_cast<uint8_t*>(0x00C3A6BC);
// the particle sets of each instance
GAME_ARRAY(uint8_t*, gProjectilePSets, 0x00C3A6B8);
// 15 trail effects (0x3C bytes each)
inline uint8_t* const trailEffects = reinterpret_cast<uint8_t*>(0x00C39B60);
inline uint8_t* const trailEffectsEnd = reinterpret_cast<uint8_t*>(0x00C39EE4);
GAME_FN(void, __cdecl, EntityRef_Remove, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __thiscall, TargetingSystem_SetTargetEntity, 0x004D6660, void* self, void* entity);
GAME_FN(float, __cdecl, LimitAngle_, 0x004D3000, float angle);
GAME_FN(uint8_t*, __cdecl, GetProjectileInstance_, 0x004D34B0, int index);

// the model infos (CModelInfo::ms_modelInfoPtrs)
GAME_ARRAY(uint8_t*, gModelInfoPtrs, 0x00C67738);

// (float pi and 2 pi, as doubles)
constexpr double kPi = 3.1415927410125732;
constexpr double kTwoPi = 6.2831854820251465;
} // namespace

// 0x4D0CA0: the model's object data (nullptr: none)
static uint8_t* __cdecl GetObjectInfo(int modelIndex) {
    int index = *reinterpret_cast<int16_t*>(gModelInfoPtrs[modelIndex] + 0x12);
    if (index == -1) {
        return nullptr;
    }
    return objectInfos + index * 0x38;
}
WEAPON_REVERSED("CObjectData::GetObjectInfo", 0x004D0CA0, &GetObjectInfo);

// 0x4D34B0
static uint8_t* __cdecl GetProjectileInstance(int index) {
    return projectileInstances + index * 0x34;
}
WEAPON_REVERSED("CProjectileInfo::GetInstance", 0x004D34B0, &GetProjectileInstance);

// 0x4D3650: the instance's first field
static int32_t __cdecl GetProjectileInstanceField0(int index) {
    return *reinterpret_cast<int32_t*>(projectileInstances + index * 0x34);
}
WEAPON_REVERSED("CProjectileInfo::GetInstanceField0", 0x004D3650, &GetProjectileInstanceField0);

// 0x4D3DD0
static uint8_t* __cdecl GetProjectilePSet(int index) {
    return gProjectilePSets[index];
}
WEAPON_REVERSED("CProjectileInfo::GetPSet", 0x004D3DD0, &GetProjectilePSet);

// 0x4D3700
static void __cdecl ProjectileInstance_Finish(int index) {
    *reinterpret_cast<int32_t*>(projectileInstances + index * 0x34 + 0xC) = 1;
    *reinterpret_cast<int32_t*>(gProjectilePSets[index] + 0xD0) = 1;
    *reinterpret_cast<int32_t*>(gProjectilePSets[index] + 0x48) = 0;
}
WEAPON_REVERSED("CProjectileInfo::FinishInstance", 0x004D3700, &ProjectileInstance_Finish);

// 0x4D3D70: the live instance of the projectile, -1 if none
static int __cdecl FindProjectileInstance(void* projectile) {
    int index = 0;
    for (uint8_t* instance = projectileInstances + 4; instance < projectileInstancesEnd; instance += 0x34, index++) {
        if (instance[0x10] != 0 && *reinterpret_cast<re::Ptr32<void>*>(instance) == projectile) {
            return index;
        }
    }
    return -1;
}
WEAPON_REVERSED("CProjectileInfo::FindInstance", 0x004D3D70, &FindProjectileInstance);

// 0x4D3DA0: the instance's projectile if it's (still) a projectile entity (type 3)
static uint8_t* __cdecl GetProjectileOfInstance(int index) {
    uint8_t* entity = *reinterpret_cast<re::Ptr32<uint8_t>*>(projectileInstances + index * 0x34 + 4);
    if (!entity || (entity[0x108] & 7) != 3) {
        return nullptr;
    }
    return entity;
}
WEAPON_REVERSED("CProjectileInfo::GetProjectile", 0x004D3DA0, &GetProjectileOfInstance);

// 0x4D3660: out = v * scale
static float* __cdecl ScaleVector(float* out, const float* v, float scale) {
    float x = v[0] * scale;
    float y = v[1] * scale;
    float z = scale * v[2];
    out[0] = x;
    out[1] = y;
    out[2] = z;
    return out;
}
WEAPON_REVERSED("CProjectileInfo::ScaleVector", 0x004D3660, &ScaleVector);

// 0x4D36B0: out = a + b
static float* __cdecl AddVectors(float* out, const float* a, const float* b) {
    float x = a[0] + b[0];
    float y = a[1] + b[1];
    float z = a[2] + b[2];
    out[0] = x;
    out[1] = y;
    out[2] = z;
    return out;
}
WEAPON_REVERSED("CProjectileInfo::AddVectors", 0x004D36B0, &AddVectors);

// 0x4D3000: into -pi..pi (in doubles, the angle kept a float)
static float __cdecl LimitAngle(float angle) {
    while (double(angle) > kPi) {
        angle = float(double(angle) - kTwoPi);
    }
    while (double(angle) < -kPi) {
        angle = float(double(angle) + kTwoPi);
    }
    return angle;
}
WEAPON_REVERSED("CProjectileInfo::LimitAngle", 0x004D3000, &LimitAngle);

// 0x4D3070: b - a, limited
static float __cdecl AngleDifference(float a, float b) {
    double limitedB = LimitAngle_(b);
    float difference = float(limitedB - double(LimitAngle_(a)));
    return LimitAngle_(difference);
}
WEAPON_REVERSED("CProjectileInfo::AngleDifference", 0x004D3070, &AngleDifference);

// 0x4D2680: every trail effect cleared
static void __cdecl TrailEffects_Clear() {
    for (uint8_t* trail = trailEffects; trail < trailEffectsEnd; trail += 0x3C) {
        *reinterpret_cast<float*>(trail + 0x30) = 0.0f;
        *reinterpret_cast<float*>(trail + 0x14) = 0.0f;
        *reinterpret_cast<float*>(trail + 0x18) = 0.0f;
        *reinterpret_cast<int32_t*>(trail + 0x0) = 0;
        *reinterpret_cast<float*>(trail + 0x1C) = 0.0f;
        *reinterpret_cast<int32_t*>(trail + 0xC) = 0;
        *reinterpret_cast<int32_t*>(trail + 0x10) = 0;
        *reinterpret_cast<int32_t*>(trail + 0x20) = 0;
        trail[0x24] = 0;
        Effect_Remove(trail + 4);
    }
}
WEAPON_REVERSED("CProjectileInfo::ClearTrails", 0x004D2680, &TrailEffects_Clear);

// 0x4D26F0: the live trails within the radius of the point stopped
static void __cdecl TrailEffects_RemoveInSphere(float x, float y, float z, float radius) {
    for (uint8_t* trail = trailEffects; trail < trailEffectsEnd; trail += 0x3C) {
        if (trail[0x24] == 0) {
            continue;
        }
        const float* position = reinterpret_cast<float*>(trail + 0x14);
        double dy = double(y) - position[1];
        double dx = double(x) - position[0];
        double dz = double(z) - position[2];
        float distanceSq = float((dy * dy + dx * dx) + dz * dz);
        if (double(distanceSq) < double(radius) * double(radius)) {
            trail[0x24] = 0;
            Effect_Remove(trail + 4);
        }
    }
}
WEAPON_REVERSED("CProjectileInfo::RemoveTrailsInSphere", 0x004D26F0, &TrailEffects_RemoveInSphere);

// 0x4D15B0: CProjectile's Render (vtable slot 18): not while +0x94 is set
static void __fastcall Projectile_Render(uint8_t* self) {
    if (*reinterpret_cast<int32_t*>(self + 0x94) != 0) {
        return;
    }
    MemoryTag_Set(memoryTags, 3);
    Entity_Render(self);
}
WEAPON_REVERSED("CProjectile::Render", 0x004D15B0, &Projectile_Render);

// 0x4D21E0: +0x154 from the instance's pset while falling fast (+0x12C below -0.1)
static void __fastcall Projectile_UpdateUnknown154(uint8_t* self) {
    uint8_t* instance = GetProjectileInstance_(*reinterpret_cast<int32_t*>(self + 0x238));
    if (double(*reinterpret_cast<float*>(self + 0x12C)) < -0.10000000149011612) {
        uint8_t* pset = *reinterpret_cast<re::Ptr32<uint8_t>*>(instance + 0x30);
        *reinterpret_cast<float*>(self + 0x154) = *reinterpret_cast<float*>(pset + 0x128);
        return;
    }
    *reinterpret_cast<float*>(self + 0x154) = 1.0f;
}
WEAPON_REVERSED("CProjectile::UpdateUnknown154", 0x004D21E0, &Projectile_UpdateUnknown154);

// CTargetingSystem

// 0x4D5AD0 (7 bytes)
static void* __fastcall TargetingSystem_GetTarget(uint8_t* self) {
    return *reinterpret_cast<re::Ptr32<void>*>(self + 0xA0);
}
WEAPON_REVERSED("CTargetingSystem::GetTarget", 0x004D5AD0, &TargetingSystem_GetTarget);

// 0x4D79E0: a target (state 2), or none (state 0)
static void __fastcall TargetingSystem_SetTarget(uint8_t* self, re::Edx edx, void* entity) {
    (void)edx;
    if (entity) {
        TargetingSystem_SetTargetEntity(self, entity);
        *reinterpret_cast<int32_t*>(self + 8) = 2;
        return;
    }
    re::Ptr32<void>* target = reinterpret_cast<re::Ptr32<void>*>(self + 0xA0);
    if (*target) {
        *reinterpret_cast<float*>(self + 0x80) = 0.0f;
        if (*target) {
            EntityRef_Remove(*target, target);
            *target = nullptr;
        }
    }
    *reinterpret_cast<int32_t*>(self + 8) = 0;
}
WEAPON_REVERSED("CTargetingSystem::SetTarget", 0x004D79E0, RE_BODY(&TargetingSystem_SetTarget));

// CPropAnim (and entity helpers next to it). Their layout isn't pinned down: raw
// offsets.

namespace {
GAME_FN(void, __cdecl, EntityRef_Add, 0x004657D0, void* entity, void* ref);
GAME_FN(uint8_t*, __cdecl, GetObjectInfo_, 0x004D0CA0, int modelIndex);
GAME_FN(void, __cdecl, PropAnim_OnFlag170Set, 0x005B2D10, void* prop);
GAME_FN(void, __cdecl, PropAnim_OnFlag174Set, 0x005B2AD0, void* prop);
GAME_FN(void, __thiscall, PropList_Add, 0x005E68F0, void* list, void* prop);
GAME_FN(void, __thiscall, PropList_Remove, 0x005E6990, void* list, void* prop);
inline void* const propList = reinterpret_cast<void*>(0x00D12AF8);
GAME_FN(void, __thiscall, PropRender_SetUnknown, 0x005F3830, void* context, uintptr_t value);
GAME_FN(void, __thiscall, PropAnim_Unknown5F4A50, 0x005F4A50, void* self, uintptr_t value);
GAME_FN(void, __thiscall, PropAnim_Unknown5F4B00, 0x005F4B00, void* self, float value);
GAME_FN(void, __thiscall, PropAnim_SetCurrent_, 0x004ED940, void* self);
GAME_FN(uint8_t, __thiscall, ModelInfo_Unknown520480, 0x00520480, void* modelInfo);
GAME_FN(uint8_t*, __thiscall, ModelInfo_Unknown5201D0, 0x005201D0, void* modelInfo);
// the prop anim that's current (a render context at 0xA453F8)
inline uint8_t* const propContext = reinterpret_cast<uint8_t*>(0x00A453F8);
GAME_VAR(void*, gCurrentPropAnim, 0x00A45404);
GAME_VAR(float, gPropAnimUnknownA453E8, 0x00A453E8);
GAME_VAR(float, gTimeStep, 0x00C1A9A4);   // (a guess)
// the chapter settings the props read (0x4EA630): a count, then {key, value} pairs
GAME_VAR(int32_t, gNumChapterSettings, 0x00C3C9E0);
inline int32_t* const chapterSettings = reinterpret_cast<int32_t*>(0x00C3C9E8);
inline int32_t* const chapterSettingsEnd = reinterpret_cast<int32_t*>(0x00C3CC40);
GAME_VAR(CPool<uint8_t>*, gPool20C7C98, 0x020C7C98);

using re::At;
} // namespace

// 0x4EA2A0: +0x254, clamped to 0..1
static void __fastcall PropAnim_SetUnknown254(uint8_t* self, re::Edx edx, float value) {
    (void)edx;
    float clamped;
    if (value < 0.0f) {
        clamped = 0.0f;
    } else if (1.0f < value) {
        clamped = 1.0f;
    } else {
        clamped = value;
    }
    At<float>(self, 0x254) = clamped;
}
WEAPON_REVERSED("CPropAnim::SetUnknown254", 0x004EA2A0, RE_BODY(&PropAnim_SetUnknown254));

// 0x4EA360
static uint8_t __fastcall PropAnim_GetUnknown184(uint8_t* self) {
    return At<int32_t>(self, 0x184) != 0;
}
WEAPON_REVERSED("CPropAnim::GetUnknown184", 0x004EA360, &PropAnim_GetUnknown184);

// 0x4EA370
static void __fastcall PropAnim_SetUnknown184(uint8_t* self, re::Edx edx, int value) {
    (void)edx;
    At<int32_t>(self, 0x184) = uint8_t(value);
}
WEAPON_REVERSED("CPropAnim::SetUnknown184", 0x004EA370, RE_BODY(&PropAnim_SetUnknown184));

// 0x4EA380: its model's object data
static uint8_t* __fastcall Entity_GetObjectInfo(uint8_t* self) {
    return GetObjectInfo_(At<int16_t>(self, 0x10E));
}
WEAPON_REVERSED("CEntity::GetObjectInfo", 0x004EA380, &Entity_GetObjectInfo);

// 0x4EA3A0
static uint8_t __fastcall PropAnim_GetUnknown17C(uint8_t* self) {
    return At<int32_t>(self, 0x17C) != 0;
}
WEAPON_REVERSED("CPropAnim::GetUnknown17C", 0x004EA3A0, &PropAnim_GetUnknown17C);

// 0x4EA3B0
static void __fastcall PropAnim_SetUnknown17C(uint8_t* self, re::Edx edx, int value) {
    (void)edx;
    At<int32_t>(self, 0x17C) = uint8_t(value);
}
WEAPON_REVERSED("CPropAnim::SetUnknown17C", 0x004EA3B0, RE_BODY(&PropAnim_SetUnknown17C));

// 0x4EA3C0: +0x1C of the object, 0 without one
static int32_t __cdecl GetUnknown1COrZero(uint8_t* object) {
    if (!object) {
        return 0;
    }
    return At<int32_t>(object, 0x1C);
}
WEAPON_REVERSED("CPropAnim::GetUnknown1COrZero", 0x004EA3C0, &GetUnknown1COrZero);

// 0x4EA3D0 / 0x4EA400: of the kind, on, and within 20
static uint8_t __fastcall PropState_IsKind1Near(int32_t* self) {
    return self[0] == 1 && reinterpret_cast<uint8_t*>(self)[8] != 0 && reinterpret_cast<float*>(self)[3] <= 20.0f;
}
WEAPON_REVERSED("CPropState::IsKind1Near", 0x004EA3D0, &PropState_IsKind1Near);

static uint8_t __fastcall PropState_IsKind2Near(int32_t* self) {
    return self[0] == 2 && reinterpret_cast<uint8_t*>(self)[8] != 0 && reinterpret_cast<float*>(self)[3] <= 20.0f;
}
WEAPON_REVERSED("CPropState::IsKind2Near", 0x004EA400, &PropState_IsKind2Near);

// 0x4EA430: into the prop list (0xD12AF8), or out of it
static void __fastcall PropAnim_SetInList(uint8_t* self, re::Edx edx, int add) {
    (void)edx;
    if (re::Flag(add)) {
        PropList_Add(propList, self);
    } else {
        PropList_Remove(propList, self);
    }
}
WEAPON_REVERSED("CPropAnim::SetInList", 0x004EA430, RE_BODY(&PropAnim_SetInList));

// 0x4EA450: one of the flags at +0x160; +0x170 and +0x174 turning on are told
static void __fastcall PropAnim_SetFlag(uint8_t* self, re::Edx edx, int index, int value) {
    (void)edx;
    bool had170 = At<int32_t>(self, 0x170) != 0;
    bool had174 = At<int32_t>(self, 0x174) != 0;
    At<int32_t>(self, 0x160 + index * 4) = uint8_t(value);
    bool has174 = At<int32_t>(self, 0x174) != 0;
    if (!had170 && At<int32_t>(self, 0x170) != 0) {
        PropAnim_OnFlag170Set(self);
    }
    if (!had174 && has174) {
        PropAnim_OnFlag174Set(self);
    }
}
WEAPON_REVERSED("CPropAnim::SetFlag", 0x004EA450, RE_BODY(&PropAnim_SetFlag));

// 0x4EA630
static void __cdecl ChapterSettings_Clear() {
    gNumChapterSettings = 0;
    for (int32_t* setting = chapterSettings; setting < chapterSettingsEnd; setting += 2) {
        setting[0] = 0;
    }
}
WEAPON_REVERSED("ChapterSettings::Clear", 0x004EA630, &ChapterSettings_Clear);

// 0x4EA660: +0x1B4 from the chapter setting of its (+0x23C)'s key, if there's one
static void __fastcall PropAnim_ApplyChapterSetting(uint8_t* self) {
    int32_t key = At<int32_t>(At<uint8_t*>(self, 0x23C), 0x2C);
    int count = gNumChapterSettings;
    for (int i = 0; i < count; i++) {
        if (chapterSettings[i * 2] == key) {
            At<int32_t>(self, 0x1B4) = chapterSettings[i * 2 + 1] == 1;
            return;
        }
    }
}
WEAPON_REVERSED("CPropAnim::ApplyChapterSetting", 0x004EA660, &PropAnim_ApplyChapterSetting);

// 0x4EABB0: the entity a use-point is on (a ped's +0x548, or its +0x438/+0x444 when
// those are prop anims: type 6)
static uint8_t* __cdecl UsePoint_GetEntity(uint8_t* point) {
    if (!point) {
        return nullptr;
    }
    uint8_t* ped = At<uint8_t*>(point, 8);
    if (!ped) {
        return At<uint8_t*>(point, 0xC);
    }
    uint8_t* entity = At<uint8_t*>(ped, 0x548);
    if (entity) {
        return entity;
    }
    entity = At<uint8_t*>(ped, 0x438);
    if (entity && (entity[0x108] & 7) == 6) {
        return entity;
    }
    entity = At<uint8_t*>(ped, 0x444);
    if (entity && (entity[0x108] & 7) == 6) {
        return entity;
    }
    return nullptr;
}
WEAPON_REVERSED("UsePoint::GetPropAnim", 0x004EABB0, &UsePoint_GetEntity);

// 0x4EAC00: the same, any kind
static uint8_t* __cdecl UsePoint_GetAnyEntity(uint8_t* point) {
    if (!point) {
        return nullptr;
    }
    uint8_t* ped = At<uint8_t*>(point, 8);
    if (!ped) {
        return At<uint8_t*>(point, 0xC);
    }
    uint8_t* entity = At<uint8_t*>(ped, 0x548);
    if (entity) {
        return entity;
    }
    entity = At<uint8_t*>(ped, 0x438);
    if (entity) {
        return entity;
    }
    return At<uint8_t*>(ped, 0x444);
}
WEAPON_REVERSED("UsePoint::GetEntity", 0x004EAC00, &UsePoint_GetAnyEntity);

// 0x4EAE90: its element of the pool at 0x20C7C98 (+0x245: slot + 1, 0 none)
static uint8_t* __fastcall PropAnim_GetPoolElement(uint8_t* self) {
    uint8_t index = self[0x245];
    if (index == 0) {
        return nullptr;
    }
    CPool<uint8_t>* pool = gPool20C7C98;
    int slot = index - 1;
    if (pool->flags[slot] & 0x80) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * slot;
}
WEAPON_REVERSED("CPropAnim::GetPoolElement", 0x004EAE90, &PropAnim_GetPoolElement);

// 0x4ED940: the current prop anim (the render context pointed at it)
static void __fastcall PropAnim_SetCurrent(uint8_t* self) {
    if (self != gCurrentPropAnim) {
        if (gCurrentPropAnim) {
            EntityRef_Remove(gCurrentPropAnim, &gCurrentPropAnim);
        }
        gCurrentPropAnim = self;
        if (self) {
            EntityRef_Add(self, &gCurrentPropAnim);
        }
    }
    At<uint8_t*>(propContext, 0x38) = self + 0x3B0;
    At<uint8_t*>(propContext, 0x10) = self + 0x1BC;
    propContext[0x1D] = 0;
    PropRender_SetUnknown(propContext, At<int32_t>(self, 0x228));
    At<uint8_t*>(self, 0x140) = propContext;
    At<uint8_t*>(propContext, 0) = self + 0x128;
}
WEAPON_REVERSED("CPropAnim::SetCurrent", 0x004ED940, &PropAnim_SetCurrent);

// 0x4EE010
static void __fastcall PropAnim_Unknown4EE010(uint8_t* self, re::Edx edx, int value) {
    (void)edx;
    At<int32_t>(self, 0x188) = 0;
    PropAnim_SetCurrent_(self);
    PropAnim_Unknown5F4A50(self + 0x128, value);
    float scaled = gPropAnimUnknownA453E8 * gTimeStep;
    PropAnim_SetCurrent_(self);
    PropAnim_Unknown5F4B00(self + 0x128, scaled);
}
WEAPON_REVERSED("CPropAnim::Unknown4EE010", 0x004EE010, RE_BODY(&PropAnim_Unknown4EE010));

// 0x4ED040 (vtable slot 31)
static int32_t __fastcall PropAnim_Vf31(uint8_t* self) {
    if (!ModelInfo_Unknown520480(gModelInfoPtrs[At<int16_t>(self, 0x10E)]) && At<int32_t>(self, 0x1A4) == 0) {
        return 0;
    }
    return 1;
}
WEAPON_REVERSED("CPropAnim::vf31", 0x004ED040, &PropAnim_Vf31);

// 0x4ED070 (vtable slot 32): 100000 unless the model's flag is set
static double __fastcall PropAnim_Vf32(uint8_t* self) {
    uint8_t* modelInfo = gModelInfoPtrs[At<int16_t>(self, 0x10E)];
    uint8_t* data = ModelInfo_Unknown5201D0(modelInfo);
    if (!ModelInfo_Unknown520480(modelInfo)) {
        return 100000.0f;
    }
    return double(At<int32_t>(data, 0x1C));
}
WEAPON_REVERSED("CPropAnim::vf32", 0x004ED070, &PropAnim_Vf32);
