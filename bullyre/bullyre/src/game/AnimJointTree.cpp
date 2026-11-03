// The AM_ animation system's joint trees: a hierarchy's skeleton (+0x14: +4 the joint
// count, +8 the joints' transforms, 0x34 bytes each with the translation at +0x24, +0xC
// the joints, 0x14 bytes each: +0 the parent, +8 the push/pop flags of the depth-first
// order), the dirty-joint bits at +0x8C, the visitors walking a subtree
// (AdjustJointVisitor 0x941F88, IKAdjustMidBones 0x941F54), and the RV lists'
// splicing. 0x6B17F0..0x6B2CFB.

#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace {

using re::At;

struct Joint {
    int32_t parent;     // +0
    int32_t unknown04;  // +4
    uint32_t flags;     // +8 &3: 1 the last child (pop), 2 has children (push)
    int32_t unknown0C;
    int32_t unknown10;
};
static_assert(sizeof(Joint) == 0x14);

inline Joint* Joints(uint8_t* hierarchy) {
    return At<Joint*>(At<uint8_t*>(hierarchy, 0x14), 0xC);
}
inline uint8_t* Transforms(uint8_t* hierarchy) {
    return At<uint8_t*>(At<uint8_t*>(hierarchy, 0x14), 8);
}

GAME_FN(int32_t, __cdecl, AnimSlot_TypeId, 0x006075F0);  // (a short in ax; eax is passed on whole)
GAME_FN(void, __thiscall, List_InsertRange, 0x006B2C20, uint8_t* list, uint8_t* before, uint8_t* first,
        uint8_t* last);

inline void SetDirty(uint8_t* hierarchy, int joint) {
    uint32_t* bits = &At<uint32_t>(hierarchy, 0x8C);
    if (joint >= 32) {
        bits += uint32_t(joint) >> 5;
        joint -= (uint32_t(joint) >> 5) << 5;
    }
    *bits |= 1u << joint;
}

// 0x6B17F0: marks a joint's transform changed
void __fastcall Hierarchy_SetDirty(uint8_t* self, re::Edx, int joint) {
    SetDirty(self, joint);
}

// 0x6B1FA0: one of the seven slots at +0x18 (null past them)
void* __fastcall Slots_Get(uint8_t* self, re::Edx, uint32_t slot) {
    if (slot > 6) {
        return nullptr;
    }
    return At<void*>(self, 0x18 + slot * 4);
}

// 0x6B1F50: the slot, unless it's of the anim-slot type (vtable slot 0 answers IsType)
void* __fastcall Slots_GetIfNotAnimSlot(uint8_t* self, re::Edx, uint32_t slot) {
    if (slot >= 7) {
        return nullptr;
    }
    void* object = At<void*>(self, 0x18 + slot * 4);
    if (object) {
        void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[0];
        int32_t type = AnimSlot_TypeId();
        if (!reinterpret_cast<uint8_t(__thiscall*)(void*, int)>(fn)(object, type)) {
            return At<void*>(self, 0x18 + slot * 4);
        }
    }
    return nullptr;
}

// 0x6B2050: AdjustJointVisitor::Visit (+8 the offset, +0xC the transforms): moves a joint
uint8_t __fastcall AdjustJoint_Visit(uint8_t* self, re::Edx, int joint) {
    const float* offset = At<float*>(self, 8);
    float* pos = reinterpret_cast<float*>(At<uint8_t*>(self, 0xC) + joint * 0x34 + 0x24);
    pos[0] = offset[0] + pos[0];
    pos[1] = offset[1] + pos[1];
    pos[2] = offset[2] + pos[2];
    return 0;  // (go on; the game leaves the translation's address in eax's upper bytes)
}

// 0x6B2080: IKAdjustMidBones::Visit (+4 the hierarchy, +8 the parent joint, +0xC the one
// to leave): the parent's other children take its transform
uint8_t __fastcall IKAdjustMidBones_Visit(uint8_t* self, re::Edx, int joint) {
    uint8_t* hierarchy = At<uint8_t*>(self, 4);
    int parent = At<int32_t>(self, 8);
    if (Joints(hierarchy)[joint].parent != parent || joint == At<int32_t>(self, 0xC)) {
        return 0;
    }
    uint8_t* transforms = Transforms(hierarchy);
    std::memcpy(transforms + joint * 0x34, transforms + parent * 0x34, 0x34);
    SetDirty(hierarchy, joint);
    return 1;
}

// 0x6B2130: the joints' parents from the push/pop order
void __fastcall Hierarchy_ComputeParents(uint8_t* self, re::Edx) {
    int32_t stack[64];
    stack[0] = 0;
    int32_t* top = stack;
    int32_t parent = -1;
    Joint* joint = Joints(self);
    for (int i = 0; i < At<int32_t>(At<uint8_t*>(self, 0x14), 4); i++, joint++) {
        joint->parent = parent;
        switch (joint->flags & 3) {
        case 0:
            parent = i;
            break;
        case 1:  // the last child: back up
            parent = *top--;
            break;
        case 2:  // children follow
            *++top = parent;
            parent = i;
            break;
        default:
            break;
        }
    }
}

// 0x6B21A0: visits the joints below `joint` in order (itself too when asked), until the
// visitor answers true or the subtree ends
void __fastcall Hierarchy_VisitSubtree(uint8_t* self, re::Edx, int joint, re::Ptr32<void>* visitor, int visitFirst) {
    using Visit = uint8_t(__thiscall*)(re::Ptr32<void>*, int);
    if (re::Flag(visitFirst)) {
        re::PtrCast<Visit>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(visitor))[0])(visitor, joint);
    }
    int depth = 1;
    bool first = true;
    const uint32_t* flags = &Joints(self)[joint].flags;
    for (;;) {
        uint32_t kind = *flags & 3;
        if (kind == 1) {
            if (--depth == 0) {
                return;
            }
        } else if (kind == 2 && !first) {
            depth++;
        }
        joint++;
        flags += 5;
        first = false;
        if (re::PtrCast<Visit>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(visitor))[0])(visitor, joint)) {
            return;
        }
    }
}

// 0x6B2C20: RV list (+4 head, +8 tail, +0x18 count; nodes +4 prev, +8 next): links the
// chain first..last in before `before` (null: at the end). The count isn't changed
void __fastcall List_InsertRangeImpl(uint8_t* self, re::Edx, uint8_t* before, uint8_t* first, uint8_t* last) {
    if (At<int32_t>(self, 0x18) == 0) {
        At<uint8_t*>(self, 4) = first;
        At<uint8_t*>(self, 8) = last;
        At<uint8_t*>(first, 4) = nullptr;
        At<uint8_t*>(last, 8) = nullptr;
        return;
    }
    if (before == nullptr) {
        uint8_t* tail = At<uint8_t*>(self, 8);
        At<uint8_t*>(last, 8) = nullptr;
        At<uint8_t*>(first, 4) = tail;
        if (tail == At<uint8_t*>(self, 8)) {
            At<uint8_t*>(self, 8) = last;
        }
        At<uint8_t*>(tail, 8) = first;
        return;
    }
    At<uint8_t*>(last, 8) = before;
    At<uint8_t*>(first, 4) = At<uint8_t*>(before, 4);
    if (before == At<uint8_t*>(self, 4)) {
        At<uint8_t*>(self, 4) = first;
        At<uint8_t*>(before, 4) = last;
        return;
    }
    At<uint8_t*>(At<uint8_t*>(before, 4), 8) = first;
    At<uint8_t*>(before, 4) = last;
}

// 0x6B2CA0: moves a node (*node; null: the source's last) from `source` in before *before
void __fastcall List_MoveFrom(uint8_t* self, re::Edx, re::Ptr32<uint8_t>* before, uint8_t* source, re::Ptr32<uint8_t>* node) {
    uint8_t* n = *node;
    if (n == nullptr) {
        n = At<uint8_t*>(source, 8);
    }
    if (n == At<uint8_t*>(source, 4)) {
        At<uint8_t*>(source, 4) = At<uint8_t*>(n, 8);
    } else {
        At<uint8_t*>(At<uint8_t*>(n, 4), 8) = At<uint8_t*>(n, 8);
    }
    if (n == At<uint8_t*>(source, 8)) {
        At<uint8_t*>(source, 8) = At<uint8_t*>(n, 4);
    } else {
        At<uint8_t*>(At<uint8_t*>(n, 8), 4) = At<uint8_t*>(n, 4);
    }
    At<int32_t>(source, 0x18) -= 1;
    List_InsertRange(self, *before, n, n);
    At<int32_t>(self, 0x18) += 1;
}

} // namespace

#define AMJOINTS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMJOINTS_REVERSED("AM_Hierarchy::SetDirty", 0x006B17F0, &Hierarchy_SetDirty);
AMJOINTS_REVERSED("AM_Slots::GetIfNotAnimSlot", 0x006B1F50, &Slots_GetIfNotAnimSlot);
AMJOINTS_REVERSED("AM_Slots::Get", 0x006B1FA0, &Slots_Get);
AMJOINTS_REVERSED("AdjustJointVisitor::Visit", 0x006B2050, &AdjustJoint_Visit);
AMJOINTS_REVERSED("IKAdjustMidBones::Visit", 0x006B2080, &IKAdjustMidBones_Visit);
AMJOINTS_REVERSED("AM_Hierarchy::ComputeParents", 0x006B2130, &Hierarchy_ComputeParents);
AMJOINTS_REVERSED("AM_Hierarchy::VisitSubtree", 0x006B21A0, &Hierarchy_VisitSubtree);
AMJOINTS_REVERSED("RV::List::InsertRange", 0x006B2C20, &List_InsertRangeImpl);
AMJOINTS_REVERSED("RV::List::MoveFrom", 0x006B2CA0, &List_MoveFrom);
