// NiImageConverter (vtable 0x9608AC, 0x438 bytes): the image converter (ms_spConverter
// 0x20E394C) with its NIF image reader inside (+0x8: slot 1 CanReadFile(extension), 2
// ReadFile(file, optDest), 3 ReadHeader(file, width, height, format, mipmap, faces)). Its
// slots: 2 ReadImageFileInfo, 3 ReadImageFile, 4 CanConvertPixelData, 5 ConvertPixelData (the
// base converter only copies between identical formats). ms_pcPlatformSubDirectory (0x20E3948)
// goes between a file's directory and name.
//
// NiPalette (vtable 0x9608C8, 0x24 bytes): its entries (RGBA bytes), on a global list under a
// critical section; the renderer's slot 72 makes its renderer data. Also NiPixelFormat's
// operator==, NiPixelData's SetPalette / AllocateData / FreeData and NiPoint2's streaming.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiFile.h"
#include "gamebryo/NiPixelData.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using ni::sys::At;
using ni::sys::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiPalette_RTTI, 0x020E3958);
GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
GAME_VAR(NiRefObject*, NiImageConverter_ms_spConverter, 0x020E394C);
GAME_VAR(char*, NiImageConverter_ms_pcPlatformSubDirectory, 0x020E3948);
GAME_VAR(void*, NiPalette_ms_pkHead, 0x020E3950);
GAME_VAR(void*, NiPalette_ms_pkTail, 0x020E3954);
inline sys::NiCriticalSection& NiPalette_ms_kCritSec = *reinterpret_cast<sys::NiCriticalSection*>(0x020E398C);

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kNiImageConverterVtable = 0x009608AC;
constexpr uintptr_t kNiPaletteVtable = 0x009608C8;

struct NiImageConverter : NiRefObject {
    uint8_t reader[0x430];   // +0x8 (NiNIFImageReader: its vtable, a critical section...)
};
static_assert(sizeof(NiImageConverter) == 0x438);

struct NiPalette : NiObject {
    uint8_t alpha;           // +0x8
    uint8_t pad9[3];
    uint32_t entryCount;     // +0xC
    uint32_t revisionID;     // +0x10
    re::Ptr32<uint32_t> entries;       // +0x14 (NiPaletteEntry: r, g, b, a; new[]'d)
    re::Ptr32<void> rendererData;      // +0x18 (deleted through its slot 0)
    re::Ptr32<NiPalette> prev;         // +0x1C (the global list)
    re::Ptr32<NiPalette> next;         // +0x20
};
static_assert(offsetof(NiPalette, entries) == 0x14);
static_assert(sizeof(NiPalette) == 0x24);

GAME_FN(void*, __cdecl, NiFile_GetFile, 0x00753380, const char* path, int mode, uint32_t bufferSize);
GAME_FN(NiFilename*, __thiscall, Filename_Construct, 0x007538C0, NiFilename* self, const char* path);
GAME_FN(uint8_t, __thiscall, Filename_GetFullPath, 0x00753AD0, NiFilename* self, char* out, uint32_t size);
GAME_FN(void, __thiscall, Reader_Construct, 0x007F75B0, void* self);
GAME_FN(void, __thiscall, Reader_Destruct, 0x007F7500, void* self);
GAME_FN(uint8_t, __thiscall, PixelFormat_NotEquals, 0x0076A090, const NiPixelFormat* self,
        const NiPixelFormat* other);
GAME_FN(NiPixelData*, __thiscall, PixelData_CopyConstruct, 0x0076A4A0, void* self, const NiPixelData* other);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<char>* self, const char* text);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, Object_SaveBinary, 0x008A20A0, void* self, void* stream);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, PixelFormat_Equals, 0x007D3AB0, const NiPixelFormat* self, const NiPixelFormat* other);
GAME_FN(void, __thiscall, PixelData_SetPalette, 0x007D3E40, NiPixelData* self, NiObject* palette);
GAME_FN(void, __thiscall, ImageConverter_Destruct, 0x007D3B80, NiImageConverter* self);
GAME_FN(NiPalette*, __thiscall, Palette_CopyConstruct, 0x007D4560, void* self, const NiPalette* other);
GAME_FN(NiPalette*, __thiscall, Palette_Assign, 0x007D4430, NiPalette* self, const NiPalette* other);
GAME_FN(void, __thiscall, Palette_AddToList, 0x007D4320, NiPalette* self);
GAME_FN(void, __thiscall, Palette_RemoveFromList, 0x007D43A0, NiPalette* self);

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

inline void AddRefImport(NiRefObject* object) {
    if (object != nullptr) {
        sys::Increment(&object->refCount);
    }
}

// new NiPaletteEntry[count] (a size that overflows: -1)
inline uint32_t* NewEntries(uint32_t count) {
    uint64_t bytes = uint64_t(count) * 4;
    return static_cast<uint32_t*>(NiNewArray((bytes >> 32) != 0 ? 0xFFFFFFFF : uint32_t(bytes)));
}

// a stream read / write of `bytes` as one component of `componentSize`
inline void LoadRaw(void* stream, void* buffer, uint32_t bytes, uint32_t componentSize) {
    scene::BinaryStream* in = At<scene::BinaryStream*>(stream, 0x188);
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}
inline void SaveRaw(void* stream, const void* buffer, uint32_t bytes, uint32_t componentSize) {
    scene::BinaryStream* out = At<scene::BinaryStream*>(stream, 0x18C);
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// ---- NiPoint2

// 0x7D39F0 LoadBinary(stream)
void __fastcall Point2_LoadBinaryImpl(float* self, re::Edx, void* stream) {
    LoadRaw(stream, &self[0], 4, 4);
    LoadRaw(stream, &self[1], 4, 4);
}

// 0x7D3A50 SaveBinary(stream)
void __fastcall Point2_SaveBinaryImpl(const float* self, re::Edx, void* stream) {
    SaveRaw(stream, &self[0], 4, 4);
    SaveRaw(stream, &self[1], 4, 4);
}

// ---- NiPixelFormat

// 0x7D3AB0 operator==: the renderer hint -1 matches any
uint8_t __fastcall PixelFormat_EqualsImpl(const NiPixelFormat* self, re::Edx, const NiPixelFormat* other) {
    if (other->format != self->format || other->flags != self->flags || other->tiling != self->tiling ||
        other->bitsPerPixel != self->bitsPerPixel) {
        return 0;
    }
    uint32_t hint = self->rendererHint;
    if (other->rendererHint != hint && hint != 0xFFFFFFFF && other->rendererHint != 0xFFFFFFFF) {
        return 0;
    }
    for (int i = 0; i < 4; i++) {
        const NiPixelComponent& a = other->components[i];
        const NiPixelComponent& b = self->components[i];
        if (a.component != b.component || a.representation != b.representation ||
            a.bitsPerComponent != b.bitsPerComponent || a.isSigned != b.isSigned) {
            return 0;
        }
    }
    return 1;
}

// ---- NiImageConverter

// 0x7D3B40 GetImageConverter
NiRefObject* __cdecl GetImageConverterImpl() {
    return NiImageConverter_ms_spConverter;
}

// 0x7D3B50
NiImageConverter* __fastcall ImageConverter_ConstructImpl(NiImageConverter* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    sys::Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiImageConverterVtable);
    Reader_Construct(self->reader);
    return self;
}

// 0x7D3B80 (the platform subdirectory freed too)
void __fastcall ImageConverter_DestructImpl(NiImageConverter* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiImageConverterVtable);
    if (NiImageConverter_ms_pcPlatformSubDirectory != nullptr) {
        NiFree(NiImageConverter_ms_pcPlatformSubDirectory);
    }
    NiImageConverter_ms_pcPlatformSubDirectory = nullptr;
    Reader_Destruct(self->reader);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    sys::Decrement(&ms_uiObjects);
}

// 0x7D3BC0 vf2 ReadImageFileInfo(filename, format, mipmap, width, height, faces): the reader's
// header read, when it reads the extension and the file opens
uint8_t __fastcall ReadImageFileInfoImpl(NiImageConverter* self, re::Edx, const char* filename, void* format,
                                         void* mipmap, void* width, void* height, void* faces) {
    NiFilename name;
    Filename_Construct(&name, filename);
    void* reader = self->reader;
    if (Virtual<uint8_t>(reader, 1, static_cast<const char*>(name.ext)) == 0) {
        return 0;
    }
    void* file = NiFile_GetFile(filename, 0, 0x8000);
    if (file == nullptr) {
        return 0;
    }
    if (Virtual<uint8_t>(file, 1) == 0) {
        Virtual<void>(file, 0, 1u);
        return 0;
    }
    uint8_t result = Virtual<uint8_t>(reader, 3, file, width, height, format, mipmap, faces);
    Virtual<void>(file, 0, 1u);
    return result != 0 ? 1 : 0;
}

// 0x7D3CB0 vf3 ReadImageFile(filename, optDest): the reader's pixel data
void* __fastcall ReadImageFileImpl(NiImageConverter* self, re::Edx, const char* filename, void* optDest) {
    NiFilename name;
    Filename_Construct(&name, filename);
    void* reader = self->reader;
    if (Virtual<uint8_t>(reader, 1, static_cast<const char*>(name.ext)) == 0) {
        return nullptr;
    }
    void* file = NiFile_GetFile(filename, 0, 0x8000);
    if (file == nullptr) {
        return nullptr;
    }
    if (Virtual<uint8_t>(file, 1) == 0) {
        Virtual<void>(file, 0, 1u);
        return nullptr;
    }
    void* data = Virtual<void*>(reader, 2, file, optDest);
    Virtual<void>(file, 0, 1u);
    return data;
}

// 0x7D3D70 vf4 CanConvertPixelData(source, dest): only the same format
uint8_t __fastcall CanConvertPixelDataImpl(NiImageConverter*, re::Edx, const NiPixelFormat* source,
                                           const NiPixelFormat* dest) {
    return PixelFormat_Equals(source, dest);
}

// 0x7D3E40 NiPixelData::SetPalette(palette)
void __fastcall PixelData_SetPaletteImpl(NiPixelData* self, re::Edx, NiObject* palette) {
    NiObject* old = self->palette;
    if (old != palette) {
        ReleaseRef(old);
        self->palette = palette;
        AddRefImport(palette);
    }
}

// 0x7D3F40 SetImageConverter(converter)
void __cdecl SetImageConverterImpl(NiRefObject* converter) {
    NiRefObject* old = NiImageConverter_ms_spConverter;
    if (old != converter) {
        ReleaseRef(old);
        NiImageConverter_ms_spConverter = converter;
        AddRefImport(converter);
    }
}

// 0x7D3F90 vf0
NiImageConverter* __fastcall ImageConverter_DeletingDestructorImpl(NiImageConverter* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kNiImageConverterVtable);
    if (NiImageConverter_ms_pcPlatformSubDirectory != nullptr) {
        NiFree(NiImageConverter_ms_pcPlatformSubDirectory);
    }
    NiImageConverter_ms_pcPlatformSubDirectory = nullptr;
    Reader_Destruct(self->reader);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    sys::Decrement(&ms_uiObjects);
    if (flags & 1) {
        NiDelete(self, 0x438);
    }
    return self;
}

// 0x7D3FF0 vf5 ConvertPixelData(source, format, optDest, mipmap): none when the format differs;
// the pixels copied into optDest when it matches (format, top level size, no more levels), its
// palette made or copied; otherwise a new copy of the source
NiPixelData* __fastcall ConvertPixelDataImpl(NiImageConverter*, re::Edx, NiPixelData* source,
                                             const NiPixelFormat* format, NiPixelData* dest, int) {
    if (PixelFormat_NotEquals(&source->format, format) != 0) {
        return nullptr;
    }
    if (dest != nullptr) {
        if (dest == source) {
            return dest;
        }
        if (PixelFormat_Equals(&dest->format, format) != 0 && dest->widths[0] == source->widths[0] &&
            dest->heights[0] == source->heights[0] && dest->mipmapLevels <= source->mipmapLevels) {
            uint32_t levels = dest->mipmapLevels;
            for (uint32_t i = 0; i < levels; i++) {
                crt::MemcpyS(dest->pixels + dest->offsets[i], dest->offsets[i + 1] - dest->offsets[i],
                             source->pixels + source->offsets[i], source->offsets[i + 1] - source->offsets[i]);
            }
            if (source->palette != nullptr) {
                if (dest->palette == nullptr) {
                    void* memory = NiNew(0x24);
                    if (memory == nullptr) {
                        PixelData_SetPalette(dest, nullptr);
                        return dest;
                    }
                    NiPalette* palette =
                        Palette_CopyConstruct(memory, re::PtrCast<const NiPalette*>(source->palette));
                    PixelData_SetPalette(dest, palette);
                    return dest;
                }
                Palette_Assign(re::PtrCast<NiPalette*>(dest->palette),
                               re::PtrCast<const NiPalette*>(source->palette));
            }
            return dest;
        }
    }
    void* memory = NiNew(0x70);
    if (memory == nullptr) {
        return nullptr;
    }
    return PixelData_CopyConstruct(memory, source);
}

// 0x7D4130 ConvertFilenameToPlatformSpecific(filename, out): the platform subdirectory put in
// when there is one
void __cdecl ConvertFilenameToPlatformSpecificImpl(const re::Ptr32<char>* filename, re::Ptr32<char>* out) {
    if (NiImageConverter_ms_pcPlatformSubDirectory != nullptr) {
        NiFilename name;
        Filename_Construct(&name, *filename);
        crt::StrcpyS(name.platformSubDir, 0x100, NiImageConverter_ms_pcPlatformSubDirectory);
        char path[0x104];
        Filename_GetFullPath(&name, path, 0x104);
        FixedString_AssignText(out, path);
        return;
    }
    FixedString_Assign(out, filename);
}

// ---- NiPixelData

// 0x7D41E0 AllocateData(levels, faces, bytesPerFace): one block, the pixels then the widths,
// heights and offsets (levels + 1)
void __fastcall PixelData_AllocateDataImpl(NiPixelData* self, re::Edx, uint32_t levels, uint32_t faces,
                                           uint32_t bytes) {
    uint32_t pixelBytes = faces * ((bytes + 3) & 0xFFFFFFFC);
    uint8_t* block = static_cast<uint8_t*>(NiMalloc(pixelBytes + levels * 0xC + 4));
    self->pixels = block;
    self->widths = reinterpret_cast<uint32_t*>(block + pixelBytes);
    self->heights = self->widths + levels;
    self->offsets = self->heights + levels;
}

// 0x7D4230 FreeData
void __fastcall PixelData_FreeDataImpl(NiPixelData* self, re::Edx) {
    NiFree(self->pixels);
    self->offsets = nullptr;
    self->heights = nullptr;
    self->widths = nullptr;
    self->pixels = nullptr;
}

// ---- NiPalette

// 0x7D4250 vf8 IsEqual(other): NiObject's, the alpha flag and entry count, then (the game's
// quirk) only the first byte of the entries (the length a bool)
uint8_t __fastcall Palette_IsEqualImpl(NiPalette* self, re::Edx, NiPalette* other) {
    if (scene::Object_IsEqual(self, other) == 0 || self->alpha != other->alpha ||
        self->entryCount != other->entryCount) {
        return 0;
    }
    uint32_t bytes = (self->entryCount * 4) != 0 ? 1 : 0;
    const uint8_t* a = re::PtrCast<const uint8_t*>(self->entries);
    const uint8_t* b = re::PtrCast<const uint8_t*>(other->entries);
    for (uint32_t i = 0; i < bytes; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

// 0x7D4320 AddToList (at the tail)
void __fastcall Palette_AddToListImpl(NiPalette* self, re::Edx) {
    sys::Lock(NiPalette_ms_kCritSec);
    if (NiPalette_ms_pkHead == nullptr) {
        NiPalette_ms_pkHead = self;
    }
    NiPalette* tail = static_cast<NiPalette*>(NiPalette_ms_pkTail);
    if (tail != nullptr) {
        tail->next = self;
        self->prev = static_cast<NiPalette*>(NiPalette_ms_pkTail);
    } else {
        self->prev = nullptr;
    }
    NiPalette_ms_pkTail = self;
    self->next = nullptr;
    sys::Unlock(NiPalette_ms_kCritSec);
}

// 0x7D43A0 RemoveFromList
void __fastcall Palette_RemoveFromListImpl(NiPalette* self, re::Edx) {
    sys::Lock(NiPalette_ms_kCritSec);
    if (NiPalette_ms_pkHead == self) {
        NiPalette_ms_pkHead = self->next;
    }
    if (NiPalette_ms_pkTail == self) {
        NiPalette_ms_pkTail = self->prev;
    }
    if (self->prev != nullptr) {
        self->prev->next = self->next;
    }
    if (self->next != nullptr) {
        self->next->prev = self->prev;
    }
    sys::Unlock(NiPalette_ms_kCritSec);
}

// 0x7D4420 vf2
const RTTI* __fastcall Palette_GetRTTIImpl(NiPalette*, re::Edx) {
    return &NiPalette_RTTI;
}

// 0x7D4430 operator=(other): its renderer data dropped, the entries copied (reallocated when the
// count differs), the revision 1
NiPalette* __fastcall Palette_AssignImpl(NiPalette* self, re::Edx, const NiPalette* other) {
    if (self == other) {
        return self;
    }
    if (self->rendererData != nullptr) {
        Virtual<void>(self->rendererData, 0, 1u);
        self->rendererData = nullptr;
    }
    self->alpha = other->alpha;
    self->revisionID = 1;
    if (self->entryCount != other->entryCount) {
        NiDeleteArray(self->entries, 4);
        self->entryCount = other->entryCount;
        self->entries = NewEntries(self->entryCount);
    }
    crt::MemcpyS(self->entries, self->entryCount * 4, other->entries, other->entryCount * 4);
    return self;
}

// 0x7D4510 vf0
NiPalette* __fastcall Palette_DeletingDestructorImpl(NiPalette* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kNiPaletteVtable);
    NiDeleteArray(self->entries, 4);
    if (self->rendererData != nullptr) {
        Virtual<void>(self->rendererData, 0, 1u);
    }
    Palette_RemoveFromList(self);
    scene::Object_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x24);
    }
    return self;
}

// 0x7D4560 NiPalette(other)
NiPalette* __fastcall Palette_CopyConstructImpl(NiPalette* self, re::Edx, const NiPalette* other) {
    scene::RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNiPaletteVtable);
    self->alpha = other->alpha;
    self->entryCount = other->entryCount;
    self->revisionID = 1;
    self->entries = NewEntries(self->entryCount);
    crt::MemcpyS(self->entries, self->entryCount * 4, other->entries, other->entryCount * 4);
    self->rendererData = nullptr;
    if (NiRenderer_ms_pkRenderer != nullptr) {
        Virtual<void>(NiRenderer_ms_pkRenderer, 72, static_cast<void*>(self));
    }
    Palette_AddToList(self);
    return self;
}

// 0x7D45F0 NiPalette(alpha, count, entries)
NiPalette* __fastcall Palette_ConstructImpl(NiPalette* self, re::Edx, int alpha, uint32_t count,
                                            const uint32_t* entries) {
    scene::RefObject_Construct(self);
    self->alpha = uint8_t(alpha);
    self->entryCount = count;
    self->vtable = reinterpret_cast<void*>(kNiPaletteVtable);
    self->revisionID = 1;
    self->entries = NewEntries(count);
    crt::MemcpyS(self->entries, self->entryCount * 4, entries, self->entryCount * 4);
    self->rendererData = nullptr;
    if (NiRenderer_ms_pkRenderer != nullptr) {
        Virtual<void>(NiRenderer_ms_pkRenderer, 72, static_cast<void*>(self));
    }
    Palette_AddToList(self);
    return self;
}

// 0x7D4670 vf4 LoadBinary(stream)
void __fastcall Palette_LoadBinaryImpl(NiPalette* self, re::Edx, void* stream) {
    scene::Object_LoadBinary(self, stream);
    uint8_t alpha;
    LoadRaw(stream, &alpha, 1, 1);
    self->alpha = alpha != 0 ? 1 : 0;
    uint32_t count;
    LoadRaw(stream, &count, 4, 4);
    if (count != self->entryCount) {
        NiDeleteArray(self->entries, 4);
        self->entryCount = count;
        self->entries = NewEntries(count);
    }
    LoadRaw(stream, self->entries, self->entryCount * 4, 1);
    if (NiRenderer_ms_pkRenderer != nullptr) {
        Virtual<void>(NiRenderer_ms_pkRenderer, 72, static_cast<void*>(self));
    }
}

// 0x7D4760 vf7 SaveBinary(stream)
void __fastcall Palette_SaveBinaryImpl(NiPalette* self, re::Edx, void* stream) {
    Object_SaveBinary(self, stream);
    uint8_t alpha = self->alpha;
    SaveRaw(stream, &alpha, 1, 1);
    SaveRaw(stream, &self->entryCount, 4, 4);
    SaveRaw(stream, self->entries, self->entryCount * 4, 1);
}

// 0x7D4800 vf9 GetViewerStrings
void __fastcall Palette_GetViewerStringsImpl(NiPalette* self, re::Edx, NiTArray<char*>* strings) {
    scene::Object_GetViewerStrings(self, strings);
    scene::AddViewerString(strings, scene::GetViewerString(NiPalette_RTTI.name));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIIMAGECONV_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIIMAGECONV_REVERSED("NiPoint2::LoadBinary", 0x007D39F0, ni::Point2_LoadBinaryImpl);
NIIMAGECONV_REVERSED("NiPoint2::SaveBinary", 0x007D3A50, ni::Point2_SaveBinaryImpl);
NIIMAGECONV_REVERSED("NiPixelFormat::operator==_7D3AB0", 0x007D3AB0, ni::PixelFormat_EqualsImpl);
NIIMAGECONV_REVERSED("NiImageConverter::GetImageConverter", 0x007D3B40, ni::GetImageConverterImpl);
NIIMAGECONV_REVERSED("NiImageConverter::NiImageConverter", 0x007D3B50, ni::ImageConverter_ConstructImpl);
NIIMAGECONV_REVERSED("NiImageConverter::~NiImageConverter", 0x007D3B80, ni::ImageConverter_DestructImpl);
NIIMAGECONV_REVERSED("NiImageConverter::ReadImageFileInfo", 0x007D3BC0, ni::ReadImageFileInfoImpl);
NIIMAGECONV_REVERSED("NiImageConverter::ReadImageFile", 0x007D3CB0, ni::ReadImageFileImpl);
NIIMAGECONV_REVERSED("NiImageConverter::CanConvertPixelData", 0x007D3D70, ni::CanConvertPixelDataImpl);
NIIMAGECONV_REVERSED("NiPixelData::SetPalette", 0x007D3E40, ni::PixelData_SetPaletteImpl);
NIIMAGECONV_REVERSED("NiImageConverter::SetImageConverter", 0x007D3F40, ni::SetImageConverterImpl);
NIIMAGECONV_REVERSED("NiImageConverter::DeletingDestructor", 0x007D3F90, ni::ImageConverter_DeletingDestructorImpl);
NIIMAGECONV_REVERSED("NiImageConverter::ConvertPixelData", 0x007D3FF0, ni::ConvertPixelDataImpl);
NIIMAGECONV_REVERSED("NiImageConverter::ConvertFilenameToPlatformSpecific", 0x007D4130,
                     ni::ConvertFilenameToPlatformSpecificImpl);
NIIMAGECONV_REVERSED("NiPixelData::AllocateData", 0x007D41E0, ni::PixelData_AllocateDataImpl);
NIIMAGECONV_REVERSED("NiPixelData::FreeData", 0x007D4230, ni::PixelData_FreeDataImpl);
NIIMAGECONV_REVERSED("NiPalette::IsEqual", 0x007D4250, ni::Palette_IsEqualImpl);
NIIMAGECONV_REVERSED("NiPalette::AddToList", 0x007D4320, ni::Palette_AddToListImpl);
NIIMAGECONV_REVERSED("NiPalette::RemoveFromList", 0x007D43A0, ni::Palette_RemoveFromListImpl);
NIIMAGECONV_REVERSED("NiPalette::GetRTTI", 0x007D4420, ni::Palette_GetRTTIImpl);
NIIMAGECONV_REVERSED("NiPalette::operator=", 0x007D4430, ni::Palette_AssignImpl);
NIIMAGECONV_REVERSED("NiPalette::DeletingDestructor", 0x007D4510, ni::Palette_DeletingDestructorImpl);
NIIMAGECONV_REVERSED("NiPalette::NiPalette(const&)", 0x007D4560, ni::Palette_CopyConstructImpl);
NIIMAGECONV_REVERSED("NiPalette::NiPalette", 0x007D45F0, ni::Palette_ConstructImpl);
NIIMAGECONV_REVERSED("NiPalette::LoadBinary", 0x007D4670, ni::Palette_LoadBinaryImpl);
NIIMAGECONV_REVERSED("NiPalette::SaveBinary", 0x007D4760, ni::Palette_SaveBinaryImpl);
NIIMAGECONV_REVERSED("NiPalette::GetViewerStrings", 0x007D4800, ni::Palette_GetViewerStringsImpl);
