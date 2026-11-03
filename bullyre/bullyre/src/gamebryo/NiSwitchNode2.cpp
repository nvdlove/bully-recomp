// NiSwitchNode's slots the first pass left (NiSwitchNode.cpp has its layout): with "update only
// the active child" set (flags bit 0), the updates, bounds, culling and controllers go to the
// active child alone (its revision brought up to the node's first); otherwise NiNode's. Attaching
// and detaching children keeps the per-child revisions in step (the node's revision reset to 1),
// the active index dropped when its child goes. Also its comparison, saving, cloning and viewer
// strings. NiLODNode shares these slots.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiSwitchNode : NiNode {
    uint16_t flags;                     // +0xF4 (bit 0 update only the active child, bit 1 the
                                        //  controllers' update flag)
    uint16_t padF6;
    int32_t index;                      // +0xF8 (-1: none)
    float savedTime;                    // +0xFC
    uint32_t revision;                  // +0x100
    NiTArray<uint32_t> childRevisions;  // +0x104 (NiTPrimitiveArray<unsigned int>)
};
static_assert(offsetof(NiSwitchNode, index) == 0xF8);
static_assert(offsetof(NiSwitchNode, childRevisions) == 0x104);
static_assert(sizeof(NiSwitchNode) == 0x114);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiSwitchNode_RTTI, 0x020E3C00);

// NiNode's
GAME_FN(void, __thiscall, Node_AttachChild, 0x00760850, void* self, NiAVObject* child, int firstAvailable);
GAME_FN(re::Ptr32<NiAVObject>*, __thiscall, Node_DetachChild, 0x0075F730, void* self, re::Ptr32<NiAVObject>* result, NiAVObject* child);
GAME_FN(re::Ptr32<NiAVObject>*, __thiscall, Node_DetachChildAt, 0x0075F680, void* self, re::Ptr32<NiAVObject>* result, uint32_t index);
GAME_FN(re::Ptr32<NiAVObject>*, __thiscall, Node_SetAt, 0x007608E0, void* self, re::Ptr32<NiAVObject>* result, uint32_t index,
        NiAVObject* child);
GAME_FN(void, __thiscall, Node_CopyMembers, 0x0075F570, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Node_SaveBinary, 0x007601D0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Node_IsEqual, 0x007602B0, void* self, void* other);
GAME_FN(void, __thiscall, Node_GetViewerStrings, 0x00760A80, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Node_UpdateWorldBound, 0x0075F290, void* self);
GAME_FN(void, __thiscall, Node_UpdateDownwardPass, 0x0075F800, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, Node_UpdateSelectedDownwardPass, 0x0075F8C0, void* self, float time);
GAME_FN(void, __thiscall, Node_UpdateRigidDownwardPass, 0x0075F9B0, void* self, float time);
GAME_FN(void, __thiscall, Node_UpdateControllers, 0x0075FA50, void* self, float time);
// NiAVObject's
GAME_FN(void, __thiscall, AVObject_Cull, 0x0075DC50, void* self, void* culler);
GAME_FN(void, __thiscall, AVObject_DoSelectedUpdate, 0x0075EFC0, void* self, float time);
GAME_FN(NiTransform*, __thiscall, Transform_Invert, 0x00770950, const NiTransform* self, NiTransform* result);
GAME_FN(void, __thiscall, Bound_Update, 0x007D2FD0, NiBound* self, const NiBound* bound, const NiTransform* xform);
// the switch node's own (NiSwitchNode.cpp's)
GAME_FN(NiSwitchNode*, __thiscall, SwitchNode_Construct, 0x007D9AC0, void* self);
// NiTPrimitiveArray<unsigned int>
GAME_FN(void, __thiscall, UintArray_SetSize, 0x0084A110, NiTArray<uint32_t>* self, uint32_t size);
GAME_FN(void, __thiscall, UintArray_SetAt, 0x00833C70, NiTArray<uint32_t>* self, uint32_t index, const uint32_t* value);
GAME_FN(char*, __cdecl, GetViewerStringInt, 0x007856A0, const char* prefix, int32_t value);

inline NiAVObject* ChildAt(NiSwitchNode* self, uint32_t index) {
    return self->children.base[index];
}

// the per-child revision slot emptied (NiTPrimitiveArray::RemoveAt, inlined)
inline void RemoveRevisionAt(NiSwitchNode* self, uint32_t index) {
    NiTArray<uint32_t>& revisions = self->childRevisions;
    if (index >= revisions.size) {
        return;
    }
    uint32_t old = revisions.base[index];
    revisions.base[index] = 0;
    if (old != 0) {
        revisions.esize--;
    }
    uint16_t last = uint16_t(revisions.size - 1);
    if (index == last) {
        revisions.size = last;
    }
}

// the active index dropped when its child is gone
inline void CheckIndex(NiSwitchNode* self) {
    int32_t index = self->index;
    if (index > -1 && (index >= int32_t(self->children.size) || self->children.base[index] == nullptr)) {
        self->index = -1;
    }
}

// a child brought up to the node's revision: its downward pass at the saved time
inline void UpdateChild(NiSwitchNode* self, uint32_t index, NiAVObject* child) {
    if (self->childRevisions.base[index] != self->revision) {
        UintArray_SetAt(&self->childRevisions, index, &self->revision);
        Virtual<void>(child, 25, self->savedTime, int(uint8_t(self->flags) >> 1) & 1);
    }
}

} // namespace

// 0x7D9400: DetachChild (slot 35)
re::Ptr32<NiAVObject>* __fastcall NiSwitchNode_DetachChild(NiSwitchNode* self, re::Edx, re::Ptr32<NiAVObject>* result, NiAVObject* child) {
    self->revision = 1;
    for (uint32_t i = 0; i < self->children.size; i++) {
        if (self->children.base[i] == child) {
            RemoveRevisionAt(self, i);
            break;
        }
    }
    Node_DetachChild(self, result, child);
    CheckIndex(self);
    return result;
}

// 0x7D94C0: DetachChildAt (slot 36)
re::Ptr32<NiAVObject>* __fastcall NiSwitchNode_DetachChildAt(NiSwitchNode* self, re::Edx, re::Ptr32<NiAVObject>* result, uint32_t index) {
    self->revision = 1;
    if (index < self->children.size) {
        RemoveRevisionAt(self, index);
    }
    Node_DetachChildAt(self, result, index);
    CheckIndex(self);
    return result;
}

// 0x7D9560: OnVisible (slot 32): the active child only (brought up to date first)
void __fastcall NiSwitchNode_OnVisible(NiSwitchNode* self, re::Edx, void* culler) {
    int32_t index = self->index;
    if (index < 0) {
        return;
    }
    NiAVObject* child = ChildAt(self, uint32_t(index));
    if (child == nullptr) {
        return;
    }
    UpdateChild(self, uint32_t(index), child);
    AVObject_Cull(child, culler);
}

// 0x7D95E0: UpdateWorldBound (slot 31): the active child's (none: radius 0), else NiNode's
void __fastcall NiSwitchNode_UpdateWorldBound(NiSwitchNode* self, re::Edx) {
    if ((self->flags & 1) == 0) {
        Node_UpdateWorldBound(self);
        return;
    }
    int32_t index = self->index;
    if (index >= 0) {
        NiAVObject* child = ChildAt(self, uint32_t(index));
        if (child != nullptr) {
            self->worldBound = child->worldBound;
            return;
        }
    }
    self->worldBound.radius = 0.0f;
}

// 0x7D9630: UpdateNodeBound (slot 19): every child brought up to date and its node bound
// updated, then the node's model bound from its world bound
void __fastcall NiSwitchNode_UpdateNodeBound(NiSwitchNode* self, re::Edx) {
    for (uint32_t i = 0; i < self->children.size; i++) {
        NiAVObject* child = ChildAt(self, i);
        if (child != nullptr) {
            UpdateChild(self, i, child);
            Virtual<void>(child, 19);
        }
    }
    NiTransform inverse;
    Transform_Invert(&self->world, &inverse);
    Bound_Update(&self->bound, &self->worldBound, &inverse);
}

// 0x7D96E0: IsEqual (slot 8): NiNode's, the index and the update-only-active flag
uint8_t __fastcall NiSwitchNode_IsEqual(NiSwitchNode* self, re::Edx, NiSwitchNode* other) {
    if (!Node_IsEqual(self, other)) {
        return 0;
    }
    if (self->index != other->index || ((other->flags ^ self->flags) & 1) != 0) {
        return 0;
    }
    return 1;
}

// 0x7D9720: UpdateDownwardPass (slot 25)
void __fastcall NiSwitchNode_UpdateDownwardPass(NiSwitchNode* self, re::Edx, float time, int updateControllers) {
    if (re::Flag(updateControllers)) {
        self->flags |= 2;
    } else {
        self->flags &= 0xFFFD;
    }
    self->savedTime = time;
    if ((self->flags & 1) == 0) {
        Node_UpdateDownwardPass(self, time, updateControllers);
        return;
    }
    self->revision++;
    if (re::Flag(updateControllers)) {
        AVObject_UpdateObjectControllers(self, time, 1);
    }
    Virtual<void>(self, 30);   // UpdateWorldData
    int32_t index = self->index;
    if (index < 0) {
        return;
    }
    NiAVObject* child = ChildAt(self, uint32_t(index));
    if (child == nullptr) {
        return;
    }
    Virtual<void>(child, 25, time, updateControllers);
    UintArray_SetAt(&self->childRevisions, uint32_t(self->index), &self->revision);
    self->worldBound = child->worldBound;
}

// 0x7D97F0: UpdateSelectedDownwardPass (slot 26)
void __fastcall NiSwitchNode_UpdateSelectedDownwardPass(NiSwitchNode* self, re::Edx, float time) {
    self->savedTime = time;
    if ((self->flags & 1) == 0) {
        Node_UpdateSelectedDownwardPass(self, time);
        return;
    }
    self->revision++;
    AVObject_UpdateObjectControllers(self, time, (uint8_t(self->NiAVObject::flags) >> 3) & 1);
    Virtual<void>(self, 30);
    int32_t index = self->index;
    if (index < 0) {
        return;
    }
    NiAVObject* child = ChildAt(self, uint32_t(index));
    if (child == nullptr) {
        return;
    }
    AVObject_DoSelectedUpdate(child, time);
    UintArray_SetAt(&self->childRevisions, uint32_t(self->index), &self->revision);
    self->worldBound = child->worldBound;
}

// 0x7D98B0: UpdateRigidDownwardPass (slot 27): the active child's rigid pass when it's selectively
// updated
void __fastcall NiSwitchNode_UpdateRigidDownwardPass(NiSwitchNode* self, re::Edx, float time) {
    self->savedTime = time;
    if ((self->flags & 1) == 0) {
        Node_UpdateRigidDownwardPass(self, time);
        return;
    }
    self->revision++;
    AVObject_UpdateObjectControllers(self, time, (uint8_t(self->NiAVObject::flags) >> 3) & 1);
    Virtual<void>(self, 30);
    int32_t index = self->index;
    if (index < 0) {
        return;
    }
    NiAVObject* child = ChildAt(self, uint32_t(index));
    if (child == nullptr) {
        return;
    }
    if (((uint8_t(child->flags) >> 1) & 1) != 0) {
        Virtual<void>(child, 27, time);
    }
    UintArray_SetAt(&self->childRevisions, uint32_t(self->index), &self->revision);
    self->worldBound = child->worldBound;
}

// 0x7D9980: UpdateControllers (slot 18): its own and the active child's
void __fastcall NiSwitchNode_UpdateControllers(NiSwitchNode* self, re::Edx, float time) {
    if ((self->flags & 1) == 0) {
        Node_UpdateControllers(self, time);
        return;
    }
    if (self->index < 0) {
        return;
    }
    AVObject_UpdateObjectControllers(self, time, 1);
    NiAVObject* child = ChildAt(self, uint32_t(self->index));
    if (child != nullptr) {
        Virtual<void>(child, 18, time);
    }
}

// 0x7D9A50: SaveBinary (slot 7): NiNode's, the flags, the index
void __fastcall NiSwitchNode_SaveBinary(NiSwitchNode* self, re::Edx, void* stream) {
    Node_SaveBinary(self, stream);
    SaveBytes(stream, &self->flags, 2);
    SaveBytes(stream, &self->index, 4);
}

// 0x7D9BA0: AttachChild (slot 34): a revision slot added for it (0)
void __fastcall NiSwitchNode_AttachChild(NiSwitchNode* self, re::Edx, NiAVObject* child, int firstAvailable) {
    Node_AttachChild(self, child, firstAvailable);
    self->revision = 1;
    NiTArray<uint32_t>* revisions = &self->childRevisions;
    uint32_t index = revisions->size;
    uint32_t zero = 0;
    if (index >= revisions->maxSize) {
        UintArray_SetSize(revisions, revisions->growBy + index);
    }
    UintArray_SetAt(revisions, index, &zero);
}

// 0x7D9C00: SetAt (slot 37): the slot's revision reset (0)
re::Ptr32<NiAVObject>* __fastcall NiSwitchNode_SetAt(NiSwitchNode* self, re::Edx, re::Ptr32<NiAVObject>* result, uint32_t index,
                                           NiAVObject* child) {
    NiTArray<uint32_t>* revisions = &self->childRevisions;
    self->revision = 1;
    uint32_t zero = 0;
    if (index >= revisions->maxSize) {
        UintArray_SetSize(revisions, revisions->growBy + index);
    }
    UintArray_SetAt(revisions, index, &zero);
    Node_SetAt(self, result, index, child);
    return result;
}

// 0x7D9C60: CreateClone (slot 3): NiNode's members, the flags and the index
NiSwitchNode* __fastcall NiSwitchNode_CreateClone(NiSwitchNode* self, re::Edx, void* cloning) {
    void* memory = NiNew(sizeof(NiSwitchNode));
    NiSwitchNode* clone = memory != nullptr ? SwitchNode_Construct(memory) : nullptr;
    Node_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    clone->index = self->index;
    return clone;
}

// 0x7D9CD0: GetViewerStrings (slot 9)
void __fastcall NiSwitchNode_GetViewerStrings(NiSwitchNode* self, re::Edx, NiTArray<char*>* strings) {
    Node_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiSwitchNode_RTTI.name));
    AddViewerString(strings, GetViewerStringInt("m_iIndex", self->index));
    AddViewerString(strings, GetViewerStringBool("m_bUpdateOnlyActive", self->flags & 1));
}

#define NISWITCH2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NISWITCH2_REVERSED("NiSwitchNode::DetachChild", 0x007D9400, NiSwitchNode_DetachChild);
NISWITCH2_REVERSED("NiSwitchNode::DetachChildAt", 0x007D94C0, NiSwitchNode_DetachChildAt);
NISWITCH2_REVERSED("NiSwitchNode::OnVisible", 0x007D9560, NiSwitchNode_OnVisible);
NISWITCH2_REVERSED("NiSwitchNode::UpdateWorldBound", 0x007D95E0, NiSwitchNode_UpdateWorldBound);
NISWITCH2_REVERSED("NiSwitchNode::UpdateNodeBound", 0x007D9630, NiSwitchNode_UpdateNodeBound);
NISWITCH2_REVERSED("NiSwitchNode::IsEqual", 0x007D96E0, NiSwitchNode_IsEqual);
NISWITCH2_REVERSED("NiSwitchNode::UpdateDownwardPass", 0x007D9720, NiSwitchNode_UpdateDownwardPass);
NISWITCH2_REVERSED("NiSwitchNode::UpdateSelectedDownwardPass", 0x007D97F0, NiSwitchNode_UpdateSelectedDownwardPass);
NISWITCH2_REVERSED("NiSwitchNode::UpdateRigidDownwardPass", 0x007D98B0, NiSwitchNode_UpdateRigidDownwardPass);
NISWITCH2_REVERSED("NiSwitchNode::UpdateControllers", 0x007D9980, NiSwitchNode_UpdateControllers);
NISWITCH2_REVERSED("NiSwitchNode::SaveBinary", 0x007D9A50, NiSwitchNode_SaveBinary);
NISWITCH2_REVERSED("NiSwitchNode::AttachChild", 0x007D9BA0, NiSwitchNode_AttachChild);
NISWITCH2_REVERSED("NiSwitchNode::SetAt", 0x007D9C00, NiSwitchNode_SetAt);
NISWITCH2_REVERSED("NiSwitchNode::CreateClone", 0x007D9C60, NiSwitchNode_CreateClone);
NISWITCH2_REVERSED("NiSwitchNode::GetViewerStrings", 0x007D9CD0, NiSwitchNode_GetViewerStrings);

} // namespace ni
