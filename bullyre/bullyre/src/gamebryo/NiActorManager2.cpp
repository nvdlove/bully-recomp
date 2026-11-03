// NiActorManager (src/gamebryo/NiActorManager.h), its timing: when a transition's pieces fall
// (the blend pair to sync on, a chain step's and a whole transition's completion infos), when
// each kind of event next happens for a sequence (activated, deactivated, a text key, the end
// key), and Update, which plays the transitions and chains out. Construction, loading and the
// timeline itself are NiActorManager.cpp's.
//
// Times here are floats as the game's x87 code (24-bit precision) rounds them: each operation
// in the game's order, in float. "No time" is -FLT_MAX throughout.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiActorManager.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::actor {
namespace {

using sys::Decrement;
using sys::Increment;
using sys::kNullFixedString;
using sys::Virtual;

GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(char*, ms_kEndTextKey, 0x020E4AD0);   // NiActorManager's "end"

constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kTextKeyMatchVtable = 0x00965A60;
constexpr uintptr_t kChainCompletionInfoVtable = 0x00965A6C;
constexpr uintptr_t kCompletionInfoVtable = 0x00965AB0;
constexpr uintptr_t kStartText = 0x00965A74;     // "start": a blend pair's key when it names none
constexpr uintptr_t kMinusOne = 0x00967DE0;      // -1.0f: a chain step to its sequence's end
constexpr float kNoTime = -3.40282347e+38f;      // -FLT_MAX
constexpr double kNoTimeD = -3.4028234663852886e+38;
constexpr float kChainLead = 0.0333f;            // (a double in the game: 0.0333f's value)

GAME_FN(void*, __thiscall, ActorManager_GetSequence, 0x00800C30, NiActorManager* self, uint32_t id);
GAME_FN(void, __thiscall, ActorManager_ProcessCallbacks, 0x00800670, NiActorManager* self);
GAME_FN(void, __thiscall, ActorManager_RaiseAnimActivatedEvents, 0x00800790, NiActorManager* self, uint32_t id);
GAME_FN(void, __thiscall, ActorManager_RaiseAnimDeactivatedEvents, 0x00800890, NiActorManager* self, uint32_t id);
GAME_FN(void, __thiscall, ActorManager_BuildTimeline, 0x00803BF0, NiActorManager* self);
GAME_FN(BlendPair*, __thiscall, ActorManager_GetNextBlendPair, 0x00800F80, NiActorManager* self, uint32_t srcID,
        uint32_t desID, Transition* transition, float time, float* blendTime);
GAME_FN(ChainCompletionInfo*, __thiscall, ActorManager_FillChainComplete, 0x00802240, NiActorManager* self,
        uint32_t srcID, uint32_t desID, float duration, float* time, float* accumulated);
GAME_FN(uint8_t, __thiscall, ActorManager_FillCompletionInfo, 0x00802540, NiActorManager* self, uint32_t srcID,
        uint32_t desID, Transition* transition, float time, float end, CompletionInfo* info);
GAME_FN(float, __thiscall, ActorManager_GetNextAnimActivatedTime, 0x00802B10, NiActorManager* self, uint32_t id);
GAME_FN(float, __thiscall, ActorManager_GetNextAnimDeactivatedTime, 0x00802DE0, NiActorManager* self, uint32_t id);
GAME_FN(float, __thiscall, ActorManager_GetNextTextKeyEventTime, 0x008030E0, NiActorManager* self, uint32_t id,
        NiTextKeyMatch* match);
GAME_FN(float, __thiscall, ActorManager_GetNextEndOfSequenceTime, 0x00803520, NiActorManager* self, uint32_t id);
GAME_FN(CompletionInfo*, __thiscall, CompletionInfo_Construct, 0x008021A0, CompletionInfo* self);
GAME_FN(void, __thiscall, CompletionInfo_Destruct, 0x008021D0, CompletionInfo* self);
GAME_FN(void, __thiscall, TextKeyMatch_Destruct, 0x00800B20, NiTextKeyMatch* self);
GAME_FN(Transition*, __thiscall, KFMTool_GetTransition, 0x00801160, NiKFMTool* self, uint32_t srcID, uint32_t desID);
GAME_FN(uint32_t, __thiscall, KFMTool_IsValidTransition, 0x008011B0, NiKFMTool* self, uint32_t srcID,
        uint32_t desID, uint8_t* valid);
// NiControllerSequence's (another chunk's)
GAME_FN(float, __thiscall, Sequence_GetKeyTimeAt, 0x008145E0, void* self, const re::Ptr32<char>* key);
GAME_FN(float, __thiscall, Sequence_GetTimeAt, 0x00815C40, void* self, const re::Ptr32<char>* key, float time);
GAME_FN(float, __thiscall, Sequence_GetTimeAtMatch, 0x008144F0, void* self, NiTextKeyMatch* match, float time);
GAME_FN(float, __thiscall, Sequence_GetKeyTimeAtMatch, 0x00813850, void* self, NiTextKeyMatch* match);
GAME_FN(float, __thiscall, Sequence_FindCorrespondingMorphFrame, 0x00814CA0, void* self, void* source, float time);
GAME_FN(uint8_t, __thiscall, Sequence_Activate, 0x00814940, void* self, int priority, int startOver, float weight,
        float easeIn, void* timeSync, int transition);
GAME_FN(uint8_t, __thiscall, Sequence_Deactivate, 0x00814A20, void* self, float easeOut, int transition);
GAME_FN(void, __thiscall, Sequence_Update, 0x00816320, void* self, float time, int updateInterpolators);
GAME_FN(float, __thiscall, Sequence_ComputeScaledTime, 0x00813A90, void* self, float time, int storeLastTime);
// NiControllerManager's
GAME_FN(uint8_t, __thiscall, Manager_BlendFromSequence, 0x008003E0, void* self, void* source, void* destination,
        float duration, const re::Ptr32<char>* destinationKey, int priority, float sourceWeight, float destinationWeight,
        void* timeSync);
GAME_FN(uint8_t, __thiscall, Manager_Morph, 0x008182E0, void* self, void* source, void* destination, float duration,
        int priority, float sourceWeight, float destinationWeight);
GAME_FN(uint8_t, __thiscall, Manager_CrossFade, 0x00818350, void* self, void* source, void* destination,
        float duration, int priority, int startOver, float weight, void* timeSync);
// the containers and strings
GAME_FN(void, __thiscall, ListBase_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, PointerList_AddTail, 0x00748170, void* list, const re::Ptr32<void>* element);
GAME_FN(void, __thiscall, FloatList_AddTail, 0x00800990, void* list, const float* value);
GAME_FN(uint32_t, __thiscall, UintList_RemoveHead, 0x00800360, void* list);
GAME_FN(float, __thiscall, FloatList_RemoveHead, 0x00800390, void* list);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, void* set, const re::Ptr32<NiRefObject>* element);
GAME_FN(void, __cdecl, Array_Free, 0x00836F80, void* p);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(float, __cdecl, General_Floor, 0x00432910, float x);   // CGeneral::Floor
GAME_FN(float, __cdecl, General_Ceil, 0x00450BA0, float x);

inline void ReleaseImport(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

inline uint32_t SeqCycleType(void* sequence) {
    return SeqField<uint32_t>(sequence, 0x24);
}
inline uint32_t SeqState(void* sequence) {
    return SeqField<uint32_t>(sequence, 0x44);
}
inline float SeqLength(void* sequence) {
    return SeqEndKeyTime(sequence) - SeqBeginKeyTime(sequence);
}

// a chain step's frames in its sequence: -1 to the end (from where it starts), else scaled
inline float StepFrames(float duration, void* sequence, float frame) {
    if (duration == -1.0f) {
        return SeqLength(sequence) - frame;
    }
    return duration * SeqFrequency(sequence);
}

// a chain step's info held in the completion info's set
inline void AddChainInfo(CompletionInfo* info, ChainCompletionInfo* chain) {
    re::Ptr32<NiRefObject> held = chain;
    if (chain != nullptr) {
        Increment(&chain->refCount);
    }
    ObjectSet_Add(&info->chainBase, &held);
    ReleaseImport(chain);
}

// the extra (still easing) sequences' part of the event times: found there, the sequence's end
// time while it's in the given state; an inactive one taken off the list. False when the
// sequence isn't one of them
bool ExtraSequenceTime(NiActorManager* self, uint32_t id, uint32_t easingState, float* result) {
    uint32_t index = 0;
    for (; index < self->extraCount; index++) {
        if (self->extraSequences[index] == id) {
            break;
        }
    }
    if (index >= self->extraCount) {
        return false;
    }
    void* sequence = ActorManager_GetSequence(self, id);
    uint32_t state = SeqState(sequence);
    if (state == 0) {
        self->extraCount--;
        self->extraSequences[index] = self->extraSequences[self->extraCount];
        *result = kNoTime;
        return true;
    }
    if (state != easingState) {
        *result = kNoTime;
        return true;
    }
    float endTime = SeqField<float>(sequence, 0x60);
    *result = self->time < endTime ? endTime : kNoTime;
    return true;
}

// the chain list searched for a sequence
inline bool InChain(NiActorManager* self, uint32_t id) {
    for (NiTListNode<uint32_t>* node = self->chainIDs.head; node != nullptr; node = node->next) {
        if (id == node->element) {
            return true;
        }
    }
    return false;
}

// the transition under way worked out (its start frame -FLT_MAX: 0)
inline Transition* FillCurrent(NiActorManager* self, CompletionInfo* info) {
    Transition* transition = KFMTool_GetTransition(self->kfmTool, self->curID, self->nextID);
    float startFrame = self->transStartFrameTime;
    if (double(startFrame) == kNoTimeD) {
        startFrame = 0.0f;
    }
    CompletionInfo_Construct(info);
    ActorManager_FillCompletionInfo(self, self->curID, self->nextID, transition, startFrame, startFrame, info);
    return transition;
}

// 0x800F80 GetNextBlendPair(source, destination (unused), transition, time, blend time out): of a
// blend's pairs, the one whose source key comes next from the time (wrapped into the source's
// range unless it clamps); the time plus the wait out. None for no pairs or an empty first one
BlendPair* __fastcall GetNextBlendPair(NiActorManager* self, re::Edx, uint32_t srcID, uint32_t, Transition* transition,
                                       float time, float* blendTime) {
    if (transition->type != 0 || transition->blendPairCount == 0 ||
        transition->blendPairs[0]->startKey == kNullFixedString) {
        return nullptr;
    }
    void* source = ActorManager_GetSequence(self, srcID);
    float end = SeqEndKeyTime(source);
    float length = end - SeqBeginKeyTime(source);
    *blendTime = time;
    if (time > end) {
        if (SeqCycleType(source) == 2) {
            return nullptr;
        }
        float ratio = time / length;
        float whole = General_Floor(ratio) * length;
        time = time - whole;
    }
    float best = kMaxFloat;
    BlendPair* bestPair = nullptr;
    for (uint32_t i = 0; i < transition->blendPairCount; i++) {
        BlendPair* pair = transition->blendPairs[i];
        float delta = Sequence_GetKeyTimeAt(source, &pair->startKey) - time;
        if (delta < 0.0f) {
            delta = delta + length;
        }
        if (best > delta) {
            best = delta;
            bestPair = pair;
        }
    }
    *blendTime = *blendTime + best;
    return bestPair;
}

// 0x802240 FillChainComplete(source, destination, duration, frame in/out, accumulated in/out):
// one chain step's timing: its transition worked out from the frame to the duration's end (-1:
// the source's end, or the frame when that's later), the step's info made; the accumulated time
// moved on by the transition, the frame the destination's start frame. The local completion info
// is let go as the game inlines it (its set's array freed, its entries not released)
ChainCompletionInfo* __fastcall FillChainComplete(NiActorManager* self, re::Edx, uint32_t srcID, uint32_t desID,
                                                  float duration, float* time, float* accumulated) {
    void* source = MapLookup(self->sequenceMap, srcID);
    void* destination = MapLookup(self->sequenceMap, desID);
    Transition* transition = KFMTool_GetTransition(self->kfmTool, srcID, desID);
    float end;
    if (duration == -1.0f) {
        end = SeqEndKeyTime(source);
        if (*time > end) {
            end = *time;
        }
    } else {
        end = duration + *time;
    }
    CompletionInfo info;
    info.vtable = reinterpret_cast<void*>(kRefObjectVtable);
    info.refCount = 0;
    Increment(&ms_uiObjects);
    info.vtable = reinterpret_cast<void*>(kCompletionInfoVtable);
    info.chainBase = nullptr;
    info.chainAllocated = 0;
    info.chainCount = 0;
    ActorManager_FillCompletionInfo(self, srcID, desID, transition, *time, end, &info);
    auto* chain = static_cast<ChainCompletionInfo*>(NiNew(0x2C));
    if (chain != nullptr) {
        chain->vtable = reinterpret_cast<void*>(kRefObjectVtable);
        chain->refCount = 0;
        Increment(&ms_uiObjects);
        chain->vtable = reinterpret_cast<void*>(kChainCompletionInfoVtable);
        chain->srcName = kNullFixedString;
        chain->desName = kNullFixedString;
    }
    FixedString_Assign(&chain->srcName, &SeqField<char*>(source, 8));
    chain->time = *accumulated;
    chain->srcFrame = *time;
    float total = *accumulated + info.duration;
    *accumulated = total;
    chain->desStart = total - transition->duration;
    chain->desEnd = *accumulated;
    chain->transition = transition;
    chain->srcID = srcID;
    chain->desID = desID;
    FixedString_Assign(&chain->desName, &SeqField<char*>(destination, 8));
    *time = info.desFrame;
    Array_Free(info.chainBase);
    info.vtable = reinterpret_cast<void*>(kRefObjectVtable);
    Decrement(&ms_uiObjects);
    return chain;
}

// 0x802540 FillCompletionInfo(source, destination, transition, time, end, info): the
// transition's timing from the time (in the source) to the end, by kind: a blend's sync frames
// and the wait for them, a morph's corresponding frame, a crossfade's scaled length (both
// wrapped into the destination's range), a chain's steps; false for another kind
uint8_t __fastcall FillCompletionInfo(NiActorManager* self, re::Edx, uint32_t srcID, uint32_t desID,
                                      Transition* transition, float time, float end, CompletionInfo* info) {
    void* source = MapLookup(self->sequenceMap, srcID);
    void* destination = MapLookup(self->sequenceMap, desID);
    switch (transition->type) {
    case 0: {
        uint32_t count = transition->blendPairCount;
        if (count == 0 || (count == 1 && transition->blendPairs[0]->startKey == kNullFixedString)) {
            float delta = end - time;
            const char* key = reinterpret_cast<const char*>(kStartText);
            if (count == 1) {
                key = transition->blendPairs[0]->targetKey;
            }
            re::Ptr32<char> text;
            FixedString_Construct(&text, key);
            info->desFrame = Sequence_GetKeyTimeAt(destination, &text);
            FixedString_Destruct(&text);
            float scaled = delta / SeqFrequency(source);
            info->duration = scaled + transition->duration;
            info->srcEnd = end;
            return 1;
        }
        float blendTime = 0.0f;
        BlendPair* pair = ActorManager_GetNextBlendPair(self, srcID, desID, transition, end, &blendTime);
        info->blendPair = pair;
        if (pair == nullptr) {
            info->srcEnd = SeqEndKeyTime(source);
            info->desFrame = SeqBeginKeyTime(destination);
            float delta = blendTime - time;
            float scaled = delta / SeqFrequency(source);
            info->duration = scaled + transition->duration;
            return 1;
        }
        const char* startKey = pair->startKey != nullptr ? pair->startKey : reinterpret_cast<const char*>(kStartText);
        re::Ptr32<char> startText;
        FixedString_Construct(&startText, startKey);
        float srcFrame = Sequence_GetKeyTimeAt(source, &startText);
        FixedString_Destruct(&startText);
        info->srcEnd = srcFrame;
        const char* targetKey =
            pair->targetKey != nullptr ? pair->targetKey : reinterpret_cast<const char*>(kStartText);
        re::Ptr32<char> targetText;
        FixedString_Construct(&targetText, targetKey);
        info->desFrame = Sequence_GetKeyTimeAt(destination, &targetText);
        FixedString_Destruct(&targetText);
        float delta = blendTime - time;
        float scaled = delta / SeqFrequency(source);
        info->duration = scaled + transition->duration;
        return 1;
    }
    case 1: {
        info->srcEnd = end;
        float elapsed = end - time;
        float scaled = elapsed / SeqFrequency(source);
        info->duration = scaled + transition->duration;
        float frame = Sequence_FindCorrespondingMorphFrame(destination, source, end / SeqFrequency(source));
        float remaining = SeqEndKeyTime(destination) - frame;
        float duration = transition->duration;
        float frames = duration == -1.0f ? SeqLength(destination) - time : duration * SeqFrequency(destination);
        if (remaining < frames) {
            float length = SeqLength(destination);
            float over = frames - remaining;
            float cycles = General_Ceil(over / length) * length;
            float wrapped = (length - frame) + cycles;
            info->desFrame = length - (wrapped - frames);
            return 1;
        }
        info->desFrame = frame + frames;
        return 1;
    }
    case 2: {
        info->srcEnd = end;
        float duration = transition->duration;
        float elapsed = end - time;
        float scaled = elapsed / SeqFrequency(source);
        info->duration = scaled + duration;
        float length = SeqLength(destination);
        float frames = duration * SeqFrequency(destination);
        if (length < frames) {
            if (SeqCycleType(destination) == 2) {
                info->desFrame = length;
                return 1;
            }
            float cycles = General_Ceil(frames / length) * length;
            info->desFrame = length - (cycles - frames);
            return 1;
        }
        info->desFrame = frames;
        return 1;
    }
    case 3: {
        float accumulated = 0.0f;
        float frame = SeqBeginKeyTime(source);
        uint32_t count = transition->chainCount;
        float duration = end;
        uint32_t previous = srcID;
        for (uint32_t i = 0; i < count; i++) {
            ChainInfo* step = &transition->chain[i];
            uint32_t id = step->sequenceID;
            void* sequence = ActorManager_GetSequence(self, id);
            AddChainInfo(info, ActorManager_FillChainComplete(self, previous, id, duration, &frame, &accumulated));
            float stepDuration = step->duration;
            previous = id;
            duration = StepFrames(stepDuration, sequence, frame);
        }
        AddChainInfo(info, ActorManager_FillChainComplete(self, previous, desID, duration, &frame, &accumulated));
        float desEnd = SeqEndKeyTime(destination) / SeqFrequency(destination);
        float lastFrame = frame / SeqFrequency(ActorManager_GetSequence(self, previous));
        info->chainTotal = (desEnd - lastFrame) + accumulated;
        return 1;
    }
    default:
        return 0;
    }
}

// the transition under way and the chain after it walked to a sequence's start: the time it
// activates, or -FLT_MAX when it isn't on the way
float WalkToActivation(NiActorManager* self, uint32_t id) {
    float accumulated = self->transEndTime;
    CompletionInfo info;
    FillCurrent(self, &info);
    float end = 0.0f;
    uint32_t previous = self->nextID;
    NiTListNode<uint32_t>* idNode = self->chainIDs.head;
    NiTListNode<float>* durationNode = self->chainDurations.head;
    if (self->chainDurations.count != 0) {
        end = info.desFrame + StepFrames(durationNode->element, self->nextSequence, info.desFrame);
        idNode = idNode != nullptr ? idNode->next : nullptr;
        durationNode = durationNode != nullptr ? durationNode->next : nullptr;
    }
    if (id != previous) {
        while (true) {
            if (idNode == nullptr) {
                CompletionInfo_Destruct(&info);
                return kNoTime;
            }
            uint32_t chainID = idNode->element;
            float duration = durationNode->element;
            idNode = idNode->next;
            durationNode = durationNode->next;
            void* sequence = ActorManager_GetSequence(self, chainID);
            Transition* step = KFMTool_GetTransition(self->kfmTool, previous, chainID);
            ActorManager_FillCompletionInfo(self, previous, chainID, step, info.desFrame, end, &info);
            accumulated = info.duration + accumulated;
            previous = chainID;
            end = info.desFrame + StepFrames(duration, sequence, info.desFrame);
            if (id == chainID) {
                break;
            }
        }
    }
    CompletionInfo_Destruct(&info);
    return accumulated;
}

// 0x802B10 GetNextAnimActivatedTime(sequence): an easing-in extra one's end; the next
// sequence's transition end; a chained one's start along the chain; else -FLT_MAX
float __fastcall GetNextAnimActivatedTime(NiActorManager* self, re::Edx, uint32_t id) {
    float extra;
    if (ExtraSequenceTime(self, id, 2, &extra)) {
        return extra;
    }
    if (id == self->curID) {
        return kNoTime;
    }
    uint32_t nextID = self->nextID;
    if (id == nextID) {
        return self->transEndTime;
    }
    if (nextID == kInvalidSequenceID || !InChain(self, id)) {
        return kNoTime;
    }
    return WalkToActivation(self, id);
}

// 0x802DE0 GetNextAnimDeactivatedTime(sequence): an easing-out extra one's end; the current
// one's transition start (unless that's passed); a chained one's end along the chain; else
// -FLT_MAX
float __fastcall GetNextAnimDeactivatedTime(NiActorManager* self, re::Edx, uint32_t id) {
    float extra;
    if (ExtraSequenceTime(self, id, 3, &extra)) {
        return extra;
    }
    uint32_t curID = self->curID;
    if (id == curID) {
        if (self->nextID == kInvalidSequenceID || self->transStartTime < self->time) {
            return kNoTime;
        }
        return self->transStartTime;
    }
    uint32_t nextID = self->nextID;
    if (nextID == kInvalidSequenceID || (id == nextID && self->chainIDs.count == 0) || !InChain(self, id)) {
        return kNoTime;
    }
    float accumulated = self->transStartTime;
    CompletionInfo info;
    Transition* transition = FillCurrent(self, &info);
    float frame = info.desFrame;
    float end = 0.0f;
    float transitionDuration = transition->duration;
    uint32_t previous = self->nextID;
    NiTListNode<uint32_t>* idNode = self->chainIDs.head;
    NiTListNode<float>* durationNode = self->chainDurations.head;
    if (self->chainDurations.count != 0) {
        end = info.desFrame + StepFrames(durationNode->element, self->nextSequence, info.desFrame);
        idNode = idNode != nullptr ? idNode->next : nullptr;
        durationNode = durationNode != nullptr ? durationNode->next : nullptr;
    }
    while (idNode != nullptr) {
        uint32_t chainID = idNode->element;
        float duration = durationNode->element;
        idNode = idNode->next;
        durationNode = durationNode->next;
        void* sequence = ActorManager_GetSequence(self, chainID);
        Transition* step = KFMTool_GetTransition(self->kfmTool, previous, chainID);
        ActorManager_FillCompletionInfo(self, previous, chainID, step, info.desFrame, end, &info);
        accumulated = ((info.srcEnd - frame) + transitionDuration) + accumulated;
        if (id == previous) {
            CompletionInfo_Destruct(&info);
            return accumulated;
        }
        previous = chainID;
        frame = info.desFrame;
        end = info.desFrame + StepFrames(duration, sequence, info.desFrame);
        transitionDuration = step->duration;
    }
    CompletionInfo_Destruct(&info);
    return kNoTime;
}

// 0x8030E0 GetNextTextKeyEventTime(sequence, match): when the sequence next reaches a key the
// match accepts: an extra or the current one's own time for it (not past, not after an easing
// out's end, the current one's not after its transition starts nor its end key once reached), a
// coming one's along the transitions and chain; else -FLT_MAX
float __fastcall GetNextTextKeyEventTime(NiActorManager* self, re::Edx, uint32_t id, NiTextKeyMatch* match) {
    uint32_t index = 0;
    for (; index < self->extraCount; index++) {
        if (self->extraSequences[index] == id) {
            break;
        }
    }
    if (index < self->extraCount) {
        void* sequence = ActorManager_GetSequence(self, id);
        if (SeqState(sequence) == 0) {
            self->extraCount--;
            self->extraSequences[index] = self->extraSequences[self->extraCount];
            return kNoTime;
        }
        float time = Sequence_GetTimeAtMatch(sequence, match, self->time);
        if (time == kNoTime || self->time > time) {
            return kNoTime;
        }
        if (SeqState(sequence) != 3 || !(SeqField<float>(sequence, 0x60) < time)) {
            return time;
        }
        return kNoTime;
    }
    uint32_t curID = self->curID;
    if (id == curID) {
        float time = Sequence_GetTimeAtMatch(self->curSequence, match, self->time);
        if (time == kNoTime || self->time > time) {
            return kNoTime;
        }
        if (time == self->time) {
            float endKey = SeqEndKeyTime(self->curSequence);
            if (endKey == Sequence_GetKeyTimeAtMatch(self->curSequence, match)) {
                return kNoTime;
            }
        }
        if (self->nextID == kInvalidSequenceID || !(self->transStartTime < time)) {
            return time;
        }
        return kNoTime;
    }
    uint32_t nextID = self->nextID;
    if (nextID == kInvalidSequenceID) {
        return kNoTime;
    }
    if (id != nextID && (self->chainIDs.count == 0 || !InChain(self, id))) {
        return kNoTime;
    }
    float accumulated = self->transEndTime;
    CompletionInfo info;
    FillCurrent(self, &info);
    float end = 0.0f;
    uint32_t previous = self->nextID;
    void* sequence = self->nextSequence;
    NiTListNode<uint32_t>* idNode = self->chainIDs.head;
    NiTListNode<float>* durationNode = self->chainDurations.head;
    if (self->chainDurations.count != 0) {
        end = StepFrames(durationNode->element, sequence, info.desFrame) + info.desFrame;
        idNode = idNode != nullptr ? idNode->next : nullptr;
        durationNode = durationNode != nullptr ? durationNode->next : nullptr;
    }
    if (id != previous) {
        while (true) {
            if (idNode == nullptr) {
                CompletionInfo_Destruct(&info);
                return kNoTime;
            }
            uint32_t chainID = idNode->element;
            float duration = durationNode->element;
            sequence = MapLookup(self->sequenceMap, chainID);
            idNode = idNode->next;
            durationNode = durationNode->next;
            Transition* step = KFMTool_GetTransition(self->kfmTool, previous, chainID);
            ActorManager_FillCompletionInfo(self, previous, chainID, step, info.desFrame, end, &info);
            accumulated = info.duration + accumulated;
            previous = chainID;
            end = StepFrames(duration, sequence, info.desFrame) + info.desFrame;
            if (id == chainID) {
                break;
            }
        }
    }
    float keyTime = Sequence_GetKeyTimeAtMatch(sequence, match);
    if (keyTime == kNoTime || (idNode != nullptr && end < keyTime)) {
        CompletionInfo_Destruct(&info);
        return kNoTime;
    }
    float frames = keyTime - info.desFrame;
    if (frames < 0.0f) {
        frames = frames + SeqLength(sequence);
    }
    float frequency = SeqFrequency(sequence);
    CompletionInfo_Destruct(&info);
    float scaled = frames / frequency;
    return scaled + accumulated;
}

// 0x803520 GetNextEndOfSequenceTime(sequence): the text key time of "end" (a local match)
float __fastcall GetNextEndOfSequenceTime(NiActorManager* self, re::Edx, uint32_t id) {
    NiTextKeyMatch match;
    match.vtable = reinterpret_cast<void*>(kRefObjectVtable);
    match.refCount = 0;
    Increment(&ms_uiObjects);
    match.vtable = reinterpret_cast<void*>(kTextKeyMatchVtable);
    match.lastMatchedKey = kNullFixedString;
    match.matchKey = kNullFixedString;
    FixedString_Assign(&match.matchKey, &ms_kEndTextKey);
    float time = ActorManager_GetNextTextKeyEventTime(self, id, &match);
    TextKeyMatch_Destruct(&match);
    return time;
}

// 0x803900 GetNextEventTime(event type, sequence, match): by type (0 activated, 1 deactivated,
// 2 text key, 3 end of sequence); -FLT_MAX for another
float __fastcall GetNextEventTime(NiActorManager* self, re::Edx, uint32_t eventType, uint32_t id, NiTextKeyMatch* match) {
    switch (eventType) {
    case 0:
        return ActorManager_GetNextAnimActivatedTime(self, id);
    case 1:
        return ActorManager_GetNextAnimDeactivatedTime(self, id);
    case 2:
        return ActorManager_GetNextTextKeyEventTime(self, id, match);
    case 3:
        return ActorManager_GetNextEndOfSequenceTime(self, id);
    default:
        return kNoTime;
    }
}

// 0x803D90 Update(time): the callbacks run; a transition under way started once its sync time
// comes (a pending blend) and finished once it ends; else the timeline rebuilt when due, the
// chain moved on (its next step's time worked out: a -1 step a frame (0.0333) before its end
// key), and a new target played: activated from nothing, deactivated to nothing, or the
// transition to it (a chain's steps listed first) started: a blend at its next sync key (pending
// until then), a morph, a crossfade
void __fastcall Update(NiActorManager* self, re::Edx, float time) {
    self->time = time;
    ActorManager_ProcessCallbacks(self);
    uint32_t state = self->transitionState;
    if (state != 0) {
        if (self->startPending != 0 && self->transStartTime <= self->time) {
            if (state == 1) {
                float duration = self->transEndTime - self->transStartTime;
                re::Ptr32<char> key;
                FixedString_Construct(&key, self->startTextKey);
                Manager_BlendFromSequence(self->manager, self->curSequence, self->nextSequence, duration, &key, 0,
                                          1.0f, 1.0f, nullptr);
                FixedString_Destruct(&key);
                float now = self->time;
                self->transStartTime = now;
                self->transEndTime = now + duration;
            }
            self->startPending = 0;
        }
        if (!(self->transEndTime < self->time)) {
            return;
        }
        uint32_t nextID = self->nextID;
        void* next = self->nextSequence;
        self->transitionState = 0;
        self->nextSequence = nullptr;
        self->curID = nextID;
        self->curSequence = next;
        self->nextID = kInvalidSequenceID;
        self->rebuildTimeline = 1;
        return;
    }
    if (self->nextChangeTime < self->time) {
        self->rebuildTimeline = 0;
        ActorManager_BuildTimeline(self);
    }
    uint32_t target;
    if (self->chainActive != 0) {
        if (double(self->chainTime) == kNoTimeD) {
            target = UintList_RemoveHead(&self->chainIDs);
            float duration = FloatList_RemoveHead(&self->chainDurations);
            if (self->chainIDs.count != 0) {
                if (duration == -1.0f) {
                    void* sequence = ActorManager_GetSequence(self, target);
                    float left = Sequence_GetTimeAt(sequence, &ms_kEndTextKey, self->time) - self->time;
                    left = left - kChainLead;
                    if (0.0f > left) {
                        left = 0.0f;
                    }
                    self->chainTime = left + self->time;
                    return;
                }
                self->chainTime = duration + self->time;
                return;
            }
            self->chainTime = kNoTime;
            self->chainActive = 0;
        } else {
            if (!(self->chainTime < self->time)) {
                return;
            }
            target = self->chainIDs.head->element;
            self->chainTime = kNoTime;
        }
    } else {
        target = self->targetID;
    }
    uint32_t curID = self->curID;
    if (target == curID) {
        if (self->rebuildTimeline != 0) {
            self->rebuildTimeline = 0;
            ActorManager_BuildTimeline(self);
        }
        return;
    }
    if (target == kInvalidSequenceID) {
        ActorManager_RaiseAnimDeactivatedEvents(self, curID);
        void* current = self->curSequence;
        self->curID = target;
        if (current == nullptr) {
            return;
        }
        Sequence_Deactivate(current, 0.0f, 0);
        self->curSequence = nullptr;
        return;
    }
    if (curID == kInvalidSequenceID) {
        void* sequence = ActorManager_GetSequence(self, target);
        Sequence_Activate(sequence, 0, 1, 1.0f, 0.0f, nullptr, 0);
        self->curID = target;
        self->curSequence = sequence;
        ActorManager_RaiseAnimActivatedEvents(self, target);
        Virtual<void>(self->manager, 20, self->time);   // NiControllerManager::Update
        self->rebuildTimeline = 0;
        ActorManager_BuildTimeline(self);
        ActorManager_ProcessCallbacks(self);
        return;
    }
    uint8_t valid = 0;
    KFMTool_IsValidTransition(self->kfmTool, curID, target, &valid);
    if (valid != 0) {
        void* destination = ActorManager_GetSequence(self, target);
        Transition* transition = KFMTool_GetTransition(self->kfmTool, self->curID, target);
        if (transition->type == 3) {
            ListBase_RemoveAll(&self->chainIDs);
            ListBase_RemoveAll(&self->chainDurations);
            uint32_t count = transition->chainCount;
            for (uint32_t i = 0; i < count; i++) {
                ChainInfo* step = &transition->chain[i];
                float duration = step->duration;
                uint32_t id = step->sequenceID;
                PointerList_AddTail(&self->chainIDs, re::PtrCast<const re::Ptr32<void>*>(&id));
                FloatList_AddTail(&self->chainDurations, &duration);
            }
            PointerList_AddTail(&self->chainIDs, re::PtrCast<const re::Ptr32<void>*>(&target));
            FloatList_AddTail(&self->chainDurations, reinterpret_cast<const float*>(kMinusOne));
            target = self->chainIDs.head->element;
            destination = ActorManager_GetSequence(self, target);
            transition = KFMTool_GetTransition(self->kfmTool, self->curID, target);
            self->chainTime = kNoTime;
            self->chainActive = 1;
        }
        self->transStartTime = self->time;
        self->startTextKey = nullptr;
        if (transition->type == 0 && transition->blendPairCount != 0) {
            Sequence_Update(self->curSequence, self->time, 0);
            self->transStartTime = kMaxFloat;
            for (uint32_t i = 0; i < transition->blendPairCount; i++) {
                float now = self->time;
                BlendPair* pair = transition->blendPairs[i];
                if (pair->startKey == nullptr) {
                    self->transStartTime = now;
                    self->startTextKey = pair->targetKey;
                    break;
                }
                float syncTime = Sequence_GetTimeAt(self->curSequence, &pair->startKey, now);
                if (syncTime != kNoTime && self->time <= syncTime && self->transStartTime > syncTime) {
                    self->transStartTime = syncTime;
                    self->startTextKey = pair->targetKey;
                }
            }
        }
        self->transEndTime = transition->duration + self->transStartTime;
        Sequence_Update(destination, self->time, 0);
        switch (transition->type) {
        case 0:
            self->transStartFrameTime = SeqField<float>(self->curSequence, 0x3C);
            if (self->time == self->transStartTime) {
                re::Ptr32<char> key;
                FixedString_Construct(&key, self->startTextKey);
                Manager_BlendFromSequence(self->manager, self->curSequence, destination, transition->duration, &key,
                                          0, 1.0f, 1.0f, nullptr);
                FixedString_Destruct(&key);
            } else {
                float delay = self->transStartTime - self->time;
                float frames = Sequence_ComputeScaledTime(self->curSequence, delay, 0);
                self->startPending = 1;
                self->transStartFrameTime = frames + self->transStartFrameTime;
            }
            self->transitionState = 1;
            break;
        case 1:
            self->transStartFrameTime = SeqField<float>(self->curSequence, 0x3C);
            Manager_Morph(self->manager, self->curSequence, destination, transition->duration, 0, 1.0f, 1.0f);
            self->transitionState = 2;
            break;
        case 2:
            self->transStartFrameTime = SeqField<float>(self->curSequence, 0x3C);
            Manager_CrossFade(self->manager, self->curSequence, destination, transition->duration, 0, 1, 1.0f,
                              nullptr);
            self->transitionState = 3;
            break;
        default:
            break;
        }
        self->nextID = target;
        self->nextSequence = destination;
        self->rebuildTimeline = 1;
    }
    if (self->rebuildTimeline != 0) {
        self->rebuildTimeline = 0;
        ActorManager_BuildTimeline(self);
    }
}

} // namespace
} // namespace ni::actor

// ---- registration ---------------------------------------------------------------------

#define NIACTORMGR2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

using namespace ni::actor;

NIACTORMGR2_REVERSED("NiActorManager::GetNextBlendPair", 0x00800F80, GetNextBlendPair);
NIACTORMGR2_REVERSED("NiActorManager::FillChainComplete", 0x00802240, FillChainComplete);
NIACTORMGR2_REVERSED("NiActorManager::FillCompletionInfo", 0x00802540, FillCompletionInfo);
NIACTORMGR2_REVERSED("NiActorManager::GetNextAnimActivatedTime", 0x00802B10, GetNextAnimActivatedTime);
NIACTORMGR2_REVERSED("NiActorManager::GetNextAnimDeactivatedTime", 0x00802DE0, GetNextAnimDeactivatedTime);
NIACTORMGR2_REVERSED("NiActorManager::GetNextTextKeyEventTime", 0x008030E0, GetNextTextKeyEventTime);
NIACTORMGR2_REVERSED("NiActorManager::GetNextEndOfSequenceTime", 0x00803520, GetNextEndOfSequenceTime);
NIACTORMGR2_REVERSED("NiActorManager::GetNextEventTime", 0x00803900, GetNextEventTime);
NIACTORMGR2_REVERSED("NiActorManager::Update", 0x00803D90, Update);
