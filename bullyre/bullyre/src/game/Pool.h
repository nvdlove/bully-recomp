#pragma once

// CPool: the game's fixed-size object pools (GTA's, with the element size stored).

#include <cstdint>
#include "reversed.h"

// A flag byte per slot: 0x80 free, the low bits count how often the slot was reused.
template <typename T>
struct CPool {
    re::Ptr32<T> objects;         // +0
    re::Ptr32<uint8_t> flags;     // +4
    int32_t size;       // +8
    int32_t elemSize;   // +0xC
    int32_t unknown10;  // +0x10
    int32_t lastUsed;   // +0x14 where the search for a free slot starts
    uint8_t ownsArrays; // +0x18 (frees them when flushed)
    uint8_t unknown19;  // +0x19

    bool IsFreeSlot(int slot) const { return (flags[slot] & 0x80) != 0; }
    // (nullptr for a free slot: the game's callers then read through it, which
    // never happens for the slots they're given)
    T* GetAt(int slot) {
        return IsFreeSlot(slot) ? nullptr : re::PtrCast<T*>(re::PtrCast<uint8_t*>(objects) + elemSize * slot);
    }
    int GetIndex(const T* object) const {
        return int((reinterpret_cast<const uint8_t*>(object) - re::PtrCast<const uint8_t*>(objects)) / elemSize);
    }
    // (inlined in the game): the slot back to the free ones
    void Delete(T* object) {
        int slot = GetIndex(object);
        flags[slot] |= 0x80;
        if (slot < lastUsed) {
            lastUsed = slot;
        }
    }

    // unknown10 is the next pool of a chain: slots past `size` go on there (the
    // object pool continues into the projectile and cutscene pools; see Pools.cpp)
    re::Ptr32<CPool<uint8_t>>& Next() { return *reinterpret_cast<re::Ptr32<CPool<uint8_t>>*>(&unknown10); }
    // unknown19: the objects are an array made with new[] (a count before them,
    // constructed and destroyed as objects). CPools never sets it
    bool IsConstructedArray() const { return unknown19 != 0; }
    int GetNoOfUsedSpaces() const {
        int used = 0;
        for (int slot = 0; slot < size; slot++) {
            if (!IsFreeSlot(slot)) {
                used++;
            }
        }
        return used;
    }
};
static_assert(sizeof(CPool<int>) == 0x1C);
