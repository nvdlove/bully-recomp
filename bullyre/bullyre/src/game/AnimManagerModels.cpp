// The AM_ manager (0x20C4910): its constructor and shutdown, models (anim objects) added
// and removed (the model list at +0x118, each model's attachment list at +0xAC and blend
// node list at +0xD8 sharing the manager's pools), attachments made (a node from the pool
// at +0xB8 tying a bone to a pose, a model or a transform) and undone, a hierarchy object
// given its HXD, clump or NIF, and the .MGR / .MXD lists of hierarchy files loaded.
// 0x6C2070..0x6C3716.

#include <cstdint>
#include <cstring>

#include "game/FileLoader.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct LinkPool {
    re::Ptr32<void> vtable;          // +0x0
    re::Ptr32<uint8_t> head;         // +0x4
    re::Ptr32<uint8_t> tail;         // +0x8
    re::Ptr32<uint8_t> links;        // +0xC
    re::Ptr32<uint8_t> freeLinks;    // +0x10
    int32_t count;         // +0x14
    int32_t used;          // +0x18
    int32_t owned;         // +0x1C
    int32_t borrowed;      // +0x20 (the links aren't its to free)
    re::Ptr32<LinkPool> owner;       // +0x24
    int32_t users;         // +0x28
};
static_assert(sizeof(LinkPool) == 0x2C);

GAME_VAR(float, ammod_msPerStep, 0x00BB6454);
GAME_VAR(float, ammod_timeStep, 0x00C1A9A4);
GAME_VAR(char*, ammod_emptyString, 0x00951028);
GAME_VAR(void*, ammod_stringTable, 0x020E216C);
inline void* const kManager = reinterpret_cast<void*>(0x020C4910);
inline void* const kSetSkeletonCallback = reinterpret_cast<void*>(0x005355B0);
inline void* const kCountChildren = reinterpret_cast<void*>(0x005EE130);

GAME_FN(void*, __cdecl, AmMod_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, AmMod_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, AmMod_FreeThunk, 0x006C80F0, void* p);
GAME_FN(void, __cdecl, AmMod_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, AmMod_PopMemoryTag, 0x005EF100);
GAME_FN(void, __thiscall, AmMod_LinkPoolRelease, 0x005EF710, LinkPool* self);
GAME_FN(void*, __cdecl, AmMod_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(uint8_t, __cdecl, AmMod_Seek, 0x0042D300, void* file, int32_t offset, uint32_t origin);
GAME_FN(uint32_t, __cdecl, AmMod_Tell, 0x0042D430, void* file);
GAME_FN(int, __cdecl, AmMod_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(void, __cdecl, AmMod_Close, 0x0042D450, void* file);
GAME_FN(uint32_t, __cdecl, AmMod_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, void* file);
GAME_FN(FileStream*, __cdecl, AmMod_OpenStream, 0x0042D4C0, int kind, int mode, void* arg);
GAME_FN(int, __cdecl, AmMod_CloseStream, 0x0042D570, FileStream* stream, FileStreamBuffer* out);
inline void* __cdecl AmMod_Memcpy(void* dst, const void* src, uint32_t n) { return crt::Memcpy(dst, src, (size_t)n); }
GAME_FN(char*, __cdecl, AmMod_FixedStringCreate, 0x00754760, const char* s);
GAME_FN(uint8_t, __thiscall, AmMod_FixedStringContains, 0x00406510, const re::Ptr32<char>* self, const char* s);
GAME_FN(void, __thiscall, AmMod_StringTableFree, 0x00754550, void* table, re::Ptr32<char>* string, uint32_t length);
GAME_FN(void*, __cdecl, AmMod_CreateEmptyClump, 0x0040B1D0, int kind);
GAME_FN(void, __thiscall, AmMod_ListInsertRange, 0x006B2C20, void* list, uint8_t* before, uint8_t* first,
        uint8_t* last);
GAME_FN(void, __thiscall, AmMod_ListMoveFrom, 0x006B2CA0, void* self, re::Ptr32<uint8_t>* before, void* source,
        re::Ptr32<uint8_t>* link);
GAME_FN(uint32_t, __thiscall, AmMod_HandleEquals, 0x006BD240, re::Ptr32<uint8_t>* self, re::Ptr32<uint8_t>* other);
GAME_FN(void, __thiscall, AmMod_ListPushBack, 0x006BF670, void* list, void* value);
GAME_FN(void, __thiscall, AmMod_HierReset, 0x006B1E30, void* hierarchy);
GAME_FN(void, __thiscall, AmMod_HierDestruct, 0x006B1EA0, void* hierarchy);
GAME_FN(void, __thiscall, AmMod_HxdDestruct, 0x006B1C00, void* hxd);
GAME_FN(void, __cdecl, AmMod_Fn6B98D0, 0x006B98D0);
GAME_FN(void, __thiscall, AmMod_SetDirty, 0x006B17F0, uint8_t* model, int joint);
GAME_FN(void, __thiscall, AmMod_ComputeParents, 0x006B2130, uint8_t* model);
GAME_FN(void, __thiscall, AmMod_AddAttachment, 0x006B5C60, uint8_t* model, uint8_t* node, float dt, int a);
GAME_FN(void*, __cdecl, AmMod_SkeletonOf, 0x006B1AD0, void* clump);
GAME_FN(void, __thiscall, AmMod_EmptyList, 0x006B5AB0, uint8_t* model, uint32_t list);
GAME_FN(void, __thiscall, AmMod_BlendOut, 0x006BD480, void* manager, uint8_t* model, uint8_t* track, float time);
GAME_FN(uint8_t*, __cdecl, AmMod_NewTrack, 0x006B8FB0);
GAME_FN(void*, __thiscall, AmMod_GetBone, 0x00464EA0, void* hierarchy, uint32_t bone);
GAME_FN(uint8_t*, __thiscall, AmMod_FirstBlendNode, 0x0060AD60, uint8_t* model);
GAME_FN(void*, __thiscall, AmMod_TakeHierarchy, 0x006BF6C0, void* manager);
GAME_FN(void, __thiscall, AmMod_ReleaseHierarchyLink, 0x006BF700, void* manager, uint8_t* link);
GAME_FN(void, __thiscall, AmMod_ReleaseHierarchy, 0x006BF740, void* manager, void* hierarchy);
GAME_FN(int, __thiscall, AmMod_LoadHierarchyFile, 0x006B5870, void* hierarchy, const char* name, void* stream,
        char* hxdName);
GAME_FN(int, __thiscall, AmMod_MakePath, 0x006BD0D0, void* manager, const char* name, char* out, int kind);
GAME_FN(void, __thiscall, AmMod_StartTrack, 0x006C0D40, void* manager, uint8_t* model, uintptr_t anim, uint8_t* track,
        int a, float c, float d);
GAME_FN(int, __thiscall, AmMod_RemoveBlendNode, 0x006C10E0, void* manager, uint8_t* model, uint8_t* node);
GAME_FN(void, __thiscall, AmMod_ReleaseHxd, 0x006C1230, void* manager, uint8_t* hxd);
GAME_FN(int, __thiscall, AmMod_LoadHxd, 0x006C1950, void* manager, const char* name, uint32_t hash, int lock,
        re::Ptr32<uint8_t>* out);
GAME_FN(int, __thiscall, AmMod_LoadHxdFile, 0x006C1C10, void* manager, uint32_t hash, const char* name,
        re::Ptr32<uint8_t>* out, int flags);
GAME_FN(int, __thiscall, AmMod_FindHierarchy, 0x006C17A0, void* manager, uint32_t hash, const char* name,
        re::Ptr32<void>* out);
GAME_FN(void, __thiscall, AmMod_Update, 0x006C1910, void* manager, uint8_t* model, float dt, int blend,
        const void* offset);
GAME_FN(int, __thiscall, AmMod_DetachNode, 0x006C2200, void* manager, uint8_t* model, uint8_t* node);
GAME_FN(int, __thiscall, AmMod_RemoveModel, 0x006C2C00, void* manager, uint8_t* model, int keepClump);
GAME_FN(int, __thiscall, AmMod_LoadHierarchyWithHxd, 0x006C2AE0, void* manager, const char* name, re::Ptr32<void>* out,
        FileStream* stream);
GAME_FN(void, __cdecl, AmMod_DestroyClump, 0x006C75C0, void* clump);
GAME_FN(void*, __cdecl, AmMod_CloneClump, 0x006C7630, void* clump);
GAME_FN(void*, __cdecl, AmMod_GetFrame, 0x006C7370, void* object);
GAME_FN(void, __cdecl, AmMod_ForAllObjects, 0x006C7380, void* clump, void* callback, void* data);
GAME_FN(int, __cdecl, AmMod_AtomicCount, 0x006C73E0, void* clump);
GAME_FN(void*, __cdecl, AmMod_FrameRoot, 0x006C9BC0, void* frame);
GAME_FN(void*, __cdecl, AmMod_ForAllChildren, 0x006C9B50, void* frame, void* callback, void* data);
GAME_FN(void*, __cdecl, AmMod_FirstAtomic, 0x005EDFF0, void* clump);
GAME_FN(void*, __cdecl, AmMod_NewFrame, 0x006CA210);
GAME_FN(uint32_t*, __cdecl, AmMod_FrameTransform, 0x006C9A90, void* frame);
GAME_FN(void, __cdecl, AmMod_AddChild, 0x006C9D80, void* parent, void* child);
GAME_FN(void*, __cdecl, AmMod_HierarchyOf, 0x005EE1A0, void* clump);
GAME_FN(void, __cdecl, AmMod_SetBoneTable, 0x006C76D0, void* frame, void* table);
GAME_FN(void, __cdecl, AmMod_SetClumpFrame, 0x006C7410, void* clump, void* frame);
GAME_FN(void, __cdecl, AmMod_SetAtomicFrame, 0x006C6A10, void* atomic, void* frame);
GAME_FN(void, __cdecl, AmMod_UpdateFrame, 0x006C9AF0, void* frame);
GAME_FN(void, __cdecl, AmMod_DestroyFrame, 0x006CA490, void* frame);

// a list sharing another's links given them back (0x5EF710's work, inlined)
inline void ReleaseShared(LinkPool* list) {
    if (list->tail) {
        At<uint8_t*>(list->tail, 8) = list->owner->freeLinks;
        list->owner->freeLinks = list->head;
    }
    LinkPool* owner = list->owner;
    list->used = 0;
    list->head = nullptr;
    list->tail = nullptr;
    owner->users -= 1;
    list->count = 0;
    list->links = nullptr;
    list->freeLinks = nullptr;
    list->owner = nullptr;
    list->used = 0;
}

// a list's own links freed (or it given back through 0x5EF710 when shared)
inline void ReleaseOwn(LinkPool* list) {
    if (list->owned != 0) {
        if (list->borrowed == 0) {
            AmMod_Free(list->links);
        }
        list->count = 0;
        list->links = nullptr;
        list->freeLinks = nullptr;
        list->owner = nullptr;
        list->used = 0;
    } else if (list->owner != nullptr) {
        AmMod_LinkPoolRelease(list);
    }
}

inline void ShareLinks(LinkPool* list, LinkPool* owner) {
    owner->users += 1;
    list->owner = owner;
    list->count = owner->count;
    list->links = nullptr;
    list->freeLinks = nullptr;
    list->owned = 0;
}

// a used hierarchy object done with: its clump, HXD and itself released (0x6C1D00, inlined)
inline void ReleaseHierarchyObject(uint8_t* self, uint8_t* hierarchy) {
    if (At<uint32_t>(hierarchy, 0) != 0) {
        At<uint32_t>(hierarchy, 0) -= 1;
    }
    if (!(At<uint32_t>(hierarchy, 4) & 8) && At<uint32_t>(hierarchy, 0) == 0 && !(At<uint32_t>(hierarchy, 4) & 2)) {
        if (At<void*>(hierarchy, 8) != nullptr) {
            AmMod_DestroyClump(At<void*>(hierarchy, 8));
            At<uint32_t>(hierarchy, 4) &= ~1u;
            At<void*>(hierarchy, 8) = nullptr;
        }
        if (At<uint8_t*>(hierarchy, 0x10) != nullptr) {
            AmMod_ReleaseHxd(self, At<uint8_t*>(hierarchy, 0x10));
        }
        if (!(At<uint8_t>(hierarchy, 4) & 4)) {
            AmMod_ReleaseHierarchy(self, hierarchy);
        }
    }
}

// the blend nodes of a model aimed at `target`'s model (+0x24) taken off
inline void RemoveNodesFor(uint8_t* self, uint8_t* model, void* target) {
    uint8_t* node = AmMod_FirstBlendNode(model);
    while (node) {
        if (At<void*>(node, 0x24) == target) {
            AmMod_RemoveBlendNode(self, model, node);
        }
        uint8_t* link = At<uint8_t*>(node, 0x34);
        if (link == nullptr) {
            break;
        }
        re::Ptr32<uint8_t>* next = At<re::Ptr32<uint8_t>*>(link, 8);
        if (next == nullptr) {
            break;
        }
        node = *next;
    }
}

// an attachment node taken off the model: its model freed (0x2 and +0x9C cleared)
inline int DetachLinked(uint8_t* self, uint8_t* model, uint8_t* node) {
    if (At<void*>(node, 0x14) != nullptr) {
        RemoveNodesFor(self, model, At<void*>(node, 0x14));
    }
    re::Ptr32<uint8_t> link = At<uint8_t*>(node, 0x10);
    re::Ptr32<uint8_t> none = nullptr;
    if (AmMod_HandleEquals(&link, &none) != 0) {
        return 0x1D;
    }
    if (At<uint8_t*>(node, 0x14) != nullptr) {
        At<uint32_t>(At<uint8_t*>(node, 0x14), 0x98) &= ~2u;
        At<void*>(At<uint8_t*>(node, 0x14), 0x9C) = nullptr;
    }
    re::Ptr32<uint8_t> before = nullptr;
    AmMod_ListMoveFrom(self + 0xB8, &before, model + 0xAC, &link);
    return 0;
}

// 0x6C2580 (thiscall): the constructor: the seven lists empty with their vtables
uint8_t* __fastcall Construct(uint8_t* self, re::Edx) {
    static const struct {
        int offset;
        uintptr_t vtable;
    } lists[] = {{0, 0x0094235C}, {0x2C, 0x0094235C}, {0x5C, 0x00942364}, {0x88, 0x00942364},
                 {0xB8, 0x00941FDC}, {0xE8, 0x00941FE4}, {0x118, 0x00942354}};
    for (const auto& l : lists) {
        for (int i = 4; i < 0x2C; i += 4) {
            At<uint32_t>(self, l.offset + i) = 0;
        }
        At<uint32_t>(self, l.offset) = l.vtable;
    }
    At<uint32_t>(self, 0x58) = 0;
    At<uint32_t>(self, 0xB4) = 0;
    At<uint32_t>(self, 0x158) = 0;
    At<uint32_t>(self, 0x1FC) = 0;
    return self;
}

// 0x6C2070 (thiscall): an attachment on a model's bone: a node from the pool (0x10: none
// left) tied to `pose` (its +0x14 on), else another model (made its child: flag 2, +0x9C;
// its object's +0xC), else `transform`; with `data` at +8. An updated model gets it applied
// now (and the child model updated once).
int __fastcall AttachNode(uint8_t* self, re::Edx, uint8_t* model, uint32_t bone, uint8_t* pose, re::Ptr32<uint8_t>* out,
                          void* data, uint8_t* child, uint8_t* transform) {
    if (model == nullptr || out == nullptr || (pose == nullptr && child == nullptr && transform == nullptr) ||
        At<void*>(model, 0x94) == nullptr) {
        return 0x16;
    }
    re::Ptr32<uint8_t> link = At<uint8_t*>(self, 0xBC);
    if (link == nullptr) {
        return 0x10;
    }
    uint32_t* node = At<uint32_t*>(link, 0);
    for (int i = 0; i < 6; i++) {
        node[i] = 0;
    }
    At<uint8_t*>(At<uint8_t*>(link, 0), 0x10) = link;
    re::Ptr32<uint8_t> before = nullptr;
    AmMod_ListMoveFrom(model + 0xAC, &before, self + 0xB8, &link);
    uint8_t* n = At<uint8_t*>(link, 0);
    if (pose != nullptr) {
        At<uint8_t*>(n, 4) = pose;
        At<uint8_t*>(n, 0) = pose + 0x14;
    } else if (child != nullptr) {
        At<uint8_t*>(n, 0x14) = child;
        At<void*>(At<uint8_t*>(link, 0), 4) = At<void*>(child, 0xC);
        n = At<uint8_t*>(link, 0);
        At<uint8_t*>(n, 0) = At<uint8_t*>(n, 4) + 0x14;
        At<uint32_t>(child, 0x98) |= 2;
        At<uint8_t*>(child, 0x9C) = model;
    } else {
        At<uint8_t*>(n, 0) = transform;
    }
    void* joint = AmMod_GetBone(At<void*>(model, 0x94), bone);
    int jointIndex = At<int32_t>(joint, 8);
    At<void*>(At<uint8_t*>(link, 0), 0xC) = joint;
    At<void*>(At<uint8_t*>(link, 0), 8) = data;
    *out = At<uint8_t*>(link, 0);
    AmMod_SetDirty(model, jointIndex);
    if (At<uint32_t>(model, 0x98) & 0x400) {
        float dt = ammod_msPerStep * ammod_timeStep;
        AmMod_AddAttachment(model, *out, dt, 1);
        uint8_t* other = At<uint8_t*>(*out, 0x14);
        if (other != nullptr && At<void*>(other, 0x14) != nullptr && !(At<uint32_t>(other, 0x98) & 0x400)) {
            At<uint32_t>(other, 0x98) &= ~2u;
            AmMod_Update(kManager, other, 0.0f, 1, nullptr);
            At<uint32_t>(other, 0x98) |= 2;
        }
    }
    return 0;
}

// 0x6C2200 (thiscall): an attachment taken off (the blend nodes aimed at its model first)
int __fastcall DetachNode(uint8_t* self, re::Edx, uint8_t* model, uint8_t* node) {
    if (model == nullptr || node == nullptr) {
        return 0x16;
    }
    if (At<void*>(model, 0x94) == nullptr) {
        return 0xB;
    }
    if (At<uint32_t>(model, 0xC4) == 0) {
        return 0x1D;
    }
    return DetachLinked(self, model, node);
}

// 0x6C2300 (thiscall): the attachment holding `child` (and parented to `model`) taken off
int __fastcall DetachModel(uint8_t* self, re::Edx, uint8_t* model, uint8_t* child) {
    if (model == nullptr || child == nullptr) {
        return 0x16;
    }
    if (At<void*>(model, 0x94) == nullptr || At<void*>(child, 0x94) == nullptr) {
        return 0xB;
    }
    if (At<uint32_t>(model, 0xC4) == 0) {
        return 0x1D;
    }
    uint8_t* node = nullptr;
    re::Ptr32<uint8_t> link = At<uint8_t*>(model, 0xB0);
    for (; link; link = At<uint8_t*>(link, 8)) {
        node = At<uint8_t*>(link, 0);
        if (At<uint8_t*>(node, 0x14) == child && At<uint8_t*>(At<uint8_t*>(node, 0x14), 0x9C) == model) {
            break;
        }
    }
    re::Ptr32<uint8_t> none = nullptr;
    if (AmMod_HandleEquals(&link, &none) != 0) {
        return 0x1D;
    }
    return DetachLinked(self, model, node);
}

// 0x6C2450 (thiscall): a hierarchy object's HXD changed (7: the same one; the old one
// released, the new one's users counted), every model on the hierarchy given the new
// skeleton (its first track restarted and resumed when the HXD has anims)
int __fastcall SetHierarchyHxd(uint8_t* self, re::Edx, uint8_t* hierarchy, uint8_t* hxd) {
    if (hierarchy == nullptr) {
        return 0x16;
    }
    uint8_t* old = At<uint8_t*>(hierarchy, 0x10);
    if (hxd == old) {
        return 7;
    }
    if (old != nullptr) {
        AmMod_ReleaseHxd(self, old);
    }
    At<uint8_t*>(hierarchy, 0x10) = hxd;
    if (hxd != nullptr) {
        At<uint32_t>(hierarchy, 0x18) = At<uint32_t>(hxd, 0x18);
        At<uint32_t>(hxd, 0x14) += 1;
    } else {
        At<uint32_t>(hierarchy, 0x18) = 0;
    }
    for (uint8_t* link = At<uint8_t*>(self, 0x11C); link; link = At<uint8_t*>(link, 8)) {
        uint8_t* model = At<uint8_t*>(link, 0);
        if (At<uint8_t*>(model, 0x94) != hierarchy) {
            continue;
        }
        if (hxd != nullptr) {
            At<void*>(model, 0x14) = AmMod_SkeletonOf(At<void*>(model, 0x10));
        } else {
            At<void*>(model, 0x14) = nullptr;
        }
        model = At<uint8_t*>(link, 0);
        AmMod_ForAllObjects(At<void*>(model, 0x10), kSetSkeletonCallback, At<void*>(model, 0x14));
        if (hxd != nullptr && At<uint32_t>(hxd, 4) != 0) {
            AmMod_StartTrack(self, At<uint8_t*>(link, 0), 0, nullptr, 0, 1.0f, -1.0f);
            model = At<uint8_t*>(link, 0);
            uint8_t* track = nullptr;
            uint32_t i = 0;
            for (; i < 7; i++) {
                if (At<uint8_t*>(model, 0x18 + i * 4) != nullptr) {
                    track = At<uint8_t*>(model, 0x18 + i * 4);
                    break;
                }
            }
            if (i >= 7) {
                track = AmMod_NewTrack();
                At<uint32_t>(model, 0x88) += 1;
                At<uint8_t*>(model, 0x18) = track;
            }
            At<uint32_t>(track, 0x28) &= ~1u;
            At<uint32_t>(track, 0x3C) &= ~1u;
        }
    }
    return 0;
}

// a NiFixedString let go of
inline void ReleaseFixedString(re::Ptr32<char>* s) {
    if (*s == ammod_emptyString) {
        return;
    }
    uint32_t length = *reinterpret_cast<uint16_t*>(*s - 2);
    if (vc::Decrement(*s - 8) == 1) {
        AmMod_StringTableFree(ammod_stringTable, s, length);
    }
}

// 0x6C2730 (thiscall): a hierarchy object with its model (NAME.NIF: only a "MOT_CTRL" one
// gets an empty clump) and its HXD (unless it has one and is locked), loaded or found by
// name / hash (or *out as it is); `lock` sets flag 2. Released again when the HXD fails.
int __fastcall LoadHierarchyNif(uint8_t* self, re::Edx, const char* name, uint32_t hash, int lock, re::Ptr32<uint8_t>* out) {
    if (name == nullptr && *out == nullptr && hash == 0) {
        return 0x16;
    }
    re::Ptr32<void> found = nullptr;
    int have;
    uint8_t* hierarchy;
    if (out != nullptr && *out != nullptr) {
        have = 1;
        hierarchy = *out;
    } else {
        have = AmMod_FindHierarchy(self, hash, name, &found);
        hierarchy = static_cast<uint8_t*>(found);
    }
    bool loaded = false;
    int result = 0;
    if (name != nullptr && !(have != 0 && (At<uint8_t>(hierarchy, 4) & 1))) {
        if (have == 0) {
            hierarchy = static_cast<uint8_t*>(AmMod_TakeHierarchy(self));
            if (hierarchy == nullptr) {
                return 0xC;
            }
            result = AmMod_LoadHierarchyFile(hierarchy, name, nullptr, nullptr);
            if (result != 0) {
                AmMod_ReleaseHierarchyLink(self, At<uint8_t*>(self, 8));
                return result;
            }
        }
        if (lock != 0) {
            At<uint32_t>(hierarchy, 4) |= 2;
        }
        char path[0x80];
        path[0] = 0;
        std::strcat(path, name);
        std::strcat(path, ".NIF");
        re::Ptr32<char> nif = AmMod_FixedStringCreate(path);
        if (AmMod_FixedStringContains(&nif, "MOT_CTRL") != 0) {
            At<void*>(hierarchy, 8) = AmMod_CreateEmptyClump(0x1B647);
        }
        ReleaseFixedString(&nif);
        loaded = true;
    }
    re::Ptr32<uint8_t> hxd = nullptr;
    uint32_t hxdHash = At<uint32_t>(hierarchy, 0x18);
    if (hxdHash != 0) {
        bool load = true;
        if (At<uint8_t*>(hierarchy, 0x10) != nullptr) {
            hxd = At<uint8_t*>(hierarchy, 0x10);
            if (At<uint8_t>(hierarchy, 4) & 2) {
                load = false;
            }
        }
        if (load) {
            result = AmMod_LoadHxd(self, nullptr, hxdHash, lock, &hxd);
            if (result != 0) {
                AmMod_ReleaseHierarchyLink(self, At<uint8_t*>(self, 8));
                return result;
            }
        }
    }
    if (loaded) {
        At<uint32_t>(hierarchy, 4) |= 1;
        At<uint8_t*>(hierarchy, 0x10) = hxd;
        if (hxd != nullptr) {
            At<uint32_t>(hxd, 0x14) += 1;
        }
    }
    if (out != nullptr) {
        *out = hierarchy;
    }
    return 0;
}

// 0x6C2950 (thiscall): a hierarchy object's clump set (null: cleared; 0x11 had none; 7 has
// one already), its HXD loaded and locked first; with `reroot`, a one-atomic clump whose
// root frame has two children or more gets two new frames (copies of the root's and its
// +0x84's transforms, the second holding the bone table) in place of the root
int __fastcall SetClump(void* self, re::Edx, uint8_t* hierarchy, void* clump, int reroot) {
    if (hierarchy == nullptr) {
        return 0x16;
    }
    if (clump == nullptr) {
        if (At<void*>(hierarchy, 8) == nullptr) {
            return 0x11;
        }
        At<uint32_t>(hierarchy, 4) &= ~1u;
        At<void*>(hierarchy, 8) = nullptr;
        return 0;
    }
    if (At<void*>(hierarchy, 8) != nullptr || (At<uint8_t>(hierarchy, 4) & 1)) {
        return 7;
    }
    if (At<uint32_t>(hierarchy, 0x18) != 0) {
        int result = AmMod_LoadHxd(self, nullptr, At<uint32_t>(hierarchy, 0x18), 1,
                                   reinterpret_cast<re::Ptr32<uint8_t>*>(hierarchy + 0x10));
        if (result != 0) {
            return result;
        }
    }
    if (uint8_t(reroot) != 0 && AmMod_AtomicCount(clump) == 1) {
        uint8_t* root = static_cast<uint8_t*>(AmMod_GetFrame(clump));
        void* child = AmMod_FrameRoot(root);   // (the root frame's root)
        int children = 1;
        AmMod_ForAllChildren(child, kCountChildren, &children);
        if (children > 2) {
            void* atomic = AmMod_FirstAtomic(clump);
            void* top = AmMod_NewFrame();
            uint32_t* topTransform = AmMod_FrameTransform(top);
            void* bones = AmMod_NewFrame();
            uint32_t* bonesTransform = AmMod_FrameTransform(bones);
            AmMod_AddChild(top, bones);
            const uint32_t* from = AmMod_FrameTransform(root);
            for (int i = 0; i < 13; i++) {
                topTransform[i] = from[i];
            }
            from = AmMod_FrameTransform(At<void*>(root, 0x84));
            for (int i = 0; i < 13; i++) {
                bonesTransform[i] = from[i];
            }
            AmMod_SetBoneTable(bones, AmMod_HierarchyOf(clump));
            AmMod_SetClumpFrame(clump, top);
            AmMod_SetAtomicFrame(atomic, bones);
            AmMod_UpdateFrame(top);
            AmMod_UpdateFrame(bones);
            AmMod_DestroyFrame(root);
        }
    }
    At<uint32_t>(hierarchy, 4) |= 9;
    At<void*>(hierarchy, 8) = clump;
    return 0;
}

// 0x6C2AE0 (thiscall): a hierarchy file loaded from its name (or a stream, the name then
// as it is) or found, with the HXD it names (its users counted), marked kept
int __fastcall LoadHierarchyWithHxd(uint8_t* self, re::Edx, const char* name, re::Ptr32<void>* out, FileStream* stream) {
    re::Ptr32<void> found = nullptr;
    re::Ptr32<uint8_t> hxd = nullptr;
    if (name == nullptr) {
        return 0x16;
    }
    char local[0x40];
    char hxdName[0x40];
    uint8_t* hierarchy = nullptr;
    bool have = false;
    if (stream == nullptr) {
        int result = AmMod_MakePath(self, name, local, 0);
        if (result != 0) {
            return result;
        }
        if (AmMod_FindHierarchy(self, 0, local, &found) != 0) {
            hierarchy = static_cast<uint8_t*>(found);
            have = true;
        }
    } else {
        std::strcpy(local, name);
    }
    if (!have) {
        hierarchy = static_cast<uint8_t*>(AmMod_TakeHierarchy(self));
        if (hierarchy == nullptr) {
            return 0xC;
        }
        int result = AmMod_LoadHierarchyFile(hierarchy, local, stream, hxdName);
        if (result != 0) {
            AmMod_ReleaseHierarchyLink(self, At<uint8_t*>(self, 8));
            return result;
        }
        if (At<uint32_t>(hierarchy, 0x18) != 0) {
            result = AmMod_LoadHxdFile(self, At<uint32_t>(hierarchy, 0x18), hxdName, &hxd, 0);
            if (result != 0) {
                return result;
            }
            At<uint8_t*>(hierarchy, 0x10) = hxd;
            At<uint32_t>(hxd, 0x14) += 1;
        }
    }
    At<uint32_t>(hierarchy, 4) |= 4;
    if (out != nullptr) {
        *out = hierarchy;
    }
    return 0;
}

// 0x6C2C00 (thiscall): a model removed (0x1E: not added): its tracks blended out and
// dropped, its attachments and blend nodes taken off, its lists given back, its clump
// destroyed (unless `keepClump`), unlinked from the manager and its hierarchy released
int __fastcall RemoveModel(uint8_t* self, re::Edx, uint8_t* model, int keepClump) {
    if (model == nullptr) {
        return 0x16;
    }
    if (!(At<uint32_t>(model, 0x98) & 0x800)) {
        return 0x1E;
    }
    for (uint32_t i = 0; i < 7; i++) {
        uint8_t* track = At<uint8_t*>(model, 0x18 + i * 4);
        if (track != nullptr) {
            AmMod_BlendOut(self, model, track, 0.0f);
            At<uint8_t*>(model, 0x18 + i * 4) = nullptr;
        }
        AmMod_EmptyList(model, i);
    }
    if (At<uint32_t>(model, 0xC4) != 0) {
        while (At<uint8_t*>(model, 0xB0) != nullptr) {
            AmMod_DetachNode(self, model, At<uint8_t*>(At<uint8_t*>(model, 0xB0), 0));
        }
    }
    if (At<uint32_t>(model, 0xF0) != 0) {
        while (At<uint8_t*>(model, 0xDC) != nullptr) {
            AmMod_RemoveBlendNode(self, model, At<uint8_t*>(At<uint8_t*>(model, 0xDC), 0));
        }
    }
    ReleaseShared(reinterpret_cast<LinkPool*>(model + 0xAC));
    ReleaseShared(reinterpret_cast<LinkPool*>(model + 0xD8));
    At<uint32_t>(model, 0x98) &= 0xFFFFF7FEu;
    if (uint8_t(keepClump) == 0) {
        AmMod_DestroyClump(At<void*>(model, 0x10));
    }
    uint8_t* link = At<uint8_t*>(model, 0xA0);
    uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
    At<void*>(model, 0x10) = nullptr;
    At<void*>(model, 0xC) = nullptr;
    At<void*>(model, 0x14) = nullptr;
    At<void*>(model, 0x94) = nullptr;
    LinkPool* models = reinterpret_cast<LinkPool*>(self + 0x118);
    if (link == models->head) {
        models->head = At<uint8_t*>(link, 8);
    } else {
        At<uint8_t*>(At<uint8_t*>(link, 4), 8) = At<uint8_t*>(link, 8);
    }
    if (link == models->tail) {
        models->tail = At<uint8_t*>(link, 4);
    } else {
        At<uint8_t*>(At<uint8_t*>(link, 8), 4) = At<uint8_t*>(link, 4);
    }
    At<uint8_t*>(link, 8) = models->owner->freeLinks;
    models->owner->freeLinks = link;
    models->used -= 1;
    At<void*>(model, 0xA0) = nullptr;
    ReleaseHierarchyObject(self, hierarchy);
    return 0;
}

// 0x6C2E50 (thiscall): a .MGR file (a count, then 0x40-byte names) read whole and each
// hierarchy loaded from it (0x1D: no stream)
int __fastcall LoadMgr(uint8_t* self, re::Edx, const char* path) {
    if (path == nullptr) {
        return 0x16;
    }
    char name[0x80];
    name[0] = 0;
    std::strcat(name, path);
    void* file = AmMod_OpenFile(name, "r", 1);
    AmMod_Seek(file, 0, 2);
    uint32_t size = AmMod_Tell(file);
    AmMod_Seek(file, 0, 0);
    AmMod_PushMemoryTag(0x13);
    void* data = AmMod_Malloc(size);
    AmMod_PopMemoryTag();
    AmMod_Read(file, data, int(size));
    AmMod_Close(file);
    FileStreamBuffer memory{static_cast<uint8_t*>(data), size};
    FileStream* stream = AmMod_OpenStream(3, 1, &memory);
    if (stream == nullptr) {
        return 0x1D;
    }
    uint32_t count = 0;   // (the game leaves it unset when the stream has neither)
    if (stream->file) {
        AmMod_ReadItems(&count, 4, 1, stream->file);
    } else if (stream->data) {
        count = *reinterpret_cast<uint32_t*>(stream->data + stream->position);
        stream->position += 4;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (stream->file) {
            AmMod_ReadItems(name, 0x40, 1, stream->file);
        } else if (stream->data) {
            AmMod_Memcpy(name, stream->data + stream->position, 0x40);
            stream->position += 0x40;
        }
        AmMod_LoadHierarchyWithHxd(self, name, nullptr, stream);
    }
    AmMod_CloseStream(stream, nullptr);
    AmMod_Free(data);
    return 0;
}

// 0x6C2FE0 (thiscall): one hierarchy file (.MXD) loaded from a file stream (0x1D: none;
// the stream's left open when the name's bad)
int __fastcall LoadMxd(uint8_t* self, re::Edx, const char* path) {
    if (path == nullptr) {
        return 0x16;
    }
    char name[0x80];
    name[0] = 0;
    std::strcat(name, path);
    FileStream* stream = AmMod_OpenStream(2, 1, name);
    if (stream == nullptr) {
        return 0x1D;
    }
    char local[0x40];
    int result = AmMod_MakePath(self, name, local, 0);
    if (result == 0) {
        AmMod_LoadHierarchyWithHxd(self, local, nullptr, stream);
        AmMod_CloseStream(stream, nullptr);
        result = 0;
    }
    return result;
}

// 0x6C30A0 (thiscall): a model added on a hierarchy object (9: has one; 0xF: 200 models):
// its HXD loaded unless there and locked (the hierarchy's last used link given back when
// that fails), the clump shared or cloned, the skeleton set, the anim (-1 or out of range:
// the first with data) started when asked, its lists made and it listed
int __fastcall AddModel(uint8_t* self, re::Edx, uint8_t* model, int anim, uint8_t* hierarchy, int shareClump,
                        int start) {
    if (model == nullptr) {
        return 0x16;
    }
    if (At<void*>(model, 0x94) != nullptr) {
        return 9;
    }
    if (At<int32_t>(self, 0x130) == 200) {
        return 0xF;
    }
    if (hierarchy == nullptr) {
        return 0x16;
    }
    uint32_t hash = At<uint32_t>(hierarchy, 0x18);
    re::Ptr32<uint8_t> hxd = nullptr;
    if (hash != 0) {
        bool load = true;
        if (At<uint8_t*>(hierarchy, 0x10) != nullptr) {
            hxd = At<uint8_t*>(hierarchy, 0x10);
            if (At<uint8_t>(hierarchy, 4) & 2) {
                load = false;
            }
        }
        if (load) {
            int result = AmMod_LoadHxd(self, nullptr, hash, 0, &hxd);
            if (result != 0) {
                AmMod_ReleaseHierarchyLink(self, At<uint8_t*>(self, 8));
                return result;
            }
        }
    }
    At<uint32_t>(hierarchy, 0) += 1;
    At<uint32_t>(model, 0x98) |= 0x801;
    AmMod_PushMemoryTag(0x24);
    At<uint8_t*>(model, 0x94) = hierarchy;
    At<uint32_t>(model, 0x88) = 0;
    if (uint8_t(shareClump) == 0) {
        At<void*>(model, 0x10) = AmMod_CloneClump(At<void*>(hierarchy, 8));
    } else {
        At<void*>(model, 0x10) = At<void*>(hierarchy, 8);
    }
    AmMod_PopMemoryTag();
    At<void*>(model, 0xC) = AmMod_GetFrame(At<void*>(model, 0x10));
    uint8_t* modelHxd = At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10);
    if (modelHxd != nullptr) {
        int index = anim;
        if (index < -1 || index >= At<int32_t>(modelHxd, 4)) {
            index = -1;
        }
        void* skeleton = AmMod_SkeletonOf(At<void*>(model, 0x10));
        At<void*>(model, 0x14) = skeleton;
        AmMod_ForAllObjects(At<void*>(model, 0x10), kSetSkeletonCallback, skeleton);
        AmMod_ComputeParents(model);
        if (index == -1) {
            uint8_t* h = At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10);
            uint32_t count = At<uint32_t>(h, 4);
            if (count != 0) {
                uint32_t i = 0;
                if (At<void*>(At<uint8_t*>(h, 0), 0) == nullptr) {
                    // (reads the anim past the last when none has data)
                    for (;;) {
                        if (i >= count) {
                            break;
                        }
                        i++;
                        uint8_t* anims = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(model, 0x94), 0x10), 0);
                        if (At<void*>(anims, int(i * 0x40)) != nullptr) {
                            break;
                        }
                    }
                }
                index = int(i);
            }
        }
        if (uint8_t(start) != 0) {
            AmMod_StartTrack(self, model, index, nullptr, 1, 1.0f, -1.0f);
        }
    }
    ShareLinks(reinterpret_cast<LinkPool*>(model + 0xAC), reinterpret_cast<LinkPool*>(self + 0xB8));
    ShareLinks(reinterpret_cast<LinkPool*>(model + 0xD8), reinterpret_cast<LinkPool*>(self + 0xE8));
    AmMod_ListPushBack(self + 0x118, model);
    At<uint8_t*>(model, 0xA0) = At<uint8_t*>(self, 0x120);
    return 0;
}

// a vector-new'd array's elements destroyed back to front, then freed
inline void DeleteArray(uint8_t* array, uint32_t size, void(__thiscall* destruct)(void*)) {
    if (array == nullptr) {
        return;
    }
    int32_t count = *reinterpret_cast<int32_t*>(array - 4);
    uint8_t* at = array + count * size;
    for (int32_t i = count - 1; i >= 0; i--) {
        at -= size;
        destruct(at);
    }
    AmMod_Free(array - 4);
}

// 0x6C32D0 (thiscall): the manager shut down: every model removed, the hierarchy objects'
// clumps (unless kept, flag 8) and HXDs released and the objects reset into the free list,
// the models' lists given back, every pool's links and arrays freed, the scratch poses
void __fastcall Shutdown(uint8_t* self, re::Edx) {
    while (At<uint8_t*>(self, 0x11C) != nullptr) {
        AmMod_RemoveModel(self, At<uint8_t*>(At<uint8_t*>(self, 0x11C), 0), 0);
    }
    LinkPool* hierUsed = reinterpret_cast<LinkPool*>(self);
    for (uint8_t* link = hierUsed->head; link; link = At<uint8_t*>(link, 8)) {
        uint8_t* hierarchy = At<uint8_t*>(link, 0);
        if (!(At<uint8_t>(hierarchy, 4) & 8) && At<void*>(hierarchy, 8) != nullptr) {
            AmMod_DestroyClump(At<void*>(hierarchy, 8));
            At<void*>(hierarchy, 8) = nullptr;
        }
        if (At<uint8_t*>(hierarchy, 0x10) != nullptr) {
            AmMod_ReleaseHxd(self, At<uint8_t*>(hierarchy, 0x10));
        }
        uint8_t* found = hierUsed->head;
        while (found && At<uint8_t*>(found, 0) != hierarchy) {
            found = At<uint8_t*>(found, 8);
        }
        AmMod_HierReset(hierarchy);
        if (found == nullptr) {
            found = hierUsed->tail;
        }
        if (found == hierUsed->head) {
            hierUsed->head = At<uint8_t*>(found, 8);
        } else {
            At<uint8_t*>(At<uint8_t*>(found, 4), 8) = At<uint8_t*>(found, 8);
        }
        if (found == hierUsed->tail) {
            hierUsed->tail = At<uint8_t*>(found, 4);
        } else {
            At<uint8_t*>(At<uint8_t*>(found, 8), 4) = At<uint8_t*>(found, 4);
        }
        hierUsed->used -= 1;
        AmMod_ListInsertRange(self + 0x2C, nullptr, found, found);
        At<int32_t>(self + 0x2C, 0x18) += 1;
    }
    for (uint8_t* link = At<uint8_t*>(self, 0x11C); link; link = At<uint8_t*>(link, 8)) {
        ReleaseShared(reinterpret_cast<LinkPool*>(At<uint8_t*>(link, 0) + 0xAC));
        ReleaseShared(reinterpret_cast<LinkPool*>(At<uint8_t*>(link, 0) + 0xD8));
    }
    ReleaseShared(reinterpret_cast<LinkPool*>(self + 0x2C));
    ReleaseOwn(hierUsed);
    DeleteArray(At<uint8_t*>(self, 0x58), 0xB0, AmMod_HierDestruct);
    At<void*>(self, 0x58) = nullptr;
    ReleaseShared(reinterpret_cast<LinkPool*>(self + 0x88));
    ReleaseOwn(reinterpret_cast<LinkPool*>(self + 0x5C));
    DeleteArray(At<uint8_t*>(self, 0xB4), 0x7C, AmMod_HxdDestruct);
    At<void*>(self, 0xB4) = nullptr;
    AmMod_Fn6B98D0();
    ReleaseOwn(reinterpret_cast<LinkPool*>(self + 0xB8));
    AmMod_Free(At<void*>(self, 0xE4));
    At<void*>(self, 0xE4) = nullptr;
    ReleaseOwn(reinterpret_cast<LinkPool*>(self + 0xE8));
    AmMod_Free(At<void*>(self, 0x114));
    At<void*>(self, 0x114) = nullptr;
    ReleaseOwn(reinterpret_cast<LinkPool*>(self + 0x118));
    for (int i = 0; i < 4; i++) {
        AmMod_FreeThunk(At<void*>(self, 0x148 + i * 4));
    }
}

} // namespace

#define AMMOD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMMOD_REVERSED("AM_Manager::Construct", 0x006C2580, &Construct);
AMMOD_REVERSED("AM_Manager::AttachNode", 0x006C2070, &AttachNode);
AMMOD_REVERSED("AM_Manager::DetachNode", 0x006C2200, &DetachNode);
AMMOD_REVERSED("AM_Manager::DetachModel", 0x006C2300, &DetachModel);
AMMOD_REVERSED("AM_Manager::SetHierarchyHxd", 0x006C2450, &SetHierarchyHxd);
AMMOD_REVERSED("AM_Manager::LoadHierarchyNif", 0x006C2730, &LoadHierarchyNif);
AMMOD_REVERSED("AM_Manager::SetClump", 0x006C2950, &SetClump);
AMMOD_REVERSED("AM_Manager::LoadHierarchyWithHxd", 0x006C2AE0, &LoadHierarchyWithHxd);
AMMOD_REVERSED("AM_Manager::RemoveModel", 0x006C2C00, &RemoveModel);
AMMOD_REVERSED("AM_Manager::LoadMgr", 0x006C2E50, &LoadMgr);
AMMOD_REVERSED("AM_Manager::LoadMxd", 0x006C2FE0, &LoadMxd);
AMMOD_REVERSED("AM_Manager::AddModel", 0x006C30A0, &AddModel);
AMMOD_REVERSED("AM_Manager::Shutdown", 0x006C32D0, &Shutdown);
