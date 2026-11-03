// NiSingleShaderMaterial (vtable 0x960810, 0x18 bytes): an NiMaterial for one shader by name,
// got from the shader factory (0x756620: ms_pkShaderFactory; its slot 2 RetrieveShader(name,
// implementation, report), 9 IsDefaultImplementation(shader), 14 / 15 the first / next shader
// library). Its slots: 3 IsShaderCurrent, 4 GetCurrentShader, 5 GetVertexInputSemantics,
// 6 UnloadShaders. NiShader: slot 3 IsInitialized, 4 Initialize, 5 (geometry), 14
// GetVertexInputSemantics; its name at +0x8 (0x77C280), its implementation at +0xC (0x853140).
//
// Also NiMaterialInstance's GetCachedShader / GetCurrentShader (what NiGeometry calls), and
// NiSingleShaderMaterialLibrary's (vtable 0x960A04; MdBullyMaterialLibrary shares them) walk
// over the shader libraries' descriptions (slot 3 GetMaterialName, 4 GetMaterialCount,
// 6 GetNextMaterialDesc).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiGeometry.h"
#include "gamebryo/NiMaterial.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiSingleShaderMaterial_RTTI, 0x020E38A0);

constexpr uintptr_t kVtable = 0x00960810;

struct NiSingleShaderMaterial : NiMaterial {
    re::Ptr32<char> shaderName;     // +0xC (NiFixedString)
    re::Ptr32<NiObject> shader;     // +0x10 (NiShaderPtr)
    uint8_t keepShader;   // +0x14 (UnloadShaders leaves it)
    uint8_t pad15[3];
};
static_assert(offsetof(NiSingleShaderMaterial, shader) == 0x10);
static_assert(sizeof(NiSingleShaderMaterial) == 0x18);

// the library's walk: the shader library it's at and the index in its description
struct MaterialLibrary : NiRefObject {
    re::Ptr32<char> name;         // +0x8
    re::Ptr32<void> library;      // +0xC (the shader library)
    uint32_t index;     // +0x10
};
static_assert(sizeof(MaterialLibrary) == 0x14);

GAME_FN(void*, __cdecl, ShaderFactory_GetInstance, 0x00756620);
GAME_FN(re::Ptr32<char>*, __thiscall, Shader_GetName, 0x0077C280, NiObject* self);
GAME_FN(uint32_t, __thiscall, Shader_GetImplementation, 0x00853140, NiObject* self);
GAME_FN(uint32_t, __thiscall, LibraryDesc_GetShaderCount, 0x007564C0, void* self);
GAME_FN(void*, __thiscall, LibraryDesc_GetShaderDesc, 0x007564D0, void* self, uint32_t index);
GAME_FN(void, __thiscall, Pointer_AssignRaw, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00518100, re::Ptr32<NiObject>* self, const re::Ptr32<ni::NiObject>* other);
GAME_FN(NiMaterial*, __thiscall, Material_Construct, 0x00762CD0, void* self, const re::Ptr32<char>* name);
GAME_FN(void, __thiscall, Material_Destruct, 0x00762AC0, void* self);
GAME_FN(NiMaterial*, __cdecl, Material_GetMaterial, 0x00516AC0, const re::Ptr32<char>* name);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, SingleShaderMaterial_Destruct, 0x007D3570, NiSingleShaderMaterial* self);
GAME_FN(NiSingleShaderMaterial*, __thiscall, SingleShaderMaterial_ConstructShader, 0x007D3450,
        NiSingleShaderMaterial* self, const re::Ptr32<char>* name, NiObject* shader, int keepShader);
GAME_FN(NiSingleShaderMaterial*, __thiscall, SingleShaderMaterial_ConstructName, 0x007D34F0,
        NiSingleShaderMaterial* self, const re::Ptr32<char>* name, const char* shaderName, int loadNow);

inline void AddRefImport(NiRefObject* object) {
    if (object != nullptr) {
        Increment(&object->refCount);
    }
}

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// ---- NiMaterialInstance

// 0x7D3280 GetCachedShader(geometry, skin, propertyState, effectState): the cached shader while
// it's up to date (not when marked for an update, nor 2 with the geometry's default material
// flag) and its material says it's current
NiObject* __fastcall Instance_GetCachedShaderImpl(NiMaterialInstance* self, re::Edx, uint8_t* geometry, NiObject* skin,
                                                  NiObject* propertyState, NiObject* effectState) {
    NiObject* shader = self->cachedShader;
    if (shader == nullptr || self->needsUpdate == 1 || (self->needsUpdate == 2 && geometry[0xE8] != 0)) {
        return nullptr;
    }
    NiObject* material = self->material;
    if (material != nullptr && Virtual<uint8_t>(material, 3, shader, geometry, skin, propertyState, effectState,
                                                self->extraData) == 0) {
        return nullptr;
    }
    return self->cachedShader;
}

// 0x7D32E0 GetCurrentShader(...): the material's, cached (a new one told about the geometry,
// its slot 5)
NiObject* __fastcall Instance_GetCurrentShaderImpl(NiMaterialInstance* self, re::Edx, uint8_t* geometry,
                                                   NiObject* skin, NiObject* propertyState, NiObject* effectState) {
    NiObject* material = self->material;
    if (material != nullptr) {
        NiObject* shader = Virtual<NiObject*>(material, 4, geometry, skin, propertyState, effectState,
                                              self->extraData);
        if (shader == nullptr) {
            return nullptr;
        }
        if (shader != self->cachedShader) {
            Pointer_AssignRaw(&self->cachedShader, shader);
            Virtual<void>(shader, 5, geometry);
        }
        self->needsUpdate = 2;
    }
    return self->cachedShader;
}

// ---- NiSingleShaderMaterial

// 0x7D3340 vf3 IsShaderCurrent(shader, geometry, skin, propertyState, effectState, extraData):
// the shader has its name
uint8_t __fastcall IsShaderCurrentImpl(NiSingleShaderMaterial* self, re::Edx, NiObject* shader, void*, void*, void*,
                                       void*, uint32_t) {
    if (shader != nullptr && *Shader_GetName(shader) == self->shaderName) {
        return 1;
    }
    return 0;
}

// 0x7D3370 vf4 GetCurrentShader(geometry, skin, propertyState, effectState, extraData): its
// shader when it's of that implementation (or -1 and the default one), else the factory's by
// name (initialized)
NiObject* __fastcall GetCurrentShaderImpl(NiSingleShaderMaterial* self, re::Edx, void*, void*, void*, void*,
                                          uint32_t extraData) {
    void* factory = ShaderFactory_GetInstance();
    if (self->shader != nullptr) {
        if (Shader_GetImplementation(self->shader) == extraData) {
            return self->shader;
        }
        if (extraData == 0xFFFFFFFF) {
            if (factory == nullptr) {
                return nullptr;
            }
            if (Virtual<uint8_t>(factory, 9, self->shader) != 0) {
                return self->shader;
            }
        }
    }
    if (factory == nullptr) {
        return nullptr;
    }
    NiObject* shader = Virtual<NiObject*>(factory, 2, static_cast<const char*>(self->shaderName), extraData, 1);
    if (shader == nullptr) {
        return nullptr;
    }
    if (Virtual<uint8_t>(shader, 3) == 0 && Virtual<uint8_t>(shader, 4) == 0) {
        return nullptr;
    }
    return shader;
}

// 0x7D35E0 vf5 GetVertexInputSemantics(geometry, extraData, semantics): the shader's (none:
// the semantics' first stream -1, false)
uint8_t __fastcall GetVertexInputSemanticsImpl(NiSingleShaderMaterial* self, re::Edx, void*, uint32_t extraData,
                                               uint8_t* semantics) {
    re::Ptr32<NiObject> held = nullptr;
    void* factory = ShaderFactory_GetInstance();
    bool useOwn = false;
    if (self->shader != nullptr) {
        if (Shader_GetImplementation(self->shader) == extraData) {
            useOwn = true;
        } else if (extraData == 0xFFFFFFFF && factory != nullptr &&
                   Virtual<uint8_t>(factory, 9, self->shader) != 0) {
            useOwn = true;
        }
    }
    if (useOwn) {
        Pointer_Assign(&held, &self->shader);
    }
    if (held == nullptr) {
        NiObject* shader =
            Virtual<NiObject*>(factory, 2, static_cast<const char*>(self->shaderName), extraData, 1);
        if (shader == nullptr) {
            At<uint32_t>(semantics, 0x10) = 0xFFFFFFFF;
            ReleaseRef(held);
            return 0;
        }
        held = shader;
        AddRefImport(held);
    }
    uint8_t result = Virtual<uint8_t>(held, 14, semantics);
    ReleaseRef(held);
    return result;
}

// 0x7D3400 vf6 UnloadShaders: its shader let go unless it keeps it
void __fastcall UnloadShadersImpl(NiSingleShaderMaterial* self, re::Edx) {
    if (self->keepShader == 0 && self->shader != nullptr) {
        ReleaseRef(self->shader);
        self->shader = nullptr;
    }
}

// 0x7D3440 vf2
const RTTI* __fastcall GetRTTIImpl(NiSingleShaderMaterial*, re::Edx) {
    return &NiSingleShaderMaterial_RTTI;
}

// 0x7D3450 NiSingleShaderMaterial(name, shader, keepShader): the shader's name, the shader
// held (initialized if it isn't)
NiSingleShaderMaterial* __fastcall ConstructShaderImpl(NiSingleShaderMaterial* self, re::Edx, const re::Ptr32<char>* name,
                                                       NiObject* shader, int keepShader) {
    Material_Construct(self, name);
    self->vtable = reinterpret_cast<void*>(kVtable);
    self->shaderName = kNullFixedString;
    self->shader = nullptr;
    FixedString_Assign(&self->shaderName, Shader_GetName(shader));
    NiObject* old = self->shader;
    if (old != shader) {
        ReleaseRef(old);
        self->shader = shader;
        AddRefImport(shader);
    }
    self->keepShader = uint8_t(keepShader);
    if (Virtual<uint8_t>(shader, 3) == 0) {
        Virtual<uint8_t>(shader, 4);
    }
    return self;
}

// 0x7D34F0 NiSingleShaderMaterial(name, shaderName, loadNow): with loadNow the factory looks
// the shader up once (a reference taken and let go)
NiSingleShaderMaterial* __fastcall ConstructNameImpl(NiSingleShaderMaterial* self, re::Edx, const re::Ptr32<char>* name,
                                                     const char* shaderName, int loadNow) {
    Material_Construct(self, name);
    self->vtable = reinterpret_cast<void*>(kVtable);
    self->shaderName = shaderName == nullptr ? static_cast<char*>(kNullFixedString) : GlobalStringTable_AddString(shaderName);
    self->shader = nullptr;
    self->keepShader = 0;
    if (re::Flag(loadNow)) {
        void* factory = ShaderFactory_GetInstance();
        NiObject* shader = Virtual<NiObject*>(factory, 2, shaderName, 0xFFFFFFFFu, 0);
        if (shader != nullptr) {
            Increment(&shader->refCount);
            if (Decrement(&shader->refCount) == 0) {
                Virtual<void>(shader, 1);
            }
        }
    }
    return self;
}

// 0x7D3570
void __fastcall DestructImpl(NiSingleShaderMaterial* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kVtable);
    ReleaseRef(self->shader);
    FixedStringRelease(&self->shaderName);
    Material_Destruct(self);
}

// 0x7D36C0 vf0
NiSingleShaderMaterial* __fastcall DeletingDestructorImpl(NiSingleShaderMaterial* self, re::Edx, uint32_t flags) {
    SingleShaderMaterial_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x18);
    }
    return self;
}

// 0x7D36E0 Create(shader, keepShader): the material of the shader's name, made when there's none
NiMaterial* __cdecl CreateFromShaderImpl(NiObject* shader, int keepShader) {
    if (shader == nullptr) {
        return nullptr;
    }
    re::Ptr32<char> name = FixedStringAddRef(*Shader_GetName(shader));
    NiMaterial* material = Material_GetMaterial(&name);
    if (material == nullptr) {
        void* memory = NiNew(0x18);
        material = memory != nullptr ? SingleShaderMaterial_ConstructShader(
                                           static_cast<NiSingleShaderMaterial*>(memory), &name, shader, keepShader)
                                     : nullptr;
    }
    FixedStringRelease(&name);
    return material;
}

// 0x7D37C0 Create(name, loadNow): the material of that name, made when there's none
NiMaterial* __cdecl CreateFromNameImpl(const char* shaderName, int loadNow) {
    re::Ptr32<char> name = shaderName == nullptr ? static_cast<char*>(kNullFixedString) : GlobalStringTable_AddString(shaderName);
    NiMaterial* material = Material_GetMaterial(&name);
    FixedStringRelease(&name);
    if (material != nullptr) {
        return material;
    }
    void* memory = NiNew(0x18);
    if (memory == nullptr) {
        return nullptr;
    }
    name = shaderName == nullptr ? static_cast<char*>(kNullFixedString) : GlobalStringTable_AddString(shaderName);
    material = SingleShaderMaterial_ConstructName(static_cast<NiSingleShaderMaterial*>(memory), &name, shaderName,
                                                  loadNow);
    FixedStringRelease(&name);
    return material;
}

// ---- NiSingleShaderMaterialLibrary

// 0x7D5F70 vf6 GetNextMaterialDesc: the next shader description, on through the factory's
// libraries
void* __fastcall Library_GetNextMaterialDescImpl(MaterialLibrary* self, re::Edx) {
    void* factory = ShaderFactory_GetInstance();
    void* library = Virtual<void*>(factory, 14);
    while (library != self->library) {
        library = Virtual<void*>(factory, 15);
    }
    self->index++;
    if (library == nullptr) {
        return nullptr;
    }
    void* desc;
    do {
        desc = Virtual<void*>(library, 5);
        if (desc != nullptr && LibraryDesc_GetShaderCount(desc) > self->index) {
            break;
        }
        library = Virtual<void*>(factory, 15);
        self->library = library;
        self->index = 0;
    } while (library != nullptr);
    if (self->library == nullptr || desc == nullptr) {
        return nullptr;
    }
    uint32_t index = self->index;
    if (LibraryDesc_GetShaderCount(desc) <= index) {
        return nullptr;
    }
    return LibraryDesc_GetShaderDesc(desc, index);
}

// 0x7D6010 vf3 GetMaterialName(index): over all the libraries' descriptions in turn
const char* __fastcall Library_GetMaterialNameImpl(MaterialLibrary*, re::Edx, uint32_t index) {
    uint32_t before = 0;
    void* factory = ShaderFactory_GetInstance();
    for (void* library = Virtual<void*>(factory, 14); library != nullptr; library = Virtual<void*>(factory, 15)) {
        void* desc = Virtual<void*>(library, 5);
        if (desc == nullptr) {
            continue;
        }
        if (LibraryDesc_GetShaderCount(desc) + before > index) {
            void* shaderDesc = LibraryDesc_GetShaderDesc(desc, index - before);
            return Virtual<const char*>(shaderDesc, 2);
        }
        before += LibraryDesc_GetShaderCount(desc);
    }
    return nullptr;
}

// 0x7D6090 vf4 GetMaterialCount
uint32_t __fastcall Library_GetMaterialCountImpl(MaterialLibrary*, re::Edx) {
    uint32_t count = 0;
    void* factory = ShaderFactory_GetInstance();
    if (factory == nullptr) {
        return 0;
    }
    for (void* library = Virtual<void*>(factory, 14); library != nullptr; library = Virtual<void*>(factory, 15)) {
        void* desc = Virtual<void*>(library, 5);
        if (desc != nullptr) {
            count += LibraryDesc_GetShaderCount(desc);
        }
    }
    return count;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISINGLESHADER_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISINGLESHADER_REVERSED("NiMaterialInstance::GetCachedShader", 0x007D3280, ni::Instance_GetCachedShaderImpl);
NISINGLESHADER_REVERSED("NiMaterialInstance::GetCurrentShader", 0x007D32E0, ni::Instance_GetCurrentShaderImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::IsShaderCurrent", 0x007D3340, ni::IsShaderCurrentImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::GetCurrentShader", 0x007D3370, ni::GetCurrentShaderImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::UnloadShaders", 0x007D3400, ni::UnloadShadersImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::GetRTTI", 0x007D3440, ni::GetRTTIImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::NiSingleShaderMaterial(NiShader*)", 0x007D3450,
                        ni::ConstructShaderImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::NiSingleShaderMaterial(const char*)", 0x007D34F0,
                        ni::ConstructNameImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::~NiSingleShaderMaterial", 0x007D3570, ni::DestructImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::GetVertexInputSemantics", 0x007D35E0,
                        ni::GetVertexInputSemanticsImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::DeletingDestructor", 0x007D36C0, ni::DeletingDestructorImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::Create(NiShader*)", 0x007D36E0, ni::CreateFromShaderImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterial::Create(const char*)", 0x007D37C0, ni::CreateFromNameImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterialLibrary::GetNextMaterialDesc", 0x007D5F70,
                        ni::Library_GetNextMaterialDescImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterialLibrary::GetMaterialName", 0x007D6010,
                        ni::Library_GetMaterialNameImpl);
NISINGLESHADER_REVERSED("NiSingleShaderMaterialLibrary::GetMaterialCount", 0x007D6090,
                        ni::Library_GetMaterialCountImpl);
