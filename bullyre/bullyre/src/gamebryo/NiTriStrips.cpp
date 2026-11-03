// NiTriStripsData (on NiTriBasedGeomData: the strip count +0x44, the strip lengths +0x48, the
// strip lists +0x4C) and NiTriStrips (an NiTriBasedGeom, vtable 0x960D98, 0xEC bytes): the slots
// the first passes left: a triangle's indices (strips alternate their winding), the strip data,
// the block allocation size, rendering, cloning, a triangle's vertices, viewer strings.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometryData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiTriStripsData : NiGeometryData {
    uint16_t triangleCount;   // +0x40 (NiTriBasedGeomData's)
    uint16_t pad42;
    uint16_t strips;          // +0x44
    uint16_t pad46;
    re::Ptr32<uint16_t> stripLengths;   // +0x48
    re::Ptr32<uint16_t> stripLists;     // +0x4C
};
static_assert(offsetof(NiTriStripsData, strips) == 0x44);
static_assert(offsetof(NiTriStripsData, stripLists) == 0x4C);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTriStripsData_RTTI, 0x020E3B30);
GAME_VAR(RTTI, NiTriStrips_RTTI, 0x020E3B64);

constexpr uintptr_t NiTriStrips_vtable = 0x00960D98;

GAME_FN(uint32_t, __thiscall, GeometryData_GetBlockAllocationSize, 0x0077A430, void* self);
GAME_FN(void, __thiscall, TriBasedGeomData_GetViewerStrings, 0x00770850, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Geometry_RenderImmediate, 0x00761550, void* self, void* renderer);
GAME_FN(void*, __thiscall, TriBasedGeom_Construct, 0x007793A0, void* self);
GAME_FN(void, __thiscall, TriBasedGeom_CopyMembers, 0x007793E0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, TriBasedGeom_GetViewerStrings, 0x00779450, void* self, NiTArray<char*>* strings);
GAME_FN(char*, __cdecl, GetViewerStringShort, 0x00785650, const char* prefix, uint32_t value);

} // namespace

// 0x7D83C0: NiTriStripsData::GetTriangleIndices(triangle, i0, i1, i2): the strip it falls in
// (each holds its length less two), the odd ones' first two swapped
void __fastcall NiTriStripsData_GetTriangleIndices(NiTriStripsData* self, re::Edx, uint32_t triangle, uint16_t* i0,
                                                   uint16_t* i1, uint16_t* i2) {
    const uint16_t* list = self->stripLists;
    const uint16_t* lengths = self->stripLengths;
    uint16_t length = lengths[0];
    uint16_t count = uint16_t(length - 2);
    uint32_t strip = 0;
    if (uint16_t(triangle) >= count) {
        do {
            triangle -= count;
            list += length;
            strip++;
            length = lengths[uint16_t(strip)];
            count = uint16_t(length - 2);
        } while (uint16_t(triangle) >= count);
    }
    uint32_t i = uint16_t(triangle);
    if ((triangle & 1) != 0) {
        *i0 = list[i + 1];
        *i1 = list[i];
    } else {
        *i0 = list[i];
        *i1 = list[i + 1];
    }
    *i2 = list[i + 2];
}

// 0x7D8440: slot 23, a lone jump to the above
RE_UNHOOKED_FN("NiTriStripsData::GetTriangleIndices(slot)", 0x007D8440, &NiTriStripsData_GetTriangleIndices);

// 0x7D8450: GetStripData(strips, lengths, lists, list length)
void __fastcall NiTriStripsData_GetStripData(NiTriStripsData* self, re::Edx, uint16_t* strips, re::Ptr32<uint16_t>* lengths,
                                             re::Ptr32<uint16_t>* lists, uint32_t* listLength) {
    *strips = self->strips;
    *lengths = self->stripLengths;
    *lists = self->stripLists;
    *listLength = uint16_t(self->strips * 2 + self->triangleCount);
}

// 0x7D8540: GetBlockAllocationSize: NiGeometryData's and the strip arrays
uint32_t __fastcall NiTriStripsData_GetBlockAllocationSize(NiTriStripsData* self, re::Edx) {
    uint32_t strips = self->strips;
    uint32_t extra = (uint32_t(uint16_t(strips * 2 + self->triangleCount)) + strips) * 2;
    return GeometryData_GetBlockAllocationSize(self) + extra;
}

// 0x7D88F0: GetViewerStrings (slot 9)
void __fastcall NiTriStripsData_GetViewerStrings(NiTriStripsData* self, re::Edx, NiTArray<char*>* strings) {
    TriBasedGeomData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTriStripsData_RTTI.name));
    AddViewerString(strings, GetViewerStringShort("m_usStrips", self->strips));
    AddViewerString(strings, GetViewerStringPointer("m_pusStripLengths", self->stripLengths));
    AddViewerString(strings, GetViewerStringPointer("m_pusStripLists", self->stripLists));
}

// 0x7D8A70: NiTriStrips::RenderImmediate(renderer): NiGeometry's, then the renderer's slot 96
// (its strips drawn)
void __fastcall NiTriStrips_RenderImmediate(void* self, re::Edx, void* renderer) {
    Geometry_RenderImmediate(self, renderer);
    Virtual<void>(renderer, 96, self);
}

// 0x7D8A90: NiTriStrips::CreateClone (slot 3); a failed NiNew is cloned into anyway
void* __fastcall NiTriStrips_CreateClone(void* self, re::Edx, void* cloning) {
    void* clone = NiNew(0xEC);
    if (clone != nullptr) {
        TriBasedGeom_Construct(clone);
        *static_cast<re::Ptr32<void>*>(clone) = reinterpret_cast<void*>(NiTriStrips_vtable);
        TriBasedGeom_CopyMembers(self, clone, cloning);
        return clone;
    }
    TriBasedGeom_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

// 0x7D8AE0: NiTriStrips::GetTriangle(triangle, p0, p1, p2): the triangle's vertices (its data's
// slot 23 for the indices)
void __fastcall NiTriStrips_GetTriangle(void* self, re::Edx, uint32_t triangle, re::Ptr32<NiPoint3>* p0, re::Ptr32<NiPoint3>* p1,
                                        re::Ptr32<NiPoint3>* p2) {
    NiGeometryData* data = At<NiGeometryData*>(self, 0xCC);
    NiPoint3* vertices = data->vertices;
    uint16_t i0;
    uint16_t i1;
    uint16_t i2;
    Virtual<void>(data, 23, triangle, &i0, &i1, &i2);
    *p0 = &vertices[i0];
    *p1 = &vertices[i1];
    *p2 = &vertices[i2];
}

// 0x7D8B50: NiTriStrips::GetViewerStrings (slot 9)
void __fastcall NiTriStrips_GetViewerStrings(void* self, re::Edx, NiTArray<char*>* strings) {
    TriBasedGeom_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTriStrips_RTTI.name));
}

#define NITRISTRIPS3_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NITRISTRIPS3_REVERSED("NiTriStripsData::GetTriangleIndices", 0x007D83C0, NiTriStripsData_GetTriangleIndices);
NITRISTRIPS3_REVERSED("NiTriStripsData::GetStripData", 0x007D8450, NiTriStripsData_GetStripData);
NITRISTRIPS3_REVERSED("NiTriStripsData::GetBlockAllocationSize", 0x007D8540, NiTriStripsData_GetBlockAllocationSize);
NITRISTRIPS3_REVERSED("NiTriStripsData::GetViewerStrings", 0x007D88F0, NiTriStripsData_GetViewerStrings);
NITRISTRIPS3_REVERSED("NiTriStrips::RenderImmediate", 0x007D8A70, NiTriStrips_RenderImmediate);
NITRISTRIPS3_REVERSED("NiTriStrips::CreateClone", 0x007D8A90, NiTriStrips_CreateClone);
NITRISTRIPS3_REVERSED("NiTriStrips::GetTriangle", 0x007D8AE0, NiTriStrips_GetTriangle);
NITRISTRIPS3_REVERSED("NiTriStrips::GetViewerStrings", 0x007D8B50, NiTriStrips_GetViewerStrings);

} // namespace ni
