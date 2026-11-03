// Gamebryo's screen-space classes, the slots the first passes left:
//   NiScreenGeometry     (vtable 0x960A28, 0xEC bytes): an NiTriShape of its own class
//   NiScreenGeometryData (vtable 0x960B10, 0x70 bytes): NiTriShapeData with the pixel-accurate
//                        flag and its screen elements (each: positions, colors, texture sets),
//                        the elements from a pool shared by all of them (0x20E39F0)
//   NiScreenPolygon      (vtable 0x960AC8, 0x1C bytes): an NiObject with its own property state
//                        and NiNew[]'d vertices, texture coordinates and colors
//   NiScreenSpaceCamera  (vtable 0x960BA0, 0x15C bytes): an NiCamera (orthographic) with the
//                        screen polygons and screen textures it draws (NiTObjectArrays)
// and NiWireframeProperty's factory and default property. Construction, destruction, cloning,
// streaming, comparison, viewer strings.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCamera.h"
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
GAME_VAR(RTTI, NiScreenGeometry_RTTI, 0x020E3A28);
GAME_VAR(RTTI, NiScreenPolygon_RTTI, 0x020E3A5C);
GAME_VAR(RTTI, NiScreenGeometryData_RTTI, 0x020E39F4);
GAME_VAR(RTTI, NiScreenSpaceCamera_RTTI, 0x020E3A90);

constexpr uintptr_t NiScreenGeometry_vtable = 0x00960A28;
constexpr uintptr_t NiScreenPolygon_vtable = 0x00960AC8;
constexpr uintptr_t NiScreenGeometryData_vtable = 0x00960B10;
constexpr uintptr_t NiScreenSpaceCamera_vtable = 0x00960BA0;
constexpr uintptr_t NiWireframeProperty_vtable = 0x00960C28;
constexpr uintptr_t ScreenElement_vtable = 0x00960A20;
constexpr uintptr_t ElementArray_vtable = 0x00961988;   // NiTPrimitiveArray<ScreenElement*>
constexpr uintptr_t ObjectArray_vtable = 0x009893C0;    // NiTObjectArray<NiPointer<...>>
constexpr uintptr_t ColorA_Construct = 0x005EFB40;

// NiScreenGeometryData::ScreenElement (0x14 bytes, its own vtable: the vector deleting destructor)
struct ScreenElement {
    re::Ptr32<void> vtable;            // +0x0
    uint16_t vertexCount;    // +0x4
    uint16_t textureSets;    // +0x6
    re::Ptr32<float> positions;        // +0x8 (NiPoint2, NiNew[]'d)
    re::Ptr32<float> colors;           // +0xC (NiColorA)
    re::Ptr32<float> textures;         // +0x10 (NiPoint2, textureSets of them per vertex)
};
static_assert(sizeof(ScreenElement) == 0x14);

// the element pool (0x20E39F0): the free elements (a set), the blocks they came from, the next
// block's size (8, doubled with each block)
struct PoolBlock {
    re::Ptr32<ScreenElement> elements;   // +0x0 (NiNew[]'d)
    uint32_t count;            // +0x4
    re::Ptr32<PoolBlock> next;           // +0x8
};
static_assert(sizeof(PoolBlock) == 0xC);

struct ElementPool {
    re::Ptr32<re::Ptr32<ScreenElement>> free;      // +0x0
    uint32_t allocated;        // +0x4
    uint32_t count;            // +0x8
    re::Ptr32<PoolBlock> blocks;         // +0xC
    uint32_t growBy;           // +0x10
    uint32_t initialSize;      // +0x14
};
static_assert(sizeof(ElementPool) == 0x18);

GAME_VAR(ElementPool*, ms_pkElementPool, 0x020E39F0);
GAME_VAR(NiRefObject*, NiWireframeProperty_ms_spDefault, 0x020E3AF8);

struct NiScreenGeometryData : NiTriShapeData {
    uint8_t pixelAccurate;               // +0x58
    uint8_t unknown59;                   // +0x59
    uint16_t pad5A;
    uint32_t unknown5C;                  // +0x5C
    NiTArray<ScreenElement*> elements;   // +0x60 (growBy 4)
};
static_assert(offsetof(NiScreenGeometryData, pixelAccurate) == 0x58);
static_assert(offsetof(NiScreenGeometryData, elements) == 0x60);
static_assert(sizeof(NiScreenGeometryData) == 0x70);

// NiPropertyState (0x38 bytes): its twelve properties by type
struct NiPropertyState : NiRefObject {
    re::Ptr32<NiObject> properties[12];   // +0x8
};
static_assert(sizeof(NiPropertyState) == 0x38);

struct NiScreenPolygon : NiObject {
    re::Ptr32<NiPropertyState> state;     // +0x8 (a reference)
    uint16_t vertexCount;       // +0xC
    uint16_t padE;
    re::Ptr32<NiPoint3> vertices;         // +0x10 (NiNew[]'d)
    re::Ptr32<float> textures;            // +0x14 (NiPoint2)
    re::Ptr32<float> colors;              // +0x18 (NiColorA)
};
static_assert(sizeof(NiScreenPolygon) == 0x1C);

struct NiScreenSpaceCamera : NiCamera {
    NiTArray<NiObject*> polygons;   // +0x13C (NiTObjectArray<NiScreenPolygonPtr>, growBy 5)
    NiTArray<NiObject*> textures;   // +0x14C (NiTObjectArray<NiScreenTexturePtr>, growBy 5)
};
static_assert(offsetof(NiScreenSpaceCamera, polygons) == 0x13C);
static_assert(sizeof(NiScreenSpaceCamera) == 0x15C);

// NiWireframeProperty (0x1C bytes): NiProperty's NiObjectNET and its flags
struct NiWireframeProperty : NiObjectNET {
    uint16_t flags;   // +0x18
    uint16_t pad1A;
};
static_assert(sizeof(NiWireframeProperty) == 0x1C);

GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, ElementPool_Destruct, 0x00800050, ElementPool* self);   // (the pools' shared destructor)
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);   // (operator new: a zero-length new[])
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __thiscall, ElementSet_Realloc, 0x0083AD20, void* set, uint32_t allocated);
// NiTPrimitiveSet<ScreenElement*>::AddUnique (the pool taking an element back)
GAME_FN(void, __thiscall, ElementSet_AddUnique, 0x00884380, void* set, const re::Ptr32<ScreenElement>* element);
GAME_FN(void, __thiscall, ElementArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, ElementArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<ScreenElement>* value);
// NiTObjectArray<NiPointer<...>>: SetSize (allocated), SetAt (the reference held), RemoveAll
GAME_FN(void*, __cdecl, NiNewPointerArray, 0x00856120, uint32_t count);
GAME_FN(void, __thiscall, ObjectArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, ObjectArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<ni::NiObject>* value);
GAME_FN(void, __thiscall, ObjectArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __cdecl, ObjectArray_DeleteArray, 0x00836F80, void* base);

GAME_FN(void, __thiscall, Point2_LoadBinary, 0x007D39F0, float* self, void* stream);
GAME_FN(void, __thiscall, Point2_SaveBinary, 0x007D3A50, const float* self, void* stream);
GAME_FN(uint8_t, __thiscall, Point2_NotEqual, 0x00779F50, const float* self, const float* other);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, ColorA_LoadBinary, 0x007625D0, float* self, void* stream);
GAME_FN(void, __thiscall, ColorA_SaveBinary, 0x00762670, const float* self, void* stream);
GAME_FN(uint8_t, __thiscall, ColorA_NotEqual, 0x00769390, const float* self, const float* other);
GAME_FN(char*, __cdecl, GetViewerStringShort, 0x00785650, const char* prefix, uint32_t value);
GAME_FN(void, __thiscall, Empty_8A20A0, 0x008A20A0, void* self, void* stream);   // (an empty base slot)

GAME_FN(void, __thiscall, TriShape_Construct, 0x0077B880, void* self);
GAME_FN(void, __thiscall, TriBasedGeom_Destruct, 0x007793C0, void* self);
GAME_FN(void, __thiscall, TriShape_CopyMembers, 0x0077B8A0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, TriShape_GetViewerStrings, 0x0077B9F0, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, TriShapeData_Construct, 0x0077BA40, void* self);
GAME_FN(void, __thiscall, TriShapeData_Destruct, 0x0077BC80, void* self);
GAME_FN(void, __thiscall, TriShapeData_LoadBinary, 0x0077BDB0, void* self, void* stream);
GAME_FN(void, __thiscall, TriShapeData_SaveBinary, 0x0077C030, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TriShapeData_IsEqual, 0x0077BB60, void* self, void* other);
GAME_FN(void, __thiscall, GeometryData_GetViewerStrings, 0x0077B4E0, void* self, NiTArray<char*>* strings);
GAME_FN(NiPropertyState*, __thiscall, PropertyState_ConstructCopy, 0x007D1FB0, void* self, NiPropertyState* other);
GAME_FN(void, __thiscall, Camera_Construct, 0x00763C20, void* self);
GAME_FN(void, __thiscall, Camera_Destruct, 0x00762D50, void* self);
GAME_FN(void, __thiscall, Camera_CopyMembers, 0x00763580, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Camera_LinkObject, 0x00763880, void* self, void* stream);
GAME_FN(void, __thiscall, Camera_GetViewerStrings, 0x00763D70, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Camera_UpdateWorldData, 0x00763650, void* self);
GAME_FN(void, __thiscall, Camera_UpdateWorldBound, 0x007631B0, void* self);

// (this file's, called where modes apply)
GAME_FN(void, __thiscall, ScreenElement_Construct, 0x007D61F0, ScreenElement* self);
GAME_FN(ScreenElement*, __cdecl, ScreenElement_NewArray, 0x007D64F0, uint32_t count);
GAME_FN(void, __thiscall, ScreenPolygon_Destruct, 0x007D6350, NiScreenPolygon* self);
GAME_FN(void, __thiscall, ScreenPolygon_CopyMembers, 0x007D63C0, NiScreenPolygon* self, NiScreenPolygon* dest,
        void* cloning);
GAME_FN(void, __thiscall, ScreenGeometryData_Construct, 0x007D7040, void* self);
GAME_FN(void, __thiscall, ElementPool_CreateNewBlock, 0x007D70C0, ElementPool* self, uint32_t count);
GAME_FN(void, __thiscall, ScreenGeometryData_Destruct, 0x007D7170, NiScreenGeometryData* self);
GAME_FN(void, __thiscall, ScreenSpaceCamera_Destruct, 0x007D7570, NiScreenSpaceCamera* self);
GAME_FN(void, __thiscall, ScreenSpaceCamera_CopyMembers, 0x007D7640, NiScreenSpaceCamera* self,
        NiScreenSpaceCamera* dest, void* cloning);
GAME_FN(NiScreenSpaceCamera*, __thiscall, ScreenSpaceCamera_Construct, 0x007D7C00, void* self);

// a reference slot assigned as the library inlines it here: the old one let go, then the new one
// held (both through the imports)
template <typename T>
inline void SetReference(re::Ptr32<T>& slot, std::type_identity_t<T>* value) {
    T* old = slot;
    if (old == value) {
        return;
    }
    if (old != nullptr && ni::sys::Decrement(&old->refCount) == 0) {
        Virtual<void>(old, 1);
    }
    slot = value;
    if (value != nullptr) {
        ni::sys::Increment(&value->refCount);
    }
}

inline uint8_t LoadByte(void* stream) {
    uint8_t value;
    LoadBytes(stream, &value, 1);
    return value;
}

inline void SaveByte(void* stream, uint8_t value) {
    SaveBytes(stream, &value, 1);
}

// an array read in one read of `bytes`, one component of `componentSize`
inline void LoadArray(void* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}

// ---- NiScreenGeometry

const RTTI* __fastcall ScreenGeometryGetRTTI(void*, re::Edx) {
    return &NiScreenGeometry_RTTI;
}

void* __cdecl ScreenGeometryCreateObject() {
    void* object = NiNew(0xEC);
    if (object == nullptr) {
        return nullptr;
    }
    TriShape_Construct(object);
    *static_cast<re::Ptr32<void>*>(object) = reinterpret_cast<void*>(NiScreenGeometry_vtable);
    return object;
}

void* __fastcall ScreenGeometryDeletingDestructor(void* self, re::Edx, uint32_t flags) {
    *static_cast<re::Ptr32<void>*>(self) = reinterpret_cast<void*>(NiScreenGeometry_vtable);
    TriBasedGeom_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xEC);
    }
    return self;
}

// vf3 CreateClone (copied into a null clone when the allocation fails, as the game has it)
void* __fastcall ScreenGeometryCreateClone(void* self, re::Edx, void* cloning) {
    void* clone = NiNew(0xEC);
    if (clone != nullptr) {
        TriShape_Construct(clone);
        *static_cast<re::Ptr32<void>*>(clone) = reinterpret_cast<void*>(NiScreenGeometry_vtable);
    }
    TriShape_CopyMembers(self, clone, cloning);
    return clone;
}

void __fastcall ScreenGeometryGetViewerStrings(void* self, re::Edx, NiTArray<char*>* strings) {
    TriShape_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiScreenGeometry_RTTI.name));
}

// ---- NiScreenGeometryData's elements and their pool

// ScreenElement(): all empty
ScreenElement* __fastcall ScreenElementConstruct(ScreenElement* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(ScreenElement_vtable);
    self->vertexCount = 0;
    self->textureSets = 0;
    self->positions = nullptr;
    self->colors = nullptr;
    self->textures = nullptr;
    return self;
}

// new ScreenElement[count] (NiNew[]: the count before the elements; none: the plain operator new
// of the count alone)
ScreenElement* __cdecl ScreenElementNewArray(uint32_t count) {
    uint32_t* block;
    if (count == 0) {
        block = static_cast<uint32_t*>(Malloc(4));
    } else {
        uint64_t bytes = uint64_t(count) * 0x14;
        uint32_t size = (bytes >> 32) != 0 ? 0xFFFFFFFF : uint32_t(bytes);
        size = size > 0xFFFFFFFB ? 0xFFFFFFFF : size + 4;
        block = static_cast<uint32_t*>(NiNewArray(size));
    }
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, 0x14, count, reinterpret_cast<void*>(0x007D61F0));
    return reinterpret_cast<ScreenElement*>(block + 1);
}

inline void ScreenElementDestruct(ScreenElement* element) {
    element->vtable = reinterpret_cast<void*>(ScreenElement_vtable);
    NiDeleteArray(element->positions, 8);
    NiDeleteArray(element->colors, 0x10);
    NiDeleteArray(element->textures, 8);
}

// the vector deleting destructor (2: an array, its count before it)
void* __fastcall ScreenElementVectorDeletingDestructor(ScreenElement* self, re::Edx, uint32_t flags) {
    if ((flags & 2) == 0) {
        ScreenElementDestruct(self);
        if ((flags & 1) != 0) {
            NiDelete(self, 0x14);
        }
        return self;
    }
    uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
    for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
        ScreenElementDestruct(&self[i]);
    }
    if ((flags & 1) != 0) {
        NiDeleteArray(block, 0x14);
    }
    return block;
}

// the pool's CreateNewBlock(count): a block of new elements, each put in the free set (grown to
// twice its size, or 1, when full), the block linked first. (The block isn't checked for null)
void __fastcall ElementPoolCreateNewBlock(ElementPool* self, re::Edx, uint32_t count) {
    PoolBlock* block = static_cast<PoolBlock*>(NiNew(0xC));
    if (block != nullptr) {
        block->count = count;
        block->next = nullptr;
        block->elements = count != 0 ? ScreenElement_NewArray(count) : nullptr;
    }
    for (uint32_t i = 0; i < count; i++) {
        ScreenElement* element = i < block->count ? &block->elements[i] : nullptr;
        if (self->count == self->allocated) {
            ElementSet_Realloc(self, self->allocated != 0 ? self->allocated * 2 : 1);
        }
        self->free[self->count] = element;
        self->count++;
    }
    block->next = self->blocks;
    self->blocks = block;
}

// _SDMInit: the pool (blocks of 8 at first)
void __cdecl ScreenGeometryDataSDMInit() {
    ElementPool* pool = static_cast<ElementPool*>(NiNew(0x18));
    if (pool == nullptr) {
        ms_pkElementPool = nullptr;
        return;
    }
    pool->free = nullptr;
    pool->allocated = 0;
    pool->count = 0;
    pool->blocks = nullptr;
    pool->growBy = 8;
    pool->initialSize = 8;
    ms_pkElementPool = pool;
}

// ---- NiScreenGeometryData

NiScreenGeometryData* __fastcall ScreenGeometryDataConstruct(NiScreenGeometryData* self, re::Edx) {
    TriShapeData_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiScreenGeometryData_vtable);
    self->pixelAccurate = 0;
    self->unknown59 = 0;
    self->unknown5C = 0;
    self->elements.vtable = reinterpret_cast<void*>(ElementArray_vtable);
    self->elements.maxSize = 4;
    self->elements.growBy = 4;
    self->elements.size = 0;
    self->elements.esize = 0;
    self->elements.base = static_cast<re::Ptr32<ScreenElement>*>(NiMalloc(0x10));
    self->elements.vtable = reinterpret_cast<void*>(ElementArray_vtable);
    return self;
}

const RTTI* __fastcall ScreenGeometryDataGetRTTI(void*, re::Edx) {
    return &NiScreenGeometryData_RTTI;
}

void* __cdecl ScreenGeometryDataCreateObject() {
    void* object = NiNew(0x70);
    if (object == nullptr) {
        return nullptr;
    }
    ScreenGeometryData_Construct(object);
    return object;
}

// each element taken out (its arrays freed, but not cleared: an element the pool hands out again
// has them freed a second time by LoadBinary, as the game has it) and given back to the pool
void __fastcall ScreenGeometryDataDestruct(NiScreenGeometryData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiScreenGeometryData_vtable);
    for (uint32_t i = 0; i < self->elements.size; i++) {
        re::Ptr32<ScreenElement> element = self->elements.base[i];
        if (i >= self->elements.size) {
            self->elements.size = uint16_t(i + 1);
        } else if (self->elements.base[i] != nullptr) {
            self->elements.esize--;
        }
        self->elements.base[i] = nullptr;
        if (element != nullptr) {
            NiDeleteArray(element->positions, 8);
            NiDeleteArray(element->colors, 0x10);
            NiDeleteArray(element->textures, 8);
            ElementSet_AddUnique(ms_pkElementPool, &element);
        }
    }
    ElementArray_SetSize(&self->elements, 0);
    self->elements.vtable = reinterpret_cast<void*>(ElementArray_vtable);
    NiFree(self->elements.base);
    TriShapeData_Destruct(self);
}

NiScreenGeometryData* __fastcall ScreenGeometryDataDeletingDestructor(NiScreenGeometryData* self, re::Edx,
                                                                      uint32_t flags) {
    ScreenGeometryData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x70);
    }
    return self;
}

void __fastcall ScreenGeometryDataGetViewerStrings(NiScreenGeometryData* self, re::Edx, NiTArray<char*>* strings) {
    GeometryData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiScreenGeometryData_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("m_bPixelAccurate", self->pixelAccurate));
    AddViewerString(strings, GetViewerStringUint("Num Elements", self->elements.esize));
}

// the pixel-accurate flag, the element count, then each element: its vertex count, and with
// vertices its positions and colors (each after a flag) and its texture sets
void __fastcall ScreenGeometryDataSaveBinary(NiScreenGeometryData* self, re::Edx, void* stream) {
    TriShapeData_SaveBinary(self, stream);
    SaveByte(stream, self->pixelAccurate);
    uint32_t count = self->elements.size;
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        ScreenElement* element = self->elements.base[i];
        uint16_t vertices = element != nullptr ? element->vertexCount : 0;
        SaveBytes(stream, &vertices, 2);
        if (vertices == 0) {
            continue;
        }
        float* textures = element->textures;
        uint16_t sets = element->textureSets;
        float* colors = element->colors;
        float* positions = element->positions;
        SaveByte(stream, positions != nullptr ? 1 : 0);
        if (positions != nullptr) {
            for (uint32_t j = 0; j < vertices; j++) {
                Point2_SaveBinary(positions + j * 2, stream);
            }
        }
        SaveByte(stream, colors != nullptr ? 1 : 0);
        if (colors != nullptr) {
            for (uint32_t j = 0; j < vertices; j++) {
                ColorA_SaveBinary(colors + j * 4, stream);
            }
        }
        SaveBytes(stream, &sets, 2);
        if (sets != 0) {
            uint32_t total = uint32_t(sets) * vertices;
            for (uint32_t k = 0; k < total; k++) {
                Point2_SaveBinary(textures + k * 2, stream);
            }
        }
    }
}

// the elements read back, each from the pool (a new block of growBy elements when it's empty, the
// next block twice as big): its old arrays freed (see the destructor), the new ones in
void __fastcall ScreenGeometryDataLoadBinary(NiScreenGeometryData* self, re::Edx, void* stream) {
    TriShapeData_LoadBinary(self, stream);
    self->pixelAccurate = LoadByte(stream) != 0 ? 1 : 0;
    uint32_t count = 0;
    LoadBytes(stream, &count, 4);
    if (count == 0) {
        return;
    }
    ElementArray_SetSize(&self->elements, count);
    for (uint32_t i = 0; i < count; i++) {
        uint16_t vertices = 0;
        LoadBytes(stream, &vertices, 2);
        if (vertices == 0) {
            continue;
        }
        uint16_t sets = 0;
        float* positions = nullptr;
        float* colors = nullptr;
        float* textures = nullptr;
        if (LoadByte(stream) != 0) {
            positions = static_cast<float*>(NiNewArray(uint32_t(vertices) * 8));
            LoadArray(stream, positions, uint32_t(vertices) * 8, 8);
        }
        if (LoadByte(stream) != 0) {
            colors = static_cast<float*>(NiNewArray(uint32_t(vertices) * 0x10));
            if (colors != nullptr) {
                VectorConstruct(colors, 0x10, vertices, reinterpret_cast<void*>(ColorA_Construct));
            }
            LoadArray(stream, colors, uint32_t(vertices) << 4, 0x10);
        }
        LoadBytes(stream, &sets, 2);
        if (sets != 0) {
            uint32_t total = uint32_t(sets) * vertices;
            textures = static_cast<float*>(NiNewArray(total * 8));
            LoadArray(stream, textures, total * 8, 8);
        }
        ElementPool* pool = ms_pkElementPool;
        if (pool->count == 0) {
            ElementPool_CreateNewBlock(pool, pool->growBy);
            pool->growBy *= 2;
        }
        re::Ptr32<ScreenElement> element = pool->free[0];
        pool->count--;
        pool->free[0] = pool->free[pool->count];
        NiDeleteArray(element->positions, 8);
        element->vertexCount = vertices;
        element->positions = positions;
        NiDeleteArray(element->colors, 0x10);
        element->colors = colors;
        NiDeleteArray(element->textures, 8);
        element->textures = textures;
        element->textureSets = sets;
        ElementArray_SetAt(&self->elements, i, &element);
    }
}

// NiTriShapeData's, the flag, the counts, then the used elements (by the used count): both or
// neither, the same counts, then per vertex its position, color (the arrays not checked) and
// texture coordinates
uint8_t __fastcall ScreenGeometryDataIsEqual(NiScreenGeometryData* self, re::Edx, NiScreenGeometryData* other) {
    if (!TriShapeData_IsEqual(self, other) || other->pixelAccurate != self->pixelAccurate) {
        return 0;
    }
    uint32_t used = other->elements.esize;
    if (used != self->elements.esize || other->elements.size != self->elements.size) {
        return 0;
    }
    for (uint32_t i = 0; i < used; i++) {
        ScreenElement* a = other->elements.base[i];
        ScreenElement* b = self->elements.base[i];
        if (a == nullptr) {
            if (b != nullptr) {
                return 0;
            }
            continue;
        }
        if (b == nullptr || a->vertexCount != b->vertexCount || a->textureSets != b->textureSets) {
            return 0;
        }
        uint32_t sets = a->textureSets;
        for (uint32_t j = 0; j < a->vertexCount; j++) {
            if (Point2_NotEqual(a->positions + j * 2, b->positions + j * 2) ||
                ColorA_NotEqual(a->colors + j * 4, b->colors + j * 4)) {
                return 0;
            }
            for (uint32_t k = 0; k < sets; k++) {
                uint32_t index = (j * sets + k) * 2;
                if (Point2_NotEqual(a->textures + index, b->textures + index)) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// ---- NiScreenPolygon

// the arrays, the property state let go, NiObject's destructor
void __fastcall ScreenPolygonDestruct(NiScreenPolygon* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiScreenPolygon_vtable);
    NiDeleteArray(self->vertices, 0xC);
    NiDeleteArray(self->textures, 8);
    NiDeleteArray(self->colors, 0x10);
    NiPropertyState* state = self->state;
    if (state != nullptr && ni::sys::Decrement(&state->refCount) == 0) {
        Virtual<void>(state, 1);
    }
    Object_Destruct(self);
}

const RTTI* __fastcall ScreenPolygonGetRTTI(void*, re::Edx) {
    return &NiScreenPolygon_RTTI;
}

// CopyMembers(dest, cloning): NiObject's, then a copy of the property state (not the vertices)
void __fastcall ScreenPolygonCopyMembers(NiScreenPolygon* self, re::Edx, NiScreenPolygon* dest, void* cloning) {
    Object_CopyMembers(self, dest, cloning);
    void* memory = NiNew(0x38);
    NiPropertyState* state = memory != nullptr ? PropertyState_ConstructCopy(memory, self->state) : nullptr;
    SetReference(dest->state, state);
}

// vf6 RegisterStreamables: NiObject's, then the twelve properties' (not checked for null)
uint8_t __fastcall ScreenPolygonRegisterStreamables(NiScreenPolygon* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    for (int i = 0; i < 12; i++) {
        Virtual<uint8_t>(self->state->properties[i], 6, stream);
    }
    return 1;
}

NiScreenPolygon* __fastcall ScreenPolygonDeletingDestructor(NiScreenPolygon* self, re::Edx, uint32_t flags) {
    ScreenPolygon_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x1C);
    }
    return self;
}

inline void ScreenPolygonInit(NiScreenPolygon* object) {
    RefObject_Construct(object);
    object->vtable = reinterpret_cast<void*>(NiScreenPolygon_vtable);
    object->state = nullptr;
    object->vertexCount = 0;
    object->vertices = nullptr;
    object->textures = nullptr;
    object->colors = nullptr;
}

// vf3 CreateClone (copied into a null clone when the allocation fails, as the game has it)
NiScreenPolygon* __fastcall ScreenPolygonCreateClone(NiScreenPolygon* self, re::Edx, void* cloning) {
    NiScreenPolygon* clone = static_cast<NiScreenPolygon*>(NiNew(0x1C));
    if (clone != nullptr) {
        ScreenPolygonInit(clone);
    }
    ScreenPolygon_CopyMembers(self, clone, cloning);
    return clone;
}

NiScreenPolygon* __cdecl ScreenPolygonCreateObject() {
    NiScreenPolygon* object = static_cast<NiScreenPolygon*>(NiNew(0x1C));
    if (object == nullptr) {
        return nullptr;
    }
    ScreenPolygonInit(object);
    return object;
}

// the vertices, then the texture coordinates and colors after their flags (the colors zeroed
// first), then the property links
void __fastcall ScreenPolygonLoadBinary(NiScreenPolygon* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    LoadBytes(stream, &self->vertexCount, 2);
    self->vertices = static_cast<NiPoint3*>(NiNewArray(uint32_t(self->vertexCount) * 0xC));
    for (uint16_t i = 0; i < self->vertexCount; i++) {
        Point3_LoadBinary(&self->vertices[i], stream);
    }
    if (LoadByte(stream) != 0) {
        self->textures = static_cast<float*>(NiNewArray(uint32_t(self->vertexCount) * 8));
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            Point2_LoadBinary(self->textures + i * 2, stream);
        }
    }
    if (LoadByte(stream) != 0) {
        uint32_t count = self->vertexCount;
        float* colors = static_cast<float*>(NiNewArray(count * 0x10));
        if (colors != nullptr) {
            for (uint32_t i = 0; i < count; i++) {
                colors[i * 4] = 0.0f;
                colors[i * 4 + 1] = 0.0f;
                colors[i * 4 + 2] = 0.0f;
                colors[i * 4 + 3] = 0.0f;
            }
        }
        self->colors = colors;
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            ColorA_LoadBinary(self->colors + i * 4, stream);
        }
    }
    Stream_ReadMultipleLinkIDs(stream);
}

// the linked properties, each in the state's slot for its type (slot 18)
void __fastcall ScreenPolygonLinkObject(NiScreenPolygon* self, re::Edx, void* stream) {
    Empty_8A20A0(self, stream);
    uint32_t count = Stream_GetNumberOfLinkIDs(stream);
    while (count != 0) {
        NiPropertyState* state = self->state;
        count--;
        NiObject* property = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        if (property == nullptr) {
            continue;
        }
        uint32_t type = Virtual<uint32_t>(property, 18);
        SetReference(state->properties[type], property);
    }
}

void __fastcall ScreenPolygonSaveBinary(NiScreenPolygon* self, re::Edx, void* stream) {
    Empty_8A20A0(self, stream);
    SaveBytes(stream, &self->vertexCount, 2);
    for (uint16_t i = 0; i < self->vertexCount; i++) {
        Point3_SaveBinary(&self->vertices[i], stream);
    }
    SaveByte(stream, self->textures != nullptr ? 1 : 0);
    if (self->textures != nullptr) {
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            Point2_SaveBinary(self->textures + i * 2, stream);
        }
    }
    SaveByte(stream, self->colors != nullptr ? 1 : 0);
    if (self->colors != nullptr) {
        for (uint16_t i = 0; i < self->vertexCount; i++) {
            ColorA_SaveBinary(self->colors + i * 4, stream);
        }
    }
    uint32_t properties = 12;
    SaveBytes(stream, &properties, 4);
    for (uint16_t i = 0; i < properties; i++) {
        Virtual<void>(stream, 12, self->state->properties[i]);   // SaveLinkID
    }
}

// NiObject's, the vertices, both texture arrays or neither and theirs, the same for the colors,
// the same twelve properties
uint8_t __fastcall ScreenPolygonIsEqual(NiScreenPolygon* self, re::Edx, NiScreenPolygon* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    uint16_t count = self->vertexCount;
    if (count != other->vertexCount) {
        return 0;
    }
    for (uint16_t i = 0; i < count; i++) {
        const NiPoint3& a = self->vertices[i];
        const NiPoint3& b = other->vertices[i];
        if (!(b.x == a.x) || !(b.y == a.y) || !(b.z == a.z)) {
            return 0;
        }
    }
    if (self->textures != nullptr) {
        if (other->textures == nullptr) {
            return 0;
        }
        for (uint16_t i = 0; i < count; i++) {
            if (!(other->textures[i * 2] == self->textures[i * 2]) ||
                !(other->textures[i * 2 + 1] == self->textures[i * 2 + 1])) {
                return 0;
            }
        }
    } else if (other->textures != nullptr) {
        return 0;
    }
    if (self->colors != nullptr) {
        if (other->colors == nullptr) {
            return 0;
        }
        for (uint16_t i = 0; i < count; i++) {
            for (int c = 0; c < 4; c++) {
                if (!(other->colors[i * 4 + c] == self->colors[i * 4 + c])) {
                    return 0;
                }
            }
        }
    } else if (other->colors != nullptr) {
        return 0;
    }
    for (uint16_t i = 0; i < 12; i++) {
        if (self->state->properties[i] != other->state->properties[i]) {
            return 0;
        }
    }
    return 1;
}

void __fastcall ScreenPolygonGetViewerStrings(NiScreenPolygon* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiScreenPolygon_RTTI.name));
    AddViewerString(strings, GetViewerStringShort("m_usVertices", self->vertexCount));
    AddViewerString(strings, GetViewerStringPointer("m_pkVertex", self->vertices));
    AddViewerString(strings, GetViewerStringPointer("m_pkTexture", self->textures));
    AddViewerString(strings, GetViewerStringPointer("m_pkColor", self->colors));
    for (int i = 0; i < 12; i++) {
        NiObject* property = self->state->properties[i];
        const RTTI* rtti = Virtual<const RTTI*>(property, 2);
        AddViewerStringInline(strings, GetViewerStringPointer(rtti->name, property));
    }
}

// ---- NiScreenSpaceCamera

// NiCamera's, then the polygons' and textures' ProcessClone (slot 11)
void __fastcall ScreenSpaceCameraProcessClone(NiScreenSpaceCamera* self, re::Edx, void* cloning) {
    AVObject_ProcessClone(self, cloning);
    for (uint32_t i = 0; i < self->polygons.size; i++) {
        NiObject* polygon = self->polygons.base[i];
        if (polygon != nullptr) {
            Virtual<void>(polygon, 11, cloning);
        }
    }
    for (uint32_t i = 0; i < self->textures.size; i++) {
        NiObject* texture = self->textures.base[i];
        if (texture != nullptr) {
            Virtual<void>(texture, 11, cloning);
        }
    }
}

// both arrays emptied (each slot let go, then RemoveAll) and freed, NiCamera's destructor
void __fastcall ScreenSpaceCameraDestruct(NiScreenSpaceCamera* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiScreenSpaceCamera_vtable);
    for (uint32_t i = 0; i < self->polygons.size; i++) {
        re::Ptr32<NiObject> none = nullptr;
        ObjectArray_SetAt(&self->polygons, i, &none);
    }
    ObjectArray_RemoveAll(&self->polygons);
    for (uint32_t i = 0; i < self->textures.size; i++) {
        re::Ptr32<NiObject> none = nullptr;
        ObjectArray_SetAt(&self->textures, i, &none);
    }
    ObjectArray_RemoveAll(&self->textures);
    self->textures.vtable = reinterpret_cast<void*>(ObjectArray_vtable);
    ObjectArray_DeleteArray(self->textures.base);
    self->polygons.vtable = reinterpret_cast<void*>(ObjectArray_vtable);
    ObjectArray_DeleteArray(self->polygons.base);
    Camera_Destruct(self);
}

const RTTI* __fastcall ScreenSpaceCameraGetRTTI(void*, re::Edx) {
    return &NiScreenSpaceCamera_RTTI;
}

// one array's clones (slot 3 CreateClone of each, held while it's put in)
inline void CloneArray(NiTArray<NiObject*>* from, NiTArray<NiObject*>* to, void* cloning) {
    ObjectArray_SetSize(to, from->size);
    to->growBy = from->growBy;
    for (uint32_t i = 0; i < from->size; i++) {
        NiObject* object = from->base[i];
        if (object == nullptr) {
            re::Ptr32<NiObject> none = nullptr;
            ObjectArray_SetAt(to, i, &none);
            continue;
        }
        ni::sys::Increment(&object->refCount);
        re::Ptr32<NiObject> clone = Virtual<NiObject*>(object, 3, cloning);
        if (clone != nullptr) {
            ni::sys::Increment(&clone->refCount);
        }
        ObjectArray_SetAt(to, i, &clone);
        if (clone != nullptr && ni::sys::Decrement(&clone->refCount) == 0) {
            Virtual<void>(clone, 1);
        }
        if (ni::sys::Decrement(&object->refCount) == 0) {
            Virtual<void>(object, 1);
        }
    }
}

void __fastcall ScreenSpaceCameraCopyMembers(NiScreenSpaceCamera* self, re::Edx, NiScreenSpaceCamera* dest,
                                             void* cloning) {
    Camera_CopyMembers(self, dest, cloning);
    CloneArray(&self->polygons, &dest->polygons, cloning);
    CloneArray(&self->textures, &dest->textures, cloning);
}

void __fastcall ScreenSpaceCameraGetViewerStrings(NiScreenSpaceCamera* self, re::Edx, NiTArray<char*>* strings) {
    Camera_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiScreenSpaceCamera_RTTI.name));
}

// NiScreenSpaceCamera(): an orthographic NiCamera, the arrays (5 slots, growing by 5)
NiScreenSpaceCamera* __fastcall ScreenSpaceCameraConstruct(NiScreenSpaceCamera* self, re::Edx) {
    Camera_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiScreenSpaceCamera_vtable);
    self->polygons.vtable = reinterpret_cast<void*>(ObjectArray_vtable);
    self->polygons.maxSize = 5;
    self->polygons.growBy = 5;
    self->polygons.size = 0;
    self->polygons.esize = 0;
    self->polygons.base = static_cast<re::Ptr32<NiObject>*>(NiNewPointerArray(5));
    self->polygons.vtable = reinterpret_cast<void*>(ObjectArray_vtable);
    self->textures.vtable = reinterpret_cast<void*>(ObjectArray_vtable);
    self->textures.maxSize = 5;
    self->textures.growBy = 5;
    self->textures.size = 0;
    self->textures.esize = 0;
    self->textures.base = static_cast<re::Ptr32<NiObject>*>(NiNewPointerArray(5));
    self->textures.vtable = reinterpret_cast<void*>(ObjectArray_vtable);
    self->frustum.ortho = 1;
    Camera_UpdateWorldData(self);
    Camera_UpdateWorldBound(self);
    return self;
}

NiScreenSpaceCamera* __fastcall ScreenSpaceCameraDeletingDestructor(NiScreenSpaceCamera* self, re::Edx,
                                                                    uint32_t flags) {
    ScreenSpaceCamera_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x15C);
    }
    return self;
}

// vf3 CreateClone (copied into a null clone when the allocation fails, as the game has it)
NiScreenSpaceCamera* __fastcall ScreenSpaceCameraCreateClone(NiScreenSpaceCamera* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x15C);
    NiScreenSpaceCamera* clone = memory != nullptr ? ScreenSpaceCamera_Construct(memory) : nullptr;
    ScreenSpaceCamera_CopyMembers(self, clone, cloning);
    return clone;
}

NiScreenSpaceCamera* __cdecl ScreenSpaceCameraCreateObject() {
    void* memory = NiNew(0x15C);
    if (memory == nullptr) {
        return nullptr;
    }
    return ScreenSpaceCamera_Construct(memory);
}

// LinkObject: NiCamera's, then (as the game has it) each array's elements added again after
// themselves (the slot's address taken before a growth that may move it) and the array emptied
void __fastcall ScreenSpaceCameraLinkObject(NiScreenSpaceCamera* self, re::Edx, void* stream) {
    Camera_LinkObject(self, stream);
    NiTArray<NiObject*>* arrays[2] = {&self->polygons, &self->textures};
    for (NiTArray<NiObject*>* array : arrays) {
        uint32_t count = array->size;
        for (uint32_t i = 0; i < count; i++) {
            uint32_t index = array->size;
            const re::Ptr32<NiObject>* item = &array->base[i];
            if (array->maxSize <= index) {
                ObjectArray_SetSize(array, array->growBy + index);
            }
            ObjectArray_SetAt(array, index, item);
        }
        ObjectArray_RemoveAll(array);
    }
}

// ---- NiWireframeProperty

NiWireframeProperty* __cdecl WireframeCreateObject() {
    NiWireframeProperty* object = static_cast<NiWireframeProperty*>(NiNew(0x1C));
    if (object == nullptr) {
        return nullptr;
    }
    ObjectNET_Construct(object);
    object->vtable = reinterpret_cast<void*>(NiWireframeProperty_vtable);
    object->flags = 0;
    return object;
}

// _SDMInit: the default property (ms_spDefault), off
void __cdecl WireframeSDMInit() {
    NiWireframeProperty* object = static_cast<NiWireframeProperty*>(NiNew(0x1C));
    if (object != nullptr) {
        ObjectNET_Construct(object);
        object->vtable = reinterpret_cast<void*>(NiWireframeProperty_vtable);
        object->flags = 0;
    }
    SetReference(NiWireframeProperty_ms_spDefault, static_cast<NiRefObject*>(object));
}

// _SDMShutdown (NiMainSDM::Shutdown's): the default let go, the slot emptied
void __cdecl WireframeSDMShutdown() {
    SetReference(NiWireframeProperty_ms_spDefault, static_cast<NiRefObject*>(nullptr));
}

// NiScreenGeometryData::_SDMShutdown (NiMainSDM::Shutdown's): the element pool destroyed and
// deleted (the pointer left as it was)
void __cdecl ScreenGeometryDataSDMShutdown() {
    ElementPool* pool = ms_pkElementPool;
    if (pool != nullptr) {
        ElementPool_Destruct(pool);
        NiDelete(pool, sizeof(ElementPool));
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISCREENPOLY_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

// lone jumps to NiCamera's (the target in the comment)
RE_UNHOOKED("NiScreenSpaceCamera::LoadBinary (jump)", 0x007D6290);            // -> 0x763660
RE_UNHOOKED("NiScreenSpaceCamera::RegisterStreamables (jump)", 0x007D62A0);   // -> 0x762E00
RE_UNHOOKED("NiScreenSpaceCamera::SaveBinary (jump)", 0x007D62B0);            // -> 0x763960
RE_UNHOOKED("NiScreenSpaceCamera::IsEqual (jump)", 0x007D62C0);               // -> 0x762E10

NISCREENPOLY_REVERSED("NiScreenGeometryData::ScreenElement::ScreenElement", 0x007D61F0, ni::ScreenElementConstruct);
NISCREENPOLY_REVERSED("NiScreenGeometry::GetRTTI", 0x007D6220, ni::ScreenGeometryGetRTTI);
NISCREENPOLY_REVERSED("NiScreenGeometry::CreateObject", 0x007D6230, ni::ScreenGeometryCreateObject);
NISCREENPOLY_REVERSED("NiScreenGeometry::DeletingDestructor", 0x007D62D0, ni::ScreenGeometryDeletingDestructor);
NISCREENPOLY_REVERSED("NiScreenGeometry::CreateClone", 0x007D6300, ni::ScreenGeometryCreateClone);
NISCREENPOLY_REVERSED("NiScreenPolygon::~NiScreenPolygon", 0x007D6350, ni::ScreenPolygonDestruct);
NISCREENPOLY_REVERSED("NiScreenPolygon::GetRTTI", 0x007D63B0, ni::ScreenPolygonGetRTTI);
NISCREENPOLY_REVERSED("NiScreenPolygon::CopyMembers", 0x007D63C0, ni::ScreenPolygonCopyMembers);
NISCREENPOLY_REVERSED("NiScreenPolygon::RegisterStreamables", 0x007D6430, ni::ScreenPolygonRegisterStreamables);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::ProcessClone", 0x007D6470, ni::ScreenSpaceCameraProcessClone);
NISCREENPOLY_REVERSED("NiScreenGeometryData::ScreenElement::operator new[]", 0x007D64F0, ni::ScreenElementNewArray);
NISCREENPOLY_REVERSED("NiScreenGeometryData::ScreenElement::VectorDeletingDestructor", 0x007D6550,
                      ni::ScreenElementVectorDeletingDestructor);
NISCREENPOLY_REVERSED("NiScreenGeometryData::SaveBinary", 0x007D6600, ni::ScreenGeometryDataSaveBinary);
NISCREENPOLY_REVERSED("NiScreenGeometryData::IsEqual", 0x007D6830, ni::ScreenGeometryDataIsEqual);
NISCREENPOLY_REVERSED("NiScreenPolygon::DeletingDestructor", 0x007D69F0, ni::ScreenPolygonDeletingDestructor);
NISCREENPOLY_REVERSED("NiScreenPolygon::CreateClone", 0x007D6A10, ni::ScreenPolygonCreateClone);
NISCREENPOLY_REVERSED("NiScreenPolygon::CreateObject", 0x007D6A70, ni::ScreenPolygonCreateObject);
NISCREENPOLY_REVERSED("NiScreenPolygon::LoadBinary", 0x007D6AB0, ni::ScreenPolygonLoadBinary);
NISCREENPOLY_REVERSED("NiScreenPolygon::LinkObject", 0x007D6C50, ni::ScreenPolygonLinkObject);
NISCREENPOLY_REVERSED("NiScreenPolygon::SaveBinary", 0x007D6CE0, ni::ScreenPolygonSaveBinary);
NISCREENPOLY_REVERSED("NiScreenPolygon::IsEqual", 0x007D6E60, ni::ScreenPolygonIsEqual);
NISCREENPOLY_REVERSED("NiScreenGeometryData::NiScreenGeometryData", 0x007D7040, ni::ScreenGeometryDataConstruct);
NISCREENPOLY_REVERSED("NiScreenGeometryData::GetRTTI", 0x007D7090, ni::ScreenGeometryDataGetRTTI);
NISCREENPOLY_REVERSED("NiScreenGeometryData::CreateObject", 0x007D70A0, ni::ScreenGeometryDataCreateObject);
NISCREENPOLY_REVERSED("NiTPointerPool<ScreenElement>::CreateNewBlock", 0x007D70C0, ni::ElementPoolCreateNewBlock);
NISCREENPOLY_REVERSED("NiScreenGeometryData::~NiScreenGeometryData", 0x007D7170, ni::ScreenGeometryDataDestruct);
NISCREENPOLY_REVERSED("NiScreenGeometryData::_SDMInit", 0x007D7230, ni::ScreenGeometryDataSDMInit);
NISCREENPOLY_REVERSED("NiScreenGeometryData::GetViewerStrings", 0x007D7270, ni::ScreenGeometryDataGetViewerStrings);
NISCREENPOLY_REVERSED("NiScreenGeometry::GetViewerStrings", 0x007D7340, ni::ScreenGeometryGetViewerStrings);
NISCREENPOLY_REVERSED("NiScreenPolygon::GetViewerStrings", 0x007D7390, ni::ScreenPolygonGetViewerStrings);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::~NiScreenSpaceCamera", 0x007D7570, ni::ScreenSpaceCameraDestruct);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::GetRTTI", 0x007D7630, ni::ScreenSpaceCameraGetRTTI);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::CopyMembers", 0x007D7640, ni::ScreenSpaceCameraCopyMembers);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::GetViewerStrings", 0x007D7830, ni::ScreenSpaceCameraGetViewerStrings);
NISCREENPOLY_REVERSED("NiScreenGeometryData::DeletingDestructor", 0x007D7880,
                      ni::ScreenGeometryDataDeletingDestructor);
NISCREENPOLY_REVERSED("NiScreenGeometryData::LoadBinary", 0x007D78C0, ni::ScreenGeometryDataLoadBinary);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::NiScreenSpaceCamera", 0x007D7C00, ni::ScreenSpaceCameraConstruct);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::DeletingDestructor", 0x007D7CB0, ni::ScreenSpaceCameraDeletingDestructor);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::CreateClone", 0x007D7CE0, ni::ScreenSpaceCameraCreateClone);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::CreateObject", 0x007D7D30, ni::ScreenSpaceCameraCreateObject);
NISCREENPOLY_REVERSED("NiScreenSpaceCamera::LinkObject", 0x007D7D50, ni::ScreenSpaceCameraLinkObject);
NISCREENPOLY_REVERSED("NiWireframeProperty::CreateObject", 0x007D7EA0, ni::WireframeCreateObject);
NISCREENPOLY_REVERSED("NiWireframeProperty::_SDMInit", 0x007D7ED0, ni::WireframeSDMInit);
// the _SDMShutdowns: never hooked in the 32-bit game, bodies at their addresses for the 64-bit build
RE_UNHOOKED_FN("NiScreenGeometryData::_SDMShutdown", 0x007D78A0, &ni::ScreenGeometryDataSDMShutdown);
RE_UNHOOKED_FN("NiWireframeProperty::_SDMShutdown", 0x007D7F40, &ni::WireframeSDMShutdown);
