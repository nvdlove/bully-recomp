// NiControllerSequence (src/gamebryo/NiControllerSequence.h), the third part: Update (the
// eases, transitions and morphs worked through, the blend weights set), binding to a target tree
// (StoreTargets), the interpolator array (adding, growing, sorting by blend interpolator),
// streaming (older files' controller lists converted in PostLinkObject), cloning, destruction,
// and the statics that take every sequence from a stream or file.
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's order.

#include <cstddef>
#include <cstdint>
#include <cstring>

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
using scene::SaveBytes;
using scene::Stream_LoadFixedString;
using scene::Stream_LoadInlineFixedString;
using scene::Stream_ReadLinkID;
using scene::Stream_SaveFixedString;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBlendInterpolator_RTTI, 0x020E57A4);
GAME_VAR(uint32_t, kNiInterpControllerClassIndex, 0x020E5654);
GAME_VAR(uint16_t, kInvalidIndex, 0x00967FA0);   // NiInterpController::INVALID_INDEX

constexpr uintptr_t NiControllerSequence_vtable = 0x00966D38;
constexpr uintptr_t CallbackArray_vtable = 0x00961988;
constexpr uintptr_t ControllerArray_vtable = 0x00965A90;   // (PostLinkObject's local array)
constexpr uintptr_t InterpArrayItem_Construct_address = 0x008141B0;
constexpr uintptr_t IDTag_Construct_address = 0x00815A10;
constexpr uint32_t kVersion_10_1_0_104 = 0x0A010068;
constexpr uint32_t kVersion_10_1_0_107 = 0x0A01006B;
constexpr uint32_t kVersion_10_1_0_113 = 0x0A010071;
constexpr uint32_t kVersion_10_3_0_1 = 0x0A030001;
constexpr uint32_t kVersion_20_1_0_1 = 0x14010001;
constexpr uint32_t kVersion_20_3_0_8 = 0x14030008;

GAME_FN(void*, __cdecl, NiAllocArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __cdecl, NiDeletePointerArray, 0x00836F80, void* p);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(NiObject*, __cdecl, NiDynamicCast, 0x00401FF0, const RTTI* rtti, NiObject* object);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(void*, __thiscall, Stream_Construct, 0x0075CE40, void* self);
GAME_FN(void, __thiscall, Stream_Destruct, 0x0075C020, void* self);
GAME_FN(uint8_t, __thiscall, Stream_Load, 0x00759E10, void* self, const char* filename);
GAME_FN(void, __thiscall, Object_Destruct, 0x007769D0, void* self);
GAME_FN(void, __thiscall, Object_LoadBinary, 0x007554E0, void* self, void* stream);
GAME_FN(void, __thiscall, Object_SaveBinary, 0x008A20A0, void* self, void* stream);
GAME_FN(void, __thiscall, Object_PostLinkObject, 0x008A20A0, void* self, void* stream);
GAME_FN(void, __thiscall, Object_CopyMembers, 0x00755410, void* self, void* dest, void* cloning);
GAME_FN(NiObject*, __thiscall, Object_CreateSharedClone, 0x007553B0, void* self, void* cloning);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* self, NiRefObject* object);
GAME_FN(void, __thiscall, ObjectNET_RemoveController, 0x007646E0, void* self, NiTimeController* controller);
GAME_FN(void, __thiscall, TimeController_SetNext, 0x00764500, NiTimeController* self, NiTimeController* next);
GAME_FN(void, __thiscall, PointerArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __thiscall, PointerArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<NiRefObject>* value);
// (this chunk's, called where modes apply)
GAME_FN(SeqInterpArrayItem*, __thiscall, InterpArrayItem_Assign, 0x008138C0, SeqInterpArrayItem* self,
        const SeqInterpArrayItem* other);
GAME_FN(void, __thiscall, InterpArrayItem_ClearValues, 0x008141D0, SeqInterpArrayItem* self);
GAME_FN(void*, __thiscall, InterpArrayItem_VectorDeletingDestructor, 0x00814250, SeqInterpArrayItem* self,
        uint32_t flags);
GAME_FN(void, __thiscall, InterpArrayItem_LoadBinary, 0x00815330, SeqInterpArrayItem* self, void* stream);
GAME_FN(void, __thiscall, IDTag_Destruct, 0x008142F0, IDTag* self);
GAME_FN(void, __thiscall, IDTag_LoadBinary, 0x00815400, IDTag* self, void* stream, re::Ptr32<NiObject>* palette);
GAME_FN(void, __thiscall, IDTag_ConvertFromPalette, 0x00815590, IDTag* self, NiStringPalette* palette, void* stream);
GAME_FN(IDTag*, __thiscall, IDTag_Construct, 0x008158E0, IDTag* self, const re::Ptr32<const char>* avObjectName,
        const re::Ptr32<const char>* propertyType, const re::Ptr32<const char>* ctlrType, const re::Ptr32<const char>* ctlrID,
        const re::Ptr32<const char>* interpolatorID);
GAME_FN(IDTag*, __thiscall, IDTag_CopyConstruct, 0x00815990, IDTag* self, const IDTag* other);
GAME_FN(NiControllerSequence*, __thiscall, Sequence_Construct, 0x008143D0, void* self);
GAME_FN(void, __thiscall, Sequence_Destruct, 0x00815B40, NiControllerSequence* self);
GAME_FN(float, __thiscall, Sequence_ComputeScaledTime, 0x00813A90, NiControllerSequence* self, float time,
        int storeLastTime);
GAME_FN(float, __thiscall, Sequence_GetKeyTimeAt, 0x008145E0, NiControllerSequence* self, const re::Ptr32<const char>* key);
GAME_FN(uint8_t, __thiscall, Sequence_Deactivate, 0x00814A20, NiControllerSequence* self, float easeOutTime,
        int transition);
GAME_FN(uint8_t, __thiscall, Sequence_StartBlend, 0x00814AD0, NiControllerSequence* self,
        NiControllerSequence* destination, float duration, float destFrame, int priority, float sourceWeight,
        float destWeight, NiControllerSequence* timeSync);
GAME_FN(float, __thiscall, Sequence_FindCorrespondingMorphFrame, 0x00814CA0, NiControllerSequence* self,
        NiControllerSequence* source, float time);
GAME_FN(void, __thiscall, Sequence_SetInterpsWeightAndTime, 0x00814F70, NiControllerSequence* self, float weight,
        float easeSpinner, float time);
GAME_FN(uint8_t, __thiscall, Sequence_SetupMultiTargetController, 0x00814670, NiControllerSequence* self,
        NiAVObject* root, void* palette);
GAME_FN(void, __thiscall, Sequence_SortByBlendInterp, 0x00815CB0, NiControllerSequence* self);
GAME_FN(void, __thiscall, Sequence_Update, 0x00816320, NiControllerSequence* self, float time,
        int updateInterpolators);
GAME_FN(uint8_t, __thiscall, Sequence_ReallocateArrays, 0x00816950, NiControllerSequence* self);
GAME_FN(void, __thiscall, Sequence_CopyMembers, 0x00816B00, NiControllerSequence* self, NiControllerSequence* dest,
        void* cloning);
GAME_FN(uint32_t, __thiscall, Sequence_AddInterpolator, 0x008174D0, NiControllerSequence* self,
        NiInterpolator* interpolator, const IDTag* tag);
GAME_FN(uint8_t, __thiscall, Sequence_AddInterpController, 0x008176E0, NiControllerSequence* self, const char* idString,
        NiTimeController* ctlr);
GAME_FN(uint8_t, __thiscall, Sequence_AddConvertedController, 0x00817D60, NiControllerSequence* self,
        const char* idString, NiTimeController* ctlr);
GAME_FN(re::Ptr32<NiControllerSequence>*, __cdecl, Sequence_CreateSequenceFromFile, 0x00814480, re::Ptr32<NiControllerSequence>* result,
        void* stream, uint32_t index);
GAME_FN(uint8_t, __cdecl, Sequence_CreateAllSequencesFromStream, 0x00817CD0, void* stream, void* array);

inline void FixedRelease(re::Ptr32<const char>* handle) {
    sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(handle));
}

// NiFixedString::operator= as the code inlines it: the new one held, the old one let go
inline void FixedAssign(re::Ptr32<const char>* slot, NiFixedString value) {
    if (*slot == value) {
        return;
    }
    if (value != sys::kNullFixedString) {
        sys::Increment(re::PtrCast<char*>(value) - 8);
    }
    FixedRelease(slot);
    *slot = value;
}

inline void TagAssign(IDTag* dest, const IDTag* source) {
    FixedAssign(&dest->avObjectName, source->avObjectName);
    FixedAssign(&dest->propertyType, source->propertyType);
    FixedAssign(&dest->ctlrType, source->ctlrType);
    FixedAssign(&dest->ctlrID, source->ctlrID);
    FixedAssign(&dest->interpolatorID, source->interpolatorID);
}

inline void Hold(void* object) {
    if (object != nullptr) {
        sys::Increment(&static_cast<NiRefObject*>(object)->refCount);
    }
}

// the NiNew[] arrays the sequence keeps (the count before the first, each constructed)
inline SeqInterpArrayItem* NewItems(uint32_t count) {
    uint32_t* block = static_cast<uint32_t*>(NiAllocArray(KeyArraySize(count, sizeof(SeqInterpArrayItem))));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, sizeof(SeqInterpArrayItem), count, reinterpret_cast<void*>(InterpArrayItem_Construct_address));
    return reinterpret_cast<SeqInterpArrayItem*>(block + 1);
}

inline IDTag* NewTags(uint32_t count) {
    uint32_t* block = static_cast<uint32_t*>(NiAllocArray(KeyArraySize(count, sizeof(IDTag))));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, sizeof(IDTag), count, reinterpret_cast<void*>(IDTag_Construct_address));
    return reinterpret_cast<IDTag*>(block + 1);
}

inline void DeleteTags(IDTag* tags) {
    uint32_t* block = reinterpret_cast<uint32_t*>(tags) - 1;
    for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
        IDTag_Destruct(&tags[i]);
    }
    NiFreeArray(block, sizeof(IDTag));
}

// ---- NiControllerSequence

// 0x8156E0: SaveBinary
void __fastcall Seq_SaveBinary(NiControllerSequence* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    Stream_SaveFixedString(stream, &self->name);
    SaveBytes(stream, &self->arraySize, 4);
    SaveBytes(stream, &self->arrayGrowBy, 4);
    for (uint32_t i = 0; i < self->arraySize; i++) {
        SeqInterpArrayItem* item = &self->interpArray[i];
        Virtual<void>(stream, 12, item->interpolator);   // SaveLinkID
        Virtual<void>(stream, 12, item->interpCtlr);
        IDTag* tag = &self->idTagArray[i];
        Stream_SaveFixedString(stream, &tag->avObjectName);
        Stream_SaveFixedString(stream, &tag->propertyType);
        Stream_SaveFixedString(stream, &tag->ctlrType);
        Stream_SaveFixedString(stream, &tag->ctlrID);
        Stream_SaveFixedString(stream, &tag->interpolatorID);
    }
    SaveBytes(stream, &self->seqWeight, 4);
    Virtual<void>(stream, 12, self->textKeys);
    SaveBytes(stream, &self->cycleType, 4);
    SaveBytes(stream, &self->frequency, 4);
    SaveBytes(stream, &self->beginKeyTime, 4);
    SaveBytes(stream, &self->endKeyTime, 4);
    Virtual<void>(stream, 12, self->owner);
    Stream_SaveFixedString(stream, &self->accumRootName);
    SaveBytes(stream, &self->unknown78, 4);
}

// 0x8158C0: CreateObject
NiObject* __cdecl Seq_CreateObject() {
    void* memory = NiNew(0x7C);
    if (memory == nullptr) {
        return nullptr;
    }
    return Sequence_Construct(memory);
}

// 0x815B40: ~NiControllerSequence
void __fastcall Seq_Destruct(NiControllerSequence* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiControllerSequence_vtable);
    if (self->interpArray != nullptr) {
        InterpArrayItem_VectorDeletingDestructor(self->interpArray, 3);
    }
    if (self->idTagArray != nullptr) {
        DeleteTags(self->idTagArray);
    }
    FixedRelease(&self->accumRootName);
    ReleaseRef(self->stringPalette);
    self->callbacks.vtable = reinterpret_cast<void*>(CallbackArray_vtable);
    NiFree(self->callbacks.base);
    ReleaseRef(self->textKeys);
    FixedRelease(&self->name);
    Object_Destruct(self);
}

// 0x815CB0: SortByBlendInterp: the items (and their tags) put in order of their blend
// interpolator's address (each one swapped with every earlier one ordered after it)
void __fastcall Seq_SortByBlendInterp(NiControllerSequence* self, re::Edx) {
    for (uint32_t i = 0; i < self->arraySize; i++) {
        SeqInterpArrayItem current;
        SeqInterpArrayItem* item = &self->interpArray[i];
        uintptr_t key = re::PtrCast<uintptr_t>(item->blendInterp);
        current.interpolator = item->interpolator;
        Hold(current.interpolator);
        current.interpCtlr = item->interpCtlr;
        Hold(current.interpCtlr);
        current.blendIdx = item->blendIdx;
        current.blendInterp = item->blendInterp;
        IDTag currentTag;
        IDTag_CopyConstruct(&currentTag, &self->idTagArray[i]);
        for (uint32_t j = 0; j < i; j++) {
            if (re::PtrCast<uintptr_t>(self->interpArray[j].blendInterp) <= key) {
                continue;
            }
            InterpArrayItem_Assign(&self->interpArray[i], &self->interpArray[j]);
            TagAssign(&self->idTagArray[i], &self->idTagArray[j]);
            InterpArrayItem_Assign(&self->interpArray[j], &current);
            TagAssign(&self->idTagArray[j], &currentTag);
            key = re::PtrCast<uintptr_t>(self->interpArray[i].blendInterp);
            InterpArrayItem_Assign(&current, &self->interpArray[i]);
            TagAssign(&currentTag, &self->idTagArray[i]);
        }
        IDTag_Destruct(&currentTag);
        ReleaseRef(current.interpCtlr);
        ReleaseRef(current.interpolator);
    }
}

// 0x816320: Update(time, updateInterpolators): the offset and the ease / transition start set the
// first time; the ease spinner (easing in / out) or weight scale (a transition's source or
// destination) from where the time is in it, ended into animating or deactivation; a morph
// source's partner's offset made to its corresponding frame (then a transition source). With
// updateInterpolators the sequence's time (a destination frame, the partner's corresponding
// frame, or the offset time) scaled and the weights and times set
void __fastcall Seq_Update(NiControllerSequence* self, re::Edx, float time, int updateInterpolators) {
    if (self->state == INACTIVE) {
        return;
    }
    if (self->offset == kInvalidFloat) {
        self->offset = -time;
    }
    if (self->startTime == kInvalidFloat) {
        self->startTime = time;
        self->endTime = self->endTime + time;
    }
    float easeSpinner = 1.0f;
    float weightScale = 1.0f;
    switch (self->state) {
    case EASINGIN:
        if (self->endTime > time) {
            easeSpinner = (time - self->startTime) / (self->endTime - self->startTime);
        } else {
            self->state = ANIMATING;
        }
        break;
    case EASINGOUT:
        if (!(self->endTime > time)) {
            Sequence_Deactivate(self, 0.0f, 0);
            return;
        }
        easeSpinner = (self->endTime - time) / (self->endTime - self->startTime);
        break;
    case MORPHSOURCE: {
        NiControllerSequence* partner = self->partnerSequence;
        float frame = Sequence_FindCorrespondingMorphFrame(partner, self, time + self->offset);
        frame = frame / self->partnerSequence->frequency;
        self->partnerSequence->offset = frame - time;
        self->state = TRANSSOURCE;
    }
        [[fallthrough]];
    case TRANSSOURCE:
        if (!(self->endTime > time)) {
            Sequence_Deactivate(self, 0.0f, 1);
            return;
        }
        weightScale = (self->endTime - time) / (self->endTime - self->startTime);
        break;
    case TRANSDEST:
        if (self->endTime > time) {
            weightScale = (time - self->startTime) / (self->endTime - self->startTime);
        } else {
            if (!(self->destFrame == kInvalidFloat)) {
                self->offset = self->destFrame - time;
                self->destFrame = kInvalidFloat;
            }
            self->state = ANIMATING;
        }
        break;
    default:
        break;
    }
    if (!re::Flag(updateInterpolators)) {
        return;
    }
    float t;
    if (!(self->destFrame == kInvalidFloat)) {
        t = self->destFrame;
    } else {
        NiControllerSequence* partner = self->partnerSequence;
        if (partner != nullptr) {
            if (!(partner->offset + time == partner->lastTime)) {
                Sequence_Update(partner, time, 0);
            }
            float frame = Sequence_FindCorrespondingMorphFrame(self, self->partnerSequence,
                                                               time + self->partnerSequence->offset);
            t = frame / self->frequency;
        } else {
            t = time + self->offset;
        }
    }
    float scaled = Sequence_ComputeScaledTime(self, t, 1);
    Sequence_SetInterpsWeightAndTime(self, self->seqWeight * weightScale, easeSpinner, scaled);
}

// a target's property of a type (its RTTI's name), or the target itself when it has none such
inline NiObjectNET* FindPropertyTarget(NiAVObject* object, const char* type) {
    for (NiTListNode<NiProperty*>* node = object->properties.head; node != nullptr;) {
        NiObject* property = re::PtrCast<NiObject*>(node->element);
        NiTListNode<NiProperty*>* next = node->next;
        const RTTI* rtti = Virtual<const RTTI*>(property, 2);
        if (crt::Strcmp(rtti->name, type) == 0) {
            return reinterpret_cast<NiObjectNET*>(property);
        }
        node = next;
    }
    return object;
}

// 0x816580: StoreTargets(root): the accumulation root found; the multi-target transform controller
// set up; each item (its interpolator's dependencies resolved, not bound yet) bound to the
// controller of its tag's type and ID on its object (or property): the stored one given the
// target when there's none (moved after the manager on the manager's target), the blend
// interpolator for its interpolator ID found or made (accumulating for the accumulation root of a
// cumulative manager); the selective update flags of the manager's target worked out again when
// anything was bound anew; the items sorted
uint8_t __fastcall Seq_StoreTargets(NiControllerSequence* self, re::Edx, NiAVObject* root) {
    NiControllerManager* owner = self->owner;
    void* palette = owner->objectPalette;
    if (self->accumRootName != sys::kNullFixedString) {
        self->accumRoot = Virtual<NiAVObject*>(palette, 18, self->accumRootName);   // GetAVObject
    }
    uint8_t changed = Sequence_SetupMultiTargetController(self, root, palette);
    for (uint32_t i = 0; i < self->arraySize; i++) {
        SeqInterpArrayItem* item = &self->interpArray[i];
        IDTag* tag = &self->idTagArray[i];
        if (item->interpolator == nullptr) {
            continue;
        }
        if (!Virtual<uint8_t>(item->interpolator, 34, palette)) {   // ResolveDependencies
            continue;
        }
        if (self->interpArray[i].blendInterp != nullptr) {
            continue;
        }
        auto* object = Virtual<NiAVObject*>(palette, 18, tag->avObjectName);
        if (object == nullptr) {
            continue;
        }
        NiObjectNET* target = object;
        if (tag->propertyType != nullptr && object->properties.head != nullptr) {
            target = FindPropertyTarget(object, tag->propertyType);
        }
        if (target == nullptr) {
            continue;
        }
        NiTimeController* ctlr = target->controller;
        for (; ctlr != nullptr; ctlr = ctlr->next) {
            const RTTI* rtti = Virtual<const RTTI*>(ctlr, 2);
            if (crt::Strcmp(rtti->name, tag->ctlrType) != 0) {
                continue;
            }
            const char* id = Virtual<const char*>(ctlr, 34);   // GetCtlrID
            if (id == nullptr) {
                if (tag->ctlrID == nullptr) {
                    break;
                }
            } else if (tag->ctlrID != nullptr && crt::Strcmp(id, tag->ctlrID) == 0) {
                break;
            }
        }
        if (ctlr == nullptr) {
            ctlr = self->interpArray[i].interpCtlr;
            if (ctlr == nullptr) {
                continue;
            }
            Virtual<void>(ctlr, 21, target);   // SetTarget
            changed = 1;
            if (target == owner->target) {
                ObjectNET_RemoveController(target, ctlr);
                TimeController_SetNext(ctlr, self->owner->next);
                TimeController_SetNext(self->owner, ctlr);
            }
        }
        uint16_t index = Virtual<uint16_t>(ctlr, 30, tag->interpolatorID);   // GetInterpolatorIndex
        if (index == kInvalidIndex) {
            continue;
        }
        NiObject* current = Virtual<NiObject*>(ctlr, 31, index);
        auto* blend = reinterpret_cast<NiBlendInterpolator*>(NiDynamicCast(&NiBlendInterpolator_RTTI, current));
        uint32_t accumulate = 0;
        if (self->accumRoot == target && re::PtrCast<uint8_t*>(self->owner)[0x58] != 0) {
            accumulate = 1;
        }
        if (blend == nullptr) {
            blend = Virtual<NiBlendInterpolator*>(ctlr, 37, uint32_t(index), 1u, accumulate, 0.0f, 2u);
            Virtual<void>(ctlr, 32, blend, uint32_t(index));   // SetInterpolator
            if (blend == nullptr) {
                continue;
            }
        }
        ctlr->flags |= 0x20;
        Pointer_Assign(reinterpret_cast<re::Ptr32<NiRefObject>*>(&self->interpArray[i].interpCtlr), ctlr);
        self->interpArray[i].blendInterp = blend;
    }
    if (changed != 0) {
        uint8_t selectiveUpdate;
        uint8_t rigid;
        Virtual<void>(self->owner->target, 24, &selectiveUpdate, 1u, &rigid);   // SetSelectiveUpdateFlags
    }
    Sequence_SortByBlendInterp(self);
    return 1;
}

// 0x816890: StartBlend(destination, duration, destTextKey, priority, sourceWeight, destWeight,
// timeSync): the destination's frame from its text key (0 when there's none or it isn't found)
uint8_t __fastcall Seq_StartBlendAtKey(NiControllerSequence* self, re::Edx, NiControllerSequence* destination,
                                       float duration, const char* destTextKey, int priority, float sourceWeight,
                                       float destWeight, NiControllerSequence* timeSync) {
    float destFrame = 0.0f;
    if (destTextKey != nullptr) {
        NiFixedString key = scene::StringTable_AddString(destTextKey);
        float time = Sequence_GetKeyTimeAt(destination, &key);
        FixedRelease(&key);
        if (!(time == kInvalidFloat)) {
            destFrame = time;
        }
    }
    return Sequence_StartBlend(self, destination, duration, destFrame, priority, sourceWeight, destWeight, timeSync);
}

// 0x816950: ReallocateArrays: the arrays grown by the growth step (none: false), the items and
// tags copied over
uint8_t __fastcall Seq_ReallocateArrays(NiControllerSequence* self, re::Edx) {
    if (self->arrayGrowBy == 0) {
        return 0;
    }
    uint32_t size = self->arraySize + self->arrayGrowBy;
    SeqInterpArrayItem* items = NewItems(size);
    IDTag* tags = NewTags(size);
    for (uint32_t i = 0; i < size; i++) {
        if (i >= self->arraySize) {
            continue;
        }
        InterpArrayItem_Assign(&items[i], &self->interpArray[i]);
        IDTag* source = &self->idTagArray[i];
        FixedString_Assign(&tags[i].avObjectName, &source->avObjectName);
        FixedString_Assign(&tags[i].propertyType, &source->propertyType);
        FixedString_Assign(&tags[i].ctlrType, &source->ctlrType);
        FixedString_Assign(&tags[i].ctlrID, &source->ctlrID);
        FixedString_Assign(&tags[i].interpolatorID, &source->interpolatorID);
    }
    if (self->interpArray != nullptr) {
        InterpArrayItem_VectorDeletingDestructor(self->interpArray, 3);
    }
    if (self->idTagArray != nullptr) {
        DeleteTags(self->idTagArray);
    }
    self->arraySize = size;
    self->idTagArray = tags;
    self->interpArray = items;
    return 1;
}

// 0x816B00: CopyMembers(dest, cloning): the name, arrays (the interpolators' shared clones; without
// an owner the controllers' clones, with one the blend indices), tags, weight, text keys (shared),
// cycle, frequency, key range, accumulation root name and +0x78
void __fastcall Seq_CopyMembers(NiControllerSequence* self, re::Edx, NiControllerSequence* dest, void* cloning) {
    Object_CopyMembers(self, dest, cloning);
    FixedString_Assign(&dest->name, &self->name);
    dest->arraySize = self->arraySize;
    dest->arrayGrowBy = self->arrayGrowBy;
    dest->interpArray = NewItems(self->arraySize);
    dest->idTagArray = NewTags(self->arraySize);
    for (uint32_t i = 0; i < self->arraySize; i++) {
        if (self->interpArray[i].interpolator != nullptr) {
            NiObject* clone = Object_CreateSharedClone(self->interpArray[i].interpolator, cloning);
            AssignReleasingFirst(dest->interpArray[i].interpolator, clone);
            if (self->owner == nullptr) {
                NiTimeController* ctlr = self->interpArray[i].interpCtlr;
                if (ctlr != nullptr) {
                    auto* ctlrClone = Virtual<NiTimeController*>(ctlr, 3, cloning);   // CreateClone
                    AssignReleasingFirst(dest->interpArray[i].interpCtlr, ctlrClone);
                }
            } else {
                dest->interpArray[i].blendIdx = self->interpArray[i].blendIdx;
            }
        }
        TagAssign(&dest->idTagArray[i], &self->idTagArray[i]);
    }
    dest->seqWeight = self->seqWeight;
    AssignReleasingFirst(dest->textKeys, self->textKeys);
    dest->cycleType = self->cycleType;
    dest->frequency = self->frequency;
    dest->beginKeyTime = self->beginKeyTime;
    dest->endKeyTime = self->endKeyTime;
    FixedAssign(&dest->accumRootName, self->accumRootName);
    dest->unknown78 = self->unknown78;
}

// 0x816FA0: LoadBinary. Before 10.1.0.104: the name and accumulation root name inline, the text
// keys link, the controller count (growth step 12), per controller its target's name (in the
// tag's object name) and link. After: the name, sizes, items and tags, weight, text keys link,
// cycle, frequency (an old phase read past before 10.3.0.1), key range (an old byte before
// 10.1.0.107), owner link, accumulation root name, +0x78 from 20.3.0.8, the string palette
// (10.1.0.113 to 20.1.0.1; else none)
void __fastcall Seq_LoadBinary(NiControllerSequence* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    uint32_t version = StreamVersion(stream);
    if (version < kVersion_10_1_0_104) {
        Stream_LoadInlineFixedString(stream, &self->name);
        Stream_LoadInlineFixedString(stream, &self->accumRootName);
        Stream_ReadLinkID(stream);
        LoadBytes(stream, &self->arraySize, 4);
        self->arrayGrowBy = 0xC;
        self->interpArray = NewItems(self->arraySize);
        self->idTagArray = NewTags(self->arraySize);
        for (uint32_t i = 0; i < self->arraySize; i++) {
            NiFixedString name = sys::kNullFixedString;
            Stream_LoadInlineFixedString(stream, &name);
            FixedAssign(&self->idTagArray[i].avObjectName, name);
            Stream_ReadLinkID(stream);
            FixedRelease(&name);
        }
        return;
    }
    if (version < kVersion_20_1_0_1) {
        Stream_LoadInlineFixedString(stream, &self->name);
    } else {
        Stream_LoadFixedString(stream, &self->name);
    }
    LoadBytes(stream, &self->arraySize, 4);
    LoadBytes(stream, &self->arrayGrowBy, 4);
    self->interpArray = NewItems(self->arraySize);
    self->idTagArray = NewTags(self->arraySize);
    for (uint32_t i = 0; i < self->arraySize; i++) {
        InterpArrayItem_LoadBinary(&self->interpArray[i], stream);
        re::Ptr32<NiObject> palette;
        IDTag_LoadBinary(&self->idTagArray[i], stream, &palette);
    }
    LoadBytes(stream, &self->seqWeight, 4);
    Stream_ReadLinkID(stream);
    uint32_t cycleType;
    LoadBytes(stream, &cycleType, 4);
    self->cycleType = cycleType;
    LoadBytes(stream, &self->frequency, 4);
    if (StreamVersion(stream) < kVersion_10_3_0_1) {
        float phase;
        LoadBytes(stream, &phase, 4);
    }
    LoadBytes(stream, &self->beginKeyTime, 4);
    LoadBytes(stream, &self->endKeyTime, 4);
    if (StreamVersion(stream) < kVersion_10_1_0_107) {
        uint8_t played;
        LoadBytes(stream, &played, 1);
    }
    Stream_ReadLinkID(stream);
    if (StreamVersion(stream) < kVersion_20_1_0_1) {
        Stream_LoadInlineFixedString(stream, &self->accumRootName);
    } else {
        Stream_LoadFixedString(stream, &self->accumRootName);
    }
    if (StreamVersion(stream) >= kVersion_20_3_0_8) {
        LoadBytes(stream, &self->unknown78, 4);
    }
    version = StreamVersion(stream);
    if (version >= kVersion_10_1_0_113 && version < kVersion_20_1_0_1) {
        NiObject* palette = Stream_ReadLinkIDObject(stream);
        AssignReleasingFirst(self->stringPalette, palette);
        return;
    }
    if (self->stringPalette != nullptr) {
        ReleaseRef(self->stringPalette);
        self->stringPalette = nullptr;
    }
}

// 0x8174B0: the deleting destructor
NiControllerSequence* __fastcall Seq_DeletingDestructor(NiControllerSequence* self, re::Edx, uint32_t flags) {
    Sequence_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x7C);
    }
    return self;
}

// 0x8174D0: AddInterpolator(interpolator, tag): into the first free item (the arrays grown when
// there's none); its index, or -1 when they can't grow
uint32_t __fastcall Seq_AddInterpolator(NiControllerSequence* self, re::Edx, NiInterpolator* interpolator,
                                        const IDTag* tag) {
    uint32_t index = 0;
    uint32_t size = self->arraySize;
    while (index < size && self->interpArray[index].interpolator != nullptr) {
        index++;
    }
    if (index == size && !Sequence_ReallocateArrays(self)) {
        return 0xFFFFFFFF;
    }
    AssignReleasingFirst(self->interpArray[index].interpolator, interpolator);
    TagAssign(&self->idTagArray[index], tag);
    return index;
}

// 0x8176E0: AddInterpController(idString, ctlr): each of the controller's interpolators added with
// its tag (the object's name from the ID string's first line; the property type from its third
// when the second is "PROP"; the controller's type and ID; the interpolator's ID), the controller
// stored with it. False when one can't be added
uint8_t __fastcall Seq_AddInterpController(NiControllerSequence* self, re::Edx, const char* idString,
                                           NiTimeController* ctlr) {
    char objectName[0x104];
    char second[0x104];
    char third[0x104];
    crt::StrcpyS(objectName, 0x104, idString);
    second[0] = '\0';
    third[0] = '\0';
    char* newline = crt::Strchr(objectName, '\n');
    if (newline != nullptr) {
        crt::StrcpyS(second, 0x104, newline + 1);
        *newline = '\0';
        newline = crt::Strchr(second, '\n');
        crt::StrcpyS(third, 0x104, newline + 1);
        *newline = '\0';
    }
    for (uint16_t i = 0; i < Virtual<uint16_t>(ctlr, 28); i++) {   // GetInterpolatorCount
        NiFixedString empty[5] = {sys::kNullFixedString, sys::kNullFixedString, sys::kNullFixedString,
                                  sys::kNullFixedString, sys::kNullFixedString};
        IDTag tag;
        IDTag_Construct(&tag, &empty[0], &empty[1], &empty[2], &empty[3], &empty[4]);
        FixedRelease(&empty[4]);
        FixedRelease(&empty[1]);
        FixedRelease(&empty[3]);
        FixedRelease(&empty[2]);
        FixedRelease(&empty[0]);

        NiFixedString value = scene::StringTable_AddString(objectName);
        FixedAssign(&tag.avObjectName, value);
        FixedRelease(&value);
        if (std::memcmp(second, "PROP", 5) == 0) {
            value = scene::StringTable_AddString(third);
            FixedAssign(&tag.propertyType, value);
            FixedRelease(&value);
        }
        const RTTI* rtti = Virtual<const RTTI*>(ctlr, 2);
        value = rtti->name != nullptr ? scene::StringTable_AddString(rtti->name) : sys::kNullFixedString;
        FixedAssign(&tag.ctlrType, value);
        FixedRelease(&value);
        const char* id = Virtual<const char*>(ctlr, 34);   // GetCtlrID
        value = id != nullptr ? scene::StringTable_AddString(id) : sys::kNullFixedString;
        FixedAssign(&tag.ctlrID, value);
        FixedRelease(&value);
        const char* interpolatorID = Virtual<const char*>(ctlr, 29, uint32_t(i));   // GetInterpolatorID
        value = interpolatorID != nullptr ? scene::StringTable_AddString(interpolatorID) : sys::kNullFixedString;
        FixedAssign(&tag.interpolatorID, value);
        FixedRelease(&value);

        NiInterpolator* interpolator = Virtual<NiInterpolator*>(ctlr, 31, uint32_t(i));
        uint32_t index = Sequence_AddInterpolator(self, interpolator, &tag);
        if (index == 0xFFFFFFFF) {
            IDTag_Destruct(&tag);
            return 0;
        }
        re::Ptr32<NiTimeController>& slot = self->interpArray[index].interpCtlr;
        if (slot != ctlr) {
            ReleaseRef(slot);
            slot = ctlr;
            sys::Increment(&ctlr->refCount);
        }
        IDTag_Destruct(&tag);
    }
    return 1;
}

// 0x817C80: CreateClone (a failed allocation still copied onto null, as the game has it)
NiControllerSequence* __fastcall Seq_CreateClone(NiControllerSequence* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x7C);
    if (memory == nullptr) {
        Sequence_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiControllerSequence* clone = Sequence_Construct(memory);
    Sequence_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x817CD0: CreateAllSequencesFromFile(stream, sequences) (static): every top-level sequence of a
// loaded stream added to the array (emptied first)
uint8_t __cdecl Seq_CreateAllSequencesFromStream(void* stream, NiTArray<NiControllerSequence*>* sequences) {
    PointerArray_RemoveAll(sequences);
    for (uint32_t i = 0; i < sys::At<uint32_t>(stream, 0x164); i++) {
        re::Ptr32<NiControllerSequence> sequence;
        Sequence_CreateSequenceFromFile(&sequence, stream, i);
        if (sequence == nullptr) {
            continue;
        }
        uint32_t index = sequences->size;
        if (sequences->maxSize <= index) {
            PointerArray_SetSize(sequences, sequences->growBy + index);
        }
        PointerArray_SetAt(sequences, index, re::PtrCast<const re::Ptr32<ni::NiRefObject>*>(&sequence));
        ReleaseRef(sequence);
    }
    return 1;
}

// 0x817D60: AddConvertedController(idString, ctlr): an older file's interp controller given the
// sequence's cycle type (not played backwards) and frequency, then added
uint8_t __fastcall Seq_AddConvertedController(NiControllerSequence* self, re::Edx, const char* idString,
                                              NiTimeController* ctlr) {
    if (idString == nullptr || ctlr == nullptr || !IsKindOf(ctlr, kNiInterpControllerClassIndex)) {
        return 0;
    }
    ctlr->flags = uint16_t(uint16_t(self->cycleType) * 2) | (ctlr->flags & 0xFFF9);
    ctlr->flags &= 0xFFEF;
    ctlr->frequency = self->frequency;
    return Sequence_AddInterpController(self, idString, ctlr);
}

// 0x817DE0: PostLinkObject: the tags' palette offsets made into strings (the palette let go).
// Before 10.1.0.104: the key range from the controllers' (0 and 0 when there's none), each
// controller's target name taken out of its tag (the tags emptied, the items cleared), then each
// controller added through AddConvertedController (the first's cycle type and frequency taken:
// a reverse cycle as a loop)
void __fastcall Seq_PostLinkObject(NiControllerSequence* self, re::Edx, void* stream) {
    Object_PostLinkObject(self, stream);
    if (self->stringPalette != nullptr) {
        for (uint32_t i = 0; i < self->arraySize; i++) {
            IDTag_ConvertFromPalette(&self->idTagArray[i], self->stringPalette, stream);
        }
        if (self->stringPalette != nullptr) {
            ReleaseRef(self->stringPalette);
            self->stringPalette = nullptr;
        }
    }
    if (StreamVersion(stream) >= kVersion_10_1_0_104) {
        return;
    }
    self->beginKeyTime = 3.40282347e+38f;
    self->endKeyTime = kInvalidFloat;
    uint32_t count = self->arraySize;
    for (uint32_t i = 0; i < count; i++) {
        NiTimeController* ctlr = self->interpArray[i].interpCtlr;
        if (self->beginKeyTime > ctlr->loKeyTime) {
            self->beginKeyTime = ctlr->loKeyTime;
        }
        if (self->endKeyTime < ctlr->hiKeyTime) {
            self->endKeyTime = ctlr->hiKeyTime;
        }
    }
    if (self->beginKeyTime == 3.40282347e+38f && self->endKeyTime == kInvalidFloat) {
        self->endKeyTime = 0.0f;
        self->beginKeyTime = 0.0f;
    }
    uint16_t nameCount = 0;
    re::Ptr32<char>* names = uint16_t(count) != 0 ? static_cast<re::Ptr32<char>*>(NiMalloc(uint32_t(uint16_t(count)) * 4)) : nullptr;
    // the controllers kept (a NiTObjectArray on the stack, its storage NiNew[]'d and zeroed)
    NiTArray<NiRefObject*> ctlrs;
    uint16_t arrayCount = uint16_t(self->arraySize);
    ctlrs.maxSize = arrayCount;
    ctlrs.growBy = 1;
    ctlrs.size = 0;
    ctlrs.esize = 0;
    ctlrs.base = nullptr;
    if (arrayCount != 0) {
        uint32_t* block = static_cast<uint32_t*>(NiAllocArray(KeyArraySize(arrayCount, 4)));
        if (block != nullptr) {
            *block = arrayCount;
            re::Ptr32<NiRefObject>* base = reinterpret_cast<re::Ptr32<NiRefObject>*>(block + 1);
            for (uint32_t k = 0; k < arrayCount; k++) {
                base[k] = nullptr;
            }
            ctlrs.base = base;
        }
    }
    ctlrs.vtable = reinterpret_cast<void*>(ControllerArray_vtable);
    for (uint32_t i = 0; i < self->arraySize; i++) {
        const char* name = self->idTagArray[i].avObjectName;
        size_t length = std::strlen(name) + 1;   // (inlined)
        char* copy = static_cast<char*>(NiMalloc(uint32_t(length)));
        crt::StrcpyS(copy, length, name);
        if (nameCount <= i) {
            nameCount = uint16_t(i + 1);
        }
        names[i] = copy;
        IDTag* tag = &self->idTagArray[i];
        re::Ptr32<const char>* fields[5] = {&tag->avObjectName, &tag->propertyType, &tag->ctlrType, &tag->ctlrID,
                                    &tag->interpolatorID};
        for (re::Ptr32<const char>* field : fields) {
            if (*field != nullptr) {
                NiFixedString old = *field;
                *field = scene::StringTable_AddString(nullptr);
                FixedRelease(&old);
            }
        }
        NiRefObject* ctlr = self->interpArray[i].interpCtlr;
        re::Ptr32<NiRefObject> held = ctlr;
        Hold(ctlr);
        PointerArray_SetAt(&ctlrs, i, &held);
        ReleaseRef(ctlr);
        InterpArrayItem_ClearValues(&self->interpArray[i]);
    }
    for (uint32_t i = 0; i < ctlrs.size; i++) {
        auto* ctlr = re::PtrCast<NiTimeController*>(ctlrs.base[i]);
        if (i == 0) {
            uint32_t cycle = (uint32_t(uint8_t(ctlr->flags)) >> 1) & 3;
            self->cycleType = cycle;
            if (cycle != 2 && cycle != 0) {
                self->cycleType = 0;
            }
            self->frequency = ctlr->frequency;
        }
        Sequence_AddConvertedController(self, names[i], ctlr);
    }
    for (uint32_t i = 0; i < nameCount; i++) {
        NiFree(names[i]);
    }
    NiDeletePointerArray(ctlrs.base);
    NiFree(names);
}

// 0x818240: CreateAllSequencesFromFile(filename, sequences) (static): the file loaded into a
// stream of its own, its sequences taken (false when it doesn't load)
uint8_t __cdecl Seq_CreateAllSequencesFromFile(const char* filename, NiTArray<NiControllerSequence*>* sequences) {
    alignas(4) uint8_t stream[0x40C];
    Stream_Construct(stream);
    if (!Stream_Load(stream, filename)) {
        Stream_Destruct(stream);
        return 0;
    }
    uint8_t result = Sequence_CreateAllSequencesFromStream(stream, sequences);
    Stream_Destruct(stream);
    return result;
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_SEQ3_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC31_SEQ3_REVERSED("NiControllerSequence::SaveBinary", 0x008156E0, &ni::anim::Seq_SaveBinary);
NIC31_SEQ3_REVERSED("NiControllerSequence::CreateObject", 0x008158C0, &ni::anim::Seq_CreateObject);
NIC31_SEQ3_REVERSED("NiControllerSequence::~NiControllerSequence", 0x00815B40, &ni::anim::Seq_Destruct);
NIC31_SEQ3_REVERSED("NiControllerSequence::SortByBlendInterp", 0x00815CB0, &ni::anim::Seq_SortByBlendInterp);
NIC31_SEQ3_REVERSED("NiControllerSequence::Update", 0x00816320, &ni::anim::Seq_Update);
NIC31_SEQ3_REVERSED("NiControllerSequence::StoreTargets", 0x00816580, &ni::anim::Seq_StoreTargets);
NIC31_SEQ3_REVERSED("NiControllerSequence::StartBlend(key)", 0x00816890, &ni::anim::Seq_StartBlendAtKey);
NIC31_SEQ3_REVERSED("NiControllerSequence::ReallocateArrays", 0x00816950, &ni::anim::Seq_ReallocateArrays);
NIC31_SEQ3_REVERSED("NiControllerSequence::CopyMembers", 0x00816B00, &ni::anim::Seq_CopyMembers);
NIC31_SEQ3_REVERSED("NiControllerSequence::LoadBinary", 0x00816FA0, &ni::anim::Seq_LoadBinary);
NIC31_SEQ3_REVERSED("NiControllerSequence::DeletingDestructor", 0x008174B0, &ni::anim::Seq_DeletingDestructor);
NIC31_SEQ3_REVERSED("NiControllerSequence::AddInterpolator", 0x008174D0, &ni::anim::Seq_AddInterpolator);
NIC31_SEQ3_REVERSED("NiControllerSequence::AddInterpController", 0x008176E0, &ni::anim::Seq_AddInterpController);
NIC31_SEQ3_REVERSED("NiControllerSequence::CreateClone", 0x00817C80, &ni::anim::Seq_CreateClone);
NIC31_SEQ3_REVERSED("NiControllerSequence::CreateAllSequencesFromFile(stream)", 0x00817CD0,
                    &ni::anim::Seq_CreateAllSequencesFromStream);
NIC31_SEQ3_REVERSED("NiControllerSequence::AddConvertedController", 0x00817D60,
                    &ni::anim::Seq_AddConvertedController);
NIC31_SEQ3_REVERSED("NiControllerSequence::PostLinkObject", 0x00817DE0, &ni::anim::Seq_PostLinkObject);
NIC31_SEQ3_REVERSED("NiControllerSequence::CreateAllSequencesFromFile", 0x00818240,
                    &ni::anim::Seq_CreateAllSequencesFromFile);
