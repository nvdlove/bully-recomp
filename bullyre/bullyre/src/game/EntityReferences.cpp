// CReferences: the pointers to an entity that are cleared when it goes (GTA's). An
// entity's +0x118 is its list of references (CEntity::unknown118 in Entity.h), each
// the address of a pointer to it; the free ones are a list of their own.

#include <cstddef>
#include <cstdint>

#include "game/Entity.h"
#include "game/Pools.h"
#include "reversed.h"

namespace {

struct CReference {
    re::Ptr32<CReference> next;    // +0
    re::Ptr32<re::Ptr32<CEntity>> pentity;   // +4
};
static_assert(sizeof(CReference) == 8);

constexpr int NUM_REFERENCES = 1200;
GAME_ARRAY(CReference, aRefs, 0x00C0F7C0);           // [1200]
GAME_VAR(CReference*, pEmptyList, 0x00C11D40);
GAME_VAR(int32_t, refsInitCounter, 0x00C11D38);        // (Init's loop counter, a global: 0 after it)

GAME_FN(int, __thiscall, Pool_GetSizeChained, 0x00435BB0, void* pool);
GAME_FN(void*, __thiscall, Pool_GetAtChained, 0x00435BD0, void* pool, int slot, int onlyUsed);

re::Ptr32<CReference>& FirstReference(CEntity* entity) {
    return *reinterpret_cast<re::Ptr32<CReference>*>(&entity->unknown118);
}

// 0x450970's loop (inlined in 0x4509B0 three times): the references that no longer
// point at the entity back to the free list
void PruneReferencesOf(CEntity* entity) {
    re::Ptr32<CReference>* link = &FirstReference(entity);
    CReference* ref = *link;
    while (ref) {
        if (*ref->pentity == entity) {
            link = &ref->next;
            ref = ref->next;
        } else {
            CReference* next = ref->next;
            *link = next;
            ref->next = pEmptyList;
            pEmptyList = ref;
            ref = next;
        }
    }
}

namespace CReferences {

// 0x450850: every reference to the free list. (The last one's next is the array's
// end, which is pEmptyList's own address: the game's, kept)
void __cdecl Init() {
    pEmptyList = &aRefs[0];
    for (int i = 0; i < NUM_REFERENCES; i++) {
        aRefs[i].next = &aRefs[i + 1];
        aRefs[i].pentity = nullptr;
    }
    refsInitCounter = 0;
}

// 0x4509B0: PruneReferences for every ped, vehicle and object (the objects' chain too)
void __cdecl PruneAllReferencesInWorld() {
    CPool<CPed>* peds = CPools::ms_pPedPool;
    for (int slot = peds->size; slot != 0;) {
        slot--;
        if (!peds->IsFreeSlot(slot)) {
            auto* ped = reinterpret_cast<CEntity*>(re::PtrCast<uint8_t*>(peds->objects) + peds->elemSize * slot);
            if (ped) {
                PruneReferencesOf(ped);
            }
        }
    }
    CPool<CVehicle>* vehicles = CPools::ms_pVehiclePool;
    for (int slot = vehicles->size; slot != 0;) {
        slot--;
        if (!vehicles->IsFreeSlot(slot)) {
            auto* vehicle = reinterpret_cast<CEntity*>(re::PtrCast<uint8_t*>(vehicles->objects) + vehicles->elemSize * slot);
            if (vehicle) {
                PruneReferencesOf(vehicle);
            }
        }
    }
    CPool<CObject>* objects = CPools::ms_pObjectPool;
    int count = objects->size;
    if (objects->unknown10) {
        count += Pool_GetSizeChained(objects->Next());
    }
    while (count != 0) {
        count--;
        CEntity* object = nullptr;
        if (count < objects->size) {
            object = reinterpret_cast<CEntity*>(re::PtrCast<uint8_t*>(objects->objects) + objects->elemSize * count);
        } else if (objects->unknown10) {
            object = static_cast<CEntity*>(Pool_GetAtChained(objects->Next(), count - objects->size, 1));
        }
        // (the flag read is the object pool's, past its end for chained slots)
        if (!objects->IsFreeSlot(count) && object) {
            PruneReferencesOf(object);
        }
    }
}

} // namespace CReferences

// 0x450880 (thiscall): `ref` is cleared when the entity goes (once; nothing when no
// reference is free)
void __fastcall Entity_RegisterReference(CEntity* self, re::Edx, re::Ptr32<CEntity>* ref) {
    for (CReference* r = FirstReference(self); r; r = r->next) {
        if (r->pentity == ref) {
            return;
        }
    }
    CReference* r = pEmptyList;
    if (r) {
        pEmptyList = r->next;
        r->next = FirstReference(self);
        FirstReference(self) = r;
        r->pentity = ref;
    }
}

// 0x4508D0 (thiscall): `ref` no longer
void __fastcall Entity_CleanUpOldReference(CEntity* self, re::Edx, re::Ptr32<CEntity>* ref) {
    re::Ptr32<CReference>* link = &FirstReference(self);
    for (CReference* r = *link; r; r = r->next) {
        if (r->pentity == ref) {
            *link = r->next;
            r->next = pEmptyList;
            pEmptyList = r;
            return;
        }
        link = &r->next;
    }
}

// 0x450910 (thiscall): every pointer still at this entity cleared, the references freed
void __fastcall Entity_ResolveReferences(CEntity* self, re::Edx) {
    for (CReference* r = FirstReference(self); r; r = r->next) {
        if (*r->pentity == self) {
            *r->pentity = nullptr;
        }
    }
    CReference* r = FirstReference(self);
    if (r) {
        while (r->next) {
            r = r->next;
        }
        r->next = pEmptyList;
        pEmptyList = FirstReference(self);
        FirstReference(self) = nullptr;
    }
}

// 0x450970 (thiscall): the references that no longer point here freed
void __fastcall Entity_PruneReferences(CEntity* self, re::Edx) {
    PruneReferencesOf(self);
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define C04B_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04B_REVERSED("CReferences::Init", 0x00450850, &CReferences::Init);
C04B_REVERSED("CReferences::RegisterReference_450880", 0x00450880, &Entity_RegisterReference);
C04B_REVERSED("CReferences::CleanUpOldReference_4508D0", 0x004508D0, &Entity_CleanUpOldReference);
C04B_REVERSED("CEntity::ResolveReferences", 0x00450910, &Entity_ResolveReferences);
C04B_REVERSED("CEntity::PruneReferences", 0x00450970, &Entity_PruneReferences);
C04B_REVERSED("CReferences::PruneAllReferencesInWorld", 0x004509B0, &CReferences::PruneAllReferencesInWorld);
