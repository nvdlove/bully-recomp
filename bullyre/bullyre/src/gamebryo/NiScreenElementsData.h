#pragma once

// NiScreenElementsData (vtable 0x955260, 0x70 bytes, from its constructor 0x7893A0): the
// NiTriShapeData of screen-space polygons (NiScreenElements' data). Each polygon is a range of
// vertices and of triangle indices, found through an indexer (a polygon's handle is its slot in
// the indexer, 0xFFFF a free slot); the arrays grow by their growth steps.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometryData.h"
#include "reversed.h"

namespace ni {

struct NiScreenElementsPolygon {
    uint16_t numVertices;    // +0x0
    uint16_t vertexOffset;   // +0x2
    uint16_t numTriangles;   // +0x4
    uint16_t indexOffset;    // +0x6
};
static_assert(sizeof(NiScreenElementsPolygon) == 8);

struct NiScreenElementsData : NiTriShapeData {
    re::Ptr32<NiScreenElementsPolygon> polygons;   // +0x58 (NiMalloc'd, maxPQuantity of them)
    re::Ptr32<uint16_t> polygonIndexer;            // +0x5C (NiMalloc'd; 0xFFFF a free slot)
    uint16_t maxPQuantity;               // +0x60
    uint16_t pGrowBy;                    // +0x62
    uint16_t pQuantity;                  // +0x64
    uint16_t maxVQuantity;               // +0x66
    uint16_t vGrowBy;                    // +0x68
    uint16_t maxIQuantity;               // +0x6A
    uint16_t iGrowBy;                    // +0x6C
    uint8_t boundNeedsUpdate;            // +0x6E
    uint8_t pad6F;
};
static_assert(offsetof(NiScreenElementsData, polygons) == 0x58);
static_assert(offsetof(NiScreenElementsData, maxPQuantity) == 0x60);
static_assert(offsetof(NiScreenElementsData, boundNeedsUpdate) == 0x6E);
static_assert(sizeof(NiScreenElementsData) == 0x70);

} // namespace ni
