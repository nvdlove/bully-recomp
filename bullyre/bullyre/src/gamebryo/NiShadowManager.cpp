// NiShadowManager (the one manager, ms_pkShadowManager, 0x108 bytes): its shadow techniques,
// click generators, shadow generators (all of them and the active ones), the pools of
// NiShadowMap / NiShadowCubeMap (list nodes with the map inside, from block allocators with
// their own locks), the texture memory budget and its default purge; NiShadowMap's set-up;
// NiShadowClickValidator; and the NiTPointerList helpers these lists use.

#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

// a list node: next, previous, the element (a pointer, or for the pools the map itself)
struct Node {
    re::Ptr32<Node> next;   // +0x0
    re::Ptr32<Node> prev;   // +0x4
    re::Ptr32<void> element;   // +0x8 (the pools' maps start here)
};
struct List {
    re::Ptr32<void> vtable;   // +0x0 (1: a new node, 2: a node freed)
    re::Ptr32<Node> head;     // +0x4
    re::Ptr32<Node> tail;     // +0x8
    uint32_t count; // +0xC
};
static_assert(sizeof(List) == 0x10);

inline Node* NewNode(List* list) {
    return Virtual<Node*>(list, 1);
}
inline void FreeNode(List* list, Node* node) {
    Virtual<void>(list, 2, node);
}
// a node linked at the tail
inline void LinkTail(List* list, Node* node) {
    node->next = nullptr;
    node->prev = list->tail;
    if (list->tail != nullptr) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->count++;
    list->tail = node;
}
// the tail node unlinked and freed
inline void DropTail(List* list) {
    Node* node = list->tail;
    Node* prev = node->prev;
    list->tail = prev;
    if (prev != nullptr) {
        prev->next = nullptr;
    } else {
        list->head = nullptr;
    }
    FreeNode(list, node);
    list->count--;
}
// the head node unlinked and freed
inline void DropHead(List* list) {
    Node* node = list->head;
    Node* next = node->next;
    list->head = next;
    if (next != nullptr) {
        next->prev = nullptr;
    } else {
        list->tail = nullptr;
    }
    FreeNode(list, node);
    list->count--;
}
// every node freed (the list's destructor, inlined)
inline void FreeAll(List* list, uintptr_t vtable) {
    list->vtable = reinterpret_cast<void*>(vtable);
    for (Node* node = list->head; node != nullptr;) {
        Node* next = node->next;
        FreeNode(list, node);
        node = next;
    }
    list->count = 0;
    list->head = nullptr;
    list->tail = nullptr;
    list->vtable = reinterpret_cast<void*>(0x0099C748);
}

using PurgeFn = void(__cdecl*)(uint32_t needed, List* active, List* maps, List* cubes, int purgeActive);

struct NiShadowManager {
    re::Ptr32<void> activeTechniques[4];        // +0x0 (by slot)
    re::Ptr32<void> knownTechniques[16];        // +0x10 (by ID)
    re::Ptr32<void> directionalMaterial;        // +0x50
    re::Ptr32<void> pointMaterial;              // +0x54
    re::Ptr32<void> spotMaterial;               // +0x58
    List mapPool;                     // +0x5C (NiTList<NiShadowMap>: vtable 0x951434)
    List mapsAvailable;               // +0x6C (vtable 0x952E10)
    List cubePool;                    // +0x7C (NiTList<NiShadowCubeMap>: vtable 0x951440)
    List cubesAvailable;              // +0x8C (vtable 0x952E10)
    re::Ptr32<re::Ptr32<void>> clickGenerators;           // +0x9C (NiTPrimitiveSet: base, allocated, count)
    uint32_t clickGeneratorsAllocated;   // +0xA0
    uint32_t clickGeneratorCount;     // +0xA4
    int32_t activeClickGenerator;     // +0xA8
    uint32_t usedMemory;              // +0xAC
    re::Ptr32<void __cdecl(uint32_t needed, List* active, List* maps, List* cubes, int purgeActive)> purge;                    // +0xB0
    uint32_t maxActiveGenerators;     // +0xB4
    uint32_t memoryBudget;            // +0xB8
    uint8_t active;                   // +0xBC
    uint8_t supported;                // +0xBD
    uint8_t padBE[2];
    re::Ptr32<NiRefObject> sceneCamera;         // +0xC0
    re::Ptr32<NiRefObject> cullingProcess;      // +0xC4
    re::Ptr32<NiRefObject> directionalSwap;     // +0xC8 (NiMaterialSwapProcessor)
    re::Ptr32<NiRefObject> pointSwap;           // +0xCC
    re::Ptr32<NiRefObject> spotSwap;            // +0xD0
    re::Ptr32<NiRefObject> clickValidator;      // +0xD4 (NiShadowClickValidator)
    List generators;                  // +0xD8 (NiTPointerList<NiShadowGeneratorPtr>: 0x95127C)
    List activeGenerators;            // +0xE8 (0x95127C)
    List renderClicks;                // +0xF8 (0x95145C)
};
static_assert(offsetof(NiShadowManager, mapPool) == 0x5C);
static_assert(offsetof(NiShadowManager, clickGenerators) == 0x9C);
static_assert(offsetof(NiShadowManager, sceneCamera) == 0xC0);
static_assert(offsetof(NiShadowManager, generators) == 0xD8);
static_assert(sizeof(NiShadowManager) == 0x108);

GAME_VAR(NiShadowManager*, ms_pkShadowManager, 0x020E2410);
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);
GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(void*, ms_pkRenderClickFactory, 0x020E23B8);   // slot 0 makes a render click
GAME_VAR(uint32_t, kNi3DRenderViewClassIndex, 0x020E35D0);
GAME_VAR(uint32_t, kNiFragmentMaterialClassIndex, 0x020E34C8);

// the node pools (NiTListAllocator): free list, block list, lock, blocks of
GAME_VAR(Node*, mapBlocks, 0x020E2400);
GAME_VAR(Node*, mapFreeList, 0x020E2404);
GAME_VAR(Node*, cubeBlocks, 0x020E2408);
GAME_VAR(Node*, cubeFreeList, 0x020E240C);
inline NiCriticalSection& cubeLock = *reinterpret_cast<NiCriticalSection*>(0x020E2414);
inline NiCriticalSection& mapLock = *reinterpret_cast<NiCriticalSection*>(0x020E2434);
GAME_VAR(uint32_t, mapBlockSize, 0x00BC75A4);
GAME_VAR(uint32_t, cubeBlockSize, 0x00BC75A8);

// Bully's NiIsKindOf: the class's bit in its RTTI's set
inline void* IsKindOf(void* object, uint32_t classIndex) {
    if (object == nullptr) {
        return nullptr;
    }
    const uint32_t* rtti = Virtual<const uint32_t*>(object, 2);
    return rtti[3 + (classIndex >> 5)] & (1u << (classIndex & 31)) ? object : nullptr;
}

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}
// NiPointer assignment: the old one released first
inline void AssignRef(re::Ptr32<NiRefObject>& slot, NiRefObject* value) {
    if (slot != value) {
        ReleaseRef(slot);
        slot = value;
        if (value != nullptr) {
            Increment(&value->refCount);
        }
    }
}

// a texture's bytes (NiTexture::FormatPrefs' pixel layout and mipmapping), as the manager
// counts them
inline uint32_t TextureBytes(uint32_t pixels, uint32_t layout, uint32_t mipmapped) {
    uint32_t bytes = pixels;
    switch (layout) {
    case 0:
    case 7:
        break;
    case 5:
        bytes >>= 1;
        break;
    case 1:
    case 8:
        bytes *= 2;
        break;
    case 11:
    case 13:
        bytes *= 8;
        break;
    case 14:
        bytes <<= 4;
        break;
    default:
        bytes *= 4;
        break;
    }
    if (mipmapped != 0) {
        bytes = uint32_t(bytes * 4) / 3;
    }
    return bytes;
}
// a texture's bytes from the texture (its width and height, slots 18 and 19)
inline uint32_t TextureBytesOf(void* texture, uint32_t faces) {
    uint32_t height = Virtual<uint32_t>(texture, 19);
    uint32_t width = Virtual<uint32_t>(texture, 18);
    uint32_t pixels = width * height;
    if (faces == 6) {
        pixels = pixels * 6;
    }
    return TextureBytes(pixels, At<uint32_t>(texture, 0x18), At<uint32_t>(texture, 0x20));
}

GAME_FN(void, __thiscall, List_RemoveAll, 0x0075DB60, List* list);
GAME_FN(void, __thiscall, List_AddTail, 0x007843D0, List* list, const re::Ptr32<NiRefObject>* element);
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, List_RemoveRef, 0x0075E6B0, List* list, re::Ptr32<NiRefObject>* result,
        const re::Ptr32<NiRefObject>* element);
GAME_FN(void, __thiscall, List_RemovePointer, 0x0075F630, List* list, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, List_RemovePos, 0x00771DA0, List* list, re::Ptr32<Node>* position);
GAME_FN(void, __thiscall, List_RemoveTail, 0x00756EB0, List* list);
GAME_FN(void, __thiscall, PoolList_RemovePos, 0x00756EF0, List* list, re::Ptr32<Node>* position);
GAME_FN(void, __thiscall, PoolList_Remove, 0x00757440, List* list, void* element);
GAME_FN(void, __thiscall, NiShadowGenerator_SetActive, 0x007736C0, void* generator, int active, int force,
        int retainShadowMaps);
GAME_FN(void, __thiscall, NiShadowGenerator_773460, 0x00773460, void* generator);
GAME_FN(void, __thiscall, NiShadowGenerator_772930, 0x00772930, void* generator);
GAME_FN(void, __thiscall, NiShadowGenerator_7735F0, 0x007735F0, void* generator);
GAME_FN(void, __thiscall, NiDynamicEffect_AttachShadowGenerator, 0x00771990, void* effect, void* generator);
GAME_FN(void, __thiscall, ClickGenerator_AddShadowGenerator, 0x007ABA20, void* clickGenerator, void* generator);
GAME_FN(void, __thiscall, ClickGenerator_PrepareGenerators, 0x007ABA80, void* clickGenerator, List* generators);
GAME_FN(void, __thiscall, ClickGeneratorSet_Add, 0x00787BE0, void* set, const re::Ptr32<NiRefObject>* generator);
GAME_FN(void, __thiscall, ClickGeneratorSet_RemoveAll, 0x00836940, void* set);
GAME_FN(void, __cdecl, NiArrayFree, 0x00836F80, void* p);
GAME_FN(uint8_t, __cdecl, NiShadowMap_Initialize, 0x007562C0, void* map, int width, int height, void* prefs,
        int a4, int a5);
GAME_FN(void, __cdecl, NiShadowMap_Shutdown, 0x00756120, void* map);
GAME_FN(uint8_t, __cdecl, NiShadowCubeMap_Initialize, 0x00760CA0, void* cube, int size, void* prefs, int a3,
        int a4);
GAME_FN(void, __cdecl, NiShadowCubeMap_Shutdown, 0x00760C00, void* cube);
GAME_FN(uint32_t, __cdecl, NiShadowManager_GetTextureSize, 0x00756E20, int width, int height, void* prefs,
        int cube);
GAME_FN(void, __cdecl, NiShadowManager_DestroyShadowMap, 0x00757780, void* map);
GAME_FN(void, __thiscall, NiShadowManager_PrepareGenerators, 0x007572A0, NiShadowManager* self);
GAME_FN(uint8_t, __cdecl, NiShadowManager_SetActive, 0x007573A0, int active, int retainShadowMaps);
GAME_FN(void*, __cdecl, NiShadowManager_GetShadowClickGenerator, 0x00757260, const re::Ptr32<char>* name);
GAME_FN(uint8_t, __cdecl, NiShadowManager_SetActiveShadowClickGenerator, 0x00757DA0, const re::Ptr32<char>* name);
GAME_FN(uint8_t, __cdecl, NiShadowManager_RegisterShadowTechnique, 0x00758610, void* technique, uint32_t slot,
        uint32_t id);
GAME_FN(uint8_t, __cdecl, NiShadowManager_RegisterShadowClickGenerator, 0x00758E70, NiRefObject* generator);
GAME_FN(NiShadowManager*, __thiscall, NiShadowManager_Construct, 0x00758F50, void* self);
GAME_FN(void, __thiscall, NiShadowManager_Destruct, 0x00758BB0, void* self);
GAME_FN(void, __cdecl, NiShadowManager_DestroyAllShadowMaps, 0x00757850);
GAME_FN(void, __cdecl, NiShadowManager_DestroyAllShadowCubeMaps, 0x00757AD0);
GAME_FN(void, __cdecl, NiShadowMapPool_CreateFreeMem, 0x007588F0);
GAME_FN(void, __cdecl, NiShadowCubeMapPool_CreateFreeMem, 0x00758AD0);
GAME_FN(void, __cdecl, NiShadowCubeMapPool_FreeMem, 0x00758A60);
GAME_FN(void, __cdecl, NiShadowMapPool_Deallocate, 0x00757020, Node* node);
GAME_FN(void, __cdecl, NiShadowCubeMapPool_Deallocate, 0x00757080, Node* node);
GAME_FN(uint32_t, __cdecl, NiStream_RegisterLoader, 0x00759EC0, const char* name, void* create);
GAME_FN(uint32_t, __cdecl, NiStream_UnregisterLoader, 0x00759EE0, const char* name);
GAME_FN(void, __thiscall, NiShadowClickValidator_Destruct, 0x00757150, void* self);
// the shadow map's pieces
GAME_FN(NiRefObject*, __cdecl, NiRenderedTexture_Create, 0x0075D960, int width, int height, void* renderer,
        void* prefs, int flags);
GAME_FN(void*, __thiscall, NiCamera_Construct, 0x00763C20, void* self);
GAME_FN(void*, __thiscall, Ni3DRenderView_Construct, 0x007561A0, void* self, void* camera, void* culler,
        int alwaysUseCameraViewport);
GAME_FN(void, __thiscall, NiViewRenderClick_AppendRenderView, 0x008316F0, void* click, void* view);
GAME_FN(void, __thiscall, NiRenderClick_SetRenderTargetGroup, 0x007ABEF0, void* click, void* group);
GAME_FN(void*, __cdecl, NiShadowManager_FindDepthBuffer, 0x00758180, void* buffer);
GAME_FN(void*, __cdecl, NiRenderTargetGroup_Create, 0x0075D670, void* buffer, void* renderer, void* depth);
GAME_FN(void*, __cdecl, NiDepthStencilBuffer_CreateCompatible, 0x007ABF40, void* buffer, void* renderer);
// the start-up's pieces
GAME_FN(void*, __cdecl, NiMaterial_GetMaterial, 0x00516AC0, const re::Ptr32<char>* name);
GAME_FN(NiRefObject*, __cdecl, CreateDirectionalShadowWriteLibrary, 0x007C4B90);
GAME_FN(NiRefObject*, __cdecl, CreateSpotShadowWriteLibrary, 0x007BA720);
GAME_FN(NiRefObject*, __cdecl, CreatePointShadowWriteLibrary, 0x007B06F0);
GAME_FN(void*, __thiscall, NiDirectionalShadowWriteMaterial_Construct, 0x007B0650, void* self, void* library,
        int flags);
GAME_FN(void*, __thiscall, NiSpotShadowWriteMaterial_Construct, 0x007AE4D0, void* self, void* library, int flags);
GAME_FN(void*, __thiscall, NiPointShadowWriteMaterial_Construct, 0x007ACF80, void* self, void* library,
        int flags);
GAME_FN(void, __thiscall, NiVisibleArray_SetAllocatedSize, 0x00776B50, void* self, uint32_t size);
GAME_FN(void*, __thiscall, NiShadowTechnique_Construct, 0x007D18F0, void* self, const re::Ptr32<char>* name, char* directional,
        char* point, char* spot, char* directionalWrite, char* pointWrite, char* spotWrite, int a, int b);
GAME_FN(void*, __thiscall, NiVSMShadowTechnique_Construct, 0x007D1070, void* self, const re::Ptr32<char>* name,
        char* directional, char* point, char* spot, char* directionalWrite, char* pointWrite, char* spotWrite, int a,
        int b);
GAME_FN(NiRefObject*, __thiscall, NiDefaultShadowClickGenerator_Construct, 0x007D0D30, void* self);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, uintptr_t size, const char* format, ...);
// the pools' blocks
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* array, uint32_t size, uint32_t count,
        void(__thiscall* construct)(void*));
GAME_FN(void, __thiscall, NiShadowCubeMap_Destruct, 0x00402A20, void* self);
GAME_FN(void, __thiscall, NiShadowMapArray_Destroy, 0x004041B0, void* array, int flags);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t size);
GAME_FN(void, __thiscall, LoaderMap_SetAt, 0x00747F00, void* map, const char* name, void* create);
GAME_FN(uint32_t, __thiscall, LoaderMap_RemoveAt, 0x008A1F90, void* map, const char* name);
GAME_VAR(void*, ms_kLoaders, 0x020E247C);

inline void SetFloatBits(void* p, uint32_t bits) {
    crt::Memcpy(p, &bits, 4);
}

} // namespace

// ---- NiShadowMap

// 0x756120: NiShadowMap::Shutdown (its texture and render click let go, flags cleared)
void __cdecl NiShadowMap_ShutdownImpl(uint8_t* map) {
    At<uint16_t>(map, 4) = 0;
    re::Ptr32<NiRefObject>& texture = At<NiRefObject*>(map, 8);
    if (texture != nullptr) {
        ReleaseRef(texture);
        texture = nullptr;
    }
    re::Ptr32<NiRefObject>& click = At<NiRefObject*>(map, 0xC);
    if (click != nullptr) {
        ReleaseRef(click);
        click = nullptr;
    }
}

// 0x7562C0: NiShadowMap::Initialize: a rendered texture, a render click looking through a new
// camera (Ni3DRenderView with the manager's culling process; the click validator), a render
// target group with a compatible depth buffer; the flags' bits 8..15 from a4 and a5
uint8_t __cdecl NiShadowMap_InitializeImpl(uint8_t* map, int width, int height, void* prefs, int a4, int a5) {
    NiShadowMap_Shutdown(map);
    At<uint16_t>(map, 4) &= 0xFFF0;
    void* renderer = ms_pkRenderer;
    AssignRef(At<NiRefObject*>(map, 8), NiRenderedTexture_Create(width, height, renderer, prefs, 0x100));
    if (At<void*>(map, 8) == nullptr) {
        NiShadowMap_Shutdown(map);
        return 0;
    }
    NiRefObject* click = Virtual<NiRefObject*>(ms_pkRenderClickFactory, 0);
    AssignRef(At<NiRefObject*>(map, 0xC), click);
    uint8_t* mapClick = At<uint8_t*>(map, 0xC);
    At<uint32_t>(mapClick, 0x34) = 7;
    NiRefObject* validator = ms_pkShadowManager->clickValidator;
    if (validator != nullptr) {
        Increment(&validator->refCount);
    }
    AssignRef(At<NiRefObject*>(mapClick, 0xC), validator);
    ReleaseRef(validator);
    void* view = NiNew(0x3C);
    if (view != nullptr) {
        void* camera = NiNew(0x13C);
        camera = camera != nullptr ? NiCamera_Construct(camera) : nullptr;
        view = Ni3DRenderView_Construct(view, camera, ms_pkShadowManager->cullingProcess, 1);
    }
    NiViewRenderClick_AppendRenderView(At<void*>(map, 0xC), view);
    void* buffer = Virtual<void*>(At<void*>(map, 8), 20);
    void* depth = NiShadowManager_FindDepthBuffer(buffer);
    if (depth != nullptr) {
        void* group = NiRenderTargetGroup_Create(buffer, renderer, depth);
        if (group != nullptr) {
            NiRenderClick_SetRenderTargetGroup(At<void*>(map, 0xC), group);
            uint16_t flags = At<uint16_t>(map, 4);
            flags = uint16_t((flags & 0xCFFF) | uint16_t(a4 << 12));
            flags = uint16_t((flags & 0xF0FF) | uint16_t(uint8_t(a5) << 8));
            At<uint16_t>(map, 4) = flags;
            return 1;
        }
    }
    NiShadowMap_Shutdown(map);
    return 0;
}

// 0x756E20: the bytes a shadow texture of that size and format takes (6 faces for a cube)
uint32_t __cdecl NiShadowManager_GetTextureSizeImpl(int width, int height, uint32_t* prefs, int cube) {
    uint32_t pixels = uint32_t(width * height);
    if (re::Flag(cube)) {
        pixels *= 6;
    }
    return TextureBytes(pixels, prefs[0], prefs[2]);
}

// ---- NiTPointerList helpers (thiscall on the list)

// 0x756EB0: the tail removed
void __fastcall List_RemoveTailImpl(List* self, re::Edx) {
    DropTail(self);
}

// 0x756EF0: RemovePos (the position moved on to the next)
void __fastcall List_RemovePosImpl(List* self, re::Edx, re::Ptr32<Node>* position) {
    Node* node = *position;
    if (node == self->head) {
        *position = node->next;
        DropHead(self);
        return;
    }
    if (node == self->tail) {
        *position = nullptr;
        List_RemoveTail(self);
        return;
    }
    Node* next = node->next;
    Node* prev = node->prev;
    *position = next;
    if (prev != nullptr) {
        prev->next = next;
    }
    if (next != nullptr) {
        next->prev = prev;
    }
    FreeNode(self, node);
    self->count--;
}

// 0x756F80: RemoveHead, its element returned as a reference (NiPointer by value)
re::Ptr32<NiRefObject>* __fastcall List_RemoveHeadRef(List* self, re::Edx, re::Ptr32<NiRefObject>* result) {
    Node* node = self->head;
    Node* next = node->next;
    self->head = next;
    if (next == nullptr) {
        self->tail = nullptr;
    } else {
        next->prev = nullptr;
    }
    NiRefObject* element = static_cast<NiRefObject*>(node->element);
    *result = element;
    if (element != nullptr) {
        Increment(&element->refCount);
    }
    FreeNode(self, node);
    self->count--;
    return result;
}

// 0x756FD0: RemoveTail, its element returned as a reference
re::Ptr32<NiRefObject>* __fastcall List_RemoveTailRef(List* self, re::Edx, re::Ptr32<NiRefObject>* result) {
    Node* node = self->tail;
    Node* prev = node->prev;
    self->tail = prev;
    if (prev == nullptr) {
        self->head = nullptr;
    } else {
        prev->next = nullptr;
    }
    NiRefObject* element = static_cast<NiRefObject*>(node->element);
    *result = element;
    if (element != nullptr) {
        Increment(&element->refCount);
    }
    FreeNode(self, node);
    self->count--;
    return result;
}

// ---- the node pools

// 0x757020 and 0x757080: a node back on its pool's free list (under the pool's lock)
void __cdecl NiShadowMapPool_DeallocateImpl(Node* node) {
    Lock(mapLock);
    node->next = mapFreeList;
    mapFreeList = node;
    Unlock(mapLock);
}
void __cdecl NiShadowCubeMapPool_DeallocateImpl(Node* node) {
    Lock(cubeLock);
    node->next = cubeFreeList;
    cubeFreeList = node;
    Unlock(cubeLock);
}

// 0x757070 and 0x7570D0: the lists' allocator slot 2 (a node freed)
void __fastcall NiShadowMapList_FreeNode(List*, re::Edx, Node* node) {
    NiShadowMapPool_Deallocate(node);
}
void __fastcall NiShadowCubeMapList_FreeNode(List*, re::Edx, Node* node) {
    NiShadowCubeMapPool_Deallocate(node);
}

// 0x7570E0 and 0x757110: the pool lists' deleting destructors
List* __fastcall NiShadowMapList_Destroy(List* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x00951434);
    List_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(0x0099C748);
    if (flags & 1) {
        NiDelete(self, sizeof(List));
    }
    return self;
}
List* __fastcall NiShadowCubeMapList_Destroy(List* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(0x00951440);
    List_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(0x0099C748);
    if (flags & 1) {
        NiDelete(self, sizeof(List));
    }
    return self;
}

// ---- NiShadowClickValidator (vtable 0x95144C, 0x20 bytes: camera, culling process, a
// NiVisibleArray)

// 0x757140: GetRTTI
void* __fastcall NiShadowClickValidator_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E3630);
}

// 0x757150: its destructor
void __fastcall NiShadowClickValidator_DestructImpl(uint8_t* self, re::Edx) {
    NiFree(At<void*>(self, 0x10));
    ReleaseRef(At<NiRefObject*>(self, 0xC));
    ReleaseRef(At<NiRefObject*>(self, 8));
    At<uint32_t>(self, 0) = 0x008FEFD4;
    Decrement(&ms_uiObjects);
}

// 0x757490: its deleting destructor
void* __fastcall NiShadowClickValidator_Destroy(uint8_t* self, re::Edx, int flags) {
    NiShadowClickValidator_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x20);
    }
    return self;
}

// ---- NiShadowManager

// 0x7571B0: the active click generator's slot 4 with the active and all generators
void __cdecl NiShadowManager_GenerateActiveClicks() {
    NiShadowManager* m = ms_pkShadowManager;
    void* generator = m->clickGenerators[m->activeClickGenerator];
    Virtual<void>(generator, 4, &m->activeGenerators, &m->generators);
}

// 0x7571E0: GetShadowTechnique by name (the active slots)
void* __cdecl NiShadowManager_GetShadowTechnique(const re::Ptr32<char>* name) {
    NiShadowManager* m = ms_pkShadowManager;
    for (uint32_t i = 0; i < 4; i++) {
        void* technique = m->activeTechniques[i];
        if (technique != nullptr && At<char*>(technique, 4) == *name) {
            return technique;
        }
    }
    return nullptr;
}

// 0x757260: GetShadowClickGenerator by name
void* __cdecl NiShadowManager_GetShadowClickGeneratorImpl(const re::Ptr32<char>* name) {
    NiShadowManager* m = ms_pkShadowManager;
    for (uint32_t i = 0; i < m->clickGeneratorCount; i++) {
        void* generator = m->clickGenerators[i];
        if (At<char*>(generator, 0x18) == *name) {
            return generator;
        }
    }
    return nullptr;
}

// 0x7572A0: every generator's callback (+0x48, when set), then the active click generator
// prepares the list
void __fastcall NiShadowManager_PrepareGeneratorsImpl(NiShadowManager* self, re::Edx) {
    void* clickGenerator = self->clickGenerators[self->activeClickGenerator];
    for (Node* node = self->generators.head; node != nullptr;) {
        void* generator = node->element;
        auto callback = At<void(__cdecl*)(void*)>(generator, 0x48);
        node = node->next;
        if (callback != nullptr) {
            callback(generator);
        }
    }
    ClickGenerator_PrepareGenerators(clickGenerator, &self->generators);
}

// 0x7572F0: GenerateRenderClicks (the active click generator's slot 3, when active)
List* __cdecl NiShadowManager_GenerateRenderClicks() {
    List_RemoveAll(&ms_pkShadowManager->renderClicks);
    NiShadowManager* m = ms_pkShadowManager;
    if (m->active) {
        void* generator = m->clickGenerators[m->activeClickGenerator];
        Virtual<void>(generator, 3, &m->activeGenerators);
        m = ms_pkShadowManager;
    }
    return &m->renderClicks;
}

// 0x757340: SetMaxActiveShadowGenerators (the excess active ones, from the tail, switched off)
void __cdecl NiShadowManager_SetMaxActiveShadowGenerators(uint32_t count) {
    NiShadowManager* m = ms_pkShadowManager;
    if (m->activeGenerators.count > count) {
        for (uint32_t n = m->activeGenerators.count - count; n != 0; n--) {
            void* generator = m->activeGenerators.tail->element;
            NiShadowGenerator_SetActive(generator, 0, 0, 0);
            NiShadowGenerator_773460(generator);
            m = ms_pkShadowManager;
        }
    }
    m->maxActiveGenerators = count;
}

// 0x7573A0: SetActive (switching on needs the device's support); every generator told
void __cdecl NiShadowManager_SetActiveImpl(int active, int retainShadowMaps) {
    NiShadowManager* m = ms_pkShadowManager;
    uint8_t value = uint8_t(active);
    if (value == m->active || (value == 1 && !m->supported)) {
        return;
    }
    m->active = value;
    for (Node* node = ms_pkShadowManager->generators.head; node != nullptr;) {
        void* generator = node->element;
        node = node->next;
        NiShadowGenerator_SetActive(generator, active, 1, retainShadowMaps);
        NiShadowGenerator_773460(generator);
    }
}

// 0x757410: each active generator with flag 8 updated (0x772930)
void __cdecl NiShadowManager_UpdateActiveGenerators() {
    for (Node* node = ms_pkShadowManager->activeGenerators.head; node != nullptr;) {
        void* generator = node->element;
        node = node->next;
        if (At<uint8_t>(generator, 8) >> 3 & 1) {
            NiShadowGenerator_772930(generator);
        }
    }
}

// 0x757440: NiTList (a pool list)::Remove by the element's address
void __fastcall PoolList_RemoveImpl(List* self, re::Edx, void* element) {
    re::Ptr32<Node> found = nullptr;
    for (Node* node = self->head; node != nullptr; node = node->next) {
        if (element == &node->element) {
            found = node;
            break;
        }
    }
    if (found != nullptr) {
        PoolList_RemovePos(self, &found);
    }
}

// 0x7574B0: ValidateShaderVersions (the shaders shadows need: both versions at least 2; else
// shadows off)
uint8_t __cdecl NiShadowManager_ValidateShaderVersions(int vertexShader, int, int, int, int pixelShader) {
    if (uint16_t(vertexShader) >= 2 && uint16_t(pixelShader) >= 2) {
        ms_pkShadowManager->supported = 1;
        return 1;
    }
    ms_pkShadowManager->supported = 0;
    NiShadowManager_SetActive(0, 0);
    return 0;
}

// 0x7574F0: AddShadowGenerator (attached to its light; active when there's room and the
// light is on, else switched off; the click generator told)
uint8_t __cdecl NiShadowManager_AddShadowGenerator(NiRefObject* generator) {
    void* effect = At<void*>(generator, 0x4C);
    NiDynamicEffect_AttachShadowGenerator(effect, generator);
    re::Ptr32<NiRefObject> held = generator;
    Increment(&generator->refCount);
    List_AddTail(&ms_pkShadowManager->generators, &held);
    ReleaseRef(generator);
    if (At<uint8_t>(generator, 8) >> 3 & 1) {
        NiShadowManager* m = ms_pkShadowManager;
        if (m->activeGenerators.count < m->maxActiveGenerators && At<uint8_t>(effect, 0xC4) != 0) {
            Increment(&generator->refCount);
            held = generator;
            List_AddTail(&ms_pkShadowManager->activeGenerators, &held);
            ReleaseRef(generator);
        } else {
            NiShadowGenerator_SetActive(generator, 0, 0, 0);
        }
    }
    NiShadowManager* m = ms_pkShadowManager;
    if (m->activeClickGenerator >= 0) {
        ClickGenerator_AddShadowGenerator(m->clickGenerators[m->activeClickGenerator], generator);
    }
    return 1;
}

// 0x7575D0: CreateShadowMap (sizes up to 2048; the purge asked for room over budget)
void* __cdecl NiShadowManager_CreateShadowMap(int width, int height, uint32_t* prefs, int a4, int a5) {
    if (width >= 0x800) {
        width = 0x800;
    }
    if (height >= 0x800) {
        height = 0x800;
    }
    uint32_t bytes = TextureBytes(uint32_t(width * height), prefs[0], prefs[2]);
    NiShadowManager* m = ms_pkShadowManager;
    if (m->usedMemory + bytes > m->memoryBudget) {
        m->purge(bytes, &m->activeGenerators, &m->mapsAvailable, &m->cubesAvailable, 0);
        m = ms_pkShadowManager;
        if (m->usedMemory + bytes > m->memoryBudget) {
            return nullptr;
        }
    }
    Node* node = NewNode(&m->mapPool);
    LinkTail(&m->mapPool, node);
    void* map = &node->element;
    if (!NiShadowMap_Initialize(map, width, height, prefs, a4, a5)) {
        DropTail(&ms_pkShadowManager->mapPool);
        return nullptr;
    }
    m = ms_pkShadowManager;
    Node* entry = NewNode(&m->mapsAvailable);
    entry->element = map;
    LinkTail(&m->mapsAvailable, entry);
    ms_pkShadowManager->usedMemory += bytes;
    return map;
}

// 0x757780: DestroyShadowMap (out of the lists, its memory given back)
void __cdecl NiShadowManager_DestroyShadowMapImpl(uint8_t* map) {
    re::Ptr32<void> element = map;
    List_RemovePointer(&ms_pkShadowManager->mapsAvailable, &element);
    PoolList_Remove(&ms_pkShadowManager->mapPool, map);
    ms_pkShadowManager->usedMemory -= TextureBytesOf(At<void*>(map, 8), 1);
    NiShadowMap_Shutdown(map);
}

// 0x757850: every shadow map destroyed
void __cdecl NiShadowManager_DestroyAllShadowMapsImpl() {
    List_RemoveAll(&ms_pkShadowManager->mapsAvailable);
    Node* node = ms_pkShadowManager->mapPool.head;
    while (node != nullptr && reinterpret_cast<uintptr_t>(node) + 8 != 0) {
        uint8_t* map = reinterpret_cast<uint8_t*>(&node->element);
        ms_pkShadowManager->usedMemory -= TextureBytesOf(At<void*>(map, 8), 1);
        NiShadowMap_Shutdown(map);
        DropHead(&ms_pkShadowManager->mapPool);
        node = ms_pkShadowManager->mapPool.head;
    }
    List_RemoveAll(&ms_pkShadowManager->mapPool);
    List_RemoveAll(&ms_pkShadowManager->mapsAvailable);
}

// 0x757980: CreateShadowCubeMap
void* __cdecl NiShadowManager_CreateShadowCubeMap(int size, void* prefs, int a3, int a4) {
    if (size >= 0x800) {
        size = 0x800;
    }
    uint32_t bytes = NiShadowManager_GetTextureSize(size, size, prefs, 1);
    NiShadowManager* m = ms_pkShadowManager;
    if (m->usedMemory + bytes > m->memoryBudget) {
        m->purge(bytes, &m->activeGenerators, &m->mapsAvailable, &m->cubesAvailable, 0);
        m = ms_pkShadowManager;
        if (m->usedMemory + bytes > m->memoryBudget) {
            return nullptr;
        }
    }
    Node* node = NewNode(&m->cubePool);
    LinkTail(&m->cubePool, node);
    void* cube = &node->element;
    if (!NiShadowCubeMap_Initialize(cube, size, prefs, a3, a4)) {
        DropTail(&ms_pkShadowManager->cubePool);
        return nullptr;
    }
    m = ms_pkShadowManager;
    Node* entry = NewNode(&m->cubesAvailable);
    entry->element = cube;
    LinkTail(&m->cubesAvailable, entry);
    ms_pkShadowManager->usedMemory += bytes;
    return cube;
}

// 0x757AD0: every shadow cube map destroyed
void __cdecl NiShadowManager_DestroyAllShadowCubeMapsImpl() {
    List_RemoveAll(&ms_pkShadowManager->cubesAvailable);
    Node* node = ms_pkShadowManager->cubePool.head;
    while (node != nullptr && reinterpret_cast<uintptr_t>(node) + 8 != 0) {
        uint8_t* cube = reinterpret_cast<uint8_t*>(&node->element);
        ms_pkShadowManager->usedMemory -= TextureBytesOf(At<void*>(cube, 8), 6);
        NiShadowCubeMap_Shutdown(cube);
        DropHead(&ms_pkShadowManager->cubePool);
        node = ms_pkShadowManager->cubePool.head;
    }
    List_RemoveAll(&ms_pkShadowManager->cubePool);
    List_RemoveAll(&ms_pkShadowManager->cubesAvailable);
}

// 0x757C10: an available shadow map of the format: the exact size, else (unless exactOnly)
// the nearest; its list position
Node* __cdecl NiShadowManager_FindAvailableShadowMap(int width, int height, uint32_t* prefs, re::Ptr32<void>* found,
                                                    int exactOnly) {
    int target = width * height;
    *found = nullptr;
    int best = 0x7FFFFFFF;
    Node* bestNode = nullptr;
    for (Node* node = ms_pkShadowManager->mapsAvailable.head; node != nullptr;) {
        void* map = node->element;
        Node* current = node;
        node = node->next;
        uint8_t* texture = At<uint8_t*>(map, 8);
        if (At<uint32_t>(texture, 0x18) == prefs[0] && At<uint32_t>(texture, 0x1C) == prefs[1] &&
            At<uint32_t>(texture, 0x20) == prefs[2]) {
            int h = Virtual<int>(texture, 19);
            int w = Virtual<int>(texture, 18);
            int difference = h * w - target;
            difference = difference < 0 ? -difference : difference;
            if (difference == 0) {
                *found = map;
                return current;
            }
            if (!re::Flag(exactOnly) && difference < best) {
                bestNode = current;
                *found = map;
                best = difference;
            }
        }
    }
    return bestNode;
}

// 0x757CF0: the same for cube maps (by width)
Node* __cdecl NiShadowManager_FindAvailableShadowCubeMap(int size, uint32_t* prefs, re::Ptr32<void>* found, int exactOnly) {
    *found = nullptr;
    int best = 0x7FFFFFFF;
    Node* bestNode = nullptr;
    for (Node* node = ms_pkShadowManager->cubesAvailable.head; node != nullptr;) {
        void* cube = node->element;
        uint8_t* texture = At<uint8_t*>(cube, 8);
        Node* current = node;
        node = node->next;
        if (At<uint32_t>(texture, 0x18) == prefs[0] && At<uint32_t>(texture, 0x1C) == prefs[1] &&
            At<uint32_t>(texture, 0x20) == prefs[2]) {
            int difference = Virtual<int>(texture, 18) - size;
            difference = difference < 0 ? -difference : difference;
            if (difference == 0) {
                *found = cube;
                return current;
            }
            if (!re::Flag(exactOnly) && difference < best) {
                bestNode = current;
                *found = cube;
                best = difference;
            }
        }
    }
    return bestNode;
}

// 0x757DA0: SetActiveShadowClickGenerator by name
uint8_t __cdecl NiShadowManager_SetActiveShadowClickGeneratorImpl(const re::Ptr32<char>* name) {
    NiShadowManager* m = ms_pkShadowManager;
    for (uint32_t i = 0; i < m->clickGeneratorCount; i++) {
        if (At<char*>(m->clickGenerators[i], 0x18) == *name) {
            m->activeClickGenerator = int32_t(i);
            NiShadowManager_PrepareGenerators(ms_pkShadowManager);
            return 1;
        }
    }
    return 0;
}

// 0x757DF0: SetSceneCamera (the click validator's too)
void __cdecl NiShadowManager_SetSceneCamera(NiRefObject* camera) {
    AssignRef(ms_pkShadowManager->sceneCamera, camera);
    NiShadowManager* m = ms_pkShadowManager;
    AssignRef(At<NiRefObject*>(m->clickValidator, 8), m->sceneCamera);
}

// 0x757E90: SetCullingProcess (the validator's, and every map's render view's)
void __cdecl NiShadowManager_SetCullingProcess(NiRefObject* process) {
    AssignRef(ms_pkShadowManager->cullingProcess, process);
    NiShadowManager* m = ms_pkShadowManager;
    AssignRef(At<NiRefObject*>(m->clickValidator, 0xC), m->cullingProcess);
    // (a click's first render view: its list's head (+0x50) element)
    auto firstView = [](void* click) { return static_cast<uint8_t*>(At<Node*>(click, 0x50)->element); };
    for (Node* node = ms_pkShadowManager->mapPool.head; node != nullptr;) {
        NiRefObject* culler = ms_pkShadowManager->cullingProcess;
        uint8_t* view = firstView(At<void*>(&node->element, 0xC));
        node = node->next;
        AssignRef(At<NiRefObject*>(view, 0x24), culler);
    }
    for (Node* node = ms_pkShadowManager->cubePool.head; node != nullptr;) {
        Node* next = node->next;
        for (int face = 0; face < 6; face++) {
            uint8_t* view = firstView(At<void*>(&node->element, 0x10 + face * 4));
            NiRefObject* culler = ms_pkShadowManager->cullingProcess;
            AssignRef(At<NiRefObject*>(view, 0x24), culler);
        }
        node = next;
    }
}

// 0x758020: a shadow map made available again (its render view's scenes cleared)
void __cdecl NiShadowManager_ReleaseShadowMap(uint8_t* map) {
    void* click = At<void*>(map, 0xC);
    if (click != nullptr) {
        void* view = IsKindOf(At<Node*>(click, 0x50)->element, kNi3DRenderViewClassIndex);
        List_RemoveAll(reinterpret_cast<List*>(static_cast<uint8_t*>(view) + 0x28));
    }
    NiShadowManager* m = ms_pkShadowManager;
    Node* entry = NewNode(&m->mapsAvailable);
    entry->element = map;
    LinkTail(&m->mapsAvailable, entry);
}

// 0x7580B0: a shadow cube map made available again (each face's view's scenes freed)
void __cdecl NiShadowManager_ReleaseShadowCubeMap(uint8_t* cube) {
    for (uint32_t face = 0; face < 6; face++) {
        void* click = At<void*>(cube, 0x10 + int(uint8_t(face)) * 4);
        if (click == nullptr) {
            continue;
        }
        void* view = IsKindOf(At<Node*>(click, 0x50)->element, kNi3DRenderViewClassIndex);
        List* scenes = reinterpret_cast<List*>(static_cast<uint8_t*>(view) + 0x28);
        for (Node* node = scenes->head; node != nullptr;) {
            Node* next = node->next;
            FreeNode(scenes, node);
            node = next;
        }
        scenes->count = 0;
        scenes->head = nullptr;
        scenes->tail = nullptr;
    }
    NiShadowManager* m = ms_pkShadowManager;
    Node* entry = NewNode(&m->cubesAvailable);
    entry->element = cube;
    LinkTail(&m->cubesAvailable, entry);
}

// 0x758180: a depth buffer for a render target: one of the maps' (the renderer's slot 28
// says it fits), else a new compatible one
void* __cdecl NiShadowManager_FindDepthBufferImpl(void* buffer) {
    void* renderer = ms_pkRenderer;
    for (Node* node = ms_pkShadowManager->mapPool.head; node != nullptr;) {
        void* click = At<void*>(&node->element, 0xC);
        node = node->next;
        if (click != nullptr) {
            void* group = At<void*>(click, 0x10);
            if (group != nullptr) {
                void* depth = Virtual<void*>(group, 28);
                if (depth != nullptr && Virtual<uint8_t>(renderer, 28, buffer, depth)) {
                    return depth;
                }
            }
        }
    }
    for (Node* node = ms_pkShadowManager->cubePool.head; node != nullptr;) {
        uint8_t* cube = reinterpret_cast<uint8_t*>(&node->element);
        node = node->next;
        for (uint32_t face = 0; face < 6; face++) {
            void* click = At<void*>(cube, 0x10 + int(uint8_t(face)) * 4);
            if (click == nullptr) {
                continue;
            }
            void* group = At<void*>(click, 0x10);
            if (group == nullptr) {
                continue;
            }
            void* depth = Virtual<void*>(group, 28);
            if (depth != nullptr && Virtual<uint8_t>(renderer, 28, buffer, depth)) {
                return depth;
            }
        }
    }
    return NiDepthStencilBuffer_CreateCompatible(buffer, renderer);
}

// one available cube map destroyed (the purge's step)
namespace {
inline void PurgeCube(List* cubes) {
    uint8_t* cube = static_cast<uint8_t*>(cubes->head->element);
    NiShadowManager* m = ms_pkShadowManager;
    re::Ptr32<Node> position = nullptr;
    for (Node* node = m->cubesAvailable.head; node != nullptr; node = node->next) {
        if (node->element == cube) {
            position = node;
            break;
        }
    }
    if (position != nullptr) {
        List_RemovePos(&m->cubesAvailable, &position);
        m = ms_pkShadowManager;
    }
    position = nullptr;
    for (Node* node = m->cubePool.head; node != nullptr; node = node->next) {
        if (cube == reinterpret_cast<uint8_t*>(&node->element)) {
            position = node;
            break;
        }
    }
    if (position != nullptr) {
        PoolList_RemovePos(&m->cubePool, &position);
    }
    uint32_t bytes = TextureBytesOf(At<void*>(cube, 8), 6);
    NiShadowCubeMap_Shutdown(cube);
    ms_pkShadowManager->usedMemory -= bytes;
}
inline bool Purged(uint32_t start, uint32_t needed) {
    NiShadowManager* m = ms_pkShadowManager;
    return m->usedMemory <= m->memoryBudget && start - m->usedMemory >= needed;
}
} // namespace

// 0x758260: the default purge: available cube maps, then available shadow maps, then (when
// asked) active generators switched off one by one, until the memory is under budget with
// `needed` bytes freed
void __cdecl NiShadowManager_DefaultPurge(uint32_t needed, List* active, List* maps, List* cubes,
                                          int purgeActive) {
    uint32_t start = ms_pkShadowManager->usedMemory;
    while (cubes->count > 0) {
        PurgeCube(cubes);
        if (Purged(start, needed)) {
            return;
        }
    }
    while (maps->count > 0) {
        NiShadowManager_DestroyShadowMap(maps->head->element);
        if (Purged(start, needed)) {
            return;
        }
    }
    if (!re::Flag(purgeActive)) {
        return;
    }
    while (active->count > 0) {
        NiShadowGenerator_SetActive(active->head->element, 0, 0, 0);
        while (cubes->count > 0) {
            PurgeCube(cubes);
        }
        while (maps->count > 0) {
            NiShadowManager_DestroyShadowMap(maps->head->element);
        }
        if (Purged(start, needed)) {
            return;
        }
    }
}

// 0x7585F0: a shadow map pool node's constructor
uint8_t* __fastcall NiShadowMapNode_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 8) = 0x008FF368;   // NiShadowMap's vtable
    At<void*>(self, 0x10) = nullptr;
    At<void*>(self, 0x14) = nullptr;
    return self;
}

// 0x758610: RegisterShadowTechnique in an active slot and under an ID (both must be free: the
// error message is formatted and dropped)
uint8_t __cdecl NiShadowManager_RegisterShadowTechniqueImpl(uint8_t* technique, uint32_t slotArg, uint32_t idArg) {
    uint16_t slot = uint16_t(slotArg);
    uint16_t id = uint16_t(idArg);
    NiShadowManager* m = ms_pkShadowManager;
    char message[0x104];
    if (m->activeTechniques[slot] != nullptr) {
        NiSprintf(message, 0x104, reinterpret_cast<const char*>(0x00951508), At<char*>(technique, 4), uint32_t(slot),
                  At<char*>(m->activeTechniques[slot], 4));
        return 0;
    }
    if (m->knownTechniques[id] != nullptr) {
        // (the message names the technique under the slot's ID, as the game's does)
        NiSprintf(message, 0x104, reinterpret_cast<const char*>(0x00951468), At<char*>(technique, 4), uint32_t(slot),
                  At<char*>(m->knownTechniques[slot], 4));
        return 0;
    }
    m->activeTechniques[slot] = technique;
    ms_pkShadowManager->knownTechniques[id] = technique;
    At<uint16_t>(technique, 0x46) = id;
    At<uint16_t>(technique, 0x44) = slot;
    return 1;
}

// 0x758710: RemoveShadowGenerator (from the active list when active, and from the list)
uint8_t __cdecl NiShadowManager_RemoveShadowGenerator(NiRefObject* generator) {
    NiShadowGenerator_7735F0(generator);
    re::Ptr32<NiRefObject> removed;
    re::Ptr32<NiRefObject> held;
    if (At<uint8_t>(generator, 8) >> 3 & 1) {
        held = generator;
        Increment(&generator->refCount);
        List_RemoveRef(&ms_pkShadowManager->activeGenerators, &removed, &held);
        ReleaseRef(removed);
        ReleaseRef(held);
    }
    held = generator;
    Increment(&generator->refCount);
    List_RemoveRef(&ms_pkShadowManager->generators, &removed, &held);
    ReleaseRef(removed);
    ReleaseRef(held);
    return 1;
}

// 0x758800: ActivateShadowGenerator (on: into the active list when there's room; off: out)
uint8_t __cdecl NiShadowManager_ActivateShadowGenerator(NiRefObject* generator, int active) {
    re::Ptr32<NiRefObject> held = generator;
    if (generator != nullptr) {
        Increment(&generator->refCount);
    }
    if (re::Flag(active)) {
        NiShadowManager* m = ms_pkShadowManager;
        if (m->activeGenerators.count >= m->maxActiveGenerators) {
            ReleaseRef(generator);
            return 0;
        }
        Node* node = m->activeGenerators.head;
        while (node != nullptr && node->element != generator) {
            node = node->next;
        }
        if (node == nullptr) {
            List_AddTail(&m->activeGenerators, &held);
        }
    } else {
        re::Ptr32<NiRefObject> removed;
        List_RemoveRef(&ms_pkShadowManager->activeGenerators, &removed, &held);
        ReleaseRef(removed);
    }
    ReleaseRef(held);
    return 1;
}

// 0x7588F0 and 0x758AD0: a new block of pool nodes (operator new[] with the count before it,
// each node constructed); its first node links the blocks, the rest go on the free list
namespace {
template <uint32_t NodeSize>
void CreateFreeMem(uint32_t& blockSizeVar, re::Ptr32<Node>& freeList, re::Ptr32<Node>& blocks, void(__thiscall* construct)(void*)) {
    uint32_t count = blockSizeVar;
    uint64_t total = uint64_t(count) * NodeSize;
    uint32_t bytes = total >> 32 ? 0xFFFFFFFF : uint32_t(total);
    bytes = bytes > 0xFFFFFFFB ? 0xFFFFFFFF : bytes + 4;
    uint32_t* memory = static_cast<uint32_t*>(NiNewArray(bytes));
    uint8_t* array = nullptr;
    if (memory != nullptr) {
        *memory = count;
        array = reinterpret_cast<uint8_t*>(memory + 1);
        VectorConstructor(array, NodeSize, count, construct);
    }
    freeList = reinterpret_cast<Node*>(array);
    *reinterpret_cast<re::Ptr32<Node>*>(array + blockSizeVar * NodeSize - NodeSize) = nullptr;
    uint32_t i = 1;
    uint32_t offset = NodeSize;
    do {
        uint8_t* base = re::PtrCast<uint8_t*>(freeList);
        *reinterpret_cast<re::Ptr32<uint8_t>*>(base + offset) = base + offset + NodeSize;
        offset += NodeSize;
    } while (i++ < blockSizeVar - 2);
    if (blocks != nullptr) {
        freeList->next = blocks;
        blocks = freeList;
    } else {
        blocks = freeList;
        freeList->next = nullptr;
    }
    freeList = re::PtrCast<Node*>(re::PtrCast<uint8_t*>(freeList) + NodeSize);
}
} // namespace
void __cdecl NiShadowMapPool_CreateFreeMemImpl() {
    CreateFreeMem<0x18>(mapBlockSize, mapFreeList, mapBlocks,
                        reinterpret_cast<void(__thiscall*)(void*)>(0x007585F0));
}
void __cdecl NiShadowCubeMapPool_CreateFreeMemImpl() {
    CreateFreeMem<0x30>(cubeBlockSize, cubeFreeList, cubeBlocks,
                        reinterpret_cast<void(__thiscall*)(void*)>(0x007589C0));
}

// 0x7589C0: a shadow cube map pool node's constructor
uint8_t* __fastcall NiShadowCubeMapNode_Construct(uint8_t* self, re::Edx) {
    At<void*>(self, 0x10) = nullptr;
    At<void*>(self, 0x14) = nullptr;
    At<uint32_t>(self, 8) = 0x008FF374;   // NiShadowCubeMap's vtable
    for (int offset = 0x18; offset <= 0x2C; offset += 4) {
        At<void*>(self, offset) = nullptr;
    }
    return self;
}

// 0x7589F0 and 0x758ED0: a node from the pool (a new block when it's empty)
Node* __cdecl NiShadowMapPool_Allocate() {
    Lock(mapLock);
    if (mapFreeList == nullptr) {
        NiShadowMapPool_CreateFreeMem();
    }
    Node* node = mapFreeList;
    mapFreeList = node->next;
    node->next = nullptr;
    node->prev = nullptr;
    Unlock(mapLock);
    return node;
}
Node* __cdecl NiShadowCubeMapPool_Allocate() {
    Lock(cubeLock);
    if (cubeFreeList == nullptr) {
        NiShadowCubeMapPool_CreateFreeMem();
    }
    Node* node = cubeFreeList;
    cubeFreeList = node->next;
    node->next = nullptr;
    node->prev = nullptr;
    Unlock(cubeLock);
    return node;
}
RE_UNHOOKED_FN("NiShadowMapPool::Allocate (jump)", 0x00758BA0, &NiShadowMapPool_Allocate);
RE_UNHOOKED("NiShadowCubeMapPool::Allocate (jump)", 0x00758F40);

// 0x758A60: the cube map pool's blocks destroyed (each node's map, last first)
void __cdecl NiShadowCubeMapPool_FreeMemImpl() {
    uint8_t* block = re::PtrCast<uint8_t*>(cubeBlocks);
    while (block != nullptr) {
        int32_t count = *reinterpret_cast<int32_t*>(block - 4);
        uint8_t* next = *reinterpret_cast<re::Ptr32<uint8_t>*>(block);
        for (int32_t i = count - 1; i >= 0; i--) {
            NiShadowCubeMap_Destruct(block + i * 0x30 + 8);
        }
        NiDeleteArray(block - 4, 0x30);
        block = next;
    }
    cubeFreeList = nullptr;
    cubeBlocks = nullptr;
}

// 0x758BB0: ~NiShadowManager
void __fastcall NiShadowManager_DestructImpl(NiShadowManager* self, re::Edx) {
    List_RemoveAll(&self->generators);
    List_RemoveAll(&self->activeGenerators);
    NiShadowManager_DestroyAllShadowMaps();
    NiShadowManager_DestroyAllShadowCubeMaps();
    for (int i = 0; i < 16; i++) {
        if (self->knownTechniques[i] != nullptr) {
            Virtual<void>(self->knownTechniques[i], 1, 1);
        }
    }
    for (Node* block = mapBlocks; block != nullptr;) {
        Node* next = block->next;
        NiShadowMapArray_Destroy(block, 3);
        block = next;
    }
    mapBlocks = nullptr;
    mapFreeList = nullptr;
    NiShadowCubeMapPool_FreeMem();
    FreeAll(&self->renderClicks, 0x00952E10);
    FreeAll(&self->activeGenerators, 0x0095127C);
    FreeAll(&self->generators, 0x0095127C);
    ReleaseRef(self->clickValidator);
    ReleaseRef(self->spotSwap);
    ReleaseRef(self->pointSwap);
    ReleaseRef(self->directionalSwap);
    ReleaseRef(self->cullingProcess);
    ReleaseRef(self->sceneCamera);
    ClickGeneratorSet_RemoveAll(&self->clickGenerators);
    NiArrayFree(self->clickGenerators);
    FreeAll(&self->cubesAvailable, 0x00952E10);
    FreeAll(&self->cubePool, 0x00951440);
    FreeAll(&self->mapsAvailable, 0x00952E10);
    FreeAll(&self->mapPool, 0x00951434);
}

// 0x758E70: RegisterShadowClickGenerator (names must be unique)
uint8_t __cdecl NiShadowManager_RegisterShadowClickGeneratorImpl(NiRefObject* generator) {
    if (NiShadowManager_GetShadowClickGenerator(&At<char*>(generator, 0x18)) != nullptr) {
        return 0;
    }
    re::Ptr32<NiRefObject> held = generator;
    if (generator != nullptr) {
        Increment(&generator->refCount);
    }
    ClickGeneratorSet_Add(&ms_pkShadowManager->clickGenerators, &held);
    ReleaseRef(generator);
    return 1;
}

// 0x758F50: NiShadowManager's constructor (the shadow-write materials found or made, their
// swap processors, the click validator)
namespace {
// a material by name, if it's a fragment material
void* FindMaterial(const char* name) {
    re::Ptr32<char> handle = GlobalStringTable_AddString(name);
    void* material = IsKindOf(NiMaterial_GetMaterial(&handle), kNiFragmentMaterialClassIndex);
    FixedStringRelease(&handle);
    return material;
}
// NiMaterialSwapProcessor(material) (0x10 bytes)
NiRefObject* NewSwapProcessor(void* material) {
    uint32_t* swap = static_cast<uint32_t*>(NiNew(0x10));
    if (swap == nullptr) {
        return nullptr;
    }
    swap[0] = 0x008FEFD4;
    swap[1] = 0;
    Increment(&ms_uiObjects);
    swap[0] = 0x00917884;
    swap[2] = reinterpret_cast<uint32_t>(material);
    if (material != nullptr) {
        Increment(static_cast<uint8_t*>(material) + 4);
    }
    swap[3] = 0xFFFFFFFF;
    return reinterpret_cast<NiRefObject*>(swap);
}
} // namespace
NiShadowManager* __fastcall NiShadowManager_ConstructImpl(NiShadowManager* self, re::Edx) {
    auto initList = [](List& list, uintptr_t vtable) {
        list.count = 0;
        list.head = nullptr;
        list.tail = nullptr;
        list.vtable = reinterpret_cast<void*>(vtable);
    };
    initList(self->mapPool, 0x00951434);
    initList(self->mapsAvailable, 0x00952E10);
    initList(self->cubePool, 0x00951440);
    initList(self->cubesAvailable, 0x00952E10);
    self->clickGenerators = nullptr;
    self->clickGeneratorsAllocated = 0;
    self->clickGeneratorCount = 0;
    self->activeClickGenerator = -1;
    self->usedMemory = 0;
    self->purge = reinterpret_cast<PurgeFn>(0x00758260);
    self->maxActiveGenerators = 8;
    self->memoryBudget = 0x4000000;
    self->active = 1;
    self->supported = 0;
    self->sceneCamera = nullptr;
    self->cullingProcess = nullptr;
    self->directionalSwap = nullptr;
    self->pointSwap = nullptr;
    self->spotSwap = nullptr;
    self->clickValidator = nullptr;
    initList(self->generators, 0x0095127C);
    initList(self->activeGenerators, 0x0095127C);
    initList(self->renderClicks, 0x0095145C);
    NiRefObject* directionalLibrary = CreateDirectionalShadowWriteLibrary();
    if (directionalLibrary != nullptr) {
        Increment(&directionalLibrary->refCount);
    }
    NiRefObject* spotLibrary = CreateSpotShadowWriteLibrary();
    if (spotLibrary != nullptr) {
        Increment(&spotLibrary->refCount);
    }
    NiRefObject* pointLibrary = CreatePointShadowWriteLibrary();
    if (pointLibrary != nullptr) {
        Increment(&pointLibrary->refCount);
    }
    self->directionalMaterial = FindMaterial("NiDirectionalShadowWriteMaterial");
    if (self->directionalMaterial == nullptr) {
        void* memory = NiNew(0x3C4);
        self->directionalMaterial =
            memory != nullptr ? NiDirectionalShadowWriteMaterial_Construct(memory, directionalLibrary, 0) : nullptr;
    }
    self->spotMaterial = FindMaterial("NiSpotShadowWriteMaterial");
    if (self->spotMaterial == nullptr) {
        void* memory = NiNew(0x3C4);
        self->spotMaterial = memory != nullptr ? NiSpotShadowWriteMaterial_Construct(memory, spotLibrary, 0) : nullptr;
    }
    self->pointMaterial = FindMaterial("NiPointShadowWriteMaterial");
    if (self->pointMaterial == nullptr) {
        void* memory = NiNew(0x3C4);
        self->pointMaterial =
            memory != nullptr ? NiPointShadowWriteMaterial_Construct(memory, pointLibrary, 0) : nullptr;
    }
    for (re::Ptr32<void>& technique : self->activeTechniques) {
        technique = nullptr;
    }
    for (re::Ptr32<void>& technique : self->knownTechniques) {
        technique = nullptr;
    }
    AssignRef(self->directionalSwap, NewSwapProcessor(self->directionalMaterial));
    AssignRef(self->spotSwap, NewSwapProcessor(self->spotMaterial));
    AssignRef(self->pointSwap, NewSwapProcessor(self->pointMaterial));
    uint32_t* validator = static_cast<uint32_t*>(NiNew(0x20));
    if (validator != nullptr) {
        validator[0] = 0x008FEFD4;
        validator[1] = 0;
        Increment(&ms_uiObjects);
        validator[0] = 0x0095144C;
        validator[2] = 0;
        validator[3] = 0;
        validator[4] = 0;
        validator[5] = 0;
        validator[6] = 0;
        validator[7] = 0x400;
        NiVisibleArray_SetAllocatedSize(validator + 4, 0x400);
    }
    AssignRef(self->clickValidator, reinterpret_cast<NiRefObject*>(validator));
    ReleaseRef(pointLibrary);
    ReleaseRef(spotLibrary);
    ReleaseRef(directionalLibrary);
    return self;
}

// 0x759570: NiShadowManager::Initialize: the manager, the three standard techniques, the
// default click generator (made active), NiShadowGenerator's stream loader
uint32_t __cdecl NiShadowManager_Initialize() {
    void* memory = NiNew(sizeof(NiShadowManager));
    ms_pkShadowManager = memory != nullptr ? NiShadowManager_Construct(memory) : nullptr;
    struct TechniqueNames {
        const char* name;
        const char* strings[6];   // in the order the game makes them
    };
    static const TechniqueNames kTechniques[3] = {
        {"NiStandardShadowTechnique",
         {"WriteDepthToColor", "WriteDepthToColor", "WriteDepthToColor", "SpotShadowMap", "ShadowCubeMap",
          "ShadowMap"}},
        {"NiPCFShadowTechnique",
         {"WriteDepthToColor", "WriteDepthToColor", "WriteDepthToColor", "SpotPCFShadowMap", "ShadowCubeMap",
          "PCFShadowMap"}},
        {"NiVSMShadowTechnique",
         {"WriteVSMDepthToColor", "WriteDepthToColor", "WriteVSMDepthToColor", "SpotVSMShadowMap", "ShadowCubeMap",
          "VSMShadowMap"}},
    };
    for (uint32_t t = 0; t < 3; t++) {
        bool vsm = t == 2;
        uint8_t* technique = static_cast<uint8_t*>(NiNew(vsm ? 0x6C : 0x4C));
        if (technique != nullptr) {
            re::Ptr32<char> name = GlobalStringTable_AddString(kTechniques[t].name);
            char* s[6];
            for (int i = 0; i < 6; i++) {
                s[i] = GlobalStringTable_AddString(kTechniques[t].strings[i]);
            }
            // (the strings go by value, the last made first)
            technique = static_cast<uint8_t*>(
                vsm ? NiVSMShadowTechnique_Construct(technique, &name, s[5], s[4], s[3], s[2], s[1], s[0], 1, 0)
                    : NiShadowTechnique_Construct(technique, &name, s[5], s[4], s[3], s[2], s[1], s[0], 1, 0));
            FixedStringRelease(&name);
        }
        if (vsm) {
            At<uint32_t>(technique, 0x38) = 0xD;
            At<uint32_t>(technique, 0x3C) = 3;
            At<uint32_t>(technique, 0x40) = 2;
        }
        SetFloatBits(technique + 0x34, 0x38D1B717);   // 1e-4
        SetFloatBits(technique + 0x28, 0);
        SetFloatBits(technique + 0x2C, 0x38D1B717);
        SetFloatBits(technique + 0x20, 0);
        SetFloatBits(technique + 0x30, 0x3F75C28F);   // 0.96
        SetFloatBits(technique + 0x24, 0x3F7AE148);   // 0.98
        NiShadowManager_RegisterShadowTechnique(technique, t, t);
    }
    void* generatorMemory = NiNew(0x60);
    NiRefObject* generator =
        generatorMemory != nullptr ? NiDefaultShadowClickGenerator_Construct(generatorMemory) : nullptr;
    NiShadowManager_RegisterShadowClickGenerator(generator);
    re::Ptr32<char> name = GlobalStringTable_AddString("NiDefaultShadowClickGenerator");
    NiShadowManager_SetActiveShadowClickGenerator(&name);
    FixedStringRelease(&name);
    return NiStream_RegisterLoader("NiShadowGenerator", reinterpret_cast<void*>(0x00773340));
}

// 0x759930: NiShadowManager::Shutdown
void __cdecl NiShadowManager_Shutdown() {
    if (ms_pkShadowManager == nullptr) {
        return;
    }
    NiStream_UnregisterLoader("NiShadowGenerator");
    NiShadowManager* m = ms_pkShadowManager;
    if (m != nullptr) {
        NiShadowManager_Destruct(m);
        NiDelete(m, sizeof(NiShadowManager));
    }
    ms_pkShadowManager = nullptr;
}

// 0x759EC0 and 0x759EE0: NiStream::RegisterLoader / UnregisterLoader (the loaders' map)
uint32_t __cdecl NiStream_RegisterLoaderImpl(const char* name, void* create) {
    LoaderMap_SetAt(ms_kLoaders, name, create);
    return 0;
}
uint32_t __cdecl NiStream_UnregisterLoaderImpl(const char* name) {
    return LoaderMap_RemoveAt(ms_kLoaders, name);
}

#define NISHADOW_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISHADOW_REVERSED("NiShadowMap::Shutdown", 0x00756120, NiShadowMap_ShutdownImpl);
NISHADOW_REVERSED("NiShadowMap::Initialize", 0x007562C0, NiShadowMap_InitializeImpl);
NISHADOW_REVERSED("NiShadowManager::GetTextureSize", 0x00756E20, NiShadowManager_GetTextureSizeImpl);
NISHADOW_REVERSED("NiTPointerList::RemoveTail", 0x00756EB0, List_RemoveTailImpl);
NISHADOW_REVERSED("NiTPointerList::RemovePos", 0x00756EF0, List_RemovePosImpl);
NISHADOW_REVERSED("NiTPointerList<NiPointer>::RemoveHead", 0x00756F80, List_RemoveHeadRef);
NISHADOW_REVERSED("NiTPointerList<NiPointer>::RemoveTail", 0x00756FD0, List_RemoveTailRef);
NISHADOW_REVERSED("NiShadowMapPool::Deallocate", 0x00757020, NiShadowMapPool_DeallocateImpl);
NISHADOW_REVERSED("NiShadowMapList::FreeNode", 0x00757070, NiShadowMapList_FreeNode);
NISHADOW_REVERSED("NiShadowCubeMapPool::Deallocate", 0x00757080, NiShadowCubeMapPool_DeallocateImpl);
NISHADOW_REVERSED("NiShadowCubeMapList::FreeNode", 0x007570D0, NiShadowCubeMapList_FreeNode);
NISHADOW_REVERSED("NiShadowMapList::Destroy", 0x007570E0, NiShadowMapList_Destroy);
NISHADOW_REVERSED("NiShadowCubeMapList::Destroy", 0x00757110, NiShadowCubeMapList_Destroy);
NISHADOW_REVERSED("NiShadowClickValidator::GetRTTI", 0x00757140, NiShadowClickValidator_GetRTTI);
NISHADOW_REVERSED("NiShadowClickValidator::~NiShadowClickValidator", 0x00757150, NiShadowClickValidator_DestructImpl);
NISHADOW_REVERSED("NiShadowManager::GenerateActiveClicks", 0x007571B0, NiShadowManager_GenerateActiveClicks);
NISHADOW_REVERSED("NiShadowManager::GetShadowTechnique", 0x007571E0, NiShadowManager_GetShadowTechnique);
NISHADOW_REVERSED("NiShadowManager::GetShadowClickGenerator", 0x00757260, NiShadowManager_GetShadowClickGeneratorImpl);
NISHADOW_REVERSED("NiShadowManager::PrepareGenerators", 0x007572A0, NiShadowManager_PrepareGeneratorsImpl);
NISHADOW_REVERSED("NiShadowManager::GenerateRenderClicks", 0x007572F0, NiShadowManager_GenerateRenderClicks);
NISHADOW_REVERSED("NiShadowManager::SetMaxActiveShadowGenerators", 0x00757340, NiShadowManager_SetMaxActiveShadowGenerators);
NISHADOW_REVERSED("NiShadowManager::SetActive", 0x007573A0, NiShadowManager_SetActiveImpl);
NISHADOW_REVERSED("NiShadowManager::UpdateActiveGenerators", 0x00757410, NiShadowManager_UpdateActiveGenerators);
NISHADOW_REVERSED("NiTList<NiShadowMap>::Remove", 0x00757440, PoolList_RemoveImpl);
NISHADOW_REVERSED("NiShadowClickValidator::Destroy", 0x00757490, NiShadowClickValidator_Destroy);
NISHADOW_REVERSED("NiShadowManager::ValidateShaderVersions", 0x007574B0, NiShadowManager_ValidateShaderVersions);
NISHADOW_REVERSED("NiShadowManager::AddShadowGenerator", 0x007574F0, NiShadowManager_AddShadowGenerator);
NISHADOW_REVERSED("NiShadowManager::CreateShadowMap", 0x007575D0, NiShadowManager_CreateShadowMap);
NISHADOW_REVERSED("NiShadowManager::DestroyShadowMap", 0x00757780, NiShadowManager_DestroyShadowMapImpl);
NISHADOW_REVERSED("NiShadowManager::DestroyAllShadowMaps", 0x00757850, NiShadowManager_DestroyAllShadowMapsImpl);
NISHADOW_REVERSED("NiShadowManager::CreateShadowCubeMap", 0x00757980, NiShadowManager_CreateShadowCubeMap);
NISHADOW_REVERSED("NiShadowManager::DestroyAllShadowCubeMaps", 0x00757AD0, NiShadowManager_DestroyAllShadowCubeMapsImpl);
NISHADOW_REVERSED("NiShadowManager::FindAvailableShadowMap", 0x00757C10, NiShadowManager_FindAvailableShadowMap);
NISHADOW_REVERSED("NiShadowManager::FindAvailableShadowCubeMap", 0x00757CF0, NiShadowManager_FindAvailableShadowCubeMap);
NISHADOW_REVERSED("NiShadowManager::SetActiveShadowClickGenerator", 0x00757DA0, NiShadowManager_SetActiveShadowClickGeneratorImpl);
NISHADOW_REVERSED("NiShadowManager::SetSceneCamera", 0x00757DF0, NiShadowManager_SetSceneCamera);
NISHADOW_REVERSED("NiShadowManager::SetCullingProcess", 0x00757E90, NiShadowManager_SetCullingProcess);
NISHADOW_REVERSED("NiShadowManager::ReleaseShadowMap", 0x00758020, NiShadowManager_ReleaseShadowMap);
NISHADOW_REVERSED("NiShadowManager::ReleaseShadowCubeMap", 0x007580B0, NiShadowManager_ReleaseShadowCubeMap);
NISHADOW_REVERSED("NiShadowManager::FindDepthBuffer", 0x00758180, NiShadowManager_FindDepthBufferImpl);
NISHADOW_REVERSED("NiShadowManager::DefaultPurge", 0x00758260, NiShadowManager_DefaultPurge);
NISHADOW_REVERSED("NiShadowMapNode::NiShadowMapNode", 0x007585F0, NiShadowMapNode_Construct);
NISHADOW_REVERSED("NiShadowManager::RegisterShadowTechnique", 0x00758610, NiShadowManager_RegisterShadowTechniqueImpl);
NISHADOW_REVERSED("NiShadowManager::RemoveShadowGenerator", 0x00758710, NiShadowManager_RemoveShadowGenerator);
NISHADOW_REVERSED("NiShadowManager::ActivateShadowGenerator", 0x00758800, NiShadowManager_ActivateShadowGenerator);
NISHADOW_REVERSED("NiShadowMapPool::CreateFreeMem", 0x007588F0, NiShadowMapPool_CreateFreeMemImpl);
NISHADOW_REVERSED("NiShadowCubeMapNode::NiShadowCubeMapNode", 0x007589C0, NiShadowCubeMapNode_Construct);
NISHADOW_REVERSED("NiShadowMapPool::Allocate", 0x007589F0, NiShadowMapPool_Allocate);
NISHADOW_REVERSED("NiShadowCubeMapPool::FreeMem", 0x00758A60, NiShadowCubeMapPool_FreeMemImpl);
NISHADOW_REVERSED("NiShadowCubeMapPool::CreateFreeMem", 0x00758AD0, NiShadowCubeMapPool_CreateFreeMemImpl);
NISHADOW_REVERSED("NiShadowManager::~NiShadowManager", 0x00758BB0, NiShadowManager_DestructImpl);
NISHADOW_REVERSED("NiShadowManager::RegisterShadowClickGenerator", 0x00758E70, NiShadowManager_RegisterShadowClickGeneratorImpl);
NISHADOW_REVERSED("NiShadowCubeMapPool::Allocate", 0x00758ED0, NiShadowCubeMapPool_Allocate);
NISHADOW_REVERSED("NiShadowManager::NiShadowManager", 0x00758F50, NiShadowManager_ConstructImpl);
NISHADOW_REVERSED("NiShadowManager::Initialize", 0x00759570, NiShadowManager_Initialize);
NISHADOW_REVERSED("NiShadowManager::Shutdown", 0x00759930, NiShadowManager_Shutdown);
NISHADOW_REVERSED("NiStream::RegisterLoader", 0x00759EC0, NiStream_RegisterLoaderImpl);
NISHADOW_REVERSED("NiStream::UnregisterLoader", 0x00759EE0, NiStream_UnregisterLoaderImpl);

} // namespace ni
