// NiShaderConstantMap's statics: the predefined mappings' and object mappings' tables (each an
// array of {id, NiFixedString name}: 0x20E3470 / count 0x20E3474, 0x20E3478 / count 0x20E347C),
// looked up by name (a "_N" suffix taken as the index) or by id, the predefined mappings'
// attribute types and valid register counts, and the dynamic effect an object mapping refers to.

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct Mapping {
    uint32_t id;           // +0x0
    re::Ptr32<const char> name;    // +0x4
};

GAME_VAR(Mapping*, PredefinedMappings, 0x020E3470);
GAME_VAR(uint32_t, PredefinedMappingCount, 0x020E3474);
GAME_VAR(Mapping*, ObjectMappings, 0x020E3478);
GAME_VAR(uint32_t, ObjectMappingCount, 0x020E347C);

GAME_FN(char*, __cdecl, NiStrcpy, 0x00747280, char* to, uint32_t size, const char* from);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
// (this file's, called where modes apply)
GAME_FN(uint32_t, __cdecl, ConstantMap_GetPredefinedMappingType, 0x0077EF10, uint32_t id, int count);
GAME_FN(uint8_t, __cdecl, ConstantMap_ParseIndexedName, 0x0077F120, const char* name, char* out, uint32_t outSize,
        long* index);
GAME_FN(uint8_t, __cdecl, ConstantMap_LookUpPredefinedMapping, 0x0077F210, const char* name, uint32_t* id,
        long* index);

// NiFixedString's assignment, inlined (the import slots' interlocked calls)
inline void FixedStringSet(re::Ptr32<const char>* slot, NiFixedString value) {
    if (*slot == value) {
        return;
    }
    if (value != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(value) - 8);
    }
    NiFixedString old = *slot;
    if (old != NullFixedString) {
        uint32_t length = *reinterpret_cast<const uint16_t*>(old - 2);
        if (ni::sys::Decrement(re::PtrCast<char*>(old) - 8) == 1) {
            StringTable_RemoveString(GlobalStringTable, slot, length);
        }
    }
    *slot = value;
}

// the predefined mapping's attribute type (8 matrix3, 9 matrix4, 7 point4, 6 point3, 5 point2,
// 4 float): the transform mappings are 3x3 for a count of 3
uint32_t __cdecl GetPredefinedMappingType(uint32_t id, int count) {
    if (id == 0 || id > 117) {
        return 0;
    }
    if ((id >= 1 && id <= 24) || (id >= 37 && id <= 68) || (id >= 88 && id <= 95)) {
        return count != 3 ? 9 : 8;
    }
    if ((id >= 25 && id <= 32) || id == 34 || id == 36 || (id >= 96 && id <= 99)) {
        return 9;
    }
    if (id == 33 || id == 35) {
        return 8;
    }
    if ((id >= 69 && id <= 87) || (id >= 100 && id <= 103)) {
        return 7;
    }
    if (id == 104 || (id >= 106 && id <= 115)) {
        return 5;
    }
    if (id == 105 || id == 117) {
        return 4;
    }
    if (id == 116) {
        return 6;
    }
    return 0;
}

// whether a predefined mapping takes this many registers
uint8_t __cdecl IsValidRegisterCount(uint32_t id, int count) {
    switch (id) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 10:
    case 11:
    case 14:
    case 15:
    case 25:
    case 27:
    case 28:
    case 29:
        return count >= 1 && count <= 7 ? 1 : 0;
    case 8:
    case 9:
    case 12:
    case 13:
        return count >= 1 && count <= 11 ? 1 : 0;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
        return count >= 8 && count <= 11 ? 1 : 0;
    default:
        return 0;
    }
}

// ParseIndexedName(name, out, outSize, index): the name copied; "NAME_D" (one digit) split into
// NAME and the index. False only for a buffer under 0x103 bytes
uint8_t __cdecl ParseIndexedName(const char* name, char* out, uint32_t outSize, long* index) {
    if (outSize + 1 < 0x104) {
        return 0;
    }
    char copy[0x104];
    crt::StrcpyS(copy, 0x104, name);
    crt::StrcpyS(out, 0x104, name);
    char* underscore = crt::Strchr(copy, '_');
    // (isdigit: the C locale's 0..9; the game's table lookup for a negative char isn't kept)
    if (underscore != nullptr && underscore[1] >= '0' && underscore[1] <= '9' && underscore[2] == 0) {
        char* context = nullptr;
        char* first = strtok_s(copy, "_", &context);
        char* second = strtok_s(nullptr, "_", &context);
        if (second != nullptr) {
            *index = crt::Atol(second);
            NiStrcpy(out, outSize, first);
        }
    }
    return 1;
}

// LookUpPredefinedMapping(name, id, index)
uint8_t __cdecl LookUpPredefinedMapping(const char* name, uint32_t* id, long* index) {
    char base[0x104];
    if (!ConstantMap_ParseIndexedName(name, base, 0x104, index)) {
        return 0;
    }
    for (uint32_t i = 0; i < PredefinedMappingCount; i++) {
        Mapping* mapping = &PredefinedMappings[i];
        if (NiStricmp(mapping->name, base) == 0) {
            *id = mapping->id;
            return 1;
        }
    }
    return 0;
}

// the type of a predefined mapping by name (the index is the stack's when the name has none: 0
// here, a deviation only for the transform mappings' 3x3 case)
uint32_t __cdecl GetPredefinedMappingTypeByName(const char* name) {
    uint32_t id = 0;
    long index = 0;
    if (ConstantMap_LookUpPredefinedMapping(name, &id, &index) && id != 0) {
        return ConstantMap_GetPredefinedMappingType(id, index);
    }
    return 0;
}

// LookUpObjectMapping(name, id)
uint8_t __cdecl LookUpObjectMapping(const char* name, uint32_t* id) {
    for (uint32_t i = 0; i < ObjectMappingCount; i++) {
        Mapping* mapping = &ObjectMappings[i];
        if (NiStricmp(mapping->name, name) == 0) {
            *id = mapping->id;
            return 1;
        }
    }
    return 0;
}

// the index-th switched-on light of a kind (1: any) on the state's light list
inline void* FindLight(re::Ptr32<void>* list, uint32_t kind, uint32_t index, bool* found) {
    bool general = kind == 1;
    uint32_t n = 0;
    for (re::Ptr32<void>* item = list; item != nullptr;) {
        void* light = item[1];
        item = static_cast<re::Ptr32<void>*>(item[0]);
        uint32_t type = At<uint8_t>(light, 0xC5);
        uint8_t match = uint8_t(((type > kind) && general) ^ (type == kind));
        if ((At<uint8_t>(light, 0xC4) & match) != 0) {
            if (n++ == index) {
                *found = true;
                return light;
            }
        }
    }
    *found = false;
    return nullptr;
}

// the index-th switched-on effect on a list (null entries skipped)
inline void* FindEffect(re::Ptr32<void>* list, uint32_t index) {
    uint32_t n = 0;
    for (re::Ptr32<void>* item = list; item != nullptr;) {
        void* effect = item[1];
        item = static_cast<re::Ptr32<void>*>(item[0]);
        if (effect != nullptr && At<uint8_t>(effect, 0xC4) != 0) {
            if (n++ == index) {
                return effect;
            }
        }
    }
    return nullptr;
}

// GetDynamicEffectForObject(state, type, index): lights (1 any, 2..4 a kind: its shadowed kind,
// three on, looked for after), 5..7 the shadowed kinds, 8 the environment map, 9 the projected
// shadows, 10 the projected lights, 11 the fog map; switched-on ones only
void* __cdecl GetDynamicEffectForObject(void* state, uint32_t type, uint32_t index) {
    if (state == nullptr) {
        return nullptr;
    }
    switch (type) {
    case 1:
    case 2:
    case 3:
    case 4: {
        re::Ptr32<void>* list = At<re::Ptr32<void>*>(state, 0xC);
        if (list == nullptr) {
            return nullptr;
        }
        bool found;
        void* light = FindLight(list, type, index, &found);
        if (found) {
            return light;
        }
        light = FindLight(list, type + 3, index, &found);
        return found ? light : nullptr;
    }
    case 5:
    case 6:
    case 7: {
        re::Ptr32<void>* list = At<re::Ptr32<void>*>(state, 0xC);
        if (list == nullptr) {
            return nullptr;
        }
        bool found;
        void* light = FindLight(list, type, index, &found);
        return found ? light : nullptr;
    }
    case 8:
    case 11: {
        void* effect = At<void*>(state, type == 8 ? 0x18 : 0x1C);
        if (effect != nullptr && At<uint8_t>(effect, 0xC4) == 0) {
            effect = nullptr;
        }
        return effect;
    }
    case 9:
        return At<re::Ptr32<void>*>(state, 0x14) != nullptr ? FindEffect(At<re::Ptr32<void>*>(state, 0x14), index) : nullptr;
    case 10:
        return At<re::Ptr32<void>*>(state, 0x10) != nullptr ? FindEffect(At<re::Ptr32<void>*>(state, 0x10), index) : nullptr;
    default:
        return nullptr;
    }
}

// LookUpPredefinedMappingName(id, name) / LookUpObjectMappingName(id, name)
uint8_t __cdecl LookUpPredefinedMappingName(uint32_t id, re::Ptr32<const char>* name) {
    for (uint32_t i = 0; i < PredefinedMappingCount; i++) {
        if (PredefinedMappings[i].id == id) {
            FixedStringSet(name, PredefinedMappings[i].name);
            return 1;
        }
    }
    return 0;
}

uint8_t __cdecl LookUpObjectMappingName(uint32_t id, re::Ptr32<const char>* name) {
    for (uint32_t i = 0; i < ObjectMappingCount; i++) {
        if (ObjectMappings[i].id == id) {
            FixedStringSet(name, ObjectMappings[i].name);
            return 1;
        }
    }
    return 0;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NICONSTMAP_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NICONSTMAP_REVERSED("NiShaderConstantMap::GetPredefinedMappingType", 0x0077EF10, &ni::GetPredefinedMappingType);
NICONSTMAP_REVERSED("NiShaderConstantMap::IsValidRegisterCount", 0x0077F000, &ni::IsValidRegisterCount);
NICONSTMAP_REVERSED("NiShaderConstantMap::ParseIndexedName", 0x0077F120, &ni::ParseIndexedName);
NICONSTMAP_REVERSED("NiShaderConstantMap::LookUpPredefinedMapping", 0x0077F210, &ni::LookUpPredefinedMapping);
NICONSTMAP_REVERSED("NiShaderConstantMap::GetPredefinedMappingTypeByName", 0x0077F2F0,
                    &ni::GetPredefinedMappingTypeByName);
NICONSTMAP_REVERSED("NiShaderConstantMap::LookUpObjectMapping", 0x0077F330, &ni::LookUpObjectMapping);
NICONSTMAP_REVERSED("NiShaderConstantMap::GetDynamicEffectForObject", 0x0077F380, &ni::GetDynamicEffectForObject);
NICONSTMAP_REVERSED("NiShaderConstantMap::LookUpPredefinedMappingName", 0x0077F610, &ni::LookUpPredefinedMappingName);
NICONSTMAP_REVERSED("NiShaderConstantMap::LookUpObjectMappingName", 0x0077F690, &ni::LookUpObjectMappingName);
