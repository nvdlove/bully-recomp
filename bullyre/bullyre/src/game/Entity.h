#pragma once

// The entities: CPlaceable (a position, or a matrix from the matrix pool), CEntity
// (a model, its Gamebryo object, its place in the world's sectors), CDummy and
// CPhysical (in the sectors through entry-info nodes; moving). GTA's classes, near
// enough; Bully's flags are whole 32-bit fields (+0x1C..+0x104) where GTA has bits.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/World.h"
#include "reversed.h"

// ---- CPlaceable ------------------------------------------------------------------------

// Where a placeable without a matrix is: a position and a heading about z
struct CSimpleTransform {
    col::CVector pos;  // +0
    float heading;     // +0xC

    // 0x46DCA0 (thiscall): into a matrix (rotation about z, the position). (The
    // game also stores uninitialised stack into the matrix's +0x3C; ours leaves it)
    void UpdateMatrix(col::CMatrix* m) const;
    // 0x46D8F0 (thiscall): into a Gamebryo transform (3x3 rotation, then the translation)
    void UpdateNiTransform(float* out) const;
};
static_assert(sizeof(CSimpleTransform) == 0x10);

struct CPlaceable;

// A matrix from the pool (gMatrixList, 0xC2A930): in one of its lists, owned by a
// placeable. (GTA's CMatrixLink)
struct CMatrixLink {
    col::CMatrix m;       // +0 (+0x40: the attached Gamebryo transform)
    re::Ptr32<CPlaceable> owner;    // +0x44
    re::Ptr32<CMatrixLink> prev;    // +0x48
    re::Ptr32<CMatrixLink> next;    // +0x4C

    void Remove() {
        next->prev = prev;
        prev->next = next;
    }
    void Insert(CMatrixLink* after) {
        next = after->next;
        after->next->prev = this;
        prev = after;
        after->next = this;
    }
};
static_assert(offsetof(CMatrixLink, owner) == 0x44);
static_assert(sizeof(CMatrixLink) == 0x50);

// The lists of matrices (heads and tails are dummy links): the placeables', the
// static ones' (taken back, oldest first, when there's no free one), the free ones
struct CMatrixLinkList {
    CMatrixLink head;           // +0
    CMatrixLink tail;           // +0x50
    CMatrixLink allocatedHead;  // +0xA0
    CMatrixLink allocatedTail;  // +0xF0
    CMatrixLink freeHead;       // +0x140
    CMatrixLink freeTail;       // +0x190

    // 0x46D6E0 (thiscall): a free one to the placeables' list (nullptr: none)
    CMatrixLink* AddToList1();
    // 0x46D720 (thiscall): a free one to the static list (nullptr: none)
    CMatrixLink* AddToList2();
};
static_assert(sizeof(CMatrixLinkList) == 0x1E0);

GAME_VAR(CMatrixLinkList, gMatrixList, 0x00C2A930);
// what GetMatrix fills for a placeable without a matrix
GAME_VAR(col::CMatrix, gDummyMatrix, 0x00C2AB18);

struct CPlaceable {
    re::Ptr32<re::Ptr32<void>> vtable;                 // +0
    CSimpleTransform placement;    // +4
    re::Ptr32<CMatrixLink> matrix;           // +0x14

    // (inlined everywhere) the position
    col::CVector& GetPosition() { return matrix ? *reinterpret_cast<col::CVector*>(&matrix->m.m[12]) : placement.pos; }

    // 0x46D9B0: the constructor
    CPlaceable* Construct();
    // 0x46DD00: the destructor (its matrix back to the free list)
    void Destruct();
    // 0x46DE60 vf0: the destructor (flags 1: and freed)
    CPlaceable* Destroy(uint8_t flags);
    // 0x46DD20: its matrix, or gDummyMatrix made from its position
    col::CMatrix* GetMatrix();
    // 0x46DC00: its matrix back to the free list (the position and heading kept)
    void RemoveMatrix();
    // 0x46DD40: a matrix from the static list (the oldest one taken when none is free)
    void AllocateStaticMatrix();
    // 0x46DDF0: a matrix (the oldest static one taken when none is free), if none
    void AllocateMatrix();
    // 0x46DE90: its matrix (only the position and heading when upright, unless forced)
    void SetMatrix(const col::CMatrix* m, int forceMatrix);
    // 0x46D9D0: its position (x, y) in the rectangle (corners in any order)
    uint8_t IsWithinArea(float x1, float y1, float x2, float y2);
    // 0x46DAB0: its position in the box (corners in any order)
    uint8_t IsWithinArea(float x1, float y1, float z1, float x2, float y2, float z2);
};
static_assert(sizeof(CPlaceable) == 0x18);

// ---- CEntity ---------------------------------------------------------------------------

enum eEntityType : uint8_t {
    ENTITY_TYPE_NOTHING = 0,
    ENTITY_TYPE_BUILDING = 1,
    ENTITY_TYPE_VEHICLE = 2,
    ENTITY_TYPE_PED = 3,
    ENTITY_TYPE_OBJECT = 4,
    ENTITY_TYPE_DUMMY = 5,
    ENTITY_TYPE_PROPANIM = 6,  // (CPropAnim)
    ENTITY_TYPE_7 = 7,         // (IsEntityPointerValid: checked by 0x4A77F0 on 0xC2C730)
};

struct CColModel;

struct CEntity : CPlaceable {
    re::Ptr32<void> rwObject;                     // +0x18 its Gamebryo object (byte 0: 1 an atomic, 2 a clump)
    // the flags (whole words; the names are GTA's where the use matches)
    uint32_t flag1C;                    // +0x1C (1 when made)
    uint32_t bUsesCollision;            // +0x20
    uint32_t flag24;                    // +0x24
    uint32_t bIsStatic;                 // +0x28 (vf7 sets it)
    uint32_t flag2C;                    // +0x2C
    uint32_t flag30;                    // +0x30
    uint32_t flag34;                    // +0x34
    uint32_t flag38;                    // +0x38
    uint32_t flag3C;                    // +0x3C (set with +0x20 by 0x45E060; GetHasCollidedWith needs it)
    uint32_t flag40;                    // +0x40 (the moving list's second pass)
    uint32_t flag44;                    // +0x44
    uint32_t flag48;                    // +0x48 (1 when made)
    uint32_t flag4C;                    // +0x4C
    uint32_t flag50;                    // +0x50
    uint32_t flag54;                    // +0x54
    uint32_t bIsBIGBuilding;            // +0x58 (in CWorld's big building list, not the sectors)
    uint32_t flag5C;                    // +0x5C
    uint32_t flag60;                    // +0x60
    uint32_t flag64;                    // +0x64
    uint32_t flag68;                    // +0x68
    uint32_t flag6C;                    // +0x6C
    uint32_t flag70;                    // +0x70
    uint32_t flag74;                    // +0x74
    uint32_t bRemoveFromWorld;          // +0x78 (the moving list's process deletes it)
    uint32_t flag7C;                    // +0x7C
    uint32_t bIsBeingRendered;          // +0x80 (while in Render)
    uint32_t flag84[8];                 // +0x84
    uint32_t flagA4;                    // +0xA4
    uint32_t flagA8;                    // +0xA8
    uint32_t bIsStaticWaitingForCollision;  // +0xAC
    uint32_t flagB0;                    // +0xB0
    uint32_t flagB4;                    // +0xB4
    uint32_t flagB8;                    // +0xB8
    uint32_t bHasPreRenderEffects;      // +0xBC (from its model: SetModelIndexNoCreate)
    uint32_t flagC0;                    // +0xC0
    uint32_t flagC4;                    // +0xC4
    uint32_t flagC8;                    // +0xC8
    uint32_t flagCC;                    // +0xCC
    uint32_t bDontRender;               // +0xD0 (Render returns)
    uint32_t flagD4;                    // +0xD4
    uint32_t flagD8;                    // +0xD8
    uint32_t flagDC;                    // +0xDC
    uint32_t flagE0;                    // +0xE0
    uint32_t flagE4;                    // +0xE4
    uint32_t flagE8;                    // +0xE8 (rendered with its alpha)
    uint32_t bAlwaysOnScreen;           // +0xEC (the on-screen tests say yes)
    uint32_t bIsPhysical;               // +0xF0 (CPhysical: the moving list)
    uint32_t flagF4;                    // +0xF4 (the line tests skip it)
    uint32_t flagF8;                    // +0xF8 (1 when made)
    uint32_t flagFC;                    // +0xFC
    uint32_t flag100;                   // +0x100
    uint32_t flag104;                   // +0x104
    uint8_t typeAndStatus;              // +0x108 bits 0-2 the type, 3-7 the status
    uint8_t unknown109;                 // +0x109
    uint16_t scanCode;                  // +0x10A
    uint16_t randomSeed;                // +0x10C
    int16_t modelIndex;                 // +0x10E
    uint8_t unknown110;                 // +0x110 (0xFF when made)
    uint8_t unknown111;                 // +0x111
    uint16_t scanCode2;                 // +0x112 (cleared by 0x45D9C0)
    re::Ptr32<void> animObject;                   // +0x114 its animated object (clumps with animation)
    uint32_t unknown118;                // +0x118
    float unknown11C;                   // +0x11C

    uint8_t GetType() const { return typeAndStatus & 7; }
    uint8_t GetStatus() const { return typeAndStatus >> 3; }

    // 0x4656A0: the constructor
    CEntity* Construct();
    // 0x467E60: the destructor
    void Destruct();
    // 0x4686E0 vf0: the destructor (flags 1: and freed)
    CEntity* Destroy(uint8_t flags);

    // 0x465EB0 vf2: into the sector lists its bounding rectangle covers
    void Add();
    // 0x4678F0 vf3: out of them
    void Remove();
    // 0x468660 vf8: a new model (the old object deleted, the new one made)
    void SetModelIndex(int16_t model, int arg);
    // 0x465E20 vf9: a new model, no object made
    void SetModelIndexNoCreate(int16_t model);
    // 0x466970 vf11: the rectangle its bounding box covers (top view)
    col::CRect* GetBoundRect(col::CRect* rect);
    // 0x465740 vf22
    uint8_t Vf22();
    // 0x4661B0 vf26: through vf25 (with `out`), then the distance between `out` and vf25's
    // result (0x41A950), returned in st0
    float GetBoundCentreVirtual(col::CVector* out);
    // 0x4661E0 vf27, vf28: twice the bounding radius
    float GetBoundDiameter();
    // 0x467E30 vf33: not a dummy, and its model has flag 0x10000
    uint8_t HasModelFlag10000();

    // 0x4667A0
    void DeleteRwObject();
    // 0x4666F0: an object made elsewhere
    void AttachToRwObject(void* object);
    // 0x466740
    void DetachFromRwObject();
    // 0x465760: (the object's frame: 0x6C9AF0)
    void UpdateRwFrame();
    // 0x465780: from its model (what vf9 stores in +0xBC)
    uint8_t ComputeHasPreRenderEffects();
    // 0x466200: a point in its space to the world
    void TransformFromObjectSpace(col::CVector* out, const col::CVector* offset);
    // 0x466C70: its bounding sphere's center, in the world
    col::CVector* GetBoundCentre(col::CVector* out);
    // 0x465E10
    float GetBoundRadius();
    // 0x466110: minus its bounding box's bottom (-min z)
    float GetDistanceFromCentreOfMassToBaseOfModel();
    // 0x465BA0: its bounding sphere and the sphere touch
    uint8_t GetIsTouching(const col::CVector* center, float radius);
    // 0x465C50: the same, with the squared distance of the centers
    uint8_t GetIsTouching(const col::CVector* center, float radius, float* distanceSq);
    // 0x465D10: GetIsTouching, the sphere grown by its speed (physicals)
    uint8_t GetIsTouchingMoving(const col::CVector* center, float radius);
    // 0x4673B0: GetIsTouching through GetBoundCentre
    uint8_t GetIsTouchingBoundCentre(const col::CVector* center, float radius);
    // 0x467460 / 0x467520: its bounding sphere in the camera's view
    uint8_t GetIsOnScreen();
    uint8_t GetIsOnScreenLarge();
    // 0x4686C0: an object, +0x48 set, and on screen
    uint8_t IsVisibleAndOnScreen();
    // 0x465A60: not every frame: by its seed, once in `frames` (a power of two) frames
    uint8_t IsFrameForRandomUpdate(uint32_t frames);
    // 0x466160: its Gamebryo object's alpha
    void SetRwObjectAlpha(int alpha);
    // 0x467B80: objects and prop anims: a value of theirs (0 for the rest)
    uint32_t GetObjectValue();
    // 0x45D3C0: static, or waiting for its collision
    uint32_t GetIsStatic();
    // 0x465AB0: its matrix (or position and heading) from its Gamebryo object's frame
    void UpdateFromRwFrame();
    // 0x466CC0 vf17: its model's effects; objects with pre-render effects (+0x209 bit 0)
    void PreRender();
    // 0x467BC0: a big building (+0x58, +0x74 set, no collision; +0x5C if its draw
    // distance is 2000 or less)
    void SetupBigBuilding();
    // 0x466CA0: GetBoundCentre without clearing `out` first
    void GetBoundCentreNoClear(col::CVector* out);

    // the virtual calls (the game's, wherever they point)
    void CallAdd() { Virtual<void>(2); }
    void CallRemove() { Virtual<void>(3); }
    void CallSetIsStatic(uint8_t value) { Virtual<void>(7, value); }
    void CallCreateRwObject(uint8_t flag, int arg) { Virtual<void>(10, flag, arg); }
    col::CRect* CallGetBoundRect(col::CRect* rect) { return Virtual<col::CRect*>(11, rect); }
    void CallGetBoundCentre(col::CVector* out) { Virtual<void>(25, out); }
    uint8_t CallVf31() { return Virtual<uint8_t>(31); }
    uint8_t CallHasModelFlag10000() { return Virtual<uint8_t>(33); }

protected:
    template <typename R, typename... A>
    R Virtual(int slot, A... args) {
        return reinterpret_cast<R(__thiscall*)(CEntity*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(this, args...);
    }
};
static_assert(offsetof(CEntity, rwObject) == 0x18);
static_assert(offsetof(CEntity, bUsesCollision) == 0x20);
static_assert(offsetof(CEntity, bIsStatic) == 0x28);
static_assert(offsetof(CEntity, flag3C) == 0x3C);
static_assert(offsetof(CEntity, bIsBIGBuilding) == 0x58);
static_assert(offsetof(CEntity, bRemoveFromWorld) == 0x78);
static_assert(offsetof(CEntity, bIsBeingRendered) == 0x80);
static_assert(offsetof(CEntity, flagA4) == 0xA4);
static_assert(offsetof(CEntity, bIsStaticWaitingForCollision) == 0xAC);
static_assert(offsetof(CEntity, bHasPreRenderEffects) == 0xBC);
static_assert(offsetof(CEntity, flagC4) == 0xC4);
static_assert(offsetof(CEntity, bDontRender) == 0xD0);
static_assert(offsetof(CEntity, bAlwaysOnScreen) == 0xEC);
static_assert(offsetof(CEntity, bIsPhysical) == 0xF0);
static_assert(offsetof(CEntity, flag104) == 0x104);
static_assert(offsetof(CEntity, typeAndStatus) == 0x108);
static_assert(offsetof(CEntity, modelIndex) == 0x10E);
static_assert(offsetof(CEntity, animObject) == 0x114);
static_assert(sizeof(CEntity) == 0x120);

// 0x4657F0: a pointer to a live entity of its type (the pools' checks)
uint8_t __cdecl IsEntityPointerValid(CEntity* entity);
// 0x4657D0 / 0x4657E0: a pointer that's cleared when the entity goes (0x450880 / 0x4508D0)
void __cdecl RegisterReference(CEntity* entity, re::Ptr32<CEntity>* ref);
void __cdecl CleanUpOldReference(CEntity* entity, re::Ptr32<CEntity>* ref);

// ---- CDummy ----------------------------------------------------------------------------

struct CDummy : CEntity {
    CEntryInfoList entryInfoList;  // +0x120
    uint32_t unknown124;           // +0x124

    // 0x465150
    CDummy* Construct();
    // 0x465180
    void Destruct();
    // 0x4651A0 vf2: into the dummy lists of the sectors it covers (not clamped to the map)
    void Add();
    // 0x4653B0 vf3
    void Remove();
};
static_assert(offsetof(CDummy, entryInfoList) == 0x120);
static_assert(sizeof(CDummy) == 0x128);

// ---- CPhysical -------------------------------------------------------------------------

struct CPhysical : CEntity {
    uint32_t unknown120;            // +0x120
    col::CVector moveSpeed;         // +0x124
    col::CVector turnSpeed;         // +0x130
    col::CVector frictionMoveSpeed; // +0x13C
    col::CVector frictionTurnSpeed; // +0x148
    float timeScale;                // +0x154 (1: the time step's factor in ApplyMoveSpeed)
    float mass;                     // +0x158
    float turnMass;                 // +0x15C
    uint32_t unknown160[3];         // +0x160
    col::CVector centreOfMass;      // +0x16C
    CEntryInfoList entryInfoList;   // +0x178
    re::Ptr32<CPtrNode> movingListNode;       // +0x17C
    uint8_t stuckCount;             // +0x180
    uint8_t numCollisionRecords;    // +0x181
    uint8_t unknown182[2];          // +0x182
    re::Ptr32<CEntity> collisionRecords[6];   // +0x184
    uint32_t unknown19C;            // +0x19C
    uint32_t unknown1A0;            // +0x1A0
    uint32_t unknown1A4;            // +0x1A4
    uint32_t unknown1A8;            // +0x1A8
    uint32_t unknown1AC;            // +0x1AC
    uint32_t unknown1B0;            // +0x1B0
    uint16_t unknown1B4;            // +0x1B4
    uint8_t unknown1B6[2];          // +0x1B6
    re::Ptr32<void> unknown1B8;               // +0x1B8 an object it owns (deleted through its vf1)
    uint8_t physicalFlags;          // +0x1BC (2: gravity, 0x10: ...)
    uint8_t physicalFlags2;         // +0x1BD (1: frozen)
    uint8_t unknown1BE[2];          // +0x1BE

    // 0x468E90
    CPhysical* Construct();
    // 0x46A720 vf0 (flags 1: and freed)
    CPhysical* Destroy(uint8_t flags);
    // 0x4691E0 vf2: into its sectors' lists (with entry-info nodes)
    void Add();
    // 0x46A770 vf3
    void Remove();
    // 0x469470: its entry-info nodes moved to the sectors it's in now (added or freed as needed)
    void RemoveAndAdd();
    // 0x4690E0 vf11: its bounding sphere's square
    col::CRect* GetBoundRect(col::CRect* rect);
    // 0x469130: the same, grown by its speed when +0xC4 and not static
    col::CRect* GetMovingBoundRect(col::CRect* rect);
    // 0x4697D0 / 0x469840
    void AddToMovingList();
    void RemoveFromMovingList();
    // 0x468D40 / 0x468D80
    uint8_t GetHasCollidedWith(CEntity* entity);
    void RemoveCollisionRecord(CEntity* entity);
    // 0x468DF0
    void ResetCollisionState();
    // 0x46A690: its rotation (a matrix's, kept at its position) and heading (z)
    void SetOrientation(float x, float y, float z);
    // 0x4698B0 vf37: the position moved by the speed over the time step (and gravity)
    void ApplyMoveSpeed();
};
static_assert(offsetof(CPhysical, moveSpeed) == 0x124);
static_assert(offsetof(CPhysical, timeScale) == 0x154);
static_assert(offsetof(CPhysical, centreOfMass) == 0x16C);
static_assert(offsetof(CPhysical, entryInfoList) == 0x178);
static_assert(offsetof(CPhysical, movingListNode) == 0x17C);
static_assert(offsetof(CPhysical, collisionRecords) == 0x184);
static_assert(offsetof(CPhysical, unknown1B4) == 0x1B4);
static_assert(offsetof(CPhysical, physicalFlags) == 0x1BC);
static_assert(sizeof(CPhysical) == 0x1C0);

// 0x465710: the game's clock (0xCF3DA0's milliseconds) in seconds
float __cdecl GetClockSeconds();
// 0x468CD0 (thiscall): a . b
float __fastcall DotProduct(const col::CVector* a, re::Edx edx, const col::CVector* b);

// 0x468E10: the entity if a ped (nullptr otherwise)
CEntity* __cdecl AsPed(CEntity* entity);
