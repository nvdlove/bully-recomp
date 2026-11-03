// NiMultiTargetTransformController (vtable 0x966290, 0x40 bytes): one blend transform
// interpolator (NiBlendTransformInterpolator, 0x30 bytes each, held in a NiNew[] array of its
// own) per target NiAVObject, the targets' local transforms set from them each update (the ones
// marked for selective update). Its interpolator slots, streaming (the targets as links), cloning
// (the targets mapped to their clones), comparison, the time extrema, viewer strings; and the
// array's vector deleting destructor (NiBlendTransformInterpolator's slot 0).

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
using scene::GetViewerString;
using scene::GetViewerStringPointer;
using scene::LoadBytes;
using scene::SaveBytes;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiMultiTargetTransformController_RTTI, 0x020E4D58);

constexpr uintptr_t NiMultiTargetTransformController_vtable = 0x00966290;
constexpr uintptr_t BlendTransformInterpolator_Construct_address = 0x0081EFE0;
constexpr uint32_t kBlendSize = 0x30;

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(char*, __cdecl, GetViewerStringUshort, 0x00785650, const char* prefix, uint16_t value);
GAME_FN(void*, __cdecl, NiAllocArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(NiObject*, __thiscall, Stream_ReadLinkIDObject, 0x0075A820, void* stream);
GAME_FN(void, __thiscall, Quaternion_FromRotation, 0x00776040, NiQuaternion* self, const NiMatrix3* m);
GAME_FN(void, __thiscall, Quaternion_ToRotation, 0x00413C50, const NiQuaternion* self, NiMatrix3* m);
// (the pose by value: translation, rotation (w, x, y, z), scale)
GAME_FN(NiInterpolator*, __thiscall, TransformInterpolator_ConstructWithPose, 0x00805320, void* self, float tx,
        float ty, float tz, float rw, float rx, float ry, float rz, float scale);

inline NiInterpolator* ConstructWithPose(void* self, const NiQuatTransform& p) {
    return TransformInterpolator_ConstructWithPose(self, p.translate.x, p.translate.y, p.translate.z, p.rotate.w,
                                                   p.rotate.x, p.rotate.y, p.rotate.z, p.scale);
}
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
GAME_FN(void, __thiscall, TimeController_ProcessClone, 0x0077DEE0, void* self, void* cloning);
// NiInterpController's (the next chunk's)
GAME_FN(void, __thiscall, InterpController_Construct, 0x00824F10, void* self);
GAME_FN(uint8_t, __thiscall, InterpController_IsEqual, 0x00824F00, void* self, void* other);
GAME_FN(void, __thiscall, InterpController_CopyMembers, 0x00824ED0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, InterpController_SaveBinary, 0x00824EF0, void* self, void* stream);
GAME_FN(void, __thiscall, InterpController_ResetTimeExtrema, 0x00824F60, void* self);
GAME_FN(void, __thiscall, InterpController_LoadBinary, 0x00824F90, void* self, void* stream);
GAME_FN(void, __thiscall, InterpController_GetViewerStrings, 0x00825000, void* self, NiTArray<char*>* strings);
// NiBlendTransformInterpolator's (the next chunk's)
GAME_FN(void, __thiscall, BlendTransformInterpolator_Destruct, 0x00826A30, void* self);
GAME_FN(void, __thiscall, BlendTransformInterpolator_CopyMembers, 0x0081F0C0, void* self, void* dest, void* cloning);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, MultiTarget_Destruct, 0x0080C2B0, NiMultiTargetTransformController* self);
GAME_FN(void, __thiscall, MultiTarget_SetNumInterps, 0x0080C5E0, NiMultiTargetTransformController* self,
        uint16_t count);
GAME_FN(void, __thiscall, MultiTarget_CopyMembers, 0x0080C790, NiMultiTargetTransformController* self,
        NiMultiTargetTransformController* dest, void* cloning);

inline NiInterpolator* Blend(const NiMultiTargetTransformController* self, uint32_t index) {
    return reinterpret_cast<NiInterpolator*>(self->blendInterps + index * kBlendSize);
}

// the clone of an object in the cloning's map (NiTPointerMap<NiObject*, NiObject*>: the
// buckets at +0x8, their count at +0x4; an item: next, key, value), or nothing
inline const re::Ptr32<void>* FindClone(void* cloning, const void* object) {
    void* map = *static_cast<re::Ptr32<void>*>(cloning);
    re::Ptr32<void>* buckets = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reinterpret_cast<uintptr_t>(map) + 8);
    uint32_t count = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(map) + 4);
    for (re::Ptr32<void>* item = static_cast<re::Ptr32<void>*>(buckets[reinterpret_cast<uintptr_t>(object) % count]); item != nullptr;
         item = static_cast<re::Ptr32<void>*>(item[0])) {
        if (item[1] == object) {
            return &item[2];
        }
    }
    return nullptr;
}

// 0x80C170: GetInterpolatorCount (slot 28)
uint16_t __fastcall MT_GetInterpolatorCount(NiMultiTargetTransformController* self, re::Edx) {
    return self->numInterps;
}

// 0x80C180: GetInterpolator(index) (slot 31): the blend interpolator
NiInterpolator* __fastcall MT_GetInterpolator(NiMultiTargetTransformController* self, re::Edx, uint16_t index) {
    return Blend(self, index);
}

// 0x80C1A0: CreateBlendInterpolator(index, ...) (slot 37): the one it has
NiInterpolator* __fastcall MT_CreateBlendInterpolator(NiMultiTargetTransformController* self, re::Edx, uint16_t index,
                                                      uint32_t, uint32_t, float, uint32_t) {
    return Blend(self, index);
}

// 0x80C1C0: GuaranteeTimeRange(start, end) (slot 38): each blend interpolator's
void __fastcall MT_GuaranteeTimeRange(NiMultiTargetTransformController* self, re::Edx, float start, float end) {
    for (uint16_t i = 0; i < self->numInterps; i++) {
        Virtual<void>(Blend(self, i), 32, start, end);
    }
}

// 0x80C220: IsEqual (the targets both missing, or equal by the other's IsEqual)
uint8_t __fastcall MT_IsEqual(NiMultiTargetTransformController* self, re::Edx, NiMultiTargetTransformController* other) {
    if (!InterpController_IsEqual(self, other) || other->numInterps != self->numInterps) {
        return 0;
    }
    for (uint16_t i = 0; i < self->numInterps; i++) {
        NiAVObject* mine = self->targets[i];
        NiAVObject* theirs = other->targets[i];
        if (mine != nullptr) {
            if (theirs == nullptr) {
                return 0;
            }
        } else {
            if (theirs != nullptr) {
                return 0;
            }
            continue;
        }
        if (!Virtual<uint8_t>(theirs, 8, mine)) {
            return 0;
        }
    }
    return 1;
}

// 0x80C2B0: ~NiMultiTargetTransformController (the array through its vector deleting destructor;
// an empty one freed as it is)
void __fastcall MT_Destruct(NiMultiTargetTransformController* self, re::Edx) {
    uint8_t* blends = self->blendInterps;
    self->vtable = reinterpret_cast<void*>(NiMultiTargetTransformController_vtable);
    if (blends != nullptr) {
        uint32_t* block = reinterpret_cast<uint32_t*>(blends) - 1;
        if (block[0] != 0) {
            Virtual<void*>(blends, 0, uint32_t(3));
        } else {
            OperatorDelete(block);
        }
    }
    NiFree(self->targets);
    TimeController_Destruct(self);
}

const RTTI* __fastcall MT_GetRTTI(NiMultiTargetTransformController*, re::Edx) {
    return &NiMultiTargetTransformController_RTTI;
}

// 0x80C320: Update(time) (slot 20): each selectively updated target's translation, rotation and
// scale (absolute) from its blend interpolator, the parts it made valid. (The value carries over
// from one target to the next where an interpolator leaves parts alone, as the game has it)
void __fastcall MT_Update(NiMultiTargetTransformController* self, re::Edx, float time) {
    NiQuatTransform value;
    value.translate = kInvalidTranslate;
    value.rotate = kInvalidRotate;
    value.scale = kInvalidFloat;
    for (uint16_t i = 0; i < self->numInterps; i++) {
        NiAVObject* target = self->targets[i];
        if (target == nullptr || ((target->flags >> 1) & 1) == 0) {
            continue;
        }
        if (!Virtual<uint8_t>(Blend(self, i), 18, time, static_cast<NiObjectNET*>(target), &value)) {
            continue;
        }
        if (value.translate.x != kInvalidFloat) {
            self->targets[i]->local.translate = value.translate;
        }
        if (value.rotate.x != kInvalidFloat) {
            Quaternion_ToRotation(&value.rotate, &self->targets[i]->local.rotate);
        }
        if (value.scale != kInvalidFloat) {
            self->targets[i]->local.scale = std::fabs(value.scale);
        }
    }
}

// 0x80C470: GetInterpolatorID(index) (slot 29): the target's name
const char* __fastcall MT_GetInterpolatorID(NiMultiTargetTransformController* self, re::Edx, uint16_t index) {
    return self->targets[index]->name;
}

// 0x80C490: GetInterpolatorIndex(id) (slot 30): the target named so (0xFFFF: none)
uint16_t __fastcall MT_GetInterpolatorIndex(NiMultiTargetTransformController* self, re::Edx, const char* id) {
    if (id == nullptr) {
        return 0xFFFF;
    }
    for (uint16_t i = 0; i < self->numInterps; i++) {
        NiAVObject* target = self->targets[i];
        if (target != nullptr && std::strcmp(target->name, id) == 0) {
            return i;
        }
    }
    return 0xFFFF;
}

// 0x80C510: ResetTimeExtrema (slot 33): the key times over the blend interpolators' active ranges
// (none: the base's), when the manager doesn't control it
void __fastcall MT_ResetTimeExtrema(NiMultiTargetTransformController* self, re::Edx) {
    if (((uint8_t(self->flags) >> 5) & 1) != 0) {
        return;
    }
    self->loKeyTime = 3.40282347e+38f;
    self->hiKeyTime = kInvalidFloat;
    for (uint16_t i = 0; i < self->numInterps; i++) {
        float end = kInvalidFloat;
        float begin = 3.40282347e+38f;
        Virtual<void>(Blend(self, i), 31, &begin, &end);
        if (begin < self->loKeyTime) {
            self->loKeyTime = begin;
        }
        if (self->hiKeyTime < end) {
            self->hiKeyTime = end;
        }
    }
    if (self->loKeyTime == 3.40282347e+38f && self->hiKeyTime == kInvalidFloat) {
        InterpController_ResetTimeExtrema(self);
    }
}

// 0x80C5E0: SetNumInterps(count): the blend interpolators made (each held) and the targets
// cleared (nothing for 0; the old ones not let go)
void __fastcall MT_SetNumInterps(NiMultiTargetTransformController* self, re::Edx, uint16_t count) {
    self->numInterps = count;
    if (count == 0) {
        return;
    }
    uint32_t n = count;
    uint32_t* block = static_cast<uint32_t*>(NiAllocArray(KeyArraySize(n, kBlendSize)));
    uint8_t* blends = nullptr;
    if (block != nullptr) {
        blends = reinterpret_cast<uint8_t*>(block + 1);
        block[0] = n;
        VectorConstruct(blends, kBlendSize, n, reinterpret_cast<void*>(BlendTransformInterpolator_Construct_address));
    }
    self->blendInterps = blends;
    for (uint32_t i = 0; i < n; i++) {
        AddRefImport(reinterpret_cast<NiRefObject*>(self->blendInterps + i * kBlendSize));
    }
    void* targets = NiMalloc(n * 4);
    self->targets = static_cast<re::Ptr32<NiAVObject>*>(targets);
    crt::Memset(targets, 0, n * 4);
}

// 0x80C690: NiBlendTransformInterpolator's vector deleting destructor (flags 2: the whole array,
// last to first, the block freed with flags 1)
void* __fastcall BlendTransform_VectorDeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    if ((flags & 2) != 0) {
        uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
        for (int32_t i = int32_t(block[0]) - 1; i >= 0; i--) {
            BlendTransformInterpolator_Destruct(self + uint32_t(i) * kBlendSize);
        }
        if ((flags & 1) != 0) {
            NiFreeArray(block, kBlendSize);
        }
        return block;
    }
    BlendTransformInterpolator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, kBlendSize);
    }
    return self;
}

// 0x80C700: CreatePoseInterpolator(index) (slot 35): a transform interpolator posed at the
// target's local transform
NiInterpolator* __fastcall MT_CreatePoseInterpolator(NiMultiTargetTransformController* self, re::Edx, uint16_t index) {
    NiAVObject* target = self->targets[index];
    NiQuaternion q;
    Quaternion_FromRotation(&q, &target->local.rotate);
    NiQuatTransform pose;
    pose.translate = target->local.translate;
    pose.rotate = q;
    pose.scale = target->local.scale;
    void* memory = NiNew(0x3C);
    if (memory == nullptr) {
        return nullptr;
    }
    return ConstructWithPose(memory, pose);
}

// 0x80C790: CopyMembers (as many blend interpolators made, each copied)
void __fastcall MT_CopyMembers(NiMultiTargetTransformController* self, re::Edx, NiMultiTargetTransformController* dest,
                               void* cloning) {
    InterpController_CopyMembers(self, dest, cloning);
    MultiTarget_SetNumInterps(dest, self->numInterps);
    for (uint16_t i = 0; i < self->numInterps; i++) {
        BlendTransformInterpolator_CopyMembers(Blend(self, i), Blend(dest, i), cloning);
    }
}

// 0x80C7F0: the deleting destructor
NiMultiTargetTransformController* __fastcall MT_DeletingDestructor(NiMultiTargetTransformController* self, re::Edx,
                                                                   uint32_t flags) {
    MultiTarget_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x40);
    }
    return self;
}

inline void InitMembers(NiMultiTargetTransformController* self) {
    self->vtable = reinterpret_cast<void*>(NiMultiTargetTransformController_vtable);
    self->blendInterps = nullptr;
    self->targets = nullptr;
    self->numInterps = 0;
}

// 0x80C810: CreateClone (a failed allocation still copied onto null)
NiMultiTargetTransformController* __fastcall MT_CreateClone(NiMultiTargetTransformController* self, re::Edx,
                                                            void* cloning) {
    void* memory = NiNew(0x40);
    if (memory == nullptr) {
        MultiTarget_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiMultiTargetTransformController* clone = static_cast<NiMultiTargetTransformController*>(memory);
    InterpController_Construct(clone);
    InitMembers(clone);
    MultiTarget_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x80C870: ProcessClone (slot 11): the blend interpolators', then the clone's targets mapped to
// the targets' clones (this one's clone missing from the map: written through null, as the
// game has it)
void __fastcall MT_ProcessClone(NiMultiTargetTransformController* self, re::Edx, void* cloning) {
    TimeController_ProcessClone(self, cloning);
    const re::Ptr32<void>* found = FindClone(cloning, self);
    NiMultiTargetTransformController* clone =
        found != nullptr ? static_cast<NiMultiTargetTransformController*>(*found) : nullptr;
    for (uint16_t i = 0; i < self->numInterps; i++) {
        Virtual<void>(Blend(self, i), 11, cloning);
        NiAVObject* target = self->targets[i];
        if (target != nullptr) {
            const re::Ptr32<void>* mapped = FindClone(cloning, target);
            if (mapped != nullptr) {
                clone->targets[i] = static_cast<NiAVObject*>(*mapped);
            }
        }
    }
}

// 0x80C930: CreateObject
NiObject* __cdecl MT_CreateObject() {
    void* memory = NiNew(0x40);
    if (memory == nullptr) {
        return nullptr;
    }
    NiMultiTargetTransformController* self = static_cast<NiMultiTargetTransformController*>(memory);
    InterpController_Construct(self);
    InitMembers(self);
    return self;
}

// 0x80C970: LoadBinary (the targets read as resolved links)
void __fastcall MT_LoadBinary(NiMultiTargetTransformController* self, re::Edx, void* stream) {
    InterpController_LoadBinary(self, stream);
    LoadBytes(stream, &self->numInterps, 2);
    MultiTarget_SetNumInterps(self, self->numInterps);
    for (uint16_t i = 0; i < self->numInterps; i++) {
        self->targets[i] = static_cast<NiAVObject*>(Stream_ReadLinkIDObject(stream));
    }
}

// 0x80C9E0: SaveBinary
void __fastcall MT_SaveBinary(NiMultiTargetTransformController* self, re::Edx, void* stream) {
    InterpController_SaveBinary(self, stream);
    SaveBytes(stream, &self->numInterps, 2);
    for (uint16_t i = 0; i < self->numInterps; i++) {
        Virtual<void>(stream, 12, self->targets[i]);   // SaveLinkID
    }
}

// 0x80CA50: GetViewerStrings
void __fastcall MT_GetViewerStrings(NiMultiTargetTransformController* self, re::Edx, NiTArray<char*>* strings) {
    char buffer[0x400];
    InterpController_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiMultiTargetTransformController_RTTI.name));
    AddViewerString(strings, GetViewerStringUshort("m_usNumInterps", self->numInterps));
    for (uint16_t i = 0; i < self->numInterps; i++) {
        if (self->targets[i] != nullptr) {
            NiSprintf(buffer, 0x400, "Target%d", uint32_t(i));
            AddViewerString(strings, GetViewerStringPointer(buffer, self->targets[i]));
        }
    }
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_MULTITARGET_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (0x80C210: a lone jmp to NiTimeController::RegisterStreamables 0x77DF70)
RE_UNHOOKED("NiMultiTargetTransformController::RegisterStreamables (thunk)", 0x0080C210);

NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::GetInterpolatorCount", 0x0080C170,
                           &ni::anim::MT_GetInterpolatorCount);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::GetInterpolator", 0x0080C180,
                           &ni::anim::MT_GetInterpolator);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::CreateBlendInterpolator", 0x0080C1A0,
                           &ni::anim::MT_CreateBlendInterpolator);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::GuaranteeTimeRange", 0x0080C1C0,
                           &ni::anim::MT_GuaranteeTimeRange);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::IsEqual", 0x0080C220, &ni::anim::MT_IsEqual);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::~NiMultiTargetTransformController", 0x0080C2B0,
                           &ni::anim::MT_Destruct);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::GetRTTI", 0x0080C310, &ni::anim::MT_GetRTTI);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::Update", 0x0080C320, &ni::anim::MT_Update);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::GetInterpolatorID", 0x0080C470,
                           &ni::anim::MT_GetInterpolatorID);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::GetInterpolatorIndex", 0x0080C490,
                           &ni::anim::MT_GetInterpolatorIndex);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::ResetTimeExtrema", 0x0080C510,
                           &ni::anim::MT_ResetTimeExtrema);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::SetNumInterps", 0x0080C5E0,
                           &ni::anim::MT_SetNumInterps);
NIC30_MULTITARGET_REVERSED("NiBlendTransformInterpolator::VectorDeletingDestructor", 0x0080C690,
                           &ni::anim::BlendTransform_VectorDeletingDestructor);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::CreatePoseInterpolator", 0x0080C700,
                           &ni::anim::MT_CreatePoseInterpolator);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::CopyMembers", 0x0080C790, &ni::anim::MT_CopyMembers);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::DeletingDestructor", 0x0080C7F0,
                           &ni::anim::MT_DeletingDestructor);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::CreateClone", 0x0080C810, &ni::anim::MT_CreateClone);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::ProcessClone", 0x0080C870,
                           &ni::anim::MT_ProcessClone);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::CreateObject", 0x0080C930,
                           &ni::anim::MT_CreateObject);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::LoadBinary", 0x0080C970, &ni::anim::MT_LoadBinary);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::SaveBinary", 0x0080C9E0, &ni::anim::MT_SaveBinary);
NIC30_MULTITARGET_REVERSED("NiMultiTargetTransformController::GetViewerStrings", 0x0080CA50,
                           &ni::anim::MT_GetViewerStrings);
