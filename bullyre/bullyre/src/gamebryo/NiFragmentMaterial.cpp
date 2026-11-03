// NiFragmentMaterial (src/gamebryo/NiFragmentMaterial.h): the shader cache by descriptor (37
// buckets, and 37 of the descriptors that failed), the program caches, the shader generation
// (render passes described by the derived material's GenerateShaderDescArray, a shader made
// for them, each pass's programs set up, the fallbacks tried when one fails), the resources
// turned into shader constant map entries, the helpers that add uniforms to a node; with its
// pieces (the render pass descriptors, the sets they keep) and the statics' getters.
//
// Names from Gamebryo 2.x where the code matches it; the program caches' and shaders' slots are
// named in comments only.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::frag;

// the defaults NiRenderer::SetDefaultProgramCache passes
GAME_VAR(uint8_t, ms_bDefaultLoadProgramCacheOnCreation, 0x00BC8B24);
GAME_VAR(uint8_t, ms_bDefaultAutoSaveProgramCache, 0x00BC8B25);
GAME_VAR(uint8_t, ms_bDefaultWriteDebugProgramData, 0x00BC8B26);
GAME_VAR(uint8_t, ms_bDefaultLockProgramCache, 0x020E34BC);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiFragmentMaterial_RTTI, 0x020E34C0);
GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
// the class index of the descriptor holders shaders return from their slot 25 (an NiDynamicCast)
GAME_VAR(uint32_t, DescriptorHolder_ClassIndex, 0x020E40EC);
// the object mappings' attribute types (12-byte entries, by mapping, up to 30)
GAME_ARRAY(uint32_t, ObjectMappingTypes, 0x020E3310);

constexpr uintptr_t NiFragmentMaterial_vtable = 0x009551B8;
constexpr uintptr_t PrimitiveArray_vtable = 0x00961988;

GAME_FN(NiMaterial*, __thiscall, Material_Construct, 0x00762CD0, void* self, const re::Ptr32<const char>* name);
GAME_FN(void, __thiscall, Material_Destruct, 0x00762AC0, void* self);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(int, __cdecl, FormatString, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, PrimitiveArray_SetSize, 0x0084A110, NiTArray<void*>* array, uint32_t size);
GAME_FN(void, __thiscall, PrimitiveArray_SetAt, 0x00833C70, NiTArray<void*>* array, uint32_t index,
        const re::Ptr32<void>* value);
// NiShaderConstantMap's, NiShaderFactory's, NiShaderAttributeDesc's
GAME_FN(uint8_t, __cdecl, ConstantMap_LookUpObjectMappingName, 0x0077F690, uint32_t mapping, re::Ptr32<const char>* name);
GAME_FN(void*, __cdecl, ShaderFactory_GetInstance, 0x00756620);
GAME_FN(void*, __thiscall, ShaderFactory_GetGlobalShaderConstantEntry, 0x007567A0, void* self,
        const re::Ptr32<const char>* key);
GAME_FN(uint8_t, __cdecl, ShaderFactory_RegisterGlobalShaderConstant, 0x00756A60, const re::Ptr32<const char>* key,
        uint32_t type, uint32_t size, const void* data);
GAME_FN(uint8_t, __cdecl, ShaderFactory_ReleaseGlobalShaderConstant, 0x00756B40, const re::Ptr32<const char>* key);
GAME_FN(uint32_t, __cdecl, AttributeDesc_GetAttributeTypeSize, 0x00784C60, uint32_t type);
// (this file's, called where modes apply)
GAME_FN(ObjectResourceOffset*, __cdecl, NewObjectOffsets, 0x00786770, uint32_t count);
GAME_FN(NiMaterialDescriptor*, __cdecl, NewDescriptors, 0x007867B0, uint32_t count);
GAME_FN(NiRefObject*, __thiscall, RetrieveFromHash, 0x00786BF0, void* self, const NiMaterialDescriptor* desc);
GAME_FN(uint8_t, __thiscall, CheckFailedShader, 0x007864B0, void* self, const NiMaterialDescriptor* desc);
GAME_FN(void, __thiscall, SetFailedShader, 0x00787CF0, void* self, const NiMaterialDescriptor* desc);
GAME_FN(void, __thiscall, RenderPass_Destruct, 0x00786CA0, RenderPassDescriptor* self);
GAME_FN(void*, __thiscall, Descriptor_VectorDeletingDestructor, 0x007871F0, NiMaterialDescriptor* self,
        uint32_t flags);
GAME_FN(void, __thiscall, ObjectOffsets_Realloc, 0x007872C0, NiTSet<ObjectResourceOffset>* self,
        uint32_t allocated);
GAME_FN(void, __thiscall, Descriptors_Realloc, 0x00787A30, NiTSet<NiMaterialDescriptor>* self, uint32_t allocated);
GAME_FN(void, __thiscall, ObjectSet_Add, 0x00787BE0, NiTSet<NiRefObject*>* self, const re::Ptr32<NiRefObject>* element);
GAME_FN(void, __thiscall, InsertIntoHash, 0x00787C60, void* self, NiRefObject* shader);   // (not reached)

inline void* const kObjectOffsetConstruct = reinterpret_cast<void*>(0x00786260);
inline void* const kDescriptorConstruct = reinterpret_cast<void*>(0x007D5470);
inline void* const kRenderPassConstruct = reinterpret_cast<void*>(0x00787A90);

using Self = NiFragmentMaterial;

// a shader's descriptor (its slot 25's holder, cast by its RTTI's class bit: the descriptor is
// at +4 of it). A holder of another class gives 4: the comparison then reads address 4 (the
// game's crash, kept)
inline const NiMaterialDescriptor* ShaderDescriptor(void* holder) {
    void* cast = nullptr;
    if (holder != nullptr) {
        const uint8_t* rtti = Virtual<const uint8_t*>(holder, 0);
        uint32_t index = DescriptorHolder_ClassIndex;
        if ((At<uint32_t>(rtti, 0xC + (index >> 5) * 4) & (1u << (index & 31))) != 0) {
            cast = holder;
        }
    }
    return reinterpret_cast<const NiMaterialDescriptor*>(reinterpret_cast<uintptr_t>(cast) + 4);
}

// the NiNew[]'d render passes let go (0x786CA0 on each, last first)
inline void DeletePasses(RenderPassDescriptor* passes) {
    uint32_t* block = reinterpret_cast<uint32_t*>(passes) - 1;
    for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
        RenderPass_Destruct(&passes[i]);
    }
    NiDeleteArray(block, 0x58);
}

// ---- the statics (NiRenderer::SetDefaultProgramCache's defaults)

uint8_t __cdecl GetDefaultLoadProgramCacheOnCreation() {
    return ms_bDefaultLoadProgramCacheOnCreation;
}
uint8_t __cdecl GetDefaultAutoSaveProgramCache() {
    return ms_bDefaultAutoSaveProgramCache;
}
uint8_t __cdecl GetDefaultWriteDebugProgramData() {
    return ms_bDefaultWriteDebugProgramData;
}
uint8_t __cdecl GetDefaultLockProgramCache() {
    return ms_bDefaultLockProgramCache;
}

// ---- the pieces

ObjectResourceOffset* __fastcall ObjectOffsetConstruct(ObjectResourceOffset* self, re::Edx) {
    self->programType = 3;
    self->objectType = 0;
    self->offset = 0;
    return self;
}

// NiTNewInterface<ObjectResourceOffset>::Allocate(count): NiNew[] (the size -1 on overflow)
ObjectResourceOffset* __cdecl AllocateObjectOffsets(uint32_t count) {
    uint64_t size = uint64_t(count) * 0xC;
    void* p = NiNewArray(size > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(size));
    if (p == nullptr) {
        return nullptr;
    }
    VectorConstruct(p, 0xC, count, kObjectOffsetConstruct);
    return static_cast<ObjectResourceOffset*>(p);
}

// NiTNewInterface<NiMaterialDescriptor>::Allocate(count): NiNew[] with the count before them
NiMaterialDescriptor* __cdecl AllocateDescriptors(uint32_t count) {
    uint64_t product = uint64_t(count) * 0x14;
    uint32_t size = product > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(product);
    size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
    uint32_t* p = static_cast<uint32_t*>(NiNewArray(size));
    if (p == nullptr) {
        return nullptr;
    }
    p[0] = count;
    VectorConstruct(p + 1, 0x14, count, kDescriptorConstruct);
    return reinterpret_cast<NiMaterialDescriptor*>(p + 1);
}

// the descriptor's vector deleting destructor (flag 2: an array, its count before it; flag 1:
// freed)
void* __fastcall DescriptorVectorDeletingDestructor(NiMaterialDescriptor* self, re::Edx, uint32_t flags) {
    if ((flags & 2) == 0) {
        Drop(self->identifier);
        if ((flags & 1) != 0) {
            NiDelete(self, 0x14);
        }
        return self;
    }
    uint32_t* block = reinterpret_cast<uint32_t*>(self) - 1;
    for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
        Drop(self[i].identifier);
    }
    if ((flags & 1) != 0) {
        NiDeleteArray(block, 0x14);
    }
    return block;
}

// NiTSet<ObjectResourceOffset>::Realloc(allocated): the used ones copied (however many there
// are), the old array freed
void __fastcall ObjectOffsetsRealloc(NiTSet<ObjectResourceOffset>* self, re::Edx, uint32_t allocated) {
    if (allocated == self->allocated) {
        return;
    }
    ObjectResourceOffset* base = nullptr;
    if (allocated != 0) {
        base = NewObjectOffsets(allocated);
        for (uint32_t i = 0; i < self->count; i++) {
            base[i] = self->base[i];
        }
    }
    NiDeleteArray(self->base, 0xC);
    self->base = base;
    self->allocated = allocated;
}

// NiTSet<NiMaterialDescriptor>::Realloc(allocated)
void __fastcall DescriptorsRealloc(NiTSet<NiMaterialDescriptor>* self, re::Edx, uint32_t allocated) {
    if (allocated == self->allocated) {
        return;
    }
    NiMaterialDescriptor* base = nullptr;
    if (allocated != 0) {
        base = NewDescriptors(allocated);
        for (uint32_t i = 0; i < self->count; i++) {
            Descriptor_Assign(&base[i], &self->base[i]);
        }
    }
    if (self->base != nullptr) {
        Descriptor_VectorDeletingDestructor(self->base, 3);
    }
    self->base = base;
    self->allocated = allocated;
}

RenderPassDescriptor* __fastcall RenderPassConstruct(RenderPassDescriptor* self, re::Edx) {
    Descriptor_Construct(&self->vertexDesc);
    Descriptor_Construct(&self->geometryDesc);
    Descriptor_Construct(&self->pixelDesc);
    self->usesNiRenderState = 0;
    self->alphaOverride = 0;
    self->alphaBlend = 0;
    self->resetObjectOffsets = 0;
    self->objectOffsets.base = nullptr;
    self->objectOffsets.allocated = 0;
    self->objectOffsets.count = 0;
    self->usePreviousSrcBlendMode = 0;
    self->usePreviousDestBlendMode = 0;
    self->srcBlendMode = 6;
    self->destBlendMode = 7;
    return self;
}

void __fastcall RenderPassDestruct(RenderPassDescriptor* self, re::Edx) {
    NiDeleteArray(self->objectOffsets.base, 0xC);
    Drop(self->pixelDesc.identifier);
    Drop(self->geometryDesc.identifier);
    Drop(self->vertexDesc.identifier);
}

// operator=: the descriptors, the flags, the blend modes, then the object offsets appended one
// by one (the set grown to twice its size when full)
RenderPassDescriptor* __fastcall RenderPassAssign(RenderPassDescriptor* self, re::Edx, const RenderPassDescriptor* other) {
    Descriptor_Assign(&self->vertexDesc, &other->vertexDesc);
    Descriptor_Assign(&self->geometryDesc, &other->geometryDesc);
    Descriptor_Assign(&self->pixelDesc, &other->pixelDesc);
    self->usesNiRenderState = other->usesNiRenderState;
    self->alphaOverride = other->alphaOverride;
    self->alphaBlend = other->alphaBlend;
    self->resetObjectOffsets = other->resetObjectOffsets;
    self->srcBlendMode = other->srcBlendMode;
    self->destBlendMode = other->destBlendMode;
    self->usePreviousSrcBlendMode = other->usePreviousSrcBlendMode;
    self->usePreviousDestBlendMode = other->usePreviousDestBlendMode;
    self->objectOffsets.count = 0;
    for (uint32_t i = 0; i < other->objectOffsets.count; i++) {
        const ObjectResourceOffset* from = &other->objectOffsets.base[i];
        NiTSet<ObjectResourceOffset>& set = self->objectOffsets;
        if (set.count == set.allocated) {
            ObjectOffsets_Realloc(&set, set.allocated != 0 ? set.allocated * 2 : 1);
        }
        set.base[set.count] = *from;
        set.count++;
    }
    return self;
}

// NiTObjectSet<NiPointer<...>>::~NiTObjectSet (one copy every such set shares)
void __fastcall ObjectSetDestruct(NiTSet<NiRefObject*>* self, re::Edx) {
    ObjectSet_RemoveAll(self);
    ObjectSet_DeleteArray(self->base);
}

// NiTObjectSet<NiPointer<...>>::Add (one copy every such set shares: the shadow manager's too):
// grown to twice its size when full, the slot assigned as an NiPointer
void __fastcall ObjectSetAdd(NiTSet<NiRefObject*>* self, re::Edx, const re::Ptr32<NiRefObject>* element) {
    if (self->count == self->allocated) {
        ObjectSet_Realloc(self, self->allocated != 0 ? self->allocated * 2 : 1);
    }
    re::Ptr32<NiRefObject>* slot = &self->base[self->count];
    self->count++;
    NiRefObject* old = *slot;
    if (old == *element) {
        return;
    }
    Release(old);
    *slot = *element;
    AddRef(*slot);
}

// NiFragmentMaterial::GetHighestObjectOffset(objectType, passes, pass): the highest offset of
// that object type back from this pass to the last one that resets the offsets
uint32_t __cdecl GetHighestObjectOffset(uint32_t objectType, RenderPassDescriptor* passes, int32_t pass) {
    uint32_t highest = 0;
    for (int32_t i = pass - 1; i >= 0; i--) {
        const NiTSet<ObjectResourceOffset>& set = passes[i].objectOffsets;
        for (uint32_t j = 0; j < set.count; j++) {
            if (set.base[j].objectType == objectType && set.base[j].offset > highest) {
                highest = set.base[j].offset;
            }
        }
        if (passes[i].resetObjectOffsets != 0) {
            break;
        }
    }
    return highest;
}

// ---- construction

NiFragmentMaterial* __fastcall Construct(Self* self, re::Edx, NiRefObject* library, const re::Ptr32<const char>* name,
                                         uint32_t vertexVersion, uint32_t geometryVersion, uint32_t pixelVersion,
                                         int autoCreateCaches) {
    Material_Construct(self, name);
    self->vtable = reinterpret_cast<void*>(NiFragmentMaterial_vtable);
    self->libraries.base = nullptr;
    self->libraries.allocated = 0;
    self->libraries.count = 0;
    self->programCaches[0] = nullptr;
    self->programCaches[1] = nullptr;
    self->programCaches[2] = nullptr;
    for (int i = 0; i < 37; i++) {
        self->shaders[i].base = nullptr;
        self->shaders[i].allocated = 0;
        self->shaders[i].count = 0;
    }
    for (int i = 0; i < 37; i++) {
        self->failedDescriptors[i].base = nullptr;
        self->failedDescriptors[i].allocated = 0;
        self->failedDescriptors[i].count = 0;
    }
    self->fallbacks.maxSize = 0;
    self->fallbacks.growBy = 1;
    self->fallbacks.size = 0;
    self->fallbacks.esize = 0;
    self->fallbacks.base = nullptr;
    self->fallbacks.vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    self->maxPassCount = 10;
    self->vertexVersion = uint16_t(vertexVersion);
    self->geometryVersion = uint16_t(geometryVersion);
    self->pixelVersion = uint16_t(pixelVersion);
    // (an NiPointer to the library for the call)
    re::Ptr32<NiRefObject> held = library;
    AddRef(library);
    ObjectSet_Add(&self->libraries, &held);
    Release(library);
    void* renderer = NiRenderer_ms_pkRenderer;
    if (uint8_t(autoCreateCaches) != 0 && renderer != nullptr) {
        Virtual<void>(renderer, 77, self);   // SetDefaultProgramCache(material)
    }
    return self;
}

// ~NiFragmentMaterial: the fallbacks' array, the failed descriptors (delete[]), the shaders, the
// program caches, the libraries, then NiMaterial's (the vtable left as it is)
void __fastcall Destruct(Self* self, re::Edx) {
    self->fallbacks.vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    NiFree(self->fallbacks.base);
    for (int i = 36; i >= 0; i--) {
        if (self->failedDescriptors[i].base != nullptr) {
            Descriptor_VectorDeletingDestructor(self->failedDescriptors[i].base, 3);
        }
    }
    for (int i = 36; i >= 0; i--) {
        ObjectSet_RemoveAll(&self->shaders[i]);
        ObjectSet_DeleteArray(self->shaders[i].base);
    }
    for (int i = 2; i >= 0; i--) {
        Release(self->programCaches[i]);
    }
    ObjectSet_RemoveAll(&self->libraries);
    ObjectSet_DeleteArray(self->libraries.base);
    Material_Destruct(self);
}

GAME_FN(void, __thiscall, FragmentMaterial_Destruct, 0x00788390, Self* self);

Self* __fastcall DeletingDestructor(Self* self, re::Edx, uint32_t flags) {
    FragmentMaterial_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3B8);
    }
    return self;
}

const RTTI* __fastcall GetRTTI(Self*, re::Edx) {
    return &NiFragmentMaterial_RTTI;
}

// ---- the program caches

// slot 7: each cache's slot 8 (SetWorkingDirectory) with the directory as an NiFixedString
void __fastcall SetWorkingDirectory(Self* self, re::Edx, const char* directory) {
    for (uint32_t i = 0; i < 3; i++) {
        void* cache = Virtual<void*>(self, 11, i);
        if (cache == nullptr) {
            continue;
        }
        NiFixedString name = directory != nullptr ? String(directory) : static_cast<const char*>(NullFixedString);
        Virtual<void>(cache, 8, &name);
        Drop(name);
    }
}

// slot 10: a new cache for the program type (the shaders made with the old one dropped first)
void __fastcall SetProgramCache(Self* self, re::Edx, NiRefObject* cache, uint32_t type) {
    if (self->programCaches[type] != cache) {
        for (int i = 0; i < 37; i++) {
            ObjectSet_RemoveAll(&self->shaders[i]);
        }
    }
    NiRefObject* old = self->programCaches[type];
    if (old != cache) {
        Release(old);
        self->programCaches[type] = cache;
        AddRef(cache);
    }
}

NiRefObject* __fastcall GetProgramCache(Self* self, re::Edx, uint32_t type) {
    return self->programCaches[type];
}

// slot 12: each cache's slot 14 (it writes itself to disk)
void __fastcall SaveProgramCaches(Self* self, re::Edx) {
    for (uint32_t i = 0; i < 3; i++) {
        void* cache = Virtual<void*>(self, 11, i);
        if (cache != nullptr) {
            Virtual<void>(cache, 14);
        }
    }
}

// GetProgramVersion(type): the material's version for the program type in the high half, plus
// every library's version
uint32_t __fastcall GetProgramVersion(Self* self, re::Edx, uint32_t type) {
    uint32_t version = 0;
    if (type == 0) {
        version = uint32_t(self->vertexVersion) << 16;
    } else if (type == 1) {
        version = uint32_t(self->pixelVersion) << 16;
    } else if (type == 2) {
        version = uint32_t(self->geometryVersion) << 16;
    }
    for (uint32_t i = 0; i < self->libraries.count; i++) {
        void* library = self->libraries.base[i];
        if (library != nullptr) {
            version += Library_GetVersion(library);
        }
    }
    return version;
}

// SetFailedGPUProgram(type, descriptor): the cache told (its slot 19) that the program of that
// name failed
void __fastcall SetFailedGPUProgram(Self* self, re::Edx, uint32_t type, const NiMaterialDescriptor* desc) {
    char name[0x104];
    Descriptor_ToString(desc, name, 0x30);
    void* cache = self->programCaches[type];
    if (cache != nullptr) {
        Virtual<void>(cache, 19, name);
    }
}

// slot 21 (the vertex program; NiStandardMaterial's builds it): the vertex cache's program of
// the descriptor's name (its slot 3, FindCachedProgram(name, uniforms, failed&))
void* __fastcall GenerateVertexShader(Self* self, re::Edx, const NiMaterialDescriptor* desc, void* uniforms) {
    char name[0x104];
    Descriptor_ToString(desc, name, 0x30);
    void* cache = self->programCaches[0];
    if (cache == nullptr) {
        return nullptr;
    }
    uint8_t failed = 0;
    return Virtual<void*>(cache, 3, name, uniforms, &failed);
}

// ---- the shader hash

// slots 14 and 15: the descriptor's four words hashed into 37 buckets (the material unused)
uint32_t __fastcall HashDescriptor(Self*, re::Edx, const NiMaterialDescriptor* desc) {
    uint32_t h = ((desc->bits[0] * 33 + desc->bits[1]) * 33 + desc->bits[2]) * 33 + desc->bits[3];
    return h % 37;
}

NiRefObject* __fastcall RetrieveFromHashImpl(Self* self, re::Edx, const NiMaterialDescriptor* desc) {
    uint32_t bucket = Virtual<uint32_t>(self, 15, desc);
    NiTSet<NiRefObject*>& set = self->shaders[bucket];
    for (uint32_t i = 0; i < set.count; i++) {
        NiRefObject* shader = set.base[i];
        if (Descriptor_Equals(ShaderDescriptor(Virtual<void*>(shader, 25)), desc)) {
            return shader;
        }
    }
    return nullptr;
}

uint8_t __fastcall CheckFailedShaderImpl(Self* self, re::Edx, const NiMaterialDescriptor* desc) {
    uint32_t bucket = Virtual<uint32_t>(self, 15, desc);
    NiTSet<NiMaterialDescriptor>& set = self->failedDescriptors[bucket];
    for (uint32_t i = 0; i < set.count; i++) {
        if (Descriptor_Equals(&set.base[i], desc)) {
            return 1;
        }
    }
    return 0;
}

// SetFailedShader(descriptor): appended to its bucket of failed descriptors
void __fastcall SetFailedShaderImpl(Self* self, re::Edx, const NiMaterialDescriptor* desc) {
    uint32_t bucket = Virtual<uint32_t>(self, 15, desc);
    NiTSet<NiMaterialDescriptor>& set = self->failedDescriptors[bucket];
    if (set.count == set.allocated) {
        Descriptors_Realloc(&set, set.allocated != 0 ? set.allocated * 2 : 1);
    }
    NiMaterialDescriptor* slot = &set.base[set.count];
    set.count++;
    Descriptor_Assign(slot, desc);
}

// slot 6: the shaders dropped, each cache's slot 16 (its programs dropped)
void __fastcall UnloadShaders(Self* self, re::Edx) {
    for (int i = 0; i < 37; i++) {
        ObjectSet_RemoveAll(&self->shaders[i]);
    }
    for (int i = 0; i < 3; i++) {
        if (self->programCaches[i] != nullptr) {
            Virtual<void>(self->programCaches[i], 16);
        }
    }
}

// AddShaderFallbackFunc(function): appended (NiTPrimitiveArray::Add); its index
uint32_t __fastcall AddShaderFallbackFunc(Self* self, re::Edx, re::Ptr32<void> function) {
    NiTArray<void*>* array = &self->fallbacks;
    uint32_t index = array->size;
    if (array->maxSize <= index) {
        PrimitiveArray_SetSize(array, array->growBy + index);
    }
    PrimitiveArray_SetAt(array, index, &function);
    return index;
}

// ---- the material's interface

// slot 3: whether the shader's descriptor is the one the geometry would make now (whatever
// GenerateDescriptor returns)
uint8_t __fastcall IsShaderCurrent(Self* self, re::Edx, NiRefObject* shader, void* geometry, void* skin, void* state,
                                   void* effects, uint32_t) {
    if (shader == nullptr) {
        return 0;
    }
    NiMaterialDescriptor desc;
    Descriptor_Construct(&desc);
    Virtual<uint8_t>(self, 18, geometry, skin, state, effects, &desc);
    void* holder = Virtual<void*>(shader, 25);
    if (holder != nullptr) {
        const NiMaterialDescriptor* current = ShaderDescriptor(holder);
        if (current != reinterpret_cast<const NiMaterialDescriptor*>(4) && Descriptor_Equals(current, &desc)) {
            FixedString_Destruct(&desc.identifier);
            return 1;
        }
    }
    Drop(desc.identifier);
    return 0;
}

// slot 4: the shader for the geometry's descriptor (slot 19)
NiRefObject* __fastcall GetCurrentShader(Self* self, re::Edx, void* geometry, void* skin, void* state, void* effects,
                                         uint32_t) {
    NiMaterialDescriptor desc;
    Descriptor_Construct(&desc);
    if (!Virtual<uint8_t>(self, 18, geometry, skin, state, effects, &desc)) {
        Drop(desc.identifier);
        return nullptr;
    }
    NiRefObject* shader = Virtual<NiRefObject*>(self, 19, &desc);
    Drop(desc.identifier);
    return shader;
}

// slot 8: whether a shader comes of the descriptor (all of eax set, 0 or 1)
uint32_t __fastcall PrecacheGPUProgramsForDescriptor(Self* self, re::Edx, const NiMaterialDescriptor* desc) {
    return Virtual<void*>(self, 19, desc) != nullptr ? 1 : 0;
}

// slot 9: the geometry's descriptor's programs made (slot 8)
uint8_t __fastcall PrecacheGPUPrograms(Self* self, re::Edx, void* geometry, void* skin, void* state, void* effects,
                                       uint32_t) {
    NiMaterialDescriptor desc;
    Descriptor_Construct(&desc);
    if (!Virtual<uint8_t>(self, 18, geometry, skin, state, effects, &desc)) {
        Drop(desc.identifier);
        return 0;
    }
    uint8_t result = Virtual<uint8_t>(self, 8, &desc);
    Drop(desc.identifier);
    return result;
}

// slot 16: an attribute type from its HLSL name (0 for none: "bool" gives 1)
uint32_t __fastcall GetAttributeTypeFromString(Self*, re::Edx, const char* name) {
    if (NiStricmp(name, "float") == 0) {
        return 4;
    }
    if (NiStricmp(name, "float2") == 0) {
        return 5;
    }
    if (NiStricmp(name, "float3") == 0) {
        return 6;
    }
    if (NiStricmp(name, "float4") == 0) {
        return 7;
    }
    if (NiStricmp(name, "float3x3") == 0) {
        return 8;
    }
    if (NiStricmp(name, "float4x4") == 0) {
        return 9;
    }
    if (NiStricmp(name, "sampler") == 0) {
        return 0xB;
    }
    if (NiStricmp(name, "int") == 0 || NiStricmp(name, "uint") == 0) {
        return 3;
    }
    return NiStricmp(name, "bool") == 0 ? 1 : 0;
}

// slot 17: the HLSL name of an attribute type, by value (a 3x3 matrix is "float4x3", and colors
// are "float4", as the game has it); the first three made with 0x754760, the rest constructed
re::Ptr32<const char>* __fastcall GetStringFromAttributeType(Self*, re::Edx, re::Ptr32<const char>* result, uint32_t type) {
    switch (type) {
    case 4:
        *result = String("float");
        return result;
    case 5:
        *result = String("float2");
        return result;
    case 6:
        *result = String("float3");
        return result;
    case 7:
    case 10:
        FixedString_Construct(result, "float4");
        return result;
    case 8:
        FixedString_Construct(result, "float4x3");
        return result;
    case 9:
        FixedString_Construct(result, "float4x4");
        return result;
    case 0xB:
        FixedString_Construct(result, "sampler");
        return result;
    case 3:
        FixedString_Construct(result, "int");
        return result;
    case 1:
        FixedString_Construct(result, "bool");
        return result;
    default:
        *result = NullFixedString;
        return result;
    }
}

// slot 35: the first library's clone of the node of that name
void* __fastcall GetAttachableNodeFromLibraryImpl(Self* self, re::Edx, const re::Ptr32<const char>* name) {
    for (uint32_t i = 0; i < self->libraries.count; i++) {
        void* library = self->libraries.base[i];
        if (library != nullptr) {
            void* node = Library_GetAttachableNodeByName(library, name, 1);
            if (node != nullptr) {
                return node;
            }
        }
    }
    return nullptr;
}

// ---- the shader's generation

// slot 19 (GenerateShader): the hashed shader, or none if the descriptor failed before; else the
// render passes described (slot 24), a shader made (27) and set up (28), each pass appended
// and its programs set up (20). A pass that fails throws the shader away and asks the fallback
// function (the next one when a fallback gives up); the descriptor is kept as failed when it
// all fails. As the game has it: when the fallbacks run out, the passes are described once more
// before it gives up, and the passes' array is used unchecked when its allocation failed
NiRefObject* __fastcall GenerateShader(Self* self, re::Edx, NiMaterialDescriptor* desc) {
    NiRefObject* found = RetrieveFromHash(self, desc);
    if (found != nullptr) {
        return found;
    }
    if (CheckFailedShader(self, desc)) {
        return nullptr;
    }
    uint32_t maxPasses = self->maxPassCount;
    uint64_t product = uint64_t(maxPasses) * 0x58;
    uint32_t size = product > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(product);
    size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
    RenderPassDescriptor* passes = nullptr;
    uint32_t* block = static_cast<uint32_t*>(NiNewArray(size));
    if (block != nullptr) {
        block[0] = maxPasses;
        passes = reinterpret_cast<RenderPassDescriptor*>(block + 1);
        VectorConstruct(passes, 0x58, maxPasses, kRenderPassConstruct);
    }

    uint32_t count = 0;
    uint32_t fallbackCount = self->fallbacks.size;
    uint32_t fallbackIndex = 0;
    using Fallback = uint8_t(__cdecl*)(NiMaterialDescriptor*, uint32_t, uint32_t, RenderPassDescriptor*, uint32_t,
                                       uint32_t*, uint32_t*);
    Fallback fallback = nullptr;
    uint32_t fallbackData = 0;
    bool nextFallback = true;
    bool failed = false;
    NiRefObject* result = nullptr;
    uint32_t passId = 0;

    for (;;) {
        if (nextFallback) {
            fallback = nullptr;
            while (fallbackIndex < fallbackCount && fallback == nullptr) {
                fallback = re::PtrCast<Fallback>(self->fallbacks.base[fallbackIndex]);
                fallbackIndex++;
            }
            nextFallback = false;
            fallbackData = 0;
            for (uint32_t i = 0; i < self->maxPassCount; i++) {
                passes[i].usesNiRenderState = 0;
                passes[i].alphaOverride = 0;
                passes[i].alphaBlend = 0;
                passes[i].resetObjectOffsets = 0;
                passes[i].srcBlendMode = 6;
                passes[i].destBlendMode = 7;
            }
            if (Virtual<uint32_t>(self, 24, desc, passes, self->maxPassCount, &count) != 0) {
                SetFailedShaderImpl(self, nullptr, desc);   // (inlined in the game)
                if (passes != nullptr) {
                    DeletePasses(passes);
                }
                return nullptr;
            }
        }
        if (failed && fallback == nullptr) {
            break;
        }
        if (count == 0) {
            SetFailedShader(self, desc);
            if (passes != nullptr) {
                DeletePasses(passes);
            }
            return nullptr;
        }
        NiRefObject* shader = Virtual<NiRefObject*>(self, 27, desc);
        if (shader == nullptr) {
            SetFailedShader(self, desc);
            if (passes != nullptr) {
                DeletePasses(passes);
            }
            return nullptr;
        }
        if (!Virtual<uint8_t>(self, 28, shader, desc, passes, count)) {
            Virtual<void>(shader, 0, 1);
            SetFailedShader(self, desc);
            if (passes != nullptr) {
                DeletePasses(passes);
            }
            return nullptr;
        }
        Virtual<void>(shader, 21, uint32_t(passes[0].usesNiRenderState));   // SetUsesNiRenderState
        passId = 0;
        uint32_t rc = 0;
        for (uint32_t i = 0; i < count; i++) {
            RenderPassDescriptor* pass = &passes[i];
            if (!Virtual<uint8_t>(shader, 19, &passId)) {   // AppendRenderPass(id&)
                continue;
            }
            if (pass->alphaOverride != 0 &&
                !Virtual<uint8_t>(shader, 20, passId, uint32_t(pass->alphaBlend), uint32_t(pass->usePreviousSrcBlendMode),
                                  pass->srcBlendMode, uint32_t(pass->usePreviousDestBlendMode),
                                  pass->destBlendMode)) {   // SetAlphaOverride
                Virtual<void>(shader, 0, 1);
                SetFailedShader(self, desc);
                DeletePasses(passes);
                return nullptr;
            }
            rc = Virtual<uint32_t>(self, 20, &pass->vertexDesc, &pass->geometryDesc, &pass->pixelDesc,
                                   &pass->objectOffsets, shader, passId);
            if (rc != 0) {
                break;
            }
        }
        if (rc == 0) {
            result = shader;
            break;
        }
        // a pass failed: the shader thrown away, the fallback asked to change the passes
        Virtual<void>(shader, 0, 1);
        failed = true;
        if (fallback != nullptr &&
            !fallback(desc, rc, passId, passes, self->maxPassCount, &count, &fallbackData)) {
            nextFallback = true;
        }
    }

    if (passes != nullptr) {
        DeletePasses(passes);
    }
    if (result == nullptr) {
        SetFailedShader(self, desc);
        return nullptr;
    }
    if (!Virtual<uint8_t>(result, 3)) {   // IsInitialized
        Virtual<uint8_t>(result, 4);      // Initialize
    }
    InsertIntoHash(self, result);
    return result;
}

// slot 20 (SetupShaderPrograms(vertex, geometry, pixel descriptors, object offsets, shader,
// pass)): the three programs made (slots 21..23, each with its uniforms) and checked (25), each
// set on the shader's pass (its slot 22) with a new constant map (23) holding its uniforms
// (26). 0x10 when the shader can't be configured or a program can't be set, 0x20 when a constant
// map can't be made or filled
uint32_t __fastcall SetupShaderPrograms(Self* self, re::Edx, NiMaterialDescriptor* vertexDesc,
                                        NiMaterialDescriptor* geometryDesc, NiMaterialDescriptor* pixelDesc,
                                        NiTSet<ObjectResourceOffset>* offsets, NiRefObject* shader, uint32_t pass) {
    if (!Virtual<uint8_t>(shader, 17)) {   // IsGenericallyConfigurable
        return 0x10;
    }
    NiRefObject* programs[3] = {nullptr, nullptr, nullptr};
    NiTSet<NiRefObject*> uniforms[3] = {};
    NiMaterialDescriptor* descs[3] = {vertexDesc, geometryDesc, pixelDesc};
    for (int i = 0; i < 3; i++) {
        NiRefObject* program = Virtual<NiRefObject*>(self, 21 + i, descs[i], &uniforms[i]);
        if (programs[i] != program) {
            Release(programs[i]);
            programs[i] = program;
            AddRef(program);
        }
    }

    // the locals let go (the game has some paths through the compiler's array destructor, with
    // 0x787BC0 and NiPointer's destructor: the same calls)
    auto cleanUp = [&]() {
        for (int i = 2; i >= 0; i--) {
            ObjectSet_RemoveAll(&uniforms[i]);
            ObjectSet_DeleteArray(uniforms[i].base);
        }
        for (int i = 2; i >= 0; i--) {
            Release(programs[i]);
        }
    };

    uint32_t rc = Virtual<uint32_t>(self, 25, programs[0], programs[1], programs[2]);
    if (rc != 0) {
        cleanUp();
        return rc;
    }
    for (int i = 0; i < 3; i++) {
        NiRefObject* program = programs[i];
        if (program == nullptr) {
            continue;
        }
        uint32_t type = At<uint32_t>(program, 8);
        if (!Virtual<uint8_t>(shader, 22, pass, program, &type)) {   // SetGPUProgram
            cleanUp();
            return 0x10;
        }
        void* map = Virtual<void*>(shader, 23, pass, type, 0u);   // CreateShaderConstantMap
        if (map == nullptr) {
            cleanUp();
            return 0x20;
        }
        for (uint32_t j = 0; j < uniforms[i].count; j++) {
            NiRefObject* resource = uniforms[i].base[j];
            if (resource != nullptr &&
                !Virtual<uint8_t>(self, 26, shader, map, resource, vertexDesc, geometryDesc, pixelDesc, offsets, pass,
                                  type)) {
                cleanUp();
                return 0x20;
            }
        }
    }
    cleanUp();
    return 0;
}

// slot 26 (AddResourceToShaderConstantMap(shader, map, resource, the three descriptors,
// offsets, pass, program type)): an entry by the resource's source: an object's (offset by the
// highest one of its type in this program), a predefined one's (a sampler's texture assigned to
// the shader instead), an attribute's, a global's (registered with the shader factory for the
// call). 1 on success
uint8_t __fastcall AddResourceToShaderConstantMap(Self* self, re::Edx, NiRefObject* shader, void* map, void* resource,
                                                  void*, void*, void*, NiTSet<ObjectResourceOffset>* offsets,
                                                  uint32_t pass, uint32_t programType) {
    uint32_t count = Resource_GetCount(resource);
    uint32_t extra = count > 1 ? count << 16 : 0;
    extra += Resource_GetExtraData(resource);
    switch (Resource_GetSource(resource)) {
    case 1: {   // an object
        uint32_t offset = 0;
        for (uint32_t i = 0; i < offsets->count; i++) {
            ObjectResourceOffset* entry = &offsets->base[i];
            if (entry->programType == programType && entry->objectType == Resource_GetObjectType(resource)) {
                offset = entry->offset;
                break;
            }
        }
        NiFixedString variable = *Resource_GetVariable(resource);
        NiFixedString semantic = *Resource_GetSemantic(resource);
        uint32_t objectType = Resource_GetObjectType(resource);
        // AddObjectEntry(key, flags, variable, extra, object type)
        return Virtual<uint32_t>(map, 8, semantic, 0u, variable, Resource_GetExtraData(resource) + offset,
                                 objectType) == 0
                   ? 1
                   : 0;
    }
    case 2: {   // predefined
        if (!FixedString_EqualsNoCase(Resource_GetType(resource), "sampler2D") &&
            !FixedString_EqualsNoCase(Resource_GetType(resource), "samplerCUBE")) {
            NiFixedString variable = *Resource_GetVariable(resource);
            NiFixedString semantic = *Resource_GetSemantic(resource);
            // AddPredefinedEntry(key, extra, flags, variable)
            return Virtual<uint32_t>(map, 3, semantic, extra, 0u, variable) == 0 ? 1 : 0;
        }
        uint32_t textureIndex = 0xFFFFFFFF;
        uint32_t offset = 0;
        if (Resource_GetObjectType(resource) != 0) {
            for (uint32_t i = 0; i < offsets->count; i++) {
                ObjectResourceOffset* entry = &offsets->base[i];
                if (entry->programType == programType && entry->objectType == Resource_GetObjectType(resource)) {
                    offset = entry->offset;
                    break;
                }
            }
        }
        uint32_t index = Resource_GetExtraData(resource) + offset;
        const re::Ptr32<const char>* variable = Resource_GetVariable(resource);
        const re::Ptr32<const char>* semantic = Resource_GetSemantic(resource);
        // the shader's slot 24: the sampler's texture (pass, &texture index, semantic, variable,
        // instance)
        return Virtual<uint8_t>(shader, 24, pass, &textureIndex, semantic, variable, index) != 0 ? 1 : 0;
    }
    case 4: {   // an attribute
        if (FixedString_EqualsNoCase(Resource_GetType(resource), "sampler")) {
            return 0;
        }
        uint32_t type = Virtual<uint32_t>(self, 16, *Resource_GetType(resource));
        uint32_t size = AttributeDesc_GetAttributeTypeSize(type);
        NiFixedString key = *Resource_GetVariable(resource);
        NiFixedString variable = *Resource_GetVariable(resource);
        // AddAttributeEntry(key, type, extra, register, count, variable, size, stride, data, copy)
        return Virtual<uint32_t>(map, 4, variable, type, extra, 0u, 0u, key, size, size, 0u, 0u) == 0 ? 1 : 0;
    }
    case 5: {   // a global
        if (FixedString_EqualsNoCase(Resource_GetType(resource), "sampler")) {
            return 0;
        }
        uint32_t type = Virtual<uint32_t>(self, 16, *Resource_GetType(resource));
        uint32_t size = AttributeDesc_GetAttributeTypeSize(type);
        NiFixedString key;
        FixedString_Copy(&key, Resource_GetVariable(resource));
        ShaderFactory_RegisterGlobalShaderConstant(&key, type, size, nullptr);
        void* entry = ShaderFactory_GetGlobalShaderConstantEntry(ShaderFactory_GetInstance(), &key);
        // AddGlobalEntry(key, type, extra, register, count, variable, size, stride, data, copy)
        uint32_t error = Virtual<uint32_t>(map, 6, key, type, extra, 0u, 0u, key, size, size, At<void*>(entry, 0x14),
                                           0u);
        ShaderFactory_ReleaseGlobalShaderConstant(Resource_GetVariable(resource));
        FixedString_Destruct(&key);
        return error == 0 ? 1 : 0;
    }
    default:
        return 0;
    }
}

// ---- adding uniforms to a node

// slot 29: "UVSet<index>"
re::Ptr32<const char>* __fastcall GenerateUVSetName(Self*, re::Edx, re::Ptr32<const char>* result, uint32_t index) {
    char name[0x20];
    FormatString(name, 0x20, "UVSet%d", index);
    *result = String(name);
    return result;
}

// slot 30: InsertTextureSampler(context, map name, sampler type (1 a cube), occurrence, object
// type): a sampler uniform named after the map ("<name><occurrence>" past the first), an object
// resource
void* __fastcall InsertTextureSampler(Self*, re::Edx, MaterialContext* context, const re::Ptr32<const char>* mapName,
                                      uint32_t samplerType, uint32_t occurrence, uint32_t objectType) {
    char variableName[0x100];
    if (occurrence != 0) {
        FormatString(variableName, 0x100, "%s%d", *mapName, occurrence);
    } else {
        FormatString(variableName, 0x100, "%s", *mapName);
    }
    NiFixedString type = NullFixedString;
    FixedString_AssignText(&type, samplerType == 1 ? "samplerCUBE" : "sampler2D");
    NiFixedString variable = String(variableName);
    NiFixedString label = String("");
    void* resource = Node_AddOutputResource(context->uniforms, &type, mapName, &label, &variable, 1, 2, objectType,
                                            occurrence);
    Drop(label);
    Drop(variable);
    Drop(type);
    return resource;
}

// slot 32: AddOutputObject(node, mapping, object type, object index, variable modifier, count):
// a uniform for the object mapping, "g_<modifier><mapping>" (and the index, past one), its type
// from the mapping's entry
void* __fastcall AddOutputObject(Self* self, re::Edx, void* node, uint32_t mapping, uint32_t objectType,
                                 uint32_t objectIndex, const char* modifier, uint32_t count) {
    NiFixedString mappingName = NullFixedString;
    if (!ConstantMap_LookUpObjectMappingName(mapping, &mappingName)) {
        Drop(mappingName);
        return nullptr;
    }
    uint32_t entry = int32_t(mapping) < 0x1E ? mapping : 0x1E;
    NiFixedString type;
    Virtual<re::Ptr32<const char>*>(self, 17, &type, ObjectMappingTypes[entry * 3]);
    if (modifier == nullptr) {
        modifier = "";
    }
    char variableName[0x100];
    if (objectIndex == 1) {
        FormatString(variableName, 0x100, "g_%s%s", modifier, mappingName);
    } else {
        FormatString(variableName, 0x100, "g_%s%s%d", modifier, mappingName, objectIndex);
    }
    NiFixedString variable = String(variableName);
    NiFixedString label = String("");
    void* resource = Node_AddOutputResource(node, &type, &mappingName, &label, &variable, count, 1, objectType,
                                            objectIndex);
    Drop(label);
    Drop(variable);
    Drop(type);
    Drop(mappingName);
    return resource;
}

// slots 33 and 34: AddOutputAttribute / AddOutputGlobal(node, name, attribute type, count): a
// uniform of that name (its semantic too), source 4 or 5
void* AddOutputUniform(Self* self, void* node, const re::Ptr32<const char>* name, uint32_t attributeType, uint32_t count,
                       uint32_t source) {
    NiFixedString type;
    Virtual<re::Ptr32<const char>*>(self, 17, &type, attributeType);
    NiFixedString label = String("");
    void* resource = Node_AddOutputResource(node, &type, name, &label, name, count, source, 0, 0);
    Drop(label);
    Drop(type);
    return resource;
}

void* __fastcall AddOutputAttribute(Self* self, re::Edx, void* node, const re::Ptr32<const char>* name, uint32_t type,
                                    uint32_t count) {
    return AddOutputUniform(self, node, name, type, count, 4);
}

void* __fastcall AddOutputGlobal(Self* self, re::Edx, void* node, const re::Ptr32<const char>* name, uint32_t type,
                                 uint32_t count) {
    return AddOutputUniform(self, node, name, type, count, 5);
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIFRAGMAT_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIFRAGMAT_REVERSED("NiFragmentMaterial::GetDefaultLoadProgramCacheOnCreation", 0x00786220,
                   &ni::GetDefaultLoadProgramCacheOnCreation);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetDefaultAutoSaveProgramCache", 0x00786230, &ni::GetDefaultAutoSaveProgramCache);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetDefaultWriteDebugProgramData", 0x00786240,
                   &ni::GetDefaultWriteDebugProgramData);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetDefaultLockProgramCache", 0x00786250, &ni::GetDefaultLockProgramCache);
NIFRAGMAT_REVERSED("NiFragmentMaterial::ObjectResourceOffset::ObjectResourceOffset", 0x00786260,
                   &ni::ObjectOffsetConstruct);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetAttributeTypeFromString", 0x00786280, &ni::GetAttributeTypeFromString);
NIFRAGMAT_REVERSED("NiFragmentMaterial::SaveProgramCaches", 0x00786390, &ni::SaveProgramCaches);
NIFRAGMAT_REVERSED("NiFragmentMaterial::HashDescriptor", 0x007863C0, &ni::HashDescriptor);
NIFRAGMAT_REVERSED("NiFragmentMaterial::PrecacheGPUPrograms(descriptor)", 0x00786410,
                   &ni::PrecacheGPUProgramsForDescriptor);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetProgramCache", 0x00786430, &ni::GetProgramCache);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetProgramVersion", 0x00786440, &ni::GetProgramVersion);
NIFRAGMAT_REVERSED("NiFragmentMaterial::CheckFailedShader", 0x007864B0, &ni::CheckFailedShaderImpl);
NIFRAGMAT_REVERSED("NiFragmentMaterial::SetFailedGPUProgram", 0x00786520, &ni::SetFailedGPUProgram);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GenerateVertexShader", 0x00786580, &ni::GenerateVertexShader);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetHighestObjectOffset", 0x007866D0, &ni::GetHighestObjectOffset);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetAttachableNodeFromLibrary", 0x00786730,
                   &ni::GetAttachableNodeFromLibraryImpl);
NIFRAGMAT_REVERSED("NiTNewInterface<ObjectResourceOffset>::Allocate", 0x00786770, &ni::AllocateObjectOffsets);
NIFRAGMAT_REVERSED("NiTNewInterface<NiMaterialDescriptor>::Allocate", 0x007867B0, &ni::AllocateDescriptors);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetStringFromAttributeType", 0x00786800, &ni::GetStringFromAttributeType);
NIFRAGMAT_REVERSED("NiFragmentMaterial::SetProgramCache", 0x00786920, &ni::SetProgramCache);
NIFRAGMAT_REVERSED("NiFragmentMaterial::SetWorkingDirectory", 0x007869A0, &ni::SetWorkingDirectory);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetCurrentShader", 0x00786A30, &ni::GetCurrentShader);
NIFRAGMAT_REVERSED("NiFragmentMaterial::PrecacheGPUPrograms", 0x00786AF0, &ni::PrecacheGPUPrograms);
NIFRAGMAT_REVERSED("NiFragmentMaterial::UnloadShaders", 0x00786BB0, &ni::UnloadShaders);
NIFRAGMAT_REVERSED("NiFragmentMaterial::RetrieveFromHash", 0x00786BF0, &ni::RetrieveFromHashImpl);
NIFRAGMAT_REVERSED("NiFragmentMaterial::RenderPassDescriptor::~RenderPassDescriptor", 0x00786CA0,
                   &ni::RenderPassDestruct);
NIFRAGMAT_REVERSED("NiFragmentMaterial::AddResourceToShaderConstantMap", 0x00786D40,
                   &ni::AddResourceToShaderConstantMap);
NIFRAGMAT_REVERSED("NiFragmentMaterial::AddOutputAttribute", 0x00787070, &ni::AddOutputAttribute);
NIFRAGMAT_REVERSED("NiFragmentMaterial::AddOutputGlobal", 0x00787130, &ni::AddOutputGlobal);
NIFRAGMAT_REVERSED("NiMaterialDescriptor::VectorDeletingDestructor", 0x007871F0,
                   &ni::DescriptorVectorDeletingDestructor);
NIFRAGMAT_REVERSED("NiTSet<ObjectResourceOffset>::Realloc", 0x007872C0, &ni::ObjectOffsetsRealloc);
NIFRAGMAT_REVERSED("NiFragmentMaterial::IsShaderCurrent", 0x00787330, &ni::IsShaderCurrent);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GenerateUVSetName", 0x00787410, &ni::GenerateUVSetName);
NIFRAGMAT_REVERSED("NiFragmentMaterial::InsertTextureSampler", 0x00787460, &ni::InsertTextureSampler);
NIFRAGMAT_REVERSED("NiFragmentMaterial::AddOutputObject", 0x00787780, &ni::AddOutputObject);
NIFRAGMAT_REVERSED("NiFragmentMaterial::AddShaderFallbackFunc", 0x007879A0, &ni::AddShaderFallbackFunc);
NIFRAGMAT_REVERSED("NiTSet<NiMaterialDescriptor>::Realloc", 0x00787A30, &ni::DescriptorsRealloc);
NIFRAGMAT_REVERSED("NiFragmentMaterial::RenderPassDescriptor::RenderPassDescriptor", 0x00787A90,
                   &ni::RenderPassConstruct);
NIFRAGMAT_REVERSED("NiFragmentMaterial::RenderPassDescriptor::operator=", 0x00787AE0, &ni::RenderPassAssign);
// (NiTObjectSet<NiPointer<...>>'s: one copy every such set in the exe shares)
NIFRAGMAT_REVERSED("NiTObjectSet::~NiTObjectSet", 0x00787BC0, &ni::ObjectSetDestruct);
NIFRAGMAT_REVERSED("NiTObjectSet::Add", 0x00787BE0, &ni::ObjectSetAdd);
NIFRAGMAT_REVERSED("NiFragmentMaterial::SetFailedShader", 0x00787CF0, &ni::SetFailedShaderImpl);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GenerateShader", 0x00787D50, &ni::GenerateShader);
NIFRAGMAT_REVERSED("NiFragmentMaterial::NiFragmentMaterial", 0x00788260, &ni::Construct);
NIFRAGMAT_REVERSED("NiFragmentMaterial::GetRTTI", 0x00788380, &ni::GetRTTI);
NIFRAGMAT_REVERSED("NiFragmentMaterial::~NiFragmentMaterial", 0x00788390, &ni::Destruct);
NIFRAGMAT_REVERSED("NiFragmentMaterial::DeletingDestructor", 0x00788460, &ni::DeletingDestructor);
NIFRAGMAT_REVERSED("NiFragmentMaterial::SetupShaderPrograms", 0x00788490, &ni::SetupShaderPrograms);
