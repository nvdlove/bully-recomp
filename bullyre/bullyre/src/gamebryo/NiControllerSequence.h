#pragma once

// NiControllerSequence (vtable 0x966D38, 0x7C bytes, constructor 0x8143D0) and
// NiControllerManager (vtable 0x966D88, 0x6C bytes, constructor 0x818900) as Bully's exe has
// them (the blend interpolators they drive: src/gamebryo/NiBlendInterpolator.h). A sequence holds one interpolator per animated
// controller, found again by its IDTag (object, property, controller type and ID, interpolator
// ID) when the manager's target tree is bound (StoreTargets); activating it adds each one to its
// controller's blend interpolator at a priority, its weight and ease spinner set each update.
// Everything in ni::anim (src/gamebryo/NiInterpolators.h).
//
// NiControllerSequence's virtual slots are NiObject's only.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiBlendInterpolator.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "reversed.h"

namespace ni::anim {

// ---- NiControllerSequence

// how an interpolator is found again: its object's name, property type, controller type and ID,
// interpolator ID (0x14 bytes, NiNew[]'d arrays)
struct IDTag {
    re::Ptr32<const char> avObjectName;     // +0x0
    re::Ptr32<const char> propertyType;     // +0x4
    re::Ptr32<const char> ctlrType;         // +0x8
    re::Ptr32<const char> ctlrID;           // +0xC
    re::Ptr32<const char> interpolatorID;   // +0x10
};
static_assert(sizeof(IDTag) == 0x14);

// InterpArrayItem: an interpolator, the controller it drives and that controller's blend
// interpolator (0x10 bytes, NiNew[]'d arrays; named apart from the blend interpolator's item)
struct SeqInterpArrayItem {
    re::Ptr32<NiInterpolator> interpolator;           // +0x0 (a reference)
    re::Ptr32<NiTimeController> interpCtlr;           // +0x4 (NiInterpController, a reference)
    re::Ptr32<NiBlendInterpolator> blendInterp;       // +0x8
    uint8_t blendIdx;                       // +0xC (its index there; 0xFF: none)
    uint8_t padD[3];
};
static_assert(sizeof(SeqInterpArrayItem) == 0x10);

struct NiControllerManager;

enum AnimState : uint32_t {
    INACTIVE,
    ANIMATING,
    EASINGIN,
    EASINGOUT,
    TRANSSOURCE,
    TRANSDEST,
    MORPHSOURCE,
};

struct NiControllerSequence : NiObject {
    re::Ptr32<const char> name;                     // +0x8
    uint32_t arraySize;                     // +0xC
    uint32_t arrayGrowBy;                   // +0x10
    re::Ptr32<SeqInterpArrayItem> interpArray;        // +0x14
    re::Ptr32<IDTag> idTagArray;                      // +0x18
    float seqWeight;                        // +0x1C (1)
    re::Ptr32<NiTextKeyExtraData> textKeys;           // +0x20 (a reference)
    uint32_t cycleType;                     // +0x24 (0 loop, 1 reverse, 2 clamp)
    float frequency;                        // +0x28 (1)
    float beginKeyTime;                     // +0x2C (FLT_MAX)
    float endKeyTime;                       // +0x30 (-FLT_MAX)
    float lastTime;                         // +0x34 (-FLT_MAX: not updated since activated)
    float weightedLastTime;                 // +0x38
    float lastScaledTime;                   // +0x3C
    re::Ptr32<NiControllerManager> owner;             // +0x40
    AnimState state;                        // +0x44
    NiTArray<NiRefObject*> callbacks;       // +0x48 (NiTPrimitiveArray<ActivationCallback*>, vtable
                                            //  0x961988: slot 1 ActivationChanged(sequence, state))
    float offset;                           // +0x58 (-FLT_MAX: set at the next update)
    float startTime;                        // +0x5C (of an ease or transition; -FLT_MAX: the next update)
    float endTime;                          // +0x60 (its length until the start is known)
    float destFrame;                        // +0x64 (a transition's frame to start at; -FLT_MAX: none)
    re::Ptr32<NiControllerSequence> partnerSequence;  // +0x68 (the one it's synchronised to)
    re::Ptr32<NiStringPalette> stringPalette;         // +0x6C (a reference: older files' IDTag strings)
    re::Ptr32<const char> accumRootName;            // +0x70
    re::Ptr32<NiAVObject> accumRoot;                  // +0x74
    uint32_t unknown78;                     // +0x78 (streamed from 20.3.0.8)
};
static_assert(offsetof(NiControllerSequence, textKeys) == 0x20);
static_assert(offsetof(NiControllerSequence, owner) == 0x40);
static_assert(offsetof(NiControllerSequence, callbacks) == 0x48);
static_assert(offsetof(NiControllerSequence, offset) == 0x58);
static_assert(offsetof(NiControllerSequence, accumRootName) == 0x70);
static_assert(sizeof(NiControllerSequence) == 0x7C);

// ---- NiControllerManager

// NiTStringMap<unsigned int> (vtable 0x951798 over NiTMapBase's 0x950760): a sequence's name to
// its index in the array
struct StringIndexMap {
    re::Ptr32<void> vtable;                   // +0x0
    uint32_t hashSize;              // +0x4 (37)
    re::Ptr32<re::Ptr32<void>> table;                   // +0x8 (NiMalloc'd; a node: next, key, value)
    uint32_t count;                 // +0xC
    uint8_t copy;                   // +0x10 (the keys copied: no)
    uint8_t pad11[3];
};
static_assert(sizeof(StringIndexMap) == 0x14);

struct NiControllerManager : NiTimeController {
    NiTArray<NiControllerSequence*> sequences;  // +0x34 (NiTObjectArray<NiControllerSequencePtr>,
                                                //  vtable 0x9893C0)
    StringIndexMap sequenceMap;                 // +0x44
    uint8_t cumulative;                         // +0x58 (the accumulation root's transform kept)
    uint8_t pad59[3];
    re::Ptr32<re::Ptr32<NiControllerSequence>> tempBlendSeqs;       // +0x5C (NiTObjectSet: base, allocated, count; the
                                                //  sequences blends made, references)
    uint32_t tempBlendAllocated;                // +0x60
    uint32_t tempBlendCount;                    // +0x64
    re::Ptr32<NiObject> objectPalette;                    // +0x68 (NiDefaultAVObjectPalette, a reference)
};
static_assert(offsetof(NiControllerManager, sequences) == 0x34);
static_assert(offsetof(NiControllerManager, sequenceMap) == 0x44);
static_assert(offsetof(NiControllerManager, cumulative) == 0x58);
static_assert(offsetof(NiControllerManager, objectPalette) == 0x68);
static_assert(sizeof(NiControllerManager) == 0x6C);

}   // namespace ni::anim
