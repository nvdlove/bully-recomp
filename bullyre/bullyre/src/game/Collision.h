#pragma once

// Collision: the models (CColModel, CCollisionData), CColStore's .col files (in
// a pool of slots; streaming id COL_ID_BASE + slot) and their areas, and some of
// the tests. (GTA San Andreas' design; names follow it where the code matches.)

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

struct CEntity;

// (in a namespace: the math types aren't shared yet, other files may have their own)
namespace col {

struct CVector {
    float x, y, z;
};

// GTA's CRect: top is the larger y. Empty: left/bottom 1e6, right/top -1e6.
struct CRect {
    float left;    // +0 min x
    float top;     // +4 max y
    float right;   // +8 max x
    float bottom;  // +0xC min y

    // 0x56ED60 (thiscall): (sets all of eax: 0 / 1)
    uint32_t OverlapsWith(const CRect* other) const;
};
static_assert(sizeof(CRect) == 16);

// The game's CMatrix: 3x4 in four rows of 4 floats (right, forward, up, pos), and
// an attached matrix (unused by the ones built on the stack here).
struct CMatrix {
    float m[16];
    re::Ptr32<void> attached;  // +0x40
};
static_assert(sizeof(CMatrix) == 0x44);

} // namespace col

struct CColSphere {
    col::CVector center;  // +0
    float radius;         // +0xC
    uint8_t surface;      // +0x10
    uint8_t piece;        // +0x11
    uint8_t pad[2];

    // 0x56EB80 (thiscall)
    void Set(float radius, const col::CVector* center);
    // 0x56EBA0 (thiscall)
    void Set(float radius, const col::CVector* center, uint8_t surface, uint8_t piece);
    // 0x56EBD0 (thiscall): where the line origin + t * dir enters and leaves the
    // sphere (dir: unit length); false when it misses
    uint8_t IntersectRay(const col::CVector* origin, const col::CVector* dir, col::CVector* entry, col::CVector* exit) const;
};
static_assert(sizeof(CColSphere) == 0x14);

// A triangle's corners: indices into the vertices
struct CColTriangle {
    uint16_t a, b, c;  // +0
    uint8_t surface;   // +6
    uint8_t light;     // +7
};
static_assert(sizeof(CColTriangle) == 8);

// Vertices are stored as 1/128ths of a unit
struct CompressedVector {
    int16_t x, y, z;
};
static_assert(sizeof(CompressedVector) == 6);

// A model's collision volumes (0x24)
struct CCollisionData {
    re::Ptr32<CColSphere> spheres;            // +0
    re::Ptr32<void> unknown04;                // +4 (an array, freed with the rest)
    re::Ptr32<struct CColBox> boxes;          // +8 (0x24 each)
    re::Ptr32<CompressedVector> vertices;     // +0xC
    re::Ptr32<CColTriangle> triangles;        // +0x10
    re::Ptr32<struct TrianglePlanes> planes;  // +0x14 (an object: destroyed, then freed)
    re::Ptr32<void> unknown18;                // +0x18 (an array; while set, the model has something registered: see RemoveCollisionVolumes)
    int16_t numTriangles;           // +0x1C
    uint8_t numSpheres;             // +0x1E
    uint8_t numBoxes;               // +0x1F
    uint8_t unknown20;              // +0x20 (the count for +4)
    uint8_t pad[3];

    // 0x56DAD0 (thiscall): returns this
    CCollisionData* Construct();
    // 0x56DB00 (thiscall): frees the volumes
    void RemoveCollisionVolumes();
};
static_assert(offsetof(CCollisionData, numTriangles) == 0x1C);
static_assert(offsetof(CCollisionData, unknown20) == 0x20);
static_assert(sizeof(CCollisionData) == 0x24);

// A model's collision (0x40, in the pool at 0xC0F604)
struct CColModel {
    col::CVector sphereCenter;  // +0 the bounding sphere
    float sphereRadius;         // +0xC
    col::CVector boxMin;        // +0x10 the bounding box (16-byte vectors)
    float unknown1C;            // +0x1C
    col::CVector boxMax;        // +0x20
    float unknown2C;            // +0x2C
    uint16_t colSlot;         // +0x30 the CColStore slot it came from (0: none)
    uint16_t unknown32;         // +0x32
    uint8_t unknown34;          // +0x34
    uint8_t unknown35[3];
    re::Ptr32<CCollisionData> data;       // +0x38 nullptr: not loaded
    re::Ptr32<void> owner;                // +0x3C the placeable whose matrix places it (for the world-space getters)

    // 0x56E7A0 (thiscall): returns this
    CColModel* Construct();
    // 0x56E7D0 (thiscall): frees the collision data
    void RemoveCollisionVolumes();
    // 0x56E770 (thiscall)
    void SetOwner(void* placeable);
    // 0x56E680 (thiscall): a vertex, in model space
    void GetVertex(col::CVector* out, int index) const;
    // 0x56E6E0 (thiscall): a vertex, in world space (through the owner's matrix)
    void GetWorldVertex(col::CVector* out, int index) const;
    // 0x56E820 (thiscall): a vertex from a world position (through the inverse)
    void SetWorldVertex(int index, const col::CVector* pos);
    // 0x56E8C0 (thiscall): a triangle's edge (0..2), in world space; false when
    // there's no such triangle or no owner
    uint8_t GetWorldTriangleEdge(col::CVector* start, col::CVector* end, int triangle, int edge) const;
    // 0x56E9C0 (thiscall): a triangle's corners, in world space
    void GetWorldTriangle(int triangle, col::CVector* corners) const;

    // 0x56E7C0: from the pool (the size is ignored)
    static CColModel* __cdecl New(size_t size);
    // 0x56EB30: back to the pool
    static void __cdecl Delete(CColModel* col);
};
static_assert(offsetof(CColModel, boxMin) == 0x10);
static_assert(offsetof(CColModel, boxMax) == 0x20);
static_assert(offsetof(CColModel, colSlot) == 0x30);
static_assert(offsetof(CColModel, unknown34) == 0x34);
static_assert(offsetof(CColModel, data) == 0x38);
static_assert(sizeof(CColModel) == 0x40);

// A .col file's slot (0x30)
struct ColDef {
    uint32_t areas[2];        // +0 a bit per area (CGame::currArea) it's used in
    col::CRect bounds;        // +8 around its models' placements
    uint32_t nameHash;        // +0x18
    re::Ptr32<void> data;               // +0x1C the file, kept once read (nullptr: never read)
    uint32_t size;            // +0x20
    int16_t firstModel;       // +0x24 the model ids in it (0x7FFF / -0x8000 when made)
    int16_t lastModel;        // +0x26
    uint8_t loaded;           // +0x28 its models have their collision
    uint8_t loadAtStart;      // +0x29 (GenerateAllBoundingBoxes' quick path loads these)
    uint8_t unknown2A;        // +0x2A
    uint8_t unknown2B;        // +0x2B
    uint8_t unknown2C[4];

    // 0x56EE70 (thiscall): the bounds emptied (only)
    ColDef* Construct();
    // 0x56ED20 (thiscall): its name hash in hex, in a static buffer
    const char* GetName() const;
};
static_assert(offsetof(ColDef, bounds) == 8);
static_assert(offsetof(ColDef, nameHash) == 0x18);
static_assert(offsetof(ColDef, firstModel) == 0x24);
static_assert(offsetof(ColDef, loaded) == 0x28);
static_assert(sizeof(ColDef) == 0x30);

// 0x56EDF0 (thiscall): a free ColDef slot, zeroed (nullptr: none)
ColDef* __fastcall ColDefPool_New(CPool<ColDef>* pool, re::Edx edx);
// 0x56F4C0 (thiscall): the arrays for `size` slots, all free (the object array
// has a count before it when the pool owns them)
void __fastcall ColDefPool_Init(CPool<ColDef>* pool, re::Edx edx);

// Finds the entities in a rectangle through the world's sectors, filtered by a
// callback, into a growing array. Built on the stack by LoadCol/RemoveCol.
struct EntityRectScan {
    re::Ptr32<uint8_t __cdecl(int user, CEntity* entity)> filter;  // +0
    col::CRect rect;                                      // +4
    uint32_t lists;       // +0x14 which of a sector's lists (1, 2, 4, 8, 0x10); 0x80: skip some entities
    int32_t user;         // +0x18 for the filter
    re::Ptr32<re::Ptr32<CEntity>> entities;   // +0x1C
    uint32_t capacity;    // +0x20
    uint32_t count;       // +0x24

    // 0x56F8D0 (thiscall): the scan; how many were found
    uint32_t Scan();
    // 0x56EFD0 (thiscall): one sector list
    void ScanList(re::Ptr32<void>* list);
};
static_assert(offsetof(EntityRectScan, lists) == 0x14);
static_assert(offsetof(EntityRectScan, count) == 0x24);

// A sphere tested against entities' collision (with the one-sphere model of
// CColSphereModel), collecting the contact points.
struct CColSphereQuery {
    col::CVector center;   // +0
    float radius;          // +0xC
    col::CMatrix matrix;   // +0x10 placed at the center
    uint32_t lists;        // +0x54 which sector lists (as EntityRectScan's)
    re::Ptr32<uint8_t> colPoints;    // +0x58 0x28 each; the entity at +0xC (a registered reference)
    int32_t numColPoints;  // +0x5C
    int32_t unknown60;     // +0x60

    // 0x5707D0 (thiscall): sets up the one-sphere model too
    CColSphereQuery* Construct(const col::CVector* center, float radius, uint32_t lists, uint8_t* colPoints, int32_t unknown60);
    // 0x570AA0 (thiscall): the sphere against an entity's collision; the points found
    int ProcessEntity(CEntity* entity);
    // 0x570B40 (thiscall): the sphere against the entities around it; the points found
    int Scan();
    // 0x5708E0 (thiscall): one sector list (entities with collision, in the current
    // chapter, the sphere touching their bounds), the points added
    void ScanList(re::Ptr32<void>* list);
};
static_assert(offsetof(CColSphereQuery, matrix) == 0x10);
static_assert(offsetof(CColSphereQuery, lists) == 0x54);
static_assert(offsetof(CColSphereQuery, unknown60) == 0x60);

namespace CColStore {

GAME_VAR(CPool<ColDef>*, ms_pColPool, 0x00CF306C);
GAME_VAR(uint8_t, ms_bRemovingAll, 0x00CF3071);       // RemoveCol leaves the models alone
GAME_VAR(uint8_t, ms_bGeneratingBoxes, 0x00CF3072);   // LoadCol leaves the entities alone
GAME_VAR(uint8_t, ms_bSaveBoxes, 0x00CF3073);         // SaveBoundingBoxes writes ColStore.bin
GAME_VAR(uint8_t, ms_bLoadBoxes, 0x00CF3074);         // the boxes come from ColStore.bin
// names (hashes) HasCollisionLoadedExcept ignores
GAME_ARRAY(uint32_t, ms_aIgnoredNames, 0x00CF307C);   // 10
GAME_VAR(uint32_t, ms_numIgnoredNames, 0x00CF30A4);
GAME_ARRAY(char, ms_nameBuffer, 0x00CF30A8);          // GetName's (8 digits)

// 0x5705A0: the pool (500) and the "generic" slot 0
void __cdecl Initialise();
// 0x570620
void __cdecl Shutdown();
// 0x56F840: a new slot for a file
int __cdecl AddColSlot(const char* name);
// 0x56EF10: -1: none
int __cdecl FindColSlot(const char* name);
// 0x56EEE0
const char* __cdecl GetColName(int slot);
// 0x56EE90: the file is used in this area
void __cdecl SetArea(int slot, int area);
// 0x56F0F0
col::CRect* __cdecl GetBoundingBox(int slot);
// 0x56F120: widens the file's model id range
void __cdecl IncludeModelIndex(int slot, int model);
// 0x56F3C0 / 0x56F3F0
int __cdecl GetFirstModel(int slot);
int __cdecl GetLastModel(int slot);
// 0x56F420
void __cdecl SetLoadAtStart(int slot, uint8_t value);
// 0x56F450
uint8_t __cdecl IsLoaded(int slot);
// 0x56F480: the file has been read (its data is kept)
uint8_t __cdecl HasData(int slot);
// 0x56EF60: the filter LoadCol/RemoveCol scan with: the entity's model is the file's
uint8_t __cdecl IsEntityFromCol(int slot, CEntity* entity);
// 0x56EDB0 / 0x56EDE0
void __cdecl AddIgnoredName(const char* name);
void __cdecl ClearIgnoredNames();

// 0x56FAC0: gives the file's models their collision (the file's data is kept)
uint8_t __cdecl LoadCol(int slot, void* data, size_t size);
// 0x56FC40: takes it back
void __cdecl RemoveCol(int slot);

// 0x56F160 / 0x56FF90: the files' bounding boxes from / to Config\Extra\ColStore.bin
void __cdecl LoadBoundingBoxes();
void __cdecl SaveBoundingBoxes();
// 0x56F330: every file, now
void __cdecl LoadAllCollision();
// 0x56F260: every file nothing keeps in memory, and the models' collision from them
void __cdecl RemoveAllCollision();
// 0x570470: at start: reads every file to get the bounding boxes
void __cdecl GenerateAllBoundingBoxes();
// 0x56F570: the files' areas, from Coll\ColAreas.cfg
void __cdecl LoadColAreas();

// 0x56FE00: the files around a position (and the other load point) in, the rest out
void __cdecl LoadCollision(const col::CVector* pos);
// 0x570080: the files around a position, read now
void __cdecl EnsureCollisionIsInMemory(const col::CVector* pos);
// 0x5701C0: a loaded file of the current area covers the position
uint8_t __cdecl HasCollisionLoaded(const col::CVector* pos);
// 0x570260: the same, not counting the ignored names
uint8_t __cdecl HasCollisionLoadedExcept(const col::CVector* pos);
// 0x570330: the area of the loaded file that covers the position, for files of one
// area only
uint8_t __cdecl FindAreaAt(const col::CVector* pos, int* area);

} // namespace CColStore

// CColModel's helpers for the sphere tests (a one-sphere model)
namespace CColSphereModel {
GAME_VAR(CColModel*, ms_pColModel, 0x00CF30B4);
GAME_VAR(CCollisionData*, ms_pColData, 0x00CF30B8);
GAME_VAR(CColSphere*, ms_pSphere, 0x00CF30BC);
// 0x570710 / 0x570790
void __cdecl Initialise();
void __cdecl Shutdown();
} // namespace CColSphereModel

// CTempColModels: the built-in collision models (0xCF3120..0xCF3158)
namespace CTempColModels {
GAME_ARRAY(CColModel*, ms_colModels, 0x00CF3120);   // 15; the first is the one for models without collision
// 0x571240
void __cdecl Shutdown();
} // namespace CTempColModels

// The surface table (CONFIG\Dat\SURFDATA.DAT)
struct CSurfaceTable {
    uint8_t unknown00[0x43];  // +0 a byte per surface
    uint8_t pad43;
    re::Ptr32<uint32_t> flags;          // +0x44 per surface, 6 bits (counted array)
    uint8_t loaded;           // +0x48
    uint8_t pad49[3];

    // 0x570D40 (thiscall)
    void Load();
};
static_assert(offsetof(CSurfaceTable, flags) == 0x44);
static_assert(offsetof(CSurfaceTable, loaded) == 0x48);

namespace SurfaceTable {
GAME_VAR(CSurfaceTable, ms_table, 0x00CF30C0);
// 0x570F40: loaded on first use
CSurfaceTable* __cdecl Get();
// 0x570F60
void __cdecl Shutdown();
} // namespace SurfaceTable
