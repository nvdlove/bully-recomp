#pragma once

// Gamebryo's animation library (NiAnimation) as Bully's exe has it: the interpolators, the
// interpolator-driven controllers and the key data objects they read, laid out from their
// constructors and checked with static_asserts. Everything in ni::anim with the key layouts
// (src/gamebryo/NiAnimationKey.h).
//
// Virtual slots (past NiObject's 0..17):
// - NiInterpolator: 18..23 Update(time, target, value&) for a NiQuatTransform, NiColorA,
//   NiPoint3, NiQuaternion, float, bool; 24..29 Is{Bool,Float,Quaternion,Point3,ColorA,
//   Transform}ValueSupported; 30 Collapse; 31 GetActiveTimeRange(&begin, &end);
//   32 GuaranteeTimeRange(start, end); 33 GetSequenceInterpolator(start, end);
//   34 ResolveDependencies(palette); 35 SetUpDependencies; 36 AlwaysUpdate.
// - NiKeyBasedInterpolator: 37 GetKeyChannelCount; 38 GetKeyCount(channel); 39 GetKeyType;
//   40 GetKeyContent; 41 GetKeyArray; 42 GetKeyStride; 43 GetChannelPosed.
// - NiTimeController (src/gamebryo/NiTimeController.h): 18 Start, 19 Stop, 20 Update(time),
//   21 SetTarget, 22 IsTransformController, 23 IsVertexController, 24 ComputeScaledTime,
//   25 OnPreDisplay, 26 IsStreamable, 27 TargetIsRequiredType.
// - NiInterpController: 28 GetInterpolatorCount; 29 GetInterpolatorID(index);
//   30 GetInterpolatorIndex(id); 31 GetInterpolator(index); 32 SetInterpolator(interp, index);
//   33 ResetTimeExtrema; 34 GetCtlrID; 35 CreatePoseInterpolator(index);
//   36 SynchronizePoseInterpolator(interp, index); 37 CreateBlendInterpolator(index, ...);
//   38 GuaranteeTimeRange(start, end); 39 InterpolatorIsCorrectType(interp, index).
// - NiSingleInterpController's kinds: 40 (NiFloat/NiPoint3InterpController) GetTargetValue's
//   helper, 41 GetTargetValue(value&) (the float, point, colour or bool the target has now).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiAnimationKey.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "gamebryo/NiTexturingProperty.h"
#include "gamebryo/NiTimeController.h"
#include "reversed.h"

namespace ni::anim {

// a translation, rotation and scale, each of which may be invalid (-FLT_MAX in its first
// component: INVALID_TRANSLATE / INVALID_ROTATE / INVALID_SCALE)
struct NiQuatTransform {
    NiPoint3 translate;    // +0x0
    NiQuaternion rotate;   // +0xC (w, x, y, z)
    float scale;           // +0x1C
};
static_assert(sizeof(NiQuatTransform) == 0x20);

// the invalid values (statics the initialisers fill)
GAME_VAR(NiPoint3, kInvalidTranslate, 0x00BC8C3C);
GAME_VAR(NiQuaternion, kInvalidRotate, 0x020E56E8);
GAME_VAR(NiQuaternion, kInvalidQuaternion, 0x020E572C);   // NiQuaternionInterpolator's
constexpr float kInvalidFloat = -3.40282347e+38f;          // -FLT_MAX

// ---- interpolators

// NiInterpolator (vtable 0x968250, constructor 0x825DA0)
struct NiInterpolator : NiObject {
    float lastTime;   // +0x8 (the last Update's time, -FLT_MAX at first)
};
static_assert(sizeof(NiInterpolator) == 0xC);

// NiKeyBasedInterpolator (vtable 0x968138, constructor 0x8253B0)
struct NiKeyBasedInterpolator : NiInterpolator {};
static_assert(sizeof(NiKeyBasedInterpolator) == 0xC);

struct NiTransformData;
struct NiRotData;
struct NiPosData;

// NiTransformInterpolator (vtable 0x965C00): channels 0 position, 1 rotation, 2 scale
struct NiTransformInterpolator : NiKeyBasedInterpolator {
    NiQuatTransform value;     // +0xC (the last result, or the posed value)
    re::Ptr32<NiTransformData> data;     // +0x2C (a reference)
    uint32_t lastTransIdx;     // +0x30
    uint32_t lastRotIdx;       // +0x34
    uint32_t lastScaleIdx;     // +0x38
};
static_assert(offsetof(NiTransformInterpolator, data) == 0x2C);
static_assert(sizeof(NiTransformInterpolator) == 0x3C);

// NiQuaternionInterpolator (vtable 0x965FB0): one rotation channel
struct NiQuaternionInterpolator : NiKeyBasedInterpolator {
    NiQuaternion value;   // +0xC
    re::Ptr32<NiRotData> data;      // +0x1C (a reference)
    uint32_t lastIdx;     // +0x20
};
static_assert(sizeof(NiQuaternionInterpolator) == 0x24);

// NiPoint3Interpolator (vtable 0x966088): one point channel
struct NiPoint3Interpolator : NiKeyBasedInterpolator {
    NiPoint3 value;    // +0xC
    re::Ptr32<NiPosData> data;   // +0x18 (a reference)
    uint32_t lastIdx;  // +0x1C
};
static_assert(sizeof(NiPoint3Interpolator) == 0x20);

// what NiPathInterpolator and the older NiPathController share, at +0xC in the one and +0x34 in
// the other: a position path (NiPosData) followed at a percentage over time (NiFloatData)
struct NiPathBlock {
    uint16_t flags;            // +0x0 (1 the path length to compute, 2 open curve, 4 allow flip
                               //  (the curve's own frame), 8 bank, 0x10 constant velocity, 0x20
                               //  follow (a rotation made), 0x40 flip; 3 by default)
    uint16_t pad2;
    uint32_t lastPathIdx;      // +0x4
    uint32_t lastPctIdx;       // +0x8
    re::Ptr32<NiPosData> pathData;       // +0xC (a reference)
    re::Ptr32<NiObject> pctData;         // +0x10 (NiFloatData, a reference)
    re::Ptr32<float> partialLengths;     // +0x14 (NiMalloc'd, one per path key: the length up to it)
    float totalLength;         // +0x18 (-1: not computed)
    float maxBankAngle;        // +0x1C
    float smoothing;           // +0x20
    int16_t followAxis;        // +0x24 (0 x, 1 y, 2 z)
    uint16_t pad26;
    float maxCurvature;        // +0x28
    int32_t bankDir;           // +0x2C (1 or -1)
};
static_assert(offsetof(NiPathBlock, pathData) == 0xC);
static_assert(offsetof(NiPathBlock, followAxis) == 0x24);
static_assert(sizeof(NiPathBlock) == 0x30);

// NiPathInterpolator (vtable 0x966160): channels 0 the path's positions, 1 the percentages
struct NiPathInterpolator : NiKeyBasedInterpolator {
    NiPathBlock path;          // +0xC
    NiQuaternion rotateValue;  // +0x3C
    NiPoint3 positionValue;    // +0x4C
    float lastUpdateTime;      // +0x58 (Update writes it; its cache compares lastTime, +0x8)
};
static_assert(offsetof(NiPathInterpolator, rotateValue) == 0x3C);
static_assert(sizeof(NiPathInterpolator) == 0x5C);

// NiPathController (vtable 0x966218): the same, driving its target's transform directly
struct NiPathController : NiTimeController {
    NiPathBlock path;          // +0x34
};
static_assert(sizeof(NiPathController) == 0x64);

// NiLookAtInterpolator
struct NiLookAtInterpolator : NiInterpolator {
    uint16_t flags;               // +0xC (bit 0 flip, bits 1..2 the axis: 0 x, 1 y, 2 z)
    uint16_t padE;
    re::Ptr32<NiAVObject> lookAt;           // +0x10 (not a reference; found again by name)
    re::Ptr32<const char> lookAtName;     // +0x14
    NiQuatTransform value;        // +0x18
    re::Ptr32<NiInterpolator> interps[3];   // +0x38 (references: the translation (a point), the roll and the
                                  //  scale (floats))
};
static_assert(offsetof(NiLookAtInterpolator, value) == 0x18);
static_assert(sizeof(NiLookAtInterpolator) == 0x44);

// NiLookAtController
struct NiLookAtController : NiTimeController {
    uint16_t lookFlags;           // +0x34 (as the interpolator's)
    uint16_t pad36;
    re::Ptr32<NiAVObject> lookAt;           // +0x38 (not a reference)
};
static_assert(sizeof(NiLookAtController) == 0x3C);

// ---- key data (NiObjects holding one key array each, or three for NiTransformData)

// a key array with its kind (count, keys, type, key size)
struct KeyChannel {
    uint32_t numKeys;         // +0x0
    re::Ptr32<NiAnimationKey> keys;     // +0x4
    KeyType type;             // +0x8
    uint8_t size;             // +0xC
    uint8_t padD[3];
};
static_assert(sizeof(KeyChannel) == 0x10);

// NiRotData (vtable 0x965E80), NiPosData (vtable 0x965B08): one channel
struct NiRotData : NiObject {
    KeyChannel rot;   // +0x8
};
static_assert(sizeof(NiRotData) == 0x18);

struct NiPosData : NiObject {
    KeyChannel pos;   // +0x8
};
static_assert(sizeof(NiPosData) == 0x18);

// NiTransformData (vtable 0x966718): rotation, position and scale channels
struct NiTransformData : NiObject {
    KeyChannel rot;     // +0x8
    KeyChannel pos;     // +0x18
    KeyChannel scale;   // +0x28 (float keys)
};
static_assert(offsetof(NiTransformData, pos) == 0x18);
static_assert(offsetof(NiTransformData, scale) == 0x28);
static_assert(sizeof(NiTransformData) == 0x38);

// ---- controllers

// NiInterpController (vtable 0x967FA8): the time controller's layout; NiSingleInterpController
// (vtable 0x968070) adds its interpolator
struct NiSingleInterpController : NiTimeController {
    re::Ptr32<NiInterpolator> interpolator;   // +0x34 (a reference)
};
static_assert(sizeof(NiSingleInterpController) == 0x38);

// NiFloatController (vtable 0x965EC8; NiRollController's base): an old-style controller with
// its own keys
struct NiFloatController : NiTimeController {
    uint32_t lastIdx;   // +0x34
    re::Ptr32<NiObject> data;     // +0x38 (NiFloatData, a reference)
};
static_assert(sizeof(NiFloatController) == 0x3C);

// ---- the other classes of these files

// NiTextKey: a time and its text
struct NiTextKey {
    float time;          // +0x0
    re::Ptr32<const char> text;  // +0x4
};
static_assert(sizeof(NiTextKey) == 8);

// NiTextKeyExtraData: NiExtraData (its name at +0x8) and the keys
struct NiTextKeyExtraData : NiObject {
    re::Ptr32<const char> name;   // +0x8 (NiExtraData's)
    uint32_t numKeys;     // +0xC
    re::Ptr32<NiTextKey> keys;      // +0x10 (NiNew[]: the count at keys[-1])
};
static_assert(sizeof(NiTextKeyExtraData) == 0x14);

// NiStringPalette
struct NiStringPalette : NiObject {
    re::Ptr32<char> buffer;          // +0x8 (NiMalloc'd)
    uint32_t allocated;    // +0xC
    uint32_t end;          // +0x10 (the bytes used)
};
static_assert(sizeof(NiStringPalette) == 0x14);

// a morph target
struct MorphTarget {
    re::Ptr32<NiPoint3> verts;              // +0x0 (NiAllocArray'd: numVertsPerTarget)
    re::Ptr32<const char> name;           // +0x4
    float legacyWeight;           // +0x8
    re::Ptr32<NiInterpolator> interpolator; // +0xC (a reference: a NiFloatInterpolator)
};
static_assert(sizeof(MorphTarget) == 0x10);

struct NiMorphData : NiObject {
    uint32_t numTargets;          // +0x8
    uint32_t numVertsPerTarget;   // +0xC
    re::Ptr32<MorphTarget> targets;         // +0x10 (NiNew[]: the count at targets[-1])
    uint8_t relativeTargets;      // +0x14
    uint8_t pad15[3];
};
static_assert(sizeof(NiMorphData) == 0x18);

// NiTextureTransformController: which map of the target NiTexturingProperty, which member of
// its transform
struct NiTextureTransformController : NiSingleInterpController {
    uint32_t unknown38;   // +0x38 (0; copied and compared)
    re::Ptr32<NiTexturingMap> map;  // +0x3C (found from the target when needed)
    uint8_t shaderMap;    // +0x40 (the index is into the shader maps)
    uint8_t pad41[3];
    uint32_t mapIndex;    // +0x44
    uint32_t member;      // +0x48 (0 TT_TRANSLATE_U, 1 _V, 2 TT_ROTATE, 3 TT_SCALE_U, 4 _V)
    re::Ptr32<char> ctlrID;         // +0x4C (NiMalloc'd when first asked for)
};
static_assert(offsetof(NiTextureTransformController, member) == 0x48);
static_assert(sizeof(NiTextureTransformController) == 0x50);

// NiMaterialColorController / NiLightColorController: which colour
struct NiColorInterpController : NiSingleInterpController {
    uint16_t field;   // +0x38 (material: 0 ambient, 1 diffuse, 2 specular, 3 self-illumination
                      //  in bits 0..2; light: bit 0 ambient, else diffuse and specular)
    uint16_t pad3A;
};
static_assert(sizeof(NiColorInterpController) == 0x3C);

// NiMultiTargetTransformController
struct NiMultiTargetTransformController : NiTimeController {
    re::Ptr32<uint8_t> blendInterps;     // +0x34 (NiBlendTransformInterpolator[numInterps], 0x30 each; each
                               //  held once more so nothing deletes them singly)
    re::Ptr32<re::Ptr32<NiAVObject>> targets;      // +0x38 (NiMalloc'd, not references)
    uint16_t numInterps;       // +0x3C
    uint16_t pad3E;
};
static_assert(sizeof(NiMultiTargetTransformController) == 0x40);

// NiBlendAccumTransformInterpolator's accumulation item (one per blend array slot)
struct AccumArrayItem {
    float lastTime;            // +0x0
    NiQuatTransform delta;     // +0x4
    NiQuatTransform start;     // +0x24
    NiMatrix3 rotation;        // +0x44
};
static_assert(sizeof(AccumArrayItem) == 0x68);

// ---- what the files share

// a reference let go as the library's code does it here (the import's decrement; DeleteThis
// when it was the last)
inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && sys::Decrement(&object->refCount) == 0) {
        re::PtrCast<void(__thiscall*)(NiRefObject*)>(VtableOf(object)[1])(object);
    }
}

inline void AddRefImport(NiRefObject* object) {
    if (object != nullptr) {
        sys::Increment(&object->refCount);
    }
}

// a reference slot assigned as most of these functions have it: the old one let go first,
// then the new one held
template <typename T, typename U>
inline void AssignReleasingFirst(re::Ptr32<T>& slot, U* value) {
    T* v = static_cast<T*>(value);
    if (slot != v) {
        ReleaseRef(slot);
        slot = v;
        AddRefImport(v);
    }
}
#if defined(_WIN64)
template <typename T, typename U>
inline void AssignReleasingFirst(re::Ptr32<T>& slot, re::Ptr32<U> value) {
    AssignReleasingFirst(slot, value.get());
}
#endif

// NiObject::IsKindOf through the class bit set of the RTTI (index at the RTTI's +8)
inline uint8_t IsKindOf(const void* object, uint32_t classIndex) {
    const uint32_t* rtti = re::PtrCast<const uint32_t*(__thiscall*)(const void*)>(VtableOf(object)[2])(object);
    return (rtti[3 + (classIndex >> 5)] & (1u << (classIndex & 31))) != 0 ? 1 : 0;
}

}   // namespace ni::anim
