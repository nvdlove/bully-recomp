// NiPersistentSrcTextureRendererData (vtable 0x9613A0, 0x8C bytes): a source texture's pixels
// kept in the renderer's format, streamable. On NiTexture::RendererData (vtable 0x961358,
// 0x58 bytes: its NiPixelFormat at +0x14). Its pixels like NiPixelData's (+0x58 the block,
// widths, heights, offsets; +0x6C the mipmap levels, +0x74 the faces), +0x78 the target
// platform, +0x7C.. a platform-specific copy (owned while ms_bFreePlatformData, 0x20E3E10),
// +0x88 its palette (a reference). Slots 18 / 19 (AllocateData / FreeData) shared with
// NiDX9PersistentSrcTextureRendererData. Also NiPixelFormat's GetPalettized and GetBits.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiPixelData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiPersistentSrcTextureRendererData_RTTI, 0x020E3E14);
GAME_VAR(uint32_t, kNiPersistentSrcTextureRendererDataClassIndex, 0x020E3E1C);
GAME_VAR(uint8_t, ms_bFreePlatformData, 0x020E3E10);
inline void* const kCreateObject = reinterpret_cast<void*>(0x007DD580);

constexpr uintptr_t kVtable = 0x009613A0;
constexpr uintptr_t kRendererDataVtable = 0x00961358;   // NiTexture::RendererData

struct NiPersistentSrcTextureRendererData : NiObject {
    re::Ptr32<void> texture;              // +0x8
    uint32_t width;             // +0xC
    uint32_t height;            // +0x10
    NiPixelFormat format;       // +0x14
    re::Ptr32<uint8_t> pixels;            // +0x58
    re::Ptr32<uint32_t> widths;           // +0x5C
    re::Ptr32<uint32_t> heights;          // +0x60
    re::Ptr32<uint32_t> offsets;          // +0x64 (levels + 1)
    uint32_t padOffset;         // +0x68
    uint32_t mipmapLevels;      // +0x6C
    uint32_t pixelStride;       // +0x70
    uint32_t faces;             // +0x74
    uint32_t targetPlatform;    // +0x78 (0 any, 1 Xenon, 2 PS3, 3 DX9)
    re::Ptr32<uint8_t> platformPixels;    // +0x7C
    uint32_t platformFaceSize;  // +0x80
    uint32_t platformUnknown84; // +0x84
    re::Ptr32<NiObject> palette;          // +0x88 (NiPalettePtr)
};
static_assert(offsetof(NiPersistentSrcTextureRendererData, format) == 0x14);
static_assert(offsetof(NiPersistentSrcTextureRendererData, pixels) == 0x58);
static_assert(offsetof(NiPersistentSrcTextureRendererData, targetPlatform) == 0x78);
static_assert(sizeof(NiPersistentSrcTextureRendererData) == 0x8C);

GAME_FN(void, __thiscall, Object_LinkObject, 0x008A20A0, void* self, void* stream);   // (does nothing)
GAME_FN(void, __thiscall, Object_SaveBinary, 0x008A20A0, void* self, void* stream);
GAME_FN(void, __thiscall, PixelFormat_SaveBinary, 0x0076B870, const NiPixelFormat* self, void* stream);
GAME_FN(uint8_t, __thiscall, PixelFormat_NotEquals, 0x0076A090, const NiPixelFormat* self,
        const NiPixelFormat* other);
GAME_FN(uint32_t, __cdecl, Stream_RegisterLoader, 0x00759EC0, const char* name, void* create);
GAME_FN(uint32_t, __cdecl, Stream_UnregisterLoader, 0x00759EE0, const char* name);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, PersistentData_Destruct, 0x007DD4E0, NiPersistentSrcTextureRendererData* self);

inline void Unref(NiObject* object) {
    if (object != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(&object->refCount)) == 0) {
        Virtual<void>(object, 1);
    }
}

inline void SaveRaw(void* stream, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// 0x7DD2F0 vf18 AllocateData(levels, faces, bytesPerFace): as NiPixelData's
void __fastcall AllocateDataImpl(NiPersistentSrcTextureRendererData* self, re::Edx, uint32_t levels,
                                 uint32_t faces, uint32_t bytes) {
    uint32_t pixelBytes = faces * ((bytes + 3) & 0xFFFFFFFC);
    uint8_t* block = static_cast<uint8_t*>(NiMalloc(pixelBytes + levels * 0xC + 4));
    self->pixels = block;
    self->widths = reinterpret_cast<uint32_t*>(block + pixelBytes);
    self->heights = self->widths + levels;
    self->offsets = self->heights + levels;
}

// 0x7DD340 vf19 FreeData
void __fastcall FreeDataImpl(NiPersistentSrcTextureRendererData* self, re::Edx) {
    if (self->pixels != nullptr) {
        NiFree(self->pixels);
        self->offsets = nullptr;
        self->heights = nullptr;
        self->widths = nullptr;
        self->pixels = nullptr;
    }
}

// 0x7DD370 NiPixelFormat::GetPalettized: PAL or PALALPHA
uint32_t __fastcall PixelFormat_GetPalettizedImpl(const NiPixelFormat* self, re::Edx) {
    return (self->format == 2 || self->format == 3) ? 1 : 0;
}

// 0x7DD390 NiPixelFormat::GetBits(component): the bits of the first channel of that kind (none: 0)
uint8_t __fastcall PixelFormat_GetBitsImpl(const NiPixelFormat* self, re::Edx, uint32_t component) {
    for (uint32_t i = 0; i < 4; i++) {
        if (self->components[i].component == component) {
            return self->components[i].bitsPerComponent;
        }
    }
    return 0;
}

// 0x7DD3C0 vf5 LinkObject(stream): the palette
void __fastcall LinkObjectImpl(NiPersistentSrcTextureRendererData* self, re::Edx, void* stream) {
    Object_LinkObject(self, stream);
    NiObject* palette = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
    NiObject* old = self->palette;
    if (old != palette) {
        Unref(old);
        self->palette = palette;
        AddRef(palette);
    }
}

// 0x7DD420 vf6 RegisterStreamables
uint8_t __fastcall RegisterStreamablesImpl(NiPersistentSrcTextureRendererData* self, re::Edx, void* stream) {
    if (Object_RegisterStreamables(self, stream) == 0) {
        return 0;
    }
    if (self->palette != nullptr) {
        Virtual<uint8_t>(self->palette, 6, stream);
    }
    return 1;
}

// 0x7DD450 NiTexture::RendererData's deleting destructor
NiObject* __fastcall RendererData_DeletingDestructorImpl(NiObject* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kRendererDataVtable);
    Object_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x58);
    }
    return self;
}

// 0x7DD4E0 (the palette let go twice: the second time it's already none)
void __fastcall DestructImpl(NiPersistentSrcTextureRendererData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kVtable);
    if (self->palette != nullptr) {
        Unref(self->palette);
        self->palette = nullptr;
    }
    if (self->pixels != nullptr) {
        NiFree(self->pixels);
        self->offsets = nullptr;
        self->heights = nullptr;
        self->widths = nullptr;
        self->pixels = nullptr;
    }
    if (ms_bFreePlatformData != 0 && self->platformPixels != nullptr) {
        NiFree(self->platformPixels);
    }
    Unref(self->palette);
    self->vtable = reinterpret_cast<void*>(kRendererDataVtable);
    Object_Destruct(self);
}

// 0x7DD5A0 vf7 SaveBinary(stream): the platform copy when it's owned and complete, else the
// pixels (the last offset: a face's size)
void __fastcall SaveBinaryImpl(NiPersistentSrcTextureRendererData* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    uint8_t platform = ms_bFreePlatformData != 0 && self->platformFaceSize != 0 &&
                               self->platformUnknown84 != 0 && self->platformPixels != nullptr
                           ? 1
                           : 0;
    PixelFormat_SaveBinary(&self->format, stream);
    Virtual<void>(stream, 12, static_cast<void*>(self->palette));   // SaveLinkID
    SaveRaw(stream, &self->mipmapLevels, 4, 4);
    SaveRaw(stream, &self->pixelStride, 4, 4);
    for (uint32_t i = 0; i < self->mipmapLevels; i++) {
        SaveRaw(stream, &self->widths[i], 4, 4);
        SaveRaw(stream, &self->heights[i], 4, 4);
        SaveRaw(stream, &self->offsets[i], 4, 4);
    }
    if (platform != 0) {
        SaveRaw(stream, &self->platformFaceSize, 4, 4);
        SaveRaw(stream, &self->platformUnknown84, 4, 4);
    } else {
        SaveRaw(stream, &self->offsets[self->mipmapLevels], 4, 4);
        SaveRaw(stream, &self->padOffset, 4, 4);
    }
    SaveRaw(stream, &self->faces, 4, 4);
    SaveRaw(stream, &self->targetPlatform, 4, 4);
    if (platform != 0) {
        SaveRaw(stream, self->platformPixels, self->platformFaceSize * self->faces, 1);
        return;
    }
    SaveRaw(stream, self->pixels, self->offsets[self->mipmapLevels] * self->faces, 1);
}

// 0x7DD820 vf8 IsEqual(other): of its class, NiObject's, the format, palettes, sizes, then (the
// game's) only the first face's pixels compared
uint8_t __fastcall IsEqualImpl(NiPersistentSrcTextureRendererData* self, re::Edx,
                               NiPersistentSrcTextureRendererData* other) {
    if (other == nullptr) {
        return 0;
    }
    const uint8_t* rtti = Virtual<const uint8_t*>(other, 2);
    uint32_t index = kNiPersistentSrcTextureRendererDataClassIndex;
    if ((At<uint32_t>(rtti, 0xC + (index >> 5) * 4) & (1u << (index & 0x1F))) == 0) {
        return 0;
    }
    if (Object_IsEqual(self, other) == 0 || PixelFormat_NotEquals(&self->format, &other->format) != 0) {
        return 0;
    }
    if (self->palette != nullptr && Virtual<uint8_t>(self->palette, 8, static_cast<void*>(other->palette)) == 0) {
        return 0;
    }
    if (self->mipmapLevels != other->mipmapLevels || self->pixelStride != other->pixelStride) {
        return 0;
    }
    for (uint32_t i = 0; i < self->mipmapLevels; i++) {
        if (self->widths[i] != other->widths[i] || self->heights[i] != other->heights[i] ||
            self->offsets[i] != other->offsets[i]) {
            return 0;
        }
    }
    uint32_t levels = self->mipmapLevels;
    if (self->offsets[levels] != other->offsets[levels] || self->padOffset != other->padOffset ||
        self->faces != other->faces || self->targetPlatform != other->targetPlatform) {
        return 0;
    }
    uint32_t bytes = self->offsets[levels];
    const uint8_t* a = self->pixels;
    const uint8_t* b = other->pixels;
    for (uint32_t i = 0; i < bytes; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

// 0x7DD9C0 (static) its loader registered again
void __cdecl RegisterLoaderImpl() {
    Stream_UnregisterLoader("NiPersistentSrcTextureRendererData");
    Stream_RegisterLoader("NiPersistentSrcTextureRendererData", kCreateObject);
}

// 0x7DD9E0 vf0
NiPersistentSrcTextureRendererData* __fastcall DeletingDestructorImpl(NiPersistentSrcTextureRendererData* self,
                                                                     re::Edx, uint32_t flags) {
    PersistentData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x8C);
    }
    return self;
}

// 0x7DDA10 vf9 GetViewerStrings
void __fastcall GetViewerStringsImpl(NiPersistentSrcTextureRendererData* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiPersistentSrcTextureRendererData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("Width", self->widths[0]));
    AddViewerString(strings, GetViewerStringUint("Height", self->heights[0]));
    AddViewerString(strings, GetViewerStringUint("MipmapLevels", self->mipmapLevels));
    AddViewerString(strings, GetViewerStringUint("Faces", self->faces));
    AddViewerString(strings,
                    GetViewerStringUint("TotalSizeInBytes", self->offsets[self->mipmapLevels] * self->faces));
    AddViewerString(strings, GetViewerStringUint("FaceSizeInBytes", self->offsets[self->mipmapLevels]));
    const char* platform;
    switch (self->targetPlatform) {
    case 0:
        platform = "NI_ANY - Generic (default)";
        break;
    case 1:
        platform = "NI_XENON";
        break;
    case 2:
        platform = "NI_PS3";
        break;
    case 3:
        platform = "NI_DX9";
        break;
    default: {
        char* text = GetViewerStringString("TargetPlatform", "UNKNOWN!");
        AddViewerString(strings, text);
        return;
    }
    }
    re::Ptr32<char> text = GetViewerStringString("TargetPlatform", platform);
    StringArray_Add(strings, &text);
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPERSISTENT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::AllocateData", 0x007DD2F0, ni::AllocateDataImpl);
NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::FreeData", 0x007DD340, ni::FreeDataImpl);
NIPERSISTENT_REVERSED("NiPixelFormat::GetPalettized", 0x007DD370, ni::PixelFormat_GetPalettizedImpl);
NIPERSISTENT_REVERSED("NiPixelFormat::GetBits", 0x007DD390, ni::PixelFormat_GetBitsImpl);
NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::LinkObject", 0x007DD3C0, ni::LinkObjectImpl);
NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::RegisterStreamables", 0x007DD420,
                      ni::RegisterStreamablesImpl);
NIPERSISTENT_REVERSED("NiTexture::RendererData::DeletingDestructor", 0x007DD450,
                      ni::RendererData_DeletingDestructorImpl);
NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::~NiPersistentSrcTextureRendererData", 0x007DD4E0,
                      ni::DestructImpl);
NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::SaveBinary", 0x007DD5A0, ni::SaveBinaryImpl);
NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::IsEqual", 0x007DD820, ni::IsEqualImpl);
NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::RegisterLoader", 0x007DD9C0, ni::RegisterLoaderImpl);
NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::DeletingDestructor", 0x007DD9E0,
                      ni::DeletingDestructorImpl);
NIPERSISTENT_REVERSED("NiPersistentSrcTextureRendererData::GetViewerStrings", 0x007DDA10,
                      ni::GetViewerStringsImpl);
