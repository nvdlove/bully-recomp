#include "game/Entity.h"

#include <cmath>

#include "game/ModelInfo.h"
#include "game/Pools.h"

#include "game/Crt.h"
#include "reversed.h"
using col::CMatrix;
using col::CRect;
using col::CVector;

namespace {
// the math
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* MultiplyPoint(CVector* out, const CMatrix* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
GAME_FN(void, __cdecl, TransformPoint, 0x0046D850, CVector* out, const CSimpleTransform* t, const CVector* v);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, CMatrix* m, float heading);
GAME_FN(void, __thiscall, Matrix_SetUnity, 0x00412770, CMatrix* m);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412720, CMatrix* m, const CMatrix* from);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, CMatrix* m);
GAME_FN(CSimpleTransform*, __thiscall, SimpleTransform_FromMatrix, 0x00435C20, CSimpleTransform* t, const CMatrix* m);
GAME_FN(CRect*, __thiscall, Rect_ExtendTo, 0x004144B0, CRect* rect, float x, float y);
GAME_FN(CMatrix*, __thiscall, Placeable_GetMatrix, 0x0046DD20, CPlaceable* placeable);
GAME_FN(void, __thiscall, Placeable_RemoveMatrix, 0x0046DC00, CPlaceable* placeable);
GAME_FN(void, __thiscall, Placeable_Destruct, 0x0046DD00, CPlaceable* placeable);
GAME_FN(void, __thiscall, Placeable_AllocateMatrix, 0x0046DDF0, CPlaceable* placeable);
GAME_FN(CPlaceable*, __thiscall, Placeable_Construct, 0x0046D9B0, CPlaceable* placeable);
GAME_FN(CMatrixLink*, __thiscall, MatrixList_AddToList1, 0x0046D6E0, CMatrixLinkList* list);
GAME_FN(CMatrixLink*, __thiscall, MatrixList_AddToList2, 0x0046D720, CMatrixLinkList* list);
GAME_FN(void, __thiscall, SimpleTransform_UpdateMatrix, 0x0046DCA0, const CSimpleTransform* t, CMatrix* m);
// the memory manager
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// the pools
GAME_FN(void*, __cdecl, PtrNodeNew, 0x0044E030, size_t size);
GAME_FN(void, __cdecl, PtrNodeDelete, 0x0044E040, void* node);
GAME_FN(void*, __cdecl, EntryInfoNodeNew, 0x00429800, size_t size);
GAME_FN(void, __cdecl, EntryInfoNodeDelete, 0x00429810, void* node);
GAME_FN(void, __thiscall, EntryInfoList_Flush, 0x00429840, CEntryInfoList* list);
GAME_FN(void, __cdecl, GetEntityPoolAndSlot, 0x0044C930, CEntity* entity, int32_t* pool, int32_t* slot);
GAME_FN(void, __thiscall, PtrNode_RemoveFromList, 0x0044DF30, CPtrNode* node, CPtrList* list);
// entities
GAME_FN(CColModel*, __cdecl, GetColModel, 0x0051AF50, CEntity* entity);
GAME_FN(void, __thiscall, Entity_Construct, 0x004656A0, CEntity* entity);
GAME_FN(void, __thiscall, Entity_Destruct, 0x00467E60, CEntity* entity);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, CEntity* entity);
GAME_FN(void, __thiscall, Entity_ResolveReferences, 0x00450910, CEntity* entity);
GAME_FN(void, __thiscall, Entity_UpdateRW, 0x00465AB0, CEntity* entity);
GAME_FN(uint8_t, __thiscall, Entity_ComputeHasPreRenderEffects, 0x00465780, CEntity* entity);
GAME_FN(void, __thiscall, Entity_TransformFromObjectSpace, 0x00466200, CEntity* entity, CVector* out, const CVector* offset);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, CEntity* entity);
GAME_FN(void, __thiscall, EntityFlags_Init, 0x00468BC0, uint32_t* flags);
GAME_FN(void, __thiscall, ColModel_SetOwner, 0x0056E770, CColModel* col, void* owner);
GAME_FN(void, __cdecl, ColModel_Unregister, 0x005E8D60, CColModel* col);
GAME_FN(uint8_t, __cdecl, IsSpecialModel, 0x0043F3A0, int model);
GAME_VAR(int32_t, ms_preRenderModel1, 0x00BF44EC);
GAME_VAR(int32_t, ms_preRenderModel2, 0x00BF44E8);
GAME_VAR(int32_t, ms_numBuildingObjects, 0x00C2A8EC);   // buildings with their object made
GAME_VAR(uint32_t, ms_frameCounter, 0x00C1A99C);        // CTimer::m_FrameCounter
// the renderer's lists (on 0xD12AF8)
GAME_FN(void, __thiscall, VisibilityList_AddVisible, 0x005E5480, void* list, CEntity* entity);
GAME_FN(void, __thiscall, VisibilityList_Remove, 0x005E6990, void* list, CEntity* entity);
inline void* const TheVisibilityList = reinterpret_cast<void*>(0x00D12AF8);
// Gamebryo
GAME_FN(void*, __cdecl, Rw_GetFrame, 0x006C7370, void* object);
GAME_FN(void, __cdecl, Rw_DestroyAtomic, 0x006C72C0, void* atomic);
GAME_FN(void, __cdecl, Rw_DestroyClump, 0x006C75C0, void* clump);
GAME_FN(void, __cdecl, Rw_UpdateFrame, 0x006C9AF0, void* frame);
GAME_FN(void, __cdecl, Rw_AtomicPrepareAlpha, 0x00824EC0, void* atomic);
GAME_FN(void, __cdecl, Rw_ForAllAtomics, 0x006C7380, void* clump, void* callback, uintptr_t data);
inline void* const SetAtomicAlphaCallback = reinterpret_cast<void*>(0x00466140);
GAME_FN(void, __thiscall, AnimManager_RemoveObject, 0x006C2C00, void* manager, void* animObject, int keep);
inline void* const TheAnimManager = reinterpret_cast<void*>(0x020C4910);
// prop anims (CPropAnim, entity type 6)
GAME_FN(void, __thiscall, PropAnim_4ED9C0, 0x004ED9C0, CEntity* prop);
GAME_FN(void, __thiscall, PropAnim_4EA570, 0x004EA570, CEntity* prop);
GAME_FN(void, __thiscall, PropAnim_4EDEB0, 0x004EDEB0, CEntity* prop, int);
GAME_FN(void, __thiscall, PropAnim_4EAC30, 0x004EAC30, CEntity* prop, int);
GAME_FN(void, __thiscall, PropAnim_4ED940, 0x004ED940, CEntity* prop);
GAME_FN(void, __thiscall, PropAnim_4EF2A0, 0x004EF2A0, CEntity* prop);
GAME_FN(void, __thiscall, PropAnim_Member128_5F40A0, 0x005F40A0, void* member);
GAME_FN(uint8_t, __thiscall, PropAnim_IsAnimated, 0x004EA360, CEntity* prop);
GAME_FN(void, __thiscall, PropAnim_SetAnimated, 0x004EA370, CEntity* prop, int);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_520430, 0x00520430, CBaseModelInfo* mi);
GAME_FN(void*, __thiscall, PAnimModelInfo_5201D0, 0x005201D0, CBaseModelInfo* mi);
GAME_FN(void, __thiscall, PAnimStore_Release, 0x00450720, void* store, int id);
inline void* const ThePAnimStore = reinterpret_cast<void*>(0x00C0F7B4);
GAME_FN(uint32_t, __thiscall, Object_4CF3C0, 0x004CF3C0, CEntity* object);
GAME_FN(void, __cdecl, Object_445170, 0x00445170, CEntity* object);
GAME_FN(void, __thiscall, Entity_443AA0, 0x00443AA0, CEntity* entity);
GAME_FN(void, __thiscall, Entity_ProcessEffects, 0x00512790, CEntity* entity);
GAME_FN(void*, __thiscall, ModelInfo_GetFirst2dEffect, 0x0050ED70, CBaseModelInfo* mi);
GAME_FN(int, __thiscall, ModelInfo_AddRef, 0x0050EB90, CBaseModelInfo* mi);
GAME_FN(int, __thiscall, ModelInfo_RemoveRef, 0x0050EBC0, CBaseModelInfo* mi);
GAME_FN(void*, __cdecl, Rw_GetLTM, 0x006C9A90, void* frame);
GAME_FN(void, __thiscall, Matrix_UpdateFromRw, 0x00412A70, CMatrix* m, void* rwMatrix);
GAME_FN(int, __thiscall, Clock_GetMilliseconds, 0x00595B30, void* clock);
inline void* const TheClock = reinterpret_cast<void*>(0x00CF3DA0);
// the camera
GAME_FN(int, __thiscall, Camera_IsSphereVisible, 0x004F13D0, void* camera, const CVector* center, float radius);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_FN(uint8_t, __thiscall, Ped_683F40, 0x00683F40, void* manager, CEntity* ped);
inline void* const ThePedManager683 = reinterpret_cast<void*>(0x00B9DAB8);
GAME_FN(float, __cdecl, Fn414490, 0x00414490, float a, float b);   // (a radius and a minimum)
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_VAR(int32_t, ms_bigPedModel, 0x00BF43A4);
// the pointer checks by type (IsEntityPointerValid)
GAME_FN(uint8_t, __cdecl, IsBuildingPointerValid, 0x004CEDB0, CEntity* entity);
GAME_FN(uint8_t, __cdecl, IsVehiclePointerValid, 0x004CE390, CEntity* entity);
GAME_FN(uint8_t, __cdecl, IsPedPointerValid, 0x004800D0, CEntity* entity);
GAME_FN(uint8_t, __cdecl, IsObjectPointerValid, 0x004D0B80, CEntity* entity);
GAME_FN(uint8_t, __cdecl, IsDummyPointerValid, 0x00465450, CEntity* entity);
GAME_FN(int, __cdecl, IsPropAnimPointerValid, 0x004ED800, CEntity* entity);
GAME_FN(uint8_t, __thiscall, IsType7PointerValid, 0x004A77F0, void* manager, CEntity* entity);
inline void* const TheType7Manager = reinterpret_cast<void*>(0x00C2C730);
GAME_VAR(CEntity*, ms_pAlwaysValidEntity, 0x00C2A8E8);
// the references (CEntity::RegisterReference / CleanUpOldReference's bodies)
GAME_FN(void, __thiscall, Entity_RegisterReference, 0x00450880, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __thiscall, Entity_CleanUpOldReference, 0x004508D0, CEntity* entity, re::Ptr32<CEntity>* ref);

} // namespace

#define ENTITY_REVERSED(name, address, fn)                                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

// ---- CSimpleTransform, the matrix list --------------------------------------------------

void CSimpleTransform::UpdateMatrix(CMatrix* m) const {
    Matrix_SetRotateZOnly(m, heading);
    m->m[12] = pos.x;
    m->m[13] = pos.y;
    m->m[14] = pos.z;
}

// 0x46D850 (cdecl): a point from the transform's space
static void __cdecl SimpleTransformPoint(CVector* out, const CSimpleTransform* t, const CVector* v) {
    if (t->heading == 0.0f) {
        out->x = t->pos.x + v->x;
        out->y = t->pos.y + v->y;
        out->z = t->pos.z + v->z;
        return;
    }
    float c = float(std::cos(t->heading));
    float s = float(std::sin(t->heading));
    out->x = c * v->x - s * v->y + t->pos.x;
    out->y = c * v->y + v->x * s + t->pos.y;
    out->z = t->pos.z + v->z;
}

CMatrixLink* CMatrixLinkList::AddToList1() {
    CMatrixLink* link = freeHead.next;
    if (link == &freeTail) {
        return nullptr;
    }
    link->Remove();
    link->Insert(&head);
    return link;
}

CMatrixLink* CMatrixLinkList::AddToList2() {
    CMatrixLink* link = freeHead.next;
    if (link == &freeTail) {
        return nullptr;
    }
    link->Remove();
    link->Insert(&allocatedHead);
    return link;
}

void CSimpleTransform::UpdateNiTransform(float* out) const {
    if (0.0f == heading) {
        out[0] = 1.0f;
        out[4] = 1.0f;
        out[3] = 0.0f;
        out[6] = 0.0f;
        out[1] = 0.0f;
        out[7] = 0.0f;
    } else {
        float c = float(std::cos(heading));
        float s = float(std::sin(heading));
        out[0] = c;
        out[3] = s;
        out[6] = 0.0f;
        out[1] = -s;
        out[4] = c;
        out[7] = 0.0f;
    }
    out[2] = 0.0f;
    out[5] = 0.0f;
    out[8] = 1.0f;
    out[9] = pos.x;
    out[10] = pos.y;
    out[11] = pos.z;
}

ENTITY_REVERSED("CSimpleTransform::UpdateNiTransform", 0x0046D8F0, &CSimpleTransform::UpdateNiTransform);
ENTITY_REVERSED("CSimpleTransform::UpdateMatrix", 0x0046DCA0, &CSimpleTransform::UpdateMatrix);
ENTITY_REVERSED("CSimpleTransform::TransformPoint", 0x0046D850, &SimpleTransformPoint);
ENTITY_REVERSED("CMatrixLinkList::AddToList1", 0x0046D6E0, &CMatrixLinkList::AddToList1);
ENTITY_REVERSED("CMatrixLinkList::AddToList2", 0x0046D720, &CMatrixLinkList::AddToList2);

// ---- CPlaceable --------------------------------------------------------------------------

static re::Ptr32<void>* const CPlaceable_vftable = reinterpret_cast<re::Ptr32<void>*>(0x0090C74C);

CPlaceable* CPlaceable::Construct() {
    placement.pos.x = 0.0f;
    vtable = CPlaceable_vftable;
    placement.pos.y = 0.0f;
    matrix = nullptr;
    placement.pos.z = 0.0f;
    placement.heading = 0.0f;
    return this;
}

void CPlaceable::Destruct() {
    vtable = CPlaceable_vftable;
    if (matrix != nullptr) {
        Placeable_RemoveMatrix(this);
    }
}

CPlaceable* CPlaceable::Destroy(uint8_t flags) {
    vtable = CPlaceable_vftable;
    if (matrix != nullptr) {
        Placeable_RemoveMatrix(this);
    }
    if (flags & 1) {
        Free(this);
    }
    return this;
}

CMatrix* CPlaceable::GetMatrix() {
    if (matrix == nullptr) {
        SimpleTransform_UpdateMatrix(&placement, &gDummyMatrix);
        return &gDummyMatrix;
    }
    return &matrix->m;
}

void CPlaceable::RemoveMatrix() {
    CMatrixLink* link = matrix;
    const float* m = link->m.m;
    placement.pos.x = m[12];
    placement.pos.y = m[13];
    placement.pos.z = m[14];
    placement.heading = float(std::atan2(-m[4], m[5]));  // (GetHeading: from the forward vector)
    matrix = nullptr;
    Matrix_SetUnity(&link->m);
    link->Remove();
    link->Insert(&gMatrixList.freeHead);
}

void CPlaceable::AllocateStaticMatrix() {
    CMatrixLink* link = matrix;
    if (link != nullptr) {
        link->Remove();
        link->Insert(&gMatrixList.allocatedHead);
        return;
    }
    link = gMatrixList.freeHead.next;
    if (link != &gMatrixList.freeTail) {
        link->Remove();
        link->Insert(&gMatrixList.allocatedHead);
    } else {
        // none free: the oldest of the first list's is taken from its owner
        Placeable_RemoveMatrix(gMatrixList.tail.prev->owner);
        link = MatrixList_AddToList2(&gMatrixList);
    }
    link->owner = this;
    matrix = link;
    Matrix_SetUnity(&link->m);
}

void CPlaceable::AllocateMatrix() {
    if (matrix != nullptr) {
        return;
    }
    CMatrixLink* link = gMatrixList.freeHead.next;
    if (link != &gMatrixList.freeTail) {
        link->Remove();
        link->Insert(&gMatrixList.head);
    } else {
        Placeable_RemoveMatrix(gMatrixList.tail.prev->owner);
        link = MatrixList_AddToList1(&gMatrixList);
    }
    link->owner = this;
    matrix = link;
}

void CPlaceable::SetMatrix(const CMatrix* m, int forceMatrix) {
    if (matrix == nullptr) {
        if (1.0f == m->m[10] && !re::Flag(forceMatrix)) {
            // upright: the position and heading are enough
            CSimpleTransform t;
            placement = *SimpleTransform_FromMatrix(&t, m);
            return;
        }
        Placeable_AllocateMatrix(this);
    }
    Matrix_Copy(&matrix->m, m);
    if (m->attached != nullptr) {
        matrix->m.attached = m->attached;
        Matrix_UpdateRW(&matrix->m);
    }
}

uint8_t CPlaceable::IsWithinArea(float x1, float y1, float x2, float y2) {
    const CVector& pos = GetPosition();
    float x = pos.x, y = pos.y;
    if (x2 < x1) {
        float t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y2 < y1) {
        float t = y1;
        y1 = y2;
        y2 = t;
    }
    return x >= x1 && x <= x2 && y >= y1 && y <= y2;
}

uint8_t CPlaceable::IsWithinArea(float x1, float y1, float z1, float x2, float y2, float z2) {
    const CVector& pos = GetPosition();
    float x = pos.x, y = pos.y, z = pos.z;
    if (x2 < x1) {
        float t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y2 < y1) {
        float t = y1;
        y1 = y2;
        y2 = t;
    }
    if (z2 < z1) {
        float t = z1;
        z1 = z2;
        z2 = t;
    }
    return x >= x1 && x <= x2 && y >= y1 && y <= y2 && z >= z1 && z <= z2;
}

ENTITY_REVERSED("CPlaceable::CPlaceable", 0x0046D9B0, &CPlaceable::Construct);
ENTITY_REVERSED("CPlaceable::~CPlaceable", 0x0046DD00, &CPlaceable::Destruct);
ENTITY_REVERSED("CPlaceable::vf0", 0x0046DE60, &CPlaceable::Destroy);
ENTITY_REVERSED("CPlaceable::GetMatrix", 0x0046DD20, &CPlaceable::GetMatrix);
ENTITY_REVERSED("CPlaceable::RemoveMatrix", 0x0046DC00, &CPlaceable::RemoveMatrix);
ENTITY_REVERSED("CPlaceable::AllocateStaticMatrix", 0x0046DD40, &CPlaceable::AllocateStaticMatrix);
ENTITY_REVERSED("CPlaceable::AllocateMatrix", 0x0046DDF0, &CPlaceable::AllocateMatrix);
ENTITY_REVERSED("CPlaceable::SetMatrix", 0x0046DE90, &CPlaceable::SetMatrix);
ENTITY_REVERSED("CPlaceable::IsWithinArea2D", 0x0046D9D0,
                static_cast<uint8_t (CPlaceable::*)(float, float, float, float)>(&CPlaceable::IsWithinArea));
ENTITY_REVERSED("CPlaceable::IsWithinArea3D", 0x0046DAB0,
                static_cast<uint8_t (CPlaceable::*)(float, float, float, float, float, float)>(&CPlaceable::IsWithinArea));

// ---- CEntity ------------------------------------------------------------------------------

static re::Ptr32<void>* const CEntity_vftable = reinterpret_cast<re::Ptr32<void>*>(0x0090BF64);

// 0x468BC0 (thiscall on +0x1C): the flags' defaults
static void __fastcall InitEntityFlags(uint32_t* flags, re::Edx) {
    for (int i = 0; i < 59; i++) {
        flags[i] = 0;
    }
    flags[0] = 1;     // +0x1C
    flags[0xB] = 1;   // +0x48
    flags[0x37] = 1;  // +0xF8
}
ENTITY_REVERSED("CEntity::InitFlags", 0x00468BC0, &InitEntityFlags);

CEntity* CEntity::Construct() {
    Placeable_Construct(this);
    vtable = CEntity_vftable;
    EntityFlags_Init(&flag1C);
    typeAndStatus = 0x20;  // (status 4, no type)
    scanCode = 0;
    scanCode2 = 0;
    modelIndex = -1;
    rwObject = nullptr;
    unknown110 = 0xFF;
    unknown111 = 0;
    animObject = nullptr;
    randomSeed = uint16_t(GameRand());
    unknown11C = 0.0f;
    unknown118 = 0;
    return this;
}

void CEntity::Destruct() {
    vtable = CEntity_vftable;
    if (modelIndex >= 0) {
        CColModel* col = GetColModel(this);
        if (col != nullptr && col->owner == this) {
            ColModel_SetOwner(col, nullptr);
        }
    }
    Entity_DeleteRwObject(this);
    Entity_ResolveReferences(this);
    Placeable_Destruct(this);
}

CEntity* CEntity::Destroy(uint8_t flags) {
    Entity_Destruct(this);  // (inlined in the game)
    if (flags & 1) {
        Free(this);
    }
    return this;
}

// Which list of a sector an entity goes in (the game's switch; type 0 keeps the
// previous iteration's list, uninitialised on the first)
static CPtrList* ListForType(CSector* sector, uint8_t type, CPtrList* previous) {
    switch (type) {
    case ENTITY_TYPE_BUILDING:
        return &sector->lists[SECTOR_BUILDINGS];
    case ENTITY_TYPE_VEHICLE:
        return &sector->lists[SECTOR_VEHICLES];
    case ENTITY_TYPE_PED:
        return &sector->lists[SECTOR_PEDS];
    case ENTITY_TYPE_OBJECT:
    case ENTITY_TYPE_PROPANIM:
    case ENTITY_TYPE_7:
        return &sector->lists[SECTOR_OBJECTS];
    case ENTITY_TYPE_DUMMY:
        return &sector->lists[SECTOR_DUMMIES];
    default:
        return previous;
    }
}

// the sectors a rectangle covers, clamped to the map (as the game does it)
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

static CPtrNode* NewNodeFor(CEntity* entity) {
    CPtrNode* node = static_cast<CPtrNode*>(PtrNodeNew(4));
    if (node != nullptr) {
        int32_t pool, slot;
        GetEntityPoolAndSlot(entity, &pool, &slot);
        node->bits = (uint32_t(slot) | 0xFFFFC000) << 4 | (uint32_t(pool) & 0xF);
    }
    return node;
}

void CEntity::Add() {
    CRect rect;
    CallGetBoundRect(&rect);
    int x1 = SectorLow(rect.left);
    int y1 = SectorLow(rect.bottom);
    int x2 = SectorHigh(rect.right);
    int y2 = SectorHigh(rect.top);
    CPtrList* list = nullptr;
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            list = ListForType(CWorld::GetSector(x, y), GetType(), list);
            CPtrNode* node = NewNodeFor(this);
            list->AddNode(node);
        }
    }
}

void CEntity::Remove() {
    if (GetType() == ENTITY_TYPE_PROPANIM && PropAnim_IsAnimated(this)) {
        PropAnim_SetAnimated(this, 0);
    }
    CColModel* col = GetColModel(this);
    if (col != nullptr) {
        if (col->data != nullptr && col->data->unknown18 != nullptr) {
            ColModel_Unregister(col);
        }
        ColModel_SetOwner(col, nullptr);
    }
    CRect rect;
    CallGetBoundRect(&rect);
    int x1 = SectorLow(rect.left);
    int y1 = SectorLow(rect.bottom);
    int x2 = SectorHigh(rect.right);
    int y2 = SectorHigh(rect.top);
    CPtrList* list = nullptr;
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            list = ListForType(CWorld::GetSector(x, y), GetType(), list);
            // its first node there
            for (CPtrNode* node = list->first; node != nullptr;) {
                if (node->GetItem() == this) {
                    PtrNode_RemoveFromList(node, list);
                    PtrNodeDelete(node);
                    break;
                }
                if (node->NextIndex() == CPtrNode::NO_NEXT) {
                    break;
                }
                node = &CPools::ms_pPtrNodes[node->NextIndex()];
            }
        }
    }
}

void CEntity::SetModelIndex(int16_t model, int arg) {
    if (modelIndex != -1) {
        Entity_DeleteRwObject(this);
    }
    modelIndex = model;
    if (model == -1) {
        bHasPreRenderEffects = 0;
        return;
    }
    bHasPreRenderEffects = Entity_ComputeHasPreRenderEffects(this);
    CallCreateRwObject(1, arg);
}

// (inlined in vf9 as in 0x465780)
static bool HasPreRenderEffects(CEntity* entity) {
    if (entity->flagC4 != 0 || IsSpecialModel(entity->modelIndex)) {
        return true;
    }
    int model = entity->modelIndex;
    if (model == ms_preRenderModel1 || model == ms_preRenderModel2) {
        return true;
    }
    // (an object's byte; only meaningful for objects, read for every type)
    return (reinterpret_cast<const uint8_t*>(entity)[0x209] & 1) != 0;
}

void CEntity::SetModelIndexNoCreate(int16_t model) {
    modelIndex = model;
    bHasPreRenderEffects = HasPreRenderEffects(this) ? 1 : 0;
}

uint8_t CEntity::ComputeHasPreRenderEffects() {
    return HasPreRenderEffects(this) ? 1 : 0;
}

CRect* CEntity::GetBoundRect(CRect* rect) {
    rect->left = 1000000.0f;
    rect->bottom = 1000000.0f;
    rect->right = -1000000.0f;
    rect->top = -1000000.0f;
    CVector boxMin = GetColModel(this)->boxMin;
    CVector boxMax = GetColModel(this)->boxMax;
    // the corners through the heading only (the matrix's position and heading)
    CVector corner;
    if (matrix != nullptr) {
        for (int i = 0; i < 2; i++) {
            const float* m = Placeable_GetMatrix(this)->m;
            CSimpleTransform t;
            t.pos.x = m[12];
            t.pos.y = m[13];
            t.pos.z = m[14];
            t.heading = float(std::atan2(-m[4], m[5]));
            TransformPoint(&corner, &t, i == 0 ? &boxMin : &boxMax);
            Rect_ExtendTo(rect, corner.x, corner.y);
        }
    } else {
        TransformPoint(&corner, &placement, &boxMin);
        Rect_ExtendTo(rect, corner.x, corner.y);
        TransformPoint(&corner, &placement, &boxMax);
        Rect_ExtendTo(rect, corner.x, corner.y);
    }
    // the other two corners
    float x = boxMin.x;
    boxMin.x = boxMax.x;
    boxMax.x = x;
    if (matrix != nullptr) {
        for (int i = 0; i < 2; i++) {
            const float* m = Placeable_GetMatrix(this)->m;
            CSimpleTransform t;
            t.pos.x = m[12];
            t.pos.y = m[13];
            t.pos.z = m[14];
            t.heading = float(std::atan2(-m[4], m[5]));
            TransformPoint(&corner, &t, i == 0 ? &boxMin : &boxMax);
            Rect_ExtendTo(rect, corner.x, corner.y);
        }
    } else {
        TransformPoint(&corner, &placement, &boxMin);
        Rect_ExtendTo(rect, corner.x, corner.y);
        TransformPoint(&corner, &placement, &boxMax);
        Rect_ExtendTo(rect, corner.x, corner.y);
    }
    return rect;
}

uint8_t CEntity::Vf22() {
    if (flagA4 != 0 && flagE4 == 0) {
        return 0;
    }
    return 1;
}

float CEntity::GetBoundCentreVirtual(CVector* out) {
    // (vf25 gets `out` as its second argument; 0x41A950 is CVector::Distance, its float
    // result (st0) the game's return value: the callers fstp it)
    CVector tmp;
    auto vf25 = reinterpret_cast<CVector*(__thiscall*)(CEntity*, CVector*, CVector*)>(static_cast<void*>(vtable[25]));
    CVector* result = vf25(this, &tmp, out);
    return reinterpret_cast<float(__cdecl*)(CVector*, CVector*)>(0x0041A950)(out, result);
}

float CEntity::GetBoundDiameter() {
    CColModel* col = GetColModel(this);
    return col->sphereRadius + col->sphereRadius;
}

uint8_t CEntity::HasModelFlag10000() {
    if (GetType() == ENTITY_TYPE_DUMMY) {
        return 0;
    }
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[modelIndex];
    return mi != nullptr && (mi->flags & 0x10000) != 0;
}

void CEntity::DeleteRwObject() {
    if (matrix != nullptr) {
        Placeable_GetMatrix(this)->attached = nullptr;  // (GetMatrix's +0x14 read, 0x8B6120)
    }
    uint8_t* object = static_cast<uint8_t*>(rwObject);
    if (object != nullptr) {
        CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[modelIndex];
        if (object[0] == 1) {
            Rw_GetFrame(object);
            if (mi->type != 1 || (mi->flags & 0x100) == 0) {
                Rw_DestroyAtomic(object);
            }
        } else if (object[0] == 2) {
            if (mi->type == 0xA) {
                PropAnim_4ED9C0(this);
                PropAnim_4EA570(this);
            }
            uint8_t* anim = static_cast<uint8_t*>(animObject);
            if (anim != nullptr && *reinterpret_cast<re::Ptr32<void>*>(anim + 0x10) != nullptr) {
                AnimManager_RemoveObject(TheAnimManager, anim, bAlwaysOnScreen != 0 ? 1 : 0);
            } else {
                Rw_DestroyClump(object);
            }
        }
        rwObject = nullptr;
        ModelInfo_RemoveRef(CModelInfo::ms_modelInfoPtrs[modelIndex]);
        if (GetType() == ENTITY_TYPE_BUILDING) {
            ms_numBuildingObjects = ms_numBuildingObjects - 1;
        }
        if (CallHasModelFlag10000()) {
            VisibilityList_Remove(TheVisibilityList, this);
        }
    }
    // prop anims: their animation's reference
    if (uint16_t(modelIndex) == 0xFFFF) {
        return;
    }
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[modelIndex];
    uint8_t* self = reinterpret_cast<uint8_t*>(this);
    if (mi->type != 0xA || GetType() != ENTITY_TYPE_PROPANIM || *reinterpret_cast<uint32_t*>(self + 0x228) == 0) {
        return;
    }
    uint16_t anim = *reinterpret_cast<uint16_t*>(self + 0x22C);
    if (anim != 0xFFFF && anim == uint16_t(mi->GetPAnimIndex())) {
        PropAnim_4EDEB0(this, 0);
        PropAnim_4EAC30(this, 1);
        PropAnim_4ED940(this);
        PropAnim_Member128_5F40A0(self + 0x128);
        PropAnim_4EF2A0(this);
        if (!PAnimModelInfo_520430(mi)) {
            PAnimStore_Release(ThePAnimStore, int16_t(anim));
        }
    } else {
        PropAnim_4EDEB0(this, 1);
        PropAnim_4EAC30(this, 1);
    }
}

void CEntity::AttachToRwObject(void* object) {
    rwObject = object;
    if (object == nullptr) {
        return;
    }
    Entity_UpdateRW(this);
    ModelInfo_AddRef(CModelInfo::ms_modelInfoPtrs[modelIndex]);
    if (CallHasModelFlag10000() && bUsesCollision != 0) {
        VisibilityList_AddVisible(TheVisibilityList, this);
    }
}

void CEntity::DetachFromRwObject() {
    if (rwObject != nullptr) {
        ModelInfo_RemoveRef(CModelInfo::ms_modelInfoPtrs[modelIndex]);
    }
    rwObject = nullptr;
    if (matrix != nullptr) {
        Placeable_GetMatrix(this)->attached = nullptr;
    }
    if (CallHasModelFlag10000()) {
        VisibilityList_Remove(TheVisibilityList, this);
    }
}

void CEntity::UpdateRwFrame() {
    if (rwObject != nullptr) {
        void* frame = static_cast<re::Ptr32<void>*>(rwObject)[1];
        if (frame != nullptr) {
            Rw_UpdateFrame(frame);
        }
    }
}

void CEntity::TransformFromObjectSpace(CVector* out, const CVector* offset) {
    if (matrix != nullptr) {
        // (0x41A9A0 writes 16 bytes: a CVector and the pad after it)
        struct { CVector v; float pad; } result;
        MultiplyPoint(&result.v, &matrix->m, offset);
        *out = result.v;
        return;
    }
    TransformPoint(out, &placement, offset);
}

CVector* CEntity::GetBoundCentre(CVector* out) {
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    Entity_TransformFromObjectSpace(this, out, &GetColModel(this)->sphereCenter);
    return out;
}

float CEntity::GetBoundRadius() {
    return GetColModel(this)->sphereRadius;
}

float CEntity::GetDistanceFromCentreOfMassToBaseOfModel() {
    return -GetColModel(this)->boxMin.z;
}

// the bounding sphere's center, in the world
static CVector WorldSphereCenter(CEntity* entity, CColModel* col) {
    // (0x41A9A0 writes 16 bytes: a CVector and the pad after it)
    struct { CVector v; float pad; } center;
    MultiplyPoint(&center.v, Placeable_GetMatrix(entity), &col->sphereCenter);
    return center.v;
}

uint8_t CEntity::GetIsTouching(const CVector* center, float radius) {
    CColModel* col = GetColModel(this);
    CVector c = WorldSphereCenter(this, col);
    float dx = c.x - center->x, dy = c.y - center->y, dz = c.z - center->z;
    float sum = col->sphereRadius + radius;
    float distanceSq = dx * dx + dy * dy + dz * dz;
    return distanceSq < sum * sum;
}

uint8_t CEntity::GetIsTouching(const CVector* center, float radius, float* distance) {
    CColModel* col = GetColModel(this);
    CVector c = WorldSphereCenter(this, col);
    float dx = c.x - center->x, dy = c.y - center->y, dz = c.z - center->z;
    float d = float(std::sqrt(std::fabs(dy * dy + dx * dx + dz * dz)));
    *distance = d;
    float sum = col->sphereRadius + radius;
    // (the distance, not its square, against the squared sum)
    return sum * sum > d;
}

uint8_t CEntity::GetIsTouchingMoving(const CVector* center, float radius) {
    CColModel* col = GetColModel(this);
    CVector c = WorldSphereCenter(this, col);
    float dx = c.x - center->x, dy = c.y - center->y, dz = c.z - center->z;
    float sum = col->sphereRadius + radius;
    if (bIsPhysical) {
        const CVector& speed = static_cast<CPhysical*>(this)->moveSpeed;
        sum = std::fabs(speed.y) + std::fabs(speed.x) + std::fabs(speed.z) + sum;
    }
    float distanceSq = dx * dx + dy * dy + dz * dz;
    return distanceSq < sum * sum;
}

uint8_t CEntity::GetIsTouchingBoundCentre(const CVector* center, float radius) {
    CVector c{0.0f, 0.0f, 0.0f};
    Entity_TransformFromObjectSpace(this, &c, &GetColModel(this)->sphereCenter);
    float sum = GetColModel(this)->sphereRadius + radius;
    float dx = c.x - center->x, dy = c.y - center->y, dz = c.z - center->z;
    float distanceSq = dx * dx + dy * dy + dz * dz;
    return distanceSq < sum * sum;
}

uint8_t CEntity::GetIsOnScreen() {
    if (bAlwaysOnScreen) {
        return 1;
    }
    CColModel* col = GetColModel(this);
    CVector center;
    Entity_TransformFromObjectSpace(this, &center, &col->sphereCenter);
    float radius = col->sphereRadius;
    if (GetType() == ENTITY_TYPE_PED) {
        if (Ped_683F40(ThePedManager683, this)) {
            return 1;
        }
        uint8_t* self = reinterpret_cast<uint8_t*>(this);
        CEntity* other = *reinterpret_cast<re::Ptr32<CEntity>*>(self + 0x1D0);
        if (other->modelIndex == ms_bigPedModel && *reinterpret_cast<uint32_t*>(self + 0x1410) != 0) {
            radius = Fn414490(radius, 4.1509318f);  // (0x90C068)
        }
    }
    return Camera_IsSphereVisible(TheCamera, &center, radius) != 0;
}

uint8_t CEntity::GetIsOnScreenLarge() {
    if (bAlwaysOnScreen) {
        return 1;
    }
    CColModel* col = GetColModel(this);
    CVector center;
    Entity_TransformFromObjectSpace(this, &center, &col->sphereCenter);
    float radius = float(col->sphereRadius * 1.5);
    return Camera_IsSphereVisible(TheCamera, &center, radius) != 0;
}

uint8_t CEntity::IsVisibleAndOnScreen() {
    if (rwObject != nullptr && flag48 != 0) {
        return Entity_GetIsOnScreen(this);
    }
    return 0;
}

uint8_t CEntity::IsFrameForRandomUpdate(uint32_t frames) {
    if (int(frames) <= 1) {
        return 1;
    }
    uint32_t n = uint32_t(randomSeed) + ms_frameCounter;
    n = n % 0xFFFF + 1;
    return (n & (0u - n)) >= frames;
}

void CEntity::SetRwObjectAlpha(int alpha) {
    uint8_t* object = static_cast<uint8_t*>(rwObject);
    if (object == nullptr) {
        return;
    }
    if (object[0] == 1) {
        Rw_AtomicPrepareAlpha(object);
        *reinterpret_cast<int*>(object + 0x1C) = alpha;
    } else if (object[0] == 2) {
        Rw_ForAllAtomics(object, SetAtomicAlphaCallback, alpha);
        *reinterpret_cast<int*>(object + 0x10) = int(float(alpha));
    }
}

uint32_t CEntity::GetObjectValue() {
    uint8_t type = GetType();
    if (type == ENTITY_TYPE_OBJECT) {
        return Object_4CF3C0(this);
    }
    if (type == ENTITY_TYPE_PROPANIM) {
        uint8_t* data = static_cast<uint8_t*>(PAnimModelInfo_5201D0(CModelInfo::ms_modelInfoPtrs[modelIndex]));
        return *reinterpret_cast<uint32_t*>(data + 0x80);
    }
    return 0;
}

uint32_t CEntity::GetIsStatic() {
    if (bIsStatic == 0 && bIsStaticWaitingForCollision == 0) {
        return 0;
    }
    return 1;
}

// the object's frame's matrix (nullptr: none)
static void* FrameLTM(CEntity* entity) {
    if (entity->rwObject == nullptr) {
        return nullptr;
    }
    void* frame = static_cast<re::Ptr32<void>*>(entity->rwObject)[1];
    return frame != nullptr ? Rw_GetLTM(frame) : nullptr;
}

void CEntity::UpdateFromRwFrame() {
    if (rwObject == nullptr || static_cast<re::Ptr32<void>*>(rwObject)[1] == nullptr || Rw_GetLTM(static_cast<re::Ptr32<void>*>(rwObject)[1]) == nullptr) {
        return;
    }
    if (matrix != nullptr) {
        if (matrix->m.attached == nullptr) {
            void* ltm = FrameLTM(this);
            matrix->m.attached = ltm;
            Matrix_UpdateFromRw(&matrix->m, ltm);
        } else {
            Matrix_UpdateFromRw(&matrix->m, matrix->m.attached);
        }
        return;
    }
    CMatrix m;
    void* ltm = FrameLTM(this);
    m.attached = ltm;
    Matrix_UpdateFromRw(&m, ltm);
    placement.heading = float(std::atan2(-m.m[4], m.m[5]));
    placement.pos.x = m.m[12];
    placement.pos.y = m.m[13];
    placement.pos.z = m.m[14];
}

void CEntity::PreRender() {
    if (uint16_t(modelIndex) == 0xFFFF) {
        return;
    }
    if (ModelInfo_GetFirst2dEffect(CModelInfo::ms_modelInfoPtrs[modelIndex]) != nullptr) {
        Entity_ProcessEffects(this);
    }
    if (bHasPreRenderEffects != 0 && GetType() == ENTITY_TYPE_OBJECT && (reinterpret_cast<uint8_t*>(this)[0x209] & 1) != 0) {
        Object_445170(this);
        Entity_443AA0(this);
        if (rwObject != nullptr) {
            void* frame = static_cast<re::Ptr32<void>*>(rwObject)[1];
            if (frame != nullptr) {
                Rw_UpdateFrame(frame);
            }
        }
    }
}

void CEntity::SetupBigBuilding() {
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[modelIndex];
    bIsBIGBuilding = 1;
    flag74 = 1;
    bUsesCollision = 0;
    float drawDistance = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(mi) + 0x24);
    if (drawDistance <= 2000.0) {
        flag5C = 1;
    }
}

void CEntity::GetBoundCentreNoClear(CVector* out) {
    Entity_TransformFromObjectSpace(this, out, &GetColModel(this)->sphereCenter);
}

float __cdecl GetClockSeconds() {
    int ms = Clock_GetMilliseconds(TheClock);
    double f = double(ms);
    if (ms < 0) {
        f += 4294967296.0;  // (unsigned)
    }
    return float(f / 1000.0);
}

float __fastcall DotProduct(const CVector* a, re::Edx, const CVector* b) {
    return b->y * a->y + b->x * a->x + b->z * a->z;
}

uint8_t __cdecl IsEntityPointerValid(CEntity* entity) {
    if (entity == nullptr) {
        return 0;
    }
    if (entity == ms_pAlwaysValidEntity) {
        return 1;
    }
    switch (entity->GetType()) {
    case ENTITY_TYPE_BUILDING:
        return IsBuildingPointerValid(entity);
    case ENTITY_TYPE_VEHICLE:
        return IsVehiclePointerValid(entity);
    case ENTITY_TYPE_PED:
        return IsPedPointerValid(entity);
    case ENTITY_TYPE_OBJECT:
        return IsObjectPointerValid(entity);
    case ENTITY_TYPE_DUMMY:
        return IsDummyPointerValid(entity);
    case ENTITY_TYPE_PROPANIM:
        return IsPropAnimPointerValid(entity) != 0;
    case ENTITY_TYPE_7:
        return IsType7PointerValid(TheType7Manager, entity);
    default:
        return 0;
    }
}

void __cdecl RegisterReference(CEntity* entity, re::Ptr32<CEntity>* ref) {
    Entity_RegisterReference(entity, ref);
}

void __cdecl CleanUpOldReference(CEntity* entity, re::Ptr32<CEntity>* ref) {
    Entity_CleanUpOldReference(entity, ref);
}

ENTITY_REVERSED("CEntity::CEntity", 0x004656A0, &CEntity::Construct);
ENTITY_REVERSED("CEntity::~CEntity", 0x00467E60, &CEntity::Destruct);
ENTITY_REVERSED("CEntity::vf0", 0x004686E0, &CEntity::Destroy);
ENTITY_REVERSED("CEntity::Add", 0x00465EB0, &CEntity::Add);
ENTITY_REVERSED("CEntity::Remove", 0x004678F0, &CEntity::Remove);
ENTITY_REVERSED("CEntity::SetModelIndex", 0x00468660, &CEntity::SetModelIndex);
ENTITY_REVERSED("CEntity::SetModelIndexNoCreate", 0x00465E20, &CEntity::SetModelIndexNoCreate);
ENTITY_REVERSED("CEntity::GetBoundRect", 0x00466970, &CEntity::GetBoundRect);
ENTITY_REVERSED("CEntity::vf22", 0x00465740, &CEntity::Vf22);
ENTITY_REVERSED("CEntity::vf26", 0x004661B0, &CEntity::GetBoundCentreVirtual);
ENTITY_REVERSED("CEntity::GetBoundDiameter", 0x004661E0, &CEntity::GetBoundDiameter);
ENTITY_REVERSED("CEntity::HasModelFlag10000", 0x00467E30, &CEntity::HasModelFlag10000);
ENTITY_REVERSED("CEntity::DeleteRwObject", 0x004667A0, &CEntity::DeleteRwObject);
ENTITY_REVERSED("CEntity::AttachToRwObject", 0x004666F0, &CEntity::AttachToRwObject);
ENTITY_REVERSED("CEntity::DetachFromRwObject", 0x00466740, &CEntity::DetachFromRwObject);
ENTITY_REVERSED("CEntity::UpdateRwFrame", 0x00465760, &CEntity::UpdateRwFrame);
ENTITY_REVERSED("CEntity::ComputeHasPreRenderEffects", 0x00465780, &CEntity::ComputeHasPreRenderEffects);
ENTITY_REVERSED("CEntity::TransformFromObjectSpace", 0x00466200, &CEntity::TransformFromObjectSpace);
ENTITY_REVERSED("CEntity::GetBoundCentre", 0x00466C70, &CEntity::GetBoundCentre);
ENTITY_REVERSED("CEntity::GetBoundRadius", 0x00465E10, &CEntity::GetBoundRadius);
ENTITY_REVERSED("CEntity::GetDistanceFromCentreOfMassToBaseOfModel", 0x00466110,
                &CEntity::GetDistanceFromCentreOfMassToBaseOfModel);
ENTITY_REVERSED("CEntity::GetIsTouching", 0x00465BA0,
                static_cast<uint8_t (CEntity::*)(const CVector*, float)>(&CEntity::GetIsTouching));
ENTITY_REVERSED("CEntity::GetIsTouchingDistance", 0x00465C50,
                static_cast<uint8_t (CEntity::*)(const CVector*, float, float*)>(&CEntity::GetIsTouching));
ENTITY_REVERSED("CEntity::GetIsTouchingMoving", 0x00465D10, &CEntity::GetIsTouchingMoving);
ENTITY_REVERSED("CEntity::GetIsTouchingBoundCentre", 0x004673B0, &CEntity::GetIsTouchingBoundCentre);
ENTITY_REVERSED("CEntity::GetIsOnScreen", 0x00467460, &CEntity::GetIsOnScreen);
ENTITY_REVERSED("CEntity::GetIsOnScreenLarge", 0x00467520, &CEntity::GetIsOnScreenLarge);
ENTITY_REVERSED("CEntity::IsVisibleAndOnScreen", 0x004686C0, &CEntity::IsVisibleAndOnScreen);
ENTITY_REVERSED("CEntity::IsFrameForRandomUpdate", 0x00465A60, &CEntity::IsFrameForRandomUpdate);
ENTITY_REVERSED("CEntity::SetRwObjectAlpha", 0x00466160, &CEntity::SetRwObjectAlpha);
ENTITY_REVERSED("CEntity::GetObjectValue", 0x00467B80, &CEntity::GetObjectValue);
ENTITY_REVERSED("IsEntityPointerValid", 0x004657F0, &IsEntityPointerValid);
ENTITY_REVERSED("CEntity::GetIsStatic", 0x0045D3C0, &CEntity::GetIsStatic);
ENTITY_REVERSED("CEntity::UpdateFromRwFrame", 0x00465AB0, &CEntity::UpdateFromRwFrame);
ENTITY_REVERSED("CEntity::PreRender", 0x00466CC0, &CEntity::PreRender);
ENTITY_REVERSED("CEntity::SetupBigBuilding", 0x00467BC0, &CEntity::SetupBigBuilding);
ENTITY_REVERSED("CEntity::GetBoundCentreNoClear", 0x00466CA0, &CEntity::GetBoundCentreNoClear);
ENTITY_REVERSED("GetClockSeconds", 0x00465710, &GetClockSeconds);
ENTITY_REVERSED("CVector::DotProduct_468CD0", 0x00468CD0, &DotProduct);
ENTITY_REVERSED("CEntity::RegisterReference", 0x004657D0, &RegisterReference);
ENTITY_REVERSED("CEntity::CleanUpOldReference", 0x004657E0, &CleanUpOldReference);

// ---- CDummy -------------------------------------------------------------------------------

static re::Ptr32<void>* const CDummy_vftable = reinterpret_cast<re::Ptr32<void>*>(0x0090BE1C);

CDummy* CDummy::Construct() {
    Entity_Construct(this);
    vtable = CDummy_vftable;
    entryInfoList.first = nullptr;
    unknown124 = 0;
    typeAndStatus = uint8_t((typeAndStatus & 0xFD) | ENTITY_TYPE_DUMMY);
    return this;
}

void CDummy::Destruct() {
    vtable = CDummy_vftable;
    EntryInfoList_Flush(&entryInfoList);
    Entity_Destruct(this);
}

// a node in the list, and its entry-info node on the entity's list
static void AddEntryInfo(CEntryInfoList* entryInfo, CEntity* entity, CPtrList* list, CSector* sector) {
    CPtrNode* node = NewNodeFor(entity);
    list->AddNode(node);
    CEntryInfoNode* info = static_cast<CEntryInfoNode*>(EntryInfoNodeNew(sizeof(CEntryInfoNode)));
    if (info != nullptr) {
        info->list = list;
        info->node = node;
        info->sector = sector;
    }
    info->prev = nullptr;
    info->next = entryInfo->first;
    if (entryInfo->first != nullptr) {
        entryInfo->first->prev = info;
    }
    entryInfo->first = info;
}

// every entry-info node out of its list and freed
static void RemoveEntryInfos(CEntryInfoList* entryInfo) {
    CEntryInfoNode* info = entryInfo->first;
    while (info != nullptr) {
        CPtrNode* node = info->node;
        PtrNode_RemoveFromList(node, info->list);
        if (node != nullptr) {
            PtrNodeDelete(node);
        }
        CEntryInfoNode* next = info->next;
        if (entryInfo->first == info) {
            entryInfo->first = next;
        }
        if (info->prev != nullptr) {
            info->prev->next = info->next;
        }
        if (info->next != nullptr) {
            info->next->prev = info->prev;
        }
        EntryInfoNodeDelete(info);
        info = next;
    }
}

void CDummy::Add() {
    CRect rect;
    CallGetBoundRect(&rect);
    // (not clamped to the map)
    int x1 = int(rect.left / 50.0 + 18.0);
    int x2 = int(rect.right / 50.0 + 18.0);
    int y2 = int(rect.top / 50.0 + 18.0);
    int y1 = int(rect.bottom / 50.0 + 18.0);
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            CSector* sector = CWorld::GetSector(x, y);
            AddEntryInfo(&entryInfoList, this, &sector->lists[SECTOR_DUMMIES], sector);
        }
    }
}

void CDummy::Remove() {
    RemoveEntryInfos(&entryInfoList);
}

ENTITY_REVERSED("CDummy::CDummy", 0x00465150, &CDummy::Construct);
ENTITY_REVERSED("CDummy::~CDummy", 0x00465180, &CDummy::Destruct);
ENTITY_REVERSED("CDummy::Add", 0x004651A0, &CDummy::Add);
ENTITY_REVERSED("CDummy::Remove", 0x004653B0, &CDummy::Remove);

// ---- CPhysical ---------------------------------------------------------------------------

static re::Ptr32<void>* const CPhysical_vftable = reinterpret_cast<re::Ptr32<void>*>(0x0090C654);

namespace {
GAME_FN(void, __thiscall, Placeable_AllocateStaticMatrix, 0x0046DD40, CPlaceable* placeable);
GAME_FN(CVector*, __thiscall, Entity_GetBoundCentre, 0x00466C70, CEntity* entity, CVector* out);
GAME_FN(float, __thiscall, Entity_GetBoundRadius, 0x00465E10, CEntity* entity);
GAME_FN(CRect*, __thiscall, Physical_GetMovingBoundRect, 0x00469130, CPhysical* physical, CRect* rect);
GAME_FN(CPtrNode*, __thiscall, PtrNode_Construct, 0x0045D670, CPtrNode* node, CEntity* entity);
GAME_FN(void, __thiscall, Matrix_SetRotate, 0x00412D00, CMatrix* m, const CVector* angles);
GAME_VAR(float, ms_fTimeStep, 0x00C1A9A4);   // CTimer::ms_fTimeStep
}

CPhysical* CPhysical::Construct() {
    Entity_Construct(this);
    vtable = CPhysical_vftable;
    moveSpeed = {0.0f, 0.0f, 0.0f};
    entryInfoList.first = nullptr;
    turnSpeed = {0.0f, 0.0f, 0.0f};
    frictionMoveSpeed = {0.0f, 0.0f, 0.0f};
    frictionTurnSpeed = {0.0f, 0.0f, 0.0f};
    centreOfMass = {0.0f, 0.0f, 0.0f};
    unknown1A8 = 0;
    unknown1AC = 0;
    unknown1B0 = 0;
    unknown1B8 = nullptr;
    bIsPhysical = 1;
    Placeable_AllocateStaticMatrix(this);
    movingListNode = nullptr;
    moveSpeed = {0.0f, 0.0f, 0.0f};
    turnSpeed = {0.0f, 0.0f, 0.0f};
    frictionMoveSpeed = {0.0f, 0.0f, 0.0f};
    timeScale = 1.0f;
    frictionTurnSpeed = {0.0f, 0.0f, 0.0f};
    stuckCount = 0;
    numCollisionRecords = 0;
    for (int i = 0; i < 6; i++) {
        collisionRecords[i] = nullptr;
    }
    unknown1A0 = 0;  // (0.0f)
    unknown1B4 = 0;
    unknown1A4 = 0;
    unknown1A8 = 0;
    centreOfMass.x = 0.0f;
    unknown19C = 0;  // (0.0f)
    unknown1AC = 0;
    unknown1B0 = 0;
    centreOfMass.y = 0.0f;
    physicalFlags2 = uint8_t((physicalFlags2 & 0xF0) | 0x10);
    bUsesCollision = 1;
    centreOfMass.z = 0.0f;
    physicalFlags = 2;
    return this;
}

CPhysical* CPhysical::Destroy(uint8_t flags) {
    vtable = CPhysical_vftable;
    if (unknown1B8 != nullptr) {
        re::Ptr32<void>* objectVtable = *static_cast<re::Ptr32<re::Ptr32<void>>*>(unknown1B8);
        re::PtrCast<void(__thiscall*)(void*, CPhysical*)>(objectVtable[1])(unknown1B8, this);
    }
    EntryInfoList_Flush(&entryInfoList);
    Entity_Destruct(this);
    if (flags & 1) {
        Free(this);
    }
    return this;
}

// which list of a sector a physical goes in (nullptr: none; buildings and dummies
// aren't physicals)
static CPtrList* PhysicalListForType(CSector* sector, uint8_t type) {
    switch (type) {
    case ENTITY_TYPE_VEHICLE:
        return &sector->lists[SECTOR_VEHICLES];
    case ENTITY_TYPE_PED:
        return &sector->lists[SECTOR_PEDS];
    case ENTITY_TYPE_OBJECT:
    case ENTITY_TYPE_PROPANIM:
    case ENTITY_TYPE_7:
        return &sector->lists[SECTOR_OBJECTS];
    default:
        return nullptr;
    }
}

void CPhysical::Add() {
    CRect rect;
    Physical_GetMovingBoundRect(this, &rect);
    int x1 = SectorLow(rect.left);
    int y1 = SectorLow(rect.bottom);
    int x2 = SectorHigh(rect.right);
    int y2 = SectorHigh(rect.top);
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            CSector* sector = CWorld::GetSector(x, y);
            CPtrList* list = PhysicalListForType(sector, GetType());
            if (list != nullptr) {
                AddEntryInfo(&entryInfoList, this, list, sector);
            }
        }
    }
}

void CPhysical::Remove() {
    RemoveEntryInfos(&entryInfoList);
}

void CPhysical::RemoveAndAdd() {
    CEntryInfoNode* info = entryInfoList.first;
    CRect rect;
    Physical_GetMovingBoundRect(this, &rect);
    int x1 = SectorLow(rect.left);
    int y1 = SectorLow(rect.bottom);
    int x2 = SectorHigh(rect.right);
    int y2 = SectorHigh(rect.top);
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            CSector* sector = CWorld::GetSector(x, y);
            CPtrList* list = PhysicalListForType(sector, GetType());
            if (list == nullptr) {
                continue;
            }
            if (info == nullptr) {
                AddEntryInfo(&entryInfoList, this, list, sector);
            } else {
                // the old entry moved to this list
                CPtrNode* node = info->node;
                PtrNode_RemoveFromList(node, info->list);
                list->AddNode(node);
                info->sector = sector;
                info->list = list;
                info = info->next;
            }
        }
    }
    // the ones left over
    while (info != nullptr) {
        CPtrNode* node = info->node;
        PtrNode_RemoveFromList(node, info->list);
        if (node != nullptr) {
            PtrNodeDelete(node);
        }
        CEntryInfoNode* next = info->next;
        if (entryInfoList.first == info) {
            entryInfoList.first = next;
        }
        if (info->prev != nullptr) {
            info->prev->next = info->next;
        }
        if (info->next != nullptr) {
            info->next->prev = info->prev;
        }
        EntryInfoNodeDelete(info);
        info = next;
    }
}

CRect* CPhysical::GetBoundRect(CRect* rect) {
    CVector center;
    Entity_GetBoundCentre(this, &center);
    float radius = Entity_GetBoundRadius(this);
    rect->left = center.x - radius;
    rect->bottom = center.y - radius;
    rect->right = center.x + radius;
    rect->top = center.y + radius;
    return rect;
}

CRect* CPhysical::GetMovingBoundRect(CRect* rect) {
    CVector center;
    Entity_GetBoundCentre(this, &center);
    float radius = Entity_GetBoundRadius(this);
    if (flagC4 != 0 && bIsStatic == 0) {
        float speed = float(std::sqrt(std::fabs(moveSpeed.x * moveSpeed.x + moveSpeed.y * moveSpeed.y + moveSpeed.z * moveSpeed.z)));
        radius = speed * ms_fTimeStep + radius;
    }
    rect->left = center.x - radius;
    rect->bottom = center.y - radius;
    rect->right = center.x + radius;
    rect->top = center.y + radius;
    return rect;
}

void CPhysical::AddToMovingList() {
    if (movingListNode != nullptr || bIsStaticWaitingForCollision != 0) {
        return;
    }
    CPtrNode* node = static_cast<CPtrNode*>(PtrNodeNew(4));
    node = node ? PtrNode_Construct(node, this) : nullptr;
    movingListNode = node;
    CWorld::ms_listMovingEntityPtrs.AddNode(node);
}

void CPhysical::RemoveFromMovingList() {
    CPtrNode* node = movingListNode;
    if (node == nullptr) {
        return;
    }
    // (the moving list's walk steps over it)
    if (node == CWorld::ms_pNextMovingNode) {
        CWorld::ms_pNextMovingNode = node->GetNext();
    }
    PtrNode_RemoveFromList(movingListNode, &CWorld::ms_listMovingEntityPtrs);
    if (movingListNode != nullptr) {
        PtrNodeDelete(movingListNode);
    }
    movingListNode = nullptr;
}

uint8_t CPhysical::GetHasCollidedWith(CEntity* entity) {
    if (flag3C == 0) {
        return 0;
    }
    for (int i = 0; i < numCollisionRecords; i++) {
        if (collisionRecords[i] == entity) {
            return 1;
        }
    }
    return 0;
}

void CPhysical::RemoveCollisionRecord(CEntity* entity) {
    int16_t i = 0;
    while (i < int16_t(numCollisionRecords)) {
        if (collisionRecords[i] == entity) {
            for (int16_t j = i; j < numCollisionRecords - 1; j++) {
                collisionRecords[j] = collisionRecords[j + 1];
            }
            numCollisionRecords--;
        } else {
            i++;
        }
    }
}

void CPhysical::ResetCollisionState() {
    unknown1A0 = 0;  // (0.0f)
    unknown1B4 = 0;
    unknown1A4 = 0;
}

void CPhysical::SetOrientation(float x, float y, float z) {
    if (matrix != nullptr) {
        CVector pos = *reinterpret_cast<CVector*>(&matrix->m.m[12]);
        CVector angles{x, y, z};
        Matrix_SetRotate(&matrix->m, &angles);
        matrix->m.m[12] = pos.x;
        matrix->m.m[13] = pos.y;
        matrix->m.m[14] = pos.z;
        // (the game also writes uninitialised stack to +0x3C; ours leaves it)
    }
    placement.heading = z;
}

void CPhysical::ApplyMoveSpeed() {
    if (physicalFlags2 & 1) {
        moveSpeed.z = 0.0f;
        moveSpeed.y = 0.0f;
        moveSpeed.x = 0.0f;
        return;
    }
    Virtual<void>(36);
    float step = timeScale * ms_fTimeStep;
    CVector& pos = GetPosition();
    pos.x = moveSpeed.x * step + pos.x;
    pos.y = moveSpeed.y * step + pos.y;
    if ((physicalFlags & 2) == 0 || Virtual<uint8_t>(34)) {
        pos.z = moveSpeed.z * step + pos.z;
        return;
    }
    // under gravity (vf35): the fall over the step
    double fall = double(moveSpeed.z) * step;
    float stepSq = step * step;
    pos.z = float(fall - double(Virtual<float>(35)) * 0.5 * stepSq + pos.z);
}

CEntity* __cdecl AsPed(CEntity* entity) {
    if (entity == nullptr) {
        return nullptr;
    }
    return entity->GetType() == ENTITY_TYPE_PED ? entity : nullptr;
}

ENTITY_REVERSED("CPhysical::CPhysical", 0x00468E90, &CPhysical::Construct);
ENTITY_REVERSED("CPhysical::vf0", 0x0046A720, &CPhysical::Destroy);
ENTITY_REVERSED("CPhysical::Add", 0x004691E0, &CPhysical::Add);
ENTITY_REVERSED("CPhysical::Remove", 0x0046A770, &CPhysical::Remove);
ENTITY_REVERSED("CPhysical::RemoveAndAdd", 0x00469470, &CPhysical::RemoveAndAdd);
ENTITY_REVERSED("CPhysical::GetBoundRect", 0x004690E0, &CPhysical::GetBoundRect);
ENTITY_REVERSED("CPhysical::GetMovingBoundRect", 0x00469130, &CPhysical::GetMovingBoundRect);
ENTITY_REVERSED("CPhysical::AddToMovingList", 0x004697D0, &CPhysical::AddToMovingList);
ENTITY_REVERSED("CPhysical::RemoveFromMovingList", 0x00469840, &CPhysical::RemoveFromMovingList);
ENTITY_REVERSED("CPhysical::GetHasCollidedWith", 0x00468D40, &CPhysical::GetHasCollidedWith);
ENTITY_REVERSED("CPhysical::RemoveCollisionRecord", 0x00468D80, &CPhysical::RemoveCollisionRecord);
ENTITY_REVERSED("CPhysical::ResetCollisionState", 0x00468DF0, &CPhysical::ResetCollisionState);
ENTITY_REVERSED("CPhysical::SetOrientation", 0x0046A690, &CPhysical::SetOrientation);
ENTITY_REVERSED("CPhysical::ApplyMoveSpeed", 0x004698B0, &CPhysical::ApplyMoveSpeed);
ENTITY_REVERSED("AsPed", 0x00468E10, &AsPed);
