// The NiGeometry kinds whose code is here: NiTriBasedGeom (vtable 0x953788), NiTriShape
// (vtable 0x953B20) and NiLines (vtable 0x9538D0), all 0xEC bytes (NiGeometry's: nothing of their
// own). Construction, cloning, drawing (the renderer's RenderShape / RenderLines), NiLines' effect
// state (only the lights' kinds it can use), viewer strings. Also NiAVObject::UpdateWorldData and
// NiGeometry's deleting destructor, which sit with them.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometry.h"
#include "gamebryo/NiGeometryData.h"
#include "gamebryo/NiSceneCommon.h"
#include "game/HeapCheck.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTriBasedGeom_RTTI, 0x020E2FD4);
GAME_VAR(RTTI, NiLines_RTTI, 0x020E3040);
GAME_VAR(RTTI, NiTriShape_RTTI, 0x020E30FC);

constexpr uintptr_t NiTriBasedGeom_vtable = 0x00953788;
constexpr uintptr_t NiLines_vtable = 0x009538D0;
constexpr uintptr_t NiTriShape_vtable = 0x00953B20;

GAME_FN(NiTransform*, __thiscall, Transform_Multiply, 0x00406830, const NiTransform* self, NiTransform* result,
        const NiTransform* other);
GAME_FN(NiObject*, __thiscall, DynamicEffectState_Construct, 0x0075F000, void* self);
GAME_FN(void, __thiscall, DynamicEffectState_AddEffect, 0x007D2700, NiObject* self, NiObject* effect);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
// NiGeometry's (another chunk's)
GAME_FN(void, __thiscall, Geometry_CopyMembers, 0x00761390, NiGeometry* self, NiGeometry* dest, void* cloning);
GAME_FN(void, __thiscall, Geometry_RenderImmediate, 0x00761550, NiGeometry* self, void* renderer);
GAME_FN(void, __thiscall, Geometry_GetViewerStrings, 0x00761C50, NiGeometry* self, NiTArray<char*>* strings);
GAME_FN(NiGeometry*, __thiscall, Geometry_Construct, 0x00761CA0, NiGeometry* self);
GAME_FN(void, __thiscall, Geometry_Destructor, 0x00761D10, NiGeometry* self);
GAME_FN(NiGeometry*, __thiscall, Geometry_ConstructWithData, 0x007624E0, NiGeometry* self, NiObject* data);
// (this chunk's, called where modes apply)
GAME_FN(NiGeometry*, __thiscall, TriBasedGeom_ConstructWithData, 0x00779380, NiGeometry* self, NiObject* data);
GAME_FN(NiGeometry*, __thiscall, TriBasedGeom_Construct, 0x007793A0, NiGeometry* self);
GAME_FN(void, __thiscall, TriBasedGeom_Destruct, 0x007793C0, NiGeometry* self);
GAME_FN(void, __thiscall, TriBasedGeom_CopyMembers, 0x007793E0, NiGeometry* self, NiGeometry* dest, void* cloning);
GAME_FN(void, __thiscall, TriBasedGeom_GetViewerStrings, 0x00779450, NiGeometry* self, NiTArray<char*>* strings);
GAME_FN(NiLinesData*, __thiscall, LinesData_Construct, 0x00779BD0, void* self, uint32_t count, NiPoint3* vertices,
        float* colors, float* textures, uint32_t textureSets, uint32_t nbtMethod, uint8_t* flags);

// ---- NiTriBasedGeom

NiGeometry* __fastcall TriBasedGeomConstructWithData(NiGeometry* self, re::Edx, NiObject* data) {
    Geometry_ConstructWithData(self, data);
    self->vtable = reinterpret_cast<void*>(NiTriBasedGeom_vtable);
    return self;
}

NiGeometry* __fastcall TriBasedGeomConstruct(NiGeometry* self, re::Edx) {
    Geometry_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTriBasedGeom_vtable);
    return self;
}

void __fastcall TriBasedGeomDestruct(NiGeometry* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiTriBasedGeom_vtable);
    Geometry_Destructor(self);
}

const RTTI* __fastcall TriBasedGeomGetRTTI(NiGeometry*, re::Edx) {
    return &NiTriBasedGeom_RTTI;
}

NiGeometry* __fastcall TriBasedGeomDeletingDestructor(NiGeometry* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiTriBasedGeom_vtable);
    Geometry_Destructor(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xEC);
    }
    return self;
}

void __fastcall TriBasedGeomGetViewerStrings(NiGeometry* self, re::Edx, NiTArray<char*>* strings) {
    Geometry_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTriBasedGeom_RTTI.name));
}

// ---- NiGeometry

NiGeometry* __fastcall GeometryDeletingDestructor(NiGeometry* self, re::Edx, uint32_t flags) {
    Geometry_Destructor(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xEC);
    }
    return self;
}

// ---- NiLines

// NiLines(count, vertices, colors, textures, sets, nbtMethod, flags): its own NiLinesData
NiGeometry* __fastcall LinesConstruct(NiGeometry* self, re::Edx, uint32_t count, NiPoint3* vertices, float* colors,
                                      float* textures, uint32_t textureSets, uint32_t nbtMethod, uint8_t* flags) {
    void* memory = NiNew(0x44);
    NiObject* data = nullptr;
    if (memory != nullptr) {
        data = LinesData_Construct(memory, count, vertices, colors, textures, textureSets, nbtMethod, flags);
    }
    Geometry_ConstructWithData(self, data);
    self->vtable = reinterpret_cast<void*>(NiLines_vtable);
    return self;
}

NiGeometry* __fastcall LinesConstructWithData(NiGeometry* self, re::Edx, NiObject* data) {
    Geometry_ConstructWithData(self, data);
    self->vtable = reinterpret_cast<void*>(NiLines_vtable);
    return self;
}

// vf34 RenderImmediate: the renderer's RenderLines (slot 98)
void __fastcall LinesRenderImmediate(NiGeometry* self, re::Edx, void* renderer) {
    Geometry_RenderImmediate(self, renderer);
    Virtual<void>(renderer, 98, self);
}

// vf29 UpdateEffectsDownward(state): a state with none of the lights' four lists (+0x10..+0x1C)
// kept as is; else a fresh state with the other effects (the list at +0xC) only
void __fastcall LinesUpdateEffectsDownward(NiGeometry* self, re::Edx, NiObject* state) {
    heapcheck::Enter(self, "NiLines::UpdateEffectsDownward");   // (testing: see HeapCheck.h)
    if (state == nullptr) {
        NiObject* old = self->effectState;
        if (old != nullptr) {
            heapcheck::Trace(old, heapcheck::kRelease);
            Release(old);
            self->effectState = nullptr;
        }
        heapcheck::Leave(self);
        return;
    }
    if (At<void*>(state, 0x10) == nullptr && At<void*>(state, 0x14) == nullptr && At<void*>(state, 0x18) == nullptr &&
        At<void*>(state, 0x1C) == nullptr) {
        heapcheck::Trace(self->effectState, heapcheck::kRelease);
        Pointer_Assign(&self->effectState, state);
        heapcheck::Trace(state, heapcheck::kAddRef);
        heapcheck::Leave(self);
        return;
    }
    void* memory = NiNew(0x20);
    NiObject* fresh = memory != nullptr ? DynamicEffectState_Construct(memory) : nullptr;
    NiObject* old = self->effectState;
    if (old != fresh) {
        heapcheck::Trace(old, heapcheck::kRelease);
        Release(old);
        self->effectState = fresh;
        AddRef(fresh);
    }
    // the effects' list: {next, effect} items
    for (re::Ptr32<void>* item = At<re::Ptr32<void>*>(state, 0xC); item != nullptr;) {
        NiObject* effect = static_cast<NiObject*>(item[1]);
        item = static_cast<re::Ptr32<void>*>(item[0]);
        DynamicEffectState_AddEffect(self->effectState, effect);
    }
    heapcheck::Leave(self);
}

// vf3 CreateClone (CopyMembers runs with a null clone when the allocation failed)
NiGeometry* __fastcall LinesCreateClone(NiGeometry* self, re::Edx, void* cloning) {
    NiGeometry* clone = static_cast<NiGeometry*>(NiNew(0xEC));
    if (clone != nullptr) {
        Geometry_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiLines_vtable);
        Geometry_CopyMembers(self, clone, cloning);
        return clone;
    }
    Geometry_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

void __fastcall LinesGetViewerStrings(NiGeometry* self, re::Edx, NiTArray<char*>* strings) {
    Geometry_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiLines_RTTI.name));
}

// ---- NiAVObject

// vf30 UpdateWorldData: the world transform from the parent's (the local one at the root), then
// the collision object's (slot 19, a tail call)
void __fastcall AVObjectUpdateWorldData(NiAVObject* self, re::Edx) {
    const NiTransform* world;
    NiTransform product;
    if (self->parent != nullptr) {
        world = Transform_Multiply(&self->parent->world, &product, &self->local);
    } else {
        world = &self->local;
    }
    self->world = *world;
    if (self->collisionObject != nullptr) {
        Virtual<void>(self->collisionObject, 19);
    }
}

// ---- NiTriShape

NiGeometry* __fastcall TriShapeConstructWithData(NiGeometry* self, re::Edx, NiObject* data) {
    TriBasedGeom_ConstructWithData(self, data);
    self->vtable = reinterpret_cast<void*>(NiTriShape_vtable);
    return self;
}

NiGeometry* __fastcall TriShapeConstruct(NiGeometry* self, re::Edx) {
    TriBasedGeom_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTriShape_vtable);
    return self;
}

NiGeometry* __fastcall TriShapeDeletingDestructor(NiGeometry* self, re::Edx, uint32_t flags) {
    TriBasedGeom_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xEC);
    }
    return self;
}

// vf38 GetTriangle(triangle, a, b, c): the corners' vertices
void __fastcall TriShapeGetTriangle(NiGeometry* self, re::Edx, uint16_t triangle, re::Ptr32<NiPoint3>* a, re::Ptr32<NiPoint3>* b,
                                    re::Ptr32<NiPoint3>* c) {
    NiTriShapeData* data = re::PtrCast<NiTriShapeData*>(self->modelData);
    NiPoint3* vertices = data->vertices;
    const uint16_t* list = data->triList;
    uint32_t i = uint32_t(triangle) * 3;
    *a = &vertices[list[i]];
    *b = &vertices[list[i + 1]];
    *c = &vertices[list[i + 2]];
}

// vf34 RenderImmediate: the renderer's RenderShape (slot 95)
void __fastcall TriShapeRenderImmediate(NiGeometry* self, re::Edx, void* renderer) {
    Geometry_RenderImmediate(self, renderer);
    Virtual<void>(renderer, 95, self);
}

NiGeometry* __fastcall TriShapeCreateClone(NiGeometry* self, re::Edx, void* cloning) {
    NiGeometry* clone = static_cast<NiGeometry*>(NiNew(0xEC));
    if (clone != nullptr) {
        TriBasedGeom_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiTriShape_vtable);
        TriBasedGeom_CopyMembers(self, clone, cloning);
        return clone;
    }
    TriBasedGeom_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

void __fastcall TriShapeGetViewerStrings(NiGeometry* self, re::Edx, NiTArray<char*>* strings) {
    TriBasedGeom_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTriShape_RTTI.name));
}

} // namespace

// lone jmps to the bases' (NiGeometry's) virtuals
RE_UNHOOKED("NiTriBasedGeom::SetModelData", 0x007793D0);   // jmp 0x7616D0
RE_UNHOOKED("NiTriBasedGeom::CopyMembers", 0x007793E0);    // jmp 0x761390
RE_UNHOOKED("NiTriBasedGeom::LoadBinary", 0x007793F0);     // jmp 0x7621F0
RE_UNHOOKED("NiTriBasedGeom::IsEqual", 0x00779400);        // jmp 0x7613D0
RE_UNHOOKED("NiLines::LinkObject", 0x007799D0);            // jmp 0x761940
RE_UNHOOKED("NiLines::SaveBinary", 0x007799E0);            // jmp 0x761A60
RE_UNHOOKED("NiTriShape::CopyMembers", 0x0077B8A0);        // jmp 0x7793E0
RE_UNHOOKED("NiTriShape::LoadBinary", 0x0077B8E0);         // jmp 0x7793F0
RE_UNHOOKED("NiTriShape::LinkObject", 0x0077B8F0);         // jmp 0x7799D0

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NITRISHAPE_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NITRISHAPE_REVERSED("NiTriBasedGeom::NiTriBasedGeomWithData", 0x00779380, &ni::TriBasedGeomConstructWithData);
NITRISHAPE_REVERSED("NiTriBasedGeom::NiTriBasedGeom", 0x007793A0, &ni::TriBasedGeomConstruct);
NITRISHAPE_REVERSED("NiTriBasedGeom::~NiTriBasedGeom", 0x007793C0, &ni::TriBasedGeomDestruct);
NITRISHAPE_REVERSED("NiTriBasedGeom::GetRTTI", 0x00779410, &ni::TriBasedGeomGetRTTI);
NITRISHAPE_REVERSED("NiTriBasedGeom::DeletingDestructor", 0x00779420, &ni::TriBasedGeomDeletingDestructor);
NITRISHAPE_REVERSED("NiTriBasedGeom::GetViewerStrings", 0x00779450, &ni::TriBasedGeomGetViewerStrings);
NITRISHAPE_REVERSED("NiLines::NiLines", 0x00779910, &ni::LinesConstruct);
NITRISHAPE_REVERSED("NiLines::NiLinesWithData", 0x00779980, &ni::LinesConstructWithData);
NITRISHAPE_REVERSED("NiGeometry::DeletingDestructor", 0x007799F0, &ni::GeometryDeletingDestructor);
NITRISHAPE_REVERSED("NiLines::RenderImmediate", 0x00779A20, &ni::LinesRenderImmediate);
NITRISHAPE_REVERSED("NiLines::UpdateEffectsDownward", 0x00779A40, &ni::LinesUpdateEffectsDownward);
NITRISHAPE_REVERSED("NiLines::CreateClone", 0x00779B30, &ni::LinesCreateClone);
NITRISHAPE_REVERSED("NiLines::GetViewerStrings", 0x00779B80, &ni::LinesGetViewerStrings);
NITRISHAPE_REVERSED("NiAVObject::UpdateWorldData", 0x00779EB0, &ni::AVObjectUpdateWorldData);
NITRISHAPE_REVERSED("NiTriShape::NiTriShapeWithData", 0x0077B860, &ni::TriShapeConstructWithData);
NITRISHAPE_REVERSED("NiTriShape::NiTriShape", 0x0077B880, &ni::TriShapeConstruct);
NITRISHAPE_REVERSED("NiTriShape::DeletingDestructor", 0x0077B900, &ni::TriShapeDeletingDestructor);
NITRISHAPE_REVERSED("NiTriShape::GetTriangle", 0x0077B930, &ni::TriShapeGetTriangle);
NITRISHAPE_REVERSED("NiTriShape::RenderImmediate", 0x0077B980, &ni::TriShapeRenderImmediate);
NITRISHAPE_REVERSED("NiTriShape::CreateClone", 0x0077B9A0, &ni::TriShapeCreateClone);
NITRISHAPE_REVERSED("NiTriShape::GetViewerStrings", 0x0077B9F0, &ni::TriShapeGetViewerStrings);
