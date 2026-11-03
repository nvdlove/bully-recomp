// CVisibilityPlugins (GTA's, on Gamebryo objects): the sorted alpha lists (CLinkList of
// {entity, distance}, far ones drawn first), fading entities by their distance to the
// camera, clump alpha and the camera variables. 0x5F2AF0..0x5F3580.
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

struct AlphaObjectInfo {
    re::Ptr32<uint8_t> entity;  // +0
    float dist;       // +4 the sort key
};
struct AlphaLink {
    AlphaObjectInfo item;  // +0
    re::Ptr32<AlphaLink> prev;       // +8
    re::Ptr32<AlphaLink> next;       // +0xC
};
static_assert(sizeof(AlphaLink) == 0x10);
// CLinkList<AlphaObjectInfo>
struct AlphaList {
    AlphaLink usedHead;   // +0x0
    AlphaLink usedTail;   // +0x10
    AlphaLink freeHead;   // +0x20
    AlphaLink freeTail;   // +0x30
    re::Ptr32<AlphaLink> links;     // +0x40
    uint32_t unknown44;
};
static_assert(sizeof(AlphaList) == 0x48);

GAME_VAR(AlphaList, ms_alphaAtomicList, 0x01B64898);      // (20)
GAME_VAR(AlphaList, ms_alphaEntityList, 0x01B648E0);      // (200)
GAME_VAR(AlphaList, ms_alphaUnderwaterList, 0x01B64928);  // (30) entities with +0xB8 set
GAME_VAR(AlphaList, ms_alphaFourthList, 0x01B64970);      // (20)
GAME_VAR(float, ms_clumpDistToCameraSq, 0x01B64870);
GAME_VAR(float, ms_cullCompsDist, 0x01B64874);
GAME_VAR(float, ms_lodDistSq, 0x01B64878);
GAME_VAR(void*, ms_pCamera, 0x01B6487C);
GAME_VAR(float*, ms_pCameraPosn, 0x01B64880);
GAME_VAR(void*, ms_pFrame, 0x01B64884);       // a frame the fading entities are drawn with
GAME_VAR(int32_t, flag_B0A4A8, 0x00B0A4A8);
GAME_VAR(int32_t, flag_B0A4AC, 0x00B0A4AC);
GAME_VAR(float, lodDistMultiplier_C3CD00, 0x00C3CD00);
GAME_VAR(int32_t, alwaysOpaqueModel_BF4488, 0x00BF4488);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
inline void* const lightAccumulator_D140B8 = reinterpret_cast<void*>(0x00D140B8);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, AlphaList_Init, 0x005F2F00, AlphaList* list, uint32_t count);
GAME_FN(AlphaLink*, __thiscall, AlphaList_InsertSorted, 0x005F2F70, AlphaList* list, AlphaObjectInfo* item);
GAME_FN(uint32_t, __cdecl, Stub_824EC0, 0x00824EC0, void*);   // (returns 0: the game's geometry getter, stubbed)
GAME_FN(void, __cdecl, RenderAtomicWithState, 0x006C6AF0, void* atomic);
GAME_FN(uint8_t*, __cdecl, AtomicGetClumpPlugin, 0x006C69E0, void* atomic);
GAME_FN(void*, __cdecl, AtomicGetFrame, 0x006C7370, void* atomic);
GAME_FN(uint8_t*, __cdecl, FrameGetLTM, 0x006CA0A0, void* frame);
GAME_FN(void*, __cdecl, CameraGetFrame, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, FrameGetMatrix, 0x006C9A90, void* frame);
GAME_FN(void*, __cdecl, RwFrameCreate, 0x006CA210);
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, RenderOneNonRoad, 0x00452B20, uint8_t* entity);
GAME_FN(float, __thiscall, MI_GetLodDistance_520320, 0x00520320, uint8_t* mi);
GAME_FN(float, __thiscall, MI_GetFadeDistance_520340, 0x00520340, uint8_t* mi);
GAME_FN(float, __thiscall, MI_GetLargestLodDistance, 0x00527420, uint8_t* mi);
GAME_FN(void*, __thiscall, MI_GetAtomicFromDistance, 0x005273E0, uint8_t* mi, float dist);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(void, __thiscall, Matrix_UpdateRwMatrix, 0x00412AC0, uint8_t* matrix, uint8_t* rwMatrix);
GAME_FN(void, __thiscall, Placeable_UpdateRwMatrix, 0x0046D8F0, uint8_t* placeable, uint8_t* rwMatrix);
GAME_FN(void, __cdecl, FrameUpdateObjects, 0x006C9AF0, void* frame);
GAME_FN(void, __cdecl, AtomicSetFrame, 0x006C6A10, void* atomic, void* frame);
GAME_FN(void, __cdecl, AtomicSetGeometry, 0x006CA7D0, void* atomic, uint32_t geometry, int flags);
GAME_FN(void, __cdecl, SetObjectWorldTransform, 0x004068E0, void* object, void* transform);
GAME_FN(void, __cdecl, RenderAtomic, 0x006C69F0, void* atomic);
GAME_FN(void, __thiscall, LightAccumulator_5EE560, 0x005EE560, void* self);

// 0x5F2B30 (esi: the atomic, alpha on the stack) called from here
__declspec(noinline) void CallRenderAtomicWithAlpha(void* atomic, int alpha) {
#if defined(_M_IX86)
    void* fn = reinterpret_cast<void*>(0x005F2B30);
    __asm {
        push esi
        mov esi, atomic
        push alpha
        call fn
        add esp, 4
        pop esi
    }
#else
    re::Ours<void (*)(void*, int32_t)>(0x005F2B30)(atomic, alpha);
#endif
}

// 0x5F2FE0 (edi: the entity, the distance on the stack) called from here
__declspec(noinline) void CallRenderFadingEntity(uint8_t* entity, float dist) {
#if defined(_M_IX86)
    void* fn = reinterpret_cast<void*>(0x005F2FE0);
    __asm {
        push edi
        mov edi, entity
        push dist
        call fn
        add esp, 4
        pop edi
    }
#else
    re::Ours<void (*)(uint8_t*, float)>(0x005F2FE0)(entity, dist);
#endif
}

// 0x5F2AF0 (cdecl): returns 1 (App.cpp's caller tests it)
int32_t __cdecl SetFlags_5F2AF0() {
    flag_B0A4A8 = 1;
    flag_B0A4AC = 1;
    return 1;
}

// 0x5F2B00 (cdecl): the clump plugin's alpha
void __cdecl SetClumpAlpha(uint8_t* plugin, int32_t alpha) {
    if (plugin) {
        vc::At<int32_t>(plugin, 0x10) = alpha;
    }
}

// 0x5F2B10 (cdecl): 0xFF without a plugin
int32_t __cdecl GetClumpAlpha(uint8_t* plugin) {
    if (!plugin) {
        return 0xFF;
    }
    return vc::At<int32_t>(plugin, 0x10);
}

// 0x5F2B60 (cdecl): a clump's atomics, each with its clump's alpha when not opaque
void __cdecl RenderFadingClump(uint8_t* clump) {
    for (uint8_t* link = vc::At<uint8_t*>(clump, 0x18); link != clump; link = vc::At<uint8_t*>(link, 8)) {
        uint8_t* atomic = vc::At<uint8_t*>(link, 0);
        uint8_t* plugin = AtomicGetClumpPlugin(atomic);
        int32_t alpha;
        if (!plugin || (alpha = vc::At<int32_t>(plugin, 0x10)) == 0xFF) {
            RenderAtomicWithState(atomic);
        } else {
            Stub_824EC0(atomic);
            Stub_824EC0(atomic);
            vc::At<int32_t>(atomic, 0x1C) = alpha;
            RenderAtomicWithState(atomic);
            Stub_824EC0(atomic);
            vc::At<int32_t>(atomic, 0x1C) = 0xFF;
        }
    }
}

// 0x5F2BD0 (cdecl): the lists' links freed
void __cdecl Shutdown() {
    Free(ms_alphaAtomicList.links);
    ms_alphaAtomicList.links = nullptr;
    Free(ms_alphaEntityList.links);
    ms_alphaEntityList.links = nullptr;
    Free(ms_alphaUnderwaterList.links);
    ms_alphaUnderwaterList.links = nullptr;
    Free(ms_alphaFourthList.links);
    ms_alphaFourthList.links = nullptr;
}

// 0x5F2C20 (cdecl): a clump's squared distance to the camera
void __cdecl SetupVehicleVariables(uint8_t* clump) {
    if (clump[0] != 2) {   // (rpCLUMP)
        return;
    }
    uint8_t* ltm = FrameGetLTM(AtomicGetFrame(clump));
    float x = vc::At<float>(ltm, 0x24);
    float y = vc::At<float>(ltm, 0x28);
    float z = vc::At<float>(ltm, 0x2C);
    float dx = x - ms_pCameraPosn[0];
    float dy = y - ms_pCameraPosn[1];
    float dz = z - ms_pCameraPosn[2];
    ms_clumpDistToCameraSq = dx * dx + dy * dy + dz * dz;
}

// ---- the atomics' render callbacks (each returns its atomic) -------------------------------

void RenderAtomicWithAlpha(void* atomic, int32_t alpha);

// the atomic drawn with its clump's alpha (as 0x5F2B30, inline), plain when opaque or without one
void RenderAtomicWithClumpAlpha(void* atomic) {
    uint8_t* plugin = AtomicGetClumpPlugin(atomic);
    int32_t alpha;
    if (plugin == nullptr || (alpha = vc::At<int32_t>(plugin, 0x10)) == 0xFF) {
        RenderAtomicWithState(atomic);
        return;
    }
    Stub_824EC0(atomic);
    Stub_824EC0(atomic);
    vc::At<int32_t>(atomic, 0x1C) = alpha;
    RenderAtomicWithState(atomic);
    Stub_824EC0(atomic);
    vc::At<int32_t>(atomic, 0x1C) = 0xFF;
}

// 0x5F2CA0: a vehicle wheel (inside the components' cull distance): the model's atomic for the
// distance (the square root of the clump's squared distance), its geometry taken when it
// differs, drawn with the clump's alpha (through 0x5F2B30)
void* __cdecl RenderWheelAtomicCB(uint8_t* atomic) {
    if (!(ms_cullCompsDist > ms_clumpDistToCameraSq)) {
        return atomic;
    }
    float dist = std::sqrt(std::fabs(ms_clumpDistToCameraSq));
    void* lod = MI_GetAtomicFromDistance(vc::At<re::Ptr32<uint8_t>>(atomic, 0x6C), dist);
    if (lod == nullptr) {
        return atomic;
    }
    if (Stub_824EC0(lod) != Stub_824EC0(atomic)) {
        AtomicSetGeometry(atomic, Stub_824EC0(lod), 1);
    }
    uint8_t* plugin = AtomicGetClumpPlugin(atomic);
    if (plugin == nullptr || vc::At<int32_t>(plugin, 0x10) == 0xFF) {
        RenderAtomicWithState(atomic);
    } else {
        RenderAtomicWithAlpha(atomic, vc::At<int32_t>(plugin, 0x10));
    }
    return atomic;
}

// 0x5F2D60: a vehicle part inside the components' cull distance, with the clump's alpha
void* __cdecl RenderVehicleAlphaAtomicCB(uint8_t* atomic) {
    if (ms_cullCompsDist > ms_clumpDistToCameraSq) {
        RenderAtomicWithClumpAlpha(atomic);
    }
    return atomic;
}

// the squared distance from the camera to a frame's position, in the game's order
float SquaredDistanceToCamera(void* frame) {
    uint8_t* ltm = FrameGetLTM(frame);
    float dx = vc::At<float>(ltm, 0x24) - ms_pCameraPosn[0];
    float dy = vc::At<float>(ltm, 0x28) - ms_pCameraPosn[1];
    float dz = vc::At<float>(ltm, 0x2C) - ms_pCameraPosn[2];
    return (dy * dy + dx * dx) + dz * dz;
}

// 0x5F2DD0: an accessory (its clump's frame inside the LOD distance), with the clump's alpha
void* __cdecl RenderAccessoryAtomicCB(uint8_t* atomic) {
    uint8_t* plugin = AtomicGetClumpPlugin(atomic);
    if (ms_lodDistSq > SquaredDistanceToCamera(AtomicGetFrame(plugin))) {
        RenderAtomicWithClumpAlpha(atomic);
    }
    return atomic;
}

// 0x5F3610: a vehicle part with alpha (inside the components' cull distance): into the alpha
// atomic list by its squared distance less 0.0001, drawn now when the list is full
void* __cdecl RenderVehicleAlphaSortedAtomicCB(uint8_t* atomic) {
    if (!(ms_cullCompsDist > ms_clumpDistToCameraSq)) {
        return atomic;
    }
    AlphaObjectInfo item;
    item.entity = atomic;
    float d2 = SquaredDistanceToCamera(AtomicGetFrame(atomic));
    item.dist = float(double(d2) - *reinterpret_cast<const double*>(0x00900AF8));
    if (AlphaList_InsertSorted(&ms_alphaAtomicList, &item) == nullptr) {
        RenderAtomicWithState(atomic);
    }
    return atomic;
}

// 0x5F2EB0 (cdecl)
void __cdecl SetRenderWareCamera(void* camera) {
    ms_pCamera = camera;
    uint8_t* matrix = FrameGetMatrix(CameraGetFrame(camera));
    ms_cullCompsDist = 5625.0f;
    ms_pCameraPosn = reinterpret_cast<float*>(matrix + 0x24);
    float d = float(double(lodDistMultiplier_C3CD00) * 60.0);
    ms_lodDistSq = d * d;
}

// 0x5F2F00 (thiscall): CLinkList::Init, all `count` links on the free list
void __fastcall AlphaList_InitImpl(AlphaList* self, re::Edx, uint32_t count) {
    uint64_t bytes = uint64_t(count) * 16;
    self->links = static_cast<AlphaLink*>(Malloc(bytes > 0xFFFFFFFFu ? size_t(-1) : size_t(bytes)));
    self->freeHead.next = &self->freeTail;
    self->usedHead.next = &self->usedTail;
    self->usedTail.prev = &self->usedHead;
    self->freeTail.prev = &self->freeHead;
    for (uint32_t i = count; i != 0; i--) {
        AlphaLink* link = &self->links[i - 1];
        link->next = self->freeHead.next;
        self->freeHead.next->prev = link;
        link->prev = &self->freeHead;
        self->freeHead.next = link;
    }
}

// 0x5F2F70 (thiscall): the item before the first one not nearer (nullptr: no free link)
AlphaLink* __fastcall AlphaList_InsertSortedImpl(AlphaList* self, re::Edx, AlphaObjectInfo* item) {
    AlphaLink* at = self->usedHead.next;
    while (at != &self->usedTail && !(item->dist <= at->item.dist)) {
        at = at->next;
    }
    AlphaLink* link = self->freeHead.next;
    if (link == &self->freeTail) {
        return nullptr;
    }
    link->item = *item;
    link->next->prev = link->prev;
    link->prev->next = link->next;
    AlphaLink* before = at->prev;
    link->next = before->next;
    before->next->prev = link;
    link->prev = before;
    before->next = link;
    return link;
}

// 0x5F2B30 (esi: atomic): drawn with the given alpha, then opaque again
void RenderAtomicWithAlpha(void* atomic, int32_t alpha) {
    Stub_824EC0(atomic);
    Stub_824EC0(atomic);
    vc::At<int32_t>(atomic, 0x1C) = alpha;
    RenderAtomicWithState(atomic);
    Stub_824EC0(atomic);
    vc::At<int32_t>(atomic, 0x1C) = 0xFF;
}
RE_UNHOOKED_FN("CVisibilityPlugins::RenderAtomicWithAlpha", 0x005F2B30, &RenderAtomicWithAlpha);

// the atomic drawn at the entity's transform with the alpha
void DrawFadingAtomic(void* atomic, int32_t alpha) {
    Stub_824EC0(atomic);
    vc::At<int32_t>(atomic, 0x1C) = alpha;
    SetObjectWorldTransform(atomic, static_cast<uint8_t*>(AtomicGetFrame(atomic)) + 0x14);
    RenderAtomic(atomic);
    Stub_824EC0(atomic);
    vc::At<int32_t>(atomic, 0x1C) = 0xFF;
    AtomicSetFrame(atomic, nullptr);
}

// 0x5F2FE0 (edi: the entity): an entity faded by its distance (peds by their own
// alpha); the faded models flagged 0x80 with additive-like blending
void RenderFadingEntity(uint8_t* entity, float dist) {
    if (vc::At<int32_t>(entity, 0xD0) != 0) {
        return;
    }
    void* atomic = nullptr;
    void* lodAtomic = nullptr;
    uint8_t blended = 0;
    uint32_t alpha;
    bool opaque = false;
    if ((entity[0x108] & 7) == 6) {
        alpha = uint32_t(int32_t(vc::At<int16_t>(entity, 0x250)));
        if (alpha == 0xFF) {
            uint8_t* mi = ms_modelInfoPtrs[vc::At<int16_t>(entity, 0x10E)];
            float left = MI_GetLodDistance_520320(mi) - dist;
            float t = left / MI_GetFadeDistance_520340(mi);
            if (t > 1.0f) {
                t = 1.0f;
            } else if (t <= 0.0f) {   // (a NaN goes on, as the x87 compare)
                return;
            }
            alpha = uint32_t(int64_t(float(double(t) * 255.0)));
        }
        if (vc::At<int16_t>(entity, 0x10E) == alwaysOpaqueModel_BF4488) {
            alpha = 0xFF;
            opaque = true;
        }
    } else {
        uint8_t* mi = ms_modelInfoPtrs[vc::At<int16_t>(entity, 0x10E)];
        atomic = vc::At<void*>(entity, 0x18);
        float near = dist - 20.0f;
        float lod = MI_GetLargestLodDistance(mi);
        float t = (lod - near) / 20.0f;
        lodAtomic = MI_GetAtomicFromDistance(mi, near);
        if (t > 1.0f) {
            t = 1.0f;
        } else if (t <= 0.0f) {   // (a NaN goes on, as the x87 compare)
            return;
        }
        blended = uint8_t((vc::At<uint32_t>(mi, 0x28) >> 7) & 1);
        alpha = uint32_t(int64_t(float(double(mi[0x2C]) * t)));
    }
    if (!opaque && alpha <= 0x20) {
        return;
    }
    int saved[7];
    if (blended) {
        GetRenderState(8, &saved[0]);
        GetRenderState(6, &saved[1]);
        GetRenderState(0xC, &saved[2]);
        GetRenderState(0xA, &saved[3]);
        GetRenderState(0xB, &saved[4]);
        GetRenderState(7, &saved[5]);
        GetRenderState(0xE, &saved[6]);
        SetRenderState(8, 0);
        SetRenderState(6, 1);
        SetRenderState(0xC, 1);
        SetRenderState(0xA, 5);
        SetRenderState(0xB, 2);
        SetRenderState(7, 2);
        SetRenderState(0xE, 0);
        SetRenderState(1, 0);
    }
    vc::Call<void>(entity, 0x5C);
    vc::At<int32_t>(entity, 0x80) = 1;
    if (atomic && !AtomicGetFrame(atomic)) {
        if (vc::At<void*>(entity, 0x14)) {
            Matrix_UpdateRwMatrix(Entity_GetMatrix(entity), FrameGetMatrix(ms_pFrame));
        } else {
            Placeable_UpdateRwMatrix(entity + 4, FrameGetMatrix(ms_pFrame));
        }
        FrameUpdateObjects(ms_pFrame);
        AtomicSetFrame(atomic, ms_pFrame);
    }
    if (lodAtomic) {
        uint32_t lodGeometry = Stub_824EC0(lodAtomic);
        if (lodGeometry != Stub_824EC0(atomic)) {
            AtomicSetGeometry(atomic, Stub_824EC0(lodAtomic), 1);
        }
        DrawFadingAtomic(atomic, int32_t(alpha));
    } else if (atomic) {
        DrawFadingAtomic(atomic, int32_t(alpha));
    } else {
        int16_t savedAlpha = vc::At<int16_t>(entity, 0x250);
        vc::At<int16_t>(entity, 0x250) = int16_t(alpha);
        vc::Call<void>(entity, 0x48);
        vc::At<int16_t>(entity, 0x250) = savedAlpha;
    }
    LightAccumulator_5EE560(lightAccumulator_D140B8);
    if (blended) {
        SetRenderState(8, saved[0]);
        SetRenderState(6, saved[1]);
        SetRenderState(0xC, saved[2]);
        SetRenderState(0xA, saved[3]);
        SetRenderState(0xB, saved[4]);
        SetRenderState(7, saved[5]);
        SetRenderState(0xE, saved[6]);
    }
    vc::At<int32_t>(entity, 0x80) = 0;
}
RE_UNHOOKED_FN("CVisibilityPlugins::RenderFadingEntity", 0x005F2FE0, &RenderFadingEntity);

// 0x5F33F0 (cdecl): into the underwater list first when flagged, else (or when it's
// full) the entity list; 0 when that is full too
uint8_t __cdecl InsertEntityIntoSortedList(uint8_t* entity, float dist) {
    AlphaObjectInfo item{entity, dist};
    if (vc::At<int32_t>(entity, 0xB8) != 0 && AlphaList_InsertSorted(&ms_alphaUnderwaterList, &item)) {
        return 1;
    }
    return AlphaList_InsertSorted(&ms_alphaEntityList, &item) != nullptr;
}

// 0x5F3440 (cdecl): into the fourth list (the link returned as the game leaves it in eax)
AlphaLink* __cdecl InsertIntoFourthList(uint8_t* entity, float dist) {
    AlphaObjectInfo item{entity, dist};
    return AlphaList_InsertSorted(&ms_alphaFourthList, &item);
}

bool NeedsDepthWriteOff(uint8_t* mi) {
    return mi[8] == 1 && (vc::At<uint32_t>(mi, 0x28) & 0x400);
}

// 0x5F3470 (cdecl): a sorted list drawn far to near (entities with an rw object; the
// ones with +0x98 faded)
void __cdecl RenderOrderedList(AlphaList* list) {
    for (AlphaLink* link = list->usedTail.prev; link != &list->usedHead; link = link->prev) {
        uint8_t* entity = link->item.entity;
        if (!vc::At<void*>(entity, 0x18)) {
            continue;
        }
        uint8_t* mi = ms_modelInfoPtrs[vc::At<int16_t>(entity, 0x10E)];
        if (NeedsDepthWriteOff(mi)) {
            SetRenderState(8, 0);
        }
        if (vc::At<int32_t>(entity, 0x98) == 0) {
            RenderOneNonRoad(entity);
        } else {
            CallRenderFadingEntity(entity, link->item.dist);
        }
        if (NeedsDepthWriteOff(mi)) {
            SetRenderState(8, 1);
        }
    }
}

// 0x5F3500 (cdecl): the fourth list drawn far to near
void __cdecl RenderFourthList() {
    AlphaList* list = &ms_alphaFourthList;
    for (AlphaLink* link = list->usedTail.prev; link != &list->usedHead; link = link->prev) {
        uint8_t* entity = link->item.entity;
        if (!vc::At<void*>(entity, 0x18)) {
            continue;
        }
        uint8_t* mi = ms_modelInfoPtrs[vc::At<int16_t>(entity, 0x10E)];
        if (NeedsDepthWriteOff(mi)) {
            SetRenderState(8, 0);
        }
        RenderOneNonRoad(entity);
        if (NeedsDepthWriteOff(mi)) {
            SetRenderState(8, 1);
        }
    }
}

// 0x5F3580 (cdecl): the lists (their end markers at 0 and 1e8) and the frame
void __cdecl Initialise() {
    AlphaList_Init(&ms_alphaAtomicList, 20);
    ms_alphaAtomicList.usedHead.item.dist = 0.0f;
    ms_alphaAtomicList.usedTail.item.dist = 100000000.0f;
    AlphaList_Init(&ms_alphaEntityList, 200);
    ms_alphaEntityList.usedHead.item.dist = 0.0f;
    ms_alphaEntityList.usedTail.item.dist = 100000000.0f;
    AlphaList_Init(&ms_alphaUnderwaterList, 30);
    ms_alphaUnderwaterList.usedHead.item.dist = 0.0f;
    ms_alphaUnderwaterList.usedTail.item.dist = 100000000.0f;
    AlphaList_Init(&ms_alphaFourthList, 20);
    ms_alphaFourthList.usedHead.item.dist = 0.0f;
    ms_alphaFourthList.usedTail.item.dist = 100000000.0f;
    ms_pFrame = RwFrameCreate();
}

} // namespace

#define VISPLUGINS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

VISPLUGINS_REVERSED("CVisibilityPlugins::SetFlags_5F2AF0", 0x005F2AF0, &SetFlags_5F2AF0);
VISPLUGINS_REVERSED("CVisibilityPlugins::SetClumpAlpha", 0x005F2B00, &SetClumpAlpha);
VISPLUGINS_REVERSED("CVisibilityPlugins::GetClumpAlpha", 0x005F2B10, &GetClumpAlpha);
VISPLUGINS_REVERSED("CVisibilityPlugins::RenderFadingClump", 0x005F2B60, &RenderFadingClump);
VISPLUGINS_REVERSED("CVisibilityPlugins::Shutdown", 0x005F2BD0, &Shutdown);
VISPLUGINS_REVERSED("CVisibilityPlugins::SetupVehicleVariables", 0x005F2C20, &SetupVehicleVariables);
VISPLUGINS_REVERSED("CVisibilityPlugins::RenderWheelAtomicCB", 0x005F2CA0, &RenderWheelAtomicCB);
VISPLUGINS_REVERSED("CVisibilityPlugins::RenderVehicleAlphaAtomicCB", 0x005F2D60, &RenderVehicleAlphaAtomicCB);
VISPLUGINS_REVERSED("CVisibilityPlugins::RenderAccessoryAtomicCB", 0x005F2DD0, &RenderAccessoryAtomicCB);
VISPLUGINS_REVERSED("CVisibilityPlugins::RenderVehicleAlphaSortedAtomicCB", 0x005F3610, &RenderVehicleAlphaSortedAtomicCB);
VISPLUGINS_REVERSED("CVisibilityPlugins::SetRenderWareCamera", 0x005F2EB0, &SetRenderWareCamera);
VISPLUGINS_REVERSED("CLinkList<AlphaObjectInfo>::Init", 0x005F2F00, &AlphaList_InitImpl);
VISPLUGINS_REVERSED("CLinkList<AlphaObjectInfo>::InsertSorted", 0x005F2F70, &AlphaList_InsertSortedImpl);
VISPLUGINS_REVERSED("CVisibilityPlugins::InsertEntityIntoSortedList", 0x005F33F0, &InsertEntityIntoSortedList);
VISPLUGINS_REVERSED("CVisibilityPlugins::InsertIntoFourthList", 0x005F3440, &InsertIntoFourthList);
VISPLUGINS_REVERSED("CVisibilityPlugins::RenderOrderedList", 0x005F3470, &RenderOrderedList);
VISPLUGINS_REVERSED("CVisibilityPlugins::RenderFourthList", 0x005F3500, &RenderFourthList);
VISPLUGINS_REVERSED("CVisibilityPlugins::Initialise", 0x005F3580, &Initialise);
