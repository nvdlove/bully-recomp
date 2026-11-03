// The player's clothing (CClothing, the one object at 0x20C5F70; 0x1C38 bytes): the 6 worn
// slots (HEAD, TORSO, the wrists, LEGS, FEET), the owned items (up to 375), the hair, the
// 35 outfit definitions (the last the "Custom Outfit"), the outfit worn, the tattoos (40),
// the iterators the store screens use, the style counts, the save block, the options read
// from Config\Dat\clthopin.dat and the rebuild of a ped's clump with new clothes.
// 0x6CA4B0..0x6CC2B7 (the catalogue and its loads are in ClothingCatalog.cpp).
//
// CClothing (by offset):
//   +0x0     6 worn items (12 each)        +0x48   375 owned items {hash, slot} (8 each)
//   +0xC00   the owned count               +0xC04  the hair (12)
//   +0xC10   35 outfits (0x54 each)        +0x178C the outfit worn (-1: none)
//   +0x1790  40 tattoos {hash, group, index} +0x1970 their count
//   +0x1974 / +0x1978 / +0x197C the owned / outfit / tattoo iterators
//   +0x1980  the worn items' +0xA sum      +0x1984 the tattoos shown
//   +0x1988  byte: the clothes changed (the ped is rebuilt on the next refresh)
//   +0x198C  the clthopin.dat options (11 per line)
//   +0x1C20 / +0x1C27 7 bytes each, +0x1C2E a byte, +0x1C30 a float, +0x1C34 a count

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

// a worn item (12): an outfit's slot, a catalogue entry
struct WornItem {
    uint32_t hash;    // +0 the model name hash
    uint32_t hash2;   // +4 the cutscene model's ("CS_<name>")
    int8_t slot;      // +8
    int8_t style;     // +9
    int8_t weight;    // +0xA (summed into +0x1980)
    uint8_t flags;    // +0xB (1 short sleeve, 2 long sleeve, 4 ungiveable, 8 special kiss,
                      //       0x10 bare arm, 0x20 initial clothing)
};
static_assert(sizeof(WornItem) == 12);

// an owned item (8): the name hash and the slot (the rest unused)
struct OwnedItem {
    uint32_t hash;
    int8_t slot;
    uint8_t unused[3];
};
static_assert(sizeof(OwnedItem) == 8);

// an outfit definition (0x54)
struct Outfit {
    uint32_t hash;        // +0
    uint32_t title;       // +4
    WornItem items[6];    // +8
    uint8_t flags;        // +0x50 (1 defined, 2 ..., 8 owned outright, 0x10 wearable)
    uint8_t unused[3];
};
static_assert(sizeof(Outfit) == 0x54);

struct Tattoo {
    uint32_t hash;
    int32_t group;
    int32_t index;
};
static_assert(sizeof(Tattoo) == 12);

// a clothing set (0x58): what's worn, the outfit, the hair (the store screens' backups)
struct ClothingSet {
    WornItem items[6];   // +0
    int32_t outfit;      // +0x48
    WornItem hair;       // +0x4C
};
static_assert(sizeof(ClothingSet) == 0x58);

struct Clothing {
    WornItem worn[6];            // +0x0
    OwnedItem owned[0x177];      // +0x48
    int32_t ownedCount;          // +0xC00
    WornItem hair;               // +0xC04
    Outfit outfits[35];          // +0xC10
    int32_t currentOutfit;       // +0x178C
    Tattoo tattoos[40];          // +0x1790
    int32_t tattooCount;         // +0x1970
    int32_t ownedCursor;         // +0x1974
    int32_t outfitCursor;        // +0x1978
    int32_t tattooCursor;        // +0x197C
    int32_t weightSum;           // +0x1980
    int32_t tattoosShown;        // +0x1984
    uint8_t changed;             // +0x1988
    uint8_t unknown1989[3];
    int32_t options[0x40];       // +0x198C
    uint8_t unknown1A8C[0x194];
    uint8_t bytes1C20[7];        // +0x1C20
    uint8_t bytes1C27[7];        // +0x1C27
    uint8_t byte1C2E;            // +0x1C2E
    uint8_t unknown1C2F;
    float float1C30;             // +0x1C30
    int32_t count1C34;           // +0x1C34
};
static_assert(offsetof(Clothing, ownedCount) == 0xC00);
static_assert(offsetof(Clothing, outfits) == 0xC10);
static_assert(offsetof(Clothing, currentOutfit) == 0x178C);
static_assert(offsetof(Clothing, tattooCount) == 0x1970);
static_assert(offsetof(Clothing, options) == 0x198C);
static_assert(offsetof(Clothing, bytes1C20) == 0x1C20);
static_assert(offsetof(Clothing, count1C34) == 0x1C34);
static_assert(sizeof(Clothing) == 0x1C38);

// the save block (0x1A8C)
struct ClothingSave {
    int32_t version;             // +0 (2)
    int32_t size;                // +4 (0x1D8C)
    int32_t slots;               // +8 (6)
    int32_t ownedMax;            // +0xC (0x177)
    int32_t outfitCount;         // +0x10 (0x23)
    int32_t tattooMax;           // +0x14 (0x28)
    WornItem worn[6];            // +0x18
    OwnedItem owned[0x177];      // +0x60
    int32_t ownedCount;          // +0xC18
    WornItem hair;               // +0xC1C
    Outfit outfits[35];          // +0xC28
    int32_t currentOutfit;       // +0x17A4
    Tattoo tattoos[40];          // +0x17A8
    int32_t tattooCount;         // +0x1988
    uint8_t block[0x100];        // +0x198C (checked only)
};
static_assert(offsetof(ClothingSave, ownedCount) == 0xC18);
static_assert(offsetof(ClothingSave, currentOutfit) == 0x17A4);
static_assert(offsetof(ClothingSave, tattooCount) == 0x1988);

// ---- game data ----
GAME_VAR(Clothing, pc_clothing, 0x020C5F70);
GAME_ARRAY(Outfit, pc_outfits, 0x020C6B80);          // pc_clothing.outfits
GAME_VAR(void*, pc_catalog, 0x020C5F68);              // the catalogue (ClothingCatalog.cpp)
GAME_VAR(uint32_t, pc_noneHead, 0x020C5F6C);          // the "nothing worn" items
GAME_VAR(uint32_t, pc_noneItem, 0x020C7BB0);
GAME_VAR(uint32_t, pc_outfitA, 0x020C7BB8);
GAME_VAR(uint32_t, pc_outfitB, 0x020C7BBC);
GAME_VAR(uint32_t, pc_outfitC, 0x020C7BC0);
GAME_VAR(uint32_t, pc_outfitD, 0x020C7BC4);
GAME_VAR(uint32_t, pc_skinBase, 0x020C7BC8);
GAME_VAR(uint32_t, pc_tankTop, 0x020C7BCC);
GAME_ARRAY(const char*, pc_tattooGroupTitles, 0x00942A00);   // CLT_L_ARM, CLT_R_ARM
inline const char* const pc_invalidText = reinterpret_cast<const char*>(0x00903168);   // "INVALID"
inline const re::Ptr32<const char>* const pc_slotNames = reinterpret_cast<const re::Ptr32<const char>*>(0x009429E4);
// the hats (NameKeys)
GAME_VAR(uint32_t, pc_hatMascot, 0x01B654A8);
GAME_VAR(uint32_t, pc_hatFries, 0x01B654AC);
GAME_VAR(uint32_t, pc_hatStrange, 0x01B654B0);
GAME_VAR(uint32_t, pc_hatCanada, 0x01B654B4);
GAME_VAR(uint32_t, pc_hatPirate, 0x01B654B8);
GAME_VAR(uint32_t, pc_hatDunce, 0x01B654BC);
GAME_VAR(uint32_t, pc_hatDM, 0x01B654C0);
GAME_VAR(uint32_t, pc_hatGnome, 0x01B654C4);
GAME_VAR(float, pc_hatHeightTall, 0x00942A1C);        // 1.7
GAME_VAR(float, pc_hatHeightMid, 0x0091C0E4);         // 1.35
GAME_VAR(float, pc_hatHeightLow, 0x00900DF8);         // 1.2
GAME_VAR(float, pc_minusOne, 0x009009AC);
GAME_VAR(void*, pc_inventoryHud, 0x00C67554);
GAME_ARRAY(uint8_t*, pc_modelInfos, 0x00C67738);
inline char* const pc_fileBuffer = reinterpret_cast<char*>(0x00BD7800);
inline void* const pc_animManager = reinterpret_cast<void*>(0x020C4910);
inline void* const pc_identityCallback = reinterpret_cast<void*>(0x006CA7D0);

GAME_FN(uint32_t, __cdecl, Pc_HashName, 0x00576ED0, const char* name);
GAME_FN(int, __cdecl, Pc_FindNameInList, 0x0073AC90, const re::Ptr32<const char>* names, int count, const char* name,
        int fallback, int);
GAME_FN(int, __cdecl, Pc_OptionIndex, 0x00488C90, const char* name);
template <typename... A>
inline int __cdecl Pc_Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
template <typename... A>
inline int __cdecl Pc_Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline void* __cdecl Pc_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(int, __cdecl, Pc_LoadFile, 0x0042D180, const char* path, char* buffer, int size, const char* mode);
GAME_FN(void, __stdcall, Pc_VectorConstruct, 0x00401240, void* array, uint32_t size, int count, void* ctor);
GAME_FN(void, __thiscall, Pc_NameKeyFromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(void, __thiscall, Pc_NameKeyDestruct, 0x0049A650, uint32_t* key);   // (empty)
GAME_FN(uint32_t*, __thiscall, Pc_CatalogFind, 0x006CC320, void* catalog, WornItem* out, const OwnedItem* item);
GAME_FN(uint32_t*, __thiscall, Pc_CatalogFindHair, 0x006CC3F0, void* catalog, WornItem* out, const uint32_t* hash);
GAME_FN(void, __thiscall, Pc_InventoryShowText, 0x00541170, void* hud, int slot, uint32_t hash);
GAME_FN(uint8_t, __cdecl, Pc_SaveCheckValue, 0x00733320, int, int, int value, int expected);
GAME_FN(uint8_t, __cdecl, Pc_SaveCheckBlock, 0x00733620, const char* name, void* data, int size);
GAME_FN(void, __thiscall, Pc_SaveCopyOwned, 0x00733410, OwnedItem* to, const OwnedItem* from);
GAME_FN(void, __thiscall, Pc_SaveCopyTattoo, 0x00733430, Tattoo* to, const Tattoo* from);
// (the functions of this file the game's own code calls through, kept as calls)
GAME_FN(WornItem*, __thiscall, Pc_CopyItem, 0x006CA530, WornItem* to, const WornItem* from);
GAME_FN(Outfit*, __thiscall, Pc_CopyOutfit, 0x006CA590, Outfit* to, const Outfit* from);
GAME_FN(void, __thiscall, Pc_SetFromOutfit, 0x006CA6E0, ClothingSet* set, int32_t outfit);
GAME_FN(uint8_t, __thiscall, Pc_PlayerOwns, 0x006CA850, Clothing* clothing, const OwnedItem* item);
GAME_FN(Outfit*, __thiscall, Pc_NextOutfit, 0x006CACC0, Clothing* clothing, int32_t* index);
GAME_FN(Outfit*, __thiscall, Pc_PrevOutfit, 0x006CAD20, Clothing* clothing, int32_t* index);
GAME_FN(void, __thiscall, Pc_Recount, 0x006CAF30, Clothing* clothing);
GAME_FN(void, __thiscall, Pc_GivePlayer, 0x006CB6C0, Clothing* clothing, const OwnedItem* item, int show);
GAME_FN(void, __thiscall, Pc_LoadOptions, 0x006CB3F0, Clothing* clothing);
// the ped rebuild
GAME_FN(uint8_t*, __cdecl, Pc_ClumpForAllAtomics, 0x006C7380, void* clump, void* callback, void* data);
GAME_FN(uint8_t*, __thiscall, Pc_BlendGetAnim, 0x006B1F50, uint8_t* blend, uint32_t name);
GAME_FN(void, __thiscall, Pc_Fn468740, 0x00468740, void* self);
GAME_FN(void, __thiscall, Pc_WeaponDetach, 0x004D9510, void* weapon);
GAME_FN(void, __thiscall, Pc_WeaponAttach, 0x004D9460, void* weapon);
GAME_FN(void, __thiscall, Pc_DeleteRwObject, 0x004667A0, uint8_t* entity);
GAME_FN(void, __cdecl, Pc_TxdAddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Pc_TxdRemoveRef, 0x005F1160, int slot);
GAME_FN(void, __thiscall, Pc_AddAnimRefs, 0x0050E960, uint8_t* modelInfo);
GAME_FN(void, __thiscall, Pc_RemoveAnimRefs, 0x0050EA40, uint8_t* modelInfo);
GAME_FN(void, __thiscall, Pc_FindLimbNodes, 0x00472FB0, uint8_t* ped);
GAME_FN(int, __thiscall, Pc_StartTrack, 0x006C0D40, void* manager, uint8_t* model, uintptr_t anim, uintptr_t track, int flags,
        float a, float b);
GAME_FN(int, __thiscall, Pc_SetPlayerTime, 0x006C0E90, void* manager, uint8_t* model, uintptr_t track, int player,
        float time, int, int);

inline void ClearItem(WornItem& item) {
    item.hash = 0;
    item.hash2 = 0;
    item.style = -1;
    item.slot = -1;
    item.flags = 0;
}

// the outfit worn (null: none)
inline Outfit* CurrentOutfit(Clothing* self) {
    int32_t index = self->currentOutfit;
    if (index == -1) {
        return nullptr;
    }
    return &self->outfits[index];
}

// 0x6CA4B0: a tattoo group's title text key
const char* __cdecl TattooGroupTitle(uint32_t group) {
    if (group > 1) {
        return pc_invalidText;
    }
    return pc_tattooGroupTitles[group];
}

// 0x6CA4D0: a slot by name (HEAD, TORSO, ...; -1 none)
int __cdecl SlotFromName(const char* name) {
    return Pc_FindNameInList(pc_slotNames, 6, name, -1, 0);
}

// 0x6CA4F0: an owned item set (eax: the hash)
uint32_t __fastcall OwnedItem_Set(OwnedItem* self, re::Edx, uint32_t hash, int slot) {
    self->hash = hash;
    self->slot = static_cast<int8_t>(slot);
    return hash;
}

// 0x6CA500
void __fastcall WornItem_Set(WornItem* self, re::Edx, uint32_t hash, uint32_t hash2, int slot, int style, int weight,
                             int flags) {
    self->hash = hash;
    self->hash2 = hash2;
    self->style = static_cast<int8_t>(style);
    self->slot = static_cast<int8_t>(slot);
    self->weight = static_cast<int8_t>(weight);
    self->flags = static_cast<uint8_t>(flags);
}

// 0x6CA530
WornItem* __fastcall WornItem_Copy(WornItem* self, re::Edx, const WornItem* from) {
    self->hash = from->hash;
    self->hash2 = from->hash2;
    self->slot = from->slot;
    self->style = from->style;
    self->weight = from->weight;
    self->flags = from->flags;
    return self;
}

// 0x6CA560: a tattoo set from its name
void __fastcall Tattoo_Set(Tattoo* self, re::Edx, const char* name, int32_t group, int32_t index) {
    uint32_t hash = Pc_HashName(name);
    self->hash = hash;
    self->group = group;
    self->index = index;
}

// 0x6CA590: an outfit copied (the flags' low 5 bits only)
Outfit* __fastcall Outfit_Copy(Outfit* self, re::Edx, const Outfit* from) {
    self->hash = from->hash;
    self->title = from->title;
    for (int i = 0; i < 6; ++i) {
        self->items[i].hash = from->items[i].hash;
        self->items[i].hash2 = from->items[i].hash2;
        self->items[i].slot = from->items[i].slot;
        self->items[i].style = from->items[i].style;
        self->items[i].weight = from->items[i].weight;
        self->items[i].flags = from->items[i].flags;
    }
    self->flags = static_cast<uint8_t>((self->flags & 0xE0) | (from->flags & 0x1F));
    return self;
}

// 0x6CA6E0: a set made to the outfit (its items only if the outfit is defined and
// wearable)
void __fastcall ClothingSet_SetFromOutfit(ClothingSet* self, re::Edx, int32_t outfit) {
    self->outfit = outfit;
    if (outfit < 0) {
        return;
    }
    Outfit copy;
    std::memcpy(&copy, &pc_outfits[outfit], sizeof(Outfit));
    if ((copy.flags & 1) == 0 || (copy.flags & 0x10) == 0) {
        return;
    }
    std::memcpy(self->items, copy.items, sizeof(copy.items));
}

// 0x6CA7D0: the clump's atomics left as they are (a RpClumpForAllAtomics callback)
void* __cdecl Identity(void* atomic) {
    return atomic;
}

// 0x6CA7E0: an owned item taken away (the rest moved down)
void __fastcall RemovePlayer(Clothing* self, re::Edx, const OwnedItem* item) {
    bool found = false;
    for (int i = 0; i < self->ownedCount; ++i) {
        OwnedItem& entry = self->owned[i];
        if (entry.hash == item->hash && item->slot == entry.slot) {
            found = true;
        } else if (!found) {
            continue;
        }
        if (i < self->ownedCount - 1) {
            entry.hash = self->owned[i + 1].hash;
            std::memcpy(&entry.slot, &self->owned[i + 1].slot, 4);
        }
    }
    if (found) {
        self->ownedCount += -1;
    }
}

// 0x6CA850
uint8_t __fastcall PlayerOwns(Clothing* self, re::Edx, const OwnedItem* item) {
    int count = self->ownedCount;
    for (int i = 0; i < count; ++i) {
        if (self->owned[i].hash == item->hash && self->owned[i].slot == item->slot) {
            return 1;
        }
    }
    return 0;
}

// 0x6CA890: the next owned item of a slot (the iterator at +0x1974; null at the end)
OwnedItem* __fastcall NextOwned(Clothing* self, re::Edx, int slot, int32_t* outIndex) {
    for (;;) {
        int32_t cursor = self->ownedCursor;
        if (cursor < 0 || cursor >= self->ownedCount || self->ownedCount <= 0) {
            self->ownedCursor = -1;
            return nullptr;
        }
        if (outIndex != nullptr) {
            *outIndex = cursor;
        }
        OwnedItem* entry = &self->owned[self->ownedCursor];
        self->ownedCursor = self->ownedCursor + 1;
        if (entry->slot == slot) {
            return entry;
        }
    }
}

// 0x6CA900: the hair set from the catalogue
void __fastcall SetPlayersHair(Clothing* self, re::Edx, uint32_t hash) {
    void* catalog = pc_catalog;
    if (catalog == nullptr) {
        return;
    }
    WornItem found;
    const WornItem* hair = reinterpret_cast<const WornItem*>(Pc_CatalogFindHair(catalog, &found, &hash));
    self->hair.hash = hair->hash;
    self->hair.hash2 = hair->hash2;
    std::memcpy(&self->hair.slot, &hair->slot, 4);
}

// 0x6CA940: what's worn stored as an outfit (the outfit worn's flag 2 carried over)
void __fastcall StoreWornAsOutfit(Clothing* self, re::Edx, int32_t index) {
    Outfit& outfit = self->outfits[index];
    std::memcpy(outfit.items, self->worn, sizeof(outfit.items));
    Outfit* current = CurrentOutfit(self);
    if (current != nullptr) {
        outfit.flags ^= (current->flags ^ outfit.flags) & 2;
    }
}

// 0x6CA9F0
uint8_t __fastcall IsWearingOutfit(Clothing* self, re::Edx, uint32_t hash) {
    Outfit* current = CurrentOutfit(self);
    return current != nullptr && current->hash == hash ? 1 : 0;
}

// 0x6CAA20: one of the three outfits at 0x20C7BB8 worn
uint8_t __fastcall IsWearingOutfitABC(Clothing* self, re::Edx) {
    Outfit* current = CurrentOutfit(self);
    if (current == nullptr) {
        return 0;
    }
    uint32_t hash = current->hash;
    return hash == pc_outfitA || hash == pc_outfitB || hash == pc_outfitC ? 1 : 0;
}

// 0x6CAA60
uint8_t __fastcall IsWearingOutfitD(Clothing* self, re::Edx) {
    Outfit* current = CurrentOutfit(self);
    return current != nullptr && current->hash == pc_outfitD ? 1 : 0;
}

// 0x6CAA90
Outfit* __fastcall FindOutfit(Clothing* self, re::Edx, uint32_t hash) {
    for (int i = 0; i < 35; ++i) {
        if (self->outfits[i].hash == hash) {
            return &self->outfits[i];
        }
    }
    return nullptr;
}

// 0x6CAAC0: a worn item of style 8
uint8_t __fastcall IsWearingStyle8(Clothing* self, re::Edx) {
    for (int i = 0; i < 6; ++i) {
        if (self->worn[i].style == 8) {
            return 1;
        }
    }
    return 0;
}

// 0x6CAAE0: the head's height scale for the hat worn (the camera's)
float __fastcall HatHeight(Clothing* self, re::Edx) {
    // (each hat compared through a NameKey made from the head item; they're destroyed
    // in reverse, as the game does)
    uint32_t keys[3];
    bool found;
    Pc_NameKeyFromValue(&keys[0], self->worn[0].hash);
    if (pc_hatFries == keys[0]) {
        found = true;
        Pc_NameKeyDestruct(&keys[0]);
    } else {
        Pc_NameKeyFromValue(&keys[1], self->worn[0].hash);
        found = pc_hatDunce == keys[1];
        Pc_NameKeyDestruct(&keys[1]);
        Pc_NameKeyDestruct(&keys[0]);
    }
    if (found) {
        return pc_hatHeightTall;
    }
    int made = 0;
    Pc_NameKeyFromValue(&keys[0], self->worn[0].hash);
    made = 1;
    found = pc_hatStrange == keys[0];
    if (!found) {
        Pc_NameKeyFromValue(&keys[1], self->worn[0].hash);
        made = 2;
        found = pc_hatGnome == keys[1];
        if (!found) {
            Pc_NameKeyFromValue(&keys[2], self->worn[0].hash);
            made = 3;
            found = pc_hatCanada == keys[2];
        }
    }
    for (int i = made - 1; i >= 0; --i) {
        Pc_NameKeyDestruct(&keys[i]);
    }
    if (found) {
        return pc_hatHeightMid;
    }
    Pc_NameKeyFromValue(&keys[0], self->worn[0].hash);
    made = 1;
    found = pc_hatMascot == keys[0];
    if (!found) {
        Pc_NameKeyFromValue(&keys[1], self->worn[0].hash);
        made = 2;
        found = pc_hatDM == keys[1];
        if (!found) {
            Pc_NameKeyFromValue(&keys[2], self->worn[0].hash);
            made = 3;
            found = pc_hatPirate == keys[2];
        }
    }
    for (int i = made - 1; i >= 0; --i) {
        Pc_NameKeyDestruct(&keys[i]);
    }
    if (found) {
        return pc_hatHeightLow;
    }
    return 1.0f;
}

// 0x6CACC0: the next defined outfit (the iterator at +0x1978; null at the end)
Outfit* __fastcall NextOutfit(Clothing* self, re::Edx, int32_t* outIndex) {
    int32_t i = self->outfitCursor;
    if (i >= 0 && i < 35) {
        for (; i < 35; ++i) {
            if (self->outfits[i].flags & 1) {
                if (outIndex != nullptr) {
                    *outIndex = i;
                }
                self->outfitCursor = i + 1;
                return &self->outfits[i];
            }
        }
    }
    self->outfitCursor = -1;
    return nullptr;
}

// 0x6CAD20: the previous defined outfit
Outfit* __fastcall PrevOutfit(Clothing* self, re::Edx, int32_t* outIndex) {
    int32_t i = self->outfitCursor;
    if (i >= 0 && i < 35) {
        for (; i >= 0; --i) {
            if (self->outfits[i].flags & 1) {
                if (outIndex != nullptr) {
                    *outIndex = i;
                }
                self->outfitCursor = i + -1;
                return &self->outfits[i];
            }
        }
    }
    self->outfitCursor = -1;
    return nullptr;
}

// 0x6CAD80: the next outfit owned outright (flag 8)
Outfit* __fastcall NextOwnedOutfit(Clothing* self, re::Edx, int32_t* outIndex) {
    for (Outfit* outfit = Pc_NextOutfit(self, outIndex); outfit != nullptr; outfit = Pc_NextOutfit(self, outIndex)) {
        if (outfit->flags & 8) {
            return outfit;
        }
    }
    return nullptr;
}

// 0x6CADB0
Outfit* __fastcall PrevOwnedOutfit(Clothing* self, re::Edx, int32_t* outIndex) {
    for (Outfit* outfit = Pc_PrevOutfit(self, outIndex); outfit != nullptr; outfit = Pc_PrevOutfit(self, outIndex)) {
        if (outfit->flags & 8) {
            return outfit;
        }
    }
    return nullptr;
}

// 0x6CADE0
void __fastcall AddTattoo(Clothing* self, re::Edx, const Tattoo* tattoo) {
    int32_t count = self->tattooCount;
    if (count >= 0x28) {
        return;
    }
    self->tattoos[count] = *tattoo;
    self->tattooCount = self->tattooCount + 1;
}

// 0x6CAE20: the next tattoo (the iterator at +0x197C; null at the end)
Tattoo* __fastcall NextTattoo(Clothing* self, re::Edx, int32_t* outIndex) {
    int32_t cursor = self->tattooCursor;
    if (cursor < 0 || cursor >= 0x28 || self->tattooCount <= 0 || cursor >= self->tattooCount) {
        self->tattooCursor = -1;
        return nullptr;
    }
    if (outIndex != nullptr) {
        *outIndex = cursor;
    }
    int32_t index = self->tattooCursor;
    self->tattooCursor = index + 1;
    return &self->tattoos[index];
}

// 0x6CAE80: what's worn kept in a set
void __fastcall Backup(Clothing* self, re::Edx, ClothingSet* set) {
    std::memcpy(set->items, self->worn, sizeof(set->items));
    Pc_SetFromOutfit(set, self->currentOutfit);
    set->hair = self->hair;
}

// 0x6CAF30: the worn items' weights summed and the tattoos shown counted (none under
// long sleeves; the group 2 ones not under short sleeves). The game's callers keep ecx
// across this (0x6CB720 does).
void __fastcall Recount(Clothing* self, re::Edx) {
    self->weightSum = 0;
    self->tattoosShown = 0;
    int32_t sum = self->worn[0].weight;
    self->weightSum = sum;
    int32_t flags = static_cast<int8_t>(self->worn[0].flags);
    self->weightSum = self->worn[1].weight + sum;
    flags = flags | 0x10 | static_cast<int8_t>(self->worn[1].flags);
    for (int i = 2; i < 6; ++i) {
        self->weightSum += self->worn[i].weight;
        flags |= static_cast<int8_t>(self->worn[i].flags);
    }
    for (int i = 0; i < self->tattooCount; ++i) {
        if (flags & 2) {
            return;
        }
        if ((flags & 1) == 0 || self->tattoos[i].index != 2) {
            self->tattoosShown += 1;
        }
    }
}

// 0x6CAFF0: the clothing written to the save
void __fastcall SaveTo(Clothing* self, re::Edx, ClothingSave* save) {
    save->version = 2;
    save->size = 0x1D8C;
    save->slots = 6;
    save->ownedMax = 0x177;
    save->outfitCount = 0x23;
    save->tattooMax = 0x28;
    for (int i = 0; i < 6; ++i) {
        Pc_CopyItem(&save->worn[i], &self->worn[i]);
    }
    for (int i = 0; i < 0x177; ++i) {
        Pc_SaveCopyOwned(&save->owned[i], &self->owned[i]);
    }
    save->ownedCount = self->ownedCount;
    Pc_CopyItem(&save->hair, &self->hair);
    for (int i = 0; i < 0x23; ++i) {
        Pc_CopyOutfit(&save->outfits[i], &self->outfits[i]);
    }
    save->currentOutfit = self->currentOutfit;
    for (int i = 0; i < 0x28; ++i) {
        Pc_SaveCopyTattoo(&save->tattoos[i], &self->tattoos[i]);
    }
    save->tattooCount = self->tattooCount;
}

// 0x6CB100: the clothing read from the save (nothing if its header or block is off)
void __fastcall LoadFrom(Clothing* self, re::Edx, ClothingSave* save) {
    if (!Pc_SaveCheckValue(0, 0, save->version, 2) || !Pc_SaveCheckValue(0, 0, save->size, 0x1D8C) ||
        !Pc_SaveCheckValue(0, 0, save->slots, 6) || !Pc_SaveCheckValue(0, 0, save->ownedMax, 0x177) ||
        !Pc_SaveCheckValue(0, 0, save->outfitCount, 0x23) || !Pc_SaveCheckValue(0, 0, save->tattooMax, 0x28) ||
        !Pc_SaveCheckBlock("ClothingSaveData", save->block, 0x100)) {
        return;
    }
    for (int i = 0; i < 6; ++i) {
        Pc_CopyItem(&self->worn[i], &save->worn[i]);
    }
    for (int i = 0; i < 0x177; ++i) {
        self->owned[i].hash = save->owned[i].hash;
        self->owned[i].slot = save->owned[i].slot;
    }
    self->ownedCount = save->ownedCount;
    self->hair.hash = save->hair.hash;
    self->hair.hash2 = save->hair.hash2;
    self->hair.slot = save->hair.slot;
    self->hair.style = save->hair.style;
    self->hair.weight = save->hair.weight;
    self->hair.flags = save->hair.flags;
    for (int i = 0; i < 0x23; ++i) {
        Pc_CopyOutfit(&self->outfits[i], &save->outfits[i]);
    }
    self->currentOutfit = save->currentOutfit;
    for (int i = 0; i < 0x28; ++i) {
        self->tattoos[i] = save->tattoos[i];
    }
    self->tattooCount = save->tattooCount;
    Pc_Recount(self);
}

// 0x6CB2F0: an owned item from a name
void __fastcall OwnedItem_FromName(OwnedItem* self, re::Edx, const char* name, int slot) {
    uint32_t hash = Pc_HashName(name);
    self->hash = hash;
    self->slot = static_cast<int8_t>(slot);
}

// 0x6CB310: an empty worn item
WornItem* __fastcall WornItem_Construct(WornItem* self, re::Edx) {
    self->hash = 0;
    self->hash2 = 0;
    self->style = -1;
    self->slot = -1;
    self->flags = 0;
    return self;
}

// 0x6CB330: a worn item from its model name (the cutscene model "CS_<name>", but the
// "nothing" items stand for themselves; no name: both 0)
void __fastcall WornItem_FromName(WornItem* self, re::Edx, const char* name, int slot, int style, int weight,
                                  int flags) {
    uint32_t hash = Pc_HashName(name);
    uint32_t hash2;
    if (name == nullptr || hash == 0 || std::strlen(name) == 0) {
        hash = 0;
        hash2 = 0;
    } else if (hash == pc_noneHead) {
        hash = pc_noneHead;
        hash2 = hash;
    } else if (hash == pc_noneItem) {
        hash = pc_noneItem;
        hash2 = hash;
    } else {
        char buffer[0x40];
        Pc_Sprintf(buffer, "CS_%s", name);
        hash2 = Pc_HashName(buffer);
    }
    self->hash2 = hash2;
    self->style = static_cast<int8_t>(style);
    self->hash = hash;
    self->slot = static_cast<int8_t>(slot);
    self->weight = static_cast<int8_t>(weight);
    self->flags = static_cast<uint8_t>(flags);
}

// 0x6CB3D0: an empty tattoo
Tattoo* __fastcall Tattoo_Construct(Tattoo* self, re::Edx) {
    self->hash = 0;
    self->group = -1;
    self->index = -1;
    return self;
}

// a text line of a Config\Dat file read into line (\r , and tabs made spaces); the
// line's length
int ReadDatLine(int& pos, int size, char* line) {
    char c = pc_fileBuffer[pos];
    int n = 0;
    if (c != '\n') {
        while (pos < size) {
            line[n] = c == '\r' || c == ',' || c == '\t' ? ' ' : c;
            c = pc_fileBuffer[pos + 1];
            ++pos;
            ++n;
            if (c == '\n') {
                break;
            }
        }
    }
    ++pos;
    line[n] = '\0';
    return n;
}

// whether a line read holds anything (not blank, not a # comment); the game's scan runs
// on past the end of a blank line as it does
bool DatLineUsed(const char* line, int length) {
    int i = 0;
    while (static_cast<signed char>(line[i]) <= ' ') {
        ++i;
    }
    return i != length && line[i] != '#';
}

// 0x6CB3F0: Config\Dat\clthopin.dat: 11 option names a line, each stored as its index
void __fastcall LoadOptions(Clothing* self, re::Edx) {
    int size = Pc_LoadFile("Config\\Dat\\clthopin.dat", pc_fileBuffer, 0x1C000, "r");
    if (size <= 0) {
        return;
    }
    int32_t* out = self->options;
    char names[11][0x20];
    char line[0x100];
    int pos = 0;
    while (pos < size) {
        int length = ReadDatLine(pos, size, line);
        if (!DatLineUsed(line, length)) {
            continue;
        }
        Pc_Sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s", names[0], names[1], names[2], names[3], names[4],
                  names[5], names[6], names[7], names[8], names[9], names[10]);
        for (int i = 0; i < 11; ++i) {
            *out = Pc_OptionIndex(names[i]);
            ++out;
        }
    }
}

// 0x6CB670: an item put on (as the catalogue has it)
void __fastcall SetPlayer(Clothing* self, re::Edx, const OwnedItem* item) {
    void* catalog = pc_catalog;
    if (catalog == nullptr) {
        return;
    }
    WornItem found;
    WornItem& slot = self->worn[item->slot];
    const WornItem* entry = reinterpret_cast<const WornItem*>(Pc_CatalogFind(catalog, &found, item));
    slot.hash = entry->hash;
    slot.hash2 = entry->hash2;
    std::memcpy(&slot.slot, &entry->slot, 4);
    Pc_Recount(self);
}

// 0x6CB6C0: an item given (if not owned already; shown on the inventory HUD if asked)
void __fastcall GivePlayer(Clothing* self, re::Edx, const OwnedItem* item, int show) {
    int32_t count = self->ownedCount;
    if (count >= 0x177) {
        return;
    }
    if (Pc_PlayerOwns(self, item)) {
        return;
    }
    self->ownedCount = count + 1;
    uint32_t hash = item->hash;
    self->owned[count].hash = hash;
    std::memcpy(&self->owned[count].slot, &item->slot, 4);
    if (static_cast<uint8_t>(show) != 0) {
        Pc_InventoryShowText(pc_inventoryHud, self->owned[count].slot, hash);
    }
}

// 0x6CB720: an outfit put on (worn as that outfit unless asked not to)
void __fastcall SetPlayerOutfit(Clothing* self, re::Edx, uint32_t hash, int loose) {
    int i = 0;
    for (; i < 0x23; ++i) {
        if (self->outfits[i].hash == hash) {
            break;
        }
    }
    if (i == 0x23) {
        return;
    }
    std::memcpy(self->worn, self->outfits[i].items, sizeof(self->worn));
    Pc_Recount(self);
    if (static_cast<uint8_t>(loose) != 0) {
        self->currentOutfit = -1;
    } else {
        self->currentOutfit = i;
    }
}

// 0x6CB7E0: an outfit's items given (those not owned, not ungiveable; only the first if
// the outfit has flag 2)
void __fastcall GivePlayerOutfit(Clothing* self, re::Edx, Outfit* outfit, int show) {
    for (int k = 0; k < 6; ++k) {
        const WornItem& item = outfit->items[k];
        uint32_t hash = item.hash;
        int8_t slot = item.slot;
        bool owned = false;
        for (int i = 0; i < self->ownedCount; ++i) {
            if (self->owned[i].hash == hash && self->owned[i].slot == slot) {
                owned = true;
                break;
            }
        }
        if (owned) {
            continue;
        }
        if ((outfit->flags & 2) && k != 0) {
            continue;
        }
        if (item.flags & 4) {
            continue;
        }
        // (the game's item leaves the 3 bytes past the slot as the stack had them)
        OwnedItem give;
        give.slot = slot;
        give.hash = hash;
        Pc_GivePlayer(self, &give, show);
    }
}

// 0x6CB880: an outfit owned (outright, or each of its items; slot 0 items not checked)
uint8_t __fastcall OwnsOutfit(Clothing* self, re::Edx, const Outfit* outfit) {
    if (outfit->flags & 8) {
        return 1;
    }
    for (int k = 0; k < 6; ++k) {
        OwnedItem item;
        int8_t slot = outfit->items[k].slot;
        item.hash = outfit->items[k].hash;
        item.slot = slot;
        if (slot == 0 || item.hash == 0) {
            continue;
        }
        if (!Pc_PlayerOwns(self, &item)) {
            return 0;
        }
    }
    return 1;
}

// 0x6CB8F0: the style the clothes add up to (the head, torso and leg/foot items' styles
// counted; styles 1, 2, 8 and 9 add to style 0; the most counted of 0 and 3..7, the later
// on a tie). In the game an empty item's style of -1 counts into the saved esi below the
// counts, so its caller (0x5A39F6) gets esi back moved on: not reproduced here.
int __fastcall StyleOfClothes(Clothing* self, re::Edx) {
    int32_t slots[12];   // slots[0]: the game's saved esi (style -1)
    int32_t* counts = slots + 1;
    Pc_Memset(counts, 0, 0x2C);
    counts[self->worn[0].style]++;
    counts[self->worn[1].style]++;
    counts[self->worn[4].style]++;
    counts[self->worn[5].style]++;
    counts[0] = counts[0] + (counts[9] + counts[8] + counts[2] + counts[1]);
    int best = 0;
    for (int i = 3; i <= 7; ++i) {
        if (!(counts[best] > counts[i])) {
            best = i;
        }
    }
    return best;
}

// 0x6CB9B0: what's worn put back from a set (the outfit's items if it names one)
void __fastcall Restore(Clothing* self, re::Edx, const ClothingSet* set) {
    int32_t outfit = set->outfit;
    self->currentOutfit = outfit;
    if (outfit < 0) {
        std::memcpy(self->worn, set->items, sizeof(self->worn));
    } else {
        Outfit* current = CurrentOutfit(self);
        if (current != nullptr && (current->flags & 1) && (current->flags & 0x10)) {
            std::memcpy(self->worn, current->items, sizeof(self->worn));
        }
    }
    self->hair.hash = set->hair.hash;
    self->hair.hash2 = set->hair.hash2;
    std::memcpy(&self->hair.slot, &set->hair.slot, 4);
    Pc_Recount(self);
}

// 0x6CBB00: an empty owned item
OwnedItem* __fastcall OwnedItem_Construct(OwnedItem* self, re::Edx) {
    self->hash = 0;
    self->slot = -1;
    return self;
}

// 0x6CBB10: CClothing's constructor
Clothing* __fastcall Construct(Clothing* self, re::Edx) {
    for (int i = 0; i < 6; ++i) {
        ClearItem(self->worn[i]);
    }
    for (int i = 0; i < 0x177; ++i) {
        self->owned[i].hash = 0;
        self->owned[i].slot = -1;
    }
    ClearItem(self->hair);
    for (int i = 0; i < 0x23; ++i) {
        Outfit& outfit = self->outfits[i];
        Pc_VectorConstruct(outfit.items, 0xC, 6, reinterpret_cast<void*>(0x006CB310));
        outfit.hash = 0;
        outfit.title = 0;
        outfit.flags = static_cast<uint8_t>((outfit.flags & 0xF2) | 0x10);
        for (int k = 0; k < 6; ++k) {
            ClearItem(outfit.items[k]);
        }
    }
    for (int i = 0; i < 0x28; ++i) {
        self->tattoos[i].hash = 0;
        self->tattoos[i].group = -1;
        self->tattoos[i].index = -1;
    }
    return self;
}

// 0x6CBCA0: CClothing reset (a new game): nothing worn or owned but the "nothing" items,
// the outfits cleared, the Custom Outfit made, clthopin.dat read
void __fastcall Init(Clothing* self, re::Edx) {
    for (int i = 0; i < 6; ++i) {
        ClearItem(self->worn[i]);
    }
    for (int i = 0; i < 0x177; ++i) {
        self->owned[i].hash = 0;
        self->owned[i].slot = -1;
    }
    self->ownedCount = 0;
    for (int i = 0; i < 0x28; ++i) {
        self->tattoos[i].hash = 0;
        self->tattoos[i].group = -1;
        self->tattoos[i].index = -1;
    }
    self->tattooCount = 0;
    for (int i = 0; i < 0x23; ++i) {
        Outfit& outfit = self->outfits[i];
        outfit.hash = 0;
        outfit.title = 0;
        outfit.flags = static_cast<uint8_t>((outfit.flags & 0xF2) | 0x10);
        for (int k = 0; k < 6; ++k) {
            ClearItem(outfit.items[k]);
        }
    }
    self->currentOutfit = -1;
    Outfit& custom = self->outfits[34];
    custom.hash = Pc_HashName("Custom Outfit");
    custom.title = Pc_HashName("CLT_CUS_OFIT");
    custom.flags = static_cast<uint8_t>((custom.flags & 0xED) | 0xD);
    ClearItem(self->hair);
    self->count1C34 = 1;
    self->owned[0].hash = pc_noneHead;
    self->owned[0].slot = 0;
    int32_t n = self->count1C34;
    self->count1C34 = n + 1;
    self->owned[n].hash = pc_noneItem;
    self->owned[n].slot = 2;
    n = self->count1C34;
    self->count1C34 = n + 1;
    self->owned[n].hash = pc_noneItem;
    self->owned[n].slot = 3;
    self->ownedCount = self->count1C34;
    for (int i = 0; i < 7; ++i) {
        self->bytes1C20[i] = 0;
        self->bytes1C27[i] = 0;
    }
    self->float1C30 = pc_minusOne;
    self->byte1C2E = 1;
    pc_tankTop = Pc_HashName("tanktop");
    pc_skinBase = Pc_HashName("skinbase");
    Pc_LoadOptions(self);
    self->weightSum = 0;
}

// 0x6CBF10: a ped's clump swapped for one with new clothes (its anim kept going, its
// weapon taken off and put back, its limbs found again)
void __fastcall RebuildPed(Clothing*, re::Edx, uint8_t* ped, void* clump) {
    uint8_t* modelInfo = pc_modelInfos[At<int16_t>(ped, 0x10E)];
    uint8_t special = (At<uint8_t>(ped, 0x108) & 7) == 3 ? 1 : 0;
    Pc_ClumpForAllAtomics(clump, pc_identityCallback, nullptr);
    int32_t flags = 0;
    int32_t anim = 0;
    float time = 0.0f;
    uint8_t* blend = Pc_BlendGetAnim(At<uint8_t*>(ped, 0x114), 0);
    if (blend != nullptr) {
        uint8_t* animData = At<uint8_t*>(blend, 0x18);
        anim = animData != nullptr ? At<uint16_t>(animData, 0x32) : -1;
        time = animData != nullptr ? At<float>(blend, 0x1C) : pc_minusOne;
        uint8_t looped = At<uint8_t>(blend, 0x28) & 1;
        if (looped) {
            flags = 1;
        }
        if (looped) {
            flags |= 2;
        }
    }
    if (special == 1) {
        Pc_Fn468740(At<void*>(ped, 0x2E4));
    }
    uint8_t detached = 0;
    if (At<int32_t>(ped, 0xEC) == 0) {
        Pc_WeaponDetach(At<void*>(ped, 0x1D0));
        detached = 1;
    }
    Pc_DeleteRwObject(ped);
    Pc_TxdAddRef(At<int16_t>(modelInfo, 0x16));
    Pc_AddAnimRefs(modelInfo);
    vc::Call<void>(modelInfo, 0xC);
    vc::Call<void>(modelInfo, 0x34, clump, 1);
    Pc_RemoveAnimRefs(modelInfo);
    Pc_TxdRemoveRef(At<int16_t>(modelInfo, 0x16));
    vc::Call<void>(ped, 0x28, 1, 1);
    if (special == 1) {
        Pc_FindLimbNodes(ped);
    }
    if (detached) {
        Pc_WeaponAttach(At<void*>(ped, 0x1D0));
    }
    if (blend == nullptr) {
        return;
    }
    uint8_t* model = At<uint8_t*>(ped, 0x114);
    uint8_t* anims = At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10);
    if (anims == nullptr || static_cast<uint32_t>(anim) >= At<uint32_t>(anims, 4)) {
        return;
    }
    uint8_t* entry = At<uint8_t*>(anims, 0) + (static_cast<uint32_t>(anim) << 6);
    if (entry == nullptr || At<int32_t>(entry, 0) == 0) {
        return;
    }
    Pc_StartTrack(pc_animManager, model, anim, 0, flags, 1.0f, pc_minusOne);
    Pc_SetPlayerTime(pc_animManager, At<uint8_t*>(ped, 0x114), 0, 0, time, 0, 0);
}

// 0x6CC0D0: an outfit defined (the script's; in the first free place, its items from the
// catalogue by name; nothing if all 35 are taken)
void __fastcall DefineOutfit(Clothing* self, re::Edx, const char* name, const char* title, const char* head,
                             const char* torso, const char* leftWrist, const char* rightWrist, const char* legs,
                             const char* feet, int, int flag2) {
    if (pc_catalog == nullptr) {
        return;
    }
    int index = 0;
    for (; index < 0x23; ++index) {
        if ((self->outfits[index].flags & 1) == 0) {
            break;
        }
    }
    if (index == 0x23) {
        return;
    }
    Outfit& outfit = self->outfits[index];
    outfit.hash = Pc_HashName(name);
    outfit.title = Pc_HashName(title);
    outfit.flags = static_cast<uint8_t>(((flag2 & 1) * 2) | (outfit.flags & 0xF1) | 0x11);
    const char* names[6] = {head, torso, leftWrist, rightWrist, legs, feet};
    for (int k = 0; k < 6; ++k) {
        OwnedItem item;
        item.hash = Pc_HashName(names[k]);
        item.slot = static_cast<int8_t>(k);
        WornItem found;
        const WornItem* entry = reinterpret_cast<const WornItem*>(Pc_CatalogFind(pc_catalog, &found, &item));
        outfit.items[k].hash = entry->hash;
        outfit.items[k].hash2 = entry->hash2;
        std::memcpy(&outfit.items[k].slot, &entry->slot, 4);
    }
}

} // namespace

#define PCLOTH_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PCLOTH_REVERSED("CClothing::TattooGroupTitle", 0x006CA4B0, &TattooGroupTitle);
PCLOTH_REVERSED("CClothing::SlotFromName", 0x006CA4D0, &SlotFromName);
PCLOTH_REVERSED("CClothingOwnedItem::Set", 0x006CA4F0, &OwnedItem_Set);
PCLOTH_REVERSED("CClothingItem::Set", 0x006CA500, &WornItem_Set);
PCLOTH_REVERSED("CClothingItem::Copy", 0x006CA530, &WornItem_Copy);
PCLOTH_REVERSED("CClothingTattoo::Set", 0x006CA560, &Tattoo_Set);
PCLOTH_REVERSED("CClothingOutfit::Copy", 0x006CA590, &Outfit_Copy);
PCLOTH_REVERSED("CClothingSet::SetFromOutfit", 0x006CA6E0, &ClothingSet_SetFromOutfit);
PCLOTH_REVERSED("CClothing::IdentityAtomicCB", 0x006CA7D0, &Identity);
PCLOTH_REVERSED("CClothing::RemovePlayer", 0x006CA7E0, &RemovePlayer);
PCLOTH_REVERSED("CClothing::PlayerOwns", 0x006CA850, &PlayerOwns);
PCLOTH_REVERSED("CClothing::NextOwned", 0x006CA890, &NextOwned);
PCLOTH_REVERSED("CClothing::SetPlayersHair", 0x006CA900, &SetPlayersHair);
PCLOTH_REVERSED("CClothing::StoreWornAsOutfit", 0x006CA940, &StoreWornAsOutfit);
PCLOTH_REVERSED("CClothing::IsWearingOutfit", 0x006CA9F0, &IsWearingOutfit);
PCLOTH_REVERSED("CClothing::IsWearingOutfitABC", 0x006CAA20, &IsWearingOutfitABC);
PCLOTH_REVERSED("CClothing::IsWearingOutfitD", 0x006CAA60, &IsWearingOutfitD);
PCLOTH_REVERSED("CClothing::FindOutfit", 0x006CAA90, &FindOutfit);
PCLOTH_REVERSED("CClothing::IsWearingStyle8", 0x006CAAC0, &IsWearingStyle8);
PCLOTH_REVERSED("CClothing::HatHeight", 0x006CAAE0, &HatHeight);
// (0x6CAD80 and 0x6CADB0 keep ecx across these two in the game)
PCLOTH_REVERSED("CClothing::NextOutfit", 0x006CACC0, &NextOutfit);
PCLOTH_REVERSED("CClothing::PrevOutfit", 0x006CAD20, &PrevOutfit);
PCLOTH_REVERSED("CClothing::NextOwnedOutfit", 0x006CAD80, &NextOwnedOutfit);
PCLOTH_REVERSED("CClothing::PrevOwnedOutfit", 0x006CADB0, &PrevOwnedOutfit);
PCLOTH_REVERSED("CClothing::AddTattoo", 0x006CADE0, &AddTattoo);
PCLOTH_REVERSED("CClothing::NextTattoo", 0x006CAE20, &NextTattoo);
PCLOTH_REVERSED("CClothing::Backup", 0x006CAE80, &Backup);
// (regcheck2: 0x6CB720 keeps ecx across this in the game, call at 0x6CB7AE)
PCLOTH_REVERSED("CClothing::Recount", 0x006CAF30, &Recount);
PCLOTH_REVERSED("CClothing::SaveTo", 0x006CAFF0, &SaveTo);
PCLOTH_REVERSED("CClothing::LoadFrom", 0x006CB100, &LoadFrom);
PCLOTH_REVERSED("CClothingOwnedItem::FromName", 0x006CB2F0, &OwnedItem_FromName);
PCLOTH_REVERSED("CClothingItem::Construct", 0x006CB310, &WornItem_Construct);
PCLOTH_REVERSED("CClothingItem::FromName", 0x006CB330, &WornItem_FromName);
PCLOTH_REVERSED("CClothingTattoo::Construct", 0x006CB3D0, &Tattoo_Construct);
PCLOTH_REVERSED("CClothing::LoadOptions", 0x006CB3F0, &LoadOptions);
PCLOTH_REVERSED("CClothing::SetPlayer", 0x006CB670, &SetPlayer);
PCLOTH_REVERSED("CClothing::GivePlayer", 0x006CB6C0, &GivePlayer);
PCLOTH_REVERSED("CClothing::SetPlayerOutfit", 0x006CB720, &SetPlayerOutfit);
PCLOTH_REVERSED("CClothing::GivePlayerOutfit", 0x006CB7E0, &GivePlayerOutfit);
PCLOTH_REVERSED("CClothing::OwnsOutfit", 0x006CB880, &OwnsOutfit);
// (never hooked: the game's version moves its caller's esi on for an empty item, which
// C++ can't do; see above)
RE_UNHOOKED("CClothing::StyleOfClothes", 0x006CB8F0);
[[maybe_unused]] static const auto styleOfClothesReference = &StyleOfClothes;
PCLOTH_REVERSED("CClothing::Restore", 0x006CB9B0, &Restore);
PCLOTH_REVERSED("CClothingOwnedItem::Construct", 0x006CBB00, &OwnedItem_Construct);
PCLOTH_REVERSED("CClothing::Construct", 0x006CBB10, &Construct);
PCLOTH_REVERSED("CClothing::Init", 0x006CBCA0, &Init);
PCLOTH_REVERSED("CClothing::RebuildPed", 0x006CBF10, &RebuildPed);
PCLOTH_REVERSED("CClothing::DefineOutfit", 0x006CC0D0, &DefineOutfit);
