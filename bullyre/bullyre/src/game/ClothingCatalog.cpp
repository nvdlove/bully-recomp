// The clothing catalogue (the 8-byte object at 0x20C5F68, alive while a clothing, tattoo or
// barber store is set up): the items of Config\Dat\Cloths.dat by slot (the 6 slots and
// the hair), the store HUDs made and freed with it, the lookups by name hash, and the
// dressing of a ped (its clump rebuilt by the model builder at 0x20C7C1C from the worn
// items, the hair and the tattoos). 0x6CA7B0..0x6CA7C5, 0x6CB550..0x6CB66D and
// 0x6CC2C0..0x6CCDAF.
//
// the catalogue: +0 the 7 slots' item lists (heap, 12-byte items), +4 their counts
// (heap, 7 int16)

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

// a catalogue item (CClothingItem, 12; see PlayerClothing.cpp)
struct CatalogItem {
    uint32_t hash;
    uint32_t hash2;
    int8_t slot;
    int8_t style;
    int8_t weight;
    uint8_t flags;
};
static_assert(sizeof(CatalogItem) == 12);

struct Catalog {
    re::Ptr32<re::Ptr32<CatalogItem>> lists;   // +0 [7]
    re::Ptr32<int16_t> counts;       // +4 [7]
};
static_assert(sizeof(Catalog) == 8);

constexpr int kHair = 6;

// ---- game data ----
GAME_VAR(Catalog*, ccat_catalog, 0x020C5F68);
inline void* const ccat_clothing = reinterpret_cast<void*>(0x020C5F70);
GAME_VAR(uint32_t, ccat_noneHead, 0x020C5F6C);
GAME_VAR(uint32_t, ccat_noneItem, 0x020C7BB0);
GAME_VAR(int32_t, ccat_giveableCount, 0x020C7BA8);   // the giveable items in Cloths.dat
GAME_VAR(int32_t, ccat_initialCount, 0x020C7BAC);    // of them, the INITIALCLOTHING ones
GAME_VAR(float, ccat_savedHealth, 0x020C7BA0);       // (-1: none)
GAME_VAR(uint8_t, ccat_refreshing, 0x020C7BD0);
GAME_VAR(uint8_t, ccat_clothesChanged, 0x020C78F8);   // CClothing +0x1988
GAME_VAR(int32_t, ccat_tattooCursor, 0x020C78EC);     // CClothing +0x197C
GAME_ARRAY(CatalogItem, ccat_worn, 0x020C5F70);       // CClothing +0: the 6 worn items
GAME_VAR(CatalogItem, ccat_hair, 0x020C6B74);         // CClothing +0xC04
GAME_VAR(uint8_t*, ccat_playerPed, 0x00C1AEA8);
GAME_ARRAY(uint8_t*, ccat_modelInfos, 0x00C67738);
GAME_VAR(uint8_t*, ccat_txdPool, 0x01B64654);         // CTxdStore's pool: +0 objects, +4 flags, +0xC size
GAME_VAR(uint32_t, ccat_armourHead, 0x01B6548C);
GAME_VAR(float, ccat_minusOne, 0x009009AC);
inline char* const ccat_fileBuffer = reinterpret_cast<char*>(0x00BD7800);
inline void* const ccat_modelBuilder = reinterpret_cast<void*>(0x020C7C1C);
// the slots' names for the model builder
inline const char* const ccat_partHead = reinterpret_cast<const char*>(0x00942B80);     // "head"
inline const char* const ccat_partTorso = reinterpret_cast<const char*>(0x00942B78);    // "torso"
inline const char* const ccat_partLWrist = reinterpret_cast<const char*>(0x00942B70);   // "lwrist"
inline const char* const ccat_partRWrist = reinterpret_cast<const char*>(0x00942B68);   // "rwrist"
inline const char* const ccat_partLegs = reinterpret_cast<const char*>(0x00942B60);     // "legs"
inline const char* const ccat_partFeet = reinterpret_cast<const char*>(0x00942B58);     // "feet"

GAME_FN(void*, __cdecl, Ccat_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Ccat_Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, Ccat_EmptyDestruct, 0x0049A650, void* self);
GAME_FN(int, __cdecl, Ccat_LoadFile, 0x0042D180, const char* path, char* buffer, int size, const char* mode);
template <typename... A>
inline int __cdecl Ccat_Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline int __cdecl Ccat_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(int, __cdecl, Ccat_StyleIndex, 0x00488C70, const char* name);
GAME_FN(int32_t, __cdecl, Ccat_TxdFindSeasonal, 0x005F2450, uint32_t name, int);
GAME_FN(void, __cdecl, Ccat_TxdSetFlag, 0x005F1030, int slot, int value);
GAME_FN(uint8_t*, __thiscall, Ccat_RgbaConstruct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __thiscall, Ccat_NameKeyFromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(void, __thiscall, Ccat_NameKeyDestruct, 0x0049A650, uint32_t* key);
GAME_FN(void, __thiscall, Ccat_PedSetHealth, 0x004782E0, uint8_t* ped, float health);
GAME_FN(uint8_t, __cdecl, Ccat_Fn43AC10, 0x0043AC10);
GAME_FN(void, __cdecl, Ccat_Fn43BE40, 0x0043BE40, int state);
GAME_FN(void, __cdecl, Ccat_Fn45B7B0, 0x0045B7B0);
GAME_FN(void, __cdecl, Ccat_Fn45B7D0, 0x0045B7D0);
GAME_FN(void*, __thiscall, Ccat_BuildClump, 0x006D1A60, void* builder, uintptr_t model, void* txd,
        const uint32_t* items, const uint32_t* items2, const re::Ptr32<const char>* parts, int count, const uint32_t* tattoos,
        int tattooCount, void* extra, const uint8_t* colour);
GAME_FN(void, __thiscall, Ccat_BuildPedClump, 0x006D1840, void* builder, uint8_t* ped, uintptr_t model,
        void* txd, const uint32_t* items, const uint32_t* items2, const re::Ptr32<const char>* parts, int count,
        const uint32_t* tattoos, int tattooCount, void* extra, const uint8_t* colour);
// CClothing's (PlayerClothing.cpp)
GAME_FN(CatalogItem*, __thiscall, Ccat_ItemConstruct, 0x006CB310, CatalogItem* item);
GAME_FN(void, __thiscall, Ccat_ItemSet, 0x006CA500, CatalogItem* item, uint32_t hash, uint32_t hash2, int slot,
        int style, int weight, int flags);
GAME_FN(void, __thiscall, Ccat_ItemFromName, 0x006CB330, CatalogItem* item, const char* name, int slot, int style,
        int weight, int flags);
GAME_FN(int, __cdecl, Ccat_SlotFromName, 0x006CA4D0, const char* name);
GAME_FN(uint32_t*, __thiscall, Ccat_NextTattoo, 0x006CAE20, void* clothing, int32_t* index);
GAME_FN(void, __thiscall, Ccat_RebuildPed, 0x006CBF10, void* clothing, uint8_t* ped, void* clump);
// this file's, kept as calls
GAME_FN(Catalog*, __thiscall, Ccat_Construct, 0x006CC2D0, Catalog* self);
GAME_FN(void, __thiscall, Ccat_FreeLists, 0x006CC2E0, Catalog* self);
GAME_FN(void, __thiscall, Ccat_Load, 0x006CC450, Catalog* self);
GAME_FN(void, __thiscall, Ccat_DressPed, 0x006CC7C0, Catalog* self, uint8_t* ped);
GAME_FN(void, __thiscall, Ccat_Refresh, 0x006CCD90, Catalog* self);
// the store HUDs
GAME_FN(void*, __thiscall, Ccat_BarberConstruct, 0x006CCF20, void* self);
GAME_FN(void*, __thiscall, Ccat_ClothingHudConstruct, 0x006CDD70, void* self);
GAME_FN(void*, __thiscall, Ccat_TattooHudConstruct, 0x006CFEE0, void* self);
GAME_FN(void, __cdecl, Ccat_SetBarberHud, 0x0053A580, void* hud);
GAME_FN(void, __cdecl, Ccat_SetClothingHud, 0x0053CCB0, void* hud);
GAME_FN(void, __cdecl, Ccat_SetTattooHud, 0x0055AF80, void* hud);

// 0x6CA7B0: the player redressed if the clothes changed (with a catalogue)
void __fastcall RefreshIfLoaded(void*, re::Edx) {
    Catalog* catalog = ccat_catalog;
    if (catalog != nullptr) {
        Ccat_Refresh(catalog);
    }
}

// 0x6CA7C0
Catalog* __cdecl GetCatalog() {
    return ccat_catalog;
}

// 0x6CB550: the catalogue made (Cloths.dat read) with the three store HUDs
void __cdecl Begin() {
    void* memory = Ccat_Malloc(8);
    Catalog* catalog = memory != nullptr ? Ccat_Construct(static_cast<Catalog*>(memory)) : nullptr;
    Catalog* old = ccat_catalog;
    if (old != nullptr) {
        Ccat_EmptyDestruct(old);
        Ccat_Free(old);
    }
    ccat_catalog = catalog;
    memory = Ccat_Malloc(0x284);
    Ccat_SetBarberHud(memory != nullptr ? Ccat_BarberConstruct(memory) : nullptr);
    memory = Ccat_Malloc(0x2EA0);
    Ccat_SetClothingHud(memory != nullptr ? Ccat_ClothingHudConstruct(memory) : nullptr);
    memory = Ccat_Malloc(0x3F8);
    Ccat_SetTattooHud(memory != nullptr ? Ccat_TattooHudConstruct(memory) : nullptr);
    Ccat_Load(ccat_catalog);
}

// 0x6CB620: the catalogue and the store HUDs freed
void __cdecl End() {
    if (ccat_catalog != nullptr) {
        Ccat_FreeLists(ccat_catalog);
        Catalog* catalog = ccat_catalog;
        if (catalog != nullptr) {
            Ccat_EmptyDestruct(catalog);
            Ccat_Free(catalog);
        }
    }
    ccat_catalog = nullptr;
    Ccat_SetBarberHud(nullptr);
    Ccat_SetClothingHud(nullptr);
    Ccat_SetTattooHud(nullptr);
}

// 0x6CC2C0
void __cdecl SetRefreshing(int value) {
    ccat_refreshing = static_cast<uint8_t>(value);
}

// 0x6CC2D0
Catalog* __fastcall Construct(Catalog* self, re::Edx) {
    self->lists = nullptr;
    self->counts = nullptr;
    return self;
}

// 0x6CC2E0
void __fastcall FreeLists(Catalog* self, re::Edx) {
    for (int i = 0; i < 7; ++i) {
        Ccat_Free(self->lists[i]);
    }
    Ccat_Free(self->lists);
    Ccat_Free(self->counts);
    self->lists = nullptr;
    self->counts = nullptr;
}

// 0x6CC320: an item looked up in its slot's list (the "nothing" items made up; none:
// an empty item)
CatalogItem* __fastcall Find(Catalog* self, re::Edx, CatalogItem* out, const uint8_t* item) {
    uint32_t hash = At<uint32_t>(const_cast<uint8_t*>(item), 0);
    int8_t slot = static_cast<int8_t>(item[4]);
    if (slot >= 0 && slot < 6) {
        CatalogItem* list = self->lists[slot];
        int count = self->counts[slot];
        for (int i = 0; i < count; ++i) {
            if (list[i].hash == hash) {
                *out = list[i];
                return out;
            }
        }
        if (hash == ccat_noneItem || hash == ccat_noneHead) {
            CatalogItem made;
            Ccat_ItemConstruct(&made);
            Ccat_ItemSet(&made, hash, hash, slot, -1, 0, 0x20);
            *out = made;
            return out;
        }
    }
    Ccat_ItemConstruct(out);
    return out;
}

// 0x6CC3F0: a hair looked up
CatalogItem* __fastcall FindHair(Catalog* self, re::Edx, CatalogItem* out, const uint32_t* hash) {
    int count = self->counts[kHair];
    CatalogItem* list = self->lists[kHair];
    for (int i = 0; i < count; ++i) {
        if (list[i].hash == *hash) {
            *out = list[i];
            return out;
        }
    }
    Ccat_ItemConstruct(out);
    return out;
}

// a Config\Dat line read (as PlayerClothing.cpp's)
int ReadLine(int& pos, int size, char* line) {
    char c = ccat_fileBuffer[pos];
    int n = 0;
    if (c != '\n') {
        while (pos < size) {
            line[n] = c == '\r' || c == ',' || c == '\t' ? ' ' : c;
            c = ccat_fileBuffer[pos + 1];
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

bool LineUsed(const char* line, int length) {
    int i = 0;
    while (static_cast<signed char>(line[i]) <= ' ') {
        ++i;
    }
    return i != length && line[i] != '#';
}

// 0x6CC450: Config\Dat\Cloths.dat read: "name slot style weight [flags...]" a line
void __fastcall Load(Catalog* self, re::Edx) {
    const int16_t sizes[7] = {100, 130, 35, 35, 75, 75, 80};
    self->lists = static_cast<re::Ptr32<CatalogItem>*>(Ccat_Malloc(0x1C));
    self->counts = static_cast<int16_t*>(Ccat_Malloc(0xE));
    for (int k = 0; k < 7; ++k) {
        int n = sizes[k];
        uint64_t bytes = static_cast<uint64_t>(static_cast<uint32_t>(n)) * 12;
        uint32_t request = bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<uint32_t>(bytes);
        CatalogItem* list = static_cast<CatalogItem*>(Ccat_Malloc(request));
        if (list != nullptr) {
            CatalogItem* item = list;
            for (int j = n - 1; j >= 0; --j) {
                Ccat_ItemConstruct(item);
                ++item;
            }
        }
        self->lists[k] = list;
        self->counts[k] = 0;
    }
    int32_t giveable = 0;
    int32_t initial = 0;
    int size = Ccat_LoadFile("Config\\Dat\\Cloths.dat", ccat_fileBuffer, 0x1C000, "r");
    if (size <= 0) {
        ccat_initialCount = initial;
        ccat_giveableCount = giveable;
        return;
    }
    char line[0x100];
    int pos = 0;
    while (pos < size) {
        int length = ReadLine(pos, size, line);
        if (!LineUsed(line, length)) {
            continue;
        }
        int32_t weight;
        char slotName[0x20];
        char name[0x20];
        char style[0x20];
        char flagNames[3][0x20];
        int got = Ccat_Sscanf(line, "%s %s %s %d %s %s %s", name, slotName, style, &weight, flagNames[0],
                              flagNames[1], flagNames[2]);
        int slot = std::memcmp(slotName, "HAIR", 5) == 0 ? kHair : Ccat_SlotFromName(slotName);
        int flags = 0;
        for (int i = 0; i < got - 4; ++i) {
            const char* flag = flagNames[i];
            if (Ccat_Stricmp(flag, "BAREARM") == 0) {
                flags |= 0x10;
            } else if (Ccat_Stricmp(flag, "LONGSLEEVE") == 0) {
                flags |= 2;
            } else if (Ccat_Stricmp(flag, "SHORTSLEEVE") == 0) {
                flags |= 1;
            } else if (Ccat_Stricmp(flag, "UNGIVEABLE") == 0) {
                flags |= 4;
            } else if (Ccat_Stricmp(flag, "SPECIALKISS") == 0) {
                flags |= 8;
            } else if (Ccat_Stricmp(flag, "INITIALCLOTHING") == 0) {
                flags |= 0x20;
            }
        }
        int styleIndex = Ccat_StyleIndex(style);
        Ccat_ItemFromName(&self->lists[slot][self->counts[slot]], name, slot != kHair ? slot : 0, styleIndex, weight,
                          flags);
        Ccat_TxdFindSeasonal(self->lists[slot][self->counts[slot]].hash, -1);
        self->counts[slot] = static_cast<int16_t>(self->counts[slot] + 1);
        if (slot != kHair && (flags & 4) == 0) {
            ++giveable;
            if (flags & 0x20) {
                ++initial;
            }
        }
    }
    ccat_giveableCount = giveable;
    ccat_initialCount = initial;
}

// what a ped is dressed in: the 6 slots' models (the cutscene ones for any ped but the
// player; the hair for a bare head; no arm slots under long sleeves; none for the
// "nothing" items) and the tattoos
struct Dress {
    uint32_t models[6];
    uint32_t tattoos[40];
    int32_t tattooCount;
};

void GatherDress(uint8_t* ped, Dress& dress) {
    uint8_t other = ped != ccat_playerPed ? 1 : 0;
    ccat_tattooCursor = 0;
    int32_t count = 0;
    for (uint32_t* tattoo = Ccat_NextTattoo(ccat_clothing, nullptr); tattoo != nullptr;
         tattoo = Ccat_NextTattoo(ccat_clothing, nullptr)) {
        dress.tattoos[count] = *tattoo;
        ++count;
    }
    dress.tattooCount = count;
    uint8_t longSleeves = 0;
    for (int k = 0; k < 6; ++k) {
        dress.models[k] = other ? ccat_worn[k].hash2 : ccat_worn[k].hash;
        if (ccat_worn[k].flags & 2) {
            longSleeves = 1;
        }
    }
    if (ccat_worn[0].hash == ccat_noneHead) {
        dress.models[0] = other ? ccat_hair.hash2 : ccat_hair.hash;
    }
    if (longSleeves) {
        dress.models[2] = 0;
        dress.models[3] = 0;
    }
    uint32_t none = ccat_noneItem;
    for (int k = 0; k < 6; ++k) {
        if (ccat_worn[k].hash == none) {
            dress.models[k] = 0;
        }
    }
}

// the ped's model's texture dictionary (the game reads through a free slot's null)
void* PedTxd(uint8_t* ped, uint32_t& model) {
    uint8_t* modelInfo = ccat_modelInfos[At<int16_t>(ped, 0x10E)];
    int slot = At<int16_t>(modelInfo, 0x16);
    uint8_t* pool = ccat_txdPool;
    model = At<uint32_t>(modelInfo, 0x20);
    uint8_t* entry;
    if (At<uint8_t*>(pool, 4)[slot] & 0x80) {
        entry = nullptr;
    } else {
        entry = At<uint8_t*>(pool, 0) + At<int32_t>(pool, 0xC) * slot;
    }
    void* txd = re::LoadVolatile<void*>(entry);
    Ccat_TxdSetFlag(slot, 0);
    return txd;
}

// 0x6CC7C0: a ped dressed in the player's clothes (its clump rebuilt; the armour head
// raises its health by half, put back when it comes off)
void __fastcall DressPed(Catalog* self, re::Edx, uint8_t* ped) {
    uint8_t state = Ccat_Fn43AC10();
    re::Ptr32<const char> parts[6] = {ccat_partHead,  ccat_partTorso, ccat_partLWrist,
                            ccat_partRWrist, ccat_partLegs,  ccat_partFeet};
    if (self->lists == nullptr) {
        Ccat_Fn43BE40(state);
        ccat_refreshing = 0;
        return;
    }
    Ccat_Fn45B7B0();
    Dress dress;
    GatherDress(ped, dress);
    uint32_t model;
    void* txd = PedTxd(ped, model);
    uint8_t colour[4];
    Ccat_RgbaConstruct(colour, 0xFF, 0xFF, 0xFF, 0xFF);
    uint8_t extra[6];
    At<uint32_t>(extra, 0) = 0;
    At<uint16_t>(extra, 4) = 0;
    void* clump = Ccat_BuildClump(ccat_modelBuilder, model, txd, dress.models, dress.models, parts, 6, dress.tattoos,
                                  dress.tattooCount, extra, colour);
    if (clump != nullptr) {
        Ccat_RebuildPed(ccat_clothing, ped, clump);
        uint32_t key;
        Ccat_NameKeyFromValue(&key, dress.models[0]);
        uint8_t armour = ccat_armourHead == key ? 1 : 0;
        Ccat_NameKeyDestruct(&key);
        if (armour) {
            ccat_savedHealth = At<float>(ped, 0x1CA4);
            float raised = At<float>(ped, 0x1CA4) * 1.5f;
            At<float>(ped, 0x1CA4) = raised;
            float other = At<float>(ped, 0x1CB8);
            Ccat_PedSetHealth(ped, other < raised ? raised : other);
        } else {
            float none = ccat_minusOne;
            float saved = ccat_savedHealth;
            if (!(saved == none)) {
                At<float>(ped, 0x1CA4) = saved;
                ccat_savedHealth = none;
            }
        }
    }
    Ccat_Fn45B7D0();
    Ccat_Fn43BE40(state);
    ccat_refreshing = 0;
}

// 0x6CCB20: a ped's model built dressed in the player's clothes (the builder's other
// entry, given the ped)
void __fastcall DressPedModel(Catalog* self, re::Edx, uint8_t* ped) {
    re::Ptr32<const char> parts[6] = {ccat_partHead,  ccat_partTorso, ccat_partLWrist,
                            ccat_partRWrist, ccat_partLegs,  ccat_partFeet};
    if (self->lists == nullptr) {
        return;
    }
    Ccat_Fn45B7B0();
    Dress dress;
    GatherDress(ped, dress);
    uint32_t model;
    void* txd = PedTxd(ped, model);
    uint8_t colour[4];
    Ccat_RgbaConstruct(colour, 0xFF, 0xFF, 0xFF, 0xFF);
    uint8_t extra[6];
    At<uint32_t>(extra, 0) = 0;
    At<uint16_t>(extra, 4) = 0;
    Ccat_BuildPedClump(ccat_modelBuilder, ped, model, txd, dress.models, dress.models, parts, 6, dress.tattoos,
                       dress.tattooCount, extra, colour);
    Ccat_Fn45B7D0();
}

// 0x6CCD90: the player redressed if the clothes changed
void __fastcall Refresh(Catalog* self, re::Edx) {
    if (ccat_clothesChanged == 0) {
        return;
    }
    uint8_t* player = ccat_playerPed;
    if (player != nullptr) {
        Ccat_DressPed(self, player);
    }
    ccat_clothesChanged = 0;
}

} // namespace

#define CLOTHCAT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CLOTHCAT_REVERSED("CClothing::RefreshIfCatalog", 0x006CA7B0, &RefreshIfLoaded);
CLOTHCAT_REVERSED("ClothingCatalog::Get", 0x006CA7C0, &GetCatalog);
CLOTHCAT_REVERSED("ClothingCatalog::Begin", 0x006CB550, &Begin);
CLOTHCAT_REVERSED("ClothingCatalog::End", 0x006CB620, &End);
CLOTHCAT_REVERSED("ClothingCatalog::SetRefreshing", 0x006CC2C0, &SetRefreshing);
CLOTHCAT_REVERSED("ClothingCatalog::Construct", 0x006CC2D0, &Construct);
CLOTHCAT_REVERSED("ClothingCatalog::FreeLists", 0x006CC2E0, &FreeLists);
CLOTHCAT_REVERSED("ClothingCatalog::Find", 0x006CC320, &Find);
CLOTHCAT_REVERSED("ClothingCatalog::FindHair", 0x006CC3F0, &FindHair);
CLOTHCAT_REVERSED("ClothingCatalog::Load", 0x006CC450, &Load);
CLOTHCAT_REVERSED("ClothingCatalog::DressPed", 0x006CC7C0, &DressPed);
CLOTHCAT_REVERSED("ClothingCatalog::DressPedModel", 0x006CCB20, &DressPedModel);
// (the game's 0x6CA7B0 jumps here with the catalogue in ecx)
CLOTHCAT_REVERSED("ClothingCatalog::Refresh", 0x006CCD90, &Refresh);
