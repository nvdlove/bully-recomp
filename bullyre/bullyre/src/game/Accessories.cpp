// The accessory manager (0xC2C730, 0xA2C bytes): the accessories knocked off peds lying
// about (up to 0x30), and the ped/accessory pairs from the data (AccessoryPedPair: which
// ped model wears which accessory, where, how likely, in which season), with the
// socket names they use (0xC2C1B0: 50 short strings).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct Vec {
    float x, y, z;
};

// a ped/accessory pair (0x18 bytes)
struct AccessoryPair {
    int32_t pedModel;         // +0x0 (-1: looked up from the hash)
    int32_t accessoryModel;   // +0x4 (-1: looked up from the hash)
    uint32_t pedHash;         // +0x8
    uint32_t accessoryHash;   // +0xC
    uint16_t socket;          // +0x10 (socket names' index)
    uint16_t socketOnAccessory;   // +0x12
    uint8_t probability;      // +0x14 (percent)
    uint8_t flags;            // +0x15 1 winter only, 2 all seasons
    uint8_t unknown16[2];
};
static_assert(sizeof(AccessoryPair) == 0x18);

struct CAccessoryManager {
    re::Ptr32<uint8_t> accessories[0x30];   // +0x0 lying about
    int32_t count;                // +0xC0
    uint8_t anyPedModel;          // +0xC4 (pairs of the model at 0xBF3E5C count too)
    uint8_t unknownC5[3];
    AccessoryPair pairs[100];     // +0xC8
    int32_t numPairs;             // +0xA28
};
static_assert(offsetof(CAccessoryManager, count) == 0xC0);
static_assert(offsetof(CAccessoryManager, pairs) == 0xC8);
static_assert(offsetof(CAccessoryManager, numPairs) == 0xA28);

// a short string (the game's: its data, its capacity, its buffer)
template <int N>
struct ShortString {
    re::Ptr32<char> data;
    int32_t capacity;
    char buffer[N];
};

// the socket names: 50 strings of 0x1C bytes, then their count
struct SocketNames {
    ShortString<0x14> names[50];
    int32_t count;   // +0x578
};
static_assert(sizeof(ShortString<0x14>) == 0x1C);
static_assert(offsetof(SocketNames, count) == 0x578);

inline CAccessoryManager* const TheAccessories = reinterpret_cast<CAccessoryManager*>(0x00C2C730);
inline SocketNames* const TheSocketNames = reinterpret_cast<SocketNames*>(0x00C2C1B0);
GAME_VAR(uint8_t, g_useModelIds, 0x00BF380E);     // (the data gives model ids, not names)
GAME_VAR(int32_t, g_modelBF3E5C, 0x00BF3E5C);
GAME_VAR(int32_t, currArea, 0x00BD1008);           // CGame::currArea
GAME_VAR(uint8_t, g_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, g_chapterSeason, 0x00A147EC); // (2: winter)
GAME_VAR(void*, TheCamera, 0x00C3CCF4);
inline void* const g_random = reinterpret_cast<void*>(0x00CF356C);
constexpr uintptr_t vtblAccessoryPedPairLoader = 0x0091253C;

GAME_FN(float, __thiscall, Entity_GetBoundRadius, 0x00465E10, void* entity);
GAME_FN(void, __thiscall, Entity_GetBoundCentre, 0x00466CA0, void* entity, Vec* out);
GAME_FN(float, __cdecl, FindGroundZ2D, 0x004634A0, float x, float y, int);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void*, __cdecl, Accessory_New, 0x004A7070, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, Accessory_Construct, 0x004A6E40, void* accessory);
GAME_FN(void, __thiscall, Accessory_SetInfo, 0x004A6C20, uint8_t* accessory, void* info);
GAME_FN(void, __thiscall, Accessory_Set, 0x004A6FA0, uint8_t* accessory, int model, int group, void* wearer, int anim, void* ped);
GAME_FN(void, __thiscall, Accessory_4A6DD0, 0x004A6DD0, uint8_t* accessory);
GAME_FN(void, __thiscall, Accessory_Clear, 0x004A7040, uint8_t* accessory);
GAME_FN(float, __thiscall, Accessory_4A6DF0, 0x004A6DF0, uint8_t* accessory);
GAME_FN(Vec*, __thiscall, Accessory_GetNodePosition, 0x004A6EF0, uint8_t* accessory, Vec* out);
GAME_FN(uint8_t, __thiscall, Container_HasRoom, 0x004A73D0, void* container);
GAME_FN(void, __thiscall, Container_Add, 0x004A73B0, void* container, uint8_t* accessory);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(int32_t, __thiscall, Fn465580, 0x00465580, void* self, uint32_t hash, void* out);
GAME_FN(int, __cdecl, GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* generator);
inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void*, __cdecl, DataReader_Open, 0x005B07E0, void* reader, int32_t* out, int);
GAME_FN(uint8_t, __thiscall, DataReader_Next, 0x005B0720, void* handle, void* loader, const char* name);
GAME_FN(void, __cdecl, DataReader_Close, 0x005B0860, void* handle);
// (their own, called where modes apply)
GAME_FN(void, __thiscall, Accessory_KeepAboveGround, 0x004A7080, uint8_t* accessory);
GAME_FN(void, __thiscall, Accessories_Add, 0x004A7630, CAccessoryManager* self, uint8_t* ped, int model, int socket,
        int socketOnAccessory, AccessoryPair* pair);
GAME_FN(void, __thiscall, Accessories_RequestModels, 0x004A79C0, CAccessoryManager* self);
GAME_FN(int32_t, __thiscall, SocketNames_Add, 0x004A7BA0, SocketNames* self, const char* name);

void DeleteAccessory(uint8_t* accessory) {
    using Fn = void(__thiscall*)(void*, int);
    re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(accessory))[0])(accessory, 1);
}

} // namespace

namespace c09accessories {

// 0x4A7080: CAccessory: lifted onto the ground when it's sunk into it (its bounds
// under the ground), and stopped
void __fastcall KeepAboveGround(uint8_t* self, re::Edx) {
    float radius = Entity_GetBoundRadius(self);
    Vec centre;
    Entity_GetBoundCentre(self, &centre);
    float ground = FindGroundZ2D(centre.x, centre.y, 1);
    float top = float(double(ground) + radius);
    if (!(top > centre.z)) {
        return;
    }
    float lift = float(double(top) - centre.z);
    uint8_t* matrix = At<uint8_t*>(self, 0x14);
    const Vec* pos = matrix ? reinterpret_cast<Vec*>(matrix + 0x30) : reinterpret_cast<Vec*>(self + 4);
    float x = pos->x;
    float y = pos->y;
    float z = float(double(pos->z) + lift);
    Entity_SetPosition(self, x, y, z);
    At<Vec>(self, 0x124) = {0.0f, 0.0f, 0.0f};
    At<uint8_t>(self, 0x1BC) = (At<uint8_t>(self, 0x1BC) & 0xFD) | 0xC;
    At<Vec>(self, 0x130) = {0.0f, 0.0f, 0.0f};
}

// 0x4A75D0: the pairs read from the data (counted as they're read)
void __fastcall LoadPairs(CAccessoryManager* self, re::Edx, void* reader) {
    int32_t local = 0;
    void* handle = DataReader_Open(reader, &local, 1);
    uintptr_t loader = vtblAccessoryPedPairLoader;   // (an AccessoryPedPairLoader: its vtable only)
    while (DataReader_Next(handle, &loader, "AccessoryPedPair")) {
        self->numPairs++;
    }
    DataReader_Close(handle);
}

// 0x4A7630: an accessory made for the ped (when it has room): on the list, its bone
// from the socket's name, worn
void __fastcall AddAccessory(CAccessoryManager* self, re::Edx, uint8_t* ped, int model, int socket, int socketOnAccessory,
                             AccessoryPair* pair) {
    void* container = At<void*>(ped, 0x1D8);
    if (!Container_HasRoom(container)) {
        return;
    }
    void* memory = Accessory_New(0x2E8);
    uint8_t* accessory = memory ? Accessory_Construct(memory) : nullptr;
    Accessory_SetInfo(accessory, pair);
    self->accessories[self->count] = accessory;
    self->count++;
    void* groups = At<void*>(At<uint8_t*>(ped, 0x114), 0x94);
    // (the game has 0x465580 write into its `ped` argument's slot: what's there stays
    // when it doesn't)
    uintptr_t slot = reinterpret_cast<uintptr_t>(ped);
    Fn465580(groups, HashName(TheSocketNames->names[socket].data), &slot);
    Accessory_Set(accessory, model, socketOnAccessory, At<void*>(ped, 0x114), int(slot), ped);
    Container_Add(container, accessory);
}

// 0x4A76F0: every one lying about deleted
void __fastcall Clear(CAccessoryManager* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        if (!self->accessories[i]) {
            continue;
        }
        Accessory_4A6DD0(self->accessories[i]);
        Accessory_Clear(self->accessories[i]);
        if (self->accessories[i]) {
            DeleteAccessory(self->accessories[i]);
        }
        self->accessories[i] = nullptr;
    }
    self->anyPedModel = 0;
    self->count = 0;
}

// 0x4A7750: the ones no longer lying about (+0x2DC 0) taken off the list (not deleted)
void __fastcall Compact(CAccessoryManager* self, re::Edx) {
    int kept = 0;
    re::Ptr32<uint8_t>* out = self->accessories;
    for (int i = 0; i < self->count; i++) {
        uint8_t* accessory = self->accessories[i];
        if (accessory && At<int32_t>(accessory, 0x2DC) == 0) {
            Accessory_4A6DD0(accessory);
        } else {
            *out++ = accessory;
            kept++;
        }
    }
    self->count = kept;
}

// 0x4A77B0: each one lying about: its vf17 (PreRender)
void __fastcall PreRender(CAccessoryManager* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        uint8_t* accessory = self->accessories[i];
        if (At<int32_t>(accessory, 0x2DC) == 0) {
            using Fn = void(__thiscall*)(void*);
            re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(accessory))[17])(accessory);
        }
    }
}

// 0x4A77F0
uint8_t __fastcall Contains(CAccessoryManager* self, re::Edx, uint8_t* accessory) {
    uint8_t found = 0;
    if (accessory) {
        for (int i = 0; i < self->count; i++) {
            if (!found && accessory == self->accessories[i]) {
                found = 1;
            }
        }
    }
    return found;
}

// 0x4A7820: the socket names' constructor (all empty)
SocketNames* __fastcall SocketNamesConstruct(SocketNames* self, re::Edx) {
    for (int i = 0; i < 50; i++) {
        self->names[i].data = self->names[i].buffer;
        self->names[i].capacity = 0x11;
        self->names[i].buffer[0] = 0;
    }
    self->count = 0;
    return self;
}

// 0x4A7880: the constructor (no pairs, nothing lying about)
CAccessoryManager* __fastcall Construct(CAccessoryManager* self, re::Edx) {
    for (int i = 0; i < 100; i++) {
        self->pairs[i].flags &= 0xFC;
    }
    for (int i = 0; i < 100; i++) {
        AccessoryPair& pair = self->pairs[i];
        pair.pedModel = -1;
        pair.accessoryModel = -1;
        pair.pedHash = 0;
        pair.accessoryHash = 0;
        pair.socket = 0;
        pair.socketOnAccessory = 0;
        pair.probability = 0;
    }
    self->numPairs = 0;
    self->count = 0;
    for (int i = 0; i < 0x30; i++) {
        self->accessories[i] = nullptr;
    }
    self->anyPedModel = 0;
    return self;
}

// 0x4A7960: one taken off the list (the last moved into its place; not deleted)
void __fastcall Remove(CAccessoryManager* self, re::Edx, uint8_t* accessory) {
    for (int i = 0; i < self->count; i++) {
        if (self->accessories[i] != accessory) {
            continue;
        }
        Accessory_4A6DD0(self->accessories[i]);
        Accessory_Clear(self->accessories[i]);
        self->accessories[i] = self->accessories[self->count - 1];
        self->accessories[self->count - 1] = nullptr;
        self->count--;
        return;
    }
}

// 0x4A79C0: the pairs' accessory models requested
void __fastcall RequestModels(CAccessoryManager* self, re::Edx) {
    for (int i = 0; i < self->numPairs; i++) {
        int32_t& model = self->pairs[i].accessoryModel;
        if (!g_useModelIds && model == -1) {
            model = GetModelIndexByHash(self->pairs[i].accessoryHash, 0, 0x2FA7);
        }
        if (model != -1 && GetStreamingInfo(model)[4] != 1) {
            RequestModel(model, 5);
        }
    }
    self->anyPedModel = 0;
}

// 0x4A7A40: the last of those lying about that's done with (gone from view, or more
// than 5 m from the camera past 0x4A6DF0's 10) deleted; the others kept on the ground
void __fastcall DropOne(CAccessoryManager* self, re::Edx) {
    int drop = -1;
    for (int i = 0; i < self->count; i++) {
        uint8_t* accessory = self->accessories[i];
        if (At<int32_t>(accessory, 0x2DC) != 0) {
            continue;
        }
        if (At<void*>(At<uint8_t*>(accessory, 0x114), 0xC) == nullptr) {
            drop = i;
            continue;
        }
        Accessory_KeepAboveGround(accessory);
        if (!(Accessory_4A6DF0(self->accessories[i]) > 10.0f)) {
            continue;
        }
        uint8_t* transform = GetLocalTransform(GetCameraNode(TheCamera));
        Vec camera = {At<float>(transform, 0x24), At<float>(transform, 0x28), At<float>(transform, 0x2C)};
        Vec node;
        Accessory_GetNodePosition(self->accessories[i], &node);
        Vec d = {camera.x - node.x, camera.y - node.y, camera.z - node.z};
        float distSq = float((double(d.y) * d.y + double(d.x) * d.x) + double(d.z) * d.z);
        if (float(std::sqrt(distSq)) > 5.0f) {
            drop = i;
        }
    }
    if (drop < 0) {
        return;
    }
    uint8_t* accessory = self->accessories[drop];
    Accessory_4A6DD0(accessory);
    Accessory_Clear(self->accessories[drop]);
    self->accessories[drop] = self->accessories[self->count - 1];
    self->accessories[self->count - 1] = nullptr;
    self->count--;
    if (accessory) {
        DeleteAccessory(accessory);
    }
}

// 0x4A7BA0: a socket name's index (the last match), added when it's new
int32_t __fastcall SocketNamesAdd(SocketNames* self, re::Edx, const char* name) {
    int count = self->count;
    int found = -1;
    if (count > 0) {
        for (int i = 0; i < count; i++) {
            const unsigned char* a = re::PtrCast<const unsigned char*>(self->names[i].data);
            const unsigned char* b = reinterpret_cast<const unsigned char*>(name);
            while (*a == *b && *a) {
                a++;
                b++;
            }
            if (*a == *b) {
                found = i;
            }
        }
        if (found >= 0) {
            return found;
        }
    }
    Strncpy(self->names[count].data, name, size_t(self->names[count].capacity));
    int index = self->count;
    self->count = index + 1;
    return index;
}

// 0x4A7C40: AccessoryPedPairLoader's vf1: the next pair read (Ped, Accessory (a name,
// or an id when the data gives ids), Socket, SocketOnAccessory, Probablility,
// WinterOnly, AllSeasons). (The values the reader doesn't write are stack garbage in
// the game; 0 here)
void __fastcall LoadPair(void*, re::Edx, void* reader) {
    using ReadString = void(__thiscall*)(void*, void*, int, const char*);
    using ReadInt = void(__thiscall*)(void*, int32_t*, const char*);
    using ReadBool = void(__thiscall*)(void*, uint8_t*, const char*);
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reader);
    AccessoryPair* pairs = TheAccessories->pairs;

    ShortString<0x21> ped = {nullptr, 0x21, {0}};
    ped.data = ped.buffer;
    re::PtrCast<ReadString>(vtable[9])(reader, &ped, 0x20, "Ped");
    pairs[TheAccessories->numPairs].pedHash = HashName(ped.data);
    if (g_useModelIds) {
        int32_t model = -1;
        re::PtrCast<ReadInt>(vtable[14])(reader, &model, "Accessory");
        pairs[TheAccessories->numPairs].accessoryModel = model;
    } else {
        ShortString<0x21> accessory = {nullptr, 0x21, {0}};
        accessory.data = accessory.buffer;
        re::PtrCast<ReadString>(vtable[9])(reader, &accessory, 0x20, "Accessory");
        pairs[TheAccessories->numPairs].accessoryHash = HashName(accessory.data);
    }
    ShortString<0x11> socket = {nullptr, 0x11, {0}};
    socket.data = socket.buffer;
    re::PtrCast<ReadString>(vtable[9])(reader, &socket, 0x10, "SocketOnPed");   // (the key is "SocketOnPed")
    pairs[TheAccessories->numPairs].socket = uint16_t(SocketNames_Add(TheSocketNames, socket.data));
    ShortString<0x11> onAccessory = {nullptr, 0x11, {0}};
    onAccessory.data = onAccessory.buffer;
    re::PtrCast<ReadString>(vtable[9])(reader, &onAccessory, 0x10, "SocketOnAccessory");
    pairs[TheAccessories->numPairs].socketOnAccessory = uint16_t(SocketNames_Add(TheSocketNames, onAccessory.data));
    int32_t probability = 0;
    re::PtrCast<ReadInt>(vtable[14])(reader, &probability, "Probablility");
    pairs[TheAccessories->numPairs].probability = uint8_t(probability);
    uint8_t winterOnly = 0;
    re::PtrCast<ReadBool>(vtable[15])(reader, &winterOnly, "WinterOnly");
    uint8_t& flags = pairs[TheAccessories->numPairs].flags;
    flags ^= (flags ^ winterOnly) & 1;
    uint8_t allSeasons = 0;
    re::PtrCast<ReadBool>(vtable[15])(reader, &allSeasons, "AllSeasons");
    uint8_t& flags2 = pairs[TheAccessories->numPairs].flags;
    flags2 ^= (uint8_t(allSeasons * 2) ^ flags2) & 2;
}

// 0x4A7E50: a ped's accessories, by its model's pairs: the season right, the chance
// met, the model loaded (requested else)
void __fastcall AddAccessoriesFor(CAccessoryManager* self, re::Edx, uint8_t* ped) {
    if ((At<uint8_t>(ped, 0x108) & 7) != 3) {
        return;
    }
    int32_t pedModel = At<int16_t>(ped, 0x10E);
    for (int i = 0; i < self->numPairs; i++) {
        AccessoryPair* pair = &self->pairs[i];
        if (pair->pedModel == -1) {
            pair->pedModel = GetModelIndexByHash(pair->pedHash, 0, 0x2FA7);
        }
        if (!self->anyPedModel && pair->accessoryModel == g_modelBF3E5C) {
            continue;
        }
        if (pair->pedModel != pedModel) {
            continue;
        }
        uint8_t winter = IsAreaInterior(currArea) && g_chapterSeason[g_chapter] == 2 ? 1 : 0;
        if (!(pair->flags & 2) && (pair->flags & 1) != winter) {
            continue;
        }
        float chance = float(double(int32_t(pair->probability)) / 100.0);
        float roll = Random_GetFloat(g_random);
        if (chance < roll) {
            continue;
        }
        if (!g_useModelIds && pair->accessoryModel == -1) {
            pair->accessoryModel = GetModelIndexByHash(pair->accessoryHash, 0, 0x2FA7);
        }
        if (pair->accessoryModel == -1) {
            continue;
        }
        if (GetStreamingInfo(pair->accessoryModel)[4] != 1) {
            Accessories_RequestModels(self);
        }
        if (GetStreamingInfo(pair->accessoryModel)[4] == 1) {
            Accessories_Add(self, ped, pair->accessoryModel, pair->socket, pair->socketOnAccessory, pair);
        }
    }
}

} // namespace c09accessories

// ---- registration ---------------------------------------------------------------------

#define ACCESSORIES_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

ACCESSORIES_REVERSED("CAccessory::KeepAboveGround", 0x004A7080, &c09accessories::KeepAboveGround);
ACCESSORIES_REVERSED("CAccessoryManager::LoadPairs", 0x004A75D0, &c09accessories::LoadPairs);
ACCESSORIES_REVERSED("CAccessoryManager::AddAccessory", 0x004A7630, &c09accessories::AddAccessory);
ACCESSORIES_REVERSED("CAccessoryManager::Clear", 0x004A76F0, &c09accessories::Clear);
ACCESSORIES_REVERSED("CAccessoryManager::Compact", 0x004A7750, &c09accessories::Compact);
ACCESSORIES_REVERSED("CAccessoryManager::PreRender", 0x004A77B0, &c09accessories::PreRender);
ACCESSORIES_REVERSED("CAccessoryManager::Contains", 0x004A77F0, &c09accessories::Contains);
ACCESSORIES_REVERSED("CAccessorySocketNames::Construct", 0x004A7820, &c09accessories::SocketNamesConstruct);
ACCESSORIES_REVERSED("CAccessoryManager::Construct", 0x004A7880, &c09accessories::Construct);
ACCESSORIES_REVERSED("CAccessoryManager::Remove", 0x004A7960, &c09accessories::Remove);
ACCESSORIES_REVERSED("CAccessoryManager::RequestModels", 0x004A79C0, &c09accessories::RequestModels);
ACCESSORIES_REVERSED("CAccessoryManager::DropOne", 0x004A7A40, &c09accessories::DropOne);
ACCESSORIES_REVERSED("CAccessorySocketNames::Add", 0x004A7BA0, &c09accessories::SocketNamesAdd);
ACCESSORIES_REVERSED("AccessoryPedPairLoader::LoadPair", 0x004A7C40, &c09accessories::LoadPair);
ACCESSORIES_REVERSED("CAccessoryManager::AddAccessoriesFor", 0x004A7E50, &c09accessories::AddAccessoriesFor);
