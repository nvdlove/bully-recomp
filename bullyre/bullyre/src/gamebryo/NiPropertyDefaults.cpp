// The properties' default instances (each class's ms_spDefault, made by its _SDMInit from
// NiMainSDM::Init: the property built inline as its CreateObject builds it, then the static
// NiPointer assigned, the old one let go first), and NiTexturingProperty's map names (the
// NiFixedStrings its _SDMInit sets up) with the lookup by name.

#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

GAME_ARRAY(const uint32_t, Color_FogDefault, 0x020E25E8);   // the fog colour's default (NiColor)

GAME_VAR(NiRefObject*, NiTexturingProperty_ms_spDefault, 0x020E28F0);
GAME_ARRAY(const char*, NiTexturingProperty_ms_akMapNames, 0x020E28F4);   // [10]
GAME_VAR(NiRefObject*, NiAlphaProperty_ms_spDefault, 0x020E2B24);
GAME_VAR(NiRefObject*, NiZBufferProperty_ms_spDefault, 0x020E2B5C);
GAME_VAR(NiRefObject*, NiMaterialProperty_ms_spDefault, 0x020E2D38);
GAME_VAR(NiRefObject*, NiFogProperty_ms_spDefault, 0x020E2F2C);
GAME_VAR(NiRefObject*, NiShadeProperty_ms_spDefault, 0x020E2F64);
GAME_VAR(NiRefObject*, NiStencilProperty_ms_spDefault, 0x020E2F9C);
GAME_VAR(NiRefObject*, NiVertexColorProperty_ms_spDefault, 0x020E303C);

GAME_FN(void*, __thiscall, ObjectNET_Construct, 0x00764550, void* self);
GAME_FN(void*, __thiscall, TexturingProperty_Construct, 0x00765DD0, void* self);
GAME_FN(void*, __thiscall, MaterialProperty_Construct, 0x00775660, void* self);
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);

constexpr uintptr_t NiAlphaProperty_vtable = 0x008FFB9C;
constexpr uintptr_t NiZBufferProperty_vtable = 0x008FFCA4;
constexpr uintptr_t NiFogProperty_vtable = 0x009179FC;
constexpr uintptr_t NiShadeProperty_vtable = 0x00917A54;
constexpr uintptr_t NiStencilProperty_vtable = 0x00917AAC;
constexpr uintptr_t NiVertexColorProperty_vtable = 0x0092D0D4;

// the static NiPointer assigned (NiPointer::operator=, inlined: the old one released first)
inline void AssignDefault(re::Ptr32<NiRefObject>& slot, void* value) {
    NiRefObject* object = static_cast<NiRefObject*>(value);
    NiRefObject* old = slot;
    if (old != object) {
        if (old != nullptr && Decrement(&old->refCount) == 0) {
            Virtual<void>(old, 1);
        }
        slot = object;
        if (object != nullptr) {
            Increment(&object->refCount);
        }
    }
}

inline void* NewProperty(uint32_t size, uintptr_t vtable) {
    void* object = NiNew(size);
    if (object == nullptr) {
        return nullptr;
    }
    ObjectNET_Construct(object);
    *static_cast<re::Ptr32<void>*>(object) = reinterpret_cast<void*>(vtable);
    return object;
}

} // namespace

// 0x76CFB0: NiAlphaProperty::_SDMInit
void __cdecl NiAlphaProperty_SDMInit() {
    void* object = NewProperty(0x1C, NiAlphaProperty_vtable);
    if (object != nullptr) {
        At<uint8_t>(object, 0x1A) = 0;
        At<uint16_t>(object, 0x18) = 0xEC;
    }
    AssignDefault(NiAlphaProperty_ms_spDefault, object);
}

// 0x76D740: NiZBufferProperty::_SDMInit
void __cdecl NiZBufferProperty_SDMInit() {
    void* object = NiNew(0x30);
    if (object != nullptr) {
        ObjectNET_Construct(object);
        At<float>(object, 0x1C) = 0.0f;
        *static_cast<re::Ptr32<void>*>(object) = reinterpret_cast<void*>(NiZBufferProperty_vtable);
        At<float>(object, 0x20) = 0.0f;
        At<uint32_t>(object, 0x24) = 0;
        At<uint16_t>(object, 0x18) = 0xF;
        At<float>(object, 0x28) = 1.0f;
        At<float>(object, 0x2C) = 1.0f;
    }
    AssignDefault(NiZBufferProperty_ms_spDefault, object);
}

// 0x775950: NiMaterialProperty::_SDMInit
void __cdecl NiMaterialProperty_SDMInit() {
    void* object = NiNew(0x5C);
    if (object != nullptr) {
        object = MaterialProperty_Construct(object);
    }
    AssignDefault(NiMaterialProperty_ms_spDefault, object);
}

// 0x7783C0: NiFogProperty::_SDMInit
void __cdecl NiFogProperty_SDMInit() {
    void* object = NewProperty(0x2C, NiFogProperty_vtable);
    if (object != nullptr) {
        At<float>(object, 0x20) = 0.0f;
        At<float>(object, 0x24) = 0.0f;
        At<float>(object, 0x28) = 0.0f;
        At<uint16_t>(object, 0x18) = 0;
        At<float>(object, 0x1C) = 1.0f;
        At<uint32_t>(object, 0x20) = Color_FogDefault[0];
        At<uint32_t>(object, 0x24) = Color_FogDefault[1];
        At<uint32_t>(object, 0x28) = Color_FogDefault[2];
    }
    AssignDefault(NiFogProperty_ms_spDefault, object);
}

// 0x7786D0: NiShadeProperty::_SDMInit
void __cdecl NiShadeProperty_SDMInit() {
    void* object = NewProperty(0x1C, NiShadeProperty_vtable);
    if (object != nullptr) {
        At<uint16_t>(object, 0x18) = 1;
    }
    AssignDefault(NiShadeProperty_ms_spDefault, object);
}

// 0x778BC0: NiStencilProperty::_SDMInit
void __cdecl NiStencilProperty_SDMInit() {
    void* object = NewProperty(0x24, NiStencilProperty_vtable);
    if (object != nullptr) {
        At<uint32_t>(object, 0x1C) = 0;
        At<uint32_t>(object, 0x20) = 0xFFFFFFFF;
        At<uint16_t>(object, 0x18) = 0x4180;
    }
    AssignDefault(NiStencilProperty_ms_spDefault, object);
}

// 0x779670: NiVertexColorProperty::_SDMInit
void __cdecl NiVertexColorProperty_SDMInit() {
    void* object = NewProperty(0x1C, NiVertexColorProperty_vtable);
    if (object != nullptr) {
        At<uint16_t>(object, 0x18) = 8;
    }
    AssignDefault(NiVertexColorProperty_ms_spDefault, object);
}

// 0x766C70: NiTexturingProperty::_SDMInit: the default, then the map names
void __cdecl NiTexturingProperty_SDMInit() {
    void* object = NiNew(0x34);
    if (object != nullptr) {
        object = TexturingProperty_Construct(object);
    }
    AssignDefault(NiTexturingProperty_ms_spDefault, object);
    static const char* const kNames[10] = {
        "Base", "Dark", "Detail", "Gloss", "Glow", "Bump", "Normal", "Parallax", "Decal", "Shader",
    };
    for (int i = 0; i < 10; i++) {
        FixedString_AssignText(&NiTexturingProperty_ms_akMapNames[i], kNames[i]);
    }
}

// ---- the _SDMShutdowns (NiMainSDM::Shutdown's): each default let go and the static NiPointer
// emptied (assigning null does both); NiTexturingProperty's map names emptied too. Registered
// unhooked: never hooked in the 32-bit game, bodies at their addresses for the 64-bit build

// 0x76D020: NiAlphaProperty::_SDMShutdown
void __cdecl NiAlphaProperty_SDMShutdown() {
    AssignDefault(NiAlphaProperty_ms_spDefault, nullptr);
}

// 0x76D7C0: NiZBufferProperty::_SDMShutdown
void __cdecl NiZBufferProperty_SDMShutdown() {
    AssignDefault(NiZBufferProperty_ms_spDefault, nullptr);
}

// 0x7759B0: NiMaterialProperty::_SDMShutdown
void __cdecl NiMaterialProperty_SDMShutdown() {
    AssignDefault(NiMaterialProperty_ms_spDefault, nullptr);
}

// 0x778450: NiFogProperty::_SDMShutdown
void __cdecl NiFogProperty_SDMShutdown() {
    AssignDefault(NiFogProperty_ms_spDefault, nullptr);
}

// 0x778740: NiShadeProperty::_SDMShutdown
void __cdecl NiShadeProperty_SDMShutdown() {
    AssignDefault(NiShadeProperty_ms_spDefault, nullptr);
}

// 0x778C40: NiStencilProperty::_SDMShutdown
void __cdecl NiStencilProperty_SDMShutdown() {
    AssignDefault(NiStencilProperty_ms_spDefault, nullptr);
}

// 0x7796E0: NiVertexColorProperty::_SDMShutdown
void __cdecl NiVertexColorProperty_SDMShutdown() {
    AssignDefault(NiVertexColorProperty_ms_spDefault, nullptr);
}

// 0x766D60: NiTexturingProperty::_SDMShutdown: the default, then the map names
void __cdecl NiTexturingProperty_SDMShutdown() {
    AssignDefault(NiTexturingProperty_ms_spDefault, nullptr);
    for (int i = 0; i < 10; i++) {
        FixedString_AssignText(&NiTexturingProperty_ms_akMapNames[i], nullptr);
    }
}

// 0x765B50: NiTexturingProperty::GetMapIDFromName(name, id&): the map whose name it is (the
// same handle, or the same text ignoring case); false for none
uint8_t __cdecl NiTexturingProperty_GetMapIDFromName(const re::Ptr32<const char>* name, uint32_t* id) {
    for (uint32_t i = 0; i < 10; i++) {
        const char* mapName = NiTexturingProperty_ms_akMapNames[i];
        const char* text = *name;
        if (mapName == text ||
            (text != nullptr && mapName != nullptr && crt::Stricmp(mapName, text) == 0)) {
            *id = i;
            return 1;
        }
    }
    return 0;
}

#define NIPROPDEFAULTS_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIPROPDEFAULTS_REVERSED("NiTexturingProperty::GetMapIDFromName", 0x00765B50, NiTexturingProperty_GetMapIDFromName);
NIPROPDEFAULTS_REVERSED("NiTexturingProperty::_SDMInit", 0x00766C70, NiTexturingProperty_SDMInit);
NIPROPDEFAULTS_REVERSED("NiAlphaProperty::_SDMInit", 0x0076CFB0, NiAlphaProperty_SDMInit);
NIPROPDEFAULTS_REVERSED("NiZBufferProperty::_SDMInit", 0x0076D740, NiZBufferProperty_SDMInit);
NIPROPDEFAULTS_REVERSED("NiMaterialProperty::_SDMInit", 0x00775950, NiMaterialProperty_SDMInit);
NIPROPDEFAULTS_REVERSED("NiFogProperty::_SDMInit", 0x007783C0, NiFogProperty_SDMInit);
NIPROPDEFAULTS_REVERSED("NiShadeProperty::_SDMInit", 0x007786D0, NiShadeProperty_SDMInit);
NIPROPDEFAULTS_REVERSED("NiStencilProperty::_SDMInit", 0x00778BC0, NiStencilProperty_SDMInit);
NIPROPDEFAULTS_REVERSED("NiVertexColorProperty::_SDMInit", 0x00779670, NiVertexColorProperty_SDMInit);
RE_UNHOOKED_FN("NiTexturingProperty::_SDMShutdown", 0x00766D60, &NiTexturingProperty_SDMShutdown);
RE_UNHOOKED_FN("NiAlphaProperty::_SDMShutdown", 0x0076D020, &NiAlphaProperty_SDMShutdown);
RE_UNHOOKED_FN("NiZBufferProperty::_SDMShutdown", 0x0076D7C0, &NiZBufferProperty_SDMShutdown);
RE_UNHOOKED_FN("NiMaterialProperty::_SDMShutdown", 0x007759B0, &NiMaterialProperty_SDMShutdown);
RE_UNHOOKED_FN("NiFogProperty::_SDMShutdown", 0x00778450, &NiFogProperty_SDMShutdown);
RE_UNHOOKED_FN("NiShadeProperty::_SDMShutdown", 0x00778740, &NiShadeProperty_SDMShutdown);
RE_UNHOOKED_FN("NiStencilProperty::_SDMShutdown", 0x00778C40, &NiStencilProperty_SDMShutdown);
RE_UNHOOKED_FN("NiVertexColorProperty::_SDMShutdown", 0x007796E0, &NiVertexColorProperty_SDMShutdown);

} // namespace ni
