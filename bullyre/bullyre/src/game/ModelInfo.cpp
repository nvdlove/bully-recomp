#include "game/ModelInfo.h"

#include "game/Streaming.h"
#include "game/TxdStore.h"
#include "log.h"
#include "reversed.h"

namespace game {
GAME_FN(uint32_t, __cdecl, HashModelName, 0x00576ED0, const char* name);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// the collision side, not reversed yet
GAME_FN(void, __thiscall, ColModel_Destroy, 0x0056E7D0, void* col);
GAME_FN(void, __cdecl, ColModelPool_Delete, 0x0056EB30, void* col);
// Gamebryo: a refcounted object let go
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, NiRefObject* object);
// the animation manager's refs on a group
GAME_FN(void, __thiscall, AnimManager_AddRef, 0x006BD010, void* animManager, uintptr_t group, void* data);
// (0x6BD030: AnimManager_6BD030 in Streaming.h, its RemoveRef)
// its butes by its name (+0x18, where modes apply)
GAME_FN(void, __thiscall, ModelInfo_FindButes, 0x0050EBF0, CBaseModelInfo* mi);
// (CTxdStore's, where modes apply)
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_RemoveRef, 0x005F1160, int slot);
// (its own, called where modes apply)
GAME_FN(void, __thiscall, ModelInfo_AddAnimRefs, 0x0050E960, CBaseModelInfo* mi);
GAME_FN(void, __thiscall, ModelInfo_RemoveAnimRefs, 0x0050EA40, CBaseModelInfo* mi);
GAME_FN(void, __thiscall, ModelInfo_Delete2dEffects, 0x0050EDA0, CBaseModelInfo* mi);
} // namespace game

CBaseModelInfo* CBaseModelInfo::Construct(uint8_t modelType) {
    type = modelType;
    unknown12 = -1;
    first2dEffect = -1;
    txdIndex = -1;
    vtable = CModelInfo::ms_baseVtable;
    colModel = nullptr;
    ownsColModel = 0;
    numRefs = 0;
    nameHash = 0;
    unknown0A = 0xFF;
    unknown0B = 0xFF;
    unknown1C = 0;
    return this;
}

// (Shutdown without the virtual call: the destructor's part)
static void DeleteColModelAndEffects(CBaseModelInfo* mi) {
    void* col = mi->colModel;
    if (col && mi->ownsColModel) {
        game::ColModel_Destroy(col);
        game::ColModelPool_Delete(col);
        mi->colModel = nullptr;
    }
}

CBaseModelInfo* CBaseModelInfo::Destroy(uint8_t deleteFlags) {
    vtable = CModelInfo::ms_baseVtable;
    DeleteColModelAndEffects(this);
    game::ModelInfo_Delete2dEffects(this);
    txdIndex = -1;
    unknown12 = -1;
    if (deleteFlags & 1) {
        game::Free(this);
    }
    return this;
}

void CBaseModelInfo::SetName(const char* name) {
    if (name) {
        nameHash = game::HashModelName(name);
        game::ModelInfo_FindButes(this);
    }
}

void CBaseModelInfo::Shutdown() {
    DeleteColModelAndEffects(this);
    DeleteObjects();
    game::ModelInfo_Delete2dEffects(this);
    txdIndex = -1;
    unknown12 = -1;
}

uint8_t CBaseModelInfo::MatchesName(const char* name) {
    return game::HashModelName(name) == nameHash;
}

void CBaseModelInfo::SetColModel(void* col, uint8_t owns) {
    colModel = col;
    ownsColModel = owns;
}

// each of its animation files (slots 7..10), with its group's data
template <typename Fn>
static void ForEachAnimFile(CBaseModelInfo* mi, Fn fn) {
    int (CBaseModelInfo::*const dependencies[])() = {&CBaseModelInfo::GetAnimFileIndex, &CBaseModelInfo::GetDependency8,
                                                     &CBaseModelInfo::GetDependency9, &CBaseModelInfo::GetDependency10};
    for (auto get : dependencies) {
        if ((mi->*get)() != -1) {
            re::Ptr32<void> data = mi;  // (the game's out slot starts out holding `this`)
            int group = game::GetAgrGroup((mi->*get)(), &data);
            fn(group, data);
        }
    }
}

void CBaseModelInfo::AddAnimRefs() {
    ForEachAnimFile(this, [](int group, void* data) { game::AnimManager_AddRef(game::animManager, group, data); });
}

void CBaseModelInfo::RemoveAnimRefs() {
    ForEachAnimFile(this, [](int group, void* data) { game::AnimManager_6BD030(game::animManager, group, data, 0); });
}

int CBaseModelInfo::AddRef() {
    numRefs++;
    game::Txd_AddRef(txdIndex);
    game::ModelInfo_AddAnimRefs(this);
    return numRefs;
}

int CBaseModelInfo::RemoveRef() {
    numRefs--;
    game::Txd_RemoveRef(txdIndex);
    game::ModelInfo_RemoveAnimRefs(this);
    return numRefs;
}

void CBaseModelInfo::Add2dEffect(C2dEffect* effect) {
    effect->next = first2dEffect;
    first2dEffect = int16_t(CModelInfo::ms_2dEffectPool.GetIndex(effect));
}

C2dEffect* CBaseModelInfo::GetFirst2dEffect() {
    if (first2dEffect == -1) {
        return nullptr;
    }
    return CModelInfo::ms_2dEffectPool.GetAt(first2dEffect);
}

void CBaseModelInfo::Delete2dEffects() {
    while (first2dEffect != -1) {
        C2dEffect* effect = CModelInfo::ms_2dEffectPool.GetAt(first2dEffect);
        first2dEffect = effect->next;
        if (effect->object14) {
            game::NiRelease(effect->object14);
        }
        effect->object14 = nullptr;
        if (effect->object18) {
            game::NiRelease(effect->object18);
        }
        effect->object18 = nullptr;
        CModelInfo::ms_2dEffectPool.Delete(effect);
    }
}

// ---- registry -------------------------------------------------------------------------

static uint8_t(__thiscall* Original_MatchesName)(CBaseModelInfo*, const char*);
static C2dEffect*(__thiscall* Original_GetFirst2dEffect)(CBaseModelInfo*);

static uint8_t __fastcall Shadow_MatchesName(CBaseModelInfo* mi, re::Edx, const char* name) {
    uint8_t theirs = Original_MatchesName(mi, name);
    uint8_t ours = mi->MatchesName(name);
    static uint32_t compared = 0;
    re::Compared("CBaseModelInfo::MatchesName", compared);
    if (re::Flag(theirs) != re::Flag(ours)) {
        re::Mismatch("CBaseModelInfo::MatchesName", "%s: game %d, ours %d", name, theirs, ours);
    }
    return theirs;
}

static C2dEffect* __fastcall Shadow_GetFirst2dEffect(CBaseModelInfo* mi, re::Edx) {
    C2dEffect* theirs = Original_GetFirst2dEffect(mi);
    C2dEffect* ours = mi->GetFirst2dEffect();
    static uint32_t compared = 0;
    re::Compared("CBaseModelInfo::GetFirst2dEffect", compared);
    if (theirs != ours) {
        re::Mismatch("CBaseModelInfo::GetFirst2dEffect", "model info %p: game %p, ours %p", mi, theirs, ours);
    }
    return theirs;
}

REVERSED("CBaseModelInfo::MatchesName", 0x0050E940, re::MemberAddress(&CBaseModelInfo::MatchesName),
         reinterpret_cast<void*>(&Shadow_MatchesName), reinterpret_cast<void**>(&Original_MatchesName), re::Mode::Ours);
REVERSED("CBaseModelInfo::GetFirst2dEffect", 0x0050ED70, re::MemberAddress(&CBaseModelInfo::GetFirst2dEffect),
         reinterpret_cast<void*>(&Shadow_GetFirst2dEffect), reinterpret_cast<void**>(&Original_GetFirst2dEffect), re::Mode::Ours);

#define MI_REVERSED(fn, address, mode)                                                                              \
    static void* Original_##fn;                                                                                     \
    REVERSED("CBaseModelInfo::" #fn, address, re::MemberAddress(&CBaseModelInfo::fn), nullptr, &Original_##fn, mode)

MI_REVERSED(Construct, 0x0050E8C0, re::Mode::Ours);
MI_REVERSED(Destroy, 0x0050EEC0, re::Mode::Ours);
MI_REVERSED(SetName, 0x0050ED40, re::Mode::Ours);
MI_REVERSED(Shutdown, 0x0050EE70, re::Mode::Ours);
MI_REVERSED(SetColModel, 0x0050EB30, re::Mode::Ours);
MI_REVERSED(AddRef, 0x0050EB90, re::Mode::Ours);
MI_REVERSED(RemoveRef, 0x0050EBC0, re::Mode::Ours);
MI_REVERSED(AddAnimRefs, 0x0050E960, re::Mode::Ours);
MI_REVERSED(RemoveAnimRefs, 0x0050EA40, re::Mode::Ours);
MI_REVERSED(Add2dEffect, 0x0050EB70, re::Mode::Ours);
MI_REVERSED(Delete2dEffects, 0x0050EDA0, re::Mode::Ours);

// ---- more of the base ------------------------------------------------------------------

namespace {
GAME_FN(void, __cdecl, MI_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, MI_PopMemoryTag, 0x005EF100);
GAME_FN(void, __thiscall, MI_Delete2dEffects, 0x0050EDA0, CBaseModelInfo* mi);
GAME_FN(int16_t, __cdecl, MI_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(int16_t, __cdecl, MI_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(uint8_t, __cdecl, MI_NiRelease, 0x005F0F90, NiRefObject* object);
GAME_FN(void, __cdecl, MI_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, MI_2dEffectPoolDelete, 0x005276C0, C2dEffect* effect);

// The butes (the game's data tree) are looked up by a path of name hashes
struct ButesPath {
    int32_t count;
    re::Ptr32<const uint32_t> parts;
};
GAME_FN(void*, __cdecl, Butes_Find, 0x0072AEA0, const ButesPath* path, const uint32_t* root);
GAME_VAR(const uint32_t, butesObjectsRoot, 0x01B65318);
GAME_VAR(const uint32_t, butesPart_1B6531C, 0x01B6531C);
GAME_VAR(const uint32_t, butesPart_1B65320, 0x01B65320);
GAME_VAR(const uint32_t, butesPart_1B65324, 0x01B65324);
GAME_VAR(const uint32_t, butesPart_1B65328, 0x01B65328);
} // namespace

void CBaseModelInfo::Destruct() {
    vtable = CModelInfo::ms_baseVtable;
    DeleteColModelAndEffects(this);
    MI_Delete2dEffects(this);
    txdIndex = -1;
    unknown12 = -1;
}

void CBaseModelInfo::SetTexDictionary(const char* name, int seasonal) {
    txdIndex = MI_FindTxdSlot(name, seasonal);
    if (txdIndex == -1) {
        txdIndex = MI_AddTxdSlot(name);
    }
}

// three places, the first found (the game reads the parts' hashes as it goes)
void CBaseModelInfo::FindButes() {
    MI_PushMemoryTag(0x2A);
    uint32_t parts[3] = {butesPart_1B6531C, butesPart_1B65320, nameHash};
    ButesPath path{3, parts};
    unknown18 = Butes_Find(&path, &butesObjectsRoot);
    if (!unknown18) {
        uint32_t parts2[2] = {butesPart_1B65324, nameHash};
        ButesPath path2{2, parts2};
        unknown18 = Butes_Find(&path2, &butesObjectsRoot);
    }
    if (!unknown18) {
        uint32_t parts3[2] = {butesPart_1B65328, nameHash};
        ButesPath path3{2, parts3};
        unknown18 = Butes_Find(&path3, &butesObjectsRoot);
    }
    MI_PopMemoryTag();
}

static void Release2dEffectObjects(C2dEffect* effect) {
    if (effect->object14) {
        MI_NiRelease(effect->object14);
    }
    effect->object14 = nullptr;
    if (effect->object18) {
        MI_NiRelease(effect->object18);
    }
    effect->object18 = nullptr;
}

C2dEffect* C2dEffect::Destroy(uint8_t deleteFlags) {
    if (deleteFlags & 2) {
        // (new[]: the count is before the first, and they go last to first)
        int32_t* block = reinterpret_cast<int32_t*>(this) - 1;
        for (int i = *block - 1; i >= 0; i--) {
            Release2dEffectObjects(this + i);
        }
        if (deleteFlags & 1) {
            MI_Free(block);
        }
        return reinterpret_cast<C2dEffect*>(block);
    }
    Release2dEffectObjects(this);
    if (deleteFlags & 1) {
        MI_2dEffectPoolDelete(this);
    }
    return this;
}

MI_REVERSED(Destruct, 0x0050EE20, re::Mode::Original);
MI_REVERSED(SetTexDictionary, 0x0050E900, re::Mode::Original);
MI_REVERSED(FindButes, 0x0050EBF0, re::Mode::Original);

static void* Original_C2dEffect_Destroy;
REVERSED("C2dEffect::Destroy", 0x0051B2A0, re::MemberAddress(&C2dEffect::Destroy), nullptr, &Original_C2dEffect_Destroy,
         re::Mode::Original);
