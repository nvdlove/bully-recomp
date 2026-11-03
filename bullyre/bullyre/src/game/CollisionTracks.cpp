// The action tree's collision tracks: BaseCollisionTrack / CollisionTrack sweep a sphere
// (radius +0x5C) along a bone (+0x210) of the playing ped between two times, and hit
// the peds, vehicles and objects in its way (up to 16 hits, 0x18 bytes each at +0x80:
// the entity, how long ago, where, which body sphere); each hit entity is damaged and,
// for peds, sent to the node of the track's first reference (+0x60). The sweep tests
// the world sectors' lists (ped lists: peds' body spheres; vehicle / object lists:
// their collision models). 0x5F7720..0x5F9FD0 (the fields' initialiser is in
// ActionTracks.cpp).
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "game/ActionTree.h"
#include "reversed.h"

using namespace acttree;

namespace {

struct Vec3 {
    float x, y, z;
};
// a hit (0x18 bytes, 16 of them at +0x80)
struct CollisionHit {
    re::Ptr32<uint8_t> entity;  // +0 a registered reference
    float age;        // +4
    Vec3 pos;         // +8
    int32_t sphere;   // +0x14
};
static_assert(sizeof(CollisionHit) == 0x18);
// CMatrix (0x412770 makes the identity; +0x40 the attached matrix)
struct GameMatrix {
    float m[12];       // +0x0 right, up, at (each padded)
    float pos[4];      // +0x30
    uint32_t attach;   // +0x40
    uint32_t owns;     // +0x44
};
static_assert(sizeof(GameMatrix) == 0x48);
// CColPoint
struct GameColPoint {
    Vec3 pos;          // +0x0
    re::Ptr32<uint8_t> entity;   // +0xC a registered reference
    Vec3 normal;       // +0x10
    uint8_t pad1C[7];  // +0x1C
    uint8_t piece;     // +0x23
    float depth;       // +0x24
    uint32_t pad28;
};
static_assert(offsetof(GameColPoint, depth) == 0x24);
// CColLine
struct GameColLine {
    float start[4];
    float end[4];
};

constexpr uint32_t kVtBaseCollisionTrack = 0x0092DEA8;
constexpr uint32_t kVtCollisionTrack = 0x0092DECC;
constexpr uint32_t kVtReference = 0x0092DAAC;

GAME_VAR(uint16_t, scanCode, 0x00C1AE88);          // CWorld::ms_nCurrentScanCode
GAME_VAR(uint8_t*, ignoreEntity, 0x00C1AE70);      // skipped by the line of sight test
GAME_VAR(uint32_t*, sectorLinks, 0x00C0F788);      // the sectors' list nodes
GAME_VAR(uint8_t*, sweepColModel, 0x00CF3140);     // the swept sphere's collision model
GAME_VAR(uint8_t, currentChapter, 0x00A147E0);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);          // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint8_t*, pedPool, 0x00C0F5F0);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(double, sweepMargin_900970, 0x00900970);  // 4
GAME_VAR(double, sectorSize_900AA0, 0x00900AA0);   // 50
GAME_VAR(double, sectorOffset_900A98, 0x00900A98); // 18
GAME_VAR(float, sectorMax_900A94, 0x00900A94);     // 35
GAME_VAR(double, headHeight_900E48, 0x00900E48);   // 0.45
GAME_VAR(float, twoTo32_900AC8, 0x00900AC8);
GAME_ARRAY(uint32_t*, sectorEntities_C1B17C, 0x00C1B17C);   // [36 * 36] of 5 lists: objects...
GAME_ARRAY(uint32_t*, sectorEntities_C1B180, 0x00C1B180);   // ...vehicles
GAME_VAR(uint32_t, grappleName_1B652A8, 0x01B652A8);
GAME_VAR(void*, soundBanks_B6F1E8, 0x00B6F1E8);
inline void* const soundStreams_CF3DA0 = reinterpret_cast<void*>(0x00CF3DA0);

GAME_FN(void, __cdecl, Entity_CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __cdecl, Entity_RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);
GAME_FN(void, __thiscall, Node_Destruct, 0x005F4E50, uint8_t* node);
GAME_FN(void, __thiscall, TrackBase_Destruct, 0x0061A8B0, uint8_t* track);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t*, __thiscall, Reference_Get0C, 0x005F4F90, uint8_t* ref, int32_t key);
GAME_FN(void, __thiscall, Reference_ResolveByPath, 0x005F5F50, uint8_t* ref);
GAME_FN(void, __thiscall, Reference_ClearTarget, 0x005F55A0, uint8_t* ref);
GAME_FN(void, __thiscall, BaseCollisionTrack_Destruct, 0x005F8200, uint8_t* track);
GAME_FN(uint8_t, __thiscall, BaseCollisionTrack_Start, 0x005F95A0, uint8_t* track, uint8_t* owner);
GAME_FN(uint8_t, __thiscall, Track_AddHit, 0x005F8A50, uint8_t* track, uint8_t* entity, const Vec3* pos, int32_t sphere);
GAME_FN(void, __thiscall, Track_AgeHits, 0x005F8AE0, uint8_t* track, float dt, float maxAge);
GAME_FN(uint8_t, __thiscall, Track_TestObjectList, 0x005F8B60, uint8_t* track, uint32_t* list, const Vec3* losStart,
        const Vec3* pos, float radius, float);
GAME_FN(uint8_t, __thiscall, Track_TestPed, 0x005F8F80, uint8_t* track, uint8_t* ped, const Vec3* losStart, const Vec3* pos,
        float radius, float margin, Vec3* hitPos, int32_t* sphere);
GAME_FN(uint8_t, __thiscall, Track_TestVehicleList, 0x005F92F0, uint8_t* track, uint32_t* list, const Vec3* pos, float radius,
        float impulse, uint8_t* damager);
GAME_FN(uint8_t, __thiscall, Track_TestObjectSectors, 0x005F9920, uint8_t* track, const Vec3* losStart, const Vec3* pos,
        float radius, float margin);
GAME_FN(uint8_t, __thiscall, Track_TestPeds, 0x005F9B00, uint8_t* track, uint8_t* list, const Vec3* losStart, const Vec3* pos,
        float radius, float margin);
GAME_FN(uint8_t, __thiscall, Track_TestVehicleSectors, 0x005F9C70, uint8_t* track, const Vec3* pos, float radius, float impulse,
        uint8_t* damager);
GAME_FN(uint8_t, __thiscall, Track_Sweep, 0x005F9E50, uint8_t* track, const Vec3* losStart, const Vec3* pos, float radius,
        float margin);
GAME_FN(uint8_t*, __cdecl, GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_FN(uint8_t, __thiscall, Entity_IsTouchingSphere, 0x00465D10, uint8_t* entity, const Vec3* centre, float radius);
GAME_FN(GameMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, uint8_t* placeable);
GAME_FN(uint8_t*, __cdecl, GetColModel, 0x0051AF50, uint8_t* entity);
GAME_FN(void, __thiscall, Matrix_Construct, 0x00412770, GameMatrix* matrix);
GAME_FN(uint8_t, __cdecl, ProcessColModels, 0x0056C8C0, GameMatrix* a, GameMatrix* b, uint8_t* colA, GameMatrix* matrixB,
        GameMatrix* matrixB2, uint8_t* colB, GameColPoint* points, int, int, int, int, int, int, uintptr_t, int);
GAME_FN(void, __thiscall, ColLine_Set, 0x00564950, GameColLine* line, const void* start, const void* end);
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, GameMatrix* from, GameMatrix* to);
GAME_FN(void, __cdecl, Matrix_TransformPoints, 0x00414DB0, void* out, int count, GameMatrix* matrix, void* in, int stride);
GAME_FN(uint8_t, __cdecl, ColLine_TestTriangles, 0x00564E90, void* line, uint8_t* colData, uint8_t* triangles);
GAME_FN(uint8_t, __cdecl, World_ProcessLineOfSight, 0x00461EA0, const Vec3* start, const Vec3* end, GameColPoint* point,
        re::Ptr32<uint8_t>* entity, int flags, int, int);
GAME_FN(void, __cdecl, World_AdvanceScanCode, 0x00415260);
GAME_FN(void, __cdecl, World_ClearScanCodes, 0x0045D840);
GAME_FN(Vec3*, __cdecl, VectorSub, 0x00414B70, Vec3* out, const Vec3* a, const Vec3* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, Vec3* v);
GAME_FN(Vec3*, __thiscall, Placeable_GetPosition, 0x00415460, uint8_t* placeable);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, uint8_t* vehicle, int32_t* out);
GAME_FN(uint8_t, __thiscall, Ped_GetBodySphere, 0x00474630, uint8_t* ped, int index, Vec3* centre, float* radius);
GAME_FN(float, __cdecl, DistToLineSegment, 0x00574A00, const Vec3* point, const void* segment, uintptr_t);
GAME_FN(void, __thiscall, Automobile_SetDamage, 0x004A8410, uint8_t* automobile, int piece, float impulse, uint8_t* damager,
        const Vec3* normal);
GAME_FN(uint8_t*, __thiscall, Ped_GetFightTarget_497100, 0x00497100, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, PedTargeting_GetTarget_4D7780, 0x004D7780, uint8_t* targeting);
GAME_FN(void, __thiscall, Box_Set, 0x00564830, void* box, const Vec3* min, const Vec3* max);
GAME_FN(uint8_t*, __cdecl, Clump_GetHierarchy, 0x005EE1A0, void* clump);
GAME_FN(uint8_t*, __cdecl, Hierarchy_GetMatrices, 0x006C7710, uint8_t* hierarchy);
GAME_FN(uint8_t*, __thiscall, AnimBlend_GetCurrent_6B1F50, 0x006B1F50, uint8_t* clump, int);
GAME_FN(void, __thiscall, Vehicle_Knock_4CC4A0_x64, 0x004CC4A0, uint8_t* vehicle, uint8_t* ped, int, float, float, float, float, uintptr_t, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Vehicle_Knock_4CC4A0(uint8_t* a0, uint8_t* a1, int a2, float a3, Vec3 a4, int a5, int a6) {
    return Vehicle_Knock_4CC4A0_x64(a0, a1, a2, a3, a4.x, a4.y, a4.z, a5, a6);
}
GAME_FN(void, __thiscall, Ped_SetGrapplePartner, 0x004744B0, uint8_t* ped, uint8_t* partner);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ctl_IsPlaying, 0x005F4800, uint8_t* controller, const uint32_t* name, int ancestors, int nested);
GAME_FN(void, __thiscall, Ctl_QueueTracks, 0x005F3F80, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_PlayNode, 0x005F45F0, uint8_t* controller, uint8_t* node);
GAME_FN(uint16_t, __thiscall, SoundBank_GetId, 0x00667B60, void* bank, const char* name);
GAME_FN(int, __thiscall, SoundStreams_GetId, 0x00595B80, void* streams, const char* name);
GAME_FN(void, __thiscall, Stat_Add_4A74C0, 0x004A74C0, uint8_t* stat, float value);
GAME_FN(void, __thiscall, Stat_Add_4DAC30, 0x004DAC30, uint8_t* stat, float value);
GAME_FN(void, __thiscall, Ped_AddStun_47CD90, 0x0047CD90, uint8_t* ped, float time, int);
GAME_FN(void, __thiscall, PedList_SetFlag_48C190, 0x0048C190, uint8_t* list, int);

CollisionHit* Hits(uint8_t* self) {
    return reinterpret_cast<CollisionHit*>(self + 0x80);
}
bool AlreadyHit(uint8_t* self, uint8_t* entity) {
    for (int i = 0; i < 16; i++) {
        if (Hits(self)[i].entity == entity) {
            return true;
        }
    }
    return false;
}
uint8_t* OwnerPed(uint8_t* self) {
    return vc::At<uint8_t*>(vc::At<uint8_t*>(self, 0x20C), 8);
}
// an entity worth testing: with an rw object and collision, in this chapter, not
// hidden, or flagged +0x100
bool Collidable(uint8_t* entity) {
    if (vc::At<void*>(entity, 0x20) && (vc::At<void*>(entity, 0x1C) || vc::At<void*>(entity, 0x60)) &&
        (ms_modelInfoPtrs[vc::At<int16_t>(entity, 0x10E)][0xB] & uint8_t(1 << currentChapter)) && vc::At<int32_t>(entity, 0xF4) == 0) {
        return true;
    }
    return vc::At<int32_t>(entity, 0x100) != 0;
}
// the next node of a sector list (0x3FFF: the end)
uint32_t* NextLink(uint32_t* node) {
    uint32_t next = *node >> 18;
    return next == 0x3FFF ? nullptr : sectorLinks + next;
}
uint8_t* LinkEntity(uint32_t* node) {
    return GetEntityFromNode(int(*node & 0xF), int((*node >> 4) & 0x3FFF));
}
// CTimer's milliseconds (unsigned) plus seconds, truncated
uint32_t TimeAfter(float seconds) {
    double now = double(int32_t(timeInMs));
    if (int32_t(timeInMs) < 0) {
        now = float(now + double(twoTo32_900AC8));
    }
    float ms = float(double(seconds) * 1000.0);
    return uint32_t(int64_t(float(double(ms) + now)));
}

// 0x5F7720 (thiscall): the two references' nodes cleared
void __fastcall Track_ClearNodes(uint8_t* self, re::Edx) {
    Reference_ClearTarget(self + 0x60);
    Reference_ClearTarget(self + 0x70);
}

// 0x5F7740 (thiscall): SetNode: both references' parent
void __fastcall Track_SetNode(uint8_t* self, re::Edx, uint8_t* node) {
    vc::Call<void>(self + 0x60, kNodeSetParent, node);
    vc::Call<void>(self + 0x70, kNodeSetParent, node);
}

// 0x5F7770 (thiscall): the collision track's fields
void __fastcall CollisionTrack_InitFields(uint8_t* self, re::Edx) {
    vc::At<float>(self, 0x18) = 0.0f;
    self[0x58] = 0;
    vc::At<float>(self, 0x1C) = -1.0f;
    self[0x59] = 0;
    vc::At<int32_t>(self, 0x24) = 1;
    vc::At<float>(self, 0x30) = 5.0f;
    vc::At<int32_t>(self, 0x210) = 0x18;
    self[0x215] = 0;
    vc::At<float>(self, 0x34) = 0.0f;
    self[0x216] = 0;
    vc::At<float>(self, 0x3C) = 0.0f;
    self[0x217] = 0;
    vc::At<float>(self, 0x48) = 0.0f;
    self[0x218] = 0;
    vc::At<float>(self, 0x4C) = 0.0f;
    self[8] = 0;
    self[0x214] = 1;
    vc::At<float>(self, 0x38) = -1.0f;
    self[0x219] = 0;
    vc::At<float>(self, 0x40) = -1.0f;
    vc::At<float>(self, 0x44) = -1.0f;
    vc::At<float>(self, 0x50) = 20.0f;
    vc::At<float>(self, 0x54) = 0.0f;
    vc::At<float>(self, 0x5C) = 0.5f;
}

void DestructReference(uint8_t* ref) {
    vc::At<uint32_t>(ref, 0) = kVtReference;
    NameKey_Destruct(ref + 8);
    Node_Destruct(ref);
}

// 0x5F8200 (thiscall): BaseCollisionTrack's destructor (the hits' references let go)
void __fastcall BaseCollisionTrack_DestructImpl(uint8_t* self, re::Edx) {
    vc::At<uint32_t>(self, 0) = kVtBaseCollisionTrack;
    for (int i = 15; i >= 0; i--) {
        CollisionHit& hit = Hits(self)[i];
        if (hit.entity) {
            Entity_CleanUpOldReference(hit.entity, &hit.entity);
        }
    }
    DestructReference(self + 0x70);
    DestructReference(self + 0x60);
    NameKey_Destruct(self + 0x2C);
    NameKey_Destruct(self + 0x28);
    NameKey_Destruct(self + 0x20);
    TrackBase_Destruct(self);
}

// 0x5F82E0 (thiscall): BaseCollisionTrack's deleting destructor
uint8_t* __fastcall BaseCollisionTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    BaseCollisionTrack_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F8300 (thiscall): Resolve: both references' nodes found (looked up by path when not)
uint32_t __fastcall Track_Resolve(uint8_t* self, re::Edx) {
    if (!Reference_Get0C(self + 0x60, -1)) {
        Reference_ResolveByPath(self + 0x60);
    }
    bool first = Reference_Get0C(self + 0x60, -1) != nullptr;
    if (!Reference_Get0C(self + 0x70, -1)) {
        Reference_ResolveByPath(self + 0x70);
    }
    bool second = Reference_Get0C(self + 0x70, -1) != nullptr;
    return first && second ? 1 : 0;
}

// 0x5F8370 (thiscall): CollisionTrack's deleting destructor
uint8_t* __fastcall CollisionTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    vc::At<uint32_t>(self, 0) = kVtCollisionTrack;
    BaseCollisionTrack_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x5F83A0 (thiscall): CollisionTrack::End: the ped's swing state cleared
uint8_t __fastcall CollisionTrack_End(uint8_t* self, re::Edx) {
    uint8_t* ped = OwnerPed(self);
    vc::At<int32_t>(ped, 0xE4) = 0;
    vc::At<int32_t>(OwnerPed(self), 0x71C) = 0;
    PedList_SetFlag_48C190(OwnerPed(self) + 0x3BC, 0);
    return 1;
}

// 0x5F8A50 (thiscall): a hit recorded in the first free slot (0: already hit, or full)
uint8_t __fastcall Track_AddHitImpl(uint8_t* self, re::Edx, uint8_t* entity, const Vec3* pos, int32_t sphere) {
    int i = 0;
    for (;; i++) {
        uint8_t* e = Hits(self)[i].entity;
        if (e == entity) {
            return 0;
        }
        if (!e) {
            break;
        }
        if (i + 1 >= 0x10) {
            return 0;
        }
    }
    CollisionHit& hit = Hits(self)[i];
    if (entity != hit.entity) {
        if (hit.entity) {
            Entity_CleanUpOldReference(hit.entity, &hit.entity);
        }
        hit.entity = entity;
        if (entity) {
            Entity_RegisterReference(entity, &hit.entity);
        }
    }
    hit.pos = *pos;
    hit.sphere = sphere;
    return 1;
}

// 0x5F8AE0 (thiscall): the hits aged; ones older than maxAge (-1: never) dropped
void __fastcall Track_AgeHitsImpl(uint8_t* self, re::Edx, float dt, float maxAge) {
    for (int i = 0; i < 16; i++) {
        CollisionHit& hit = Hits(self)[i];
        if (!hit.entity) {
            hit.age = 0.0f;
            continue;
        }
        float age = float(double(hit.age) + double(dt));
        hit.age = age;
        if (maxAge != -1.0f && maxAge < age) {
            if (hit.entity) {
                Entity_CleanUpOldReference(hit.entity, &hit.entity);
                hit.entity = nullptr;
            }
            hit.age = 0.0f;
        }
    }
}

// a matrix at a position (the identity; its fourth position float the game leaves
// uninitialised)
void MatrixAt(GameMatrix* m, const Vec3& pos) {
    m->attach = 0;
    Matrix_Construct(m);
    m->pos[0] = pos.x;
    m->pos[1] = pos.y;
    m->pos[2] = pos.z;
    m->pos[3] = 0.0f;
}
void InitColPoint(GameColPoint* point) {
    point->pos = {0.0f, 0.0f, 0.0f};
    point->entity = nullptr;
    point->normal = {0.0f, 0.0f, 0.0f};
    point->depth = 3.4028235e38f;
}
void ReleaseColPoint(GameColPoint* point) {
    if (point->entity) {
        Entity_CleanUpOldReference(point->entity, &point->entity);
    }
}

// the line of sight from `start` to the entity's (x, y) at `z` is blocked (the scan code
// advanced after, except for TestPed's)
bool LineOfSightBlocked(uint8_t* entity, const Vec3* start, const Vec3& end, bool advanceScan = true) {
    GameColPoint point;
    InitColPoint(&point);
    re::Ptr32<uint8_t> hitEntity = nullptr;
    ignoreEntity = entity;
    uint8_t blocked = World_ProcessLineOfSight(start, &end, &point, &hitEntity, 0x49, 0, 0);
    ignoreEntity = nullptr;
    if (advanceScan) {
        World_AdvanceScanCode();
    }
    ReleaseColPoint(&point);
    return blocked != 0;
}

// 0x5F8B60 (thiscall): a sector's object list swept (peds and objects, types 6 / 4):
// the sweep's sphere model against theirs, else the line against their triangles; the
// hit kept unless a line of sight from `losStart` is blocked
uint8_t __fastcall Track_TestObjectListImpl(uint8_t* self, re::Edx, uint32_t* list, const Vec3* losStart, const Vec3* pos,
                                            float radius, float) {
    GameColPoint point;
    InitColPoint(&point);
    uint8_t result = 0;
    Vec3 centre = *pos;
    GameMatrix from, to;
    MatrixAt(&from, vc::At<Vec3>(self, 0x200));
    MatrixAt(&to, centre);
    uint16_t code = scanCode;
    for (uint32_t* node = list; node;) {
        uint8_t* entity = LinkEntity(node);
        node = NextLink(node);
        if (vc::At<uint16_t>(entity, 0x10A) == code) {
            continue;
        }
        vc::At<uint16_t>(entity, 0x10A) = code;
        if (!Collidable(entity)) {
            continue;
        }
        if (!vc::Call<uint8_t>(entity, 0x7C) || vc::Call<uint8_t>(entity, 0x78)) {
            continue;
        }
        if (!Entity_IsTouchingSphere(entity, &centre, radius)) {
            continue;
        }
        uint8_t type = entity[0x108] & 7;
        if (type != 6 && type != 4) {
            continue;
        }
        GameMatrix* matrix = Placeable_GetMatrix(entity);
        uint8_t* colModel = GetColModel(entity);
        if (!ProcessColModels(&from, &to, sweepColModel, matrix, matrix, colModel, &point, 0, 0, 0, 0, 0, 0xF, 0, 1)) {
            uint8_t* colData = vc::At<uint8_t*>(colModel, 0x38);
            if (!colData || int8_t(colData[0x1F]) <= 0) {
                continue;
            }
            GameColLine line;
            ColLine_Set(&line, self + 0x200, pos);
            uint8_t* triangles = vc::At<uint8_t*>(colData, 8);
            GameMatrix inverse;
            inverse.attach = 0;
            Matrix_Invert(matrix, &inverse);
            float local[8];
            Matrix_TransformPoints(local, 2, &inverse, &line, 0x10);
            if (!ColLine_TestTriangles(local, triangles, triangles + 0x10)) {
                continue;
            }
            point.pos = *pos;
        }
        if (losStart) {
            Vec3 end{matrix->pos[0], matrix->pos[1], losStart->z};
            if (LineOfSightBlocked(entity, losStart, end)) {
                continue;
            }
        }
        if (Track_AddHit(self, entity, &point.pos, -1)) {
            result = 1;
        }
    }
    ReleaseColPoint(&point);
    return result;
}

// 0x5F8F80 (thiscall): a ped hit by the sweep: the first of its body spheres (0..5)
// near the line (a biker's: first the torso, from sphere 3); the hit point on the
// sphere towards `pos`, the sphere's index out
uint8_t __fastcall Track_TestPedImpl(uint8_t* self, re::Edx, uint8_t* ped, const Vec3* losStart, const Vec3* pos, float radius,
                                     float margin, Vec3* hitPos, int32_t* sphereOut) {
    uint8_t onBike = 0;
    uint8_t* vehicle = vc::At<uint8_t*>(ped, 0x1554);
    if (vehicle && vc::At<int32_t>(vehicle, 0x46C) == 0) {
        int32_t kind;
        if (*Vehicle_GetVehicleClass(vehicle, &kind) != 7 && *Vehicle_GetVehicleClass(vehicle, &kind) != 6) {
            return 0;
        }
        onBike = 1;
        radius = float(double(radius) + 1.0);
    }
    Vec3 centre = *pos;
    if (!(Collidable(ped) || onBike)) {
        return 0;
    }
    if (!Entity_IsTouchingSphere(ped, &centre, radius)) {
        return 0;
    }
    // the swept segment: from the last position along to `pos`
    struct {
        Vec3 start;
        Vec3 dir;
    } segment;
    segment.start = vc::At<Vec3>(self, 0x200);
    Vec3 d;
    Vec3* dp = VectorSub(&d, pos, reinterpret_cast<Vec3*>(self + 0x200));
    segment.dir = *dp;
    Vec3 sphereCentre{0.0f, 0.0f, 0.0f};
    float sphereRadius;
    uint8_t second = 0;
    uint8_t losDone = 0;
    for (int i = 0;;) {
        if (second) {
            i = 3;
            Vector_Normalise(&segment.dir);
        }
        if (Ped_GetBodySphere(ped, i, &sphereCentre, &sphereRadius)) {
            float dist = DistToLineSegment(&sphereCentre, &segment, 0);
            if (sphereRadius + margin > dist) {
                if (losStart && !losDone) {
                    Vec3 end = *Placeable_GetPosition(ped);
                    end.z = float(double(end.z) + headHeight_900E48);
                    losDone = 1;
                    if (LineOfSightBlocked(ped, losStart, end, false)) {
                        return 0;
                    }
                }
                Vec3 t;
                *hitPos = *VectorSub(&t, pos, &sphereCentre);
                Vector_Normalise(hitPos);
                float s = float(double(sphereRadius) * hitPos->x);
                hitPos->x = float(double(s) + double(sphereCentre.x));
                s = float(double(sphereRadius) * hitPos->y);
                hitPos->y = float(double(s) + double(sphereCentre.y));
                s = float(double(sphereRadius) * hitPos->z);
                hitPos->z = float(double(s) + double(sphereCentre.z));
                *sphereOut = i;
                if (onBike || i != 0) {
                    return 1;
                }
                second = 1;
            } else if (second) {
                return 1;
            }
        }
        i++;
        if (i >= 6) {
            return 0;
        }
    }
}

// 0x5F92F0 (thiscall): a sector's vehicle list swept (their collision models); each
// vehicle hit also damaged at the hit piece
uint8_t __fastcall Track_TestVehicleListImpl(uint8_t* self, re::Edx, uint32_t* list, const Vec3* pos, float radius, float impulse,
                                             uint8_t* damager) {
    GameColPoint point;
    InitColPoint(&point);
    uint8_t result = 0;
    Vec3 centre = *pos;
    GameMatrix from, to;
    MatrixAt(&from, vc::At<Vec3>(self, 0x200));
    MatrixAt(&to, centre);
    uint16_t code = scanCode;
    for (uint32_t* node = list; node;) {
        uint8_t* entity = LinkEntity(node);
        node = NextLink(node);
        if (vc::At<uint16_t>(entity, 0x10A) == code) {
            continue;
        }
        uint8_t type = entity[0x108] & 7;
        vc::At<uint16_t>(entity, 0x10A) = code;
        if (type != 2 || !Collidable(entity)) {
            continue;
        }
        if (!Entity_IsTouchingSphere(entity, &centre, radius) || vc::At<int32_t>(entity, 0x46C) != 0) {
            continue;
        }
        GameMatrix* matrix = Placeable_GetMatrix(entity);
        uint8_t* colModel = GetColModel(entity);
        if (!ProcessColModels(&from, &to, sweepColModel, matrix, matrix, colModel, &point, 0, 0, 0, 0, 0, 0xF, 0, 1)) {
            continue;
        }
        if (Track_AddHit(self, entity, &point.pos, -1)) {
            result = 1;
        }
        Automobile_SetDamage(entity, point.piece, impulse, damager, &point.normal);
    }
    ReleaseColPoint(&point);
    return result;
}

// 0x5F95A0 (thiscall): BaseCollisionTrack::Start: the hits cleared
uint8_t __fastcall BaseCollisionTrack_StartImpl(uint8_t* self, re::Edx, uint8_t*) {
    for (int i = 0; i < 16; i++) {
        CollisionHit& hit = Hits(self)[i];
        if (hit.entity) {
            Entity_CleanUpOldReference(hit.entity, &hit.entity);
            hit.entity = nullptr;
        }
        hit.age = 0.0f;
    }
    return 1;
}

// 0x5F95E0 (thiscall): CollisionTrack::Start: the ped's swing state set until the
// sweep's start time
uint8_t __fastcall CollisionTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    vc::At<uint8_t*>(self, 0x20C) = owner;
    BaseCollisionTrack_Start(self, owner);
    uint8_t* ped = OwnerPed(self);
    vc::At<int32_t>(ped, 0x71C) = 1;
    ped[0x4E5] = 1;
    vc::At<int32_t>(ped, 0x4E8) = 1;
    float start = vc::At<float>(self, 0x18);
    vc::At<int32_t>(ped, 0xE4) = 1;
    vc::At<uint32_t>(ped, 0x720) = TimeAfter(start);
    return 1;
}

// the sector range around a point (+-4), clamped to the 36 x 36 grid
struct SectorRange {
    int x0, y0, x1, y1;
};
int SectorIndex(float v) {
    float scaled = float(double(v) / sectorSize_900AA0);
    return int(float(double(scaled) + sectorOffset_900A98));
}
SectorRange SectorsAround(const Vec3* pos) {
    float maxX = float(double(pos->x) + sweepMargin_900970);
    float minY = float(double(pos->y) - sweepMargin_900970);
    float maxY = float(double(pos->y) + sweepMargin_900970);
    float minX = float(double(pos->x) - sweepMargin_900970);
    SectorRange r;
    float v = float(SectorIndex(minX));
    r.x0 = int(v > 0.0f ? v : 0.0f);
    v = float(SectorIndex(minY));
    r.y0 = int(v > 0.0f ? v : 0.0f);
    v = float(SectorIndex(maxX));
    r.x1 = int(v < sectorMax_900A94 ? v : sectorMax_900A94);
    v = float(SectorIndex(maxY));
    r.y1 = int(v < sectorMax_900A94 ? v : sectorMax_900A94);
    return r;
}

// 0x5F9920 (thiscall): the object lists of the sectors around `pos`
uint8_t __fastcall Track_TestObjectSectorsImpl(uint8_t* self, re::Edx, const Vec3* losStart, const Vec3* pos, float radius, float margin) {
    uint8_t result = 0;
    SectorRange r = SectorsAround(pos);
    for (int y = r.y0; y <= r.y1; y++) {
        for (int x = r.x0; x <= r.x1; x++) {
            uint32_t* list = sectorEntities_C1B17C[(x + y * 36) * 5];
            result |= Track_TestObjectList(self, list, losStart, pos, radius, margin);
        }
    }
    return result;
}

// 0x5F9B00 (thiscall): peds swept: the ones in `list` ({?, count, peds...}), or
// without one the whole ped pool (last slot first)
uint8_t __fastcall Track_TestPedsImpl(uint8_t* self, re::Edx, uint8_t* list, const Vec3* losStart, const Vec3* pos, float radius,
                                      float margin) {
    Vec3 hitPos{0.0f, 0.0f, 0.0f};
    int32_t sphere;
    uint8_t result = 0;
    if (list) {
        for (int i = 0; i < vc::At<int32_t>(list, 4); i++) {
            uint8_t* ped = reinterpret_cast<re::Ptr32<uint8_t>*>(list + 8)[i];
            if (!ped || AlreadyHit(self, ped)) {
                continue;
            }
            sphere = 0;
            if (Track_TestPed(self, ped, losStart, pos, radius, margin, &hitPos, &sphere)) {
                Track_AddHit(self, ped, &hitPos, sphere);
                result = 1;
            }
        }
        return result;
    }
    uint8_t* pool = pedPool;
    for (int slot = vc::At<int32_t>(pool, 8); slot != 0;) {
        slot--;
        if (vc::At<uint8_t*>(pool, 4)[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = vc::At<uint8_t*>(pool, 0) + vc::At<int32_t>(pool, 0xC) * slot;
        if (!ped || AlreadyHit(self, ped)) {
            continue;
        }
        sphere = 0;
        if (Track_TestPed(self, ped, losStart, pos, radius, margin, &hitPos, &sphere)) {
            Track_AddHit(self, ped, &hitPos, sphere);
            result = 1;
        }
    }
    return result;
}

// 0x5F9C70 (thiscall): the vehicle lists of the sectors around `pos`
uint8_t __fastcall Track_TestVehicleSectorsImpl(uint8_t* self, re::Edx, const Vec3* pos, float radius, float impulse, uint8_t* damager) {
    uint8_t result = 0;
    SectorRange r = SectorsAround(pos);
    for (int y = r.y0; y <= r.y1; y++) {
        for (int x = r.x0; x <= r.x1; x++) {
            uint32_t* list = sectorEntities_C1B180[(x + y * 36) * 5];
            result |= Track_TestVehicleList(self, list, pos, radius, impulse, damager);
        }
    }
    return result;
}

// 0x5F9E50 (thiscall): one sweep: the ped's fight target first, then the peds near it
// (unless +0x59), the vehicles and the objects
uint8_t __fastcall Track_SweepImpl(uint8_t* self, re::Edx, const Vec3* losStart, const Vec3* pos, float radius, float margin) {
    if (scanCode < 0xFFFF) {
        scanCode++;
    } else {
        World_ClearScanCodes();
        scanCode = 1;
    }
    uint8_t* ped = OwnerPed(self);
    uint8_t skipPeds = self[0x59];
    uint8_t result = 0;
    uint8_t* target = Ped_GetFightTarget_497100(ped);
    if (target && (target[0x108] & 7) != 3) {
        target = nullptr;
    }
    if (!target) {
        target = PedTargeting_GetTarget_4D7780(vc::At<uint8_t*>(ped, 0x1C8));
        if (target && (target[0x108] & 7) != 3) {
            target = nullptr;
        }
    }
    if (target && !AlreadyHit(self, target)) {
        Vec3 hitPos{0.0f, 0.0f, 0.0f};
        int32_t sphere;   // (uninitialised in the game too: set on a hit)
        if (Track_TestPed(self, target, losStart, pos, radius, margin, &hitPos, &sphere)) {
            Track_AddHit(self, target, &hitPos, sphere);
            result = 1;
        }
    }
    if (!skipPeds) {
        result |= Track_TestPeds(self, ped + 0x3BC, losStart, pos, radius, margin);
    }
    result |= Track_TestVehicleSectors(self, pos, radius, vc::At<float>(self, 0x34), ped);
    return Track_TestObjectSectors(self, losStart, pos, radius, margin) | result;
}

// 0x5F9FD0 (thiscall): CollisionTrack::Update: between its start and end times the
// bone swept from its last position; every new hit damaged and sent to the track's node
uint8_t __fastcall CollisionTrack_Update(uint8_t* self, re::Edx, float dt) {
    uint8_t* owner = vc::At<uint8_t*>(self, 0x20C);
    uint8_t* ped = vc::At<uint8_t*>(owner, 8);
    int32_t bone = vc::At<int32_t>(self, 0x210);
    uint8_t* matrices = Hierarchy_GetMatrices(Clump_GetHierarchy(vc::At<void*>(ped, 0x18)));
    float start = vc::At<float>(self, 0x18);
    float end = vc::At<float>(self, 0x1C);
    Vec3 bonePos = *reinterpret_cast<Vec3*>(matrices + bone * 0x34 + 0x24);
    float time = vc::At<float>(vc::At<uint8_t*>(vc::At<uint8_t*>(self, 0x20C), 0), 8);
    Vec3& last = vc::At<Vec3>(self, 0x200);
    if (start > time) {
        last = bonePos;
        return 1;
    }
    bool active;
    if (start != start || time != time) {
        active = false;
    } else if (time <= end) {
        active = true;
    } else {
        active = end < 0.0f;
    }
    if (active) {
        if (vc::At<int32_t>(ped, 0x71C) != 2) {
            last = bonePos;
        }
        vc::At<int32_t>(ped, 0x71C) = 2;
        vc::At<uint32_t>(ped, 0x720) = TimeAfter(end);
    } else if (time > end) {
        vc::At<int32_t>(ped, 0x71C) = 3;
        float trackEnd = vc::At<float>(self, 0x10);
        if (0.0f < trackEnd) {
            vc::At<uint32_t>(ped, 0x720) = TimeAfter(trackEnd);
            return 1;
        }
        float left = 0.0f;
        uint8_t* anim = AnimBlend_GetCurrent_6B1F50(vc::At<uint8_t*>(ped, 0x114), 0);
        if (anim) {
            uint8_t* hier = vc::At<uint8_t*>(anim, 0x18);
            float at = hier ? vc::At<float>(anim, 0x1C) : -1.0f;
            float length = hier ? vc::At<float>(hier, 0xC) : -1.0f;
            left = float(double(length) - double(at));
            if (0.0f > left) {
                left = 0.0f;
            }
        }
        vc::At<uint32_t>(ped, 0x720) = TimeAfter(left);
        return 1;
    }
    // the sweep from the ped's position (at the bone's height; +0.5 above it with +0x215)
    uint8_t* matrix = vc::At<uint8_t*>(ped, 0x14);
    Vec3 pedPos = matrix ? *reinterpret_cast<Vec3*>(matrix + 0x30) : *reinterpret_cast<Vec3*>(ped + 4);
    Vec3 from{pedPos.x, pedPos.y, bonePos.z};
    if (self[0x215]) {
        from.z = float(double(pedPos.z) + 0.5);
    }
    float radius = vc::At<float>(self, 0x5C);
    vc::At<float>(sweepColModel, 0xC) = radius;
    vc::At<float>(vc::At<uint8_t*>(vc::At<uint8_t*>(sweepColModel, 0x38), 0), 0xC) = radius;
    Vec3 boxMax{radius, radius, radius};
    float negative = -radius;
    Vec3 boxMin{negative, negative, negative};
    Box_Set(sweepColModel + 0x10, &boxMin, &boxMax);
    Vec3 d;
    VectorSub(&d, &bonePos, &last);
    float lengthSq = std::fabs(d.x * d.x + d.y * d.y + d.z * d.z);
    float length = float(std::sqrt(double(lengthSq)));
    float sweepRadius = length + (radius + radius);
    if (Track_Sweep(self, &from, &bonePos, sweepRadius, radius)) {
        for (int i = 0; i < 16; i++) {
            CollisionHit& hit = Hits(self)[i];
            uint8_t* entity = hit.entity;
            float age = hit.age;
            if (!entity) {
                continue;
            }
            if (!(age == 0.0f || vc::At<float>(self, 0x30) > age)) {
                continue;
            }
            uint8_t type = entity[0x108] & 7;
            if (type == 2) {
                uint8_t* node = Reference_Get0C(self + 0x60, -1);
                if (node && !vc::Call<uint8_t*>(node, kNodeFindPlayable, vc::At<uint8_t*>(self, 0x20C))) {
                    break;
                }
                hit.age = 0.0f;
                Vehicle_Knock_4CC4A0(entity, ped, -1, 0.0f, hit.pos, 0, 1);
                continue;
            }
            uint8_t* target = type == 3 ? entity : nullptr;
            uint8_t grapple = self[0x215] && type == 3 ? 1 : 0;
            uint8_t* node = Reference_Get0C(self + 0x60, -1);
            uint8_t* playable = nullptr;
            bool perform = false;
            if (node) {
                if (!target) {
                    perform = true;
                } else {
                    if (grapple) {
                        uint8_t* partner = vc::At<uint8_t*>(target, 0x155C);
                        if ((partner || vc::At<uint8_t*>(ped, 0x155C)) && (partner != ped || vc::At<uint8_t*>(ped, 0x155C) != target)) {
                            break;
                        }
                        Ped_SetGrapplePartner(target, ped);
                        Ped_SetGrapplePartner(ped, target);
                    }
                    playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, vc::At<uint8_t*>(self, 0x20C));
                    if (grapple) {
                        Ped_SetGrapplePartner(target, nullptr);
                        Ped_SetGrapplePartner(ped, nullptr);
                    }
                    if (!playable) {
                        break;
                    }
                }
            }
            if (!perform) {
                if (!target || vc::At<uint8_t*>(ped, 0x155C) != target) {
                    perform = true;
                } else {
                    // (grappled already: the hit dropped)
                    if (hit.entity) {
                        Entity_CleanUpOldReference(hit.entity, &hit.entity);
                        hit.entity = nullptr;
                    }
                    hit.age = 0.0f;
                    continue;
                }
            }
            hit.age = 0.0f;
            float damage = vc::At<float>(self, 0x34);
            if (type == 6 || type == 4) {
                damage = vc::At<float>(self, 0x50);
            }
            Vec3 hitPos = hit.pos;
            Vec3 dir;
            VectorSub(&dir, &last, &bonePos);
            float f3c = vc::At<float>(self, 0x3C);
            float f38 = vc::At<float>(self, 0x38);
            int32_t sphere = hit.sphere;
            // (the byte flags go on the stack as dwords whose upper bytes are stale)
            int b214 = self[0x214];
            int b219 = self[0x219];
            int b58 = self[0x58];
            int b218 = self[0x218];
            int32_t v2c = vc::At<int32_t>(self, 0x2C);
            uint8_t* node70 = Reference_Get0C(self + 0x70, -1);
            using DamageFn = uint8_t(__thiscall*)(uint8_t*, float, float, uint8_t*, int, Vec3*, Vec3*, uint8_t*, int, int32_t, int,
                                                  float, float, int, int, int, int32_t, float, int);
#if defined(_M_X64)
            if (*reinterpret_cast<const uint32_t*>(ped) == 0 || *reinterpret_cast<const uint32_t*>(entity) == 0) {   // (bring-up check)
                std::fprintf(stderr, "x64 check: collision track %p: owner %p ped %p (vtable %08X) hit %p (vtable %08X)\n",
                             static_cast<void*>(self), static_cast<void*>(owner), static_cast<void*>(ped),
                             *reinterpret_cast<const uint32_t*>(ped), static_cast<void*>(entity), *reinterpret_cast<const uint32_t*>(entity));
                std::fflush(stderr);
                __debugbreak();
            }
#endif
            DamageFn damageFn = re::PtrCast<DamageFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[0x3C / 4]);
            if (!damageFn(entity, damage, 0.0f, ped, -1, &hitPos, &dir, node70, b218, v2c, 0, f38, f3c, b58, b214, b219, sphere, 0.0f, 0)) {
                continue;
            }
            if (Reference_Get0C(self + 0x60, -1) && (entity[0x108] & 7) != 6 && grapple && target &&
                (!Ped_IsSeatedInVehicle(target) ||
                 Ctl_IsPlaying(vc::At<uint8_t*>(ped, 0x2E0) + 0x258, &grappleName_1B652A8, 1, 1))) {
                Ped_SetGrapplePartner(target, ped);
                Ped_SetGrapplePartner(ped, target);
                vc::At<int32_t>(target, 0x1500) = self[0x216];
                vc::At<int32_t>(ped, 0x1500) = self[0x216];
            }
            if (vc::Call<uint8_t*>(entity, 0x40)) {
                if (vc::At<const char*>(self, 0x28)) {
                    uint16_t id = SoundBank_GetId(soundBanks_B6F1E8, vc::At<const char*>(self, 0x28));
                    vc::At<int32_t>(vc::At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x48) = id;
                }
                if (vc::At<const char*>(self, 0x20)) {
                    int stream = SoundStreams_GetId(soundStreams_CF3DA0, vc::At<const char*>(self, 0x20));
                    vc::At<int32_t>(vc::At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x40) = stream;
                    uint8_t v = uint8_t(vc::At<int32_t>(self, 0x24));
                    vc::At<uint8_t>(vc::At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x44) = v;
                }
                float f44 = vc::At<float>(self, 0x44);
                vc::At<float>(vc::At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x54) = f44;
                float f40 = vc::At<float>(self, 0x40);
                vc::At<float>(vc::At<uint8_t*>(vc::Call<uint8_t*>(entity, 0x40), 0x10), 0x58) = f40;
            }
            if (target) {
                float f4c = vc::At<float>(self, 0x4C);
                Stat_Add_4A74C0(vc::At<uint8_t*>(target, 0x1D8), f4c);
                // (the game reuses the damage's stack slot: the same value again)
                Stat_Add_4DAC30(vc::At<uint8_t*>(target, 0x1D4), f4c);
                Ped_AddStun_47CD90(ped, vc::At<float>(self, 0x54), 0);
            }
            if (playable && (entity[0x108] & 7) == 3) {
                uint8_t* controller = vc::At<uint8_t*>(ped, 0x2E0) + 0x258;
                if (self[0x217]) {
                    Ctl_QueueTracks(controller, playable);
                } else {
                    Ctl_PlayNode(controller, playable);
                }
            }
        }
    }
    Track_AgeHits(self, dt, -1.0f);
    last = bonePos;
    return 1;
}

} // namespace

#define COLTRACKS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

COLTRACKS_REVERSED("BaseCollisionTrack::ClearNodes", 0x005F7720, &Track_ClearNodes);
COLTRACKS_REVERSED("BaseCollisionTrack::SetNode", 0x005F7740, &Track_SetNode);
COLTRACKS_REVERSED("CollisionTrack::InitFields", 0x005F7770, &CollisionTrack_InitFields);
COLTRACKS_REVERSED("BaseCollisionTrack::Destruct", 0x005F8200, &BaseCollisionTrack_DestructImpl);
COLTRACKS_REVERSED("BaseCollisionTrack::DeletingDestruct", 0x005F82E0, &BaseCollisionTrack_DeletingDestruct);
COLTRACKS_REVERSED("BaseCollisionTrack::Resolve", 0x005F8300, &Track_Resolve);
COLTRACKS_REVERSED("CollisionTrack::DeletingDestruct", 0x005F8370, &CollisionTrack_DeletingDestruct);
COLTRACKS_REVERSED("CollisionTrack::End", 0x005F83A0, &CollisionTrack_End);
COLTRACKS_REVERSED("CollisionTrack::AddHit", 0x005F8A50, &Track_AddHitImpl);
COLTRACKS_REVERSED("BaseCollisionTrack::AgeHits", 0x005F8AE0, &Track_AgeHitsImpl);
COLTRACKS_REVERSED("CollisionTrack::TestObjectList", 0x005F8B60, &Track_TestObjectListImpl);
COLTRACKS_REVERSED("CollisionTrack::TestPed", 0x005F8F80, &Track_TestPedImpl);
COLTRACKS_REVERSED("CollisionTrack::TestVehicleList", 0x005F92F0, &Track_TestVehicleListImpl);
COLTRACKS_REVERSED("BaseCollisionTrack::Start", 0x005F95A0, &BaseCollisionTrack_StartImpl);
COLTRACKS_REVERSED("CollisionTrack::Start", 0x005F95E0, &CollisionTrack_Start);
COLTRACKS_REVERSED("BaseCollisionTrack::TestObjectSectors", 0x005F9920, &Track_TestObjectSectorsImpl);
COLTRACKS_REVERSED("BaseCollisionTrack::TestPeds", 0x005F9B00, &Track_TestPedsImpl);
COLTRACKS_REVERSED("CollisionTrack::TestVehicleSectors", 0x005F9C70, &Track_TestVehicleSectorsImpl);
COLTRACKS_REVERSED("CollisionTrack::Sweep", 0x005F9E50, &Track_SweepImpl);
COLTRACKS_REVERSED("CollisionTrack::Update", 0x005F9FD0, &CollisionTrack_Update);
