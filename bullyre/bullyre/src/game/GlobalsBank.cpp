// The globals bank (0x727DC0..0x728293): the action tree's /Global/GlobalsBank node (its
// .act file loaded when the tree lacks it) mirrored as a tree of entries, each with its
// node's name, its children's entries and its Butes tracks by key (the tuning sets the
// game reads, ButesDefaults.cpp). Built once (0x727F10, from 0x72BE85); the arrays are
// counted blocks (the count before the first element, MSVC's new[]).

#include <cstdint>

#include "game/ActionTree.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using vc::At;

struct BankTrack;

// a node's entry (0x14)
struct BankEntry {
    uint32_t name;        // +0x0 a name key
    uint32_t childCount;  // +0x4
    re::Ptr32<BankEntry> children;  // +0x8 (a counted block)
    uint32_t trackCount;  // +0xC
    re::Ptr32<BankTrack> tracks;    // +0x10 (a counted block)
};
static_assert(sizeof(BankEntry) == 0x14);
// a track by its key (8)
struct BankTrack {
    uint32_t key;    // +0x0 a name key
    re::Ptr32<uint8_t> track;  // +0x4 a Butes (owned)
};
static_assert(sizeof(BankTrack) == 8);

GAME_FN(void*, __cdecl, GB_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, GB_Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, GB_NameKey_FromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(uint32_t*, __thiscall, GB_NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, GB_NameKey_Destruct, 0x0049A650, uint32_t* key);
GAME_FN(uint32_t*, __thiscall, GB_TrackDesc_GetKey, 0x005FA9B0, void* desc, uint32_t* out);
GAME_FN(uint8_t*, __cdecl, GB_ActionTree_Find, 0x005F5B10, const char* path, uint8_t* from);
GAME_FN(uint8_t*, __cdecl, GB_ActFile_Load, 0x005F6DB0, const char* path, uint8_t* parent, void* names);
GAME_FN(void, __stdcall, GB_ConstructArray, 0x00401240, void* array, uint32_t size, int32_t count, void* ctor);

// this file's, at the game's addresses (so modes apply)
GAME_FN(void, __cdecl, GB_BuildEntry, 0x00727DE0, uint8_t* node, BankEntry* entry);
GAME_FN(BankEntry*, __thiscall, GB_Child, 0x00727FF0, BankEntry* entry, int32_t i);
GAME_FN(void, __thiscall, GB_SetTrack, 0x00728000, BankEntry* entry, int32_t i, const uint32_t* key, uint8_t* track);
GAME_FN(void, __thiscall, GB_SetTrackCount, 0x00728080, BankEntry* entry, uint32_t count);
GAME_FN(void, __thiscall, GB_Destruct, 0x00728130, BankEntry* entry);
GAME_FN(void*, __thiscall, GB_VectorDeletingDestruct, 0x007281A0, BankEntry* entry, int flags);
GAME_FN(void, __thiscall, GB_SetChildCount, 0x00728200, BankEntry* entry, uint32_t count);
inline void* const gbEntryConstruct = reinterpret_cast<void*>(0x00727DC0);
inline void* const gbTrackConstruct = reinterpret_cast<void*>(0x00727F90);

// the Butes tracks' type id (its guard and id)
inline uint16_t ButesTypeId() {
    return uint16_t(acttree::TypeIdAt<0x00C0F5C4, 0x00C0F5C0>());
}

// a counted block's size for count elements (saturating: new[]'s overflow check)
inline uint32_t BlockBytes(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    uint32_t total = (bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes);
    return total > 0xFFFFFFFBu ? 0xFFFFFFFFu : total + 4;
}

// the tracks' block released: each track deleted and its key destructed, last first
inline void FreeTracks(BankEntry* entry) {
    BankTrack* tracks = entry->tracks;
    if (tracks) {
        uint32_t* block = reinterpret_cast<uint32_t*>(tracks) - 1;
        for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
            if (tracks[i].track) {
                vc::Call<void*>(tracks[i].track, acttree::kTrackDestruct, 1);
            }
            GB_NameKey_Destruct(&tracks[i].key);
        }
        GB_Free(block);
    }
}

#define GLOBALSBANK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

} // namespace

// 0x727DC0 (thiscall): an entry, unnamed (-1) and empty (new[]'s element constructor)
BankEntry* __fastcall BankEntry_Construct(BankEntry* self, re::Edx) {
    GB_NameKey_FromValue(&self->name, 0xFFFFFFFFu);
    self->childCount = 0;
    self->children = nullptr;
    self->trackCount = 0;
    self->tracks = nullptr;
    return self;
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::Construct", 0x00727DC0, &BankEntry_Construct);

// 0x727DE0 (cdecl): an entry from its node: the name, the children's entries (recursively)
// and the node's tracks that are Butes (the others deleted)
void __cdecl BankEntry_Build(uint8_t* node, BankEntry* entry) {
    uint32_t name;
    GB_NameKey_Copy(&name, vc::Call<const uint32_t*>(node, acttree::kNodeGetName));
    entry->name = name;
    int32_t children = vc::Call<int32_t>(node, acttree::kNodeNumChildren);
    GB_SetChildCount(entry, uint32_t(children));
    for (int32_t i = 0; i < children; i++) {
        uint8_t* child = vc::Call<uint8_t*>(node, acttree::kNodeGetChild, i);
        GB_BuildEntry(child, GB_Child(entry, i));
    }
    int32_t tracks = vc::Call<int32_t>(node, acttree::kNodeNumTracks);
    GB_SetTrackCount(entry, uint32_t(tracks));
    for (int32_t i = 0; i < tracks; i++) {
        uint32_t key;
        GB_TrackDesc_GetKey(vc::Call<void*>(node, acttree::kNodeGetTrackKey, i), &key);
        uint8_t* track = vc::Call<uint8_t*>(node, acttree::kNodeCreateTrack, i);
        if (track) {
            if (vc::Call<uint8_t>(track, acttree::kTrackIsA, int(ButesTypeId()))) {
                GB_SetTrack(entry, i, &key, track);
            } else {
                vc::Call<void*>(track, acttree::kTrackDestruct, 1);
            }
        }
        GB_NameKey_Destruct(&key);
    }
    GB_NameKey_Destruct(&name);
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::Build", 0x00727DE0, &BankEntry_Build);

// 0x727F10 (cdecl): the bank built from /Global/GlobalsBank (the file at `path` loaded
// when the tree lacks it). (A failed allocation builds into null, as the game does.)
BankEntry* __cdecl GlobalsBank_Load(const char* path) {
    uint8_t* node = GB_ActionTree_Find("/Global/GlobalsBank", nullptr);
    if (!node) {
        node = GB_ActFile_Load(path, nullptr, nullptr);
    }
    BankEntry* entry = static_cast<BankEntry*>(GB_Malloc(sizeof(BankEntry)));
    if (entry) {
        GB_NameKey_FromValue(&entry->name, 0xFFFFFFFFu);
        entry->childCount = 0;
        entry->children = nullptr;
        entry->trackCount = 0;
        entry->tracks = nullptr;
        GB_BuildEntry(node, entry);
        return entry;
    }
    GB_BuildEntry(node, nullptr);
    return nullptr;
}
GLOBALSBANK_REVERSED("GlobalsBank::Load", 0x00727F10, &GlobalsBank_Load);

// 0x727F90 (thiscall): a track slot, unnamed (-1) and empty (new[]'s element constructor)
BankTrack* __fastcall BankTrack_Construct(BankTrack* self, re::Edx) {
    GB_NameKey_FromValue(&self->key, 0xFFFFFFFFu);
    self->track = nullptr;
    return self;
}
GLOBALSBANK_REVERSED("GlobalsBank::Track::Construct", 0x00727F90, &BankTrack_Construct);

// 0x727FB0 (thiscall): the child entry with this name, or null
BankEntry* __fastcall BankEntry_FindChild(BankEntry* self, re::Edx, const uint32_t* name) {
    for (uint32_t i = 0; i < self->childCount; i++) {
        if (*name == self->children[i].name) {
            return &self->children[i];
        }
    }
    return nullptr;
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::FindChild", 0x00727FB0, &BankEntry_FindChild);

// 0x727FF0 (thiscall): child entry i
BankEntry* __fastcall BankEntry_Child(BankEntry* self, re::Edx, int32_t i) {
    return &self->children[i];
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::Child", 0x00727FF0, &BankEntry_Child);

// 0x728000 (thiscall): track slot i set (the key copied raw)
void __fastcall BankEntry_SetTrack(BankEntry* self, re::Edx, int32_t i, const uint32_t* key, uint8_t* track) {
    BankTrack* slot = &self->tracks[i];
    slot->key = *key;
    slot->track = track;
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::SetTrack", 0x00728000, &BankEntry_SetTrack);

// 0x728020 (thiscall): the track with this key, or null
uint8_t* __fastcall BankEntry_FindTrack(BankEntry* self, re::Edx, const uint32_t* key) {
    for (uint32_t i = 0; i < self->trackCount; i++) {
        if (*key == self->tracks[i].key) {
            return self->tracks[i].track;
        }
    }
    return nullptr;
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::FindTrack", 0x00728020, &BankEntry_FindTrack);

// 0x728060 (thiscall): track slot i
BankTrack* __fastcall BankEntry_TrackSlot(BankEntry* self, re::Edx, int32_t i) {
    return &self->tracks[i];
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::TrackSlot", 0x00728060, &BankEntry_TrackSlot);

// 0x728070 (thiscall): track i
uint8_t* __fastcall BankEntry_Track(BankEntry* self, re::Edx, int32_t i) {
    return self->tracks[i].track;
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::Track", 0x00728070, &BankEntry_Track);

// 0x728080 (thiscall): the tracks replaced by `count` empty slots (none for 0: the old
// block's pointer then stays, freed, as the game leaves it)
void __fastcall BankEntry_SetTrackCount(BankEntry* self, re::Edx, uint32_t count) {
    FreeTracks(self);
    self->trackCount = count;
    if (count != 0) {
        uint32_t* block = static_cast<uint32_t*>(GB_Malloc(BlockBytes(count, sizeof(BankTrack))));
        if (block) {
            block[0] = count;
            GB_ConstructArray(block + 1, sizeof(BankTrack), int32_t(count), gbTrackConstruct);
            self->tracks = reinterpret_cast<BankTrack*>(block + 1);
            return;
        }
        self->tracks = nullptr;
    }
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::SetTrackCount", 0x00728080, &BankEntry_SetTrackCount);

// 0x728130 (thiscall): an entry's destructor: its children (their block deleted), its
// tracks, then its name
void __fastcall BankEntry_Destruct(BankEntry* self, re::Edx) {
    if (self->children) {
        GB_VectorDeletingDestruct(self->children, 3);
    }
    FreeTracks(self);
    GB_NameKey_Destruct(&self->name);
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::Destruct", 0x00728130, &BankEntry_Destruct);

// 0x7281A0 (thiscall): the vector deleting destructor (flags 2: a counted block, last
// first; 1: freed). Returns what was (or would be) freed.
void* __fastcall BankEntry_VectorDeletingDestruct(BankEntry* self, re::Edx, int flags) {
    uint8_t f = uint8_t(flags);
    if (f & 2) {
        uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
        for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
            GB_Destruct(&self[i]);
        }
        if (f & 1) {
            GB_Free(block);
        }
        return block;
    }
    GB_Destruct(self);
    if (f & 1) {
        GB_Free(self);
    }
    return self;
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::VectorDeletingDestruct", 0x007281A0, &BankEntry_VectorDeletingDestruct);

// 0x728200 (thiscall): the children replaced by `count` empty entries (the old ones
// destructed last first; for 0 the old block's pointer stays, as with the tracks)
void __fastcall BankEntry_SetChildCount(BankEntry* self, re::Edx, uint32_t count) {
    BankEntry* children = self->children;
    if (children) {
        uint32_t* block = reinterpret_cast<uint32_t*>(children) - 1;
        for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
            GB_Destruct(&children[i]);
        }
        GB_Free(block);
    }
    self->childCount = count;
    if (count != 0) {
        uint32_t* block = static_cast<uint32_t*>(GB_Malloc(BlockBytes(count, sizeof(BankEntry))));
        if (block) {
            block[0] = count;
            GB_ConstructArray(block + 1, sizeof(BankEntry), int32_t(count), gbEntryConstruct);
            self->children = reinterpret_cast<BankEntry*>(block + 1);
            return;
        }
        self->children = nullptr;
    }
}
GLOBALSBANK_REVERSED("GlobalsBank::Entry::SetChildCount", 0x00728200, &BankEntry_SetChildCount);
