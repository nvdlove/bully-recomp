// NiShadowGeneratorComponent (vtables 0x98A55C / 0x98A430 at +8, 0x4C bytes): an entity's
// shadow generator, made for the entity's light (its "Scene Root Pointer", an NiDynamicEffect)
// once the shadow manager exists, and kept in step with the properties: "Cast Shadows",
// "Static Shadows", "Size Hint", "Strictly Observe Size Hint", "Unaffected Casters" /
// "Unaffected Receivers" (entities whose scene roots the generator leaves out), "Shadow
// Technique", "Render Backfaces", "Depth Bias", "Use Default Depth Bias" and the read-only
// "Shadow Generator Pointer". A property not made unique (its bit 3..10 in the flags) is read
// from the master component, up the chain (the GetX helpers). The interface methods get the
// interface pointer (+8); the helpers the object. The default constructor, the destructor and
// DestroyShadowGenerator are the other pass's (0x83D050, 0x83D160, 0x83D100).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiShadowGenerator.h"
#include "reversed.h"

namespace ni::ec {
namespace {

struct ShadowComponent : NiRefObject {
    re::Ptr32<void> ifaceVtable;                     // +0x8
    uint32_t flags;                        // +0xC (bit 0 the generator's settings stale, 1 the
                                           //  casters changed, 2 the receivers; 3..10 the
                                           //  properties made unique, in the order below)
    re::Ptr32<NiShadowGenerator> generator;          // +0x10 (a reference)
    re::Ptr32<NiObject> sceneRoot;                   // +0x14 (a reference: the light, NiDynamicEffect)
    re::Ptr32<ShadowComponent> master;               // +0x18 (a reference)
    NiTArray<void*> unaffectedCasters;     // +0x1C (NiTPrimitiveArray<NiEntityInterface*>)
    NiTArray<void*> unaffectedReceivers;   // +0x2C
    re::Ptr32<const char> technique;               // +0x3C ("NiStandardShadowTechnique")
    float depthBias;                       // +0x40
    uint16_t sizeHint;                     // +0x44 (0x400)
    uint8_t castShadows;                   // +0x46 (1)
    uint8_t staticShadows;                 // +0x47 (0)
    uint8_t strictlyObserveSizeHint;       // +0x48 (0)
    uint8_t renderBackfaces;               // +0x49 (1)
    uint8_t useDefaultDepthBias;           // +0x4A (1)
    uint8_t pad4B;
};
static_assert(offsetof(ShadowComponent, generator) == 0x10);
static_assert(offsetof(ShadowComponent, master) == 0x18);
static_assert(offsetof(ShadowComponent, unaffectedCasters) == 0x1C);
static_assert(offsetof(ShadowComponent, unaffectedReceivers) == 0x2C);
static_assert(offsetof(ShadowComponent, technique) == 0x3C);
static_assert(offsetof(ShadowComponent, sizeHint) == 0x44);
static_assert(offsetof(ShadowComponent, useDefaultDepthBias) == 0x4A);
static_assert(sizeof(ShadowComponent) == 0x4C);

enum : uint32_t {
    kStale = 0x1,
    kCastersChanged = 0x2,
    kReceiversChanged = 0x4,
    kUniqueCastShadows = 0x8,
    kUniqueStaticShadows = 0x10,
    kUniqueSizeHint = 0x20,
    kUniqueStrictlyObserveSizeHint = 0x40,
    kUniqueTechnique = 0x80,
    kUniqueRenderBackfaces = 0x100,
    kUniqueDepthBias = 0x200,
    kUniqueUseDefaultDepthBias = 0x400,
};

constexpr uintptr_t ShadowComponent_vtable = 0x0098A55C;
constexpr uintptr_t ShadowComponentIface_vtable = 0x0098A430;
const char* const kNiStandardShadowTechnique = reinterpret_cast<const char*>(0x009516C8);

GAME_VAR(NiFixedString, ERR_SCENE_ROOT_POINTER_NOT_FOUND, 0x020E5CAC);
GAME_VAR(NiFixedString, ERR_SCENE_ROOT_POINTER_NOT_DYNAMIC_EFFECT, 0x020E5CB0);
GAME_VAR(NiFixedString, ST_SHADOW_MAP_SIZE_HINT, 0x020E5CB4);    // the semantic types
GAME_VAR(NiFixedString, ST_SHADOW_MAP_TECHNIQUE, 0x020E5CB8);
GAME_VAR(NiFixedString, ST_SHADOW_MAP_DEPTH_BIAS, 0x020E5CBC);
GAME_VAR(NiFixedString, ms_kClassName, 0x020E5CC0);
GAME_VAR(NiFixedString, ms_kComponentName, 0x020E5CC4);           // "Shadow Generator"
GAME_VAR(NiFixedString, PROP_CAST_SHADOWS, 0x020E5CC8);
GAME_VAR(NiFixedString, PROP_STATIC_SHADOWS, 0x020E5CCC);
GAME_VAR(NiFixedString, PROP_SIZE_HINT, 0x020E5CD0);
GAME_VAR(NiFixedString, PROP_STRICTLY_OBSERVE_SIZE_HINT, 0x020E5CD4);
GAME_VAR(NiFixedString, PROP_UNAFFECTED_CASTERS, 0x020E5CD8);
GAME_VAR(NiFixedString, PROP_UNAFFECTED_RECEIVERS, 0x020E5CDC);
GAME_VAR(NiFixedString, PROP_SHADOW_TECHNIQUE, 0x020E5CE0);
GAME_VAR(NiFixedString, PROP_RENDER_BACKFACES, 0x020E5CE4);
GAME_VAR(NiFixedString, PROP_DEPTH_BIAS, 0x020E5CE8);
GAME_VAR(NiFixedString, PROP_USE_DEFAULT_DEPTH_BIAS, 0x020E5CEC);
GAME_VAR(NiFixedString, PROP_SHADOW_GENERATOR_POINTER, 0x020E5CF0);
// the descriptions, in the properties' order (0x20E5CF4..0x20E5D18)
constexpr uintptr_t kDescriptions = 0x020E5CF4;
GAME_VAR(NiFixedString, PROP_SCENE_ROOT_POINTER, 0x020E5D1C);
GAME_VAR(NiUniqueID, ms_kTemplateID, 0x020E5C98);
GAME_VAR(uint32_t, ms_kTemplateIDGuard, 0x020E5CA8);

// the class indices of NiDynamicEffect and NiNode (the RTTIs' bit sets, Bully's IsKindOf)
GAME_VAR(uint32_t, NiDynamicEffect_ClassIndex, 0x020E2C08);
GAME_VAR(uint32_t, NiNode_ClassIndex, 0x020E2550);
GAME_VAR(void*, NiShadowManager_ms_pkShadowManager, 0x020E2410);

// this file's helpers, through their addresses
GAME_FN(uint8_t, __thiscall, GetCastShadows, 0x0084A1F0, ShadowComponent* self);
GAME_FN(uint8_t, __thiscall, GetStaticShadows, 0x0084A220, ShadowComponent* self);
GAME_FN(uint16_t, __thiscall, GetSizeHint, 0x0084A250, ShadowComponent* self);
GAME_FN(uint8_t, __thiscall, GetStrictlyObserveSizeHint, 0x0084A280, ShadowComponent* self);
GAME_FN(const re::Ptr32<const char>*, __thiscall, GetShadowTechnique, 0x0084A2B0, ShadowComponent* self);
GAME_FN(uint8_t, __thiscall, GetRenderBackfaces, 0x0084A2E0, ShadowComponent* self);
GAME_FN(float, __thiscall, GetDepthBias, 0x0084A310, ShadowComponent* self);
GAME_FN(uint8_t, __thiscall, GetUseDefaultDepthBias, 0x0084A340, ShadowComponent* self);
GAME_FN(uint8_t, __thiscall, ShouldUpdateGenerator, 0x0084A370, ShadowComponent* self);
GAME_FN(void, __thiscall, ApplyInheritedValues, 0x0084A410, ShadowComponent* self);
GAME_FN(ShadowComponent*, __thiscall, Component_Construct, 0x0084B0D0, void* self, uint32_t casterCount,
        uint32_t casterGrowBy, uint32_t receiverCount, uint32_t receiverGrowBy, ShadowComponent* master);
GAME_FN(void, __thiscall, SetCastShadows, 0x0084B180, ShadowComponent* self, int value);
GAME_FN(void, __thiscall, SetStaticShadows, 0x0084B1D0, ShadowComponent* self, int value);
GAME_FN(void, __thiscall, SetSizeHint, 0x0084B210, ShadowComponent* self, uint32_t value);
GAME_FN(void, __thiscall, SetStrictlyObserveSizeHint, 0x0084B260, ShadowComponent* self, int value);
GAME_FN(void, __thiscall, SetShadowTechnique, 0x0084B2A0, ShadowComponent* self, const re::Ptr32<const char>* value);
GAME_FN(void, __thiscall, SetRenderBackfaces, 0x0084B2E0, ShadowComponent* self, int value);
GAME_FN(void, __thiscall, SetDepthBias, 0x0084B320, ShadowComponent* self, float value);
GAME_FN(void, __thiscall, SetUseDefaultDepthBias, 0x0084B380, ShadowComponent* self, int value);
GAME_FN(void, __thiscall, CreateShadowGenerator, 0x0084B3C0, ShadowComponent* self, NiObject* effect);
GAME_FN(void, __thiscall, ArrayUpdateSize, 0x008498F0, NiTArray<void*>* array);
GAME_FN(void, __thiscall, PointerArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
// the other pass's
GAME_FN(void, __thiscall, DestroyShadowGenerator, 0x0083D100, ShadowComponent* self);
GAME_FN(void, __thiscall, PrimitiveArray_Construct, 0x0082F290, void* self, uint32_t maxSize, uint32_t growBy);
// NiShadowGenerator, NiShadowManager, NiTPointerList, NiPointer
GAME_FN(NiShadowGenerator*, __thiscall, ShadowGenerator_Construct, 0x00773110, void* self, NiObject* effect);
GAME_FN(void, __cdecl, ShadowManager_AddShadowGenerator, 0x007574F0, NiShadowGenerator* generator);
GAME_FN(void, __thiscall, ShadowGenerator_AttachUnaffectedCasterNode, 0x00772690, NiShadowGenerator* self,
        NiObject* node);
GAME_FN(void, __thiscall, ShadowGenerator_AttachUnaffectedReceiverNode, 0x00772700, NiShadowGenerator* self,
        NiObject* node);
GAME_FN(void, __thiscall, ShadowGenerator_SetShadowTechnique, 0x0040E740, NiShadowGenerator* self,
        const re::Ptr32<const char>* name, int nameMatches);
GAME_FN(void, __thiscall, ShadowGenerator_SetDepthBiasToDefault, 0x0040DDB0, NiShadowGenerator* self);
GAME_FN(void, __thiscall, ShadowGenerator_MarkShadowMapsDirty, 0x00772930, NiShadowGenerator* self);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);

inline ShadowComponent* Self(void* iface) {
    return FromIface<ShadowComponent>(iface);
}

inline bool Is(const re::Ptr32<const char>* name, const re::Ptr32<const char>& property) {
    return *name == property;
}

// the value properties (not the arrays nor the pointer)
inline bool IsValueProperty(const re::Ptr32<const char>* name) {
    NiFixedString n = *name;
    return n == PROP_CAST_SHADOWS || n == PROP_STATIC_SHADOWS || n == PROP_SIZE_HINT ||
           n == PROP_STRICTLY_OBSERVE_SIZE_HINT || n == PROP_SHADOW_TECHNIQUE || n == PROP_RENDER_BACKFACES ||
           n == PROP_DEPTH_BIAS || n == PROP_USE_DEFAULT_DEPTH_BIAS;
}

inline bool IsArrayProperty(const re::Ptr32<const char>* name) {
    return *name == PROP_UNAFFECTED_CASTERS || *name == PROP_UNAFFECTED_RECEIVERS;
}

// any of its properties
inline bool IsProperty(const re::Ptr32<const char>* name) {
    return IsValueProperty(name) || IsArrayProperty(name) || *name == PROP_SHADOW_GENERATOR_POINTER;
}

// a value property's "made unique" bit (0 for the others)
inline uint32_t UniqueBit(const re::Ptr32<const char>* name) {
    NiFixedString n = *name;
    if (n == PROP_CAST_SHADOWS) {
        return kUniqueCastShadows;
    }
    if (n == PROP_STATIC_SHADOWS) {
        return kUniqueStaticShadows;
    }
    if (n == PROP_SIZE_HINT) {
        return kUniqueSizeHint;
    }
    if (n == PROP_STRICTLY_OBSERVE_SIZE_HINT) {
        return kUniqueStrictlyObserveSizeHint;
    }
    if (n == PROP_SHADOW_TECHNIQUE) {
        return kUniqueTechnique;
    }
    if (n == PROP_RENDER_BACKFACES) {
        return kUniqueRenderBackfaces;
    }
    if (n == PROP_DEPTH_BIAS) {
        return kUniqueDepthBias;
    }
    if (n == PROP_USE_DEFAULT_DEPTH_BIAS) {
        return kUniqueUseDefaultDepthBias;
    }
    return 0;
}

// NiObject::IsKindOf through the RTTI's class bit set: the object, or null
inline NiObject* KindOf(NiObject* object, uint32_t classIndex) {
    const uint32_t* rtti = re::PtrCast<const uint32_t*(__thiscall*)(NiObject*)>(VtableOf(object)[2])(object);
    return ((1u << (classIndex & 31)) & rtti[3 + (classIndex >> 5)]) != 0 ? object : nullptr;
}

// ---- NiTPrimitiveArray<T*>::SetSize (folded for every pointer element type: 485 callers)

// the slots past a smaller size cleared (counted out of the used ones), then the array moved to
// a new block of `size` (the new slots zero; none for 0) and the old one freed. 16-bit counts,
// as the array keeps them
void __fastcall PointerArraySetSize(NiTArray<void*>* self, re::Edx, uint32_t size) {
    if (size == self->maxSize) {
        return;
    }
    if (size < self->size) {
        for (uint16_t i = uint16_t(size); i < self->size; i++) {
            if (self->base[i] != nullptr) {
                self->base[i] = nullptr;
                self->esize--;
            }
        }
        self->size = uint16_t(size);
    }
    re::Ptr32<void>* old = self->base;
    self->maxSize = uint16_t(size);
    if (size != 0) {
        self->base = static_cast<re::Ptr32<void>*>(NiMalloc(uint32_t(uint16_t(size)) * 4));
        for (uint16_t i = 0; i < self->size; i++) {
            self->base[i] = old[i];
        }
        for (uint16_t i = self->size; i < self->maxSize; i++) {
            self->base[i] = nullptr;
        }
    } else {
        self->base = nullptr;
    }
    NiFree(old);
}

// ---- the getters up the master chain (the first component with the bit set, or the last)

template <uint32_t Bit>
inline ShadowComponent* Owner(ShadowComponent* self) {
    while ((self->flags & Bit) == 0 && self->master != nullptr) {
        self = self->master;
    }
    return self;
}

uint8_t __fastcall GetCastShadows_(ShadowComponent* self, re::Edx) {
    return Owner<kUniqueCastShadows>(self)->castShadows;
}
uint8_t __fastcall GetStaticShadows_(ShadowComponent* self, re::Edx) {
    return Owner<kUniqueStaticShadows>(self)->staticShadows;
}
uint16_t __fastcall GetSizeHint_(ShadowComponent* self, re::Edx) {
    return Owner<kUniqueSizeHint>(self)->sizeHint;
}
uint8_t __fastcall GetStrictlyObserveSizeHint_(ShadowComponent* self, re::Edx) {
    return Owner<kUniqueStrictlyObserveSizeHint>(self)->strictlyObserveSizeHint;
}
const re::Ptr32<const char>* __fastcall GetShadowTechnique_(ShadowComponent* self, re::Edx) {
    return &Owner<kUniqueTechnique>(self)->technique;
}
uint8_t __fastcall GetRenderBackfaces_(ShadowComponent* self, re::Edx) {
    return Owner<kUniqueRenderBackfaces>(self)->renderBackfaces;
}
float __fastcall GetDepthBias_(ShadowComponent* self, re::Edx) {
    return Owner<kUniqueDepthBias>(self)->depthBias;
}
uint8_t __fastcall GetUseDefaultDepthBias_(ShadowComponent* self, re::Edx) {
    return Owner<kUniqueUseDefaultDepthBias>(self)->useDefaultDepthBias;
}

// the generator needs its settings again: marked stale, or an inherited value differs from the
// one kept (the first asked of the master directly, the rest of itself)
uint8_t __fastcall ShouldUpdateGenerator_(ShadowComponent* self, re::Edx) {
    if ((self->flags & kStale) != 0) {
        return 1;
    }
    uint8_t cast = ((self->flags & kUniqueCastShadows) == 0 && self->master != nullptr) ? GetCastShadows(self->master)
                                                                                         : self->castShadows;
    if (cast != self->castShadows || GetStaticShadows(self) != self->staticShadows ||
        GetSizeHint(self) != self->sizeHint || GetStrictlyObserveSizeHint(self) != self->strictlyObserveSizeHint ||
        *GetShadowTechnique(self) != self->technique || GetRenderBackfaces(self) != self->renderBackfaces) {
        return 1;
    }
    float bias = GetDepthBias(self);
    if (!(self->depthBias == bias) || GetUseDefaultDepthBias(self) != self->useDefaultDepthBias) {
        return 1;
    }
    return 0;
}


// the inherited values copied into its own (those not made unique: from the master's chain)
void __fastcall ApplyInheritedValues_(ShadowComponent* self, re::Edx) {
    uint32_t flags = self->flags;
    self->castShadows = ((flags & kUniqueCastShadows) == 0 && self->master != nullptr) ? GetCastShadows(self->master)
                                                                                        : self->castShadows;
    self->staticShadows = ((flags & kUniqueStaticShadows) == 0 && self->master != nullptr)
                              ? GetStaticShadows(self->master)
                              : self->staticShadows;
    self->sizeHint = ((flags & kUniqueSizeHint) == 0 && self->master != nullptr) ? GetSizeHint(self->master)
                                                                                 : self->sizeHint;
    self->strictlyObserveSizeHint = ((flags & kUniqueStrictlyObserveSizeHint) == 0 && self->master != nullptr)
                                        ? GetStrictlyObserveSizeHint(self->master)
                                        : self->strictlyObserveSizeHint;
    const re::Ptr32<const char>* technique = ((flags & kUniqueTechnique) == 0 && self->master != nullptr)
                                         ? GetShadowTechnique(self->master)
                                         : &self->technique;
    FixedString_Assign(&self->technique, technique);
    flags = self->flags;
    self->renderBackfaces = ((flags & kUniqueRenderBackfaces) == 0 && self->master != nullptr)
                                ? GetRenderBackfaces(self->master)
                                : self->renderBackfaces;
    self->depthBias = ((flags & kUniqueDepthBias) == 0 && self->master != nullptr) ? GetDepthBias(self->master)
                                                                                   : self->depthBias;
    self->useDefaultDepthBias = ((flags & kUniqueUseDefaultDepthBias) == 0 && self->master != nullptr)
                                    ? GetUseDefaultDepthBias(self->master)
                                    : self->useDefaultDepthBias;
}

// ---- the static names (the exe's texts)

constexpr StaticString kNames[] = {
    {0x020E5CAC, 0x0098B4AC},   // "Scene Root Pointer not found."
    {0x020E5CB0, 0x0098B480},   // "Scene Root Pointer not an NiDynamicEffect."
    {0x020E5CB4, 0x0098B468},   // "Shadow Map Size Hint"
    {0x020E5CB8, 0x0098B450},   // "Shadow Map Technique"
    {0x020E5CBC, 0x0098B438},   // "Shadow Map Depth Bias"
    {0x020E5CC0, 0x008FFE34},   // "NiShadowGeneratorComponent"
    {0x020E5CC4, 0x0098B424},   // "Shadow Generator"
    {0x020E5CC8, 0x008FFE24},   // "Cast Shadows"
    {0x020E5CCC, 0x008FFE14},   // "Static Shadows"
    {0x020E5CD0, 0x008FFE08},   // "Size Hint"
    {0x020E5CD4, 0x008FFDEC},   // "Strictly Observe Size Hint"
    {0x020E5CD8, 0x008FFDD8},   // "Unaffected Casters"
    {0x020E5CDC, 0x008FFDC0},   // "Unaffected Receivers"
    {0x020E5CE0, 0x008FFDAC},   // "Shadow Technique"
    {0x020E5CE4, 0x008FFD98},   // "Render Backfaces"
    {0x020E5CE8, 0x008FFD74},   // "Depth Bias"
    {0x020E5CEC, 0x008FFD80},   // "Use Default Depth Bias"
    {0x020E5CF0, 0x0098B408},   // "Shadow Generator Pointer"
    {0x020E5CF4, 0x0098B3D4},   // "Whether or not the shadow generator casts shadows."
    {0x020E5CF8, 0x0098B350},   // "Whether or not the shadow generator will generate shadow maps..."
    {0x020E5CFC, 0x0098B2F8},   // "A hint for the size of the shadow map..."
    {0x020E5D00, 0x0098B248},   // "Indicates whether or not a new shadow map should be created..."
    {0x020E5D04, 0x0098B1F8},   // "A collection of entities that will not cast shadows..."
    {0x020E5D08, 0x0098B1A0},   // "A collection of entities that will not receive shadows..."
    {0x020E5D0C, 0x0098B160},   // "The shadow technique to use when generating shadow maps."
    {0x020E5D10, 0x0098B0B0},   // "Whether or not the shadow generator will use backward-facing..."
    {0x020E5D14, 0x0098B018},   // "A bias value used to offset the shadow map depth values..."
    {0x020E5D18, 0x0098AF60},   // "Whether or not to allow the shadowing system to set the depth..."
    {0x020E5D1C, 0x0098AF48},   // "Scene Root Pointer"
};

void __cdecl SDMInit() {
    SetStaticStrings(kNames);
}

void __cdecl SDMShutdown() {
    ClearStaticStrings(kNames);
}

inline re::Ptr32<const char>* Description(int index) {
    return reinterpret_cast<re::Ptr32<const char>*>(kDescriptions + 4 * index);
}

// ---- the interface's methods

NiUniqueID* __fastcall GetTemplateID_(void*, re::Edx, NiUniqueID* result) {
    return ReturnTemplateID(result, ms_kTemplateIDGuard, ms_kTemplateID, 0x8F29B1C1, 0x41E0D71B, 0x855458AE,
                            0x339E064B);
}

// resettable: a master, and the value made unique
uint8_t __fastcall CanResetProperty(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* canReset) {
    ShadowComponent* self = Self(iface);
    uint32_t bit = UniqueBit(name);
    if (bit != 0) {
        *canReset = (self->master != nullptr && (self->flags & bit) != 0) ? 1 : 0;
        return 1;
    }
    if (IsArrayProperty(name) || Is(name, PROP_SHADOW_GENERATOR_POINTER)) {
        *canReset = 0;
        return 1;
    }
    return 0;
}

// (the byte CanResetProperty writes is the name's slot)
uint8_t __fastcall ResetProperty(void* iface, re::Edx, const re::Ptr32<const char>* name) {
    uint8_t canReset = uint8_t(reinterpret_cast<uintptr_t>(name));
    uint8_t result = V(iface, slot::CanResetProperty, name, &canReset);
    if (!result || canReset == 0) {
        return 0;
    }
    uint32_t bit = UniqueBit(name);
    if (bit == 0) {
        return 0;
    }
    Self(iface)->flags &= ~bit;
    return 1;
}

uint8_t __fastcall MakePropertyUnique(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* madeUnique) {
    uint8_t canReset = uint8_t(reinterpret_cast<uintptr_t>(name));
    if (!V(iface, slot::CanResetProperty, name, &canReset)) {
        return 0;
    }
    ShadowComponent* self = Self(iface);
    if (canReset != 0 || self->master == nullptr) {
        *madeUnique = 0;
        return 1;
    }
    uint32_t bit = UniqueBit(name);
    if (bit == 0) {
        *madeUnique = 0;
        return 1;
    }
    self->flags |= bit;
    *madeUnique = 1;
    return 1;
}

// unique: with a master, as CanResetProperty says (its result not looked at); the arrays and
// the pointer always
uint8_t __fastcall IsPropertyUnique(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* unique) {
    if (IsValueProperty(name)) {
        if (At<void*>(iface, 0x10) != nullptr) {
            V(iface, slot::CanResetProperty, name, unique);
            return 1;
        }
    } else if (!IsArrayProperty(name) && !Is(name, PROP_SHADOW_GENERATOR_POINTER)) {
        return 0;
    }
    *unique = 1;
    return 1;
}

// serializable: unique and not read-only (the pointer never). The read-only byte starts as the
// high byte of the pushed `this`
uint8_t __fastcall IsPropertySerializable(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* serializable) {
    if (!IsValueProperty(name) && !IsArrayProperty(name)) {
        if (Is(name, PROP_SHADOW_GENERATOR_POINTER)) {
            *serializable = 0;
            return 1;
        }
        return 0;
    }
    uint8_t unique = uint8_t(reinterpret_cast<uintptr_t>(name));
    V(iface, slot::IsPropertyUnique, name, &unique);
    uint8_t readOnly = uint8_t(reinterpret_cast<uintptr_t>(iface) >> 24);
    V(iface, slot::IsPropertyReadOnly, name, &readOnly);
    *serializable = (unique != 0 && readOnly == 0) ? 1 : 0;
    return 1;
}

// inheritable: the values (not the arrays nor the pointer)
uint8_t __fastcall IsPropertyInheritable(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* inheritable) {
    if (IsValueProperty(name)) {
        *inheritable = 1;
        return 1;
    }
    if (IsArrayProperty(name) || Is(name, PROP_SHADOW_GENERATOR_POINTER)) {
        *inheritable = 0;
        return 1;
    }
    return 0;
}

uint8_t __fastcall IsExternalAssetPath(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* isPath) {
    if (!IsProperty(name)) {
        return 0;
    }
    *isPath = 0;
    return 1;
}

// one, or the arrays' counts
uint8_t __fastcall GetElementCount(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t* count) {
    ShadowComponent* self = Self(iface);
    if (IsValueProperty(name) || Is(name, PROP_SHADOW_GENERATOR_POINTER)) {
        *count = 1;
        return 1;
    }
    if (Is(name, PROP_UNAFFECTED_CASTERS)) {
        *count = self->unaffectedCasters.size;
        return 1;
    }
    if (Is(name, PROP_UNAFFECTED_RECEIVERS)) {
        *count = self->unaffectedReceivers.size;
        return 1;
    }
    return 0;
}

uint8_t __fastcall IsCollection_(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* isCollection) {
    if (IsValueProperty(name) || Is(name, PROP_SHADOW_GENERATOR_POINTER)) {
        *isCollection = 0;
        return 1;
    }
    if (IsArrayProperty(name)) {
        *isCollection = 1;
        return 1;
    }
    return 0;
}

// the pointer
uint8_t __fastcall GetObject_(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<NiObject>* value, uint32_t index) {
    if (index == 0 && Is(name, PROP_SHADOW_GENERATOR_POINTER)) {
        *value = Self(iface)->generator;
        return 1;
    }
    return 0;
}

re::Ptr32<const char>* __fastcall GetClassName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnString(result, ms_kClassName);
}

re::Ptr32<const char>* __fastcall GetName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnString(result, ms_kComponentName);
}

// the property's own name (the pointer: none)
uint8_t __fastcall GetDisplayName(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    if (IsValueProperty(name) || IsArrayProperty(name)) {
        FixedString_Assign(value, name);
        return 1;
    }
    if (Is(name, PROP_SHADOW_GENERATOR_POINTER)) {
        FixedString_SetString(value, nullptr);
        return 1;
    }
    return 0;
}

// the primitive types (and the semantic types: the three map settings have their own)
template <bool Semantic>
uint8_t __fastcall GetType(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    NiFixedString n = *name;
    const re::Ptr32<const char>* type;
    if (n == PROP_CAST_SHADOWS || n == PROP_STATIC_SHADOWS) {
        type = &PT_BOOL;
    } else if (n == PROP_SIZE_HINT) {
        type = Semantic ? &ST_SHADOW_MAP_SIZE_HINT : &PT_USHORT;
    } else if (n == PROP_STRICTLY_OBSERVE_SIZE_HINT) {
        type = &PT_BOOL;
    } else if (n == PROP_UNAFFECTED_CASTERS || n == PROP_UNAFFECTED_RECEIVERS) {
        type = &PT_ENTITYPOINTER;
    } else if (n == PROP_SHADOW_TECHNIQUE) {
        type = Semantic ? &ST_SHADOW_MAP_TECHNIQUE : &PT_STRING;
    } else if (n == PROP_RENDER_BACKFACES) {
        type = &PT_BOOL;
    } else if (n == PROP_DEPTH_BIAS) {
        type = Semantic ? &ST_SHADOW_MAP_DEPTH_BIAS : &PT_FLOAT;
    } else if (n == PROP_USE_DEFAULT_DEPTH_BIAS) {
        type = &PT_BOOL;
    } else if (n == PROP_SHADOW_GENERATOR_POINTER) {
        type = &PT_NIOBJECTPOINTER;
    } else {
        return 0;
    }
    FixedString_Assign(value, type);
    return 1;
}

// the descriptions (the pointer: none)
uint8_t __fastcall GetDescription(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    const re::Ptr32<const char>* const properties[] = {
        &PROP_CAST_SHADOWS,       &PROP_STATIC_SHADOWS,       &PROP_SIZE_HINT,
        &PROP_STRICTLY_OBSERVE_SIZE_HINT, &PROP_UNAFFECTED_CASTERS, &PROP_UNAFFECTED_RECEIVERS,
        &PROP_SHADOW_TECHNIQUE,   &PROP_RENDER_BACKFACES,     &PROP_DEPTH_BIAS,
        &PROP_USE_DEFAULT_DEPTH_BIAS,
    };
    NiFixedString n = *name;
    for (int i = 0; i < 10; i++) {
        if (n == *properties[i]) {
            FixedString_Assign(value, Description(i));
            return 1;
        }
    }
    if (n == PROP_SHADOW_GENERATOR_POINTER) {
        FixedString_SetString(value, nullptr);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetCategory(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* category) {
    if (!IsProperty(name)) {
        return 0;
    }
    FixedString_Assign(category, &ms_kComponentName);
    return 1;
}

// read-only: the pointer; the depth bias while the default one is used
uint8_t __fastcall IsPropertyReadOnly(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* readOnly) {
    NiFixedString n = *name;
    if (n == PROP_CAST_SHADOWS || n == PROP_STATIC_SHADOWS || n == PROP_SIZE_HINT ||
        n == PROP_STRICTLY_OBSERVE_SIZE_HINT || n == PROP_UNAFFECTED_CASTERS || n == PROP_UNAFFECTED_RECEIVERS ||
        n == PROP_SHADOW_TECHNIQUE || n == PROP_RENDER_BACKFACES || n == PROP_USE_DEFAULT_DEPTH_BIAS) {
        *readOnly = 0;
        return 1;
    }
    if (n == PROP_DEPTH_BIAS) {
        *readOnly = GetUseDefaultDepthBias(Self(iface));
        return 1;
    }
    if (n == PROP_SHADOW_GENERATOR_POINTER) {
        *readOnly = 1;
        return 1;
    }
    return 0;
}

// an array cut to `count` (the entries past it cleared through SetPropertyData first)
uint8_t __fastcall SetElementCount(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t count, uint8_t* countSet) {
    ShadowComponent* self = Self(iface);
    if (IsValueProperty(name) || Is(name, PROP_SHADOW_GENERATOR_POINTER)) {
        *countSet = 0;
        return 1;
    }
    NiTArray<void*>* array;
    if (Is(name, PROP_UNAFFECTED_CASTERS)) {
        array = &self->unaffectedCasters;
    } else if (Is(name, PROP_UNAFFECTED_RECEIVERS)) {
        array = &self->unaffectedReceivers;
    } else {
        return 0;
    }
    uint32_t size = array->size;
    for (uint32_t i = count; i < size; i++) {
        V(iface, slot::SetEntity, name, static_cast<void*>(nullptr), i);
    }
    PointerArray_SetSize(array, count);
    *countSet = 1;
    return 1;
}

// the arrays' entities (null past the end)
uint8_t __fastcall GetEntity(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<void>* value, uint32_t index) {
    ShadowComponent* self = Self(iface);
    NiTArray<void*>* array;
    if (Is(name, PROP_UNAFFECTED_CASTERS)) {
        array = &self->unaffectedCasters;
    } else if (Is(name, PROP_UNAFFECTED_RECEIVERS)) {
        array = &self->unaffectedReceivers;
    } else {
        return 0;
    }
    *value = index < array->size ? array->base[index] : nullptr;
    return 1;
}

// an array's entry set (grown to hold it, the count trimmed after), marked changed
uint8_t __fastcall SetEntity(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<void> value, uint32_t index) {
    ShadowComponent* self = Self(iface);
    NiTArray<void*>* array;
    uint32_t changed;
    if (Is(name, PROP_UNAFFECTED_CASTERS)) {
        array = &self->unaffectedCasters;
        changed = kCastersChanged;
    } else if (Is(name, PROP_UNAFFECTED_RECEIVERS)) {
        array = &self->unaffectedReceivers;
        changed = kReceiversChanged;
    } else {
        return 0;
    }
    if (array->maxSize <= index) {
        PointerArray_SetSize(array, array->growBy + index);
    }
    PointerArray_SetAt(array, index, &value);
    ArrayUpdateSize(array);
    self->flags |= changed;
    return 1;
}

uint8_t __fastcall GetString(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value, uint32_t index) {
    if (index == 0 && Is(name, PROP_SHADOW_TECHNIQUE)) {
        FixedString_Assign(value, GetShadowTechnique(Self(iface)));
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetUShort(void* iface, re::Edx, const re::Ptr32<const char>* name, uint16_t* value, uint32_t index) {
    if (index == 0 && Is(name, PROP_SIZE_HINT)) {
        *value = GetSizeHint(Self(iface));
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    ShadowComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_CAST_SHADOWS) {
        *value = GetCastShadows(self);
        return 1;
    }
    if (n == PROP_STATIC_SHADOWS) {
        *value = GetStaticShadows(self);
        return 1;
    }
    if (n == PROP_STRICTLY_OBSERVE_SIZE_HINT) {
        *value = GetStrictlyObserveSizeHint(self);
        return 1;
    }
    if (n == PROP_RENDER_BACKFACES) {
        *value = GetRenderBackfaces(self);
        return 1;
    }
    if (n == PROP_USE_DEFAULT_DEPTH_BIAS) {
        *value = GetUseDefaultDepthBias(self);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetFloat(void* iface, re::Edx, const re::Ptr32<const char>* name, float* value, uint32_t index) {
    if (index == 0 && Is(name, PROP_DEPTH_BIAS)) {
        *value = GetDepthBias(Self(iface));
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetString(void* iface, re::Edx, const re::Ptr32<const char>* name, const re::Ptr32<const char>* value,
                             uint32_t index) {
    if (index == 0 && Is(name, PROP_SHADOW_TECHNIQUE)) {
        SetShadowTechnique(Self(iface), value);
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetUShort(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t value, uint32_t index) {
    if (index == 0 && Is(name, PROP_SIZE_HINT)) {
        SetSizeHint(Self(iface), value);
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, int value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    ShadowComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_CAST_SHADOWS) {
        SetCastShadows(self, value);
        return 1;
    }
    if (n == PROP_STATIC_SHADOWS) {
        SetStaticShadows(self, value);
        return 1;
    }
    if (n == PROP_STRICTLY_OBSERVE_SIZE_HINT) {
        SetStrictlyObserveSizeHint(self, value);
        return 1;
    }
    if (n == PROP_RENDER_BACKFACES) {
        SetRenderBackfaces(self, value);
        return 1;
    }
    if (n == PROP_USE_DEFAULT_DEPTH_BIAS) {
        SetUseDefaultDepthBias(self, value);
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetFloat(void* iface, re::Edx, const re::Ptr32<const char>* name, float value, uint32_t index) {
    if (index == 0 && Is(name, PROP_DEPTH_BIAS)) {
        SetDepthBias(Self(iface), value);
        return 1;
    }
    return 0;
}

// ---- the object's own

ShadowComponent* __fastcall ComponentConstruct(ShadowComponent* self, re::Edx, uint32_t casterCount,
                                               uint32_t casterGrowBy, uint32_t receiverCount,
                                               uint32_t receiverGrowBy, ShadowComponent* master) {
    ConstructBases(self);
    self->vtable = reinterpret_cast<void*>(ShadowComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(ShadowComponentIface_vtable);
    self->flags = 0;
    self->generator = nullptr;
    self->sceneRoot = nullptr;
    self->master = master;
    AddRef(master);
    PrimitiveArray_Construct(&self->unaffectedCasters, casterCount, casterGrowBy);
    PrimitiveArray_Construct(&self->unaffectedReceivers, receiverCount, receiverGrowBy);
    self->technique = ni::scene::StringTable_AddString(kNiStandardShadowTechnique);
    self->depthBias = 0.0f;
    self->castShadows = 1;
    self->renderBackfaces = 1;
    self->useDefaultDepthBias = 1;
    self->staticShadows = 0;
    self->strictlyObserveSizeHint = 0;
    self->sizeHint = 0x400;
    return self;
}

// the setters: a change from the inherited value kept as its own (made unique), the generator
// remade (destroyed: Update makes the new one) or its settings marked stale
void __fastcall SetCastShadows_(ShadowComponent* self, re::Edx, int value) {
    uint32_t flags = self->flags;
    uint8_t current = ((flags & kUniqueCastShadows) == 0 && self->master != nullptr) ? GetCastShadows(self->master)
                                                                                      : self->castShadows;
    if (current != uint8_t(value)) {
        self->castShadows = uint8_t(value);
        self->flags = flags | kUniqueCastShadows;
        if (uint8_t(value) == 0) {
            DestroyShadowGenerator(self);
        }
    }
}

void __fastcall SetStaticShadows_(ShadowComponent* self, re::Edx, int value) {
    uint32_t flags = self->flags;
    uint8_t current = ((flags & kUniqueStaticShadows) == 0 && self->master != nullptr)
                          ? GetStaticShadows(self->master)
                          : self->staticShadows;
    if (current != uint8_t(value)) {
        self->staticShadows = uint8_t(value);
        self->flags = flags | kUniqueStaticShadows | kStale;
    }
}

void __fastcall SetSizeHint_(ShadowComponent* self, re::Edx, uint32_t value) {
    uint32_t flags = self->flags;
    uint16_t current = ((flags & kUniqueSizeHint) == 0 && self->master != nullptr) ? GetSizeHint(self->master)
                                                                                   : self->sizeHint;
    if (current != uint16_t(value)) {
        self->sizeHint = uint16_t(value);
        self->flags = flags | kUniqueSizeHint;
        DestroyShadowGenerator(self);
    }
}

void __fastcall SetStrictlyObserveSizeHint_(ShadowComponent* self, re::Edx, int value) {
    uint32_t flags = self->flags;
    uint8_t current = ((flags & kUniqueStrictlyObserveSizeHint) == 0 && self->master != nullptr)
                          ? GetStrictlyObserveSizeHint(self->master)
                          : self->strictlyObserveSizeHint;
    if (current != uint8_t(value)) {
        self->strictlyObserveSizeHint = uint8_t(value);
        self->flags = flags | kUniqueStrictlyObserveSizeHint;
        DestroyShadowGenerator(self);
    }
}

void __fastcall SetShadowTechnique_(ShadowComponent* self, re::Edx, const re::Ptr32<const char>* value) {
    const re::Ptr32<const char>* current = ((self->flags & kUniqueTechnique) == 0 && self->master != nullptr)
                                       ? GetShadowTechnique(self->master)
                                       : &self->technique;
    if (*current != *value) {
        FixedString_Assign(&self->technique, value);
        self->flags |= kUniqueTechnique | kStale;
    }
}

void __fastcall SetRenderBackfaces_(ShadowComponent* self, re::Edx, int value) {
    uint32_t flags = self->flags;
    uint8_t current = ((flags & kUniqueRenderBackfaces) == 0 && self->master != nullptr)
                          ? GetRenderBackfaces(self->master)
                          : self->renderBackfaces;
    if (current != uint8_t(value)) {
        self->renderBackfaces = uint8_t(value);
        self->flags = flags | kUniqueRenderBackfaces | kStale;
    }
}

void __fastcall SetDepthBias_(ShadowComponent* self, re::Edx, float value) {
    uint32_t flags = self->flags;
    float current = ((flags & kUniqueDepthBias) == 0 && self->master != nullptr) ? GetDepthBias(self->master)
                                                                                 : self->depthBias;
    if (value != current) {
        self->depthBias = value;
        self->flags = flags | kUniqueDepthBias | kStale;
    }
}

void __fastcall SetUseDefaultDepthBias_(ShadowComponent* self, re::Edx, int value) {
    uint32_t flags = self->flags;
    uint8_t current = ((flags & kUniqueUseDefaultDepthBias) == 0 && self->master != nullptr)
                          ? GetUseDefaultDepthBias(self->master)
                          : self->useDefaultDepthBias;
    if (current != uint8_t(value)) {
        self->useDefaultDepthBias = uint8_t(value);
        self->flags = flags | kUniqueUseDefaultDepthBias | kStale;
    }
}

// a new generator for the light (the old one destroyed first), given to the shadow manager
void __fastcall CreateShadowGenerator_(ShadowComponent* self, re::Edx, NiObject* effect) {
    if (self->generator != nullptr) {
        DestroyShadowGenerator(self);
    }
    void* memory = NiNew(sizeof(NiShadowGenerator));
    NiShadowGenerator* generator = memory != nullptr ? ShadowGenerator_Construct(memory, effect) : nullptr;
    NiShadowGenerator* old = self->generator;
    if (old != generator) {
        Release(old);
        self->generator = generator;
        AddRef(generator);
    }
    ShadowManager_AddShadowGenerator(self->generator);
}

// NiTPrimitiveArray::SetAt as the clone inlines it (no growing: the copy was made as large)
inline void CopyEntry(NiTArray<void*>* to, uint32_t index, void* value) {
    if (index < to->size) {
        if (value == nullptr) {
            if (to->base[index] != nullptr) {
                to->esize--;
            }
        } else if (to->base[index] == nullptr) {
            to->esize++;
        }
    } else {
        to->size = uint16_t(index + 1);
        if (value != nullptr) {
            to->esize++;
        }
    }
    to->base[index] = value;
}

// a copy: its flags, values and arrays; its master this (inheriting) or its own
void* __fastcall Clone_(void* iface, re::Edx, int inherit) {
    ShadowComponent* self = Self(iface);
    ShadowComponent* master = re::Flag(inherit) ? self : static_cast<ShadowComponent*>(self->master);
    void* memory = NiNew(sizeof(ShadowComponent));
    ShadowComponent* clone = memory == nullptr
                                 ? nullptr
                                 : Component_Construct(memory, self->unaffectedCasters.maxSize,
                                                       self->unaffectedCasters.growBy,
                                                       self->unaffectedReceivers.maxSize,
                                                       self->unaffectedReceivers.growBy, master);
    clone->flags = self->flags;
    FixedString_Assign(&clone->technique, &self->technique);
    clone->depthBias = self->depthBias;
    clone->sizeHint = self->sizeHint;
    clone->castShadows = self->castShadows;
    clone->staticShadows = self->staticShadows;
    clone->strictlyObserveSizeHint = self->strictlyObserveSizeHint;
    clone->renderBackfaces = self->renderBackfaces;
    clone->useDefaultDepthBias = self->useDefaultDepthBias;
    for (uint32_t i = 0; i < self->unaffectedCasters.size; i++) {
        CopyEntry(&clone->unaffectedCasters, i, self->unaffectedCasters.base[i]);
    }
    for (uint32_t i = 0; i < self->unaffectedReceivers.size; i++) {
        CopyEntry(&clone->unaffectedReceivers, i, self->unaffectedReceivers.base[i]);
    }
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(clone) + 8);
}

void* __fastcall GetMasterComponent(void* iface, re::Edx) {
    ShadowComponent* master = Self(iface)->master;
    return master != nullptr ? reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(master) + 8) : nullptr;
}

// none: every property made unique, the master let go; else the new master held
void __fastcall SetMasterComponent(void* iface, re::Edx, void* masterIface) {
    ShadowComponent* self = Self(iface);
    if (masterIface == nullptr) {
        StringSet names{FixedStringArray_New(10), 10, 0};
        V<void>(iface, slot::GetPropertyNames, &names);
        uint32_t count = names.used;
        for (uint32_t i = 0; i < count; i++) {
            uint8_t madeUnique = 0;   // (its slot: the argument's, the null master)
            V(iface, slot::MakePropertyUnique, &names.base[i], &madeUnique);
        }
        ShadowComponent* master = self->master;
        if (master != nullptr) {
            Release(master);
            self->master = nullptr;
        }
        if (names.base != nullptr) {
            FixedStringArray_Delete(names.base, 3);
        }
        return;
    }
    ShadowComponent* master = FromIface<ShadowComponent>(masterIface);
    if (self->master != master) {
        Release(self->master);
        self->master = master;
        AddRef(master);
    }
}

// the scene root pointer (the light) is what it depends on
void __fastcall GetDependentPropertyNames(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_SCENE_ROOT_POINTER);
}

void __fastcall GetPropertyNames_(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_CAST_SHADOWS);
    StringSetAdd(names, &PROP_STATIC_SHADOWS);
    StringSetAdd(names, &PROP_SIZE_HINT);
    StringSetAdd(names, &PROP_STRICTLY_OBSERVE_SIZE_HINT);
    StringSetAdd(names, &PROP_UNAFFECTED_CASTERS);
    StringSetAdd(names, &PROP_UNAFFECTED_RECEIVERS);
    StringSetAdd(names, &PROP_SHADOW_TECHNIQUE);
    StringSetAdd(names, &PROP_RENDER_BACKFACES);
    StringSetAdd(names, &PROP_DEPTH_BIAS);
    StringSetAdd(names, &PROP_USE_DEFAULT_DEPTH_BIAS);
    StringSetAdd(names, &PROP_SHADOW_GENERATOR_POINTER);
}

// the error handler told (NiEntityErrorInterface slot 2: message, description, the entity's
// name, the property's); the empty description lives in the argument slot the game reuses
void ReportError(void* errors, void* entity, const re::Ptr32<const char>* message) {
    NiFixedString description = NullFixedString;
    NiFixedString entityName;
    V<re::Ptr32<const char>*>(entity, slot::GetName, &entityName);
    V<void>(errors, 2, message, &description, &entityName, &PROP_SCENE_ROOT_POINTER);
    FixedStringRelease(&entityName);
    FixedStringRelease(&description);
}

// an entity's scene root (when it's an NiNode) given to the generator as an unaffected caster
// or receiver; a null one means not all of them were there
inline void AttachUnaffected(ShadowComponent* self, NiObject* object, bool casters, uint8_t& allFound) {
    if (object == nullptr) {
        allFound = 0;
        return;
    }
    NiObject* node = KindOf(object, NiNode_ClassIndex);
    if (node != nullptr) {
        if (casters) {
            ShadowGenerator_AttachUnaffectedCasterNode(self->generator, node);
        } else {
            ShadowGenerator_AttachUnaffectedReceiverNode(self->generator, node);
        }
    }
}

// the light from the entity's "Scene Root Pointer" (an NiDynamicEffect, else an error); a
// generator made for it while shadows are cast and the shadow manager exists, its settings
// brought up to date, its unaffected casters / receivers rebuilt when they changed
void __fastcall Update(void* iface, re::Edx, void* entity, float, void* errors, void*) {
    ShadowComponent* self = Self(iface);
    NiObject* sceneRoot = nullptr;
    if (!V(entity, slot::GetObject, &PROP_SCENE_ROOT_POINTER, &sceneRoot, 0u)) {
        ReportError(errors, entity, &ERR_SCENE_ROOT_POINTER_NOT_FOUND);
    }
    if (sceneRoot == nullptr) {
        return;
    }
    NiObject* effect = KindOf(sceneRoot, NiDynamicEffect_ClassIndex);
    if (effect == nullptr) {
        ReportError(errors, entity, &ERR_SCENE_ROOT_POINTER_NOT_DYNAMIC_EFFECT);
        return;
    }
    if (self->sceneRoot != effect) {
        DestroyShadowGenerator(self);
        Pointer_Assign(&self->sceneRoot, effect);
    }
    uint8_t cast = ((self->flags & kUniqueCastShadows) == 0 && self->master != nullptr) ? GetCastShadows(self->master)
                                                                                         : self->castShadows;
    if (cast != self->castShadows || GetSizeHint(self) != self->sizeHint ||
        GetStrictlyObserveSizeHint(self) != self->strictlyObserveSizeHint) {
        DestroyShadowGenerator(self);
    }
    if (self->generator == nullptr && GetCastShadows(self) && NiShadowManager_ms_pkShadowManager != nullptr) {
        CreateShadowGenerator(self, effect);
        self->flags |= kStale | kCastersChanged;
        self->flags |= kReceiversChanged;
    }
    NiShadowGenerator* generator = self->generator;
    if (generator == nullptr) {
        return;
    }
    if (ShouldUpdateGenerator(self)) {
        if (GetStaticShadows(self)) {
            generator->flags |= 4;
        } else {
            generator->flags &= 0xFFFB;
        }
        uint16_t sizeHint = GetSizeHint(self);
        self->generator->sizeHint = sizeHint;
        generator = self->generator;
        if (GetStrictlyObserveSizeHint(self)) {
            generator->flags |= 0x20;
        } else {
            generator->flags &= 0xFFDF;
        }
        const re::Ptr32<const char>* technique = GetShadowTechnique(self);
        ShadowGenerator_SetShadowTechnique(self->generator, technique, 1);
        generator = self->generator;
        if (GetRenderBackfaces(self)) {
            generator->flags |= 0x10;
        } else {
            generator->flags &= 0xFFEF;
        }
        if (GetUseDefaultDepthBias(self)) {
            ShadowGenerator_SetDepthBiasToDefault(self->generator);
        } else {
            float bias = GetDepthBias(self);
            self->generator->depthBias = bias;
        }
        ApplyInheritedValues(self);
        self->flags &= ~kStale;
    }

    // The game's stack slots, kept as they are shared: the object an entity hands back (it
    // stays from the call before when one isn't written: uninitialised the first time, ours
    // starts null), the casters' count (the same), and the error handler's argument slot,
    // reused as the casters' inner index and then the receivers' count.
    NiObject* object = nullptr;
    uint32_t casterCount = 0;
    uint32_t slot30 = uint32_t(reinterpret_cast<uintptr_t>(errors));
    bool changed = false;
    if ((self->flags & kCastersChanged) != 0) {
        PointerList_RemoveAll(&self->generator->unaffectedCasters);
        uint8_t allFound = 1;
        for (uint32_t i = 0; i < self->unaffectedCasters.size; i++) {
            void* e = self->unaffectedCasters.base[i];
            if (e != nullptr && V(e, slot::GetElementCount, &PROP_SCENE_ROOT_POINTER, &casterCount)) {
                for (slot30 = 0; slot30 < casterCount; slot30++) {
                    V(e, slot::GetObject, &PROP_SCENE_ROOT_POINTER, &object, slot30);
                    AttachUnaffected(self, object, true, allFound);
                }
            }
        }
        if (allFound) {
            self->flags &= ~kCastersChanged;
        }
        changed = true;
    }
    if ((self->flags & kReceiversChanged) != 0) {
        PointerList_RemoveAll(&self->generator->unaffectedReceivers);
        uint8_t allFound = 1;
        for (uint32_t i = 0; i < self->unaffectedReceivers.size; i++) {
            void* e = self->unaffectedReceivers.base[i];
            if (e != nullptr && V(e, slot::GetElementCount, &PROP_SCENE_ROOT_POINTER, &slot30)) {
                for (uint32_t j = 0; j < slot30; j++) {
                    V(e, slot::GetObject, &PROP_SCENE_ROOT_POINTER, &object, j);
                    AttachUnaffected(self, object, false, allFound);
                }
            }
        }
        if (allFound) {
            self->flags &= ~kReceiversChanged;
        }
    } else if (!changed) {
        return;
    }
    generator = self->generator;
    generator->flags |= 2;
    if ((generator->flags & 4) == 0) {
        ShadowGenerator_MarkShadowMapsDirty(generator);
    }
}

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NISHADOWCOMP_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace sc = ni::ec;

NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetTemplateID", 0x00849920, &sc::GetTemplateID_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::CanResetProperty", 0x008499D0, &sc::CanResetProperty);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::ResetProperty", 0x00849B70, &sc::ResetProperty);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::MakePropertyUnique", 0x00849C50, &sc::MakePropertyUnique);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::IsPropertyUnique", 0x00849D80, &sc::IsPropertyUnique);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::IsPropertySerializable", 0x00849E10, &sc::IsPropertySerializable);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::IsPropertyInheritable", 0x00849EE0, &sc::IsPropertyInheritable);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::IsExternalAssetPath", 0x00849F60, &sc::IsExternalAssetPath);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetElementCount", 0x00849FD0, &sc::GetElementCount);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::IsCollection", 0x0084A060, &sc::IsCollection_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetPropertyData(NiObject*)", 0x0084A0E0, &sc::GetObject_);
NISHADOWCOMP_REVERSED("NiTPrimitiveArray<void*>::SetSize", 0x0084A110, &sc::PointerArraySetSize);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetCastShadows", 0x0084A1F0, &sc::GetCastShadows_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetStaticShadows", 0x0084A220, &sc::GetStaticShadows_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetSizeHint", 0x0084A250, &sc::GetSizeHint_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetStrictlyObserveSizeHint", 0x0084A280,
                      &sc::GetStrictlyObserveSizeHint_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetShadowTechnique", 0x0084A2B0, &sc::GetShadowTechnique_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetRenderBackfaces", 0x0084A2E0, &sc::GetRenderBackfaces_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetDepthBias", 0x0084A310, &sc::GetDepthBias_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetUseDefaultDepthBias", 0x0084A340, &sc::GetUseDefaultDepthBias_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::ShouldUpdateGenerator", 0x0084A370, &sc::ShouldUpdateGenerator_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::ApplyInheritedValues", 0x0084A410, &sc::ApplyInheritedValues_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::_SDMInit", 0x0084A530, &sc::SDMInit);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::_SDMShutdown", 0x0084A6F0, &sc::SDMShutdown);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetClassName", 0x0084A850, &sc::GetClassName_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetName", 0x0084A880, &sc::GetName_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetDisplayName", 0x0084A8B0, &sc::GetDisplayName);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetPrimitiveType", 0x0084A940, &sc::GetType<false>);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetSemanticType", 0x0084AA20, &sc::GetType<true>);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetDescription", 0x0084AB00, &sc::GetDescription);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetCategory", 0x0084AC40, &sc::GetCategory);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::IsPropertyReadOnly", 0x0084ACC0, &sc::IsPropertyReadOnly);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetElementCount", 0x0084AD50, &sc::SetElementCount);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetPropertyData(float)", 0x0084AE80, &sc::GetFloat);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetPropertyData(bool)", 0x0084AEB0, &sc::GetBool);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetPropertyData(unsigned short)", 0x0084AF50, &sc::GetUShort);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetPropertyData(NiFixedString)", 0x0084AF80, &sc::GetString);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetPropertyData(NiEntityInterface*)", 0x0084AFC0, &sc::GetEntity);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetPropertyData(NiEntityInterface*)", 0x0084B030, &sc::SetEntity);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::NiShadowGeneratorComponent", 0x0084B0D0, &sc::ComponentConstruct);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetCastShadows", 0x0084B180, &sc::SetCastShadows_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetStaticShadows", 0x0084B1D0, &sc::SetStaticShadows_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetSizeHint", 0x0084B210, &sc::SetSizeHint_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetStrictlyObserveSizeHint", 0x0084B260,
                      &sc::SetStrictlyObserveSizeHint_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetShadowTechnique", 0x0084B2A0, &sc::SetShadowTechnique_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetRenderBackfaces", 0x0084B2E0, &sc::SetRenderBackfaces_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetDepthBias", 0x0084B320, &sc::SetDepthBias_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetUseDefaultDepthBias", 0x0084B380, &sc::SetUseDefaultDepthBias_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::CreateShadowGenerator", 0x0084B3C0, &sc::CreateShadowGenerator_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::Clone", 0x0084B440, &sc::Clone_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::Update", 0x0084B5A0, &sc::Update);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetPropertyData(float)", 0x0084BAB0, &sc::SetFloat);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetPropertyData(bool)", 0x0084BAE0, &sc::SetBool);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetPropertyData(unsigned short)", 0x0084BB80, &sc::SetUShort);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetPropertyData(NiFixedString)", 0x0084BBB0, &sc::SetString);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::SetMasterComponent", 0x0084BBE0, &sc::SetMasterComponent);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetDependentPropertyNames", 0x0084BCD0,
                      &sc::GetDependentPropertyNames);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetPropertyNames", 0x0084BD10, &sc::GetPropertyNames_);
NISHADOWCOMP_REVERSED("NiShadowGeneratorComponent::GetMasterComponent", 0x0084BF80, &sc::GetMasterComponent);
