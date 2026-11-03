// The points of interest (CPOI: a CPlaceable with 10 ped slots, a POI group, a kind),
// their lists (POIList: a chain of POI pool slots through the next-links at 0xBC5D08,
// POIList objects in the pool at 0x20C7C58) and the POI manager (0xC0F5E4: 20 entries,
// a CPOI of its own).
//
// CPOI: +0x18 the slots (10 of {an entity reference, a flag}), +0x68 a float, +0x6C the
// POI group, +0x70 the kind (0x10: any), +0x78 the ped type wanted (0xC: any), +0x80 the
// state (1: grouped), +0x81 the slots used (0 or less: 10), +0x82 disabled, +0x83 no
// peds already in a POI.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Pool.h"
#include "reversed.h"

namespace {

struct PoiSlot {
    re::Ptr32<void> ped;              // +0 (an entity reference: set through 0x5D7F10)
    uint8_t flag;           // +4
    uint8_t unknown5[3];    // +5
};
static_assert(sizeof(PoiSlot) == 8);

GAME_VAR(CPool<uint8_t>*, ms_poiPool, 0x020C7C84);        // the points (0xA4)
GAME_VAR(CPool<uint8_t>*, ms_poiListPool, 0x020C7C58);    // the POILists (0x1C)
GAME_VAR(uint16_t*, ms_poiNext, 0x00BC5D08);              // the chains' next links
inline void* const ms_poiChains = reinterpret_cast<void*>(0x00BC5D04);
GAME_VAR(uint8_t*, ms_pedPopulation, 0x00C2C108);
GAME_VAR(uint8_t*, ms_poiManager, 0x00C0F5E4);
GAME_VAR(const char*, ms_poiListName, 0x00A145E0);
// the models a POI takes no peds of
GAME_VAR(int32_t, ms_model_BF42DC, 0x00BF42DC);
GAME_VAR(int32_t, ms_model_BF42D4, 0x00BF42D4);
GAME_VAR(int32_t, ms_model_BF42D0, 0x00BF42D0);
GAME_VAR(int32_t, ms_model_BF42CC, 0x00BF42CC);
inline void* const CPOI_vftable = reinterpret_cast<void*>(0x00905684);
inline void* const POIList_vftable = reinterpret_cast<void*>(0x00905840);
inline void* const IArchivable_vftable = reinterpret_cast<void*>(0x00900CDC);
inline void* const ManagerPOI_vftable = reinterpret_cast<void*>(0x00901858);

GAME_FN(void, __thiscall, Ref_Set, 0x005D7F10, void* ref, void* entity);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, Placeable_Construct, 0x0046D9B0, void* self);
GAME_FN(void, __thiscall, Placeable_Destruct, 0x0046DD00, void* self);
GAME_FN(void, __thiscall, Matrix_Construct, 0x00412770, void* matrix);
GAME_FN(void, __thiscall, Placeable_SetMatrix, 0x0046DE90, void* self, const void* matrix, int);
GAME_FN(void, __thiscall, Group_AddMember, 0x00489F50, void* group, void* ped);
GAME_FN(void, __thiscall, Group_RemoveMember, 0x00489770, void* group, void* ped);
GAME_FN(int, __thiscall, Group_GetCount, 0x00489700, void* group);
GAME_FN(void, __thiscall, Group_ClearMembers, 0x004897E0, void* group);
GAME_FN(void, __thiscall, Ped_SetPOI, 0x00473200, void* ped, void* poi);
GAME_FN(void, __thiscall, Ped_Fn_473250, 0x00473250, void* ped, int);
GAME_FN(uint8_t, __thiscall, Ped_Fn_4765C0, 0x004765C0, void* ped);
GAME_FN(void, __thiscall, Ped_Fn_47B7E0, 0x0047B7E0, void* ped, int, int, int);
GAME_FN(void, __cdecl, Fn_52FE40, 0x0052FE40, int model);
GAME_FN(void, __thiscall, Population_RemovePOIGroup, 0x0049D480, void* population, void* group);
GAME_FN(uint8_t*, __thiscall, Objectives_GetCurrent, 0x00470340, void* objectives);
GAME_FN(void*, __thiscall, WanderObjective_Construct, 0x0046FC80, void* self, void* ped, float);
GAME_FN(void, __thiscall, Objectives_Add, 0x00470300, void* objectives, void* objective);
GAME_FN(uint8_t*, __thiscall, Fn_5DB960, 0x005DB960, void* self);
GAME_FN(void, __thiscall, Archive_ReadObject, 0x005B0720, void* archive, void* object, const char* name);
// (ours, called where modes apply)
GAME_FN(uint8_t, __thiscall, POI_IsFull, 0x00447590, uint8_t* poi);
GAME_FN(void, __thiscall, POI_ReleaseAll, 0x00447620, uint8_t* poi);
GAME_FN(void, __thiscall, POI_RemoveGroup, 0x004474C0, uint8_t* poi);
GAME_FN(void, __thiscall, POI_SetDisabled, 0x004478F0, uint8_t* poi, int disabled);
GAME_FN(void, __thiscall, POI_Destruct, 0x00447950, uint8_t* poi);
GAME_FN(uint8_t*, __thiscall, POI_Construct, 0x00447650, uint8_t* poi);
GAME_FN(void, __thiscall, Chains_Clear, 0x00447A70, void* chains, int head);
GAME_FN(uint8_t*, __thiscall, POIListPool_New, 0x00447B00, CPool<uint8_t>* pool);
GAME_FN(void*, __cdecl, POIList_New, 0x00447B80, size_t size);
GAME_FN(uint8_t*, __thiscall, POIList_Construct, 0x00447A30, uint8_t* list, int value);
GAME_FN(uint8_t, __thiscall, Group_ContainsPed, 0x00448300, void* group, void* ped);
GAME_FN(uint8_t*, __thiscall, Manager_Construct, 0x00448360, uint8_t* manager);
GAME_FN(void, __thiscall, Manager_Destruct, 0x004483B0, uint8_t* manager);
GAME_FN(uint8_t, __thiscall, Manager_Fn_448590, 0x00448590, uint8_t* manager, uint8_t* poi, float x, float y, float z);

using re::At;
template <typename R, typename... A>
R VCall(void* object, uint32_t offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

PoiSlot* Slots(uint8_t* poi) {
    return reinterpret_cast<PoiSlot*>(poi + 0x18);
}

int MaxSlots(uint8_t* poi) {
    int8_t n = int8_t(poi[0x81]);
    return n > 0 ? n : 10;
}

// the POI is one of the pool's (a slot's own address)
bool InPool(uint8_t* poi) {
    CPool<uint8_t>* pool = ms_poiPool;
    int index = int((poi - pool->objects) / pool->elemSize);
    if (index < 0 || index >= pool->size) {
        return false;
    }
    return poi == pool->objects + pool->elemSize * index;
}

// a POI pool slot (nullptr when free)
uint8_t* PoolPoint(uint32_t index) {
    CPool<uint8_t>* pool = ms_poiPool;
    if (pool->flags[index] & 0x80) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * index;
}

} // namespace

namespace POIs {

// 0x4473D0 (thiscall, 3 arguments unused): 0x446B40
static void* __fastcall Fn_4473D0(void* self, re::Edx, int, int, int) {
    return reinterpret_cast<void*(__thiscall*)(void*)>(0x00446B40)(self);
}

// 0x4473E0 (thiscall on a ped): its model one of three
static uint32_t __fastcall Ped_IsPOIModel(uint8_t* ped, re::Edx) {
    int model = At<int16_t>(ped, 0x10E);
    return model == ms_model_BF42D4 || model == ms_model_BF42D0 || model == ms_model_BF42CC;
}

// 0x447410 (thiscall): the ped into the first free slot (with the flag); into the group
// when grouped, and the POI to the ped
static uint8_t __fastcall POI_AddPed(uint8_t* self, re::Edx, uint8_t* ped, int flag) {
    if (At<void*>(self, 0x6C) && !(ms_pedPopulation[0x7898] & 0x10)) {
        return 0;
    }
    int max = MaxSlots(self);
    if (!ped || (self[0x83] && ped[0x1570] == 1)) {
        return 0;
    }
    int i = 0;
    for (; i < max; i++) {
        if (!Slots(self)[i].ped) {
            break;
        }
    }
    if (i >= max) {
        return 0;
    }
    Ref_Set(&Slots(self)[i], ped);
    Slots(self)[i].flag = uint8_t(flag);
    if (int8_t(self[0x80]) == 1 && At<void*>(self, 0x6C)) {
        Group_AddMember(At<void*>(self, 0x6C), ped);
    }
    Ped_SetPOI(ped, self);
    return 1;
}

// 0x4474C0 (thiscall): the POI group dropped (out of the population's, deleted)
static void __fastcall POI_RemoveGroupImpl(uint8_t* self, re::Edx) {
    uint8_t* group = At<uint8_t*>(self, 0x6C);
    if (!group) {
        return;
    }
    At<void*>(group, 0x40) = nullptr;
    Population_RemovePOIGroup(ms_pedPopulation, At<void*>(self, 0x6C));
    if (void* g = At<void*>(self, 0x6C)) {
        VCall<void>(g, 0, 1);
    }
    At<void*>(self, 0x6C) = nullptr;
    self[0x80] = 0;
}

// 0x447500 (thiscall): the slots with a ped
static int __fastcall POI_CountPeds(uint8_t* self, re::Edx) {
    int n = 0;
    for (int i = 0; i < 10; i++) {
        if (Slots(self)[i].ped) {
            n++;
        }
    }
    return n;
}

// 0x447560 (thiscall): the ped's slot's flag
static void __fastcall POI_SetPedFlag(uint8_t* self, re::Edx, void* ped, int flag) {
    if (!ped) {
        return;
    }
    for (int i = 0; i < 10; i++) {
        if (Slots(self)[i].ped == ped) {
            Slots(self)[i].flag = uint8_t(flag);
            return;
        }
    }
}

// 0x447590 (thiscall): no free slot
static uint8_t __fastcall POI_IsFullImpl(uint8_t* self, re::Edx) {
    int max = MaxSlots(self);
    for (int i = 0; i < max; i++) {
        if (!Slots(self)[i].ped) {
            return 0;
        }
    }
    return 1;
}

// 0x4475D0 (thiscall): the ped out (of the group too when grouped; its slot released
// through vf2)
static void __fastcall POI_RemovePed(uint8_t* self, re::Edx, void* ped) {
    if (!ped) {
        return;
    }
    int i = 0;
    for (; i < 10; i++) {
        if (Slots(self)[i].ped == ped) {
            break;
        }
    }
    if (i >= 10) {
        return;
    }
    if (int8_t(self[0x80]) == 1) {
        Group_RemoveMember(At<void*>(self, 0x6C), ped);
    }
    VCall<void>(self, 8, i);
}

// 0x447620 (thiscall): every used slot released (vf2)
static void __fastcall POI_ReleaseAllImpl(uint8_t* self, re::Edx) {
    for (int i = 0; i < 10; i++) {
        if (Slots(self)[i].ped) {
            VCall<void>(self, 8, i);
        }
    }
}

// 0x447650 (thiscall): the constructor
static uint8_t* __fastcall POI_ConstructImpl(uint8_t* self, re::Edx) {
    Placeable_Construct(self);
    At<void*>(self, 0) = CPOI_vftable;
    for (int i = 0; i < 10; i++) {
        Slots(self)[i].ped = nullptr;
    }
    At<void*>(self, 0x6C) = nullptr;
    At<int32_t>(self, 0x70) = 0x10;
    At<int32_t>(self, 0x78) = 0xC;
    At<int32_t>(self, 0x7C) = 10;
    self[0x80] = 0;
    self[0x81] = 0;
    self[0x82] = 0;
    self[0x83] = 0;
    uint8_t matrix[0x44];
    At<void*>(matrix, 0x40) = nullptr;
    Matrix_Construct(matrix);
    Placeable_SetMatrix(self, matrix, 0);
    for (int i = 0; i < 10; i++) {
        Ref_Set(&Slots(self)[i], nullptr);
        Slots(self)[i].flag = 1;
    }
    return self;
}

// CPOI vf1 (0x4476F0): can the ped come (not of the three models; the group not full and
// of its type, or the POI's type and not full; not already at this POI or one of its kind)
static uint8_t __fastcall POI_CanTakePed(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    int model = At<int16_t>(ped, 0x10E);
    if (model == ms_model_BF42DC || model == ms_model_BF42D4 || model == ms_model_BF42D0 || model == ms_model_BF42CC) {
        return 0;
    }
    if (void* group = At<void*>(self, 0x6C)) {
        if (VCall<uint8_t>(group, 4)) {
            return 0;
        }
        int32_t tmp;
        if (*VCall<int32_t*>(At<void*>(self, 0x6C), 8, &tmp) != 0xC &&
            *VCall<int32_t*>(At<void*>(self, 0x6C), 8, &tmp) != At<int32_t>(ped, 0x1310)) {
            return 0;
        }
    } else {
        int32_t type = At<int32_t>(self, 0x78);
        if (type != 0xC && type != At<int32_t>(ped, 0x1310)) {
            return 0;
        }
        if (POI_IsFull(self)) {
            return 0;
        }
    }
    uint8_t* current = At<uint8_t*>(ped, 0x1D28);
    if (current) {
        if (current == self) {
            return 0;
        }
        int32_t kind = At<int32_t>(current, 0x70);
        if (kind != 0x10 && kind == At<int32_t>(self, 0x70)) {
            return 0;
        }
    }
    return 1;
}

// CPOI vf2 (0x4477B0): the slot released: a wander objective (0.3) for a ped of kind 1 on
// objective 9, the ped let go (and 0x47B7E0 unless 0x4765C0, for flagged slots)
static void __fastcall POI_ReleaseSlot(uint8_t* self, re::Edx, int i) {
    PoiSlot* slot = &Slots(self)[i];
    uint8_t* ped = static_cast<uint8_t*>(slot->ped);
    if (!ped) {
        return;
    }
    if (ped[0x1570] == 1) {
        uint8_t* objective = Objectives_GetCurrent(ped + 0x5F0);
        if (objective && At<int32_t>(objective, 8) == 9) {
            if (void* memory = Malloc(0x14)) {
                if (void* wander = WanderObjective_Construct(memory, slot->ped, 0.30000001192092896f)) {
                    Objectives_Add(static_cast<uint8_t*>(slot->ped) + 0x5F0, wander);
                }
            }
        }
    }
    Ped_Fn_473250(slot->ped, 0);
    if (slot->flag && !Ped_Fn_4765C0(slot->ped)) {
        Fn_52FE40(At<int16_t>(slot->ped, 0x10E));
        Ped_Fn_47B7E0(slot->ped, 1, 0, 1);
    }
    Ref_Set(slot, nullptr);
    slot->flag = 1;
}

// 0x447870 (thiscall): the group's members as the slots (their count cleared first when
// 10 or fewer), grouped
static void __fastcall POI_SetGroup(uint8_t* self, re::Edx, uint8_t* group) {
    POI_ReleaseAll(self);
    if (!group) {
        return;
    }
    if (Group_GetCount(group) <= 10) {
        Group_ClearMembers(group);
    }
    for (uint32_t i = 0; int(i) < Group_GetCount(group); i++) {
        void* member = i < At<uint32_t>(group, 0x2C) ? At<void*>(group, 4 + i * 4) : nullptr;
        Ref_Set(&Slots(self)[i], member);
    }
    self[0x80] = 1;
    At<uint8_t*>(self, 0x6C) = group;
}

// 0x4478F0 (thiscall): disabled or not (enabling nothing else; disabling releases the
// slots and the group, unless it's a kind 3 POI outside the pool)
static void __fastcall POI_SetDisabledImpl(uint8_t* self, re::Edx, int disabled) {
    uint8_t flag = uint8_t(disabled);
    if (!flag) {
        if (At<int32_t>(self, 0x70) == 3 && !InPool(self)) {
            return;
        }
        POI_ReleaseAll(self);
        POI_RemoveGroup(self);
        self[0x80] = 0;
    }
    self[0x82] = flag;
}

// 0x447950 (thiscall): the destructor
static void __fastcall POI_DestructImpl(uint8_t* self, re::Edx) {
    At<void*>(self, 0) = CPOI_vftable;
    if (At<int32_t>(self, 0x70) != 3 || InPool(self)) {
        POI_ReleaseAll(self);
        POI_RemoveGroup(self);
        self[0x80] = 0;
        self[0x82] = 0;
    }
    for (int i = 9; i >= 0; i--) {
        Ref_Set(&Slots(self)[i], nullptr);
    }
    Placeable_Destruct(self);
}

// CPOI vf0 (0x4479D0): the destructor (flags 1: and freed)
static uint8_t* __fastcall POI_Destroy(uint8_t* self, re::Edx, int flags) {
    POI_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x4479F0: do the names differ
static uint8_t __cdecl NamesDiffer(const re::Ptr32<const char>* a, const char* b) {
    return std::strcmp(*a, b) != 0;
}

// 0x447A30 (thiscall): POIList's constructor
static uint8_t* __fastcall POIList_ConstructImpl(uint8_t* self, re::Edx, int value) {
    At<int32_t>(self, 0x10) = value;
    At<void*>(self, 0) = POIList_vftable;
    self[0x14] = 0xFF;
    At<uint16_t>(self, 0x16) = 0xFFFF;
    At<uint16_t>(self, 0x18) = 0xFFFF;
    return self;
}

// 0x447A60 (thiscall): POIList's destructor
static void __fastcall POIList_Destruct(uint8_t* self, re::Edx) {
    self[0x14] = 0xFF;
    At<void*>(self, 0) = IArchivable_vftable;
}

// 0x447A70 (thiscall, on 0xBC5D04): the chain from the slot unlinked (0xFFFF), up to its
// end (0xFFFE); +8 to none
static void __fastcall Chains_ClearImpl(uint8_t* self, re::Edx, int head) {
    uint16_t* next = At<uint16_t*>(self, 4);
    uint16_t link = uint16_t(head);
    uint16_t old;
    do {
        old = next[link];
        next[link] = 0xFFFF;
        link = old;
    } while (old != 0xFFFE);
    At<uint16_t>(self, 8) = 0xFFFF;
}

// 0x447AC0 (thiscall): the list's length
static int __fastcall POIList_Count(uint8_t* self, re::Edx) {
    uint16_t link = At<uint16_t>(self, 0x16);
    if (link == 0xFFFF) {
        return 0;
    }
    int n = 1;
    for (link = ms_poiNext[link]; link != 0xFFFE; link = ms_poiNext[link]) {
        n++;
    }
    return n;
}

// 0x447B00 (thiscall): CPool<POIList>::New: the next free slot from the last one used,
// round once; zeroed
static uint8_t* __fastcall POIListPool_NewImpl(CPool<uint8_t>* self, re::Edx) {
    bool wrapped = false;
    if (!(self->flags[self->lastUsed] & 0x80)) {
        do {
            self->lastUsed++;
            if (self->lastUsed >= self->size) {
                if (wrapped) {
                    return nullptr;
                }
                self->lastUsed = 0;
                wrapped = true;
            }
        } while (!(self->flags[self->lastUsed] & 0x80));
    }
    self->flags[self->lastUsed] &= 0x7F;
    uint8_t& flag = self->flags[self->lastUsed];
    flag = uint8_t(flag ^ (((flag + 1) ^ flag) & 0x7F));
    uint8_t* object = self->objects + self->elemSize * self->lastUsed;
    std::memset(object, 0, 0x1C);
    return object;
}

// 0x447B80: POIList's operator new (the size unused)
static void* __cdecl POIList_NewImpl(size_t) {
    return POIListPool_New(ms_poiListPool);
}

// 0x447B90 (thiscall): the list's first point (the cursor after it)
static uint8_t* __fastcall POIList_First(uint8_t* self, re::Edx) {
    uint16_t head = At<uint16_t>(self, 0x16);
    if (head == 0xFFFF) {
        return nullptr;
    }
    At<uint16_t>(self, 0x18) = ms_poiNext[head];
    return PoolPoint(head);
}

// 0x447BD0 (thiscall): the list's next point
static uint8_t* __fastcall POIList_Next(uint8_t* self, re::Edx) {
    uint16_t cursor = At<uint16_t>(self, 0x18);
    if (cursor == 0xFFFF || cursor == 0xFFFE) {
        return nullptr;
    }
    At<uint16_t>(self, 0x18) = ms_poiNext[cursor];
    return PoolPoint(cursor);
}

// 0x447C20 (thiscall): every point of the list disabled and deleted, the chain unlinked
static void __fastcall POIList_Clear(uint8_t* self, re::Edx) {
    uint16_t head = At<uint16_t>(self, 0x16);
    if (head == 0xFFFF) {
        return;
    }
    At<uint16_t>(self, 0x18) = ms_poiNext[head];
    uint8_t* point = PoolPoint(head);
    while (point) {
        uint8_t* next = nullptr;
        uint16_t cursor = At<uint16_t>(self, 0x18);
        if (cursor != 0xFFFF && cursor != 0xFFFE) {
            At<uint16_t>(self, 0x18) = ms_poiNext[cursor];
            next = PoolPoint(cursor);
        }
        POI_SetDisabled(point, 0);
        if (point) {
            VCall<void>(point, 0, 1);
        }
        point = next;
    }
    Chains_Clear(ms_poiChains, At<uint16_t>(self, 0x16));
    At<uint16_t>(self, 0x16) = 0xFFFF;
}

// 0x447CF0: POIList's slot back to its pool
static void __cdecl POIList_Delete(uint8_t* list) {
    CPool<uint8_t>* pool = ms_poiListPool;
    int index = int((list - pool->objects) / pool->elemSize);
    pool->flags[index] |= 0x80;
    if (index < pool->lastUsed) {
        pool->lastUsed = index;
    }
}

// 0x448230 (thiscall): an entity reference copied
static void* __fastcall Ref_Copy(void* self, re::Edx, const re::Ptr32<void>* other) {
    Ref_Set(self, *other);
    return self;
}

// 0x448250 (thiscall; unused): `count` POILists loaded (with 0x5DB960's +0x44, else -2),
// the empty ones deleted
static void __fastcall LoadPOILists(void*, re::Edx, void* archive, int count, int) {
    uint8_t* owner = Fn_5DB960(reinterpret_cast<void*>(0x00D02850));
    int value = owner ? At<int32_t>(owner, 0x44) : -2;
    for (int i = count; i > 0; i--) {
        void* memory = POIList_New(0x1C);
        uint8_t* list = memory ? POIList_Construct(static_cast<uint8_t*>(memory), value) : nullptr;
        Archive_ReadObject(archive, list, ms_poiListName);
        if (At<uint16_t>(list, 0x16) == 0xFFFF) {
            VCall<void>(list, 0, 1);
        }
    }
}

// 0x4482C0 (thiscall on the POI manager): an entry out (the rest moved down)
static void __fastcall Manager_RemoveEntry(uint8_t* self, re::Edx, uint32_t index) {
    for (uint32_t i = index; i < At<uint32_t>(self, 0xA0) - 1; i++) {
        At<int32_t>(self, i * 8) = At<int32_t>(self, i * 8 + 8);
        At<int32_t>(self, i * 8 + 4) = At<int32_t>(self, i * 8 + 0xC);
    }
    At<int32_t>(self, 0xA0) += -1;
}

// 0x448300 (thiscall on a POI group): is the ped a member (through a reference)
static uint8_t __fastcall Group_ContainsPedImpl(uint8_t* self, re::Edx, void* ped) {
    void* ref = nullptr;
    Ref_Set(&ref, ped);
    int found = -1;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        if (At<void*>(self, 4 + i * 4) == ref) {
            found = int(i);
            break;
        }
    }
    Ref_Set(&ref, nullptr);
    return found != -1;
}

// 0x448360 (thiscall): the POI manager's constructor
static uint8_t* __fastcall Manager_ConstructImpl(uint8_t* self, re::Edx) {
    for (int i = 0; i < 20; i++) {
        self[i * 8] = 1;
        At<int32_t>(self, i * 8 + 4) = 0;
    }
    At<int32_t>(self, 0xA0) = 0;
    uint8_t* poi = self + 0xA8;
    POI_Construct(poi);
    At<void*>(poi, 0) = ManagerPOI_vftable;
    poi[0xA0] = 0xFF;
    self[0xA5] = 1;
    return self;
}

// 0x4483B0 (thiscall): the POI manager's destructor (its entries' objects deleted)
static void __fastcall Manager_DestructImpl(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xA0); i++) {
        if (void* object = At<void*>(self, i * 8 + 4)) {
            VCall<void>(object, 0, 1);
        }
    }
    At<int32_t>(self, 0xA0) = 0;
    At<void*>(self, 0xA8) = ManagerPOI_vftable;
    POI_Destruct(self + 0xA8);
}

// 0x448400: the POI manager made (once) and switched on
static void __cdecl Manager_Init() {
    if (!ms_poiManager) {
        void* memory = Malloc(0x150);
        ms_poiManager = memory ? Manager_Construct(static_cast<uint8_t*>(memory)) : nullptr;
    }
    ms_poiManager[0xA4] = 1;
    ms_poiManager[0xA5] = 1;
    At<int32_t>(ms_poiManager, 0x14C) = 0;
}

// 0x448460 (thiscall): the object of the entry (flag 0) whose POI's group has the ped
static void* __fastcall Manager_FindByPed(uint8_t* self, re::Edx, void* ped) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xA0); i++) {
        if (self[i * 8] == 0) {
            uint8_t* object = At<uint8_t*>(self, i * 8 + 4);
            if (Group_ContainsPed(At<void*>(object, 0x6C), ped)) {
                return object;
            }
        }
    }
    return nullptr;
}

// 0x4484B0: the POIList pool's size
static int32_t __cdecl POIListPool_GetSize() {
    return ms_poiListPool->size;
}

// 0x4484C0 (thiscall): CPOI's assignment (the placement, the slots through references,
// the fields from +0x68)
static uint8_t* __fastcall POI_Assign(uint8_t* self, re::Edx, uint8_t* other) {
    for (uint32_t o = 4; o <= 0x14; o += 4) {
        At<int32_t>(self, o) = At<int32_t>(other, o);
    }
    for (int i = 0; i < 10; i++) {
        Ref_Set(&Slots(self)[i], Slots(other)[i].ped);
        Slots(self)[i].flag = Slots(other)[i].flag;
    }
    At<float>(self, 0x68) = At<float>(other, 0x68);
    for (uint32_t o = 0x6C; o <= 0x7C; o += 4) {
        At<int32_t>(self, o) = At<int32_t>(other, o);
    }
    self[0x80] = other[0x80];
    self[0x81] = other[0x81];
    self[0x82] = other[0x82];
    self[0x83] = other[0x83];
    return self;
}

// 0x448730 (thiscall): 0x448590 while the manager is on
static uint8_t __fastcall Manager_Fn_448730(uint8_t* self, re::Edx, uint8_t* poi, float x, float y, float z) {
    if (!ms_poiManager[0xA4] || !ms_poiManager[0xA5]) {
        return 0;
    }
    return Manager_Fn_448590(self, poi, x, y, z);
}

// 0x448780 (thiscall): the same for enabled POIs (or disabled kind 3 ones while +0x14C
// is clear)
static uint8_t __fastcall Manager_Fn_448780(uint8_t* self, re::Edx, uint8_t* poi, float x, float y, float z) {
    if (!ms_poiManager[0xA4] || !ms_poiManager[0xA5]) {
        return 0;
    }
    if (poi[0x82] && (At<int32_t>(poi, 0x70) != 3 || At<int32_t>(self, 0x14C) != 0)) {
        return 0;
    }
    return Manager_Fn_448590(self, poi, x, y, z);
}

// 0x448960: the POI manager deleted
static void __cdecl Manager_Shutdown() {
    if (uint8_t* manager = ms_poiManager) {
        Manager_Destruct(manager);
        Free(manager);
        ms_poiManager = nullptr;
    }
}

} // namespace POIs

// ---- registration ----------------------------------------------------------------------

#define C03F_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace POIs {
C03F_REVERSED("CPOI::Fn_4473D0", 0x004473D0, &Fn_4473D0);
C03F_REVERSED("CPed::IsPOIModel", 0x004473E0, &Ped_IsPOIModel);
C03F_REVERSED("CPOI::AddPed", 0x00447410, &POI_AddPed);
C03F_REVERSED("CPOI::RemoveGroup", 0x004474C0, &POI_RemoveGroupImpl);
C03F_REVERSED("CPOI::CountPeds", 0x00447500, &POI_CountPeds);
C03F_REVERSED("CPOI::SetPedFlag", 0x00447560, &POI_SetPedFlag);
C03F_REVERSED("CPOI::IsFull", 0x00447590, &POI_IsFullImpl);
C03F_REVERSED("CPOI::RemovePed", 0x004475D0, &POI_RemovePed);
C03F_REVERSED("CPOI::ReleaseAll", 0x00447620, &POI_ReleaseAllImpl);
C03F_REVERSED("CPOI::Construct", 0x00447650, &POI_ConstructImpl);
C03F_REVERSED("CPOI::CanTakePed", 0x004476F0, &POI_CanTakePed);
C03F_REVERSED("CPOI::ReleaseSlot", 0x004477B0, &POI_ReleaseSlot);
C03F_REVERSED("CPOI::SetGroup", 0x00447870, &POI_SetGroup);
C03F_REVERSED("CPOI::SetDisabled", 0x004478F0, &POI_SetDisabledImpl);
C03F_REVERSED("CPOI::Destruct", 0x00447950, &POI_DestructImpl);
C03F_REVERSED("CPOI::Destroy", 0x004479D0, &POI_Destroy);
C03F_REVERSED("POIs::NamesDiffer", 0x004479F0, &NamesDiffer);
C03F_REVERSED("POIList::Construct", 0x00447A30, &POIList_ConstructImpl);
C03F_REVERSED("POIList::Destruct", 0x00447A60, &POIList_Destruct);
C03F_REVERSED("POIChains::Clear", 0x00447A70, &Chains_ClearImpl);
C03F_REVERSED("POIList::Count", 0x00447AC0, &POIList_Count);
C03F_REVERSED("CPool<POIList>::New", 0x00447B00, &POIListPool_NewImpl);
C03F_REVERSED("POIList::operator new", 0x00447B80, &POIList_NewImpl);
C03F_REVERSED("POIList::First", 0x00447B90, &POIList_First);
C03F_REVERSED("POIList::Next", 0x00447BD0, &POIList_Next);
C03F_REVERSED("POIList::Clear", 0x00447C20, &POIList_Clear);
C03F_REVERSED("POIList::operator delete", 0x00447CF0, &POIList_Delete);
C03F_REVERSED("CEntityRef::Copy", 0x00448230, &Ref_Copy);
C03F_REVERSED("POIs::LoadPOILists", 0x00448250, &LoadPOILists);
C03F_REVERSED("CPOIManager::RemoveEntry", 0x004482C0, &Manager_RemoveEntry);
C03F_REVERSED("CPOIGroup::ContainsPed", 0x00448300, &Group_ContainsPedImpl);
C03F_REVERSED("CPOIManager::Construct", 0x00448360, &Manager_ConstructImpl);
C03F_REVERSED("CPOIManager::Destruct", 0x004483B0, &Manager_DestructImpl);
C03F_REVERSED("CPOIManager::Init", 0x00448400, &Manager_Init);
C03F_REVERSED("CPOIManager::FindByPed", 0x00448460, &Manager_FindByPed);
C03F_REVERSED("CPool<POIList>::GetSize", 0x004484B0, &POIListPool_GetSize);
C03F_REVERSED("CPOI::Assign", 0x004484C0, &POI_Assign);
C03F_REVERSED("CPOIManager::Fn_448730", 0x00448730, &Manager_Fn_448730);
C03F_REVERSED("CPOIManager::Fn_448780", 0x00448780, &Manager_Fn_448780);
C03F_REVERSED("CPOIManager::Shutdown", 0x00448960, &Manager_Shutdown);
} // namespace POIs
