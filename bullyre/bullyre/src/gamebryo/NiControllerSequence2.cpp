// NiControllerSequence (src/gamebryo/NiControllerSequence.h), the second part: activation and
// deactivation (eases, transitions, synchronisation to a partner), blends and morphs between two
// sequences, the morph frame correspondence ("morph:" text keys), the blend weights each update,
// the multi-target transform controller made for the transform controllers, cloning, the IDTag
// and interp item streaming. The rest: src/gamebryo/NiControllerSequence.cpp and 3.
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's order.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiControllerSequence.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::LoadBytes;
using scene::Stream_LoadFixedString;
using scene::Stream_LoadInlineFixedString;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiMultiTargetTransformController_RTTI, 0x020E4D58);
GAME_VAR(RTTI, NiTransformController_RTTI, 0x020E4EF8);
GAME_VAR(uint16_t, kInvalidIndex, 0x00967FA0);   // NiInterpController::INVALID_INDEX

constexpr uintptr_t NiMultiTargetTransformController_vtable = 0x00966290;
constexpr uint32_t kVersion_10_1_0_111 = 0x0A01006F;
constexpr uint32_t kVersion_10_1_0_113 = 0x0A010071;
constexpr uint32_t kVersion_20_1_0_1 = 0x14010001;

GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(void, __thiscall, Object_ProcessClone, 0x007553F0, void* self, void* cloning);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* self, NiRefObject* object);
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, Pointer_AssignFrom, 0x00518100, re::Ptr32<NiRefObject>* self, const re::Ptr32<NiRefObject>* other);
GAME_FN(NiTimeController*, __thiscall, ObjectNET_GetController, 0x007647D0, void* self, const RTTI* rtti);
GAME_FN(void, __thiscall, ObjectNET_RemoveController, 0x007646E0, void* self, NiTimeController* controller);
GAME_FN(void, __thiscall, TimeController_SetNext, 0x00764500, NiTimeController* self, NiTimeController* next);
GAME_FN(void, __thiscall, InterpController_Construct, 0x00824F10, void* self);
GAME_FN(void, __thiscall, MultiTarget_SetNumInterps, 0x0080C5E0, NiMultiTargetTransformController* self,
        uint16_t count);
// (this chunk's, called where modes apply)
GAME_FN(uint16_t, __thiscall, MultiTarget_AddInterpolatorTarget, 0x00813800, NiMultiTargetTransformController* self,
        NiAVObject* target);
GAME_FN(uint8_t, __thiscall, Sequence_VerifyDependencies, 0x008137D0, NiControllerSequence* self,
        NiControllerSequence* sequence);
GAME_FN(void, __thiscall, Sequence_AttachInterpolators, 0x008139C0, NiControllerSequence* self, int priority);
GAME_FN(void, __thiscall, Sequence_DetachInterpolators, 0x00813A20, NiControllerSequence* self);
GAME_FN(float, __thiscall, Sequence_ComputeScaledTime, 0x00813A90, NiControllerSequence* self, float time,
        int storeLastTime);
GAME_FN(uint8_t, __thiscall, Sequence_CanSyncTo, 0x00813BC0, NiControllerSequence* self, NiControllerSequence* partner);
GAME_FN(uint8_t, __thiscall, Sequence_Activate, 0x00814940, NiControllerSequence* self, int priority, int startOver,
        float weight, float easeInTime, NiControllerSequence* timeSync, int transition);
GAME_FN(uint8_t, __thiscall, Sequence_Deactivate, 0x00814A20, NiControllerSequence* self, float easeOutTime,
        int transition);

constexpr float kMinDuration = 0.0001f;   // (0x90CF18)

inline void FixedRelease(re::Ptr32<const char>* handle) {
    sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(handle));
}

// a reference held by the item arrays' code (the import's increment)
inline void Hold(void* object) {
    if (object != nullptr) {
        sys::Increment(&static_cast<NiRefObject*>(object)->refCount);
    }
}

// the callbacks told of the state (slot 1: ActivationChanged(sequence, state))
inline void NotifyCallbacks(NiControllerSequence* self) {
    uint32_t count = self->callbacks.size;
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(self->callbacks.base[i], 1, self, uint32_t(self->state));
    }
}

// the clone of an object in the cloning's map (NiTPointerMap<NiObject*, NiObject*>), or null
inline const re::Ptr32<void>* FindClone(void* cloning, const void* object) {
    uint8_t* map = *static_cast<re::Ptr32<uint8_t>*>(cloning);
    uint32_t count = *reinterpret_cast<uint32_t*>(map + 4);
    re::Ptr32<void>* buckets = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(map + 8);
    for (re::Ptr32<void>* node = static_cast<re::Ptr32<void>*>(buckets[reinterpret_cast<uintptr_t>(object) % count]); node != nullptr;
         node = static_cast<re::Ptr32<void>*>(node[0])) {
        if (node[1] == object) {
            return &node[2];
        }
    }
    return nullptr;
}

// ---- NiControllerSequence

// 0x814670: SetupMultiTargetController(root, palette): the root's NiMultiTargetTransformController
// (one made, a target slot per transform controller tag, when it has none: manager controlled,
// moved after the manager in its root's chain); then each transform controller's target taken
// into it (unless it's the accumulation root of a cumulative manager), its own transform
// controller removed, the item driven through the multi-target's blend interpolator. True when
// one was made
uint8_t __fastcall Seq_SetupMultiTargetController(NiControllerSequence* self, re::Edx, NiAVObject* root, void* palette) {
    uint8_t made = 0;
    auto* multi = reinterpret_cast<NiMultiTargetTransformController*>(
        ObjectNET_GetController(root, reinterpret_cast<const RTTI*>(&NiMultiTargetTransformController_RTTI)));
    if (multi != nullptr) {
        Hold(multi);
    } else {
        uint32_t count = 0;
        for (uint32_t i = 0; i < self->arraySize; i++) {
            const char* type = self->idTagArray[i].ctlrType;
            if (type != sys::kNullFixedString && NiStricmp(type, "NiTransformController") == 0) {
                count++;
            }
        }
        if (uint16_t(count) == 0) {
            return 0;
        }
        void* memory = NiNew(0x40);
        if (memory != nullptr) {
            InterpController_Construct(memory);
            multi = static_cast<NiMultiTargetTransformController*>(memory);
            multi->vtable = reinterpret_cast<void*>(NiMultiTargetTransformController_vtable);
            multi->blendInterps = nullptr;
            multi->targets = nullptr;
            multi->numInterps = 0;
            MultiTarget_SetNumInterps(multi, uint16_t(count));
            Hold(multi);
        }
        Virtual<void>(multi, 21, root);   // SetTarget
        multi->flags |= 0x20;
        made = 1;
        NiTimeController* manager = re::PtrCast<NiTimeController*>(self->owner);
        if (root == static_cast<NiAVObject*>(manager->target)) {
            ObjectNET_RemoveController(root, multi);
            AssignReleasingFirst(multi->next, manager->next);
            NiTimeController* old = manager->next;
            if (old != multi) {
                ReleaseRef(old);
                manager->next = multi;
                Hold(multi);
            }
        }
    }
    for (uint32_t i = 0; i < self->arraySize; i++) {
        IDTag* tag = &self->idTagArray[i];
        SeqInterpArrayItem* item = &self->interpArray[i];
        const char* type = tag->ctlrType;
        if (type == sys::kNullFixedString || crt::Strcmp(type, "NiTransformController") != 0) {
            continue;
        }
        auto* target = Virtual<NiAVObject*>(palette, 18, tag->avObjectName);   // GetAVObject
        if (target == nullptr) {
            continue;
        }
        if (self->accumRoot == target && re::PtrCast<uint8_t*>(self->owner)[0x58] != 0) {
            continue;   // (a cumulative manager's accumulation root keeps its own)
        }
        NiTimeController* transform =
            ObjectNET_GetController(target, reinterpret_cast<const RTTI*>(&NiTransformController_RTTI));
        uint16_t index = 0;
        bool found = false;
        for (; index < multi->numInterps; index++) {
            if (multi->targets[index] == target) {
                found = true;
                break;
            }
        }
        if (found && index != kInvalidIndex) {
            multi->targets[index] = target;
            target->flags |= 0x80;
        } else {
            index = MultiTarget_AddInterpolatorTarget(multi, target);
            if (index == kInvalidIndex) {
                continue;
            }
        }
        ObjectNET_RemoveController(target, transform);
        if (item->interpCtlr != multi) {
            ReleaseRef(item->interpCtlr);
            item->interpCtlr = multi;
            Hold(multi);
        }
        auto* blend = reinterpret_cast<NiBlendInterpolator*>(multi->blendInterps + uint32_t(index) * 0x30);
        item->blendInterp = blend;
        blend->flags |= 1;
    }
    ReleaseRef(multi);
    return made;
}

// 0x814940: Activate(priority, startOver, weight, easeInTime, timeSyncSequence, transition): from
// inactive only; synchronised to the time sync sequence when it can be (not in a loop of partners,
// its "morph:" keys matching); the interpolators attached; easing in (or a transition's
// destination) when there's an ease-in time; the offset reset to start over
uint8_t __fastcall Seq_Activate(NiControllerSequence* self, re::Edx, int priority, int startOver, float weight,
                                float easeInTime, NiControllerSequence* timeSync, int transition) {
    if (self->state != INACTIVE) {
        return 0;
    }
    self->partnerSequence = nullptr;
    if (timeSync != nullptr) {
        NiControllerSequence* partner = timeSync->partnerSequence;
        if (partner != nullptr) {
            if (partner == self || !Sequence_VerifyDependencies(self, partner)) {
                return 0;
            }
        }
        if (!Sequence_CanSyncTo(self, timeSync)) {
            return 0;
        }
        self->partnerSequence = timeSync;
    }
    Sequence_AttachInterpolators(self, priority);
    self->seqWeight = weight;
    if (easeInTime > 0.0f) {
        self->startTime = kInvalidFloat;
        self->endTime = easeInTime;
        self->state = re::Flag(transition) ? TRANSDEST : EASINGIN;
    } else {
        self->state = ANIMATING;
    }
    if (re::Flag(startOver)) {
        self->offset = kInvalidFloat;
    }
    self->lastTime = kInvalidFloat;
    NotifyCallbacks(self);
    return 1;
}

// 0x814A20: Deactivate(easeOutTime, transition): easing out (or a transition's source) when
// there's an ease-out time; else at once: the offset kept so a restart continues, the
// interpolators detached
uint8_t __fastcall Seq_Deactivate(NiControllerSequence* self, re::Edx, float easeOutTime, int transition) {
    if (self->state == INACTIVE) {
        return 0;
    }
    if (easeOutTime > 0.0f) {
        self->startTime = kInvalidFloat;
        self->endTime = easeOutTime;
        self->state = re::Flag(transition) ? TRANSSOURCE : EASINGOUT;
    } else {
        if (!(self->lastTime == kInvalidFloat)) {
            self->offset = (self->weightedLastTime / self->frequency - self->lastTime) + self->offset;
        }
        self->state = INACTIVE;
        self->destFrame = kInvalidFloat;
        self->partnerSequence = nullptr;
        Sequence_DetachInterpolators(self);
    }
    NotifyCallbacks(self);
    return 1;
}

// 0x814AD0: StartBlend(destination, duration, destFrame, priority, sourceWeight, destWeight,
// timeSync): this one restarted at its current frame and blended out as a transition source while
// the destination is activated as the transition's destination at its frame
uint8_t __fastcall Seq_StartBlend(NiControllerSequence* self, re::Edx, NiControllerSequence* destination, float duration,
                                  float destFrame, int priority, float sourceWeight, float destWeight,
                                  NiControllerSequence* timeSync) {
    Sequence_Deactivate(self, 0.0f, 1);
    if (duration <= 0.0f) {
        duration = kMinDuration;
    }
    self->destFrame = self->lastScaledTime / self->frequency;
    destination->destFrame = destFrame / destination->frequency;
    if (self->state != INACTIVE) {
        return 0;
    }
    self->partnerSequence = nullptr;
    Sequence_AttachInterpolators(self, priority);
    self->seqWeight = sourceWeight;
    self->lastTime = kInvalidFloat;
    self->state = ANIMATING;
    NotifyCallbacks(self);
    if (!Sequence_Activate(destination, priority, 0, destWeight, duration, timeSync, 1)) {
        return 0;
    }
    Sequence_Deactivate(self, duration, 1);
    return 1;
}

// 0x814BC0: StartMorph(destination, duration, priority, sourceWeight, destWeight): this one
// restarted synchronised to the destination, the destination activated (easing in as the
// transition's destination), this one the morph source
uint8_t __fastcall Seq_StartMorph(NiControllerSequence* self, re::Edx, NiControllerSequence* destination, float duration,
                                  int priority, float sourceWeight, float destWeight) {
    Sequence_Deactivate(self, 0.0f, 1);
    if (!Sequence_Activate(self, priority, 0, sourceWeight, duration, destination, 1)) {
        return 0;
    }
    if (destination->state != INACTIVE) {
        return 0;
    }
    destination->partnerSequence = nullptr;
    Sequence_AttachInterpolators(destination, priority);
    destination->seqWeight = destWeight;
    if (duration > 0.0f) {
        destination->startTime = kInvalidFloat;
        destination->state = TRANSDEST;
        destination->endTime = duration;
    } else {
        destination->state = ANIMATING;
    }
    destination->offset = kInvalidFloat;
    destination->lastTime = kInvalidFloat;
    NotifyCallbacks(destination);
    self->state = MORPHSOURCE;
    return 1;
}

// the text keys of a sequence (its NiTextKeyExtraData's)
inline NiTextKey* KeysOf(NiControllerSequence* sequence, uint32_t& count) {
    count = sequence->textKeys->numKeys;
    return sequence->textKeys->keys;
}

// 0x814CA0: FindCorrespondingMorphFrame(source, time): where the source is between its two
// "morph:" keys around its time (the last before, the first at or after; wrapping a loop), the
// same place between this sequence's keys of those names; kept in this one's key range
float __fastcall Seq_FindCorrespondingMorphFrame(NiControllerSequence* self, re::Edx, NiControllerSequence* source,
                                                 float time) {
    float srcTime = Sequence_ComputeScaledTime(source, time, 0);
    uint32_t count;
    NiTextKey* keys = KeysOf(source, count);
    NiTextKey* prev = nullptr;
    NiTextKey* next = nullptr;
    NiTextKey* first = nullptr;
    NiTextKey* last = nullptr;
    for (uint32_t i = 0; i < count; i++) {
        NiTextKey* key = &keys[i];
        if (key->text == nullptr || crt::Strncmp(key->text, "morph:", 6) != 0) {
            continue;
        }
        last = key;
        if (first == nullptr) {
            first = key;
        }
        if (srcTime > key->time) {
            prev = key;
        } else if (next == nullptr) {
            next = key;
        }
    }
    bool wrapped = false;
    float prevTime;
    float nextTime;
    if (next == nullptr) {
        prevTime = prev->time;
        next = first;
        wrapped = true;
        nextTime = (source->endKeyTime - source->beginKeyTime) + first->time;
    } else {
        if (prev == nullptr) {
            prev = last;
            prevTime = (source->beginKeyTime - source->endKeyTime) + last->time;
        } else {
            prevTime = prev->time;
        }
        nextTime = next->time;
    }
    float ratio = (srcTime - prevTime) / (nextTime - prevTime);

    uint32_t myCount;
    NiTextKey* myKeys = KeysOf(self, myCount);
    bool havePrev = prev == nullptr;
    bool haveNext = next == nullptr;
    NiTextKey* myPrev = nullptr;
    NiTextKey* myNext = nullptr;
    for (uint32_t i = 0; i < myCount; i++) {
        NiTextKey* key = &myKeys[i];
        if (havePrev) {
            if (haveNext) {
                break;
            }
        } else {
            if (crt::Strcmp(key->text, prev->text) == 0) {
                myPrev = key;
                havePrev = true;
            }
            if (haveNext) {
                continue;
            }
        }
        if (crt::Strcmp(key->text, next->text) == 0) {
            myNext = key;
            haveNext = true;
        }
    }
    float myNextTime = 0.0f;
    float myPrevTime = 0.0f;
    if (myPrev == myNext) {
        myPrevTime = myPrev->time;
        myNextTime = myNext->time;
        if (wrapped) {
            myNextTime = (self->endKeyTime - self->beginKeyTime) + myNextTime;
        } else {
            myPrevTime = myPrevTime - (self->endKeyTime - self->beginKeyTime);
        }
    } else {
        if (myNext != nullptr) {
            myNextTime = myNext->time;
        }
        if (myPrev != nullptr) {
            myPrevTime = myPrev->time;
        }
        if (myNextTime < myPrevTime) {
            myNextTime = myNextTime + (self->endKeyTime - self->beginKeyTime);
        }
    }
    float frame = myPrevTime + (myNextTime - myPrevTime) * ratio;
    if (self->beginKeyTime > frame) {
        return frame + (self->endKeyTime - self->beginKeyTime);
    }
    if (self->endKeyTime < frame) {
        return frame - (self->endKeyTime - self->beginKeyTime);
    }
    return frame;
}

// 0x814F70: SetInterpsWeightAndTime(weight, easeSpinner, time): each item's weight and ease
// spinner in its blend interpolator (the weights to normalise again when either changed; not for
// a blend interpolator's single interpolator), and its update time
void __fastcall Seq_SetInterpsWeightAndTime(NiControllerSequence* self, re::Edx, float weight, float easeSpinner,
                                            float time) {
    for (uint32_t i = 0; i < self->arraySize; i++) {
        SeqInterpArrayItem* item = &self->interpArray[i];
        NiBlendInterpolator* blend = item->blendInterp;
        if (blend == nullptr) {
            continue;
        }
        if (blend->interpCount != 1 || item->blendIdx != blend->singleIdx) {
            float* slot = &blend->interpArray[item->blendIdx].weight;
            if (!(weight == *slot)) {
                *slot = weight;
                blend->flags |= 4;
                blend->highSumOfWeights = kInvalidFloat;
                blend->nextHighSumOfWeights = kInvalidFloat;
                blend->highEaseSpinner = kInvalidFloat;
            }
        }
        blend = item->blendInterp;
        if (blend->interpCount != 1 || item->blendIdx != blend->singleIdx) {
            float* slot = &blend->interpArray[item->blendIdx].easeSpinner;
            if (!(easeSpinner == *slot)) {
                *slot = easeSpinner;
                blend->flags |= 4;
                blend->highSumOfWeights = kInvalidFloat;
                blend->nextHighSumOfWeights = kInvalidFloat;
                blend->highEaseSpinner = kInvalidFloat;
            }
        }
        blend = item->blendInterp;
        if (blend->interpCount == 1 && item->blendIdx == blend->singleIdx) {
            blend->singleTime = time;
        } else {
            blend->interpArray[item->blendIdx].updateTime = time;
        }
    }
}

// 0x815070: ProcessClone: without an owner the interpolators and controllers processed; else (an
// active sequence's interpolators mapped to their clones in the clone) the controllers and blend
// interpolators mapped to their clones (or kept), and the owner, partner and accumulation root too
void __fastcall Seq_ProcessClone(NiControllerSequence* self, re::Edx, void* cloning) {
    Object_ProcessClone(self, cloning);
    const re::Ptr32<void>* found = FindClone(cloning, self);
    NiControllerSequence* clone = found != nullptr ? static_cast<NiControllerSequence*>(*found) : nullptr;
    for (uint32_t i = 0; i < self->arraySize; i++) {
        SeqInterpArrayItem* item = &self->interpArray[i];
        if (item->interpolator == nullptr) {
            continue;
        }
        if (self->owner == nullptr) {
            Virtual<void>(item->interpolator, 11, cloning);
            if (self->interpArray[i].interpCtlr != nullptr) {
                Virtual<void>(self->interpArray[i].interpCtlr, 11, cloning);
            }
            continue;
        }
        if (self->state == INACTIVE) {
            Virtual<void>(item->interpolator, 11, cloning);
        } else {
            const re::Ptr32<void>* mapped = FindClone(cloning, item->interpolator);
            if (mapped != nullptr) {
                Pointer_Assign(reinterpret_cast<re::Ptr32<NiRefObject>*>(&clone->interpArray[i].interpolator),
                               static_cast<NiRefObject*>(*mapped));
            } else {
                Pointer_AssignFrom(reinterpret_cast<re::Ptr32<NiRefObject>*>(&clone->interpArray[i].interpolator),
                                   reinterpret_cast<const re::Ptr32<NiRefObject>*>(&item->interpolator));
            }
        }
        NiTimeController* ctlr = self->interpArray[i].interpCtlr;
        const re::Ptr32<void>* mappedCtlr = FindClone(cloning, ctlr);
        NiTimeController* cloneCtlr = mappedCtlr != nullptr ? static_cast<NiTimeController*>(*mappedCtlr) : ctlr;
        re::Ptr32<NiTimeController>& slot = clone->interpArray[i].interpCtlr;
        if (slot != cloneCtlr) {
            ReleaseRef(slot);
            slot = cloneCtlr;
            Hold(cloneCtlr);
        }
        NiBlendInterpolator* blend = self->interpArray[i].blendInterp;
        const re::Ptr32<void>* mappedBlend = FindClone(cloning, blend);
        clone->interpArray[i].blendInterp =
            mappedBlend != nullptr ? static_cast<NiBlendInterpolator*>(*mappedBlend) : blend;
    }
    const re::Ptr32<void>* owner = FindClone(cloning, self->owner);
    clone->owner = owner != nullptr ? static_cast<NiControllerManager*>(*owner) : static_cast<ni::anim::NiControllerManager*>(self->owner);
    const re::Ptr32<void>* partner = FindClone(cloning, self->partnerSequence);
    clone->partnerSequence = partner != nullptr ? static_cast<NiControllerSequence*>(*partner) : static_cast<ni::anim::NiControllerSequence*>(self->partnerSequence);
    const re::Ptr32<void>* accumRoot = FindClone(cloning, self->accumRoot);
    clone->accumRoot = accumRoot != nullptr ? static_cast<NiAVObject*>(*accumRoot) : static_cast<ni::NiAVObject*>(self->accumRoot);
}

// 0x815330: InterpArrayItem::LoadBinary: the interpolator and controller links (resolved at once);
// before 10.1.0.111 an old blend interpolator link and priority read past
void __fastcall Item_LoadBinary(SeqInterpArrayItem* self, re::Edx, void* stream) {
    NiObject* interpolator = Stream_ReadLinkIDObject(stream);
    AssignReleasingFirst(self->interpolator, interpolator);
    NiObject* ctlr = Stream_ReadLinkIDObject(stream);
    AssignReleasingFirst(self->interpCtlr, ctlr);
    if (StreamVersion(stream) < kVersion_10_1_0_111) {
        Stream_ReadLinkIDObject(stream);
        uint16_t priority;
        LoadBytes(stream, &priority, 2);
    }
}

// 0x815400: IDTag::LoadBinary(stream, palette&): inline strings before 10.1.0.113; to 20.1.0.1 the
// string palette's link (resolved into palette) and five offsets into it (kept in the fields for
// PostLinkObject); then fixed strings
void __fastcall Tag_LoadBinary(IDTag* self, re::Edx, void* stream, re::Ptr32<NiObject>* palette) {
    uint32_t version = StreamVersion(stream);
    if (version < kVersion_10_1_0_113) {
        Stream_LoadInlineFixedString(stream, &self->avObjectName);
        Stream_LoadInlineFixedString(stream, &self->propertyType);
        Stream_LoadInlineFixedString(stream, &self->ctlrType);
        Stream_LoadInlineFixedString(stream, &self->ctlrID);
        Stream_LoadInlineFixedString(stream, &self->interpolatorID);
        return;
    }
    if (version < kVersion_20_1_0_1) {
        *palette = Stream_ReadLinkIDObject(stream);
        uint32_t offsets[5] = {0, 0, 0, 0, 0};
        LoadBytes(stream, &offsets[0], 4);
        LoadBytes(stream, &offsets[1], 4);
        LoadBytes(stream, &offsets[2], 4);
        LoadBytes(stream, &offsets[3], 4);
        LoadBytes(stream, &offsets[4], 4);
        uint32_t* fields = reinterpret_cast<uint32_t*>(self);
        fields[0] = offsets[0];
        fields[1] = offsets[1];
        fields[2] = offsets[2];
        fields[3] = offsets[3];
        fields[4] = offsets[4];
        return;
    }
    Stream_LoadFixedString(stream, &self->avObjectName);
    Stream_LoadFixedString(stream, &self->propertyType);
    Stream_LoadFixedString(stream, &self->ctlrType);
    Stream_LoadFixedString(stream, &self->ctlrID);
    Stream_LoadFixedString(stream, &self->interpolatorID);
}

// 0x815590: IDTag::ConvertFromPalette(palette, stream): the offsets the fields hold made into
// strings from the palette's buffer (-1: none; the fields cleared raw first)
void __fastcall Tag_ConvertFromPalette(IDTag* self, re::Edx, NiStringPalette* palette, void*) {
    uint32_t* fields = reinterpret_cast<uint32_t*>(self);
    uint32_t offsets[5] = {fields[0], fields[1], fields[2], fields[3], fields[4]};
    NiFixedString empty = sys::kNullFixedString;
    crt::MemcpyS(&self->avObjectName, 4, &empty, 4);
    crt::MemcpyS(&self->propertyType, 4, &empty, 4);
    crt::MemcpyS(&self->ctlrType, 4, &empty, 4);
    crt::MemcpyS(&self->ctlrID, 4, &empty, 4);
    crt::MemcpyS(&self->interpolatorID, 4, &empty, 4);
    re::Ptr32<const char>* targets[5] = {&self->avObjectName, &self->propertyType, &self->ctlrType, &self->ctlrID,
                                 &self->interpolatorID};
    for (int i = 0; i < 5; i++) {
        if (offsets[i] != 0xFFFFFFFF) {
            FixedString_AssignText(targets[i], palette->buffer + offsets[i]);
        }
    }
    FixedRelease(&empty);
}

// 0x8158E0: IDTag(avObjectName, propertyType, ctlrType, ctlrID, interpolatorID)
IDTag* __fastcall Tag_Construct(IDTag* self, re::Edx, const re::Ptr32<const char>* avObjectName,
                                const re::Ptr32<const char>* propertyType, const re::Ptr32<const char>* ctlrType,
                                const re::Ptr32<const char>* ctlrID, const re::Ptr32<const char>* interpolatorID) {
    self->avObjectName = sys::kNullFixedString;
    self->propertyType = sys::kNullFixedString;
    self->ctlrType = sys::kNullFixedString;
    self->ctlrID = sys::kNullFixedString;
    self->interpolatorID = sys::kNullFixedString;
    FixedString_AssignText(&self->avObjectName, nullptr);
    FixedString_AssignText(&self->propertyType, nullptr);
    FixedString_AssignText(&self->ctlrType, nullptr);
    FixedString_AssignText(&self->ctlrID, nullptr);
    FixedString_AssignText(&self->interpolatorID, nullptr);
    FixedString_Assign(&self->avObjectName, avObjectName);
    FixedString_Assign(&self->propertyType, propertyType);
    FixedString_Assign(&self->ctlrType, ctlrType);
    FixedString_Assign(&self->ctlrID, ctlrID);
    FixedString_Assign(&self->interpolatorID, interpolatorID);
    return self;
}

// 0x815990: IDTag(const IDTag&)
IDTag* __fastcall Tag_CopyConstruct(IDTag* self, re::Edx, const IDTag* other) {
    self->avObjectName = sys::kNullFixedString;
    self->propertyType = sys::kNullFixedString;
    self->ctlrType = sys::kNullFixedString;
    self->ctlrID = sys::kNullFixedString;
    self->interpolatorID = sys::kNullFixedString;
    FixedString_Assign(&self->avObjectName, &other->avObjectName);
    FixedString_Assign(&self->propertyType, &other->propertyType);
    FixedString_Assign(&self->ctlrType, &other->ctlrType);
    FixedString_Assign(&self->ctlrID, &other->ctlrID);
    FixedString_Assign(&self->interpolatorID, &other->interpolatorID);
    return self;
}

GAME_FN(IDTag*, __thiscall, IDTag_Construct, 0x008158E0, IDTag* self, const re::Ptr32<const char>* avObjectName,
        const re::Ptr32<const char>* propertyType, const re::Ptr32<const char>* ctlrType, const re::Ptr32<const char>* ctlrID,
        const re::Ptr32<const char>* interpolatorID);

// 0x815A10: IDTag() (through the five-string constructor with empty temporaries)
IDTag* __fastcall Tag_ConstructDefault(IDTag* self, re::Edx) {
    NiFixedString t[5] = {sys::kNullFixedString, sys::kNullFixedString, sys::kNullFixedString, sys::kNullFixedString,
                          sys::kNullFixedString};
    IDTag_Construct(self, &t[0], &t[1], &t[2], &t[3], &t[4]);
    FixedRelease(&t[0]);
    FixedRelease(&t[1]);
    FixedRelease(&t[2]);
    FixedRelease(&t[3]);
    FixedRelease(&t[4]);
    return self;
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_SEQ2_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC31_SEQ2_REVERSED("NiControllerSequence::SetupMultiTargetController", 0x00814670,
                    &ni::anim::Seq_SetupMultiTargetController);
NIC31_SEQ2_REVERSED("NiControllerSequence::Activate", 0x00814940, &ni::anim::Seq_Activate);
NIC31_SEQ2_REVERSED("NiControllerSequence::Deactivate", 0x00814A20, &ni::anim::Seq_Deactivate);
NIC31_SEQ2_REVERSED("NiControllerSequence::StartBlend", 0x00814AD0, &ni::anim::Seq_StartBlend);
NIC31_SEQ2_REVERSED("NiControllerSequence::StartMorph", 0x00814BC0, &ni::anim::Seq_StartMorph);
NIC31_SEQ2_REVERSED("NiControllerSequence::FindCorrespondingMorphFrame", 0x00814CA0,
                    &ni::anim::Seq_FindCorrespondingMorphFrame);
NIC31_SEQ2_REVERSED("NiControllerSequence::SetInterpsWeightAndTime", 0x00814F70,
                    &ni::anim::Seq_SetInterpsWeightAndTime);
NIC31_SEQ2_REVERSED("NiControllerSequence::ProcessClone", 0x00815070, &ni::anim::Seq_ProcessClone);
NIC31_SEQ2_REVERSED("NiControllerSequence::InterpArrayItem::LoadBinary", 0x00815330, &ni::anim::Item_LoadBinary);
NIC31_SEQ2_REVERSED("NiControllerSequence::IDTag::LoadBinary", 0x00815400, &ni::anim::Tag_LoadBinary);
NIC31_SEQ2_REVERSED("NiControllerSequence::IDTag::ConvertFromPalette", 0x00815590,
                    &ni::anim::Tag_ConvertFromPalette);
NIC31_SEQ2_REVERSED("NiControllerSequence::IDTag::IDTag(strings)", 0x008158E0, &ni::anim::Tag_Construct);
NIC31_SEQ2_REVERSED("NiControllerSequence::IDTag::IDTag(copy)", 0x00815990, &ni::anim::Tag_CopyConstruct);
NIC31_SEQ2_REVERSED("NiControllerSequence::IDTag::IDTag", 0x00815A10, &ni::anim::Tag_ConstructDefault);
