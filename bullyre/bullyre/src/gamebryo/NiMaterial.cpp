// NiMaterial and NiMaterialLibrary (src/gamebryo/NiMaterial.h): the global map of materials by
// name and its iteration, the libraries (a material is asked of each in turn when it doesn't
// exist yet), the default shader directory.

#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiMaterial.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiMaterial_RTTI, 0x020E2708);

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiMaterial_vtable = 0x00951DC4;
constexpr uintptr_t NiMaterialLibrary_vtable = 0x00951DA8;

GAME_VAR(volatile long, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(void*, NiMaterial_ms_pkMaterials, 0x020E2704);   // NiTFixedStringMap<NiMaterial*>

// the libraries' array
struct LibraryArray {
    re::Ptr32<re::Ptr32<NiMaterialLibrary>> data;
    uint32_t allocated;
    uint32_t count;
};
GAME_VAR(LibraryArray, NiMaterialLibrary_ms_kLibraries, 0x020E25F4);
GAME_ARRAY(char, NiMaterialLibrary_ms_acDefaultWorkingDirectory, 0x020E2600);   // 0x104 chars

GAME_FN(void, __thiscall, LibraryArray_SetAllocated, 0x0083AD20, LibraryArray* self, uint32_t allocated);
GAME_FN(NiMaterial*, __cdecl, Material_GetMaterial, 0x00516AC0, const re::Ptr32<const char>* name);
GAME_FN(void*, __cdecl, ShaderSystemReady, 0x00756620);   // the global at 0x20E23FC
GAME_FN(void, __thiscall, MaterialMap_GetNext, 0x008C2290, void* map, re::Ptr32<void>* pos, re::Ptr32<const char>* key,
        re::Ptr32<NiMaterial>* value);
GAME_FN(void, __thiscall, MaterialMap_SetAt, 0x008C27A0, void* map, const re::Ptr32<const char>* key, NiMaterial* value);
GAME_FN(uint8_t, __thiscall, MaterialMap_RemoveAt, 0x008B9860, void* map, const re::Ptr32<const char>* key);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(void, __cdecl, Path_Standardize, 0x00752C20, char* path);
GAME_FN(uint8_t, __cdecl, Path_IsRelative, 0x00753B50, const char* path);
GAME_FN(void, __cdecl, Path_ConvertToAbsolute, 0x00753280, char* path, uint32_t size);
// (this chunk's, called where modes apply)
GAME_FN(void, __cdecl, Library_Remove, 0x00762820, NiMaterialLibrary* library);
GAME_FN(void, __thiscall, Library_Destructor, 0x00762860, NiMaterialLibrary* self);
GAME_FN(void, __thiscall, Material_Destructor, 0x00762AC0, NiMaterial* self);

// ---- the libraries

// slot 2 of NiMaterialLibrary, and the same "none" for several classes' slots
NiObject* __fastcall ReturnNone(void*, re::Edx, void*) {
    return nullptr;
}

NiMaterialLibrary* __cdecl GetMaterialLibrary(const re::Ptr32<const char>* name) {
    LibraryArray& libraries = NiMaterialLibrary_ms_kLibraries;
    for (uint32_t i = 0; i < libraries.count; i++) {
        NiMaterialLibrary* library = libraries.data[i];
        if (library != nullptr && library->name == *name) {
            return library;
        }
    }
    return nullptr;
}

// the library taken off the array (the last one moved into its place)
void __cdecl RemoveLibrary(NiMaterialLibrary* library) {
    LibraryArray& libraries = NiMaterialLibrary_ms_kLibraries;
    uint32_t count = libraries.count;
    for (uint32_t i = 0; i < count; i++) {
        if (libraries.data[i] == library) {
            if (int32_t(i) >= 0) {
                libraries.count = count - 1;
                libraries.data[i] = libraries.data[count - 1];
            }
            return;
        }
    }
}

NiMaterialLibrary* __fastcall LibraryConstruct(NiMaterialLibrary* self, re::Edx, const re::Ptr32<const char>* name) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiMaterialLibrary_vtable);
    self->name = NullFixedString;
    FixedString_Assign(&self->name, name);
    LibraryArray& libraries = NiMaterialLibrary_ms_kLibraries;
    if (libraries.count == libraries.allocated) {
        LibraryArray_SetAllocated(&libraries, libraries.allocated != 0 ? libraries.allocated * 2 : 1);
    }
    libraries.data[libraries.count] = self;
    libraries.count++;
    return self;
}

void __fastcall LibraryDestruct(NiMaterialLibrary* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiMaterialLibrary_vtable);
    Library_Remove(self);
    FixedStringRelease(&self->name);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    _InterlockedDecrement(&NiRefObject_ms_uiObjects);
}

NiMaterialLibrary* __fastcall LibraryDeletingDestructor(NiMaterialLibrary* self, re::Edx, uint32_t flags) {
    Library_Destructor(self);
    if (flags & 1) {
        NiDelete(self, 0xC);
    }
    return self;
}

// CreateMaterial(kName): the existing one, or the first library's that makes it
NiMaterial* __cdecl CreateMaterial(const re::Ptr32<const char>* name) {
    NiMaterial* material = Material_GetMaterial(name);
    if (material != nullptr) {
        return material;
    }
    LibraryArray& libraries = NiMaterialLibrary_ms_kLibraries;
    for (uint32_t i = 0; i < libraries.count; i++) {
        NiMaterialLibrary* library = libraries.data[i];
        if (library != nullptr && ShaderSystemReady() != nullptr) {
            NiMaterial* made = Virtual<NiMaterial*>(library, 2, name);
            if (made != nullptr) {
                return made;
            }
        }
    }
    return nullptr;
}

// the shader directory (null when empty)
const char* __cdecl GetDefaultWorkingDirectory() {
    return NiMaterialLibrary_ms_acDefaultWorkingDirectory[0] != 0 ? NiMaterialLibrary_ms_acDefaultWorkingDirectory
                                                                  : nullptr;
}

// SetDefaultWorkingDirectory(path): standardized and absolute
void __cdecl SetDefaultWorkingDirectory(const char* path) {
    char* directory = NiMaterialLibrary_ms_acDefaultWorkingDirectory;
    if (path == nullptr) {
        directory[0] = 0;
        return;
    }
    crt::StrcpyS(directory, 0x104, path);
    Path_Standardize(directory);
    if (Path_IsRelative(directory)) {
        Path_ConvertToAbsolute(directory, 0x104);
    }
}

// ---- the materials

// the map's first position (the first non-empty bucket)
void* __cdecl GetFirstMaterialIter() {
    void* map = NiMaterial_ms_pkMaterials;
    uint32_t buckets = At<uint32_t>(map, 4);
    re::Ptr32<void>* table = At<re::Ptr32<void>*>(map, 8);
    for (uint32_t i = 0; i < buckets; i++) {
        if (table[i] != nullptr) {
            return table[i];
        }
    }
    return nullptr;
}

NiMaterial* __cdecl GetNextMaterial(re::Ptr32<void>* pos) {
    if (*pos == nullptr) {
        return nullptr;
    }
    NiFixedString name = NullFixedString;
    re::Ptr32<NiMaterial> material = nullptr;
    MaterialMap_GetNext(NiMaterial_ms_pkMaterials, pos, &name, &material);
    FixedStringRelease(&name);
    return material;
}

// UnloadShadersForAllMaterials: each material's slot 6
void __cdecl UnloadShadersForAllMaterials() {
    re::Ptr32<void> pos = GetFirstMaterialIter();
    re::Ptr32<NiMaterial> material = nullptr;
    NiFixedString name = NullFixedString;
    if (pos == nullptr) {
        return;
    }
    do {
        MaterialMap_GetNext(NiMaterial_ms_pkMaterials, &pos, &name, &material);
        if (material != nullptr) {
            Virtual<void>(material, 6);
        }
    } while (pos != nullptr);
    FixedStringRelease(&name);
}

// the map's item freed (its name emptied)
void __fastcall MapDeleteItem(void*, re::Edx, void* item) {
    FixedString_AssignText(&At<NiFixedString>(item, 4), nullptr);
    At<NiMaterial*>(item, 8) = nullptr;
    NiDelete(item, 0xC);
}

NiMaterial* __fastcall MaterialConstruct(NiMaterial* self, re::Edx, const re::Ptr32<const char>* name) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiMaterial_vtable);
    NiFixedString s = *name;
    if (s != NullFixedString) {
        _InterlockedIncrement(reinterpret_cast<volatile long*>(re::PtrCast<char*>(s) - 8));
    }
    self->name = *name;
    MaterialMap_SetAt(NiMaterial_ms_pkMaterials, &self->name, self);
    return self;
}

void __fastcall MaterialDestruct(NiMaterial* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiMaterial_vtable);
    MaterialMap_RemoveAt(NiMaterial_ms_pkMaterials, &self->name);
    FixedStringRelease(&self->name);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    _InterlockedDecrement(&NiRefObject_ms_uiObjects);
}

NiMaterial* __fastcall MaterialDeletingDestructor(NiMaterial* self, re::Edx, uint32_t flags) {
    Material_Destructor(self);
    if (flags & 1) {
        NiDelete(self, 0xC);
    }
    return self;
}

const RTTI* __fastcall MaterialGetRTTI(NiMaterial*, re::Edx) {
    return &NiMaterial_RTTI;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIMAT_REVERSED(name, address, fn)                                                      \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIMAT_REVERSED("NiMaterialLibrary::CreateMaterialNone", 0x007627D0, &ni::ReturnNone);
NIMAT_REVERSED("NiMaterialLibrary::GetMaterialLibrary", 0x007627E0, &ni::GetMaterialLibrary);
NIMAT_REVERSED("NiMaterialLibrary::RemoveLibrary", 0x00762820, &ni::RemoveLibrary);
NIMAT_REVERSED("NiMaterialLibrary::~NiMaterialLibrary", 0x00762860, &ni::LibraryDestruct);
NIMAT_REVERSED("NiMaterialLibrary::NiMaterialLibrary", 0x007628C0, &ni::LibraryConstruct);
NIMAT_REVERSED("NiMaterialLibrary::DeletingDestructor", 0x00762940, &ni::LibraryDeletingDestructor);
NIMAT_REVERSED("NiMaterialLibrary::CreateMaterial", 0x00762980, &ni::CreateMaterial);
NIMAT_REVERSED("NiMaterialLibrary::GetDefaultWorkingDirectory", 0x00762A00, &ni::GetDefaultWorkingDirectory);
NIMAT_REVERSED("NiMaterial::GetFirstMaterialIter", 0x00762A10, &ni::GetFirstMaterialIter);
NIMAT_REVERSED("NiMaterialLibrary::SetDefaultWorkingDirectory", 0x00762A40, &ni::SetDefaultWorkingDirectory);
NIMAT_REVERSED("NiTFixedStringMap<NiMaterial*>::DeleteItem", 0x00762A90, &ni::MapDeleteItem);
NIMAT_REVERSED("NiMaterial::~NiMaterial", 0x00762AC0, &ni::MaterialDestruct);
NIMAT_REVERSED("NiMaterial::GetRTTI", 0x00762B20, &ni::MaterialGetRTTI);
NIMAT_REVERSED("NiMaterial::GetNextMaterial", 0x00762B30, &ni::GetNextMaterial);
NIMAT_REVERSED("NiMaterial::UnloadShadersForAllMaterials", 0x00762BA0, &ni::UnloadShadersForAllMaterials);
NIMAT_REVERSED("NiMaterial::NiMaterial", 0x00762CD0, &ni::MaterialConstruct);
NIMAT_REVERSED("NiMaterial::DeletingDestructor", 0x00762D30, &ni::MaterialDeletingDestructor);
