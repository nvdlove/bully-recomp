// CTargetingSystem, second half (0x4D70A0..0x4D8A40): a ped's (mostly the player's)
// lock-on. The candidates list (+0x90: entities in range, each with a score), scoring an
// entity against the view cone (0x4D70A0: -1e11 is "not targetable"), the scans of the
// ped pool and the sectors in front, the per-frame update and its target states
// (0 none, 1 locked, 2 set from outside, 3 a lock target at +0xA4 waiting to be
// taken), and the constructor/destructor. Pool: 0xC0F638 (0xE0 bytes each).
//
// Arithmetic is float as the game's x87 code in single precision (D3D9's FPU mode);
// the CRT's atan/sin/cos are std's.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Ped.h"
#include "game/Pool.h"
#include "game/VCall.h"
#include "reversed.h"

namespace targeting_c12c {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// a candidate (0x1C bytes; its entity a registered reference)
struct TargetEntry {
    re::Ptr32<CEntity> entity;        // +0
    uint32_t seenTime;      // +4 (0x4D6A00: its visibility last tested)
    uint32_t visibleTime;   // +8 (last seen)
    uint32_t unknownC;      // +0xC
    int32_t kind;           // +0x10 (0x4D5AE0)
    float score;            // +0x14 (kNoTarget: not targetable)
    uint32_t flags;         // +0x18 bit 0 always visible, bit 2 found by the last scan
};
static_assert(sizeof(TargetEntry) == 0x1C);

// the candidates (an array made with new[]: its count before it)
struct TargetList {
    re::Ptr32<TargetEntry> data;      // +0
    uint32_t capacity;      // +4
    uint32_t count;         // +8
};
static_assert(sizeof(TargetList) == 0xC);

struct CTargetingSystem {
    re::Ptr32<void> vtable;                   // +0x0 (0x9139FC)
    re::Ptr32<CPed> ped;                      // +0x4 the owner
    int32_t state;                  // +0x8 0 none, 1 locked, 2 set from outside, 3 lock target pending
    uint32_t unknownC;              // +0xC
    CVector source;                 // +0x10 where the view starts (the camera's when aiming)
    CVector origin;                 // +0x1C the ped's position, 1.2 up
    CVector direction;              // +0x28
    float heading;                  // +0x34 of `direction`
    float pitch;                    // +0x38
    float halfAngle;                // +0x3C the cone's (+0x44 or, aiming, +0x4C)
    float rangeOverride;            // +0x40 (-1: none)
    float halfAngleNormal;          // +0x44
    float heightScale;              // +0x48
    float halfAngleAiming;          // +0x4C
    uint32_t unknown50;             // +0x50
    uint32_t unknown54;             // +0x54
    float range;                    // +0x58
    float halfAngleWide;            // +0x5C the second, wider cone
    float heightScaleWide;          // +0x60
    float rangeWide;                // +0x64
    uint32_t unknown68;             // +0x68
    uint32_t unknown6C;             // +0x6C
    CVector lockDirection;          // +0x70
    uint8_t locked;                 // +0x7C
    uint8_t wasSteady;              // +0x7D
    uint8_t isPlayer;               // +0x7E
    uint8_t unknown7F;              // +0x7F
    float lockTime;                 // +0x80 (added to each frame while locked)
    uint8_t unknown84;              // +0x84 (counts as steady)
    uint8_t targetWasUp;            // +0x85 (the target ped's +0x1C98 clear)
    uint8_t unknown86[2];           // +0x86
    float weaponRangeScale;         // +0x88 while holding weaponModel
    int32_t weaponModel;            // +0x8C (-1: none)
    TargetList list;                // +0x90
    re::Ptr32<CEntity> partnerTarget;         // +0x9C (registered)
    re::Ptr32<CEntity> target;                // +0xA0 (registered)
    re::Ptr32<CEntity> lockTarget;            // +0xA4 (registered)
    re::Ptr32<CEntity> lookTarget;            // +0xA8 (registered, dropped 10 ms after +0xC0)
    re::Ptr32<CEntity> refAC;                 // +0xAC (registered)
    re::Ptr32<CEntity> refB0;                 // +0xB0 (registered)
    uint8_t unknownB4;              // +0xB4
    uint8_t unknownB5[3];           // +0xB5
    uint32_t timeB8;                // +0xB8 until when +0xB0 is kept
    uint32_t unknownBC;             // +0xBC
    uint32_t lookTime;              // +0xC0
    uint32_t targetTime;            // +0xC4 the target last changed
    uint32_t steadyTime;            // +0xC8 (the update counts as steady in this ms)
    uint32_t scanTime;              // +0xCC
    uint32_t unknownD0;             // +0xD0
    uint32_t scoreTime;             // +0xD4
    uint32_t targetScoreTime;       // +0xD8
    uint32_t unknownDC;             // +0xDC
};
static_assert(sizeof(CTargetingSystem) == 0xE0);
static_assert(offsetof(CTargetingSystem, halfAngle) == 0x3C);
static_assert(offsetof(CTargetingSystem, range) == 0x58);
static_assert(offsetof(CTargetingSystem, lockDirection) == 0x70);
static_assert(offsetof(CTargetingSystem, lockTime) == 0x80);
static_assert(offsetof(CTargetingSystem, weaponModel) == 0x8C);
static_assert(offsetof(CTargetingSystem, list) == 0x90);
static_assert(offsetof(CTargetingSystem, target) == 0xA0);
static_assert(offsetof(CTargetingSystem, timeB8) == 0xB8);
static_assert(offsetof(CTargetingSystem, scanTime) == 0xCC);
static_assert(offsetof(CTargetingSystem, targetScoreTime) == 0xD8);

namespace {

constexpr float kNoTarget = -1e11f;   // (0xD1BA43B7)
constexpr float kPi = 3.14159274f;

// a field at an offset (for the ones no header names)
template <typename T>
re::Field<T>& F(const void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uintptr_t>(base) + offset);
}

// an entity's position (its matrix's, else the placeable's)
const CVector* EntityPos(const void* entity) {
    uint8_t* matrix = F<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<const CVector*>(matrix + 0x30) : &F<const CVector>(entity, 4);
}
uint8_t EntityType(const void* entity) {
    return F<uint8_t>(entity, 0x108) & 7;
}
int16_t EntityModel(const void* entity) {
    return F<int16_t>(entity, 0x10E);
}

GAME_VAR(uint32_t, TimeInMilliseconds, 0x00C1A9B4);
// the targeting settings (+0x20 range scale, +0x28 ms, +0x2C cone, +0x30 locked cone
// in degrees, +0x34 locked height scale)
GAME_VAR(uint8_t*, TargetingSettings, 0x020C88B8);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_VAR(int32_t, CameraViewSlot, 0x00C3CCF4);
GAME_VAR(CPed*, ControlledPedA, 0x00C2B120);
GAME_VAR(CPed*, ControlledPedB, 0x00C2B124);
GAME_VAR(uint32_t, RetargetDelay, 0x00A45370);
GAME_VAR(CPool<CPed>*, PedPool, 0x00C0F5F0);
GAME_VAR(uint16_t, CurrentScanCode, 0x00C1AE88);
GAME_ARRAY(uint32_t*, SectorEntityLists, 0x00C1B17C);   // 5 words per sector (36 wide)
GAME_VAR(uint8_t*, EntityListNodes, 0x00C0F788);
GAME_ARRAY(uint8_t*, ModelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, TargetableModelA, 0x00BF40EC);
GAME_VAR(int32_t, TargetableModelB, 0x00BF40F4);
GAME_VAR(int32_t, PlayerUntargetableModel, 0x00BF40F8);
inline void* const PropTargetList = reinterpret_cast<void*>(0x00BD0CF8);

GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(float, __cdecl, AngleDifference, 0x00432980, float a, float b);
GAME_FN(float, __cdecl, GetAngleOfVector2D, 0x00433190, const CVector* v);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(float, __thiscall, Camera_GetZoom, 0x004F1250, void* camera);
GAME_FN(CVector*, __thiscall, Camera_GetForward, 0x0041AA70, void* camera, CVector* out);
GAME_FN(void*, __cdecl, Camera_GetView, 0x006C82F0, uintptr_t slot);
GAME_FN(uint8_t*, __cdecl, View_GetMatrix, 0x006C9A90, void* view);   // +0x24 its position
GAME_FN(void, __thiscall, Ped_GetForward, 0x004154A0, CPed* ped, CVector* out);
GAME_FN(void, __thiscall, RefB0_Assign, 0x00478760, re::Ptr32<CEntity>* self, re::Ptr32<CEntity>* from);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, VectorConstructorIterator, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
inline void* const TargetEntry_DefaultCtor = reinterpret_cast<void*>(0x004D6380);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(CEntity*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, int slot);
GAME_FN(uint8_t, __thiscall, PropList_Contains, 0x0042DD80, void* list, CEntity* entity);
GAME_FN(uint16_t*, __cdecl, GetObjectInfo, 0x004D0CA0, int model);
GAME_FN(uint8_t*, __cdecl, GetPedData, 0x004D5A40, CEntity* entity);   // +0x220 the health
GAME_FN(uint32_t, __cdecl, GetFrameTimeMs, 0x00446870);

// the first half (TargetingSystem.cpp) and this file's own, through the game's addresses
GAME_FN(float, __thiscall, TS_SelectInCone, 0x004D58E0, CTargetingSystem* self, float range, float halfWidth,
        float halfHeight, float distance, float heading, float pitch);
GAME_FN(uint8_t, __thiscall, TS_IsAiming, 0x004D59E0, CTargetingSystem* self);
GAME_FN(int32_t, __thiscall, TS_GetEntityKind, 0x004D5AE0, CTargetingSystem* self, CEntity* entity);
GAME_FN(float, __thiscall, TS_GetRangeScale, 0x004D5D90, CTargetingSystem* self);
GAME_FN(float, __thiscall, TS_GetZoom, 0x004D5DF0, CTargetingSystem* self);
GAME_FN(void, __thiscall, TS_Fn4D5EA0, 0x004D5EA0, CTargetingSystem* self);
GAME_FN(int32_t, __thiscall, TargetList_Find, 0x004D62D0, TargetList* list, TargetEntry* entry);
GAME_FN(TargetEntry*, __thiscall, TargetEntry_Construct, 0x004D63A0, TargetEntry* self, CEntity* entity);
GAME_FN(void, __thiscall, TargetEntry_Destruct, 0x004D63E0, TargetEntry* self);
GAME_FN(TargetEntry*, __thiscall, TargetEntry_Assign, 0x004D6400, TargetEntry* self, const TargetEntry* from);
GAME_FN(void, __cdecl, DirectionToAngles_x64, 0x004D6480, float, float, float, float* heading, float* pitch);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void DirectionToAngles(CVector a0, float* a1, float* a2) {
    return DirectionToAngles_x64(a0.x, a0.y, a0.z, a1, a2);
}
GAME_FN(void, __thiscall, TS_Reset, 0x004D64F0, CTargetingSystem* self);
GAME_FN(uint8_t, __thiscall, TS_SetTargetEntity, 0x004D6660, CTargetingSystem* self, CEntity* entity);
GAME_FN(uint8_t, __thiscall, TS_Fn4D6770, 0x004D6770, CTargetingSystem* self);
GAME_FN(void, __thiscall, TS_ClearTarget, 0x004D6860, CTargetingSystem* self);
GAME_FN(void, __thiscall, TS_UpdateVisibility, 0x004D6A00, CTargetingSystem* self, TargetEntry* entry, float rate);
GAME_FN(uint8_t, __thiscall, TS_CanSee, 0x004D6DA0, CTargetingSystem* self, TargetEntry* entry, int flag);
GAME_FN(CEntity*, __thiscall, TS_FindBestTarget, 0x004D6F10, CTargetingSystem* self);
GAME_FN(float, __thiscall, TS_Score, 0x004D70A0, CTargetingSystem* self, int kind, float x, float y, float z,
        float width, float height);
GAME_FN(void*, __thiscall, TargetEntry_VectorDeletingDtor, 0x004D7830, TargetEntry* self, int flags);
GAME_FN(void, __thiscall, TargetList_SetCapacity, 0x004D78C0, TargetList* list, uint32_t capacity, int setCount);
GAME_FN(void, __thiscall, TS_SetLockTarget, 0x004D7A40, CTargetingSystem* self, CEntity* entity);
GAME_FN(float, __thiscall, TS_ScoreEntity, 0x004D7AF0, CTargetingSystem* self, int kind, CEntity* entity, int force);
GAME_FN(void, __thiscall, TS_UpdateScores, 0x004D7C30, CTargetingSystem* self, int force);
GAME_FN(void, __thiscall, TS_UpdateTargetScore, 0x004D7D80, CTargetingSystem* self, int force);
GAME_FN(void, __thiscall, TargetList_PushBack, 0x004D7FF0, TargetList* list, const TargetEntry* entry);
GAME_FN(void, __thiscall, TS_PickBestTarget, 0x004D8040, CTargetingSystem* self);
GAME_FN(void, __thiscall, TS_AddCandidate, 0x004D8070, CTargetingSystem* self, CEntity* entity);
GAME_FN(void, __thiscall, TS_Clear, 0x004D81A0, CTargetingSystem* self);
GAME_FN(void, __thiscall, TargetList_RemoveAt, 0x004D8230, TargetList* list, uint32_t index);
GAME_FN(void, __thiscall, TS_ScanForTargets, 0x004D84C0, CTargetingSystem* self, int force);

// a registered reference set (the old one's registration dropped)
void SetRef(re::Ptr32<CEntity>* ref, CEntity* entity) {
    if (*ref == entity) {
        return;
    }
    if (*ref) {
        CleanUpOldReference(*ref, ref);
    }
    *ref = entity;
    if (entity) {
        RegisterReference(entity, ref);
    }
}
void ClearRef(re::Ptr32<CEntity>* ref) {
    if (*ref) {
        CleanUpOldReference(*ref, ref);
        *ref = nullptr;
    }
}

float Zoom(const CTargetingSystem* self) {
    return self->ped->pedType == 13 ? Camera_GetZoom(TheCamera) : 1.0f;
}

// the new[] array's entries released, last first, and the block freed
void DestroyEntries(TargetEntry* data) {
    uint32_t* block = reinterpret_cast<uint32_t*>(data) - 1;
    for (int i = int(*block) - 1; i >= 0; i--) {
        TargetEntry& entry = data[i];
        if (entry.entity) {
            CleanUpOldReference(entry.entity, &entry.entity);
            entry.entity = nullptr;
        }
    }
    Free(block);
}

} // namespace

// 0x4D70A0: how well an entity-sized box at `x, y, z` (`width` across, `height` tall)
// sits in the view cone (the higher the better; kNoTarget: not at all). Locked: the
// locked cone, else the normal cone and, missing it, the wider one (200 down). `kind`
// (0x4D5AE0) takes 1000 (and some 500 more) off.
float __fastcall Score(CTargetingSystem* self, re::Edx, int kind, float x, float y, float z, float width, float height) {
    CVector pos{x, y, z};
    CVector toOrigin;
    VectorSub(&toOrigin, &pos, &self->origin);
    float originDistance = std::sqrt(std::fabs(toOrigin.y * toOrigin.y + toOrigin.x * toOrigin.x + toOrigin.z * toOrigin.z));
    CVector toSource;
    VectorSub(&toSource, &pos, &self->source);
    float distance = std::sqrt(std::fabs(toSource.y * toSource.y + toSource.x * toSource.x + toSource.z * toSource.z));
    float heading = GetAngleOfVector2D(&toSource);
    float halfWidthAngle = float(std::atan(double(width / (distance + distance))));
    float flat = std::sqrt(std::fabs(toSource.y * toSource.y + toSource.x * toSource.x));
    float halfHeight = height * 0.5f;
    float low = float(std::atan(double((toSource.z - halfHeight) / flat)));
    float high = float(std::atan(double((toSource.z + halfHeight) / flat)));
    float spread = AngleDifference(low, high);
    float halfSpread = std::fabs(spread) * 0.5f;
    float pitch = spread * 0.5f + low;

    bool minus500 = false;
    if (self->locked) {
        if (TS_GetRangeScale(self) * self->range < originDistance) {
            return kNoTarget;
        }
        float range = self->rangeOverride == -1.0f ? TS_GetRangeScale(self) * self->range
                                                   : TS_GetRangeScale(self) * self->rangeOverride;
        uint8_t* settings = TargetingSettings;
        float fovLocked = F<float>(settings, 0x30);
        float zoom = Zoom(self);
        float halfFov = fovLocked * kPi / 180.0f * 0.5f / zoom;
        float score = TS_SelectInCone(self, range, halfFov + halfWidthAngle,
                                      F<float>(TargetingSettings, 0x34) * halfFov + halfSpread, distance, heading, pitch);
        if (score == kNoTarget) {
            return kNoTarget;
        }
        switch (kind) {
        case 1:
            return score;
        case 3:
            return score - 500.0f;
        case 5:
            minus500 = !self->isPlayer || !(6.0f > distance);
            break;
        case 8:
            if (2.0f < originDistance) {
                return kNoTarget;
            }
            minus500 = true;
            break;
        case 4:
        case 7:
            minus500 = true;
            break;
        default:
            break;
        }
        float result = score - 1000.0f;
        return minus500 ? result - 500.0f : result;
    }

    float halfFov = F<float>(TargetingSettings, 0x2C) * kPi / 180.0f * 0.5f;
    float off = std::fabs(AngleDifference(self->heading, heading));
    if (halfFov > kPi - off) {
        return kNoTarget;
    }
    float range = self->rangeOverride == -1.0f ? TS_GetRangeScale(self) * self->range
                                               : TS_GetRangeScale(self) * self->rangeOverride;
    float zoom = Zoom(self);
    float angle = self->halfAngle / zoom;
    float score = TS_SelectInCone(self, range, angle + halfWidthAngle, self->heightScale * angle + halfSpread, distance,
                                  heading, pitch);
    if (score != kNoTarget) {
        switch (kind) {
        case 1:
            return score;
        case 3:
            return score - 500.0f;
        case 5:
            minus500 = !self->isPlayer || !(6.0f > distance);
            break;
        case 8:
            if (2.0f < distance) {
                return kNoTarget;
            }
            minus500 = true;
            break;
        case 4:
        case 7:
            minus500 = true;
            break;
        default:
            break;
        }
        float result = score - 1000.0f;
        return minus500 ? result - 500.0f : result;
    }
    // the wider cone
    if (TS_IsAiming(self)) {
        return kNoTarget;
    }
    if (kind == 6 || kind == 8 || kind == 7) {
        return kNoTarget;
    }
    float wideRange = (self->rangeOverride == -1.0f || !(self->rangeOverride < self->rangeWide))
                          ? TS_GetRangeScale(self) * self->rangeWide
                          : TS_GetRangeScale(self) * self->rangeOverride;
    float wideAngle = self->halfAngleWide / TS_GetZoom(self);
    float wideScore = TS_SelectInCone(self, wideRange, wideAngle + halfWidthAngle,
                                      self->heightScaleWide * wideAngle + halfSpread, distance, heading, pitch);
    if (wideScore == kNoTarget) {
        return kNoTarget;
    }
    float result = wideScore - 200.0f;
    switch (kind) {
    case 1:
        return result;
    case 2:
    case 5:
        if (self->isPlayer && distance <= 1.25f) {
            return result;
        }
        return kNoTarget;
    case 3:
        if (self->isPlayer) {
            return result - 500.0f;
        }
        return kNoTarget;
    case 4:
        if (self->isPlayer && distance <= 1.25f) {
            return result - 500.0f;
        }
        return kNoTarget;
    default:
        return kNoTarget;
    }
}

// 0x4D7780: whom it's looking at: the target, else +0xA8 (kept from before), else the
// partner's target, the partner, or the best candidate (then kept in +0xA8)
CEntity* __fastcall GetLookTarget(CTargetingSystem* self) {
    CEntity* entity = self->target;
    if (entity) {
        return entity;
    }
    entity = self->lookTarget;
    if (entity) {
        return entity;
    }
    entity = self->partnerTarget;
    if (!entity) {
        entity = re::PtrCast<CEntity*>(self->ped->partner);
        if (!entity) {
            entity = TS_FindBestTarget(self);
        }
    }
    SetRef(&self->lookTarget, entity);
    self->lookTime = TimeInMilliseconds;
    return entity;
}

// 0x4D7800
void __fastcall ClearPartnerTarget(CTargetingSystem* self) {
    ClearRef(&self->partnerTarget);
}

// 0x4D7830: TargetEntry's vector deleting destructor (flags 2: the new[] array, 1: freed)
void* __fastcall TargetEntry_VectorDeletingDtorImpl(TargetEntry* self, re::Edx, int flags) {
    uint8_t f = uint8_t(flags);
    if (f & 2) {
        uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
        for (int i = int(*block) - 1; i >= 0; i--) {
            TargetEntry& entry = self[i];
            if (entry.entity) {
                CleanUpOldReference(entry.entity, &entry.entity);
                entry.entity = nullptr;
            }
        }
        if (f & 1) {
            Free(block);
        }
        return block;
    }
    if (self->entity) {
        CleanUpOldReference(self->entity, &self->entity);
        self->entity = nullptr;
    }
    if (f & 1) {
        Free(self);
    }
    return self;
}

// 0x4D78C0: a new array of `capacity` (the entries copied over), the count set to it if
// `setCount`
void __fastcall TargetList_SetCapacityImpl(TargetList* list, re::Edx, uint32_t capacity, int setCount) {
    TargetEntry* data = nullptr;
    list->capacity = capacity;
    if (capacity != 0) {
        // (the size saturates on overflow, as new[]'s)
        uint64_t bytes = uint64_t(capacity) * 0x1C;
        uint32_t size = (bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes);
        size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
        uint32_t* block = static_cast<uint32_t*>(Malloc(size));
        if (block) {
            data = reinterpret_cast<TargetEntry*>(block + 1);
            *block = capacity;
            VectorConstructorIterator(data, 0x1C, capacity, TargetEntry_DefaultCtor);
        }
    }
    if (list->data) {
        for (uint32_t i = 0; i < list->count; i++) {
            TargetEntry_Assign(&data[i], &list->data[i]);
        }
        if (list->data) {
            DestroyEntries(list->data);
        }
    }
    list->data = data;
    if (re::Flag(setCount)) {
        list->count = list->capacity;
    }
}

// 0x4D7A40: a lock target to take (state 3), the target dropped; none: state 0
void __fastcall SetLockTarget(CTargetingSystem* self, re::Edx, CEntity* entity) {
    SetRef(&self->lockTarget, entity);
    if (self->target) {
        self->lockTime = 0.0f;
        ClearRef(&self->target);
    }
    self->state = entity ? 3 : 0;
}

// 0x4D7AF0: an entity's score (kNoTarget for kind 0, and for entities other than
// peds and prop anims that vf31 says aren't targetable, unless `force`)
float __fastcall ScoreEntity(CTargetingSystem* self, re::Edx, int kind, CEntity* entity, int force) {
    if (kind == 0) {
        return kNoTarget;
    }
    uint8_t type = EntityType(entity);
    if (type == 3) {
        // a ped: aimed at its middle
        CVector pos = *EntityPos(entity);
        pos.z = vc::Call<float>(entity, 0x6C) * 0.5f + pos.z;
        float height = vc::Call<float>(entity, 0x6C);
        float width = vc::Call<float>(entity, 0x70);
        return TS_Score(self, kind, pos.x, pos.y, pos.z, width, height);
    }
    if (type != 6 && !vc::Call<uint8_t>(entity, 0x7C) && !re::Flag(force)) {
        return kNoTarget;
    }
    const CVector* pos = EntityPos(entity);
    float height = vc::Call<float>(entity, 0x6C);
    float width = vc::Call<float>(entity, 0x70);
    return TS_Score(self, kind, pos->x, pos->y, pos->z, width, height);
}

// 0x4D7C30: every candidate's score (at most every 200 ms unless `force`, once per ms);
// ones out of sight 300 ms (and not always visible) don't count, nor, while the
// player's locked, ones it can't see
void __fastcall UpdateScores(CTargetingSystem* self, re::Edx, int force) {
    if (!re::Flag(force) && TimeInMilliseconds - self->scoreTime <= 200) {
        return;
    }
    if (TimeInMilliseconds == self->scoreTime) {
        return;
    }
    self->scoreTime = TimeInMilliseconds;
    int left = int(self->list.count);
    if (left <= 0) {
        return;
    }
    for (int offset = 0; left != 0; offset++, left--) {
        TargetEntry* entry = &self->list.data[offset];
        int32_t kind = TS_GetEntityKind(self, entry->entity);
        float score = TS_ScoreEntity(self, kind, entry->entity, 0);
        if (score != kNoTarget) {
            if (TimeInMilliseconds - entry->seenTime > 500) {
                TS_UpdateVisibility(self, entry, 0.3f);
            }
            if (TimeInMilliseconds - entry->visibleTime >= 300 && !(entry->flags & 1)) {
                score = kNoTarget;
            }
        }
        if (self->isPlayer && self->locked && entry->entity && score != kNoTarget && !TS_CanSee(self, entry, 1)) {
            score = kNoTarget;
        }
        if (score != entry->score || entry->kind != kind) {
            entry->score = score;
            entry->kind = kind;
        }
    }
}

// 0x4D7D80: the target's own score (every 10 ms unless `force`); a target ped that
// goes down (+0x1C98) while the player's locked drops the target
void __fastcall UpdateTargetScore(CTargetingSystem* self, re::Edx, int force) {
    CEntity* target = self->target;
    if (!target) {
        return;
    }
    if (!re::Flag(force) && TimeInMilliseconds - self->targetScoreTime <= 10) {
        return;
    }
    self->targetScoreTime = TimeInMilliseconds;
    // (an entry built to search with, as the game's inlined constructor)
    TargetEntry probe{};
    probe.entity = target;
    RegisterReference(target, &probe.entity);
    probe.score = kNoTarget;
    probe.flags &= ~3u;   // (the game keeps uninitialised stack in the other bits)
    int index = -1;
    for (uint32_t i = 0; i < self->list.count; i++) {
        if (self->list.data[i].entity == probe.entity) {
            index = int(i);
            break;
        }
    }
    if (probe.entity) {
        CleanUpOldReference(probe.entity, &probe.entity);
        probe.entity = nullptr;
    }
    if (index == -1) {
        return;
    }
    TargetEntry* entry = &self->list.data[index];
    int32_t kind = TS_GetEntityKind(self, entry->entity);
    float score = TS_ScoreEntity(self, kind, entry->entity, 0);
    CEntity* entity = entry->entity;
    if (EntityType(entity) == 3) {
        uint8_t down = F<uint8_t>(entity, 0x1C98);
        if (self->isPlayer && self->targetWasUp && down) {
            TS_ClearTarget(self);
            return;
        }
        self->targetWasUp = down == 0;
    }
    if (score != kNoTarget) {
        TS_UpdateVisibility(self, entry, 1.0f);
        if (TimeInMilliseconds - entry->visibleTime > 2000) {
            score = kNoTarget;
        }
    }
    if (score != entry->score || entry->kind != kind) {
        entry->kind = kind;
        entry->score = score;
    }
}

// 0x4D7F60: the best target as the partner sees it (with the partner's +0x1424 set
// meanwhile), kept in +0x9C unless it's the partner
void __fastcall UpdatePartnerTarget(CTargetingSystem* self) {
    CPed* partner = self->ped->partner;
    if (!partner) {
        return;
    }
    int32_t& flag = F<int32_t>(partner, 0x1424);
    uint32_t was = flag != 0;
    flag = 1;
    TS_UpdateScores(self, 1);
    CEntity* best = TS_FindBestTarget(self);
    F<int32_t>(self->ped->partner, 0x1424) = int32_t(was);
    TS_UpdateScores(self, 1);
    if (best == re::PtrCast<CEntity*>(self->ped->partner)) {
        best = nullptr;
    }
    SetRef(&self->partnerTarget, best);
}

// 0x4D7FF0: an entry appended (the capacity doubled when full)
void __fastcall TargetList_PushBackImpl(TargetList* list, re::Edx, const TargetEntry* entry) {
    if (list->count == list->capacity) {
        TargetList_SetCapacity(list, list->capacity ? list->capacity * 2 : 1, 0);
    }
    TargetEntry_Assign(&list->data[list->count], entry);
    list->count++;
}

// 0x4D8040: the best candidate as the target (locked if there is one)
void __fastcall PickBestTarget(CTargetingSystem* self) {
    TS_UpdateScores(self, 1);
    TS_SetTargetEntity(self, TS_FindBestTarget(self));
    if (self->target) {
        self->state = 1;
    }
}

// 0x4D8070: an entity into the candidates (if not there) and marked as found
void __fastcall AddCandidate(CTargetingSystem* self, re::Edx, CEntity* entity) {
    if (!entity) {
        return;
    }
    TargetEntry probe{};
    probe.entity = entity;
    RegisterReference(entity, &probe.entity);
    probe.score = kNoTarget;
    probe.flags &= ~3u;   // (uninitialised stack in the game)
    uint32_t index = 0xFFFFFFFF;
    for (uint32_t i = 0; i < self->list.count; i++) {
        if (self->list.data[i].entity == probe.entity) {
            index = i;
            break;
        }
    }
    if (probe.entity) {
        CleanUpOldReference(probe.entity, &probe.entity);
        probe.entity = nullptr;
    }
    if (index == 0xFFFFFFFF) {
        TargetEntry added;
        TargetEntry_Construct(&added, entity);
        TargetList_PushBack(&self->list, &added);
        index = self->list.count - 1;
        if (added.entity) {
            CleanUpOldReference(added.entity, &added.entity);
        }
    }
    self->list.data[index].flags |= 4;
}

// 0x4D81A0: the targets and candidates dropped
void __fastcall Clear(CTargetingSystem* self) {
    ClearRef(&self->lookTarget);
    ClearRef(&self->target);
    ClearRef(&self->lockTarget);
    if (self->list.data) {
        TargetEntry_VectorDeletingDtor(self->list.data, 3);
        self->list.data = nullptr;
    }
    self->list.count = 0;
    self->list.capacity = 0;
}

// 0x4D8230: the entry at `index` removed (the rest moved down; the array freed when
// empty, halved when under half full)
void __fastcall TargetList_RemoveAtImpl(TargetList* list, re::Edx, uint32_t index) {
    for (; index < list->count - 1; index++) {
        TargetEntry_Assign(&list->data[index], &list->data[index + 1]);
    }
    if (--list->count == 0) {
        if (list->data) {
            DestroyEntries(list->data);
            list->data = nullptr;
        }
        list->capacity = 0;
        list->count = 0;
    }
    if (list->count < list->capacity >> 1) {
        TargetList_SetCapacity(list, list->capacity >> 1, 0);
    }
}

// 0x4D8370: the constructor
CTargetingSystem* __fastcall Construct(CTargetingSystem* self, re::Edx, CPed* ped) {
    self->source = {0.0f, 0.0f, 0.0f};
    self->ped = ped;
    self->origin = {0.0f, 0.0f, 0.0f};
    self->vtable = reinterpret_cast<void*>(0x009139FC);
    self->direction = {0.0f, 0.0f, 0.0f};
    self->lockDirection = {0.0f, 0.0f, 0.0f};
    self->list = {nullptr, 0, 0};
    self->partnerTarget = nullptr;
    self->target = nullptr;
    self->lockTarget = nullptr;
    self->lookTarget = nullptr;
    self->refAC = nullptr;
    self->refB0 = nullptr;
    TS_Reset(self);
    return self;
}

// 0x4D83F0: the destructor (the references' registrations dropped)
void __fastcall Destruct(CTargetingSystem* self) {
    self->vtable = reinterpret_cast<void*>(0x009139FC);
    TS_Clear(self);
    re::Ptr32<CEntity>* refs[] = {&self->refB0, &self->refAC, &self->lookTarget, &self->lockTarget, &self->target,
                        &self->partnerTarget};
    for (re::Ptr32<CEntity>* ref : refs) {
        if (*ref) {
            CleanUpOldReference(*ref, ref);
        }
    }
    if (self->list.data) {
        TargetEntry_VectorDeletingDtor(self->list.data, 3);
        self->list.data = nullptr;
    }
}

// 0x4D84C0: the candidates found anew (every 500 ms unless `force`): the peds in range,
// then the entities in the sectors (50 units) under the origin and the point up to 50
// in front, and the ped's +0x438; the ones not found again dropped
void __fastcall ScanForTargets(CTargetingSystem* self, re::Edx, int force) {
    if (!re::Flag(force) && TimeInMilliseconds - self->scanTime <= 500) {
        return;
    }
    self->scanTime = TimeInMilliseconds;
    for (uint32_t i = 0; i < self->list.count; i++) {
        self->list.data[i].flags &= ~4u;
    }
    CPool<CPed>* pool = PedPool;
    int slot = pool->size;
    // (0x4D5D90 inlined)
    float scale = Zoom(self);
    if (self->weaponModel != -1 && EntityModel(self->ped->weapon) == self->weaponModel) {
        scale = self->weaponRangeScale * scale;
    }
    float range = scale * self->range;
    float rangeSq = range * range;
    while (slot != 0) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        CEntity* ped = reinterpret_cast<CEntity*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
        if (!ped || ped == re::PtrCast<CEntity*>(self->ped)) {
            continue;
        }
        CVector d;
        VectorSub(&d, EntityPos(ped), &self->origin);
        d.z = d.z + 1.0f;
        if (rangeSq > d.y * d.y + d.x * d.x + d.z * d.z) {
            TS_AddCandidate(self, ped);
        }
    }

    float originX = self->origin.x;
    float originY = self->origin.y;
    scale = Zoom(self);
    if (self->weaponModel != -1 && EntityModel(self->ped->weapon) == self->weaponModel) {
        scale = self->weaponRangeScale * scale;
    }
    range = scale * self->range;
    float reach = range < 0.0f ? 0.0f : (range > 50.0f ? 50.0f : range);
    CPed* owner = self->ped;
    float forwardX, forwardY;
    if (owner->matrix) {
        forwardX = F<float>(owner->matrix, 0x10);
        forwardY = F<float>(owner->matrix, 0x14);
    } else {
        forwardX = -float(std::sin(double(owner->heading)));
        forwardY = float(std::cos(double(owner->heading)));
    }
    float stepX = reach * forwardX;
    float stepY = reach * forwardY;
    float farX = self->origin.x + stepX;
    float farY = self->origin.y + stepY;
    // the sector cells (36 x 36 of 50 units, truncated)
    int32_t cell[8];
    cell[0] = int32_t(int64_t(originX / 50.0f + 18.0f));
    cell[1] = int32_t(int64_t(originY / 50.0f + 18.0f));
    cell[2] = int32_t(int64_t(farX / 50.0f + 18.0f));
    cell[3] = int32_t(int64_t(farY / 50.0f + 18.0f));
    int cells = 1;
    if (cell[0] != cell[2] || cell[1] != cell[3]) {
        cells = 2;
        cell[4] = cell[0];
        cell[5] = cell[3];
        if (cell[3] != cell[1] && cell[0] != cell[2]) {
            cells = 4;
            cell[6] = cell[2];
            cell[7] = cell[1];
        }
    }
    if (CurrentScanCode >= 0xFFFF) {
        ClearScanCodes();
        CurrentScanCode = 1;
    } else {
        CurrentScanCode++;
    }
    for (int c = 0; c < cells; c++) {
        uint32_t* node = SectorEntityLists[(cell[c * 2] + cell[c * 2 + 1] * 36) * 5];
        while (node) {
            uint32_t packed = *node;
            CEntity* entity = GetEntityInPool(packed & 0xF, (packed >> 4) & 0x3FFF);
            node = (packed >> 18) == 0x3FFF ? nullptr : reinterpret_cast<uint32_t*>(EntityListNodes + (packed >> 18) * 4);
            if (F<uint16_t>(entity, 0x10A) == CurrentScanCode) {
                continue;
            }
            F<uint16_t>(entity, 0x10A) = CurrentScanCode;
            if (F<uint32_t>(entity, 0xC4) != 0) {
                continue;
            }
            int16_t model = EntityModel(entity);
            if (EntityType(entity) == 6) {
                uint8_t* info = ModelInfoPtrs[model];
                if (!info || !info[0x90]) {
                    continue;
                }
                if (!vc::Call<uint8_t>(entity, 0x7C) && !PropList_Contains(PropTargetList, entity) &&
                    EntityModel(entity) != TargetableModelA && EntityModel(entity) != TargetableModelB) {
                    continue;
                }
                if (self->ped->pedType == 13 && EntityModel(entity) == PlayerUntargetableModel) {
                    continue;
                }
            } else {
                if (!vc::Call<uint8_t>(entity, 0x7C)) {
                    continue;
                }
                uint16_t* objectInfo = GetObjectInfo(EntityModel(entity));
                if (!objectInfo || !(*objectInfo & 0x8000)) {
                    continue;
                }
                int16_t health = F<int16_t>(entity, 0x220);
                if (health > 299 && health > 0) {
                    continue;
                }
            }
            TS_AddCandidate(self, entity);
        }
    }
    CEntity* extra = self->ped->entityRef438.entity;
    if (extra && vc::Call<uint8_t>(extra, 0x7C)) {
        TS_AddCandidate(self, self->ped->entityRef438.entity);
    }
    for (int i = int(self->list.count) - 1; i >= 0; i--) {
        TargetEntry* entry = &self->list.data[i];
        if (!entry->entity || !(entry->flags & 4)) {
            TargetList_RemoveAt(&self->list, uint32_t(i));
        }
    }
}

namespace {
// (0x4D8A40's shared tail) the target lost beyond the range (from its point 1 up)
void CheckTargetRange(CTargetingSystem* self, uint8_t aiming, CEntity* previous) {
    CVector up{0.0f, 0.0f, 1.0f};
    CVector point;
    VectorAdd(&point, EntityPos(self->target), &up);
    float scale = TS_GetRangeScale(self);
    float range = self->range * scale * F<float>(TargetingSettings, 0x20);
    CVector d;
    VectorSub(&d, &point, &self->origin);
    if (range < Vector_Magnitude(&d)) {
        if (aiming) {
            TS_ClearTarget(self);
        } else {
            TS_SetLockTarget(self, previous);
        }
    }
}
} // namespace

// 0x4D8A40: the per-frame update (the player's, or the controlled peds')
void __fastcall Update(CTargetingSystem* self, re::Edx, int force) {
    if (self->lookTarget && TimeInMilliseconds - self->lookTime > 10) {
        ClearRef(&self->lookTarget);
    }
    CPed* ped = self->ped;
    if (ped->pedType != 13 && ControlledPedB != ped && ControlledPedA != ped) {
        return;
    }
    if (self->state == 2) {
        if (self->target && F<uint32_t>(self->target, 0xC0) == 0) {
            return;
        }
        TS_ClearTarget(self);
        return;
    }
    CEntity* previous = self->target;
    uint8_t aiming = TS_IsAiming(self);
    TS_Fn4D5EA0(self);
    F<uint8_t>(self->ped, 0x1DE5) = 0;
    if (!self->isPlayer && self->refAC) {
        self->unknownB4 = 1;
        RefB0_Assign(&self->refB0, &self->refAC);
        ClearRef(&self->refAC);
        self->timeB8 = F<uint32_t>(TargetingSettings, 0x28) + TimeInMilliseconds;
    }
    uint8_t steady = uint8_t(uint8_t(TimeInMilliseconds == self->steadyTime) | self->unknown84);
    if (!steady && self->wasSteady) {
        self->locked = 0;
    }
    self->halfAngle = aiming ? self->halfAngleAiming : self->halfAngleNormal;
    CVector up{0.0f, 0.0f, 1.2f};
    CVector origin;
    VectorAdd(&origin, EntityPos(self->ped), &up);
    CPed* owner = self->ped;
    CVector direction;
    Ped_GetForward(owner, &direction);
    CVector source = origin;
    if (aiming) {
        uint8_t* view = View_GetMatrix(Camera_GetView(CameraViewSlot));
        source = F<CVector>(view, 0x24);
        CVector forward;
        direction = *Camera_GetForward(TheCamera, &forward);
    } else if (steady) {
        if (self->locked) {
            direction = self->lockDirection;
            if (CEntity* target = self->target) {
                source = *EntityPos(target);
                source.z = vc::Call<float>(target, 0x6C) * 0.6f + source.z;
                CVector step{direction.x * 0.2f, direction.y * 0.2f, direction.z * 0.2f};
                source.x = step.x + source.x;
                source.y = source.y + step.y;
                source.z = step.z + source.z;
            }
            self->halfAngle = self->halfAngleNormal;
        } else if (!self->isPlayer) {
            direction = self->lockDirection;
        }
    } else if (self->target) {
        CVector d;
        VectorSub(&d, EntityPos(self->target), EntityPos(owner));
        direction = d;
        Vector_Normalise(&direction);
    }
    self->source = source;
    self->origin = origin;
    self->direction = direction;
    DirectionToAngles(direction, &self->heading, &self->pitch);
    TS_ScanForTargets(self, force);
    TS_UpdateScores(self, force);
    TS_UpdateTargetScore(self, force);
    if (!steady && self->wasSteady) {
        TS_UpdateScores(self, 1);
    }
    self->wasSteady = steady;

    switch (self->state) {
    case 0:
        if (self->isPlayer) {
            TS_PickBestTarget(self);
        } else if (self->target) {
            self->lockTime = 0.0f;
            ClearRef(&self->target);
        }
        break;
    case 1:
        if (!self->isPlayer) {
            TS_ClearTarget(self);
        } else if (steady && self->locked) {
            if (TimeInMilliseconds - self->targetTime >= RetargetDelay) {
                TS_UpdateScores(self, 1);
                CEntity* best = TS_FindBestTarget(self);
                if (best) {
                    TS_SetTargetEntity(self, best);
                    if (self->target != previous) {
                        self->targetTime = TimeInMilliseconds;
                    }
                }
                if (self->target) {
                    CheckTargetRange(self, aiming, previous);
                }
            }
        } else if (!TS_Fn4D6770(self)) {
            if (aiming) {
                TS_ClearTarget(self);
            } else {
                TS_SetLockTarget(self, previous);
            }
        }
        break;
    case 3: {
        CEntity* lock = self->lockTarget;
        uint8_t down = 0;
        if (lock) {
            uint8_t* data = GetPedData(lock);
            if (data) {
                down = F<int16_t>(data, 0x220) <= 0;
            }
        }
        uint8_t keep = 1;
        if (!lock || !self->isPlayer || down) {
            keep = 0;
        } else {
            TargetEntry probe{};   // (the game's is uninitialised stack before the constructor)
            TargetEntry_Construct(&probe, lock);
            int32_t index = TargetList_Find(&self->list, &probe);
            TargetEntry_Destruct(&probe);
            if (index != -1 && self->list.data[index].score != kNoTarget) {
                keep = down;
                ClearRef(&self->refAC);
                ClearRef(&self->refB0);
                TS_PickBestTarget(self);
                if (TS_SetTargetEntity(self, self->lockTarget)) {
                    ClearRef(&self->lockTarget);
                    break;
                }
            }
        }
        if (steady && self->locked) {
            TS_UpdateScores(self, 1);
            CEntity* best = TS_FindBestTarget(self);
            if (best) {
                TS_PickBestTarget(self);
                TS_SetTargetEntity(self, best);
                if (self->target != previous) {
                    self->targetTime = TimeInMilliseconds;
                }
            }
            if (self->target) {
                ClearRef(&self->lockTarget);
                CheckTargetRange(self, aiming, previous);
            }
        } else if (!keep) {
            TS_ClearTarget(self);
        }
        break;
    }
    default:
        break;
    }

    // +0xB0: the player's dropped once +0xB4 is set, the others' when their time is up
    bool dropB0;
    if (self->isPlayer) {
        dropB0 = self->unknownB4 && self->refB0;
    } else {
        dropB0 = self->refB0 && self->timeB8 < TimeInMilliseconds;
    }
    if (dropB0) {
        ClearRef(&self->refB0);
        self->unknownB4 = 0;
    }
    if (self->isPlayer && self->target) {
        self->lockTime = float(double(GetFrameTimeMs()) + double(self->lockTime));
    }
}

#define TARGETING_C12C_REVERSED(name, address, fn)   \
    static void* RE_CONCAT(Original_, __LINE__);     \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

TARGETING_C12C_REVERSED("CTargetingSystem::Score", 0x004D70A0, &Score);
TARGETING_C12C_REVERSED("CTargetingSystem::GetLookTarget", 0x004D7780, &GetLookTarget);
TARGETING_C12C_REVERSED("CTargetingSystem::ClearPartnerTarget", 0x004D7800, &ClearPartnerTarget);
TARGETING_C12C_REVERSED("TargetEntry::VectorDeletingDtor", 0x004D7830, &TargetEntry_VectorDeletingDtorImpl);
TARGETING_C12C_REVERSED("TargetList::SetCapacity", 0x004D78C0, &TargetList_SetCapacityImpl);
TARGETING_C12C_REVERSED("CTargetingSystem::SetLockTarget", 0x004D7A40, &SetLockTarget);
TARGETING_C12C_REVERSED("CTargetingSystem::ScoreEntity", 0x004D7AF0, &ScoreEntity);
TARGETING_C12C_REVERSED("CTargetingSystem::UpdateScores", 0x004D7C30, &UpdateScores);
TARGETING_C12C_REVERSED("CTargetingSystem::UpdateTargetScore", 0x004D7D80, &UpdateTargetScore);
TARGETING_C12C_REVERSED("CTargetingSystem::UpdatePartnerTarget", 0x004D7F60, &UpdatePartnerTarget);
TARGETING_C12C_REVERSED("TargetList::PushBack", 0x004D7FF0, &TargetList_PushBackImpl);
TARGETING_C12C_REVERSED("CTargetingSystem::PickBestTarget", 0x004D8040, &PickBestTarget);
TARGETING_C12C_REVERSED("CTargetingSystem::AddCandidate", 0x004D8070, &AddCandidate);
TARGETING_C12C_REVERSED("CTargetingSystem::Clear", 0x004D81A0, &Clear);
TARGETING_C12C_REVERSED("TargetList::RemoveAt", 0x004D8230, &TargetList_RemoveAtImpl);
TARGETING_C12C_REVERSED("CTargetingSystem::Construct", 0x004D8370, &Construct);
TARGETING_C12C_REVERSED("CTargetingSystem::Destruct", 0x004D83F0, &Destruct);
TARGETING_C12C_REVERSED("CTargetingSystem::ScanForTargets", 0x004D84C0, &ScanForTargets);
TARGETING_C12C_REVERSED("CTargetingSystem::Update", 0x004D8A40, &Update);

} // namespace targeting_c12c
