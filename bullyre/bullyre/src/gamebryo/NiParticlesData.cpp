// NiParticlesData (vtable 0x9617E8, 0x54 bytes: NiGeometryData and per particle a radius +0x40,
// a size +0x44 (both NiMalloc'd), a rotation +0x48 (NiQuaternion, NiNew[]'d), a rotation angle
// +0x4C (NiMalloc'd) and axis +0x50 (NiPoint3, NiNew[]'d); "NiAutoNormalParticlesData" loads as
// one too): factory, destructor, streaming (rotations of files before 10.3.0.5 turned into angles
// and axes), comparison, viewer strings, the active count, particle removal. And NiParticles'
// factory (src/gamebryo/NiParticles.cpp has the rest; "NiAutoNormalParticles" loads as one).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometryData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiParticlesData_RTTI, 0x020E3F50);

constexpr uintptr_t NiParticlesData_vtable = 0x009617E8;
constexpr uintptr_t NiParticles_vtable = 0x00961898;

struct NiParticlesData : NiGeometryData {
    re::Ptr32<float> radii;            // +0x40
    re::Ptr32<float> sizes;            // +0x44
    re::Ptr32<float> rotations;        // +0x48 (NiQuaternion: 4 floats each)
    re::Ptr32<float> rotationAngles;   // +0x4C
    re::Ptr32<NiPoint3> rotationAxes;  // +0x50
};
static_assert(offsetof(NiParticlesData, radii) == 0x40);
static_assert(sizeof(NiParticlesData) == 0x54);

GAME_FN(void, __thiscall, GeometryData_ConstructDefault, 0x0077A5A0, void* self);
GAME_FN(void, __thiscall, GeometryData_Destruct, 0x0077B230, void* self);
GAME_FN(void, __thiscall, GeometryData_LoadBinary, 0x0077A880, void* self, void* stream);
GAME_FN(void, __thiscall, GeometryData_SaveBinary, 0x0077ACF0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, GeometryData_IsEqual, 0x0077AFD0, void* self, void* other);
GAME_FN(void, __thiscall, GeometryData_GetViewerStrings, 0x0077B4E0, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Geometry_Construct, 0x00761CA0, void* self);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, Quaternion_LoadBinary, 0x007625D0, float* self, void* stream);   // (NiColorA's: 4 floats)
GAME_FN(void, __thiscall, Quaternion_SaveBinary, 0x00776310, const float* self, void* stream);
GAME_FN(void, __thiscall, Quaternion_ToAngleAxis, 0x00775F50, const float* self, float* angle, NiPoint3* axis);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);

inline uint8_t LoadFlag(void* stream) {
    uint8_t flag;
    LoadBytes(stream, &flag, 1);
    return flag;
}

inline void SaveFlag(void* stream, bool flag) {
    uint8_t value = flag ? 1 : 0;
    SaveBytes(stream, &value, 1);
}

// an array streamed in one read or write of `bytes`, one component of 4 bytes
inline void LoadArray(void* stream, void* buffer, uint32_t bytes) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t componentSize = 4;
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}
inline void SaveArray(void* stream, const void* buffer, uint32_t bytes) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t componentSize = 4;
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// slot 22 RemoveParticle(index): the last active particle moved into its place (each array there
// is), one fewer active
void __fastcall RemoveParticle(NiParticlesData* self, re::Edx, int index) {
    uint16_t last = uint16_t(self->activeVertexCount - 1);
    uint16_t at = uint16_t(index);
    if (at != last) {
        self->vertices[at] = self->vertices[last];
        if (self->colors != nullptr) {
            for (int c = 0; c < 4; c++) {
                self->colors[at * 4 + c] = self->colors[last * 4 + c];
            }
        }
        if (self->normals != nullptr) {
            self->normals[at] = self->normals[last];
        }
        if (self->radii != nullptr) {
            self->radii[at] = self->radii[last];
        }
        if (self->sizes != nullptr) {
            self->sizes[at] = self->sizes[last];
        }
        if (self->rotations != nullptr) {
            for (int c = 0; c < 4; c++) {
                self->rotations[at * 4 + c] = self->rotations[last * 4 + c];
            }
        }
        if (self->rotationAngles != nullptr) {
            self->rotationAngles[at] = self->rotationAngles[last];
        }
        if (self->rotationAxes != nullptr) {
            self->rotationAxes[at] = self->rotationAxes[last];
        }
    }
    self->activeVertexCount--;
}

const RTTI* __fastcall GetRTTI(void*, re::Edx) {
    return &NiParticlesData_RTTI;
}

// slot 18 SetActiveVertexCount: clamped to the vertex count
void __fastcall SetActiveVertexCount(NiParticlesData* self, re::Edx, int count) {
    uint16_t active = uint16_t(count);
    self->activeVertexCount = active <= self->vertexCount ? active : self->vertexCount;
}

NiParticlesData* __fastcall DeletingDestructor(NiParticlesData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiParticlesData_vtable);
    NiFree(self->radii);
    NiFree(self->sizes);
    NiDeleteArray(self->rotations, 0x10);
    NiFree(self->rotationAngles);
    NiDeleteArray(self->rotationAxes, 0xC);
    GeometryData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x54);
    }
    return self;
}

// the factory: no per-particle arrays, the consistency volatile (0x8000 in bits 12..15)
NiParticlesData* __cdecl CreateObject() {
    NiParticlesData* object = static_cast<NiParticlesData*>(NiNew(0x54));
    if (object == nullptr) {
        return nullptr;
    }
    GeometryData_ConstructDefault(object);
    object->radii = nullptr;
    object->sizes = nullptr;
    object->rotations = nullptr;
    object->rotationAngles = nullptr;
    object->rotationAxes = nullptr;
    object->dirtyFlags = uint16_t((object->dirtyFlags & 0xFFF) | 0x8000);
    object->vtable = reinterpret_cast<void*>(NiParticlesData_vtable);
    return object;
}

// NiGeometryData's; the radii (all 1 when absent), the active count, the sizes (all 1 when absent),
// the rotations; before 10.3.0.5 the rotations become angles and axes and are dropped, after, the
// angles and axes follow (each after its flag)
void __fastcall LoadBinary(NiParticlesData* self, re::Edx, void* stream) {
    GeometryData_LoadBinary(self, stream);
    self->radii = static_cast<float*>(NiMalloc(uint32_t(self->vertexCount) * 4));
    if (LoadFlag(stream) != 0) {
        LoadArray(stream, self->radii, uint32_t(self->vertexCount) * 4);
    } else {
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            self->radii[i] = 1.0f;
        }
    }
    LoadBytes(stream, &self->activeVertexCount, 2);
    uint8_t hasSizes = LoadFlag(stream);
    self->sizes = static_cast<float*>(NiMalloc(uint32_t(self->vertexCount) * 4));
    if (hasSizes != 0) {
        LoadArray(stream, self->sizes, uint32_t(self->vertexCount) * 4);
    } else {
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            self->sizes[i] = 1.0f;
        }
    }
    if (LoadFlag(stream) != 0) {
        self->rotations = static_cast<float*>(NiNewArray(uint32_t(self->vertexCount) * 0x10));
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            Quaternion_LoadBinary(self->rotations + i * 4, stream);
        }
    }
    if (StreamVersion(stream) < 0x0A030005) {
        if (self->rotations == nullptr) {
            return;
        }
        self->rotationAngles = static_cast<float*>(NiMalloc(uint32_t(self->vertexCount) * 4));
        self->rotationAxes = static_cast<NiPoint3*>(NiNewArray(uint32_t(self->vertexCount) * 0xC));
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            Quaternion_ToAngleAxis(self->rotations + i * 4, &self->rotationAngles[i], &self->rotationAxes[i]);
        }
        NiDeleteArray(self->rotations, 0x10);
        self->rotations = nullptr;
        return;
    }
    if (LoadFlag(stream) != 0) {
        self->rotationAngles = static_cast<float*>(NiMalloc(uint32_t(self->vertexCount) * 4));
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            LoadBytes(stream, &self->rotationAngles[i], 4);
        }
    }
    if (LoadFlag(stream) != 0) {
        self->rotationAxes = static_cast<NiPoint3*>(NiNewArray(uint32_t(self->vertexCount) * 0xC));
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            Point3_LoadBinary(&self->rotationAxes[i], stream);
        }
    }
}

void __fastcall SaveBinary(NiParticlesData* self, re::Edx, void* stream) {
    GeometryData_SaveBinary(self, stream);
    SaveFlag(stream, self->radii != nullptr);
    if (self->radii != nullptr) {
        SaveArray(stream, self->radii, uint32_t(self->vertexCount) * 4);
    }
    SaveBytes(stream, &self->activeVertexCount, 2);
    SaveFlag(stream, self->sizes != nullptr);
    if (self->sizes != nullptr) {
        SaveArray(stream, self->sizes, uint32_t(self->vertexCount) * 4);
    }
    SaveFlag(stream, self->rotations != nullptr);
    if (self->rotations != nullptr) {
        for (uint32_t i = 0; i < self->vertexCount; i++) {
            Quaternion_SaveBinary(self->rotations + i * 4, stream);
        }
    }
    SaveFlag(stream, self->rotationAngles != nullptr);
    if (self->rotationAngles != nullptr) {
        for (uint32_t i = 0; i < self->vertexCount; i++) {
            SaveBytes(stream, &self->rotationAngles[i], 4);
        }
    }
    SaveFlag(stream, self->rotationAxes != nullptr);
    if (self->rotationAxes != nullptr) {
        for (uint32_t i = 0; i < self->vertexCount; i++) {
            Point3_SaveBinary(&self->rotationAxes[i], stream);
        }
    }
}

// NiGeometryData's; the radii (all vertices), sizes and rotations (the active ones) both or
// neither and equal; the angles and axes compared when this one has them (the other's not
// checked, as the game has it)
uint8_t __fastcall IsEqual(NiParticlesData* self, re::Edx, NiParticlesData* other) {
    if (!GeometryData_IsEqual(self, other)) {
        return 0;
    }
    if ((self->radii == nullptr) != (other->radii == nullptr)) {
        return 0;
    }
    if (self->radii != nullptr) {
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            if (!(other->radii[i] == self->radii[i])) {
                return 0;
            }
        }
    }
    if ((self->sizes == nullptr) != (other->sizes == nullptr)) {
        return 0;
    }
    if (self->sizes != nullptr) {
        for (uint16_t i = 0; i < self->activeVertexCount; i++) {
            if (!(other->sizes[i] == self->sizes[i])) {
                return 0;
            }
        }
    }
    if ((self->rotations == nullptr) != (other->rotations == nullptr)) {
        return 0;
    }
    if (self->rotations != nullptr) {
        for (uint32_t i = 0; i < self->activeVertexCount; i++) {
            for (int c = 0; c < 4; c++) {
                if (!(other->rotations[i * 4 + c] == self->rotations[i * 4 + c])) {
                    return 0;
                }
            }
        }
    }
    if (self->rotationAngles != nullptr) {
        for (uint32_t i = 0; i < self->activeVertexCount; i++) {
            if (!(other->rotationAngles[i] == self->rotationAngles[i])) {
                return 0;
            }
        }
    }
    if (self->rotationAxes != nullptr) {
        for (uint32_t i = 0; i < self->activeVertexCount; i++) {
            const NiPoint3& a = self->rotationAxes[i];
            const NiPoint3& b = other->rotationAxes[i];
            if (!(b.x == a.x) || !(b.y == a.y) || !(b.z == a.z)) {
                return 0;
            }
        }
    }
    return 1;
}

void __fastcall GetViewerStrings(NiParticlesData* self, re::Edx, NiTArray<char*>* strings) {
    GeometryData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiParticlesData_RTTI.name));
    AddViewerString(strings, GetViewerStringPointer("m_pfRadii", self->radii));
    AddViewerString(strings, GetViewerStringPointer("m_pfSizes", self->sizes));
    AddViewerString(strings, GetViewerStringPointer("m_pkRotations", self->rotations));
    AddViewerString(strings, GetViewerStringPointer("m_pfRotationAngles", self->rotationAngles));
    AddViewerString(strings, GetViewerStringPointer("m_pkRotationAxes", self->rotationAxes));
}

// NiParticles' factory
void* __cdecl ParticlesCreateObject() {
    void* object = NiNew(0xEC);
    if (object == nullptr) {
        return nullptr;
    }
    Geometry_Construct(object);
    *static_cast<re::Ptr32<void>*>(object) = reinterpret_cast<void*>(NiParticles_vtable);
    return object;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPARTDATA_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIPARTDATA_REVERSED("NiParticlesData::RemoveParticle", 0x007DFD40, ni::RemoveParticle);
NIPARTDATA_REVERSED("NiParticlesData::GetRTTI", 0x007DFE60, ni::GetRTTI);
NIPARTDATA_REVERSED("NiParticlesData::SetActiveVertexCount", 0x007DFE70, ni::SetActiveVertexCount);
NIPARTDATA_REVERSED("NiParticlesData::DeletingDestructor", 0x007DFE90, ni::DeletingDestructor);
NIPARTDATA_REVERSED("NiParticlesData::CreateObject", 0x007DFEF0, ni::CreateObject);
NIPARTDATA_REVERSED("NiParticlesData::LoadBinary", 0x007DFF40, ni::LoadBinary);
NIPARTDATA_REVERSED("NiParticlesData::SaveBinary", 0x007E0290, ni::SaveBinary);
NIPARTDATA_REVERSED("NiParticlesData::IsEqual", 0x007E04D0, ni::IsEqual);
NIPARTDATA_REVERSED("NiParticlesData::GetViewerStrings", 0x007E06B0, ni::GetViewerStrings);
NIPARTDATA_REVERSED("NiParticles::CreateObject", 0x007E0830, ni::ParticlesCreateObject);
