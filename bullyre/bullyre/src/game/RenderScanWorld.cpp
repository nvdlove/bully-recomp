// CRenderer's world scan and its render passes over the visible list (GTA's
// ConstructRenderList, ScanWorld, ScanBigBuildingList, RenderEverythingBarRoads,
// RenderFadingInEntities, RenderFirstPersonVehicle). The sector setup and the
// polygon scan-converter (0x450C90 .. 0x452AC0) are called where they are.

#include <cmath>
#include <cstdint>

#include "game/Entity.h"
#include "game/World.h"
#include "reversed.h"
#include "runflags.h"

namespace {

// GTA's CLinkList<T> (0x48): used and free lists between dummy heads and tails
struct RsLink {
    re::Ptr32<void> item;      // +0 (an entity, an atomic)
    float dist;      // +4
    re::Ptr32<RsLink> prev;    // +8
    re::Ptr32<RsLink> next;    // +0xC
};
static_assert(sizeof(RsLink) == 0x10);

struct RsLinkList {
    RsLink usedHead;     // +0
    RsLink usedTail;     // +0x10
    RsLink freeHead;     // +0x20
    RsLink freeTail;     // +0x30
    re::Ptr32<RsLink> links;       // +0x40
    uint32_t unknown44;  // +0x44
};
static_assert(sizeof(RsLinkList) == 0x48);

// (0x452AC0, inlined in RenderFadingInEntities) every used link to the free list
inline void RsLinkList_Clear(RsLinkList* list) {
    while (list->usedHead.next != &list->usedTail) {
        RsLink* link = list->usedHead.next;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = list->freeHead.next;
        list->freeHead.next->prev = link;
        link->prev = &list->freeHead;
        list->freeHead.next = link;
    }
}

// the visible lists
GAME_ARRAY(CEntity*, rs_aVisibleEntityPtrs, 0x00C11FC0);
GAME_VAR(int32_t, rs_nNoOfVisibleEntities, 0x00C13F00);
GAME_VAR(re::Ptr32<CEntity>*, rs_pVisibleEntityWrite, 0x00C11D60);     // the next free slot of the list above
GAME_VAR(void*, rs_pVisibleLodWrite, 0x00C11D5C);            // (= 0xC11D68)
GAME_VAR(int32_t, rs_nNoOfVisibleLods, 0x00C11D64);
GAME_VAR(CEntity*, rs_pFirstPersonVehicle, 0x00C11D58);
GAME_VAR(col::CVector, rs_vecCameraPosition, 0x00C13F0C);
GAME_VAR(uint8_t, rs_flagC13F04, 0x00C13F04);                // cleared by each scan
GAME_VAR(RsLinkList, rs_fadingEntities, 0x00C13F28);          // sorted, drawn far to near
GAME_VAR(RsLinkList, rs_alphaAtomics, 0x01B64898);            // a vehicle's alpha atomics
GAME_VAR(RsLinkList, rs_sortedList1, 0x01B648E0);
GAME_VAR(RsLinkList, rs_sortedList2, 0x01B64928);
GAME_VAR(RsLinkList, rs_sortedList3, 0x01B64970);
GAME_VAR(uint16_t, rs_nCurrentScanCode2, 0x00C1AE8C);         // (the entities' +0x112)
GAME_VAR(uint32_t, rs_frameCounter, 0x00C1A99C);              // CTimer::m_FrameCounter
GAME_VAR(uint8_t, rs_bDisableStreaming, 0x00CA3984);
GAME_VAR(uint8_t*, rs_ptrNodes, 0x00C0F788);                  // CPools::ms_pPtrNodes' objects
GAME_ARRAY(CPtrList, rs_bigBuildingLists, 0x00C216B8);        // [area]: [0] everywhere
GAME_VAR(int32_t, rs_bigBuildingArea, 0x00BD0FC4);
GAME_ARRAY(uint8_t*, rs_modelInfoPtrs, 0x00C67738);           // CModelInfo::ms_modelInfoPtrs
GAME_VAR(void*, rs_camera, 0x00C3CCF4);                       // the Gamebryo camera
// the interior the camera is in (scans only its rectangle)
GAME_VAR(uint8_t, rs_bCameraInInterior, 0x00C936FA);
GAME_VAR(int8_t, rs_cameraInterior, 0x00C936F9);
inline uint8_t* const rs_interiors = reinterpret_cast<uint8_t*>(0x00C92D94);   // 0x60 each
GAME_VAR(uint8_t, rs_bNoPlayerSector, 0x020C5BE6);
inline void* const rs_renderState = reinterpret_cast<void*>(0x00D140B8);

GAME_FN(void, __thiscall, RS_LinkList_Clear, 0x00452AC0, RsLinkList* list);
GAME_FN(void, __thiscall, RS_LinkList_InsertSorted, 0x005F2F70, RsLinkList* list, RsLink* item);   // (item: entity, dist)
GAME_FN(void, __cdecl, RS_SetupVehicleAlphaAtomics, 0x005F2C20, void* rwObject);
GAME_FN(void, __cdecl, RS_RenderAlphaAtomics, 0x005F2B60, RsLinkList* list);
GAME_FN(void, __thiscall, RS_RenderState_Reset, 0x005EE560, void* state);
GAME_FN(int, __cdecl, RS_GetObjectAlpha, 0x005F2B10, void* rwObject);
GAME_FN(uint8_t, __cdecl, RS_InsertEntityIntoSortedList, 0x005F33F0, CEntity* entity, float dist);
GAME_FN(col::CVector*, __cdecl, RS_VectorSub, 0x00414B70, col::CVector* out, const col::CVector* a, const col::CVector* b);
GAME_FN(void, __cdecl, RS_SetRenderState, 0x00519790, int state, int value);
GAME_FN(CEntity*, __cdecl, RS_GetEntityInPool, 0x0044A3E0, int pool, int slot);
GAME_FN(int, __cdecl, RS_SetupBigBuildingVisibility, 0x00452000, CEntity* entity);
GAME_FN(void, __cdecl, RS_RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, RS_ClearScanCodes2, 0x0045D9C0);
GAME_FN(const float*, __cdecl, RS_Camera_GetFrustumXY, 0x006C8310, void* camera);   // (right, top) at distance 1
GAME_FN(float, __cdecl, RS_Camera_GetFar, 0x006C82D0, void* camera);
GAME_FN(void*, __cdecl, RS_Camera_GetNode, 0x006C82F0, void* camera);
GAME_FN(const float*, __cdecl, RS_Node_GetWorldTransform, 0x006C9A90, void* node);   // rotation[9], translation, scale
GAME_FN(void, __cdecl, RS_TransformPoints, 0x00451410, float* out, const float* in, int count, const float* transform);
GAME_FN(void, __cdecl, RS_ScanSectorPoly, 0x00450C90, float* poly, int count, void(__cdecl* scan)(CSector*));
GAME_FN(void, __cdecl, RS_ScanSectorList, 0x00452430, CSector* sector);
GAME_FN(void, __cdecl, RS_ScanSectorListLod, 0x00452770, CSector* sector);
GAME_FN(void, __cdecl, RS_ProjectToHeight, 0x00451380, float* point, const float* from, float z);   // along from->point
GAME_FN(void, __thiscall, RS_Rect_Extend, 0x00414570, float* rect, const float* point2d);
GAME_FN(void, __cdecl, RS_GetPlayerCoors, 0x00446900, col::CVector* out);
GAME_FN(void, __cdecl, RS_PreScan, 0x0051FDA0);
// ours, called where modes apply
GAME_FN(void, __cdecl, RS_RenderOneNonRoad, 0x00452B20, CEntity* entity);
GAME_FN(void, __cdecl, RS_ScanBigBuildingList, 0x00452B90, CPtrList* list);
GAME_FN(void, __cdecl, RS_ScanWorld, 0x00452F20);

inline void CallRender(CEntity* entity) {
    reinterpret_cast<void(__thiscall*)(CEntity*)>(static_cast<void*>(entity->vtable[0x48 / 4]))(entity);
}

} // namespace

namespace CRenderer {

// 0x452B20: one entity's vf18 (Render); a vehicle's alpha atomics around it
void __cdecl RenderOneNonRoad(CEntity* entity) {
    if (entity->GetType() == ENTITY_TYPE_VEHICLE) {
        RS_SetupVehicleAlphaAtomics(entity->rwObject);
        RS_LinkList_Clear(&rs_alphaAtomics);
    }
    CallRender(entity);
    if (entity->GetType() == ENTITY_TYPE_VEHICLE) {
        entity->bIsBeingRendered = 1;
        RS_RenderAlphaAtomics(&rs_alphaAtomics);
        entity->bIsBeingRendered = 0;
    }
    RS_RenderState_Reset(rs_renderState);
}

// 0x452B90: the big buildings of a list onto the visible list (1: visible; 4: its
// model requested). One in four frames (by the entity's random seed) goes through the
// full test; the others are taken as visible unless the last test said otherwise
void __cdecl ScanBigBuildingList(CPtrList* list) {
    CPtrNode* node = list->first;
    uint32_t frame = rs_frameCounter & 3;
    if (!node) {
        return;
    }
    for (;;) {
        CEntity* entity = RS_GetEntityInPool(node->bits & 0xF, (node->bits >> 4) & 0x3FFF);
        int result;
        if (entity->flagA4 == 0 && uint32_t(entity->randomSeed & 3) == frame) {
            result = 1;
        } else {
            entity->flagA4 = 1;
            result = RS_SetupBigBuildingVisibility(entity);
        }
        if (result == 1) {
            *rs_pVisibleEntityWrite = entity;
            rs_pVisibleEntityWrite++;
            rs_nNoOfVisibleEntities++;
            entity->flagA4 = 0;
        } else if (result == 4 && !rs_bDisableStreaming) {
            RS_RequestModel(entity->modelIndex, 0);
        }
        uint32_t next = node->bits >> 18;
        if (next == CPtrNode::NO_NEXT) {
            break;
        }
        node = reinterpret_cast<CPtrNode*>(rs_ptrNodes + next * 4);
        if (!node) {
            break;
        }
    }
}

// 0x452C70: the visible list but roads (model flag 4 of +0x28 on buildings):
// vehicles with +0x84 to the fading list, vehicles and faded peds to the sorted
// list (drawn now when it's full), the rest drawn now
void __cdecl RenderEverythingBarRoads() {
    RS_LinkList_Clear(&rs_fadingEntities);
    for (int i = 0; i < rs_nNoOfVisibleEntities; i++) {
        CEntity* entity = rs_aVisibleEntityPtrs[i];
        uint8_t type = entity->GetType();
        if (type == ENTITY_TYPE_BUILDING && (rs_modelInfoPtrs[entity->modelIndex][0x28] & 4)) {
            continue;
        }
        if (type == ENTITY_TYPE_VEHICLE || (type == ENTITY_TYPE_PED && RS_GetObjectAlpha(entity->rwObject) != 0xFF)) {
            if (entity->GetType() == ENTITY_TYPE_VEHICLE && entity->flag84[0] != 0) {
                RsLink item;
                item.item = entity;
                col::CVector d;
                const col::CVector* v = RS_VectorSub(&d, &rs_vecCameraPosition, &entity->GetPosition());
                item.dist = v->x * v->x + v->y * v->y + v->z * v->z;   // (squared here)
                RS_LinkList_InsertSorted(&rs_fadingEntities, &item);
                continue;
            }
            col::CVector d;
            const col::CVector* v = RS_VectorSub(&d, &rs_vecCameraPosition, &entity->GetPosition());
            float distSq = v->x * v->x + v->y * v->y + v->z * v->z;
            if (RS_InsertEntityIntoSortedList(entity, std::sqrt(std::fabs(distSq)))) {
                continue;
            }
        }
        RS_RenderOneNonRoad(entity);
    }
}

// 0x452DE0: the fading list, far to near (RenderOneNonRoad inlined, its list
// clear too)
void __cdecl RenderFadingInEntities() {
    for (RsLink* link = rs_fadingEntities.usedTail.prev; link != &rs_fadingEntities.usedHead; link = link->prev) {
        CEntity* entity = static_cast<CEntity*>(link->item);
        if (entity->GetType() == ENTITY_TYPE_VEHICLE) {
            RS_SetupVehicleAlphaAtomics(entity->rwObject);
            RsLinkList_Clear(&rs_alphaAtomics);
        }
        CallRender(entity);
        if (entity->GetType() == ENTITY_TYPE_VEHICLE) {
            entity->bIsBeingRendered = 1;
            RS_RenderAlphaAtomics(&rs_alphaAtomics);
            entity->bIsBeingRendered = 0;
        }
        RS_RenderState_Reset(rs_renderState);
    }
}

// 0x452EC0
void __cdecl RenderFirstPersonVehicle() {
    if (!rs_pFirstPersonVehicle) {
        return;
    }
    RS_SetRenderState(0xE, 1);
    RS_SetRenderState(8, 1);
    RS_SetRenderState(6, 1);
    RS_SetRenderState(0xC, 1);
    RS_SetRenderState(0xA, 5);
    RS_SetRenderState(0xB, 6);
    RS_RenderOneNonRoad(rs_pFirstPersonVehicle);
    RS_SetRenderState(0xE, 0);
}

static float ToSectorF(float v) {
    return float(v / 50.0 + 18.0);
}
static int ToSectorI(float v) {
    return int(v / 50.0 + 18.0);
}
// (the game goes through float and unsigned conversions: the same as clamping)
static uint32_t ClampSector(float v) {
    int s = ToSectorI(v);
    if (s <= 0) {
        s = 0;
    }
    if (s >= 35) {
        s = 35;
    }
    return uint32_t(s);
}

// 0x452F20: the sectors the camera sees onto the lists. In an interior: the sectors
// of its rectangle's corners; looking down (the view's z below -0.95): the
// rectangle of the frustum's corners dropped to z -100; otherwise the frustum's
// triangle scan-converted (up to 300 units, the far part with the LOD scan past
// that), and the player's own sector
void __cdecl ScanWorld() {
    const float* frustum = RS_Camera_GetFrustumXY(rs_camera);
    float farDist = RS_Camera_GetFar(rs_camera);
    float pts[9][3];
    pts[0][0] = 0.0f;
    pts[0][1] = 0.0f;
    pts[0][2] = 0.0f;
    pts[1][0] = -frustum[0] * farDist;
    pts[1][1] = frustum[1] * farDist;
    pts[1][2] = farDist;
    pts[2][0] = frustum[0] * farDist;
    pts[2][1] = frustum[1] * farDist;
    pts[2][2] = farDist;
    pts[3][0] = frustum[0] * farDist;
    pts[3][1] = -frustum[1] * farDist;
    pts[3][2] = farDist;
    pts[4][0] = -frustum[0] * farDist;
    pts[4][1] = -frustum[1] * farDist;
    pts[4][2] = farDist;
    const float* transform = RS_Node_GetWorldTransform(RS_Camera_GetNode(rs_camera));

    rs_pFirstPersonVehicle = nullptr;
    RS_LinkList_Clear(&rs_sortedList1);
    RS_LinkList_Clear(&rs_sortedList2);
    RS_LinkList_Clear(&rs_sortedList3);
    if (rs_nCurrentScanCode2 >= 0xFFFF) {
        RS_ClearScanCodes2();
        rs_nCurrentScanCode2 = 1;
    } else {
        rs_nCurrentScanCode2++;
    }
    rs_flagC13F04 = 0;

    // the corners at 300 units (and a fifth of the way across), of the upper or
    // the lower side by the view's z
    const float* a = 0.0f < transform[8] ? pts[4] : pts[1];
    const float* b = 0.0f < transform[8] ? pts[3] : pts[2];
    float inv = 1.0f / farDist;
    float a300[3] = {a[0] * 300.0f, a[1] * 300.0f, a[2] * 300.0f};
    float b300[3] = {b[0] * 300.0f, b[1] * 300.0f, b[2] * 300.0f};
    pts[5][0] = inv * a300[0];
    pts[5][1] = inv * a300[1];
    pts[5][2] = inv * a300[2];
    pts[6][0] = inv * b300[0];
    pts[6][1] = inv * b300[1];
    pts[6][2] = inv * b300[2];
    pts[7][0] = float(pts[5][0] * double(0.2f));
    pts[7][1] = float(pts[5][1] * double(0.2f));
    pts[7][2] = pts[5][2];
    pts[8][0] = float(pts[6][0] * double(0.2f));
    pts[8][1] = float(pts[6][1] * double(0.2f));
    pts[8][2] = pts[6][2];
    RS_TransformPoints(&pts[0][0], &pts[0][0], 9, transform);

    // (the remaster's seamless doors scan from a camera through a door: the main camera's interior
    // box isn't that camera's: runflags.h)
    if (rs_bCameraInInterior && !re::PortalScan()) {
        const uint8_t* interior = rs_cameraInterior == -1 ? nullptr : rs_interiors + rs_cameraInterior * 0x60;
        float x1 = *reinterpret_cast<const float*>(interior + 0x40);
        float y1 = *reinterpret_cast<const float*>(interior + 0x44);
        float x2 = *reinterpret_cast<const float*>(interior + 0x50);
        float y2 = *reinterpret_cast<const float*>(interior + 0x54);
        int sectors[4][2];
        sectors[0][0] = ToSectorI(x1);
        sectors[0][1] = ToSectorI(y1);
        sectors[1][0] = ToSectorI(x2);
        sectors[1][1] = ToSectorI(y2);
        int count = 1;
        if (sectors[0][0] != sectors[1][0] || sectors[0][1] != sectors[1][1]) {
            count = 2;
            sectors[2][0] = sectors[0][0];
            sectors[2][1] = sectors[1][1];
            if (sectors[1][1] != sectors[0][1] && sectors[0][0] != sectors[1][0]) {
                count = 4;
                sectors[3][0] = sectors[1][0];
                sectors[3][1] = sectors[0][1];
            }
        }
        for (int i = 0; i < count; i++) {
            RS_ScanSectorList(CWorld::GetSector(sectors[i][0], sectors[i][1]));
        }
    } else if (transform[8] > -0.95f) {
        float poly[6];
        poly[0] = ToSectorF(pts[0][0]);
        poly[1] = ToSectorF(pts[0][1]);
        if (300.0f < farDist) {
            poly[2] = ToSectorF(pts[7][0]);
            poly[3] = ToSectorF(pts[7][1]);
            poly[4] = ToSectorF(pts[8][0]);
            poly[5] = ToSectorF(pts[8][1]);
            RS_ScanSectorPoly(poly, 3, RS_ScanSectorListLod);
            poly[0] = ToSectorF(pts[0][0]);
            poly[1] = ToSectorF(pts[0][1]);
            poly[2] = ToSectorF(pts[5][0]);
            poly[3] = ToSectorF(pts[5][1]);
            poly[4] = ToSectorF(pts[6][0]);
            poly[5] = ToSectorF(pts[6][1]);
            RS_ScanSectorPoly(poly, 3, RS_ScanSectorList);
        } else {
            poly[2] = ToSectorF(pts[1][0]);
            poly[3] = ToSectorF(pts[1][1]);
            poly[4] = ToSectorF(pts[2][0]);
            poly[5] = ToSectorF(pts[2][1]);
            RS_ScanSectorPoly(poly, 3, RS_ScanSectorList);
        }
        if (!rs_bNoPlayerSector && !re::PortalScan()) {
            col::CVector player;
            RS_GetPlayerCoors(&player);
            int y = ToSectorI(player.y);
            int x = ToSectorI(player.x);
            RS_ScanSectorList(CWorld::GetSector(x, y));
        } else if (re::PortalScan()) {
            // (the seamless doors' pass, from a camera through a door: the sectors around that camera,
            // as the player's for the main one; the thin end of the view's triangle can miss them)
            int cx = ToSectorI(pts[0][0]);
            int cy = ToSectorI(pts[0][1]);
            for (int y = cy - 1; y <= cy + 1; y++) {
                for (int x = cx - 1; x <= cx + 1; x++) {
                    if (x >= 0 && x < 36 && y >= 0 && y < 36) {
                        RS_ScanSectorList(CWorld::GetSector(x, y));
                    }
                }
            }
        }
    } else {
        // (minX, maxY, maxX, minY)
        float rect[4] = {1000000.0f, -1000000.0f, -1000000.0f, 1000000.0f};
        for (int i = 1; i <= 4; i++) {
            RS_ProjectToHeight(pts[i], pts[0], -100.0f);
            float point[2] = {pts[i][0], pts[i][1]};
            RS_Rect_Extend(rect, point);
        }
        uint32_t x1 = ClampSector(rect[0]);
        uint32_t x2 = ClampSector(rect[2]);
        uint32_t y1 = ClampSector(rect[3]);
        uint32_t y2 = ClampSector(rect[1]);
        for (uint32_t x = x1; x <= x2; x++) {
            for (uint32_t y = y1; y <= y2; y++) {
                RS_ScanSectorList(CWorld::GetSector(int(x), int(y)));
            }
        }
    }
    if (re::PortalScan()) {
        return;   // (the remaster's seamless doors: runflags.h)
    }
    RS_ScanBigBuildingList(&rs_bigBuildingLists[rs_bigBuildingArea]);
    RS_ScanBigBuildingList(&rs_bigBuildingLists[0]);
}

// 0x453A00: the lists emptied, the camera's position, then ScanWorld
void __cdecl ConstructRenderList() {
    RS_PreScan();
    rs_nNoOfVisibleEntities = 0;
    rs_nNoOfVisibleLods = 0;
    rs_pVisibleEntityWrite = rs_aVisibleEntityPtrs;
    rs_pVisibleLodWrite = reinterpret_cast<void*>(0x00C11D68);
    const float* transform = RS_Node_GetWorldTransform(RS_Camera_GetNode(rs_camera));
    rs_vecCameraPosition.x = transform[9];
    rs_vecCameraPosition.y = transform[10];
    rs_vecCameraPosition.z = transform[11];
    RS_ScanWorld();
}

} // namespace CRenderer

// ---- registration ---------------------------------------------------------------------

#define C04C_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04C_REVERSED("CRenderer::RenderOneNonRoad", 0x00452B20, &CRenderer::RenderOneNonRoad);
C04C_REVERSED("CRenderer::ScanBigBuildingList", 0x00452B90, &CRenderer::ScanBigBuildingList);
C04C_REVERSED("CRenderer::RenderEverythingBarRoads", 0x00452C70, &CRenderer::RenderEverythingBarRoads);
C04C_REVERSED("CRenderer::RenderFadingInEntities", 0x00452DE0, &CRenderer::RenderFadingInEntities);
C04C_REVERSED("CRenderer::RenderFirstPersonVehicle", 0x00452EC0, &CRenderer::RenderFirstPersonVehicle);
C04C_REVERSED("CRenderer::ScanWorld", 0x00452F20, &CRenderer::ScanWorld);
C04C_REVERSED("CRenderer::ConstructRenderList", 0x00453A00, &CRenderer::ConstructRenderList);
