// NiTriBasedGeomData (vtable 0x952C70, 0x44 bytes): NiGeometryData (0x40 bytes, another
// chunk's) with the triangle count and the active one; and NiGeometryData's slot 21 (the vertex
// count and the active one at once), whose code sits here.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

// NiGeometryData's bytes kept opaque (its vertex count +0xC, active vertex count +0x30)
struct NiTriBasedGeomData : NiObject {
    uint8_t geometryData[0x40 - 0x8];   // +0x8
    uint16_t triangles;                 // +0x40
    uint16_t activeTriangles;           // +0x42
};
static_assert(offsetof(NiTriBasedGeomData, triangles) == 0x40);
static_assert(sizeof(NiTriBasedGeomData) == 0x44);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTriBasedGeomData_RTTI, 0x020E2BC8);

constexpr uintptr_t NiTriBasedGeomData_vtable = 0x00952C70;

GAME_FN(void, __thiscall, GeometryData_Construct, 0x0077A5A0, void* self);
GAME_FN(void, __thiscall, GeometryData_Destruct, 0x0077B230, void* self);
GAME_FN(uint8_t, __thiscall, GeometryData_IsEqual, 0x0077AFD0, void* self, void* other);
GAME_FN(void, __thiscall, GeometryData_LoadBinary, 0x0077A880, void* self, void* stream);
GAME_FN(void, __thiscall, GeometryData_SaveBinary, 0x0077ACF0, void* self, void* stream);
GAME_FN(void, __thiscall, GeometryData_GetViewerStrings, 0x0077B4E0, void* self, NiTArray<char*>* strings);
GAME_FN(char*, __cdecl, GetViewerStringShort, 0x00785650, const char* prefix, uint32_t value);

NiTriBasedGeomData* __fastcall Construct(NiTriBasedGeomData* self, re::Edx) {
    GeometryData_Construct(self);
    self->triangles = 0;
    self->activeTriangles = 0;
    self->vtable = reinterpret_cast<void*>(NiTriBasedGeomData_vtable);
    return self;
}

void __fastcall Destruct(NiTriBasedGeomData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiTriBasedGeomData_vtable);
    GeometryData_Destruct(self);
}

// vf8: the triangle counts
uint8_t __fastcall IsEqual(NiTriBasedGeomData* self, re::Edx, NiTriBasedGeomData* other) {
    if (!GeometryData_IsEqual(self, other)) {
        return 0;
    }
    return self->triangles == other->triangles && self->activeTriangles == other->activeTriangles ? 1 : 0;
}

// NiGeometryData's slot 21 (SetVertexCount: the count and the active count)
void __fastcall GeometryDataSetVertexCount(NiObject* self, re::Edx, int vertices) {
    At<uint16_t>(self, 0xC) = uint16_t(vertices);
    At<uint16_t>(self, 0x30) = uint16_t(vertices);
}

// slot 25 SetTriangleCount (the count and the active count)
void __fastcall SetTriangleCount(NiTriBasedGeomData* self, re::Edx, int triangles) {
    self->triangles = uint16_t(triangles);
    self->activeTriangles = uint16_t(triangles);
}

NiTriBasedGeomData* __fastcall DeletingDestructor(NiTriBasedGeomData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiTriBasedGeomData_vtable);
    GeometryData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x44);
    }
    return self;
}

// vf4: the count (all of it active)
void __fastcall LoadBinary(NiTriBasedGeomData* self, re::Edx, void* stream) {
    GeometryData_LoadBinary(self, stream);
    LoadBytes(stream, &self->triangles, 2);
    self->activeTriangles = self->triangles;
}

void __fastcall SaveBinary(NiTriBasedGeomData* self, re::Edx, void* stream) {
    GeometryData_SaveBinary(self, stream);
    SaveBytes(stream, &self->triangles, 2);
}

void __fastcall GetViewerStrings(NiTriBasedGeomData* self, re::Edx, NiTArray<char*>* strings) {
    GeometryData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTriBasedGeomData_RTTI.name));
    AddViewerString(strings, GetViewerStringShort("m_usTriangles", self->triangles));
    AddViewerString(strings, GetViewerStringShort("m_usActiveTriangles", self->activeTriangles));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NITRIBASED_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NITRIBASED_REVERSED("NiTriBasedGeomData::NiTriBasedGeomData", 0x007706F0, &ni::Construct);
NITRIBASED_REVERSED("NiTriBasedGeomData::~NiTriBasedGeomData", 0x00770710, &ni::Destruct);
NITRIBASED_REVERSED("NiTriBasedGeomData::IsEqual", 0x00770720, &ni::IsEqual);
NITRIBASED_REVERSED("NiGeometryData::SetActiveVertexCount", 0x00770760, &ni::GeometryDataSetVertexCount);
NITRIBASED_REVERSED("NiTriBasedGeomData::SetTriangleCount", 0x00770780, &ni::SetTriangleCount);
NITRIBASED_REVERSED("NiTriBasedGeomData::DeletingDestructor", 0x00770790, &ni::DeletingDestructor);
NITRIBASED_REVERSED("NiTriBasedGeomData::LoadBinary", 0x007707C0, &ni::LoadBinary);
NITRIBASED_REVERSED("NiTriBasedGeomData::SaveBinary", 0x00770810, &ni::SaveBinary);
NITRIBASED_REVERSED("NiTriBasedGeomData::GetViewerStrings", 0x00770850, &ni::GetViewerStrings);
