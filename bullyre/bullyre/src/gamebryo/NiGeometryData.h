#pragma once

// NiGeometryData (vtable 0x9539E0, 0x40 bytes) and the kinds whose code is in this part of the
// library: NiLinesData (vtable 0x953970, 0x44) and NiTriShapeData (vtable 0x953BC8, 0x58; on
// NiTriBasedGeomData, another chunk's, which adds the triangle count). From the constructors at
// 0x77A4A0, 0x779BD0 and 0x77BA40.
//
// The arrays come from the object's group when it has one (slot 16 GetGroup: an NiObjectGroup,
// 0x10 bytes: its buffer +0x4, the next free byte +0x8, the objects using it +0xC), else from
// NiNew[] (NiDeleteArray frees them).
//
// Virtual slots past NiObject's: 19 ContainsVertexData, 21 SetActiveVertexCount (the count
// Replace sets), 23 GetTriangleIndices, 24 GetStripData, 25 SetTriangleCount.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiGeometryData : NiObject {
    re::Ptr32<NiObject> group;            // +0x8 (NiObjectGroup*; a link ID's group when streamed)
    uint16_t vertexCount;       // +0xC
    uint16_t id;                // +0xE (ms_usNextID, 0xBC8ACC, under the lock at 0x20E30DC)
    NiBound bound;              // +0x10
    re::Ptr32<NiPoint3> vertices;         // +0x20
    re::Ptr32<NiPoint3> normals;          // +0x24 (followed by binormals and tangents when bits 12..15 of
                                //  the data flags say so: three times the count)
    re::Ptr32<float> colors;              // +0x28 (NiColorA, 0x10 bytes each)
    re::Ptr32<float> textures;            // +0x2C (NiPoint2, a set of vertexCount per texture set)
    uint16_t activeVertexCount; // +0x30
    uint16_t dataFlags;         // +0x32 (bits 0..5 the texture sets, 12..15 the normal/binormal/
                                //  tangent method)
    uint16_t dirtyFlags;        // +0x34 (the consistency in bits 12..15)
    uint8_t keepFlags;          // +0x36
    uint8_t compressFlags;      // +0x37
    re::Ptr32<NiObject> additionalData;   // +0x38 (NiAdditionalGeometryDataPtr)
    re::Ptr32<void> rendererData;         // +0x3C (the renderer's buffer data: slot 1 ContainsVertexData)
};
static_assert(offsetof(NiGeometryData, vertexCount) == 0xC);
static_assert(offsetof(NiGeometryData, bound) == 0x10);
static_assert(offsetof(NiGeometryData, vertices) == 0x20);
static_assert(offsetof(NiGeometryData, activeVertexCount) == 0x30);
static_assert(offsetof(NiGeometryData, dirtyFlags) == 0x34);
static_assert(offsetof(NiGeometryData, additionalData) == 0x38);
static_assert(sizeof(NiGeometryData) == 0x40);

struct NiLinesData : NiGeometryData {
    re::Ptr32<uint8_t> flags;             // +0x40 (per vertex: connected to the next; NiMalloc'd)
};
static_assert(sizeof(NiLinesData) == 0x44);

// NiTriShapeData's shared normals: per vertex, the vertices sharing its normal
struct NiSharedNormalArray {
    uint16_t count;             // +0x0
    uint16_t pad2;
    re::Ptr32<uint16_t> indices;          // +0x4 (from the pools below)
};
static_assert(sizeof(NiSharedNormalArray) == 8);

// the pools the shared normals' indices come from (each twice the size of the one before)
struct NiSharedNormalPool {
    re::Ptr32<uint16_t> data;             // +0x0 (NiMalloc'd)
    re::Ptr32<uint16_t> cursor;           // +0x4
    uint32_t size;              // +0x8 (indices)
    uint32_t free;              // +0xC
    re::Ptr32<NiSharedNormalPool> next;   // +0x10
};
static_assert(sizeof(NiSharedNormalPool) == 0x14);

struct NiTriShapeData : NiGeometryData {
    uint16_t triangleCount;     // +0x40 (NiTriBasedGeomData's)
    uint16_t pad42;
    uint32_t triListLength;     // +0x44 (three per triangle)
    re::Ptr32<uint16_t> triList;          // +0x48 (NiMalloc'd)
    re::Ptr32<NiSharedNormalArray> sharedNormals;   // +0x4C (new[]'d)
    uint16_t sharedNormalsArraySize;      // +0x50
    uint16_t pad52;
    re::Ptr32<NiSharedNormalPool> sharedNormalPools;   // +0x54
};
static_assert(offsetof(NiTriShapeData, triListLength) == 0x44);
static_assert(offsetof(NiTriShapeData, sharedNormalPools) == 0x54);
static_assert(sizeof(NiTriShapeData) == 0x58);

} // namespace ni
