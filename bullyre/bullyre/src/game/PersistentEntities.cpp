// PersistentEntities (the one at 0xBA6C4C): the areas whose special entities scripts
// asked to keep loaded (AreaLoadSpecialEntities), a one-off override
// (AreaEnsureSpecialEntitiesAreCreatedWithOverride), and loading them now.
// 0x6AE6F0..0x6AEBAE.

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

namespace {

struct PersistentEntities {
    int32_t areas[4];      // +0 name hashes of the areas whose entities to keep
    uint32_t areaCount;    // +0x10
    int32_t framesLeft;    // +0x14 frames the peds stay held after a forced load
    int32_t overrideArea;  // +0x18 name hash (-1: none)
    int32_t overrideKind;  // +0x1C the check kind it answers yes to (0: answers no)
};
static_assert(offsetof(PersistentEntities, overrideKind) == 0x1C);

struct CVector2D {
    float x, y;
};

GAME_VAR(uint8_t*, pe_playerPed, 0x00C1AEA8);  // CWorld::Players[0].m_pPed
GAME_VAR(CPool<uint8_t>*, pe_pedPool, 0x00C0F5F0);
GAME_VAR(uint8_t, pe_ensuringSpecialEntities, 0x00C13F04);
inline void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);

GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, NameKey_FromValue, 0x005FA870, uint32_t* key, int32_t value);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* key);
GAME_FN(int32_t, __thiscall, MissionMgr_Find, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetStats, 0x006AA7C0, void* mgr, int32_t mission);
GAME_FN(int32_t*, __thiscall, MissionMgr_Current, 0x006AA680, void* mgr);
GAME_FN(uint8_t, __thiscall, Mission_IsRunning, 0x006AA3F0, void* mission);
GAME_FN(void, __cdecl, EnsureCollisionIsInMemory, 0x00570080, const CVector2D* xy);
GAME_FN(void, __cdecl, Fn_439780, 0x00439780);
GAME_FN(void, __cdecl, LoadingScreen_Message, 0x0043C4F0, const char* title, const char* text, uintptr_t);
GAME_FN(void, __cdecl, LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __cdecl, CreateSpecialEntities_453A00, 0x00453A00);
GAME_FN(void, __thiscall, Ped_474C10, 0x00474C10, void* ped, int, int);

const float* PlayerPosition() {
    uint8_t* ped = pe_playerPed;
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x14);
    return reinterpret_cast<const float*>(matrix ? matrix + 0x30 : ped + 4);
}

// the mission `hash` names: its index (the key object is made and destroyed around it)
int32_t FindMission(int32_t hash) {
    uint32_t key;
    NameKey_FromValue(&key, hash);
    int32_t mission = MissionMgr_Find(kMissionMgr, &key);
    NameKey_Destruct(&key);
    return mission;
}

bool IsCurrentMission(int32_t mission) {
    return Mission_IsRunning(MissionMgr_Current(kMissionMgr)) && *MissionMgr_Current(kMissionMgr) == mission;
}

bool Passed(int32_t mission) {
    return *reinterpret_cast<uint16_t*>(MissionMgr_GetStats(kMissionMgr, mission) + 2) != 0;
}

// 0x6AE6F0
void __fastcall SetOverride(PersistentEntities* self, re::Edx, const char* area, int32_t kind) {
    self->overrideArea = -1;
    if (area) {
        self->overrideArea = int32_t(HashName(area));
    }
    self->overrideKind = kind;
}

// 0x6AE730
void __fastcall AddArea(PersistentEntities* self, re::Edx, int32_t hash) {
    uint32_t count = self->areaCount;
    for (uint32_t i = 0; i < count; i++) {
        if (self->areas[i] == hash) {
            return;
        }
    }
    if (count != 4) {
        self->areas[count] = hash;
        self->areaCount++;
    }
}

// 0x6AE760 (the last one moves into the gap)
void __fastcall RemoveArea(PersistentEntities* self, re::Edx, int32_t hash) {
    uint32_t count = self->areaCount;
    for (uint32_t i = 0; i < count; i++) {
        if (self->areas[i] == hash) {
            self->areas[i] = self->areas[count - 1];
            self->areaCount--;
            return;
        }
    }
}

// 0x6AE790: whether an entity with this condition (kind, name hash: a mission's, or an
// area's for kind 5) is to exist now
uint8_t __fastcall ShouldExist(PersistentEntities* self, re::Edx, int32_t kind, int32_t hash) {
    uint8_t result = 0;
    switch (kind) {
    case 1: {  // the mission not passed and not running
        int32_t mission = FindMission(hash);
        if (!Passed(mission)) {
            if (!IsCurrentMission(mission)) {
                result = 1;
            }
        }
        break;
    }
    case 2: {  // the mission running
        int32_t mission = FindMission(hash);
        if (IsCurrentMission(mission)) {
            result = 1;
        }
        break;
    }
    case 3: {  // running or passed
        int32_t mission = FindMission(hash);
        if (IsCurrentMission(mission) || Passed(mission)) {
            result = 1;
        }
        break;
    }
    case 4:  // passed
        result = Passed(FindMission(hash)) ? 1 : 0;
        break;
    case 5:  // an area kept loaded
        for (uint32_t i = 0; i < self->areaCount; i++) {
            if (self->areas[i] == hash) {
                result = 1;
                break;
            }
        }
        break;
    case 6:  // not passed
        result = Passed(FindMission(hash)) ? 0 : 1;
        break;
    case 7: {  // not running
        result = 1;
        int32_t mission = FindMission(hash);
        if (IsCurrentMission(mission)) {
            result = 0;
        }
        break;
    }
    default:  // 0 and anything above 7
        break;
    }
    if (self->overrideArea != -1 && self->overrideArea == hash) {
        if (kind == self->overrideKind) {
            result = 1;
        }
        if (self->overrideKind == 0) {
            return 0;
        }
    }
    return result;
}

// 0x6AEA10: loads the special entities around the player now and holds every ped
// for 4 frames
void __fastcall EnsureSpecialEntitiesInMemory(PersistentEntities* self, re::Edx) {
    const float* pos = PlayerPosition();
    CVector2D xy{pos[0], pos[1]};
    EnsureCollisionIsInMemory(&xy);
    Fn_439780();
    pos = PlayerPosition();
    xy = CVector2D{pos[0], pos[1]};
    EnsureCollisionIsInMemory(&xy);
    LoadingScreen_Message("PersistentEntities::EnsureSpecialEntitiesInMemory()- 1",
                          "CStreaming::LoadAllRequestedModels()", 0);
    LoadAllRequestedModels(0);
    pe_ensuringSpecialEntities = 1;
    CreateSpecialEntities_453A00();
    LoadingScreen_Message("PersistentEntities::EnsureSpecialEntitiesInMemory()- 2",
                          "CStreaming::LoadAllRequestedModels()", 0);
    LoadAllRequestedModels(0);
    pe_ensuringSpecialEntities = 0;
    self->framesLeft = 4;
    CPool<uint8_t>* pool = pe_pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        uint8_t* ped = pool->GetAt(slot);
        if (ped) {
            *reinterpret_cast<int32_t*>(ped + 0x1358) = 1;
            Ped_474C10(ped, 0, 0);
        }
    }
}

// 0x6AEB40: counts the held frames down (loading what's requested) and lets the peds go
void __fastcall Update(PersistentEntities* self, re::Edx) {
    if (self->framesLeft == 0) {
        return;
    }
    LoadingScreen_Message("PersistentEntities::Update()", "CStreaming::LoadAllRequestedModels()", 0);
    LoadAllRequestedModels(0);
    if (--self->framesLeft != 0) {
        return;
    }
    CPool<uint8_t>* pool = pe_pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        uint8_t* ped = pool->GetAt(slot);
        if (ped) {
            *reinterpret_cast<int32_t*>(ped + 0x1358) = 0;
            Ped_474C10(ped, 1, 0);
        }
    }
}

} // namespace

#define PERSIST_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PERSIST_REVERSED("PersistentEntities::SetOverride", 0x006AE6F0, &SetOverride);
PERSIST_REVERSED("PersistentEntities::AddArea", 0x006AE730, &AddArea);
PERSIST_REVERSED("PersistentEntities::RemoveArea", 0x006AE760, &RemoveArea);
PERSIST_REVERSED("PersistentEntities::ShouldExist", 0x006AE790, &ShouldExist);
PERSIST_REVERSED("PersistentEntities::EnsureSpecialEntitiesInMemory", 0x006AEA10, &EnsureSpecialEntitiesInMemory);
PERSIST_REVERSED("PersistentEntities::Update", 0x006AEB40, &Update);
