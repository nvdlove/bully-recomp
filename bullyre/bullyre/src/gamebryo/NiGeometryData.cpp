// NiGeometryData, NiLinesData and NiTriShapeData (src/gamebryo/NiGeometryData.h): construction
// (the ID under its lock, the bound from the vertices), the arrays replaced, streaming (the
// arrays from the object's group when it has one), comparison, the vertex ordering used to weld
// (a quicksort of indices by position, normal, color, texture coordinates), viewer strings. And
// the triangle list and shared normals of NiTriShapeData.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometryData.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiGeometryData_RTTI, 0x020E30A8);
GAME_VAR(RTTI, NiLinesData_RTTI, 0x020E3074);
GAME_VAR(RTTI, NiTriShapeData_RTTI, 0x020E3130);

constexpr uintptr_t NiGeometryData_vtable = 0x009539E0;
constexpr uintptr_t NiLinesData_vtable = 0x00953970;
constexpr uintptr_t NiTriShapeData_vtable = 0x00953BC8;
constexpr uintptr_t ColorA_Construct = 0x005EFB40;          // NiColorA's constructor
constexpr uintptr_t SharedNormalArray_Construct = 0x0077BC50;

GAME_VAR(ni::sys::NiCriticalSection, NiGeometryData_ms_kCritSec, 0x020E30DC);
GAME_VAR(uint16_t, NiGeometryData_ms_usNextID, 0x00BC8ACC);

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count, void* constructor);
GAME_FN(void, __cdecl, Renderer_PurgeGeometryData, 0x00755680, void* data);
GAME_FN(NiObject*, __thiscall, Stream_GetGroupFromID, 0x00759DD0, void* stream, uint32_t id);
GAME_FN(uint32_t, __thiscall, Stream_GetIDFromGroup, 0x00759DE0, void* stream, NiObject* group);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const void* self, const void* other);
GAME_FN(uint8_t, __thiscall, ColorA_NotEqual, 0x00769390, const void* self, const void* other);
GAME_FN(void, __thiscall, ObjectGroup_Destruct, 0x007D1CE0, NiObject* self);
GAME_FN(void, __thiscall, Bound_LoadBinary, 0x007D23D0, NiBound* self, void* stream);
GAME_FN(void, __thiscall, Bound_SaveBinary, 0x007D2410, NiBound* self, void* stream);
GAME_FN(uint8_t, __thiscall, Bound_Equals, 0x007D2840, const NiBound* self, const NiBound* other);
GAME_FN(void, __thiscall, Bound_ComputeFromData, 0x007D2890, NiBound* self, uint32_t count, const NiPoint3* points);
GAME_FN(uint32_t, __thiscall, Object_GetBlockAllocationSize, 0x00824EC0, const void* self);
GAME_FN(void, __thiscall, Object_LinkObjectNone, 0x008A20A0, void* self, void* stream);   // NiObject's empty ones
GAME_FN(char*, __cdecl, GetViewerStringByte, 0x00785600, const char* prefix, uint32_t value);
GAME_FN(char*, __cdecl, GetViewerStringShort, 0x00785650, const char* prefix, uint32_t value);
GAME_FN(char*, __cdecl, GetViewerStringInt, 0x007856A0, const char* prefix, int value);
// NiTriBasedGeomData's (another chunk's)
GAME_FN(void, __thiscall, TriBasedGeomData_Construct, 0x007706F0, void* self);
GAME_FN(void, __thiscall, TriBasedGeomData_Destruct, 0x00770710, void* self);
GAME_FN(uint8_t, __thiscall, TriBasedGeomData_IsEqual, 0x00770720, void* self, void* other);
GAME_FN(void, __thiscall, TriBasedGeomData_LoadBinary, 0x007707C0, void* self, void* stream);
GAME_FN(void, __thiscall, TriBasedGeomData_SaveBinary, 0x00770810, void* self, void* stream);
GAME_FN(void, __thiscall, TriBasedGeomData_GetViewerStrings, 0x00770850, void* self, NiTArray<char*>* strings);
// (this file's, called where modes apply)
GAME_FN(int, __thiscall, GeometryData_CompareVertices, 0x0077A250, NiGeometryData* self, uint32_t first,
        uint32_t second, uint32_t flags);
GAME_FN(NiGeometryData*, __thiscall, GeometryData_Construct, 0x0077A4A0, NiGeometryData* self, uint32_t count,
        NiPoint3* vertices, NiPoint3* normals, float* colors, float* textures, uint32_t textureSets,
        uint32_t nbtMethod);
GAME_FN(NiGeometryData*, __thiscall, GeometryData_ConstructDefault, 0x0077A5A0, NiGeometryData* self);
GAME_FN(uint16_t, __thiscall, GeometryData_ChoosePivot, 0x0077A7D0, NiGeometryData* self, uint16_t* indices,
        uint32_t flags, int low, int high);
GAME_FN(void, __thiscall, GeometryData_LoadBinary, 0x0077A880, NiGeometryData* self, void* stream);
GAME_FN(void, __thiscall, GeometryData_SaveBinary, 0x0077ACF0, NiGeometryData* self, void* stream);
GAME_FN(uint8_t, __thiscall, GeometryData_IsEqual, 0x0077AFD0, NiGeometryData* self, NiGeometryData* other);
GAME_FN(void, __thiscall, GeometryData_Destruct, 0x0077B230, NiGeometryData* self);
GAME_FN(void, __thiscall, GeometryData_SortVertices, 0x0077B350, NiGeometryData* self, uint16_t* indices,
        uint32_t flags, int low, int high);
GAME_FN(void, __thiscall, GeometryData_GetViewerStrings, 0x0077B4E0, NiGeometryData* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, SharedNormalPool_Destruct, 0x0077BD10, NiSharedNormalPool* self);
GAME_FN(void*, __thiscall, SharedNormalPool_DeletingDestructor, 0x0077BC60, NiSharedNormalPool* self, uint32_t flags);

// a binary stream read or write of `bytes` in one component of `componentSize` (the array
// reads: swapped per element when the endianness differs)
inline void Read(void* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t size = componentSize;
    in->position += in->read(in, buffer, bytes, &size, 1);
}
inline void Write(void* stream, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t size = componentSize;
    out->position += out->write(out, buffer, bytes, &size, 1);
}

// NiNew[]'s size for count elements (-1 when it overflows)
inline uint32_t ArraySize(uint32_t count, uint32_t size) {
    uint64_t product = uint64_t(count) * size;
    return (product >> 32) != 0 ? 0xFFFFFFFF : uint32_t(product);
}

// slot 16 GetGroup
inline NiObject* GetGroup(NiGeometryData* self) {
    return Virtual<NiObject*>(self, 16);
}

// an array taken from the group (its cursor moved on, one more user)
inline void* TakeFromGroup(NiGeometryData* self, uint32_t bytes) {
    NiObject* group = GetGroup(self);
    uint8_t* p = At<uint8_t*>(group, 8);
    At<uint32_t>(group, 0xC) += 1;
    At<uint8_t*>(group, 8) = p + bytes;
    return p;
}

// one user of a group gone (the group deleted with its last)
inline void ReleaseGroup(NiObject* group) {
    if (--At<int32_t>(group, 0xC) == 0) {
        ObjectGroup_Destruct(group);
        NiDelete(group, 0x10);
    }
}

// the ID taken under the lock
inline void TakeID(NiGeometryData* self) {
    ni::sys::Lock(NiGeometryData_ms_kCritSec);
    self->id = NiGeometryData_ms_usNextID;
    NiGeometryData_ms_usNextID++;
    ni::sys::Unlock(NiGeometryData_ms_kCritSec);
}

// a reference slot assigned as the streaming code does it (the old let go first)
inline void AssignReleasingFirst(re::Ptr32<NiObject>& slot, NiObject* value) {
    NiObject* old = slot;
    if (old != value) {
        Release(old);
        slot = value;
        AddRef(value);
    }
}

// ---- comparisons

// the arrays compared component by component: -1 when the first is greater, 1 when it's less
// (NaN: equal). Its arguments are in ecx, eax and edi: unhooked, called directly
int CompareFloats(const float* first, const float* second, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (first[i] > second[i]) {
            return -1;
        }
        if (first[i] < second[i]) {
            return 1;
        }
    }
    return 0;
}

// NiPoint2::operator!=
uint8_t __fastcall Point2NotEqual(const float* self, re::Edx, const float* other) {
    if (other[0] == self[0] && other[1] == self[1]) {
        return 0;
    }
    return 1;
}

// ---- NiGeometryData

// GetTextureSet(set): the set's coordinates (none past the sets)
float* __fastcall GetTextureSet(NiGeometryData* self, re::Edx, uint16_t set) {
    if (self->textures != nullptr && set < (self->dataFlags & 0x3F)) {
        return self->textures + uint32_t(self->vertexCount) * set * 2;
    }
    return nullptr;
}

// SetData(count, vertices, normals, colors, textures, sets, nbtMethod): the arrays taken over
// (the old ones kept), the bound made from the vertices
void __fastcall SetData(NiGeometryData* self, re::Edx, uint32_t count, NiPoint3* vertices, NiPoint3* normals,
                        float* colors, float* textures, uint32_t textureSets, uint32_t nbtMethod) {
    Virtual<void>(self, 21, count);   // SetActiveVertexCount
    self->normals = normals;
    self->vertices = vertices;
    if (self->activeVertexCount != 0 && vertices != nullptr) {
        Bound_ComputeFromData(&self->bound, self->activeVertexCount, vertices);
    }
    self->colors = colors;
    self->textures = textures;
    self->dataFlags = uint16_t((self->dataFlags & 0xFC0) | (uint8_t(textureSets) & 0x3F) | uint16_t(nbtMethod));
}

// vf19 ContainsVertexData(parameter): the renderer's data first
uint8_t __fastcall ContainsVertexData(NiGeometryData* self, re::Edx, uint32_t parameter) {
    if (self->rendererData != nullptr && Virtual<uint8_t>(self->rendererData, 1, parameter)) {
        return 1;
    }
    switch (parameter) {
    case 0:
        return self->vertices != nullptr ? 1 : 0;
    case 3:
        return self->normals != nullptr ? 1 : 0;
    case 4:
        return self->colors != nullptr ? 1 : 0;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        return self->textures != nullptr && uint8_t(self->dataFlags & 0x3F) >= parameter - 4 ? 1 : 0;
    case 13:
    case 14:
        return self->normals != nullptr && (self->dataFlags & 0xF000) != 0 ? 1 : 0;
    default:
        return 0;
    }
}

// CompareVertices(first, second, flags): -1 / 0 / 1 by position (flags 1), normal (2), color (4),
// texture coordinates (8), in that order
int __fastcall CompareVertices(NiGeometryData* self, re::Edx, uint32_t first, uint32_t second, uint32_t flags) {
    uint16_t a = uint16_t(first);
    uint16_t b = uint16_t(second);
    if ((uint8_t(flags) & 1) != 0) {
        int result = CompareFloats(&self->vertices[b].x, &self->vertices[a].x, 3);
        if (result != 0) {
            return result;
        }
    }
    if (self->normals != nullptr && (uint8_t(flags) & 2) != 0) {
        int result = CompareFloats(&self->normals[b].x, &self->normals[a].x, 3);
        if (result != 0) {
            return result;
        }
    }
    if (self->colors != nullptr && (uint8_t(flags) & 4) != 0) {
        int result = CompareFloats(self->colors + b * 4, self->colors + a * 4, 4);
        if (result != 0) {
            return result;
        }
    }
    float* textures = self->textures;
    if (textures != nullptr && (uint8_t(flags) & 8) != 0) {
        uint32_t count = self->vertexCount;
        uint16_t sets = uint16_t(self->dataFlags & 0x3F);
        uint32_t offset = 0;
        for (uint16_t set = 0; set < sets; set++) {
            uint32_t base = uint16_t(offset);
            int result = CompareFloats(textures + (base + b) * 2, textures + (base + a) * 2, 2);
            if (result != 0) {
                return result;
            }
            offset += count;
        }
    }
    return 0;
}

// vf6 RegisterStreamables: the additional data too
uint8_t __fastcall RegisterStreamables(NiGeometryData* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->additionalData != nullptr && !Virtual<uint8_t>(self->additionalData, 6, stream)) {
        return 0;
    }
    return 1;
}

// vf15 GetBlockAllocationSize: the arrays' bytes
uint32_t __fastcall GetBlockAllocationSize(NiGeometryData* self, re::Edx) {
    uint32_t size = self->vertices != nullptr ? uint32_t(self->vertexCount) * 0xC : 0;
    if (self->normals != nullptr) {
        uint32_t factor = (self->dataFlags & 0xF000) != 0 ? 3 : 1;
        size += factor * self->vertexCount * 0xC;
    }
    if (self->colors != nullptr) {
        size += uint32_t(self->vertexCount) * 0x10;
    }
    if (self->textures != nullptr) {
        size += (uint8_t(self->dataFlags) & 0x3F) * uint32_t(self->vertexCount) * 8;
    }
    return Object_GetBlockAllocationSize(self) + size;
}

// NiGeometryData(count, vertices, normals, colors, textures, sets, nbtMethod)
NiGeometryData* __fastcall Construct(NiGeometryData* self, re::Edx, uint32_t count, NiPoint3* vertices,
                                     NiPoint3* normals, float* colors, float* textures, uint32_t textureSets,
                                     uint32_t nbtMethod) {
    RefObject_Construct(self);
    self->vertices = vertices;
    self->vertexCount = uint16_t(count);
    self->activeVertexCount = uint16_t(count);
    self->normals = normals;
    self->colors = colors;
    self->dataFlags = 0;
    self->vtable = reinterpret_cast<void*>(NiGeometryData_vtable);
    self->group = nullptr;
    self->textures = textures;
    self->dirtyFlags = 0;
    self->keepFlags = 0;
    self->compressFlags = 0;
    self->additionalData = nullptr;
    self->rendererData = nullptr;
    self->dataFlags = uint16_t((self->dataFlags & 0xFC0) | (uint8_t(textureSets) & 0x3F) | uint16_t(nbtMethod));
    if (self->activeVertexCount != 0 && self->vertices != nullptr) {
        Bound_ComputeFromData(&self->bound, self->activeVertexCount, self->vertices);
    }
    TakeID(self);
    self->dirtyFlags &= 0x0FFF;
    return self;
}

const RTTI* __fastcall GetRTTI(NiGeometryData*, re::Edx) {
    return &NiGeometryData_RTTI;
}

NiGeometryData* __fastcall ConstructDefault(NiGeometryData* self, re::Edx) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiGeometryData_vtable);
    self->group = nullptr;
    self->vertexCount = 0;
    self->vertices = nullptr;
    self->normals = nullptr;
    self->colors = nullptr;
    self->textures = nullptr;
    self->activeVertexCount = 0;
    self->dataFlags = 0;
    self->dirtyFlags = 0;
    self->keepFlags = 0;
    self->compressFlags = 0;
    self->additionalData = nullptr;
    self->rendererData = nullptr;
    TakeID(self);
    self->dirtyFlags &= 0x0FFF;
    return self;
}

// Replace(count, vertices, normals, colors, textures, sets, nbtMethod): the arrays that change
// freed (or their group's use given up), then SetData's work
void __fastcall Replace(NiGeometryData* self, re::Edx, uint32_t count, NiPoint3* vertices, NiPoint3* normals,
                        float* colors, float* textures, uint32_t textureSets, uint32_t nbtMethod) {
    if (GetGroup(self) == nullptr) {
        if (vertices != self->vertices) {
            NiDeleteArray(self->vertices, 0xC);
        }
        if (normals != self->normals) {
            NiDeleteArray(self->normals, 0xC);
        }
        if (colors != self->colors) {
            NiDeleteArray(self->colors, 0x10);
        }
        if (textures != self->textures) {
            NiDeleteArray(self->textures, 8);
        }
    } else {
        if (vertices != self->vertices) {
            ReleaseGroup(GetGroup(self));
        }
        if (normals != self->normals) {
            ReleaseGroup(GetGroup(self));
        }
        if (colors != self->colors) {
            ReleaseGroup(GetGroup(self));
        }
        if (textures != self->textures) {
            ReleaseGroup(GetGroup(self));
        }
    }
    Virtual<void>(self, 21, count);   // SetActiveVertexCount
    self->normals = normals;
    self->vertices = vertices;
    if (self->activeVertexCount != 0 && vertices != nullptr) {
        Bound_ComputeFromData(&self->bound, self->activeVertexCount, vertices);
    }
    self->textures = textures;
    self->colors = colors;
    self->dataFlags = uint16_t((self->dataFlags & 0xFC0) | (uint8_t(textureSets) & 0x3F) | uint16_t(nbtMethod));
}

// the quicksort's pivot: the median of the first, middle and last vertices
uint16_t __fastcall ChoosePivot(NiGeometryData* self, re::Edx, uint16_t* indices, uint32_t flags, int low, int high) {
    uint16_t middle = indices[(high + low) >> 1];
    uint16_t first = indices[low];
    if (GeometryData_CompareVertices(self, first, middle, flags) < 0) {
        uint16_t last = indices[high];
        if (GeometryData_CompareVertices(self, middle, last, flags) < 0) {
            return middle;
        }
        return GeometryData_CompareVertices(self, first, last, flags) < 0 ? last : first;
    }
    uint16_t last = indices[high];
    if (GeometryData_CompareVertices(self, first, last, flags) < 0) {
        return first;
    }
    return GeometryData_CompareVertices(self, middle, last, flags) < 0 ? last : middle;
}

// the vertex indices sorted (Hoare's partition, the smaller half recursed into... as the game
// has it: the left part recursed into unless the right end was the split)
void __fastcall SortVertices(NiGeometryData* self, re::Edx, uint16_t* indices, uint32_t flags, int low, int high) {
    while (low < high) {
        int i = low - 1;
        int j = high + 1;
        uint32_t pivot = GeometryData_ChoosePivot(self, indices, flags, low, high);
        for (;;) {
            do {
                j--;
            } while (GeometryData_CompareVertices(self, pivot, indices[j], flags) < 0);
            do {
                i++;
            } while (GeometryData_CompareVertices(self, indices[i], pivot, flags) < 0);
            if (i >= j) {
                break;
            }
            uint16_t swap = indices[i];
            indices[i] = indices[j];
            indices[j] = swap;
        }
        if (j == high) {
            high = j - 1;
        } else {
            GeometryData_SortVertices(self, indices, flags, low, j);
            low = j + 1;
        }
    }
}

// the vertices' order by CompareVertices (order[i]: the i-th smallest) and its inverse
void __fastcall ComputeVertexOrder(NiGeometryData* self, re::Edx, re::Ptr32<uint16_t>* order, re::Ptr32<uint16_t>* inverse,
                                   uint32_t flags) {
    uint16_t count = self->vertexCount;
    *order = static_cast<uint16_t*>(NiMalloc(uint32_t(count) * 2));
    *inverse = static_cast<uint16_t*>(NiMalloc(uint32_t(count) * 2));
    for (uint16_t i = 0; i < count; i++) {
        (*order)[i] = i;
    }
    GeometryData_SortVertices(self, *order, flags, 0, int(count) - 1);
    for (uint16_t i = 0; i < count; i++) {
        (*inverse)[(*order)[i]] = i;
    }
}

// vf4 LoadBinary
void __fastcall LoadBinary(NiGeometryData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    if (StreamVersion(stream) >= 0x0A010072) {
        uint32_t groupID;
        Read(stream, &groupID, 4, 4);
        self->group = Stream_GetGroupFromID(stream, groupID);
    }
    Read(stream, &self->vertexCount, 2, 2);
    self->activeVertexCount = self->vertexCount;
    Read(stream, &self->keepFlags, 1, 1);
    Read(stream, &self->compressFlags, 1, 1);
    uint8_t hasVertices = 0;
    Read(stream, &hasVertices, 1, 1);
    if (hasVertices != 0) {
        if (GetGroup(self) != nullptr) {
            self->vertices = static_cast<NiPoint3*>(TakeFromGroup(self, uint32_t(self->vertexCount) * 0xC));
        } else {
            self->vertices = static_cast<NiPoint3*>(NiNewArray(ArraySize(self->vertexCount, 0xC)));
        }
        Read(stream, self->vertices, uint32_t(self->vertexCount) * 0xC, 4);
    }
    uint32_t normalCount = self->vertexCount;
    Read(stream, &self->dataFlags, 2, 2);
    if ((self->dataFlags & 0xF000) != 0) {
        normalCount *= 3;
    }
    uint8_t hasNormals = 0;
    Read(stream, &hasNormals, 1, 1);
    if (hasNormals != 0) {
        if (GetGroup(self) != nullptr) {
            self->normals = static_cast<NiPoint3*>(TakeFromGroup(self, normalCount * 0xC));
        } else {
            self->normals = static_cast<NiPoint3*>(NiNewArray(ArraySize(normalCount, 0xC)));
        }
        Read(stream, self->normals, normalCount * 0xC, 4);
    }
    Bound_LoadBinary(&self->bound, stream);
    uint8_t hasColors = 0;
    Read(stream, &hasColors, 1, 1);
    if (hasColors != 0) {
        if (GetGroup(self) != nullptr) {
            self->colors = static_cast<float*>(TakeFromGroup(self, uint32_t(self->vertexCount) << 4));
        } else {
            uint32_t count = self->vertexCount;
            void* colors = NiNewArray(ArraySize(count, 0x10));
            if (colors != nullptr) {
                VectorConstructor(colors, 0x10, count, reinterpret_cast<void*>(ColorA_Construct));
            } else {
                colors = nullptr;
            }
            self->colors = static_cast<float*>(colors);
        }
        Read(stream, self->colors, uint32_t(self->vertexCount) << 4, 4);
    }
    if ((self->dataFlags & 0x3F) != 0) {
        uint32_t count = (self->dataFlags & 0x3F) * uint32_t(self->vertexCount);
        if (GetGroup(self) != nullptr) {
            self->textures = static_cast<float*>(TakeFromGroup(self, count * 8));
        } else {
            self->textures = static_cast<float*>(NiNewArray(ArraySize(count, 8)));
        }
        Read(stream, self->textures, count * 8, 4);
    }
    Read(stream, &self->dirtyFlags, 2, 2);
    if (StreamVersion(stream) >= 0x0A030007) {
        Stream_ReadLinkID(stream);   // the additional data
    }
}

// vf5 LinkObject: the additional data from 10.3.0.7 on
void __fastcall LinkObject(NiGeometryData* self, re::Edx, void* stream) {
    Object_LinkObjectNone(self, stream);
    if (StreamVersion(stream) >= 0x0A030007) {
        NiObject* data = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        AssignReleasingFirst(self->additionalData, data);
    }
}

// vf7 SaveBinary (a missing texture array clears the set count first)
void __fastcall SaveBinary(NiGeometryData* self, re::Edx, void* stream) {
    Object_LinkObjectNone(self, stream);   // NiObject::SaveBinary: nothing
    uint32_t groupID = Stream_GetIDFromGroup(stream, self->group);
    Write(stream, &groupID, 4, 4);
    Write(stream, &self->vertexCount, 2, 2);
    Write(stream, &self->keepFlags, 1, 1);
    Write(stream, &self->compressFlags, 1, 1);
    uint8_t hasVertices = self->vertices != nullptr ? 1 : 0;
    Write(stream, &hasVertices, 1, 1);
    if (hasVertices != 0) {
        Write(stream, self->vertices, uint32_t(self->vertexCount) * 0xC, 4);
    }
    if (self->textures == nullptr) {
        self->dataFlags &= 0xFFC0;
    }
    Write(stream, &self->dataFlags, 2, 2);
    uint32_t normalCount = self->vertexCount;
    if ((self->dataFlags & 0xF000) != 0) {
        normalCount *= 3;
    }
    uint8_t hasNormals = self->normals != nullptr ? 1 : 0;
    Write(stream, &hasNormals, 1, 1);
    if (hasNormals != 0) {
        Write(stream, self->normals, normalCount * 0xC, 4);
    }
    Bound_SaveBinary(&self->bound, stream);
    uint8_t hasColors = self->colors != nullptr ? 1 : 0;
    Write(stream, &hasColors, 1, 1);
    if (hasColors != 0) {
        Write(stream, self->colors, uint32_t(self->vertexCount) << 4, 4);
    }
    uint32_t count = (uint8_t(self->dataFlags) & 0x3F) * uint32_t(self->vertexCount);
    if (count != 0 && self->textures != nullptr) {
        Write(stream, self->textures, count * 8, 4);
    }
    uint32_t consistency = self->dirtyFlags & 0xF000;
    Write(stream, &consistency, 2, 2);
    Virtual<void>(stream, 12, self->additionalData);   // SaveLinkID
}

// whether one is there and the other not
inline bool Mismatched(const void* a, const void* b) {
    return (a != nullptr) != (b != nullptr);
}

// vf8 IsEqual (the colors' presence check looks at the texture arrays for the reverse case, as
// the game's does)
uint8_t __fastcall IsEqual(NiGeometryData* self, re::Edx, NiGeometryData* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    uint16_t count = self->vertexCount;
    if (count != other->vertexCount || self->activeVertexCount != other->activeVertexCount) {
        return 0;
    }
    if (Mismatched(self->vertices, other->vertices) || self->keepFlags != other->keepFlags ||
        self->compressFlags != other->compressFlags) {
        return 0;
    }
    if (self->vertices != nullptr) {
        for (uint32_t i = 0; i < count; i++) {
            if (Point3_NotEqual(&self->vertices[i], &other->vertices[i])) {
                return 0;
            }
        }
    }
    if (Mismatched(self->normals, other->normals)) {
        return 0;
    }
    if (self->normals != nullptr) {
        uint16_t method = self->dataFlags & 0xF000;
        if (method != (other->dataFlags & 0xF000)) {
            return 0;
        }
        uint32_t normalCount = method == 0 ? self->vertexCount : uint32_t(self->vertexCount) * 3;
        for (uint32_t i = 0; i < normalCount; i++) {
            if (Point3_NotEqual(&self->normals[i], &other->normals[i])) {
                return 0;
            }
        }
    }
    if (!Bound_Equals(&self->bound, &other->bound)) {
        return 0;
    }
    if (self->colors != nullptr && other->colors == nullptr) {
        return 0;
    }
    if (self->textures == nullptr && other->textures != nullptr) {
        return 0;
    }
    if (self->colors != nullptr) {
        uint32_t colorCount = self->vertexCount;
        for (uint32_t i = 0; i < colorCount; i++) {
            if (ColorA_NotEqual(self->colors + i * 4, other->colors + i * 4)) {
                return 0;
            }
        }
    }
    if (Mismatched(self->textures, other->textures)) {
        return 0;
    }
    if (self->textures != nullptr) {
        if (((uint8_t(other->dataFlags) ^ uint8_t(self->dataFlags)) & 0x3F) != 0) {
            return 0;
        }
        uint32_t textureCount = (self->dataFlags & 0x3F) * uint32_t(self->vertexCount);
        for (uint32_t i = 0; i < textureCount; i++) {
            if (Point2NotEqual(self->textures + i * 2, nullptr, other->textures + i * 2)) {
                return 0;
            }
        }
    }
    if (Mismatched(self->additionalData, other->additionalData)) {
        return 0;
    }
    if (self->additionalData != nullptr && other->additionalData != nullptr &&
        !Virtual<uint8_t>(self->additionalData, 8, other->additionalData)) {
        return 0;
    }
    return 1;
}

// ~NiGeometryData: the renderer's data purged, the arrays freed (or the group's use given up)
void __fastcall Destruct(NiGeometryData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiGeometryData_vtable);
    Renderer_PurgeGeometryData(self);
    if (self->additionalData != nullptr) {
        Release(self->additionalData);
        self->additionalData = nullptr;
    }
    if (self->group == nullptr) {
        NiDeleteArray(self->vertices, 0xC);
        NiDeleteArray(self->normals, 0xC);
        NiDeleteArray(self->colors, 0x10);
        NiDeleteArray(self->textures, 8);
    } else {
        if (self->vertices != nullptr) {
            ReleaseGroup(self->group);
        }
        if (self->normals != nullptr) {
            ReleaseGroup(self->group);
        }
        if (self->colors != nullptr) {
            ReleaseGroup(self->group);
        }
        if (self->textures != nullptr) {
            ReleaseGroup(self->group);
        }
    }
    Release(self->additionalData);
    Object_Destruct(self);
}

NiGeometryData* __fastcall DeletingDestructor(NiGeometryData* self, re::Edx, uint32_t flags) {
    GeometryData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x40);
    }
    return self;
}

void __fastcall GetViewerStrings(NiGeometryData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiGeometryData_RTTI.name));
    AddViewerString(strings, GetViewerStringShort("m_usVertices", self->vertexCount));
    AddViewerString(strings, GetViewerStringShort("m_usActiveVertices", self->activeVertexCount));
    AddViewerString(strings, GetViewerStringPointer("m_pkVertex", self->vertices));
    AddViewerString(strings, GetViewerStringPointer("m_pkNormal", self->normals));
    AddViewerString(strings, GetViewerStringInt("NormalBinormalTangent", self->dataFlags & 0xF000));
    AddViewerString(strings, Bound_GetViewerString(&self->bound, "m_kBound"));
    AddViewerString(strings, GetViewerStringPointer("m_pkColor", self->colors));
    AddViewerString(strings, GetViewerStringShort("Num Texture Sets", uint8_t(self->dataFlags) & 0x3F));
    AddViewerString(strings, GetViewerStringPointer("m_pkTexture", self->textures));
    AddViewerString(strings, GetViewerStringShort("m_usDirtyFlags", self->dirtyFlags));
    AddViewerString(strings, GetViewerStringByte("m_ucKeepFlags", self->keepFlags));
    AddViewerString(strings, GetViewerStringByte("m_ucCompressFlags", self->compressFlags));
    if (self->additionalData != nullptr) {
        Virtual<void>(self->additionalData, 9, strings);
        return;
    }
    AddViewerString(strings, GetViewerStringInt("m_spAdditionalGeomData", 0));
}

// ---- NiLinesData

// NiLinesData(count, vertices, colors, textures, sets, nbtMethod, flags): without flags, every
// even vertex joins the next (separate segments)
NiLinesData* __fastcall LinesDataConstruct(NiLinesData* self, re::Edx, uint32_t count, NiPoint3* vertices,
                                           float* colors, float* textures, uint32_t textureSets,
                                           uint32_t nbtMethod, uint8_t* flags) {
    GeometryData_Construct(self, count, vertices, nullptr, colors, textures, textureSets, nbtMethod);
    self->vtable = reinterpret_cast<void*>(NiLinesData_vtable);
    if (flags != nullptr) {
        self->flags = flags;
        return self;
    }
    uint16_t n = uint16_t(count);
    self->flags = static_cast<uint8_t*>(NiMalloc(n));
    for (uint16_t i = 0; i < n; i++) {
        self->flags[i] = uint8_t(~i) & 1;
    }
    return self;
}

NiLinesData* __fastcall LinesDataConstructDefault(NiLinesData* self, re::Edx) {
    GeometryData_ConstructDefault(self);
    self->vtable = reinterpret_cast<void*>(NiLinesData_vtable);
    self->flags = nullptr;
    return self;
}

void __fastcall LinesDataSetFlags(NiLinesData* self, re::Edx, uint8_t* flags) {
    self->flags = flags;
}

uint8_t __fastcall LinesDataIsEqual(NiLinesData* self, re::Edx, NiLinesData* other) {
    if (!GeometryData_IsEqual(self, other)) {
        return 0;
    }
    uint8_t* flags = self->flags;
    if (Mismatched(flags, other->flags)) {
        return 0;
    }
    if (flags != nullptr) {
        uint16_t count = self->vertexCount;
        for (uint16_t i = 0; i < count; i++) {
            if (flags[i] != other->flags[i]) {
                return 0;
            }
        }
    }
    return 1;
}

const RTTI* __fastcall LinesDataGetRTTI(NiLinesData*, re::Edx) {
    return &NiLinesData_RTTI;
}

NiLinesData* __fastcall LinesDataDeletingDestructor(NiLinesData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiLinesData_vtable);
    NiFree(self->flags);
    GeometryData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x44);
    }
    return self;
}

void __fastcall LinesDataLoadBinary(NiLinesData* self, re::Edx, void* stream) {
    GeometryData_LoadBinary(self, stream);
    self->flags = static_cast<uint8_t*>(NiMalloc(self->vertexCount));
    Read(stream, self->flags, self->vertexCount, 1);
}

void __fastcall LinesDataSaveBinary(NiLinesData* self, re::Edx, void* stream) {
    GeometryData_SaveBinary(self, stream);
    Write(stream, self->flags, self->vertexCount, 1);
}

void __fastcall LinesDataGetViewerStrings(NiLinesData* self, re::Edx, NiTArray<char*>* strings) {
    GeometryData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiLinesData_RTTI.name));
    AddViewerString(strings, GetViewerStringPointer("m_pkFlags", self->flags));
}

// ---- NiTriShapeData

NiTriShapeData* __fastcall TriShapeDataConstruct(NiTriShapeData* self, re::Edx) {
    TriBasedGeomData_Construct(self);
    self->triListLength = 0;
    self->triList = nullptr;
    self->sharedNormals = nullptr;
    self->sharedNormalsArraySize = 0;
    self->sharedNormalPools = nullptr;
    self->vtable = reinterpret_cast<void*>(NiTriShapeData_vtable);
    return self;
}

// Replace(triangles, list): the old list freed when it changes (slot 25 SetTriangleCount)
void __fastcall TriShapeDataReplace(NiTriShapeData* self, re::Edx, uint32_t triangles, uint16_t* list) {
    if (list != self->triList) {
        NiFree(self->triList);
    }
    Virtual<void>(self, 25, triangles);
    self->triList = list;
    self->triListLength = uint32_t(self->triangleCount) * 3;
}

// SetData(triangles, list)
void __fastcall TriShapeDataSetData(NiTriShapeData* self, re::Edx, uint32_t triangles, uint16_t* list) {
    Virtual<void>(self, 25, triangles);
    self->triListLength = uint32_t(self->triangleCount) * 3;
    self->triList = list;
}

// vf24 GetStripData(strips, stripLengths, triList, listLength): the list as is
void __fastcall TriShapeDataGetStripData(NiTriShapeData* self, re::Edx, uint16_t* triangles, re::Ptr32<uint16_t>* stripLengths,
                                         re::Ptr32<uint16_t>* list, uint32_t* listLength) {
    *triangles = self->triangleCount;
    *stripLengths = nullptr;
    *list = self->triList;
    *listLength = uint32_t(self->triangleCount) * 3;
}

uint8_t __fastcall TriShapeDataIsEqual(NiTriShapeData* self, re::Edx, NiTriShapeData* other) {
    if (!TriBasedGeomData_IsEqual(self, other)) {
        return 0;
    }
    uint32_t length = self->triListLength;
    if (length != other->triListLength) {
        return 0;
    }
    if (self->triList == nullptr) {
        return other->triList == nullptr ? 1 : 0;
    }
    if (other->triList == nullptr) {
        return 0;
    }
    return std::memcmp(self->triList, other->triList, length * 2) == 0 ? 1 : 0;
}

// a shared normal pool and the ones after it, deleted
void* __fastcall SharedNormalPoolDeletingDestructor(NiSharedNormalPool* self, re::Edx, uint32_t flags) {
    SharedNormalPool_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x14);
    }
    return self;
}

void __fastcall SharedNormalPoolDestruct(NiSharedNormalPool* self, re::Edx) {
    NiFree(self->data);
    NiSharedNormalPool* next = self->next;
    if (next != nullptr) {
        SharedNormalPool_Destruct(next);
        NiDelete(next, 0x14);
    }
}

void __fastcall TriShapeDataDestruct(NiTriShapeData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiTriShapeData_vtable);
    NiFree(self->triList);
    NiSharedNormalPool* pools = self->sharedNormalPools;
    if (pools != nullptr) {
        SharedNormalPool_Destruct(pools);
        NiDelete(pools, 0x14);
    }
    NiDeleteArray(self->sharedNormals, 8);
    TriBasedGeomData_Destruct(self);
}

// vf23 GetTriangleIndices(triangle, a, b, c)
void __fastcall TriShapeDataGetTriangleIndices(NiTriShapeData* self, re::Edx, uint16_t triangle, uint16_t* a,
                                               uint16_t* b, uint16_t* c) {
    uint32_t i = triangle;
    *a = self->triList[i * 3];
    *b = self->triList[i * 3 + 1];
    *c = self->triList[i * 3 + 2];
}

// the deleting destructor (the destructor and the first pool's inlined)
NiTriShapeData* __fastcall TriShapeDataDeletingDestructor(NiTriShapeData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiTriShapeData_vtable);
    NiFree(self->triList);
    NiSharedNormalPool* pools = self->sharedNormalPools;
    if (pools != nullptr) {
        NiFree(pools->data);
        if (pools->next != nullptr) {
            SharedNormalPool_DeletingDestructor(pools->next, 1);
        }
        NiDelete(pools, 0x14);
    }
    NiDeleteArray(self->sharedNormals, 8);
    TriBasedGeomData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x58);
    }
    return self;
}

// a new pool of `size` indices
inline NiSharedNormalPool* NewPool(uint32_t size, uint32_t bytes) {
    NiSharedNormalPool* pool = static_cast<NiSharedNormalPool*>(NiNew(0x14));
    if (pool == nullptr) {
        return nullptr;
    }
    uint16_t* data = static_cast<uint16_t*>(NiMalloc(bytes));
    pool->data = data;
    pool->cursor = data;
    pool->size = size;
    pool->free = size;
    pool->next = nullptr;
    return pool;
}

// vf4: the triangle list, then the shared normals (each one's indices from the first pool with
// room; a new pool twice the newest's size when none has)
void __fastcall TriShapeDataLoadBinary(NiTriShapeData* self, re::Edx, void* stream) {
    TriBasedGeomData_LoadBinary(self, stream);
    Read(stream, &self->triListLength, 4, 4);
    uint8_t hasList = 0;
    Read(stream, &hasList, 1, 1);
    if (hasList != 0 && self->triListLength != 0) {
        self->triList = static_cast<uint16_t*>(NiMalloc(self->triListLength * 2));
        Read(stream, self->triList, self->triListLength * 2, 2);
    }
    Read(stream, &self->sharedNormalsArraySize, 2, 2);
    uint16_t arraySize = self->sharedNormalsArraySize;
    if (arraySize == 0) {
        return;
    }
    void* arrays = NiNewArray(ArraySize(arraySize, 8));
    if (arrays != nullptr) {
        VectorConstructor(arrays, 8, arraySize, reinterpret_cast<void*>(SharedNormalArray_Construct));
    } else {
        arrays = nullptr;
    }
    self->sharedNormals = static_cast<NiSharedNormalArray*>(arrays);
    uint32_t firstSize = self->sharedNormalsArraySize;
    NiSharedNormalPool* first = static_cast<NiSharedNormalPool*>(NiNew(0x14));
    if (first != nullptr) {
        uint16_t* data = static_cast<uint16_t*>(NiMalloc(firstSize * 2));
        first->data = data;
        first->cursor = data;
        first->size = firstSize;
        first->free = firstSize;
        first->next = nullptr;
    } else {
        first = nullptr;
    }
    self->sharedNormalPools = first;
    for (uint32_t i = 0; uint16_t(i) < self->sharedNormalsArraySize; i++) {
        uint16_t* indices = nullptr;
        uint16_t count;
        Read(stream, &count, 2, 2);
        if (count != 0) {
            NiSharedNormalPool* pool = self->sharedNormalPools;
            while (pool != nullptr && count > pool->free) {
                pool = pool->next;
            }
            if (pool == nullptr) {
                uint32_t size = self->sharedNormalPools->size * 2;
                pool = NewPool(size, size * 2);
                pool->next = self->sharedNormalPools;
                self->sharedNormalPools = pool;
            }
            indices = pool->cursor;
            pool->free -= count;
            pool->cursor = indices + count;
            Read(stream, indices, uint32_t(count) * 2, 2);
        }
        NiSharedNormalArray* entry = &self->sharedNormals[uint16_t(i)];
        if (count != 0 && indices != nullptr) {
            entry->count = count;
            entry->indices = indices;
        } else {
            entry->count = 0;
            entry->indices = nullptr;
        }
    }
}

void __fastcall TriShapeDataSaveBinary(NiTriShapeData* self, re::Edx, void* stream) {
    TriBasedGeomData_SaveBinary(self, stream);
    Write(stream, &self->triListLength, 4, 4);
    uint8_t hasList = self->triList != nullptr ? 1 : 0;
    Write(stream, &hasList, 1, 1);
    if (hasList != 0) {
        Write(stream, self->triList, self->triListLength * 2, 2);
    }
    Write(stream, &self->sharedNormalsArraySize, 2, 2);
    for (uint16_t i = 0; i < self->sharedNormalsArraySize; i++) {
        uint32_t count = self->sharedNormals[i].count;
        Write(stream, &count, 2, 2);
        if (uint16_t(count) != 0) {
            Write(stream, self->sharedNormals[i].indices, uint32_t(uint16_t(count)) * 2, 2);
        }
    }
}

void __fastcall TriShapeDataGetViewerStrings(NiTriShapeData* self, re::Edx, NiTArray<char*>* strings) {
    TriBasedGeomData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTriShapeData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiTriListLength", self->triListLength));
    AddViewerString(strings, GetViewerStringPointer("m_pusTriList", self->triList));
}

} // namespace

// CompareFloats takes its arguments in ecx, eax and edi
RE_UNHOOKED("NiGeometryData::CompareFloats", 0x00779F00);
RE_UNHOOKED("NiLinesData::LinkObject", 0x00779CB0);              // jmp 0x77AC90
RE_UNHOOKED("NiTriStripsData::RegisterStreamables", 0x0077BB50);   // jmp 0x7D6210

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIGEOMDATA_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIGEOMDATA_REVERSED("NiLinesData::NiLinesData", 0x00779BD0, &ni::LinesDataConstruct);
NIGEOMDATA_REVERSED("NiLinesData::NiLinesDataDefault", 0x00779C50, &ni::LinesDataConstructDefault);
NIGEOMDATA_REVERSED("NiLinesData::SetFlags", 0x00779C70, &ni::LinesDataSetFlags);
NIGEOMDATA_REVERSED("NiLinesData::IsEqual", 0x00779CC0, &ni::LinesDataIsEqual);
NIGEOMDATA_REVERSED("NiLinesData::GetRTTI", 0x00779D40, &ni::LinesDataGetRTTI);
NIGEOMDATA_REVERSED("NiLinesData::DeletingDestructor", 0x00779D50, &ni::LinesDataDeletingDestructor);
NIGEOMDATA_REVERSED("NiLinesData::LoadBinary", 0x00779D90, &ni::LinesDataLoadBinary);
NIGEOMDATA_REVERSED("NiLinesData::SaveBinary", 0x00779DE0, &ni::LinesDataSaveBinary);
NIGEOMDATA_REVERSED("NiLinesData::GetViewerStrings", 0x00779E20, &ni::LinesDataGetViewerStrings);
NIGEOMDATA_REVERSED("NiPoint2::operator!=", 0x00779F50, &ni::Point2NotEqual);
NIGEOMDATA_REVERSED("NiGeometryData::GetTextureSet", 0x0077A000, &ni::GetTextureSet);
NIGEOMDATA_REVERSED("NiGeometryData::SetData", 0x0077A030, &ni::SetData);
NIGEOMDATA_REVERSED("NiGeometryData::ContainsVertexData", 0x0077A0A0, &ni::ContainsVertexData);
NIGEOMDATA_REVERSED("NiGeometryData::CompareVertices", 0x0077A250, &ni::CompareVertices);
NIGEOMDATA_REVERSED("NiGeometryData::RegisterStreamables", 0x0077A3F0, &ni::RegisterStreamables);
NIGEOMDATA_REVERSED("NiGeometryData::GetBlockAllocationSize", 0x0077A430, &ni::GetBlockAllocationSize);
NIGEOMDATA_REVERSED("NiGeometryData::NiGeometryData", 0x0077A4A0, &ni::Construct);
NIGEOMDATA_REVERSED("NiGeometryData::GetRTTI", 0x0077A590, &ni::GetRTTI);
NIGEOMDATA_REVERSED("NiGeometryData::NiGeometryDataDefault", 0x0077A5A0, &ni::ConstructDefault);
NIGEOMDATA_REVERSED("NiGeometryData::Replace", 0x0077A640, &ni::Replace);
NIGEOMDATA_REVERSED("NiGeometryData::ChoosePivot", 0x0077A7D0, &ni::ChoosePivot);
NIGEOMDATA_REVERSED("NiGeometryData::LoadBinary", 0x0077A880, &ni::LoadBinary);
NIGEOMDATA_REVERSED("NiGeometryData::LinkObject", 0x0077AC90, &ni::LinkObject);
NIGEOMDATA_REVERSED("NiGeometryData::SaveBinary", 0x0077ACF0, &ni::SaveBinary);
NIGEOMDATA_REVERSED("NiGeometryData::IsEqual", 0x0077AFD0, &ni::IsEqual);
NIGEOMDATA_REVERSED("NiGeometryData::~NiGeometryData", 0x0077B230, &ni::Destruct);
NIGEOMDATA_REVERSED("NiGeometryData::SortVertices", 0x0077B350, &ni::SortVertices);
NIGEOMDATA_REVERSED("NiGeometryData::DeletingDestructor", 0x0077B430, &ni::DeletingDestructor);
NIGEOMDATA_REVERSED("NiGeometryData::ComputeVertexOrder", 0x0077B450, &ni::ComputeVertexOrder);
NIGEOMDATA_REVERSED("NiGeometryData::GetViewerStrings", 0x0077B4E0, &ni::GetViewerStrings);
NIGEOMDATA_REVERSED("NiTriShapeData::NiTriShapeData", 0x0077BA40, &ni::TriShapeDataConstruct);
NIGEOMDATA_REVERSED("NiTriShapeData::Replace", 0x0077BA70, &ni::TriShapeDataReplace);
NIGEOMDATA_REVERSED("NiTriShapeData::SetData", 0x0077BAB0, &ni::TriShapeDataSetData);
NIGEOMDATA_REVERSED("NiTriShapeData::GetStripData", 0x0077BAE0, &ni::TriShapeDataGetStripData);
NIGEOMDATA_REVERSED("NiTriShapeData::IsEqual", 0x0077BB60, &ni::TriShapeDataIsEqual);
NIGEOMDATA_REVERSED("NiTriShapeData::SharedNormalPool::DeletingDestructor", 0x0077BC60,
                    &ni::SharedNormalPoolDeletingDestructor);
NIGEOMDATA_REVERSED("NiTriShapeData::~NiTriShapeData", 0x0077BC80, &ni::TriShapeDataDestruct);
NIGEOMDATA_REVERSED("NiTriShapeData::GetTriangleIndices", 0x0077BCD0, &ni::TriShapeDataGetTriangleIndices);
NIGEOMDATA_REVERSED("NiTriShapeData::SharedNormalPool::~SharedNormalPool", 0x0077BD10, &ni::SharedNormalPoolDestruct);
NIGEOMDATA_REVERSED("NiTriShapeData::DeletingDestructor", 0x0077BD40, &ni::TriShapeDataDeletingDestructor);
NIGEOMDATA_REVERSED("NiTriShapeData::LoadBinary", 0x0077BDB0, &ni::TriShapeDataLoadBinary);
NIGEOMDATA_REVERSED("NiTriShapeData::SaveBinary", 0x0077C030, &ni::TriShapeDataSaveBinary);
NIGEOMDATA_REVERSED("NiTriShapeData::GetViewerStrings", 0x0077C1B0, &ni::TriShapeDataGetViewerStrings);
