// The points of interest, second part: CStaticPOI's and CDynamicPOI's constructors,
// CDynamicPOI's timer and virtuals, and the dynamic POIs' pool (0xA8-byte points, the
// pool at 0x20C7C64) with its new and delete. 0x4292D0..0x429496.

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
#define POI2_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

// A point of interest (CPOI's fields as far as these use them)
struct CPOI2 {
    re::Ptr32<re::Ptr32<void>> vtable;                // +0
    uint8_t unknown04[0x14];
    re::Ptr32<void> peds[2][2];             // +0x18 by slot: a ped (a registered reference) and ? (8 bytes a slot)
    uint8_t unknown28[0x44];
    re::Ptr32<void> group;                  // +0x6C its CPOIGroup
    uint8_t unknown70[0x30];
    uint8_t unknownA0;            // +0xA0 (0xFF when made)
    uint8_t unknownA1[3];
    float timer;                  // +0xA4 CDynamicPOI: counts down (in its group of one or none)
};
static_assert(offsetof(CPOI2, peds) == 0x18);
static_assert(offsetof(CPOI2, group) == 0x6C);
static_assert(offsetof(CPOI2, unknownA0) == 0xA0);
static_assert(offsetof(CPOI2, timer) == 0xA4);
static_assert(sizeof(CPOI2) == 0xA8);

inline re::Ptr32<void>* const poi2_staticVtable = reinterpret_cast<re::Ptr32<void>*>(0x00901858);
inline re::Ptr32<void>* const poi2_dynamicVtable = reinterpret_cast<re::Ptr32<void>*>(0x00901868);
GAME_VAR(CPool<uint8_t>*, poi2_dynamicPool, 0x020C7C64);

GAME_FN(void, __thiscall, POI2_Construct, 0x00447650, CPOI2* self);   // CPOI's constructor
GAME_FN(void, __thiscall, POI2_Group_Fn4897E0, 0x004897E0, void* group);
GAME_FN(int, __thiscall, POI2_Group_Count, 0x00489700, void* group);
GAME_FN(uint8_t, __thiscall, POI2_Ped_Fn473250, 0x00473250, void* ped, int arg);
GAME_FN(void, __thiscall, POI2_SetRef, 0x005D7F10, re::Ptr32<void>* ref, void* entity);
inline void* __cdecl POI2_Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(void*, __thiscall, POI2_DynamicPool_New, 0x004293E0, CPool<uint8_t>* pool);

} // namespace

namespace poi {

// 0x4292D0 (thiscall): CStaticPOI's constructor
CPOI2* __fastcall StaticPOI_Construct(CPOI2* self, re::Edx) {
    POI2_Construct(self);
    self->vtable = poi2_staticVtable;
    self->unknownA0 = 0xFF;
    return self;
}

// 0x4292F0 (thiscall): CDynamicPOI's constructor (its timer 0)
CPOI2* __fastcall DynamicPOI_Construct(CPOI2* self, re::Edx) {
    POI2_Construct(self);
    self->timer = 0.0f;
    self->unknownA0 = 0xFF;
    self->vtable = poi2_dynamicVtable;
    return self;
}

// 0x429320 vf3 (thiscall): its kind: 2
int32_t* __fastcall DynamicPOI_GetKind(CPOI2*, re::Edx, int32_t* out) {
    *out = 2;
    return out;
}

// 0x429330 (thiscall): CDynamicPOI's constructor with its timer
CPOI2* __fastcall DynamicPOI_ConstructTimer(CPOI2* self, re::Edx, float timer) {
    POI2_Construct(self);
    self->timer = timer;
    self->unknownA0 = 0xFF;
    self->vtable = poi2_dynamicVtable;
    return self;
}

// 0x429360 (thiscall): its group's 0x4897E0, when it has one
void __fastcall DynamicPOI_GroupFn(CPOI2* self, re::Edx) {
    if (self->group) {
        POI2_Group_Fn4897E0(self->group);
    }
}

// 0x429370 (thiscall): its timer down by the step while it's in a group of one or none
void __fastcall DynamicPOI_Update(CPOI2* self, re::Edx, float step) {
    if (!self->group || POI2_Group_Count(self->group) > 1) {
        return;
    }
    if (0.0f < self->timer) {
        self->timer = self->timer - step;
    }
}

// 0x4293B0 vf2 (thiscall): a slot's ped let go (its 0x473250 with 0, the reference cleared)
void __fastcall DynamicPOI_ReleaseSlot(CPOI2* self, re::Edx, int slot) {
    re::Ptr32<void>* ref = reinterpret_cast<re::Ptr32<void>*>(reinterpret_cast<uint8_t*>(self) + 0x18 + slot * 8);
    if (*ref) {
        POI2_Ped_Fn473250(*ref, 0);
        POI2_SetRef(ref, nullptr);
    }
}

// 0x4293E0 (thiscall): CPool::New for the dynamic POIs: the next free slot from the last
// one used, round once; zeroed
void* __fastcall DynamicPool_New(CPool<uint8_t>* self, re::Edx) {
    uint8_t* flags = self->flags;
    bool wrapped = false;
    if (!(flags[self->lastUsed] & 0x80)) {
        do {
            self->lastUsed++;
            if (self->lastUsed >= self->size) {
                if (wrapped) {
                    return nullptr;
                }
                self->lastUsed = 0;
                wrapped = true;
            }
        } while (!(flags[self->lastUsed] & 0x80));
    }
    flags[self->lastUsed] &= 0x7F;
    // (the reuse count in the low 7 bits, one up)
    uint8_t& flag = self->flags[self->lastUsed];
    flag = uint8_t(flag ^ (((flag + 1) ^ flag) & 0x7F));
    void* point = self->objects + self->elemSize * self->lastUsed;
    POI2_Memset(point, 0, 0xA8);
    return point;
}

// 0x429460: CDynamicPOI's operator new (from its pool; the size isn't looked at)
void* __cdecl DynamicPOI_New(uint32_t /*size*/) {
    return POI2_DynamicPool_New(poi2_dynamicPool);
}

// 0x429470: CDynamicPOI's operator delete (its slot back to the pool)
void __cdecl DynamicPOI_Delete(void* p) {
    CPool<uint8_t>* pool = poi2_dynamicPool;
    int slot = int((reinterpret_cast<intptr_t>(p) - re::PtrCast<intptr_t>(pool->objects)) / pool->elemSize);
    pool->flags[slot] |= 0x80;
    if (slot < pool->lastUsed) {
        pool->lastUsed = slot;
    }
}

POI2_REVERSED("CStaticPOI::Construct", 0x004292D0, &StaticPOI_Construct);
POI2_REVERSED("CDynamicPOI::Construct", 0x004292F0, &DynamicPOI_Construct);
POI2_REVERSED("CDynamicPOI::GetKind", 0x00429320, &DynamicPOI_GetKind);
POI2_REVERSED("CDynamicPOI::ConstructTimer", 0x00429330, &DynamicPOI_ConstructTimer);
POI2_REVERSED("CDynamicPOI::GroupFn", 0x00429360, &DynamicPOI_GroupFn);
POI2_REVERSED("CDynamicPOI::Update", 0x00429370, &DynamicPOI_Update);
POI2_REVERSED("CDynamicPOI::ReleaseSlot", 0x004293B0, &DynamicPOI_ReleaseSlot);
POI2_REVERSED("CDynamicPOI::Pool_New", 0x004293E0, &DynamicPool_New);
POI2_REVERSED("CDynamicPOI::New", 0x00429460, &DynamicPOI_New);
POI2_REVERSED("CDynamicPOI::Delete", 0x00429470, &DynamicPOI_Delete);

} // namespace poi
