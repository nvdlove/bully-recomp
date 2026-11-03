// 0x515610 to 0x515A60: MdBullyCullingProcess, the main view's culler (an
// NiCullingProcess): the scene walked against the camera's six frustum planes, visible
// geometry appended to the MdBullyAccumulator, and small geometry right at the near
// plane put on a separate list instead (the main view click draws it with a short
// frustum). Objects are filtered by their +0xB0 flags (4 / 8: mirror only / not in the
// mirror).

#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_FN(void, __thiscall, NiCullingProcess_Construct, 0x00776A30, void* self, void* visibleSet);
GAME_FN(void, __thiscall, NiCullingProcess_SetFrustum, 0x007769F0, void* self, const void* frustum);
GAME_FN(void, __thiscall, NiObject_Destruct, 0x007769D0, void* self);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(int, __thiscall, NiBound_WhichSide, 0x0040DA20, const void* bound, const void* plane);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rttiInfo, void* object);
GAME_FN(uint32_t, __thiscall, NiTArray_Add, 0x004099B0, void* self, const re::Ptr32<uint8_t>* item);
GAME_FN(void, __thiscall, NiTArray_SetSize, 0x0084A110, void* self, uint32_t size);
GAME_FN(void, __thiscall, NiTArray_SetAt, 0x00833C70, void* self, uint32_t index, const re::Ptr32<uint8_t>* item);
// +0xB0 flags 4 / 8: drawn only in the mirror / never in it
GAME_FN(uint8_t, __cdecl, DrawnInMirror, 0x004073B0, void* object);
GAME_FN(uint8_t, __cdecl, DrawnOutsideMirror, 0x004073D0, void* object);
GAME_FN(uint8_t, __cdecl, IsMirrorPass_Game, 0x00515A60);
GAME_FN(void, __thiscall, AddVisible_Game, 0x00515810, void* accumulator, uint8_t* geometry);
GAME_FN(void, __thiscall, CullTree_Game, 0x005158A0, void* self, uint8_t* object, void* accumulator,
        void* nearList);

inline void* const NiNode_RTTI = reinterpret_cast<void*>(0x020E2548);
inline void* const NiGeometry_RTTI = reinterpret_cast<void*>(0x020E25B4);
GAME_VAR(uint8_t, ms_bMirrorPass, 0x00C675F0);

constexpr uint32_t kMdBullyCullingProcessVtable = 0x0091758C;

struct MdBullyCullingProcess {
    uint32_t vtable;            // +0
    int32_t refCount;           // +4
    uint8_t unknown8[8];        // +8 (+0xC: the visible set)
    re::Ptr32<uint8_t> camera;            // +0x10
    float frustum[7];           // +0x14 NiFrustum
    float planes[6][4];         // +0x30 NiFrustumPlanes: normal, constant
    uint32_t activePlanes;      // +0x90
};
static_assert(sizeof(MdBullyCullingProcess) == 0x94);
static_assert(offsetof(MdBullyCullingProcess, planes) == 0x30);
static_assert(offsetof(MdBullyCullingProcess, activePlanes) == 0x90);

uint16_t Flags(const uint8_t* object) {
    return *reinterpret_cast<const uint16_t*>(object + 0xA8);
}

// NiAVObject's world bound: centre +0x1C, radius +0x28. The distance of the centre in
// front of a plane (x87: the sum rounded to a float, then the difference)
float PlaneDistance(const uint8_t* object, const float* plane) {
    const float* centre = reinterpret_cast<const float*>(object + 0x1C);
    float dot = float(double(plane[1]) * centre[1] + double(plane[0]) * centre[0] + double(plane[2]) * centre[2]);
    return float(double(dot) - plane[3]);
}
float Radius(const uint8_t* object) {
    return *reinterpret_cast<const float*>(object + 0x28);
}

} // namespace

namespace MdBullyCulling {

// 0x515A60: in the mirror's pass (set around the mirror's render)
static uint8_t __cdecl IsMirrorPass() {
    return ms_bMirrorPass;
}

// 0x515660 (object in esi, the planes on the stack; cdecl otherwise): drawn in this
// pass (its +0xB0 flags) and not wholly behind one of the six planes (with a margin of
// 1 on the radius)
static uint8_t IsVisible(uint8_t* object, const float* planes) {
    if (!object || (Flags(object) & 1)) {
        return 0;
    }
    if (IsMirrorPass_Game()) {
        if (!DrawnInMirror(object)) {
            return 0;
        }
    } else if (!DrawnOutsideMirror(object)) {
        return 0;
    }
    double margin = -(double(Radius(object)) + 1.0);
    for (int i = 0; i < 6; i++) {
        if (PlaneDistance(object, planes + i * 4) < margin) {
            return 0;
        }
    }
    return 1;
}
RE_UNHOOKED_FN("MdBullyCullingProcess::IsVisible", 0x00515660, &IsVisible);

// 0x5156F0 (object in ecx, the plane in eax): flagged 0x4000, small (radius under 2)
// and within 2.25 in front of the (near) plane
static uint8_t IsAtNearPlane(uint8_t* object, const float* plane) {
    if (!object || (Flags(object) & 1) || !(Flags(object) & 0x4000)) {
        return 0;
    }
    float distance = PlaneDistance(object, plane);
    double margin = -(double(Radius(object)) + 1.0);
    if (margin > distance) {
        return 0;
    }
    if (!(2.0f > Radius(object))) {
        return 0;
    }
    return distance < 2.25f ? 1 : 0;
}
RE_UNHOOKED_FN("MdBullyCullingProcess::IsAtNearPlane", 0x005156F0, &IsAtNearPlane);

// 0x515850 (cdecl, with the planes in edi and the near list in ebx): one geometry:
// onto the near list or appended to the accumulator
static void CullGeometry(re::Ptr32<uint8_t> geometry, void* accumulator, const float* planes, void* nearList) {
    if (!geometry || (Flags(geometry) & 1)) {
        return;
    }
    if (!IsVisible(geometry, planes)) {
        return;
    }
    if (nearList && IsAtNearPlane(geometry, planes)) {
        NiTArray_Add(nearList, &geometry);
        return;
    }
    AddVisible_Game(accumulator, geometry);
}
RE_UNHOOKED_FN("MdBullyCullingProcess::CullGeometry", 0x00515850, &CullGeometry);

// 0x515610 (thiscall): the constructor
static MdBullyCullingProcess* __fastcall Construct(MdBullyCullingProcess* self, re::Edx, void* visibleSet) {
    NiCullingProcess_Construct(self, visibleSet);
    self->vtable = kMdBullyCullingProcessVtable;
    return self;
}

// 0x515630 (thiscall): the deleting destructor
static void* __fastcall Destroy(MdBullyCullingProcess* self, re::Edx, int flags) {
    self->vtable = kMdBullyCullingProcessVtable;
    NiObject_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x94);
    }
    return self;
}

// 0x515770 (thiscall) vf2: Process (NiCullingProcess's): the object's OnVisible
// (vtable[32]) unless its bound is wholly outside an active plane; planes it's wholly
// inside are dropped for the call (the mask is restored after)
static void __fastcall Process(MdBullyCullingProcess* self, re::Edx, uint8_t* object) {
    uint32_t saved = self->activePlanes;
    if (!saved) {
        vc::Call<void>(object, 0x80, self);
        return;
    }
    uint32_t i = 0;
    for (; i < 6; i++) {
        uint32_t bit = 1u << i;
        if (self->activePlanes & bit) {
            int side = NiBound_WhichSide(object + 0x1C, self->planes[i]);
            if (side == 2) {
                break;
            }
            if (side == 1) {
                self->activePlanes &= ~bit;
            }
        }
    }
    if (i == 6) {
        vc::Call<void>(object, 0x80, self);
    }
    self->activePlanes = saved;
}
// 0x515796: the loop above (Ghidra split the function at the branch target)
RE_UNHOOKED("MdBullyCullingProcess::Process_515796", 0x00515796);

// 0x515810 (thiscall, on the accumulator): the geometry appended to its array (+0xC)
static void __fastcall AddVisible(uint8_t* accumulator, re::Edx, re::Ptr32<uint8_t> geometry) {
    uint8_t* array = accumulator + 0xC;
    uint16_t maxSize = *reinterpret_cast<uint16_t*>(array + 8);
    uint32_t index = *reinterpret_cast<uint16_t*>(array + 0xA);
    if (index >= maxSize) {
        NiTArray_SetSize(array, *reinterpret_cast<uint16_t*>(array + 0xE) + index);
    }
    NiTArray_SetAt(array, index, &geometry);
}

// 0x5158A0 (thiscall): a subtree: objects with their own geometry list (+0xB8, count
// +0xBE) are tested once with their bound and their geometry taken together; else
// geometry is culled alone and nodes recursed into
static void __fastcall CullTree(MdBullyCullingProcess* self, re::Edx, uint8_t* object, void* accumulator,
                                void* nearList) {
    if (!object || (Flags(object) & 1)) {
        return;
    }
    const float* planes = self->planes[0];
    uint16_t& listCount = *reinterpret_cast<uint16_t*>(object + 0xBE);
    re::Ptr32<uint8_t>* list = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(object + 0xB8);
    if (listCount > 0) {
        if (!IsVisible(object, planes)) {
            return;
        }
        if (nearList && IsAtNearPlane(object, planes)) {
            for (uint32_t i = 0; i < listCount; i++) {
                NiTArray_Add(nearList, &(*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(object + 0xB8))[i]);
            }
            return;
        }
        for (uint32_t i = 0; i < listCount; i++) {
            list = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(object + 0xB8);
            CullGeometry(list[i], accumulator, planes, nearList);
        }
        return;
    }
    if (uint8_t* geometry = IsKindOf(NiGeometry_RTTI, object)) {
        CullGeometry(geometry, accumulator, planes, nearList);
        return;
    }
    uint8_t* node = IsKindOf(NiNode_RTTI, object);
    if (!node) {
        return;
    }
    // NiNode's children: data +0xC8, count +0xCE
    for (uint32_t i = 0; i < *reinterpret_cast<uint16_t*>(node + 0xCE); i++) {
        uint8_t* child = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(node + 0xC8))[i];
        CullTree_Game(self, child, accumulator, nearList);
    }
}

// 0x515A10 (thiscall): Cull: the camera's frustum set, then the scene walked (nothing
// without a camera, scene or accumulator)
static void __fastcall Cull(MdBullyCullingProcess* self, re::Edx, uint8_t* camera, uint8_t* scene, void* accumulator,
                            void* nearList) {
    if (!camera || !scene || !accumulator) {
        return;
    }
    self->camera = camera;
    NiCullingProcess_SetFrustum(self, camera + 0x104);
    CullTree_Game(self, scene, accumulator, nearList);
}

// 0x515A50 (3 bytes: fldz; ret): a virtual returning 0.0f (SoundThing's vf8 and others)
[[maybe_unused]] static float __fastcall ReturnZero(void*, re::Edx) {
    return 0.0f;
}
RE_UNHOOKED_FN("SoundThing::vf8_ReturnZero", 0x00515A50, &ReturnZero);

} // namespace MdBullyCulling

// ---- registration ----------------------------------------------------------------------

#define C16B_CULL_REVERSED(name, fn, address)                                                        \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace MdBullyCulling {

C16B_CULL_REVERSED("MdBullyCullingProcess::Construct", Construct, 0x00515610);
C16B_CULL_REVERSED("MdBullyCullingProcess::Destroy", Destroy, 0x00515630);
C16B_CULL_REVERSED("MdBullyCullingProcess::Process", Process, 0x00515770);
C16B_CULL_REVERSED("MdBullyAccumulator::AddVisible", AddVisible, 0x00515810);
C16B_CULL_REVERSED("MdBullyCullingProcess::CullTree", CullTree, 0x005158A0);
C16B_CULL_REVERSED("MdBullyCullingProcess::Cull", Cull, 0x00515A10);
C16B_CULL_REVERSED("MdBullyCullingProcess::IsMirrorPass", IsMirrorPass, 0x00515A60);

} // namespace MdBullyCulling
