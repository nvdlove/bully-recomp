// NiScreenElementsData (src/gamebryo/NiScreenElementsData.h): screen-space polygons on an
// NiTriShapeData. Insert adds a polygon (a fan of triangles unless a list is given) and grows the
// arrays by their growth steps, SetRectangle / SetColors / SetTexture / SetTextures fill a
// polygon's vertices, RemoveAll empties it; with the streaming, comparison and viewer strings.
//
// The growth: the arrays grow by whole steps, round((excess / step) + 0.5) + 1 of them, in
// 16-bit arithmetic; the texture sets are copied at the new vertex count's stride, as the game
// has it.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometryData.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiScreenElementsData.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

using Data = NiScreenElementsData;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiScreenElementsData_RTTI, 0x020E34F4);
GAME_VAR(NiPoint3, NiPoint3_UNIT_Z, 0x00BC8AA8);

constexpr uintptr_t NiScreenElementsData_vtable = 0x00955260;

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, Bound_ComputeFromData, 0x007D2890, NiBound* self, uint32_t count, const NiPoint3* points);
// NiTriShapeData's (another chunk's)
GAME_FN(void, __thiscall, TriShapeData_Construct, 0x0077BA40, void* self);
GAME_FN(uint8_t, __thiscall, TriShapeData_IsEqual, 0x0077BB60, void* self, void* other);
GAME_FN(void, __thiscall, TriShapeData_Destruct, 0x0077BC80, void* self);
GAME_FN(void, __thiscall, TriShapeData_LoadBinary, 0x0077BDB0, void* self, void* stream);
GAME_FN(void, __thiscall, TriShapeData_SaveBinary, 0x0077C030, void* self, void* stream);
GAME_FN(void, __thiscall, TriShapeData_GetViewerStrings, 0x0077C1B0, void* self, NiTArray<char*>* strings);
GAME_FN(float*, __thiscall, GeometryData_GetTextureSet, 0x0077A000, void* self, uint32_t set);
// the viewer strings (NiViewerString.cpp's)
GAME_FN(char*, __cdecl, GetViewerStringShort, 0x00785650, const char* prefix, uint32_t value);

// a size for NiNew[]: -1 when the product overflows
inline uint32_t ArraySize(uint32_t count, uint32_t size) {
    uint64_t product = uint64_t(count) * size;
    return product > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(product);
}

// how much an array grows to hold `needed` more: whole steps (the quotient made a float, then
// truncated after adding 0.5, then one step more), in 16-bit arithmetic
inline uint16_t Grown(int32_t needed, uint16_t step, uint16_t current) {
    float steps = float(needed) / float(step);
    int32_t rounded = int32_t(steps + 0.5f);   // (the main thread's 24-bit x87, fistp truncating)
    return uint16_t((uint16_t(rounded) + 1) * step + current);
}

// the polygon a handle stands for (null for a bad handle or a free slot)
inline NiScreenElementsPolygon* PolygonOf(Data* self, int32_t polygon) {
    if (polygon < 0 || polygon >= int32_t(self->maxPQuantity)) {
        return nullptr;
    }
    uint16_t index = self->polygonIndexer[polygon];
    if (index == 0xFFFF) {
        return nullptr;
    }
    return &self->polygons[index];
}

// NiStream's binary reads and writes of `bytes` in one component of `componentSize`
inline void Load(void* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t size = componentSize;
    in->position += in->read(in, buffer, bytes, &size, 1);
}
inline void Save(void* stream, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t size = componentSize;
    out->position += out->write(out, buffer, bytes, &size, 1);
}

// ---- construction

// NiScreenElementsData(wantNormals, wantColors, textureSets, maxPolygons, polygonGrowBy,
// maxVertices, vertexGrowBy, maxTriangles, triangleGrowBy): counts of 0 or less made 1; the
// normals -Z, the colors zero (the normals written even when their allocation failed, the
// game's)
Data* __fastcall Construct(Data* self, re::Edx, int wantNormals, int wantColors, uint32_t textureSets,
                           int32_t maxPolygons, int32_t polygonGrowBy, int32_t maxVertices, int32_t vertexGrowBy,
                           int32_t maxTriangles, int32_t triangleGrowBy) {
    TriShapeData_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiScreenElementsData_vtable);
    self->maxPQuantity = uint16_t(maxPolygons > 0 ? maxPolygons : 1);
    self->pGrowBy = uint16_t(polygonGrowBy > 0 ? polygonGrowBy : 1);
    self->pQuantity = 0;
    self->polygons = static_cast<NiScreenElementsPolygon*>(NiMalloc(uint32_t(self->maxPQuantity) * 8));
    self->polygonIndexer = static_cast<uint16_t*>(NiMalloc(uint32_t(self->maxPQuantity) * 2));
    crt::Memset(self->polygonIndexer, 0xFF, uint32_t(self->maxPQuantity) * 2);
    self->maxVQuantity = uint16_t(maxVertices > 0 ? maxVertices : 1);
    self->vGrowBy = uint16_t(vertexGrowBy > 0 ? vertexGrowBy : 1);
    self->vertexCount = 0;
    self->activeVertexCount = 0;
    self->vertices = static_cast<NiPoint3*>(NiNewArray(ArraySize(self->maxVQuantity, 0xC)));
    if (uint8_t(wantNormals) != 0) {
        self->normals = static_cast<NiPoint3*>(NiNewArray(ArraySize(self->maxVQuantity, 0xC)));
        for (uint16_t i = 0; i < self->maxVQuantity; i++) {
            self->normals[i].x = -NiPoint3_UNIT_Z.x;
            self->normals[i].y = -NiPoint3_UNIT_Z.y;
            self->normals[i].z = -NiPoint3_UNIT_Z.z;
        }
    } else {
        self->normals = nullptr;
    }
    float* colors = nullptr;
    if (uint8_t(wantColors) != 0) {
        uint32_t count = self->maxVQuantity;
        colors = static_cast<float*>(NiNewArray(ArraySize(count, 0x10)));
        if (colors != nullptr) {
            for (uint32_t i = 0; i < count * 4; i++) {
                colors[i] = 0.0f;
            }
        }
    }
    self->colors = colors;
    uint16_t sets = uint16_t(textureSets);
    if (sets > 0) {
        self->textures = static_cast<float*>(NiNewArray(ArraySize(uint32_t(self->maxVQuantity) * sets, 8)));
        self->dataFlags = uint16_t((self->dataFlags & ~0x3F) | (sets & 0x3F));
    } else {
        self->textures = nullptr;
    }
    self->maxIQuantity = uint16_t(maxTriangles > 0 ? maxTriangles * 3 : 3);
    self->iGrowBy = uint16_t(triangleGrowBy > 0 ? triangleGrowBy * 3 : 3);
    self->triangleCount = 0;
    At<uint16_t>(self, 0x42) = 0;
    self->triListLength = 0;
    self->triList = static_cast<uint16_t*>(NiMalloc(uint32_t(self->maxIQuantity) * 2));
    self->boundNeedsUpdate = 0;
    return self;
}

// the scalar deleting destructor (the destructor inlined)
Data* __fastcall DeletingDestructor(Data* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiScreenElementsData_vtable);
    NiFree(self->polygons);
    NiFree(self->polygonIndexer);
    TriShapeData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x70);
    }
    return self;
}

const RTTI* __fastcall GetRTTI(Data*, re::Edx) {
    return &NiScreenElementsData_RTTI;
}

// ---- the polygons

// Insert(vertices, triangles, triangle list): a new polygon of at least 3 vertices (a fan when
// there's no list; a list is needed for a triangle count), its handle (-1 when refused)
int32_t __fastcall Insert(Data* self, re::Edx, uint32_t numVerticesArg, uint32_t numTrianglesArg,
                          const uint16_t* triList) {
    uint16_t numVertices = uint16_t(numVerticesArg);
    if (numVertices < 3) {
        return -1;
    }
    uint16_t numTriangles = uint16_t(numTrianglesArg);
    if (numTriangles != 0 && triList == nullptr) {
        return -1;
    }
    if (numTriangles == 0) {
        numTriangles = uint16_t(numVertices - 2);
    }

    uint16_t index = self->pQuantity;
    uint32_t handle = index;
    self->pQuantity = uint16_t(index + 1);
    if (uint16_t(index + 1) > self->maxPQuantity) {
        uint16_t newMax = uint16_t(self->pGrowBy + self->maxPQuantity);
        NiScreenElementsPolygon* polygons = static_cast<NiScreenElementsPolygon*>(NiMalloc(uint32_t(newMax) * 8));
        crt::MemcpyS(polygons, uint32_t(self->maxPQuantity) * 8, self->polygons, uint32_t(self->maxPQuantity) * 8);
        NiFree(self->polygons);
        self->polygons = polygons;
        uint16_t* indexer = static_cast<uint16_t*>(NiMalloc(uint32_t(newMax) * 2));
        crt::MemcpyS(indexer, uint32_t(newMax) * 2, self->polygonIndexer, uint32_t(self->maxPQuantity) * 2);
        crt::Memset(indexer + self->maxPQuantity, 0xFF, (uint32_t(newMax) - self->maxPQuantity) * 2);
        NiFree(self->polygonIndexer);
        self->maxPQuantity = newMax;
        self->polygonIndexer = indexer;
    }
    NiScreenElementsPolygon* polygon = &self->polygons[index];
    polygon->numVertices = numVertices;
    polygon->vertexOffset = self->vertexCount;
    polygon->numTriangles = numTriangles;
    polygon->indexOffset = uint16_t(self->triangleCount * 3);
    for (uint16_t i = 0; i < self->pQuantity; i++) {
        if (self->polygonIndexer[i] == 0xFFFF) {
            self->polygonIndexer[i] = index;
            handle = i;
            break;
        }
    }

    // the vertices (slot 21: SetActiveVertexCount)
    Virtual<void>(self, 21, uint32_t(uint16_t(polygon->numVertices + self->vertexCount)));
    int32_t needed = int32_t(self->vertexCount) - int32_t(self->maxVQuantity);
    if (needed > 0) {
        uint16_t newMax = Grown(needed, self->vGrowBy, self->maxVQuantity);
        uint32_t count = newMax;
        uint32_t oldBytes = uint32_t(self->maxVQuantity) * 0xC;
        NiPoint3* vertices = static_cast<NiPoint3*>(NiNewArray(ArraySize(count, 0xC)));
        crt::MemcpyS(vertices, count * 0xC, self->vertices, oldBytes);
        NiDeleteArray(self->vertices, 0xC);
        self->vertices = vertices;
        if (self->normals != nullptr) {
            NiPoint3* normals = static_cast<NiPoint3*>(NiNewArray(ArraySize(count, 0xC)));
            crt::MemcpyS(normals, count * 0xC, self->normals, oldBytes);
            NiDeleteArray(self->normals, 0xC);
            self->normals = normals;
            for (uint16_t i = self->maxVQuantity; i < newMax; i++) {
                normals[i].x = -NiPoint3_UNIT_Z.x;
                normals[i].y = -NiPoint3_UNIT_Z.y;
                normals[i].z = -NiPoint3_UNIT_Z.z;
            }
        }
        if (self->colors != nullptr) {
            float* colors = static_cast<float*>(NiNewArray(ArraySize(count, 0x10)));
            if (colors != nullptr) {
                for (uint32_t i = 0; i < count * 4; i++) {
                    colors[i] = 0.0f;
                }
            }
            crt::MemcpyS(colors, count << 4, self->colors, uint32_t(self->maxVQuantity) << 4);
            NiDeleteArray(self->colors, 0x10);
            self->colors = colors;
        }
        uint16_t sets = uint16_t(self->dataFlags & 0x3F);
        if (sets > 0) {
            uint8_t* textures = static_cast<uint8_t*>(NiNewArray(ArraySize(uint32_t(sets) * count, 8)));
            uint32_t bytes = uint32_t(self->maxVQuantity) * 8;
            for (uint32_t s = 0; s < sets; s++) {
                uint32_t offset = uint32_t(self->vertexCount) * s * 8;
                crt::MemcpyS(textures + offset, bytes, re::PtrCast<uint8_t*>(self->textures) + offset, bytes);
            }
            NiDeleteArray(self->textures, 8);
            self->textures = reinterpret_cast<float*>(textures);
        }
        self->maxVQuantity = newMax;
    }

    // the triangles (slot 25: SetTriangleCount)
    Virtual<void>(self, 25, uint32_t(uint16_t(self->triangleCount + numTriangles)));
    uint32_t indices = uint32_t(numTriangles) * 3;
    self->triListLength += indices;
    needed = int32_t(self->triangleCount) * 3 - int32_t(self->maxIQuantity);
    if (needed > 0) {
        uint16_t newMax = Grown(needed, self->iGrowBy, self->maxIQuantity);
        uint32_t bytes = uint32_t(newMax) * 2;
        uint16_t* list = static_cast<uint16_t*>(NiMalloc(bytes));
        crt::MemcpyS(list, bytes, self->triList, uint32_t(self->maxIQuantity) * 2);
        NiFree(self->triList);
        self->triList = list;
        self->maxIQuantity = newMax;
    }
    uint16_t* to = self->triList + polygon->indexOffset;
    if (triList != nullptr) {
        crt::MemcpyS(to, indices * 2, triList, indices * 2);
        for (uint32_t i = 0; i < indices; i++) {
            to[i] = uint16_t(triList[i] + polygon->vertexOffset);
        }
    } else {
        for (uint16_t i = 0; i < numTriangles; i++) {
            to[0] = polygon->vertexOffset;
            to[1] = uint16_t(polygon->vertexOffset + i + 1);
            to[2] = uint16_t(polygon->vertexOffset + i + 2);
            to += 3;
        }
    }
    self->dirtyFlags |= 1;
    return int32_t(uint16_t(handle));
}

// RemoveAll: the polygons zeroed (the indexer left as it is), no vertices or triangles
void __fastcall RemoveAll(Data* self, re::Edx) {
    if (self->pQuantity == 0) {
        return;
    }
    crt::Memset(self->polygons, 0, uint32_t(self->pQuantity) * 8);
    self->pQuantity = 0;
    Virtual<void>(self, 21, 0u);
    Virtual<void>(self, 25, 0u);
    self->dirtyFlags |= 0xFFF;
    self->boundNeedsUpdate = 1;
}

// SetRectangle(polygon, left, top, width, height): a 4-vertex polygon's corners, counter-
// clockwise from the top left (the right and bottom edges summed in float)
uint8_t __fastcall SetRectangle(Data* self, re::Edx, int32_t handle, float left, float top, float width, float height) {
    NiScreenElementsPolygon* polygon = PolygonOf(self, handle);
    if (polygon == nullptr || polygon->numVertices != 4) {
        return 0;
    }
    float right = left + width;
    float bottom = top + height;
    NiPoint3* v = &self->vertices[polygon->vertexOffset];
    v[0].x = left;
    v[0].y = top;
    v[0].z = 0.0f;
    v[1].x = left;
    v[1].y = bottom;
    v[1].z = 0.0f;
    v[2].x = right;
    v[2].y = bottom;
    v[2].z = 0.0f;
    v[3].x = right;
    v[3].y = top;
    v[3].z = 0.0f;
    self->dirtyFlags |= 1;
    self->boundNeedsUpdate = 1;
    return 1;
}

// SetColors(polygon, color): every vertex of the polygon
uint8_t __fastcall SetColors(Data* self, re::Edx, int32_t handle, const uint32_t* color) {
    if (self->colors == nullptr) {
        return 0;
    }
    NiScreenElementsPolygon* polygon = PolygonOf(self, handle);
    if (polygon == nullptr || polygon->numVertices == 0) {
        return 0;
    }
    uint32_t* colors = re::PtrCast<uint32_t*>(self->colors) + uint32_t(polygon->vertexOffset) * 4;
    for (uint16_t i = 0; i < polygon->numVertices; i++) {
        colors[i * 4 + 0] = color[0];
        colors[i * 4 + 1] = color[1];
        colors[i * 4 + 2] = color[2];
        colors[i * 4 + 3] = color[3];
    }
    self->dirtyFlags |= 4;
    return 1;
}

// SetTexture(polygon, vertex, set, uv)
uint8_t __fastcall SetTexture(Data* self, re::Edx, int32_t handle, int32_t vertex, uint32_t set, const uint32_t* uv) {
    if (self->textures == nullptr) {
        return 0;
    }
    NiScreenElementsPolygon* polygon = PolygonOf(self, handle);
    if (polygon == nullptr || uint16_t(set) >= (self->dataFlags & 0x3F)) {
        return 0;
    }
    uint32_t* textures = reinterpret_cast<uint32_t*>(GeometryData_GetTextureSet(self, set));
    if (vertex < 0 || vertex >= int32_t(polygon->numVertices)) {
        return 0;
    }
    uint32_t i = uint32_t(polygon->vertexOffset) + vertex;
    textures[i * 2] = uv[0];
    textures[i * 2 + 1] = uv[1];
    self->dirtyFlags |= 8;
    return 1;
}

// SetTextures(polygon, set, left, top, right, bottom): a 4-vertex polygon's texture corners, as
// SetRectangle places them
uint8_t __fastcall SetTextures(Data* self, re::Edx, int32_t handle, uint32_t set, float left, float top, float right,
                               float bottom) {
    if (self->textures == nullptr) {
        return 0;
    }
    NiScreenElementsPolygon* polygon = PolygonOf(self, handle);
    if (polygon == nullptr || polygon->numVertices != 4 || uint16_t(set) >= (self->dataFlags & 0x3F)) {
        return 0;
    }
    float* uv = GeometryData_GetTextureSet(self, set) + uint32_t(polygon->vertexOffset) * 2;
    uv[0] = left;
    uv[1] = top;
    uv[2] = left;
    uv[3] = bottom;
    uv[4] = right;
    uv[5] = bottom;
    uv[6] = right;
    uv[7] = top;
    self->dirtyFlags |= 8;
    return 1;
}

// UpdateBound: the bound made from the vertices
void __fastcall UpdateBound(Data* self, re::Edx) {
    self->boundNeedsUpdate = 0;
    if (self->vertexCount > 0) {
        Bound_ComputeFromData(&self->bound, self->vertexCount, self->vertices);
    }
}

// ---- NiObject's

uint8_t __fastcall IsEqual(Data* self, re::Edx, Data* other) {
    if (!TriShapeData_IsEqual(self, other)) {
        return 0;
    }
    if (self->maxPQuantity != other->maxPQuantity || self->pGrowBy != other->pGrowBy ||
        self->pQuantity != other->pQuantity || self->maxVQuantity != other->maxVQuantity ||
        self->vGrowBy != other->vGrowBy || self->maxIQuantity != other->maxIQuantity ||
        self->iGrowBy != other->iGrowBy) {
        return 0;
    }
    uint32_t count = self->maxPQuantity;
    if (std::memcmp(self->polygons, other->polygons, count * 8) != 0) {
        return 0;
    }
    return std::memcmp(self->polygonIndexer, other->polygonIndexer, count * 2) == 0 ? 1 : 0;
}

void __fastcall LoadBinary(Data* self, re::Edx, void* stream) {
    TriShapeData_LoadBinary(self, stream);
    Load(stream, &self->maxPQuantity, 2, 2);
    self->polygons = static_cast<NiScreenElementsPolygon*>(NiMalloc(uint32_t(self->maxPQuantity) * 8));
    self->polygonIndexer = static_cast<uint16_t*>(NiMalloc(uint32_t(self->maxPQuantity) * 2));
    uint32_t count = self->maxPQuantity;
    Load(stream, self->polygons, count * 8, 8);
    Load(stream, self->polygonIndexer, count * 2, 2);
    Load(stream, &self->pGrowBy, 2, 2);
    Load(stream, &self->pQuantity, 2, 2);
    Load(stream, &self->maxVQuantity, 2, 2);
    Load(stream, &self->vGrowBy, 2, 2);
    Load(stream, &self->maxIQuantity, 2, 2);
    Load(stream, &self->iGrowBy, 2, 2);
}

void __fastcall SaveBinary(Data* self, re::Edx, void* stream) {
    TriShapeData_SaveBinary(self, stream);
    Save(stream, &self->maxPQuantity, 2, 2);
    uint32_t count = self->maxPQuantity;
    Save(stream, self->polygons, count * 8, 8);
    Save(stream, self->polygonIndexer, count * 2, 2);
    Save(stream, &self->pGrowBy, 2, 2);
    Save(stream, &self->pQuantity, 2, 2);
    Save(stream, &self->maxVQuantity, 2, 2);
    Save(stream, &self->vGrowBy, 2, 2);
    Save(stream, &self->maxIQuantity, 2, 2);
    Save(stream, &self->iGrowBy, 2, 2);
}

void __fastcall GetViewerStrings(Data* self, re::Edx, NiTArray<char*>* strings) {
    TriShapeData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiScreenElementsData_RTTI.name));
    AddViewerString(strings, GetViewerStringPointer("m_akPolygon", self->polygons));
    AddViewerString(strings, GetViewerStringPointer("m_ausPIndexer", self->polygonIndexer));
    AddViewerString(strings, GetViewerStringShort("m_usMaxPQuantity", self->maxPQuantity));
    AddViewerString(strings, GetViewerStringShort("m_usPGrowBy", self->pGrowBy));
    AddViewerString(strings, GetViewerStringShort("m_usPQuantity", self->pQuantity));
    AddViewerString(strings, GetViewerStringShort("m_usMaxVQuantity", self->maxVQuantity));
    AddViewerString(strings, GetViewerStringShort("m_usVGrowBy", self->vGrowBy));
    AddViewerString(strings, GetViewerStringShort("m_usMaxIQuantity", self->maxIQuantity));
    AddViewerString(strings, GetViewerStringShort("m_usIGrowBy", self->iGrowBy));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISCREENDATA_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISCREENDATA_REVERSED("NiScreenElementsData::UpdateBound", 0x00788830, &ni::UpdateBound);
NISCREENDATA_REVERSED("NiScreenElementsData::IsEqual", 0x007888A0, &ni::IsEqual);
NISCREENDATA_REVERSED("NiScreenElementsData::GetRTTI", 0x00788A50, &ni::GetRTTI);
NISCREENDATA_REVERSED("NiScreenElementsData::DeletingDestructor", 0x00788A60, &ni::DeletingDestructor);
NISCREENDATA_REVERSED("NiScreenElementsData::Insert", 0x00788AA0, &ni::Insert);
NISCREENDATA_REVERSED("NiScreenElementsData::RemoveAll", 0x00789010, &ni::RemoveAll);
NISCREENDATA_REVERSED("NiScreenElementsData::SetRectangle", 0x00789060, &ni::SetRectangle);
NISCREENDATA_REVERSED("NiScreenElementsData::SetColors", 0x00789160, &ni::SetColors);
NISCREENDATA_REVERSED("NiScreenElementsData::SetTexture", 0x007891F0, &ni::SetTexture);
NISCREENDATA_REVERSED("NiScreenElementsData::SetTextures", 0x00789300, &ni::SetTextures);
NISCREENDATA_REVERSED("NiScreenElementsData::NiScreenElementsData", 0x007893A0, &ni::Construct);
NISCREENDATA_REVERSED("NiScreenElementsData::LoadBinary", 0x007895D0, &ni::LoadBinary);
NISCREENDATA_REVERSED("NiScreenElementsData::SaveBinary", 0x00789750, &ni::SaveBinary);
NISCREENDATA_REVERSED("NiScreenElementsData::GetViewerStrings", 0x007898B0, &ni::GetViewerStrings);
