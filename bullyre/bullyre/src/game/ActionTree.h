#pragma once

// The action tree (Act\*.act, the /Global/... paths): nodes with conditions and tracks,
// played by an ActionController per ped. Shared by ActionController.cpp, ActionNodes*.cpp,
// ActionTracks*.cpp and ActionConditions*.cpp.

#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace acttree {

// The game's own run-time type ids: each class's id is taken from a counter (0xBF3830)
// the first time it's asked for (a guard word with bit 1, then the id)
GAME_VAR(int16_t, typeIdCounter, 0x00BF3830);
inline int16_t TypeId(uint32_t& guard, int16_t& id) {
    if (!(guard & 1)) {
        guard |= 1;
        id = typeIdCounter;
        typeIdCounter = int16_t(id + 1);
    }
    return id;
}
// a class's id slot (guard, id) by their addresses
template <uintptr_t Guard, uintptr_t Id>
int16_t TypeIdAt() {
    return TypeId(*reinterpret_cast<uint32_t*>(Guard), *reinterpret_cast<int16_t*>(Id));
}
// the ids used here
inline int16_t PlayableId() { return TypeIdAt<0x01B649CC, 0x01B649C8>(); }           // ActionNodePlayable
inline int16_t NodeReferenceTrackId() { return TypeIdAt<0x01B649D4, 0x01B649D0>(); } // INodeReferenceTrack
inline int16_t ReferenceId() { return TypeIdAt<0x01B649F0, 0x01B649EC>(); }          // ActionNodeReference
inline int16_t MultiParentId() { return TypeIdAt<0x01B649F8, 0x01B649F4>(); }        // ActionNodeMultiParentReference
inline int16_t WeightedId() { return TypeIdAt<0x01B64A00, 0x01B649FC>(); }           // ConditionWeightedRandom
inline int16_t NodeId() { return TypeIdAt<0x01B64A08, 0x01B64A04>(); }               // ActionNode
inline int16_t ImplementationId() { return TypeIdAt<0x01B64A10, 0x01B64A0C>(); }     // ActionNodeImplementation
inline int16_t ControllerTrackId() { return TypeIdAt<0x00C2C03C, 0x00C2C038>(); }    // tracks with a controller at +0x84

// The tree's root (/Global) and the key GetParent uses when given -1
GAME_VAR(uint8_t*, root, 0x01B649DC);
GAME_VAR(int32_t, currentKey, 0x00B0A604);
GAME_VAR(uint32_t, unresolvedHash, 0x01B649E8);   // a name no node has

// ActionNode's virtuals (byte offsets in its vtable)
enum NodeVf : int {
    kNodeDestruct = 0x0,       // (flags)
    kNodeIsA = 0x4,            // (type id)
    kNodeGetName = 0x8,        // the name's hash, by address
    kNodeFindChild = 0xC,      // (name*)
    kNodeUpdateWeights = 0x10,
    kNodeResolve = 0x14,
    kNodeAddChild = 0x18,
    kNodeSetParent = 0x1C,
    kNodeClearChildren = 0x20,
    kNodeRemoveChild = 0x24,
    kNodeNumChildren = 0x2C,
    kNodeGetChild = 0x30,      // (i)
    kNodeGetParent = 0x34,     // (key; -1: the current one)
    kNodeNumConditions = 0x38,
    kNodeGetCondition = 0x3C,  // (i)
    kNodeNumTracks = 0x40,
    kNodeCreateTrack = 0x44,   // (i)
    kNodeGetTrackKey = 0x48,   // (i)
    kNodeFindPlayable = 0x4C,  // (owner)
    kNodeGetConditions = 0x50,
    kNodeSetTarget = 0x54,     // (node, key, set parent)
    kNodeAddCondition = 0x58,
    kNodeReserve = 0x5C,
};

// A playing track: linked through +4 in its controller's lists
struct TrackLink {
    re::Ptr32<TrackLink> next;
};
struct TrackList {
    re::Ptr32<TrackLink> head;  // +0
    re::Ptr32<TrackLink> tail;  // +4
    int32_t count;    // +8
};
static_assert(sizeof(TrackList) == 0xC);
// a position in a list: the link and the one before it
struct TrackIter {
    re::Ptr32<TrackLink> node;
    re::Ptr32<TrackLink> prev;
};
inline uint8_t* TrackOf(TrackLink* link) {
    return link ? reinterpret_cast<uint8_t*>(link) - 4 : nullptr;
}
inline TrackLink* LinkOf(uint8_t* track) {
    return track ? reinterpret_cast<TrackLink*>(track + 4) : nullptr;
}
// the track's fields the controller uses
inline uint8_t& TrackKeepsAlive(uint8_t* track) { return track[8]; }
inline float& TrackStart(uint8_t* track) { return vc::At<float>(track, 0xC); }   // < 0: at once
inline float& TrackEnd(uint8_t* track) { return vc::At<float>(track, 0x10); }     // < 0: never
inline int32_t& TrackKey(uint8_t* track) { return vc::At<int32_t>(track, 0x14); }

enum TrackVf : int {
    kTrackIsA = 0x0,
    kTrackDestruct = 0x4,  // (flags)
    kTrackStart = 0x8,     // (owner)
    kTrackUpdate = 0xC,    // (dt) still running
    kTrackEnd = 0x10,
    kTrackResolve = 0x14,
    kTrackSetNode = 0x1C,  // (node)
};

// A ped's controller of the node it plays and that node's tracks
struct ActionController {
    uint8_t busy;             // +0x0 inside an update: plays and stops are deferred
    uint8_t playPending;      // +0x1
    uint8_t stopPending;      // +0x2
    uint8_t pad3;
    int16_t pendingPriority;  // +0x4 -1: none
    uint8_t pad6[2];
    float time;               // +0x8 in the node
    uint8_t ending;           // +0xC
    uint8_t padD[3];
    re::Ptr32<uint8_t> node;            // +0x10 playing
    re::Ptr32<uint8_t> pendingNode;     // +0x14
    re::Ptr32<uint8_t> owner;           // +0x18 (its key stack at +0x44 / +0x48)
    uint8_t startingTracks;   // +0x1C
    uint8_t pad1D[3];
    TrackList pending;        // +0x20 not started yet
    TrackList active;         // +0x2C
};
static_assert(sizeof(ActionController) == 0x38);

// the owner's current key (its key stack's top)
inline int32_t OwnerKey(uint8_t* owner) {
    return vc::At<int32_t>(owner, 0x48 + vc::At<int32_t>(owner, 0x44) * 4);
}

} // namespace acttree
