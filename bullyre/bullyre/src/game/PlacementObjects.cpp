// The model definitions of the placement files' model section (0x4373F0): one object
// per kind of model (SimpleObject, TimeObject, WeaponObject, ... by the game's RTTI),
// the record's bytes read over the object (after its vtable), then its vf2 makes the
// model info from them (vf1: the object's size). Plus the 2D effects of the section.
// The objects are laid out by offset (only these touch them); every one starts with the
// model id (+4) and its name (+8).

#include <cstdint>
#include <cstring>

#include "game/Placement.h"
#include "reversed.h"

namespace {

GAME_ARRAY(uint8_t*, po_modelInfoPtrs, 0x00C67738);
GAME_VAR(void*, po_sharedColModel, 0x00CF312C);    // (the peds')
GAME_VAR(uint8_t, po_loadModelSection, 0x00BF3812);
GAME_VAR(void*, po_pedStats, 0x00C2C084);
// the model id ranges of the kinds, first/last (-1 until one's seen)
GAME_VAR(int32_t, po_firstVehicle, 0x00A136A8);
GAME_VAR(int32_t, po_lastVehicle, 0x00A136AC);
GAME_VAR(int32_t, po_firstPAnim, 0x00A136A0);
GAME_VAR(int32_t, po_lastPAnim, 0x00A136A4);
GAME_VAR(int32_t, po_firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, po_lastWeapon, 0x00A136B4);
GAME_VAR(int32_t, po_firstItem, 0x00A136B8);
GAME_VAR(int32_t, po_lastItem, 0x00A136BC);
GAME_VAR(int32_t, po_firstAccessory, 0x00A136C0);
GAME_VAR(int32_t, po_lastAccessory, 0x00A136C4);
GAME_VAR(int32_t, po_firstPed, 0x00A136C8);
GAME_VAR(int32_t, po_lastPed, 0x00A136CC);
GAME_VAR(int32_t, po_firstSpecialPed, 0x00A136D0);  // "spfirst"
GAME_VAR(int32_t, po_lastSpecialPed, 0x00A136D4);   // "splast"
GAME_VAR(int32_t, po_firstCloth, 0x00A136D8);
GAME_VAR(int32_t, po_lastCloth, 0x00A136DC);

GAME_FN(uint8_t*, __cdecl, PO_AddAtomicModel, 0x0051C740, int id, const char* name);
GAME_FN(uint8_t*, __cdecl, PO_AddTimeModel, 0x0051C7A0, int id, const char* name);
GAME_FN(uint8_t*, __cdecl, PO_AddWeaponModel, 0x0051C800, int id, const char* name);
GAME_FN(uint8_t*, __cdecl, PO_AddAccessoryModel, 0x0051C860, int id, const char* name);
GAME_FN(uint8_t*, __cdecl, PO_AddVehicleModel, 0x0051C8C0, int id, const char* name, int isCar);
GAME_FN(uint8_t*, __cdecl, PO_AddPedModel, 0x0051C960, int id, const char* name, int special);
GAME_FN(uint8_t*, __cdecl, PO_RedefinePedModel, 0x0051B360, int id, const char* name, int special);
GAME_FN(uint8_t*, __cdecl, PO_AddPAnimModel, 0x0051C9D0, int id, const char* name);
GAME_FN(uint8_t*, __cdecl, PO_AddClothModel, 0x0051CA60, int id, const char* name);
GAME_FN(void, __cdecl, PO_AddModelName, 0x0043F370, const char* name, int id);
GAME_FN(void, __cdecl, PO_Fn_527510, 0x00527510, int id, int);
GAME_FN(void, __cdecl, PO_Fn_425350, 0x00425350);   // (after each ped)
GAME_FN(void, __cdecl, PO_Fn_425370, 0x00425370);   // (after each vehicle)
GAME_FN(void, __cdecl, PO_Fn_429E80, 0x00429E80, uint8_t* mi, uint32_t value);
GAME_FN(void, __thiscall, PO_SetDrawDistance, 0x00527440, uint8_t* mi, float distance);
GAME_FN(void, __thiscall, PO_SetTexDictionary, 0x0050E900, uint8_t* mi, const char* name, int);
GAME_FN(void, __thiscall, PO_SetColModel, 0x0050EB30, uint8_t* mi, void* col, int owns);
GAME_FN(void, __thiscall, PO_Add2dEffect, 0x0050EB70, uint8_t* mi, void* effect);
GAME_FN(uint8_t*, __thiscall, PO_GetTimeInfo, 0x00824EC0, uint8_t* mi);
GAME_FN(void, __thiscall, PO_Weapon_535F80, 0x00535F80, uint8_t* mi, const char*);
GAME_FN(void, __thiscall, PO_Vehicle_534B00, 0x00534B00, uint8_t* mi, const char* vehicleClass);
GAME_FN(uint16_t, __thiscall, PO_Handling_FindIndex, 0x004C9BE0, void* handling, const char* name);
inline void* const po_handlingData = reinterpret_cast<void*>(0x00C2DED0);
GAME_FN(int, __cdecl, PO_Fn_4CE3E0, 0x004CE3E0, const char*);
GAME_FN(void, __thiscall, PO_PAnim_520040, 0x00520040, uint8_t* mi, const char* anim);
GAME_FN(void, __thiscall, PO_PAnim_520180, 0x00520180, uint8_t* mi, const char*);
GAME_FN(void, __thiscall, PO_Ped_522190, 0x00522190, uint8_t* mi, const char*);
GAME_FN(void, __thiscall, PO_Ped_5221C0, 0x005221C0, uint8_t* mi, const char*);
GAME_FN(void, __thiscall, PO_Ped_5221F0, 0x005221F0, uint8_t* mi, const char*);
GAME_FN(void, __thiscall, PO_Ped_522370, 0x00522370, uint8_t* mi, int);
GAME_FN(void, __thiscall, PO_Ped_5226D0, 0x005226D0, uint8_t* mi, const char*, const char*);
GAME_FN(void, __thiscall, PO_Ped_5227D0, 0x005227D0, uint8_t* mi, const char*, const char*);
GAME_FN(int, __cdecl, PO_Fn_488B70, 0x00488B70, const char*);
GAME_FN(int, __cdecl, PO_Fn_488B10, 0x00488B10, const char*);
GAME_FN(int, __thiscall, PO_PedStats_Find, 0x0049A0F0, void* stats, const char* name);
// the 2D effects: made, their textures from the "particle" dictionary
GAME_FN(void*, __cdecl, PO_Effect_New, 0x00527620, uint32_t size);
GAME_FN(void, __cdecl, PO_PushTxd, 0x005F0A90);
GAME_FN(void, __cdecl, PO_PopTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, PO_FindTxdSlot, 0x005F22D0, const char* name, int);
GAME_FN(void, __cdecl, PO_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, PO_GetTexture, 0x005F1770, const char* name, int);
inline const char* const po_particle = reinterpret_cast<const char*>(0x00901970);
// the model section's
GAME_FN(void, __cdecl, PO_Free, 0x005EEAB0, void* p);
GAME_FN(uint32_t, __cdecl, PO_File_Read, 0x0042D2A0, void* file, void* buffer, uint32_t size);
GAME_FN(uint32_t, __cdecl, PO_File_Seek, 0x0042D300, void* file, uint32_t offset, int from);
GAME_FN(void*, __cdecl, PO_FindColModel, 0x0051A960, int id);
GAME_FN(void, __cdecl, PO_Fn_51CBD0, 0x0051CBD0, int id);
GAME_FN(void*, __cdecl, PO_Fn_51AFA0, 0x0051AFA0, int id);
GAME_FN(void, __cdecl, PO_Fn_51C650, 0x0051C650, int id);
GAME_FN(void, __thiscall, PO_C2DEffectObject_Add, 0x00436310, void* object);

using re::At;
int32_t Id(void* object) {
    return At<int32_t>(object, 4);
}
const char* Name(void* object) {
    return static_cast<const char*>(object) + 8;
}

// (inlined: a kind's id range)
void TrackRange(int32_t& first, int32_t& last, int32_t id) {
    if (id < first || first == -1) {
        first = id;
    }
    if (id > last) {
        last = id;
    }
}

// (inlined: the model info's colours/flags of the atomic kinds)
void SetColourBytes(uint8_t* mi, uint8_t b, uint8_t c2d, uint8_t c2e, uint8_t c2f) {
    mi[0xB] = b;
    mi[0x2D] = c2d;
    mi[0x2E] = c2e;
    mi[0x2F] = c2f;
    mi[0x32] = 0;
    mi[0x31] = 0;
    mi[0x30] = 0;
}

// (inlined: "strncmp(a, b, n) == 0" as repe cmpsb)
bool Same(const char* a, const char* b, size_t n) {
    return std::memcmp(a, b, n) == 0;
}

} // namespace

namespace ModelObjects {

// 0x435A80 BaseObject vf0: the destructor (flags 1: freed)
static void* __fastcall BaseObject_Destroy(void* self, re::Edx, uint8_t flags) {
    At<uint32_t>(self, 0) = 0x00903488;
    if (flags & 1) {
        PO_Free(self);
    }
    return self;
}

// the sizes (vf1)
static uint32_t __fastcall SimpleObject_Size(void*, re::Edx) {
    return 0x58;
}
static uint32_t __fastcall TimeObject_Size(void*, re::Edx) {
    return 0x7C;
}
static uint32_t __fastcall WeaponObject_Size(void*, re::Edx) {
    return 0x90;
}
static uint32_t __fastcall VehicleObject_Size(void*, re::Edx) {
    return 0xF8;
}
static uint32_t __fastcall PedObject_Size(void*, re::Edx) {
    return 0x258;
}
static uint32_t __fastcall C2DEffectObject_Size(void*, re::Edx) {
    return 0x84;
}
static uint32_t __fastcall PAnimObject_Size(void*, re::Edx) {
    return 0x8C;
}
static uint32_t __fastcall SecondaryObject_Size(void*, re::Edx) {
    return 0x48;
}

// 0x436310 C2DEffectObject vf2: a 2D effect (0x40) from the object's fields (+8 on),
// its two textures (+0x44, +0x64) from "particle", onto the model info
static void __fastcall C2DEffectObject_Add(uint8_t* self, re::Edx) {
    uint8_t* effect = static_cast<uint8_t*>(PO_Effect_New(0x40));
    if (effect) {
        At<uint16_t>(effect, 0x10) = 0xFFFF;
        At<float>(effect, 0) = 0.0f;
        At<float>(effect, 4) = 0.0f;
        At<float>(effect, 8) = 0.0f;
    }
    // (a failed allocation is written through anyway)
    At<uint32_t>(effect, 0) = At<uint32_t>(self, 8);
    At<uint32_t>(effect, 4) = At<uint32_t>(self, 0xC);
    At<uint32_t>(effect, 8) = At<uint32_t>(self, 0x10);
    effect[0xC] = self[0x14];
    effect[0xD] = self[0x15];
    effect[0xE] = self[0x16];
    effect[0xF] = self[0x17];
    std::memcpy(effect + 0x14, self + 0x18, 0x2C);
    PO_PushTxd();
    PO_SetCurrentTxd(PO_FindTxdSlot(po_particle, 0));
    At<void*>(effect, 0x14) = PO_GetTexture(reinterpret_cast<const char*>(self + 0x44), 0);
    At<void*>(effect, 0x18) = PO_GetTexture(reinterpret_cast<const char*>(self + 0x64), 0);
    PO_PopTxd();
    PO_Add2dEffect(po_modelInfoPtrs[Id(self)], effect);
}

// 0x4363E0 SimpleObject vf2: an atomic model; "nogo_" and "walkable_" names (either all
// lower or all upper case) get flag 0x1000000
static void __fastcall SimpleObject_Add(uint8_t* self, re::Edx) {
    uint8_t* mi = PO_AddAtomicModel(Id(self), Name(self));
    PO_SetDrawDistance(mi, At<float>(self, 0x34));
    PO_SetTexDictionary(mi, reinterpret_cast<const char*>(self + 0x1C), 0);
    PO_Fn_429E80(mi, At<uint32_t>(self, 0x38));
    const char* name = Name(self);
    bool special = (name[0] == 'n' && name[1] == 'o' && name[2] == 'g' && name[3] == 'o' && name[4] == '_') ||
                   (name[0] == 'N' && name[1] == 'O' && name[2] == 'G' && name[3] == 'O' && name[4] == '_') ||
                   (name[0] == 'w' && name[1] == 'a' && name[2] == 'l' && name[3] == 'k' && name[4] == 'a' && name[5] == 'b' &&
                    name[6] == 'l' && name[7] == 'e' && name[8] == '_') ||
                   (name[0] == 'W' && name[1] == 'A' && name[2] == 'L' && name[3] == 'K' && name[4] == 'A' && name[5] == 'B' &&
                    name[6] == 'L' && name[7] == 'E' && name[8] == '_');
    if (special) {
        At<uint32_t>(mi, 0x28) |= 0x1000000;
    } else {
        At<uint32_t>(mi, 0x28) &= ~0x1000000u;
    }
    SetColourBytes(mi, self[0x48], self[0x4C], self[0x50], self[0x54]);
    PO_Fn_527510(Id(self), 0);
    PO_AddModelName(Name(self), Id(self));
}

// 0x436530 TimeObject vf2: a time model (its name at +0x10), its hours (+0x64, +0x68)
static void __fastcall TimeObject_Add(uint8_t* self, re::Edx) {
    const char* name = reinterpret_cast<const char*>(self + 0x10);
    uint8_t* mi = PO_AddTimeModel(Id(self), name);
    PO_SetDrawDistance(mi, At<float>(self, 0x50));
    At<uint32_t>(mi, 0x34) = At<uint32_t>(self, 0x64);
    At<uint32_t>(mi, 0x38) = At<uint32_t>(self, 0x68);
    PO_SetTexDictionary(mi, reinterpret_cast<const char*>(self + 0x30), 0);
    PO_Fn_429E80(mi, At<uint32_t>(self, 0x54));
    if (uint8_t* info = PO_GetTimeInfo(mi)) {
        At<int32_t>(info, 0x3C) = Id(self);
    }
    SetColourBytes(mi, self[0x6C], self[0x70], self[0x74], self[0x78]);
    PO_Fn_527510(Id(self), 0);
    PO_AddModelName(name, Id(self));
}

// 0x4365E0 WeaponObject vf2
static void __fastcall WeaponObject_Add(uint8_t* self, re::Edx) {
    TrackRange(po_firstWeapon, po_lastWeapon, Id(self));
    uint8_t* mi = PO_AddWeaponModel(Id(self), Name(self));
    PO_SetTexDictionary(mi, reinterpret_cast<const char*>(self + 0x28), 0);
    reinterpret_cast<void(__thiscall*)(uint8_t*, const char*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(mi))[6]))(mi, reinterpret_cast<const char*>(self + 0x48));
    PO_Weapon_535F80(mi, reinterpret_cast<const char*>(self + 0x68));
    At<float>(mi, 0x50) = At<float>(self, 0x8C);
    PO_AddModelName(Name(self), Id(self));
}

// (ItemObject's and SecondaryObject's): an atomic model drawn to 30 units, flag 0x400000
static uint8_t* AddItemModel(uint8_t* self) {
    uint8_t* mi = PO_AddAtomicModel(Id(self), Name(self));
    PO_SetTexDictionary(mi, reinterpret_cast<const char*>(self + 0x28), 0);
    At<float>(mi, 0x24) = 30.0f;
    SetColourBytes(mi, 0xFF, 0, 0, 0);
    PO_Fn_527510(Id(self), 0);
    At<uint32_t>(mi, 0x28) |= 0x400000;
    PO_AddModelName(Name(self), Id(self));
    return mi;
}

// 0x436660 ItemObject vf2
static void __fastcall ItemObject_Add(uint8_t* self, re::Edx) {
    AddItemModel(self);
    TrackRange(po_firstItem, po_lastItem, Id(self));
}

// 0x4366F0 SecondaryObject vf2
static void __fastcall SecondaryObject_Add(uint8_t* self, re::Edx) {
    AddItemModel(self);
}

// 0x436760 AccessoryObject vf2
static void __fastcall AccessoryObject_Add(uint8_t* self, re::Edx) {
    uint8_t* mi = PO_AddAccessoryModel(Id(self), Name(self));
    PO_SetTexDictionary(mi, reinterpret_cast<const char*>(self + 0x28), 0);
    PO_AddModelName(Name(self), Id(self));
    if (po_firstAccessory == -1) {
        po_firstAccessory = Id(self);
    }
    po_lastAccessory = Id(self);
}

// 0x4367D0 VehicleObject vf2: a vehicle model ("car" or "bike" at +0x48), its handling
// (+0x58), its underscores in +0xB8 (after the first letter) made spaces
static void __fastcall VehicleObject_Add(uint8_t* self, re::Edx) {
    // (the flag is pushed as the whole register: 0xFFFFFF00 for a name before "car")
    const char* kind = reinterpret_cast<const char*>(self + 0x48);
    int cmp = std::memcmp(kind, "car", 4);
    uint32_t isCar = cmp == 0 ? 1u : (cmp < 0 ? 0xFFFFFF00u : 0u);
    uint8_t* mi = PO_AddVehicleModel(Id(self), Name(self), int(isCar));
    PO_SetTexDictionary(mi, reinterpret_cast<const char*>(self + 0x28), 0);
    reinterpret_cast<void(__thiscall*)(uint8_t*, const char*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(mi))[6]))(mi, reinterpret_cast<const char*>(self + 0x78));
    PO_Vehicle_534B00(mi, reinterpret_cast<const char*>(self + 0x98));
    char* gameName = reinterpret_cast<char*>(self + 0xB8);
    if (*gameName) {
        char* p = gameName;
        do {
            p++;
            if (*p == '_') {
                *p = ' ';
            }
        } while (*p);
    }
    At<uint32_t>(mi, 0xDC) = At<uint32_t>(self, 0xF4);
    if (Same(kind, "car", 4)) {
        float value = At<float>(self, 0xDC);
        At<uint16_t>(mi, 0x5C) = At<uint16_t>(self, 0xD8);
        At<int32_t>(mi, 0x50) = 0;
        At<float>(mi, 0x58) = value;
    } else if (Same(kind, "bike", 5)) {
        float value = At<float>(self, 0xDC);
        At<int32_t>(mi, 0x50) = 1;
        At<float>(mi, 0xE0) = float(At<int32_t>(self, 0xD8));
        At<float>(mi, 0x58) = value;
    }
    PO_AddModelName(Name(self), Id(self));
    At<uint16_t>(mi, 0x5E) = PO_Handling_FindIndex(po_handlingData, reinterpret_cast<const char*>(self + 0x58));
    int value = PO_Fn_4CE3E0(reinterpret_cast<const char*>(self + 0xE0));
    At<int32_t>(mi, 0x54) = value;
    if (value != -1) {
        At<uint16_t>(mi, 0x62) = At<uint16_t>(self, 0xEC);
    }
    TrackRange(po_firstVehicle, po_lastVehicle, Id(self));
    PO_Fn_425370();
}

// 0x436970 PAnimObject vf2
static void __fastcall PAnimObject_Add(uint8_t* self, re::Edx) {
    uint8_t* mi = PO_AddPAnimModel(Id(self), Name(self));
    const char* anim = reinterpret_cast<const char*>(self + 0x48);
    PO_PAnim_520040(mi, anim);
    PO_SetTexDictionary(mi, reinterpret_cast<const char*>(self + 0x28), 0);
    reinterpret_cast<void(__thiscall*)(uint8_t*, const char*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(mi))[6]))(mi, anim);
    PO_PAnim_520180(mi, reinterpret_cast<const char*>(self + 0x68));
    mi[0x8F] = At<int32_t>(self, 0x88) != 0;
    PO_AddModelName(Name(self), Id(self));
    if (po_firstPAnim == -1) {
        po_firstPAnim = Id(self);
    }
    po_lastPAnim = Id(self);
}

// 0x436A10 ClothObject vf2
static void __fastcall ClothObject_Add(uint8_t* self, re::Edx) {
    TrackRange(po_firstCloth, po_lastCloth, Id(self));
    uint8_t* mi = PO_AddClothModel(Id(self), Name(self));
    PO_SetTexDictionary(mi, reinterpret_cast<const char*>(self + 0x28), 0);
    PO_AddModelName(Name(self), Id(self));
}

// 0x437220 PedObject vf2: a ped model (or the existing one of type 8 redefined; another
// type leaves it null, and the game writes through it). "spfirst"/"splast" mark the
// special peds; models before the first special one aren't special
static void __fastcall PedObject_Add(uint8_t* self, re::Edx) {
    int32_t id = Id(self);
    uint8_t* existing = po_modelInfoPtrs[id];
    uint8_t* mi = nullptr;
    int32_t first = po_firstSpecialPed;
    if (first == -1 && Same(Name(self), "spfirst", 8)) {
        first = id;
        po_firstSpecialPed = id;
    }
    if (po_lastSpecialPed == -1 && Same(Name(self), "splast", 7)) {
        po_lastSpecialPed = Id(self);
    }
    id = Id(self);
    bool before = id < first || first == -1;
    if (!existing) {
        mi = PO_AddPedModel(id, Name(self), !before);
    } else if (existing[8] == 8) {
        // (pushed as the whole register: the model info's upper bytes with the flag)
        uint32_t flag = (uint32_t(reinterpret_cast<uintptr_t>(existing)) & 0xFFFFFF00u) | uint32_t(!before);
        mi = PO_RedefinePedModel(id, Name(self), int(flag));
    }
    mi[0x78] = self[4];
    PO_SetTexDictionary(mi, reinterpret_cast<const char*>(self + 0x28), 0);
    reinterpret_cast<void(__thiscall*)(uint8_t*, const char*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(mi))[6]))(mi, reinterpret_cast<const char*>(self + 0x90));
    PO_Ped_522190(mi, reinterpret_cast<const char*>(self + 0xB0));
    PO_Ped_5221C0(mi, reinterpret_cast<const char*>(self + 0xD0));
    PO_Ped_5221F0(mi, reinterpret_cast<const char*>(self + 0xF0));
    PO_Ped_522370(mi, PO_Fn_488B70(reinterpret_cast<const char*>(self + 0x48)));
    PO_SetColModel(mi, po_sharedColModel, 0);
    At<int32_t>(mi, 0x4C) = PO_Fn_488B10(reinterpret_cast<const char*>(self + 0x60));
    mi[0x50] = At<int32_t>(self, 0x110) != 0;
    At<int32_t>(mi, 0x54) = PO_PedStats_Find(po_pedStats, reinterpret_cast<const char*>(self + 0x78));
    At<uint16_t>(mi, 0x5C) = At<uint16_t>(self, 0x114);
    At<int32_t>(mi, 0x60) = 0;
    PO_Ped_5226D0(mi, reinterpret_cast<const char*>(self + 0x118), reinterpret_cast<const char*>(self + 0x138));
    PO_Ped_5227D0(mi, reinterpret_cast<const char*>(self + 0x1B8), reinterpret_cast<const char*>(self + 0x1D8));
    TrackRange(po_firstPed, po_lastPed, Id(self));
    PO_Fn_425350();
    PO_AddModelName(Name(self), Id(self));
}

// 0x4373F0: the model section: its end (skipped to unless 0xBF3812), the model count,
// the 2D effect count and size; each model's id, kind and bytes, read over that kind's
// object, which makes the model info when there's none yet (its collision and the
// 0x51AFA0 object put on it either way), +0xA given `flag`; then the 2D effects
static uint8_t __cdecl LoadModelSection(IplStream* stream, int flag) {
    // the objects on the stack (only their vtables and the effect's +8..+0x10 set)
    alignas(4) uint8_t simple[0x58] = {}, time[0x7C] = {}, weapon[0x90] = {}, money[0x48] = {}, item[0x48] = {}, secondary[0x48] = {},
                       accessory[0x48] = {}, vehicle[0xF8] = {}, ped[0x258] = {}, effect[0x84] = {}, panim[0x8C] = {}, cloth[0x48] = {};
    uint8_t* objects[12] = {simple, time, weapon, money, item, secondary, accessory, vehicle, ped, effect, panim, cloth};
    static const uint32_t vtables[12] = {0x0090358C, 0x0090359C, 0x009035AC, 0x009035BC, 0x009035CC, 0x009035DC,
                                         0x009035EC, 0x009035FC, 0x0090360C, 0x0090361C, 0x0090362C, 0x0090363C};
    for (int i = 0; i < 12; i++) {
        At<uint32_t>(objects[i], 0) = vtables[i];
    }
    At<float>(effect, 8) = 0.0f;
    At<float>(effect, 0xC) = 0.0f;
    At<float>(effect, 0x10) = 0.0f;
    auto read = [stream](void* buffer, uint32_t size) {
        if (!stream->data) {
            PO_File_Read(stream->file, buffer, size);
        } else {
            std::memcpy(buffer, stream->data + stream->position, size);
            stream->position += size;
        }
    };
    uint32_t end = 0;
    read(&end, 4);
    if (!po_loadModelSection) {
        if (stream->data) {
            stream->position = end;
        } else {
            PO_File_Seek(stream->file, end, 0);
        }
        return 1;
    }
    int32_t modelCount = 0, effectCount = 0;
    uint32_t effectSize = 0;
    read(&modelCount, 4);
    read(&effectCount, 4);
    read(&effectSize, 4);
    for (int32_t i = 0; i < modelCount; i++) {
        int32_t id = 0, kind = 0;
        uint32_t size = 0;
        read(&id, 4);
        read(&kind, 4);
        read(&size, 4);
        read(objects[kind] + 4, size);
        void* col = PO_FindColModel(id);
        if (col) {
            PO_Fn_51CBD0(id);
        }
        void* other = PO_Fn_51AFA0(id);
        if (other) {
            PO_Fn_51C650(id);
        }
        if (!po_modelInfoPtrs[id]) {
            reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(objects[kind]))[2]))(objects[kind]);
        }
        uint8_t* mi = po_modelInfoPtrs[id];
        if (col) {
            PO_SetColModel(mi, col, 1);
        }
        if (other) {
            re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(mi);
            if (mi[8] == 1 || mi[8] == 3) {
                re::PtrCast<void(__thiscall*)(uint8_t*, void*)>(vtable[0x30 / 4])(mi, other);
            } else {
                re::PtrCast<void(__thiscall*)(uint8_t*, void*, int)>(vtable[0x34 / 4])(mi, other, 1);
            }
        }
        mi[0xA] = uint8_t(flag);
    }
    for (int32_t i = 0; i < effectCount; i++) {
        read(effect + 4, effectSize);
        PO_C2DEffectObject_Add(effect);
    }
    return 1;
}

#define PLACEOBJ_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

PLACEOBJ_REVERSED("BaseObject::vf0", 0x00435A80, &BaseObject_Destroy);
PLACEOBJ_REVERSED("C2DEffectObject::vf2", 0x00436310, &C2DEffectObject_Add);
PLACEOBJ_REVERSED("SimpleObject::vf1", 0x004363D0, &SimpleObject_Size);
PLACEOBJ_REVERSED("SimpleObject::vf2", 0x004363E0, &SimpleObject_Add);
PLACEOBJ_REVERSED("TimeObject::vf1", 0x00436520, &TimeObject_Size);
PLACEOBJ_REVERSED("TimeObject::vf2", 0x00436530, &TimeObject_Add);
PLACEOBJ_REVERSED("WeaponObject::vf1", 0x004365D0, &WeaponObject_Size);
PLACEOBJ_REVERSED("WeaponObject::vf2", 0x004365E0, &WeaponObject_Add);
PLACEOBJ_REVERSED("ItemObject::vf2", 0x00436660, &ItemObject_Add);
PLACEOBJ_REVERSED("SecondaryObject::vf2", 0x004366F0, &SecondaryObject_Add);
PLACEOBJ_REVERSED("AccessoryObject::vf2", 0x00436760, &AccessoryObject_Add);
PLACEOBJ_REVERSED("VehicleObject::vf1", 0x004367C0, &VehicleObject_Size);
PLACEOBJ_REVERSED("VehicleObject::vf2", 0x004367D0, &VehicleObject_Add);
PLACEOBJ_REVERSED("PedObject::vf1", 0x00436940, &PedObject_Size);
PLACEOBJ_REVERSED("C2DEffectObject::vf1", 0x00436950, &C2DEffectObject_Size);
PLACEOBJ_REVERSED("PAnimObject::vf1", 0x00436960, &PAnimObject_Size);
PLACEOBJ_REVERSED("PAnimObject::vf2", 0x00436970, &PAnimObject_Add);
PLACEOBJ_REVERSED("SecondaryObject::vf1", 0x00436A00, &SecondaryObject_Size);
PLACEOBJ_REVERSED("ClothObject::vf2", 0x00436A10, &ClothObject_Add);
PLACEOBJ_REVERSED("PedObject::vf2", 0x00437220, &PedObject_Add);
PLACEOBJ_REVERSED("IplLoaderImpl::LoadModelSection", 0x004373F0, &LoadModelSection);

} // namespace ModelObjects
