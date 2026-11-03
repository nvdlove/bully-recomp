// 0x518C00 to 0x518E30: Gamebryo's NiViewRenderClick and NiShadowRenderClick
// (constructors, destructors, RTTI), Bully's MdBullyShadowRenderClick (its Render
// sets the "in mirror" flag from the shadow generator while it draws) and the factory
// the shadow manager makes them with.

#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_VAR(uint8_t, ms_bSkipMainView_20C823C, 0x020C823C);
GAME_VAR(uint8_t, ms_bInMirror_BC74BC, 0x00BC74BC);

GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __thiscall, NiRenderClick_Construct, 0x00515C00, void* self);
GAME_FN(void, __thiscall, NiRenderClick_Destruct, 0x00515CA0, void* self);
GAME_FN(void, __thiscall, NiViewRenderClick_Render, 0x00776D80, void* self, uint32_t frameId);
GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, NiTArray_SetSize_776B50, 0x00776B50, void* array, uint32_t size);
GAME_FN(void, __thiscall, NiViewRenderClick_Construct, 0x00518D00, void* self);
GAME_FN(void, __thiscall, NiViewRenderClick_Destruct, 0x00518C70, void* self);
GAME_FN(void, __thiscall, NiShadowRenderClick_Construct, 0x00518D90, void* self);

constexpr uint32_t kNiTListBaseVtable = 0x009178F4;
constexpr uint32_t kNiTPointerListBaseVtable = 0x00917904;
constexpr uint32_t kNiTPointerListVtable = 0x00917934;
constexpr uint32_t kViewRenderClickVtable = 0x00917944;
constexpr uint32_t kShadowRenderClickVtable = 0x00917914;
constexpr uint32_t kMdShadowRenderClickVtable = 0x00917964;

// NiTPointerList<NiPointer<NiRenderView>>
struct ViewList {
    uint32_t vtable;  // +0
    re::Ptr32<void> head;       // +4
    re::Ptr32<void> tail;       // +8
    uint32_t count;   // +0xC
};
static_assert(sizeof(ViewList) == 0x10);

struct ViewRenderClick {
    uint32_t vtable;         // +0 (NiRenderClick up to +0x4C)
    int32_t refCount;        // +4
    uint16_t flags;          // +8
    uint16_t unknownA;       // +0xA
    uint8_t unknownC[0x2C];  // +0xC
    float unknown38[4];      // +0x38
    re::Ptr32<char> name;              // +0x48
    ViewList views;          // +0x4C
    re::Ptr32<uint8_t> processor;      // +0x5C NiPointer
    uint32_t unknown60;      // +0x60
    uint32_t array64[4];     // +0x64 data, ?, ?, grow by (4)
    uint32_t unknown74;      // +0x74
    float unknown78;         // +0x78
    float unknown7C;         // +0x7C
};
static_assert(sizeof(ViewRenderClick) == 0x80);
static_assert(offsetof(ViewRenderClick, views) == 0x4C);

struct ShadowRenderClick : ViewRenderClick {
    re::Ptr32<uint8_t> generator;   // +0x80 NiShadowGenerator
    uint8_t flags84;      // +0x84
    uint8_t unknown85[3];
};
static_assert(sizeof(ShadowRenderClick) == 0x88);

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

} // namespace

namespace NiShadowRenderClicks {

// 0x518C00 (thiscall): NiTListBase<..NiPointer<NiRenderView>>'s deleting destructor
static void* __fastcall ListBase_Destroy(ViewList* self, re::Edx, int flags) {
    self->vtable = kNiTListBaseVtable;
    if (flags & 1) {
        NiFreeSized(self, sizeof(ViewList));
    }
    return self;
}

// 0x518C30 (thiscall) vf6: MdBullyShadowRenderClick::Render: drawn with 0xBC74BC set
// from the generator's flag bit 4, then bit 4 of +0x84 cleared
static void __fastcall MdRender(ShadowRenderClick* self, re::Edx, uint32_t frameId) {
    if (ms_bSkipMainView_20C823C) {
        return;
    }
    ms_bInMirror_BC74BC = (self->generator[8] >> 4) & 1;
    NiViewRenderClick_Render(self, frameId);
    ms_bInMirror_BC74BC = 0;
    self->flags84 &= 0xEF;
}

// 0x518C70 (thiscall): NiViewRenderClick's destructor body
static void __fastcall ViewClick_Destruct(ViewRenderClick* self, re::Edx) {
    NiFree(reinterpret_cast<void*>(self->array64[0]));
    Release(self->processor);
    self->views.vtable = kNiTPointerListBaseVtable;
    NiTPointerList_RemoveAll(&self->views);
    self->views.vtable = kNiTListBaseVtable;
    NiRenderClick_Destruct(self);
}

// 0x518CC0 (thiscall): NiShadowRenderClick::GetRTTI
static void* __fastcall ShadowClick_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E2E90);
}

// 0x518CD0 (thiscall): NiTPointerListBase's deleting destructor
static void* __fastcall PointerListBase_Destroy(ViewList* self, re::Edx, int flags) {
    self->vtable = kNiTPointerListBaseVtable;
    NiTPointerList_RemoveAll(self);
    self->vtable = kNiTListBaseVtable;
    if (flags & 1) {
        NiFreeSized(self, sizeof(ViewList));
    }
    return self;
}

// 0x518D00 (thiscall): NiViewRenderClick's constructor
static ViewRenderClick* __fastcall ViewClick_Construct(ViewRenderClick* self, re::Edx) {
    NiRenderClick_Construct(self);
    self->vtable = kViewRenderClickVtable;
    self->views.count = 0;
    self->views.head = nullptr;
    self->views.tail = nullptr;
    self->views.vtable = kNiTPointerListVtable;
    self->processor = nullptr;
    self->unknown60 = 0;
    self->array64[0] = 0;
    self->array64[1] = 0;
    self->array64[2] = 0;
    self->array64[3] = 4;
    NiTArray_SetSize_776B50(self->array64, 0);
    self->unknown74 = 0;
    self->unknown78 = 0.0f;
    self->unknown7C = 0.0f;
    return self;
}

// 0x518D50 (thiscall): NiViewRenderClick::GetRTTI
static void* __fastcall ViewClick_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E2DBC);
}

// 0x518D60 (thiscall): NiViewRenderClick's deleting destructor
static void* __fastcall ViewClick_Destroy(ViewRenderClick* self, re::Edx, int flags) {
    NiViewRenderClick_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, sizeof(ViewRenderClick));
    }
    return self;
}

// 0x518D90 (thiscall): NiShadowRenderClick's constructor (flag bit 1 cleared, the
// colour at +0x38 white)
static ShadowRenderClick* __fastcall ShadowClick_Construct(ShadowRenderClick* self, re::Edx) {
    NiViewRenderClick_Construct(self);
    self->flags &= 0xFFFD;
    for (float& f : self->unknown38) {
        f = 1.0f;
    }
    self->vtable = kShadowRenderClickVtable;
    self->generator = nullptr;
    self->flags84 = 0x10;
    return self;
}

// 0x518DF0 (thiscall): MdBullyShadowRenderClick::GetRTTI
static void* __fastcall MdShadowClick_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x00C676D8);
}

// 0x518E00 (thiscall): NiShadowRenderClick's deleting destructor
static void* __fastcall ShadowClick_Destroy(ShadowRenderClick* self, re::Edx, int flags) {
    self->vtable = kShadowRenderClickVtable;
    NiViewRenderClick_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, sizeof(ShadowRenderClick));
    }
    return self;
}

// 0x518E30 (thiscall, `this` unused) vf0: MdShadowRenderClickFactory: a new
// MdBullyShadowRenderClick
static ShadowRenderClick* __fastcall Factory_Create(void*, re::Edx) {
    auto* click = static_cast<ShadowRenderClick*>(NiNew(sizeof(ShadowRenderClick)));
    if (!click) {
        return nullptr;
    }
    NiShadowRenderClick_Construct(click);
    click->vtable = kMdShadowRenderClickVtable;
    return click;
}

} // namespace NiShadowRenderClicks

// ---- registration ----------------------------------------------------------------------

#define C16_SHADOWCLICK_REVERSED(name, fn, address)                                                  \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace NiShadowRenderClicks {

C16_SHADOWCLICK_REVERSED("NiTListBase<NiPointer<NiRenderView>>::Destroy", ListBase_Destroy, 0x00518C00);
C16_SHADOWCLICK_REVERSED("MdBullyShadowRenderClick::Render", MdRender, 0x00518C30);
C16_SHADOWCLICK_REVERSED("NiViewRenderClick::Destruct", ViewClick_Destruct, 0x00518C70);
C16_SHADOWCLICK_REVERSED("NiShadowRenderClick::GetRTTI", ShadowClick_GetRTTI, 0x00518CC0);
C16_SHADOWCLICK_REVERSED("NiTPointerListBase<NiPointer<NiRenderView>>::Destroy", PointerListBase_Destroy,
                         0x00518CD0);
C16_SHADOWCLICK_REVERSED("NiViewRenderClick::Construct", ViewClick_Construct, 0x00518D00);
C16_SHADOWCLICK_REVERSED("NiViewRenderClick::GetRTTI", ViewClick_GetRTTI, 0x00518D50);
C16_SHADOWCLICK_REVERSED("NiViewRenderClick::Destroy", ViewClick_Destroy, 0x00518D60);
C16_SHADOWCLICK_REVERSED("NiShadowRenderClick::Construct", ShadowClick_Construct, 0x00518D90);
C16_SHADOWCLICK_REVERSED("MdBullyShadowRenderClick::GetRTTI", MdShadowClick_GetRTTI, 0x00518DF0);
C16_SHADOWCLICK_REVERSED("NiShadowRenderClick::Destroy", ShadowClick_Destroy, 0x00518E00);
C16_SHADOWCLICK_REVERSED("MdShadowRenderClickFactory::Create", Factory_Create, 0x00518E30);

} // namespace NiShadowRenderClicks
