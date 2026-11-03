// The old-style controllers of files before 10.1.0.104 turned into interpolators after loading (a
// load callback of NiStream's): a NiLookAtController into a NiLookAtInterpolator on the target's
// NiTransformController (the transform interpolator's position and scale keys moved to
// interpolators of its own), a NiRollController's keys into that look-at interpolator's roll, a
// NiPathController into a NiPathInterpolator on a transform controller (spliced in its place
// when one has to be made); each old controller taken off its target. Over a scene graph,
// recursively.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::StreamVersion;
using scene::Virtual;

GAME_VAR(uint8_t, NiTransformController_RTTI, 0x020E4EF8);
GAME_VAR(uint32_t, kNiObjectNETClassIndex, 0x020E277C);
GAME_VAR(uint32_t, kNiAVObjectClassIndex, 0x020E251C);
GAME_VAR(uint32_t, kNiNodeClassIndex, 0x020E2550);
GAME_VAR(uint32_t, kNiTransformInterpolatorClassIndex, 0x020E4B14);
GAME_VAR(uint32_t, kNiRollControllerClassIndex, 0x020E4C18);
GAME_VAR(uint32_t, kNiPathControllerClassIndex, 0x020E4D2C);
GAME_VAR(uint32_t, kNiLookAtInterpolatorClassIndex, 0x020E4DFC);
GAME_VAR(uint32_t, kNiLookAtControllerClassIndex, 0x020E4E30);
GAME_VAR(uint32_t, kNiTransformControllerClassIndex, 0x020E4F00);

// NiObjectNET's controller chain
GAME_FN(NiTimeController*, __thiscall, ObjectNET_GetController, 0x007647D0, NiObjectNET* self, const void* rtti);
GAME_FN(void, __thiscall, ObjectNET_SetController, 0x007643C0, NiObjectNET* self, NiTimeController* controller);
GAME_FN(void, __thiscall, ObjectNET_RemoveController, 0x007646E0, NiObjectNET* self, NiTimeController* controller);
// the new controllers and interpolators
GAME_FN(NiTimeController*, __thiscall, TransformController_Construct, 0x00810660, void* self);
GAME_FN(NiInterpolator*, __thiscall, LookAtInterpolator_Construct, 0x0080DE20, void* self, NiAVObject* lookAt,
        uint16_t axis, uint8_t flip);
GAME_FN(NiInterpolator*, __thiscall, Point3Interpolator_Construct, 0x00808540, void* self, NiObject* data);
GAME_FN(NiInterpolator*, __thiscall, FloatInterpolator_Construct, 0x00812260, void* self, NiObject* data);
GAME_FN(NiInterpolator*, __thiscall, PathInterpolator_Construct, 0x0080A3F0, void* self, NiPosData* path,
        NiObject* pct);
GAME_FN(void, __thiscall, PathInterpolator_CalcTotalLengthIfNeeded, 0x00809FA0, void* self);
// (the key chunk's helpers: a controller's timing copied to another; the look-at interpolator's
// object and its three interpolators set; the transform interpolator's position and scale keys
// read; the new point and float interpolators' keys set)
GAME_FN(void, __cdecl, CopyControllerTiming, 0x008046C0, NiTimeController* from, NiTimeController* to);
GAME_FN(void, __thiscall, LookAtInterpolator_SetLookAt, 0x00804400, void* self, NiAVObject* lookAt);
GAME_FN(void, __thiscall, LookAtInterpolator_SetTranslateInterpolator, 0x00804450, void* self, NiInterpolator* interp);
GAME_FN(void, __thiscall, LookAtInterpolator_SetRollInterpolator, 0x008044C0, void* self, NiInterpolator* interp);
GAME_FN(void, __thiscall, LookAtInterpolator_SetScaleInterpolator, 0x00804530, void* self, NiInterpolator* interp);
GAME_FN(NiAnimationKey*, __thiscall, TransformInterpolator_GetPosData, 0x00804620, void* self, uint32_t* numKeys,
        KeyType* type, uint8_t* size);
GAME_FN(NiAnimationKey*, __thiscall, TransformInterpolator_GetScaleData, 0x00804670, void* self, uint32_t* numKeys,
        KeyType* type, uint8_t* size);
GAME_FN(void, __thiscall, Point3Interpolator_ReplaceKeys, 0x008045A0, void* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
GAME_FN(void, __thiscall, FloatInterpolator_SetKeys, 0x00804380, void* self, NiAnimationKey* keys, uint32_t numKeys,
        KeyType type);
// NiTransformData's (src/gamebryo/NiKeyData.cpp): the arrays let go without freeing (moved)
GAME_FN(void, __thiscall, TransformData_SetPosAnim, 0x0080F960, void* self, NiAnimationKey* keys, uint32_t numKeys,
        KeyType type);
GAME_FN(void, __thiscall, TransformData_SetScaleAnim, 0x0080F9A0, void* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
// (this file's, called where modes apply)
GAME_FN(void, __cdecl, ConvertLookAtController, 0x00804750, NiLookAtController* controller);
GAME_FN(void, __cdecl, ConvertRollController, 0x008049C0, NiFloatController* controller);
GAME_FN(void, __cdecl, ConvertPathController, 0x00804AE0, NiPathController* controller);
GAME_FN(void, __cdecl, ConvertOldControllers, 0x00804D40, void* stream, NiObject* object);

// 0x804750: a NiLookAtController made into a NiLookAtInterpolator on the target's transform
// controller (a new one, given the old one's timing, when the target has none driven by a
// transform interpolator); that transform interpolator's position and scale keys moved to point
// and float interpolators of the look-at one; the old controller taken off
void __cdecl ConvertLookAt(NiLookAtController* old) {
    NiObjectNET* target = old->target;
    uint8_t created = 0;
    re::Ptr32<NiTimeController> tc = ObjectNET_GetController(target, &NiTransformController_RTTI);
    NiInterpolator* existing = tc != nullptr ? Virtual<NiInterpolator*>(tc, 31, uint32_t(0)) : nullptr;
    if (existing == nullptr || !IsKindOf(existing, kNiTransformInterpolatorClassIndex)) {
        created = 1;
        void* memory = NiNew(0x38);
        tc = memory != nullptr ? TransformController_Construct(memory) : nullptr;
        CopyControllerTiming(old, tc);
    }
    void* memory = NiNew(0x44);
    NiLookAtInterpolator* lookAt =
        memory != nullptr ? static_cast<NiLookAtInterpolator*>(LookAtInterpolator_Construct(memory, nullptr, 0, 0))
                          : nullptr;
    LookAtInterpolator_SetLookAt(lookAt, old->lookAt);
    if ((old->lookFlags & 1) != 0) {
        lookAt->flags |= 1;
    } else {
        lookAt->flags &= 0xFFFE;
    }
    lookAt->flags ^= uint16_t((uint8_t(old->lookFlags) ^ uint8_t(lookAt->flags)) & 6);
    NiInterpolator* transform = Virtual<NiInterpolator*>(tc, 31, uint32_t(0));
    if (transform != nullptr) {
        AddRefImport(transform);
        if (IsKindOf(transform, kNiTransformInterpolatorClassIndex)) {
            NiTransformInterpolator* ti = static_cast<NiTransformInterpolator*>(transform);
            if (ti->data != nullptr && ti->data->pos.numKeys != 0) {
                void* m = NiNew(0x20);
                NiInterpolator* point = m != nullptr ? Point3Interpolator_Construct(m, nullptr) : nullptr;
                uint32_t numKeys;
                KeyType type;
                uint8_t size;
                NiAnimationKey* keys = TransformInterpolator_GetPosData(ti, &numKeys, &type, &size);
                Point3Interpolator_ReplaceKeys(point, keys, numKeys, type);
                TransformData_SetPosAnim(ti->data, nullptr, 0, NOINTERP);
                LookAtInterpolator_SetTranslateInterpolator(lookAt, point);
            }
            if (ti->data != nullptr && ti->data->scale.numKeys != 0) {
                void* m = NiNew(0x18);
                NiInterpolator* scale = m != nullptr ? FloatInterpolator_Construct(m, nullptr) : nullptr;
                uint32_t numKeys;
                KeyType type;
                uint8_t size;
                NiAnimationKey* keys = TransformInterpolator_GetScaleData(ti, &numKeys, &type, &size);
                FloatInterpolator_SetKeys(scale, keys, numKeys, type);
                TransformData_SetScaleAnim(ti->data, nullptr, 0, NOINTERP);
                LookAtInterpolator_SetScaleInterpolator(lookAt, scale);
            }
        }
    }
    Virtual<void>(tc, 32, static_cast<NiInterpolator*>(lookAt), uint32_t(0));   // SetInterpolator
    Virtual<void>(lookAt, 30);                                                  // Collapse
    if (created) {
        Virtual<void>(tc, 21, target);   // SetTarget
    }
    if (target != nullptr) {
        ObjectNET_RemoveController(target, old);
    }
    ReleaseRef(transform);
}

// 0x8049C0: a NiRollController's keys made into the roll interpolator of the look-at
// interpolator on the target's transform controller (when there is one), the range guaranteed
// and the extrema reset; the old controller taken off either way
void __cdecl ConvertRoll(NiFloatController* old) {
    NiObjectNET* target = old->target;
    for (NiTimeController* c = target->controller; c != nullptr; c = c->next) {
        if (!IsKindOf(c, kNiTransformControllerClassIndex)) {
            continue;
        }
        NiInterpolator* interp = Virtual<NiInterpolator*>(c, 31, uint32_t(0));
        if (interp == nullptr || !IsKindOf(interp, kNiLookAtInterpolatorClassIndex)) {
            continue;
        }
        NiObject* data = old->data;
        void* memory = NiNew(0x18);
        NiInterpolator* roll = memory != nullptr ? FloatInterpolator_Construct(memory, data) : nullptr;
        LookAtInterpolator_SetRollInterpolator(interp, roll);
        CopyControllerTiming(old, c);
        float begin;
        float end;
        Virtual<void>(interp, 31, &begin, &end);   // GetActiveTimeRange
        Virtual<void>(interp, 32, begin, end);     // GuaranteeTimeRange
        Virtual<void>(c, 33);                      // ResetTimeExtrema
        Virtual<void>(interp, 30);                 // Collapse
        break;
    }
    ObjectNET_RemoveController(target, old);
}

// 0x804AE0: a NiPathController made into a NiPathInterpolator (its settings copied): onto the
// target's transform controller when that has no interpolator, else onto a new transform
// controller put in the old one's place in the chain; the old controller taken off
void __cdecl ConvertPath(NiPathController* old) {
    NiObjectNET* target = old->target;
    re::Ptr32<NiTimeController> tc = ObjectNET_GetController(target, &NiTransformController_RTTI);
    AddRefImport(tc);
    void* memory = NiNew(0x5C);
    NiPathInterpolator* path =
        memory != nullptr
            ? static_cast<NiPathInterpolator*>(PathInterpolator_Construct(memory, old->path.pathData, old->path.pctData))
            : nullptr;
    NiPathBlock& b = path->path;
    const uint16_t f = old->path.flags;
    b.flags = (f & 4) != 0 ? uint16_t(b.flags | 4) : uint16_t(b.flags & 0xFFFB);
    b.flags = (f & 8) != 0 ? uint16_t(b.flags | 8) : uint16_t(b.flags & 0xFFF7);
    b.bankDir = old->path.bankDir;
    if ((f & 0x10) != 0) {
        b.flags |= 0x10;
        PathInterpolator_CalcTotalLengthIfNeeded(path);
    } else {
        b.flags &= 0xFFEF;
    }
    b.flags = (old->path.flags & 0x20) != 0 ? uint16_t(b.flags | 0x20) : uint16_t(b.flags & 0xFFDF);
    b.maxBankAngle = old->path.maxBankAngle;
    b.smoothing = old->path.smoothing;
    b.followAxis = old->path.followAxis;
    b.flags = (old->path.flags & 0x40) != 0 ? uint16_t(b.flags | 0x40) : uint16_t(b.flags & 0xFFBF);
    b.flags = (old->path.flags & 2) != 0 ? uint16_t(b.flags | 2) : uint16_t(b.flags & 0xFFFD);
    if (tc != nullptr && Virtual<NiInterpolator*>(tc, 31, uint32_t(0)) == nullptr) {
        Virtual<void>(tc, 32, static_cast<NiInterpolator*>(path), uint32_t(0));   // SetInterpolator
        CopyControllerTiming(old, tc);
    } else {
        void* m = NiNew(0x38);
        NiTimeController* made = m != nullptr ? TransformController_Construct(m) : nullptr;
        AssignReleasingFirst(tc, made);
        Virtual<void>(tc, 32, static_cast<NiInterpolator*>(path), uint32_t(0));
        Virtual<void>(tc, 21, target);   // SetTarget (put first on the chain)
        CopyControllerTiming(old, tc);
        // taken back off the front and spliced in after the old controller
        ObjectNET_SetController(target, tc->next);
        AssignReleasingFirst(tc->next, old->next);
        if (old->next != tc) {
            ReleaseRef(old->next);
            old->next = tc;
            sys::Increment(&tc->refCount);
        }
    }
    Virtual<void>(path, 30);   // Collapse
    if (target != nullptr) {
        ObjectNET_RemoveController(target, old);
    }
    if (sys::Decrement(&tc->refCount) == 0) {
        Virtual<void>(tc, 1);   // DeleteThis
    }
}

// 0x804D40: the object's old controllers converted (a NiAVObject's), then a node's children's
void __cdecl ConvertControllers(void* stream, NiObject* object) {
    if (object == nullptr || !IsKindOf(object, kNiAVObjectClassIndex)) {
        return;
    }
    NiAVObject* av = static_cast<NiAVObject*>(object);
    NiTimeController* c = av->controller;
    while (c != nullptr) {
        NiTimeController* next = c->next;
        if (IsKindOf(c, kNiRollControllerClassIndex)) {
            ConvertRollController(static_cast<NiFloatController*>(c));
        } else if (IsKindOf(c, kNiLookAtControllerClassIndex)) {
            ConvertLookAtController(static_cast<NiLookAtController*>(c));
        } else if (IsKindOf(c, kNiPathControllerClassIndex)) {
            ConvertPathController(static_cast<NiPathController*>(c));
        }
        c = next;
    }
    if (IsKindOf(object, kNiNodeClassIndex)) {
        NiNode* node = static_cast<NiNode*>(object);
        for (uint32_t i = 0; i < node->children.size; i++) {
            NiAVObject* child = i < node->children.size ? node->children.base[i] : nullptr;
            ConvertOldControllers(stream, child);
        }
    }
}

// 0x804EA0: the load callback: an older file's objects converted
void __cdecl ConvertOnLoad(void* stream, NiObject* object) {
    if (StreamVersion(stream) < 0x0A010068 && object != nullptr && IsKindOf(object, kNiObjectNETClassIndex)) {
        ConvertOldControllers(stream, object);
    }
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_CONVERT_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC30_CONVERT_REVERSED("NiAnimConversion::ConvertLookAtController", 0x00804750, &ni::anim::ConvertLookAt);
NIC30_CONVERT_REVERSED("NiAnimConversion::ConvertRollController", 0x008049C0, &ni::anim::ConvertRoll);
NIC30_CONVERT_REVERSED("NiAnimConversion::ConvertPathController", 0x00804AE0, &ni::anim::ConvertPath);
NIC30_CONVERT_REVERSED("NiAnimConversion::ConvertControllers", 0x00804D40, &ni::anim::ConvertControllers);
NIC30_CONVERT_REVERSED("NiAnimConversion::ConvertOnLoad", 0x00804EA0, &ni::anim::ConvertOnLoad);
