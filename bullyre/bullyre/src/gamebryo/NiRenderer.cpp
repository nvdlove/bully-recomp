// NiRenderer's own code (vtable 0x9510A8; the one renderer is NiRenderer::ms_pkRenderer): the
// constructor (the default material made), the destructor, the static Remove* hooks the
// library's destructors call, camera data in and out, pixel/NDC conversions, and the small
// virtuals that forward to others.

#include <cmath>

#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t kNiRendererVtable = 0x009510A8;
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);
inline void* const ms_NiRendererRTTI = reinterpret_cast<void*>(0x020E2384);
GAME_VAR(const char*, ms_NiRendererRTTIName, 0x020E2384);

// NiRenderer's fields used here
struct NiRendererFields {
    re::Ptr32<void> vtable;                  // +0x0
    uint32_t refCount;             // +0x4
    re::Ptr32<NiRefObject> unknown8;         // +0x8 (a reference)
    re::Ptr32<void> unknownC;                // +0xC
    re::Ptr32<void> unknown10;               // +0x10
    re::Ptr32<NiRefObject> defaultMaterial;  // +0x14
    re::Ptr32<NiRefObject> currentMaterial;  // +0x18
    re::Ptr32<NiRefObject> unknown1C;        // +0x1C
    float unknown20[4];            // +0x20
    NiCriticalSection locks[3];    // +0x30, +0x50, +0x70
    uint8_t unknown90;             // +0x90
    uint8_t pad91[3];
    int32_t frameState;            // +0x94 (1, 2 and 4: inside a frame)
    uint32_t unknown98;
    uint32_t unknown9C;            // +0x9C
    uint8_t unknownA0;             // +0xA0
};
static_assert(offsetof(NiRendererFields, locks) == 0x30);
static_assert(offsetof(NiRendererFields, unknown90) == 0x90);
static_assert(offsetof(NiRendererFields, unknownA0) == 0xA0);

GAME_FN(void, __thiscall, NiObject_Construct, 0x00755260, void* self);
GAME_FN(void, __thiscall, NiObject_Destruct, 0x007769D0, void* self);
GAME_FN(void, __thiscall, NiRenderer_Destruct, 0x007558C0, void* self);
// the default material: its shader library's (one of the big generated functions), then
// the material (0x3C0 bytes)
GAME_FN(NiRefObject*, __cdecl, CreateDefaultMaterialLibrary, 0x00798130);
GAME_FN(NiRefObject*, __thiscall, NiDefaultMaterial_Construct, 0x00797F90, void* self, NiRefObject* library,
        int flags);
// the getters slot 77 passes to slot 76
GAME_FN(void*, __cdecl, Renderer_Getter762A00, 0x00762A00);
GAME_FN(uint8_t, __cdecl, Renderer_Getter786250, 0x00786250);
GAME_FN(uint8_t, __cdecl, Renderer_Getter786220, 0x00786220);
GAME_FN(uint8_t, __cdecl, Renderer_Getter786240, 0x00786240);
GAME_FN(uint8_t, __cdecl, Renderer_Getter786230, 0x00786230);
GAME_FN(uint32_t, __thiscall, Forward789060, 0x00789060, void* self, uint32_t a, float b, float c, float d, float e);
GAME_FN(uint32_t, __thiscall, Forward789300, 0x00789300, void* self, uint32_t a, uint32_t b, float c, float d,
        float e, float f);
GAME_FN(void, __thiscall, NiGeometry_ApplyMaterial, 0x00761F70, void* geometry, void* material, int index);
// cameras
GAME_FN(void, __thiscall, NiFrustum_Construct, 0x00771890, void* self, int ortho);
GAME_FN(void*, __thiscall, NiMatrix3_SetColumns, 0x00768780, void* self, const void* col0, const void* col1,
        const void* col2);
GAME_FN(void, __thiscall, NiTransform_Invert, 0x00770950, const void* self, void* inverse);
GAME_FN(void*, __thiscall, NiTransform_Multiply, 0x00406830, const void* self, void* result, const void* other);
GAME_FN(void, __thiscall, NiCamera_SetViewFrustum, 0x00762D60, void* camera, const void* frustum);
// viewer strings
GAME_FN(char*, __cdecl, NiGetViewerString, 0x00785560, const char* text);
GAME_FN(void, __thiscall, CharArray_SetSize, 0x0084A110, NiTArray<char*>* self, uint32_t size);
GAME_FN(void, __thiscall, CharArray_SetAt, 0x00833C70, NiTArray<char*>* self, uint32_t index, const re::Ptr32<char>* value);
GAME_FN(void, __thiscall, NiObject_GetViewerStrings, 0x007555A0, void* self, NiTArray<char*>* strings);

inline NiRendererFields* Fields(void* renderer) {
    return static_cast<NiRendererFields*>(renderer);
}

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// the renderer's static hooks: a slot of the renderer, when there is one
inline void CallRenderer(int slot, void* object) {
    if (ms_pkRenderer != nullptr) {
        Virtual<void>(ms_pkRenderer, slot, object);
    }
}

inline bool InsideFrame(NiRendererFields* r) {
    int32_t state = r->frameState;
    return (state == 1 || state == 2 || state == 4) && (r->unknown90 == 1 || state == 4);
}

} // namespace

// 0x755660, 0x755680, 0x7556A0, 0x7556C0, 0x7556E0: NiRenderer::RemoveEffect, RemoveGeometryData,
// RemoveMaterial, RemoveSkinInstance, RemoveSkinPartition (the renderer's purge slots 50, 48,
// 49, 51, 52)
void __cdecl NiRenderer_RemoveEffect(void* effect) {
    CallRenderer(50, effect);
}
void __cdecl NiRenderer_RemoveGeometryData(void* data) {
    CallRenderer(48, data);
}
void __cdecl NiRenderer_RemoveMaterial(void* material) {
    CallRenderer(49, material);
}
void __cdecl NiRenderer_RemoveSkinInstance(void* skin) {
    CallRenderer(51, skin);
}
void __cdecl NiRenderer_RemoveSkinPartition(void* partition) {
    CallRenderer(52, partition);
}

// 0x755720: NiRenderer slot 38: slot 37 with (32, 8)
void __fastcall NiRenderer_vf38(void* self, re::Edx, void* arg) {
    Virtual<void>(self, 37, arg, 0x20, 8);
}

// 0x755740: NiRenderer slot 77: slot 76 with five settings read through their getters (the
// getters return a byte; the game pushes all of eax, whose upper bytes are left over)
void __fastcall NiRenderer_vf77(void* self, re::Edx, void* arg) {
    void* e = Renderer_Getter762A00();
    uint8_t d = Renderer_Getter786250();
    uint8_t c = Renderer_Getter786220();
    uint8_t b = Renderer_Getter786240();
    uint8_t a = Renderer_Getter786230();
    Virtual<void>(self, 76, arg, uint32_t(a), uint32_t(b), uint32_t(c), uint32_t(d), e);
}

// 0x755780: NiRenderer::_SDMShutdown (NiMainSDM::Shutdown's): the renderer forgotten
void __cdecl NiRenderer_SDMShutdown() {
    ms_pkRenderer = nullptr;
}

// 0x755790: NiRenderer::ConvertFromPixelsToNDC (against the render target's width and height
// from slot 32)
void __fastcall NiRenderer_ConvertFromPixelsToNDC(void* self, re::Edx, uint32_t x, uint32_t y, float* ndcX,
                                                  float* ndcY) {
    void* target = Virtual<void*>(self, 32);
    *ndcX = float(x) / float(At<uint32_t>(target, 8));
    target = Virtual<void*>(self, 32);
    *ndcY = float(y) / float(At<uint32_t>(target, 0xC));
}

// 0x755810: NiRenderer::ConvertFromNDCToPixels (truncated)
void __fastcall NiRenderer_ConvertFromNDCToPixels(void* self, re::Edx, float ndcX, float ndcY, uint32_t* x,
                                                  uint32_t* y) {
    void* target = Virtual<void*>(self, 32);
    *x = uint32_t(crt::FloatToInt(float(At<uint32_t>(target, 8)) * ndcX));
    target = Virtual<void*>(self, 32);
    *y = uint32_t(crt::FloatToInt(float(At<uint32_t>(target, 0xC)) * ndcY));
}

// 0x7558C0: ~NiRenderer
void __fastcall NiRenderer_DestructImpl(void* self, re::Edx) {
    NiRendererFields* r = Fields(self);
    r->vtable = reinterpret_cast<void*>(kNiRendererVtable);
    ms_pkRenderer = nullptr;
    Import<void>(kDeleteCriticalSection, &r->locks[2].section);
    Import<void>(kDeleteCriticalSection, &r->locks[1].section);
    Import<void>(kDeleteCriticalSection, &r->locks[0].section);
    ReleaseRef(r->unknown1C);
    ReleaseRef(r->currentMaterial);
    ReleaseRef(r->defaultMaterial);
    ReleaseRef(r->unknown8);
    NiObject_Destruct(self);
}

// 0x755970: NiRenderer::GetRTTI
void* __fastcall NiRenderer_GetRTTI(void*, re::Edx) {
    return ms_NiRendererRTTI;
}

// 0x755980: NiRenderer slot 91: two fields set
void __fastcall NiRenderer_vf91(void* self, re::Edx, void* a, void* b) {
    Fields(self)->unknownC = a;
    Fields(self)->unknown10 = b;
}

// 0x7559A0: NiRenderer::SetDefaultMaterial (none: the renderer's own)
void __fastcall NiRenderer_SetDefaultMaterial(void* self, re::Edx, NiRefObject* material) {
    NiRendererFields* r = Fields(self);
    if (material == nullptr) {
        material = r->defaultMaterial;
    }
    NiRefObject* old = r->currentMaterial;
    if (old != material) {
        ReleaseRef(old);
        r->currentMaterial = material;
        if (material != nullptr) {
            Increment(&material->refCount);
        }
    }
}

// 0x7559F0 and 0x755A30: forwarded to the object at +0xCC
uint32_t __fastcall NiRenderer_Forward789060(void* self, re::Edx, uint32_t a, float b, float c, float d, float e) {
    return Forward789060(At<void*>(self, 0xCC), a, b, c, d, e);
}
uint32_t __fastcall NiRenderer_Forward789300(void* self, re::Edx, uint32_t a, uint32_t b, float c, float d, float e,
                                             float f) {
    return Forward789300(At<void*>(self, 0xCC), a, b, c, d, e, f);
}

// 0x755A70: NiRenderer's constructor (it becomes ms_pkRenderer; its default material made)
void* __fastcall NiRenderer_Construct(void* self, re::Edx) {
    NiObject_Construct(self);
    NiRendererFields* r = Fields(self);
    r->vtable = reinterpret_cast<void*>(kNiRendererVtable);
    r->unknown8 = nullptr;
    r->defaultMaterial = nullptr;
    r->currentMaterial = nullptr;
    r->unknown1C = nullptr;
    for (float& f : r->unknown20) {
        f = 0.0f;
    }
    for (NiCriticalSection& cs : r->locks) {
        cs.owner = 0;
        cs.lockCount = 0;
        Import<void>(kInitializeCriticalSection, &cs.section);
    }
    r->unknown90 = 0;
    r->frameState = 0;
    r->unknown9C = 0;
    r->unknownA0 = 0;
    ms_pkRenderer = self;
    NiRefObject* library = CreateDefaultMaterialLibrary();
    if (library != nullptr) {
        Increment(&library->refCount);
    }
    void* memory = NiNew(0x3C0);
    NiRefObject* material = memory != nullptr ? NiDefaultMaterial_Construct(memory, library, 0) : nullptr;
    Virtual<void>(material, 13);
    NiRefObject* old = r->defaultMaterial;
    if (old != material) {
        ReleaseRef(old);
        r->defaultMaterial = material;
        Increment(&material->refCount);
    }
    ReleaseRef(library);
    return self;
}

// 0x755B80: NiRenderer's deleting destructor
void* __fastcall NiRenderer_Destroy(void* self, re::Edx, int flags) {
    NiRenderer_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0xA4);
    }
    return self;
}

// 0x755BB0 and 0x755C30: NiRenderer slots 93 and 94: the target's two objects (+0xC4, +0xC8)
// kept (held through a temporary reference), then slots 95 / 96
namespace {
inline void TakeTargetObjects(NiRendererFields* r, void* target) {
    NiRefObject* a = At<NiRefObject*>(target, 0xC4);
    if (a != nullptr) {
        Increment(&a->refCount);
    }
    r->unknownC = a;
    ReleaseRef(a);
    NiRefObject* b = At<NiRefObject*>(target, 0xC8);
    if (b != nullptr) {
        Increment(&b->refCount);
    }
    r->unknown10 = b;
    ReleaseRef(b);
}
} // namespace
void __fastcall NiRenderer_vf93(void* self, re::Edx, void* target) {
    TakeTargetObjects(Fields(self), target);
    Virtual<void>(self, 95, target);
}
void __fastcall NiRenderer_vf94(void* self, re::Edx, void* target) {
    TakeTargetObjects(Fields(self), target);
    Virtual<void>(self, 96, target);
}

// 0x755CB0: NiRenderer::SetCameraData(camera): its world position, direction, up and right
// (the rotation's columns), frustum and viewport to slot 86, inside a frame
void __fastcall NiRenderer_SetCameraData(void* self, re::Edx, uint8_t* camera) {
    const float* rotate = reinterpret_cast<const float*>(camera + 0x60);
    float right[3] = {rotate[2], rotate[5], rotate[8]};
    float up[3] = {rotate[1], rotate[4], rotate[7]};
    float direction[3] = {rotate[0], rotate[3], rotate[6]};
    if (InsideFrame(Fields(self))) {
        Virtual<void>(self, 86, camera + 0x84, direction, up, right, camera + 0x104, camera + 0x128);
    }
}

// 0x755D50: NiRenderer::GetCameraData(camera): the renderer's camera (slot 88, inside a frame)
// set as the camera's world transform; its local one made relative to its parent
void __fastcall NiRenderer_GetCameraData(void* self, re::Edx, uint8_t* camera) {
    alignas(4) uint8_t frustum[0x1C];
    NiFrustum_Construct(frustum, 0);
    float viewport[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    // (outside a frame the game uses these uninitialised: ours are zeros)
    float location[3] = {};
    float direction[3] = {};
    float up[3] = {};
    float right[3] = {};
    if (InsideFrame(Fields(self))) {
        Virtual<void>(self, 88, location, direction, up, right, frustum, viewport);
    }
    NiTransform world;
    world.translate = {location[0], location[1], location[2]};
    NiMatrix3 columns;
    world.rotate = *static_cast<NiMatrix3*>(NiMatrix3_SetColumns(&columns, direction, up, right));
    world.scale = 1.0f;
    NiTransform* local = &world;
    void* parent = At<void*>(camera, 0x18);
    NiTransform inverse;
    NiTransform product;
    if (parent != nullptr) {
        NiTransform_Invert(static_cast<uint8_t*>(parent) + 0x60, &inverse);
        local = static_cast<NiTransform*>(NiTransform_Multiply(&inverse, &product, &world));
    }
    NiTransform copy = *local;
    At<float>(camera, 0x5C) = std::fabs(copy.scale);
    At<float>(camera, 0x90) = world.scale;
    At<NiPoint3>(camera, 0x50) = copy.translate;
    At<NiMatrix3>(camera, 0x2C) = copy.rotate;
    At<NiPoint3>(camera, 0x84) = world.translate;
    At<NiMatrix3>(camera, 0x60) = world.rotate;
    NiCamera_SetViewFrustum(camera, frustum);
    crt::Memcpy(camera + 0x128, viewport, 0x10);
}

// 0x755F20: NiRenderer slot 42: the current material applied to a geometry
void __fastcall NiRenderer_vf42(void* self, re::Edx, void* geometry) {
    NiGeometry_ApplyMaterial(geometry, Fields(self)->currentMaterial, -1);
}

// 0x755F40: NiRenderer::GetViewerStrings
void __fastcall NiRenderer_GetViewerStrings(void* self, re::Edx, NiTArray<char*>* strings) {
    NiObject_GetViewerStrings(self, strings);
    re::Ptr32<char> text = NiGetViewerString(ms_NiRendererRTTIName);
    uint32_t size = strings->size;
    if (size >= strings->maxSize) {
        CharArray_SetSize(strings, size + strings->growBy);
    }
    CharArray_SetAt(strings, size, &text);
}

#define NIRENDERER_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIRENDERER_REVERSED("NiRenderer::RemoveEffect", 0x00755660, NiRenderer_RemoveEffect);
NIRENDERER_REVERSED("NiRenderer::RemoveGeometryData", 0x00755680, NiRenderer_RemoveGeometryData);
NIRENDERER_REVERSED("NiRenderer::RemoveMaterial", 0x007556A0, NiRenderer_RemoveMaterial);
NIRENDERER_REVERSED("NiRenderer::RemoveSkinInstance", 0x007556C0, NiRenderer_RemoveSkinInstance);
NIRENDERER_REVERSED("NiRenderer::RemoveSkinPartition", 0x007556E0, NiRenderer_RemoveSkinPartition);
NIRENDERER_REVERSED("NiRenderer::vf38", 0x00755720, NiRenderer_vf38);
NIRENDERER_REVERSED("NiRenderer::vf77", 0x00755740, NiRenderer_vf77);
// (never hooked in the 32-bit game: a body at its address for the 64-bit build)
RE_UNHOOKED_FN("NiRenderer::_SDMShutdown", 0x00755780, &NiRenderer_SDMShutdown);
NIRENDERER_REVERSED("NiRenderer::ConvertFromPixelsToNDC", 0x00755790, NiRenderer_ConvertFromPixelsToNDC);
NIRENDERER_REVERSED("NiRenderer::ConvertFromNDCToPixels", 0x00755810, NiRenderer_ConvertFromNDCToPixels);
NIRENDERER_REVERSED("NiRenderer::~NiRenderer", 0x007558C0, NiRenderer_DestructImpl);
NIRENDERER_REVERSED("NiRenderer::GetRTTI", 0x00755970, NiRenderer_GetRTTI);
NIRENDERER_REVERSED("NiRenderer::vf91", 0x00755980, NiRenderer_vf91);
NIRENDERER_REVERSED("NiRenderer::SetDefaultMaterial", 0x007559A0, NiRenderer_SetDefaultMaterial);
NIRENDERER_REVERSED("NiRenderer::Forward789060", 0x007559F0, NiRenderer_Forward789060);
NIRENDERER_REVERSED("NiRenderer::Forward789300", 0x00755A30, NiRenderer_Forward789300);
NIRENDERER_REVERSED("NiRenderer::NiRenderer", 0x00755A70, NiRenderer_Construct);
NIRENDERER_REVERSED("NiRenderer::Destroy", 0x00755B80, NiRenderer_Destroy);
NIRENDERER_REVERSED("NiRenderer::vf93", 0x00755BB0, NiRenderer_vf93);
NIRENDERER_REVERSED("NiRenderer::vf94", 0x00755C30, NiRenderer_vf94);
NIRENDERER_REVERSED("NiRenderer::SetCameraData", 0x00755CB0, NiRenderer_SetCameraData);
NIRENDERER_REVERSED("NiRenderer::GetCameraData", 0x00755D50, NiRenderer_GetCameraData);
NIRENDERER_REVERSED("NiRenderer::vf42", 0x00755F20, NiRenderer_vf42);
NIRENDERER_REVERSED("NiRenderer::GetViewerStrings", 0x00755F40, NiRenderer_GetViewerStrings);

} // namespace ni
