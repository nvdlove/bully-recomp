#pragma once

// Model info: one per model id (0 .. 0x2FA7), from the IDE files.

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

enum eModelInfoType : uint8_t {
    MODEL_INFO_SIMPLE = 1,   // CSimpleModelInfo (+0x2C is set for them, see RequestModel)
    MODEL_INFO_TYPE_1 = MODEL_INFO_SIMPLE,
    MODEL_INFO_TIME = 3,     // CTimeModelInfo
    MODEL_INFO_TYPE_3 = MODEL_INFO_TIME,
    MODEL_INFO_WEAPON = 4,   // CWeaponModelInfo
    MODEL_INFO_CLUMP = 6,    // CClumpModelInfo (5: see SetAtomicRendererCallbacks)
    MODEL_INFO_VEHICLE = 7,  // CVehicleModelInfo
    MODEL_INFO_PED = 8,      // CPedModelInfo
    MODEL_INFO_PANIM = 10,   // CPAnimModelInfo: a prop animation
};

// A model's 2D effect (lights, particles, ...), in a pool; a model's effects are a list.
struct C2dEffect {
    uint8_t unknown00[0x10];
    int16_t next;         // +0x10 the model's next one (pool slot), -1: none
    uint8_t unknown12[2];
    re::Ptr32<struct NiRefObject> object14;  // +0x14 (Gamebryo, refcounted)
    re::Ptr32<struct NiRefObject> object18;  // +0x18
    uint8_t unknown1C[0x24];

    // 0x51B2A0: the (vector) deleting destructor (thiscall; flags 1: freed, 2: an array)
    C2dEffect* Destroy(uint8_t flags);
};
static_assert(sizeof(C2dEffect) == 0x40);

// What every model info has (CBaseModelInfo's 0x20 bytes). The kinds below put their
// own fields after it.
struct CBaseModelInfoFields {
    re::Ptr32<re::Ptr32<void>> vtable;           // +0x00
    uint32_t nameHash;       // +0x04
    uint8_t type;            // +0x08 eModelInfoType
    uint8_t ownsColModel;    // +0x09
    uint8_t unknown0A;       // +0x0A (0xFF when made)
    uint8_t unknown0B;       // +0x0B (0xFF when made)
    re::Ptr32<void> colModel;          // +0x0C CColModel
    int16_t first2dEffect;   // +0x10 C2dEffect pool slot, -1: none
    int16_t unknown12;       // +0x12 (-1 when made)
    int16_t numRefs;         // +0x14 entities using it
    int16_t txdIndex;        // +0x16 its texture dictionary (CTxdStore slot)
    re::Ptr32<void> unknown18;         // +0x18 its entry in the butes, found by its name (FindButes)
    uint8_t unknown1C;       // +0x1C set: not a model info but a stand-in (CModelInfo::SetStandIn)
    uint8_t unknown1D[3];
};
static_assert(sizeof(CBaseModelInfoFields) == 0x20);

// Only what's understood so far; the rest is padding until it is. (+0x20.. are
// CSimpleModelInfo's, used where the type is simple or time.)
struct CBaseModelInfo : CBaseModelInfoFields {
    uint8_t unknown20[8];    // +0x20
    uint32_t flags;          // +0x28 (simple and time models)
    uint8_t unknown2C;       // +0x2C (0xFF: set when requested with flag 0x10)

    // 0x50E8C0: what CBaseModelInfo's constructor does (thiscall; returns this, as the
    // game's constructor leaves it in eax)
    CBaseModelInfo* Construct(uint8_t modelType);
    // 0x50EE20: the destructor's body (not freed)
    void Destruct();
    // vf0 0x50EEC0: the destructor (flags 1: and freed)
    CBaseModelInfo* Destroy(uint8_t flags);
    // vf1 0x50ED40
    void SetName(const char* name);
    // vf2 0x50EE70: its collision (if its own), its objects, its effects
    void Shutdown();
    // 0x50E940
    uint8_t MatchesName(const char* name);
    // 0x50EB30
    void SetColModel(void* col, uint8_t owns);
    // 0x50E900: its dictionary by name (added if there's none)
    void SetTexDictionary(const char* name, int seasonal);
    // 0x50EBF0: its butes (+0x18) by its name
    void FindButes();
    // 0x50EB90 / 0x50EBC0: an entity using it: its dictionary and animation files too
    int AddRef();
    int RemoveRef();
    // 0x50E960 / 0x50EA40: its animation files' refs
    void AddAnimRefs();
    void RemoveAnimRefs();
    // 0x50EB70: to the front of its effects
    void Add2dEffect(C2dEffect* effect);
    // 0x50ED70
    C2dEffect* GetFirst2dEffect();
    // 0x50EDA0
    void Delete2dEffects();

    // the virtual calls (the game's, wherever they point)
    CBaseModelInfo* CallDestroy(uint8_t deleteFlags) { return Virtual<CBaseModelInfo*>(0, deleteFlags); }
    void CallSetName(const char* name) { Virtual<void>(1, name); }
    void CallShutdown() { Virtual<void>(2); }

    // vf3: frees its Gamebryo objects
    void DeleteObjects() { Virtual<void>(3); }
    // vf4: a new instance of its objects (clumps: their scene graph)
    void* CreateInstance() { return Virtual<void*>(4); }
    // vf6: its animation file by name (clumps)
    void SetAnimFile(const char* name) { Virtual<void>(6, name); }

    // the files it needs streamed with it (-1: none). Slot 7 is the animation file
    // for clump models; 8..10 are per type (peds: three, vehicles and weapons: one)
    int GetAnimFileIndex() { return Virtual<int>(7); }
    int GetDependency8() { return Virtual<int>(8); }
    int GetDependency9() { return Virtual<int>(9); }
    int GetDependency10() { return Virtual<int>(10); }
    // CPAnimModelInfo: its prop animation (-1: none)
    int16_t GetPAnimIndex() { return Virtual<int16_t>(11); }

private:
    template <typename R, typename... A>
    R Virtual(int slot, A... args) {
        return reinterpret_cast<R(__thiscall*)(CBaseModelInfo*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(this, args...);
    }
};
static_assert(offsetof(CBaseModelInfo, type) == 0x08);
static_assert(offsetof(CBaseModelInfo, colModel) == 0x0C);
static_assert(offsetof(CBaseModelInfo, unknown18) == 0x18);
static_assert(offsetof(C2dEffect, object18) == 0x18);
static_assert(offsetof(CBaseModelInfo, numRefs) == 0x14);
static_assert(offsetof(CBaseModelInfo, txdIndex) == 0x16);
static_assert(offsetof(CBaseModelInfo, flags) == 0x28);
static_assert(offsetof(CBaseModelInfo, unknown2C) == 0x2C);

// The kinds. Their fields follow the base's 0x20 bytes; Base() is the same object as a
// CBaseModelInfo.
#define MODEL_INFO_BASE \
    CBaseModelInfo* Base() { return reinterpret_cast<CBaseModelInfo*>(this); }

// Simple (0x34, ms_simpleModelStore) and time (0x40, ms_timeModelStore) models
struct CSimpleModelInfo : CBaseModelInfoFields {
    re::Ptr32<void> object;            // +0x20 (cleared by 0x527410 with +0x28)
    uint8_t unknown24[4];
    uint32_t flags;          // +0x28
    uint8_t unknown2C[8];
    MODEL_INFO_BASE

    // 0x51A770: the constructor (thiscall)
    CSimpleModelInfo* Construct();
    // 0x51A9D0: the (vector) deleting destructor
    CSimpleModelInfo* Destroy(uint8_t flags);
};
static_assert(sizeof(CSimpleModelInfo) == 0x34);

struct CTimeModelInfo : CSimpleModelInfo {
    uint8_t unknown34[8];
    int32_t otherModel;      // +0x3C (-1 when made)

    // 0x51A790
    CTimeModelInfo* Construct();
    // 0x51AA40
    CTimeModelInfo* Destroy(uint8_t flags);
};
static_assert(offsetof(CTimeModelInfo, otherModel) == 0x3C);
static_assert(sizeof(CTimeModelInfo) == 0x40);

// Models with a scene graph ("clump", as in GTA)
struct CClumpModelInfo : CBaseModelInfoFields {
    re::Ptr32<void> clump;             // +0x20 its Gamebryo scene graph, nullptr: not loaded
    int32_t animFileIndex;   // +0x24 -1: none
    uint8_t unknown28[0x20];
    re::Ptr32<void> unknown48;         // +0x48 (the renderer's, see SetClump)
    MODEL_INFO_BASE

    // 0x5109B0 / 0x5109E0: the constructors (thiscall; the first is type 6)
    CClumpModelInfo* Construct();
    CClumpModelInfo* Construct(uint8_t modelType);
    // 0x510D90: the destructor's body
    void Destruct();
    // 0x50E7A0: before its name is set (store models aren't constructed again)
    void Init();
    // vf3 0x510C40
    void DeleteObjects();
    // vf4 0x510D60
    void* CreateInstance();
    // vf6 0x510A10
    void SetAnimFile(const char* name);
    // vf12 0x510D20: an instance at a matrix (its 0x34 bytes copied in)
    void* CreateInstance(const void* matrix);
    // vf13 0x510CB0: its scene graph loaded
    void SetClump(void* newClump, int arg);
    // 0x510B70: frame ids by name (a list of {name, id, flags}, name nullptr at the end)
    void SetFrameIds(const void* list);
    // 0x510BF0
    void SetAtomicRendererCallbacks(void* node);
};
static_assert(offsetof(CClumpModelInfo, animFileIndex) == 0x24);
static_assert(offsetof(CClumpModelInfo, unknown48) == 0x48);
static_assert(sizeof(CClumpModelInfo) == 0x4C);

struct CWeaponModelInfo : CClumpModelInfo {
    int32_t animFile2;       // +0x4C (vf8) -1: none
    float unknown50;         // +0x50 (30 when made)
    uint8_t unknown54[4];
};
static_assert(sizeof(CWeaponModelInfo) == 0x58);

struct CAccessoryModelInfo : CClumpModelInfo {
    int32_t boneIds[2];      // +0x4C RightHand, LeftHand (-1: not found)

    // vf1 0x50E7E0
    void SetName(const char* name);
    // vf13 0x50E7B0
    void SetClump(void* newClump, int arg);
};
static_assert(sizeof(CAccessoryModelInfo) == 0x54);

struct CPedModelInfo : CClumpModelInfo {
    uint8_t unknown4C[0x30];
    uint32_t altNameHash;    // +0x7C (also found by GetModelIndexByHash)
    uint8_t unknown80[0x48];
    int32_t animFiles[3];    // +0xC8 vf8..vf10

    // vf1 0x51A7C0
    void SetName(const char* name);
    // vf8..10 0x51A7D0 0x51A7E0 0x51A7F0
    int GetDependency8();
    int GetDependency9();
    int GetDependency10();
    // vf0 0x51A800
    CPedModelInfo* Destroy(uint8_t flags);
};
static_assert(offsetof(CPedModelInfo, altNameHash) == 0x7C);
static_assert(offsetof(CPedModelInfo, animFiles) == 0xC8);
static_assert(sizeof(CPedModelInfo) == 0xD4);

struct CVehicleModelInfo : CClumpModelInfo {
    uint8_t unknown4C[4];
    int32_t vehicleClass;    // +0x50 (-1 when added; 1: see IsVehicleClass1)
    uint8_t unknown54[8];
    int16_t unknown5C;       // +0x5C (-1 when added)
    uint8_t unknown5E[0x82];
    float unknownE0;         // +0xE0 (999.99 when added)
    int32_t unknownE4;       // +0xE4
    uint8_t unknownE8[0x68];
    int32_t unknown150;      // +0x150
    uint8_t unknown154[0x8C];
};
static_assert(offsetof(CVehicleModelInfo, vehicleClass) == 0x50);
static_assert(offsetof(CVehicleModelInfo, unknownE0) == 0xE0);
static_assert(offsetof(CVehicleModelInfo, unknown150) == 0x150);
static_assert(sizeof(CVehicleModelInfo) == 0x1E0);

// A prop with its own animation (an "act" tree, .cat files)
#pragma pack(push, 2)
struct CPAnimModelInfo : CClumpModelInfo {
    uint8_t unknown4C[0x16];
    struct EffectSlot {
        int16_t id;          // +0 (from the act data's +0x30 / +0x3C / +0x48)
        uint8_t kind;        // +2 (+0x38 / +0x44 / +0x50)
        uint8_t pad;
    } effects[3];            // +0x62 +0x66 +0x6A
    uint8_t unknown6E[2];
    re::Ptr32<void> actData;           // +0x70 its act data (butes), nullptr: none
    re::Ptr32<void> actData74;         // +0x74
    re::Ptr32<void> unknown78;         // +0x78
    re::Ptr32<void> unknown7C;         // +0x7C (+0x78 when named)
    int32_t unknown80;       // +0x80
    int16_t panimIndex;      // +0x84 its .cat (-1: not found yet)
    uint8_t unknown86[2];
    re::Ptr32<void> startNode;         // +0x88 its act tree's start node
    int16_t unknown8C;       // +0x8C an animation file (-1: none)
    uint8_t isPx;            // +0x8E its name starts "px"
    uint8_t unknown8F[5];

    // 0x51A820
    CPAnimModelInfo* Construct();
    // 0x51AAB0
    CPAnimModelInfo* Destroy(uint8_t flags);
    // vf1 0x520AE0
    void SetName(const char* name);
    // vf11 0x51A860
    int16_t GetPAnimIndex();
    // 0x5205B0: its act data from the butes (by its name, else the default one)
    void LoadActData();
    // 0x520A20: its prop animation index (+0x84) if unset
    void UpdateAnimIndex();
    // 0x520180 / 0x520370
    void SetUnknown8C(const char* name);
    int16_t GetUnknown8C();
    // 0x520530
    void SetUnknown78(const char* name);
    // 0x520320 / 0x520340
    float GetUnknownFloat1();
    float GetUnknownFloat2();
    // 0x520390
    void* GetActField(int which);
    // 0x520420
    int GetUnknown4C(int i);
    // 0x520430
    uint8_t IsPx();
    // 0x520440 / 0x520460 / 0x520480: its act data's +0x80 is this kind
    uint8_t IsKind1();
    uint8_t IsKind2();
    uint8_t IsKind3();
    // 0x520030
    void ResetUnknown78();
};
#pragma pack(pop)
static_assert(offsetof(CPAnimModelInfo, effects) == 0x62);
static_assert(offsetof(CPAnimModelInfo, actData) == 0x70);
static_assert(offsetof(CPAnimModelInfo, panimIndex) == 0x84);
static_assert(offsetof(CPAnimModelInfo, startNode) == 0x88);
static_assert(offsetof(CPAnimModelInfo, isPx) == 0x8E);
static_assert(sizeof(CPAnimModelInfo) == 0x94);

// The name index (made at start): an AVL tree over the model ids by name hash, with
// CTxdStore's node encoding (each id's two children as (id << 1 | a balance bit)).
struct ModelNameTree {
    struct Node {
        int16_t greater;  // +0
        int16_t less;     // +2
    } nodes[0x2FA8];
    int16_t root;  // +0xBEA0

    // 0x51BCE0 (thiscall): flags 1: an exact match; 2 / 4: else the nearest below / above
    int16_t Find(uint32_t hash, uint8_t flags);
    // 0x51BA60 (thiscall): adds the id under its model's name (nothing if the name's there)
    int16_t Insert(int16_t id);
    // 0x51BD90 (thiscall): the node with the name out, its id (-1: none)
    int16_t Remove(uint32_t hash);
    // a subtree's height, -1 if it isn't a valid tree (for testing)
    int Check(int16_t node, int64_t low, int64_t high, int depth) const;

private:
    int16_t Child(int16_t node, int side) const;
    void SetChild(int16_t node, int side, int16_t child);
    int Balance(int16_t node) const;
    void SetBalance(int16_t node, int balance);
    int16_t Rotate(int16_t node, int balance);
    void Replace(int16_t parent, int side, int16_t child);
};
static_assert(offsetof(ModelNameTree, root) == 0xBEA0);
static_assert(sizeof(ModelNameTree) == 0xBEA2);

// Config\Extra\ModelInd.bin: (name hash, id) sorted by hash, when ms_hashedNames
struct ModelHashTable {
    struct Entry {
        uint32_t hash;
        uint16_t id;
        uint16_t pad;
    };
    re::Ptr32<Entry> entries;   // +0
    int32_t count;  // +4

    // 0x51A870 (thiscall)
    void Load();
    // 0x51A8F0 (thiscall): the id, -1: none
    int Find(uint32_t hash);
};
static_assert(sizeof(ModelHashTable) == 8);

namespace CModelInfo {
GAME_ARRAY(CBaseModelInfo*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(ModelNameTree*, ms_pNameTree, 0x00C67730);   // nullptr: names are looked up id by id
GAME_VAR(ModelHashTable, ms_hashTable, 0x00C89F38);
GAME_VAR(uint8_t, ms_hashedNames, 0x00BF380C);        // names are looked up in ms_hashTable
GAME_VAR(int32_t, ms_numStandIns, 0x00C735D8);
GAME_VAR(int32_t, ms_numClumpModelInfos, 0x00C66078);

// The stores: pools for the simple, time and prop animation models and the effects;
// fixed arrays (a count, then the objects) for the rest
GAME_VAR(CPool<CSimpleModelInfo>, ms_simpleModelStore, 0x00A76AD4);
GAME_VAR(CPool<CTimeModelInfo>, ms_timeModelStore, 0x00A76AF0);
GAME_VAR(CPool<CPAnimModelInfo>, ms_panimModelStore, 0x00A76B0C);
GAME_VAR(CPool<C2dEffect>, ms_2dEffectPool, 0x00A76B28);   // (the pool itself, not a pointer)
GAME_VAR(int32_t, ms_numWeaponModels, 0x00C735E0);
GAME_ARRAY(CWeaponModelInfo, ms_weaponModelStore, 0x00C735E4);  // 150
GAME_VAR(int32_t, ms_numAccessoryModels, 0x00C76978);
GAME_ARRAY(CAccessoryModelInfo, ms_accessoryModelStore, 0x00C7697C);  // 21
GAME_VAR(int32_t, ms_numClumpModels, 0x00C77060);
GAME_ARRAY(CClumpModelInfo, ms_clumpModelStore, 0x00C77064);  // 5
GAME_VAR(int32_t, ms_numVehicleModels, 0x00C771E0);
GAME_ARRAY(CVehicleModelInfo, ms_vehicleModelStore, 0x00C771E4);  // 32
GAME_VAR(int32_t, ms_numPedModels, 0x00C7ADE8);
GAME_ARRAY(CPedModelInfo, ms_pedModelStore, 0x00C7ADEC);  // 0x123
GAME_VAR(int32_t, ms_numClothModels, 0x00C89EE8);
GAME_ARRAY(CClumpModelInfo, ms_clothModelStore, 0x00C89EEC);  // 1 (CClothModelInfo, 0x4C)

inline re::Ptr32<void>* const ms_baseVtable = reinterpret_cast<re::Ptr32<void>*>(0x009149A4);
inline re::Ptr32<void>* const ms_clumpVtable = reinterpret_cast<re::Ptr32<void>*>(0x00914B10);
inline re::Ptr32<void>* const ms_simpleVtable = reinterpret_cast<re::Ptr32<void>*>(0x00917B00);
inline re::Ptr32<void>* const ms_timeVtable = reinterpret_cast<re::Ptr32<void>*>(0x00917B38);
inline re::Ptr32<void>* const ms_weaponVtable = reinterpret_cast<re::Ptr32<void>*>(0x00917B70);
inline re::Ptr32<void>* const ms_panimVtable = reinterpret_cast<re::Ptr32<void>*>(0x00917C64);
}

// Not reversed yet: called where they are.
namespace game {
// CPAnimModelInfo 0x520A20 (thiscall): sets its prop animation index (+0x84) if unset
GAME_FN(void, __thiscall, PAnimModelInfo_UpdateAnimIndex, 0x00520A20, CBaseModelInfo*);
}
