// The Butes database (Act/Globals.act): a tree of named nodes with typed values, the
// lookups through it by a path of name keys, the cached Butes objects (0x20C8878..), the
// Butes classes' destructors, a list of load callbacks and the projectile Butes'
// defaults. 0x72A830..0x72BEE0.
//
// A node (0x14 bytes): +0 its name key, +4 child count, +8 the children, +0xC value
// count, +0x10 the values ((key, value) pairs). A path: +0 count, +4 the name keys.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using vc::At;

struct ButesNode {
    uint32_t key;          // +0x0
    uint32_t numChildren;  // +0x4
    re::Ptr32<ButesNode> children;   // +0x8
    uint32_t numValues;    // +0xC
    re::Ptr32<uint32_t> values;      // +0x10 (key, value) pairs
};
static_assert(sizeof(ButesNode) == 0x14);

struct ButesPath {
    int32_t count;   // +0x0
    re::Ptr32<uint32_t> keys;  // +0x4
};
static_assert(sizeof(ButesPath) == 0x8);

// a load callback: (function, data), linked
struct ButesCallback {
    uint32_t fn;          // +0x0
    uint32_t data;        // +0x4
    re::Ptr32<ButesCallback> next;  // +0x8
};
static_assert(sizeof(ButesCallback) == 0xC);

GAME_VAR(ButesCallback*, bdCallbacks, 0x020C886C);
GAME_VAR(uint8_t*, bdFile, 0x020C8870);       // the loaded Act/Globals.act
GAME_VAR(ButesNode*, bdRoot, 0x020C8874);
GAME_ARRAY(uint32_t, bdCached, 0x020C8878);   // the Butes objects by class, below

GAME_FN(ButesNode*, __thiscall, BdNode_FindChild, 0x00727FB0, ButesNode* node, const uint32_t* key);
GAME_FN(ButesNode*, __thiscall, BdNode_ChildAt, 0x00727FF0, void* node, uint32_t index);
GAME_FN(uint32_t, __thiscall, BdNode_Get, 0x00728020, ButesNode* node, const uint32_t* key);
GAME_FN(uint32_t*, __thiscall, BdNode_ValueAt, 0x00728060, ButesNode* node, int index);
GAME_FN(uint32_t, __thiscall, BdNode_ValueDataAt, 0x00728070, ButesNode* node, int index);
GAME_FN(uint8_t*, __cdecl, BdFile_Load, 0x00727F10, const char* path);
GAME_FN(uint32_t, __cdecl, BdFindInTree, 0x0072AEF0, ButesNode* node, const uint32_t* key, const uint32_t* type);
GAME_FN(uint32_t, __cdecl, BdGetGlobal, 0x0072AE50, const uint32_t* type);
GAME_FN(void, __cdecl, BdCacheGlobals, 0x0072AF50);
GAME_FN(void*, __cdecl, BdMalloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, BdFree, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, BdKey_Destruct, 0x0049A650, uint32_t* key);   // (does nothing)
GAME_FN(uint32_t*, __thiscall, BdKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, BdKey_FromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(void, __thiscall, BdKey_SetName, 0x005FA890, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, BdKey_Assign, 0x005772F0, uint32_t* key, uint32_t value);
GAME_FN(void, __thiscall, BdTrackBase_Destruct, 0x0061A8B0, uint8_t* track);
GAME_FN(uint8_t*, __thiscall, BdRgba_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);

// the node a path leads to from the root (null when a step is missing)
ButesNode* WalkPath(const ButesPath* path) {
    ButesNode* node = bdRoot;
    if (node == nullptr) {
        return nullptr;
    }
    for (int i = 0; i < path->count; i++) {
        node = BdNode_FindChild(node, &path->keys[i]);
        if (node == nullptr) {
            return nullptr;
        }
    }
    return node;
}

} // namespace

namespace butesdb {

// 0x72AE50: the root's value of a type (a global Butes object)
uint32_t __cdecl GetGlobal(const uint32_t* type) {
    int count = int(bdRoot->numValues);
    for (int i = 0; i < count; i++) {
        if (*type == *BdNode_ValueAt(bdRoot, i)) {
            return BdNode_ValueDataAt(bdRoot, i);
        }
    }
    return 0;
}

// 0x72AEA0: the value of a type at the end of a path
uint32_t __cdecl Find(const ButesPath* path, const uint32_t* type) {
    ButesNode* node = WalkPath(path);
    if (node == nullptr) {
        return 0;
    }
    return BdNode_Get(node, type);
}

// 0x72AEF0: the first node in a subtree (depth first) named `key` that has a value of
// the type: that value
uint32_t __cdecl FindInTree(ButesNode* node, const uint32_t* key, const uint32_t* type) {
    if (*key == node->key) {
        uint32_t value = BdNode_Get(node, type);
        if (value != 0) {
            return value;
        }
    }
    for (uint32_t i = 0; i < node->numChildren; i++) {
        uint32_t value = BdFindInTree(BdNode_ChildAt(node, i), key, type);
        if (value != 0) {
            return value;
        }
    }
    return 0;
}

// 0x72AF50: the global Butes objects looked up once (by class name)
void __cdecl CacheGlobals() {
    static const char* const kNames[] = {
        "FightCamButes", "CoasterCamButes", "VehicleCamButes", "FollowCam3Butes", "DodgeballCamButes",
        "DodgeballPhysicsButes", "DodgeballAIButes", "DodgeballAIRampButes", "DodgeballGameButes", "AIButes",
        "FightCollisionButes", "ButtonTriggerButes", "TimePeriodButes", "TimeCycleButes", "DodgeballArenaButes",
        "DebugPlayerButes", "PlayerButes", "FrisbeeButes", "FirstPersonCamButes", "OffsetCamButes",
        "ProjectileButes", "CrosshairButes", "ActionMapButes", "PickupButes", "ObjectButes",
        "CinematicButes", "MenuButes", "CinematicShotButes", "CinematicShotGroupButes", "StimulusButes",
        "PunishmentButes", "PropButes", "PropExtraButes", "PickupListButes", "BikeButes",
        "PhotoButes", "TaggingButes", "SurfaceSoundRangeButes", "AreaTransitionButes", "TrafficButes",
        "PlayerAwarenessButes", "StoreCamButes", "SoccerPButes", "RadarButes", "ArmWrestlingButes",
        "MiscGameButes", "ArcadeRaceCameraButes", "GlobalSpineControlButes", "SpineControlWeightButes",
    };
    static_assert(sizeof(kNames) / sizeof(kNames[0]) == 49);
    // (the game keeps every key until the end; their destructor does nothing)
    for (int i = 0; i < 49; i++) {
        uint32_t key;
        BdKey_Construct(&key, kNames[i]);
        bdCached[i] = BdGetGlobal(&key);
        BdKey_Destruct(&key);
    }
}

// 0x72B7D0: the number of children of the node at a path
uint32_t __cdecl ChildCount(const ButesPath* path) {
    ButesNode* node = WalkPath(path);
    return node != nullptr ? node->numChildren : 0;
}

// 0x72B810: the value of a type in the index'th child of the node at a path
uint32_t __cdecl GetFromChild(const ButesPath* path, uint32_t index, const uint32_t* type) {
    ButesNode* node = WalkPath(path);
    if (node == nullptr || index >= node->numChildren) {
        return 0;
    }
    return BdNode_Get(BdNode_ChildAt(node, index), type);
}

// 0x72B870: a load callback added at the front
void __cdecl AddCallback(uint32_t fn, uint32_t data) {
    ButesCallback* entry = static_cast<ButesCallback*>(BdMalloc(sizeof(ButesCallback)));
    entry->fn = fn;
    entry->data = data;
    entry->next = bdCallbacks;
    bdCallbacks = entry;
}

// 0x72B8B0: the first callback with that function and data removed
void __cdecl RemoveCallback(uint32_t fn, uint32_t data) {
    ButesCallback* prev = nullptr;
    for (ButesCallback* entry = bdCallbacks; entry != nullptr; prev = entry, entry = entry->next) {
        if (entry->fn == fn && entry->data == data) {
            if (prev == nullptr) {
                bdCallbacks = bdCallbacks->next;
            } else {
                prev->next = entry->next;
            }
            BdFree(entry);
            return;
        }
    }
}

// 0x72BE80: Act/Globals.act loaded, its root, the global Butes objects
void __cdecl Load() {
    bdFile = BdFile_Load("Act/Globals.act");
    bdRoot = BdNode_ChildAt(bdFile, 0);
    BdCacheGlobals();
}

// 0x72BEB0: the first node named `key` with a value of the type under the node at a path
// (the path's walk stops at a missing step, and the search then starts from null: the
// game reads through it)
uint32_t __cdecl FindUnder(const ButesPath* path, const uint32_t* key, const uint32_t* type) {
    ButesNode* node = bdRoot;
    for (int i = 0; node != nullptr && i < path->count; i++) {
        node = BdNode_FindChild(node, &path->keys[i]);
    }
    return BdFindInTree(node, key, type);
}

// ---- the Butes classes' destructors -------------------------------------------------
// The vtable set, each name field cleared in order (kSetName: through 0x5FA890, else
// assigned 0 with 0x5772F0), their destructors (nothing) in reverse, the track base's
// destructor.

constexpr uint32_t kSetName = 0x10000;

template <uint32_t Field>
void ClearField(uint8_t* self) {
    if constexpr ((Field & kSetName) != 0) {
        BdKey_SetName(&At<uint32_t>(self, Field & 0xFFFF), nullptr);
    } else {
        BdKey_Assign(&At<uint32_t>(self, Field), 0);
    }
}
template <uint32_t Vt, uint32_t... Fields>
void __fastcall ButesDtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = Vt;
    (ClearField<Fields>(self), ...);
    constexpr uint32_t offsets[] = {0, (Fields & 0xFFFF)...};
    for (int i = int(sizeof...(Fields)); i >= 1; i--) {
        BdKey_Destruct(&At<uint32_t>(self, offsets[i]));
    }
    BdTrackBase_Destruct(self);
}

// ---- ProjectileButes ----------------------------------------------------------------

// a name field set to none twice over (through a temporary key, then 0)
void ResetName(uint8_t* self, int at) {
    uint32_t key;
    BdKey_FromValue(&key, 0);
    BdKey_Assign(&At<uint32_t>(self, at), key);
    BdKey_Destruct(&key);
    BdKey_Assign(&At<uint32_t>(self, at), 0);
}

// 0x72B910 (thiscall): ProjectileButes' defaults
void __fastcall ProjectileButes_SetDefaults(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.2f;
    At<float>(self, 0x2C) = 0.8f;
    At<float>(self, 0x38) = 10.0f;
    At<float>(self, 0x3C) = 10.0f;
    At<int32_t>(self, 0x1C) = 0xFF;
    At<int32_t>(self, 0x20) = 0xFF;
    At<float>(self, 0x4C) = 0.2f;
    At<int32_t>(self, 0x24) = 0xFF;
    At<int32_t>(self, 0x28) = 0xFF;
    At<float>(self, 0x50) = 0.4f;
    At<int32_t>(self, 0x30) = 5;
    At<uint8_t>(self, 0x34) = 1;
    At<uint8_t>(self, 0x40) = 0;
    At<int32_t>(self, 0x44) = 0x57;
    At<int32_t>(self, 0x48) = 5000;
    ResetName(self, 0x54);
    ResetName(self, 0x58);
    At<float>(self, 0x5C) = 0.001f;
    At<float>(self, 0x60) = 0.2f;
    At<uint8_t>(self, 0x7C) = 0;
    At<int32_t>(self, 0x88) = 0;
    At<float>(self, 0x64) = 0.4f;
    At<uint8_t>(self, 0x8C) = 0;
    At<uint8_t>(self, 0x8D) = 0;
    At<float>(self, 0x68) = -1.0f;
    At<uint8_t>(self, 0x8E) = 0;
    At<float>(self, 0x6C) = -1.0f;
    At<uint8_t>(self, 0x8F) = 1;
    At<float>(self, 0x70) = -1.0f;
    At<uint8_t>(self, 0x90) = 1;
    At<float>(self, 0x74) = -1.0f;
    At<float>(self, 0x78) = -1.0f;
    At<float>(self, 0x80) = 1.0f;
    At<float>(self, 0x84) = 1.0f;
    At<float>(self, 0x94) = 0.0f;
    At<float>(self, 0x9C) = 0.0f;
    At<float>(self, 0x98) = 1.0f;
    At<float>(self, 0xA0) = 0.2f;
    ResetName(self, 0xA4);
    ResetName(self, 0xA8);
    ResetName(self, 0xAC);
    At<int32_t>(self, 0xB0) = 2;
    ResetName(self, 0xB4);
    ResetName(self, 0xB8);
    At<int32_t>(self, 0xBC) = 2;
    ResetName(self, 0xC0);
    At<int32_t>(self, 0xC4) = 2;
    At<int32_t>(self, 0xC8) = 300;
    At<int32_t>(self, 0xCC) = 0;
    At<float>(self, 0xD0) = 0.0f;
    At<float>(self, 0xD8) = 30.0f;
    At<uint8_t>(self, 0xD4) = 0;
    At<uint8_t>(self, 0xDC) = 0;
    At<float>(self, 0xE8) = 0.0f;
    At<uint8_t>(self, 0xDD) = 0;
    At<int32_t>(self, 0xE0) = 0;
    At<float>(self, 0xEC) = 1.0f;
    At<uint8_t>(self, 0xE4) = 1;
    At<int32_t>(self, 0xF0) = 0;
    At<float>(self, 0xF4) = 0.2f;
    At<uint8_t>(self, 0x124) = 0;
    At<float>(self, 0xF8) = 0.03f;
    At<float>(self, 0xFC) = 0.008f;
    At<float>(self, 0x100) = 0.2f;
    At<float>(self, 0x104) = 0.0f;
    At<float>(self, 0x108) = 0.4f;
    At<float>(self, 0x114) = 0.4f;
    At<float>(self, 0x10C) = 0.0f;
    At<float>(self, 0x110) = 0.0f;
    At<float>(self, 0x118) = 10.0f;
    At<float>(self, 0x11C) = 2.0f;
    At<float>(self, 0x120) = 0.0f;
    At<float>(self, 0x128) = 1.0f;
    ResetName(self, 0x12C);
    At<uint8_t>(self, 0x130) = 0;
    BdKey_SetName(&At<uint32_t>(self, 0x134), nullptr);
    BdKey_SetName(&At<uint32_t>(self, 0x138), nullptr);
    BdKey_SetName(&At<uint32_t>(self, 0x13C), nullptr);
    BdKey_SetName(&At<uint32_t>(self, 0x140), nullptr);
    At<uint8_t>(self, 0x144) = 0;
    At<uint8_t>(self, 0x145) = 0;
    uint8_t rgba[4];
    uint8_t* colour = BdRgba_Set(rgba, 0, 0, 0, 0);
    At<uint8_t>(self, 0x146) = colour[0];
    At<uint8_t>(self, 0x147) = colour[1];
    At<uint8_t>(self, 0x148) = colour[2];
    At<uint8_t>(self, 0x149) = colour[3];
    At<int16_t>(self, 0x14A) = -2;
    At<int32_t>(self, 0x14C) = 0;
    At<int16_t>(self, 0x150) = -2;
    At<int16_t>(self, 0x152) = -2;
    At<int32_t>(self, 0x154) = 1;
}

} // namespace butesdb

#define BUTESDB_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

using namespace butesdb;

BUTESDB_REVERSED("Butes::GetGlobal", 0x0072AE50, &GetGlobal);
BUTESDB_REVERSED("Butes::Find", 0x0072AEA0, &Find);
BUTESDB_REVERSED("Butes::FindInTree", 0x0072AEF0, &FindInTree);
BUTESDB_REVERSED("Butes::CacheGlobals", 0x0072AF50, &CacheGlobals);
BUTESDB_REVERSED("Butes::ChildCount", 0x0072B7D0, &ChildCount);
BUTESDB_REVERSED("Butes::GetFromChild", 0x0072B810, &GetFromChild);
BUTESDB_REVERSED("Butes::AddCallback", 0x0072B870, &AddCallback);
BUTESDB_REVERSED("Butes::RemoveCallback", 0x0072B8B0, &RemoveCallback);
BUTESDB_REVERSED("Butes::Load", 0x0072BE80, &Load);
BUTESDB_REVERSED("Butes::FindUnder", 0x0072BEB0, &FindUnder);
BUTESDB_REVERSED("ProjectileButes::SetDefaults", 0x0072B910, &ProjectileButes_SetDefaults);

BUTESDB_REVERSED("FrisbeeButes::Destruct", 0x0072A830, (&ButesDtor<0x009345D0>));
BUTESDB_REVERSED("FirstPersonCamButes::Destruct", 0x0072A840, (&ButesDtor<0x009345E8>));
BUTESDB_REVERSED("OffsetCamButes::Destruct", 0x0072A850, (&ButesDtor<0x00934600, 0x58, 0x5C, 0x64, 0x80>));
BUTESDB_REVERSED("CrosshairButes::Destruct", 0x0072A8C0, (&ButesDtor<0x00934630, 0x18>));
BUTESDB_REVERSED("ActionMapButes::Destruct", 0x0072A8F0, (&ButesDtor<0x00934648>));
BUTESDB_REVERSED("PickupButes::Destruct", 0x0072A900, (&ButesDtor<0x009053B4>));
BUTESDB_REVERSED("ObjectButes::Destruct", 0x0072A910,
                 (&ButesDtor<0x00934660, 0x1C, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C, 0x74, 0x78, 0x8C, kSetName | 0x90,
                             kSetName | 0x94, kSetName | 0x9C, kSetName | 0xA0>));
BUTESDB_REVERSED("CinematicButes::Destruct", 0x0072AA50, (&ButesDtor<0x00934678>));
BUTESDB_REVERSED("MenuButes::Destruct", 0x0072AA60, (&ButesDtor<0x00934690>));
BUTESDB_REVERSED("CinematicShotButes::Destruct", 0x0072AA70, (&ButesDtor<0x00914398>));
BUTESDB_REVERSED("CinematicShotGroupButes::Destruct", 0x0072AA80,
                 (&ButesDtor<0x009346A8, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C>));
BUTESDB_REVERSED("StimulusButes::Destruct", 0x0072AB50, (&ButesDtor<0x009346C0>));
BUTESDB_REVERSED("PunishmentButes::Destruct", 0x0072AB60, (&ButesDtor<0x009346D8, 0x2C>));
BUTESDB_REVERSED("PropButes::Destruct", 0x0072AB90,
                 (&ButesDtor<0x009346F0, kSetName | 0x30, kSetName | 0x34, kSetName | 0x3C, kSetName | 0x40,
                             kSetName | 0x48, kSetName | 0x4C, 0x54, kSetName | 0x58, kSetName | 0x5C, kSetName | 0x60,
                             0x68, 0x6C, 0x70, kSetName | 0x80>));
BUTESDB_REVERSED("PropExtraButes::Destruct", 0x0072ACB0, (&ButesDtor<0x00934708>));
BUTESDB_REVERSED("PickupListButes::Destruct", 0x0072ACC0,
                 (&ButesDtor<0x00934720, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48>));
BUTESDB_REVERSED("BikeButes::Destruct", 0x0072AD60, (&ButesDtor<0x00934738>));
BUTESDB_REVERSED("PhotoButes::Destruct", 0x0072AD70, (&ButesDtor<0x00934750>));
BUTESDB_REVERSED("TaggingButes::Destruct", 0x0072AD80, (&ButesDtor<0x00934768>));
BUTESDB_REVERSED("SurfaceSoundRangeButes::Destruct", 0x0072AD90, (&ButesDtor<0x00934780>));
BUTESDB_REVERSED("AreaTransitionButes::Destruct", 0x0072ADA0, (&ButesDtor<0x00934798>));
BUTESDB_REVERSED("TrafficButes::Destruct", 0x0072ADB0, (&ButesDtor<0x009347B0>));
BUTESDB_REVERSED("PlayerAwarenessButes::Destruct", 0x0072ADC0, (&ButesDtor<0x009347C8>));
BUTESDB_REVERSED("StoreCamButes::Destruct", 0x0072ADD0, (&ButesDtor<0x009347E0>));
BUTESDB_REVERSED("SoccerPButes::Destruct", 0x0072ADE0, (&ButesDtor<0x009347F8>));
BUTESDB_REVERSED("RadarButes::Destruct", 0x0072ADF0, (&ButesDtor<0x00934810>));
BUTESDB_REVERSED("ArmWrestlingButes::Destruct", 0x0072AE00, (&ButesDtor<0x00934828>));
BUTESDB_REVERSED("MiscGameButes::Destruct", 0x0072AE10, (&ButesDtor<0x00934840>));
BUTESDB_REVERSED("ArcadeRaceCameraButes::Destruct", 0x0072AE20, (&ButesDtor<0x00934858>));
BUTESDB_REVERSED("GlobalSpineControlButes::Destruct", 0x0072AE30, (&ButesDtor<0x00934870>));
BUTESDB_REVERSED("SpineControlWeightButes::Destruct", 0x0072AE40, (&ButesDtor<0x00934888>));
BUTESDB_REVERSED("FightCamButes::Destruct", 0x0072BD20, (&ButesDtor<0x00937FCC>));
BUTESDB_REVERSED("ProjectileButes::Destruct", 0x0072BD30,
                 (&ButesDtor<0x00934618, 0x54, 0x58, 0xA4, 0xA8, 0xAC, 0xB4, 0xB8, 0xC0, 0x12C, kSetName | 0x134,
                             kSetName | 0x138, kSetName | 0x13C, kSetName | 0x140>));
