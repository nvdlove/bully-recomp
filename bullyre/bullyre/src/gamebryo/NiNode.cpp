// NiNode (vtable 0x951BF8, 0xF4 bytes): an NiAVObject with children (an NiTObjectArray of
// references: NiAVObject::parent points back) and the dynamic effects attached to it. Its own
// virtual slots: 34 AttachChild, 35 DetachChild, 36 DetachChildAt, 37 SetAt, 38 UpdateUpwardPass;
// the update passes recurse over the children, merging their world bounds into its own.
//
// Also NiDynamicEffectState's constructor (the lights, shadows and maps pushed down the graph:
// 0x20 bytes) and NiTObjectArray<NiAVObjectPtr>'s Add / AddFirstEmpty.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "game/HeapCheck.h"
#include "log.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiNode_RTTI, 0x020E2548);

constexpr uintptr_t NiNode_vtable = 0x00951BF8;
constexpr uintptr_t ChildArray_vtable = 0x009893C0;      // NiTObjectArray<NiAVObjectPtr>
constexpr uintptr_t EffectList_vtable = 0x00952E10;      // NiTPointerList<NiDynamicEffect*>
constexpr uintptr_t PointerListBase_vtable = 0x0099C748;
constexpr uintptr_t RefObject_vtable = 0x008FEFD4;
constexpr uintptr_t DynamicEffectState_vtable = 0x00951BEC;

GAME_VAR(volatile long, NiRefObject_ms_uiObjects, 0x020E2168);

using Node = NiTListNode<NiObject*>;
using List = NiTPointerList<NiObject*>;
using ChildArray = NiTArray<NiAVObject*>;

inline List* Effects(NiNode* node) {
    return reinterpret_cast<List*>(&node->effects);
}

// the merge of a child's world bound into the node's (a zero radius: nothing yet)
GAME_FN(void, __thiscall, Bound_Merge, 0x007D2E60, NiBound* self, const NiBound* other);
GAME_FN(void, __thiscall, Bound_Update, 0x007D2FD0, NiBound* self, const NiBound* bound, const NiTransform* xform);
GAME_FN(void, __thiscall, Transform_Invert, 0x00770950, const NiTransform* self, NiTransform* result);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Multiply, 0x00768920, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_TransposeTimes, 0x00768F60, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(NiPoint3*, __thiscall, Matrix3_MultiplyPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(NiPoint3*, __cdecl, Point3_TimesMatrix, 0x00768AD0, NiPoint3* result, const NiPoint3* point,
        const NiMatrix3* matrix);
// NiTObjectArray<NiAVObjectPtr>
GAME_FN(void, __thiscall, ChildArray_SetSize, 0x00856170, ChildArray* self, uint32_t size);
GAME_FN(void, __thiscall, ChildArray_SetAt, 0x0040E800, ChildArray* self, uint32_t index, const re::Ptr32<NiAVObject>* child);
GAME_FN(re::Ptr32<NiAVObject>*, __thiscall, ChildArray_RemoveAt, 0x00818530, ChildArray* self, re::Ptr32<NiAVObject>* result,
        uint32_t index);
GAME_FN(re::Ptr32<NiAVObject>*, __cdecl, ChildArray_Allocate, 0x00856120, uint32_t count);
GAME_FN(void, __cdecl, ChildArray_Free, 0x00836F80, re::Ptr32<NiAVObject>* base);
// NiDynamicEffect, NiDynamicEffectState, the shadow generator
GAME_FN(uint8_t, __thiscall, Effect_IsUnaffectedNode, 0x00771D70, NiObject* effect, NiNode* node);
GAME_FN(void, __thiscall, Effect_AttachAffectedNode, 0x00771A10, NiObject* effect, NiNode* node);
GAME_FN(void, __thiscall, Effect_DetachAffectedNode, 0x00771F10, NiObject* effect, NiNode* node);
GAME_FN(void, __thiscall, ShadowGenerator_MarkDirty, 0x00772930, void* generator);
GAME_FN(NiObject*, __thiscall, EffectState_Copy, 0x007D2580, NiObject* self);
GAME_FN(void, __thiscall, EffectState_AddEffect, 0x007D2700, NiObject* self, NiObject* effect);
GAME_FN(void, __thiscall, EffectState_RemoveEffect, 0x007D2790, NiObject* self, NiObject* effect);
GAME_FN(void, __thiscall, EffectState_Destruct, 0x007D2500, NiObject* self);
GAME_FN(NiObject*, __thiscall, EffectState_Construct, 0x0075F000, void* self);
GAME_FN(NiObject*, __thiscall, PropertyState_Construct, 0x007D2370, void* self);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
GAME_FN(NiObject*, __thiscall, EffectList_RemovePos, 0x00771DA0, List* self, re::Ptr32<Node>* pos);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// (this chunk's, called where modes apply)
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Node_UpdatePropertiesUpward, 0x0075FAA0, NiNode* self, re::Ptr32<NiObject>* result);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Node_UpdateEffectsUpward, 0x00760490, NiNode* self, re::Ptr32<NiObject>* result);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Node_PushLocalEffects, 0x0075FB50, NiNode* self, re::Ptr32<NiObject>* result,
        NiObject* parentState, int copy);
GAME_FN(void, __thiscall, Node_AttachEffect, 0x0075F4A0, NiNode* self, NiObject* effect);
GAME_FN(void, __thiscall, Node_DetachAllEffects, 0x0075F510, NiNode* self);
GAME_FN(NiObject*, __thiscall, EffectList_Remove, 0x0075F630, List* self, const re::Ptr32<NiObject>* effect);
GAME_FN(re::Ptr32<NiAVObject>*, __thiscall, Node_DetachChildAt, 0x0075F680, NiNode* self, re::Ptr32<NiAVObject>* result, uint32_t index);
GAME_FN(void, __thiscall, Node_CopyMembers, 0x0075F570, NiNode* self, NiNode* dest, void* cloning);
GAME_FN(void, __thiscall, Node_CopyEffects, 0x007600E0, NiNode* self, List* effects, void* cloning);
GAME_FN(void, __thiscall, Node_Destructor, 0x007603B0, NiNode* self);
GAME_FN(NiNode*, __thiscall, Node_Construct, 0x00760780, void* self, uint32_t childCount);
GAME_FN(uint32_t, __thiscall, ChildArray_Add, 0x00760680, ChildArray* self, const re::Ptr32<NiAVObject>* child);
GAME_FN(uint32_t, __thiscall, ChildArray_AddFirstEmpty, 0x007606C0, ChildArray* self, const re::Ptr32<NiAVObject>* child);
GAME_FN(void, __thiscall, AVObject_Cull, 0x0075DC50, NiAVObject* self, void* culler);

// a child's world bound taken into the node's (merged, or copied while the node has none)
inline void GrowBound(NiNode* self, NiAVObject* child) {
    if (child->worldBound.radius != 0.0f) {
        if (self->worldBound.radius == 0.0f) {
            self->worldBound = child->worldBound;
        } else {
            Bound_Merge(&self->worldBound, &child->worldBound);
        }
    }
}

// ---- NiDynamicEffectState

NiObject* __fastcall EffectStateConstruct(NiObject* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(&NiRefObject_ms_uiObjects);
    At<uint8_t>(self, 8) = 0;
    At<void*>(self, 0xC) = nullptr;    // the light list
    At<void*>(self, 0x10) = nullptr;   // the projected lights
    At<void*>(self, 0x14) = nullptr;   // the projected shadows
    At<void*>(self, 0x18) = nullptr;   // the environment map
    At<void*>(self, 0x1C) = nullptr;   // the fog map
    self->vtable = reinterpret_cast<void*>(DynamicEffectState_vtable);
    heapcheck::Trace(self, heapcheck::kAlloc);   // (testing: see NiPropertyState.cpp's destructor)
    return self;
}

NiObject* __fastcall EffectStateDeletingDestructor(NiObject* self, re::Edx, uint32_t flags) {
    EffectState_Destruct(self);
    if (flags & 1) {
        heapcheck::SetContext(self);   // (testing: the states' churn is kept out of the quarantine)
        NiDelete(self, 0x20);
        heapcheck::SetContext(nullptr);
    }
    return self;
}

// ---- the update passes

// vf21 SetMaterialNeedsUpdate(bNeedsUpdate) / vf22 SetDefaultMaterialNeedsUpdateFlag(bFlag):
// passed on to the children (NiAVObject's do nothing, NiGeometry's set them)
void __fastcall SetMaterialNeedsUpdate(NiNode* self, re::Edx, void* arg) {
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 21, arg);
        }
    }
}

void __fastcall SetDefaultMaterialNeedsUpdateFlag(NiNode* self, re::Edx, void* arg) {
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 22, arg);
        }
    }
}

// vf20 ApplyTransform(kMat, kTrn, bOnLeft): on the left, the transform taken into the node's
// frame for the children (always applied on their left)
void __fastcall ApplyTransform(NiNode* self, re::Edx, const NiMatrix3* matrix, const NiPoint3* translate,
                               int onLeft) {
    if (re::Flag(onLeft)) {
        NiMatrix3 product;
        NiMatrix3 rotate;
        Matrix3_TransposeTimes(&self->local.rotate, &rotate,
                               Matrix3_Multiply(matrix, &product, &self->local.rotate));
        NiPoint3 moved;
        const NiPoint3* m = Matrix3_MultiplyPoint(matrix, &moved, &self->local.translate);
        NiPoint3 sum;
        sum.x = m->x + translate->x;
        sum.y = m->y + translate->y;
        sum.z = m->z + translate->z;
        NiPoint3 delta;
        delta.x = sum.x - self->local.translate.x;
        delta.y = sum.y - self->local.translate.y;
        delta.z = sum.z - self->local.translate.z;
        float scale = self->local.scale;
        NiPoint3 local;
        const NiPoint3* r = Point3_TimesMatrix(&local, &delta, &self->local.rotate);
        float inverse = 1.0f / scale;
        NiPoint3 childTranslate;
        float y = r->y * inverse;
        float z = r->z * inverse;
        childTranslate.x = inverse * r->x;
        childTranslate.y = y;
        childTranslate.z = z;
        for (uint32_t i = 0; i < self->children.size; i++) {
            NiAVObject* child = self->children.base[i];
            if (child != nullptr) {
                Virtual<void>(child, 20, &rotate, &childTranslate, 1);
            }
        }
    } else {
        for (uint32_t i = 0; i < self->children.size; i++) {
            NiAVObject* child = self->children.base[i];
            if (child != nullptr) {
                Virtual<void>(child, 20, matrix, translate, 1);
            }
        }
    }
}

// vf38 UpdateUpwardPass: the bound, then the parent's
void __fastcall UpdateUpwardPass(NiNode* self, re::Edx) {
    NiNode* parent = self->parent;
    Virtual<void>(self, 31);   // UpdateWorldBound
    if (parent != nullptr) {
        Virtual<void>(parent, 38);
    }
}

// vf31 UpdateWorldBound: the children's merged
void __fastcall UpdateWorldBound(NiNode* self, re::Edx) {
    self->worldBound.radius = 0.0f;
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            GrowBound(self, child);
        }
    }
}

// vf28 UpdatePropertiesDownward
void __fastcall UpdatePropertiesDownward(NiNode* self, re::Edx, NiObject* parentState) {
    re::Ptr32<NiObject> state;
    AVObject_PushLocalProperties(self, &state, parentState, 1);
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 28, state);
        }
    }
    Release(state);
}

// vf19 UpdateNodeBound: the children's, then the model bound from the world one
void __fastcall UpdateNodeBound(NiNode* self, re::Edx) {
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 19);
        }
    }
    NiTransform inverse;
    Transform_Invert(&self->world, &inverse);
    Bound_Update(&self->bound, &self->worldBound, &inverse);
}

// vf32 OnVisible: the children culled in turn (when there's a bound)
void __fastcall OnVisible(NiNode* self, re::Edx, void* culler) {
    if (self->worldBound.radius != 0.0f) {
        for (uint32_t i = 0; i < self->children.size; i++) {
            NiAVObject* child = self->children.base[i];
            if (child != nullptr) {
                AVObject_Cull(child, culler);
            }
        }
    }
}

// vf33 PurgeRendererData: its effects' (the renderer's slot 50) and the children's
void __fastcall PurgeRendererData(NiNode* self, re::Edx, void* renderer) {
    AVObject_PurgeRendererData(self, renderer);
    for (Node* node = Effects(self)->head; node != nullptr;) {
        NiObject* effect = node->element;
        node = node->next;
        Virtual<void>(renderer, 50, effect);
    }
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 33, renderer);
        }
    }
}

// vf25 UpdateDownwardPass
void __fastcall UpdateDownwardPass(NiNode* self, re::Edx, float time, int updateControllers) {
    if (uint8_t(updateControllers) != 0) {
        AVObject_UpdateObjectControllers(self, time, 1);
    }
    Virtual<void>(self, 30);   // UpdateWorldData
    self->worldBound.radius = 0.0f;
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 25, time, updateControllers);
            GrowBound(self, child);
        }
    }
}

// vf26 UpdateSelectedDownwardPass: the selected children, rigid ones by their own pass
void __fastcall UpdateSelectedDownwardPass(NiNode* self, re::Edx, float time) {
    AVObject_UpdateObjectControllers(self, time, (uint8_t(self->flags) >> 3) & 1);
    if ((uint8_t(self->flags) >> 2) & 1) {
        Virtual<void>(self, 30);
    }
    self->worldBound.radius = 0.0f;
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child == nullptr) {
            continue;
        }
        uint8_t flags = uint8_t(child->flags);
        if ((flags >> 1) & 1) {
            Virtual<void>(child, ((flags >> 4) & 1) ? 27 : 26, time);
        }
        GrowBound(self, child);
    }
}

// vf27 UpdateRigidDownwardPass: the world bound from the model bound (no merging)
void __fastcall UpdateRigidDownwardPass(NiNode* self, re::Edx, float time) {
    AVObject_UpdateObjectControllers(self, time, (uint8_t(self->flags) >> 3) & 1);
    if ((uint8_t(self->flags) >> 2) & 1) {
        Virtual<void>(self, 30);
        Bound_Update(&self->worldBound, &self->bound, &self->world);
    }
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr && ((uint8_t(child->flags) >> 1) & 1)) {
            Virtual<void>(child, 27, time);
        }
    }
}

// vf18 UpdateControllers
void __fastcall UpdateControllers(NiNode* self, re::Edx, float time) {
    AVObject_UpdateObjectControllers(self, time, 1);
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 18, time);
        }
    }
}

// vf24 SetSelectiveUpdateFlags(bSelectiveUpdate, bSelectiveUpdateTransforms, bRigid): from the
// children up
void __fastcall SetSelectiveUpdateFlags(NiNode* self, re::Edx, uint8_t* selectiveUpdate, int transformsArg,
                                        uint8_t* rigid) {
    uint8_t transformController = AVObject_HasTransformController(self);
    uint8_t transforms = uint8_t(transformsArg);
    if (transforms == 0) {
        if (transformController != 0) {
            transforms = 1;
        } else {
            transforms = ((uint8_t(self->flags) >> 7) & 1) ? 1 : 0;
        }
    }
    // (the children get the argument's dword with its low byte replaced, as the game passes it)
    int passed = int((uint32_t(transformsArg) & 0xFFFFFF00u) | transforms);
    *selectiveUpdate = transforms;
    *rigid = 1;
    for (uint32_t i = 0; i < self->children.size; i++) {
        if (i >= self->children.size) {
            continue;
        }
        NiAVObject* child = self->children.base[i];
        if (child == nullptr) {
            continue;
        }
        uint8_t childSelective = 0;
        uint8_t childRigid = 1;
        Virtual<void>(child, 24, &childSelective, passed, &childRigid);
        if (childSelective != 0) {
            *selectiveUpdate = 1;
        }
        if (childRigid == 0) {
            *rigid = 0;
        }
    }
    uint8_t propertyController = AVObject_HasPropertyController(self);
    if (propertyController != 0 || self->controller != nullptr) {
        *selectiveUpdate = 1;
        self->flags |= 2;
    } else if (*selectiveUpdate != 0) {
        self->flags |= 2;
    } else {
        self->flags &= 0xFFFD;
    }
    if (transforms != 0) {
        self->flags |= 4;
    } else {
        self->flags &= 0xFFFB;
    }
    if (propertyController != 0) {
        self->flags |= 8;
    } else {
        self->flags &= 0xFFF7;
    }
    if (*rigid != 0) {
        self->flags |= 0x10;
    } else {
        self->flags &= 0xFFEF;
    }
    if (transformController != 0) {
        *rigid = 0;
    }
}

// ---- property and effect state

// UpdatePropertiesUpward: the state from the root down to this node (a new empty one at the root)
re::Ptr32<NiObject>* __fastcall UpdatePropertiesUpward(NiNode* self, re::Edx, re::Ptr32<NiObject>* result) {
    NiObject* state = nullptr;
    if (self->parent != nullptr) {
        re::Ptr32<NiObject> upward;
        NiObject* got = *Node_UpdatePropertiesUpward(self->parent, &upward);
        if (got != nullptr) {
            state = got;
            AddRef(state);
        }
        Release(upward);
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
    AVObject_PushLocalProperties(self, result, state, 0);
    Release(state);
    return result;
}

// UpdateEffectsUpward likewise (no state at the root)
re::Ptr32<NiObject>* __fastcall UpdateEffectsUpward(NiNode* self, re::Edx, re::Ptr32<NiObject>* result) {
    NiObject* state = nullptr;
    if (self->parent != nullptr) {
        re::Ptr32<NiObject> upward;
        NiObject* got = *Node_UpdateEffectsUpward(self->parent, &upward);
        if (got != nullptr) {
            state = got;
            AddRef(state);
        }
        Release(upward);
    }
    Node_PushLocalEffects(self, result, state, 0);
    Release(state);
    return result;
}

// the parent's effects that leave this node out are taken off (on a copy), the node's own put on
// (bCopyOnChange: on a copy)
re::Ptr32<NiObject>* __fastcall PushLocalEffects(NiNode* self, re::Edx, re::Ptr32<NiObject>* result, NiObject* parentState, int copy) {
    re::Ptr32<NiObject> state = parentState;
    AddRef(state);
    heapcheck::Trace(state, heapcheck::kAddRef);
    uint8_t copied = 0;
    if (parentState != nullptr) {
        // the three effect lists ({next, effect} items): the lights, then two more
        for (int list = 0; list < 3; list++) {
            void* item = At<void*>(parentState, 0xC + list * 4);
            void* previousItem = nullptr;
            while (item != nullptr) {
                heapcheck::CheckItem(item, previousItem, "NiNode::PushLocalEffects");   // (testing)
                previousItem = item;
                NiObject* effect = At<NiObject*>(item, 4);
                item = At<void*>(item, 0);
                if (effect == nullptr) {
                    continue;
                }
                if (Effect_IsUnaffectedNode(effect, self)) {
                    if (!copied) {
                        NiObject* copy2 = EffectState_Copy(parentState);
                        if (state != copy2) {
                            Release(state);
                            state = copy2;
                            AddRef(copy2);
                        }
                        copied = 1;
                    }
                    EffectState_RemoveEffect(state, effect);
                }
                if (list == 0) {
                    // the light's shadow generator marked dirty
                    void* generator = At<void*>(effect, 0xD4);
                    if (generator != nullptr) {
                        At<uint16_t>(generator, 8) |= 2;
                        if (((uint8_t(At<uint16_t>(generator, 8)) >> 2) & 1) == 0) {
                            ShadowGenerator_MarkDirty(generator);
                        }
                    }
                }
            }
        }
        // the two single effects (environment map, fog map)
        NiObject* first = At<NiObject*>(parentState, 0x18);
        NiObject* second = At<NiObject*>(parentState, 0x1C);
        if (first != nullptr && Effect_IsUnaffectedNode(first, self)) {
            if (!copied) {
                Pointer_Assign(&state, EffectState_Copy(parentState));
                copied = 1;
            }
            EffectState_RemoveEffect(state, first);
        }
        if (second != nullptr && Effect_IsUnaffectedNode(second, self)) {
            if (!copied) {
                Pointer_Assign(&state, EffectState_Copy(parentState));
                copied = 1;
            }
            EffectState_RemoveEffect(state, second);
        }
    }
    if (Effects(self)->count != 0) {
        bool assign = true;
        NiObject* from = nullptr;
        if (parentState == nullptr) {
            void* p = NiNew(0x20);
            from = p != nullptr ? EffectState_Construct(p) : nullptr;
        } else if (uint8_t(copy) != 0) {
            if (copied) {
                assign = false;
            } else {
                from = EffectState_Copy(parentState);
            }
        } else {
            if (copied) {
                assign = false;
            } else {
                from = parentState;
            }
        }
        if (assign) {
            Pointer_Assign(&state, from);
        }
        for (Node* node = Effects(self)->head; node != nullptr;) {
            NiObject* effect = node->element;
            node = node->next;
            EffectState_AddEffect(state, effect);
        }
    }
    *result = state;
    AddRef(state);
    Release(state);
    heapcheck::Trace(state, heapcheck::kAddRef);   // (the push's result handed over: net 0 here)
    return result;
}

// vf29 UpdateEffectsDownward
void __fastcall UpdateEffectsDownward(NiNode* self, re::Edx, NiObject* parentState) {
    heapcheck::Enter(self, "NiNode::UpdateEffectsDownward");   // (testing: see HeapCheck.h)
    re::Ptr32<NiObject> state;
    Node_PushLocalEffects(self, &state, parentState, 1);
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            // (testing: a child under two nodes is updated by two threads at once)
            if (heapcheck::g_validateFrees && child->parent != self) {
                static int reports = 0;
                if (reports < 16) {
                    reports++;
                    const char* name = At<const char*>(self, 8);
                    const char* childName = At<const char*>(child, 8);
                    re::Log("heap_check: node %p (%s): child %p (%s) at slot %u has parent %p", static_cast<void*>(self),
                            name ? name : "", static_cast<void*>(child), childName ? childName : "", i,
                            static_cast<void*>(child->parent));
                    heapcheck::LogStack();
                }
            }
            Virtual<void>(child, 29, state);
        }
    }
    heapcheck::Trace(state, heapcheck::kRelease);
    Release(state);
    heapcheck::Leave(self);
}

// AttachEffect: at the head of the list (once), the effect told
void __fastcall AttachEffect(NiNode* self, re::Edx, NiObject* effect) {
    List* effects = Effects(self);
    for (Node* node = effects->head; node != nullptr; node = node->next) {
        if (effect == node->element) {
            return;
        }
    }
    Node* node = Virtual<Node*>(effects, 1);   // the allocator's new node
    node->element = effect;
    node->prev = nullptr;
    node->next = effects->head;
    if (effects->head != nullptr) {
        effects->head->prev = node;
    } else {
        effects->tail = node;
    }
    effects->count++;
    effects->head = node;
    Effect_AttachAffectedNode(effect, self);
}

void __fastcall DetachAllEffects(NiNode* self, re::Edx) {
    List* effects = Effects(self);
    while (effects->count != 0) {
        Node* node = effects->head;
        NiObject* effect = node->element;
        Node* next = node->next;
        effects->head = next;
        if (next != nullptr) {
            next->prev = nullptr;
        } else {
            effects->tail = nullptr;
        }
        Virtual<void>(effects, 2, node);
        effects->count--;
        Effect_DetachAffectedNode(effect, self);
    }
}

void __fastcall DetachEffect(NiNode* self, re::Edx, re::Ptr32<NiObject> effect) {
    List* effects = Effects(self);
    for (Node* node = effects->head; node != nullptr; node = node->next) {
        if (effect == node->element) {
            EffectList_Remove(effects, &effect);
            Effect_DetachAffectedNode(effect, self);
            return;
        }
    }
}

// NiTPointerList<NiDynamicEffect*>::Remove(element): the element (itself when it isn't there)
NiObject* __fastcall EffectListRemove(List* self, re::Edx, const re::Ptr32<NiObject>* effect) {
    Node* found = nullptr;
    for (Node* node = self->head; node != nullptr; node = node->next) {
        if (*effect == node->element) {
            found = node;
            break;
        }
    }
    if (found != nullptr) {
        re::Ptr32<Node> pos = found;
        return EffectList_RemovePos(self, &pos);
    }
    return *effect;
}

// ---- children

// vf34 AttachChild(pkChild, bFirstAvail)
void __fastcall AttachChild(NiNode* self, re::Edx, NiAVObject* child, int firstAvail) {
    if (child == nullptr || child->parent == self) {
        return;
    }
    AddRef(child);
    AVObject_AttachParent(child, self);
    re::Ptr32<NiAVObject> held = child;
    AddRef(child);
    if (uint8_t(firstAvail) != 0) {
        ChildArray_AddFirstEmpty(&self->children, &held);
    } else {
        ChildArray_Add(&self->children, &held);
    }
    Release(child);
    Release(child);
}

// vf35 DetachChild(pkChild): the child back (null when it isn't one)
re::Ptr32<NiAVObject>* __fastcall DetachChild(NiNode* self, re::Edx, re::Ptr32<NiAVObject>* result, NiAVObject* child) {
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* c = self->children.base[i];
        if (c == nullptr) {
            continue;
        }
        AddRef(c);
        if (c == child) {
            c->parent = nullptr;
            re::Ptr32<NiAVObject> removed;
            ChildArray_RemoveAt(&self->children, &removed, i);
            Release(removed);
            *result = c;
            AddRef(c);
            Release(c);
            return result;
        }
        Release(c);
    }
    *result = nullptr;
    return result;
}

// vf36 DetachChildAt(i)
re::Ptr32<NiAVObject>* __fastcall DetachChildAt(NiNode* self, re::Edx, re::Ptr32<NiAVObject>* result, uint32_t index) {
    if (index >= self->children.size) {
        *result = nullptr;
        return result;
    }
    NiAVObject* child = self->children.base[index];
    if (child != nullptr) {
        AddRef(child);
        child->parent = nullptr;
        re::Ptr32<NiAVObject> removed;
        ChildArray_RemoveAt(&self->children, &removed, index);
        Release(removed);
    }
    *result = child;
    if (child != nullptr) {
        AddRef(child);
        Release(child);
    }
    return result;
}

// vf37 SetAt(i, pkChild): the child that was there back
re::Ptr32<NiAVObject>* __fastcall SetAt(NiNode* self, re::Edx, re::Ptr32<NiAVObject>* result, uint32_t index, NiAVObject* child) {
    if (index >= self->children.size) {
        if (child != nullptr) {
            AVObject_AttachParent(child, self);
        }
        re::Ptr32<NiAVObject> held = child;
        AddRef(child);
        if (index >= self->children.maxSize) {
            ChildArray_SetSize(&self->children, self->children.growBy + index);
        }
        ChildArray_SetAt(&self->children, index, &held);
        Release(child);
        *result = nullptr;
        return result;
    }
    NiAVObject* old = self->children.base[index];
    if (old != nullptr) {
        AddRef(old);
        old->parent = nullptr;
    }
    if (child != nullptr) {
        AVObject_AttachParent(child, self);
    }
    re::Ptr32<NiAVObject> held = child;
    AddRef(child);
    ChildArray_SetAt(&self->children, index, &held);
    Release(child);
    *result = old;
    if (old != nullptr) {
        AddRef(old);
        Release(old);
    }
    return result;
}

// NiTObjectArray<NiAVObjectPtr>::Add: at the end, grown by growBy when full
uint32_t __fastcall ChildArrayAdd(ChildArray* self, re::Edx, const re::Ptr32<NiAVObject>* child) {
    uint32_t index = self->size;
    if (index >= self->maxSize) {
        ChildArray_SetSize(self, self->growBy + index);
    }
    ChildArray_SetAt(self, index, child);
    return index;
}

// AddFirstEmpty: the first empty slot (-1 for a null element)
uint32_t __fastcall ChildArrayAddFirstEmpty(ChildArray* self, re::Edx, const re::Ptr32<NiAVObject>* child) {
    NiAVObject* value = *child;
    if (value == nullptr) {
        return 0xFFFFFFFF;
    }
    uint16_t size = self->size;
    for (uint16_t i = 0; i < self->size; i++) {
        if (self->base[i] == nullptr) {
            NiAVObject* old = self->base[i];
            if (old != value) {
                Release(old);
                self->base[i] = *child;
                AddRef(self->base[i]);
            }
            self->esize++;
            return i;
        }
    }
    uint32_t index = size;
    if (index >= self->maxSize) {
        ChildArray_SetSize(self, self->growBy + index);
    }
    ChildArray_SetAt(self, index, child);
    return index;
}

// vf23 GetObjectByName: this one, else the first child's answer
NiAVObject* __fastcall GetObjectByName(NiNode* self, re::Edx, const re::Ptr32<const char>* name) {
    NiAVObject* found = reinterpret_cast<NiAVObject*>(AVObject_GetObjectByName(self, name));
    if (found != nullptr) {
        return found;
    }
    for (uint32_t i = 0; i < self->children.size; i++) {
        if (i >= self->children.size) {
            continue;
        }
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            NiAVObject* r = Virtual<NiAVObject*>(child, 23, name);
            if (r != nullptr) {
                return r;
            }
        }
    }
    return nullptr;
}

// ---- construction

NiNode* __fastcall Construct(NiNode* self, re::Edx, uint32_t childCountArg) {
    uint16_t childCount = uint16_t(childCountArg);
    AVObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiNode_vtable);
    self->children.vtable = reinterpret_cast<void*>(ChildArray_vtable);
    self->children.maxSize = childCount;
    self->children.growBy = 1;
    self->children.size = 0;
    self->children.esize = 0;
    self->children.base = childCount != 0 ? ChildArray_Allocate(childCount) : nullptr;
    self->children.vtable = reinterpret_cast<void*>(ChildArray_vtable);
    self->effects.count = 0;
    self->effects.head = nullptr;
    self->effects.tail = nullptr;
    self->effects.vtable = reinterpret_cast<void*>(EffectList_vtable);
    self->flags |= 0x100;
    return self;
}

void __fastcall Destruct(NiNode* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiNode_vtable);
    Node_DetachAllEffects(self);
    for (uint32_t i = 0; i < self->children.size; i++) {
        re::Ptr32<NiAVObject> detached;
        Node_DetachChildAt(self, &detached, i);
        Release(detached);
    }
    List* effects = Effects(self);
    effects->vtable = reinterpret_cast<void*>(EffectList_vtable);
    for (Node* node = effects->head; node != nullptr;) {
        Node* next = node->next;
        Virtual<void>(effects, 2, node);
        node = next;
    }
    effects->count = 0;
    effects->head = nullptr;
    effects->tail = nullptr;
    effects->vtable = reinterpret_cast<void*>(PointerListBase_vtable);
    self->children.vtable = reinterpret_cast<void*>(ChildArray_vtable);
    ChildArray_Free(self->children.base);
    AVObject_Destruct(self);
}

NiNode* __fastcall DeletingDestructor(NiNode* self, re::Edx, uint32_t flags) {
    Node_Destructor(self);
    if (flags & 1) {
        NiDelete(self, 0xF4);
    }
    return self;
}

const RTTI* __fastcall GetRTTI(NiNode*, re::Edx) {
    return &NiNode_RTTI;
}

// ---- cloning

// vf3 CreateClone
NiNode* __fastcall CreateClone(NiNode* self, re::Edx, void* cloning) {
    void* p = NiNew(0xF4);
    if (p != nullptr) {
        NiNode* clone = Node_Construct(p, 0);
        Node_CopyMembers(self, clone, cloning);
        return clone;
    }
    Node_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

// CopyMembers: the children's clones set in place
void __fastcall CopyMembers(NiNode* self, re::Edx, NiNode* dest, void* cloning) {
    AVObject_CopyMembers(self, dest, cloning);
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            NiAVObject* clone = Virtual<NiAVObject*>(child, 3, cloning);
            NiAVObject* old;
            Virtual<void>(dest, 37, &old, i, clone);   // SetAt
            Release(old);
        }
    }
}

// vf11 ProcessClone: the clone gets the effects (their clones where there are)
void __fastcall ProcessClone(NiNode* self, re::Edx, void* cloning) {
    AVObject_ProcessClone(self, cloning);
    void* map = At<void*>(cloning, 0);   // the clone map
    re::Ptr32<void>* table = At<re::Ptr32<void>*>(map, 8);
    NiNode* clone = nullptr;
    for (void* item = table[reinterpret_cast<uintptr_t>(self) % At<uint32_t>(map, 4)]; item != nullptr;
         item = At<void*>(item, 0)) {
        if (At<NiNode*>(item, 4) == self) {
            clone = At<NiNode*>(item, 8);
            break;
        }
    }
    if (self->effects.count != 0) {
        Node_CopyEffects(clone, Effects(self), cloning);
    }
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<void>(child, 11, cloning);
        }
    }
}

// (on the clone) the source's effects attached, last to first: their clones, or (unless the
// cloning's effect behaviour, +0xC, is 1) the effects themselves; none when it's 0
void __fastcall CopyEffects(NiNode* self, re::Edx, List* effects, void* cloning) {
    if (At<uint32_t>(cloning, 0xC) == 0) {
        return;
    }
    for (Node* node = effects->tail; node != nullptr;) {
        NiObject* effect = node->element;
        node = node->prev;
        void* map = At<void*>(cloning, 0);
        re::Ptr32<void>* table = At<re::Ptr32<void>*>(map, 8);
        void* item = table[reinterpret_cast<uintptr_t>(effect) % At<uint32_t>(map, 4)];
        for (; item != nullptr; item = At<void*>(item, 0)) {
            if (At<NiObject*>(item, 4) == effect) {
                break;
            }
        }
        NiObject* attach;
        if (item != nullptr) {
            attach = At<NiObject*>(item, 8);
        } else {
            if (At<uint32_t>(cloning, 0xC) == 1) {
                continue;
            }
            attach = effect;
        }
        if (attach != nullptr) {
            Node_AttachEffect(self, attach);
        }
    }
}

// ---- streaming

// vf4: the children and the effects as link IDs
void __fastcall LoadBinary(NiNode* self, re::Edx, void* stream) {
    AVObject_LoadBinary(self, stream);
    self->flags |= 0x100;
    Stream_ReadMultipleLinkIDs(stream);
    Stream_ReadMultipleLinkIDs(stream);
}

// vf5
void __fastcall LinkObject(NiNode* self, re::Edx, void* stream) {
    AVObject_LinkObject(self, stream);
    uint32_t count = Stream_GetNumberOfLinkIDs(stream);
    if (count != 0) {
        ChildArray_SetSize(&self->children, count);
        for (uint32_t i = 0; i < count; i++) {
            NiAVObject* child = static_cast<NiAVObject*>(Stream_ResolveLinkID(stream));
            NiAVObject* old;
            Virtual<void>(self, 37, &old, i, child);   // SetAt
            Release(old);
        }
    }
    for (uint32_t effects = Stream_GetNumberOfLinkIDs(stream); effects != 0;) {
        effects--;
        NiObject* effect = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        if (effect != nullptr) {
            // AttachEffect without the check
            List* list = Effects(self);
            Node* node = Virtual<Node*>(list, 1);
            node->element = effect;
            node->prev = nullptr;
            node->next = list->head;
            if (list->head != nullptr) {
                list->head->prev = node;
            } else {
                list->tail = node;
            }
            list->count++;
            list->head = node;
            Effect_AttachAffectedNode(effect, self);
        }
    }
}

// vf6
uint8_t __fastcall RegisterStreamables(NiNode* self, re::Edx, void* stream) {
    if (!AVObject_RegisterStreamables(self, stream)) {
        return 0;
    }
    for (uint32_t i = 0; i < self->children.size; i++) {
        if (i >= self->children.size) {
            continue;
        }
        NiAVObject* child = self->children.base[i];
        if (child != nullptr) {
            Virtual<uint8_t>(child, 6, stream);
        }
    }
    for (Node* node = Effects(self)->head; node != nullptr;) {
        NiObject* effect = node->element;
        node = node->next;
        if (effect != nullptr) {
            Virtual<uint8_t>(effect, 6, stream);
        }
    }
    return 1;
}

// vf7: the children, then the effects last to first
void __fastcall SaveBinary(NiNode* self, re::Edx, void* stream) {
    AVObject_SaveBinary(self, stream);
    uint32_t count = self->children.size;
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        NiAVObject* child = i < self->children.size ? self->children.base[i] : nullptr;
        Virtual<void>(stream, 12, child);   // SaveLinkID
    }
    int32_t effects = int32_t(self->effects.count);
    SaveBytes(stream, &effects, 4);
    if (effects > 0) {
        for (Node* node = Effects(self)->tail; node != nullptr;) {
            NiObject* effect = node->element;
            node = node->prev;
            Virtual<void>(stream, 12, effect);
        }
    }
}

// vf8
uint8_t __fastcall IsEqual(NiNode* self, re::Edx, NiNode* other) {
    if (!AVObject_IsEqual(self, other)) {
        return 0;
    }
    uint32_t count = self->children.size;
    if (count != other->children.size) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        NiAVObject* a = i < self->children.size ? self->children.base[i] : nullptr;
        NiAVObject* b = i < other->children.size ? other->children.base[i] : nullptr;
        if (a == nullptr) {
            if (b != nullptr) {
                return 0;
            }
        } else if (b == nullptr || !Virtual<uint8_t>(a, 8, b)) {
            return 0;
        }
    }
    uint32_t effects = self->effects.count;
    if (effects != other->effects.count) {
        return 0;
    }
    if (effects != 0) {
        Node* mine = Effects(self)->head;
        Node* theirs = Effects(other)->head;
        while (mine != nullptr) {
            NiObject* a = mine->element;
            mine = mine->next;
            NiObject* b = theirs->element;
            theirs = theirs->next;
            if (a == nullptr) {
                if (b != nullptr) {
                    return 0;
                }
            } else if (b == nullptr || !Virtual<uint8_t>(a, 8, b)) {
                return 0;
            }
        }
    }
    return 1;
}

// vf9
void __fastcall GetViewerStrings(NiNode* self, re::Edx, NiTArray<char*>* strings) {
    AVObject_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiNode_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("m_bVisual", self->worldBound.radius != 0.0f ? 1 : 0));
    if (self->effects.count != 0) {
        int index = 0;
        for (Node* node = Effects(self)->head; node != nullptr;) {
            NiObject* effect = node->element;
            node = node->next;
            char name[64];
            NiSprintf(name, 0x40, "effect[%d]", index);
            AddViewerStringInline(strings, GetViewerStringPointer(name, effect));
            index++;
        }
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NINODE_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NINODE_REVERSED("NiDynamicEffectState::NiDynamicEffectState", 0x0075F000, &ni::EffectStateConstruct);
NINODE_REVERSED("NiDynamicEffectState::DeletingDestructor", 0x0075F040, &ni::EffectStateDeletingDestructor);
NINODE_REVERSED("NiNode::SetMaterialNeedsUpdate", 0x0075F060, &ni::SetMaterialNeedsUpdate);
NINODE_REVERSED("NiNode::SetDefaultMaterialNeedsUpdateFlag", 0x0075F0A0, &ni::SetDefaultMaterialNeedsUpdateFlag);
NINODE_REVERSED("NiNode::ApplyTransform", 0x0075F0E0, &ni::ApplyTransform);
NINODE_REVERSED("NiNode::UpdateUpwardPass", 0x0075F270, &ni::UpdateUpwardPass);
NINODE_REVERSED("NiNode::UpdateWorldBound", 0x0075F290, &ni::UpdateWorldBound);
NINODE_REVERSED("NiNode::UpdatePropertiesDownward", 0x0075F310, &ni::UpdatePropertiesDownward);
NINODE_REVERSED("NiNode::UpdateNodeBound", 0x0075F380, &ni::UpdateNodeBound);
NINODE_REVERSED("NiNode::OnVisible", 0x0075F3E0, &ni::OnVisible);
NINODE_REVERSED("NiNode::PurgeRendererData", 0x0075F430, &ni::PurgeRendererData);
NINODE_REVERSED("NiNode::AttachEffect", 0x0075F4A0, &ni::AttachEffect);
NINODE_REVERSED("NiNode::DetachAllEffects", 0x0075F510, &ni::DetachAllEffects);
NINODE_REVERSED("NiNode::CopyMembers", 0x0075F570, &ni::CopyMembers);
NINODE_REVERSED("NiNode::LoadBinary", 0x0075F600, &ni::LoadBinary);
NINODE_REVERSED("NiTPointerList<NiDynamicEffect*>::Remove", 0x0075F630, &ni::EffectListRemove);
NINODE_REVERSED("NiNode::DetachChildAt", 0x0075F680, &ni::DetachChildAt);
NINODE_REVERSED("NiNode::DetachChild", 0x0075F730, &ni::DetachChild);
NINODE_REVERSED("NiNode::UpdateDownwardPass", 0x0075F800, &ni::UpdateDownwardPass);
NINODE_REVERSED("NiNode::UpdateSelectedDownwardPass", 0x0075F8C0, &ni::UpdateSelectedDownwardPass);
NINODE_REVERSED("NiNode::UpdateRigidDownwardPass", 0x0075F9B0, &ni::UpdateRigidDownwardPass);
NINODE_REVERSED("NiNode::UpdateControllers", 0x0075FA50, &ni::UpdateControllers);
NINODE_REVERSED("NiNode::UpdatePropertiesUpward", 0x0075FAA0, &ni::UpdatePropertiesUpward);
NINODE_REVERSED("NiNode::PushLocalEffects", 0x0075FB50, &ni::PushLocalEffects);
NINODE_REVERSED("NiNode::UpdateEffectsDownward", 0x0075FE60, &ni::UpdateEffectsDownward);
NINODE_REVERSED("NiNode::SetSelectiveUpdateFlags", 0x0075FEE0, &ni::SetSelectiveUpdateFlags);
NINODE_REVERSED("NiNode::DetachEffect", 0x00760030, &ni::DetachEffect);
NINODE_REVERSED("NiNode::GetObjectByName", 0x00760080, &ni::GetObjectByName);
NINODE_REVERSED("NiNode::CopyEffects", 0x007600E0, &ni::CopyEffects);
NINODE_REVERSED("NiNode::RegisterStreamables", 0x00760150, &ni::RegisterStreamables);
NINODE_REVERSED("NiNode::SaveBinary", 0x007601D0, &ni::SaveBinary);
NINODE_REVERSED("NiNode::IsEqual", 0x007602B0, &ni::IsEqual);
NINODE_REVERSED("NiNode::~NiNode", 0x007603B0, &ni::Destruct);
NINODE_REVERSED("NiNode::GetRTTI", 0x00760480, &ni::GetRTTI);
NINODE_REVERSED("NiNode::UpdateEffectsUpward", 0x00760490, &ni::UpdateEffectsUpward);
NINODE_REVERSED("NiNode::ProcessClone", 0x00760510, &ni::ProcessClone);
NINODE_REVERSED("NiNode::LinkObject", 0x00760590, &ni::LinkObject);
NINODE_REVERSED("NiTObjectArray<NiAVObjectPtr>::Add", 0x00760680, &ni::ChildArrayAdd);
NINODE_REVERSED("NiTObjectArray<NiAVObjectPtr>::AddFirstEmpty", 0x007606C0, &ni::ChildArrayAddFirstEmpty);
NINODE_REVERSED("NiNode::NiNode", 0x00760780, &ni::Construct);
NINODE_REVERSED("NiNode::DeletingDestructor", 0x00760820, &ni::DeletingDestructor);
NINODE_REVERSED("NiNode::AttachChild", 0x00760850, &ni::AttachChild);
NINODE_REVERSED("NiNode::SetAt", 0x007608E0, &ni::SetAt);
NINODE_REVERSED("NiNode::CreateClone", 0x00760A10, &ni::CreateClone);
NINODE_REVERSED("NiNode::GetViewerStrings", 0x00760A80, &ni::GetViewerStrings);
