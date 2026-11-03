// The RenderWare emulation over Gamebryo: RpHAnimHierarchy (0x20: +0 flags (1 a sub-
// hierarchy, 2 no matrices of its own), +4 the node count, +8 the matrices (0x34 each),
// +0xC the nodes (0x14 each: +0 id, +4 index, +8 flags, +0xC frame, +0x10 a name), +0x10
// its frame, +0x14 itself, +0x1C its AM_ pose (0x40 per node)): created, copied, cloned
// with a frame, destroyed, and a frame's bone fields. 0x6C7B80..0x6C809C.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

GAME_VAR(char*, rwh_emptyString, 0x00951028);
GAME_VAR(void*, rwh_stringTable, 0x020E216C);
inline void* const kMatrixConstruct = reinterpret_cast<void*>(0x00413DC0);
inline void* const kNodeConstruct = reinterpret_cast<void*>(0x006C7B80);

GAME_FN(void, __cdecl, Rwh_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, Rwh_PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, Rwh_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Rwh_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, Rwh_FreeThunk, 0x006C80F0, void* p);
GAME_FN(void*, __cdecl, Rwh_NiAlloc, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, Rwh_NiFreeArray, 0x00752570, void* p, uint32_t size);
GAME_FN(void, __stdcall, Rwh_VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __thiscall, Rwh_FixedStringAssign, 0x00401CD0, re::Ptr32<char>* self, re::Ptr32<char>* other);
GAME_FN(void, __thiscall, Rwh_StringTableFree, 0x00754550, void* table, re::Ptr32<char>* string, uint32_t length);
GAME_FN(uint8_t*, __cdecl, Rwh_NewPose, 0x006C8150, uint32_t count, uint32_t stride);
GAME_FN(void*, __thiscall, Rwh_NodesDelete, 0x006C7B90, void* nodes, int flags);
GAME_FN(uint8_t*, __thiscall, Rwh_Construct, 0x006C7C60, void* self, int32_t count, const uint32_t* flags,
        const uint32_t* ids, uint32_t hierarchyFlags, uint32_t stride);
GAME_FN(uint8_t*, __cdecl, Rwh_Create, 0x006C7D80, int32_t count, const uint32_t* flags, const uint32_t* ids,
        uint32_t hierarchyFlags, uint32_t stride);
GAME_FN(int, __cdecl, Rwh_Destroy, 0x006C7FD0, uint8_t* hierarchy);

inline uint32_t ArrayBytes(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return (bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes);
}

inline void ReleaseFixedString(re::Ptr32<char>* s) {
    if (*s == rwh_emptyString) {
        return;
    }
    uint32_t length = *reinterpret_cast<uint16_t*>(*s - 2);
    if (vc::Decrement(*s - 8) == 1) {
        Rwh_StringTableFree(rwh_stringTable, s, length);
    }
}

// 0x6C7B80 (thiscall): a node's constructor (its name empty)
uint8_t* __fastcall Node_Construct(uint8_t* self, re::Edx) {
    At<char*>(self, 0x10) = rwh_emptyString;
    return self;
}

// 0x6C7B90 (thiscall): the nodes' deleting destructor (flags 2: the whole array, its
// count before it; 1: freed)
uint8_t* __fastcall Nodes_Delete(uint8_t* self, re::Edx, int flags) {
    if (flags & 2) {
        uint8_t* block = self - 4;
        int32_t count = At<int32_t>(block, 0);
        for (int32_t i = count - 1; i >= 0; i--) {
            ReleaseFixedString(reinterpret_cast<re::Ptr32<char>*>(self + i * 0x14 + 0x10));
        }
        if (flags & 1) {
            Rwh_Free(block);
        }
        return block;
    }
    ReleaseFixedString(reinterpret_cast<re::Ptr32<char>*>(self + 0x10));
    if (flags & 1) {
        Rwh_Free(self);
    }
    return self;
}

// 0x6C7C60 (thiscall): the constructor: the pose (memory tag 4), the matrices (unless flag
// 2), the nodes with their ids, indices and flags
uint8_t* __fastcall Construct(uint8_t* self, re::Edx, int32_t count, const uint32_t* flags, const uint32_t* ids,
                              uint32_t hierarchyFlags, uint32_t stride) {
    At<uint8_t*>(self, 0x1C) = Rwh_NewPose(uint32_t(count), stride);
    At<uint32_t>(self, 0) = hierarchyFlags;
    At<int32_t>(self, 4) = count;
    At<void*>(self, 0x10) = nullptr;
    Rwh_PushMemoryTag(4);
    void* matrices = nullptr;
    if (!(hierarchyFlags & 2)) {
        uint32_t n = At<uint32_t>(self, 4);
        matrices = Rwh_NiAlloc(ArrayBytes(n, 0x34));
        if (matrices != nullptr) {
            Rwh_VectorConstruct(matrices, 0x34, n, kMatrixConstruct);
        }
    }
    At<void*>(self, 8) = matrices;
    uint32_t bytes = ArrayBytes(uint32_t(count), 0x14);
    uint32_t total = bytes + 4;
    if (total < bytes) {
        total = 0xFFFFFFFFu;
    }
    uint32_t* block = static_cast<uint32_t*>(Rwh_Malloc(total));
    uint8_t* nodes = nullptr;
    if (block != nullptr) {
        block[0] = uint32_t(count);
        nodes = reinterpret_cast<uint8_t*>(block + 1);
        Rwh_VectorConstruct(nodes, 0x14, uint32_t(count), kNodeConstruct);
    }
    At<uint8_t*>(self, 0xC) = nodes;
    Rwh_PopMemoryTag();
    for (int32_t i = 0; i < count; i++) {
        uint8_t* node = At<uint8_t*>(self, 0xC) + i * 0x14;
        At<void*>(node, 0xC) = nullptr;
        if (ids != nullptr) {
            At<uint32_t>(At<uint8_t*>(self, 0xC) + i * 0x14, 0) = ids[i];
        }
        At<int32_t>(At<uint8_t*>(self, 0xC) + i * 0x14, 4) = i;
        if (flags != nullptr) {
            At<uint32_t>(At<uint8_t*>(self, 0xC) + i * 0x14, 8) = flags[i];
        }
    }
    At<uint8_t*>(self, 0x14) = self;
    return self;
}

// 0x6C7D80: RpHAnimHierarchyCreate (memory tag 4)
uint8_t* __cdecl Create(int32_t count, const uint32_t* flags, const uint32_t* ids, uint32_t hierarchyFlags,
                        uint32_t stride) {
    Rwh_PushMemoryTag(4);
    void* p = Rwh_Malloc(0x20);
    uint8_t* hierarchy = p != nullptr ? Rwh_Construct(p, count, flags, ids, hierarchyFlags, stride) : nullptr;
    Rwh_PopMemoryTag();
    return hierarchy;
}

// 0x6C7DD0: a new hierarchy with another's nodes (the names' references moved over)
uint8_t* __cdecl CreateFromHierarchy(uint8_t* source, uint32_t hierarchyFlags, uint32_t stride) {
    int32_t count = At<int32_t>(source, 4);
    Rwh_PushMemoryTag(4);
    void* p = Rwh_Malloc(0x20);
    uint8_t* hierarchy = p != nullptr ? Rwh_Construct(p, count, nullptr, nullptr, hierarchyFlags, stride) : nullptr;
    Rwh_PopMemoryTag();
    for (int32_t i = 0; i < At<int32_t>(hierarchy, 4); i++) {
        uint8_t* from = At<uint8_t*>(source, 0xC) + i * 0x14;
        uint8_t* to = At<uint8_t*>(hierarchy, 0xC) + i * 0x14;
        At<uint32_t>(to, 0) = At<uint32_t>(from, 0);
        At<uint32_t>(At<uint8_t*>(hierarchy, 0xC) + i * 0x14, 4) = At<uint32_t>(At<uint8_t*>(source, 0xC) + i * 0x14, 4);
        At<uint32_t>(At<uint8_t*>(hierarchy, 0xC) + i * 0x14, 8) = At<uint32_t>(At<uint8_t*>(source, 0xC) + i * 0x14, 8);
        At<void*>(At<uint8_t*>(hierarchy, 0xC) + i * 0x14, 0xC) = nullptr;
        char* name = At<char*>(At<uint8_t*>(source, 0xC) + i * 0x14, 0x10);
        re::Ptr32<char>* target = reinterpret_cast<re::Ptr32<char>*>(At<uint8_t*>(hierarchy, 0xC) + i * 0x14 + 0x10);
        if (*target == name) {
            continue;
        }
        if (name != rwh_emptyString) {
            vc::Increment(name - 8);
        }
        ReleaseFixedString(target);
        *target = name;
    }
    return hierarchy;
}

// 0x6C7EF0 (the source in ebx: not hookable): a frame's copy of a hierarchy (null for a
// sub-hierarchy)
uint8_t* CloneHierarchy(uint8_t* source, void* frame) {
    if (source == nullptr || (At<uint32_t>(source, 0) & 1)) {
        return nullptr;
    }
    uint8_t* hierarchy = Rwh_Create(At<int32_t>(source, 4), nullptr, nullptr, At<uint32_t>(source, 0),
                                    At<uint32_t>(At<uint8_t*>(source, 0x1C), 0x20));
    for (int32_t i = 0; i < At<int32_t>(hierarchy, 4); i++) {
        for (int k = 0; k < 3; k++) {
            At<uint32_t>(At<uint8_t*>(hierarchy, 0xC) + i * 0x14, k * 4) =
                At<uint32_t>(At<uint8_t*>(source, 0xC) + i * 0x14, k * 4);
        }
        At<void*>(At<uint8_t*>(hierarchy, 0xC) + i * 0x14, 0xC) = nullptr;
        Rwh_FixedStringAssign(reinterpret_cast<re::Ptr32<char>*>(At<uint8_t*>(hierarchy, 0xC) + i * 0x14 + 0x10),
                              reinterpret_cast<re::Ptr32<char>*>(At<uint8_t*>(source, 0xC) + i * 0x14 + 0x10));
    }
    At<void*>(hierarchy, 0x10) = frame;
    return hierarchy;
}

// 0x6C7FA0: a frame's bone fields copied from another's (its hierarchy cloned for it)
uint8_t* __cdecl Frame_CopyBone(uint8_t* frame, uint8_t* source) {
    At<uint32_t>(frame, 0x90) = At<uint32_t>(source, 0x90);
    At<uint8_t*>(frame, 0x94) = CloneHierarchy(At<uint8_t*>(source, 0x94), frame);
    return frame;
}

// 0x6C7FD0: RpHAnimHierarchyDestroy (a sub-hierarchy keeps its arrays; returns 0)
int __cdecl Destroy(uint8_t* hierarchy) {
    uint32_t flags = At<uint32_t>(hierarchy, 0);
    if (!(flags & 1)) {
        if (!(flags & 2)) {
            Rwh_NiFreeArray(At<void*>(hierarchy, 8), 0x34);
        }
        if (At<void*>(hierarchy, 0xC) != nullptr) {
            Rwh_NodesDelete(At<void*>(hierarchy, 0xC), 3);
        }
    }
    void* pose = At<void*>(hierarchy, 0x1C);
    At<void*>(hierarchy, 8) = nullptr;
    At<void*>(hierarchy, 0xC) = nullptr;
    Rwh_FreeThunk(pose);
    if (At<uint8_t*>(hierarchy, 0x10) != nullptr) {
        At<void*>(At<uint8_t*>(hierarchy, 0x10), 0x94) = nullptr;
    }
    Rwh_Free(hierarchy);
    return 0;
}

// 0x6C8040: a frame's hierarchy let go of (its nodes' frames cleared; destroyed when it's
// the frame's own) and the bone id reset
uint8_t* __cdecl Frame_ClearHierarchy(uint8_t* frame) {
    uint8_t* hierarchy = At<uint8_t*>(frame, 0x94);
    if (hierarchy != nullptr) {
        for (int32_t i = 0; i < At<int32_t>(hierarchy, 4); i++) {
            At<void*>(At<uint8_t*>(hierarchy, 0xC) + i * 0x14, 0xC) = nullptr;
        }
        if (At<uint8_t*>(hierarchy, 0x10) == frame) {
            Rwh_Destroy(hierarchy);
        }
        At<void*>(frame, 0x94) = nullptr;
    }
    At<int32_t>(frame, 0x90) = -1;
    return frame;
}

} // namespace

RE_UNHOOKED("RpHAnimHierarchy::CloneForFrame", 0x006C7EF0);

#define RWHANIM_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

RWHANIM_REVERSED("RpHAnimNode::Construct", 0x006C7B80, &Node_Construct);
RWHANIM_REVERSED("RpHAnimNode::DeleteArray", 0x006C7B90, &Nodes_Delete);
RWHANIM_REVERSED("RpHAnimHierarchy::Construct", 0x006C7C60, &Construct);
RWHANIM_REVERSED("RpHAnimHierarchy::Create", 0x006C7D80, &Create);
RWHANIM_REVERSED("RpHAnimHierarchy::CreateFromHierarchy", 0x006C7DD0, &CreateFromHierarchy);
RWHANIM_REVERSED("RwFrame::CopyBone", 0x006C7FA0, &Frame_CopyBone);
RWHANIM_REVERSED("RpHAnimHierarchy::Destroy", 0x006C7FD0, &Destroy);
RWHANIM_REVERSED("RwFrame::ClearHierarchy", 0x006C8040, &Frame_ClearHierarchy);
