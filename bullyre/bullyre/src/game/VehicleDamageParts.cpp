// A vehicle's damageable parts in its Gamebryo model: each part is a "<part>_dummy"
// node holding an "_ok" and a "_dam" version (one shown, the other app-culled), and
// the paint colours set on the materials named "vertexcolor_01/02".

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace c01vparts {

using InterlockedFn = long(__stdcall*)(volatile long*);
GAME_VAR(InterlockedFn, Import_InterlockedIncrement, 0x008FC174);
GAME_VAR(InterlockedFn, Import_InterlockedDecrement, 0x008FC170);

GAME_FN(uint8_t, __thiscall, FixedString_Equals, 0x00406510, re::Ptr32<char>* string, const char* s);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* string);
GAME_FN(void, __thiscall, FixedStringPool_Free, 0x00754550, void* pool, re::Ptr32<char>* string, uint32_t length);
GAME_VAR(void*, fixedStringPool, 0x020E216C);
GAME_VAR(char*, emptyFixedString, 0x00951028);

GAME_FN(void*, __cdecl, NiDynamicCast, 0x00401FF0, const void* rtti, void* object);
GAME_FN(void*, __thiscall, NiNode_GetAt, 0x00401D30, void* node, uint32_t index);
GAME_FN(void*, __thiscall, NiAVObject_GetProperty, 0x0075DC10, void* object, int type);
inline void* __cdecl Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }

inline const void* const NiNode_RTTI = reinterpret_cast<const void*>(0x020E2548);
inline const void* const NiGeometry_RTTI = reinterpret_cast<const void*>(0x020E25B4);   // (a geometry class)
GAME_VAR(uint32_t, niClassBit_NiNode, 0x020E2550);
GAME_VAR(uint32_t, niClassBit_NiMaterialProperty, 0x020E2D0C);

// Gamebryo objects, what's used of them
struct NiObject {
    re::Ptr32<re::Ptr32<void>> vtable;
};
using re::At;
// NiObjectNET::m_kName
re::Ptr32<char>& Name(void* object) {
    return At<char*>(object, 8);
}
// NiAVObject's flags (bit 0: app culled)
uint16_t& Flags(void* object) {
    return At<uint16_t>(object, 0xA8);
}
// NiNode's children (NiTArray at +0xC4: the array +0xC8, the size +0xCE)
re::Ptr32<void>* Children(void* node) {
    return At<re::Ptr32<void>*>(node, 0xC8);
}
uint16_t ChildCount(void* node) {
    return At<uint16_t>(node, 0xCE);
}
// (inlined NiIsKindOf: the class bit in the object's RTTI bit set)
void* IsKindOf(void* object, uint32_t bit) {
    auto rtti = re::PtrCast<const uint8_t*(__thiscall*)(void*)>(static_cast<NiObject*>(object)->vtable[2])(object);
    uint32_t word;
    std::memcpy(&word, rtti + 0xC + (bit >> 5) * 4, 4);
    return (word & (1u << (bit & 31))) ? object : nullptr;
}

// (inlined NiFixedString copy and release)
void AddRef(char* s) {
    if (s != emptyFixedString) {
        Import_InterlockedIncrement(reinterpret_cast<volatile long*>(s - 8));
    }
}
void Release(re::Ptr32<char>* string) {
    char* s = *string;
    if (s == emptyFixedString) {
        return;
    }
    uint16_t length;
    std::memcpy(&length, s - 2, 2);
    if (Import_InterlockedDecrement(reinterpret_cast<volatile long*>(s - 8)) == 1) {
        FixedStringPool_Free(fixedStringPool, string, length);
    }
}

struct DamagePart {
    re::Ptr32<void> dummy;   // +0 the "<part>_dummy" node
    re::Ptr32<void> dam;     // +4 the "_dam" node under it
    re::Ptr32<void> ok;      // +8 the "_ok" node
};
struct VehicleDamageParts {
    re::Ptr32<void> root;              // +0
    DamagePart parts[20];    // +4 (7 bump_front ... 19 windscreen)
};
static_assert(sizeof(VehicleDamageParts) == 0xF4);

GAME_FN(void, __thiscall, Parts_FindOkAndDam, 0x00411790, VehicleDamageParts* self, void* node, int part);
GAME_FN(void, __thiscall, SetChildrenCulled, 0x004118A0, void* self, void* node, int culled);
GAME_FN(void, __cdecl, SetMaterialColour, 0x00411940, void* geometry, const uint8_t* colour1, const uint8_t* colour2);
GAME_FN(void, __cdecl, SetColoursRecursive, 0x00411A50, void* object, const uint8_t* colour1, const uint8_t* colour2);
GAME_FN(void, __thiscall, Parts_FindDummies, 0x00411B20, VehicleDamageParts* self, void* object);

// A list of the vehicles' model instances (doubly linked, the sentinel at +8; each
// node's +0x2C: its parts)
struct PartsListNode {
    re::Ptr32<PartsListNode> next;          // +0
    uint8_t unknown4[0x28];
    re::Ptr32<VehicleDamageParts> parts;    // +0x2C
};
static_assert(offsetof(PartsListNode, parts) == 0x2C);

// 0x4116D0 (thiscall): the parts cleared (the root kept)
void __fastcall ClearParts(VehicleDamageParts* self, re::Edx) {
    for (int i = 0; i < 20; i++) {
        self->parts[i].dummy = nullptr;
        self->parts[i].dam = nullptr;
        self->parts[i].ok = nullptr;
    }
}

// 0x411700: one part's dummy of every model in the list gets the transform (the
// rotation, translation and scale each only when asked)
void __cdecl SetPartTransform(uint8_t* list, int part, const float* transform, int setRotation,
                              int setTranslation, int setScale) {
    auto sentinel = reinterpret_cast<PartsListNode*>(list + 8);
    for (PartsListNode* node = sentinel->next; node != sentinel; node = node->next) {
        if (!node->parts) {
            continue;
        }
        void* dummy = node->parts->parts[part].dummy;
        if (!dummy) {
            continue;
        }
        if (re::Flag(setRotation)) {
            std::memcpy(&At<float>(dummy, 0x2C), transform, 9 * 4);
        }
        if (re::Flag(setTranslation)) {
            At<float>(dummy, 0x50) = transform[9];
            At<float>(dummy, 0x54) = transform[10];
            At<float>(dummy, 0x58) = transform[11];
        }
        if (re::Flag(setScale)) {
            At<float>(dummy, 0x5C) = transform[12] < 0.0f ? -transform[12] : transform[12];
        }
    }
}

// 0x411790 (thiscall): the "_ok" and "_dam" nodes under a part's dummy
void __fastcall FindOkAndDam(VehicleDamageParts* self, re::Edx, void* node, int part) {
    AddRef(Name(node));
    re::Ptr32<char> name = Name(node);
    if (FixedString_Equals(&name, "_ok")) {
        self->parts[part].ok = node;
    } else if (FixedString_Equals(&name, "_dam")) {
        self->parts[part].dam = node;
    } else {
        for (uint32_t i = 0; i < ChildCount(node); i++) {
            void* child = Children(node)[i];
            if (child && IsKindOf(child, niClassBit_NiNode)) {
                Parts_FindOkAndDam(self, child, part);
            }
        }
    }
    Release(&name);
}

// 0x4118A0 (thiscall, `this` only passed on): every node under `node` (not itself) app-culled or shown
void __fastcall SetChildrenCulledImpl(void* self, re::Edx, void* node, int culled) {
    for (uint32_t i = 0; i < ChildCount(node); i++) {
        void* child = Children(node)[i];
        if (!child) {
            continue;
        }
        if (static_cast<uint8_t>(culled) == 0) {
            Flags(child) &= 0xFFFE;
        } else {
            Flags(child) |= 1;
        }
        if (IsKindOf(child, niClassBit_NiNode)) {
            SetChildrenCulled(self, child, culled);
        }
    }
}

// 0x411940: the paint colour on a geometry's material: "vertexcolor_01" takes the
// first colour, "vertexcolor_02" the second (RGB bytes)
void __cdecl SetMaterialColourImpl(void* geometry, const uint8_t* colour1, const uint8_t* colour2) {
    void* property = NiAVObject_GetProperty(geometry, 3);
    if (!property) {
        return;
    }
    void* material = IsKindOf(property, niClassBit_NiMaterialProperty);
    if (!material) {
        return;
    }
    const uint8_t* colour;
    if (FixedString_Equals(&Name(material), "vertexcolor_01")) {
        colour = colour1;
    } else if (FixedString_Equals(&Name(material), "vertexcolor_02")) {
        colour = colour2;
    } else {
        return;
    }
    float r = static_cast<float>(colour[0] / 255.0);
    float g = static_cast<float>(colour[1] / 255.0);
    float b = static_cast<float>(colour[2] / 255.0);
    At<int32_t>(material, 0x54)++;   // the revision
    At<float>(material, 0x28) = r;
    At<float>(material, 0x2C) = g;
    At<float>(material, 0x30) = b;
}

// 0x411A50: the paint colours on everything under `object` (an object with its own
// list at +0xB8/+0xBE takes those instead)
void __cdecl SetColoursRecursiveImpl(void* object, const uint8_t* colour1, const uint8_t* colour2) {
    if (!object) {
        return;
    }
    uint16_t count = At<uint16_t>(object, 0xBE);
    if (count != 0) {
        for (uint32_t i = 0; i < At<uint16_t>(object, 0xBE); i++) {
            SetMaterialColour(At<re::Ptr32<void>*>(object, 0xB8)[i], colour1, colour2);
        }
        return;
    }
    if (void* geometry = NiDynamicCast(NiGeometry_RTTI, object)) {
        SetMaterialColour(geometry, colour1, colour2);
    }
    if (void* node = NiDynamicCast(NiNode_RTTI, object)) {
        for (uint32_t i = 0; i < ChildCount(node); i++) {
            if (void* child = Children(node)[i]) {
                SetColoursRecursive(child, colour1, colour2);
            }
        }
    }
}

// 0x411B20 (thiscall): the parts' dummies found under `object` (a node)
void __fastcall FindDummies(VehicleDamageParts* self, re::Edx, void* object) {
    if (!object) {
        return;
    }
    void* node = IsKindOf(object, niClassBit_NiNode);
    if (!node) {
        return;
    }
    AddRef(Name(node));
    re::Ptr32<char> name = Name(node);
    if (FixedString_Equals(&name, "wing_lf_dummy")) {
        self->parts[13].dummy = node;
        Parts_FindOkAndDam(self, node, 13);
        Release(&name);   // (inlined here, the others call the destructor)
        return;
    }
    static const struct {
        re::Ptr32<const char> name;
        int part;
    } dummies[] = {
        {"wing_rf_dummy", 9},     {"wing_lr_dummy", 14},   {"wing_rr_dummy", 10},  {"bump_front_dummy", 7},
        {"bump_rear_dummy", 8},   {"door_rr_dummy", 12},   {"door_lr_dummy", 16},  {"door_rf_dummy", 11},
        {"door_lf_dummy", 15},    {"bonnet_dummy", 17},    {"boot_dummy", 18},     {"windscreen_dummy", 19},
    };
    for (const auto& dummy : dummies) {
        if (FixedString_Equals(&name, dummy.name)) {
            self->parts[dummy.part].dummy = node;
            Parts_FindOkAndDam(self, node, dummy.part);
            FixedString_Destruct(&name);
            return;
        }
    }
    for (uint32_t i = 0; i < ChildCount(node); i++) {
        if (void* child = NiNode_GetAt(node, i)) {
            Parts_FindDummies(self, child);
        }
    }
    FixedString_Destruct(&name);
}

// 0x411E60 (thiscall): every part undamaged: "_dam" hidden, "_ok" shown
void __fastcall ResetParts(VehicleDamageParts* self, re::Edx) {
    for (int i = 0; i < 20; i++) {
        if (self->parts[i].dam) {
            SetChildrenCulled(self, self->parts[i].dam, 1);
        }
        if (self->parts[i].ok) {
            SetChildrenCulled(self, self->parts[i].ok, 0);
        }
    }
}

// 0x411EA0: one part damaged on every model in the list
void __cdecl DamagePartAll(uint8_t* list, int part) {
    auto sentinel = reinterpret_cast<PartsListNode*>(list + 8);
    for (PartsListNode* node = sentinel->next; node != sentinel; node = node->next) {
        VehicleDamageParts* parts = node->parts;
        if (!parts) {
            continue;
        }
        if (parts->parts[part].dam) {
            SetChildrenCulled(parts, parts->parts[part].dam, 0);
        }
        if (parts->parts[part].ok) {
            SetChildrenCulled(parts, parts->parts[part].ok, 1);
        }
    }
}

// 0x411F00 and 0x411F30 (thiscall, two identical copies): set up from the model's root;
// returns this
VehicleDamageParts* __fastcall Init(VehicleDamageParts* self, re::Edx, void* root) {
    self->root = root;
    Memset(self->parts, 0, sizeof(self->parts));
    Parts_FindDummies(self, root);
    return self;
}

} // namespace c01vparts

// ---- registration ----------------------------------------------------------------------

#define VPARTS_REVERSED_C01(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01vparts {
VPARTS_REVERSED_C01("VehicleDamageParts::Clear", 0x004116D0, &ClearParts);
VPARTS_REVERSED_C01("VehicleDamageParts::SetPartTransform", 0x00411700, &SetPartTransform);
VPARTS_REVERSED_C01("VehicleDamageParts::FindOkAndDam", 0x00411790, &FindOkAndDam);
VPARTS_REVERSED_C01("VehicleDamageParts::SetChildrenCulled", 0x004118A0, &SetChildrenCulledImpl);
VPARTS_REVERSED_C01("VehicleDamageParts::SetMaterialColour", 0x00411940, &SetMaterialColourImpl);
VPARTS_REVERSED_C01("VehicleDamageParts::SetColoursRecursive", 0x00411A50, &SetColoursRecursiveImpl);
VPARTS_REVERSED_C01("VehicleDamageParts::FindDummies", 0x00411B20, &FindDummies);
VPARTS_REVERSED_C01("VehicleDamageParts::Reset", 0x00411E60, &ResetParts);
VPARTS_REVERSED_C01("VehicleDamageParts::DamagePartAll", 0x00411EA0, &DamagePartAll);
VPARTS_REVERSED_C01("VehicleDamageParts::Init", 0x00411F00, &Init);
VPARTS_REVERSED_C01("VehicleDamageParts::Init (copy)", 0x00411F30, &Init);
} // namespace c01vparts
