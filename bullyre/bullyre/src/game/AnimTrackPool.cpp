// AM_AnimTrack's pool: the tracks live in two RV lists (the free one at 0xBB615C, the
// used one at 0xBB6130; nodes: +0 the track, +4 prev, +8 next), handed out reset and
// taken back. Also the track's reset, its vtable's clone and apply slots.
// 0x6B8F30..0x6B9094, 0x6B9380, 0x6B99D0, 0x6B9A80.

#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

// the pools' RV lists (+4 head, +8 tail, +0x18 count)
inline uint8_t* const kUsedList = reinterpret_cast<uint8_t*>(0x00BB6130);
inline uint8_t* const kFreeList = reinterpret_cast<uint8_t*>(0x00BB615C);

GAME_FN(int, __thiscall, Hxd_ReleaseGroup, 0x006B3DA0, uint8_t* hxd, uint32_t group, int keep);
GAME_FN(void, __thiscall, List_InsertRange, 0x006B2C20, uint8_t* list, uint8_t* before, uint8_t* first, uint8_t* last);
GAME_FN(void, __thiscall, Track_Reset, 0x006B8F30, uint8_t* track);
GAME_FN(uint8_t*, __cdecl, Track_Allocate, 0x006B8FB0);
GAME_FN(uint8_t*, __thiscall, Track_CopyFrom, 0x006B4670, uint8_t* track, uint8_t* other);
GAME_FN(uint8_t, __thiscall, Track_ApplyPlayer, 0x006B6FB0, uint8_t* track, uint8_t* model, uint8_t* starts,
        void* travel, float* rootRotation, int player);

inline void ReleasePlayerAnim(uint8_t* self, int base) {
    uint8_t* anim = At<uint8_t*>(self, base);
    if (anim && At<uint8_t*>(anim, 8)) {
        Hxd_ReleaseGroup(At<uint8_t*>(anim, 8), At<uint32_t>(anim, 0x20), 0);
    }
}

// 0x6B8F30: back to the constructor's state (the players' anims released)
void __fastcall Reset(uint8_t* self, re::Edx) {
    At<float>(self, 0xC) = 1.0f;
    At<float>(self, 0x10) = 0.0f;
    At<float>(self, 0x14) = 0.0f;
    At<uint32_t>(self, 8) = 0;
    At<float>(self, 0x40) = 0.0f;
    At<uint32_t>(self, 0x44) = 0;
    ReleasePlayerAnim(self, 0x18);
    At<float>(self, 0x1C) = 0.0f;
    At<uint32_t>(self, 0x18) = 0;
    At<float>(self, 0x20) = 0.0f;
    At<uint32_t>(self, 0x28) = 0;
    At<float>(self, 0x24) = 1.0f;
    ReleasePlayerAnim(self, 0x2C);
    At<float>(self, 0x30) = 0.0f;
    At<uint32_t>(self, 0x2C) = 0;
    At<float>(self, 0x34) = 0.0f;
    At<uint32_t>(self, 0x3C) = 0;
    At<float>(self, 0x38) = 1.0f;
}

// 0x6B8FB0: a track from the free list (reset), moved onto the used list; null when none
uint8_t* __cdecl Allocate() {
    uint8_t* node = At<uint8_t*>(kFreeList, 4);
    if (node == nullptr) {
        return nullptr;
    }
    Track_Reset(At<uint8_t*>(node, 0));
    if (node == At<uint8_t*>(kFreeList, 4)) {
        At<uint8_t*>(kFreeList, 4) = At<uint8_t*>(node, 8);
    } else {
        At<uint8_t*>(At<uint8_t*>(node, 4), 8) = At<uint8_t*>(node, 8);
    }
    if (node == At<uint8_t*>(kFreeList, 8)) {
        At<uint8_t*>(kFreeList, 8) = At<uint8_t*>(node, 4);
    } else {
        At<uint8_t*>(At<uint8_t*>(node, 8), 4) = At<uint8_t*>(node, 4);
    }
    At<int32_t>(kFreeList, 0x18) -= 1;
    List_InsertRange(kUsedList, nullptr, node, node);
    At<int32_t>(kUsedList, 0x18) += 1;
    return At<uint8_t*>(node, 0);
}

// 0x6B9020: vtable slot 1: back to the free list (the used list's last node when the
// track isn't on it)
void __fastcall Release(uint8_t* self, re::Edx) {
    Track_Reset(self);
    uint8_t* head = At<uint8_t*>(kUsedList, 4);
    uint8_t* node = head;
    while (node && At<uint8_t*>(node, 0) != self) {
        node = At<uint8_t*>(node, 8);
    }
    if (node == nullptr) {
        node = At<uint8_t*>(kUsedList, 8);
    }
    if (node == head) {
        At<uint8_t*>(kUsedList, 4) = At<uint8_t*>(node, 8);
    } else {
        At<uint8_t*>(At<uint8_t*>(node, 4), 8) = At<uint8_t*>(node, 8);
    }
    if (node == At<uint8_t*>(kUsedList, 8)) {
        At<uint8_t*>(kUsedList, 8) = At<uint8_t*>(node, 4);
    } else {
        At<uint8_t*>(At<uint8_t*>(node, 8), 4) = At<uint8_t*>(node, 4);
    }
    At<int32_t>(kUsedList, 0x18) -= 1;
    List_InsertRange(kFreeList, nullptr, node, node);
    At<int32_t>(kFreeList, 0x18) += 1;
}

// 0x6B9380: a track for slot 0..6 (+0x44) (ret 4; ecx unused)
uint8_t* __fastcall AllocateForSlot(void*, re::Edx, uint32_t slot) {
    if (slot > 6) {
        return nullptr;
    }
    uint8_t* track = Track_Allocate();
    At<uint32_t>(track, 0x44) = slot;
    return track;
}

// 0x6B99D0: vtable slot 4: applies the first player
uint8_t __fastcall ApplyFirst(uint8_t* self, re::Edx, uint8_t* model, uint8_t* starts, void* travel,
                              float* rootRotation) {
    return Track_ApplyPlayer(self, model, starts, travel, rootRotation, 0);
}

// 0x6B9A80: vtable slot 2: a copy from the pool
uint8_t* __fastcall Clone(uint8_t* self, re::Edx) {
    uint8_t* copy = Track_Allocate();
    Track_CopyFrom(copy, self);
    return copy;
}


// the pool's scratch poses and tracks
GAME_VAR(uint8_t*, pool_scratchA, 0x020C48A0);
GAME_VAR(uint8_t*, pool_scratchB, 0x020C48A4);
GAME_VAR(uint8_t*, pool_tracks, 0x020C4894);  // 300 tracks of 0x48 (a count before them)
GAME_FN(uint8_t*, __cdecl, NewPose, 0x006C8150, uint32_t count, uint32_t stride);
GAME_FN(void, __cdecl, DeletePose, 0x006C80F0, uint8_t* pose);
GAME_FN(void, __thiscall, LinkPool_Init, 0x005EF750, uint8_t* pool, uint32_t count);
GAME_FN(void, __thiscall, LinkPool_Release, 0x005EF710, uint8_t* pool);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
inline void* const kTrackCtor = reinterpret_cast<void*>(0x006B45F0);

// 0x6B9610: makes the pool: the used list owns 300 links, the free list borrows them,
// 300 tracks made, reset and put on the free list
void __cdecl Pool_Init() {
    if (pool_scratchA == nullptr) {
        pool_scratchA = NewPose(0x40, 0x24);
        pool_scratchB = NewPose(0x40, 0x24);
    }
    LinkPool_Init(kUsedList, 300);
    At<int32_t>(kUsedList, 0x28) += 1;
    At<int32_t>(kFreeList, 0x14) = At<int32_t>(kUsedList, 0x14);
    At<uint8_t*>(kFreeList, 0xC) = nullptr;
    At<uint8_t*>(kFreeList, 0x10) = nullptr;
    At<uint8_t*>(kFreeList, 0x24) = kUsedList;
    At<int32_t>(kFreeList, 0x1C) = 0;
    int32_t* block = static_cast<int32_t*>(Malloc(0x5464));
    if (block) {
        block[0] = 300;
        VectorConstruct(block + 1, 0x48, 300, kTrackCtor);
        pool_tracks = reinterpret_cast<uint8_t*>(block + 1);
    } else {
        pool_tracks = nullptr;
    }
    for (int offset = 0; offset < 300 * 0x48; offset += 0x48) {
        Track_Reset(pool_tracks + offset);
    }
    for (int offset = 0; offset < 300 * 0x48; offset += 0x48) {
        // a link from the owner's free links onto the free list's tail
        uint8_t* owner = At<uint8_t*>(kFreeList, 0x24);
        uint8_t* link = At<uint8_t*>(owner, 0x10);
        At<uint8_t*>(owner, 0x10) = At<uint8_t*>(link, 8);
        At<int32_t>(kFreeList, 0x18) += 1;
        At<uint8_t*>(link, 0) = pool_tracks + offset;
        At<uint8_t*>(link, 4) = At<uint8_t*>(kFreeList, 8);
        At<uint8_t*>(link, 8) = nullptr;
        uint8_t* tail = At<uint8_t*>(kFreeList, 8);
        if (At<uint8_t*>(kFreeList, 4) == tail && tail == nullptr) {
            At<uint8_t*>(kFreeList, 4) = link;
        } else {
            At<uint8_t*>(tail, 8) = link;
        }
        At<uint8_t*>(kFreeList, 8) = link;
    }
}

// 0x6B98D0: tears the pool down
void __cdecl Pool_Shutdown() {
    if (pool_scratchA) {
        DeletePose(pool_scratchA);
    }
    if (pool_scratchB) {
        DeletePose(pool_scratchB);
    }
    uint8_t* tail = At<uint8_t*>(kFreeList, 8);
    if (tail) {
        At<uint8_t*>(tail, 8) = At<uint8_t*>(At<uint8_t*>(kFreeList, 0x24), 0x10);
        At<uint8_t*>(At<uint8_t*>(kFreeList, 0x24), 0x10) = At<uint8_t*>(kFreeList, 4);
    }
    uint8_t* owner = At<uint8_t*>(kFreeList, 0x24);
    At<int32_t>(kFreeList, 0x18) = 0;
    At<uint8_t*>(kFreeList, 4) = nullptr;
    At<uint8_t*>(kFreeList, 8) = nullptr;
    At<int32_t>(owner, 0x28) -= 1;
    At<int32_t>(kFreeList, 0x14) = 0;
    At<uint8_t*>(kFreeList, 0xC) = nullptr;
    At<uint8_t*>(kFreeList, 0x10) = nullptr;
    At<uint8_t*>(kFreeList, 0x24) = nullptr;
    At<int32_t>(kFreeList, 0x18) = 0;
    if (At<int32_t>(kUsedList, 0x1C) != 0) {
        if (At<int32_t>(kUsedList, 0x20) == 0) {
            Free(At<void*>(kUsedList, 0xC));
        }
        At<int32_t>(kUsedList, 0x14) = 0;
        At<void*>(kUsedList, 0xC) = nullptr;
        At<void*>(kUsedList, 0x10) = nullptr;
        At<void*>(kUsedList, 0x24) = nullptr;
        At<int32_t>(kUsedList, 0x18) = 0;
    } else if (At<void*>(kUsedList, 0x24) != nullptr) {
        LinkPool_Release(kUsedList);
    }
    if (pool_tracks) {
        Free(pool_tracks - 4);
    }
    pool_tracks = nullptr;
}

} // namespace

#define AMPOOL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMPOOL_REVERSED("AM_AnimTrack::Reset", 0x006B8F30, &Reset);
AMPOOL_REVERSED("AM_AnimTrack::Allocate", 0x006B8FB0, &Allocate);
AMPOOL_REVERSED("AM_AnimTrack::Release", 0x006B9020, &Release);
AMPOOL_REVERSED("AM_AnimTrack::AllocateForSlot", 0x006B9380, &AllocateForSlot);
AMPOOL_REVERSED("AM_AnimTrackPool::Init", 0x006B9610, &Pool_Init);
AMPOOL_REVERSED("AM_AnimTrackPool::Shutdown", 0x006B98D0, &Pool_Shutdown);
AMPOOL_REVERSED("AM_AnimTrack::ApplyFirst", 0x006B99D0, &ApplyFirst);
AMPOOL_REVERSED("AM_AnimTrack::Clone", 0x006B9A80, &Clone);
