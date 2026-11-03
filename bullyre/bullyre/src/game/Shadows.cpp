// CShadows, part 1 (0x525490..0x5273DF): shadows cast onto the collision of the world's
// entities (GTA's CastShadowEntity: the shadow's quad clipped to each collision triangle
// it overlaps, then either drawn at once through the render buffer or kept as a list of
// polygons, a "poly bunch"), the static shadows (20, kept between frames while their
// owner keeps storing them) and the ped shadow's sun values.
//
// Found here: the static shadows are GTA's CStaticShadow (0x40 each at 0xC93BE0), the
// poly bunches GTA's CPolyBunch (0x68, free list at 0xC93760). The clip works in two
// buffers of 10 points (0xC9DC60, their texture coordinates 0xC9DB70) that are
// function statics in the game (guard bits at 0xC9DD50), ping-ponged per edge.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Entity.h"
#include "reversed.h"

namespace shadows17 {

using col::CVector;

// A shadow kept as polygons (up to 7 points each), in a list
struct CPolyBunch {
    CVector verts[7];    // +0
    re::Ptr32<CPolyBunch> next;    // +0x54
    int16_t numVerts;    // +0x58
    uint8_t u[7];        // +0x5A texture coordinates * 200
    uint8_t v[7];        // +0x61
};
static_assert(offsetof(CPolyBunch, next) == 0x54);
static_assert(offsetof(CPolyBunch, u) == 0x5A);
static_assert(offsetof(CPolyBunch, v) == 0x61);
static_assert(sizeof(CPolyBunch) == 0x68);

struct CStaticShadow {
    uint32_t id;             // +0
    re::Ptr32<CPolyBunch> polyBunch;   // +4
    uint32_t timeCreated;    // +8
    CVector pos;             // +0xC
    float frontX;            // +0x18
    float frontY;            // +0x1C
    float sideX;             // +0x20
    float sideY;             // +0x24
    float zDistance;         // +0x28
    float scale;             // +0x2C
    re::Ptr32<void> texture;           // +0x30
    int16_t intensity;       // +0x34
    uint8_t type;            // +0x36
    uint8_t red;             // +0x37
    uint8_t green;           // +0x38
    uint8_t blue;            // +0x39
    uint8_t justCreated;     // +0x3A
    uint8_t rendered;        // +0x3B
    uint8_t temporary;       // +0x3C (freed 5 s after it stops being stored)
    uint8_t pad3D[3];
};
static_assert(offsetof(CStaticShadow, zDistance) == 0x28);
static_assert(offsetof(CStaticShadow, intensity) == 0x34);
static_assert(offsetof(CStaticShadow, temporary) == 0x3C);
static_assert(sizeof(CStaticShadow) == 0x40);

// the render buffer's vertex (GTA's RenderBuffer, 0xBD7800, 0xB32 of them)
struct BufferVertex {
    CVector pos;             // +0
    uint32_t unknownC[3];    // +0xC (not written by the shadows)
    uint32_t color;          // +0x18 ARGB
    float u;                 // +0x1C
    float v;                 // +0x20
};
static_assert(sizeof(BufferVertex) == 0x24);

// A sector's lists (0xC1B178, 36 x 36 of 20 bytes): the first holds the entities
struct CSector {
    re::Ptr32<uint32_t> firstNode;     // +0 packed nodes (see CastShadowSectorList)
    uint32_t unknown4[4];
};
static_assert(sizeof(CSector) == 0x14);

GAME_VAR(CPolyBunch*, pEmptyBunchList, 0x00C93760);
GAME_ARRAY(CStaticShadow, aStaticShadows, 0x00C93BE0);
GAME_ARRAY(uint16_t, ShadowIndexList, 0x00C93730);   // a triangle fan's indices
GAME_VAR(uint32_t, castStaticsGuard, 0x00C9DD50);
inline CVector (*const castList)[10] = reinterpret_cast<CVector (*)[10]>(0x00C9DC60);
inline CVector (*const castTexture)[10] = reinterpret_cast<CVector (*)[10]>(0x00C9DB70);
GAME_ARRAY(CVector, castPoints, 0x00C9DB40);          // the shadow's quad, in the entity's space
// CalcPedShadowValues' results
GAME_VAR(float, pedShadowDisplacementX, 0x00C93714);
GAME_VAR(float, pedShadowDisplacementY, 0x00C93710);
GAME_VAR(float, pedShadowFrontX, 0x00C9370C);
GAME_VAR(float, pedShadowFrontY, 0x00C93708);
GAME_VAR(float, pedShadowSideX, 0x00C93704);
GAME_VAR(float, pedShadowSideY, 0x00C93700);

GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);
GAME_ARRAY(CSector, sectors, 0x00C1B178);
GAME_VAR(uint32_t*, sectorNodes, 0x00C0F788);         // the packed nodes' array
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, extraAreaActive, 0x00BD09EC);
GAME_VAR(int8_t, extraArea, 0x00BD09EB);
GAME_VAR(uint8_t, cutsceneRunning, 0x020C5BE4);
GAME_VAR(void*, cameraNode, 0x00C3CCF4);               // TheCamera +0x8C
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(CColModel*, __cdecl, GetColModel, 0x0051AF50, CEntity* entity);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, CPlaceable* self, CVector* out);
GAME_FN(CVector*, __thiscall, Placeable_GetPosition, 0x00415460, CPlaceable* self);
GAME_FN(CVector*, __thiscall, SimpleTransform_GetRight, 0x00473600, CSimpleTransform* self, CVector* out);
GAME_FN(void, __thiscall, ColModel_GetVertex, 0x0056E680, CColModel* col, CVector* out, int index);
GAME_FN(void, __cdecl, CalcTriangleNormal, 0x005649C0, const CVector* a, const CVector* b, const CVector* c, CVector* normal);
// the collision's triangle tree (CCollisionData +0x14): the triangles near a sphere
GAME_FN(uint16_t, __thiscall, TriangleTree_FindFirst, 0x005617F0, void* tree, const CVector* center, float radius, int flag);
GAME_FN(uint16_t, __thiscall, TriangleTree_FindNext, 0x005618A0, void* tree);
GAME_FN(void, __cdecl, RenderBuffer_StartStoring, 0x0052AE10, int numIndices, int numVerts, re::Ptr32<uint16_t>* indices, re::Ptr32<BufferVertex>* verts, int* base);
GAME_FN(void, __cdecl, RenderBuffer_StopStoring, 0x0049A650);   // (empty in Bully)
GAME_FN(CEntity*, __cdecl, GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);   // (+0x24: the position)
GAME_FN(int, __thiscall, Camera_IsSphereVisible, 0x004F13D0, void* camera, const CVector* center, float radius);
GAME_FN(void, __thiscall, StaticShadow_Free, 0x00524FF0, CStaticShadow* shadow);
GAME_FN(void, __cdecl, CastShadowEntityXY_Game, 0x00525490, CEntity* entity, float startX, float startY, float endX, float endY,
        CVector* pos, float frontX, float frontY, float sideX, float sideY, int intensity, int red, int green, int blue,
        float zDistance, float scale, re::Ptr32<CPolyBunch>* ppPolyBunch);
GAME_FN(void, __cdecl, CastShadowSectorList_Game, 0x00526830, CSector* sector, float startX, float startY, float endX, float endY,
        CVector* pos, float frontX, float frontY, float sideX, float sideY, int intensity, int red, int green, int blue,
        float zDistance, float scale, re::Ptr32<CPolyBunch>* ppPolyBunch);
GAME_FN(void, __cdecl, GeneratePolysForStaticShadow_Game, 0x00526B00, int index);
GAME_FN(uint8_t, __cdecl, StoreStaticShadow_Game, 0x00526DD0, uint32_t id, int type, void* texture, CVector* pos, float frontX,
        float frontY, float sideX, float sideY, int intensity, int red, int green, int blue, float zDistance, float scale,
        float drawDistance, int temporary, float upDistance);

using re::At;

// (the game's min / max: the first when they're equal)
inline float MinF(float a, float b) {
    return a <= b ? a : b;
}
inline float MaxF(float a, float b) {
    return b <= a ? a : b;
}

inline const CVector& EntityPos(CEntity* e) {
    return e->GetPosition();
}

// 0x525490: the shadow's quad (at pos, front and side half-extents) clipped to each
// triangle of the entity's collision it can touch (within zDistance around pos.z),
// dropped onto the triangle's plane; drawn now (ppPolyBunch nullptr) or kept as poly
// bunches (from the free list, while there are any)
void __cdecl CastShadowEntityXY(CEntity* entity, float startX, float startY, float endX, float endY, CVector* pos, float frontX,
                                float frontY, float sideX, float sideY, int16_t intensity, uint8_t red, uint8_t green,
                                uint8_t blue, float zDistance, float scale, re::Ptr32<CPolyBunch>* ppPolyBunch) {
    (void)startX;
    (void)startY;
    (void)endX;
    (void)endY;
    // (the function statics)
    uint32_t guard = castStaticsGuard;
    if ((guard & 1) == 0) {
        guard |= 1;
        castStaticsGuard = guard;
        for (int i = 0; i < 20; i++) {
            castList[0][i] = CVector{0.0f, 0.0f, 0.0f};
        }
    }
    if ((guard & 2) == 0) {
        guard |= 2;
        castStaticsGuard = guard;
        for (int i = 0; i < 20; i++) {
            castTexture[0][i] = CVector{0.0f, 0.0f, 0.0f};
        }
    }
    if ((guard & 4) == 0) {
        castStaticsGuard = guard | 4;
        for (int i = 0; i < 4; i++) {
            castPoints[i] = CVector{0.0f, 0.0f, 0.0f};
        }
    }
    CVector tri[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

    CColModel* colModel = GetColModel(entity);
    CCollisionData* data = colModel->data;
    if (!data || data->numTriangles == 0) {
        return;
    }
    CVector forward;
    Placeable_GetForward(entity, &forward);
    CVector right;
    if (entity->matrix) {
        right = CVector{entity->matrix->m.m[0], entity->matrix->m.m[1], entity->matrix->m.m[2]};
    } else {
        SimpleTransform_GetRight(&entity->placement, &right);
    }

    float frontRight = frontX * right.x + frontY * right.y;
    float frontForward = frontX * forward.x + frontY * forward.y;
    float sideRight = sideX * right.x + sideY * right.y;
    float sideForward = sideX * forward.x + sideY * forward.y;
    float dx = pos->x - EntityPos(entity).x;
    float dy = pos->y - EntityPos(entity).y;
    float lengthRight = dx * right.x + dy * right.y;
    float lengthForward = dx * forward.x + dy * forward.y;

    castPoints[0].x = (lengthRight + frontRight) - sideRight;
    castPoints[0].y = (lengthForward + frontForward) - sideForward;
    castPoints[1].x = (lengthRight + frontRight) + sideRight;
    castPoints[1].y = (lengthForward + frontForward) + sideForward;
    castPoints[2].x = (lengthRight - frontRight) + sideRight;
    castPoints[2].y = (lengthForward - frontForward) + sideForward;
    castPoints[3].x = (lengthRight - frontRight) - sideRight;
    castPoints[3].y = (lengthForward - frontForward) - sideForward;

    float halfZ = zDistance * 0.5f;
    float maxZ = (pos->z - EntityPos(entity).z) + halfZ;
    float minX = MinF(MinF(castPoints[3].x, castPoints[2].x), MinF(castPoints[1].x, castPoints[0].x));
    float maxX = MaxF(MaxF(castPoints[3].x, castPoints[2].x), MaxF(castPoints[1].x, castPoints[0].x));
    float minY = MinF(MinF(castPoints[3].y, castPoints[2].y), MinF(castPoints[1].y, castPoints[0].y));
    float maxY = MaxF(MaxF(castPoints[3].y, castPoints[2].y), MaxF(castPoints[1].y, castPoints[0].y));

    void* tree = data->planes;
    uint32_t index;
    if (tree) {
        CVector center;
        center.x = (maxX - minX) * 0.5f + minX;
        center.y = (maxY - minY) * 0.5f + minY;
        center.z = maxZ - halfZ;
        float radius = MaxF(std::fabs(frontY) + std::fabs(sideY), std::fabs(frontX) + std::fabs(sideX));
        radius = MaxF(radius, halfZ);
        index = TriangleTree_FindFirst(tree, &center, radius, 0);
    } else {
        index = 0;
    }

    for (;;) {
        CVector normal = {0.0f, 0.0f, 0.0f};
        if (data->planes) {
            if (index == 0xFFFF) {
                return;
            }
        } else if (int(index) >= data->numTriangles) {
            return;
        }
        const CColTriangle& t = data->triangles[index];
        ColModel_GetVertex(colModel, &tri[0], t.a);
        ColModel_GetVertex(colModel, &tri[1], t.b);
        ColModel_GetVertex(colModel, &tri[2], t.c);
        CalcTriangleNormal(&tri[0], &tri[1], &tri[2], &normal);
        const CVector& a = tri[0];
        const CVector& b = tri[1];
        const CVector& c = tri[2];

        if (static_cast<double>(std::fabs(normal.z)) > static_cast<double>(0.1f)) {
            float minZ = maxZ - zDistance;
            if ((minX < a.x || minX < b.x || minX < c.x) && (a.x < maxX || b.x < maxX || c.x < maxX) &&
                (minY < a.y || minY < b.y || minY < c.y) && (a.y < maxY || b.y < maxY || c.y < maxY) &&
                (a.z < maxZ || b.z < maxZ || c.z < maxZ) && (minZ < a.z || minZ < b.z || minZ < c.z)) {
                for (int i = 0; i < 4; i++) {
                    castList[0][i].x = castPoints[i].x;
                    castList[0][i].y = castPoints[i].y;
                }
                castTexture[0][0].x = 0.0f;
                castTexture[0][0].y = 0.0f;
                castTexture[0][1].x = 1.0f;
                castTexture[0][1].y = 0.0f;
                castTexture[0][2].x = 1.0f;
                castTexture[0][2].y = 1.0f;
                castTexture[0][3].x = 0.0f;
                castTexture[0][3].y = 1.0f;
                float edgeX[3] = {b.x - a.x, c.x - b.x, a.x - c.x};
                float edgeY[3] = {b.y - a.y, c.y - b.y, a.y - c.y};

                // each edge in turn: the polygon's part on its inner side
                int src = 1;
                int dst = 0;
                int16_t count = 4;
                for (int e = 0; e < 3; e++) {
                    src = (src + 1) % 2;
                    dst = (dst + 1) % 2;
                    int n = count;
                    const CVector& start = tri[e];
                    float ex = edgeX[e];
                    float ey = edgeY[e];
                    CVector* from = castList[src];
                    CVector* fromTex = castTexture[src];
                    CVector* to = castList[dst];
                    CVector* toTex = castTexture[dst];
                    count = 0;
                    int16_t state = 0;  // 1: the last point was inside, 2: outside
                    auto side = [&](const CVector& p) { return (p.x - start.x) * ey - (p.y - start.y) * ex; };
                    // (where prev -> cur crosses the edge)
                    auto addCrossing = [&](int prev, int cur, float cpCur) {
                        float cpPrev = std::fabs(side(from[prev]));
                        float t1 = cpPrev / (std::fabs(cpCur) + cpPrev);
                        float t0 = 1.0f - t1;
                        toTex[count].x = fromTex[prev].x * t0 + fromTex[cur].x * t1;
                        toTex[count].y = fromTex[prev].y * t0 + fromTex[cur].y * t1;
                        to[count].x = from[prev].x * t0 + t1 * from[cur].x;
                        to[count].y = t0 * from[prev].y + from[cur].y * t1;
                        count++;
                    };
                    auto addPoint = [&](int cur) {
                        toTex[count].x = fromTex[cur].x;
                        toTex[count].y = fromTex[cur].y;
                        to[count].x = from[cur].x;
                        to[count].y = from[cur].y;
                        count++;
                    };
                    for (int k = 0; k < n; k++) {
                        float cp = side(from[k]);
                        if (cp <= 0.0f) {
                            if (state == 1) {
                                addCrossing(k - 1, k, cp);
                            }
                            state = 2;
                        } else if (state == 0 || state == 1) {
                            addPoint(k);
                            state = 1;
                        } else if (state == 2) {
                            addCrossing(k - 1, k, cp);
                            addPoint(k);
                            state = 1;
                        } else {
                            state = 1;
                        }
                    }
                    // the closing edge, last -> first
                    float cpFirst = side(from[0]);
                    if ((cpFirst > 0.0f && state == 2) || (cpFirst <= 0.0f && state == 1)) {
                        const CVector& last = from[n - 1];
                        const CVector& lastTex = fromTex[n - 1];
                        float cpLast = std::fabs(side(last));
                        float t1 = cpLast / (std::fabs(cpFirst) + cpLast);
                        float t0 = 1.0f - t1;
                        toTex[count].x = fromTex[0].x * t1 + lastTex.x * t0;
                        toTex[count].y = lastTex.y * t0 + fromTex[0].y * t1;
                        to[count].x = last.x * t0 + t1 * from[0].x;
                        to[count].y = t0 * last.y + from[0].y * t1;
                        count++;
                    }
                }

                if (count > 2) {
                    int numVerts = count;
                    CVector* list = castList[dst];
                    CVector* tex = castTexture[dst];
                    // onto the triangle's plane
                    float d = normal.x * a.x + normal.y * a.y + normal.z * a.z;
                    for (int i = 0; i < numVerts; i++) {
                        list[i].z = -(((list[i].x * normal.x + list[i].y * normal.y) - d) / normal.z);
                    }
                    // into the world
                    for (int i = 0; i < numVerts; i++) {
                        float x = list[i].x;
                        list[i].x = right.x * list[i].x + forward.x * list[i].y + Placeable_GetPosition(entity)->x;
                        list[i].y = forward.y * list[i].y + x * right.y + Placeable_GetPosition(entity)->y;
                        list[i].z = Placeable_GetPosition(entity)->z + list[i].z;
                    }
                    if (!ppPolyBunch) {
                        int numIndices = numVerts * 3 - 6;
                        re::Ptr32<uint16_t> indices;
                        re::Ptr32<BufferVertex> verts;
                        int base;
                        RenderBuffer_StartStoring(numIndices, numVerts, &indices, &verts, &base);
                        uint32_t color = uint32_t(int(intensity)) << 24 | uint32_t(red) << 16 | uint32_t(green) << 8 | blue;
                        for (int i = 0; i < numVerts; i++) {
                            verts[i].color = color;
                            verts[i].u = tex[i].x * scale;
                            verts[i].v = tex[i].y * scale;
                            verts[i].pos.x = list[i].x;
                            verts[i].pos.y = list[i].y;
                            verts[i].pos.z = list[i].z + 0.03f;
                        }
                        for (int i = 0; i < numIndices; i++) {
                            indices[i] = uint16_t(ShadowIndexList[i] + base);
                        }
                        RenderBuffer_StopStoring();
                    } else if (CPolyBunch* bunch = pEmptyBunchList) {
                        pEmptyBunchList = bunch->next;
                        bunch->next = *ppPolyBunch;
                        *ppPolyBunch = bunch;
                        bunch->numVerts = count;
                        for (int i = 0; i < numVerts; i++) {
                            bunch->verts[i] = list[i];
                            bunch->u[i] = uint8_t(int(tex[i].x * 200.0f));
                            bunch->v[i] = uint8_t(int(tex[i].y * 200.0f));
                        }
                    }
                }
            }
        }
        if (data->planes) {
            index = TriangleTree_FindNext(data->planes);
        } else {
            index++;
        }
    }
}

// 0x526700: CShadows::UpdateStaticShadows: the ones not stored again this frame (or,
// temporary ones, 5 s after they were made) freed, their bunches back to the free list
void __cdecl UpdateStaticShadows() {
    for (int i = 0; i < 20; i++) {
        CStaticShadow& shadow = aStaticShadows[i];
        if (shadow.polyBunch && !shadow.justCreated && (!shadow.temporary || shadow.timeCreated + 5000 < timeInMs)) {
            // (StaticShadow::Free inlined: the whole list before the free ones)
            if (CPolyBunch* first = shadow.polyBunch) {
                CPolyBunch* last = first;
                while (last->next) {
                    last = last->next;
                }
                last->next = pEmptyBunchList;
                pEmptyBunchList = first;
            }
            shadow.polyBunch = nullptr;
            shadow.id = 0;
        }
        shadow.justCreated = 0;
    }
}

// 0x526770: CShadows::CalcPedShadowValues (the sun's direction x, y)
void __cdecl CalcPedShadowValues(float sunX, float sunY) {
    float dist = std::sqrt(std::fabs(sunY * sunY + sunX * sunX));
    float recip = 1.0f / dist;
    float mult = dist + 1.0f;
    float negX = -sunX;
    pedShadowDisplacementX = negX * mult * recip * 0.5f;
    float negY = -sunY;
    pedShadowDisplacementY = negY * mult * recip * 0.5f;
    pedShadowFrontX = negY * recip * 0.5f;
    pedShadowFrontY = sunX * recip * 0.5f;
    pedShadowSideX = negX * 0.5f;
    pedShadowSideY = negY * 0.5f;
}

// 0x526830: CShadows::CastShadowSectorList: onto each entity of a sector's list (once a
// scan) that collides, is in this chapter and area, and overlaps the shadow's rectangle
// and height range
void __cdecl CastShadowSectorList(CSector* sector, float startX, float startY, float endX, float endY, CVector* pos, float frontX,
                                  float frontY, float sideX, float sideY, int16_t intensity, uint8_t red, uint8_t green,
                                  uint8_t blue, float zDistance, float scale, re::Ptr32<CPolyBunch>* ppPolyBunch) {
    // (a node: bits 0-3 the pool, 4-17 the slot, 18-31 the next node, 0x3FFF: none)
    uint32_t* node = sector->firstNode;
    while (node) {
        uint32_t value = *node;
        CEntity* entity = GetEntityFromNode(value & 0xF, (value >> 4) & 0x3FFF);
        node = (value >> 18) == 0x3FFF ? nullptr : sectorNodes + (value >> 18);
        if (entity->scanCode == currentScanCode) {
            continue;
        }
        entity->scanCode = currentScanCode;
        CColModel* colModel = GetColModel(entity);
        if (!entity->bUsesCollision || (!entity->flag1C && !entity->flag60)) {
            continue;
        }
        if ((modelInfoPtrs[entity->modelIndex][0xB] & uint8_t(1u << (chapter & 31))) == 0 || entity->flagF4) {
            continue;
        }
        if ((colModel->data && colModel->data->unknown18) || entity->flag84[7]) {
            continue;
        }
        uint8_t area = entity->unknown111;
        if (area != currArea && area != 1 && (!extraAreaActive || area != extraArea)) {
            continue;
        }
        col::CRect rect;
        reinterpret_cast<void(__thiscall*)(CEntity*, col::CRect*)>(static_cast<void*>(entity->vtable[11]))(entity, &rect);
        if (!(startX < rect.right) || !(rect.left < endX) || !(startY < rect.top) || !(rect.bottom < endY)) {
            continue;
        }
        const CVector* entityPos = &entity->GetPosition();
        CColModel* col1 = GetColModel(entity);
        if (!(pos->z - zDistance < entityPos->z + col1->boxMax.z)) {
            continue;
        }
        entityPos = &entity->GetPosition();
        CColModel* col2 = GetColModel(entity);
        if (!(entityPos->z + col2->boxMin.z < pos->z)) {
            continue;
        }
        CastShadowEntityXY_Game(entity, startX, startY, endX, endY, pos, frontX, frontY, sideX, sideY, intensity, red, green,
                                blue, zDistance, scale, ppPolyBunch);
    }
}

// 0x526B00: CShadows::GeneratePolysForStaticShadow: its bunches from the sectors under it
void __cdecl GeneratePolysForStaticShadow(int16_t index) {
    CStaticShadow& shadow = aStaticShadows[index];
    CVector center = shadow.pos;
    float extentX = std::fabs(shadow.frontX) + std::fabs(shadow.sideX);
    float extentY = std::fabs(shadow.frontY) + std::fabs(shadow.sideY);
    float minX = center.x - extentX;
    float maxX = center.x + extentX;
    float minY = center.y - extentY;
    float maxY = center.y + extentY;
    // (GetSector: / 50 + 18, clamped to 0..35 as floats)
    int startX = int(MaxF(float(int(minX / 50.0f + 18.0f)), 0.0f));
    int startY = int(MaxF(float(int(minY / 50.0f + 18.0f)), 0.0f));
    int endX = int(MinF(float(int(maxX / 50.0f + 18.0f)), 35.0f));
    int endY = int(MinF(float(int(maxY / 50.0f + 18.0f)), 35.0f));
    if (currentScanCode >= 0xFFFF) {
        ClearScanCodes();
        currentScanCode = 1;
    } else {
        currentScanCode++;
    }
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            CastShadowSectorList_Game(&sectors[y * 36 + x], minX, minY, maxX, maxY, &center, shadow.frontX, shadow.frontY,
                                      shadow.sideX, shadow.sideY, 0, 0, 0, 0, shadow.zDistance, shadow.scale, &shadow.polyBunch);
        }
    }
}

// 0x526DD0: CShadows::StoreStaticShadow: kept (and faded out over the last quarter of
// drawDistance, colors doubled) if it's in range; the same id at nearly the same place
// only refreshed. False when it covers nothing
uint8_t __cdecl StoreStaticShadow(uint32_t id, uint8_t type, void* texture, CVector* pos, float frontX, float frontY, float sideX,
                                  float sideY, int16_t intensity, uint8_t red, uint8_t green, uint8_t blue, float zDistance,
                                  float scale, float drawDistance, uint8_t temporary, float upDistance) {
    uint8_t* camera = GetLocalTransform(GetCameraNode(cameraNode));
    float dx = pos->x - At<float>(camera, 0x24);
    float dy = pos->y - At<float>(camera, 0x28);
    float distSq = dy * dy + dx * dx;
    if (distSq < drawDistance * drawDistance) {
        if (drawDistance != 0.0f) {
            float dist = std::sqrt(std::fabs(distSq));
            if (dist >= drawDistance * 0.75f) {
                float fade = 1.0f - (dist - drawDistance * 0.75f) * (4.0f / drawDistance);
                intensity = int16_t(int(float(int(intensity)) * fade));
                red = uint8_t(int(float(red) * fade));
                green = uint8_t(int(float(green) * fade));
                blue = uint8_t(int(float(blue) * fade));
            }
        }
    } else if (drawDistance != 0.0f) {
        return 1;
    }
    int r = red * 2 > 0xFF ? 0xFF : red * 2;
    int g = green * 2 > 0xFF ? 0xFF : green * 2;
    int b = blue * 2 > 0xFF ? 0xFF : blue * 2;

    int16_t slot = 0;
    for (; slot < 20; slot++) {
        if (aStaticShadows[slot].id == id && aStaticShadows[slot].polyBunch) {
            break;
        }
    }
    if (slot < 20) {
        CStaticShadow& shadow = aStaticShadows[slot];
        float adx = std::fabs(pos->x - shadow.pos.x);
        bool same = adx < upDistance && std::fabs(pos->y - shadow.pos.y) < upDistance;
        if (!same) {
            same = adx < 0.02f && std::fabs(pos->y - shadow.pos.y) < 0.02f &&
                   static_cast<double>(std::fabs(pos->z - shadow.pos.z)) < static_cast<double>(0.05f) && shadow.frontX == frontX &&
                   shadow.frontY == frontY && shadow.sideX == sideX && shadow.sideY == sideY;
        }
        if (same) {
            shadow.type = type;
            shadow.zDistance = zDistance;
            shadow.texture = texture;
            shadow.scale = scale;
            shadow.intensity = intensity;
            shadow.red = uint8_t(r);
            shadow.green = uint8_t(g);
            shadow.blue = uint8_t(b);
            shadow.timeCreated = timeInMs;
            shadow.justCreated = 1;
            shadow.temporary = temporary;
            return 1;
        }
        StaticShadow_Free(&shadow);
    } else {
        for (slot = 0; slot < 20; slot++) {
            if (!aStaticShadows[slot].polyBunch) {
                break;
            }
        }
        if (slot == 20) {
            return 1;
        }
    }
    CStaticShadow& shadow = aStaticShadows[slot];
    shadow.id = id;
    shadow.zDistance = zDistance;
    shadow.type = type;
    shadow.scale = scale;
    shadow.texture = texture;
    shadow.red = uint8_t(r);
    shadow.green = uint8_t(g);
    shadow.intensity = intensity;
    shadow.blue = uint8_t(b);
    shadow.pos = *pos;
    shadow.frontX = frontX;
    shadow.frontY = frontY;
    shadow.justCreated = 1;
    shadow.temporary = temporary;
    shadow.sideX = sideX;
    shadow.timeCreated = timeInMs;
    shadow.sideY = sideY;
    GeneratePolysForStaticShadow_Game(slot);
    return shadow.polyBunch ? 1 : 0;
}

// 0x527280: a light's static shadow (id a + b, type 2), stored while within 27 of the
// camera and in view (always during cutscenes)
void __cdecl StoreLightShadow(int idA, int idB, void* texture, CVector* pos, float frontX, float frontY, float sideX, float sideY,
                              uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t* camera = GetLocalTransform(GetCameraNode(cameraNode));
    float dx = pos->x - At<float>(camera, 0x24);
    float dy = pos->y - At<float>(camera, 0x28);
    if (!(dy * dy + dx * dx < 729.0f) && !cutsceneRunning) {
        return;
    }
    float radius = MaxF(std::fabs(frontY) + std::fabs(sideY), std::fabs(frontX) + std::fabs(sideX));
    if (cutsceneRunning || Camera_IsSphereVisible(theCamera, pos, radius)) {
        StoreStaticShadow_Game(uint32_t(idA + idB), 2, texture, pos, frontX, frontY, sideX, sideY, 0x80, red, green, blue, 6.0f, 1.0f,
                               27.0f, 0, 0.4f);
    }
}

} // namespace shadows17

// ---- registration ----------------------------------------------------------------------

#define SHADOWS17_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace shadows17 {
SHADOWS17_REVERSED("CShadows::CastShadowEntityXY", 0x00525490, &CastShadowEntityXY);
SHADOWS17_REVERSED("CShadows::UpdateStaticShadows", 0x00526700, &UpdateStaticShadows);
SHADOWS17_REVERSED("CShadows::CalcPedShadowValues", 0x00526770, &CalcPedShadowValues);
SHADOWS17_REVERSED("CShadows::CastShadowSectorList", 0x00526830, &CastShadowSectorList);
SHADOWS17_REVERSED("CShadows::GeneratePolysForStaticShadow", 0x00526B00, &GeneratePolysForStaticShadow);
SHADOWS17_REVERSED("CShadows::StoreStaticShadow", 0x00526DD0, &StoreStaticShadow);
SHADOWS17_REVERSED("CShadows::StoreLightShadow", 0x00527280, &StoreLightShadow);
} // namespace shadows17
