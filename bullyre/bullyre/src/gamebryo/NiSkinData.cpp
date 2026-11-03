// NiSkinData (vtable 0x961168, 0x48 bytes): the skin's root-parent-to-skin transform and, per
// bone, its skin-to-bone transform, bound and vertex weights ({vertex, weight}, 8 bytes each);
// the skin partition (a reference: linked only from files before 10.1.0.101). Slot 6
// (RegisterStreamables) is NiObject's, shared with Ni2DBuffer (Ni2DBuffer.cpp).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiSkinData_RTTI, 0x020E3D08);

constexpr uintptr_t kVtable = 0x00961168;

struct BoneVertData {
    uint16_t vertex;   // +0x0
    uint16_t pad2;
    float weight;      // +0x4
};
static_assert(sizeof(BoneVertData) == 8);

struct BoneData {
    NiTransform skinToBone;     // +0x0
    NiBound bound;              // +0x34
    re::Ptr32<BoneVertData> weights;      // +0x44 (new[]'d)
    uint16_t vertexCount;       // +0x48
    uint16_t pad4A;
};
static_assert(sizeof(BoneData) == 0x4C);

struct NiSkinData : NiObject {
    re::Ptr32<NiObject> partition;          // +0x8 (NiSkinPartitionPtr)
    NiTransform rootParentToSkin; // +0xC
    re::Ptr32<BoneData> bones;              // +0x40 (new[]'d)
    uint32_t boneCount;           // +0x44
};
static_assert(offsetof(NiSkinData, bones) == 0x40);
static_assert(sizeof(NiSkinData) == 0x48);

GAME_FN(void, __thiscall, Transform_LoadBinary, 0x00770A40, NiTransform* self, void* stream);
GAME_FN(void, __thiscall, Transform_SaveBinary, 0x00770A90, const NiTransform* self, void* stream);
GAME_FN(uint8_t, __thiscall, Transform_NotEquals, 0x007709F0, const NiTransform* self, const NiTransform* other);
GAME_FN(void, __thiscall, Bound_LoadBinary, 0x007D23D0, NiBound* self, void* stream);
GAME_FN(void, __thiscall, Bound_SaveBinary, 0x007D2410, const NiBound* self, void* stream);
GAME_FN(uint8_t, __thiscall, Bound_Equals, 0x007D2840, const NiBound* self, const NiBound* other);
GAME_FN(void, __thiscall, Object_LinkObject, 0x008A20A0, void* self, void* stream);   // (does nothing)
GAME_FN(void, __thiscall, Object_SaveBinary, 0x008A20A0, void* self, void* stream);   // (the same)
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count, void* constructor);
inline void* const kBoneDataConstruct = reinterpret_cast<void*>(0x007DB100);   // weights 0, count 0
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, SkinData_FreeBoneVertData, 0x007DB0C0, NiSkinData* self);

inline void Unref(NiObject* object) {
    if (object != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(&object->refCount)) == 0) {
        Virtual<void>(object, 1);
    }
}

// new[] with the size overflow check (-1)
inline void* NewArray(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return NiNewArray((bytes >> 32) != 0 ? 0xFFFFFFFF : uint32_t(bytes));
}

inline void LoadRaw(void* stream, void* buffer, uint32_t bytes) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t componentSize = bytes;
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}

inline void SaveRaw(void* stream, const void* buffer, uint32_t bytes) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t componentSize = bytes;
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// 0x7DB0C0 FreeBoneVertData: each bone's weights deleted
void __fastcall SkinData_FreeBoneVertDataImpl(NiSkinData* self, re::Edx) {
    if (self->bones == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < self->boneCount; i++) {
        NiDeleteArray(self->bones[i].weights, 8);
        self->bones[i].weights = nullptr;
    }
}

// 0x7DB110 vf2
const RTTI* __fastcall SkinData_GetRTTIImpl(NiSkinData*, re::Edx) {
    return &NiSkinData_RTTI;
}

// 0x7DB120 vf5 LinkObject(stream): the partition, from old files
void __fastcall SkinData_LinkObjectImpl(NiSkinData* self, re::Edx, void* stream) {
    Object_LinkObject(self, stream);
    if (StreamVersion(stream) < 0x0A010065) {
        NiObject* partition = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        NiObject* old = self->partition;
        if (old != partition) {
            Unref(old);
            self->partition = partition;
            AddRef(partition);
        }
    }
}

// 0x7DB180 vf8 IsEqual(other): the transforms, bounds and weights
uint8_t __fastcall SkinData_IsEqualImpl(NiSkinData* self, re::Edx, NiSkinData* other) {
    if (Object_IsEqual(self, other) == 0 || Transform_NotEquals(&self->rootParentToSkin, &other->rootParentToSkin) != 0 ||
        self->boneCount != other->boneCount) {
        return 0;
    }
    for (uint32_t i = 0; i < self->boneCount; i++) {
        const BoneData& a = self->bones[i];
        const BoneData& b = other->bones[i];
        if (Transform_NotEquals(&a.skinToBone, &b.skinToBone) != 0 || Bound_Equals(&a.bound, &b.bound) == 0 ||
            a.vertexCount != b.vertexCount) {
            return 0;
        }
        uint32_t count = a.vertexCount;
        for (uint32_t j = 0; j < count; j++) {
            if (a.weights[j].vertex != b.weights[j].vertex || !(b.weights[j].weight == a.weights[j].weight)) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x7DB280 vf0
NiSkinData* __fastcall SkinData_DeletingDestructorImpl(NiSkinData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kVtable);
    SkinData_FreeBoneVertData(self);
    NiDeleteArray(self->bones, 0x4C);
    Unref(self->partition);
    Object_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x48);
    }
    return self;
}

// 0x7DB320 vf4 LoadBinary(stream): the bones' data made new; their weights only when the file
// has them (a flag byte)
void __fastcall SkinData_LoadBinaryImpl(NiSkinData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    Transform_LoadBinary(&self->rootParentToSkin, stream);
    LoadRaw(stream, &self->boneCount, 4);
    uint32_t count = self->boneCount;
    BoneData* bones = static_cast<BoneData*>(NewArray(count, 0x4C));
    if (bones != nullptr) {
        VectorConstructor(bones, 0x4C, count, kBoneDataConstruct);
    }
    self->bones = bones;
    if (StreamVersion(stream) < 0x0A010065) {
        Stream_ReadLinkID(stream);
    }
    uint8_t hasWeights;
    LoadRaw(stream, &hasWeights, 1);
    for (uint32_t i = 0; i < self->boneCount; i++) {
        BoneData& bone = self->bones[i];
        Transform_LoadBinary(&bone.skinToBone, stream);
        Bound_LoadBinary(&bone.bound, stream);
        LoadRaw(stream, &bone.vertexCount, 2);
        if (hasWeights == 0) {
            bone.weights = nullptr;
            continue;
        }
        uint32_t weightCount = bone.vertexCount;
        BoneVertData* weights = static_cast<BoneVertData*>(NewArray(weightCount, 8));
        if (weights != nullptr) {
            for (int32_t k = int32_t(weightCount) - 1; k >= 0; k--) {
                weights[weightCount - 1 - k].vertex = 0;
            }
        }
        bone.weights = weights;
        for (uint32_t j = 0; j < bone.vertexCount; j++) {
            BoneVertData* weight = &bone.weights[j];
            LoadRaw(stream, &weight->vertex, 2);
            LoadRaw(stream, &weight->weight, 4);
        }
    }
}

// 0x7DB530 vf7 SaveBinary(stream): the weights flag from the first bone's (none: a crash, as
// the game's)
void __fastcall SkinData_SaveBinaryImpl(NiSkinData* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    Transform_SaveBinary(&self->rootParentToSkin, stream);
    SaveRaw(stream, &self->boneCount, 4);
    uint8_t hasWeights = self->bones[0].weights != nullptr ? 1 : 0;
    SaveRaw(stream, &hasWeights, 1);
    for (uint32_t i = 0; i < self->boneCount; i++) {
        BoneData& bone = self->bones[i];
        Transform_SaveBinary(&bone.skinToBone, stream);
        Bound_SaveBinary(&bone.bound, stream);
        SaveRaw(stream, &bone.vertexCount, 2);
        if (hasWeights != 0) {
            for (uint32_t j = 0; j < bone.vertexCount; j++) {
                BoneVertData* weight = &bone.weights[j];
                SaveRaw(stream, &weight->vertex, 2);
                SaveRaw(stream, &weight->weight, 4);
            }
        }
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISKINDATA_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISKINDATA_REVERSED("NiSkinData::FreeBoneVertData", 0x007DB0C0, ni::SkinData_FreeBoneVertDataImpl);
NISKINDATA_REVERSED("NiSkinData::GetRTTI", 0x007DB110, ni::SkinData_GetRTTIImpl);
NISKINDATA_REVERSED("NiSkinData::LinkObject", 0x007DB120, ni::SkinData_LinkObjectImpl);
NISKINDATA_REVERSED("NiSkinData::IsEqual", 0x007DB180, ni::SkinData_IsEqualImpl);
NISKINDATA_REVERSED("NiSkinData::DeletingDestructor", 0x007DB280, ni::SkinData_DeletingDestructorImpl);
NISKINDATA_REVERSED("NiSkinData::LoadBinary", 0x007DB320, ni::SkinData_LoadBinaryImpl);
NISKINDATA_REVERSED("NiSkinData::SaveBinary", 0x007DB530, ni::SkinData_SaveBinaryImpl);
