// NiControllerSequence (vtable 0x966D38, 0x7C bytes: src/gamebryo/NiControllerSequence.h), the
// first part: its time (ComputeScaledTime), the text key queries (GetKeyTimeAt, GetTimeAt and
// their text key match forms), the interpolators' attach / detach to their blend interpolators,
// whether a sequence can be synchronised to another (the "morph:" keys), the IDTag and interp
// array item helpers, construction, streaming links, comparison, the static that takes a sequence
// from a loaded stream. The rest: src/gamebryo/NiControllerSequence2.cpp.
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's order.

#include <cmath>
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

using scene::Stream_ResolveLinkID;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiControllerSequence_RTTI, 0x020E509C);
GAME_VAR(uint32_t, kNiControllerSequenceClassIndex, 0x020E50A4);
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(uint8_t, kInvalidBlendIndex, 0x009683CC);   // NiBlendInterpolator::INVALID_INDEX (0xFF)

constexpr uintptr_t NiControllerSequence_vtable = 0x00966D38;
constexpr uintptr_t CallbackArray_vtable = 0x00961988;   // NiTPrimitiveArray<ActivationCallback*>
constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiTextKeyMatch_vtable = 0x00965A60;
constexpr uint32_t kVersion_10_1_0_104 = 0x0A010068;

GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, Object_Construct, 0x00755260, void* self);
GAME_FN(uint8_t, __thiscall, Object_RegisterStreamables, 0x00755230, void* self, void* stream);
GAME_FN(void, __thiscall, Object_LinkObject, 0x008A20A0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Object_IsEqual, 0x007552A0, void* self, void* other);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(void, __thiscall, TextKeyMatch_Destruct, 0x00800B20, void* self);
// ~NiMaterialInstance (the same two references let go: an item's, folded)
GAME_FN(void, __thiscall, InterpArrayItem_DestructFolded, 0x00761500, SeqInterpArrayItem* self);
// (this file's, called where modes apply)
GAME_FN(float, __thiscall, Sequence_ComputeScaledTime, 0x00813A90, NiControllerSequence* self, float time,
        int storeLastTime);
GAME_FN(float, __thiscall, Sequence_GetKeyTimeAtMatch, 0x00813850, NiControllerSequence* self, void* match);
GAME_FN(float, __thiscall, Sequence_GetTimeAtMatch, 0x008144F0, NiControllerSequence* self, void* match, float time);
GAME_FN(uint8_t, __thiscall, InterpArrayItem_IsEqual, 0x00813D80, SeqInterpArrayItem* self, SeqInterpArrayItem* other);
GAME_FN(uint8_t, __thiscall, IDTag_IsEqual, 0x00813DF0, IDTag* self, IDTag* other);

// NiTextKeyMatch as these build it on the stack (NiRefObject's constructor inlined: the object
// counted): the key to match, no last match
struct TextKeyMatch {
    re::Ptr32<void> vtable;                 // +0x0
    uint32_t refCount;            // +0x4
    NiFixedString lastMatchedKey; // +0x8
    NiFixedString matchKey;       // +0xC
};

inline void ConstructMatch(TextKeyMatch& match, const re::Ptr32<const char>* key) {
    match.vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    match.refCount = 0;
    sys::Increment(&NiRefObject_ms_uiObjects);
    match.vtable = reinterpret_cast<void*>(NiTextKeyMatch_vtable);
    match.lastMatchedKey = sys::kNullFixedString;
    match.matchKey = sys::kNullFixedString;
    FixedString_Assign(&match.matchKey, key);
}

// a handle let go (NiFixedString's destructor, as the code inlines it: the import's decrement)
inline void FixedRelease(re::Ptr32<const char>* handle) {
    sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(handle));
}

// ---- NiControllerSequence

// 0x8137D0: VerifyDependencies(sequence): false when this one is in the sequence's chain of
// partners (synchronising would loop)
uint8_t __fastcall Seq_VerifyDependencies(NiControllerSequence* self, re::Edx, NiControllerSequence* sequence) {
    for (NiControllerSequence* partner = sequence->partnerSequence; partner != nullptr;
         partner = partner->partnerSequence) {
        if (partner == self) {
            return 0;
        }
    }
    return 1;
}

// 0x813850: GetKeyTimeAt(match): the first text key's time the match takes (-FLT_MAX: none)
float __fastcall Seq_GetKeyTimeAtMatch(NiControllerSequence* self, re::Edx, void* match) {
    if (match == nullptr || self->textKeys == nullptr) {
        return kInvalidFloat;
    }
    NiTextKey* keys = self->textKeys->keys;
    uint32_t count = self->textKeys->numKeys;
    for (uint32_t i = 0; i < count; i++) {
        if (Virtual<uint8_t>(match, 2, &keys[i].text)) {
            return keys[i].time;
        }
    }
    return kInvalidFloat;
}

// 0x8138C0: InterpArrayItem::operator=
SeqInterpArrayItem* __fastcall Item_Assign(SeqInterpArrayItem* self, re::Edx, const SeqInterpArrayItem* other) {
    AssignReleasingFirst(self->interpolator, other->interpolator);
    AssignReleasingFirst(self->interpCtlr, other->interpCtlr);
    self->blendInterp = other->blendInterp;
    self->blendIdx = other->blendIdx;
    return self;
}

// 0x813950: ClearTargets: every item's controller let go and its blend interpolator forgotten
void __fastcall Seq_ClearTargets(NiControllerSequence* self, re::Edx) {
    for (uint32_t i = 0; i < self->arraySize; i++) {
        SeqInterpArrayItem& item = self->interpArray[i];
        if (item.interpCtlr != nullptr) {
            ReleaseRef(item.interpCtlr);
            item.interpCtlr = nullptr;
        }
        self->interpArray[i].blendInterp = nullptr;
    }
}

// 0x8139C0: AttachInterpolators(priority): each interpolator added to its blend interpolator
// (no weight yet, ease spinner 1)
void __fastcall Seq_AttachInterpolators(NiControllerSequence* self, re::Edx, int priority) {
    for (uint32_t i = 0; i < self->arraySize; i++) {
        SeqInterpArrayItem& item = self->interpArray[i];
        if (item.interpolator != nullptr && item.blendInterp != nullptr) {
            item.blendIdx = Virtual<uint8_t>(item.blendInterp, 37, item.interpolator, 0.0f, priority, 1.0f);
        }
    }
}

// 0x813A20: DetachInterpolators: each one taken out of its blend interpolator
void __fastcall Seq_DetachInterpolators(NiControllerSequence* self, re::Edx) {
    for (uint32_t i = 0; i < self->arraySize; i++) {
        SeqInterpArrayItem& item = self->interpArray[i];
        if (item.blendInterp == nullptr) {
            continue;
        }
        NiInterpolator* removed;
        Virtual<re::Ptr32<NiInterpolator>*>(item.blendInterp, 38, &removed, uint32_t(item.blendIdx));
        ReleaseRef(removed);
    }
}

// 0x813A90: ComputeScaledTime(time, storeLastTime): the time since the last update scaled by the
// frequency and added on (from 0 the first time); looped into the key range (unless it lands on
// its end), then clamped to it. With storeLastTime the times kept
float __fastcall Seq_ComputeScaledTime(NiControllerSequence* self, re::Edx, float time, int storeLastTime) {
    float weighted = self->weightedLastTime;
    float delta;
    if (self->lastTime == kInvalidFloat) {
        weighted = 0.0f;
        delta = time;
    } else {
        delta = time - self->lastTime;
    }
    weighted = self->frequency * delta + weighted;
    float scaled = weighted;
    if (self->cycleType == 0) {
        float span = self->endKeyTime - self->beginKeyTime;
        if (span == 0.0f) {
            scaled = self->beginKeyTime;
        } else {
            if (!(self->endKeyTime == weighted)) {
                float offset = weighted - self->beginKeyTime;
                float r = float(std::fmod(double(offset), double(span)));
                scaled = r + self->beginKeyTime;
            }
            if (self->beginKeyTime > scaled) {
                scaled = scaled + span;
            }
        }
    }
    if (self->endKeyTime < scaled) {
        scaled = self->endKeyTime;
    } else if (self->beginKeyTime > scaled) {
        scaled = self->beginKeyTime;
    }
    if (re::Flag(storeLastTime)) {
        self->weightedLastTime = weighted;
        self->lastTime = time;
        self->lastScaledTime = scaled;
    }
    return scaled;
}

// 0x813BC0: CanSyncTo(partner): both have text keys, and every "morph:" key of each is in the
// other's (at least one there)
uint8_t __fastcall Seq_CanSyncTo(NiControllerSequence* self, re::Edx, NiControllerSequence* partner) {
    NiTextKeyExtraData* mine = self->textKeys;
    if (mine == nullptr) {
        return 0;
    }
    NiTextKeyExtraData* theirs = partner->textKeys;
    if (theirs == nullptr) {
        return 0;
    }
    uint32_t myCount = mine->numKeys;
    NiTextKey* myKeys = mine->keys;
    uint32_t theirCount = theirs->numKeys;
    NiTextKey* theirKeys = theirs->keys;
    bool found = false;
    for (uint32_t i = 0; i < theirCount; i++) {
        const char* text = theirKeys[i].text;
        if (crt::Strncmp(text, "morph:", 6) != 0) {
            continue;
        }
        found = true;
        uint32_t j = 0;
        while (j < myCount && crt::Strcmp(text, myKeys[j].text) != 0) {
            j++;
        }
        if (j >= myCount) {
            return 0;
        }
    }
    if (!found) {
        return 0;
    }
    found = false;
    for (uint32_t i = 0; i < myCount; i++) {
        const char* text = myKeys[i].text;
        if (crt::Strncmp(text, "morph:", 6) != 0) {
            continue;
        }
        found = true;
        uint32_t j = 0;
        while (j < theirCount && crt::Strcmp(text, theirKeys[j].text) != 0) {
            j++;
        }
        if (j >= theirCount) {
            return 0;
        }
    }
    return found ? 1 : 0;
}

// 0x813D80: InterpArrayItem::IsEqual (the interpolators and controllers both missing or equal)
uint8_t __fastcall Item_IsEqual(SeqInterpArrayItem* self, re::Edx, SeqInterpArrayItem* other) {
    NiInterpolator* mine = self->interpolator;
    NiInterpolator* theirs = other->interpolator;
    if (mine != nullptr) {
        if (theirs == nullptr) {
            return 0;
        }
    } else if (theirs != nullptr) {
        return 0;
    }
    if (mine != nullptr && theirs != nullptr && !Virtual<uint8_t>(mine, 8, theirs)) {
        return 0;
    }
    NiTimeController* myCtlr = self->interpCtlr;
    if (myCtlr != nullptr) {
        if (other->interpCtlr == nullptr) {
            return 0;
        }
    } else if (other->interpCtlr != nullptr) {
        return 0;
    }
    if (myCtlr != nullptr && other->interpCtlr != nullptr && !Virtual<uint8_t>(myCtlr, 8, other->interpCtlr)) {
        return 0;
    }
    return 1;
}

// 0x813DF0: IDTag::IsEqual (the same five handles)
uint8_t __fastcall Tag_IsEqual(IDTag* self, re::Edx, IDTag* other) {
    if (self->avObjectName == other->avObjectName && self->propertyType == other->propertyType &&
        self->ctlrType == other->ctlrType && self->ctlrID == other->ctlrID &&
        self->interpolatorID == other->interpolatorID) {
        return 1;
    }
    return 0;
}

// 0x813E30: RegisterStreamables: the interpolators, controllers, tag strings, text keys, name and
// accumulation root's name
uint8_t __fastcall Seq_RegisterStreamables(NiControllerSequence* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    for (uint32_t i = 0; i < self->arraySize; i++) {
        SeqInterpArrayItem* item = &self->interpArray[i];
        if (item->interpolator != nullptr) {
            Virtual<uint8_t>(item->interpolator, 6, stream);
        }
        if (item->interpCtlr != nullptr) {
            Virtual<uint8_t>(item->interpCtlr, 6, stream);
        }
        IDTag* tag = &self->idTagArray[i];
        Virtual<void>(stream, 8, &tag->avObjectName);   // RegisterFixedString
        Virtual<void>(stream, 8, &tag->propertyType);
        Virtual<void>(stream, 8, &tag->ctlrType);
        Virtual<void>(stream, 8, &tag->ctlrID);
        Virtual<void>(stream, 8, &tag->interpolatorID);
    }
    if (self->textKeys != nullptr) {
        Virtual<uint8_t>(self->textKeys, 6, stream);
    }
    Virtual<void>(stream, 8, &self->name);
    Virtual<void>(stream, 8, &self->accumRootName);
    return 1;
}

// 0x813F20: LinkObject: the text keys; before 10.1.0.104 each item's controller, after it the
// owner
void __fastcall Seq_LinkObject(NiControllerSequence* self, re::Edx, void* stream) {
    Object_LinkObject(self, stream);
    if (StreamVersion(stream) < kVersion_10_1_0_104) {
        AssignReleasingFirst(self->textKeys, static_cast<NiTextKeyExtraData*>(Stream_ResolveLinkID(stream)));
        for (uint32_t i = 0; i < self->arraySize; i++) {
            NiTimeController* ctlr = static_cast<NiTimeController*>(Stream_ResolveLinkID(stream));
            AssignReleasingFirst(self->interpArray[i].interpCtlr, ctlr);
        }
        return;
    }
    AssignReleasingFirst(self->textKeys, static_cast<NiTextKeyExtraData*>(Stream_ResolveLinkID(stream)));
    self->owner = static_cast<NiControllerManager*>(Stream_ResolveLinkID(stream));
}

// a pointer pair compared as the IsEquals have it: both missing, or both there and equal (slot 8)
inline bool BothEqual(void* mine, void* theirs) {
    if (mine != nullptr) {
        if (theirs == nullptr) {
            return false;
        }
    } else if (theirs != nullptr) {
        return false;
    }
    if (mine != nullptr && theirs != nullptr && !Virtual<uint8_t>(mine, 8, theirs)) {
        return false;
    }
    return true;
}

// 0x814050: IsEqual (the name and sizes, every item and tag, the weight, text keys, cycle,
// frequency, key range, owner, accumulation root name and the field at +0x78)
uint8_t __fastcall Seq_IsEqual(NiControllerSequence* self, re::Edx, NiControllerSequence* other) {
    if (!Object_IsEqual(self, other) || self->name != other->name || self->arraySize != other->arraySize ||
        self->arrayGrowBy != other->arrayGrowBy) {
        return 0;
    }
    for (uint32_t i = 0; i < self->arraySize; i++) {
        if (!InterpArrayItem_IsEqual(&self->interpArray[i], &other->interpArray[i])) {
            return 0;
        }
        if (!IDTag_IsEqual(&self->idTagArray[i], &other->idTagArray[i])) {
            return 0;
        }
    }
    if (!(other->seqWeight == self->seqWeight)) {
        return 0;
    }
    if (!BothEqual(self->textKeys, other->textKeys)) {
        return 0;
    }
    if (self->cycleType != other->cycleType) {
        return 0;
    }
    if (!(other->frequency == self->frequency) || !(other->beginKeyTime == self->beginKeyTime) ||
        !(other->endKeyTime == self->endKeyTime)) {
        return 0;
    }
    if (!BothEqual(self->owner, other->owner)) {
        return 0;
    }
    if (self->accumRootName != other->accumRootName) {
        return 0;
    }
    return self->unknown78 == other->unknown78 ? 1 : 0;
}

// 0x8141B0: SeqInterpArrayItem()
SeqInterpArrayItem* __fastcall Item_Construct(SeqInterpArrayItem* self, re::Edx) {
    self->interpolator = nullptr;
    self->interpCtlr = nullptr;
    self->blendInterp = nullptr;
    self->blendIdx = kInvalidBlendIndex;
    return self;
}

// 0x8141D0: InterpArrayItem::ClearValues
void __fastcall Item_ClearValues(SeqInterpArrayItem* self, re::Edx) {
    if (self->interpolator != nullptr) {
        ReleaseRef(self->interpolator);
        self->interpolator = nullptr;
    }
    if (self->interpCtlr != nullptr) {
        ReleaseRef(self->interpCtlr);
        self->interpCtlr = nullptr;
    }
    self->blendInterp = nullptr;
    self->blendIdx = kInvalidBlendIndex;
}

// 0x814250: SeqInterpArrayItem's vector deleting destructor (flag 2: the NiNew[] array, each item's
// controller then interpolator let go from the last; flag 1: freed)
void* __fastcall Item_VectorDeletingDestructor(SeqInterpArrayItem* self, re::Edx, uint32_t flags) {
    if ((flags & 2) != 0) {
        uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
        for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
            ReleaseRef(self[i].interpCtlr);
            ReleaseRef(self[i].interpolator);
        }
        if ((flags & 1) != 0) {
            NiFreeArray(block, sizeof(SeqInterpArrayItem));
        }
        return block;
    }
    InterpArrayItem_DestructFolded(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(SeqInterpArrayItem));
    }
    return self;
}

// 0x8142F0: ~IDTag (the strings let go, the last first)
void __fastcall Tag_Destruct(IDTag* self, re::Edx) {
    FixedRelease(&self->interpolatorID);
    FixedRelease(&self->ctlrID);
    FixedRelease(&self->ctlrType);
    FixedRelease(&self->propertyType);
    FixedRelease(&self->avObjectName);
}

// 0x8143D0: NiControllerSequence()
NiControllerSequence* __fastcall Seq_Construct(NiControllerSequence* self, re::Edx) {
    Object_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiControllerSequence_vtable);
    self->name = sys::kNullFixedString;
    self->seqWeight = 1.0f;
    self->arraySize = 0;
    self->arrayGrowBy = 0;
    self->interpArray = nullptr;
    self->idTagArray = nullptr;
    self->textKeys = nullptr;
    self->frequency = 1.0f;
    self->cycleType = 0;
    self->beginKeyTime = 3.40282347e+38f;
    self->owner = nullptr;
    self->state = INACTIVE;
    self->endKeyTime = kInvalidFloat;
    self->lastTime = kInvalidFloat;
    self->weightedLastTime = kInvalidFloat;
    self->lastScaledTime = kInvalidFloat;
    self->callbacks.maxSize = 0;
    self->callbacks.size = 0;
    self->callbacks.esize = 0;
    self->callbacks.base = nullptr;
    self->callbacks.growBy = 1;
    self->callbacks.vtable = reinterpret_cast<void*>(CallbackArray_vtable);
    self->offset = kInvalidFloat;
    self->startTime = kInvalidFloat;
    self->partnerSequence = nullptr;
    self->endTime = kInvalidFloat;
    self->destFrame = kInvalidFloat;
    self->stringPalette = nullptr;
    self->accumRootName = sys::kNullFixedString;
    self->accumRoot = nullptr;
    self->unknown78 = 0;
    return self;
}

const RTTI* __fastcall Seq_GetRTTI(NiControllerSequence*, re::Edx) {
    return &NiControllerSequence_RTTI;
}

// 0x814480: CreateSequenceFromFile(stream, index) (static): the stream's top-level object at
// that index when it's a sequence (a NiControllerSequencePtr returned)
re::Ptr32<NiControllerSequence>* __cdecl Seq_CreateSequenceFromFile(re::Ptr32<NiControllerSequence>* result, void* stream,
                                                         uint32_t index) {
    if (index >= sys::At<uint32_t>(stream, 0x164)) {
        *result = nullptr;
        return result;
    }
    NiObject* object = sys::At<re::Ptr32<NiObject>*>(stream, 0x15C)[index];
    NiControllerSequence* sequence = nullptr;
    if (object != nullptr && IsKindOf(object, kNiControllerSequenceClassIndex)) {
        sequence = static_cast<NiControllerSequence*>(object);
    }
    *result = sequence;
    AddRefImport(sequence);
    return result;
}

// 0x8144F0: GetTimeAt(match, time): the time the next text key the match takes is reached, from
// time on (the first such key's in the next loop when there's none ahead; in a clamped or
// reversed sequence with none ahead: the begin key time's); -FLT_MAX when none matches
float __fastcall Seq_GetTimeAtMatch(NiControllerSequence* self, re::Edx, void* match, float time) {
    float keyTime = self->beginKeyTime;
    float scaled = Sequence_ComputeScaledTime(self, self->offset + time, 0);
    bool firstWrap = self->cycleType == 0;
    bool found = false;
    NiTextKeyExtraData* textKeys = self->textKeys;
    if (textKeys == nullptr) {
        return kInvalidFloat;
    }
    uint32_t count = textKeys->numKeys;
    NiTextKey* keys = textKeys->keys;
    bool ahead = false;
    for (uint32_t i = 0; i < count; i++) {
        if (!Virtual<uint8_t>(match, 2, &keys[i].text)) {
            continue;
        }
        found = true;
        float t = keys[i].time;
        if (scaled <= t) {
            keyTime = t;
            ahead = true;
            break;
        }
        if (firstWrap) {
            keyTime = t;
            firstWrap = false;
        }
    }
    if (!ahead) {
        if (!found) {
            return kInvalidFloat;
        }
        if (self->cycleType == 0) {
            keyTime = (self->endKeyTime - self->beginKeyTime) + keyTime;
        }
    }
    float result = (keyTime - scaled) / self->frequency + time;
    return result;
}

// 0x8145E0: GetKeyTimeAt(key): the first text key's time of that name (-FLT_MAX: none)
float __fastcall Seq_GetKeyTimeAt(NiControllerSequence* self, re::Edx, const re::Ptr32<const char>* key) {
    if (*key == sys::kNullFixedString) {
        return kInvalidFloat;
    }
    TextKeyMatch match;
    ConstructMatch(match, key);
    float time = Sequence_GetKeyTimeAtMatch(self, &match);
    TextKeyMatch_Destruct(&match);
    return time;
}

// 0x815C40: GetTimeAt(key, time): the time the next text key of that name is reached
float __fastcall Seq_GetTimeAt(NiControllerSequence* self, re::Edx, const re::Ptr32<const char>* key, float time) {
    TextKeyMatch match;
    ConstructMatch(match, key);
    float result = Sequence_GetTimeAtMatch(self, &match, time);
    TextKeyMatch_Destruct(&match);
    return result;
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC31_SEQ_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC31_SEQ_REVERSED("NiControllerSequence::VerifyDependencies", 0x008137D0, &ni::anim::Seq_VerifyDependencies);
NIC31_SEQ_REVERSED("NiControllerSequence::GetKeyTimeAt(match)", 0x00813850, &ni::anim::Seq_GetKeyTimeAtMatch);
NIC31_SEQ_REVERSED("NiControllerSequence::InterpArrayItem::operator=", 0x008138C0, &ni::anim::Item_Assign);
NIC31_SEQ_REVERSED("NiControllerSequence::ClearTargets", 0x00813950, &ni::anim::Seq_ClearTargets);
NIC31_SEQ_REVERSED("NiControllerSequence::AttachInterpolators", 0x008139C0, &ni::anim::Seq_AttachInterpolators);
NIC31_SEQ_REVERSED("NiControllerSequence::DetachInterpolators", 0x00813A20, &ni::anim::Seq_DetachInterpolators);
NIC31_SEQ_REVERSED("NiControllerSequence::ComputeScaledTime", 0x00813A90, &ni::anim::Seq_ComputeScaledTime);
NIC31_SEQ_REVERSED("NiControllerSequence::CanSyncTo", 0x00813BC0, &ni::anim::Seq_CanSyncTo);
NIC31_SEQ_REVERSED("NiControllerSequence::InterpArrayItem::IsEqual", 0x00813D80, &ni::anim::Item_IsEqual);
NIC31_SEQ_REVERSED("NiControllerSequence::IDTag::IsEqual", 0x00813DF0, &ni::anim::Tag_IsEqual);
NIC31_SEQ_REVERSED("NiControllerSequence::RegisterStreamables", 0x00813E30, &ni::anim::Seq_RegisterStreamables);
NIC31_SEQ_REVERSED("NiControllerSequence::LinkObject", 0x00813F20, &ni::anim::Seq_LinkObject);
NIC31_SEQ_REVERSED("NiControllerSequence::IsEqual", 0x00814050, &ni::anim::Seq_IsEqual);
NIC31_SEQ_REVERSED("NiControllerSequence::InterpArrayItem::SeqInterpArrayItem", 0x008141B0, &ni::anim::Item_Construct);
NIC31_SEQ_REVERSED("NiControllerSequence::InterpArrayItem::ClearValues", 0x008141D0, &ni::anim::Item_ClearValues);
NIC31_SEQ_REVERSED("NiControllerSequence::InterpArrayItem::VectorDeletingDestructor", 0x00814250,
                   &ni::anim::Item_VectorDeletingDestructor);
NIC31_SEQ_REVERSED("NiControllerSequence::IDTag::~IDTag", 0x008142F0, &ni::anim::Tag_Destruct);
NIC31_SEQ_REVERSED("NiControllerSequence::NiControllerSequence", 0x008143D0, &ni::anim::Seq_Construct);
NIC31_SEQ_REVERSED("NiControllerSequence::GetRTTI", 0x00814470, &ni::anim::Seq_GetRTTI);
NIC31_SEQ_REVERSED("NiControllerSequence::CreateSequenceFromFile", 0x00814480,
                   &ni::anim::Seq_CreateSequenceFromFile);
NIC31_SEQ_REVERSED("NiControllerSequence::GetTimeAt(match)", 0x008144F0, &ni::anim::Seq_GetTimeAtMatch);
NIC31_SEQ_REVERSED("NiControllerSequence::GetKeyTimeAt", 0x008145E0, &ni::anim::Seq_GetKeyTimeAt);
NIC31_SEQ_REVERSED("NiControllerSequence::GetTimeAt", 0x00815C40, &ni::anim::Seq_GetTimeAt);
