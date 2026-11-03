// NiSourceTexture (src/gamebryo/NiSourceTexture.h): creation from a file or from pixel data,
// the renderer data, the size, streaming (the file looked for along the stream's search path,
// textures shared through the stream's texture map), comparison, destruction.

#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSourceTexture.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiSourceTexture_RTTI, 0x020E2A10);

constexpr uintptr_t NiTexture_vtable = 0x009519B8;
constexpr uintptr_t NiSourceTexture_vtable = 0x00952560;

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
GAME_VAR(uint8_t, NiSourceTexture_ms_bPreload, 0x00BC7620);
GAME_VAR(uint8_t, NiSourceTexture_ms_bDestroyAppDataFlag, 0x00BC7622);

GAME_FN(void, __thiscall, Texture_LoadBinary, 0x007548C0, void* self, void* stream);
GAME_FN(void, __thiscall, Texture_LinkObject, 0x007548D0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Texture_RegisterStreamables, 0x007654D0, void* self, void* stream);
GAME_FN(void, __thiscall, Texture_SaveBinary, 0x007654E0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Texture_IsEqual, 0x007654F0, void* self, void* other);
GAME_FN(void, __thiscall, Texture_GetViewerStrings, 0x007551A0, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Texture_AddToList, 0x0075D7D0, NiTexture* self);
GAME_FN(void, __thiscall, Texture_RemoveFromList, 0x0075D850, NiTexture* self);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* slot, NiObject* value);   // NiPointer = T*
GAME_FN(void, __thiscall, Pointer_AssignPointer, 0x00518100, re::Ptr32<NiObject>* slot, const re::Ptr32<NiObject>* value);
GAME_FN(NiObject*, __thiscall, Stream_GetTexture, 0x0075A3E0, void* stream, const char* name, NiObject* texture);
GAME_FN(void, __thiscall, Stream_SetTexture, 0x0075A3D0, void* stream, const char* name, NiObject* texture);
GAME_FN(void, __thiscall, SearchPath_SetFilePath, 0x008C5400, void* searchPath, const char* path);
GAME_FN(uint8_t, __cdecl, NiFile_Access, 0x007533C0, const char* path, int mode);
GAME_FN(void, __cdecl, NiPath_Standardize, 0x00752C20, char* path);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignCString, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(void, __cdecl, ConvertToPlatformSpecificFilename, 0x007D4130, const re::Ptr32<const char>* filename,
        re::Ptr32<const char>* platformFilename);
GAME_FN(void*, __cdecl, ImageConverter_Get, 0x007D3B40);   // NiImageConverter::GetImageConverter
// (this chunk's, called where modes apply)
GAME_FN(void, __cdecl, SourceTexture_StandardizeFilename, 0x00769740, re::Ptr32<const char>* filename);
GAME_FN(NiSourceTexture*, __thiscall, SourceTexture_Construct, 0x007697A0, void* self);
GAME_FN(void, __thiscall, SourceTexture_Destruct, 0x00769810, NiSourceTexture* self);

// NiPointer::operator=(T*) as inlined here: the old one let go first
inline void AssignReleaseFirst(re::Ptr32<NiObject>& slot, NiObject* value) {
    NiObject* old = slot;
    if (old != value) {
        Release(old);
        slot = value;
        AddRef(value);
    }
}

// vf22 CreateRendererData: the renderer's (slot 67); the application's pixel data then dropped
// (slot 21) when the flag says so and the texture is static
uint8_t __fastcall CreateRendererData(NiSourceTexture* self, re::Edx) {
    void* renderer = NiRenderer_ms_pkRenderer;
    if (renderer != nullptr && !Virtual<uint8_t>(renderer, 67, self)) {
        return 0;
    }
    if (NiSourceTexture_ms_bDestroyAppDataFlag != 0 && self->rendererData != nullptr && self->isStatic != 0) {
        Virtual<void>(self, 21);
    }
    return 1;
}

// vf18 GetWidth: the renderer data's, else the source's top level
uint32_t __fastcall GetWidth(NiSourceTexture* self, re::Edx) {
    if (self->rendererData != nullptr) {
        return At<uint32_t>(self->rendererData, 0xC);
    }
    if (self->persistentSrcRendererData != nullptr) {
        return At<uint32_t*>(self->persistentSrcRendererData, 0x5C)[0];
    }
    if (self->srcPixelData != nullptr) {
        return At<uint32_t*>(self->srcPixelData, 0x54)[0];
    }
    return 0;
}

// vf19 GetHeight
uint32_t __fastcall GetHeight(NiSourceTexture* self, re::Edx) {
    if (self->rendererData != nullptr) {
        return At<uint32_t>(self->rendererData, 0x10);
    }
    if (self->persistentSrcRendererData != nullptr) {
        return At<uint32_t*>(self->persistentSrcRendererData, 0x60)[0];
    }
    if (self->srcPixelData != nullptr) {
        return At<uint32_t*>(self->srcPixelData, 0x58)[0];
    }
    return 0;
}

// vf5 LinkObject: the linked source is the persistent renderer data (20.2.0.4 on, when the
// flag says so) or the pixel data; the other let go
void __fastcall LinkObject(NiSourceTexture* self, re::Edx, void* stream) {
    Texture_LinkObject(self, stream);
    if (Stream_GetNumberOfLinkIDs(stream) == 0) {
        return;
    }
    if (StreamVersion(stream) >= 0x14020004 && self->srcRendererDataIsPersistent != 0) {
        Pointer_Assign(&self->persistentSrcRendererData, static_cast<NiObject*>(Stream_ResolveLinkID(stream)));
        if (self->srcPixelData != nullptr) {
            Release(self->srcPixelData);
            self->srcPixelData = nullptr;
        }
    } else {
        Pointer_Assign(&self->srcPixelData, static_cast<NiObject*>(Stream_ResolveLinkID(stream)));
        if (self->persistentSrcRendererData != nullptr) {
            Release(self->persistentSrcRendererData);
            self->persistentSrcRendererData = nullptr;
        }
    }
}

// vf6 RegisterStreamables: the file name, then the source object
uint8_t __fastcall RegisterStreamables(NiSourceTexture* self, re::Edx, void* stream) {
    if (!Texture_RegisterStreamables(self, stream)) {
        return 0;
    }
    Virtual<uint8_t>(stream, 8, &self->filename);   // RegisterFixedString
    NiObject* source = self->persistentSrcRendererData;
    if (source == nullptr) {
        source = self->srcPixelData;
        if (source == nullptr) {
            return 1;
        }
    }
    return Virtual<uint8_t>(source, 6, stream);
}

// both null, or both there and equal (their slot 8)
inline bool SourcesEqual(NiObject* a, NiObject* b) {
    if ((a == nullptr) != (b == nullptr)) {
        return false;
    }
    return a == nullptr || Virtual<uint8_t>(a, 8, b) != 0;
}

// vf8 IsEqual
uint8_t __fastcall IsEqual(NiSourceTexture* self, re::Edx, NiSourceTexture* other) {
    if (!Texture_IsEqual(self, other)) {
        return 0;
    }
    if (self->filename != other->filename) {
        return 0;
    }
    if (!SourcesEqual(self->srcPixelData, other->srcPixelData) ||
        !SourcesEqual(self->persistentSrcRendererData, other->persistentSrcRendererData)) {
        return 0;
    }
    if (self->pixelLayout != other->pixelLayout || self->mipMapped != other->mipMapped ||
        self->alphaFormat != other->alphaFormat || self->isStatic != other->isStatic ||
        self->loadDirectToRendererHint != other->loadDirectToRendererHint ||
        self->srcRendererDataIsPersistent != other->srcRendererDataIsPersistent) {
        return 0;
    }
    return 1;
}

// the file name in the library's form (NiPath's standardising), through a copy
void __cdecl StandardizeFilename(re::Ptr32<const char>* filename) {
    char path[0x104];
    crt::StrcpyS(path, 0x104, *filename);
    NiPath_Standardize(path);
    FixedString_AssignCString(filename, path);
}

NiSourceTexture* __fastcall Construct(NiSourceTexture* self, re::Edx) {
    ObjectNET_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTexture_vtable);
    self->pixelLayout = 6;
    self->alphaFormat = 3;
    self->mipMapped = 2;
    self->rendererData = nullptr;
    Texture_AddToList(self);
    self->vtable = reinterpret_cast<void*>(NiSourceTexture_vtable);
    self->filename = NullFixedString;
    self->platformFilename = NullFixedString;
    self->persistentSrcRendererData = nullptr;
    self->srcPixelData = nullptr;
    self->loadDirectToRendererHint = 0;
    self->srcRendererDataIsPersistent = 0;
    self->isStatic = 1;
    return self;
}

const RTTI* __fastcall GetRTTI(NiSourceTexture*, re::Edx) {
    return &NiSourceTexture_RTTI;
}

// ~NiSourceTexture (the sources let go and nulled, then their NiPointers' own destructors)
void __fastcall Destruct(NiSourceTexture* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiSourceTexture_vtable);
    if (self->srcPixelData != nullptr) {
        Release(self->srcPixelData);
        self->srcPixelData = nullptr;
    }
    if (self->persistentSrcRendererData != nullptr) {
        Release(self->persistentSrcRendererData);
        self->persistentSrcRendererData = nullptr;
    }
    Release(self->srcPixelData);
    Release(self->persistentSrcRendererData);
    FixedStringRelease(&self->platformFilename);
    FixedStringRelease(&self->filename);
    self->vtable = reinterpret_cast<void*>(NiTexture_vtable);
    if (self->rendererData != nullptr) {
        Virtual<void>(self->rendererData, 0, 1);
    }
    Texture_RemoveFromList(self);
    ObjectNET_Destruct(self);
}

// the preloading's renderer data made at once (none: the texture deleted, null)
inline NiSourceTexture* Preload(NiSourceTexture* texture) {
    if (NiSourceTexture_ms_bPreload != 0 && !Virtual<uint8_t>(texture, 22)) {
        Virtual<void>(texture, 0, 1);
        return nullptr;
    }
    return texture;
}

// Create(kFilename, kPrefs, bLoadDirectToRendererHint, bSrcRendererDataIsPersistent)
NiSourceTexture* __cdecl CreateFromFile(const re::Ptr32<const char>* filename, const uint32_t* prefs, int loadDirect,
                                        int persistent) {
    void* p = NiNew(0x44);
    NiSourceTexture* texture = p != nullptr ? SourceTexture_Construct(p) : nullptr;
    texture->pixelLayout = prefs[0];
    texture->alphaFormat = prefs[1];
    texture->mipMapped = prefs[2];
    texture->loadDirectToRendererHint = uint8_t(loadDirect);
    texture->srcRendererDataIsPersistent = uint8_t(persistent);
    FixedString_Assign(&texture->filename, filename);
    SourceTexture_StandardizeFilename(&texture->filename);
    ConvertToPlatformSpecificFilename(&texture->filename, &texture->platformFilename);
    return Preload(texture);
}

// Create(pkPixelData, kPrefs)
NiSourceTexture* __cdecl CreateFromPixelData(NiObject* pixels, const uint32_t* prefs) {
    void* p = NiNew(0x44);
    NiSourceTexture* texture = p != nullptr ? SourceTexture_Construct(p) : nullptr;
    texture->pixelLayout = prefs[0];
    texture->alphaFormat = prefs[1];
    texture->mipMapped = prefs[2];
    AssignReleaseFirst(texture->srcPixelData, pixels);
    return Preload(texture);
}

// vf20 LoadPixelDataFromFile: through the image converter, when there's a file and no source
void __fastcall LoadPixelDataFromFile(NiSourceTexture* self, re::Edx) {
    if (self->filename == NullFixedString) {
        return;
    }
    if (self->srcPixelData != nullptr || self->persistentSrcRendererData != nullptr) {
        return;
    }
    void* converter = ImageConverter_Get();
    re::Ptr32<NiObject> pixels = Virtual<NiObject*>(converter, 3, self->platformFilename, self->srcPixelData);  // ReadImageFile
    AddRef(pixels);
    if (self->srcPixelData == nullptr || pixels != self->srcPixelData) {
        Pointer_AssignPointer(&self->srcPixelData, &pixels);
    }
    Release(pixels);
}

// vf4 LoadBinary (the texture held while loading). An external file is looked for along the
// stream's search path: a texture the stream already has under a found name replaces this one
// (the stream's ChangeObject, slot 10); else the first path that exists is taken and this
// texture recorded under it
void __fastcall LoadBinary(NiSourceTexture* self, re::Edx, void* stream) {
    Texture_LoadBinary(self, stream);
    AddRef(self);
    uint8_t external = 0;
    LoadBytes(stream, &external, 1);
    if (StreamVersion(stream) < 0x14010001) {
        Stream_LoadInlineFixedString(stream, &self->filename);
    } else {
        Stream_LoadFixedString(stream, &self->filename);
    }
    Stream_AddLinkIDBlock(stream, 1);
    Stream_ReadLinkID(stream);
    if (self->filename != nullptr) {
        ConvertToPlatformSpecificFilename(&self->filename, &self->platformFilename);
    }

    if (external) {
        void* searchPath = At<void*>(stream, 0x124);
        char path[0x104];
        SearchPath_SetFilePath(searchPath, self->platformFilename);
        Virtual<void>(searchPath, 1);   // Reset
        NiObject* found = nullptr;
        while (Virtual<uint8_t>(searchPath, 2, path, 0x104)) {   // GetNextSearchPath
            found = Stream_GetTexture(stream, path, self);
            if (found != nullptr) {
                break;
            }
        }
        if (found != nullptr) {
            FixedString_AssignCString(&self->platformFilename, path);
            Virtual<void>(stream, 10, found);   // ChangeObject
        } else {
            Virtual<void>(searchPath, 1);
            if (Virtual<uint8_t>(searchPath, 2, path, 0x104)) {
                do {
                    if (NiFile_Access(path, 0)) {
                        FixedString_AssignCString(&self->platformFilename, path);
                        Stream_SetTexture(stream, self->platformFilename, self);
                        break;
                    }
                } while (Virtual<uint8_t>(searchPath, 2, path, 0x104));
            }
        }
    } else if (self->platformFilename != NullFixedString) {
        NiObject* found = Stream_GetTexture(stream, self->platformFilename, self);
        if (found == nullptr) {
            Stream_SetTexture(stream, self->platformFilename, self);
        } else {
            Virtual<void>(stream, 10, found);
        }
    }

    LoadBytes(stream, &self->pixelLayout, 4);
    LoadBytes(stream, &self->mipMapped, 4);
    LoadBytes(stream, &self->alphaFormat, 4);
    uint8_t value;
    LoadBytes(stream, &value, 1);
    self->isStatic = value != 0 ? 1 : 0;
    if (StreamVersion(stream) >= 0x0A010067) {
        LoadBytes(stream, &value, 1);
        self->loadDirectToRendererHint = value != 0 ? 1 : 0;
    }
    if (StreamVersion(stream) >= 0x14020004) {
        LoadBytes(stream, &value, 1);
        self->srcRendererDataIsPersistent = value != 0 ? 1 : 0;
    }
    Release(self);
}

// vf7 SaveBinary: external when there's no source; the source's link (the persistent one when
// the flag says so); the flags written whatever the version
void __fastcall SaveBinary(NiSourceTexture* self, re::Edx, void* stream) {
    Texture_SaveBinary(self, stream);
    uint8_t external = (self->persistentSrcRendererData == nullptr && self->srcPixelData == nullptr) ? 1 : 0;
    SaveBytes(stream, &external, 1);
    Stream_SaveFixedString(stream, &self->filename);
    if (self->srcRendererDataIsPersistent != 0) {
        Virtual<void>(stream, 12, self->persistentSrcRendererData);   // SaveLinkID
    } else {
        Virtual<void>(stream, 12, self->srcPixelData);
    }
    SaveBytes(stream, &self->pixelLayout, 4);
    SaveBytes(stream, &self->mipMapped, 4);
    SaveBytes(stream, &self->alphaFormat, 4);
    uint8_t value = self->isStatic;
    SaveBytes(stream, &value, 1);
    value = self->loadDirectToRendererHint;
    SaveBytes(stream, &value, 1);
    value = self->srcRendererDataIsPersistent;
    SaveBytes(stream, &value, 1);
}

NiSourceTexture* __fastcall DeletingDestructor(NiSourceTexture* self, re::Edx, uint32_t flags) {
    SourceTexture_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x44);
    }
    return self;
}

// vf9
void __fastcall GetViewerStrings(NiSourceTexture* self, re::Edx, NiTArray<char*>* strings) {
    Texture_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiSourceTexture_RTTI.name));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISRCTEX_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISRCTEX_REVERSED("NiSourceTexture::CreateRendererData", 0x007694F0, &ni::CreateRendererData);
NISRCTEX_REVERSED("NiSourceTexture::GetWidth", 0x00769530, &ni::GetWidth);
NISRCTEX_REVERSED("NiSourceTexture::GetHeight", 0x00769560, &ni::GetHeight);
NISRCTEX_REVERSED("NiSourceTexture::LinkObject", 0x00769590, &ni::LinkObject);
NISRCTEX_REVERSED("NiSourceTexture::RegisterStreamables", 0x00769630, &ni::RegisterStreamables);
NISRCTEX_REVERSED("NiSourceTexture::IsEqual", 0x00769680, &ni::IsEqual);
NISRCTEX_REVERSED("NiSourceTexture::StandardizeFilename", 0x00769740, &ni::StandardizeFilename);
NISRCTEX_REVERSED("NiSourceTexture::NiSourceTexture", 0x007697A0, &ni::Construct);
NISRCTEX_REVERSED("NiSourceTexture::GetRTTI", 0x00769800, &ni::GetRTTI);
NISRCTEX_REVERSED("NiSourceTexture::~NiSourceTexture", 0x00769810, &ni::Destruct);
NISRCTEX_REVERSED("NiSourceTexture::CreateFromFile", 0x00769910, &ni::CreateFromFile);
NISRCTEX_REVERSED("NiSourceTexture::CreateFromPixelData", 0x007699A0, &ni::CreateFromPixelData);
NISRCTEX_REVERSED("NiSourceTexture::LoadPixelDataFromFile", 0x00769A40, &ni::LoadPixelDataFromFile);
NISRCTEX_REVERSED("NiSourceTexture::LoadBinary", 0x00769AF0, &ni::LoadBinary);
NISRCTEX_REVERSED("NiSourceTexture::SaveBinary", 0x00769DF0, &ni::SaveBinary);
NISRCTEX_REVERSED("NiSourceTexture::DeletingDestructor", 0x00769F60, &ni::DeletingDestructor);
NISRCTEX_REVERSED("NiSourceTexture::GetViewerStrings", 0x00769F80, &ni::GetViewerStrings);
