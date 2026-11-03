// Small pieces of the scene graph's classes left between the finished files: NiAVObject's
// selected updates, NiMaterialInstance's constructor, NiRenderStep's deleting destructor, two
// geometry data GetRTTIs, NiGeometryData::CreateNormals and NiTriShapeData's shared normal array
// constructor.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
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
GAME_VAR(RTTI, NiTriBasedGeomData_RTTI, 0x020E2BC8);
GAME_VAR(RTTI, NiTriShapeData_RTTI, 0x020E3130);
GAME_VAR(volatile long, NiRefObject_ms_uiObjects, 0x020E2168);

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);

// NiMaterialInstance (0x10 bytes): the material, its shader cache, the material's revision it
// was made at (-1: none), whether it needs an update
struct NiMaterialInstance {
    re::Ptr32<void> material;      // +0x0
    re::Ptr32<void> cache;         // +0x4
    int32_t revision;    // +0x8
    uint32_t update;     // +0xC
};
static_assert(sizeof(NiMaterialInstance) == 0x10);

struct NiRenderStep : NiRefObject {
    uint8_t unknown8[0x10];   // +0x8 (the callbacks and their data)
    re::Ptr32<const char> name;       // +0x18
    uint32_t unknown1C;       // +0x1C
};
static_assert(sizeof(NiRenderStep) == 0x20);

} // namespace

// 0x75DB30: NiAVObject::UpdateSelected(time): slot 26 UpdateSelectedDownwardPass, then the
// parent's slot 38 UpdateUpwardPass
void __fastcall NiAVObject_UpdateSelected(NiAVObject* self, re::Edx, float time) {
    Virtual<void>(self, 26, time);
    if (self->parent != nullptr) {
        Virtual<void>(self->parent, 38);
    }
}

// 0x75EFC0: NiAVObject::DoSelectedUpdate(time): with selective update on, the rigid pass (slot 27)
// or the selected pass (slot 26)
void __fastcall NiAVObject_DoSelectedUpdate(NiAVObject* self, re::Edx, float time) {
    uint8_t flags = uint8_t(self->flags);
    if (((flags >> 1) & 1) == 0) {
        return;
    }
    if (((flags >> 4) & 1) != 0) {
        Virtual<void>(self, 27, time);
        return;
    }
    Virtual<void>(self, 26, time);
}

// 0x7614E0: NiMaterialInstance::NiMaterialInstance
NiMaterialInstance* __fastcall NiMaterialInstance_Construct(NiMaterialInstance* self, re::Edx) {
    self->material = nullptr;
    self->cache = nullptr;
    self->revision = -1;
    self->update = 1;
    return self;
}

// 0x765230: NiRenderStep::DeletingDestructor (its name let go, NiRefObject's destructor)
NiRenderStep* __fastcall NiRenderStep_DeletingDestructor(NiRenderStep* self, re::Edx, uint32_t flags) {
    FixedStringRelease(&self->name);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    _InterlockedDecrement(&NiRefObject_ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiRenderStep));
    }
    return self;
}

// 0x770770: NiTriBasedGeomData::GetRTTI
const RTTI* __fastcall NiTriBasedGeomData_GetRTTI(void*, re::Edx) {
    return &NiTriBasedGeomData_RTTI;
}

// 0x779F90: NiGeometryData::CreateNormals(zero): the normals new[]'d when there are none (three
// times as many with binormals and tangents), zeroed when asked
void __fastcall NiGeometryData_CreateNormals(NiGeometryData* self, re::Edx, int zero) {
    uint32_t sets = (self->dataFlags & 0xF000) != 0 ? 3 : 1;
    if (self->normals == nullptr) {
        self->normals = static_cast<NiPoint3*>(NiNewArray(uint32_t(self->vertexCount) * sets * 0xC));
    }
    if (re::Flag(zero)) {
        crt::Memset(self->normals, 0, uint32_t(self->vertexCount) * sets * 0xC);
    }
}

// 0x77BC40: NiTriShapeData::GetRTTI
const RTTI* __fastcall NiTriShapeData_GetRTTI(void*, re::Edx) {
    return &NiTriShapeData_RTTI;
}

// 0x77BC50: NiTriShapeData's shared normal array's constructor (empty)
NiSharedNormalArray* __fastcall NiSharedNormalArray_Construct(NiSharedNormalArray* self, re::Edx) {
    self->count = 0;
    self->indices = nullptr;
    return self;
}

#define NISCENEMISC_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NISCENEMISC_REVERSED("NiAVObject::UpdateSelected", 0x0075DB30, NiAVObject_UpdateSelected);
NISCENEMISC_REVERSED("NiAVObject::DoSelectedUpdate", 0x0075EFC0, NiAVObject_DoSelectedUpdate);
NISCENEMISC_REVERSED("NiMaterialInstance::NiMaterialInstance", 0x007614E0, NiMaterialInstance_Construct);
NISCENEMISC_REVERSED("NiRenderStep::DeletingDestructor", 0x00765230, NiRenderStep_DeletingDestructor);
NISCENEMISC_REVERSED("NiTriBasedGeomData::GetRTTI", 0x00770770, NiTriBasedGeomData_GetRTTI);
NISCENEMISC_REVERSED("NiGeometryData::CreateNormals", 0x00779F90, NiGeometryData_CreateNormals);
NISCENEMISC_REVERSED("NiTriShapeData::GetRTTI", 0x0077BC40, NiTriShapeData_GetRTTI);
NISCENEMISC_REVERSED("NiSharedNormalArray::NiSharedNormalArray", 0x0077BC50, NiSharedNormalArray_Construct);

} // namespace ni
