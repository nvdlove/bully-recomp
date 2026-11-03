#include "game/World.h"

#include <cmath>

#include "game/Entity.h"
#include "game/ModelInfo.h"
#include "game/Pools.h"
#include "reversed.h"

using col::CVector;

namespace {
GAME_FN(void*, __cdecl, PtrNodeNew, 0x0044E030, size_t size);        // CPtrNode::operator new
GAME_FN(void, __cdecl, PtrNodeDelete, 0x0044E040, void* node);       // CPtrNode::operator delete
GAME_FN(void*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, int slot);
GAME_FN(void, __cdecl, GetEntityPoolAndSlot, 0x0044C930, CEntity* entity, int32_t* pool, int32_t* slot);
GAME_FN(void, __thiscall, PtrNode_RemoveFromList, 0x0044DF30, CPtrNode* node, CPtrList* list);
GAME_FN(CPtrNode*, __thiscall, PtrNode_Construct, 0x0045D670, CPtrNode* node, CEntity* entity);
GAME_FN(void, __thiscall, PtrList_Flush, 0x0044E070, CPtrList* list);
GAME_FN(void, __thiscall, Physical_AddToMovingList, 0x004697D0, CEntity* entity);
GAME_FN(void, __thiscall, Physical_RemoveFromMovingList, 0x00469840, CEntity* entity);
GAME_FN(float, __thiscall, Entity_GetBoundRadius, 0x00465E10, CEntity* entity);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, ClearScanCodes_, 0x0045D840);
GAME_FN(CEntity*, __cdecl, FindNearestObjectOfTypeSectorList_, 0x0045D750, int model, const CVector* pos, CPtrList* list,
        CEntity* found, float* nearest);
GAME_FN(CEntity*, __cdecl, FindNearestObjectOfType_, 0x0045DE60, int model, float range, const CVector* pos);
GAME_FN(void, __cdecl, FindObjectsOfTypeInRangeSectorList_, 0x0045DB40, int model, CPtrList* list, const CVector* pos, float range,
        int16_t* count, int16_t maxCount, re::Ptr32<CEntity>* out);
GAME_FN(void, __cdecl, FindObjectsInRangeSectorList_, 0x0045DC00, CPtrList* list, const CVector* pos, float range, int twoD,
        int16_t* count, int16_t maxCount, re::Ptr32<CEntity>* out);
// the renderer's lists of what's lit / visible (on 0xD12AF8)
GAME_FN(void, __thiscall, VisibilityList_Add, 0x005E6510, void* list, CEntity* entity);
GAME_FN(void, __thiscall, VisibilityList_Remove, 0x005E6990, void* list, CEntity* entity);
inline void* const TheVisibilityList = reinterpret_cast<void*>(0x00D12AF8);
GAME_VAR(float, ms_fNearestObjectRange, 0x00A14A90);   // SetNearestObjectOfTypeFlags' range
GAME_FN(void, __thiscall, Physical_RemoveCollisionRecord, 0x00468D80, CEntity* physical, CEntity* entity);
GAME_FN(int, __thiscall, Pool_GetChainSize, 0x00435BB0, CPool<uint8_t>* pool);   // the chained pools' slots
GAME_FN(void*, __thiscall, Pool_GetChainedAt, 0x00435BD0, CPool<uint8_t>* pool, int slot, int);
GAME_FN(void, __cdecl, RegisterReference_, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, CleanUpOldReference_, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
}

#define WORLD_REVERSED(name, address, fn)                                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

// ---- CPtrNode, CPtrList ---------------------------------------------------------------------

CPtrNode* CPtrNode::GetNext() const {
    uint32_t next = NextIndex();
    return next == NO_NEXT ? nullptr : &CPools::ms_pPtrNodes[next];
}

CEntity* CPtrNode::GetItem() const {
    return static_cast<CEntity*>(GetEntityInPool(bits & 0xF, (bits >> 4) & 0x3FFF));
}

void CPtrNode::LinkTo(CPtrNode* next) {
    if (next == nullptr) {
        bits |= 0xFFFC0000;
    } else {
        // (the index is cut to 16 bits before the shift)
        uint32_t index = uint16_t((reinterpret_cast<intptr_t>(next) - re::PtrCast<intptr_t>(CPools::ms_pPtrNodes)) >> 2);
        bits = index << 18 | (bits & 0x3FFFF);
    }
}

CPtrNode* CPtrNode::Construct(CEntity* entity) {
    int32_t pool, slot;
    GetEntityPoolAndSlot(entity, &pool, &slot);
    bits = (uint32_t(slot) | 0xFFFFC000) << 4 | (uint32_t(pool) & 0xF);
    return this;
}

void CPtrNode::RemoveFromList(CPtrList* list) {
    CPtrNode* node = list->first;
    if (node == this) {
        list->first = GetNext();
        return;
    }
    while (node != nullptr) {
        uint32_t next = node->NextIndex();
        CPtrNode* nextNode = next == NO_NEXT ? nullptr : &CPools::ms_pPtrNodes[next];
        if (nextNode == this) {
            // the one before takes its next
            node->bits = (bits & 0xFFFC0000) | (node->bits & 0x3FFFF);
            return;
        }
        if (next == NO_NEXT) {
            return;
        }
        node = &CPools::ms_pPtrNodes[next];
    }
}

void CPtrList::Flush() {
    CPtrNode* node = first;
    while (node != nullptr) {
        CPtrNode* next = node->GetNext();
        PtrNode_RemoveFromList(node, this);
        PtrNodeDelete(node);  // (inlined in the game)
        node = next;
    }
}

WORLD_REVERSED("CPtrNode::RemoveFromList", 0x0044DF30, &CPtrNode::RemoveFromList);
WORLD_REVERSED("CPtrNode::CPtrNode", 0x0045D670, &CPtrNode::Construct);
WORLD_REVERSED("CPtrList::Flush", 0x0044E070, &CPtrList::Flush);

// ---- sectors ------------------------------------------------------------------------------

// The sector a coordinate is in, the way the game does it: truncated, then clamped
// to the map as floats
static float SectorCoord(float v) {
    return float(int(v / 50.0 + 18.0));
}
static int SectorLow(float v) {
    float f = SectorCoord(v);
    return int(f > 0.0f ? f : 0.0f);
}
static int SectorHigh(float v) {
    float f = SectorCoord(v);
    return int(f < 35.0f ? f : 35.0f);
}

// (inlined in the game: GTA's AdvanceCurrentScanCode)
static void AdvanceCurrentScanCode() {
    if (CWorld::ms_nCurrentScanCode >= 0xFFFF) {
        ClearScanCodes_();
        CWorld::ms_nCurrentScanCode = 1;
    } else {
        CWorld::ms_nCurrentScanCode++;
    }
}

// (CVector::Magnitude inlined)
static float Magnitude(const CVector& v) {
    return std::sqrt(std::fabs(v.x * v.x + v.y * v.y + v.z * v.z));
}
static float Magnitude2D(const CVector& v) {
    return std::sqrt(std::fabs(v.x * v.x + v.y * v.y));
}

namespace CWorld {

void __cdecl Add(CEntity* entity) {
    if (entity == nullptr) {
        return;
    }
    if (entity->bIsBIGBuilding) {
        CPtrNode* node = static_cast<CPtrNode*>(PtrNodeNew(4));
        node = node ? PtrNode_Construct(node, entity) : nullptr;
        ms_bigBuildingsList.AddNode(node);
    } else {
        entity->CallAdd();
    }
    if (!entity->bIsStatic && !entity->bIsStaticWaitingForCollision && entity->bIsPhysical) {
        Physical_AddToMovingList(entity);
    }
}

void __cdecl Remove(CEntity* entity) {
    if (entity == nullptr) {
        return;
    }
    if (entity->bIsBIGBuilding) {
        CPtrNode* node = ms_bigBuildingsList.first;
        while (node != nullptr) {
            if (node->GetItem() == entity) {
                CPtrNode* next = node->GetNext();
                PtrNode_RemoveFromList(node, &ms_bigBuildingsList);
                if (node != nullptr) {
                    PtrNodeDelete(node);
                }
                node = next;
            } else {
                if (node->NextIndex() == CPtrNode::NO_NEXT) {
                    break;
                }
                node = &CPools::ms_pPtrNodes[node->NextIndex()];
            }
        }
    } else {
        entity->CallRemove();
    }
    if (entity->bIsPhysical) {
        Physical_RemoveFromMovingList(entity);
    }
    if (entity->CallHasModelFlag10000()) {
        VisibilityList_Remove(TheVisibilityList, entity);
    }
}

static void ClearScanCodesInList(CPtrList& list) {
    for (CPtrNode* node = list.first; node != nullptr;) {
        node->GetItem()->scanCode = 0;
        if (node->NextIndex() == CPtrNode::NO_NEXT) {
            break;
        }
        node = &CPools::ms_pPtrNodes[node->NextIndex()];
    }
}

void __cdecl ClearScanCodes() {
    for (int i = 0; i < NUM_SECTORS_X * NUM_SECTORS_Y; i++) {
        CSector& sector = ms_aSectors[i];
        ClearScanCodesInList(sector.lists[SECTOR_BUILDINGS]);
        ClearScanCodesInList(sector.lists[SECTOR_VEHICLES]);
        ClearScanCodesInList(sector.lists[SECTOR_PEDS]);
        ClearScanCodesInList(sector.lists[SECTOR_OBJECTS]);
        ClearScanCodesInList(sector.lists[SECTOR_DUMMIES]);
    }
}

uint8_t __cdecl IsInWorld(float x, float y) {
    if (!(x > -900.0) || !(x < 900.0)) {
        return 0;
    }
    return y > -900.0 && y < 900.0;
}

void __cdecl ClearExtraEntities() {
    ms_numExtraEntities = 0;
}

void __cdecl AddExtraEntity(CEntity* entity) {
    if (ms_numExtraEntities < 7) {
        ms_aExtraEntities[ms_numExtraEntities] = entity;
        ms_numExtraEntities = ms_numExtraEntities + 1;
    }
}

CEntity* __cdecl FindNearestObjectOfTypeSectorList(int model, const CVector* pos, CPtrList* list, CEntity* found, float* nearest) {
    for (CPtrNode* node = list->first; node != nullptr;) {
        CEntity* entity = node->GetItem();
        node = node->GetNext();
        if (entity->modelIndex == model) {
            CVector diff;
            VectorSub(&diff, &entity->GetPosition(), pos);
            float distance = Magnitude(diff);
            if (distance < *nearest) {
                *nearest = distance;
                found = entity;
            }
        }
    }
    return found;
}

CEntity* __cdecl FindNearestObjectOfType(int model, float range, const CVector* pos) {
    int x1 = SectorLow(pos->x - range);
    int y1 = SectorLow(pos->y - range);
    int x2 = SectorHigh(pos->x + range);
    int y2 = SectorHigh(pos->y + range);
    CEntity* found = nullptr;
    float nearest = 4.0f;  // (not the range: only what's within 4 units)
    AdvanceCurrentScanCode();
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            CSector* sector = GetSector(x, y);
            found = FindNearestObjectOfTypeSectorList_(model, pos, &sector->lists[SECTOR_BUILDINGS], found, &nearest);
            found = FindNearestObjectOfTypeSectorList_(model, pos, &sector->lists[SECTOR_OBJECTS], found, &nearest);
        }
    }
    return found;
}

void __cdecl SetNearestObjectOfTypeFlags(int model, const CVector* pos, int hide, int enable) {
    CEntity* entity = FindNearestObjectOfType_(model, ms_fNearestObjectRange, pos);
    if (entity == nullptr) {
        return;
    }
    uint32_t on = uint8_t(enable);
    entity->bUsesCollision = on;
    entity->flag3C = on;
    entity->bDontRender = uint8_t(hide);
    if (entity->CallHasModelFlag10000()) {
        if (re::Flag(enable)) {
            VisibilityList_Add(TheVisibilityList, entity);
        } else {
            VisibilityList_Remove(TheVisibilityList, entity);
        }
    }
}

void __cdecl FindObjectsOfTypeInRangeSectorList(int model, CPtrList* list, const CVector*, float, int16_t* count, int16_t maxCount,
                                                re::Ptr32<CEntity>* out) {
    uint16_t scanCode = ms_nCurrentScanCode;
    for (CPtrNode* node = list->first; node != nullptr;) {
        CEntity* entity = node->GetItem();
        node = node->GetNext();
        if (entity->scanCode == scanCode) {
            continue;
        }
        entity->scanCode = scanCode;
        bool matches;
        if (model == -2) {
            matches = entity->CallVf31() || model == entity->modelIndex;
        } else {
            matches = model == -1 || model == entity->modelIndex;
        }
        if (matches && *count < maxCount) {
            if (out != nullptr) {
                out[*count] = entity;
            }
            *count = *count + 1;
        }
    }
}

void __cdecl FindObjectsInRangeSectorList(CPtrList* list, const CVector* pos, float range, int twoD, int16_t* count, int16_t maxCount,
                                          re::Ptr32<CEntity>* out) {
    uint16_t scanCode = ms_nCurrentScanCode;
    for (CPtrNode* node = list->first; node != nullptr;) {
        CEntity* entity = node->GetItem();
        node = node->GetNext();
        if (entity->scanCode == scanCode) {
            continue;
        }
        entity->scanCode = scanCode;
        CVector diff;
        VectorSub(&diff, pos, &entity->GetPosition());
        float distance = re::Flag(twoD) ? Magnitude2D(diff) : Magnitude(diff);
        // (the game gets the radius twice, the first thrown away)
        if (Entity_GetBoundRadius(entity) + range > distance && *count < maxCount) {
            if (out != nullptr) {
                out[*count] = entity;
            }
            *count = *count + 1;
        }
    }
}

void __cdecl FindObjectsOfTypeInRange(int model, const CVector* pos, float range, int16_t* count, int16_t maxCount, re::Ptr32<CEntity>* out,
                                      uint32_t lists) {
    if (model == -1 && (lists & 0x400) != 0) {
        model = -2;
    }
    int x1 = SectorLow(pos->x - range);
    int y1 = SectorLow(pos->y - range);
    int x2 = SectorHigh(pos->x + range);
    int y2 = SectorHigh(pos->y + range);
    AdvanceCurrentScanCode();
    *count = 0;
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            CSector* sector = GetSector(x, y);
            if (lists & 1) {
                FindObjectsOfTypeInRangeSectorList_(model, &sector->lists[SECTOR_BUILDINGS], pos, range, count, maxCount, out);
            }
            if (lists & 2) {
                FindObjectsOfTypeInRangeSectorList_(model, &sector->lists[SECTOR_VEHICLES], pos, range, count, maxCount, out);
            }
            if (lists & 4) {
                FindObjectsOfTypeInRangeSectorList_(model, &sector->lists[SECTOR_PEDS], pos, range, count, maxCount, out);
            }
            if (lists & 8) {
                FindObjectsOfTypeInRangeSectorList_(model, &sector->lists[SECTOR_OBJECTS], pos, range, count, maxCount, out);
            }
            if (lists & 0x10) {
                FindObjectsOfTypeInRangeSectorList_(model, &sector->lists[SECTOR_DUMMIES], pos, range, count, maxCount, out);
            }
        }
    }
}

void __cdecl FindObjectsInRange(const CVector* pos, float range, int twoD, int16_t* count, int16_t maxCount, re::Ptr32<CEntity>* out,
                                uint32_t lists) {
    int x1 = SectorLow(pos->x - range);
    int y1 = SectorLow(pos->y - range);
    int x2 = SectorHigh(pos->x + range);
    int y2 = SectorHigh(pos->y + range);
    AdvanceCurrentScanCode();
    *count = 0;
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            CSector* sector = GetSector(x, y);
            if (lists & 1) {
                FindObjectsInRangeSectorList_(&sector->lists[SECTOR_BUILDINGS], pos, range, twoD, count, maxCount, out);
            }
            if (lists & 2) {
                FindObjectsInRangeSectorList_(&sector->lists[SECTOR_VEHICLES], pos, range, twoD, count, maxCount, out);
            }
            if (lists & 4) {
                FindObjectsInRangeSectorList_(&sector->lists[SECTOR_PEDS], pos, range, twoD, count, maxCount, out);
            }
            if (lists & 8) {
                FindObjectsInRangeSectorList_(&sector->lists[SECTOR_OBJECTS], pos, range, twoD, count, maxCount, out);
            }
            if (lists & 0x10) {
                FindObjectsInRangeSectorList_(&sector->lists[SECTOR_DUMMIES], pos, range, twoD, count, maxCount, out);
            }
        }
    }
}

void __cdecl ClearScanCodes2() {
    for (int i = 0; i < NUM_SECTORS_X * NUM_SECTORS_Y; i++) {
        CSector& sector = ms_aSectors[i];
        const int order[5] = {SECTOR_BUILDINGS, SECTOR_VEHICLES, SECTOR_PEDS, SECTOR_OBJECTS, SECTOR_DUMMIES};
        for (int list : order) {
            for (CPtrNode* node = sector.lists[list].first; node != nullptr;) {
                node->GetItem()->scanCode2 = 0;
                if (node->NextIndex() == CPtrNode::NO_NEXT) {
                    break;
                }
                node = &CPools::ms_pPtrNodes[node->NextIndex()];
            }
        }
    }
}

void __cdecl RemoveFromCollisionRecords(CEntity* entity) {
    CPool<CPed>* peds = CPools::ms_pPedPool;
    for (int slot = peds->size; slot != 0;) {
        slot--;
        if (!peds->IsFreeSlot(slot)) {
            CEntity* ped = reinterpret_cast<CEntity*>(re::PtrCast<uint8_t*>(peds->objects) + peds->elemSize * slot);
            if (ped != nullptr && ped != entity) {
                Physical_RemoveCollisionRecord(ped, entity);
            }
        }
    }
    CPool<CVehicle>* vehicles = CPools::ms_pVehiclePool;
    for (int slot = vehicles->size; slot != 0;) {
        slot--;
        if (!vehicles->IsFreeSlot(slot)) {
            CEntity* vehicle = reinterpret_cast<CEntity*>(re::PtrCast<uint8_t*>(vehicles->objects) + vehicles->elemSize * slot);
            if (vehicle != nullptr && vehicle != entity) {
                Physical_RemoveCollisionRecord(vehicle, entity);
                reinterpret_cast<void(__thiscall*)(CEntity*, CEntity*)>(static_cast<void*>(vehicle->vtable[48]))(vehicle, entity);
            }
        }
    }
    // the objects, and the pools chained to theirs
    CPool<uint8_t>* objects = re::PtrCast<CPool<uint8_t>*>(CPools::ms_pObjectPool);
    int slot = objects->size;
    if (objects->Next() != nullptr) {
        slot += Pool_GetChainSize(objects->Next());
    }
    while (slot != 0) {
        slot--;
        void* object = nullptr;
        if (slot < objects->size) {
            object = objects->objects + objects->elemSize * slot;
        } else if (objects->Next() != nullptr) {
            object = Pool_GetChainedAt(objects->Next(), slot - objects->size, 1);
        }
        // (the flag is read from the object pool's own array, past its end for the chained slots)
        if ((objects->flags[slot] & 0x80) == 0 && object != nullptr && object != entity) {
            Physical_RemoveCollisionRecord(static_cast<CEntity*>(object), entity);
        }
    }
}

void __cdecl SetAllVehiclesFlag33A(int value) {
    CPool<CVehicle>* vehicles = CPools::ms_pVehiclePool;
    int size = vehicles->size;
    for (int slot = 0; slot < size; slot++) {
        if (!vehicles->IsFreeSlot(slot)) {
            uint8_t* vehicle = re::PtrCast<uint8_t*>(vehicles->objects) + vehicles->elemSize * slot;
            if (vehicle != nullptr) {
                vehicle[0x33A] ^= (uint8_t(value << 6) ^ vehicle[0x33A]) & 0x40;
            }
        }
    }
}

uint8_t __cdecl HasModelFlag80000(CEntity* entity) {
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[entity->modelIndex];
    if (mi->type != 1) {
        return 0;
    }
    return uint8_t((mi->flags >> 19) & 1);
}

} // namespace CWorld

CColPoint* CColPoint::Assign(const CColPoint* other) {
    point = other->point;
    CEntity* newEntity = other->entity;
    if (newEntity != entity) {
        if (entity != nullptr) {
            CleanUpOldReference_(entity, &entity);
        }
        entity = newEntity;
        if (newEntity != nullptr) {
            RegisterReference_(newEntity, &entity);
        }
    }
    normal = other->normal;
    piece = other->piece;
    surfaceA = other->surfaceA;
    surfaceB = other->surfaceB;
    depth = other->depth;
    return this;
}

void CColPoint::SetEntity(CEntity* newEntity) {
    if (newEntity == entity) {
        return;
    }
    if (entity != nullptr) {
        CleanUpOldReference_(entity, &entity);
    }
    entity = newEntity;
    if (newEntity != nullptr) {
        RegisterReference_(newEntity, &entity);
    }
}

WORLD_REVERSED("CColPoint::operator=", 0x0046AA70, &CColPoint::Assign);
WORLD_REVERSED("CColPoint::SetEntity", 0x0045F1F0, &CColPoint::SetEntity);
WORLD_REVERSED("CWorld::ClearScanCodes2", 0x0045D9C0, &CWorld::ClearScanCodes2);
WORLD_REVERSED("CWorld::RemoveFromCollisionRecords", 0x0045E690, &CWorld::RemoveFromCollisionRecords);
WORLD_REVERSED("CWorld::SetAllVehiclesFlag33A", 0x0045EE40, &CWorld::SetAllVehiclesFlag33A);
WORLD_REVERSED("CWorld::HasModelFlag80000", 0x0045EE90, &CWorld::HasModelFlag80000);
WORLD_REVERSED("CWorld::Add", 0x0045D6B0, &CWorld::Add);
WORLD_REVERSED("CWorld::Remove", 0x0045DD70, &CWorld::Remove);
WORLD_REVERSED("CWorld::ClearScanCodes", 0x0045D840, &CWorld::ClearScanCodes);
WORLD_REVERSED("CWorld::IsInWorld", 0x0045D450, &CWorld::IsInWorld);
WORLD_REVERSED("CWorld::ClearExtraEntities", 0x0045D5D0, &CWorld::ClearExtraEntities);
WORLD_REVERSED("CWorld::AddExtraEntity", 0x0045D5E0, &CWorld::AddExtraEntity);
WORLD_REVERSED("CWorld::FindNearestObjectOfTypeSectorList", 0x0045D750, &CWorld::FindNearestObjectOfTypeSectorList);
WORLD_REVERSED("CWorld::FindNearestObjectOfType", 0x0045DE60, &CWorld::FindNearestObjectOfType);
WORLD_REVERSED("CWorld::SetNearestObjectOfTypeFlags", 0x0045E060, &CWorld::SetNearestObjectOfTypeFlags);
WORLD_REVERSED("CWorld::FindObjectsOfTypeInRangeSectorList", 0x0045DB40, &CWorld::FindObjectsOfTypeInRangeSectorList);
WORLD_REVERSED("CWorld::FindObjectsInRangeSectorList", 0x0045DC00, &CWorld::FindObjectsInRangeSectorList);
WORLD_REVERSED("CWorld::FindObjectsOfTypeInRange", 0x0045E0D0, &CWorld::FindObjectsOfTypeInRange);
WORLD_REVERSED("CWorld::FindObjectsInRange", 0x0045E3C0, &CWorld::FindObjectsInRange);

CVector* __cdecl MultiplyMatrixVector(CVector* out, const col::CMatrix* m, const CVector* v) {
    const float* f = m->m;
    out->x = f[4] * v->y + f[0] * v->x + f[8] * v->z + f[12];
    out->y = f[1] * v->x + f[5] * v->y + f[9] * v->z + f[13];
    out->z = f[2] * v->x + f[6] * v->y + f[10] * v->z + f[14];
    return out;
}

WORLD_REVERSED("operator*(CMatrix, CVector)", 0x0045D4A0, &MultiplyMatrixVector);
