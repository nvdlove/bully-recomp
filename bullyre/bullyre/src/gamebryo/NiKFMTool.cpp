// NiKFMTool (src/gamebryo/NiActorManager.h), the pieces of it in this part of the library: the
// constructor (its two default transitions) and destructor, a sequence's destructor (its own
// transitions deleted), a transition's constructor and blend pair clean-up, the transition
// lookups, the chain info arrays, and the NiTPointerMap deleting destructor the tool's maps share.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiActorManager.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::actor {
namespace {

using sys::Decrement;
using sys::Increment;
using sys::kNullFixedString;

GAME_VAR(long, ms_uiObjects, 0x020E2168);
constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kKFMToolVtable = 0x00965A94;
constexpr uintptr_t kMapVtable = 0x00965A9C;        // NiTPointerMap<unsigned int, T*>
constexpr uintptr_t kMapBaseVtable = 0x00950760;
constexpr uintptr_t kChainInfoConstruct = 0x008003D0;

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(void, __thiscall, Map_RemoveAll, 0x005AC520, PointerMap* self);
// (this file's, called where modes apply)
GAME_FN(Transition*, __thiscall, KFMTool_GetTransitionFromSequence, 0x00800AA0, NiKFMTool* self, uint32_t desID,
        Sequence* sequence);
GAME_FN(void, __thiscall, BlendPair_Destruct, 0x00801BE0, BlendPair* self);
GAME_FN(void, __thiscall, Transition_ClearBlendPairs, 0x00801C90, Transition* self);
GAME_FN(void, __thiscall, Sequence_Destruct, 0x00802420, Sequence* self);
GAME_FN(void, __thiscall, KFMTool_Destruct, 0x00803620, NiKFMTool* self);

inline void FixedStringRelease(re::Ptr32<char>* handle) {
    sys::FixedStringRelease(handle);
}

// a map destroyed as its destructor inlines it: emptied as itself and as its base, the table freed
inline void DestroyMap(PointerMap& map) {
    map.vtable = reinterpret_cast<void*>(kMapVtable);
    Map_RemoveAll(&map);
    map.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    Map_RemoveAll(&map);
    NiFree(map.table);
}

// a map made as the constructors inline it: 37 empty buckets
inline void MakeMap(PointerMap& map) {
    map.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    map.hashSize = 0x25;
    map.count = 0;
    map.table = static_cast<re::Ptr32<MapNode>*>(NiMalloc(0x94));
    crt::Memset(map.table, 0, map.hashSize * 4);
    map.vtable = reinterpret_cast<void*>(kMapVtable);
}

// a transition's own allocations and itself deleted (as the destructors inline it)
inline void DeleteTransition(Transition* transition) {
    Transition_ClearBlendPairs(transition);
    NiDeleteArray(transition->chain, 8);
    NiFree(transition->blendPairs);
    NiDelete(transition, 0x24);
}

// a default transition (the constructor's)
inline Transition* NewDefaultTransition(uint32_t type) {
    auto* transition = static_cast<Transition*>(NiNew(0x24));
    if (transition == nullptr) {
        return nullptr;
    }
    transition->type = type;
    transition->duration = 0.25f;
    transition->blendPairs = nullptr;
    transition->blendAllocated = 0;
    transition->blendPairCount = 0;
    transition->chain = nullptr;
    transition->chainAllocated = 0;
    transition->chainCount = 0;
    transition->storedType = 6;
    return transition;
}

// 0x8003D0 ChainInfo(): no sequence, no duration
ChainInfo* __fastcall ChainInfoConstruct(ChainInfo* self, re::Edx) {
    self->duration = 0.0f;
    self->sequenceID = 0;
    return self;
}

// 0x800A60 NiNew[] of count ChainInfos (no count before them)
ChainInfo* __cdecl ChainInfoNewArray(uint32_t count) {
    uint64_t product = uint64_t(count) * 8;
    void* array = NiNewArray((product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product));
    if (array == nullptr) {
        return nullptr;
    }
    VectorConstruct(array, 8, count, reinterpret_cast<void*>(kChainInfoConstruct));
    return static_cast<ChainInfo*>(array);
}

// 0x800AA0 GetTransitionFromSequence(destination ID, source sequence): its transition there, or
// none (the tool itself unused)
Transition* __fastcall GetTransitionFromSequence(NiKFMTool*, re::Edx, uint32_t desID, Sequence* sequence) {
    return static_cast<Transition*>(MapLookup(sequence->transitions, desID));
}

// 0x801160 GetTransition(source ID, destination ID)
Transition* __fastcall GetTransition(NiKFMTool* self, re::Edx, uint32_t srcID, uint32_t desID) {
    for (MapNode* node = self->sequences.table[srcID % self->sequences.hashSize]; node != nullptr; node = node->next) {
        if (srcID == node->key) {
            if (node->value == nullptr) {
                return nullptr;
            }
            return KFMTool_GetTransitionFromSequence(self, desID, static_cast<Sequence*>(node->value));
        }
    }
    return nullptr;
}

// 0x8011B0 IsValidTransition(source ID, destination ID, valid out): 0 with whether there's a
// transition, 2 (an error) for an unknown source
uint32_t __fastcall IsValidTransition(NiKFMTool* self, re::Edx, uint32_t srcID, uint32_t desID, uint8_t* valid) {
    for (MapNode* node = self->sequences.table[srcID % self->sequences.hashSize]; node != nullptr; node = node->next) {
        if (srcID == node->key) {
            if (node->value == nullptr) {
                return 2;
            }
            Transition* transition =
                KFMTool_GetTransitionFromSequence(self, desID, static_cast<Sequence*>(node->value));
            *valid = transition != nullptr ? 1 : 0;
            return 0;
        }
    }
    return 2;
}

// 0x801510 Transition(type, duration): no blend pairs, no chain, stored type 6
Transition* __fastcall TransitionConstruct(Transition* self, re::Edx, uint32_t type, float duration) {
    self->duration = duration;
    self->type = type;
    self->blendPairs = nullptr;
    self->blendAllocated = 0;
    self->blendPairCount = 0;
    self->chain = nullptr;
    self->chainAllocated = 0;
    self->chainCount = 0;
    self->storedType = 6;
    return self;
}

// 0x801A80 NiKFMTool(base path): the path held, the other strings empty, the maps, the default
// sync (morph) and nonsync (crossfade) transitions. (A failed allocation of one is written
// through anyway, as the original does.)
NiKFMTool* __fastcall Construct(NiKFMTool* self, re::Edx, const re::Ptr32<char>* basePath) {
    self->vtable = reinterpret_cast<void*>(kRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kKFMToolVtable);
    if (*basePath != kNullFixedString) {
        Increment(*basePath - 8);
    }
    self->basePath = *basePath;
    self->unknownC = kNullFixedString;
    self->modelPath = kNullFixedString;
    self->modelRoot = kNullFixedString;
    MakeMap(self->sequences);
    MakeMap(self->sequenceGroups);
    Transition* sync = NewDefaultTransition(1);
    self->defaultSyncTransition = sync;
    sync->storedType = 4;
    Transition* nonSync = NewDefaultTransition(2);
    self->defaultNonSyncTransition = nonSync;
    nonSync->storedType = 5;
    return self;
}

// 0x801BE0 ~BlendPair
void __fastcall BlendPairDestruct(BlendPair* self, re::Edx) {
    FixedStringRelease(&self->targetKey);
    FixedStringRelease(&self->startKey);
}

// 0x801C50 NiTPointerMap<unsigned int, T*>'s deleting destructor
PointerMap* __fastcall MapDeletingDestructor(PointerMap* self, re::Edx, uint32_t flags) {
    DestroyMap(*self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x801C90 ClearBlendPairs: each pair destroyed and deleted, none left
void __fastcall ClearBlendPairs(Transition* self, re::Edx) {
    for (uint32_t i = 0; i < self->blendPairCount; i++) {
        BlendPair* pair = self->blendPairs[i];
        if (pair != nullptr) {
            BlendPair_Destruct(pair);
            NiDelete(pair, 8);
        }
    }
    self->blendPairCount = 0;
}

// 0x802420 ~Sequence: its transitions deleted but for the tool's defaults (stored type 4 / 5),
// the map, the filename
void __fastcall SequenceDestruct(Sequence* self, re::Edx) {
    for (MapNode* node = MapFirst(self->transitions); node != nullptr;) {
        auto* transition = static_cast<Transition*>(node->value);
        node = MapNext(self->transitions, node);
        uint32_t stored = transition->storedType;
        uint32_t type = stored == 6 ? transition->type : stored;
        if (type == 4) {
            continue;
        }
        if ((stored == 6 ? transition->type : stored) == 5) {
            continue;
        }
        DeleteTransition(transition);
    }
    DestroyMap(self->transitions);
    FixedStringRelease(&self->filename);
}

// 0x803620 ~NiKFMTool: the sequences, the groups, the default transitions, the maps, the strings
void __fastcall Destruct(NiKFMTool* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kKFMToolVtable);
    for (MapNode* node = MapFirst(self->sequences); node != nullptr;) {
        auto* sequence = static_cast<Sequence*>(node->value);
        node = MapNext(self->sequences, node);
        if (sequence != nullptr) {
            Sequence_Destruct(sequence);
            NiDelete(sequence, 0x1C);
        }
    }
    for (MapNode* node = MapFirst(self->sequenceGroups); node != nullptr;) {
        auto* group = static_cast<SequenceGroup*>(node->value);
        node = MapNext(self->sequenceGroups, node);
        if (group != nullptr) {
            NiDeleteArray(group->sequences, 0x18);
            FixedStringRelease(&group->name);
            NiDelete(group, 0x14);
        }
    }
    if (self->defaultSyncTransition != nullptr) {
        DeleteTransition(self->defaultSyncTransition);
    }
    if (self->defaultNonSyncTransition != nullptr) {
        DeleteTransition(self->defaultNonSyncTransition);
    }
    DestroyMap(self->sequenceGroups);
    DestroyMap(self->sequences);
    FixedStringRelease(&self->modelRoot);
    FixedStringRelease(&self->modelPath);
    FixedStringRelease(&self->unknownC);
    FixedStringRelease(&self->basePath);
    self->vtable = reinterpret_cast<void*>(kRefObjectVtable);
    Decrement(&ms_uiObjects);
}

// 0x803BD0 the deleting destructor
NiKFMTool* __fastcall DeletingDestructor(NiKFMTool* self, re::Edx, uint32_t flags) {
    KFMTool_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x40);
    }
    return self;
}

} // namespace
} // namespace ni::actor

// ---- registration ---------------------------------------------------------------------

#define NIKFMTOOL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

using namespace ni::actor;

NIKFMTOOL_REVERSED("NiKFMTool::ChainInfo::ChainInfo", 0x008003D0, ChainInfoConstruct);
NIKFMTOOL_REVERSED("NiKFMTool::ChainInfo::NewArray", 0x00800A60, ChainInfoNewArray);
NIKFMTOOL_REVERSED("NiKFMTool::GetTransitionFromSequence", 0x00800AA0, GetTransitionFromSequence);
NIKFMTOOL_REVERSED("NiKFMTool::GetTransition", 0x00801160, GetTransition);
NIKFMTOOL_REVERSED("NiKFMTool::IsValidTransition", 0x008011B0, IsValidTransition);
NIKFMTOOL_REVERSED("NiKFMTool::Transition::Transition", 0x00801510, TransitionConstruct);
NIKFMTOOL_REVERSED("NiKFMTool::NiKFMTool", 0x00801A80, Construct);
NIKFMTOOL_REVERSED("NiKFMTool::Transition::BlendPair::~BlendPair", 0x00801BE0, BlendPairDestruct);
NIKFMTOOL_REVERSED("NiTPointerMap<unsigned int,T*>::DeletingDestructor_801C50", 0x00801C50, MapDeletingDestructor);
NIKFMTOOL_REVERSED("NiKFMTool::Transition::ClearBlendPairs", 0x00801C90, ClearBlendPairs);
NIKFMTOOL_REVERSED("NiKFMTool::Sequence::~Sequence", 0x00802420, SequenceDestruct);
NIKFMTOOL_REVERSED("NiKFMTool::~NiKFMTool", 0x00803620, Destruct);
NIKFMTOOL_REVERSED("NiKFMTool::DeletingDestructor", 0x00803BD0, DeletingDestructor);
