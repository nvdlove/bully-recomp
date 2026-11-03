// NiSourceCubeMap (vtable 0x961068, 0x48 bytes: NiSourceTexture and a word at +0x44): a cube map
// whose six faces are one NiPixelData of six faces, put together from six face images (read
// through the image converter, or linked pixel data in old files). Its factory, destructor,
// streaming (files before 10.3.0.6 have their own layout: the six names, the six links, the format
// preferences), comparison, viewer strings, the pixel data from its file, the renderer data.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiPixelData.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSourceTexture.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiSourceCubeMap_RTTI, 0x020E3CA0);
GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);

constexpr uintptr_t NiSourceCubeMap_vtable = 0x00961068;

struct NiSourceCubeMap : NiSourceTexture {
    uint32_t unknown44;   // +0x44 (0)
};
static_assert(sizeof(NiSourceCubeMap) == 0x48);

GAME_FN(void, __thiscall, SourceTexture_Construct, 0x007697A0, void* self);
GAME_FN(void, __thiscall, SourceTexture_Destruct, 0x00769810, void* self);
GAME_FN(uint8_t, __thiscall, SourceTexture_RegisterStreamables, 0x00769630, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, SourceTexture_IsEqual, 0x00769680, void* self, void* other);
GAME_FN(void, __thiscall, SourceTexture_GetViewerStrings, 0x00769F80, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, SourceTexture_LoadBinary, 0x00769AF0, void* self, void* stream);
GAME_FN(void, __thiscall, SourceTexture_LinkObject, 0x00769590, void* self, void* stream);
GAME_FN(void, __thiscall, Texture_LoadBinary, 0x007548C0, void* self, void* stream);
GAME_FN(void*, __cdecl, ImageConverter_Get, 0x007D3B40);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* slot, NiObject* value);
GAME_FN(NiPixelData*, __thiscall, PixelData_Construct, 0x0076A610, void* self, uint32_t width, uint32_t height,
        const void* format, uint32_t levels, uint32_t faces);
GAME_FN(void, __cdecl, NiPath_Standardize, 0x00752C20, char* path);
GAME_FN(uint8_t, __cdecl, NiFile_Access, 0x007533C0, const char* path, int mode);
GAME_FN(void, __thiscall, SearchPath_SetFilePath, 0x008C5400, void* searchPath, const char* path);
GAME_FN(void, __thiscall, Stream_LoadCString, 0x0075AEA0, void* stream, re::Ptr32<char>* string);
// (this file's, called where modes apply)
GAME_FN(char*, __cdecl, ConvertFilenameToPlatformSpecific, 0x007D3D90, const char* filename);
GAME_FN(void, __thiscall, CubeMap_CombineFaces, 0x007DA5A0, NiSourceCubeMap* self, NiPixelData* face0,
        NiPixelData* face1, NiPixelData* face2, NiPixelData* face3, NiPixelData* face4, NiPixelData* face5);
GAME_FN(void, __thiscall, CubeMap_LoadFaces, 0x007DA9A0, NiSourceCubeMap* self, const char* name0,
        const char* name1, const char* name2, const char* name3, const char* name4, const char* name5,
        void* searchPath);

// a reference slot assigned through the imports: the old one let go, then the new one held
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

// six faces held (NiPixelDataPtr[6]), let go last to first
struct Faces {
    re::Ptr32<NiPixelData> face[6] = {};
    ~Faces() {
        for (int i = 5; i >= 0; i--) {
            NiPixelData* f = face[i];
            if (f != nullptr && ni::sys::Decrement(&f->refCount) == 0) {
                Virtual<void>(f, 1);
            }
        }
    }
};

// vf6 RegisterStreamables: NiSourceTexture's
uint8_t __fastcall CubeMapRegisterStreamables(NiSourceCubeMap* self, re::Edx, void* stream) {
    return SourceTexture_RegisterStreamables(self, stream) != 0 ? 1 : 0;
}

// vf8: NiSourceTexture's, the same format preferences and static flag
uint8_t __fastcall CubeMapIsEqual(NiSourceCubeMap* self, re::Edx, NiSourceCubeMap* other) {
    if (!SourceTexture_IsEqual(self, other)) {
        return 0;
    }
    if (At<uint32_t>(self, 0x18) != At<uint32_t>(other, 0x18) || At<uint32_t>(self, 0x20) != At<uint32_t>(other, 0x20) ||
        At<uint32_t>(self, 0x1C) != At<uint32_t>(other, 0x1C) || self->isStatic != other->isStatic) {
        return 0;
    }
    return 1;
}

// slot 20 LoadPixelDataFromFile: without pixel data, read from the platform file name
void __fastcall CubeMapLoadPixelDataFromFile(NiSourceCubeMap* self, re::Edx) {
    if (self->srcPixelData != nullptr || self->platformFilename == NullFixedString) {
        return;
    }
    void* converter = ImageConverter_Get();
    NiObject* data = Virtual<NiObject*>(converter, 3, self->platformFilename, static_cast<void*>(nullptr));
    Pointer_Assign(&self->srcPixelData, data);
}

// slot 22 CreateRendererData: the renderer's (its slot 69); true without a renderer
uint8_t __fastcall CubeMapCreateRendererData(NiSourceCubeMap* self, re::Edx) {
    void* renderer = NiRenderer_ms_pkRenderer;
    if (renderer != nullptr && !Virtual<uint8_t>(renderer, 69, static_cast<void*>(self))) {
        return 0;
    }
    return 1;
}

// CombineFaces(six faces): one six-face NiPixelData of the first face's size, format and levels,
// each face's levels copied in (memcpy_s, sized by the destination's level and the source's)
void __fastcall CubeMapCombineFaces(NiSourceCubeMap* self, re::Edx, NiPixelData* face0, NiPixelData* face1,
                                    NiPixelData* face2, NiPixelData* face3, NiPixelData* face4,
                                    NiPixelData* face5) {
    void* memory = NiNew(0x70);
    NiPixelData* data = nullptr;
    if (memory != nullptr) {
        data = PixelData_Construct(memory, face0->widths[0], face0->heights[0], &face0->format, face0->mipmapLevels, 6);
    }
    SetReference(self->srcPixelData, static_cast<NiObject*>(data));
    Faces faces;
    NiPixelData* given[6] = {face0, face1, face2, face3, face4, face5};
    for (int i = 0; i < 6; i++) {
        SetReference(faces.face[i], given[i]);
    }
    for (uint32_t f = 0; f < 6; f++) {
        NiPixelData* source = faces.face[f];
        for (uint32_t level = 0; level < source->mipmapLevels; level++) {
            NiPixelData* target = static_cast<NiPixelData*>(self->srcPixelData);
            uint32_t sourceSize = source->offsets[level + 1] - source->offsets[level];
            uint8_t* from = source->pixels + source->offsets[level];
            uint32_t targetSize = target->offsets[level + 1] - target->offsets[level];
            uint8_t* to = target->pixels + target->offsets[level] + target->offsets[target->mipmapLevels] * f;
            crt::MemcpyS(to, targetSize, from, sourceSize);
        }
    }
}

// LinkObject: NiSourceTexture's from 10.3.0.6 on; before, NiObjectNET's and the six linked faces
// (when there are six links), put together if there's no pixel data yet
void __fastcall CubeMapLinkObject(NiSourceCubeMap* self, re::Edx, void* stream) {
    if (StreamVersion(stream) > 0x0A030005) {
        SourceTexture_LinkObject(self, stream);
        return;
    }
    ObjectNET_LinkObject(self, stream);
    Faces faces;
    if (Stream_GetNumberOfLinkIDs(stream) == 6) {
        for (int i = 0; i < 6; i++) {
            SetReference(faces.face[i], static_cast<NiPixelData*>(Stream_ResolveLinkID(stream)));
        }
    }
    if (self->srcPixelData == nullptr) {
        CubeMap_CombineFaces(self, faces.face[0], faces.face[1], faces.face[2], faces.face[3], faces.face[4],
                             faces.face[5]);
    }
}

const RTTI* __fastcall CubeMapGetRTTI(void*, re::Edx) {
    return &NiSourceCubeMap_RTTI;
}

// LoadFaces(six file names, search path): each name copied, standardized, made platform-specific
// and looked for along the search path; the first existing file read (the image converter's slot
// 3). (A face not found leaks its two name copies, as the game has it.) Then put together
void __fastcall CubeMapLoadFaces(NiSourceCubeMap* self, re::Edx, const char* name0, const char* name1,
                                 const char* name2, const char* name3, const char* name4, const char* name5,
                                 void* searchPath) {
    Faces faces;
    const char* names[6] = {name0, name1, name2, name3, name4, name5};
    void* converter = ImageConverter_Get();
    char path[0x104];
    for (int i = 0; i < 6; i++) {
        const char* name = names[i];
        uint32_t size = uint32_t(std::strlen(name)) + 1;
        char* copy = static_cast<char*>(NiMalloc(size));
        crt::StrcpyS(copy, size, name);
        NiPath_Standardize(copy);
        char* platformName = ConvertFilenameToPlatformSpecific(copy);
        SearchPath_SetFilePath(searchPath, platformName);
        Virtual<void>(searchPath, 1);   // Reset
        while (Virtual<uint8_t>(searchPath, 2, static_cast<char*>(path), 0x104u)) {   // GetNextSearchPath
            if (NiFile_Access(path, 0)) {
                SetReference(faces.face[i], Virtual<NiPixelData*>(converter, 3, static_cast<const char*>(path),
                                                                  static_cast<void*>(nullptr)));
                NiFree(platformName);
                NiFree(copy);
                break;
            }
        }
    }
    CubeMap_CombineFaces(self, faces.face[0], faces.face[1], faces.face[2], faces.face[3], faces.face[4],
                         faces.face[5]);
}

NiSourceCubeMap* __cdecl CubeMapCreateObject() {
    NiSourceCubeMap* object = static_cast<NiSourceCubeMap*>(NiNew(0x48));
    if (object == nullptr) {
        return nullptr;
    }
    SourceTexture_Construct(object);
    object->vtable = reinterpret_cast<void*>(NiSourceCubeMap_vtable);
    object->unknown44 = 0;
    return object;
}

// LoadBinary: NiSourceTexture's from 10.3.0.6 on. Before (the object held while it loads): the
// texture's, the external flag, six face names and six face links, the faces read when external,
// then the format preferences (+0x18, +0x20, +0x1C) and the static flag
void __fastcall CubeMapLoadBinary(NiSourceCubeMap* self, re::Edx, void* stream) {
    if (StreamVersion(stream) >= 0x0A030006) {
        SourceTexture_LoadBinary(self, stream);
        return;
    }
    Texture_LoadBinary(self, stream);
    ni::sys::Increment(&self->refCount);
    uint8_t external = 0;
    LoadBytes(stream, &external, 1);
    re::Ptr32<char> names[6] = {};
    for (int i = 0; i < 6; i++) {
        Stream_LoadCString(stream, &names[i]);
    }
    Stream_AddLinkIDBlock(stream, 6);
    for (int i = 0; i < 6; i++) {
        Stream_ReadLinkID(stream);
    }
    if (external != 0) {
        CubeMap_LoadFaces(self, names[0], names[1], names[2], names[3], names[4], names[5], At<void*>(stream, 0x124));
    }
    for (int i = 0; i < 6; i++) {
        NiFree(names[i]);
    }
    uint32_t value;
    LoadBytes(stream, &value, 4);
    At<uint32_t>(self, 0x18) = value;
    LoadBytes(stream, &value, 4);
    At<uint32_t>(self, 0x20) = value;
    LoadBytes(stream, &value, 4);
    At<uint32_t>(self, 0x1C) = value;
    uint8_t isStatic = self->isStatic;
    LoadBytes(stream, &isStatic, 1);
    self->isStatic = isStatic != 0 ? 1 : 0;
    if (ni::sys::Decrement(&self->refCount) == 0) {
        Virtual<void>(self, 1);
    }
}

// the deleting destructor: the pixel data let go, NiSourceTexture's destructor
NiSourceCubeMap* __fastcall CubeMapDeletingDestructor(NiSourceCubeMap* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiSourceCubeMap_vtable);
    NiObject* data = self->srcPixelData;
    if (data != nullptr) {
        if (ni::sys::Decrement(&data->refCount) == 0) {
            Virtual<void>(data, 1);
        }
        self->srcPixelData = nullptr;
    }
    SourceTexture_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x48);
    }
    return self;
}

void __fastcall CubeMapGetViewerStrings(NiSourceCubeMap* self, re::Edx, NiTArray<char*>* strings) {
    SourceTexture_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiSourceCubeMap_RTTI.name));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NICUBEMAP_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

RE_UNHOOKED("NiSourceCubeMap::SaveBinary (jump)", 0x007DA4C0);   // -> 0x769DF0 NiSourceTexture::SaveBinary

NICUBEMAP_REVERSED("NiSourceCubeMap::RegisterStreamables", 0x007DA4A0, ni::CubeMapRegisterStreamables);
NICUBEMAP_REVERSED("NiSourceCubeMap::IsEqual", 0x007DA4D0, ni::CubeMapIsEqual);
NICUBEMAP_REVERSED("NiSourceCubeMap::LoadPixelDataFromFile", 0x007DA510, ni::CubeMapLoadPixelDataFromFile);
NICUBEMAP_REVERSED("NiSourceCubeMap::CreateRendererData", 0x007DA580, ni::CubeMapCreateRendererData);
NICUBEMAP_REVERSED("NiSourceCubeMap::CombineFaces", 0x007DA5A0, ni::CubeMapCombineFaces);
NICUBEMAP_REVERSED("NiSourceCubeMap::LinkObject", 0x007DA820, ni::CubeMapLinkObject);
NICUBEMAP_REVERSED("NiSourceCubeMap::GetRTTI", 0x007DA990, ni::CubeMapGetRTTI);
NICUBEMAP_REVERSED("NiSourceCubeMap::LoadFaces", 0x007DA9A0, ni::CubeMapLoadFaces);
NICUBEMAP_REVERSED("NiSourceCubeMap::CreateObject", 0x007DABD0, ni::CubeMapCreateObject);
NICUBEMAP_REVERSED("NiSourceCubeMap::LoadBinary", 0x007DAC00, ni::CubeMapLoadBinary);
NICUBEMAP_REVERSED("NiSourceCubeMap::DeletingDestructor", 0x007DADD0, ni::CubeMapDeletingDestructor);
NICUBEMAP_REVERSED("NiSourceCubeMap::GetViewerStrings", 0x007DAE20, ni::CubeMapGetViewerStrings);
