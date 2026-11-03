#pragma once

// More of Gamebryo's animation library (NiAnimation) as Bully's exe has it, laid out from the
// constructors and checked with static_asserts: the geometry morpher controller, the extra data
// controllers, NiFlipController, and the float and colour interpolators with their key data.
// Everything in ni::anim with the rest of NiAnimation (src/gamebryo/NiInterpolators.h).
//
// Virtual slots past NiInterpController's (NiInterpolators.h):
// - NiSingleInterpController: 40 InterpTargetIsCorrectType(target).
// - NiGeomMorpherController: 40 GetTargetVertices, 41 ComputeBound (the target data's bound
//   from all the morph targets).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiExtraDataTypes.h"
#include "gamebryo/NiInterpolators.h"
#include "reversed.h"

namespace ni::anim {

// ---- NiGeomMorpherController (vtable 0x9668E8, 0x44 bytes, constructed inline by CreateObject
// 0x8116F0): one float interpolator per morph target, the target geometry's vertices made as the
// weighted sum of the targets' (on the pre-display pass, or a task)

// an interpolator and its last weight (NiNew[]: the count before the first)
struct MorphInterpItem {
    re::Ptr32<NiInterpolator> interpolator;   // +0x0 (a reference)
    float weight;                   // +0x4
};
static_assert(sizeof(MorphInterpItem) == 8);

struct NiGeomMorpherController : NiTimeController {
    uint16_t morphFlags;            // +0x34 (bit 0: the normals made again)
    uint16_t pad36;
    re::Ptr32<NiMorphData> data;              // +0x38 (a reference)
    re::Ptr32<MorphInterpItem> items;         // +0x3C (one per target)
    uint8_t needPreDisplay;         // +0x40
    uint8_t ignoreBaseTarget;       // +0x41 (relative targets: the base's weight 0 and the vertices
                                    //  not cleared first)
    uint8_t alwaysUpdate;           // +0x42
    uint8_t bound;                  // +0x43 (the bound computed)
};
static_assert(offsetof(NiGeomMorpherController, data) == 0x38);
static_assert(offsetof(NiGeomMorpherController, needPreDisplay) == 0x40);
static_assert(sizeof(NiGeomMorpherController) == 0x44);

// ---- NiExtraDataController (vtable 0x9685E0, 0x40 bytes, constructor 0x828090) and its kinds:
// NiFloatExtraDataController (vtable 0x966BD0, 0x40), NiFloatsExtraDataController (vtable
// 0x966A58, 0x48), NiFloatsExtraDataPoint3Controller (vtable 0x966990, 0x48),
// NiColorExtraDataController (vtable 0x966ED0, 0x40)

struct NiExtraDataController : NiSingleInterpController {
    re::Ptr32<const char> extraDataName;    // +0x38
    re::Ptr32<NiObject> extraData;            // +0x3C (a reference: the target's extra data of that name)
};
static_assert(sizeof(NiExtraDataController) == 0x40);

// the floats (or the three of a point from there) at one index of a NiFloatsExtraData
struct NiFloatsExtraDataController : NiExtraDataController {
    int32_t floatsExtraDataIndex;   // +0x40 (-1)
    re::Ptr32<const char> ctlrID;           // +0x44 ("name[index]", made when first asked for)
};
static_assert(sizeof(NiFloatsExtraDataController) == 0x48);

// ---- NiFlipController (vtable 0x966C78, 0x54 bytes, constructor 0x813590): a texturing
// property's map flipped through a set of textures by a float's index

struct NiFlipController : NiSingleInterpController {
    NiTArray<NiObject*> textures;   // +0x38 (NiTObjectArray<NiSourceTexturePtr>, vtable 0x9893C0)
    uint32_t index;                 // +0x48 (the texture shown)
    uint32_t affectedMap;           // +0x4C (a map index; 0x400 on: a shader map's)
    re::Ptr32<char> ctlrID;                   // +0x50 (NiMalloc'd when first asked for)
};
static_assert(offsetof(NiFlipController, index) == 0x48);
static_assert(sizeof(NiFlipController) == 0x54);

// ---- key data with one channel: NiFloatData (vtable 0x965AC0), NiColorData (vtable 0x966F78)

struct NiKeyChannelData : NiObject {
    uint32_t numKeys;               // +0x8
    re::Ptr32<NiAnimationKey> keys;           // +0xC
    KeyType type;                   // +0x10
    uint8_t size;                   // +0x14
    uint8_t pad15[3];
};
static_assert(sizeof(NiKeyChannelData) == 0x18);

// ---- NiFloatInterpolator (vtable 0x966B00, 0x18 bytes), NiColorInterpolator (vtable 0x966E00,
// 0x24 bytes)

struct NiFloatInterpolator : NiKeyBasedInterpolator {
    float value;                    // +0xC (-FLT_MAX: none)
    re::Ptr32<NiKeyChannelData> data;         // +0x10 (NiFloatData, a reference)
    uint32_t lastIdx;               // +0x14
};
static_assert(sizeof(NiFloatInterpolator) == 0x18);

struct NiColorInterpolator : NiKeyBasedInterpolator {
    NiColorA value;                 // +0xC (r -FLT_MAX: none)
    re::Ptr32<NiKeyChannelData> data;         // +0x1C (NiColorData, a reference)
    uint32_t lastIdx;               // +0x20
};
static_assert(sizeof(NiColorInterpolator) == 0x24);

}   // namespace ni::anim
