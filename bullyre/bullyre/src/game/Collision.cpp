#include "game/Collision.h"

#include <cstring>

#include "game/ModelInfo.h"
#include "game/Streaming.h"

#include "game/Crt.h"
#include "reversed.h"
using col::CMatrix;
using col::CRect;
using col::CVector;

namespace {
// the game's memory manager
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, FreeArray, 0x005EEA60, void* p);   // (what the collision arrays are freed with)
GAME_FN(void*, __cdecl, MallocTagged, 0x005EEA50, size_t size, int);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
// the compiler's `new T[n]` constructor loop
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* p, size_t size, int count, void(__thiscall* ctor)(void*));
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
// CFileMgr
GAME_FN(int, __cdecl, OpenFile, 0x0042D260, const char* name, const char* mode, int);
GAME_FN(int, __cdecl, ReadFromFile, 0x0042D2A0, int fd, void* buffer, int size);
GAME_FN(int, __cdecl, WriteToFile, 0x0042D2D0, int fd, const void* buffer, int size);
GAME_FN(uint8_t, __cdecl, ReadLine, 0x0042D330, int fd, char* buffer, int size);
GAME_FN(void, __cdecl, CloseFile, 0x0042D450, int fd);
// CFileLoader: a .col file's models (the first time: sets up the slot's ids and box)
GAME_FN(uint8_t, __cdecl, LoadCollisionFile, 0x0042C6C0, void* data, uint32_t size, int slot);
GAME_FN(uint8_t, __cdecl, LoadCollisionFileFirstTime, 0x0042C8E0, void* data, uint32_t size, int slot);
GAME_FN(void, __cdecl, GenerateBoundingBoxes, 0x0052ED40);   // (when not from ColStore.bin)
GAME_FN(int, __cdecl, GetAreaFromName, 0x004305D0, const char* name);
// CStreaming
GAME_FN(CStreamingInfo*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, SetColLoaded, 0x0052DFF0, int slot, int loaded);   // the entry's load state
GAME_FN(void, __cdecl, LoadAllRequestedModels, 0x00533EE0, int priorityOnly);
GAME_VAR(uint8_t, ms_disableStreaming, 0x00CA3984);
// the math
GAME_FN(float*, __thiscall, GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412A10, CMatrix* m, const float* from);
GAME_FN(void, __thiscall, Matrix_SetTranslate, 0x004127C0, CMatrix* m, const CVector* pos);
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const CMatrix* m, CMatrix* out);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Matrix_MultiplyPoint(CVector* out, const CMatrix* m, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, m, v); }
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline void Matrix_MultiplyPoint4(float* out4, const CMatrix* m, const CVector* v) { re::CallVec16<0x0045D500, void>(out4, m, v); }   // (writes 4 floats)
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, DecompressVector, 0x00565D50, CVector* out, const CompressedVector* v);
GAME_FN(uint8_t, __cdecl, SolveQuadratic, 0x00433400, float a, float b, float c, float* t1, float* t2);
GAME_FN(uint8_t, __thiscall, Rect_IsPointInside, 0x00435AA0, const CRect* rect, const CVector* pos);
GAME_FN(uint8_t, __thiscall, Rect_IsPointInsideMargin, 0x00435AF0, const CRect* rect, const CVector* pos, float margin);
// what's registered for a model with data +0x18 (physics?)
GAME_FN(void, __cdecl, ColModel_Unregister, 0x005E8D60, CColModel* col);
GAME_FN(void, __cdecl, ColModel_Register, 0x005EB8B0, CColModel* col);
GAME_FN(void, __thiscall, TrianglePlanes_Destroy, 0x005611B0, TrianglePlanes* planes);
GAME_FN(CColModel*, __thiscall, ColModelPool_New, 0x007305B0, void* pool);
GAME_VAR(CPool<CColModel>*, ms_pColModelPool, 0x00C0F604);
// models without model info (their collision only)
GAME_FN(CColModel*, __cdecl, GetColModelOfModel, 0x0051A960, int model);
GAME_FN(void, __cdecl, RemoveColModelOfModel, 0x0051CBD0, int model);
GAME_FN(void, __thiscall, ModelInfo_SetColModel, 0x0050EB30, CBaseModelInfo* mi, CColModel* col, int owns);
// CWorld
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, CEntity* entity);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, CEntity* entity);
GAME_FN(void, __thiscall, Entity_466260, 0x00466260, CEntity* entity);
GAME_FN(uint8_t, __cdecl, Entity_45EE90, 0x0045EE90, CEntity* entity);   // (skipped by scans with list flag 0x80)
GAME_FN(CEntity*, __cdecl, GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_VAR(uint16_t, ms_nCurrentScanCode, 0x00C1AE88);
GAME_VAR(uint32_t*, ms_pNodes, 0x00C0F788);   // the sector lists' nodes
// the sectors: 36 x 36 of 50 units, five lists each (0x14)
inline uint8_t* const ms_aSectors = reinterpret_cast<uint8_t*>(0x00C1B178);
GAME_FN(void*, __cdecl, GetEntityColModel, 0x0051AF50, CEntity* entity);
GAME_FN(int, __cdecl, ProcessColModels, 0x0056C050, const CMatrix* m1, CColModel* col1, const float* m2, void* col2, uint8_t* points,
        uintptr_t, uintptr_t, int32_t* numPoints, int);
GAME_FN(uint8_t, __thiscall, Entity_TouchesSphere, 0x00465BA0, CEntity* entity, const CVector* center, float radius);
GAME_VAR(uint8_t, ms_currentChapter, 0x00A147E0);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, CleanUpReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
// the game state LoadCollision/EnsureCollisionIsInMemory look at
GAME_VAR(int32_t, currArea, 0x00BD1008);                // CGame::currArea
GAME_VAR(float, ms_colLoadMargin, 0x00A135D4);          // (50)
GAME_VAR(uint8_t, gbRefreshEntities, 0x00BF3815);       // LoadCol/RemoveCol take the file's entities out of the world and back
GAME_VAR(uint8_t, gbColLoadAtStartOnly, 0x00BF380F);    // GenerateAllBoundingBoxes only loads the flagged files
GAME_VAR(uint8_t, gbExtraLoadPoint, 0x00BD09E9);        // a second position to load around
GAME_VAR(int8_t, gExtraLoadArea, 0x00BD09EB);
GAME_VAR(float, gExtraLoadX, 0x00BD09F0);
GAME_VAR(float, gExtraLoadY, 0x00BD09F4);
GAME_FN(uint8_t, __cdecl, Fn_42E430, 0x0042E430);
GAME_FN(int, __cdecl, Fn_4A84A0, 0x004A84A0);
GAME_FN(uint8_t, __thiscall, Fn_70D9F0, 0x0070D9F0, void* self, const CRect* rect);
GAME_VAR(void*, obj_20C8704, 0x020C8704);
GAME_FN(int, __thiscall, Fn_4F12F0, 0x004F12F0, void* self);
inline void* const obj_C3CC68 = reinterpret_cast<void*>(0x00C3CC68);
GAME_FN(void, __thiscall, Fn_696150, 0x00696150, void* self, const char* name, int);
GAME_VAR(void*, obj_20C32A4, 0x020C32A4);
GAME_VAR(int32_t, int_BA6890, 0x00BA6890);
GAME_FN(void, __cdecl, Fn_45B7B0, 0x0045B7B0);
GAME_FN(void, __cdecl, Fn_45B7D0, 0x0045B7D0);
// growing array {data, capacity, count}: (new capacity, keep the count as capacity)
GAME_FN(void, __thiscall, PtrArray_Reserve, 0x006DB480, void* array, uint32_t capacity, int);

// (ours, called where they are so each one's mode applies)
GAME_FN(void, __thiscall, CollisionData_Construct, 0x0056DAD0, CCollisionData* data);
GAME_FN(void, __thiscall, CollisionData_RemoveCollisionVolumes, 0x0056DB00, CCollisionData* data);
GAME_FN(void, __thiscall, ColModel_Construct, 0x0056E7A0, CColModel* col);
GAME_FN(void, __thiscall, ColModel_RemoveCollisionVolumes, 0x0056E7D0, CColModel* col);
GAME_FN(CColModel*, __cdecl, ColModel_New, 0x0056E7C0, size_t size);
GAME_FN(void, __cdecl, ColModel_Delete, 0x0056EB30, CColModel* col);
GAME_FN(const char*, __thiscall, ColDef_GetName, 0x0056ED20, const ColDef* def);
GAME_FN(ColDef*, __thiscall, ColDefPool_New_Game, 0x0056EDF0, CPool<ColDef>* pool);
GAME_FN(void, __thiscall, ColDefPool_Init_Game, 0x0056F4C0, CPool<ColDef>* pool);
GAME_FN(uint32_t, __thiscall, EntityRectScan_Scan, 0x0056F8D0, EntityRectScan* scan);
GAME_FN(void, __thiscall, EntityRectScan_ScanList, 0x0056EFD0, EntityRectScan* scan, re::Ptr32<void>* list);
GAME_FN(void, __thiscall, ColSphereQuery_ScanList, 0x005708E0, CColSphereQuery* query, re::Ptr32<void>* list);
GAME_FN(void, __thiscall, SurfaceTable_Load, 0x00570D40, CSurfaceTable* table);
GAME_FN(int, __cdecl, Col_AddColSlot, 0x0056F840, const char* name);
GAME_FN(int, __cdecl, Col_FindColSlot, 0x0056EF10, const char* name);
GAME_FN(void, __cdecl, Col_SetArea, 0x0056EE90, int slot, int area);
GAME_FN(void, __cdecl, Col_LoadBoundingBoxes, 0x0056F160);
GAME_FN(void, __cdecl, Col_LoadAllCollision, 0x0056F330);
GAME_FN(void, __cdecl, Col_RemoveAllCollision, 0x0056F260);
inline const auto Col_IsEntityFromCol = reinterpret_cast<uint8_t(__cdecl*)(int, CEntity*)>(0x0056EF60);

// entity fields (CEntity isn't laid out here yet)
uint16_t& ScanCode(CEntity* e) {
    return *reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(e) + 0x10A);
}
int16_t ModelIndex(CEntity* e) {
    return *reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(e) + 0x10E);
}

// the vertices' 1/128ths
constexpr double kVertexScale = 1.0 / 128.0;
constexpr double kVertexUnscale = 128.0;

CVector Decompress(const CompressedVector& v) {
    return CVector{float(v.x * kVertexScale), float(v.y * kVertexScale), float(v.z * kVertexScale)};
}
} // namespace

// ---- CRect ------------------------------------------------------------------------

uint32_t CRect::OverlapsWith(const CRect* other) const {
    // (written so NaNs go the way the game's compares send them)
    if (other->right < left || other->left > right || other->top < bottom || !(other->bottom <= top)) {
        return 0;
    }
    return 1;
}

// ---- CColSphere --------------------------------------------------------------------

void CColSphere::Set(float r, const CVector* c) {
    radius = r;
    center = *c;
}

void CColSphere::Set(float r, const CVector* c, uint8_t surf, uint8_t pc) {
    radius = r;
    center = *c;
    surface = surf;
    piece = pc;
}

uint8_t CColSphere::IntersectRay(const CVector* origin, const CVector* dir, CVector* entry, CVector* exit) const {
    float r = radius;
    CVector d;
    VectorSub(&d, origin, &center);
    float b = dir->y * d.y + dir->x * d.x + dir->z * d.z;
    float c = (d.y * d.y + d.x * d.x + d.z * d.z) - r * r;
    float t1, t2;
    uint8_t hit = SolveQuadratic(1.0f, b + b, c, &t1, &t2);
    if (!hit) {
        return hit;
    }
    CVector v{dir->x * t1, dir->y * t1, t1 * dir->z};
    CVector sum;
    *entry = *VectorAdd(&sum, origin, &v);
    v = CVector{dir->x * t2, dir->y * t2, t2 * dir->z};
    *exit = *VectorAdd(&sum, origin, &v);
    return 1;
}

// ---- CCollisionData ----------------------------------------------------------------

CCollisionData* CCollisionData::Construct() {
    spheres = nullptr;
    unknown04 = nullptr;
    boxes = nullptr;
    vertices = nullptr;
    triangles = nullptr;
    planes = nullptr;
    unknown18 = nullptr;
    numTriangles = 0;
    numSpheres = 0;
    numBoxes = 0;
    unknown20 = 0;
    return this;
}

void CCollisionData::RemoveCollisionVolumes() {
    if (spheres) {
        FreeArray(spheres);
    }
    if (unknown04) {
        FreeArray(unknown04);
    }
    if (boxes) {
        FreeArray(boxes);
    }
    if (vertices) {
        FreeArray(vertices);
    }
    if (triangles) {
        FreeArray(triangles);
    }
    if (unknown18) {
        FreeArray(unknown18);
    }
    if (planes) {
        TrianglePlanes_Destroy(planes);
        Free(planes);
    }
    numSpheres = 0;
    unknown20 = 0;
    numBoxes = 0;
    numTriangles = 0;
    spheres = nullptr;
    unknown04 = nullptr;
    boxes = nullptr;
    vertices = nullptr;
    triangles = nullptr;
    planes = nullptr;
    unknown18 = nullptr;
}

// ---- CColModel ---------------------------------------------------------------------

CColModel* CColModel::Construct() {
    sphereCenter = CVector{0.0f, 0.0f, 0.0f};
    colSlot = 0;
    unknown32 = 0;
    data = nullptr;
    owner = nullptr;
    unknown34 = 0;
    return this;
}

void CColModel::RemoveCollisionVolumes() {
    if (!data) {
        return;
    }
    if (data->unknown18) {
        ColModel_Unregister(this);
    }
    CollisionData_RemoveCollisionVolumes(data);
    // (then deleted, whose destructor removes them again)
    if (CCollisionData* d = data) {
        CollisionData_RemoveCollisionVolumes(d);
        Free(d);
    }
    data = nullptr;
}

void CColModel::SetOwner(void* placeable) {
    owner = placeable;
    if (placeable && data && data->unknown18) {
        ColModel_Register(this);
    }
}

void CColModel::GetVertex(CVector* out, int index) const {
    *out = Decompress(data->vertices[index]);
}

void CColModel::GetWorldVertex(CVector* out, int index) const {
    float v[4];
    CVector local = Decompress(data->vertices[index]);
    std::memcpy(v, &local, sizeof(local));
    CMatrix m;
    Matrix_Copy(&m, GetMatrix(owner));
    Matrix_MultiplyPoint4(v, &m, reinterpret_cast<CVector*>(v));
    *out = CVector{v[0], v[1], v[2]};
}

void CColModel::SetWorldVertex(int index, const CVector* pos) {
    CMatrix m;
    Matrix_Copy(&m, GetMatrix(owner));
    CMatrix inverse;
    inverse.attached = nullptr;
    Matrix_Invert(&m, &inverse);
    CVector local;
    Matrix_MultiplyPoint(&local, &inverse, pos);
    // (truncated, as _ftol does)
    CompressedVector& v = data->vertices[index];
    v.x = int16_t(int64_t(local.x * kVertexUnscale));
    v.y = int16_t(int64_t(local.y * kVertexUnscale));
    v.z = int16_t(int64_t(local.z * kVertexUnscale));
}

uint8_t CColModel::GetWorldTriangleEdge(CVector* start, CVector* end, int triangle, int edge) const {
    if (!data || triangle < 0 || triangle >= data->numTriangles || !owner) {
        return 0;
    }
    const uint16_t* corners = &data->triangles[triangle].a;
    int next = edge == 2 ? 0 : edge + 1;
    DecompressVector(start, &data->vertices[corners[edge]]);
    DecompressVector(end, &data->vertices[corners[next]]);
    CMatrix m;
    Matrix_Copy(&m, GetMatrix(owner));
    CVector world;
    Matrix_MultiplyPoint(&world, &m, start);
    *start = world;
    Matrix_MultiplyPoint(&world, &m, end);
    *end = world;
    return 1;
}

void CColModel::GetWorldTriangle(int triangle, CVector* corners) const {
    const CColTriangle& t = data->triangles[triangle];
    corners[0] = Decompress(data->vertices[t.a]);
    corners[1] = Decompress(data->vertices[t.b]);
    corners[2] = Decompress(data->vertices[t.c]);
    CMatrix m;
    Matrix_Copy(&m, GetMatrix(owner));
    CVector world;
    for (int i = 0; i < 3; i++) {
        Matrix_MultiplyPoint(&world, &m, &corners[i]);
        corners[i] = world;
    }
}

CColModel* __cdecl CColModel::New(size_t) {
    return ColModelPool_New(ms_pColModelPool);
}

void __cdecl CColModel::Delete(CColModel* col) {
    ms_pColModelPool->Delete(col);
}

// ---- ColDef and its pool -----------------------------------------------------------

namespace {
constexpr float kRectEmpty = 1000000.0f;

void ResetBounds(CRect& r) {
    r.left = kRectEmpty;
    r.bottom = kRectEmpty;
    r.right = -kRectEmpty;
    r.top = -kRectEmpty;
}

// (the game's pool access: nullptr for a free slot, and callers read through it)
ColDef* Def(int slot) {
    return CColStore::ms_pColPool->GetAt(slot);
}

// A file's area bit. (The game copies the two words and indexes the copy, reading
// its stack for areas past 63; here that reads on into the ColDef.)
bool InArea(const ColDef* def, int area) {
    const uint32_t* word = area >= 32 ? &def->areas[uint32_t(area) >> 5] : &def->areas[0];
    return (*word & (1u << (area & 31))) != 0;
}

// `new`'s size for n elements plus a count before them (saturating like the compiler's)
uint32_t CountedArraySize(uint32_t n, uint32_t elemSize) {
    uint64_t bytes = uint64_t(n) * elemSize;
    uint32_t size = bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : uint32_t(bytes);
    return size > 0xFFFFFFFFu - 4 ? 0xFFFFFFFFu : size + 4;
}
} // namespace

ColDef* ColDef::Construct() {
    ResetBounds(bounds);
    return this;
}

const char* ColDef::GetName() const {
    char* buffer = CColStore::ms_nameBuffer;
    buffer[8] = '\0';
    uint32_t hash = nameHash;
    for (int i = 7; i >= 0; i--) {
        uint32_t digit = hash & 0xF;
        buffer[i] = char(digit < 10 ? '0' + digit : 'A' - 10 + digit);
        hash >>= 4;
    }
    return buffer;
}

ColDef* __fastcall ColDefPool_New(CPool<ColDef>* pool, re::Edx) {
    uint8_t* flags = pool->flags;
    bool wrapped = false;
    if (!(flags[pool->lastUsed] & 0x80)) {
        do {
            if (++pool->lastUsed >= pool->size) {
                if (wrapped) {
                    return nullptr;
                }
                pool->lastUsed = 0;
                wrapped = true;
            }
        } while (!(flags[pool->lastUsed] & 0x80));
    }
    flags[pool->lastUsed] &= 0x7F;
    uint8_t& flag = pool->flags[pool->lastUsed];
    flag = uint8_t((flag & 0x80) | ((flag + 1) & 0x7F));   // (the reuse count)
    ColDef* def = reinterpret_cast<ColDef*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * pool->lastUsed);
    std::memset(def, 0, 0x30);
    return def;
}

void __fastcall ColDefPool_Init(CPool<ColDef>* pool, re::Edx) {
    pool->ownsArrays = 1;
    pool->elemSize = 0x30;
    if (pool->unknown19) {
        // constructed, with the count before them
        int count = pool->size;
        uint32_t* raw = static_cast<uint32_t*>(Malloc(CountedArraySize(uint32_t(count), 0x30)));
        if (raw) {
            *raw = uint32_t(count);
            ConstructArray(raw + 1, 0x30, count, reinterpret_cast<void(__thiscall*)(void*)>(0x0056EE70));
            pool->objects = reinterpret_cast<ColDef*>(raw + 1);
        } else {
            pool->objects = nullptr;
        }
    } else {
        pool->objects = static_cast<ColDef*>(Malloc(size_t(pool->size) * 0x30));
    }
    pool->flags = static_cast<uint8_t*>(Malloc(size_t(pool->size)));
    pool->lastUsed = 0;
    for (int i = 0; i < pool->size; i++) {
        pool->flags[i] = 0x80;
    }
}

// ---- the rect scan -----------------------------------------------------------------

void EntityRectScan::ScanList(re::Ptr32<void>* list) {
    uint32_t* node = static_cast<uint32_t*>(*list);
    while (node) {
        uint32_t entry = *node;
        CEntity* entity = GetEntityFromNode(int(entry & 0xF), int((entry >> 4) & 0x3FFF));
        node = (entry >> 18) == 0x3FFF ? nullptr : ms_pNodes + (entry >> 18);
        if (ScanCode(entity) == ms_nCurrentScanCode) {
            continue;
        }
        ScanCode(entity) = ms_nCurrentScanCode;
        if ((lists & 0x80) && Entity_45EE90(entity)) {
            continue;
        }
        CRect r;
        auto getBoundRect = reinterpret_cast<void*(__thiscall*)(CEntity*, CRect*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[11]));
        getBoundRect(entity, &r);
        if (rect.right < r.left || rect.left > r.right || rect.top < r.bottom || !(rect.bottom <= r.top)) {
            continue;
        }
        if (!filter(user, entity)) {
            continue;
        }
        if (count == capacity) {
            PtrArray_Reserve(&entities, capacity ? capacity * 2 : 1, 0);
        }
        entities[count++] = entity;
    }
}

namespace {
// A coordinate's sector, clamped to 0 (low) or 35 (high). (Through a float, as the game does.)
int SectorLow(double v) {
    float s = float(int(v / 50.0 + 18.0));
    return int(s > 0.0f ? s : 0.0f);
}
int SectorHigh(double v) {
    float s = float(int(v / 50.0 + 18.0));
    return int(s < 35.0f ? s : 35.0f);
}

void NextScanCode() {
    if (ms_nCurrentScanCode < 0xFFFF) {
        ms_nCurrentScanCode++;
    } else {
        ClearScanCodes();
        ms_nCurrentScanCode = 1;
    }
}

// The sectors' lists in a range, in the order the game goes through them
template <typename F>
void ForEachSectorList(int x0, int y0, int x1, int y1, uint32_t lists, F scanList) {
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            re::Ptr32<void>* sector = reinterpret_cast<re::Ptr32<void>*>(ms_aSectors + (y * 36 + x) * 0x14);
            if (lists & 1) {
                scanList(&sector[0]);
            }
            if (lists & 2) {
                scanList(&sector[2]);
            }
            if (lists & 4) {
                scanList(&sector[3]);
            }
            if (lists & 8) {
                scanList(&sector[1]);
            }
            if (lists & 0x10) {
                scanList(&sector[4]);
            }
        }
    }
}
} // namespace

uint32_t EntityRectScan::Scan() {
    int x0 = SectorLow(rect.left);
    int y0 = SectorLow(rect.bottom);
    int x1 = SectorHigh(rect.right);
    int y1 = SectorHigh(rect.top);
    NextScanCode();
    ForEachSectorList(x0, y0, x1, y1, lists, [this](re::Ptr32<void>* list) { EntityRectScan_ScanList(this, list); });
    return count;
}

// ---- CColStore ---------------------------------------------------------------------

namespace CColStore {

void __cdecl Initialise() {
    if (!ms_pColPool) {
        auto* pool = static_cast<CPool<ColDef>*>(Malloc(sizeof(CPool<ColDef>)));
        if (pool) {
            pool->unknown10 = 0;
            pool->objects = nullptr;
            pool->flags = nullptr;
            pool->lastUsed = 0;
            pool->ownsArrays = 1;
            pool->size = 500;
            pool->unknown19 = 0;
            ColDefPool_Init_Game(pool);
        }
        ms_pColPool = pool;
    }
    Col_AddColSlot("generic");
}

void __cdecl Shutdown() {
    for (int slot = 0; slot < 500; slot++) {
        ColDef* def = Def(slot);
        if (!def) {
            continue;
        }
        if (def->loaded) {
            game::RemoveCol(slot);
        }
        ms_pColPool->Delete(def);
    }
    CPool<ColDef>* pool = ms_pColPool;
    if (pool) {
        if (pool->size > 0) {
            if (pool->ownsArrays) {
                if (pool->unknown19) {
                    if (pool->objects) {
                        Free(re::PtrCast<uint8_t*>(pool->objects) - 4);
                    }
                } else {
                    Free(pool->objects);
                }
                Free(pool->flags);
            }
            pool->objects = nullptr;
            pool->flags = nullptr;
            pool->size = 0;
            pool->lastUsed = 0;
        }
        Free(pool);
    }
    ms_pColPool = nullptr;
}

int __cdecl AddColSlot(const char* name) {
    ColDef* def = ColDefPool_New_Game(ms_pColPool);
    if (def) {
        def->Construct();
    }
    // (a full pool: the game writes through nullptr)
    def->loaded = 0;
    def->loadAtStart = 0;
    def->unknown2B = 0;
    def->unknown2A = 0;
    ResetBounds(def->bounds);
    def->areas[0] = 0xFFFFFFFF;
    def->areas[1] = 0xFFFFFFFF;
    def->firstModel = 0x7FFF;
    def->lastModel = -0x8000;
    def->nameHash = HashName(name);
    def->data = nullptr;
    def->size = 0;
    return ms_pColPool->GetIndex(def);
}

int __cdecl FindColSlot(const char* name) {
    for (int slot = 0; slot < ms_pColPool->size; slot++) {
        ColDef* def = Def(slot);
        if (def && def->nameHash == HashName(name)) {
            return slot;
        }
    }
    return -1;
}

const char* __cdecl GetColName(int slot) {
    return ColDef_GetName(Def(slot));
}

void __cdecl SetArea(int slot, int area) {
    ColDef* def = Def(slot);
    uint32_t* word = area >= 32 ? &def->areas[uint32_t(area) >> 5] : &def->areas[0];
    *word |= 1u << (area & 31);
}

CRect* __cdecl GetBoundingBox(int slot) {
    // (8 for a free slot)
    return reinterpret_cast<CRect*>(reinterpret_cast<uintptr_t>(Def(slot)) + offsetof(ColDef, bounds));
}

void __cdecl IncludeModelIndex(int slot, int model) {
    ColDef* def = Def(slot);
    if (model < def->firstModel) {
        def->firstModel = int16_t(model);
    }
    if (model > def->lastModel) {
        def->lastModel = int16_t(model);
    }
}

int __cdecl GetFirstModel(int slot) {
    return Def(slot)->firstModel;
}

int __cdecl GetLastModel(int slot) {
    return Def(slot)->lastModel;
}

void __cdecl SetLoadAtStart(int slot, uint8_t value) {
    Def(slot)->loadAtStart = value;
}

uint8_t __cdecl IsLoaded(int slot) {
    return Def(slot)->loaded;
}

uint8_t __cdecl HasData(int slot) {
    return Def(slot)->data != nullptr;
}

uint8_t __cdecl IsEntityFromCol(int slot, CEntity* entity) {
    ColDef* def = Def(slot);
    int model = ModelIndex(entity);
    // (unsigned compares, as the game's)
    if (uint32_t(model) < uint32_t(int(def->firstModel)) || uint32_t(model) > uint32_t(int(def->lastModel))) {
        return 0;
    }
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[model];
    if (!mi || !mi->colModel) {
        return 0;
    }
    auto* col = static_cast<CColModel*>(mi->colModel);
    return col->colSlot == uint32_t(slot) || col == CTempColModels::ms_colModels[0];
}

void __cdecl AddIgnoredName(const char* name) {
    ms_aIgnoredNames[ms_numIgnoredNames] = HashName(name);
    ms_numIgnoredNames++;
}

void __cdecl ClearIgnoredNames() {
    ms_numIgnoredNames = 0;
}

namespace {
void StartScan(EntityRectScan& scan, const ColDef* def, int slot) {
    scan.filter = Col_IsEntityFromCol;
    scan.rect = def->bounds;
    scan.lists = 9;
    scan.user = slot;
    scan.entities = nullptr;
    scan.capacity = 0;
    scan.count = 0;
}
} // namespace

uint8_t __cdecl LoadCol(int slot, void* data, size_t size) {
    ColDef* def = Def(slot);
    if (!def->data) {
        game::PushMemoryTag(0x18);
        def->data = Malloc(size);
        def->size = uint32_t(size);
        game::PopMemoryTag();
        std::memcpy(def->data, data, size);
    }
    // the entities of the file's models leave the world while their models change
    EntityRectScan scan;
    StartScan(scan, def, slot);
    uint32_t found = 0;
    if (!ms_bGeneratingBoxes && gbRefreshEntities) {
        found = EntityRectScan_Scan(&scan);
        if (found) {
            for (int i = 0; i < int(scan.count); i++) {
                World_Remove(scan.entities[i]);
            }
        }
    }
    uint8_t ok = def->firstModel <= def->lastModel ? LoadCollisionFile(def->data, def->size, slot)
                                                   : LoadCollisionFileFirstTime(def->data, def->size, slot);
    if (!ok) {
        // (the entities stay out of the world)
        if (scan.entities) {
            Free(scan.entities);
        }
        return 0;
    }
    def->loaded = 1;
    if (gbRefreshEntities && found) {
        for (int i = 0; i < int(scan.count); i++) {
            Entity_466260(scan.entities[i]);
            World_Add(scan.entities[i]);
        }
    }
    if (scan.entities) {
        Free(scan.entities);
    }
    return 1;
}

void __cdecl RemoveCol(int slot) {
    ColDef* def = Def(slot);
    def->loaded = 0;
    if (ms_bRemovingAll) {
        return;
    }
    EntityRectScan scan;
    StartScan(scan, def, slot);
    uint32_t found = 0;
    // (not for a file that never got its box)
    if (gbRefreshEntities && !(double(def->bounds.left) == 1000000.0)) {
        found = EntityRectScan_Scan(&scan);
        if (found) {
            for (int i = 0; i < int(scan.count); i++) {
                World_Remove(scan.entities[i]);
            }
        }
    }
    for (int model = def->firstModel; model <= def->lastModel; model++) {
        CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[model];
        if (mi) {
            auto* col = static_cast<CColModel*>(mi->colModel);
            if (col && col->colSlot == uint32_t(slot)) {
                ColModel_RemoveCollisionVolumes(col);
            }
            continue;
        }
        // a model with only its collision
        CColModel* col = GetColModelOfModel(model);
        if (col) {
            if (col->data && col->data->unknown18) {
                ColModel_Unregister(col);
            }
            RemoveColModelOfModel(model);
            ColModel_RemoveCollisionVolumes(col);
            ColModel_Delete(col);
        }
    }
    if (gbRefreshEntities && found) {
        for (int i = 0; i < int(scan.count); i++) {
            CEntity* entity = scan.entities[i];
            CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[ModelIndex(entity)];
            if (!mi->colModel) {
                ModelInfo_SetColModel(mi, CTempColModels::ms_colModels[0], 0);
            }
            Entity_466260(entity);
            World_Add(entity);
        }
    }
    if (scan.entities) {
        Free(scan.entities);
    }
}

namespace {
const char* const kColStoreBin = "Config\\Extra\\ColStore.bin";
}

void __cdecl LoadBoundingBoxes() {
    if (!ms_bLoadBoxes) {
        return;
    }
    int fd = OpenFile(kColStoreBin, "r", 1);
    int count;
    ReadFromFile(fd, &count, 4);
    for (int slot = 0; slot < count; slot++) {
        uint8_t present;
        ReadFromFile(fd, &present, 1);
        if (present) {
            CRect rect;
            ResetBounds(rect);
            ReadFromFile(fd, &rect, 16);
            Def(slot)->bounds = rect;
        }
        game::LoadingScreen("CColStore::Load()", "In loop", 0);
    }
    int written;
    ReadFromFile(fd, &written, 4);
    CloseFile(fd);
}

void __cdecl SaveBoundingBoxes() {
    if (!ms_bSaveBoxes) {
        return;
    }
    int fd = OpenFile(kColStoreBin, "w", 1);
    int count = ms_pColPool->size;
    WriteToFile(fd, &count, 4);
    int written = 0;
    for (int slot = 0; slot < count; slot++) {
        uint8_t present;
        if (Def(slot)) {
            written++;
            present = 1;
            WriteToFile(fd, &present, 1);
            WriteToFile(fd, &Def(slot)->bounds, 16);
        } else {
            present = 0;
            WriteToFile(fd, &present, 1);
        }
    }
    WriteToFile(fd, &written, 4);
    CloseFile(fd);
}

// (the game's profiler markers around these are empty functions: left out)
void __cdecl LoadAllCollision() {
    for (int slot = 1; slot < 500; slot++) {
        game::LoadingScreen(nullptr, nullptr, 0);
        if (Def(slot)) {
            game::RequestModel(slot + COL_ID_BASE, 0);
        }
    }
    game::LoadingScreen("CColStore::LoadAllCollision()", "CStreaming::LoadAllRequestedModels()", 0);
    LoadAllRequestedModels(0);
}

void __cdecl RemoveAllCollision() {
    ms_bRemovingAll = 1;
    int size = ms_pColPool->size;
    for (int id = 0; id < TXD_ID_BASE; id++) {
        CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
        if (!mi || !mi->colModel) {
            continue;
        }
        auto* col = static_cast<CColModel*>(mi->colModel);
        int slot = col->colSlot;
        if (slot > 0 && slot < size && Def(slot) && !(GetStreamingInfo(slot + COL_ID_BASE)->flags & STREAMING_KEEP_IN_MEMORY)) {
            ColModel_RemoveCollisionVolumes(col);
        }
    }
    for (int slot = 1; slot < 500; slot++) {
        if (Def(slot) && !(GetStreamingInfo(slot + COL_ID_BASE)->flags & STREAMING_KEEP_IN_MEMORY)) {
            game::RemoveModel(slot + COL_ID_BASE);
        }
    }
    ms_bRemovingAll = 0;
}

void __cdecl GenerateAllBoundingBoxes() {
    const char* title = "CColStore::GenerateAllBoundingBoxes()";
    ms_bGeneratingBoxes = 1;
    game::PushMemoryTag(0x18);
    if (gbColLoadAtStartOnly) {
        for (int slot = 1; slot < 500; slot++) {
            ColDef* def = Def(slot);
            if (def && def->loadAtStart) {
                game::RequestModel(slot + COL_ID_BASE, 0);
            }
        }
        game::LoadingScreen(title, "CStreaming::LoadAllRequestedModels()", 0);
        LoadAllRequestedModels(0);
    } else {
        // every file read once: loading one works out its box
        game::LoadingScreen(title, "CColStore::LoadAllCollision()", 0);
        Col_LoadAllCollision();
        game::LoadingScreen(title, "Load", 0);
        if (ms_bLoadBoxes) {
            Col_LoadBoundingBoxes();
        } else {
            GenerateBoundingBoxes();
        }
        game::LoadingScreen(title, "CColStore::RemoveAllCollision()", 0);
        Col_RemoveAllCollision();
    }
    game::LoadingScreen(title, "finish", 0);
    game::PopMemoryTag();
    ms_bGeneratingBoxes = 0;
}

void __cdecl LoadColAreas() {
    for (int slot = 0; slot < 500; slot++) {
        if (ColDef* def = Def(slot)) {
            def->areas[0] = 0;
            def->areas[1] = 0;
        }
    }
    int area = -1;
    int fd = OpenFile("Coll\\ColAreas.cfg", "r", 1);
    if (!fd) {
        return;
    }
    // area names on their own; the files in them below, indented
    char line[256];
    char name[256];
    while (ReadLine(fd, line, 256)) {
        if (line[0] == ';') {
            continue;
        }
        if (line[0] == '*') {
            break;
        }
        name[0] = '\0';
        std::memset(name + 1, 0, 255);
        if (!Sscanf(line, "%s", name) || !name[0]) {
            continue;
        }
        if (line[0] == ' ' || line[0] == '\t') {
            // (a name not found reads the flag before the pool's)
            int slot = Col_FindColSlot(name);
            if (Def(slot)) {
                Col_SetArea(slot, area);
            }
        } else {
            area = GetAreaFromName(name);
        }
    }
    // files in no area are in all of them
    for (int slot = 0; slot < 500; slot++) {
        ColDef* def = Def(slot);
        if (def && def->areas[0] == 0 && def->areas[1] == 0) {
            def->areas[0] = 0xFFFFFFFF;
            def->areas[1] = 0xFFFFFFFF;
        }
    }
    CloseFile(fd);
}

void __cdecl LoadCollision(const CVector* pos) {
    if (ms_disableStreaming) {
        return;
    }
    for (int slot = 1; slot < 500; slot++) {
        ColDef* def = Def(slot);
        if (!def) {
            continue;
        }
        bool wanted = InArea(def, currArea) && Rect_IsPointInsideMargin(&def->bounds, pos, ms_colLoadMargin);
        if (!wanted && gbExtraLoadPoint && InArea(def, gExtraLoadArea)) {
            // (only x and y are read)
            CVector extra{gExtraLoadX, gExtraLoadY, 0.0f};
            wanted = Rect_IsPointInsideMargin(&def->bounds, &extra, ms_colLoadMargin);
        }
        if (!wanted && Fn_42E430() && Fn_4A84A0() == 0xE) {
            wanted = Fn_70D9F0(obj_20C8704, &def->bounds);
        }
        if (!wanted) {
            wanted = (GetStreamingInfo(slot + COL_ID_BASE)->flags & STREAMING_KEEP_IN_MEMORY) != 0;
        }
        if (wanted) {
            if (!def->loaded) {
                game::LoadCol(slot, nullptr, 0);
                SetColLoaded(slot, 1);
            }
        } else if (def->loaded) {
            game::RemoveCol(slot);
            SetColLoaded(slot, 0);
        }
    }
}

void __cdecl EnsureCollisionIsInMemory(const CVector* pos) {
    if (ms_disableStreaming) {
        return;
    }
    for (int slot = 1; slot < 500; slot++) {
        ColDef* def = Def(slot);
        if (!def || !InArea(def, currArea) || !Rect_IsPointInsideMargin(&def->bounds, pos, ms_colLoadMargin)) {
            continue;
        }
        if (def->data && def->size) {
            continue;
        }
        game::RequestModel(slot + COL_ID_BASE, 0xC);
        if (Fn_4F12F0(obj_C3CC68) == 0) {
            int_BA6890 = 7;
            Fn_696150(obj_20C32A4, "LOADCOL", 0);
        }
        Fn_45B7B0();
        game::LoadingScreen("CColStore::EnsureCollisionIsInMemory()", "CStreaming::LoadAllRequestedModels()", 0);
        LoadAllRequestedModels(1);
        Fn_45B7D0();
    }
}

uint8_t __cdecl HasCollisionLoaded(const CVector* pos) {
    int area = currArea;
    for (int slot = 1; slot < 500; slot++) {
        ColDef* def = Def(slot);
        if (def && def->loaded && InArea(def, area) && Rect_IsPointInside(&def->bounds, pos)) {
            return 1;
        }
    }
    return 0;
}

uint8_t __cdecl HasCollisionLoadedExcept(const CVector* pos) {
    uint32_t numIgnored = ms_numIgnoredNames;
    for (int slot = 1; slot < 500; slot++) {
        ColDef* def = Def(slot);
        if (!def || !def->loaded || !InArea(def, currArea) || !Rect_IsPointInside(&def->bounds, pos)) {
            continue;
        }
        bool ignored = false;
        for (uint32_t i = 0; i < numIgnored; i++) {
            if (def->nameHash == ms_aIgnoredNames[i]) {
                ignored = true;
                break;
            }
        }
        if (!ignored) {
            return 1;
        }
    }
    return 0;
}

uint8_t __cdecl FindAreaAt(const CVector* pos, int* area) {
    int found = currArea;
    for (int slot = 1; slot < 500; slot++) {
        ColDef* def = Def(slot);
        if (!def || !def->loaded) {
            continue;
        }
        // one area only. (The game counts the mask's non-zero nibbles, so two areas
        // in one nibble pass, as the higher; and the second word's areas come out
        // without the 32.)
        int areas = 0;
        for (int w = 0; w < 2; w++) {
            uint32_t word = def->areas[w];
            bool many = false;
            for (uint32_t bits = word; bits != 0; bits >>= 4) {
                if (!(bits & 0xF)) {
                    continue;
                }
                if (++areas == 1) {
                    int highest = 0;
                    for (uint32_t t = word >> 1; t != 0; t >>= 1) {
                        highest++;
                    }
                    found = highest;
                } else {
                    many = true;
                    break;
                }
            }
            if (many) {
                break;
            }
        }
        if (areas != 1) {
            continue;
        }
        const CRect& r = def->bounds;
        if (r.left > pos->x || r.right < pos->x || r.bottom > pos->y || !(r.top >= pos->y)) {
            continue;
        }
        *area = found;
        return 1;
    }
    return 0;
}

} // namespace CColStore

// ---- the sphere model and query ----------------------------------------------------

namespace CColSphereModel {

void __cdecl Initialise() {
    if (!ms_pColModel) {
        CColModel* col = ColModel_New(sizeof(CColModel));
        if (col) {
            ColModel_Construct(col);
        }
        ms_pColModel = col;
    }
    if (!ms_pColData) {
        auto* data = static_cast<CCollisionData*>(Malloc(sizeof(CCollisionData)));
        if (data) {
            CollisionData_Construct(data);
        }
        ms_pColData = data;
    }
    if (!ms_pSphere) {
        ms_pSphere = static_cast<CColSphere*>(MallocTagged(sizeof(CColSphere), 0));
    }
}

void __cdecl Shutdown() {
    if (CColModel* col = ms_pColModel) {
        ColModel_RemoveCollisionVolumes(col);
        ColModel_Delete(col);
        ms_pColModel = nullptr;
    }
    ms_pSphere = nullptr;
    ms_pColData = nullptr;
}

} // namespace CColSphereModel

CColSphereQuery* CColSphereQuery::Construct(const CVector* c, float r, uint32_t listFlags, uint8_t* points, int32_t u60) {
    center = CVector{0.0f, 0.0f, 0.0f};
    matrix.attached = nullptr;
    radius = r;
    center = *c;
    lists = listFlags;
    colPoints = points;
    unknown60 = u60;
    // the one-sphere model, at the origin with this radius
    CColModel* col = CColSphereModel::ms_pColModel;
    col->sphereRadius = r;
    col->sphereCenter = CVector{0.0f, 0.0f, 0.0f};
    col->boxMin = CVector{-r, -r, -r};
    col->boxMax = CVector{r, r, r};
    CColSphere* sphere = CColSphereModel::ms_pSphere;
    sphere->center = CVector{0.0f, 0.0f, 0.0f};
    sphere->radius = r;
    CCollisionData* data = CColSphereModel::ms_pColData;
    col->data = data;
    data->numSpheres = 1;
    data->spheres = sphere;
    data->unknown20 = 0;
    data->numBoxes = 0;
    data->numTriangles = 0;
    CVector pos = *c;
    Matrix_SetTranslate(&matrix, &pos);
    return this;
}

int CColSphereQuery::ProcessEntity(CEntity* entity) {
    numColPoints = 0;
    void* entityCol = GetEntityColModel(entity);
    const float* entityMatrix = GetMatrix(entity);
    if (ProcessColModels(&matrix, CColSphereModel::ms_pColModel, entityMatrix, entityCol, colPoints, 0, 0, &numColPoints, 0)) {
        // each point holds a reference to the entity
        for (int i = 0; i < numColPoints; i++) {
            re::Ptr32<CEntity>* slot = reinterpret_cast<re::Ptr32<CEntity>*>(colPoints + i * 0x28 + 0xC);
            if (*slot == entity) {
                continue;
            }
            if (*slot) {
                CleanUpReference(*slot, slot);
            }
            *slot = entity;
            if (entity) {
                RegisterReference(entity, slot);
            }
        }
    }
    return numColPoints;
}

void CColSphereQuery::ScanList(re::Ptr32<void>* list) {
    uint32_t* node = static_cast<uint32_t*>(*list);
    while (node) {
        uint32_t entry = *node;
        CEntity* entity = GetEntityFromNode(int(entry & 0xF), int((entry >> 4) & 0x3FFF));
        node = (entry >> 18) == 0x3FFF ? nullptr : ms_pNodes + (entry >> 18);
        if (ScanCode(entity) == ms_nCurrentScanCode) {
            continue;
        }
        ScanCode(entity) = ms_nCurrentScanCode;
        uint8_t* e = reinterpret_cast<uint8_t*>(entity);
        if (!*reinterpret_cast<re::Ptr32<void>*>(e + 0x20)) {
            continue;
        }
        if (!*reinterpret_cast<re::Ptr32<void>*>(e + 0x1C) && !*reinterpret_cast<re::Ptr32<void>*>(e + 0x60)) {
            continue;
        }
        // the model is in the current chapter (a byte of bits; 8 bits' shift)
        CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[ModelIndex(entity)];
        if (!(mi->unknown0B & uint8_t(1u << (ms_currentChapter & 31)))) {
            continue;
        }
        if (*reinterpret_cast<re::Ptr32<void>*>(e + 0xF4)) {
            continue;
        }
        if ((lists & 0x80) && Entity_45EE90(entity)) {
            continue;
        }
        CVector c = center;
        if (!Entity_TouchesSphere(entity, &c, radius)) {
            continue;
        }
        void* entityCol = GetEntityColModel(entity);
        int first = numColPoints;
        if (!ProcessColModels(&matrix, CColSphereModel::ms_pColModel, GetMatrix(entity), entityCol, colPoints, 0, 0, &numColPoints,
                              unknown60)) {
            continue;
        }
        for (int i = first; i < numColPoints; i++) {
            re::Ptr32<CEntity>* slot = reinterpret_cast<re::Ptr32<CEntity>*>(colPoints + i * 0x28 + 0xC);
            if (*slot == entity) {
                continue;
            }
            if (*slot) {
                CleanUpReference(*slot, slot);
            }
            *slot = entity;
            RegisterReference(entity, slot);
        }
    }
}

int CColSphereQuery::Scan() {
    numColPoints = 0;
    int x0 = SectorLow(double(center.x) - radius);
    int y0 = SectorLow(double(center.y) - radius);
    int x1 = SectorHigh(double(radius) + center.x);
    int y1 = SectorHigh(double(center.y) + radius);
    NextScanCode();
    ForEachSectorList(x0, y0, x1, y1, lists, [this](re::Ptr32<void>* list) { ColSphereQuery_ScanList(this, list); });
    return numColPoints;
}

// ---- CTempColModels, the surface table ---------------------------------------------

void __cdecl CTempColModels::Shutdown() {
    for (int i = 0; i < 15; i++) {
        if (CColModel* col = ms_colModels[i]) {
            ColModel_RemoveCollisionVolumes(col);
            ColModel_Delete(col);
        }
    }
}

void CSurfaceTable::Load() {
    char line[256];
    line[0] = '\0';
    std::memset(line + 1, 0, 255);
    int fd = OpenFile("CONFIG\\Dat\\SURFDATA.DAT", "r", 1);
    // the number of surfaces, their flags (six 0/1 each), then a byte per surface
    int state = 0;
    int i = 0;
    int count = 0;
    int v[6] = {};   // (the game's aren't initialised: a short line keeps the last ones)
    while (ReadLine(fd, line, 256)) {
        const char* p = line;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            p++;
        }
        if (*p == '#' || *p == '\0') {
            continue;
        }
        if (state == 0) {
            Sscanf(p, "%d", &count);
            uint32_t* raw = static_cast<uint32_t*>(Malloc(CountedArraySize(uint32_t(count), 4)));
            if (raw) {
                *raw = uint32_t(count);
                flags = raw + 1;
            } else {
                flags = nullptr;
            }
            state = 1;
        } else if (state == 1) {
            Sscanf(p, "%d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
            uint32_t& f = flags[i];
            for (int bit = 0; bit < 6; bit++) {
                f = (f & ~(1u << bit)) | (uint32_t(v[bit] != 0) << bit);
            }
            if (++i >= count) {
                state = 2;
                i = 0;
            }
        } else if (state == 2) {
            int value;
            Sscanf(p, "%d", &value);
            unknown00[i] = uint8_t(value);
            if (++i >= 0x43) {
                break;
            }
        }
    }
    CloseFile(fd);
    loaded = 1;
}

CSurfaceTable* __cdecl SurfaceTable::Get() {
    if (!ms_table.loaded) {
        SurfaceTable_Load(&ms_table);
    }
    return &ms_table;
}

void __cdecl SurfaceTable::Shutdown() {
    if (ms_table.flags) {
        Free(ms_table.flags - 1);
    }
    ms_table.loaded = 0;
    ms_table.flags = nullptr;
}

// ---- registration: all original until tested ---------------------------------------

#define COL_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, fn, nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)
#define COL_FN(fn) reinterpret_cast<void*>(&fn)

COL_REVERSED("CRect::OverlapsWith", 0x0056ED60, re::MemberAddress(&CRect::OverlapsWith));
COL_REVERSED("CColSphere::Set", 0x0056EB80, re::MemberAddress(static_cast<void (CColSphere::*)(float, const CVector*)>(&CColSphere::Set)));
COL_REVERSED("CColSphere::Set4", 0x0056EBA0,
             re::MemberAddress(static_cast<void (CColSphere::*)(float, const CVector*, uint8_t, uint8_t)>(&CColSphere::Set)));
COL_REVERSED("CColSphere::IntersectRay", 0x0056EBD0, re::MemberAddress(&CColSphere::IntersectRay));
COL_REVERSED("CCollisionData::Construct", 0x0056DAD0, re::MemberAddress(&CCollisionData::Construct));
COL_REVERSED("CCollisionData::RemoveCollisionVolumes", 0x0056DB00, re::MemberAddress(&CCollisionData::RemoveCollisionVolumes));
COL_REVERSED("CColModel::Construct", 0x0056E7A0, re::MemberAddress(&CColModel::Construct));
COL_REVERSED("CColModel::RemoveCollisionVolumes", 0x0056E7D0, re::MemberAddress(&CColModel::RemoveCollisionVolumes));
COL_REVERSED("CColModel::SetOwner", 0x0056E770, re::MemberAddress(&CColModel::SetOwner));
COL_REVERSED("CColModel::GetVertex", 0x0056E680, re::MemberAddress(&CColModel::GetVertex));
COL_REVERSED("CColModel::GetWorldVertex", 0x0056E6E0, re::MemberAddress(&CColModel::GetWorldVertex));
COL_REVERSED("CColModel::SetWorldVertex", 0x0056E820, re::MemberAddress(&CColModel::SetWorldVertex));
COL_REVERSED("CColModel::GetWorldTriangleEdge", 0x0056E8C0, re::MemberAddress(&CColModel::GetWorldTriangleEdge));
COL_REVERSED("CColModel::GetWorldTriangle", 0x0056E9C0, re::MemberAddress(&CColModel::GetWorldTriangle));
COL_REVERSED("CColModel::New", 0x0056E7C0, COL_FN(CColModel::New));
COL_REVERSED("CColModel::Delete", 0x0056EB30, COL_FN(CColModel::Delete));
COL_REVERSED("ColDef::Construct", 0x0056EE70, re::MemberAddress(&ColDef::Construct));
COL_REVERSED("ColDef::GetName", 0x0056ED20, re::MemberAddress(&ColDef::GetName));
COL_REVERSED("ColDefPool::New", 0x0056EDF0, COL_FN(ColDefPool_New));
COL_REVERSED("ColDefPool::Init", 0x0056F4C0, COL_FN(ColDefPool_Init));
COL_REVERSED("EntityRectScan::Scan", 0x0056F8D0, re::MemberAddress(&EntityRectScan::Scan));
COL_REVERSED("EntityRectScan::ScanList", 0x0056EFD0, re::MemberAddress(&EntityRectScan::ScanList));
COL_REVERSED("CColStore::Initialise", 0x005705A0, COL_FN(CColStore::Initialise));
COL_REVERSED("CColStore::Shutdown", 0x00570620, COL_FN(CColStore::Shutdown));
COL_REVERSED("CColStore::AddColSlot", 0x0056F840, COL_FN(CColStore::AddColSlot));
COL_REVERSED("CColStore::FindColSlot", 0x0056EF10, COL_FN(CColStore::FindColSlot));
COL_REVERSED("CColStore::GetColName", 0x0056EEE0, COL_FN(CColStore::GetColName));
COL_REVERSED("CColStore::SetArea", 0x0056EE90, COL_FN(CColStore::SetArea));
COL_REVERSED("CColStore::GetBoundingBox", 0x0056F0F0, COL_FN(CColStore::GetBoundingBox));
COL_REVERSED("CColStore::IncludeModelIndex", 0x0056F120, COL_FN(CColStore::IncludeModelIndex));
COL_REVERSED("CColStore::GetFirstModel", 0x0056F3C0, COL_FN(CColStore::GetFirstModel));
COL_REVERSED("CColStore::GetLastModel", 0x0056F3F0, COL_FN(CColStore::GetLastModel));
COL_REVERSED("CColStore::SetLoadAtStart", 0x0056F420, COL_FN(CColStore::SetLoadAtStart));
COL_REVERSED("CColStore::IsLoaded", 0x0056F450, COL_FN(CColStore::IsLoaded));
COL_REVERSED("CColStore::HasData", 0x0056F480, COL_FN(CColStore::HasData));
COL_REVERSED("CColStore::IsEntityFromCol", 0x0056EF60, COL_FN(CColStore::IsEntityFromCol));
COL_REVERSED("CColStore::AddIgnoredName", 0x0056EDB0, COL_FN(CColStore::AddIgnoredName));
COL_REVERSED("CColStore::ClearIgnoredNames", 0x0056EDE0, COL_FN(CColStore::ClearIgnoredNames));
COL_REVERSED("CColStore::LoadCol", 0x0056FAC0, COL_FN(CColStore::LoadCol));
COL_REVERSED("CColStore::RemoveCol", 0x0056FC40, COL_FN(CColStore::RemoveCol));
COL_REVERSED("CColStore::LoadBoundingBoxes", 0x0056F160, COL_FN(CColStore::LoadBoundingBoxes));
COL_REVERSED("CColStore::SaveBoundingBoxes", 0x0056FF90, COL_FN(CColStore::SaveBoundingBoxes));
COL_REVERSED("CColStore::LoadAllCollision", 0x0056F330, COL_FN(CColStore::LoadAllCollision));
COL_REVERSED("CColStore::RemoveAllCollision", 0x0056F260, COL_FN(CColStore::RemoveAllCollision));
COL_REVERSED("CColStore::GenerateAllBoundingBoxes", 0x00570470, COL_FN(CColStore::GenerateAllBoundingBoxes));
COL_REVERSED("CColStore::LoadColAreas", 0x0056F570, COL_FN(CColStore::LoadColAreas));
COL_REVERSED("CColStore::LoadCollision", 0x0056FE00, COL_FN(CColStore::LoadCollision));
COL_REVERSED("CColStore::EnsureCollisionIsInMemory", 0x00570080, COL_FN(CColStore::EnsureCollisionIsInMemory));
COL_REVERSED("CColStore::HasCollisionLoaded", 0x005701C0, COL_FN(CColStore::HasCollisionLoaded));
COL_REVERSED("CColStore::HasCollisionLoadedExcept", 0x00570260, COL_FN(CColStore::HasCollisionLoadedExcept));
COL_REVERSED("CColStore::FindAreaAt", 0x00570330, COL_FN(CColStore::FindAreaAt));
COL_REVERSED("CColSphereModel::Initialise", 0x00570710, COL_FN(CColSphereModel::Initialise));
COL_REVERSED("CColSphereModel::Shutdown", 0x00570790, COL_FN(CColSphereModel::Shutdown));
COL_REVERSED("CColSphereQuery::Construct", 0x005707D0, re::MemberAddress(&CColSphereQuery::Construct));
COL_REVERSED("CColSphereQuery::ProcessEntity", 0x00570AA0, re::MemberAddress(&CColSphereQuery::ProcessEntity));
COL_REVERSED("CColSphereQuery::Scan", 0x00570B40, re::MemberAddress(&CColSphereQuery::Scan));
COL_REVERSED("CColSphereQuery::ScanList", 0x005708E0, re::MemberAddress(&CColSphereQuery::ScanList));
COL_REVERSED("CTempColModels::Shutdown", 0x00571240, COL_FN(CTempColModels::Shutdown));
COL_REVERSED("CSurfaceTable::Load", 0x00570D40, re::MemberAddress(&CSurfaceTable::Load));
COL_REVERSED("SurfaceTable::Get", 0x00570F40, COL_FN(SurfaceTable::Get));
COL_REVERSED("SurfaceTable::Shutdown", 0x00570F60, COL_FN(SurfaceTable::Shutdown));
