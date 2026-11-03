// NiDynamicEffect and NiLight (src/gamebryo/NiDynamicEffect.h): the affected / unaffected node
// lists, the switch and the shadow generator, cloning (the lists remapped to the clones),
// streaming, comparison, viewer strings; NiTPointerList::RemovePos.

#include <cstdint>
#include <intrin.h>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDynamicEffect.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiDynamicEffect_RTTI, 0x020E2C00);
GAME_VAR(RTTI, NiLight_RTTI, 0x020E2C68);

constexpr uintptr_t NiDynamicEffect_vtable = 0x00952D50;
constexpr uintptr_t NiLight_vtable = 0x00952E68;
constexpr uintptr_t NodeList_vtable = 0x0095145C;      // NiTPointerList<NiNode*>
constexpr uintptr_t ListBase_vtable = 0x00952E10;      // the list's destructor's
constexpr uintptr_t Allocator_vtable = 0x0099C748;     // its allocator's, last

GAME_VAR(long, NiDynamicEffect_ms_iNextIndex, 0x00BC8A6C);
GAME_VAR(void*, NiShadowManager_ms_pkShadowManager, 0x020E2410);

using NodeList = NiTPointerList<NiNode*>;
using ListNode = NiTListNode<NiNode*>;

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, Node_AttachEffect, 0x0075F4A0, NiNode* self, NiDynamicEffect* effect);
GAME_FN(void, __thiscall, Node_DetachEffect, 0x00760030, NiNode* self, NiDynamicEffect* effect);
GAME_FN(void, __thiscall, List_RemovePointer, 0x0075F630, NodeList* list, const re::Ptr32<NiNode>* element);
GAME_FN(void, __thiscall, List_RemoveAll, 0x0075DB60, NodeList* list);
GAME_FN(void, __cdecl, ShadowManager_AddShadowGenerator, 0x007574F0, void* generator);
GAME_FN(void, __cdecl, ShadowManager_RemoveShadowGenerator, 0x00758710, void* generator);
GAME_FN(void, __cdecl, Renderer_RemoveEffect, 0x00755660, NiDynamicEffect* effect);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, NiPoint3* self, void* stream);
GAME_FN(char*, __thiscall, Color_GetViewerString, 0x00762710, const NiPoint3* self, const char* prefix);
// (this chunk's, called where modes apply)
GAME_FN(uint8_t, __thiscall, ShadowGenerator_SetActive, 0x007736C0, void* generator, int active, int force,
        int retainShadowMaps);
GAME_FN(void, __thiscall, DynamicEffect_AttachAffectedNode, 0x00771A10, NiDynamicEffect* self, NiNode* node);
GAME_FN(void, __thiscall, DynamicEffect_DetachAllAffectedNodes, 0x00771A80, NiDynamicEffect* self);
GAME_FN(void, __thiscall, DynamicEffect_AttachUnaffectedNode, 0x00771AE0, NiDynamicEffect* self, NiNode* node);
GAME_FN(void, __thiscall, DynamicEffect_DetachAllUnaffectedNodes, 0x00771B50, NiDynamicEffect* self);
GAME_FN(void, __thiscall, DynamicEffect_CopyMembers, 0x00771BA0, NiDynamicEffect* self, NiDynamicEffect* dest,
        void* cloning);
GAME_FN(uint8_t, __thiscall, DynamicEffect_IsEqual, 0x00771CC0, NiDynamicEffect* self, NiDynamicEffect* other);
GAME_FN(void, __thiscall, DynamicEffect_Destruct, 0x00771E80, NiDynamicEffect* self);
GAME_FN(void, __thiscall, DynamicEffect_LoadBinary, 0x00772070, NiDynamicEffect* self, void* stream);
GAME_FN(void, __thiscall, DynamicEffect_SaveBinary, 0x007720D0, NiDynamicEffect* self, void* stream);
GAME_FN(NiDynamicEffect*, __thiscall, DynamicEffect_Construct, 0x00772180, void* self);
GAME_FN(void, __thiscall, DynamicEffect_GetViewerStrings, 0x00772230, NiDynamicEffect* self,
        NiTArray<char*>* strings);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);

// ---- the lists (NiTPointerList, inlined)

inline ListNode* NewNode(NodeList* list) {
    return Virtual<ListNode*>(list, 1);
}
inline void FreeNode(NodeList* list, ListNode* node) {
    Virtual<void>(list, 2, node);
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

// the head unlinked and freed: its element
inline NiNode* RemoveHead(NodeList* list) {
    ListNode* n = list->head;
    NiNode* element = n->element;
    ListNode* next = n->next;
    list->head = next;
    if (next != nullptr) {
        next->prev = nullptr;
    } else {
        list->tail = nullptr;
    }
    FreeNode(list, n);
    list->count--;
    return element;
}

// the list's destructor, inlined: emptied, then the base and allocator vtables
inline void DestroyList(NodeList* list) {
    list->vtable = reinterpret_cast<void*>(ListBase_vtable);
    List_RemoveAll(list);
    list->vtable = reinterpret_cast<void*>(Allocator_vtable);
}

// NiTPointerMap<NiObject*, NiObject*>'s lookup (the cloning process's map: its first field)
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

// ---- NiDynamicEffect

// SetSwitch(bOn, bUpdateShadowGenerator)
void __fastcall SetSwitch(NiDynamicEffect* self, re::Edx, int on, int updateGenerator) {
    self->on = uint8_t(on);
    if (re::Flag(updateGenerator) && self->shadowGenerator != nullptr) {
        ShadowGenerator_SetActive(self->shadowGenerator, on, 0, 0);
    }
}

// SetShadowGenerator: the effect type moved into the shadowed range (+3) or back
void __fastcall SetShadowGenerator(NiDynamicEffect* self, re::Edx, NiShadowGenerator* generator) {
    self->shadowGenerator = generator;
    uint8_t type = self->effectType;
    if (generator != nullptr) {
        if (type < 5) {
            self->effectType = uint8_t(type + 3);
        }
    } else if (type >= 5) {
        self->effectType = uint8_t(type - 3);
    }
}

// vf6 RegisterStreamables: the shadow generator's too
uint8_t __fastcall RegisterStreamables(NiDynamicEffect* self, re::Edx, void* stream) {
    if (!AVObject_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->shadowGenerator != nullptr) {
        return Virtual<uint8_t>(self->shadowGenerator, 6, stream);
    }
    return 1;
}

// AttachAffectedNode: at the head (once); the node told (NiNode::AttachEffect)
void __fastcall AttachAffectedNode(NiDynamicEffect* self, re::Edx, NiNode* node) {
    if (Contains(&self->affectedNodes, node)) {
        return;
    }
    AddHead(&self->affectedNodes, node);
    Node_AttachEffect(node, self);
}

void __fastcall DetachAllAffectedNodes(NiDynamicEffect* self, re::Edx) {
    while (self->affectedNodes.count != 0) {
        NiNode* node = RemoveHead(&self->affectedNodes);
        Node_DetachEffect(node, self);
    }
}

void __fastcall AttachUnaffectedNode(NiDynamicEffect* self, re::Edx, NiNode* node) {
    if (Contains(&self->unaffectedNodes, node)) {
        return;
    }
    AddHead(&self->unaffectedNodes, node);
}

void __fastcall DetachAllUnaffectedNodes(NiDynamicEffect* self, re::Edx) {
    while (self->unaffectedNodes.count != 0) {
        RemoveHead(&self->unaffectedNodes);
    }
}

// CopyMembers: the shadow generator cloned for the clone (inactive when the clone is off; given
// to the shadow manager when there is one, else kept)
void __fastcall CopyMembers(NiDynamicEffect* self, re::Edx, NiDynamicEffect* dest, void* cloning) {
    AVObject_CopyMembers(self, dest, cloning);
    dest->pushCount = self->pushCount;
    dest->revisionID = self->revisionID;
    dest->on = self->on;
    dest->shadowGenerator = nullptr;
    if (self->shadowGenerator == nullptr) {
        return;
    }
    void* generator = Virtual<void*>(self->shadowGenerator, 3, cloning);   // CreateClone
    At<NiDynamicEffect*>(generator, 0x4C) = dest;
    if (!dest->on) {
        ShadowGenerator_SetActive(generator, 0, 0, 0);
    }
    if (NiShadowManager_ms_pkShadowManager != nullptr) {
        ShadowManager_AddShadowGenerator(generator);
        return;
    }
    dest->shadowGenerator = static_cast<NiShadowGenerator*>(generator);
}

// vf5 LinkObject: the unaffected nodes (each at the head)
void __fastcall LinkObject(NiDynamicEffect* self, re::Edx, void* stream) {
    AVObject_LinkObject(self, stream);
    uint32_t count = Stream_GetNumberOfLinkIDs(stream);
    while (count != 0) {
        count--;
        NiNode* node = static_cast<NiNode*>(Stream_ResolveLinkID(stream));
        if (node != nullptr) {
            AddHead(&self->unaffectedNodes, node);
        }
    }
}

// both lists the same length, their entries null in the same places (the nodes themselves not
// compared)
inline bool ListsMatch(const NodeList* a, const NodeList* b) {
    if (a->count != b->count) {
        return false;
    }
    if (a->count == 0) {
        return true;
    }
    ListNode* x = a->head;
    ListNode* y = b->head;
    while (x != nullptr) {
        NiNode* ex = x->element;
        x = x->next;
        NiNode* ey = y->element;
        y = y->next;
        if ((ex == nullptr) != (ey == nullptr)) {
            return false;
        }
    }
    return true;
}

// vf8 IsEqual
uint8_t __fastcall IsEqual(NiDynamicEffect* self, re::Edx, NiDynamicEffect* other) {
    if (!AVObject_IsEqual(self, other)) {
        return 0;
    }
    if (!ListsMatch(&self->affectedNodes, &other->affectedNodes) ||
        !ListsMatch(&self->unaffectedNodes, &other->unaffectedNodes)) {
        return 0;
    }
    return 1;
}

// IsUnaffectedNode
uint8_t __fastcall IsUnaffectedNode(NiDynamicEffect* self, re::Edx, NiNode* node) {
    return Contains(&self->unaffectedNodes, node) ? 1 : 0;
}

// NiTPointerList::RemovePos(kPos): the node at the iterator unlinked and freed, the iterator
// moved to the next one; its element
NiNode* __fastcall ListRemovePos(NodeList* self, re::Edx, re::Ptr32<ListNode>* position) {
    ListNode* node = *position;
    if (node == self->head) {
        *position = node->next;
        ListNode* n = self->head;
        ListNode* next = n->next;
        self->head = next;
        if (next != nullptr) {
            next->prev = nullptr;
        } else {
            self->tail = nullptr;
        }
        NiNode* element = n->element;
        FreeNode(self, n);
        self->count--;
        return element;
    }
    if (node == self->tail) {
        *position = nullptr;
        ListNode* n = self->tail;
        ListNode* prev = n->prev;
        self->tail = prev;
        if (prev != nullptr) {
            prev->next = nullptr;
        } else {
            self->head = nullptr;
        }
        NiNode* element = n->element;
        FreeNode(self, n);
        self->count--;
        return element;
    }
    ListNode* prev = node->prev;
    ListNode* next = node->next;
    *position = next;
    if (prev != nullptr) {
        prev->next = next;
    }
    if (next != nullptr) {
        next->prev = prev;
    }
    NiNode* element = node->element;
    FreeNode(self, node);
    self->count--;
    return element;
}

// ~NiDynamicEffect
void __fastcall Destruct(NiDynamicEffect* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiDynamicEffect_vtable);
    DynamicEffect_DetachAllAffectedNodes(self);
    DynamicEffect_DetachAllUnaffectedNodes(self);
    if (self->shadowGenerator != nullptr) {
        At<NiDynamicEffect*>(self->shadowGenerator, 0x4C) = nullptr;
        if (NiShadowManager_ms_pkShadowManager != nullptr) {
            ShadowManager_RemoveShadowGenerator(self->shadowGenerator);
        }
    }
    DestroyList(&self->unaffectedNodes);
    DestroyList(&self->affectedNodes);
    AVObject_Destruct(self);
}

const RTTI* __fastcall GetRTTI(NiDynamicEffect*, re::Edx) {
    return &NiDynamicEffect_RTTI;
}

// DetachAffectedNode: when it's there, removed and the node told
void __fastcall DetachAffectedNode(NiDynamicEffect* self, re::Edx, re::Ptr32<NiNode> node) {
    if (!Contains(&self->affectedNodes, node)) {
        return;
    }
    List_RemovePointer(&self->affectedNodes, &node);
    Node_DetachEffect(node, self);
}

// vf11 ProcessClone: the shadow generator's; then (unless the copy type is 0) the clone's lists
// built from this one's (from the tails back), each node mapped to its clone (unmapped nodes
// kept as they are, unless the copy type is 1)
void __fastcall ProcessClone(NiDynamicEffect* self, re::Edx, void* cloning) {
    const PointerMap* map = static_cast<PointerMap*>(*static_cast<const re::Ptr32<PointerMap>*>(cloning));
    re::Ptr32<void> found = nullptr;
    NiDynamicEffect* clone = nullptr;
    if (MapLookup(map, self, found)) {
        clone = static_cast<NiDynamicEffect*>(found);
    }
    if (self->shadowGenerator != nullptr) {
        Virtual<void>(self->shadowGenerator, 11, cloning);
    }
    uint32_t copyType = At<uint32_t>(cloning, 0x10);
    if (copyType == 0) {
        return;
    }
    for (ListNode* n = self->affectedNodes.tail; n != nullptr;) {
        void* node = n->element;
        n = n->prev;
        re::Ptr32<void> mapped;
        if (MapLookup(static_cast<PointerMap*>(*static_cast<const re::Ptr32<PointerMap>*>(cloning)), node, mapped)) {
            node = mapped;
        } else if (At<uint32_t>(cloning, 0x10) == 1) {
            continue;
        }
        if (node != nullptr) {
            DynamicEffect_AttachAffectedNode(clone, static_cast<NiNode*>(node));
        }
    }
    for (ListNode* n = self->unaffectedNodes.tail; n != nullptr;) {
        void* node = n->element;
        n = n->prev;
        re::Ptr32<void> mapped;
        if (MapLookup(static_cast<PointerMap*>(*static_cast<const re::Ptr32<PointerMap>*>(cloning)), node, mapped)) {
            node = mapped;
        } else if (At<uint32_t>(cloning, 0x10) == 1) {
            continue;
        }
        if (node != nullptr) {
            DynamicEffect_AttachUnaffectedNode(clone, static_cast<NiNode*>(node));
        }
    }
}

// vf4 LoadBinary: the switch from 10.1.0.102; the unaffected nodes' links
void __fastcall LoadBinary(NiDynamicEffect* self, re::Edx, void* stream) {
    AVObject_LoadBinary(self, stream);
    if (StreamVersion(stream) >= 0x0A010066) {
        uint8_t on;
        LoadBytes(stream, &on, 1);
        self->on = on != 0 ? 1 : 0;
    }
    Stream_ReadMultipleLinkIDs(stream);
}

// vf7 SaveBinary: the switch, the unaffected nodes (from the tail back)
void __fastcall SaveBinary(NiDynamicEffect* self, re::Edx, void* stream) {
    AVObject_SaveBinary(self, stream);
    uint8_t on = self->on;
    SaveBytes(stream, &on, 1);
    int32_t count = int32_t(self->unaffectedNodes.count);
    SaveBytes(stream, &count, 4);
    if (count > 0) {
        for (ListNode* n = self->unaffectedNodes.tail; n != nullptr;) {
            NiNode* node = n->element;
            n = n->prev;
            Virtual<void>(stream, 12, node);   // SaveLinkID
        }
    }
}

NiDynamicEffect* __fastcall Construct(NiDynamicEffect* self, re::Edx) {
    AVObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiDynamicEffect_vtable);
    self->effectType = 0xFF;
    self->pushCount = 0;
    self->shadowGenerator = nullptr;
    self->on = 1;
    self->revisionID = 1;
    self->affectedNodes.count = 0;
    self->affectedNodes.head = nullptr;
    self->affectedNodes.tail = nullptr;
    self->affectedNodes.vtable = reinterpret_cast<void*>(NodeList_vtable);
    self->unaffectedNodes.count = 0;
    self->unaffectedNodes.head = nullptr;
    self->unaffectedNodes.tail = nullptr;
    self->unaffectedNodes.vtable = reinterpret_cast<void*>(NodeList_vtable);
    self->index = _InterlockedIncrement(&NiDynamicEffect_ms_iNextIndex);
    return self;
}

NiDynamicEffect* __fastcall DeletingDestructor(NiDynamicEffect* self, re::Edx, uint32_t flags) {
    DynamicEffect_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0xF8);
    }
    return self;
}

// one node's viewer string ("<kind> node[i]" = its address), SetAt inlined
inline void AddNodeString(NiTArray<char*>* strings, const char* format, int index, NiNode* node) {
    char label[0x40];
    NiSprintf(label, 0x40, format, index);
    AddViewerStringInline(strings, GetViewerStringPointer(label, node));
}

// vf9 (the unaffected nodes listed only when there are affected ones)
void __fastcall GetViewerStrings(NiDynamicEffect* self, re::Edx, NiTArray<char*>* strings) {
    AVObject_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiDynamicEffect_RTTI.name));
    if (self->affectedNodes.count == 0) {
        return;
    }
    int index = 0;
    for (ListNode* n = self->affectedNodes.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        AddNodeString(strings, "affected node[%d]", index, node);
        index++;
    }
    for (ListNode* n = self->unaffectedNodes.head; n != nullptr;) {
        NiNode* node = n->element;
        n = n->next;
        AddNodeString(strings, "unaffected node[%d]", index, node);
        index++;
    }
}

// ---- NiLight

void __fastcall LightDestruct(NiLight* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiLight_vtable);
    Renderer_RemoveEffect(self);
    DynamicEffect_Destruct(self);
}

void __fastcall LightCopyMembers(NiLight* self, re::Edx, NiLight* dest, void* cloning) {
    DynamicEffect_CopyMembers(self, dest, cloning);
    dest->dimmer = self->dimmer;
    dest->ambient = self->ambient;
    dest->diffuse = self->diffuse;
    dest->specular = self->specular;
}

const RTTI* __fastcall LightGetRTTI(NiLight*, re::Edx) {
    return &NiLight_RTTI;
}

NiLight* __fastcall LightConstruct(NiLight* self, re::Edx) {
    DynamicEffect_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiLight_vtable);
    self->rendererData = nullptr;
    self->dimmer = 1.0f;
    self->ambient = {1.0f, 1.0f, 1.0f};
    self->diffuse = {1.0f, 1.0f, 1.0f};
    self->specular = {1.0f, 1.0f, 1.0f};
    return self;
}

NiLight* __fastcall LightDeletingDestructor(NiLight* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiLight_vtable);
    Renderer_RemoveEffect(self);
    DynamicEffect_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x124);
    }
    return self;
}

void __fastcall LightLoadBinary(NiLight* self, re::Edx, void* stream) {
    DynamicEffect_LoadBinary(self, stream);
    LoadBytes(stream, &self->dimmer, 4);
    Point3_LoadBinary(&self->ambient, stream);
    Point3_LoadBinary(&self->diffuse, stream);
    Point3_LoadBinary(&self->specular, stream);
}

void __fastcall LightSaveBinary(NiLight* self, re::Edx, void* stream) {
    DynamicEffect_SaveBinary(self, stream);
    SaveBytes(stream, &self->dimmer, 4);
    Point3_SaveBinary(&self->ambient, stream);
    Point3_SaveBinary(&self->diffuse, stream);
    Point3_SaveBinary(&self->specular, stream);
}

// vf8: the dimmer and the colours (NiColor's != is NiPoint3's code)
uint8_t __fastcall LightIsEqual(NiLight* self, re::Edx, NiLight* other) {
    if (!DynamicEffect_IsEqual(self, other)) {
        return 0;
    }
    if (!(other->dimmer == self->dimmer) || Point3_NotEqual(&self->ambient, &other->ambient) ||
        Point3_NotEqual(&self->diffuse, &other->diffuse) || Point3_NotEqual(&self->specular, &other->specular)) {
        return 0;
    }
    return 1;
}

void __fastcall LightGetViewerStrings(NiLight* self, re::Edx, NiTArray<char*>* strings) {
    DynamicEffect_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiLight_RTTI.name));
    AddViewerString(strings, GetViewerStringFloat("m_fDimmer", self->dimmer));
    AddViewerString(strings, Color_GetViewerString(&self->ambient, "m_kAmb"));
    AddViewerString(strings, Color_GetViewerString(&self->diffuse, "m_kDiff"));
    AddViewerString(strings, Color_GetViewerString(&self->specular, "m_kSpec"));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIDYNEFFECT_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIDYNEFFECT_REVERSED("NiDynamicEffect::SetSwitch", 0x00771960, &ni::SetSwitch);
NIDYNEFFECT_REVERSED("NiDynamicEffect::SetShadowGenerator", 0x00771990, &ni::SetShadowGenerator);
NIDYNEFFECT_REVERSED("NiDynamicEffect::RegisterStreamables", 0x007719D0, &ni::RegisterStreamables);
NIDYNEFFECT_REVERSED("NiDynamicEffect::AttachAffectedNode", 0x00771A10, &ni::AttachAffectedNode);
NIDYNEFFECT_REVERSED("NiDynamicEffect::DetachAllAffectedNodes", 0x00771A80, &ni::DetachAllAffectedNodes);
NIDYNEFFECT_REVERSED("NiDynamicEffect::AttachUnaffectedNode", 0x00771AE0, &ni::AttachUnaffectedNode);
NIDYNEFFECT_REVERSED("NiDynamicEffect::DetachAllUnaffectedNodes", 0x00771B50, &ni::DetachAllUnaffectedNodes);
NIDYNEFFECT_REVERSED("NiDynamicEffect::CopyMembers", 0x00771BA0, &ni::CopyMembers);
NIDYNEFFECT_REVERSED("NiDynamicEffect::LinkObject", 0x00771C40, &ni::LinkObject);
NIDYNEFFECT_REVERSED("NiDynamicEffect::IsEqual", 0x00771CC0, &ni::IsEqual);
NIDYNEFFECT_REVERSED("NiDynamicEffect::IsUnaffectedNode", 0x00771D70, &ni::IsUnaffectedNode);
NIDYNEFFECT_REVERSED("NiTPointerList<NiNode*>::RemovePos", 0x00771DA0, &ni::ListRemovePos);
NIDYNEFFECT_REVERSED("NiDynamicEffect::~NiDynamicEffect", 0x00771E80, &ni::Destruct);
NIDYNEFFECT_REVERSED("NiDynamicEffect::GetRTTI", 0x00771F00, &ni::GetRTTI);
NIDYNEFFECT_REVERSED("NiDynamicEffect::DetachAffectedNode", 0x00771F10, &ni::DetachAffectedNode);
NIDYNEFFECT_REVERSED("NiDynamicEffect::ProcessClone", 0x00771F60, &ni::ProcessClone);
NIDYNEFFECT_REVERSED("NiDynamicEffect::LoadBinary", 0x00772070, &ni::LoadBinary);
NIDYNEFFECT_REVERSED("NiDynamicEffect::SaveBinary", 0x007720D0, &ni::SaveBinary);
NIDYNEFFECT_REVERSED("NiDynamicEffect::NiDynamicEffect", 0x00772180, &ni::Construct);
NIDYNEFFECT_REVERSED("NiDynamicEffect::DeletingDestructor", 0x00772200, &ni::DeletingDestructor);
NIDYNEFFECT_REVERSED("NiDynamicEffect::GetViewerStrings", 0x00772230, &ni::GetViewerStrings);
NIDYNEFFECT_REVERSED("NiLight::~NiLight", 0x00773820, &ni::LightDestruct);
NIDYNEFFECT_REVERSED("NiLight::CopyMembers", 0x00773840, &ni::LightCopyMembers);
RE_UNHOOKED("NiLight::LinkObject", 0x007738D0);            // a jmp to NiDynamicEffect's
RE_UNHOOKED("NiLight::RegisterStreamables", 0x007738E0);   // a jmp to NiDynamicEffect's
NIDYNEFFECT_REVERSED("NiLight::GetRTTI", 0x007738F0, &ni::LightGetRTTI);
NIDYNEFFECT_REVERSED("NiLight::NiLight", 0x00773900, &ni::LightConstruct);
NIDYNEFFECT_REVERSED("NiLight::DeletingDestructor", 0x007739A0, &ni::LightDeletingDestructor);
NIDYNEFFECT_REVERSED("NiLight::LoadBinary", 0x007739E0, &ni::LightLoadBinary);
NIDYNEFFECT_REVERSED("NiLight::SaveBinary", 0x00773A50, &ni::LightSaveBinary);
NIDYNEFFECT_REVERSED("NiLight::IsEqual", 0x00773AC0, &ni::LightIsEqual);
NIDYNEFFECT_REVERSED("NiLight::GetViewerStrings", 0x00773B40, &ni::LightGetViewerStrings);
