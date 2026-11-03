// NiShadowGenerator (src/gamebryo/NiShadowGenerator.h): the caster and receiver geometry lists
// (the effect's affected nodes' geometry less what's under the unaffected nodes), the shadow
// maps' assignment, dirty marking and release, activation, streaming, cloning, comparison.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDynamicEffect.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiShadowGenerator.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiShadowGenerator_RTTI, 0x020E2C34);

constexpr uintptr_t NiShadowGenerator_vtable = 0x00952E20;
constexpr uintptr_t NodeList_vtable = 0x0095145C;
constexpr uintptr_t ListBase_vtable = 0x00952E10;
constexpr uintptr_t Allocator_vtable = 0x0099C748;
constexpr uintptr_t NiShadowCubeMap_RTTI = 0x020E257C;   // its class index at +0x8

// Bully's class indices (the bit each class has in its RTTI's set, +0xC)
GAME_VAR(uint32_t, NiNode_ClassIndex, 0x020E2550);
GAME_VAR(uint32_t, NiShadowCubeMap_ClassIndex, 0x020E2584);
GAME_VAR(uint32_t, NiGeometry_ClassIndex, 0x020E25BC);
GAME_VAR(uint32_t, Caster_ClassIndex, 0x020E2FDC);   // the casting geometry's class

GAME_VAR(uint8_t*, NiShadowManager_ms_pkShadowManager, 0x020E2410);

using NodeList = NiTPointerList<NiNode*>;
using ListNode = NiTListNode<NiNode*>;

GAME_FN(void, __thiscall, Object_DoNothing, 0x008A20A0, void* self, void* stream);
GAME_FN(void, __thiscall, List_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, List_RemovePointer, 0x0075F630, void* list, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, GeometryList_AddTail, 0x00748170, void* list, const re::Ptr32<void>* element);
GAME_FN(void*, __cdecl, ShadowManager_GetShadowTechnique, 0x007571E0, const re::Ptr32<const char>* name);
GAME_FN(void, __cdecl, ShadowManager_AddShadowGenerator, 0x007574F0, NiShadowGenerator* generator);
GAME_FN(uint8_t, __cdecl, ShadowManager_ActivateShadowGenerator, 0x00758800, NiShadowGenerator* generator,
        int active);
GAME_FN(void, __cdecl, ShadowManager_ReleaseShadowMap, 0x00758020, void* map);
GAME_FN(void, __cdecl, ShadowManager_ReleaseShadowCubeMap, 0x007580B0, void* map);
// (this chunk's, called where modes apply)
GAME_FN(void, __thiscall, DynamicEffect_SetShadowGenerator, 0x00771990, NiDynamicEffect* effect,
        NiShadowGenerator* generator);
GAME_FN(void*, __thiscall, List_RemovePos, 0x00771DA0, void* list, re::Ptr32<void>* position);
GAME_FN(void*, __cdecl, ShadowMap_IsKindOf, 0x00772620, const void* rtti, void* map);
GAME_FN(void, __cdecl, ShadowCubeMap_SetGenerator, 0x00772660, void* cube, NiShadowGenerator* generator);
GAME_FN(void, __thiscall, Generator_AttachUnaffectedCasterNode, 0x00772690, NiShadowGenerator* self, NiNode* node);
GAME_FN(void, __thiscall, Generator_AttachUnaffectedReceiverNode, 0x00772700, NiShadowGenerator* self,
        NiNode* node);
GAME_FN(uint8_t, __thiscall, Generator_SubtreeContains, 0x00772770, NiShadowGenerator* self, NiNode* node,
        void* geometry);
GAME_FN(void, __thiscall, Generator_AddCasterGeometry, 0x00772860, NiShadowGenerator* self, NiNode* node,
        void* list);
GAME_FN(void, __thiscall, Generator_MarkShadowMapsDirty, 0x00772930, NiShadowGenerator* self);
GAME_FN(void, __thiscall, Generator_RemoveGeometry, 0x00773220, NiShadowGenerator* self, NiNode* node, void* list);
GAME_FN(void, __thiscall, Generator_GetReceiverGeometryList, 0x007733E0, NiShadowGenerator* self, void* list);
GAME_FN(void, __thiscall, Generator_ForceMaterialUpdate, 0x00773460, NiShadowGenerator* self);
GAME_FN(void, __thiscall, Generator_ReleaseAllShadowMaps, 0x007735F0, NiShadowGenerator* self);
GAME_FN(void, __thiscall, Generator_Destruct, 0x007737A0, NiShadowGenerator* self);

// ---- helpers

inline bool HasClassBit(const void* rtti, uint32_t index) {
    return (At<uint32_t>(rtti, 0xC + (index >> 5) * 4) & (1u << (index & 31))) != 0;
}
// NiIsKindOf on an NiObject (its RTTI from slot 2)
inline void* KindOf(void* object, uint32_t index) {
    return HasClassBit(Virtual<const void*>(object, 2), index) ? object : nullptr;
}
// a node's child i, null past the end
inline NiAVObject* ChildAt(NiNode* node, uint32_t i) {
    return i < node->children.size ? node->children.base[i] : nullptr;
}

inline ListNode* NewNode(NodeList* list) {
    return Virtual<ListNode*>(list, 1);
}
inline bool Contains(const NodeList* list, NiNode* node) {
    for (ListNode* n = list->head; n != nullptr; n = n->next) {
        if (n->element == node) {
            return true;
        }
    }
    return false;
}
inline void AddHead(NodeList* list, NiNode* node) {
    ListNode* n = NewNode(list);
    n->element = node;
    n->prev = nullptr;
    n->next = list->head;
    if (list->head != nullptr) {
        list->head->prev = n;
    } else {
        list->tail = n;
    }
    list->count++;
    list->head = n;
}
inline void AddTail(NodeList* list, NiNode* node) {
    ListNode* n = NewNode(list);
    n->element = node;
    n->next = nullptr;
    n->prev = list->tail;
    if (list->tail != nullptr) {
        list->tail->next = n;
    } else {
        list->head = n;
    }
    list->count++;
    list->tail = n;
}
inline void DestroyList(NodeList* list) {
    list->vtable = reinterpret_cast<void*>(ListBase_vtable);
    List_RemoveAll(list);
    list->vtable = reinterpret_cast<void*>(Allocator_vtable);
}

// the shadow maps through the callbacks (none without them)
inline uint32_t MapCount(NiShadowGenerator* self) {
    return self->shadowMapCount != nullptr ? self->shadowMapCount(self) : 0;
}
inline uint8_t* MapAt(NiShadowGenerator* self, uint32_t index) {
    return self->retrieveShadowMap != nullptr ? static_cast<uint8_t*>(self->retrieveShadowMap(self, index))
                                              : nullptr;
}
// a cube map's six face clicks (+0x10), the map made a cube map by its class bit (slot 0
// GetRTTI: NiShadowMap's own root)
inline re::Ptr32<uint8_t>* CubeClicks(uint8_t* map) {
    uint8_t* cube = HasClassBit(Virtual<const void*>(map, 0), NiShadowCubeMap_ClassIndex) ? map : nullptr;
    return reinterpret_cast<re::Ptr32<uint8_t>*>(cube + 0x10);
}

struct MapItem {   // (the game's layouts: 0xC, the map's first 0xC)
    re::Ptr32<MapItem> next;
    re::Ptr32<void> key;
    re::Ptr32<void> value;
};
static_assert(sizeof(MapItem) == 0xC);
struct PointerMap {
    re::Ptr32<void> vtable;
    uint32_t hashSize;
    re::Ptr32<re::Ptr32<MapItem>> table;
};
inline bool MapLookup(const PointerMap* map, void* key, re::Ptr32<void>& value) {
    for (MapItem* item = map->table[uintptr_t(key) % map->hashSize]; item != nullptr; item = item->next) {
        if (item->key == key) {
            value = item->value;
            return true;
        }
    }
    return false;
}

// ---- NiShadowGenerator

// GetShadowMapCount (the count callback's)
uint32_t __fastcall GetShadowMapCount(NiShadowGenerator* self, re::Edx) {
    return MapCount(self);
}

// RetrieveShadowMap(uiIndex)
void* __fastcall RetrieveShadowMap(NiShadowGenerator* self, re::Edx, uint32_t index) {
    return MapAt(self, index);
}

// NiTPointerList's deleting destructor (vtable 0x952E10)
NodeList* __fastcall ListDeletingDestructor(NodeList* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(ListBase_vtable);
    List_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(Allocator_vtable);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// vf6 RegisterStreamables: the technique's name
uint8_t __fastcall RegisterStreamables(NiShadowGenerator* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    return Virtual<uint8_t>(stream, 8, reinterpret_cast<re::Ptr32<const char>*>(static_cast<uint8_t*>(self->technique) + 4));
}

// both lists' elements the same, pairwise (the shorter one's length; the counts checked first)
inline bool ListsEqual(const NodeList* a, const NodeList* b) {
    ListNode* x = a->head;
    ListNode* y = b->head;
    while (x != nullptr && y != nullptr) {
        NiNode* ex = x->element;
        x = x->next;
        NiNode* ey = y->element;
        y = y->next;
        if (ex != ey) {
            return false;
        }
    }
    return true;
}

// vf8 IsEqual: the effect, the technique, the lists
uint8_t __fastcall IsEqual(NiShadowGenerator* self, re::Edx, NiShadowGenerator* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    if (other->dynamicEffect != self->dynamicEffect || other->technique != self->technique ||
        other->unaffectedCasters.count != self->unaffectedCasters.count ||
        other->unaffectedReceivers.count != self->unaffectedReceivers.count) {
        return 0;
    }
    if (!ListsEqual(&self->unaffectedCasters, &other->unaffectedCasters) ||
        !ListsEqual(&self->unaffectedReceivers, &other->unaffectedReceivers)) {
        return 0;
    }
    return 1;
}

// NiDynamicCast for the shadow maps (their RTTI from slot 0): the map when it's of the class
void* __cdecl ShadowMapIsKindOf(const void* rtti, void* map) {
    if (map == nullptr) {
        return nullptr;
    }
    return HasClassBit(Virtual<const void*>(map, 0), At<uint32_t>(rtti, 8)) ? map : nullptr;
}

// a cube map's six clicks given the generator
void __cdecl ShadowCubeMapSetGenerator(uint8_t* cube, NiShadowGenerator* generator) {
    for (int i = 0; i < 6; i++) {
        uint8_t* click = At<uint8_t*>(cube, 0x10 + i * 4);
        if (click != nullptr) {
            At<NiShadowGenerator*>(click, 0x80) = generator;
        }
    }
}

void __fastcall AttachUnaffectedCasterNode(NiShadowGenerator* self, re::Edx, NiNode* node) {
    if (!Contains(&self->unaffectedCasters, node)) {
        AddHead(&self->unaffectedCasters, node);
    }
}

void __fastcall AttachUnaffectedReceiverNode(NiShadowGenerator* self, re::Edx, NiNode* node) {
    if (!Contains(&self->unaffectedReceivers, node)) {
        AddHead(&self->unaffectedReceivers, node);
    }
}

// whether the geometry is under the node (the children's count taken once)
uint8_t __fastcall SubtreeContains(NiShadowGenerator* self, re::Edx, NiNode* node, void* geometry) {
    if (node == nullptr) {
        return 0;
    }
    uint32_t count = node->children.size;
    for (uint32_t i = 0; i < count; i++) {
        NiAVObject* child = ChildAt(node, i);
        void* asGeometry = child != nullptr ? KindOf(child, NiGeometry_ClassIndex) : nullptr;
        if (asGeometry == geometry) {
            return 1;
        }
        child = ChildAt(node, i);
        if (child != nullptr) {
            void* asNode = KindOf(child, NiNode_ClassIndex);
            if (asNode != nullptr && Generator_SubtreeContains(self, static_cast<NiNode*>(asNode), geometry)) {
                return 1;
            }
        }
    }
    return 0;
}

// the casting geometry under a node (not app-culled, not flag 0x200) added to the list's tail
void __fastcall AddCasterGeometry(NiShadowGenerator* self, re::Edx, NiNode* node, void* list) {
    if (node == nullptr || (node->flags & 1) || ((node->flags >> 9) & 1)) {
        return;
    }
    uint32_t count = node->children.size;
    for (uint32_t i = 0; i < count; i++) {
        NiAVObject* child = ChildAt(node, i);
        if (child == nullptr) {
            continue;
        }
        if (HasClassBit(Virtual<const void*>(child, 2), Caster_ClassIndex)) {
            re::Ptr32<void> element = child;
            GeometryList_AddTail(list, &element);
        } else if (child->flags & 0x100) {
            Generator_AddCasterGeometry(self, static_cast<NiNode*>(static_cast<NiAVObject*>(child)), list);
        }
    }
}

// MarkShadowMapsDirty: every map's click(s) to be rendered again
void __fastcall MarkShadowMapsDirty(NiShadowGenerator* self, re::Edx) {
    self->flags |= 1;
    uint32_t count = MapCount(self);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* map = MapAt(self, i);
        if ((map[4] & 0xF) == 0) {
            uint8_t* click = At<uint8_t*>(map, 0xC);
            if (click != nullptr) {
                click[0x84] |= 0x10;
            }
        } else {
            re::Ptr32<uint8_t>* clicks = CubeClicks(map);
            for (int face = 0; face < 6; face++) {
                if (clicks[face] != nullptr) {
                    clicks[face][0x84] |= 0x10;
                }
            }
        }
    }
}

// GetShadowMapsDirty: still dirty while any click waits to be rendered (no maps: dirty); the
// flag cleared once none does
uint8_t __fastcall GetShadowMapsDirty(NiShadowGenerator* self, re::Edx) {
    if ((self->flags & 1) == 0) {
        return 0;
    }
    if (self->shadowMapCount == nullptr) {
        return 1;
    }
    uint32_t count = self->shadowMapCount(self);
    if (count == 0) {
        return 1;
    }
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* map = MapAt(self, i);
        if ((map[4] & 0xF) == 0) {
            uint8_t* click = At<uint8_t*>(map, 0xC);
            if (click != nullptr && (click[0x84] & 0x10)) {
                return 1;
            }
        } else {
            re::Ptr32<uint8_t>* clicks = CubeClicks(map);
            for (int face = 0; face < 6; face++) {
                if (clicks[face] != nullptr && (clicks[face][0x84] & 0x10)) {
                    return 1;
                }
            }
        }
    }
    self->flags &= 0xFFFE;
    return 0;
}

// vf11 ProcessClone: (unless the copy type is 0) the clone's lists from this one's, from the
// tails back, each node mapped to its clone (unmapped ones kept, unless the copy type is 1)
void __fastcall ProcessClone(NiShadowGenerator* self, re::Edx, void* cloning) {
    re::Ptr32<void> found = nullptr;
    NiShadowGenerator* clone = nullptr;
    if (MapLookup(static_cast<PointerMap*>(*static_cast<const re::Ptr32<PointerMap>*>(cloning)), self, found)) {
        clone = static_cast<NiShadowGenerator*>(found);
    }
    if (At<uint32_t>(cloning, 0x10) == 0) {
        return;
    }
    for (ListNode* n = self->unaffectedCasters.tail; n != nullptr;) {
        void* node = n->element;
        n = n->prev;
        re::Ptr32<void> mapped;
        if (MapLookup(static_cast<PointerMap*>(*static_cast<const re::Ptr32<PointerMap>*>(cloning)), node, mapped)) {
            node = mapped;
        } else if (At<uint32_t>(cloning, 0x10) == 1) {
            continue;
        }
        if (node != nullptr) {
            Generator_AttachUnaffectedCasterNode(clone, static_cast<NiNode*>(node));
        }
    }
    for (ListNode* n = self->unaffectedReceivers.tail; n != nullptr;) {
        void* node = n->element;
        n = n->prev;
        re::Ptr32<void> mapped;
        if (MapLookup(static_cast<PointerMap*>(*static_cast<const re::Ptr32<PointerMap>*>(cloning)), node, mapped)) {
            node = mapped;
        } else if (At<uint32_t>(cloning, 0x10) == 1) {
            continue;
        }
        if (node != nullptr) {
            Generator_AttachUnaffectedReceiverNode(clone, static_cast<NiNode*>(node));
        }
    }
}

// vf4 LoadBinary: the technique by name (unknown: the manager's first active one), the flags,
// the lists' and the effect's links, the depth bias, the size hint, and from 20.3.0.7 three more
void __fastcall LoadBinary(NiShadowGenerator* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    NiFixedString name = NullFixedString;
    Stream_LoadFixedString(stream, &name);
    LoadBytes(stream, &self->flags, 2);
    void* technique = ShadowManager_GetShadowTechnique(&name);
    self->technique = technique;
    if (technique == nullptr) {
        re::Ptr32<void>* active = re::PtrCast<re::Ptr32<void>*>(NiShadowManager_ms_pkShadowManager);
        for (uint32_t i = 0; i < 4; i++) {
            if (active[uint16_t(i)] != nullptr) {
                self->technique = active[uint16_t(i)];
                break;
            }
        }
    }
    Stream_ReadMultipleLinkIDs(stream);
    Stream_ReadMultipleLinkIDs(stream);
    Stream_ReadLinkID(stream);
    LoadBytes(stream, &self->depthBias, 4);
    LoadBytes(stream, &self->sizeHint, 2);
    if (StreamVersion(stream) > 0x14030006) {
        LoadBytes(stream, &self->unknown58, 4);
        LoadBytes(stream, &self->unknown5C, 4);
        LoadBytes(stream, &self->unknown60, 4);
    }
    FixedStringRelease(&name);
}

// vf5 LinkObject: the lists (each at the tail), the effect; then given to the shadow manager
void __fastcall LinkObject(NiShadowGenerator* self, re::Edx, void* stream) {
    Object_DoNothing(self, stream);
    for (uint32_t count = Stream_GetNumberOfLinkIDs(stream); count != 0; count--) {
        NiNode* node = static_cast<NiNode*>(Stream_ResolveLinkID(stream));
        if (node != nullptr) {
            AddTail(&self->unaffectedCasters, node);
        }
    }
    for (uint32_t count = Stream_GetNumberOfLinkIDs(stream); count != 0; count--) {
        NiNode* node = static_cast<NiNode*>(Stream_ResolveLinkID(stream));
        if (node != nullptr) {
            AddTail(&self->unaffectedReceivers, node);
        }
    }
    self->dynamicEffect = static_cast<NiDynamicEffect*>(Stream_ResolveLinkID(stream));
    ShadowManager_AddShadowGenerator(self);
}

// vf7 SaveBinary
void __fastcall SaveBinary(NiShadowGenerator* self, re::Edx, void* stream) {
    Object_DoNothing(self, stream);
    Stream_SaveFixedString(stream, reinterpret_cast<re::Ptr32<const char>*>(static_cast<uint8_t*>(self->technique) + 4));
    SaveBytes(stream, &self->flags, 2);
    uint32_t count = self->unaffectedCasters.count;
    SaveBytes(stream, &count, 4);
    for (ListNode* n = self->unaffectedCasters.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        Virtual<void>(stream, 12, node);   // SaveLinkID
    }
    count = self->unaffectedReceivers.count;
    SaveBytes(stream, &count, 4);
    for (ListNode* n = self->unaffectedReceivers.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        Virtual<void>(stream, 12, node);
    }
    Virtual<void>(stream, 12, self->dynamicEffect);
    SaveBytes(stream, &self->depthBias, 4);
    SaveBytes(stream, &self->sizeHint, 2);
    SaveBytes(stream, &self->unknown58, 4);
    SaveBytes(stream, &self->unknown5C, 4);
    SaveBytes(stream, &self->unknown60, 4);
}

const RTTI* __fastcall GetRTTI(NiShadowGenerator*, re::Edx) {
    return &NiShadowGenerator_RTTI;
}

// NiShadowGenerator(pkDynEffect) (the flags word not cleared first: its upper bits stay as the
// memory had them)
NiShadowGenerator* __fastcall Construct(NiShadowGenerator* self, re::Edx, NiDynamicEffect* effect) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiShadowGenerator_vtable);
    self->technique = nullptr;
    self->unaffectedCasters.count = 0;
    self->unaffectedCasters.head = nullptr;
    self->unaffectedCasters.tail = nullptr;
    self->unaffectedCasters.vtable = reinterpret_cast<void*>(NodeList_vtable);
    self->unaffectedReceivers.vtable = reinterpret_cast<void*>(NodeList_vtable);
    self->unaffectedReceivers.count = 0;
    self->unaffectedReceivers.head = nullptr;
    self->unaffectedReceivers.tail = nullptr;
    self->depthBias = 0.0f;
    self->flags |= 2;
    self->unknown58 = 0.0f;
    self->dynamicEffect = effect;
    self->unknown5C = 0.0f;
    self->unknown60 = 0.0f;
    self->callback30 = nullptr;
    self->assignShadowMap = nullptr;
    self->retrieveShadowMap = nullptr;
    self->shadowMapCount = nullptr;
    self->callback40 = nullptr;
    self->callback44 = nullptr;
    self->cleanup = nullptr;
    self->extraData = nullptr;
    self->sizeHint = 0x400;
    if (((self->flags >> 2) & 1) == 0) {
        Generator_MarkShadowMapsDirty(self);
    }
    uint16_t flags = uint16_t((self->flags & 0xFFDB) | 0x3D0);
    self->flags = flags;
    if (self->dynamicEffect->on != 0) {
        self->flags = uint16_t(flags | 8);
    } else {
        self->flags = uint16_t(flags & 0xFFF7);
    }
    return self;
}

// IsUnaffectedReceiverNode-style test: whether the geometry is under one of the unaffected
// receiver nodes
uint8_t __fastcall IsUnaffectedReceiverGeometry(NiShadowGenerator* self, re::Edx, void* geometry) {
    if (geometry == nullptr) {
        return 0;
    }
    for (ListNode* n = self->unaffectedReceivers.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        if (Generator_SubtreeContains(self, node, geometry)) {
            return 1;
        }
    }
    return 0;
}

// the geometry under a node removed from the list (the node's children counted once)
void __fastcall RemoveGeometry(NiShadowGenerator* self, re::Edx, NiNode* node, void* list) {
    if (node == nullptr) {
        return;
    }
    uint32_t count = node->children.size;
    for (uint32_t i = 0; i < count; i++) {
        NiAVObject* child = ChildAt(node, i);
        if (child == nullptr) {
            continue;
        }
        if (HasClassBit(Virtual<const void*>(child, 2), NiGeometry_ClassIndex)) {
            re::Ptr32<void> element = child;
            List_RemovePointer(list, &element);
        } else if (child->flags & 0x100) {
            Generator_RemoveGeometry(self, static_cast<NiNode*>(static_cast<NiAVObject*>(child)), list);
        }
    }
}

// GetCasterGeometryList(kList): the effect's affected nodes' casting geometry, less what's under
// its unaffected nodes and this generator's unaffected casters
void __fastcall GetCasterGeometryList(NiShadowGenerator* self, re::Edx, void* list) {
    List_RemoveAll(list);
    NiDynamicEffect* effect = self->dynamicEffect;
    if (effect == nullptr) {
        return;
    }
    for (ListNode* n = effect->affectedNodes.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        Generator_AddCasterGeometry(self, node, list);
    }
    for (ListNode* n = self->dynamicEffect->unaffectedNodes.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        Generator_RemoveGeometry(self, node, list);
    }
    for (ListNode* n = self->unaffectedCasters.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        Generator_RemoveGeometry(self, node, list);
    }
}

// GetReceiverGeometryList(kList): the same with the unaffected receivers
void __fastcall GetReceiverGeometryList(NiShadowGenerator* self, re::Edx, void* list) {
    List_RemoveAll(list);
    NiDynamicEffect* effect = self->dynamicEffect;
    if (effect == nullptr) {
        return;
    }
    for (ListNode* n = effect->affectedNodes.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        Generator_AddCasterGeometry(self, node, list);
    }
    for (ListNode* n = self->dynamicEffect->unaffectedNodes.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        Generator_RemoveGeometry(self, node, list);
    }
    for (ListNode* n = self->unaffectedReceivers.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        Generator_RemoveGeometry(self, node, list);
    }
}

// the receivers' materials marked for an update (each geometry's slot 21), through a list on the
// stack (vtable 0x952E10; emptied, its vtable left)
void __fastcall ForceMaterialUpdate(NiShadowGenerator* self, re::Edx) {
    NiTPointerList<NiObject*> list;
    list.count = 0;
    list.head = nullptr;
    list.tail = nullptr;
    list.vtable = reinterpret_cast<void*>(ListBase_vtable);
    Generator_GetReceiverGeometryList(self, &list);
    for (NiTListNode<NiObject*>* n = list.head; n != nullptr;) {
        NiObject* geometry = n->element;
        n = n->next;
        Virtual<void>(geometry, 21, 1);
    }
    list.vtable = reinterpret_cast<void*>(ListBase_vtable);
    List_RemoveAll(&list);
}

// AssignShadowMap(pkShadowMap, uiIndex, kPoolPos, pkGeometry): when active, the map (and its
// clicks) given this generator and taken out of the manager's available maps (by position when
// there is one), then the assignment callback's answer
uint8_t __fastcall AssignShadowMap(NiShadowGenerator* self, re::Edx, uint8_t* map, uint32_t index, void* position,
                                   void* geometry) {
    if (((self->flags >> 3) & 1) == 0) {
        return 0;
    }
    self->flags |= 1;
    uint32_t kind = map[4] & 0xF;
    if (kind == 0) {
        uint8_t* click = At<uint8_t*>(map, 0xC);
        if (click != nullptr) {
            At<NiShadowGenerator*>(click, 0x80) = self;
        }
        void* list = NiShadowManager_ms_pkShadowManager + 0x6C;
        re::Ptr32<void> slot = position;
        if (position != nullptr) {
            List_RemovePos(list, &slot);
        } else {
            slot = map;
            List_RemovePointer(list, &slot);
        }
        click = At<uint8_t*>(map, 0xC);
        if (click != nullptr) {
            click[0x84] |= 0x10;
        }
    } else if (kind == 1) {
        ShadowCubeMap_SetGenerator(map, self);
        void* list = NiShadowManager_ms_pkShadowManager + 0x8C;
        re::Ptr32<void> slot = position;
        if (position != nullptr) {
            List_RemovePos(list, &slot);
        } else {
            slot = map;
            List_RemovePointer(list, &slot);
        }
        uint8_t* cube = static_cast<uint8_t*>(ShadowMap_IsKindOf(reinterpret_cast<const void*>(NiShadowCubeMap_RTTI), map));
        for (int face = 0; face < 6; face++) {
            uint8_t* click = At<uint8_t*>(cube, 0x10 + face * 4);
            if (click != nullptr) {
                click[0x84] |= 0x10;
            }
        }
    }
    uint8_t result = self->assignShadowMap(self, geometry, map, index);
    Generator_ForceMaterialUpdate(self);
    return result;
}

// ReleaseAllShadowMaps: the maps' clicks let go of this generator and the maps given back to the
// manager; the cleanup callback; the receivers' materials updated
void __fastcall ReleaseAllShadowMaps(NiShadowGenerator* self, re::Edx) {
    self->flags |= 2;
    if (((self->flags >> 2) & 1) == 0) {
        Generator_MarkShadowMapsDirty(self);
    }
    uint32_t count = MapCount(self);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* map = MapAt(self, i);
        uint32_t kind = map[4] & 0xF;
        if (kind == 0) {
            uint8_t* click = At<uint8_t*>(map, 0xC);
            if (click != nullptr) {
                At<NiShadowGenerator*>(click, 0x80) = nullptr;
            }
            ShadowManager_ReleaseShadowMap(map);
        } else if (kind == 1) {
            for (int face = 0; face < 6; face++) {
                uint8_t* click = At<uint8_t*>(map, 0x10 + face * 4);
                if (click != nullptr) {
                    At<NiShadowGenerator*>(click, 0x80) = nullptr;
                }
            }
            ShadowManager_ReleaseShadowCubeMap(map);
        }
    }
    if (self->cleanup != nullptr) {
        self->cleanup(self);
    }
    Generator_ForceMaterialUpdate(self);
}

// SetActive(bActive, bForceMaterialUpdate, bRetainShadowMaps): through the shadow manager (not
// on for an effect that's off); the effect's type kept in step
uint8_t __fastcall SetActive(NiShadowGenerator* self, re::Edx, int active, int forceMaterialUpdate,
                             int retainShadowMaps) {
    NiDynamicEffect* effect = self->dynamicEffect;
    if (effect == nullptr) {
        return 0;
    }
    uint8_t result = 1;
    uint8_t on = uint8_t(active);
    if (on != ((self->flags >> 3) & 1)) {
        if (effect->on == 0 && on != 0) {
            result = 0;
        } else if (NiShadowManager_ms_pkShadowManager == nullptr ||
                   !ShadowManager_ActivateShadowGenerator(self, active)) {
            result = 0;
        } else {
            if (on != 0) {
                self->flags |= 8;
            } else {
                self->flags &= 0xFFF7;
                if (!re::Flag(retainShadowMaps)) {
                    Generator_ReleaseAllShadowMaps(self);
                }
            }
            if (re::Flag(forceMaterialUpdate)) {
                Generator_ForceMaterialUpdate(self);
            }
        }
    }
    if ((self->flags >> 3) & 1) {
        if (self->dynamicEffect->effectType < 5) {
            self->dynamicEffect->effectType = uint8_t(self->dynamicEffect->effectType + 3);
        }
    } else if (self->dynamicEffect->effectType >= 5) {
        self->dynamicEffect->effectType = uint8_t(self->dynamicEffect->effectType - 3);
    }
    return result;
}

// ~NiShadowGenerator
void __fastcall Destruct(NiShadowGenerator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShadowGenerator_vtable);
    Generator_ReleaseAllShadowMaps(self);
    if (self->dynamicEffect != nullptr) {
        DynamicEffect_SetShadowGenerator(self->dynamicEffect, nullptr);
    }
    DestroyList(&self->unaffectedReceivers);
    DestroyList(&self->unaffectedCasters);
    Object_Destruct(self);
}

NiShadowGenerator* __fastcall DeletingDestructor(NiShadowGenerator* self, re::Edx, uint32_t flags) {
    Generator_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x68);
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISHADOWGEN_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISHADOWGEN_REVERSED("NiShadowGenerator::GetShadowMapCount", 0x00772440, &ni::GetShadowMapCount);
NISHADOWGEN_REVERSED("NiShadowGenerator::RetrieveShadowMap", 0x00772460, &ni::RetrieveShadowMap);
NISHADOWGEN_REVERSED("NiTPointerList<NiNode*>::DeletingDestructor", 0x007724E0, &ni::ListDeletingDestructor);
NISHADOWGEN_REVERSED("NiShadowGenerator::RegisterStreamables", 0x00772560, &ni::RegisterStreamables);
NISHADOWGEN_REVERSED("NiShadowGenerator::IsEqual", 0x00772590, &ni::IsEqual);
NISHADOWGEN_REVERSED("NiShadowMap::IsKindOf", 0x00772620, &ni::ShadowMapIsKindOf);
NISHADOWGEN_REVERSED("NiShadowCubeMap::SetGenerator", 0x00772660, &ni::ShadowCubeMapSetGenerator);
NISHADOWGEN_REVERSED("NiShadowGenerator::AttachUnaffectedCasterNode", 0x00772690, &ni::AttachUnaffectedCasterNode);
NISHADOWGEN_REVERSED("NiShadowGenerator::AttachUnaffectedReceiverNode", 0x00772700, &ni::AttachUnaffectedReceiverNode);
NISHADOWGEN_REVERSED("NiShadowGenerator::SubtreeContains", 0x00772770, &ni::SubtreeContains);
NISHADOWGEN_REVERSED("NiShadowGenerator::AddCasterGeometry", 0x00772860, &ni::AddCasterGeometry);
NISHADOWGEN_REVERSED("NiShadowGenerator::MarkShadowMapsDirty", 0x00772930, &ni::MarkShadowMapsDirty);
NISHADOWGEN_REVERSED("NiShadowGenerator::GetShadowMapsDirty", 0x007729E0, &ni::GetShadowMapsDirty);
NISHADOWGEN_REVERSED("NiShadowGenerator::ProcessClone", 0x00772B60, &ni::ProcessClone);
NISHADOWGEN_REVERSED("NiShadowGenerator::LoadBinary", 0x00772C60, &ni::LoadBinary);
NISHADOWGEN_REVERSED("NiShadowGenerator::LinkObject", 0x00772DF0, &ni::LinkObject);
NISHADOWGEN_REVERSED("NiShadowGenerator::SaveBinary", 0x00772ED0, &ni::SaveBinary);
NISHADOWGEN_REVERSED("NiShadowGenerator::GetRTTI", 0x00773100, &ni::GetRTTI);
NISHADOWGEN_REVERSED("NiShadowGenerator::NiShadowGenerator", 0x00773110, &ni::Construct);
NISHADOWGEN_REVERSED("NiShadowGenerator::IsUnaffectedReceiverGeometry", 0x007731D0, &ni::IsUnaffectedReceiverGeometry);
NISHADOWGEN_REVERSED("NiShadowGenerator::RemoveGeometry", 0x00773220, &ni::RemoveGeometry);
NISHADOWGEN_REVERSED("NiShadowGenerator::GetCasterGeometryList", 0x00773360, &ni::GetCasterGeometryList);
NISHADOWGEN_REVERSED("NiShadowGenerator::GetReceiverGeometryList", 0x007733E0, &ni::GetReceiverGeometryList);
NISHADOWGEN_REVERSED("NiShadowGenerator::ForceMaterialUpdate", 0x00773460, &ni::ForceMaterialUpdate);
NISHADOWGEN_REVERSED("NiShadowGenerator::AssignShadowMap", 0x007734D0, &ni::AssignShadowMap);
NISHADOWGEN_REVERSED("NiShadowGenerator::ReleaseAllShadowMaps", 0x007735F0, &ni::ReleaseAllShadowMaps);
NISHADOWGEN_REVERSED("NiShadowGenerator::SetActive", 0x007736C0, &ni::SetActive);
NISHADOWGEN_REVERSED("NiShadowGenerator::~NiShadowGenerator", 0x007737A0, &ni::Destruct);
NISHADOWGEN_REVERSED("NiShadowGenerator::DeletingDestructor", 0x00773800, &ni::DeletingDestructor);
