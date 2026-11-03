#pragma once

// CWorld: the map's 36 x 36 sectors of 50 units (-900..900 both ways), each with a
// list of entities per kind, the lists of big buildings and moving entities, and the
// searches over them. (GTA's CWorld; Bully's lists are singly linked 4-byte nodes that
// name their entity by pool and slot rather than by pointer.)

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

struct CEntity;

// A node of an entity list (4 bytes, in the pointer-node pool): bits 0-3 the entity's
// pool (ePoolType), 4-17 its slot, 18-31 the next node's index in
// CPools::ms_pPtrNodes (0x3FFF: none).
struct CPtrNode {
    uint32_t bits;

    static constexpr uint32_t NO_NEXT = 0x3FFF;

    uint32_t NextIndex() const { return bits >> 18; }
    CPtrNode* GetNext() const;
    // (the game's GetEntityInPool, 0x44A3E0)
    CEntity* GetItem() const;
    // what the game writes when it puts a node in front of `next` (nullptr: the end)
    void LinkTo(CPtrNode* next);

    // 0x45D670 (thiscall): the node for an entity (no next)
    CPtrNode* Construct(CEntity* entity);
    // 0x44DF30 (thiscall): unlinks it from a list (not freed; nothing if not in it)
    void RemoveFromList(struct CPtrList* list);
};
static_assert(sizeof(CPtrNode) == 4);

struct CPtrList {
    re::Ptr32<CPtrNode> first;

    // (inlined in the game) a node to the front
    void AddNode(CPtrNode* node) {
        node->LinkTo(first);
        first = node;
    }
    // 0x44E070 (thiscall): unlinks and frees every node
    void Flush();
};
static_assert(sizeof(CPtrList) == 4);

// A sector: its lists by entity kind (what CWorld::Add puts where)
enum eSectorList : int32_t {
    SECTOR_BUILDINGS = 0,  // type 1
    SECTOR_OBJECTS = 1,    // types 4, 6, 7 (objects, prop anims, ...)
    SECTOR_VEHICLES = 2,   // type 2
    SECTOR_PEDS = 3,       // type 3
    SECTOR_DUMMIES = 4,    // type 5
};

struct CSector {
    CPtrList lists[5];
};
static_assert(sizeof(CSector) == 0x14);

// Where a physical entity or a dummy is in the sectors (0x14, in the pool at
// 0xC0F5EC): one per sector list it's in, a doubly linked list on the entity.
struct CEntryInfoNode {
    re::Ptr32<CPtrList> list;        // +0
    re::Ptr32<CPtrNode> node;        // +4
    re::Ptr32<CSector> sector;       // +8
    re::Ptr32<CEntryInfoNode> prev;  // +0xC
    re::Ptr32<CEntryInfoNode> next;  // +0x10
};
static_assert(sizeof(CEntryInfoNode) == 0x14);

struct CEntryInfoList {
    re::Ptr32<CEntryInfoNode> first;
};

namespace CWorld {

constexpr int NUM_SECTORS_X = 36;
constexpr int NUM_SECTORS_Y = 36;

GAME_ARRAY(CSector, ms_aSectors, 0x00C1B178);                  // [36 * 36], row (y) major
GAME_VAR(CPtrList, ms_bigBuildingsList, 0x00C216B8);           // entities with flag +0x58
GAME_VAR(CPtrList, ms_listMovingEntityPtrs, 0x00C1AEA4);
GAME_VAR(CPtrNode*, ms_pNextMovingNode, 0x00C1AE84);           // the moving list's walk (kept valid by removals)
GAME_VAR(uint16_t, ms_nCurrentScanCode, 0x00C1AE88);
GAME_ARRAY(CEntity*, ms_aExtraEntities, 0x00C1AA64);           // [7] (read at 0x45F790)
GAME_VAR(int32_t, ms_numExtraEntities, 0x00C1AE68);

inline CSector* GetSector(int x, int y) {
    return &ms_aSectors[y * NUM_SECTORS_X + x];
}

// 0x45D6B0: into the sectors (vf2) or the big buildings, and the moving list if it moves
void __cdecl Add(CEntity* entity);
// 0x45DD70
void __cdecl Remove(CEntity* entity);
// 0x45D840: every entity's scan code to 0 (when the code wraps)
void __cdecl ClearScanCodes();
// 0x45D450: inside -900..900 both ways
uint8_t __cdecl IsInWorld(float x, float y);
// 0x45D5D0 / 0x45D5E0: a list of up to 7 entities (for the moving list's process)
void __cdecl ClearExtraEntities();
void __cdecl AddExtraEntity(CEntity* entity);

// 0x45D750: the entity with the model nearest to pos in a list, if nearer than
// *nearest (which it lowers); `found` when none is
CEntity* __cdecl FindNearestObjectOfTypeSectorList(int model, const col::CVector* pos, CPtrList* list, CEntity* found, float* nearest);
// 0x45DE60: in the buildings and objects within range (and 4 units)
CEntity* __cdecl FindNearestObjectOfType(int model, float range, const col::CVector* pos);
// 0x45E060: the nearest building or object of the model at pos (within 0xA14A90's range): its
// render-off flag (+0xD0) and its flags +0x20 and +0x3C
void __cdecl SetNearestObjectOfTypeFlags(int model, const col::CVector* pos, int hide, int enable);
// 0x45DB40: every entity of the model in a list (-1: any, -2: those whose vf31 says
// so), not yet scanned. (pos and range aren't looked at)
void __cdecl FindObjectsOfTypeInRangeSectorList(int model, CPtrList* list, const col::CVector* pos, float range, int16_t* count,
                                                int16_t maxCount, re::Ptr32<CEntity>* out);
// 0x45DC00: every entity whose bounding sphere is within range of pos (2D: in x, y)
void __cdecl FindObjectsInRangeSectorList(CPtrList* list, const col::CVector* pos, float range, int twoD, int16_t* count,
                                          int16_t maxCount, re::Ptr32<CEntity>* out);
// 0x45E0D0: over the sectors within range; `lists`: 1 buildings, 2 vehicles, 4 peds,
// 8 objects, 0x10 dummies (0x400 with model -1: the -2 test)
void __cdecl FindObjectsOfTypeInRange(int model, const col::CVector* pos, float range, int16_t* count, int16_t maxCount,
                                      re::Ptr32<CEntity>* out, uint32_t lists);

// 0x45E3C0: FindObjectsOfTypeInRange's twin: every entity whose bounding sphere is
// within range (2D: in x, y), over the sectors within range
void __cdecl FindObjectsInRange(const col::CVector* pos, float range, int twoD, int16_t* count, int16_t maxCount, re::Ptr32<CEntity>* out,
                                uint32_t lists);
// 0x45D9C0: every entity's second scan code (+0x112) to 0
void __cdecl ClearScanCodes2();
// 0x45E690: an entity out of every ped's, vehicle's and object's collision records
// (vehicles: their vf48 too)
void __cdecl RemoveFromCollisionRecords(CEntity* entity);
// 0x45EE40: every vehicle's flag 0x40 of +0x33A
void __cdecl SetAllVehiclesFlag33A(int value);
// 0x45EE90: a simple model with flag 0x80000 (the scans' list flag 0x80 skips them)
uint8_t __cdecl HasModelFlag80000(CEntity* entity);

} // namespace CWorld

// A collision point (0x28): where, the entity hit (a registered reference), the normal
struct CColPoint {
    col::CVector point;  // +0
    re::Ptr32<CEntity> entity;     // +0xC
    col::CVector normal; // +0x10
    int32_t piece;       // +0x1C
    uint16_t surfaceA;   // +0x20
    uint16_t surfaceB;   // +0x22
    float depth;         // +0x24

    // 0x46AA70 (thiscall)
    CColPoint* Assign(const CColPoint* other);
    // 0x45F1F0 (thiscall): the entity (the references moved)
    void SetEntity(CEntity* newEntity);
};
static_assert(sizeof(CColPoint) == 0x28);

// 0x45D4A0: m * v (the matrix's 3x3 and translation; rows of 4 floats)
col::CVector* __cdecl MultiplyMatrixVector(col::CVector* out, const col::CMatrix* m, const col::CVector* v);
