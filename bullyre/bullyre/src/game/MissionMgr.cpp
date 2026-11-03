// The mission manager (CMissionMgr, one at 0x20C3CA0), its mission slots and controllers,
// and the gift requirements. 0x6A99D0..0x6AB6E7.
//
// CMissionMgr: +0 vtable, +4 the missions' tree (its root node), +8 [100] controllers
//   (0xC: two action controllers and the mission, -1: free), +0x4B8 the missions' data
//   (CMissionData, 0x4C each), +0x4BC / +0x5A0 the two slots (0xE4 each: the running
//   mission and a class / a side mission), +0x684 the missions' tracks (for their names),
//   +0x688 the missions' records (6 bytes: +0 attempts, +2 times passed, +4 flags: 1
//   running, 2 started, 8 unlocked, 0x10 class, 0x40 hidden), +0x744 the count, +0x748
//   how many are passed, +0x74D, +0x750.. its state.
// A slot: +0 the mission (-1: none), +4 its script, +8 its state (0 idle, 1 starting,
//   4 running, 8 passed, 0xB done, 0xC failed), +0xC.. its flags, +0x18 the next mission,
//   +0x20 its exit popup (FENameList), +0x84 its last mission, +0x88, +0x8C its texture
//   dictionary, +0x90 [10] (model, texture), +0xE0 their count.
// The mission data (0x4C): +0x18 its name, +0x1C its script, +0x20 its title, +0x25,
//   +0x28 its kind (0 story, 1 class, 3, 4 tutorial, 5), +0x36, +0x4A counted for the
//   completion.
// The gifts (0x508): +0 [64] requirements (0x14: +0 chance, +1 always, +2 the gift, +4
//   the model, +8 / +0x10 missions needed, +0xC a mission that stops it), +0x500 their
//   count, +0x504 [n] the gifts given.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace msnmgr {

namespace {

using re::At;

template <typename R, typename... A>
R VCall(void* object, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(object, args...);
}

void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
void* const kCamera = reinterpret_cast<void*>(0x00C3CC68);
void* const kUserInput = reinterpret_cast<void*>(0x00A45A30);
void* const kLurStore = reinterpret_cast<void*>(0x00D02850);
GAME_VAR(int32_t, g_missionCount, 0x020C43E4);   // the manager's +0x744
GAME_VAR(uint8_t*, g_missionData, 0x020C4158);   // its +0x4B8
GAME_VAR(uint8_t*, g_missionRecords, 0x020C4328); // its +0x688
GAME_VAR(uint8_t*, g_missionTracks, 0x020C4324); // its +0x684
GAME_ARRAY(int32_t, g_slot0, 0x020C415C);         // its +0x4BC
GAME_ARRAY(int32_t, g_slot1, 0x020C4240);         // its +0x5A0
GAME_VAR(void*, g_currentController, 0x020C432C);
GAME_VAR(void*, g_updatingController, 0x020C434C);
GAME_VAR(int32_t, g_tutorialMission, 0x020C43F8);  // its +0x758
GAME_VAR(uint8_t, g_tutorialDone, 0x020C43FC);
GAME_VAR(uint8_t, g_tutorialTimed, 0x020C4404);
GAME_VAR(uint32_t, g_tutorialTimeout, 0x020C4408);
GAME_VAR(uint32_t, g_timeInMs, 0x00C1A9B4);
GAME_VAR(float, g_timeStep, 0x00C1A9A4);
GAME_VAR(float, g_controllerStepScale, 0x00BA6BBC);  // (0.02)
GAME_VAR(uint8_t, g_clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, g_clockMinutes, 0x00BCEB4A);
GAME_VAR(uint8_t*, g_schoolConfig, 0x020C88B8);  // (+0x68 a fade time in ms)
GAME_VAR(uint8_t, g_fadeColourR, 0x00C3CD2C);
GAME_VAR(uint8_t, g_fadeColourG, 0x00C3CD2D);
GAME_VAR(uint8_t, g_fadeColourB, 0x00C3CD2E);
GAME_VAR(uint8_t, g_fadeColourA, 0x00C3CD2F);
GAME_VAR(int32_t, g_modelFirstA, 0x00A136B8);
GAME_VAR(int32_t, g_modelLastA, 0x00A136BC);
GAME_VAR(int32_t, g_modelFirstB, 0x00A136B0);
GAME_VAR(int32_t, g_modelLastB, 0x00A136B4);
GAME_VAR(int16_t, g_typeIdCounter, 0x00BF3830);
GAME_VAR(uint32_t, g_giftMissionKey, 0x01B65478);

GAME_FN(uint32_t*, __thiscall, MM_NameKeyFromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(uint32_t*, __thiscall, MM_NameKey, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, MM_NameKeyDestruct, 0x0049A650, uint32_t* key);
GAME_FN(int32_t, __thiscall, MM_Find, 0x006A9FD0, void* mgr, const uint32_t* key);
GAME_FN(uint8_t*, __thiscall, MM_Record, 0x006AA7C0, void* mgr, int32_t mission);
GAME_FN(void, __thiscall, MM_Start, 0x006AD100, void* mgr, int32_t mission, int);
GAME_FN(uint8_t, __thiscall, MM_AnyClass, 0x006AA840, void* mgr);
GAME_FN(uint8_t, __thiscall, MM_SlotIs, 0x006AA410, int32_t* slot, int32_t mission);
GAME_FN(int32_t, __thiscall, MM_CountTree, 0x006AAA70, void* mgr, const uint32_t* key, void* node);
GAME_FN(int32_t, __thiscall, MM_CountTreeAttempts, 0x006AAB70, void* mgr, const uint32_t* key, void* node);
GAME_FN(uint8_t*, __thiscall, MM_FirstRunning, 0x006AAF20, void* mgr, const uint32_t* key, void* node);
GAME_FN(uint8_t, __thiscall, MM_HasTag, 0x006AB290, void* mgr, int32_t mission, uint32_t hash);
GAME_FN(void, __thiscall, MM_ControllerStop, 0x006A9E60, int32_t* controller);
GAME_FN(void, __thiscall, MM_FinishTutorial, 0x006AAD60, void* mgr, int32_t mission);
inline int __cdecl MM_Rand() { return crt::Rand(); }
GAME_FN(uint8_t*, __thiscall, MM_PedStats, 0x00477DC0, uint8_t* ped);
GAME_FN(int32_t*, __thiscall, MM_PedFeeling, 0x00475380, uint8_t* ped, int32_t* out, int what);
GAME_FN(int, __thiscall, MM_CameraFadeState, 0x004F12F0, void* camera);
GAME_FN(void, __thiscall, MM_CameraFade, 0x004F1AD0, void* camera, float seconds, int in, int);
GAME_FN(void, __cdecl, MM_SetClock, 0x00426070, int hours, int minutes);
GAME_FN(void, __cdecl, MM_ShowFail, 0x007074E0, const char* key, int, float, uintptr_t reason, uintptr_t, int);
GAME_FN(void, __thiscall, MM_InputSet50DFD0, 0x0050DFD0, void* input, int);
GAME_FN(void, __thiscall, MM_PopupShow, 0x00705220, void* popup, int, const char* title, const char* buttons,
        int a, int b);
GAME_FN(void, __cdecl, MM_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, MM_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, MM_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, MM_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void, __cdecl, MM_TextureDestroy, 0x005F0F90, void* texture);
GAME_FN(uint8_t*, __cdecl, MM_ModelInfo, 0x0051B450, int model);
GAME_FN(uint8_t, __cdecl, MM_Model5411B0, 0x005411B0, int model);
GAME_FN(uint8_t, __cdecl, MM_Model5411E0, 0x005411E0, int model);
GAME_FN(uint8_t*, __thiscall, MM_LurFindScript, 0x005DBCC0, void* store, const char* name);
GAME_FN(uint8_t*, __thiscall, MM_LurCurrent, 0x005DB940, void* store);
GAME_FN(uint8_t*, __thiscall, MM_LurGetScript, 0x005DB8D0, void* store, int index);
GAME_FN(int32_t, __thiscall, MM_ScriptCall, 0x005D8200, uint8_t* script, const char* name, int args, int results);
GAME_FN(uint32_t, __cdecl, MM_HashName, 0x00576ED0, const char* name);
GAME_FN(void, __cdecl, MM_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, MM_PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, MM_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, MM_Free, 0x005EEAB0, void* p);
GAME_FN(void*, __thiscall, MM_ControllerConstruct, 0x005F3E50, void* controller);
GAME_FN(void, __thiscall, MM_ControllerReset, 0x005F40A0, void* controller);
GAME_FN(void, __thiscall, MM_ControllerDestruct, 0x005F40F0, void* controller);
GAME_FN(void, __thiscall, MM_ControllerPlay, 0x005F4A50, void* controller, void* node);
GAME_FN(void, __thiscall, MM_ControllerUpdate, 0x005F4B00, void* controller, float dt);
GAME_FN(void, __cdecl, MM_EntityCleanUpOldReference, 0x004657E0, uint8_t* entity, void* reference);
GAME_FN(void, __thiscall, MM_TrackDestruct, 0x0061A8B0, uint8_t* track);

uint8_t* Data(void* mgr, int32_t mission) {
    int32_t index = mission;
    if (index < 0 || index >= At<int32_t>(mgr, 0x744)) {
        index = 0;
    }
    return At<uint8_t*>(mgr, 0x4B8) + index * 0x4C;
}

// the global manager's data, clamped the same way
uint8_t* GlobalData(int32_t mission) {
    int32_t index = mission;
    if (index < 0 || index >= g_missionCount) {
        index = 0;
    }
    return g_missionData + index * 0x4C;
}

uint8_t* Record(void* mgr, int32_t mission) {
    return At<uint8_t*>(mgr, 0x688) + mission * 6;
}

bool Done(int32_t mission) {
    return At<uint16_t>(MM_Record(kMissionMgr, mission), 2) > 0;
}

// a slot's state: in use (not idle, starting or done)
bool SlotActive(const int32_t* slot) {
    return slot[0] != -1 && slot[2] != 0 && slot[2] != 1 && slot[2] != 0xB;
}
// running (not idle or done)
bool SlotRunning(const int32_t* slot) {
    return slot[0] != -1 && slot[2] != 0 && slot[2] != 0xB;
}
// running the mission
bool SlotRuns(const int32_t* slot, int32_t mission) {
    return slot[0] == mission && slot[2] != 0 && slot[2] != 0xB;
}

// a controller made current
void MakeCurrent(void* controller) {
    At<void*>(controller, 0x18) = &g_currentController;
    g_currentController = controller;
}

// a name key's mission done (the key made from a value, destroyed after)
enum class Need { Done, NotDone };

} // namespace

// ---- the gifts ---------------------------------------------------------------------------------

// 0x6A99D0: the requirements cleared
void __fastcall Gifts_Reset(uint8_t* self) {
    At<int32_t>(self, 0x500) = 0;
}

// 0x6A99E0: a requirement added
void __fastcall Gifts_Add(uint8_t* self, re::Edx, const uint32_t* requirement) {
    uint32_t* entry = reinterpret_cast<uint32_t*>(self + At<int32_t>(self, 0x500) * 0x14);
    entry[0] = requirement[0];
    entry[1] = requirement[1];
    entry[2] = requirement[2];
    entry[3] = requirement[3];
    entry[4] = requirement[4];
    At<int32_t>(self, 0x500) += 1;
}

// 0x6A9A20: the gift for a ped (its model's requirement, rolled against its chance, the
// missions it needs done and the one that stops it); the ped's mood gates some kinds
int32_t __fastcall Gifts_For(uint8_t* self, re::Edx, uint8_t* ped) {
    for (int i = 0; i < At<int32_t>(self, 0x500); ++i) {
        uint8_t* entry = self + i * 0x14;
        if (At<int32_t>(entry, 4) != At<int16_t>(ped, 0x10E)) {
            continue;
        }
        if (At<uint8_t>(entry, 1) == 0 && At<int32_t>(self, 0x504 + i * 4) > 0) {
            continue;
        }
        if ((MM_Rand() * 0x62) / 0x7FFF >= At<int8_t>(entry, 0)) {
            continue;
        }
        bool ok = true;
        uint32_t key;
        if (At<uint32_t>(entry, 8) != 0) {
            MM_NameKeyFromValue(&key, At<uint32_t>(entry, 8));
            ok = At<uint16_t>(MM_Record(kMissionMgr, MM_Find(kMissionMgr, &key)), 2) > 0;
            MM_NameKeyDestruct(&key);
        }
        if (ok && At<uint32_t>(entry, 0x10) != 0) {
            MM_NameKeyFromValue(&key, At<uint32_t>(entry, 0x10));
            ok = At<uint16_t>(MM_Record(kMissionMgr, MM_Find(kMissionMgr, &key)), 2) > 0;
            MM_NameKeyDestruct(&key);
        }
        if (ok && At<uint32_t>(entry, 0xC) != 0) {
            MM_NameKeyFromValue(&key, At<uint32_t>(entry, 0xC));
            ok = !(At<uint16_t>(MM_Record(kMissionMgr, MM_Find(kMissionMgr, &key)), 2) > 0);
            MM_NameKeyDestruct(&key);
        }
        if (!ok) {
            continue;
        }
        uint16_t* kind = &At<uint16_t>(entry, 2);
        if (At<uint8_t>(MM_PedStats(ped), 0x50) != 0 && (*kind == 1 || *kind == 2 || *kind == 0)) {
            int32_t mission = MM_Find(kMissionMgr, &g_giftMissionKey);
            if (At<uint16_t>(MM_Record(kMissionMgr, mission), 2) > 0 ||
                (At<uint8_t>(MM_Record(kMissionMgr, mission), 4) & 8) != 0) {
                *kind = 0;
                return i;
            }
        }
        int32_t feeling;
        uint16_t gift = At<uint16_t>(entry, 2);
        if ((gift == 0x16 || gift == 0x17 || gift == 0x18) && *MM_PedFeeling(ped, &feeling, 0xD) != 4) {
            return -1;
        }
        if (At<uint8_t>(MM_PedStats(ped), 0x50) != 0 || At<uint16_t>(entry, 2) != 2) {
            return i;
        }
        if (*MM_PedFeeling(ped, &feeling, 0xD) == 4 || *MM_PedFeeling(ped, &feeling, 0xD) == 3) {
            return i;
        }
        return -1;
    }
    return -1;
}

// 0x6A9C70: the first requirement for a ped's model still waiting on a mission (its
// stopping mission not done, one it needs not done yet)
int32_t __fastcall Gifts_FindPending(uint8_t* self, re::Edx, uint8_t* ped) {
    for (int i = 0; i < At<int32_t>(self, 0x500); ++i) {
        uint8_t* entry = self + i * 0x14;
        if (At<int32_t>(entry, 4) != At<int16_t>(ped, 0x10E)) {
            continue;
        }
        if (At<uint8_t>(entry, 1) == 0 && At<int32_t>(self, 0x504 + i * 4) > 0) {
            continue;
        }
        uint32_t key;
        if (At<uint32_t>(entry, 0xC) != 0) {
            MM_NameKeyFromValue(&key, At<uint32_t>(entry, 0xC));
            bool done = At<uint16_t>(MM_Record(kMissionMgr, MM_Find(kMissionMgr, &key)), 2) > 0;
            MM_NameKeyDestruct(&key);
            if (done) {
                continue;
            }
        }
        if (At<uint32_t>(entry, 8) != 0) {
            MM_NameKeyFromValue(&key, At<uint32_t>(entry, 8));
            bool done = At<uint16_t>(MM_Record(kMissionMgr, MM_Find(kMissionMgr, &key)), 2) > 0;
            MM_NameKeyDestruct(&key);
            if (!done) {
                return i;
            }
        }
        if (At<uint32_t>(entry, 0x10) == 0) {
            continue;
        }
        MM_NameKeyFromValue(&key, At<uint32_t>(entry, 0x10));
        bool done = At<uint16_t>(MM_Record(kMissionMgr, MM_Find(kMissionMgr, &key)), 2) > 0;
        MM_NameKeyDestruct(&key);
        if (!done) {
            return i;
        }
    }
    return -1;
}

// ---- the records -------------------------------------------------------------------------------

// 0x6A9DA0 (not in the function list): a mission record cleared (flags 0x10, 0x80 kept)
uint8_t* __fastcall Record_Clear(uint8_t* self) {
    At<uint8_t>(self, 4) &= 0x90;
    At<uint16_t>(self, 0) = 0;
    At<uint16_t>(self, 2) = 0;
    return self;
}

// 0x6A9DC0: a record copied (flags 2 .. 0x40 of it)
uint8_t* __fastcall Record_Assign(uint8_t* self, re::Edx, const uint8_t* from) {
    At<uint16_t>(self, 0) = *reinterpret_cast<const uint16_t*>(from);
    At<uint16_t>(self, 2) = *reinterpret_cast<const uint16_t*>(from + 2);
    uint16_t flags = *reinterpret_cast<const uint16_t*>(from + 4);
    for (int bit = 1; bit <= 6; ++bit) {
        uint8_t mask = uint8_t(1 << bit);
        uint8_t value = uint8_t((flags >> bit) << bit);
        At<uint8_t>(self, 4) = uint8_t(((value ^ At<uint8_t>(self, 4)) & mask) ^ At<uint8_t>(self, 4));
    }
    return self;
}

// ---- the controllers ---------------------------------------------------------------------------

// 0x6A9E60: a controller stopped: its mission's running flag off, both action
// controllers reset
void __fastcall Controller_Stop(int32_t* self) {
    At<uint8_t>(g_missionRecords, self[2] * 6 + 4) &= 0xFE;
    void* a = reinterpret_cast<void*>(self[0]);
    MakeCurrent(a);
    MM_ControllerReset(a);
    void* b = reinterpret_cast<void*>(self[1]);
    MakeCurrent(b);
    MM_ControllerReset(b);
    self[2] = -1;
}

// 0x6A9EC0 (CMissionData vf1): the deleting destructor (an array's too, flag 2)
uint8_t* __fastcall MissionData_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    auto destruct = [](uint8_t* data) {
        At<uint32_t>(data, 0) = 0x009063D8;
        MM_NameKeyDestruct(&At<uint32_t>(data, 0x44));
        MM_NameKeyDestruct(&At<uint32_t>(data, 0x20));
        MM_NameKeyDestruct(&At<uint32_t>(data, 0x1C));
        MM_NameKeyDestruct(&At<uint32_t>(data, 0x18));
        MM_TrackDestruct(data);
    };
    if (flags & 2) {
        uint8_t* block = self - 4;
        int32_t count = At<int32_t>(block, 0);
        for (int32_t i = count - 1; i >= 0; --i) {
            destruct(self + i * 0x4C);
        }
        if (flags & 1) {
            MM_Free(block);
        }
        return block;
    }
    destruct(self);
    if (flags & 1) {
        MM_Free(self);
    }
    return self;
}

// 0x6A9F80: both slots' +0x16 set (the menu's up)
void __fastcall MissionMgr_SetInMenu(uint8_t* self, re::Edx, int inMenu) {
    for (int i = 0; i < 2; ++i) {
        At<uint8_t>(self, 0x4D2 + i * 0xE4) = uint8_t(inMenu);
    }
}

// 0x6A9FA0: the controller running a mission (-1: none)
int32_t __fastcall MissionMgr_FindController(uint8_t* self, re::Edx, int32_t mission) {
    for (int i = 0; i < 100; ++i) {
        if (At<int32_t>(self, 0x10 + i * 0xC) == mission) {
            return i;
        }
    }
    return -1;
}

// 0x6A9FD0: a mission by name (its track's key; -1: none). (Also the user input's
// controls' lookup: the same layout.)
int32_t __fastcall MissionMgr_Find(uint8_t* self, re::Edx, const uint32_t* key) {
    int32_t count = At<int32_t>(self, 0x744);
    for (int32_t i = 0; i < count; ++i) {
        void* track = At<re::Ptr32<void>*>(self, 0x684)[i];
        const uint32_t* name = VCall<const uint32_t*>(track, 8);
        if (*key == *name) {
            return i;
        }
    }
    return -1;
}

// 0x6AA030: the class running (the running mission of kind 1; -1: none)
int32_t __fastcall MissionMgr_RunningClass(uint8_t* self) {
    int32_t count = At<int32_t>(self, 0x744);
    for (int32_t i = 0; i < count; ++i) {
        if ((At<uint8_t*>(self, 0x688)[i * 6 + 4] & 1) != 0 && At<int32_t>(At<uint8_t*>(self, 0x4B8), i * 0x4C + 0x28) == 1) {
            return i;
        }
    }
    return -1;
}

// 0x6AA080: the camera isn't fading out
uint8_t __cdecl MissionMgr_NotFading() {
    return MM_CameraFadeState(kCamera) != 2;
}

// 0x6AA0A0: a fade to black out (the school's fade time)
void __cdecl MissionMgr_FadeOut() {
    g_fadeColourR = 0;
    g_fadeColourG = 0;
    g_fadeColourB = 0;
    g_fadeColourA = 0xFF;
    float seconds = float(At<int32_t>(g_schoolConfig, 0x68)) / 1000.0f;
    MM_CameraFade(kCamera, seconds, 1, 0);
}

// 0x6AA0F0: and back in
void __cdecl MissionMgr_FadeIn() {
    g_fadeColourR = 0;
    g_fadeColourG = 0;
    g_fadeColourB = 0;
    g_fadeColourA = 0xFF;
    float seconds = float(At<int32_t>(g_schoolConfig, 0x68)) / 1000.0f;
    MM_CameraFade(kCamera, seconds, 0, 0);
}

// 0x6AA140: the missions' count
int32_t __fastcall MissionMgr_Count(uint8_t* self) {
    return At<int32_t>(self, 0x744);
}

// 0x6AA150: the passed missions counted
void __fastcall MissionMgr_CountPassed(uint8_t* self) {
    int32_t count = At<int32_t>(self, 0x744);
    At<int32_t>(self, 0x748) = 0;
    uint16_t* passed = reinterpret_cast<uint16_t*>(At<uint8_t*>(self, 0x688) + 2);
    for (; count > 0; --count, passed += 3) {
        if (*passed != 0) {
            At<int32_t>(self, 0x748) += 1;
        }
    }
}

// 0x6AA190: a slot cleared
void __fastcall Slot_Reset(int32_t* self) {
    uint8_t* s = reinterpret_cast<uint8_t*>(self);
    self[0] = -1;
    self[1] = 0;
    self[2] = 0;
    for (int i = 0xC; i <= 0x16; ++i) {
        s[i] = 0;
    }
    self[6] = -1;
    s[0x1C] = 0;
    s[0x1D] = 0;
    s[0x1E] = 0;
    s[0x70] = 0;
    s[0x80] = 0;
    At<int32_t>(s, 0x84) = -1;
    At<int32_t>(s, 0x88) = 0;
    At<int32_t>(s, 0x8C) = -1;
    At<int32_t>(s, 0xE0) = 0;
}

// 0x6AA1F0: the clock moved on to the next part of the school day
void __cdecl MissionMgr_SkipToNextPeriod() {
    int hours = g_clockHours;
    int minutes = g_clockMinutes;
    if (hours > 24) {
        MM_SetClock(hours, 0);
        return;
    }
    if (hours == 0 || hours >= 23) {
        MM_SetClock(1, 0);
    } else if (hours <= 8) {
        MM_SetClock(9, 0);
    } else if (hours <= 11) {
        if (hours < 11 || minutes < 30) {
            MM_SetClock(11, 30);
        } else {
            MM_SetClock(13, 0);
        }
    } else if (hours == 12) {
        MM_SetClock(13, 0);
    } else if (hours <= 15) {
        if (hours < 15 || minutes < 30) {
            MM_SetClock(15, 30);
        } else {
            MM_SetClock(18, 0);
        }
    } else if (hours <= 17) {
        MM_SetClock(18, 0);
    } else if (hours <= 20) {
        MM_SetClock(21, 0);
    } else {
        MM_SetClock(23, 0);
    }
}

// 0x6AA300: a running mission failed (busted, knocked out: dead or passed out by the
// clock, else the reason given), its flags gathered
uint8_t __fastcall Slot_Fail(int32_t* self, re::Edx, int a, int busted, int knockedOut, int keep, int showMessage,
                             uint32_t reason, int c) {
    uint8_t* s = reinterpret_cast<uint8_t*>(self);
    if (self[1] == 0) {
        return 0;
    }
    uint8_t wasBusted = (s[0xF] != 0 || uint8_t(busted) != 0) ? 1 : 0;
    s[0xF] = wasBusted;
    uint8_t wasOut = (s[0x14] != 0 || uint8_t(knockedOut) != 0) ? 1 : 0;
    s[0x14] = wasOut;
    s[0x10] = (s[0x10] != 0 || uint8_t(c) != 0) ? 1 : 0;
    if (self[2] != 4) {
        return 0;
    }
    if (uint8_t(showMessage) != 0) {
        uint32_t message;
        if (wasBusted) {
            message = 0x00941CF4;  // "M_FAIL_BUSTED"
        } else if (wasOut) {
            uint8_t hours = g_clockHours;
            bool passedOut;
            if (hours == 1) {
                passedOut = g_clockMinutes >= 0x3B;
            } else {
                passedOut = uint8_t(hours - 2) <= 2;
            }
            message = passedOut ? 0x00941CD8 : 0x00941CE8;  // "M_FAIL_PASSOUT" / "M_FAIL_DEAD"
        } else {
            message = reason;
        }
        MM_ShowFail(reinterpret_cast<const char*>(0x00941CD0), 0, 0.0f, message, 0, 0);  // ("M_FAIL")
    }
    s[0xC] = 0;
    self[2] = 0xC;
    s[0xE] = uint8_t(a);
    s[0x13] = uint8_t(keep) == 0;
    return 1;
}

// 0x6AA3F0: a slot in use
uint8_t __fastcall Slot_IsActive(int32_t* self) {
    return SlotActive(self);
}

// 0x6AA410: a slot running this mission
uint8_t __fastcall Slot_Is(int32_t* self, re::Edx, int32_t mission) {
    return self[0] == mission && self[2] != 0 && self[2] != 0xB;
}

// 0x6AA430: a slot running
uint8_t __fastcall Slot_IsRunning(int32_t* self) {
    return SlotRunning(self);
}

// 0x6AA450: a slot's mission abandoned for the next (the flags normalised; a running one
// fails quietly)
void __fastcall Slot_Abandon(int32_t* self, re::Edx, int32_t next) {
    uint8_t* s = reinterpret_cast<uint8_t*>(self);
    self[6] = next;
    s[0x1C] = 1;
    s[0x1D] = 0;
    if (self[1] == 0) {
        return;
    }
    s[0xF] = s[0xF] != 0;
    s[0x14] = s[0x14] != 0;
    s[0x10] = s[0x10] != 0;
    if (self[2] == 4) {
        s[0xC] = 0;
        self[2] = 0xC;
        s[0xE] = 0;
        s[0x13] = 0;
    }
}

// 0x6AA4A0: another mission asked for: started when nothing runs, else the exit popup
void __fastcall Slot_Request(int32_t* self, re::Edx, int32_t mission) {
    if (self[0] == -1 || self[2] == 0 || self[2] == 0xB) {
        MM_Start(kMissionMgr, mission, 0);
        return;
    }
    uint8_t* s = reinterpret_cast<uint8_t*>(self);
    self[6] = mission;
    s[0x1D] = 1;
    s[0x16] = 1;
    MM_InputSet50DFD0(kUserInput, 0);
    MM_PopupShow(s + 0x20, 1, reinterpret_cast<const char*>(0x00941D04), reinterpret_cast<const char*>(0x00941D14), 7,
                 8);  // ("EXIT_MISSION", "EXIT_BUTTONS")
}

// 0x6AA510: a slot's models' textures looked up in its dictionary (up to 10)
void __fastcall Slot_LoadTextures(uint8_t* self) {
    if (At<int32_t>(self, 0x8C) < 0) {
        return;
    }
    MM_PushCurrentTxd();
    MM_SetCurrentTxd(At<int32_t>(self, 0x8C));
    auto scan = [&](int32_t first, int32_t last, uint8_t(__cdecl* skip)(int)) {
        for (int32_t model = first; model <= last; ++model) {
            const char* name = At<const char*>(MM_ModelInfo(model), 0x4C);
            if (name == nullptr || skip(model)) {
                continue;
            }
            void* texture = MM_FindTexture(name, nullptr);
            if (texture == nullptr) {
                continue;
            }
            int32_t count = At<int32_t>(self, 0xE0);
            if (count < 10) {
                At<int32_t>(self, 0xE0) = count + 1;
                At<int32_t>(self, 0x90 + count * 8) = model;
                At<void*>(self, 0x94 + count * 8) = texture;
            }
        }
    };
    scan(g_modelFirstA, g_modelLastA, MM_Model5411B0);
    scan(g_modelFirstB, g_modelLastB, MM_Model5411E0);
    MM_PopCurrentTxd();
}

// 0x6AA620: and released
void __fastcall Slot_ReleaseTextures(uint8_t* self) {
    for (int i = 0; i < At<int32_t>(self, 0xE0); ++i) {
        MM_TextureDestroy(At<void*>(self, 0x94 + i * 8));
    }
}

// 0x6AA660: a mission's data (out of range: the first's)
uint8_t* __fastcall MissionMgr_Data(uint8_t* self, re::Edx, int32_t mission) {
    return Data(self, mission);
}

// 0x6AA680 / 0x6AA690: the slots
int32_t* __fastcall MissionMgr_Slot0(uint8_t* self) {
    return &At<int32_t>(self, 0x4BC);
}
int32_t* __fastcall MissionMgr_Slot1(uint8_t* self) {
    return &At<int32_t>(self, 0x5A0);
}

// 0x6AA6A0: the running script is a slot's mission's
uint8_t __fastcall MissionMgr_ScriptIsMission(uint8_t* self) {
    for (int i = 0; i < 2; ++i) {
        int32_t mission = At<int32_t>(self, 0x4BC + i * 0xE4);
        if (mission == -1) {
            continue;
        }
        uint8_t* script = MM_LurFindScript(kLurStore, At<const char*>(GlobalData(mission), 0x1C));
        if (script == MM_LurCurrent(kLurStore)) {
            return 1;
        }
    }
    return 0;
}

// 0x6AA710: the slot of the running script's mission (else slot 0)
int32_t* __fastcall MissionMgr_ScriptSlot(uint8_t* self) {
    for (int i = 0; i < 2; ++i) {
        int32_t mission = At<int32_t>(self, 0x4BC + i * 0xE4);
        if (mission == -1) {
            continue;
        }
        uint8_t* script = MM_LurFindScript(kLurStore, At<const char*>(GlobalData(mission), 0x1C));
        if (script == MM_LurCurrent(kLurStore)) {
            return &At<int32_t>(self, 0x4BC + i * 0xE4);
        }
    }
    return &At<int32_t>(self, 0x4BC);
}

// 0x6AA790: the current slot (1 when in use, else 0)
int32_t* __cdecl MissionMgr_CurrentSlot() {
    if (SlotActive(g_slot1)) {
        return g_slot1;
    }
    return g_slot0;
}

// 0x6AA7C0: a mission's record
uint8_t* __fastcall MissionMgr_Record(uint8_t* self, re::Edx, int32_t mission) {
    return Record(self, mission);
}

// 0x6AA7E0: a slot in use
uint32_t __cdecl MissionMgr_AnyActive() {
    bool a = SlotActive(g_slot0);
    bool b = SlotActive(g_slot1);
    return a || b;
}

// 0x6AA840: a class running in a slot
uint8_t __fastcall MissionMgr_InClass(uint8_t* self) {
    int32_t mission = At<int32_t>(self, 0x4BC);
    if (mission > -1 && At<int32_t>(Data(self, mission), 0x28) == 1) {
        return 1;
    }
    mission = At<int32_t>(self, 0x5A0);
    if (mission > -1 && At<int32_t>(Data(self, mission), 0x28) == 1) {
        return 1;
    }
    return 0;
}

// 0x6AA890: a kind-5 mission running in a slot
uint8_t __fastcall MissionMgr_InKind5(uint8_t* self) {
    int32_t mission = At<int32_t>(self, 0x4BC);
    if (mission > -1 && At<int32_t>(Data(self, mission), 0x28) == 5) {
        return 1;
    }
    mission = At<int32_t>(self, 0x5A0);
    if (mission > -1 && At<int32_t>(Data(self, mission), 0x28) == 5) {
        return 1;
    }
    return 0;
}

// 0x6AA8F0: a slot running
uint8_t __cdecl MissionMgr_AnyRunning() {
    bool a = SlotRunning(g_slot0);
    bool b = SlotRunning(g_slot1);
    return a || b;
}

// 0x6AA940: a slot running the mission
uint8_t __fastcall MissionMgr_IsRunning(void*, re::Edx, int32_t mission) {
    bool a = SlotRuns(g_slot0, mission);
    bool b = SlotRuns(g_slot1, mission);
    return a || b;
}

// 0x6AA9A0: a mission may start (no side mission running, no class; classes and kind 5
// always; else nothing in slot 0, or it's running this one when `ignoreRunning` is clear)
uint8_t __fastcall MissionMgr_IsAvailable(uint8_t* self, re::Edx, int32_t mission, int ignoreRunning) {
    int32_t kind = At<int32_t>(GlobalData(mission), 0x28);
    bool classLike = kind == 5 || At<int32_t>(GlobalData(mission), 0x28) == 1;
    if (SlotActive(&At<int32_t>(self, 0x5A0))) {
        return 0;
    }
    if (MM_AnyClass(self)) {
        return 0;
    }
    if (classLike || !SlotActive(&At<int32_t>(self, 0x4BC))) {
        return 1;
    }
    if (uint8_t(ignoreRunning) == 0 && MM_SlotIs(&At<int32_t>(self, 0x4BC), mission)) {
        return 1;
    }
    return 0;
}

// 0x6AAA70 / 0x6AAB70: over the missions' tree: the node named `key` (from `node`, null:
// the root) and its children, their records' passes (+2) / attempts (+0, less the
// running ones) summed; nodes found under earlier siblings added. (Mission 0 isn't
// counted.)
template <int Field, bool LessRunning>
int32_t __fastcall MissionMgr_CountTree(uint8_t* self, re::Edx, const uint32_t* key, void* node) {
    uint32_t (__thiscall* const recurse)(void*, const uint32_t*, void*) =
        reinterpret_cast<uint32_t(__thiscall*)(void*, const uint32_t*, void*)>(LessRunning ? 0x006AAB70 : 0x006AAA70);
    int32_t total = 0;
    if (node == nullptr) {
        node = At<void*>(self, 4);
    }
    auto add = [&](int32_t& sum, void* child) {
        int32_t mission = MM_Find(self, VCall<const uint32_t*>(child, 8));
        if (mission > 0) {
            sum += At<uint16_t>(At<uint8_t*>(self, 0x688), mission * 6 + Field);
            if constexpr (LessRunning) {
                if (SlotRuns(g_slot0, mission) || SlotRuns(g_slot1, mission)) {
                    sum -= 1;
                }
            }
        }
    };
    for (int i = 0; i < VCall<int32_t>(node, 0x2C); ++i) {
        void* child = VCall<void*>(node, 0x30, i);
        const uint32_t* name = VCall<const uint32_t*>(child, 8);
        if (*key == *name) {
            int32_t sum = total;
            add(sum, child);
            for (int j = 0; j < VCall<int32_t>(child, 0x2C); ++j) {
                add(sum, VCall<void*>(child, 0x30, j));
            }
            return sum;
        }
        if (VCall<int32_t>(child, 0x2C) != 0) {
            total += int32_t(recurse(self, key, child));
        }
    }
    return total;
}

// 0x6AAD10: a slot's mission has +0x36 clear
uint8_t __fastcall MissionMgr_Slot36Clear(uint8_t* self) {
    int32_t mission = At<int32_t>(self, 0x4BC);
    if (mission > -1 && At<uint8_t>(Data(self, mission), 0x36) == 0) {
        return 1;
    }
    mission = At<int32_t>(self, 0x5A0);
    if (mission > -1 && At<uint8_t>(Data(self, mission), 0x36) == 0) {
        return 1;
    }
    return 0;
}

// 0x6AAD60 (not in the function list): the tutorial finished: its controller stopped,
// its record passed once more
void __fastcall MissionMgr_FinishTutorial(uint8_t* self, re::Edx, int32_t mission) {
    int32_t index = -1;
    for (int i = 0; i < 100; ++i) {
        if (At<int32_t>(self, 0x10 + i * 0xC) == mission) {
            index = i;
            break;
        }
    }
    MM_ControllerStop(&At<int32_t>(self, 8 + index * 0xC));
    uint8_t* record = At<uint8_t*>(self, 0x688) + mission * 6;
    record[4] &= 0xFE;
    record = At<uint8_t*>(self, 0x688) + mission * 6;
    record[4] &= 0xFD;
    At<uint16_t>(At<uint8_t*>(self, 0x688), mission * 6 + 2) += 1;
    At<int32_t>(self, 0x758) = -1;
}

// 0x6AADD0: the classes running in controllers marked (record flag 0x10)
void __fastcall MissionMgr_MarkClasses(uint8_t* self) {
    for (int i = 0; i < 100; ++i) {
        int32_t mission = At<int32_t>(self, 0x10 + i * 0xC);
        if (mission != -1 && At<int32_t>(Data(self, mission), 0x28) == 1) {
            g_missionRecords[mission * 6 + 4] |= 0x10;
        }
    }
}

// 0x6AAF10: a mission's track
void* __fastcall MissionMgr_Track(uint8_t* self, re::Edx, int32_t mission) {
    return At<re::Ptr32<void>*>(self, 0x684)[mission];
}

// 0x6AAF20: under the tree node named `key`: the first child mission running (record flag
// 1), its data (null: none)
uint8_t* __fastcall MissionMgr_FirstRunning(uint8_t* self, re::Edx, const uint32_t* key, void* node) {
    if (node == nullptr) {
        node = At<void*>(self, 4);
    }
    for (int i = 0; i < VCall<int32_t>(node, 0x2C); ++i) {
        void* child = VCall<void*>(node, 0x30, i);
        if (*key == *VCall<const uint32_t*>(child, 8)) {
            for (int j = 0; j < VCall<int32_t>(child, 0x2C); ++j) {
                void* grandchild = VCall<void*>(child, 0x30, j);
                int32_t mission = MM_Find(self, VCall<const uint32_t*>(grandchild, 8));
                if (mission > 0 && (At<uint8_t*>(self, 0x688)[mission * 6 + 4] & 1) != 0) {
                    if (mission >= At<int32_t>(self, 0x744)) {
                        mission = 0;
                    }
                    return At<uint8_t*>(self, 0x4B8) + mission * 0x4C;
                }
            }
            return nullptr;
        }
        if (VCall<int32_t>(child, 0x2C) != 0) {
            uint8_t* found = MM_FirstRunning(self, key, child);
            if (found != nullptr) {
                return found;
            }
        }
    }
    return nullptr;
}

// 0x6AB010
void __fastcall MissionMgr_Set74D(uint8_t* self, re::Edx, int value) {
    At<uint8_t>(self, 0x74D) = uint8_t(value);
}

// 0x6AB020: a story (0) / kind-3 mission passed and repeatable (+0x25 clear)
uint8_t __fastcall MissionMgr_IsReplayable(uint8_t* self, re::Edx, int32_t mission) {
    uint8_t* data = At<uint8_t*>(self, 0x4B8);
    int32_t index = mission < 0 || mission >= At<int32_t>(self, 0x744) ? 0 : mission;
    if (At<int32_t>(data, index * 0x4C + 0x28) != 0) {
        index = mission < 0 || mission >= At<int32_t>(self, 0x744) ? 0 : mission;
        if (At<int32_t>(data, index * 0x4C + 0x28) != 3) {
            return 0;
        }
    }
    index = mission < 0 || mission >= At<int32_t>(self, 0x744) ? 0 : mission;
    if (At<uint8_t>(data, index * 0x4C + 0x25) == 0 && At<uint16_t>(At<uint8_t*>(self, 0x688), mission * 6 + 2) > 0) {
        return 1;
    }
    return 0;
}

// 0x6AB0A0: a value of the node its controller plays (the last controller running the
// mission): the first child's, by its kind (four node kinds: +0x38, +0x24, +0x24,
// +0x28); -1 when none. The child made for it is deleted after.
int32_t __fastcall MissionMgr_NodeValue(uint8_t* self, re::Edx, int32_t mission) {
    void* tree = nullptr;
    for (int i = 0; i < 100; ++i) {
        uint8_t* controller = self + 8 + i * 0xC;
        if (At<int32_t>(controller, 8) == mission) {
            tree = At<void*>(At<uint8_t*>(controller, 0), 0x10);
        }
    }
    int32_t result = -1;
    if (tree == nullptr || VCall<int32_t>(tree, 0x40) < 1) {
        return -1;
    }
    void* child = VCall<void*>(tree, 0x44, 0);
    auto isA = [&](uintptr_t guardAddress, uintptr_t idAddress) {
        uint32_t& guard = *reinterpret_cast<uint32_t*>(guardAddress);
        int16_t& id = *reinterpret_cast<int16_t*>(idAddress);
        if (!(guard & 1)) {
            guard |= 1;
            id = g_typeIdCounter;
            g_typeIdCounter = int16_t(id + 1);
        }
        return VCall<uint8_t>(child, 0, int(uint16_t(id))) != 0;
    };
    void* a = nullptr;
    void* b = nullptr;
    void* c = nullptr;
    void* d = nullptr;
    if (child != nullptr && isA(0x01B65998, 0x01B65994)) {
        a = child;
    }
    if (child != nullptr && isA(0x01B659A8, 0x01B659A4)) {
        b = child;
    }
    if (child != nullptr && isA(0x01B659B0, 0x01B659AC)) {
        c = child;
    }
    if (child != nullptr && isA(0x01B659B8, 0x01B659B4)) {
        d = child;
    }
    if (a != nullptr) {
        result = At<int32_t>(a, 0x38);
    } else if (b != nullptr) {
        result = At<int32_t>(b, 0x24);
    } else if (c != nullptr) {
        result = At<int32_t>(c, 0x24);
    } else if (d != nullptr) {
        result = At<int32_t>(d, 0x28);
    }
    if (child != nullptr) {
        VCall<void>(child, 4, 1);
    }
    return result;
}

// 0x6AB290: a mission's track (or one it's nested in) is named `hash`
uint8_t __fastcall MissionMgr_HasTag(uint8_t* self, re::Edx, int32_t mission, uint32_t hash) {
    void* track = At<re::Ptr32<void>*>(self, 0x684)[mission];
    while (track != nullptr) {
        if (*VCall<const uint32_t*>(track, 8) == hash) {
            return 1;
        }
        track = VCall<void*>(track, 0x34, -1);
    }
    return 0;
}

// 0x6AB2E0: a debug mission
uint8_t __fastcall MissionMgr_IsDebug(uint8_t* self, re::Edx, int32_t mission) {
    return MM_HasTag(self, mission, MM_HashName("Debug"));
}

// 0x6AB310: a texture from the running slots' dictionaries
void* __fastcall MissionMgr_FindTexture(uint8_t* self, re::Edx, const char* name, const char* mask) {
    for (int i = 0; i < 2; ++i) {
        uint8_t* slot = self + 0x4BC + i * 0xE4;
        if (At<int32_t>(slot, 0) < 0 || At<int32_t>(slot, 0x8C) < 0) {
            continue;
        }
        MM_PushCurrentTxd();
        MM_SetCurrentTxd(At<int32_t>(slot, 0x8C));
        void* texture = MM_FindTexture(name, mask);
        MM_PopCurrentTxd();
        if (texture != nullptr) {
            return texture;
        }
    }
    return nullptr;
}

// 0x6AB380: an arcade mission
uint8_t __fastcall MissionMgr_IsArcade(uint8_t* self, re::Edx, int32_t mission) {
    if (MM_HasTag(self, mission, MM_HashName("ArcadeMiniGames"))) {
        return 1;
    }
    if (MM_HasTag(self, mission, MM_HashName("ArcadeRace"))) {
        return 1;
    }
    return 0;
}

// 0x6AB3D0: a model's texture from the running slots' lists (null: none)
void* __fastcall MissionMgr_ModelTexture(uint8_t* self, re::Edx, int32_t model) {
    for (int i = 0; i < 2; ++i) {
        uint8_t* slot = self + 0x4BC + i * 0xE4;
        if (At<int32_t>(slot, 0) < 0 || At<int32_t>(slot, 0x8C) < 0) {
            continue;
        }
        int32_t count = At<int32_t>(slot, 0xE0);
        for (int j = 0; j < count; ++j) {
            if (At<int32_t>(slot, 0x90 + j * 8) == model) {
                void* texture = At<void*>(slot, 0x94 + j * 8);
                if (texture != nullptr) {
                    return texture;
                }
                break;
            }
        }
    }
    return nullptr;
}

// 0x6AB430: a controller started on a mission: its action controllers made (once) and
// reset, the mission's "Active" node played; story / kind-3 missions call the main
// script's F_UnlockModelChanges
uint8_t __fastcall Controller_Start(int32_t* self, re::Edx, int32_t mission) {
    self[2] = mission;
    for (int i = 0; i < 2; ++i) {
        if (self[i] == 0) {
            MM_PushMemoryTag(0x26);
            void* memory = MM_Malloc(0x38);
            self[i] = memory != nullptr ? reinterpret_cast<int32_t>(MM_ControllerConstruct(memory)) : 0;
            MM_PopMemoryTag();
        }
        void* controller = reinterpret_cast<void*>(self[i]);
        MakeCurrent(controller);
        MM_ControllerReset(controller);
    }
    uint32_t key;
    MM_NameKey(&key, "Active");
    void* node = VCall<void*>(re::PtrCast<re::Ptr32<void>*>(g_missionTracks)[self[2]], 0xC, &key);
    MM_NameKeyDestruct(&key);
    void* controller = reinterpret_cast<void*>(self[0]);
    MakeCurrent(controller);
    MM_ControllerPlay(controller, node);
    int32_t kind = At<int32_t>(GlobalData(mission), 0x28);
    if (kind == 0 || At<int32_t>(GlobalData(mission), 0x28) == 3) {
        uint8_t* script = MM_LurGetScript(kLurStore, 0);
        if (script != nullptr) {
            MM_ScriptCall(script, "F_UnlockModelChanges", 0, 1);
        }
    }
    return 1;
}

// 0x6AB580: a controller updated; the tutorial (kind 4) times out after 2 minutes once
// told to, or ends when done
void __fastcall Controller_Update(int32_t* self, re::Edx, float dt) {
    int32_t mission = self[2];
    if (mission == -1) {
        return;
    }
    if (At<int32_t>(GlobalData(mission), 0x28) == 4) {
        if (g_tutorialMission == -1) {
            g_tutorialDone = 0;
            g_tutorialMission = self[2];
            g_tutorialTimed = 0;
            g_tutorialTimeout = g_timeInMs + 120000;
        } else {
            if (mission != g_tutorialMission) {
                return;
            }
            bool timedOut = g_tutorialTimed != 0 && g_tutorialTimeout < g_timeInMs;
            if (g_tutorialDone != 0 || timedOut) {
                MM_FinishTutorial(kMissionMgr, mission);
                return;
            }
        }
    }
    g_updatingController = self;
    void* a = reinterpret_cast<void*>(self[0]);
    MakeCurrent(a);
    MM_ControllerUpdate(a, dt);
    void* b = reinterpret_cast<void*>(self[1]);
    MakeCurrent(b);
    MM_ControllerUpdate(b, dt);
}

// 0x6AB670: a named node of the mission played on the second controller (and stepped)
void __fastcall Controller_PlayNode(int32_t* self, re::Edx, const char* name) {
    uint32_t key;
    MM_NameKey(&key, name);
    void* node = VCall<void*>(re::PtrCast<re::Ptr32<void>*>(g_missionTracks)[self[2]], 0xC, &key);
    MM_NameKeyDestruct(&key);
    if (node == nullptr) {
        return;
    }
    void* controller = reinterpret_cast<void*>(self[1]);
    MakeCurrent(controller);
    MM_ControllerPlay(controller, node);
    float dt = g_controllerStepScale * g_timeStep;
    MM_ControllerUpdate(reinterpret_cast<void*>(self[1]), dt);
}

} // namespace msnmgr

#define MSNMGR_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(msnmgr_original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(msnmgr_original_, __LINE__), \
             re::Mode::Original)

MSNMGR_REVERSED("CGifts::Reset", 0x006A99D0, &msnmgr::Gifts_Reset);
MSNMGR_REVERSED("CGifts::Add", 0x006A99E0, &msnmgr::Gifts_Add);
MSNMGR_REVERSED("CGifts::For", 0x006A9A20, &msnmgr::Gifts_For);
MSNMGR_REVERSED("CGifts::FindPending", 0x006A9C70, &msnmgr::Gifts_FindPending);
MSNMGR_REVERSED("CMissionRecord::Clear", 0x006A9DA0, &msnmgr::Record_Clear);
MSNMGR_REVERSED("CMissionRecord::Assign", 0x006A9DC0, &msnmgr::Record_Assign);
MSNMGR_REVERSED("CMissionController::Stop", 0x006A9E60, &msnmgr::Controller_Stop);
MSNMGR_REVERSED("CMissionData::DeletingDestruct", 0x006A9EC0, &msnmgr::MissionData_DeletingDestruct);
MSNMGR_REVERSED("CMissionMgr::SetInMenu", 0x006A9F80, &msnmgr::MissionMgr_SetInMenu);
MSNMGR_REVERSED("CMissionMgr::FindController", 0x006A9FA0, &msnmgr::MissionMgr_FindController);
MSNMGR_REVERSED("CMissionMgr::Find", 0x006A9FD0, &msnmgr::MissionMgr_Find);
MSNMGR_REVERSED("CMissionMgr::RunningClass", 0x006AA030, &msnmgr::MissionMgr_RunningClass);
MSNMGR_REVERSED("CMissionMgr::NotFading", 0x006AA080, &msnmgr::MissionMgr_NotFading);
MSNMGR_REVERSED("CMissionMgr::FadeOut", 0x006AA0A0, &msnmgr::MissionMgr_FadeOut);
MSNMGR_REVERSED("CMissionMgr::FadeIn", 0x006AA0F0, &msnmgr::MissionMgr_FadeIn);
MSNMGR_REVERSED("CMissionMgr::Count", 0x006AA140, &msnmgr::MissionMgr_Count);
MSNMGR_REVERSED("CMissionMgr::CountPassed", 0x006AA150, &msnmgr::MissionMgr_CountPassed);
MSNMGR_REVERSED("CMissionSlot::Reset", 0x006AA190, &msnmgr::Slot_Reset);
MSNMGR_REVERSED("CMissionMgr::SkipToNextPeriod", 0x006AA1F0, &msnmgr::MissionMgr_SkipToNextPeriod);
MSNMGR_REVERSED("CMissionSlot::Fail", 0x006AA300, &msnmgr::Slot_Fail);
MSNMGR_REVERSED("CMissionSlot::IsActive", 0x006AA3F0, &msnmgr::Slot_IsActive);
MSNMGR_REVERSED("CMissionSlot::Is", 0x006AA410, &msnmgr::Slot_Is);
MSNMGR_REVERSED("CMissionSlot::IsRunning", 0x006AA430, &msnmgr::Slot_IsRunning);
MSNMGR_REVERSED("CMissionSlot::Abandon", 0x006AA450, &msnmgr::Slot_Abandon);
MSNMGR_REVERSED("CMissionSlot::Request", 0x006AA4A0, &msnmgr::Slot_Request);
MSNMGR_REVERSED("CMissionSlot::LoadTextures", 0x006AA510, &msnmgr::Slot_LoadTextures);
MSNMGR_REVERSED("CMissionSlot::ReleaseTextures", 0x006AA620, &msnmgr::Slot_ReleaseTextures);
MSNMGR_REVERSED("CMissionMgr::Data", 0x006AA660, &msnmgr::MissionMgr_Data);
MSNMGR_REVERSED("CMissionMgr::Slot0", 0x006AA680, &msnmgr::MissionMgr_Slot0);
MSNMGR_REVERSED("CMissionMgr::Slot1", 0x006AA690, &msnmgr::MissionMgr_Slot1);
MSNMGR_REVERSED("CMissionMgr::ScriptIsMission", 0x006AA6A0, &msnmgr::MissionMgr_ScriptIsMission);
MSNMGR_REVERSED("CMissionMgr::ScriptSlot", 0x006AA710, &msnmgr::MissionMgr_ScriptSlot);
MSNMGR_REVERSED("CMissionMgr::CurrentSlot", 0x006AA790, &msnmgr::MissionMgr_CurrentSlot);
MSNMGR_REVERSED("CMissionMgr::Record", 0x006AA7C0, &msnmgr::MissionMgr_Record);  // (regcheck: 0x69FC95.. "sbb reg, reg" after it: false hits)
MSNMGR_REVERSED("CMissionMgr::AnyActive", 0x006AA7E0, &msnmgr::MissionMgr_AnyActive);
MSNMGR_REVERSED("CMissionMgr::InClass", 0x006AA840, &msnmgr::MissionMgr_InClass);
MSNMGR_REVERSED("CMissionMgr::InKind5", 0x006AA890, &msnmgr::MissionMgr_InKind5);
MSNMGR_REVERSED("CMissionMgr::AnyRunning", 0x006AA8F0, &msnmgr::MissionMgr_AnyRunning);
MSNMGR_REVERSED("CMissionMgr::IsRunning", 0x006AA940, &msnmgr::MissionMgr_IsRunning);
MSNMGR_REVERSED("CMissionMgr::IsAvailable", 0x006AA9A0, &msnmgr::MissionMgr_IsAvailable);
MSNMGR_REVERSED("CMissionMgr::CountPassedUnder", 0x006AAA70, (&msnmgr::MissionMgr_CountTree<2, false>));
MSNMGR_REVERSED("CMissionMgr::CountAttemptsUnder", 0x006AAB70, (&msnmgr::MissionMgr_CountTree<0, true>));
MSNMGR_REVERSED("CMissionMgr::Slot36Clear", 0x006AAD10, &msnmgr::MissionMgr_Slot36Clear);
MSNMGR_REVERSED("CMissionMgr::FinishTutorial", 0x006AAD60, &msnmgr::MissionMgr_FinishTutorial);
MSNMGR_REVERSED("CMissionMgr::MarkClasses", 0x006AADD0, &msnmgr::MissionMgr_MarkClasses);
MSNMGR_REVERSED("CMissionMgr::Track", 0x006AAF10, &msnmgr::MissionMgr_Track);
MSNMGR_REVERSED("CMissionMgr::FirstRunning", 0x006AAF20, &msnmgr::MissionMgr_FirstRunning);
MSNMGR_REVERSED("CMissionMgr::Set74D", 0x006AB010, &msnmgr::MissionMgr_Set74D);
MSNMGR_REVERSED("CMissionMgr::IsReplayable", 0x006AB020, &msnmgr::MissionMgr_IsReplayable);
MSNMGR_REVERSED("CMissionMgr::NodeValue", 0x006AB0A0, &msnmgr::MissionMgr_NodeValue);
MSNMGR_REVERSED("CMissionMgr::HasTag", 0x006AB290, &msnmgr::MissionMgr_HasTag);
MSNMGR_REVERSED("CMissionMgr::IsDebug", 0x006AB2E0, &msnmgr::MissionMgr_IsDebug);
MSNMGR_REVERSED("CMissionMgr::FindTexture", 0x006AB310, &msnmgr::MissionMgr_FindTexture);
MSNMGR_REVERSED("CMissionMgr::IsArcade", 0x006AB380, &msnmgr::MissionMgr_IsArcade);
MSNMGR_REVERSED("CMissionMgr::ModelTexture", 0x006AB3D0, &msnmgr::MissionMgr_ModelTexture);
MSNMGR_REVERSED("CMissionController::Start", 0x006AB430, &msnmgr::Controller_Start);
MSNMGR_REVERSED("CMissionController::Update", 0x006AB580, &msnmgr::Controller_Update);
MSNMGR_REVERSED("CMissionController::PlayNode", 0x006AB670, &msnmgr::Controller_PlayNode);
