#pragma once

// NiSkinPartition (vtable 0x952C00, 0x10 bytes): a skinned mesh's triangles split into
// partitions that each use few enough bones for the hardware; and its Partition (vtable
// 0x952BFC: one slot, the destructor; 0x2C bytes; new[]'d with its count before the array).
// From the constructors at 0x76E360 and 0x76DE00, and MakePartitions (0x770500).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiSkinPartitionPart {
    re::Ptr32<void> vtable;              // +0x0 (0x952BFC)
    re::Ptr32<uint16_t> bones;           // +0x4 (the skin's bone indices this partition uses)
    re::Ptr32<float> weights;            // +0x8 (bonesPerVertex per vertex)
    re::Ptr32<uint16_t> vertexMap;       // +0xC (the partition's vertices' indices in the mesh)
    re::Ptr32<uint8_t> bonePalette;      // +0x10 (bonesPerVertex per vertex: indices into bones)
    re::Ptr32<uint16_t> triList;         // +0x14 (three per triangle, or the strips)
    re::Ptr32<uint16_t> stripLengths;    // +0x18
    uint16_t vertices;         // +0x1C
    uint16_t triangles;        // +0x1E
    uint16_t boneCount;        // +0x20
    uint16_t strips;           // +0x22
    uint16_t bonesPerVertex;   // +0x24
    uint16_t pad26;
    re::Ptr32<void> buffData;            // +0x28 (the renderer's)
};
static_assert(offsetof(NiSkinPartitionPart, vertices) == 0x1C);
static_assert(offsetof(NiSkinPartitionPart, bonesPerVertex) == 0x24);
static_assert(sizeof(NiSkinPartitionPart) == 0x2C);

struct NiSkinPartition : NiObject {
    uint32_t partitionCount;           // +0x8
    re::Ptr32<NiSkinPartitionPart> partitions;   // +0xC
};
static_assert(sizeof(NiSkinPartition) == 0x10);

} // namespace ni
