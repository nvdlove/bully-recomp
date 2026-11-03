#pragma once

// NiActorManager (vtable 0x965A88, 0xB4 bytes, constructor 0x801540) and the NiKFMTool it plays
// (vtable 0x965A94, 0x40 bytes, constructor 0x801A80): an actor's sequences by ID, the
// transitions between them (blend, morph, crossfade, chain), the timeline of events its
// callback object hears about (activated, deactivated, text key, end of sequence), and the
// NiControllerManager doing the playing. Layouts from the constructors; everything in ni::actor.
//
// NiControllerSequence and NiControllerManager are read by offset here (their files are another
// chunk's): a sequence's name +0x8, cycle type +0x24 (2 clamp), frequency +0x28, begin and end
// key times +0x2C / +0x30, last scaled time +0x3C, state +0x44 (0 inactive, 2 easing in, 3 easing
// out), end time +0x60; a manager's sequences +0x38 (count +0x3E) and its name map +0x44.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni::actor {

constexpr uint32_t kInvalidSequenceID = 0xFFFFFFFE;
constexpr uint32_t kAnySequenceID = 0xFFFFFFFF;
constexpr float kMaxFloat = 3.40282347e+38f;

// NiTPointerMap<unsigned int, T*> (vtable 0x965A9C over 0x950760): buckets of nodes
struct MapNode {
    re::Ptr32<MapNode> next;   // +0x0
    uint32_t key;    // +0x4
    re::Ptr32<void> value;     // +0x8
};
struct PointerMap {
    re::Ptr32<void> vtable;        // +0x0
    uint32_t hashSize;   // +0x4 (37)
    re::Ptr32<re::Ptr32<MapNode>> table;     // +0x8 (NiMalloc)
    uint32_t count;      // +0xC
};
static_assert(sizeof(PointerMap) == 0x10);

inline void* MapLookup(const PointerMap& map, uint32_t key) {
    for (MapNode* node = map.table[key % map.hashSize]; node != nullptr; node = node->next) {
        if (key == node->key) {
            return node->value;
        }
    }
    return nullptr;
}

// the first node of a map (its iteration's start)
inline MapNode* MapFirst(const PointerMap& map) {
    for (uint32_t i = 0; i < map.hashSize; i++) {
        if (map.table[i] != nullptr) {
            return map.table[i];
        }
    }
    return nullptr;
}

// the node after one (the next in its bucket, or the first of a later bucket)
inline MapNode* MapNext(const PointerMap& map, const MapNode* node) {
    if (node->next != nullptr) {
        return node->next;
    }
    for (uint32_t i = node->key % map.hashSize + 1; i < map.hashSize; i++) {
        if (map.table[i] != nullptr) {
            return map.table[i];
        }
    }
    return nullptr;
}

// ---- NiKFMTool

// Transition::BlendPair (8 bytes): the source's text key to blend from, the destination's to
// blend into
struct BlendPair {
    re::Ptr32<char> startKey;    // +0x0 (NiFixedString)
    re::Ptr32<char> targetKey;   // +0x4
};
static_assert(sizeof(BlendPair) == 8);

// Transition::ChainInfo (8 bytes)
struct ChainInfo {
    uint32_t sequenceID;   // +0x0
    float duration;        // +0x4 (-1: to the sequence's end)
};
static_assert(sizeof(ChainInfo) == 8);

// NiKFMTool::Transition (0x24 bytes)
struct Transition {
    uint32_t type;              // +0x0 (0 blend, 1 morph, 2 crossfade, 3 chain; 4 / 5 the defaults)
    float duration;             // +0x4 (0.25)
    re::Ptr32<re::Ptr32<BlendPair>> blendPairs;     // +0x8 (NiTPrimitiveSet: base, allocated, count)
    uint32_t blendAllocated;    // +0xC
    uint32_t blendPairCount;    // +0x10
    re::Ptr32<ChainInfo> chain;           // +0x14 (NiNew[], no count)
    uint32_t chainAllocated;    // +0x18
    uint32_t chainCount;        // +0x1C
    uint32_t storedType;        // +0x20 (6: the type itself; 4 / 5 the default sync / nonsync ones)
};
static_assert(offsetof(Transition, chain) == 0x14);
static_assert(sizeof(Transition) == 0x24);

// NiKFMTool::Sequence (0x1C bytes)
struct Sequence {
    uint32_t sequenceID;      // +0x0
    re::Ptr32<char> filename;           // +0x4 (NiFixedString)
    uint32_t animIndex;       // +0x8
    PointerMap transitions;   // +0xC (by destination ID)
};
static_assert(sizeof(Sequence) == 0x1C);

// NiKFMTool::SequenceGroup (0x14 bytes)
struct SequenceGroup {
    uint32_t groupID;    // +0x0
    re::Ptr32<char> name;          // +0x4
    re::Ptr32<void> sequences;     // +0x8 (0x18-byte SequenceInfo array)
    uint32_t count;      // +0xC
    uint32_t unknown10;
};
static_assert(sizeof(SequenceGroup) == 0x14);

struct NiKFMTool : NiRefObject {
    re::Ptr32<char> basePath;                        // +0x8 (NiFixedString; NiActorManager::Create sets it)
    re::Ptr32<char> unknownC;                        // +0xC
    re::Ptr32<char> modelPath;                       // +0x10 (the NIF)
    re::Ptr32<char> modelRoot;                       // +0x14 (the actor root's name in it)
    PointerMap sequences;                  // +0x18 (Sequence* by ID)
    PointerMap sequenceGroups;             // +0x28
    re::Ptr32<Transition> defaultSyncTransition;     // +0x38 (morph, stored type 4)
    re::Ptr32<Transition> defaultNonSyncTransition;  // +0x3C (crossfade, stored type 5)
};
static_assert(offsetof(NiKFMTool, sequences) == 0x18);
static_assert(sizeof(NiKFMTool) == 0x40);

// ---- NiActorManager

// NiTextKeyMatch (vtable 0x965A60, 0x10 bytes): slot 2 IsKeyMatch(key)
struct NiTextKeyMatch : NiRefObject {
    re::Ptr32<char> lastMatchedKey;   // +0x8 (NiFixedString)
    re::Ptr32<char> matchKey;         // +0xC
};
static_assert(sizeof(NiTextKeyMatch) == 0x10);

// a callback registration (NiRefObject: type, sequence (or any), text key match)
struct CallbackData : NiRefObject {
    uint32_t eventType;       // +0x8
    uint32_t sequenceID;      // +0xC
    re::Ptr32<NiTextKeyMatch> match;    // +0x10 (a reference)
};

// a timeline entry (in a 0x1C-byte list node)
struct TimelineData {
    float time;               // +0x0
    uint32_t eventType;       // +0x4 (0 activated, 1 deactivated, 2 text key, 3 end of sequence)
    uint32_t sequenceID;      // +0x8
    re::Ptr32<NiTextKeyMatch> match;    // +0xC (a reference)
    re::Ptr32<char> textKey;            // +0x10 (NiFixedString)
};
static_assert(sizeof(TimelineData) == 0x14);
static_assert(sizeof(NiTListNode<TimelineData>) == 0x1C);

// the result of working out a transition's timing (vtable 0x965AB0, 0x28 bytes, built on the
// stack): the destination's start frame, the transition's length, the source's end, the chain's
// total, the blend pair, and the chain steps' ChainCompletionInfos
struct CompletionInfo : NiRefObject {
    float desFrame;            // +0x8
    float duration;            // +0xC
    float srcEnd;              // +0x10
    float chainTotal;          // +0x14
    re::Ptr32<BlendPair> blendPair;      // +0x18
    re::Ptr32<re::Ptr32<NiRefObject>> chainBase;   // +0x1C (NiTObjectSet: base, allocated, count)
    uint32_t chainAllocated;   // +0x20
    uint32_t chainCount;       // +0x24
};
static_assert(sizeof(CompletionInfo) == 0x28);

// one chain step's timing (vtable 0x965A6C, 0x2C bytes)
struct ChainCompletionInfo : NiRefObject {
    float time;                // +0x8
    float desStart;            // +0xC
    float desEnd;              // +0x10
    float srcFrame;            // +0x14
    re::Ptr32<char> srcName;             // +0x18 (NiFixedString)
    re::Ptr32<char> desName;             // +0x1C
    re::Ptr32<Transition> transition;    // +0x20
    uint32_t srcID;            // +0x24
    uint32_t desID;            // +0x28
};
static_assert(sizeof(ChainCompletionInfo) == 0x2C);

struct NiActorManager : NiRefObject {
    re::Ptr32<NiKFMTool> kfmTool;                        // +0x8 (a reference)
    re::Ptr32<NiAVObject> nifRoot;                       // +0xC (a reference)
    re::Ptr32<NiRefObject> manager;                      // +0x10 (NiControllerManager, a reference)
    re::Ptr32<NiRefObject> boneLOD;                      // +0x14 (NiBoneLODController, a reference)
    re::Ptr32<void> callbackObject;                      // +0x18 (slots 1..4: activated, deactivated,
                                               //  text key, end of sequence)
    uint32_t targetID;                         // +0x1C
    float time;                                // +0x20
    float nextChangeTime;                      // +0x24 (the timeline's last event)
    uint8_t rebuildTimeline;                   // +0x28
    uint8_t startPending;                      // +0x29 (a blend waiting for its sync key)
    uint8_t pad2A[2];
    uint32_t transitionState;                  // +0x2C (0 none, 1 blending, 2 morphing, 3 crossfading)
    float transStartTime;                      // +0x30
    float transEndTime;                        // +0x34
    float transStartFrameTime;                 // +0x38
    re::Ptr32<const char> startTextKey;                  // +0x3C (the destination's key to blend into)
    uint32_t curID;                            // +0x40
    re::Ptr32<void> curSequence;                         // +0x44 (NiControllerSequence)
    uint32_t nextID;                           // +0x48
    re::Ptr32<void> nextSequence;                        // +0x4C
    float chainTime;                           // +0x50 (when the chain moves on; -FLT_MAX: not worked out)
    NiTPointerList<uint32_t> chainIDs;         // +0x54
    NiTPointerList<float> chainDurations;      // +0x64 (vtable 0x965A7C)
    uint8_t chainActive;                       // +0x74
    uint8_t cumulative;                        // +0x75
    uint8_t pad76[2];
    PointerMap sequenceMap;                    // +0x78 (NiControllerSequence* by ID)
    NiTArray<CallbackData*> callbacks;         // +0x88 (vtable 0x961DE8)
    NiTPointerList<TimelineData> timeline;     // +0x98 (vtable 0x965A54)
    re::Ptr32<uint32_t> extraSequences;                  // +0xA8 (NiTPrimitiveSet: base, allocated, count)
    uint32_t extraAllocated;                   // +0xAC
    uint32_t extraCount;                       // +0xB0
};
static_assert(offsetof(NiActorManager, transitionState) == 0x2C);
static_assert(offsetof(NiActorManager, chainIDs) == 0x54);
static_assert(offsetof(NiActorManager, sequenceMap) == 0x78);
static_assert(offsetof(NiActorManager, callbacks) == 0x88);
static_assert(offsetof(NiActorManager, timeline) == 0x98);
static_assert(sizeof(NiActorManager) == 0xB4);

// a controller sequence's fields by offset
template <typename T>
inline re::Field<T>& SeqField(void* sequence, uint32_t offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(sequence) + offset);
}
inline float SeqFrequency(void* sequence) {
    return SeqField<float>(sequence, 0x28);
}
inline float SeqBeginKeyTime(void* sequence) {
    return SeqField<float>(sequence, 0x2C);
}
inline float SeqEndKeyTime(void* sequence) {
    return SeqField<float>(sequence, 0x30);
}

} // namespace ni::actor
