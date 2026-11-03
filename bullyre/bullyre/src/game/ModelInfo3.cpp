// The model info kinds: clump (and its frames), simple and time, weapon, accessory, ped,
// and the prop animations' (CPAnimModelInfo, with its pool).

#include "game/ModelInfo.h"

#include <cstring>

#include "game/Crt.h"
#include "reversed.h"
namespace {
GAME_FN(void, __cdecl, MIK_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, MIK_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, MIK_PopMemoryTag, 0x005EF100);
GAME_FN(uint32_t, __cdecl, MIK_HashName, 0x00576ED0, const char* name);
inline int __cdecl MIK_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline char* __cdecl MIK_Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
inline void* __cdecl MIK_Memset(void* p, int value, size_t n) { return crt::Memset(p, value, n); }
GAME_FN(int, __cdecl, MIK_GetAnimFileIndex, 0x0052DAD0, const char* name);
GAME_FN(void, __cdecl, MIK_TxdAddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, MIK_TxdRemoveRef, 0x005F1160, int slot);

// the base's and each other's (where modes apply)
GAME_FN(void, __thiscall, MIK_BaseConstruct, 0x0050E8C0, void* mi, uint8_t type);
GAME_FN(void, __thiscall, MIK_BaseDestruct, 0x0050EE20, void* mi);
GAME_FN(void, __thiscall, MIK_BaseSetName, 0x0050ED40, void* mi, const char* name);
GAME_FN(void, __thiscall, MIK_AddAnimRefs, 0x0050E960, void* mi);
GAME_FN(void, __thiscall, MIK_RemoveAnimRefs, 0x0050EA40, void* mi);
GAME_FN(void, __thiscall, MIK_ClumpConstruct, 0x005109E0, void* mi, uint8_t type);
GAME_FN(void, __thiscall, MIK_ClumpDestruct, 0x00510D90, void* mi);
GAME_FN(void, __thiscall, MIK_ClumpSetClump, 0x00510CB0, void* mi, void* clump, int arg);
GAME_FN(void, __thiscall, MIK_SimpleDestruct, 0x00527630, void* mi);
GAME_FN(void, __cdecl, MIK_SimplePoolDelete, 0x00527660, void* mi);
GAME_FN(void, __cdecl, MIK_TimePoolDelete, 0x00527690, void* mi);
GAME_FN(void, __cdecl, MIK_PAnimPoolDelete, 0x00520D70, void* mi);
GAME_FN(void, __thiscall, MIK_PedSetName, 0x00522220, void* mi, const char* name, int);
GAME_FN(void, __thiscall, MIK_PedDestruct, 0x00522410, void* mi);
GAME_FN(void, __thiscall, MIK_PAnimLoadActData, 0x005205B0, void* mi);
GAME_FN(int16_t, __cdecl, MIK_FindPAnimIndex, 0x00520960, const char* name, CBaseModelInfo* mi);
GAME_FN(int, __cdecl, MIK_GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(void*, __thiscall, MIK_PAnimPoolNew, 0x00520A50, CPool<CPAnimModelInfo>* pool);

// Gamebryo scene graphs (the clump side, not reversed yet)
GAME_FN(void*, __cdecl, Ni_GetRoot, 0x006C7370, void* clump);                      // (its root node)
GAME_FN(void, __cdecl, MIK_SetBoneTable, 0x006C76D0, void* frame, void* table);
GAME_FN(void*, __cdecl, Ni_ForAllChildren, 0x006C9B50, void* node, void* callback, void* data);
GAME_FN(void, __cdecl, Ni_ForAllAtomics, 0x006C7380, void* node, void* callback, void* data);
GAME_FN(void, __cdecl, Ni_DeleteClump, 0x006C75C0, void* clump);
GAME_FN(void*, __cdecl, Ni_CloneClump, 0x006C7630, void* clump);
GAME_FN(void*, __cdecl, Ni_GetMatrix, 0x006C9A90, void* node);
GAME_FN(uint32_t, __cdecl, Ni_GetNameHash, 0x005EF870, void* node);
GAME_FN(uint8_t, __cdecl, Ni_IsSkinned_5EE0B0, 0x005EE0B0, void* clump);
GAME_FN(uint32_t*, __cdecl, Ni_5EE100, 0x005EE100, void* clump);
GAME_FN(void*, __cdecl, Ni_6B1AD0, 0x006B1AD0, void* node);
// the renderer's model loading (thiscall on 0x20C4910)
GAME_FN(void, __thiscall, Renderer_6C2950, 0x006C2950, void* renderer, void* handle, void* clump, int arg);
GAME_FN(int, __thiscall, Renderer_LoadModel, 0x006C2AE0, void* renderer, const char* path, re::Ptr32<void>* handle, uintptr_t);
inline void* const renderer = reinterpret_cast<void*>(0x020C4910);
GAME_FN(int, __thiscall, Skeleton_FindBone, 0x00465580, void* skeleton, uint32_t hash, int32_t* id);

// (callbacks the game passes by address)
constexpr uintptr_t FindFrameFromNameCB = 0x00510A40;
constexpr uintptr_t FindFrameFromIdCB = 0x00510A90;
constexpr uintptr_t FillFrameArrayCB = 0x00510AD0;
constexpr uintptr_t SetClumpAtomicCB = 0x00510C90;
constexpr uintptr_t SetAtomicRendererCB = 0x006CA7D0;
constexpr uintptr_t AccessoryAtomicCB = 0x005F2DD0;

GAME_ARRAY(const char* const, accessoryBoneNames, 0x00A45D2C);   // "RightHand", "LeftHand"
GAME_ARRAY(char, panimPath, 0x00C221A8);                           // 0x80, shared

// the prop animations' act data (butes) and its lookups (as CBaseModelInfo::FindButes)
struct ButesPath {
    int32_t count;
    re::Ptr32<const uint32_t> parts;
};
GAME_FN(void*, __cdecl, Butes_Find, 0x0072AEA0, const ButesPath* path, const uint32_t* root);
GAME_FN(void*, __cdecl, Butes_FindChild, 0x0072BEB0, const ButesPath* path, const uint32_t* child, const uint32_t* root);
GAME_FN(uint32_t*, __thiscall, ButesPart_FromString, 0x005FA910, uint32_t* part, const char* name);
GAME_VAR(const uint32_t, butes_1B653A0, 0x01B653A0);
GAME_VAR(const uint32_t, butes_1B65320, 0x01B65320);
GAME_VAR(const uint32_t, butes_1B6538C, 0x01B6538C);
GAME_VAR(const uint32_t, butes_1B653BC, 0x01B653BC);   // the default act (not px)
GAME_VAR(const uint32_t, butes_1B653C0, 0x01B653C0);   // the default px act
inline const uint32_t* const butesRoot_1B653A4 = reinterpret_cast<const uint32_t*>(0x01B653A4);
inline const uint32_t* const butesRoot_1B653A8 = reinterpret_cast<const uint32_t*>(0x01B653A8);
inline const uint32_t* const butesRoot_1B65390 = reinterpret_cast<const uint32_t*>(0x01B65390);
// an effect id by name (thiscall on *0xB6F1E8)
GAME_FN(int16_t, __thiscall, Effects_FindId, 0x006679A0, void* effects, void* name);
GAME_VAR(void*, effectsManager, 0x00B6F1E8);
GAME_VAR(int32_t, panimKind1, 0x01B654CC);
GAME_VAR(int32_t, panimKind2, 0x01B654D0);
GAME_VAR(int32_t, panimKind3, 0x01B654D4);
GAME_VAR(const float, panimFloat80, 0x009051D8);
GAME_VAR(const float, panimFloat60, 0x00906954);
GAME_VAR(const float, panimFloat20, 0x0090CE58);
GAME_VAR(const float, panimFloat30, 0x00900E68);
// the cat store's index by name (thiscall on 0xC0F7B4)
GAME_FN(uint16_t, __thiscall, CatStore_FindIndex, 0x004507B0, void* catStore, const char* name);
inline void* const catStore = reinterpret_cast<void*>(0x00C0F7B4);

using re::At;

// 0x51A9D0 / 0x51AA40 / 0x51AAB0: the vector deleting destructors (flags 2: an array
// from new[], the count before it, destroyed last to first; 1: freed)
template <typename T>
T* VectorDelete(T* self, uint8_t deleteFlags, void(__thiscall* destruct)(void*), void(__cdecl* poolDelete)(void*)) {
    if (deleteFlags & 2) {
        int32_t* block = reinterpret_cast<int32_t*>(self) - 1;
        for (int i = *block - 1; i >= 0; i--) {
            destruct(self + i);
        }
        if (deleteFlags & 1) {
            MIK_Free(block);
        }
        return reinterpret_cast<T*>(block);
    }
    destruct(self);
    if (deleteFlags & 1) {
        poolDelete(self);
    }
    return self;
}
} // namespace

// ---- clumps --------------------------------------------------------------------------

CClumpModelInfo* CClumpModelInfo::Construct() {
    MIK_BaseConstruct(this, MODEL_INFO_CLUMP);
    vtable = CModelInfo::ms_clumpVtable;
    animFileIndex = -1;
    CModelInfo::ms_numClumpModelInfos++;
    return this;
}

CClumpModelInfo* CClumpModelInfo::Construct(uint8_t modelType) {
    MIK_BaseConstruct(this, modelType);
    vtable = CModelInfo::ms_clumpVtable;
    animFileIndex = -1;
    CModelInfo::ms_numClumpModelInfos++;
    return this;
}

// (the scene graph isn't deleted here, only let go)
void CClumpModelInfo::Destruct() {
    vtable = CModelInfo::ms_clumpVtable;
    CModelInfo::ms_numClumpModelInfos--;
    if (clump) {
        clump = nullptr;
        MIK_TxdRemoveRef(txdIndex);
        MIK_RemoveAnimRefs(this);
        if (unknown48) {
            Renderer_6C2950(renderer, unknown48, nullptr, 0);
        }
    }
    MIK_BaseDestruct(this);
}

void CClumpModelInfo::Init() {
    clump = nullptr;
}

void CClumpModelInfo::DeleteObjects() {
    if (!clump) {
        return;
    }
    Ni_DeleteClump(clump);
    clump = nullptr;
    MIK_TxdRemoveRef(txdIndex);
    MIK_RemoveAnimRefs(this);
    if (unknown48) {
        Renderer_6C2950(renderer, unknown48, nullptr, 0);
    }
}

void* CClumpModelInfo::CreateInstance() {
    if (!clump) {
        return nullptr;
    }
    MIK_PushMemoryTag(0x24);
    void* instance = Ni_CloneClump(clump);
    MIK_PopMemoryTag();
    return instance;
}

void CClumpModelInfo::SetAnimFile(const char* name) {
    if (MIK_Stricmp(name, "null")) {
        animFileIndex = MIK_GetAnimFileIndex(name);
    }
}

void* CClumpModelInfo::CreateInstance(const void* matrix) {
    if (!clump) {
        return nullptr;
    }
    void* instance = Base()->CreateInstance();
    memcpy(Ni_GetMatrix(Ni_GetRoot(instance)), matrix, 0x34);
    return instance;
}

void CClumpModelInfo::SetClump(void* newClump, int arg) {
    clump = newClump;
    MIK_TxdAddRef(txdIndex);
    MIK_AddAnimRefs(this);
    if (Ni_IsSkinned_5EE0B0(newClump)) {
        Ni_GetRoot(newClump);   // (result unused)
        uint32_t* skin = Ni_5EE100(newClump);
        Ni_ForAllAtomics(newClump, reinterpret_cast<void*>(SetClumpAtomicCB), skin);
        *skin = 0x3000;
    }
    if (unknown48) {
        Renderer_6C2950(renderer, unknown48, newClump, arg);
    }
}

void CClumpModelInfo::SetFrameIds(const void* list) {
    struct NameIdAssociation {
        re::Ptr32<const char> name;
        int32_t id;
        uint32_t flags;   // 1: skipped
    };
    const auto* entries = static_cast<const NameIdAssociation*>(list);
    for (int i = 0; entries[i].name; i++) {
        if (entries[i].flags & 1) {
            continue;
        }
        struct {
            uint32_t hash;
            re::Ptr32<void> found;
        } search{0, nullptr};
        search.hash = MIK_HashName(entries[i].name);
        Ni_ForAllChildren(Ni_GetRoot(clump), reinterpret_cast<void*>(FindFrameFromNameCB), &search);
        if (search.found) {
            At<int32_t>(search.found, 0x98) = entries[i].id;
        }
    }
}

// (the check makes no difference: both ways end the same)
void CClumpModelInfo::SetAtomicRendererCallbacks(void* node) {
    if (!node) {
        node = clump;
    }
    if (type != 5 && Ni_6B1AD0(node)) {
        Ni_ForAllAtomics(node, reinterpret_cast<void*>(SetAtomicRendererCB), nullptr);
        return;
    }
    Ni_ForAllAtomics(node, reinterpret_cast<void*>(SetAtomicRendererCB), nullptr);
}

// The frames: the scene graph's nodes carry an id at +0x98 (0: none)
namespace CClumpModelInfoFrames {

// 0x510A40 (callback): the first node without an id whose name hash is data[0]
void* __cdecl FindFrameFromNameCB_(void* node, void* data) {
    auto* search = static_cast<uint32_t*>(data);
    if (At<int32_t>(node, 0x98) == 0 && search[0] == Ni_GetNameHash(node)) {
        reinterpret_cast<re::Ptr32<void>*>(search)[1] = node;
        return nullptr;
    }
    Ni_ForAllChildren(node, reinterpret_cast<void*>(FindFrameFromNameCB), data);
    return reinterpret_cast<re::Ptr32<void>*>(search)[1] ? nullptr : node;
}

// 0x510A90 (callback): the node with id data[0]
void* __cdecl FindFrameFromIdCB_(void* node, void* data) {
    auto* search = static_cast<int32_t*>(data);
    if (search[0] == At<int32_t>(node, 0x98)) {
        reinterpret_cast<re::Ptr32<void>*>(search)[1] = node;
        return nullptr;
    }
    Ni_ForAllChildren(node, reinterpret_cast<void*>(FindFrameFromIdCB), data);
    return reinterpret_cast<re::Ptr32<void>*>(search)[1] ? nullptr : node;
}

// 0x510C90 (callback, SetClump's for a skinned clump): the atomic's frame given the skin's bone
// table; 0 (the walk goes on)
void* __cdecl SetClumpAtomicCB_(void* atomic, void* table) {
    MIK_SetBoneTable(At<void*>(atomic, 4), table);
    return nullptr;
}

// 0x510AD0 (callback): each node with an id into the array at its id
void* __cdecl FillFrameArrayCB_(void* node, void* data) {
    int32_t id = At<int32_t>(node, 0x98);
    if (id > 0) {
        static_cast<re::Ptr32<void>*>(data)[id] = node;
    }
    Ni_ForAllChildren(node, reinterpret_cast<void*>(FillFrameArrayCB), data);
    return node;
}

// 0x510B00
void* __cdecl GetFrameFromId(void* clump, int id) {
    struct {
        int32_t id;
        re::Ptr32<void> found;
    } search{id, nullptr};
    Ni_ForAllChildren(Ni_GetRoot(clump), reinterpret_cast<void*>(FindFrameFromIdCB), &search);
    return search.found;
}

// 0x510B40
void __cdecl FillFrameArray(void* clump, re::Ptr32<void>* frames) {
    Ni_ForAllChildren(Ni_GetRoot(clump), reinterpret_cast<void*>(FillFrameArrayCB), frames);
}

} // namespace CClumpModelInfoFrames

// ---- simple, time -------------------------------------------------------------------

CSimpleModelInfo* CSimpleModelInfo::Construct() {
    MIK_BaseConstruct(this, MODEL_INFO_SIMPLE);
    vtable = CModelInfo::ms_simpleVtable;
    return this;
}

CSimpleModelInfo* CSimpleModelInfo::Destroy(uint8_t deleteFlags) {
    return VectorDelete(this, deleteFlags, MIK_SimpleDestruct, MIK_SimplePoolDelete);
}

CTimeModelInfo* CTimeModelInfo::Construct() {
    MIK_BaseConstruct(this, MODEL_INFO_TIME);
    vtable = CModelInfo::ms_timeVtable;
    otherModel = -1;
    return this;
}

// (the simple model's destructor: the time model has none of its own)
CTimeModelInfo* CTimeModelInfo::Destroy(uint8_t deleteFlags) {
    return VectorDelete(this, deleteFlags, MIK_SimpleDestruct, MIK_TimePoolDelete);
}

// ---- accessories ---------------------------------------------------------------------

// (its bones: the clump's scene graph is set up with 1, whatever `arg`)
void CAccessoryModelInfo::SetClump(void* newClump, int) {
    MIK_ClumpSetClump(this, newClump, 1);
    Ni_ForAllAtomics(clump, reinterpret_cast<void*>(SetAtomicRendererCB), reinterpret_cast<void*>(AccessoryAtomicCB));
}

// its model from models\accssory\ (sic) and its two hands' bones
void CAccessoryModelInfo::SetName(const char* name) {
    MIK_BaseSetName(this, name);
    // (the game's buffer is 0x20 bytes, overrun by names over 15 characters)
    char path[0x100] = "models\\accssory\\";
    strcat(path, name);
    if (Renderer_LoadModel(renderer, path, &unknown48, 0) != 0) {
        boneIds[0] = -1;
        boneIds[1] = -1;
        return;
    }
    for (int i = 0; i < 2; i++) {
        if (!Skeleton_FindBone(unknown48, MIK_HashName(accessoryBoneNames[i]), &boneIds[i])) {
            boneIds[i] = -1;
        }
    }
}

// ---- peds ----------------------------------------------------------------------------

void CPedModelInfo::SetName(const char* name) {
    MIK_PedSetName(this, name, 0);
}

int CPedModelInfo::GetDependency8() {
    return animFiles[0];
}
int CPedModelInfo::GetDependency9() {
    return animFiles[1];
}
int CPedModelInfo::GetDependency10() {
    return animFiles[2];
}

CPedModelInfo* CPedModelInfo::Destroy(uint8_t deleteFlags) {
    MIK_PedDestruct(this);
    if (deleteFlags & 1) {
        MIK_Free(this);
    }
    return this;
}

// ---- prop animations -----------------------------------------------------------------

CPAnimModelInfo* CPAnimModelInfo::Construct() {
    MIK_ClumpConstruct(this, MODEL_INFO_PANIM);
    actData = nullptr;
    actData74 = nullptr;
    unknown78 = nullptr;
    unknown80 = 0;
    startNode = nullptr;
    vtable = CModelInfo::ms_panimVtable;
    panimIndex = -1;
    unknown8C = -1;
    return this;
}

CPAnimModelInfo* CPAnimModelInfo::Destroy(uint8_t deleteFlags) {
    return VectorDelete(this, deleteFlags, MIK_ClumpDestruct, MIK_PAnimPoolDelete);
}

// its model from models\wprops\ (through the shared path buffer), then its act data
void CPAnimModelInfo::SetName(const char* name) {
    MIK_BaseSetName(this, name);
    isPx = 0;
    if (name && strlen(name) >= 2 && name[0] == 'p' && name[1] == 'x') {
        isPx = 1;
    }
    strcpy(panimPath, "models\\wprops\\");
    strcat(panimPath, name);
    Renderer_LoadModel(renderer, panimPath, &unknown48, 0);
    MIK_PAnimLoadActData(this);
    unknown7C = unknown78;
}

int16_t CPAnimModelInfo::GetPAnimIndex() {
    return panimIndex;
}

void CPAnimModelInfo::LoadActData() {
    MIK_PushMemoryTag(0x2A);
    uint32_t parts[2] = {butes_1B653A0, butes_1B65320};
    ButesPath path{2, parts};
    uint32_t namePart = nameHash;
    actData = Butes_FindChild(&path, &namePart, butesRoot_1B653A4);
    actData74 = Butes_FindChild(&path, &namePart, butesRoot_1B653A8);
    if (!actData) {
        // the default act
        uint32_t defaults[3] = {butes_1B653A0, butes_1B65320, isPx ? butes_1B653C0 : butes_1B653BC};
        ButesPath defaultPath{3, defaults};
        actData = Butes_Find(&defaultPath, butesRoot_1B653A4);
    }
    if (actData) {
        if (const char* name78 = At<const char*>(actData, 0x54)) {
            uint32_t parts78[3] = {butes_1B653A0, butes_1B6538C, 0};
            ButesPart_FromString(&parts78[2], name78);
            ButesPath path78{3, parts78};
            unknown78 = Butes_Find(&path78, butesRoot_1B65390);
        }
        static const int nameOffsets[3] = {0x30, 0x3C, 0x48};
        for (int i = 0; i < 3; i++) {
            if (void* effectName = At<void*>(actData, nameOffsets[i])) {
                effects[i].id = Effects_FindId(effectsManager, effectName);
            }
        }
        for (int i = 0; i < 3; i++) {
            effects[i].kind = At<uint8_t>(actData, nameOffsets[i] + 8);
        }
    }
    MIK_PopMemoryTag();
}

void CPAnimModelInfo::UpdateAnimIndex() {
    if (panimIndex == -1 && actData) {
        panimIndex = MIK_FindPAnimIndex(At<const char*>(actData, 0x6C), Base());
    }
}

void CPAnimModelInfo::SetUnknown8C(const char* name) {
    // (a case-sensitive "null", unlike the clumps' SetAnimFile)
    if (name && strcmp(name, "null") != 0) {
        unknown8C = int16_t(MIK_GetAnimFileIndex(name));
        return;
    }
    unknown8C = -1;
}

int16_t CPAnimModelInfo::GetUnknown8C() {
    return unknown8C;
}

void CPAnimModelInfo::SetUnknown78(const char* name) {
    uint32_t parts[3] = {butes_1B653A0, butes_1B6538C, 0};
    ButesPart_FromString(&parts[2], name);
    ButesPath path{3, parts};
    unknown78 = Butes_Find(&path, butesRoot_1B65390);
}

float CPAnimModelInfo::GetUnknownFloat1() {
    if (actData && At<int32_t>(actData, 0x20) == 3) {
        return panimFloat80;
    }
    return panimFloat60;
}

float CPAnimModelInfo::GetUnknownFloat2() {
    if (actData && At<int32_t>(actData, 0x20) == 4) {
        return 0.0f;
    }
    if (actData && At<int32_t>(actData, 0x20) == 3) {
        return panimFloat20;
    }
    return panimFloat30;
}

// 1..10: the +0x74 data's +0x18.. (by 4), else (or without it) the act data's +0x1C
void* CPAnimModelInfo::GetActField(int which) {
    if (!actData74) {
        return At<void*>(actData, 0x1C);
    }
    if (uint32_t(which - 1) > 9) {
        return At<void*>(actData, 0x1C);
    }
    return At<void*>(actData74, 0x14 + 4 * which);
}

int CPAnimModelInfo::GetUnknown4C(int i) {
    return At<int8_t>(this, 0x4C + i);
}

uint8_t CPAnimModelInfo::IsPx() {
    return isPx;
}

uint8_t CPAnimModelInfo::IsKind1() {
    return panimKind1 == At<int32_t>(actData, 0x80);
}
uint8_t CPAnimModelInfo::IsKind2() {
    return panimKind2 == At<int32_t>(actData, 0x80);
}
uint8_t CPAnimModelInfo::IsKind3() {
    return panimKind3 == At<int32_t>(actData, 0x80);
}

void CPAnimModelInfo::ResetUnknown78() {
    unknown78 = unknown7C;
}

namespace CPAnimModelInfoStatics {

// 0x520960: a prop animation's .cat index by its act file's path (-1: none). The path
// goes through the shared buffer, '\' to '/', the name after the last '/' without ".act"
int16_t __cdecl FindPAnimIndex(const char* name, CBaseModelInfo* mi) {
    if (!name) {
        return -1;
    }
    MIK_Strncpy(panimPath, name, 0x80);
    for (int i = 0; panimPath[i] && i < 0x80; i++) {
        if (panimPath[i] == '\\') {
            panimPath[i] = '/';
        }
    }
    char* file = strrchr(panimPath, '/');
    if (!file) {
        return -1;
    }
    if (char* extension = strstr(panimPath, ".act")) {
        *extension = '\0';
    }
    uint16_t index = CatStore_FindIndex(catStore, file + 1);
    if (mi) {
        MIK_GetModelIndexByHash(mi->nameHash, 0, 0x2FA7);   // (result unused)
    }
    return int16_t(index);
}

// 0x520A50: CPool<CPAnimModelInfo>::New (thiscall), zeroed; nullptr when full
CPAnimModelInfo* __fastcall PoolNew(CPool<CPAnimModelInfo>* pool, re::Edx) {
    bool wrapped = false;
    if (!(pool->flags[pool->lastUsed] & 0x80)) {
        do {
            if (++pool->lastUsed >= pool->size) {
                if (wrapped) {
                    return nullptr;
                }
                pool->lastUsed = 0;
                wrapped = true;
            }
        } while (!(pool->flags[pool->lastUsed] & 0x80));
    }
    uint8_t& flag = pool->flags[pool->lastUsed];
    flag &= 0x7F;
    flag = uint8_t((flag & 0x80) | ((flag + 1) & 0x7F));
    auto* object = reinterpret_cast<CPAnimModelInfo*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * pool->lastUsed);
    MIK_Memset(object, 0, sizeof(CPAnimModelInfo));
    return object;
}

// 0x520AD0 / 0x520D70: operator new / delete, from its pool
void* __cdecl OperatorNew(size_t) {
    return MIK_PAnimPoolNew(&CModelInfo::ms_panimModelStore);
}

void __cdecl OperatorDelete(void* object) {
    CModelInfo::ms_panimModelStore.Delete(static_cast<CPAnimModelInfo*>(object));
}

} // namespace CPAnimModelInfoStatics

// ---- registry ------------------------------------------------------------------------

#define MIK_REVERSED(name, address, fn)                                                                        \
    static void* RE_CONCAT(Original_, __LINE__);                                                              \
    REVERSED(name, address, fn, nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)
#define MIK_MEMBER(name, address, member) MIK_REVERSED(name, address, re::MemberAddress(member))
#define MIK_STATIC(name, address, fn) MIK_REVERSED(name, address, reinterpret_cast<void*>(fn))

MIK_MEMBER("CClumpModelInfo::Construct", 0x005109B0, static_cast<CClumpModelInfo* (CClumpModelInfo::*)()>(&CClumpModelInfo::Construct));
MIK_MEMBER("CClumpModelInfo::ConstructWithType", 0x005109E0,
           static_cast<CClumpModelInfo* (CClumpModelInfo::*)(uint8_t)>(&CClumpModelInfo::Construct));
MIK_MEMBER("CClumpModelInfo::Destruct", 0x00510D90, &CClumpModelInfo::Destruct);
MIK_MEMBER("CClumpModelInfo::Init", 0x0050E7A0, &CClumpModelInfo::Init);
MIK_MEMBER("CClumpModelInfo::DeleteObjects", 0x00510C40, &CClumpModelInfo::DeleteObjects);
MIK_MEMBER("CClumpModelInfo::CreateInstance", 0x00510D60, static_cast<void* (CClumpModelInfo::*)()>(&CClumpModelInfo::CreateInstance));
MIK_MEMBER("CClumpModelInfo::CreateInstanceAt", 0x00510D20,
           static_cast<void* (CClumpModelInfo::*)(const void*)>(&CClumpModelInfo::CreateInstance));
MIK_MEMBER("CClumpModelInfo::SetAnimFile", 0x00510A10, &CClumpModelInfo::SetAnimFile);
MIK_MEMBER("CClumpModelInfo::SetClump", 0x00510CB0, &CClumpModelInfo::SetClump);
MIK_MEMBER("CClumpModelInfo::SetFrameIds", 0x00510B70, &CClumpModelInfo::SetFrameIds);
MIK_MEMBER("CClumpModelInfo::SetAtomicRendererCallbacks", 0x00510BF0, &CClumpModelInfo::SetAtomicRendererCallbacks);
MIK_STATIC("CClumpModelInfo::FindFrameFromNameCB", 0x00510A40, &CClumpModelInfoFrames::FindFrameFromNameCB_);
MIK_STATIC("CClumpModelInfo::FindFrameFromIdCB", 0x00510A90, &CClumpModelInfoFrames::FindFrameFromIdCB_);
MIK_STATIC("CClumpModelInfo::FillFrameArrayCB", 0x00510AD0, &CClumpModelInfoFrames::FillFrameArrayCB_);
MIK_STATIC("CClumpModelInfo::SetClumpAtomicCB", 0x00510C90, &CClumpModelInfoFrames::SetClumpAtomicCB_);
MIK_STATIC("CClumpModelInfo::GetFrameFromId", 0x00510B00, &CClumpModelInfoFrames::GetFrameFromId);
MIK_STATIC("CClumpModelInfo::FillFrameArray", 0x00510B40, &CClumpModelInfoFrames::FillFrameArray);
MIK_MEMBER("CSimpleModelInfo::Construct", 0x0051A770, &CSimpleModelInfo::Construct);
MIK_MEMBER("CSimpleModelInfo::Destroy", 0x0051A9D0, &CSimpleModelInfo::Destroy);
MIK_MEMBER("CTimeModelInfo::Construct", 0x0051A790, &CTimeModelInfo::Construct);
MIK_MEMBER("CTimeModelInfo::Destroy", 0x0051AA40, &CTimeModelInfo::Destroy);
MIK_MEMBER("CAccessoryModelInfo::SetClump", 0x0050E7B0, &CAccessoryModelInfo::SetClump);
MIK_MEMBER("CAccessoryModelInfo::SetName", 0x0050E7E0, &CAccessoryModelInfo::SetName);
MIK_MEMBER("CPedModelInfo::SetName", 0x0051A7C0, &CPedModelInfo::SetName);
MIK_MEMBER("CPedModelInfo::GetDependency8", 0x0051A7D0, &CPedModelInfo::GetDependency8);
MIK_MEMBER("CPedModelInfo::GetDependency9", 0x0051A7E0, &CPedModelInfo::GetDependency9);
MIK_MEMBER("CPedModelInfo::GetDependency10", 0x0051A7F0, &CPedModelInfo::GetDependency10);
MIK_MEMBER("CPedModelInfo::Destroy", 0x0051A800, &CPedModelInfo::Destroy);
MIK_MEMBER("CPAnimModelInfo::Construct", 0x0051A820, &CPAnimModelInfo::Construct);
MIK_MEMBER("CPAnimModelInfo::Destroy", 0x0051AAB0, &CPAnimModelInfo::Destroy);
MIK_MEMBER("CPAnimModelInfo::SetName", 0x00520AE0, &CPAnimModelInfo::SetName);
MIK_MEMBER("CPAnimModelInfo::GetPAnimIndex", 0x0051A860, &CPAnimModelInfo::GetPAnimIndex);
MIK_MEMBER("CPAnimModelInfo::LoadActData", 0x005205B0, &CPAnimModelInfo::LoadActData);
MIK_MEMBER("CPAnimModelInfo::UpdateAnimIndex", 0x00520A20, &CPAnimModelInfo::UpdateAnimIndex);
MIK_MEMBER("CPAnimModelInfo::SetUnknown8C", 0x00520180, &CPAnimModelInfo::SetUnknown8C);
MIK_MEMBER("CPAnimModelInfo::GetUnknown8C", 0x00520370, &CPAnimModelInfo::GetUnknown8C);
MIK_MEMBER("CPAnimModelInfo::SetUnknown78", 0x00520530, &CPAnimModelInfo::SetUnknown78);
MIK_MEMBER("CPAnimModelInfo::GetUnknownFloat1", 0x00520320, &CPAnimModelInfo::GetUnknownFloat1);
MIK_MEMBER("CPAnimModelInfo::GetUnknownFloat2", 0x00520340, &CPAnimModelInfo::GetUnknownFloat2);
MIK_MEMBER("CPAnimModelInfo::GetActField", 0x00520390, &CPAnimModelInfo::GetActField);
MIK_MEMBER("CPAnimModelInfo::GetUnknown4C", 0x00520420, &CPAnimModelInfo::GetUnknown4C);
MIK_MEMBER("CPAnimModelInfo::IsPx", 0x00520430, &CPAnimModelInfo::IsPx);
MIK_MEMBER("CPAnimModelInfo::IsKind1", 0x00520440, &CPAnimModelInfo::IsKind1);
MIK_MEMBER("CPAnimModelInfo::IsKind2", 0x00520460, &CPAnimModelInfo::IsKind2);
MIK_MEMBER("CPAnimModelInfo::IsKind3", 0x00520480, &CPAnimModelInfo::IsKind3);
MIK_MEMBER("CPAnimModelInfo::ResetUnknown78", 0x00520030, &CPAnimModelInfo::ResetUnknown78);
MIK_STATIC("CPAnimModelInfo::FindPAnimIndex", 0x00520960, &CPAnimModelInfoStatics::FindPAnimIndex);
MIK_STATIC("CPool<CPAnimModelInfo>::New", 0x00520A50, &CPAnimModelInfoStatics::PoolNew);
MIK_STATIC("CPAnimModelInfo::operator new", 0x00520AD0, &CPAnimModelInfoStatics::OperatorNew);
MIK_STATIC("CPAnimModelInfo::operator delete", 0x00520D70, &CPAnimModelInfoStatics::OperatorDelete);
