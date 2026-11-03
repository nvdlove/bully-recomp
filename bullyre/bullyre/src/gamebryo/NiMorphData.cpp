// NiMorphData (vtable 0x966350, 0x18 bytes): the morph targets of a geometry morpher, each a
// vertex array with its name and (from older files) a float interpolator for its weight, NiNew[]'d
// with the count before them (MorphTarget, 0x10 bytes). Streaming (older files: the weight keys
// made into an interpolator, the names made up), comparison, viewer strings; and
// NiFloatInterpolator's key replacement the older load uses.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::AddViewerString;
using scene::AddViewerStringInline;
using scene::At;
using scene::BinaryStream;
using scene::GetViewerString;
using scene::GetViewerStringString;
using scene::GetViewerStringUint;
using scene::LoadBytes;
using scene::Object_GetViewerStrings;
using scene::Object_IsEqual;
using scene::Object_LoadBinary;
using scene::Object_RegisterStreamables;
using scene::RefObject_Construct;
using scene::SaveBytes;
using scene::Stream_LoadFixedString;
using scene::Stream_LoadInlineFixedString;
using scene::Stream_SaveFixedString;
using scene::StreamVersion;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiMorphData_RTTI, 0x020E4D8C);

constexpr uintptr_t NiMorphData_vtable = 0x00966350;
constexpr uintptr_t NiFloatData_vtable = 0x00965AC0;
constexpr uintptr_t MorphTarget_Construct_address = 0x0080CCB0;   // (this file's, the array's constructor)

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void*, __cdecl, NiAllocArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __thiscall, Object_Destruct, 0x007769D0, void* self);
GAME_FN(void, __thiscall, Object_SaveBinaryNone, 0x008A20A0, void* self, void* stream);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
// NiFloatInterpolator's and NiFloatData's (the next chunk's)
GAME_FN(NiInterpolator*, __thiscall, FloatInterpolator_Construct, 0x00812260, void* self, NiObject* data);
GAME_FN(void, __thiscall, FloatData_ReplaceAnim, 0x00812D30, NiObject* self, NiAnimationKey* keys, uint32_t numKeys,
        KeyType type);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, FloatInterpolator_ReplaceKeys, 0x0080CC30, void* self, NiAnimationKey* keys,
        uint32_t numKeys, KeyType type);
GAME_FN(void, __thiscall, MorphTarget_Destruct, 0x0080CCD0, MorphTarget* self);
GAME_FN(uint8_t, __thiscall, MorphTarget_IsEqual, 0x0080CD30, const MorphTarget* self, const MorphTarget* other,
        uint32_t numVerts);
GAME_FN(void*, __thiscall, MorphTarget_VectorDeletingDestructor, 0x0080CDE0, MorphTarget* self, uint32_t flags);
GAME_FN(void, __thiscall, MorphTarget_LoadBinary, 0x0080CF50, MorphTarget* self, void* stream, uint32_t numVerts);

// the stream's floats, 4 bytes a component
inline void LoadFloats(void* stream, void* buffer, uint32_t bytes) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t componentSize = 4;
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}

inline void SaveFloats(void* stream, const void* buffer, uint32_t bytes) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t componentSize = 4;
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// a vertex array's NiAllocArray size (no count before it; -1 when it overflows)
inline uint32_t VertsSize(uint32_t count) {
    uint64_t product = uint64_t(count) * 0xC;
    return (product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product);
}

// a target's parts let go (its destructor, inlined in the vector one)
inline void DestroyTarget(MorphTarget* target) {
    NiFreeArray(target->verts, 0xC);
    ReleaseRef(target->interpolator);
    sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&target->name));
}

// 0x80CB80: CreateObject
NiObject* __cdecl Morph_CreateObject() {
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    NiMorphData* self = static_cast<NiMorphData*>(memory);
    RefObject_Construct(self);
    self->numTargets = 0;
    self->numVertsPerTarget = 0;
    self->targets = nullptr;
    self->relativeTargets = 0;
    self->vtable = reinterpret_cast<void*>(NiMorphData_vtable);
    return self;
}

const RTTI* __fastcall Morph_GetRTTI(NiMorphData*, re::Edx) {
    return &NiMorphData_RTTI;
}

// 0x80CBD0: RegisterStreamables (the targets' names)
uint8_t __fastcall Morph_RegisterStreamables(NiMorphData* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    for (uint32_t i = 0; i < self->numTargets; i++) {
        Virtual<void>(stream, 8, &self->targets[i].name);   // RegisterFixedString
    }
    return 1;
}

// 0x80CC30: NiFloatInterpolator::ReplaceKeys(keys, count, type) (its data made when missing; no
// keys: the data's emptied, when there is data)
void __fastcall FloatInterp_ReplaceKeys(void* self, re::Edx, NiAnimationKey* keys, uint32_t numKeys, KeyType type) {
    re::Ptr32<NiObject>& data = At<NiObject*>(self, 0x10);
    if (numKeys != 0) {
        if (data == nullptr) {
            void* memory = NiNew(0x18);
            NiObject* made = nullptr;
            if (memory != nullptr) {
                RefObject_Construct(memory);
                made = static_cast<NiObject*>(memory);
                made->vtable = reinterpret_cast<void*>(NiFloatData_vtable);
                At<uint32_t>(made, 8) = 0;
                At<void*>(made, 0xC) = nullptr;
                At<uint32_t>(made, 0x10) = 0;
                At<uint8_t>(made, 0x14) = 0;
            }
            Pointer_Assign(&data, made);
        }
        FloatData_ReplaceAnim(data, keys, numKeys, type);
        At<uint32_t>(self, 0x14) = 0;   // the last index
        return;
    }
    if (data != nullptr) {
        FloatData_ReplaceAnim(data, nullptr, 0, NOINTERP);
    }
}

// 0x80CCB0: MorphTarget()
MorphTarget* __fastcall Target_Construct(MorphTarget* self, re::Edx) {
    self->verts = nullptr;
    self->name = nullptr;
    self->legacyWeight = 0.0f;
    self->interpolator = nullptr;
    return self;
}

// 0x80CCD0: ~MorphTarget
void __fastcall Target_Destruct(MorphTarget* self, re::Edx) {
    DestroyTarget(self);
}

// 0x80CD30: MorphTarget::IsEqual(other, numVerts) (the vertices, then the name handle)
uint8_t __fastcall Target_IsEqual(const MorphTarget* self, re::Edx, const MorphTarget* other, uint32_t numVerts) {
    for (uint32_t i = 0; i < numVerts; i++) {
        const NiPoint3& a = self->verts[i];
        const NiPoint3& b = other->verts[i];
        if (b.x != a.x || b.y != a.y || b.z != a.z) {
            return 0;
        }
    }
    return self->name == other->name ? 1 : 0;
}

// 0x80CDE0: MorphTarget's vector deleting destructor
void* __fastcall Target_VectorDeletingDestructor(MorphTarget* self, re::Edx, uint32_t flags) {
    if ((flags & 2) != 0) {
        uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
        for (int32_t i = int32_t(block[0]) - 1; i >= 0; i--) {
            DestroyTarget(&self[i]);
        }
        if ((flags & 1) != 0) {
            NiFreeArray(block, 0x10);
        }
        return block;
    }
    MorphTarget_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x80CEB0: IsEqual
uint8_t __fastcall Morph_IsEqual(NiMorphData* self, re::Edx, NiMorphData* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    uint32_t count = self->numTargets;
    if (count != other->numTargets || self->numVertsPerTarget != other->numVertsPerTarget ||
        self->relativeTargets != other->relativeTargets) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        const MorphTarget* theirs = i < other->numTargets ? &other->targets[i] : nullptr;
        if (!MorphTarget_IsEqual(&self->targets[i], theirs, self->numVertsPerTarget)) {
            return 0;
        }
    }
    return 1;
}

// 0x80CF50: MorphTarget::LoadBinary(stream, numVerts) (the name; the legacy weight between
// 10.1.0.104 and 20.1.0.3; the vertices)
void __fastcall Target_LoadBinary(MorphTarget* self, re::Edx, void* stream, uint32_t numVerts) {
    if (StreamVersion(stream) < 0x14010001) {
        Stream_LoadInlineFixedString(stream, &self->name);
    } else {
        Stream_LoadFixedString(stream, &self->name);
    }
    if (StreamVersion(stream) > 0x0A010067 && StreamVersion(stream) < 0x14010003) {
        LoadBytes(stream, &self->legacyWeight, 4);
    }
    self->verts = static_cast<NiPoint3*>(NiAllocArray(VertsSize(numVerts)));
    LoadFloats(stream, self->verts, numVerts * 0xC);
}

// 0x80D010: LoadBinary. Before 10.1.0.104 each target: its weight keys made into a float
// interpolator (collapsed), its name "MT n", its vertices one by one
void __fastcall Morph_LoadBinary(NiMorphData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    LoadBytes(stream, &self->numTargets, 4);
    LoadBytes(stream, &self->numVertsPerTarget, 4);
    uint8_t relative;
    LoadBytes(stream, &relative, 1);
    uint32_t count = self->numTargets;
    self->relativeTargets = relative == 1 ? 1 : 0;
    uint32_t* block = static_cast<uint32_t*>(NiAllocArray(KeyArraySize(count, 0x10)));
    MorphTarget* targets = nullptr;
    if (block != nullptr) {
        targets = reinterpret_cast<MorphTarget*>(block + 1);
        block[0] = count;
        VectorConstruct(targets, 0x10, count, reinterpret_cast<void*>(MorphTarget_Construct_address));
    }
    self->targets = targets;
    for (uint32_t i = 0; i < self->numTargets; i++) {
        if (StreamVersion(stream) >= 0x0A010068) {
            MorphTarget_LoadBinary(&self->targets[i], stream, self->numVertsPerTarget);
            continue;
        }
        uint32_t numKeys;
        KeyType type;
        LoadBytes(stream, &numKeys, 4);
        LoadBytes(stream, &type, 4);
        if (numKeys != 0) {
            uint8_t size = ms_keySizes[KeyIndex(FLOATKEY, type)];
            NiAnimationKey* keys = ms_create[KeyIndex(FLOATKEY, type)](stream, numKeys);
            ms_fillDerived[KeyIndex(FLOATKEY, type)](keys, numKeys, size);
            void* memory = NiNew(0x18);
            NiInterpolator* made = memory != nullptr ? FloatInterpolator_Construct(memory, nullptr) : nullptr;
            FloatInterpolator_ReplaceKeys(made, keys, numKeys, type);
            AssignReleasingFirst(self->targets[i].interpolator, made);
            Virtual<void>(made, 30);   // Collapse
        }
        char buffer[0x100];
        NiSprintf(buffer, 0x100, "MT %d", i);
        re::Ptr32<char> name = sys::GlobalStringTable_AddString(buffer);
        re::Ptr32<char>& slot = *re::PtrCast<re::Ptr32<char>*>(&self->targets[i].name);
        if (slot != name) {
            sys::FixedStringAddRef(name);
            sys::FixedStringRelease(&slot);
            slot = name;
        }
        sys::FixedStringRelease(&name);
        NiPoint3* verts = static_cast<NiPoint3*>(NiAllocArray(VertsSize(self->numVertsPerTarget)));
        for (uint32_t v = 0; v < self->numVertsPerTarget; v++) {
            Point3_LoadBinary(&verts[v], stream);
        }
        MorphTarget& target = self->targets[i];
        NiFreeArray(target.verts, 0xC);
        target.verts = verts;
    }
}

// 0x80D380: SaveBinary
void __fastcall Morph_SaveBinary(NiMorphData* self, re::Edx, void* stream) {
    Object_SaveBinaryNone(self, stream);
    SaveBytes(stream, &self->numTargets, 4);
    SaveBytes(stream, &self->numVertsPerTarget, 4);
    uint8_t relative = self->relativeTargets;
    SaveBytes(stream, &relative, 1);
    for (uint32_t i = 0; i < self->numTargets; i++) {
        MorphTarget* target = &self->targets[i];
        uint32_t numVerts = self->numVertsPerTarget;
        Stream_SaveFixedString(stream, &target->name);
        if (numVerts != 0) {
            SaveFloats(stream, target->verts, numVerts * 0xC);
        }
    }
}

// 0x80D490: the deleting destructor (the targets through their vector destructor)
NiMorphData* __fastcall Morph_DeletingDestructor(NiMorphData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiMorphData_vtable);
    if (self->targets != nullptr) {
        MorphTarget_VectorDeletingDestructor(self->targets, 3);
    }
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x80D4D0: GetViewerStrings
void __fastcall Morph_GetViewerStrings(NiMorphData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiMorphData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiNumTargets", self->numTargets));
    AddViewerString(strings, GetViewerStringUint("m_uiNumVertsPerTarget", self->numVertsPerTarget));
    AddViewerString(strings, GetViewerString("MorphTargets"));
    for (uint32_t i = 0; i < self->numTargets; i++) {
        const MorphTarget* target = i < self->numTargets ? &self->targets[i] : nullptr;
        AddViewerStringInline(strings, GetViewerStringString("m_pcName", target->name));
    }
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_MORPH_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC30_MORPH_REVERSED("NiMorphData::CreateObject", 0x0080CB80, &ni::anim::Morph_CreateObject);
NIC30_MORPH_REVERSED("NiMorphData::GetRTTI", 0x0080CBC0, &ni::anim::Morph_GetRTTI);
NIC30_MORPH_REVERSED("NiMorphData::RegisterStreamables", 0x0080CBD0, &ni::anim::Morph_RegisterStreamables);
NIC30_MORPH_REVERSED("NiFloatInterpolator::ReplaceKeys", 0x0080CC30, &ni::anim::FloatInterp_ReplaceKeys);
NIC30_MORPH_REVERSED("NiMorphData::MorphTarget::MorphTarget", 0x0080CCB0, &ni::anim::Target_Construct);
NIC30_MORPH_REVERSED("NiMorphData::MorphTarget::~MorphTarget", 0x0080CCD0, &ni::anim::Target_Destruct);
NIC30_MORPH_REVERSED("NiMorphData::MorphTarget::IsEqual", 0x0080CD30, &ni::anim::Target_IsEqual);
NIC30_MORPH_REVERSED("NiMorphData::MorphTarget::VectorDeletingDestructor", 0x0080CDE0,
                     &ni::anim::Target_VectorDeletingDestructor);
NIC30_MORPH_REVERSED("NiMorphData::IsEqual", 0x0080CEB0, &ni::anim::Morph_IsEqual);
NIC30_MORPH_REVERSED("NiMorphData::MorphTarget::LoadBinary", 0x0080CF50, &ni::anim::Target_LoadBinary);
NIC30_MORPH_REVERSED("NiMorphData::LoadBinary", 0x0080D010, &ni::anim::Morph_LoadBinary);
NIC30_MORPH_REVERSED("NiMorphData::SaveBinary", 0x0080D380, &ni::anim::Morph_SaveBinary);
NIC30_MORPH_REVERSED("NiMorphData::DeletingDestructor", 0x0080D490, &ni::anim::Morph_DeletingDestructor);
NIC30_MORPH_REVERSED("NiMorphData::GetViewerStrings", 0x0080D4D0, &ni::anim::Morph_GetViewerStrings);
