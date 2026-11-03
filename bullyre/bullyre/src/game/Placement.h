#pragma once

// CIplStore: the placement files (.ipb, .ipc when CStreaming::ms_bUseIpc), one slot each
// (streaming id IPB_ID_BASE + slot), GTA San Andreas' IPL store: the bounds of what each
// placed, a quad tree over them (to know which to load near a point), and the ranges of
// pool slots its entities took (to delete them again).
// The slots are a CPool of IplDefs, or - with CColStore::ms_bLoadBoxes, the bounds read
// from Config\Extra\IplStore.bin - a plain array.
// The files are read by IplLoaderImpl (a binary section reader with a loader object per
// section: objects, triggers, points, paths, POIs, rails, ...; their virtual functions
// are further down).

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Pool.h"
#include "reversed.h"

// An IPB's slot (0x78)
struct IplDef {
    col::CRect bounds;          // +0 (empty until the file was read once)
    char name[0x20];            // +0x10
    uint32_t nameHash;          // +0x30
    int32_t area;               // +0x34 the interior it's for (0 outside, -1 at first)
    re::Ptr32<uint8_t> data;              // +0x38 the file, kept after the first read
    int32_t dataSize;           // +0x3C
    // the pool slots its entities took, first/last (7FFF/8000: none)
    int16_t firstBuilding;      // +0x40
    int16_t lastBuilding;       // +0x42
    int16_t firstDummy;         // +0x44
    int16_t lastDummy;          // +0x46
    int16_t firstPropAnim;      // +0x48
    int16_t lastPropAnim;       // +0x4A
    int16_t firstObject;        // +0x4C
    int16_t lastObject;         // +0x4E
    int16_t firstProjectile;    // +0x50
    int16_t lastProjectile;     // +0x52
    int16_t unknown54;          // +0x54 (-1)
    uint8_t unknown56;          // +0x56
    uint8_t loaded;             // +0x57
    uint8_t required;           // +0x58 near enough (set by the quad tree scan)
    uint8_t ignore;             // +0x59 (until its bounds are known)
    uint8_t ignoreWhenDeleted;  // +0x5A
    uint8_t unknown5B;          // +0x5B
    uint8_t haveBounds;         // +0x5C
    uint8_t firstLoad;          // +0x5D (from IplStore.bin: the model info section still to read)
    uint8_t pad5E[2];
    int32_t filePos;            // +0x60 where a load in parts stopped
    int32_t unknown64[5];       // +0x64
};
static_assert(offsetof(IplDef, nameHash) == 0x30);
static_assert(offsetof(IplDef, firstBuilding) == 0x40);
static_assert(offsetof(IplDef, loaded) == 0x57);
static_assert(offsetof(IplDef, filePos) == 0x60);
static_assert(sizeof(IplDef) == 0x78);

// The plain array (0xBD5C78)
struct IplDefArray {
    re::Ptr32<IplDef> data;       // +0
    uint32_t capacity;  // +4
    uint32_t count;     // +8
    // 0x439610 (thiscall): room for `capacity`, the old ones copied; with `setCount` the
    // count becomes the capacity
    void Resize(uint32_t capacity, int setCount);
};
static_assert(sizeof(IplDefArray) == 0xC);

// A placement file being read (0x10): from its file when there's no data
struct IplStream {
    re::Ptr32<void> file;         // +0
    re::Ptr32<uint8_t> data;      // +4
    uint32_t position;  // +8
    uint32_t size;      // +0xC
};

namespace CIplStore {

GAME_VAR(CPool<IplDef>*, ms_pPool, 0x00BD5C4C);
GAME_VAR(IplDefArray, ms_array, 0x00BD5C78);
GAME_VAR(void*, ms_pQuadTree, 0x00BD5B40);
GAME_VAR(CPool<uint8_t>*, ms_pQuadTreeNodePool, 0x00CF3568);   // 0x30-byte nodes, 256
GAME_VAR(int32_t, ms_scanArea, 0x00BD5C84);                     // the quad tree scan's area
GAME_VAR(uint8_t, ms_bSaveStoreBin, 0x00BD5C88);
GAME_ARRAY(char, ms_debugName, 0x00BD5B48);                     // "name [slot]" of the last load

// 0x4387A0 (thiscall): the bounds emptied (the array's constructor)
IplDef* __fastcall IplDef_Construct(IplDef* def, re::Edx edx);
// 0x435A10: the rest as a new slot's
void __cdecl IplDef_Init(IplDef* def);
// 0x436AF0: 100, or the array's count
int __cdecl GetSize();
// 0x436B10 (thiscall on the pool): a free slot, zeroed (nullptr: full)
IplDef* __fastcall Pool_New(CPool<IplDef>* pool, re::Edx edx);
// 0x4396B0 / 0x437BE0 (thiscall): the pools' arrays for `size` slots
void __fastcall Pool_Init(CPool<IplDef>* pool, re::Edx edx);
void __fastcall QuadTreePool_Init(CPool<uint8_t>* pool, re::Edx edx);
// 0x437130 (thunk 0x437D00): -1 when there's none
int __cdecl FindIplSlot(const char* name);
// 0x4371B0 (thunk 0x437D10)
int __cdecl FindIplSlotByHash(uint32_t hash);
// 0x437CB0 (a free slot's: 0x10)
const char* __cdecl GetIplName(int slot);
// 0x438440 (thunk 0x438810)
int __cdecl AddIplSlot(const char* name);
// 0x4384B0 (thunk 0x438840): an entity of the file into its slot ranges
void __cdecl IncludeEntity(int slot, uint8_t* entity);
// 0x4377F0 (thunk 0x437D20): its entities deleted (not while the player is in a
// vehicle from it: then only flagged)
void __cdecl RemoveIpb(int slot);
// 0x4385E0 (thunk 0x438830): every loaded one the streaming doesn't keep
void __cdecl RemoveIpbsNotKept();
// 0x438690 (thunk 0x438820): the same for the triggers, points and paths only
void __cdecl RemoveIpbExtrasNotKept();
// 0x4387C0: the quad tree's node pool
void __cdecl CreateQuadTreeNodePool();
// 0x438850: the props whose placement waited for their trigger
void __cdecl CreatePendingProps();
// 0x4390A0 (thunk 0x439760): the file read (all of it, or on from where it stopped);
// false: not done yet
uint8_t __cdecl LoadIpb(int slot, uint8_t* data, int size, int flag);
// 0x439370 (thunk 0x439770): the ones near `pos` loaded (and with `removeFar` the others removed)
void __cdecl LoadIpls(const float* pos, int removeFar);
// 0x439590 (thunk 0x439780)
void __cdecl ReloadAllIpbs();
// 0x439790: the bounds from Config\Extra\IplStore.bin
void __cdecl LoadStoreBin();
// 0x4398F0 (thunk 0x4399E0)
void __cdecl Initialise();
// 0x4399F0 (thunk 0x439D20): the bounds to Config\Extra\IplStore.bin (CColStore::ms_bSaveBoxes)
void __cdecl SaveStoreBin();
// 0x439B90: load what's near, remove what isn't (the last argument unused)
void __cdecl LoadNear(float x, float y, float z, int);
// 0x439CF0: the same, the position as a vector
void __cdecl LoadNearVector(col::CVector pos, int flag);
// 0x436A70: the quad tree scan's: flags the file required
void __cdecl MarkRequired(const float* xy, IplDef* def);
// 0x436AB0: the files of an area (-1: the current one) required
void __cdecl MarkAreaRequired(const float* pos, int area);

} // namespace CIplStore
