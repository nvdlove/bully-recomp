// The triangle data classes' leftovers (src/gamebryo/NiGeometryData.h, NiTriStripsData.cpp,
// NiTriStripsData2.cpp):
//   NiTriShapeData::CalculateNormals (slot 20, NiTriShapeDynamicData's and NiScreenGeometryData's
//     too) and NiPoint3::UnitizeVector (the table-driven square root)
//   NiTriStripsData (vtable 0x960D00, 0x50 bytes: NiTriBasedGeomData, the strip count +0x44, the
//     lengths +0x48 and the indices +0x4C, NiMalloc'd or from the object group): factory,
//     destructor, comparison, streaming
//   NiTriStrips (vtable 0x960D98, 0xEC bytes): factory, GetRTTI
//   NiTriShapeDynamicData (vtable 0x960E38, 0x58 bytes: NiTriShapeData whose active vertex and
//     triangle counts (+0x30, +0x42) are streamed and set by the caller)
// Main-thread maths: float, in the game's order.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometryData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTriStripsData_RTTI, 0x020E3B30);
GAME_VAR(RTTI, NiTriStrips_RTTI, 0x020E3B64);
GAME_VAR(RTTI, NiTriShapeDynamicData_RTTI, 0x020E3B98);

constexpr uintptr_t NiTriStripsData_vtable = 0x00960D00;
constexpr uintptr_t NiTriStrips_vtable = 0x00960D98;
constexpr uintptr_t NiTriShapeDynamicData_vtable = 0x00960E38;

// NiFastSqrt's table: the mantissa's top bits (and the exponent's parity) to the root's mantissa
GAME_VAR(uint32_t*, FastSqrtTable, 0x020E2BFC);

struct NiTriStripsData : NiGeometryData {
    uint16_t triangleCount;     // +0x40 (NiTriBasedGeomData's)
    uint16_t activeTriangles;   // +0x42
    uint16_t stripCount;        // +0x44
    uint16_t pad46;
    re::Ptr32<uint16_t> stripLengths;     // +0x48
    re::Ptr32<uint16_t> stripLists;       // +0x4C
};
static_assert(offsetof(NiTriStripsData, stripCount) == 0x44);
static_assert(sizeof(NiTriStripsData) == 0x50);

// NiObjectGroup: size, buffer, the next free byte, the objects using it
struct ObjectGroup {
    uint32_t size;
    re::Ptr32<uint8_t> buffer;
    re::Ptr32<uint8_t> free;       // +0x8
    uint32_t refCount;   // +0xC
};
static_assert(sizeof(ObjectGroup) == 0x10);

GAME_FN(void, __thiscall, GeometryData_CreateNormals, 0x00779F90, NiGeometryData* self, int initialize);
GAME_FN(void, __cdecl, Point3_UnitizeVector, 0x007D56A0, NiPoint3* point);
GAME_FN(void, __cdecl, Point3_UnitizeVectors, 0x00770B10, NiPoint3* points, uint32_t count, uint32_t stride);
GAME_FN(void, __thiscall, ObjectGroup_Destruct, 0x007D1CE0, ObjectGroup* self);
GAME_FN(void, __thiscall, TriBasedGeomData_Construct, 0x007706F0, void* self);
GAME_FN(void, __thiscall, TriBasedGeomData_Destruct, 0x00770710, void* self);
GAME_FN(uint8_t, __thiscall, TriBasedGeomData_IsEqual, 0x00770720, void* self, void* other);
GAME_FN(void, __thiscall, TriBasedGeomData_LoadBinary, 0x007707C0, void* self, void* stream);
GAME_FN(void, __thiscall, TriBasedGeomData_SaveBinary, 0x00770810, void* self, void* stream);
GAME_FN(void, __thiscall, TriBasedGeom_Construct, 0x007793A0, void* self);
GAME_FN(void, __thiscall, TriShapeData_Construct, 0x0077BA40, void* self);
GAME_FN(void, __thiscall, TriShapeData_Destruct, 0x0077BC80, void* self);
GAME_FN(uint8_t, __thiscall, TriShapeData_IsEqual, 0x0077BB60, void* self, void* other);
GAME_FN(void, __thiscall, TriShapeData_LoadBinary, 0x0077BDB0, void* self, void* stream);
GAME_FN(void, __thiscall, TriShapeData_SaveBinary, 0x0077C030, void* self, void* stream);
GAME_FN(void, __thiscall, TriShapeData_GetViewerStrings, 0x0077C1B0, void* self, NiTArray<char*>* strings);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, TriStripsData_Destruct, 0x007D8560, NiTriStripsData* self);

// an array streamed in one read or write of `bytes`, one component of `componentSize`
inline void LoadArray(void* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}
inline void SaveArray(void* stream, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// ---- NiPoint3::UnitizeVector, NiTriShapeData::CalculateNormals

// UnitizeVector(point): scaled by 1 / NiFastSqrt((x*x + y*y) + z*z) (zero for a +0 length)
void __cdecl UnitizeVector(NiPoint3* v) {
    float squared = v->x * v->x + v->y * v->y + v->z * v->z;
    uint32_t bits;
    std::memcpy(&bits, &squared, 4);
    float inv;
    if (bits == 0) {
        inv = 0.0f;
    } else {
        uint16_t exponent = uint16_t((bits >> 23) - 0x7F);
        uint32_t mantissa = bits & 0x7FFFFF;
        if ((exponent & 1) != 0) {
            mantissa |= 0x800000;
        }
        int32_t rootExponent = int32_t(int16_t(exponent) >> 1) + 0x7F;
        uint32_t rootBits = (uint32_t(rootExponent) << 23) | FastSqrtTable[mantissa >> 16];
        float root;
        std::memcpy(&root, &rootBits, 4);
        inv = 1.0f / root;
    }
    v->x = v->x * inv;
    v->y = inv * v->y;
    v->z = inv * v->z;
}

inline void AddTo(NiPoint3* normal, const NiPoint3& n) {
    normal->x = normal->x + n.x;
    normal->y = normal->y + n.y;
    normal->z = n.z + normal->z;
}

// the vertices sharing a vertex's normal (when the shared normals cover all the vertices) get the
// face normal too, the last first
inline void AddToShared(NiTriShapeData* self, uint32_t vertex, const NiPoint3& n) {
    const NiSharedNormalArray& shared = self->sharedNormals[vertex];
    for (uint16_t k = shared.count; k != 0; k--) {
        AddTo(&self->normals[shared.indices[k - 1]], n);
    }
}

// CalculateNormals: the normals made (zeroed); each active triangle's unit face normal added to
// its three vertices' normals (and those sharing them); the active vertices' normals unitized
void __fastcall TriShapeDataCalculateNormals(NiTriShapeData* self, re::Edx) {
    GeometryData_CreateNormals(self, 1);
    const uint16_t* triangle = self->triList;
    uint16_t triangles = At<uint16_t>(self, 0x42);   // the active triangles
    for (uint32_t t = triangles; t != 0; t--) {
        uint32_t i0 = triangle[0];
        uint32_t i1 = triangle[1];
        uint32_t i2 = triangle[2];
        triangle += 3;
        const NiPoint3* p = self->vertices;
        NiPoint3 e1 = {p[i1].x - p[i0].x, p[i1].y - p[i0].y, p[i1].z - p[i0].z};
        NiPoint3 e2 = {p[i2].x - p[i1].x, p[i2].y - p[i1].y, p[i2].z - p[i1].z};
        NiPoint3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e2.z * e1.x, e1.x * e2.y - e2.x * e1.y};
        Point3_UnitizeVector(&n);
        AddTo(&self->normals[i0], n);
        AddTo(&self->normals[i1], n);
        AddTo(&self->normals[i2], n);
        if (self->sharedNormals != nullptr && self->sharedNormalsArraySize == self->vertexCount) {
            AddToShared(self, i0, n);
            AddToShared(self, i1, n);
            AddToShared(self, i2, n);
        }
    }
    Point3_UnitizeVectors(self->normals, self->activeVertexCount, 0xC);
    self->dirtyFlags |= 2;
}

// ---- NiTriStripsData

NiTriStripsData* __cdecl TriStripsDataCreateObject() {
    NiTriStripsData* object = static_cast<NiTriStripsData*>(NiNew(0x50));
    if (object == nullptr) {
        return nullptr;
    }
    TriBasedGeomData_Construct(object);
    object->vtable = reinterpret_cast<void*>(NiTriStripsData_vtable);
    object->stripCount = 0;
    object->stripLengths = nullptr;
    object->stripLists = nullptr;
    return object;
}

const RTTI* __fastcall TriStripsDataGetRTTI(void*, re::Edx) {
    return &NiTriStripsData_RTTI;
}

// NiTriBasedGeomData's, the strip count and lengths, then the indices (both arrays or neither;
// the count: two per strip plus the triangles)
uint8_t __fastcall TriStripsDataIsEqual(NiTriStripsData* self, re::Edx, NiTriStripsData* other) {
    if (!TriBasedGeomData_IsEqual(self, other)) {
        return 0;
    }
    uint16_t strips = self->stripCount;
    if (strips != other->stripCount) {
        return 0;
    }
    for (uint32_t i = 0; i < strips; i++) {
        if (self->stripLengths[i] != other->stripLengths[i]) {
            return 0;
        }
    }
    uint32_t indices = uint16_t(strips * 2 + self->triangleCount);
    if (self->stripLists == nullptr) {
        return other->stripLists == nullptr ? 1 : 0;
    }
    if (other->stripLists == nullptr) {
        return 0;
    }
    for (uint32_t i = 0; i < indices; i++) {
        if (self->stripLists[i] != other->stripLists[i]) {
            return 0;
        }
    }
    return 1;
}

// the arrays freed, or (from an object group, slot 16) the group let go of once per array
void __fastcall TriStripsDataDestruct(NiTriStripsData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiTriStripsData_vtable);
    if (Virtual<ObjectGroup*>(self, 16) == nullptr) {
        NiFree(self->stripLengths);
        NiFree(self->stripLists);
    } else {
        for (int i = 0; i < 2; i++) {
            ObjectGroup* group = Virtual<ObjectGroup*>(self, 16);
            if (--group->refCount == 0) {
                ObjectGroup_Destruct(group);
                NiDelete(group, 0x10);
            }
        }
    }
    TriBasedGeomData_Destruct(self);
}

// an array of `bytes` from the object group (its next free bytes, the group held once more) or
// NiMalloc'd
inline uint16_t* AllocateArray(NiTriStripsData* self, uint32_t bytes) {
    if (Virtual<ObjectGroup*>(self, 16) == nullptr) {
        return static_cast<uint16_t*>(NiMalloc(bytes));
    }
    ObjectGroup* group = Virtual<ObjectGroup*>(self, 16);
    uint8_t* block = group->free;
    group->refCount++;
    group->free = block + bytes;
    return reinterpret_cast<uint16_t*>(block);
}

// the strip count, the lengths, the indices after their flag
void __fastcall TriStripsDataLoadBinary(NiTriStripsData* self, re::Edx, void* stream) {
    TriBasedGeomData_LoadBinary(self, stream);
    LoadBytes(stream, &self->stripCount, 2);
    if (self->stripCount == 0) {
        return;
    }
    self->stripLengths = AllocateArray(self, uint32_t(self->stripCount) * 2);
    LoadArray(stream, self->stripLengths, uint32_t(self->stripCount) * 2, 2);
    uint8_t hasLists = 0;
    LoadBytes(stream, &hasLists, 1);
    if (hasLists != 0) {
        uint32_t indices = uint16_t(self->stripCount * 2 + self->triangleCount);
        self->stripLists = AllocateArray(self, indices * 2);
        LoadArray(stream, self->stripLists, indices * 2, 2);
    }
}

void __fastcall TriStripsDataSaveBinary(NiTriStripsData* self, re::Edx, void* stream) {
    TriBasedGeomData_SaveBinary(self, stream);
    SaveBytes(stream, &self->stripCount, 2);
    if (self->stripCount == 0) {
        return;
    }
    SaveArray(stream, self->stripLengths, uint32_t(self->stripCount) * 2, 2);
    uint8_t hasLists = self->stripLists != nullptr ? 1 : 0;
    SaveBytes(stream, &hasLists, 1);
    if (hasLists != 0) {
        SaveArray(stream, self->stripLists, uint32_t(uint16_t(self->stripCount * 2 + self->triangleCount)) * 2, 2);
    }
}

NiTriStripsData* __fastcall TriStripsDataDeletingDestructor(NiTriStripsData* self, re::Edx, uint32_t flags) {
    TriStripsData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x50);
    }
    return self;
}

// ---- NiTriStrips

void* __cdecl TriStripsCreateObject() {
    void* object = NiNew(0xEC);
    if (object == nullptr) {
        return nullptr;
    }
    TriBasedGeom_Construct(object);
    *static_cast<re::Ptr32<void>*>(object) = reinterpret_cast<void*>(NiTriStrips_vtable);
    return object;
}

const RTTI* __fastcall TriStripsGetRTTI(void*, re::Edx) {
    return &NiTriStrips_RTTI;
}

// ---- NiTriShapeDynamicData

// slot 24 GetStripData(strips&, lengths&, indices&, total&): the active triangles as one list
void __fastcall DynamicGetStripData(NiTriShapeData* self, re::Edx, uint16_t* strips, re::Ptr32<uint16_t>* lengths,
                                    re::Ptr32<uint16_t>* indices, uint32_t* total) {
    *strips = At<uint16_t>(self, 0x42);
    *lengths = nullptr;
    *indices = self->triList;
    *total = uint32_t(At<uint16_t>(self, 0x42)) * 3;
}

NiTriShapeData* __cdecl DynamicCreateObject() {
    NiTriShapeData* object = static_cast<NiTriShapeData*>(NiNew(0x58));
    if (object == nullptr) {
        return nullptr;
    }
    TriShapeData_Construct(object);
    object->vtable = reinterpret_cast<void*>(NiTriShapeDynamicData_vtable);
    object->activeVertexCount = 0;
    At<uint16_t>(object, 0x42) = 0;
    return object;
}

// NiTriShapeData's and the same active counts
uint8_t __fastcall DynamicIsEqual(NiTriShapeData* self, re::Edx, NiTriShapeData* other) {
    if (!TriShapeData_IsEqual(self, other) || self->activeVertexCount != other->activeVertexCount ||
        At<uint16_t>(self, 0x42) != At<uint16_t>(other, 0x42)) {
        return 0;
    }
    return 1;
}

const RTTI* __fastcall DynamicGetRTTI(void*, re::Edx) {
    return &NiTriShapeDynamicData_RTTI;
}

// slot 18 SetActiveVertexCount
void __fastcall DynamicSetActiveVertexCount(NiTriShapeData* self, re::Edx, int count) {
    self->activeVertexCount = uint16_t(count);
}

// slot 22 SetActiveTriangleCount
void __fastcall DynamicSetActiveTriangleCount(NiTriShapeData* self, re::Edx, int count) {
    At<uint16_t>(self, 0x42) = uint16_t(count);
}

// slot 25 SetTriangleCount (the count only)
void __fastcall DynamicSetTriangleCount(NiTriShapeData* self, re::Edx, int count) {
    self->triangleCount = uint16_t(count);
}

// slot 21 SetVertexCount (the count only)
void __fastcall DynamicSetVertexCount(NiTriShapeData* self, re::Edx, int count) {
    self->vertexCount = uint16_t(count);
}

NiTriShapeData* __fastcall DynamicDeletingDestructor(NiTriShapeData* self, re::Edx, uint32_t flags) {
    TriShapeData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x58);
    }
    return self;
}

void __fastcall DynamicLoadBinary(NiTriShapeData* self, re::Edx, void* stream) {
    TriShapeData_LoadBinary(self, stream);
    LoadBytes(stream, &self->activeVertexCount, 2);
    LoadBytes(stream, &At<uint16_t>(self, 0x42), 2);
}

void __fastcall DynamicSaveBinary(NiTriShapeData* self, re::Edx, void* stream) {
    TriShapeData_SaveBinary(self, stream);
    SaveBytes(stream, &self->activeVertexCount, 2);
    SaveBytes(stream, &At<uint16_t>(self, 0x42), 2);
}

void __fastcall DynamicGetViewerStrings(NiTriShapeData* self, re::Edx, NiTArray<char*>* strings) {
    TriShapeData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTriShapeDynamicData_RTTI.name));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NITRISHAPEDYN_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NITRISHAPEDYN_REVERSED("NiPoint3::UnitizeVector", 0x007D56A0, ni::UnitizeVector);
NITRISHAPEDYN_REVERSED("NiTriShapeData::CalculateNormals", 0x007D5730, ni::TriShapeDataCalculateNormals);
NITRISHAPEDYN_REVERSED("NiTriStripsData::CreateObject", 0x007D8360, ni::TriStripsDataCreateObject);
NITRISHAPEDYN_REVERSED("NiTriStripsData::GetRTTI", 0x007D83B0, ni::TriStripsDataGetRTTI);
NITRISHAPEDYN_REVERSED("NiTriStripsData::IsEqual", 0x007D8490, ni::TriStripsDataIsEqual);
NITRISHAPEDYN_REVERSED("NiTriStripsData::~NiTriStripsData", 0x007D8560, ni::TriStripsDataDestruct);
NITRISHAPEDYN_REVERSED("NiTriStripsData::LoadBinary", 0x007D8690, ni::TriStripsDataLoadBinary);
NITRISHAPEDYN_REVERSED("NiTriStripsData::SaveBinary", 0x007D87E0, ni::TriStripsDataSaveBinary);
NITRISHAPEDYN_REVERSED("NiTriStripsData::DeletingDestructor", 0x007D88D0, ni::TriStripsDataDeletingDestructor);
NITRISHAPEDYN_REVERSED("NiTriStrips::CreateObject", 0x007D8A00, ni::TriStripsCreateObject);
NITRISHAPEDYN_REVERSED("NiTriStrips::GetRTTI", 0x007D8A60, ni::TriStripsGetRTTI);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::GetStripData", 0x007D8BA0, ni::DynamicGetStripData);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::CreateObject", 0x007D8BD0, ni::DynamicCreateObject);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::IsEqual", 0x007D8C30, ni::DynamicIsEqual);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::GetRTTI", 0x007D8C70, ni::DynamicGetRTTI);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::SetActiveVertexCount", 0x007D8C80, ni::DynamicSetActiveVertexCount);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::SetActiveTriangleCount", 0x007D8C90,
                       ni::DynamicSetActiveTriangleCount);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::SetTriangleCount", 0x007D8CA0, ni::DynamicSetTriangleCount);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::SetVertexCount", 0x007D8CB0, ni::DynamicSetVertexCount);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::DeletingDestructor", 0x007D8CC0, ni::DynamicDeletingDestructor);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::LoadBinary", 0x007D8CE0, ni::DynamicLoadBinary);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::SaveBinary", 0x007D8D40, ni::DynamicSaveBinary);
NITRISHAPEDYN_REVERSED("NiTriShapeDynamicData::GetViewerStrings", 0x007D8DA0, ni::DynamicGetViewerStrings);
