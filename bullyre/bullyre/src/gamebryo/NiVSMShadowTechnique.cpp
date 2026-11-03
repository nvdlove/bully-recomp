// NiVSMShadowTechnique (vtable 0x9606EC, 0x6C bytes: an NiShadowTechnique, src/gamebryo/
// NiDefaultShadowClickGenerator.cpp has its constructor): variance shadow maps, blurred after
// they're rendered by two screen-filling passes (a horizontal pass into the technique's own blur
// texture, then back into the shadow map). Its slots: RTTI, destruction, PrepareShadowMap (the
// map's filtering), AppendPostProcessRenderClicks (the two blur clicks), and the blur resources
// it keeps (made again when a shadow map is larger than they are).
//
// With the pieces they use: the shadow click generator's pool of post-process render clicks
// (each a screen-filling view with a texturing property), NiScreenFillingRenderView::
// AttachProperty, NiTexturingProperty::SetBaseTextureTransform, NiTextureTransform's default
// constructor and SetScale.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiVSMShadowTechnique_RTTI, 0x020E379C);
GAME_VAR(RTTI, NiScreenFillingRenderView_RTTIObject, 0x020E41F0);   // (NiDynamicCast's argument)
GAME_VAR(uint32_t, kNiShadowCubeMapClassIndex, 0x020E2584);
GAME_VAR(uint32_t, kNiScreenFillingRenderViewClassIndex, 0x020E41F8);
GAME_VAR(uint32_t, kNiTexturingPropertyClassIndex, 0x020E28C4);
GAME_VAR(uint8_t*, ms_pkRenderer, 0x020E2380);
GAME_VAR(float, NiPoint2_ZERO_x, 0x020E3940);
GAME_VAR(float, NiPoint2_ZERO_y, 0x020E3944);
GAME_VAR(float, kHalf, 0x009055A0);

constexpr uintptr_t NiVSMShadowTechnique_vtable = 0x009606EC;

struct NiVSMShadowTechnique {
    re::Ptr32<void> vtable;                 // +0x0
    uint8_t base[0x34];           // +0x4 (NiShadowTechnique's: the name, fragments, ...)
    uint32_t formatPrefs[3];      // +0x38 (the shadow maps' texture prefs)
    uint8_t base44[8];            // +0x44
    uint8_t disabled;             // +0x4C (no blurring)
    uint8_t settingsChanged;      // +0x4D (the blur quads' slot 21 called once with 1)
    uint16_t pad4E;
    uint32_t blurKernel;          // +0x50 (0: no blurring)
    float blurWidth;              // +0x54 (the blur texture's size)
    float blurHeight;             // +0x58
    re::Ptr32<NiRefObject> blurTexture;     // +0x5C (NiRenderedTexturePtr)
    re::Ptr32<NiRefObject> blurTarget;      // +0x60 (NiRenderTargetGroupPtr)
    re::Ptr32<NiRefObject> blurLibrary;     // +0x64 (the shadow click generator's node library)
    re::Ptr32<NiRefObject> blurMaterial;    // +0x68 (NiVSMBlurMaterialPtr)
};
static_assert(offsetof(NiVSMShadowTechnique, disabled) == 0x4C);
static_assert(offsetof(NiVSMShadowTechnique, blurTexture) == 0x5C);
static_assert(sizeof(NiVSMShadowTechnique) == 0x6C);

// NiTextureTransform (0x48 bytes)
struct NiTextureTransform {
    float translate[2];   // +0x0
    float rotate;         // +0x8
    float scale[2];       // +0xC
    float center[2];      // +0x14
    uint8_t dirty;        // +0x1C (the matrix to make again)
    uint8_t pad1D[3];
    float matrix[9];      // +0x20
    uint32_t method;      // +0x44 (2)
};
static_assert(offsetof(NiTextureTransform, dirty) == 0x1C);
static_assert(sizeof(NiTextureTransform) == 0x48);

// NiShadowClickGenerator's pool: its render clicks (an NiTObjectSet at +8) and how many of them
// this frame has used (+0x14)
struct ClickPool {
    re::Ptr32<void> vtable;
    uint32_t refCount;
    re::Ptr32<re::Ptr32<NiRefObject>> clicks;   // +0x8
    uint32_t allocated;     // +0xC
    uint32_t count;         // +0x10
    uint32_t used;          // +0x14
};

GAME_FN(void, __thiscall, ShadowTechnique_Destruct, 0x007D0F40, void* self);
GAME_FN(void*, __cdecl, RenderedTexture_Create, 0x0075D960, uint32_t width, uint32_t height, void* renderer,
        const uint32_t* prefs, uint32_t arg);
GAME_FN(void*, __cdecl, RenderTargetGroup_CreateForBuffer, 0x0075D720, void* buffer, void* renderer,
        int useDefaultDepth, int createCompatibleDepth);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* self, void* value);
GAME_FN(void*, __cdecl, CreateShadowClickGeneratorLibrary, 0x007ECED0);
GAME_FN(void*, __thiscall, VSMBlurMaterial_Construct, 0x007ECDD0, void* self, NiRefObject* library, int autoCreate);
GAME_FN(void*, __cdecl, DynamicCast, 0x00401FF0, const void* rtti, void* object);   // NiDynamicCast
GAME_FN(void*, __thiscall, AVObject_GetProperty, 0x0075DC10, void* self, int type);
GAME_FN(void, __thiscall, AVObject_AttachProperty, 0x004099F0, void* self, void* property);   // (the game's)
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, void* self);
GAME_FN(void*, __thiscall, ZBufferProperty_Construct, 0x00407B40, void* self);   // (the game's)
GAME_FN(void*, __thiscall, StencilProperty_Construct, 0x005191D0, void* self);   // (the game's)
GAME_FN(void*, __thiscall, ViewRenderClick_Construct, 0x00518D00, void* self);   // (the game's)
GAME_FN(void, __thiscall, ViewRenderClick_AppendRenderView, 0x008316F0, void* click, void* view);
GAME_FN(void*, __thiscall, ScreenFillingRenderView_Construct, 0x007EAED0, void* self);
GAME_FN(void*, __thiscall, TexturingProperty_Construct, 0x00765DD0, void* self);
GAME_FN(void*, __thiscall, TexturingMap_Construct, 0x00765BA0, void* self);
GAME_FN(void, __thiscall, MapArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, void* set, const re::Ptr32<NiRefObject>* element);
GAME_FN(uint8_t, __thiscall, Geometry_ApplyAndSetActiveMaterial, 0x00761F70, void* self, void* material, int index);
GAME_FN(void, __thiscall, PropertyList_AddHead, 0x00407BD0, void* list, const re::Ptr32<NiRefObject>* property);   // (the game's)
GAME_FN(void, __thiscall, ClickGenerator_AddRenderClick, 0x007CF680, void* self, void* click);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, VSM_Destruct, 0x007D1280, NiVSMShadowTechnique* self);
GAME_FN(uint8_t, __thiscall, VSM_UpdateBlurResources, 0x007D1380, NiVSMShadowTechnique* self, void* map);
GAME_FN(void*, __thiscall, ClickPool_GetPostProcessRenderClick, 0x007ABBD0, ClickPool* self, void* texture,
        void* target, void* material);
GAME_FN(void, __thiscall, ScreenFillingView_AttachProperty, 0x007ABAF0, void* self, NiRefObject* property);
GAME_FN(NiTextureTransform*, __thiscall, TextureTransform_ConstructDefault, 0x00765A10, void* self);
GAME_FN(void, __thiscall, TextureTransform_SetScale, 0x007D0E70, NiTextureTransform* self, const float* scale);
GAME_FN(void, __thiscall, TexturingProperty_SetBaseTextureTransform, 0x007D0EE0, void* self,
        NiTextureTransform* transform);

inline uint8_t IsKindOf(void* object, uint32_t classIndex) {
    const uint32_t* rtti = Virtual<const uint32_t*>(object, 2);
    return (rtti[3 + (classIndex >> 5)] & (1u << (classIndex & 31))) != 0 ? 1 : 0;
}

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// a reference member let go and cleared
inline void Clear(re::Ptr32<NiRefObject>& slot) {
    NiRefObject* object = slot;
    if (object != nullptr) {
        ReleaseRef(object);
        slot = nullptr;
    }
}

// NiPointer::operator= as the code inlines it: the old one released first
inline void AssignRef(re::Ptr32<NiRefObject>& slot, NiRefObject* value) {
    NiRefObject* old = slot;
    if (old != value) {
        ReleaseRef(old);
        slot = value;
        if (value != nullptr) {
            Increment(&value->refCount);
        }
    }
}

// a texture's size as the x87 loads it (an unsigned value)
inline double UnsignedValue(uint32_t value) {
    return double(value);
}

// the property's flags (a property's +0x18), written through a pointer the code doesn't check:
// a failed NiNew writes at 0x18 as the game does
inline uint16_t& PropertyFlags(void* property) {
    return *reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(property) + 0x18);
}

// the blur quad's z-buffer (no test, no write) and stencil (0xC00) properties, made when missing
inline void SetUpQuadProperties(NiVSMShadowTechnique* self, void* quad) {
    void* zbuffer = AVObject_GetProperty(quad, 0xB);
    if (zbuffer == nullptr) {
        void* memory = NiNew(0x30);
        void* created = memory != nullptr ? ZBufferProperty_Construct(memory) : nullptr;
        PropertyFlags(created) &= 0xFFFC;
        AVObject_AttachProperty(quad, created);
    } else {
        PropertyFlags(zbuffer) &= 0xFFFC;
    }
    void* stencil = AVObject_GetProperty(quad, 7);
    if (stencil == nullptr) {
        void* memory = NiNew(0x24);
        void* created = memory != nullptr ? StencilProperty_Construct(memory) : nullptr;
        PropertyFlags(created) |= 0xC00;
        AVObject_AttachProperty(quad, created);
    } else {
        PropertyFlags(stencil) |= 0xC00;
    }
    if (self->settingsChanged != 0) {
        Virtual<void>(quad, 21, 1);
    }
}

// a render click's first view as an NiScreenFillingRenderView (NiDynamicCast)
inline void* ScreenFillingView(void* click) {
    void* firstView = At<void*>(At<void*>(click, 0x50), 8);
    return DynamicCast(&NiScreenFillingRenderView_RTTIObject, firstView);
}

} // namespace

// ---- NiScreenFillingRenderView and NiShadowClickGenerator

// 0x7ABAF0: NiScreenFillingRenderView::AttachProperty (the quad's, the properties marked changed)
void __fastcall NiScreenFillingRenderView_AttachProperty(void* self, re::Edx, NiRefObject* property) {
    void* quad = At<void*>(self, 0x20);
    re::Ptr32<NiRefObject> held = property;
    if (held != nullptr) {
        Increment(&held->refCount);
    }
    PropertyList_AddHead(static_cast<uint8_t*>(quad) + 0x94, &held);
    ReleaseRef(held);
    At<uint8_t>(self, 0x38) = 1;
}

// 0x7ABBD0: the next post-process render click of the pool (a new one when they're all used this
// frame: a screen-filling view with a texturing property, appended to the pool), its quad given
// the material, its base map the texture, the click the render target
void* __fastcall NiShadowClickGenerator_GetPostProcessRenderClick(ClickPool* self, re::Edx, void* texture, void* target,
                                                                  void* material) {
    NiRefObject* click;
    if (self->used < self->count) {
        click = self->clicks[self->used];
    } else {
        void* memory = NiNew(0x80);
        click = static_cast<NiRefObject*>(memory != nullptr ? ViewRenderClick_Construct(memory) : nullptr);
        memory = NiNew(0x3C);
        void* view = memory != nullptr ? ScreenFillingRenderView_Construct(memory) : nullptr;
        memory = NiNew(0x34);
        uint8_t* property = static_cast<uint8_t*>(memory != nullptr ? TexturingProperty_Construct(memory) : nullptr);
        memory = NiNew(0x10);
        re::Ptr32<void> map = memory != nullptr ? TexturingMap_Construct(memory) : nullptr;
        void* old = At<re::Ptr32<void>*>(property, 0x20)[0];
        if (map != old) {
            if (old != nullptr) {
                Virtual<void>(old, 0, 1);
            }
            MapArray_SetAt(property + 0x1C, 0, &map);
        }
        ScreenFillingView_AttachProperty(view, reinterpret_cast<NiRefObject*>(property));
        ViewRenderClick_AppendRenderView(click, view);
        At<uint32_t>(click, 0x34) = 7;
        re::Ptr32<NiRefObject> held = click;
        Increment(&click->refCount);
        ObjectSet_Add(&self->clicks, &held);
        if (Decrement(&click->refCount) == 0) {
            Virtual<void>(click, 1);
        }
    }
    void* firstView = At<void*>(At<void*>(click, 0x50), 8);
    void* view = firstView != nullptr && IsKindOf(firstView, kNiScreenFillingRenderViewClassIndex) ? firstView : nullptr;
    void* quad = At<void*>(view, 0x20);
    Geometry_ApplyAndSetActiveMaterial(quad, material, -1);
    void* property = AVObject_GetProperty(quad, 8);
    void* texturing = property != nullptr && IsKindOf(property, kNiTexturingPropertyClassIndex) ? property : nullptr;
    void* baseMap = At<re::Ptr32<void>*>(texturing, 0x20)[0];
    AssignRef(At<NiRefObject*>(baseMap, 8), static_cast<NiRefObject*>(texture));
    AssignRef(At<NiRefObject*>(click, 0x10), static_cast<NiRefObject*>(target));
    self->used++;
    return click;
}

// ---- NiTextureTransform and NiTexturingProperty

// 0x765A10: NiTextureTransform::NiTextureTransform (no translation or rotation, unit scale,
// centered; its matrix to make)
NiTextureTransform* __fastcall NiTextureTransform_ConstructDefault(NiTextureTransform* self, re::Edx) {
    self->translate[0] = NiPoint2_ZERO_x;
    self->rotate = 0.0f;
    self->translate[1] = NiPoint2_ZERO_y;
    self->scale[0] = 1.0f;
    self->scale[1] = 1.0f;
    self->center[0] = kHalf;
    self->center[1] = kHalf;
    self->dirty = 1;
    self->method = 2;
    return self;
}

// 0x7D0E70: NiTextureTransform::SetScale (the matrix marked dirty unless it's clean and the scale
// the same)
void __fastcall NiTextureTransform_SetScale(NiTextureTransform* self, re::Edx, const float* scale) {
    if (self->dirty == 0 && scale[0] == self->scale[0] && scale[1] == self->scale[1]) {
        self->dirty = 0;
    } else {
        self->dirty = 1;
    }
    self->scale[0] = scale[0];
    self->scale[1] = scale[1];
}

// 0x7D0EE0: NiTexturingProperty::SetBaseTextureTransform (a base map made when there's none; the
// transform kept, not copied)
void __fastcall NiTexturingProperty_SetBaseTextureTransform(uint8_t* self, re::Edx, NiTextureTransform* transform) {
    void* map = At<re::Ptr32<void>*>(self, 0x20)[0];
    if (map != nullptr) {
        At<NiTextureTransform*>(map, 0xC) = transform;
        return;
    }
    void* memory = NiNew(0x10);
    re::Ptr32<void> created = memory != nullptr ? TexturingMap_Construct(memory) : nullptr;
    MapArray_SetAt(self + 0x1C, 0, &created);
    *re::PtrCast<re::Ptr32<NiTextureTransform>*>(re::PtrCast<uintptr_t>(created) + 0xC) = transform;
}

// ---- NiVSMShadowTechnique

// 0x7D0EC0: PrepareShadowMap (slot 3): the map's texture filtered (its flags' filter 1)
void __fastcall NiVSMShadowTechnique_PrepareShadowMap(NiVSMShadowTechnique*, re::Edx, uint8_t* map) {
    At<uint16_t>(map, 4) = uint16_t((At<uint16_t>(map, 4) & 0xF1FF) | 0x100);
}

// 0x7D1060: GetRTTI (slot 0)
const RTTI* __fastcall NiVSMShadowTechnique_GetRTTI(NiVSMShadowTechnique*, re::Edx) {
    return &NiVSMShadowTechnique_RTTI;
}

// 0x7D1280: the destructor: the blur resources let go (then the members' destructors, with
// nothing left), NiShadowTechnique's
void __fastcall NiVSMShadowTechnique_Destruct(NiVSMShadowTechnique* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiVSMShadowTechnique_vtable);
    Clear(self->blurTexture);
    Clear(self->blurTarget);
    Clear(self->blurMaterial);
    Clear(self->blurLibrary);
    ReleaseRef(self->blurMaterial);
    ReleaseRef(self->blurLibrary);
    ReleaseRef(self->blurTarget);
    ReleaseRef(self->blurTexture);
    ShadowTechnique_Destruct(self);
}

// 0x7D1380: the blur resources for a shadow map: the blur texture and its render target made
// again when the map is larger than they are; the library and material made once; false when
// blurring is off
uint8_t __fastcall NiVSMShadowTechnique_UpdateBlurResources(NiVSMShadowTechnique* self, re::Edx, uint8_t* map) {
    if (self->disabled != 0) {
        return 0;
    }
    void* mapTexture = At<void*>(map, 8);
    bool recreate = double(self->blurWidth) < UnsignedValue(Virtual<uint32_t>(mapTexture, 18));
    if (!recreate) {
        recreate = double(self->blurHeight) < UnsignedValue(Virtual<uint32_t>(At<void*>(map, 8), 19));
    }
    if (recreate) {
        self->blurWidth = float(UnsignedValue(Virtual<uint32_t>(At<void*>(map, 8), 18)));
        self->blurHeight = float(UnsignedValue(Virtual<uint32_t>(At<void*>(map, 8), 19)));
        Clear(self->blurTexture);
        Clear(self->blurTarget);
    }
    if (self->blurTarget == nullptr) {
        uint32_t height = uint32_t(crt::FloatToInt(self->blurHeight));
        uint32_t width = uint32_t(crt::FloatToInt(self->blurWidth));
        void* texture = RenderedTexture_Create(width, height, ms_pkRenderer, self->formatPrefs, 0x100);
        Pointer_Assign(&self->blurTexture, texture);
        void* buffer = Virtual<void*>(self->blurTexture, 20);
        void* target = RenderTargetGroup_CreateForBuffer(buffer, ms_pkRenderer, 0, 0);
        Pointer_Assign(&self->blurTarget, target);
        if (self->blurMaterial == nullptr || self->blurLibrary == nullptr) {
            Pointer_Assign(&self->blurLibrary, CreateShadowClickGeneratorLibrary());
            void* memory = NiNew(0x3BC);
            void* material = memory != nullptr ? VSMBlurMaterial_Construct(memory, self->blurLibrary, 0) : nullptr;
            Pointer_Assign(&self->blurMaterial, material);
            Virtual<void>(ms_pkRenderer, 77, self->blurMaterial);
        }
    }
    return 1;
}

// 0x7D1580: the deleting destructor (slot 1)
NiVSMShadowTechnique* __fastcall NiVSMShadowTechnique_DeletingDestructor(NiVSMShadowTechnique* self, re::Edx,
                                                                         uint32_t flags) {
    VSM_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiVSMShadowTechnique));
    }
    return self;
}

// 0x7D15A0: AppendPostProcessRenderClicks(map, click generator) (slot 2): not for cube maps; the
// shadow map blurred into the blur texture (the viewport its part of it), then back into the map
// (its texture transform scaled to that part, flipped)
void __fastcall NiVSMShadowTechnique_AppendPostProcessRenderClicks(NiVSMShadowTechnique* self, re::Edx, uint8_t* map,
                                                                   ClickPool* generator) {
    if (map != nullptr) {
        // (a shadow map's GetRTTI is its slot 0)
        const uint32_t* rtti = Virtual<const uint32_t*>(map, 0);
        uint32_t index = kNiShadowCubeMapClassIndex;
        if ((rtti[3 + (index >> 5)] & (1u << (index & 31))) != 0) {
            return;
        }
    }
    if (self->blurKernel == 0 || !VSM_UpdateBlurResources(self, map)) {
        return;
    }
    float scaleX = float(UnsignedValue(Virtual<uint32_t>(At<void*>(map, 8), 18))) / self->blurWidth;
    float scaleY = float(UnsignedValue(Virtual<uint32_t>(At<void*>(map, 8), 19))) / self->blurHeight;
    float left = 0.0f;
    float top = 1.0f;
    float right = scaleX;
    float bottom = 1.0f - scaleY;
    uint8_t* click = static_cast<uint8_t*>(ClickPool_GetPostProcessRenderClick(
        generator, At<void*>(map, 8), self->blurTarget, self->blurMaterial));
    At<float>(click, 0x14) = left;
    At<float>(click, 0x18) = right;
    At<float>(click, 0x1C) = top;
    At<float>(click, 0x20) = bottom;
    void* view = ScreenFillingView(click);
    void* quad = At<void*>(view, 0x20);
    PropertyFlags(AVObject_GetProperty(quad, 8)) &= 0xFFF1;
    SetUpQuadProperties(self, quad);
    AVObject_UpdateProperties(quad);
    ClickGenerator_AddRenderClick(generator, click);

    void* mapClick = At<void*>(map, 0xC);
    void* back = ClickPool_GetPostProcessRenderClick(generator, self->blurTexture, At<void*>(mapClick, 0x10),
                                                     self->blurMaterial);
    void* backView = ScreenFillingView(back);
    void* backQuad = At<void*>(backView, 0x20);
    uint8_t* texturing = static_cast<uint8_t*>(AVObject_GetProperty(backQuad, 8));
    PropertyFlags(texturing) = uint16_t((PropertyFlags(texturing) & 0xFFF5) | 4);
    void* baseMap = At<re::Ptr32<void>*>(texturing, 0x20)[0];
    NiTextureTransform* transform;
    if (baseMap == nullptr || (transform = At<NiTextureTransform*>(baseMap, 0xC)) == nullptr) {
        void* memory = NiNew(0x48);
        transform = memory != nullptr ? TextureTransform_ConstructDefault(memory) : nullptr;
        TexturingProperty_SetBaseTextureTransform(texturing, transform);
    }
    SetUpQuadProperties(self, backQuad);
    float scale[2] = {scaleX, -scaleY};
    TextureTransform_SetScale(transform, scale);
    AVObject_UpdateProperties(backQuad);
    ClickGenerator_AddRenderClick(generator, back);
    self->settingsChanged = 0;
}

#define NIVSM_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIVSM_REVERSED("NiTextureTransform::NiTextureTransformDefault", 0x00765A10, NiTextureTransform_ConstructDefault);
NIVSM_REVERSED("NiScreenFillingRenderView::AttachProperty", 0x007ABAF0, NiScreenFillingRenderView_AttachProperty);
NIVSM_REVERSED("NiShadowClickGenerator::GetPostProcessRenderClick", 0x007ABBD0, NiShadowClickGenerator_GetPostProcessRenderClick);
NIVSM_REVERSED("NiTextureTransform::SetScale", 0x007D0E70, NiTextureTransform_SetScale);
NIVSM_REVERSED("NiVSMShadowTechnique::PrepareShadowMap", 0x007D0EC0, NiVSMShadowTechnique_PrepareShadowMap);
NIVSM_REVERSED("NiTexturingProperty::SetBaseTextureTransform", 0x007D0EE0, NiTexturingProperty_SetBaseTextureTransform);
NIVSM_REVERSED("NiVSMShadowTechnique::GetRTTI", 0x007D1060, NiVSMShadowTechnique_GetRTTI);
NIVSM_REVERSED("NiVSMShadowTechnique::~NiVSMShadowTechnique", 0x007D1280, NiVSMShadowTechnique_Destruct);
NIVSM_REVERSED("NiVSMShadowTechnique::UpdateBlurResources", 0x007D1380, NiVSMShadowTechnique_UpdateBlurResources);
NIVSM_REVERSED("NiVSMShadowTechnique::DeletingDestructor", 0x007D1580, NiVSMShadowTechnique_DeletingDestructor);
NIVSM_REVERSED("NiVSMShadowTechnique::AppendPostProcessRenderClicks", 0x007D15A0, NiVSMShadowTechnique_AppendPostProcessRenderClicks);

} // namespace ni
