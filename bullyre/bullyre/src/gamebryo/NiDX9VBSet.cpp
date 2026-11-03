// The dynamic vertex buffer sets of the DX9 renderer's buffer management (Gamebryo's NiVBSet /
// NiVBBlock, names guessed): a set (0x34 bytes, made by 0x8A5390) keeps vertex buffers of one
// kind in blocks, each a D3D vertex buffer whose free space is handed out; the blocks are kept in
// an array and in a list sorted by their free space. Blocks come from a static free list
// (0x20E7504, 0xBCB940 made at a time) and go back to it.
//
// A set: +0x4 the FVF, +0x8 its flags (0x10000000: blocks only as big as asked; 0x80000000:
// software processing), +0xC the block size, +0x10 the device (held), +0x14 the usage, +0x18
// the pool, +0x1C the blocks (NiTPrimitiveArray, vtable 0x961988: +0x20 the array, +0x26 its
// size), +0x2C the blocks sorted by free space (ascending).
//
// A block (0x44): +0x0 its set, +0x4 its index in the set's array, +0x8 the vertex buffer, +0xC
// its size, +0x10 / +0x14 / +0x18 / +0x1C the set's FVF, flags, usage and pool, +0x24 its free
// bytes, +0x28 its (largest?) free run, +0x2C its chips (an NiTPrimitiveArray: +0x30 the array,
// +0x34 / +0x36 / +0x38 / +0x3A its counts), +0x3C / +0x40 the next / previous in a list.
// NiTPrimitiveArray<T*>'s RemoveAt and AddFirstEmpty for 4-byte elements are here too.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c41 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;
constexpr uint32_t kBlockSize = 0x44;
constexpr int kDevCreateVertexBuffer = 26;

GAME_VAR(uint8_t*, ms_pkFreeBlocks, 0x020E7504);
GAME_VAR(uint32_t, ms_uiBlocksAtATime, 0x00BCB940);

GAME_FN(void, __thiscall, NiTArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, NiTArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, Block_UpdateFreeSpace, 0x008A7500, void* self, uint32_t a1);
GAME_FN(void, __cdecl, Block_ShutdownChips, 0x008A7630);
GAME_FN(void, __thiscall, Block_Destruct, 0x008A75A0, void* self);

// this file's (called where modes apply)
GAME_FN(void, __thiscall, Set_Unlink, 0x008A4AC0, void* self, uint8_t* block);
GAME_FN(void*, __thiscall, PrimitiveArray_RemoveAt, 0x008A4B10, void* array, uint32_t index);
GAME_FN(void, __thiscall, Set_InsertSorted, 0x008A4BB0, void* self, uint8_t* block);
GAME_FN(void, __thiscall, Block_Reset, 0x008A4C60, void* self);
GAME_FN(void, __thiscall, Set_ReleaseBlock, 0x008A4CC0, void* self, uint8_t* block);
GAME_FN(uint32_t, __thiscall, PrimitiveArray_AddFirstEmpty, 0x008A4D20, void* array, const re::Ptr32<void>* value);
GAME_FN(void, __cdecl, Block_CreateFreeBlocks, 0x008A4ED0);

// a block taken out of whatever list it's in and put at the head of the free list
inline void PushFree(uint8_t* block) {
    uint8_t* next = At<uint8_t*>(block, 0x3C);
    uint8_t* previous = At<uint8_t*>(block, 0x40);
    if (next != nullptr) {
        At<uint8_t*>(next, 0x40) = previous;
    }
    if (previous != nullptr) {
        At<uint8_t*>(previous, 0x3C) = next;
    }
    if (ms_pkFreeBlocks != nullptr) {
        At<uint8_t*>(ms_pkFreeBlocks, 0x40) = block;
    }
    At<uint8_t*>(block, 0x3C) = ms_pkFreeBlocks;
    At<uint8_t*>(block, 0x40) = nullptr;
    ms_pkFreeBlocks = block;
}

// a block's vertex buffer released, the block reset and made free (ReleaseBlock, inlined)
inline void ReleaseBlockInline(uint8_t* block) {
    if (At<void*>(block, 8) != nullptr) {
        Com<long>(At<void*>(block, 8), kRelease);
        At<void*>(block, 8) = nullptr;
    }
    Block_Reset(block);
    PushFree(block);
}

} // namespace

// 0x8A4A90: a set's usage and pool (write only and managed; software processing added for the
// flag 0x80000000). (Name a guess.)
void __cdecl Set_InitUsage(uint8_t* set) {
    At<uint32_t>(set, 0x14) = 8;
    At<uint32_t>(set, 0x18) = 1;
    if ((At<uint32_t>(set, 8) & 0x80000000) != 0) {
        At<uint32_t>(set, 0x14) = 0x18;
    }
}

// 0x8A4AC0 (thiscall): a block taken out of the set's sorted list (when it's in it)
void __fastcall Set_UnlinkImpl(uint8_t* self, re::Edx, uint8_t* block) {
    uint8_t* node = At<uint8_t*>(self, 0x2C);
    if (node == nullptr) {
        return;
    }
    while (node != block) {
        node = At<uint8_t*>(node, 0x3C);
        if (node == nullptr) {
            return;
        }
    }
    if (At<uint8_t*>(node, 0x3C) != nullptr) {
        At<uint8_t*>(At<uint8_t*>(node, 0x3C), 0x40) = At<uint8_t*>(node, 0x40);
    }
    if (At<uint8_t*>(node, 0x40) != nullptr) {
        At<uint8_t*>(At<uint8_t*>(node, 0x40), 0x3C) = At<uint8_t*>(node, 0x3C);
    }
    if (node == At<uint8_t*>(self, 0x2C)) {
        At<uint8_t*>(self, 0x2C) = At<uint8_t*>(node, 0x3C);
    }
}

// 0x8A4B10 (thiscall): NiTPrimitiveArray<T*>::RemoveAt (the element, its slot emptied, the size
// shrunk when it was the last)
void* __fastcall PrimitiveArray_RemoveAtImpl(uint8_t* self, re::Edx, uint32_t index) {
    if (index >= At<uint16_t>(self, 0xA)) {
        return nullptr;
    }
    re::Ptr32<void>* slot = &At<re::Ptr32<void>*>(self, 4)[index];
    void* element = *slot;
    *slot = nullptr;
    if (element != nullptr) {
        At<uint16_t>(self, 0xC)--;
    }
    uint16_t last = uint16_t(At<uint16_t>(self, 0xA) - 1);
    if (index == last) {
        At<uint16_t>(self, 0xA) = last;
    }
    return element;
}

// 0x8A4B60 (thiscall): the first block with at least `size` free bytes, taken out of the sorted
// list
uint8_t* __fastcall Set_FindBlock(uint8_t* self, re::Edx, uint32_t size) {
    uint8_t* block = At<uint8_t*>(self, 0x2C);
    for (;;) {
        if (block == nullptr) {
            return nullptr;
        }
        if (size <= At<uint32_t>(block, 0x24)) {
            break;
        }
        block = At<uint8_t*>(block, 0x3C);
    }
    if (At<uint8_t*>(block, 0x3C) != nullptr) {
        At<uint8_t*>(At<uint8_t*>(block, 0x3C), 0x40) = At<uint8_t*>(block, 0x40);
    }
    if (At<uint8_t*>(block, 0x40) != nullptr) {
        At<uint8_t*>(At<uint8_t*>(block, 0x40), 0x3C) = At<uint8_t*>(block, 0x3C);
    }
    if (block == At<uint8_t*>(self, 0x2C)) {
        At<uint8_t*>(self, 0x2C) = At<uint8_t*>(block, 0x3C);
    }
    return block;
}

// 0x8A4BB0 (thiscall): a block put in the sorted list (after those with as much free space or
// less)
void __fastcall Set_InsertSortedImpl(uint8_t* self, re::Edx, uint8_t* block) {
    if (At<uint8_t*>(self, 0x2C) == nullptr) {
        At<uint8_t*>(self, 0x2C) = block;
        At<uint8_t*>(block, 0x3C) = nullptr;
        At<uint8_t*>(At<uint8_t*>(self, 0x2C), 0x40) = nullptr;
        return;
    }
    uint8_t* node = At<uint8_t*>(self, 0x2C);
    uint8_t* previous = nullptr;
    while (At<uint32_t>(node, 0x24) <= At<uint32_t>(block, 0x24)) {
        previous = node;
        if (At<uint8_t*>(node, 0x3C) == nullptr) {
            At<uint8_t*>(node, 0x3C) = block;
            At<uint8_t*>(block, 0x40) = node;
            At<uint8_t*>(block, 0x3C) = nullptr;
            return;
        }
        node = At<uint8_t*>(node, 0x3C);
    }
    if (previous != nullptr) {
        At<uint8_t*>(previous, 0x3C) = block;
    }
    At<uint8_t*>(node, 0x40) = block;
    At<uint8_t*>(block, 0x3C) = node;
    At<uint8_t*>(block, 0x40) = previous;
    if (node == At<uint8_t*>(self, 0x2C)) {
        At<uint8_t*>(self, 0x2C) = block;
    }
}

// 0x8A4C20: the free blocks deleted (the chips' shutdown first). (Name a guess.)
void __cdecl Block_ShutdownFreeBlocks() {
    Block_ShutdownChips();
    uint8_t* block = ms_pkFreeBlocks;
    while (block != nullptr) {
        uint8_t* next = At<uint8_t*>(block, 0x3C);
        Block_Destruct(block);
        NiDelete(block, kBlockSize);
        block = next;
    }
    ms_pkFreeBlocks = nullptr;
}

// 0x8A4C60 (thiscall): a block's fields cleared and its chips array emptied
void __fastcall Block_ResetImpl(uint8_t* self, re::Edx) {
    for (int offset = 0; offset <= 0x28; offset += 4) {
        At<uint32_t>(self, offset) = 0;
    }
    for (uint16_t i = 0; i < At<uint16_t>(self, 0x36); i++) {
        At<re::Ptr32<void>*>(self, 0x30)[i] = nullptr;
    }
    At<uint16_t>(self, 0x36) = 0;
    At<uint16_t>(self, 0x38) = 0;
    At<uint8_t*>(self, 0x3C) = nullptr;
    At<uint8_t*>(self, 0x40) = nullptr;
}

// 0x8A4CC0 (thiscall, `this` unused): a block released (its vertex buffer released, reset, made
// free)
void __fastcall Set_ReleaseBlockImpl(void*, re::Edx, uint8_t* block) {
    if (block != nullptr) {
        ReleaseBlockInline(block);
    }
}

// 0x8A4D20 (thiscall): NiTPrimitiveArray<T*>::AddFirstEmpty (the first empty slot, else at the
// end, grown as needed; the index; -1 for null)
uint32_t __fastcall PrimitiveArray_AddFirstEmptyImpl(uint8_t* self, re::Edx, const re::Ptr32<void>* value) {
    if (*value == nullptr) {
        return 0xFFFFFFFF;
    }
    for (uint16_t i = 0; i < At<uint16_t>(self, 0xA); i++) {
        if (At<re::Ptr32<void>*>(self, 4)[i] == nullptr) {
            At<re::Ptr32<void>*>(self, 4)[i] = *value;
            At<uint16_t>(self, 0xC)++;
            return i;
        }
    }
    uint32_t index = At<uint16_t>(self, 0xA);
    if (index >= At<uint16_t>(self, 8)) {
        NiTArray_SetSize(self, At<uint16_t>(self, 0xE) + index);
    }
    NiTArray_SetAt(self, index, value);
    return index;
}

// 0x8A4DA0 (thiscall): a set's blocks all released, its device let go, its array freed (its
// destructor)
void __fastcall Set_Destruct(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x26); i++) {
        uint8_t* block = nullptr;
        if (i < At<uint16_t>(self, 0x26)) {
            re::Ptr32<void>* slot = &At<re::Ptr32<void>*>(self, 0x20)[i];
            block = static_cast<uint8_t*>(*slot);
            *slot = nullptr;
            if (block != nullptr) {
                At<uint16_t>(self, 0x28)--;
            }
            uint16_t last = uint16_t(At<uint16_t>(self, 0x26) - 1);
            if (i == last) {
                At<uint16_t>(self, 0x26) = last;
            }
        }
        if (block != nullptr) {
            ReleaseBlockInline(block);
        }
    }
    if (At<void*>(self, 0x10) != nullptr) {
        Com<long>(At<void*>(self, 0x10), kRelease);
    }
    At<void*>(self, 0x10) = nullptr;
    At<uint32_t>(self, 0x1C) = kPrimitiveArrayVtable;
    NiFree(At<void*>(self, 0x20));
}

// 0x8A4E70 (thiscall): a block's free space updated (0x8A7500); when it changed the block taken
// out of the sorted list and either released (all of it free) or put back in order
void __fastcall Set_UpdateBlock(uint8_t* self, re::Edx, uint32_t index, uint32_t a2) {
    uint8_t* block = At<re::Ptr32<uint8_t>*>(self, 0x20)[index];
    if (block == nullptr) {
        return;
    }
    uint32_t free = At<uint32_t>(block, 0x24);
    Block_UpdateFreeSpace(block, a2);
    if (free == At<uint32_t>(block, 0x24)) {
        return;
    }
    Set_Unlink(self, block);
    if (At<uint32_t>(block, 0x28) == At<uint32_t>(block, 0xC)) {
        PrimitiveArray_RemoveAt(self + 0x1C, index);
        Set_ReleaseBlock(self, block);
        return;
    }
    Set_InsertSorted(self, block);
}

// 0x8A4ED0: the free block list topped up (0xBCB940 blocks made, constructed, put at its head)
void __cdecl Block_CreateFreeBlocksImpl() {
    for (uint32_t i = 0; i < ms_uiBlocksAtATime; i++) {
        uint8_t* block = static_cast<uint8_t*>(NiNew(kBlockSize));
        if (block != nullptr) {
            At<uint16_t>(block, 0x34) = 0;
            At<uint16_t>(block, 0x3A) = 1;
            At<uint16_t>(block, 0x36) = 0;
            At<uint16_t>(block, 0x38) = 0;
            At<void*>(block, 0x30) = nullptr;
            At<uint32_t>(block, 0x2C) = kPrimitiveArrayVtable;
            Block_Reset(block);
        }
        PushFree(block);
    }
}

// 0x8A4F60 (thiscall): a new block of `size` bytes (the set's block size at least, unless the set
// says otherwise): its vertex buffer made, a block from the free list (topped up when empty)
// filled from the set and added to the set's array; null when the vertex buffer can't be made
uint8_t* __fastcall Set_CreateBlock(uint8_t* self, re::Edx, uint32_t size) {
    if ((At<uint32_t>(self, 8) & 0x10000000) == 0 && size <= At<uint32_t>(self, 0xC)) {
        size = At<uint32_t>(self, 0xC);
    }
    void* buffer = nullptr;
    if (Com<long>(At<void*>(self, 0x10), kDevCreateVertexBuffer, size, At<uint32_t>(self, 0x14),
                  At<uint32_t>(self, 4), At<uint32_t>(self, 0x18), &buffer, static_cast<void*>(nullptr)) < 0) {
        return nullptr;
    }
    uint8_t* block = ms_pkFreeBlocks;
    if (block == nullptr) {
        Block_CreateFreeBlocks();
        block = ms_pkFreeBlocks;
    }
    ms_pkFreeBlocks = At<uint8_t*>(block, 0x3C);
    if (ms_pkFreeBlocks != nullptr) {
        At<uint8_t*>(ms_pkFreeBlocks, 0x40) = nullptr;
    }
    At<uint8_t*>(block, 0x3C) = nullptr;
    At<uint8_t*>(block, 0x40) = nullptr;
    At<uint8_t*>(block, 0) = self;
    At<void*>(block, 8) = buffer;
    At<uint32_t>(block, 0xC) = size;
    At<uint32_t>(block, 0x24) = size;
    At<uint32_t>(block, 0x28) = size;
    At<uint32_t>(block, 0x10) = At<uint32_t>(self, 4);
    At<uint32_t>(block, 0x14) = At<uint32_t>(self, 8);
    At<uint32_t>(block, 0x1C) = At<uint32_t>(self, 0x18);
    At<uint32_t>(block, 0x18) = At<uint32_t>(self, 0x14);
    re::Ptr32<void> held = block;
    At<uint32_t>(block, 4) = PrimitiveArray_AddFirstEmpty(self + 0x1C, &held);
    return block;
}

#define C41I_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C41I_REVERSED("NiVBSet::InitUsage", 0x008A4A90, Set_InitUsage);
C41I_REVERSED("NiVBSet::Unlink", 0x008A4AC0, Set_UnlinkImpl);
C41I_REVERSED("NiTPrimitiveArray<void*>::RemoveAt", 0x008A4B10, PrimitiveArray_RemoveAtImpl);
C41I_REVERSED("NiVBSet::FindBlock", 0x008A4B60, Set_FindBlock);
C41I_REVERSED("NiVBSet::InsertSorted", 0x008A4BB0, Set_InsertSortedImpl);
C41I_REVERSED("NiVBBlock::ShutdownFreeBlocks", 0x008A4C20, Block_ShutdownFreeBlocks);
C41I_REVERSED("NiVBBlock::Reset", 0x008A4C60, Block_ResetImpl);
C41I_REVERSED("NiVBSet::ReleaseBlock", 0x008A4CC0, Set_ReleaseBlockImpl);
C41I_REVERSED("NiTPrimitiveArray<void*>::AddFirstEmpty", 0x008A4D20, PrimitiveArray_AddFirstEmptyImpl);
C41I_REVERSED("NiVBSet::~NiVBSet", 0x008A4DA0, Set_Destruct);
C41I_REVERSED("NiVBSet::UpdateBlock", 0x008A4E70, Set_UpdateBlock);
C41I_REVERSED("NiVBBlock::CreateFreeBlocks", 0x008A4ED0, Block_CreateFreeBlocksImpl);
C41I_REVERSED("NiVBSet::CreateBlock", 0x008A4F60, Set_CreateBlock);

} // namespace ni::c41
