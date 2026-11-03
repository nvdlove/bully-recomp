// The AM_ animation manager (0x20C4910; Rockstar Vancouver's anim library): its setup (the
// pooled lists of hierarchy objects and HXD files, the blend and 0x18-byte node pools, the
// four scratch poses, the key codecs registered), taking and giving back pooled objects,
// the codec table (0x20C5C58, 16 at most), a scratch pose's allocation, the 0x3C-byte
// blend node's constructor, and the anim groups' unload timers.
// 0x6BE840, 0x6BF670..0x6C0703, 0x6C0A00..0x6C0B17, 0x6C80A0..0x6C81C5.
//
// The manager (all lists are RW-style link pools, 0x2C; see RwLinkPool.cpp):
//   +0x000 hierarchy objects in use (its own 0x334 links), +0x02C the free ones,
//   +0x058 the 0xB0-byte hierarchy objects (0x334), +0x05C HXDs in use (200 links),
//   +0x088 the free HXDs, +0x0B4 the HXDs (0x7C each, 200), +0x0B8 the 0x18-byte nodes
//   (250, at +0xE4), +0x0E8 the blend nodes (250 of 0x3C, at +0x114), +0x118 200 more
//   links, +0x144 the pose being written, +0x148 four scratch poses (0x40 joints, 0x24 each),
//   +0x158 the pending callbacks' count, +0x15C them.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct LinkPool {
    uint32_t unknown0;     // +0x0
    re::Ptr32<uint8_t> head;         // +0x4
    re::Ptr32<uint8_t> tail;         // +0x8
    re::Ptr32<uint8_t> links;        // +0xC (0xC each: value, previous, next)
    re::Ptr32<uint8_t> freeLinks;    // +0x10
    int32_t count;         // +0x14
    int32_t used;          // +0x18
    int32_t owned;         // +0x1C
    uint32_t unknown20;    // +0x20
    re::Ptr32<LinkPool> owner;       // +0x24 (whose free links are used)
    int32_t users;         // +0x28
};
static_assert(sizeof(LinkPool) == 0x2C);

GAME_ARRAY(uint32_t, amset_zeroVector, 0x00BCC564);
GAME_VAR(int32_t, amset_codecCount, 0x020C5F58);
GAME_ARRAY(uint8_t, amset_codecs, 0x020C5C58);   // 0x30 each
GAME_VAR(float, amset_msPerStep, 0x00BB6454);   // 0.02
GAME_VAR(float, amset_timeStep, 0x00C1A9A4);    // CTimer::ms_fTimeStep

GAME_FN(void*, __cdecl, AmSet_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, AmSet_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, AmSet_PopMemoryTag, 0x005EF100);
GAME_FN(void, __stdcall, AmSet_VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count,
        void* ctor);
GAME_FN(void, __thiscall, AmSet_LinkPoolInit, 0x005EF750, LinkPool* self, uint32_t count);
GAME_FN(void, __thiscall, AmSet_LinkPoolReset, 0x005EF6C0, LinkPool* self);
GAME_FN(void, __thiscall, AmSet_HxdConstruct, 0x006B2EB0, void* self);
GAME_FN(void, __thiscall, AmSet_HierReset, 0x006B1E30, void* self);
GAME_FN(void, __thiscall, AmSet_HxdReset, 0x006B1B20, void* self);
GAME_FN(void, __thiscall, AmSet_ListMoveFrom, 0x006B2CA0, void* self, re::Ptr32<uint8_t>* before, void* source,
        re::Ptr32<uint8_t>* link);
GAME_FN(int, __thiscall, AmSet_ReleaseGroup, 0x006B3DA0, uint8_t* hxd, uint32_t group, int keep);
GAME_FN(void, __cdecl, AmSet_Fn6B9610, 0x006B9610);
GAME_FN(void, __cdecl, AmSet_Fn72FF70, 0x0072FF70);
GAME_FN(uint8_t*, __cdecl, AmSet_FindCodec, 0x006C80B0, uint32_t kind);
GAME_FN(int, __cdecl, AmSet_AddCodec, 0x006C8100, const uint32_t* codec);
GAME_FN(uint8_t*, __cdecl, AmSet_NewPose, 0x006C8150, uint32_t count, uint32_t stride);
inline void* const kHierConstruct = reinterpret_cast<void*>(0x006B1DF0);

inline uint32_t ArrayBytes(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return (bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes);
}

// a value added at the list's end (a link from its owner's free ones: 0x6BF670, inlined)
inline void PushBack(LinkPool* list, void* value) {
    uint8_t* link = list->owner->freeLinks;
    list->owner->freeLinks = At<uint8_t*>(link, 8);
    list->used++;
    At<void*>(link, 0) = value;
    At<uint8_t*>(link, 4) = list->tail;
    At<uint8_t*>(link, 8) = nullptr;
    uint8_t* tail = list->tail;
    if (list->head == tail && tail == nullptr) {
        list->head = link;
    } else {
        At<uint8_t*>(tail, 8) = link;
    }
    list->tail = link;
}

// `count` zeroed links of its own (the reset inlined unless `callReset`)
inline void OwnLinks(LinkPool* pool, int32_t count, bool callReset) {
    pool->count = count;
    uint8_t* links = static_cast<uint8_t*>(AmSet_Malloc(ArrayBytes(uint32_t(count), 0xC)));
    if (links) {
        for (int i = 0; i < count; i++) {
            At<uint32_t>(links, i * 0xC) = 0;
            At<uint32_t>(links, i * 0xC + 4) = 0;
            At<uint32_t>(links, i * 0xC + 8) = 0;
        }
    }
    pool->links = links;
    pool->owner = pool;
    pool->owned = 1;
    if (callReset) {
        AmSet_LinkPoolReset(pool);
        return;
    }
    pool->freeLinks = nullptr;
    for (int32_t i = 0; i < pool->count; i++) {
        uint8_t* link = pool->links + i * 0xC;
        At<uint8_t*>(link, 8) = pool->owner->freeLinks;
        pool->owner->freeLinks = link;
        pool->used--;
    }
    pool->used = 0;
}

// a list sharing another's links
inline void ShareLinks(LinkPool* list, LinkPool* owner) {
    owner->users++;
    list->count = owner->count;
    list->links = nullptr;
    list->freeLinks = nullptr;
    list->owner = owner;
    list->owned = 0;
}

// 0x6BE840 (thiscall): a blend node (0x3C): no rotation, no offset, the low three flag bits
// of +0x38 cleared
void BlendNodeInit(uint32_t* node) {
    node[10] = 0;
    node[0] = 0;
    node[11] = 0;
    node[8] = 0;
    node[9] = 0;
    node[13] = 0;
    node[12] = 0;
    node[1] = amset_zeroVector[0];
    node[2] = amset_zeroVector[1];
    node[3] = amset_zeroVector[2];
    reinterpret_cast<float*>(node)[4] = 1.0f;
    reinterpret_cast<float*>(node)[5] = 0.0f;
    reinterpret_cast<float*>(node)[6] = 0.0f;
    reinterpret_cast<float*>(node)[7] = 0.0f;
    node[14] &= 0xFFFFFFF8u;
}
void __fastcall BlendNode_Construct(uint32_t* self, re::Edx) {
    // (fldz stored to +0x28/+0x2C, the same bits as the zeros)
    BlendNodeInit(self);
}

// 0x6BF820 (thiscall): the manager's setup, once (+0x58 set)
void __fastcall Init(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0x58) != nullptr) {
        return;
    }
    LinkPool* hierUsed = reinterpret_cast<LinkPool*>(self);
    LinkPool* hierFree = reinterpret_cast<LinkPool*>(self + 0x2C);
    AmSet_LinkPoolInit(hierUsed, 0x334);
    ShareLinks(hierFree, hierUsed);
    uint32_t* block = static_cast<uint32_t*>(AmSet_Malloc(0x334 * 0xB0 + 4));
    uint8_t* hiers = nullptr;
    if (block) {
        block[0] = 0x334;
        hiers = reinterpret_cast<uint8_t*>(block + 1);
        AmSet_VectorConstruct(hiers, 0xB0, 0x334, kHierConstruct);
    }
    At<uint8_t*>(self, 0x58) = hiers;
    for (int i = 0; i < 0x334; i++) {
        PushBack(hierFree, At<uint8_t*>(self, 0x58) + i * 0xB0);
    }

    LinkPool* hxdUsed = reinterpret_cast<LinkPool*>(self + 0x5C);
    LinkPool* hxdFree = reinterpret_cast<LinkPool*>(self + 0x88);
    OwnLinks(hxdUsed, 200, true);
    ShareLinks(hxdFree, hxdUsed);
    block = static_cast<uint32_t*>(AmSet_Malloc(200 * 0x7C + 4));
    uint8_t* hxds = nullptr;
    if (block) {
        block[0] = 200;
        hxds = reinterpret_cast<uint8_t*>(block + 1);
        for (int i = 0; i < 200; i++) {
            AmSet_HxdConstruct(hxds + i * 0x7C);
        }
    }
    At<uint8_t*>(self, 0xB4) = hxds;
    for (int i = 0; i < 200; i++) {
        PushBack(hxdFree, At<uint8_t*>(self, 0xB4) + i * 0x7C);
    }

    OwnLinks(reinterpret_cast<LinkPool*>(self + 0x118), 200, false);
    AmSet_Fn6B9610();
    AmSet_Fn72FF70();

    LinkPool* nodes = reinterpret_cast<LinkPool*>(self + 0xB8);
    OwnLinks(nodes, 250, false);
    uint32_t* nodeArray = static_cast<uint32_t*>(AmSet_Malloc(6000));
    if (nodeArray) {
        for (int i = 0; i < 250 * 6; i++) {
            nodeArray[i] = 0;
        }
    }
    At<uint32_t*>(self, 0xE4) = nodeArray;
    for (int i = 0; i < 250; i++) {
        PushBack(nodes, At<uint8_t*>(self, 0xE4) + i * 0x18);
    }

    LinkPool* blends = reinterpret_cast<LinkPool*>(self + 0xE8);
    OwnLinks(blends, 250, false);
    uint32_t* blendArray = static_cast<uint32_t*>(AmSet_Malloc(15000));
    if (blendArray) {
        for (int i = 0; i < 250; i++) {
            uint32_t* node = blendArray + i * 15;
            node[0] = 0;   // (+0..+0xC zeroed first; the vector's overwritten)
            node[1] = 0;
            node[2] = 0;
            node[3] = 0;
            BlendNodeInit(node);
        }
    }
    At<uint32_t*>(self, 0x114) = blendArray;
    for (int i = 0; i < 250; i++) {
        PushBack(blends, At<uint8_t*>(self, 0x114) + i * 0x3C);
    }

    At<uint32_t>(self, 0x144) = 0;
    for (int i = 0; i < 4; i++) {
        At<uint8_t*>(self, 0x148 + i * 4) = AmSet_NewPose(0x40, 0x24);
    }

    // the key codecs: all share the pose functions, each its sampler, loader and two more
    uint32_t codec[12] = {999, 0x24, 0x24, 0x006C7820, 0x006C7B40, 0x006BBB20, 0x006C7740, 0,
                          0x006BBAC0, 0x006BBAF0, 0x006BB570, 0};
    AmSet_AddCodec(codec);
    codec[0] = 1000;
    codec[5] = 0x006BC3C0;
    codec[8] = 0x006BBC50;
    codec[9] = 0x006BBCC0;
    codec[10] = 0x006BB580;
    AmSet_AddCodec(codec);
    codec[0] = 1001;
    codec[5] = 0x006BC5F0;
    codec[8] = 0x006BBD30;
    codec[9] = 0x006BBDC0;
    codec[10] = 0x006BB5C0;
    AmSet_AddCodec(codec);
    codec[0] = 1002;
    codec[5] = 0x006BC930;
    codec[8] = 0x006BBE50;
    codec[9] = 0x006BBED0;
    codec[10] = 0x006BB610;
    AmSet_AddCodec(codec);
    codec[0] = 1003;
    codec[8] = 0x006BBF50;
    codec[9] = 0x006BBF80;
    codec[5] = 0x006BBFB0;
    codec[10] = 0x006BB650;
    AmSet_AddCodec(codec);
    codec[0] = 1004;
    codec[8] = 0x006BC1E0;
    codec[9] = 0x006BC210;
    codec[5] = 0x006BC240;
    codec[10] = 0x006BB660;
    AmSet_AddCodec(codec);
}

// the pooled objects: taken from the free list into the used one, given back reset
template <int Used, int Free>
void* Take(uint8_t* self) {
    re::Ptr32<uint8_t> link = At<uint8_t*>(self + Free, 4);
    if (link == nullptr) {
        return nullptr;
    }
    re::Ptr32<uint8_t> before = nullptr;
    AmSet_ListMoveFrom(self + Used, &before, self + Free, &link);
    return At<void*>(link, 0);
}

// 0x6BF6C0 (thiscall): a hierarchy object (null: none left)
void* __fastcall TakeHierarchy(uint8_t* self, re::Edx) {
    return Take<0, 0x2C>(self);
}

// 0x6BF790 (thiscall): an HXD (null: none left)
void* __fastcall TakeHxd(uint8_t* self, re::Edx) {
    return Take<0x5C, 0x88>(self);
}

// 0x6BF700 (thiscall): a used hierarchy object's link given back (its object reset)
void __fastcall ReleaseHierarchyLink(uint8_t* self, re::Edx, re::Ptr32<uint8_t> link) {
    if (link) {
        AmSet_HierReset(At<void*>(link, 0));
    }
    re::Ptr32<uint8_t> before = nullptr;
    AmSet_ListMoveFrom(self + 0x2C, &before, self, &link);
}

// 0x6BF7E0 (thiscall): a used HXD's link given back (the HXD reset)
void __fastcall ReleaseHxdLink(uint8_t* self, re::Edx, re::Ptr32<uint8_t> link) {
    if (link) {
        AmSet_HxdReset(At<void*>(link, 0));
    }
    re::Ptr32<uint8_t> before = nullptr;
    AmSet_ListMoveFrom(self + 0x88, &before, self + 0x5C, &link);
}

// 0x6BF740 (thiscall): a used hierarchy object given back (reset even when it isn't in the
// list; then the list's last link moves)
void __fastcall ReleaseHierarchy(uint8_t* self, re::Edx, void* object) {
    re::Ptr32<uint8_t> link = At<uint8_t*>(self, 4);
    while (link && At<void*>(link, 0) != object) {
        link = At<uint8_t*>(link, 8);
    }
    AmSet_HierReset(object);
    re::Ptr32<uint8_t> before = nullptr;
    AmSet_ListMoveFrom(self + 0x2C, &before, self, &link);
}

// 0x6BF670 (thiscall): a value added at a list's end
void __fastcall List_PushBack(LinkPool* self, re::Edx, void* value) {
    PushBack(self, value);
}

// 0x6C0A00 (thiscall): flags 0x400 and 0x2000 cleared on every model in the list at +0x118
void __fastcall ClearModelsUpdated(uint8_t* self, re::Edx) {
    for (uint8_t* link = At<uint8_t*>(self, 0x11C); link; link = At<uint8_t*>(link, 8)) {
        At<uint32_t>(At<uint8_t*>(link, 0), 0x98) &= 0xFFFFDBFFu;
    }
}

// 0x6C0A30 (thiscall): the used HXDs' groups waiting to unload (+0x34) count down their
// time (+0, ms) by this step's; at 0 the group's released
void __fastcall UpdateGroupTimers(uint8_t* self, re::Edx) {
    for (uint8_t* link = At<uint8_t*>(self, 0x60); link; link = At<uint8_t*>(link, 8)) {
        uint8_t* hxd = At<uint8_t*>(link, 0);
        if (hxd == nullptr) {
            continue;
        }
        for (uint32_t i = 0; i < At<uint32_t>(hxd, 0x68); i++) {
            uint8_t* group = i < At<uint32_t>(hxd, 0x68) ? At<uint8_t*>(hxd, 0x6C) + i * 0x3C : nullptr;
            if (At<uint8_t>(group, 0x34) == 0 || At<int32_t>(group, 0) == 0) {
                continue;
            }
            float ms = amset_msPerStep * amset_timeStep;
            ms = ms * 1000.0f;
            int32_t left = At<int32_t>(group, 0) - int32_t(int64_t(ms));
            float f = float(left);
            if (f < 0.0f) {
                f = 0.0f;
            }
            int32_t now = int32_t(int64_t(f));
            At<int32_t>(group, 0) = now;
            if (now == 0) {
                AmSet_ReleaseGroup(hxd, i, 0);
            }
        }
    }
}

// 0x6C80A0: the codec table emptied (returns 1)
int __cdecl ClearCodecs() {
    amset_codecCount = 0;
    return 1;
}

// 0x6C80B0: the codec for a key kind (null: none)
uint8_t* __cdecl FindCodec(uint32_t kind) {
    for (int32_t i = 0; i < amset_codecCount; i++) {
        if (At<uint32_t>(amset_codecs, i * 0x30) == kind) {
            return amset_codecs + i * 0x30;
        }
    }
    return nullptr;
}

// 0x6C8100: a codec added (0: the table's full or the kind's there)
int __cdecl AddCodec(const uint32_t* codec) {
    int32_t n = amset_codecCount;
    if (n >= 0x10) {
        return 0;
    }
    if (AmSet_FindCodec(codec[0]) != nullptr) {
        return 0;
    }
    uint32_t* to = reinterpret_cast<uint32_t*>(amset_codecs + n * 0x30);
    for (int i = 0; i < 12; i++) {
        to[i] = codec[i];
    }
    amset_codecCount = n + 1;
    return 1;
}

// 0x6C8150: a scratch pose (0x4C bytes then count entries of `stride`, memory tag 4)
uint8_t* __cdecl NewPose(uint32_t count, uint32_t stride) {
    AmSet_PushMemoryTag(4);
    uint8_t* pose = static_cast<uint8_t*>(AmSet_Malloc(count * stride + 0x4C));
    AmSet_PopMemoryTag();
    At<float>(pose, 4) = 0.0f;
    At<uint32_t>(pose, 0x20) = stride;
    At<uint32_t>(pose, 0x24) = stride;
    At<uint32_t>(pose, 0) = 0;
    At<float>(pose, 0x14) = -1.0f;
    At<uint32_t>(pose, 8) = 0;
    At<uint32_t>(pose, 0xC) = 0;
    At<uint32_t>(pose, 0x10) = 0;
    At<uint32_t>(pose, 0x18) = 0;
    At<uint32_t>(pose, 0x1C) = 0;
    At<uint32_t>(pose, 0x30) = 0;
    At<uint32_t>(pose, 0x34) = 0;
    At<uint32_t>(pose, 0x3C) = 0;
    At<uint32_t>(pose, 0x40) = 0;
    At<uint32_t>(pose, 0x44) = 0;
    At<uint32_t>(pose, 0x48) = 0;
    At<uint32_t>(pose, 0x2C) = count;
    At<int32_t>(pose, 0x28) = -1;
    At<uint8_t*>(pose, 0x38) = pose;
    return pose;
}

} // namespace

// 0x6C80F0: a lone jmp to the game's free (CNoTrackObject::operator delete)
RE_UNHOOKED("AM::Free_6C80F0", 0x006C80F0);

#define AMSET_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMSET_REVERSED("AM_BlendNode::Construct", 0x006BE840, &BlendNode_Construct);
AMSET_REVERSED("AM_Manager::Init", 0x006BF820, &Init);
AMSET_REVERSED("AM_Manager::TakeHierarchy", 0x006BF6C0, &TakeHierarchy);
AMSET_REVERSED("AM_Manager::TakeHxd", 0x006BF790, &TakeHxd);
AMSET_REVERSED("AM_Manager::ReleaseHierarchyLink", 0x006BF700, &ReleaseHierarchyLink);
AMSET_REVERSED("AM_Manager::ReleaseHxdLink", 0x006BF7E0, &ReleaseHxdLink);
AMSET_REVERSED("AM_Manager::ReleaseHierarchy", 0x006BF740, &ReleaseHierarchy);
AMSET_REVERSED("RV::LinkList::PushBack", 0x006BF670, &List_PushBack);
AMSET_REVERSED("AM_Manager::ClearModelsUpdated", 0x006C0A00, &ClearModelsUpdated);
AMSET_REVERSED("AM_Manager::UpdateGroupTimers", 0x006C0A30, &UpdateGroupTimers);
AMSET_REVERSED("AM_Codecs::Clear", 0x006C80A0, &ClearCodecs);
AMSET_REVERSED("AM_Codecs::Find", 0x006C80B0, &FindCodec);
AMSET_REVERSED("AM_Codecs::Add", 0x006C8100, &AddCodec);
AMSET_REVERSED("AM_Pose::New", 0x006C8150, &NewPose);
