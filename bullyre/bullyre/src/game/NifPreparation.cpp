// 0x40A7F0 to 0x40AD10: preparing a loaded NIF for the game's renderer: each geometry
// gets its extra data, alpha / texturing setup, the shader maps and parameters, a skin
// partition and precaching; the geometry of a tree is gathered into the root's list
// (+0xB4, an NiTArray) that the flag setters walk.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_FN(void*, __thiscall, NiAVObject_GetProperty, 0x0075DC10, void* object, int type);
GAME_FN(void, __thiscall, NiAVObject_AttachProperty, 0x004099F0, void* object, void* property);
GAME_FN(void, __thiscall, NiAVObject_Fn75E040, 0x0075E040, void* object);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void*, __thiscall, NiTexturingProperty_Construct, 0x00765DD0, void* self);
GAME_FN(void, __thiscall, TexturingMap_Construct, 0x00765BD0, void* self, void* texture, uint32_t uvSet, uint32_t clamp,
        uint32_t filter, uintptr_t);
GAME_FN(void, __thiscall, TexturingProperty_SetShaderMap, 0x007660D0, void* self, uint32_t index, void* map);
GAME_FN(void, __thiscall, TexturingMap_SetTexture, 0x00829350, void* self, void* texture);
GAME_FN(void*, __cdecl, GetRandomDirectionsTexture, 0x004072D0);
GAME_FN(void*, __thiscall, NiObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<char>* name);
GAME_FN(uint8_t, __thiscall, NiObjectNET_AddExtraData, 0x00764600, void* object, const re::Ptr32<char>* name, void* data);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void*, __thiscall, NiFloatsExtraData_Construct, 0x0076CA70, void* self, uint32_t count, const float* values);
GAME_FN(void, __thiscall, NiFloatsExtraData_SetArray, 0x0076C8B0, void* self, uint32_t count, const float* values);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rttiInfo, void* object);
GAME_FN(uint32_t, __thiscall, NiTArray_Add, 0x004099B0, void* self, void* item);
GAME_FN(void, __thiscall, NiTArray_Compact, 0x00407740, void* self);
GAME_FN(void, __thiscall, NiTArray_UpdateSize, 0x00407C50, void* self);
GAME_FN(void, __cdecl, GatherGeometry, 0x0040A9D0, void* object, void* list);
GAME_FN(void, __cdecl, SetupTreeAlpha, 0x00409F60, void* object, float fade);
GAME_FN(uint8_t, __cdecl, IsSkeletonNode, 0x00406790, void* object);
GAME_FN(uint8_t, __cdecl, HasTagEither, 0x00407200, void* object);
GAME_FN(uint8_t, __cdecl, HasTag528, 0x00407270, void* object);
GAME_FN(uint8_t, __cdecl, HasNameTag, 0x00407050, void* object);
GAME_FN(uint8_t, __cdecl, IsDrawable, 0x00406F40, void* object);
GAME_FN(void, __cdecl, MakeSkinPartition, 0x00408740, void* geometry);
GAME_FN(void, __cdecl, AddFadeExtraData, 0x00406DF0, void* object);
GAME_FN(void, __cdecl, RecordAlphaState, 0x0040A260, void* object);
GAME_FN(void, __cdecl, ClearDepthBias, 0x00406FB0, void* object);
GAME_FN(void, __cdecl, SetDepthBias, 0x004070C0, void* object, int force, int doubled);
GAME_FN(void, __cdecl, AddFloatsExtraData, 0x00407160, void* object);
GAME_FN(void, __cdecl, EnsureBaseMap, 0x0040A740, void* geometry);
GAME_FN(void, __cdecl, SetupShaderMaps, 0x0040A7F0, void* geometry);
GAME_FN(void, __cdecl, PrecacheGeometry, 0x004085B0, void* geometry);
GAME_FN(void, __cdecl, PrepareTree, 0x0040AAF0, void* object, int, int, int, int, int, void* list);
GAME_FN(void, __cdecl, BuildTreeBoneMaps, 0x00408680, void* object, void* pose);
GAME_FN(void, __thiscall, Fn_40EB20, 0x0040EB20, void* self);
GAME_FN(uint8_t*, __cdecl, FindByName, 0x00407C90, void* object, const char* name, int exact);
GAME_FN(void, __thiscall, NiPointer_Release, 0x007549D0, void* pointer);
GAME_FN(void*, __thiscall, NiNode_GetAt, 0x00401D30, void* node, uint32_t index);
GAME_FN(uint8_t, __thiscall, FixedString_Equals, 0x00406480, const re::Ptr32<char>* self, const char* s);
GAME_FN(void, __cdecl, CountBoneNodes, 0x004090B0, void* node, int32_t* counter);
GAME_FN(void, __cdecl, BuildBoneTable, 0x004091A0, void* node, int isLast, void* table, int32_t* counter);
GAME_FN(void*, __cdecl, BuildBone, 0x00408E90, void* node, void* root, void* previous, int mode, void* table,
        int32_t* counter, void* parent);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, CMemoryMgr_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void*, __thiscall, BoneTable_Construct, 0x006C7C60, void* self, int32_t count, uintptr_t, uintptr_t, int, int);
GAME_FN(uint8_t*, __cdecl, Clump_Create, 0x006C7540);
GAME_FN(void*, __cdecl, NewBone, 0x006CA210);
GAME_FN(void, __cdecl, Clump_SetRootBone, 0x006C7410, void* clump, void* bone);
GAME_FN(uint8_t*, __cdecl, Atomic_Create, 0x006C6F50);
GAME_FN(void, __cdecl, Atomic_SetObject, 0x006C6C60, void* atomic, void* object, int);   // (the object by NiPointer value)
GAME_FN(void, __cdecl, Clump_AddAtomic, 0x006C74A0, void* clump, void* atomic);
GAME_FN(void, __cdecl, Atomic_SetBone, 0x006C6A10, void* atomic, void* bone);
GAME_FN(void, __cdecl, Bone_AddChild, 0x006C9D80, void* parent, void* child);
GAME_FN(void, __cdecl, Bone_SetTable, 0x006C76D0, void* bone, void* table);
GAME_FN(void, __cdecl, PrepareModel_, 0x0040AD10, void* object, void* pose, int flag400, int flag800);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignCString, 0x00406730, re::Ptr32<char>* self, const char* s);
GAME_FN(void, __thiscall, NiObjectNET_SetName, 0x0077C3A0, void* self, re::Ptr32<char>* name);
GAME_FN(void*, __thiscall, Fn_411F30, 0x00411F30, void* self, void* object);
GAME_FN(uint8_t, __cdecl, ResetFirstPlacedTransform, 0x004093F0, void* object);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, ms_bResetBounds_20C5BE6, 0x020C5BE6);
GAME_VAR(uint32_t, ms_NiNodeBit, 0x020E2550);
inline re::Ptr32<char>* const ms_floatsName_BCC518 = reinterpret_cast<re::Ptr32<char>*>(0x00BCC518);
GAME_VAR(uint32_t, ms_NiTexturingPropertyBit, 0x020E28C4);
GAME_VAR(uint32_t, ms_NiFloatsExtraDataBit, 0x020E2AC4);
inline void* const ms_NiGeometryRTTI = reinterpret_cast<void*>(0x020E25B4);
inline void* const ms_NiNodeRTTI = reinterpret_cast<void*>(0x020E2548);
constexpr uint32_t kShaderMapVtable = 0x008FFA78;

uint8_t* IsKindOfBit(void* object, uint32_t bit) {
    if (!object) {
        return nullptr;
    }
    uint8_t* rtti = vc::Call<uint8_t*>(object, 0x08);
    uint32_t bits = *reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4);
    return (bits & (1u << (bit & 0x1F))) ? static_cast<uint8_t*>(object) : nullptr;
}

uint16_t& W(void* p, int offset) {
    return *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(p) + offset);
}

uint8_t* ChildAt(uint8_t* node, uint32_t i) {
    return i < W(node, 0xCE) ? (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(node + 0xC8))[i] : nullptr;
}

void* NewShaderMap(uint32_t uvSet) {
    uint8_t* map = static_cast<uint8_t*>(NiNew(0x14));
    if (!map) {
        return nullptr;
    }
    TexturingMap_Construct(map, GetRandomDirectionsTexture(), uvSet, 3, 1, 0);
    *reinterpret_cast<uint32_t*>(map) = kShaderMapVtable;
    *reinterpret_cast<uint32_t*>(map + 0x10) = 0;
    return map;
}

} // namespace

namespace NifPreparation {

// 0x40A7F0: the random directions texture as a shader map (the third when there are
// three and it exists, else set as shader map 2, or 0 when there are none), and the
// floats extra data (0xBCC518) set to 0.5, 100, 0, 1
static void __cdecl SetupShaderMaps_(void* geometry) {
    uint8_t* texturing = IsKindOfBit(NiAVObject_GetProperty(geometry, 8), ms_NiTexturingPropertyBit);
    if (!texturing) {
        void* p = NiNew(0x34);
        texturing = static_cast<uint8_t*>(p ? NiTexturingProperty_Construct(p) : nullptr);
        NiAVObject_AttachProperty(geometry, texturing);
    }
    uint8_t* maps = *reinterpret_cast<re::Ptr32<uint8_t>*>(texturing + 0x30);
    if (maps && W(maps, 0xA) != 0 && (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(maps + 4))[0] != nullptr) {
        void* third = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(maps + 4))[2];
        if (W(maps, 0xA) > 2 && third) {
            TexturingMap_SetTexture(third, GetRandomDirectionsTexture());
        } else {
            TexturingProperty_SetShaderMap(texturing, 2, NewShaderMap(2));
        }
    } else {
        TexturingProperty_SetShaderMap(texturing, 0, NewShaderMap(0));
    }
    uint8_t* floats = IsKindOfBit(NiObjectNET_GetExtraData(geometry, ms_floatsName_BCC518), ms_NiFloatsExtraDataBit);
    float values[4] = {0.5f, 100.0f, 0.0f, 1.0f};
    if (floats) {
        NiFloatsExtraData_SetArray(floats, 4, values);
        return;
    }
    void* p = NiNew(0x14);
    uint8_t* data = static_cast<uint8_t*>(p ? NiFloatsExtraData_Construct(p, 4, values) : nullptr);
    FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(reinterpret_cast<uintptr_t>(data) + 8), ms_floatsName_BCC518);
    NiObjectNET_AddExtraData(geometry, ms_floatsName_BCC518, data);
}

// 0x40A9D0: a tree's geometry added to `list` (each object's 0x75E040 on the way)
static void __cdecl GatherGeometry_(uint8_t* object, void* list) {
    if (!object || (object[0xA8] & 1)) {
        return;
    }
    NiAVObject_Fn75E040(object);
    uint8_t* geometry = IsKindOf(ms_NiGeometryRTTI, object);
    uint8_t* node = IsKindOf(ms_NiNodeRTTI, object);
    if (geometry) {
        void* item = geometry;
        NiTArray_Add(list, &item);
        return;
    }
    if (!node) {
        return;
    }
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        GatherGeometry(ChildAt(node, i), list);
    }
}

// 0x40AA70: the geometry list at +0xB4 filled and compacted
static void __cdecl GatherTreeGeometry(uint8_t* object) {
    if (!object || (object[0xA8] & 1)) {
        return;
    }
    GatherGeometry(object, object + 0xB4);
    NiTArray_Compact(object + 0xB4);
}

// 0x40AAA0: the alpha set up for the object's fade (+0xAC, when flag 0x2000 is set; 1
// otherwise)
static void __cdecl SetupFade(uint8_t* object) {
    if (!object || (object[0xA8] & 1)) {
        return;
    }
    if ((W(object, 0xA8) >> 13) & 1) {
        SetupTreeAlpha(object, *reinterpret_cast<float*>(object + 0xAC));
        return;
    }
    SetupTreeAlpha(object, 1.0f);
}

// 0x40AAF0: a tree prepared: skeleton nodes hidden; flags 0x400 and 0x800 from the
// arguments, 0x200 and 0x4000 from the arguments or the tags; each geometry set up and
// added to `list`; children that aren't drawable detached
static void __cdecl PrepareTree_(uint8_t* object, int flag400, int flag800, int depthBias, int tagEither, int tag528,
                                 void* list) {
    if (!object || (object[0xA8] & 1)) {
        return;
    }
    if (IsSkeletonNode(object)) {
        W(object, 0xA8) |= 1;
        return;
    }
    uint16_t& flags = W(object, 0xA8);
    flags = re::Flag(flag400) ? uint16_t(flags | 0x400) : uint16_t(flags & 0xFBFF);
    flags = re::Flag(flag800) ? uint16_t(flags | 0x800) : uint16_t(flags & 0xF7FF);
    uint8_t either;
    if (!re::Flag(tagEither) && !HasTagEither(object)) {
        either = 0;
    } else {
        flags |= 0x200;
        either = 1;
    }
    uint8_t other;
    if (!re::Flag(tag528) && !HasTag528(object)) {
        other = 0;
    } else {
        flags |= 0x4000;
        other = 1;
    }
    uint8_t* geometry = IsKindOf(ms_NiGeometryRTTI, object);
    uint8_t* node = IsKindOf(ms_NiNodeRTTI, object);
    if (geometry) {
        NiAVObject_Fn75E040(geometry);
        void* item = geometry;
        NiTArray_Add(list, &item);
        MakeSkinPartition(geometry);
        AddFadeExtraData(geometry);
        RecordAlphaState(geometry);
        ClearDepthBias(geometry);
        SetDepthBias(geometry, depthBias, flag800);
        AddFloatsExtraData(geometry);
        EnsureBaseMap(geometry);
        SetupShaderMaps(geometry);
        PrecacheGeometry(geometry);
        return;
    }
    if (!node) {
        return;
    }
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        uint8_t* child = ChildAt(node, i);
        if (IsDrawable(child)) {
            int bias = (re::Flag(depthBias) || HasNameTag(child)) ? 1 : 0;
            PrepareTree(child, flag400, flag800, bias, either, other, list);
        } else {
            uint8_t* detached = nullptr;   // an NiPointer, released
            vc::Call<void>(node, 0x90, &detached, i);   // DetachChildAt
            if (detached && vc::Decrement(detached + 4) == 0) {
                vc::Call<void>(detached, 0x04);
            }
        }
    }
    Fn_40EB20(node + 0xC4);
    NiTArray_UpdateSize(node + 0xC4);
}

// 0x40AD10: a model's tree prepared: its bone maps (with a pose), then PrepareTree with
// the root's tags, then the geometry list compacted
static void __cdecl PrepareModel(uint8_t* object, void* pose, int flag400, int flag800) {
    if (!object || (object[0xA8] & 1)) {
        return;
    }
    if (pose) {
        BuildTreeBoneMaps(object, pose);
    }
    uint8_t* list = object + 0xB4;
    uint8_t other = HasTag528(object);
    uint8_t either = HasTagEither(object);
    uint8_t tagged = HasNameTag(object);
    PrepareTree(object, flag400, flag800, tagged, either, other, list);
    NiTArray_Compact(list);
}

// a named child detached from its parent (+0x18) by DetachChild (vtable[35], returning
// an NiPointer that's released)
static void DetachNamed(uint8_t* object, const char* name) {
    uint8_t* child = FindByName(object, name, 1);
    if (!child) {
        return;
    }
    void* detached;
    vc::Call<void>(*reinterpret_cast<re::Ptr32<void>*>(child + 0x18), 0x8C, &detached, child);
    NiPointer_Release(&detached);
}

// 0x40AD80: a model's clump from its NIF: the skeleton's root by the model's kind (0:
// "Dummy", without "Root Footsteps"; 1: "DUMMY" or the first child; 2, 5, 7: "Root";
// 3: the given name; 4: "Scene Root"; 6: as 0, or the child holding an "OffsetNode"
// when 0x20C5BE6 is set; 8: "Root" or "Dummy"; others: `modelId` itself; "Scene Root"
// as the fallback, the object when it's not a node), then its bone table and bones, the
// atomic, the preparation, the name, and the model's extras
static uint8_t* __cdecl CreateClump(uint8_t* object, int32_t modelId, uint32_t kind, const char* name) {
    uint8_t* root = nullptr;
    const char* tryName = nullptr;
    switch (kind) {
    case 0:
        root = FindByName(object, "Dummy", 1);
        DetachNamed(object, "Root Footsteps");
        break;
    case 1:
        root = FindByName(object, "DUMMY", 0);
        if (!root) {
            uint32_t count = W(object, 0xCE);
            for (uint32_t i = 0; i < count && !root; i++) {
                root = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(object + 0xC8))[i];
            }
        }
        break;
    case 2:
    case 5:
    case 7:
        tryName = "Root";
        break;
    case 3:
        root = FindByName(object, name, 0);
        break;
    case 4:
        root = FindByName(object, "Scene Root", 1);
        break;
    case 6:
        if (ms_bResetBounds_20C5BE6) {
            root = FindByName(object, "Scene Root", 1);
            uint8_t* scene = IsKindOf(ms_NiNodeRTTI, root);
            if (scene) {
                for (uint32_t i = 0; i < W(scene, 0xCE); i++) {
                    uint8_t* child = IsKindOf(ms_NiNodeRTTI, NiNode_GetAt(scene, i));
                    if (!child) {
                        continue;
                    }
                    for (uint32_t j = 0; j < W(child, 0xCE); j++) {
                        // (a null grandchild is read at +8 all the same)
                        uint8_t* grandchild = ChildAt(child, j);
                        if (FixedString_Equals(reinterpret_cast<const re::Ptr32<char>*>(reinterpret_cast<uintptr_t>(grandchild) + 8),
                                               "OffsetNode")) {
                            root = child;
                            break;
                        }
                    }
                }
            }
        } else {
            root = FindByName(object, "Dummy", 1);
            DetachNamed(object, "Root Footsteps");
        }
        break;
    case 8:
        root = FindByName(object, "Root", 0);
        if (!root) {
            tryName = "Dummy";
        }
        break;
    default:
        root = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(uint32_t(modelId)));
        break;
    }
    if (tryName) {
        root = FindByName(object, tryName, 0);
        if (!root) {
            root = FindByName(object, "Scene Root", 1);
        }
    }
    uint8_t* skeleton = object;
    if (root) {
        uint8_t* rtti = vc::Call<uint8_t*>(root, 0x08);
        if (*reinterpret_cast<uint32_t*>(rtti + 0xC + (ms_NiNodeBit >> 5) * 4) & (1u << (ms_NiNodeBit & 0x1F))) {
            skeleton = root;
        }
    }
    int32_t bones = 0;
    CountBoneNodes(skeleton, &bones);
    uint8_t* table = nullptr;
    PushMemoryTag(4);
    void* p = CMemoryMgr_Malloc(0x20);
    if (p) {
        table = static_cast<uint8_t*>(BoneTable_Construct(p, bones, 0, 0, 0x3000, 0x24));
    }
    PopMemoryTag();
    *reinterpret_cast<int32_t*>(table + 0x18) = -1;
    int32_t counter = 0;
    BuildBoneTable(skeleton, 1, *reinterpret_cast<re::Ptr32<void>*>(table + 0xC), &counter);
    uint8_t* clump = Clump_Create();
    *reinterpret_cast<int32_t*>(clump + 0x14) = modelId;
    Clump_SetRootBone(clump, NewBone());
    int32_t counter2 = 0;
    void* rootBone = *reinterpret_cast<re::Ptr32<void>*>(clump + 4);
    BuildBone(skeleton, rootBone, rootBone, 2, *reinterpret_cast<re::Ptr32<void>*>(table + 0xC), &counter2, rootBone);
    uint8_t* atomic = Atomic_Create();
    *reinterpret_cast<int32_t*>(atomic + 0x24) = modelId;
    if (object) {
        vc::Increment(object + 4);   // (the NiPointer passed by value)
    }
    Atomic_SetObject(atomic, object, 0);
    *reinterpret_cast<uint32_t*>(atomic + 0x20) = 0;
    Clump_AddAtomic(clump, atomic);
    void* firstChild = *reinterpret_cast<re::Ptr32<void>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(clump + 4) + 0x84);
    if (firstChild) {
        Atomic_SetBone(atomic, firstChild);
    } else {
        Atomic_SetBone(atomic, NewBone());
        Bone_AddChild(*reinterpret_cast<re::Ptr32<void>*>(clump + 4), *reinterpret_cast<re::Ptr32<void>*>(atomic + 4));
    }
    uint8_t* atomicBone = *reinterpret_cast<re::Ptr32<uint8_t>*>(atomic + 4);
    void* tableBone = atomicBone;
    if (bones > 1 && *reinterpret_cast<re::Ptr32<void>*>(atomicBone + 0x84)) {
        tableBone = *reinterpret_cast<re::Ptr32<void>*>(atomicBone + 0x84);
    }
    Bone_SetTable(tableBone, table);
    PrepareModel_(*reinterpret_cast<re::Ptr32<void>*>(atomic + 0x14), table, 0, 0);
    AVObject_Update(*reinterpret_cast<re::Ptr32<void>*>(atomic + 0x14), 0.0f, 1);
    FixedString_AssignCString(reinterpret_cast<re::Ptr32<char>*>(atomic + 0x18), name);
    NiObjectNET_SetName(*reinterpret_cast<re::Ptr32<void>*>(atomic + 0x14), reinterpret_cast<re::Ptr32<char>*>(atomic + 0x18));
    if (uint32_t(modelId - 0x110) <= 0x1A) {
        void* q = CMemoryMgr_Malloc(0xF4);
        *reinterpret_cast<re::Ptr32<void>*>(atomic + 0x78) = q ? Fn_411F30(q, *reinterpret_cast<re::Ptr32<void>*>(atomic + 0x14)) : nullptr;
    }
    if (!clump[0x34]) {
        ResetFirstPlacedTransform(*reinterpret_cast<re::Ptr32<void>*>(atomic + 0x14));
    }
    if (modelId != 0x1B647 && ms_modelInfoPtrs[modelId]) {
        *reinterpret_cast<int32_t*>(atomic + 0x7C) = *reinterpret_cast<int16_t*>(ms_modelInfoPtrs[modelId] + 0x16);
    }
    return clump;
}

} // namespace NifPreparation

// ---- registration ----------------------------------------------------------------------

#define C00_NIF_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace NifPreparation {
C00_NIF_REVERSED("NifPreparation::SetupShaderMaps", SetupShaderMaps_, 0x0040A7F0);
C00_NIF_REVERSED("NifPreparation::GatherGeometry", GatherGeometry_, 0x0040A9D0);
C00_NIF_REVERSED("NifPreparation::GatherTreeGeometry", GatherTreeGeometry, 0x0040AA70);
C00_NIF_REVERSED("NifPreparation::SetupFade", SetupFade, 0x0040AAA0);
C00_NIF_REVERSED("NifPreparation::PrepareTree", PrepareTree_, 0x0040AAF0);
C00_NIF_REVERSED("NifPreparation::PrepareModel", PrepareModel, 0x0040AD10);
C00_NIF_REVERSED("NifPreparation::CreateClump", CreateClump, 0x0040AD80);
} // namespace NifPreparation
