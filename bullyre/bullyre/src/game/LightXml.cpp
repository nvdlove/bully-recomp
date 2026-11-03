// 0x40B1D0 to 0x40BA40: the lights' XML (TinyXML: PROPERTY elements with a Name
// attribute, a Class and the value as text), a scene-root clump, and the C++ runtime /
// STL code instantiated here (std::bad_alloc, the allocators, std::string::erase).
// Several helpers take their element in a register (eax, esi, ecx with the caller
// cleaning): written, not hooked.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(uint8_t*, __thiscall, NiNode_Construct, 0x00760780, void* self, int childCount);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_FN(void, __thiscall, NiObjectNET_SetName, 0x0077C3A0, void* self, re::Ptr32<char>* name);
GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_FN(void*, __cdecl, CreateClump, 0x0040AD80, void* object, int32_t modelId, uint32_t kind, const char* name);

// TinyXML
GAME_FN(void*, __thiscall, TiXmlNode_FirstChildElement, 0x008574C0, const void* node, const char* name);
GAME_FN(void*, __thiscall, TiXmlNode_NextSiblingElement, 0x00857530, const void* node, const char* name);
GAME_FN(const char*, __thiscall, TiXmlElement_Attribute, 0x00857A90, const void* element, const char* name);
GAME_FN(const char*, __thiscall, TiXmlElement_GetText, 0x00857590, const void* element);
GAME_FN(void, __thiscall, TiXmlNode_Destruct, 0x008579C0, void* self);
template <typename... A>
inline int __cdecl sscanf_(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(void, __cdecl, ParseLightProperties, 0x0040B5D0, void* light, const void* element);
GAME_FN(void, __thiscall, NiTransform_Construct, 0x00770920, void* self);
GAME_ARRAY(const char*, ms_propertyClasses, 0x009D3D40);   // 15: Boolean, ...
GAME_ARRAY(const char*, ms_lightPropertyNames, 0x009D3D7C);   // 10: Cast Shadows, ...

// the C runtime
GAME_FN(void*, __cdecl, CMemoryMgr_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, CMemoryMgr_Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, exception_Construct, 0x0085BDE2, void* self, const re::Ptr32<const char>* message);
GAME_FN(void, __thiscall, exception_Copy, 0x0085BE48, void* self, const void* other);
GAME_FN(void, __thiscall, exception_Destruct, 0x0085BEA0, void* self);
inline void __stdcall CxxThrowException_(void* object, const void* throwInfo) { crt::CxxThrowException(object, throwInfo); }
inline void __cdecl Xran() { crt::Xran(); }   // std::_Xran: throws out_of_range
inline int __cdecl memmove_s_(void* dst, size_t size, const void* src, size_t count) { return crt::MemmoveS(dst, size, src, count); }
GAME_FN(void, __thiscall, Vector_Reserve, 0x006DB480, void* self, uint32_t capacity, int);
GAME_FN(void*, __cdecl, Allocate1, 0x0040B740, uint32_t count, void*);
constexpr uint32_t kBadAllocVtable = 0x008FFF1C;
inline const void* const kBadAllocThrowInfo = reinterpret_cast<const void*>(0x009C1BA0);

void ReleaseString(re::Ptr32<char>* handle) {
    char* s = *handle;
    if (s == ms_nullString) {
        return;
    }
    uint16_t length = *reinterpret_cast<uint16_t*>(s - 2);
    if (vc::Decrement(s - 8) == 1) {
        StringTable_Release(ms_pStringTable, handle, length);
    }
}

void Release(uint8_t* object) {
    if (vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

uint8_t* NewNode(const char* text) {
    void* p = NiNew(0xF4);
    uint8_t* node = nullptr;
    if (p) {
        node = NiNode_Construct(p, 0);
        if (node) {
            vc::Increment(node + 4);
        }
    }
    re::Ptr32<char> name = StringTable_AddString(text);
    NiObjectNET_SetName(node, &name);
    ReleaseString(&name);
    return node;
}

int Compare(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return static_cast<uint8_t>(*a) - static_cast<uint8_t>(*b);
}

[[noreturn]] void ThrowBadAlloc() {
    uint8_t exception[0xC];
    re::Ptr32<const char> message = nullptr;
    exception_Construct(exception, &message);
    *reinterpret_cast<uint32_t*>(exception) = kBadAllocVtable;
    CxxThrowException_(exception, kBadAllocThrowInfo);
    for (;;) {
    }
}

} // namespace

namespace LightXml {

// 0x40B1D0: a clump of an empty "Scene Root" with a "mot_ctrl" child (kind 4). (The
// memory tag pushed is never popped: the game's)
static void* __cdecl CreateEmptyClump(int32_t modelId) {
    PushMemoryTag(4);
    uint8_t* root = NewNode("Scene Root");
    uint8_t* child = NewNode("mot_ctrl");
    vc::Call<void>(root, 0x88, child, 0);   // AttachChild
    void* clump = CreateClump(root, modelId, 4, "NULL");
    if (child) {
        Release(child);
    }
    Release(root);
    return clump;
}

// 0x40B340 (thiscall): std::bad_alloc's destructor
static void __fastcall BadAlloc_Destruct(uint32_t* self, re::Edx) {
    *self = kBadAllocVtable;
    exception_Destruct(self);
}

// 0x40B350 (thiscall): its deleting destructor
static uint32_t* __fastcall BadAlloc_Destroy(uint32_t* self, re::Edx, int flags) {
    *self = kBadAllocVtable;
    exception_Destruct(self);
    if (flags & 1) {
        CMemoryMgr_Free(self);
    }
    return self;
}

// 0x40B380 (stdcall): a light element's "Light Type": 0 Ambient, 1 Point, 2 Spot, 3
// Directional, 4 anything else or none
static uint32_t __stdcall GetLightType(const void* element) {
    for (void* p = TiXmlNode_FirstChildElement(element, "PROPERTY"); p; p = TiXmlNode_NextSiblingElement(p, "PROPERTY")) {
        const char* name = TiXmlElement_Attribute(p, "Name");
        if (!name || Compare(name, "Light Type") != 0) {
            continue;
        }
        const char* value = TiXmlElement_GetText(p);
        if (!value) {
            continue;
        }
        if (Compare(value, "Ambient") == 0) {
            return 0;
        }
        if (Compare(value, "Point") == 0) {
            return 1;
        }
        if (Compare(value, "Spot") == 0) {
            return 2;
        }
        return Compare(value, "Directional") != 0 ? 4 : 3;
    }
    return 4;
}

// 0x40B490 (the element in ecx, the caller cleaning the stack): the property's Class is
// the `index`th of the class names (an unknown class counts as the first)
[[maybe_unused]] static uint8_t IsPropertyClass(const void* element, uint32_t index) {
    const char* name = TiXmlElement_Attribute(element, "Class");
    uint32_t i = 0;
    for (; i < 15; i++) {
        if (Compare(name, ms_propertyClasses[i]) == 0) {
            break;
        }
    }
    if (i == 15) {
        i = 0;
    }
    return i == index;
}
RE_UNHOOKED_FN("LightXml::IsPropertyClass", 0x0040B490, &IsPropertyClass);

// 0x40B4F0 (the element in eax): a Boolean property's value (anything but "FALSE")
[[maybe_unused]] static uint8_t GetBool(const void* element) {
    if (!IsPropertyClass(element, 0)) {
        return 0;
    }
    return Compare(TiXmlElement_GetText(element), "FALSE") != 0;
}
RE_UNHOOKED_FN("LightXml::GetBool", 0x0040B4F0, &GetBool);

// 0x40B540 (the element in esi): an integer property's value (as 16 bits)
[[maybe_unused]] static uint16_t GetInt(const void* element) {
    if (!IsPropertyClass(element, 1)) {
        return 0;
    }
    int32_t value = 0;
    sscanf_(TiXmlElement_GetText(element), "%d", &value);
    return uint16_t(value);
}
RE_UNHOOKED_FN("LightXml::GetInt", 0x0040B540, &GetInt);

// 0x40B580 (the name in esi): the light property's index (0 when unknown)
[[maybe_unused]] static uint32_t GetLightPropertyIndex(const char* name) {
    for (uint32_t i = 0; i < 10; i++) {
        if (Compare(name, ms_lightPropertyNames[i]) == 0) {
            return i;
        }
    }
    return 0;
}
RE_UNHOOKED_FN("LightXml::GetLightPropertyIndex", 0x0040B580, &GetLightPropertyIndex);

// 0x40B5D0: a light's properties from its element: flags at +0x78, +0x79, +0x7C, +0x7D,
// +0x7E, an integer at +0x7A, a float at +0x80
static void __cdecl ParseLightProperties_(uint8_t* light, const void* element) {
    for (const void* p = TiXmlNode_FirstChildElement(element, "PROPERTY"); p;
         p = TiXmlNode_NextSiblingElement(p, "PROPERTY")) {
        const char* name = TiXmlElement_Attribute(p, "Name");
        if (!name) {
            continue;
        }
        switch (GetLightPropertyIndex(name)) {
        case 0:
            light[0x78] = GetBool(p);
            break;
        case 1:
            light[0x79] = GetBool(p);
            break;
        case 2:
            *reinterpret_cast<uint16_t*>(light + 0x7A) = GetInt(p);
            break;
        case 3:
            light[0x7C] = GetBool(p);
            break;
        case 7:
            light[0x7D] = GetBool(p);
            break;
        case 8:
            light[0x7E] = GetBool(p);
            break;
        case 9: {
            float value = 0.0f;
            if (IsPropertyClass(p, 2)) {
                float parsed = 0.0f;
                sscanf_(TiXmlElement_GetText(p), "%f", &parsed);
                value = parsed;
            }
            *reinterpret_cast<float*>(light + 0x80) = value;
            break;
        }
        default:
            break;
        }
    }
}

// 0x40B700 (stdcall): a light's properties from its template then its own element
static void __stdcall ParseLight(const void* element, const void* templateElement, void* light) {
    if (!element || !light) {
        return;
    }
    if (templateElement) {
        ParseLightProperties(light, templateElement);
    }
    ParseLightProperties(light, element);
}

// 0x40B740: std::allocator<char>::allocate (bad_alloc on overflow)
static void* __cdecl AllocateBytes(uint32_t count, void*) {
    if (count > 0 && 0xFFFFFFFFu / count < 1) {
        ThrowBadAlloc();
    }
    return CMemoryMgr_Malloc(count);
}

// 0x40B7A0: std::allocator of 8 byte items
static void* __cdecl Allocate8(uint32_t count, void*) {
    if (count > 0 && 0xFFFFFFFFu / count < 8) {
        ThrowBadAlloc();
    }
    return CMemoryMgr_Malloc(count * 8);
}

// 0x40B800 (thiscall): std::bad_alloc's copy constructor
static uint32_t* __fastcall BadAlloc_Copy(uint32_t* self, re::Edx, const void* other) {
    exception_Copy(self, other);
    *self = kBadAllocVtable;
    return self;
}

// 0x40B820 (thiscall): TiXmlDocument's deleting destructor (its error string freed
// unless it's the shared empty one)
static uint8_t* __fastcall TiXmlDocument_Destroy(uint8_t* self, re::Edx, int flags) {
    *reinterpret_cast<uint32_t*>(self) = 0x008FFF84;
    void* rep = *reinterpret_cast<re::Ptr32<void>*>(self + 0x34);
    if (rep != reinterpret_cast<void*>(0x020E6004)) {
        CMemoryMgr_Free(rep);
    }
    TiXmlNode_Destruct(self);
    if (flags & 1) {
        CMemoryMgr_Free(self);
    }
    return self;
}

// 0x40B860: a transform from an element's Translation, Scale and Rotation (three ROWs)
static void __stdcall ParseTransform(const void* element, uint8_t* transform) {
    NiTransform_Construct(transform);
    for (const void* p = TiXmlNode_FirstChildElement(element, "PROPERTY"); p;
         p = TiXmlNode_NextSiblingElement(p, "PROPERTY")) {
        const char* name = TiXmlElement_Attribute(p, "Name");
        if (!name) {
            continue;
        }
        if (Compare(name, "Translation") == 0) {
            const char* text = TiXmlElement_GetText(p);
            if (text) {
                sscanf_(text, "%f, %f, %f", transform + 0x24, transform + 0x28, transform + 0x2C);
            }
        }
        if (Compare(name, "Scale") == 0) {
            const char* text = TiXmlElement_GetText(p);
            if (text) {
                sscanf_(text, "%f", transform + 0x30);
            }
        }
        if (Compare(name, "Rotation") == 0) {
            float* row = reinterpret_cast<float*>(transform);
            int i = 0;
            for (const void* r = TiXmlNode_FirstChildElement(p, "ROW"); r && i <= 2;
                 r = TiXmlNode_NextSiblingElement(r, "ROW"), i++, row += 3) {
                const char* text = TiXmlElement_GetText(r);
                if (text) {
                    float x = 0, y = 0, z = 0;   // (uninitialised in the game when parsing fails)
                    sscanf_(text, "%f, %f, %f", &x, &y, &z);
                    row[0] = x;
                    row[1] = y;
                    row[2] = z;
                }
            }
        }
    }
}

// 0x40B9E0 (thiscall; `this` unused): std::allocator<char>::allocate
static void* __fastcall Allocator_Allocate(void*, re::Edx, uint32_t count) {
    return Allocate1(count, nullptr);
}

// 0x40BA00 (thiscall): a pointer vector's push_back (+0 items, +4 capacity, +8 size;
// doubling)
static void __fastcall PointerVector_PushBack(uint32_t* self, re::Edx, const uint32_t* item) {
    uint32_t capacity = self[1];
    if (self[2] == capacity) {
        Vector_Reserve(self, capacity ? capacity * 2 : 1, 0);
    }
    reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(self[0]))[self[2]] = *item;
    self[2]++;
}

// 0x40BA40 (thiscall): std::string::erase(pos, count) (+4 the buffer or its pointer when
// the capacity (+0x18) is 16 or more, +0x14 the size)
static uint8_t* __fastcall String_Erase(uint8_t* self, re::Edx, uint32_t pos, uint32_t count) {
    uint32_t& size = *reinterpret_cast<uint32_t*>(self + 0x14);
    if (size < pos) {
        Xran();
    }
    uint32_t rest = size - pos;
    if (rest < count) {
        count = rest;
    }
    if (count == 0) {
        return self;
    }
    uint32_t capacity = *reinterpret_cast<uint32_t*>(self + 0x18);
    char* data = capacity >= 0x10 ? static_cast<char*>(*reinterpret_cast<re::Ptr32<char>*>(self + 4)) : reinterpret_cast<char*>(self + 4);
    memmove_s_(data + pos, capacity - pos, data + pos + count, rest - count);
    size -= count;
    data = *reinterpret_cast<uint32_t*>(self + 0x18) >= 0x10 ? static_cast<char*>(*reinterpret_cast<re::Ptr32<char>*>(self + 4))
                                                              : reinterpret_cast<char*>(self + 4);
    data[size] = 0;
    return self;
}

} // namespace LightXml

// ---- registration ----------------------------------------------------------------------

#define C00_LIGHTXML_REVERSED(name, fn, address)                                                    \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace LightXml {
C00_LIGHTXML_REVERSED("LightXml::CreateEmptyClump", CreateEmptyClump, 0x0040B1D0);
C00_LIGHTXML_REVERSED("std::bad_alloc::Destruct", BadAlloc_Destruct, 0x0040B340);
C00_LIGHTXML_REVERSED("std::bad_alloc::Destroy", BadAlloc_Destroy, 0x0040B350);
C00_LIGHTXML_REVERSED("LightXml::GetLightType", GetLightType, 0x0040B380);
C00_LIGHTXML_REVERSED("LightXml::ParseLightProperties", ParseLightProperties_, 0x0040B5D0);
C00_LIGHTXML_REVERSED("LightXml::ParseLight", ParseLight, 0x0040B700);
C00_LIGHTXML_REVERSED("std::allocator<char>::allocate", AllocateBytes, 0x0040B740);
C00_LIGHTXML_REVERSED("std::allocator<8>::allocate", Allocate8, 0x0040B7A0);
C00_LIGHTXML_REVERSED("std::bad_alloc::Copy", BadAlloc_Copy, 0x0040B800);
C00_LIGHTXML_REVERSED("TiXmlDocument::Destroy", TiXmlDocument_Destroy, 0x0040B820);
C00_LIGHTXML_REVERSED("LightXml::ParseTransform", ParseTransform, 0x0040B860);
C00_LIGHTXML_REVERSED("std::allocator<char>::allocate_this", Allocator_Allocate, 0x0040B9E0);
C00_LIGHTXML_REVERSED("PointerVector::PushBack", PointerVector_PushBack, 0x0040BA00);
C00_LIGHTXML_REVERSED("std::string::erase", String_Erase, 0x0040BA40);
} // namespace LightXml
