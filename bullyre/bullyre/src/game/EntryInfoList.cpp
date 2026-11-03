// CEntryInfoList::Flush: a physical's (or dummy's) sector entries let go. 0x429840.

#include <cstdint>

#include "game/Pools.h"
#include "game/World.h"
#include "reversed.h"

#define ENTRYINFO_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace EntryInfoList {

// 0x429840 (thiscall): every node unlinked and back to its pool (GTA's
// CEntryInfoList::Flush; the nodes' sector list entries are left to the caller)
void __fastcall Flush(CEntryInfoList* self, re::Edx) {
    CEntryInfoNode* node = self->first;
    while (node) {
        CEntryInfoNode* next = node->next;
        if (self->first == node) {
            self->first = next;
        }
        if (node->prev) {
            node->prev->next = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        }
        CPool<CEntryInfoNode>* pool = CPools::ms_pEntryInfoNodePool;
        int slot = int((reinterpret_cast<intptr_t>(node) - re::PtrCast<intptr_t>(pool->objects)) / pool->elemSize);
        pool->flags[slot] |= 0x80;
        if (slot < pool->lastUsed) {
            pool->lastUsed = slot;
        }
        node = next;
    }
}

ENTRYINFO_REVERSED("CEntryInfoList::Flush", 0x00429840, &Flush);

} // namespace EntryInfoList
