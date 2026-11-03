// NiTextureEffect (vtable 0x952668, 0x190 bytes): an NiDynamicEffect that projects a texture
// (a projected light or shadow, an environment or fog map). Its model-space projection (matrix
// and translation) is turned into a world-space one by UpdateProjection whenever its world
// transform changes (slot 30 UpdateWorldData), by the coordinate mode: world parallel / world
// perspective (through the world transform's inverse), sphere map (fixed), specular / diffuse
// cube map (the rotation only); a clipping plane goes along. Its streaming, cloning, comparison
// and viewer strings, and the texture type names (ms_akTextureTypeNames: their _SDMInit and the
// lookup by name).
//
// Main-thread float maths: float operations in the game's order.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDynamicEffect.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiPlane {
    NiPoint3 normal;   // +0x0
    float constant;    // +0xC
};
static_assert(sizeof(NiPlane) == 0x10);

struct NiTextureEffect : NiDynamicEffect {
    NiMatrix3 modelProjMat;     // +0xF8
    NiPoint3 modelProjTrans;    // +0x11C
    NiMatrix3 worldProjMat;     // +0x128
    NiPoint3 worldProjTrans;    // +0x14C
    re::Ptr32<NiRefObject> texture;       // +0x158 (NiTexturePtr)
    uint32_t filter;            // +0x15C (0)
    uint32_t clamp;             // +0x160 (3)
    uint32_t textureMode;       // +0x164 (0 projected light, 1 projected shadow, 2 environment
                                //  map, 3 fog map)
    uint32_t coordMode;         // +0x168 (0 world parallel, 1 world perspective, 2 sphere map,
                                //  3 specular cube map, 4 diffuse cube map)
    uint8_t planeEnable;        // +0x16C
    uint8_t pad16D[3];
    NiPlane modelPlane;         // +0x170
    NiPlane worldPlane;         // +0x180
};
static_assert(offsetof(NiTextureEffect, modelProjMat) == 0xF8);
static_assert(offsetof(NiTextureEffect, worldProjMat) == 0x128);
static_assert(offsetof(NiTextureEffect, texture) == 0x158);
static_assert(offsetof(NiTextureEffect, planeEnable) == 0x16C);
static_assert(offsetof(NiTextureEffect, worldPlane) == 0x180);
static_assert(sizeof(NiTextureEffect) == 0x190);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTextureEffect_RTTI, 0x020E2A78);
GAME_ARRAY(const char*, NiTextureEffect_ms_akTextureTypeNames, 0x020E2AAC);   // [4]
GAME_VAR(NiMatrix3, NiMatrix3_IDENTITY, 0x00BC75FC);
GAME_VAR(NiPoint3, NiPoint3_ZERO_BC8A90, 0x00BC8A90);
GAME_VAR(double, kHalf, 0x008FF1F8);       // 0.5
GAME_VAR(double, kMinusHalf, 0x00912CD0);  // -0.5
GAME_VAR(float, kHalfF, 0x009055A0);       // 0.5f

constexpr uintptr_t NiTextureEffect_vtable = 0x00952668;

GAME_FN(void, __thiscall, DynamicEffect_Construct, 0x00772180, void* self);
GAME_FN(void, __thiscall, DynamicEffect_Destruct, 0x00771E80, void* self);
GAME_FN(void, __thiscall, DynamicEffect_CopyMembers, 0x00771BA0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, DynamicEffect_LoadBinary, 0x00772070, void* self, void* stream);
GAME_FN(void, __thiscall, DynamicEffect_LinkObject, 0x00771C40, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, DynamicEffect_RegisterStreamables, 0x007719D0, void* self, void* stream);
GAME_FN(void, __thiscall, DynamicEffect_SaveBinary, 0x007720D0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, DynamicEffect_IsEqual, 0x00771CC0, void* self, void* other);
GAME_FN(void, __thiscall, DynamicEffect_GetViewerStrings, 0x00772230, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, AVObject_UpdateWorldData, 0x00779EB0, void* self);
GAME_FN(void, __cdecl, NiRenderer_RemoveEffect, 0x00755660, void* effect);
GAME_FN(NiPlane*, __thiscall, Plane_Construct, 0x00771010, NiPlane* self, const NiPoint3* normal, float constant);
GAME_FN(char*, __thiscall, Plane_GetViewerString, 0x00771070, const NiPlane* self, const char* prefix);
GAME_FN(void, __thiscall, Plane_LoadBinary, 0x007D23D0, NiPlane* self, void* stream);    // (NiBound's: 4 floats)
GAME_FN(void, __thiscall, Plane_SaveBinary, 0x007D2410, NiPlane* self, void* stream);
GAME_FN(uint8_t, __thiscall, Plane_NotEqual, 0x00769390, const NiPlane* self, const NiPlane* other);   // (NiColorA's)
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Multiply, 0x00768920, const NiMatrix3* self, NiMatrix3* result,
        const NiMatrix3* other);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Scale, 0x00768A10, const NiMatrix3* self, NiMatrix3* result, float scale);
GAME_FN(NiPoint3*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const NiMatrix3* self, NiPoint3* result,
        const NiPoint3* point);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Transpose, 0x00768C40, const NiMatrix3* self, NiMatrix3* result);
GAME_FN(uint8_t, __thiscall, Matrix3_Equals, 0x007686A0, const NiMatrix3* self, const NiMatrix3* other);
GAME_FN(void, __thiscall, Matrix3_LoadBinary, 0x007690A0, NiMatrix3* self, void* stream);
GAME_FN(void, __thiscall, Matrix3_SaveBinary, 0x00769100, NiMatrix3* self, void* stream);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, NiPoint3* self, void* stream);
GAME_FN(char*, __cdecl, FilterModeViewerString, 0x00767530, const char* prefix, uint32_t filter);
GAME_FN(char*, __cdecl, ClampModeViewerString, 0x00767470, const char* prefix, uint32_t clamp);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);
// (this file's, called where modes apply)
GAME_FN(NiTextureEffect*, __thiscall, TextureEffect_Construct, 0x0076BA80, void* self);
GAME_FN(void, __thiscall, TextureEffect_UpdateProjection, 0x0076BBA0, NiTextureEffect* self);
GAME_FN(void, __thiscall, TextureEffect_CopyMembers, 0x0076BFE0, NiTextureEffect* self, NiTextureEffect* dest,
        void* cloning);
GAME_FN(char*, __cdecl, TextureModeViewerString, 0x0076C4A0, const char* prefix, uint32_t mode);
GAME_FN(char*, __cdecl, CoordModeViewerString, 0x0076C550, const char* prefix, uint32_t mode);

// NiPointer::operator= as the code inlines it: the old one released first
inline void AssignTexture(re::Ptr32<NiRefObject>& slot, NiRefObject* value) {
    NiRefObject* old = slot;
    if (old != value) {
        if (old != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(&old->refCount)) == 0) {
            Virtual<void>(old, 1);
        }
        slot = value;
        if (value != nullptr) {
            _InterlockedIncrement(reinterpret_cast<volatile long*>(&value->refCount));
        }
    }
}

// NiTArray<char*>::Add of a viewer string
inline void Add(NiTArray<char*>* strings, char* value) {
    AddViewerString(strings, value);
}

} // namespace

// 0x76BA40: NiPoint3::operator== (1 or 0, a 32-bit result)
uint32_t __fastcall NiPoint3_Equals(const NiPoint3* self, re::Edx, const NiPoint3* other) {
    if (other->x == self->x && other->y == self->y && other->z == self->z) {
        return 1;
    }
    return 0;
}

// 0x76BA80: the constructor: identity projections, wrap clamping, world parallel, the planes
// zero (type 8)
NiTextureEffect* __fastcall NiTextureEffect_Construct(NiTextureEffect* self, re::Edx) {
    DynamicEffect_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTextureEffect_vtable);
    self->modelProjMat = NiMatrix3_IDENTITY;
    self->modelProjTrans = {0.0f, 0.0f, 0.0f};
    self->worldProjMat = NiMatrix3_IDENTITY;
    self->worldProjTrans = {0.0f, 0.0f, 0.0f};
    self->texture = nullptr;
    Plane_Construct(&self->modelPlane, &NiPoint3_ZERO_BC8A90, 0.0f);
    Plane_Construct(&self->worldPlane, &NiPoint3_ZERO_BC8A90, 0.0f);
    self->filter = 0;
    self->textureMode = 0;
    self->coordMode = 0;
    self->planeEnable = 0;
    self->effectType = 8;
    self->clamp = 3;
    return self;
}

// 0x76BB40: GetRTTI
const RTTI* __fastcall NiTextureEffect_GetRTTI(NiTextureEffect*, re::Edx) {
    return &NiTextureEffect_RTTI;
}

// 0x76BB50: GetTextureTypeFromName(name, type&): the same handle, or the same text ignoring case
uint8_t __cdecl NiTextureEffect_GetTextureTypeFromName(const re::Ptr32<const char>* name, uint32_t* type) {
    for (uint32_t i = 0; i < 4; i++) {
        const char* typeName = NiTextureEffect_ms_akTextureTypeNames[i];
        const char* text = *name;
        if (typeName == text ||
            (text != nullptr && typeName != nullptr && crt::Stricmp(typeName, text) == 0)) {
            *type = i;
            return 1;
        }
    }
    return 0;
}

// 0x76BBA0: UpdateProjection: the world-space projection from the model one and the world
// transform (sphere maps: fixed, the world rotation's columns halved; cube maps: the rotation
// only; the others: inverse scale, the translation too), then the world clipping plane
void __fastcall NiTextureEffect_UpdateProjection(NiTextureEffect* self, re::Edx) {
    const NiMatrix3& rotate = self->world.rotate;
    if (self->coordMode == 2) {
        self->worldProjTrans.y = kHalfF;
        self->worldProjTrans.x = kHalfF;
        self->worldProjTrans.z = 0.0f;
        float m01 = rotate.m[0][1];
        self->worldProjMat.m[0][0] = float(double(rotate.m[0][2]) * kHalf);
        self->worldProjMat.m[1][0] = float(double(m01) * kMinusHalf);
        self->worldProjMat.m[2][0] = 0.0f;
        float m11 = rotate.m[1][1];
        self->worldProjMat.m[0][1] = float(double(rotate.m[1][2]) * kHalf);
        self->worldProjMat.m[1][1] = float(double(m11) * kMinusHalf);
        self->worldProjMat.m[2][1] = 0.0f;
        float m21 = rotate.m[2][1];
        self->worldProjMat.m[0][2] = float(double(rotate.m[2][2]) * kHalf);
        self->worldProjMat.m[1][2] = float(double(m21) * kMinusHalf);
        self->worldProjMat.m[2][2] = 0.0f;
    } else if (self->coordMode == 3 || self->coordMode == 4) {
        self->worldProjTrans.z = 0.0f;
        self->worldProjTrans.y = 0.0f;
        self->worldProjTrans.x = 0.0f;
        NiMatrix3 transposed;
        NiMatrix3 product;
        Matrix3_Transpose(&rotate, &transposed);
        self->worldProjMat = *Matrix3_Multiply(&self->modelProjMat, &product, &transposed);
    } else {
        float inverseScale = 1.0f / self->world.scale;
        NiMatrix3 transposed;
        NiMatrix3 product;
        NiMatrix3 scaled;
        Matrix3_Transpose(&rotate, &transposed);
        const NiMatrix3* projection = Matrix3_Multiply(&self->modelProjMat, &product, &transposed);
        self->worldProjMat = *Matrix3_Scale(projection, &scaled, inverseScale);
        NiPoint3 moved;
        const NiPoint3* p = Matrix3_TimesPoint(&self->worldProjMat, &moved, &self->world.translate);
        float x = self->modelProjTrans.x - p->x;
        float y = self->modelProjTrans.y - p->y;
        float z = self->modelProjTrans.z - p->z;
        self->worldProjTrans.x = x;
        self->worldProjTrans.y = y;
        self->worldProjTrans.z = z;
    }
    NiPoint3 normal;
    Matrix3_TimesPoint(&rotate, &normal, &self->modelPlane.normal);
    self->revisionID++;
    self->worldPlane.normal = normal;
    float dot = self->world.translate.y * normal.y + self->world.translate.x * normal.x;
    dot = dot + self->world.translate.z * normal.z;
    self->worldPlane.constant = dot + self->modelPlane.constant * self->world.scale;
}

// 0x76BDF0: UpdateWorldData (slot 30): NiAVObject's, then the projection
void __fastcall NiTextureEffect_UpdateWorldData(NiTextureEffect* self, re::Edx) {
    AVObject_UpdateWorldData(self);
    TextureEffect_UpdateProjection(self);
}

// 0x76BE20: LinkObject (slot 5): the texture
void __fastcall NiTextureEffect_LinkObject(NiTextureEffect* self, re::Edx, void* stream) {
    DynamicEffect_LinkObject(self, stream);
    NiRefObject* texture = static_cast<NiRefObject*>(Stream_ResolveLinkID(stream));
    AssignTexture(self->texture, texture);
}

// 0x76BE80: RegisterStreamables (slot 6): the texture too
uint8_t __fastcall NiTextureEffect_RegisterStreamables(NiTextureEffect* self, re::Edx, void* stream) {
    if (!DynamicEffect_RegisterStreamables(self, stream)) {
        return 0;
    }
    if (self->texture != nullptr) {
        Virtual<uint8_t>(self->texture, 6, stream);
    }
    return 1;
}

// 0x76BF00: the deleting destructor (the renderer's data purged, the texture let go)
NiTextureEffect* __fastcall NiTextureEffect_DeletingDestructor(NiTextureEffect* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiTextureEffect_vtable);
    NiRenderer_RemoveEffect(self);
    NiRefObject* texture = self->texture;
    if (texture != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(&texture->refCount)) == 0) {
        Virtual<void>(texture, 1);
    }
    DynamicEffect_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiTextureEffect));
    }
    return self;
}

// 0x76BF60: the texture type names (the _SDMInit's)
void __cdecl NiTextureEffect_SDMInit() {
    FixedString_AssignText(&NiTextureEffect_ms_akTextureTypeNames[0], "ProjectedLight");
    FixedString_AssignText(&NiTextureEffect_ms_akTextureTypeNames[1], "ProjectedShadow");
    FixedString_AssignText(&NiTextureEffect_ms_akTextureTypeNames[2], "EnvironmentMap");
    FixedString_AssignText(&NiTextureEffect_ms_akTextureTypeNames[3], "FogMap");
}

// 0x76BFA0: the texture type names emptied (the _SDMShutdown's, from NiMainSDM::Shutdown)
void __cdecl NiTextureEffect_SDMShutdown() {
    for (int i = 0; i < 4; i++) {
        FixedString_AssignText(&NiTextureEffect_ms_akTextureTypeNames[i], nullptr);
    }
}

// 0x76BFE0: CopyMembers (the world projection isn't copied)
void __fastcall NiTextureEffect_CopyMembers(NiTextureEffect* self, re::Edx, NiTextureEffect* dest, void* cloning) {
    DynamicEffect_CopyMembers(self, dest, cloning);
    AssignTexture(dest->texture, self->texture);
    dest->modelProjMat = self->modelProjMat;
    dest->modelProjTrans = self->modelProjTrans;
    dest->filter = self->filter;
    dest->clamp = self->clamp;
    dest->textureMode = self->textureMode;
    dest->coordMode = self->coordMode;
    dest->planeEnable = self->planeEnable;
    dest->modelPlane = self->modelPlane;
    dest->worldPlane = self->worldPlane;
}

// 0x76C110: LoadBinary (slot 4); before 10.3.0.4 two 16-bit values more (dropped)
void __fastcall NiTextureEffect_LoadBinary(NiTextureEffect* self, re::Edx, void* stream) {
    DynamicEffect_LoadBinary(self, stream);
    Matrix3_LoadBinary(&self->modelProjMat, stream);
    Point3_LoadBinary(&self->modelProjTrans, stream);
    uint32_t value;
    LoadBytes(stream, &value, 4);
    self->filter = value;
    LoadBytes(stream, &value, 4);
    self->clamp = value;
    LoadBytes(stream, &value, 4);
    self->textureMode = value;
    LoadBytes(stream, &value, 4);
    self->coordMode = value;
    Stream_ReadLinkID(stream);
    uint8_t enable;
    LoadBytes(stream, &enable, 1);
    self->planeEnable = enable != 0 ? 1 : 0;
    Plane_LoadBinary(&self->modelPlane, stream);
    if (StreamVersion(stream) < 0x0A030004) {
        uint16_t unused;
        LoadBytes(stream, &unused, 2);
        LoadBytes(stream, &unused, 2);
    }
}

// 0x76C290: SaveBinary (slot 7)
void __fastcall NiTextureEffect_SaveBinary(NiTextureEffect* self, re::Edx, void* stream) {
    DynamicEffect_SaveBinary(self, stream);
    Matrix3_SaveBinary(&self->modelProjMat, stream);
    Point3_SaveBinary(&self->modelProjTrans, stream);
    SaveBytes(stream, &self->filter, 4);
    SaveBytes(stream, &self->clamp, 4);
    SaveBytes(stream, &self->textureMode, 4);
    SaveBytes(stream, &self->coordMode, 4);
    Virtual<void>(stream, 12, self->texture);   // SaveLinkID
    uint8_t enable = self->planeEnable;
    SaveBytes(stream, &enable, 1);
    Plane_SaveBinary(&self->modelPlane, stream);
}

// 0x76C3A0: IsEqual (slot 8): the other's members compared against this one's
uint8_t __fastcall NiTextureEffect_IsEqual(NiTextureEffect* self, re::Edx, NiTextureEffect* other) {
    if (!DynamicEffect_IsEqual(self, other)) {
        return 0;
    }
    if (!Matrix3_Equals(&other->modelProjMat, &self->modelProjMat)) {
        return 0;
    }
    if (Point3_NotEqual(&other->modelProjTrans, &self->modelProjTrans)) {
        return 0;
    }
    if (other->filter != self->filter || other->clamp != self->clamp || other->textureMode != self->textureMode ||
        other->coordMode != self->coordMode || other->planeEnable != self->planeEnable) {
        return 0;
    }
    if (Plane_NotEqual(&other->modelPlane, &self->modelPlane)) {
        return 0;
    }
    return 1;
}

// 0x76C450: CreateClone (slot 3); CopyMembers is called on a failed NiNew too, as the game has it
NiTextureEffect* __fastcall NiTextureEffect_CreateClone(NiTextureEffect* self, re::Edx, void* cloning) {
    void* memory = NiNew(sizeof(NiTextureEffect));
    NiTextureEffect* clone = memory != nullptr ? TextureEffect_Construct(memory) : nullptr;
    TextureEffect_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x76C4A0: the texture mode's viewer string (another value: the buffer as NiMalloc left it)
char* __cdecl NiTextureEffect_TextureModeViewerString(const char* prefix, uint32_t mode) {
    uint32_t size = uint32_t(std::strlen(prefix)) + 0x19;
    char* text = static_cast<char*>(NiMalloc(size));
    switch (mode) {
    case 0:
        NiSprintf(text, size, "%s = PROJECTED_LIGHT", prefix);
        break;
    case 1:
        NiSprintf(text, size, "%s = PROJECTED_SHADOW", prefix);
        break;
    case 2:
        NiSprintf(text, size, "%s = ENVIRONMENT_MAP", prefix);
        break;
    case 3:
        NiSprintf(text, size, "%s = FOG_MAP", prefix);
        break;
    default:
        break;
    }
    return text;
}

// 0x76C550: the coordinate mode's viewer string
char* __cdecl NiTextureEffect_CoordModeViewerString(const char* prefix, uint32_t mode) {
    uint32_t size = uint32_t(std::strlen(prefix)) + 0x19;
    char* text = static_cast<char*>(NiMalloc(size));
    switch (mode) {
    case 0:
        NiSprintf(text, size, "%s = WORLD_PARALLEL", prefix);
        break;
    case 1:
        NiSprintf(text, size, "%s = WORLD_PERSPECTIVE", prefix);
        break;
    case 2:
        NiSprintf(text, size, "%s = SPHERE_MAP", prefix);
        break;
    case 3:
        NiSprintf(text, size, "%s = SPECULAR_CUBE_MAP", prefix);
        break;
    case 4:
        NiSprintf(text, size, "%s = DIFFUSE_CUBE_MAP", prefix);
        break;
    default:
        NiSprintf(text, size, "%s = UNKNOWN!!!", prefix);
        break;
    }
    return text;
}

// 0x76C630: GetViewerStrings (slot 9): the texture mode twice, as the game has it
void __fastcall NiTextureEffect_GetViewerStrings(NiTextureEffect* self, re::Edx, NiTArray<char*>* strings) {
    DynamicEffect_GetViewerStrings(self, strings);
    Add(strings, GetViewerString(NiTextureEffect_RTTI.name));
    Add(strings, Matrix3_GetViewerString(&self->modelProjMat, "m_kModelProjMat"));
    Add(strings, Point3_GetViewerString(&self->modelProjTrans, "m_kModelProjTrans"));
    Add(strings, TextureModeViewerString("m_eTextureMode", self->textureMode));
    Add(strings, FilterModeViewerString("m_eFilter", self->filter));
    Add(strings, ClampModeViewerString("m_eClamp", self->clamp));
    Add(strings, TextureModeViewerString("m_eTextureMode", self->textureMode));
    Add(strings, CoordModeViewerString("m_eCoordMode", self->coordMode));
    Add(strings, GetViewerStringBool("m_bPlaneEnable", self->planeEnable));
    Add(strings, Plane_GetViewerString(&self->modelPlane, "m_kModelPlane"));
}

#define NITEXEFFECT_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NITEXEFFECT_REVERSED("NiPoint3::operator==", 0x0076BA40, NiPoint3_Equals);
NITEXEFFECT_REVERSED("NiTextureEffect::NiTextureEffect", 0x0076BA80, NiTextureEffect_Construct);
NITEXEFFECT_REVERSED("NiTextureEffect::GetRTTI", 0x0076BB40, NiTextureEffect_GetRTTI);
NITEXEFFECT_REVERSED("NiTextureEffect::GetTextureTypeFromName", 0x0076BB50, NiTextureEffect_GetTextureTypeFromName);
NITEXEFFECT_REVERSED("NiTextureEffect::UpdateProjection", 0x0076BBA0, NiTextureEffect_UpdateProjection);
NITEXEFFECT_REVERSED("NiTextureEffect::UpdateWorldData", 0x0076BDF0, NiTextureEffect_UpdateWorldData);
NITEXEFFECT_REVERSED("NiTextureEffect::LinkObject", 0x0076BE20, NiTextureEffect_LinkObject);
NITEXEFFECT_REVERSED("NiTextureEffect::RegisterStreamables", 0x0076BE80, NiTextureEffect_RegisterStreamables);
NITEXEFFECT_REVERSED("NiTextureEffect::DeletingDestructor", 0x0076BF00, NiTextureEffect_DeletingDestructor);
NITEXEFFECT_REVERSED("NiTextureEffect::_SDMInit", 0x0076BF60, NiTextureEffect_SDMInit);
// (never hooked in the 32-bit game: a body at its address for the 64-bit build)
RE_UNHOOKED_FN("NiTextureEffect::_SDMShutdown", 0x0076BFA0, &NiTextureEffect_SDMShutdown);
NITEXEFFECT_REVERSED("NiTextureEffect::CopyMembers", 0x0076BFE0, NiTextureEffect_CopyMembers);
NITEXEFFECT_REVERSED("NiTextureEffect::LoadBinary", 0x0076C110, NiTextureEffect_LoadBinary);
NITEXEFFECT_REVERSED("NiTextureEffect::SaveBinary", 0x0076C290, NiTextureEffect_SaveBinary);
NITEXEFFECT_REVERSED("NiTextureEffect::IsEqual", 0x0076C3A0, NiTextureEffect_IsEqual);
NITEXEFFECT_REVERSED("NiTextureEffect::CreateClone", 0x0076C450, NiTextureEffect_CreateClone);
NITEXEFFECT_REVERSED("NiTextureEffect::TextureModeViewerString", 0x0076C4A0, NiTextureEffect_TextureModeViewerString);
NITEXEFFECT_REVERSED("NiTextureEffect::CoordModeViewerString", 0x0076C550, NiTextureEffect_CoordModeViewerString);
NITEXEFFECT_REVERSED("NiTextureEffect::GetViewerStrings", 0x0076C630, NiTextureEffect_GetViewerStrings);

} // namespace ni
