// Ni2DBuffer (vtable 0x960710, 0x14 bytes): a renderer surface's size and its renderer data (a
// reference); its slots 18 FastCopy and 19 Copy hand the work to the renderer (its slots 58 and
// 59). NiRenderedCubeMap (vtable 0x9607A8, 0x50 bytes): an NiRenderedTexture with six face
// buffers. And NiObjectGroup (the block NiStream allocates its objects from: size, buffer,
// next free byte, reference count).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiTexture.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, Ni2DBuffer_RTTI, 0x020E3804);
GAME_VAR(RTTI, NiRenderedCubeMap_RTTI, 0x020E386C);
GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);

constexpr uintptr_t kNi2DBufferVtable = 0x00960710;
constexpr uintptr_t kNiTextureVtable = 0x009519B8;
constexpr uintptr_t kNiRenderedCubeMapVtable = 0x009607A8;

struct Ni2DBuffer : NiObject {
    uint32_t width;            // +0x8
    uint32_t height;           // +0xC
    re::Ptr32<NiObject> rendererData;    // +0x10 (a reference)
};
static_assert(sizeof(Ni2DBuffer) == 0x14);

struct NiRenderedCubeMap : NiRenderedTexture {
    uint32_t currentFace;      // +0x34 (0)
    re::Ptr32<Ni2DBuffer> faces[6];      // +0x38 (references)
};
static_assert(offsetof(NiRenderedCubeMap, faces) == 0x38);
static_assert(sizeof(NiRenderedCubeMap) == 0x50);

struct NiObjectGroup {
    uint32_t size;             // +0x0
    re::Ptr32<uint8_t> buffer;           // +0x4
    re::Ptr32<uint8_t> free;             // +0x8
    uint32_t refCount;         // +0xC
};
static_assert(sizeof(NiObjectGroup) == 0x10);

GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
GAME_FN(void, __thiscall, Buffer_SetRendererData, 0x007ABEF0, Ni2DBuffer* self, NiObject* data);
GAME_FN(void, __thiscall, Texture_AddToList, 0x0075D7D0, NiTexture* self);
GAME_FN(void, __thiscall, Texture_RemoveFromList, 0x0075D850, NiTexture* self);
// (this file's, called where modes apply)
GAME_FN(Ni2DBuffer*, __cdecl, Ni2DBuffer_Create, 0x007D1E40, uint32_t width, uint32_t height);
GAME_FN(void, __thiscall, Ni2DBuffer_Destruct, 0x007D1E00, Ni2DBuffer* self);
GAME_FN(NiRenderedCubeMap*, __thiscall, RenderedCubeMap_Construct, 0x007D3080, NiRenderedCubeMap* self);
GAME_FN(void, __thiscall, RenderedCubeMap_Destruct, 0x007D30F0, NiRenderedCubeMap* self);

inline void Unref(NiObject* object) {
    if (object != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(&object->refCount)) == 0) {
        Virtual<void>(object, 1);
    }
}

// Ni2DBuffer's constructor, inlined where it's made
inline void Construct2DBuffer(Ni2DBuffer* buffer) {
    RefObject_Construct(buffer);
    buffer->vtable = reinterpret_cast<void*>(kNi2DBufferVtable);
    buffer->width = 0;
    buffer->height = 0;
    buffer->rendererData = nullptr;
}

// ---- NiObjectGroup

// 0x7D1C90 NiObjectGroup(size)
NiObjectGroup* __fastcall ObjectGroup_ConstructImpl(NiObjectGroup* self, re::Edx, uint32_t size) {
    self->size = size;
    if (size != 0) {
        uint8_t* buffer = static_cast<uint8_t*>(NiMalloc(size));
        self->buffer = buffer;
        self->free = buffer;
        self->refCount = 0;
        return self;
    }
    self->buffer = nullptr;
    self->free = self->buffer;
    self->refCount = 0;
    return self;
}

// 0x7D1CE0 ~NiObjectGroup
void __fastcall ObjectGroup_DestructImpl(NiObjectGroup* self, re::Edx) {
    NiFree(self->buffer);
}

// ---- Ni2DBuffer

// 0x7D1CF0 SetDimensions(width, height) (the renderer data, after a device reset)
void __fastcall SetDimensionsImpl(Ni2DBuffer* self, re::Edx, uint32_t width, uint32_t height) {
    self->width = width;
    self->height = height;
}

// 0x7D1D10 vf6 RegisterStreamables: NiObject's, as a bool (Ni2DBuffer, NiRenderTargetGroup,
// NiSkinData, NiAVObjectPalette share it)
uint8_t __fastcall RegisterStreamablesImpl(NiObject* self, re::Edx, void* stream) {
    return Object_RegisterStreamables(self, stream) != 0 ? 1 : 0;
}

// 0x7D1D30 vf18 FastCopy(dest, srcRect, destX, destY): the renderer's (slot 58), when both
// buffers have renderer data
uint8_t __fastcall FastCopyImpl(Ni2DBuffer* self, re::Edx, Ni2DBuffer* dest, const void* srcRect, uint32_t destX,
                                uint32_t destY) {
    void* renderer = NiRenderer_ms_pkRenderer;
    if (self->rendererData == nullptr || renderer == nullptr || dest->rendererData == nullptr) {
        return 0;
    }
    return Virtual<uint8_t>(renderer, 58, self, dest, srcRect, destX, destY);
}

// 0x7D1D80 vf19 Copy(dest, srcRect, destRect, filter): the renderer's (slot 59)
uint8_t __fastcall CopyImpl(Ni2DBuffer* self, re::Edx, Ni2DBuffer* dest, const void* srcRect, const void* destRect,
                            uint32_t filter) {
    void* renderer = NiRenderer_ms_pkRenderer;
    if (self->rendererData == nullptr || renderer == nullptr || dest->rendererData == nullptr) {
        return 0;
    }
    return Virtual<uint8_t>(renderer, 59, self, dest, srcRect, destRect, filter);
}

// 0x7D1DD0
Ni2DBuffer* __fastcall ConstructImpl(Ni2DBuffer* self, re::Edx) {
    RefObject_Construct(self);
    self->width = 0;
    self->height = 0;
    self->vtable = reinterpret_cast<void*>(kNi2DBufferVtable);
    self->rendererData = nullptr;
    return self;
}

// 0x7D1DF0 vf2
const RTTI* __fastcall GetRTTIImpl(Ni2DBuffer*, re::Edx) {
    return &Ni2DBuffer_RTTI;
}

// 0x7D1E00
void __fastcall DestructImpl(Ni2DBuffer* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNi2DBufferVtable);
    Unref(self->rendererData);
    Object_Destruct(self);
}

// 0x7D1E40 Create(width, height) (no allocation check: a failed one writes through null)
Ni2DBuffer* __cdecl CreateImpl(uint32_t width, uint32_t height) {
    Ni2DBuffer* buffer = static_cast<Ni2DBuffer*>(NiNew(0x14));
    if (buffer != nullptr) {
        Construct2DBuffer(buffer);
    }
    buffer->width = width;
    buffer->height = height;
    return buffer;
}

// 0x7D1EA0 Create(width, height, rendererData): none without renderer data that has a surface
// (+0xC)
Ni2DBuffer* __cdecl CreateWithDataImpl(uint32_t width, uint32_t height, NiObject* data) {
    if (data == nullptr || At<void*>(data, 0xC) == nullptr) {
        return nullptr;
    }
    Ni2DBuffer* buffer = static_cast<Ni2DBuffer*>(NiNew(0x14));
    if (buffer != nullptr) {
        Construct2DBuffer(buffer);
    }
    buffer->height = height;
    buffer->width = width;
    Pointer_Assign(&buffer->rendererData, data);
    Buffer_SetRendererData(buffer, data);
    return buffer;
}

// 0x7D1F10 vf0
Ni2DBuffer* __fastcall DeletingDestructorImpl(Ni2DBuffer* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kNi2DBufferVtable);
    Unref(self->rendererData);
    Object_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x14);
    }
    return self;
}

// 0x7D1F60 vf9 GetViewerStrings: NiObject's and the class name
void __fastcall GetViewerStringsImpl(Ni2DBuffer* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(Ni2DBuffer_RTTI.name));
}

// ---- NiRenderedCubeMap

// 0x7D3060 vf18 GetWidth: the first face's
uint32_t __fastcall CubeMap_GetWidthImpl(NiRenderedCubeMap* self, re::Edx) {
    return self->faces[0]->width;
}

// 0x7D3070 vf19 GetHeight
uint32_t __fastcall CubeMap_GetHeightImpl(NiRenderedCubeMap* self, re::Edx) {
    return self->faces[0]->height;
}

// 0x7D3080 (NiTexture's and NiRenderedTexture's constructors inlined)
NiRenderedCubeMap* __fastcall CubeMap_ConstructImpl(NiRenderedCubeMap* self, re::Edx) {
    ObjectNET_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNiTextureVtable);
    self->pixelLayout = 6;
    self->alphaFormat = 3;
    self->mipMapped = 2;
    self->rendererData = nullptr;
    Texture_AddToList(self);
    self->buffer = nullptr;
    self->vtable = reinterpret_cast<void*>(kNiRenderedCubeMapVtable);
    for (int i = 0; i < 6; i++) {
        self->faces[i] = nullptr;
    }
    self->currentFace = 0;
    return self;
}

// 0x7D30E0 vf2
const RTTI* __fastcall CubeMap_GetRTTIImpl(NiRenderedCubeMap*, re::Edx) {
    return &NiRenderedCubeMap_RTTI;
}

// 0x7D30F0 (the faces let go last to first, then NiRenderedTexture's and NiTexture's parts)
void __fastcall CubeMap_DestructImpl(NiRenderedCubeMap* self, re::Edx) {
    for (int i = 5; i >= 0; i--) {
        Unref(self->faces[i]);
    }
    Unref(self->buffer);
    self->vtable = reinterpret_cast<void*>(kNiTextureVtable);
    if (self->rendererData != nullptr) {
        Virtual<void>(self->rendererData, 0, 1u);
    }
    Texture_RemoveFromList(self);
    ObjectNET_Destruct(self);
}

// 0x7D3170 Create(size, renderer, prefs): six square face buffers (the size a power of two),
// then the renderer's slot 70 makes its renderer data (none: deleted)
NiRenderedCubeMap* __cdecl CubeMap_CreateImpl(uint32_t size, void* renderer, const uint32_t* prefs) {
    if (renderer == nullptr || size == 0 || (size & (size - 1)) != 0) {
        return nullptr;
    }
    void* memory = NiNew(0x50);
    NiRenderedCubeMap* cube =
        memory != nullptr ? RenderedCubeMap_Construct(static_cast<NiRenderedCubeMap*>(memory)) : nullptr;
    cube->pixelLayout = prefs[0];
    cube->alphaFormat = prefs[1];
    cube->mipMapped = prefs[2];
    for (int i = 0; i < 6; i++) {
        Ni2DBuffer* face = Ni2DBuffer_Create(size, size);
        Ni2DBuffer* old = cube->faces[i];
        if (old != face) {
            Unref(old);
            cube->faces[i] = face;
            AddRef(face);
        }
    }
    if (Virtual<uint8_t>(renderer, 70, static_cast<void*>(cube)) != 0) {
        return cube;
    }
    Virtual<void>(cube, 0, 1u);
    return nullptr;
}

// 0x7D3260 vf0
NiRenderedCubeMap* __fastcall CubeMap_DeletingDestructorImpl(NiRenderedCubeMap* self, re::Edx, uint32_t flags) {
    RenderedCubeMap_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x50);
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NI2DBUFFER_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NI2DBUFFER_REVERSED("NiObjectGroup::NiObjectGroup", 0x007D1C90, ni::ObjectGroup_ConstructImpl);
NI2DBUFFER_REVERSED("NiObjectGroup::~NiObjectGroup", 0x007D1CE0, ni::ObjectGroup_DestructImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::SetDimensions", 0x007D1CF0, ni::SetDimensionsImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::RegisterStreamables", 0x007D1D10, ni::RegisterStreamablesImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::FastCopy", 0x007D1D30, ni::FastCopyImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::Copy", 0x007D1D80, ni::CopyImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::Ni2DBuffer", 0x007D1DD0, ni::ConstructImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::GetRTTI", 0x007D1DF0, ni::GetRTTIImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::~Ni2DBuffer", 0x007D1E00, ni::DestructImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::Create", 0x007D1E40, ni::CreateImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::CreateWithRendererData", 0x007D1EA0, ni::CreateWithDataImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::DeletingDestructor", 0x007D1F10, ni::DeletingDestructorImpl);
NI2DBUFFER_REVERSED("Ni2DBuffer::GetViewerStrings", 0x007D1F60, ni::GetViewerStringsImpl);
NI2DBUFFER_REVERSED("NiRenderedCubeMap::GetWidth", 0x007D3060, ni::CubeMap_GetWidthImpl);
NI2DBUFFER_REVERSED("NiRenderedCubeMap::GetHeight", 0x007D3070, ni::CubeMap_GetHeightImpl);
NI2DBUFFER_REVERSED("NiRenderedCubeMap::NiRenderedCubeMap", 0x007D3080, ni::CubeMap_ConstructImpl);
NI2DBUFFER_REVERSED("NiRenderedCubeMap::GetRTTI", 0x007D30E0, ni::CubeMap_GetRTTIImpl);
NI2DBUFFER_REVERSED("NiRenderedCubeMap::~NiRenderedCubeMap", 0x007D30F0, ni::CubeMap_DestructImpl);
NI2DBUFFER_REVERSED("NiRenderedCubeMap::Create", 0x007D3170, ni::CubeMap_CreateImpl);
NI2DBUFFER_REVERSED("NiRenderedCubeMap::DeletingDestructor", 0x007D3260, ni::CubeMap_DeletingDestructorImpl);
