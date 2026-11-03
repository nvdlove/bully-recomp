// ActionController: plays an action node for a ped: the node's tracks go on a pending
// list, start (move to the active list) when the controller's time reaches their start,
// update every frame and end at their end time; a node ends when no active track keeps
// it alive. Plays and stops asked for during an update are deferred to its end. Also the
// owner's key stack, a few nodes looked up once, the type ids and the tree's shutdown.
// 0x5F37D0..0x5F4DE0.
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cstdint>
#include <cstdlib>

#include "game/ActionTree.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace acttree;

namespace {

GAME_FN(uint8_t*, __cdecl, ActionTree_Find, 0x005F5B10, const char* path, uint8_t* from);
GAME_FN(int32_t, __thiscall, Node_GetRootKey, 0x005F4FA0, uint8_t* node);
GAME_FN(uint8_t, __thiscall, Node_HasAncestorNamed, 0x005F5090, uint8_t* node, const uint32_t* name, int32_t key);
GAME_FN(uint8_t, __thiscall, Node_HasAncestor, 0x005F50E0, uint8_t* node, uint8_t* ancestor, int32_t key);
GAME_FN(void, __cdecl, Entity_CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(TrackIter*, __thiscall, List_FindWithPrev, 0x005F3DA0, TrackList* list, TrackIter* out, TrackLink* link);
GAME_FN(TrackLink*, __thiscall, List_Remove, 0x005F3DE0, TrackList* list, TrackIter* it);
GAME_FN(void, __thiscall, Ctl_DeletePending, 0x005F3E90, ActionController* self);
GAME_FN(void, __thiscall, Ctl_DeleteActive, 0x005F3EE0, ActionController* self);
GAME_FN(void, __thiscall, Ctl_EndActive, 0x005F3F30, ActionController* self);
GAME_FN(void, __thiscall, Ctl_QueueTracks, 0x005F3F80, ActionController* self, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_Stop, 0x005F40A0, ActionController* self);
GAME_FN(void, __thiscall, Ctl_StartTracks, 0x005F4110, ActionController* self, float dt, int all);
GAME_FN(void, __thiscall, Ctl_EndFinished, 0x005F4270, ActionController* self, float dt);
GAME_FN(uint8_t, __thiscall, Ctl_UpdateActive, 0x005F4390, ActionController* self, float dt);
GAME_FN(void, __thiscall, Ctl_Flush, 0x005F44E0, ActionController* self, float dt);
GAME_FN(void, __thiscall, Ctl_PlayNode, 0x005F45F0, ActionController* self, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_QueueTrack, 0x005F46E0, ActionController* self, int index, uint8_t* node);
GAME_FN(uint8_t, __thiscall, Ctl_IsPlaying, 0x005F4800, ActionController* self, const uint32_t* name, int ancestors, int nested);
GAME_FN(uint8_t, __thiscall, Ctl_IsPlayingNode, 0x005F48D0, ActionController* self, uint8_t* node, int ancestors, int nested);
GAME_FN(void, __thiscall, Ctl_Play, 0x005F4A50, ActionController* self, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_QueueAllTracks, 0x005F4AC0, ActionController* self, uint8_t* node);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* key);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __cdecl, ActFiles_Shutdown, 0x0061A870);
GAME_FN(void, __cdecl, Conditions_Shutdown, 0x005FB7E0);

GAME_VAR(uint8_t*, hitTreeBase, 0x01B649B4);
GAME_VAR(uint8_t*, swimmingBase, 0x01B649B8);
GAME_VAR(uint8_t*, npcFacial, 0x01B649BC);
GAME_VAR(uint8_t*, inLiquidIsSwimming, 0x01B649C0);
GAME_VAR(uint8_t*, fallingOppsBase, 0x01B649C4);
GAME_VAR(double, randomLow_9009A0, 0x009009A0);   // 0
GAME_VAR(double, randMax_900CE8, 0x00900CE8);     // 32767

// the track's link appended to a list
void Append(TrackList& list, TrackLink* link) {
    link->next = nullptr;
    if (!list.head) {
        list.head = link;
    } else {
        list.tail->next = link;
    }
    list.count++;
    list.tail = link;
}

// 0x5F37D0 (cdecl): nodes looked up once
void __cdecl CacheNodes() {
    hitTreeBase = ActionTree_Find("/Global/HitTree/Base/HitTreeBase", nullptr);
    swimmingBase = ActionTree_Find("/Global/Actions/Swimming/Base", nullptr);
    npcFacial = ActionTree_Find("/Global/NPCFacial", nullptr);
    inLiquidIsSwimming = ActionTree_Find("/Global/AI/InLiquid/IsSwimming", nullptr);
    fallingOppsBase = ActionTree_Find("/Global/Player/JumpActions/Jump/FallingOpps/FallingOppsBase", nullptr);
}

// 0x5F3830 (thiscall on the owner): its tree node, the key stack restarted with the
// node's root key
void __fastcall Owner_SetNode(uint8_t* self, re::Edx, uint8_t* node) {
    vc::At<uint8_t*>(self, 0x40) = node;
    if (node) {
        vc::At<int32_t>(self, 0x44) = 0;
        int32_t key = Node_GetRootKey(node);
        vc::At<int32_t>(self, 0x48 + vc::At<int32_t>(self, 0x44) * 4) = key;
    }
}

// 0x5F3860 (thiscall on the owner): a key pushed (two at most)
void __fastcall Owner_PushKey(uint8_t* self, re::Edx, int32_t key) {
    int32_t top = vc::At<int32_t>(self, 0x44);
    if (top < 1) {
        top++;
        vc::At<int32_t>(self, 0x44) = top;
        vc::At<int32_t>(self, 0x48 + top * 4) = key;
    }
}

// 0x5F3C10 (thiscall): an action context's constructor (its entity reference let go)
uint8_t* __fastcall Context_Construct(uint8_t* self, re::Edx) {
    self[0] = 0;
    vc::At<float>(self, 8) = 0.0f;
    vc::At<float>(self, 0xC) = 0.0f;
    vc::At<uint8_t*>(self, 4) = nullptr;
    for (int at = 0x10; at <= 0x28; at += 4) {
        vc::At<float>(self, at) = 0.0f;
    }
    vc::At<int32_t>(self, 0x40) = 0;
    self[0x44] = 1;
    vc::At<float>(self, 0x2C) = -1.0f;
    vc::At<int32_t>(self, 0x48) = -1;
    vc::At<float>(self, 0x30) = -1.0f;
    vc::At<int32_t>(self, 0x60) = 0;
    self[0x68] = 0;
    vc::At<float>(self, 0x38) = 0.0f;
    vc::At<float>(self, 0x4C) = -1.0f;
    vc::At<float>(self, 0x50) = -1.0f;
    vc::At<float>(self, 0x54) = 0.0f;
    vc::At<float>(self, 0x58) = 0.0f;
    re::Ptr32<uint8_t>* ref = &vc::At<uint8_t*>(self, 4);
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
        *ref = nullptr;
    }
    vc::At<float>(self, 0x3C) = 0.0f;
    self[0x34] = 0;
    vc::At<int32_t>(self, 0x64) = 0;
    return self;
}

// 0x5F3CA0 / 0x5F3CE0 / 0x5F4C30 / 0x5F4C70 (cdecl): type ids
int16_t __cdecl GetPlayableId() {
    return PlayableId();
}
int16_t __cdecl GetNodeReferenceTrackId() {
    return NodeReferenceTrackId();
}
int16_t __cdecl GetReferenceId() {
    return ReferenceId();
}
int16_t __cdecl GetMultiParentId() {
    return MultiParentId();
}

// 0x5F3D20 (thiscall on a controller): a play request kept if its priority is higher
void __fastcall Ctl_RequestPlay(ActionController* self, re::Edx, uint8_t* node, int priority, int force) {
    if (re::Flag(force)) {
        self->playPending = 1;
    }
    if (priority > self->pendingPriority) {
        self->pendingPriority = int16_t(priority);
        self->pendingNode = node;
    }
}

// 0x5F3D50 (thiscall): the node at `path` is the one playing (or, `ancestors`, one of
// its ancestors under the owner's key)
uint8_t __fastcall Ctl_IsPlayingPath(ActionController* self, re::Edx, const char* path, int ancestors) {
    uint8_t* node = ActionTree_Find(path, nullptr);
    if (!node || !self->node) {
        return 0;
    }
    if (self->node == node) {
        return 1;
    }
    if (!re::Flag(ancestors)) {
        return 0;
    }
    return Node_HasAncestor(self->node, node, OwnerKey(self->owner));
}

// 0x5F3DA0 (thiscall on a list): the link and the one before it ({0, 0}: not there)
TrackIter* __fastcall List_FindWithPrevImpl(TrackList* self, re::Edx, TrackIter* out, TrackLink* link) {
    TrackLink* p = self->head;
    TrackLink* prev = nullptr;
    out->node = nullptr;
    out->prev = nullptr;
    if (!p) {
        return out;
    }
    while (p != link) {
        prev = p;
        p = p->next;
        if (!p) {
            return out;
        }
    }
    out->node = p;
    out->prev = prev;
    return out;
}

// 0x5F3DE0 (thiscall on a list): the link at the position out
TrackLink* __fastcall List_RemoveImpl(TrackList* self, re::Edx, TrackIter* it) {
    TrackLink* node = it->node;
    TrackLink* prev = it->prev;
    if (node == self->head) {
        self->head = self->head->next;
        if (self->tail == node) {
            self->tail = nullptr;
        }
    } else {
        if (!prev) {
            TrackIter found;
            prev = List_FindWithPrev(self, &found, node)->prev;
        }
        prev->next = node->next;
        if (self->tail == node) {
            self->tail = prev;
        }
    }
    node->next = nullptr;
    self->count--;
    return node;
}

// 0x5F3E50 (thiscall): the constructor
ActionController* __fastcall Ctl_Construct(ActionController* self, re::Edx) {
    self->time = 0.0f;
    self->busy = 0;
    self->playPending = 0;
    self->stopPending = 0;
    self->pendingPriority = -1;
    self->node = nullptr;
    self->pendingNode = nullptr;
    self->owner = nullptr;
    self->startingTracks = 0;
    self->pending = {nullptr, nullptr, 0};
    self->active = {nullptr, nullptr, 0};
    self->ending = 0;
    return self;
}

// the list emptied, each track deleted (`end`: ended first)
void DeleteAll(TrackList& list, bool end) {
    while (list.head) {
        TrackLink* link = list.head;
        list.head = link->next;
        if (!link->next) {
            list.tail = nullptr;
        }
        link->next = nullptr;
        list.count--;
        uint8_t* track = TrackOf(link);
        if (end) {
            vc::Call<void>(track, kTrackEnd);
            vc::Call<void>(track, kTrackDestruct, 1);
        } else if (track) {
            vc::Call<void>(track, kTrackDestruct, 1);
        }
    }
}

// 0x5F3E90 (thiscall): the pending tracks deleted
void __fastcall Ctl_DeletePendingImpl(ActionController* self, re::Edx) {
    DeleteAll(self->pending, false);
}

// 0x5F3EE0 (thiscall): the active tracks deleted
void __fastcall Ctl_DeleteActiveImpl(ActionController* self, re::Edx) {
    DeleteAll(self->active, false);
}

// 0x5F3F30 (thiscall): the active tracks ended and deleted
void __fastcall Ctl_EndActiveImpl(ActionController* self, re::Edx) {
    DeleteAll(self->active, true);
}

// 0x5F3F80 (thiscall): a node's tracks made and put on the pending list (node
// references told their node and resolved, under the owner's key)
void __fastcall Ctl_QueueTracksImpl(ActionController* self, re::Edx, uint8_t* node) {
    if (!node) {
        return;
    }
    int count = vc::Call<int>(node, kNodeNumTracks);
    for (int i = 0; i < count; i++) {
        vc::Call<int32_t>(node, kNodeGetTrackKey, i);
        uint8_t* track = vc::Call<uint8_t*>(node, kNodeCreateTrack, i);
        TrackLink* link = nullptr;
        if (track) {
            if (vc::Call<uint8_t>(track, kTrackIsA, int(uint16_t(NodeReferenceTrackId())))) {
                currentKey = OwnerKey(self->owner);
                vc::Call<void>(track, kTrackSetNode, node);
                vc::Call<void>(track, kTrackResolve);
            }
            link = LinkOf(track);
        }
        // (a track the node couldn't make: the game writes through nullptr)
        link->next = nullptr;
        if (!self->pending.head) {
            self->pending.head = link;
        } else {
            self->pending.tail->next = link;
        }
        self->pending.count++;
        self->pending.tail = link;
    }
}

// 0x5F4070 (thiscall): everything dropped (the tracks not ended)
void __fastcall Ctl_Clear(ActionController* self, re::Edx) {
    self->pendingNode = nullptr;
    self->pendingPriority = -1;
    Ctl_DeletePending(self);
    Ctl_DeleteActive(self);
    self->time = 0.0f;
    self->node = nullptr;
    self->ending = 0;
}

// 0x5F40A0 (thiscall): stopped (the active tracks ended); deferred while busy
void __fastcall Ctl_StopImpl(ActionController* self, re::Edx) {
    if (self->busy) {
        self->stopPending = 1;
        return;
    }
    self->busy = 1;
    Ctl_EndActive(self);
    self->busy = 0;
    self->pendingNode = nullptr;
    self->pendingPriority = -1;
    Ctl_DeletePending(self);
    Ctl_DeleteActive(self);
    self->time = 0.0f;
    self->node = nullptr;
    self->ending = 0;
}

// 0x5F40F0 (thiscall): the tracks dropped, the node kept
void __fastcall Ctl_DropTracks(ActionController* self, re::Edx) {
    self->pendingNode = nullptr;
    self->pendingPriority = -1;
    Ctl_DeletePending(self);
    Ctl_DeleteActive(self);
}

// 0x5F4110 (thiscall): the pending tracks due by time + dt (`all`: also the ones
// starting at once, start < 0) moved to the active list and started
void __fastcall Ctl_StartTracksImpl(ActionController* self, re::Edx, float dt, int all) {
    if (self->startingTracks) {
        return;
    }
    uint8_t savedBusy = self->busy;
    TrackLink* cur = self->pending.head;
    self->startingTracks = 1;
    self->busy = 1;
    TrackLink* prev = nullptr;
    while (cur) {
        uint8_t* track = TrackOf(cur);
        float start = TrackStart(track);
        bool due = (re::Flag(all) && start < 0.0f) || (start >= 0.0f && start <= self->time + dt);
        if (!due) {
            prev = cur;
            cur = cur->next;
            continue;
        }
        bool wasHead = cur == self->pending.head;
        TrackIter it{cur, prev};
        List_Remove(&self->pending, &it);
        Append(self->active, LinkOf(track));
        vc::Call<void>(track, kTrackStart, self->owner);
        if (wasHead) {
            cur = self->pending.head;
            prev = nullptr;
        } else {
            cur = prev->next;
        }
    }
    self->busy = savedBusy;
    self->startingTracks = 0;
}

// an active track out of the list, the position moved on (read before the track's calls)
TrackLink* RemoveActive(ActionController* self, TrackLink* cur, re::Ptr32<TrackLink>& prev) {
    TrackIter it{cur, prev};
    if (cur == self->active.head) {
        List_Remove(&self->active, &it);
        prev = nullptr;
        return self->active.head;
    }
    List_Remove(&self->active, &it);
    return prev ? prev->next : nullptr;
}

// 0x5F4270 (thiscall): the active tracks past their end time: updated a last time,
// ended and deleted
void __fastcall Ctl_EndFinishedImpl(ActionController* self, re::Edx, float dt) {
    TrackLink* cur = self->active.head;
    re::Ptr32<TrackLink> prev = nullptr;
    while (cur) {
        uint8_t* track = TrackOf(cur);
        float end = TrackEnd(track);
        if (!(end >= 0.0f && self->time >= end)) {
            prev = cur;
            cur = cur->next;
            continue;
        }
        cur = RemoveActive(self, cur, prev);
        vc::Call<uint8_t>(track, kTrackUpdate, dt);
        vc::Call<void>(track, kTrackEnd);
        vc::Call<void>(track, kTrackDestruct, 1);
    }
}

// 0x5F4390 (thiscall): the active tracks updated; the ones done or past their end
// ended and deleted. Whether one still running keeps the node alive
uint8_t __fastcall Ctl_UpdateActiveImpl(ActionController* self, re::Edx, float dt) {
    if (!self->active.head) {
        return 0;
    }
    uint8_t alive = 0;
    TrackLink* cur = self->active.head;
    re::Ptr32<TrackLink> prev = nullptr;
    while (cur) {
        uint8_t* track = TrackOf(cur);
        float end = TrackEnd(track);
        uint8_t running = vc::Call<uint8_t>(track, kTrackUpdate, dt);
        bool expired = end >= 0.0f && self->time >= end;
        if (running && !expired) {
            if (TrackKeepsAlive(track)) {
                alive = 1;
            }
            prev = cur;
            cur = cur->next;
            continue;
        }
        cur = RemoveActive(self, cur, prev);
        vc::Call<void>(track, kTrackEnd);
        vc::Call<void>(track, kTrackDestruct, 1);
    }
    return alive;
}

float RandomUnit() {
    float range = float(1.0 - randomLow_9009A0);
    float fraction = float(double(Rand()) / randMax_900CE8);
    return float(double(fraction * range) + randomLow_9009A0);
}

// 0x5F44E0 (thiscall): the owner's four random numbers renewed, then every track run
// out: the active ones ended, all pending ones started, updated once and ended
void __fastcall Ctl_FlushImpl(ActionController* self, re::Edx, float dt) {
    if (self->owner) {
        vc::At<float>(self->owner, 0x24) = RandomUnit();
        vc::At<float>(self->owner, 0x28) = RandomUnit();
        vc::At<float>(self->owner, 0x2C) = RandomUnit();
        vc::At<float>(self->owner, 0x30) = RandomUnit();
    }
    uint8_t savedBusy = self->busy;
    self->ending = 1;
    self->busy = 1;
    Ctl_EndActive(self);
    Ctl_StartTracks(self, dt, 1);
    Ctl_UpdateActive(self, dt);
    Ctl_EndActive(self);
    self->busy = savedBusy;
    self->ending = 0;
}

// 0x5F45F0 (thiscall): a (playable) node played now, or once the update is done
void __fastcall Ctl_PlayNodeImpl(ActionController* self, re::Edx, uint8_t* node) {
    if (!node) {
        return;
    }
    if (self->busy) {
        self->pendingNode = node;
        self->playPending = 1;
        self->pendingPriority = 0;
        return;
    }
    Ctl_Flush(self, 0.0f);
    self->time = 0.0f;
    self->playPending = 0;
    self->pendingNode = nullptr;
    self->pendingPriority = -1;
    Ctl_DeletePending(self);
    Ctl_DeleteActive(self);
    self->node = node;
    Ctl_QueueTracks(self, node);
}

// 0x5F4660 (thiscall): the tracks of the node (or of the playable node it leads to
// for the owner) queued without playing it (nullptr: the game calls through it)
void __fastcall Ctl_QueueNodeTracks(ActionController* self, re::Edx, uint8_t* node) {
    if (node && vc::Call<uint8_t>(node, kNodeIsA, int(uint16_t(PlayableId())))) {
        Ctl_QueueTracks(self, node);
        return;
    }
    uint8_t* playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, self->owner);
    if (playable) {
        Ctl_QueueTracks(self, playable);
    }
}

// 0x5F46E0 (thiscall): the node's track `index` queued unless one with its key is active
void __fastcall Ctl_QueueTrackImpl(ActionController* self, re::Edx, int index, uint8_t* node) {
    for (TrackLink* link = self->active.head; link; link = link->next) {
        int32_t key = TrackKey(TrackOf(link));
        if (key == vc::Call<int32_t>(node, kNodeGetTrackKey, index)) {
            return;
        }
    }
    uint8_t* track = vc::Call<uint8_t*>(node, kNodeCreateTrack, index);
    TrackLink* link = nullptr;
    if (track) {
        if (vc::Call<uint8_t>(track, kTrackIsA, int(uint16_t(NodeReferenceTrackId())))) {
            vc::Call<void>(track, kTrackSetNode, node);
            vc::Call<void>(track, kTrackResolve);
        }
        link = LinkOf(track);
    }
    link->next = nullptr;
    if (!self->pending.head) {
        self->pending.count++;
        self->pending.head = link;
        self->pending.tail = link;
        return;
    }
    self->pending.tail->next = link;
    self->pending.count++;
    self->pending.tail = link;
}

// 0x5F4790 (thiscall): the active track with the key ended and deleted (returns what
// the game leaves in eax: the last track looked at, or the deleted one)
uint8_t* __fastcall Ctl_EndTrackWithKey(ActionController* self, re::Edx, int32_t key) {
    TrackLink* link = self->active.head;
    if (!link) {
        return nullptr;   // (eax as it came in)
    }
    TrackLink* prev = nullptr;
    while (TrackKey(TrackOf(link)) != key) {
        prev = link;
        link = link->next;
        if (!link) {
            return TrackOf(prev);
        }
    }
    TrackIter it{link, prev};
    List_Remove(&self->active, &it);
    uint8_t* track = TrackOf(link);
    vc::Call<void>(track, kTrackEnd);
    return vc::Call<uint8_t*>(track, kTrackDestruct, 1);
}

// 0x5F4800 (thiscall): the playing node has the name (or, `ancestors`, one of its
// ancestors does; `nested`: or a controller track's controller plays it)
uint8_t __fastcall Ctl_IsPlayingImpl(ActionController* self, re::Edx, const uint32_t* name, int ancestors, int nested) {
    if (!self->node) {
        return 0;
    }
    if (*name == *vc::Call<uint32_t*>(self->node, kNodeGetName)) {
        return 1;
    }
    if (re::Flag(ancestors) && Node_HasAncestorNamed(self->node, name, OwnerKey(self->owner))) {
        return 1;
    }
    if (!re::Flag(nested)) {
        return 0;
    }
    for (TrackLink* link = self->active.head; link; link = link->next) {
        uint8_t* track = TrackOf(link);
        if (!track) {
            continue;
        }
        if (vc::Call<uint8_t>(track, kTrackIsA, int(uint16_t(ControllerTrackId()))) && track + 0x84 &&
            Ctl_IsPlaying(reinterpret_cast<ActionController*>(track + 0x84), name, ancestors, 1)) {
            return 1;
        }
    }
    return 0;
}

// 0x5F48D0 (thiscall): the same by node (the controller tracks also looked at when the
// owner's +0x50 is positive)
uint8_t __fastcall Ctl_IsPlayingNodeImpl(ActionController* self, re::Edx, uint8_t* node, int ancestors, int nested) {
    if (!self->node) {
        return 0;
    }
    if (self->node == node) {
        return 1;
    }
    if (re::Flag(ancestors) && Node_HasAncestor(self->node, node, OwnerKey(self->owner))) {
        return 1;
    }
    if (!re::Flag(nested) && vc::At<int32_t>(self->owner, 0x50) <= 0) {
        return 0;
    }
    for (TrackLink* link = self->active.head; link; link = link->next) {
        uint8_t* track = TrackOf(link);
        if (!track) {
            continue;
        }
        if (vc::Call<uint8_t>(track, kTrackIsA, int(uint16_t(ControllerTrackId()))) && track + 0x84 &&
            Ctl_IsPlayingNode(reinterpret_cast<ActionController*>(track + 0x84), node, ancestors, 1)) {
            return 1;
        }
    }
    return 0;
}

// 0x5F49A0 (thiscall): IsPlaying by the node's name
uint8_t __fastcall Ctl_IsPlayingName(ActionController* self, re::Edx, const char* name, int ancestors, int nested) {
    uint32_t key;
    NameKey_Construct(&key, name);
    uint8_t result = Ctl_IsPlaying(self, &key, ancestors, nested);
    NameKey_Destruct(&key);
    return result;
}

// 0x5F49E0 (thiscall): the active track `index`
uint8_t* __fastcall Ctl_GetActiveTrack(ActionController* self, re::Edx, int index) {
    if (index < 0 || index >= self->active.count) {
        return nullptr;
    }
    TrackLink* link = self->active.head;
    for (; index != 0; index--) {
        link = link->next;
    }
    return link ? TrackOf(link) : nullptr;
}

// 0x5F4A10 (thiscall): the node ended now
void __fastcall Ctl_End(ActionController* self, re::Edx, float dt) {
    self->playPending = 0;
    Ctl_Flush(self, dt);
    Ctl_DeleteActive(self);
    Ctl_DeletePending(self);
    self->time = 0.0f;
    self->node = nullptr;
    self->ending = 0;
}

// 0x5F4A50 (thiscall): a node played (a non-playable one: the playable node it leads
// to for the owner)
void __fastcall Ctl_PlayImpl(ActionController* self, re::Edx, uint8_t* node) {
    if (!node) {
        return;
    }
    if (self->busy) {
        self->pendingNode = node;
        self->playPending = 1;
        self->pendingPriority = 0;
        return;
    }
    if (vc::Call<uint8_t>(node, kNodeIsA, int(uint16_t(PlayableId())))) {
        Ctl_PlayNode(self, node);
        return;
    }
    Ctl_PlayNode(self, vc::Call<uint8_t*>(node, kNodeFindPlayable, self->owner));
}

// 0x5F4AC0 (thiscall): all a node's tracks queued (the ones not active)
void __fastcall Ctl_QueueAllTracksImpl(ActionController* self, re::Edx, uint8_t* node) {
    if (!node) {
        return;
    }
    int count = vc::Call<int>(node, kNodeNumTracks);
    for (int i = 0; i < count; i++) {
        Ctl_QueueTrack(self, i, node);
    }
}

// 0x5F4B00 (thiscall): a frame: finished tracks ended, due ones started, the active
// ones updated; the node ends when none keeps it alive or a play / stop is pending
void __fastcall Ctl_Update(ActionController* self, re::Edx, float dt) {
    if (!self->node) {
        return;
    }
    self->busy = 1;
    Ctl_EndFinished(self, dt);
    Ctl_StartTracks(self, dt, 0);
    uint8_t alive = Ctl_UpdateActive(self, dt);
    self->busy = 0;
    if (alive && !self->playPending && !self->stopPending) {
        self->time = float(double(self->time) + double(dt));
        return;
    }
    self->playPending = 0;
    Ctl_Flush(self, dt);
    Ctl_DeleteActive(self);
    Ctl_DeletePending(self);
    self->time = 0.0f;
    self->node = nullptr;
    self->ending = 0;
    if (self->stopPending) {
        Ctl_Stop(self);
        self->stopPending = 0;
    }
    if (self->pendingNode) {
        Ctl_Play(self, self->pendingNode);
    }
}

// 0x5F4BB0 (thiscall): 0x5F4660 with all the tracks queued (0x5F4AC0)
void __fastcall Ctl_QueueNodeAllTracks(ActionController* self, re::Edx, uint8_t* node) {
    if (node && vc::Call<uint8_t>(node, kNodeIsA, int(uint16_t(PlayableId())))) {
        Ctl_QueueAllTracks(self, node);
        return;
    }
    uint8_t* playable = vc::Call<uint8_t*>(node, kNodeFindPlayable, self->owner);
    if (playable) {
        Ctl_QueueAllTracks(self, playable);
    }
}

// 0x5F4DE0 (cdecl): the act files, the conditions' tables and the tree deleted
void __cdecl ActionTree_Shutdown() {
    ActFiles_Shutdown();
    Conditions_Shutdown();
    if (root) {
        vc::Call<void>(root, kNodeDestruct, 1);
    }
}

} // namespace

#define ACTCTL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTCTL_REVERSED("ActionTree::CacheNodes", 0x005F37D0, &CacheNodes);
ACTCTL_REVERSED("ActionOwner::SetNode", 0x005F3830, &Owner_SetNode);
ACTCTL_REVERSED("ActionOwner::PushKey", 0x005F3860, &Owner_PushKey);
ACTCTL_REVERSED("ActionContext::Construct", 0x005F3C10, &Context_Construct);
ACTCTL_REVERSED("ActionNodePlayable::TypeId", 0x005F3CA0, &GetPlayableId);
ACTCTL_REVERSED("INodeReferenceTrack::TypeId", 0x005F3CE0, &GetNodeReferenceTrackId);
ACTCTL_REVERSED("ActionController::RequestPlay", 0x005F3D20, &Ctl_RequestPlay);
ACTCTL_REVERSED("ActionController::IsPlayingPath", 0x005F3D50, &Ctl_IsPlayingPath);
ACTCTL_REVERSED("TrackList::FindWithPrev", 0x005F3DA0, &List_FindWithPrevImpl);
ACTCTL_REVERSED("TrackList::Remove", 0x005F3DE0, &List_RemoveImpl);
ACTCTL_REVERSED("ActionController::Construct", 0x005F3E50, &Ctl_Construct);
ACTCTL_REVERSED("ActionController::DeletePending", 0x005F3E90, &Ctl_DeletePendingImpl);
ACTCTL_REVERSED("ActionController::DeleteActive", 0x005F3EE0, &Ctl_DeleteActiveImpl);
ACTCTL_REVERSED("ActionController::EndActive", 0x005F3F30, &Ctl_EndActiveImpl);
ACTCTL_REVERSED("ActionController::QueueTracks", 0x005F3F80, &Ctl_QueueTracksImpl);
ACTCTL_REVERSED("ActionController::Clear", 0x005F4070, &Ctl_Clear);
ACTCTL_REVERSED("ActionController::Stop", 0x005F40A0, &Ctl_StopImpl);
ACTCTL_REVERSED("ActionController::DropTracks", 0x005F40F0, &Ctl_DropTracks);
ACTCTL_REVERSED("ActionController::StartTracks", 0x005F4110, &Ctl_StartTracksImpl);
ACTCTL_REVERSED("ActionController::EndFinished", 0x005F4270, &Ctl_EndFinishedImpl);
ACTCTL_REVERSED("ActionController::UpdateActive", 0x005F4390, &Ctl_UpdateActiveImpl);
ACTCTL_REVERSED("ActionController::Flush", 0x005F44E0, &Ctl_FlushImpl);
ACTCTL_REVERSED("ActionController::PlayNode", 0x005F45F0, &Ctl_PlayNodeImpl);
ACTCTL_REVERSED("ActionController::QueueNodeTracks", 0x005F4660, &Ctl_QueueNodeTracks);
ACTCTL_REVERSED("ActionController::QueueTrack", 0x005F46E0, &Ctl_QueueTrackImpl);
ACTCTL_REVERSED("ActionController::EndTrackWithKey", 0x005F4790, &Ctl_EndTrackWithKey);
ACTCTL_REVERSED("ActionController::IsPlaying", 0x005F4800, &Ctl_IsPlayingImpl);
ACTCTL_REVERSED("ActionController::IsPlayingNode", 0x005F48D0, &Ctl_IsPlayingNodeImpl);
ACTCTL_REVERSED("ActionController::IsPlayingName", 0x005F49A0, &Ctl_IsPlayingName);
ACTCTL_REVERSED("ActionController::GetActiveTrack", 0x005F49E0, &Ctl_GetActiveTrack);
ACTCTL_REVERSED("ActionController::End", 0x005F4A10, &Ctl_End);
ACTCTL_REVERSED("ActionController::Play", 0x005F4A50, &Ctl_PlayImpl);
ACTCTL_REVERSED("ActionController::QueueAllTracks", 0x005F4AC0, &Ctl_QueueAllTracksImpl);
ACTCTL_REVERSED("ActionController::Update", 0x005F4B00, &Ctl_Update);
ACTCTL_REVERSED("ActionController::QueueNodeAllTracks", 0x005F4BB0, &Ctl_QueueNodeAllTracks);
ACTCTL_REVERSED("ActionNodeReference::TypeId", 0x005F4C30, &GetReferenceId);
ACTCTL_REVERSED("ActionNodeMultiParentReference::TypeId", 0x005F4C70, &GetMultiParentId);
ACTCTL_REVERSED("ActionTree::Shutdown", 0x005F4DE0, &ActionTree_Shutdown);
