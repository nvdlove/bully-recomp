// NiAVObject (vtable 0x951A70, 0xC4 bytes): a node of the scene graph. Its transforms, its
// world bound, its properties (a list of references), its collision object, the selective
// update flags; the update passes (Update, UpdateDownwardPass, UpdateSelectedDownwardPass...),
// the property and effect state pushed down the graph, streaming and cloning.
//
// Virtual slots (after NiObject's 0..17): 18 UpdateControllers, 19 UpdateNodeBound,
// 20 ApplyTransform, 23 GetObjectByName, 24 SetSelectiveUpdateFlags, 25 UpdateDownwardPass,
// 26 UpdateSelectedDownwardPass, 27 UpdateRigidDownwardPass, 28 UpdatePropertiesDownward,
// 29 UpdateEffectsDownward, 30 UpdateWorldData, 31 UpdateWorldBound, 32 OnVisible,
// 33 PurgeRendererData. NiObject's: 0 destructor, 2 GetRTTI, 3 CreateClone, 4 LoadBinary,
// 5 LinkObject, 6 RegisterStreamables, 7 SaveBinary, 8 IsEqual, 9 GetViewerStrings,
// 11 ProcessClone, 12 PostLinkObject.

#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
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
GAME_VAR(RTTI, NiAVObject_RTTI, 0x020E2514);
GAME_VAR(NiPoint3, NiPoint3_ZERO, 0x020E2D44);

constexpr uintptr_t NiAVObject_vtable = 0x00951A70;
constexpr uintptr_t PropertyList_vtable = 0x0095127C;        // NiTPointerList<NiPropertyPtr>
constexpr uintptr_t PointerListBase_vtable = 0x0099C748;     // its base's (the allocator's)
constexpr uintptr_t GeometryArray_vtable = 0x0091756C;       // NiTPrimitiveArray<NiGeometry*>
constexpr uintptr_t GeometryArrayBase_vtable = 0x0091750C;   // NiTArray<NiGeometry*>

using Node = NiTListNode<NiObject*>;
using List = NiTPointerList<NiObject*>;

// the list's allocator: slot 2 frees a node
inline void FreeNode(List* list, Node* node) {
    Virtual<void>(list, 2, node);
}

// NiPointer's release, inlined everywhere
inline void Unref(NiObject* object) {
    Release(object);
}

GAME_FN(void, __thiscall, Transform_MakeIdentity, 0x00770920, NiTransform* self);
GAME_FN(uint8_t, __thiscall, Transform_NotEqual, 0x007709F0, const NiTransform* self, const NiTransform* other);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Matrix3_LoadBinary, 0x007690A0, NiMatrix3* self, void* stream);
GAME_FN(void, __thiscall, Matrix3_SaveBinary, 0x00769100, NiMatrix3* self, void* stream);
GAME_FN(void, __thiscall, PropertyList_AddHead, 0x00407BD0, List* list, const re::Ptr32<NiObject>* element);
GAME_FN(re::Ptr32<Node>*, __thiscall, PropertyList_RemoveHead, 0x00756F80, List* list, re::Ptr32<NiObject>* result);
GAME_FN(re::Ptr32<Node>*, __thiscall, PropertyList_RemoveTail, 0x00756FD0, List* list, re::Ptr32<NiObject>* result);
GAME_FN(NiObject*, __thiscall, Object_Clone, 0x007553B0, NiObject* self, void* cloning);
GAME_FN(void*, __thiscall, CollisionObject_GetSceneGraphObject, 0x00856850, NiObject* self);
GAME_FN(NiObject*, __thiscall, PropertyState_Construct, 0x007D2370, void* self);
GAME_FN(NiObject*, __thiscall, PropertyState_ConstructCopy, 0x007D1FB0, void* self, NiObject* other);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
// (this chunk's, called where modes apply)
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Node_UpdatePropertiesUpward, 0x0075FAA0, NiNode* self, re::Ptr32<NiObject>* result);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Node_UpdateEffectsUpward, 0x00760490, NiNode* self, re::Ptr32<NiObject>* result);
GAME_FN(NiObject*, __thiscall, AVObject_GetProperty, 0x0075DC10, NiAVObject* self, int type);
GAME_FN(void, __thiscall, AVObject_Destructor, 0x0075DEF0, NiAVObject* self);
GAME_FN(void, __stdcall, PropertyList_ProcessClone, 0x0075E2C0, List* list, void* cloning);
GAME_FN(void, __stdcall, PropertyList_CopyTo, 0x0075EA60, NiAVObject* dest, List* list, void* cloning);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, PropertyList_RemovePos, 0x0075DE40, List* list, re::Ptr32<NiObject>* result, re::Ptr32<Node>* pos);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, PropertyList_Remove, 0x0075E6B0, List* list, re::Ptr32<NiObject>* result,
        const re::Ptr32<NiObject>* element);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, List* list);

// ---- updates

// Update(fTime, bUpdateControllers): the downward pass here, the upward one from the parent
void __fastcall Update(NiAVObject* self, re::Edx, float time, int updateControllers) {
    Virtual<void>(self, 25, time, updateControllers);   // UpdateDownwardPass
    if (self->parent != nullptr) {
        Virtual<void>(self->parent, 38);   // UpdateUpwardPass
    }
}

// UpdateObjectControllers(fTime, bProperties): the properties' controllers (when asked), then
// its own
void __fastcall UpdateObjectControllers(NiAVObject* self, re::Edx, float time, int properties) {
    if (re::Flag(properties)) {
        for (Node* node = reinterpret_cast<List*>(&self->properties)->head; node != nullptr;) {
            NiObject* property = node->element;
            node = node->next;
            if (property != nullptr && At<void*>(property, 0xC) != nullptr) {
                Virtual<void>(property, 19, time);   // NiProperty::Update
            }
        }
    }
    for (NiObject* controller = re::PtrCast<NiObject*>(self->controller); controller != nullptr;
         controller = At<NiObject*>(controller, 0x30)) {
        Virtual<void>(controller, 20, time);   // NiTimeController::Update
    }
}

// vf25
void __fastcall UpdateDownwardPass(NiAVObject* self, re::Edx, float time, int updateControllers) {
    if (re::Flag(updateControllers)) {
        AVObject_UpdateObjectControllers(self, time, 1);
    }
    Virtual<void>(self, 30);   // UpdateWorldData
    Virtual<void>(self, 31);   // UpdateWorldBound
}

// vf26 and vf27 (UpdateSelectedDownwardPass, UpdateRigidDownwardPass)
void __fastcall UpdateSelectedDownwardPass(NiAVObject* self, re::Edx, float time) {
    uint8_t flags = uint8_t(self->flags);
    AVObject_UpdateObjectControllers(self, time, (flags >> 3) & 1);
    if ((uint8_t(self->flags) >> 2) & 1) {
        Virtual<void>(self, 30);
        Virtual<void>(self, 31);
    }
}

// vf18
void __fastcall UpdateControllers(NiAVObject* self, re::Edx, float time) {
    AVObject_UpdateObjectControllers(self, time, 1);
}

// the properties from the root down to here, pushed down from here
void __fastcall UpdateProperties(NiAVObject* self, re::Edx) {
    NiObject* state = nullptr;
    if (self->parent != nullptr) {
        re::Ptr32<NiObject> upward;
        NiObject* got = *Node_UpdatePropertiesUpward(self->parent, &upward);
        if (got != nullptr) {
            state = got;
            AddRef(state);
        }
        Unref(upward);
    } else {
        void* p = NiNew(0x38);
        if (p != nullptr) {
            NiObject* created = PropertyState_Construct(p);
            if (created != nullptr) {
                state = created;
                AddRef(state);
            }
        }
    }
    Virtual<void>(self, 28, state);   // UpdatePropertiesDownward
    Unref(state);
}

// the dynamic effects likewise
void __fastcall UpdateEffects(NiAVObject* self, re::Edx) {
    NiObject* state = nullptr;
    if (self->parent != nullptr) {
        re::Ptr32<NiObject> upward;
        NiObject* got = *Node_UpdateEffectsUpward(self->parent, &upward);
        if (got != nullptr) {
            state = got;
            AddRef(state);
            heapcheck::Trace(state, heapcheck::kAddRef);
        }
        heapcheck::Trace(upward, heapcheck::kRelease);
        Unref(upward);
    }
    Virtual<void>(self, 29, state);   // UpdateEffectsDownward
    heapcheck::Trace(state, heapcheck::kRelease);
    Unref(state);
}

// a property state with this object's properties over the parent's (a copy of it when asked;
// the parent's own when there are none)
re::Ptr32<NiObject>* __fastcall PushLocalProperties(NiAVObject* self, re::Edx, re::Ptr32<NiObject>* result, NiObject* parentState,
                                          int copy) {
    List* properties = reinterpret_cast<List*>(&self->properties);
    if (properties->count == 0) {
        *result = parentState;
        AddRef(parentState);
        return result;
    }
    re::Ptr32<NiObject> state = nullptr;
    NiObject* from;
    if (re::Flag(copy)) {
        void* p = NiNew(0x38);
        from = p != nullptr ? PropertyState_ConstructCopy(p, parentState) : nullptr;
    } else {
        from = parentState;
    }
    Pointer_Assign(&state, from);
    for (Node* node = properties->head; node != nullptr;) {
        NiObject* property = node->element;
        node = node->next;
        if (property != nullptr) {
            // NiPropertyState::SetProperty, inlined
            int type = Virtual<int>(property, 18);
            re::Ptr32<NiObject>& slot = At<NiObject*>(state, 8 + type * 4);
            NiObject* old = slot;
            if (old != property) {
                Unref(old);
                slot = property;
                AddRef(property);
            }
        }
    }
    *result = state;
    AddRef(state);
    Unref(state);
    return result;
}

uint8_t __fastcall HasTransformController(NiAVObject* self, re::Edx) {
    for (NiObject* controller = re::PtrCast<NiObject*>(self->controller); controller != nullptr;
         controller = At<NiObject*>(controller, 0x30)) {
        if (Virtual<uint8_t>(controller, 22)) {   // IsTransformController
            return 1;
        }
    }
    return 0;
}

uint8_t __fastcall HasPropertyController(NiAVObject* self, re::Edx) {
    for (Node* node = reinterpret_cast<List*>(&self->properties)->head; node != nullptr;) {
        NiObject* property = node->element;
        node = node->next;
        if (property != nullptr && At<void*>(property, 0xC) != nullptr) {
            return 1;
        }
    }
    return 0;
}

// vf24 SetSelectiveUpdateFlags(bSelectiveUpdate, bSelectiveUpdateTransforms, bRigid)
void __fastcall SetSelectiveUpdateFlags(NiAVObject* self, re::Edx, uint8_t* selectiveUpdate, int transforms,
                                        uint8_t* rigid) {
    uint8_t isRigid;
    if (AVObject_HasTransformController(self)) {
        isRigid = 0;
    } else {
        isRigid = (uint8_t(self->flags) >> 7) & 1 ? 0 : 1;
    }
    *rigid = isRigid;
    uint8_t updateTransforms = uint8_t(transforms);
    if (updateTransforms == 0) {
        updateTransforms = isRigid == 0;
    }
    uint8_t propertyControllers = 0;
    for (Node* node = reinterpret_cast<List*>(&self->properties)->head; node != nullptr;) {
        NiObject* property = node->element;
        node = node->next;
        if (property != nullptr && At<void*>(property, 0xC) != nullptr) {
            propertyControllers = 1;
            break;
        }
    }
    if (propertyControllers == 0 && updateTransforms == 0 && self->controller == nullptr) {
        *selectiveUpdate = 0;
        self->flags &= 0xFFFD;
        self->flags &= 0xFFFB;
    } else {
        *selectiveUpdate = 1;
        self->flags |= 2;
        if (updateTransforms != 0) {
            self->flags |= 4;
        } else {
            self->flags &= 0xFFFB;
        }
    }
    if (propertyControllers != 0) {
        self->flags |= 0x18;
    } else {
        self->flags &= 0xFFF7;
        self->flags |= 0x10;
    }
}

// ---- properties, parent, collision

// the first property of a type (NiProperty::Type, slot 18)
NiObject* __fastcall GetProperty(NiAVObject* self, re::Edx, int type) {
    for (Node* node = reinterpret_cast<List*>(&self->properties)->head; node != nullptr;) {
        NiObject* property = node->element;
        node = node->next;
        if (property != nullptr && Virtual<int>(property, 18) == type) {
            return property;
        }
    }
    return nullptr;
}

// RemoveProperty(eType): the first of that type, taken off the list
re::Ptr32<NiObject>* __fastcall RemoveProperty(NiAVObject* self, re::Edx, re::Ptr32<NiObject>* result, int type) {
    for (Node* node = reinterpret_cast<List*>(&self->properties)->head; node != nullptr;) {
        NiObject* property = node->element;
        node = node->next;
        if (property == nullptr) {
            continue;
        }
        re::Ptr32<NiObject> held = property;
        AddRef(held);
        if (Virtual<int>(property, 18) == type) {
            re::Ptr32<NiObject> removed;
            PropertyList_Remove(reinterpret_cast<List*>(&self->properties), &removed, &held);
            Unref(removed);
            *result = held;
            AddRef(held);
            Unref(held);
            return result;
        }
        Unref(held);
    }
    *result = nullptr;
    return result;
}

// the parent set (the old one gives the object up first)
void __fastcall AttachParent(NiAVObject* self, re::Edx, NiNode* parent) {
    if (self->parent != nullptr) {
        NiObject* detached;
        Virtual<void>(self->parent, 35, &detached, self);   // DetachChild
        Unref(detached);
    }
    self->parent = parent;
}

void __fastcall SetCollisionObject(NiAVObject* self, re::Edx, NiObject* collision) {
    NiObject* old = self->collisionObject;
    if (old != collision) {
        Unref(old);
        self->collisionObject = collision;
        AddRef(collision);
    }
    if (self->collisionObject != nullptr &&
        CollisionObject_GetSceneGraphObject(self->collisionObject) != static_cast<void*>(self)) {
        Virtual<void>(self->collisionObject, 18, self);   // SetSceneGraphObject
    }
}

// NiPropertyState::SetProperty: the property in its type's slot
void __fastcall PropertyState_SetProperty(NiObject* self, re::Edx, NiObject* property) {
    if (property == nullptr) {
        return;
    }
    int type = Virtual<int>(property, 18);
    re::Ptr32<NiObject>& slot = At<NiObject*>(self, 8 + type * 4);
    NiObject* old = slot;
    if (old != property) {
        Unref(old);
        slot = property;
        AddRef(property);
    }
}

// ---- culling, names, renderer data

// Cull(culler): handed to the culler (its slot 2) unless app culled
void __fastcall Cull(NiAVObject* self, re::Edx, void* culler) {
    if ((self->flags & 1) == 0) {
        Virtual<void>(culler, 2, self);
    }
}

// vf23
NiAVObject* __fastcall GetObjectByName(NiAVObject* self, re::Edx, const re::Ptr32<const char>* name) {
    if (*name == NullFixedString) {
        return nullptr;
    }
    return self->name == *name ? self : nullptr;
}

// vf33: the material property's renderer data purged (the renderer's slot 49)
void __fastcall PurgeRendererData(NiAVObject* self, re::Edx, void* renderer) {
    NiObject* material = AVObject_GetProperty(self, 3);
    if (material != nullptr) {
        Virtual<void>(renderer, 49, material);
    }
}

// ---- the property list (NiTPointerList<NiPropertyPtr>'s, one copy of each)

// RemoveAll: every node freed through the allocator (whatever the list's element type)
void __fastcall ListRemoveAll(List* self, re::Edx) {
    for (Node* node = self->head; node != nullptr;) {
        Node* next = node->next;
        FreeNode(self, node);
        node = next;
    }
    self->count = 0;
    self->head = nullptr;
    self->tail = nullptr;
}

// RemovePos(kPos): the element at the position, the position moved to the next
re::Ptr32<NiObject>* __fastcall ListRemovePos(List* self, re::Edx, re::Ptr32<NiObject>* result, re::Ptr32<Node>* pos) {
    Node* node = *pos;
    if (node == self->head) {
        *pos = node->next;
        PropertyList_RemoveHead(self, result);
        return result;
    }
    if (node == self->tail) {
        *pos = nullptr;
        PropertyList_RemoveTail(self, result);
        return result;
    }
    Node* prev = node->prev;
    Node* next = node->next;
    *pos = next;
    if (prev != nullptr) {
        prev->next = next;
    }
    if (next != nullptr) {
        next->prev = prev;
    }
    NiObject* element = node->element;
    AddRef(element);
    FreeNode(self, node);
    self->count--;
    *result = element;
    if (element != nullptr) {
        AddRef(element);
        Unref(element);
    }
    return result;
}

// Remove(element): the element itself back when it isn't there
re::Ptr32<NiObject>* __fastcall ListRemove(List* self, re::Edx, re::Ptr32<NiObject>* result, const re::Ptr32<NiObject>* element) {
    Node* found = nullptr;
    for (Node* node = self->head; node != nullptr; node = node->next) {
        if (*element == node->element) {
            found = node;
            break;
        }
    }
    re::Ptr32<NiObject> removed = nullptr;
    const re::Ptr32<NiObject>* from;
    if (found != nullptr) {
        re::Ptr32<Node> pos = found;
        from = PropertyList_RemovePos(self, &removed, &pos);
    } else {
        from = element;
    }
    NiObject* value = *from;
    *result = value;
    AddRef(value);
    if (found != nullptr) {
        Unref(removed);
    }
    return result;
}

// ---- construction

NiAVObject* __fastcall Construct(NiAVObject* self, re::Edx) {
    ObjectNET_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiAVObject_vtable);
    self->properties.count = 0;
    self->properties.head = nullptr;
    self->properties.tail = nullptr;
    self->properties.vtable = reinterpret_cast<void*>(PropertyList_vtable);
    self->collisionObject = nullptr;
    self->geometries.maxSize = 0;
    self->geometries.growBy = 1;
    self->geometries.size = 0;
    self->geometries.esize = 0;
    self->geometries.base = nullptr;
    self->geometries.vtable = reinterpret_cast<void*>(GeometryArray_vtable);
    self->parent = nullptr;
    self->flags = 0;
    self->unknownB0 = 0;
    for (uint16_t i = 0; i < self->geometries.size; i++) {   // (none: inlined RemoveAll)
        self->geometries.base[i] = nullptr;
    }
    self->geometries.size = 0;
    self->geometries.esize = 0;
    self->unknownAC = 1.0f;
    Transform_MakeIdentity(&self->local);
    Transform_MakeIdentity(&self->world);
    self->worldBound.center = NiPoint3_ZERO;
    self->worldBound.radius = 0.0f;
    self->flags = uint16_t((self->flags & 0xFFF1) | 0x10);
    if (self->collisionObject != nullptr) {
        Unref(self->collisionObject);
        self->collisionObject = nullptr;
    }
    return self;
}

void __fastcall Destruct(NiAVObject* self, re::Edx) {
    List* properties = reinterpret_cast<List*>(&self->properties);
    self->vtable = reinterpret_cast<void*>(NiAVObject_vtable);
    PointerList_RemoveAll(properties);
    if (self->collisionObject != nullptr) {
        Unref(self->collisionObject);
        self->collisionObject = nullptr;
    }
    // ~NiTPrimitiveArray<NiGeometry*>, ~NiCollisionObjectPtr (null by now), ~the list
    self->geometries.vtable = reinterpret_cast<void*>(GeometryArrayBase_vtable);
    NiFree(self->geometries.base);
    Unref(self->collisionObject);
    properties->vtable = reinterpret_cast<void*>(PropertyList_vtable);
    PointerList_RemoveAll(properties);
    properties->vtable = reinterpret_cast<void*>(PointerListBase_vtable);
    ObjectNET_Destruct(self);
}

// vf0
NiAVObject* __fastcall DeletingDestructor(NiAVObject* self, re::Edx, uint32_t flags) {
    AVObject_Destructor(self);
    if (flags & 1) {
        NiDelete(self, 0xC4);
    }
    return self;
}

// vf2
const RTTI* __fastcall GetRTTI(NiAVObject*, re::Edx) {
    return &NiAVObject_RTTI;
}

// ---- streaming

// vf4
void __fastcall LoadBinary(NiAVObject* self, re::Edx, void* stream) {
    ObjectNET_LoadBinary(self, stream);
    LoadBytes(stream, &self->flags, 2);
    Point3_LoadBinary(&self->local.translate, stream);
    Matrix3_LoadBinary(&self->local.rotate, stream);
    LoadBytes(stream, &self->local.scale, 4);
    Stream_ReadMultipleLinkIDs(stream);     // the properties
    Stream_ReadLinkID(stream);              // the collision object
    if (StreamVersion(stream) < 0x14000004) {
        self->flags &= 0xFFBF;
    }
}

// vf5
void __fastcall LinkObject(NiAVObject* self, re::Edx, void* stream) {
    ObjectNET_LinkObject(self, stream);
    for (uint32_t count = Stream_GetNumberOfLinkIDs(stream); count != 0;) {
        count--;
        NiObject* property = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        re::Ptr32<NiObject> held = property;
        AddRef(held);
        PropertyList_AddHead(reinterpret_cast<List*>(&self->properties), &held);
        Unref(property);
    }
    NiObject* collision = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
    NiObject* old = self->collisionObject;
    if (old != collision) {
        Unref(old);
        self->collisionObject = collision;
        AddRef(collision);
    }
    if (self->collisionObject != nullptr) {
        Virtual<void>(self->collisionObject, 22, StreamVersion(stream), 0);
    }
}

// vf6
uint8_t __fastcall RegisterStreamables(NiAVObject* self, re::Edx, void* stream) {
    if (!ObjectNET_RegisterStreamables(self, stream)) {
        return 0;
    }
    for (Node* node = reinterpret_cast<List*>(&self->properties)->head; node != nullptr;) {
        NiObject* property = node->element;
        node = node->next;
        if (property != nullptr) {
            Virtual<uint8_t>(property, 6, stream);
        }
    }
    if (self->collisionObject != nullptr) {
        Virtual<uint8_t>(self->collisionObject, 6, stream);
    }
    return 1;
}

// vf7: the properties saved last to first
void __fastcall SaveBinary(NiAVObject* self, re::Edx, void* stream) {
    ObjectNET_SaveBinary(self, stream);
    SaveBytes(stream, &self->flags, 2);
    Point3_SaveBinary(&self->local.translate, stream);
    Matrix3_SaveBinary(&self->local.rotate, stream);
    SaveBytes(stream, &self->local.scale, 4);
    int32_t count = int32_t(self->properties.count);
    SaveBytes(stream, &count, 4);
    if (count > 0) {
        re::Ptr32<NiObject>* properties = static_cast<re::Ptr32<NiObject>*>(NiMalloc(uint32_t(count) * 4));
        re::Ptr32<NiObject>* out = properties;
        for (Node* node = reinterpret_cast<List*>(&self->properties)->head; node != nullptr; node = node->next) {
            *out++ = node->element;
        }
        for (int32_t i = count - 1; i >= 0; i--) {
            Virtual<void>(stream, 12, properties[i]);   // SaveLinkID
        }
        NiFree(properties);
    }
    Virtual<void>(stream, 12, self->collisionObject);
}

// vf8
uint8_t __fastcall IsEqual(NiAVObject* self, re::Edx, NiAVObject* other) {
    if (!ObjectNET_IsEqual(self, other)) {
        return 0;
    }
    if (((uint8_t(other->flags) ^ uint8_t(self->flags)) & 1) != 0) {
        return 0;
    }
    if (Transform_NotEqual(&self->local, &other->local)) {
        return 0;
    }
    uint32_t count = self->properties.count;
    if (count != other->properties.count) {
        return 0;
    }
    if (count != 0) {
        Node* mine = reinterpret_cast<List*>(&self->properties)->head;
        Node* theirs = reinterpret_cast<List*>(&other->properties)->head;
        while (mine != nullptr) {
            NiObject* a = mine->element;
            mine = mine->next;
            NiObject* b = theirs->element;
            theirs = theirs->next;
            if (a == nullptr) {
                if (b != nullptr) {
                    return 0;
                }
            } else {
                if (b == nullptr || !Virtual<uint8_t>(a, 8, b)) {
                    return 0;
                }
            }
        }
    }
    NiObject* a = self->collisionObject;
    NiObject* b = other->collisionObject;
    if (a != nullptr && b != nullptr) {
        return Virtual<uint8_t>(a, 8, b) ? 1 : 0;
    }
    return a == b ? 1 : 0;
}

// vf9
void __fastcall GetViewerStrings(NiAVObject* self, re::Edx, NiTArray<char*>* strings) {
    ObjectNET_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiAVObject_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("m_bAppCulled", self->flags & 1));
    AddViewerString(strings, Point3_GetViewerString(&self->local.translate, "m_localTranslate"));
    AddViewerString(strings, Matrix3_GetViewerString(&self->local.rotate, "m_localRotate"));
    AddViewerString(strings, GetViewerStringFloat("m_fLocalScale", self->local.scale));
    AddViewerString(strings, Point3_GetViewerString(&self->world.translate, "m_worldTranslate"));
    AddViewerString(strings, Matrix3_GetViewerString(&self->world.rotate, "m_worldRotate"));
    AddViewerString(strings, GetViewerStringFloat("m_worldScale", self->world.scale));
    AddViewerString(strings, Bound_GetViewerString(&self->worldBound, "m_kWorldBound"));
    AddViewerString(strings, GetViewerStringBool("SelUpdate", (uint8_t(self->flags) >> 1) & 1));
    AddViewerString(strings, GetViewerStringBool("SelUpdateTransforms", (uint8_t(self->flags) >> 2) & 1));
    AddViewerString(strings, GetViewerStringBool("SelUpdateRigid", (uint8_t(self->flags) >> 4) & 1));
    AddViewerString(strings, GetViewerStringBool("SelUpdatePropControllers", (uint8_t(self->flags) >> 3) & 1));
    if (self->properties.count != 0) {
        for (Node* node = reinterpret_cast<List*>(&self->properties)->head; node != nullptr;) {
            NiObject* property = node->element;
            node = node->next;
            const RTTI* rtti = Virtual<const RTTI*>(property, 2);
            AddViewerStringInline(strings, GetViewerStringPointer(rtti->name, property));
        }
    }
    if (self->collisionObject != nullptr) {
        AddViewerString(strings, GetViewerStringPointer("m_spCollisionObject", self->collisionObject));
    }
}

// ---- cloning

// the property list's ProcessClone: each property not processed yet (the cloning process's
// process map, +4: a NiTPointerMap keyed by object)
void __stdcall ListProcessClone(List* list, void* cloning) {
    for (Node* node = list->head; node != nullptr;) {
        NiObject* property = node->element;
        node = node->next;
        void* map = At<void*>(cloning, 4);
        uint32_t buckets = At<uint32_t>(map, 4);
        re::Ptr32<void>* table = At<re::Ptr32<void>*>(map, 8);
        void* item = table[reinterpret_cast<uintptr_t>(property) % buckets];
        for (; item != nullptr; item = At<void*>(item, 0)) {
            if (At<NiObject*>(item, 4) == property) {
                break;
            }
        }
        if (item == nullptr) {
            Virtual<void>(property, 11, cloning);
        }
    }
}

// vf11
void __fastcall ProcessClone(NiAVObject* self, re::Edx, void* cloning) {
    ObjectNET_ProcessClone(self, cloning);
    if (self->properties.count != 0) {
        PropertyList_ProcessClone(reinterpret_cast<List*>(&self->properties), cloning);
    }
    if (self->collisionObject != nullptr) {
        Virtual<void>(self->collisionObject, 11, cloning);
    }
}

// the properties' clones put on the clone's list (last to first, each at the head)
void __stdcall ListCopyTo(NiAVObject* dest, List* list, void* cloning) {
    for (Node* node = list->tail; node != nullptr;) {
        NiObject* property = node->element;
        node = node->prev;
        NiObject* clone = Object_Clone(property, cloning);
        re::Ptr32<NiObject> held = clone;
        AddRef(held);
        PropertyList_AddHead(reinterpret_cast<List*>(&dest->properties), &held);
        Unref(clone);
    }
}

// CopyMembers(dest, cloning)
void __fastcall CopyMembers(NiAVObject* self, re::Edx, NiAVObject* dest, void* cloning) {
    ObjectNET_CopyMembers(self, dest, cloning);
    dest->flags = self->flags;
    std::memcpy(&dest->local, &self->local, sizeof(NiTransform));
    if (self->properties.count != 0) {
        PropertyList_CopyTo(dest, reinterpret_cast<List*>(&self->properties), cloning);
    }
    if (self->collisionObject != nullptr) {
        NiObject* clone = Virtual<NiObject*>(self->collisionObject, 3, cloning);   // CreateClone
        AVObject_SetCollisionObject(dest, clone);
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIAV_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIAV_REVERSED("NiAVObject::Update", 0x0075DB00, &ni::Update);
NIAV_REVERSED("NiTPointerList::RemoveAll", 0x0075DB60, &ni::ListRemoveAll);
NIAV_REVERSED("NiAVObject::AttachParent", 0x0075DBA0, &ni::AttachParent);
NIAV_REVERSED("NiAVObject::GetProperty", 0x0075DC10, &ni::GetProperty);
NIAV_REVERSED("NiAVObject::Cull", 0x0075DC50, &ni::Cull);
NIAV_REVERSED("NiAVObject::GetObjectByName", 0x0075DC70, &ni::GetObjectByName);
NIAV_REVERSED("NiAVObject::PurgeRendererData", 0x0075DCA0, &ni::PurgeRendererData);
NIAV_REVERSED("NiAVObject::RegisterStreamables", 0x0075DCC0, &ni::RegisterStreamables);
NIAV_REVERSED("NiAVObject::UpdateObjectControllers", 0x0075DD20, &ni::UpdateObjectControllers);
NIAV_REVERSED("NiAVObject::SetCollisionObject", 0x0075DD80, &ni::SetCollisionObject);
NIAV_REVERSED("NiPropertyState::SetProperty", 0x0075DDF0, &ni::PropertyState_SetProperty);
NIAV_REVERSED("NiTPointerList<NiPropertyPtr>::RemovePos", 0x0075DE40, &ni::ListRemovePos);
NIAV_REVERSED("NiAVObject::~NiAVObject", 0x0075DEF0, &ni::Destruct);
NIAV_REVERSED("NiAVObject::GetRTTI", 0x0075DF90, &ni::GetRTTI);
NIAV_REVERSED("NiAVObject::UpdateDownwardPass", 0x0075DFA0, &ni::UpdateDownwardPass);
NIAV_REVERSED("NiAVObject::UpdateSelectedDownwardPass", 0x0075DFD0, &ni::UpdateSelectedDownwardPass);
NIAV_REVERSED("NiAVObject::UpdateControllers", 0x0075E020, &ni::UpdateControllers);
NIAV_REVERSED("NiAVObject::UpdateProperties", 0x0075E040, &ni::UpdateProperties);
NIAV_REVERSED("NiAVObject::HasTransformController", 0x0075E0E0, &ni::HasTransformController);
NIAV_REVERSED("NiAVObject::HasPropertyController", 0x0075E110, &ni::HasPropertyController);
NIAV_REVERSED("NiAVObject::PushLocalProperties", 0x0075E140, &ni::PushLocalProperties);
NIAV_REVERSED("NiAVObject::UpdateEffects", 0x0075E240, &ni::UpdateEffects);
NIAV_REVERSED("NiAVObject::ProcessCloneProperties", 0x0075E2C0, &ni::ListProcessClone);
NIAV_REVERSED("NiAVObject::ProcessClone", 0x0075E310, &ni::ProcessClone);
NIAV_REVERSED("NiAVObject::LoadBinary", 0x0075E350, &ni::LoadBinary);
NIAV_REVERSED("NiAVObject::LinkObject", 0x0075E3F0, &ni::LinkObject);
NIAV_REVERSED("NiAVObject::SaveBinary", 0x0075E4C0, &ni::SaveBinary);
NIAV_REVERSED("NiAVObject::IsEqual", 0x0075E5E0, &ni::IsEqual);
NIAV_REVERSED("NiTPointerList<NiPropertyPtr>::Remove", 0x0075E6B0, &ni::ListRemove);
NIAV_REVERSED("NiAVObject::NiAVObject", 0x0075E750, &ni::Construct);
NIAV_REVERSED("NiAVObject::DeletingDestructor", 0x0075E870, &ni::DeletingDestructor);
NIAV_REVERSED("NiAVObject::RemoveProperty", 0x0075E8A0, &ni::RemoveProperty);
NIAV_REVERSED("NiAVObject::SetSelectiveUpdateFlags", 0x0075E980, &ni::SetSelectiveUpdateFlags);
NIAV_REVERSED("NiAVObject::CopyProperties", 0x0075EA60, &ni::ListCopyTo);
NIAV_REVERSED("NiAVObject::CopyMembers", 0x0075EAE0, &ni::CopyMembers);
NIAV_REVERSED("NiAVObject::GetViewerStrings", 0x0075EB60, &ni::GetViewerStrings);
