// NiTexturingProperty and its maps (src/gamebryo/NiTexturingProperty.h): construction, the
// multi-texture flag, the shader maps, streaming (with the old formats' remapping of the normal
// and parallax maps to the decal slots), comparison, cloning, viewer strings.

#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiTexturingProperty.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTexturingProperty_RTTI, 0x020E28BC);

constexpr uintptr_t NiTexturingProperty_vtable = 0x00952098;
constexpr uintptr_t MapArray_vtable = 0x00961988;   // NiTPrimitiveArray<Map*>
constexpr uintptr_t Map_vtable = 0x009520E8;
constexpr uintptr_t BumpMap_vtable = 0x009520F8;
constexpr uintptr_t ParallaxMap_vtable = 0x00952108;
constexpr uintptr_t ShaderMap_vtable = 0x008FFA78;

GAME_VAR(float, NiPoint2_ZERO_x, 0x020E3940);
GAME_VAR(float, NiPoint2_ZERO_y, 0x020E3944);
GAME_VAR(uint8_t, NiTexture_ms_kDefaultPrefs, 0x00BC7624);   // NiTexture::FormatPrefs

using Map = NiTexturingMap;
using BumpMap = NiTexturingBumpMap;
using Map14 = NiTexturingMap14;
using MapArray = NiTArray<Map*>;
using Property = NiTexturingProperty;

GAME_FN(void, __thiscall, MapArray_SetSize, 0x0084A110, MapArray* self, uint32_t size);
GAME_FN(void, __thiscall, MapArray_SetAt, 0x00833C70, MapArray* self, uint32_t index, const re::Ptr32<ni::Map>* value);
GAME_FN(MapArray*, __thiscall, MapArray_Construct, 0x0082F290, void* self, uint32_t maxSize, uint32_t growBy);
GAME_FN(NiObject*, __cdecl, SourceTexture_Create, 0x007699A0, const void* pixelData, const void* prefs);   // NiSourceTexture::Create(NiPixelData*, prefs)
GAME_FN(void*, __thiscall, TextureTransform_Construct, 0x0077ED00, void* self, const float* translate, float rotate,
        const float* scale, const float* center, uint32_t method);
GAME_FN(void, __thiscall, TextureTransform_LoadBinary, 0x0077ED50, void* self, void* stream);
GAME_FN(void, __thiscall, TextureTransform_SaveBinary, 0x0077EDD0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TextureTransform_NotEqual, 0x0077EAC0, const void* self, const void* other);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// NiProperty's thunks to NiObjectNET's
GAME_FN(void, __thiscall, Property_LoadBinary, 0x007548C0, void* self, void* stream);
GAME_FN(void, __thiscall, Property_LinkObject, 0x007548D0, void* self, void* stream);
GAME_FN(void, __thiscall, Property_CopyMembers, 0x007654C0, void* self, void* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, Property_RegisterStreamables, 0x007654D0, void* self, void* stream);
GAME_FN(void, __thiscall, Property_SaveBinary, 0x007654E0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Property_IsEqual, 0x007654F0, void* self, void* other);
GAME_FN(void, __thiscall, Property_GetViewerStrings, 0x00765530, void* self, NiTArray<char*>* strings);
// (this chunk's, called where modes apply)
GAME_FN(void, __thiscall, Property_Destructor, 0x00765A50, Property* self);
GAME_FN(Map*, __thiscall, Map_CopyConstruct, 0x00766E60, void* self, const Map* other);
GAME_FN(void, __thiscall, Map_LoadBinary, 0x00766F30, Map* self, void* stream);
GAME_FN(void, __thiscall, Map_SaveBinary, 0x00767100, Map* self, void* stream);
GAME_FN(uint8_t, __thiscall, Map_Equals, 0x00765C70, const Map* self, const Map* other);
GAME_FN(uint8_t, __thiscall, BumpMap_Equals, 0x00765CE0, const BumpMap* self, const BumpMap* other);
GAME_FN(uint8_t, __thiscall, ShaderMap_Equals, 0x00765D60, const Map14* self, const Map14* other);
GAME_FN(uint8_t, __thiscall, ParallaxMap_Equals, 0x00765D90, const Map14* self, const Map14* other);
GAME_FN(char*, __cdecl, ClampModeViewerString, 0x00767470, const char* prefix, uint32_t mode);
GAME_FN(char*, __cdecl, FilterModeViewerString, 0x00767530, const char* prefix, uint32_t mode);

// the multi-texture flag: set when any map past the base is there
inline void UpdateMultiTexture(Property* self, uint16_t& flags) {
    for (uint32_t i = 1; i < self->maps.size; i++) {
        if (self->maps.base[i] != nullptr) {
            flags |= 1;
            return;
        }
    }
    flags &= 0xFFFE;
}

// a map's defaults (the constructor, inlined)
inline void InitMap(Map* map, uintptr_t vtable) {
    map->flags = 0;
    map->texture = nullptr;
    map->transform = nullptr;
    map->flags = uint16_t((map->flags & 0xF100) | 0x3100);
    map->vtable = reinterpret_cast<void*>(vtable);
}

// NiTArray::SetAt with the esize accounting, inlined
inline void SetAtInline(MapArray* array, uint32_t index, Map* map) {
    if (index < array->size) {
        if (map == nullptr) {
            if (array->base[index] != nullptr) {
                array->esize--;
            }
        } else if (array->base[index] == nullptr) {
            array->esize++;
        }
    } else {
        array->size = uint16_t(index + 1);
        if (map != nullptr) {
            array->esize++;
        }
    }
    array->base[index] = map;
}

// a new empty NiTPrimitiveArray (inlined in places)
inline MapArray* NewMapArray() {
    MapArray* array = static_cast<MapArray*>(NiNew(0x10));
    if (array == nullptr) {
        return nullptr;
    }
    array->maxSize = 0;
    array->growBy = 1;
    array->size = 0;
    array->esize = 0;
    array->base = nullptr;
    array->vtable = reinterpret_cast<void*>(MapArray_vtable);
    return array;
}

// ---- the property

// the common part of the constructors: 9 empty slots (the base map first when given)
void ConstructCommon(Property* self, Map* base) {
    ObjectNET_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTexturingProperty_vtable);
    self->flags = 0;
    self->maps.vtable = reinterpret_cast<void*>(MapArray_vtable);
    self->maps.maxSize = 9;
    self->maps.growBy = 1;
    self->maps.size = 0;
    self->maps.esize = 0;
    self->maps.base = static_cast<re::Ptr32<Map>*>(NiMalloc(0x24));
    self->maps.vtable = reinterpret_cast<void*>(MapArray_vtable);
    self->shaderMaps = nullptr;
    re::Ptr32<Map> none = base;
    MapArray_SetAt(&self->maps, 0, &none);
    for (uint32_t i = 1; i <= 8; i++) {
        none = nullptr;
        MapArray_SetAt(&self->maps, i, &none);
    }
    self->flags &= 0xF00F;
    uint16_t flags = self->flags;
    UpdateMultiTexture(self, flags);
    self->flags = flags;
    self->flags = uint16_t((flags & 0xFFF5) | 4);   // apply mode: modulate
}

Property* __fastcall Construct(Property* self, re::Edx) {
    ConstructCommon(self, nullptr);
    return self;
}

// NiTexturingProperty(pcTextureName): the base map from a source texture of that file
Property* __fastcall ConstructWithTexture(Property* self, re::Edx, void* pixelData) {   // NiTexturingProperty(NiPixelData*)
    ObjectNET_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTexturingProperty_vtable);
    self->flags = 0;
    self->maps.vtable = reinterpret_cast<void*>(MapArray_vtable);
    self->maps.maxSize = 9;
    self->maps.growBy = 1;
    self->maps.size = 0;
    self->maps.esize = 0;
    self->maps.base = static_cast<re::Ptr32<Map>*>(NiMalloc(0x24));
    self->maps.vtable = reinterpret_cast<void*>(MapArray_vtable);
    self->shaderMaps = nullptr;
    NiObject* texture = SourceTexture_Create(pixelData, &NiTexture_ms_kDefaultPrefs);
    re::Ptr32<Map> map = static_cast<Map*>(NiNew(0x10));
    if (map != nullptr) {
        map->vtable = reinterpret_cast<void*>(Map_vtable);
        map->flags = 0;
        map->texture = texture;
        AddRef(texture);
        map->transform = nullptr;
        map->flags = uint16_t((map->flags & 0xF100) | 0x3100);
    }
    MapArray_SetAt(&self->maps, 0, &map);
    for (uint32_t i = 1; i <= 8; i++) {
        re::Ptr32<Map> none = nullptr;
        MapArray_SetAt(&self->maps, i, &none);
    }
    self->flags &= 0xF00F;
    uint16_t flags = self->flags;
    UpdateMultiTexture(self, flags);
    self->flags = flags;
    self->flags = uint16_t((flags & 0xFFF5) | 4);
    return self;
}

void __fastcall Destruct(Property* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiTexturingProperty_vtable);
    for (uint32_t i = 0; i < self->maps.size; i++) {
        Map* map = self->maps.base[i];
        if (map != nullptr) {
            Virtual<void>(map, 0, 1);
        }
    }
    if (self->shaderMaps != nullptr) {
        for (uint32_t i = 0; i < self->shaderMaps->size; i++) {
            Map* map = self->shaderMaps->base[i];
            if (map != nullptr) {
                Virtual<void>(map, 0, 1);
            }
        }
        if (self->shaderMaps != nullptr) {
            Virtual<void>(self->shaderMaps, 0, 1);
        }
    }
    self->maps.vtable = reinterpret_cast<void*>(MapArray_vtable);
    NiFree(self->maps.base);
    ObjectNET_Destruct(self);
}

Property* __fastcall DeletingDestructor(Property* self, re::Edx, uint32_t flags) {
    Property_Destructor(self);
    if (flags & 1) {
        NiDelete(self, 0x34);
    }
    return self;
}

const RTTI* __fastcall GetRTTI(Property*, re::Edx) {
    return &NiTexturingProperty_RTTI;
}

// Type (NiProperty's slot 18): TEXTURING
int __fastcall Type(Property*, re::Edx) {
    return 8;
}

// the multi-texture flag after a map changed (set for a map, else from the others)
void __fastcall UpdateMultiTextureFlag(Property* self, re::Edx, Map* map) {
    if (map != nullptr) {
        self->flags |= 1;
        return;
    }
    for (uint32_t i = 1; i < self->maps.size; i++) {
        if (self->maps.base[i] != nullptr) {
            self->flags |= 1;
            return;
        }
    }
    self->flags &= 0xFFFE;
}

// SetShaderMap(uiIndex, pkShaderMap): the one there deleted
void __fastcall SetShaderMap(Property* self, re::Edx, uint32_t index, re::Ptr32<Map> map) {
    if (self->shaderMaps == nullptr) {
        self->shaderMaps = NewMapArray();
    }
    MapArray* array = self->shaderMaps;
    if (index < array->size) {
        Map* old = array->base[index];
        MapArray_SetAt(array, index, &map);
        if (old != nullptr) {
            Virtual<void>(old, 0, 1);
        }
        return;
    }
    if (map != nullptr) {
        if (index >= array->maxSize) {
            MapArray_SetSize(array, array->growBy + index);
        }
        MapArray_SetAt(array, index, &map);
    }
}

// ---- the maps

Map* __fastcall MapConstruct(Map* self, re::Edx) {
    self->flags = 0;
    self->vtable = reinterpret_cast<void*>(Map_vtable);
    self->texture = nullptr;
    self->transform = nullptr;
    self->flags = uint16_t((self->flags & 0xF100) | 0x3100);
    return self;
}

// Map(pkTexture, uiIndex, eClampMode, eFilterMode, pkTextureTransform)
Map* __fastcall MapConstructWith(Map* self, re::Edx, NiObject* texture, uint32_t index, uint32_t clamp,
                                 uint32_t filter, void* transform) {
    self->vtable = reinterpret_cast<void*>(Map_vtable);
    self->flags = 0;
    self->texture = texture;
    AddRef(texture);
    uint16_t modes = uint16_t(uint8_t((uint8_t(clamp) << 4) | uint8_t(filter)) << 8);
    self->transform = transform;
    uint16_t flags = uint16_t((modes | (self->flags & 0xC0FF)) & 0xFF00);
    self->flags = uint16_t(flags | uint16_t(index));
    return self;
}

// ~Map
void __fastcall MapDestruct(Map* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(Map_vtable);
    NiDelete(self->transform, 0x48);
    Release(self->texture);
}

// the deleting destructors (Map, BumpMap, ParallaxMap: their sizes)
template <uint32_t Size>
Map* __fastcall MapDeletingDestructor(Map* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(Map_vtable);
    NiDelete(self->transform, 0x48);
    Release(self->texture);
    if (flags & 1) {
        NiDelete(self, Size);
    }
    return self;
}

// Map(const Map&): the texture shared, the transform copied
Map* __fastcall MapCopyConstruct(Map* self, re::Edx, const Map* other) {
    self->vtable = reinterpret_cast<void*>(Map_vtable);
    self->flags = other->flags;
    self->texture = other->texture;
    AddRef(self->texture);
    if (other->transform == nullptr) {
        self->transform = nullptr;
        return self;
    }
    void* p = NiNew(0x48);
    if (p == nullptr) {
        self->transform = nullptr;
        return self;
    }
    const float* t = static_cast<const float*>(other->transform);
    float center[2] = {t[5], t[6]};
    float scale[2] = {t[3], t[4]};
    float translate[2] = {t[0], t[1]};
    self->transform = TextureTransform_Construct(p, translate, t[2], scale, center, At<uint32_t>(t, 0x44));
    return self;
}

uint8_t __fastcall MapEquals(const Map* self, re::Edx, const Map* other) {
    if (self->texture == nullptr) {
        if (other->texture != nullptr) {
            return 0;
        }
    } else if (!Virtual<uint8_t>(self->texture, 8, other->texture)) {
        return 0;
    }
    if (self->flags != other->flags) {
        return 0;
    }
    const void* a = self->transform;
    const void* b = other->transform;
    if (a != nullptr && b != nullptr) {
        return TextureTransform_NotEqual(a, b) ? 0 : 1;
    }
    if (a == nullptr && b != nullptr) {
        return 0;
    }
    if (a != nullptr && b == nullptr) {
        return 0;
    }
    return 1;
}

uint8_t __fastcall BumpMapEquals(const BumpMap* self, re::Edx, const BumpMap* other) {
    if (!Map_Equals(self, other)) {
        return 0;
    }
    return self->lumaScale == other->lumaScale && self->lumaOffset == other->lumaOffset &&
                   self->bumpMat[0] == other->bumpMat[0] && self->bumpMat[1] == other->bumpMat[1] &&
                   self->bumpMat[2] == other->bumpMat[2] && self->bumpMat[3] == other->bumpMat[3]
               ? 1
               : 0;
}

uint8_t __fastcall ShaderMapEquals(const Map14* self, re::Edx, const Map14* other) {
    if (!Map_Equals(self, other)) {
        return 0;
    }
    return self->value == other->value ? 1 : 0;
}

uint8_t __fastcall ParallaxMapEquals(const Map14* self, re::Edx, const Map14* other) {
    if (!Map_Equals(self, other)) {
        return 0;
    }
    float a, b;
    std::memcpy(&a, &self->value, 4);
    std::memcpy(&b, &other->value, 4);
    return a == b ? 1 : 0;
}

// ParallaxMap::GetClassType (slot 3)
int __fastcall ParallaxGetClassType(Map*, re::Edx) {
    return 2;
}

// Map::LoadBinary: the texture's link, the flags (older: clamp, filter, set as 32-bit values),
// the transform when there's one
void __fastcall MapLoadBinary(Map* self, re::Edx, void* stream) {
    Stream_ReadLinkID(stream);
    if (StreamVersion(stream) < 0x14010002) {
        uint32_t clamp;
        LoadBytes(stream, &clamp, 4);
        self->flags = uint16_t((self->flags & 0xCFFF) | uint16_t(clamp << 12));
        uint32_t filter;
        LoadBytes(stream, &filter, 4);
        self->flags = uint16_t((self->flags & 0xF0FF) | uint16_t(uint8_t(filter) << 8));
        uint32_t set;
        LoadBytes(stream, &set, 4);
        self->flags = uint16_t((self->flags & 0xFF00) | uint16_t(set));
    } else {
        LoadBytes(stream, &self->flags, 2);
    }
    if (StreamVersion(stream) < 0x0A030004) {
        uint16_t ps2;
        LoadBytes(stream, &ps2, 2);
        LoadBytes(stream, &ps2, 2);
    }
    self->transform = nullptr;
    uint8_t hasTransform = 0;
    LoadBytes(stream, &hasTransform, 1);
    if (hasTransform == 1) {
        float* t = static_cast<float*>(NiNew(0x48));
        if (t != nullptr) {
            t[0] = NiPoint2_ZERO_x;
            t[2] = 0.0f;
            t[1] = NiPoint2_ZERO_y;
            t[3] = 1.0f;
            t[4] = 1.0f;
            t[5] = 0.5f;
            t[6] = 0.5f;
            At<uint8_t>(t, 0x1C) = 1;
            At<uint32_t>(t, 0x44) = 2;
        }
        self->transform = t;
        TextureTransform_LoadBinary(t, stream);
    }
}

void __fastcall MapSaveBinary(Map* self, re::Edx, void* stream) {
    Virtual<void>(stream, 12, self->texture);   // SaveLinkID
    SaveBytes(stream, &self->flags, 2);
    uint8_t hasTransform = self->transform != nullptr ? 1 : 0;
    SaveBytes(stream, &hasTransform, 1);
    if (self->transform != nullptr) {
        TextureTransform_SaveBinary(self->transform, stream);
    }
}

void __fastcall BumpMapLoadBinary(BumpMap* self, re::Edx, void* stream) {
    Map_LoadBinary(self, stream);
    LoadBytes(stream, &self->lumaScale, 4);
    LoadBytes(stream, &self->lumaOffset, 4);
    LoadBytes(stream, &self->bumpMat[0], 4);
    LoadBytes(stream, &self->bumpMat[1], 4);
    LoadBytes(stream, &self->bumpMat[2], 4);
    LoadBytes(stream, &self->bumpMat[3], 4);
}

void __fastcall BumpMapSaveBinary(BumpMap* self, re::Edx, void* stream) {
    Map_SaveBinary(self, stream);
    SaveBytes(stream, &self->lumaScale, 4);
    SaveBytes(stream, &self->lumaOffset, 4);
    SaveBytes(stream, &self->bumpMat[0], 4);
    SaveBytes(stream, &self->bumpMat[1], 4);
    SaveBytes(stream, &self->bumpMat[2], 4);
    SaveBytes(stream, &self->bumpMat[3], 4);
}

// the parallax map's (and the shader map's: the same code) one value
void __fastcall Map14LoadBinary(Map14* self, re::Edx, void* stream) {
    Map_LoadBinary(self, stream);
    LoadBytes(stream, &self->value, 4);
}

void __fastcall Map14SaveBinary(Map14* self, re::Edx, void* stream) {
    Map_SaveBinary(self, stream);
    SaveBytes(stream, &self->value, 4);
}

// ---- streaming

// vf4
void __fastcall LoadBinary(Property* self, re::Edx, void* stream) {
    Property_LoadBinary(self, stream);
    uint16_t* flags = &self->flags;
    if (StreamVersion(stream) < 0x14010002) {
        uint32_t applyMode;
        LoadBytes(stream, &applyMode, 4);
        if (applyMode == 3 || applyMode == 4) {
            applyMode = 2;
        }
        *flags = uint16_t(uint16_t(applyMode * 2) | (*flags & 0xFFF1));
    } else {
        LoadBytes(stream, flags, 2);
        *flags &= 0xF00F;
    }
    uint32_t count;
    LoadBytes(stream, &count, 4);
    uint32_t size = count;
    if (StreamVersion(stream) < 0x14020005 && count > 6) {
        size = count + 2;
    }
    MapArray_SetSize(&self->maps, size);
    Stream_AddLinkIDBlock(stream, count);
    for (uint32_t i = 0; i < count; i++) {
        re::Ptr32<Map> map = nullptr;
        uint8_t hasMap;
        LoadBytes(stream, &hasMap, 1);
        uint32_t index;
        if (i == 5) {
            Stream_AddLinkIDBlock(stream, hasMap);
            if (hasMap != 0) {
                BumpMap* bump = static_cast<BumpMap*>(NiNew(0x28));
                if (bump != nullptr) {
                    bump->flags = 0;
                    bump->texture = nullptr;
                    bump->lumaScale = 1.0f;
                    bump->lumaOffset = 0.0f;
                    bump->bumpMat[0] = 0.5f;
                    bump->transform = nullptr;
                    bump->bumpMat[3] = 0.5f;
                    bump->flags = uint16_t((bump->flags & 0xF100) | 0x3100);
                    bump->vtable = reinterpret_cast<void*>(BumpMap_vtable);
                    bump->bumpMat[1] = 0.0f;
                    bump->bumpMat[2] = 0.0f;
                }
                map = bump;
                Virtual<void>(map, 1, stream);
            }
            index = 5;
        } else if (i == 7 && StreamVersion(stream) >= 0x14020005) {
            Stream_AddLinkIDBlock(stream, hasMap);
            if (hasMap != 0) {
                Map14* parallax = static_cast<Map14*>(NiNew(0x14));
                if (parallax != nullptr) {
                    parallax->flags = 0;
                    parallax->texture = nullptr;
                    float half = 0.5f;
                    std::memcpy(&parallax->value, &half, 4);
                    parallax->transform = nullptr;
                    parallax->flags = uint16_t((parallax->flags & 0xF100) | 0x3100);
                    parallax->vtable = reinterpret_cast<void*>(ParallaxMap_vtable);
                }
                map = parallax;
                Virtual<void>(map, 1, stream);
            }
            index = 7;
        } else {
            Stream_AddLinkIDBlock(stream, hasMap);
            if (hasMap != 0) {
                Map* plain = static_cast<Map*>(NiNew(0x10));
                if (plain != nullptr) {
                    InitMap(plain, Map_vtable);
                }
                map = plain;
                Virtual<void>(map, 1, stream);
            }
            index = i;
            if (index >= self->maps.maxSize) {
                MapArray_SetSize(&self->maps, self->maps.growBy + index);
            }
        }
        MapArray_SetAt(&self->maps, index, &map);
    }
    uint32_t shaderCount;
    LoadBytes(stream, &shaderCount, 4);
    Stream_AddLinkIDBlock(stream, shaderCount);
    if (shaderCount != 0) {
        void* p = NiNew(0x10);
        self->shaderMaps = p != nullptr ? MapArray_Construct(p, shaderCount, 1) : nullptr;
        for (uint32_t i = 0; i < shaderCount; i++) {
            uint8_t hasMap;
            LoadBytes(stream, &hasMap, 1);
            Stream_AddLinkIDBlock(stream, hasMap);
            Map* map = nullptr;
            if (hasMap != 0) {
                Map14* shader = static_cast<Map14*>(NiNew(0x14));
                if (shader != nullptr) {
                    InitMap(shader, ShaderMap_vtable);
                    shader->value = 0;
                }
                map = shader;
                Virtual<void>(map, 1, stream);
            }
            SetAtInline(self->shaderMaps, i, map);
        }
    }
    UpdateMultiTexture(self, *flags);
}

// vf5: the textures; for the older files the normal and parallax maps moved past the decals
void __fastcall LinkObject(Property* self, re::Edx, void* stream) {
    Property_LinkObject(self, stream);
    uint32_t count = Stream_GetNumberOfLinkIDs(stream);
    self->flags &= 0xF00F;
    for (uint32_t i = 0; i < count; i++) {
        if (Stream_GetNumberOfLinkIDs(stream) == 0) {
            continue;
        }
        re::Ptr32<Map>* slot = &self->maps.base[i];
        NiObject* texture = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        Map* map = *slot;
        if (map->texture != texture) {
            Release(map->texture);
            map->texture = texture;
            AddRef(texture);
        }
        if (i >= 8) {
            uint32_t flags = self->flags;
            self->flags = uint16_t(((((flags >> 4) & 0xFF) + 1) << 4) | (flags & 0xF00F));
        }
    }
    if (StreamVersion(stream) < 0x14020005) {
        uint16_t size = self->maps.size;
        re::Ptr32<Map> normal = size > 6 ? self->maps.base[6] : nullptr;
        re::Ptr32<Map> parallax = size > 7 ? self->maps.base[7] : nullptr;
        if (normal != nullptr || parallax != nullptr) {
            uint32_t extra = normal != nullptr ? 1 : 0;
            if (parallax != nullptr) {
                extra++;
            }
            if ((self->flags & 0xFF0) != 0 && extra != 0) {
                uint32_t from = uint32_t(size) - 1;
                if (from >= 8) {
                    uint32_t to = from + extra;
                    do {
                        re::Ptr32<Map>* source = &self->maps.base[from];   // (taken before a reallocation)
                        if (to >= self->maps.maxSize) {
                            MapArray_SetSize(&self->maps, self->maps.growBy + to);
                        }
                        MapArray_SetAt(&self->maps, to, source);
                        from--;
                        to--;
                    } while (from >= 8);
                }
            }
            re::Ptr32<Map> none;
            if (normal != nullptr) {
                if (self->maps.maxSize <= 8) {
                    MapArray_SetSize(&self->maps, self->maps.growBy + 8);
                }
                MapArray_SetAt(&self->maps, 8, &normal);
                none = nullptr;
                MapArray_SetAt(&self->maps, 6, &none);
                uint32_t flags = self->flags;
                self->flags = uint16_t(((((flags >> 4) & 0xFF) + 1) << 4) | (flags & 0xF00F));
            } else {
                none = nullptr;
                MapArray_SetAt(&self->maps, 8, &none);
            }
            if (parallax != nullptr) {
                if (self->maps.maxSize <= 9) {
                    MapArray_SetSize(&self->maps, self->maps.growBy + 9);
                }
                MapArray_SetAt(&self->maps, 9, &parallax);
                none = nullptr;
                MapArray_SetAt(&self->maps, 7, &none);
                uint32_t flags = self->flags;
                self->flags = uint16_t(((((flags >> 4) & 0xFF) + 1) << 4) | (flags & 0xF00F));
            } else if (extra == 2) {
                none = nullptr;
                MapArray_SetAt(&self->maps, 9, &none);
            }
        }
    }
    uint32_t shaderCount = Stream_GetNumberOfLinkIDs(stream);
    for (uint32_t i = 0; i < shaderCount; i++) {
        if (Stream_GetNumberOfLinkIDs(stream) == 0) {
            continue;
        }
        re::Ptr32<Map>* slot = &self->shaderMaps->base[i];
        NiObject* texture = static_cast<NiObject*>(Stream_ResolveLinkID(stream));
        Map* map = *slot;
        if (map->texture != texture) {
            Release(map->texture);
            map->texture = texture;
            AddRef(texture);
        }
    }
}

// vf6: the maps' textures
uint8_t __fastcall RegisterStreamables(Property* self, re::Edx, void* stream) {
    uint8_t result = Property_RegisterStreamables(self, stream);
    if (!result) {
        return result;
    }
    for (uint32_t i = 0; i < self->maps.size; i++) {
        Map* map = self->maps.base[i];
        if (map != nullptr && map->texture != nullptr) {
            Virtual<uint8_t>(map->texture, 6, stream);
        }
    }
    MapArray* shaders = self->shaderMaps;
    if (shaders != nullptr) {
        for (uint32_t i = 0; i < shaders->size; i++) {
            Map* map = shaders->base[i];
            if (map != nullptr && map->texture != nullptr) {
                Virtual<uint8_t>(map->texture, 6, stream);
            }
            shaders = self->shaderMaps;
        }
    }
    return 1;
}

// vf7
void __fastcall SaveBinary(Property* self, re::Edx, void* stream) {
    Property_SaveBinary(self, stream);
    SaveBytes(stream, &self->flags, 2);
    uint32_t count = self->maps.size;
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        Map* map = self->maps.base[i];
        uint8_t has = map != nullptr ? 1 : 0;
        SaveBytes(stream, &has, 1);
        if (has) {
            Virtual<void>(map, 2, stream);
        }
    }
    if (self->shaderMaps == nullptr) {
        uint32_t none = 0;
        SaveBytes(stream, &none, 4);
        return;
    }
    uint32_t shaderCount = self->shaderMaps->size;
    SaveBytes(stream, &shaderCount, 4);
    for (uint32_t i = 0; i < shaderCount; i++) {
        Map* map = self->shaderMaps->base[i];
        uint8_t has = map != nullptr ? 1 : 0;
        SaveBytes(stream, &has, 1);
        if (has) {
            Virtual<void>(map, 2, stream);
        }
    }
}

// vf8
uint8_t __fastcall IsEqual(Property* self, re::Edx, Property* other) {
    if (!Property_IsEqual(self, other) || self->flags != other->flags) {
        return 0;
    }
    uint32_t count = self->maps.size;
    if (count != other->maps.size) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        Map* a = self->maps.base[i];
        Map* b = other->maps.base[i];
        if (a == nullptr) {
            if (b != nullptr) {
                return 0;
            }
            continue;
        }
        if (b == nullptr) {
            return 0;
        }
        uint8_t same;
        if (i == 5) {
            same = BumpMap_Equals(static_cast<BumpMap*>(a), static_cast<BumpMap*>(b));
        } else if (i == 7) {
            same = ParallaxMap_Equals(static_cast<Map14*>(a), static_cast<Map14*>(b));
        } else {
            same = Map_Equals(a, b);
        }
        if (!same) {
            return 0;
        }
    }
    if (self->shaderMaps == nullptr) {
        return other->shaderMaps == nullptr ? 1 : 0;
    }
    if (other->shaderMaps == nullptr) {
        return 0;
    }
    uint32_t shaderCount = self->shaderMaps->size;
    if (shaderCount != other->shaderMaps->size) {
        return 0;
    }
    for (uint32_t i = 0; i < shaderCount; i++) {
        Map* a = self->shaderMaps->base[i];
        Map* b = other->shaderMaps->base[i];
        if (a == nullptr) {
            if (b != nullptr) {
                return 0;
            }
            continue;
        }
        if (b == nullptr || !ShaderMap_Equals(static_cast<Map14*>(a), static_cast<Map14*>(b))) {
            return 0;
        }
    }
    return 1;
}

// CopyMembers: the maps copied (sharing their textures)
void __fastcall CopyMembers(Property* self, re::Edx, Property* dest, void* cloning) {
    Property_CopyMembers(self, dest, cloning);
    dest->flags = self->flags;
    uint32_t count = self->maps.size;
    MapArray_SetSize(&dest->maps, count);
    for (uint32_t i = 0; i < count; i++) {
        if (self->maps.base[i] == nullptr) {
            continue;
        }
        re::Ptr32<Map> copy;
        uint32_t index;
        if (i == 5) {
            BumpMap* bump = static_cast<BumpMap*>(NiNew(0x28));
            if (bump != nullptr) {
                const BumpMap* from = static_cast<const BumpMap*>(self->maps.base[5]);
                Map_CopyConstruct(bump, from);
                bump->vtable = reinterpret_cast<void*>(BumpMap_vtable);
                bump->lumaScale = from->lumaScale;
                bump->lumaOffset = from->lumaOffset;
                bump->bumpMat[0] = from->bumpMat[0];
                bump->bumpMat[1] = from->bumpMat[1];
                bump->bumpMat[2] = from->bumpMat[2];
                bump->bumpMat[3] = from->bumpMat[3];
            }
            copy = bump;
            if (dest->maps.maxSize <= 5) {
                MapArray_SetSize(&dest->maps, dest->maps.growBy + 5);
            }
            index = 5;
        } else if (i == 7) {
            Map14* parallax = static_cast<Map14*>(NiNew(0x14));
            if (parallax != nullptr) {
                const Map14* from = static_cast<const Map14*>(self->maps.base[7]);
                Map_CopyConstruct(parallax, from);
                parallax->vtable = reinterpret_cast<void*>(ParallaxMap_vtable);
                parallax->value = from->value;
            }
            copy = parallax;
            if (dest->maps.maxSize <= 7) {
                MapArray_SetSize(&dest->maps, dest->maps.growBy + 7);
            }
            index = 7;
        } else {
            void* p = NiNew(0x10);
            copy = p != nullptr ? Map_CopyConstruct(p, self->maps.base[i]) : nullptr;
            if (i >= dest->maps.maxSize) {
                MapArray_SetSize(&dest->maps, dest->maps.growBy + i);
            }
            index = i;
        }
        MapArray_SetAt(&dest->maps, index, &copy);
    }
    if (self->shaderMaps == nullptr) {
        return;
    }
    if (dest->shaderMaps == nullptr) {
        dest->shaderMaps = NewMapArray();
    }
    uint32_t shaderCount = self->shaderMaps->size;
    MapArray_SetSize(dest->shaderMaps, shaderCount);
    for (uint32_t i = 0; i < shaderCount; i++) {
        if (self->shaderMaps->base[i] == nullptr) {
            continue;
        }
        Map14* copy = static_cast<Map14*>(NiNew(0x14));
        if (copy != nullptr) {
            const Map14* from = static_cast<const Map14*>(self->shaderMaps->base[i]);
            Map_CopyConstruct(copy, from);
            copy->vtable = reinterpret_cast<void*>(ShaderMap_vtable);
            copy->value = from->value;
        }
        MapArray* array = dest->shaderMaps;
        if (i >= array->maxSize) {
            MapArray_SetSize(array, array->growBy + i);
        }
        SetAtInline(array, i, copy);
    }
}

// ---- viewer strings

char* __cdecl ClampModeString(const char* prefix, uint32_t mode) {
    size_t size = std::strlen(prefix) + 0x14;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    switch (mode) {
    case 0:
        NiSprintf(text, size, "%s = CLAMP_S_CLAMP_T", prefix);
        break;
    case 1:
        NiSprintf(text, size, "%s = CLAMP_S_WRAP_T", prefix);
        break;
    case 2:
        NiSprintf(text, size, "%s = WRAP_S_CLAMP_T", prefix);
        break;
    case 3:
        NiSprintf(text, size, "%s = WRAP_S_WRAP_T", prefix);
        break;
    default:
        NiSprintf(text, size, "%s = UNKNOWN!!!", prefix);
        break;
    }
    return text;
}

char* __cdecl FilterModeString(const char* prefix, uint32_t mode) {
    size_t size = std::strlen(prefix) + 0x1E;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    switch (mode) {
    case 0:
        NiSprintf(text, size, "%s = FILTER_NEAREST", prefix);
        break;
    case 1:
        NiSprintf(text, size, "%s = FILTER_BILERP", prefix);
        break;
    case 2:
        NiSprintf(text, size, "%s = FILTER_TRILERP", prefix);
        break;
    case 3:
        NiSprintf(text, size, "%s = FILTER_NEAREST_MIPNEAREST", prefix);
        break;
    case 4:
        NiSprintf(text, size, "%s = FILTER_NEAREST_MIPLERP", prefix);
        break;
    case 5:
        NiSprintf(text, size, "%s = FILTER_BILERP_MIPNEAREST", prefix);
        break;
    default:
        NiSprintf(text, size, "%s = UNKNOWN!!!", prefix);
        break;
    }
    return text;
}

inline void AddByAdd(NiTArray<char*>* strings, re::Ptr32<char> value) {
    StringArray_Add(strings, &value);
}

// vf9
void __fastcall GetViewerStrings(Property* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTexturingProperty_RTTI.name));
    static const char* const names[] = {"MultiTexture Base Map",   "MultiTexture Dark Map",
                                        "MultiTexture Detail Map", "MultiTexture Gloss Map",
                                        "MultiTexture Glow Map",   "MultiTexture Bump Map",
                                        "MultiTexture Normal Map", "MultiTexture Parallax Map"};
    uint32_t count = self->maps.size;
    for (uint32_t i = 0; i < count; i++) {
        Map* map = self->maps.base[i];
        if (map == nullptr) {
            continue;
        }
        char label[64];
        if (i <= 7) {
            AddByAdd(strings, GetViewerStringUint(names[i], i));
            if (i == 5) {
                const BumpMap* bump = static_cast<const BumpMap*>(map);
                AddByAdd(strings, GetViewerStringFloat("m_fLumaScale", bump->lumaScale));
                AddByAdd(strings, GetViewerStringFloat("m_fLumaOffset", bump->lumaOffset));
                AddByAdd(strings, GetViewerStringFloat("m_fBumpMat00", bump->bumpMat[0]));
                AddByAdd(strings, GetViewerStringFloat("m_fBumpMat01", bump->bumpMat[1]));
                AddByAdd(strings, GetViewerStringFloat("m_fBumpMat10", bump->bumpMat[2]));
                AddByAdd(strings, GetViewerStringFloat("m_fBumpMat11", bump->bumpMat[3]));
            } else if (i == 7) {
                float offset;
                std::memcpy(&offset, &static_cast<const Map14*>(map)->value, 4);
                AddByAdd(strings, GetViewerStringFloat("m_fParallaxOffset", offset));
            }
        } else {
            NiSprintf(label, 0x40, "MultiTexture Decal Map %d", i - 8);
            AddViewerString(strings, GetViewerStringUint(label, i));
        }
        char field[64];
        NiSprintf(field, 0x40, "m_spTexture ");
        AddViewerString(strings, GetViewerStringPointer(field, map->texture));
        NiSprintf(field, 0x40, "Clamp Mode ");
        AddViewerString(strings, ClampModeViewerString(field, (map->flags >> 12) & 3));
        NiSprintf(field, 0x40, "Filter Mode");
        AddViewerString(strings, FilterModeViewerString(field, uint8_t(map->flags >> 8) & 0xF));
        NiSprintf(field, 0x40, "Texture Coord Index ");
        AddViewerString(strings, GetViewerStringUint(field, uint8_t(map->flags)));
    }
    if (self->shaderMaps == nullptr) {
        return;
    }
    uint32_t shaderCount = self->shaderMaps->size;
    for (uint32_t i = 0; i < shaderCount; i++) {
        Map* map = self->shaderMaps->base[i];
        if (map == nullptr) {
            continue;
        }
        AddViewerStringInline(strings, GetViewerStringUint("Shader Map", i));
        AddViewerStringInline(strings, GetViewerStringUint("m_uiID", static_cast<Map14*>(map)->value));
        char field[64];
        NiSprintf(field, 0x40, "m_spTexture ");
        AddViewerStringInline(strings, GetViewerStringPointer(field, map->texture));
        NiSprintf(field, 0x40, "Clamp Mode ");
        AddViewerStringInline(strings, ClampModeViewerString(field, (map->flags >> 12) & 3));
        NiSprintf(field, 0x40, "Filter Mode");
        AddViewerStringInline(strings, FilterModeViewerString(field, uint8_t(map->flags >> 8) & 0xF));
        NiSprintf(field, 0x40, "Texture Coord Index ");
        AddViewerStringInline(strings, GetViewerStringUint(field, uint8_t(map->flags)));
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NITEXP_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NITEXP_REVERSED("NiTexturingProperty::~NiTexturingProperty", 0x00765A50, &ni::Destruct);
NITEXP_REVERSED("NiTexturingProperty::GetRTTI", 0x00765AE0, &ni::GetRTTI);
NITEXP_REVERSED("NiTexturingProperty::Type", 0x00765AF0, &ni::Type);
NITEXP_REVERSED("NiTexturingProperty::UpdateMultiTextureFlag", 0x00765B00, &ni::UpdateMultiTextureFlag);
NITEXP_REVERSED("NiTexturingProperty::Map::Map", 0x00765BA0, &ni::MapConstruct);
NITEXP_REVERSED("NiTexturingProperty::Map::MapWith", 0x00765BD0, &ni::MapConstructWith);
NITEXP_REVERSED("NiTexturingProperty::Map::~Map", 0x00765C30, &ni::MapDestruct);
NITEXP_REVERSED("NiTexturingProperty::Map::operator==", 0x00765C70, &ni::MapEquals);
NITEXP_REVERSED("NiTexturingProperty::BumpMap::operator==", 0x00765CE0, &ni::BumpMapEquals);
NITEXP_REVERSED("NiTexturingProperty::ShaderMap::operator==", 0x00765D60, &ni::ShaderMapEquals);
NITEXP_REVERSED("NiTexturingProperty::ParallaxMap::operator==", 0x00765D90, &ni::ParallaxMapEquals);
NITEXP_REVERSED("NiTexturingProperty::ParallaxMap::GetClassType", 0x00765DC0, &ni::ParallaxGetClassType);
NITEXP_REVERSED("NiTexturingProperty::NiTexturingProperty", 0x00765DD0, &ni::Construct);
NITEXP_REVERSED("NiTexturingProperty::DeletingDestructor", 0x00765F10, &ni::DeletingDestructor);
NITEXP_REVERSED("NiTexturingProperty::NiTexturingPropertyWithTexture", 0x00765F30, &ni::ConstructWithTexture);
NITEXP_REVERSED("NiTexturingProperty::SetShaderMap", 0x007660D0, &ni::SetShaderMap);
NITEXP_REVERSED("NiTexturingProperty::LoadBinary", 0x007661A0, &ni::LoadBinary);
NITEXP_REVERSED("NiTexturingProperty::LinkObject", 0x00766600, &ni::LinkObject);
NITEXP_REVERSED("NiTexturingProperty::RegisterStreamables", 0x00766920, &ni::RegisterStreamables);
NITEXP_REVERSED("NiTexturingProperty::SaveBinary", 0x007669B0, &ni::SaveBinary);
NITEXP_REVERSED("NiTexturingProperty::IsEqual", 0x00766B70, &ni::IsEqual);
NITEXP_REVERSED("NiTexturingProperty::Map::DeletingDestructor", 0x00766E10, &ni::MapDeletingDestructor<0x10>);
NITEXP_REVERSED("NiTexturingProperty::Map::MapCopy", 0x00766E60, &ni::MapCopyConstruct);
NITEXP_REVERSED("NiTexturingProperty::Map::LoadBinary", 0x00766F30, &ni::MapLoadBinary);
NITEXP_REVERSED("NiTexturingProperty::Map::SaveBinary", 0x00767100, &ni::MapSaveBinary);
NITEXP_REVERSED("NiTexturingProperty::BumpMap::LoadBinary", 0x00767190, &ni::BumpMapLoadBinary);
NITEXP_REVERSED("NiTexturingProperty::BumpMap::SaveBinary", 0x00767270, &ni::BumpMapSaveBinary);
NITEXP_REVERSED("NiTexturingProperty::ParallaxMap::LoadBinary", 0x00767350, &ni::Map14LoadBinary);
NITEXP_REVERSED("NiTexturingProperty::ParallaxMap::SaveBinary", 0x00767390, &ni::Map14SaveBinary);
NITEXP_REVERSED("NiTexturingProperty::BumpMap::DeletingDestructor", 0x007673D0, &ni::MapDeletingDestructor<0x28>);
NITEXP_REVERSED("NiTexturingProperty::ParallaxMap::DeletingDestructor", 0x00767420,
                &ni::MapDeletingDestructor<0x14>);
NITEXP_REVERSED("NiTexturingProperty::ClampModeViewerString", 0x00767470, &ni::ClampModeString);
NITEXP_REVERSED("NiTexturingProperty::FilterModeViewerString", 0x00767530, &ni::FilterModeString);
NITEXP_REVERSED("NiTexturingProperty::CopyMembers", 0x00767620, &ni::CopyMembers);
NITEXP_REVERSED("NiTexturingProperty::GetViewerStrings", 0x007678E0, &ni::GetViewerStrings);
