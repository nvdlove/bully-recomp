// CRenderer's visibility (GTA's names; 0x450BC0 .. 0x452B00): the sector polygon
// scan-converter (ScanSectorPoly), what an entity's model says about drawing it
// (SetupEntityVisibility, SetupBigBuildingVisibility, ShouldModelBeStreamed), the
// sector lists' scans into the visible lists, PreRender, the big buildings sorted
// to the front of the sectors' building lists, and the fading entities' passes.
//
// Visibility results: 0 invisible, 1 visible, 2 visible (no distance sort), 3 off
// screen (in range), 4 stream it in.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

enum eVisibility : int {
    VIS_INVISIBLE = 0,
    VIS_VISIBLE = 1,
    VIS_VISIBLE_UNSORTED = 2,
    VIS_OFFSCREEN = 3,
    VIS_STREAMME = 4,
};

// the entity fields read here (CEntity, Entity.h)
constexpr uint32_t E_POSITION = 0x4, E_MATRIX = 0x14, E_RWOBJECT = 0x18, E_FLAG48 = 0x48, E_BIGBUILDING = 0x58,
                   E_FLAG5C = 0x5C, E_FLAG8C = 0x8C, E_FLAG98 = 0x98, E_OFFSCREEN = 0xA4, E_WASVISIBLE = 0xA8, E_FLAGB4 = 0xB4,
                   E_TYPE = 0x108, E_MODEL = 0x10E, E_AREA = 0x111, E_SCANCODE2 = 0x112, E_ANIM = 0x114, E_LODFADE = 0x11C;
// the model info's (CSimpleModelInfo, CTimeModelInfo; ModelInfo.h)
constexpr uint32_t MI_TYPE = 0x8, MI_CHAPTERS = 0xB, MI_RELATED = 0x20, MI_FLAGS = 0x28, MI_ALPHA = 0x2C, MI_TIMEON = 0x34,
                   MI_TIMEOFF = 0x38, MI_OTHERMODEL = 0x3C, MI_WEAPONDIST = 0x50;

constexpr int NUM_SECTORS_X = 36, NUM_SECTORS_Y = 36;
constexpr uint32_t SECTOR_SIZE = 0x14;   // five lists

GAME_ARRAY(uint8_t, sectors, 0x00C1B178);                     // CWorld::ms_aSectors [36 * 36]
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, currentChapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);
GAME_ARRAY(float, seasonNightStart, 0x00A147C8);              // hours, by season (time models' default)
GAME_ARRAY(float, seasonNightEnd, 0x00A147B8);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, bSecondAreaVisible, 0x00BD09EC);
GAME_VAR(uint8_t, bSecondAreaForced, 0x00BD09E8);
GAME_VAR(int8_t, secondArea, 0x00BD09EB);
GAME_ARRAY(float, vecCameraPosition, 0x00C13F0C);             // CRenderer::ms_vecCameraPosition
GAME_ARRAY(uint8_t*, aVisibleEntityPtrs, 0x00C11FC0);         // [2000]
GAME_VAR(int32_t, nNoOfVisibleEntities, 0x00C13F00);
GAME_VAR(re::Ptr32<uint8_t>*, pVisibleEntityWrite, 0x00C11D60);
GAME_ARRAY(uint8_t*, aVisibleLodPtrs, 0x00C11D68);            // [150]
GAME_VAR(int32_t, nNoOfVisibleLods, 0x00C11D64);
GAME_VAR(re::Ptr32<uint8_t>*, pVisibleLodWrite, 0x00C11D5C);
GAME_VAR(uint8_t, bLoadingPriority, 0x00C13F04);
GAME_VAR(void*, pBuffer_C13F68, 0x00C13F68);
GAME_VAR(uint16_t, nCurrentScanCode2, 0x00C1AE8C);
GAME_VAR(uint8_t, bCameraInInterior, 0x00C936FA);
GAME_VAR(int8_t, cameraInterior, 0x00C936F8);
inline uint8_t* const interiorMinZ = reinterpret_cast<uint8_t*>(0x00C92DE0);   // 0x60 each
inline uint8_t* const interiorMaxZ = reinterpret_cast<uint8_t*>(0x00C92DF0);
GAME_VAR(void*, camera, 0x00C3CCF4);
GAME_VAR(uint8_t, bDisableStreaming, 0x00CA3984);             // CStreaming::ms_disableStreaming
GAME_VAR(int32_t, numModelsRequested, 0x00CA3988);            // CStreaming::ms_numModelsRequested
GAME_VAR(uint32_t*, ptrNodes, 0x00C0F788);                    // CPools::ms_pPtrNodes' array
inline void* const linkList_C13F28 = reinterpret_cast<void*>(0x00C13F28);
inline uint8_t* const alphaEntityList = reinterpret_cast<uint8_t*>(0x01B648E0);   // CVisibilityPlugins' (a CLinkList)
inline void* const alphaUnderwaterEntityList = reinterpret_cast<void*>(0x01B64928);
inline void* const renderStateObj = reinterpret_cast<void*>(0x00D140B8);
inline void* const agrManager = reinterpret_cast<void*>(0x020C4910);
inline void* const obj_C2C730 = reinterpret_cast<void*>(0x00C2C730);

// the model infos
GAME_FN(uint8_t, __thiscall, MI_IsAvailable, 0x007FFDC0, uint8_t* mi);        // (always 1)
GAME_FN(float, __thiscall, MI_GetLargestLodDistance, 0x00527420, uint8_t* mi);
GAME_FN(void*, __thiscall, MI_GetAtomicFromDistance, 0x005273E0, uint8_t* mi, float dist);
GAME_FN(void, __thiscall, MI_FadeIn, 0x00450BC0, uint8_t* mi);
GAME_FN(void, __thiscall, MI_FadeOut, 0x00450BE0, uint8_t* mi);
GAME_FN(int16_t, __thiscall, PAnimMI_GetAnimIndex, 0x00520370, uint8_t* mi);
GAME_FN(uint8_t, __thiscall, PAnimMI_520430, 0x00520430, uint8_t* mi);
GAME_FN(float, __thiscall, PAnimMI_GetStreamDistance, 0x00520320, uint8_t* mi);
// the entities
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreenLarge, 0x00467520, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, Entity_IsVisibleAndOnScreen, 0x004686C0, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, Entity_467590, 0x00467590, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, Entity_IsOccluded, 0x0051DDB0, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, Entity_IsInCurrentArea, 0x004514E0, uint8_t* entity);
GAME_FN(int, __cdecl, SetupEntityVisibilityFn, 0x00451660, uint8_t* entity);
GAME_FN(void*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, int slot);
GAME_FN(void, __thiscall, PtrNode_RemoveFromList, 0x0044DF30, uint32_t* node, re::Ptr32<uint32_t>* list);
GAME_FN(void, __cdecl, SortBIGBuildingsForSectorListFn, 0x004515B0, re::Ptr32<uint32_t>* list);
GAME_FN(void, __cdecl, SortBIGBuildingsFn, 0x00452A80);
// the streaming
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);   // +4 the load state (1 loaded)
GAME_FN(void*, __cdecl, GetAgrGroup, 0x0052DB50, int id, int* entry);
GAME_FN(void, __thiscall, AgrManager_6BD010, 0x006BD010, void* manager, void* group, int entry);
GAME_FN(void, __thiscall, AgrGroup_6B56B0, 0x006B56B0, void* group, int entry, int);
GAME_FN(uint8_t, __cdecl, IsAnimLoaded4151E0, 0x004151E0, int id);
GAME_FN(uint8_t, __cdecl, Anim4151C0, 0x004151C0, int id);
GAME_FN(uint8_t, __cdecl, IsModelLoadingFn, 0x00450C00, int id);
// Gamebryo stand-ins (the RenderWare calls' leftovers: 0x824EC0 returns 0, 0x6CA7D0 its first argument)
GAME_FN(void*, __cdecl, Atomic_GetGeometry, 0x00824EC0, void* atomic);
GAME_FN(int, __cdecl, Geometry_Destroy, 0x004514C0, void* geometry);
GAME_FN(void*, __cdecl, Atomic_SetGeometry, 0x006CA7D0, void* atomic, void* geometry, int);
GAME_FN(void, __thiscall, Anim_6C0970, 0x006C0970, void* manager, void* anim, float, const float* offset, int, int);
GAME_FN(void*, __cdecl, Camera_GetFrame, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, Frame_GetLTM, 0x006C9A90, void* frame);   // +0x24 the position
GAME_FN(void*, __cdecl, Object_40F7A0, 0x0040F7A0, void* object);
GAME_FN(uint8_t, __thiscall, Object_40E230, 0x0040E230, void* self, void* object);
// the sorted lists
GAME_FN(uint8_t, __cdecl, InsertEntityIntoSortedList, 0x005F33F0, uint8_t* entity, float dist);
GAME_FN(uint8_t, __cdecl, InsertEntityIntoSortedList2, 0x005F3440, uint8_t* entity, float dist);
GAME_FN(void, __cdecl, RenderFadingList, 0x005F3470, void* list);
GAME_FN(void, __thiscall, LinkList_Init, 0x005F2F00, void* list, int size);
// the rest
GAME_FN(uint8_t, __cdecl, IsTimeInRange, 0x00425E40, int fromHours, int toHours, int fromMinutes, int toMinutes);
GAME_FN(uint8_t, __cdecl, IsPlayerInFastVehicle, 0x004469A0);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const float* v);
GAME_FN(float, __cdecl, SqrtAbs, 0x0040DF90, float value);   // sqrt(|value|)
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
GAME_FN(void, __thiscall, Obj_C2C730_4A77B0, 0x004A77B0, void* self);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

const float* EntityPos(uint8_t* e) {
    uint8_t* matrix = At<uint8_t*>(e, E_MATRIX);
    return matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(e + E_POSITION);
}

void* ModelObject(uint8_t* mi) {   // vf5
    using Fn = void*(__thiscall*)(uint8_t*);
    return re::GameFnAt<Fn>(At<uint8_t*>(mi, 0) + 0x14)(mi);
}

void CreateRwObject(uint8_t* e) {   // vf10 (1, 1)
    using Fn = void(__thiscall*)(uint8_t*, int, int);
    re::GameFnAt<Fn>(At<uint8_t*>(e, 0) + 0x28)(e, 1, 1);
}

bool InChapter(uint8_t* mi) {
    return (mi[MI_CHAPTERS] & uint8_t(1 << (currentChapter & 0x1F))) != 0;
}

// the area test (0x4514E0, inlined in some)
bool IsInCurrentArea(uint8_t* e) {
    uint8_t area = e[E_AREA];
    return area == currArea || area == 1 || (bSecondAreaVisible && area == secondArea);
}

// a time model's hours (the season's night when -1)
uint8_t IsTimeModelOn(uint8_t* mi) {
    int from = At<int32_t>(mi, MI_TIMEON);
    int season = chapterSeason[currentChapter];
    int to = At<int32_t>(mi, MI_TIMEOFF);
    if (from == -1) {
        from = int(seasonNightStart[season]);
    }
    if (to == -1) {
        to = int(seasonNightEnd[season]);
    }
    return IsTimeInRange(from, to, 0, 0);
}

// the new LOD atomic's geometry (stand-ins)
void SwapGeometry(void* rwObject, void* atomic, bool checkOld) {
    void* old = Atomic_GetGeometry(rwObject);
    if (Atomic_GetGeometry(atomic) != old) {
        if (!checkOld || Atomic_GetGeometry(rwObject)) {
            Geometry_Destroy(Atomic_GetGeometry(rwObject));
        }
        Atomic_SetGeometry(rwObject, Atomic_GetGeometry(atomic), 1);
    }
}

// ---- small ones -------------------------------------------------------------------------

// 0x450BC0 (thiscall): the model's alpha up by 16
void __fastcall MI_FadeInImpl(uint8_t* mi, re::Edx) {
    uint8_t alpha = mi[MI_ALPHA];
    mi[MI_ALPHA] = alpha < 0xEF ? uint8_t(alpha + 0x10) : uint8_t(0xFF);
}

// 0x450BE0 (thiscall): down by 16, to 0
void __fastcall MI_FadeOutImpl(uint8_t* mi, re::Edx) {
    uint8_t alpha = mi[MI_ALPHA];
    mi[MI_ALPHA] = uint8_t(alpha - (alpha < 0x10 ? alpha : 0x10));
}

// 0x450C00: a model on its way in (load states 3 and 4)
uint8_t __cdecl IsModelLoading(int id) {
    if (GetStreamingInfo(id)[4] != 3 && GetStreamingInfo(id)[4] != 4) {
        return 0;
    }
    return 1;
}

// 0x4514C0: (the geometry's destroy: freed)
int __cdecl GeometryDestroyImpl(void* geometry) {
    Free(geometry);
    return 1;
}

// 0x4514E0 (thiscall): in the area shown (1: everywhere)
uint8_t __fastcall Entity_IsInCurrentAreaImpl(uint8_t* e, re::Edx) {
    return IsInCurrentArea(e);
}

// 0x451510
void __cdecl Shutdown() {
    Free(pBuffer_C13F68);
    pBuffer_C13F68 = nullptr;
}

// 0x450C30: the fading entities (the alpha list)
void __cdecl RenderFadingEntities() {
    SetRenderState(0xE, 1);
    EmptyThiscall(renderStateObj);
    EmptyThiscall(renderStateObj);
    RenderFadingList(alphaEntityList);
}

// 0x450C60: the underwater ones
void __cdecl RenderFadingUnderwaterEntities() {
    SetRenderState(0xE, 1);
    EmptyThiscall(renderStateObj);
    EmptyThiscall(renderStateObj);
    RenderFadingList(alphaUnderwaterEntityList);
}

// 0x451530: every visible entity's PreRender (vf17), then the fading ones'
void __cdecl PreRender() {
    for (int i = 0; i < nNoOfVisibleEntities; i++) {
        uint8_t* e = aVisibleEntityPtrs[i];
        (*reinterpret_cast<re::Ptr32<void __thiscall(uint8_t*)>*>(At<uint8_t*>(e, 0) + 0x44))(e);
    }
    for (int i = 0; i < nNoOfVisibleLods; i++) {
        uint8_t* e = aVisibleLodPtrs[i];
        (*reinterpret_cast<re::Ptr32<void __thiscall(uint8_t*)>*>(At<uint8_t*>(e, 0) + 0x44))(e);
    }
    Obj_C2C730_4A77B0(obj_C2C730);
    // (a CLinkList: head at +0, its next at +0xC; tail at +0x10; a link's item first)
    for (uint8_t* link = At<uint8_t*>(alphaEntityList, 0xC); link != alphaEntityList + 0x10; link = At<uint8_t*>(link, 0xC)) {
        uint8_t* e = At<uint8_t*>(link, 0);
        (*reinterpret_cast<re::Ptr32<void __thiscall(uint8_t*)>*>(At<uint8_t*>(e, 0) + 0x44))(e);
    }
}

// 0x4515B0: a sector list's big buildings (+0x58) to its front
void __cdecl SortBIGBuildingsForSectorList(re::Ptr32<uint32_t>* list) {
    uint32_t* node = *list;
    while (node) {
        void* e = GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF);
        if (At<uint32_t>(e, E_BIGBUILDING)) {
            uint32_t nextIndex = *node >> 18;
            uint32_t* next = nextIndex == 0x3FFF ? nullptr : &ptrNodes[nextIndex];
            PtrNode_RemoveFromList(node, list);
            if (!*list) {
                *node |= 0xFFFC0000;
            } else {
                uint32_t index = uint16_t((re::PtrCast<intptr_t>(*list) - re::PtrCast<intptr_t>(ptrNodes)) >> 2);
                *node = index << 18 | (*node & 0x3FFFF);
            }
            *list = node;
            node = next;
        } else {
            uint32_t nextIndex = *node >> 18;
            if (nextIndex == 0x3FFF) {
                return;
            }
            node = &ptrNodes[nextIndex];
        }
    }
}

// 0x452A80: every sector's buildings list
void __cdecl SortBIGBuildings() {
    uint8_t* sector = sectors;
    do {
        for (int x = NUM_SECTORS_X; x != 0; x--) {
            SortBIGBuildingsForSectorListFn(reinterpret_cast<re::Ptr32<uint32_t>*>(sector));
            sector += SECTOR_SIZE;
        }
    } while (sector < sectors + NUM_SECTORS_X * NUM_SECTORS_Y * SECTOR_SIZE);
}

// 0x452AC0 (thiscall): CLinkList::Clear, every used link to the free list
void __fastcall LinkList_Clear(uint8_t* list, re::Edx) {
    uint8_t* tail = list + 0x10;
    uint8_t* freeHead = list + 0x20;
    while (At<uint8_t*>(list, 0xC) != tail) {
        uint8_t* link = At<uint8_t*>(list, 0xC);
        At<uint8_t*>(At<uint8_t*>(link, 0xC), 8) = At<uint8_t*>(link, 8);
        At<uint8_t*>(At<uint8_t*>(link, 8), 0xC) = At<uint8_t*>(link, 0xC);
        At<uint8_t*>(link, 0xC) = At<uint8_t*>(freeHead, 0xC);
        At<uint8_t*>(At<uint8_t*>(freeHead, 0xC), 8) = link;
        At<uint8_t*>(link, 8) = freeHead;
        At<uint8_t*>(freeHead, 0xC) = link;
    }
}

// 0x452B00: the fading list (40 links), the big buildings sorted
void __cdecl Init() {
    LinkList_Init(linkList_C13F28, 40);
    SortBIGBuildingsFn();
}

// ---- ScanSectorPoly ---------------------------------------------------------------------

using ScanFunc = void(__cdecl*)(void* sector);

// 0x450C90: every sector the polygon (x, y points in sector units) covers, a row at a
// time, to `scan`
void __cdecl ScanSectorPoly(const float* poly, int numVertices, ScanFunc scan) {
    auto X = [poly](int i) { return poly[i * 2]; };
    auto Y = [poly](int i) { return poly[i * 2 + 1]; };
    float miny = Y(0);
    float maxy = Y(0);
    int a2 = 0;
    int xstart = 9999;
    int xend = -9999;
    for (int i = 1; i < numVertices; i++) {
        if (Y(i) < miny) {
            miny = Y(i);
            a2 = i;
        } else if (maxy < Y(i)) {
            maxy = Y(i);
        }
    }
    int y = int(miny);
    int yend = int(maxy);

    // left: back from the lowest point to the first edge that changes row
    int b1, b2 = a2;
    for (int i = 1;; i++) {
        b1 = b2--;
        if (b2 < 0) {
            b2 = numVertices - 1;
        }
        if (double(X(b1)) < double(xstart)) {
            xstart = int(X(b1));
        }
        if (int(Y(b1)) != int(Y(b2)) || i >= numVertices) {
            break;
        }
    }
    // right: forward
    int a1;
    for (int i = 1;; i++) {
        a1 = a2++;
        if (a2 == numVertices) {
            a2 = 0;
        }
        if (double(xend) < double(X(a1))) {
            xend = int(X(a1));
        }
        if (int(Y(a1)) != int(Y(a2)) || i >= numVertices) {
            break;
        }
    }

    // each edge's x stepped to the next whole row
    float ceilA = float(std::ceil(double(Y(a1))));
    float deltaA = (X(a2) - X(a1)) / (Y(a2) - Y(a1));
    float xA = (ceilA - Y(a1)) * deltaA + X(a1);
    float ceilB = float(std::ceil(double(Y(b1))));
    float deltaB = (X(b2) - X(b1)) / (Y(b2) - Y(b1));
    float xB = deltaB * (ceilB - Y(b1)) + X(b1);
    if (y != yend) {
        if (deltaB < 0.0f && xstart > int(xB)) {
            xstart = int(xB);
        }
        if (deltaA >= 0.0f && xend < int(xA)) {
            xend = int(xA);
        }
    }

    while (y <= yend && y < NUM_SECTORS_Y) {
        if (y >= 0 && xstart < NUM_SECTORS_X) {
            for (int x = xstart; x <= xend && x != NUM_SECTORS_X; x++) {
                if (x >= 0) {
                    scan(sectors + (y * NUM_SECTORS_X + x) * SECTOR_SIZE);
                }
            }
        }
        y++;
        xA += deltaA;
        xB += deltaB;

        // the left side
        if (y == int(Y(b2))) {
            if (y == yend) {
                if (deltaB < 0.0f) {
                    do {
                        xstart = int(X(b2));
                        b2--;
                        if (b2 < 0) {
                            b2 = numVertices - 1;
                        }
                    } while (xstart > int(X(b2)));
                } else {
                    xstart = int(xB - deltaB);
                }
            } else {
                // the next edge
                if (deltaB < 0.0f) {
                    xstart = int(X(b2));
                } else {
                    xstart = int(xB - deltaB);
                }
                float x1;
                do {
                    b1 = b2--;
                    if (b2 < 0) {
                        b2 = numVertices - 1;
                    }
                    x1 = X(b1);
                    if (xstart > int(x1)) {
                        xstart = int(x1);
                    }
                } while (y == int(Y(b2)));
                deltaB = (X(b2) - X(b1)) / (Y(b2) - Y(b1));
                float t = float(std::ceil(double(Y(b1)))) - Y(b1);
                xB = deltaB * t + x1;
                if (deltaB < 0.0f && xstart > int(xB)) {
                    xstart = int(xB);
                }
            }
        } else {
            xstart = deltaB < 0.0f ? int(xB) : int(xB - deltaB);
        }

        // the right side
        if (y == int(Y(a2))) {
            if (y == yend) {
                if (!(deltaA >= 0.0f)) {
                    xend = int(xA - deltaA);
                } else {
                    do {
                        xend = int(X(a2));
                        a2++;
                        if (a2 == numVertices) {
                            a2 = 0;
                        }
                    } while (xend < int(X(a2)));
                }
            } else {
                if (!(deltaA >= 0.0f)) {
                    xend = int(xA - deltaA);
                } else {
                    xend = int(X(a2));
                }
                float x1;
                do {
                    a1 = a2++;
                    if (a2 == numVertices) {
                        a2 = 0;
                    }
                    x1 = X(a1);
                    if (xend < int(x1)) {
                        xend = int(x1);
                    }
                } while (y == int(Y(a2)));
                deltaA = (X(a2) - X(a1)) / (Y(a2) - Y(a1));
                float t = float(std::ceil(double(Y(a1)))) - Y(a1);
                xA = deltaA * t + x1;
                if (deltaA >= 0.0f && xend < int(xA)) {
                    xend = int(xA);
                }
            }
        } else {
            xend = !(deltaA >= 0.0f) ? int(xA - deltaA) : int(xA);
        }
    }
}

// ---- the visibility tests -----------------------------------------------------------------

// 0x451660
int __cdecl SetupEntityVisibility(uint8_t* e) {
    uint8_t* mi = modelInfoPtrs[At<int16_t>(e, E_MODEL)];
    uint8_t type = mi[MI_TYPE];
    uint8_t timeVisible = 1;
    if ((type == 1 || type == 3) && !MI_IsAvailable(mi)) {
        return VIS_INVISIBLE;
    }
    if (!InChapter(mi)) {
        if ((e[E_TYPE] & 7) != 3) {
            Entity_DeleteRwObject(e);
        }
        return VIS_INVISIBLE;
    }
    if (!At<uint32_t>(e, E_FLAG48)) {
        return VIS_INVISIBLE;
    }
    const float* pos = EntityPos(e);
    float dx = pos[0] - vecCameraPosition[0];
    float dy = pos[1] - vecCameraPosition[1];
    float dz = pos[2] - vecCameraPosition[2];
    float distSq = dy * dy + dx * dx + dz * dz;
    void* rw = At<void*>(e, E_RWOBJECT);

    if (type == 3) {
        int other = At<int32_t>(mi, MI_OTHERMODEL);
        if (!IsTimeModelOn(mi)) {
            if (other == -1 || ModelObject(modelInfoPtrs[other])) {
                Entity_DeleteRwObject(e);
                return VIS_INVISIBLE;
            }
            timeVisible = 0;
        } else if (other != -1 && ModelObject(modelInfoPtrs[other])) {
            mi[MI_ALPHA] = 0xFF;
        }
    } else if (type == 10) {
        // a prop animation: its animation streamed in near the camera
        uint8_t* pmi = modelInfoPtrs[At<int16_t>(e, E_MODEL)];
        int anim = PAnimMI_GetAnimIndex(pmi);
        if (anim != -1 && !IsPlayerInFastVehicle()) {
            int entry;
            if (!IsAnimLoaded4151E0(anim) && !Anim4151C0(anim) && !IsModelLoadingFn(anim)) {
                if (distSq < 299.29f) {
                    void* group = GetAgrGroup(anim, &entry);
                    if (group) {
                        RequestModel(anim, 0xC);
                        AgrManager_6BD010(agrManager, group, entry);
                        AgrGroup_6B56B0(group, entry, 0x14);
                    }
                }
            } else if (distSq < 299.29f && IsAnimLoaded4151E0(anim)) {
                void* group = GetAgrGroup(anim, &entry);
                if (group) {
                    AgrManager_6BD010(agrManager, group, entry);
                    AgrGroup_6B56B0(group, entry, 0x14);
                }
            }
        }
        if (PAnimMI_520430(pmi)) {
            return VIS_INVISIBLE;
        }
        float streamDist = PAnimMI_GetStreamDistance(pmi);
        float dist = SqrtAbs(distSq);
        if (At<void*>(pmi, MI_RELATED)) {
            if (!rw) {
                CreateRwObject(e);
            }
            if (Entity_GetIsOnScreenLarge(e) && !Entity_IsOccluded(e)) {
                At<uint32_t>(e, E_FLAG98) = 0;
                return VIS_VISIBLE;
            }
            return VIS_OFFSCREEN;
        }
        if (double(dist) - 30.0 < double(streamDist)) {
            RequestModel(At<int16_t>(e, E_MODEL), 0xC);
        }
        return VIS_OFFSCREEN;
    } else if (type == 4) {
        if (!rw) {
            return VIS_INVISIBLE;
        }
        if (!Entity_GetIsOnScreen(e) || Entity_IsOccluded(e)) {
            return VIS_OFFSCREEN;
        }
        float drawDist = At<float>(mi, MI_WEAPONDIST);
        if (distSq > drawDist * drawDist) {
            return VIS_OFFSCREEN;
        }
        if (!At<uint32_t>(e, E_FLAG8C)) {
            return VIS_VISIBLE;
        }
        InsertEntityIntoSortedList(e, SqrtAbs(distSq));
        At<uint32_t>(e, E_FLAG98) = 0;
        return VIS_INVISIBLE;
    } else if (type != 1 || At<uint32_t>(e, E_FLAGB4)) {
        if (!rw) {
            return VIS_INVISIBLE;
        }
        if (!Entity_GetIsOnScreen(e) || Entity_IsOccluded(e)) {
            return VIS_OFFSCREEN;
        }
        if (!At<uint32_t>(e, E_FLAG8C)) {
            return VIS_VISIBLE;
        }
        InsertEntityIntoSortedList(e, SqrtAbs(distSq));
        At<uint32_t>(e, E_FLAG98) = 0;
        return VIS_INVISIBLE;
    }

    // simple and time models: their LODs by distance, faded in and out
    if (!Entity_IsInCurrentArea(e)) {
        return VIS_INVISIBLE;
    }
    if (At<uint32_t>(mi, MI_FLAGS) & 0x1000000) {
        return VIS_INVISIBLE;
    }
    float lodDist = MI_GetLargestLodDistance(mi);
    float dist = SqrtAbs(distSq);
    if (dist > 330.0f && lodDist > dist) {
        dist = lodDist;
    }
    void* atomic = MI_GetAtomicFromDistance(mi, dist);
    if (dist < At<float>(e, E_LODFADE)) {
        if (!atomic || (At<uint32_t>(mi, MI_FLAGS) & 0x20) || mi[MI_ALPHA] == 0) {
            return VIS_INVISIBLE;
        }
        MI_FadeOut(mi);
        InsertEntityIntoSortedList(e, dist);
        At<uint32_t>(e, E_FLAG98) = 1;
        return VIS_INVISIBLE;
    }
    if (atomic) {
        if (!rw) {
            CreateRwObject(e);
        }
        void* rwObject = At<void*>(e, E_RWOBJECT);
        if (!rwObject) {
            return VIS_INVISIBLE;
        }
        SwapGeometry(rwObject, atomic, true);
        if (!(At<uint32_t>(mi, MI_FLAGS) & 0x80)) {
            MI_FadeIn(mi);
        } else {
            mi[MI_ALPHA] = 0xFF;
        }
        if (!Entity_GetIsOnScreen(e) || Entity_IsOccluded(e)) {
            mi[MI_ALPHA] = 0xFF;
            return VIS_OFFSCREEN;
        }
        if (mi[MI_ALPHA] != 0xFF) {
            InsertEntityIntoSortedList(e, dist);
            At<uint32_t>(e, E_FLAG98) = 1;
            return VIS_INVISIBLE;
        }
        uint32_t flags = At<uint32_t>(mi, MI_FLAGS);
        if (!(flags & 0x40) && !At<uint32_t>(e, E_FLAG8C)) {
            if (!(flags & 0x40000)) {
                return VIS_VISIBLE;
            }
            InsertEntityIntoSortedList2(e, dist);
            return VIS_INVISIBLE;
        }
        InsertEntityIntoSortedList(e, dist);
        At<uint32_t>(e, E_FLAG98) = 0;
        return VIS_INVISIBLE;
    }
    uint32_t flags = At<uint32_t>(mi, MI_FLAGS);
    if (flags & 0x20) {
        if ((flags & 0x2000000) && IsPlayerInFastVehicle()) {
            return VIS_INVISIBLE;
        }
        float nearer = dist - 30.0f;
        if (nearer < lodDist && timeVisible) {
            return VIS_STREAMME;
        }
    } else {
        void* nearAtomic = MI_GetAtomicFromDistance(mi, dist - 20.0f);
        if (nearAtomic) {
            if (!rw) {
                CreateRwObject(e);
            }
            void* rwObject = At<void*>(e, E_RWOBJECT);
            if (!rwObject) {
                return VIS_INVISIBLE;
            }
            SwapGeometry(rwObject, nearAtomic, false);
            MI_FadeIn(mi);
            if (Entity_GetIsOnScreen(e) && !Entity_IsOccluded(e)) {
                InsertEntityIntoSortedList(e, dist);
                At<uint32_t>(e, E_FLAG98) = 1;
                return VIS_OFFSCREEN;
            }
            mi[MI_ALPHA] = 0xFF;
            return VIS_INVISIBLE;
        }
        float nearer = dist - 50.0f;
        if (nearer < lodDist && timeVisible) {
            if ((At<uint32_t>(mi, MI_FLAGS) & 0x2000000) && IsPlayerInFastVehicle()) {
                return VIS_INVISIBLE;
            }
            return VIS_STREAMME;
        }
    }
    if (bSecondAreaForced && e[E_AREA] == secondArea) {
        mi[MI_ALPHA] = 0xFF;
        return VIS_STREAMME;
    }
    MI_GetLargestLodDistance(mi);   // (the result unused)
    return VIS_INVISIBLE;
}

// 0x451E70: a model to stream in for a point (the camera to be): in its area and
// chapter, on at the time, within its LOD distance (and 30 or 50)
uint8_t __cdecl ShouldModelBeStreamed(uint8_t* e, const float* point) {
    if (!IsInCurrentArea(e)) {
        return 0;
    }
    uint8_t* mi = modelInfoPtrs[At<int16_t>(e, E_MODEL)];
    if ((mi[MI_TYPE] == 1 || mi[MI_TYPE] == 3) && !MI_IsAvailable(mi)) {
        return 0;
    }
    if (!InChapter(mi)) {
        return 0;
    }
    if (mi[MI_TYPE] == 3 && !IsTimeModelOn(mi)) {
        return 0;
    }
    const float* pos = EntityPos(e);
    float d[3] = {pos[0], pos[1], pos[2]};
    float p[3] = {point[0], point[1], point[2]};
    d[0] = d[0] - p[0];
    d[1] = d[1] - p[1];
    d[2] = d[2] - p[2];
    float dist = Vector_Magnitude(d);
    if (mi[MI_FLAGS] & 0x20) {
        float lod = MI_GetLargestLodDistance(mi);
        return double(dist) - 30.0 < double(lod);
    }
    float lod = MI_GetLargestLodDistance(mi);
    return double(dist) - 50.0 < double(lod);
}

// 0x452000: a big building (a LOD, drawn from far): visible while its related (HD)
// model isn't
int __cdecl SetupBigBuildingVisibility(uint8_t* e) {
    uint8_t* mi = modelInfoPtrs[At<int16_t>(e, E_MODEL)];
    uint8_t timeVisible = 1;
    if (!IsInCurrentArea(e)) {
        return VIS_INVISIBLE;
    }
    if (At<uint32_t>(mi, MI_FLAGS) & 0x1000000) {
        return VIS_INVISIBLE;
    }
    if ((mi[MI_TYPE] == 1 || mi[MI_TYPE] == 3) && !MI_IsAvailable(mi)) {
        return VIS_INVISIBLE;
    }
    if (!InChapter(mi)) {
        return VIS_INVISIBLE;
    }
    if (mi[MI_TYPE] == 3) {
        int other = At<int32_t>(mi, MI_OTHERMODEL);
        if (!IsTimeModelOn(mi)) {
            if (other == -1 || ModelObject(modelInfoPtrs[other])) {
                Entity_DeleteRwObject(e);
                return VIS_INVISIBLE;
            }
            timeVisible = 0;
        } else if (other != -1 && ModelObject(modelInfoPtrs[other])) {
            mi[MI_ALPHA] = 0xFF;
        }
    } else if (mi[MI_TYPE] == 7) {
        return Entity_IsVisibleAndOnScreen(e) ? VIS_VISIBLE : VIS_INVISIBLE;
    }
    const float* pos = EntityPos(e);
    float d[3] = {pos[0], pos[1], pos[2]};
    d[0] = d[0] - vecCameraPosition[0];
    d[1] = d[1] - vecCameraPosition[1];
    d[2] = d[2] - vecCameraPosition[2];
    float dist = Vector_Magnitude(d);
    if (MI_GetLargestLodDistance(mi) > dist && 330.0f > dist) {
        // near enough for the related model: none while it's drawn
        uint8_t* related = At<uint8_t*>(mi, MI_RELATED);
        if (!related) {
            return VIS_INVISIBLE;
        }
        if (ModelObject(related) && related[MI_ALPHA] == 0xFF) {
            return VIS_INVISIBLE;
        }
        if (related[MI_TYPE] == 3) {
            int other = At<int32_t>(related, MI_OTHERMODEL);
            if (other != -1 && ModelObject(modelInfoPtrs[other])) {
                return VIS_INVISIBLE;
            }
        }
    }
    void* atomic = MI_GetAtomicFromDistance(mi, dist);
    bool fadeInOnly = false;
    if (!atomic) {
        atomic = MI_GetAtomicFromDistance(mi, dist - 20.0f);
        fadeInOnly = true;
    }
    if (atomic) {
        if (!At<void*>(e, E_RWOBJECT)) {
            CreateRwObject(e);
        }
        void* rwObject = At<void*>(e, E_RWOBJECT);
        void* old = Atomic_GetGeometry(rwObject);
        if (Atomic_GetGeometry(atomic) != old) {
            Atomic_SetGeometry(rwObject, Atomic_GetGeometry(atomic), 1);
        }
        MI_FadeIn(mi);
        if (!Entity_IsVisibleAndOnScreen(e) || !Entity_467590(e) || Entity_IsOccluded(e)) {
            mi[MI_ALPHA] = 0xFF;
            return VIS_INVISIBLE;
        }
        if (fadeInOnly || mi[MI_ALPHA] != 0xFF) {
            InsertEntityIntoSortedList(e, dist);
            At<uint32_t>(e, E_FLAG98) = 1;
            return VIS_INVISIBLE;
        }
        uint32_t flags = At<uint32_t>(mi, MI_FLAGS);
        if (flags & 0x40) {
            InsertEntityIntoSortedList(e, dist);
            At<uint32_t>(e, E_FLAG98) = 0;
            return VIS_INVISIBLE;
        }
        if (flags & 0x40000) {
            InsertEntityIntoSortedList2(e, dist);
            return VIS_INVISIBLE;
        }
        return VIS_VISIBLE;
    }
    if (At<uint32_t>(e, E_FLAG5C)) {
        float lod = MI_GetLargestLodDistance(mi);
        if (double(dist) - 50.0 < double(lod) && timeVisible) {
            return Entity_GetIsOnScreen(e) ? VIS_STREAMME : VIS_INVISIBLE;
        }
    }
    Entity_DeleteRwObject(e);
    return VIS_INVISIBLE;
}

// ---- the sector scans -------------------------------------------------------------------

void AddVisibleEntity(uint8_t* e) {
    *pVisibleEntityWrite++ = e;
    nNoOfVisibleEntities++;
}

void AddVisibleLod(uint8_t* e) {
    if (nNoOfVisibleLods < 0x95) {
        *pVisibleLodWrite++ = e;
        nNoOfVisibleLods++;
    }
}

// off screen but within 30 units (x, y) of the camera: kept for their shadows and
// collision (the lod list, or the visible one for some atomics)
void AddNearOffscreen(uint8_t* e) {
    const float* pos = EntityPos(e);
    float dx = vecCameraPosition[0] - pos[0];
    if (!(-30.0f < dx) || !(dx < 30.0f)) {
        return;
    }
    pos = EntityPos(e);
    float dy = vecCameraPosition[1] - pos[1];
    if (!(-30.0f < dy) || !(dy < 30.0f)) {
        return;
    }
    uint8_t* rw = At<uint8_t*>(e, E_RWOBJECT);
    if (rw && rw[0] == 1 && nNoOfVisibleEntities < 0x7CF) {
        void* object = At<void*>(rw, 0x14);
        if (Object_40E230(Object_40F7A0(object), object)) {
            AddVisibleEntity(e);
            return;
        }
    }
    AddVisibleLod(e);
}

// the player (ped type 13) is 1 opaque, 2 while faded; the rest by SetupEntityVisibility
int VisibilityOf(uint8_t* e) {
    if ((e[E_TYPE] & 7) == 3 && At<int32_t>(e, 0x1310) == 0xD) {
        return e[0x1DE7] < 0xFF ? 2 : 1;
    }
    return SetupEntityVisibilityFn(e);
}

// 0x452430: a sector's five lists into the visible lists (each entity once a frame)
void __cdecl ScanSectorList(re::Ptr32<uint32_t>* lists) {
    uint8_t inInterior = bCameraInInterior;
    uint16_t scanCode = nCurrentScanCode2;
    for (int i = 5; i != 0; i--, lists++) {
        uint32_t* node = *lists;
        while (node) {
            auto* e = static_cast<uint8_t*>(GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF));
            uint32_t nextIndex = *node >> 18;
            node = nextIndex == 0x3FFF ? nullptr : &ptrNodes[nextIndex];
            if (At<uint16_t>(e, E_SCANCODE2) == scanCode) {
                continue;
            }
            At<uint16_t>(e, E_SCANCODE2) = scanCode;
            uint32_t wasOffscreen = At<uint32_t>(e, E_OFFSCREEN);
            At<uint32_t>(e, E_OFFSCREEN) = 0;
            At<uint32_t>(e, E_WASVISIBLE) = 0;
            int vis = VisibilityOf(e);
            if (inInterior) {
                // (outside the interior's heights: nothing)
                const float* pos = EntityPos(e);
                uint8_t* ltm = Frame_GetLTM(Camera_GetFrame(camera));
                float h = pos[2] - At<float>(ltm, 0x2C);
                if (At<float>(interiorMaxZ, cameraInterior * 0x60) < h || h < At<float>(interiorMinZ, cameraInterior * 0x60)) {
                    vis = VIS_INVISIBLE;
                }
            }
            switch (vis) {
            case VIS_VISIBLE:
                AddVisibleEntity(e);
                [[fallthrough]];
            case VIS_VISIBLE_UNSORTED:
                if (wasOffscreen) {
                    At<uint32_t>(e, E_WASVISIBLE) = 1;
                    uint8_t* anim = At<uint8_t*>(e, E_ANIM);
                    if (anim && At<void*>(anim, 0x14)) {
                        Anim_6C0970(agrManager, anim, 0.0f, nullptr, 1, 1);
                    }
                }
                break;
            case VIS_OFFSCREEN:
                At<uint32_t>(e, E_OFFSCREEN) = inInterior == 0;
                AddNearOffscreen(e);
                break;
            case VIS_STREAMME:
                if (!bDisableStreaming && (!bLoadingPriority || numModelsRequested < 10)) {
                    RequestModel(At<int16_t>(e, E_MODEL), 0);
                }
                break;
            default:
                break;
            }
        }
    }
}

// 0x452770: ScanSectorList for the sectors nearest the camera: models streamed with
// priority (m_loadingPriority when one isn't loaded yet), no interior height test
void __cdecl ScanSectorList_Priority(re::Ptr32<uint32_t>* lists) {
    uint8_t inInterior = bCameraInInterior;
    uint16_t scanCode = nCurrentScanCode2;
    for (int i = 5; i != 0; i--, lists++) {
        uint32_t* node = *lists;
        while (node) {
            auto* e = static_cast<uint8_t*>(GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF));
            uint32_t nextIndex = *node >> 18;
            node = nextIndex == 0x3FFF ? nullptr : &ptrNodes[nextIndex];
            if (At<uint16_t>(e, E_SCANCODE2) == scanCode) {
                continue;
            }
            At<uint16_t>(e, E_SCANCODE2) = scanCode;
            uint32_t wasOffscreen = At<uint32_t>(e, E_OFFSCREEN);
            At<uint32_t>(e, E_OFFSCREEN) = 0;
            At<uint32_t>(e, E_WASVISIBLE) = 0;
            switch (VisibilityOf(e)) {
            case VIS_VISIBLE:
                AddVisibleEntity(e);
                [[fallthrough]];
            case VIS_VISIBLE_UNSORTED:
                if (wasOffscreen) {
                    uint8_t* anim = At<uint8_t*>(e, E_ANIM);
                    At<uint32_t>(e, E_WASVISIBLE) = 1;
                    if (anim && At<void*>(anim, 0x14)) {
                        // vf4: its animation raised by -(+0x35C)
                        using Fn = uint8_t(__thiscall*)(uint8_t*);
                        if (re::GameFnAt<Fn>(At<uint8_t*>(e, 0) + 0x10)(e)) {
                            float offset[3] = {0.0f, 0.0f, -At<float>(e, 0x35C)};
                            Anim_6C0970(agrManager, At<uint8_t*>(e, E_ANIM), 0.0f, offset, 1, 1);
                        } else {
                            Anim_6C0970(agrManager, At<uint8_t*>(e, E_ANIM), 0.0f, nullptr, 1, 1);
                        }
                    }
                }
                break;
            case VIS_OFFSCREEN:
                At<uint32_t>(e, E_OFFSCREEN) = inInterior == 0;
                AddNearOffscreen(e);
                break;
            case VIS_STREAMME:
                if (!bDisableStreaming) {
                    RequestModel(At<int16_t>(e, E_MODEL), 0);
                    if (GetStreamingInfo(At<int16_t>(e, E_MODEL))[4] != 1) {
                        bLoadingPriority = 1;
                    }
                }
                break;
            default:
                break;
            }
        }
    }
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define C04B_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04B_REVERSED("CBaseModelInfo::FadeIn", 0x00450BC0, &MI_FadeInImpl);
C04B_REVERSED("CBaseModelInfo::FadeOut", 0x00450BE0, &MI_FadeOutImpl);
C04B_REVERSED("CStreaming::IsModelLoading", 0x00450C00, &IsModelLoading);
C04B_REVERSED("CRenderer::RenderFadingEntities", 0x00450C30, &RenderFadingEntities);
C04B_REVERSED("CRenderer::RenderFadingUnderwaterEntities", 0x00450C60, &RenderFadingUnderwaterEntities);
C04B_REVERSED("CRenderer::ScanSectorPoly", 0x00450C90, &ScanSectorPoly);
C04B_REVERSED("Geometry::Destroy", 0x004514C0, &GeometryDestroyImpl);
C04B_REVERSED("CEntity::IsInCurrentArea", 0x004514E0, &Entity_IsInCurrentAreaImpl);
C04B_REVERSED("CRenderer::Shutdown", 0x00451510, &Shutdown);
C04B_REVERSED("CRenderer::PreRender", 0x00451530, &PreRender);
C04B_REVERSED("CRenderer::SortBIGBuildingsForSectorList", 0x004515B0, &SortBIGBuildingsForSectorList);
C04B_REVERSED("CRenderer::SetupEntityVisibility", 0x00451660, &SetupEntityVisibility);
C04B_REVERSED("CRenderer::ShouldModelBeStreamed", 0x00451E70, &ShouldModelBeStreamed);
C04B_REVERSED("CRenderer::SetupBigBuildingVisibility", 0x00452000, &SetupBigBuildingVisibility);
C04B_REVERSED("CRenderer::ScanSectorList", 0x00452430, &ScanSectorList);
C04B_REVERSED("CRenderer::ScanSectorList_Priority", 0x00452770, &ScanSectorList_Priority);
C04B_REVERSED("CRenderer::SortBIGBuildings", 0x00452A80, &SortBIGBuildings);
C04B_REVERSED("CLinkList::Clear", 0x00452AC0, &LinkList_Clear);
C04B_REVERSED("CRenderer::Init", 0x00452B00, &Init);
