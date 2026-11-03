// NiSkinPartition (src/gamebryo/NiSkinPartition.h): MakePartitions, which splits a skinned mesh's
// triangles into partitions of at most so many bones (each vertex keeping at most so many
// weights), and its builders; the partitions' streaming and comparison.
//
// The builders' working types (all NiMalloc'd arrays grown by doubling, as NiTPrimitiveSet):
//   VertexInteractions: per vertex, its (bone, weight) pairs (8 bytes each), 0xC bytes, new[]'d
//   BoneSet: a sorted set of 16-bit bone (or triangle) indices, 0xC bytes, NiNew'd
//   BoneSetCollection: the BoneSets, a set of pointers
//   a NiTPointerMap<unsigned int, float> (vtable 0x952C48 on NiTMapBase's 0x960680, 37
//   buckets): a triangle's bones and their largest weights.
// The sorting goes through the C runtime's qsort / bsearch with the game's comparators.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSkinPartition.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiSkinPartition_RTTI, 0x020E2B94);

constexpr uintptr_t NiSkinPartition_vtable = 0x00952C00;
constexpr uintptr_t Partition_vtable = 0x00952BFC;
constexpr uintptr_t WeightMap_vtable = 0x00952C48;
constexpr uintptr_t TMapBase_vtable = 0x00960680;
constexpr uintptr_t Partition_ConstructAddress = 0x0076DE00;       // Partition::Partition
constexpr uintptr_t Interactions_ConstructAddress = 0x0066F280;    // three words zeroed
constexpr uintptr_t CompareShorts = 0x0076DDE0;                     // ascending 16-bit
constexpr uintptr_t CompareWeights = 0x0076DD90;                    // by weight, the largest first

using Partition = NiSkinPartitionPart;
using Compare = int(__cdecl*)(const void*, const void*);

struct Interaction {
    uint32_t bone;
    float weight;
};
struct VertexInteractions {
    re::Ptr32<Interaction> data;   // +0x0
    uint32_t allocated;  // +0x4
    uint32_t count;      // +0x8
};
static_assert(sizeof(VertexInteractions) == 0xC);

struct BoneSet {
    re::Ptr32<uint16_t> data;      // +0x0
    uint32_t allocated;  // +0x4
    uint32_t count;      // +0x8
};
static_assert(sizeof(BoneSet) == 0xC);

struct BoneSetCollection {
    re::Ptr32<re::Ptr32<BoneSet>> data;
    uint32_t allocated;
    uint32_t count;
};

// two sorted sets walked together (their union in order; 0xFFFF past an end)
struct BoneSetMerger {
    uint32_t a;          // +0x0 (the index in first)
    uint32_t b;          // +0x4 (the index in second)
    uint16_t valueA;     // +0x8
    uint16_t valueB;     // +0xA
    re::Ptr32<BoneSet> first;      // +0xC
    re::Ptr32<BoneSet> second;     // +0x10
};
static_assert(sizeof(BoneSetMerger) == 0x14);

struct MapItem {
    re::Ptr32<MapItem> next;
    uint32_t key;
    float value;
};
struct WeightMap {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t hashSize;   // +0x4
    re::Ptr32<re::Ptr32<MapItem>> table;     // +0x8
    uint32_t count;      // +0xC (its allocator's)
};

GAME_FN(void, __cdecl, Renderer_RemoveSkinPartition, 0x007556E0, NiSkinPartition* partition);
GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(void, __thiscall, TMap_RemoveAll, 0x005AC520, WeightMap* map);
GAME_FN(void, __thiscall, Allocator_FreeNode, 0x00401B30, void* allocator, MapItem* item);
GAME_FN(void, __thiscall, GeometryData_ComputeVertexOrder, 0x0077B450, void* data, re::Ptr32<uint16_t>* order,
        re::Ptr32<uint16_t>* inverse, uint32_t flags);
GAME_FN(int, __thiscall, GeometryData_CompareVertices, 0x0077A250, void* data, uint32_t first, uint32_t second,
        uint32_t flags);
GAME_FN(void, __thiscall, PointerSet_SetSize, 0x0083AD20, BoneSetCollection* set, uint32_t allocated);
// (this chunk's, called where modes apply)
GAME_FN(void, __thiscall, Partition_Destruct, 0x0076DE30, Partition* self);
GAME_FN(uint32_t, __thiscall, Partition_GetStripLengthSum, 0x0076DE80, const Partition* self);
GAME_FN(uint8_t, __thiscall, Partition_IsEqual, 0x0076DEA0, const Partition* self, const Partition* other);
GAME_FN(void, __thiscall, Interactions_Normalize, 0x0076E430, VertexInteractions* self);
GAME_FN(uint8_t, __thiscall, BoneSet_Contains, 0x0076E4F0, const BoneSet* self, const BoneSet* other);
GAME_FN(uint8_t, __thiscall, BoneSet_ContainsBonesOf, 0x0076E560, const BoneSet* self,
        const VertexInteractions* interactions);
GAME_FN(uint16_t, __thiscall, Merger_Next, 0x0076E5C0, BoneSetMerger* self);
GAME_FN(uint16_t*, __thiscall, Partition_MakeVertexMap, 0x0076E650, Partition* self, const BoneSet* triangles,
        void* data);
GAME_FN(void, __thiscall, Partition_MakeTriangleList, 0x0076E770, Partition* self, const BoneSet* triangles,
        void* data, const uint16_t* map);
GAME_FN(void, __thiscall, WeightMap_SetAt, 0x0076E930, WeightMap* self, uint32_t key, float value);
GAME_FN(void, __thiscall, Interactions_Realloc, 0x0076E9C0, VertexInteractions* self, uint32_t allocated);
GAME_FN(void, __thiscall, BoneSet_Realloc, 0x0076EA30, BoneSet* self, uint32_t allocated);
GAME_FN(uint32_t, __thiscall, BoneSet_UnionSize, 0x0076EA90, const BoneSet* self, const BoneSet* other);
GAME_FN(void, __thiscall, BoneSet_Merge, 0x0076EB10, BoneSet* self, const BoneSet* other);
GAME_FN(uint8_t, __thiscall, BoneSet_ContainsTriangle, 0x0076EBB0, const BoneSet* self, const uint16_t* triangle,
        const VertexInteractions* interactions);
GAME_FN(void, __thiscall, Partition_Create, 0x0076EBF0, Partition* self, const BoneSet* triangles,
        const BoneSet* bones, void* data, const VertexInteractions* interactions, int weighted, int palette,
        int bonesPerVertex, uint32_t skinBones);
GAME_FN(void, __thiscall, Partition_LoadBinary, 0x0076EFA0, Partition* self, void* stream);
GAME_FN(void, __thiscall, Partition_SaveBinary, 0x0076F2B0, Partition* self, void* stream);
GAME_FN(void, __thiscall, Skin_AddTriangleWeights, 0x0076F590, NiSkinPartition* self, WeightMap* map,
        const uint16_t* triangle, const VertexInteractions* interactions);
GAME_FN(int32_t, __cdecl, Skin_FindBoneSet, 0x0076F640, const BoneSetCollection* sets, const uint16_t* triangle,
        const VertexInteractions* interactions, uint8_t* used, int usedValue);
GAME_FN(void, __thiscall, TriangleSet_AddPartitionTriangles, 0x0076F810, BoneSet* self, int32_t partition,
        const int32_t* triangleParts, uint32_t triangles);
GAME_FN(uint8_t, __thiscall, Skin_RemoveExtraInteractions, 0x0076F870, NiSkinPartition* self, void* data,
        int maxBones, int maxBonesPerVertex, VertexInteractions* interactions);
GAME_FN(void, __thiscall, Skin_MergeBoneSets, 0x0076FEB0, NiSkinPartition* self, int maxBones,
        BoneSetCollection* sets);
GAME_FN(VertexInteractions*, __thiscall, Skin_CreateVertexInteractions, 0x00770000, NiSkinPartition* self,
        uint32_t vertices, void* skinData);
GAME_FN(void, __thiscall, Skin_CreateBoneSets, 0x00770120, NiSkinPartition* self, void* data,
        const VertexInteractions* interactions, BoneSetCollection* sets);
GAME_FN(void, __thiscall, Skin_AssignTriangles, 0x00770350, NiSkinPartition* self, void* data, void* skinData,
        int maxBones, const VertexInteractions* interactions, BoneSetCollection* sets, re::Ptr32<int32_t>* triangleParts);

// a geometry data's counts and triangles (NiGeometryData +0xC, NiTriBasedGeomData +0x40; slot
// 23 GetTriangleIndices)
inline uint32_t VertexCount(void* data) {
    return At<uint16_t>(data, 0xC);
}
inline uint32_t TriangleCount(void* data) {
    return At<uint16_t>(data, 0x40);
}
inline void GetTriangle(void* data, uint32_t index, uint16_t* triangle) {
    Virtual<void>(data, 23, int(uint16_t(index)), &triangle[0], &triangle[1], &triangle[2]);
}
inline bool Degenerate(const uint16_t* t) {
    return t[0] == t[1] || t[1] == t[2] || t[2] == t[0];
}

// NiNew[]'s size for count elements and the count before them (-1 when it overflows)
inline uint32_t ArrayBytes(uint32_t count, uint32_t size) {
    uint64_t product = uint64_t(count) * size;
    uint32_t bytes = (product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product);
    uint32_t total = bytes + 4;
    return total < bytes ? 0xFFFFFFFF : total;
}

inline void FreeBoneSet(BoneSet* set) {
    if (set != nullptr) {
        NiFree(set->data);
        NiDelete(set, 0xC);
    }
}

// one read or write of `bytes` in one component of `componentSize`
inline void LoadRaw(void* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}
inline void SaveRaw(void* stream, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// ---- Partition

// ~Partition (its arrays freed)
void __fastcall PartitionDestruct(Partition* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(Partition_vtable);
    NiFree(self->bones);
    NiFree(self->weights);
    NiFree(self->vertexMap);
    NiFree(self->bonePalette);
    NiFree(self->triList);
    NiFree(self->stripLengths);
}

// GetStripLengthSum
uint32_t __fastcall PartitionGetStripLengthSum(const Partition* self, re::Edx) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < self->strips; i++) {
        sum += self->stripLengths[i];
    }
    return sum;
}

// operator==: the counts, then the arrays' bytes (the palette: both missing or the same)
uint8_t __fastcall PartitionIsEqual(const Partition* self, re::Edx, const Partition* other) {
    if (self->vertices != other->vertices || self->triangles != other->triangles ||
        self->boneCount != other->boneCount || self->strips != other->strips ||
        self->bonesPerVertex != other->bonesPerVertex) {
        return 0;
    }
    if (std::memcmp(self->bones, other->bones, uint32_t(self->boneCount) * 2) != 0 ||
        std::memcmp(self->vertexMap, other->vertexMap, uint32_t(self->vertices) * 2) != 0) {
        return 0;
    }
    uint32_t perVertex = uint32_t(self->bonesPerVertex) * self->vertices;
    if (std::memcmp(self->weights, other->weights, perVertex * 4) != 0) {
        return 0;
    }
    if (self->strips == 0) {
        if (std::memcmp(self->triList, other->triList, uint32_t(self->triangles) * 6) != 0) {
            return 0;
        }
    } else {
        if (std::memcmp(self->stripLengths, other->stripLengths, uint32_t(self->strips) * 2) != 0) {
            return 0;
        }
        if (std::memcmp(self->triList, other->triList, Partition_GetStripLengthSum(self) * 2) != 0) {
            return 0;
        }
    }
    if ((self->bonePalette == nullptr) != (other->bonePalette == nullptr)) {
        return 0;
    }
    if (self->bonePalette == nullptr) {
        return 1;
    }
    return std::memcmp(self->bonePalette, other->bonePalette, perVertex) == 0 ? 1 : 0;
}

// MakeVertexMap(triangles, data): the partition's vertices numbered in the order its triangles
// use them (the vertex map: partition to mesh); the mesh-to-partition map returned (0xFFFF:
// unused), the caller frees it
uint16_t* __fastcall PartitionMakeVertexMap(Partition* self, re::Edx, const BoneSet* triangles, void* data) {
    uint32_t meshVertices = VertexCount(data);
    uint16_t* map = static_cast<uint16_t*>(NiMalloc(meshVertices * 2));
    for (uint32_t i = 0; i < meshVertices; i++) {
        map[i] = 0xFFFF;
    }
    uint32_t next = 0;
    for (uint32_t t = 0; t < self->triangles; t++) {
        uint16_t triangle[3];
        GetTriangle(data, triangles->data[t], triangle);
        for (int k = 0; k < 3; k++) {
            if (map[triangle[k]] == 0xFFFF) {
                map[triangle[k]] = uint16_t(next);
                next++;
            }
        }
    }
    uint16_t* vertexMap = static_cast<uint16_t*>(NiMalloc(next * 2));
    self->vertices = uint16_t(next);
    self->vertexMap = vertexMap;
    for (uint32_t v = 0; v < meshVertices; v++) {
        uint16_t local = map[v];
        if (local != 0xFFFF) {
            self->vertexMap[local] = uint16_t(v);
        }
    }
    return map;
}

// MakeTriangleList(triangles, data, map): the triangles in the partition's vertex numbers
void __fastcall PartitionMakeTriangleList(Partition* self, re::Edx, const BoneSet* triangles, void* data,
                                          const uint16_t* map) {
    self->triList = static_cast<uint16_t*>(NiMalloc(uint32_t(self->triangles) * 6));
    for (uint32_t t = 0; t < self->triangles; t++) {
        uint16_t triangle[3];
        GetTriangle(data, triangles->data[t], triangle);
        uint16_t* out = self->triList + t * 3;
        out[0] = map[triangle[0]];
        out[1] = map[triangle[1]];
        out[2] = map[triangle[2]];
    }
}

// Partition's vector deleting destructor (flags 2: the new[]'d array, its count before it)
void* __fastcall PartitionVectorDeletingDestructor(Partition* self, re::Edx, uint32_t flags) {
    if ((flags & 2) == 0) {
        Partition_Destruct(self);
        if (flags & 1) {
            NiDelete(self, 0x2C);
        }
        return self;
    }
    uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
    for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
        Partition* p = self + i;
        p->vtable = reinterpret_cast<void*>(Partition_vtable);
        NiFree(p->bones);
        NiFree(p->weights);
        NiFree(p->vertexMap);
        NiFree(p->bonePalette);
        NiFree(p->triList);
        NiFree(p->stripLengths);
    }
    if (flags & 1) {
        NiDeleteArray(block, 0x2C);
    }
    return block;
}

// Create(triangles, bones, data, interactions, weighted, palette, bonesPerVertex, skinBones):
// the partition's bones (at least bonesPerVertex of them, the extra ones 0), its vertices and
// triangles; weighted: each vertex's bonesPerVertex largest weights (largest first) with their
// palette entries; else every vertex a weight for each of the partition's bones (and, with
// palette, the trivial palette)
void __fastcall PartitionCreate(Partition* self, re::Edx, const BoneSet* triangles, const BoneSet* bones, void* data,
                                const VertexInteractions* interactions, int weighted, int palette,
                                int bonesPerVertex, uint32_t skinBones) {
    uint32_t perVertexMax = uint8_t(bonesPerVertex);
    int32_t count = int32_t(bones->count);
    uint32_t boneCount = count > int32_t(perVertexMax) ? uint32_t(count) : perVertexMax;
    self->boneCount = uint16_t(boneCount);
    self->triangles = uint16_t(triangles->count);
    self->bones = static_cast<uint16_t*>(NiMalloc(uint32_t(uint16_t(boneCount)) * 2));
    for (uint32_t i = 0; i < self->boneCount; i++) {
        self->bones[i] = i < bones->count ? bones->data[i] : 0;
    }
    uint16_t* map = Partition_MakeVertexMap(self, triangles, data);
    Partition_MakeTriangleList(self, triangles, data, map);
    NiFree(map);

    if (re::Flag(weighted)) {
        self->bonesPerVertex = uint16_t(perVertexMax);
        uint32_t entries = perVertexMax * self->vertices;
        self->weights = static_cast<float*>(NiMalloc(entries * 4));
        self->bonePalette = static_cast<uint8_t*>(NiMalloc(entries));
        uint32_t* local = static_cast<uint32_t*>(NiMalloc(skinBones * 4));   // skin bone to partition bone
        for (uint32_t i = 0; i < bones->count; i++) {
            local[self->bones[i]] = i;
        }
        float* weightOut = self->weights;
        uint8_t* paletteOut = self->bonePalette;
        float* weights = static_cast<float*>(NiMalloc(uint32_t(self->bonesPerVertex) * 4));
        uint8_t* entriesOf = static_cast<uint8_t*>(NiMalloc(self->bonesPerVertex));
        for (uint32_t v = 0; v < self->vertices; v++) {
            const VertexInteractions& vi = interactions[self->vertexMap[v]];
            uint32_t k = 0;
            for (; k < vi.count; k++) {
                weights[k] = vi.data[k].weight;
                entriesOf[k] = uint8_t(local[vi.data[k].bone]);
            }
            for (; k < self->bonesPerVertex; k++) {
                weights[k] = 0.0f;
                entriesOf[k] = 0;
            }
            // the largest first (a selection: each taken one replaced by -FLT_MAX)
            for (uint32_t m = 0; m < self->bonesPerVertex; m++) {
                float best = weights[0];
                uint32_t bestIndex = 0;
                for (uint32_t q = 1; q < self->bonesPerVertex; q++) {
                    if (best < weights[q]) {
                        best = weights[q];
                        bestIndex = q;
                    }
                }
                *weightOut++ = weights[bestIndex];
                *paletteOut++ = entriesOf[bestIndex];
                weights[bestIndex] = -3.40282347e+38f;
            }
        }
        NiFree(weights);
        NiFree(entriesOf);
        NiFree(local);
        return;
    }

    self->bonesPerVertex = self->boneCount;
    uint32_t entries = uint32_t(self->boneCount) * self->vertices;
    float* out = static_cast<float*>(NiMalloc(entries * 4));
    self->weights = out;
    for (uint32_t v = 0; v < self->vertices; v++) {
        const VertexInteractions& vi = interactions[self->vertexMap[v]];
        uint32_t b = 0;
        for (; b < bones->count; b++) {
            float weight = 0.0f;
            uint32_t bone = bones->data[b];
            for (uint32_t k = 0; k < vi.count; k++) {
                if (vi.data[k].bone == bone) {
                    weight = vi.data[k].weight;
                    break;
                }
            }
            *out++ = weight;
        }
        for (; b < self->boneCount; b++) {
            *out++ = 0.0f;
        }
    }
    if (re::Flag(palette)) {
        uint8_t* p = static_cast<uint8_t*>(NiMalloc(entries));
        self->bonePalette = p;
        for (uint32_t v = 0; v < self->vertices; v++) {
            for (uint32_t b = 0; b < self->boneCount; b++) {
                *p++ = uint8_t(b);
            }
        }
    }
}

// Partition::LoadBinary: the counts, then each array (most of them only when there's a flag
// byte saying so)
void __fastcall PartitionLoadBinary(Partition* self, re::Edx, void* stream) {
    LoadBytes(stream, &self->vertices, 2);
    LoadBytes(stream, &self->triangles, 2);
    LoadBytes(stream, &self->boneCount, 2);
    LoadBytes(stream, &self->strips, 2);
    LoadBytes(stream, &self->bonesPerVertex, 2);
    self->bones = static_cast<uint16_t*>(NiMalloc(uint32_t(self->boneCount) * 2));
    LoadRaw(stream, self->bones, uint32_t(self->boneCount) * 2, 2);
    uint8_t present;
    LoadBytes(stream, &present, 1);
    if (present) {
        self->vertexMap = static_cast<uint16_t*>(NiMalloc(uint32_t(self->vertices) * 2));
        LoadRaw(stream, self->vertexMap, uint32_t(self->vertices) * 2, 2);
    }
    uint32_t entries = uint32_t(self->bonesPerVertex) * self->vertices;
    LoadBytes(stream, &present, 1);
    if (present) {
        self->weights = static_cast<float*>(NiMalloc(entries * 4));
        LoadRaw(stream, self->weights, entries * 4, 4);
    }
    uint32_t indices;
    if (self->strips == 0) {
        indices = uint32_t(self->triangles) * 3;
    } else {
        self->stripLengths = static_cast<uint16_t*>(NiMalloc(uint32_t(self->strips) * 2));
        LoadRaw(stream, self->stripLengths, uint32_t(self->strips) * 2, 2);
        indices = 0;
        for (uint32_t i = 0; i < self->strips; i++) {
            indices += self->stripLengths[i];
        }
    }
    LoadBytes(stream, &present, 1);
    if (present) {
        self->triList = static_cast<uint16_t*>(NiMalloc(indices * 2));
        LoadRaw(stream, self->triList, indices * 2, 2);
    }
    LoadBytes(stream, &present, 1);
    if (present) {
        self->bonePalette = static_cast<uint8_t*>(NiMalloc(entries));
        LoadRaw(stream, self->bonePalette, entries, 1);
    }
}

void __fastcall PartitionSaveBinary(Partition* self, re::Edx, void* stream) {
    SaveBytes(stream, &self->vertices, 2);
    SaveBytes(stream, &self->triangles, 2);
    SaveBytes(stream, &self->boneCount, 2);
    SaveBytes(stream, &self->strips, 2);
    SaveBytes(stream, &self->bonesPerVertex, 2);
    SaveRaw(stream, self->bones, uint32_t(self->boneCount) * 2, 2);
    uint8_t present = self->vertexMap != nullptr ? 1 : 0;
    SaveBytes(stream, &present, 1);
    if (present) {
        SaveRaw(stream, self->vertexMap, uint32_t(self->vertices) * 2, 2);
    }
    uint32_t entries = uint32_t(self->bonesPerVertex) * self->vertices;
    present = self->weights != nullptr ? 1 : 0;
    SaveBytes(stream, &present, 1);
    if (present) {
        SaveRaw(stream, self->weights, entries * 4, 4);
    }
    uint32_t indices;
    if (self->strips == 0) {
        indices = uint32_t(self->triangles) * 3;
    } else {
        SaveRaw(stream, self->stripLengths, uint32_t(self->strips) * 2, 2);
        indices = 0;
        for (uint32_t i = 0; i < self->strips; i++) {
            indices += self->stripLengths[i];
        }
    }
    present = self->triList != nullptr ? 1 : 0;
    SaveBytes(stream, &present, 1);
    if (present) {
        SaveRaw(stream, self->triList, indices * 2, 2);
    }
    present = self->bonePalette != nullptr ? 1 : 0;
    SaveBytes(stream, &present, 1);
    if (present) {
        SaveRaw(stream, self->bonePalette, entries, 1);
    }
}

// ---- the builders' sets

// VertexInteractions::Normalize: the weights made to sum to 1
void __fastcall InteractionsNormalize(VertexInteractions* self, re::Edx) {
    float sum = 0.0f;
    uint32_t count = self->count;
    for (uint32_t i = 0; i < count; i++) {
        sum = self->data[i].weight + sum;
    }
    float inv = 1.0f / sum;
    for (uint32_t i = 0; i < self->count; i++) {
        self->data[i].weight = inv * self->data[i].weight;
    }
}

// BoneSet::Contains(other): every bone of the (sorted) other in this (sorted) set
uint8_t __fastcall BoneSetContains(const BoneSet* self, re::Edx, const BoneSet* other) {
    uint32_t i = 0;
    for (uint32_t j = 0; j < other->count; j++) {
        uint32_t count = self->count;
        if (i < count) {
            uint16_t wanted = other->data[j];
            while (self->data[i] < wanted) {
                i++;
                if (i >= count) {
                    break;
                }
            }
        }
        if (i == count || self->data[i] != other->data[j]) {
            return 0;
        }
        i++;
    }
    return 1;
}

// whether every bone a vertex's interactions name is in this set
uint8_t __fastcall BoneSetContainsBonesOf(const BoneSet* self, re::Edx, const VertexInteractions* interactions) {
    uint32_t count = interactions->count;
    for (uint32_t k = 0; k < count; k++) {
        uint32_t key = *reinterpret_cast<const uint16_t*>(&interactions->data[k].bone);
        if (crt::Bsearch(&key, self->data, self->count, 2, reinterpret_cast<Compare>(CompareShorts)) == nullptr) {
            return 0;
        }
    }
    return 1;
}

// the merger's next value (the smaller of the two; both advanced when equal)
uint16_t __fastcall MergerNext(BoneSetMerger* self, re::Edx) {
    uint16_t a = self->valueA;
    uint16_t b = self->valueB;
    if (b < a) {
        self->b++;
        self->valueB = self->b < self->second->count ? self->second->data[self->b] : 0xFFFF;
        return b;
    }
    if (b == a) {
        self->b++;
        self->valueB = self->b < self->second->count ? self->second->data[self->b] : 0xFFFF;
    }
    self->a++;
    self->valueA = self->a < self->first->count ? self->first->data[self->a] : 0xFFFF;
    return a;
}

// NiTMapBase<unsigned int, float>::SetAt
void __fastcall WeightMapSetAt(WeightMap* self, re::Edx, uint32_t key, float value) {
    uint32_t index = key % self->hashSize;
    for (MapItem* item = self->table[index]; item != nullptr; item = item->next) {
        if (key == item->key) {
            item->value = value;
            return;
        }
    }
    MapItem* item = Virtual<MapItem*>(self, 3);   // NewItem
    Virtual<void>(self, 1, item, key, value);      // SetValue
    item->next = self->table[index];
    self->table[index] = item;
    self->count++;
}

// NiTPointerMap<unsigned int, float>::DeleteItem: the value cleared, the item given back to the
// allocator (+0xC)
void __fastcall WeightMapDeleteItem(WeightMap* self, re::Edx, MapItem* item) {
    item->value = 0.0f;
    Allocator_FreeNode(reinterpret_cast<uint8_t*>(self) + 0xC, item);
}

// the sets' Realloc(newSize) (NiTPrimitiveSet::Realloc: the elements kept, as many as fit)
void __fastcall InteractionsRealloc(VertexInteractions* self, re::Edx, uint32_t allocated) {
    if (allocated == self->allocated) {
        return;
    }
    Interaction* data = nullptr;
    if (allocated != 0) {
        data = static_cast<Interaction*>(NiMalloc(allocated * 8));
        for (uint32_t i = 0; i < self->count; i++) {
            data[i] = self->data[i];
        }
    }
    NiFree(self->data);
    self->data = data;
    self->allocated = allocated;
}

void __fastcall BoneSetRealloc(BoneSet* self, re::Edx, uint32_t allocated) {
    if (allocated == self->allocated) {
        return;
    }
    uint16_t* data = nullptr;
    if (allocated != 0) {
        data = static_cast<uint16_t*>(NiMalloc(allocated * 2));
        for (uint32_t i = 0; i < self->count; i++) {
            data[i] = self->data[i];
        }
    }
    NiFree(self->data);
    self->data = data;
    self->allocated = allocated;
}

// a merger over two sets (their first values read as they are, even from an empty set)
inline BoneSetMerger MakeMerger(const BoneSet* first, const BoneSet* second) {
    BoneSetMerger merger;
    merger.a = 0;
    merger.b = 0;
    merger.valueA = first->data[0];
    merger.valueB = second->data[0];
    merger.first = const_cast<BoneSet*>(first);
    merger.second = const_cast<BoneSet*>(second);
    return merger;
}

// UnionSize(other): the size of the two sets' union
uint32_t __fastcall BoneSetUnionSize(const BoneSet* self, re::Edx, const BoneSet* other) {
    BoneSetMerger merger = MakeMerger(self, other);
    uint32_t count = 0;
    while (merger.valueA != 0xFFFF || merger.valueB != 0xFFFF) {
        Merger_Next(&merger);
        count++;
    }
    return count;
}

// Merge(other): this set becomes the union
void __fastcall BoneSetMerge(BoneSet* self, re::Edx, const BoneSet* other) {
    uint32_t allocated = other->count + self->count;
    BoneSetMerger merger = MakeMerger(self, other);
    uint16_t* data = static_cast<uint16_t*>(NiMalloc(allocated * 2));
    uint32_t count = 0;
    while (merger.valueA != 0xFFFF || merger.valueB != 0xFFFF) {
        data[count] = Merger_Next(&merger);
        count++;
    }
    NiFree(self->data);
    self->allocated = allocated;
    self->data = data;
    self->count = count;
}

// ContainsTriangle(triangle, interactions): the bones of all three vertices in this set
uint8_t __fastcall BoneSetContainsTriangle(const BoneSet* self, re::Edx, const uint16_t* triangle,
                                           const VertexInteractions* interactions) {
    for (int k = 0; k < 3; k++) {
        if (!BoneSet_ContainsBonesOf(self, &interactions[triangle[k]])) {
            return 0;
        }
    }
    return 1;
}

// the triangles assigned to a partition added to this set (their indices)
void __fastcall TriangleSetAddPartitionTriangles(BoneSet* self, re::Edx, int32_t partition,
                                                 const int32_t* triangleParts, uint32_t triangles) {
    for (uint32_t t = 0; t < triangles; t++) {
        if (triangleParts[t] == partition) {
            if (self->count == self->allocated) {
                BoneSet_Realloc(self, self->allocated != 0 ? self->allocated * 2 : 1);
            }
            self->data[self->count] = uint16_t(t);
            self->count++;
        }
    }
}

// ---- NiSkinPartition's builders

// a triangle's bones in the map, each with its largest weight over the three vertices
void __fastcall AddTriangleWeights(NiSkinPartition*, re::Edx, WeightMap* map, const uint16_t* triangle,
                                   const VertexInteractions* interactions) {
    for (int k = 0; k < 3; k++) {
        const VertexInteractions& vi = interactions[triangle[k]];
        uint32_t count = vi.count;
        for (uint32_t i = 0; i < count; i++) {
            const Interaction& interaction = vi.data[i];
            uint32_t bone = interaction.bone;
            bool keep = false;
            for (MapItem* item = map->table[bone % map->hashSize]; item != nullptr; item = item->next) {
                if (bone == item->key) {
                    float existing = item->value;
                    keep = !(interaction.weight > existing);
                    break;
                }
            }
            if (!keep) {
                WeightMap_SetAt(map, bone, interaction.weight);
            }
        }
    }
}

// the first set (whose used flag is usedValue) that holds the triangle's bones: marked used
int32_t __cdecl FindBoneSet(const BoneSetCollection* sets, const uint16_t* triangle,
                            const VertexInteractions* interactions, uint8_t* used, int usedValue) {
    for (uint32_t i = 0; i < sets->count; i++) {
        if (used[i] == uint8_t(usedValue) && BoneSet_ContainsTriangle(sets->data[i], triangle, interactions)) {
            used[i] = 1;
            return int32_t(i);
        }
    }
    return -1;
}

// the growing set of vertex indices (NiTPrimitiveSet<unsigned int>'s Add, inlined)
struct IndexSet {
    uint32_t* data = nullptr;
    uint32_t allocated = 0;
    uint32_t count = 0;

    void Add(uint32_t value) {
        if (count == allocated) {
            uint32_t grown = allocated != 0 ? allocated * 2 : 1;
            if (grown != allocated) {
                uint32_t* copy = nullptr;
                if (grown != 0) {
                    copy = static_cast<uint32_t*>(NiMalloc(grown * 4));
                    for (uint32_t i = 0; i < count; i++) {
                        copy[i] = data[i];
                    }
                }
                NiFree(data);
                data = copy;
                allocated = grown;
            }
        }
        data[count] = value;
        count++;
    }
};

// two vertices with the same bones in the same order
inline bool SameBones(const VertexInteractions& a, const VertexInteractions& b) {
    if (a.count != b.count) {
        return false;
    }
    for (uint32_t k = 0; k < a.count; k++) {
        if (a.data[k].bone != b.data[k].bone) {
            return false;
        }
    }
    return true;
}

// the map's inlined destructor and the builder's arrays freed
inline void FreeBuilder(WeightMap* map, uint16_t* order, uint16_t* inverse, uint32_t* group) {
    NiFree(order);
    NiFree(inverse);
    NiFree(group);
    map->vtable = reinterpret_cast<void*>(WeightMap_vtable);
    TMap_RemoveAll(map);
    map->vtable = reinterpret_cast<void*>(TMapBase_vtable);
    TMap_RemoveAll(map);
    NiFree(map->table);
}

// RemoveExtraInteractions(data, maxBones, maxBonesPerVertex, interactions): while a triangle
// needs more than maxBones bones, its least-weighted bone is dropped from its vertices (and from
// every vertex at the same position with the same bones), the rest renormalized; false when a
// vertex loses its last bone. Then every vertex keeps at most maxBonesPerVertex weights
uint8_t __fastcall RemoveExtraInteractions(NiSkinPartition* self, re::Edx, void* data, int maxBones,
                                           int maxBonesPerVertex, VertexInteractions* interactions) {
    uint32_t triangles = TriangleCount(data);
    uint32_t vertices = VertexCount(data);
    WeightMap map;
    map.vtable = reinterpret_cast<void*>(TMapBase_vtable);
    map.hashSize = 0x25;
    map.count = 0;
    map.table = static_cast<re::Ptr32<MapItem>*>(NiMalloc(0x94));
    crt::Memset(map.table, 0, map.hashSize * 4);
    map.vtable = reinterpret_cast<void*>(WeightMap_vtable);
    IndexSet group;
    re::Ptr32<uint16_t> order = nullptr;
    re::Ptr32<uint16_t> inverse = nullptr;
    GeometryData_ComputeVertexOrder(data, &order, &inverse, 1);

    for (uint32_t t = 0; t < triangles; t++) {
        uint16_t triangle[3];
        GetTriangle(data, t, triangle);
        if (Degenerate(triangle)) {
            continue;
        }
        Skin_AddTriangleWeights(self, &map, triangle, interactions);
        uint32_t remaining = map.count;
        if (remaining > uint8_t(maxBones)) {
            for (;;) {
                // the least-weighted bone (map order; the first of equals)
                MapItem* item = nullptr;
                for (uint32_t b = 0; b < map.hashSize; b++) {
                    if (map.table[b] != nullptr) {
                        item = map.table[b];
                        break;
                    }
                }
                float least = 3.40282347e+38f;
                uint32_t leastBone = 0;
                do {
                    float weight = item->value;
                    uint32_t key = item->key;
                    MapItem* next = item->next;
                    if (next == nullptr) {
                        for (uint32_t b = key % map.hashSize + 1; b < map.hashSize; b++) {
                            if (map.table[b] != nullptr) {
                                next = map.table[b];
                                break;
                            }
                        }
                    }
                    if (least > weight) {
                        least = weight;
                        leastBone = key;
                    }
                    item = next;
                } while (item != nullptr);

                for (int k = 0; k < 3; k++) {
                    uint32_t vertex = triangle[k];
                    if (group.allocated == 0) {
                        uint32_t* one = static_cast<uint32_t*>(NiMalloc(4));
                        NiFree(group.data);
                        group.data = one;
                        group.allocated = 1;
                    }
                    group.data[0] = vertex;
                    group.count = 1;
                    const VertexInteractions& mine = interactions[vertex];
                    uint32_t position = inverse[vertex];
                    // the vertices at the same position (the sorted order's neighbours) with the
                    // same bones
                    for (uint32_t p = position; p != 0; p--) {
                        uint32_t other = order[p - 1];
                        if (GeometryData_CompareVertices(data, other, vertex, 1) != 0) {
                            break;
                        }
                        if (SameBones(mine, interactions[other])) {
                            group.Add(other);
                        }
                    }
                    for (uint32_t p = position; p < vertices - 1; p++) {
                        uint32_t other = order[p + 1];
                        if (GeometryData_CompareVertices(data, other, vertex, 1) != 0) {
                            break;
                        }
                        if (SameBones(mine, interactions[other])) {
                            group.Add(other);
                        }
                    }
                    for (uint32_t g = 0; g < group.count; g++) {
                        VertexInteractions& vi = interactions[group.data[g]];
                        uint32_t count = vi.count;
                        uint32_t found = 0;
                        for (; found < count; found++) {
                            if (vi.data[found].bone == leastBone) {
                                break;
                            }
                        }
                        if (found == count) {
                            continue;
                        }
                        for (uint32_t d = found; d < vi.count - 1; d++) {
                            vi.data[d] = vi.data[d + 1];
                        }
                        vi.count--;
                        if (vi.count == 0) {
                            FreeBuilder(&map, order, inverse, group.data);
                            return 0;
                        }
                        Interactions_Normalize(&vi);
                    }
                }
                remaining--;
                if (remaining == uint8_t(maxBones)) {
                    break;
                }
                TMap_RemoveAll(&map);
                Skin_AddTriangleWeights(self, &map, triangle, interactions);
            }
        }
        TMap_RemoveAll(&map);
    }

    uint32_t perVertex = uint8_t(maxBonesPerVertex);
    for (uint32_t v = 0; v < vertices; v++) {
        VertexInteractions& vi = interactions[v];
        if (vi.count > perVertex) {
            crt::Qsort(vi.data, vi.count, 8, reinterpret_cast<Compare>(CompareWeights));
            vi.count = perVertex;
            Interactions_Normalize(&vi);
        }
    }
    FreeBuilder(&map, order, inverse, group.data);
    return 1;
}

// MergeBoneSets(maxBones, sets): while two sets fit together in maxBones, the pair sharing the
// most bones merged (and every set the merged one now contains dropped)
void __fastcall MergeBoneSets(NiSkinPartition*, re::Edx, int maxBones, BoneSetCollection* sets) {
    uint32_t limit = uint8_t(maxBones);
    for (;;) {
        uint32_t first = 0;
        uint32_t second = 0;
        int32_t best = -1;
        if (sets->count == 0) {
            return;
        }
        for (uint32_t i = 0; i < sets->count; i++) {
            BoneSet* a = sets->data[i];
            for (uint32_t j = i + 1; j < sets->count; j++) {
                BoneSet* b = sets->data[j];
                uint32_t size = BoneSet_UnionSize(a, b);
                if (size <= limit) {
                    int32_t shared = int32_t(b->count + a->count - size);
                    if (shared > best) {
                        first = i;
                        second = j;
                        best = shared;
                    }
                }
            }
        }
        if (best < 0) {
            return;
        }
        BoneSet_Merge(sets->data[first], sets->data[second]);
        FreeBoneSet(sets->data[second]);
        sets->count--;
        sets->data[second] = sets->data[sets->count];
        BoneSet* merged = sets->data[first];
        for (uint32_t k = 0; k < sets->count;) {
            BoneSet* other = sets->data[k];
            if (other != merged && BoneSet_Contains(merged, other)) {
                FreeBoneSet(other);
                sets->count--;
                sets->data[k] = sets->data[sets->count];
            } else {
                k++;
            }
        }
    }
}

// CreateVertexInteractions(vertices, skinData): each vertex's (bone, weight) pairs from the
// skin's bones (NiSkinData: its bone data +0x40, 0x4C bytes each: the vertex weights +0x44, their
// count +0x48; the bone count +0x44), normalized
VertexInteractions* __fastcall CreateVertexInteractions(NiSkinPartition*, re::Edx, uint32_t vertices, void* skinData) {
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(ArrayBytes(vertices, 0xC)));
    VertexInteractions* interactions = nullptr;
    if (block != nullptr) {
        *block = vertices;
        interactions = reinterpret_cast<VertexInteractions*>(block + 1);
        VectorConstruct(interactions, 0xC, vertices, reinterpret_cast<void*>(Interactions_ConstructAddress));
    }
    uint32_t bones = At<uint32_t>(skinData, 0x44);
    for (uint32_t b = 0; b < bones; b++) {
        uint8_t* boneData = At<uint8_t*>(skinData, 0x40) + b * 0x4C;
        for (uint32_t k = 0; k < At<uint16_t>(boneData, 0x48); k++) {
            uint8_t* weightData = At<uint8_t*>(boneData, 0x44) + k * 8;
            float weight = At<float>(weightData, 4);
            VertexInteractions& vi = interactions[At<uint16_t>(weightData, 0)];
            if (vi.count == vi.allocated) {
                Interactions_Realloc(&vi, vi.allocated != 0 ? vi.allocated * 2 : 1);
            }
            vi.data[vi.count].bone = b;
            vi.data[vi.count].weight = weight;
            vi.count++;
        }
    }
    for (uint32_t v = 0; v < vertices; v++) {
        Interactions_Normalize(&interactions[v]);
    }
    return interactions;
}

// CreateBoneSets(data, interactions, sets): each triangle's bones as a sorted set, kept unless an
// existing set holds them all (existing sets this one holds all of dropped)
void __fastcall CreateBoneSets(NiSkinPartition*, re::Edx, void* data, const VertexInteractions* interactions,
                               BoneSetCollection* sets) {
    uint32_t triangles = TriangleCount(data);
    for (uint32_t t = 0; t < triangles; t++) {
        uint16_t triangle[3];
        GetTriangle(data, t, triangle);
        if (Degenerate(triangle)) {
            continue;
        }
        BoneSet* set = static_cast<BoneSet*>(NiNew(0xC));
        if (set != nullptr) {
            set->data = nullptr;
            set->allocated = 0;
            set->count = 0;
        } else {
            set = nullptr;
        }
        for (int k = 0; k < 3; k++) {
            const VertexInteractions& vi = interactions[triangle[k]];
            uint32_t count = vi.count;
            for (uint32_t i = 0; i < count; i++) {
                uint16_t bone = uint16_t(vi.data[i].bone);
                bool found = false;
                for (uint32_t s = 0; s < set->count; s++) {
                    if (set->data[s] == bone) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    continue;
                }
                if (set->count == set->allocated) {
                    BoneSet_Realloc(set, set->allocated != 0 ? set->allocated * 2 : 1);
                }
                set->data[set->count] = bone;
                set->count++;
            }
        }
        crt::Qsort(set->data, set->count, 2, reinterpret_cast<Compare>(CompareShorts));
        uint32_t i = 0;
        bool covered = false;
        while (i < sets->count) {
            BoneSet* existing = sets->data[i];
            if (BoneSet_Contains(existing, set)) {
                FreeBoneSet(set);
                covered = true;
                break;
            }
            if (BoneSet_Contains(set, existing)) {
                FreeBoneSet(existing);
                sets->count--;
                sets->data[i] = sets->data[sets->count];
            } else {
                i++;
            }
        }
        if (!covered && i == sets->count) {
            if (sets->count == sets->allocated) {
                PointerSet_SetSize(sets, sets->allocated != 0 ? sets->allocated * 2 : 1);
            }
            sets->data[sets->count] = set;
            sets->count++;
        }
    }
}

// AssignTriangles(data, skinData, maxBones, interactions, sets, triangleParts): the sets merged,
// each triangle given the first set holding its bones (a used one first; -1 for degenerate
// ones), the unused sets dropped (the swapped-in one's used flag not swapped: it's checked again
// at the same index); the partition count set
void __fastcall AssignTriangles(NiSkinPartition* self, re::Edx, void* data, void*, int maxBones,
                                const VertexInteractions* interactions, BoneSetCollection* sets,
                                re::Ptr32<int32_t>* triangleParts) {
    uint32_t triangles = TriangleCount(data);
    Skin_CreateBoneSets(self, data, interactions, sets);
    Skin_MergeBoneSets(self, maxBones, sets);
    int32_t* parts = static_cast<int32_t*>(NiMalloc(triangles * 4));
    *triangleParts = parts;
    uint8_t* used = static_cast<uint8_t*>(NiMalloc(sets->count));
    for (uint32_t i = 0; i < sets->count; i++) {
        used[i] = 0;
    }
    for (uint32_t t = 0; t < triangles; t++) {
        uint16_t triangle[3];
        GetTriangle(data, t, triangle);
        if (Degenerate(triangle)) {
            (*triangleParts)[t] = -1;
            continue;
        }
        int32_t part = Skin_FindBoneSet(sets, triangle, interactions, used, 1);
        if (part < 0) {
            part = Skin_FindBoneSet(sets, triangle, interactions, used, 0);
        }
        (*triangleParts)[t] = part;
    }
    for (uint32_t i = 0; i < sets->count;) {
        if (used[i] != 0) {
            i++;
            continue;
        }
        sets->count--;
        sets->data[i] = sets->data[sets->count];
        int32_t moved = int32_t(sets->count);
        for (uint32_t t = 0; t < triangles; t++) {
            if ((*triangleParts)[t] == moved) {
                (*triangleParts)[t] = int32_t(i);
            }
        }
    }
    self->partitionCount = sets->count;
    NiFree(used);
}

// a new[] of partitions (the game's operator new for none), constructed
inline Partition* NewPartitions(uint32_t count) {
    uint32_t* block = static_cast<uint32_t*>(count == 0 ? OperatorNew(4) : NiNewArray(ArrayBytes(count, 0x2C)));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    Partition* partitions = reinterpret_cast<Partition*>(block + 1);
    VectorConstruct(partitions, 0x2C, count, reinterpret_cast<void*>(Partition_ConstructAddress));
    return partitions;
}

// MakePartitions(pkTriData, pkSkinData, ucMaxBonesPerPartition, ucMaxBonesPerVertex, bNeedPalette)
uint8_t __fastcall MakePartitions(NiSkinPartition* self, re::Edx, void* data, void* skinData, int maxBones,
                                  int maxBonesPerVertex, int needPalette) {
    BoneSetCollection sets = {nullptr, 0, 0};
    VertexInteractions* interactions = Skin_CreateVertexInteractions(self, uint16_t(VertexCount(data)), skinData);
    uint8_t ok = Skin_RemoveExtraInteractions(self, data, maxBones, maxBonesPerVertex, interactions);
    if (ok) {
        re::Ptr32<int32_t> triangleParts = nullptr;
        Skin_AssignTriangles(self, data, skinData, maxBones, interactions, &sets, &triangleParts);
        Partition* partitions = NewPartitions(self->partitionCount);
        int weighted = uint8_t(maxBones) != uint8_t(maxBonesPerVertex) ? 1 : 0;
        BoneSet triangles = {nullptr, 0, 0};
        Partition* partition = partitions;
        for (uint32_t i = 0; i < self->partitionCount; i++) {
            TriangleSet_AddPartitionTriangles(&triangles, int32_t(i), triangleParts, TriangleCount(data));
            BoneSet* bones = sets.data[i];
            Partition_Create(partition, &triangles, bones, data, interactions, weighted, needPalette,
                             maxBonesPerVertex, At<uint32_t>(skinData, 0x44));
            triangles.count = 0;
            FreeBoneSet(bones);
            partition++;
        }
        NiFree(triangleParts);
        self->partitions = partitions;
        NiFree(triangles.data);
    }
    if (interactions != nullptr) {
        uint32_t* block = reinterpret_cast<uint32_t*>(interactions) - 1;
        for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
            NiFree(interactions[i].data);
        }
        NiDeleteArray(block, 0xC);
    }
    NiFree(sets.data);
    return ok;
}

// ---- NiSkinPartition

NiSkinPartition* __fastcall Construct(NiSkinPartition* self, re::Edx) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiSkinPartition_vtable);
    self->partitions = nullptr;
    return self;
}

// vf8 IsEqual: the partitions one by one
uint8_t __fastcall IsEqual(NiSkinPartition* self, re::Edx, NiSkinPartition* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    uint32_t count = self->partitionCount;
    if (count != other->partitionCount) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (!Partition_IsEqual(&self->partitions[i], &other->partitions[i])) {
            return 0;
        }
    }
    return 1;
}

const RTTI* __fastcall GetRTTI(NiSkinPartition*, re::Edx) {
    return &NiSkinPartition_RTTI;
}

// the deleting destructor: the renderer's data dropped, the partitions delete[]d (the game's
// operator delete for an empty array)
NiSkinPartition* __fastcall DeletingDestructor(NiSkinPartition* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiSkinPartition_vtable);
    Renderer_RemoveSkinPartition(self);
    Partition* partitions = self->partitions;
    if (partitions != nullptr) {
        uint32_t* block = reinterpret_cast<uint32_t*>(partitions) - 1;
        if (*block == 0) {
            OperatorDelete(block);
        } else {
            Virtual<void>(partitions, 0, 3);
        }
    }
    Object_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// NiTPointerMap<unsigned int, float>'s deleting destructor
WeightMap* __fastcall WeightMapDeletingDestructor(WeightMap* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(WeightMap_vtable);
    TMap_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(TMapBase_vtable);
    TMap_RemoveAll(self);
    NiFree(self->table);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// vf4 LoadBinary: the partitions new[]'d and read
void __fastcall LoadBinary(NiSkinPartition* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    LoadBytes(stream, &self->partitionCount, 4);
    self->partitions = NewPartitions(self->partitionCount);
    for (uint32_t i = 0; i < self->partitionCount; i++) {
        Partition_LoadBinary(self->partitions + i, stream);
    }
}

GAME_FN(void, __thiscall, Object_DoNothing, 0x008A20A0, void* self, void* stream);

// vf7 SaveBinary
void __fastcall SaveBinary(NiSkinPartition* self, re::Edx, void* stream) {
    Object_DoNothing(self, stream);
    SaveBytes(stream, &self->partitionCount, 4);
    for (uint32_t i = 0; i < self->partitionCount; i++) {
        Partition_SaveBinary(self->partitions + i, stream);
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISKINPART_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISKINPART_REVERSED("NiSkinPartition::Partition::~Partition", 0x0076DE30, &ni::PartitionDestruct);
NISKINPART_REVERSED("NiSkinPartition::Partition::GetStripLengthSum", 0x0076DE80, &ni::PartitionGetStripLengthSum);
NISKINPART_REVERSED("NiSkinPartition::Partition::operator==", 0x0076DEA0, &ni::PartitionIsEqual);
NISKINPART_REVERSED("NiSkinPartition::NiSkinPartition", 0x0076E360, &ni::Construct);
NISKINPART_REVERSED("NiSkinPartition::IsEqual", 0x0076E3B0, &ni::IsEqual);
NISKINPART_REVERSED("NiSkinPartition::GetRTTI", 0x0076E420, &ni::GetRTTI);
NISKINPART_REVERSED("NiSkinPartition::VertexInteractions::Normalize", 0x0076E430, &ni::InteractionsNormalize);
NISKINPART_REVERSED("NiSkinPartition::BoneSet::Contains", 0x0076E4F0, &ni::BoneSetContains);
NISKINPART_REVERSED("NiSkinPartition::BoneSet::ContainsBonesOf", 0x0076E560, &ni::BoneSetContainsBonesOf);
NISKINPART_REVERSED("NiSkinPartition::BoneSetMerger::Next", 0x0076E5C0, &ni::MergerNext);
NISKINPART_REVERSED("NiSkinPartition::Partition::MakeVertexMap", 0x0076E650, &ni::PartitionMakeVertexMap);
NISKINPART_REVERSED("NiSkinPartition::Partition::MakeTriangleList", 0x0076E770, &ni::PartitionMakeTriangleList);
NISKINPART_REVERSED("NiSkinPartition::DeletingDestructor", 0x0076E830, &ni::DeletingDestructor);
NISKINPART_REVERSED("NiSkinPartition::Partition::VectorDeletingDestructor", 0x0076E890,
                    &ni::PartitionVectorDeletingDestructor);
NISKINPART_REVERSED("NiTMapBase<unsigned int,float>::SetAt", 0x0076E930, &ni::WeightMapSetAt);
NISKINPART_REVERSED("NiTPointerMap<unsigned int,float>::DeleteItem", 0x0076E9A0, &ni::WeightMapDeleteItem);
NISKINPART_REVERSED("NiSkinPartition::VertexInteractions::Realloc", 0x0076E9C0, &ni::InteractionsRealloc);
NISKINPART_REVERSED("NiSkinPartition::BoneSet::Realloc", 0x0076EA30, &ni::BoneSetRealloc);
NISKINPART_REVERSED("NiSkinPartition::BoneSet::UnionSize", 0x0076EA90, &ni::BoneSetUnionSize);
NISKINPART_REVERSED("NiSkinPartition::BoneSet::Merge", 0x0076EB10, &ni::BoneSetMerge);
NISKINPART_REVERSED("NiSkinPartition::BoneSet::ContainsTriangle", 0x0076EBB0, &ni::BoneSetContainsTriangle);
NISKINPART_REVERSED("NiSkinPartition::Partition::Create", 0x0076EBF0, &ni::PartitionCreate);
NISKINPART_REVERSED("NiSkinPartition::Partition::LoadBinary", 0x0076EFA0, &ni::PartitionLoadBinary);
NISKINPART_REVERSED("NiSkinPartition::Partition::SaveBinary", 0x0076F2B0, &ni::PartitionSaveBinary);
NISKINPART_REVERSED("NiSkinPartition::AddTriangleWeights", 0x0076F590, &ni::AddTriangleWeights);
NISKINPART_REVERSED("NiSkinPartition::FindBoneSet", 0x0076F640, &ni::FindBoneSet);
NISKINPART_REVERSED("NiSkinPartition::LoadBinary", 0x0076F6A0, &ni::LoadBinary);
NISKINPART_REVERSED("NiSkinPartition::SaveBinary", 0x0076F760, &ni::SaveBinary);
NISKINPART_REVERSED("NiTPointerMap<unsigned int,float>::DeletingDestructor", 0x0076F7D0,
                    &ni::WeightMapDeletingDestructor);
NISKINPART_REVERSED("NiSkinPartition::TriangleSet::AddPartitionTriangles", 0x0076F810,
                    &ni::TriangleSetAddPartitionTriangles);
NISKINPART_REVERSED("NiSkinPartition::RemoveExtraInteractions", 0x0076F870, &ni::RemoveExtraInteractions);
NISKINPART_REVERSED("NiSkinPartition::MergeBoneSets", 0x0076FEB0, &ni::MergeBoneSets);
NISKINPART_REVERSED("NiSkinPartition::CreateVertexInteractions", 0x00770000, &ni::CreateVertexInteractions);
NISKINPART_REVERSED("NiSkinPartition::CreateBoneSets", 0x00770120, &ni::CreateBoneSets);
NISKINPART_REVERSED("NiSkinPartition::AssignTriangles", 0x00770350, &ni::AssignTriangles);
NISKINPART_REVERSED("NiSkinPartition::MakePartitions", 0x00770500, &ni::MakePartitions);
