// 0x515A70 to 0x516330: Gamebryo's NiRenderClick (constructor, destructor, RTTI) and
// Bully's MdBullyMainViewRenderClick, the click that draws the world each frame: the
// sky and clouds, then the scene through the culler and the accumulator (once more
// from the mirror's camera when there is a mirror), the "render last" and "render
// close" layers, and the geometry right at the near plane with a short frustum.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"
#include "runflags.h"

namespace {

GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_VAR(char*, ms_renderClickDefaultName, 0x020E2DB8);
inline void* const ms_numRefObjects = reinterpret_cast<void*>(0x020E2168);
GAME_VAR(uint8_t, ms_bClearToWhite_CE754D, 0x00CE754D);
GAME_ARRAY(uint8_t, ms_timecycColours, 0x020C47F8);   // two RGB colours: the sky's top and bottom
GAME_VAR(uint8_t, ms_bSkipMainView_20C823C, 0x020C823C);
GAME_VAR(uint8_t, ms_bMirrorPass, 0x00C675F0);
GAME_VAR(uint8_t, ms_bInMirror_BC74BC, 0x00BC74BC);
GAME_VAR(uint32_t, ms_accumulatorClassIndex, 0x00C675C4);   // MdBullyAccumulator's bit in the RTTI's class set

GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __thiscall, NiRenderClick_Construct, 0x00515C00, void* self);
GAME_FN(void, __thiscall, NiRenderClick_Destruct, 0x00515CA0, void* self);
GAME_FN(void, __thiscall, MainViewClick_Destruct, 0x00515D20, void* self);
GAME_FN(void, __thiscall, MainViewClick_RenderBackground, 0x00515E20, void* self, void* camera);
GAME_FN(void, __thiscall, MainViewClick_RenderScene, 0x00515F50, void* self, void* camera, int mirrorPass);
GAME_FN(void, __thiscall, Set68_Realloc, 0x00776B50, void* self, uint32_t size);
GAME_FN(void, __thiscall, NiCamera_SetViewFrustum, 0x00762D60, void* camera, const float* frustum);
GAME_FN(void, __thiscall, MdBullyCullingProcess_Cull, 0x00515A10, void* self, void* camera, void* scene,
        void* accumulator, void* nearList);
GAME_FN(uint8_t, __thiscall, MdBullyMirror_IsEnabled, 0x00518710, void* mirror);
GAME_FN(uint8_t*, __thiscall, MdBullyMirror_GetCamera, 0x005187F0, void* mirror, void* camera);
GAME_FN(uint8_t, __cdecl, Fn_538C70, 0x00538C70);
GAME_FN(void, __cdecl, Fn_5EE1D0, 0x005EE1D0);
GAME_FN(void, __cdecl, Fn_407550, 0x00407550, void* camera);   // the camera made the renderer's
GAME_FN(void, __cdecl, Fn_409960, 0x00409960);
GAME_FN(void, __cdecl, SetAlphaToCoverage, 0x00407460, int on);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, RenderTree, 0x00408410, void* node);
GAME_FN(void, __cdecl, RenderChildrenLastFlagged, 0x004084E0, void* node);
GAME_FN(void, __cdecl, Clouds_RenderBackground, 0x0050FAE0, int topR, int topG, int topB, int bottomR,
        int bottomG, int bottomB, int alpha);
GAME_FN(void, __cdecl, Clouds_RenderSkyDomes, 0x005104C0, void* camera);
GAME_FN(void, __cdecl, Clouds_RenderMoonAndStars, 0x0050F320);
GAME_FN(void, __cdecl, Clouds_RenderHorizon, 0x0050FFD0);

constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uint32_t kNiRenderClickVtable = 0x009175A4;
constexpr uint32_t kMainViewClickVtable = 0x009175C4;
constexpr uint32_t kNiTArrayGeometryVtable = 0x0091750C;
constexpr uint32_t kNiTPrimitiveArrayGeometryVtable = 0x0091756C;

struct NiRenderClick {
    uint32_t vtable;       // +0
    int32_t refCount;      // +4
    uint16_t flags;        // +8
    uint16_t unknownA;     // +0xA
    re::Ptr32<uint8_t> object0C;     // +0xC NiPointer
    re::Ptr32<uint8_t> object10;     // +0x10 NiPointer
    float viewport[4];     // +0x14 left, right, top, bottom
    uint32_t unknown24[5]; // +0x24
    float unknown38[4];    // +0x38
    re::Ptr32<char> name;            // +0x48 NiFixedString
};
static_assert(sizeof(NiRenderClick) == 0x4C);

// NiTPrimitiveArray<NiGeometry*>
struct GeometryArray {
    uint32_t vtable;        // +0
    re::Ptr32<re::Ptr32<uint8_t>> data;         // +4
    uint16_t maxSize;       // +8
    uint16_t size;          // +0xA
    uint16_t effectiveSize; // +0xC
    uint16_t growBy;        // +0xE
};
static_assert(sizeof(GeometryArray) == 0x10);

struct MainViewClick : NiRenderClick {
    re::Ptr32<uint8_t> culler;         // +0x4C MdBullyCullingProcess
    re::Ptr32<uint8_t> camera;         // +0x50
    re::Ptr32<uint8_t> clouds;         // +0x54 (drawn before the scene)
    re::Ptr32<uint8_t> scene;          // +0x58
    re::Ptr32<uint8_t> renderLast;     // +0x5C
    re::Ptr32<uint8_t> renderClose;    // +0x60 (drawn with the short frustum)
    re::Ptr32<uint8_t> mirror;         // +0x64 MdBullyMirror
    uint32_t set68[4];       // +0x68 data, ?, ?, grow by (0x10)
    GeometryArray nearList;  // +0x78 geometry right at the near plane
};
static_assert(sizeof(MainViewClick) == 0x88);
static_assert(offsetof(MainViewClick, culler) == 0x4C);
static_assert(offsetof(MainViewClick, nearList) == 0x78);

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

// NiPointer assignment
void Assign(re::Ptr32<uint8_t>& slot, uint8_t* object) {
    if (slot == object) {
        return;
    }
    Release(slot);
    slot = object;
    if (object) {
        vc::Increment(object + 4);
    }
}

void ReleaseString(re::Ptr32<char>* handle) {
    char* s = *handle;
    if (s == ms_nullString) {
        return;
    }
    uint16_t length = *reinterpret_cast<uint16_t*>(s - 2);
    if (vc::Decrement(s - 8) == 1) {
        StringTable_Release(ms_pStringTable, handle, length);
    }
}

float& MinNearPlane(uint8_t* camera) {
    return *reinterpret_cast<float*>(camera + 0x120);   // NiCamera's, after the frustum
}

} // namespace

namespace MdBullyRenderClick {

// 0x515A70, 0x515AC0, 0x515B10, 0x515B60, 0x515BB0 (thiscall): the NiPointer setters
static void __fastcall SetCamera(MainViewClick* self, re::Edx, uint8_t* camera) {
    Assign(self->camera, camera);
}
static void __fastcall SetClouds(MainViewClick* self, re::Edx, uint8_t* node) {
    Assign(self->clouds, node);
}
static void __fastcall SetRenderLast(MainViewClick* self, re::Edx, uint8_t* node) {
    Assign(self->renderLast, node);
}
static void __fastcall SetRenderClose(MainViewClick* self, re::Edx, uint8_t* node) {
    Assign(self->renderClose, node);
}
static void __fastcall SetMirror(MainViewClick* self, re::Edx, uint8_t* mirror) {
    Assign(self->mirror, mirror);
}

// 0x515C00 (thiscall): NiRenderClick's constructor (NiRefObject's inlined). Flags +8
// get bits 1 and 2 set and bit 4 cleared, the rest left as they were (never set
// before: uninitialised)
static NiRenderClick* __fastcall RenderClick_Construct(NiRenderClick* self, re::Edx) {
    self->vtable = kNiRefObjectVtable;
    self->refCount = 0;
    vc::Increment(ms_numRefObjects);
    self->vtable = kNiRenderClickVtable;
    self->object0C = nullptr;
    self->object10 = nullptr;
    self->viewport[0] = 0.0f;
    self->viewport[1] = 1.0f;
    self->viewport[2] = 1.0f;
    self->viewport[3] = 0.0f;
    for (uint32_t& u : self->unknown24) {
        u = 0;
    }
    for (float& f : self->unknown38) {
        f = 0.0f;
    }
    char* name = ms_renderClickDefaultName;
    if (name != ms_nullString) {
        vc::Increment(name - 8);
        name = ms_renderClickDefaultName;
    }
    self->name = name;
    self->flags = uint16_t((self->flags & 0xFFFB) | 3);
    return self;
}

// 0x515C90 (thiscall): NiRenderClick::GetRTTI
static void* __fastcall RenderClick_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E2D84);
}

// 0x515CA0 (thiscall): NiRenderClick's destructor body
static void __fastcall RenderClick_Destruct(NiRenderClick* self, re::Edx) {
    ReleaseString(&self->name);
    Release(self->object10);
    Release(self->object0C);
    self->vtable = kNiRefObjectVtable;
    vc::Decrement(ms_numRefObjects);
}

// 0x515D20 (thiscall): the destructor body
static void __fastcall Destruct(MainViewClick* self, re::Edx) {
    self->vtable = kMainViewClickVtable;
    self->nearList.vtable = kNiTArrayGeometryVtable;
    NiFree(self->nearList.data);
    NiFree(reinterpret_cast<void*>(self->set68[0]));
    Release(self->mirror);
    Release(self->renderClose);
    Release(self->renderLast);
    Release(self->scene);
    Release(self->clouds);
    Release(self->camera);
    Release(self->culler);
    NiRenderClick_Destruct(self);
}

// 0x515E10 (thiscall): GetRTTI
static void* __fastcall GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x00C675F4);
}

// 0x515E20 (thiscall, `this` unused): the sky: the background gradient in the
// timecyc's colours (white with 0xCE754D set and 0x538C70 false), the sky domes, the
// moon and stars and the horizon
static void __fastcall RenderBackground(MainViewClick*, re::Edx, uint8_t* camera) {
    int c[6];
    if (ms_bClearToWhite_CE754D && !Fn_538C70()) {
        for (int& v : c) {
            v = 0xFF;
        }
    } else {
        for (int i = 0; i < 6; i++) {
            c[i] = ms_timecycColours[i];
        }
    }
    Fn_5EE1D0();
    if (ms_pRenderer) {
        vc::Call<void*>(ms_pRenderer, 0x08);   // (GetRTTI, result unused)
    }
    SetAlphaToCoverage(0);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0x14, 1);
    Clouds_RenderBackground(c[0], c[1], c[2], c[3], c[4], c[5], 0xFF);
    Clouds_RenderSkyDomes(camera);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0xE, 1);
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
    Clouds_RenderMoonAndStars();
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    Clouds_RenderHorizon();
}

// 0x515F50 (thiscall): the scene from `camera`: the clouds layer, the scene culled
// into the renderer's sorter (+8; used as the accumulator when it's an
// MdBullyAccumulator) between its StartAccumulating and FinishAccumulating (vtable[18],
// [19]), the "render last" layer, then the "render close" one with the near and far
// planes at 0.0005 and 10. The near list only outside the mirror pass
static void __fastcall RenderScene(MainViewClick* self, re::Edx, uint8_t* camera, int mirrorPass) {
    Fn_407550(camera);
    RenderTree(self->clouds);
    uint8_t* sorter = *reinterpret_cast<re::Ptr32<uint8_t>*>(ms_pRenderer + 8);
    uint8_t* culler = self->culler;
    uint8_t* accumulator = nullptr;
    if (sorter) {
        const uint8_t* rtti = vc::Call<const uint8_t*>(sorter, 0x08);
        uint32_t index = ms_accumulatorClassIndex;
        uint32_t bits = *reinterpret_cast<const uint32_t*>(rtti + 0xC + (index >> 5) * 4);
        accumulator = (bits & (1u << (index & 0x1F))) ? sorter : nullptr;
    }
    // (no sorter: the game calls through null here)
    vc::Call<void>(sorter, 0x48, camera);
    void* nearList = re::Flag(mirrorPass) ? nullptr : &self->nearList;
    MdBullyCullingProcess_Cull(culler, camera, self->scene, accumulator, nearList);
    vc::Call<void>(sorter, 0x4C);
    RenderChildrenLastFlagged(self->renderLast);
    if (!re::Flag(mirrorPass)) {
        re::SeamlessDoorsDraw(culler, self->scene, sorter, accumulator);   // (the remaster's: an interior seen through its door, runflags.h)
    }

    float frustum[7];
    std::memcpy(frustum, camera + 0x104, sizeof(frustum));
    float savedNear = frustum[4];
    float savedFar = frustum[5];
    float savedMinNear = MinNearPlane(camera);
    frustum[4] = 0.0005f;
    frustum[5] = 10.0f;
    MinNearPlane(camera) = 0.0005f;
    NiCamera_SetViewFrustum(camera, frustum);
    Fn_407550(camera);
    RenderTree(self->renderClose);
    frustum[4] = savedNear;
    frustum[5] = savedFar;
    MinNearPlane(camera) = savedMinNear;
    NiCamera_SetViewFrustum(camera, frustum);
    Fn_407550(camera);
}

// 0x516090 (thiscall) vf6: Render (the frame id unused): the sky, the mirror's pass
// when the mirror is on (then renderer vtable[85](0, 4) for the renderer's modes 1, 2
// and 4), the near list emptied, the main pass
static void __fastcall Render(MainViewClick* self, re::Edx, uint32_t) {
    if (ms_bSkipMainView_20C823C) {
        return;
    }
    MainViewClick_RenderBackground(self, self->camera);
    if (MdBullyMirror_IsEnabled(self->mirror)) {
        ms_bMirrorPass = 1;
        ms_bInMirror_BC74BC = 1;
        uint8_t* mirrorCamera = MdBullyMirror_GetCamera(self->mirror, self->camera);
        MainViewClick_RenderScene(self, mirrorCamera, 1);
        uint8_t* renderer = ms_pRenderer;
        int32_t mode = *reinterpret_cast<int32_t*>(renderer + 0x94);
        if ((mode == 1 || mode == 2 || mode == 4) && (renderer[0x90] == 1 || mode == 4)) {
            vc::Call<void>(renderer, 0x154, 0, 4);
        }
        ms_bMirrorPass = 0;
    }
    ms_bInMirror_BC74BC = 0;
    for (uint16_t i = 0; i < self->nearList.size; i++) {
        self->nearList.data[i] = nullptr;
    }
    self->nearList.size = 0;
    self->nearList.effectiveSize = 0;
    MainViewClick_RenderScene(self, self->camera, 0);
}

// 0x516170 (thiscall): the near list drawn (RenderImmediate) with the near and far
// planes at 0.05 and 10, then the render states back
static void __fastcall RenderNearList(MainViewClick* self, re::Edx) {
    uint8_t* camera = self->camera;
    float frustum[7];
    std::memcpy(frustum, camera + 0x104, sizeof(frustum));
    float savedNear = frustum[4];
    float savedFar = frustum[5];
    float savedMinNear = MinNearPlane(camera);
    frustum[4] = 0.05f;
    frustum[5] = 10.0f;
    MinNearPlane(camera) = 0.05f;
    NiCamera_SetViewFrustum(self->camera, frustum);
    Fn_5EE1D0();
    SetRenderState(6, 1);
    SetRenderState(8, 1);
    Fn_407550(self->camera);
    uint8_t* renderer = ms_pRenderer;
    for (uint32_t i = 0; i < self->nearList.size; i++) {
        uint8_t* geometry = self->nearList.data[i];
        if (geometry) {
            vc::Call<void>(geometry, 0x88, renderer);
        }
    }
    frustum[4] = savedNear;
    frustum[5] = savedFar;
    MinNearPlane(self->camera) = savedMinNear;
    NiCamera_SetViewFrustum(self->camera, frustum);
    Fn_409960();
    Fn_5EE1D0();
    if (ms_pRenderer) {
        vc::Call<void*>(ms_pRenderer, 0x08);   // (GetRTTI, result unused)
    }
    SetAlphaToCoverage(0);
    SetRenderState(6, 0);
    SetRenderState(8, 0);
    SetRenderState(0x14, 1);
}

// 0x5162A0 (thiscall): NiRenderClick's deleting destructor
static void* __fastcall RenderClick_Destroy(NiRenderClick* self, re::Edx, int flags) {
    NiRenderClick_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x4C);
    }
    return self;
}

// 0x5162C0 (thiscall): the constructor
static MainViewClick* __fastcall Construct(MainViewClick* self, re::Edx) {
    NiRenderClick_Construct(self);
    self->vtable = kMainViewClickVtable;
    self->culler = nullptr;
    self->camera = nullptr;
    self->clouds = nullptr;
    self->scene = nullptr;
    self->renderLast = nullptr;
    self->renderClose = nullptr;
    self->mirror = nullptr;
    self->set68[0] = 0;
    self->set68[1] = 0;
    self->set68[2] = 0;
    self->set68[3] = 0x10;
    Set68_Realloc(self->set68, 0x10);
    self->nearList.maxSize = 0;
    self->nearList.size = 0;
    self->nearList.effectiveSize = 0;
    self->nearList.data = nullptr;
    self->nearList.growBy = 1;
    self->nearList.vtable = kNiTPrimitiveArrayGeometryVtable;
    return self;
}

// 0x516330 (thiscall): the deleting destructor
static void* __fastcall Destroy(MainViewClick* self, re::Edx, int flags) {
    MainViewClick_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x88);
    }
    return self;
}

} // namespace MdBullyRenderClick

// ---- registration ----------------------------------------------------------------------

#define C16B_CLICK_REVERSED(name, fn, address)                                                       \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace MdBullyRenderClick {

C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::SetCamera", SetCamera, 0x00515A70);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::SetClouds", SetClouds, 0x00515AC0);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::SetRenderLast", SetRenderLast, 0x00515B10);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::SetRenderClose", SetRenderClose, 0x00515B60);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::SetMirror", SetMirror, 0x00515BB0);
C16B_CLICK_REVERSED("NiRenderClick::Construct", RenderClick_Construct, 0x00515C00);
C16B_CLICK_REVERSED("NiRenderClick::GetRTTI", RenderClick_GetRTTI, 0x00515C90);
C16B_CLICK_REVERSED("NiRenderClick::Destruct", RenderClick_Destruct, 0x00515CA0);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::Destruct", Destruct, 0x00515D20);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::GetRTTI", GetRTTI, 0x00515E10);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::RenderBackground", RenderBackground, 0x00515E20);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::RenderScene", RenderScene, 0x00515F50);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::Render", Render, 0x00516090);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::RenderNearList", RenderNearList, 0x00516170);
C16B_CLICK_REVERSED("NiRenderClick::Destroy", RenderClick_Destroy, 0x005162A0);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::Construct", Construct, 0x005162C0);
C16B_CLICK_REVERSED("MdBullyMainViewRenderClick::Destroy", Destroy, 0x00516330);

} // namespace MdBullyRenderClick
