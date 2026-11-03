// NiPathInterpolator (vtable 0x966160, 0x5C bytes) and the older NiPathController (vtable
// 0x966218, 0x64): a position along a NiPosData path at a percentage (NiFloatData keys) over
// time, optionally at constant velocity (the path's arc length: 5-point Gauss-Legendre
// quadrature per segment, then Newton's method on the parameter), optionally with a rotation
// following the curve (its frame, smoothed across key boundaries, flipped, re-axed and banked by
// curvature). The two classes share the code (src/gamebryo/NiInterpolators.h, NiPathBlock: at
// +0xC in the one, +0x34 in the other); the game has each function twice, so do we, through
// templates over the block's offset and the copies' addresses.
//
// Main-thread float maths at the game's 24-bit x87 precision: float operations in the game's
// order (each product and sum rounded as its code stores or keeps them).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::At;
using scene::GetViewerString;
using scene::LoadBytes;
using scene::SaveBytes;
using scene::Stream_ReadLinkID;
using scene::Stream_ResolveLinkID;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiPathInterpolator_RTTI, 0x020E4CE8);
GAME_VAR(RTTI, NiPathController_RTTI, 0x020E4D24);

constexpr uintptr_t NiPathInterpolator_vtable = 0x00966160;
constexpr uintptr_t NiPathController_vtable = 0x00966218;

constexpr uint16_t kLengthDirty = 0x1;
constexpr uint16_t kOpenCurve = 0x2;
constexpr uint16_t kAllowFlip = 0x4;
constexpr uint16_t kBank = 0x8;
constexpr uint16_t kConstVelocity = 0x10;
constexpr uint16_t kFollow = 0x20;
constexpr uint16_t kFlip = 0x40;

GAME_VAR(NiMatrix3, kMatrixIdentity, 0x00BC75FC);   // NiMatrix3::IDENTITY
GAME_VAR(NiPoint3, kUnitZ, 0x00BC8AA8);             // NiPoint3::UNIT_Z
GAME_VAR(NiPoint3, kInvalidPoint, 0x00BC8C48);
GAME_VAR(float, kFourOverPi, 0x00966210);           // (the static below starts from it)
GAME_VAR(float, kMaxFloat, 0x00966158);             // FLT_MAX

GAME_FN(void, __cdecl, PosKey_CoordinateFrame, 0x007F8EA0, float t, const NiAnimationKey* key0,
        const NiAnimationKey* key1, KeyType type, NiPoint3* tangent, NiPoint3* normal, NiPoint3* binormal,
        float* curvature);
GAME_FN(float, __cdecl, FloatKey_GenInterp, 0x007F8300, float time, NiAnimationKey* keys, KeyType type,
        uint32_t numKeys, uint32_t* lastIdx, uint8_t size);
GAME_FN(NiPoint3*, __cdecl, Point3_Scale, 0x00412320, NiPoint3* out, float s, const NiPoint3* v);
GAME_FN(NiPoint3*, __thiscall, Point3_Add, 0x004065A0, const NiPoint3* self, NiPoint3* out, const NiPoint3* other);
GAME_FN(float, __thiscall, Point3_Unitize, 0x00415130, NiPoint3* self);
GAME_FN(NiPoint3*, __thiscall, Point3_Cross, 0x007D5660, const NiPoint3* self, NiPoint3* out, const NiPoint3* other);
GAME_FN(NiPoint3*, __thiscall, Point3_UnitCross, 0x00762F30, const NiPoint3* self, NiPoint3* out,
        const NiPoint3* other);
GAME_FN(void, __thiscall, Matrix3_MakeRotation, 0x00768820, NiMatrix3* self, float angle, float x, float y, float z);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Multiply, 0x00768920, const NiMatrix3* self, NiMatrix3* out,
        const NiMatrix3* other);
GAME_FN(void, __thiscall, Quaternion_FromRotation, 0x00776040, NiQuaternion* self, const NiMatrix3* m);
// the bases' (NiKeyBasedInterpolator / NiInterpolator, NiTimeController, NiObject)
GAME_FN(void, __thiscall, KeyBasedInterpolator_Construct, 0x008253B0, void* self);
GAME_FN(void, __thiscall, KeyBasedInterpolator_Destruct, 0x008253D0, void* self);
GAME_FN(void, __thiscall, KeyBasedInterpolator_CopyMembers, 0x008253E0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Interpolator_LoadBinary, 0x007F7AB0, void* self, void* stream);
GAME_FN(void, __thiscall, Interpolator_SaveBinary, 0x008253F0, void* self, void* stream);   // (nothing)
GAME_FN(void, __thiscall, Interpolator_LinkObject, 0x008253F0, void* self, void* stream);   // (the same)
GAME_FN(uint8_t, __thiscall, Interpolator_RegisterStreamables, 0x007DC970, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Interpolator_IsEqual, 0x00825400, void* self, void* other);
GAME_FN(void, __thiscall, KeyBasedInterpolator_GetViewerStrings, 0x008255C0, void* self, NiTArray<char*>* strings);
GAME_FN(NiInterpolator*, __thiscall, Interpolator_GetSequenceInterpolator, 0x00825DF0, void* self, float start,
        float end);
GAME_FN(NiAnimationKey*, __thiscall, KeyBasedInterpolator_GetKeyAt, 0x00825450, void* self, uint32_t index,
        uint16_t channel);
GAME_FN(void, __thiscall, Object_ProcessClone, 0x007553F0, void* self, void* cloning);
GAME_FN(NiTimeController*, __thiscall, TimeController_Construct, 0x0077DFA0, void* self);
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
GAME_FN(void, __thiscall, TimeController_ProcessClone, 0x0077DEE0, void* self, void* cloning);
GAME_FN(uint8_t, __thiscall, TimeController_RegisterStreamables, 0x0077DF70, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_LinkObject, 0x0077DF10, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_LoadBinary, 0x0077E1F0, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_SaveBinary, 0x0077E2F0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TimeController_IsEqual, 0x0077DB90, void* self, void* other);
GAME_FN(void, __thiscall, TimeController_CopyMembers, 0x0077E100, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, TimeController_GetViewerStrings, 0x0077E550, void* self, NiTArray<char*>* strings);
GAME_FN(uint8_t, __thiscall, TimeController_DontDoUpdate, 0x0077D890, void* self, float time);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, PathInterpolator_Destruct, 0x00808DF0, void* self);
GAME_FN(void, __thiscall, PathInterpolator_CopyMembers, 0x0080A680, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, PathController_Destruct, 0x0080AA00, void* self);
GAME_FN(void, __thiscall, PathController_CopyMembers, 0x0080BE80, void* self, void* dest, void* cloning);
// NiFloatData's (the percentages; the next chunk's)
GAME_FN(void, __thiscall, FloatData_GuaranteeKeysAtStartAndEnd, 0x00812BB0, NiObject* self, float start, float end);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, FloatData_GetSequenceData, 0x00812D90, NiObject* self, re::Ptr32<NiObject>* result, float start,
        float end);

// NiFloatData's keys (the NiObject's +8: count, keys, type, key size)
inline KeyChannel& FloatChannel(NiObject* data) {
    return At<KeyChannel>(data, 8);
}

// ---- the shared code, over the block's offset and each copy's addresses

struct PathAddrs {
    uint32_t block;                // the NiPathBlock's offset in the object
    uintptr_t getSpeed;            // (the copies of this file's functions)
    uintptr_t getLength;
    uintptr_t getTotalLength;
    uintptr_t getPercentLoc;
    uintptr_t getLoc;
    uintptr_t calcRotMatrix;
    uintptr_t getPosition;
    uintptr_t getPathKeys;
    uintptr_t weights;             // the Gauss-Legendre weights and abscissas (5 each, a copy per class)
    uintptr_t abscissas;
    uintptr_t bankScale;           // CalcRotMatrix's function-local static (4/pi) and its guard
    uintptr_t bankScaleGuard;
};
constexpr PathAddrs kInterp{0xC,      0x00809940, 0x008099D0, 0x00809F00, 0x00809FC0, 0x0080A1A0, 0x00809190,
                            0x00809130, 0x008090D0, 0x00BC8BE0, 0x00BC8BF4, 0x020E4CE0, 0x020E4CE4};
constexpr PathAddrs kCtlr{0x34,       0x0080B3C0, 0x0080B450, 0x0080B8A0, 0x0080B940, 0x0080BB20, 0x0080AC10,
                          0x0080ABB0, 0x0080AB30, 0x00BC8C08, 0x00BC8C1C, 0x020E4D1C, 0x020E4D20};

template <PathAddrs A>
inline NiPathBlock& Block(void* self) {
    return *reinterpret_cast<NiPathBlock*>(reinterpret_cast<uintptr_t>(self) + A.block);
}

// the copies called through their addresses (so modes apply)
template <PathAddrs A>
inline float CallGetSpeed(void* self, uint32_t i0, uint32_t i1, float t) {
    return reinterpret_cast<float(__thiscall*)(void*, uint32_t, uint32_t, float)>(A.getSpeed)(self, i0, i1, t);
}
template <PathAddrs A>
inline float CallGetLength(void* self, uint32_t i0, uint32_t i1, float t) {
    return reinterpret_cast<float(__thiscall*)(void*, uint32_t, uint32_t, float)>(A.getLength)(self, i0, i1, t);
}
template <PathAddrs A>
inline float CallGetTotalLength(void* self) {
    return reinterpret_cast<float(__thiscall*)(void*)>(A.getTotalLength)(self);
}
template <PathAddrs A>
inline void CallGetPercentLoc(void* self, float pct, uint32_t* i0, uint32_t* i1, float* t) {
    reinterpret_cast<void(__thiscall*)(void*, float, uint32_t*, uint32_t*, float*)>(A.getPercentLoc)(self, pct, i0, i1,
                                                                                                    t);
}
template <PathAddrs A>
inline void CallGetLoc(void* self, float t, uint32_t* i0, uint32_t* i1, float* u) {
    reinterpret_cast<void(__thiscall*)(void*, float, uint32_t*, uint32_t*, float*)>(A.getLoc)(self, t, i0, i1, u);
}
template <PathAddrs A>
inline void CallCalcRotMatrix(void* self, uint32_t i0, uint32_t i1, float u, NiMatrix3* m) {
    reinterpret_cast<void(__thiscall*)(void*, uint32_t, uint32_t, float, NiMatrix3*)>(A.calcRotMatrix)(self, i0, i1,
                                                                                                      u, m);
}
template <PathAddrs A>
inline NiPoint3* CallGetPosition(void* self, NiPoint3* result, uint32_t i0, uint32_t i1, float u) {
    return reinterpret_cast<NiPoint3*(__thiscall*)(void*, NiPoint3*, uint32_t, uint32_t, float)>(A.getPosition)(
        self, result, i0, i1, u);
}

// the path's keys (none without data)
struct PathKeys {
    NiAnimationKey* keys = nullptr;
    KeyType type = NOINTERP;
    uint8_t size = 0;
};
inline PathKeys Keys(const NiPathBlock& b) {
    PathKeys k;
    if (b.pathData != nullptr) {
        k.type = b.pathData->pos.type;
        k.size = b.pathData->pos.size;
        k.keys = b.pathData->pos.keys;
    }
    return k;
}

// GetPosition(result, i0, i1, t): the path between two keys
template <PathAddrs A>
NiPoint3* PathGetPosition(void* self, NiPoint3* result, uint32_t i0, uint32_t i1, float t) {
    PathKeys k = Keys(Block<A>(self));
    ms_interp[KeyIndex(POSKEY, k.type)](t, KeyAt(k.keys, i0, k.size), KeyAt(k.keys, i1, k.size), result);
    return result;
}

// GetSpeed(i0, i1, t): the length of the path's derivative there
template <PathAddrs A>
float PathGetSpeed(void* self, uint32_t i0, uint32_t i1, float t) {
    PathKeys k = Keys(Block<A>(self));
    NiPoint3 d;
    ms_interpD1[KeyIndex(POSKEY, k.type)](t, KeyAt(k.keys, i0, k.size), KeyAt(k.keys, i1, k.size), &d);
    float squared = (d.x * d.x + d.y * d.y) + d.z * d.z;
    return crt::Sqrtf(squared);
}

// GetLength(i0, i1, t): the arc length from key i0 to the parameter t (5-point Gauss-Legendre)
template <PathAddrs A>
float PathGetLength(void* self, uint32_t i0, uint32_t i1, float t) {
    const float* weights = reinterpret_cast<const float*>(A.weights);
    const float* abscissas = reinterpret_cast<const float*>(A.abscissas);
    float sum = 0.0f;
    for (int k = 0; k < 5; k++) {
        float at = abscissas[k] * t;
        float speed = CallGetSpeed<A>(self, i0, i1, at);
        sum = speed * weights[k] + sum;
    }
    return sum * t;
}

// GetTotalLength: computed when negative (the partial lengths remade: one per key, the first 0;
// no keys writes past the empty block, as the game has it)
template <PathAddrs A>
float PathGetTotalLength(void* self) {
    NiPathBlock& b = Block<A>(self);
    if (b.totalLength < 0.0f) {
        uint32_t numKeys = b.pathData != nullptr ? b.pathData->pos.numKeys : 0;
        NiFree(b.partialLengths);
        float* lengths = static_cast<float*>(NiMalloc(numKeys * 4));
        b.partialLengths = lengths;
        lengths[0] = 0.0f;
        uint32_t last = numKeys - 1;
        if (last != 0) {
            uint32_t i = 0;
            do {
                float length = CallGetLength<A>(self, i, i + 1, 1.0f);
                b.partialLengths[i + 1] = b.partialLengths[i] + length;
                i++;
            } while (i < last);
        }
        b.totalLength = b.partialLengths[numKeys - 1];
    }
    return b.totalLength;
}

// GetPercentLoc(pct, &i0, &i1, &t): the segment and parameter at a fraction of the arc length
// (wrapped into 0..1); the parameter refined by Newton's method (32 steps at most)
template <PathAddrs A>
void PathGetPercentLoc(void* self, float pct, uint32_t* i0Out, uint32_t* i1Out, float* tOut) {
    NiPathBlock& b = Block<A>(self);
    uint32_t numKeys = b.pathData != nullptr ? b.pathData->pos.numKeys : 0;
    if (pct == 0.0f) {
        *i0Out = 0;
        *i1Out = 1;
        *tOut = 0.0f;
        return;
    }
    if (pct < 0.0f) {
        float r = float(std::fmod(double(pct), 1.0));
        pct = r + 1.0f;
    }
    if (pct == 1.0f) {
        *i0Out = numKeys - 2;
        *i1Out = numKeys - 1;
        *tOut = 1.0f;
        return;
    }
    if (1.0f < pct) {
        pct = float(std::fmod(double(pct), 1.0));
    }
    float target = pct * b.totalLength;
    uint32_t i0 = 0;
    uint32_t i1 = 1;
    float offset = 0.0f;
    float t = 0.0f;
    uint32_t last = numKeys - 1;
    if (last != 0) {
        const float* lengths = b.partialLengths;
        for (;;) {
            if (lengths[i0 + 1] >= target) {
                const float* l = b.partialLengths;
                offset = target - l[i0];
                t = offset / (l[i1] - l[i0]);
                break;
            }
            i0++;
            i1++;
            if (i0 >= last) {
                break;
            }
        }
    }
    for (uint32_t iteration = 0;;) {
        float diff = CallGetLength<A>(self, i0, i1, t) - offset;
        float absDiff = std::fabs(diff);
        if (absDiff <= 1e-4f) {
            break;
        }
        float speed = CallGetSpeed<A>(self, i0, i1, t);
        t = t - diff / speed;
        iteration++;
        if (iteration >= 0x20) {
            break;
        }
    }
    *i0Out = i0;
    *i1Out = i1;
    *tOut = t;
}

// GetLoc(t, &i0, &i1, &u): the segment and parameter at a key time (constant velocity: at the
// percentage instead); the search starts from the last segment found, wrapping once
template <PathAddrs A>
void PathGetLoc(void* self, float t, uint32_t* i0Out, uint32_t* i1Out, float* uOut) {
    NiPathBlock& b = Block<A>(self);
    if ((b.flags & kConstVelocity) != 0) {
        CallGetPercentLoc<A>(self, t, i0Out, i1Out, uOut);
        b.lastPathIdx = *i0Out;
        return;
    }
    uint32_t numKeys = 0;
    uint8_t size = 0;
    const NiAnimationKey* keys = nullptr;
    if (b.pathData != nullptr) {
        numKeys = b.pathData->pos.numKeys;
        size = b.pathData->pos.size;
        keys = b.pathData->pos.keys;
    }
    if (keys->time >= t) {
        *i0Out = 0;
        *i1Out = 1;
        *uOut = 0.0f;
        return;
    }
    if (KeyAt(keys, numKeys - 1, size)->time <= t) {
        *i0Out = numKeys - 2;
        *i1Out = numKeys - 1;
        *uOut = 1.0f;
        return;
    }
    uint32_t i = b.lastPathIdx;
    uint32_t j = 0;
    uint8_t first = 1;
    if (i < numKeys - 1) {
        do {
            float ti = KeyAt(keys, i, size)->time;
            if (ti == t || (ti < t && KeyAt(keys, i + 1, size)->time > t)) {
                j = i + 1;
                b.lastPathIdx = i;
                break;
            }
            if (i == numKeys - 2 && first) {
                i = 0;
                first = 0;
            } else {
                i++;
            }
        } while (i < numKeys - 1);
    }
    float ti = KeyAt(keys, i, size)->time;
    *uOut = (t - ti) / (KeyAt(keys, j, size)->time - ti);
    *i0Out = i;
    *i1Out = j;
}

// the smoothing at a key boundary: the frame's normal and curvature blended with the
// neighbouring segment's (weight `own` on this segment's at `ownAt`, `other` on the neighbour's
// at `otherAt`), the binormal remade
inline void BlendFrame(const NiAnimationKey* key0, const NiAnimationKey* key1, const NiAnimationKey* nkey0,
                       const NiAnimationKey* nkey1, KeyType type, float ownAt, float otherAt, float own, float other,
                       const NiPoint3& tangent, NiPoint3& normal, NiPoint3& binormal, float& curvature) {
    NiPoint3 tangentUnused;
    NiPoint3 normal2;
    float curvature2;
    PosKey_CoordinateFrame(ownAt, key0, key1, type, &tangentUnused, &normal, &binormal, &curvature);
    PosKey_CoordinateFrame(otherAt, nkey0, nkey1, type, &tangentUnused, &normal2, &binormal, &curvature2);
    NiPoint3 scaledOther;
    NiPoint3 scaledOwn;
    NiPoint3 sum;
    Point3_Scale(&scaledOther, other, &normal2);
    Point3_Scale(&scaledOwn, own, &normal);
    normal = *Point3_Add(&scaledOwn, &sum, &scaledOther);
    Point3_Unitize(&normal);
    curvature = curvature2 * other + own * curvature;
    NiPoint3 cross;
    binormal = *Point3_Cross(&tangent, &cross, &normal);
}

// CalcRotMatrix(i0, i1, t, &m): the rotation following the path (identity under two keys).
// Without "allow flip": x the tangent, y UNIT_Z x tangent, z tangent x y. With it: the curve's
// own frame (tangent, -normal, -binormal), its normal and curvature smoothed over `smoothing`
// at both ends of the segment. Then flip, the follow axis, and the bank about the tangent
// (bankDir * maxBankAngle, scaled by 4/pi atan(curvature / maxCurvature) below the maximum)
template <PathAddrs A>
void PathCalcRotMatrix(void* self, uint32_t i0, uint32_t i1, float t, NiMatrix3* result) {
    NiPathBlock& b = Block<A>(self);
    NiMatrix3 m = kMatrixIdentity;
    NiPosData* data = b.pathData;
    if (data != nullptr && data->pos.numKeys >= 2) {
        uint32_t numKeys = data->pos.numKeys;
        KeyType type = data->pos.type;
        uint8_t size = data->pos.size;
        const NiAnimationKey* keys = data->pos.keys;
        const NiAnimationKey* key0 = KeyAt(keys, i0, size);
        const NiAnimationKey* key1 = KeyAt(keys, i1, size);
        NiPoint3 tangent;
        NiPoint3 normal;
        NiPoint3 binormal;
        float curvature;
        if ((b.flags & kAllowFlip) != 0) {
            PosKey_CoordinateFrame(t, key0, key1, type, &tangent, &normal, &binormal, &curvature);
            float s = b.smoothing;
            if (s != 0.0f) {
                if (!(1.0f - s <= t)) {
                    // the start of the segment: blended with the one before (wrapping on a
                    // closed curve)
                    if (s >= t) {
                        uint32_t prev = ((b.flags & kOpenCurve) == 0 && i0 == 0) ? numKeys - 1 : i0;
                        if (prev >= 1) {
                            float other = (s - t) / (s + s);
                            float own = 1.0f - other;
                            BlendFrame(key0, key1, KeyAt(keys, prev - 1, size), KeyAt(keys, prev, size), type, s,
                                       1.0f - s, own, other, tangent, normal, binormal, curvature);
                        }
                    }
                } else {
                    // the end of the segment: blended with the one after
                    uint32_t next = ((b.flags & kOpenCurve) == 0 && i1 == numKeys - 1) ? 0 : i1;
                    if (next < numKeys - 1) {
                        float own = ((1.0f - t) + s) / (s + s);
                        float other = 1.0f - own;
                        BlendFrame(key0, key1, KeyAt(keys, next, size), KeyAt(keys, next + 1, size), type, 1.0f - s,
                                   s, own, other, tangent, normal, binormal, curvature);
                    }
                }
            }
            m.m[0][0] = tangent.x;
            m.m[1][0] = tangent.y;
            m.m[2][0] = tangent.z;
            m.m[0][1] = -normal.x;
            m.m[1][1] = -normal.y;
            m.m[2][1] = -normal.z;
            m.m[0][2] = -binormal.x;
            m.m[1][2] = -binormal.y;
            m.m[2][2] = -binormal.z;
        } else {
            PosKey_CoordinateFrame(t, key0, key1, type, &tangent, &normal, &binormal, &curvature);
            m.m[0][0] = tangent.x;
            m.m[1][0] = tangent.y;
            m.m[2][0] = tangent.z;
            NiPoint3 c;
            Point3_UnitCross(&kUnitZ, &c, &tangent);
            m.m[0][1] = c.x;
            m.m[1][1] = c.y;
            m.m[2][1] = c.z;
            m.m[0][2] = c.z * tangent.y - c.y * tangent.z;
            m.m[1][2] = tangent.z * c.x - c.z * tangent.x;
            m.m[2][2] = c.y * tangent.x - c.x * tangent.y;
        }
        if ((b.flags & kFlip) != 0) {
            m.m[0][0] = -tangent.x;
            m.m[1][0] = -tangent.y;
            m.m[2][0] = -tangent.z;
            m.m[0][1] = -m.m[0][1];
            m.m[1][1] = -m.m[1][1];
            m.m[2][1] = -m.m[2][1];
        }
        if (b.followAxis == 1) {
            NiPoint3 x{m.m[0][0], m.m[1][0], m.m[2][0]};
            m.m[0][0] = -m.m[0][1];
            m.m[1][0] = -m.m[1][1];
            m.m[2][0] = -m.m[2][1];
            m.m[0][1] = x.x;
            m.m[1][1] = x.y;
            m.m[2][1] = x.z;
        } else if (b.followAxis == 2) {
            NiPoint3 x{m.m[0][0], m.m[1][0], m.m[2][0]};
            m.m[0][0] = -m.m[0][2];
            m.m[1][0] = -m.m[1][2];
            m.m[2][0] = -m.m[2][2];
            m.m[0][2] = x.x;
            m.m[1][2] = x.y;
            m.m[2][2] = x.z;
        }
        if ((b.flags & kBank) != 0) {
            uint32_t& guard = *reinterpret_cast<uint32_t*>(A.bankScaleGuard);
            float& scale = *reinterpret_cast<float*>(A.bankScale);
            if ((guard & 1) == 0) {
                guard |= 1;
                scale = kFourOverPi;
            }
            float bank = float(b.bankDir) * b.maxBankAngle;
            if (curvature < b.maxCurvature) {
                float ratio = curvature / b.maxCurvature;
                float angle = crt::Atanf(ratio);
                bank = angle * scale * bank;
            }
            NiMatrix3 rotation;
            Matrix3_MakeRotation(&rotation, bank, tangent.x, tangent.y, tangent.z);
            NiMatrix3 product;
            m = *Matrix3_Multiply(&rotation, &product, &m);
        }
    }
    *result = m;
}

// the partial lengths, total length, flags and parameters compared (what both IsEquals end with;
// the lengths over the path's key count)
inline uint8_t BlockTailEqual(const NiPathBlock& a, const NiPathBlock& b, uint32_t numKeys) {
    if (a.partialLengths != nullptr) {
        if (b.partialLengths == nullptr) {
            return 0;
        }
    } else if (b.partialLengths != nullptr) {
        return 0;
    }
    if (a.partialLengths != nullptr && std::memcmp(a.partialLengths, b.partialLengths, numKeys * 4) != 0) {
        return 0;
    }
    if (a.totalLength != b.totalLength) {
        return 0;
    }
    if (((a.flags ^ b.flags) & 0x3) != 0 || a.bankDir != b.bankDir || ((a.flags ^ b.flags) & 0x3C) != 0) {
        return 0;
    }
    if (a.maxBankAngle != b.maxBankAngle || a.smoothing != b.smoothing || a.followAxis != b.followAxis ||
        ((a.flags ^ b.flags) & kFlip) != 0) {
        return 0;
    }
    return 1;
}

// a data reference compared: both there and equal (slot 8), or both missing
inline uint8_t DataEqual(NiObject* data, NiObject* other) {
    if (data != nullptr) {
        if (other == nullptr) {
            return 0;
        }
    } else {
        if (other != nullptr) {
            return 0;
        }
        return 1;
    }
    return Virtual<uint8_t>(data, 8, other) ? 1 : 0;
}

// LoadBinary's part: flags, bank direction, maximum bank angle, smoothing, follow axis, then the
// two links
inline void LoadBlock(NiPathBlock& b, void* stream) {
    LoadBytes(stream, &b.flags, 2);
    int32_t bankDir;
    LoadBytes(stream, &bankDir, 4);
    b.bankDir = bankDir;
    LoadBytes(stream, &b.maxBankAngle, 4);
    LoadBytes(stream, &b.smoothing, 4);
    LoadBytes(stream, &b.followAxis, 2);
    Stream_ReadLinkID(stream);
    Stream_ReadLinkID(stream);
}

inline void SaveBlock(NiPathBlock& b, void* stream) {
    SaveBytes(stream, &b.flags, 2);
    SaveBytes(stream, &b.bankDir, 4);
    SaveBytes(stream, &b.maxBankAngle, 4);
    SaveBytes(stream, &b.smoothing, 4);
    SaveBytes(stream, &b.followAxis, 2);
    Virtual<void>(stream, 12, b.pathData);   // SaveLinkID
    Virtual<void>(stream, 12, b.pctData);
}

// LinkObject's part: the two data resolved, the length marked to compute (computed now at
// constant velocity), the path's maximum curvature
template <PathAddrs A>
void LinkBlock(void* self, void* stream) {
    NiPathBlock& b = Block<A>(self);
    NiPosData* path = static_cast<NiPosData*>(Stream_ResolveLinkID(stream));
    AssignReleasingFirst(b.pathData, path);
    NiObject* pct = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
    AssignReleasingFirst(b.pctData, pct);
    b.flags |= kLengthDirty;
    if ((b.flags & kConstVelocity) != 0 && (b.flags & kLengthDirty) != 0) {
        b.totalLength = CallGetTotalLength<A>(self);
        b.flags &= ~kLengthDirty;
    }
    NiPosData* data = b.pathData;
    if (data != nullptr && data->pos.numKeys != 0) {
        b.maxCurvature = ms_curvature[KeyIndex(POSKEY, data->pos.type)](data->pos.keys, data->pos.numKeys);
    }
}

// CopyMembers' part: the data cloned (slot 3), the flags and parameters, the partial lengths
// copied (over this one's key count)
template <PathAddrs A>
void CopyBlock(void* self, void* dest, void* cloning) {
    NiPathBlock& s = Block<A>(self);
    NiPathBlock& d = Block<A>(dest);
    d.flags = s.flags;
    if (s.pathData != nullptr) {
        NiPosData* clone = Virtual<NiPosData*>(s.pathData, 3, cloning);
        AssignReleasingFirst(d.pathData, clone);
    }
    if (s.pctData != nullptr) {
        NiObject* clone = Virtual<NiObject*>(s.pctData, 3, cloning);
        AssignReleasingFirst(d.pctData, clone);
    }
    d.flags = (s.flags & kAllowFlip) != 0 ? uint16_t(d.flags | kAllowFlip) : uint16_t(d.flags & ~kAllowFlip);
    d.flags = (s.flags & kBank) != 0 ? uint16_t(d.flags | kBank) : uint16_t(d.flags & ~kBank);
    d.bankDir = s.bankDir;
    if ((s.flags & kConstVelocity) != 0) {
        d.flags |= kConstVelocity;
        if ((d.flags & kLengthDirty) != 0) {
            d.totalLength = CallGetTotalLength<A>(dest);
            d.flags &= ~kLengthDirty;
        }
    } else {
        d.flags &= ~kConstVelocity;
    }
    d.flags = (s.flags & kFollow) != 0 ? uint16_t(d.flags | kFollow) : uint16_t(d.flags & ~kFollow);
    d.maxBankAngle = s.maxBankAngle;
    d.smoothing = s.smoothing;
    d.followAxis = s.followAxis;
    d.flags = (s.flags & kFlip) != 0 ? uint16_t(d.flags | kFlip) : uint16_t(d.flags & ~kFlip);
    d.flags = (s.flags & kOpenCurve) != 0 ? uint16_t(d.flags | kOpenCurve) : uint16_t(d.flags & ~kOpenCurve);
    if (s.partialLengths != nullptr) {
        uint32_t numKeys = s.pathData != nullptr ? s.pathData->pos.numKeys : 0;
        uint32_t bytes = numKeys * 4;
        d.partialLengths = static_cast<float*>(NiMalloc(bytes));
        crt::MemcpyS(d.partialLengths, bytes, s.partialLengths, bytes);
    }
    d.totalLength = s.totalLength;
}

// a new block's defaults (the constructors' and factories')
inline void InitBlock(NiPathBlock& b) {
    b.maxBankAngle = 0.0f;
    b.smoothing = 0.0f;
    b.maxCurvature = 0.0f;
    b.lastPathIdx = 0;
    b.totalLength = -1.0f;
    b.lastPctIdx = 0;
    b.followAxis = 0;
    b.partialLengths = nullptr;
    b.bankDir = 1;
    b.flags = kLengthDirty | kOpenCurve;
}

// ---- NiPathInterpolator

inline void InterpInit(NiPathInterpolator* self, NiPosData* path, NiObject* pct) {
    self->vtable = reinterpret_cast<void*>(NiPathInterpolator_vtable);
    self->path.pathData = path;
    self->path.pctData = pct;
    self->rotateValue = kInvalidQuaternion;
    self->positionValue = kInvalidPoint;
    InitBlock(self->path);
    self->lastUpdateTime = kInvalidFloat;
}

// 0x808DB0: GetKeyChannelCount
uint16_t __fastcall Interp_GetKeyChannelCount(NiPathInterpolator*, re::Edx) {
    return 2;
}

// 0x808DC0: GetKeyContent(channel): the path's POSKEY, the percentages' FLOATKEY
uint32_t __fastcall Interp_GetKeyContent(NiPathInterpolator*, re::Edx, uint16_t channel) {
    if (channel == 0) {
        return POSKEY;
    }
    return channel != 1 ? NUMKEYCONTENTS : FLOATKEY;
}

// 0x808DF0: ~NiPathInterpolator
void __fastcall Interp_Destruct(NiPathInterpolator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiPathInterpolator_vtable);
    if (self->path.pathData != nullptr) {
        ReleaseRef(self->path.pathData);
        self->path.pathData = nullptr;
    }
    if (self->path.pctData != nullptr) {
        ReleaseRef(self->path.pctData);
        self->path.pctData = nullptr;
    }
    NiFree(self->path.partialLengths);
    ReleaseRef(self->path.pctData);
    ReleaseRef(self->path.pathData);
    KeyBasedInterpolator_Destruct(self);
}

const RTTI* __fastcall Interp_GetRTTI(NiPathInterpolator*, re::Edx) {
    return &NiPathInterpolator_RTTI;
}

// the channel's keys (0 the path, 1 the percentages)
inline const KeyChannel* InterpChannel(NiPathInterpolator* self, uint16_t channel) {
    if (channel == 0) {
        return self->path.pathData != nullptr ? &self->path.pathData->pos : nullptr;
    }
    if (channel == 1) {
        return self->path.pctData != nullptr ? &FloatChannel(self->path.pctData) : nullptr;
    }
    return nullptr;
}

// 0x808EA0: GetKeyCount
uint32_t __fastcall Interp_GetKeyCount(NiPathInterpolator* self, re::Edx, uint16_t channel) {
    const KeyChannel* c = InterpChannel(self, channel);
    return c != nullptr ? c->numKeys : 0;
}

// 0x808ED0: GetKeyType
uint32_t __fastcall Interp_GetKeyType(NiPathInterpolator* self, re::Edx, uint16_t channel) {
    const KeyChannel* c = InterpChannel(self, channel);
    return c != nullptr ? c->type : 0;
}

// 0x808F00: GetKeyArray
NiAnimationKey* __fastcall Interp_GetKeyArray(NiPathInterpolator* self, re::Edx, uint16_t channel) {
    const KeyChannel* c = InterpChannel(self, channel);
    return c != nullptr ? c->keys : nullptr;
}

// 0x808F30: GetKeyStride
uint8_t __fastcall Interp_GetKeyStride(NiPathInterpolator* self, re::Edx, uint16_t channel) {
    const KeyChannel* c = InterpChannel(self, channel);
    return c != nullptr ? c->size : 0;
}

// 0x808F60: GetActiveTimeRange(&begin, &end): the percentages' first and last key times (by the
// key queries); none: 0 and 0
void __fastcall Interp_GetActiveTimeRange(NiPathInterpolator* self, re::Edx, float* begin, float* end) {
    *begin = kMaxFloat;
    *end = kInvalidFloat;
    if (Virtual<uint32_t>(self, 38, uint32_t(1)) != 0) {
        const NiAnimationKey* first = KeyBasedInterpolator_GetKeyAt(self, 0, 1);
        uint32_t count = Virtual<uint32_t>(self, 38, uint32_t(1));
        const NiAnimationKey* last = KeyBasedInterpolator_GetKeyAt(self, count - 1, 1);
        if (first != nullptr && last != nullptr) {
            if (first->time < *begin) {
                *begin = first->time;
            }
            if (*end < last->time) {
                *end = last->time;
            }
        }
    }
    if (double(*begin) == 3.4028234663852886e+38 && double(*end) == -3.4028234663852886e+38) {
        *begin = 0.0f;
        *end = 0.0f;
    }
}

// 0x809020: GuaranteeTimeRange(start, end): the percentages' keys at both ends
void __fastcall Interp_GuaranteeTimeRange(NiPathInterpolator* self, re::Edx, float start, float end) {
    if (self->path.pctData != nullptr) {
        FloatData_GuaranteeKeysAtStartAndEnd(self->path.pctData, start, end);
    }
}

// 0x809050: ProcessClone (the data's too)
void __fastcall Interp_ProcessClone(NiPathInterpolator* self, re::Edx, void* cloning) {
    Object_ProcessClone(self, cloning);
    if (self->path.pathData != nullptr) {
        Virtual<void>(self->path.pathData, 11, cloning);
    }
    if (self->path.pctData != nullptr) {
        Virtual<void>(self->path.pctData, 11, cloning);
    }
}

// 0x809090: RegisterStreamables
uint8_t __fastcall Interp_RegisterStreamables(NiPathInterpolator* self, re::Edx, void* stream) {
    if (!Interpolator_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->path.pathData != nullptr) {
        Virtual<uint8_t>(self->path.pathData, 6, stream);
    }
    if (self->path.pctData != nullptr) {
        Virtual<uint8_t>(self->path.pctData, 6, stream);
    }
    return 1;
}

// 0x8090D0: GetPathKeys(&count, &type, &size): the path's keys
NiAnimationKey* __fastcall Interp_GetPathKeys(NiPathInterpolator* self, re::Edx, uint32_t* numKeys, KeyType* type,
                                              uint8_t* size) {
    NiPosData* data = self->path.pathData;
    if (data != nullptr) {
        *numKeys = data->pos.numKeys;
        *type = data->pos.type;
        *size = data->pos.size;
        return data->pos.keys;
    }
    *numKeys = 0;
    *type = NOINTERP;
    *size = 0;
    return nullptr;
}

// 0x809110: the deleting destructor
NiPathInterpolator* __fastcall Interp_DeletingDestructor(NiPathInterpolator* self, re::Edx, uint32_t flags) {
    PathInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x5C);
    }
    return self;
}

// 0x809130 .. 0x80A1A0: the shared code's NiPathInterpolator copies
NiPoint3* __fastcall Interp_GetPosition(NiPathInterpolator* self, re::Edx, NiPoint3* result, uint32_t i0, uint32_t i1,
                                        float t) {
    return PathGetPosition<kInterp>(self, result, i0, i1, t);
}
void __fastcall Interp_CalcRotMatrix(NiPathInterpolator* self, re::Edx, uint32_t i0, uint32_t i1, float t,
                                     NiMatrix3* result) {
    PathCalcRotMatrix<kInterp>(self, i0, i1, t, result);
}
float __fastcall Interp_GetSpeed(NiPathInterpolator* self, re::Edx, uint32_t i0, uint32_t i1, float t) {
    return PathGetSpeed<kInterp>(self, i0, i1, t);
}
float __fastcall Interp_GetLength(NiPathInterpolator* self, re::Edx, uint32_t i0, uint32_t i1, float t) {
    return PathGetLength<kInterp>(self, i0, i1, t);
}
float __fastcall Interp_GetTotalLength(NiPathInterpolator* self, re::Edx) {
    return PathGetTotalLength<kInterp>(self);
}
void __fastcall Interp_GetPercentLoc(NiPathInterpolator* self, re::Edx, float pct, uint32_t* i0, uint32_t* i1,
                                     float* t) {
    PathGetPercentLoc<kInterp>(self, pct, i0, i1, t);
}
void __fastcall Interp_GetLoc(NiPathInterpolator* self, re::Edx, float t, uint32_t* i0, uint32_t* i1, float* u) {
    PathGetLoc<kInterp>(self, t, i0, i1, u);
}

// 0x809A40: GetSequenceInterpolator(start, end) (the percentages cut to the span)
NiInterpolator* __fastcall Interp_GetSequenceInterpolator(NiPathInterpolator* self, re::Edx, float start, float end) {
    NiPathInterpolator* clone =
        static_cast<NiPathInterpolator*>(Interpolator_GetSequenceInterpolator(self, start, end));
    if (self->path.pctData != nullptr) {
        re::Ptr32<NiObject> data = nullptr;
        FloatData_GetSequenceData(self->path.pctData, &data, start, end);
        AssignReleasingFirst(clone->path.pctData, data);
        clone->path.lastPctIdx = 0;
        ReleaseRef(data);
    }
    return clone;
}

// 0x809B00: LoadBinary
void __fastcall Interp_LoadBinary(NiPathInterpolator* self, re::Edx, void* stream) {
    Interpolator_LoadBinary(self, stream);
    LoadBlock(self->path, stream);
}

// 0x809BF0: SaveBinary
void __fastcall Interp_SaveBinary(NiPathInterpolator* self, re::Edx, void* stream) {
    Interpolator_SaveBinary(self, stream);
    SaveBlock(self->path, stream);
}

// 0x809CE0: IsEqual
uint8_t __fastcall Interp_IsEqual(NiPathInterpolator* self, re::Edx, NiPathInterpolator* other) {
    if (!Interpolator_IsEqual(self, other)) {
        return 0;
    }
    if (!DataEqual(self->path.pathData, other->path.pathData)) {
        return 0;
    }
    if (!DataEqual(self->path.pctData, other->path.pctData)) {
        return 0;
    }
    uint32_t numKeys = 0;
    if (self->path.partialLengths != nullptr && other->path.partialLengths != nullptr) {
        KeyType type;
        uint8_t size;
        reinterpret_cast<NiAnimationKey*(__thiscall*)(void*, uint32_t*, KeyType*, uint8_t*)>(kInterp.getPathKeys)(
            self, &numKeys, &type, &size);
    }
    return BlockTailEqual(self->path, other->path, numKeys);
}

// 0x809FA0: the total length computed when marked (at constant velocity's setting)
void __fastcall Interp_CalcTotalLengthIfNeeded(NiPathInterpolator* self, re::Edx) {
    if ((self->path.flags & kLengthDirty) != 0) {
        self->path.totalLength = CallGetTotalLength<kInterp>(self);
        self->path.flags &= ~kLengthDirty;
    }
}

// 0x80A300: LinkObject
void __fastcall Interp_LinkObject(NiPathInterpolator* self, re::Edx, void* stream) {
    Interpolator_LinkObject(self, stream);
    LinkBlock<kInterp>(self, stream);
}

// 0x80A3F0: NiPathInterpolator(path, percentages)
NiPathInterpolator* __fastcall Interp_Construct(NiPathInterpolator* self, re::Edx, NiPosData* path, NiObject* pct) {
    KeyBasedInterpolator_Construct(self);
    InterpInit(self, path, pct);
    AddRefImport(path);
    AddRefImport(pct);
    return self;
}

// 0x80A4B0: Update(time, target, transform&) (slot 18): the position at the time's percentage
// (and the rotation, following). The cache compares lastTime, which this never sets (it keeps
// the time at +0x58), as the game has it; a cached invalid position marks the value invalid
uint8_t __fastcall Interp_Update(NiPathInterpolator* self, re::Edx, float time, NiObjectNET*, NiQuatTransform* value) {
    if (time == self->lastTime) {
        if (kInvalidPoint.x == self->positionValue.x && kInvalidPoint.y == self->positionValue.y &&
            kInvalidPoint.z == self->positionValue.z) {
            value->translate.x = kInvalidFloat;
            value->rotate.x = kInvalidFloat;
            value->scale = kInvalidFloat;
            return 0;
        }
        value->translate = self->positionValue;
        if ((self->path.flags & kFollow) != 0) {
            value->rotate = self->rotateValue;
        }
        return 1;
    }
    NiObject* pct = self->path.pctData;
    if (pct == nullptr) {
        return 0;
    }
    KeyChannel& pk = FloatChannel(pct);
    uint8_t size = pk.size;
    uint32_t numKeys = pk.numKeys;
    KeyType type = pk.type;
    NiAnimationKey* keys = pk.keys;
    if (keys == nullptr || self->path.pathData == nullptr || self->path.pathData->pos.keys == nullptr) {
        return 0;
    }
    float u = FloatKey_GenInterp(time, keys, type, numKeys, &self->path.lastPctIdx, size);
    uint32_t i0;
    uint32_t i1;
    CallGetLoc<kInterp>(self, u, &i0, &i1, &u);
    if ((self->path.flags & kFollow) != 0) {
        NiMatrix3 m;
        CallCalcRotMatrix<kInterp>(self, i0, i1, u, &m);
        Quaternion_FromRotation(&self->rotateValue, &m);
    }
    NiPoint3 p;
    self->positionValue = *CallGetPosition<kInterp>(self, &p, i0, i1, u);
    value->translate = self->positionValue;
    if ((self->path.flags & kFollow) != 0) {
        value->rotate = self->rotateValue;
    }
    self->lastUpdateTime = time;
    return 1;
}

// 0x80A680: CopyMembers
void __fastcall Interp_CopyMembers(NiPathInterpolator* self, re::Edx, NiPathInterpolator* dest, void* cloning) {
    KeyBasedInterpolator_CopyMembers(self, dest, cloning);
    CopyBlock<kInterp>(self, dest, cloning);
}

// 0x80A830: CreateObject
NiObject* __cdecl Interp_CreateObject() {
    void* memory = NiNew(0x5C);
    if (memory == nullptr) {
        return nullptr;
    }
    NiPathInterpolator* self = static_cast<NiPathInterpolator*>(memory);
    KeyBasedInterpolator_Construct(self);
    InterpInit(self, nullptr, nullptr);
    return self;
}

// 0x80A8E0: GetViewerStrings
void __fastcall Interp_GetViewerStrings(NiPathInterpolator* self, re::Edx, NiTArray<char*>* strings) {
    KeyBasedInterpolator_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiPathInterpolator_RTTI.name));
}

// 0x80A930: CreateClone
NiPathInterpolator* __fastcall Interp_CreateClone(NiPathInterpolator* self, re::Edx, void* cloning) {
    auto copyMembers = PathInterpolator_CopyMembers;
    void* memory = NiNew(0x5C);
    if (memory == nullptr) {
        copyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiPathInterpolator* clone = static_cast<NiPathInterpolator*>(memory);
    KeyBasedInterpolator_Construct(clone);
    InterpInit(clone, nullptr, nullptr);
    copyMembers(self, clone, cloning);
    return clone;
}

// ---- NiPathController

inline void CtlrInit(NiPathController* self) {
    self->vtable = reinterpret_cast<void*>(NiPathController_vtable);
    self->path.pathData = nullptr;
    self->path.pctData = nullptr;
    InitBlock(self->path);
}

// 0x80AA00: ~NiPathController
void __fastcall Ctlr_Destruct(NiPathController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiPathController_vtable);
    if (self->path.pathData != nullptr) {
        ReleaseRef(self->path.pathData);
        self->path.pathData = nullptr;
    }
    if (self->path.pctData != nullptr) {
        ReleaseRef(self->path.pctData);
        self->path.pctData = nullptr;
    }
    NiFree(self->path.partialLengths);
    ReleaseRef(self->path.pctData);
    ReleaseRef(self->path.pathData);
    TimeController_Destruct(self);
}

const RTTI* __fastcall Ctlr_GetRTTI(NiPathController*, re::Edx) {
    return &NiPathController_RTTI;
}

// 0x80AAB0: ProcessClone
void __fastcall Ctlr_ProcessClone(NiPathController* self, re::Edx, void* cloning) {
    TimeController_ProcessClone(self, cloning);
    if (self->path.pathData != nullptr) {
        Virtual<void>(self->path.pathData, 11, cloning);
    }
    if (self->path.pctData != nullptr) {
        Virtual<void>(self->path.pctData, 11, cloning);
    }
}

// 0x80AAF0: RegisterStreamables
uint8_t __fastcall Ctlr_RegisterStreamables(NiPathController* self, re::Edx, void* stream) {
    if (!TimeController_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->path.pathData != nullptr) {
        Virtual<uint8_t>(self->path.pathData, 6, stream);
    }
    if (self->path.pctData != nullptr) {
        Virtual<uint8_t>(self->path.pctData, 6, stream);
    }
    return 1;
}

// 0x80AB30: GetPathKeys(&keys, &count, &type, &size)
void __fastcall Ctlr_GetPathKeys(NiPathController* self, re::Edx, re::Ptr32<NiAnimationKey>* keys, uint32_t* numKeys,
                                 KeyType* type, uint8_t* size) {
    NiPosData* data = self->path.pathData;
    if (data != nullptr) {
        *numKeys = data->pos.numKeys;
        *type = data->pos.type;
        *size = data->pos.size;
        *keys = data->pos.keys;
        return;
    }
    *numKeys = 0;
    *type = NOINTERP;
    *keys = nullptr;
    *size = 0;
}

// 0x80AB90: the deleting destructor
NiPathController* __fastcall Ctlr_DeletingDestructor(NiPathController* self, re::Edx, uint32_t flags) {
    PathController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x64);
    }
    return self;
}

// 0x80ABB0 .. 0x80BB20: the shared code's NiPathController copies
NiPoint3* __fastcall Ctlr_GetPosition(NiPathController* self, re::Edx, NiPoint3* result, uint32_t i0, uint32_t i1,
                                      float t) {
    return PathGetPosition<kCtlr>(self, result, i0, i1, t);
}
void __fastcall Ctlr_CalcRotMatrix(NiPathController* self, re::Edx, uint32_t i0, uint32_t i1, float t,
                                   NiMatrix3* result) {
    PathCalcRotMatrix<kCtlr>(self, i0, i1, t, result);
}
float __fastcall Ctlr_GetSpeed(NiPathController* self, re::Edx, uint32_t i0, uint32_t i1, float t) {
    return PathGetSpeed<kCtlr>(self, i0, i1, t);
}
float __fastcall Ctlr_GetLength(NiPathController* self, re::Edx, uint32_t i0, uint32_t i1, float t) {
    return PathGetLength<kCtlr>(self, i0, i1, t);
}
float __fastcall Ctlr_GetTotalLength(NiPathController* self, re::Edx) {
    return PathGetTotalLength<kCtlr>(self);
}
void __fastcall Ctlr_GetPercentLoc(NiPathController* self, re::Edx, float pct, uint32_t* i0, uint32_t* i1, float* t) {
    PathGetPercentLoc<kCtlr>(self, pct, i0, i1, t);
}
void __fastcall Ctlr_GetLoc(NiPathController* self, re::Edx, float t, uint32_t* i0, uint32_t* i1, float* u) {
    PathGetLoc<kCtlr>(self, t, i0, i1, u);
}

// 0x80B4C0: LoadBinary
void __fastcall Ctlr_LoadBinary(NiPathController* self, re::Edx, void* stream) {
    TimeController_LoadBinary(self, stream);
    LoadBlock(self->path, stream);
}

// 0x80B5B0: SaveBinary
void __fastcall Ctlr_SaveBinary(NiPathController* self, re::Edx, void* stream) {
    TimeController_SaveBinary(self, stream);
    SaveBlock(self->path, stream);
}

// 0x80B6A0: IsEqual (both data compared without a null check, as the game has it)
uint8_t __fastcall Ctlr_IsEqual(NiPathController* self, re::Edx, NiPathController* other) {
    if (!TimeController_IsEqual(self, other)) {
        return 0;
    }
    if (!Virtual<uint8_t>(self->path.pathData, 8, other->path.pathData)) {
        return 0;
    }
    if (!Virtual<uint8_t>(self->path.pctData, 8, other->path.pctData)) {
        return 0;
    }
    uint32_t numKeys = 0;
    if (self->path.partialLengths != nullptr && other->path.partialLengths != nullptr) {
        re::Ptr32<NiAnimationKey> keys;
        KeyType type;
        uint8_t size;
        reinterpret_cast<void(__thiscall*)(void*, re::Ptr32<NiAnimationKey>*, uint32_t*, KeyType*, uint8_t*)>(
            kCtlr.getPathKeys)(self, &keys, &numKeys, &type, &size);
    }
    return BlockTailEqual(self->path, other->path, numKeys);
}

// 0x80BC80: LinkObject
void __fastcall Ctlr_LinkObject(NiPathController* self, re::Edx, void* stream) {
    TimeController_LinkObject(self, stream);
    LinkBlock<kCtlr>(self, stream);
}

// 0x80BD70: Update(time) (slot 20): the target's translation (and rotation, following) at the
// scaled time's percentage. The percentages are sampled with the path's key size, as the game
// has it
void __fastcall Ctlr_Update(NiPathController* self, re::Edx, float time) {
    NiObject* pct = self->path.pctData;
    if (pct == nullptr) {
        return;
    }
    KeyChannel& pk = FloatChannel(pct);
    KeyType type = pk.type;
    NiAnimationKey* keys = pk.keys;
    uint32_t numKeys = pk.numKeys;
    if (keys == nullptr) {
        return;
    }
    NiPosData* path = self->path.pathData;
    if (path == nullptr) {
        return;
    }
    uint8_t size = path->pos.size;
    if (path->pos.keys == nullptr || TimeController_DontDoUpdate(self, time)) {
        return;
    }
    float u = FloatKey_GenInterp(self->scaledTime, keys, type, numKeys, &self->path.lastPctIdx, size);
    uint32_t i0;
    uint32_t i1;
    CallGetLoc<kCtlr>(self, u, &i0, &i1, &u);
    if ((self->path.flags & kFollow) != 0) {
        NiMatrix3 m;
        CallCalcRotMatrix<kCtlr>(self, i0, i1, u, &m);
        static_cast<NiAVObject*>(self->target)->local.rotate = m;
    }
    NiPoint3 p;
    CallGetPosition<kCtlr>(self, &p, i0, i1, u);
    static_cast<NiAVObject*>(self->target)->local.translate = p;
}

// 0x80BE80: CopyMembers
void __fastcall Ctlr_CopyMembers(NiPathController* self, re::Edx, NiPathController* dest, void* cloning) {
    TimeController_CopyMembers(self, dest, cloning);
    CopyBlock<kCtlr>(self, dest, cloning);
}

// 0x80C030: CreateObject
NiObject* __cdecl Ctlr_CreateObject() {
    void* memory = NiNew(0x64);
    if (memory == nullptr) {
        return nullptr;
    }
    NiPathController* self = static_cast<NiPathController*>(memory);
    TimeController_Construct(self);
    CtlrInit(self);
    return self;
}

// 0x80C090: GetViewerStrings
void __fastcall Ctlr_GetViewerStrings(NiPathController* self, re::Edx, NiTArray<char*>* strings) {
    TimeController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiPathController_RTTI.name));
}

// 0x80C0E0: CreateClone
NiPathController* __fastcall Ctlr_CreateClone(NiPathController* self, re::Edx, void* cloning) {
    auto copyMembers = PathController_CopyMembers;
    void* memory = NiNew(0x64);
    if (memory == nullptr) {
        copyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiPathController* clone = static_cast<NiPathController*>(memory);
    TimeController_Construct(clone);
    CtlrInit(clone);
    copyMembers(self, clone, cloning);
    return clone;
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_PATH_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC30_PATH_REVERSED("NiPathInterpolator::GetKeyChannelCount", 0x00808DB0, &ni::anim::Interp_GetKeyChannelCount);
NIC30_PATH_REVERSED("NiPathInterpolator::GetKeyContent", 0x00808DC0, &ni::anim::Interp_GetKeyContent);
NIC30_PATH_REVERSED("NiPathInterpolator::~NiPathInterpolator", 0x00808DF0, &ni::anim::Interp_Destruct);
NIC30_PATH_REVERSED("NiPathInterpolator::GetRTTI", 0x00808E90, &ni::anim::Interp_GetRTTI);
NIC30_PATH_REVERSED("NiPathInterpolator::GetKeyCount", 0x00808EA0, &ni::anim::Interp_GetKeyCount);
NIC30_PATH_REVERSED("NiPathInterpolator::GetKeyType", 0x00808ED0, &ni::anim::Interp_GetKeyType);
NIC30_PATH_REVERSED("NiPathInterpolator::GetKeyArray", 0x00808F00, &ni::anim::Interp_GetKeyArray);
NIC30_PATH_REVERSED("NiPathInterpolator::GetKeyStride", 0x00808F30, &ni::anim::Interp_GetKeyStride);
NIC30_PATH_REVERSED("NiPathInterpolator::GetActiveTimeRange", 0x00808F60, &ni::anim::Interp_GetActiveTimeRange);
NIC30_PATH_REVERSED("NiPathInterpolator::GuaranteeTimeRange", 0x00809020, &ni::anim::Interp_GuaranteeTimeRange);
NIC30_PATH_REVERSED("NiPathInterpolator::ProcessClone", 0x00809050, &ni::anim::Interp_ProcessClone);
NIC30_PATH_REVERSED("NiPathInterpolator::RegisterStreamables", 0x00809090, &ni::anim::Interp_RegisterStreamables);
NIC30_PATH_REVERSED("NiPathInterpolator::GetPathKeys", 0x008090D0, &ni::anim::Interp_GetPathKeys);
NIC30_PATH_REVERSED("NiPathInterpolator::DeletingDestructor", 0x00809110, &ni::anim::Interp_DeletingDestructor);
NIC30_PATH_REVERSED("NiPathInterpolator::GetPosition", 0x00809130, &ni::anim::Interp_GetPosition);
NIC30_PATH_REVERSED("NiPathInterpolator::CalcRotMatrix", 0x00809190, &ni::anim::Interp_CalcRotMatrix);
NIC30_PATH_REVERSED("NiPathInterpolator::GetSpeed", 0x00809940, &ni::anim::Interp_GetSpeed);
NIC30_PATH_REVERSED("NiPathInterpolator::GetLength", 0x008099D0, &ni::anim::Interp_GetLength);
NIC30_PATH_REVERSED("NiPathInterpolator::GetSequenceInterpolator", 0x00809A40,
                    &ni::anim::Interp_GetSequenceInterpolator);
NIC30_PATH_REVERSED("NiPathInterpolator::LoadBinary", 0x00809B00, &ni::anim::Interp_LoadBinary);
NIC30_PATH_REVERSED("NiPathInterpolator::SaveBinary", 0x00809BF0, &ni::anim::Interp_SaveBinary);
NIC30_PATH_REVERSED("NiPathInterpolator::IsEqual", 0x00809CE0, &ni::anim::Interp_IsEqual);
NIC30_PATH_REVERSED("NiPathInterpolator::GetTotalLength", 0x00809F00, &ni::anim::Interp_GetTotalLength);
NIC30_PATH_REVERSED("NiPathInterpolator::CalcTotalLengthIfNeeded", 0x00809FA0,
                    &ni::anim::Interp_CalcTotalLengthIfNeeded);
NIC30_PATH_REVERSED("NiPathInterpolator::GetPercentLoc", 0x00809FC0, &ni::anim::Interp_GetPercentLoc);
NIC30_PATH_REVERSED("NiPathInterpolator::GetLoc", 0x0080A1A0, &ni::anim::Interp_GetLoc);
NIC30_PATH_REVERSED("NiPathInterpolator::LinkObject", 0x0080A300, &ni::anim::Interp_LinkObject);
NIC30_PATH_REVERSED("NiPathInterpolator::NiPathInterpolator", 0x0080A3F0, &ni::anim::Interp_Construct);
NIC30_PATH_REVERSED("NiPathInterpolator::Update", 0x0080A4B0, &ni::anim::Interp_Update);
NIC30_PATH_REVERSED("NiPathInterpolator::CopyMembers", 0x0080A680, &ni::anim::Interp_CopyMembers);
NIC30_PATH_REVERSED("NiPathInterpolator::CreateObject", 0x0080A830, &ni::anim::Interp_CreateObject);
NIC30_PATH_REVERSED("NiPathInterpolator::GetViewerStrings", 0x0080A8E0, &ni::anim::Interp_GetViewerStrings);
NIC30_PATH_REVERSED("NiPathInterpolator::CreateClone", 0x0080A930, &ni::anim::Interp_CreateClone);
NIC30_PATH_REVERSED("NiPathController::~NiPathController", 0x0080AA00, &ni::anim::Ctlr_Destruct);
NIC30_PATH_REVERSED("NiPathController::GetRTTI", 0x0080AAA0, &ni::anim::Ctlr_GetRTTI);
NIC30_PATH_REVERSED("NiPathController::ProcessClone", 0x0080AAB0, &ni::anim::Ctlr_ProcessClone);
NIC30_PATH_REVERSED("NiPathController::RegisterStreamables", 0x0080AAF0, &ni::anim::Ctlr_RegisterStreamables);
NIC30_PATH_REVERSED("NiPathController::GetPathKeys", 0x0080AB30, &ni::anim::Ctlr_GetPathKeys);
NIC30_PATH_REVERSED("NiPathController::DeletingDestructor", 0x0080AB90, &ni::anim::Ctlr_DeletingDestructor);
NIC30_PATH_REVERSED("NiPathController::GetPosition", 0x0080ABB0, &ni::anim::Ctlr_GetPosition);
NIC30_PATH_REVERSED("NiPathController::CalcRotMatrix", 0x0080AC10, &ni::anim::Ctlr_CalcRotMatrix);
NIC30_PATH_REVERSED("NiPathController::GetSpeed", 0x0080B3C0, &ni::anim::Ctlr_GetSpeed);
NIC30_PATH_REVERSED("NiPathController::GetLength", 0x0080B450, &ni::anim::Ctlr_GetLength);
NIC30_PATH_REVERSED("NiPathController::LoadBinary", 0x0080B4C0, &ni::anim::Ctlr_LoadBinary);
NIC30_PATH_REVERSED("NiPathController::SaveBinary", 0x0080B5B0, &ni::anim::Ctlr_SaveBinary);
NIC30_PATH_REVERSED("NiPathController::IsEqual", 0x0080B6A0, &ni::anim::Ctlr_IsEqual);
NIC30_PATH_REVERSED("NiPathController::GetTotalLength", 0x0080B8A0, &ni::anim::Ctlr_GetTotalLength);
NIC30_PATH_REVERSED("NiPathController::GetPercentLoc", 0x0080B940, &ni::anim::Ctlr_GetPercentLoc);
NIC30_PATH_REVERSED("NiPathController::GetLoc", 0x0080BB20, &ni::anim::Ctlr_GetLoc);
NIC30_PATH_REVERSED("NiPathController::LinkObject", 0x0080BC80, &ni::anim::Ctlr_LinkObject);
NIC30_PATH_REVERSED("NiPathController::Update", 0x0080BD70, &ni::anim::Ctlr_Update);
NIC30_PATH_REVERSED("NiPathController::CopyMembers", 0x0080BE80, &ni::anim::Ctlr_CopyMembers);
NIC30_PATH_REVERSED("NiPathController::CreateObject", 0x0080C030, &ni::anim::Ctlr_CreateObject);
NIC30_PATH_REVERSED("NiPathController::GetViewerStrings", 0x0080C090, &ni::anim::Ctlr_GetViewerStrings);
NIC30_PATH_REVERSED("NiPathController::CreateClone", 0x0080C0E0, &ni::anim::Ctlr_CreateClone);
