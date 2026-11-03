// The sound banks (cSCREAMBankManager, 0xCF5DC4; see SoundBanks.cpp): the load queue,
// loading and unloading (manual counts at the bank's +0x33), the timed requests (twenty
// at +0x1A0: a bank, flags, the time it lasts to), the weapons', missions', weather's and
// triggers' banks, the area change and the update. 0x599F20..0x59B190, with 0x59ACA0
// (missing from the function lists).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Pool.h"
#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

// a bank (0x48 bytes; see SoundBanks2.cpp)
struct Bank {
    char name[0x32];      // +0
    int8_t slot;          // +0x32 the voice engine's (-1 not loaded)
    uint8_t manual;       // +0x33 the requests holding it
    uint8_t flags;        // +0x34 1 loaded, 2 its load done, 4 its unload done, 8 a LOD
    uint8_t pad35[3];
    uint32_t type;        // +0x38 the low five bits: 1 static, 2 mission, 3 area, 4 trigger,
                          // 5 manual, 6 area force load, 7 mission force load
    int32_t unknown3C;    // +0x3C
    int32_t size;         // +0x40
    re::Ptr32<void> file;           // +0x44
};
static_assert(sizeof(Bank) == 0x48);

// a queued load or unload (4 bytes at +0x114)
struct QueuedBank {
    int16_t bank;
    uint8_t flags;   // 1 a load
    uint8_t pad;
};

// a timed request (8 bytes at +0x1A0)
struct TimedBank {
    int16_t bank;
    uint8_t flags;   // 1 active
    uint8_t pad;
    uint32_t until;
};

// a weapon's slot (8 bytes at +0x194)
struct WeaponSlot {
    uint8_t held;      // +0 (0: released after its time)
    uint8_t flags;     // +1 1 its bank requested
    uint8_t pad2[2];
    uint32_t until;    // +4
};

// a trigger's bank (see SoundBanks.cpp)
struct TriggerBank {
    int16_t bank;
    uint8_t flags;   // 1 loaded for it
    uint8_t pad3;
    uint32_t hash;
};

struct AreaLists {
    uint8_t bankCount;
    uint8_t reverbCount;
    uint8_t pad2[2];
    re::Ptr32<TriggerBank> banks;
    re::Ptr32<void> reverbs;
};
static_assert(sizeof(AreaLists) == 0xC);

// a mission's banks (see SoundBanks.cpp)
struct MissionBanks {
    int32_t mission;
    int16_t loadCount;
    int16_t unloadCount;
    int16_t load[10];
    int16_t unload[10];
};
static_assert(sizeof(MissionBanks) == 0x30);

inline uint8_t* const soundBanks = reinterpret_cast<uint8_t*>(0x00CF5DC4);
inline uint8_t* const theSounds = reinterpret_cast<uint8_t*>(0x00CF3DA0);
inline uint8_t* const voiceEngine = reinterpret_cast<uint8_t*>(0x00CF7268);
inline void* const clothing = reinterpret_cast<void*>(0x020C5F70);
GAME_ARRAY(AreaLists, areaLists, 0x00CF5AA4);
GAME_ARRAY(void*, channels, 0x00CF4288);
GAME_ARRAY(int32_t, bankLists, 0x00BD7800);        // (the scratch buffer: six lists of 250)
GAME_VAR(int32_t, numBanks, 0x00CF68F4);
GAME_VAR(int16_t*, bankGroups, 0x00CF68F0);
GAME_VAR(int32_t, firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, lastWeapon, 0x00A136B4);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, banksLocked, 0x00CF3BF0);
GAME_VAR(uint8_t, streamsBlocked, 0x00CF4286);
GAME_VAR(uint8_t*, ambience, 0x00CF4374);
GAME_VAR(uint8_t*, ambienceEmitters, 0x00CF4278);
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason, 0x00A147EC);
GAME_VAR(int16_t, newWeatherType, 0x00CE756C);
GAME_VAR(uint8_t, forceSnow, 0x00CE754E);
GAME_VAR(int32_t, cowDanceFrames, 0x00CF68F8);
GAME_VAR(uint32_t, cowOutfit, 0x01B6548C);
GAME_VAR(int32_t, queuedCount, 0x00CF5ED4);        // (the manager's +0x110)
GAME_VAR(CPool<uint8_t>*, triggerPool, 0x020C7C48);

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, LoadingScreen_Message, 0x0043C4F0, const char* title, const char* text, uintptr_t);
GAME_FN(void*, __thiscall, VoiceEngine_LoadBank, 0x005AA350, void* engine, int bank, int async);
GAME_FN(int16_t, __thiscall, VoiceEngine_AddBank, 0x005A91F0, void* engine, void* handle);
GAME_FN(void, __thiscall, VoiceEngine_Unload, 0x005AA400, void* engine, void* handle);
GAME_FN(void, __thiscall, VoiceEngine_Fn5AA3B0, 0x005AA3B0, void* engine);
GAME_FN(void, __thiscall, Channel_Fn5A6EA0, 0x005A6EA0, void* channel);
GAME_FN(float, __cdecl, Audio_Fn59E4F0, 0x0059E4F0, float value, int);
GAME_FN(void, __cdecl, Sounds_PlayLoop, 0x00597400, re::Ptr32<void>* ref, int32_t id, int arg, int a4, int volume, int pitch,
        int unused);
GAME_FN(uint8_t, __thiscall, Clothing_IsWearingOutfit, 0x006CA9F0, void* clothing, uint32_t hash);
GAME_FN(const char*, __cdecl, GetAreaName, 0x004305B0, int area);
GAME_FN(void, __cdecl, Objects_StopAllAmbientSounds, 0x004D01F0);
GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(int, __thiscall, Sounds_FindTrigger, 0x00596120, void* sounds, uint32_t hash);
GAME_FN(uint8_t, __thiscall, Area_Contains, 0x006D6FA0, void* area, const CVector* point);
GAME_FN(uint8_t, __cdecl, IsPlayerInFastVehicle, 0x004469A0);
GAME_FN(uint8_t, __stdcall, IsOutsideArea, 0x00594740, int area);
// its own
GAME_FN(int, __thiscall, Banks_Find, 0x00597FC0, void* self, const char* name);
GAME_FN(MissionBanks*, __thiscall, Banks_FindMission, 0x00597E10, void* self, int32_t mission);
GAME_FN(uint8_t, __thiscall, Banks_SameGroup, 0x00597EA0, void* self, int a, int b, int16_t* order);
GAME_FN(int16_t, __thiscall, Banks_GroupSlot, 0x005980C0, void* self, int bank);
GAME_FN(int, __thiscall, Banks_ForWeapon, 0x00598180, void* self, int32_t model);
GAME_FN(void, __thiscall, Banks_RemoveQueued, 0x00598830, void* self, int index);
GAME_FN(void, __thiscall, Banks_Unqueue, 0x005988A0, void* self, int bank);
GAME_FN(void, __thiscall, Banks_Loaded, 0x005994A0, void* self, int bank);
GAME_FN(uint32_t, __thiscall, Banks_IsLoaded, 0x005995F0, void* self, int bank);
GAME_FN(void, __thiscall, Banks_Unloaded, 0x00599520, void* self, int bank);
GAME_FN(uint8_t, __thiscall, Banks_LoadNow, 0x00599620, void* self, int bank);
GAME_FN(uint8_t, __thiscall, Banks_ShouldQueue, 0x00599FD0, void* self, int bank, int isLoad);
GAME_FN(void, __thiscall, Banks_Queue, 0x0059A130, void* self, int bank, int isLoad);
GAME_FN(void, __thiscall, Banks_UnloadNow, 0x0059A1A0, void* self, int bank, int high, int);
GAME_FN(void, __thiscall, Banks_LoadAsync, 0x0059A270, void* self, int bank);
GAME_FN(void, __thiscall, Banks_Unload, 0x0059A420, void* self, int bank, int manual, int);
GAME_FN(void, __thiscall, Banks_RefreshTimed, 0x0059A6E0, void* self);
GAME_FN(void, __thiscall, Banks_ReleaseTimedAll, 0x0059A740, void* self);
GAME_FN(void, __thiscall, Banks_ReleaseTimed, 0x0059A7D0, void* self, int bank);
GAME_FN(void, __thiscall, Banks_Request, 0x0059A860, void* self, int bank, int ms, int now);
GAME_FN(void, __thiscall, Banks_UnloadAll, 0x0059A950, void* self);
GAME_FN(void, __thiscall, Banks_UpdateWeather, 0x0059AA90, void* self, int keep, int now);
GAME_FN(void, __thiscall, Banks_UpdateCowDance, 0x0059AC30, void* self);
GAME_FN(void, __thiscall, Banks_UpdateTriggerBanks, 0x0059ACA0, void* self);
GAME_FN(void, __thiscall, Banks_ReleaseArea, 0x0059AED0, void* self, int area);

Bank* BanksOf(uint8_t* self) {
    return At<Bank*>(self, 8);
}
QueuedBank* QueueOf(uint8_t* self) {
    return &At<QueuedBank>(self, 0x114);
}
TimedBank* TimedOf(uint8_t* self) {
    return &At<TimedBank>(self, 0x1A0);
}
int32_t& QueueCount(uint8_t* self) {
    return At<int32_t>(self, 0x110);
}
int32_t TypeOf(const Bank& bank) {
    return int32_t(bank.type << 27) >> 27;   // (signed five bits)
}

// a manual request dropped (the bank unloaded when none is left)
void ReleaseManual(uint8_t* self, int bank, int arg) {
    if (BanksOf(self)[bank].manual == 0) {
        // ("Warning - Trying to unload manual bank %s when not loaded!")
    } else {
        BanksOf(self)[bank].manual--;
    }
    if (BanksOf(self)[bank].manual == 0) {
        Banks_UnloadNow(self, bank, bank >> 31, arg);
    }
}

// the trigger at a trigger hash's index (null: none)
void* TriggerOf(uint32_t hash) {
    int index = Sounds_FindTrigger(theSounds, hash);
    if (index == -1 || index < 0) {
        return nullptr;
    }
    CPool<uint8_t>* pool = triggerPool;
    if (pool->flags[index] & 0x80) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * index;
}

// ---- the queue --------------------------------------------------------------------------------

// 0x599F20 (thiscall): a weapon's bank ready (noted in its slot; a slot not held lets
// it go three seconds on)
uint8_t __fastcall Banks_WeaponReady(uint8_t* self, re::Edx, int bank, int32_t weapon) {
    if (weapon < firstWeapon || weapon > lastWeapon) {
        return 0;
    }
    if ((At<WeaponSlot*>(self, 0x194)[weapon - firstWeapon].flags & 1) != 0 || bank == -1) {
        return 0;
    }
    if (Banks_ForWeapon(nullptr, weapon) != bank) {
        return 0;
    }
    if (uint8_t(Banks_IsLoaded(self, bank)) == 0) {
        return 0;
    }
    At<WeaponSlot*>(self, 0x194)[weapon - firstWeapon].flags |= 1;
    if (At<WeaponSlot*>(self, 0x194)[weapon - firstWeapon].held == 0) {
        At<WeaponSlot*>(self, 0x194)[weapon - firstWeapon].until = m_snTimeInMilliseconds + 3000;
    }
    return 1;
}

// 0x599FD0 (thiscall): whether a load (or unload) of the bank needs queuing: a queued
// entry of its group settles it (the queue's first done at once, the opposite one
// dropped); none: when it isn't already so
uint8_t __fastcall Banks_ShouldQueueImpl(uint8_t* self, re::Edx, int bank, int isLoad) {
    int32_t count = QueueCount(self);
    if (count >= 0x1E || bank < 0) {
        return 0;
    }
    int matches = 0;
    int last = -1;
    uint8_t result = 1;
    int32_t order = 0;
    for (int i = 0; i < count; i++) {
        int16_t o;
        if (Banks_SameGroup(self, uint16_t(QueueOf(self)[i].bank), bank, &o) != 0) {
            order = uint16_t(o);
            matches++;
            last = i;
        }
    }
    if (matches >= 1) {
        uint8_t queuedLoad = QueueOf(self)[last].flags & 1;
        if (queuedLoad == uint8_t(isLoad)) {
            result = 0;
            if (uint8_t(isLoad) != 0 && int16_t(order) > 0) {
                if (last == 0) {
                    result = 1;
                } else {
                    QueueOf(self)[last].bank = int16_t(bank);
                }
            }
        } else if (queuedLoad == 0) {
            result = 1;
        } else if (last == 0 && self[0x18C] == 0) {
            result = 1;
            Banks_Loaded(self, QueueOf(self)[0].bank);
        } else {
            result = 0;
            Banks_RemoveQueued(self, last);
        }
    }
    if (matches != 0) {
        return result;
    }
    Bank* banks = BanksOf(self);
    if (banks != nullptr && (banks[Banks_GroupSlot(self, bank)].flags & 1) != 0) {
        return uint8_t(isLoad) != 1 ? 1 : 0;
    }
    return uint8_t(isLoad) != 0 ? 1 : 0;
}

// 0x59A130 (thiscall)
void __fastcall Banks_QueueImpl(uint8_t* self, re::Edx, int bank, int isLoad) {
    if (Banks_ShouldQueue(self, bank, isLoad) == 0) {
        return;
    }
    QueueOf(self)[QueueCount(self)].bank = int16_t(bank);
    uint8_t& flags = QueueOf(self)[QueueCount(self)].flags;
    flags = uint8_t(flags ^ ((flags ^ uint8_t(isLoad)) & 1));
    int32_t count = QueueCount(self);
    if (count == 0) {
        self[0x18C] = QueueOf(self)[0].flags & 1;
    }
    QueueCount(self) = count + 1;
}

// 0x59A1A0 (thiscall; the other two arguments unused): a bank out of the voice engine
void __fastcall Banks_UnloadNowImpl(uint8_t* self, re::Edx, int bank, int, int) {
    Banks_Unqueue(self, bank);
    Bank* b = &BanksOf(self)[bank];
    if (b->flags & 1) {
        VoiceEngine_Unload(voiceEngine, At<void*>(self, 0x10 + b->slot * 8));
        At<uint32_t>(self, 0x10C) = m_snTimeInMilliseconds;
        BanksOf(self)[bank].flags &= 0xFE;
        At<int32_t>(self, 0xC + BanksOf(self)[bank].slot * 8) = 0;
        At<int32_t>(self, 0x10 + BanksOf(self)[bank].slot * 8) = 0;
        BanksOf(self)[bank].slot = -1;
        BanksOf(self)[bank].manual = 0;
        return;
    }
    if (b->slot != -1) {
        VoiceEngine_Unload(voiceEngine, At<void*>(self, 0x10 + b->slot * 8));
        At<uint32_t>(self, 0x10C) = m_snTimeInMilliseconds;
        At<int32_t>(self, 0xC + BanksOf(self)[bank].slot * 8) = 0;
        BanksOf(self)[bank].slot = -1;
        BanksOf(self)[bank].manual = 0;
    }
}

// 0x59A270 (thiscall): a bank loaded in the background (queued behind others; -2: the
// queue's first again). (With -2 the result is stored at bank -2's slot: the game's)
void __fastcall Banks_LoadAsyncImpl(uint8_t* self, re::Edx, int bank) {
    if (banksLocked != 0) {
        return;
    }
    int load;
    if (bank == -2) {
        self[0x18C] = 0;
        load = QueueOf(self)[0].bank;
    } else {
        if (QueueCount(self) != 0) {
            Banks_Queue(self, bank, 1);
            return;
        }
        Bank* banks = BanksOf(self);
        if (banks != nullptr && (banks[Banks_GroupSlot(self, bank)].flags & 1) != 0) {
            return;
        }
        if (banks[bank].slot != -1) {
            return;
        }
        load = bank;
    }
    // (the game builds "audio\xact\<name>" for its debug messages here)
    void* handle = VoiceEngine_LoadBank(voiceEngine, load, 1);
    if (handle != nullptr) {
        int16_t slot = VoiceEngine_AddBank(voiceEngine, handle);
        BanksOf(self)[bank].slot = int8_t(slot);
        At<void*>(self, 0x10 + slot * 8) = handle;
        At<Bank*>(self, 0xC + slot * 8) = &BanksOf(self)[bank];
        return;
    }
    At<uint32_t>(self, 0x10C) = m_snTimeInMilliseconds;
    if (bank != -2) {
        Banks_Queue(self, load, 1);
        self[0x18C] = 0;
    }
}

// 0x59A3E0 (thiscall): a bank loaded (a manual request counted)
void __fastcall Banks_Load(uint8_t* self, re::Edx, int bank, int manual, int async) {
    if (uint8_t(manual) != 0) {
        BanksOf(self)[bank].manual++;
    }
    if (uint8_t(async) != 0) {
        Banks_LoadAsync(self, bank);
        return;
    }
    Banks_LoadNow(self, bank);
}

// 0x59A420 (thiscall): a bank released (unloaded with no manual request left)
void __fastcall Banks_UnloadImpl(uint8_t* self, re::Edx, int bank, int manual, int arg) {
    if (uint8_t(manual) != 0) {
        if (BanksOf(self)[bank].manual == 0) {
            // ("Warning - Trying to unload manual bank %s when not loaded!")
        } else {
            BanksOf(self)[bank].manual--;
        }
    }
    if (BanksOf(self)[bank].manual == 0) {
        Banks_UnloadNow(self, bank, bank >> 31, arg);
    }
}

// 0x59A490 (thiscall): a mission starting: its static banks out, its banks in (the force
// loaded ones at once, after the stream channels are freed)
void __fastcall Banks_MissionStarted(uint8_t* self, re::Edx, int32_t mission) {
    MissionBanks* banks = Banks_FindMission(self, mission);
    if (banks == nullptr) {
        return;
    }
    for (int i = 0; i < banks->unloadCount; i++) {
        ReleaseManual(self, banks->unload[i], 0);
    }
    for (int i = 0; i < banks->loadCount; i++) {
        int32_t type = TypeOf(BanksOf(self)[banks->load[i]]);
        if (type == 7 || type == 6) {
            if (streamsBlocked == 0) {
                for (int c = 0; c < 16; c++) {
                    if (c != 0) {
                        Channel_Fn5A6EA0(channels[c]);
                    }
                }
            }
            break;
        }
    }
    for (int i = 0; i < banks->loadCount; i++) {
        int bank = banks->load[i];
        int32_t type = TypeOf(BanksOf(self)[bank]);
        bool now = type == 7 || type == 6;
        BanksOf(self)[bank].manual++;
        if (now) {
            Banks_LoadNow(self, bank);
        } else {
            Banks_LoadAsync(self, bank);
        }
    }
}

// 0x59A600 (thiscall): a mission ending: its banks out, the static ones back
void __fastcall Banks_MissionEnded(uint8_t* self, re::Edx, int32_t mission) {
    MissionBanks* banks = Banks_FindMission(self, mission);
    if (banks == nullptr) {
        return;
    }
    for (int i = 0; i < banks->loadCount; i++) {
        ReleaseManual(self, banks->load[i], 0);
    }
    for (int i = 0; i < banks->unloadCount; i++) {
        int16_t bank = banks->unload[i];
        BanksOf(self)[bank].manual++;
        Banks_LoadNow(self, bank);
    }
}

// ---- the timed requests --------------------------------------------------------------------

// 0x59A6E0 (thiscall): the requests lasting beyond six seconds reloaded (the others dropped)
void __fastcall Banks_RefreshTimedImpl(uint8_t* self) {
    for (int i = 0; i < 20; i++) {
        TimedBank& t = TimedOf(self)[i];
        if ((t.flags & 1) == 0) {
            continue;
        }
        if (t.until > m_snTimeInMilliseconds + 6000) {
            int16_t bank = t.bank;
            BanksOf(self)[bank].manual++;
            Banks_LoadAsync(self, bank);
        } else {
            t.flags &= 0xFE;
        }
    }
}

// 0x59A740 (thiscall): every request's bank released (those ending within six seconds
// dropped)
void __fastcall Banks_ReleaseTimedAllImpl(uint8_t* self) {
    for (int i = 0; i < 20; i++) {
        TimedBank& t = TimedOf(self)[i];
        if ((t.flags & 1) == 0) {
            continue;
        }
        ReleaseManual(self, t.bank, 1);
        if (t.until < m_snTimeInMilliseconds + 6000) {
            t.flags &= 0xFE;
        }
    }
}

// 0x59A7D0 (thiscall): a bank's request ended
void __fastcall Banks_ReleaseTimedImpl(uint8_t* self, re::Edx, int bank) {
    int i = 0;
    for (; i < 20; i++) {
        TimedBank& t = TimedOf(self)[i];
        if ((t.flags & 1) != 0 && int32_t(t.bank) == bank) {
            break;
        }
    }
    if (i == 20) {
        return;
    }
    ReleaseManual(self, TimedOf(self)[i].bank, 1);
    TimedOf(self)[i].flags &= 0xFE;
}

// 0x59A860 (thiscall): a bank kept loaded for `ms` more (a new request loads it)
void __fastcall Banks_RequestImpl(uint8_t* self, re::Edx, int bank, int ms, int now) {
    int free = -1;
    int i = 0;
    for (; i < 20; i++) {
        TimedBank& t = TimedOf(self)[i];
        if ((t.flags & 1) != 0 && int32_t(t.bank) == bank) {
            break;
        }
        if ((t.flags & 1) == 0) {
            free = i;
        }
    }
    if (i == 20) {
        if (free == -1) {
            return;
        }
        BanksOf(self)[bank].manual++;
        if (uint8_t(now) == 0) {
            Banks_LoadAsync(self, bank);
        } else {
            Banks_LoadNow(self, bank);
        }
        TimedOf(self)[free].flags |= 1;
        TimedOf(self)[free].bank = int16_t(bank);
        TimedOf(self)[free].until = m_snTimeInMilliseconds + ms;
        return;
    }
    int32_t until = int32_t(ms + m_snTimeInMilliseconds);
    if (until > int32_t(TimedOf(self)[i].until)) {
        TimedOf(self)[i].until = uint32_t(until);
    }
    Bank* banks = BanksOf(self);
    if (banks == nullptr || banks[bank].manual != 0) {
        return;
    }
    banks[bank].manual++;
    if (uint8_t(now) != 0) {
        Banks_LoadNow(self, bank);
        return;
    }
    Banks_LoadAsync(self, bank);
}

// ---- unloading everything --------------------------------------------------------------------

// 0x59A950 (thiscall)
void __fastcall Banks_UnloadAllImpl(uint8_t* self) {
    uint64_t i = 0;
    if (uint32_t(numBanks) == 0) {
        return;
    }
    do {
        if (BanksOf(self)[uint32_t(i)].flags & 1) {
            Banks_UnloadNow(self, int(uint32_t(i)), int(uint32_t(i >> 32)), 1);
        }
        i++;
    } while (i < uint64_t(int64_t(numBanks)));
}

// 0x59A9A0 (thiscall): everything unloaded and freed
void __fastcall Banks_Shutdown(uint8_t* self) {
    Banks_UnloadAll(self);
    for (int a = 0; a < 64; a++) {
        if (areaLists[a].bankCount != 0) {
            Free(areaLists[a].banks);
            Free(areaLists[a].reverbs);
        }
    }
    Free(At<void*>(self, 0x194));
    At<void*>(self, 0x194) = nullptr;
    Free(bankGroups);
    bankGroups = nullptr;
    Free(At<void*>(self, 0x19C));
    At<void*>(self, 0x19C) = nullptr;
    Free(At<void*>(self, 8));
    At<void*>(self, 8) = nullptr;
    Free(At<void*>(self, 0x324));
    At<void*>(self, 0x324) = nullptr;
}

// 0x59AA30 (thiscall): the static banks loaded
uint8_t __fastcall Banks_LoadStatic(uint8_t* self) {
    for (int i = 0; i < numBanks; i++) {
        LoadingScreen_Message("cSCREAMBankManager::LoadStaticBanks()", "LoadManualBank", 0);
        uint8_t type = uint8_t(BanksOf(self)[i].type) & 0x1F;
        if (type == 1) {
            BanksOf(self)[i].manual = uint8_t(BanksOf(self)[i].manual + type);
            Banks_LoadNow(self, i);
        }
    }
    return 1;
}

// ---- the weather, the cow and the triggers -----------------------------------------------------

// 0x59AA90 (thiscall): outside, the weather's bank and its sound (keep: the sound given
// instead), and the feet's banks for the season
void __fastcall Banks_UpdateWeatherImpl(uint8_t* self, re::Edx, int keep, int now) {
    int32_t season = -1;
    int32_t old = At<int32_t>(self, 0x24C);
    int32_t sound = keep;
    if (uint8_t(keep) == 0 && IsOutsideArea(At<int32_t>(self, 0x314)) != 0) {
        season = chapterSeason[chapter];
        int32_t weather = newWeatherType;
        if (forceSnow != 0) {
            At<int32_t>(self, 0x24C) = At<int16_t>(self, 0x2B4 + weather * 8);
            sound = At<int32_t>(self, 0x2B0 + weather * 8);
        } else {
            int index = weather + season * 6;
            At<int32_t>(self, 0x24C) = At<int16_t>(self, 0x254 + index * 8);
            sound = At<int32_t>(self, 0x250 + index * 8);
        }
    }
    if (At<int32_t>(self, 0x24C) != -1) {
        if (At<int32_t>(self, 0x24C) != old && old != -1) {
            Banks_ReleaseTimed(self, old);
        }
        Banks_Request(self, At<int32_t>(self, 0x24C), 5000, 0);
        Bank* banks = BanksOf(self);
        if (banks != nullptr && (banks[Banks_GroupSlot(self, uint16_t(At<int32_t>(self, 0x24C)))].flags & 1) != 0) {
            int volume = int32_t(Audio_Fn59E4F0(1024.0f, 4));
            Sounds_PlayLoop(reinterpret_cast<re::Ptr32<void>*>(self + 0x310), sound, 0, 2, volume, 0, 0);
        }
    }
    if (uint8_t(keep) == 0 && IsOutsideArea(At<int32_t>(self, 0x314)) != 0) {
        switch (season) {
        case 0:
        case 1:
        case 3:
            Banks_Request(self, Banks_Find(soundBanks, "FEET\\FeetExtr.bnk"), 2000, now);
            break;
        case 2:
            Banks_Request(self, Banks_Find(soundBanks, "FEET\\FeetSnow.bnk"), 2000, now);
            Banks_Request(self, Banks_Find(soundBanks, "Weapons\\SnoBall.bnk"), 2000, 0);
            break;
        default:
            break;
        }
    }
}

// 0x59AC30 (thiscall): every tenth update, the cow dance's bank while the player wears
// the outfit
void __fastcall Banks_UpdateCowDanceImpl(uint8_t* self) {
    int32_t r = cowDanceFrames % 10;
    cowDanceFrames++;
    if (r != 0) {
        return;
    }
    int bank = Banks_Find(self, "MISSION\\CowDance.bnk");
    if (Clothing_IsWearingOutfit(clothing, cowOutfit) != 0) {
        Banks_Request(self, bank, 3000, 0);
    }
}

// 0x59ACA0 (thiscall; missing from the function lists, jumped to from 0x59AF90): the
// area's trigger banks: released when the camera leaves their trigger, loaded when it
// enters (not in a fast vehicle, nor in an update that released one)
void __fastcall Banks_UpdateTriggerBanksImpl(uint8_t* self) {
    uint8_t released = 0;
    CVector camera;
    GetCameraPos(&camera);
    for (int i = 0; i < areaLists[At<int32_t>(self, 0x314)].bankCount; i++) {
        TriggerBank* t = &areaLists[At<int32_t>(self, 0x314)].banks[i];
        if (t->hash == 0) {
            break;
        }
        int bank = t->bank;
        if ((BanksOf(self)[bank].flags & 1) == 0) {
            continue;
        }
        void* trigger = TriggerOf(t->hash);
        if (trigger == nullptr) {
            continue;
        }
        if ((areaLists[At<int32_t>(self, 0x314)].banks[i].flags & 1) == 0) {
            continue;
        }
        if (Area_Contains(trigger, &camera) != 0) {
            continue;
        }
        Banks_Unload(self, bank, 1, 1);
        areaLists[At<int32_t>(self, 0x314)].banks[i].flags &= 0xFE;
        released = 1;
    }
    if (released || IsPlayerInFastVehicle() != 0) {
        return;
    }
    for (int i = 0; i < areaLists[At<int32_t>(self, 0x314)].bankCount; i++) {
        TriggerBank* t = &areaLists[At<int32_t>(self, 0x314)].banks[i];
        if (t->hash == 0) {
            return;
        }
        int bank = t->bank;
        if ((BanksOf(self)[bank].flags & 1) != 0) {
            continue;
        }
        void* trigger = TriggerOf(t->hash);
        if (trigger == nullptr) {
            continue;
        }
        if (BanksOf(self)[bank].manual == 0) {
            areaLists[At<int32_t>(self, 0x314)].banks[i].flags &= 0xFE;
        }
        if ((areaLists[At<int32_t>(self, 0x314)].banks[i].flags & 1) != 0) {
            continue;
        }
        if (Area_Contains(trigger, &camera) == 0) {
            continue;
        }
        BanksOf(self)[bank].manual++;
        Banks_LoadAsync(self, bank);
        areaLists[At<int32_t>(self, 0x314)].banks[i].flags |= 1;
    }
}

// 0x59AED0 (thiscall): an area's trigger banks released
void __fastcall Banks_ReleaseAreaImpl(uint8_t* self, re::Edx, uint32_t area) {
    if (area > 0x3F) {
        return;
    }
    for (int i = 0; i < areaLists[area].bankCount; i++) {
        if ((areaLists[area].banks[i].flags & 1) == 0) {
            continue;
        }
        ReleaseManual(self, areaLists[area].banks[i].bank, 1);
        areaLists[area].banks[i].flags &= 0xFE;
    }
}

// ---- the update and the area change ------------------------------------------------------------

// 0x59AF90 (thiscall): the queue's first finished (or given up after 4 s), the weather,
// then with the queue empty the weapons' and timed banks released and the triggers'
void __fastcall Banks_Update(uint8_t* self) {
    if (At<int32_t>(self, 0x314) == 0x40) {
        return;
    }
    Banks_UpdateCowDance(self);
    if (queuedCount != 0) {
        if (self[0x18C] != 0) {
            Banks_LoadAsync(soundBanks, -2);
        }
        int bank = QueueOf(self)[0].bank;
        uint8_t isLoad = QueueOf(self)[0].flags & 1;
        Bank* b = &BanksOf(self)[bank];
        if (isLoad != 0 && (b->flags & 2) != 0) {
            Banks_Loaded(self, bank);
        } else if (isLoad == 0 && (b->flags & 4) != 0) {
            Banks_Unloaded(self, bank);
        } else if (At<uint32_t>(self, 0x10C) + 4000 < m_snTimeInMilliseconds) {
            // ("Probable error- async %s of bank %s(%d) failed to complete within 4 seconds")
            if (QueueOf(self)[0].flags & 1) {
                Banks_Unloaded(self, QueueOf(self)[0].bank);
            }
        }
    }
    Banks_UpdateWeather(self, 0, 0);
    if (QueueCount(self) != 0 || m_snTimeInMilliseconds < 1500) {
        return;
    }
    for (int w = 0; w < At<int32_t>(self, 0x190); w++) {
        WeaponSlot& slot = At<WeaponSlot*>(self, 0x194)[w];
        if ((slot.flags & 1) == 0 || slot.held != 0 || m_snTimeInMilliseconds <= slot.until) {
            continue;
        }
        int bank = Banks_ForWeapon(nullptr, firstWeapon + w);
        if (bank != -1) {
            Banks_Unload(self, bank, 1, 1);
        }
        At<WeaponSlot*>(self, 0x194)[w].flags &= 0xFE;
    }
    for (int i = 0; i < 20; i++) {
        TimedBank& t = TimedOf(self)[i];
        if ((t.flags & 1) == 0 || m_snTimeInMilliseconds <= t.until) {
            continue;
        }
        if (uint16_t(t.bank) != 0xFFFF) {
            ReleaseManual(self, t.bank, 1);
        }
        t.flags &= 0xFE;
    }
    Banks_UpdateTriggerBanks(self);
}

// 0x59B190 (thiscall): the area changed: every bank that isn't static, and loaded or
// held, unloaded; then the force loads, the missions', the area's and the manual ones
// back in
void __fastcall Banks_ChangeArea(uint8_t* self) {
    GetAreaName(At<int32_t>(self, 0x314));   // (for the debug output)
    GetAreaName(currArea);
    if (At<int32_t>(self, 0x314) == currArea) {
        return;
    }
    self[4] = 1;
    ambienceEmitters[0xA8] = 1;
    Banks_ReleaseTimedAll(self);
    Banks_UpdateWeather(self, 1, 0);
    Banks_ReleaseArea(self, At<int32_t>(self, 0x314));
    Banks_ReleaseArea(self, currArea);
    Objects_StopAllAmbientSounds();
    VoiceEngine_Fn5AA3B0(voiceEngine);
    QueueCount(self) = 0;
    // six lists of 250 in the scratch buffer: 0 loaded, 1 area force loads, 2 mission force
    // loads, 3 the area's, 4 the missions', 5 the manual ones
    int32_t counts[6] = {0, 0, 0, 0, 0, 0};
    int32_t* lists[6];
    for (int j = 0; j < 6; j++) {
        lists[j] = bankLists + j * 250;
    }
    for (int i = 0; i < numBanks; i++) {
        Bank& b = BanksOf(self)[i];
        int32_t type = TypeOf(b);
        if (type == 1) {
            continue;
        }
        if ((b.flags & 1) != 0 || b.manual != 0) {
            if (type == 2) {
                lists[4][counts[4]++] = i;
            } else if (type == 5) {
                lists[5][counts[5]++] = i;
            } else if (type == 7) {
                lists[2][counts[2]++] = i;
            }
        }
        int32_t bit = At<int32_t>(self, 0x31C) * currArea + i;
        uint8_t mask = uint8_t(1u << ((bit % 8) & 31));
        if ((uint8_t(BanksOf(self)[i].type) & 0x1F) == 6 && (At<uint8_t*>(self, 0x324)[bit / 8] & mask) != 0) {
            lists[1][counts[1]++] = i;
        }
        bit = At<int32_t>(self, 0x31C) * currArea + i;
        mask = uint8_t(1u << ((bit % 8) & 31));
        if ((uint8_t(BanksOf(self)[i].type) & 0x1F) == 3 && (At<uint8_t*>(self, 0x324)[bit / 8] & mask) != 0) {
            lists[3][counts[3]++] = i;
        }
        if ((BanksOf(self)[i].flags & 1) != 0 || BanksOf(self)[i].slot != -1) {
            lists[0][counts[0]++] = i;
        }
    }
    for (int j = 0; j < counts[0]; j++) {
        int8_t slot = BanksOf(self)[lists[0][j]].slot;
        if (slot != -1) {
            VoiceEngine_Unload(voiceEngine, At<void*>(self, 0x10 + slot * 8));
        }
    }
    for (int j = 0; j < counts[0]; j++) {
        Bank& b = BanksOf(self)[lists[0][j]];
        if (b.slot == -1) {
            continue;
        }
        b.flags &= 0xFE;
        At<int32_t>(self, 0xC + BanksOf(self)[lists[0][j]].slot * 8) = 0;
        At<int32_t>(self, 0x10 + BanksOf(self)[lists[0][j]].slot * 8) = 0;
        BanksOf(self)[lists[0][j]].slot = -1;
        BanksOf(self)[lists[0][j]].manual = 0;
        // ("\n UNloaded Bank: [ %s ]\n\n")
    }
    // (the game then checks the voice slots up to 500 times with nothing changing them)
    for (int tries = 500; tries > 0; tries--) {
        bool busy = false;
        for (int s = 0; s < 32; s++) {
            Bank* b = At<Bank*>(self, 0xC + s * 8);
            if (b != nullptr && (b->flags & 4) == 0 && (uint8_t(b->type) & 0x1F) != 1) {
                busy = true;
            }
        }
        if (!busy) {
            break;
        }
    }
    for (int j = 1; j < 6; j++) {
        for (int k = 0; k < counts[j]; k++) {
            int bank = lists[j][k];
            bool now = j == 1 || j == 2;
            BanksOf(self)[bank].manual++;
            if (now) {
                Banks_LoadNow(self, bank);
            } else {
                Banks_LoadAsync(self, bank);
            }
        }
    }
    At<int32_t>(self, 0x314) = currArea;
    Banks_RefreshTimed(self);
    Banks_UpdateWeather(self, 0, 1);
    ambienceEmitters[0xA8] = 0;
}

// ---- registration -----------------------------------------------------------------------------

#define SOUNDBANKS3_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(soundbanks3_original_, __LINE__);                                              \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(soundbanks3_original_, __LINE__), \
             re::Mode::Original)

SOUNDBANKS3_REVERSED("SoundBanks::WeaponReady", 0x00599F20, &Banks_WeaponReady);
SOUNDBANKS3_REVERSED("SoundBanks::ShouldQueue", 0x00599FD0, &Banks_ShouldQueueImpl);
SOUNDBANKS3_REVERSED("SoundBanks::Queue", 0x0059A130, &Banks_QueueImpl);
SOUNDBANKS3_REVERSED("SoundBanks::UnloadNow", 0x0059A1A0, &Banks_UnloadNowImpl);
SOUNDBANKS3_REVERSED("SoundBanks::LoadAsync", 0x0059A270, &Banks_LoadAsyncImpl);
SOUNDBANKS3_REVERSED("SoundBanks::Load", 0x0059A3E0, &Banks_Load);
SOUNDBANKS3_REVERSED("SoundBanks::Unload", 0x0059A420, &Banks_UnloadImpl);
SOUNDBANKS3_REVERSED("SoundBanks::MissionStarted", 0x0059A490, &Banks_MissionStarted);
SOUNDBANKS3_REVERSED("SoundBanks::MissionEnded", 0x0059A600, &Banks_MissionEnded);
SOUNDBANKS3_REVERSED("SoundBanks::RefreshTimed", 0x0059A6E0, &Banks_RefreshTimedImpl);
SOUNDBANKS3_REVERSED("SoundBanks::ReleaseTimedAll", 0x0059A740, &Banks_ReleaseTimedAllImpl);
SOUNDBANKS3_REVERSED("SoundBanks::ReleaseTimed", 0x0059A7D0, &Banks_ReleaseTimedImpl);
SOUNDBANKS3_REVERSED("SoundBanks::Request", 0x0059A860, &Banks_RequestImpl);
SOUNDBANKS3_REVERSED("SoundBanks::UnloadAll", 0x0059A950, &Banks_UnloadAllImpl);
SOUNDBANKS3_REVERSED("SoundBanks::Shutdown", 0x0059A9A0, &Banks_Shutdown);
SOUNDBANKS3_REVERSED("SoundBanks::LoadStatic", 0x0059AA30, &Banks_LoadStatic);
SOUNDBANKS3_REVERSED("SoundBanks::UpdateWeather", 0x0059AA90, &Banks_UpdateWeatherImpl);
SOUNDBANKS3_REVERSED("SoundBanks::UpdateCowDance", 0x0059AC30, &Banks_UpdateCowDanceImpl);
SOUNDBANKS3_REVERSED("SoundBanks::UpdateTriggerBanks", 0x0059ACA0, &Banks_UpdateTriggerBanksImpl);
SOUNDBANKS3_REVERSED("SoundBanks::ReleaseArea", 0x0059AED0, &Banks_ReleaseAreaImpl);
SOUNDBANKS3_REVERSED("SoundBanks::Update", 0x0059AF90, &Banks_Update);
SOUNDBANKS3_REVERSED("SoundBanks::ChangeArea", 0x0059B190, &Banks_ChangeArea);

} // namespace
