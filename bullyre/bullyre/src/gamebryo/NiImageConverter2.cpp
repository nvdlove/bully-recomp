// Leftovers of the classes around the image converter (src/gamebryo/NiImageConverter.cpp,
// NiSkinInstance.cpp, NiDirectionalLight.cpp): NiImageConverter's other
// ConvertFilenameToPlatformSpecific (into a new NiMalloc'd path) and its _SDMInit (the default
// converter), the stream factories of NiPalette and NiDirectionalLight, NiSkinInstance's
// destructor; and NiSingleShaderMaterialLibrary's (vtable 0x960A04, 0x14 bytes: NiMaterialLibrary
// and the shader-library walk, src/gamebryo/NiSingleShaderMaterial.cpp) construction,
// destruction, CreateMaterial and _SDMInit.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiFile.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using ni::scene::At;
using ni::scene::Virtual;

GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(NiRefObject*, NiImageConverter_ms_spConverter, 0x020E394C);
GAME_VAR(char*, NiImageConverter_ms_pcPlatformSubDirectory, 0x020E3948);
GAME_VAR(void*, NiSingleShaderMaterialLibrary_ms_pkLibrary, 0x020E39EC);

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kNiImageConverterVtable = 0x009608AC;
constexpr uintptr_t kNiPaletteVtable = 0x009608C8;
constexpr uintptr_t kNiSkinInstanceVtable = 0x00953200;
constexpr uintptr_t kLibraryVtable = 0x00960A04;

GAME_FN(NiFilename*, __thiscall, Filename_Construct, 0x007538C0, NiFilename* self, const char* path);
GAME_FN(uint8_t, __thiscall, Filename_GetFullPath, 0x00753AD0, NiFilename* self, char* out, uint32_t size);
GAME_FN(void, __thiscall, Reader_Construct, 0x007F75B0, void* self);   // NiNIFImageReader
GAME_FN(void, __thiscall, Palette_AddToList, 0x007D4320, void* self);
GAME_FN(void*, __thiscall, DirectionalLight_Construct, 0x007D5500, void* self);
// NiRenderer's static: the renderer (if any) told to let go of a skin instance's data (its slot 53)
GAME_FN(void, __cdecl, Renderer_PurgeSkinInstance, 0x00755700, void* skin);
GAME_FN(void, __thiscall, MaterialLibrary_Construct, 0x007628C0, void* self, const re::Ptr32<const char>* name);
GAME_FN(void, __thiscall, MaterialLibrary_Destruct, 0x00762860, void* self);
GAME_FN(void*, __cdecl, SingleShaderMaterial_Create, 0x007D37C0, const char* name, int autoCreateShader);
// (this file's, called where modes apply)
GAME_FN(void*, __thiscall, SingleShaderMaterialLibrary_Construct, 0x007D6150, void* self);

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

// ---- NiImageConverter

// ConvertFilenameToPlatformSpecific(filename): a new 0x104-byte path, with the platform
// subdirectory put in when there is one, else the name copied (strncpy_s, truncated to fit)
char* __cdecl ConvertFilenameToPlatformSpecific(const char* filename) {
    char* out = static_cast<char*>(NiMalloc(0x104));
    if (NiImageConverter_ms_pcPlatformSubDirectory != nullptr) {
        NiFilename name;
        Filename_Construct(&name, filename);
        crt::StrcpyS(name.platformSubDir, 0x100, NiImageConverter_ms_pcPlatformSubDirectory);
        Filename_GetFullPath(&name, out, 0x104);
    } else {
        crt::StrncpyS(out, 0x104, filename, std::strlen(filename));
    }
    return out;
}

// _SDMInit: the default converter (an NiImageConverter with its NIF reader) held in ms_spConverter
void __cdecl ImageConverterSDMInit() {
    NiRefObject* converter = static_cast<NiRefObject*>(NiNew(0x438));
    if (converter != nullptr) {
        converter->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
        converter->refCount = 0;
        ni::sys::Increment(&ms_uiObjects);
        converter->vtable = reinterpret_cast<void*>(kNiImageConverterVtable);
        Reader_Construct(reinterpret_cast<uint8_t*>(converter) + 8);
    }
    SetReference(NiImageConverter_ms_spConverter, converter);
}

// _SDMShutdown (NiMainSDM::Shutdown's): the default converter let go, the slot emptied
void __cdecl ImageConverterSDMShutdown() {
    SetReference(NiImageConverter_ms_spConverter, nullptr);
}

// ---- NiPalette

// the stream's factory: an empty palette (alpha off, no entries, revision 1) on the global list
void* __cdecl PaletteCreateObject() {
    uint8_t* palette = static_cast<uint8_t*>(NiNew(0x24));
    if (palette == nullptr) {
        return nullptr;
    }
    ni::scene::RefObject_Construct(palette);
    *reinterpret_cast<re::Ptr32<void>*>(palette) = reinterpret_cast<void*>(kNiPaletteVtable);
    At<uint8_t>(palette, 0x8) = 0;
    At<uint32_t>(palette, 0xC) = 0;
    At<void*>(palette, 0x14) = nullptr;
    At<uint32_t>(palette, 0x10) = 1;
    At<void*>(palette, 0x18) = nullptr;
    Palette_AddToList(palette);
    return palette;
}

// ---- NiSkinInstance

// ~NiSkinInstance: the bone matrices freed, the renderer's data purged, the partition and the
// skin data let go, NiObject's destructor
void __fastcall SkinInstanceDestruct(NiObject* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiSkinInstanceVtable);
    NiFree(At<void*>(self, 0x14));
    Renderer_PurgeSkinInstance(self);
    NiRefObject* partition = At<NiRefObject*>(self, 0xC);
    if (partition != nullptr && ni::sys::Decrement(&partition->refCount) == 0) {
        Virtual<void>(partition, 1);
    }
    NiRefObject* data = At<NiRefObject*>(self, 0x8);
    if (data != nullptr && ni::sys::Decrement(&data->refCount) == 0) {
        Virtual<void>(data, 1);
    }
    ni::scene::Object_Destruct(self);
}

// ---- NiDirectionalLight

void* __cdecl DirectionalLightCreateObject() {
    void* light = NiNew(0x130);
    if (light == nullptr) {
        return nullptr;
    }
    return DirectionalLight_Construct(light);
}

// ---- NiSingleShaderMaterialLibrary

// the deleting destructor: the walk state cleared, NiMaterialLibrary's destructor
void* __fastcall LibraryDeletingDestructor(NiRefObject* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kLibraryVtable);
    At<void*>(self, 0xC) = nullptr;
    At<uint32_t>(self, 0x10) = 0;
    MaterialLibrary_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x14);
    }
    return self;
}

// slot 2 CreateMaterial(name): an NiSingleShaderMaterial for the shader of that name (its shader
// made at once)
void* __fastcall LibraryCreateMaterial(NiRefObject*, re::Edx, const re::Ptr32<const char>* name) {
    return SingleShaderMaterial_Create(*name, 1);
}

// NiSingleShaderMaterialLibrary(): "NiSingleShaderMaterialLibrary"
void* __fastcall LibraryConstruct(NiRefObject* self, re::Edx) {
    NiFixedString name = ni::scene::StringTable_AddString("NiSingleShaderMaterialLibrary");
    MaterialLibrary_Construct(self, &name);
    ni::scene::FixedStringRelease(&name);
    At<void*>(self, 0xC) = nullptr;
    At<uint32_t>(self, 0x10) = 0;
    self->vtable = reinterpret_cast<void*>(kLibraryVtable);
    return self;
}

// _SDMInit: the library (kept in a plain pointer)
void __cdecl LibrarySDMInit() {
    void* memory = NiNew(0x14);
    NiSingleShaderMaterialLibrary_ms_pkLibrary =
        memory != nullptr ? SingleShaderMaterialLibrary_Construct(memory) : nullptr;
}

// _SDMShutdown (NiMainSDM::Shutdown's): the library deleted (its slot 0, the deleting destructor)
void __cdecl LibrarySDMShutdown() {
    void* library = NiSingleShaderMaterialLibrary_ms_pkLibrary;
    if (library != nullptr) {
        Virtual<void>(library, 0, 1u);
    }
    NiSingleShaderMaterialLibrary_ms_pkLibrary = nullptr;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIIMAGECONV2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIIMAGECONV2_REVERSED("NiImageConverter::ConvertFilenameToPlatformSpecific(new)", 0x007D3D90,
                      ni::ConvertFilenameToPlatformSpecific);
NIIMAGECONV2_REVERSED("NiImageConverter::_SDMInit", 0x007D3E90, ni::ImageConverterSDMInit);
NIIMAGECONV2_REVERSED("NiPalette::CreateObject", 0x007D44C0, ni::PaletteCreateObject);
NIIMAGECONV2_REVERSED("NiSkinInstance::~NiSkinInstance", 0x007D4850, ni::SkinInstanceDestruct);
NIIMAGECONV2_REVERSED("NiDirectionalLight::CreateObject", 0x007D55F0, ni::DirectionalLightCreateObject);
NIIMAGECONV2_REVERSED("NiSingleShaderMaterialLibrary::DeletingDestructor", 0x007D6100,
                      ni::LibraryDeletingDestructor);
NIIMAGECONV2_REVERSED("NiSingleShaderMaterialLibrary::CreateMaterial", 0x007D6130, ni::LibraryCreateMaterial);
NIIMAGECONV2_REVERSED("NiSingleShaderMaterialLibrary::NiSingleShaderMaterialLibrary", 0x007D6150,
                      ni::LibraryConstruct);
NIIMAGECONV2_REVERSED("NiSingleShaderMaterialLibrary::_SDMInit", 0x007D61C0, ni::LibrarySDMInit);
// the _SDMShutdowns: never hooked in the 32-bit game, bodies at their addresses for the 64-bit build
RE_UNHOOKED_FN("NiImageConverter::_SDMShutdown", 0x007D3F10, &ni::ImageConverterSDMShutdown);
RE_UNHOOKED_FN("NiSingleShaderMaterialLibrary::_SDMShutdown", 0x007D60E0, &ni::LibrarySDMShutdown);
