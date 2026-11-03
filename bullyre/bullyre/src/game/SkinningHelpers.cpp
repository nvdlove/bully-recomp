// 0x407C90 to 0x4094E0: walking NIF scene graphs: finding objects by name, the skinned
// geometry's bone matrices (the bone map extra data, the 3x4 / 4x4 palettes), setting
// flags, colours and floats on every geometry of a tree, building bone hierarchies, and
// two renderer helpers.

#include <cmath>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(uint8_t, __thiscall, FixedString_Equals, 0x00406480, const re::Ptr32<char>* self, const char* s);
GAME_FN(uint8_t, __thiscall, FixedString_EqualsNoCase, 0x004064E0, const re::Ptr32<char>* self, const char* s);
GAME_FN(int, __cdecl, stricmp_, 0x00753820, const char* a, const char* b);
inline void* __cdecl memset_(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rttiInfo, void* object);
GAME_FN(void*, __thiscall, NiObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<char>* name);
GAME_FN(uint8_t, __thiscall, NiObjectNET_AddExtraData, 0x00764600, void* object, const re::Ptr32<char>* name, void* data);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void*, __cdecl, NiMalloc, 0x00752300, size_t size);
GAME_FN(void*, __thiscall, NiIntegersExtraData_Construct, 0x0076DA50, void* self, uint32_t count, const int32_t* values);
GAME_FN(void*, __thiscall, Geometry_GetShader, 0x00761630, void* geometry);
GAME_FN(float*, __thiscall, NiTransform_Multiply, 0x00406830, const void* self, void* out, const void* other);
GAME_FN(void, __cdecl, NiTransform_To4x4, 0x008B60A0, void* out, const void* transform);
GAME_FN(uint32_t, __thiscall, Renderer_GetFrameId, 0x00887220, void* self);
GAME_FN(uint8_t, __cdecl, IsSkeletonNode, 0x00406790, void* object);
GAME_FN(void*, __cdecl, BuildBoneMap, 0x00407E10, void* geometry, void* skin, void* skinData, void* pose);
GAME_FN(void, __cdecl, UpdateBoneMatrices, 0x00408060, void* geometry, void* pose);
GAME_FN(void*, __cdecl, Fn_5EDFF0, 0x005EDFF0, void* object);
GAME_FN(uint8_t*, __cdecl, Fn_6C7700, 0x006C7700, void* object);
GAME_FN(void, __thiscall, Renderer_Fn87C3E0, 0x0087C3E0, void* renderer);
GAME_FN(void*, __thiscall, NiSkinPartition_Construct, 0x0076E360, void* self);
GAME_FN(uint8_t, __thiscall, NiSkinPartition_Make, 0x00770500, void* self, void* data, void* skinData, int, int, int);
GAME_FN(void, __thiscall, NiSkinInstance_SetPartition, 0x007643C0, void* self, void* partition);
GAME_FN(void, __cdecl, SetColourExtraData, 0x00406BB0, void* object, uint32_t rgba);
GAME_FN(void, __cdecl, SetColour, 0x00406CF0, void* object, float r, float g, float b, float a);
GAME_FN(void, __thiscall, SetFlag800, 0x004065D0, void* object, int on);
GAME_FN(void, __cdecl, SetFloatsExtraData, 0x00407340, void* object, const float* values);
GAME_FN(void*, __cdecl, NewBone, 0x006CA210);
GAME_FN(void, __cdecl, Fn_5EF7B0, 0x005EF7B0, void* out, char* name);
GAME_FN(void, __thiscall, NiTransform_Construct, 0x00770920, void* self);
GAME_FN(uint8_t, __thiscall, Renderer_BeginUsingDefaultRenderTargetGroup, 0x00407940, void* self, uint32_t flags);
GAME_FN(uint8_t, __thiscall, Renderer_BeginUsingRenderTargetGroup, 0x00407890, void* self, void* group, uint32_t flags);

GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
GAME_VAR(re::Ptr32<char>*, ms_pBoneMapName, 0x009D3BAC);   // a pointer to an NiFixedString
GAME_VAR(uint32_t, ms_NiNodeBit, 0x020E2550);
GAME_VAR(uint32_t, ms_bit_20E25BC, 0x020E25BC);
inline void* const ms_NiNodeRTTI = reinterpret_cast<void*>(0x020E2548);
inline void* const ms_NiGeometryRTTI = reinterpret_cast<void*>(0x020E25B4);
inline void* const ms_BoneMapRTTI = reinterpret_cast<void*>(0x020E2B60);
inline void* const ms_ShaderRTTI_20DCE10 = reinterpret_cast<void*>(0x020DCE10);
inline void* const ms_NiDX9RendererRTTI = reinterpret_cast<void*>(0x020E6D18);
GAME_VAR(uint32_t, ms_NiDX9RendererBit, 0x020E6D20);
inline void* const ms_rtti_20E2BC8 = reinterpret_cast<void*>(0x020E2BC8);

uint32_t& U(void* p, int offset) {
    return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + offset);
}
uint16_t& W(void* p, int offset) {
    return *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(p) + offset);
}
uint8_t* P(void* p, int offset) {
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<uint8_t*>(p) + offset);
}

bool HasBit(void* object, uint32_t bit) {
    uint8_t* rtti = vc::Call<uint8_t*>(object, 0x08);
    return (*reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4) & (1u << (bit & 0x1F))) != 0;
}

// NiNode::GetAt (null past the end)
uint8_t* ChildAt(uint8_t* node, uint32_t i) {
    return i < W(node, 0xCE) ? reinterpret_cast<re::Ptr32<uint8_t>*>(P(node, 0xC8))[i] : nullptr;
}

// a child node that isn't geometry (the bone hierarchy's nodes)
bool IsBoneNode(uint8_t* child) {
    return child && HasBit(child, ms_NiNodeBit) && !HasBit(child, ms_bit_20E25BC);
}

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

void AssignString(re::Ptr32<char>* to, char* s) {
    if (*to == s) {
        return;
    }
    if (s != ms_nullString) {
        vc::Increment(s - 8);
    }
    ReleaseString(to);
    *to = s;
}

void SetWorldTransform(uint8_t* object, const void* transform) {
    const uint32_t* s = static_cast<const uint32_t*>(transform);
    for (int i = 0; i < 13; i++) {
        U(object, 0x2C + i * 4) = s[i];
    }
    float& scale = *reinterpret_cast<float*>(object + 0x5C);
    scale = std::fabs(scale);
}

// the walk the flag setters share: an object not hidden: its cached geometry list (+0xB8,
// count +0xBE) if it has one, else itself as geometry, else its children as a node
template <typename Geometry, typename Recurse>
void Walk(uint8_t* object, Geometry geometry, Recurse recurse) {
    if (!object || (object[0xA8] & 1)) {
        return;
    }
    if (W(object, 0xBE) != 0) {
        for (uint32_t i = 0; i < W(object, 0xBE); i++) {
            geometry(reinterpret_cast<re::Ptr32<uint8_t>*>(P(object, 0xB8))[i]);
        }
        return;
    }
    uint8_t* g = IsKindOf(ms_NiGeometryRTTI, object);
    if (g) {
        geometry(g);
        return;
    }
    uint8_t* node = IsKindOf(ms_NiNodeRTTI, object);
    if (!node) {
        return;
    }
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        recurse(ChildAt(node, i));
    }
}

} // namespace

namespace SkinningHelpers {

// 0x407C90: the object or descendant (nodes only) with this name (exact, or ignoring case)
static uint8_t* __cdecl FindByName(uint8_t* object, const char* name, int exact) {
    if (!object || !name) {
        return nullptr;
    }
    if (re::Flag(exact)) {
        if (FixedString_Equals(re::PtrCast<const re::Ptr32<char>*>(object + 8), name)) {
            return object;
        }
    } else {
        re::Ptr32<char> temp = StringTable_AddString(name);
        uint8_t same = FixedString_EqualsNoCase(&temp, *reinterpret_cast<re::Ptr32<char>*>(object + 8));
        FixedString_Destruct(&temp);
        if (same) {
            return object;
        }
    }
    uint8_t* node = IsKindOf(ms_NiNodeRTTI, object);
    if (!node) {
        return nullptr;
    }
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        auto find = reinterpret_cast<uint8_t*(__cdecl*)(uint8_t*, const char*, int)>(0x00407C90);
        uint8_t* found = find(ChildAt(node, i), name, exact);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

// 0x407D60: a skinned geometry (+0xD0) in the tree
static uint8_t __cdecl HasSkin(uint8_t* object) {
    if (!object) {
        return 0;
    }
    if (HasBit(object, ms_bit_20E25BC) && U(object, 0xD0) != 0) {
        return 1;
    }
    if (!HasBit(object, ms_NiNodeBit)) {
        return 0;
    }
    for (uint32_t i = 0; i < W(object, 0xCE); i++) {
        uint8_t* child = reinterpret_cast<re::Ptr32<uint8_t>*>(P(object, 0xC8))[i];
        if (child && reinterpret_cast<uint8_t(__cdecl*)(uint8_t*)>(0x00407D60)(child)) {
            return 1;
        }
    }
    return 0;
}

// 0x407E10: the bone map: for each of the skin's bones (+0x14, count +0x44 of the skin
// data) the index of the pose's bone (+0xC table of 0x14, its name at +0x10) of the same
// name (ignoring case; its own index when none), kept as integers extra data on the
// geometry. (128 bones at most: the game's stack array)
static void* __cdecl BuildBoneMap_(void* geometry, uint8_t* skin, uint8_t* skinData, uint8_t* pose) {
    uint32_t count = U(skinData, 0x44);
    re::Ptr32<uint8_t>* bones = reinterpret_cast<re::Ptr32<uint8_t>*>(P(skin, 0x14));
    int32_t map[128];
    memset_(map, 0, count * 4);
    for (uint32_t i = 0; i < count; i++) {
        map[i] = int32_t(i);
        re::Ptr32<char> boneName = *reinterpret_cast<re::Ptr32<char>*>(bones[i] + 8);   // (held while looking)
        if (boneName != ms_nullString) {
            vc::Increment(boneName - 8);
        }
        for (int32_t j = 0; j < *reinterpret_cast<int32_t*>(pose + 4); j++) {
            re::Ptr32<char> entry = *reinterpret_cast<re::Ptr32<char>*>(P(pose, 0xC) + j * 0x14 + 0x10);
            if (entry != ms_nullString) {
                vc::Increment(entry - 8);
            }
            const char* a = *reinterpret_cast<re::Ptr32<char>*>(bones[i] + 8);
            const char* b = *reinterpret_cast<re::Ptr32<char>*>(P(pose, 0xC) + j * 0x14 + 0x10);
            bool same = a == b || (b && a && stricmp_(a, b) == 0);
            ReleaseString(&entry);
            if (same) {
                map[i] = j;
                break;
            }
        }
        ReleaseString(&boneName);
    }
    void* p = NiNew(0x14);
    uint8_t* data = static_cast<uint8_t*>(p ? NiIntegersExtraData_Construct(p, count, map) : nullptr);
    AssignString(reinterpret_cast<re::Ptr32<char>*>(data + 8), *ms_pBoneMapName);
    NiObjectNET_AddExtraData(geometry, ms_pBoneMapName, data);
    return data;
}

// 0x408060: a skinned geometry's bone matrices from the pose (+8: 0x34 byte transforms):
// each bone's pose transform times its skin-to-bone transform, as 3x4 or 4x4 matrices
// (the shader's register count per bone) into the skin instance's buffer (+0x28, size
// +0x24; the old one isn't freed when it grows), and the frame they were made in
static void __cdecl UpdateBoneMatrices_(uint8_t* geometry, uint8_t* pose) {
    if (!geometry || !pose) {
        return;
    }
    uint8_t* skin = P(geometry, 0xD0);
    if (!skin) {
        return;
    }
    uint8_t* skinData = P(skin, 8);
    if (!skinData) {
        return;
    }
    uint8_t* boneData = P(skinData, 0x40);   // 0x4C each, its transform first
    uint8_t* map = IsKindOf(ms_BoneMapRTTI, NiObjectNET_GetExtraData(geometry, ms_pBoneMapName));
    if (!map) {
        map = static_cast<uint8_t*>(BuildBoneMap(geometry, skin, skinData, pose));
        if (!map) {
            return;
        }
    }
    uint32_t count = U(map, 0xC);
    int32_t* indices = reinterpret_cast<int32_t*>(P(map, 0x10));
    if (!count || !indices) {
        return;
    }
    int32_t registers = *reinterpret_cast<int32_t*>(skin + 0x20);
    if (U(skin, 0x1C) < 1 || registers < 1) {
        uint8_t* shader = IsKindOf(ms_ShaderRTTI_20DCE10, Geometry_GetShader(geometry));
        registers = *reinterpret_cast<int32_t*>(shader + 0x6C);
        U(skin, 0x1C) = count;
        *reinterpret_cast<int32_t*>(skin + 0x20) = registers;
    }
    uint32_t size = uint32_t(registers) * count * 0x10;
    if (U(skin, 0x24) < size) {
        U(skin, 0x28) = reinterpret_cast<uintptr_t>(NiMalloc(size));
        U(skin, 0x24) = size;
    }
    uint8_t* out = P(skin, 0x28);
    float* out3x4 = reinterpret_cast<float*>(out);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t a[13];
        uint32_t b[13];
        const uint32_t* from = reinterpret_cast<const uint32_t*>(P(pose, 8) + indices[i] * 0x34);
        for (int k = 0; k < 13; k++) {
            a[k] = from[k];
        }
        from = reinterpret_cast<const uint32_t*>(boneData);
        for (int k = 0; k < 13; k++) {
            b[k] = from[k];
        }
        float r[13];   // rotation (9), translation (3), scale
        NiTransform_Multiply(a, r, b);
        int32_t mode = *reinterpret_cast<int32_t*>(skin + 0x20);
        if (mode == 4) {
            NiTransform_To4x4(out, r);
        } else if (mode == 3) {
            float s = r[12];
            out3x4[0] = r[0] * s;
            out3x4[4] = r[3] * s;
            out3x4[8] = r[6] * s;
            out3x4[1] = r[1] * s;
            out3x4[5] = r[4] * s;
            out3x4[9] = r[7] * s;
            out3x4[2] = r[2] * s;
            out3x4[6] = r[5] * s;
            out3x4[10] = s * r[8];
            out3x4[3] = r[9];
            out3x4[7] = r[10];
            out3x4[11] = r[11];
        }
        boneData += 0x4C;
        out += registers * 0x10;
        out3x4 += 12;
    }
    uint8_t* renderer = IsKindOf(ms_NiDX9RendererRTTI, ms_pRenderer);
    U(skin, 0x18) = Renderer_GetFrameId(P(renderer, 0x6B8));
}

// 0x4082A0: UpdateBoneMatrices over a tree
static void __cdecl UpdateTreeBoneMatrices(uint8_t* object, uint8_t* pose) {
    if (!object || !pose) {
        return;
    }
    Walk(
        object, [&](uint8_t* g) { UpdateBoneMatrices(g, pose); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*, uint8_t*)>(0x004082A0)(child, pose); });
}

// 0x408380: a model's attached objects (the list at +8, nodes at +0x38 of theirs) posed
// from the model's pose: their bone matrices, and their world transform the pose's
// root. (A hidden object stops the walk: the game loops on it for ever)
static void __cdecl UpdateAttachedBoneMatrices(uint8_t* model) {
    if (!model) {
        return;
    }
    void* instance = *reinterpret_cast<re::Ptr32<void>*>(static_cast<uint8_t*>(Fn_5EDFF0(model)) + 4);
    if (!instance) {
        return;
    }
    uint8_t* pose = Fn_6C7700(instance);
    if (!pose) {
        return;
    }
    uint8_t* head = model + 8;
    uint8_t* node = *reinterpret_cast<re::Ptr32<uint8_t>*>(head);
    while (node != head) {
        uint8_t* object = P(node, -0x38);
        if (object[0xA8] & 1) {
            continue;
        }
        reinterpret_cast<void(__cdecl*)(uint8_t*, uint8_t*)>(0x004082A0)(object, pose);
        SetWorldTransform(P(node, -0x38), P(pose, 8));
        node = *reinterpret_cast<re::Ptr32<uint8_t>*>(node);
    }
}

// 0x408410: the geometry of a tree given to the renderer (vtable[34])
static void __cdecl RenderTree(uint8_t* object) {
    Walk(
        object, [&](uint8_t* g) { vc::Call<void>(g, 0x88, ms_pRenderer); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*)>(0x00408410)(child); });
}

// 0x4084E0: a node's children rendered, those flagged +0xB0 & 0x10 skipped except the
// last of them, which goes last
static void __cdecl RenderChildrenLastFlagged(uint8_t* object) {
    if (!object || (object[0xA8] & 1)) {
        return;
    }
    IsKindOf(ms_NiGeometryRTTI, object);   // (unused)
    uint8_t* node = IsKindOf(ms_NiNodeRTTI, object);
    if (!node) {
        return;
    }
    auto render = reinterpret_cast<void(__cdecl*)(uint8_t*)>(0x00408410);
    uint8_t* last = nullptr;
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        uint8_t* child = reinterpret_cast<re::Ptr32<uint8_t>*>(P(node, 0xC8))[i];
        if (child && HasBit(child, ms_bit_20E25BC) && (child[0xB0] & 0x10)) {
            last = child;
            continue;
        }
        render(child);
    }
    if (last) {
        render(last);
    }
}

// 0x4085B0: a geometry's data made static (+0x34 consistency 0x4000, +0x36 0, +0x37 9)
// and precached by the renderer (vtable[46]; 0x3C, 4 when skinned with a partition)
static void __cdecl PrecacheGeometry(uint8_t* geometry) {
    uint8_t* data = P(geometry, 0xCC);
    W(data, 0x34) = uint16_t((W(data, 0x34) & 0xFFF) | 0x4000);
    data[0x36] = 0;
    data[0x37] = 9;
    uint8_t* skin = P(geometry, 0xD0);
    if (skin && U(skin, 0xC) != 0) {
        if (vc::Call<uint8_t>(ms_pRenderer, 0xB8, geometry, 0x3C, 4)) {
            Renderer_Fn87C3E0(IsKindOf(ms_NiDX9RendererRTTI, ms_pRenderer));
        }
        return;
    }
    if (!vc::Call<uint8_t>(ms_pRenderer, 0xB8, geometry, 0, 0)) {
        return;
    }
    uint8_t* renderer = ms_pRenderer;
    Renderer_Fn87C3E0(renderer && HasBit(renderer, ms_NiDX9RendererBit) ? renderer : nullptr);
}

// 0x408680: bone maps built for a tree's skinned geometry (skeleton nodes left out)
static void __cdecl BuildTreeBoneMaps(uint8_t* object, uint8_t* pose) {
    if (!object || (object[0xA8] & 1) || IsSkeletonNode(object)) {
        return;
    }
    uint8_t* geometry = IsKindOf(ms_NiGeometryRTTI, object);
    uint8_t* node = IsKindOf(ms_NiNodeRTTI, object);
    if (geometry) {
        uint8_t* skin = P(geometry, 0xD0);
        if (skin && P(skin, 8)) {
            BuildBoneMap(geometry, skin, P(skin, 8), pose);
        }
        return;
    }
    if (!node) {
        return;
    }
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        reinterpret_cast<void(__cdecl*)(uint8_t*, uint8_t*)>(0x00408680)(ChildAt(node, i), pose);
    }
}

// 0x408740: a skin partition made for skinned geometry without one (4 bones a partition)
static void __cdecl MakeSkinPartition(uint8_t* geometry) {
    uint8_t* skin = P(geometry, 0xD0);
    if (!skin) {
        return;
    }
    uint8_t* skinData = P(skin, 8);
    if (!skinData || U(skin, 0xC) != 0) {
        return;
    }
    uint8_t* data = IsKindOf(ms_rtti_20E2BC8, P(geometry, 0xCC));
    if (!data) {
        return;
    }
    void* p = NiNew(0x10);
    void* partition = p ? NiSkinPartition_Construct(p) : nullptr;
    if (NiSkinPartition_Make(partition, data, skinData, 0x41, 4, 0)) {
        uint8_t* old = P(skin, 0xC);
        if (old) {
            if (vc::Decrement(old + 4) == 0) {
                vc::Call<void>(old, 0x04);
            }
            U(skin, 0xC) = 0;
        }
        NiSkinInstance_SetPartition(skin, partition);
    }
}

// 0x4087E0: SetColourExtraData over a tree
static void __cdecl SetTreeColour(uint8_t* object, uint32_t rgba) {
    Walk(
        object, [&](uint8_t* g) { SetColourExtraData(g, rgba); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*, uint32_t)>(0x004087E0)(child, rgba); });
}

// 0x4088C0: SetColour over a tree
static void __cdecl SetTreeColourFloats(uint8_t* object, float r, float g, float b, float a) {
    Walk(
        object, [&](uint8_t* geometry) { SetColour(geometry, r, g, b, a); },
        [&](uint8_t* child) {
            reinterpret_cast<void(__cdecl*)(uint8_t*, float, float, float, float)>(0x004088C0)(child, r, g, b, a);
        });
}

// 0x4089F0: flag 0x800 of +0xA8 over a tree
static void __cdecl SetTreeFlag800(uint8_t* object, int on) {
    Walk(
        object, [&](uint8_t* g) { SetFlag800(g, on); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*, int)>(0x004089F0)(child, on); });
}

// 0x408AE0: SetFloatsExtraData over a tree
static void __cdecl SetTreeFloats(uint8_t* object, const float* values) {
    Walk(
        object, [&](uint8_t* g) { SetFloatsExtraData(g, values); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*, const float*)>(0x00408AE0)(child, values); });
}

// 0x408BC0: the geometry's +0xB0 flags cleared but the top four bits, over a tree
static void __cdecl ClearTreeFlags(uint8_t* object) {
    Walk(
        object, [&](uint8_t* g) { U(g, 0xB0) &= 0xF0000000; },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*)>(0x00408BC0)(child); });
}

// 0x408C90: flag 4 of +0xB0 over a tree
static void __cdecl SetTreeFlag4(uint8_t* object, int on) {
    Walk(
        object, [&](uint8_t* g) { U(g, 0xB0) = re::Flag(on) ? (U(g, 0xB0) | 4) : (U(g, 0xB0) & 0xFFFFFFFB); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*, int)>(0x00408C90)(child, on); });
}

// 0x408D90: flag 8 of +0xB0 over a tree
static void __cdecl SetTreeFlag8(uint8_t* object, int on) {
    Walk(
        object, [&](uint8_t* g) { U(g, 0xB0) = re::Flag(on) ? (U(g, 0xB0) | 8) : (U(g, 0xB0) & 0xFFFFFFF7); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*, int)>(0x00408D90)(child, on); });
}

// 0x408E90: a bone for a node (mode 1: a new first child of `parentBone`, 0: a new next
// sibling of `previous`, 2: `parentBone` itself, else `root`): its links, the node's world
// transform (twice), its name, then its child nodes' bones. Returns the bone. Each new
// bone also goes into `table` (0x14 an entry, at +0xC)
static uint8_t* __cdecl BuildBone(uint8_t* node, uint8_t* root, uint8_t* previous, int mode, uint8_t* table,
                                  int32_t* counter, uint8_t* parent) {
    uint8_t* bone;
    if (mode == 1 || mode == 0) {
        bone = static_cast<uint8_t*>(NewBone());
        U(previous, mode == 1 ? 0x84 : 0x88) = reinterpret_cast<uintptr_t>(bone);
        if (table) {
            U(table, *counter * 0x14 + 0xC) = reinterpret_cast<uintptr_t>(bone);
            (*counter)++;
        }
        U(bone, 4) = reinterpret_cast<uintptr_t>(parent);
    } else if (mode == 2) {
        bone = previous;
    } else {
        bone = root;
        U(bone, 4) = reinterpret_cast<uintptr_t>(parent);
    }
    U(bone, 0x88) = 0;   // next sibling
    U(bone, 0x84) = 0;   // first child
    U(bone, 0x8C) = reinterpret_cast<uintptr_t>(root);
    for (int i = 0; i < 13; i++) {
        U(bone, 0x14 + i * 4) = U(node, 0x2C + i * 4);
        U(bone, 0x48 + i * 4) = U(node, 0x2C + i * 4);
    }
    FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(bone + 8), re::PtrCast<const re::Ptr32<char>*>(node + 8));
    U(bone, 0x94) = 0;
    char* name = *reinterpret_cast<re::Ptr32<char>*>(bone + 8);
    if (name != ms_nullString && *reinterpret_cast<uint16_t*>(name - 4) != 0) {
        Fn_5EF7B0(bone + 0x9C, name);
    }
    uint32_t count = W(node, 0xCE);
    bool first = true;
    uint8_t* last = root;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* child = ChildAt(node, i);
        if (!IsBoneNode(child)) {
            continue;
        }
        auto build = reinterpret_cast<uint8_t*(__cdecl*)(uint8_t*, uint8_t*, uint8_t*, int, uint8_t*, int32_t*, uint8_t*)>(
            0x00408E90);
        if (first) {
            first = false;
            last = build(child, root, bone, 1, table, counter, bone);
        } else {
            last = build(child, root, last, 0, table, counter, bone);
        }
    }
    return bone;
}

// 0x4090B0: the bone nodes in a tree, counted
static void __cdecl CountBoneNodes(uint8_t* node, int32_t* counter) {
    (*counter)++;
    uint32_t count = W(node, 0xCE);
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* child = ChildAt(node, i);
        if (IsBoneNode(child)) {
            reinterpret_cast<void(__cdecl*)(uint8_t*, int32_t*)>(0x004090B0)(child, counter);
        }
    }
}

// 0x4091A0: a node's table entry (0x14: index twice, flags (1: no child nodes, 2: not
// the last child), name), then its child nodes'
static void __cdecl BuildBoneTable(uint8_t* node, int isLast, uint8_t* table, int32_t* counter) {
    int32_t childNodes = 0;
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        if (IsBoneNode(reinterpret_cast<re::Ptr32<uint8_t>*>(P(node, 0xC8))[i])) {
            childNodes++;
        }
    }
    uint8_t* entry = table + *counter * 0x14;
    U(entry, 8) = 0;
    if (!re::Flag(isLast)) {
        U(table + *counter * 0x14, 8) |= 2;
    }
    if (childNodes == 0) {
        U(table + *counter * 0x14, 8) |= 1;
    }
    AssignString(reinterpret_cast<re::Ptr32<char>*>(table + *counter * 0x14 + 0x10), *reinterpret_cast<re::Ptr32<char>*>(node + 8));
    U(table + *counter * 0x14, 0) = uint32_t(*counter);
    U(table + *counter * 0x14, 4) = uint32_t(*counter);
    (*counter)++;
    int32_t index = 0;
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        uint8_t* child = reinterpret_cast<re::Ptr32<uint8_t>*>(P(node, 0xC8))[i];
        if (!IsBoneNode(child)) {
            continue;
        }
        reinterpret_cast<void(__cdecl*)(uint8_t*, int, uint8_t*, int32_t*)>(0x004091A0)(child, index == childNodes - 1,
                                                                                        table, counter);
        index++;
    }
}

// 0x4093F0: the first object (depth first) placed away from the origin gets the identity
// world transform; 1 when one was found
static uint8_t __cdecl ResetFirstPlacedTransform(uint8_t* object) {
    const float* t = reinterpret_cast<const float*>(object + 0x50);
    if (t[0] == 0.0f && t[1] == 0.0f && t[2] == 0.0f) {
        uint8_t* node = IsKindOf(ms_NiNodeRTTI, object);
        if (!node) {
            return 0;
        }
        for (uint32_t i = 0; i < W(node, 0xCE); i++) {
            if (reinterpret_cast<uint8_t(__cdecl*)(uint8_t*)>(0x004093F0)(ChildAt(node, i))) {
                return 1;
            }
        }
        return 0;
    }
    uint8_t identity[0x34];
    NiTransform_Construct(identity);
    SetWorldTransform(object, identity);
    return 1;
}

// 0x4094B0: BeginUsingDefaultRenderTargetGroup when in a frame with no group in use
static void __cdecl BeginDefaultTargets(uint32_t flags) {
    uint8_t* r = ms_pRenderer;
    int32_t state = *reinterpret_cast<int32_t*>(r + 0x94);
    if ((state == 1 || state == 2 || state == 4) && !r[0x90]) {
        Renderer_BeginUsingDefaultRenderTargetGroup(r, flags);
    }
}

// 0x4094E0: BeginUsingRenderTargetGroup likewise
static void __cdecl BeginTargets(void* group, uint32_t flags) {
    uint8_t* r = ms_pRenderer;
    int32_t state = *reinterpret_cast<int32_t*>(r + 0x94);
    if ((state == 1 || state == 2 || state == 4) && !r[0x90]) {
        Renderer_BeginUsingRenderTargetGroup(r, group, flags);
    }
}

} // namespace SkinningHelpers

// ---- registration ----------------------------------------------------------------------

#define C00_SKIN_REVERSED(name, fn, address)                                                        \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace SkinningHelpers {
C00_SKIN_REVERSED("SkinningHelpers::FindByName", FindByName, 0x00407C90);
C00_SKIN_REVERSED("SkinningHelpers::HasSkin", HasSkin, 0x00407D60);
C00_SKIN_REVERSED("SkinningHelpers::BuildBoneMap", BuildBoneMap_, 0x00407E10);
C00_SKIN_REVERSED("SkinningHelpers::UpdateBoneMatrices", UpdateBoneMatrices_, 0x00408060);
C00_SKIN_REVERSED("SkinningHelpers::UpdateTreeBoneMatrices", UpdateTreeBoneMatrices, 0x004082A0);
C00_SKIN_REVERSED("SkinningHelpers::UpdateAttachedBoneMatrices", UpdateAttachedBoneMatrices, 0x00408380);
C00_SKIN_REVERSED("SkinningHelpers::RenderTree", RenderTree, 0x00408410);
C00_SKIN_REVERSED("SkinningHelpers::RenderChildrenLastFlagged", RenderChildrenLastFlagged, 0x004084E0);
C00_SKIN_REVERSED("SkinningHelpers::PrecacheGeometry", PrecacheGeometry, 0x004085B0);
C00_SKIN_REVERSED("SkinningHelpers::BuildTreeBoneMaps", BuildTreeBoneMaps, 0x00408680);
C00_SKIN_REVERSED("SkinningHelpers::MakeSkinPartition", MakeSkinPartition, 0x00408740);
C00_SKIN_REVERSED("SkinningHelpers::SetTreeColour", SetTreeColour, 0x004087E0);
C00_SKIN_REVERSED("SkinningHelpers::SetTreeColourFloats", SetTreeColourFloats, 0x004088C0);
C00_SKIN_REVERSED("SkinningHelpers::SetTreeFlag800", SetTreeFlag800, 0x004089F0);
C00_SKIN_REVERSED("SkinningHelpers::SetTreeFloats", SetTreeFloats, 0x00408AE0);
C00_SKIN_REVERSED("SkinningHelpers::ClearTreeFlags", ClearTreeFlags, 0x00408BC0);
C00_SKIN_REVERSED("SkinningHelpers::SetTreeFlag4", SetTreeFlag4, 0x00408C90);
C00_SKIN_REVERSED("SkinningHelpers::SetTreeFlag8", SetTreeFlag8, 0x00408D90);
C00_SKIN_REVERSED("SkinningHelpers::BuildBone", BuildBone, 0x00408E90);
C00_SKIN_REVERSED("SkinningHelpers::CountBoneNodes", CountBoneNodes, 0x004090B0);
C00_SKIN_REVERSED("SkinningHelpers::BuildBoneTable", BuildBoneTable, 0x004091A0);
C00_SKIN_REVERSED("SkinningHelpers::ResetFirstPlacedTransform", ResetFirstPlacedTransform, 0x004093F0);
C00_SKIN_REVERSED("SkinningHelpers::BeginDefaultTargets", BeginDefaultTargets, 0x004094B0);
C00_SKIN_REVERSED("SkinningHelpers::BeginTargets", BeginTargets, 0x004094E0);
} // namespace SkinningHelpers
